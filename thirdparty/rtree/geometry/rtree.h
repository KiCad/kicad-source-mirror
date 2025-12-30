//TITLE
//
//    R-TREES: A DYNAMIC INDEX STRUCTURE FOR SPATIAL SEARCHING
//
//DESCRIPTION
//
//    A C++ templated version of the RTree algorithm.
//    For more information please read the comments in RTree.h
//
//AUTHORS
//
//    * 1983 Original algorithm and test code by Antonin Guttman and Michael Stonebraker, UC Berkely
//    * 1994 ANCI C ported from original test code by Melinda Green - melinda@superliminal.com
//    * 1995 Sphere volume fix for degeneracy problem submitted by Paul Brook
//    * 2004 Templated C++ port by Greg Douglas
//    * 2013 CERN (www.cern.ch)
//    * 2020 KiCad Developers - Add std::iterator support for searching
//    * 2020 KiCad Developers - Add container nearest neighbor based on Hjaltason & Samet
//    * 2022 KiCad Developers - Slight optimizations in RectSphericalVolume
//

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2013 CERN
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef RTREE_H
#define RTREE_H

// NOTE This file compiles under MSVC 6 SP5 and MSVC .Net 2003 it may not work on other compilers without modification.

// NOTE These next few lines may be win32 specific, you may need to modify them to compile on other platform
#include <cassert>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include <algorithm>
#include <array>
#include <functional>
#include <iterator>
#include <limits>
#include <queue>
#include <vector>

#ifdef DEBUG
#define ASSERT assert    // RTree uses ASSERT( condition )
#else
#define ASSERT( _x )
#endif

//
// RTree.h
//

#define RTREE_TEMPLATE          template <class DATATYPE, class ELEMTYPE, int NUMDIMS, \
    class ELEMTYPEREAL, int TMAXNODES, int TMINNODES>
#define RTREE_SEARCH_TEMPLATE   template <class DATATYPE, class ELEMTYPE, int NUMDIMS, \
    class ELEMTYPEREAL, int TMAXNODES, int TMINNODES, class VISITOR>
#define RTREE_QUAL              RTree<DATATYPE, ELEMTYPE, NUMDIMS, ELEMTYPEREAL, TMAXNODES, \
    TMINNODES>
#define RTREE_SEARCH_QUAL       RTree<DATATYPE, ELEMTYPE, NUMDIMS, ELEMTYPEREAL, TMAXNODES, \
    TMINNODES, VISITOR>

#define RTREE_DONT_USE_MEMPOOLS     // This version does not contain a fixed memory allocator, fill in lines with EXAMPLE to implement one.
#define RTREE_USE_SPHERICAL_VOLUME  // Better split classification, may be slower on some systems

// Fwd decl
class RTFileStream;    // File I/O helper class, look below for implementation and notes.


/// \class RTree
/// Implementation of RTree, a multidimensional bounding rectangle tree.
/// Example usage: For a 3-dimensional tree use RTree<Object*, float, 3> myTree;
///
/// This modified, templated C++ version by Greg Douglas at Auran (http://www.auran.com)
///
/// DATATYPE Referenced data, should be int, void*, obj* etc. no larger than sizeof<void*> and simple type
/// ELEMTYPE Type of element such as int or float
/// NUMDIMS Number of dimensions such as 2 or 3
/// ELEMTYPEREAL Type of element that allows fractional and large values such as float or double, for use in volume calcs
///
/// NOTES: Inserting and removing data requires the knowledge of its constant Minimal Bounding Rectangle.
///        This version uses new/delete for nodes, I recommend using a fixed size allocator for efficiency.
///        Instead of using a callback function for returned results, I recommend and efficient pre-sized, grow-only memory
///        array similar to MFC CArray or STL Vector for returning search query result.
///
template <class DATATYPE, class ELEMTYPE, int NUMDIMS,
          class ELEMTYPEREAL = ELEMTYPE, int TMAXNODES = 8, int TMINNODES = TMAXNODES / 2>
class RTree
{
protected:

    struct Node; // Fwd decl.  Used by other internal structs and iterator

public:
    /// Minimal bounding rectangle (n-dimensional)
    struct Rect
    {
        ELEMTYPE m_min[NUMDIMS]; ///< Min dimensions of bounding box
        ELEMTYPE m_max[NUMDIMS]; ///< Max dimensions of bounding box
    };

    // These constant must be declared after Branch and before Node struct
    // Stuck up here for MSVC 6 compiler.  NSVC .NET 2003 is much happier.
    enum {
        MAXNODES = TMAXNODES,                       ///< Max elements in node
        MINNODES = TMINNODES                        ///< Min elements in node
    };

    struct Statistics {
        int maxDepth;
        int avgDepth;

        int maxNodeLoad;
        int avgNodeLoad;
        int totalItems;
    };

public:

    RTree();
    virtual ~RTree();

    /// Insert entry
    /// \param a_min Min of bounding rect
    /// \param a_max Max of bounding rect
    /// \param a_dataId Positive Id of data.  Maybe zero, but negative numbers not allowed.
    void Insert( const ELEMTYPE     a_min[NUMDIMS],
                 const ELEMTYPE     a_max[NUMDIMS],
                 const DATATYPE&    a_dataId );

    /// Remove entry
    /// \param a_min Min of bounding rect
    /// \param a_max Max of bounding rect
    /// \param a_dataId Positive Id of data.  Maybe zero, but negative numbers not allowed.
    /// \return  1 if record not found, 0 if success.
    bool Remove( const ELEMTYPE     a_min[NUMDIMS],
                 const ELEMTYPE     a_max[NUMDIMS],
                 const DATATYPE&    a_dataId );

    /// Find all within search rectangle
    /// \param a_min Min of search bounding rect
    /// \param a_max Max of search bounding rect
    /// \param a_callback Callback function to return result.  Callback should return 'true' to continue searching
    /// \return Returns the number of entries found
    int Search( const ELEMTYPE a_min[NUMDIMS],
                const ELEMTYPE a_max[NUMDIMS],
                std::function<bool (const DATATYPE&)> a_callback ) const;

    /// Find all within search rectangle
    /// \param a_min Min of search bounding rect
    /// \param a_max Max of search bounding rect
    /// \param a_callback Callback function to return result.  Callback should return 'true' to continue searching
    /// \param aFinished This is set to true if the search completed and false if it was interupted
    /// \return Returns the number of entries found
    int Search( const ELEMTYPE a_min[NUMDIMS], const ELEMTYPE a_max[NUMDIMS],
            std::function<bool( const DATATYPE& )> a_callback, bool& aFinished ) const;

    template <class VISITOR>
    int Search( const ELEMTYPE a_min[NUMDIMS], const ELEMTYPE a_max[NUMDIMS], VISITOR& a_visitor ) const
    {
  #ifdef _DEBUG

        for( int index = 0; index < NUMDIMS; ++index )
        {
            ASSERT( a_min[index] <= a_max[index] );
        }

  #endif    // _DEBUG

        Rect rect;

        for( int axis = 0; axis < NUMDIMS; ++axis )
        {
            rect.m_min[axis] = a_min[axis];
            rect.m_max[axis] = a_max[axis];
        }


        // NOTE: May want to return search result another way, perhaps returning the number of found elements here.
        int cnt = 0;

        Search( m_root, &rect, a_visitor, cnt );

        return cnt;
    }

    /// Calculate Statistics

    Statistics CalcStats();

    /// Remove all entries from tree
    void    RemoveAll();

    /// Count the data elements in this container.  This is slow as no internal counter is maintained.
    int     Count() const;

    /// Load tree contents from file
    bool    Load( const char* a_fileName );

    /// Load tree contents from stream
    bool    Load( RTFileStream& a_stream ) const;


    /// Save tree contents to file
    bool    Save( const char* a_fileName );

    /// Save tree contents to stream
    bool    Save( RTFileStream& a_stream ) const;

    /**
     * Gets an ordered vector of the nearest data elements to a specified point
     * @param aPoint coordinate to measure against
     * @param aTerminate Callback routine to check when we have gathered sufficient elements
     * @param aFilter Callback routine to remove specific elements from the query results
     * @param aSquaredDist Callback routine to measure the distance from the point to the data element
     * @return vector of matching elements and their distance to the point
     */
    std::vector<std::pair<ELEMTYPE, DATATYPE>> NearestNeighbors(
            const ELEMTYPE aPoint[NUMDIMS],
            std::function<bool( const std::size_t aNumResults, const ELEMTYPE aMinDist )> aTerminate,
            std::function<bool( const DATATYPE aElement )> aFilter,
            std::function<ELEMTYPE( const ELEMTYPE a_point[NUMDIMS], const DATATYPE a_data )> aSquaredDist ) const;

public:
    /// Iterator is not remove safe.
    class Iterator
    {
    private:
        enum
        {
            MAX_STACK = 32
        }; // Max stack size. Allows almost n^32 where n is number of branches in node

        struct StackElement
        {
            Node* m_node;
            int   m_branchIndex;
        };

    public:
        typedef std::forward_iterator_tag iterator_category;
        typedef DATATYPE                  value_type;
        typedef ptrdiff_t                 difference_type;
        typedef DATATYPE*                 pointer;
        typedef DATATYPE&                 reference;

    public:
        Iterator() : m_stack( {} ), m_tos( 0 )
        {
            for( int i = 0; i < NUMDIMS; ++i )
            {
                m_rect.m_min[i] = std::numeric_limits<ELEMTYPE>::min();
                m_rect.m_max[i] = std::numeric_limits<ELEMTYPE>::max();
            }
        }

        Iterator( const Rect& aRect ) : m_stack( {} ), m_tos( 0 ), m_rect( aRect )
        {
        }

        ~Iterator()
        {
        }

        /// Is iterator pointing to valid data
        constexpr bool IsNotNull() const
        {
            return m_tos > 0;
        }

        /// Access the current data element. Caller must be sure iterator is not NULL first.
        DATATYPE& operator*()
        {
            ASSERT( IsNotNull() );
            StackElement& curTos = m_stack[m_tos - 1];
            return curTos.m_node->m_branch[curTos.m_branchIndex].m_data;
        }

        /// Access the current data element. Caller must be sure iterator is not NULL first.
        const DATATYPE& operator*() const
        {
            ASSERT( IsNotNull() );
            StackElement& curTos = m_stack[m_tos - 1];
            return curTos.m_node->m_branch[curTos.m_branchIndex].m_data;
        }

        DATATYPE* operator->()
        {
            ASSERT( IsNotNull() );
            StackElement& curTos = m_stack[m_tos - 1];
            return &( curTos.m_node->m_branch[curTos.m_branchIndex].m_data );
        }

        /// Prefix ++ operator
        Iterator& operator++()
        {
            FindNextData();
            return *this;
        }

        /// Postfix ++ operator
        Iterator operator++( int )
        {
            Iterator retval = *this;
            FindNextData();
            return retval;
        }

        bool operator==( const Iterator& rhs ) const
        {
            return ( ( m_tos <= 0 && rhs.m_tos <= 0 )
                     || ( m_tos == rhs.m_tos && m_stack[m_tos].m_node == rhs.m_stack[m_tos].m_node
                                && m_stack[m_tos].m_branchIndex
                                           == rhs.m_stack[m_tos].m_branchIndex ) );
        }

        bool operator!=( const Iterator& rhs ) const
        {
            return ( ( m_tos > 0 || rhs.m_tos > 0 )
                     && ( m_tos != rhs.m_tos || m_stack[m_tos].m_node != rhs.m_stack[m_tos].m_node
                                || m_stack[m_tos].m_branchIndex
                                           != rhs.m_stack[m_tos].m_branchIndex ) );
        }

    private:
        /// Find the next data element in the tree (For internal use only)
        void FindNextData()
        {
            while( m_tos > 0 )
            {
                StackElement curTos     = Pop();
                int          nextBranch = curTos.m_branchIndex + 1;

                if( curTos.m_node->IsLeaf() )
                {
                    // Keep walking through siblings until we find an overlapping leaf
                    for( int i = nextBranch; i < curTos.m_node->m_count; i++ )
                    {
                        if( RTree::Overlap( &m_rect, &curTos.m_node->m_branch[i].m_rect ) )
                        {
                            Push( curTos.m_node, i );
                            return;
                        }
                    }
                    // No more data, so it will fall back to previous level
                }
                else
                {
                    // Look for an overlapping sibling that we can use as the fall-back node
                    // when we've iterated down the current branch
                    for( int i = nextBranch; i < curTos.m_node->m_count; i++ )
                    {
                        if( RTree::Overlap( &m_rect, &curTos.m_node->m_branch[i].m_rect ) )
                        {
                            Push( curTos.m_node, i );
                            break;
                        }
                    }

                    Node* nextLevelnode = curTos.m_node->m_branch[curTos.m_branchIndex].m_child;

                    // Since cur node is not a leaf, push first of next level,
                    // zero-th branch to get deeper into the tree
                    Push( nextLevelnode, 0 );

                    // If the branch is a leaf, and it overlaps, then break with the current data
                    // Otherwise, we allow it to seed our next iteration as it may have siblings that
                    // do overlap
                    if( nextLevelnode->IsLeaf()
                            && RTree::Overlap( &m_rect, &nextLevelnode->m_branch[0].m_rect ) )
                        return;
                }
            }
        }

        /// Push node and branch onto iteration stack (For internal use only)
        void Push( Node* a_node, int a_branchIndex )
        {
            m_stack[m_tos].m_node = a_node;
            m_stack[m_tos].m_branchIndex = a_branchIndex;
            ++m_tos;
            ASSERT( m_tos <= MAX_STACK );
        }

        /// Pop element off iteration stack (For internal use only)
        StackElement& Pop()
        {
            ASSERT( m_tos > 0 );
            --m_tos;
            return m_stack[m_tos];
        }

        std::array<StackElement, MAX_STACK> m_stack; ///< Stack for iteration
        int                                 m_tos;   ///< Top Of Stack index
        Rect                                m_rect;  ///< Search rectangle

        friend class RTree;
        // Allow hiding of non-public functions while allowing manipulation by logical owner
    };

    using iterator       = Iterator;
    using const_iterator = const Iterator;

    iterator begin( const Rect& aRect ) const
    {
        iterator retval( aRect );

        if( !m_root->m_count )
            return retval;

        retval.Push( m_root, 0 );

        // If the first leaf matches, return the root pointer, otherwise,
        // increment to the first match or empty if none.
        if( m_root->IsLeaf() && Overlap( &aRect, &m_root->m_branch[0].m_rect ) )
            return retval;

        ++retval;
        return retval;
    }

    iterator begin() const
    {
        Rect full_rect;

        std::fill_n( full_rect.m_min, NUMDIMS, INT_MIN );
        std::fill_n( full_rect.m_max, NUMDIMS, INT_MAX );

        return begin( full_rect );
    }

    iterator end() const
    {
        iterator retval;
        return retval;
    }

    iterator end( const Rect& aRect ) const
    {
        return end();
    }


protected:
    /// May be data or may be another subtree
    /// The parents level determines this.
    /// If the parents level is 0, then this is data
    struct Branch
    {
        Rect m_rect;                              ///< Bounds
        union
        {
            Node*       m_child;                    ///< Child node
            DATATYPE    m_data;                     ///< Data Id or Ptr
        };
    };

    /// Node for each branch level
    struct Node
    {
        constexpr bool IsInternalNode() const { return m_level > 0; }   // Not a leaf, but a internal node
        constexpr bool IsLeaf()         const { return m_level == 0; }  // A leaf, contains data

        int     m_count;                            ///< Count
        int     m_level;                            ///< Leaf is zero, others positive
        Branch  m_branch[MAXNODES];                 ///< Branch
    };

    /// A link list of nodes for reinsertion after a delete operation
    struct ListNode
    {
        ListNode*   m_next;                         ///< Next in list
        Node*       m_node;                         ///< Node
    };

    /// Variables for finding a split partition
    struct PartitionVars
    {
        int             m_partition[MAXNODES + 1];
        int             m_total;
        int             m_minFill;
        bool            m_taken[MAXNODES + 1];
        int             m_count[2];
        Rect            m_cover[2];
        ELEMTYPEREAL    m_area[2];

        Branch          m_branchBuf[MAXNODES + 1];
        int             m_branchCount;
        Rect            m_coverSplit;
        ELEMTYPEREAL    m_coverSplitArea;
    };

    /// Data structure used for Nearest Neighbor search implementation
    struct NNNode
    {
        Branch m_branch;
        ELEMTYPE minDist;
        bool isLeaf;

        inline bool operator<(const NNNode &other) const
        {
            /// This is reversed on purpose to use std::priority_queue
            return other.minDist < minDist;
        }
    };

    Node*           AllocNode() const;
    void            FreeNode( Node* a_node ) const;
    void            InitNode( Node* a_node ) const;
    void            InitRect( Rect* a_rect ) const;
    bool            InsertRectRec( const Rect*      a_rect,
                                   const DATATYPE&  a_id,
                                   Node*            a_node,
                                   Node**           a_newNode,
                                   int              a_level ) const;
    bool            InsertRect( const Rect* a_rect, const DATATYPE& a_id, Node** a_root, int a_level ) const;
    Rect            NodeCover( Node* a_node ) const;
    bool            AddBranch( const Branch* a_branch, Node* a_node, Node** a_newNode ) const;
    void            DisconnectBranch( Node* a_node, int a_index ) const;
    int             PickBranch( const Rect* a_rect, Node* a_node ) const;
    Rect            CombineRect( const Rect* a_rectA, const Rect* a_rectB ) const;
    void            SplitNode( Node* a_node, const Branch* a_branch, Node** a_newNode ) const;
    ELEMTYPEREAL    RectSphericalVolume( const Rect* a_rect ) const;
    ELEMTYPEREAL    RectVolume( const Rect* a_rect ) const;
    ELEMTYPEREAL    CalcRectVolume( const Rect* a_rect ) const;
    void            GetBranches( Node* a_node, const Branch* a_branch, PartitionVars* a_parVars ) const;
    void            ChoosePartition( PartitionVars* a_parVars, int a_minFill ) const;
    void            LoadNodes( Node* a_nodeA, Node* a_nodeB, PartitionVars* a_parVars ) const;
    void            InitParVars( PartitionVars* a_parVars, int a_maxRects, int a_minFill ) const;
    void            PickSeeds( PartitionVars* a_parVars ) const;
    void            Classify( int a_index, int a_group, PartitionVars* a_parVars ) const;
    bool            RemoveRect( const Rect* a_rect, const DATATYPE& a_id, Node** a_root ) const;
    bool            RemoveRectRec( const Rect*      a_rect,
                                   const DATATYPE&  a_id,
                                   Node*            a_node,
                                   ListNode**       a_listNode ) const;
    ListNode*       AllocListNode() const;
    void            FreeListNode( ListNode* a_listNode ) const;
    static bool     Overlap( const Rect* a_rectA, const Rect* a_rectB );
    void            ReInsert( Node* a_node, ListNode** a_listNode ) const;
    ELEMTYPE        MinDist( const ELEMTYPE a_point[NUMDIMS], const Rect& a_rect ) const;

    bool Search( const Node* a_node, const Rect* a_rect, int& a_foundCount,
                 std::function<bool (const DATATYPE&)> a_callback ) const;

    template <class VISITOR>
    bool Search( const Node* a_node, const Rect* a_rect, VISITOR& a_visitor, int& a_foundCount ) const
    {
        ASSERT( a_node );
        ASSERT( a_node->m_level >= 0 );
        ASSERT( a_rect );

        if( a_node->IsInternalNode() ) // This is an internal node in the tree
        {
            for( int index = 0; index < a_node->m_count; ++index )
            {
                if( Overlap( a_rect, &a_node->m_branch[index].m_rect ) )
                {
                    if( !Search( a_node->m_branch[index].m_child, a_rect, a_visitor, a_foundCount ) )
                    {
                        return false; // Don't continue searching
                    }
                }
            }
        }
        else // This is a leaf node
        {
            for( int index = 0; index < a_node->m_count; ++index )
            {
                if( Overlap( a_rect, &a_node->m_branch[index].m_rect ) )
                {
                    const DATATYPE& id = a_node->m_branch[index].m_data;

                    if( !a_visitor( id ) )
                        return false;

                    a_foundCount++;
                }
            }
        }

        return true; // Continue searching
    }

    void    RemoveAllRec( Node* a_node ) const;
    void    Reset() const;
    void    CountRec( const Node* a_node, int& a_count ) const;

    bool    SaveRec( const Node* a_node, RTFileStream& a_stream ) const;
    bool    LoadRec( const Node* a_node, RTFileStream& a_stream ) const;

    Node*           m_root;                         ///< Root of tree
    ELEMTYPEREAL    m_unitSphereVolume;             ///< Unit sphere constant for required number of dimensions
};


// Because there is not stream support, this is a quick and dirty file I/O helper.
// Users will likely replace its usage with a Stream implementation from their favorite API.
class RTFileStream
{
    FILE* m_file;
public:


    RTFileStream()
    {
        m_file = NULL;
    }

    ~RTFileStream()
    {
        Close();
    }

    bool OpenRead( const char* a_fileName )
    {
        m_file = std::fopen( a_fileName, "rb" );

        if( !m_file )
        {
            return false;
        }

        return true;
    }

    bool OpenWrite( const char* a_fileName )
    {
        m_file = std::fopen( a_fileName, "wb" );

        if( !m_file )
        {
            return false;
        }

        return true;
    }

    void Close()
    {
        if( m_file )
        {
            std::fclose( m_file );
            m_file = NULL;
        }
    }

    template <typename TYPE>
    size_t Write( const TYPE& a_value )
    {
        ASSERT( m_file );
        return std::fwrite( (void*) &a_value, sizeof(a_value), 1, m_file );
    }

    template <typename TYPE>
    size_t WriteArray( const TYPE* a_array, int a_count )
    {
        ASSERT( m_file );
        return std::fwrite( (void*) a_array, sizeof(TYPE) * a_count, 1, m_file );
    }

    template <typename TYPE>
    size_t Read( TYPE& a_value )
    {
        ASSERT( m_file );
        return std::fread( (void*) &a_value, sizeof(a_value), 1, m_file );
    }

    template <typename TYPE>
    size_t ReadArray( TYPE* a_array, int a_count )
    {
        ASSERT( m_file );
        return std::fread( (void*) a_array, sizeof(TYPE) * a_count, 1, m_file );
    }
};


RTREE_TEMPLATE RTREE_QUAL::RTree()
{
    ASSERT( MAXNODES > MINNODES );
    ASSERT( MINNODES > 0 );


    // We only support machine word size simple data type eg. integer index or object pointer.
    // Since we are storing as union with non data branch
    ASSERT( sizeof(DATATYPE) == sizeof(void*) || sizeof(DATATYPE) == sizeof(int) );

    // Precomputed volumes of the unit spheres for the first few dimensions
    const float UNIT_SPHERE_VOLUMES[] =
    {
        0.000000f, 2.000000f, 3.141593f,    // Dimension  0,1,2
        4.188790f, 4.934802f, 5.263789f,    // Dimension  3,4,5
        5.167713f, 4.724766f, 4.058712f,    // Dimension  6,7,8
        3.298509f, 2.550164f, 1.884104f,    // Dimension  9,10,11
        1.335263f, 0.910629f, 0.599265f,    // Dimension  12,13,14
        0.381443f, 0.235331f, 0.140981f,    // Dimension  15,16,17
        0.082146f, 0.046622f, 0.025807f,    // Dimension  18,19,20
    };

    m_root = AllocNode();
    m_root->m_level     = 0;
    m_unitSphereVolume  = (ELEMTYPEREAL) UNIT_SPHERE_VOLUMES[NUMDIMS];
}


RTREE_TEMPLATE
RTREE_QUAL::~RTree() {
    Reset(); // Free, or reset node memory
}


RTREE_TEMPLATE
void RTREE_QUAL::Insert( const ELEMTYPE     a_min[NUMDIMS],
                         const ELEMTYPE     a_max[NUMDIMS],
                         const DATATYPE&    a_dataId )
{
#ifdef _DEBUG

    for( int index = 0; index<NUMDIMS; ++index )
    {
        ASSERT( a_min[index] <= a_max[index] );
    }

#endif    // _DEBUG

    Rect rect;

    for( int axis = 0; axis < NUMDIMS; ++axis )
    {
        rect.m_min[axis] = a_min[axis];
        rect.m_max[axis] = a_max[axis];
    }

    InsertRect( &rect, a_dataId, &m_root, 0 );
}


RTREE_TEMPLATE
bool RTREE_QUAL::Remove( const ELEMTYPE     a_min[NUMDIMS],
                         const ELEMTYPE     a_max[NUMDIMS],
                         const DATATYPE&    a_dataId )
{
#ifdef _DEBUG

    for( int index = 0; index<NUMDIMS; ++index )
    {
        ASSERT( a_min[index] <= a_max[index] );
    }

#endif    // _DEBUG

    Rect rect;

    for( int axis = 0; axis < NUMDIMS; ++axis )
    {
        rect.m_min[axis] = a_min[axis];
        rect.m_max[axis] = a_max[axis];
    }

    return RemoveRect( &rect, a_dataId, &m_root );
}


RTREE_TEMPLATE
int RTREE_QUAL::Search( const ELEMTYPE a_min[NUMDIMS],
                        const ELEMTYPE a_max[NUMDIMS],
                        std::function<bool (const DATATYPE&)> a_callback ) const
{
#ifdef _DEBUG

    for( int index = 0; index < NUMDIMS; ++index )
    {
        ASSERT( a_min[index] <= a_max[index] );
    }

#endif    // _DEBUG

    Rect rect;

    for( int axis = 0; axis < NUMDIMS; ++axis )
    {
        rect.m_min[axis] = a_min[axis];
        rect.m_max[axis] = a_max[axis];
    }

    // NOTE: May want to return search result another way, perhaps returning the number of found elements here.

    int foundCount = 0;
    Search( m_root, &rect, foundCount, a_callback );
    return foundCount;
}


RTREE_TEMPLATE
int RTREE_QUAL::Search( const ELEMTYPE a_min[NUMDIMS], const ELEMTYPE a_max[NUMDIMS],
        std::function<bool( const DATATYPE& )> a_callback, bool& aFinished ) const
{
#ifdef _DEBUG

    for( int index = 0; index < NUMDIMS; ++index )
    {
        ASSERT( a_min[index] <= a_max[index] );
    }

#endif // _DEBUG

    Rect rect;

    for( int axis = 0; axis < NUMDIMS; ++axis )
    {
        rect.m_min[axis] = a_min[axis];
        rect.m_max[axis] = a_max[axis];
    }

    // NOTE: May want to return search result another way, perhaps returning the number of found elements here.

    int foundCount = 0;
    aFinished      = Search( m_root, &rect, foundCount, a_callback );
    return foundCount;
}


RTREE_TEMPLATE
std::vector<std::pair<ELEMTYPE, DATATYPE>> RTREE_QUAL::NearestNeighbors(
        const ELEMTYPE a_point[NUMDIMS],
        std::function<bool( const std::size_t aNumResults, const ELEMTYPE aMinDist )> aTerminate,
        std::function<bool( const DATATYPE aElement )> aFilter,
        std::function<ELEMTYPE( const ELEMTYPE a_point[NUMDIMS], const DATATYPE a_data )> aSquaredDist ) const
{
    std::vector<std::pair<ELEMTYPE, DATATYPE>> result;
    std::priority_queue<NNNode> search_q;

    for( int i = 0; i < m_root->m_count; ++i )
    {
        if( m_root->IsLeaf() )
        {
            search_q.push( NNNode{ m_root->m_branch[i],
                               aSquaredDist( a_point, m_root->m_branch[i].m_data ),
                               m_root->IsLeaf() });
        }
        else
        {
            search_q.push( NNNode{ m_root->m_branch[i],
                               MinDist(a_point, m_root->m_branch[i].m_rect),
                               m_root->IsLeaf() });
        }
    }

    while( !search_q.empty() )
    {
        const NNNode curNode = search_q.top();

        if( aTerminate( result.size(), curNode.minDist ) )
            break;

        search_q.pop();

        if( curNode.isLeaf )
        {
            if( aFilter( curNode.m_branch.m_data ) )
                result.emplace_back( curNode.minDist, curNode.m_branch.m_data );
        }
        else
        {
            Node* node = curNode.m_branch.m_child;

            for( int i = 0; i < node->m_count; ++i )
            {
                NNNode newNode;
                newNode.isLeaf = node->IsLeaf();
                newNode.m_branch = node->m_branch[i];
                if( newNode.isLeaf )
                    newNode.minDist = aSquaredDist( a_point, newNode.m_branch.m_data );
                else
                    newNode.minDist = this->MinDist( a_point, node->m_branch[i].m_rect );

                search_q.push( newNode );
            }
        }
    }

    return result;
}

RTREE_TEMPLATE
int RTREE_QUAL::Count() const
{
    int count = 0;

    CountRec( m_root, count );

    return count;
}


RTREE_TEMPLATE
void RTREE_QUAL::CountRec( const Node* a_node, int& a_count ) const
{
    if( a_node->IsInternalNode() ) // not a leaf node
    {
        for( int index = 0; index < a_node->m_count; ++index )
        {
            CountRec( a_node->m_branch[index].m_child, a_count );
        }
    }
    else // A leaf node
    {
        a_count += a_node->m_count;
    }
}


RTREE_TEMPLATE
bool RTREE_QUAL::Load( const char* a_fileName )
{
    RemoveAll(); // Clear existing tree

    RTFileStream stream;

    if( !stream.OpenRead( a_fileName ) )
    {
        return false;
    }

    bool result = Load( stream );

    stream.Close();

    return result;
}


RTREE_TEMPLATE
bool RTREE_QUAL::Load( RTFileStream& a_stream ) const
{
    // Write some kind of header
    int _dataFileId         = ('R' << 0) | ('T' << 8) | ('R' << 16) | ('E' << 24);
    int _dataSize           = sizeof(DATATYPE);
    int _dataNumDims        = NUMDIMS;
    int _dataElemSize       = sizeof(ELEMTYPE);
    int _dataElemRealSize   = sizeof(ELEMTYPEREAL);
    int _dataMaxNodes       = TMAXNODES;
    int _dataMinNodes       = TMINNODES;

    int dataFileId          = 0;
    int dataSize            = 0;
    int dataNumDims         = 0;
    int dataElemSize        = 0;
    int dataElemRealSize    = 0;
    int dataMaxNodes        = 0;
    int dataMinNodes        = 0;

    a_stream.Read( dataFileId );
    a_stream.Read( dataSize );
    a_stream.Read( dataNumDims );
    a_stream.Read( dataElemSize );
    a_stream.Read( dataElemRealSize );
    a_stream.Read( dataMaxNodes );
    a_stream.Read( dataMinNodes );

    bool result = false;

    // Test if header was valid and compatible
    if( (dataFileId == _dataFileId)
        && (dataSize == _dataSize)
        && (dataNumDims == _dataNumDims)
        && (dataElemSize == _dataElemSize)
        && (dataElemRealSize == _dataElemRealSize)
        && (dataMaxNodes == _dataMaxNodes)
        && (dataMinNodes == _dataMinNodes)
         )
    {
        // Recursively load tree
        result = LoadRec( m_root, a_stream );
    }

    return result;
}


RTREE_TEMPLATE
bool RTREE_QUAL::LoadRec( const Node* a_node, RTFileStream& a_stream ) const
{
    a_stream.Read( a_node->m_level );
    a_stream.Read( a_node->m_count );

    if( a_node->IsInternalNode() ) // not a leaf node
    {
        for( int index = 0; index < a_node->m_count; ++index )
        {
            const Branch* curBranch = &a_node->m_branch[index];

            a_stream.ReadArray( curBranch->m_rect.m_min, NUMDIMS );
            a_stream.ReadArray( curBranch->m_rect.m_max, NUMDIMS );

            curBranch->m_child = AllocNode();
            LoadRec( curBranch->m_child, a_stream );
        }
    }
    else // A leaf node
    {
        for( int index = 0; index < a_node->m_count; ++index )
        {
            const Branch* curBranch = &a_node->m_branch[index];

            a_stream.ReadArray( curBranch->m_rect.m_min, NUMDIMS );
            a_stream.ReadArray( curBranch->m_rect.m_max, NUMDIMS );

            a_stream.Read( curBranch->m_data );
        }
    }

    return true; // Should do more error checking on I/O operations
}


RTREE_TEMPLATE
bool RTREE_QUAL::Save( const char* a_fileName )
{
    RTFileStream stream;

    if( !stream.OpenWrite( a_fileName ) )
    {
        return false;
    }

    bool result = Save( stream );

    stream.Close();

    return result;
}


RTREE_TEMPLATE
bool RTREE_QUAL::Save( RTFileStream& a_stream ) const
{
    // Write some kind of header
    int dataFileId          = ('R' << 0) | ('T' << 8) | ('R' << 16) | ('E' << 24);
    int dataSize            = sizeof(DATATYPE);
    int dataNumDims         = NUMDIMS;
    int dataElemSize        = sizeof(ELEMTYPE);
    int dataElemRealSize    = sizeof(ELEMTYPEREAL);
    int dataMaxNodes        = TMAXNODES;
    int dataMinNodes        = TMINNODES;

    a_stream.Write( dataFileId );
    a_stream.Write( dataSize );
    a_stream.Write( dataNumDims );
    a_stream.Write( dataElemSize );
    a_stream.Write( dataElemRealSize );
    a_stream.Write( dataMaxNodes );
    a_stream.Write( dataMinNodes );

    // Recursively save tree
    bool result = SaveRec( m_root, a_stream );

    return result;
}


RTREE_TEMPLATE
bool RTREE_QUAL::SaveRec( const Node* a_node, RTFileStream& a_stream ) const
{
    a_stream.Write( a_node->m_level );
    a_stream.Write( a_node->m_count );

    if( a_node->IsInternalNode() ) // not a leaf node
    {
        for( int index = 0; index < a_node->m_count; ++index )
        {
            const Branch* curBranch = &a_node->m_branch[index];

            a_stream.WriteArray( curBranch->m_rect.m_min, NUMDIMS );
            a_stream.WriteArray( curBranch->m_rect.m_max, NUMDIMS );

            SaveRec( curBranch->m_child, a_stream );
        }
    }
    else // A leaf node
    {
        for( int index = 0; index < a_node->m_count; ++index )
        {
            const Branch* curBranch = &a_node->m_branch[index];

            a_stream.WriteArray( curBranch->m_rect.m_min, NUMDIMS );
            a_stream.WriteArray( curBranch->m_rect.m_max, NUMDIMS );

            a_stream.Write( curBranch->m_data );
        }
    }

    return true; // Should do more error checking on I/O operations
}


RTREE_TEMPLATE
void RTREE_QUAL::RemoveAll()
{
    // Delete all existing nodes
    Reset();

    m_root = AllocNode();
    m_root->m_level = 0;
}


RTREE_TEMPLATE
void RTREE_QUAL::Reset() const
{
#ifdef RTREE_DONT_USE_MEMPOOLS
    // Delete all existing nodes
    RemoveAllRec( m_root );
#else    // RTREE_DONT_USE_MEMPOOLS
    // Just reset memory pools.  We are not using complex types
    // EXAMPLE
#endif    // RTREE_DONT_USE_MEMPOOLS
}


RTREE_TEMPLATE
void RTREE_QUAL::RemoveAllRec( Node* a_node ) const
{
    ASSERT( a_node );
    ASSERT( a_node->m_level >= 0 );

    if( a_node->IsInternalNode() ) // This is an internal node in the tree
    {
        for( int index = 0; index < a_node->m_count; ++index )
        {
            RemoveAllRec( a_node->m_branch[index].m_child );
        }
    }

    FreeNode( a_node );
}


RTREE_TEMPLATE
typename RTREE_QUAL::Node* RTREE_QUAL::AllocNode() const
{
    Node* newNode;

#ifdef RTREE_DONT_USE_MEMPOOLS
    newNode = new Node;
#else       // RTREE_DONT_USE_MEMPOOLS
    // EXAMPLE
#endif      // RTREE_DONT_USE_MEMPOOLS
    InitNode( newNode );
    return newNode;
}


RTREE_TEMPLATE
void RTREE_QUAL::FreeNode( Node* a_node ) const
{
    ASSERT( a_node );

#ifdef RTREE_DONT_USE_MEMPOOLS
    delete a_node;
#else       // RTREE_DONT_USE_MEMPOOLS
    // EXAMPLE
#endif      // RTREE_DONT_USE_MEMPOOLS
}


// Allocate space for a node in the list used in DeletRect to
// store Nodes that are too empty.
RTREE_TEMPLATE
typename RTREE_QUAL::ListNode* RTREE_QUAL::AllocListNode() const
{
#ifdef RTREE_DONT_USE_MEMPOOLS
    return new ListNode;
#else       // RTREE_DONT_USE_MEMPOOLS
    // EXAMPLE
#endif      // RTREE_DONT_USE_MEMPOOLS
}


RTREE_TEMPLATE
void RTREE_QUAL::FreeListNode( ListNode* a_listNode ) const
{
#ifdef RTREE_DONT_USE_MEMPOOLS
    delete a_listNode;
#else       // RTREE_DONT_USE_MEMPOOLS
    // EXAMPLE
#endif      // RTREE_DONT_USE_MEMPOOLS
}


RTREE_TEMPLATE
void RTREE_QUAL::InitNode( Node* a_node ) const
{
    a_node->m_count = 0;
    a_node->m_level = -1;
}


RTREE_TEMPLATE
void RTREE_QUAL::InitRect( Rect* a_rect ) const
{
    for( int index = 0; index < NUMDIMS; ++index )
    {
        a_rect->m_min[index] = (ELEMTYPE) 0;
        a_rect->m_max[index] = (ELEMTYPE) 0;
    }
}


// Inserts a new data rectangle into the index structure.
// Recursively descends tree, propagates splits back up.
// Returns 0 if node was not split.  Old node updated.
// If node was split, returns 1 and sets the pointer pointed to by
// new_node to point to the new node.  Old node updated to become one of two.
// The level argument specifies the number of steps up from the leaf
// level to insert; e.g. a data rectangle goes in at level = 0.
RTREE_TEMPLATE
bool RTREE_QUAL::InsertRectRec( const Rect*     a_rect,
                                const DATATYPE& a_id,
                                Node*           a_node,
                                Node**          a_newNode,
                                int             a_level ) const
{
    ASSERT( a_rect && a_node && a_newNode );
    ASSERT( a_level >= 0 && a_level <= a_node->m_level );

    int     index;
    Branch  branch;
    Node*   otherNode;

    // Still above level for insertion, go down tree recursively
    if( a_node->m_level > a_level )
    {
        index = PickBranch( a_rect, a_node );

        if( !InsertRectRec( a_rect, a_id, a_node->m_branch[index].m_child, &otherNode, a_level ) )
        {
            // Child was not split
            a_node->m_branch[index].m_rect =
                CombineRect( a_rect, &(a_node->m_branch[index].m_rect) );
            return false;
        }
        else // Child was split
        {
            a_node->m_branch[index].m_rect = NodeCover( a_node->m_branch[index].m_child );
            branch.m_child  = otherNode;
            branch.m_rect   = NodeCover( otherNode );
            return AddBranch( &branch, a_node, a_newNode );
        }
    }
    else if( a_node->m_level == a_level ) // Have reached level for insertion. Add rect, split if necessary
    {
        branch.m_rect   = *a_rect;
        branch.m_child  = (Node*) a_id;
        // Child field of leaves contains id of data record
        return AddBranch( &branch, a_node, a_newNode );
    }
    else
    {
        // Should never occur
        ASSERT( 0 );
        return false;
    }
}


// Insert a data rectangle into an index structure.
// InsertRect provides for splitting the root;
// returns 1 if root was split, 0 if it was not.
// The level argument specifies the number of steps up from the leaf
// level to insert; e.g. a data rectangle goes in at level = 0.
// InsertRect2 does the recursion.
//
RTREE_TEMPLATE
bool RTREE_QUAL::InsertRect( const Rect* a_rect, const DATATYPE& a_id, Node** a_root, int a_level ) const
{
    ASSERT( a_rect && a_root );
    ASSERT( a_level >= 0 && a_level <= (*a_root)->m_level );
#ifdef _DEBUG

    for( int index = 0; index < NUMDIMS; ++index )
    {
        ASSERT( a_rect->m_min[index] <= a_rect->m_max[index] );
    }

#endif    // _DEBUG

    Node*   newRoot;
    Node*   newNode;
    Branch  branch;

    if( InsertRectRec( a_rect, a_id, *a_root, &newNode, a_level ) ) // Root split
    {
        newRoot = AllocNode();                                      // Grow tree taller and new root
        newRoot->m_level    = (*a_root)->m_level + 1;
        branch.m_rect       = NodeCover( *a_root );
        branch.m_child      = *a_root;
        AddBranch( &branch, newRoot, NULL );
        branch.m_rect   = NodeCover( newNode );
        branch.m_child  = newNode;
        AddBranch( &branch, newRoot, NULL );
        *a_root = newRoot;
        return true;
    }

    return false;
}


// Find the smallest rectangle that includes all rectangles in branches of a node.
RTREE_TEMPLATE
typename RTREE_QUAL::Rect RTREE_QUAL::NodeCover( Node* a_node ) const
{
    ASSERT( a_node );

    bool     firstTime = true;
    Rect    rect;
    InitRect( &rect );

    for( int index = 0; index < a_node->m_count; ++index )
    {
        if( firstTime )
        {
            rect = a_node->m_branch[index].m_rect;
            firstTime = false;
        }
        else
        {
            rect = CombineRect( &rect, &(a_node->m_branch[index].m_rect) );
        }
    }

    return rect;
}


// Add a branch to a node.  Split the node if necessary.
// Returns 0 if node not split.  Old node updated.
// Returns 1 if node split, sets *new_node to address of new node.
// Old node updated, becomes one of two.
RTREE_TEMPLATE
bool RTREE_QUAL::AddBranch( const Branch* a_branch, Node* a_node, Node** a_newNode ) const
{
    ASSERT( a_branch );
    ASSERT( a_node );

    if( a_node->m_count < MAXNODES ) // Split won't be necessary
    {
        a_node->m_branch[a_node->m_count] = *a_branch;
        ++a_node->m_count;

        return false;
    }
    else
    {
        ASSERT( a_newNode );

        SplitNode( a_node, a_branch, a_newNode );
        return true;
    }
}


// Disconnect a dependent node.
// Caller must return (or stop using iteration index) after this as count has changed
RTREE_TEMPLATE
void RTREE_QUAL::DisconnectBranch( Node* a_node, int a_index ) const
{
    ASSERT( a_node && (a_index >= 0) && (a_index < MAXNODES) );
    ASSERT( a_node->m_count > 0 );

    // Remove element by swapping with the last element to prevent gaps in array
    a_node->m_branch[a_index] = a_node->m_branch[a_node->m_count - 1];

    --a_node->m_count;
}


// Pick a branch.  Pick the one that will need the smallest increase
// in area to accomodate the new rectangle.  This will result in the
// least total area for the covering rectangles in the current node.
// In case of a tie, pick the one which was smaller before, to get
// the best resolution when searching.
RTREE_TEMPLATE
int RTREE_QUAL::PickBranch( const Rect* a_rect, Node* a_node ) const
{
    ASSERT( a_rect && a_node );

    bool            firstTime = true;
    ELEMTYPEREAL    increase;
    ELEMTYPEREAL    bestIncr = (ELEMTYPEREAL) -1;
    ELEMTYPEREAL    area;
    ELEMTYPEREAL    bestArea = 0;
    int             best = 0;
    Rect            tempRect;

    for( int index = 0; index < a_node->m_count; ++index )
    {
        Rect* curRect = &a_node->m_branch[index].m_rect;
        area        = CalcRectVolume( curRect );
        tempRect    = CombineRect( a_rect, curRect );
        increase    = CalcRectVolume( &tempRect ) - area;

        if( (increase < bestIncr) || firstTime )
        {
            best        = index;
            bestArea    = area;
            bestIncr    = increase;
            firstTime   = false;
        }
        else if( (increase == bestIncr) && (area < bestArea) )
        {
            best        = index;
            bestArea    = area;
            bestIncr    = increase;
        }
    }

    return best;
}


// Combine two rectangles into larger one containing both
RTREE_TEMPLATE
typename RTREE_QUAL::Rect RTREE_QUAL::CombineRect( const Rect* a_rectA, const Rect* a_rectB ) const
{
    ASSERT( a_rectA && a_rectB );

    Rect newRect;

    for( int index = 0; index < NUMDIMS; ++index )
    {
        newRect.m_min[index] = std::min( a_rectA->m_min[index], a_rectB->m_min[index] );
        newRect.m_max[index] = std::max( a_rectA->m_max[index], a_rectB->m_max[index] );
    }

    return newRect;
}


// Split a node.
// Divides the nodes branches and the extra one between two nodes.
// Old node is one of the new ones, and one really new one is created.
// Tries more than one method for choosing a partition, uses best result.
RTREE_TEMPLATE
void RTREE_QUAL::SplitNode( Node* a_node, const Branch* a_branch, Node** a_newNode ) const
{
    ASSERT( a_node );
    ASSERT( a_branch );

    // Could just use local here, but member or external is faster since it is reused
    PartitionVars   localVars;
    PartitionVars*  parVars = &localVars;
    int             level;

    // Load all the branches into a buffer, initialize old node
    level = a_node->m_level;
    GetBranches( a_node, a_branch, parVars );

    // Find partition
    ChoosePartition( parVars, MINNODES );

    // Put branches from buffer into 2 nodes according to chosen partition
    *a_newNode = AllocNode();
    (*a_newNode)->m_level = a_node->m_level = level;
    LoadNodes( a_node, *a_newNode, parVars );

    ASSERT( (a_node->m_count + (*a_newNode)->m_count) == parVars->m_total );
}


// Calculate the n-dimensional volume of a rectangle
RTREE_TEMPLATE
ELEMTYPEREAL RTREE_QUAL::RectVolume( const Rect* a_rect ) const
{
    ASSERT( a_rect );

    ELEMTYPEREAL volume = (ELEMTYPEREAL) 1;

    for( int index = 0; index<NUMDIMS; ++index )
    {
        volume *= a_rect->m_max[index] - a_rect->m_min[index];
    }

    ASSERT( volume >= (ELEMTYPEREAL) 0 );

    return volume;
}


// The exact volume of the bounding sphere for the given Rect
RTREE_TEMPLATE
ELEMTYPEREAL RTREE_QUAL::RectSphericalVolume( const Rect* a_rect ) const
{
    ASSERT( a_rect );

    ELEMTYPEREAL sumOfSquares = (ELEMTYPEREAL) 0;

    for( int index = 0; index < NUMDIMS; ++index )
    {
        ELEMTYPEREAL halfExtent =
            ( (ELEMTYPEREAL) a_rect->m_max[index] - (ELEMTYPEREAL) a_rect->m_min[index] ) * 0.5f;
        sumOfSquares += halfExtent * halfExtent;
    }

    // Pow maybe slow, so test for common dims like 2,3 and just use x*x, x*x*x.
    if( NUMDIMS == 2 )
    {
        return sumOfSquares * m_unitSphereVolume;
    }
    else if( NUMDIMS == 3 )
    {
        ELEMTYPEREAL radius = (ELEMTYPEREAL) std::sqrt( sumOfSquares );

        return radius * radius * radius * m_unitSphereVolume;
    }
    else
    {
        ELEMTYPEREAL radius = (ELEMTYPEREAL) std::sqrt( sumOfSquares );

        return (ELEMTYPEREAL) (std::pow( radius, NUMDIMS ) * m_unitSphereVolume);
    }
}


// Use one of the methods to calculate retangle volume
RTREE_TEMPLATE
ELEMTYPEREAL RTREE_QUAL::CalcRectVolume( const Rect* a_rect ) const
{
#ifdef RTREE_USE_SPHERICAL_VOLUME
    return RectSphericalVolume( a_rect );   // Slower but helps certain merge cases
#else                                       // RTREE_USE_SPHERICAL_VOLUME
    return RectVolume( a_rect );            // Faster but can cause poor merges
#endif                                      // RTREE_USE_SPHERICAL_VOLUME
}


// Load branch buffer with branches from full node plus the extra branch.
RTREE_TEMPLATE
void RTREE_QUAL::GetBranches( Node* a_node, const Branch* a_branch, PartitionVars* a_parVars ) const
{
    ASSERT( a_node );
    ASSERT( a_branch );

    ASSERT( a_node->m_count == MAXNODES );

    // Load the branch buffer
    for( int index = 0; index < MAXNODES; ++index )
    {
        a_parVars->m_branchBuf[index] = a_node->m_branch[index];
    }

    a_parVars->m_branchBuf[MAXNODES] = *a_branch;
    a_parVars->m_branchCount = MAXNODES + 1;

    // Calculate rect containing all in the set
    a_parVars->m_coverSplit = a_parVars->m_branchBuf[0].m_rect;

    for( int index = 1; index < MAXNODES + 1; ++index )
    {
        a_parVars->m_coverSplit =
            CombineRect( &a_parVars->m_coverSplit, &a_parVars->m_branchBuf[index].m_rect );
    }

    a_parVars->m_coverSplitArea = CalcRectVolume( &a_parVars->m_coverSplit );

    InitNode( a_node );
}


// Method #0 for choosing a partition:
// As the seeds for the two groups, pick the two rects that would waste the
// most area if covered by a single rectangle, i.e. evidently the worst pair
// to have in the same group.
// Of the remaining, one at a time is chosen to be put in one of the two groups.
// The one chosen is the one with the greatest difference in area expansion
// depending on which group - the rect most strongly attracted to one group
// and repelled from the other.
// If one group gets too full (more would force other group to violate min
// fill requirement) then other group gets the rest.
// These last are the ones that can go in either group most easily.
RTREE_TEMPLATE
void RTREE_QUAL::ChoosePartition( PartitionVars* a_parVars, int a_minFill ) const
{
    ASSERT( a_parVars );

    ELEMTYPEREAL    biggestDiff;
    int             group, chosen = 0, betterGroup = 0;

    InitParVars( a_parVars, a_parVars->m_branchCount, a_minFill );
    PickSeeds( a_parVars );

    while( ( (a_parVars->m_count[0] + a_parVars->m_count[1]) < a_parVars->m_total )
           && ( a_parVars->m_count[0] < (a_parVars->m_total - a_parVars->m_minFill) )
           && ( a_parVars->m_count[1] < (a_parVars->m_total - a_parVars->m_minFill) ) )
    {
        biggestDiff = (ELEMTYPEREAL) -1;

        for( int index = 0; index<a_parVars->m_total; ++index )
        {
            if( !a_parVars->m_taken[index] )
            {
                const Rect*     curRect = &a_parVars->m_branchBuf[index].m_rect;
                const Rect      rect0   = CombineRect( curRect, &a_parVars->m_cover[0] );
                const Rect      rect1   = CombineRect( curRect, &a_parVars->m_cover[1] );
                ELEMTYPEREAL    growth0 = CalcRectVolume( &rect0 ) - a_parVars->m_area[0];
                ELEMTYPEREAL    growth1 = CalcRectVolume( &rect1 ) - a_parVars->m_area[1];
                ELEMTYPEREAL    diff    = growth1 - growth0;

                if( diff >= 0 )
                {
                    group = 0;
                }
                else
                {
                    group = 1;
                    diff  = -diff;
                }

                if( diff > biggestDiff )
                {
                    biggestDiff = diff;
                    chosen = index;
                    betterGroup = group;
                }
                else if( (diff == biggestDiff)
                         && (a_parVars->m_count[group] < a_parVars->m_count[betterGroup]) )
                {
                    chosen = index;
                    betterGroup = group;
                }
            }
        }

        Classify( chosen, betterGroup, a_parVars );
    }

    // If one group too full, put remaining rects in the other
    if( (a_parVars->m_count[0] + a_parVars->m_count[1]) < a_parVars->m_total )
    {
        if( a_parVars->m_count[0] >= a_parVars->m_total - a_parVars->m_minFill )
        {
            group = 1;
        }
        else
        {
            group = 0;
        }

        for( int index = 0; index<a_parVars->m_total; ++index )
        {
            if( !a_parVars->m_taken[index] )
            {
                Classify( index, group, a_parVars );
            }
        }
    }

    ASSERT( (a_parVars->m_count[0] + a_parVars->m_count[1]) == a_parVars->m_total );
    ASSERT( (a_parVars->m_count[0] >= a_parVars->m_minFill)
            && (a_parVars->m_count[1] >= a_parVars->m_minFill) );
}


// Copy branches from the buffer into two nodes according to the partition.
RTREE_TEMPLATE
void RTREE_QUAL::LoadNodes( Node* a_nodeA, Node* a_nodeB, PartitionVars* a_parVars ) const
{
    ASSERT( a_nodeA );
    ASSERT( a_nodeB );
    ASSERT( a_parVars );

    for( int index = 0; index < a_parVars->m_total; ++index )
    {
        ASSERT( a_parVars->m_partition[index] == 0 || a_parVars->m_partition[index] == 1 );

        if( a_parVars->m_partition[index] == 0 )
        {
            AddBranch( &a_parVars->m_branchBuf[index], a_nodeA, NULL );
        }
        else if( a_parVars->m_partition[index] == 1 )
        {
            AddBranch( &a_parVars->m_branchBuf[index], a_nodeB, NULL );
        }
    }
}


// Initialize a PartitionVars structure.
RTREE_TEMPLATE
void RTREE_QUAL::InitParVars( PartitionVars* a_parVars, int a_maxRects, int a_minFill ) const
{
    ASSERT( a_parVars );

    a_parVars->m_count[0]   = a_parVars->m_count[1] = 0;
    a_parVars->m_area[0]    = a_parVars->m_area[1] = (ELEMTYPEREAL) 0;
    a_parVars->m_total      = a_maxRects;
    a_parVars->m_minFill    = a_minFill;

    for( int index = 0; index < a_maxRects; ++index )
    {
        a_parVars->m_taken[index]       = false;
        a_parVars->m_partition[index]   = -1;
    }
}


RTREE_TEMPLATE
void RTREE_QUAL::PickSeeds( PartitionVars* a_parVars ) const
{
    int             seed0 = 0, seed1 = 0;
    ELEMTYPEREAL    worst, waste;
    ELEMTYPEREAL    area[MAXNODES + 1];

    for( int index = 0; index<a_parVars->m_total; ++index )
    {
        area[index] = CalcRectVolume( &a_parVars->m_branchBuf[index].m_rect );
    }

    worst = -a_parVars->m_coverSplitArea - 1;

    for( int indexA = 0; indexA < a_parVars->m_total - 1; ++indexA )
    {
        for( int indexB = indexA + 1; indexB < a_parVars->m_total; ++indexB )
        {
            Rect oneRect = CombineRect( &a_parVars->m_branchBuf[indexA].m_rect,
                                        &a_parVars->m_branchBuf[indexB].m_rect );
            waste = CalcRectVolume( &oneRect ) - area[indexA] - area[indexB];

            if( waste >= worst )
            {
                worst   = waste;
                seed0   = indexA;
                seed1   = indexB;
            }
        }
    }

    Classify( seed0, 0, a_parVars );
    Classify( seed1, 1, a_parVars );
}


// Put a branch in one of the groups.
RTREE_TEMPLATE
void RTREE_QUAL::Classify( int a_index, int a_group, PartitionVars* a_parVars ) const
{
    ASSERT( a_parVars );
    ASSERT( !a_parVars->m_taken[a_index] );

    a_parVars->m_partition[a_index] = a_group;
    a_parVars->m_taken[a_index]     = true;

    if( a_parVars->m_count[a_group] == 0 )
    {
        a_parVars->m_cover[a_group] = a_parVars->m_branchBuf[a_index].m_rect;
    }
    else
    {
        a_parVars->m_cover[a_group] = CombineRect( &a_parVars->m_branchBuf[a_index].m_rect,
                                                   &a_parVars->m_cover[a_group] );
    }

    a_parVars->m_area[a_group] = CalcRectVolume( &a_parVars->m_cover[a_group] );
    ++a_parVars->m_count[a_group];
}


// Delete a data rectangle from an index structure.
// Pass in a pointer to a Rect, the tid of the record, ptr to ptr to root node.
// Returns 1 if record not found, 0 if success.
// RemoveRect provides for eliminating the root.
RTREE_TEMPLATE
bool RTREE_QUAL::RemoveRect( const Rect* a_rect, const DATATYPE& a_id, Node** a_root ) const
{
    ASSERT( a_rect && a_root );
    ASSERT( *a_root );

    Node*       tempNode;
    ListNode*   reInsertList = NULL;

    if( !RemoveRectRec( a_rect, a_id, *a_root, &reInsertList ) )
    {
        // Found and deleted a data item
        // Reinsert any branches from eliminated nodes
        while( reInsertList )
        {
            tempNode = reInsertList->m_node;

            for( int index = 0; index < tempNode->m_count; ++index )
            {
                InsertRect( &(tempNode->m_branch[index].m_rect),
                            tempNode->m_branch[index].m_data,
                            a_root,
                            tempNode->m_level );
            }

            ListNode* remLNode = reInsertList;
            reInsertList = reInsertList->m_next;

            FreeNode( remLNode->m_node );
            FreeListNode( remLNode );
        }

        // Check for redundant root (not leaf, 1 child) and eliminate
        if( (*a_root)->m_count == 1 && (*a_root)->IsInternalNode() )
        {
            tempNode = (*a_root)->m_branch[0].m_child;

            ASSERT( tempNode );
            FreeNode( *a_root );
            *a_root = tempNode;
        }

        return false;
    }
    else
    {
        return true;
    }
}


// Delete a rectangle from non-root part of an index structure.
// Called by RemoveRect.  Descends tree recursively,
// merges branches on the way back up.
// Returns 1 if record not found, 0 if success.
RTREE_TEMPLATE
bool RTREE_QUAL::RemoveRectRec( const Rect*     a_rect,
                                const DATATYPE& a_id,
                                Node*           a_node,
                                ListNode**      a_listNode ) const
{
    ASSERT( a_rect && a_node && a_listNode );
    ASSERT( a_node->m_level >= 0 );

    if( a_node->IsInternalNode() ) // not a leaf node
    {
        for( int index = 0; index < a_node->m_count; ++index )
        {
            if( Overlap( a_rect, &(a_node->m_branch[index].m_rect) ) )
            {
                if( !RemoveRectRec( a_rect, a_id, a_node->m_branch[index].m_child, a_listNode ) )
                {
                    if( a_node->m_branch[index].m_child->m_count >= MINNODES )
                    {
                        // child removed, just resize parent rect
                        a_node->m_branch[index].m_rect =
                            NodeCover( a_node->m_branch[index].m_child );
                    }
                    else
                    {
                        // child removed, not enough entries in node, eliminate node
                        ReInsert( a_node->m_branch[index].m_child, a_listNode );
                        DisconnectBranch( a_node, index ); // Must return after this call as count has changed
                    }

                    return false;
                }
            }
        }

        return true;
    }
    else // A leaf node
    {
        for( int index = 0; index < a_node->m_count; ++index )
        {
            if( a_node->m_branch[index].m_child == (Node*) a_id )
            {
                DisconnectBranch( a_node, index ); // Must return after this call as count has changed
                return false;
            }
        }

        return true;
    }
}


// Decide whether two rectangles overlap.
RTREE_TEMPLATE
bool RTREE_QUAL::Overlap( const Rect* a_rectA, const Rect* a_rectB )
{
    ASSERT( a_rectA && a_rectB );

    for( int index = 0; index < NUMDIMS; ++index )
    {
        if( a_rectA->m_min[index] > a_rectB->m_max[index]
            || a_rectB->m_min[index] > a_rectA->m_max[index] )
        {
            return false;
        }
    }

    return true;
}


// Add a node to the reinsertion list.  All its branches will later
// be reinserted into the index structure.
RTREE_TEMPLATE
void RTREE_QUAL::ReInsert( Node* a_node, ListNode** a_listNode ) const
{
    ListNode* newListNode;

    newListNode = AllocListNode();
    newListNode->m_node = a_node;
    newListNode->m_next = *a_listNode;
    *a_listNode = newListNode;
}


// Search in an index tree or subtree for all data rectangles that overlap the argument rectangle.
RTREE_TEMPLATE
bool RTREE_QUAL::Search( const Node* a_node, const Rect* a_rect, int& a_foundCount,
        std::function<bool (const DATATYPE&)> a_callback ) const
{
    ASSERT( a_node );
    ASSERT( a_node->m_level >= 0 );
    ASSERT( a_rect );

    if( a_node->IsInternalNode() ) // This is an internal node in the tree
    {
        for( int index = 0; index < a_node->m_count; ++index )
        {
            if( Overlap( a_rect, &a_node->m_branch[index].m_rect ) )
            {
                if( !Search( a_node->m_branch[index].m_child, a_rect, a_foundCount, a_callback ) )
                {
                    return false; // Don't continue searching
                }
            }
        }
    }
    else // This is a leaf node
    {
        for( int index = 0; index < a_node->m_count; ++index )
        {
            if( Overlap( a_rect, &a_node->m_branch[index].m_rect ) )
            {
                const DATATYPE& id = a_node->m_branch[index].m_data;
                ++a_foundCount;

                if( a_callback && !a_callback( id ) )
                {
                    return false; // Don't continue searching
                }
            }
        }
    }

    return true; // Continue searching
}


//calculate the minimum distance between a point and a rectangle as defined by Manolopoulos et al.
// returns Euclidean norm to ensure value fits in ELEMTYPE
RTREE_TEMPLATE
ELEMTYPE RTREE_QUAL::MinDist( const ELEMTYPE a_point[NUMDIMS], const Rect& a_rect ) const
{
    const ELEMTYPE *q, *s, *t;
    q = a_point;
    s = a_rect.m_min;
    t = a_rect.m_max;
    double minDist = 0.0;

    for( int index = 0; index < NUMDIMS; index++ )
    {
        int r = q[index];

        if( q[index] < s[index] )
        {
            r = s[index];
        }
        else if( q[index] > t[index] )
        {
            r = t[index];
        }

        double addend = q[index] - r;
        minDist += addend * addend;
    }

    return std::lround( std::sqrt( minDist ) );
}


#undef RTREE_TEMPLATE
#undef RTREE_QUAL
#undef RTREE_SEARCH_TEMPLATE
#undef RTREE_SEARCH_QUAL

#endif    // RTREE_H
