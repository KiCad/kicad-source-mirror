/////////////////////////////////////////////////////////////////////////////
// Name:        3d_class.cpp
/////////////////////////////////////////////////////////////////////////////


#ifdef __GNUG__
#pragma implementation
#pragma interface
#endif

#include "fctsys.h"

#include "common.h"

#include "3d_struct.h"
#include "3d_viewer.h"


/****************************/
S3D_Vertex::S3D_Vertex(void)
/****************************/
{
	x = y = z = 0.0;
}


/**************************************************************************/
S3D_Material::S3D_Material(Struct3D_Master * father, const wxString & name):
		EDA_BaseStruct( father, -1)
/**************************************************************************/
{
	m_DiffuseColor.x = m_DiffuseColor.y = m_DiffuseColor.z = 1.0;
    m_SpecularColor.x = m_SpecularColor.y = m_SpecularColor.z = 1.0;
    m_AmbientIntensity = 1.0;
    m_Transparency = 0.0;
    m_Shininess = 1.0;
	m_Name = name;
}

/***********************************/
void S3D_Material::SetMaterial(void)
/***********************************/
{
    glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
	glColor4f(m_DiffuseColor.x * m_AmbientIntensity,
		m_DiffuseColor.y * m_AmbientIntensity,
		m_DiffuseColor.z * m_AmbientIntensity,
		1.0 - m_Transparency );
#if 0
	glColorMaterial(GL_FRONT_AND_BACK,GL_SPECULAR);
	glColor3f(m_SpecularColor.x, m_SpecularColor.y,m_SpecularColor.z);
#endif
    glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
}

/****************************************************/
void Struct3D_Master::Copy(Struct3D_Master * pattern)
/****************************************************/
{
	m_Shape3DName = pattern->m_Shape3DName;
	m_MatScale = pattern->m_MatScale;
	m_MatRotation = pattern->m_MatRotation;
	m_MatPosition = pattern->m_MatPosition;
	m_3D_Drawings = NULL;
	m_Materials = NULL;
}

/***************************************************************/
Struct3D_Master::Struct3D_Master(EDA_BaseStruct * StructFather):
		EDA_BaseStruct( StructFather, -1)
/***************************************************************/
{
	m_MatScale.x = m_MatScale.y = m_MatScale.z = 1.0;
	m_3D_Drawings = NULL;
	m_Materials = NULL;
}


/***************************************/
Struct3D_Master:: ~Struct3D_Master(void)
/***************************************/
{
Struct3D_Shape * next;
S3D_Material * nextmat;

	for( ; m_3D_Drawings != NULL; m_3D_Drawings = next )
	{
		next = (Struct3D_Shape *) m_3D_Drawings->Pnext;
		delete m_3D_Drawings;
	}

	for( ; m_Materials != NULL;  m_Materials = nextmat )
	{
		nextmat = (S3D_Material *)  m_Materials->Pnext;
		delete  m_Materials;
	}
}



/***************************************************************/
Struct3D_Shape::Struct3D_Shape(EDA_BaseStruct * StructFather):
		EDA_BaseStruct( StructFather, -1)
/***************************************************************/
{
	m_3D_Coord = NULL;
	m_3D_CoordIndex = NULL;
	m_3D_Points = 0;
}


/***************************************/
Struct3D_Shape:: ~Struct3D_Shape(void)
/***************************************/
{
	delete m_3D_Coord;
	delete m_3D_CoordIndex;
}

