/** 
 * @file llviewernetwork.h
 * @author James Cook
 * @brief Networking constants and globals for viewer.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2008, Linden Research, Inc.
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

#ifndef LL_LLVIEWERNETWORK_H
#define LL_LLVIEWERNETWORK_H

class LLHost;

enum EGridInfo
{
	GRID_INFO_NONE,
	GRID_INFO_ADITI,
	GRID_INFO_AGNI,
	GRID_INFO_OSGRID,
	GRID_INFO_DEEPGRID,
	GRID_INFO_EMERALD,
	GRID_INFO_FRANCOGRID,
	GRID_INFO_MONDEDARWIN,
	GRID_INFO_EUGRID,
	GRID_INFO_GIANTGRID,
	GRID_INFO_NEWWORLD,
	GRID_INFO_3RDROCK,
	GRID_INFO_LEGENDCITY,
	GRID_INFO_UVATAR,
	GRID_INFO_GRIDSPLASH,
	GRID_INFO_WORLDSIMTERRA,
	GRID_INFO_CYBERLANDIA,
	GRID_INFO_TRIBALNET,
	GRID_INFO_METROPOLIS,
	GRID_INFO_AUGRID,
	GRID_INFO_AVATARHANGOUT,
	GRID_INFO_GERMANGRID,
	GRID_INFO_DGPGRID,
	GRID_INFO_LIFESIM,
	GRID_INFO_FANTASYWORLD,
	GRID_INFO_RMHDESIGN,
	GRID_INFO_REACTIONGRID,
	GRID_INFO_FUTURELIFE,
	GRID_INFO_PSEUDOSPACE,
	GRID_INFO_YOURGRID,
	GRID_INFO_MYOPENGRID,
	GRID_INFO_PHOENIXSIM,
	GRID_INFO_VIRTUALSIMS,
	GRID_INFO_GRID4US,
	GRID_INFO_OPENKANSAI,
	GRID_INFO_SCHWEIZ,
	GRID_INFO_DREAMLAND,
	GRID_INFO_ALTLIFE,
	GRID_INFO_SCIENCESIM,
	GRID_INFO_UNICA,
	GRID_INFO_GORGRID,
	GRID_INFO_CUONGRID,
	GRID_INFO_ARTGRID,
	GRID_INFO_LOCAL,
	GRID_INFO_OTHER, // IP address set via -user or other command line option
	GRID_INFO_COUNT
};


struct LLGridData
{
	const char* mLabel;
	const char* mName;
	const char* mLoginURI;
	const char* mHelperURI;
};

extern F32 gPacketDropPercentage;
extern F32 gInBandwidth;
extern F32 gOutBandwidth;
extern EGridInfo gGridChoice;
extern LLGridData gGridInfo[];
extern char gGridName[MAX_STRING];		/* Flawfinder: ignore */

const S32 MAC_ADDRESS_BYTES = 6;
extern unsigned char gMACAddress[MAC_ADDRESS_BYTES];		/* Flawfinder: ignore */

#endif
