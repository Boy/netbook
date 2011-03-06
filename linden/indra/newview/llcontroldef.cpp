/** 
 * @file llcontroldef.cpp
 * @author James Cook
 * @brief Viewer control settings
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

// Put default viewer settings in here

#include "llviewerprecompiledheaders.h"

#include "llviewercontrol.h"

#include "indra_constants.h"

#include "v3math.h"
#include "v3dmath.h"
#include "llrect.h"
#include "v4color.h"
#include "v4coloru.h"
#include "v3color.h"
#include "llfirstuse.h"

// For Listeners
#include "audioengine.h"
#include "llagent.h"
#include "llconsole.h"
#include "lldrawpoolterrain.h"
#include "llflexibleobject.h"
#include "llfeaturemanager.h"
#include "llglslshader.h"
#include "llmediaengine.h"
#include "llpanelgeneral.h"
#include "llpanelinput.h"
#include "llsky.h"
#include "llvieweraudio.h"
#include "llviewerimagelist.h"
#include "llviewerthrottle.h"
#include "llviewerwindow.h"
#include "llvoavatar.h"
#include "llvosky.h"
#include "llvotree.h"
#include "llvovolume.h"
#include "llworld.h"
#include "pipeline.h"
#include "llviewerjoystick.h"
#include "llviewerparcelmgr.h"
#include "llparcel.h"
#include "llnotify.h"
#include "llkeyboard.h"

void declare_settings()
{
	// Somewhat under 1024 by 768, to give space for Windows task bar / Mac menu bar
	// to emphasize window isn't actually maximized.
	const S32 WINDOW_WIDTH = 1000;
	const S32 WINDOW_HEIGHT = 560;

	//------------------------------------------------------------------------
	// Color constants
	//------------------------------------------------------------------------

	const S32 TOOL_PANEL_HEIGHT = 162 + 32;
	const S32 TOOL_PANEL_WIDTH = 75 + 8;

	// Colors that can be changed in the UI
	gSavedSettings.declareColor4("EffectColor", LLColor4(1.f, 1.f, 1.f, 1.f), "Particle effects color");
	gSavedSettings.declareColor4("SystemChatColor",	LLColor4(0.8f, 1.f, 1.f, 1.f), "Color of chat messages from SL System");
	gSavedSettings.declareColor4("AgentChatColor",	LLColor4(1.0f, 1.0f, 1.0f, 1.0f), "Color of chat messages from other residents");
	gSavedSettings.declareColor4("ObjectChatColor",	LLColor4(0.7f, 0.9f, 0.7f, 1.0f), "Color of chat messages from objects");
	gSavedSettings.declareColor4("llOwnerSayChatColor",	LLColor4(0.99f, 0.99f, 0.69f, 1.0f), "Color of chat messages from objects only visible to the owner");
	gSavedSettings.declareColor4("BackgroundChatColor",	LLColor4(0.f, 0.f, 0.f, 1.0f), "Color of chat bubble background");
	gSavedSettings.declareColor4("ScriptErrorColor",	LLColor4(0.82f, 0.82f, 0.99f, 1.0f), "Color of script error messages");
	gSavedSettings.declareColor4("HTMLLinkColor",	LLColor4(0.6f, 0.6f, 1.0f, 1.0f), "Color of hyperlinks");
	
	// color palette in color picker
	gSavedSettings.declareColor4("ColorPaletteEntry01", LLColor4 ( 0.0f, 0.0f, 0.0f, 1.0f ), "Color picker palette entry");
	gSavedSettings.declareColor4("ColorPaletteEntry02", LLColor4 ( 0.5f, 0.5f, 0.5f, 1.0f ), "Color picker palette entry");
	gSavedSettings.declareColor4("ColorPaletteEntry03", LLColor4 ( 0.5f, 0.0f, 0.0f, 1.0f ), "Color picker palette entry");
	gSavedSettings.declareColor4("ColorPaletteEntry04", LLColor4 ( 0.5f, 0.5f, 0.0f, 1.0f ), "Color picker palette entry");
	gSavedSettings.declareColor4("ColorPaletteEntry05", LLColor4 ( 0.0f, 0.5f, 0.0f, 1.0f ), "Color picker palette entry");
	gSavedSettings.declareColor4("ColorPaletteEntry06", LLColor4 ( 0.0f, 0.5f, 0.5f, 1.0f ), "Color picker palette entry");
	gSavedSettings.declareColor4("ColorPaletteEntry07", LLColor4 ( 0.0f, 0.0f, 0.5f, 1.0f ), "Color picker palette entry");
	gSavedSettings.declareColor4("ColorPaletteEntry08", LLColor4 ( 0.5f, 0.0f, 0.5f, 1.0f ), "Color picker palette entry");
	gSavedSettings.declareColor4("ColorPaletteEntry09", LLColor4 ( 0.5f, 0.5f, 0.0f, 1.0f ), "Color picker palette entry");
	gSavedSettings.declareColor4("ColorPaletteEntry10", LLColor4 ( 0.0f, 0.25f, 0.25f, 1.0f ), "Color picker palette entry");
	gSavedSettings.declareColor4("ColorPaletteEntry11", LLColor4 ( 0.0f, 0.5f, 1.0f, 1.0f ), "Color picker palette entry");
	gSavedSettings.declareColor4("ColorPaletteEntry12", LLColor4 ( 0.0f, 0.25f, 0.5f, 1.0f ), "Color picker palette entry");
	gSavedSettings.declareColor4("ColorPaletteEntry13", LLColor4 ( 0.5f, 0.0f, 1.0f, 1.0f ), "Color picker palette entry");
	gSavedSettings.declareColor4("ColorPaletteEntry14", LLColor4 ( 0.5f, 0.25f, 0.0f, 1.0f ), "Color picker palette entry");
	gSavedSettings.declareColor4("ColorPaletteEntry15", LLColor4 ( 1.0f, 1.0f, 1.0f, 1.0f ), "Color picker palette entry");
	gSavedSettings.declareColor4("ColorPaletteEntry16", LLColor4 ( 1.0f, 1.0f, 1.0f, 1.0f ), "Color picker palette entry");

	gSavedSettings.declareColor4("ColorPaletteEntry17", LLColor4 ( 1.0f, 1.0f, 1.0f, 1.0f ), "Color picker palette entry");
	gSavedSettings.declareColor4("ColorPaletteEntry18", LLColor4 ( 0.75f, 0.75f, 0.75f, 1.0f ), "Color picker palette entry");
	gSavedSettings.declareColor4("ColorPaletteEntry19", LLColor4 ( 1.0f, 0.0f, 0.0f, 1.0f ), "Color picker palette entry");
	gSavedSettings.declareColor4("ColorPaletteEntry20", LLColor4 ( 1.0f, 1.0f, 0.0f, 1.0f ), "Color picker palette entry");
	gSavedSettings.declareColor4("ColorPaletteEntry21", LLColor4 ( 0.0f, 1.0f, 0.0f, 1.0f ), "Color picker palette entry");
	gSavedSettings.declareColor4("ColorPaletteEntry22", LLColor4 ( 0.0f, 1.0f, 1.0f, 1.0f ), "Color picker palette entry");
	gSavedSettings.declareColor4("ColorPaletteEntry23", LLColor4 ( 0.0f, 0.0f, 1.0f, 1.0f ), "Color picker palette entry");
	gSavedSettings.declareColor4("ColorPaletteEntry24", LLColor4 ( 1.0f, 0.0f, 1.0f, 1.0f ), "Color picker palette entry");
	gSavedSettings.declareColor4("ColorPaletteEntry25", LLColor4 ( 1.0f, 1.0f, 0.5f, 1.0f ), "Color picker palette entry");
	gSavedSettings.declareColor4("ColorPaletteEntry26", LLColor4 ( 0.0f, 1.0f, 0.5f, 1.0f ), "Color picker palette entry");
	gSavedSettings.declareColor4("ColorPaletteEntry27", LLColor4 ( 0.5f, 1.0f, 1.0f, 1.0f ), "Color picker palette entry");
	gSavedSettings.declareColor4("ColorPaletteEntry28", LLColor4 ( 0.5f, 0.5f, 1.0f, 1.0f ), "Color picker palette entry");
	gSavedSettings.declareColor4("ColorPaletteEntry29", LLColor4 ( 1.0f, 0.0f, 0.5f, 1.0f ), "Color picker palette entry");
	gSavedSettings.declareColor4("ColorPaletteEntry30", LLColor4 ( 1.0f, 0.5f, 0.0f, 1.0f ), "Color picker palette entry");
	gSavedSettings.declareColor4("ColorPaletteEntry31", LLColor4 ( 1.0f, 1.0f, 1.0f, 1.0f ), "Color picker palette entry");
	gSavedSettings.declareColor4("ColorPaletteEntry32", LLColor4 ( 1.0f, 1.0f, 1.0f, 1.0f ), "Color picker palette entry");

	//------------------------------------------------------------------------
	// Main menu
	//------------------------------------------------------------------------
	gSavedSettings.declareS32("MenuBarHeight", 18, "", NO_PERSIST );
	gSavedSettings.declareS32("MenuBarWidth", 410, "", NO_PERSIST );

	gSavedSettings.declareF32("MenuAccessKeyTime", 0.25f, "Time (seconds) in which the menu key must be tapped to move focus to the menu bar");
	gSavedSettings.declareBOOL("UseAltKeyForMenus", TRUE, "Access menus via keyboard by tapping Alt");
	// Which background overlay to use
	gSavedSettings.declareS32("MapOverlayIndex", 0, "Currently selected world map type");


	//------------------------------------------------------------------------
	// Pie Menus
	//------------------------------------------------------------------------
	gSavedSettings.declareF32("PieMenuLineWidth", 2.5f, "Width of lines in pie menu display (pixels)");

	//------------------------------------------------------------------------
	// Drop Shadows
	//------------------------------------------------------------------------
	gSavedSettings.declareS32("DropShadowButton",	2, "Drop shadow width for buttons (pixels)");
	gSavedSettings.declareS32("DropShadowFloater",	5, "Drop shadow width for floaters (pixels)");
	gSavedSettings.declareS32("DropShadowSlider",	3, "Drop shadow width for sliders (pixels)");
	gSavedSettings.declareS32("DropShadowTooltip",	4, "Drop shadow width for tooltips (pixels)");

	//------------------------------------------------------------------------
	// Buttons
	//------------------------------------------------------------------------
	gSavedSettings.declareS32("ButtonHPad", 10, "Default horizontal spacing between buttons (pixels)");		// space from left of button to text
	gSavedSettings.declareS32("ButtonVPad", 1, "Default vertical spacing between buttons (pixels)");		// space from bottom of button to text
	gSavedSettings.declareS32("ButtonHeightSmall", 16, "Default height for small buttons (pixels)");
	gSavedSettings.declareS32("ButtonHeight", 20, "Default height for normal buttons (pixels)");
	gSavedSettings.declareF32("ButtonFlashRate", 2.f, "Frequency at which buttons flash (hz)");
	gSavedSettings.declareS32("ButtonFlashCount", 3, "Number of flashes after which flashing buttons stay lit up");
	//gSavedSettings.declareS32("ButtonHeightToolbar", 32, "[NOT USED]");

	//gSavedSettings.declareS32("DefaultButtonWidth", DEFAULT_BUTTON_WIDTH, "[NOT USED]");
	//gSavedSettings.declareS32("DefaultButtonHeight", DEFAULT_BUTTON_HEIGHT, "[NOT USED]");

	//------------------------------------------------------------------------
	// Scroll Lists
	//------------------------------------------------------------------------
	gSavedSettings.declareF32("TypeAheadTimeout", 1.5f, "Time delay before clearing type-ahead buffer in lists (seconds)");

	//------------------------------------------------------------------------
	// ToolTips
	//------------------------------------------------------------------------
	gSavedSettings.declareF32("ToolTipDelay", 0.7f, "Seconds before displaying tooltip when mouse stops over UI element");
	gSavedSettings.declareF32("DragAndDropToolTipDelay", 0.1f, "Seconds before displaying tooltip when performing drag and drop operation");

	//------------------------------------------------------------------------
	// Auto-Open Folders
	//------------------------------------------------------------------------
	gSavedSettings.declareF32("FolderAutoOpenDelay", 0.75f, "Seconds before automatically expanding the folder under the mouse when performing inventory drag and drop");
	gSavedSettings.declareF32("InventoryAutoOpenDelay", 1.f, "Seconds before automatically opening inventory when mouse is over inventory button when performing inventory drag and drop");
	gSavedSettings.declareBOOL("ShowEmptyFoldersWhenSearching", FALSE, "Shows folders that do not have any visible contents when applying a filter to inventory");
	gSavedSettings.declareS32("FilterItemsPerFrame", 500, "Maximum number of inventory items to match against search filter every frame (lower to increase framerate while searching, higher to improve search speed)");
	gSavedSettings.declareBOOL("DebugInventoryFilters", FALSE, "Turn on debugging display for inventory filtering");
	gSavedSettings.declareF32("FolderLoadingMessageWaitTime", 0.5f, "Seconds to wait before showing the LOADING... text in folder views");

	//------------------------------------------------------------------------
	// Status bar
	//------------------------------------------------------------------------
	gSavedSettings.declareS32("StatusBarHeight", 26, "Height of menu/status bar at top of screen (pixels)" );
	//gSavedSettings.declareS32("StatusBarButtonWidth", 80, "[NOT USED]");

	gSavedSettings.declareS32("StatusBarPad", 10, "Spacing between popup buttons at bottom of screen (Stand up, Release Controls)");

	//gSavedSettings.declareS32("ChatWidth", 250, "[NOT USED]");

	//------------------------------------------------------------------------
	// Toolbar bar
	//------------------------------------------------------------------------
	//gSavedSettings.declareS32("ToolBarHeight", 20, "[NOT USED]" );
	//gSavedSettings.declareS32("ToolBarWidth", 480, "[NOT USED]" );
	//gSavedSettings.declareS32("ToolBarButtonWidth", 80, "[NOT USED]" );

	//------------------------------------------------------------------------
	// Fonts
	//------------------------------------------------------------------------
	gSavedSettings.declareF32("FontScreenDPI",	83.f, "Font resolution, higher is bigger (pixels per inch)");		// windows standard
	gSavedSettings.declareF32("FontSizeMonospace", 9.f, "Size of monospaced font (points, or 1/72 of an inch)");
	gSavedSettings.declareF32("FontSizeSmall",	9.f, "Size of small font (points, or 1/72 of an inch)");
	gSavedSettings.declareF32("FontSizeMedium",	10.f, "Size of medium font (points, or 1/72 of an inch)");
	gSavedSettings.declareF32("FontSizeLarge",	12.f, "Size of large font (points, or 1/72 of an inch)");
	gSavedSettings.declareF32("FontSizeHuge",	16.f, "Size of huge font (points, or 1/72 of an inch)");

	gSavedSettings.declareString("FontMonospace",		"profontwindows.ttf", "Name of monospace font (Truetype file name)");
	gSavedSettings.declareString("FontSansSerif",		"MtBkLfRg.ttf", "Name of san-serif font (Truetype file name)");
#if LL_WINDOWS
	// Lists Japanese, Korean, and Chinese sanserif fonts available in
	// Windows XP and Vista, as well as "Arial Unicode MS".
	gSavedSettings.declareString(
		"FontSansSerifFallback",
		"MSGOTHIC.TTC;gulim.ttc;simhei.ttf;ArialUni.ttf",
		"Name of fallback san-serif font (Truetype file name)");
#elif LL_DARWIN
	// This is a fairly complete Japanese font that ships with Mac OS X.
	// The first filename is in UTF8, but it shows up in the font menu as "Hiragino Kaku Gothic Pro W3".
	// The third filename is in UTF8, but it shows up in the font menu as "STHeiti Light"
	gSavedSettings.declareString("FontSansSerifFallback",	"\xE3\x83\x92\xE3\x83\xA9\xE3\x82\xAD\xE3\x82\x99\xE3\x83\x8E\xE8\xA7\x92\xE3\x82\xB3\xE3\x82\x99 Pro W3.otf;\xE3\x83\x92\xE3\x83\xA9\xE3\x82\xAD\xE3\x82\x99\xE3\x83\x8E\xE8\xA7\x92\xE3\x82\xB3\xE3\x82\x99 ProN W3.otf;AppleGothic.dfont;AppleGothic.ttf;\xe5\x8d\x8e\xe6\x96\x87\xe7\xbb\x86\xe9\xbb\x91.ttf", "Name of san-serif font (Truetype file name)");
#else
	// 'unicode.ttf' doesn't exist, but hopefully an international
	// user can take the hint and drop in their favourite local font.
	gSavedSettings.declareString("FontSansSerifFallback",	"unicode.ttf", "Name of fallback san-serif font (Truetype file name)");
#endif
	gSavedSettings.declareF32("FontSansSerifFallbackScale", 1.0, "Scale of fallback font relative to huge font (fraction of huge font size)");
	gSavedSettings.declareString("FontSansSerifBold",	"MtBdLfRg.ttf", "Name of bold font (Truetype file name)");

	//------------------------------------------------------------------------
	// Chat
	//------------------------------------------------------------------------

	// 0 = small, 1 = big
	gSavedSettings.declareS32("ChatFontSize", 0, "Size of chat text in chat console (0 = small, 1 = big)");

	// Does the console occupy full window width or only left 2/3?
	gSavedSettings.declareBOOL("ChatFullWidth", TRUE, "Chat console takes up full width of SL window");

	// opacity of console background
	gSavedSettings.declareF32("ConsoleBackgroundOpacity", 0.4f, "Opacity of chat console (0.0 = completely transparent, 1.0 = completely opaque)");
	gSavedSettings.declareS32("ConsoleMaxLines", 40, "Max number of lines of chat text visible in console.");

	// Seconds to keep line of text on console
	gSavedSettings.declareF32("ChatPersistTime", 15.f, "Time for which chat stays visible in console (seconds)");
	gSavedSettings.declareBOOL("PlayTypingAnim", FALSE, "Your avatar plays the typing animation whenever you type in the chat bar");

	// show chat in bubbles above avatar heads
	gSavedSettings.declareBOOL("UseChatBubbles", FALSE, "Show chat above avatars head in chat bubbles");
	gSavedSettings.declareF32("ChatBubbleOpacity", 0.5f, "Opacity of chat bubble background (0.0 = completely transparent, 1.0 = completely opaque)");

	gSavedSettings.declareBOOL("AllowMUpose", TRUE, "Allow MU* pose style in chat and IM (with ':' as a synonymous to '/me ')");
	gSavedSettings.declareBOOL("AutoCloseOOC", TRUE, "Auto-close OOC chat (i.e. add \"))\" if not found and \"((\" was used)");

	gSavedSettings.declareBOOL("AllowIdleAFK", TRUE, "Automatically set AFK (away from keyboard) mode when idle");
	gSavedSettings.declareF32("AFKTimeout", 900.f, "Time before automatically setting AFK (away from keyboard) mode (seconds)");	// 15 minutes

	gSavedSettings.declareBOOL("SmallAvatarNames", TRUE, "Display avatar name text in smaller font");
	gSavedSettings.declareBOOL("ScriptErrorsAsChat", FALSE, "Display script errors and warning in chat history");

	gSavedSettings.declareBOOL("ChatShowTimestamps", TRUE, "Show timestamps in chat");

	gSavedSettings.declareBOOL("EnableVoiceChat", FALSE, "Enable talking to other residents with a microphone");
	gSavedSettings.declareBOOL("VoiceCallsFriendsOnly", FALSE, "Only accept voice calls from residents on your friends list");
	gSavedSettings.declareBOOL("PTTCurrentlyEnabled", TRUE, "", NO_PERSIST);
	gSavedSettings.declareBOOL("ShowVoiceChannelPopup", FALSE, "Controls visibility of the current voice channel popup above the voice tab");
	gSavedSettings.declareBOOL("EnablePushToTalk", TRUE, "Must hold down a key or moouse button when talking into your microphone");
	gSavedSettings.declareString("PushToTalkButton", "MiddleMouse", "Which button or keyboard key is used for push-to-talk");
	gSavedSettings.declareBOOL("PushToTalkToggle", FALSE, "Should the push-to-talk button behave as a toggle");
	gSavedSettings.declareS32("VoiceEarLocation", 0, "Location of the virtual ear for voice");
	gSavedSettings.declareString("VivoxDebugServerName", "bhd.vivox.com", "Hostname of the vivox account server to use for voice when not connected to Agni.");
	gSavedSettings.declareColor4("SpeakingColor", LLColor4(0.f, 1.f, 0.f, 1.f), "Color of various indicators when resident is speaking on a voice channel.");
	gSavedSettings.declareColor4("OverdrivenColor", LLColor4(1.f, 0.f, 0.f, 1.f), "Color of various indicators when resident is speaking too loud.");
	gSavedSettings.declareString("VoiceInputAudioDevice", "Default", "Audio input device to use for voice");
	gSavedSettings.declareString("VoiceOutputAudioDevice", "Default", "Audio output device to use for voice");
	gSavedSettings.declareString("VivoxDebugLevel", "-1", "Logging level to use when launching the vivox daemon");

	//voice amplitude images;
	
	/*
	gSavedSettings.declareString("VoiceImageLevel0", "5b41b4c3-eb70-0f0f-17d9-1765cbd07d39", "Texture UUID for voice image level 0");	
	gSavedSettings.declareString("VoiceImageLevel1", "b06ffd0a-7bfe-0449-0bbc-df291f1857c4", "Texture UUID for voice image level 1");
	gSavedSettings.declareString("VoiceImageLevel2", "bfa16494-b731-5b59-3260-9e4fd29d63f7", "Texture UUID for voice image level 2");
	gSavedSettings.declareString("VoiceImageLevel3", "6951074f-de1d-3c55-cb2f-e972877f7f81", "Texture UUID for voice image level 3");
	gSavedSettings.declareString("VoiceImageLevel4", "ce3df80a-f0c5-a7cb-f5bc-d3bb38949d0d", "Texture UUID for voice image level 4");
	gSavedSettings.declareString("VoiceImageLevel5", "8d0e359c-5cea-bdf5-b6b0-32d2fea6355c", "Texture UUID for voice image level 5");
	gSavedSettings.declareString("VoiceImageLevel6", "ad9e64e0-a763-5d8c-f2e8-7b5dfdb7f20f", "Texture UUID for voice image level 6");
	*/
	
	/*
	// Jeffrey's first version
	gSavedSettings.declareString("VoiceImageLevel0", "5b41b4c3-eb70-0f0f-17d9-1765cbd07d39", "Texture UUID for voice image level 0");	
	gSavedSettings.declareString("VoiceImageLevel1", "cde76ae8-0044-d575-8e2c-65fb0a14cbde", "Texture UUID for voice image level 1");
	gSavedSettings.declareString("VoiceImageLevel2", "cde76ae8-0044-d575-8e2c-65fb0a14cbde", "Texture UUID for voice image level 2");
	gSavedSettings.declareString("VoiceImageLevel3", "cde76ae8-0044-d575-8e2c-65fb0a14cbde", "Texture UUID for voice image level 3");
	gSavedSettings.declareString("VoiceImageLevel4", "cde76ae8-0044-d575-8e2c-65fb0a14cbde", "Texture UUID for voice image level 4");
	gSavedSettings.declareString("VoiceImageLevel5", "cde76ae8-0044-d575-8e2c-65fb0a14cbde", "Texture UUID for voice image level 5");
	gSavedSettings.declareString("VoiceImageLevel6", "cde76ae8-0044-d575-8e2c-65fb0a14cbde", "Texture UUID for voice image level 6");
	*/
	
	// Brent's first version
	/*
	gSavedSettings.declareString("VoiceImageLevel0", "5b41b4c3-eb70-0f0f-17d9-1765cbd07d39", "Texture UUID for voice image level 0");	
	gSavedSettings.declareString("VoiceImageLevel1", "72365124-c7a7-a1f9-3d7a-d8e521eb5011", "Texture UUID for voice image level 1");
	gSavedSettings.declareString("VoiceImageLevel2", "72365124-c7a7-a1f9-3d7a-d8e521eb5011", "Texture UUID for voice image level 2");
	gSavedSettings.declareString("VoiceImageLevel3", "72365124-c7a7-a1f9-3d7a-d8e521eb5011", "Texture UUID for voice image level 3");
	gSavedSettings.declareString("VoiceImageLevel4", "72365124-c7a7-a1f9-3d7a-d8e521eb5011", "Texture UUID for voice image level 4");
	gSavedSettings.declareString("VoiceImageLevel5", "72365124-c7a7-a1f9-3d7a-d8e521eb5011", "Texture UUID for voice image level 5");
	gSavedSettings.declareString("VoiceImageLevel6", "72365124-c7a7-a1f9-3d7a-d8e521eb5011", "Texture UUID for voice image level 6");
	*/
	
	/*
	// Brent's second version
	gSavedSettings.declareString("VoiceImageLevel0", "5b41b4c3-eb70-0f0f-17d9-1765cbd07d39", "Texture UUID for voice image level 0");	
	gSavedSettings.declareString("VoiceImageLevel1", "4ee6a7ac-472e-b5ff-7125-f6213798cbee", "Texture UUID for voice image level 1");
	gSavedSettings.declareString("VoiceImageLevel2", "4ee6a7ac-472e-b5ff-7125-f6213798cbee", "Texture UUID for voice image level 2");
	gSavedSettings.declareString("VoiceImageLevel3", "4ee6a7ac-472e-b5ff-7125-f6213798cbee", "Texture UUID for voice image level 3");
	gSavedSettings.declareString("VoiceImageLevel4", "4ee6a7ac-472e-b5ff-7125-f6213798cbee", "Texture UUID for voice image level 4");
	gSavedSettings.declareString("VoiceImageLevel5", "4ee6a7ac-472e-b5ff-7125-f6213798cbee", "Texture UUID for voice image level 5");
	gSavedSettings.declareString("VoiceImageLevel6", "4ee6a7ac-472e-b5ff-7125-f6213798cbee", "Texture UUID for voice image level 6");
	*/
	
	// Jeffrey's tweak on 4/9/07
	gSavedSettings.declareString("VoiceImageLevel0", "041ee5a0-cb6a-9ac5-6e49-41e9320507d5", "Texture UUID for voice image level 0");	
	gSavedSettings.declareString("VoiceImageLevel1", "29de489d-0491-fb00-7dab-f9e686d31e83", "Texture UUID for voice image level 1");
	gSavedSettings.declareString("VoiceImageLevel2", "29de489d-0491-fb00-7dab-f9e686d31e83", "Texture UUID for voice image level 2");
	gSavedSettings.declareString("VoiceImageLevel3", "29de489d-0491-fb00-7dab-f9e686d31e83", "Texture UUID for voice image level 3");
	gSavedSettings.declareString("VoiceImageLevel4", "29de489d-0491-fb00-7dab-f9e686d31e83", "Texture UUID for voice image level 4");
	gSavedSettings.declareString("VoiceImageLevel5", "29de489d-0491-fb00-7dab-f9e686d31e83", "Texture UUID for voice image level 5");
	gSavedSettings.declareString("VoiceImageLevel6", "29de489d-0491-fb00-7dab-f9e686d31e83", "Texture UUID for voice image level 6");
	
	gSavedSettings.declareString("VoiceHost", "127.0.0.1", "Client SLVoice host to connect to");
	gSavedSettings.declareU32("VoicePort", 44124, "Client SLVoice port to connect to");
	
	//------------------------------------------------------------------------
	// Caution Script Permission Prompts
	//------------------------------------------------------------------------

	gSavedSettings.declareBOOL("PermissionsCautionEnabled", TRUE, "When enabled, changes the handling of script permission requests to help avoid accidental granting of certain permissions, such as the debit permission", NO_PERSIST);
	gSavedSettings.declareS32("PermissionsCautionNotifyBoxHeight", 344, "Height of caution-style notification messages", NO_PERSIST);

	//------------------------------------------------------------------------
	// Other....
	//------------------------------------------------------------------------

	gSavedSettings.declareString("LongDateFormat", "%A %d %B %Y", "Long date format to use" );
	gSavedSettings.declareString("ShortDateFormat", "%Y-%m-%d", "Short date format to use" );
	gSavedSettings.declareString("TimeFormat", "%H:%M", "Time format to use" );
	gSavedSettings.declareString("TimestampFormat", "%a %d %b %Y %H:%M:%S", "Timestamp format to use" );

	gSavedSettings.declareBOOL("RestrainedLife", FALSE, "Toggles the RestrainedLife features (BDSM lockable toys support). Needs a restart of the viewer.");
	gSavedSettings.declareBOOL("RestrainedLifeDebug", FALSE, "Toggles the RestrainedLife debug mode (displays the commands when in debug mode).");
	gSavedSettings.declareBOOL("RestrainedLifeNoSetEnv", FALSE, "When TRUE, forbids to set the environment (time of day) via RestrainedLife. Needs a restart of the viewer.");
	gSavedSettings.declareBOOL("RestrainedLifeForbidGiveToRLV", FALSE, "When TRUE, forbids to give sub-folders to the #RLV RestrainedLife folder.");

	gSavedSettings.declareBOOL("ScriptHelpFollowsCursor", FALSE, "Scripting help window updates contents based on script editor contents under text cursor");
	gSavedSettings.declareBOOL("SaveScriptsAsMono", TRUE, "When set to TRUE, save scripts in inventory as Mono scripts instead of LSL2");

	gSavedSettings.declareS32("LastFeatureVersion", 0, "[DO NOT MODIFY] Version number for tracking hardware changes", TRUE);
	gSavedSettings.declareS32("NumSessions", 0, "Number of successful logins to Second Life");
	gSavedSettings.declareBOOL("ShowInventory", FALSE, "Open inventory window on login");
	gSavedSettings.declareBOOL("ChatOnlineNotification", TRUE, "Provide notifications for when friend log on and off of SL");

	gSavedSettings.declareString("DefaultObjectTexture", "89556747-24cb-43ed-920b-47caed15465f", "Texture used as 'Default' in texture picker. (UUID texture reference)" );  // maple texture

	gSavedSettings.declareBOOL("ShowPropertyLines", FALSE, "Show line overlay demarking property boundaries");
	gSavedSettings.declareBOOL("ShowParcelOwners", FALSE, "", NO_PERSIST);
	gSavedSettings.declareBOOL("ToolboxAutoMove", FALSE, "[NOT USED]");
	gSavedSettings.declareBOOL("ToolboxShowMore", TRUE, "Whether to show additional build tool controls", TRUE);

	gSavedSettings.declareRect("ToolboxRect", LLRect(0, 100, 100, 100), "Rectangle for tools window" ); // only care about position

	// User interface button states
	gSavedSettings.declareBOOL("FirstPersonBtnState",	FALSE,	"", NO_PERSIST);
	gSavedSettings.declareBOOL("MouselookBtnState",		FALSE,	"", NO_PERSIST);
	gSavedSettings.declareBOOL("ThirdPersonBtnState",	TRUE,	"", NO_PERSIST);
	gSavedSettings.declareBOOL("BuildBtnState",			FALSE,	"", NO_PERSIST);

	gSavedSettings.declareBOOL("HideReleaseKeys",  TRUE, "Always hide the Release Keys button");
	gSavedSettings.declareBOOL("HideMasterRemote", TRUE, "Hide the Master Remote control when useless");
	gSavedSettings.declareBOOL("ShowGroupsButton", TRUE, "Show the Groups button in the toolbar (requires a restart)");

	//gSavedSettings.declareBOOL("TalkBtnState",		FALSE, "[NOT USED]");

	gSavedSettings.declareBOOL("ShowPermissions",	FALSE, "", NO_PERSIST);
	gSavedSettings.declareBOOL("ShowTools",			FALSE, "", NO_PERSIST);


	gSavedSettings.declareString("NextLoginLocation", "", "Location to log into by default.");	// if present in settings.ini, will force you to that sim/x/y/z on next login 

//	gSavedSettings.declareBOOL("ShowBasicHelpOnLaunch",	TRUE);
	gSavedSettings.declareRect("BasicHelpRect",			LLRect(0, 404, 467, 0), "Rectangle for help window" );  // Only width and height are used

	gSavedSettings.declareS32("LastPrefTab", 0, "Last selected tab in preferences window");

	gSavedSettings.declareString("LSLHelpURL", "https://wiki.secondlife.com/wiki/[LSL_STRING]", "URL that points to LSL help files, with [LSL_STRING] corresponding to the referenced LSL function or keyword");
	// link for editable wiki (https doesn't seem to work right now with our embedded browser)
	//gSavedSettings.declareString("LSLHelpURL", "https://wiki.secondlife.com/wiki/[LSL_STRING]", "URL that points to LSL help files, with [LSL_STRING] corresponding to the referenced LSL function or keyword");
	// Wearable default images
//	const char* UI_IMG_BLACK_UUID		= "e2244626-f22f-4839-8123-1e7baddeb659";
	const char* UI_IMG_WHITE_UUID		= "5748decc-f629-461c-9a36-a35a221fe21f";
//	const char* UI_IMG_DARKGRAY_UUID	= "267e26d3-e0e1-41b8-91b1-3b337102928d";
//	const char* UI_IMG_LIGHTGRAY_UUID	= "c520bf46-cc5d-412b-a60b-9f1bd245189f";

	gSavedSettings.declareString("UIImgDefaultShirtUUID",		UI_IMG_WHITE_UUID,					"", NO_PERSIST);
	gSavedSettings.declareString("UIImgDefaultPantsUUID",		UI_IMG_WHITE_UUID,					"", NO_PERSIST);
	gSavedSettings.declareString("UIImgDefaultEyesUUID",		"6522e74d-1660-4e7f-b601-6f48c1659a77", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgDefaultHairUUID",		"7ca39b4c-bd19-4699-aff7-f93fd03d3e7b", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgDefaultShoesUUID",		UI_IMG_WHITE_UUID,						"", NO_PERSIST);
	gSavedSettings.declareString("UIImgDefaultSocksUUID",		UI_IMG_WHITE_UUID,					"", NO_PERSIST);
	gSavedSettings.declareString("UIImgDefaultGlovesUUID",		UI_IMG_WHITE_UUID,					"", NO_PERSIST);
	gSavedSettings.declareString("UIImgDefaultJacketUUID",		UI_IMG_WHITE_UUID,					"", NO_PERSIST);
	gSavedSettings.declareString("UIImgDefaultUnderwearUUID",	UI_IMG_WHITE_UUID,					"", NO_PERSIST);
	gSavedSettings.declareString("UIImgDefaultSkirtUUID",		UI_IMG_WHITE_UUID,					"", NO_PERSIST);

	// Utility color for texture defaults
	gSavedSettings.declareString("UIImgWhiteUUID",				UI_IMG_WHITE_UUID,						"", NO_PERSIST);

	// Movement widget controls
	const S32 MOVE_BTN_COL1 = 20;
	const S32 MOVE_BTN_COL2 = MOVE_BTN_COL1 + 25;
	const S32 MOVE_BTN_COL3 = MOVE_BTN_COL2 + 21;
	const S32 MOVE_BTN_COL4 = MOVE_BTN_COL3 + 25;
	const S32 MOVE_BTN_COL5 = MOVE_BTN_COL4 + 25;
//	const S32 MOVE_BTN_COL6 = MOVE_BTN_COL5 + 20;
	const S32 MOVE_BTN_ROW1 = 4;
	const S32 MOVE_BTN_ROW2 = MOVE_BTN_ROW1 + 25;
	const S32 MOVE_BTN_ROW3 = MOVE_BTN_ROW2 + 25;
	const S32 MOVE_BTN_FLY_LEFT = MOVE_BTN_COL1+96;
	const S32 MOVE_BTN_FLY_WIDTH = 40;
	const S32 MOVE_BTN_FLY_RIGHT = MOVE_BTN_FLY_LEFT + MOVE_BTN_FLY_WIDTH;

	//gSavedSettings.declareBOOL("CreateObjectsCentered",	FALSE, "[NOT USED]");

	//gSavedSettings.declareBOOL("ShowMoveArrows", TRUE, "[NOT USED]");

	gSavedSettings.declareRect("SlideLeftBtnRect",
		LLRect(MOVE_BTN_COL1, MOVE_BTN_ROW3,	MOVE_BTN_COL2,	MOVE_BTN_ROW2),
		"", NO_PERSIST	);
	gSavedSettings.declareRect("TurnLeftBtnRect",
		LLRect(MOVE_BTN_COL1, MOVE_BTN_ROW2,	MOVE_BTN_COL2,	MOVE_BTN_ROW1),
		"", NO_PERSIST  );
	gSavedSettings.declareRect("ForwardBtnRect",
		LLRect(MOVE_BTN_COL2, MOVE_BTN_ROW3,	MOVE_BTN_COL3,	MOVE_BTN_ROW2),
		"", NO_PERSIST	);
	gSavedSettings.declareRect("BackwardBtnRect",
		LLRect(MOVE_BTN_COL2, MOVE_BTN_ROW2,	MOVE_BTN_COL3,	MOVE_BTN_ROW1),
		"", NO_PERSIST	);
	gSavedSettings.declareRect("SlideRightBtnRect",
		LLRect(MOVE_BTN_COL3, MOVE_BTN_ROW3,	MOVE_BTN_COL4,	MOVE_BTN_ROW2),
		"", NO_PERSIST  );
	gSavedSettings.declareRect("TurnRightBtnRect",
		LLRect(MOVE_BTN_COL3, MOVE_BTN_ROW2,	MOVE_BTN_COL4,	MOVE_BTN_ROW1),
		"", NO_PERSIST	);
	gSavedSettings.declareRect("MoveUpBtnRect",
		LLRect(MOVE_BTN_COL4, MOVE_BTN_ROW3,	MOVE_BTN_COL5,	MOVE_BTN_ROW2),
		"", NO_PERSIST  );
	gSavedSettings.declareRect("MoveDownBtnRect",
		LLRect(MOVE_BTN_COL4, MOVE_BTN_ROW2,	MOVE_BTN_COL5,	MOVE_BTN_ROW1),
		"", NO_PERSIST	);
	gSavedSettings.declareBOOL("FlyBtnState",			FALSE,	"", NO_PERSIST);
	gSavedSettings.declareBOOL("SitBtnState",			FALSE,	"", NO_PERSIST);
	gSavedSettings.declareRect("FlyBtnRect",
		LLRect(MOVE_BTN_FLY_LEFT, 20,	MOVE_BTN_FLY_RIGHT,	 4), "", NO_PERSIST	);
	gSavedSettings.declareBOOL("RunBtnState",			FALSE,	"", NO_PERSIST);
	gSavedSettings.declareRect("RunBtnRect",
		LLRect(MOVE_BTN_FLY_LEFT, 40,	MOVE_BTN_FLY_RIGHT,	 24), "", NO_PERSIST	);

	const S32 MOVE_WIDTH = MOVE_BTN_FLY_RIGHT + 4;
	const S32 MOVE_HEIGHT = MOVE_BTN_ROW3 + 4;
	gSavedSettings.declareRect("FloaterMoveRect", LLRect(0, MOVE_HEIGHT, MOVE_WIDTH, 0), "Rectangle for avatar control window");

	// 0 = never, 1 = fade, 2 = always
	gSavedSettings.declareS32("RenderName", 2, "Controls display of names above avatars (0 = never, 1 = fade, 2 = always)");
	gSavedSettings.declareF32("RenderNameShowTime", 10.f, "Fade avatar names after specified time (seconds)");		// seconds
	gSavedSettings.declareF32("RenderNameFadeDuration", 1.f, "Time interval over which to fade avatar names (seconds)");	// seconds
	gSavedSettings.declareBOOL("RenderNameHideSelf", FALSE, "Don't display own name above avatar");
	gSavedSettings.declareBOOL("RenderHideGroupTitle", FALSE, "Don't show my group title in my name label");
	gSavedSettings.declareBOOL("RenderGroupTitleAll", TRUE, "Show group titles in name labels");

	// Camera widget controls
	const S32 CAMERA_LEFT = MOVE_BTN_FLY_RIGHT + 10;
	const S32 CAMERA_WIDTH = 16 + 64 + 16 + 64 + 16;
	const S32 CAMERA_HEIGHT = 64;
	gSavedSettings.declareRect("FloaterCameraRect",
		LLRect(CAMERA_LEFT, CAMERA_HEIGHT, CAMERA_LEFT+CAMERA_WIDTH, 0), "Rectangle for camera control window");

	// Tool view
	LLRect floater_tools_rect;
	floater_tools_rect.setOriginAndSize(0, 300, TOOL_PANEL_WIDTH, 368);
	//gSavedSettings.declareRect("FloaterToolsRect",	floater_tools_rect, "[NOT USED]");
	gSavedSettings.declareRect("ToolHelpRect",	LLRect(8, TOOL_PANEL_HEIGHT-16, TOOL_PANEL_WIDTH -8, TOOL_PANEL_HEIGHT-16-16), "", NO_PERSIST); // relative to ToolPanelRect

	gSavedSettings.declareRect("FloaterFriendsRect", LLRect(0, 400, 250, 0), "Rectangle for friends window");
	gSavedSettings.declareRect("FloaterGroupsRect", LLRect(0, 300, 275, 0), "Rectangle for groups window");
	gSavedSettings.declareRect("FloaterSnapshotRect", LLRect(0, 200, 200, 400), "Rectangle for snapshot window");

	//gSavedSettings.declareRect("AccountHistoryRect", LLRect(100, 500, 500, 200), "[NOT USED]");

	gSavedSettings.declareRect("FloaterTeleportHistoryRect", LLRect(20, 20, 470, 200), "Rectangle for teleport history window");

	// Energy bar
	//gSavedSettings.declareBOOL("ShowEnergyPanel", FALSE, "[NOT USED]");
	gSavedSettings.declareS32("EnergyFromTop", 20, "", NO_PERSIST );
	gSavedSettings.declareS32("EnergyWidth", 175, "", NO_PERSIST );
	gSavedSettings.declareS32("EnergyHeight", 40, "", NO_PERSIST );

	gSavedSettings.declareBOOL("UIFloaterTestBool", FALSE, "Example saved setting for the test floater");

	//------------------------------------------------------------------------
	// UI UUIDS
	//------------------------------------------------------------------------
	gSavedSettings.declareString("UIImgBtnCloseInactiveUUID",	"779e4fa3-9b13-f74a-fba9-3886fe9c86ba", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnCloseActiveUUID",		"47a8c844-cd2a-4b1a-be01-df8b1612fe5d", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnClosePressedUUID",	"e5821134-23c0-4bd0-af06-7fa95b9fb01a", "", NO_PERSIST);

	gSavedSettings.declareString("UIImgBtnMinimizeInactiveUUID","6e72abba-1378-437f-bf7a-f0c15f3e99a3", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnMinimizeActiveUUID",	"34c9398d-bb78-4643-9633-46a2fa3e9637", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnMinimizePressedUUID",	"39801651-26cb-4926-af57-7af9352c273c", "", NO_PERSIST);

	gSavedSettings.declareString("UIImgBtnRestoreInactiveUUID",	"0eafa471-70af-4882-b8c1-40a310929744", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnRestoreActiveUUID",	"111b39de-8928-4690-b7b2-e17d5c960277", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnRestorePressedUUID",	"90a0ed5c-2e7b-4845-9958-a64a1b30f312", "", NO_PERSIST);

	gSavedSettings.declareString("UIImgBtnTearOffInactiveUUID",	"74e1a96f-4833-a24d-a1bb-1bce1468b0e7", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnTearOffActiveUUID",	"74e1a96f-4833-a24d-a1bb-1bce1468b0e7", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnTearOffPressedUUID",	"d2524c13-4ba6-af7c-e305-8ac6cc18d86a", "", NO_PERSIST);

	// Stay in IM after hitting return.
	gSavedSettings.declareBOOL("PinTalkViewOpen", TRUE, "Stay in IM after hitting return");

	// Close chat after hitting return.
	gSavedSettings.declareBOOL("CloseChatOnReturn", FALSE, "Close chat after hitting return");

	gSavedSettings.declareBOOL("HideNotificationsInChat", FALSE, "Do not echo and log notifications in the chat");

	// Copy IM messages into chat history
	gSavedSettings.declareBOOL("ChatHistoryTornOff", TRUE, "Show chat history window separately from Communicate window.");
	gSavedSettings.declareBOOL("UseOldChatHistory", FALSE, "Use the old style 'Chat history' window instead of the new 'Local chat' window (requires a restart).");
	gSavedSettings.declareBOOL("ShowFriends", FALSE, "", NO_PERSIST);
	gSavedSettings.declareBOOL("ShowGroups", FALSE, "", NO_PERSIST);
	gSavedSettings.declareBOOL("ShowAudioVolume", FALSE, "", NO_PERSIST);
	gSavedSettings.declareBOOL("IMInChatHistory", FALSE, "Copy IM into chat history");
	gSavedSettings.declareBOOL("IMShowTimestamps", TRUE, "Show timestamps in IM");
	gSavedSettings.declareBOOL("HideIMInChat", FALSE, "Hide IM in chat console");

	// Has the user intentionally entered chatting mode, hence wanting the
	// chat UI to be displayed, keyboard focus to go into chat, etc.
	gSavedSettings.declareBOOL("ChatVisible", FALSE, "Chat bar is visible");

	gSavedSettings.declareString("UIImgDirectionArrowUUID",		"586383e8-4d9b-4fba-9196-2b5938e79c2c", "", NO_PERSIST);

	gSavedSettings.declareString("UIImgResizeBottomRightUUID",	"e3690e25-9690-4f6c-a745-e7dcd885285a", "", NO_PERSIST);

	// Move buttons
	gSavedSettings.declareString("UIImgBtnForwardOutUUID",		"a0eb4021-1b20-4a53-892d-8faa9265a6f5", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnForwardInUUID",		"54197a61-f5d1-4c29-95d2-c071d08849cb", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnSlideLeftOutUUID",	"82476321-0374-4c26-9567-521535ab4cd7", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnSlideLeftInUUID",		"724996f5-b956-46f6-9844-4fcfce1d5e83", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnLeftOutUUID",			"13a93910-6b44-45eb-ad3a-4d1324c59bac", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnLeftInUUID",			"95463c78-aaa6-464d-892d-3a805b6bb7bf", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnRightOutUUID",		"5a44fd04-f52b-4c30-8b00-4a31e27614bd", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnRightInUUID",			"5e616d0d-4335-476f-9977-560bccd009da", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnSlideRightOutUUID",	"1fbe4e60-0607-44d1-a50a-032eff56ae75", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnSlideRightInUUID",	"7eeb57d2-3f37-454d-a729-8b217b8be443", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnMoveUpInUUID",		"49b4b357-e430-4b56-b9e0-05b8759c3c82", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnMoveUpOutUUID",		"f887146d-829f-4e39-9211-cf872b78f97c", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnMoveDownInUUID",		"b92a70b9-c841-4c94-b4b3-cee9eb460d48", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnMoveDownOutUUID",		"b5abc9fa-9e62-4e03-bc33-82c4c1b6b689", "", NO_PERSIST);

//	gSavedSettings.declareString("UIImgBtnPopupOutUUID",		"f41ecdbf-e4b7-4eae-80fa-f0c842d85c1c");
//	gSavedSettings.declareString("UIImgBtnPopupInUUID",			"432fd877-f2ad-45ce-8ae7-d1ced88462cb");

	// Scrollbar
	gSavedSettings.declareString("UIImgBtnScrollUpOutUUID",		"dad084d7-9a46-452a-b0ff-4b9f1cefdde9",	"", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnScrollUpInUUID",		"a93abdf3-27b5-4e22-a8fa-c48216cd2e3a",	"", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnScrollDownOutUUID",	"b4ecdecf-5c8d-44e7-b882-17a77e88ed55",	"", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnScrollDownInUUID",	"d2421bab-2eaf-4863-b8f6-5e4c52519247",	"", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnScrollLeftOutUUID",	"43773e8d-49aa-48e0-80f3-a04715f4677a",	"", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnScrollLeftInUUID",	"ea137a32-6718-4d05-9c22-7d570d27b2cd",	"", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnScrollRightOutUUID",	"3d700d19-e708-465d-87f2-46c8c0ee7938",	"", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnScrollRightInUUID",	"b749de64-e903-4c3c-ac0b-25fb6fa39cb5",	"", NO_PERSIST);

	gSavedSettings.declareString("UIImgBtnJumpLeftOutUUID",	    "3c18c87e-5f50-14e2-e744-f44734aa365f",	"", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnJumpLeftInUUID",	    "9cad3e6d-2d6d-107d-f8ab-5ba272b5bfe1",	"", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnJumpRightOutUUID",	"ff9a71eb-7414-4cf8-866e-a701deb7c3cf",	"", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnJumpRightInUUID",	    "7dabc040-ec13-2309-ddf7-4f161f6de2f4",	"", NO_PERSIST);

	// Spin control
	gSavedSettings.declareString("UIImgBtnSpinUpOutUUID",			"56576e6e-6710-4e66-89f9-471b59122794", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnSpinUpInUUID",			"c8450082-96a0-4319-8090-d3ff900b4954", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnSpinDownOutUUID",			"b6d240dd-5602-426f-b606-bbb49a30726d", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnSpinDownInUUID",			"a985ac71-052f-48e6-9c33-d931c813ac92", "", NO_PERSIST);

	// Radio button control
	gSavedSettings.declareString("UIImgRadioActiveUUID",			"7a1ba9b8-1047-4d1e-9cfc-bc478c80b63f", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgRadioActiveSelectedUUID",	"52f09e07-5816-4052-953c-94c6c10479b7", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgRadioInactiveUUID",			"90688481-67ff-4af0-be69-4aa084bcad1e", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgRadioInactiveSelectedUUID",	"1975db39-aa29-4251-aea0-409ac09d414d", "", NO_PERSIST);

	// Checkbox control
	gSavedSettings.declareString("UIImgCheckboxActiveUUID",			"05bb64ee-96fd-4243-b74e-f40a41bc53ba", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgCheckboxActiveSelectedUUID", "cf4a2ed7-1533-4686-9dde-df9a37ddca55", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgCheckboxInactiveUUID",		"7d94cb59-32a2-49bf-a516-9e5a2045f9d9", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgCheckboxInactiveSelectedUUID", "c817c642-9abd-4236-9287-ae0513fe7d2b", "", NO_PERSIST);

	// Tab panels
	gSavedSettings.declareString("UIImgBtnTabTopPartialOutUUID",	"932ad585-0e45-4a57-aa23-4cf81beeb7b0", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnTabTopPartialInUUID",		"7c6c6c26-0e25-4438-89bd-30d8b8e9d704", "",  NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnTabBottomPartialOutUUID",	"8dca716c-b29c-403a-9886-91c028357d6e", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnTabBottomPartialInUUID",	"eb0b0904-8c91-4f24-b500-1180b91140de", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnTabTopOutUUID",			"1ed83f57-41cf-4052-a3b4-2e8bb78d8191", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnTabTopInUUID",			"16d032e8-817b-4368-8a4e-b7b947ae3889", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnTabBottomOutUUID",		"bf0a8779-689b-48c3-bb9a-6af546366ef4", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnTabBottomInUUID",			"c001d8fd-a869-4b6f-86a1-fdcb106df9c7", "", NO_PERSIST);

	// Tools
	// TODO: Move to gViewerArt
	gSavedSettings.declareString("UIImgGrabUUID",					"c63f124c-6340-4fbf-b59e-0869a44adb64", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgGrabSelectedUUID",			"c1e21504-f136-451d-b8e9-929037812f1d", "", NO_PERSIST);

	gSavedSettings.declareString("UIImgMoveUUID",					"2fa5dc06-bcdd-4e09-a426-f9f262d4fa65", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgMoveSelectedUUID",			"46f17c7b-8381-48c3-b628-6a406e060dd6", "", NO_PERSIST);

	gSavedSettings.declareString("UIImgRotateUUID",					"c34b1eaa-aae3-4351-b082-e26c0b636779", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgRotateSelectedUUID",			"cdfb7fde-0d13-418a-9d89-2bd91019fc95", "", NO_PERSIST);

	gSavedSettings.declareString("UIImgScaleUUID",					"88a90fef-b448-4883-9344-ecf378a60433", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgScaleSelectedUUID",			"55aa57ef-508a-47f7-8867-85d21c5a810d", "", NO_PERSIST);

	gSavedSettings.declareString("UIImgFaceUUID",					"ce15fd63-b0b6-463c-a37d-ea6393208b3e", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgFaceSelectedUUID",			"b4870163-6208-42a9-9801-93133bf9a6cd", "", NO_PERSIST);

	gSavedSettings.declareString("UIImgFocusUUID",					"57bc39d1-288c-4519-aea6-6d1786a5c274", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgFocusSelectedUUID",			"ab6a730e-ddfd-4982-9a32-c6de3de6d31d", "", NO_PERSIST);

	gSavedSettings.declareString("UIImgCreateUUID",					"7a0b1bdb-b5d9-4df5-bac2-ba230da93b5b", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgCreateSelectedUUID",			"0098b015-3daf-4cfe-a72f-915369ea97c2", "", NO_PERSIST);

	// Gun Tool texures
	gSavedSettings.declareBOOL("ShowCrosshairs",					TRUE, "Display crosshairs when in mouselook mode");
	gSavedSettings.declareString("UIImgCrosshairsUUID",				"6e1a3980-bf2d-4274-8970-91e60d85fb52", "Image to use for crosshair display (UUID texture reference)");

	gSavedSettings.declareString("Language", 						"default", "Language specifier (for XUI)" );
	gSavedSettings.declareString("SystemLanguage", 					"en-us", "Language indicated by system settings (for XUI)" );
	
	/////////////////////////////////////////////////
	// Other booleans
	gSavedSettings.declareBOOL("DebugPermissions",					FALSE, "Log permissions for selected inventory items");

	gSavedSettings.declareBOOL("ApplyColorImmediately", TRUE, "Preview selections in color picker immediately");
	gSavedSettings.declareBOOL("ApplyTextureImmediately", TRUE, "Preview selections in texture picker immediately");

	gSavedSettings.declareBOOL("CreateToolKeepSelected", FALSE, "After using create tool, keep the create tool active");
	gSavedSettings.declareBOOL("CreateToolCopySelection", FALSE, "", NO_PERSIST);
	gSavedSettings.declareBOOL("CreateToolCopyCenters", TRUE, "", NO_PERSIST);
	gSavedSettings.declareBOOL("CreateToolCopyRotates", FALSE, "", NO_PERSIST);

	gSavedSettings.declareBOOL("QuietSnapshotsToDisk", FALSE, "Take snapshots to disk without playing animation or sound");
	gSavedSettings.declareBOOL("DisableCameraConstraints", TRUE, "Disable the normal bounds put on the camera by avatar position");
	
	//gSavedSettings.declareBOOL("LogTimestamps", FALSE, "[NOT USED]");
	//gSavedSettings.declareBOOL("AgentUpdateMouseQuery", FALSE, "[NOT USED]");
	gSavedSettings.declareBOOL("AutoLogin", FALSE, "Login automatically using last username/password combination");
	gSavedSettings.declareBOOL("LoginAsGod", FALSE, "Attempt to login with god powers (Linden accounts only)");
	//gSavedSettings.declareBOOL("CameraFromPelvis", FALSE, "[NOT USED]");
	gSavedSettings.declareBOOL("CameraOffset", FALSE, "Render with camera offset from view frustum (rendering debug)");
	//gSavedSettings.declareBOOL("DynamicNearClip", TRUE, "[NOT USED]");
	gSavedSettings.declareBOOL("AnimationDebug", FALSE, "Show active animations in a bubble above avatars head");
	gSavedSettings.declareBOOL("DisplayAvatarAgentTarget", FALSE, "Show avatar positioning locators (animation debug)");
	//gSavedSettings.declareBOOL("DisplaySkeletons", TRUE, "[NOT USED]");
	gSavedSettings.declareBOOL("DisplayTimecode", FALSE, "Display timecode on screen");
	//gSavedSettings.declareBOOL("Drone", FALSE, "[NOT USED]");
	gSavedSettings.declareBOOL("DisableRendering", FALSE, "Disable GL rendering and GUI (load testing)");
	//gSavedSettings.declareBOOL("DumpPolyMeshTable", FALSE, "[NOT USED]");
	//gSavedSettings.declareBOOL("LimitAvatarToValidRegions", TRUE, "[NOT USED]");
	gSavedSettings.declareBOOL("VerboseLogs", FALSE, "Display source file and line number for each log item for debugging purposes");
	gSavedSettings.declareBOOL("FirstPersonAvatarVisible", FALSE, "Display avatar and attachments below neck while in mouselook");
	gSavedSettings.declareBOOL("ShowNearClip", FALSE, "", NO_PERSIST);
	gSavedSettings.declareBOOL("DebugWindowProc", FALSE, "Log windows messages");
	gSavedSettings.declareBOOL("ShowTangentBasis", FALSE, "Render normal and binormal (debugging bump mapping)");
	gSavedSettings.declareBOOL("AnimateTextures", TRUE, "Enable texture animation (debug)");

	// Selection stuff
	gSavedSettings.declareBOOL("LimitSelectDistance", TRUE, "Disallow selection of objects beyond max select distance");
	gSavedSettings.declareF32( "MaxSelectDistance", 32.f, "Maximum allowed selection distance (meters from avatar)");
	gSavedSettings.declareBOOL("LimitDragDistance", TRUE, "Limit translation of object via translate tool");
	gSavedSettings.declareF32( "MaxDragDistance", 48.f, "Maximum allowed translation distance in a single operation of translate tool (meters from start point)");
	gSavedSettings.declareBOOL("SelectOwnedOnly", FALSE, "Select only objects you own" );
	gSavedSettings.declareBOOL("SelectMovableOnly", FALSE, "Select only objects you can move" );
	gSavedSettings.declareBOOL("RectangleSelectInclusive", TRUE, "Select objects that have at least one vertex inside selection rectangle" );
	gSavedSettings.declareBOOL("RenderHiddenSelections", TRUE, "Show selection lines on objects that are behind other objects" );
	gSavedSettings.declareBOOL("RenderLightRadius", FALSE, "Render the radius of selected lights" );

	gSavedSettings.declareF32("SelectionHighlightThickness", 0.010f, "Thickness of selection highlight line (fraction of view distance)");
	gSavedSettings.declareF32("SelectionHighlightUScale", 0.1f, "Scale of texture display on selection highlight line (fraction of texture size)");
	gSavedSettings.declareF32("SelectionHighlightVScale", 1.f, "Scale of texture display on selection highlight line (fraction of texture size)");
	gSavedSettings.declareF32("SelectionHighlightAlpha", 0.40f, "Opacity of selection highlight (0.0 = completely transparent, 1.0 = completely opaque)" );
	gSavedSettings.declareF32("SelectionHighlightAlphaTest", 0.1f, "Alpha value below which pixels are displayed on selection highlight line (0.0 = show all pixels, 1.0 = show now pixels)");
	gSavedSettings.declareF32("SelectionHighlightUAnim", 0.f, "Rate at which texture animates along U direction in selection highlight line (fraction of texture per second)");
	gSavedSettings.declareF32("SelectionHighlightVAnim", 0.5f, "Rate at which texture animates along V direction in selection highlight line (fraction of texture per second)");

	gSavedSettings.declareBOOL("LogMessages", FALSE, "Log network traffic");
	gSavedSettings.declareBOOL("MouseSun", FALSE, "", NO_PERSIST);

	gSavedSettings.declareBOOL("ShowAxes", FALSE, "Render coordinate frame at your position");

	gSavedSettings.declareBOOL("ShowMiniMap", TRUE, "Display mini map on login");
	gSavedSettings.declareBOOL("UseOldTrackingDots", FALSE, "Use old T-like tracking dots in mini-map (takes effect after viewer restart)");
	gSavedSettings.declareBOOL("ShowWorldMap", FALSE, "Display world map on login");
	gSavedSettings.declareBOOL("ShowToolBar", TRUE, "Show toolbar at bottom of screen");
	gSavedSettings.declareBOOL("ShowCameraControls", FALSE, "Display camera controls on login");
	gSavedSettings.declareBOOL("ShowMovementControls", FALSE, "Display movement controls on login");

	gSavedSettings.declareBOOL("ShowWebBrowser", FALSE, "Show in-world web browser");

	gSavedSettings.declareBOOL("ShowLeaders", FALSE, "", NO_PERSIST);
	gSavedSettings.declareBOOL("ShowDirectory", FALSE, "", NO_PERSIST);

	gSavedSettings.declareBOOL("AutoLoadWebProfiles", FALSE, "Automatically load ALL profile webpages without asking first.");

	gSavedSettings.declareBOOL("ShowCommunicate", FALSE, "", NO_PERSIST);
	gSavedSettings.declareBOOL("ShowChatHistory", FALSE, "", NO_PERSIST);

#ifdef LL_RELEASE_FOR_DOWNLOAD
	gSavedSettings.declareBOOL("ShowConsoleWindow", FALSE, "Show log in separate OS window");
#else
	gSavedSettings.declareBOOL("ShowConsoleWindow", TRUE, "Show log in separate OS window");
#endif

	// These are ignorable warnings

	gSavedSettings.addWarning("AboutDirectX9");
	gSavedSettings.addWarning("AboutBadPCI");
	gSavedSettings.addWarning("AboutOldGraphicsDriver");
	gSavedSettings.addWarning("AboutPCIGraphics");
	gSavedSettings.addWarning("ReturnToOwner");
	gSavedSettings.addWarning("QuickTimeInstalled");
	gSavedSettings.addWarning("BrowserLaunch");
	gSavedSettings.addWarning("DeedObject");
	gSavedSettings.addWarning("NewClassified");

	// These are warnings that appear on the first experience of that condition.
	
	LLFirstUse::addConfigVariable("FirstBalanceIncrease");
	LLFirstUse::addConfigVariable("FirstBalanceDecrease");
	LLFirstUse::addConfigVariable("FirstSit");
	LLFirstUse::addConfigVariable("FirstMap");
	LLFirstUse::addConfigVariable("FirstGoTo");
	LLFirstUse::addConfigVariable("FirstBuild");
	LLFirstUse::addConfigVariable("FirstLeftClickNoHit");
	LLFirstUse::addConfigVariable("FirstTeleport");
	LLFirstUse::addConfigVariable("FirstOverrideKeys");
	LLFirstUse::addConfigVariable("FirstAttach");
	LLFirstUse::addConfigVariable("FirstAppearance");
	LLFirstUse::addConfigVariable("FirstInventory");
	LLFirstUse::addConfigVariable("FirstSandbox");
	LLFirstUse::addConfigVariable("FirstFlexible");
	LLFirstUse::addConfigVariable("FirstDebugMenus");
	LLFirstUse::addConfigVariable("FirstStreamingMusic");
	LLFirstUse::addConfigVariable("FirstStreamingVideo");
	LLFirstUse::addConfigVariable("FirstSculptedPrim");
	LLFirstUse::addConfigVariable("FirstVoice");

	gSavedSettings.declareBOOL("ShowDebugConsole", FALSE, "Show log in SL window");
	gSavedSettings.declareBOOL("ShowDebugStats", FALSE, "Show performance stats display");
	gSavedSettings.declareBOOL("OpenDebugStatBasic", TRUE, "Expand basic performance stats display");
	gSavedSettings.declareBOOL("OpenDebugStatAdvanced", FALSE, "Expand advanced performance stats display");
	gSavedSettings.declareBOOL("OpenDebugStatNet", TRUE, "Expand network stats display");
	gSavedSettings.declareBOOL("OpenDebugStatRender", TRUE, "Expand render stats display");
	gSavedSettings.declareBOOL("OpenDebugStatSim", TRUE, "Expand simulator performance stats display");
	gSavedSettings.declareBOOL("ShowDepthBuffer", FALSE, "Show depth buffer contents");

	gSavedSettings.declareBOOL("DebugShowTime", FALSE, "Show depth buffer contents");
	gSavedSettings.declareBOOL("DebugShowRenderInfo", FALSE, "Show depth buffer contents");
	
//	gSavedSettings.declareBOOL("ShowHUD", TRUE);
	//gSavedSettings.declareBOOL("ShowHUDText", TRUE, "[NOT USED]");
	//gSavedSettings.declareBOOL("ShowHeadlight", FALSE, "[NOT USED]");
	//gSavedSettings.declareBOOL("ShowLand", TRUE, "[NOT USED]");
//	gSavedSettings.declareBOOL("ShowMove", TRUE);
	//gSavedSettings.declareBOOL("SurfaceDetail", TRUE, "[NOT USED]");
	//gSavedSettings.declareBOOL("ShowObjectBounds", FALSE, "[NOT USED]");
	//gSavedSettings.declareBOOL("ShowObjectEdit", FALSE, "[NOT USED]");
	gSavedSettings.declareBOOL("ShowObjectUpdates", FALSE, "Show when update messages are received for individual objects");
	//gSavedSettings.declareBOOL("ShowObjects", TRUE, "[NOT USED]");
	//gSavedSettings.declareBOOL("ShowRegions", FALSE, "[NOT USED]");
//	gSavedSettings.declareBOOL("ShowTalk", TRUE);
	//gSavedSettings.declareBOOL("ShowTimerBar", FALSE, "[NOT USED]");
	//gSavedSettings.declareBOOL("ShowWater", FALSE, "[NOT USED]");
	gSavedSettings.declareBOOL("SpeedTest", FALSE, "Performance testing mode, no network");
	//gSavedSettings.declareBOOL("TempMouseLook", FALSE, "[NOT USED]");
	gSavedSettings.declareBOOL("UseEnergy", TRUE, "", NO_PERSIST);
//	gSavedSettings.declareBOOL("UseFirstPersonDrag", FALSE);
	//gSavedSettings.declareBOOL("UseLighting", TRUE, "[NOT USED]");
	//gSavedSettings.declareBOOL("UseWireframe", FALSE, NO_PERSIST);
	gSavedSettings.declareBOOL("VelocityInterpolate", TRUE, "Extrapolate object motion from last packet based on received velocity");
	gSavedSettings.declareBOOL("PingInterpolate", FALSE, "Extrapolate object position along velocity vector based on ping delay");
	gSavedSettings.declareBOOL("AvatarBacklight", FALSE, "Add rim lighting to avatar rendering to approximate shininess of skin");

	// Startup stuff
	gSavedSettings.declareF32("PrecachingDelay", 6.f, "Delay when logging in to load world before showing it (seconds)");	// seconds

	// Rendering stuff
	gSavedSettings.declareF32("RenderGamma",				0.f, "Sets gamma exponent for renderer");
	gSavedSettings.declareF32("RenderNightBrightness",		1.5f, "Brightness multiplier for moon during nighttime" );
	gSavedSettings.declareBOOL("RenderWater",				TRUE, "Display water" );
	gSavedSettings.declareF32("RenderFarClip",			32.f, "Distance of far clip plane from camera (meters)" );
	gSavedSettings.declareF32("RenderFogRatio",			4.0f, "Distance from camera where fog reaches maximum density (fraction or multiple of far clip distance)");
	gSavedSettings.declareBOOL("RenderAnisotropic",			FALSE, "Render textures using anisotropic filtering" );
	gSavedSettings.declareBOOL("ShowXUINames",			FALSE, "Display XUI Names as Tooltips" );
	gSavedSettings.declareS32("RenderLightingDetail",		0, "Amount of detail for lighting objects/avatars/terrain (0=sun/moon only, 1=enable local lights)" );
	gSavedSettings.declareS32("RenderTerrainDetail",		0, "Detail applied to terrain texturing (0 = none, 1 or 2 = full)" );
	gSavedSettings.declareF32("RenderVolumeLODFactor",		0.f, "Controls level of detail of primitives (multiplier for current screen area when calculated level of detail)" );
	gSavedSettings.declareF32("RenderFlexTimeFactor",		0.f, "Controls level of detail of flexible objects (multiplier for amount of time spent processing flex objects)" );
	gSavedSettings.declareF32("RenderTreeLODFactor",		0.f, "Controls level of detail of vegetatopm (multiplier for current screen area when calculated level of detail)" );
	gSavedSettings.declareF32("RenderAvatarLODFactor",		0.f, "Controls level of detail of avatars (multiplier for current screen area when calculated level of detail)" );
	gSavedSettings.declareF32("RenderBumpmapMinDistanceSquared", 100.f, "Maximum distance at which to render bumpmapped primitives (distance in meters, squared)" );
	gSavedSettings.declareS32("RenderMaxPartCount",			256, "Maximum number of particles to display on screen");
	gSavedSettings.declareBOOL("RenderVBOEnable",			TRUE, "Use GL Vertex Buffer Objects" );
	gSavedSettings.declareS32("RenderReflectionRes",		64, "Reflection map resolution.");
	//gSavedSettings.declareBOOL("RenderUseTriStrips",		FALSE, "[NOT USED]");
	//gSavedSettings.declareBOOL("RenderCullBySize",		FALSE, "[NOT USED]" );
	gSavedSettings.declareF32("RenderTerrainScale",			12.f, "Terrain detail texture scale");
	gSavedSettings.declareBOOL("VertexShaderEnable",		FALSE, "Enable/disable all GLSL shaders (debug)");
	gSavedSettings.declareBOOL("RenderInitError",			FALSE, "Error occured while initializing GL");
	gSavedSettings.declareBOOL("RenderRippleWater",			FALSE, "Display more realistic water, with refraction (requires pixel shader support on your video card)");
	gSavedSettings.declareBOOL("RenderDynamicReflections",	FALSE, "Generate a dynamic cube map for reflections (objects reflect their environment, experimental).");
	gSavedSettings.declareBOOL("RenderGlow",				FALSE, "Make light sources glow.");
	gSavedSettings.declareF32("RenderGlowStrength",			1.25f, "Strength of glow");
	gSavedSettings.declareS32("RenderGlowSize",			5, "Size of glow (in pixels)");
	gSavedSettings.declareS32("RenderGlowResolution",		256, "Glow map resolution.");
	gSavedSettings.declareBOOL("RenderObjectBump",			FALSE, "Show bumpmapping on primitives");
	gSavedSettings.declareS32("RenderAvatarMode",			0, "Controls how avatars are rendered (0 = normal, 1 = bump mapped, 2 = bump mapped and wavy cloth)");
	gSavedSettings.declareBOOL("RenderAvatarVP",			FALSE, "Use vertex programs to perform hardware skinning of avatar");
	gSavedSettings.declareS32("RenderAvatarMaxVisible",		35, "Maximum number of avatars to display at any one time");
	//gSavedSettings.declareBOOL("RenderForceGetTexImage",	FALSE, "[NOT USED]");
	gSavedSettings.declareBOOL("RenderFastUI",			FALSE, "[NOT USED]");
	gSavedSettings.declareBOOL("RenderUseSharedDrawables",	TRUE, "Collapse transforms on moving linked objects for faster updates");
	gSavedSettings.declareS32("DebugBeaconLineWidth", 		1, "Size of lines for Debug Beacons");

	// Snapshot params
	gSavedSettings.declareBOOL("RenderUIInSnapshot",		FALSE, "Display user interface in snapshot" );
	gSavedSettings.declareBOOL("RenderHUDInSnapshot",		FALSE, "Display HUD attachments in snapshot" );
	gSavedSettings.declareBOOL("HighResSnapshot",			FALSE, "Double resolution of snapshot from current window resolution" );
	gSavedSettings.declareBOOL("CompressSnapshotsToDisk",		FALSE, "Compress snapshots saved to disk (Using JPEG 2000)" );
	gSavedSettings.declareBOOL("FreezeTime",				FALSE, "", FALSE );
	gSavedSettings.declareBOOL("UseFreezeFrame",			FALSE, "Freeze time when taking snapshots.");
	gSavedSettings.declareBOOL("CloseSnapshotOnKeep",		TRUE, "Close snapshot window after saving snapshot" );
	gSavedSettings.declareBOOL("KeepAspectForSnapshot",		FALSE, "Use full window when taking snapshot, regardless of requested image size" );
	gSavedSettings.declareBOOL("AutoSnapshot",			TRUE, "Update snapshot when camera stops moving, or any parameter changes" );
	gSavedSettings.declareS32("LastSnapshotType",			0, "Select this as next type of snapshot to take (0 = postcard, 1 = texture, 2 = local image)" );
	gSavedSettings.declareS32("LastSnapshotWidth",			1024, "The width of the last snapshot, in px" );
	gSavedSettings.declareS32("LastSnapshotHeight",			600, "The height of the last snapshot, in px" );
	
	gSavedSettings.declareS32("SnapshotPostcardLastResolution",	0, "Take next postcard snapshot at this resolution" );
	gSavedSettings.declareS32("SnapshotTextureLastResolution",	0, "Take next texture snapshot at this resolution" );
	gSavedSettings.declareS32("SnapshotLocalLastResolution",	0, "Take next local snapshot at this resolution" );
	gSavedSettings.declareS32("SnapshotQuality",			75, "Quality setting of postcard JPEGs (0 = worst, 100 = best)" );
	gSavedSettings.declareS32("SnapshotFormat",         		0, "Format of save-to-disk snapshots (0=PNG, 1=JPEG, 2=BMP)" );

	gSavedSettings.declareBOOL("DisableVerticalSync",		TRUE, "Update frames as fast as possible (FALSE = update frames between display scans)" );

	// Statistics stuff
	gSavedSettings.declareBOOL("StatsAutoRun", FALSE, "Play back autopilot");
	gSavedSettings.declareS32("StatsNumRuns", -1, "Loop autopilot playback this number of times");
	//gSavedSettings.declareBOOL("StatsContinuousLoop", FALSE, "[NOT USED]");
	gSavedSettings.declareBOOL("StatsQuitAfterRuns", FALSE, "Quit application after this number of autopilot playback runs");
	gSavedSettings.declareBOOL("StatsSessionTrackFrameStats", FALSE, "Track rendering and network statistics");
	gSavedSettings.declareString("StatsPilotFile", "pilot.txt", "Filename for stats logging autopilot path");
	gSavedSettings.declareString("StatsSummaryFile", "fss.txt", "Filename for stats logging summary");
	gSavedSettings.declareString("StatsFile", "fs.txt", "Filename for stats logging output");

	// Image pipeline stuff
	gSavedSettings.declareS32("GraphicsCardMemorySetting", -1, "Amount of memory on your video card (-1 = autodetect, 0 = 16MB, 1 = 32MB, 2 = 64MB, 3 = 128MB, 4 = 256MB, 5 = 512MB)"); // default to auto-detect
	//gSavedSettings.declareS32("ImageRadioTexMem", 0, "Texture memory allocation (0 = <512 megabytes system RAM, 1 = >512 megabytes system RAM)");
	//gSavedSettings.declareS32("ImageRadioVidCardMem", 1, "Video card onboard memory (0 = 16MB, 1 = 32MB, 2 = 64MB, 3 = 128MB, 4 = 256MB, 5 = 512MB)");
	//gSavedSettings.declareU32("LastRAMDetected", 0, "[DO NOT MODIFY] Detected system memory (bytes)");  // used to detect RAM changes
	gSavedSettings.declareBOOL("ImagePipelineUseHTTP", FALSE, "If TRUE use HTTP GET to fetch textures from the server");

	// Image compression
	gSavedSettings.declareBOOL("LosslessJ2CUpload", FALSE, "Use lossless compression for small image uploads");
	
	// Threading
	gSavedSettings.declareBOOL("RunMultipleThreads", FALSE, "If TRUE keep background threads active during render");

	// Cooperative Multitasking
	gSavedSettings.declareS32("BackgroundYieldTime", 40, "Amount of time to yield every frame to other applications when SL is not the foreground window (milliseconds)");

	// Camera control
	gSavedSettings.declareBOOL("AutoPilotLocksCamera", FALSE, "Keep camera position locked when avatar walks to selected position");
	//gSavedSettings.declareBOOL("AvatarLooksAtCamera", TRUE, "[NOT USED]");
	//gSavedSettings.declareF32("FlyHeightOffGround",		1.f, "[NOT USED]");
	gSavedSettings.declareF32("DynamicCameraStrength", 2.f, "Amount camera lags behind avatar motion (0 = none, 30 = avatar velocity)");

	gSavedSettings.declareVec3("CameraOffsetBuild",		LLVector3(-6.0f,  0,  6.0f), "Default camera position relative to focus point when entering build mode");
	gSavedSettings.declareVec3("CameraOffsetDefault",	LLVector3(-3.0f,  0,  0.75f), "Default camera offset from avatar");
	//gSavedSettings.declareVec3("CameraOffsetDefault",	LLVector3(-3.0f,  0,  1.5f));

	//gSavedSettings.declareVec3("FocusOffsetBuild",		LLVector3(4,  0,  0), "[NOT USED]");
	gSavedSettings.declareVec3("FocusOffsetDefault",	LLVector3(1,  0,  1), "Default focus point offset relative to avatar (x-axis is forward)");
	gSavedSettings.declareBOOL("TrackFocusObject", TRUE, "Camera tracks last object zoomed on");
	gSavedSettings.declareBOOL("CameraMouseWheelZoom", TRUE, "Camera zooms in and out with mousewheel");

	gSavedSettings.declareVec3d("FocusPosOnLogout",		LLVector3d(0,  0,  0), "Camera focus point when last logged out (global coordinates)");
	gSavedSettings.declareVec3d("CameraPosOnLogout",	LLVector3d(0,  0,  0), "Camera position when last logged out (global coordinates)");

	// Terrain coloring
	// JC 8/28/2002 - Adjusted to make the beta farm look good, with
	// 20 meter water height.  Talk with me before changing these.
	gSavedSettings.declareF32("TerrainColorStartHeight", 20.f, "Starting altitude for terrain texturing (meters)"); // -1 to 1
	gSavedSettings.declareF32("TerrainColorHeightRange", 60.f, "Altitude range over which a given terrain texture has effect (meters)");	// max land height

	// Avatar stuff
	gSavedSettings.declareF32("PitchFromMousePosition", 90.f, "Vertical range over which avatar head tracks mouse position (degrees of head rotation from top of window to bottom)");
	gSavedSettings.declareF32("YawFromMousePosition", 90.f, "Horizontal range over which avatar head tracks mouse position (degrees of head rotation from left of window to right)");
	gSavedSettings.declareF32("ZoomTime", 0.4f, "Time of transition between different camera modes (seconds)");
	gSavedSettings.declareS32("AvatarCompositeLimit", 5, "Maximum number of avatars to display appearance changes on the fly");

	// Default throttle
	// These must also be changed in llviewerthrottle.h
	// Currently matches BW_PRESET_300
	gSavedSettings.declareF32("ThrottleBandwidthKBPS", 1000.f, "Maximum allowable downstream bandwidth (kilo bits per second)");

	gSavedSettings.declareBOOL("ConnectionPortEnabled", FALSE, "Use the custom connection port?");
	gSavedSettings.declareU32("ConnectionPort", 13000, "Custom connection port number");

	// File xfer throttle
	gSavedSettings.declareF32("XferThrottle", 150000.f, "Maximum allowable downstream bandwidth for asset transfers (bits per second)");

	//gSavedSettings.declareS32("BWRadio", 0, "[NOT USED]");

	gSavedSettings.declareRect("ChatterboxRect", LLRect(0, 400, 350, 0), "Rectangle for chatterbox window");
	gSavedSettings.declareRect("FloaterActiveSpeakersRect", LLRect(0, 300, 250, 0), "Rectangle for active speakers window");

	// Avatar customizing floaters
	gSavedSettings.declareRect("FloaterCustomizeAppearanceRect", LLRect(0, 540, 494, 0), "Rectangle for avatar customization window");

	// Build options floater
	gSavedSettings.declareRect("FloaterBuildOptionsRect", LLRect(0,0,0,0), "Rectangle for build options window.");
	
	gSavedSettings.declareRect("FloaterJoystickRect", LLRect(0,0,0,0), "Rectangle for joystick controls window.");

	// Map floater
	gSavedSettings.declareRect("FloaterMapRect", LLRect(0, 225, 200, 0), "Rectangle for world map");

	//Lag-o-Meter floater
	gSavedSettings.declareRect("FloaterLagMeter", LLRect(0, 142, 350, 0), "Rectangle for lag meter");
	gSavedSettings.declareBOOL("LagMeterShrunk", FALSE, "Last large/small state for lag meter");

	gSavedSettings.declareF32("MapScale", 128.f, "World map zoom level (pixels per region)");

	gSavedSettings.declareF32("MiniMapScale", 128.f, "Miniature world map zoom levle (pixels per region)");

	gSavedSettings.declareBOOL("MiniMapRotate",	TRUE, "Rotate miniature world map to avatar direction");

	gSavedSettings.declareString("UIImgBtnPanUpOutUUID",	"47a8c844-cd2a-4b1a-be01-df8b1612fe5d", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnPanUpInUUID",		"e5821134-23c0-4bd0-af06-7fa95b9fb01a", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnPanDownOutUUID",	"47a8c844-cd2a-4b1a-be01-df8b1612fe5d", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnPanDownInUUID",	"e5821134-23c0-4bd0-af06-7fa95b9fb01a", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnPanLeftOutUUID",	"47a8c844-cd2a-4b1a-be01-df8b1612fe5d", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnPanLeftInUUID",	"e5821134-23c0-4bd0-af06-7fa95b9fb01a", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnPanRightOutUUID",	"47a8c844-cd2a-4b1a-be01-df8b1612fe5d", "", NO_PERSIST);
	gSavedSettings.declareString("UIImgBtnPanRightInUUID",	"e5821134-23c0-4bd0-af06-7fa95b9fb01a", "", NO_PERSIST);

	// Talk panel
	gSavedSettings.declareRect("FloaterIMRect",				LLRect(0, 10*16, 500, 0), "Rectangle for IM window");

	// Chat floater
	// Rectangle should almost fill the bottom of the screen on 800x600
	// Note that the saved rect size is the size with history shown.
	gSavedSettings.declareRect("FloaterChatRect",			LLRect( 0, 10*16 + 12, 500, 0 ), "Rectangle for chat history");
	gSavedSettings.declareRect("FloaterContactsRect",		LLRect( 0, 390, 395, 0 ), "Rectangle for chat history");
	gSavedSettings.declareRect("FloaterMuteRect3",			LLRect( 0, 300, 300, 0), "Rectangle for mute window");
	gSavedPerAccountSettings.declareString("BusyModeResponse",		"The Resident you messaged is in 'busy mode' which means they have requested not to be disturbed.  Your message will still be shown in their IM panel for later viewing.", "Auto response to instant messages while in busy mode.");
	gSavedPerAccountSettings.declareString("InstantMessageLogPath",	"", "Path to your log files.");
	gSavedPerAccountSettings.declareBOOL("LogInstantMessages",	TRUE, "Log Instant Messages");
	gSavedPerAccountSettings.declareBOOL("LogChat",	TRUE, "Log Chat");
	gSavedPerAccountSettings.declareBOOL("LogShowHistory",	TRUE, "Log Show History");
	gSavedPerAccountSettings.declareBOOL("IMLogTimestamp",	TRUE, "Log Timestamp of Instant Messages");
	gSavedPerAccountSettings.declareBOOL("LogChatTimestamp",	TRUE, "Log Timestamp of Chat");
	gSavedPerAccountSettings.declareBOOL("LogChatIM",	FALSE, "Log Incoming Instant Messages with Chat");
	gSavedPerAccountSettings.declareBOOL("LogTimestampDate",	TRUE, "Include Date with Timestamp");

	// Inventory
	gSavedSettings.declareRect("FloaterInventoryRect", LLRect(0, 400, 300, 0), "Rectangle for inventory window" );

	gSavedSettings.declareRect("FloaterPrimImportRect", LLRect(0, 0, 0, 0), "Rectangle for the object import window" );

	// properties, only width and height is used.
	gSavedSettings.declareRect("PropertiesRect", LLRect(0, 320, 350, 0), "Rectangle for inventory item properties window");

	// Previews - only width and height are used
	gSavedSettings.declareRect("PreviewTextureRect",			LLRect(0, 400, 400, 0), "Rectangle for texture preview window" );  // Only width and height are used
	gSavedSettings.declareRect("PreviewScriptRect",				LLRect(0, 550, 500, 0), "Rectangle for script preview window" );  // Only width and height are used
	gSavedSettings.declareRect("LSLHelpRect",					LLRect(0, 400, 400, 0), "Rectangle for LSL help window" );  // Only width and height are used
	gSavedSettings.declareRect("PreviewLandmarkRect",			LLRect(0,  90, 300, 0), "Rectangle for landmark preview window" );  // Only width and height are used
	gSavedSettings.declareRect("PreviewURLRect",				LLRect(0,  90, 300, 0), "Rectangle for URL preview window" );  // Only width and height are used
	gSavedSettings.declareRect("PreviewEventRect",				LLRect(0, 530, 420, 0), "Rectangle for Event preview window" );  // Only width and height are used
	gSavedSettings.declareRect("PreviewClassifiedRect",			LLRect(0, 530, 420, 0), "Rectangle for URL preview window" );  // Only width and height are used
	gSavedSettings.declareRect("PreviewSoundRect",				LLRect(0,  85, 300, 0), "Rectangle for sound preview window" );  // Only width and height are used
	gSavedSettings.declareRect("PreviewObjectRect",				LLRect(0,  85, 300, 0), "Rectangle for object preview window" );  // Only width and height are used
	gSavedSettings.declareRect("PreviewWearableRect",			LLRect(0,  85, 300, 0), "Rectangle for wearable preview window" );  // Only width and height are used
	gSavedSettings.declareRect("PreviewAnimRect",				LLRect(0,  85, 300, 0), "Rectangle for animation preview window" );  // Only width and height are used
	// permissions manager
	gSavedSettings.declareRect("PermissionsManagerRect",		LLRect(0,  85, 300, 0), "Rectangle for permissions manager window" );  // Only width and height are used

	// Land floater - force to top left
	//gSavedSettings.declareRect("FloaterLandRect3", LLRect(0, 370, 340, 0));
	//gSavedSettings.declareRect("FloaterLandRect4", LLRect(0, 370, 400, 0), "Rectangle for About Land window"); // deprecated
	gSavedSettings.declareRect("FloaterLandRect5", LLRect(0, 370, 460, 0), "Rectangle for About Land window");

	// Texture Picker
	gSavedSettings.declareRect("TexturePickerRect",				LLRect(0, 290, 350, 0), "Rectangle for texture picker" );  // Only width and height are used
	gSavedSettings.declareBOOL("TexturePickerShowFolders", TRUE, "Show folders with no texures in texture picker");

	gSavedSettings.declareRect("FloaterGestureRect", LLRect(0, 480, 320, 0), "Rectangle for gestures window");
	gSavedSettings.declareRect("FloaterClothingRect", LLRect(0, 480, 320, 0), "Rectangle for clothing window");
	gSavedSettings.declareBOOL("ClothingBtnState", FALSE, "", NO_PERSIST);
	gSavedSettings.declareRect("FloaterHTMLRect", LLRect(0, 500, 700, 0), "Rectangle for HTML window");

	gSavedSettings.declareRect("FloaterRegionInfo", LLRect(0, 512, 480, 0), "Rectangle for region info window");

	// Landmark Picker
	gSavedSettings.declareRect("FloaterLandmarkRect", LLRect(0, 290, 310, 0), "Rectangle for landmark picker" );  // Only width and height are used

	// editors
	// Only width and height are used
	gSavedSettings.declareRect("NotecardEditorRect", LLRect(0, 400, 400, 0), "Rectangle for notecard editor");

	// recompile everything dialog
	gSavedSettings.declareRect("CompileOutputRect", LLRect(0, 400, 300, 0), "Rectangle for script Recompile Everything output window");

	// L$
	gSavedSettings.declareRect("FloaterPayRectB", LLRect(0, 150, 400, 0), "Rectangle for pay window");

	// Buy
	gSavedSettings.declareRect("FloaterBuyRect", LLRect(0, 250, 300, 0), "Rectangle for buy window");

	// Buy Contents
	gSavedSettings.declareRect("FloaterBuyContentsRect", LLRect(0, 250, 300, 0), "Rectangle for Buy Contents window");

	// Open Contents
	gSavedSettings.declareRect("FloaterOpenObjectRect", LLRect(0, 350, 300, 0), "Rectangle for Open Object window");

	// the about box
	gSavedSettings.declareRect("FloaterAboutRect", LLRect(0, 440, 470, 0), "Rectangle for About window");

	// the mean box
	gSavedSettings.declareRect("FloaterBumpRect", LLRect(0, 180, 400, 0), "Rectangle for Bumps/Hits window");

	// the inspect box
	gSavedSettings.declareRect("FloaterInspectRect", LLRect(0, 400, 400, 0), "Rectangle for Object Inspect window");

	// World map.  If 0,0,0,0, will attempt to size to fullscreen.
	gSavedSettings.declareRect("FloaterWorldMapRect",
								LLRect(0,0,0,0), "Rectangle for world map window");

	// Find dialog.
	gSavedSettings.declareRect("FloaterFindRect2", LLRect(0, 570, 780, 0), "Rectangle for Find window");

	// Talk To dialog, force to top of screen
	//gSavedSettings.declareRect("FloaterTalkToRect", LLRect(0, 130, 330, 0), "[NOT USED]");
	// Script error/debug dialog, force to top of screen
	gSavedSettings.declareRect("FloaterScriptDebugRect", LLRect(0, 130, 450, 0), "Rectangle for Script Error/Debug window");

	// HUD Console
	gSavedSettings.declareS32("ConsoleBufferSize",	40, "Size of chat console history (lines of chat)");

	//gSavedSettings.declareString("UIImgCompassTextureUUID",		"79156764-de98-4815-9d50-b10a7646bcf4", "[NOT USED]");

	// Script Panel
	//gSavedSettings.declareRect("ScriptPanelRect",  LLRect(250,  175 + 400,  250 + 400, 175), "[NOT USED]");

	// volume floater
	gSavedSettings.declareRect("FloaterAudioVolumeRect", LLRect(0, 440, 470, 0), "Rectangle for Audio Volume window");
	
	// Radio button sets
	gSavedSettings.declareU32("AvatarSex", 0, "", NO_PERSIST);

	// Radio button sets
	gSavedSettings.declareS32("RadioLandBrushAction", 0, "Last selected land modification operation (0 = flatten, 1 = raise, 2 = lower, 3 = smooth, 4 = roughen, 5 = revert)");
	gSavedSettings.declareS32("RadioLandBrushSize", 0, "Size of land modification brush (0 = small, 1 = medium, 2 = large)");
	gSavedSettings.declareF32("LandBrushForce", 1.0, "Multiplier for land modification brush force");

	// Build Options Panel
	gSavedSettings.declareBOOL("SnapEnabled", TRUE, "Enable snapping to grid");
	gSavedSettings.declareBOOL("SnapToMouseCursor", FALSE, "When snapping to grid, center object on nearest grid point to mouse cursor");
	gSavedSettings.declareF32 ("GridResolution", 0.5f, "Size of single grid step (meters)");
	gSavedSettings.declareF32 ("GridDrawSize", 12.0f, "Visible extent of 2D snap grid (meters)");
	gSavedSettings.declareBOOL("GridSubUnit", FALSE, "Display fractional grid steps, relative to grid size");
	gSavedSettings.declareF32("GridOpacity", 0.7f, "Grid line opacity (0.0 = completely transparent, 1.0 = completely opaque)");
	gSavedSettings.declareBOOL("GridCrossSections", FALSE, "Highlight cross sections of prims with grid manipulation plane.");
	
	gSavedSettings.declareS32("GridMode", 0, "Snap grid reference frame (0 = world, 1 = local, 2 = reference object)");
	//gSavedSettings.declareBOOL("GridIsLocal", FALSE, "[NOT USED]");
	gSavedSettings.declareS32("GridSubdivision", 32, "Maximum number of times to divide single snap grid unit when GridSubUnit is true");
	gSavedSettings.declareF32 ("RotationStep", 1.0f, "All rotations via rotation tool are constrained to multiples of this unit (degrees)");

	// Saved state for window
	gSavedSettings.declareBOOL("WindowMaximized", TRUE, "SL viewer window maximized on login");
	gSavedSettings.declareS32("WindowHeight", WINDOW_HEIGHT, "SL viewer window height");
	gSavedSettings.declareS32("WindowWidth", WINDOW_WIDTH, "SL viewer window width");
	gSavedSettings.declareS32("WindowX", 10, "X coordinate of lower left corner of SL viewer window, relative to primary display (pixels)");
	gSavedSettings.declareS32("WindowY", 10, "Y coordinate of lower left corner of SL viewer window, relative to primary display (pixels)");

	// Fullscreen menu options
	gSavedSettings.declareBOOL("FullScreen", FALSE, "Run SL in fullscreen mode");
//#if LL_DARWIN
//	gSavedSettings.declareBOOL("FullScreen", FALSE);
//#else
//	gSavedSettings.declareBOOL("FullScreen", TRUE);
//#endif

	// Fullscreen actual settings
	gSavedSettings.declareS32("FullScreenWidth", 1024, "Fullscreen resolution in width");
	gSavedSettings.declareS32("FullScreenHeight", 600, "Fullscreen resolution in height");
	gSavedSettings.declareF32("FullScreenAspectRatio", 1.7066f, "Aspect ratio of fullscreen display (width / height)");
	gSavedSettings.declareBOOL("FullScreenAutoDetectAspectRatio", TRUE, "Automatically detect proper aspect ratio for fullscreen display");
	
	// UI general settigns
	gSavedSettings.declareBOOL("TabToTextFieldsOnly", FALSE, "TAB key takes you to next text entry field, instead of next widget");
	gSavedSettings.declareF32("UIScaleFactor", 1.f, "Size of UI relative to default layout on 1024x768 screen");
	gSavedSettings.declareBOOL("UIAutoScale", TRUE, "Keep UI scale consistent across different resolutions");
	
	// Login
	gSavedSettings.declareString("FirstName", "", "Login first name");
	gSavedSettings.declareString("LastName", "", "Login last name");
    gSavedPerAccountSettings.declareU32("LastLogoff", 0, "Last logoff");

	// Legacy password storage.  Now stored in separate file.
	gSavedSettings.declareString("Marker", "", "[NOT USED]");

	gSavedSettings.declareBOOL("RememberPassword", TRUE, "Keep password (in encrypted form) for next login");
	gSavedSettings.declareBOOL("LoginLastLocation", TRUE, "Login at same location you last logged out");
	gSavedSettings.declareBOOL("ShowStartLocation", TRUE, "Display starting location menu on login screen");
	gSavedSettings.declareBOOL("FlyingAtExit", FALSE, "Was flying when last logged out, so fly when logging in");
	gSavedSettings.declareBOOL("ForceShowGrid", TRUE, "Always show grid dropdown on login screen");

//	gSavedSettings.declareString("AvatarTexture", "be20de2d-7812-4e0e-80f2-33aadf185a9f");
	gSavedSettings.declareU32("RegionTextureSize", 256, "Terrain texture dimensions (power of 2)");

	// Selection option
	gSavedSettings.declareBOOL("EditLinkedParts", FALSE, "Select individual parts of linked objects", NO_PERSIST);

	// Selection beam
	gSavedSettings.declareBOOL("ShowSelectionBeam", TRUE, "Show selection particle beam when selecting or interacting with objects.");
	
	// Scale manipulator
	gSavedSettings.declareBOOL("ScaleUniform", FALSE, "Scale selected objects evenly about center of selection");
	gSavedSettings.declareBOOL("ScaleShowAxes", FALSE, "Show indicator of selected scale axis when scaling");
	gSavedSettings.declareBOOL("ScaleStretchTextures", TRUE, "Stretch textures along with object when scaling");

	//------------------------------------------------------------------------
	// Help viewer
	//------------------------------------------------------------------------
	gSavedSettings.declareString("HelpHomeURL", "help/index.html", "URL of initial help page");
	gSavedSettings.declareString("HelpLastVisitedURL", "help/index.html", "URL of last help page, will be shown next time help is accessed");

	// HTML dialog (general purpose)
	gSavedSettings.declareRect("HtmlFloaterRect", LLRect(100,460,370,100), "Rectangle for HTML Floater window");

	// HTML sim release message floater
	gSavedSettings.declareRect("HtmlReleaseMessage", LLRect(46,520,400,128), "Rectangle for HTML Release Message Floater window");
	
	
	// HTML help 
	gSavedSettings.declareString("HtmlHelpLastPage", "", "Last URL visited via help system");
	gSavedSettings.declareRect("HtmlHelpRect", LLRect(16,650,600,128), "Rectangle for HTML help window");
	gSavedSettings.declareRect("HtmlFindRect", LLRect(16,650,600,128), "Rectangle for HTML find window");

	// Audio
	gSavedSettings.declareBOOL("ShowVolumeSettingsPopup", FALSE, "Show individual volume slider for voice, sound effects, etc");
	gSavedSettings.declareF32("AudioLevelMaster", 1.0f, "Master audio level, or overall volume");
	gSavedSettings.declareF32("AudioLevelSFX", 	  1.0f, "Audio level of in-world sound effects");
	gSavedSettings.declareF32("AudioLevelAmbient",0.5f, "Audio level of environment sounds");
	gSavedSettings.declareF32("AudioLevelUI", 	 0.5f, "Audio level of UI sound effects");
	gSavedSettings.declareF32("AudioLevelMusic", 1.0f, "Audio level of streaming music");
	gSavedSettings.declareF32("AudioLevelVoice", 1.0f, "Audio level of voice chat");
	gSavedSettings.declareF32("AudioLevelMedia", 1.0f, "Audio level of Quicktime movies");
	gSavedSettings.declareF32("AudioLevelMic", 1.0f, "Audio level of microphone input");

// 	gSavedSettings.declareF32("MediaAudioVolume", 1.0f, "Audio level of Quicktime movies"); // removed

	gSavedSettings.declareF32("AudioLevelDistance", 1.0f, "Scale factor for audio engine (multiple of world scale, 2.0 = audio falls off twice as fast)");
	gSavedSettings.declareF32("AudioLevelDoppler", 1.0f, "Scale of doppler effect on moving audio sources (1.0 = normal, <1.0 = diminished doppler effect, >1.0 = enhanced doppler effect)");
	gSavedSettings.declareF32("AudioLevelRolloff", 1.0f, "Controls the distance-based dropoff of audio volume (fraction or multiple of default audio rolloff)");

	gSavedSettings.declareBOOL("AudioStreamingMusic", FALSE, "Enable streaming audio");
	gSavedSettings.declareBOOL("AudioStreamingVideo", FALSE, "Enable streaming video");

	//UI Sounds

	gSavedSettings.declareBOOL("UISndDebugSpamToggle", FALSE, "Log UI sound effects as they are played");

	gSavedSettings.declareF32("UISndHealthReductionThreshold", 10.f, "Amount of health reduction required to trigger \"pain\" sound");
	gSavedSettings.declareF32("UISndMoneyChangeThreshold", 50.f, "Amount of change in L$ balance required to trigger \"money\" sound");

	gSavedSettings.declareString("UISndAlert",		        "ed124764-705d-d497-167a-182cd9fa2e6c", "Sound file for alerts (uuid for sound asset)");
	//gSavedSettings.declareString("UISndAppearanceAnimate",	"6cf2be26-90cb-2669-a599-f5ab7698225f", "[NOT USED]");
	gSavedSettings.declareString("UISndBadKeystroke",       "2ca849ba-2885-4bc3-90ef-d4987a5b983a", "Sound file for invalid keystroke (uuid for sound asset)");
	//gSavedSettings.declareString("UISndChatFromObject", 	"00000000-0000-0000-0000-000000000000", "[NOT USED]");
	gSavedSettings.declareString("UISndClick",		        "4c8c3c77-de8d-bde2-b9b8-32635e0fd4a6", "Sound file for mouse click (uuid for sound asset)");
	gSavedSettings.declareString("UISndClickRelease",       "4c8c3c77-de8d-bde2-b9b8-32635e0fd4a6", "Sound file for mouse button release (uuid for sound asset)");
//	gSavedSettings.declareString("UISndError",		        "cb58f920-5b52-8a49-b81c-e532adbbe6f1", "Sound file for UI error (uuid for sound asset)");
	gSavedSettings.declareString("UISndHealthReductionF",   "219c5d93-6c09-31c5-fb3f-c5fe7495c115", "Sound file for female pain (uuid for sound asset)");
	gSavedSettings.declareString("UISndHealthReductionM",   "e057c244-5768-1056-c37e-1537454eeb62", "Sound file for male pain (uuid for sound asset)");
	//gSavedSettings.declareString("UISndIncomingChat",		"00000000-0000-0000-0000-000000000000", "[NOT USED]");
	//gSavedSettings.declareString("UISndIncomingIM",		    "00000000-0000-0000-0000-000000000000", "[NOT USED]");
	//gSavedSettings.declareString("UISndInvApplyToObject", 	"00000000-0000-0000-0000-000000000000", "[NOT USED]");
	gSavedSettings.declareString("UISndInvalidOp",	        "4174f859-0d3d-c517-c424-72923dc21f65", "Sound file for invalid operations (uuid for sound asset)");
	//gSavedSettings.declareString("UISndInventoryCopyToInv",	"00000000-0000-0000-0000-000000000000", "[NOT USED]");
	gSavedSettings.declareString("UISndMoneyChangeDown",  	"104974e3-dfda-428b-99ee-b0d4e748d3a3", "Sound file for L$ balance increase (uuid for sound asset)");
	gSavedSettings.declareString("UISndMoneyChangeUp",  	"77a018af-098e-c037-51a6-178f05877c6f", "Sound file for L$ balance decrease(uuid for sound asset)");
	gSavedSettings.declareString("UISndNewIncomingIMSession",     "67cc2844-00f3-2b3c-b991-6418d01e1bb7", "Sound file for new instant message session(uuid for sound asset)");
	//gSavedSettings.declareString("UISndObjectCopyToInv",	"00000000-0000-0000-0000-000000000000", "[NOT USED]");
	gSavedSettings.declareString("UISndObjectCreate",	  	"f4a0660f-5446-dea2-80b7-6482a082803c", "Sound file for object creation (uuid for sound asset)");
	gSavedSettings.declareString("UISndObjectDelete",	  	"0cb7b00a-4c10-6948-84de-a93c09af2ba9", "Sound file for object deletion (uuid for sound asset)");
	gSavedSettings.declareString("UISndObjectRezIn",	  	"3c8fc726-1fd6-862d-fa01-16c5b2568db6", "Sound file for rezzing objects (uuid for sound asset)");
	gSavedSettings.declareString("UISndObjectRezOut",	  	"00000000-0000-0000-0000-000000000000", "Sound file for derezzing objects (uuid for sound asset)");
	gSavedSettings.declareString("UISndPieMenuAppear",   	"8eaed61f-92ff-6485-de83-4dcc938a478e", "Sound file for opening pie menu (uuid for sound asset)");
	gSavedSettings.declareString("UISndPieMenuHide",   	    "00000000-0000-0000-0000-000000000000", "Sound file for closing pie menu (uuid for sound asset)");
	gSavedSettings.declareString("UISndPieMenuSliceHighlight0", 	"d9f73cf8-17b4-6f7a-1565-7951226c305d", "Sound file for selecting pie menu item 0 (uuid for sound asset)");
	gSavedSettings.declareString("UISndPieMenuSliceHighlight1", 	"f6ba9816-dcaf-f755-7b67-51b31b6233e5", "Sound file for selecting pie menu item 1 (uuid for sound asset)");
	gSavedSettings.declareString("UISndPieMenuSliceHighlight2", 	"7aff2265-d05b-8b72-63c7-dbf96dc2f21f", "Sound file for selecting pie menu item 2 (uuid for sound asset)");
	gSavedSettings.declareString("UISndPieMenuSliceHighlight3", 	"09b2184e-8601-44e2-afbb-ce37434b8ba1", "Sound file for selecting pie menu item 3 (uuid for sound asset)");
	gSavedSettings.declareString("UISndPieMenuSliceHighlight4", 	"bbe4c7fc-7044-b05e-7b89-36924a67593c", "Sound file for selecting pie menu item 4 (uuid for sound asset)");
	gSavedSettings.declareString("UISndPieMenuSliceHighlight5", 	"d166039b-b4f5-c2ec-4911-c85c727b016c", "Sound file for selecting pie menu item 5 (uuid for sound asset)");
	gSavedSettings.declareString("UISndPieMenuSliceHighlight6", 	"242af82b-43c2-9a3b-e108-3b0c7e384981", "Sound file for selecting pie menu item 6 (uuid for sound asset)");
	gSavedSettings.declareString("UISndPieMenuSliceHighlight7", 	"c1f334fb-a5be-8fe7-22b3-29631c21cf0b", "Sound file for selecting pie menu item 7 (uuid for sound asset)");
	gSavedSettings.declareString("UISndSnapshot",	  	    "3d09f582-3851-c0e0-f5ba-277ac5c73fb4", "Sound file for taking a snapshot (uuid for sound asset)");
	//gSavedSettings.declareString("UISndStartAutopilot", 	"00000000-0000-0000-0000-000000000000", "[NOT USED]");
	//gSavedSettings.declareString("UISndStartFollowpilot", 	"00000000-0000-0000-0000-000000000000", "[NOT USED]");
	gSavedSettings.declareString("UISndStartIM",		    "c825dfbc-9827-7e02-6507-3713d18916c1", "Sound file for starting a new IM session (uuid for sound asset)");
	//gSavedSettings.declareString("UISndStopAutopilot",   	"00000000-0000-0000-0000-000000000000", "[NOT USED]");
	gSavedSettings.declareString("UISndTeleportOut", 		"d7a9a565-a013-2a69-797d-5332baa1a947", "Sound file for teleporting (uuid for sound asset)");
	//gSavedSettings.declareString("UISndTextureApplyToObject",	"00000000-0000-0000-0000-000000000000", "[NOT USED]");
	//gSavedSettings.declareString("UISndTextureCopyToInv", 	"00000000-0000-0000-0000-000000000000", "[NOT USED]");
	gSavedSettings.declareString("UISndTyping", 	        "5e191c7b-8996-9ced-a177-b2ac32bfea06", "Sound file for starting to type a chat message (uuid for sound asset)");
//	gSavedSettings.declareString("UISndWarning",	        "449bc80c-91b6-6365-8fd1-95bd91016624", "Sound file for alerts (uuid for sound asset)");
	gSavedSettings.declareString("UISndWindowClose",  	    "2c346eda-b60c-ab33-1119-b8941916a499", "Sound file for closing a window (uuid for sound asset)");
	gSavedSettings.declareString("UISndWindowOpen",	  	    "c80260ba-41fd-8a46-768a-6bf236360e3a", "Sound file for opening a window (uuid for sound asset)");

	// Sky params
	gSavedSettings.declareBOOL("SkyOverrideSimSunPosition", FALSE, "", NO_PERSIST);
	gSavedSettings.declareVec3("SkySunDefaultPosition", LLVector3(1.f, 0.f, 0.1f), "Default position of sun in sky (direction in world coordinates)");
	gSavedSettings.declareF32("SkyAmbientScale", 0.3f, "Controls strength of ambient, or non-directional light from the sun and moon (fraction or multiple of default ambient level)");
	gSavedSettings.declareColor3("SkyNightColorShift", LLColor3(0.7f, 0.7f, 1.0f), "Controls moonlight color (base color applied to moon as light source)");
	gSavedSettings.declareBOOL("FixedWeather", FALSE, "Weather effects do not change over time");

	// Cache Stuff
	gSavedSettings.declareU32("VFSSalt", 1, "[DO NOT MODIFY] Controls local file caching behavior");
	gSavedSettings.declareU32("VFSOldSize", 0, "[DO NOT MODIFY] Controls resizing of local file cache");
// 	gSavedSettings.declareU32("VFSSize", 2, "Controls amount of hard drive space reserved for local file caching (0 = 50MB, 1 = 200MB, 2 = 500MB, 3 = 1000MB)");
 	gSavedSettings.declareU32("CacheSize", 500, "Controls amount of hard drive space reserved for local file caching in MB");
 	gSavedSettings.declareString("CacheLocation", "", "Controls the location of the local disk cache");
 	gSavedSettings.declareString("NewCacheLocation", "", "Change the location of the local disk cache to this");
 	gSavedSettings.declareU32("CacheValidateCounter", 0, "Used to distribute cache validation");
	// Delete all files in cache directory on startup
	gSavedSettings.declareBOOL("PurgeCacheOnStartup", FALSE, "Clear local file cache every time viewer is run");
	gSavedSettings.declareBOOL("PurgeCacheOnNextStartup", FALSE, "Clear local file cache next time viewer is run");

	// Used for special titles such as "Second Life - Special E3 2003 Beta"
	gSavedSettings.declareBOOL("ShowOverlayTitle", FALSE, "Prints watermark text message on screen");
	gSavedSettings.declareString("OverlayTitle", "Set_via_OverlayTitle_in_settings.xml", 
		"Controls watermark text message displayed on screen when \"ShowOverlayTitle\" is enabled (one word, underscores become spaces)");  // Must be one word, but underscores are replaced by spaces. Hah!

	// Secret debug stuff.
	gSavedSettings.declareBOOL("UseDebugMenus", FALSE, "Turns on \"Debug\" menu");
	gSavedSettings.declareS32("ServerChoice", 0, "[DO NOT MODIFY] Controls which grid you connect to");
	gSavedSettings.declareString("CustomServer", "", "Specifies IP address or hostname of grid to which you connect");
	gSavedSettings.declareBOOL("UseDebugLogin", FALSE, "Provides extra control over which grid to connect to");

	// First run is true on the first startup of a given installation.
	// It is not related to whether your ACCOUNT has been logged in before.
	// Set to false if you reach the login screen.
	gSavedSettings.declareBOOL("FirstRunThisInstall", TRUE, "Specifies that you have not run the viewer since you installed the latest update");

	// Is this the first successful login for a given installation?
	// It is not related to whether your ACCOUNT has been logged in before.
	// Set to false if you successfully connect.
	gSavedSettings.declareBOOL("FirstLoginThisInstall", TRUE, "Specifies that you have not successfully logged in since you installed the latest update");

	// The last version that was run with this prefs file.  Default to a version that will never be current,
	// and update after the setting is used in the startup sequence.
	gSavedSettings.declareString("LastRunVersion", "0.0.0", "Version number of last instance of the viewer that you ran");
	// Local cache version (change if format changes)
	gSavedSettings.declareS32("LocalCacheVersion", 0, "Version number of cache");

	// cached mean collision values
	gSavedSettings.declareBOOL("MeanCollisionBump", FALSE, "You have experienced an abuse of being bumped by an object or avatar" );
	gSavedSettings.declareBOOL("MeanCollisionPushObject", FALSE, "You have experienced an abuse of being pushed by a scripted object");
	gSavedSettings.declareBOOL("MeanCollisionSelected", FALSE, "You have experienced an abuse of being pushed via a selected object");
	gSavedSettings.declareBOOL("MeanCollisionScripted", FALSE, "You have experienced an abuse from a scripted object");
	gSavedSettings.declareBOOL("MeanCollisionPhysical", FALSE, "You have experienced an abuse from a physical object");

	// Does left-click show menu, or only do grabbing?
	gSavedSettings.declareBOOL("LeftClickShowMenu", FALSE, "Left click opens pie menu (FALSE = left click touches or grabs object)");

	gSavedSettings.declareF32("MouseSensitivity", 3.f, "Controls responsiveness of mouse when in mouselook mode (fraction or multiple of default mouse sensitivity)");
	gSavedSettings.declareBOOL("MouseSmooth", FALSE, "Smooths out motion of mouse when in mouselook mode.");
	gSavedSettings.declareBOOL("InvertMouse", FALSE, "When in mouselook, moving mouse up looks down and vice verse (FALSE = moving up looks up)");

	gSavedSettings.declareBOOL("EditCameraMovement", FALSE, "When entering build mode, camera moves up above avatar");
	gSavedSettings.declareBOOL("AppearanceCameraMovement", TRUE, "When entering appearance editing mode, camera zooms in on currently selected portion of avatar");

	gSavedSettings.declareBOOL("AltShowsPhysical", FALSE, "When ALT key is held down, physical objects are rendered in red.");
	gSavedSettings.declareBOOL("BeaconAlwaysOn", FALSE, "Beacons / highlighting always on");
	gSavedSettings.declareBOOL("scriptsbeacon", FALSE, "Beacon / Highlight scripted objects");
	gSavedSettings.declareBOOL("physicalbeacon", TRUE, "Beacon / Highlight physical objects");
	gSavedSettings.declareBOOL("soundsbeacon", FALSE, "Beacon / Highlight sound generators");
	gSavedSettings.declareBOOL("particlesbeacon", FALSE, "Beacon / Highlight particle generators");
	gSavedSettings.declareBOOL("scripttouchbeacon", TRUE, "Beacon / Highlight scripted objects with touch function");
	gSavedSettings.declareBOOL("renderbeacons", FALSE, "Beacon / Highlight particle generators");
	gSavedSettings.declareBOOL("renderhighlights", TRUE, "Beacon / Highlight scripted objects with touch function");

	gSavedSettings.declareBOOL("MuteAudio", FALSE, "All audio plays at 0 volume (streaming audio still takes up bandwidth, for example)");
	gSavedSettings.declareBOOL("MuteWhenMinimized", TRUE, "Mute audio when SL window is minimized");

	gSavedSettings.declareBOOL("MuteMusic", FALSE, "Music plays at 0 volume (streaming audio still takes up bandwidth)");
	gSavedSettings.declareBOOL("MuteMedia", FALSE, "Media plays at 0 volume (streaming audio still takes up bandwidth)");
	gSavedSettings.declareBOOL("MuteVoice", FALSE, "Voice plays at 0 volume (streaming audio still takes up bandwidth)");
	gSavedSettings.declareBOOL("MuteSounds", FALSE, "Sound effects play at 0 volume");
	gSavedSettings.declareBOOL("MuteAmbient", FALSE, "Ambient sound effects, such as wind noise, play at 0 volume");
	gSavedSettings.declareBOOL("MuteUI", FALSE, "UI sound effects play at 0 volume");

	gSavedSettings.declareS32("NotifyBoxWidth", 350, "Width of notification messages");
	gSavedSettings.declareS32("NotifyBoxHeight", 200, "Height of notification messages");

	gSavedSettings.declareS32("GroupNotifyBoxWidth", 400, "Width of group notice messages");
	gSavedSettings.declareS32("GroupNotifyBoxHeight", 260, "Height of group notice messages");

	// Time in seconds.
	gSavedSettings.declareF32("NotifyTipDuration", 4.f, "Length of time that notification tips stay on screen (seconds)");

	gSavedSettings.declareBOOL("NotifyMoneyChange", TRUE, "Pop up notifications for all L$ transactions");

	gSavedSettings.declareBOOL("ShowNewInventory", TRUE,
		"Automatically views new notecards/textures/landmarks");
	gSavedSettings.declareBOOL("AutoAcceptNewInventory", FALSE,
		"Automatically accept new notecards/textures/landmarks");

	// Bitfield
	// 1 = by date
	// 2 = folders always by name
	// 4 = system folders to top
	// This where the default first time user gets his settings.
	gSavedSettings.declareU32("InventorySortOrder", 1 | 2 | 4, "Specifies sort key for inventory items (+0 = name, +1 = date, +2 = folders always by name, +4 = system folders to top)");
	gSavedSettings.declareU32("RecentItemsSortOrder", 1, "Specifies sort key for recent inventory items (+0 = name, +1 = date, +2 = folders always by name, +4 = system folders to top)");
	gSavedSettings.declareU32("TexturePickerSortOrder", 2, "Specifies sort key for textures in texture picker (+0 = name, +1 = date, +2 = folders always by name, +4 = system folders to top)");
	gSavedSettings.declareU32("AvatarPickerSortOrder", 2, "Specifies sort key for textures in avatar picker (+0 = name, +1 = date, +2 = folders always by name, +4 = system folders to top)");

	// Pixels away from edge that windows snap.
	gSavedSettings.declareS32("SnapMargin", 10, "Controls maximum distance between windows before they auto-snap together (pixels)");

	// Will be set on first run
	gSavedSettings.declareS32("FloaterViewBottom", -1, "[DO NOT MODIFY] Controls layout of floating windows within SL window");

	// Automatically fly when up key held down, and automatically stop
	// flying when landing on something.
	gSavedSettings.declareBOOL("AutomaticFly", TRUE, "Fly by holding jump key or using \"Fly\" command (FALSE = fly by using \"Fly\" command only)");

	// Index of the last find panel you opened.
	gSavedSettings.declareString("LastFindPanel", "find_all_panel", "Controls which find operation appears by default when clicking \"Find\" button ");

	// grab keystrokes at last possible moment to minimize latency
	gSavedSettings.declareBOOL("AsyncKeyboard", TRUE, "Improves responsiveness to keyboard input when at low framerates");

	// Numpad numbers move avatar even when numlock is off/you're using a Mac?
	gSavedSettings.declareS32("NumpadControl", 0, "How numpad keys control your avatar. 0 = Like the normal arrow keys, 1 = Numpad moves avatar when numlock is off, 2 = Numpad moves avatar regardless of numlock (use this if you have no numlock)");

	// 1.2.9: For transition from 1.2.8 to 1.2.9, need to ask people
	// this question regardless of number of executions.
	// 1.6.10: We're just defaulting crash reporting to on
	// 1.7.x: We ask at crash time, but leave this so when you flip back and
	// forth between 1.6 and 1.7 it doesn't ask you every time.
	gSavedSettings.declareBOOL("AskedAboutCrashReports", TRUE, "Turns off dialog asking if you want to enable crash reporting");

	// Default for the "Online" checkbox in Find -> People
	gSavedSettings.declareBOOL("FindPeopleOnline", TRUE, "Limits people search to only users who are logged on");

	// Default for checkboxes in Find -> Land
	//gSavedSettings.declareBOOL("FindLandForSale", TRUE);
	//gSavedSettings.declareBOOL("FindLandAuction", TRUE);

	// Default for Find -> Land combo box
	gSavedSettings.declareString("FindLandType", "All", "Controls which type of land you are searching for in Find Land interface (\"All\", \"Auction\", \"For Sale\")");

	gSavedSettings.declareBOOL("FindLandPrice", TRUE, "Enables filtering of land search results by price");
	gSavedSettings.declareBOOL("FindLandArea", FALSE, "Enables filtering of land search results by area");

	// Checkboxes in Find -> Popular
	// Should these all be the same?  I imagine we might want a single "show mature." - bbc
	gSavedSettings.declareU32("PreferredMaturity", 13, "Setting for the user's preferred maturity level");
	gSavedSettings.declareBOOL("ShowPGSearchAll", TRUE, "Display results of search All that are flagged as PG");
	gSavedSettings.declareBOOL("ShowMatureSearchAll", FALSE, "Display results of search All that are flagged as mature");
	gSavedSettings.declareBOOL("ShowAdultSearchAll", FALSE, "Display results of search All that are flagged as adult");
	gSavedSettings.declareBOOL("ShowPGSims", TRUE, "Display results of find places or find popular that are in PG sims");
	gSavedSettings.declareBOOL("ShowMatureSims", FALSE, "Display results of find places or find popular that are in mature sims");
	gSavedSettings.declareBOOL("ShowAdultSims", FALSE, "Display results of find places or find popular that are in adult sims");
	gSavedSettings.declareBOOL("ShowPGEvents", TRUE, "Display results of find events that are flagged as PG");
	gSavedSettings.declareBOOL("ShowMatureEvents", FALSE, "Display results of find events that are flagged as mature");
	gSavedSettings.declareBOOL("ShowAdultEvents", FALSE, "Display results of find events that are flagged as adult");
	gSavedSettings.declareBOOL("ShowPGClassifieds", TRUE, "Display results of find classifieds that are flagged as PG");
	gSavedSettings.declareBOOL("ShowMatureClassifieds", FALSE, "Display results of find classifieds that are flagged as mature");
	gSavedSettings.declareBOOL("ShowAdultClassifieds", FALSE, "Display results of find classifieds that are flagged as adult");
	gSavedSettings.declareBOOL("ShowPGGroups", TRUE, "Display results of find groups that are in flagged as PG");
	gSavedSettings.declareBOOL("ShowMatureGroups", FALSE, "Display results of find groups that are in flagged as mature");
	gSavedSettings.declareBOOL("ShowAdultGroups", FALSE, "Display results of find groups that are in flagged as adult");
	gSavedSettings.declareBOOL("ShowPGLand", TRUE, "Display results of find land sales that are flagged as PG");
	gSavedSettings.declareBOOL("ShowMatureLand", TRUE, "Display results of find land sales that are flagged as mature");
	gSavedSettings.declareBOOL("ShowAdultLand", TRUE, "Display results of find land sales that are flagged as adult");
	
	gSavedSettings.declareBOOL("FindPlacesPictures", TRUE, "Display only results of find places that have pictures");

	gSavedSettings.declareBOOL("MapShowPGEvents", TRUE, "Show PG events on world map");
	gSavedSettings.declareBOOL("MapShowMatureEvents", FALSE, "Show mature events on world map");
	gSavedSettings.declareBOOL("MapShowAdultEvents", FALSE, "Show adult events on world map");
	gSavedSettings.declareBOOL("MapShowLandForSale", FALSE, "Show land for sale on world map");
	gSavedSettings.declareBOOL("MapShowTelehubs", TRUE, "Show telehubs on world map");
	gSavedSettings.declareBOOL("MapShowPeople", TRUE, "Show other users on world map");
	gSavedSettings.declareBOOL("MapShowInfohubs", TRUE, "Show infohubs on the world map");

	// Search panel in directory uses this URL for queries
	gSavedSettings.declareString("SearchURLDefault",
		"http://secondlife.com/app/search/index.php?",
		"URL to load for empty searches");
	gSavedSettings.declareString("SearchURLQuery",
		"http://secondlife.com/app/search/search_proxy.php?q=[QUERY]&s=[COLLECTION]&",
		"URL to use for searches");
	// Version 2 added [SESSION], must invalidate old saved settings.
	gSavedSettings.declareString("SearchURLSuffix2",
		"mat=[MATURITY]&t=[TEEN]&region=[REGION]&x=[X]&y=[Y]&z=[Z]&session=[SESSION]",
		"Parameters added to end of search queries");

	// Arrow keys move avatar while in chat?
	gSavedSettings.declareBOOL("ArrowKeysMoveAvatar", TRUE, "While cursor is in chat entry box, arrow keys still control your avatar");
	gSavedSettings.declareBOOL("ChatBarStealsFocus", TRUE, "Whenever keyboard focus is removed from the UI, and the chat bar is visible, the chat bar takes focus");

	// Show yellow selection fence in snapshots for auctions?
	gSavedSettings.declareBOOL("AuctionShowFence", TRUE, "When auctioning land, include parcel boundary marker in snapshot");

	// Use DX9 to probe hardware on startup.  Only do this once,
	// because it's slow.
	gSavedSettings.declareBOOL("ProbeHardwareOnStartup", TRUE, "Query current hardware configuration on application startup");

	// enable/disable system color picker
	gSavedSettings.declareBOOL("UseDefaultColorPicker", FALSE, "Use color picker supplied by operating system");
	gSavedSettings.declareF32("PickerContextOpacity", 0.35f, "Controls overall opacity of context frustrum connecting color and texture pickers with their swatches");

	gSavedSettings.declareF32("ColumnHeaderDropDownDelay", 0.3f, "Time in seconds of mouse click before column header shows sort options list");
	// support for avatar exporter
	//gSavedSettings.declareString("AvExportPath", "", "[NOT USED]");
	//gSavedSettings.declareString("AvExportBaseName", "", "[NOT USED]");

	// Show in-world hover tips for objects
	gSavedSettings.declareBOOL("ShowHoverTips", TRUE, "Show descriptive tooltip when mouse hovers over items in world");
	gSavedSettings.declareBOOL("ShowLandHoverTip", FALSE, "Show descriptive tooltip when mouse hovers over land");
	gSavedSettings.declareBOOL("ShowAllObjectHoverTip", FALSE, "Show descriptive tooltip when mouse hovers over non-interactive and interactive objects.");

	// Use an external web browser (Firefox, Internet Explorer)
	// CP: making this TRUE by default since there is no internal Web browser
	//	   now and other components may interrogate this setting
	gSavedSettings.declareBOOL("UseExternalBrowser", TRUE, "[NOT USED]");
	gSavedSettings.declareBOOL("CookiesEnabled", TRUE, "Accept cookies from Web sites?");

	// browser home page
	gSavedSettings.declareString("BrowserHomePage", "http://my.opera.com/boylane", "[NOT USED]");

	// browser proxy variables
	gSavedSettings.declareBOOL("BrowserProxyEnabled", FALSE, "Use Web Proxy");
	gSavedSettings.declareString("BrowserProxyAddress", "", "Address for the Web Proxy]");
	gSavedSettings.declareS32("BrowserProxyPort", 3128, "Port for Web Proxy");
	gSavedSettings.declareS32("BrowserProxySocks45", 5, "[NOT USED]");
	gSavedSettings.declareString("BrowserProxyExclusions", "", "[NOT USED]");

	// Allow user to completely disable web pages on prims
	gSavedSettings.declareBOOL("UseWebPagesOnPrims", FALSE, "[NOT USED]");

	// use object-object occlusion culling
	gSavedSettings.declareBOOL("UseOcclusion", TRUE, "Enable object culling based on occlusion (coverage) by other objects");

	gSavedSettings.declareBOOL("DoubleClickAutoPilot", FALSE, "Enable double-click auto pilot");
	gSavedSettings.declareBOOL("DoubleClickTeleport", FALSE, "Enable double-click teleport");
	
	//cheesy beacon effects
	gSavedSettings.declareBOOL("CheesyBeacon", FALSE, "Enable cheesy beacon effects");

	//flycam controls and joystick mapping
	gSavedSettings.declareS32("FlycamAxis0", 0, "Flycam hardware axis mapping for internal axis 0 ([0, 5]).");
	gSavedSettings.declareS32("FlycamAxis1", 1, "Flycam hardware axis mapping for internal axis 1 ([0, 5]).");
	gSavedSettings.declareS32("FlycamAxis2", 2, "Flycam hardware axis mapping for internal axis 2 ([0, 5]).");
	gSavedSettings.declareS32("FlycamAxis3", 3, "Flycam hardware axis mapping for internal axis 3 ([0, 5]).");
	gSavedSettings.declareS32("FlycamAxis4", 4, "Flycam hardware axis mapping for internal axis 4 ([0, 5]).");
	gSavedSettings.declareS32("FlycamAxis5", 5, "Flycam hardware axis mapping for internal axis 5 ([0, 5]).");
	gSavedSettings.declareS32("FlycamAxis6", -1, "Flycam hardware axis mapping for internal axis 6 ([0, 5]).");

	gSavedSettings.declareF32("FlycamAxisScale0", 1, "Flycam axis 0 scaler.");
	gSavedSettings.declareF32("FlycamAxisScale1", 1, "Flycam axis 1 scaler.");
	gSavedSettings.declareF32("FlycamAxisScale2", 1, "Flycam axis 2 scaler.");
	gSavedSettings.declareF32("FlycamAxisScale3", 1, "Flycam axis 3 scaler.");
	gSavedSettings.declareF32("FlycamAxisScale4", 1, "Flycam axis 4 scaler.");
	gSavedSettings.declareF32("FlycamAxisScale5", 1, "Flycam axis 5 scaler.");
	gSavedSettings.declareF32("FlycamAxisScale6", 1, "Flycam axis 6 scaler.");

	gSavedSettings.declareF32("FlycamAxisDeadZone0", 0.1f, "Flycam axis 0 dead zone.");
	gSavedSettings.declareF32("FlycamAxisDeadZone1", 0.1f, "Flycam axis 1 dead zone.");
	gSavedSettings.declareF32("FlycamAxisDeadZone2", 0.1f, "Flycam axis 2 dead zone.");
	gSavedSettings.declareF32("FlycamAxisDeadZone3", 0.1f, "Flycam axis 3 dead zone.");
	gSavedSettings.declareF32("FlycamAxisDeadZone4", 0.1f, "Flycam axis 4 dead zone.");
	gSavedSettings.declareF32("FlycamAxisDeadZone5", 0.1f, "Flycam axis 5 dead zone.");
	gSavedSettings.declareF32("FlycamAxisDeadZone6", 0.1f, "Flycam axis 6 dead zone.");

	gSavedSettings.declareF32("FlycamFeathering", 16.f, "Flycam feathering (less is softer)");
	gSavedSettings.declareBOOL("FlycamAutoLeveling", TRUE, "Keep Flycam level.");
	gSavedSettings.declareBOOL("FlycamAbsolute", FALSE, "Treat Flycam values as absolute positions (not deltas).");
	gSavedSettings.declareBOOL("FlycamZoomDirect", FALSE, "Map flycam zoom axis directly to camera zoom."); 

	// logitech LCD settings
	gSavedSettings.declareS32("LCDDestination", 0, "Which LCD to use");
	gSavedSettings.declareBOOL("DisplayChat", TRUE, "Display Latest Chat message on LCD");
	gSavedSettings.declareBOOL("DisplayIM", TRUE, "Display Latest IM message on LCD");
	gSavedSettings.declareBOOL("DisplayRegion", TRUE, "Display Location information on LCD");
	gSavedSettings.declareBOOL("DisplayDebug", TRUE, "Display Network Information on LCD");
	gSavedSettings.declareBOOL("DisplayDebugConsole", TRUE, "Display Console Debug Information on LCD");
	gSavedSettings.declareBOOL("DisplayLinden", TRUE, "Display Account Information on LCD");

	// Vector Processor & Math
	gSavedSettings.declareBOOL("VectorizePerfTest", TRUE, "Test SSE/vectorization performance and choose fastest version.");
	gSavedSettings.declareBOOL("VectorizeEnable", TRUE, "Enable general vector operations and data alignment.");
	gSavedSettings.declareBOOL("VectorizeSkin", TRUE, "Enable vector operations for avatar skinning.");
	gSavedSettings.declareU32( "VectorizeProcessor", 2, "0=Compiler Default, 1=SSE, 2=SSE2, autodetected", NO_PERSIST);

	//
	// crash_settings.xml
	//

	// Setting name is shared with win_crash_logger
	gCrashSettings.declareS32(CRASH_BEHAVIOR_SETTING, CRASH_BEHAVIOR_NEVER_SEND, "Controls behavior when viewer crashes "
		"(0 = ask before sending crash report, 1 = always send crash report, 2 = never send crash report)");

}


void fixup_settings()
{
#if LL_RELEASE_FOR_DOWNLOAD
	// Force some settings on startup
	gSavedSettings.setBOOL("AnimateTextures", TRUE); // Force AnimateTextures to always be on
#endif
	
	// Special code to tweak with defaults
	std::string last_major, last_minor, last_patch;
	S32 digit = gLastRunVersion.find_first_of("0123456789");
	S32 dot = gLastRunVersion.find_first_of('.', digit);
	if (dot != std::string::npos && digit != std::string::npos)
	{
		last_major = gLastRunVersion.substr(digit, dot-digit);
		digit = dot+1;
		dot = gLastRunVersion.find_first_of('.', digit);
	}
	if (dot != std::string::npos && digit != std::string::npos)
	{
		last_minor = gLastRunVersion.substr(digit, dot-digit);
		digit = dot+1;
		dot = gLastRunVersion.find_first_of('.', digit);
	}
	if (dot != std::string::npos && digit != std::string::npos)
	{
		last_patch = gLastRunVersion.substr(digit, dot-digit);
	}
	// 1.18.x -> 1.19.x
	if (last_major == "1" && last_minor == "18")
	{
		if (!gSavedSettings.hasLoaded("EnableVoiceChat"))
		{
			gSavedSettings.setBOOL("EnableVoiceChat", FALSE); // Default 1.18.x users to voice chat disabled
		}
	}

	gSavedSettings.clearLoaded();
}

////////////////////////////////////////////////////////////////////////////
// Listeners

class LLAFKTimeoutListener: public LLSimpleListener
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		gAFKTimeout = (F32) event->getValue().asReal();
		return true;
	}
};
static LLAFKTimeoutListener afk_timeout_listener;

class LLMouseSensitivityListener: public LLSimpleListener
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		gMouseSensitivity = (F32) event->getValue().asReal();
		return true;
	}
};
static LLMouseSensitivityListener mouse_sensitivity_listener;


class LLInvertMouseListener: public LLSimpleListener
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		gInvertMouse = event->getValue().asBoolean();
		return true;
	}
};
static LLInvertMouseListener invert_mouse_listener;

class LLRenderAvatarMouselookListener: public LLSimpleListener
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLVOAvatar::sVisibleInFirstPerson = event->getValue().asBoolean();
		return true;
	}
};
static LLRenderAvatarMouselookListener render_avatar_mouselook_listener;

class LLRenderFarClipListener: public LLSimpleListener
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		F32 draw_distance = (F32) event->getValue().asReal();
		gAgent.mDrawDistance = draw_distance;
		if (gWorldPointer)
		{
			gWorldPointer->setLandFarClip(draw_distance);
		}
		return true;
	}
};
static LLRenderFarClipListener render_far_clip_listener;

class LLTerrainDetailListener: public LLSimpleListener
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLDrawPoolTerrain::sDetailMode = event->getValue().asInteger();
		return true;
	}
};
static LLTerrainDetailListener terrain_detail_listener;


class LLSetShaderListener: public LLSimpleListener
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLShaderMgr::setShaders();
		return true;
	}
};
static LLSetShaderListener set_shader_listener;

class LLReleaseGLBufferListener: public LLSimpleListener
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		gPipeline.releaseGLBuffers();
		LLShaderMgr::setShaders();
		return true;
	}
};
static LLReleaseGLBufferListener release_gl_buffer_listener;

class LLVolumeLODListener: public LLSimpleListener
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLVOVolume::sLODFactor = (F32) event->getValue().asReal();
		LLVOVolume::sDistanceFactor = 1.f-LLVOVolume::sLODFactor * 0.1f;
		return true;
	}
};
static LLVolumeLODListener volume_lod_listener;

class LLAvatarLODListener: public LLSimpleListener
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLVOAvatar::sLODFactor = (F32) event->getValue().asReal();
		return true;
	}
};
static LLAvatarLODListener avatar_lod_listener;

class LLTreeLODListener: public LLSimpleListener
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLVOTree::sTreeFactor = (F32) event->getValue().asReal();
		return true;
	}
};
static LLTreeLODListener tree_lod_listener;

class LLFlexLODListener: public LLSimpleListener
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLVolumeImplFlexible::sUpdateFactor = (F32) event->getValue().asReal();
		return true;
	}
};
static LLFlexLODListener flex_lod_listener;

class LLGammaListener: public LLSimpleListener
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		F32 gamma = (F32) event->getValue().asReal();
		if (gamma == 0.0f)
		{
			gamma = 1.0f; // restore normal gamma
		}
		if (gamma != gViewerWindow->getWindow()->getGamma())
		{
			// Only save it if it's changed
			if (!gViewerWindow->getWindow()->setGamma(gamma))
			{
				llwarns << "setGamma failed!" << llendl;
			}
		}

		return true;
	}
};
static LLGammaListener gamma_listener;

class LLNightBrightnessListener: public LLSimpleListener
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLVOSky::sNighttimeBrightness = (F32) event->getValue().asReal();
		return true;
	}
};
static LLNightBrightnessListener night_brightness_listener;

const F32 MAX_USER_FOG_RATIO = 4.f;
const F32 MIN_USER_FOG_RATIO = 0.5f;

class LLFogRatioListener: public LLSimpleListener
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		F32 fog_ratio = llmax(MIN_USER_FOG_RATIO, 
							llmin((F32) event->getValue().asReal(), 
							MAX_USER_FOG_RATIO));
		gSky.setFogRatio(fog_ratio);
		return true;
	}
};
static LLFogRatioListener fog_ratio_listener;

class LLMaxPartCountListener: public LLSimpleListener
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLViewerPartSim::setMaxPartCount(event->getValue().asInteger());
		return true;
	}
};
static LLMaxPartCountListener max_partCount_listener;

const S32 MAX_USER_COMPOSITE_LIMIT = 100;
const S32 MIN_USER_COMPOSITE_LIMIT = 0;

class LLCompositeLimitListener: public LLSimpleListener
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		S32 composite_limit = llmax(MIN_USER_COMPOSITE_LIMIT, 
							llmin((S32)event->getValue().asInteger(), 
							MAX_USER_COMPOSITE_LIMIT));
		LLVOAvatar::sMaxOtherAvatarsToComposite = composite_limit;
		return true;
	}
};
static LLCompositeLimitListener composite_limit_listener;

class LLVideoMemoryListener: public LLSimpleListener
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		gImageList.updateMaxResidentTexMem(event->getValue().asInteger());
		return true;
	}
};
static LLVideoMemoryListener video_memory_listener;

class LLBandwidthListener: public LLSimpleListener
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		gViewerThrottle.setMaxBandwidth((F32) event->getValue().asReal());
		return true;
	}
};
static LLBandwidthListener bandwidth_listener;

class LLChatFontSizeListener: public LLSimpleListener
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		gConsole->setFontSize(event->getValue().asInteger());
		return true;
	}
};
static LLChatFontSizeListener chat_font_size_listener;

class LLChatPersistTimeListener: public LLSimpleListener
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		gConsole->setLinePersistTime((F32) event->getValue().asReal());
		return true;
	}
};
static LLChatPersistTimeListener chat_persist_time_listener;

class LLConsoleMaxLinesListener: public LLSimpleListener
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		gConsole->setMaxLines(event->getValue().asInteger());
		return true;
	}
};
static LLConsoleMaxLinesListener console_max_lines_listener;

// Listener for all volume settings
class LLAudioListener: public LLSimpleListener
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		audio_update_volume(true);
		return true;
	}
};
static LLAudioListener audio_listener;

class LLJoystickListener : public LLSimpleListener
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLViewerJoystick::updateCamera(TRUE);
		return true;
	}
};
static LLJoystickListener joystick_listener;

void stop_video();
void prepare_video(const LLParcel *parcel);

class LLAudioStreamMusicListener: public LLSimpleListener
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if (gAudiop)
		{
			if ( event->getValue().asBoolean() )
			{
				if (gParcelMgr
					&& gParcelMgr->getAgentParcel()
					&& !gParcelMgr->getAgentParcel()->getMusicURL().empty())
				{
					// if stream is already playing, don't call this
					// otherwise music will briefly stop
					if ( ! gAudiop->isInternetStreamPlaying() )
					{
						gAudiop->startInternetStream(gParcelMgr->getAgentParcel()->getMusicURL().c_str());
					}
				}
			}
			else
			{
				gAudiop->stopInternetStream();
			}
		}
		return true;
	}
};

static LLAudioStreamMusicListener audio_stream_music_listener;



class LLAudioStreamMediaListener: public LLSimpleListener
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if (LLMediaEngine::getInstance() && LLMediaEngine::getInstance()->isAvailable())
		{
			if (event->getValue().asBoolean())
			{
				gMessageSystem->setHandlerFunc ( "ParcelMediaCommandMessage", LLMediaEngine::process_parcel_media );
				gMessageSystem->setHandlerFunc ( "ParcelMediaUpdate", LLMediaEngine::process_parcel_media_update );
				if ( ( gParcelMgr ) &&
					 ( gParcelMgr->getAgentParcel () ) && 
					 ( !gParcelMgr->getAgentParcel()->getMediaURL().empty() ) )
				{
					prepare_video ( gParcelMgr->getAgentParcel () );
				}
			}
			else
			{
				gMessageSystem->setHandlerFunc("ParcelMediaCommandMessage", null_message_callback);
				gMessageSystem->setHandlerFunc ( "ParcelMediaUpdate", null_message_callback );
				stop_video();
			}
		}
		else
		{
			if (gSavedSettings.getWarning("QuickTimeInstalled"))
			{
				gSavedSettings.setWarning("QuickTimeInstalled", FALSE);

				LLNotifyBox::showXml("NoQuickTime" );
			}
		}

		return true;
	}
};

static LLAudioStreamMediaListener audio_stream_media_listener;


class LLUseOcclusionListener: public LLSimpleListener
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLPipeline::sUseOcclusion = (event->getValue().asBoolean() && gGLManager.mHasOcclusionQuery 
				&& gFeatureManagerp->isFeatureAvailable("UseOcclusion") && !gUseWireframe);
		return true;
	}
};
static LLUseOcclusionListener use_occlusion_listener;

class LLNumpadControlListener: public LLSimpleListener
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if (gKeyboard)
		{
			gKeyboard->setNumpadDistinct(static_cast<LLKeyboard::e_numpad_distinct>(event->getValue().asInteger()));
		}
		return true;
	}
};

static LLNumpadControlListener numpad_control_listener;

class LLRenderUseVBOListener: public LLSimpleListener
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		gPipeline.setUseVBO(event->getValue().asBoolean());
		return true;
	}
};
static LLRenderUseVBOListener render_use_vbo_listener;

class LLRenderLightingDetailListener: public LLSimpleListener
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		gPipeline.setLightingDetail(event->getValue().asInteger());
		return true;
	}
};
static LLRenderLightingDetailListener render_lighting_detail_listener;

////////////////////////////////////////////////////////////////////////////

void settings_setup_listeners()
{
	gSavedSettings.getControl("FirstPersonAvatarVisible")->addListener(&render_avatar_mouselook_listener);
	gSavedSettings.getControl("MouseSensitivity")->addListener(&mouse_sensitivity_listener);
	gSavedSettings.getControl("InvertMouse")->addListener(&invert_mouse_listener);
	gSavedSettings.getControl("AFKTimeout")->addListener(&afk_timeout_listener);
	gSavedSettings.getControl("RenderFarClip")->addListener(&render_far_clip_listener);
	gSavedSettings.getControl("RenderTerrainDetail")->addListener(&terrain_detail_listener);
	gSavedSettings.getControl("RenderRippleWater")->addListener(&set_shader_listener);
	gSavedSettings.getControl("RenderAvatarVP")->addListener(&set_shader_listener);
	gSavedSettings.getControl("VertexShaderEnable")->addListener(&set_shader_listener);
	gSavedSettings.getControl("RenderDynamicReflections")->addListener(&set_shader_listener);
	gSavedSettings.getControl("RenderGlow")->addListener(&release_gl_buffer_listener);
	gSavedSettings.getControl("RenderGlowResolution")->addListener(&release_gl_buffer_listener);
	gSavedSettings.getControl("RenderAvatarMode")->addListener(&set_shader_listener);
	gSavedSettings.getControl("RenderVolumeLODFactor")->addListener(&volume_lod_listener);
	gSavedSettings.getControl("RenderAvatarLODFactor")->addListener(&avatar_lod_listener);
	gSavedSettings.getControl("RenderTreeLODFactor")->addListener(&tree_lod_listener);
	gSavedSettings.getControl("RenderFlexTimeFactor")->addListener(&flex_lod_listener);
	gSavedSettings.getControl("ThrottleBandwidthKBPS")->addListener(&bandwidth_listener);
	gSavedSettings.getControl("RenderGamma")->addListener(&gamma_listener);
	gSavedSettings.getControl("RenderNightBrightness")->addListener(&night_brightness_listener);
	gSavedSettings.getControl("RenderFogRatio")->addListener(&fog_ratio_listener);
	gSavedSettings.getControl("RenderMaxPartCount")->addListener(&max_partCount_listener);
	gSavedSettings.getControl("AvatarCompositeLimit")->addListener(&composite_limit_listener);
	gSavedSettings.getControl("GraphicsCardMemorySetting")->addListener(&video_memory_listener);
	gSavedSettings.getControl("ChatFontSize")->addListener(&chat_font_size_listener);
	gSavedSettings.getControl("ChatPersistTime")->addListener(&chat_persist_time_listener);
	gSavedSettings.getControl("ConsoleMaxLines")->addListener(&console_max_lines_listener);
	gSavedSettings.getControl("UseOcclusion")->addListener(&use_occlusion_listener);
	gSavedSettings.getControl("AudioLevelMaster")->addListener(&audio_listener);
 	gSavedSettings.getControl("AudioLevelSFX")->addListener(&audio_listener);
 	gSavedSettings.getControl("AudioLevelUI")->addListener(&audio_listener);
	gSavedSettings.getControl("AudioLevelAmbient")->addListener(&audio_listener);
	gSavedSettings.getControl("AudioLevelMusic")->addListener(&audio_listener);
	gSavedSettings.getControl("AudioLevelMedia")->addListener(&audio_listener);
	gSavedSettings.getControl("AudioLevelVoice")->addListener(&audio_listener);
	gSavedSettings.getControl("AudioLevelDistance")->addListener(&audio_listener);
	gSavedSettings.getControl("AudioLevelDoppler")->addListener(&audio_listener);
	gSavedSettings.getControl("AudioLevelRolloff")->addListener(&audio_listener);
	gSavedSettings.getControl("AudioStreamingMusic")->addListener(&audio_stream_music_listener);
	gSavedSettings.getControl("AudioStreamingVideo")->addListener(&audio_stream_media_listener);
	gSavedSettings.getControl("MuteAudio")->addListener(&audio_listener);
	gSavedSettings.getControl("MuteMusic")->addListener(&audio_listener);
	gSavedSettings.getControl("MuteMedia")->addListener(&audio_listener);
	gSavedSettings.getControl("MuteVoice")->addListener(&audio_listener);
	gSavedSettings.getControl("MuteAmbient")->addListener(&audio_listener);
	gSavedSettings.getControl("MuteUI")->addListener(&audio_listener);
	gSavedSettings.getControl("RenderVBOEnable")->addListener(&render_use_vbo_listener);
	gSavedSettings.getControl("RenderLightingDetail")->addListener(&render_lighting_detail_listener);
	gSavedSettings.getControl("NumpadControl")->addListener(&numpad_control_listener);
	gSavedSettings.getControl("FlycamAxis0")->addListener(&joystick_listener);
	gSavedSettings.getControl("FlycamAxis1")->addListener(&joystick_listener);
	gSavedSettings.getControl("FlycamAxis2")->addListener(&joystick_listener);
	gSavedSettings.getControl("FlycamAxis3")->addListener(&joystick_listener);
	gSavedSettings.getControl("FlycamAxis4")->addListener(&joystick_listener);
	gSavedSettings.getControl("FlycamAxis5")->addListener(&joystick_listener);
	gSavedSettings.getControl("FlycamAxis6")->addListener(&joystick_listener);
}
