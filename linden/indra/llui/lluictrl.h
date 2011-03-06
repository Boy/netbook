/** 
 * @file lluictrl.h
 * @author James Cook, Richard Nelson, Tom Yedwab
 * @brief Abstract base class for UI controls
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

#ifndef LL_LLUICTRL_H
#define LL_LLUICTRL_H

#include "llview.h"
#include "llrect.h"
#include "llsd.h"

//
// Classes
//
class LLFontGL;
class LLButton;
class LLTextBox;
class LLLineEditor;
class LLUICtrl;
class LLPanel;
class LLCtrlSelectionInterface;
class LLCtrlListInterface;
class LLCtrlScrollInterface;

typedef void (*LLUICtrlCallback)(LLUICtrl* ctrl, void* userdata);
typedef BOOL (*LLUICtrlValidate)(LLUICtrl* ctrl, void* userdata);

class LLFocusableElement
{
	friend class LLFocusMgr; // allow access to focus change handlers
public:
	LLFocusableElement();
	virtual ~LLFocusableElement() {};

	virtual void	setFocus( BOOL b );
	virtual BOOL	hasFocus() const;

	void			setFocusLostCallback(void (*cb)(LLFocusableElement* caller, void*), void* user_data = NULL) { mFocusLostCallback = cb; mFocusCallbackUserData = user_data; }
	void			setFocusReceivedCallback( void (*cb)(LLFocusableElement*, void*), void* user_data = NULL)	{ mFocusReceivedCallback = cb; mFocusCallbackUserData = user_data; }
	void			setFocusChangedCallback( void (*cb)(LLFocusableElement*, void*), void* user_data = NULL )		{ mFocusChangedCallback = cb; mFocusCallbackUserData = user_data; }

protected:
	virtual void	onFocusReceived();
	virtual void	onFocusLost();
	void			(*mFocusLostCallback)( LLFocusableElement* caller, void* userdata );
	void			(*mFocusReceivedCallback)( LLFocusableElement* ctrl, void* userdata );
	void			(*mFocusChangedCallback)( LLFocusableElement* ctrl, void* userdata );
	void*			mFocusCallbackUserData;
};

class LLUICtrl
: public LLView, public LLFocusableElement
{
public:
	LLUICtrl();
	LLUICtrl( const LLString& name, const LLRect& rect, BOOL mouse_opaque,
		LLUICtrlCallback callback,
		void* callback_userdata,
		U32 reshape=FOLLOWS_NONE);
	virtual ~LLUICtrl();

	// LLView interface
	//virtual BOOL	handleToolTip(S32 x, S32 y, LLString& msg, LLRect* sticky_rect);
	virtual void	initFromXML(LLXMLNodePtr node, LLView* parent);
	virtual LLXMLNodePtr getXML(bool save_children = true) const;

	virtual LLSD	getValue() const { return LLSD(); }

	// Defaults to no-op
	virtual BOOL	setTextArg( const LLString& key, const LLStringExplicit& text );

	// Defaults to no-op
	virtual BOOL	setLabelArg( const LLString& key, const LLStringExplicit& text );

	// Defaults to return NULL
	virtual LLCtrlSelectionInterface* getSelectionInterface();
	virtual LLCtrlListInterface* getListInterface();
	virtual LLCtrlScrollInterface* getScrollInterface();

	virtual void	setFocus( BOOL b );
	virtual BOOL	hasFocus() const;

	virtual void	onFocusReceived();
	virtual void	onFocusLost();

	virtual void	onLostTop();	// called when registered as top ctrl and user clicks elsewhere

	virtual void	setTabStop( BOOL b );
	virtual BOOL	hasTabStop() const;

	// Defaults to false
	virtual BOOL	acceptsTextInput() const;

	// Default to no-op
	virtual void	onTabInto();
	virtual void	clear();

	virtual void	setIsChrome(BOOL is_chrome);
	virtual BOOL	getIsChrome() const;

	virtual void	onCommit();

	virtual BOOL	isCtrl() const	{ return TRUE; }
	// "Tentative" controls have a proposed value, but haven't committed
	// it yet.  This is used when multiple objects are selected and we
	// want to display a parameter that differs between the objects.
	virtual void	setTentative(BOOL b);
	virtual BOOL	getTentative() const;

	// Returns containing panel/floater or NULL if none found.
	LLPanel*		getParentPanel() const;

	void*			getCallbackUserData() const								{ return mCallbackUserData; }
	void			setCallbackUserData( void* data )						{ mCallbackUserData = data; }
	
	void			setCommitCallback( void (*cb)(LLUICtrl*, void*) )		{ mCommitCallback = cb; }
	void			setValidateBeforeCommit( BOOL(*cb)(LLUICtrl*, void*) )	{ mValidateCallback = cb; }
	void			setLostTopCallback( void (*cb)(LLUICtrl*, void*) )		{ mLostTopCallback = cb; }

	// Defaults to no-op!
	virtual	void	setDoubleClickCallback( void (*cb)(void*) );

	// Defaults to no-op
	virtual void	setColor(const LLColor4& color);

	// Defaults to no-op
	virtual void	setMinValue(LLSD min_value);
	virtual void	setMaxValue(LLSD max_value);

	/*virtual*/ BOOL focusFirstItem(BOOL prefer_text_fields = FALSE );

	class LLTextInputFilter : public LLQueryFilter, public LLSingleton<LLTextInputFilter>
	{
		/*virtual*/ filterResult_t operator() (const LLView* const view, const viewList_t & children) const 
		{
			return filterResult_t(view->isCtrl() && static_cast<const LLUICtrl *>(view)->acceptsTextInput(), TRUE);
		}
	};

	// Returns TRUE if the user has modified this control.   Editable controls should override this.
	virtual BOOL	isDirty() const			{ return FALSE;		};
	// Clear the dirty state
	virtual void	resetDirty()			{};

protected:

	void			(*mCommitCallback)( LLUICtrl* ctrl, void* userdata );
	void			(*mLostTopCallback)( LLUICtrl* ctrl, void* userdata );
	BOOL			(*mValidateCallback)( LLUICtrl* ctrl, void* userdata );

	void*			mCallbackUserData;
	BOOL			mTentative;
	BOOL			mTabStop;

private:
	BOOL			mIsChrome;


};

#endif  // LL_LLUICTRL_H
