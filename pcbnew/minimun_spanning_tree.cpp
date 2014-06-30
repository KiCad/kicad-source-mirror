/**
 * @file minimun_spanning_tree.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 Jean-Pierre Charras
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
 *
 * derived from this article:
 * http://compprog.wordpress.com/2007/11/09/minimal-spanning-trees-prims-algorithm
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

#include <limits.h>

#include <minimun_spanning_tree.h>
#include <class_pad.h>

/*
 * The class MIN_SPAN_TREE calculates the rectilinear minimum spanning tree
 * of a set of points (pads usually having the same net)
 * using the Prim's algorithm.
 */

/*
 *  Prim's Algorithm
 *   Step 0
 *   Pick any vertex as a starting vertex. (Call it S).
 *   Mark it with any given flag, say 1.
 *
 *   Step 1
 *   Find the nearest neighbour of S (call it P1).
 *   Mark both P1 and the edge SP1.
 *   cheapest unmarked edge in the graph that doesn't close a marked circuit.
 *   Mark this edge.
 *
 *   Step 2
 *   Find the nearest unmarked neighbour to the marked subgraph
 *   (i.e., the closest vertex to any marked vertex).
 *   Mark it and the edge connecting the vertex.
 *
 *   Step 3
 *   Repeat Step 2 until all vertices are marked.
 *   The marked subgraph is a minimum spanning tree.
 */
MIN_SPAN_TREE::MIN_SPAN_TREE()
{
    MSP_Init( 0 );
}


void MIN_SPAN_TREE::MSP_Init( int aNodesCount )
{
    m_Size = std::max( aNodesCount, 1 );
    inTree.clear();
    linkedTo.clear();
    distTo.clear();

    if( m_Size == 0 )
        return;

    // Reserve space in memory
    inTree.reserve( m_Size );
    linkedTo.reserve( m_Size );
    distTo.reserve( m_Size );

    // Initialize values:
    for( int ii = 0; ii < m_Size; ii++ )
    {
        // Initialise dist with infinity:
        distTo.push_back( INT_MAX );

        // Mark all nodes as NOT beeing in the minimum spanning tree:
        inTree.push_back( 0 );

        linkedTo.push_back( 0 );
    }
}


/* updateDistances(int target)
 *   should be called immediately after target is added to the tree;
 *   updates dist so that the values are correct (goes through target's
 *   neighbours making sure that the distances between them and the tree
 *   are indeed minimum)
 */
void MIN_SPAN_TREE::updateDistances( int target )
{
    for( int ii = 0; ii < m_Size; ++ii )
    {
        if( !inTree[ii] )   // no need to evaluate weight for already in tree items
        {
            int weight = GetWeight( target, ii );
            if( (weight > 0) && (distTo[ii] > weight ) )
            {
                distTo[ii]   = weight;
                linkedTo[ii] = target;
            }
        }
    }
}


void MIN_SPAN_TREE::BuildTree()
{
    // Add the first node to the tree
    inTree[0] = 1;
    updateDistances( 0 );

    for( int treeSize = 1; treeSize < m_Size; ++treeSize )
    {
        // Find the node with the smallest distance to the tree
        int min = -1;

        for( int ii = 0; ii < m_Size; ++ii )
        {
            if( !inTree[ii] )
            {
                if( (min == -1) || (distTo[min] > distTo[ii]) )
                    min = ii;
            }
        }

        inTree[min] = 1;
        updateDistances( min );
    }
}
