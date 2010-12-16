/*************************************************************
* Copyright (c) 2010 by Egor N. Chashchin. All Rights Reserved.          *
**************************************************************/

/*
*	SOP_PickPtc.cpp - Scallop - system for generating and visualization of attraction areas
*	of stochastic non-linear attractors. SOP_SaveScallopPtc - SOP-node for pointcloud saving
*
*	Version: 0.95
*	Authors: Egor N. Chashchin
*	Contact: iqcook@gmail.com
*
*/

//#define INT_PARM(name, idx, vi, t) { return evalInt(name, &ifdIndirect[idx], vi, t); }
//#define STR_PARM(str, name, idx, vi, t) { evalString(str, name, &ifdIndirect[idx], vi, (float)t); }

class SOP_SaveScallopPtc :
	public SOP_Node
{
public:
	SOP_SaveScallopPtc(OP_Network *net, const char *name, OP_Operator *entry) : SOP_Node(net,name,entry)
	{
		if (!ifdIndirect) ifdIndirect = allocIndirect(256);
	};
	virtual ~SOP_SaveScallopPtc() {};

	static OP_Node	*creator(OP_Network  *net, const char *name,	OP_Operator *entry);
	static PRM_Template	 templateList[];

protected:
	virtual OP_ERROR	 cookMySop   (OP_Context &context);

	static int* ifdIndirect;
};

int* SOP_SaveScallopPtc::ifdIndirect = 0;

PRM_Name transferName("transfer","Transfer");
PRM_Name enableName("enable","Enable");

PRM_Template SOP_SaveScallopPtc::templateList[]=
{
	// SPOOL SETUP
	PRM_Template(PRM_TOGGLE,1,&enableName,PRMoneDefaults),
	PRM_Template(PRM_TOGGLE,1,&transferName,PRMoneDefaults),
	PRM_Template(PRM_FILE,1,&savePathName,&savePathDef),
	PRM_Template()
};

OP_Node *SOP_SaveScallopPtc::creator(OP_Network *net,const char *name,OP_Operator *entry)
{
	return new SOP_SaveScallopPtc(net, name, entry);
};

OP_ERROR SOP_SaveScallopPtc::cookMySop(OP_Context &context)
{
	gdp->clearAndDestroy();
	float now = context.getTime();
	bool enable = (evalInt("enable",&ifdIndirect[0],0,now)!=0);
	bool transfer = (evalInt("transfer",&ifdIndirect[1],0,now)!=0);

	// NOT ENABLE
	if(!enable)
	{
		if(lockInputs(context) >= UT_ERROR_ABORT) return error();
		const GU_Detail* input = inputGeo(0,context);
		if(transfer)
		{
			gdp->copy(*input);
		};
		unlockInputs();
		return error();
	};

	// SPOOL
	UT_String path;
	STR_PARM(path,"path", 2, 0, now);

	if(path == "") return error();

	if(lockInputs(context) >= UT_ERROR_ABORT) return error();
	const GU_Detail* input = inputGeo(0,context);

	GB_AttributeRef dt = input->findDiffuseAttribute(GEO_POINT_DICT);
	GB_AttributeRef wt = input->findPointAttrib("width",GB_ATTRIB_FLOAT);
	GB_AttributeRef pt = input->findPointAttrib("parameter",GB_ATTRIB_FLOAT);

	if(!GBisAttributeRefValid(dt))
	{
		// NOTHING TO SAVE!
		unlockInputs();
		return error();
	};

	if(!GBisAttributeRefValid(wt))
	{
		// NOTHING TO SAVE!
		unlockInputs();
		return error();
	};

	if(!GBisAttributeRefValid(pt))
	{
		// NOTHING TO SAVE!
		unlockInputs();
		return error();
	};

	const GEO_PointList& pl = input->points();
	int count = pl.entries();
	if(count == 0) return error();

	FILE* fp = fopen(path.buffer(),"wb");

	if(fp == NULL)
	{
		unlockInputs();
		return error();
	}

	fwrite(&count,sizeof(int),1,fp);

	float D[8];

	float* P = D;
	float* R = P+3;
	float* Cd = R+1;
	float* T = Cd+3;

	for(int i=0;i<count;i++)
	{
		const GEO_Point* _p = pl[i];
		UT_Vector3 v = _p->getPos();

		memcpy(P,v.vec,12);

		const float* w = _p->castAttribData<float>(wt);
		*R=*w;

		const float* p = _p->castAttribData<float>(pt);
		*T=*p;

		const float* C = _p->castAttribData<float>(dt);
		memcpy(Cd,C,12);

		fwrite(D,32,1,fp); // P
	};

	fclose(fp);

	// COPY IF NEEDED
	if(transfer) gdp->copy(*input);
	unlockInputs();

	return error();
};