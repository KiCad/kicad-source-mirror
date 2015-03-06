/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Mario Luzeiro <mrluzeiro@gmail.com>
 * Copyright (C) 2004 Jean-Pierre Charras, jp.charras at wanadoo.fr
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
#include <3d_material.h>
#include <gal/opengl/glm/glm.hpp>

/**
 * @note For historical reasons the 3D modeling unit is 0.1 inch
 * 1 3Dunit = 2.54 mm = 0.1 inch = 100 mils
 */
#define UNITS3D_TO_UNITSPCB (IU_PER_MILS * 100)

/**
 * scaling factor for 3D shape offset ( S3D_MASTER::m_MatPosition member )
 * Was in inches in legacy version, and, due to a mistake, still in inches
 * in .kicad_pcb files (which are using mm)
 * so this scaling convert file units (inch) to 3D units (0.1 inch), only
 * for S3D_MASTER::m_MatPosition parameter
 */
#define SCALE_3D_CONV 10

class S3D_MASTER;
class STRUCT_3D_SHAPE;

// S3D_VERTEX manages a opengl 3D coordinate (3 float numbers: x,y,z coordinates)
// float are widely used in opengl functions.
// they are used here in coordinates which are also used in opengl functions.
#define S3D_VERTEX glm::vec3

// S3DPOINT manages a set of 3 double values (x,y,z )
// It is used for values which are not directly used in opengl functions.
// It is used in dialogs, or when reading/writing files for instance
class S3DPOINT
{
public:
    double x, y, z;

public:
    S3DPOINT()
    {
        x = y = z = 0.0;
    }

    S3DPOINT( double px, double py, double pz)
    {
        x = px;
        y = py;
        z = pz;
    }
};

// Master structure for a 3D footprint shape description
class S3D_MASTER : public EDA_ITEM
{
public:
    S3DPOINT      m_MatScale;       ///< a scaling factor for the entire 3D footprint shape
    S3DPOINT      m_MatRotation;    ///< a grotation for the entire 3D footprint shape
    S3DPOINT      m_MatPosition;    ///< an offset for the entire 3D footprint shape
    STRUCT_3D_SHAPE* m_3D_Drawings; ///< the list of basic shapes
    S3D_MATERIAL*   m_Materials;    ///< the list of materiels used by the shapes

    enum FILE3D_TYPE
    {
        FILE3D_NONE = 0,
        FILE3D_VRML,
        FILE3D_IDF,
        FILE3D_UNKNOWN
    };

    // Check defaults in S3D_MASTER
    bool        m_use_modelfile_diffuseColor;
    bool        m_use_modelfile_emissiveColor;
    bool        m_use_modelfile_specularColor;
    bool        m_use_modelfile_ambientIntensity;
    bool        m_use_modelfile_transparency;
    bool        m_use_modelfile_shininess;

private:
    wxString    m_Shape3DName;      // the 3D shape filename in 3D library
    FILE3D_TYPE m_ShapeType;
    double      m_lastTransparency;         // last transparency value from
                                            // last material in use
    bool        m_loadTransparentObjects;
    bool        m_loadNonTransparentObjects;



public:
    S3D_MASTER( EDA_ITEM* aParent );
    ~S3D_MASTER();

    S3D_MASTER* Next() const { return (S3D_MASTER*) Pnext; }
    S3D_MASTER* Back() const { return (S3D_MASTER*) Pback; }

    // Accessors
    void SetLastTransparency( double aValue ) { m_lastTransparency = aValue; }

    void SetLoadTransparentObjects( bool aLoad )
        { m_loadTransparentObjects = aLoad; }

    void SetLoadNonTransparentObjects( bool aLoad )
        { m_loadNonTransparentObjects = aLoad; }

    void Insert( S3D_MATERIAL* aMaterial );

    /**
     * Function IsOpenGlAllowed
     * @return true if opengl current list accepts a gl data
     * used to filter transparent objects, which are drawn after
     * non transparent objects
     */
    bool IsOpenGlAllowed();

    void Copy( S3D_MASTER* pattern );

    /**
     * Function ReadData
     * Select the parser to read the 3D data file (vrml, x3d ...)
     * and build the description objects list
     */
    int  ReadData();

    /**
     * Function ObjectCoordsTo3DUnits
     * @param aVertices = a list of 3D coordinates in shape units
     * to convert to 3D canvas units, according to the
     * footprint 3Dshape rotation, offset and scale parameters
     */
    void ObjectCoordsTo3DUnits( std::vector< S3D_VERTEX >& aVertices );

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const { ShowDummy( os ); } // override
#endif

    /**
     * Function Is3DType
     * returns true if the argument matches the type of model referred to
     * by m_Shape3DName
     */
    bool Is3DType( enum FILE3D_TYPE aShapeType );

    const wxString& GetShape3DName( void )
    {
        return m_Shape3DName;
    }

    /** Get class name
     * @return  string "S3D_MASTER"
     */
    virtual wxString GetClass() const
    {
        return wxT( "S3D_MASTER" );
    }

    /**
     * Function GetShape3DFullFilename
     * @return the full filename of the 3D shape,
     * expanding environment variable (if any ) and/or adding default 3D path
     * given by environment variable KISYS3DMOD
     */
    const wxString GetShape3DFullFilename();

    /**
     * Function SetShape3DName
     * @param aShapeName = file name of the data file relative to the 3D shape
     *
     * Set the filename of the 3D shape, and depending on the file extention
     * (vrl, x3d, idf ) the type of file.
     */
    void SetShape3DName( const wxString& aShapeName );
};


/* Describes a complex 3D */
class STRUCT_3D_SHAPE : public EDA_ITEM
{
public:
    S3D_VERTEX* m_3D_Coord;
    int*        m_3D_CoordIndex;
    int         m_3D_Points;

public:
    STRUCT_3D_SHAPE( EDA_ITEM* aParent );
    ~STRUCT_3D_SHAPE();

    STRUCT_3D_SHAPE* Next() const { return (STRUCT_3D_SHAPE*) Pnext; }
    STRUCT_3D_SHAPE* Back() const { return (STRUCT_3D_SHAPE*) Pback; }

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const { ShowDummy( os ); } // override
#endif
};


/**
 * Class S3DPOINT_VALUE_CTRL
 * displays a S3DPOINT for editing (in dialogs).  A S3DPOINT is a triplet of values
 * Values can be scale, rotation, offset...
 */
class S3DPOINT_VALUE_CTRL
{
private:
    wxTextCtrl*   m_XValueCtrl, * m_YValueCtrl, * m_ZValueCtrl;

public:
    S3DPOINT_VALUE_CTRL( wxWindow* parent, wxBoxSizer* BoxSizer );

    ~S3DPOINT_VALUE_CTRL();

    /**
     * Function GetValue
     * @return the 3D point in internal units.
     */
    S3DPOINT   GetValue();
    void       SetValue( S3DPOINT a3Dpoint );
    void       Enable( bool enbl );
    void       SetToolTip( const wxString& text );
};

#endif // STRUCT_3D_H
