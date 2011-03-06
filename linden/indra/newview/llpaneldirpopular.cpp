/** 
 * @file llpaneldirpopular.cpp
 * @brief Popular places as measured by dwell.
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

#include "llpaneldirpopular.h"

// linden library includes
#include "llfontgl.h"
#include "message.h"
#include "llqueryflags.h"

// viewer project includes
#include "llagent.h"
#include "llcheckboxctrl.h"
#include "llviewercontrol.h"
#include "lluiconstants.h"
#include "lltextbox.h"
#include "llnotify.h"

LLPanelDirPopular::LLPanelDirPopular(const std::string& name, LLFloaterDirectory* floater)
	:	LLPanelDirBrowser(name, floater),
		mRequested(false)
{
}

BOOL LLPanelDirPopular::postBuild()
{
	LLPanelDirBrowser::postBuild();

	childSetCommitCallback("incpictures", onCommitAny, this);
	childSetCommitCallback("incpg", onCommitAny, this);
	childSetCommitCallback("incmature", onCommitAny, this);
	childSetCommitCallback("incadult", onCommitAny, this);

	mCurrentSortColumn = "dwell";
	mCurrentSortAscending = FALSE;

	// Don't request popular until first drawn.  JC
	// requestPopular();

	return TRUE;
}

LLPanelDirPopular::~LLPanelDirPopular()
{
	// Children all cleaned up by default view destructor.
}


// virtual
void LLPanelDirPopular::draw()
{
	updateMaturityCheckbox();
	LLPanelDirBrowser::draw();
	
	if (!mRequested)
	{
		requestPopular();
		mRequested = true;
	}
}


void LLPanelDirPopular::requestPopular()
{
	LLMessageSystem* msg = gMessageSystem;
	BOOL pictures_only = childGetValue("incpictures").asBoolean();
	BOOL inc_pg = childGetValue("incpg").asBoolean();
	BOOL inc_mature = childGetValue("incmature").asBoolean();
	BOOL inc_adult = childGetValue("incadult").asBoolean();
	if (!(inc_pg || inc_mature || inc_adult))
	{
		LLNotifyBox::showXml("NoContentToSearch");
		return;
	}

	U32 flags = 0x0;
	if (inc_pg)
	{
		flags |= DFQ_INC_PG;
	}
	if (inc_mature)
	{
		flags |= DFQ_INC_MATURE;
	}
	if (inc_adult)
	{
		flags |= DFQ_INC_ADULT;
	}
	if (pictures_only)
	{
		flags |= DFQ_PICTURES_ONLY;
	}

	setupNewSearch();

	msg->newMessage("DirPopularQuery");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID());
	msg->addUUID("SessionID", gAgent.getSessionID());
	msg->nextBlock("QueryData");
	msg->addUUID("QueryID", getSearchID());
	msg->addU32("QueryFlags", flags);
	gAgent.sendReliableMessage();
}


// static
void LLPanelDirPopular::onClickSearch(void* data)
{
	LLPanelDirPopular* self = (LLPanelDirPopular*)data;
	self->requestPopular();
}

// static
void LLPanelDirPopular::onCommitAny(LLUICtrl* ctrl, void* data)
{
	LLPanelDirPopular* self = (LLPanelDirPopular*)data;
	self->requestPopular();
}
