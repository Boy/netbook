/** 
 * @file llchatbar.cpp
 * @brief LLChatBar class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2008, Linden Research, Inc.
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

//MK
#include "linden_common.h"
//mk

#include "llviewerprecompiledheaders.h"

#include "llchatbar.h"

#include "imageids.h"
#include "llfontgl.h"
#include "llrect.h"
#include "llerror.h"
#include "llparcel.h"
#include "llstring.h"
#include "message.h"
#include "llfocusmgr.h"

#include "llagent.h"
#include "llbutton.h"
#include "llcombobox.h"
#include "llviewercontrol.h"
#include "llfloaterchat.h"
#include "llgesturemgr.h"
#include "llkeyboard.h"
#include "lllineeditor.h"
#include "llstatusbar.h"
#include "lltextbox.h"
#include "lluiconstants.h"
#include "llviewergesture.h"			// for triggering gestures
#include "llviewermenu.h"		// for deleting object with DEL key
#include "llviewerstats.h"
#include "llviewerwindow.h"
#include "llframetimer.h"
#include "llresmgr.h"
#include "llworld.h"
#include "llinventorymodel.h"
#include "llmultigesture.h"
#include "llui.h"
#include "llviewermenu.h"
#include "llvieweruictrlfactory.h"

//MK
extern BOOL RRenabled;
//mk

//
// Globals
//
const F32 AGENT_TYPING_TIMEOUT = 5.f;	// seconds

LLChatBar *gChatBar = NULL;

// legacy calllback glue
void toggleChatHistory(void* user_data);


class LLChatBarGestureObserver : public LLGestureManagerObserver
{
public:
	LLChatBarGestureObserver(LLChatBar* chat_barp) : mChatBar(chat_barp){}
	virtual ~LLChatBarGestureObserver() {}
	virtual void changed() { mChatBar->refreshGestures(); }
private:
	LLChatBar* mChatBar;
};


//
// Functions
//

//inline constructor
// for chat bars embedded in floaters, etc
LLChatBar::LLChatBar(const std::string& name) 
:	LLPanel(name, LLRect(), BORDER_NO),
	mInputEditor(NULL),
	mGestureLabelTimer(),
	mLastSpecialChatChannel(0),
	mIsBuilt(FALSE),
	mDynamicLayout(FALSE),
	mGestureCombo(NULL),
	mObserver(NULL)
{
}

LLChatBar::LLChatBar(const std::string& name, const LLRect& rect) 
:	LLPanel(name, rect, BORDER_NO),
	mInputEditor(NULL),
	mGestureLabelTimer(),
	mLastSpecialChatChannel(0),
	mIsBuilt(FALSE),
	mDynamicLayout(TRUE),
	mGestureCombo(NULL),
	mObserver(NULL)
{
	setIsChrome(TRUE);
	
	gUICtrlFactory->buildPanel(this,"panel_chat_bar.xml");
	
	mIsFocusRoot = TRUE;

	setRect(rect); // override xml rect
	
	setBackgroundOpaque(TRUE);
	setBackgroundVisible(TRUE);

	// Start visible if we left the app while chatting.
	setVisible( gSavedSettings.getBOOL("ChatVisible") );

	// Apply custom layout.
	layout();

#if !LL_RELEASE_FOR_DOWNLOAD
	childDisplayNotFound();
#endif
	
}


LLChatBar::~LLChatBar()
{
	delete mObserver;
	mObserver = NULL;
	// LLView destructor cleans up children
}

BOOL LLChatBar::postBuild()
{
	childSetAction("History", toggleChatHistory, this);
	childSetAction("Say", onClickSay, this);
	childSetAction("Shout", onClickShout, this);

	// attempt to bind to an existing combo box named gesture
	setGestureCombo(LLUICtrlFactory::getComboBoxByName(this, "Gesture"));

	LLButton * sayp = static_cast<LLButton*>(getChildByName("Say", TRUE));
	if(sayp)
	{
		setDefaultBtn(sayp);
	}

	mInputEditor = LLUICtrlFactory::getLineEditorByName(this, "Chat Editor");
	if (mInputEditor)
	{
		mInputEditor->setCallbackUserData(this);
		mInputEditor->setKeystrokeCallback(&onInputEditorKeystroke);
		mInputEditor->setFocusLostCallback(&onInputEditorFocusLost, this);
		mInputEditor->setFocusReceivedCallback( &onInputEditorGainFocus, this );
		mInputEditor->setCommitOnFocusLost( FALSE );
		mInputEditor->setRevertOnEsc( FALSE );
		mInputEditor->setIgnoreTab(TRUE);
		mInputEditor->setPassDelete(TRUE);

		mInputEditor->setMaxTextLength(1023);
		mInputEditor->setEnableLineHistory(TRUE);
	}

	mIsBuilt = TRUE;

	return TRUE;
}

//-----------------------------------------------------------------------
// Overrides
//-----------------------------------------------------------------------

// virtual
void LLChatBar::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLPanel::reshape(width, height, called_from_parent);
	if (mIsBuilt)
	{
		layout();
	}
}

// virtual
BOOL LLChatBar::handleKeyHere( KEY key, MASK mask, BOOL called_from_parent )
{
	BOOL handled = FALSE;

	if( getVisible() && getEnabled() && !called_from_parent)
	{
		// ALT-RETURN is reserved for windowed/fullscreen toggle
		if( KEY_RETURN == key )
		{
			//if (childGetValue("Chat Editor").asString().empty())
			//{
			//	// no text, just close chat bar
			//	stopChat();
			//	return TRUE;
			//}

			if (mask == MASK_CONTROL)
			{
				// shout
				sendChat(CHAT_TYPE_SHOUT);
				handled = TRUE;
			}
			else if (mask == MASK_SHIFT)
			{
				// whisper
				sendChat( CHAT_TYPE_WHISPER );
				handled = TRUE;
			}
			else if (mask == MASK_NONE)
			{
				// say
				sendChat( CHAT_TYPE_NORMAL );
				handled = TRUE;
			}
		}
		// only do this in main chatbar
		else if ( KEY_ESCAPE == key && mask == MASK_NONE && gChatBar == this)
		{
			stopChat();

			handled = TRUE;
		}
	}
	return handled;
}


void LLChatBar::layout()
{
	if (!mDynamicLayout) return;

	S32 rect_width = mRect.getWidth();
	S32 pad = 4;

	LLRect gesture_rect;
	S32 gesture_width = 0;
	if (childGetRect("Gesture", gesture_rect))
	{
		gesture_width = gesture_rect.getWidth();
	}
	S32 input_width = 0;
	S32 btn_width = 64;
	S32 segment_width = btn_width + pad;
	S32 x = pad;
	S32 y = 3;
	LLRect r;

	r.setOriginAndSize(x, y, btn_width, BTN_HEIGHT);
	childSetRect("History", r);

	x += segment_width;
	// Hack this one up so it looks nice.
	if (mInputEditor)
	{
		input_width = rect_width - 3 * segment_width - 3 * pad - gesture_width;
		r.setOriginAndSize(x, y + 2, input_width, 18);
		mInputEditor->reshape(r.getWidth(), r.getHeight(), TRUE);
		mInputEditor->setRect(r);
	}

	x += input_width + pad;
	r.setOriginAndSize(x, y, btn_width, BTN_HEIGHT);
	childSetRect("Say", r);

	x += segment_width;
	r.setOriginAndSize(x, y, btn_width, BTN_HEIGHT);
	childSetRect("Shout", r);

	x = rect_width - (pad + gesture_width);
	r.setOriginAndSize(x, y, gesture_width, BTN_HEIGHT);
	childSetRect("Gesture", r);
}


void LLChatBar::refresh()
{
	//BOOL chat_mode = gSavedSettings.getBOOL("ChatVisible");

	//// Grab focus when no one else has it, and we're in chat mode.
	//if (!gFocusMgr.getKeyboardFocus()
	//	&& chat_mode)
	//{
	//	childSetFocus("Chat Editor", TRUE);
	//}

	// Only show this view when user wants to be chatting
	//setVisible(chat_mode);

	// hide in mouselook, but keep previous visibility state
	//BOOL mouselook = gAgent.cameraMouselook();
	// call superclass setVisible so that we don't overwrite the saved setting
	if (mDynamicLayout)
	{
		LLPanel::setVisible(gSavedSettings.getBOOL("ChatVisible"));
	}

	// HACK: Leave the name of the gesture in place for a few seconds.
	const F32 SHOW_GESTURE_NAME_TIME = 2.f;
	if (mGestureLabelTimer.getStarted() && mGestureLabelTimer.getElapsedTimeF32() > SHOW_GESTURE_NAME_TIME)
	{
		LLCtrlListInterface* gestures = mGestureCombo ? mGestureCombo->getListInterface() : NULL;
		if (gestures) gestures->selectFirstItem();
		mGestureLabelTimer.stop();
	}

	if ((gAgent.getTypingTime() > AGENT_TYPING_TIMEOUT) && (gAgent.getRenderState() & AGENT_STATE_TYPING))
	{
		gAgent.stopTyping();
	}

	childSetValue("History", LLFloaterChat::instanceVisible(LLSD()));

	if (mInputEditor)
	{
		childSetEnabled("Say", mInputEditor->getText().size() > 0);
		childSetEnabled("Shout", mInputEditor->getText().size() > 0);
	}
}

void LLChatBar::refreshGestures()
{
	LLCtrlListInterface* gestures = mGestureCombo ? mGestureCombo->getListInterface() : NULL;
	if (mGestureCombo && gestures)
	{
		//store current selection so we can maintain it
		LLString cur_gesture = mGestureCombo->getValue().asString();
		gestures->selectFirstItem();
		LLString label = mGestureCombo->getValue().asString();;
		// clear
		gestures->clearRows();

		// collect list of unique gestures
		std::map <std::string, BOOL> unique;
		LLGestureManager::item_map_t::iterator it;
		for (it = gGestureManager.mActive.begin(); it != gGestureManager.mActive.end(); ++it)
		{
			LLMultiGesture* gesture = (*it).second;
			if (gesture)
			{
				if (!gesture->mTrigger.empty())
				{
					unique[gesture->mTrigger] = TRUE;
				}
			}
		}

		// ad unique gestures
		std::map <std::string, BOOL>::iterator it2;
		for (it2 = unique.begin(); it2 != unique.end(); ++it2)
		{
			gestures->addSimpleElement((*it2).first);
		}
		
		gestures->sortByColumn(0, TRUE);
		// Insert label after sorting
		gestures->addSimpleElement(label, ADD_TOP);
		
		if (!cur_gesture.empty())
		{
			gestures->selectByValue(LLSD(cur_gesture));
		}
		else
		{
			gestures->selectFirstItem();
		}
	}
}

// Move the cursor to the correct input field.
void LLChatBar::setKeyboardFocus(BOOL focus)
{
	if (focus)
	{
		if (mInputEditor)
		{
			mInputEditor->setFocus(TRUE);
			mInputEditor->selectAll();
		}
	}
	else if (gFocusMgr.childHasKeyboardFocus(this))
	{
		if (mInputEditor)
		{
			mInputEditor->deselect();
		}
		setFocus(FALSE);
	}
}


// Ignore arrow keys in chat bar
void LLChatBar::setIgnoreArrowKeys(BOOL b)
{
	if (mInputEditor)
	{
		mInputEditor->setIgnoreArrowKeys(b);
	}
}

BOOL LLChatBar::inputEditorHasFocus()
{
	return mInputEditor && mInputEditor->hasFocus();
}

LLString LLChatBar::getCurrentChat()
{
	return mInputEditor ? mInputEditor->getText() : LLString::null;
}

void LLChatBar::setGestureCombo(LLComboBox* combo)
{
	mGestureCombo = combo;
	if (mGestureCombo)
	{
		mGestureCombo->setCommitCallback(onCommitGesture);
		mGestureCombo->setCallbackUserData(this);

		// now register observer since we have a place to put the results
		mObserver = new LLChatBarGestureObserver(this);
		gGestureManager.addObserver(mObserver);

		// refresh list from current active gestures
		refreshGestures();
	}
}

//-----------------------------------------------------------------------
// Internal functions
//-----------------------------------------------------------------------

// If input of the form "/20foo" or "/20 foo", returns "foo" and channel 20.
// Otherwise returns input and channel 0.
LLWString LLChatBar::stripChannelNumber(const LLWString &mesg, S32* channel)
{
	if (mesg[0] == '/'
		&& mesg[1] == '/')
	{
		// This is a "repeat channel send"
		*channel = mLastSpecialChatChannel;
		return mesg.substr(2, mesg.length() - 2);
	}
	else if (mesg[0] == '/'
			 && mesg[1]
			 && LLStringOps::isDigit(mesg[1]))
	{
		// This a special "/20" speak on a channel
		S32 pos = 0;

		// Copy the channel number into a string
		llwchar channel_string[64];
		llwchar c;
		do
		{
			c = mesg[pos+1];
			channel_string[pos] = c;
			pos++;
		}
		while(c && pos < 64 && LLStringOps::isDigit(c));
		
		// Move the pointer forward to the first non-whitespace char
		// Check isspace before looping, so we can handle "/33foo"
		// as well as "/33 foo"
		while(c && iswspace(c))
		{
			c = mesg[pos+1];
			pos++;
		}

		
		mLastSpecialChatChannel = strtol(wstring_to_utf8str(channel_string).c_str(), NULL, 10);
		*channel = mLastSpecialChatChannel;
		return mesg.substr(pos, mesg.length() - pos);
	}
	else
	{
		// This is normal chat.
		*channel = 0;
		return mesg;
	}
}


void LLChatBar::sendChat( EChatType type )
{
	LLWString text;
	if (mInputEditor) text = mInputEditor->getWText();
	LLWString::trim(text);

	if (!text.empty())
	{
		// store sent line in history, duplicates will get filtered
		if (mInputEditor) mInputEditor->updateHistory();
		// Check if this is destined for another channel
		S32 channel = 0;
		stripChannelNumber(text, &channel);

		std::string utf8text = wstring_to_utf8str(text);
		// Try to trigger a gesture, if not chat to a script.
		std::string utf8_revised_text;
		if (0 == channel)
		{
			if (gSavedSettings.getBOOL("AutoCloseOOC"))
			{
				// Try to find any unclosed OOC chat (i.e. an opening
				// double parenthesis without a matching closing double
				// parenthesis.
				if (utf8text.find("((") != -1 && utf8text.find("))") == -1)
				{
					if (utf8text.at(utf8text.length() - 1) == ')')
					{
						// cosmetic: add a space first to avoid a closing triple parenthesis
						utf8text += " ";
					}
					// add the missing closing double parenthesis.
					utf8text += "))";
				}
			}

			// Convert MU*s style poses into IRC emotes here.
			if (gSavedSettings.getBOOL("AllowMUpose") && utf8text.find(":") == 0 && utf8text.length() > 3)
			{
				if (utf8text.find(":'") == 0)
				{
					utf8text.replace(0, 1, "/me");
 				}
				else if (isalpha(utf8text.at(1)))	// Do not prevent smileys and such.
				{
					utf8text.replace(0, 1, "/me ");
				}
			}
//MK
////			// discard returned "found" boolean
////			gGestureManager.triggerAndReviseString(utf8text, &utf8_revised_text);
			BOOL found_gesture=gGestureManager.triggerAndReviseString(utf8text, &utf8_revised_text);

			if (RRenabled && gAgent.mRRInterface.contains ("sendchat") && !gAgent.mRRInterface.containsSubstr ("redirchat:"))
			{
				// user is forbidden to send any chat message on channel 0 except emotes and OOC text
				utf8_revised_text = gAgent.mRRInterface.crunchEmote (utf8_revised_text, 20);
				if (found_gesture && utf8_revised_text=="...") utf8_revised_text="";
			}
//mk
		}
		else
		{
//MK
			std::ostringstream stream;
			stream << "sendchannel:" << channel;
			if (RRenabled && gAgent.mRRInterface.contains ("sendchannel") && // user prevented from chatting on private channels
				!gAgent.mRRInterface.contains (stream.str ())) { // and this channel is no exception
				utf8_revised_text = "";
			}
			else
//mk
				utf8_revised_text = utf8text;
		}

		utf8_revised_text = utf8str_trim(utf8_revised_text);

		if (!utf8_revised_text.empty())
		{
			// Chat with animation
			sendChatFromViewer(utf8_revised_text, type, TRUE);
		}
	}
	childSetValue("Chat Editor", LLString::null);

	gAgent.stopTyping();

	// If the user wants to stop chatting on hitting return, lose focus
	// and go out of chat mode.
	if (gChatBar == this && gSavedSettings.getBOOL("CloseChatOnReturn"))
	{
		stopChat();
	}
}


//-----------------------------------------------------------------------
// Static functions
//-----------------------------------------------------------------------

// static 
void LLChatBar::startChat(void* userdata)
{
	const char* line = (const char*)userdata;

	gChatBar->setVisible(TRUE);
	gChatBar->setKeyboardFocus(TRUE);
	gSavedSettings.setBOOL("ChatVisible", TRUE);

	if (gChatBar->mInputEditor)
	{
		if (line)
		{
			std::string line_string(line);
			gChatBar->mInputEditor->setText(line_string);
		}
		// always move cursor to end so users don't obliterate chat when accidentally hitting WASD
		gChatBar->mInputEditor->setCursorToEnd();
	}
}


// Exit "chat mode" and do the appropriate focus changes
// static
void LLChatBar::stopChat()
{
	// In simple UI mode, we never release focus from the chat bar
	gChatBar->setKeyboardFocus(FALSE);

	// If we typed a movement key and pressed return during the
	// same frame, the keyboard handlers will see the key as having
	// gone down this frame and try to move the avatar.
	gKeyboard->resetKeys();
	gKeyboard->resetMaskKeys();

	// stop typing animation
	gAgent.stopTyping();

	// hide chat bar so it doesn't grab focus back
	gChatBar->setVisible(FALSE);
}

void LLChatBar::setVisible(BOOL visible)
{
	gSavedSettings.setBOOL("ChatVisible", visible);
	LLPanel::setVisible(visible);
}

// static
void LLChatBar::onInputEditorKeystroke( LLLineEditor* caller, void* userdata )
{
	LLChatBar* self = (LLChatBar *)userdata;

	LLWString raw_text;
	if (self->mInputEditor) raw_text = self->mInputEditor->getWText();

	// Can't trim the end, because that will cause autocompletion
	// to eat trailing spaces that might be part of a gesture.
	LLWString::trimHead(raw_text);

	S32 length = raw_text.length();

	if( (length > 0) && (raw_text[0] != '/') )  // forward slash is used for escape (eg. emote) sequences
	{
//MK
		if (!RRenabled || !gAgent.mRRInterface.containsSubstr ("redirchat:"))
//mk
			gAgent.startTyping();
	}
	else
	{
		gAgent.stopTyping();
	}

	/* Doesn't work -- can't tell the difference between a backspace
	   that killed the selection vs. backspace at the end of line.
	if (length > 1 
		&& text[0] == '/'
		&& key == KEY_BACKSPACE)
	{
		// the selection will already be deleted, but we need to trim
		// off the character before
		LLString new_text = raw_text.substr(0, length-1);
		self->mInputEditor->setText( new_text );
		self->mInputEditor->setCursorToEnd();
		length = length - 1;
	}
	*/

	KEY key = gKeyboard->currentKey();

	// Ignore "special" keys, like backspace, arrows, etc.
	if (length > 1 
		&& raw_text[0] == '/'
		&& key < KEY_SPECIAL)
	{
		// we're starting a gesture, attempt to autocomplete

		std::string utf8_trigger = wstring_to_utf8str(raw_text);
		std::string utf8_out_str(utf8_trigger);

		if (gGestureManager.matchPrefix(utf8_trigger, &utf8_out_str))
		{
			if (self->mInputEditor)
			{
				self->mInputEditor->setText(utf8_out_str);
				S32 outlength = self->mInputEditor->getLength(); // in characters
			
				// Select to end of line, starting from the character
				// after the last one the user typed.
				self->mInputEditor->setSelection(length, outlength);
			}
		}

		//llinfos << "GESTUREDEBUG " << trigger 
		//	<< " len " << length
		//	<< " outlen " << out_str.getLength()
		//	<< llendl;
	}
}

// static
void LLChatBar::onInputEditorFocusLost( LLFocusableElement* caller, void* userdata)
{
	// stop typing animation
	gAgent.stopTyping();
}

// static
void LLChatBar::onInputEditorGainFocus( LLFocusableElement* caller, void* userdata )
{
	LLFloaterChat::setHistoryCursorAndScrollToEnd();
}

// static
void LLChatBar::onClickSay( void* userdata )
{
	LLChatBar* self = (LLChatBar*) userdata;
	self->sendChat( CHAT_TYPE_NORMAL );
}

// static
void LLChatBar::onClickShout( void* userdata )
{
	LLChatBar *self = (LLChatBar *)userdata;
	self->sendChat( CHAT_TYPE_SHOUT );
}

void LLChatBar::sendChatFromViewer(const std::string &utf8text, EChatType type, BOOL animate)
{
	sendChatFromViewer(utf8str_to_wstring(utf8text), type, animate);
}

void LLChatBar::sendChatFromViewer(const LLWString &wtext, EChatType type, BOOL animate)
{
	LLMessageSystem* msg = gMessageSystem;

	// Look for "/20 foo" channel chats.
	S32 channel = 0;
	LLWString out_text = stripChannelNumber(wtext, &channel);
	std::string utf8_out_text = wstring_to_utf8str(out_text);
	std::string utf8_text = wstring_to_utf8str(wtext);

	utf8_text = utf8str_trim(utf8_text);
	if (!utf8_text.empty())
	{
		utf8_text = utf8str_truncate(utf8_text, MAX_STRING - 1);
	}
//MK
	if (RRenabled && channel >= 2147483647 && gAgent.mRRInterface.contains ("sendchat"))
	{
		// When prevented from talking, remove the ability to talk on the DEBUG_CHANNEL altogether, since it is a way of cheating
		return;
	}
	if (RRenabled && channel == 0)
	{
		// transform the type according to chatshout, chatnormal and chatwhisper restrictions
		if (type == CHAT_TYPE_WHISPER && gAgent.mRRInterface.contains ("chatwhisper"))
		{
			type = CHAT_TYPE_NORMAL;
		}
		if (type == CHAT_TYPE_SHOUT && gAgent.mRRInterface.contains ("chatshout"))
		{
			type = CHAT_TYPE_NORMAL;
		}
		if ((type == CHAT_TYPE_SHOUT || type == CHAT_TYPE_NORMAL)
			&& gAgent.mRRInterface.contains ("chatnormal"))
		{
			type = CHAT_TYPE_WHISPER;
		}
		if (gAgent.mRRInterface.containsSubstr ("redirchat:"))
 		{
			animate = false;
		}
	}
//mk
	// Don't animate for chats people can't hear (chat to scripts)
	if (animate && (channel == 0))
	{
		if (type == CHAT_TYPE_WHISPER)
		{
			lldebugs << "You whisper " << utf8_text << llendl;
			gAgent.sendAnimationRequest(ANIM_AGENT_WHISPER, ANIM_REQUEST_START);
		}
		else if (type == CHAT_TYPE_NORMAL)
		{
			lldebugs << "You say " << utf8_text << llendl;
			gAgent.sendAnimationRequest(ANIM_AGENT_TALK, ANIM_REQUEST_START);
		}
		else if (type == CHAT_TYPE_SHOUT)
		{
			lldebugs << "You shout " << utf8_text << llendl;
			gAgent.sendAnimationRequest(ANIM_AGENT_SHOUT, ANIM_REQUEST_START);
		}
		else
		{
			llinfos << "send_chat_from_viewer() - invalid volume" << llendl;
			return;
		}
	}
	else
	{
		if (type != CHAT_TYPE_START && type != CHAT_TYPE_STOP)
		{
			lldebugs << "Channel chat: " << utf8_text << llendl;
		}
	}

//MK
	if (RRenabled && channel == 0)
	{
		std::string restriction;

		// We might want to redirect this chat or emote (and exit this function early on)
		if (utf8_out_text.find ("/me ") == 0 // emote
			|| utf8_out_text.find ("/me's") == 0) // emote
		{
			if (gAgent.mRRInterface.containsSubstr ("rediremote:"))
			{
				restriction = "rediremote:";
			}
		}
		else if (utf8_out_text.find ("((") != 0 || utf8_out_text.find ("))") != utf8_out_text.length () - 2) // not OOC text
		{
			if (gAgent.mRRInterface.containsSubstr ("redirchat:"))
			{
				restriction = "redirchat:";
			}
		}

		if (!restriction.empty())
		{
			// Public chat or emote redirected => for each redirection, send the same message on the target channel
			RRMAP::iterator it = gAgent.mRRInterface.sSpecialObjectBehaviours.begin ();
			std::string behav;
			while (it != gAgent.mRRInterface.sSpecialObjectBehaviours.end())
			{
				behav = it->second;
				if (behav.find (restriction) == 0)
				{
					S32 ch = atoi (behav.substr (restriction.length()).c_str());
					std::ostringstream stream;
					stream << "sendchannel:" << ch;
					if (!gAgent.mRRInterface.contains ("sendchannel") || // user not prevented from chatting on private channels
						gAgent.mRRInterface.contains (stream.str ())) // or this channel is no exception
					{
						if (ch > 0 && ch < 2147483647)
						{
							msg->newMessageFast(_PREHASH_ChatFromViewer);
							msg->nextBlockFast(_PREHASH_AgentData);
							msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
							msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
							msg->nextBlockFast(_PREHASH_ChatData);
							msg->addStringFast(_PREHASH_Message, utf8_out_text);
							msg->addU8Fast(_PREHASH_Type, type);
							msg->addS32("Channel", ch);

							gAgent.sendReliableMessage();
						}
					}
				}
				it++;
			}

			gViewerStats->incStat(LLViewerStats::ST_CHAT_COUNT);

			// We have redirected the chat message, don't send it on the original channel
			return;
		}
	}

	std::string crunchedText = utf8_out_text;

	// There is a redirection in order but this particular message is an emote or an OOC text, so we didn't
	// redirect it. However it has not gone through crunchEmote yet, so we need to do this here
	if (RRenabled && channel == 0 && gAgent.mRRInterface.containsSubstr ("redirchat:"))
	{
		crunchedText = gAgent.mRRInterface.crunchEmote(crunchedText, 20);
	}
//mk
	msg->newMessageFast(_PREHASH_ChatFromViewer);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_ChatData);
////	msg->addStringFast(_PREHASH_Message, utf8_out_text);
//MK	
	msg->addStringFast(_PREHASH_Message, crunchedText);
//mk
	msg->addU8Fast(_PREHASH_Type, type);
	msg->addS32("Channel", channel);

	gAgent.sendReliableMessage();

	gViewerStats->incStat(LLViewerStats::ST_CHAT_COUNT);
}


// static
void LLChatBar::onCommitGesture(LLUICtrl* ctrl, void* data)
{
	LLChatBar* self = (LLChatBar*)data;
	LLCtrlListInterface* gestures = self->mGestureCombo ? self->mGestureCombo->getListInterface() : NULL;
	if (gestures)
	{
		S32 index = gestures->getFirstSelectedIndex();
		if (index == 0)
		{
			return;
		}
		const std::string& trigger = gestures->getSelectedValue().asString();

//MK
		if (!RRenabled || !gAgent.mRRInterface.contains ("sendchat"))
		{
//mk
			// pretend the user chatted the trigger string, to invoke
			// substitution and logging.
			std::string text(trigger);
			std::string revised_text;
			gGestureManager.triggerAndReviseString(text, &revised_text);

			revised_text = utf8str_trim(revised_text);
			if (!revised_text.empty())
			{
				// Don't play nodding animation
				self->sendChatFromViewer(revised_text, CHAT_TYPE_NORMAL, FALSE);
			}
//MK
		}
//mk
	}
	self->mGestureLabelTimer.start();
	if (self->mGestureCombo != NULL)
	{
		// free focus back to chat bar
		self->mGestureCombo->setFocus(FALSE);
	}
}

void toggleChatHistory(void* user_data)
{
	LLFloaterChat::toggleInstance(LLSD());
}
