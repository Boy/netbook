/** 
 * @file lltoolgun.cpp
 * @brief LLToolGun class implementation
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

#include "lltoolgun.h"

#include "llviewerwindow.h"
#include "llagent.h"
#include "llviewercontrol.h"
#include "llsky.h"
#include "llresmgr.h"
#include "llfontgl.h"
#include "llui.h"
#include "llviewerimagelist.h"
#include "llviewercamera.h"
#include "llhudmanager.h"
#include "lltoolmgr.h"
#include "lltoolgrab.h"

LLToolGun::LLToolGun( LLToolComposite* composite )
	:
	LLTool( "gun", composite )
{
	mCrosshairImg = gImageList.getImage( LLUUID( gSavedSettings.getString("UIImgCrosshairsUUID") ), MIPMAP_FALSE, TRUE );
}

void LLToolGun::handleSelect()
{
	gViewerWindow->hideCursor();
	gViewerWindow->moveCursorToCenter();
	gViewerWindow->mWindow->setMouseClipping(TRUE);
}

void LLToolGun::handleDeselect()
{
	gViewerWindow->moveCursorToCenter();
	gViewerWindow->showCursor();
	gViewerWindow->mWindow->setMouseClipping(FALSE);
}

BOOL LLToolGun::handleMouseDown(S32 x, S32 y, MASK mask)
{
	gGrabTransientTool = this;
	gToolMgr->getCurrentToolset()->selectTool( gToolGrab );

	return gToolGrab->handleMouseDown(x, y, mask);
}

BOOL LLToolGun::handleHover(S32 x, S32 y, MASK mask) 
{
	if( gAgent.cameraMouselook() )
	{
		#if 1 //LL_WINDOWS || LL_DARWIN
			const F32 NOMINAL_MOUSE_SENSITIVITY = 0.0025f;
		#else
			const F32 NOMINAL_MOUSE_SENSITIVITY = 0.025f;
		#endif

		
		F32 mouse_sensitivity = clamp_rescale(gMouseSensitivity, 0.f, 15.f, 0.5f, 2.75f) * NOMINAL_MOUSE_SENSITIVITY;

		// ...move the view with the mouse

		// get mouse movement delta
		S32 dx = -gViewerWindow->getCurrentMouseDX();
		S32 dy = -gViewerWindow->getCurrentMouseDY();
		
		if (dx != 0 || dy != 0)
		{
			// ...actually moved off center
			if (gInvertMouse)
			{
				gAgent.pitch(mouse_sensitivity * -dy);
			}
			else
			{
				gAgent.pitch(mouse_sensitivity * dy);
			}
			LLVector3 skyward = gAgent.getReferenceUpVector();
			gAgent.rotate(mouse_sensitivity * dx, skyward.mV[VX], skyward.mV[VY], skyward.mV[VZ]);

			if (gSavedSettings.getBOOL("MouseSun"))
			{
				gSky.setSunDirection(gCamera->getAtAxis(), LLVector3(0.f, 0.f, 0.f));
				gSky.setOverrideSun(TRUE);
				gSavedSettings.setVector3("SkySunDefaultPosition", gCamera->getAtAxis());
			}

			gViewerWindow->moveCursorToCenter();
			gViewerWindow->hideCursor();
		}

		lldebugst(LLERR_USER_INPUT) << "hover handled by LLToolGun (mouselook)" << llendl;
	}
	else
	{
		lldebugst(LLERR_USER_INPUT) << "hover handled by LLToolGun (not mouselook)" << llendl;
	}

	// HACK to avoid assert: error checking system makes sure that the cursor is set during every handleHover.  This is actually a no-op since the cursor is hidden.
	gViewerWindow->setCursor(UI_CURSOR_ARROW);  

	return TRUE;
}

void LLToolGun::draw()
{
	if( gSavedSettings.getBOOL("ShowCrosshairs") )
	{
		gl_draw_image(
			( gViewerWindow->getWindowWidth() - mCrosshairImg->getWidth() ) / 2,
			( gViewerWindow->getWindowHeight() - mCrosshairImg->getHeight() ) / 2,
			mCrosshairImg );
	}
}
