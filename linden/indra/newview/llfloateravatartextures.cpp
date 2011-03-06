/** 
 * @file llfloateravatartextures.cpp
 * @brief Debugging view showing underlying avatar textures and baked textures.
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

#include "llfloateravatartextures.h"

#include "lltexturectrl.h"

#include "llvieweruictrlfactory.h"
#include "llviewerobjectlist.h"
#include "llvoavatar.h"

LLFloaterAvatarTextures::LLFloaterAvatarTextures(const LLUUID& id) : 
	LLFloater("avatar_texture_debug"),
	mID(id)
{
}

LLFloaterAvatarTextures::~LLFloaterAvatarTextures()
{
}

LLFloaterAvatarTextures* LLFloaterAvatarTextures::show(const LLUUID &id)
{

	LLFloaterAvatarTextures* floaterp = new LLFloaterAvatarTextures(id);

	// Builds and adds to gFloaterView
	gUICtrlFactory->buildFloater(floaterp, "floater_avatar_textures.xml");

	gFloaterView->addChild(floaterp);
	floaterp->open();	/*Flawfinder: ignore*/

	gFloaterView->adjustToFitScreen(floaterp, FALSE);

	return floaterp;
}

BOOL LLFloaterAvatarTextures::postBuild()
{
	mBakedHead = (LLTextureCtrl*)getChildByName("baked_head");
	mBakedEyes = (LLTextureCtrl*)getChildByName("baked_eyes");
	mBakedUpper = (LLTextureCtrl*)getChildByName("baked_upper_body");
	mBakedLower = (LLTextureCtrl*)getChildByName("baked_lower_body");
	mBakedSkirt = (LLTextureCtrl*)getChildByName("baked_skirt");
	mHair = (LLTextureCtrl*)getChildByName("hair");
	mMakeup = (LLTextureCtrl*)getChildByName("head_bodypaint");
	mEye = (LLTextureCtrl*)getChildByName("eye_texture");
	mShirt = (LLTextureCtrl*)getChildByName("shirt");
	mUpperTattoo = (LLTextureCtrl*)getChildByName("upper_bodypaint");
	mUpperJacket = (LLTextureCtrl*)getChildByName("upper_jacket");
	mGloves = (LLTextureCtrl*)getChildByName("gloves");
	mUndershirt = (LLTextureCtrl*)getChildByName("undershirt");
	mPants = (LLTextureCtrl*)getChildByName("pants");
	mLowerTattoo = (LLTextureCtrl*)getChildByName("lower_bodypaint");
	mShoes = (LLTextureCtrl*)getChildByName("shoes");
	mSocks = (LLTextureCtrl*)getChildByName("socks");
	mJacket = (LLTextureCtrl*)getChildByName("jacket");
	mUnderpants = (LLTextureCtrl*)getChildByName("underpants");
	mSkirt = (LLTextureCtrl*)getChildByName("skirt_texture");
	mTitle = getTitle();

	childSetAction("Dump", onClickDump, this);

	refresh();
	return TRUE;
}

void LLFloaterAvatarTextures::draw()
{
	refresh();
	LLFloater::draw();
}

#if !LL_RELEASE_FOR_DOWNLOAD
static void update_texture_ctrl(LLVOAvatar* avatarp,
								 LLTextureCtrl* ctrl,
								 LLVOAvatar::ETextureIndex te)
{
	LLUUID id = avatarp->getTE(te)->getID();
	if (id == IMG_DEFAULT_AVATAR)
	{
		ctrl->setImageAssetID(LLUUID::null);
		ctrl->setToolTip(LLString("IMG_DEFAULT_AVATAR"));
	}
	else
	{
		ctrl->setImageAssetID(id);
		ctrl->setToolTip(id.asString());
	}
}

static LLVOAvatar* find_avatar(const LLUUID& id)
{
	LLViewerObject *obj = gObjectList.findObject(id);
	while (obj && obj->isAttachment())
	{
		obj = (LLViewerObject *)obj->getParent();
	}

	if (obj && obj->isAvatar())
	{
		return (LLVOAvatar*)obj;
	}
	else
	{
		return NULL;
	}
}

void LLFloaterAvatarTextures::refresh()
{
	LLVOAvatar *avatarp = find_avatar(mID);
	if (avatarp)
	{
		char firstname[DB_FIRST_NAME_BUF_SIZE];	/*Flawfinder: ignore*/
		char lastname[DB_LAST_NAME_BUF_SIZE];		/*Flawfinder: ignore*/
		if (gCacheName->getName(avatarp->getID(), firstname, lastname))
		{
			LLString name;
			name.assign( firstname );
			name.append( " " );
			name.append( lastname );

			setTitle(mTitle + ": " + name);
		}
		update_texture_ctrl(avatarp, mBakedHead,	LLVOAvatar::TEX_HEAD_BAKED);
		update_texture_ctrl(avatarp, mBakedEyes,	LLVOAvatar::TEX_EYES_BAKED);
		update_texture_ctrl(avatarp, mBakedUpper,	LLVOAvatar::TEX_UPPER_BAKED);
		update_texture_ctrl(avatarp, mBakedLower,	LLVOAvatar::TEX_LOWER_BAKED);
		update_texture_ctrl(avatarp, mBakedSkirt,	LLVOAvatar::TEX_SKIRT_BAKED);

		update_texture_ctrl(avatarp, mMakeup,		LLVOAvatar::TEX_HEAD_BODYPAINT);
		update_texture_ctrl(avatarp, mHair,			LLVOAvatar::TEX_HAIR);
		update_texture_ctrl(avatarp, mEye,			LLVOAvatar::TEX_EYES_IRIS);

		update_texture_ctrl(avatarp, mShirt,		LLVOAvatar::TEX_UPPER_SHIRT);
		update_texture_ctrl(avatarp, mUpperTattoo,	LLVOAvatar::TEX_UPPER_BODYPAINT);
		update_texture_ctrl(avatarp, mUpperJacket,	LLVOAvatar::TEX_UPPER_JACKET);
		update_texture_ctrl(avatarp, mGloves,		LLVOAvatar::TEX_UPPER_GLOVES);
		update_texture_ctrl(avatarp, mUndershirt,	LLVOAvatar::TEX_UPPER_UNDERSHIRT);

		update_texture_ctrl(avatarp, mPants,		LLVOAvatar::TEX_LOWER_PANTS);
		update_texture_ctrl(avatarp, mLowerTattoo,	LLVOAvatar::TEX_LOWER_BODYPAINT);
		update_texture_ctrl(avatarp, mShoes,		LLVOAvatar::TEX_LOWER_SHOES);
		update_texture_ctrl(avatarp, mSocks,		LLVOAvatar::TEX_LOWER_SOCKS);
		update_texture_ctrl(avatarp, mJacket,		LLVOAvatar::TEX_LOWER_JACKET);
		update_texture_ctrl(avatarp, mUnderpants,	LLVOAvatar::TEX_LOWER_UNDERPANTS);
		update_texture_ctrl(avatarp, mSkirt,		LLVOAvatar::TEX_SKIRT);
	}
	else
	{
		setTitle(mTitle + ": INVALID AVATAR (" + mID.asString() + ")");
	}
}

#else

void LLFloaterAvatarTextures::refresh()
{
}

#endif

// static
void LLFloaterAvatarTextures::onClickDump(void* data)
{
#if !LL_RELEASE_FOR_DOWNLOAD
	LLFloaterAvatarTextures* self = (LLFloaterAvatarTextures*)data;
	LLVOAvatar* avatarp = find_avatar(self->mID);
	if (!avatarp) return;

	for (S32 i = 0; i < avatarp->getNumTEs(); i++)
	{
		const LLTextureEntry* te = avatarp->getTE(i);
		if (!te) continue;

		llinfos << "Avatar TE " << i << " id " << te->getID() << llendl;
	}
#endif
}
