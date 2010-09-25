/*************************************************************
* Copyright (c) 2010 by Egor N. Chashchin. All Rights Reserved.          *
**************************************************************/

/*
*	SOP_PickPtc.cpp - Scallop - system for generating and visualization of attraction areas
*	of stochastic non-linear attractors. SOP_PickPtc - SOP-node for pointcloud splicing
*
*	Version: 0.95
*	Authors: Egor N. Chashchin
*	Contact: iqcook@gmail.com
*
*/

int thecallbackfuncarch(void *data, int index, float time,const PRM_Template *tplate);

#define CHUNCKSIZE 4194304

class Buffer
{
public:
	Buffer() : fp(NULL) , count(0) , size(0) {};
	~Buffer() { Close(); };

	bool Open(const char *);
	bool Save(float*, int size);
	int Next() { count++; return count;  };
	bool Close();

private:
	float D[CHUNCKSIZE];
	FILE* fp;
	int count;
	int size;
};

bool Buffer::Open(const char *f)
{
	Close();
	count = 0;
	fp = fopen(f,"wb");
	if(fp == NULL) return false;

	fwrite(&count,sizeof(int),1,fp);
	return true;
};
bool Buffer::Save(float*_D, int sz)
{
	if(fp == NULL) return false;

	if(sz+size < CHUNCKSIZE)
	{
		memcpy(D+size,_D,4*sz);
		size+=sz;
		return true;
	};

	fwrite(D,sizeof(float),size,fp);

	size=sz;
	memcpy(D,_D,4*sz);

	return true;
};
bool Buffer::Close()
{
	if(fp != NULL)
	{
		// WRITE COUNT
		if(size!=0) fwrite(D,sizeof(float),size,fp);
		fseek(fp,0,0);
		fwrite(&count,sizeof(int),1,fp);
		fclose(fp);
	}
	fp = NULL;
	size = 0;
	return true;
};


class Chunck
{
public:
	Chunck() {};
	UT_BoundingBox box;
	UT_String path;
	void Setup();
	bool Test(float* data);
	bool Save();

	Buffer B;

	static bool plain;
private:
	//GU_Detail geo;


	//int ct, wt, dt, rt, ptt;
	int count;
};

bool Chunck::plain = false;

void Chunck::Setup()
{
	count = 0;
	/*path = "";
	int zeroi = 0;
	ct = geo.addAttrib("count",4,GB_ATTRIB_INT,&zeroi);

	float zero=0.0f;
	wt = geo.addAttrib("radius",4,GB_ATTRIB_FLOAT,&zero);

	dt = geo.addDiffuseAttribute(GEO_POINT_DICT);
	geo.addVariableName("Cd","Cd");

	float one=1.0f;
	rt = geo.addPointAttrib("width",4,GB_ATTRIB_FLOAT,&one);
	geo.addVariableName("width","WIDTH");

	ptt = geo.addPointAttrib("parameter",4,GB_ATTRIB_FLOAT,&zero);
	geo.addVariableName("parameter","PARAMETER");

	geo.addAttrib("showpoints",4,GB_ATTRIB_FLOAT,&one);
	geo.addAttrib("revealsize",4,GB_ATTRIB_FLOAT,&one);*/
};

bool Chunck::Test(float* data)
{
	float* P = data;

	UT_Vector3 pos(P);
	if(!box.isInside(pos)) return false;

	B.Save(data,8);
	B.Next();

	/*float* R = data+3;
	float* Cd = data+4;
	float* T = data+5;

	GEO_Point* pt = geo.appendPoint();
	pt->setPos(pos);

	float* w = pt->castAttribData<float>(rt);
	*w=*R;

	float* p = pt->castAttribData<float>(ptt);
	*p=*T;

	float* C = pt->castAttribData<float>(dt);
	memcpy(C,Cd,12);*/

	count++;

	return true;
};

bool Chunck::Save()
{
	/*GEO_PointList& pl = geo.points();
	int _count = pl.entries();
	if(_count == 0) return false;

	if(!plain)
	{
		// SET CNT
		{
			int *attrib_data = (int *) geo.attribs().getAttribData(ct);
			*attrib_data=count;
		}
		// SET RADIUS
		{
			float radius = 0;
			for(int i=0;i<_count;i++)
			{
				GEO_Point* pt = pl[i];
				float* w = pt->castAttribData<float>(rt);
				if(*w > radius) radius = *w;
			};
			float *attrib_data = (float *) geo.attribs().getAttribData(wt);
			*attrib_data=radius;
		}
		// SAVE
		geo.save(path,1,NULL);
		return true;
	};

	FILE* fp = fopen(path.buffer(),"wb");

	fwrite(&_count,sizeof(int),1,fp);

	float D[8];

	float* P = D;
	float* R = P+3;
	float* Cd = R+1;
	float* T = Cd+3;

	for(int i=0;i<count;i++)
	{
		GEO_Point* pt = pl[i];
		UT_Vector3 v = pt->getPos();

		memcpy(P,v.vec,12);

		float* w = pt->castAttribData<float>(rt);
		*R=*w;

		float* p = pt->castAttribData<float>(ptt);
		*T=*p;

		float* C = pt->castAttribData<float>(dt);
		memcpy(Cd,C,12);

		fwrite(D,32,1,fp); // P
	};

	fclose(fp);*/
	return true;
};

class SOP_PickPtc :
	public SOP_Node
{
public:
	SOP_PickPtc(OP_Network *net, const char *name, OP_Operator *entry) : SOP_Node(net,name,entry)
	{
		if (!ifdIndirect) ifdIndirect = allocIndirect(256);
	};
	virtual ~SOP_PickPtc() {};

	static OP_Node	*creator(OP_Network  *net, const char *name,	OP_Operator *entry);
	static PRM_Template	 templateList[];

	void SaveArchive(float);
	void SaveVolArchive(float);

protected:
	virtual OP_ERROR	 cookMySop   (OP_Context &context);

	static int* ifdIndirect;
};

int* SOP_PickPtc::ifdIndirect = 0;

PRM_Name cloudPathName("cloudpath","Cloud Data");
PRM_Default cloudpathDef(0.0,"");

PRM_Name chuncksCountName("chunks","Chuncks");

PRM_Name tglPlainName("plain","Plain Data");

PRM_Name btnSpoolRI("saveri","Save Rib Archive");

PRM_Template SOP_PickPtc::templateList[]=
{
	// SPOOL SETUP
	PRM_Template(PRM_FILE,1,&cloudPathName,&cloudpathDef),
	PRM_Template(PRM_INT,1,&chuncksCountName,PRMfourDefaults),
	PRM_Template(PRM_FILE,1,&savePathName,&savePathDef),
	PRM_Template(PRM_TOGGLE,1,&tglPlainName,PRMzeroDefaults),
	PRM_Template(PRM_CALLBACK,	1, &btnSpoolRI, NULL, NULL, NULL,thecallbackfuncarch),

	PRM_Template()
};

OP_Node *SOP_PickPtc::creator(OP_Network *net,const char *name,OP_Operator *entry)
{
	return new SOP_PickPtc(net, name, entry);
};

OP_ERROR SOP_PickPtc::cookMySop(OP_Context &context)
{
	gdp->clearAndDestroy();
	float now = context.getTime();

	if(lockInputs(context) >= UT_ERROR_ABORT) return error();
	const GU_Detail* input = inputGeo(0,context);

	// SPOOL
	UT_String path;
	STR_PARM(path,"cloudpath", 0, 0, now);

	if(path == "") return error();

	UT_String savepath;
	STR_PARM(savepath,"path", 2, 0, now);

	FILE* fp = fopen(path.buffer(),"rb");

	if(fp == NULL) return error();

	int count = 0;
	fread(&count,sizeof(int),1,fp);
	fclose(fp);

	if(count < 1) return error();

	int chunks = evalInt("chunks",&ifdIndirect[1],0,now);

	Chunck::plain = (evalInt("plain",&ifdIndirect[1],0,now)!=0);

	const GEO_PrimList& pl = input->primitives();

	if(pl.entries() == 0) return error();

	//int ecnt = 0;

	int passes = pl.entries()/chunks+1;

	Chunck* C = new Chunck[chunks];
	//int index = 0;
	for(int i=0;i<passes;i++)
	{
		for(int j=0;j<chunks;j++) C[j].Setup();
		cout << i << "-th pass" << endl;
		int amt = (i!=passes-1 ? chunks : pl.entries()%chunks);

		for(int j=0;j<amt;j++)
		{
			const GEO_PrimVolume* v = dynamic_cast<const GEO_PrimVolume*>(pl[i*chunks+j]);
			UT_BoundingBox b;
			v->getBBox(&b);
			C[j].box = b;

			UT_String path = "";
			path.sprintf("%s.%d.ptc",savepath.buffer(),i*chunks+j);
			C[j].B.Open(path.buffer());
		};

		FILE* fp = fopen(path.buffer(),"rb");

		if(fp == NULL) return error();

		fread(&count,sizeof(int),1,fp);

		float data[8];
		for(int j=0;j<count;j++)
		{
			fread(data,4,8,fp);
			for(int k=0;k<amt;k++)
			{
				if(C[k].Test(data)) break;
			};
		};

		//for(int k=0;k<amt;k++)
		//{
		//	C[k].B.Close();
		//	//C[k].Save();
		//	//C[k].geo.clearAndDestroy();
		//};

		fclose(fp);
	};
	unlockInputs();
	delete [] C;

	return error();
};

int thecallbackfuncarch(void *data, int index, float time,const PRM_Template *tplate)
{
	SOP_PickPtc *me = (SOP_PickPtc *) data;
	me->SaveArchive(time);
	return 0;
};

void SOP_PickPtc::SaveArchive(float now)
{
	UT_String savepath;
	STR_PARM(savepath,"path", 2, 0, now);

	UT_String buf;

	OP_Context context(now);

	if(lockInputs(context) >= UT_ERROR_ABORT) return;
	const GU_Detail* input = inputGeo(0,context);

	const GEO_PrimList& pl = input->primitives();
	int cnt = pl.entries();

	if(cnt == 0) return;

	buf.sprintf("%s.rib",savepath.buffer());

	ofstream fout( buf.buffer()/*, ios_base::out*/);

	float radius = 0;
	int rt = input->findAttrib("radius",4,GB_ATTRIB_FLOAT);
	if(rt != -1)
	{
		const float *attrib_data = (const float *) input->attribs().getAttribData(rt);
		radius=*attrib_data;
	};

	for(int i=0;i<cnt;i++)
	{
		const GEO_PrimVolume* v = dynamic_cast<const GEO_PrimVolume*>(pl[i]);
		if(v==NULL) continue;

		fout << "Attribute \"user\" \"float id\" [" << float(i)/float(cnt) << "]" << endl;

		UT_BoundingBox b;
		v->getBBox(&b);

		UT_String path;
		path.sprintf("%s.%d.ptc",savepath.buffer(),i);

		fout << "Procedural \"DynamicLoad\" [\"DPROC_scpoint\" \"" << path.buffer() << "\"] [";

		UT_Vector3 mIN = b.minvec();
		UT_Vector3 mAX = b.maxvec();

		fout << mIN.x()-radius << " " << mAX.x()+radius << " ";
		fout << mIN.y()-radius << " " << mAX.y()+radius << " ";
		fout << mIN.z()-radius << " " << mAX.z()+radius << "]" << endl;
	}

	unlockInputs();
};

