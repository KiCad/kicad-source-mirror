/*****************/
/*	3d_struct.h  */
/*****************/

#ifndef STRUCT_3D_H
#define STRUCT_3D_H

#include "base_struct.h"


/* 3D modeling units -> PCB units conversion scale:
 * 1 "3D model unit" wings3d = 1 unit = 2.54 mm = 0.1 inch
 */
#define UNITS3D_TO_UNITSPCB 1000


class S3D_MASTER;
class Struct3D_Shape;

class S3D_Color     /* 3D color (R, G, G) 3 floats range 0 to 1.0*/
{
public:
    double m_Red, m_Green, m_Blue;
public: S3D_Color()
    {
        m_Red = m_Green = m_Blue = 0;
    }
};

class S3D_Vertex    /*  3D coordinate (3 float numbers: x,y,z coordinates)*/
{
public:
    double x, y, z;
public: S3D_Vertex();
};

class S3D_MATERIAL : public EDA_BaseStruct       /* openGL "material" data*/
{
public:
    wxString   m_Name;
    S3D_Vertex m_DiffuseColor;
    S3D_Vertex m_EmissiveColor;
    S3D_Vertex m_SpecularColor;
    float      m_AmbientIntensity;
    float      m_Transparency;
    float      m_Shininess;

public: S3D_MATERIAL( S3D_MASTER* father, const wxString& name );

    S3D_MATERIAL* Next() const { return (S3D_MATERIAL*) Pnext; }
    S3D_MATERIAL* Back() const { return (S3D_MATERIAL*) Pback; }

    void SetMaterial();
};


/* Master structure for a 3D item description */
class S3D_MASTER : public EDA_BaseStruct
{
public:
    wxString        m_Shape3DName; /* 3D shape name in 3D library */
    S3D_Vertex      m_MatScale;
    S3D_Vertex      m_MatRotation;
    S3D_Vertex      m_MatPosition;
    Struct3D_Shape* m_3D_Drawings;
    S3D_MATERIAL*   m_Materials;

public: S3D_MASTER( EDA_BaseStruct* aParent );
    ~S3D_MASTER();

    S3D_MASTER* Next() const { return (S3D_MASTER*) Pnext; }
    S3D_MASTER* Back() const { return (S3D_MASTER*) Pback; }

    void Insert( S3D_MATERIAL* aMaterial )
    {
        aMaterial->SetNext( m_Materials );
        m_Materials = aMaterial;
    }


    void Copy( S3D_MASTER* pattern );
    int  ReadData();
    int  ReadMaterial( FILE* file, int* LineNum );
    int  ReadChildren( FILE* file, int* LineNum );
    int  ReadShape( FILE* file, int* LineNum );
    int  ReadAppearance( FILE* file, int* LineNum );
    int  ReadGeometry( FILE* file, int* LineNum );
    void Set_Object_Coords( S3D_Vertex* coord, int nbcoord );
};


/* Describes a complex 3D */
class Struct3D_Shape : public EDA_BaseStruct
{
public:
    S3D_Vertex* m_3D_Coord;
    int*        m_3D_CoordIndex;
    int         m_3D_Points;

public: Struct3D_Shape( EDA_BaseStruct* aParent );
    ~Struct3D_Shape();

    Struct3D_Shape* Next() const { return (Struct3D_Shape*) Pnext; }
    Struct3D_Shape* Back() const { return (Struct3D_Shape*) Pback; }

    int ReadData( FILE* file, int* LineNum );
};


/* Display and edit a Vertex (triplet of values) in INCHES or MM or without
 * units.
 * internal_unit is the internal unit number by inch:
 * - 1000 for EESchema
 * - 10000 for PcbNew
 */
class WinEDA_VertexCtrl
{
private:
    int           m_Units;
    int           m_Internal_Unit;
    wxTextCtrl*   m_XValueCtrl, * m_YValueCtrl, * m_ZValueCtrl;
    wxStaticText* m_Text;

public:
    WinEDA_VertexCtrl( wxWindow* parent, const wxString& title,
                       wxBoxSizer* BoxSizer, int units, int internal_unit );

    ~WinEDA_VertexCtrl();

    S3D_Vertex GetValue();
    void       SetValue( S3D_Vertex vertex );
    void       Enable( bool enbl );
    void       SetToolTip( const wxString& text );
};


#endif /* STRUCT_3D_H */
