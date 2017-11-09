/*
 * Copyright (C) 1998, 2000-2007, 2010, 2011, 2012, 2013 SINTEF ICT,
 * Applied Mathematics, Norway.
 * Copyright (C) 2013 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef _HE_TRIANG_H_
#define _HE_TRIANG_H_

#define TTL_USE_NODE_ID   // Each node gets it's own unique id
#define TTL_USE_NODE_FLAG // Each node gets a flag (can be set to true or false)

#include <list>
#include <unordered_set>
#include <vector>
#include <iostream>
#include <fstream>
#include <ttl/ttl_util.h>
#include <memory>
#include <layers_id_colors_and_visibility.h>
#include <math/vector2d.h>

class BOARD_CONNECTED_ITEM;
class CN_CLUSTER;

namespace ttl
{
    class TRIANGULATION_HELPER;
}

/**
 * The half-edge data structure
 */
namespace hed
{
// Helper typedefs
class NODE;
class EDGE;
typedef std::shared_ptr<NODE> NODE_PTR;
typedef std::shared_ptr<EDGE> EDGE_PTR;
typedef std::weak_ptr<EDGE> EDGE_WEAK_PTR;
typedef std::vector<NODE_PTR> NODES_CONTAINER;

/**
 * \class NODE
 * \brief \b Node class for data structures (Inherits from HandleId)
 *
 * \note
 * - To enable node IDs, TTL_USE_NODE_ID must be defined.
 * - To enable node flags, TTL_USE_NODE_FLAG must be defined.
 * - TTL_USE_NODE_ID and TTL_USE_NODE_FLAG should only be enabled if this functionality is
 *   required by the application, because they increase the memory usage for each Node object.
 */
class NODE
{
protected:
#ifdef TTL_USE_NODE_FLAG
    /// TTL_USE_NODE_FLAG must be defined
    bool m_flag;
#endif

#ifdef TTL_USE_NODE_ID
    /// TTL_USE_NODE_ID must be defined
    static int id_count;

    /// A unique id for each node (TTL_USE_NODE_ID must be defined)
    int m_id;
#endif

    /// Node coordinates
    const int m_x, m_y;

public:
    /// Constructor
    NODE( int aX = 0, int aY = 0, std::shared_ptr<CN_CLUSTER> aCluster = nullptr ) :
#ifdef TTL_USE_NODE_FLAG
        m_flag( false ),
#endif
#ifdef TTL_USE_NODE_ID
        m_id( id_count++ ),
#endif
        m_x( aX ), m_y( aY )
    {
    }

    /// Destructor
    ~NODE() {

    }

    const VECTOR2D Pos() const { return VECTOR2D( m_x, m_y ); }

    /// Returns the x-coordinate
    inline int GetX() const
    {
        return m_x;
    }

    /// Returns the y-coordinate
    inline int GetY() const
    {
        return m_y;
    }

    inline VECTOR2I GetPos() const
    {
        return VECTOR2I( m_x, m_y );
    }

#ifdef TTL_USE_NODE_ID
    /// Returns the id (TTL_USE_NODE_ID must be defined)

    inline void SetId( int aId )
    {
        m_id = aId;
    }

    inline int Id() const
    {
        return m_id;
    }
#endif

#ifdef TTL_USE_NODE_FLAG
    /// Sets the flag (TTL_USE_NODE_FLAG must be defined)
    inline void SetFlag( bool aFlag )
    {
        m_flag = aFlag;
    }

    /// Returns the flag (TTL_USE_NODE_FLAG must be defined)
    inline const bool& GetFlag() const
    {
        return m_flag;
    }
#endif
};


/**
 * \class EDGE
 * \brief \b %Edge class in the in the half-edge data structure.
 */
class EDGE
{
public:
    /// Constructor
    EDGE() : m_isLeadingEdge( false )
    {
    }

    /// Destructor
    virtual ~EDGE()
    {

    }

    /// Sets the source node
    inline void SetSourceNode( const NODE_PTR& aNode )
    {
        m_sourceNode = aNode;
    }

    /// Sets the next edge in face
    inline void SetNextEdgeInFace( const EDGE_PTR& aEdge )
    {
        m_nextEdgeInFace = aEdge;
    }

    /// Sets the twin edge
    inline void SetTwinEdge( const EDGE_PTR& aEdge )
    {
        m_twinEdge = aEdge;
    }

    /// Sets the edge as a leading edge
    inline void SetAsLeadingEdge( bool aLeading = true )
    {
        m_isLeadingEdge = aLeading;
    }

    /// Checks if an edge is a leading edge
    inline bool IsLeadingEdge() const
    {
        return m_isLeadingEdge;
    }

    /// Returns the twin edge
    inline EDGE_PTR GetTwinEdge() const
    {
        if( m_twinEdge.expired() )
            return nullptr;

        return m_twinEdge.lock();
    }

    inline void ClearTwinEdge()
    {
        m_twinEdge.reset();
    }

    /// Returns the next edge in face
    inline const EDGE_PTR& GetNextEdgeInFace() const
    {
        assert ( m_nextEdgeInFace );
        return m_nextEdgeInFace;
    }

    /// Retuns the source node
    inline const NODE_PTR& GetSourceNode() const
    {
        return m_sourceNode;
    }

    /// Returns the target node
    virtual const NODE_PTR& GetTargetNode() const
    {
        return m_nextEdgeInFace->GetSourceNode();
    }

    void Clear()
    {
        m_sourceNode.reset();
        m_nextEdgeInFace.reset();

        if( !m_twinEdge.expired() )
        {
            m_twinEdge.lock()->ClearTwinEdge();
            m_twinEdge.reset();
        }
    }

protected:
    NODE_PTR        m_sourceNode;
    EDGE_WEAK_PTR   m_twinEdge;
    EDGE_PTR        m_nextEdgeInFace;
    bool            m_isLeadingEdge;
};

class DART; // Forward declaration (class in this namespace)

/**
 * \class TRIANGULATION
 * \brief \b %Triangulation class for the half-edge data structure with adaption to TTL.
 */
class TRIANGULATION
{
protected:
    /// One half-edge for each arc
    std::list<EDGE_PTR> m_leadingEdges;

    ttl::TRIANGULATION_HELPER* m_helper;

    void addLeadingEdge( EDGE_PTR& aEdge )
    {
        aEdge->SetAsLeadingEdge();
        m_leadingEdges.push_front( aEdge );
    }

    bool removeLeadingEdgeFromList( EDGE_PTR& aLeadingEdge );

    void cleanAll();

    /** Swaps the edge associated with \e dart in the actual data structure.
     *
     *   <center>
     *   \image html swapEdge.gif
     *   </center>
     *
     *   \param aDart
     *   Some of the functions require a dart as output.
     *   If this is required by the actual function, the dart should be delivered
     *   back in a position as seen if it was glued to the edge when swapping (rotating)
     *   the edge CCW; see the figure.
     *
     *   \note
     *   - If the edge is \e constrained, or if it should not be swapped for
     *     some other reason, this function need not do the actual swap of the edge.
     *   - Some functions in TTL require that \c swapEdge is implemented such that
     *     darts outside the quadrilateral are not affected by the swap.
     */
    void swapEdge( DART& aDart );

    /**
     * Splits the triangle associated with \e dart in the actual data structure into
     * three new triangles joining at \e point.
     *
     * <center>
     * \image html splitTriangle.gif
     * </center>
     *
     * \param aDart
     * Output: A CCW dart incident with the new node; see the figure.
     */
    void splitTriangle( DART& aDart, const NODE_PTR& aPoint );

    /**
     * The reverse operation of TTLtraits::splitTriangle.
     * This function is only required for functions that involve
     * removal of interior nodes; see for example TrinagulationHelper::RemoveInteriorNode.
     *
     * <center>
     * \image html reverse_splitTriangle.gif
     * </center>
     */
    void reverseSplitTriangle( DART& aDart );

    /**
     * Removes a triangle with an edge at the boundary of the triangulation
     * in the actual data structure
     */
    void removeBoundaryTriangle( DART& aDart );

public:
    /// Default constructor
    TRIANGULATION();

    /// Copy constructor
    TRIANGULATION( const TRIANGULATION& aTriangulation );

    /// Destructor
    ~TRIANGULATION();

    /// Creates a Delaunay triangulation from a set of points
    void CreateDelaunay( NODES_CONTAINER::iterator aFirst, NODES_CONTAINER::iterator aLast );

    /// Creates an initial Delaunay triangulation from two enclosing triangles
    //  When using rectangular boundary - loop through all points and expand.
    //  (Called from createDelaunay(...) when starting)
    EDGE_PTR InitTwoEnclosingTriangles( NODES_CONTAINER::iterator aFirst,
                                        NODES_CONTAINER::iterator aLast );

    // These two functions are required by TTL for Delaunay triangulation

    /// Swaps the edge associated with diagonal
    void SwapEdge( EDGE_PTR& aDiagonal );

    /// Splits the triangle associated with edge into three new triangles joining at point
    EDGE_PTR SplitTriangle( EDGE_PTR& aEdge, const NODE_PTR& aPoint );

    // Functions required by TTL for removing nodes in a Delaunay triangulation

    /// Removes the boundary triangle associated with edge
    void RemoveTriangle( EDGE_PTR& aEdge ); // boundary triangle required

    /// The reverse operation of removeTriangle
    void ReverseSplitTriangle( EDGE_PTR& aEdge );

    /// Creates an arbitrary CCW dart
    DART CreateDart();

    /// Returns a list of "triangles" (one leading half-edge for each triangle)
    const std::list<EDGE_PTR>& GetLeadingEdges() const
    {
        return m_leadingEdges;
    }

    /// Returns the number of triangles
    int NoTriangles() const
    {
        return (int) m_leadingEdges.size();
    }

    /// Returns a list of half-edges (one half-edge for each arc)
    void GetEdges( std::list<EDGE_PTR>& aEdges, bool aSkipBoundaryEdges = false ) const;

#ifdef TTL_USE_NODE_FLAG
    /// Sets flag in all the nodes
    void FlagNodes( bool aFlag ) const;

    /// Returns a list of nodes. This function requires TTL_USE_NODE_FLAG to be defined. \see Node.
    std::list<NODE_PTR>* GetNodes() const;
#endif

    /// Swaps edges until the triangulation is Delaunay (constrained edges are not swapped)
    void OptimizeDelaunay();

    /// Checks if the triangulation is Delaunay
    bool CheckDelaunay() const;

    /// Returns an arbitrary interior node (as the source node of the returned edge)
    EDGE_PTR GetInteriorNode() const;

    EDGE_PTR GetBoundaryEdgeInTriangle( const EDGE_PTR& aEdge ) const;

    /// Returns an arbitrary boundary edge
    EDGE_PTR GetBoundaryEdge() const;

    /// Print edges for plotting with, e.g., gnuplot
    void PrintEdges( std::ofstream& aOutput ) const;

    friend class ttl::TRIANGULATION_HELPER;
};
} // End of hed namespace

#endif
