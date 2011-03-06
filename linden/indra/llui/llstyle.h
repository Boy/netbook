/** 
 * @file llstyle.h
 * @brief Text style class
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

#ifndef LL_LLSTYLE_H
#define LL_LLSTYLE_H

#include "v4color.h"
#include "llresmgr.h"
#include "llfont.h"
#include "llimagegl.h"

class LLStyle
{
public:
	LLStyle();
	LLStyle(const LLStyle &style);
	LLStyle(BOOL is_visible, const LLColor4 &color, const LLString& font_name);

	LLStyle &operator=(const LLStyle &rhs);

	virtual ~LLStyle();

	virtual void init (BOOL is_visible, const LLColor4 &color, const LLString& font_name);
	virtual void free ();

	bool operator==(const LLStyle &rhs) const;
	bool operator!=(const LLStyle &rhs) const;

	virtual const LLColor4& getColor() const;
	virtual void setColor(const LLColor4 &color);

	virtual BOOL isVisible() const;
	virtual void setVisible(BOOL is_visible);

	virtual const LLString& getFontString() const;
	virtual void setFontName(const LLString& fontname);
	virtual LLFONT_ID getFontID() const;

	virtual const LLString& getLinkHREF() const;
	virtual void setLinkHREF(const LLString& fontname);
	virtual BOOL isLink() const;

	virtual LLImageGL *getImage() const;
	virtual void setImage(const LLString& src);
	virtual BOOL isImage() const;
	virtual void setImageSize(S32 width, S32 height);

	BOOL	getIsEmbeddedItem() const	{ return mIsEmbeddedItem; }
	void	setIsEmbeddedItem( BOOL b ) { mIsEmbeddedItem = b; }

public:	
	BOOL        mItalic;
	BOOL        mBold;
	BOOL        mUnderline;
	BOOL		mDropShadow;
	S32         mImageWidth;
	S32         mImageHeight;

protected:
	BOOL		mVisible;
	LLColor4	mColor;
	LLString	mFontName;
	LLFONT_ID   mFontID;
	LLString	mLink;
	LLPointer<LLImageGL> mImagep;

	BOOL		mIsEmbeddedItem;
};

#endif  // LL_LLSTYLE_H
