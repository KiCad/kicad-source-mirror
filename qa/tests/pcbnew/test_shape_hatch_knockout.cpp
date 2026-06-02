/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pcbnew_utils/board_construction_utils.h>
#include <board.h>
#include <footprint.h>
#include <pcb_shape.h>
#include <geometry/shape_poly_set.h>

/**
 * Tests for PCB_SHAPE::getHatchingKnockouts().
 *
 * A hatched shape subtracts same-layer graphics from its fill so the pattern does not clash
 * with them. The footprint courtyard must only be subtracted when the shape is itself on a
 * courtyard layer. Regression coverage for
 * https://gitlab.com/kicad/code/kicad/-/issues/24488 where a hatched shape on a User layer
 * subtracted the front courtyard, leaving holes in the pattern.
 */

namespace
{
/// Promotes the protected getHatchingKnockouts() to public so it can be tested directly.
class HATCH_KNOCKOUT_SHAPE : public PCB_SHAPE
{
public:
    using PCB_SHAPE::PCB_SHAPE;

    SHAPE_POLY_SET CallGetHatchingKnockouts() const { return getHatchingKnockouts(); }
};


/// Build a board with one footprint carrying a 10x10mm front courtyard centred on the origin.
std::unique_ptr<BOARD> makeBoardWithCourtyard()
{
    std::unique_ptr<BOARD>     board = std::make_unique<BOARD>();
    std::unique_ptr<FOOTPRINT> footprint = std::make_unique<FOOTPRINT>( board.get() );

    const VECTOR2I size( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) );
    const int      width = pcbIUScale.mmToIU( 0.1 );

    KI_TEST::DrawRect( *footprint, VECTOR2I( 0, 0 ), size, 0, width, F_CrtYd );

    footprint->SetReference( "U1" );
    footprint->SetPosition( VECTOR2I( 0, 0 ) );

    board->Add( footprint.release() );
    return board;
}


/// Add a hatch-filled rectangle covering the courtyard area on the requested layer.
HATCH_KNOCKOUT_SHAPE* addHatchedRect( BOARD& aBoard, PCB_LAYER_ID aLayer )
{
    HATCH_KNOCKOUT_SHAPE* shape = new HATCH_KNOCKOUT_SHAPE( &aBoard, SHAPE_T::RECTANGLE );

    shape->SetLayer( aLayer );
    shape->SetStart( VECTOR2I( pcbIUScale.mmToIU( -20 ), pcbIUScale.mmToIU( -20 ) ) );
    shape->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 20 ), pcbIUScale.mmToIU( 20 ) ) );
    shape->SetStroke( STROKE_PARAMS( pcbIUScale.mmToIU( 0.2 ), LINE_STYLE::SOLID ) );
    shape->SetFillMode( FILL_T::HATCH );

    aBoard.Add( shape );
    return shape;
}
} // namespace


BOOST_AUTO_TEST_SUITE( PcbShapeHatchKnockout )

/**
 * A hatched shape on a User layer produces no knockouts, since the only same-layer item would
 * be the front courtyard and that must not be subtracted from a non-courtyard layer.
 */
BOOST_AUTO_TEST_CASE( UserLayerIgnoresCourtyard )
{
    std::unique_ptr<BOARD> board = makeBoardWithCourtyard();
    HATCH_KNOCKOUT_SHAPE*  shape = addHatchedRect( *board, User_1 );

    SHAPE_POLY_SET knockouts = shape->CallGetHatchingKnockouts();

    BOOST_CHECK_EQUAL( knockouts.OutlineCount(), 0 );
}


/**
 * A hatched shape on the courtyard layer subtracts the footprint courtyard, so the knockouts
 * contain the courtyard region.
 */
BOOST_AUTO_TEST_CASE( CourtyardLayerKnocksOutCourtyard )
{
    std::unique_ptr<BOARD> board = makeBoardWithCourtyard();
    HATCH_KNOCKOUT_SHAPE*  shape = addHatchedRect( *board, F_CrtYd );

    SHAPE_POLY_SET knockouts = shape->CallGetHatchingKnockouts();

    BOOST_CHECK_GT( knockouts.OutlineCount(), 0 );
    BOOST_CHECK_GT( knockouts.Area(), 0.0 );
}

BOOST_AUTO_TEST_SUITE_END()
