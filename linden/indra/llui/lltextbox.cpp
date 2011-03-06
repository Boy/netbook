/** 
 * @file lltextbox.cpp
 * @brief A text display widget
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

#include "linden_common.h"

#include "lltextbox.h"

#include "llerror.h"
#include "llgl.h"
#include "llui.h"
#include "lluictrlfactory.h"
#include "llcontrol.h"
#include "llfocusmgr.h"
#include "llstl.h"
#include <boost/tokenizer.hpp>

LLTextBox::LLTextBox(const LLString& name, const LLRect& rect, const LLString& text,
					 const LLFontGL* font, BOOL mouse_opaque)
:	LLUICtrl(name, rect, mouse_opaque, NULL, NULL, FOLLOWS_LEFT | FOLLOWS_TOP ),
	mTextColor(			LLUI::sColorsGroup->getColor( "LabelTextColor" ) ),
	mDisabledColor(		LLUI::sColorsGroup->getColor( "LabelDisabledColor" ) ),
	mBackgroundColor(	LLUI::sColorsGroup->getColor( "DefaultBackgroundColor" ) ),
	mBorderColor(		LLUI::sColorsGroup->getColor( "DefaultHighlightLight" ) ),
	mHoverColor(		LLUI::sColorsGroup->getColor( "LabelSelectedColor" ) ),
	mHoverActive( FALSE ),
	mHasHover( FALSE ),
	mBackgroundVisible( FALSE ),
	mBorderVisible( FALSE ),
	mFontStyle(LLFontGL::DROP_SHADOW_SOFT),
	mBorderDropShadowVisible( FALSE ),
	mUseEllipses( FALSE ),
	mHPad(0),
	mVPad(0),
	mHAlign( LLFontGL::LEFT ),
	mVAlign( LLFontGL::TOP ),
	mClickedCallback(NULL),
	mCallbackUserData(NULL)
{
	// TomY TODO Nuke this eventually
	setText( !text.empty() ? text : name );
	mFontGL = font ? font : LLFontGL::sSansSerifSmall;
	setTabStop(FALSE);
}

LLTextBox::LLTextBox(const LLString& name, const LLString& text, F32 max_width,
					 const LLFontGL* font, BOOL mouse_opaque) :
	LLUICtrl(name, LLRect(0, 0, 1, 1), mouse_opaque, NULL, NULL, FOLLOWS_LEFT | FOLLOWS_TOP),	
	mFontGL(font ? font : LLFontGL::sSansSerifSmall),
	mTextColor(LLUI::sColorsGroup->getColor("LabelTextColor")),
	mDisabledColor(LLUI::sColorsGroup->getColor("LabelDisabledColor")),
	mBackgroundColor(LLUI::sColorsGroup->getColor("DefaultBackgroundColor")),
	mBorderColor(LLUI::sColorsGroup->getColor("DefaultHighlightLight")),
	mHoverColor( LLUI::sColorsGroup->getColor( "LabelSelectedColor" ) ),
	mHoverActive( FALSE ),
	mHasHover( FALSE ),
	mBackgroundVisible(FALSE),
	mBorderVisible(FALSE),
	mFontStyle(LLFontGL::DROP_SHADOW_SOFT),
	mBorderDropShadowVisible(FALSE),
	mUseEllipses( FALSE ),
	mHPad(0),
	mVPad(0),
	mHAlign(LLFontGL::LEFT),
	mVAlign( LLFontGL::TOP ),
	mClickedCallback(NULL),
	mCallbackUserData(NULL)
{
	setWrappedText(!text.empty() ? text : name, max_width);
	reshapeToFitText();
	setTabStop(FALSE);
}

LLTextBox::~LLTextBox()
{
}

// virtual
EWidgetType LLTextBox::getWidgetType() const
{
	return WIDGET_TYPE_TEXT_BOX;
}

// virtual
LLString LLTextBox::getWidgetTag() const
{
	return LL_TEXT_BOX_TAG;
}

BOOL LLTextBox::handleMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;

	// HACK: Only do this if there actually is a click callback, so that
	// overly large text boxes in the older UI won't start eating clicks.
	if (mClickedCallback)
	{
		handled = TRUE;

		// Route future Mouse messages here preemptively.  (Release on mouse up.)
		gFocusMgr.setMouseCapture( this );
		
		if (mSoundFlags & MOUSE_DOWN)
		{
			make_ui_sound("UISndClick");
		}
	}

	return handled;
}


BOOL LLTextBox::handleMouseUp(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;

	// We only handle the click if the click both started and ended within us

	// HACK: Only do this if there actually is a click callback, so that
	// overly large text boxes in the older UI won't start eating clicks.
	if (mClickedCallback
		&& hasMouseCapture())
	{
		handled = TRUE;

		// Release the mouse
		gFocusMgr.setMouseCapture( NULL );

		if (mSoundFlags & MOUSE_UP)
		{
			make_ui_sound("UISndClickRelease");
		}

		// DO THIS AT THE VERY END to allow the button to be destroyed as a result of being clicked.
		// If mouseup in the widget, it's been clicked
		if (mClickedCallback)
		{
			(*mClickedCallback)( mCallbackUserData );
		}
	}

	return handled;
}

BOOL LLTextBox::handleHover(S32 x, S32 y, MASK mask)
{
	if(mHoverActive)
	{
		mHasHover = TRUE; // This should be set every frame during a hover.
		return TRUE;
	}
	return LLView::handleHover(x,y,mask);
}

void LLTextBox::setText(const LLStringExplicit& text)
{
	mText.assign(text);
	setLineLengths();
}

void LLTextBox::setLineLengths()
{
	mLineLengthList.clear();
	
	LLString::size_type  cur = 0;
	LLString::size_type  len = mText.getWString().size();

	while (cur < len)
	{
		LLString::size_type end = mText.getWString().find('\n', cur);
		LLString::size_type runLen;
		
		if (end == LLString::npos)
		{
			runLen = len - cur;
			cur = len;
		}
		else
		{
			runLen = end - cur;
			cur = end + 1; // skip the new line character
		}

		mLineLengthList.push_back( (S32)runLen );
	}
}

void LLTextBox::setWrappedText(const LLStringExplicit& in_text, F32 max_width)
{
	if (max_width < 0.0)
	{
		max_width = (F32)getRect().getWidth();
	}

	LLWString wtext = utf8str_to_wstring(in_text);
	LLWString final_wtext;

	LLWString::size_type  cur = 0;;
	LLWString::size_type  len = wtext.size();

	while (cur < len)
	{
		LLWString::size_type end = wtext.find('\n', cur);
		if (end == LLWString::npos)
		{
			end = len;
		}
		
		LLWString::size_type runLen = end - cur;
		if (runLen > 0)
		{
			LLWString run(wtext, cur, runLen);
			LLWString::size_type useLen =
				mFontGL->maxDrawableChars(run.c_str(), max_width, runLen, TRUE);

			final_wtext.append(wtext, cur, useLen);
			cur += useLen;
		}

		if (cur < len)
		{
			if (wtext[cur] == '\n')
			{
				cur += 1;
			}
			final_wtext += '\n';
		}
	}

	LLString final_text = wstring_to_utf8str(final_wtext);
	setText(final_text);
}

S32 LLTextBox::getTextPixelWidth()
{
	S32 max_line_width = 0;
	if( mLineLengthList.size() > 0 )
	{
		S32 cur_pos = 0;
		for (std::vector<S32>::iterator iter = mLineLengthList.begin();
			iter != mLineLengthList.end(); ++iter)
		{
			S32 line_length = *iter;
			S32 line_width = mFontGL->getWidth( mText.getWString().c_str(), cur_pos, line_length );
			if( line_width > max_line_width )
			{
				max_line_width = line_width;
			}
			cur_pos += line_length+1;
		}
	}
	else
	{
		max_line_width = mFontGL->getWidth(mText.getWString().c_str());
	}
	return max_line_width;
}

S32 LLTextBox::getTextPixelHeight()
{
	S32 num_lines = mLineLengthList.size();
	if( num_lines < 1 )
	{
		num_lines = 1;
	}
	return (S32)(num_lines * mFontGL->getLineHeight());
}


void LLTextBox::setValue(const LLSD& value )
{
	setText(value.asString());
}

LLSD LLTextBox::getValue() const
{
	return LLSD(getText());
}

BOOL LLTextBox::setTextArg( const LLString& key, const LLStringExplicit& text )
{
	mText.setArg(key, text);
	setLineLengths();
	return TRUE;
}

void LLTextBox::draw()
{
	if( getVisible() )
	{
		if (mBorderVisible)
		{
			gl_rect_2d_offset_local(getLocalRect(), 2, FALSE);
		}

		if( mBorderDropShadowVisible )
		{
			static LLColor4 color_drop_shadow = LLUI::sColorsGroup->getColor("ColorDropShadow");
			static S32 drop_shadow_tooltip = LLUI::sConfigGroup->getS32("DropShadowTooltip");
			gl_drop_shadow(0, mRect.getHeight(), mRect.getWidth(), 0,
				color_drop_shadow, drop_shadow_tooltip);
		}
	
		if (mBackgroundVisible)
		{
			LLRect r( 0, mRect.getHeight(), mRect.getWidth(), 0 );
			gl_rect_2d( r, mBackgroundColor );
		}

		S32 text_x = 0;
		switch( mHAlign )
		{
		case LLFontGL::LEFT:	
			text_x = mHPad;						
			break;
		case LLFontGL::HCENTER:
			text_x = mRect.getWidth() / 2;
			break;
		case LLFontGL::RIGHT:
			text_x = mRect.getWidth() - mHPad;
			break;
		}

		S32 text_y = mRect.getHeight() - mVPad;

		if ( getEnabled() )
		{
			if(mHasHover)
			{
				drawText( text_x, text_y, mHoverColor );
			}
			else
			{
				drawText( text_x, text_y, mTextColor );
			}				

		}
		else
		{
			drawText( text_x, text_y, mDisabledColor );
		}

		if (sDebugRects)
		{
			drawDebugRect();
		}
	}

	mHasHover = FALSE; // This is reset every frame.
}

void LLTextBox::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	// reparse line lengths
	setLineLengths();
	LLView::reshape(width, height, called_from_parent);
}

void LLTextBox::drawText( S32 x, S32 y, const LLColor4& color )
{
	if( !mLineLengthList.empty() )
	{
		S32 cur_pos = 0;
		for (std::vector<S32>::iterator iter = mLineLengthList.begin();
			iter != mLineLengthList.end(); ++iter)
		{
			S32 line_length = *iter;
			mFontGL->render(mText.getWString(), cur_pos, (F32)x, (F32)y, color,
							mHAlign, mVAlign,
							mFontStyle,
							line_length, mRect.getWidth(), NULL, TRUE, mUseEllipses );
			cur_pos += line_length + 1;
			y -= llfloor(mFontGL->getLineHeight());
		}
	}
	else
	{
		mFontGL->render(mText.getWString(), 0, (F32)x, (F32)y, color,
						mHAlign, mVAlign, 
						mFontStyle,
						S32_MAX, mRect.getWidth(), NULL, TRUE, mUseEllipses);
	}
}


void LLTextBox::reshapeToFitText()
{
	S32 width = getTextPixelWidth();
	S32 height = getTextPixelHeight();
	reshape( width + 2 * mHPad, height + 2 * mVPad );
}

// virtual
LLXMLNodePtr LLTextBox::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLUICtrl::getXML();

	// Attributes

	node->createChild("font", TRUE)->setStringValue(LLFontGL::nameFromFont(mFontGL));

	node->createChild("halign", TRUE)->setStringValue(LLFontGL::nameFromHAlign(mHAlign));

	addColorXML(node, mTextColor, "text_color", "LabelTextColor");
	addColorXML(node, mDisabledColor, "disabled_color", "LabelDisabledColor");
	addColorXML(node, mBackgroundColor, "bg_color", "DefaultBackgroundColor");
	addColorXML(node, mBorderColor, "border_color", "DefaultHighlightLight");

	node->createChild("bg_visible", TRUE)->setBoolValue(mBackgroundVisible);

	node->createChild("border_visible", TRUE)->setBoolValue(mBorderVisible);

	node->createChild("border_drop_shadow_visible", TRUE)->setBoolValue(mBorderDropShadowVisible);

	node->createChild("h_pad", TRUE)->setIntValue(mHPad);

	node->createChild("v_pad", TRUE)->setIntValue(mVPad);

	// Contents

	node->setStringValue(mText);

	return node;
}

// static
LLView* LLTextBox::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	LLString name("text_box");
	node->getAttributeString("name", name);
	LLFontGL* font = LLView::selectFont(node);

	LLString text = node->getTextContents();

	// TomY Yes I know this is a hack, but insert a space to make a blank text field
	if (text == "")
	{
		text = " ";
	}

	LLTextBox* text_box = new LLTextBox(name,
		LLRect(),
		text,
		font,
		FALSE);
		

	LLFontGL::HAlign halign = LLView::selectFontHAlign(node);
	text_box->setHAlign(halign);

	text_box->initFromXML(node, parent);

	LLString font_style;
	if (node->getAttributeString("font-style", font_style))
	{
		text_box->mFontStyle = LLFontGL::getStyleFromString(font_style);
	}
	
	BOOL mouse_opaque = text_box->getMouseOpaque();
	if (node->getAttributeBOOL("mouse_opaque", mouse_opaque))
	{
		text_box->setMouseOpaque(mouse_opaque);
	}	

	if(node->hasAttribute("text_color"))
	{
		LLColor4 color;
		LLUICtrlFactory::getAttributeColor(node, "text_color", color);
		text_box->setColor(color);
	}

	if(node->hasAttribute("hover_color"))
	{
		LLColor4 color;
		LLUICtrlFactory::getAttributeColor(node, "hover_color", color);
		text_box->setHoverColor(color);
		text_box->setHoverActive(true);
	}

	BOOL hover_active = FALSE;
	if(node->getAttributeBOOL("hover", hover_active))
	{
		text_box->setHoverActive(hover_active);
	}


	return text_box;
}
