/** 
 * @file llfeaturemanager.cpp
 * @brief LLFeatureManager class implementation
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2008, Linden Research, Inc.
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

#include <iostream>
#include <fstream>

#include "llviewerprecompiledheaders.h"

#include "llfeaturemanager.h"
#include "lldir.h"

#include "llsys.h"
#include "llgl.h"
#include "llsecondlifeurls.h"

#include "llviewercontrol.h"
#include "llworld.h"
#include "pipeline.h"
#include "lldrawpoolterrain.h"
#include "llviewerimagelist.h"
#include "llwindow.h"
#include "llui.h"

#if LL_WINDOWS
#include "lldxhardware.h"
#endif

//
// externs
//
extern LLMemoryInfo gSysMemory;
extern LLCPUInfo gSysCPU;

#if LL_DARWIN
const char FEATURE_TABLE_FILENAME[] = "featuretable_mac.txt";
#elif LL_LINUX
const char FEATURE_TABLE_FILENAME[] = "featuretable_linux.txt";
#elif LL_SOLARIS
const char FEATURE_TABLE_FILENAME[] = "featuretable_solaris.txt";
#else
const char FEATURE_TABLE_FILENAME[] = "featuretable.txt";
#endif

const char GPU_TABLE_FILENAME[] = "gpu_table.txt";

LLFeatureManager *gFeatureManagerp = NULL;

LLFeatureInfo::LLFeatureInfo(const char *name, const BOOL available, const S32 level) : mValid(TRUE)
{
	mName = name;
	mAvailable = available;
	mRecommendedLevel = level;
}

LLFeatureList::LLFeatureList(const char *name)
{
	mName = name;
}

LLFeatureList::~LLFeatureList()
{
}

void LLFeatureList::addFeature(const char *name, const BOOL available, const S32 level)
{
	if (mFeatures.count(name))
	{
		llwarns << "LLFeatureList::Attempting to add preexisting feature " << name << llendl;
	}

	LLFeatureInfo fi(name, available, level);
	mFeatures[name] = fi;
}

BOOL LLFeatureList::isFeatureAvailable(const char *name)
{
	if (mFeatures.count(name))
	{
		return mFeatures[name].mAvailable;
	}

	llwarns << "Feature " << name << " not on feature list!" << llendl;
	return FALSE;
}

S32 LLFeatureList::getRecommendedLevel(const char *name)
{
	if (mFeatures.count(name))
	{
		return mFeatures[name].mRecommendedLevel;
	}

	llwarns << "Feature " << name << " not on feature list!" << llendl;
	return -1;
}

BOOL LLFeatureList::maskList(LLFeatureList &mask)
{
	//llinfos << "Masking with " << mask.mName << llendl;
	//
	// Lookup the specified feature mask, and overlay it on top of the
	// current feature mask.
	//

	LLFeatureInfo mask_fi;

	feature_map_t::iterator feature_it;
	for (feature_it = mask.mFeatures.begin(); feature_it != mask.mFeatures.end(); ++feature_it)
	{
		mask_fi = feature_it->second;
		//
		// Look for the corresponding feature
		//
		if (!mFeatures.count(mask_fi.mName))
		{
			llwarns << "Feature " << mask_fi.mName << " in mask not in top level!" << llendl;
			continue;
		}

		LLFeatureInfo &cur_fi = mFeatures[mask_fi.mName];
		if (mask_fi.mAvailable && !cur_fi.mAvailable)
		{
			llwarns << "Mask attempting to reenabling disabled feature, ignoring " << cur_fi.mName << llendl;
			continue;
		}
		cur_fi.mAvailable = mask_fi.mAvailable;
		cur_fi.mRecommendedLevel = llmin(cur_fi.mRecommendedLevel, mask_fi.mRecommendedLevel);
#ifndef LL_RELEASE_FOR_DOWNLOAD
		llinfos << "Feature mask " << mask.mName
				<< " Feature " << mask_fi.mName
				<< " Mask: " << mask_fi.mRecommendedLevel
				<< " Now: " << cur_fi.mRecommendedLevel << llendl;
#endif
	}

#if 0 && !LL_RELEASE_FOR_DOWNLOAD
	llinfos << "After applying mask " << mask.mName << llendl;
	dump();
#endif
	return TRUE;
}

void LLFeatureList::dump()
{
	llinfos << "Feature list: " << mName << llendl;
	llinfos << "--------------" << llendl;

	LLFeatureInfo fi;
	feature_map_t::iterator feature_it;
	for (feature_it = mFeatures.begin(); feature_it != mFeatures.end(); ++feature_it)
	{
		fi = feature_it->second;
		llinfos << fi.mName << "\t\t" << fi.mAvailable << ":" << fi.mRecommendedLevel << llendl;
	}
	llinfos << llendl;
}

LLFeatureList *LLFeatureManager::findMask(const char *name)
{
	if (mMaskList.count(name))
	{
		return mMaskList[name];
	}

	return NULL;
}

BOOL LLFeatureManager::maskFeatures(const char *name)
{
	LLFeatureList *maskp = findMask(name);
	if (!maskp)
	{
// 		llwarns << "Unknown feature mask " << name << llendl;
		return FALSE;
	}
	llinfos << "Applying Feature Mask: " << name << llendl;
	return maskList(*maskp);
}

BOOL LLFeatureManager::loadFeatureTables()
{
	std::string data_path = gDirUtilp->getAppRODataDir();

	data_path += gDirUtilp->getDirDelimiter();

	data_path += FEATURE_TABLE_FILENAME;


	char	name[MAX_STRING+1];	 /*Flawfinder: ignore*/

	llifstream file;
	U32		version;
	
	file.open(data_path.c_str()); 	 /*Flawfinder: ignore*/

	if (!file)
	{
		llwarns << "Unable to open feature table!" << llendl;
		return FALSE;
	}

	// Check file version
	file >> name;
	file >> version;
	if (strcmp(name, "version"))
	{
		llwarns << data_path << " does not appear to be a valid feature table!" << llendl;
		return FALSE;
	}

	mTableVersion = version;

	LLFeatureList *flp = NULL;
	while (!file.eof())
	{
		char buffer[MAX_STRING];		 /*Flawfinder: ignore*/
		name[0] = 0;

		file >> name;
		
		if (strlen(name) >= 2 && 	 /*Flawfinder: ignore*/
			name[0] == '/' && 
			name[1] == '/')
		{
			// This is a comment.
			file.getline(buffer, MAX_STRING);
			continue;
		}

		if (strlen(name) == 0)		 /*Flawfinder: ignore*/
		{
			// This is a blank line
			file.getline(buffer, MAX_STRING);
			continue;
		}

		if (!strcmp(name, "list"))
		{
			if (flp)
			{
				//flp->dump();
			}
			// It's a new mask, create it.
			file >> name;
			if (mMaskList.count(name))
			{
				llerrs << "Overriding mask " << name << ", this is invalid!" << llendl;
			}

			if (!flp)
			{
				//
				// The first one is always the default
				//
				flp = this;
			}
			else
			{
				flp = new LLFeatureList(name);
				mMaskList[name] = flp;
			}

		}
		else
		{
			if (!flp)
			{
				llerrs << "Specified parameter before <list> keyword!" << llendl;
			}
			S32 available, recommended;
			file >> available >> recommended;
			flp->addFeature(name, available, recommended);
		}
	}
	file.close();

	return TRUE;
}

void LLFeatureManager::loadGPUClass()
{
	std::string data_path = gDirUtilp->getAppRODataDir();

	data_path += gDirUtilp->getDirDelimiter();

	data_path += GPU_TABLE_FILENAME;

	// defaults
	mGPUClass = 0;
	mGPUString = gGLManager.getRawGLString();
	
	llifstream file;
		
	file.open(data_path.c_str()); 		 /*Flawfinder: ignore*/

	if (!file)
	{
		llwarns << "Unable to open GPU table: " << data_path << "!" << llendl;
		return;
	}

	std::string renderer = gGLManager.getRawGLString();
	for (std::string::iterator i = renderer.begin(); i != renderer.end(); ++i)
	{
		*i = tolower(*i);
	}
	
	while (!file.eof())
	{
		char buffer[MAX_STRING];		 /*Flawfinder: ignore*/
		buffer[0] = 0;

		file.getline(buffer, MAX_STRING);
		
		if (strlen(buffer) >= 2 && 	 /*Flawfinder: ignore*/
			buffer[0] == '/' && 
			buffer[1] == '/')
		{
			// This is a comment.
			continue;
		}

		if (strlen(buffer) == 0)	 /*Flawfinder: ignore*/
		{
			// This is a blank line
			continue;
		}

		char* cls, *label, *expr;
		
		label = strtok(buffer, "\t");
		expr = strtok(NULL, "\t");
		cls = strtok(NULL, "\t");

		if (label == NULL || expr == NULL || cls == NULL)
		{
			continue;
		}
	
		for (U32 i = 0; i < strlen(expr); i++)	 /*Flawfinder: ignore*/
		{
			expr[i] = tolower(expr[i]);
		}
		
		char* ex = strtok(expr, ".*");
		char* rnd = (char*) renderer.c_str();

		while (ex != NULL && rnd != NULL)
		{
			rnd = strstr(rnd, ex);
			if (rnd != NULL)
			{
				rnd += strlen(ex);
			}
			ex = strtok(NULL, ".*");
		}
		
		if (rnd != NULL)
		{
			file.close();
			llinfos << "GPU is " << label << llendl;
			mGPUString = label;
			mGPUClass = (S32) strtol(cls, NULL, 10);	
			file.close();
			return;
		}
	}
	file.close();

	llwarns << "Couldn't match GPU to a class: " << gGLManager.getRawGLString() << llendl;
}

void LLFeatureManager::cleanupFeatureTables()
{
	std::for_each(mMaskList.begin(), mMaskList.end(), DeletePairedPointer());
	mMaskList.clear();
}


void LLFeatureManager::initCPUFeatureMasks()
{
	if (gSysMemory.getPhysicalMemoryClamped() <= 256*1024*1024)
	{
		maskFeatures("RAM256MB");
	}
	
#if LL_SOLARIS && defined(__sparc) 	//  even low MHz SPARCs are fast
#error The 800 is hinky. Would something like a LL_MIN_MHZ make more sense here?
	if (gSysCPU.getMhz() < 800)
#else
	if (gSysCPU.getMhz() < 1100)
#endif
	{
		maskFeatures("CPUSlow");
	}
	if (isSafe())
	{
		maskFeatures("safe");
	}
}

void LLFeatureManager::initGraphicsFeatureMasks()
{
	loadGPUClass();
	
	if (mGPUClass >= 0 && mGPUClass < 4)
	{
		const char* class_table[] =
		{
			"Class0",
			"Class1",
			"Class2",
			"Class3"
		};

		llinfos << "Setting GPU Class to " << class_table[mGPUClass] << llendl;
		maskFeatures(class_table[mGPUClass]);
	}
	
	if (!gGLManager.mHasFragmentShader)
	{
		maskFeatures("NoPixelShaders");
	}
	if (!gGLManager.mHasVertexShader)
	{
		maskFeatures("NoVertexShaders");
	}
	if (gGLManager.mIsNVIDIA)
	{
		maskFeatures("NVIDIA");
	}
	if (gGLManager.mIsGF2or4MX)
	{
		maskFeatures("GeForce2");
	}
	if (gGLManager.mIsATI)
	{
		maskFeatures("ATI");
	}
	if (gGLManager.mIsGFFX)
	{
		maskFeatures("GeForceFX");
	}
	if (gGLManager.mIsIntel)
	{
		maskFeatures("Intel");
	}
	if (gGLManager.mGLVersion < 1.5f)
	{
		maskFeatures("OpenGLPre15");
	}
	// Replaces ' ' with '_' in mGPUString to deal with inability for parser to handle spaces
	std::string gpustr = mGPUString;
	for (std::string::iterator iter = gpustr.begin(); iter != gpustr.end(); ++iter)
	{
		if (*iter == ' ')
		{
			*iter = '_';
		}
	}
// 	llinfos << "Masking features from gpu table match: " << gpustr << llendl;
	maskFeatures(gpustr.c_str());

	if (isSafe())
	{
		maskFeatures("safe");
	}
}

void LLFeatureManager::applyRecommendedFeatures()
{
	// see featuretable.txt / featuretable_linux.txt / featuretable_mac.txt

	llinfos << "Applying Recommended Features" << llendl;
#ifndef LL_RELEASE_FOR_DOWNLOAD
	dump();
#endif
	
	// Enabling VBO
	if (getRecommendedLevel("RenderVBO"))
	{
		gSavedSettings.setBOOL("RenderVBOEnable", TRUE);
	}
	else
	{
		gSavedSettings.setBOOL("RenderVBOEnable", FALSE);
	}

	// Anisotropic rendering
	BOOL aniso = getRecommendedLevel("RenderAniso");
	LLImageGL::sGlobalUseAnisotropic	= aniso;
	gSavedSettings.setBOOL("RenderAnisotropic", LLImageGL::sGlobalUseAnisotropic);

	// Render Avatar Mode
	BOOL avatar_vp = getRecommendedLevel("RenderAvatarVP");
	S32 avatar_mode = getRecommendedLevel("RenderAvatarMode");
	if (avatar_vp == FALSE)
		avatar_mode = 0;
	gSavedSettings.setBOOL("RenderAvatarVP", avatar_vp);
	gSavedSettings.setS32("RenderAvatarMode", avatar_mode);
	
	// Render Distance
	S32 far_clip = getRecommendedLevel("RenderDistance");
	gSavedSettings.setF32("RenderFarClip", (F32)far_clip);

	// Lighting
	S32 lighting = getRecommendedLevel("RenderLighting");
	gSavedSettings.setS32("RenderLightingDetail", lighting);

	// ObjectBump
	BOOL bump = getRecommendedLevel("RenderObjectBump");
	gSavedSettings.setBOOL("RenderObjectBump", bump);
	
	// Particle Count
	S32 max_parts = getRecommendedLevel("RenderParticleCount");
	gSavedSettings.setS32("RenderMaxPartCount", max_parts);
	LLViewerPartSim::setMaxPartCount(max_parts);

	// RippleWater
	BOOL ripple = getRecommendedLevel("RenderRippleWater");
	gSavedSettings.setBOOL("RenderRippleWater", ripple);

	// Occlusion Culling
	BOOL occlusion = getRecommendedLevel("UseOcclusion");
	gSavedSettings.setBOOL("UseOcclusion", occlusion);
	
	// Vertex Shaders
	S32 shaders = getRecommendedLevel("VertexShaderEnable");
	gSavedSettings.setBOOL("VertexShaderEnable", shaders);

	// Terrain
	S32 terrain = getRecommendedLevel("RenderTerrainDetail");
	gSavedSettings.setS32("RenderTerrainDetail", terrain);
	LLDrawPoolTerrain::sDetailMode = terrain;

	// Set the amount of VRAM we have available
	if (isSafe())
	{
		gSavedSettings.setS32("GraphicsCardMemorySetting", 1);  // 32 MB in 'safe' mode
	}
	else
	{
		S32 idx = gSavedSettings.getS32("GraphicsCardMemorySetting");
		// -1 indicates use default (max), don't change
		if (idx != -1)
		{
			idx = LLViewerImageList::getMaxVideoRamSetting(-2); // get max recommended setting
			gSavedSettings.setS32("GraphicsCardMemorySetting", idx);
		}
	}
}
