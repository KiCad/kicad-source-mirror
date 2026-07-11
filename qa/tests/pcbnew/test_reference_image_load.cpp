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
#include <boost/test/data/test_case.hpp>

#include <board.h>
#include <kiid.h>
#include <layer_ids.h>
#include <pcb_reference_image.h>
#include <reference_image.h>
#include <bitmap_base.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr_parser.h>
#include <pcbnew_utils/board_file_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <richio.h>
#include <settings/settings_manager.h>

#include <filesystem>
#include <fstream>
#include <sstream>

namespace
{

struct REFERENCE_IMAGE_LOAD_TEST_FIXTURE
{
    REFERENCE_IMAGE_LOAD_TEST_FIXTURE() {}
};


struct REFERENCE_IMAGE_LOAD_TEST_CASE
{
    // Which one to look at in the file
    KIID     m_imageUuid;
    // Expected values
    bool     m_expectedLocked;
    VECTOR2I m_expectedPos;
    double   m_expectedScale;
    // This should also check correct image decoding, as it won't work otherwise
    VECTOR2I m_expectedPixelSize;
};


struct REFERENCE_IMAGE_LOAD_BOARD_TEST_CASE: public KI_TEST::BOARD_LOAD_TEST_CASE
{
    // List of images to check
    std::vector<REFERENCE_IMAGE_LOAD_TEST_CASE> m_imageCases;
};

const std::vector<REFERENCE_IMAGE_LOAD_BOARD_TEST_CASE> ReferenceImageLoading_testCases{
    {
            "reference_images_load_save",
            std::nullopt,
            {
                    // From top to bottom in the board file
                    {
                            "7dde345e-020a-4fdd-af77-588b452be5e0",
                            false,
                            { 100, 46 },
                            1.0,
                            { 64, 64 },
                    },
                    {
                            "e4fd52dd-1d89-4c43-b621-aebfc9788d5c",
                            true,
                            { 100, 65 },
                            1.0,
                            { 64, 64 },
                    },
                    {
                            "d402397e-bce0-4cae-a398-b5aeef397e87",
                            false,
                            { 100, 90 },
                            2.0,
                            { 64, 64 }, // It's the same size, but scaled
                    },
            },
    },
};

// Minimal 1x1 RGB PNG with a pHYs chunk of 3780 pixels/meter in both axes.
// 3780 PPM -> 37.8 px/cm -> 37.8 * 2.54 = 96.012 -> 96 PPI, but the pre-fix code truncated
// "37.8" to 37 -> 94 PPI.
const std::vector<unsigned char> png_3780_ppm = {
    0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x02, 0x00, 0x00, 0x00, 0x90, 0x77, 0x53,
    0xDE, 0x00, 0x00, 0x00, 0x09, 0x70, 0x48, 0x59, 0x73, 0x00, 0x00, 0x0E, 0xC4, 0x00, 0x00, 0x0E,
    0xC4, 0x01, 0x95, 0x2B, 0x0E, 0x1B, 0x00, 0x00, 0x00, 0x0C, 0x49, 0x44, 0x41, 0x54, 0x78, 0xDA,
    0x63, 0xF8, 0xCF, 0xC0, 0x00, 0x00, 0x03, 0x01, 0x01, 0x00, 0xF7, 0x03, 0x41, 0x43, 0x00, 0x00,
    0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82
};


std::unique_ptr<BOARD> parseBoardString( const std::string& aData )
{
    STRING_LINE_READER        reader( aData, wxT( "migration test board" ) );
    PCB_IO_KICAD_SEXPR_PARSER parser( &reader, nullptr, nullptr );

    return std::unique_ptr<BOARD>( dynamic_cast<BOARD*>( parser.Parse() ) );
}


double firstImageScale( const BOARD& aBoard )
{
    for( BOARD_ITEM* item : aBoard.Drawings() )
    {
        if( item->Type() == PCB_REFERENCE_IMAGE_T )
            return static_cast<PCB_REFERENCE_IMAGE*>( item )->GetReferenceImage().GetImageScale();
    }

    return -1.0;
}

} // namespace


BOOST_DATA_TEST_CASE_F( REFERENCE_IMAGE_LOAD_TEST_FIXTURE, ReferenceImageLoading,
                        boost::unit_test::data::make( ReferenceImageLoading_testCases ), testCase )
{
    const auto doBoardTest = [&]( const BOARD& aBoard )
    {
        for( const REFERENCE_IMAGE_LOAD_TEST_CASE& imageTestCase : testCase.m_imageCases )
        {
            BOOST_TEST_MESSAGE(
                    "Checking for image with UUID: " << imageTestCase.m_imageUuid.AsString() );

            const auto& image =
                    static_cast<PCB_REFERENCE_IMAGE&>( KI_TEST::RequireBoardItemWithTypeAndId(
                            aBoard, PCB_REFERENCE_IMAGE_T, imageTestCase.m_imageUuid ) );
            const REFERENCE_IMAGE& refImage = image.GetReferenceImage();

            BOOST_CHECK_EQUAL( image.IsLocked(), imageTestCase.m_expectedLocked );
            BOOST_CHECK_EQUAL( image.GetPosition(), imageTestCase.m_expectedPos * 1000000 );
            BOOST_CHECK_CLOSE( refImage.GetImageScale(), imageTestCase.m_expectedScale, 1e-6 );

            const BITMAP_BASE& bitmap = refImage.GetImage();

            BOOST_CHECK_EQUAL( bitmap.GetSizePixels(), imageTestCase.m_expectedPixelSize );
        }
    };

    KI_TEST::LoadAndTestBoardFile( testCase.m_BoardFileRelativePath, true, doBoardTest,
                                   testCase.m_ExpectedBoardVersion );
}


/**
 * Test that flipping a reference image changes its associated layer, matching
 * the behavior of all other PCB item types.
 */
BOOST_FIXTURE_TEST_CASE( ReferenceImageFlipLayer, REFERENCE_IMAGE_LOAD_TEST_FIXTURE )
{
    const auto doBoardTest = [&]( const BOARD& aBoard )
    {
        KIID targetUuid( "7dde345e-020a-4fdd-af77-588b452be5e0" );

        auto& image = static_cast<PCB_REFERENCE_IMAGE&>(
                KI_TEST::RequireBoardItemWithTypeAndId( aBoard, PCB_REFERENCE_IMAGE_T,
                                                        targetUuid ) );

        BOOST_REQUIRE( image.GetReferenceImage().GetImage().GetSizePixels().x > 0 );

        PCB_LAYER_ID origLayer = image.GetLayer();
        VECTOR2I     origPos = image.GetPosition();

        // Flip left-right and verify layer changes
        image.Flip( origPos, FLIP_DIRECTION::LEFT_RIGHT );

        PCB_LAYER_ID flippedLayer = aBoard.FlipLayer( origLayer );
        BOOST_CHECK_EQUAL( image.GetLayer(), flippedLayer );

        // Position should stay the same since we flipped around it
        BOOST_CHECK_EQUAL( image.GetPosition(), origPos );

        // Flip again to return to original state
        image.Flip( origPos, FLIP_DIRECTION::LEFT_RIGHT );

        BOOST_CHECK_EQUAL( image.GetLayer(), origLayer );
        BOOST_CHECK_EQUAL( image.GetPosition(), origPos );

        // Same test for top-bottom flip
        image.Flip( origPos, FLIP_DIRECTION::TOP_BOTTOM );

        BOOST_CHECK_EQUAL( image.GetLayer(), flippedLayer );
        BOOST_CHECK_EQUAL( image.GetPosition(), origPos );

        image.Flip( origPos, FLIP_DIRECTION::TOP_BOTTOM );

        BOOST_CHECK_EQUAL( image.GetLayer(), origLayer );
        BOOST_CHECK_EQUAL( image.GetPosition(), origPos );
    };

    KI_TEST::LoadAndTestBoardFile( "reference_images_load_save", true, doBoardTest );
}


/**
 * A board predating the PNG pixel-density fix stored an image scale that compensated for the
 * truncated PPI. Loading it must re-scale by the corrected/truncated PPI ratio so the rendered
 * size is preserved, while a current-version board is loaded verbatim.
 */
BOOST_FIXTURE_TEST_CASE( ReferenceImagePpiScaleMigration, REFERENCE_IMAGE_LOAD_TEST_FIXTURE )
{
    BOARD board;
    auto  image = std::make_unique<PCB_REFERENCE_IMAGE>( &board );

    wxMemoryBuffer buffer;
    buffer.AppendData( png_3780_ppm.data(), png_3780_ppm.size() );
    BOOST_REQUIRE( image->GetReferenceImage().ReadImageFile( buffer ) );

    BOOST_REQUIRE_EQUAL( image->GetReferenceImage().GetImage().GetPPI(), 96 );
    BOOST_REQUIRE_EQUAL( image->GetReferenceImage().GetImage().GetLegacyPPI(), 94 );

    image->GetReferenceImage().SetImageScale( 2.0 );
    image->SetLayer( F_Cu );
    board.Add( image.release() );

    const std::string path = ( std::filesystem::temp_directory_path()
                               / "issue23575_ppi_migration.kicad_pcb" ).string();
    KI_TEST::DumpBoardToFile( board, path );

    std::ifstream     in( path );
    std::stringstream ss;
    ss << in.rdbuf();
    const std::string current = ss.str();

    // Current format version: the stored scale is already correct, so it loads verbatim.
    std::unique_ptr<BOARD> reloaded = parseBoardString( current );
    BOOST_REQUIRE( reloaded );
    BOOST_CHECK_CLOSE( firstImageScale( *reloaded ), 2.0, 1e-6 );

    // Simulate a pre-fix file by lowering the format version below the migration cutoff.  Match
    // the live version rather than a literal so a later format bump does not break this test.
    std::string       legacy = current;
    const std::string version = std::to_string( SEXPR_BOARD_FILE_VERSION );
    const size_t      pos = legacy.find( version );
    BOOST_REQUIRE( pos != std::string::npos );
    legacy.replace( pos, version.size(), "20260512" );

    std::unique_ptr<BOARD> migrated = parseBoardString( legacy );
    BOOST_REQUIRE( migrated );
    BOOST_CHECK_CLOSE( firstImageScale( *migrated ), 2.0 * 96.0 / 94.0, 1e-3 );
}
