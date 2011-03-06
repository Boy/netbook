/**
 * @file llpaneldirfind.cpp
 * @brief The "All" panel in the Search directory.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2008, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llpaneldirfind.h"

// linden library includes
#include "llclassifiedflags.h"
#include "llfontgl.h"
#include "llparcel.h"
#include "llqueryflags.h"
#include "message.h"

// viewer project includes
#include "llagent.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "lllineeditor.h"
#include "llcombobox.h"
#include "llviewercontrol.h"
#include "llmenucommands.h"
#include "llmenugl.h"
#include "lltextbox.h"
#include "lluiconstants.h"
#include "llviewerimagelist.h"
#include "llviewermessage.h"
#include "llfloateravatarinfo.h"
#include "lldir.h"
#include "llviewercontrol.h"
#include "llviewerregion.h"		// for region name for search urls
#include "llvieweruictrlfactory.h"
#include "llfloaterdirectory.h"
#include "llpaneldirbrowser.h"
#include "llnotify.h"

#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

//MK
extern BOOL RRenabled;
//mk

//---------------------------------------------------------------------------
// LLPanelDirFindAll - Google search appliance based search
//---------------------------------------------------------------------------

class LLPanelDirFindAll
:	public LLPanelDirBrowser,
	public LLWebBrowserCtrlObserver
{
public:
	LLPanelDirFindAll(const std::string& name, LLFloaterDirectory* floater);
	/*virtual*/ ~LLPanelDirFindAll();

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void draw();

	void search(const std::string& search_text);
	void focus();
	void navigateToDefaultPage();

private:
	static void onClickBack( void* data );
	static void onClickForward( void* data );
	static void onClickHome( void* data );
	static void onClickSearch( void* data );
	static void onCommitSearch(LLUICtrl*, void* data);

	static std::string buildSearchURL(const std::string& search_text, const std::string& collection, bool inc_pg, bool inc_mature, bool inc_adult);
	static std::string getSearchURLSuffix(bool inc_pg, bool inc_mature, bool inc_adult);

	/*virtual*/ void onNavigateBegin( const EventType& eventIn );
	/*virtual*/ void onNavigateComplete( const EventType& eventIn );
		// Used to update progress indicator
	/*virtual*/ void onLocationChange( const EventType& eventIn );
		// Debugging info to console

private:
#if LL_LIBXUL_ENABLED
	LLWebBrowserCtrl* mWebBrowser;
#endif // LL_LIBXUL_ENABLED
};


LLPanelDirFindAll::LLPanelDirFindAll(const std::string& name, LLFloaterDirectory* floater)
:	LLPanelDirBrowser(name, floater)
#if LL_LIBXUL_ENABLED
	,mWebBrowser(NULL)
#endif // LL_LIBXUL_ENABLED
{
	mMinSearchChars = 3;
}

BOOL LLPanelDirFindAll::postBuild()
{
	LLPanelDirBrowser::postBuild();

	childSetAction("back_btn", onClickBack, this);
	childSetAction("home_btn", onClickHome, this);
	childSetAction("forward_btn", onClickForward, this);
	childSetCommitCallback("search_editor", onCommitSearch, this);
	childSetAction("search_btn", onClickSearch, this);

	// showcase doesn't have maturity flags -- it's all PG
	if (hasChild("incmature"))
	{
		// Teens don't get mature checkbox
		if (gAgent.wantsPGOnly())
		{
			childSetValue("incmature", FALSE);
			childSetValue("incadult", FALSE);
			childHide("incmature");
			childHide("incadult");
			childSetValue("incpg", TRUE);
			childDisable("incpg");
		}		
		
		if (!gAgent.canAccessMature())
		{
			childSetValue("incmature", FALSE);
			childDisable("incmature");
		}
		
		if (!gAgent.canAccessAdult())
		{
			childSetValue("incadult", FALSE);
			childDisable("incadult");
		}
	}

#if LL_LIBXUL_ENABLED
	mWebBrowser = LLViewerUICtrlFactory::getWebBrowserByName(this, "find_browser");
	if (mWebBrowser)
	{
		// new pages appear in same window as the results page now
		mWebBrowser->setOpenInInternalBrowser( false );
		mWebBrowser->setOpenInExternalBrowser( false );	

		// need to handle secondlife:///app/ URLs for direct teleports
		mWebBrowser->setOpenAppSLURLs( true );

		// redirect 404 pages from S3 somewhere else
		mWebBrowser->set404RedirectUrl( childGetText("redirect_404_url") );
		
		// Track updates for progress display.
		mWebBrowser->addObserver(this);

		navigateToDefaultPage();
	}
#endif // LL_LIBXUL_ENABLED

	return TRUE;
}

LLPanelDirFindAll::~LLPanelDirFindAll()
{
#if LL_LIBXUL_ENABLED
	if (mWebBrowser) mWebBrowser->remObserver(this);
#endif // LL_LIBXUL_ENABLED
}

// virtual
void LLPanelDirFindAll::draw()
{
	// enable/disable buttons depending on state
#if LL_LIBXUL_ENABLED
	if ( mWebBrowser )
	{
		bool enable_back = mWebBrowser->canNavigateBack();	
		childSetEnabled( "back_btn", enable_back );

		bool enable_forward = mWebBrowser->canNavigateForward();	
		childSetEnabled( "forward_btn", enable_forward );
	}
#endif // LL_LIBXUL_ENABLED

	// showcase doesn't have maturity flags -- it's all PG
	if (hasChild("incmature"))
	{
		updateMaturityCheckbox();
	}

	LLPanelDirBrowser::draw();
}

void LLPanelDirFindAll::search(const std::string& search_text)
{
	BOOL inc_pg = childGetValue("incpg").asBoolean();
	BOOL inc_mature = childGetValue("incmature").asBoolean();
	BOOL inc_adult = childGetValue("incadult").asBoolean();
	if (!(inc_pg || inc_mature || inc_adult))
	{
		LLNotifyBox::showXml("NoContentToSearch");
		return;
	}
	
	if (!search_text.empty())
	{
		std::string selected_collection = childGetValue( "Category" ).asString();
		std::string url = buildSearchURL(search_text, selected_collection, inc_pg, inc_mature, inc_adult);
#if LL_LIBXUL_ENABLED
		if (mWebBrowser)
		{
			mWebBrowser->navigateTo(url);
		}
#endif // LL_LIBXUL_ENABLED
	}
	else
	{
		// empty search text
		navigateToDefaultPage();
	}

	childSetText("search_editor", search_text);
}

void LLPanelDirFindAll::focus()
{
	childSetFocus("search_editor");
}

void LLPanelDirFindAll::navigateToDefaultPage()
{
	std::string start_url = gSavedSettings.getString("SearchURLDefault");
	BOOL inc_pg = childGetValue("incpg").asBoolean();
	BOOL inc_mature = childGetValue("incmature").asBoolean();
	BOOL inc_adult = childGetValue("incadult").asBoolean();
	if (!(inc_pg || inc_mature || inc_adult))
	{
		// if nothing's checked, just go for pg; we don't notify in
		// this case because it's a default page.
		inc_pg = true;
	}

	start_url += getSearchURLSuffix(inc_pg, inc_mature, inc_adult);
//MK
	if (!RRenabled || !gAgent.mRRInterface.mContainsShowloc)
	{
//mk
		llinfos << "default url: "  << start_url << llendl;
//MK
	}
//mk
#if LL_LIBXUL_ENABLED
	if (mWebBrowser)
	{
		mWebBrowser->navigateTo( start_url );
	}
#endif //LL_LIBXUL_ENABLED
}

// static
std::string LLPanelDirFindAll::buildSearchURL(const std::string& search_text, const std::string& collection, 
										   bool inc_pg, bool inc_mature, bool inc_adult)
{
	std::string url = gSavedSettings.getString("SearchURLDefault");
	if (!search_text.empty())
	{
		// Replace spaces with "+" for use by Google search appliance
		// Yes, this actually works for double-spaces
		// " foo  bar" becomes "+foo++bar" and works fine. JC
		std::string search_text_with_plus = search_text;
		std::string::iterator it = search_text_with_plus.begin();
		for ( ; it != search_text_with_plus.end(); ++it )
		{
			if ( std::isspace( *it ) )
			{
				*it = '+';
			}
		}

		// Our own special set of allowed chars (RFC1738 http://www.ietf.org/rfc/rfc1738.txt)
		// Note that "+" is one of them, so we can do "+" addition first.
		const char* allowed =   
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
			"0123456789"
			"-._~$+!*'()";
		std::string query = LLURI::escape(search_text_with_plus, allowed);

		url = gSavedSettings.getString("SearchURLQuery");
		std::string substring = "[QUERY]";
		std::string::size_type where = url.find(substring);
		if (where != std::string::npos)
		{
			url.replace(where, substring.length(), query);
		}

		// replace the collection name with the one selected from the combo box
		// std::string selected_collection = childGetValue( "Category" ).asString();
		substring = "[COLLECTION]";
		where = url.find(substring);
		if (where != std::string::npos)
		{
			url.replace(where, substring.length(), collection);
		}

	}
	url += getSearchURLSuffix(inc_pg, inc_mature, inc_adult);
//MK
	if (!RRenabled || !gAgent.mRRInterface.mContainsShowloc)
	{
//mk
		llinfos << "search url " << url << llendl;
//MK
	}
//mk
	return url;
}

// static
std::string LLPanelDirFindAll::getSearchURLSuffix(bool inc_pg, bool inc_mature, bool inc_adult)
{
	std::string url = gSavedSettings.getString("SearchURLSuffix2");

	// if the mature checkbox is unchecked, modify query to remove 
	// terms with given phrase from the result set
	// This builds a value from 1-7 by or-ing together the flags, and then converts
	// it to a string. 
	std::string substring="[MATURITY]";
	S32 maturityFlag = 
		(inc_pg ? SEARCH_PG : SEARCH_NONE) |
		(inc_mature ? SEARCH_MATURE : SEARCH_NONE) |
		(inc_adult ? SEARCH_ADULT : SEARCH_NONE);
	url.replace(url.find(substring), substring.length(), boost::lexical_cast<std::string>(maturityFlag));
	
	// Include region and x/y position, not for the GSA, but
	// just to get logs on the web server for search_proxy.php
	// showing where people were standing when they searched.
	std::string region_name;
	LLViewerRegion* region = gAgent.getRegion();
	if (region)
	{
		region_name = region->getName();
	}
	// take care of spaces in names
	region_name = LLURI::escape(region_name);
	substring = "[REGION]";
	url.replace(url.find(substring), substring.length(), region_name);

	LLVector3 pos_region = gAgent.getPositionAgent();

	std::string x = llformat("%.0f", pos_region.mV[VX]);
	substring = "[X]";
	url.replace(url.find(substring), substring.length(), x);
	std::string y = llformat("%.0f", pos_region.mV[VY]);
	substring = "[Y]";
	url.replace(url.find(substring), substring.length(), y);
	std::string z = llformat("%.0f", pos_region.mV[VZ]);
	substring = "[Z]";
	url.replace(url.find(substring), substring.length(), z);

	LLUUID session_id = gAgent.getSessionID();
	std::string session_string = session_id.getString();
	substring = "[SESSION]";
	url.replace(url.find(substring), substring.length(), session_string);

	// and set the flag for the teen grid
	std::string teen_string = gAgent.isTeen() ? "y" : "n";
	std::string teen_tag = "[TEEN]";
	url.replace( url.find( teen_tag ), teen_tag.length(), teen_string );	
	
	return url;
}


// static
void LLPanelDirFindAll::onClickBack( void* data )
{
#if LL_LIBXUL_ENABLED
	LLPanelDirFindAll* self = ( LLPanelDirFindAll* )data;
	if ( self->mWebBrowser )
	{
		self->mWebBrowser->navigateBack();
	}
#endif // LL_LIBXUL_ENABLED
}

// static
void LLPanelDirFindAll::onClickForward( void* data )
{
#if LL_LIBXUL_ENABLED
	LLPanelDirFindAll* self = ( LLPanelDirFindAll* )data;
	if ( self->mWebBrowser )
	{
		self->mWebBrowser->navigateForward();
	}
#endif // LL_LIBXUL_ENABLED
}

// static
void LLPanelDirFindAll::onClickHome( void* data )
{
#if LL_LIBXUL_ENABLED
	LLPanelDirFindAll* self = ( LLPanelDirFindAll* )data;
	if ( self->mWebBrowser )
	{
		self->mWebBrowser->navigateHome();
	}
#endif // LL_LIBXUL_ENABLED
}

// static
void LLPanelDirFindAll::onCommitSearch(LLUICtrl*, void* data)
{
	onClickSearch(data);
}

// static
void LLPanelDirFindAll::onClickSearch(void* data)
{
	LLPanelDirFindAll* self = ( LLPanelDirFindAll* )data;
	LLString search_text = self->childGetText("search_editor");
	self->search(search_text);

	LLFloaterDirectory::sNewSearchCount++;
}

void LLPanelDirFindAll::onNavigateBegin( const EventType& eventIn )
{
	childSetText("status_text", childGetText("loading_text"));
}

void LLPanelDirFindAll::onNavigateComplete( const EventType& eventIn )
{
	childSetText("status_text", childGetText("done_text"));
}

void LLPanelDirFindAll::onLocationChange( const EventType& eventIn )
{
//MK
	if (!RRenabled || !gAgent.mRRInterface.mContainsShowloc)
	{
//mk
		llinfos << eventIn.getStringValue() << llendl;
//MK
	}
//mk
}

//---------------------------------------------------------------------------
// LLPanelDirFindAllInterface
//---------------------------------------------------------------------------

// static
LLPanelDirFindAll* LLPanelDirFindAllInterface::create(LLFloaterDirectory* floater)
{
	return new LLPanelDirFindAll("find_all_panel", floater);
}

// static
void LLPanelDirFindAllInterface::search(LLPanelDirFindAll* panel,
										const std::string& search_text)
{
	panel->search(search_text);
}

// static
void LLPanelDirFindAllInterface::focus(LLPanelDirFindAll* panel)
{
	panel->focus();
}

//---------------------------------------------------------------------------
// LLPanelDirFindAllOld - deprecated if new Google search works out. JC
//---------------------------------------------------------------------------

LLPanelDirFindAllOld::LLPanelDirFindAllOld(const std::string& name, LLFloaterDirectory* floater)
	:	LLPanelDirBrowser(name, floater)
{
	mMinSearchChars = 3;
}

BOOL LLPanelDirFindAllOld::postBuild()
{
	LLPanelDirBrowser::postBuild();

	childSetKeystrokeCallback("name", &LLPanelDirBrowser::onKeystrokeName, this);

	childSetAction("Search", onClickSearch, this);
	childDisable("Search");
	setDefaultBtn( "Search" );

	return TRUE;
}

LLPanelDirFindAllOld::~LLPanelDirFindAllOld()
{
	// Children all cleaned up by default view destructor.
}

// virtual
void LLPanelDirFindAllOld::draw()
{
	updateMaturityCheckbox();
	LLPanelDirBrowser::draw();
}

// static
void LLPanelDirFindAllOld::onCommitScope(LLUICtrl* ctrl, void* data)
{
	LLPanelDirFindAllOld* self = (LLPanelDirFindAllOld*)data;
	self->setFocus(TRUE);
}

// static
void LLPanelDirFindAllOld::onClickSearch(void *userdata)
{
	LLPanelDirFindAllOld *self = (LLPanelDirFindAllOld *)userdata;

	if (self->childGetValue("name").asString().length() < self->mMinSearchChars)
	{
		return;
	};

	BOOL inc_pg = self->childGetValue("incpg").asBoolean();
	BOOL inc_mature = self->childGetValue("incmature").asBoolean();
	BOOL inc_adult = self->childGetValue("incadult").asBoolean();
	if (!(inc_pg || inc_mature || inc_adult))
	{
		LLNotifyBox::showXml("NoContentToSearch");
		return;
	}

	self->setupNewSearch();

	// Figure out scope
	U32 scope = 0x0;
	scope |= DFQ_PEOPLE;	// people (not just online = 0x01 | 0x02)
	// places handled below
	scope |= DFQ_EVENTS;	// events
	scope |= DFQ_GROUPS;	// groups
	if (inc_pg)
	{
		scope |= DFQ_INC_PG;
	}
	if (inc_mature)
	{
		scope |= DFQ_INC_MATURE;
	}
	if (inc_adult)
	{
		scope |= DFQ_INC_ADULT;
	}

	// send the message
	LLMessageSystem *msg = gMessageSystem;
	S32 start_row = 0;
	sendDirFindQuery(msg, self->mSearchID, self->childGetValue("name").asString(), scope, start_row);

	// Also look up classified ads. JC 12/2005
	BOOL filter_auto_renew = FALSE;
	U32 classified_flags = pack_classified_flags_request(filter_auto_renew, inc_pg, inc_mature, inc_adult);
	msg->newMessage("DirClassifiedQuery");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID());
	msg->addUUID("SessionID", gAgent.getSessionID());
	msg->nextBlock("QueryData");
	msg->addUUID("QueryID", self->mSearchID);
	msg->addString("QueryText", self->childGetValue("name").asString());
	msg->addU32("QueryFlags", classified_flags);
	msg->addU32("Category", 0);	// all categories
	msg->addS32("QueryStart", 0);
	gAgent.sendReliableMessage();

	// Need to use separate find places query because places are
	// sent using the more compact DirPlacesReply message.
	U32 query_flags = DFQ_DWELL_SORT;
	if (inc_pg)
	{
		query_flags |= DFQ_INC_PG;
	}
	if (inc_mature)
	{
		query_flags |= DFQ_INC_MATURE;
	}
	if (inc_adult)
	{
		query_flags |= DFQ_INC_ADULT;
	}
	msg->newMessage("DirPlacesQuery");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID() );
	msg->addUUID("SessionID", gAgent.getSessionID());
	msg->nextBlock("QueryData");
	msg->addUUID("QueryID", self->mSearchID );
	msg->addString("QueryText", self->childGetValue("name").asString());
	msg->addU32("QueryFlags", query_flags );
	msg->addS32("QueryStart", 0 ); // Always get the first 100 when using find ALL
	msg->addS8("Category", LLParcel::C_ANY);
	msg->addString("SimName", NULL);
	gAgent.sendReliableMessage();
}
