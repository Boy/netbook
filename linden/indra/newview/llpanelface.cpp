/** 
 * @file llpanelface.cpp
 * @brief Panel in the tools floater for editing face textures, colors, etc.
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

#include "llviewerprecompiledheaders.h"

// file include
#include "llpanelface.h"
 
// library includes
#include "llerror.h"
#include "llfocusmgr.h"
#include "llrect.h"
#include "llstring.h"
#include "llfontgl.h"

// project includes
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcolorswatch.h"
#include "llcombobox.h"
#include "lldrawpoolbump.h"
#include "lllineeditor.h"
#include "llresmgr.h"
#include "llselectmgr.h"
#include "llspinctrl.h"
#include "lltextbox.h"
#include "lltexturectrl.h"
#include "lltextureentry.h"
#include "lltooldraganddrop.h"
#include "llui.h"
#include "llviewercontrol.h"
#include "llviewerobject.h"
#include "llviewerstats.h"
#include "llmediaengine.h"

#include "llvieweruictrlfactory.h"

//
// Methods
//

BOOL	LLPanelFace::postBuild()
{
	LLRect	rect = this->getRect();
	LLTextureCtrl*	mTextureCtrl;
	LLColorSwatchCtrl*	mColorSwatch;

	LLTextBox*		mLabelTexGen;
	LLComboBox*		mComboTexGen;

	LLCheckBoxCtrl	*mCheckFullbright;
	
	LLTextBox*		mLabelColorTransp;
	LLSpinCtrl*		mCtrlColorTransp;		// transparency = 1 - alpha

	setMouseOpaque(FALSE);
	mTextureCtrl = LLUICtrlFactory::getTexturePickerByName(this,"texture control");
	if(mTextureCtrl)
	{
		mTextureCtrl->setDefaultImageAssetID(LLUUID( gSavedSettings.getString( "DefaultObjectTexture" )));
		mTextureCtrl->setCommitCallback( LLPanelFace::onCommitTexture );
		mTextureCtrl->setOnCancelCallback( LLPanelFace::onCancelTexture );
		mTextureCtrl->setOnSelectCallback( LLPanelFace::onSelectTexture );
		mTextureCtrl->setDragCallback(LLPanelFace::onDragTexture);
		mTextureCtrl->setCallbackUserData( this );
		mTextureCtrl->setFollowsTop();
		mTextureCtrl->setFollowsLeft();
		// Don't allow (no copy) or (no transfer) textures to be selected during immediate mode
		mTextureCtrl->setImmediateFilterPermMask(PERM_COPY | PERM_TRANSFER);
		// Allow any texture to be used during non-immediate mode.
		mTextureCtrl->setNonImmediateFilterPermMask(PERM_NONE);
		LLAggregatePermissions texture_perms;
		if (gSelectMgr->selectGetAggregateTexturePermissions(texture_perms))
		{
			BOOL can_copy = 
				texture_perms.getValue(PERM_COPY) == LLAggregatePermissions::AP_EMPTY || 
				texture_perms.getValue(PERM_COPY) == LLAggregatePermissions::AP_ALL;
			BOOL can_transfer = 
				texture_perms.getValue(PERM_TRANSFER) == LLAggregatePermissions::AP_EMPTY || 
				texture_perms.getValue(PERM_TRANSFER) == LLAggregatePermissions::AP_ALL;
			mTextureCtrl->setCanApplyImmediately(can_copy && can_transfer);
		}
		else
		{
			mTextureCtrl->setCanApplyImmediately(FALSE);
		}
	}

	mColorSwatch = LLUICtrlFactory::getColorSwatchByName(this,"colorswatch");
	if(mColorSwatch)
	{
		mColorSwatch->setCommitCallback(LLPanelFace::onCommitColor);
		mColorSwatch->setOnCancelCallback(LLPanelFace::onCancelColor);
		mColorSwatch->setOnSelectCallback(LLPanelFace::onSelectColor);
		mColorSwatch->setCallbackUserData( this );
		mColorSwatch->setFollowsTop();
		mColorSwatch->setFollowsLeft();
		mColorSwatch->setCanApplyImmediately(TRUE);
	}

	mLabelColorTransp = LLUICtrlFactory::getTextBoxByName(this,"color trans");
	if(mLabelColorTransp)
	{
		mLabelColorTransp->setFollowsTop();
		mLabelColorTransp->setFollowsLeft();
	}

	mCtrlColorTransp = LLUICtrlFactory::getSpinnerByName(this,"ColorTrans");
	if(mCtrlColorTransp)
	{
		mCtrlColorTransp->setCommitCallback(LLPanelFace::onCommitAlpha);
		mCtrlColorTransp->setCallbackUserData(this);
		mCtrlColorTransp->setPrecision(0);
		mCtrlColorTransp->setFollowsTop();
		mCtrlColorTransp->setFollowsLeft();
	}

	mCheckFullbright = LLUICtrlFactory::getCheckBoxByName(this,"checkbox fullbright");
	if (mCheckFullbright)
	{
		mCheckFullbright->setCommitCallback(LLPanelFace::onCommitFullbright);
		mCheckFullbright->setCallbackUserData( this );
	}
	mLabelTexGen = LLUICtrlFactory::getTextBoxByName(this,"tex gen");
	mComboTexGen = LLUICtrlFactory::getComboBoxByName(this,"combobox texgen");
	if(mComboTexGen)
	{
		mComboTexGen->setCommitCallback(LLPanelFace::onCommitTexGen);
		mComboTexGen->setFollows(FOLLOWS_LEFT | FOLLOWS_TOP);	
		mComboTexGen->setCallbackUserData( this );
	}
	childSetCommitCallback("combobox shininess",&LLPanelFace::onCommitShiny,this);
	childSetCommitCallback("combobox bumpiness",&LLPanelFace::onCommitBump,this);
	childSetCommitCallback("TexScaleU",&LLPanelFace::onCommitTextureInfo, this);
	childSetCommitCallback("checkbox flip s",&LLPanelFace::onCommitTextureInfo, this);
	childSetCommitCallback("TexScaleV",&LLPanelFace::onCommitTextureInfo, this);
	childSetCommitCallback("checkbox flip t",&LLPanelFace::onCommitTextureInfo, this);
	childSetCommitCallback("TexRot",&LLPanelFace::onCommitTextureInfo, this);
	childSetAction("button apply",&onClickApply,this);
	childSetCommitCallback("TexOffsetU",LLPanelFace::onCommitTextureInfo, this);
	childSetCommitCallback("TexOffsetV",LLPanelFace::onCommitTextureInfo, this);
	childSetAction("button align",onClickAutoFix,this);

	clearCtrls();

	return TRUE;
}

LLPanelFace::LLPanelFace(const std::string& name)
:	LLPanel(name)
{
}


LLPanelFace::~LLPanelFace()
{
	// Children all cleaned up by default view destructor.
}


void LLPanelFace::sendTexture()
{
	LLTextureCtrl* mTextureCtrl = gUICtrlFactory->getTexturePickerByName(this,"texture control");
	if(!mTextureCtrl) return;
	if( !mTextureCtrl->getTentative() )
	{
		// we grab the item id first, because we want to do a
		// permissions check in the selection manager. ARGH!
		LLUUID id = mTextureCtrl->getImageItemID();
		if(id.isNull())
		{
			id = mTextureCtrl->getImageAssetID();
		}
		gSelectMgr->selectionSetImage(id);
	}
}

void LLPanelFace::sendBump()
{	
	LLComboBox*	mComboBumpiness = gUICtrlFactory->getComboBoxByName(this,"combobox bumpiness");
	if(!mComboBumpiness)return;
	U8 bump = (U8) mComboBumpiness->getCurrentIndex() & TEM_BUMP_MASK;
	gSelectMgr->selectionSetBumpmap( bump );
}

void LLPanelFace::sendTexGen()
{
	LLComboBox*	mComboTexGen = gUICtrlFactory->getComboBoxByName(this,"combobox texgen");
	if(!mComboTexGen)return;
	U8 tex_gen = (U8) mComboTexGen->getCurrentIndex() << TEM_TEX_GEN_SHIFT;
	gSelectMgr->selectionSetTexGen( tex_gen );
}

void LLPanelFace::sendShiny()
{
	LLComboBox*	mComboShininess = gUICtrlFactory->getComboBoxByName(this,"combobox shininess");
	if(!mComboShininess)return;
	U8 shiny = (U8) mComboShininess->getCurrentIndex() & TEM_SHINY_MASK;
	gSelectMgr->selectionSetShiny( shiny );
}

void LLPanelFace::sendFullbright()
{
	LLCheckBoxCtrl*	mCheckFullbright = gUICtrlFactory->getCheckBoxByName(this,"checkbox fullbright");
	if(!mCheckFullbright)return;
	U8 fullbright = mCheckFullbright->get() ? TEM_FULLBRIGHT_MASK : 0;
	gSelectMgr->selectionSetFullbright( fullbright );
}

void LLPanelFace::sendColor()
{
	
	LLColorSwatchCtrl*	mColorSwatch = LLViewerUICtrlFactory::getColorSwatchByName(this,"colorswatch");
	if(!mColorSwatch)return;
	LLColor4 color = mColorSwatch->get();

	gSelectMgr->selectionSetColorOnly( color );
}

void LLPanelFace::sendAlpha()
{	
	LLSpinCtrl*	mCtrlColorTransp = LLViewerUICtrlFactory::getSpinnerByName(this,"ColorTrans");
	if(!mCtrlColorTransp)return;
	F32 alpha = (100.f - mCtrlColorTransp->get()) / 100.f;

	gSelectMgr->selectionSetAlphaOnly( alpha );
}


struct LLPanelFaceSetTEFunctor : public LLSelectedTEFunctor
{
	LLPanelFaceSetTEFunctor(LLPanelFace* panel) : mPanel(panel) {}
	virtual bool apply(LLViewerObject* object, S32 te)
	{
		BOOL valid;
		F32 value;
		LLSpinCtrl*	ctrlTexScaleS = LLViewerUICtrlFactory::getSpinnerByName(mPanel,"TexScaleU");
		LLSpinCtrl*	ctrlTexScaleT = LLViewerUICtrlFactory::getSpinnerByName(mPanel,"TexScaleV");
		LLSpinCtrl*	ctrlTexOffsetS = LLViewerUICtrlFactory::getSpinnerByName(mPanel,"TexOffsetU");
		LLSpinCtrl*	ctrlTexOffsetT = LLViewerUICtrlFactory::getSpinnerByName(mPanel,"TexOffsetV");
		LLSpinCtrl*	ctrlTexRotation = LLViewerUICtrlFactory::getSpinnerByName(mPanel,"TexRot");
		LLCheckBoxCtrl*	checkFlipScaleS = LLViewerUICtrlFactory::getCheckBoxByName(mPanel,"checkbox flip s");
		LLCheckBoxCtrl*	checkFlipScaleT = LLViewerUICtrlFactory::getCheckBoxByName(mPanel,"checkbox flip t");
		LLComboBox*		comboTexGen = LLViewerUICtrlFactory::getComboBoxByName(mPanel,"combobox texgen");
		if (ctrlTexScaleS)
		{
			valid = !ctrlTexScaleS->getTentative() || !checkFlipScaleS->getTentative();
			if (valid)
			{
				value = ctrlTexScaleS->get();
				if( checkFlipScaleS->get() )
				{
					value = -value;
				}
				if (comboTexGen->getCurrentIndex() == 1)
				{
					value *= 0.5f;
				}
				object->setTEScaleS( te, value );
			}
		}

		if (ctrlTexScaleT)
		{
			valid = !ctrlTexScaleT->getTentative() || !checkFlipScaleT->getTentative();
			if (valid)
			{
				value = ctrlTexScaleT->get();
				if( checkFlipScaleT->get() )
				{
					value = -value;
				}
				if (comboTexGen->getCurrentIndex() == 1)
				{
					value *= 0.5f;
				}
				object->setTEScaleT( te, value );
			}
		}

		if (ctrlTexOffsetS)
		{
			valid = !ctrlTexOffsetS->getTentative();
			if (valid)
			{
				value = ctrlTexOffsetS->get();
				object->setTEOffsetS( te, value );
			}
		}

		if (ctrlTexOffsetT)
		{
			valid = !ctrlTexOffsetT->getTentative();
			if (valid)
			{
				value = ctrlTexOffsetT->get();
				object->setTEOffsetT( te, value );
			}
		}

		if (ctrlTexRotation)
		{
			valid = !ctrlTexRotation->getTentative();
			if (valid)
			{
				value = ctrlTexRotation->get() * DEG_TO_RAD;
				object->setTERotation( te, value );
			}
		}
		return true;
	}
private:
	LLPanelFace* mPanel;
};

struct LLPanelFaceSendFunctor : public LLSelectedObjectFunctor
{
	virtual bool apply(LLViewerObject* object)
	{
		object->sendTEUpdate();
		return true;
	}
};

void LLPanelFace::sendTextureInfo()
{
	LLPanelFaceSetTEFunctor setfunc(this);
	gSelectMgr->getSelection()->applyToTEs(&setfunc);

	LLPanelFaceSendFunctor sendfunc;
	gSelectMgr->getSelection()->applyToObjects(&sendfunc);
}

void LLPanelFace::getState()
{
	LLViewerObject* objectp = gSelectMgr->getSelection()->getFirstObject();

	if( objectp
		&& objectp->getPCode() == LL_PCODE_VOLUME)
	{
		BOOL editable = objectp->permModify();

		// only turn on auto-adjust button if there is a media renderer and the media is loaded
		childSetEnabled("textbox autofix",FALSE);
		//mLabelTexAutoFix->setEnabled ( FALSE );
		childSetEnabled("button align",FALSE);
		//mBtnAutoFix->setEnabled ( FALSE );
		
		if ( LLMediaEngine::getInstance()->getMediaRenderer () )
			if ( LLMediaEngine::getInstance()->getMediaRenderer ()->isLoaded () )
			{	
				childSetEnabled("textbox autofix",editable);
				//mLabelTexAutoFix->setEnabled ( editable );
				childSetEnabled("button align",editable);
				//mBtnAutoFix->setEnabled ( editable );
			}
		childSetEnabled("button apply",editable);

		bool identical;
		LLTextureCtrl*	texture_ctrl = LLViewerUICtrlFactory::getTexturePickerByName(this,"texture control");
		
		// Texture
		{
			LLUUID id;
			struct f1 : public LLSelectedTEGetFunctor<LLUUID>
			{
				LLUUID get(LLViewerObject* object, S32 te)
				{
					LLViewerImage* image = object->getTEImage(te);
					return image ? image->getID() : LLUUID::null;
				}
			} func;
			identical = gSelectMgr->getSelection()->getSelectedTEValue( &func, id );
			
			if (identical)
			{
				// All selected have the same texture
				if(texture_ctrl)
				{
					texture_ctrl->setTentative( FALSE );
					texture_ctrl->setEnabled( editable );
					texture_ctrl->setImageAssetID( id );
				}
			}
			else
			{
				if(texture_ctrl)
				{
					if( id.isNull() )
					{
						// None selected
						texture_ctrl->setTentative( FALSE );
						texture_ctrl->setEnabled( FALSE );
						texture_ctrl->setImageAssetID( LLUUID::null );
					}
					else
					{
						// Tentative: multiple selected with different textures
						texture_ctrl->setTentative( TRUE );
						texture_ctrl->setEnabled( editable );
						texture_ctrl->setImageAssetID( id );
					}
				}
			}
		}
		
		LLAggregatePermissions texture_perms;
		if(texture_ctrl)
		{
// 			texture_ctrl->setValid( editable );
		
			if (gSelectMgr->selectGetAggregateTexturePermissions(texture_perms))
			{
				BOOL can_copy = 
					texture_perms.getValue(PERM_COPY) == LLAggregatePermissions::AP_EMPTY || 
					texture_perms.getValue(PERM_COPY) == LLAggregatePermissions::AP_ALL;
				BOOL can_transfer = 
					texture_perms.getValue(PERM_TRANSFER) == LLAggregatePermissions::AP_EMPTY || 
					texture_perms.getValue(PERM_TRANSFER) == LLAggregatePermissions::AP_ALL;
				texture_ctrl->setCanApplyImmediately(can_copy && can_transfer);
			}
			else
			{
				texture_ctrl->setCanApplyImmediately(FALSE);
			}
		}

		// Texture scale
		{
			childSetEnabled("tex scale",editable);
			//mLabelTexScale->setEnabled( editable );
			F32 scale_s = 1.f;
			struct f2 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					return object->getTE(face)->mScaleS;
				}
			} func;
			identical = gSelectMgr->getSelection()->getSelectedTEValue( &func, scale_s );
			childSetValue("TexScaleU",editable ? llabs(scale_s) : 0);
			childSetTentative("TexScaleU",LLSD((BOOL)(!identical)));
			childSetEnabled("TexScaleU",editable);
			childSetValue("checkbox flip s",LLSD((BOOL)(scale_s < 0 ? TRUE : FALSE )));
			childSetTentative("checkbox flip s",LLSD((BOOL)((!identical) ? TRUE : FALSE )));
			childSetEnabled("checkbox flip s",editable);
		}

		{
			F32 scale_t = 1.f;
			struct f3 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					return object->getTE(face)->mScaleT;
				}
			} func;
			identical = gSelectMgr->getSelection()->getSelectedTEValue( &func, scale_t );

			childSetValue("TexScaleV",llabs(editable ? llabs(scale_t) : 0));
			childSetTentative("TexScaleV",LLSD((BOOL)(!identical)));
			childSetEnabled("TexScaleV",editable);
			childSetValue("checkbox flip t",LLSD((BOOL)(scale_t< 0 ? TRUE : FALSE )));
			childSetTentative("checkbox flip t",LLSD((BOOL)((!identical) ? TRUE : FALSE )));
			childSetEnabled("checkbox flip t",editable);
		}

		// Texture offset
		{
			childSetEnabled("tex offset",editable);
			F32 offset_s = 0.f;
			struct f4 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					return object->getTE(face)->mOffsetS;
				}
			} func;
			identical = gSelectMgr->getSelection()->getSelectedTEValue( &func, offset_s );
			childSetValue("TexOffsetU", editable ? offset_s : 0);
			childSetTentative("TexOffsetU",!identical);
			childSetEnabled("TexOffsetU",editable);
		}

		{
			F32 offset_t = 0.f;
			struct f5 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					return object->getTE(face)->mOffsetT;
				}
			} func;
			identical = gSelectMgr->getSelection()->getSelectedTEValue( &func, offset_t );
			childSetValue("TexOffsetV", editable ? offset_t : 0);
			childSetTentative("TexOffsetV",!identical);
			childSetEnabled("TexOffsetV",editable);
		}

		// Texture rotation
		{
			childSetEnabled("tex rotate",editable);
			F32 rotation = 0.f;
			struct f6 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					return object->getTE(face)->mRotation;
				}
			} func;
			identical = gSelectMgr->getSelection()->getSelectedTEValue( &func, rotation );
			childSetValue("TexRot", editable ? rotation * RAD_TO_DEG : 0);
			childSetTentative("TexRot",!identical);
			childSetEnabled("TexRot",editable);
		}

		// Color swatch
		LLColorSwatchCtrl*	mColorSwatch = LLViewerUICtrlFactory::getColorSwatchByName(this,"colorswatch");
		LLColor4 color = LLColor4::white;
		if(mColorSwatch)
		{
			struct f7 : public LLSelectedTEGetFunctor<LLColor4>
			{
				LLColor4 get(LLViewerObject* object, S32 face)
				{
					return object->getTE(face)->getColor();
				}
			} func;
			identical = gSelectMgr->getSelection()->getSelectedTEValue( &func, color );
			
			mColorSwatch->setOriginal(color);
			mColorSwatch->set(color, TRUE);

			mColorSwatch->setValid(editable);
			mColorSwatch->setEnabled( editable );
			mColorSwatch->setCanApplyImmediately( editable );
		}
		// Color transparency
		{
			childSetEnabled("color trans",editable);
		}

		F32 transparency = (1.f - color.mV[VALPHA]) * 100.f;
		{
			childSetValue("ColorTrans", editable ? transparency : 0);
			childSetEnabled("ColorTrans",editable);
		}

		// Bump
		{
			F32 shinyf = 0.f;
			struct f8 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					return (F32)(object->getTE(face)->getShiny());
				}
			} func;
			identical = gSelectMgr->getSelection()->getSelectedTEValue( &func, shinyf );
			LLCtrlSelectionInterface* combobox_shininess =
			      childGetSelectionInterface("combobox shininess");
			if (combobox_shininess)
			{
				combobox_shininess->selectNthItem((S32)shinyf);
			}
			else
			{
				llwarns << "failed childGetSelectionInterface for 'combobox shininess'" << llendl;
			}
			childSetEnabled("combobox shininess",editable);
			childSetTentative("combobox shininess",!identical);
			childSetEnabled("label shininess",editable);
		}

		{
			F32 bumpf = 0.f;
			struct f9 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					return (F32)(object->getTE(face)->getBumpmap());
				}
			} func;
			identical = gSelectMgr->getSelection()->getSelectedTEValue( &func, bumpf );
			LLCtrlSelectionInterface* combobox_bumpiness =
			      childGetSelectionInterface("combobox bumpiness");
			if (combobox_bumpiness)
			{
				combobox_bumpiness->selectNthItem((S32)bumpf);
			}
			else
			{
				llwarns << "failed childGetSelectionInterface for 'combobox bumpiness'" << llendl;
			}
			childSetEnabled("combobox bumpiness",editable);
			childSetTentative("combobox bumpiness",!identical);
			childSetEnabled("label bumpiness",editable);
		}

		{
			F32 genf = 0.f;
			struct f10 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					return (F32)(object->getTE(face)->getTexGen());
				}
			} func;
			identical = gSelectMgr->getSelection()->getSelectedTEValue( &func, genf );
			S32 selected_texgen = ((S32) genf) >> TEM_TEX_GEN_SHIFT;
			LLCtrlSelectionInterface* combobox_texgen =
			      childGetSelectionInterface("combobox texgen");
			if (combobox_texgen)
			{
				combobox_texgen->selectNthItem(selected_texgen);
			}
			else
			{
				llwarns << "failed childGetSelectionInterface for 'combobox texgen'" << llendl;
			}
			childSetEnabled("combobox texgen",editable);
			childSetTentative("combobox texgen",!identical);
			childSetEnabled("tex gen",editable);

			if (selected_texgen == 1)
			{
				childSetText("tex scale",childGetText("string repeats per meter"));
				childSetValue("TexScaleU", 2.0f * childGetValue("TexScaleU").asReal() );
				childSetValue("TexScaleV", 2.0f * childGetValue("TexScaleV").asReal() );
			}
			else
			{
				childSetText("tex scale",childGetText("string repeats per face"));
			}

		}

		{
			F32 fullbrightf = 0.f;
			struct f11 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					return (F32)(object->getTE(face)->getFullbright());
				}
			} func;
			identical = gSelectMgr->getSelection()->getSelectedTEValue( &func, fullbrightf );

			childSetValue("checkbox fullbright",(S32)fullbrightf);
			childSetEnabled("checkbox fullbright",editable);
			childSetTentative("checkbox fullbright",!identical);
		}
		
		// Repeats per meter label
		{
			childSetEnabled("rpt",editable);
		}

		// Repeats per meter
		{
			F32 repeats = 1.f;
			struct f12 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					U32 s_axis = VX;
					U32 t_axis = VY;
					// BUG: Only repeats along S axis
					// BUG: Only works for boxes.
					LLPrimitive::getTESTAxes(face, &s_axis, &t_axis);
					return object->getTE(face)->mScaleS / object->getScale().mV[s_axis];
				}
			} func;			
			identical = gSelectMgr->getSelection()->getSelectedTEValue( &func, repeats );
			
			childSetValue("rptctrl", editable ? repeats : 0);
			childSetTentative("rptctrl",!identical);
			LLComboBox*	mComboTexGen = LLViewerUICtrlFactory::getComboBoxByName(this,"combobox texgen");
			if (mComboTexGen)
			{
				BOOL enabled = editable && (!mComboTexGen || mComboTexGen->getCurrentIndex() != 1);
				childSetEnabled("rptctrl",enabled);
				childSetEnabled("button apply",enabled);
			}
		}
	}
	else
	{
		// Disable all UICtrls
		clearCtrls();

		// Disable non-UICtrls
		LLTextureCtrl*	texture_ctrl = LLUICtrlFactory::getTexturePickerByName(this,"texture control"); 
		if(texture_ctrl)
		{
			texture_ctrl->setImageAssetID( LLUUID::null );
			texture_ctrl->setEnabled( FALSE );  // this is a LLUICtrl, but we don't want it to have keyboard focus so we add it as a child, not a ctrl.
// 			texture_ctrl->setValid(FALSE);
		}
		LLColorSwatchCtrl* mColorSwatch = LLUICtrlFactory::getColorSwatchByName(this,"colorswatch");
		if(mColorSwatch)
		{
			mColorSwatch->setEnabled( FALSE );			
			mColorSwatch->setValid(FALSE);
		}
		childSetEnabled("color trans",FALSE);
		childSetEnabled("rpt",FALSE);
		childSetEnabled("tex scale",FALSE);
		childSetEnabled("tex offset",FALSE);
		childSetEnabled("tex rotate",FALSE);
		childSetEnabled("tex gen",FALSE);
		childSetEnabled("label shininess",FALSE);
		childSetEnabled("label bumpiness",FALSE);

		childSetEnabled("textbox autofix",FALSE);

		childSetEnabled("button align",FALSE);
		childSetEnabled("button apply",FALSE);
	}
}


void LLPanelFace::refresh()
{
	getState();
}

//
// Static functions
//


// static
void LLPanelFace::onCommitColor(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->sendColor();
}

// static
void LLPanelFace::onCommitAlpha(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->sendAlpha();
}

// static
void LLPanelFace::onCancelColor(LLUICtrl* ctrl, void* userdata)
{
	gSelectMgr->selectionRevertColors();
}

// static
void LLPanelFace::onSelectColor(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	gSelectMgr->saveSelectedObjectColors();
	self->sendColor();
}

// static
void LLPanelFace::onCommitBump(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->sendBump();
}

// static
void LLPanelFace::onCommitTexGen(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->sendTexGen();
}

// static
void LLPanelFace::onCommitShiny(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->sendShiny();
}

// static
void LLPanelFace::onCommitFullbright(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->sendFullbright();
}

// static
BOOL LLPanelFace::onDragTexture(LLUICtrl*, LLInventoryItem* item, void*)
{
	BOOL accept = TRUE;
	for (LLObjectSelection::root_iterator iter = gSelectMgr->getSelection()->root_begin();
		 iter != gSelectMgr->getSelection()->root_end(); iter++)
	{
		LLSelectNode* node = *iter;
		LLViewerObject* obj = node->getObject();
		if(!LLToolDragAndDrop::isInventoryDropAcceptable(obj, item))
		{
			accept = FALSE;
			break;
		}
	}
	return accept;
}

// static
void LLPanelFace::onCommitTexture( LLUICtrl* ctrl, void* userdata )
{
	LLPanelFace* self = (LLPanelFace*) userdata;

	gViewerStats->incStat(LLViewerStats::ST_EDIT_TEXTURE_COUNT );
	
	self->sendTexture();
}

// static
void LLPanelFace::onCancelTexture(LLUICtrl* ctrl, void* userdata)
{
	gSelectMgr->selectionRevertTextures();
}

// static
void LLPanelFace::onSelectTexture(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	gSelectMgr->saveSelectedObjectTextures();
	self->sendTexture();
}


// static
void LLPanelFace::onCommitTextureInfo( LLUICtrl* ctrl, void* userdata )
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->sendTextureInfo();
}

// Commit the number of repeats per meter
// static
void LLPanelFace::onClickApply(void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	
	gFocusMgr.setKeyboardFocus( NULL );

	//F32 repeats_per_meter = self->mCtrlRepeatsPerMeter->get();
	F32 repeats_per_meter = (F32)self->childGetValue( "rptctrl" ).asReal();//self->mCtrlRepeatsPerMeter->get();
	gSelectMgr->selectionTexScaleAutofit( repeats_per_meter );
}

// commit the fit media texture to prim button

struct LLPanelFaceSetMediaFunctor : public LLSelectedTEFunctor
{
	virtual bool apply(LLViewerObject* object, S32 te)
	{
		// only do this if it's a media texture
		if ( object->getTE ( te )->getID() ==  LLMediaEngine::getInstance()->getImageUUID () )
		{
			// make sure we're valid
			if ( LLMediaEngine::getInstance()->getMediaRenderer() )
			{
				// calculate correct scaling based on media dimensions and next-power-of-2 texture dimensions
				F32 scaleS = (F32)LLMediaEngine::getInstance()->getMediaRenderer()->getMediaWidth() / 
								(F32)LLMediaEngine::getInstance()->getMediaRenderer()->getTextureWidth();

				F32 scaleT = (F32)LLMediaEngine::getInstance()->getMediaRenderer()->getMediaHeight() /
								(F32)LLMediaEngine::getInstance()->getMediaRenderer()->getTextureHeight();

				// set scale and adjust offset
				object->setTEScaleS( te, scaleS );
				object->setTEScaleT( te, scaleT );	// don't need to flip Y anymore since QT does this for us now.
				object->setTEOffsetS( te, -( 1.0f - scaleS ) / 2.0f );
				object->setTEOffsetT( te, -( 1.0f - scaleT ) / 2.0f );
			}
		}
		return true;
	}
};

void LLPanelFace::onClickAutoFix(void* userdata)
{
	LLPanelFaceSetMediaFunctor setfunc;
	gSelectMgr->getSelection()->applyToTEs(&setfunc);

	LLPanelFaceSendFunctor sendfunc;
	gSelectMgr->getSelection()->applyToObjects(&sendfunc);
}
