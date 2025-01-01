/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2017 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file sg_helpers.h
 *
 * Define a number of macros to aid in repetitious code which is probably best expressed
 * as a preprocessor macro rather than as a template. This header also declares a number
 * of functions which are only of use within the sg_* classes.
 */

#ifndef SG_HELPERS_H
#define SG_HELPERS_H

#include <iostream>
#include <string>
#include <algorithm>
#include <vector>
#include "plugins/3dapi/sg_base.h"
#include "plugins/3dapi/sg_types.h"
#include <glm/glm.hpp>

class SGNORMALS;
class SGCOORDS;
class SGCOORDINDEX;


// Function to drop references within an SGNODE
// The node being destroyed must remove itself from the object reference's
// backpointer list in order to avoid a segfault.
#define DROP_REFS( aType, aList )                         \
    do                                                    \
    {                                                     \
        std::vector<aType*>::iterator sL = aList.begin(); \
        std::vector<aType*>::iterator eL = aList.end();   \
        while( sL != eL )                                 \
        {                                                 \
            ( (SGNODE*) *sL )->delNodeRef( this );        \
            ++sL;                                         \
        }                                                 \
        aList.clear();                                    \
    } while( 0 )


// Function to delete owned objects within an SGNODE
// The owned object's parent is set to NULL before
// deletion to avoid a redundant 'unlinkChildNode' call.
#define DEL_OBJS( aType, aList )                             \
    do                                                       \
    {                                                        \
        std::vector<aType*>::iterator sL = aList.begin();    \
        std::vector<aType*>::iterator eL = aList.end();      \
        while( sL != eL )                                    \
        {                                                    \
            ( (SGNODE*) *sL )->SetParent( nullptr, false );  \
            delete *sL;                                      \
            ++sL;                                            \
        }                                                    \
        aList.clear();                                       \
    } while( 0 )


// Function to unlink a child or reference node when that child or
// reference node is being destroyed.
#define UNLINK_NODE( aNodeID, aType, aNode, aOwnedList, aRefList, isChild ) \
    do                                                                      \
    {                                                                       \
        if( aNodeID == aNode->GetNodeType() )                               \
        {                                                                   \
            std::vector<aType*>*          oSL;                              \
            std::vector<aType*>::iterator sL;                               \
            std::vector<aType*>::iterator eL;                               \
            if( isChild )                                                   \
            {                                                               \
                oSL = &aOwnedList;                                          \
                sL  = oSL->begin();                                         \
                eL  = oSL->end();                                           \
                while( sL != eL )                                           \
                {                                                           \
                    if( (SGNODE*) *sL == aNode )                            \
                    {                                                       \
                        oSL->erase( sL );                                   \
                        return;                                             \
                    }                                                       \
                    ++sL;                                                   \
                }                                                           \
            }                                                               \
            else                                                            \
            {                                                               \
                oSL = &aRefList;                                            \
                sL  = oSL->begin();                                         \
                eL  = oSL->end();                                           \
                while( sL != eL )                                           \
                {                                                           \
                    if( (SGNODE*) *sL == aNode )                            \
                    {                                                       \
                        delNodeRef( this );                                 \
                        oSL->erase( sL );                                   \
                        return;                                             \
                    }                                                       \
                    ++sL;                                                   \
                }                                                           \
            }                                                               \
            return;                                                         \
        }                                                                   \
    } while( 0 )


// Function to check a node type, check for an existing reference,
// and add the node type to the reference list if applicable
#define ADD_NODE( aNodeID, aType, aNode, aOwnedList, aRefList, isChild )                           \
    do                                                                                             \
    {                                                                                              \
        if( aNodeID == aNode->GetNodeType() )                                                      \
        {                                                                                          \
            std::vector<aType*>::iterator sL;                                                      \
            sL = std::find( aOwnedList.begin(), aOwnedList.end(), aNode );                         \
            if( sL != aOwnedList.end() )                                                           \
                return true;                                                                       \
            sL = std::find( aRefList.begin(), aRefList.end(), aNode );                             \
            if( sL != aRefList.end() )                                                             \
                return true;                                                                       \
            if( isChild )                                                                          \
            {                                                                                      \
                SGNODE* ppn = (SGNODE*) aNode->GetParent();                                        \
                if( nullptr != ppn )                                                               \
                {                                                                                  \
                    if( this != ppn )                                                              \
                    {                                                                              \
                        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n"; \
                        std::cerr << " * [BUG] object '" << aNode->GetName();                      \
                        std::cerr << "' has multiple parents '" << ppn->GetName() << "', '";       \
                        std::cerr << m_Name << "'\n";                                              \
                        return false;                                                              \
                    }                                                                              \
                }                                                                                  \
                aOwnedList.push_back( (aType*) aNode );                                            \
                aNode->SetParent( this, false );                                                   \
            }                                                                                      \
            else                                                                                   \
            {                                                                                      \
                /*if( nullptr == aNode->GetParent() ) { \
                std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n"; \
                std::cerr << " * [BUG] object '" << aNode->GetName(); \
                std::cerr << "' has no parent\n"; \
                std::cerr << " * [INFO] possible copy assignment or copy constructor bug\n"; \
                return false; \
            } */                                             \
                aRefList.push_back( (aType*) aNode );                                              \
                aNode->addNodeRef( this );                                                         \
            }                                                                                      \
            return true;                                                                           \
        }                                                                                          \
    } while( 0 )


// Function to find a node object given a (non-unique) node name
#define FIND_NODE( aType, aName, aNodeList, aCallingNode )         \
    do                                                             \
    {                                                              \
        std::vector<aType*>::iterator sLA = aNodeList.begin();     \
        std::vector<aType*>::iterator eLA = aNodeList.end();       \
        SGNODE*                       psg = nullptr;               \
        while( sLA != eLA )                                        \
        {                                                          \
            if( (SGNODE*) *sLA != aCallingNode )                   \
            {                                                      \
                psg = (SGNODE*) ( *sLA )->FindNode( aName, this ); \
                if( nullptr != psg )                               \
                    return psg;                                    \
            }                                                      \
            ++sLA;                                                 \
        }                                                          \
    } while( 0 )


namespace S3D
{
    bool degenerate( glm::dvec3* pts ) noexcept;

    //
    // Normals calculations from triangles
    //

    /*
     * Take an array of 3D coordinates and its corresponding index set and calculates
     * the normals assuming that indices are given in CCW order.
     *
     * Care must be taken in using this function to ensure that:
     *  -# All coordinates are indexed; unindexed coordinates are assigned normal(0,0,1);
     *     when dealing with VRML models which may list and reuse one large coordinate set it
     *     is necessary to gather all index sets and perform this operation only once.
     *  -# Index sets must represent triangles (multiple of 3 indices) and must not be
     *     degenerate, that is all indices and coordinates in a triad must be unique.
     *
     * @param coords is the array of 3D vertices.
     * @param index is the array of 3x vertex indices (triads).
     * @param norms is an empty array which holds the normals corresponding to each vector.
     * @return true on success; otherwise false.
     */
    bool CalcTriangleNormals( std::vector< SGPOINT > coords, std::vector< int >& index,
                              std::vector< SGVECTOR >& norms );

    // formats a floating point number for text output to a VRML file
    void FormatFloat( std::string& result, double value );

    // format orientation data for VRML output
    void FormatOrientation( std::string& result, const SGVECTOR& axis, double rotation );

    // format point data for VRML output
    void FormatPoint( std::string& result, const SGPOINT& point );

    // format vector data for VRML output
    void FormatVector( std::string& result, const SGVECTOR& aVector );

    // format Color data for VRML output
    void FormatColor( std::string& result, const SGCOLOR& aColor );

    //
    // Cache related WRITE functions
    //

    // write out an XYZ vertex
    bool WritePoint( std::ostream& aFile, const SGPOINT& aPoint );

    // write out a unit vector
    bool WriteVector( std::ostream& aFile, const SGVECTOR& aVector );

    // write out an RGB color
    bool WriteColor( std::ostream& aFile, const SGCOLOR& aColor );

    /**
     * Read the text tag of a binary cache file which is the NodeTag and unique ID number combined.
     *
     * @param aFile is a binary file open for reading.
     * @param aName will hold the tag name on successful return.
     * @return will be the NodeType which the tag represents or S3D::SGTYPES::SGTYPE_END on
     *         failure.
     */
    S3D::SGTYPES ReadTag( std::istream& aFile, std::string& aName );

    // read an XYZ vertex
    bool ReadPoint( std::istream& aFile, SGPOINT& aPoint );

    // read a unit vector
    bool ReadVector( std::istream& aFile, SGVECTOR& aVector );

    // read an RGB color
    bool ReadColor( std::istream& aFile, SGCOLOR& aColor );
}

#endif  // SG_HELPERS_H
