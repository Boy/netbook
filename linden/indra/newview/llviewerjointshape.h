/** 
 * @file llviewerjointshape.h
 * @brief Implementation of LLViewerJointShape class
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

#ifndef LL_LLVIEWERJOINTSHAPE_H
#define LL_LLVIEWERJOINTSHAPE_H

#include "llviewerjoint.h"
#include "llviewerimage.h"

//-----------------------------------------------------------------------------
// class LLViewerJointShape
//-----------------------------------------------------------------------------
class LLViewerJointShape :
	public LLViewerJoint
{
public:
	enum ShapeType
	{
		ST_NULL,
		ST_CUBE,
		ST_SPHERE,
		ST_CYLINDER
	};

protected:
	ShapeType		mType;
	LLColor4		mColor;
	LLPointer<LLViewerImage> mTexture;

	static F32		sColorScale;

public:

	// Constructor
	LLViewerJointShape();
	LLViewerJointShape( ShapeType type, F32 red, F32 green, F32 blue, F32 alpha );

	// Destructor
	virtual ~LLViewerJointShape();

	// Gets the shape type
	ShapeType getType();

	// Sets the shape type
	void setType( ShapeType type );

	// Gets the shape color
	void getColor( F32 *red, F32 *green, F32 *blue, F32 *alpha );

	// Sets the color scale factor applied to all subsequent setColor() calls.
	static void setColorScale( F32 factor ) { sColorScale = factor; }

	// Sets the shape color
	void setColor( F32 red, F32 green, F32 blue, F32 alpha );

	// Gets the shape texture
	LLViewerImage *getTexture();

	// Sets the shape texture
	void setTexture( LLViewerImage *texture );

	virtual void drawBone();
	virtual BOOL isTransparent();
	/*virutal*/ BOOL isAnimatable() { return FALSE; }
	/*virtual*/ U32 drawShape( F32 pixelArea, BOOL first_pass );
};

#endif // LL_LLVIEWERJOINTSHAPE_H
