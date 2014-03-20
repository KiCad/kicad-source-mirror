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


class S3D_MASTER;
class S3D_VERTEX;

extern void TransfertToGLlist( std::vector< S3D_VERTEX >& aVertices, double aBiuTo3DUnits );

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
     * Return string representing x3d file in vrml format
     * Function Load must be called before this function, otherwise empty
     * data set is returned.
     */
    wxString VRML_representation();

private:
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
 * class WRL_MODEL_PARSER
 * Parses
 */
class VRML_MODEL_PARSER: public S3D_MODEL_PARSER
{
public:
    VRML_MODEL_PARSER( S3D_MASTER* aMaster );
    ~VRML_MODEL_PARSER();
    void Load( const wxString aFilename );

private:
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
    int readMaterial( FILE* file, int* LineNum );
    int readChildren( FILE* file, int* LineNum );
    int readShape( FILE* file, int* LineNum );
    int readAppearance( FILE* file, int* LineNum );
    int readGeometry( FILE* file, int* LineNum );

    /**
     * Function ReadCoordList
     * reads 3D coordinate lists like:
     *      coord Coordinate { point [
     *        -5.24489 6.57640e-3 -9.42129e-2,
     *        -5.11821 6.57421e-3 0.542654,
     *        -3.45868 0.256565 1.32000 ] }
     *  or:
     *  normal Normal { vector [
     *        0.995171 -6.08102e-6 9.81541e-2,
     *        0.923880 -4.09802e-6 0.382683,
     *        0.707107 -9.38186e-7 0.707107]
     *      }
     *
     *  text_buffer contains the first line of this node :
     *     "coord Coordinate { point ["
     */
    void readCoordsList( FILE* file, char* text_buffer, std::vector< double >& aList,
                         int* LineNum );
};

#endif // MODELPARSERS_H
