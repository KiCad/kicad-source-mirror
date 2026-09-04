/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <qa_utils/file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <diff_merge/sym_digest.h>
#include <erc/erc_settings.h>
#include <json_common.h>
#include <mdw_placed_parts.h>
#include <project.h>
#include <project/project_file.h>
#include <sch_screen.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <settings/settings_manager.h>

#include <filesystem>
#include <fstream>
#include <set>
#include <tuple>


BOOST_AUTO_TEST_SUITE( MdwPlacedParts )


// Release Design reads placed parts from the saved schematic, so it needs no open editor
BOOST_AUTO_TEST_CASE( CollectFromSavedSchematicMatchesPlacedSymbols )
{
    SETTINGS_MANAGER           manager;
    std::unique_ptr<SCHEMATIC> editor;

    KI_TEST::LoadSchematic( manager, "../common/git/mdw_ctrl_board/ctrl-board", editor );

    PROJECT&            project = manager.Prj();
    SCHEMATIC_SETTINGS* editorSettings = project.GetProjectFile().m_SchematicSettings;
    const std::string   projectFile =
            KI_TEST::GetEeschemaTestDataDir() + "../common/git/mdw_ctrl_board/ctrl-board.kicad_pro";

    auto read = [&]()
    {
        std::ifstream in( projectFile, std::ios::binary );
        return std::string( std::istreambuf_iterator<char>( in ), std::istreambuf_iterator<char>() );
    };

    const std::string bytes = read();

    // An edit the user has not saved, which the borrowed schematic's settings must not replace
    project.GetProjectFile().m_ErcSettings->SetSeverity( ERCE_ENDPOINT_OFF_GRID, RPT_SEVERITY_IGNORE );
    std::multiset<std::tuple<std::string, std::string, std::string>> expected;
    std::multiset<std::tuple<std::string, std::string, std::string>> actual;

    // What the schematic editor's handler reported for the same design
    for( const SCH_SHEET_PATH& sheet : editor->Hierarchy() )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            expected.emplace( symbol->GetRef( &sheet ).utf8_string(),
                              symbol->GetLibId().Format().wx_str().utf8_string(),
                              KICAD_DIFF::PlacedSymbolDigest( *symbol ).utf8_string() );
        }
    }

    std::optional<std::string> collected = SchCollectPlacedParts(
            KI_TEST::GetEeschemaTestDataDir() + "../common/git/mdw_ctrl_board/ctrl-board.kicad_sch", project );

    BOOST_REQUIRE( collected );

    // Each load walks a screen's items in its own order
    for( const nlohmann::json& part : nlohmann::json::parse( *collected ) )
        actual.emplace( part["reference"].get<std::string>(), part["lib_id"].get<std::string>(),
                        part["digest"].get<std::string>() );

    BOOST_CHECK( actual == expected );

    // The open editor keeps its own settings
    BOOST_CHECK( project.GetProjectFile().m_SchematicSettings == editorSettings );
    BOOST_CHECK_EQUAL( project.GetProjectFile().m_ErcSettings->GetSeverity( ERCE_ENDPOINT_OFF_GRID ),
                       RPT_SEVERITY_IGNORE );
    BOOST_CHECK( read() == bytes );
}


// A release reads the project it is made from and must leave its file as it was
BOOST_AUTO_TEST_CASE( CollectLeavesTheProjectFileUnchanged )
{
    KI_TEST::SCOPED_TEMP_DIR dir( wxS( "mdw_placed_parts" ) );
    std::filesystem::path    source = KI_TEST::GetTestDataRootDir();

    std::filesystem::copy( source / "common" / "git" / "mdw_ctrl_board", dir.Path() );

    SETTINGS_MANAGER      manager;
    std::filesystem::path projectFile = dir.Path() / "ctrl-board.kicad_pro";

    manager.LoadProject( wxString::FromUTF8( projectFile.string() ) );

    auto read = [&]()
    {
        std::ifstream in( projectFile, std::ios::binary );
        return std::string( std::istreambuf_iterator<char>( in ), std::istreambuf_iterator<char>() );
    };

    // An editor closing saves the project, so compare what such a save writes
    manager.SaveProject();
    std::string before = read();

    BOOST_REQUIRE( SchCollectPlacedParts( dir.ChildPathStr( wxS( "ctrl-board.kicad_sch" ) ), manager.Prj() ) );

    manager.SaveProject();
    BOOST_CHECK( read() == before );
}


BOOST_AUTO_TEST_SUITE_END()
