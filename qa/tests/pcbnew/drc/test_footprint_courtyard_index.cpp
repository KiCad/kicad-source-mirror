/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <set>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_construction_utils.h>
#include <board.h>
#include <footprint.h>
#include <footprint_courtyard_index.h>

/*
 * The courtyard spatial index replaces the full-board scan that intersectsCourtyard()-style
 * predicates used to run.  It is only correct if it returns every footprint whose courtyard
 * bounding box overlaps the query box, which is precisely the broad-phase test the downstream
 * collision check applies.  These tests pin that invariant against a brute-force reference so a
 * future change to the index (e.g. indexing by footprint body instead of courtyard) cannot
 * silently drop courtyard collisions.
 */

namespace
{

int mm( double aMillimetres )
{
    return pcbIUScale.mmToIU( aMillimetres );
}


/// Add a footprint with a single rectangular front courtyard centred on its origin.
FOOTPRINT* addFootprint( BOARD& aBoard, const wxString& aRef, const VECTOR2I& aPos,
                         const VECTOR2I& aCourtyardSize )
{
    std::unique_ptr<FOOTPRINT> footprint = std::make_unique<FOOTPRINT>( &aBoard );

    KI_TEST::DrawRect( *footprint, VECTOR2I( 0, 0 ), aCourtyardSize, 0, mm( 0.1 ), F_CrtYd );

    footprint->SetReference( aRef );

    // SetPosition moves the already-drawn graphics, so it must run after DrawRect for the
    // courtyard to land at aPos rather than the origin.
    footprint->SetPosition( aPos );

    FOOTPRINT* raw = footprint.get();
    aBoard.Add( footprint.release() );

    return raw;
}


/// The footprints whose combined courtyard bounding box overlaps aBox, computed by linear scan.
std::set<FOOTPRINT*> bruteForceOverlap( BOARD& aBoard, const BOX2I& aBox )
{
    std::set<FOOTPRINT*> result;

    for( FOOTPRINT* footprint : aBoard.Footprints() )
    {
        BOX2I bbox;
        bool  hasCourtyard = false;

        for( PCB_LAYER_ID side : { F_Cu, B_Cu } )
        {
            const SHAPE_POLY_SET& courtyard = footprint->GetCourtyard( side );

            if( courtyard.OutlineCount() == 0 )
                continue;

            if( hasCourtyard )
                bbox.Merge( courtyard.BBox() );
            else
                bbox = courtyard.BBox();

            hasCourtyard = true;
        }

        if( hasCourtyard && bbox.Intersects( aBox ) )
            result.insert( footprint );
    }

    return result;
}


std::set<FOOTPRINT*> indexOverlap( const FOOTPRINT_COURTYARD_INDEX& aIndex, const BOX2I& aBox )
{
    std::set<FOOTPRINT*> result;

    aIndex.QueryOverlapping( aBox,
            [&]( FOOTPRINT* aFootprint ) -> bool
            {
                result.insert( aFootprint );
                return true;
            } );

    return result;
}

} // namespace


BOOST_AUTO_TEST_SUITE( FootprintCourtyardIndex )


BOOST_AUTO_TEST_CASE( MatchesBruteForceScan )
{
    BOARD board;

    addFootprint( board, "U1", { 0, 0 }, { mm( 2 ), mm( 2 ) } );
    addFootprint( board, "U2", { mm( 50 ), 0 }, { mm( 2 ), mm( 2 ) } );
    addFootprint( board, "U3", { mm( 100 ), mm( 100 ) }, { mm( 2 ), mm( 2 ) } );

    // A footprint whose courtyard is far larger than its body: indexing the body bounds instead
    // of the courtyard bounds would wrongly exclude it from far-reaching queries.
    addFootprint( board, "U4", { 0, mm( 50 ) }, { mm( 60 ), mm( 60 ) } );

    // A footprint with no courtyard graphics must never be returned.
    std::unique_ptr<FOOTPRINT> noCourtyard = std::make_unique<FOOTPRINT>( &board );
    noCourtyard->SetReference( "U5" );
    noCourtyard->SetPosition( { mm( 50 ), mm( 50 ) } );
    board.Add( noCourtyard.release() );

    FOOTPRINT_COURTYARD_INDEX index( &board );

    const std::vector<BOX2I> queries = {
        BOX2I( VECTOR2I( mm( -5 ), mm( -5 ) ), VECTOR2I( mm( 10 ), mm( 10 ) ) ),   // around U1 only
        BOX2I( VECTOR2I( mm( 200 ), mm( 200 ) ), VECTOR2I( mm( 10 ), mm( 10 ) ) ), // empty region
        BOX2I( VECTOR2I( mm( 20 ), mm( 40 ) ), VECTOR2I( mm( 5 ), mm( 5 ) ) ),     // only U4's large courtyard
        BOX2I( VECTOR2I( mm( -50 ), mm( -50 ) ), VECTOR2I( mm( 250 ), mm( 250 ) ) ) // everything
    };

    for( const BOX2I& query : queries )
        BOOST_CHECK( indexOverlap( index, query ) == bruteForceOverlap( board, query ) );
}


BOOST_AUTO_TEST_SUITE_END()
