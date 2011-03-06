/** 
 * @file lldynamictexture.cpp
 * @brief Implementation of LLDynamicTexture class
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

#include "lldynamictexture.h"
#include "linked_lists.h"
#include "llimagegl.h"
#include "llglheaders.h"
#include "llviewerwindow.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewerimage.h"
#include "llvertexbuffer.h"


// static
LLLinkedList<LLDynamicTexture> LLDynamicTexture::sInstances[ LLDynamicTexture::ORDER_COUNT ];
S32 LLDynamicTexture::sNumRenders = 0;

//-----------------------------------------------------------------------------
// LLDynamicTexture()
//-----------------------------------------------------------------------------
LLDynamicTexture::LLDynamicTexture(S32 width, S32 height, S32 components, EOrder order, BOOL clamp) : 
	mWidth(width), 
	mHeight(height),
	mComponents(components),
	mTexture(NULL),
	mLastBindTime(0),
	mClamp(clamp)
{
	llassert((1 <= components) && (components <= 4));

	generateGLTexture();

	llassert( 0 <= order && order < ORDER_COUNT );
	LLDynamicTexture::sInstances[ order ].addData(this);
}

//-----------------------------------------------------------------------------
// LLDynamicTexture()
//-----------------------------------------------------------------------------
LLDynamicTexture::~LLDynamicTexture()
{
	releaseGLTexture();
	for( S32 order = 0; order < ORDER_COUNT; order++ )
	{
		LLDynamicTexture::sInstances[order].removeData(this);  // will fail in all but one case.
	}
}

//-----------------------------------------------------------------------------
// releaseGLTexture()
//-----------------------------------------------------------------------------
void LLDynamicTexture::releaseGLTexture()
{
	if (mTexture.notNull())
	{
// 		llinfos << "RELEASING " << (mWidth*mHeight*mComponents)/1024 << "K" << llendl;
		mTexture = NULL;
	}
}

//-----------------------------------------------------------------------------
// generateGLTexture()
//-----------------------------------------------------------------------------
void LLDynamicTexture::generateGLTexture()
{
	generateGLTexture(-1, 0, 0, FALSE);
}

void LLDynamicTexture::generateGLTexture(LLGLint internal_format, LLGLenum primary_format, LLGLenum type_format, BOOL swap_bytes)
{
	if (mComponents < 1 || mComponents > 4)
	{
		llerrs << "Bad number of components in dynamic texture: " << mComponents << llendl;
	}
	releaseGLTexture();
	LLPointer<LLImageRaw> raw_image = new LLImageRaw(mWidth, mHeight, mComponents);
	mTexture = new LLImageGL(mWidth, mHeight, mComponents, FALSE);
	if (internal_format >= 0)
	{
		mTexture->setExplicitFormat(internal_format, primary_format, type_format, swap_bytes);
	}
// 	llinfos << "ALLOCATING " << (mWidth*mHeight*mComponents)/1024 << "K" << llendl;
	mTexture->createGLTexture(0, raw_image);
	mTexture->setClamp(mClamp, mClamp);
}

//-----------------------------------------------------------------------------
// render()
//-----------------------------------------------------------------------------
BOOL LLDynamicTexture::render()
{
	return FALSE;
}

//-----------------------------------------------------------------------------
// preRender()
//-----------------------------------------------------------------------------
void LLDynamicTexture::preRender(BOOL clear_depth)
{
	{
		// force rendering to on-screen portion of frame buffer
		LLCoordScreen window_pos;
		gViewerWindow->getWindow()->getPosition( &window_pos );
		mOrigin.set(0, gViewerWindow->getWindowDisplayHeight() - mHeight);  // top left corner

		if (window_pos.mX < 0)
		{
			mOrigin.mX = -window_pos.mX;
		}
		if (window_pos.mY < 0)
		{
			mOrigin.mY += window_pos.mY;
		}

		LLImageGL::unbindTexture(0, GL_TEXTURE_2D);
	}
	// Set up camera
	mCamera.setOrigin(*gCamera);
	mCamera.setAxes(*gCamera);
	mCamera.setAspect(gCamera->getAspect());
	mCamera.setView(gCamera->getView());
	mCamera.setNear(gCamera->getNear());

	glViewport(mOrigin.mX, mOrigin.mY, mWidth, mHeight);
	if (clear_depth)
	{
		glClear(GL_DEPTH_BUFFER_BIT);
	}
}

//-----------------------------------------------------------------------------
// postRender()
//-----------------------------------------------------------------------------
void LLDynamicTexture::postRender(BOOL success)
{
	{
		if (success)
		{
			success = mTexture->setSubImageFromFrameBuffer(0, 0, mOrigin.mX, mOrigin.mY, mWidth, mHeight);
		}
	}

	// restore viewport
	gViewerWindow->setupViewport();

	// restore camera
	gCamera->setOrigin(mCamera);
	gCamera->setAxes(mCamera);
	gCamera->setAspect(mCamera.getAspect());
	gCamera->setView(mCamera.getView());
	gCamera->setNear(mCamera.getNear());
}

//-----------------------------------------------------------------------------
// bindTexture()
//-----------------------------------------------------------------------------
void LLDynamicTexture::bindTexture()
{
	LLViewerImage::bindTexture(mTexture,0);
}

void LLDynamicTexture::unbindTexture()
{
	LLImageGL::unbindTexture(0, GL_TEXTURE_2D);
}

//-----------------------------------------------------------------------------
// static
// updateDynamicTextures()
// Calls update on each dynamic texture.  Calls each group in order: "first," then "middle," then "last."
//-----------------------------------------------------------------------------
BOOL LLDynamicTexture::updateAllInstances()
{
	sNumRenders = 0;
	if (gGLManager.mIsDisabled)
	{
		return TRUE;
	}

	BOOL result = FALSE;
	for( S32 order = 0; order < ORDER_COUNT; order++ )
	{
		for (LLDynamicTexture *dynamicTexture = LLDynamicTexture::sInstances[order].getFirstData();
			dynamicTexture;
			dynamicTexture = LLDynamicTexture::sInstances[order].getNextData())
		{
			if (dynamicTexture->needsRender())
			{	
				dynamicTexture->preRender();	// Must be called outside of startRender()

				LLVertexBuffer::startRender();
				if (dynamicTexture->render())
				{
					result = TRUE;
					sNumRenders++;
				}
				LLVertexBuffer::stopRender();
		
				dynamicTexture->postRender(result);
			}
		}
	}

	return result;
}

//-----------------------------------------------------------------------------
// static
// destroyGL()
//-----------------------------------------------------------------------------
void LLDynamicTexture::destroyGL()
{
}

//-----------------------------------------------------------------------------
// static
// restoreGL()
//-----------------------------------------------------------------------------
void LLDynamicTexture::restoreGL()
{
}
