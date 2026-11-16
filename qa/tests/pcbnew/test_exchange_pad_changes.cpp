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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>

#include <board.h>
#include <footprint.h>
#include <footprint_utils.h>


struct EXCHANGE_PAD_CHANGES_FIXTURE
{
    std::unique_ptr<FOOTPRINT> load( const wxString& aName )
    {
        std::unique_ptr<FOOTPRINT> fp = KI_TEST::ReadFootprintFromFile(
                KI_TEST::GetPcbnewTestDataDir() + "exchange_pad_changes.pretty", aName );

        BOOST_REQUIRE( fp );
        return fp;
    }

    // Placement on the board must never read as a pad change
    std::unique_ptr<FOOTPRINT> placeOnBack( const FOOTPRINT& aLibrary )
    {
        std::unique_ptr<FOOTPRINT> placed( static_cast<FOOTPRINT*>( aLibrary.Clone() ) );
        placed->SetParent( &m_board );
        placed->SetPosition( VECTOR2I( 25400000, 12700000 ) );
        placed->SetOrientation( ANGLE_90 );
        placed->Flip( placed->GetPosition(), FLIP_DIRECTION::LEFT_RIGHT );
        return placed;
    }

    BOARD m_board;
};


BOOST_FIXTURE_TEST_SUITE( ExchangePadChanges, EXCHANGE_PAD_CHANGES_FIXTURE )


// Rev 1 is the campaign's acme footprint; Rev 2 is the stock P7.62 variant with pad 2 moved
BOOST_AUTO_TEST_CASE( PadMoveIsCounted )
{
    std::unique_ptr<FOOTPRINT> rev1 = load( wxS( "R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal" ) );
    std::unique_ptr<FOOTPRINT> rev2 = load( wxS( "R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal" ) );
    std::unique_ptr<FOOTPRINT> placed = placeOnBack( *rev1 );

    BOOST_TEST( !ComparePads( *placed, *rev1 ).Any() );

    PAD_CHANGE_SUMMARY changes = ComparePads( *placed, *rev2 );
    BOOST_TEST( changes.moved == 1 );
    BOOST_TEST( changes.added == 0 );
    BOOST_TEST( changes.removed == 0 );
}


// Thermal vias reuse the exposed pad's number and the paste pads have none
BOOST_AUTO_TEST_CASE( SharedPadNumbersPairByPosition )
{
    std::unique_ptr<FOOTPRINT> plain = load( wxS( "HSOP-8-1EP_3.9x4.9mm_P1.27mm_EP2.3x2.3mm" ) );
    std::unique_ptr<FOOTPRINT> vias = load( wxS( "HSOP-8-1EP_3.9x4.9mm_P1.27mm_EP2.3x2.3mm_ThermalVias" ) );
    std::unique_ptr<FOOTPRINT> placed = placeOnBack( *plain );

    BOOST_TEST( !ComparePads( *placed, *plain ).Any() );

    PAD_CHANGE_SUMMARY changes = ComparePads( *placed, *vias );
    BOOST_TEST( changes.moved == 0 );
    BOOST_TEST( changes.added == 5 );
    BOOST_TEST( changes.removed == 0 );
}


BOOST_AUTO_TEST_SUITE_END()
