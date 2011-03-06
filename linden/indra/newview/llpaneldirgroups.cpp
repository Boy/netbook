/** 
 * @file llpaneldirgroups.cpp
 * @brief Groups panel in the Find directory.
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

#include "llpaneldirgroups.h"

// linden library includes
#include "llagent.h"
//#include "llfontgl.h"
#include "message.h"
#include "llqueryflags.h"
#include "llviewercontrol.h"
#include "llnotify.h"

// viewer project includes

LLPanelDirGroups::LLPanelDirGroups(const std::string& name, LLFloaterDirectory* floater)
	:	LLPanelDirBrowser(name, floater)
{
	mMinSearchChars = 3;
}


BOOL LLPanelDirGroups::postBuild()
{
	LLPanelDirBrowser::postBuild();

	childSetKeystrokeCallback("name", &LLPanelDirBrowser::onKeystrokeName, this);

	childSetAction("Search", &LLPanelDirBrowser::onClickSearchCore, this);
	childDisable("Search");
	setDefaultBtn( "Search" );

	return TRUE;
}

LLPanelDirGroups::~LLPanelDirGroups()
{
	// Children all cleaned up by default view destructor.
}

// virtual
void LLPanelDirGroups::draw()
{
	updateMaturityCheckbox();
	LLPanelDirBrowser::draw();
}


// virtual
void LLPanelDirGroups::performQuery()
{
	if (childGetValue("name").asString().length() < mMinSearchChars)
	{
		return;
	}

	BOOL inc_pg = childGetValue("incpg").asBoolean();
	BOOL inc_mature = childGetValue("incmature").asBoolean();
	BOOL inc_adult = childGetValue("incadult").asBoolean();
	if (!(inc_pg || inc_mature || inc_adult))
	{
		LLNotifyBox::showXml("NoContentToSearch");
		return;
	}

	setupNewSearch();

	// groups
	U32 scope = DFQ_GROUPS;
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

	mCurrentSortColumn = "score";
	mCurrentSortAscending = FALSE;

	// send the message
	sendDirFindQuery(
		gMessageSystem,
		mSearchID,
		childGetValue("name").asString(),
		scope,
		mSearchStart);
}