/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <pcbnew_utils/board_test_utils.h>

#include <board.h>
#include <diff_merge/fp_digest.h>
#include <footprint.h>
#include <netinfo.h>
#include <pad.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>
#include <settings/settings_manager.h>

#include <wx/filename.h>


using namespace KICAD_DIFF;


namespace
{
wxString libraryPath()
{
    wxFileName fn( KI_TEST::GetPcbnewTestDataDir(), wxEmptyString );
    fn.RemoveLastDir();
    fn.AppendDir( wxS( "libraries" ) );
    fn.AppendDir( wxS( "Resistor_SMD.pretty" ) );
    return fn.GetPath();
}


std::unique_ptr<FOOTPRINT> loadLibraryFootprint( const wxString& aName )
{
    PCB_IO_KICAD_SEXPR io;
    return io.FootprintLoad( libraryPath(), aName );
}


struct BOARD_FIXTURE
{
    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};
} // namespace


BOOST_AUTO_TEST_SUITE( FpDigest )


// Placing a footprint gives it a position, a side, a reference, nets and overrides that DRC's
// library parity check ignores, so the digest ignores them too
BOOST_FIXTURE_TEST_CASE( PlacedCopyDigestsAsItsLibraryFootprint, BOARD_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue22267", m_board );

    // The writer takes polygon points from board coordinates, so a rotation left in place shows
    std::unique_ptr<FOOTPRINT> library = KI_TEST::ReadFootprintFromFile(
            KI_TEST::GetPcbnewTestDataDir() + "issue24045/QFN-24_L4.0-W4.0-P0.50-BL-EP2.6.kicad_mod",
            wxS( "QFN-24_L4.0-W4.0-P0.50-BL-EP2.6" ) );
    BOOST_REQUIRE( library );

    const wxString libraryDigest = FootprintDigest( *library );
    BOOST_CHECK( libraryDigest.StartsWith( wxS( "v1:" ) ) );

    FOOTPRINT* placed = static_cast<FOOTPRINT*>( library->Duplicate( false ) );
    m_board->Add( placed );

    placed->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 71.3 ), pcbIUScale.mmToIU( 64.2 ) ) );
    placed->SetOrientation( ANGLE_90 );
    placed->Flip( placed->GetPosition(), FLIP_DIRECTION::TOP_BOTTOM );
    placed->SetReference( wxS( "R99" ) );
    placed->SetValue( wxS( "4k7" ) );
    placed->SetDNP( true );
    placed->SetLocked( true );
    placed->Pads().front()->SetNet( m_board->FindNet( 1 ) );
    placed->Pads().front()->SetLocalClearance( pcbIUScale.mmToIU( 0.3 ) );
    placed->SetLocalSolderMaskMargin( pcbIUScale.mmToIU( 0.05 ) );

    BOOST_CHECK( !placed->FootprintNeedsUpdate( library.get(), BOARD_ITEM::COMPARE_FLAGS::DRC ) );
    BOOST_CHECK_EQUAL( FootprintDigest( *placed ), libraryDigest );

    placed->Pads().front()->SetSize( PADSTACK::ALL_LAYERS,
                                     VECTOR2I( pcbIUScale.mmToIU( 1.2 ), pcbIUScale.mmToIU( 1.0 ) ) );

    BOOST_CHECK( placed->FootprintNeedsUpdate( library.get(), BOARD_ITEM::COMPARE_FLAGS::DRC ) );
    BOOST_CHECK_NE( FootprintDigest( *placed ), libraryDigest );
}


// A release digest stands in for the parity check, so the two agree on footprints placed in real boards
BOOST_FIXTURE_TEST_CASE( DigestAgreesWithLibraryParity, BOARD_FIXTURE )
{
    int same = 0;

    for( const char* boardName : { "issue22267", "issue22475/issue22475", "issue23550/issue23550", "issue5313" } )
    {
        KI_TEST::LoadBoard( m_settingsManager, boardName, m_board );

        for( FOOTPRINT* placed : m_board->Footprints() )
        {
            if( placed->GetFPID().GetLibNickname() != "Resistor_SMD" )
                continue;

            std::unique_ptr<FOOTPRINT> library = loadLibraryFootprint( placed->GetFPID().GetLibItemName() );

            if( !library )
                continue;

            bool needsUpdate = placed->FootprintNeedsUpdate( library.get(), BOARD_ITEM::COMPARE_FLAGS::DRC );

            // Nothing here sits off a quarter turn, so DRC's tolerance never applies and both agree both ways
            BOOST_TEST_INFO( boardName << " " << placed->GetReference() );
            BOOST_CHECK_EQUAL( FootprintDigest( *placed ) == FootprintDigest( *library ), !needsUpdate );

            if( !needsUpdate )
                same++;
        }
    }

    BOOST_CHECK_GT( same, 0 );
}


BOOST_AUTO_TEST_SUITE_END()
