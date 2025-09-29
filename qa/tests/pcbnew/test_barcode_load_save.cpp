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

#include <filesystem>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <boost/test/unit_test.hpp>

#include <board.h>
#include <footprint.h>
#include <pcb_text.h>
#include <pcb_barcode.h>
#include <pcbnew_utils/board_file_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <settings/settings_manager.h>

BOOST_AUTO_TEST_CASE( BarcodeWriteRead )
{
    SETTINGS_MANAGER settingsManager( true );

    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    PCB_BARCODE* barcode = new PCB_BARCODE( board.get() );
    barcode->SetText( wxT( "12345" ) );
    barcode->SetLayer( F_SilkS );
    barcode->SetPosition( VECTOR2I( 1000000, 2000000 ) );
    barcode->SetWidth( 3000000 );
    barcode->SetHeight( 3000000 );
    barcode->SetTextSize( pcbIUScale.mmToIU( 1.5 ) );
    barcode->SetKind( BARCODE_T::QR_CODE );
    barcode->SetErrorCorrection( BARCODE_ECC_T::M );
    barcode->AssembleBarcode( true, true );

    const KIID id = barcode->m_Uuid;

    board->Add( barcode, ADD_MODE::APPEND, true );

    const std::filesystem::path savePath =
            std::filesystem::temp_directory_path() / "barcode_roundtrip.kicad_pcb";

    KI_TEST::DumpBoardToFile( *board, savePath.string() );
    std::unique_ptr<BOARD> board2 = KI_TEST::ReadBoardFromFileOrStream( savePath.string() );

    const auto& loaded = static_cast<PCB_BARCODE&>(
            KI_TEST::RequireBoardItemWithTypeAndId( *board2, PCB_BARCODE_T, id ) );

    BOOST_CHECK( loaded.GetText() == barcode->GetText() );
    BOOST_CHECK_EQUAL( static_cast<int>( loaded.GetKind() ), static_cast<int>( barcode->GetKind() ) );
    BOOST_CHECK_EQUAL( static_cast<int>( loaded.GetErrorCorrection() ),
                       static_cast<int>( barcode->GetErrorCorrection() ) );
    BOOST_CHECK_EQUAL( loaded.GetWidth(), barcode->GetWidth() );
    BOOST_CHECK_EQUAL( loaded.GetHeight(), barcode->GetHeight() );
    BOOST_CHECK_EQUAL( loaded.GetTextSize(), barcode->GetTextSize() );

    BOX2I bbox = loaded.GetPolyShape().BBox();
    BOOST_CHECK_EQUAL( bbox.Centre(), loaded.GetPosition() );
}


BOOST_AUTO_TEST_CASE( BarcodeFootprintWriteRead )
{
    SETTINGS_MANAGER settingsManager( true );

    FOOTPRINT footprint( nullptr );

    PCB_BARCODE* barcode = new PCB_BARCODE( &footprint );
    barcode->SetText( wxT( "12345" ) );
    barcode->SetLayer( F_SilkS );
    barcode->SetPosition( VECTOR2I( 1000000, 2000000 ) );
    barcode->SetWidth( 3000000 );
    barcode->SetHeight( 3000000 );
    barcode->SetTextSize( pcbIUScale.mmToIU( 1.5 ) );
    barcode->SetKind( BARCODE_T::QR_CODE );
    barcode->SetErrorCorrection( BARCODE_ECC_T::M );
    barcode->AssembleBarcode( true, true );

    const KIID id = barcode->m_Uuid;

    footprint.Add( barcode, ADD_MODE::APPEND, true );

    const std::filesystem::path savePath =
            std::filesystem::temp_directory_path() / "barcode_roundtrip.kicad_mod";

    KI_TEST::DumpFootprintToFile( footprint, savePath.string() );
    std::unique_ptr<FOOTPRINT> footprint2 =
            KI_TEST::ReadFootprintFromFileOrStream( savePath.string() );

    PCB_BARCODE* loaded = nullptr;

    for( BOARD_ITEM* item : footprint2->GraphicalItems() )
    {
        if( item->Type() == PCB_BARCODE_T && item->m_Uuid == id )
        {
            loaded = static_cast<PCB_BARCODE*>( item );
            break;
        }
    }

    BOOST_REQUIRE( loaded != nullptr );
    BOOST_CHECK( loaded->GetText() == barcode->GetText() );
    BOOST_CHECK_EQUAL( static_cast<int>( loaded->GetKind() ), static_cast<int>( barcode->GetKind() ) );
    BOOST_CHECK_EQUAL( static_cast<int>( loaded->GetErrorCorrection() ),
                       static_cast<int>( barcode->GetErrorCorrection() ) );
    BOOST_CHECK_EQUAL( loaded->GetWidth(), barcode->GetWidth() );
    BOOST_CHECK_EQUAL( loaded->GetHeight(), barcode->GetHeight() );
    BOOST_CHECK_EQUAL( loaded->GetTextSize(), barcode->GetTextSize() );

    BOX2I bbox = loaded->GetPolyShape().BBox();
    BOOST_CHECK_EQUAL( bbox.Centre(), loaded->GetPosition() );
}


BOOST_AUTO_TEST_CASE( BarcodePositioningAlignment )
{
    SETTINGS_MANAGER settingsManager( true );

    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    // Test multiple barcode types and positions to ensure consistent alignment
    struct TestCase
    {
        BARCODE_T kind;
        VECTOR2I position;
        int width;
        int height;
        bool withText;
        bool knockout;
        double angle;
        wxString text;
    };

    std::vector<TestCase> testCases = {
        // Basic QR codes at different positions
        { BARCODE_T::QR_CODE, VECTOR2I( 0, 0 ), 2000000, 2000000, false, false, 0.0, "TEST1" },
        { BARCODE_T::QR_CODE, VECTOR2I( 5000000, 3000000 ), 3000000, 3000000, false, false, 0.0, "TEST2" },
        { BARCODE_T::QR_CODE, VECTOR2I( -2000000, -1000000 ), 1500000, 1500000, false, false, 0.0, "TEST3" },

        // With text
        { BARCODE_T::QR_CODE, VECTOR2I( 1000000, 2000000 ), 2500000, 2500000, true, false, 0.0, "WITHTEXT" },

        // With knockout
        { BARCODE_T::QR_CODE, VECTOR2I( 2000000, 1000000 ), 2000000, 2000000, false, true, 0.0, "KNOCKOUT" },

        // With rotation
        { BARCODE_T::QR_CODE, VECTOR2I( 3000000, 2000000 ), 2000000, 2000000, false, false, 45.0, "ROTATED" },

        // Different barcode types
        { BARCODE_T::CODE_39, VECTOR2I( 4000000, 1000000 ), 3000000, 800000, false, false, 0.0, "CODE39TEST" },
        { BARCODE_T::CODE_128, VECTOR2I( 1000000, 4000000 ), 3500000, 1000000, false, false, 0.0, "CODE128" },
        { BARCODE_T::DATA_MATRIX, VECTOR2I( 3000000, 3000000 ), 1800000, 1800000, false, false, 0.0, "DATAMATRIX" },
        { BARCODE_T::MICRO_QR_CODE, VECTOR2I( 2000000, 4000000 ), 1200000, 1200000, false, false, 0.0, "µQR" },

        // Combined scenarios
        { BARCODE_T::QR_CODE, VECTOR2I( 1500000, 1500000 ), 2200000, 2200000, true, true, 90.0, "COMPLEX" },
    };

    for( size_t i = 0; i < testCases.size(); ++i )
    {
        const auto& tc = testCases[i];

        PCB_BARCODE* barcode = new PCB_BARCODE( board.get() );
        barcode->SetText( tc.text );
        barcode->SetLayer( F_SilkS );
        barcode->SetPosition( tc.position );
        barcode->SetWidth( tc.width );
        barcode->SetHeight( tc.height );
        barcode->SetKind( tc.kind );
        barcode->SetErrorCorrection( BARCODE_ECC_T::M );
        barcode->Text().SetVisible( tc.withText );
        barcode->SetIsKnockout( tc.knockout );

        if( tc.angle != 0.0 )
        {
            barcode->Rotate( tc.position, EDA_ANGLE( tc.angle, DEGREES_T ) );
        }

        barcode->AssembleBarcode( true, true );

        // Check that the polygon center matches the position
        BOX2I bbox = barcode->GetPolyShape().BBox();
        VECTOR2I actualCenter = bbox.GetCenter();
        VECTOR2I expectedCenter = barcode->GetPosition();

        BOOST_CHECK_MESSAGE( actualCenter == expectedCenter,
            "Test case " << i << " (" << tc.text.ToStdString() << "): "
            << "Expected center (" << expectedCenter.x << ", " << expectedCenter.y << ") "
            << "but got (" << actualCenter.x << ", " << actualCenter.y << ")" );

        board->Add( barcode, ADD_MODE::APPEND, true );
    }
}
