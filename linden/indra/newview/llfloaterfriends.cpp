/** 
 * @file llfloaterfriends.cpp
 * @author Phoenix
 * @date 2005-01-13
 * @brief Implementation of the friends floater
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


#include "llviewerprecompiledheaders.h"

#include "llfloaterfriends.h"

#include <sstream>

#include "lldir.h"

#include "llagent.h"
#include "llappviewer.h"	// for gLastVersionChannel
#include "llfloateravatarpicker.h"
#include "llviewerwindow.h"
#include "llbutton.h"
#include "llcallingcard.h"
#include "llfloateravatarinfo.h"
#include "llinventorymodel.h"
#include "llnamelistctrl.h"
#include "llnotify.h"
#include "llresmgr.h"
#include "llimview.h"
#include "llvieweruictrlfactory.h"
#include "llmenucommands.h"
#include "llviewercontrol.h"
#include "llviewermessage.h"
#include "lltimer.h"
#include "lltextbox.h"

//Maximum number of people you can select to do an operation on at once.
#define MAX_FRIEND_SELECT 20
#define RIGHTS_CHANGE_TIMEOUT 5.0
#define OBSERVER_TIMEOUT 0.5

// simple class to observe the calling cards.
class LLLocalFriendsObserver : public LLFriendObserver, public LLEventTimer
{
public: 
	LLLocalFriendsObserver(LLFloaterFriends* floater) : mFloater(floater), LLEventTimer(OBSERVER_TIMEOUT)
	{
		mEventTimer.stop();
	}
	virtual ~LLLocalFriendsObserver()
	{
		mFloater = NULL;
	}
	virtual void changed(U32 mask)
	{
		// events can arrive quickly in bulk - we need not process EVERY one of them -
		// so we wait a short while to let others pile-in, and process them in aggregate.
		mEventTimer.start();
		mEventTimer.reset();

		// save-up all the mask-bits which have come-in
		mMask |= mask;
	}
	virtual BOOL tick()
	{
		mFloater->updateFriends(mMask);

		mEventTimer.stop();
		mMask = 0;

		return FALSE;
	}
	
protected:
	LLFloaterFriends* mFloater;
	U32 mMask;
};

LLFloaterFriends* LLFloaterFriends::sInstance = NULL;

LLFloaterFriends::LLFloaterFriends() :
	LLFloater(),
	LLEventTimer(1000000),
	mObserver(NULL),
	mShowMaxSelectWarning(TRUE),
	mAllowRightsChange(TRUE),
	mNumRightsChanged(0)
{
	sInstance = this;
	mEventTimer.stop();
	mObserver = new LLLocalFriendsObserver(this);
	LLAvatarTracker::instance().addObserver(mObserver);
	gSavedSettings.setBOOL("ShowFriends", TRUE);
	gUICtrlFactory->buildFloater(this, "floater_friends.xml");
	refreshUI();
}

LLFloaterFriends::~LLFloaterFriends()
{
	LLAvatarTracker::instance().removeObserver(mObserver);
	delete mObserver;
	sInstance = NULL;
	gSavedSettings.setBOOL("ShowFriends", FALSE);
}

BOOL LLFloaterFriends::tick()
{
	mEventTimer.stop();
	mPeriod = 1000000;
	mAllowRightsChange = TRUE;
	updateFriends(LLFriendObserver::ADD);
	return FALSE;
}

// static
void LLFloaterFriends::show(void*)
{
	if(sInstance)
	{
		sInstance->open();	/*Flawfinder: ignore*/
	}
	else
	{
		LLFloaterFriends* self = new LLFloaterFriends;
		self->open(); /*Flawfinder: ignore*/
	}
}


// static
BOOL LLFloaterFriends::visible(void*)
{
	return sInstance && sInstance->getVisible();
}


// static
void LLFloaterFriends::toggle(void*)
{
	if (sInstance)
	{
		sInstance->close();
	}
	else
	{
		show();
	}
}


void LLFloaterFriends::updateFriends(U32 changed_mask)
{
	if (!sInstance) return;
	LLUUID selected_id;
	LLCtrlListInterface *friends_list = sInstance->childGetListInterface("friend_list");
	if (!friends_list) return;
	LLCtrlScrollInterface *friends_scroll = sInstance->childGetScrollInterface("friend_list");
	if (!friends_scroll) return;
	
	// We kill the selection warning, otherwise we'll spam with warning popups
	// if the maximum amount of friends are selected
	mShowMaxSelectWarning = false;

	LLDynamicArray<LLUUID> selected_friends = sInstance->getSelectedIDs();
	if(changed_mask & (LLFriendObserver::ADD | LLFriendObserver::REMOVE | LLFriendObserver::ONLINE))
	{
		refreshNames();
	}
	else if(changed_mask & LLFriendObserver::POWERS)
	{
		--mNumRightsChanged;
		if(mNumRightsChanged > 0)
		{
			mPeriod = RIGHTS_CHANGE_TIMEOUT;	
			mEventTimer.start();
			mEventTimer.reset();
			mAllowRightsChange = FALSE;
		}
		else
		{
			tick();
		}
	}
	if(selected_friends.size() > 0)
	{
		// only non-null if friends was already found. This may fail,
		// but we don't really care here, because refreshUI() will
		// clean up the interface.
		friends_list->setCurrentByID(selected_id);
		for(LLDynamicArray<LLUUID>::iterator itr = selected_friends.begin(); itr != selected_friends.end(); ++itr)
		{
			friends_list->setSelectedByValue(*itr, true);
		}
	}

	refreshUI();
	mShowMaxSelectWarning = true;
}

// virtual
BOOL LLFloaterFriends::postBuild()
{
	mFriendsList = LLUICtrlFactory::getScrollListByName(this, "friend_list");
	mFriendsList->setMaxSelectable(MAX_FRIEND_SELECT);
	mFriendsList->setMaximumSelectCallback(onMaximumSelect);
	mFriendsList->setCommitOnSelectionChange(TRUE);
	childSetCommitCallback("friend_list", onSelectName, this);
	childSetDoubleClickCallback("friend_list", onClickIM);

	refreshNames();

	childSetAction("im_btn", onClickIM, this);
	childSetAction("profile_btn", onClickProfile, this);
	childSetAction("offer_teleport_btn", onClickOfferTeleport, this);
	childSetAction("pay_btn", onClickPay, this);
	childSetAction("add_btn", onClickAddFriend, this);
	childSetAction("remove_btn", onClickRemove, this);
	childSetAction("close_btn", onClickClose, this);

	setDefaultBtn("im_btn");

	updateFriends(LLFriendObserver::ADD);
	refreshUI();

	// primary sort = online status, secondary sort = name
	mFriendsList->sortByColumn("friend_name", TRUE);
	mFriendsList->sortByColumn("icon_online_status", TRUE);

	return TRUE;
}


void LLFloaterFriends::addFriend(const std::string& name, const LLUUID& agent_id)
{
	LLAvatarTracker& at = LLAvatarTracker::instance();
	const LLRelationship* relationInfo = at.getBuddyInfo(agent_id);
	if(!relationInfo) return;
	BOOL online = relationInfo->isOnline();

	LLSD element;
	element["id"] = agent_id;
	LLSD& friend_column = element["columns"][LIST_FRIEND_NAME];
	friend_column["column"] = "friend_name";
	friend_column["value"] = name.c_str();
	friend_column["font"] = "SANSSERIF";
	friend_column["font-style"] = "NORMAL";	

	LLSD& online_status_column = element["columns"][LIST_ONLINE_STATUS];
	online_status_column["column"] = "icon_online_status";
	online_status_column["type"] = "icon";
	if (online)
	{
		friend_column["font-style"] = "BOLD";	
		online_status_column["value"] = gViewerArt.getString("icon_avatar_online.tga");		
	}

	LLSD& online_column = element["columns"][LIST_VISIBLE_ONLINE];
	online_column["column"] = "icon_visible_online";
	online_column["type"] = "checkbox";
	online_column["value"] = relationInfo->isRightGrantedTo(LLRelationship::GRANT_ONLINE_STATUS);

	LLSD& visible_map_column = element["columns"][LIST_VISIBLE_MAP];
	visible_map_column["column"] = "icon_visible_map";
	visible_map_column["type"] = "checkbox";
	visible_map_column["value"] = relationInfo->isRightGrantedTo(LLRelationship::GRANT_MAP_LOCATION);

	LLSD& edit_my_object_column = element["columns"][LIST_EDIT_MINE];
	edit_my_object_column["column"] = "icon_edit_mine";
	edit_my_object_column["type"] = "checkbox";
	edit_my_object_column["value"] = relationInfo->isRightGrantedTo(LLRelationship::GRANT_MODIFY_OBJECTS);

	LLSD& edit_their_object_column = element["columns"][LIST_EDIT_THEIRS];
	edit_their_object_column["column"] = "icon_edit_theirs";
	edit_their_object_column["type"] = "text";
	if(relationInfo->isRightGrantedFrom(LLRelationship::GRANT_MODIFY_OBJECTS))
	{
		edit_their_object_column["type"] = "icon";
		edit_their_object_column["value"] = gViewerArt.getString("ff_edit_theirs.tga");
	}

	LLSD& update_gen_column = element["columns"][LIST_FRIEND_UPDATE_GEN];
	update_gen_column["column"] = "friend_last_update_generation";
	update_gen_column["value"] = relationInfo->getChangeSerialNum();

	mFriendsList->addElement(element, ADD_BOTTOM);
}

// propagate actual relationship to UI
void LLFloaterFriends::updateFriendItem(LLScrollListItem* itemp, const LLRelationship* info)
{
	if (!sInstance) return;
	if (!info) return;
	if (!itemp) return;

	itemp->getColumn(LIST_ONLINE_STATUS)->setValue(info->isOnline() ? gViewerArt.getString("icon_avatar_online.tga") : LLString());
	// render name of online friends in bold text
	((LLScrollListText*)itemp->getColumn(LIST_FRIEND_NAME))->setFontStyle(info->isOnline() ? LLFontGL::BOLD : LLFontGL::NORMAL);	
	itemp->getColumn(LIST_VISIBLE_ONLINE)->setValue(info->isRightGrantedTo(LLRelationship::GRANT_ONLINE_STATUS));
	itemp->getColumn(LIST_VISIBLE_MAP)->setValue(info->isRightGrantedTo(LLRelationship::GRANT_MAP_LOCATION));
	itemp->getColumn(LIST_EDIT_MINE)->setValue(info->isRightGrantedTo(LLRelationship::GRANT_MODIFY_OBJECTS));
	itemp->getColumn(LIST_FRIEND_UPDATE_GEN)->setValue(info->getChangeSerialNum());

	// enable this item, in case it was disabled after user input
	itemp->setEnabled(TRUE);

	// changed item in place, need to request sort
	mFriendsList->sortItems();
}

void LLFloaterFriends::refreshRightsChangeList()
{
	if (!sInstance) return;
	LLDynamicArray<LLUUID> friends = getSelectedIDs();
	S32 num_selected = friends.size();

	bool can_offer_teleport = num_selected >= 1;
	bool selected_friends_online = true;

	LLTextBox* processing_label = LLUICtrlFactory::getTextBoxByName(this, "process_rights_label");

	if(!mAllowRightsChange)
	{
		if(processing_label)
		{
			processing_label->setVisible(true);
			// ignore selection for now
			friends.clear();
			num_selected = 0;
		}
	}
	else if(processing_label)
	{
		processing_label->setVisible(false);
	}

	const LLRelationship* friend_status = NULL;
	for(LLDynamicArray<LLUUID>::iterator itr = friends.begin(); itr != friends.end(); ++itr)
	{
		friend_status = LLAvatarTracker::instance().getBuddyInfo(*itr);
		if (friend_status)
		{
			if(!friend_status->isOnline())
			{
				can_offer_teleport = false;
				selected_friends_online = false;
			}
		}
		else // missing buddy info, don't allow any operations
		{
			can_offer_teleport = false;
		}
	}
	
	if (num_selected == 0)  // nothing selected
	{
		childSetEnabled("im_btn", FALSE);
		childSetEnabled("offer_teleport_btn", FALSE);
	}
	else // we have at least one friend selected...
	{
		// only allow IMs to groups when everyone in the group is online
		// to be consistent with context menus in inventory and because otherwise
		// offline friends would be silently dropped from the session
		childSetEnabled("im_btn", selected_friends_online || num_selected == 1);
		childSetEnabled("offer_teleport_btn", can_offer_teleport);
	}
}

struct SortFriendsByID
{
	bool operator() (const LLScrollListItem* const a, const LLScrollListItem* const b) const
	{
		return a->getValue().asUUID() < b->getValue().asUUID();
	}
};

void LLFloaterFriends::refreshNames()
{
	if (!sInstance) return;
	LLDynamicArray<LLUUID> selected_ids = getSelectedIDs();	
	S32 pos = mFriendsList->getScrollPos();	
	
	// get all buddies we know about
	LLAvatarTracker::buddy_map_t all_buddies;
	LLAvatarTracker::instance().copyBuddyList(all_buddies);
	
	// get all friends in list and sort by UUID
	std::vector<LLScrollListItem*> items = mFriendsList->getAllData();
	std::sort(items.begin(), items.end(), SortFriendsByID());

	std::vector<LLScrollListItem*>::iterator item_it = items.begin();
	std::vector<LLScrollListItem*>::iterator item_end = items.end();
	
	LLAvatarTracker::buddy_map_t::iterator buddy_it;
	for (buddy_it = all_buddies.begin() ; buddy_it != all_buddies.end(); ++buddy_it)
	{
		// erase any items that reflect residents who are no longer buddies
		while(item_it != item_end && buddy_it->first > (*item_it)->getValue().asUUID())
		{
			mFriendsList->deleteItems((*item_it)->getValue());
			++item_it;
		}

		// update existing friends with new info
		if (item_it != item_end && buddy_it->first == (*item_it)->getValue().asUUID())
		{
			const LLRelationship* info = buddy_it->second;
			if (!info) 
			{	
				++item_it;
				continue;
			}
			
			S32 last_change_generation = (*item_it)->getColumn(LIST_FRIEND_UPDATE_GEN)->getValue().asInteger();
			if (last_change_generation < info->getChangeSerialNum())
			{
				// update existing item in UI
				updateFriendItem(mFriendsList->getItem(buddy_it->first), info);
			}
			++item_it;
		}
		// add new friend to list
		else 
		{
			const LLUUID& buddy_id = buddy_it->first;
			char first_name[DB_FIRST_NAME_BUF_SIZE];	/*Flawfinder: ignore*/	
			char last_name[DB_LAST_NAME_BUF_SIZE];		/*Flawfinder: ignore*/

			gCacheName->getName(buddy_id, first_name, last_name);
			std::ostringstream fullname;
			fullname << first_name << " " << last_name;
			addFriend(fullname.str(), buddy_id);
		}
	}

	mFriendsList->selectMultiple(selected_ids);
	mFriendsList->setScrollPos(pos);
}


void LLFloaterFriends::refreshUI()
{	
	if (!sInstance) return;
	int num_selected = mFriendsList->getAllSelected().size();
	BOOL single_selected = (num_selected == 1);
	BOOL some_selected = (num_selected > 0);

	//Options that can only be performed with one friend selected
	childSetEnabled("profile_btn", single_selected);
	childSetEnabled("pay_btn", single_selected);

	//Options that can be performed with up to MAX_FRIEND_SELECT friends selected
	childSetEnabled("remove_btn", some_selected);
	childSetEnabled("im_btn", some_selected);
	childSetEnabled("friend_rights", some_selected);

	refreshRightsChangeList();
}

// static
LLDynamicArray<LLUUID> LLFloaterFriends::getSelectedIDs()
{
	LLUUID selected_id;
	LLDynamicArray<LLUUID> friend_ids;
	if(sInstance)
	{
		std::vector<LLScrollListItem*> selected = sInstance->mFriendsList->getAllSelected();
		for(std::vector<LLScrollListItem*>::iterator itr = selected.begin(); itr != selected.end(); ++itr)
		{
			friend_ids.push_back((*itr)->getUUID());
		}
	}
	return friend_ids;
}

// static
void LLFloaterFriends::onSelectName(LLUICtrl* ctrl, void* user_data)
{
	if(sInstance)
	{
		sInstance->refreshUI();
		// check to see if rights have changed
		sInstance->applyRightsToFriends();
	}
}

//static
void LLFloaterFriends::onMaximumSelect(void* user_data)
{
	LLString::format_map_t args;
	args["[MAX_SELECT]"] = llformat("%d", MAX_FRIEND_SELECT);
	LLNotifyBox::showXml("MaxListSelectMessage", args);
};

// static
void LLFloaterFriends::onClickProfile(void* user_data)
{
	//llinfos << "LLFloaterFriends::onClickProfile()" << llendl;
	LLDynamicArray<LLUUID> ids = getSelectedIDs();
	if(ids.size() > 0)
	{
		LLUUID agent_id = ids[0];
		BOOL online;
		online = LLAvatarTracker::instance().isBuddyOnline(agent_id);
		LLFloaterAvatarInfo::showFromFriend(agent_id, online);
	}
}

// static
void LLFloaterFriends::onClickIM(void* user_data)
{
	//llinfos << "LLFloaterFriends::onClickIM()" << llendl;
	LLDynamicArray<LLUUID> ids = getSelectedIDs();
	if(ids.size() > 0)
	{
		if(ids.size() == 1)
		{
			LLUUID agent_id = ids[0];
			const LLRelationship* info = LLAvatarTracker::instance().getBuddyInfo(agent_id);
			char first[DB_FIRST_NAME_BUF_SIZE];	/* Flawfinder: ignore */
			char last[DB_LAST_NAME_BUF_SIZE];	/* Flawfinder: ignore */
			if(info && gCacheName->getName(agent_id, first, last))
			{
				char buffer[MAX_STRING];	/* Flawfinder: ignore */
				snprintf(buffer, MAX_STRING, "%s %s", first, last);	/* Flawfinder: ignore */
				gIMMgr->setFloaterOpen(TRUE);
				gIMMgr->addSession(
					buffer,
					IM_NOTHING_SPECIAL,
					agent_id);
			}		
		}
		else
		{
			gIMMgr->setFloaterOpen(TRUE);
			gIMMgr->addSession("Friends Conference",
								IM_SESSION_CONFERENCE_START,
								ids[0],
								ids);
		}
		make_ui_sound("UISndStartIM");
	}
}

// static
void LLFloaterFriends::requestFriendship(const LLUUID& target_id, const LLString& target_name)
{
	// HACK: folder id stored as "message"
	LLUUID calling_card_folder_id = gInventory.findCategoryUUIDForType(LLAssetType::AT_CALLINGCARD);
	std::string message = calling_card_folder_id.asString();
	send_improved_im(target_id,
					 target_name.c_str(),
					 message.c_str(),
					 IM_ONLINE,
					 IM_FRIENDSHIP_OFFERED);
}

struct LLAddFriendData
{
	LLUUID mID;
	std::string mName;
};

// static
void LLFloaterFriends::callbackAddFriend(S32 option, void* data)
{
	LLAddFriendData* add = (LLAddFriendData*)data;
	if (option == 0)
	{
		requestFriendship(add->mID, add->mName);
	}
	delete add;
}

// static
void LLFloaterFriends::onPickAvatar(const std::vector<std::string>& names,
									const std::vector<LLUUID>& ids,
									void* )
{
	if (names.empty()) return;
	if (ids.empty()) return;
	requestFriendshipDialog(ids[0], names[0]);
}

// static
void LLFloaterFriends::requestFriendshipDialog(const LLUUID& id,
											   const std::string& name)
{
	if(id == gAgentID)
	{
		LLNotifyBox::showXml("AddSelfFriend");
		return;
	}

	LLAddFriendData* data = new LLAddFriendData();
	data->mID = id;
	data->mName = name;
	
	// TODO: accept a line of text with this dialog
	LLString::format_map_t args;
	args["[NAME]"] = name;
	gViewerWindow->alertXml("AddFriend", args, callbackAddFriend, data);
}

// static
void LLFloaterFriends::onClickAddFriend(void* user_data)
{
	LLFloaterAvatarPicker* picker = LLFloaterAvatarPicker::show(onPickAvatar, user_data, FALSE, TRUE);
	if (sInstance)
	{
		sInstance->addDependentFloater(picker);
	}
}

// static
void LLFloaterFriends::onClickRemove(void* user_data)
{
	//llinfos << "LLFloaterFriends::onClickRemove()" << llendl;
	LLDynamicArray<LLUUID> ids = getSelectedIDs();
	LLStringBase<char>::format_map_t args;
	if(ids.size() > 0)
	{
		LLString msgType = "RemoveFromFriends";
		if(ids.size() == 1)
		{
			LLUUID agent_id = ids[0];
			char first[DB_FIRST_NAME_BUF_SIZE];		/*Flawfinder: ignore*/
			char last[DB_LAST_NAME_BUF_SIZE];		/*Flawfinder: ignore*/
			if(gCacheName->getName(agent_id, first, last))
			{
				args["[FIRST_NAME]"] = first;
				args["[LAST_NAME]"] = last;	
			}
		}
		else
		{
			msgType = "RemoveMultipleFromFriends";
		}
		gViewerWindow->alertXml(msgType,
			args,
			&handleRemove,
			(void*)new LLDynamicArray<LLUUID>(ids));
	}
}

// static
void LLFloaterFriends::onClickOfferTeleport(void*)
{
	LLDynamicArray<LLUUID> ids = getSelectedIDs();
	if(ids.size() > 0)
	{	
		handle_lure(ids);
	}
}

// static
void LLFloaterFriends::onClickPay(void*)
{
	LLDynamicArray<LLUUID> ids = getSelectedIDs();
	if(ids.size() == 1)
	{	
		handle_pay_by_id(ids[0]);
	}
}

// static
void LLFloaterFriends::onClickClose(void* user_data)
{
	//llinfos << "LLFloaterFriends::onClickClose()" << llendl;
	if(sInstance)
	{
		sInstance->onClose(false);
	}
}

void LLFloaterFriends::confirmModifyRights(rights_map_t& ids, EGrantRevoke command)
{
	if (ids.empty()) return;
	
	LLStringBase<char>::format_map_t args;
	if(ids.size() > 0)
	{
		// copy map of ids onto heap
		rights_map_t* rights = new rights_map_t(ids); 
		// package with panel pointer
		std::pair<LLFloaterFriends*, rights_map_t*>* user_data = new std::pair<LLFloaterFriends*, rights_map_t*>(this, rights);

		// for single friend, show their name
		if(ids.size() == 1)
		{
			LLUUID agent_id = ids.begin()->first;
			char first[DB_FIRST_NAME_BUF_SIZE];		/*Flawfinder: ignore*/
			char last[DB_LAST_NAME_BUF_SIZE];		/*Flawfinder: ignore*/
			if(gCacheName->getName(agent_id, first, last))
			{
				args["[FIRST_NAME]"] = first;
				args["[LAST_NAME]"] = last;	
			}
			if (command == GRANT)
			{
				gViewerWindow->alertXml("GrantModifyRights", args, modifyRightsConfirmation, user_data);
			}
			else
			{
				gViewerWindow->alertXml("RevokeModifyRights", args, modifyRightsConfirmation, user_data);
			}
		}
		else
		{
			if (command == GRANT)
			{
				gViewerWindow->alertXml("GrantModifyRightsMultiple", args, modifyRightsConfirmation, user_data);
			}
			else
			{
				gViewerWindow->alertXml("RevokeModifyRightsMultiple", args, modifyRightsConfirmation, user_data);
			}
		}
	}
}

// static
void LLFloaterFriends::modifyRightsConfirmation(S32 option, void* user_data)
{
	std::pair<LLFloaterFriends*, rights_map_t*>* data = (std::pair<LLFloaterFriends*, rights_map_t*>*)user_data;
	LLFloaterFriends* panelp = data->first;

	if(panelp)
	{
		if(0 == option)
		{
			panelp->sendRightsGrant(*(data->second));
		}
		else
		{
			// need to resync view with model, since user cancelled operation
			rights_map_t* rights = data->second;
			rights_map_t::iterator rights_it;
			for (rights_it = rights->begin(); rights_it != rights->end(); ++rights_it)
			{
				LLScrollListItem* itemp = panelp->mFriendsList->getItem(rights_it->first);
				const LLRelationship* info = LLAvatarTracker::instance().getBuddyInfo(rights_it->first);
				panelp->updateFriendItem(itemp, info);
			}
		}
		panelp->refreshUI();
	}

	delete data->second;
	delete data;
}

void LLFloaterFriends::applyRightsToFriends()
{
	if (!sInstance) return;
	BOOL rights_changed = FALSE;

	// store modify rights separately for confirmation
	rights_map_t rights_updates;

	BOOL need_confirmation = FALSE;
	EGrantRevoke confirmation_type = GRANT;

	// this assumes that changes only happened to selected items
	std::vector<LLScrollListItem*> selected = mFriendsList->getAllSelected();
	for(std::vector<LLScrollListItem*>::iterator itr = selected.begin(); itr != selected.end(); ++itr)
	{
		LLUUID id = (*itr)->getValue();
		const LLRelationship* buddy_relationship = LLAvatarTracker::instance().getBuddyInfo(id);
		if (buddy_relationship == NULL) continue;

		bool show_online_staus = (*itr)->getColumn(LIST_VISIBLE_ONLINE)->getValue().asBoolean();
		bool show_map_location = (*itr)->getColumn(LIST_VISIBLE_MAP)->getValue().asBoolean();
		bool allow_modify_objects = (*itr)->getColumn(LIST_EDIT_MINE)->getValue().asBoolean();

		S32 rights = buddy_relationship->getRightsGrantedTo();
		if(buddy_relationship->isRightGrantedTo(LLRelationship::GRANT_ONLINE_STATUS) != show_online_staus)
		{
			rights_changed = TRUE;
			if(show_online_staus) 
			{
				rights |= LLRelationship::GRANT_ONLINE_STATUS;
			}
			else 
			{
				// ONLINE_STATUS necessary for MAP_LOCATION
				rights &= ~LLRelationship::GRANT_ONLINE_STATUS;
				rights &= ~LLRelationship::GRANT_MAP_LOCATION;
				// propagate rights constraint to UI
				(*itr)->getColumn(LIST_VISIBLE_MAP)->setValue(FALSE);
			}
		}
		if(buddy_relationship->isRightGrantedTo(LLRelationship::GRANT_MAP_LOCATION) != show_map_location)
		{
			rights_changed = TRUE;
			if(show_map_location) 
			{
				// ONLINE_STATUS necessary for MAP_LOCATION
				rights |= LLRelationship::GRANT_MAP_LOCATION;
				rights |= LLRelationship::GRANT_ONLINE_STATUS;
				(*itr)->getColumn(LIST_VISIBLE_ONLINE)->setValue(TRUE);
			}
			else 
			{
				rights &= ~LLRelationship::GRANT_MAP_LOCATION;
			}
		}
		
		// now check for change in modify object rights, which requires confirmation
		if(buddy_relationship->isRightGrantedTo(LLRelationship::GRANT_MODIFY_OBJECTS) != allow_modify_objects)
		{
			rights_changed = TRUE;
			need_confirmation = TRUE;

			if(allow_modify_objects)
			{
				rights |= LLRelationship::GRANT_MODIFY_OBJECTS;
				confirmation_type = GRANT;
			}
			else
			{
				rights &= ~LLRelationship::GRANT_MODIFY_OBJECTS;
				confirmation_type = REVOKE;
			}
		}

		if (rights_changed)
		{
			rights_updates.insert(std::make_pair(id, rights));
			// disable these ui elements until response from server
			// to avoid race conditions
			(*itr)->setEnabled(FALSE);
		}
	}

	// separately confirm grant and revoke of modify rights
	if (need_confirmation)
	{
		confirmModifyRights(rights_updates, confirmation_type);
	}
	else
	{
		sendRightsGrant(rights_updates);
	}
}

void LLFloaterFriends::sendRightsGrant(rights_map_t& ids)
{
	if (ids.empty()) return;

	LLMessageSystem* msg = gMessageSystem;

	// setup message header
	msg->newMessageFast(_PREHASH_GrantUserRights);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUID(_PREHASH_AgentID, gAgent.getID());
	msg->addUUID(_PREHASH_SessionID, gAgent.getSessionID());

	rights_map_t::iterator id_it;
	rights_map_t::iterator end_it = ids.end();
	for(id_it = ids.begin(); id_it != end_it; ++id_it)
	{
		msg->nextBlockFast(_PREHASH_Rights);
		msg->addUUID(_PREHASH_AgentRelated, id_it->first);
		msg->addS32(_PREHASH_RelatedRights, id_it->second);
	}

	mNumRightsChanged = ids.size();
	gAgent.sendReliableMessage();
}

// static
void LLFloaterFriends::handleRemove(S32 option, void* user_data)
{
	LLDynamicArray<LLUUID>* ids = static_cast<LLDynamicArray<LLUUID>*>(user_data);
	for(LLDynamicArray<LLUUID>::iterator itr = ids->begin(); itr != ids->end(); ++itr)
	{
		LLUUID id = (*itr);
		const LLRelationship* ip = LLAvatarTracker::instance().getBuddyInfo(id);
		if(ip)
		{			
			switch(option)
			{
			case 0: // YES
				if( ip->isRightGrantedTo(LLRelationship::GRANT_MODIFY_OBJECTS))
				{
					LLAvatarTracker::instance().empower(id, FALSE);
					LLAvatarTracker::instance().notifyObservers();
				}
				LLAvatarTracker::instance().terminateBuddy(id);
				LLAvatarTracker::instance().notifyObservers();
				gInventory.addChangedMask(LLInventoryObserver::LABEL | LLInventoryObserver::CALLING_CARD, LLUUID::null);
				gInventory.notifyObservers();
				break;

			case 1: // NO
			default:
				llinfos << "No removal performed." << llendl;
				break;
			}
		}
		
	}
	delete ids;
}
