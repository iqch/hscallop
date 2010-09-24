int thecallbackfuncarchvol(void *data, int index, float time,const PRM_Template *tplate);

class VoxelBox
{
public:
	VoxelBox() {};
	void Setup(int x, int y, int z)
	{
		R.size(x,y,z);
		G.size(x,y,z);
		B.size(x,y,z);
		div[0]=x; div[1]=y; div[2]=z;
		count = 0;
	};
	
	UT_BoundingBox box;

	bool Test(float* data);
	bool Normalize();

	UT_VoxelArrayF R;
	UT_VoxelArrayF G;
	UT_VoxelArrayF B;

	int div[3];

private:
	int count;
};

bool VoxelBox::Test(float* data)
{
	float* P = data;
	float* C = P+4;

	UT_Vector3 pos(P);
	if(!box.isInside(pos)) return false;

	// EMBED POINT

	int c[3];

	UT_Vector3 miN = box.minvec();

	c[0] = (P[0]-miN.x())/box.sizeX()*div[0]; if(c[0] >= div[0]) c[0]=div[0]-1; if(c[0] < 0) c[0]=0;
	c[1] = (P[1]-miN.y())/box.sizeY()*div[1]; if(c[1] >= div[1]) c[1]=div[1]-1; if(c[1] < 0) c[1]=0;
	c[2] = (P[2]-miN.z())/box.sizeZ()*div[2]; if(c[2] >= div[2]) c[2]=div[2]-1; if(c[2] < 0) c[2]=0;

	float r = R.getValue(c[0],c[1],c[2]);
	float g = G.getValue(c[0],c[1],c[2]);
	float b = B.getValue(c[0],c[1],c[2]);

	R.setValue(c[0],c[1],c[2],r+C[0]);
	G.setValue(c[0],c[1],c[2],g+C[1]);
	B.setValue(c[0],c[1],c[2],b+C[2]);

	count++;

	return true;
};

bool VoxelBox::Normalize()
{
	for(int i=0;i<div[0];i++)
	for(int j=0;j<div[1];j++)
	for(int k=0;k<div[2];k++)
	{
		float r = R.getValue(i,j,k);
		R.setValue(i,j,k,min(r,1.0f));
		float g = G.getValue(i,j,k);
		G.setValue(i,j,k,min(g,1.0f));
		float b = B.getValue(i,j,k);
		B.setValue(i,j,k,min(b,1.0f));
	};
	return true;
};

class SOP_Voxelize :
	public SOP_Node
{
public:
	SOP_Voxelize(OP_Network *net, const char *name, OP_Operator *entry) : SOP_Node(net,name,entry) 
	{
		if (!ifdIndirect) ifdIndirect = allocIndirect(256);
	};
	virtual ~SOP_Voxelize() {};

	static OP_Node	*creator(OP_Network  *net, const char *name,	OP_Operator *entry);
	static PRM_Template	 templateList[];

	void SaveVolArchive(float);

protected:
	virtual OP_ERROR	 cookMySop   (OP_Context &context);

	static int* ifdIndirect;
};

int* SOP_Voxelize::ifdIndirect = 0;

PRM_Name useUniformGridName("useunfgr","Use Uniform Spacing");
PRM_Name uniformGridName("unfgr","Spacing");
PRM_Default unfDef[] = { PRM_Default(5,""), PRM_Default(5,""), PRM_Default(5,"") };

PRM_Name voxelSizeName("voxelsz","Voxel Size");
PRM_Default vszDef(0.05,"");

PRM_Name btnSpoolVolumeRI("savevolri","Save Volume Rib Archive");

PRM_Template SOP_Voxelize::templateList[]=
{
	// SPOOL SETUP
	PRM_Template(PRM_FILE,1,&cloudPathName,&cloudpathDef),
	PRM_Template(PRM_INT,1,&chuncksCountName,PRMfourDefaults),

	PRM_Template(PRM_TOGGLE,1,&useUniformGridName,PRMzeroDefaults),
	PRM_Template(PRM_INT,3,&uniformGridName,unfDef),
	PRM_Template(PRM_FLT,1,&voxelSizeName,&vszDef),

	PRM_Template(PRM_CALLBACK,	1, &btnSpoolVolumeRI, NULL, NULL, NULL,thecallbackfuncarchvol),	
	PRM_Template(PRM_FILE,1,&savePathName,&savePathDef),

	PRM_Template()
};

OP_Node *SOP_Voxelize::creator(OP_Network *net,const char *name,OP_Operator *entry)
{
	return new SOP_Voxelize(net, name, entry);
};

float func(const UT_Vector3&);
OP_ERROR SOP_Voxelize::cookMySop(OP_Context &context)
{
	gdp->clearAndDestroy();
	float now = context.getTime();

	if(lockInputs(context) >= UT_ERROR_ABORT) return error();
	const GU_Detail* input = inputGeo(0,context);

	// SPOOL
	UT_String path;
	STR_PARM(path,"cloudpath", 0, 0, now);

	if(path == "") return error();

	FILE* fp = fopen(path.buffer(),"rb");

	if(fp == NULL) return error();

	int count = 0;
	fread(&count,sizeof(int),1,fp);
	fclose(fp);

	if(count < 1) return error();

	int cdi = gdp->addDiffuseAttribute(GEO_PRIMITIVE_DICT);

	int chunks = evalInt("chunks",&ifdIndirect[1],0,now);

	bool useDim = (evalInt("useunfgr",&ifdIndirect[2],0,now)!= 0);

	float vsize = 0.01;

	int dim[3] = {5,5,5};
	if(useDim)
	{
		dim[0] = evalInt("unfgr",&ifdIndirect[3],0,now);
		dim[1] = evalInt("unfgr",&ifdIndirect[3],1,now);
		dim[2] = evalInt("unfgr",&ifdIndirect[3],2,now);
	}
	else
	{
		vsize = evalFloat("voxelsz",&ifdIndirect[4],0,now);
	}

	const GEO_PrimList& pl = input->primitives();

	if(pl.entries() == 0) return error();

	int passes = pl.entries()/chunks+1;

	VoxelBox* C = new VoxelBox[chunks];
	//int index = 0;
	for(int i=0;i<passes;i++)
	{
		cout << i << "-th pass" << endl;

		int amt = (i!=passes-1 ? chunks : pl.entries()%chunks);

		for(int j=0;j<amt;j++)
		{
			const GEO_PrimVolume* v = dynamic_cast<const GEO_PrimVolume*>(pl[i*chunks+j]);
			UT_BoundingBox b;
			v->getBBox(&b);
			C[j].box = b;

			if(!useDim)
			{
				dim[0] = b.sizeX()/vsize+1;
				dim[1] = b.sizeY()/vsize+1;
				dim[2] = b.sizeZ()/vsize+1;
			};

			C[j].Setup(dim[0],dim[1],dim[2]);
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

		fclose(fp);

		for(int j=0;j<amt;j++)
		{
			C[j].Normalize();

			GEO_Primitive* r = GU_PrimVolume::buildFromFunction(gdp,func,C[j].box,C[j].div[0],C[j].div[1],C[j].div[2]);
			GU_PrimVolume* R = dynamic_cast<GU_PrimVolume*>(r);
			R->setVoxels(&C[j].R);

			float* _r = R->castAttribData<float>(cdi);
			_r[0]=1; _r[1]=0; _r[2]=0;

			GEO_Primitive* g = GU_PrimVolume::buildFromFunction(gdp,func,C[j].box,C[j].div[0],C[j].div[1],C[j].div[2]);
			GU_PrimVolume* G = dynamic_cast<GU_PrimVolume*>(g);
			G->setVoxels(&C[j].G);

			float* _g = G->castAttribData<float>(cdi);
			_g[0]=0; _g[1]=1; _g[2]=0;

			GEO_Primitive* b = GU_PrimVolume::buildFromFunction(gdp,func,C[j].box,C[j].div[0],C[j].div[1],C[j].div[2]);
			GU_PrimVolume* B = dynamic_cast<GU_PrimVolume*>(b);
			B->setVoxels(&C[j].B);

			float* _b = B->castAttribData<float>(cdi);
			_b[0]=0; _b[1]=0; _b[2]=1;
		}
	};
	unlockInputs();
	delete [] C;

	return error();
};

int thecallbackfuncarchvol(void *data, int index, float time,const PRM_Template *tplate)
{
	SOP_Voxelize *me = (SOP_Voxelize *) data;
	me->SaveVolArchive(time);
	return 0;
};

void SOP_Voxelize::SaveVolArchive(float now)
{
	UT_String savepath;
	STR_PARM(savepath,"path", 6, 0, now);

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

	bool useDim = (evalInt("useunfgr",&ifdIndirect[2],0,now)!= 0);

	float vsize = 0.01;

	int dim[3] = {5,5,5};
	if(useDim)
	{
		dim[0] = evalInt("unfgr",&ifdIndirect[3],0,now);
		dim[1] = evalInt("unfgr",&ifdIndirect[3],1,now);
		dim[2] = evalInt("unfgr",&ifdIndirect[3],2,now);

		fout << "Attribute \"user\""
			<< " \"int dim0\" " << dim[0]
		<< " \"int dim1\" " << dim[1]
		<< " \"int dim2\" " << dim[2]
		<< endl;
	}
	else
	{
		vsize = evalFloat("voxelsz",&ifdIndirect[4],0,now);
		fout << "Attribute \"user\" \"float radius\" [" << vsize << "]" << endl;
	}

	fout << "#Extend by: " << radius << endl;

	for(int i=0;i<cnt;i++)
	{
		const GEO_PrimVolume* v = dynamic_cast<const GEO_PrimVolume*>(pl[i]);
		if(v==NULL) continue;

		fout << "Attribute \"user\" \"float id\" [ " << float(i)/float(cnt) << " ]" << endl;

		UT_BoundingBox b;
		v->getBBox(&b);
		fout << "Attribute \"user\""
			<< " \"float bbox00\" " << b.minvec().x()
			<< " \"float bbox01\" " << b.minvec().y()
			<< " \"float bbox02\" " << b.minvec().z()
			<< " \"float bbox10\" " << b.maxvec().x()
			<< " \"float bbox11\" " << b.maxvec().y()
			<< " \"float bbox12\" " << b.maxvec().z()
			<< endl;

		UT_String path;
		path.sprintf("%s.%d.ptc",savepath.buffer(),i);

		fout << "Procedural \"DynamicLoad\" [\"DPROC_scvol\" \"" << path.buffer() << "\"] [";

		UT_Vector3 mIN = b.minvec();
		UT_Vector3 mAX = b.maxvec();

		fout << mIN.x()-radius << " " << mAX.x()+radius << " ";
		fout << mIN.y()-radius << " " << mAX.y()+radius << " ";
		fout << mIN.z()-radius << " " << mAX.z()+radius << "]" << endl;

		/*fout << "Blobby 1 [8 1004 0 0 0 1 1] [] [\"IMPL_ScallopVolume\" \"" << path.buffer() << "\"]" << endl;
		//Blobby 1 [8 1004 0 0 0 1 1] [] ["IMPL_ScallopVolume" "H://out/out.0.ptc"] */
	}

	unlockInputs();
};