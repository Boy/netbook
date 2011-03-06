/** 
 * @file llviewernetwork.cpp
 * @author James Cook, Richard Nelson
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

#include "llviewerprecompiledheaders.h"

#include "llviewernetwork.h"

LLGridData gGridInfo[GRID_INFO_COUNT] = 
{
	{ "None", "", "", ""},
	{ "SecondLife Beta", 
	  "util.aditi.lindenlab.com", 
	  "https://login.aditi.lindenlab.com/cgi-bin/login.cgi",
	  "http://aditi-secondlife.webdev.lindenlab.com/helpers/" },
	{ "SecondLife", 
	  "util.agni.lindenlab.com", 
	  "https://login.agni.lindenlab.com/cgi-bin/login.cgi",
	  "https://secondlife.com/helpers/" },
	{ "OSGrid",
	  "osgrid.org",
	  "http://osgrid.org:8002/",
	  "http://osgrid.org/" },
	{ "DeepGrid",
	  "deepgrid.com",
	  "http://login.deepgrid.com:8002/",
	  "" },
	{ "Emerald Network",
	  "emerald.webhop.net",
	  "http://emerald.webhop.net:8002/",
	  "" },
	{ "FrancoGrid",
	  "francogrid.org",
	  "http://grid.francogrid.com:8002/",
	  "" },
	{ "Le Monde de Darwin",
	  "lemondededarwin.com",
	  "http://94.23.8.158:8002/",
	  "" },
	{ "EU-Grid",
	  "eu-grid.org",
	  "http://eu-grid.org:8002/",
	  "" },
	{ "GiantGrid", 
	  "giantgrid.no-ip.biz",
	  "http://Gianttest.no-ip.biz:8002/",
	  "http://gianttest.no-ip.biz/giantmap/" },
	{ "The New World Grid",
	  "newworldgrid.com",
	  "http://grid.newworldgrid.com:8002/",
	  "http://account.newworldgrid.com/" },
	{ "3rd Rock Grid",
	  "3rdrockgrid.com",
	  "http://grid.3rdrockgrid.com:8002/",
	  "http://3rdrockgrid.com/main/rg_files/wr/" },
	{ "Legend City Online",
	  "legendcityonline.com",
	  "http://login.legendcityonline.com:8002/",
	  "https://secure.legendcityonline.com/" },
	{ "Uvatar",
	  "uvatar.com",
	  "http://uvatar.com:8002/",
	  "" },
	{ "Grid Splash",
	  "gridsplash.com",
	  "http://grid.gridsplash.com:8002/",
	  "" },
	{ "World Sim Terra",
	  "wsterra.com",
	  "http://wsterra.com:8002/",
	  "http://wsterra.com/" },
	{ "Cyberlandia",
	  "cyberlandia.net",
	  "http://grid.cyberlandia.net:8002/",
	  "http://grid.cyberlandia.net/" },
	{ "Tribal Net",
	  "tribalnet.se",
	  "http://login.tribalnet.se/",
	  "" },
	{ "Metropolis",
	  "hypergrid.org",
	  "http://hypergrid.org:8002/",
	  "http://metropolis.hypergrid.org/oswi.php" },
	{ "AUGrid",
	  "augrid.org",
	  "http://augrid.org:8002/",
	  "http://augrid.org/" },
	{ "Avatar Hangout",
	  "avatarhangout.com",
	  "http://login.avatarhangout.com:8002/",
	  "http://avatarhangout.com/" },
	{ "German Grid",
	  "germangrid.eu",
	  "http://germangrid.eu:8002/",
	  "http://germangrid.eu/" },
	{ "DGP Grid",
	  "dgpgrid.selfip.org",
	  "http://dgpgrid.selfip.org:8002/",
	  "" },
	{ "LifeSim",
	  "lifesim.com.br",
	  "http://216.40.235.66:8002/",
	  "http://www.lifesim.com.br/" },
	{ "Fantasy World Estates",
	  "fw-estates.com",
	  "http://maingrid.fw-estates.com:8002/",
	  "http://grid006.fw-estates.com/" },
	{ "RMH Design Estate",
	  "rmhdesignestate.com",
	  "http://maingrid.rmhdesignestate.com:8002/",
	  "http://grid006.rmhdesignestate.com/" },
	{ "ReactionGrid",
	  "gsquared.info",
	  "http://gsquared.info:8008/",
	  "" },
	{ "A Future Life",
	  "afuturelife.com",
	  "http://66.240.233.142/",
	  "" },
	{ "Pseudospace",
	  "pseudospace.net",
	  "http://grid.pseudospace.net:8002/",
	  "http://www.pseudospace.net/" },
	{ "YourGrid",
	  "yourgrid.de",
	  "http://yourgrid.de:8002/",
	  "" },
	{ "MyOpenGrid",
	  "myopengrid.com",
	  "http://www.myopengrid.com:8002/",
	  "http://www.myopengrid.com/" },
	{ "PhoenixSim",
	  "phoenix-community.net",
	  "http://ps.phoenix-community.net:8002/",
	  "http://ps.phoenix-community.net/os/" },
	{ "Virtual Sims",
	  "virtualsims.net",
	  "http://virtualsims.net:8002/",
	  "http://66.71.246.212/" },
	{ "Grid4Us",
	  "grid4us.net",
	  "http://grid4us.net:8002/",
	  "http://grid4us.net/" },
	{ "OpenKansai",
	  "os.taf-jp.com",
	  "http://os.taf-jp.com:8002/",
	  "" },
	{ "Schweiz",
	  "2lifegrid.game-host.org",
	  "http://2lifegrid.game-host.org:8002/",
	  "" },
	{ "Dreamland Grid",
	  "dreamlandgrid.com",
	  "http://dreamlandgrid.com:8002/",
	  "" },
	{ "Your Alternative Life",
	  "tiog.ath.cx",
	  "http://tiog.ath.cx:8002/",
	  "http://grid01.from-ne.com/tios/services/" },
	{ "ScienceSim",
	  "sciencesim.com",
	  "http://island.sciencesim.com:8002/",
	  "http://sciencesim.com/scisim/" },
	{ "Unica",
	  "unica.it",
	  "http://grid.unica.it:9000/",
	  "" },
	{ "The Gor Grid",
	  "thegorgrid.com",
	  "http://thegorgrid.com:8002/",
	  "http://thegorgrid.com/" },
	{ "Cuon Grid",
	  "sim-linuxmain.org",
	  "http://sim-linuxmain.org:8002/",
	  "" },
	{ "ArtgridOnLine",
	  "artgridonline.com",
	  "http://artgridonline.com:8002/",
	  "" },
	{ "Local", 
	  "localhost", 
	  "https://login.dmz.lindenlab.com/cgi-bin/login.cgi",
	  "" },
	{ "Other", 
	  "", 
	  "https://login.dmz.lindenlab.com/cgi-bin/login.cgi",
	  "" }
};

// Use this to figure out which domain name and login URI to use.

EGridInfo gGridChoice = GRID_INFO_NONE;
char gGridName[MAX_STRING];		/* Flawfinder: ignore */

F32 gPacketDropPercentage = 0.f;
F32 gInBandwidth = 0.f;
F32 gOutBandwidth = 0.f;

unsigned char gMACAddress[MAC_ADDRESS_BYTES];		/* Flawfinder: ignore */
