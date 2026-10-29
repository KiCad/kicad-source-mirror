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

#include <qa_utils/file_utils.h>

#include <board.h>
#include <kiid.h>
#include <footprint.h>
#include <board_design_settings.h>
#include <pcb_field.h>
#include <pcb_text.h>
#include <pcbnew_utils/board_file_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <settings/settings_manager.h>

#include <filesystem>


namespace
{

struct FOOTPRINT_LOAD_TEST_CASE
{
    // Which footprint to look at in the file
    KIID     m_footprintUuid;
    // Expected values
    bool     m_expectedLocked;
    VECTOR2I m_expectedPos;
};


struct FOOTPRINT_LOAD_BOARD_TEST_CASE: public KI_TEST::BOARD_LOAD_TEST_CASE
{
    // List of images to check
    std::vector<FOOTPRINT_LOAD_TEST_CASE> m_fpCases;
};


const std::vector<FOOTPRINT_LOAD_BOARD_TEST_CASE> FootprintLoadSave_testCases{
    {
            "footprints_load_save",
            std::nullopt,
            {
                    // From top to bottom in the board file
                    {
                            "898cf321-03c7-40bb-8d78-4bc5e52986c2",
                            false,
                            { 100, 80 },
                    },
                    {
                            "0775cd70-2e84-4592-a160-456c37a8f4f6",
                            true,
                            { 100, 100 },
                    },
            },
    },
};

} // namespace

/**
 * Simple tests cases that load a board file and check expected properties of the footprints.
 * This is not the same as FpLibLoadSave as it loads _board files_, and not footprint
 * files from a library.
 */
BOOST_DATA_TEST_CASE( FootprintLoadSave,
                      boost::unit_test::data::make( FootprintLoadSave_testCases ), testCase )
{
    const auto doBoardTest = [&]( const BOARD& aBoard )
    {
        for( const FOOTPRINT_LOAD_TEST_CASE& fooprintTestCase : testCase.m_fpCases )
        {
            BOOST_TEST_MESSAGE( "Checking for footprint with UUID: "
                                << fooprintTestCase.m_footprintUuid.AsString() );

            const auto& fp = static_cast<FOOTPRINT&>( KI_TEST::RequireBoardItemWithTypeAndId(
                    aBoard, PCB_FOOTPRINT_T, fooprintTestCase.m_footprintUuid ) );

            BOOST_CHECK_EQUAL( fp.IsLocked(), fooprintTestCase.m_expectedLocked );
            BOOST_CHECK_EQUAL( fp.GetPosition(), fooprintTestCase.m_expectedPos * 1000000 );
        }
    };

    KI_TEST::LoadAndTestBoardFile( testCase.m_BoardFileRelativePath, true, doBoardTest,
                                   testCase.m_ExpectedBoardVersion );
}


/**
 * Fields are written without a header of their own, inside the footprint's (property ...), so
 * their lock takes a different path through the writer than a plain footprint text's does.
 */
BOOST_AUTO_TEST_CASE( FootprintFieldLockRoundTrip )
{
    BOARD board;

    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 20 ), 0 ) );
    fp->SetReference( wxS( "U1" ) );
    fp->GetField( FIELD_T::REFERENCE )->SetLocked( true );

    PCB_FIELD* userField = new PCB_FIELD( fp, FIELD_T::USER, wxS( "Label" ) );
    userField->SetText( wxS( "BezelSW0" ) );
    userField->SetLayer( F_SilkS );
    userField->SetLocked( true );
    fp->Add( userField );

    // A plain footprint text is the control: it has always round-tripped its lock
    PCB_TEXT* fpText = new PCB_TEXT( fp );
    fpText->SetText( wxS( "fp text" ) );
    fpText->SetLayer( F_SilkS );
    fpText->SetLocked( true );
    fp->Add( fpText );

    board.Add( fp );

    KI_TEST::SCOPED_TEMP_DIR    tempDir( "kicad_qa_field_lock" );
    const std::filesystem::path path = tempDir.Path() / "field_lock.kicad_pcb";

    KI_TEST::DumpBoardToFile( board, path );
    std::unique_ptr<BOARD> restored = KI_TEST::ReadBoardFromFileOrStream( path.string() );
    BOOST_REQUIRE( restored );
    BOOST_REQUIRE_EQUAL( restored->Footprints().size(), 1 );

    FOOTPRINT* loaded = restored->Footprints().front();

    BOOST_CHECK( loaded->GetField( FIELD_T::REFERENCE )->IsLocked() );
    BOOST_REQUIRE( loaded->GetField( wxS( "Label" ) ) );
    BOOST_CHECK( loaded->GetField( wxS( "Label" ) )->IsLocked() );

    for( BOARD_ITEM* item : loaded->GraphicalItems() )
    {
        if( item->Type() == PCB_TEXT_T )
            BOOST_CHECK( item->IsLocked() );
    }
}


/**
 * A field with no thickness token is using auto thickness. Loading one builds the field before
 * its effects are parsed, so nothing stamped on it then may survive the parse.
 */
BOOST_AUTO_TEST_CASE( FootprintUserFieldKeepsAutoThickness )
{
    BOARD board;

    // Auto thickness only differs from the board default when that default is not zero
    board.GetDesignSettings().m_TextThickness[LAYER_CLASS_SILK] = pcbIUScale.mmToIU( 0.15 );

    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 20 ), 0 ) );
    fp->SetReference( wxS( "U1" ) );

    PCB_FIELD* userField = new PCB_FIELD( fp, FIELD_T::USER, wxS( "Label" ) );
    userField->SetText( wxS( "BezelSW0" ) );
    userField->SetLayer( F_SilkS );
    userField->SetAutoThickness( true );
    fp->Add( userField );

    board.Add( fp );

    BOOST_REQUIRE( userField->GetAutoThickness() );

    KI_TEST::SCOPED_TEMP_DIR    tempDir( "kicad_qa_auto_thickness" );
    const std::filesystem::path path = tempDir.Path() / "auto_thickness.kicad_pcb";

    KI_TEST::DumpBoardToFile( board, path );
    std::unique_ptr<BOARD> restored = KI_TEST::ReadBoardFromFileOrStream( path.string() );
    BOOST_REQUIRE( restored );
    BOOST_REQUIRE_EQUAL( restored->Footprints().size(), 1 );

    PCB_FIELD* loaded = restored->Footprints().front()->GetField( wxS( "Label" ) );
    BOOST_REQUIRE( loaded );
    BOOST_CHECK( loaded->GetAutoThickness() );
}


/**
 * The other side of the same call: a legacy property carries no effects at all, so the board
 * defaults are the only style it can get.
 */
BOOST_AUTO_TEST_CASE( LegacyFootprintPropertyTakesBoardDefaults )
{
    const auto doBoardTest = [&]( const BOARD& aBoard )
    {
        BOOST_REQUIRE( !aBoard.Footprints().empty() );

        PCB_FIELD* field = aBoard.Footprints().front()->GetField( wxS( "myfield" ) );

        BOOST_REQUIRE( field );
        BOOST_CHECK( !field->GetAutoThickness() );
        BOOST_CHECK_EQUAL( field->GetTextThickness(),
                           aBoard.GetDesignSettings().GetTextThickness( field->GetLayer() ) );
    };

    // Saving upgrades the file, so the roundtrip also covers the field once it has its own effects
    KI_TEST::LoadAndTestBoardFile( "custom_fields", true, doBoardTest );
}
