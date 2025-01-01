/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
 * @file wrlfacet.h
 * declares classes to help manage normals calculations from VRML files
 */


#ifndef WRLFACET_H
#define WRLFACET_H

#include <list>
#include <vector>
#include "wrltypes.h"
#include "plugins/3dapi/ifsg_all.h"


class SGNODE;

class FACET
{
public:
    FACET();

    void Init();
    bool HasMinPoints();
    bool HasColors();

    /**
     * Add the vertex and its associated index to the internal list of polygon vertices.
     */
    void AddVertex( WRLVEC3F& aVertex, int aIndex );

    /**
     * Add the given RGB color to the internal list.
     *
     * For per-face coloring only a single color needs to be specified.  For a per-vertex
     * coloring the color must be specified for each vertex.
     */
    void AddColor( const SGCOLOR& aColor );

    /**
     * Calculate the normal to the facet assuming a CCW orientation and perform the calculation
     * of the angle weighted vertex normals.
     *
     * @return is the max. magnitude of any component of the normal or zero if there is a
     *         fault or the normal has already been calculated.
     */
    float CalcFaceNormal();

    void Renormalize( float aMaxValue );

    /**
     * Calculate the weighted normal for the given vertex.
     *
     * @param aIndex is the VRML file's Vertex Index for the vertex to be processed.
     * @param aFacetList is the list of all faces which share this vertex.
     */
    void CalcVertexNormal( int aIndex, std::list< FACET* >& aFacetList, float aCreaseAngle );

    /**
     * Retrieve the angle weighted normal for the given vertex index.
     *
     * @param aIndex is the VRML file's Vertex Index for the vertex to be processed.
     * @param aNorm will hold the result.
     */
    bool GetWeightedNormal( int aIndex, WRLVEC3F& aNorm );

    /**
     * Retrieve the normal for this facet.
     *
     * @param aNorm will hold the result.
     */
    bool GetFaceNormal( WRLVEC3F& aNorm );

    /**
     * Package the internal data as triangles with corresponding per-vertex normals.
     *
     * @param aVertexList is the list of vertices to add to.
     * @param aNormalsList is the list of per-vertex normals to add to.
     * @param aColorsList is the list of per-vertex colors (if any) to add to.
     * @param aVertexOrder informs the function of the vertex winding order.
     */
    bool GetData( std::vector< WRLVEC3F >& aVertexList, std::vector< WRLVEC3F >& aNormalsList,
         std::vector< SGCOLOR >& aColorsList, WRL1_ORDER aVertexOrder );

    int GetMaxIndex()
    {
        return maxIdx;
    }

    /**
     * Add a pointer to this object at each position within \a aFacetList referenced by the
     * internal vertex indices.
     */
    void CollectVertices( std::vector< std::list< FACET* > >& aFacetList );

private:
    std::vector< WRLVEC3F > vertices;       // vertices of the facet
    std::vector< SGCOLOR >  colors;         // per-vertex/per-face color (if any)
    std::vector< int >      indices;        // index of each vertex

    WRLVEC3F                face_normal;    // normal of this facet
    std::vector< WRLVEC3F > norms;          // per-vertex normals
    std::vector< WRLVEC3F > vnweight;       // angle weighted per vertex normal

    int maxIdx; // maximum index used
};


class SHAPE
{
public:
    ~SHAPE();

    FACET* NewFacet();
    SGNODE* CalcShape( SGNODE* aParent, SGNODE* aColor, WRL1_ORDER aVertexOrder,
            float aCreaseLimit = 0.74317, bool isVRML2 = false );

private:
    std::list< FACET* > facets;
};

#endif  // WRLFACET_H
