/*
 * Copyright (C) 1998, 2000-2007, 2010, 2011, 2012, 2013 SINTEF ICT,
 * Applied Mathematics, Norway.
 *
 * Contact information: E-mail: tor.dokken@sintef.no
 * SINTEF ICT, Department of Applied Mathematics,
 * P.O. Box 124 Blindern,
 * 0314 Oslo, Norway.
 *
 * This file is part of TTL.
 *
 * TTL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * TTL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public
 * License along with TTL. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * In accordance with Section 7(b) of the GNU Affero General Public
 * License, a covered work must retain the producer line in every data
 * file that is created or manipulated using TTL.
 *
 * Other Usage
 * You can be released from the requirements of the license by purchasing
 * a commercial license. Buying such a license is mandatory as soon as you
 * develop commercial activities involving the TTL library without
 * disclosing the source code of your own applications.
 *
 * This file may be used in accordance with the terms contained in a
 * written agreement between you and SINTEF ICT.
 */

#ifndef _TTL_H_
#define _TTL_H_

#include <list>
#include <iterator>

// Debugging
#ifdef DEBUG_TTL
static void errorAndExit( char* aMessage )
{
    cout << "\n!!! ERROR: " << aMessage << " !!!\n" << endl;
    exit(-1);
}
#endif

// Next on TOPOLOGY:
// - get triangle strips
// - weighted graph, algorithms using a weight (real) for each edge,
//   e.g. an "abstract length". Use for minimum spanning tree
//   or some arithmetics on weights?
// - Circulators as defined in CGAL with more STL compliant code

// - analyze in detail locateFace: e.g. detect 0-orbit in case of infinite loop
//   around a node etc.

/**
 * \brief Main interface to TTL
*
* This namespace contains the basic generic algorithms for the TTL,
* the Triangulation Template Library.\n
*
* Examples of functionality are:
* - Incremental Delaunay triangulation
* - Constrained triangulation
* - Insert/remove nodes and constrained edges
* - Traversal operations
* - Misc. queries for extracting information for visualisation systems etc.
*
* \par General requirements and assumptions:
* - \e DART_TYPE and \e TRAITS_TYPE should be implemented in accordance with the description
*   in \ref api.
* - A \b "Requires:" section in the documentation of a function template
*   shows which functionality is required in \e TRAITS_TYPE to
*   support that specific function.\n
*   Functionalty required in \e DART_TYPE is the same (almost) for all
*   function templates; see \ref api and the example referred to.
* - When a reference to a \e dart object is passed to a function in TTL,
*   it is assumed that it is oriented \e counterclockwise (CCW) in a triangle
*   unless it is explicitly mentioned that it can also be \e clockwise (CW).
*   The same applies for a dart that is passed from a function in TTL to
*   the users TRAITS_TYPE class (or struct).
* - When an edge (represented with a dart) is swapped, it is assumed that darts
*   outside the quadrilateral where the edge is a diagonal are not affected by
*   the swap. Thus, \ref hed::TTLtraits::swapEdge "TRAITS_TYPE::swapEdge"
*   must be implemented in accordance with this rule.
*
* \par Glossary:
* - General terms are explained in \ref api.
* - \e CCW - counterclockwise
* - \e CW  - clockwise
* - \e 0_orbit, \e 1_orbit and \e 2_orbit: A sequence of darts around
*   a node, around an edge and in a triangle respectively;
*   see get_0_orbit_interior and get_0_orbit_boundary
* - \e arc - In a triangulation an arc is equivalent with an edge
*
* \see
* \ref ttl_util and \ref api
*
* \author
* �yvind Hjelle, oyvindhj@ifi.uio.no
*/


namespace ttl
{
class TRIANGULATION_HELPER
{
#ifndef DOXYGEN_SHOULD_SKIP_THIS

public:
    TRIANGULATION_HELPER( hed::TRIANGULATION& aTriang ) :
        m_triangulation( aTriang )
    {
    }

    // Delaunay Triangulation
    template <class TRAITS_TYPE, class DART_TYPE, class POINT_TYPE>
    bool InsertNode( DART_TYPE& aDart, POINT_TYPE& aPoint );

    template <class TRAITS_TYPE, class DART_TYPE>
    void RemoveRectangularBoundary( DART_TYPE& aDart );

    template <class TRAITS_TYPE, class DART_TYPE>
    void RemoveNode( DART_TYPE& aDart );

    template <class TRAITS_TYPE, class DART_TYPE>
    void RemoveBoundaryNode( DART_TYPE& aDart );

    template <class TRAITS_TYPE, class DART_TYPE>
    void RemoveInteriorNode( DART_TYPE& aDart );

    // Topological and Geometric Queries
    // ---------------------------------
    template <class TRAITS_TYPE, class POINT_TYPE, class DART_TYPE>
    static bool LocateFaceSimplest( const POINT_TYPE& aPoint, DART_TYPE& aDart );

    template <class TRAITS_TYPE, class POINT_TYPE, class DART_TYPE>
    static bool LocateTriangle( const POINT_TYPE& aPoint, DART_TYPE& aDart );

    template <class TRAITS_TYPE, class POINT_TYPE, class DART_TYPE>
    static bool InTriangle( const POINT_TYPE& aPoint, const DART_TYPE& aDart );

    template <class DART_TYPE, class DART_LIST_TYPE>
    static void GetBoundary( const DART_TYPE& aDart, DART_LIST_TYPE& aBoundary );

    template <class DART_TYPE>
    static bool IsBoundaryEdge( const DART_TYPE& aDart );

    template <class DART_TYPE>
    static bool IsBoundaryFace( const DART_TYPE& aDart );

    template <class DART_TYPE>
    static bool IsBoundaryNode( const DART_TYPE& aDart );

    template <class DART_TYPE>
    static int GetDegreeOfNode( const DART_TYPE& aDart );

    template <class DART_TYPE, class DART_LIST_TYPE>
    static void Get0OrbitInterior( const DART_TYPE& aDart, DART_LIST_TYPE& aOrbit );

    template <class DART_TYPE, class DART_LIST_TYPE>
    static void Get0OrbitBoundary( const DART_TYPE& aDart, DART_LIST_TYPE& aOrbit );

    template <class DART_TYPE>
    static bool Same0Orbit( const DART_TYPE& aD1, const DART_TYPE& aD2 );

    template <class DART_TYPE>
    static bool Same1Orbit( const DART_TYPE& aD1, const DART_TYPE& aD2 );

    template <class DART_TYPE>
    static bool Same2Orbit( const DART_TYPE& aD1, const DART_TYPE& aD2 );

    template <class TRAITS_TYPE, class DART_TYPE>
    static bool SwappableEdge( const DART_TYPE& aDart, bool aAllowDegeneracy = false );

    template <class DART_TYPE>
    static void PositionAtNextBoundaryEdge( DART_TYPE& aDart );

    template <class TRAITS_TYPE, class DART_TYPE>
    static bool ConvexBoundary( const DART_TYPE& aDart );

    // Utilities for Delaunay Triangulation
    // ------------------------------------
    template <class TRAITS_TYPE, class DART_TYPE, class DART_LIST_TYPE>
    void OptimizeDelaunay( DART_LIST_TYPE& aElist );

    template <class TRAITS_TYPE, class DART_TYPE, class DART_LIST_TYPE>
    void OptimizeDelaunay( DART_LIST_TYPE& aElist, const typename DART_LIST_TYPE::iterator aEnd );

    template <class TRAITS_TYPE, class DART_TYPE>
    bool SwapTestDelaunay( const DART_TYPE& aDart, bool aCyclingCheck = false ) const;

    template <class TRAITS_TYPE, class DART_TYPE>
    void RecSwapDelaunay( DART_TYPE& aDiagonal );

    template <class TRAITS_TYPE, class DART_TYPE, class LIST_TYPE>
    void SwapEdgesAwayFromInteriorNode( DART_TYPE& aDart, LIST_TYPE& aSwappedEdges );

    template <class TRAITS_TYPE, class DART_TYPE, class LIST_TYPE>
    void SwapEdgesAwayFromBoundaryNode( DART_TYPE& aDart, LIST_TYPE& aSwappedEdges );

    template <class TRAITS_TYPE, class DART_TYPE, class DART_LIST_TYPE>
    void SwapEdgeInList( const typename DART_LIST_TYPE::iterator& aIt, DART_LIST_TYPE& aElist );

    // Constrained Triangulation
    // -------------------------
    template <class TRAITS_TYPE, class DART_TYPE>
    static DART_TYPE InsertConstraint( DART_TYPE& aDStart, DART_TYPE& aDEnd, bool aOptimizeDelaunay );

private:
    hed::TRIANGULATION& m_triangulation;

    template <class TRAITS_TYPE, class FORWARD_ITERATOR, class DART_TYPE>
    void insertNodes( FORWARD_ITERATOR aFirst, FORWARD_ITERATOR aLast, DART_TYPE& aDart );

    template <class TOPOLOGY_ELEMENT_TYPE, class DART_TYPE>
    static bool isMemberOfFace( const TOPOLOGY_ELEMENT_TYPE& aTopologyElement, const DART_TYPE& aDart );

    template <class TRAITS_TYPE, class NODE_TYPE, class DART_TYPE>
    static bool locateFaceWithNode( const NODE_TYPE& aNode, DART_TYPE& aDartIter );

    template <class DART_TYPE>
    static void getAdjacentTriangles( const DART_TYPE& aDart, DART_TYPE& aT1, DART_TYPE& aT2,
                                      DART_TYPE& aT3 );

    template <class DART_TYPE>
    static void getNeighborNodes( const DART_TYPE& aDart, std::list<DART_TYPE>& aNodeList,
                                  bool& aBoundary );

    template <class TRAITS_TYPE, class DART_TYPE>
    static bool degenerateTriangle( const DART_TYPE& aDart );
};

#endif // DOXYGEN_SHOULD_SKIP_THIS


  /** @name Delaunay Triangulation */
//@{
/**
 * Inserts a new node in an existing Delaunay triangulation and
 * swaps edges to obtain a new Delaunay triangulation.
 * This is the basic function for incremental Delaunay triangulation.
 * When starting from a set of points, an initial Delaunay triangulation
 * can be created as two triangles forming a rectangle that contains
 * all the points.
 * After \c insertNode has been called repeatedly with all the points,
 * removeRectangularBoundary can be called to remove triangles
 * at the boundary of the triangulation so that the boundary
 * form the convex hull of the points.
 *
 * Note that this incremetal scheme will run much faster if the points
 * have been sorted lexicographically on \e x and \e y.
 *
 * \param aDart
 * An arbitrary CCW dart in the tringulation.\n
 * Output: A CCW dart incident to the new node.
 *
 * \param aPoint
 * A point (node) to be inserted in the triangulation.
 *
 * \retval bool
 * \c true if \e point was inserted; \c false if not.\n
 * If \e point is outside the triangulation, or the input dart is not valid,
 * \c false is returned.
 *
 * \require
 *  - \ref hed::TTLtraits::splitTriangle "TRAITS_TYPE::splitTriangle" (DART_TYPE&, const POINT_TYPE&)
 *
 * \using
 * - locateTriangle
 * - RecSwapDelaunay
 *
 * \note
 * - For efficiency reasons \e dart should be close to the insertion \e point.
 *
 * \see
 * removeRectangularBoundary
 */
template <class TRAITS_TYPE, class DART_TYPE, class POINT_TYPE>
bool TRIANGULATION_HELPER::InsertNode( DART_TYPE& aDart, POINT_TYPE& aPoint )
{
    bool found = LocateTriangle<TRAITS_TYPE>( aPoint, aDart );

    if( !found )
    {
#ifdef DEBUG_TTL
        cout << "ERROR: Triangulation::insertNode: NO triangle found. /n";
#endif
        return false;
    }

    // ??? can we hide the dart? this is not possible if one triangle only
    m_triangulation.splitTriangle( aDart, aPoint );

    DART_TYPE d1 = aDart;
    d1.Alpha2().Alpha1().Alpha2().Alpha0().Alpha1();

    DART_TYPE d2 = aDart;
    d2.Alpha1().Alpha0().Alpha1();

    // Preserve a dart as output incident to the node and CCW
    DART_TYPE d3 = aDart;
    d3.Alpha2();
    aDart = d3; // and see below
    //DART_TYPE dsav = d3;
    d3.Alpha0().Alpha1();

    //if (!TRAITS_TYPE::fixedEdge(d1) && !IsBoundaryEdge(d1)) {
    if( !IsBoundaryEdge( d1 ) )
    {
        d1.Alpha2();
        RecSwapDelaunay<TRAITS_TYPE>( d1 );
    }

    //if (!TRAITS_TYPE::fixedEdge(d2) && !IsBoundaryEdge(d2)) {
    if( !IsBoundaryEdge( d2 ) )
    {
        d2.Alpha2();
        RecSwapDelaunay<TRAITS_TYPE>( d2 );
    }

    // Preserve the incoming dart as output incident to the node and CCW
    //d = dsav.Alpha2();
    aDart.Alpha2();
    //if (!TRAITS_TYPE::fixedEdge(d3) && !IsBoundaryEdge(d3)) {
    if( !IsBoundaryEdge( d3 ) )
    {
        d3.Alpha2();
        RecSwapDelaunay<TRAITS_TYPE>( d3 );
    }

    return true;
}

//------------------------------------------------------------------------------------------------
// Private/Hidden function (might change later)
template <class TRAITS_TYPE, class FORWARD_ITERATOR, class DART_TYPE>
void TRIANGULATION_HELPER::insertNodes( FORWARD_ITERATOR aFirst, FORWARD_ITERATOR aLast,
                                        DART_TYPE& aDart )
{

    // Assumes that the dereferenced point objects are pointers.
    // References to the point objects are then passed to TTL.

    FORWARD_ITERATOR it;
    for( it = aFirst; it != aLast; ++it )
    {
        InsertNode<TRAITS_TYPE>( aDart, **it );
    }
}


/** Removes the rectangular boundary of a triangulation as a final step of an
 *   incremental Delaunay triangulation.
 *   The four nodes at the corners will be removed and the resulting triangulation
 *   will have a convex boundary and be Delaunay.
 *
 *   \param dart
 *   A CCW dart at the boundary of the triangulation\n
 *   Output: A CCW dart at the new boundary
 *
 *   \using
 *   - RemoveBoundaryNode
 *
 *   \note
 *   - This function requires that the boundary of the m_triangulation is
 *     a rectangle with four nodes (one in each corner).
 */
template <class TRAITS_TYPE, class DART_TYPE>
void TRIANGULATION_HELPER::RemoveRectangularBoundary( DART_TYPE& aDart )
{
    DART_TYPE d_next = aDart;
    DART_TYPE d_iter;

    for( int i = 0; i < 4; i++ )
    {
        d_iter = d_next;
        d_next.Alpha0();
        PositionAtNextBoundaryEdge( d_next );
        RemoveBoundaryNode<TRAITS_TYPE>( d_iter );
    }

    aDart = d_next; // Return a dart at the new boundary
}

/** Removes the node associated with \e dart and
 *   updates the triangulation to be Delaunay.
 *
 *   \using
 *   - RemoveBoundaryNode if \e dart represents a node at the boundary
 *   - RemoveInteriorNode if \e dart represents an interior node
 *
 *   \note
 *   - The node cannot belong to a fixed (constrained) edge that is not
 *     swappable. (An endless loop is likely to occur in this case).
 */
template <class TRAITS_TYPE, class DART_TYPE>
void TRIANGULATION_HELPER::RemoveNode( DART_TYPE& aDart )
{

    if( isBoundaryNode( aDart ) )
        RemoveBoundaryNode<TRAITS_TYPE>( aDart );
    else
        RemoveInteriorNode<TRAITS_TYPE>( aDart );
}

/** Removes the boundary node associated with \e dart and
 *   updates the triangulation to be Delaunay.
 *
 *   \using
 *   - SwapEdgesAwayFromBoundaryNode
 *   - OptimizeDelaunay
 *
 *   \require
 *   - \ref hed::TTLtraits::removeBoundaryTriangle "TRAITS_TYPE::removeBoundaryTriangle" (Dart&)
 */
template <class TRAITS_TYPE, class DART_TYPE>
void TRIANGULATION_HELPER::RemoveBoundaryNode( DART_TYPE& aDart )
{

    // ... and update Delaunay
    // - CCW dart must be given (for remove)
    // - No dart is delivered back now (but this is possible if
    //   we assume that there is not only one triangle left in the m_triangulation.

    // Position at boundary edge and CCW
    if( !IsBoundaryEdge( aDart ) )
    {
        aDart.Alpha1(); // ensures that next function delivers back a CCW dart (if the given dart is CCW)
        PositionAtNextBoundaryEdge( aDart );
    }

    std::list<DART_TYPE> swapped_edges;
    SwapEdgesAwayFromBoundaryNode<TRAITS_TYPE>( aDart, swapped_edges );

    // Remove boundary triangles and remove the new boundary from the list
    // of swapped edges, see below.
    DART_TYPE d_iter = aDart;
    DART_TYPE dnext = aDart;
    bool bend = false;
    while( bend == false )
    {
        dnext.Alpha1().Alpha2();
        if( IsBoundaryEdge( dnext ) )
            bend = true; // Stop when boundary

        // Generic: Also remove the new boundary from the list of swapped edges
        DART_TYPE n_bedge = d_iter;
        n_bedge.Alpha1().Alpha0().Alpha1().Alpha2(); // new boundary edge

        // ??? can we avoid find if we do this in swap away?
        typename std::list<DART_TYPE>::iterator it;
        it = find( swapped_edges.begin(), swapped_edges.end(), n_bedge );

        if( it != swapped_edges.end() )
            swapped_edges.erase( it );

        // Remove the boundary triangle
        m_triangulation.removeBoundaryTriangle( d_iter );
        d_iter = dnext;
    }

    // Optimize Delaunay
    typedef std::list<DART_TYPE> DART_LIST_TYPE;
    OptimizeDelaunay<TRAITS_TYPE, DART_TYPE, DART_LIST_TYPE>( swapped_edges );
}


/** Removes the interior node associated with \e dart and
 *   updates the triangulation to be Delaunay.
 *
 *   \using
 *   - SwapEdgesAwayFromInteriorNode
 *   - OptimizeDelaunay
 *
 *   \require
 *   - \ref hed::TTLtraits::reverse_splitTriangle "TRAITS_TYPE::reverse_splitTriangle" (Dart&)
 *
 *   \note
 *   - The node cannot belong to a fixed (constrained) edge that is not
 *     swappable. (An endless loop is likely to occur in this case).
 */
template <class TRAITS_TYPE, class DART_TYPE>
void TRIANGULATION_HELPER::RemoveInteriorNode( DART_TYPE& aDart )
{
    // ... and update to Delaunay.
    // Must allow degeneracy temporarily, see comments in swap edges away
    // Assumes:
    // - revese_splitTriangle does not affect darts
    //   outside the resulting triangle.

    // 1) Swaps edges away from the node until degree=3 (generic)
    // 2) Removes the remaining 3 triangles and creates a new to fill the hole
    //    unsplitTriangle which is required
    // 3) Runs LOP on the platelet to obtain a Delaunay m_triangulation
    // (No dart is delivered as output)

    // Assumes dart is counterclockwise

    std::list<DART_TYPE> swapped_edges;
    SwapEdgesAwayFromInteriorNode<TRAITS_TYPE>( aDart, swapped_edges );

    // The reverse operation of split triangle:
    // Make one triangle of the three triangles at the node associated with dart
    // TRAITS_TYPE::
    m_triangulation.reverseSplitTriangle( aDart );

    // ???? Not generic yet if we are very strict:
    // When calling unsplit triangle, darts at the three opposite sides may
    // change!
    // Should we hide them longer away??? This is possible since they cannot
    // be boundary edges.
    // ----> Or should we just require that they are not changed???

    // Make the swapped-away edges Delaunay.
    // Note the theoretical result: if there are no edges in the list,
    // the triangulation is Delaunay already

    OptimizeDelaunay<TRAITS_TYPE, DART_TYPE>( swapped_edges );
}

//@} // End of Delaunay Triangulation Group

/** @name Topological and Geometric Queries */
//@{
//------------------------------------------------------------------------------------------------
// Private/Hidden function (might change later)
template <class TOPOLOGY_ELEMENT_TYPE, class DART_TYPE>
bool TRIANGULATION_HELPER::isMemberOfFace( const TOPOLOGY_ELEMENT_TYPE& aTopologyElement,
                                           const DART_TYPE& aDart )
{
    // Check if the given topology element (node, edge or face) is a member of the face
    // Assumes:
    // - DART_TYPE::isMember(TOPOLOGY_ELEMENT_TYPE)

    DART_TYPE dart_iter = aDart;

    do
    {
        if( dart_iter.isMember( aTopologyElement ) )
            return true;
        dart_iter.Alpha0().Alpha1();
    }
    while( dart_iter != aDart );

    return false;
}

//------------------------------------------------------------------------------------------------
// Private/Hidden function (might change later)
template <class TRAITS_TYPE, class NODE_TYPE, class DART_TYPE>
bool TRIANGULATION_HELPER::locateFaceWithNode( const NODE_TYPE& aNode, DART_TYPE& aDartIter )
{
    // Locate a face in the topology structure with the given node as a member
    // Assumes:
    // - TRAITS_TYPE::Orient2D(DART_TYPE, DART_TYPE, NODE_TYPE)
    // - DART_TYPE::isMember(NODE_TYPE)
    // - Note that if false is returned, the node might still be in the
    //   topology structure. Application programmer
    //   should check all if by hypothesis the node is in the topology structure;
    //   see doc. on LocateTriangle.

    bool status = LocateFaceSimplest<TRAITS_TYPE>( aNode, aDartIter );

    if( status == false )
        return status;

    // True was returned from LocateFaceSimplest, but if the located triangle is
    // degenerate and the node is on the extension of the edges,
    // the node might still be inside. Check if node is a member and return false
    // if not. (Still the node might be in the topology structure, see doc. above
    // and in locateTriangle(const POINT_TYPE& point, DART_TYPE& dart_iter)

    return isMemberOfFace( aNode, aDartIter );
}

/** Locates the face containing a given point.
 *   It is assumed that the tessellation (e.g. a triangulation) is \e regular in the sense that
 *   there are no holes, the boundary is convex and there are no degenerate faces.
 *
 *   \param aPoint
 *   A point to be located
 *
 *   \param aDart
 *   An arbitrary CCW dart in the triangulation\n
 *   Output: A CCW dart in the located face
 *
 *   \retval bool
 *   \c true if a face is found; \c false if not.
 *
 *   \require
 *   - \ref hed::TTLtraits::Orient2D "TRAITS_TYPE::Orient2D" (DART_TYPE&, DART_TYPE&, POINT_TYPE&)
 *
 *   \note
 *   - If \c false is returned, \e point may still be inside a face if the tessellation is not
 *     \e regular as explained above.
 *
 *   \see
 *   LocateTriangle
 */
template <class TRAITS_TYPE, class POINT_TYPE, class DART_TYPE>
bool TRIANGULATION_HELPER::LocateFaceSimplest( const POINT_TYPE& aPoint, DART_TYPE& aDart )
{
    // Not degenerate triangles if point is on the extension of the edges
    // But inTriangle may be called in case of true (may update to inFace2)
    // Convex boundary
    // no holes
    // convex faces (works for general convex faces)
    // Not specialized for triangles, but ok?
    //
    // TRAITS_TYPE::orint2d(POINT_TYPE) is the half open half-plane defined
    // by the dart:
    // n1 = dart.node()
    // n2 = dart.Alpha0().node
    // Only the following gives true:
    // ((n2->x()-n1->x())*(point.y()-n1->y()) >= (point.x()-n1->x())*(n2->y()-n1->y()))

    DART_TYPE dart_start;
    dart_start = aDart;
    DART_TYPE dart_prev;

    DART_TYPE d0;
    for( ;; )
    {
        d0 = aDart;
        d0.Alpha0();

        if( TRAITS_TYPE::Orient2D( aDart, d0, aPoint ) >= 0 )
        {
            aDart.Alpha0().Alpha1();
            if( aDart == dart_start )
                return true; // left to all edges in face
        }
        else
        {
            dart_prev = aDart;
            aDart.Alpha2();

            if( aDart == dart_prev )
                return false; // iteration to outside boundary

            dart_start = aDart;
            dart_start.Alpha0();

            aDart.Alpha1(); // avoid twice on same edge and ccw in next
        }
    }
}


/** Locates the triangle containing a given point.
 *   It is assumed that the triangulation is \e regular in the sense that there
 *   are no holes and the boundary is convex.
 *   This function deals with degeneracy to some extent, but round-off errors may still
 *   lead to a wrong result if triangles are degenerate.
 *
 *   \param point
 *   A point to be located
 *
 *   \param dart
 *   An arbitrary CCW dart in the triangulation\n
 *   Output: A CCW dart in the located triangle
 *
 *   \retval bool
 *   \c true if a triangle is found; \c false if not.\n
 *   If \e point is outside the m_triangulation, in which case \c false is returned,
 *   then the edge associated with \e dart will be at the boundary of the m_triangulation.
 *
 *   \using
 *   - LocateFaceSimplest
 *   - InTriangle
 */
template <class TRAITS_TYPE, class POINT_TYPE, class DART_TYPE>
bool TRIANGULATION_HELPER::LocateTriangle( const POINT_TYPE& aPoint, DART_TYPE& aDart )
{
    // The purpose is to have a fast and stable procedure that
    //  i) avoids concluding that a point is inside a triangle if it is not inside
    // ii) avoids infinite loops

    // Thus, if false is returned, the point might still be inside a triangle in
    // the triangulation. But this will probably only occur in the following cases:
    //  i) There are holes in the triangulation which causes the procedure to stop.
    // ii) The boundary of the m_triangulation is not convex.
    // ii) There might be degenerate triangles interior to the triangulation, or on the
    //     the boundary, which in some cases might cause the procedure to stop there due
    //     to the logic of the algorithm.

    // It is the application programmer's responsibility to check further if false is
    // returned. For example, if by hypothesis the point is inside a triangle
    // in the triangulation and and false is returned, then all triangles in the
    // triangulation should be checked by the application. This can be done using
    // the function:
    // bool inTriangle(const POINT_TYPE& point, const DART_TYPE& dart).

    // Assumes:
    // - CrossProduct2D, ScalarProduct2D etc., see functions called

    bool status = LocateFaceSimplest<TRAITS_TYPE>( aPoint, aDart );

    if( status == false )
        return status;

    // There may be degeneracy, i.e., the point might be outside the triangle
    // on the extension of the edges of a degenerate triangle.

    // The next call returns true if inside a non-degenerate or a degenerate triangle,
    // but false if the point coincides with the "supernode" in the case where all
    // edges are degenerate.
    return InTriangle<TRAITS_TYPE>( aPoint, aDart );
}

/** Checks if \e point is inside the triangle associated with \e dart.
 *   This function deals with degeneracy to some extent, but round-off errors may still
 *   lead to wrong result if the triangle is degenerate.
 *
 *   \param aDart
 *   A CCW dart in the triangle
 *
 *   \require
 *    - \ref hed::TTLtraits::CrossProduct2D "TRAITS_TYPE::CrossProduct2D" (DART_TYPE&, POINT_TYPE&)
 *    - \ref hed::TTLtraits::ScalarProduct2D "TRAITS_TYPE::ScalarProduct2D" (DART_TYPE&, POINT_TYPE&)
 *
 *   \see
 *   InTriangleSimplest
 */
template <class TRAITS_TYPE, class POINT_TYPE, class DART_TYPE>
bool TRIANGULATION_HELPER::InTriangle( const POINT_TYPE& aPoint, const DART_TYPE& aDart )
{

    // SHOULD WE INCLUDE A STRATEGY WITH EDGE X e_1 ETC? TO GUARANTEE THAT
    // ONLY ON ONE EDGE? BUT THIS DOES NOT SOLVE PROBLEMS WITH
    // notInE1 && notInE1.neghbour ?

    // Returns true if inside (but not necessarily strictly inside)
    // Works for degenerate triangles, but not when all edges are degenerate,
    // and the aPoint coincides with all nodes;
    // then false is always returned.

    typedef typename TRAITS_TYPE::REAL_TYPE REAL_TYPE;

    DART_TYPE dart_iter = aDart;

    REAL_TYPE cr1 = TRAITS_TYPE::CrossProduct2D( dart_iter, aPoint );
    if( cr1 < 0 )
        return false;

    dart_iter.Alpha0().Alpha1();
    REAL_TYPE cr2 = TRAITS_TYPE::CrossProduct2D( dart_iter, aPoint );

    if( cr2 < 0 )
        return false;

    dart_iter.Alpha0().Alpha1();
    REAL_TYPE cr3 = TRAITS_TYPE::CrossProduct2D( dart_iter, aPoint );
    if( cr3 < 0 )
        return false;

    // All cross products are >= 0
    // Check for degeneracy
    if( cr1 != 0 || cr2 != 0 || cr3 != 0 )
        return true; // inside non-degenerate face

    // All cross-products are zero, i.e. degenerate triangle, check if inside
    // Strategy: d.ScalarProduct2D >= 0 && alpha0(d).d.ScalarProduct2D >= 0 for one of
    // the edges. But if all edges are degenerate and the aPoint is on (all) the nodes,
    // then "false is returned".

    DART_TYPE dart_tmp = dart_iter;
    REAL_TYPE sc1 = TRAITS_TYPE::ScalarProduct2D( dart_tmp, aPoint );
    REAL_TYPE sc2 = TRAITS_TYPE::ScalarProduct2D( dart_tmp.Alpha0(), aPoint );

    if( sc1 >= 0 && sc2 >= 0 )
    {
        // test for degenerate edge
        if( sc1 != 0 || sc2 != 0 )
            return true; // interior to this edge or on a node (but see comment above)
    }

    dart_tmp = dart_iter.Alpha0().Alpha1();
    sc1 = TRAITS_TYPE::ScalarProduct2D( dart_tmp, aPoint );
    sc2 = TRAITS_TYPE::ScalarProduct2D( dart_tmp.Alpha0(), aPoint );

    if( sc1 >= 0 && sc2 >= 0 )
    {
        // test for degenerate edge
        if( sc1 != 0 || sc2 != 0 )
            return true; // interior to this edge or on a node (but see comment above)
    }

    dart_tmp = dart_iter.Alpha1();
    sc1 = TRAITS_TYPE::ScalarProduct2D( dart_tmp, aPoint );
    sc2 = TRAITS_TYPE::ScalarProduct2D( dart_tmp.Alpha0(), aPoint );

    if( sc1 >= 0 && sc2 >= 0 )
    {
        // test for degenerate edge
        if( sc1 != 0 || sc2 != 0 )
            return true; // interior to this edge or on a node (but see comment above)
    }

    // Not on any of the edges of the degenerate triangle.
    // The only possibility for the aPoint to be "inside" is that all edges are degenerate
    // and the point coincide with all nodes. So false is returned in this case.

    return false;
}


  //------------------------------------------------------------------------------------------------
// Private/Hidden function (might change later)
template <class DART_TYPE>
void TRIANGULATION_HELPER::getAdjacentTriangles( const DART_TYPE& aDart, DART_TYPE& aT1,
                                                 DART_TYPE& aT2, DART_TYPE& aT3 )
{

    DART_TYPE dart_iter = aDart;

    // add first
    if( dart_iter.Alpha2() != aDart )
    {
        aT1 = dart_iter;
        dart_iter = aDart;
    }

    // add second
    dart_iter.Alpha0();
    dart_iter.Alpha1();
    DART_TYPE dart_prev = dart_iter;

    if( ( dart_iter.Alpha2() ) != dart_prev )
    {
        aT2 = dart_iter;
        dart_iter = dart_prev;
    }

    // add third
    dart_iter.Alpha0();
    dart_iter.Alpha1();
    dart_prev = dart_iter;

    if( ( dart_iter.Alpha2() ) != dart_prev )
        aT3 = dart_iter;
}

//------------------------------------------------------------------------------------------------
/** Gets the boundary as sequence of darts, where the edges associated with the darts are boundary
 *   edges, given a dart with an associating edge at the boundary of a topology structure.
 *   The first dart in the sequence will be the given one, and the others will have the same
 *   orientation (CCW or CW) as the first.
 *   Assumes that the given dart is at the boundary.
 *
 *   \param aDart
 *   A dart at the boundary (CCW or CW)
 *
 *   \param aBoundary
 *   A sequence of darts, where the associated edges are the boundary edges
 *
 *   \require
 *   - DART_LIST_TYPE::push_back (DART_TYPE&)
 */
template <class DART_TYPE, class DART_LIST_TYPE>
void TRIANGULATION_HELPER::GetBoundary( const DART_TYPE& aDart, DART_LIST_TYPE& aBoundary )
{
    // assumes the given dart is at the boundary (by edge)

    DART_TYPE dart_iter( aDart );
    aBoundary.push_back( dart_iter ); // Given dart as first element
    dart_iter.Alpha0();
    PositionAtNextBoundaryEdge( dart_iter );

    while( dart_iter != aDart )
    {
        aBoundary.push_back( dart_iter );
        dart_iter.Alpha0();
        PositionAtNextBoundaryEdge( dart_iter );
    }
}

/** Checks if the edge associated with \e dart is at
 *   the boundary of the m_triangulation.
 *
 *   \par Implements:
 *   \code
 *   DART_TYPE dart_iter = dart;
 *   if (dart_iter.Alpha2() == dart)
 *     return true;
 *   else
 *     return false;
 *   \endcode
 */
template <class DART_TYPE>
bool TRIANGULATION_HELPER::IsBoundaryEdge( const DART_TYPE& aDart )
{
    DART_TYPE dart_iter = aDart;

    if( dart_iter.Alpha2() == aDart )
        return true;
    else
        return false;
}

/** Checks if the face associated with \e dart is at
 *   the boundary of the m_triangulation.
 */
template <class DART_TYPE>
bool TRIANGULATION_HELPER::IsBoundaryFace( const DART_TYPE& aDart )
{
    // Strategy: boundary if alpha2(d)=d

    DART_TYPE dart_iter( aDart );
    DART_TYPE dart_prev;

    do
    {
        dart_prev = dart_iter;

        if( dart_iter.Alpha2() == dart_prev )
            return true;
        else
            dart_iter = dart_prev; // back again

        dart_iter.Alpha0();
        dart_iter.Alpha1();

    } while( dart_iter != aDart );

    return false;
}

/** Checks if the node associated with \e dart is at
 *   the boundary of the m_triangulation.
 */
template <class DART_TYPE>
bool TRIANGULATION_HELPER::IsBoundaryNode( const DART_TYPE& aDart )
{
    // Strategy: boundary if alpha2(d)=d

    DART_TYPE dart_iter( aDart );
    DART_TYPE dart_prev;

    // If input dart is reached again, then internal node
    // If alpha2(d)=d, then boundary

    do
    {
        dart_iter.Alpha1();
        dart_prev = dart_iter;
        dart_iter.Alpha2();

        if( dart_iter == dart_prev )
            return true;

    } while( dart_iter != aDart );

    return false;
}

/** Returns the degree of the node associated with \e dart.
 *
 *   \par Definition:
 *   The \e degree (or valency) of a node \e V in a m_triangulation,
 *   is defined as the number of edges incident with \e V, i.e.,
 *   the number of edges joining \e V with another node in the triangulation.
 */
template <class DART_TYPE>
int TRIANGULATION_HELPER::GetDegreeOfNode( const DART_TYPE& aDart )
{
    DART_TYPE dart_iter( aDart );
    DART_TYPE dart_prev;

    // If input dart is reached again, then interior node
    // If alpha2(d)=d, then boundary

    int degree = 0;
    bool boundaryVisited = false;
    do
    {
        dart_iter.Alpha1();
        degree++;
        dart_prev = dart_iter;

        dart_iter.Alpha2();

        if( dart_iter == dart_prev )
        {
            if( !boundaryVisited )
            {
                boundaryVisited = true;
                // boundary is reached first time, count in the reversed direction
                degree++; // count the start since it is not done above
                dart_iter = aDart;
                dart_iter.Alpha2();
            } else
                return degree;
        }

    } while( dart_iter != aDart );

    return degree;
}

// Modification of GetDegreeOfNode:
// Strategy, reverse the list and start in the other direction if the boundary
// is reached. NB. copying of darts but ok., or we could have collected pointers,
// but the memory management.

// NOTE: not symmetry if we choose to collect opposite edges
//       now we collect darts with radiating edges

// Remember that we must also copy the node, but ok with push_back
// The size of the list will be the degree of the node

// No CW/CCW since topology only

// Each dart consists of an incident edge and an adjacent node.
// But note that this is only how we interpret the dart in this implementation.
// Given this list, how can we find the opposite edges:
//   We can perform alpha1 on each, but for boundary nodes we will get one edge twice.
//   But this is will always be the last dart!
// The darts in the list are in sequence and starts with the alpha0(dart)
// alpha0, alpha1 and alpha2

// Private/Hidden function
template <class DART_TYPE>
void TRIANGULATION_HELPER::getNeighborNodes( const DART_TYPE& aDart,
                                             std::list<DART_TYPE>& aNodeList, bool& aBoundary )
{
    DART_TYPE dart_iter( aDart );
    dart_iter.Alpha0(); // position the dart at an opposite node

    DART_TYPE dart_prev = dart_iter;
    bool start_at_boundary = false;
    dart_iter.Alpha2();

    if( dart_iter == dart_prev )
        start_at_boundary = true;
    else
        dart_iter = dart_prev; // back again

    DART_TYPE dart_start = dart_iter;

    do
    {
        aNodeList.push_back( dart_iter );
        dart_iter.Alpha1();
        dart_iter.Alpha0();
        dart_iter.Alpha1();
        dart_prev = dart_iter;
        dart_iter.Alpha2();

        if( dart_iter == dart_prev )
        {
            // boundary reached
            aBoundary = true;

            if( start_at_boundary == true )
            {
                // add the dart which now is positioned at the opposite boundary
                aNodeList.push_back( dart_iter );
                return;
            }
            else
            {
                // call the function again such that we start at the boundary
                // first clear the list and reposition to the initial node
                dart_iter.Alpha0();
                aNodeList.clear();
                getNeighborNodes( dart_iter, aNodeList, aBoundary );

                return; // after one recursive step
            }
        }
    }
    while( dart_iter != dart_start );

    aBoundary = false;
}

/** Gets the 0-orbit around an interior node.
 *
 *   \param aDart
 *   A dart (CCW or CW) positioned at an \e interior node.
 *
 *   \retval aOrbit
 *   Sequence of darts with one orbit for each arc. All the darts have the same
 *   orientation (CCW or CW) as \e dart, and \e dart is the first element
 *   in the sequence.
 *
 *   \require
 *   - DART_LIST_TYPE::push_back (DART_TYPE&)
 *
 *   \see
 *   Get0OrbitBoundary
 */
template <class DART_TYPE, class DART_LIST_TYPE>
void TRIANGULATION_HELPER::Get0OrbitInterior( const DART_TYPE& aDart, DART_LIST_TYPE& aOrbit )
{
    DART_TYPE d_iter = aDart;
    aOrbit.push_back( d_iter );
    d_iter.Alpha1().Alpha2();

    while( d_iter != aDart )
    {
        aOrbit.push_back( d_iter );
        d_iter.Alpha1().Alpha2();
    }
}

/** Gets the 0-orbit around a node at the boundary
 *
 *   \param aDart
 *   A dart (CCW or CW) positioned at a \e boundary \e node and at a \e boundary \e edge.
 *
 *   \retval orbit
 *   Sequence of darts with one orbit for each arc. All the darts, \e exept \e the \e last one,
 *   have the same orientation (CCW or CW) as \e dart, and \e dart is the first element
 *   in the sequence.
 *
 *   \require
 *   - DART_LIST_TYPE::push_back (DART_TYPE&)
 *
 *   \note
 *   - The last dart in the sequence have opposite orientation compared to the others!
 *
 *   \see
 *   Get0OrbitInterior
 */
template <class DART_TYPE, class DART_LIST_TYPE>
void TRIANGULATION_HELPER::Get0OrbitBoundary( const DART_TYPE& aDart, DART_LIST_TYPE& aOrbit )
{
    DART_TYPE dart_prev;
    DART_TYPE d_iter = aDart;

    do
    {
        aOrbit.push_back( d_iter );
        d_iter.Alpha1();
        dart_prev = d_iter;
        d_iter.Alpha2();
    }
    while( d_iter != dart_prev );

    aOrbit.push_back( d_iter ); // the last one with opposite orientation
}

/** Checks if the two darts belong to the same 0-orbit, i.e.,
 *   if they share a node.
 *   \e d1 and/or \e d2 can be CCW or CW.
 *
 *   (This function also examines if the the node associated with
 *   \e d1 is at the boundary, which slows down the function (slightly).
 *   If it is known that the node associated with \e d1 is an interior
 *   node and a faster version is needed, the user should implement his/her
 *   own version.)
 */
template <class DART_TYPE>
bool TRIANGULATION_HELPER::Same0Orbit( const DART_TYPE& aD1, const DART_TYPE& aD2 )
{
    // Two copies of the same dart
    DART_TYPE d_iter = aD2;
    DART_TYPE d_end = aD2;

    if( isBoundaryNode( d_iter ) )
    {
        // position at both boundary edges
        PositionAtNextBoundaryEdge( d_iter );
        d_end.Alpha1();
        PositionAtNextBoundaryEdge( d_end );
    }

    for( ;; )
    {
        if( d_iter == aD1 )
            return true;

        d_iter.Alpha1();

        if( d_iter == aD1 )
            return true;

        d_iter.Alpha2();

        if( d_iter == d_end )
            break;
    }

    return false;
}

/** Checks if the two darts belong to the same 1-orbit, i.e.,
 *   if they share an edge.
 *   \e d1 and/or \e d2 can be CCW or CW.
 */
template <class DART_TYPE>
bool TRIANGULATION_HELPER::Same1Orbit( const DART_TYPE& aD1, const DART_TYPE& aD2 )
{
    DART_TYPE d_iter = aD2;

    // (Also works at the boundary)
    return ( d_iter == aD1 || d_iter.Alpha0() == aD1 ||
             d_iter.Alpha2() == aD1 || d_iter.Alpha0() == aD1 );
}

//------------------------------------------------------------------------------------------------
/** Checks if the two darts belong to the same 2-orbit, i.e.,
 *   if they lie in the same triangle.
 *   \e d1 and/or \e d2 can be CCW or CW
 */
template <class DART_TYPE>
bool TRIANGULATION_HELPER::Same2Orbit( const DART_TYPE& aD1, const DART_TYPE& aD2 )
{
    DART_TYPE d_iter = aD2;

    return ( d_iter == aD1 || d_iter.Alpha0() == aD1 || d_iter.Alpha1() == aD1 ||
            d_iter.Alpha0() == aD1 || d_iter.Alpha1() == aD1 || d_iter.Alpha0() == aD1 );
}

// Private/Hidden function
template <class TRAITS_TYPE, class DART_TYPE>
bool TRIANGULATION_HELPER::degenerateTriangle( const DART_TYPE& aDart )
{
    // Check if triangle is degenerate
    // Assumes CCW dart

    DART_TYPE d1 = aDart;
    DART_TYPE d2 = d1;
    d2.Alpha1();

    return ( TRAITS_TYPE::CrossProduct2D( d1, d2 ) == 0 );
}

/** Checks if the edge associated with \e dart is swappable, i.e., if the edge
 *   is a diagonal in a \e strictly convex (or convex) quadrilateral.
 *
 *   \param aAllowDegeneracy
 *   If set to true, the function will also return true if the numerical calculations
 *   indicate that the quadrilateral is convex only, and not necessarily strictly
 *   convex.
 *
 *   \require
 *   - \ref hed::TTLtraits::CrossProduct2D "TRAITS_TYPE::CrossProduct2D" (Dart&, Dart&)
 */
template <class TRAITS_TYPE, class DART_TYPE>
bool TRIANGULATION_HELPER::SwappableEdge( const DART_TYPE& aDart, bool aAllowDegeneracy )
{
    // How "safe" is it?

    if( IsBoundaryEdge( aDart ) )
        return false;

    // "angles" are at the diagonal
    DART_TYPE d1 = aDart;
    d1.Alpha2().Alpha1();
    DART_TYPE d2 = aDart;
    d2.Alpha1();

    if( aAllowDegeneracy )
    {
        if( TRAITS_TYPE::CrossProduct2D( d1, d2 ) < 0.0 )
            return false;
    }
    else
    {
        if( TRAITS_TYPE::CrossProduct2D( d1, d2 ) <= 0.0 )
            return false;
    }

    // Opposite side (still angle at the diagonal)
    d1 = aDart;
    d1.Alpha0();
    d2 = d1;
    d1.Alpha1();
    d2.Alpha2().Alpha1();

    if( aAllowDegeneracy )
    {
        if( TRAITS_TYPE::CrossProduct2D( d1, d2 ) < 0.0 )
            return false;
    }
    else
    {
        if( TRAITS_TYPE::CrossProduct2D( d1, d2 ) <= 0.0 )
            return false;
    }

    return true;
}

/** Given a \e dart, CCW or CW, positioned in a 0-orbit at the boundary of a tessellation.
 *   Position \e dart at a boundary edge in the same 0-orbit.\n
 *   If the given \e dart is CCW, \e dart is positioned at the left boundary edge
 *   and will be CW.\n
 *   If the given \e dart is CW, \e dart is positioned at the right boundary edge
 *   and will be CCW.
 *
 *   \note
 *   - The given \e dart must have a source node at the boundary, otherwise an
 *     infinit loop occurs.
 */
template <class DART_TYPE>
void TRIANGULATION_HELPER::PositionAtNextBoundaryEdge( DART_TYPE& aDart )
{
    DART_TYPE dart_prev;

    // If alpha2(d)=d, then boundary

    //old convention: dart.Alpha0();
    do
    {
        aDart.Alpha1();
        dart_prev = aDart;
        aDart.Alpha2();
    }
    while( aDart != dart_prev );
}

/** Checks if the boundary of a triangulation is convex.
 *
 *   \param dart
 *   A CCW dart at the boundary of the m_triangulation
 *
 *   \require
 *   - \ref hed::TTLtraits::CrossProduct2D "TRAITS_TYPE::CrossProduct2D" (const Dart&, const Dart&)
 */
template <class TRAITS_TYPE, class DART_TYPE>
bool TRIANGULATION_HELPER::ConvexBoundary( const DART_TYPE& aDart )
{
    std::list<DART_TYPE> blist;
    getBoundary( aDart, blist );

    int no;
    no = (int) blist.size();
    typename std::list<DART_TYPE>::const_iterator bit = blist.begin();
    DART_TYPE d1 = *bit;
    ++bit;
    DART_TYPE d2;
    bool convex = true;

    for( ; bit != blist.end(); ++bit )
    {
        d2 = *bit;
        double crossProd = TRAITS_TYPE::CrossProduct2D( d1, d2 );

        if( crossProd < 0.0 )
        {
            //cout << "!!! Boundary is NOT convex: crossProd = " << crossProd << endl;
            convex = false;
            return convex;
        }

        d1 = d2;
    }

    // Check the last angle
    d2 = *blist.begin();
    double crossProd = TRAITS_TYPE::CrossProduct2D( d1, d2 );

    if( crossProd < 0.0 )
    {
        //cout << "!!! Boundary is NOT convex: crossProd = " << crossProd << endl;
        convex = false;
    }

    //if (convex)
    //  cout << "\n---> Boundary is convex\n" << endl;
    //cout << endl;
    return convex;
}

//@} // End of Topological and Geometric Queries Group

/** @name Utilities for Delaunay Triangulation */
//@{
//------------------------------------------------------------------------------------------------
/** Optimizes the edges in the given sequence according to the
 *   \e Delaunay criterion, i.e., such that the edge will fullfill the
 *   \e circumcircle criterion (or equivalently the \e MaxMin
 *   angle criterion) with respect to the quadrilaterals where
 *   they are diagonals.
 *
 *   \param aElist
 *   The sequence of edges
 *
 *   \require
 *   - \ref hed::TTLtraits::swapEdge "TRAITS_TYPE::swapEdge" (DART_TYPE& \e dart)\n
 *     \b Note: Must be implemented such that \e dart is delivered back in a position as
 *     seen if it was glued to the edge when swapping (rotating) the edge CCW
 *
 *   \using
 *   - swapTestDelaunay
 */
template <class TRAITS_TYPE, class DART_TYPE, class DART_LIST_TYPE>
void TRIANGULATION_HELPER::OptimizeDelaunay( DART_LIST_TYPE& aElist )
{
    OptimizeDelaunay<TRAITS_TYPE, DART_TYPE, DART_LIST_TYPE>( aElist, aElist.end() );
}

//------------------------------------------------------------------------------------------------
template <class TRAITS_TYPE, class DART_TYPE, class DART_LIST_TYPE>
void TRIANGULATION_HELPER::OptimizeDelaunay( DART_LIST_TYPE& aElist,
                                             const typename DART_LIST_TYPE::iterator aEnd )
{
    // CCW darts
    // Optimize here means Delaunay, but could be any criterion by
    // requiring a "should swap" in the traits class, or give
    // a function object?
    // Assumes that elist has only one dart for each arc.
    // Darts outside the quadrilateral are preserved

    // For some data structures it is possible to preserve
    // all darts when swapping. Thus a preserve_darts_when swapping
    // ccould be given to indicate this and we would gain performance by avoiding
    // find in list.

    // Requires that swap retuns a dart in the "same position when rotated CCW"
    // (A vector instead of a list may be better.)

    // First check that elist is not empty
    if( aElist.empty() )
        return;

    // Avoid cycling by more extensive circumcircle test
    bool cycling_check = true;
    bool optimal = false;
    typename DART_LIST_TYPE::iterator it;

    typename DART_LIST_TYPE::iterator end_opt = aEnd;

    // Hmm... The following code is trying to derefence an iterator that may
    // be invalid. This may lead to debug error on Windows, so we comment out
    // this code. Checking elist.empty() above will prevent some
    // problems...
    //
    // last_opt is passed the end of the "active list"
    //typename DART_LIST_TYPE::iterator end_opt;
    //if (*end != NULL)
    //  end_opt = end;
    //else
    //  end_opt = elist.end();

    while( !optimal )
    {
        optimal = true;
        for( it = aElist.begin(); it != end_opt; ++it )
        {
            if( SwapTestDelaunay<TRAITS_TYPE>( *it, cycling_check ) )
            {
                // Preserve darts. Potential darts in the list are:
                // - The current dart
                // - the four CCW darts on the boundary of the quadrilateral
                // (the current arc has only one dart)

                SwapEdgeInList<TRAITS_TYPE, DART_TYPE>( it, aElist );

                optimal = false;
            } // end if should swap
        } // end for
    } // end pass
}

/** Checks if the edge associated with \e dart should be swapped according
 *   to the \e Delaunay criterion, i.e., the \e circumcircle criterion (or
 *   equivalently the \e MaxMin angle criterion).
 *
 *   \param aCyclingCheck
 *   Must be set to \c true when used in connection with optimization algorithms,
 *   e.g., OptimizeDelaunay. This will avoid cycling and infinite loops in nearly
 *   neutral cases.
 *
 *   \require
 *   - \ref hed::TTLtraits::ScalarProduct2D "TRAITS_TYPE::ScalarProduct2D" (DART_TYPE&, DART_TYPE&)
 *   - \ref hed::TTLtraits::CrossProduct2D "TRAITS_TYPE::CrossProduct2D" (DART_TYPE&, DART_TYPE&)
 */
template <class TRAITS_TYPE, class DART_TYPE>
#if ((_MSC_VER > 0) && (_MSC_VER < 1300))//#ifdef _MSC_VER
bool TRIANGULATION_HELPER::SwapTestDelaunay(const DART_TYPE& aDart, bool aCyclingCheck = false) const
{
#else
bool TRIANGULATION_HELPER::SwapTestDelaunay( const DART_TYPE& aDart, bool aCyclingCheck ) const
{
#endif
    // The general strategy is taken from Cline & Renka. They claim that
    // their algorithm insure numerical stability, but experiments show
    // that this is not correct for neutral, or almost neutral cases.
    // I have extended this strategy (without using tolerances) to avoid
    // cycling and infinit loops when used in connection with LOP algorithms;
    // see the comments below.

    typedef typename TRAITS_TYPE::REAL_TYPE REAL_TYPE;

    if( IsBoundaryEdge( aDart ) )
        return false;

    DART_TYPE v11 = aDart;
    v11.Alpha1().Alpha0();
    DART_TYPE v12 = v11;
    v12.Alpha1();

    DART_TYPE v22 = aDart;
    v22.Alpha2().Alpha1().Alpha0();
    DART_TYPE v21 = v22;
    v21.Alpha1();

    REAL_TYPE cos1 = TRAITS_TYPE::ScalarProduct2D( v11, v12 );
    REAL_TYPE cos2 = TRAITS_TYPE::ScalarProduct2D( v21, v22 );

    // "Angles" are opposite to the diagonal.
    // The diagonals should be swapped iff (t1+t2) .gt. 180
    // degrees. The following two tests insure numerical
    // stability according to Cline & Renka. But experiments show
    // that cycling may still happen; see the aditional test below.
    if( cos1 >= 0 && cos2 >= 0 ) // both angles are grater or equual 90
        return false;

    if( cos1 < 0 && cos2 < 0 ) // both angles are less than 90
        return true;

    REAL_TYPE sin1 = TRAITS_TYPE::CrossProduct2D( v11, v12 );
    REAL_TYPE sin2 = TRAITS_TYPE::CrossProduct2D( v21, v22 );
    REAL_TYPE sin12 = sin1 * cos2 + cos1 * sin2;

    if( sin12 >= 0 ) // equality represents a neutral case
        return false;

    if( aCyclingCheck )
    {
        // situation so far is sin12 < 0. Test if this also
        // happens for the swapped edge.

        // The numerical calculations so far indicate that the edge is
        // not Delaunay and should not be swapped. But experiments show that
        // in neutral cases, or almost neutral cases, it may happen that
        // the swapped edge may again be found to be not Delaunay and thus
        // be swapped if we return true here. This may lead to cycling and
        // an infinte loop when used, e.g., in connection with OptimizeDelaunay.
        //
        // In an attempt to avoid this we test if the swapped edge will
        // also be found to be not Delaunay by repeating the last test above
        // for the swapped edge.
        // We now rely on the general requirement for TRAITS_TYPE::swapEdge which
        // should deliver CCW dart back in "the same position"; see the general
        // description. This will insure numerical stability as the next calculation
        // is the same as if this function was called again with the swapped edge.
        // Cycling is thus impossible provided that the initial tests above does
        // not result in ambiguity (and they should probably not do so).

        v11.Alpha0();
        v12.Alpha0();
        v21.Alpha0();
        v22.Alpha0();
        // as if the edge was swapped/rotated CCW
        cos1 = TRAITS_TYPE::ScalarProduct2D( v22, v11 );
        cos2 = TRAITS_TYPE::ScalarProduct2D( v12, v21 );
        sin1 = TRAITS_TYPE::CrossProduct2D( v22, v11 );
        sin2 = TRAITS_TYPE::CrossProduct2D( v12, v21 );
        sin12 = sin1 * cos2 + cos1 * sin2;

        if( sin12 < 0 )
        {
            // A neutral case, but the tests above lead to swapping
            return false;
        }
    }

    return true;
}

//-----------------------------------------------------------------------
//
//        x
//"     /   \                                                           "
//     /  |  \      Darts:
//oe2 /   |   \     oe2 = oppEdge2
//   x....|....x
//    \  d|  d/     d   = diagonal (input and output)
//     \  |  /
//  oe1 \   /       oe1 = oppEdge1
//        x
//
//-----------------------------------------------------------------------
/** Recursively swaps edges in the triangulation according to the \e Delaunay criterion.
 *
 *   \param aDiagonal
 *   A CCW dart representing the edge where the recursion starts from.
 *
 *   \require
 *   - \ref hed::TTLtraits::swapEdge "TRAITS_TYPE::swapEdge" (DART_TYPE&)\n
 *     \b Note: Must be implemented such that the darts outside the quadrilateral
 *     are not affected by the swap.
 *
 *   \using
 *   - Calls itself recursively
 */
template <class TRAITS_TYPE, class DART_TYPE>
void TRIANGULATION_HELPER::RecSwapDelaunay( DART_TYPE& aDiagonal )
{
    if( !SwapTestDelaunay<TRAITS_TYPE>( aDiagonal ) )
        // ??? swapTestDelaunay also checks if boundary, so this can be optimized
        return;

    // Get the other "edges" of the current triangle; see illustration above.
    DART_TYPE oppEdge1 = aDiagonal;
    oppEdge1.Alpha1();
    bool b1;

    if( IsBoundaryEdge( oppEdge1 ) )
        b1 = true;
    else
    {
        b1 = false;
        oppEdge1.Alpha2();
    }

    DART_TYPE oppEdge2 = aDiagonal;
    oppEdge2.Alpha0().Alpha1().Alpha0();
    bool b2;

    if( IsBoundaryEdge( oppEdge2 ) )
        b2 = true;
    else
    {
        b2 = false;
        oppEdge2.Alpha2();
    }

    // Swap the given diagonal
    m_triangulation.swapEdge( aDiagonal );

    if( !b1 )
        RecSwapDelaunay<TRAITS_TYPE>( oppEdge1 );

    if( !b2 )
        RecSwapDelaunay<TRAITS_TYPE>( oppEdge2 );
}

/** Swaps edges away from the (interior) node associated with
 *   \e dart such that that exactly three edges remain incident
 *   with the node.
 *   This function is used as a first step in RemoveInteriorNode
 *
 *   \retval dart
 *   A CCW dart incident with the node
 *
 *   \par Assumes:
 *   - The node associated with \e dart is interior to the
 *     triangulation.
 *
 *   \require
 *   - \ref hed::TTLtraits::swapEdge "TRAITS_TYPE::swapEdge" (DART_TYPE& \e dart)\n
 *     \b Note: Must be implemented such that \e dart is delivered back in a position as
 *     seen if it was glued to the edge when swapping (rotating) the edge CCW
 *
 *   \note
 *   - A degenerate triangle may be left at the node.
 *   - The function is not unique as it depends on which dart
 *     at the node that is given as input.
 *
 *   \see
 *   SwapEdgesAwayFromBoundaryNode
 */
template <class TRAITS_TYPE, class DART_TYPE, class LIST_TYPE>
void TRIANGULATION_HELPER::SwapEdgesAwayFromInteriorNode( DART_TYPE& aDart,
                                                          LIST_TYPE& aSwappedEdges )
{

    // Same iteration as in fixEdgesAtCorner, but not boundary
    DART_TYPE dnext = aDart;

    // Allow degeneracy, otherwise we might end up with degree=4.
    // For example, the reverse operation of inserting a point on an
    // existing edge gives a situation where all edges are non-swappable.
    // Ideally, degeneracy in this case should be along the actual node,
    // but there is no strategy for this now.
    // ??? An alternative here is to wait with degeneracy till we get an
    // infinite loop with degree > 3.
    bool allowDegeneracy = true;

    int degree = getDegreeOfNode( aDart );
    DART_TYPE d_iter;

    while( degree > 3 )
    {
        d_iter = dnext;
        dnext.Alpha1().Alpha2();

        if( SwappableEdge<TRAITS_TYPE>( d_iter, allowDegeneracy ) )
        {
            m_triangulation.swapEdge( d_iter ); // swap the edge away
            // Collect swapped edges in the list
            // "Hide" the dart on the other side of the edge to avoid it being changed for
            // other swaps
            DART_TYPE swapped_edge = d_iter; // it was delivered back
            swapped_edge.Alpha2().Alpha0(); // CCW (if not at boundary)
            aSwappedEdges.push_back( swapped_edge );

            degree--;
        }
    }

    // Output, incident to the node
    aDart = dnext;
}

/** Swaps edges away from the (boundary) node associated with
 *   \e dart in such a way that when removing the edges that remain incident
 *   with the node, the boundary of the triangulation will be convex.
 *   This function is used as a first step in RemoveBoundaryNode
 *
 *   \retval dart
 *   A CCW dart incident with the node
 *
 *   \require
 *   - \ref hed::TTLtraits::swapEdge "TRAITS_TYPE::swapEdge" (DART_TYPE& \e dart)\n
 *     \b Note: Must be implemented such that \e dart is delivered back in a position as
 *     seen if it was glued to the edge when swapping (rotating) the edge CCW
 *
 *   \par Assumes:
 *   - The node associated with \e dart is at the boundary of the m_triangulation.
 *
 *   \see
 *   SwapEdgesAwayFromInteriorNode
 */
template <class TRAITS_TYPE, class DART_TYPE, class LIST_TYPE>
void TRIANGULATION_HELPER::SwapEdgesAwayFromBoundaryNode( DART_TYPE& aDart,
                                                          LIST_TYPE& aSwappedEdges )
{
    // All darts that are swappable.
    // To treat collinear nodes at an existing boundary, we must allow degeneracy
    // when swapping to the boundary.
    // dart is CCW and at the boundary.
    // The 0-orbit runs CCW
    // Deliver the dart back in the "same position".
    // Assume for the swap in the traits class:
    // - A dart on the swapped edge is delivered back in a position as
    //   seen if it was glued to the edge when swapping (rotating) the edge CCW

    //int degree = getDegreeOfNode(dart);

    passes:
    // Swap swappable edges that radiate from the node away
    DART_TYPE d_iter = aDart; // ???? can simply use dart
    d_iter.Alpha1().Alpha2(); // first not at boundary
    DART_TYPE d_next = d_iter;
    bool bend = false;
    bool swapped_next_to_boundary = false;
    bool swapped_in_pass = false;

    bool allowDegeneracy; // = true;
    DART_TYPE tmp1, tmp2;

    while( !bend )
    {
        d_next.Alpha1().Alpha2();

        if( IsBoundaryEdge( d_next ) )
            bend = true;  // then it is CW since alpha2

        // To allow removing among collinear nodes at the boundary,
        // degenerate triangles must be allowed
        // (they will be removed when used in connection with RemoveBoundaryNode)
        tmp1 = d_iter;
        tmp1.Alpha1();
        tmp2 = d_iter;
        tmp2.Alpha2().Alpha1(); // don't bother with boundary (checked later)

        if( IsBoundaryEdge( tmp1 ) && IsBoundaryEdge( tmp2 ) )
            allowDegeneracy = true;
        else
            allowDegeneracy = false;

        if( SwappableEdge<TRAITS_TYPE>( d_iter, allowDegeneracy ) )
        {
            m_triangulation.swapEdge( d_iter );

            // Collect swapped edges in the list
            // "Hide" the dart on the other side of the edge to avoid it being changed for
            // other swapps
            DART_TYPE swapped_edge = d_iter; // it was delivered back
            swapped_edge.Alpha2().Alpha0(); // CCW
            aSwappedEdges.push_back( swapped_edge );

            //degree--; // if degree is 2, or bend=true, we are done
            swapped_in_pass = true;
            if( bend )
                swapped_next_to_boundary = true;
        }

        if( !bend )
            d_iter = d_next;
    }

    // Deliver a dart as output in the same position as the incoming dart
    if( swapped_next_to_boundary )
    {
        // Assume that "swapping is CCW and dart is preserved in the same position
        d_iter.Alpha1().Alpha0().Alpha1();  // CW and see below
    }
    else
    {
        d_iter.Alpha1(); // CW and see below
    }
    PositionAtNextBoundaryEdge( d_iter ); // CCW

    aDart = d_iter; // for next pass or output

    // If a dart was swapped in this iteration we must run it more
    if( swapped_in_pass )
        goto passes;
}

/**  Swap the the edge associated with iterator \e it and update affected darts
 *   in \e elist accordingly.
 *   The darts affected by the swap are those in the same quadrilateral.
 *   Thus, if one want to preserve one or more of these darts on should
 *   keep them in \e elist.
 */
template <class TRAITS_TYPE, class DART_TYPE, class DART_LIST_TYPE>
void TRIANGULATION_HELPER::SwapEdgeInList( const typename DART_LIST_TYPE::iterator& aIt,
                                           DART_LIST_TYPE& aElist )
{

    typename DART_LIST_TYPE::iterator it1, it2, it3, it4;
    DART_TYPE dart( *aIt );

    //typename TRAITS_TYPE::DART_TYPE d1 = dart; d1.Alpha2().Alpha1();
    //typename TRAITS_TYPE::DART_TYPE d2 =   d1; d2.Alpha0().Alpha1();
    //typename TRAITS_TYPE::DART_TYPE d3 = dart; d3.Alpha0().Alpha1();
    //typename TRAITS_TYPE::DART_TYPE d4 =   d3; d4.Alpha0().Alpha1();
    DART_TYPE d1 = dart;
    d1.Alpha2().Alpha1();
    DART_TYPE d2 = d1;
    d2.Alpha0().Alpha1();
    DART_TYPE d3 = dart;
    d3.Alpha0().Alpha1();
    DART_TYPE d4 = d3;
    d4.Alpha0().Alpha1();

    // Find pinters to the darts that may change.
    // ??? Note, this is not very efficient since we must use find, which is O(N),
    // four times.
    // - Solution?: replace elist with a vector of pair (dart,number)
    //   and avoid find?
    // - make a function for swapping generically?
    // - sould we use another container type or,
    // - erase them and reinsert?
    // - or use two lists?
    it1 = find( aElist.begin(), aElist.end(), d1 );
    it2 = find( aElist.begin(), aElist.end(), d2 );
    it3 = find( aElist.begin(), aElist.end(), d3 );
    it4 = find( aElist.begin(), aElist.end(), d4 );

    m_triangulation.swapEdge( dart );
    // Update the current dart which may have changed
    *aIt = dart;

    // Update darts that may have changed again (if they were present)
    // Note that dart is delivered back after swapping
    if( it1 != aElist.end() )
    {
        d1 = dart;
        d1.Alpha1().Alpha0();
        *it1 = d1;
    }

    if( it2 != aElist.end() )
    {
        d2 = dart;
        d2.Alpha2().Alpha1();
        *it2 = d2;
    }

    if( it3 != aElist.end() )
    {
        d3 = dart;
        d3.Alpha2().Alpha1().Alpha0().Alpha1();
        *it3 = d3;
    }

    if( it4 != aElist.end() )
    {
        d4 = dart;
        d4.Alpha0().Alpha1();
        *it4 = d4;
    }
}

//@} // End of Utilities for Delaunay Triangulation Group

}
// End of ttl namespace scope (but other files may also contain functions for ttl)

#endif // _TTL_H_
