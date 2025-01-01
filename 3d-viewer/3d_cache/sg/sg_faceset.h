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
 * @file sg_faceset.h
 */


#ifndef SG_FACESET_H
#define SG_FACESET_H

#include <vector>
#include "3d_cache/sg/sg_node.h"


class SGCOLORS;
class SGCOORDS;
class SGNORMALS;
class SGCOLORINDEX;
class SGCOORDINDEX;

/**
 * Define an indexed face set for a scenegraph.
 */
class SGFACESET : public SGNODE
{
public:
    SGFACESET( SGNODE* aParent );
    virtual ~SGFACESET();

    virtual bool SetParent( SGNODE* aParent, bool notify = true ) override;

    SGNODE* FindNode( const char* aNodeName, const SGNODE* aCaller ) override;
    bool AddRefNode( SGNODE* aNode ) override;
    bool AddChildNode( SGNODE* aNode ) override;

    bool CalcNormals( SGNODE** aPtr );

    void ReNameNodes( void ) override;
    bool WriteVRML( std::ostream& aFile, bool aReuseFlag ) override;

    bool WriteCache( std::ostream& aFile, SGNODE* parentNode ) override;
    bool ReadCache( std::istream& aFile, SGNODE* parentNode ) override;

    /**
     * Add all internal coordinate indices to the given list in preparation for a normals
     * calculation.
     */
    void GatherCoordIndices( std::vector< int >& aIndexList );

    void unlinkChildNode( const SGNODE* aNode ) override;
    void unlinkRefNode( const SGNODE* aNode ) override;

    // validate the data held by this face set
    bool validate( void );

    // owned objects
    SGCOLORS*       m_Colors;
    SGCOORDS*       m_Coords;
    SGCOORDINDEX*   m_CoordIndices;
    SGNORMALS*      m_Normals;

    // referenced objects
    SGCOLORS*       m_RColors;
    SGCOORDS*       m_RCoords;
    SGNORMALS*      m_RNormals;

private:
    bool valid;
    bool validated;
    void unlinkNode( const SGNODE* aNode, bool isChild );
    bool addNode( SGNODE* aNode, bool isChild );

};

/*
    p.88
    IndexedFaceSet {
        color               NULL
        coord               NULL
        normal              NULL
        texCoord            NULL
        ccw                 TRUE
        colorIndex          []
        colorPerVertex      TRUE
        convex              TRUE
        coordIndex          []
        creaseAngle         0
        normalIndex         []
        normalPerVertex     TRUE
        solid               TRUE
        texCoordIndex       []
    }
*/

#endif  // SG_FACESET_H
