/** 
 * @file lltoolcomp.cpp
 * @brief Composite tools
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

#include "lltoolcomp.h"

#include "llgl.h"
#include "indra_constants.h"

#include "llmanip.h"
#include "llmaniprotate.h"
#include "llmanipscale.h"
#include "llmaniptranslate.h"
#include "llmenugl.h"			// for right-click menu hack
#include "llselectmgr.h"
#include "lltoolfocus.h"
#include "lltoolgrab.h"
#include "lltoolgun.h"
#include "lltoolmgr.h"
#include "lltoolselect.h"
#include "lltoolselectrect.h"
#include "lltoolplacer.h"
#include "llviewermenu.h"
#include "llviewerobject.h"
#include "llviewerwindow.h"
#include "llagent.h"
#include "llfloatertools.h"
#include "llviewercontrol.h"

const S32 BUTTON_HEIGHT = 16;
const S32 BUTTON_WIDTH_SMALL = 32;
const S32 BUTTON_WIDTH_BIG = 48;
const S32 HPAD = 4;

// Globals
LLToolCompInspect   *gToolInspect = NULL;
LLToolCompTranslate	*gToolTranslate = NULL;
LLToolCompScale		*gToolStretch = NULL;
LLToolCompRotate	*gToolRotate = NULL;
LLToolCompCreate	*gToolCreate = NULL;
LLToolCompGun		*gToolGun = NULL;

extern LLControlGroup gSavedSettings;


//-----------------------------------------------------------------------
// LLToolComposite

//static
void LLToolComposite::setCurrentTool( LLTool* new_tool )
{
	if( mCur != new_tool )
	{
		if( mSelected )
		{
			mCur->handleDeselect();
			mCur = new_tool;
			mCur->handleSelect();
		}
		else
		{
			mCur = new_tool;
		}
	}
}

LLToolComposite::LLToolComposite(const LLString& name)
	: LLTool(name),
	  mCur(NULL), mDefault(NULL), mSelected(FALSE),
	  mMouseDown(FALSE), mManip(NULL), mSelectRect(NULL)
{
}

// Returns to the default tool
BOOL LLToolComposite::handleMouseUp(S32 x, S32 y, MASK mask)
{ 
	BOOL handled = mCur->handleMouseUp( x, y, mask );
	if( handled )
	{
		setCurrentTool( mDefault );
	}
 return handled;
}

void LLToolComposite::onMouseCaptureLost()
{
	mCur->onMouseCaptureLost();
	setCurrentTool( mDefault );
}

BOOL LLToolComposite::isSelecting()
{ 
	return mCur == mSelectRect; 
}

void LLToolComposite::handleSelect()
{
	if (!gSavedSettings.getBOOL("EditLinkedParts"))
	{
		gSelectMgr->promoteSelectionToRoot();
	}
	mCur = mDefault; 
	mCur->handleSelect(); 
	mSelected = TRUE; 
}

//----------------------------------------------------------------------------
// LLToolCompInspect
//----------------------------------------------------------------------------

LLToolCompInspect::LLToolCompInspect()
: LLToolComposite("Inspect")
{
	mSelectRect		= new LLToolSelectRect(this);
	mDefault = mSelectRect;
}


LLToolCompInspect::~LLToolCompInspect()
{
	delete mSelectRect;
	mSelectRect = NULL;
}

BOOL LLToolCompInspect::handleMouseDown(S32 x, S32 y, MASK mask)
{
	mMouseDown = TRUE;
	gViewerWindow->hitObjectOrLandGlobalAsync(x, y, mask, pickCallback);
	return TRUE;
}

void LLToolCompInspect::pickCallback(S32 x, S32 y, MASK mask)
{
	LLViewerObject* hit_obj = gViewerWindow->lastObjectHit();

	if (!gToolInspect->mMouseDown)
	{
		// fast click on object, but mouse is already up...just do select
		gToolInspect->mSelectRect->handleObjectSelection(hit_obj, mask, gSavedSettings.getBOOL("EditLinkedParts"), FALSE);
		return;
	}

	if( hit_obj )
	{
		if (gSelectMgr->getSelection()->getObjectCount())
		{
			gEditMenuHandler = gSelectMgr;
		}
		gToolInspect->setCurrentTool( gToolInspect->mSelectRect );
		gToolInspect->mSelectRect->handleMouseDown( x, y, mask );

	}
	else
	{
		gToolInspect->setCurrentTool( gToolInspect->mSelectRect );
		gToolInspect->mSelectRect->handleMouseDown( x, y, mask);
	}
}

BOOL LLToolCompInspect::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	return TRUE;
}

//----------------------------------------------------------------------------
// LLToolCompTranslate
//----------------------------------------------------------------------------

LLToolCompTranslate::LLToolCompTranslate()
	: LLToolComposite("Move")
{
	mManip		= new LLManipTranslate(this);
	mSelectRect		= new LLToolSelectRect(this);

	mCur			= mManip;
	mDefault		= mManip;
}

LLToolCompTranslate::~LLToolCompTranslate()
{
	delete mManip;
	mManip = NULL;

	delete mSelectRect;
	mSelectRect = NULL;
}

BOOL LLToolCompTranslate::handleHover(S32 x, S32 y, MASK mask)
{
	if( !mCur->hasMouseCapture() )
	{
		setCurrentTool( mManip );
	}
	return mCur->handleHover( x, y, mask );
}


BOOL LLToolCompTranslate::handleMouseDown(S32 x, S32 y, MASK mask)
{
	mMouseDown = TRUE;
	gViewerWindow->hitObjectOrLandGlobalAsync(x, y, mask, pickCallback, TRUE);
	return TRUE;
}

void LLToolCompTranslate::pickCallback(S32 x, S32 y, MASK mask)
{
	LLViewerObject* hit_obj = gViewerWindow->lastObjectHit();

	gToolTranslate->mManip->highlightManipulators(x, y);
	if (!gToolTranslate->mMouseDown)
	{
		// fast click on object, but mouse is already up...just do select
		gToolTranslate->mSelectRect->handleObjectSelection(hit_obj, mask, gSavedSettings.getBOOL("EditLinkedParts"), FALSE);
		return;
	}

	if( hit_obj || gToolTranslate->mManip->getHighlightedPart() != LLManip::LL_NO_PART )
	{
		if (gToolTranslate->mManip->getSelection()->getObjectCount())
		{
			gEditMenuHandler = gSelectMgr;
		}

		BOOL can_move = gToolTranslate->mManip->canAffectSelection();

		if(	LLManip::LL_NO_PART != gToolTranslate->mManip->getHighlightedPart() && can_move)
		{
			gToolTranslate->setCurrentTool( gToolTranslate->mManip );
			gToolTranslate->mManip->handleMouseDownOnPart( x, y, mask );
		}
		else
		{
			gToolTranslate->setCurrentTool( gToolTranslate->mSelectRect );
			gToolTranslate->mSelectRect->handleMouseDown( x, y, mask );

			// *TODO: add toggle to trigger old click-drag functionality
			// gToolTranslate->mManip->handleMouseDownOnPart( XY_part, x, y, mask);
		}
	}
	else
	{
		gToolTranslate->setCurrentTool( gToolTranslate->mSelectRect );
		gToolTranslate->mSelectRect->handleMouseDown( x, y, mask);
	}
}

BOOL LLToolCompTranslate::handleMouseUp(S32 x, S32 y, MASK mask)
{
	mMouseDown = FALSE;
	return LLToolComposite::handleMouseUp(x, y, mask);
}

LLTool* LLToolCompTranslate::getOverrideTool(MASK mask)
{
	if (mask == MASK_CONTROL)
	{
		return gToolRotate;
	}
	else if (mask == (MASK_CONTROL | MASK_SHIFT))
	{
		return gToolStretch;
	}
	return LLToolComposite::getOverrideTool(mask);
}

BOOL LLToolCompTranslate::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	if (mManip->getSelection()->isEmpty() && mManip->getHighlightedPart() == LLManip::LL_NO_PART)
	{
		// You should already have an object selected from the mousedown.
		// If so, show its properties
		gFloaterTools->showPanel(LLFloaterTools::PANEL_CONTENTS);
		return TRUE;
	}
	// Nothing selected means the first mouse click was probably
	// bad, so try again.
	return FALSE;
}


void LLToolCompTranslate::render()
{
	mCur->render();

	if( mCur != mManip )
	{
		LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
		mManip->renderGuidelines();
	}
}


//-----------------------------------------------------------------------
// LLToolCompScale

LLToolCompScale::LLToolCompScale()
	: LLToolComposite("Stretch")
{
	mManip = new LLManipScale(this);
	mSelectRect = new LLToolSelectRect(this);

	mCur = mManip;
	mDefault = mManip;
}

LLToolCompScale::~LLToolCompScale()
{
	delete mManip;
	delete mSelectRect;
}

BOOL LLToolCompScale::handleHover(S32 x, S32 y, MASK mask)
{
	if( !mCur->hasMouseCapture() )
	{
		setCurrentTool(mManip );
	}
	return mCur->handleHover( x, y, mask );
}


BOOL LLToolCompScale::handleMouseDown(S32 x, S32 y, MASK mask)
{
	mMouseDown = TRUE;
	gViewerWindow->hitObjectOrLandGlobalAsync(x, y, mask, pickCallback);
	return TRUE;
}

void LLToolCompScale::pickCallback(S32 x, S32 y, MASK mask)
{
	LLViewerObject* hit_obj = gViewerWindow->lastObjectHit();

	gToolStretch->mManip->highlightManipulators(x, y);
	if (!gToolStretch->mMouseDown)
	{
		// fast click on object, but mouse is already up...just do select
		gToolStretch->mSelectRect->handleObjectSelection(hit_obj, mask, gSavedSettings.getBOOL("EditLinkedParts"), FALSE);

		return;
	}

	if( hit_obj || gToolStretch->mManip->getHighlightedPart() != LLManip::LL_NO_PART)
	{
		if (gToolStretch->mManip->getSelection()->getObjectCount())
		{
			gEditMenuHandler = gSelectMgr;
		}
		if(	LLManip::LL_NO_PART != gToolStretch->mManip->getHighlightedPart() )
		{
			gToolStretch->setCurrentTool( gToolStretch->mManip );
			gToolStretch->mManip->handleMouseDownOnPart( x, y, mask );
		}
		else
		{
			gToolStretch->setCurrentTool( gToolStretch->mSelectRect );
			gToolStretch->mSelectRect->handleMouseDown( x, y, mask );
		}
	}
	else
	{
		gToolStretch->setCurrentTool( gToolStretch->mSelectRect );
		gToolStretch->mCur->handleMouseDown( x, y, mask );
	}
}

BOOL LLToolCompScale::handleMouseUp(S32 x, S32 y, MASK mask)
{
	mMouseDown = FALSE;
	return LLToolComposite::handleMouseUp(x, y, mask);
}

LLTool* LLToolCompScale::getOverrideTool(MASK mask)
{
	if (mask == MASK_CONTROL)
	{
		return gToolRotate;
	}

	return LLToolComposite::getOverrideTool(mask);
}


BOOL LLToolCompScale::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	if (!mManip->getSelection()->isEmpty() && mManip->getHighlightedPart() == LLManip::LL_NO_PART)
	{
		// You should already have an object selected from the mousedown.
		// If so, show its properties
		gFloaterTools->showPanel(LLFloaterTools::PANEL_CONTENTS);
		//gBuildView->setPropertiesPanelOpen(TRUE);
		return TRUE;
	}
	else
	{
		// Nothing selected means the first mouse click was probably
		// bad, so try again.
		return handleMouseDown(x, y, mask);
	}
}


void LLToolCompScale::render()
{
	mCur->render();

	if( mCur != mManip )
	{
		LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
		mManip->renderGuidelines();
	}
}

//-----------------------------------------------------------------------
// LLToolCompCreate

LLToolCompCreate::LLToolCompCreate()
	: LLToolComposite("Create")
{
	mPlacer = new LLToolPlacer();
	mSelectRect = new LLToolSelectRect(this);

	mCur = mPlacer;
	mDefault = mPlacer;
	mObjectPlacedOnMouseDown = FALSE;
}


LLToolCompCreate::~LLToolCompCreate()
{
	delete mPlacer;
	delete mSelectRect;
}


BOOL LLToolCompCreate::handleMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL handled = FALSE;
	mMouseDown = TRUE;

	if ( !(mask == MASK_SHIFT) && !(mask == MASK_CONTROL) )
	{
		setCurrentTool( mPlacer );
		handled = mPlacer->placeObject( x, y, mask );
	}
	else
	{
		gViewerWindow->hitObjectOrLandGlobalAsync(x, y, mask, pickCallback);
		handled = TRUE;
	}
	
	mObjectPlacedOnMouseDown = TRUE;

	return TRUE;
}

void LLToolCompCreate::pickCallback(S32 x, S32 y, MASK mask)
{
	// *NOTE: We mask off shift and control, so you cannot
	// multi-select multiple objects with the create tool.
	mask = (mask & ~MASK_SHIFT);
	mask = (mask & ~MASK_CONTROL);

	gToolCreate->setCurrentTool( gToolCreate->mSelectRect );
	gToolCreate->mSelectRect->handleMouseDown( x, y, mask);
}

BOOL LLToolCompCreate::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	return handleMouseDown(x, y, mask);
}

BOOL LLToolCompCreate::handleMouseUp(S32 x, S32 y, MASK mask)
{
	BOOL handled = FALSE;

	if ( mMouseDown && !mObjectPlacedOnMouseDown && !(mask == MASK_SHIFT) && !(mask == MASK_CONTROL) )
	{
		setCurrentTool( mPlacer );
		handled = mPlacer->placeObject( x, y, mask );
	}

	mObjectPlacedOnMouseDown = FALSE;
	mMouseDown = FALSE;

	if (!handled)
	{
		handled = LLToolComposite::handleMouseUp(x, y, mask);
	}

	return handled;
}

//-----------------------------------------------------------------------
// LLToolCompRotate

LLToolCompRotate::LLToolCompRotate()
	: LLToolComposite("Rotate")
{
	mManip = new LLManipRotate(this);
	mSelectRect = new LLToolSelectRect(this);

	mCur = mManip;
	mDefault = mManip;
}


LLToolCompRotate::~LLToolCompRotate()
{
	delete mManip;
	delete mSelectRect;
}

BOOL LLToolCompRotate::handleHover(S32 x, S32 y, MASK mask)
{
	if( !mCur->hasMouseCapture() )
	{
		setCurrentTool( mManip );
	}
	return mCur->handleHover( x, y, mask );
}


BOOL LLToolCompRotate::handleMouseDown(S32 x, S32 y, MASK mask)
{
	mMouseDown = TRUE;
	gViewerWindow->hitObjectOrLandGlobalAsync(x, y, mask, pickCallback);
	return TRUE;
}

void LLToolCompRotate::pickCallback(S32 x, S32 y, MASK mask)
{
	LLViewerObject* hit_obj = gViewerWindow->lastObjectHit();

	gToolRotate->mManip->highlightManipulators(x, y);
	if (!gToolRotate->mMouseDown)
	{
		// fast click on object, but mouse is already up...just do select
		gToolRotate->mSelectRect->handleObjectSelection(hit_obj, mask, gSavedSettings.getBOOL("EditLinkedParts"), FALSE);
		return;
	}
	
	if( hit_obj || gToolRotate->mManip->getHighlightedPart() != LLManip::LL_NO_PART)
	{
		if (gToolRotate->mManip->getSelection()->getObjectCount())
		{
			gEditMenuHandler = gSelectMgr;
		}
		if(	LLManip::LL_NO_PART != gToolRotate->mManip->getHighlightedPart() )
		{
			gToolRotate->setCurrentTool( gToolRotate->mManip );
			gToolRotate->mManip->handleMouseDownOnPart( x, y, mask );
		}
		else
		{
			gToolRotate->setCurrentTool( gToolRotate->mSelectRect );
			gToolRotate->mSelectRect->handleMouseDown( x, y, mask );
		}
	}
	else
	{
		gToolRotate->setCurrentTool( gToolRotate->mSelectRect );
		gToolRotate->mCur->handleMouseDown( x, y, mask );
	}
}

BOOL LLToolCompRotate::handleMouseUp(S32 x, S32 y, MASK mask)
{
	mMouseDown = FALSE;
	return LLToolComposite::handleMouseUp(x, y, mask);
}

LLTool* LLToolCompRotate::getOverrideTool(MASK mask)
{
	if (mask == (MASK_CONTROL | MASK_SHIFT))
	{
		return gToolStretch;
	}
	return LLToolComposite::getOverrideTool(mask);
}

BOOL LLToolCompRotate::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	if (!mManip->getSelection()->isEmpty() && mManip->getHighlightedPart() == LLManip::LL_NO_PART)
	{
		// You should already have an object selected from the mousedown.
		// If so, show its properties
		gFloaterTools->showPanel(LLFloaterTools::PANEL_CONTENTS);
		//gBuildView->setPropertiesPanelOpen(TRUE);
		return TRUE;
	}
	else
	{
		// Nothing selected means the first mouse click was probably
		// bad, so try again.
		return handleMouseDown(x, y, mask);
	}
}


void LLToolCompRotate::render()
{
	mCur->render();

	if( mCur != mManip )
	{
		LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
		mManip->renderGuidelines();
	}
}


//-----------------------------------------------------------------------
// LLToolCompGun

LLToolCompGun::LLToolCompGun()
	: LLToolComposite("Mouselook")
{
	mGun = new LLToolGun(this);
	mGrab = new LLToolGrab(this);
	mNull = new LLTool("null", this);

	setCurrentTool(mGun);
	mDefault = mGun;
}


LLToolCompGun::~LLToolCompGun()
{
	delete mGun;
	mGun = NULL;

	delete mGrab;
	mGrab = NULL;

	delete mNull;
	mNull = NULL;
}

BOOL LLToolCompGun::handleHover(S32 x, S32 y, MASK mask)
{
	// *NOTE: This hack is here to make mouselook kick in again after
	// item selected from context menu.
	if ( mCur == mNull && !gPopupMenuView->getVisible() )
	{
		gSelectMgr->deselectAll();
		setCurrentTool( (LLTool*) mGrab );
	}

	// Note: if the tool changed, we can't delegate the current mouse event
	// after the change because tools can modify the mouse during selection and deselection.
	// Instead we let the current tool handle the event and then make the change.
	// The new tool will take effect on the next frame.

	mCur->handleHover( x, y, mask );

	// If mouse button not down...
	if( !gViewerWindow->getLeftMouseDown())
	{
		// let ALT switch from gun to grab
		if ( mCur == mGun && (mask & MASK_ALT) )
		{
			setCurrentTool( (LLTool*) mGrab );
		}
		else if ( mCur == mGrab && !(mask & MASK_ALT) )
		{
			setCurrentTool( (LLTool*) mGun );
			setMouseCapture(TRUE);
		}
	}

	return TRUE; 
}


BOOL LLToolCompGun::handleMouseDown(S32 x, S32 y, MASK mask)
{ 
	// if the left button is grabbed, don't put up the pie menu
	if (gAgent.leftButtonGrabbed())
	{
		gAgent.setControlFlags(AGENT_CONTROL_ML_LBUTTON_DOWN);
		return FALSE;
	}

	// On mousedown, start grabbing
	gGrabTransientTool = this;
	gToolMgr->getCurrentToolset()->selectTool( (LLTool*) mGrab );

	return gToolGrab->handleMouseDown(x, y, mask);
}


BOOL LLToolCompGun::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	// if the left button is grabbed, don't put up the pie menu
	if (gAgent.leftButtonGrabbed())
	{
		gAgent.setControlFlags(AGENT_CONTROL_ML_LBUTTON_DOWN);
		return FALSE;
	}

	// On mousedown, start grabbing
	gGrabTransientTool = this;
	gToolMgr->getCurrentToolset()->selectTool( (LLTool*) mGrab );

	return gToolGrab->handleDoubleClick(x, y, mask);
}


BOOL LLToolCompGun::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	/* JC - suppress context menu 8/29/2002

	// On right mouse, go through some convoluted steps to
	// make the build menu appear.
	setCurrentTool( (LLTool*) mNull );

	// This should return FALSE, meaning the context menu will
	// be shown.
	return FALSE;
	*/

	// Returning true will suppress the context menu
	return TRUE;
}


BOOL LLToolCompGun::handleMouseUp(S32 x, S32 y, MASK mask)
{
	gAgent.setControlFlags(AGENT_CONTROL_ML_LBUTTON_UP);
	setCurrentTool( (LLTool*) mGun );
	return TRUE;
}

void LLToolCompGun::onMouseCaptureLost()
{
	if (mComposite)
	{
		mComposite->onMouseCaptureLost();
		return;
	}
	mCur->onMouseCaptureLost();

	// JC - I don't know if this is necessary.  Maybe we could lose capture
	// if someone ALT-Tab's out when in mouselook.
	setCurrentTool( (LLTool*) mGun );
}

void	LLToolCompGun::handleSelect()
{
	LLToolComposite::handleSelect();
	setMouseCapture(TRUE);
}

void	LLToolCompGun::handleDeselect()
{
	LLToolComposite::handleDeselect();
	setMouseCapture(FALSE);
}


BOOL LLToolCompGun::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	if (clicks > 0)
	{
		gAgent.changeCameraToDefault();

	}
	return TRUE;
}
