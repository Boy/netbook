/** 
 * @file llfloaternamedesc.cpp
 * @brief LLFloaterNameDesc class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2008, Linden Research, Inc.
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

#include "llfloaternamedesc.h"

// project includes
#include "lllineeditor.h"
#include "llresmgr.h"
#include "lltextbox.h"
#include "llbutton.h"
#include "llviewerwindow.h"
#include "llfocusmgr.h"
#include "llradiogroup.h"
#include "lldbstrings.h"
#include "lldir.h"
#include "llviewercontrol.h"
#include "llviewermenufile.h"	// upload_new_resource()
#include "llvieweruictrlfactory.h"

// linden includes
#include "llassetstorage.h"
#include "llinventorytype.h"

const S32 PREVIEW_LINE_HEIGHT = 19;
const S32 PREVIEW_CLOSE_BOX_SIZE = 16;
const S32 PREVIEW_BORDER_WIDTH = 2;
const S32 PREVIEW_RESIZE_HANDLE_SIZE = S32(RESIZE_HANDLE_WIDTH * OO_SQRT2) + PREVIEW_BORDER_WIDTH;
const S32 PREVIEW_VPAD = 2;
const S32 PREVIEW_HPAD = PREVIEW_RESIZE_HANDLE_SIZE;
const S32 PREVIEW_HEADER_SIZE = 3 * PREVIEW_LINE_HEIGHT + PREVIEW_VPAD;
const S32 PREF_BUTTON_WIDTH = 64;
const S32 PREF_BUTTON_HEIGHT = 16;

//-----------------------------------------------------------------------------
// LLFloaterNameDesc()
//-----------------------------------------------------------------------------
LLFloaterNameDesc::LLFloaterNameDesc(const std::string& filename )
	:
	LLFloater("Name/Description Floater")
{
	mFilenameAndPath = filename;
	mFilename.assign(strrchr( filename.c_str(), gDirUtilp->getDirDelimiter()[0]) + 1);
	// SL-5521 Maintain capitalization of filename when making the inventory item. JC
	//LLString::toLower(mFilename);
	mIsAudio = FALSE;
}

//-----------------------------------------------------------------------------
// postBuild()
//-----------------------------------------------------------------------------
BOOL LLFloaterNameDesc::postBuild()
{
	LLRect r;

	LLString asset_name = mFilename;
	LLString::replaceNonstandardASCII( asset_name, '?' );
	LLString::replaceChar(asset_name, '|', '?');
	LLString::stripNonprintable(asset_name);
	LLString::trim(asset_name);

	char* asset_name_str = (char*)asset_name.c_str();
	char* end_p = strrchr(asset_name_str, '.');		 // strip extension if exists
	if( !end_p )
	{
		end_p = asset_name_str + strlen( asset_name_str );		/* Flawfinder: ignore */
	}
	else
	if( !stricmp( end_p, ".wav") )
	{
		mIsAudio = TRUE;
	}
		
	S32 len = llmin( (S32) (DB_INV_ITEM_NAME_STR_LEN), (S32) (end_p - asset_name_str) );

	asset_name = asset_name.substr( 0, len );

	setTitle(mFilename);

	centerWindow();

	S32 line_width = mRect.getWidth() - 2 * PREVIEW_HPAD;
	S32 y = mRect.getHeight() - PREVIEW_LINE_HEIGHT;

	r.setLeftTopAndSize( PREVIEW_HPAD, y, line_width, PREVIEW_LINE_HEIGHT );
	y -= PREVIEW_LINE_HEIGHT;

	r.setLeftTopAndSize( PREVIEW_HPAD, y, line_width, PREVIEW_LINE_HEIGHT );    

	childSetCommitCallback("name_form", doCommit, this);
	childSetValue("name_form", LLSD(asset_name));

	LLLineEditor *NameEditor = LLViewerUICtrlFactory::getLineEditorByName(this, "name_form");
	if (NameEditor)
	{
		NameEditor->setMaxTextLength(DB_INV_ITEM_NAME_STR_LEN);
		NameEditor->setPrevalidate(&LLLineEditor::prevalidatePrintableNotPipe);
	}

	y -= llfloor(PREVIEW_LINE_HEIGHT * 1.2f);
	y -= PREVIEW_LINE_HEIGHT;

	r.setLeftTopAndSize( PREVIEW_HPAD, y, line_width, PREVIEW_LINE_HEIGHT );  
	childSetCommitCallback("description_form", doCommit, this);
	LLLineEditor *DescEditor = LLViewerUICtrlFactory::getLineEditorByName(this, "description_form");
	if (DescEditor)
	{
		DescEditor->setMaxTextLength(DB_INV_ITEM_DESC_STR_LEN);
		DescEditor->setPrevalidate(&LLLineEditor::prevalidatePrintableNotPipe);
	}

	y -= llfloor(PREVIEW_LINE_HEIGHT * 1.2f);

	// Cancel button
	childSetAction("cancel_btn", onBtnCancel, this);

	// OK button
	childSetAction("ok_btn", onBtnOK, this);
	setDefaultBtn("ok_btn");

	return TRUE;
}

//-----------------------------------------------------------------------------
// LLFloaterNameDesc()
//-----------------------------------------------------------------------------
LLFloaterNameDesc::~LLFloaterNameDesc()
{
	gFocusMgr.releaseFocusIfNeeded( this ); // calls onCommit()
}

//-----------------------------------------------------------------------------
// centerWindow()
//-----------------------------------------------------------------------------
void LLFloaterNameDesc::centerWindow()
{
	LLRect window_rect = gViewerWindow->getRootView()->getRect();

	S32 dialog_left = window_rect.mLeft + (window_rect.getWidth() - mRect.getWidth()) / 2;
	S32 dialog_bottom = window_rect.mBottom + (window_rect.getHeight() - mRect.getHeight()) / 2;

	translate( dialog_left - mRect.mLeft, dialog_bottom - mRect.mBottom );
}

// Sub-classes should override this function if they allow editing
//-----------------------------------------------------------------------------
// onCommit()
//-----------------------------------------------------------------------------
void LLFloaterNameDesc::onCommit()
{
}

// static 
//-----------------------------------------------------------------------------
// onCommit()
//-----------------------------------------------------------------------------
void LLFloaterNameDesc::doCommit( class LLUICtrl *, void* userdata )
{
	LLFloaterNameDesc* self = (LLFloaterNameDesc*) userdata;
	self->onCommit();
}

// static 
//-----------------------------------------------------------------------------
// onBtnOK()
//-----------------------------------------------------------------------------
void LLFloaterNameDesc::onBtnOK( void* userdata )
{
	LLFloaterNameDesc *fp =(LLFloaterNameDesc *)userdata;

	fp->childDisable("ok_btn"); // don't allow inadvertent extra uploads
	
	upload_new_resource(fp->mFilenameAndPath, // file
		fp->childGetValue("name_form").asString(), 
		fp->childGetValue("description_form").asString(), 
		0, LLAssetType::AT_NONE, LLInventoryType::IT_NONE);
	fp->close(false);
}

// static 
//-----------------------------------------------------------------------------
// onBtnCancel()
//-----------------------------------------------------------------------------
void LLFloaterNameDesc::onBtnCancel( void* userdata )
{
	LLFloaterNameDesc *fp =(LLFloaterNameDesc *)userdata;
	fp->close(false);
}
