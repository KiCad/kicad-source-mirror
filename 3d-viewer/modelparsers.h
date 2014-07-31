/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Tuomas Vaherkoski <tuomasvaherkoski@gmail.com>
 * Copyright (C) 1992-2013 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <vector>
#include <wx/string.h>
#include <3d_mesh_model.h>

class S3D_MASTER;
class S3D_MODEL_PARSER;
class X3D_MODEL_PARSER;

/**
 * abstract class S3D_MODEL_PARSER
 * Base class for 3D model parsers.
 */
class S3D_MODEL_PARSER
{
public:
    S3D_MODEL_PARSER(S3D_MASTER* aMaster) :
        master( aMaster )
    {}

    virtual ~S3D_MODEL_PARSER()
    {}

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
     * Function Load
     *
     * Concrete parsers should implement this function
     */
    virtual void Load( const wxString aFilename ) = 0;

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
    void Load( const wxString aFilename );

    typedef std::map< wxString, wxString > PROPERTY_MAP;
    typedef std::vector< wxXmlNode* > NODE_LIST;

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

    /**
     * Return string representing x3d file in vrml2 format
     * Function Load must be called before this function, otherwise empty
     * data set is returned.
     */
    wxString VRML2_representation();

private:
    wxString m_Filename;
    S3D_MESH *m_model;
    std::vector<S3D_MESH *> childs;

    std::vector< wxString > vrml_materials;
    std::vector< wxString > vrml_points;
    std::vector< wxString > vrml_coord_indexes;

    void readTransform( wxXmlNode* aTransformNode );
    void readMaterial( wxXmlNode* aMatNode );
    void readIndexedFaceSet( wxXmlNode* aFaceNode, PROPERTY_MAP& aTransfromProps );
    bool parseDoubleTriplet( const wxString& aData, S3D_VERTEX& aResult );

    void rotate( S3D_VERTEX& aCoordinate, S3D_VERTEX& aRotAxis, double angle );
};



/**
 * class VRML2_MODEL_PARSER
 * Parses
 */
class VRML2_MODEL_PARSER: public S3D_MODEL_PARSER
{
public:
    VRML2_MODEL_PARSER( S3D_MASTER* aMaster );
    ~VRML2_MODEL_PARSER();
    void Load( const wxString aFilename );

    /**
     * Return string representing VRML2 file in vrml2 format
     * Function Load must be called before this function, otherwise empty
     * data set is returned.
     */
    wxString VRML2_representation();

private:
    int read_Transform();
    int read_DEF();
    int read_Shape();
    int read_Appearance();
    int read_material();
    int read_Material();
    int read_IndexedFaceSet();
    int read_Coordinate();
    int read_Normal();
    int read_NormalIndex();
    int read_Color();
    int read_coordIndex();
    int read_colorIndex();

    bool m_normalPerVertex;
    bool colorPerVertex;
    S3D_MESH *m_model;
    std::vector<S3D_MESH *> childs;
    FILE *m_file;
    S3D_MATERIAL *m_Materials;
    wxString m_Filename;
};


/**
 * class VRML1_MODEL_PARSER
 * Parses
 */
class VRML1_MODEL_PARSER: public S3D_MODEL_PARSER
{
public:
    VRML1_MODEL_PARSER( S3D_MASTER* aMaster );
    ~VRML1_MODEL_PARSER();
    void Load( const wxString aFilename );

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

    bool m_normalPerVertex;
    bool colorPerVertex;
    S3D_MESH *m_model;
    std::vector<S3D_MESH *> childs;
    S3D_MATERIAL *m_Materials;
    FILE *m_file;
    wxString m_Filename;
};

/**
 * class VRML_MODEL_PARSER
 * Parses
 */
class VRML_MODEL_PARSER: public S3D_MODEL_PARSER
{
public:
    VRML_MODEL_PARSER( S3D_MASTER* aMaster );
    ~VRML_MODEL_PARSER();
    void Load( const wxString aFilename );

private:
    VRML1_MODEL_PARSER *vrml1_parser;
    VRML2_MODEL_PARSER *vrml2_parser;
};


#endif // MODELPARSERS_H
