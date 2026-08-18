/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <boost/test/unit_test.hpp>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <footprint.h>
#include <pcb_shape.h>

#include <line_ending.h>
#include <pcbnew_utils/board_file_utils.h>
#include <pcbnew_utils/board_test_utils.h>

#include <filesystem>


BOOST_AUTO_TEST_SUITE( PcbLineEndingIo )


namespace
{

void checkEnding( const LINE_ENDING& aEnding, LINE_ENDING_STYLE aStyle, int aLength, int aWidth,
                  int aStrokeWidth )
{
    BOOST_CHECK_EQUAL( static_cast<int>( aEnding.GetStyle() ), static_cast<int>( aStyle ) );
    BOOST_CHECK_EQUAL( aEnding.GetLength(), aLength );
    BOOST_CHECK_EQUAL( aEnding.GetWidth(), aWidth );
    BOOST_CHECK_EQUAL( aEnding.GetStrokeWidth(), aStrokeWidth );
}

PCB_SHAPE* findFirstSegment( FOOTPRINT* aFootprint )
{
    for( BOARD_ITEM* item : aFootprint->GraphicalItems() )
    {
        PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( item );

        if( shape && shape->GetShape() == SHAPE_T::SEGMENT )
            return shape;
    }

    return nullptr;
}


} // namespace


BOOST_AUTO_TEST_CASE( FootprintLineEndingsRoundTrip )
{
    FOOTPRINT footprint( nullptr );
    footprint.SetReference( wxS( "U1" ) );
    footprint.SetValue( wxS( "LineEndingFootprint" ) );

    auto* line = new PCB_SHAPE( &footprint );
    line->SetShape( SHAPE_T::SEGMENT );
    line->SetStart( VECTOR2I( 0, 0 ) );
    line->SetEnd( VECTOR2I( 1000000, 0 ) );
    line->SetLayer( F_SilkS );
    line->SetWidth( 100000 );

    line->SetStartEndingStyle( LINE_ENDING_STYLE::SQUARE );
    line->SetStartEndingLength( 300000 );
    line->SetStartEndingWidth( 200000 );
    line->SetStartEndingStrokeWidth( 50000 );

    line->SetEndEndingStyle( LINE_ENDING_STYLE::ARROW_OPEN );
    line->SetEndEndingLength( 500000 );
    line->SetEndEndingWidth( 400000 );
    line->SetEndEndingStrokeWidth( 100000 );

    footprint.Add( line, ADD_MODE::APPEND, true );

    KI_TEST::TEMPORARY_DIRECTORY tempLib( "kicad_qa_fp_line_ending_io", ".pretty" );
    const std::filesystem::path  savePath = tempLib.GetPath() / "line_ending_roundtrip.kicad_mod";

    KI_TEST::DumpFootprintToFile( footprint, savePath.string() );
    std::unique_ptr<FOOTPRINT> loadedFootprint =
            KI_TEST::ReadFootprintFromFileOrStream( savePath.string() );

    BOOST_REQUIRE( loadedFootprint );

    PCB_SHAPE* loadedLine = findFirstSegment( loadedFootprint.get() );
    BOOST_REQUIRE( loadedLine );
    checkEnding( loadedLine->GetStartEnding(), LINE_ENDING_STYLE::SQUARE, 300000, 200000, 50000 );
    checkEnding( loadedLine->GetEndEnding(), LINE_ENDING_STYLE::ARROW_OPEN, 500000, 400000, 100000 );
}


BOOST_AUTO_TEST_CASE( FootprintLineEndingEqualLengthWidthRoundTrip )
{
    FOOTPRINT footprint( nullptr );
    footprint.SetReference( wxS( "U1" ) );
    footprint.SetValue( wxS( "LineEndingEqualSizeFootprint" ) );

    auto* line = new PCB_SHAPE( &footprint );
    line->SetShape( SHAPE_T::SEGMENT );
    line->SetStart( VECTOR2I( 0, 0 ) );
    line->SetEnd( VECTOR2I( 1000000, 0 ) );
    line->SetLayer( F_SilkS );
    line->SetWidth( 100000 );

    line->SetStartEndingStyle( LINE_ENDING_STYLE::SQUARE );
    line->SetStartEndingLength( 300000 );
    line->SetStartEndingWidth( 300000 );

    footprint.Add( line, ADD_MODE::APPEND, true );

    KI_TEST::TEMPORARY_DIRECTORY tempLib( "kicad_qa_fp_line_ending_equal_size_io", ".pretty" );
    const std::filesystem::path  savePath = tempLib.GetPath() / "line_ending_equal_size.kicad_mod";

    KI_TEST::DumpFootprintToFile( footprint, savePath.string() );
    std::unique_ptr<FOOTPRINT> loadedFootprint =
            KI_TEST::ReadFootprintFromFileOrStream( savePath.string() );

    BOOST_REQUIRE( loadedFootprint );

    PCB_SHAPE* loadedLine = findFirstSegment( loadedFootprint.get() );
    BOOST_REQUIRE( loadedLine );
    checkEnding( loadedLine->GetStartEnding(), LINE_ENDING_STYLE::SQUARE, 300000, 300000, 0 );
}


BOOST_AUTO_TEST_SUITE_END()
