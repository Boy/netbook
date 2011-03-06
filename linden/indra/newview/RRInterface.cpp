#include "llviewerprecompiledheaders.h"

#include "llagent.h"
#include "lldrawpoolalpha.h"
#include "llfloaterchat.h"
#include "llfloatermap.h"
#include "llfloaterworldmap.h"
#include "llfocusmgr.h"
#include "llhudtext.h"
#include "llselectmgr.h"
#include "llstartup.h"
#include "llvoavatar.h"
#include "llinventoryview.h"
#include "lloverlaybar.h"
#include "llurlsimstring.h"
#include "lltracker.h"
#include "llviewercontrol.h"
#include "llviewermenu.h"
#include "llviewerobjectlist.h"
#include "llviewerwindow.h"
#include "llinventorybridge.h"
#include "llsky.h"
#include "llviewermessage.h"
#include "pipeline.h"

#include "RRInterface.h"

extern BOOL RRNoSetEnv;

// --
// Local functions
std::string dumpList2String (std::deque<std::string> list, std::string sep, int size = -1)
{
	bool found_one = false;
	if (size < 0) size = (int)list.size();
	std::string res = "";
	for (int i = 0; i < (int)list.size() && i < size; ++i) {
		if (found_one) res += sep;
		found_one = true;
		res += list[i];
	}
	return res;
}

int match (std::deque<std::string> list, std::string str)
{
	// does str contain list[0]/list[1]/.../list[n] ?
	// yes => return the size of the list
	// no  => try again after removing the last element
	// return 0 if never found
	// Exception : if str starts with a "~" character, the match must be exact
	unsigned int size = list.size();
	std::string dump;
	while (size > 0) {
		dump = dumpList2String (list, "/", (int)size);
		if (str != "" && str[0] == '~' && str != dump) return 0;
		if (str.find (dump) != -1) return (int)size;
		size--;
	}
	return 0;
}

std::deque<std::string> getSubList (std::deque<std::string> list, int min, int max = -1)
{
	if (min < 0) min = 0;
	if (max < 0) max = list.size() - 1;
	std::deque<std::string> res;
	for (int i = min; i <= max; ++i) {
		res.push_back (list[i]);
	}
	return res;
}

bool findMultiple (std::deque<std::string> list, std::string str)
{
	// returns true if all the tokens in list are contained into str
	unsigned int size = list.size();
	for (unsigned int i = 0; i < size; i++) {
		if (str.find (list[i]) == -1) return false;
	}
	return true;
}

void refreshCachedVariable (std::string var)
{
	// Call this function when adding/removing a restriction only, i.e. in this file
	// Test the cached variables in the code of the viewer itself
	BOOL contained = gAgent.mRRInterface.contains (var);
	if (var == "detach" || var.find ("detach:") == 0) {
		gAgent.mRRInterface.mContainsDetach = contained;
		gAgent.mRRInterface.mHasLockedHuds = gAgent.mRRInterface.hasLockedHuds();
		if (gAgent.mRRInterface.mHasLockedHuds) {
			// To force the viewer to render the HUDs again, just in case
			LLPipeline::sShowHUDAttachments = TRUE;
		}
	}
	else if (var == "showinv")				gAgent.mRRInterface.mContainsShowinv = contained;
	else if (var == "unsit")				gAgent.mRRInterface.mContainsUnsit = contained;
	else if (var == "fartouch")				gAgent.mRRInterface.mContainsFartouch = contained;
	else if (var == "showworldmap")			gAgent.mRRInterface.mContainsShowworldmap = contained;
	else if (var == "showminimap")			gAgent.mRRInterface.mContainsShowminimap = contained;
	else if (var == "showloc")				gAgent.mRRInterface.mContainsShowloc = contained;
	else if (var == "shownames")			gAgent.mRRInterface.mContainsShownames = contained;
	else if (var == "setenv")				gAgent.mRRInterface.mContainsSetenv = contained;
	else if (var == "fly")					gAgent.mRRInterface.mContainsFly = contained;
	else if (var == "edit")					gAgent.mRRInterface.mContainsEdit = contained;
	else if (var == "rez")					gAgent.mRRInterface.mContainsRez = contained;
	else if (var == "showhovertextall")		gAgent.mRRInterface.mContainsShowhovertextall = contained;
	else if (var == "showhovertexthud")		gAgent.mRRInterface.mContainsShowhovertexthud = contained;
	else if (var == "showhovertextworld")	gAgent.mRRInterface.mContainsShowhovertextworld = contained;
}

std::string getFirstName (std::string fullName)
{
	int ind = fullName.find (" ");
	if (ind != -1) return fullName.substr (0, ind);
	else return fullName;
}

std::string getLastName (std::string fullName)
{
	int ind = fullName.find (" ");
	if (ind != -1) return fullName.substr (ind+1);
	else return fullName;
}

void updateAllHudTexts ()
{
	LLHUDText::TextObjectIterator text_it;
	
	for (text_it = LLHUDText::sTextObjects.begin(); 
		text_it != LLHUDText::sTextObjects.end(); 
		++text_it)
	{
		LLHUDText *hudText = *text_it;
		if (hudText && hudText->mLastMessageText != "") {
			// do not update the floating names of the avatars around
			LLViewerObject* obj = hudText->getSourceObject();
			if (obj && !obj->isAvatar()) {
				hudText->setStringUTF8(hudText->mLastMessageText);
			}
		}
	}
}

void updateOneHudText (LLUUID uuid)
{
	LLViewerObject* obj = gObjectList.findObject(uuid);
	if (obj) {
		if (obj->mText.notNull()) {
			LLHUDText *hudText = obj->mText.get();
			if (hudText && hudText->mLastMessageText != "") {
				hudText->setStringUTF8(hudText->mLastMessageText);
			}
		}
	}
}
// --





RRInterface::RRInterface():
	sInventoryFetched(FALSE),
	sAllowCancelTp(TRUE),
	sSitTargetId(),
	sTimeBeforeReattaching(0)
{
	sAllowedU32 = 
	",AvatarSex"			// 0 female, 1 male
	",";

	sAllowedS32 = ",";
	sAllowedF32 = ",";
	sAllowedBOOLEAN = ",";
	sAllowedSTRING = ",";
	sAllowedVEC3 = ",";
	sAllowedVEC3D = ",";
	sAllowedRECT = ",";
	sAllowedCOL4 = ",";
	sAllowedCOL3 = ",";
	sAllowedCOL4U = ",";

	sAssetToReattach.setNull();
}

RRInterface::~RRInterface()
{
}

std::string RRInterface::getVersion ()
{
	return "RestrainedLife viewer v"RR_VERSION; // there is no '+' between the string and the macro
}

BOOL RRInterface::isAllowed (LLUUID object_uuid, std::string action, BOOL log_it)
{
	BOOL debug = gSavedSettings.getBOOL("RestrainedLifeDebug") && log_it;
	if (debug) {
		llinfos << object_uuid.asString() << "      " << action << llendl;
	}
	RRMAP::iterator it = sSpecialObjectBehaviours.find (object_uuid.asString());
	while (it != sSpecialObjectBehaviours.end() &&
			it != sSpecialObjectBehaviours.upper_bound(object_uuid.asString()))
	{
		if (debug) {
			llinfos << "  checking " << it->second << llendl;
		}
		if (it->second == action) {
			if (debug) {
				llinfos << "  => forbidden. " << llendl;
			}
			return FALSE;
		}
		it++;
	}
	if (debug) {
		llinfos << "  => allowed. " << llendl;
	}
	return TRUE;
}

BOOL RRInterface::contains (std::string action)
{
	RRMAP::iterator it = sSpecialObjectBehaviours.begin ();
	LLString::toLower(action);
//	llinfos << "looking for " << action << llendl;
	while (it != sSpecialObjectBehaviours.end()) {
		if (it->second == action) {
//			llinfos << "found " << it->second << llendl;
			return TRUE;
		}
		it++;
	}
	return FALSE;
}

BOOL RRInterface::containsSubstr (std::string action)
{
	RRMAP::iterator it = sSpecialObjectBehaviours.begin ();
	LLString::toLower(action);
//	llinfos << "looking for " << action << llendl;
	while (it != sSpecialObjectBehaviours.end()) {
		if (it->second.find (action) != -1) {
//			llinfos << "found " << it->second << llendl;
			return TRUE;
		}
		it++;
	}
	return FALSE;
}

BOOL RRInterface::add (LLUUID object_uuid, std::string action, std::string option)
{
	if (gSavedSettings.getBOOL("RestrainedLifeDebug")) {
		llinfos << object_uuid.asString() << "       " << action << "      " << option << llendl;
	}

	std::string canon_action = action;
	if (option!="") action+=":"+option;

	if (isAllowed (object_uuid, action)) {
		// Notify if needed
		notify (object_uuid, action, "=n");
		
		// Actions to do BEFORE inserting the new behav
		if (action=="showinv") {
			//LLInventoryView::cleanup ();
			for (int i=0; i<LLInventoryView::sActiveViews.count(); ++i) {
				if (LLInventoryView::sActiveViews.get(i)->getVisible()) {
					LLInventoryView::sActiveViews.get(i)->setVisible (false);
				}
			}
		}
		else if (action=="showminimap") {
			gFloaterMap->setVisible(FALSE);
		}
		else if (action=="shownames") {
			LLFloaterChat::getInstance()->childSetVisible("active_speakers_panel", FALSE);
		}
		else if (action=="fly") {
 			gAgent.setFlying (FALSE);
   		}
		else if (action=="showworldmap" || action == "showloc") {
			if (gFloaterWorldMap->getVisible()) {
				LLFloaterWorldMap::toggle(NULL);
			}
		}
		else if (action=="edit") {
			LLPipeline::setRenderBeacons(FALSE);
			LLPipeline::setRenderScriptedBeacons(FALSE);
			LLPipeline::setRenderScriptedTouchBeacons(FALSE);
			LLPipeline::setRenderPhysicalBeacons(FALSE);
			LLPipeline::setRenderSoundBeacons(FALSE);
			LLPipeline::setRenderParticleBeacons(FALSE);
			LLPipeline::setRenderHighlights(FALSE);
			LLDrawPoolAlpha::sShowDebugAlpha = FALSE;
		}
		else if (action=="setenv") {
			if (RRNoSetEnv) {
				return TRUE;
			}
			gSavedSettings.setBOOL("SkyOverrideSimSunPosition", FALSE);
			gSavedSettings.setBOOL("MouseSun", FALSE);
			gSavedSettings.setBOOL("FixedWeather", FALSE);
		}

		// Insert the new behav
		sSpecialObjectBehaviours.insert(std::pair<std::string, std::string>(object_uuid.asString(), action));
		refreshCachedVariable(action);

		// Actions to do AFTER inserting the new behav
		if (action=="showhovertextall" || action=="showloc" || action=="shownames"
			|| action=="showhovertexthud" || action=="showhovertextworld" ) {
			updateAllHudTexts();
		}
		if (canon_action == "showhovertext") {
			updateOneHudText(LLUUID(option));
		}
		return TRUE;
	}
	return FALSE;
}

BOOL RRInterface::remove (LLUUID object_uuid, std::string action, std::string option)
{
	BOOL debug = gSavedSettings.getBOOL("RestrainedLifeDebug");
	if (debug) {
		llinfos << object_uuid.asString() << "       " << action << "      " << option << llendl;
	}

	std::string canon_action = action;
	if (option!="") action+=":"+option;
	
	// Notify if needed
	notify (object_uuid, action, "=y");
	
	// Actions to do BEFORE removing the behav

	// Remove the behav
	RRMAP::iterator it = sSpecialObjectBehaviours.find (object_uuid.asString());
	while (it != sSpecialObjectBehaviours.end() &&
			it != sSpecialObjectBehaviours.upper_bound(object_uuid.asString()))
	{
		if (debug) {
			llinfos << "  checking " << it->second << llendl;
		}
		if (it->second == action) {
			sSpecialObjectBehaviours.erase(it);
			if (debug) {
				llinfos << "  => removed. " << llendl;
			}
			refreshCachedVariable(action);

			// Actions to do AFTER removing the behav
			if (action=="showhovertextall" || action=="showloc" || action=="shownames"
				|| action=="showhovertexthud" || action=="showhovertextworld" ) {
				updateAllHudTexts();
			}
			if (canon_action == "showhovertext") {
				updateOneHudText(LLUUID(option));
			}

			return TRUE;
		}
		it++;
	}

	return FALSE;
}

BOOL RRInterface::clear (LLUUID object_uuid, std::string command)
{
	BOOL debug = gSavedSettings.getBOOL("RestrainedLifeDebug");
	if (debug) {
		llinfos << object_uuid.asString() << "   /   " << command << llendl;
	}

	// Notify if needed
	notify (object_uuid, "clear" + (command!=""? ":"+command : ""), "");
	
	RRMAP::iterator it;
	it = sSpecialObjectBehaviours.begin ();
	while (it != sSpecialObjectBehaviours.end()) {
		if (debug) {
			llinfos << "  checking " << it->second << llendl;
		}
		if (it->first==object_uuid.asString() && (command=="" || it->second.find (command)!=-1)) {
			if (debug) {
				llinfos << it->second << " => removed. " << llendl;
			}
			std::string tmp = it->second;
			sSpecialObjectBehaviours.erase(it);
			refreshCachedVariable(tmp);
			it = sSpecialObjectBehaviours.begin ();
		}
		else {
			it++;
		}
	}
	updateAllHudTexts();
	return TRUE;
}

BOOL RRInterface::garbageCollector (BOOL all) {
	BOOL debug = gSavedSettings.getBOOL("RestrainedLifeDebug");
	RRMAP::iterator it;
	BOOL res=FALSE;
	LLUUID uuid;
	LLViewerObject *objp=NULL;
	it = sSpecialObjectBehaviours.begin ();
	while (it != sSpecialObjectBehaviours.end()) {
		uuid.set (it->first);
		if (all || !uuid.isNull ()) {
//			if (debug) {
//				llinfos << "testing " << it->first << llendl;
//			}
			objp = gObjectList.findObject(uuid);
			if (!objp) {
				if (debug) {
					llinfos << it->first << " not found => cleaning... " << llendl;
				}
				clear (uuid);
				res=TRUE;
				it=sSpecialObjectBehaviours.begin ();
			} else {
				it++;
			}
		} else {
			if (debug) {
				llinfos << "ignoring " << it->second << llendl;
			}
			it++;
		}
	}
	return res;
}

std::deque<std::string> RRInterface::parse (std::string str, std::string sep)
{
	int ind;
	int length = sep.length();
	std::deque<std::string> res;
	
	do {
		ind=str.find(sep);
		if (ind!=-1) {
			res.push_back (str.substr (0, ind));
			str=str.substr (ind+length);
		}
		else {
			res.push_back (str);
		}
	} while (ind!=-1);
	
	return res;
}


void RRInterface::notify (LLUUID object_uuid, std::string action, std::string suffix)
{
	// scan the list of restrictions, when finding "notify" say the restriction on the specified channel
	RRMAP::iterator it;
	int length = 7; // size of "notify:"
	int size;
	std::deque<std::string> tokens;
	LLUUID uuid;
	std::string rule;
	it = sSpecialObjectBehaviours.begin ();
	
	while (it != sSpecialObjectBehaviours.end()) {
		uuid.set (it->first);
		rule = it->second; // we are looking for rules like "notify:2222;tp", if action contains "tp" then notify the scripts on channel 2222
		if (rule.find("notify:") == 0) {
			// found a possible notification to send
			rule = rule.substr(length); // keep right part only (here "2222;tp")
			tokens = parse (rule, ";");
			size = tokens.size();
			if (size == 1 || size > 1 && action.find(tokens[1]) != -1) {
				answerOnChat(tokens[0], "/" + action + suffix); // suffix can be "=n", "=y" or whatever else we want, "/" is needed to avoid some clever griefing
			}
		}
		it++;
	}
}


BOOL RRInterface::parseCommand (std::string command, std::string& behaviour, std::string& option, std::string& param)
{
	int ind = command.find("=");
	behaviour=command;
	option="";
	param="";
	if (ind!=-1) {
		behaviour=command.substr(0, ind);
		param=command.substr(ind+1);
		ind=behaviour.find(":");
		if (ind!=-1) {
			option=behaviour.substr(ind+1);
			behaviour=behaviour.substr(0, ind); // keep in this order (option first, then behav) or crash
		}
		return TRUE;
	}
	return FALSE;
}

BOOL RRInterface::handleCommand (LLUUID uuid, std::string command)
{
	BOOL debug = gSavedSettings.getBOOL("RestrainedLifeDebug");
	// 1. check the command is actually a single one or a list of commands separated by ","
	if (command.find (",")!=-1) {
		BOOL res=TRUE;
		std::deque<std::string> list_of_commands=parse (command, ",");
		for (unsigned int i=0; i<list_of_commands.size (); ++i) {
			if (!handleCommand (uuid, list_of_commands.at(i))) res=FALSE;
		}
		return res;
	}

	// 2. this is a single command, possibly inside a 1-level recursive call (unimportant)
	// if the viewer is not fully initialized and the user does not have control of their avatar,
	// don't execute the command but retain it for later, when it is fully initialized
	// If there is another object still waiting to be automatically reattached, retain all RLV commands
	// as well to avoid an infinite loop if the one it will kick off is also locked.
	// This is valid as the object that would possibly be kicked off by the one to reattach, will have
	// its restrictions wiped out by the garbage collector
	if (LLStartUp::getStartupState() != STATE_STARTED
		|| (sAssetToReattach.notNull() && sTimeBeforeReattaching > 0)) {
		Command cmd;
		cmd.uuid=uuid;
		cmd.command=command;
		if (debug) {
			llinfos << "Retaining command : " << command << llendl;
		}
		sRetainedCommands.push_back (cmd);
		return TRUE;
	}

	// 3. parse the command, which is of one of these forms :
	// behav=param
	// behav:option=param
	std::string behav;
	std::string option;
	std::string param;
	LLString::toLower(command);
	if (parseCommand (command, behav, option, param)) // detach=n, recvchat=n, recvim=n, unsit=n, recvim:<uuid>=add, clear=tplure:
	{
		if (debug) {
			llinfos << "[" << uuid.asString() << "]  [" << behav << "]  [" << option << "] [" << param << "]" << llendl;
		}
		if (behav=="version") return answerOnChat (param, getVersion ());
		else if (behav=="getoutfit") return answerOnChat (param, getOutfit (option));
		else if (behav=="getattach") return answerOnChat (param, getAttachments (option));
		else if (behav=="getstatus") return answerOnChat (param, getStatus (uuid, option));
		else if (behav=="getstatusall") {
			uuid.setNull();
			return answerOnChat (param, getStatus (uuid, option));
		}
		else if (behav=="getinv") return answerOnChat (param, getInventoryList (option));
		else if (behav=="getinvworn") return answerOnChat (param, getInventoryList (option, TRUE));
		else if (behav=="getsitid") return answerOnChat (param, getSitTargetId ().asString());
		else if (behav=="getpath") return answerOnChat (param, getFullPath (getItem(uuid), option)); // option can be empty (=> find path to object) or the name of an attach pt or the name of a clothing layer
		else if (behav=="findfolder") return answerOnChat (param, getFullPath (findCategoryUnderRlvShare (option)));
		else if (behav.find ("getenv_") == 0) return answerOnChat (param, getEnvironment (behav));
		else if (behav.find ("getdebug_") == 0) return answerOnChat (param, getDebugSetting (behav));
		else {
			if (param=="n" || param=="add") add (uuid, behav, option);
			else if (param=="y" || param=="rem") remove (uuid, behav, option);
			else if (behav=="clear") clear (uuid, param);
			else if (param=="force") force (uuid, behav, option);
			else return FALSE;
		}
	}
	else // clear
	{
		if (debug) {
			llinfos << uuid.asString() << "       " << behav << llendl;
		}
		if (behav=="clear") clear (uuid);
		else return FALSE;
	}
	return TRUE;
}

BOOL RRInterface::fireCommands ()
{
	BOOL ok=TRUE;
	if (sRetainedCommands.size ()) {
		if (gSavedSettings.getBOOL("RestrainedLifeDebug")) {
			llinfos << "Firing commands : " << sRetainedCommands.size () << llendl;
		}
		Command cmd;
		while (!sRetainedCommands.empty ()) {
			cmd=sRetainedCommands[0];
			ok=ok & handleCommand (cmd.uuid, cmd.command);
			sRetainedCommands.pop_front ();
		}
	}
	return ok;
}

BOOL RRInterface::force (LLUUID object_uuid, std::string command, std::string option)
{
	BOOL debug = gSavedSettings.getBOOL("RestrainedLifeDebug");
	if (debug) {
		llinfos << command << "     " << option << llendl;
	}
	if (command=="sit") { // sit:UUID
		BOOL allowed_to_sittp=TRUE;
		if (!isAllowed (object_uuid, "sittp")) {
			allowed_to_sittp=FALSE;
			remove (object_uuid, "sittp", "");
		}
		LLUUID uuid (option);
		gLastHitNonFloraObjectID=uuid;
		handle_sit_or_stand ();
		if (!allowed_to_sittp) add (object_uuid, "sittp", "");
	}
	else if (command=="unsit") { // unsit
		if (debug) {
			llinfos << "trying to unsit" << llendl;
		}
		if (gAgent.getAvatarObject() &&
			gAgent.getAvatarObject()->mIsSitting) {
			if (debug) {
				llinfos << "found avatar object" << llendl;
			}
			if (gAgent.mRRInterface.mContainsUnsit) {
				if (debug) {
					llinfos << "prevented from unsitting" << llendl;
				}
				return TRUE;
			}
			if (debug) {
				llinfos << "unsitting agent" << llendl;
			}
			LLOverlayBar::onClickStandUp(NULL);
			send_agent_update(TRUE, TRUE);
		}
	}
	else if (command=="remoutfit") { // remoutfit:shoes
		if (option=="") {
			gAgent.removeWearable (WT_GLOVES);
			gAgent.removeWearable (WT_JACKET);
			gAgent.removeWearable (WT_PANTS);
			gAgent.removeWearable (WT_SHIRT);
			gAgent.removeWearable (WT_SHOES);
			gAgent.removeWearable (WT_SKIRT);
			gAgent.removeWearable (WT_SOCKS);
			gAgent.removeWearable (WT_UNDERPANTS);
			gAgent.removeWearable (WT_UNDERSHIRT);
		} else {
			EWearableType type = getOutfitLayerAsType (option);
			if (type != WT_INVALID) {
				// clothes and hair only, not skin, eyes, or shape
				if (type == WT_HAIR || LLWearable::typeToAssetType(type) == LLAssetType::AT_CLOTHING) {
					gAgent.removeWearable (type); // remove by layer
				}
			}
			else forceDetachByName (option); // remove by category (in RLV share)
		}
	}
	else if (command=="detach") { // detach:chest=force OR detach:restraints/cuffs=force
		LLViewerJointAttachment* attachpt = findAttachmentPointFromName (option, TRUE); // exact name
		if (attachpt != NULL || option == "") return forceDetach (option); // remove by attach pt
		else forceDetachByName (option);
	}
	else if (command=="detachme") { // detachme=force to detach this object specifically
		return forceDetachByUuid (object_uuid.asString()); // remove by uuid
	}
	else if (command=="detachthis") { // detachthis=force to detach the folder containing this object
		return forceDetachByName (getFullPath (getItem(object_uuid), option), FALSE);
	}
	else if (command=="detachall") { // detachall:cuffs=force to detach a folder and its subfolders
		return forceDetachByName (option, TRUE);
	}
	else if (command=="detachallthis") { // detachallthis=force to detach the folder containing this object and also its subfolders
		return forceDetachByName (getFullPath (getItem(object_uuid), option), TRUE);
	}
	else if (command=="tpto") { // tpto:X/Y/Z=force (X, Y, Z are GLOBAL coordinates)
		BOOL allowed_to_tploc=TRUE;
		BOOL allowed_to_unsit=TRUE;
		BOOL res;
		if (!isAllowed (object_uuid, "tploc")) {
			allowed_to_tploc=FALSE;
			remove (object_uuid, "tploc", "");
		}
		if (!isAllowed (object_uuid, "unsit")) {
			allowed_to_unsit=FALSE;
			remove (object_uuid, "unsit", "");
		}
		res = forceTeleport (option);
		if (!allowed_to_tploc) add (object_uuid, "tploc", "");
		if (!allowed_to_unsit) add (object_uuid, "unsit", "");
		return res;
	}
	else if (command=="attach" || command == "addoutfit") { // attach:cuffs=force
		return forceAttach (option);
	}
	else if (command=="attachthis") { // attachthis=force to attach the folder containing this object
		return forceAttach (getFullPath (getItem(object_uuid), option), FALSE);
	}
	else if (command=="attachall") { // attachall:cuffs=force to attach a folder and its subfolders
		return forceAttach (option, TRUE);
	}
	else if (command=="attachallthis") { // attachallthis=force to attach the folder containing this object and its subfolders
		return forceAttach (getFullPath (getItem(object_uuid), option), TRUE);
	}
	else if (command.find ("setenv_") == 0) {
		BOOL res = TRUE;
		BOOL allowed = TRUE;
		if (!RRNoSetEnv) {
			if (!isAllowed (object_uuid, "setenv")) {
				allowed=FALSE;
				remove (object_uuid, "setenv", "");
			}
			if (!mContainsSetenv) res = forceEnvironment (command, option);
			if (!allowed) add (object_uuid, "setenv", "");
		}
		return res;
	}
	else if (command.find ("setdebug_") == 0) {
		BOOL res = TRUE;
		BOOL allowed = TRUE;
		if (!isAllowed (object_uuid, "setdebug")) {
			allowed=FALSE;
			remove (object_uuid, "setdebug", "");
		}
		if (!contains("setdebug")) res = forceDebugSetting (command, option);
		if (!allowed) add (object_uuid, "setdebug", "");
		return res;
	}
	else if (command=="setrot") { // setrot:angle_radians=force
		BOOL res = TRUE;
		LLVOAvatar* avatar = gAgent.getAvatarObject();
		if (!avatar) return FALSE;
		F32 val = atof (option.c_str());
		gAgent.startCameraAnimation();
		LLVector3 rot (0.0, 1.0, 0.0);
		rot = rot.rotVec(-val, LLVector3::z_axis);
		rot.normVec();
		gAgent.resetAxes(rot);
		return res;
	}
	return TRUE;
}

BOOL RRInterface::answerOnChat (std::string channel, std::string msg)
{
	if (channel == "0") return FALSE; // protection against abusive "@getstatus=0" commands
	if (atoi (channel.c_str()) <= 0) return FALSE; // prevent from trying to say something on a negative channel, or on a non-numerical channel
	gChatBar->sendChatFromViewer("/"+channel+" "+msg, CHAT_TYPE_SHOUT, FALSE);
	if (gSavedSettings.getBOOL("RestrainedLifeDebug")) {
		llinfos << "/" << channel << " " << msg << llendl;
	}
	return TRUE;
}

std::string RRInterface::crunchEmote (std::string msg, unsigned int truncateTo) {
	std::string crunched = msg;

	if (msg.find ("/me ") == 0 || msg.find ("/me'") == 0) {
		// Only allow emotes without "spoken" text.
		// Forbid text containing any symbol which could be used as quotes.
		if (msg.find ("\"") != -1 || msg.find ("''") != -1
		    || msg.find ("(")  != -1 || msg.find (")")  != -1
		    || msg.find (" -") != -1 || msg.find ("- ") != -1
		    || msg.find ("*")  != -1 || msg.find ("=")  != -1
		    || msg.find ("^")  != -1 || msg.find ("_")  != -1
		    || msg.find ("?")  != -1 || msg.find ("~")  != -1)
		{
			crunched = "...";
		}
		else if (!contains ("emote")) {
			// Only allow short emotes.
			int i = msg.find (".");
			if (i != -1) {
				crunched = msg.substr (0, ++i);
			}
			if (crunched.length () > truncateTo) {
				crunched = crunched.substr (0, truncateTo);
			}
		}
	}
	else if (msg.find ("/") == 0) {
		// only allow short gesture names (to avoid cheats).
		if (msg.length () > 7) { // allows things like "/ao off", "/hug X"
			crunched = "...";
		}
	}
	else if (msg.find ("((") != 0 || msg.find ("))") != msg.length () - 2) {
		// Only allow OOC chat, starting with "((" and ending with "))".
		crunched = "...";
	}
	return crunched;
}

std::string RRInterface::getOutfitLayerAsString (EWearableType layer)
{
	switch (layer) {
		case WT_SKIN: return WS_SKIN;
		case WT_GLOVES: return WS_GLOVES;
		case WT_JACKET: return WS_JACKET;
		case WT_PANTS: return WS_PANTS;
		case WT_SHIRT: return WS_SHIRT;
		case WT_SHOES: return WS_SHOES;
		case WT_SKIRT: return WS_SKIRT;
		case WT_SOCKS: return WS_SOCKS;
		case WT_UNDERPANTS: return WS_UNDERPANTS;
		case WT_UNDERSHIRT: return WS_UNDERSHIRT;
		case WT_EYES: return WS_EYES;
		case WT_HAIR: return WS_HAIR;
		case WT_SHAPE: return WS_SHAPE;
		default: return "";
	}
}

EWearableType RRInterface::getOutfitLayerAsType (std::string layer)
{
	if (layer==WS_SKIN) return WT_SKIN;
	if (layer==WS_GLOVES) return WT_GLOVES;
	if (layer==WS_JACKET) return WT_JACKET;
	if (layer==WS_PANTS) return WT_PANTS;
	if (layer==WS_SHIRT) return WT_SHIRT;
	if (layer==WS_SHOES) return WT_SHOES;
	if (layer==WS_SKIRT) return WT_SKIRT;
	if (layer==WS_SOCKS) return WT_SOCKS;
	if (layer==WS_UNDERPANTS) return WT_UNDERPANTS;
	if (layer==WS_UNDERSHIRT) return WT_UNDERSHIRT;
	if (layer==WS_EYES) return WT_EYES;
	if (layer==WS_HAIR) return WT_HAIR;
	if (layer==WS_SHAPE) return WT_SHAPE;
	return WT_INVALID;
}

std::string RRInterface::getOutfit (std::string layer)
{
	if (layer==WS_SKIN) return (gAgent.getWearable (WT_SKIN) != NULL? "1" : "0");
	if (layer==WS_GLOVES) return (gAgent.getWearable (WT_GLOVES) != NULL? "1" : "0");
	if (layer==WS_JACKET) return (gAgent.getWearable (WT_JACKET) != NULL? "1" : "0");
	if (layer==WS_PANTS) return (gAgent.getWearable (WT_PANTS) != NULL? "1" : "0");
	if (layer==WS_SHIRT)return (gAgent.getWearable (WT_SHIRT) != NULL? "1" : "0");
	if (layer==WS_SHOES) return (gAgent.getWearable (WT_SHOES) != NULL? "1" : "0");
	if (layer==WS_SKIRT) return (gAgent.getWearable (WT_SKIRT) != NULL? "1" : "0");
	if (layer==WS_SOCKS) return (gAgent.getWearable (WT_SOCKS) != NULL? "1" : "0");
	if (layer==WS_UNDERPANTS) return (gAgent.getWearable (WT_UNDERPANTS) != NULL? "1" : "0");
	if (layer==WS_UNDERSHIRT) return (gAgent.getWearable (WT_UNDERSHIRT) != NULL? "1" : "0");
	if (layer==WS_EYES) return (gAgent.getWearable (WT_EYES) != NULL? "1" : "0");
	if (layer==WS_HAIR) return (gAgent.getWearable (WT_HAIR) != NULL? "1" : "0");
	if (layer==WS_SHAPE) return (gAgent.getWearable (WT_SHAPE) != NULL? "1" : "0");
	return getOutfit (WS_GLOVES)+getOutfit (WS_JACKET)+getOutfit (WS_PANTS)
			+getOutfit (WS_SHIRT)+getOutfit (WS_SHOES)+getOutfit (WS_SKIRT)
			+getOutfit (WS_SOCKS)+getOutfit (WS_UNDERPANTS)+getOutfit (WS_UNDERSHIRT)
			+getOutfit (WS_SKIN)+getOutfit (WS_EYES)+getOutfit (WS_HAIR)+getOutfit (WS_SHAPE);
}

std::string RRInterface::getAttachments (std::string attachpt)
{
	BOOL debug = gSavedSettings.getBOOL("RestrainedLifeDebug");
	std::string res="";
	std::string name;
	if (attachpt=="") res+="0"; // to match the LSL macros
	LLVOAvatar* avatar = gAgent.getAvatarObject();
	for (LLViewerJointAttachment* attachment = avatar->mAttachmentPoints.getFirstData(); 
		attachment;
		attachment = avatar->mAttachmentPoints.getNextData())
	{
		name=attachment->getName ();
		LLString::toLower(name);
		if (debug) {
			llinfos << "trying <" << name << ">" << llendl;
		}
		if (attachpt=="" || attachpt==name) {
			if (!attachment->getItemID().isNull()) res+="1"; //attachment->getName ();
			else res+="0";
		}
	}
	return res;
}

std::string RRInterface::getStatus (LLUUID object_uuid, std::string rule)
{
	std::string res="";
	std::string name;
	RRMAP::iterator it;
	if (object_uuid.isNull()) {
		it = sSpecialObjectBehaviours.begin();
	}
	else {
		it = sSpecialObjectBehaviours.find (object_uuid.asString());
	}
	bool is_first=true;
	while (it != sSpecialObjectBehaviours.end() &&
			(object_uuid.isNull() || it != sSpecialObjectBehaviours.upper_bound(object_uuid.asString())))
	{
		if (rule=="" || it->second.find (rule)!=-1) {
			//if (!is_first) 
			res+="/";
			res+=it->second;
			is_first=false;
		}
		it++;
	}
	return res;
}

BOOL RRInterface::forceDetach (std::string attachpt)
{
	BOOL debug = gSavedSettings.getBOOL("RestrainedLifeDebug");
	std::string name;
	BOOL res=FALSE;
	LLVOAvatar* avatar = gAgent.getAvatarObject();
	for (LLViewerJointAttachment* attachment = avatar->mAttachmentPoints.getFirstData(); 
		attachment;
		attachment = avatar->mAttachmentPoints.getNextData())
	{
		name=attachment->getName ();
		LLString::toLower(name);
		if (debug) {
			llinfos << "trying <" << name << ">" << llendl;
		}
		if (attachpt=="" || attachpt==name) {
			if (debug) {
				llinfos << "found => detaching" << llendl;
			}
			if (attachment->getObject()) {
				handle_detach_from_avatar ((void*)attachment);
				res=TRUE;
			}
		}
	}
	return res;
}

BOOL RRInterface::forceDetachByUuid (std::string object_uuid)
{
	BOOL res=FALSE;
	LLVOAvatar* avatar = gAgent.getAvatarObject();
	LLViewerObject* object = gObjectList.findObject(LLUUID (object_uuid));
	if (object) {
		for (LLViewerJointAttachment* attachment = avatar->mAttachmentPoints.getFirstData(); 
			attachment;
			attachment = avatar->mAttachmentPoints.getNextData())
		{
			if (attachment && attachment->getObject() == object) {
				handle_detach_from_avatar ((void*)attachment);
				res=TRUE;
			}
		}
	}
	return res;
}

BOOL RRInterface::hasLockedHuds ()
{
	LLVOAvatar* avatar = gAgent.getAvatarObject();
	if (avatar == NULL) return FALSE;
	for (LLViewerJointAttachment* attachment = avatar->mAttachmentPoints.getFirstData(); 
		attachment;
		attachment = avatar->mAttachmentPoints.getNextData())
	{
		LLViewerObject* obj;
		if (attachment && (obj=attachment->getObject()) != NULL) {
			if (obj->isHUDAttachment()) {
				if (!isAllowed (obj->getRootEdit()->getID(), "detach", FALSE)) return TRUE;
				if (contains ("detach:"+findAttachmentNameFromPoint(attachment))) return TRUE;
			}
		}
	}
	return FALSE;
}


std::string RRInterface::getInventoryList (std::string path, BOOL withWornInfo /* = FALSE */)
{
	std::string res = "";
	LLInventoryModel::cat_array_t* cats;
	LLInventoryModel::item_array_t* items;
	LLInventoryCategory* root = NULL;
	if (path == "") root = getRlvShare();
	else root = getCategoryUnderRlvShare (path);
	
	if (root) {
		gInventory.getDirectDescendentsOf (root->getUUID(), cats, items);
		if(cats) {
			S32 count = cats->count();
			bool found_one = false;
			if (withWornInfo) {
				res += "|" + getWornItems (root);
				found_one = true;
			}
			for(S32 i = 0; i < count; ++i) {
				LLInventoryCategory* cat = cats->get(i);
				LLString name = cat->getName();
				if (name != "" && name[0] !=  '.') { // hidden folders => invisible to the list
					if (found_one) res += ",";
					res += name.c_str();
					if (withWornInfo) {
						res += "|" + getWornItems (cat);
					}
					found_one = true;
				}
			}
		}
	}

	return res;
}

std::string RRInterface::getWornItems (LLInventoryCategory* cat)
{
	// Returns a string of 2 digits according to the proportion of worn items in this folder and its children :
	// First digit is this folder, second digit is children folders
	// 0 : No item contained in the folder
	// 1 : Some items contained but none is worn
	// 2 : Some items contained and some of them are worn
	// 3 : Some items contained and all of them are worn
	std::string res_as_string = "0";
	int res			= 0;
	int subRes		= 0;
	int prevSubRes	= 0;
	int nbItems		= 0;
	int nbWorn		= 0;
	BOOL isNoMod	= FALSE;
	BOOL isRoot		= (getRlvShare() == cat);
	
	// if cat exists, scan all the items inside it
	if (cat) {
	
		LLInventoryModel::cat_array_t* cats;
		LLInventoryModel::item_array_t* items;
		
		// retrieve all the objects contained in this folder
		gInventory.getDirectDescendentsOf (cat->getUUID(), cats, items);
		if (!isRoot && items) { // do not scan the shared root
		
			// scan them one by one
			S32 count = items->count();
			for(S32 i = 0; i < count; ++i) {
			
				LLViewerInventoryItem* item = (LLViewerInventoryItem*)items->get(i);

				if (item) {
					LLVOAvatar* avatar = gAgent.getAvatarObject();
					if (item->getType() == LLAssetType::AT_OBJECT
					 || item->getType() == LLAssetType::AT_CLOTHING
					 || item->getType() == LLAssetType::AT_BODYPART) {
						nbItems++;
					}
					if( avatar && avatar->isWearingAttachment( item->getUUID() ) 
						|| gAgent.isWearingItem (item->getUUID())) nbWorn++;

					// special case : this item is no-mod, hence we need to check its parent folder
					// is correctly named, and that the item is alone in its folder.
					// If so, then the calling method will have to deal with a special character instead
					// of a number
					if (count == 1
					 && item->getType() == LLAssetType::AT_OBJECT
					 && !item->getPermissions().allowModifyBy(gAgent.getID())) {
						if (findAttachmentPointFromName (cat->getName()) != NULL) {
							isNoMod = TRUE;
						}
					}
				}
			}
		}
		
		// scan every subfolder of the folder we are scanning, recursively
		// note : in the case of no-mod items we shouldn't have sub-folders, so no need to check
		if (cats && !isNoMod) {
		
			S32 count = cats->count();
			for(S32 i = 0; i < count; ++i) {

				LLViewerInventoryCategory* cat_child = (LLViewerInventoryCategory*)cats->get(i);

				if (cat_child) {
					std::string tmp = getWornItems (cat_child);
					// translate the result for no-mod items into something the upper levels can understand
					if (tmp == "N") {
						if (!isRoot) {
							nbWorn++;
							nbItems++;
						}
					}
					else if (tmp== "n") {
						if (!isRoot) {
							nbItems++;
						}
					}
					else if (cat_child->getName() != "" && cat_child->getName()[0] != '.') { // we don't want to include invisible folders, except the ones containing a no-mod item
						// This is an actual sub-folder with several items and sub-folders inside,
						// so retain its score to integrate it into the current one
						// As it is a sub-folder, to integrate it we need to reduce its score first (consider "0" as "ignore")
						// "00" = 0, "01" = 1, "10" = 1, "30" = 3, "03" = 3, "33" = 3, all the rest gives 2 (some worn, some not worn)
						if      (tmp == "00")                               subRes = 0;
						else if (tmp == "11" || tmp == "01" || tmp == "10") subRes = 1;
						else if (tmp == "33" || tmp == "03" || tmp == "30") subRes = 3;
						else subRes = 2;

						// Then we must combine with the previous sibling sub-folders
						// Same rule as above, set to 2 in all cases except when prevSubRes == subRes or when either == 0 (nothing present, ignore)
						if      (prevSubRes == 0 && subRes == 0) subRes = 0;
						else if (prevSubRes == 0 && subRes == 1) subRes = 1;
						else if (prevSubRes == 1 && subRes == 0) subRes = 1;
						else if (prevSubRes == 1 && subRes == 1) subRes = 1;
						else if (prevSubRes == 0 && subRes == 3) subRes = 3;
						else if (prevSubRes == 3 && subRes == 0) subRes = 3;
						else if (prevSubRes == 3 && subRes == 3) subRes = 3;
						else subRes = 2;
						prevSubRes = subRes;
					}
				}
			}
		}
	}

	if (isNoMod) {
		// the folder contains one no-mod object and is named from an attachment point
		// => return a special character that will be handled by the calling method
		if (nbWorn > 0) return "N";
		else return "n";
	}
	else {
		if (isRoot || nbItems == 0) res = 0; // forcibly hide all items contained directly under #RLV
		else if (nbWorn >= nbItems) res = 3;
		else if (nbWorn > 0) res = 2;
		else res = 1;
	}
	std::stringstream str;
	str << res;
	str << subRes;
	res_as_string = str.str();
	return res_as_string;
}

LLInventoryCategory* RRInterface::getRlvShare ()
{
//	BOOL debug = gSavedSettings.getBOOL("RestrainedLifeDebug");
	LLInventoryModel::cat_array_t* cats;
	LLInventoryModel::item_array_t* items;
	gInventory.getDirectDescendentsOf (
					gInventory.findCategoryUUIDForType(LLAssetType::AT_CATEGORY), cats, items
	);

	if(cats) {
		S32 count = cats->count();
		for(S32 i = 0; i < count; ++i) {
			LLInventoryCategory* cat = cats->get(i);
			std::string name = cat->getName();
			if (name == RR_SHARED_FOLDER) {
//				if (debug) {
//					llinfos << "found " << name << llendl;
//				}
				return cat;
			}
		}
	}
	return NULL;
}

BOOL RRInterface::isUnderRlvShare (LLInventoryItem* item)
{
	if (item == NULL) return FALSE;
	LLInventoryCategory* res = NULL;
	LLInventoryCategory* rlv = getRlvShare();
	if (rlv == NULL) return FALSE;
	LLUUID root_id = gInventory.findCategoryUUIDForType(LLAssetType::AT_CATEGORY);
	
	const LLUUID& cat_id = item->getParentUUID();
	res = gInventory.getCategory (cat_id);
	
	while (res && res->getUUID() != root_id) {
		if (res == rlv) return TRUE;
		const LLUUID& parent_id = res->getParentUUID();
		res = gInventory.getCategory (parent_id);
	}
	return FALSE;
}

void RRInterface::renameAttachment (LLInventoryItem* item, LLViewerJointAttachment* attachment)
{
	// DEPRECATED : done directly in the viewer code
	// if item is worn and shared, check its name
	// if it doesn't contain the name of attachment, append it
	// (but truncate the name first if it's too long)
	if (!item || !attachment) return;
	LLVOAvatar* avatar = gAgent.getAvatarObject();
	
	if( avatar && avatar->isWearingAttachment( item->getUUID() ) ) {
		if (isUnderRlvShare (item)) {
			LLViewerJointAttachment* attachpt = findAttachmentPointFromName (item->getName());
			if (attachpt == NULL) {
				
			}
		}
	}
}

LLInventoryCategory* RRInterface::getCategoryUnderRlvShare (std::string catName, LLInventoryCategory* root)
{
	if (root == NULL) root = getRlvShare();
	if (catName == "") return root;
	LLString::toLower (catName);
	std::deque<std::string> tokens = parse (catName, "/");

	// Preliminary action : remove everything after pipes ("|"), including pipes themselves
	// This way we can feed the result of a @getinvworn command directly into this method
	// without having to clean what is after the pipes
	int nb = tokens.size();
	for (int i=0; i<nb; ++i) {
		std::string tok = tokens[i];
		int ind = tok.find ("|");
		if (ind != -1) {
			tok = tok.substr (0, ind);
			tokens[i] = tok;
		}
	}

	if (root) {

		LLInventoryModel::cat_array_t* cats;
		LLInventoryModel::item_array_t* items;
		gInventory.getDirectDescendentsOf (root->getUUID(), cats, items);
		
		if(cats) {
			S32 count = cats->count();
			LLInventoryCategory* cat = NULL;
			
			// we need to scan first and retain the best match
			int max_size_index = -1;
			int max_size = 0;
			
			for(S32 i = 0; i < count; ++i) {
				cat = cats->get(i);
				LLString name = cat->getName();
				if (name != "" && name[0] !=  '.') { // ignore invisible folders
					LLString::toLower (name);

					int size = match (tokens, name);
					if (size > max_size) {
						max_size = size;
						max_size_index = i;
					}
				}
			}

			// only now we can grab the best match and either continue deeper or return it
			if (max_size > 0) {
				cat = cats->get(max_size_index);
				if (max_size == tokens.size()) return cat;
				else return getCategoryUnderRlvShare (
									dumpList2String (
										getSubList (tokens, max_size)
									, "/")
								, cat);
			}
		}
	}

	if (gSavedSettings.getBOOL("RestrainedLifeDebug")) {
		llinfos << "category not found" << llendl;
	}
	return NULL;
}

LLInventoryCategory* RRInterface::findCategoryUnderRlvShare (std::string catName, LLInventoryCategory* root)
{
	if (root == NULL) root = getRlvShare();
	LLString::toLower (catName);
	std::deque<std::string> tokens = parse (catName, "&&");

	if (root) {
		LLInventoryModel::cat_array_t* cats;
		LLInventoryModel::item_array_t* items;
		gInventory.getDirectDescendentsOf (root->getUUID(), cats, items);
		
		if(cats)
		{
			S32 count = cats->count();
			LLInventoryCategory* cat = NULL;
			
			for(S32 i = 0; i < count; ++i)
			{
				cat = cats->get(i);
				
				// search deeper first
				LLInventoryCategory* found = findCategoryUnderRlvShare (catName, cat);
				if (found != NULL) return found;
				
			}
		}
		// return this category if it matches
		LLString name = root->getName();
		LLString::toLower (name);
		// We can't find invisible folders ('.') and dropped folders ('~')
		if (name != "" && name[0] != '.' && name[0] != '~' && findMultiple (tokens, name)) return root;
	}
	// didn't find anything
	return NULL;
}

std::string RRInterface::findAttachmentNameFromPoint (LLViewerJointAttachment* attachpt)
{
	// return the lowercased name of the attachment, or empty if null
	if (attachpt == NULL) return "";
	std::string res = attachpt->getName();
	LLString::toLower(res);
	return res;
}

// This struct is meant to be used in RRInterface::findAttachmentPointFromName below
typedef struct
{
	int length;
	int index;
	LLViewerJointAttachment* attachment;
} Candidate;

LLViewerJointAttachment* RRInterface::findAttachmentPointFromName (std::string objectName, BOOL exactName)
{
	BOOL debug = gSavedSettings.getBOOL("RestrainedLifeDebug");
	// for each possible attachment point, check whether its name appears in the name of
	// the item.
	// We are going to scan the whole list of attachments, but we won't decide which one to take right away.
	// Instead, for each matching point, we will store in lists the following results :
	// - length of its name
	// - right-most index where it is found in the name
	// - a pointer to that attachment point
	// When we have that list, choose the highest index, and in case of ex-aequo choose the longest length
	LLVOAvatar* avatar = gAgent.getAvatarObject();
	LLString::toLower(objectName);
	std::string attachName;
	int ind = -1;
	bool found_one = false;
	std::vector<Candidate> candidates;
	
	for(LLViewerJointAttachment *attachment = avatar->mAttachmentPoints.getFirstData();
		attachment;
		attachment = avatar->mAttachmentPoints.getNextData())
	{
		if (attachment) {
			attachName = attachment->getName();
			LLString::toLower(attachName);
//			if (debug) {
//				llinfos << "trying attachment " << attachName << llendl;
//			}
			if (exactName && objectName == attachName) return attachment;
			else if (!exactName && (ind = objectName.rfind (attachName)) != -1)
			{
				Candidate new_candidate;
				new_candidate.index = ind;
				new_candidate.length = attachName.length();
				new_candidate.attachment = attachment;
				candidates.push_back (new_candidate);
				found_one = true;
				if (debug) {
					llinfos << "new candidate '" << attachName << "' : index=" << new_candidate.index << "   length=" << new_candidate.length << llendl;
				}
			}
		}
	}
	if (!found_one) {
		if (debug) {
			llinfos << "no attachment found" << llendl;
		}
		return NULL;
	}
	// Now that we have at least one candidate, we have to decide which one to return
	LLViewerJointAttachment* res = NULL;
	Candidate candidate;
	unsigned int i;
	int ind_res = -1;
	int max_index = -1;
	int max_length = -1;
	// Find the highest index
	for (i=0; i<candidates.size(); ++i) {
		candidate = candidates[i];
		if (candidate.index > max_index) max_index = candidate.index;
	}
	// Find the longest match among the ones found at that index
	for (i=0; i<candidates.size(); ++i) {
		candidate = candidates[i];
		if (candidate.index == max_index) {
			if (candidate.length > max_length) {
				max_length = candidate.length;
				ind_res = i;
			}
		}
	}
	// Return this attachment point
	if (ind_res > -1) {
		candidate = candidates[ind_res];
		res = candidate.attachment;
		if (debug && res) {
			llinfos << "returning '" << res->getName() << "'" << llendl;
		}
	}
	return res;
}

LLViewerJointAttachment* RRInterface::findAttachmentPointFromParentName (LLInventoryItem* item)
{
	if (item) {
		// => look in parent folder (this could be a no-mod item), use its name to find the target
		// attach point
		LLViewerInventoryCategory* cat;
		const LLUUID& parent_id = item->getParentUUID();
		cat = gInventory.getCategory (parent_id);
		return findAttachmentPointFromName (cat->getName());
	}
	return NULL;
}

S32 RRInterface::findAttachmentPointNumber (LLViewerJointAttachment* attachment)
{
	return gAgent.getAvatarObject()->mAttachmentPoints.reverseLookup(attachment);
}

void RRInterface::fetchInventory (LLInventoryCategory* root)
{
	// do this only once on login

	if (sInventoryFetched) return;

	bool last_step = false;

	if (root == NULL) {
		root = getRlvShare();
		last_step = true;
	}

	if (root) {
		LLViewerInventoryCategory* viewer_root = (LLViewerInventoryCategory*) root;
		viewer_root->fetchDescendents ();

		LLInventoryModel::cat_array_t* cats;
		LLInventoryModel::item_array_t* items;
		
		// retrieve all the shared folders
		gInventory.getDirectDescendentsOf (viewer_root->getUUID(), cats, items);
		if (cats) {
			S32 count = cats->count();
			for(S32 i = 0; i < count; ++i) {
				LLInventoryCategory* cat = (LLInventoryCategory*)cats->get(i);
				fetchInventory (cat);
			}
		}

	}

	if (last_step) sInventoryFetched = TRUE;
}

void confirm_replace_attachment_rez(S32 option, void* user_data);

BOOL RRInterface::forceAttach (std::string category, BOOL recursive /* = FALSE */)
{
	BOOL debug = gSavedSettings.getBOOL("RestrainedLifeDebug");
	// find the category under RLV shared folder
	LLInventoryCategory* cat = getCategoryUnderRlvShare (category);
	BOOL isRoot = (getRlvShare() == cat);

	// if exists, wear all the items inside it
	if (cat) {
		LLInventoryModel::cat_array_t* cats;
		LLInventoryModel::item_array_t* items;

		// retrieve all the objects contained in this folder
		gInventory.getDirectDescendentsOf (cat->getUUID(), cats, items);
		if (items) {
			// wear them one by one
			S32 count = items->count();
			for(S32 i = 0; i < count; ++i) {
				if (!isRoot) {
					LLViewerInventoryItem* item = (LLViewerInventoryItem*)items->get(i);
					if (debug) {
						llinfos << "trying to attach " << item->getName() << llendl;
					}
					
					// this is an object to attach somewhere
					if (item && item->getType() == LLAssetType::AT_OBJECT) {
						LLViewerJointAttachment* attachpt = findAttachmentPointFromName (item->getName());
						
						if (attachpt) {
							if (debug) {
								llinfos << "attaching item to " << attachpt->getName() << llendl;
							}
							// mimick rez_attachment without displaying an Xml alert to confirm
							S32 number = findAttachmentPointNumber (attachpt);
							if (attachpt->getObject()
								&& isAllowed(attachpt->getObject()->getRootEdit()->getID(), "detach")
								&& !contains("detach:"+findAttachmentNameFromPoint(attachpt))
								|| !attachpt->getObject() && !contains("detach:"+findAttachmentNameFromPoint(attachpt)))
							{
								LLAttachmentRezAction* rez_action = new LLAttachmentRezAction;
								rez_action->mItemID = item->getUUID();
								rez_action->mAttachPt = number;
								confirm_replace_attachment_rez(0/*YES*/, (void*)rez_action);
							}
						}
					
					}
					// this is a piece of clothing
					else if (item->getType() == LLAssetType::AT_CLOTHING
							 || item->getType() == LLAssetType::AT_BODYPART) {
						wear_inventory_item_on_avatar (item);
					}
				}
			}
		}
		
		// scan every subfolder of the folder we are attaching, in order to attach no-mod items
		if (cats) {
			// for each subfolder, attach the first item it contains according to its name
			S32 count = cats->count();
			for(S32 i = 0; i < count; ++i) {
				LLViewerInventoryCategory* cat_child = (LLViewerInventoryCategory*)cats->get(i);
				LLViewerJointAttachment* attachpt = findAttachmentPointFromName (cat_child->getName());
				
				if (attachpt && !isRoot) {
					// this subfolder is properly named => attach the first item it contains
					LLInventoryModel::cat_array_t* cats_grandchildren; // won't be used here
					LLInventoryModel::item_array_t* items_grandchildren; // actual no-mod item(s)
					gInventory.getDirectDescendentsOf (cat_child->getUUID(), 
														cats_grandchildren, items_grandchildren);

					if (items_grandchildren && items_grandchildren->count() == 1) {
						LLViewerInventoryItem* item_grandchild = 
								(LLViewerInventoryItem*)items_grandchildren->get(0);

						if (item_grandchild && item_grandchild->getType() == LLAssetType::AT_OBJECT
							&& !item_grandchild->getPermissions().allowModifyBy(gAgent.getID())
							&& findAttachmentPointFromParentName (item_grandchild) != NULL) { // it is no-mod and its parent is named correctly
							// we use the attach point from the name of the folder, not the no-mod item
							// mimick rez_attachment without displaying an Xml alert to confirm
							S32 number = findAttachmentPointNumber (attachpt);
							if (attachpt->getObject()
								&& isAllowed(attachpt->getObject()->getRootEdit()->getID(), "detach")
								&& !contains("detach:"+findAttachmentNameFromPoint(attachpt))
								|| !attachpt->getObject() && !contains("detach:"+findAttachmentNameFromPoint(attachpt)))
							{
								LLAttachmentRezAction* rez_action = new LLAttachmentRezAction;
								rez_action->mItemID = item_grandchild->getUUID();
								rez_action->mAttachPt = number;
								confirm_replace_attachment_rez(0/*YES*/, (void*)rez_action);
							}
							
						}
					}
				}
				if (recursive && cat_child->getName().find (".") != 0) { // attachall and not invisible)
					forceAttach (getFullPath (cat_child), recursive);
				}
			}
		}
	}
	return TRUE;
}

BOOL RRInterface::forceDetachByName (std::string category, BOOL recursive /* = FALSE */)
{
	BOOL debug = gSavedSettings.getBOOL("RestrainedLifeDebug");
	// find the category under RLV shared folder
	LLInventoryCategory* cat = getCategoryUnderRlvShare (category);
	LLVOAvatar* avatar = gAgent.getAvatarObject();
	BOOL isRoot = (getRlvShare() == cat);

	// if exists, detach/unwear all the items inside it
	if (cat) {
		LLInventoryModel::cat_array_t* cats;
		LLInventoryModel::item_array_t* items;
		
		// retrieve all the objects contained in this folder
		gInventory.getDirectDescendentsOf (cat->getUUID(), cats, items);
		if (items) {
			// unwear them one by one
			S32 count = items->count();
			for(S32 i = 0; i < count; ++i) {
				if (!isRoot) {
					LLViewerInventoryItem* item = (LLViewerInventoryItem*)items->get(i);
					if (debug) {
						llinfos << "trying to detach " << item->getName() << llendl;
					}

					// attached object
					if (item->getType() == LLAssetType::AT_OBJECT) {
						// find the attachpoint from which to detach
						for(LLViewerJointAttachment* attachment = avatar->mAttachmentPoints.getFirstData();
							attachment;
							attachment = avatar->mAttachmentPoints.getNextData())
						{
							if (attachment->getObject()) {
								if (attachment->getItemID() == item->getUUID()) {
									handle_detach_from_avatar ((void*)attachment);
									break;
								}
							}
						}
					
					}
					// piece of clothing
					else if (item->getType() == LLAssetType::AT_CLOTHING) {
						LLWearable* layer = gAgent.getWearableFromWearableItem (item->getUUID());
						if (layer != NULL) gAgent.removeWearable (layer->getType());
					}
				}
			}
		}

		if (cats) {
			// for each subfolder, detach the first item it contains (only for single no-mod items contained in appropriately named folders)
			S32 count = cats->count();
			for(S32 i = 0; i < count; ++i) {
				LLViewerInventoryCategory* cat_child = (LLViewerInventoryCategory*)cats->get(i);
				LLInventoryModel::cat_array_t* cats_grandchildren; // won't be used here
				LLInventoryModel::item_array_t* items_grandchildren; // actual no-mod item(s)
				gInventory.getDirectDescendentsOf (cat_child->getUUID(), 
													cats_grandchildren, items_grandchildren);

				if (!isRoot && items_grandchildren && items_grandchildren->count() == 1) { // only one item
					LLViewerInventoryItem* item_grandchild = 
							(LLViewerInventoryItem*)items_grandchildren->get(0);

					if (item_grandchild && item_grandchild->getType() == LLAssetType::AT_OBJECT
						&& !item_grandchild->getPermissions().allowModifyBy(gAgent.getID())
						&& findAttachmentPointFromParentName (item_grandchild) != NULL) { // and it is no-mod and its parent is named correctly
						// detach this object
						// find the attachpoint from which to detach
						for(LLViewerJointAttachment* attachment = avatar->mAttachmentPoints.getFirstData();
							attachment;
							attachment = avatar->mAttachmentPoints.getNextData())
						{
							if (attachment->getObject()) {
								if (attachment->getItemID() == item_grandchild->getUUID()) {
									handle_detach_from_avatar ((void*)attachment);
									break;
								}
							}
						}
					}
				}

				if (recursive && cat_child->getName().find (".") != 0) { // detachall and not invisible)
					forceDetachByName (getFullPath (cat_child), recursive);
				}
			}
		}
	}
	return TRUE;
}

BOOL RRInterface::forceTeleport (std::string location)
{
	BOOL debug = gSavedSettings.getBOOL("RestrainedLifeDebug");
	// location must be X/Y/Z where X, Y and Z are ABSOLUTE coordinates => use a script in-world to translate from local to global
	LLString loc (location);
	std::string region_name;
	S32 x = 128;
	S32 y = 128;
	S32 z = 0;
	std::deque<std::string> tokens=parse (location, "/");
	if (tokens.size()==3) {
		x=atoi (tokens.at(0).c_str());
		y=atoi (tokens.at(1).c_str());
		z=atoi (tokens.at(2).c_str());
	}
	else {
		return FALSE;
	}

	if (debug) {
		llinfos << tokens.at(0) << "," << tokens.at(1) << "," << tokens.at(2) << "     " << x << "," << y << "," << z << llendl;
	}
	LLVector3d pos_global;
	pos_global.mdV[VX] = (F32)x;
	pos_global.mdV[VY] = (F32)y;
	pos_global.mdV[VZ] = (F32)z;

	sAllowCancelTp = FALSE; // will be checked once receiving the tp order from the sim, then set to TRUE again

	gAgent.teleportViaLocation (pos_global);
	return TRUE;
}

std::string RRInterface::stringReplace (std::string s, std::string what, std::string by, BOOL caseSensitive /* = FALSE */)
{
//	llinfos << "trying to replace <" << what << "> in <" << s << "> by <" << by << ">" << llendl;
	if (what == "" || what == " ") return s; // avoid an infinite loop
	int ind;
	int old_ind = 0;
	int len_what = what.length();
	int len_by = by.length();
	if (len_by == 0) len_by = 1; // avoid an infinite loop

	while ((ind = s.find ("%20")) != -1) {	// unescape
		s = s.replace (ind, 3, " ");
	}

	std::string lower = s;
	if (!caseSensitive) {
		LLString::toLower (lower);
		LLString::toLower (what);
	}

	while ((ind = lower.find (what, old_ind)) != -1) {
//		llinfos << "ind=" << ind << "    old_ind=" << old_ind << llendl;
		s = s.replace (ind, len_what, by);
		old_ind = ind + len_by;
		lower = s;
		if (!caseSensitive) LLString::toLower (lower);
	}
	return s;
}

std::string RRInterface::getDummyName (std::string name, EChatAudible audible /* = CHAT_AUDIBLE_FULLY */)
{
	int len = name.length();
	if (len < 2) return ""; // just to avoid crashing in some cases
	unsigned char hash = name.at(3) + len; // very lame hash function I know... but it should be linear enough (the old length method was way too gaussian with a peak at 11 to 16 characters)
	unsigned char mod = hash % 28;
	std::string res = "";
	switch (mod) {
		case 0:		res = "A resident";			break;
		case 1:		res = "This resident";		break;
		case 2:		res = "That resident";		break;
		case 3:		res = "An individual";		break;
		case 4:		res = "This individual";	break;
		case 5:		res = "That individual";	break;
		case 6:		res = "A person";			break;
		case 7:		res = "This person";		break;
		case 8:		res = "That person";		break;
		case 9:		res = "A stranger";			break;
		case 10:	res = "This stranger";		break;
		case 11:	res = "That stranger";		break;
		case 12:	res = "A human being";		break;
		case 13:	res = "This human being";	break;
		case 14:	res = "That human being";	break;
		case 15:	res = "An agent";			break;
		case 16:	res = "This agent";			break;
		case 17:	res = "That agent";			break;
		case 18:	res = "A soul";				break;
		case 19:	res = "This soul";			break;
		case 20:	res = "That soul";			break;
		case 21:	res = "Somebody";			break;
		case 22:	res = "Some people";		break;
		case 23:	res = "Someone";			break;
		case 24:	res = "Mysterious one";		break;
		case 25:	res = "An unknown being";	break;
		case 26:	res = "Unidentified one";	break;
		default:	res = "An unknown person";	break;
	}
	if (audible == CHAT_AUDIBLE_BARELY) res += " afar";
	return res;
}

std::string RRInterface::getCensoredMessage (std::string str)
{
	// Hide every occurrence of the name of anybody around (found in cache, so not completely accurate nor completely immediate)
	S32 i;
	for (i=0; i<gObjectList.getNumObjects(); ++i) {
		LLViewerObject* object = gObjectList.getObject(i);
		if (object && object->isAvatar()) {
			std::string name;
			std::string dummy_name;
			char firstname[DB_FIRST_NAME_BUF_SIZE];	/*Flawfinder: ignore*/
			char lastname[DB_LAST_NAME_BUF_SIZE];	/*Flawfinder: ignore*/
			BOOL res = gCacheName->getName(object->getID(), firstname, lastname);
			if (res) {
				name.assign(firstname);
				name.append(" ");
				name.append(lastname);
				dummy_name = getDummyName (name);
				str = stringReplace (str, name, dummy_name); // full name first
//				str = stringReplace (str, std::string(firstname), dummy_name); // first name
//				str = stringReplace (str, std::string(lastname), dummy_name); // last name
			}
		}
	}
	return str;
}

BOOL RRInterface::forceEnvironment (std::string command, std::string option) {
	// command is "setenv_<something>"
	double val = atof (option.c_str());

	int length = 7; // size of "setenv_"
	command = command.substr (length);

	if (command == "daytime") {
		if (val >= 0.0f) {
			LLVector3 sun_direction;
			if (val < 0.125f || val >= 0.875f) {
				// midnight
				sun_direction.setVec(0.0f, 0.3f, -1.0f);
			} else if (val >= 0.125f && val < 0.4875f) {
				// sunrise
				sun_direction.setVec(1.0f, 0.f, 0.2f);
			} else if (val >= 0.4875f && val < 0.625) {
				// noon
				sun_direction.setVec(0.0f, 0.3f, 1.0f);
			} else {
				// sunset
				sun_direction.setVec(-1.0f, 0.f, 0.2f);
			}
			gSky.setOverrideSun(TRUE);
			gSky.setSunDirection(sun_direction, LLVector3(0.f, 0.f, 0.f));
		} else {
			gSky.setOverrideSun(FALSE);
		}
	}

	return TRUE;
}

std::string RRInterface::getEnvironment (std::string command)
{
	F64 res = 0;
	int length = 7; // size of "getenv_"
	command = command.substr (length);

	if (command == "daytime") {
		if (gSky.getOverrideSun()) {
			res = gSky.getSunDirection().mV[2];
			if (res >= 0.19 && res <= 0.21) {
				if (gSky.getSunDirection().mV[0] > 0) {
					res = 0.25;
				} else {
					res = 0.75;
				}
			} else {
				if (res > 0) {
					res = 0.567;
				} else {
					res = 0.0;
				}
			}
		} else {
			res = -1.0;
		}
	}

	std::stringstream str;
	str << res;
	return str.str();
}

BOOL RRInterface::forceDebugSetting (std::string command, std::string option)
{
	//	MK: As some debug settings are critical to the user's experience and others
	//	are just useless/not used, we are following a whitelist approach : only allow
	//	certain debug settings to be changed and not all.

	// command is "setdebug_<something>"

	int length = 9; // size of "setdebug_"
	command = command.substr (length);
	LLString::toLower(command);
	std::string allowed;
	std::string tmp;
	int ind;

	allowed = sAllowedU32;
	tmp = allowed;
	LLString::toLower(tmp);
	if ((ind = tmp.find (","+command+",")) != -1) {
		gSavedSettings.setU32 (allowed.substr(++ind, command.length()), atoi(option.c_str()));
		return TRUE;
	}

	return TRUE;
}

std::string RRInterface::getDebugSetting (std::string command)
{
	std::stringstream res;
	int length = 9; // size of "getdebug_"
	command = command.substr (length);
	LLString::toLower(command);
	std::string allowed;
	std::string tmp;
	int ind;

	allowed = sAllowedU32;
	tmp = allowed;
	LLString::toLower(tmp);
	if ((ind = tmp.find (","+command+",")) != -1) {
		res << gSavedSettings.getU32 (allowed.substr(++ind, command.length()));
	}

	return res.str();
}

std::string RRInterface::getFullPath (LLInventoryCategory* cat)
{
	if (cat == NULL) return "";
	LLInventoryCategory* rlv = gAgent.mRRInterface.getRlvShare();
	if (rlv == NULL) return "";
	LLInventoryCategory* res = cat;
	std::deque<std::string> tokens;

	while (res && res != rlv) {
		tokens.push_front (res->getName());
		const LLUUID& parent_id = res->getParentUUID();
		res = gInventory.getCategory (parent_id);
	}
	return dumpList2String (tokens, "/");
}

std::string RRInterface::getFullPath (LLInventoryItem* item, std::string option)
{
	// Returns the path from the shared root to this object, or to the object worn at the attach point or clothing layer pointed by option if any
	if (option != "") {
		item = NULL; // an option is specified => we don't want to check the item that issued the command, but something else that is currently worn (object or clothing)

		EWearableType wearable_type = gAgent.mRRInterface.getOutfitLayerAsType (option);
		if (wearable_type != WT_INVALID) { // this is a clothing layer => replace item with the piece clothing
			LLUUID id = gAgent.getWearableItem (wearable_type);
			if (id.notNull()) item = gInventory.getItem(id);
			if (item != NULL && !gAgent.mRRInterface.isUnderRlvShare(item)) item = NULL; // security : we would return the path even if the item was not shared otherwise
		}
		else { // this is not a clothing layer => it has to be an attachment point
			LLViewerJointAttachment* attach_point = gAgent.mRRInterface.findAttachmentPointFromName (option, TRUE);
			if (attach_point) {
				LLViewerObject* attached_object = attach_point->getObject();
				item = getItemAux (attached_object, gAgent.mRRInterface.getRlvShare());
				if (item != NULL && !gAgent.mRRInterface.isUnderRlvShare(item)) item = NULL; // security : we would return the path even if the item was not shared otherwise
			}
		}
	}

	if (item != NULL && !gAgent.mRRInterface.isUnderRlvShare(item)) item = NULL; // security : we would return the path even if the item was not shared otherwise
	if (item == NULL) return "";
	LLUUID parent_id = item->getParentUUID();
	LLInventoryCategory* parent_cat = gInventory.getCategory (parent_id);

	if (item->getType() == LLAssetType::AT_OBJECT && !item->getPermissions().allowModifyBy(gAgent.getID())) {
		if (gAgent.mRRInterface.findAttachmentPointFromName(parent_cat->getName()) != NULL) {
			// this item is no-mod and its parent folder contains the name of an attach point
			// => probably we want the full path only to the containing folder of that folder
			parent_id = parent_cat->getParentUUID();
			parent_cat = gInventory.getCategory (parent_id);
			return getFullPath (parent_cat);
		}
	}

	return getFullPath (parent_cat);
}


LLInventoryItem* RRInterface::getItemAux (LLViewerObject* attached_object, LLInventoryCategory* root)
{
	// auxiliary function for getItem()
	if (!attached_object) return NULL;
	LLVOAvatar* avatar = gAgent.getAvatarObject();
	if (root && avatar) {
		LLInventoryModel::cat_array_t* cats;
		LLInventoryModel::item_array_t* items;
		gInventory.getDirectDescendentsOf (root->getUUID(), cats, items);
		S32 count;
		S32 i;
		LLInventoryItem* item = NULL;
		LLInventoryCategory* cat = NULL;

		// Try to find the item in the current category
		count = items->count();
		for(i = 0; i < count; ++i) {
			item = items->get(i);
			if (item 
				&& (item->getType() == LLAssetType::AT_OBJECT || item->getType() == LLAssetType::AT_CLOTHING)
				&& avatar->getWornAttachment (item->getUUID()) == attached_object
				) {
				// found the item in the current category
				return item;
			}
		}

		// We didn't find it here => browse the children categories
		count = cats->count();
		for(i = 0; i < count; ++i) {
			cat = cats->get(i);
			item = getItemAux (attached_object, cat);
			if (item != NULL) return item;
		}
	}
	// We didn't find it (this should not happen)
	return NULL;
}

LLInventoryItem* RRInterface::getItem (LLUUID wornObjectUuidInWorld)
{
	// return the inventory item corresponding to the viewer object which UUID is "wornObjectUuidInWorld", if any
	LLViewerObject* object = gObjectList.findObject (wornObjectUuidInWorld);
	if (object != NULL) {
		object = object->getRootEdit();
		if (object->isAttachment()) {
			return getItemAux (object, gInventory.getCategory(gInventory.findCategoryUUIDForType(LLAssetType::AT_CATEGORY))); //gAgent.mRRInterface.getRlvShare());
		}
	}
	// This object is not worn => it has nothing to do with any inventory item
	return NULL;
}

void RRInterface::attachObjectByUUID (LLUUID assetUUID, int attachPtNumber)
{
	// caution : this method does NOT check that the target attach point is already used by a locked item
	LLAttachmentRezAction* rez_action = new LLAttachmentRezAction;
	rez_action->mItemID = assetUUID;
	rez_action->mAttachPt = attachPtNumber;
	confirm_replace_attachment_rez(0/*YES*/, (void*)rez_action);
}

bool RRInterface::canDetachAllSelectedObjects ()
{
	for (LLObjectSelection::iterator iter = gSelectMgr->getSelection()->begin();
		 iter != gSelectMgr->getSelection()->end(); )
	{
		LLObjectSelection::iterator curiter = iter++;
		LLViewerObject* object = (*curiter)->getObject();
		if (object && !gAgent.mRRInterface.isAllowed (object->getRootEdit()->getID(), "detach"))
		{
			return false;
		}
								
		LLVOAvatar *avatarp = gAgent.getAvatarObject();
		LLUUID inv_item_id = LLUUID::null;
		LLInventoryItem* inv_item = gAgent.mRRInterface.getItem(object->getRootEdit()->getID());
		if (inv_item) inv_item_id = inv_item->getUUID();
		if (avatarp && contains("detach:"+avatarp->getAttachedPointName(inv_item_id)))
		{
			return false;
		}	
	}
	return true;
}

bool RRInterface::isSittingOnAnySelectedObject()
{
	if (gAgent.getAvatarObject() && !gAgent.getAvatarObject()->mIsSitting) {
		return false;
	}

	for (LLObjectSelection::iterator iter = gSelectMgr->getSelection()->begin();
		 iter != gSelectMgr->getSelection()->end(); )
	{
		LLObjectSelection::iterator curiter = iter++;
		LLViewerObject* object = (*curiter)->getObject();
		if (object && object->isSeat())
		{
			return true;
		}
	}
	return false;
}
