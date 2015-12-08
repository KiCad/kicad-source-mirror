/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
 * @file sg_api.h
 * provides the API which plugins require to manipulate the SG* classes.
 *
 */

#ifndef SG_API_H
#define SG_API_H

#include <3d_cache/sg/sg_types.h>

class SGCOLOR;
class SGPOINT;
class SGVECTOR;
class SGNODE;
class SCENEGRAPH;

// creation and manipulation of base types
typedef SGCOLOR* (*NEW_SGBASE_COLOR)(void);
typedef bool ( *SET_SGBASE_COLOR )( SGCOLOR* aColor,
                                    float aRedVal, float aGreenVal, float aBlueVal );
typedef bool ( *GET_SGBASE_COLOR )( SGCOLOR* aColor,
                                    float& aRedVal, float& aGreenVal, float& aBlueVal );

typedef SGPOINT* ( *NEW_SGBASE_POINT )(void);
typedef bool ( *SET_SGBASE_POINT )( SGPOINT* aPoint,
                                    double aXVal, double aYVal, double aZVal );
typedef bool ( *GET_SGBASE_POINT )( SGPOINT* aPoint,
                                    double& aXVal, double& aYVal, double& aZVal );

typedef SGVECTOR* (*NEW_SGBASE_VECTOR)(void);
typedef bool ( *SET_SGBASE_VECTOR )( SGPOINT* aVector,
                                    double aXVal, double aYVal, double aZVal );
typedef bool ( *GET_SGBASE_VECTOR )( SGPOINT* aVector,
                                    double& aXVal, double& aYVal, double& aZVal );
// creation of scenegraph nodes:
typedef SGNODE* (*NEW_SG_NODE)( SGNODE* aParent, S3D::SGTYPES aNodeType);

namespace S3D
{
    struct SG_API
    {
        // creation of basic SG types
        NEW_SGBASE_COLOR  NewColor;
        NEW_SGBASE_POINT  NewPoint;
        NEW_SGBASE_VECTOR NewVector;
        // manipulators for basic SG types
        SET_SGBASE_COLOR SetSGColor;
        GET_SGBASE_COLOR GetSGColor;
        SET_SGBASE_POINT SetSGPoint;
        GET_SGBASE_POINT GetSGPoint;
        SET_SGBASE_VECTOR SetSGVector;
        GET_SGBASE_VECTOR GetSGVector;

        // creation of nodes
        NEW_SG_NODE NewNode;

    };

    // generic node class; this must never be instantiated as an underived class
    class API_SGNODE
    {
    protected:
        SGNODE* node;
        S3D::SGTYPES nodeType;

    public:
        API_SGNODE();

        SGNODE* GetNode( void );

        bool AttachNode( SGNODE* aNode );
        bool GetNodeType( S3D::SGTYPES& aNodeType ) const;

        bool GetParent( SGNODE const*& aParent ) const;
        bool SetParent( SGNODE* aParent );

        bool GetName( const char*& aName );
        bool SetName( const char *aName );
        bool GetNodeTypeName( S3D::SGTYPES aNodeType, const char*& aName ) const;

        bool FindNode( const char *aNodeName, const SGNODE *aCaller, SGNODE*& aNode );
    };

    // Transforms and operations
    class API_TRANSFORM : public API_SGNODE
    {
    public:
        API_TRANSFORM( SGNODE* aParent );

        bool GetLocalTransforms( SGNODE** aNodeList, int& nListItems );
        bool GetOtherTransforms( SGNODE** aNodeList, int& nListItems );
        bool AddOtherTransform( SGNODE* aTransformNode );

        bool GetLocalShapes( SGNODE** aNodeList, int& nListItems );
        bool GetOtherShapes( SGNODE** aNodeList, int& nListItems );
        bool AddOtherShape( SGNODE* aTransformNode );

        bool GetTranslation( SGPOINT& aPoint ) const;
        bool GetRotation( SGVECTOR& aVector, double& aAngle ) const;
        bool GetScale( SGPOINT& aScale ) const;

        bool SetTranslation( const SGPOINT& aPoint );
        bool SetRotation( const SGVECTOR& aVector, double aAngle );
        bool SetScale( const SGPOINT& aScale );
    };

    // Appearance and operations
    class API_APPEARANCE : public API_SGNODE
    {
    public:
        API_APPEARANCE( SGNODE *aParent );

        bool SetEmissive( float aRVal, float aGVal, float aBVal );
        bool SetEmissive( const SGCOLOR* aRGBColor );
        bool SetEmissive( const SGCOLOR& aRGBColor );

        bool SetDiffuse( float aRVal, float aGVal, float aBVal );
        bool SetDiffuse( const SGCOLOR* aRGBColor );
        bool SetDiffuse( const SGCOLOR& aRGBColor );

        bool SetSpecular( float aRVal, float aGVal, float aBVal );
        bool SetSpecular( const SGCOLOR* aRGBColor );
        bool SetSpecular( const SGCOLOR& aRGBColor );

        bool SetAmbient( double aVal );
        bool SetShininess( double aVal );
        bool SetTransparency( double aVal );

        bool GetEmissive( SGCOLOR& aRGBColor );
        bool GetDiffuse( SGCOLOR& aRGBColor );
        bool GetSpecular( SGCOLOR& aRGBColor );
        bool GetAmbient( double& aVal );
        bool GetShininess( double& aVal );
        bool GetTransparency( double& aVal );
    };

    // XXX - Color Index and operations
    // XXX - Colors and operations
    // XXX - Coordinate Index and operations
    // XXX - Coordinates and operations
    // XXX - Face Set and operations
    // XXX - Face Set Index (base class for other indices) and operations
    // XXX - Normals Index and operations
    // XXX - Normals and operations
    // XXX - Shape and operations
};

#endif  // SG_API_H
