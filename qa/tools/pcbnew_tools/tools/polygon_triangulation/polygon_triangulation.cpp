/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Alejandro García Montoro <alejandro.garciamontoro@gmail.com>
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

#include <geometry/shape_line_chain.h>
#include <geometry/shape_poly_set.h>

#include <pcbnew_utils/board_file_utils.h>

#include <qa_utils/utility_registry.h>

#include <board.h>
#include <core/ignore.h>
#include <zone.h>
#include <core/profile.h>

#include <atomic>
#include <thread>
#include <unordered_set>
#include <utility>


void unfracture( SHAPE_POLY_SET::POLYGON* aPoly, SHAPE_POLY_SET::POLYGON* aResult )
{
    assert( aPoly->size() == 1 );

    struct EDGE
    {
        int m_index = 0;
        SHAPE_LINE_CHAIN* m_poly = nullptr;
        bool m_duplicate = false;

        EDGE( SHAPE_LINE_CHAIN *aPolygon, int aIndex ) :
            m_index(aIndex),
            m_poly(aPolygon)
            {}

        bool compareSegs( const SEG& s1, const SEG& s2) const
        {
            return (s1.A == s2.A && s1.B == s2.B) || (s1.A == s2.B && s1.B == s2.A);
        }

        bool operator==( const EDGE& aOther ) const
        {
            return compareSegs(
                    m_poly->Segment( m_index ), aOther.m_poly->Segment( aOther.m_index ) );
        }

        bool operator!=( const EDGE& aOther ) const
        {
            return !compareSegs(
                    m_poly->Segment( m_index ), aOther.m_poly->Segment( aOther.m_index ) );
        }

        struct HASH
        {
            std::size_t operator()(  const EDGE& aEdge ) const
            {
                const auto& a = aEdge.m_poly->Segment( aEdge.m_index );
                return (std::size_t) ( a.A.x + a.B.x + a.A.y + a.B.y );
            }
        };

    };

    struct EDGE_LIST_ENTRY
    {
        int index;
        EDGE_LIST_ENTRY *next;
    };

    std::unordered_set<EDGE, EDGE::HASH> uniqueEdges;

    auto lc = (*aPoly)[0];
    lc.Simplify();

    auto edgeList = std::make_unique<EDGE_LIST_ENTRY []>( lc.SegmentCount() );

    for(int i = 0; i < lc.SegmentCount(); i++)
    {
        edgeList[i].index = i;
        edgeList[i].next = &edgeList[ (i != lc.SegmentCount() - 1) ? i + 1 : 0 ];
    }

    std::unordered_set<EDGE_LIST_ENTRY*> queue;

    for(int i = 0; i < lc.SegmentCount(); i++)
    {
        EDGE e ( &lc, i );
        uniqueEdges.insert( e );
    }

    for(int i = 0; i < lc.SegmentCount(); i++)
    {
        EDGE e ( &lc, i );
        auto it = uniqueEdges.find(e);
        if (it != uniqueEdges.end() && it->m_index != i )
        {
            int e1 = it->m_index;
            int e2 = i;
            if( e1 > e2 )
                std::swap(e1, e2);

            int e1_prev = e1 - 1;
            if (e1_prev < 0)
                e1_prev = lc.SegmentCount() - 1;

            int e2_prev = e2 - 1;
            if (e2_prev < 0)
                e2_prev = lc.SegmentCount() - 1;

            int e1_next = e1 + 1;
            if (e1_next == lc.SegmentCount() )
                e1_next = 0;

            int e2_next = e2 + 1;
                if (e2_next == lc.SegmentCount() )
                    e2_next = 0;

            edgeList[e1_prev].next = &edgeList[ e2_next ];
            edgeList[e2_prev].next = &edgeList[ e1_next ];
            edgeList[i].next = nullptr;
            edgeList[it->m_index].next = nullptr;
        }
    }

    for(int i = 0; i < lc.SegmentCount(); i++)
    {
        if ( edgeList[i].next )
            queue.insert ( &edgeList[i] );
    }

    auto edgeBuf = std::make_unique<EDGE_LIST_ENTRY* []>( lc.SegmentCount() );

    int n = 0;
    int outline = -1;
aResult->clear();
    while (queue.size())
    {
        auto e_first = (*queue.begin());
        auto e = e_first;
        int cnt=0;

        do
        {
            edgeBuf[cnt++] = e;
            e = e->next;
        } while( e != e_first );

        SHAPE_LINE_CHAIN outl;

        for(int i = 0; i < cnt ;i++)
        {
            auto p = lc.CPoint(edgeBuf[i]->index);
            outl.Append( p );
            queue.erase( edgeBuf[i] );
        }

        //        auto p_last = lc.Point( edgeBuf[cnt-1]->index + 1 );
        //        outl.Append( p_last );

        outl.SetClosed(true);

        bool cw = outl.Area() > 0.0;

        if(cw)
            outline = n;

        aResult->push_back(outl);
        n++;
    }

    assert(outline >= 0);

    if(outline !=0 )
        std::swap( (*aResult) [0], (*aResult)[outline] );
}


enum POLY_TRI_RET_CODES
{
    LOAD_FAILED = KI_TEST::RET_CODES::TOOL_SPECIFIC,
};


int polygon_triangulation_main( int argc, char *argv[] )
{
    std::string filename;

    if( argc > 1 )
        filename = argv[1];

    auto brd = KI_TEST::ReadBoardFromFileOrStream( filename );

    if( !brd )
        return POLY_TRI_RET_CODES::LOAD_FAILED;


    PROF_TIMER cnt( "allBoard" );


    std::atomic<size_t> zonesToTriangulate( 0 );
    std::atomic<size_t> threadsFinished( 0 );

    size_t parallelThreadCount = std::max<size_t>( std::thread::hardware_concurrency(), 2 );
    for( size_t ii = 0; ii < parallelThreadCount; ++ii )
    {
        std::thread t = std::thread( [&brd, &zonesToTriangulate, &threadsFinished]() {
            for( size_t areaId = zonesToTriangulate.fetch_add( 1 );
                        areaId < static_cast<size_t>( brd->GetAreaCount() );
                        areaId = zonesToTriangulate.fetch_add( 1 ) )
            {
                auto zone = brd->GetArea( areaId );

                // NOTE: this could be refactored to do multiple layers from the same zone in
                // parallel, but since the test case doesn't have any of these, I'm not bothering
                // to do that right now.
                for( PCB_LAYER_ID layer : zone->GetLayerSet().Seq() )
                {
                    SHAPE_POLY_SET poly = *zone->GetFilledPolysList( layer );

                    poly.CacheTriangulation();

                    ignore_unused( poly );
#if 0
                PROF_TIMER unfrac("unfrac");
                poly.Unfracture();
                unfrac.Show();

                PROF_TIMER triangulate("triangulate");

                for(int i =0; i< poly.OutlineCount(); i++)
                {
                    poly.triangulatePoly( &poly.Polygon(i) );
                }
                triangulate.Show();
#endif
                }
            }

            threadsFinished++;
        } );

        t.detach();
    }

    while( threadsFinished < parallelThreadCount )
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

    cnt.Show();

    return KI_TEST::RET_CODES::OK;
}


static bool registered = UTILITY_REGISTRY::Register( {
        "polygon_triangulation",
        "Process polygon triangulation on a PCB",
        polygon_triangulation_main,
} );
