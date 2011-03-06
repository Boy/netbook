/** 
 * @file llcheckboxctrl.cpp
 * @brief LLCheckBoxCtrl base class
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

// The mutants are coming!

#include "linden_common.h"

#include "llcheckboxctrl.h"

#include "llgl.h"
#include "llui.h"
#include "lluiconstants.h"
#include "lluictrlfactory.h"
#include "llcontrol.h"

#include "llstring.h"
#include "llfontgl.h"
#include "lltextbox.h"
#include "llkeyboard.h"
#include "llviewborder.h"

const U32 MAX_STRING_LENGTH = 10;

 
LLCheckBoxCtrl::LLCheckBoxCtrl(const LLString& name, const LLRect& rect, 
							    const LLString& label, 
								const LLFontGL* font,
								void (*commit_callback)(LLUICtrl* ctrl, void* userdata),
								void* callback_user_data,
								BOOL initial_value,
								BOOL use_radio_style,
								const LLString& control_which)
:	LLUICtrl(name, rect, TRUE, commit_callback, callback_user_data, FOLLOWS_LEFT | FOLLOWS_TOP),
	mTextEnabledColor( LLUI::sColorsGroup->getColor( "LabelTextColor" ) ),
	mTextDisabledColor( LLUI::sColorsGroup->getColor( "LabelDisabledColor" ) ),
	mRadioStyle( use_radio_style ),
	mInitialValue( initial_value ),
	mSetValue( initial_value )
{
	if (font)
	{
		mFont = font;
	}
	else
	{
		mFont = LLFontGL::sSansSerifSmall;
	}

	// must be big enough to hold all children
	setUseBoundingRect(TRUE);

	mKeyboardFocusOnClick = TRUE;

	// Label (add a little space to make sure text actually renders)
	const S32 FUDGE = 10;
	S32 text_width = mFont->getWidth( label ) + FUDGE;
	S32 text_height = llround(mFont->getLineHeight());
	LLRect label_rect;
	label_rect.setOriginAndSize(
		LLCHECKBOXCTRL_HPAD + LLCHECKBOXCTRL_BTN_SIZE + LLCHECKBOXCTRL_SPACING,
		LLCHECKBOXCTRL_VPAD + 1, // padding to get better alignment
		text_width + LLCHECKBOXCTRL_HPAD,
		text_height );
	mLabel = new LLTextBox( "CheckboxCtrl Label", label_rect, label.c_str(), mFont );
	mLabel->setFollowsLeft();
	mLabel->setFollowsBottom();
	addChild(mLabel);

	// Button
	// Note: button cover the label by extending all the way to the right.
	LLRect btn_rect;
	btn_rect.setOriginAndSize(
		LLCHECKBOXCTRL_HPAD,
		LLCHECKBOXCTRL_VPAD,
		LLCHECKBOXCTRL_BTN_SIZE + LLCHECKBOXCTRL_SPACING + text_width + LLCHECKBOXCTRL_HPAD,
		llmax( text_height, LLCHECKBOXCTRL_BTN_SIZE ) + LLCHECKBOXCTRL_VPAD);
	LLString active_true_id, active_false_id;
	LLString inactive_true_id, inactive_false_id;
	if (mRadioStyle)
	{
		active_true_id = "UIImgRadioActiveSelectedUUID";
		active_false_id = "UIImgRadioActiveUUID";
		inactive_true_id = "UIImgRadioInactiveSelectedUUID";
		inactive_false_id = "UIImgRadioInactiveUUID";
		mButton = new LLButton(
			"Radio control button", btn_rect,
			active_false_id, active_true_id, control_which,
			&LLCheckBoxCtrl::onButtonPress, this, LLFontGL::sSansSerif ); 
		mButton->setDisabledImages( inactive_false_id, inactive_true_id );
		mButton->setHoverGlowStrength(0.35f);
	}
	else
	{
		active_false_id = "UIImgCheckboxActiveUUID";
		active_true_id = "UIImgCheckboxActiveSelectedUUID";
		inactive_true_id = "UIImgCheckboxInactiveSelectedUUID";
		inactive_false_id = "UIImgCheckboxInactiveUUID";
		mButton = new LLButton(
			"Checkbox control button", btn_rect,
			active_false_id, active_true_id, control_which,
			&LLCheckBoxCtrl::onButtonPress, this, LLFontGL::sSansSerif ); 
		mButton->setDisabledImages( inactive_false_id, inactive_true_id );
		mButton->setHoverGlowStrength(0.35f);
	}
	mButton->setIsToggle(TRUE);
	mButton->setToggleState( initial_value );
	mButton->setFollowsLeft();
	mButton->setFollowsBottom();
	mButton->setCommitOnReturn(FALSE);
	addChild(mButton);
}

LLCheckBoxCtrl::~LLCheckBoxCtrl()
{
	// Children all cleaned up by default view destructor.
}


// static
void LLCheckBoxCtrl::onButtonPress( void *userdata )
{
	LLCheckBoxCtrl* self = (LLCheckBoxCtrl*) userdata;

	if (self->mRadioStyle)
	{
		self->setValue(TRUE);
	}

	self->setControlValue(self->getValue());
	// HACK: because buttons don't normally commit
	self->onCommit();

	if (self->mKeyboardFocusOnClick)
	{
		self->setFocus( TRUE );
		self->onFocusReceived();
	}
}

void LLCheckBoxCtrl::onCommit()
{
	if( getEnabled() )
	{
		setTentative(FALSE);
		LLUICtrl::onCommit();
	}
}

void LLCheckBoxCtrl::setEnabled(BOOL b)
{
	LLView::setEnabled(b);
	mButton->setEnabled(b);
}

void LLCheckBoxCtrl::clear()
{
	setValue( FALSE );
}

void LLCheckBoxCtrl::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	//stretch or shrink bounding rectangle of label when rebuilding UI at new scale
	const S32 FUDGE = 10;
	S32 text_width = mFont->getWidth( mLabel->getText() ) + FUDGE;
	S32 text_height = llround(mFont->getLineHeight());
	LLRect label_rect;
	label_rect.setOriginAndSize(
		LLCHECKBOXCTRL_HPAD + LLCHECKBOXCTRL_BTN_SIZE + LLCHECKBOXCTRL_SPACING,
		LLCHECKBOXCTRL_VPAD,
		text_width,
		text_height );
	mLabel->setRect(label_rect);

	LLRect btn_rect;
	btn_rect.setOriginAndSize(
		LLCHECKBOXCTRL_HPAD,
		LLCHECKBOXCTRL_VPAD,
		LLCHECKBOXCTRL_BTN_SIZE + LLCHECKBOXCTRL_SPACING + text_width,
		llmax( text_height, LLCHECKBOXCTRL_BTN_SIZE ) );
	mButton->setRect( btn_rect );
	
	LLUICtrl::reshape(width, height, called_from_parent);
}

void LLCheckBoxCtrl::draw()
{
	if (mEnabled)
	{
		mLabel->setColor( mTextEnabledColor );
	}
	else
	{
		mLabel->setColor( mTextDisabledColor );
	}

	// Draw children
	LLUICtrl::draw();
}

//virtual
void LLCheckBoxCtrl::setValue(const LLSD& value )
{
	mButton->setValue( value );
}

//virtual
LLSD LLCheckBoxCtrl::getValue() const
{
	return mButton->getValue();
}

void LLCheckBoxCtrl::setLabel( const LLStringExplicit& label )
{
	mLabel->setText( label );
	reshape(getRect().getWidth(), getRect().getHeight(), FALSE);
}

LLString LLCheckBoxCtrl::getLabel() const
{
	return mLabel->getText();
}

BOOL LLCheckBoxCtrl::setLabelArg( const LLString& key, const LLStringExplicit& text )
{
	BOOL res = mLabel->setTextArg(key, text);
	reshape(getRect().getWidth(), getRect().getHeight(), FALSE);
	return res;
}

//virtual
LLString LLCheckBoxCtrl::getControlName() const
{
	return mButton->getControlName();
}

// virtual
void LLCheckBoxCtrl::setControlName(const LLString& control_name, LLView* context)
{
	mButton->setControlName(control_name, context);
}


// virtual		Returns TRUE if the user has modified this control.
BOOL	 LLCheckBoxCtrl::isDirty() const
{
	if ( mButton )
	{
		return (mSetValue != mButton->getToggleState());
	}
	return FALSE;		// Shouldn't get here
}


// virtual			Clear dirty state
void	LLCheckBoxCtrl::resetDirty()
{
	if ( mButton )
	{
		mSetValue = mButton->getToggleState();
	}
}



// virtual
LLXMLNodePtr LLCheckBoxCtrl::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLUICtrl::getXML();

	node->createChild("label", TRUE)->setStringValue(mLabel->getText());

	LLString control_name = mButton->getControlName();
	
	node->createChild("initial_value", TRUE)->setBoolValue(mInitialValue);

	node->createChild("font", TRUE)->setStringValue(LLFontGL::nameFromFont(mFont));

	node->createChild("radio_style", TRUE)->setBoolValue(mRadioStyle);

	return node;
}

// static
LLView* LLCheckBoxCtrl::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	LLString name("checkbox");
	node->getAttributeString("name", name);

	LLString label("");
	node->getAttributeString("label", label);

	LLFontGL* font = LLView::selectFont(node);

	BOOL radio_style = FALSE;
	node->getAttributeBOOL("radio_style", radio_style);

	LLUICtrlCallback callback = NULL;

	if (label.empty())
	{
		label.assign(node->getTextContents());
	}

	LLRect rect;
	createRect(node, rect, parent, LLRect());

	LLCheckBoxCtrl* checkbox = new LLCheckboxCtrl(name, 
		rect, 
		label,
		font,
		callback,
		NULL,
		FALSE,
		radio_style); // if true, draw radio button style icons

	BOOL initial_value = checkbox->getValue().asBoolean();
	node->getAttributeBOOL("initial_value", initial_value);

	LLColor4 color;
	color = LLUI::sColorsGroup->getColor( "LabelTextColor" );
	LLUICtrlFactory::getAttributeColor(node,"text_enabled_color", color);
	checkbox->setEnabledColor(color);

	color = LLUI::sColorsGroup->getColor( "LabelDisabledColor" );
	LLUICtrlFactory::getAttributeColor(node,"text_disabled_color", color);
	checkbox->setDisabledColor(color);

	checkbox->setValue(initial_value);

	checkbox->initFromXML(node, parent);

	return checkbox;
}
