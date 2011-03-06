/** 
 * @file llmotioncontroller.h
 * @brief Implementation of LLMotionController class.
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

#ifndef LL_LLMOTIONCONTROLLER_H
#define LL_LLMOTIONCONTROLLER_H

//-----------------------------------------------------------------------------
// Header files
//-----------------------------------------------------------------------------
#include <string>
#include <map>
#include <deque>

#include "lluuidhashmap.h"
#include "llmotion.h"
#include "llpose.h"
#include "llframetimer.h"
#include "llstatemachine.h"
#include "llstring.h"

//-----------------------------------------------------------------------------
// Class predeclaration
// This is necessary because llcharacter.h includes this file.
//-----------------------------------------------------------------------------
class LLCharacter;

//-----------------------------------------------------------------------------
// LLMotionRegistry
//-----------------------------------------------------------------------------
typedef LLMotion*(*LLMotionConstructor)(const LLUUID &id);

class LLMotionTableEntry
{
public:
	LLMotionTableEntry();
	LLMotionTableEntry(LLMotionConstructor constructor, const LLUUID& id);
	~LLMotionTableEntry(){};

	LLMotion* create(const LLUUID& id);
	static BOOL uuidEq(const LLUUID &uuid, const LLMotionTableEntry &id_pair)
	{
		if (uuid == id_pair.mID)
		{
			return TRUE;
		}
		return FALSE;
	}

	const LLUUID& getID() { return mID; }

protected:
	LLMotionConstructor		mConstructor;
	LLUUID	mID;
};

class LLMotionRegistry
{
public:
	// Constructor
	LLMotionRegistry();

	// Destructor
	~LLMotionRegistry();

	// adds motion classes to the registry
	// returns true if successfull
	BOOL addMotion( const LLUUID& id, LLMotionConstructor create);

	// creates a new instance of a named motion
	// returns NULL motion is not registered
	LLMotion *createMotion( const LLUUID &id );

	// initialization of motion failed, don't try to create this motion again
	void markBad( const LLUUID& id );


protected:
	LLUUIDHashMap<LLMotionTableEntry, 32>	mMotionTable;
};

//-----------------------------------------------------------------------------
// class LLMotionController
//-----------------------------------------------------------------------------
class LLMotionController
{
public:
	typedef std::list<LLMotion*> motion_list_t;
	typedef std::set<LLMotion*> motion_set_t;
	
public:
	// Constructor
	LLMotionController();

	// Destructor
	virtual ~LLMotionController();

	// set associated character
	// this must be called exactly once by the containing character class.
	// this is generally done in the Character constructor
	void setCharacter( LLCharacter *character );

	// registers a motion with the controller
	// (actually just forwards call to motion registry)
	// returns true if successfull
	BOOL addMotion( const LLUUID& id, LLMotionConstructor create );

	// creates a motion from the registry
	LLMotion *createMotion( const LLUUID &id );

	// unregisters a motion with the controller
	// (actually just forwards call to motion registry)
	// returns true if successfull
	void removeMotion( const LLUUID& id );

	// start motion
	// begins playing the specified motion
	// returns true if successful
	BOOL startMotion( const LLUUID &id, F32 start_offset );

	// stop motion
	// stops a playing motion
	// in reality, it begins the ease out transition phase
	// returns true if successful
	BOOL stopMotionLocally( const LLUUID &id, BOOL stop_immediate );

	// update motions
	// invokes the update handlers for each active motion
	// activates sequenced motions
	// deactivates terminated motions`
	void updateMotion();

	// flush motions
	// releases all motion instances
	void flushAllMotions();

	//Flush is a liar.
	void deactivateAllMotions();	

	// pause and continue all motions
	void pause();
	void unpause();
	BOOL isPaused() { return mPaused; }

	void setTimeStep(F32 step);

	void setTimeFactor(F32 time_factor);
	F32 getTimeFactor() { return mTimeFactor; }

	motion_list_t& getActiveMotions() { return mActiveMotions; }

//protected:
	bool isMotionActive( LLMotion *motion );
	bool isMotionLoading( LLMotion *motion );
	LLMotion *findMotion( const LLUUID& id );

protected:
	void deleteAllMotions();
	void addLoadedMotion(LLMotion *motion);
	BOOL activateMotion(LLMotion *motion, F32 time);
	BOOL deactivateMotion(LLMotion *motion, bool remove_weight);
	void updateRegularMotions();
	void updateAdditiveMotions();
	void resetJointSignatures();
	void updateMotionsByType(LLMotion::LLMotionBlendType motion_type);
protected:

	F32					mTimeFactor;
	static LLMotionRegistry	sRegistry;
	LLPoseBlender		mPoseBlender;

	LLCharacter			*mCharacter;

//	Life cycle of an animation:
//
//	Animations are instantiated and immediately put in the mAllMotions map for their entire lifetime.
//	If the animations depend on any asset data, the appropriate data is fetched from the data server,
//	and the animation is put on the mLoadingMotions list.
//	Once an animations is loaded, it will be initialized and put on the mLoadedMotions deque.
//	Any animation that is currently playing also sits in the mActiveMotions list.

	std::map<LLUUID, LLMotion*>	mAllMotions;

	motion_set_t		mLoadingMotions;
	motion_list_t		mLoadedMotions;
	motion_list_t		mActiveMotions;
	
	LLFrameTimer		mTimer;
	F32					mTime;
	F32					mTimeOffset;
	F32					mLastTime;
	BOOL				mHasRunOnce;
	BOOL				mPaused;
	F32					mTimeStep;
	S32					mTimeStepCount;
	F32					mLastInterp;
	F32					mPauseTime;

	U8					mJointSignature[2][LL_CHARACTER_MAX_JOINTS];
};

//-----------------------------------------------------------------------------
// Class declaractions
//-----------------------------------------------------------------------------
#include "llcharacter.h"

#endif // LL_LLMOTIONCONTROLLER_H

