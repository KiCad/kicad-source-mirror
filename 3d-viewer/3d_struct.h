/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file 3d_struct.h
 */

#ifndef STRUCT_3D_H
#define STRUCT_3D_H

#include <common.h>
#include <base_struct.h>


/* 3D modeling units -> PCB units conversion scale:
 * 1 "3D model unit" wings3d = 1 unit = 2.54 mm = 0.1 inch = 100 mils
 */
#define UNITS3D_TO_UNITSPCB (IU_PER_MILS * 100)


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

/*  S3D_Vertex manage a 3D coordinate (3 float numbers: x,y,z coordinates)*/
class S3D_Vertex
{
public:
    double x, y, z;

public:
    S3D_Vertex()
    {
        x = y = z = 0.0;
    }

    S3D_Vertex( double px, double py, double pz)
    {
        x = px;
        y = py;
        z = pz;
    }
};

class S3D_MATERIAL : public EDA_ITEM       /* openGL "material" data*/
{
public:
    wxString   m_Name;
    S3D_Vertex m_DiffuseColor;
    S3D_Vertex m_EmissiveColor;
    S3D_Vertex m_SpecularColor;
    float      m_AmbientIntensity;
    float      m_Transparency;
    float      m_Shininess;

public:
    S3D_MATERIAL( S3D_MASTER* father, const wxString& name );

    S3D_MATERIAL* Next() const { return (S3D_MATERIAL*) Pnext; }
    S3D_MATERIAL* Back() const { return (S3D_MATERIAL*) Pback; }

    void SetMaterial();

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const { ShowDummy( os ); } // override
#endif
};


/* Master structure for a 3D item description */
class S3D_MASTER : public EDA_ITEM
{
public:
    wxString        m_Shape3DName; /* 3D shape name in 3D library */
    S3D_Vertex      m_MatScale;
    S3D_Vertex      m_MatRotation;
    S3D_Vertex      m_MatPosition;
    Struct3D_Shape* m_3D_Drawings;
    S3D_MATERIAL*   m_Materials;

public:
    S3D_MASTER( EDA_ITEM* aParent );
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

    /**
     * Function ReadMaterial
     * read the description of a 3D material definition in the form:
     * DEF yellow material Material (
     * DiffuseColor 1.00000 1.00000 0.00000e 0
     * EmissiveColor 0.00000e 0 0.00000e 0 0.00000e 0
     * SpecularColor 1.00000 1.00000 1.00000
     * AmbientIntensity 1.00000
     * Transparency 0.00000e 0
     * Shininess 1.00000
     *)
     * Or type:
     * USE yellow material
     */
    int  ReadMaterial( FILE* file, int* LineNum );
    int  ReadChildren( FILE* file, int* LineNum );
    int  ReadShape( FILE* file, int* LineNum );
    int  ReadAppearance( FILE* file, int* LineNum );
    int  ReadGeometry( FILE* file, int* LineNum );
    void Set_Object_Coords( std::vector< S3D_Vertex >& aVertices );

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const { ShowDummy( os ); } // override
#endif
};


/* Describes a complex 3D */
class Struct3D_Shape : public EDA_ITEM
{
public:
    S3D_Vertex* m_3D_Coord;
    int*        m_3D_CoordIndex;
    int         m_3D_Points;

public:
    Struct3D_Shape( EDA_ITEM* aParent );
    ~Struct3D_Shape();

    Struct3D_Shape* Next() const { return (Struct3D_Shape*) Pnext; }
    Struct3D_Shape* Back() const { return (Struct3D_Shape*) Pback; }

    int ReadData( FILE* file, int* LineNum );

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const { ShowDummy( os ); } // override
#endif
};


/**
 * Class VERTEX_VALUE_CTRL
 * displays a vertex for editing.  A vertex is a triplet of values
 * Values can be scale, rotation, offset...
 */
class VERTEX_VALUE_CTRL
{
private:
    wxTextCtrl*   m_XValueCtrl, * m_YValueCtrl, * m_ZValueCtrl;
    wxStaticText* m_Text;

public:
    VERTEX_VALUE_CTRL( wxWindow* parent, const wxString& title, wxBoxSizer* BoxSizer );

    ~VERTEX_VALUE_CTRL();

    /**
     * Function GetValue
     * @return the vertex in internal units.
     */
    S3D_Vertex GetValue();
    void       SetValue( S3D_Vertex vertex );
    void       Enable( bool enbl );
    void       SetToolTip( const wxString& text );
};

#endif // STRUCT_3D_H
