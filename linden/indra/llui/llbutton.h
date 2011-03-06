/** 
 * @file llbutton.h
 * @brief Header for buttons
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

#ifndef LL_LLBUTTON_H
#define LL_LLBUTTON_H

#include "lluuid.h"
#include "llcontrol.h"
#include "lluictrl.h"
#include "v4color.h"
#include "llframetimer.h"
#include "llfontgl.h"
#include "llimage.h"
#include "lluistring.h"

//
// Constants
//

// PLEASE please use these "constants" when building your own buttons.
// They are loaded from settings.xml at run time.
extern S32	LLBUTTON_H_PAD;
extern S32	LLBUTTON_V_PAD;
extern S32	BTN_HEIGHT_SMALL;
extern S32	BTN_HEIGHT;


// All button widths should be rounded up to this size
extern S32	BTN_GRID;


class LLUICtrlFactory;

//
// Classes
//

class LLButton
: public LLUICtrl
{
public:
	// simple button with text label
	LLButton(const LLString& name, const LLRect &rect, const LLString& control_name = LLString(), 
			 void (*on_click)(void*) = NULL, void *data = NULL);

	LLButton(const LLString& name, const LLRect& rect, 
			 const LLString &unselected_image,
			 const LLString &selected_image,
			 const LLString& control_name,	
			 void (*click_callback)(void*),
			 void *callback_data = NULL,
			 const LLFontGL* mGLFont = NULL,
			 const LLString& unselected_label = LLString::null,
			 const LLString& selected_label = LLString::null );

	virtual ~LLButton();
	void init(void (*click_callback)(void*), void *callback_data, const LLFontGL* font, const LLString& control_name);
	virtual EWidgetType getWidgetType() const;
	virtual LLString getWidgetTag() const;
	
	void			addImageAttributeToXML(LLXMLNodePtr node, const LLString& imageName,
										const LLUUID&	imageID,const LLString&	xmlTagName) const;
	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);

	virtual BOOL	handleUnicodeCharHere(llwchar uni_char, BOOL called_from_parent);
	virtual BOOL	handleKeyHere(KEY key, MASK mask, BOOL called_from_parent);
	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);
	virtual void	draw();

	virtual void	onMouseCaptureLost();

	// HACK: "committing" a button is the same as clicking on it.
	virtual void	onCommit();

	void			setUnselectedLabelColor( const LLColor4& c )		{ mUnselectedLabelColor = c; }
	void			setSelectedLabelColor( const LLColor4& c )			{ mSelectedLabelColor = c; }

	void			setClickedCallback( void (*cb)(void *data), void* data = NULL ); // mouse down and up within button
	void			setMouseDownCallback( void (*cb)(void *data) )		{ mMouseDownCallback = cb; }	// mouse down within button
	void			setMouseUpCallback( void (*cb)(void *data) )		{ mMouseUpCallback = cb; }		// mouse up, EVEN IF NOT IN BUTTON
	void			setHeldDownCallback( void (*cb)(void *data) )		{ mHeldDownCallback = cb; }		// Mouse button held down and in button
	void			setHeldDownDelay( F32 seconds, S32 frames = 0)		{ mHeldDownDelay = seconds; mHeldDownFrameDelay = frames; }

	F32				getHeldDownTime() const								{ return mMouseDownTimer.getElapsedTimeF32(); }

	BOOL			getIsToggle() const { return mIsToggle; }
	void			setIsToggle(BOOL is_toggle) { mIsToggle = is_toggle; }
	BOOL			toggleState();
	BOOL			getToggleState() const	{ return mToggleState; }
	void			setToggleState(BOOL b);

	void			setFlashing( BOOL b );
	BOOL			getFlashing() const		{ return mFlashing; }

	void			setHAlign( LLFontGL::HAlign align )		{ mHAlign = align; }
	void			setLeftHPad( S32 pad )					{ mLeftHPad = pad; }
	void			setRightHPad( S32 pad )					{ mRightHPad = pad; }

	const LLString	getLabelUnselected() const { return wstring_to_utf8str(mUnselectedLabel); }
	const LLString	getLabelSelected() const { return wstring_to_utf8str(mSelectedLabel); }

	void			setImageColor(const LLString& color_control);
	void			setImageColor(const LLColor4& c);
	virtual void	setColor(const LLColor4& c);

	void			setImages(const LLString &image_name, const LLString &selected_name);
	void			setDisabledImages(const LLString &image_name, const LLString &selected_name);
	void			setDisabledImages(const LLString &image_name, const LLString &selected_name, const LLColor4& c);
	
	void			setHoverImages(const LLString &image_name, const LLString &selected_name);

	void			setDisabledImageColor(const LLColor4& c)		{ mDisabledImageColor = c; }

	void			setDisabledSelectedLabelColor( const LLColor4& c )	{ mDisabledSelectedLabelColor = c; }

	void			setImageOverlay(const LLString &image_name, LLFontGL::HAlign alignment = LLFontGL::HCENTER, const LLColor4& color = LLColor4::white);
	LLPointer<LLUIImage> getImageOverlay() { return mImageOverlay; }
	

	virtual void	setValue(const LLSD& value );
	virtual LLSD	getValue() const;

	void			setLabel( const LLStringExplicit& label);
	virtual BOOL	setLabelArg( const LLString& key, const LLStringExplicit& text );
	void			setLabelUnselected(const LLStringExplicit& label);
	void			setLabelSelected(const LLStringExplicit& label);
	void			setDisabledLabel(const LLStringExplicit& disabled_label);
	void			setDisabledSelectedLabel(const LLStringExplicit& disabled_label);
	void			setDisabledLabelColor( const LLColor4& c )		{ mDisabledLabelColor = c; }
	
	void			setFont(const LLFontGL *font)		
		{ mGLFont = ( font ? font : LLFontGL::sSansSerif); }
	void			setScaleImage(BOOL scale)			{ mScaleImage = scale; }

	void			setDropShadowedText(BOOL b)			{ mDropShadowedText = b; }

	void			setBorderEnabled(BOOL b)					{ mBorderEnabled = b; }

	static void		onHeldDown(void *userdata);  // to be called by gIdleCallbacks

	void			setHoverGlowStrength(F32 strength) { mHoverGlowStrength = strength; }

public:
	void			setImageUnselected(const LLString &image_name);
	void			setImageSelected(const LLString &image_name);
	void			setImageHoverSelected(const LLString &image_name);
	void			setImageHoverUnselected(const LLString &image_name);
	void			setImageDisabled(const LLString &image_name);
	void			setImageDisabledSelected(const LLString &image_name);

	void			setImageUnselected(LLPointer<LLUIImage> image);
	void			setImageSelected(LLPointer<LLUIImage> image);
	void			setImageHoverSelected(LLPointer<LLUIImage> image);
	void			setImageHoverUnselected(LLPointer<LLUIImage> image);
	void			setImageDisabled(LLPointer<LLUIImage> image);
	void			setImageDisabledSelected(LLPointer<LLUIImage> image);

	void			setCommitOnReturn(BOOL commit) { mCommitOnReturn = commit; }
	BOOL			getCommitOnReturn() { return mCommitOnReturn; }

	void			setHelpURLCallback(std::string help_url);
	LLString		getHelpURL() { return mHelpURL; }
protected:
	virtual void	drawBorder(const LLColor4& color, S32 size);

protected:

	void			(*mClickedCallback)(void* data );
	void			(*mMouseDownCallback)(void *data);
	void			(*mMouseUpCallback)(void *data);
	void			(*mHeldDownCallback)(void *data);

	const LLFontGL	*mGLFont;
	
	LLFrameTimer	mMouseDownTimer;
	S32				mMouseDownFrame;
	F32				mHeldDownDelay;		// seconds, after which held-down callbacks get called
	S32				mHeldDownFrameDelay;	// frames, after which held-down callbacks get called

	LLPointer<LLUIImage>	mImageOverlay;
	LLFontGL::HAlign		mImageOverlayAlignment;
	LLColor4				mImageOverlayColor;

	LLPointer<LLUIImage>	mImageUnselected;
	LLUIString				mUnselectedLabel;
	LLColor4				mUnselectedLabelColor;

	LLPointer<LLUIImage>	mImageSelected;
	LLUIString				mSelectedLabel;
	LLColor4				mSelectedLabelColor;

	LLPointer<LLUIImage>	mImageHoverSelected;

	LLPointer<LLUIImage>	mImageHoverUnselected;

	LLPointer<LLUIImage>	mImageDisabled;
	LLUIString				mDisabledLabel;
	LLColor4				mDisabledLabelColor;

	LLPointer<LLUIImage>	mImageDisabledSelected;
	LLUIString				mDisabledSelectedLabel;
	LLColor4				mDisabledSelectedLabelColor;


	LLUUID			mImageUnselectedID;
	LLUUID			mImageSelectedID;
	LLUUID			mImageHoverSelectedID;
	LLUUID			mImageHoverUnselectedID;
	LLUUID			mImageDisabledID;
	LLUUID			mImageDisabledSelectedID;
	LLString		mImageUnselectedName;
	LLString		mImageSelectedName;
	LLString		mImageHoverSelectedName;
	LLString		mImageHoverUnselectedName;
	LLString		mImageDisabledName;
	LLString		mImageDisabledSelectedName;

	LLColor4		mHighlightColor;
	LLColor4		mUnselectedBgColor;
	LLColor4		mSelectedBgColor;

	LLColor4		mImageColor;
	LLColor4		mDisabledImageColor;

	BOOL			mIsToggle;
	BOOL			mToggleState;
	BOOL			mScaleImage;

	BOOL			mDropShadowedText;

	BOOL			mBorderEnabled;

	BOOL			mFlashing;

	LLFontGL::HAlign mHAlign;
	S32				mLeftHPad;
	S32				mRightHPad;

	F32				mHoverGlowStrength;
	F32				mCurGlowStrength;

	BOOL			mNeedsHighlight;
	BOOL			mCommitOnReturn;

	LLString		mHelpURL;

	LLPointer<LLUIImage> mImagep;

	LLFrameTimer	mFlashingTimer;
};

// Helpful functions
S32 round_up(S32 grid, S32 value);

#endif  // LL_LLBUTTON_H
