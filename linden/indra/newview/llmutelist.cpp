/** 
 * @file llmutelist.cpp
 * @author Richard Nelson, James Cook
 * @brief Management of list of muted players
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2008, Linden Research, Inc.
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

/*
 * How should muting work?
 * Mute an avatar
 * Mute a specific object (accidentally spamming)
 *
 * right-click avatar, mute
 * see list of recent chatters, mute
 * type a name to mute?
 *
 * show in list whether chatter is avatar or object
 *
 * need fast lookup by id
 * need lookup by name, doesn't have to be fast
 */

#include "llviewerprecompiledheaders.h"

#include "llmutelist.h"

#include <boost/tokenizer.hpp>

#include "llcrc.h"
#include "lldir.h"
#include "lldispatcher.h"
#include "llsdserialize.h"
#include "llxfermanager.h"
#include "message.h"

#include "llagent.h"
#include "llfloatermute.h"
#include "llviewergenericmessage.h"	// for gGenericDispatcher
#include "llviewerwindow.h"
#include "llworld.h" //for particle system banning
#include "llviewerobject.h" 
#include "llviewerobjectlist.h"

LLMuteList* gMuteListp = NULL;

std::map<LLUUID, F32> LLMuteList::sUserVolumeSettings;


// "emptymutelist"
class LLDispatchEmptyMuteList : public LLDispatchHandler
{
public:
	virtual bool operator()(
		const LLDispatcher* dispatcher,
		const std::string& key,
		const LLUUID& invoice,
		const sparam_t& strings)
	{
		gMuteListp->setLoaded();
		return true;
	}
};

static LLDispatchEmptyMuteList sDispatchEmptyMuteList;

//-----------------------------------------------------------------------------
// LLMute()
//-----------------------------------------------------------------------------
const char BY_NAME_SUFFIX[] = " (by name)";
const char AGENT_SUFFIX[] = " (resident)";
const char OBJECT_SUFFIX[] = " (object)";
const char GROUP_SUFFIX[] = " (group)";

LLString LLMute::getDisplayName() const
{
	LLString name_with_suffix = mName;
	switch (mType)
	{
		case BY_NAME:
		default:
			name_with_suffix += BY_NAME_SUFFIX;
			break;
		case AGENT:
			name_with_suffix += AGENT_SUFFIX;
			break;
		case OBJECT:
			name_with_suffix += OBJECT_SUFFIX;
			break;
		case GROUP:
			name_with_suffix += GROUP_SUFFIX;
			break;
	}
	return name_with_suffix;
}

void LLMute::setFromDisplayName(const LLString& display_name)
{
	size_t pos = 0;
	mName = display_name;
	
	pos = mName.rfind(GROUP_SUFFIX);
	if (pos != std::string::npos)
	{
		mName.erase(pos);
		mType = GROUP;
		return;
	}
	
	pos = mName.rfind(OBJECT_SUFFIX);
	if (pos != std::string::npos)
	{
		mName.erase(pos);
		mType = OBJECT;
		return;
	}
	
	pos = mName.rfind(AGENT_SUFFIX);
	if (pos != std::string::npos)
	{
		mName.erase(pos);
		mType = AGENT;
		return;
	}
	
	pos = mName.rfind(BY_NAME_SUFFIX);
	if (pos != std::string::npos)
	{
		mName.erase(pos);
		mType = BY_NAME;
		return;
	}
	
	llwarns << "Unable to set mute from display name " << display_name << llendl;
	return;
}

//-----------------------------------------------------------------------------
// LLMuteList()
//-----------------------------------------------------------------------------
LLMuteList::LLMuteList() :
	mIsLoaded(FALSE)
{
	LLMessageSystem* msg = gMessageSystem;

	// Register our various callbacks
	msg->setHandlerFuncFast(_PREHASH_MuteListUpdate, processMuteListUpdate);
	msg->setHandlerFuncFast(_PREHASH_UseCachedMuteList, processUseCachedMuteList);

	gGenericDispatcher.addHandler("emptymutelist", &sDispatchEmptyMuteList);

	// load per-resident voice volume information
	// conceptually, this is part of the mute list information, although it is only stored locally
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, "volume_settings.xml");

	LLSD settings_llsd;
	llifstream file;
	file.open(filename.c_str());
	if (file.is_open())
	{
		LLSDSerialize::fromXML(settings_llsd, file);
	}

	for (LLSD::map_const_iterator iter = settings_llsd.beginMap();
		 iter != settings_llsd.endMap(); ++iter)
	{
		sUserVolumeSettings.insert(std::make_pair(LLUUID(iter->first), (F32)iter->second.asReal()));
	}
}

//-----------------------------------------------------------------------------
// ~LLMuteList()
//-----------------------------------------------------------------------------
LLMuteList::~LLMuteList()
{
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, "volume_settings.xml");
	LLSD settings_llsd;

	for(user_volume_map_t::iterator iter = sUserVolumeSettings.begin(); iter != sUserVolumeSettings.end(); ++iter)
	{
		settings_llsd[iter->first.asString()] = iter->second;
	}

	llofstream file;
	file.open(filename.c_str());
	LLSDSerialize::toPrettyXML(settings_llsd, file);
}

BOOL LLMuteList::isLinden(const LLString& name) const
{
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(" ");
	tokenizer tokens(name, sep);
	tokenizer::iterator token_iter = tokens.begin();
	
	if (token_iter == tokens.end()) return FALSE;
	token_iter++;
	if (token_iter == tokens.end()) return FALSE;
	
	LLString last_name = *token_iter;
	return last_name == "Linden";
}


BOOL LLMuteList::add(const LLMute& mute, U32 flags)
{
	// Can't mute text from Lindens
	if ((mute.mType == LLMute::AGENT)
		&& isLinden(mute.mName) && (flags & LLMute::flagTextChat || flags == 0))
	{
		gViewerWindow->alertXml("MuteLinden");
		return FALSE;
	}
	
	// Can't mute self.
	if (mute.mType == LLMute::AGENT
		&& mute.mID == gAgent.getID())
	{
		return FALSE;
	}
	
	if (mute.mType == LLMute::BY_NAME)
	{		
		// Can't mute empty string by name
		if (mute.mName.empty()) 
		{
			llwarns << "Trying to mute empty string by-name" << llendl;
			return FALSE;
		}

		// Null mutes must have uuid null
		if (mute.mID.notNull())
		{
			llwarns << "Trying to add by-name mute with non-null id" << llendl;
			return FALSE;
		}

		std::pair<string_set_t::iterator, bool> result = mLegacyMutes.insert(mute.mName);
		if (result.second)
		{
			llinfos << "Muting by name " << mute.mName << llendl;
			updateAdd(mute);
			notifyObservers();
			return TRUE;
		}
		else
		{
			// was duplicate
			return FALSE;
		}
	}
	else
	{
		// Need a local (non-const) copy to set up flags properly.
		LLMute localmute = mute;
		
		// If an entry for the same entity is already in the list, remove it, saving flags as necessary.
		mute_set_t::iterator it = mMutes.find(localmute);
		if (it != mMutes.end())
		{
			// This mute is already in the list.  Save the existing entry's flags if that's warranted.
			localmute.mFlags = it->mFlags;
			
			mMutes.erase(it);
			// Don't need to call notifyObservers() here, since it will happen after the entry has been re-added below.
		}
		else
		{
			// There was no entry in the list previously.  Fake things up by making it look like the previous entry had all properties unmuted.
			localmute.mFlags = LLMute::flagAll;
		}

		if(flags)
		{
			// The user passed some combination of flags.  Make sure those flag bits are turned off (i.e. those properties will be muted).
			localmute.mFlags &= (~flags);
		}
		else
		{
			// The user passed 0.  Make sure all flag bits are turned off (i.e. all properties will be muted).
			localmute.mFlags = 0;
		}
		
		// (re)add the mute entry.
		{			
			std::pair<mute_set_t::iterator, bool> result = mMutes.insert(localmute);
			if (result.second)
			{
				llinfos << "Muting " << localmute.mName << " id " << localmute.mID << " flags " << localmute.mFlags << llendl;
				updateAdd(localmute);
				notifyObservers();
				if(!(localmute.mFlags & LLMute::flagParticles))
				{
					//Kill all particle systems owned by muted task
					if(localmute.mType == LLMute::AGENT || localmute.mType == LLMute::OBJECT)
					{
						gWorldPointer->mPartSim.clearParticlesByOwnerID(localmute.mID);
					}
				}
				return TRUE;
			}
		}
	}
	
	// If we were going to return success, we'd have done it by now.
	return FALSE;
}

void LLMuteList::updateAdd(const LLMute& mute)
{
	// Update the database
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_UpdateMuteListEntry);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_MuteData);
	msg->addUUIDFast(_PREHASH_MuteID, mute.mID);
	msg->addStringFast(_PREHASH_MuteName, mute.mName);
	msg->addS32("MuteType", mute.mType);
	msg->addU32("MuteFlags", mute.mFlags);
	gAgent.sendReliableMessage();

	mIsLoaded = TRUE;
}


BOOL LLMuteList::remove(const LLMute& mute, U32 flags)
{
	BOOL found = FALSE;
	
	// First, remove from main list.
	mute_set_t::iterator it = mMutes.find(mute);
	if (it != mMutes.end())
	{
		LLMute localmute = *it;
		bool remove = true;
		if(flags)
		{
			// If the user passed mute flags, we may only want to turn some flags on.
			localmute.mFlags |= flags;
			
			if(localmute.mFlags == LLMute::flagAll)
			{
				// Every currently available mute property has been masked out.
				// Remove the mute entry entirely.
			}
			else
			{
				// Only some of the properties are masked out.  Update the entry.
				remove = false;
			}
		}
		else
		{
			// The caller didn't pass any flags -- just remove the mute entry entirely.
		}
		
		// Always remove the entry from the set -- it will be re-added with new flags if necessary.
		mMutes.erase(it);

		if(remove)
		{
			// The entry was actually removed.  Notify the server.
			updateRemove(localmute);
			llinfos << "Unmuting " << localmute.mName << " id " << localmute.mID << " flags " << localmute.mFlags << llendl;
		}
		else
		{
			// Flags were updated, the mute entry needs to be retransmitted to the server and re-added to the list.
			mMutes.insert(localmute);
			updateAdd(localmute);
			llinfos << "Updating mute entry " << localmute.mName << " id " << localmute.mID << " flags " << localmute.mFlags << llendl;
		}
		
		// Must be after erase.
		notifyObservers();
		found = TRUE;
	}

	// Clean up any legacy mutes
	string_set_t::iterator legacy_it = mLegacyMutes.find(mute.mName);
	if (legacy_it != mLegacyMutes.end())
	{
		// Database representation of legacy mute is UUID null.
		LLMute mute(LLUUID::null, *legacy_it, LLMute::BY_NAME);
		updateRemove(mute);
		mLegacyMutes.erase(legacy_it);
		// Must be after erase.
		notifyObservers();
		found = TRUE;
	}
	
	return found;
}


void LLMuteList::updateRemove(const LLMute& mute)
{
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_RemoveMuteListEntry);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_MuteData);
	msg->addUUIDFast(_PREHASH_MuteID, mute.mID);
	msg->addString("MuteName", mute.mName);
	gAgent.sendReliableMessage();
}


std::vector<LLMute> LLMuteList::getMutes() const
{
	std::vector<LLMute> mutes;
	
	for (mute_set_t::const_iterator it = mMutes.begin();
		 it != mMutes.end();
		 ++it)
	{
		mutes.push_back(*it);
	}
	
	for (string_set_t::const_iterator it = mLegacyMutes.begin();
		 it != mLegacyMutes.end();
		 ++it)
	{
		LLMute legacy(LLUUID::null, *it);
		mutes.push_back(legacy);
	}
	
	std::sort(mutes.begin(), mutes.end(), compare_by_name());
	return mutes;
}

//-----------------------------------------------------------------------------
// loadFromFile()
//-----------------------------------------------------------------------------
BOOL LLMuteList::loadFromFile(const LLString& filename)
{
	if(!filename.size())
	{
		llwarns << "Mute List Filename is Empty!" << llendl;
		return FALSE;
	}

	FILE* fp = LLFile::fopen(filename.c_str(), "rb");		/*Flawfinder: ignore*/
	if (!fp)
	{
		llwarns << "Couldn't open mute list " << filename << llendl;
		return FALSE;
	}

	// *NOTE: Changing the size of these buffers will require changes
	// in the scanf below.
	char id_buffer[MAX_STRING];		/*Flawfinder: ignore*/
	char name_buffer[MAX_STRING];		/*Flawfinder: ignore*/
	char buffer[MAX_STRING];		/*Flawfinder: ignore*/
	while (!feof(fp) 
		   && fgets(buffer, MAX_STRING, fp))
	{
		id_buffer[0] = '\0';
		name_buffer[0] = '\0';
		S32 type = 0;
		U32 flags = 0;
		sscanf(	/* Flawfinder: ignore */
			buffer, " %d %254s %254[^|]| %u\n", &type, id_buffer, name_buffer,
			&flags);
		LLUUID id = LLUUID(id_buffer);
		LLMute mute(id, name_buffer, (LLMute::EType)type, flags);
		if (mute.mID.isNull()
			|| mute.mType == LLMute::BY_NAME)
		{
			mLegacyMutes.insert(mute.mName);
		}
		else
		{
			mMutes.insert(mute);
		}
	}
	fclose(fp);
	mIsLoaded = TRUE;
	notifyObservers();	
	return TRUE;
}

//-----------------------------------------------------------------------------
// saveToFile()
//-----------------------------------------------------------------------------
BOOL LLMuteList::saveToFile(const LLString& filename)
{
	if(!filename.size())
	{
		llwarns << "Mute List Filename is Empty!" << llendl;
		return FALSE;
	}

	FILE* fp = LLFile::fopen(filename.c_str(), "wb");		/*Flawfinder: ignore*/
	if (!fp)
	{
		llwarns << "Couldn't open mute list " << filename << llendl;
		return FALSE;
	}
	// legacy mutes have null uuid
	char id_string[UUID_STR_LENGTH];		/*Flawfinder: ignore*/
	LLUUID::null.toString(id_string);
	for (string_set_t::iterator it = mLegacyMutes.begin();
		 it != mLegacyMutes.end();
		 ++it)
	{
		fprintf(fp, "%d %s %s|\n", (S32)LLMute::BY_NAME, id_string, it->c_str());
	}
	for (mute_set_t::iterator it = mMutes.begin();
		 it != mMutes.end();
		 ++it)
	{
		it->mID.toString(id_string);
		const LLString& name = it->mName;
		fprintf(fp, "%d %s %s|%u\n", (S32)it->mType, id_string, name.c_str(), it->mFlags);
	}
	fclose(fp);
	return TRUE;
}


BOOL LLMuteList::isMuted(const LLUUID& id, const LLString& name, U32 flags) const
{
	LLUUID id_to_check = id;
	
	// for objects, check for muting on their parent prim
	LLViewerObject *objectp = gObjectList.findObject(id);
	if ((objectp) && (!objectp->isAvatar()))
	{
		LLViewerObject *parentp = (LLViewerObject *)objectp->getParent();
		if (parentp)
		{
			id_to_check = parentp->getID();
		}
	}
	
	// don't need name or type for lookup
	LLMute mute(id_to_check);
	mute_set_t::const_iterator mute_it = mMutes.find(mute);
	if (mute_it != mMutes.end())
	{
		// If any of the flags the caller passed are set, this item isn't considered muted for this caller.
		if(flags & mute_it->mFlags)
		{
			return FALSE;
		}
		return TRUE;
	}

	// empty names can't be legacy-muted
	if (name.empty()) return FALSE;

	// Look in legacy pile
	string_set_t::const_iterator legacy_it = mLegacyMutes.find(name);
	return legacy_it != mLegacyMutes.end();
}

//-----------------------------------------------------------------------------
// requestFromServer()
//-----------------------------------------------------------------------------
void LLMuteList::requestFromServer(const LLUUID& agent_id)
{
	char agent_id_string[UUID_STR_LENGTH];		/*Flawfinder: ignore*/
	char filename[LL_MAX_PATH];		/*Flawfinder: ignore*/
	agent_id.toString(agent_id_string);
	snprintf(filename, sizeof(filename), "%s.cached_mute", gDirUtilp->getExpandedFilename(LL_PATH_CACHE,agent_id_string).c_str());			/* Flawfinder: ignore */
	LLCRC crc;
	crc.update(filename);

	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_MuteListRequest);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, agent_id);
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_MuteData);
	msg->addU32Fast(_PREHASH_MuteCRC, crc.getCRC());
	gAgent.sendReliableMessage();
}

//-----------------------------------------------------------------------------
// cache()
//-----------------------------------------------------------------------------

void LLMuteList::cache(const LLUUID& agent_id)
{
	// Write to disk even if empty.
	if(mIsLoaded)
	{
		char agent_id_string[UUID_STR_LENGTH];		/*Flawfinder: ignore*/
		char filename[LL_MAX_PATH];		/*Flawfinder: ignore*/
		agent_id.toString(agent_id_string);
		snprintf(filename, sizeof(filename), "%s.cached_mute", gDirUtilp->getExpandedFilename(LL_PATH_CACHE,agent_id_string).c_str());			/* Flawfinder: ignore */
		saveToFile(filename);
	}
}

void LLMuteList::setSavedResidentVolume(const LLUUID& id, F32 volume)
{
	// store new value in volume settings file
	sUserVolumeSettings[id] = volume;
}

F32 LLMuteList::getSavedResidentVolume(const LLUUID& id)
{
	const F32 DEFAULT_VOLUME = 0.5f;
	
	user_volume_map_t::iterator found_it = sUserVolumeSettings.find(id);
	if (found_it != sUserVolumeSettings.end())
	{
		return found_it->second;
	}
	//FIXME: assumes default, should get this from somewhere
	return DEFAULT_VOLUME;
}


//-----------------------------------------------------------------------------
// Static message handlers
//-----------------------------------------------------------------------------

void LLMuteList::processMuteListUpdate(LLMessageSystem* msg, void**)
{
	llinfos << "LLMuteList::processMuteListUpdate()" << llendl;
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_MuteData, _PREHASH_AgentID, agent_id);
	if(agent_id != gAgent.getID())
	{
		llwarns << "Got an mute list update for the wrong agent." << llendl;
		return;
	}
	char filename[MAX_STRING];		/*Flawfinder: ignore*/
	filename[0] = '\0';
	msg->getStringFast(_PREHASH_MuteData, _PREHASH_Filename, MAX_STRING, filename);
	
	std::string *local_filename_and_path = new std::string(gDirUtilp->getExpandedFilename( LL_PATH_CACHE, filename ));
	gXferManager->requestFile(local_filename_and_path->c_str(),
							  filename,
							  LL_PATH_CACHE,
							  msg->getSender(),
							  TRUE, // make the remote file temporary.
							  onFileMuteList,
							  (void**)local_filename_and_path,
							  LLXferManager::HIGH_PRIORITY);
}

void LLMuteList::processUseCachedMuteList(LLMessageSystem* msg, void**)
{
	llinfos << "LLMuteList::processUseCachedMuteList()" << llendl;
	if (!gMuteListp) return;

	char agent_id_string[UUID_STR_LENGTH];		/*Flawfinder: ignore*/
	gAgent.getID().toString(agent_id_string);
	char filename[LL_MAX_PATH];		/*Flawfinder: ignore*/
	snprintf(filename, sizeof(filename), "%s.cached_mute", gDirUtilp->getExpandedFilename(LL_PATH_CACHE,agent_id_string).c_str());			/* Flawfinder: ignore */
	gMuteListp->loadFromFile(filename);
}

void LLMuteList::onFileMuteList(void** user_data, S32 error_code, LLExtStat ext_status)
{
	llinfos << "LLMuteList::processMuteListFile()" << llendl;
	if (!gMuteListp) return;

	std::string *local_filename_and_path = (std::string*)user_data;
	if(local_filename_and_path && !local_filename_and_path->empty() && (error_code == 0))
	{
		gMuteListp->loadFromFile(local_filename_and_path->c_str());
		LLFile::remove(local_filename_and_path->c_str());
	}
	delete local_filename_and_path;
}

void LLMuteList::addObserver(LLMuteListObserver* observer)
{
	mObservers.insert(observer);
}

void LLMuteList::removeObserver(LLMuteListObserver* observer)
{
	mObservers.erase(observer);
}

void LLMuteList::setLoaded()
{
	mIsLoaded = TRUE;
	notifyObservers();
}

void LLMuteList::notifyObservers()
{
	// HACK: LLFloaterMute is constructed before LLMuteList,
	// so it can't easily observe it.  Changing this requires
	// much reshuffling of the startup process. JC
	if (gFloaterMute)
	{
		gFloaterMute->refreshMuteList();
	}

	for (observer_set_t::iterator it = mObservers.begin();
		it != mObservers.end();
		)
	{
		LLMuteListObserver* observer = *it;
		observer->onChange();
		// In case onChange() deleted an entry.
		it = mObservers.upper_bound(observer);
	}
}
