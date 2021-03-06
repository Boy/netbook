#ifndef LL_RRINTERFACE_H
#define LL_RRINTERFACE_H

#define RR_PREFIX "@"
#define RR_VERSION "1.20.2 (RV 1.19.0.5)"

#define RR_SHARED_FOLDER "#RLV"
#define RR_RLV_REDIR_FOLDER_PREFIX "#RLV/~"
// Length of the "#RLV/" string constant in characters.
#define RR_HRLVS_LENGTH 5

// wearable types as strings
#define WS_ALL "all"
#define WS_EYES "eyes"
#define WS_SKIN "skin"
#define WS_SHAPE "shape"
#define WS_HAIR "hair"
#define WS_GLOVES "gloves"
#define WS_JACKET "jacket"
#define WS_PANTS "pants"
#define WS_SHIRT "shirt"
#define WS_SHOES "shoes"
#define WS_SKIRT "skirt"
#define WS_SOCKS "socks"
#define WS_UNDERPANTS "underpants"
#define WS_UNDERSHIRT "undershirt"


//#include <set>
#include <deque>
#include <map>
#include <string>

#include "lluuid.h"
#include "llchatbar.h"
#include "llinventorymodel.h"
#include "llviewermenu.h"
#include "llwearable.h"

typedef std::multimap<std::string, std::string> RRMAP;
typedef struct Command {
	LLUUID uuid;
	std::string command;
} Command;

class RRInterface
{
public:
	
	RRInterface ();
	~RRInterface ();

	std::string getVersion ();
	BOOL isAllowed (LLUUID object_uuid, std::string action, BOOL log_it = TRUE);
	BOOL contains (std::string action);
	BOOL containsSubstr (std::string action);

	BOOL add (LLUUID object_uuid, std::string action, std::string option);
	BOOL remove (LLUUID object_uuid, std::string action, std::string option);
	BOOL clear (LLUUID object_uuid, std::string command="");
	BOOL garbageCollector (BOOL all=TRUE); // if false, don't clear rules attached to NULL_KEY as they are issued from external objects (only cleared when changing parcel)
	std::deque<std::string> parse (std::string str, std::string sep); // utility function
	void notify (LLUUID object_uuid, std::string action, std::string suffix); // scan the list of restrictions, when finding "notify" say the action on the specified channel

	BOOL parseCommand (std::string command, std::string& behaviour, std::string& option, std::string& param);
	BOOL handleCommand (LLUUID uuid, std::string command);
	BOOL fireCommands (); // execute commands buffered while the viewer was initializing (mostly useful for force-sit as when the command is sent the object is not necessarily rezzed yet)
	BOOL force (LLUUID object_uuid, std::string command, std::string option);

	BOOL answerOnChat (std::string channel, std::string msg);
	std::string crunchEmote (std::string msg, unsigned int truncateTo);

	std::string getOutfitLayerAsString (EWearableType layer);
	EWearableType getOutfitLayerAsType (std::string layer);
	std::string getOutfit (std::string layer);
	std::string getAttachments (std::string attachpt);

	std::string getStatus (LLUUID object_uuid, std::string rule); // if object_uuid is null, return all
	BOOL forceDetach (std::string attachpt);
	BOOL forceDetachByUuid (std::string object_uuid);

	BOOL hasLockedHuds ();
	std::string getInventoryList (std::string path, BOOL withWornInfo = FALSE);
	std::string getWornItems (LLInventoryCategory* cat);
	LLInventoryCategory* getRlvShare (); // return pointer to #RLV folder or null if does not exist
	BOOL isUnderRlvShare (LLInventoryItem* item);
	void renameAttachment (LLInventoryItem* item, LLViewerJointAttachment* attachment); // DEPRECATED
	LLInventoryCategory* getCategoryUnderRlvShare (std::string catName, LLInventoryCategory* root = NULL);
	LLInventoryCategory* findCategoryUnderRlvShare (std::string catName, LLInventoryCategory* root = NULL);
	std::string findAttachmentNameFromPoint (LLViewerJointAttachment* attachpt);
	LLViewerJointAttachment* findAttachmentPointFromName (std::string objectName, BOOL exactName = FALSE);
	LLViewerJointAttachment* findAttachmentPointFromParentName (LLInventoryItem* item);
	S32 findAttachmentPointNumber (LLViewerJointAttachment* attachment);
	void fetchInventory (LLInventoryCategory* root = NULL);

	BOOL forceAttach (std::string category, BOOL recursive = FALSE);
	BOOL forceDetachByName (std::string category, BOOL recursive = FALSE);

	BOOL getAllowCancelTp() { return sAllowCancelTp; }
	void setAllowCancelTp(BOOL newval) { sAllowCancelTp = newval; }

	std::string getParcelName () { return sParcelName; }
	void setParcelName (std::string newval) { sParcelName = newval; }

	BOOL forceTeleport (std::string location);

	std::string stringReplace (std::string s, std::string what, std::string by, BOOL caseSensitive = FALSE);

	std::string getDummyName (std::string name, EChatAudible audible = CHAT_AUDIBLE_FULLY); // return "someone", "unknown" etc according to the length of the name (when shownames is on)
	std::string getCensoredMessage (std::string str); // replace names by dummy names

	LLUUID getSitTargetId () { return sSitTargetId; }
	void setSitTargetId (LLUUID newval) { sSitTargetId = newval; }

	BOOL forceEnvironment (std::string command, std::string option); // command is "setenv_<something>", option is a list of floats (separated by "/")
	std::string getEnvironment (std::string command); // command is "getenv_<something>"
	
	BOOL forceDebugSetting (std::string command, std::string option); // command is "setdebug_<something>", option is a list of values (separated by "/")
	std::string getDebugSetting (std::string command); // command is "getdebug_<something>"

	std::string getFullPath (LLInventoryCategory* cat);
	std::string getFullPath (LLInventoryItem* item, std::string option = "");
	LLInventoryItem* getItemAux (LLViewerObject* attached_object, LLInventoryCategory* root);
	LLInventoryItem* getItem (LLUUID wornObjectUuidInWorld);
	void attachObjectByUUID (LLUUID assetUUID, int attachPtNumber = 0);
	
	bool canDetachAllSelectedObjects();
	bool isSittingOnAnySelectedObject();

	// Some cache variables to accelerate common checks
	BOOL mHasLockedHuds;
	BOOL mContainsDetach;
	BOOL mContainsShowinv;
	BOOL mContainsUnsit;
	BOOL mContainsFartouch;
	BOOL mContainsShowworldmap;
	BOOL mContainsShowminimap;
	BOOL mContainsShowloc;
	BOOL mContainsShownames;
	BOOL mContainsSetenv;
	BOOL mContainsFly;
	BOOL mContainsEdit;
	BOOL mContainsRez;
	BOOL mContainsShowhovertextall;
	BOOL mContainsShowhovertexthud;
	BOOL mContainsShowhovertextworld;

	// Allowed debug settings (initialized in the ctor)
	std::string sAllowedU32;
	std::string sAllowedS32;
	std::string sAllowedF32;
	std::string sAllowedBOOLEAN;
	std::string sAllowedSTRING;
	std::string sAllowedVEC3;
	std::string sAllowedVEC3D;
	std::string sAllowedRECT;
	std::string sAllowedCOL4;
	std::string sAllowedCOL3;
	std::string sAllowedCOL4U;

	// These should be private but we may want to browse them from the outside world, so let's keep them public
	RRMAP sSpecialObjectBehaviours;
	std::deque<Command> sRetainedCommands;

	// When a locked attachment is kicked off by another one with llAttachToAvatar() in a script, retain its UUID here, to reattach it later 
	LLUUID sAssetToReattach;
	int sTimeBeforeReattaching;

private:
	std::string sParcelName; // for convenience (gAgent does not retain the name of the current parcel)
	BOOL sInventoryFetched; // FALSE at first, used to fetch RL Share inventory once upon login
	BOOL sAllowCancelTp; // TRUE unless forced to TP with @tpto (=> receive TP order from server, act like it is a lure from a Linden => don't show the cancel button)
	LLUUID sSitTargetId;
};


#endif
