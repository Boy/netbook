/** 
 * @file llviewerjointmesh.cpp
 * @brief Implementation of LLViewerJointMesh class
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

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include "llviewerprecompiledheaders.h"

#include "imageids.h"
#include "llfasttimer.h"

#include "llagent.h"
#include "llapr.h"
#include "llbox.h"
#include "lldrawable.h"
#include "lldrawpoolavatar.h"
#include "lldrawpoolbump.h"
#include "lldynamictexture.h"
#include "llface.h"
#include "llgldbg.h"
#include "llglheaders.h"
#include "lltexlayer.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewerimagelist.h"
#include "llviewerjointmesh.h"
#include "llvoavatar.h"
#include "llsky.h"
#include "pipeline.h"
#include "llglslshader.h"
#include "llmath.h"
#include "v4math.h"
#include "m3math.h"
#include "m4math.h"

#if !LL_DARWIN && !LL_LINUX && !LL_SOLARIS
extern PFNGLWEIGHTPOINTERARBPROC glWeightPointerARB;
extern PFNGLWEIGHTFVARBPROC glWeightfvARB;
extern PFNGLVERTEXBLENDARBPROC glVertexBlendARB;
#endif
extern BOOL gRenderForSelect;

static LLPointer<LLVertexBuffer> sRenderBuffer = NULL;
static const U32 sRenderMask = LLVertexBuffer::MAP_VERTEX |
							   LLVertexBuffer::MAP_NORMAL |
							   LLVertexBuffer::MAP_TEXCOORD;


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// LLViewerJointMesh::LLSkinJoint
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// LLSkinJoint
//-----------------------------------------------------------------------------
LLSkinJoint::LLSkinJoint()
{
	mJoint       = NULL;
}

//-----------------------------------------------------------------------------
// ~LLSkinJoint
//-----------------------------------------------------------------------------
LLSkinJoint::~LLSkinJoint()
{
	mJoint = NULL;
}


//-----------------------------------------------------------------------------
// LLSkinJoint::setupSkinJoint()
//-----------------------------------------------------------------------------
BOOL LLSkinJoint::setupSkinJoint( LLViewerJoint *joint)
{
	// find the named joint
	mJoint = joint;
	if ( !mJoint )
	{
		llinfos << "Can't find joint" << llendl;
	}

	// compute the inverse root skin matrix
	mRootToJointSkinOffset.clearVec();

	LLVector3 rootSkinOffset;
	while (joint)
	{
		rootSkinOffset += joint->getSkinOffset();
		joint = (LLViewerJoint*)joint->getParent();
	}

	mRootToJointSkinOffset = -rootSkinOffset;
	mRootToParentJointSkinOffset = mRootToJointSkinOffset;
	mRootToParentJointSkinOffset += mJoint->getSkinOffset();

	return TRUE;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// LLViewerJointMesh
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

BOOL LLViewerJointMesh::sPipelineRender = FALSE;
EAvatarRenderPass LLViewerJointMesh::sRenderPass = AVATAR_RENDER_PASS_SINGLE;
U32 LLViewerJointMesh::sClothingMaskImageName = 0;
LLColor4 LLViewerJointMesh::sClothingInnerColor;

//-----------------------------------------------------------------------------
// LLViewerJointMesh()
//-----------------------------------------------------------------------------
LLViewerJointMesh::LLViewerJointMesh()
	:
	mTexture( NULL ),
	mLayerSet( NULL ),
	mTestImageName( 0 ),
	mIsTransparent(FALSE)
{

	mColor[0] = 1.0f;
	mColor[1] = 1.0f;
	mColor[2] = 1.0f;
	mColor[3] = 1.0f;
	mShiny = 0.0f;
	mCullBackFaces = TRUE;

	mMesh = NULL;

	mNumSkinJoints = 0;
	mSkinJoints = NULL;

	mFace = NULL;

	mMeshID = 0;
	mUpdateXform = FALSE;

	mValid = FALSE;
}


//-----------------------------------------------------------------------------
// ~LLViewerJointMesh()
// Class Destructor
//-----------------------------------------------------------------------------
LLViewerJointMesh::~LLViewerJointMesh()
{
	mMesh = NULL;
	mTexture = NULL;
	freeSkinData();
}


//-----------------------------------------------------------------------------
// LLViewerJointMesh::allocateSkinData()
//-----------------------------------------------------------------------------
BOOL LLViewerJointMesh::allocateSkinData( U32 numSkinJoints )
{
	mSkinJoints = new LLSkinJoint[ numSkinJoints ];
	mNumSkinJoints = numSkinJoints;
	return TRUE;
}

//-----------------------------------------------------------------------------
// getSkinJointByIndex()
//-----------------------------------------------------------------------------
S32 LLViewerJointMesh::getBoundJointsByIndex(S32 index, S32 &joint_a, S32& joint_b)
{
	S32 num_joints = 0;
	if (mNumSkinJoints == 0)
	{
		return num_joints;
	}

	joint_a = -1;
	joint_b = -1;

	LLPolyMesh *reference_mesh = mMesh->getReferenceMesh();

	if (index < reference_mesh->mJointRenderData.count())
	{
		LLJointRenderData* render_datap = reference_mesh->mJointRenderData[index];
		if (render_datap->mSkinJoint)
		{
			joint_a = render_datap->mSkinJoint->mJoint->mJointNum;
		}
		num_joints++;
	}
	if (index + 1 < reference_mesh->mJointRenderData.count())
	{
		LLJointRenderData* render_datap = reference_mesh->mJointRenderData[index + 1];
		if (render_datap->mSkinJoint)
		{
			joint_b = render_datap->mSkinJoint->mJoint->mJointNum;

			if (joint_a == -1)
			{
				joint_a = render_datap->mSkinJoint->mJoint->getParent()->mJointNum;
			}
		}
		num_joints++;
	}
	return num_joints;
}

//-----------------------------------------------------------------------------
// LLViewerJointMesh::freeSkinData()
//-----------------------------------------------------------------------------
void LLViewerJointMesh::freeSkinData()
{
	mNumSkinJoints = 0;
	delete [] mSkinJoints;
	mSkinJoints = NULL;
}

//--------------------------------------------------------------------
// LLViewerJointMesh::getColor()
//--------------------------------------------------------------------
void LLViewerJointMesh::getColor( F32 *red, F32 *green, F32 *blue, F32 *alpha )
{
	*red   = mColor[0];
	*green = mColor[1];
	*blue  = mColor[2];
	*alpha = mColor[3];
}

//--------------------------------------------------------------------
// LLViewerJointMesh::setColor()
//--------------------------------------------------------------------
void LLViewerJointMesh::setColor( F32 red, F32 green, F32 blue, F32 alpha )
{
	mColor[0] = red;
	mColor[1] = green;
	mColor[2] = blue;
	mColor[3] = alpha;
}


//--------------------------------------------------------------------
// LLViewerJointMesh::getTexture()
//--------------------------------------------------------------------
//LLViewerImage *LLViewerJointMesh::getTexture()
//{
//	return mTexture;
//}

//--------------------------------------------------------------------
// LLViewerJointMesh::setTexture()
//--------------------------------------------------------------------
void LLViewerJointMesh::setTexture( LLViewerImage *texture )
{
	mTexture = texture;

	// texture and dynamic_texture are mutually exclusive
	if( texture )
	{
		mLayerSet = NULL;
		//texture->bindTexture(0);
		//texture->setClamp(TRUE, TRUE);
	}
}

//--------------------------------------------------------------------
// LLViewerJointMesh::setLayerSet()
// Sets the shape texture (takes precedence over normal texture)
//--------------------------------------------------------------------
void LLViewerJointMesh::setLayerSet( LLTexLayerSet* layer_set )
{
	mLayerSet = layer_set;
	
	// texture and dynamic_texture are mutually exclusive
	if( layer_set )
	{
		mTexture = NULL;
	}
}



//--------------------------------------------------------------------
// LLViewerJointMesh::getMesh()
//--------------------------------------------------------------------
LLPolyMesh *LLViewerJointMesh::getMesh()
{
	return mMesh;
}

//-----------------------------------------------------------------------------
// LLViewerJointMesh::setMesh()
//-----------------------------------------------------------------------------
void LLViewerJointMesh::setMesh( LLPolyMesh *mesh )
{
	// set the mesh pointer
	mMesh = mesh;

	// release any existing skin joints
	freeSkinData();

	if ( mMesh == NULL )
	{
		return;
	}

	// acquire the transform from the mesh object
	setPosition( mMesh->getPosition() );
	setRotation( mMesh->getRotation() );
	setScale( mMesh->getScale() );

	// create skin joints if necessary
	if ( mMesh->hasWeights() && !mMesh->isLOD())
	{
		U32 numJointNames = mMesh->getNumJointNames();
		
		allocateSkinData( numJointNames );
		std::string *jointNames = mMesh->getJointNames();

		U32 jn;
		for (jn = 0; jn < numJointNames; jn++)
		{
			//llinfos << "Setting up joint " << jointNames[jn].c_str() << llendl;
			LLViewerJoint* joint = (LLViewerJoint*)(getRoot()->findJoint(jointNames[jn]) );
			mSkinJoints[jn].setupSkinJoint( joint );
		}
	}

	// setup joint array
	if (!mMesh->isLOD())
	{
		setupJoint((LLViewerJoint*)getRoot());
	}

//	llinfos << "joint render entries: " << mMesh->mJointRenderData.count() << llendl;
}

//-----------------------------------------------------------------------------
// setupJoint()
//-----------------------------------------------------------------------------
void LLViewerJointMesh::setupJoint(LLViewerJoint* current_joint)
{
//	llinfos << "Mesh: " << getName() << llendl;

//	S32 joint_count = 0;
	U32 sj;
	for (sj=0; sj<mNumSkinJoints; sj++)
	{
		LLSkinJoint &js = mSkinJoints[sj];

		if (js.mJoint != current_joint)
		{
			continue;
		}

		// we've found a skinjoint for this joint..

		// is the last joint in the array our parent?
		if(mMesh->mJointRenderData.count() && mMesh->mJointRenderData[mMesh->mJointRenderData.count() - 1]->mWorldMatrix == &current_joint->getParent()->getWorldMatrix())
		{
			// ...then just add ourselves
			LLViewerJoint* jointp = js.mJoint;
			mMesh->mJointRenderData.put(new LLJointRenderData(&jointp->getWorldMatrix(), &js));
//			llinfos << "joint " << joint_count << js.mJoint->getName() << llendl;
//			joint_count++;
		}
		// otherwise add our parent and ourselves
		else
		{
			mMesh->mJointRenderData.put(new LLJointRenderData(&current_joint->getParent()->getWorldMatrix(), NULL));
//			llinfos << "joint " << joint_count << current_joint->getParent()->getName() << llendl;
//			joint_count++;
			mMesh->mJointRenderData.put(new LLJointRenderData(&current_joint->getWorldMatrix(), &js));
//			llinfos << "joint " << joint_count << current_joint->getName() << llendl;
//			joint_count++;
		}
	}

	// depth-first traversal
	for (LLJoint::child_list_t::iterator iter = current_joint->mChildren.begin();
		 iter != current_joint->mChildren.end(); ++iter)
	{
		LLViewerJoint* child_joint = (LLViewerJoint*)(*iter);
		setupJoint(child_joint);
	}
}

const S32 NUM_AXES = 3;

// register layoud
// rotation X 0-n
// rotation Y 0-n
// rotation Z 0-n
// pivot parent 0-n -- child = n+1

static LLMatrix4	gJointMatUnaligned[32];
static LLMatrix3	gJointRotUnaligned[32];
static LLVector4	gJointPivot[32];

//-----------------------------------------------------------------------------
// uploadJointMatrices()
//-----------------------------------------------------------------------------
void LLViewerJointMesh::uploadJointMatrices()
{
	S32 joint_num;
	LLPolyMesh *reference_mesh = mMesh->getReferenceMesh();
	LLDrawPool *poolp = mFace ? mFace->getPool() : NULL;
	BOOL hardware_skinning = (poolp && poolp->getVertexShaderLevel() > 0) ? TRUE : FALSE;

	//calculate joint matrices
	for (joint_num = 0; joint_num < reference_mesh->mJointRenderData.count(); joint_num++)
	{
		LLMatrix4 joint_mat = *reference_mesh->mJointRenderData[joint_num]->mWorldMatrix;

		if (hardware_skinning)
		{
			joint_mat *= LLDrawPoolAvatar::getModelView();
		}
		gJointMatUnaligned[joint_num] = joint_mat;
		gJointRotUnaligned[joint_num] = joint_mat.getMat3();
	}

	BOOL last_pivot_uploaded = FALSE;
	S32 j = 0;

	//upload joint pivots
	for (joint_num = 0; joint_num < reference_mesh->mJointRenderData.count(); joint_num++)
	{
		LLSkinJoint *sj = reference_mesh->mJointRenderData[joint_num]->mSkinJoint;
		if (sj)
		{
			if (!last_pivot_uploaded)
			{
				LLVector4 parent_pivot(sj->mRootToParentJointSkinOffset);
				parent_pivot.mV[VW] = 0.f;
				gJointPivot[j++] = parent_pivot;
			}

			LLVector4 child_pivot(sj->mRootToJointSkinOffset);
			child_pivot.mV[VW] = 0.f;

			gJointPivot[j++] = child_pivot;

			last_pivot_uploaded = TRUE;
		}
		else
		{
			last_pivot_uploaded = FALSE;
		}
	}

	//add pivot point into transform
	for (S32 i = 0; i < j; i++)
	{
		LLVector3 pivot;
		pivot = LLVector3(gJointPivot[i]);
		pivot = pivot * gJointRotUnaligned[i];
		gJointMatUnaligned[i].translate(pivot);
	}

	// upload matrices
	if (hardware_skinning)
	{
		GLfloat mat[45*4];
		memset(mat, 0, sizeof(GLfloat)*45*4);

		for (joint_num = 0; joint_num < reference_mesh->mJointRenderData.count(); joint_num++)
		{
			gJointMatUnaligned[joint_num].transpose();

			for (S32 axis = 0; axis < NUM_AXES; axis++)
			{
				F32* vector = gJointMatUnaligned[joint_num].mMatrix[axis];
				//glProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB, LL_CHARACTER_MAX_JOINTS_PER_MESH * axis + joint_num+5, (GLfloat*)vector);
				U32 offset = LL_CHARACTER_MAX_JOINTS_PER_MESH*axis+joint_num;
				memcpy(mat+offset*4, vector, sizeof(GLfloat)*4);
				//glProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB, LL_CHARACTER_MAX_JOINTS_PER_MESH * axis + joint_num+6, (GLfloat*)vector);
				//cgGLSetParameterArray4f(gPipeline.mAvatarMatrix, offset, 1, vector);
			}
		}
		glUniform4fvARB(gAvatarMatrixParam, 45, mat);
	}
}

//--------------------------------------------------------------------
// LLViewerJointMesh::drawBone()
//--------------------------------------------------------------------
void LLViewerJointMesh::drawBone()
{
}

//--------------------------------------------------------------------
// LLViewerJointMesh::isTransparent()
//--------------------------------------------------------------------
BOOL LLViewerJointMesh::isTransparent()
{
	return mIsTransparent;
}

//--------------------------------------------------------------------
// DrawElementsBLEND and utility code
//--------------------------------------------------------------------

// compate_int is used by the qsort function to sort the index array
int compare_int(const void *a, const void *b)
{
	if (*(U32*)a < *(U32*)b)
	{
		return -1;
	}
	else if (*(U32*)a > *(U32*)b)
	{
		return 1;
	}
	else return 0;
}

void llDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices)
{
	if (end-start+1 > (U32) gGLManager.mGLMaxVertexRange ||
		count > gGLManager.mGLMaxIndexRange)
	{
		glDrawElements(mode,count,type,indices);
	}
	else
	{
		glDrawRangeElements(mode,start,end,count,type,indices);
	}
}

//--------------------------------------------------------------------
// LLViewerJointMesh::drawShape()
//--------------------------------------------------------------------
U32 LLViewerJointMesh::drawShape( F32 pixelArea, BOOL first_pass)
{
	if (!mValid || !mMesh || !mFace || !mVisible || 
		mFace->mVertexBuffer.isNull() ||
		mMesh->getNumFaces() == 0) 
	{
		return 0;
	}

	U32 triangle_count = 0;

	stop_glerror();
	
	//----------------------------------------------------------------
	// setup current color
	//----------------------------------------------------------------
	if (!gRenderForSelect)
	{
		if ((mFace->getPool()->getVertexShaderLevel() > 0))
		{
			glColor4f(0,0,0,1);
			
			if (gMaterialIndex > 0)
			{
				glVertexAttrib4fvARB(gMaterialIndex, mColor.mV);
			}
			
			if (mShiny && gSpecularIndex > 0)
			{
				glVertexAttrib4fARB(gSpecularIndex, 1,1,1,1);
			}
		}
		else
		{
			glColor4fv(mColor.mV);
		}
	}

	stop_glerror();
	
	LLGLSSpecular specular(LLColor4(1.f,1.f,1.f,1.f), gRenderForSelect ? 0.0f : mShiny && !(mFace->getPool()->getVertexShaderLevel() > 0));

	LLGLEnable texture_2d((gRenderForSelect && isTransparent()) ? GL_TEXTURE_2D : 0);
	
	//----------------------------------------------------------------
	// setup current texture
	//----------------------------------------------------------------
	llassert( !(mTexture.notNull() && mLayerSet) );  // mutually exclusive

	if (mTestImageName)
	{
		LLImageGL::bindExternalTexture( mTestImageName, 0, GL_TEXTURE_2D ); 

		if (mIsTransparent)
		{
			glColor4f(1.f, 1.f, 1.f, 1.f);
		}
		else
		{
			glColor4f(0.7f, 0.6f, 0.3f, 1.f);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_COMBINE_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_INTERPOLATE_ARB);

			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,		GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,		GL_SRC_COLOR);

			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB,		GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB,		GL_SRC_COLOR);
			
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB,		GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB,		GL_ONE_MINUS_SRC_ALPHA);
		}
	}
	else if( mLayerSet )
	{
		if(	mLayerSet->hasComposite() )
		{
			mLayerSet->getComposite()->bindTexture();
		}
		else
		{
			llwarns << "Layerset without composite" << llendl;
			gImageList.getImage(IMG_DEFAULT)->bind();
		}
	}
	else
	if ( mTexture.notNull() )
	{
		mTexture->bind();
		if (!mTexture->getClampS()) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		}
		if (!mTexture->getClampT()) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
	}
	else
	{
		gImageList.getImage(IMG_DEFAULT_AVATAR)->bind();
	}
	
	LLGLDisable tex(gRenderForSelect && !isTransparent() ? GL_TEXTURE_2D : 0);

	if (gRenderForSelect)
	{
		if (isTransparent())
		{
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_COMBINE_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_REPLACE);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB,		GL_MODULATE);

			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,		GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,		GL_SRC_COLOR);

			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB,		GL_TEXTURE);  // GL_TEXTURE_ENV_COLOR is set in renderPass1
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB,	GL_SRC_ALPHA);
		}
	}
	else
	{
		//----------------------------------------------------------------
		// by default, backface culling is enabled
		//----------------------------------------------------------------
		/*if (sRenderPass == AVATAR_RENDER_PASS_CLOTHING_INNER)
		{
			LLImageGL::bindExternalTexture( sClothingMaskImageName, 1, GL_TEXTURE_2D );

			glClientActiveTextureARB(GL_TEXTURE0_ARB);
			glActiveTextureARB(GL_TEXTURE0_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_COMBINE_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_MODULATE);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB,		GL_REPLACE);

			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,		GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,		GL_SRC_COLOR);

			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB,		GL_PRIMARY_COLOR_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB,	GL_SRC_ALPHA);

			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB,		GL_PRIMARY_COLOR_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB,		GL_SRC_COLOR);

			glClientActiveTextureARB(GL_TEXTURE1_ARB);
			glEnable(GL_TEXTURE_2D); // Texture unit 1
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_COMBINE_ARB);
			glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR,	sClothingInnerColor.mV);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_INTERPOLATE_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB,		GL_REPLACE);

			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB,		GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB,	GL_SRC_ALPHA);

			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,		GL_CONSTANT_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,		GL_SRC_COLOR);

			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB,		GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB,		GL_SRC_COLOR);

			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB,		GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB,		GL_SRC_ALPHA);
		}
		else if (sRenderPass == AVATAR_RENDER_PASS_CLOTHING_OUTER)
		{
			glAlphaFunc(GL_GREATER, 0.1f);
			LLImageGL::bindExternalTexture( sClothingMaskImageName, 1, GL_TEXTURE_2D );

			glClientActiveTextureARB(GL_TEXTURE0_ARB);
			glActiveTextureARB(GL_TEXTURE0_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_COMBINE_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_MODULATE);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB,		GL_REPLACE);

			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,		GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,		GL_SRC_COLOR);

			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB,		GL_PRIMARY_COLOR_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB,	GL_SRC_ALPHA);

			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB,		GL_PRIMARY_COLOR_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB,		GL_SRC_COLOR);

			glClientActiveTextureARB(GL_TEXTURE1_ARB);
			glEnable(GL_TEXTURE_2D); // Texture unit 1
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_COMBINE_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_REPLACE);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB,		GL_MODULATE);

			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,		GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,		GL_SRC_COLOR);

			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB,		GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB,	GL_SRC_ALPHA);
		}*/
	}

	mFace->mVertexBuffer->setBuffer(sRenderMask);

	U32 start = mMesh->mFaceVertexOffset;
	U32 end = start + mMesh->mFaceVertexCount - 1;
	U32 count = mMesh->mFaceIndexCount;
	U32* indicesp = ((U32*) mFace->mVertexBuffer->getIndicesPointer()) + mMesh->mFaceIndexOffset;

	if (mMesh->hasWeights())
	{
		if ((mFace->getPool()->getVertexShaderLevel() > 0))
		{
			if (first_pass)
			{
				uploadJointMatrices();
			}
			llDrawRangeElements(GL_TRIANGLES, start, end, count, GL_UNSIGNED_INT, indicesp);
		}
		else
		{
			llDrawRangeElements(GL_TRIANGLES, start, end, count, GL_UNSIGNED_INT, indicesp);
		}
	}
	else
	{
		glPushMatrix();
		LLMatrix4 jointToWorld = getWorldMatrix();
		glMultMatrixf((GLfloat*)jointToWorld.mMatrix);
		llDrawRangeElements(GL_TRIANGLES, start, end, count, GL_UNSIGNED_INT, indicesp);
		glPopMatrix();
	}

	triangle_count += mMesh->mFaceIndexCount;
	
	if (mTestImageName)
	{
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	}

	/*if (sRenderPass != AVATAR_RENDER_PASS_SINGLE)
	{
		LLImageGL::unbindTexture(1, GL_TEXTURE_2D);
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_MODULATE);

		// Return to the default texture.
		LLImageGL::unbindTexture(0, GL_TEXTURE_2D);
		glClientActiveTextureARB(GL_TEXTURE0_ARB);
		glActiveTextureARB(GL_TEXTURE0_ARB);

		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_MODULATE);
		glAlphaFunc(GL_GREATER, 0.01f);
	}*/

	if (mTexture.notNull()) {
		if (!mTexture->getClampS()) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		}
		if (!mTexture->getClampT()) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}
	}

	return triangle_count;
}

//-----------------------------------------------------------------------------
// updateFaceSizes()
//-----------------------------------------------------------------------------
void LLViewerJointMesh::updateFaceSizes(U32 &num_vertices, U32& num_indices, F32 pixel_area)
{
	// Do a pre-alloc pass to determine sizes of data.
	if (mMesh && mValid)
	{
		mMesh->mFaceVertexOffset = num_vertices;
		mMesh->mFaceVertexCount = mMesh->getNumVertices();
		mMesh->mFaceIndexOffset = num_indices;
		mMesh->mFaceIndexCount = mMesh->getSharedData()->mNumTriangleIndices;

		mMesh->getReferenceMesh()->mCurVertexCount = mMesh->mFaceVertexCount;

		num_vertices += mMesh->getNumVertices();
		num_indices += mMesh->mFaceIndexCount;
	}
}

//-----------------------------------------------------------------------------
// updateFaceData()
//-----------------------------------------------------------------------------
void LLViewerJointMesh::updateFaceData(LLFace *face, F32 pixel_area, BOOL damp_wind)
{
	U32 i;
	
	mFace = face;

	LLStrider<LLVector3> verticesp;
	LLStrider<LLVector3> normalsp;
	LLStrider<LLVector3> binormalsp;
	LLStrider<LLVector2> tex_coordsp;
	LLStrider<F32>		 vertex_weightsp;
	LLStrider<LLVector4> clothing_weightsp;
	LLStrider<U32> indicesp;

	// Copy data into the faces from the polymesh data.
	if (mMesh && mValid)
	{
		if (mMesh->getNumVertices())
		{
			S32 index = face->getGeometryAvatar(verticesp, normalsp, binormalsp, tex_coordsp, vertex_weightsp, clothing_weightsp);
			face->mVertexBuffer->getIndexStrider(indicesp);

			if (-1 == index)
			{
				return;
			}

			for (i = 0; i < mMesh->getNumVertices(); i++)
			{
				verticesp[mMesh->mFaceVertexOffset + i] = *(mMesh->getCoords() + i);
				tex_coordsp[mMesh->mFaceVertexOffset + i] = *(mMesh->getTexCoords() + i);
				normalsp[mMesh->mFaceVertexOffset + i] = *(mMesh->getNormals() + i);
				binormalsp[mMesh->mFaceVertexOffset + i] = *(mMesh->getBinormals() + i);
				vertex_weightsp[mMesh->mFaceVertexOffset + i] = *(mMesh->getWeights() + i);
				if (damp_wind)
				{
					clothing_weightsp[mMesh->mFaceVertexOffset + i] = LLVector4(0,0,0,0);
				}
				else
				{
					clothing_weightsp[mMesh->mFaceVertexOffset + i] = (*(mMesh->getClothingWeights() + i));
				}
			}

			for (S32 i = 0; i < mMesh->getNumFaces(); i++)
			{
				for (U32 j = 0; j < 3; j++)
				{
					U32 k = i*3+j+mMesh->mFaceIndexOffset;
					indicesp[k] = mMesh->getFaces()[i][j] + mMesh->mFaceVertexOffset;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// updateLOD()
//-----------------------------------------------------------------------------
BOOL LLViewerJointMesh::updateLOD(F32 pixel_area, BOOL activate)
{
	BOOL valid = mValid;
	setValid(activate, TRUE);
	return (valid != activate);
}

// static
void LLViewerJointMesh::updateGeometryOriginal(LLFace *mFace, LLPolyMesh *mMesh)
{
	LLStrider<LLVector3> o_vertices;
	LLStrider<LLVector3> o_normals;

	//get vertex and normal striders
	LLVertexBuffer *buffer = mFace->mVertexBuffer;
	buffer->getVertexStrider(o_vertices,  0);
	buffer->getNormalStrider(o_normals,   0);

	F32 last_weight = F32_MAX;
	LLMatrix4 gBlendMat;
	LLMatrix3 gBlendRotMat;

	const F32* weights = mMesh->getWeights();
	const LLVector3* coords = mMesh->getCoords();
	const LLVector3* normals = mMesh->getNormals();
	for (U32 index = 0; index < mMesh->getNumVertices(); index++)
	{
		U32 bidx = index + mMesh->mFaceVertexOffset;
		
		// blend by first matrix
		F32 w = weights[index]; 
		
		// Maybe we don't have to change gBlendMat.
		// Profiles of a single-avatar scene on a Mac show this to be a very
		// common case.  JC
		if (w == last_weight)
		{
			o_vertices[bidx] = coords[index] * gBlendMat;
			o_normals[bidx] = normals[index] * gBlendRotMat;
			continue;
		}
		
		last_weight = w;

		S32 joint = llfloor(w);
		w -= joint;
		
		// No lerp required in this case.
		if (w == 1.0f)
		{
			gBlendMat = gJointMatUnaligned[joint+1];
			o_vertices[bidx] = coords[index] * gBlendMat;
			gBlendRotMat = gJointRotUnaligned[joint+1];
			o_normals[bidx] = normals[index] * gBlendRotMat;
			continue;
		}
		
		// Try to keep all the accesses to the matrix data as close
		// together as possible.  This function is a hot spot on the
		// Mac. JC
		LLMatrix4 &m0 = gJointMatUnaligned[joint+1];
		LLMatrix4 &m1 = gJointMatUnaligned[joint+0];
		
		gBlendMat.mMatrix[VX][VX] = lerp(m1.mMatrix[VX][VX], m0.mMatrix[VX][VX], w);
		gBlendMat.mMatrix[VX][VY] = lerp(m1.mMatrix[VX][VY], m0.mMatrix[VX][VY], w);
		gBlendMat.mMatrix[VX][VZ] = lerp(m1.mMatrix[VX][VZ], m0.mMatrix[VX][VZ], w);

		gBlendMat.mMatrix[VY][VX] = lerp(m1.mMatrix[VY][VX], m0.mMatrix[VY][VX], w);
		gBlendMat.mMatrix[VY][VY] = lerp(m1.mMatrix[VY][VY], m0.mMatrix[VY][VY], w);
		gBlendMat.mMatrix[VY][VZ] = lerp(m1.mMatrix[VY][VZ], m0.mMatrix[VY][VZ], w);

		gBlendMat.mMatrix[VZ][VX] = lerp(m1.mMatrix[VZ][VX], m0.mMatrix[VZ][VX], w);
		gBlendMat.mMatrix[VZ][VY] = lerp(m1.mMatrix[VZ][VY], m0.mMatrix[VZ][VY], w);
		gBlendMat.mMatrix[VZ][VZ] = lerp(m1.mMatrix[VZ][VZ], m0.mMatrix[VZ][VZ], w);

		gBlendMat.mMatrix[VW][VX] = lerp(m1.mMatrix[VW][VX], m0.mMatrix[VW][VX], w);
		gBlendMat.mMatrix[VW][VY] = lerp(m1.mMatrix[VW][VY], m0.mMatrix[VW][VY], w);
		gBlendMat.mMatrix[VW][VZ] = lerp(m1.mMatrix[VW][VZ], m0.mMatrix[VW][VZ], w);

		o_vertices[bidx] = coords[index] * gBlendMat;
		
		LLMatrix3 &n0 = gJointRotUnaligned[joint+1];
		LLMatrix3 &n1 = gJointRotUnaligned[joint+0];
		
		gBlendRotMat.mMatrix[VX][VX] = lerp(n1.mMatrix[VX][VX], n0.mMatrix[VX][VX], w);
		gBlendRotMat.mMatrix[VX][VY] = lerp(n1.mMatrix[VX][VY], n0.mMatrix[VX][VY], w);
		gBlendRotMat.mMatrix[VX][VZ] = lerp(n1.mMatrix[VX][VZ], n0.mMatrix[VX][VZ], w);

		gBlendRotMat.mMatrix[VY][VX] = lerp(n1.mMatrix[VY][VX], n0.mMatrix[VY][VX], w);
		gBlendRotMat.mMatrix[VY][VY] = lerp(n1.mMatrix[VY][VY], n0.mMatrix[VY][VY], w);
		gBlendRotMat.mMatrix[VY][VZ] = lerp(n1.mMatrix[VY][VZ], n0.mMatrix[VY][VZ], w);

		gBlendRotMat.mMatrix[VZ][VX] = lerp(n1.mMatrix[VZ][VX], n0.mMatrix[VZ][VX], w);
		gBlendRotMat.mMatrix[VZ][VY] = lerp(n1.mMatrix[VZ][VY], n0.mMatrix[VZ][VY], w);
		gBlendRotMat.mMatrix[VZ][VZ] = lerp(n1.mMatrix[VZ][VZ], n0.mMatrix[VZ][VZ], w);
		
		o_normals[bidx] = normals[index] * gBlendRotMat;
	}
}

const U32 UPDATE_GEOMETRY_CALL_MASK			= 0x1FFF; // 8K samples before overflow
const U32 UPDATE_GEOMETRY_CALL_OVERFLOW		= ~UPDATE_GEOMETRY_CALL_MASK;
static bool sUpdateGeometryCallPointer		= false;
static F64 sUpdateGeometryGlobalTime		= 0.0 ;
static F64 sUpdateGeometryElapsedTime		= 0.0 ;
static F64 sUpdateGeometryElapsedTimeOff	= 0.0 ;
static F64 sUpdateGeometryElapsedTimeOn		= 0.0 ;
static F64 sUpdateGeometryRunAvgOff[10];
static F64 sUpdateGeometryRunAvgOn[10];
static U32 sUpdateGeometryRunCount			= 0 ;
static U32 sUpdateGeometryCalls				= 0 ;
static U32 sUpdateGeometryLastProcessor		= 0 ;
void (*LLViewerJointMesh::sUpdateGeometryFunc)(LLFace* face, LLPolyMesh* mesh);

void LLViewerJointMesh::updateGeometry()
{
	extern BOOL gVectorizePerfTest;
	extern U32	gVectorizeProcessor;

	if (!(mValid
		  && mMesh
		  && mFace
		  && mMesh->hasWeights()
		  && mFace->mVertexBuffer.notNull()
		  && LLShaderMgr::getVertexShaderLevel(LLShaderMgr::SHADER_AVATAR) == 0))
	{
		return;
	}

	if (!gVectorizePerfTest)
	{
		// Once we've measured performance, just run the specified
		// code version.
		if(sUpdateGeometryFunc == updateGeometryOriginal)
			uploadJointMatrices();
		sUpdateGeometryFunc(mFace, mMesh);
	}
	else
	{
		// At startup, measure the amount of time in skinning and choose
		// the fastest one.
		LLTimer ug_timer ;
		
		if (sUpdateGeometryCallPointer)
		{
			if(sUpdateGeometryFunc == updateGeometryOriginal)
				uploadJointMatrices();
			// call accelerated version for this processor
			sUpdateGeometryFunc(mFace, mMesh);
		}
		else
		{
			uploadJointMatrices();
			updateGeometryOriginal(mFace, mMesh);
		}
	
		sUpdateGeometryElapsedTime += ug_timer.getElapsedTimeF64();
		++sUpdateGeometryCalls;
		if(0 != (sUpdateGeometryCalls & UPDATE_GEOMETRY_CALL_OVERFLOW))
		{
			F64 time_since_app_start = ug_timer.getElapsedSeconds();
			if(sUpdateGeometryGlobalTime == 0.0 
				|| sUpdateGeometryLastProcessor != gVectorizeProcessor)
			{
				sUpdateGeometryGlobalTime		= time_since_app_start;
				sUpdateGeometryElapsedTime		= 0;
				sUpdateGeometryCalls			= 0;
				sUpdateGeometryRunCount			= 0;
				sUpdateGeometryLastProcessor	= gVectorizeProcessor;
				sUpdateGeometryCallPointer		= false;
				return;
			}
			F64 percent_time_in_function = 
				( sUpdateGeometryElapsedTime * 100.0 ) / ( time_since_app_start - sUpdateGeometryGlobalTime ) ;
			sUpdateGeometryGlobalTime = time_since_app_start;
			if (!sUpdateGeometryCallPointer)
			{
				// First set of run data is with vectorization off.
				sUpdateGeometryCallPointer = true;
				llinfos << "profile (avg of " << sUpdateGeometryCalls << " samples) = "
					<< "vectorize off " << percent_time_in_function
					<< "% of time with "
					<< (sUpdateGeometryElapsedTime / (F64)sUpdateGeometryCalls)
					<< " seconds per call "
					<< llendl;
				sUpdateGeometryRunAvgOff[sUpdateGeometryRunCount] = percent_time_in_function;
				sUpdateGeometryElapsedTimeOff += sUpdateGeometryElapsedTime;
				sUpdateGeometryCalls = 0;
			}
			else
			{
				// Second set of run data is with vectorization on.
				sUpdateGeometryCallPointer = false;
				llinfos << "profile (avg of " << sUpdateGeometryCalls << " samples) = "
					<< "VEC on " << percent_time_in_function
					<< "% of time with "
					<< (sUpdateGeometryElapsedTime / (F64)sUpdateGeometryCalls)
					<< " seconds per call "
					<< llendl;
				sUpdateGeometryRunAvgOn[sUpdateGeometryRunCount] = percent_time_in_function ;
				sUpdateGeometryElapsedTimeOn += sUpdateGeometryElapsedTime;

				sUpdateGeometryCalls = 0;
				sUpdateGeometryRunCount++;
				F64 a = 0.0, b = 0.0;
				for(U32 i = 0; i<sUpdateGeometryRunCount; i++)
				{
					a += sUpdateGeometryRunAvgOff[i];
					b += sUpdateGeometryRunAvgOn[i];
				}
				a /= sUpdateGeometryRunCount;
				b /= sUpdateGeometryRunCount;
				F64 perf_boost = ( sUpdateGeometryElapsedTimeOff - sUpdateGeometryElapsedTimeOn ) / sUpdateGeometryElapsedTimeOn;
				llinfos << "run averages (" << (F64)sUpdateGeometryRunCount
					<< "/10) vectorize off " << a
					<< "% : vectorize type " << gVectorizeProcessor
					<< " " << b
					<< "% : performance boost " 
					<< perf_boost * 100.0
					<< "%"
					<< llendl ;
				if(sUpdateGeometryRunCount == 10)
				{
					// In case user runs test again, force reset of data on
					// next run.
					sUpdateGeometryGlobalTime = 0.0;

					// We have data now on which version is faster.  Switch to that
					// code and save the data for next run.
					gVectorizePerfTest = FALSE;
					gSavedSettings.setBOOL("VectorizePerfTest", FALSE);

					if (perf_boost > 0.0)
					{
						llinfos << "Vectorization improves avatar skinning performance, "
							<< "keeping on for future runs."
							<< llendl;
						gSavedSettings.setBOOL("VectorizeSkin", TRUE);
					}
					else
					{
						// SIMD decreases performance, fall back to original code
						llinfos << "Vectorization decreases avatar skinning performance, "
							<< "switching back to original code."
							<< llendl;

						gSavedSettings.setBOOL("VectorizeSkin", FALSE);
					}
				}
			}
			sUpdateGeometryElapsedTime = 0.0f;
		}
	}
}

void LLViewerJointMesh::dump()
{
	if (mValid)
	{
		llinfos << "Usable LOD " << mName << llendl;
	}
}

void LLViewerJointMesh::writeCAL3D(apr_file_t* fp, S32 material_num, LLCharacter* characterp)
{
	apr_file_printf(fp, "\t<SUBMESH NUMVERTICES=\"%d\" NUMFACES=\"%d\" MATERIAL=\"%d\" NUMLODSTEPS=\"0\" NUMSPRINGS=\"0\" NUMTEXCOORDS=\"1\">\n", mMesh->getNumVertices(), mMesh->getNumFaces(), material_num);

	const LLVector3* mesh_coords = mMesh->getCoords();
	const LLVector3* mesh_normals = mMesh->getNormals();
	const LLVector2* mesh_uvs = mMesh->getTexCoords();
	const F32* mesh_weights = mMesh->getWeights();
	LLVector3 mesh_offset;
	LLVector3 scale(1.f, 1.f, 1.f);
	S32 joint_a = -1;
	S32 joint_b = -1;
	S32 num_bound_joints = 0;
	
	if(!mMesh->hasWeights())
	{
		num_bound_joints = 1;
		LLJoint* cur_joint = this;
		while(cur_joint)
		{
			if (cur_joint->mJointNum != -1 && joint_a == -1)
			{
				joint_a = cur_joint->mJointNum;
			}
			mesh_offset += cur_joint->getSkinOffset();
			cur_joint = cur_joint->getParent();
		}
	}

	for (S32 i = 0; i < (S32)mMesh->getNumVertices(); i++)
	{
		LLVector3 coord = mesh_coords[i];

		if (mMesh->hasWeights())
		{
			// calculate joint to which this skinned vertex is bound
			num_bound_joints = getBoundJointsByIndex(llfloor(mesh_weights[i]), joint_a, joint_b);
			LLJoint* first_joint = characterp->getCharacterJoint(joint_a);
			LLJoint* second_joint = characterp->getCharacterJoint(joint_b);

			LLVector3 first_joint_offset;
			LLJoint* cur_joint = first_joint;
			while(cur_joint)
			{
				first_joint_offset += cur_joint->getSkinOffset();
				cur_joint = cur_joint->getParent();
			}

			LLVector3 second_joint_offset;
			cur_joint = second_joint;
			while(cur_joint)
			{
				second_joint_offset += cur_joint->getSkinOffset();
				cur_joint = cur_joint->getParent();
			}

			LLVector3 first_coord = coord - first_joint_offset;
			first_coord.scaleVec(first_joint->getScale());
			LLVector3 second_coord = coord - second_joint_offset;
			if (second_joint)
			{
				second_coord.scaleVec(second_joint->getScale());
			}
			
			coord = lerp(first_joint_offset + first_coord, second_joint_offset + second_coord, fmodf(mesh_weights[i], 1.f));
		}

		// add offset to move rigid mesh to target location
		coord += mesh_offset;
		coord *= 100.f;

		apr_file_printf(fp, "		<VERTEX ID=\"%d\" NUMINFLUENCES=\"%d\">\n", i, num_bound_joints);
		apr_file_printf(fp, "			<POS>%.4f %.4f %.4f</POS>\n", coord.mV[VX], coord.mV[VY], coord.mV[VZ]);
		apr_file_printf(fp, "			<NORM>%.6f %.6f %.6f</NORM>\n", mesh_normals[i].mV[VX], mesh_normals[i].mV[VY], mesh_normals[i].mV[VZ]);
		apr_file_printf(fp, "			<TEXCOORD>%.6f %.6f</TEXCOORD>\n", mesh_uvs[i].mV[VX], 1.f - mesh_uvs[i].mV[VY]);
		if (num_bound_joints >= 1)
		{
			apr_file_printf(fp, "			<INFLUENCE ID=\"%d\">%.2f</INFLUENCE>\n", joint_a + 1, 1.f - fmod(mesh_weights[i], 1.f));
		}
		if (num_bound_joints == 2)
		{
			apr_file_printf(fp, "			<INFLUENCE ID=\"%d\">%.2f</INFLUENCE>\n", joint_b + 1, fmod(mesh_weights[i], 1.f));
		}
		apr_file_printf(fp, "		</VERTEX>\n");
	}

	LLPolyFace* mesh_faces = mMesh->getFaces();
	for (S32 i = 0; i < mMesh->getNumFaces(); i++)
	{
		apr_file_printf(fp, "		<FACE VERTEXID=\"%d %d %d\" />\n", mesh_faces[i][0], mesh_faces[i][1], mesh_faces[i][2]);
	}
	
	apr_file_printf(fp, "	</SUBMESH>\n");
}

// End
