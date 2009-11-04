/////////////////////////////////////////////////////////////////////////////
// Name:        3d_class.cpp
/////////////////////////////////////////////////////////////////////////////


#ifdef __GNUG__
#pragma implementation
#pragma interface
#endif

#include "fctsys.h"

#include "3d_viewer.h"


S3D_Vertex::S3D_Vertex()
{
    x = y = z = 0.0;
}


S3D_MATERIAL::S3D_MATERIAL( S3D_MASTER* father, const wxString& name ) :
    EDA_BaseStruct( father, NOT_USED )
{
    m_DiffuseColor.x   = m_DiffuseColor.y = m_DiffuseColor.z = 1.0;
    m_SpecularColor.x  = m_SpecularColor.y = m_SpecularColor.z = 1.0;
    m_AmbientIntensity = 1.0;
    m_Transparency     = 0.0;
    m_Shininess = 1.0;
    m_Name = name;
}


void S3D_MATERIAL::SetMaterial()
{
    glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );
    glColor4f( m_DiffuseColor.x * m_AmbientIntensity,
               m_DiffuseColor.y * m_AmbientIntensity,
               m_DiffuseColor.z * m_AmbientIntensity,
               1.0 - m_Transparency );
#if 0
    glColorMaterial( GL_FRONT_AND_BACK, GL_SPECULAR );
    glColor3f( m_SpecularColor.x, m_SpecularColor.y, m_SpecularColor.z );
#endif
    glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );
}


void S3D_MASTER::Copy( S3D_MASTER* pattern )
{
    m_Shape3DName = pattern->m_Shape3DName;
    m_MatScale    = pattern->m_MatScale;
    m_MatRotation = pattern->m_MatRotation;
    m_MatPosition = pattern->m_MatPosition;
    m_3D_Drawings = NULL;
    m_Materials   = NULL;
}


S3D_MASTER::S3D_MASTER( EDA_BaseStruct* aParent ) :
    EDA_BaseStruct( aParent, NOT_USED )
{
    m_MatScale.x  = m_MatScale.y = m_MatScale.z = 1.0;
    m_3D_Drawings = NULL;
    m_Materials   = NULL;
}


S3D_MASTER:: ~S3D_MASTER()
{
    Struct3D_Shape* next;
    S3D_MATERIAL*   nextmat;

    for( ; m_3D_Drawings != NULL; m_3D_Drawings = next )
    {
        next = m_3D_Drawings->Next();
        delete m_3D_Drawings;
    }

    for( ; m_Materials != NULL; m_Materials = nextmat )
    {
        nextmat = m_Materials->Next();
        delete m_Materials;
    }
}


Struct3D_Shape::Struct3D_Shape( EDA_BaseStruct* aParent ) :
    EDA_BaseStruct( aParent, NOT_USED )
{
    m_3D_Coord = NULL;
    m_3D_CoordIndex = NULL;
    m_3D_Points     = 0;
}


Struct3D_Shape:: ~Struct3D_Shape()
{
    delete m_3D_Coord;
    delete m_3D_CoordIndex;
}
