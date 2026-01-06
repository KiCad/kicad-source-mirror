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
#include <geometry/convex_hull.h>

BOOST_AUTO_TEST_CASE( BarcodeWriteRead )
{
    SETTINGS_MANAGER settingsManager;

    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    PCB_BARCODE* barcode = new PCB_BARCODE( board.get() );
    barcode->SetText( wxT( "12345" ) );
    barcode->SetLayer( F_SilkS );
    barcode->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 1.0 ), pcbIUScale.mmToIU( 2.0 ) ) );
    barcode->SetWidth( pcbIUScale.mmToIU( 3.0 ) );
    barcode->SetHeight( pcbIUScale.mmToIU( 3.0 ) );
    barcode->SetTextSize( pcbIUScale.mmToIU( 1.5 ) );
    barcode->SetKind( BARCODE_T::QR_CODE );
    barcode->SetErrorCorrection( BARCODE_ECC_T::M );
    barcode->AssembleBarcode();

    const KIID id = barcode->m_Uuid;

    board->Add( barcode, ADD_MODE::APPEND, true );

    const std::filesystem::path savePath = std::filesystem::temp_directory_path() / "barcode_roundtrip.kicad_pcb";

    std::filesystem::remove( savePath );
    KI_TEST::DumpBoardToFile( *board, savePath.string() );
    std::unique_ptr<BOARD> board2 = KI_TEST::ReadBoardFromFileOrStream( savePath.string() );
    BOARD_ITEM&            item2 = KI_TEST::RequireBoardItemWithTypeAndId( *board2, PCB_BARCODE_T, id );
    PCB_BARCODE&           loaded = static_cast<PCB_BARCODE&>( item2 );

    BOOST_CHECK( loaded.GetText() == barcode->GetText() );
    BOOST_CHECK_EQUAL( (int) loaded.GetKind(), (int) barcode->GetKind() );
    BOOST_CHECK_EQUAL( (int) loaded.GetErrorCorrection(), (int) barcode->GetErrorCorrection() );
    BOOST_CHECK_EQUAL( loaded.GetWidth(), barcode->GetWidth() );
    BOOST_CHECK_EQUAL( loaded.GetHeight(), barcode->GetHeight() );
    BOOST_CHECK_EQUAL( loaded.GetTextSize(), barcode->GetTextSize() );
    BOOST_CHECK_EQUAL( loaded.GetPolyShape().BBox().Centre(), barcode->GetPolyShape().BBox().Centre() );
}


BOOST_AUTO_TEST_CASE( BarcodeFootprintWriteRead )
{
    SETTINGS_MANAGER settingsManager;

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
    barcode->AssembleBarcode();

    const KIID id = barcode->m_Uuid;

    footprint.Add( barcode, ADD_MODE::APPEND, true );

    const std::filesystem::path savePath = std::filesystem::temp_directory_path() / "barcode_roundtrip.kicad_mod";

    std::filesystem::remove( savePath );
    KI_TEST::DumpFootprintToFile( footprint, savePath.string() );
    std::unique_ptr<FOOTPRINT> footprint2 = KI_TEST::ReadFootprintFromFileOrStream( savePath.string() );

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
    BOOST_CHECK_EQUAL( (int) loaded->GetKind(), (int) barcode->GetKind() );
    BOOST_CHECK_EQUAL( (int) loaded->GetErrorCorrection(), (int) barcode->GetErrorCorrection() );
    BOOST_CHECK_EQUAL( loaded->GetWidth(), barcode->GetWidth() );
    BOOST_CHECK_EQUAL( loaded->GetHeight(), barcode->GetHeight() );
    BOOST_CHECK_EQUAL( loaded->GetTextSize(), barcode->GetTextSize() );
    BOOST_CHECK_EQUAL( loaded->GetPolyShape().BBox().Centre(), barcode->GetPolyShape().BBox().Centre() );
}


BOOST_AUTO_TEST_CASE( BarcodePositioningAlignment )
{
    SETTINGS_MANAGER settingsManager;

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
        { BARCODE_T::MICRO_QR_CODE, VECTOR2I( 2000000, 4000000 ), 1200000, 1200000, false, false, 0.0, "microQR" },

        // Combined scenarios
        { BARCODE_T::QR_CODE, VECTOR2I( 1500000, 1500000 ), 2200000, 2200000, true, true, 90.0, "COMPLEX" },
    };

    for( size_t i = 0; i < testCases.size(); ++i )
    {
        const auto& tc = testCases[i];

        PCB_BARCODE* barcode = new PCB_BARCODE( board.get() );
        barcode->SetText( tc.text );
        barcode->Text().SetVisible( false );
        barcode->SetLayer( F_SilkS );
        barcode->SetWidth( tc.width );
        barcode->SetHeight( tc.height );
        barcode->SetKind( tc.kind );
        barcode->SetErrorCorrection( BARCODE_ECC_T::M );

        barcode->AssembleBarcode();
        SHAPE_POLY_SET canonicalPoly = barcode->GetPolyShape();

        barcode->SetPosition( tc.position );

        if( tc.angle != 0.0 )
            barcode->Rotate( tc.position, EDA_ANGLE( tc.angle, DEGREES_T ) );

        barcode->Text().SetVisible( tc.withText );
        barcode->SetIsKnockout( tc.knockout );

        barcode->AssembleBarcode();
        SHAPE_POLY_SET barcodePoly = barcode->GetPolyShape();

        // Barcode poly should completely cover canonical poly
        canonicalPoly.Rotate( barcode->GetAngle() );
        canonicalPoly.Move( barcode->GetPosition() );

        SHAPE_POLY_SET noHolesPoly;
        barcodePoly.Unfracture();

        for( int ii = 0; ii < barcodePoly.OutlineCount(); ++ii )
            noHolesPoly.AddOutline( barcodePoly.Outline( ii ) );

        // Handle rounding errors
        if( tc.angle != 0.0 )
            noHolesPoly.Inflate( 1, CORNER_STRATEGY::ROUND_ALL_CORNERS, ARC_LOW_DEF );

        canonicalPoly.BooleanSubtract( noHolesPoly );
        BOOST_CHECK_MESSAGE( canonicalPoly.IsEmpty(),
                             "Test case " << i << " (" << tc.text.ToStdString() << "): "
                             "barcode poly isn't aligned with canonical shape" );

        board->Add( barcode, ADD_MODE::APPEND, true );
    }
}
