/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2015 Mario Luzeiro <mrluzeiro@gmail.com>
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
#include <3d_types.h>
#include <CBBox.h>


class S3D_MASTER;
class STRUCT_3D_SHAPE;
class S3D_MODEL_PARSER;

// Master structure for a 3D footprint shape description
class S3D_MASTER : public EDA_ITEM
{
public:
    S3DPOINT            m_MatScale;     ///< a scaling factor for the entire 3D footprint shape
    S3DPOINT            m_MatRotation;  ///< a grotation for the entire 3D footprint shape
    S3DPOINT            m_MatPosition;  ///< an offset for the entire 3D footprint shape
    STRUCT_3D_SHAPE*    m_3D_Drawings;  ///< the list of basic shapes
    S3D_MATERIAL*       m_Materials;    ///< the list of materiels used by the shapes
    S3D_MODEL_PARSER*   m_parser;       ///< it store the loaded file to be rendered later

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
    wxString            m_Shape3DName;          ///< The 3D shape filename in 3D library
    FILE3D_TYPE         m_ShapeType;            ///< Shape type based on filename extension
    wxString            m_Shape3DFullFilename;  ///< Full file path name
    wxString            m_Shape3DNameExtension; ///< Extension of the shape file name

public:
    S3D_MASTER( EDA_ITEM* aParent );
    ~S3D_MASTER();

    S3D_MASTER* Next() const { return (S3D_MASTER*) Pnext; }
    S3D_MASTER* Back() const { return (S3D_MASTER*) Pback; }

    // Accessors
    void Insert( S3D_MATERIAL* aMaterial );

    void Copy( S3D_MASTER* pattern );

    /**
     * Function ReadData
     * Select the parser to read the 3D data file (vrml, x3d ...)
     * and build the description objects list
     * @param aParser the parser that should be used to read model data and stored in
     */
    int  ReadData( S3D_MODEL_PARSER* aParser );

    void Render( bool aIsRenderingJustNonTransparentObjects,
                 bool aIsRenderingJustTransparentObjects );

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
     * Function GetShape3DExtension
     * @return the extension of the filename of the 3D shape,
     */
    const wxString GetShape3DExtension();

    /**
     * Function SetShape3DName
     * @param aShapeName = file name of the data file relative to the 3D shape
     *
     * Set the filename of the 3D shape, and depending on the file extention
     * (vrl, x3d, idf ) the type of file.
     */
    void SetShape3DName( const wxString& aShapeName );

    /**
     * Function getBBox Model Space Bouding Box
     * @return return the model space bouding box
     */
    CBBOX &getBBox();

    /**
     * Function getFastAABBox
     * @return return the Axis Align Bounding Box of the other bouding boxes
     */
    CBBOX &getFastAABBox();

private:
    void    calcBBox();
    CBBOX   m_BBox;             ///< Model oriented Bouding Box
    CBBOX   m_fastAABBox;       ///< Axis Align Bounding Box that contain the other bounding boxes
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

#endif // STRUCT_3D_H
