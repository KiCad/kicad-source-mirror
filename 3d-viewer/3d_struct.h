	/********************************************************/
	/*	3d_struct.h :  definition des structures de donnees */
	/*  pour la representation 3D des modules               */
	/********************************************************/

#ifndef STRUCT_3D_H
#define STRUCT_3D_H

#include "base_struct.h"

/* 3D modeler units -> PCB units conversion scale:
	1 "3D unit modeler" = 1 unit wings3d = 2,54 mm = 0.1 inch */
#define UNITS3D_TO_UNITSPCB 1000


class Struct3D_Master;
class Struct3D_Shape;

class S3D_Color		/* This is a 3D color (R, G, G) 3 floats range 0 to 1.0*/
{
public:
	double m_Red, m_Green, m_Blue;
public:
	S3D_Color()
	{
		m_Red = m_Green = m_Blue = 0;
	}
};

class S3D_Vertex	/* This is a 3D coordinate (3 float numbers: x,y,z coordinates)*/
{
public:
	double x, y, z;
public:
	S3D_Vertex();
};

class S3D_Material: public EDA_BaseStruct		/* openGL "material" data*/
{
public:
	wxString m_Name;
	S3D_Vertex m_DiffuseColor;
    S3D_Vertex m_EmissiveColor;
    S3D_Vertex m_SpecularColor;
    float m_AmbientIntensity;
    float m_Transparency;
    float m_Shininess;

public:
	S3D_Material(Struct3D_Master * father, const wxString & name);
	void SetMaterial();
};

/*******************************************/
class Struct3D_Master: public EDA_BaseStruct
/*******************************************/
/* Master structure for a 3D item description */
{
public:
	wxString m_Shape3DName;		/* 3D shape name in 3D library */
	S3D_Vertex m_MatScale;
	S3D_Vertex m_MatRotation;
	S3D_Vertex m_MatPosition;
	Struct3D_Shape * m_3D_Drawings;
	S3D_Material *m_Materials;

public:

	Struct3D_Master(EDA_BaseStruct * StructFather);
	~Struct3D_Master();

	void Copy(Struct3D_Master * pattern);
	int ReadData();
	int ReadMaterial(FILE * file, int *LineNum);
	int ReadChildren(FILE * file, int *LineNum);
	int ReadShape(FILE * file, int *LineNum);
	int ReadAppearance(FILE * file, int *LineNum);
	int ReadGeometry(FILE * file, int *LineNum);
	void Set_Object_Coords(S3D_Vertex * coord, int nbcoord );

};


/*********************************************/
class Struct3D_Shape: public EDA_BaseStruct
/*********************************************/
/* decrit une forme complexe 3D */
{
public:
	S3D_Vertex * m_3D_Coord;
	int * m_3D_CoordIndex;
	int m_3D_Points;

public:

	Struct3D_Shape(EDA_BaseStruct * StructFather);
	~Struct3D_Shape();

	int ReadData(FILE * file, int *LineNum);
};



/*****************************************************************/
/* Classe pour afficher et editer un Vertex (triplet de valeurs),*/
/* en INCHES ou MM ou sans unites								 */
/*****************************************************************/
/* internal_unit is the internal unit number by inch:
	- 1000 for EESchema
	- 10000 for PcbNew
*/
class WinEDA_VertexCtrl
{
private:
	int m_Units;
	int m_Internal_Unit;
	wxTextCtrl * m_XValueCtrl, * m_YValueCtrl, * m_ZValueCtrl;
	wxStaticText * m_Text;

public:
	// Constructor and destructor
	WinEDA_VertexCtrl(wxWindow *parent, const wxString & title,
						wxBoxSizer * BoxSizer,
						int units, int internal_unit);

	~WinEDA_VertexCtrl();

	S3D_Vertex GetValue();
	void SetValue(S3D_Vertex vertex);
	void Enable(bool enbl);
	void SetToolTip(const wxString & text);
};



#endif /* STRUCT_3D_H */
