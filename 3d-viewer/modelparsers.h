/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Tuomas Vaherkoski <tuomasvaherkoski@gmail.com>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file modelparsers.h
 */

#ifndef MODELPARSERS_H
#define MODELPARSERS_H

#include <map>
#include <memory>
#include <vector>
#include <wx/string.h>
#include <3d_mesh_model.h>

class S3D_MASTER;
class X3D_MODEL_PARSER;

/**
 * abstract class S3D_MODEL_PARSER
 * Base class for 3D model parsers.
 */
class S3D_MODEL_PARSER
{
public:
    S3D_MODEL_PARSER( S3D_MASTER* aMaster ) :
        master( aMaster )
    {}

    virtual ~S3D_MODEL_PARSER(){}

    S3D_MASTER* GetMaster()
    {
        return master;
    }

    /**
     * Function Create
     * Factory method for creating concrete 3D model parsers
     * Notice that the caller is responsible to delete created parser.
     *
     * @param aMaster is master object that the parser will fill.
     * @param aExtension is file extension of the file you are going to parse.
     */
    static S3D_MODEL_PARSER* Create( S3D_MASTER* aMaster, const wxString aExtension );

    /**
     * virtual Function
     * Concrete parsers should implement this function
     * @param aFilename = the full file name of the file to load
     * @return true if as succeeded
     */
    virtual bool Load( const wxString& aFilename ) {
        return false;
    };

    S3D_MESH_PTRS childs;

private:
    S3D_MASTER* master;
};


class wxXmlNode;

/**
 * class X3D_MODEL_PARSER
 * Implements parser for X3D file format (VRML2.0 successor)
 * X3D files can be exported from eg. Blender */
class X3D_MODEL_PARSER: public S3D_MODEL_PARSER
{
public:
    X3D_MODEL_PARSER( S3D_MASTER* aMaster );
    ~X3D_MODEL_PARSER();

    bool Load( const wxString& aFilename );

    typedef std::map< wxString, wxString > PROPERTY_MAP;
    typedef std::vector< wxXmlNode* >      NODE_LIST;

    /**
     * Function GetChildsByName
     * Searches all child nodes with aName.
     *
     * @param aParent is node to search from
     * @param aName is the name of node you try to find
     * @param aResult contains found nodes
     */
    static void GetChildsByName( wxXmlNode* aParent, const wxString aName, NODE_LIST& aResult );

    /**
     * Function GetNodeProperties
     * Collects all node properties to map.
     *
     * @param aNode is an XML node.
     * @param aProps contains map of found properties.
     */
    static void GetNodeProperties( wxXmlNode* aNode, PROPERTY_MAP& aProps );

private:
    wxString                 m_Filename;
    S3D_MESH_PTR             m_model;

    std::vector< wxString > vrml_materials;
    std::vector< wxString > vrml_points;
    std::vector< wxString > vrml_coord_indexes;

    void readTransform( wxXmlNode* aTransformNode );
    void readMaterial( wxXmlNode* aMatNode );
    void readIndexedFaceSet( wxXmlNode* aFaceNode, PROPERTY_MAP& aTransfromProps );
    bool parseDoubleTriplet( const wxString& aData, S3D_VERTEX& aResult );

    void rotate( S3D_VERTEX& aCoordinate, S3D_VERTEX& aRotAxis, double angle );
};


typedef std::map< std::string, std::vector< glm::vec3 > > VRML2_COORDINATE_MAP;
typedef std::map< std::string, S3D_MESH_PTR > VRML2_DEF_GROUP_MAP;

/**
 * class VRML2_MODEL_PARSER
 * Parses
 */
class VRML2_MODEL_PARSER
{
public:
    VRML2_MODEL_PARSER( S3D_MODEL_PARSER* aModelParser );
    ~VRML2_MODEL_PARSER();

    bool Load( const wxString& aFilename );

    /**
     * Function Load
     * Load a VRML2 filename and apply a transformation to the root
     * @param aFilename file name with path
     * @param aTransformationModel a model with translation, rotation and scale to apply to default root
     * @return bool - true if finnished with success
     */
    bool Load( const wxString& aFilename, S3D_MESH_PTR aTransformationModel );

    /**
     * Return string representing VRML2 file in vrml2 format
     * Function Load must be called before this function, otherwise empty
     * data set is returned.
     */
    wxString VRML2_representation();

private:
    int loadFileModel( S3D_MESH_PTR transformationModel );
    int read_Transform();
    int read_DEF();
    int read_DEF_Coordinate();
    int read_Shape();
    int read_appearance();
    int read_Appearance();
    int read_material();
    int read_Material();
    int read_IndexedFaceSet();
    int read_IndexedLineSet();
    int read_Coordinate();
    int read_CoordinateDef();
    int read_Normal();
    int read_NormalIndex();
    int read_Color();
    int read_coordIndex();
    int read_colorIndex();
    int read_geometry();
    int read_IndexedFaceSet_USE();
    int read_Transform_USE();
    int read_Inline();

    /** Function debug_enter
     * Used in debug to increase a ' ' in the m_debugSpacer,
     * should be called after the first debug comment in a function
     */
    void debug_enter();

    /** Function debug_exit
     * Used in debug to decrease a ' ' in the m_debugSpacer,
     * should be called before the last debug comment in a funtion before exit
     */
    void debug_exit();

    bool                      m_normalPerVertex;
    bool                      colorPerVertex;
    S3D_MESH_PTR              m_model;                  ///< It stores the current model that the parsing is adding data
    FILE*                     m_file;
    wxFileName                m_Filename;
    VRML2_COORDINATE_MAP      m_defCoordinateMap;
    VRML2_DEF_GROUP_MAP       m_defGroupMap;            ///< Stores a list of labels for groups and meshs that will be used later by the USE keyword
    S3D_MODEL_PARSER*         m_ModelParser;
    S3D_MASTER*               m_Master;
    wxString                  m_debugSpacer;            ///< Used to give identation space

    int                       m_counter_DEF_GROUP;      ///< Counts the number of DEF * GROUPS used
    int                       m_counter_USE_GROUP;      ///< Counts the number of USE * used, in the end, if m_counter_DEF_GROUP > 0 and m_counter_USE_GROUP == 0 then it will add the first group with childs

    bool                      m_discardLastGeometry;    ///< If true, it should not store the latest loaded geometry (used to discard IndexedLineSet, but load it)
};


/**
 * class VRML1_MODEL_PARSER
 * Parses
 */
class VRML1_MODEL_PARSER
{
public:
    VRML1_MODEL_PARSER( S3D_MODEL_PARSER* aModelParser );
    ~VRML1_MODEL_PARSER();

    bool Load( const wxString& aFilename );

    /**
     * Return string representing VRML2 file in vrml2 format
     * Function Load must be called before this function, otherwise empty
     * data set is returned.
     */
    wxString VRML2_representation();

private:
    int read_separator();
    int readMaterial();
    int readCoordinate3();
    int readIndexedFaceSet();

    int readMaterial_ambientColor();
    int readMaterial_diffuseColor();
    int readMaterial_emissiveColor();
    int readMaterial_specularColor();
    int readMaterial_shininess();
    int readMaterial_transparency();

    int readCoordinate3_point();

    int readIndexedFaceSet_coordIndex();
    int readIndexedFaceSet_materialIndex();

    bool                     m_normalPerVertex;
    bool                     colorPerVertex;
    S3D_MESH_PTR             m_model;
    FILE*                    m_file;
    wxString                 m_Filename;
    S3D_MODEL_PARSER*        m_ModelParser;
    S3D_MASTER*              m_Master;
};

/**
 * class VRML_MODEL_PARSER
 * Parses
 */
class VRML_MODEL_PARSER: public S3D_MODEL_PARSER
{
public:
    /**
     * ctor: initialize a VRML file parser
     * @param aMaster = a ref to a 3D footprint shape description to fill
     * by the vrml file data
     */
    VRML_MODEL_PARSER( S3D_MASTER* aMaster );
    ~VRML_MODEL_PARSER();

    /**
     * Function load
     * Load a 3D file and build a S3D_MASTER shape.
     * file has .vrml ext and can be VRML 1 or VRML 2 format
     * @param aFilename = the full filename to read
     * @param aVrmlunits_to_3Dunits = the csaling factor to convert the 3D file unit
     * to our internal units.
     */
    bool Load( const wxString& aFilename );
};


#endif // MODELPARSERS_H
