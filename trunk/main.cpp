/*************************************************************
* Copyright (c) 2010 by Egor N. Chashchin. All Rights Reserved.          *
**************************************************************/

/*
*	main.cpp - Scallop - system for generating and visualization of attraction areas
*	of stochastic non-linear attractors. Dso entry point
*
*	Version: 0.97
*	Authors: Egor N. Chashchin
*	Contact: iqcook@gmail.com
*
*/
#ifdef LINUX
#define DLLEXPORT
#define SIZEOF_VOID_P 8
#else
#define DLLEXPORT __declspec(dllexport)
#endif
#define MAKING_DSO

#define SESI_LITTLE_ENDIAN 1

// CRT
#include <sys\stat.h>

#include <limits.h>
#include <strstream>

#include <iostream>
using namespace std;

// H
#include <UT/UT_DSOVersion.h>

#include <GU/GU_Detail.h>
#include <GU/GU_PrimVolume.h>
#include <OBJ/OBJ_Node.h>
#include <SOP/SOP_Node.h>

#include <PRM/PRM_Include.h>
#include <PRM/PRM_Template.h>
#include <PRM/PRM_SpareData.h>
#include <PRM/PRM_ChoiceList.h>

#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <OP/OP_Caller.h>

#include <UT/UT_Vector3.h>
#include <UT/UT_Ramp.h>
#include <UT/UT_Interrupt.h>

#include <VEX/VEX_Error.h>
#include <CVEX/CVEX_Context.h>
#include <CVEX/CVEX_Value.h>

#include <SHOP/SHOP_Node.h>

#include "SOP_Scallop.cpp"

//////////////////////////////////////////////////////////////////////////

void newSopOperator(OP_OperatorTable *table)
{
	table->addOperator(new OP_Operator(
		"hdk_scallop","Scallop",
		SOP_Scallop::creator,SOP_Scallop::templateList,
		0,1));
	//table->addOperator(new OP_Operator(
	//	"hdk_savescallopptc","Scallop: Save Ptc",
	//	SOP_SaveScallopPtc::creator,SOP_SaveScallopPtc::templateList,
	//	1,1));
	//table->addOperator(new OP_Operator(
	//	"hdk_pickptc","Scallop: Copy from Ptc",
	//	SOP_PickPtc::creator,SOP_PickPtc::templateList,
	//	1,1));
	//table->addOperator(new OP_Operator(
	//	"hdk_voxelizeptc","Scallop: Voxelize",
	//	SOP_Voxelize::creator,SOP_Voxelize::templateList,
	//	1,1));
}