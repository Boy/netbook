/** 
 * @file lltoolselect.cpp
 * @brief LLToolSelect class implementation
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

#include "lltoolselect.h"

#include "llagent.h"
#include "llviewercontrol.h"
#include "lldrawable.h"
#include "llmanip.h"
#include "llmenugl.h"
#include "llselectmgr.h"
#include "lltoolmgr.h"
#include "llfloaterscriptdebug.h"
#include "llviewercamera.h"
#include "llviewermenu.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h" 
#include "llviewerregion.h" 
#include "llviewerwindow.h"
#include "llvoavatar.h"
#include "llworld.h"

// Globals
LLToolSelect		*gToolSelect = NULL;
extern BOOL gAllowSelectAvatar;
//MK
extern BOOL RRenabled;
//mk

const F32 SELECTION_ROTATION_TRESHOLD = 0.1f;

LLToolSelect::LLToolSelect( LLToolComposite* composite )
:	LLTool( "Select", composite ),
	mIgnoreGroup( FALSE )
{
 }

// True if you selected an object.
BOOL LLToolSelect::handleMouseDown(S32 x, S32 y, MASK mask)
{
//MK
	if (RRenabled && gAgent.mRRInterface.mContainsEdit)
	{
		return FALSE;
	}
//mk
	BOOL handled = FALSE;

	// didn't click in any UI object, so must have clicked in the world
	LLViewerObject*	object = NULL;

	// You must hit the body for this tool to think you hit the object.
	object = gObjectList.findObject( gLastHitObjectID );

	if (object)
	{
//MK
		if (RRenabled && gAgent.mRRInterface.mContainsFartouch)
		{
			LLVector3 pos = object->getPositionRegion ();
			pos -= gAgent.getPositionAgent ();
			if (pos.magVec () >= 1.5)
			{
				gLastHitObjectID.setNull();
				mSelectObjectID.setNull();
				return FALSE;
			}
		}
//mk
		mSelectObjectID = object->getID();
		handled = TRUE;
	}
	else
	{
		mSelectObjectID.setNull();
	}

	// Pass mousedown to agent
	LLTool::handleMouseDown(x, y, mask);

	return handled;
}

BOOL LLToolSelect::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	//RN: double click to toggle individual/linked picking???
	return LLTool::handleDoubleClick(x, y, mask);
}

// static
LLHandle<LLObjectSelection> LLToolSelect::handleObjectSelection(LLViewerObject *object, MASK mask, BOOL ignore_group, BOOL temp_select)
{
//MK
	if (RRenabled && !temp_select && gAgent.mRRInterface.mContainsEdit)
	{
		return FALSE;
	}

	if (object)
	{
		if (RRenabled && gAgent.mRRInterface.mContainsFartouch && !object->isHUDAttachment())
		{
			LLVector3 pos = object->getPositionRegion ();
			pos -= gAgent.getPositionAgent ();
			if (pos.magVec () >= 1.5)
			{
				return FALSE;
			}
		}
	}
//mk
	BOOL select_owned = gSavedSettings.getBOOL("SelectOwnedOnly");
	BOOL select_movable = gSavedSettings.getBOOL("SelectMovableOnly");
	
	// *NOTE: These settings must be cleaned up at bottom of function.
	if (temp_select || gAllowSelectAvatar)
	{
		gSavedSettings.setBOOL("SelectOwnedOnly", FALSE);
		gSavedSettings.setBOOL("SelectMovableOnly", FALSE);
		gSelectMgr->setForceSelection(TRUE);
	}

	BOOL extend_select = (mask == MASK_SHIFT) || (mask == MASK_CONTROL);

	// If no object, check for icon, then just deselect
	if (!object)
	{
		if (gLastHitHUDIcon && gLastHitHUDIcon->getSourceObject())
		{
			LLFloaterScriptDebug::show(gLastHitHUDIcon->getSourceObject()->getID());
		}
		else if (!extend_select)
		{
			gSelectMgr->deselectAll();
		}
	}
	else
	{
		BOOL already_selected = object->isSelected();

		if ( extend_select )
		{
			if ( already_selected )
			{
				if ( ignore_group )
				{
					gSelectMgr->deselectObjectOnly(object);
				}
				else
				{
					gSelectMgr->deselectObjectAndFamily(object, TRUE, TRUE);
				}
			}
			else
			{
				if ( ignore_group )
				{
					gSelectMgr->selectObjectOnly(object, SELECT_ALL_TES);
				}
				else
				{
					gSelectMgr->selectObjectAndFamily(object);
				}
			}
		}
		else
		{
			// Save the current zoom values because deselect resets them.
			F32 target_zoom;
			F32 current_zoom;
			gSelectMgr->getAgentHUDZoom(target_zoom, current_zoom);

			// JC - Change behavior to make it easier to select children
			// of linked sets. 9/3/2002
			if( !already_selected || ignore_group)
			{
				// ...lose current selection in favor of just this object
				gSelectMgr->deselectAll();
			}

			if ( ignore_group )
			{
				gSelectMgr->selectObjectOnly(object, SELECT_ALL_TES);
			}
			else
			{
				gSelectMgr->selectObjectAndFamily(object);
			}

			// restore the zoom to the previously stored values.
			gSelectMgr->setAgentHUDZoom(target_zoom, current_zoom);
		}

		if (!gAgent.getFocusOnAvatar() &&										// if camera not glued to avatar
			LLVOAvatar::findAvatarFromAttachment(object) != gAgent.getAvatarObject() &&	// and it's not one of your attachments
			object != gAgent.getAvatarObject())									// and it's not you
		{
			// have avatar turn to face the selected object(s)
			LLVector3d selection_center = gSelectMgr->getSelectionCenterGlobal();
			selection_center = selection_center - gAgent.getPositionGlobal();
			LLVector3 selection_dir;
			selection_dir.setVec(selection_center);
			selection_dir.mV[VZ] = 0.f;
			selection_dir.normVec();
			if (!object->isAvatar() && gAgent.getAtAxis() * selection_dir < 0.6f)
			{
				LLQuaternion target_rot;
				target_rot.shortestArc(LLVector3::x_axis, selection_dir);
				gAgent.startAutoPilotGlobal(gAgent.getPositionGlobal(), "", &target_rot, NULL, NULL, 1.f, SELECTION_ROTATION_TRESHOLD);
			}
		}

		if (temp_select)
		{
			if (!already_selected)
			{
				LLViewerObject* root_object = (LLViewerObject*)object->getRootEdit();
				LLObjectSelectionHandle selection = gSelectMgr->getSelection();

				// this is just a temporary selection
				LLSelectNode* select_node = selection->findNode(root_object);
				if (select_node)
				{
					select_node->setTransient(TRUE);
				}

				for (S32 i = 0; i < (S32)root_object->mChildList.size(); i++)
				{
					select_node = selection->findNode(root_object->mChildList[i]);
					if (select_node)
					{
						select_node->setTransient(TRUE);
					}
				}

			}
		} //if(temp_select)
	} //if(!object)

	// Cleanup temp select settings above.
	if (temp_select || gAllowSelectAvatar)
	{
		gSavedSettings.setBOOL("SelectOwnedOnly", select_owned);
		gSavedSettings.setBOOL("SelectMovableOnly", select_movable);
		gSelectMgr->setForceSelection(FALSE);
	}

	return gSelectMgr->getSelection();
}

BOOL LLToolSelect::handleMouseUp(S32 x, S32 y, MASK mask)
{
	mIgnoreGroup = gSavedSettings.getBOOL("EditLinkedParts");

	LLViewerObject* object = gObjectList.findObject(mSelectObjectID);
	LLToolSelect::handleObjectSelection(object, mask, mIgnoreGroup, FALSE);

	return LLTool::handleMouseUp(x, y, mask);
}

void LLToolSelect::handleDeselect()
{
	if(	hasMouseCapture() )
	{
		setMouseCapture( FALSE );  // Calls onMouseCaptureLost() indirectly
	}
}


void LLToolSelect::stopEditing()
{
	if(	hasMouseCapture() )
	{
		setMouseCapture( FALSE );  // Calls onMouseCaptureLost() indirectly
	}
}

void LLToolSelect::onMouseCaptureLost()
{
	// Finish drag

	gSelectMgr->enableSilhouette(TRUE);

	// Clean up drag-specific variables
	mIgnoreGroup = FALSE;
}


