/** 
 * @file llurlrequest.cpp
 * @author Phoenix
 * @date 2005-04-28
 * @brief Implementation of the URLRequest class and related classes.
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2008, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "linden_common.h"
#include "llurlrequest.h"

#include <curl/curl.h>
#include <algorithm>

#include "llioutil.h"
#include "llmemtype.h"
#include "llpumpio.h"
#include "llsd.h"
#include "llstring.h"
#include "apr-1/apr_env.h"

static const U32 HTTP_STATUS_PIPE_ERROR = 499;

/**
 * String constants
 */
const std::string CONTEXT_DEST_URI_SD_LABEL("dest_uri");


static
size_t headerCallback(void* data, size_t size, size_t nmemb, void* user);

/**
 * class LLURLRequestDetail
 */
class LLURLRequestDetail
{
public:
	LLURLRequestDetail();
	~LLURLRequestDetail();
	CURLM* mCurlMulti;
 	CURL* mCurl;
	struct curl_slist* mHeaders;
	char* mURL;
	char mCurlErrorBuf[CURL_ERROR_SIZE + 1];		/* Flawfinder: ignore */
	bool mNeedToRemoveEasyHandle;
	LLBufferArray* mResponseBuffer;
	LLChannelDescriptors mChannels;
	U8* mLastRead;
	U32 mBodyLimit;
	bool mIsBodyLimitSet;
};

LLURLRequestDetail::LLURLRequestDetail() :
	mCurlMulti(NULL),
	mCurl(NULL),
	mHeaders(NULL),
	mURL(NULL),
	mNeedToRemoveEasyHandle(false),
	mResponseBuffer(NULL),
	mLastRead(NULL),
	mBodyLimit(0),
	mIsBodyLimitSet(false)
	
{
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
	mCurlErrorBuf[0] = '\0';
}

LLURLRequestDetail::~LLURLRequestDetail()
{
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
	if(mCurl)
	{
		if(mNeedToRemoveEasyHandle && mCurlMulti)
		{
			curl_multi_remove_handle(mCurlMulti, mCurl);
			mNeedToRemoveEasyHandle = false;
		}
		curl_easy_cleanup(mCurl);
		mCurl = NULL;
	}
	if(mCurlMulti)
	{
		curl_multi_cleanup(mCurlMulti);
		mCurlMulti = NULL;
	}
	if(mHeaders)
	{
		curl_slist_free_all(mHeaders);
		mHeaders = NULL;
	}
	delete[] mURL;
	mURL = NULL;
	mResponseBuffer = NULL;
	mLastRead = NULL;
}


/**
 * class LLURLRequest
 */

static std::string sCAFile("");
static std::string sCAPath("");

LLURLRequest::LLURLRequest(LLURLRequest::ERequestAction action) :
	mAction(action)
{
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
	initialize();
}

LLURLRequest::LLURLRequest(
	LLURLRequest::ERequestAction action,
	const std::string& url) :
	mAction(action)
{
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
	initialize();
	setURL(url);
}

LLURLRequest::~LLURLRequest()
{
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
	delete mDetail;
}

void LLURLRequest::setURL(const std::string& url)
{
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
	if(mDetail->mURL)
	{
		// *NOTE: if any calls to set the url have been made to curl,
		// this will probably lead to a crash.
		delete[] mDetail->mURL;
		mDetail->mURL = NULL;
	}
	if(!url.empty())
	{
		mDetail->mURL = new char[url.size() + 1];
		url.copy(mDetail->mURL, url.size());
		mDetail->mURL[url.size()] = '\0';
	}
}

void LLURLRequest::addHeader(const char* header)
{
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
	mDetail->mHeaders = curl_slist_append(mDetail->mHeaders, header);
}

void LLURLRequest::requestEncoding(const char* encoding)
{
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
	curl_easy_setopt(mDetail->mCurl, CURLOPT_ENCODING, encoding);
}

void LLURLRequest::setBodyLimit(U32 size)
{
	mDetail->mBodyLimit = size;
	mDetail->mIsBodyLimitSet = true;
}

void LLURLRequest::checkRootCertificate(bool check, const char* caBundle)
{
	curl_easy_setopt(mDetail->mCurl, CURLOPT_SSL_VERIFYPEER, (check? TRUE : FALSE));
	if (caBundle)
	{
		curl_easy_setopt(mDetail->mCurl, CURLOPT_CAINFO, caBundle);
	}
}

void LLURLRequest::setCallback(LLURLRequestComplete* callback)
{
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
	mCompletionCallback = callback;

	curl_easy_setopt(mDetail->mCurl, CURLOPT_HEADERFUNCTION, &headerCallback);
	curl_easy_setopt(mDetail->mCurl, CURLOPT_WRITEHEADER, callback);
}

// Added to mitigate the effect of libcurl looking
// for the ALL_PROXY and http_proxy env variables
// and deciding to insert a Pragma: no-cache
// header! The only usage of this method at the
// time of this writing is in llhttpclient.cpp
// in the request() method, where this method
// is called with use_proxy = FALSE
void LLURLRequest::useProxy(bool use_proxy)
{
    static char *env_proxy;

    if (use_proxy && (env_proxy == NULL))
    {
        apr_status_t status;
        apr_pool_t* pool;
        apr_pool_create(&pool, NULL);
        status = apr_env_get(&env_proxy, "ALL_PROXY", pool);
        if (status != APR_SUCCESS)
        {
            status = apr_env_get(&env_proxy, "http_proxy", pool);
        }
        if (status != APR_SUCCESS)
        {
           use_proxy = FALSE;
        }
        apr_pool_destroy(pool);
    }


    lldebugs << "use_proxy = " << (use_proxy?'Y':'N') << ", env_proxy = " << env_proxy << llendl;

    if (env_proxy && use_proxy)
    {
        curl_easy_setopt(mDetail->mCurl, CURLOPT_PROXY, env_proxy);
    }
    else
    {
        curl_easy_setopt(mDetail->mCurl, CURLOPT_PROXY, "");
    }
}

// virtual
LLIOPipe::EStatus LLURLRequest::handleError(
	LLIOPipe::EStatus status,
	LLPumpIO* pump)
{
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
	if(mCompletionCallback && pump)
	{
		LLURLRequestComplete* complete = NULL;
		complete = (LLURLRequestComplete*)mCompletionCallback.get();
		complete->httpStatus(
			HTTP_STATUS_PIPE_ERROR,
			LLIOPipe::lookupStatusString(status));
		complete->responseStatus(status);
		pump->respond(complete);
		mCompletionCallback = NULL;
	}
	return status;
}

// virtual
LLIOPipe::EStatus LLURLRequest::process_impl(
	const LLChannelDescriptors& channels,
	buffer_ptr_t& buffer,
	bool& eos,
	LLSD& context,
	LLPumpIO* pump)
{
	PUMP_DEBUG;
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
	//llinfos << "LLURLRequest::process_impl()" << llendl;
	if(!buffer) return STATUS_ERROR;
	switch(mState)
	{
	case STATE_INITIALIZED:
	{
		PUMP_DEBUG;
		// We only need to wait for input if we are uploading
		// something.
		if(((HTTP_PUT == mAction) || (HTTP_POST == mAction)) && !eos)
		{
			// we're waiting to get all of the information
			return STATUS_BREAK;
		}

		// *FIX: bit of a hack, but it should work. The configure and
		// callback method expect this information to be ready.
		mDetail->mResponseBuffer = buffer.get();
		mDetail->mChannels = channels;
		if(!configure())
		{
			return STATUS_ERROR;
		}
		mState = STATE_WAITING_FOR_RESPONSE;

		// *FIX: Maybe we should just go to the next state now...
		return STATUS_BREAK;
	}
	case STATE_WAITING_FOR_RESPONSE:
	case STATE_PROCESSING_RESPONSE:
	{
		PUMP_DEBUG;
		const S32 MAX_CALLS = 5;
		S32 count = MAX_CALLS;
		CURLMcode code;
		LLIOPipe::EStatus status = STATUS_BREAK;
		S32 queue;
		do
		{
			LLFastTimer t2(LLFastTimer::FTM_CURL);
			code = curl_multi_perform(mDetail->mCurlMulti, &queue);			
		}while((CURLM_CALL_MULTI_PERFORM == code) && (queue > 0) && count--);
		CURLMsg* curl_msg;
		do
		{
			curl_msg = curl_multi_info_read(mDetail->mCurlMulti, &queue);
			if(curl_msg && (curl_msg->msg == CURLMSG_DONE))
			{
				mState = STATE_HAVE_RESPONSE;

				CURLcode result = curl_msg->data.result;
				switch(result)
				{
				case CURLE_OK:
				case CURLE_WRITE_ERROR:
					// NB: The error indication means that we stopped the
					// writing due the body limit being reached
					if(mCompletionCallback && pump)
					{
						LLURLRequestComplete* complete = NULL;
						complete = (LLURLRequestComplete*)
							mCompletionCallback.get();
						complete->responseStatus(
								result == CURLE_OK
									? STATUS_OK : STATUS_STOP);
						LLPumpIO::links_t chain;
						LLPumpIO::LLLinkInfo link;
						link.mPipe = mCompletionCallback;
						link.mChannels = LLBufferArray::makeChannelConsumer(
							channels);
						chain.push_back(link);
						pump->respond(chain, buffer, context);
						mCompletionCallback = NULL;
					}
					break;
				case CURLE_COULDNT_CONNECT:
					status = STATUS_NO_CONNECTION;
					break;
				default:
					llwarns << "URLRequest Error: " << curl_msg->data.result
							<< ", "
#if LL_DARWIN
							// curl_easy_strerror was added in libcurl 7.12.0.  Unfortunately, the version in the Mac OS X 10.3.9 SDK is 7.10.2...
							// There's a problem with the custom curl headers in our build that keeps me from #ifdefing this on the libcurl version number
							// (the correct check would be #if LIBCURL_VERSION_NUM >= 0x070c00).  We'll fix the header problem soon, but for now
							// just punt and print the numeric error code on the Mac.
							<< curl_msg->data.result
#else // LL_DARWIN
							<< curl_easy_strerror(curl_msg->data.result)
#endif // LL_DARWIN
							<< ", "
							<< (mDetail->mURL ? mDetail->mURL : "<EMPTY URL>")
							<< llendl;
					status = STATUS_ERROR;
					break;
				}
				curl_multi_remove_handle(mDetail->mCurlMulti, mDetail->mCurl);
				mDetail->mNeedToRemoveEasyHandle = false;
			}
		}while(curl_msg && (queue > 0));
		return status;
	}
	case STATE_HAVE_RESPONSE:
		PUMP_DEBUG;
		// we already stuffed everything into channel in in the curl
		// callback, so we are done.
		eos = true;
		return STATUS_DONE;

	default:
		PUMP_DEBUG;
		return STATUS_ERROR;
	}
}

void LLURLRequest::initialize()
{
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
	mState = STATE_INITIALIZED;
	mDetail = new LLURLRequestDetail;
	mDetail->mCurl = curl_easy_init();
	mDetail->mCurlMulti = curl_multi_init();
	curl_easy_setopt(mDetail->mCurl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(mDetail->mCurl, CURLOPT_WRITEFUNCTION, &downCallback);
	curl_easy_setopt(mDetail->mCurl, CURLOPT_WRITEDATA, this);
	curl_easy_setopt(mDetail->mCurl, CURLOPT_READFUNCTION, &upCallback);
	curl_easy_setopt(mDetail->mCurl, CURLOPT_READDATA, this);
	curl_easy_setopt(
		mDetail->mCurl,
		CURLOPT_ERRORBUFFER,
		mDetail->mCurlErrorBuf);

	if(sCAPath != std::string(""))
	{
		curl_easy_setopt(mDetail->mCurl, CURLOPT_CAPATH, sCAPath.c_str());
	}
	if(sCAFile != std::string(""))
	{
		curl_easy_setopt(mDetail->mCurl, CURLOPT_CAINFO, sCAFile.c_str());
	}
}

bool LLURLRequest::configure()
{
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
	bool rv = false;
	S32 bytes = mDetail->mResponseBuffer->countAfter(
   		mDetail->mChannels.in(),
		NULL);
	switch(mAction)
	{
	case HTTP_HEAD:
		curl_easy_setopt(mDetail->mCurl, CURLOPT_HEADER, 1);
		curl_easy_setopt(mDetail->mCurl, CURLOPT_NOBODY, 1);
		curl_easy_setopt(mDetail->mCurl, CURLOPT_FOLLOWLOCATION, 1);
		rv = true;
		break;

	case HTTP_GET:
		curl_easy_setopt(mDetail->mCurl, CURLOPT_HTTPGET, 1);
		curl_easy_setopt(mDetail->mCurl, CURLOPT_FOLLOWLOCATION, 1);
		rv = true;
		break;

	case HTTP_PUT:
		// Disable the expect http 1.1 extension. POST and PUT default
		// to turning this on, and I am not too sure what it means.
		addHeader("Expect:");

		curl_easy_setopt(mDetail->mCurl, CURLOPT_UPLOAD, 1);
		curl_easy_setopt(mDetail->mCurl, CURLOPT_INFILESIZE, bytes);
		rv = true;
		break;

	case HTTP_POST:
		// Disable the expect http 1.1 extension. POST and PUT default
		// to turning this on, and I am not too sure what it means.
		addHeader("Expect:");

		// Disable the content type http header.
		// *FIX: what should it be?
		addHeader("Content-Type:");

		// Set the handle for an http post
		curl_easy_setopt(mDetail->mCurl, CURLOPT_POST, 1);
		curl_easy_setopt(mDetail->mCurl, CURLOPT_POSTFIELDS, NULL);
		curl_easy_setopt(mDetail->mCurl, CURLOPT_POSTFIELDSIZE, bytes);
		rv = true;
		break;

	case HTTP_DELETE:
		// Set the handle for an http post
		curl_easy_setopt(mDetail->mCurl, CURLOPT_CUSTOMREQUEST, "DELETE");
		rv = true;
		break;

	default:
		llwarns << "Unhandled URLRequest action: " << mAction << llendl;
		break;
	}
	if(rv)
	{
		if(mDetail->mHeaders)
		{
			curl_easy_setopt(
				mDetail->mCurl,
				CURLOPT_HTTPHEADER,
				mDetail->mHeaders);
		}
		curl_easy_setopt(mDetail->mCurl, CURLOPT_URL, mDetail->mURL);
		lldebugs << "URL: " << mDetail->mURL << llendl;
		curl_multi_add_handle(mDetail->mCurlMulti, mDetail->mCurl);
		mDetail->mNeedToRemoveEasyHandle = true;
	}
	return rv;
}

// static
size_t LLURLRequest::downCallback(
	void* data,
	size_t size,
	size_t nmemb,
	void* user)
{
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
	LLURLRequest* req = (LLURLRequest*)user;
	if(STATE_WAITING_FOR_RESPONSE == req->mState)
	{
		req->mState = STATE_PROCESSING_RESPONSE;
	}
	U32 bytes = size * nmemb;
	if (req->mDetail->mIsBodyLimitSet)
	{
		if (bytes > req->mDetail->mBodyLimit)
		{
			bytes = req->mDetail->mBodyLimit;
			req->mDetail->mBodyLimit = 0;
		}
		else
		{
			req->mDetail->mBodyLimit -= bytes;
		}
	}

	req->mDetail->mResponseBuffer->append(
		req->mDetail->mChannels.out(),
		(U8*)data,
		bytes);
	return bytes;
}

// static
size_t LLURLRequest::upCallback(
	void* data,
	size_t size,
	size_t nmemb,
	void* user)
{
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
	LLURLRequest* req = (LLURLRequest*)user;
	S32 bytes = llmin(
		(S32)(size * nmemb),
		req->mDetail->mResponseBuffer->countAfter(
			req->mDetail->mChannels.in(),
			req->mDetail->mLastRead));
	req->mDetail->mLastRead =  req->mDetail->mResponseBuffer->readAfter(
		req->mDetail->mChannels.in(),
		req->mDetail->mLastRead,
		(U8*)data,
		bytes);
	return bytes;
}

static
size_t headerCallback(void* data, size_t size, size_t nmemb, void* user)
{
	const char* header_line = (const char*)data;
	size_t header_len = size * nmemb;
	LLURLRequestComplete* complete = (LLURLRequestComplete*)user;

	if (!complete || !header_line)
	{
		return header_len;
	}

	// *TODO: This should be a utility in llstring.h: isascii()
	for (size_t i = 0; i < header_len; ++i)
	{
		if (header_line[i] < 0)
		{
			return header_len;
		}
	}

	std::string header(header_line, header_len);

	// Per HTTP spec the first header line must be the status line.
	if (!complete->haveHTTPStatus())
	{
		std::string::iterator end = header.end();
		std::string::iterator pos1 = std::find(header.begin(), end, ' ');
		if (pos1 != end) ++pos1;
		std::string::iterator pos2 = std::find(pos1, end, ' ');
		if (pos2 != end) ++pos2;
		std::string::iterator pos3 = std::find(pos2, end, '\r');

		std::string version(header.begin(), pos1);
		std::string status(pos1, pos2);
		std::string reason(pos2, pos3);

		int statusCode = atoi(status.c_str());
		if (statusCode > 0)
		{
			complete->httpStatus((U32)statusCode, reason);
		}
		else
		{
			llwarns << "Unable to parse http response status line: "
					<< header << llendl;
			complete->httpStatus(499,"Unable to parse status line.");
		}
		return header_len;
	}

	std::string::iterator sep = std::find(header.begin(),header.end(),':');

	if (sep != header.end())
	{
		std::string key(header.begin(), sep);
		std::string value(sep + 1, header.end());

		key = utf8str_tolower(utf8str_trim(key));
		value = utf8str_trim(value);

		complete->header(key, value);
	}
	else
	{
		LLString::trim(header);
		if (!header.empty())
		{
			llwarns << "Unable to parse header: " << header << llendl;
		}
	}

	return header_len;
}

//static 
void LLURLRequest::setCertificateAuthorityFile(const std::string& file_name)
{
	sCAFile = file_name;
}

//static 
void LLURLRequest::setCertificateAuthorityPath(const std::string& path)
{
	sCAPath = path;
}

/**
 * LLContextURLExtractor
 */
// virtual
LLIOPipe::EStatus LLContextURLExtractor::process_impl(
	const LLChannelDescriptors& channels,
	buffer_ptr_t& buffer,
	bool& eos,
	LLSD& context,
	LLPumpIO* pump)
{
	PUMP_DEBUG;
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
	// The destination host is in the context.
	if(context.isUndefined() || !mRequest)
	{
		return STATUS_PRECONDITION_NOT_MET;
	}

	// copy in to out, since this just extract the URL and does not
	// actually change the data.
	LLChangeChannel change(channels.in(), channels.out());
	std::for_each(buffer->beginSegment(), buffer->endSegment(), change);

	// find the context url
	if(context.has(CONTEXT_DEST_URI_SD_LABEL))
	{
		mRequest->setURL(context[CONTEXT_DEST_URI_SD_LABEL]);
		return STATUS_DONE;
	}
	return STATUS_ERROR;
}


/**
 * LLURLRequestComplete
 */
LLURLRequestComplete::LLURLRequestComplete() :
	mRequestStatus(LLIOPipe::STATUS_ERROR),
	mHaveHTTPStatus(false)
{
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
}

// virtual
LLURLRequestComplete::~LLURLRequestComplete()
{
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
}

//virtual 
void LLURLRequestComplete::header(const std::string& header, const std::string& value)
{
}

//virtual 
void LLURLRequestComplete::httpStatus(U32 status, const std::string& reason)
{
	mHaveHTTPStatus = true;
}

//virtual 
void LLURLRequestComplete::complete(const LLChannelDescriptors& channels,
		const buffer_ptr_t& buffer)
{
	if(STATUS_OK == mRequestStatus)
	{
		response(channels, buffer);
	}
	else
	{
		noResponse();
	}
}

//virtual 
void LLURLRequestComplete::response(const LLChannelDescriptors& channels,
		const buffer_ptr_t& buffer)
{
	llwarns << "LLURLRequestComplete::response default implementation called"
		<< llendl;
}

//virtual 
void LLURLRequestComplete::noResponse()
{
	llwarns << "LLURLRequestComplete::noResponse default implementation called"
		<< llendl;
}

void LLURLRequestComplete::responseStatus(LLIOPipe::EStatus status)
{
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
	mRequestStatus = status;
}

// virtual
LLIOPipe::EStatus LLURLRequestComplete::process_impl(
	const LLChannelDescriptors& channels,
	buffer_ptr_t& buffer,
	bool& eos,
	LLSD& context,
	LLPumpIO* pump)
{
	PUMP_DEBUG;
	complete(channels, buffer);
	return STATUS_OK;
}
