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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <lib_id.h>
#include <schematic.h>
#include <sch_io/altium/sch_io_altium.h>
#include <sch_label.h>
#include <sch_line.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <settings/settings_manager.h>

#include <map>
#include <optional>
#include <set>
#include <vector>


namespace
{

struct ALTIUM_SCH_IMPORT_FIXTURE
{
    ALTIUM_SCH_IMPORT_FIXTURE() : m_schematic( nullptr )
    {
        m_settingsManager.LoadProject( "" );
        m_schematic.SetProject( &m_settingsManager.Prj() );
        m_schematic.Reset();
    }

    ~ALTIUM_SCH_IMPORT_FIXTURE() { m_schematic.Reset(); }

    wxString dataFile( const wxString& aName ) const
    {
        return wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir()
                                   + "/plugins/altium/issue22943/" )
               + aName;
    }

    wxString issue24861DataFile( const wxString& aName ) const
    {
        return wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir()
                                   + "/plugins/altium/issue24861/" )
               + aName;
    }

    wxString ticket1303DataFile( const wxString& aName ) const
    {
        return wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir()
                                   + "/plugins/altium/ticket1303/" )
               + aName;
    }

    SETTINGS_MANAGER m_settingsManager;
    SCHEMATIC        m_schematic;
};

} // namespace


BOOST_FIXTURE_TEST_SUITE( AltiumSchImport, ALTIUM_SCH_IMPORT_FIXTURE )


// https://gitlab.com/kicad/code/kicad/-/issues/22943
// A component placed from an external Altium library must be addressed by a well-formed library
// id (source library nickname + real library reference) so it resolves against the library that
// project import registers, instead of the importer's internal per-placement name.
BOOST_AUTO_TEST_CASE( Issue22943_SourceLibrarySymbolLibId )
{
    SCH_IO_ALTIUM plugin;

    SCH_SHEET* rootSheet = plugin.LoadSchematicFile( dataFile( "1_cover.SchDoc" ), &m_schematic );
    BOOST_REQUIRE( rootSheet );
    BOOST_REQUIRE( rootSheet->GetScreen() );

    // The real reference names of every symbol contained in the source library.
    wxArrayString libNames;
    plugin.EnumerateSymbolLib( libNames, dataFile( "mounting_holes.SchLib" ) );

    std::set<wxString> libSymbolNames( libNames.begin(), libNames.end() );
    BOOST_REQUIRE( libSymbolNames.count( wxT( "MH M3" ) ) );

    std::vector<SCH_SYMBOL*> fromMountingHoles;

    for( SCH_ITEM* item : rootSheet->GetScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );

        if( sym->GetLibId().GetUniStringLibNickname() == wxT( "mounting_holes" ) )
            fromMountingHoles.push_back( sym );
    }

    BOOST_REQUIRE( !fromMountingHoles.empty() );

    // Every symbol drawn from the source library must name an item that actually exists there.
    for( SCH_SYMBOL* sym : fromMountingHoles )
    {
        const LIB_ID& libId = sym->GetLibId();
        BOOST_CHECK( libId.IsValid() );
        BOOST_CHECK_MESSAGE( libSymbolNames.count( libId.GetUniStringLibItemName() ),
                             "Library id '" << libId.Format().wx_str()
                                            << "' does not resolve in mounting_holes.SchLib" );
    }
}


// https://gitlab.com/kicad/code/kicad/-/issues/24861
BOOST_AUTO_TEST_CASE( Issue24861_RepeatedSchematicChannels )
{
    SCH_IO_ALTIUM plugin;

    std::map<std::string, UTF8> properties;
    properties.emplace( "project_file", UTF8( issue24861DataFile( wxT( "Repeated_Schematic.PrjPcb" ) ) ) );
    properties.emplace( "sch0", UTF8( issue24861DataFile( wxT( "Repeated_Schematic.SchDoc" ) ) ) );
    properties.emplace( "sch1", UTF8( issue24861DataFile( wxT( "Channel.SchDoc" ) ) ) );

    SCH_SHEET* rootSheet = plugin.LoadSchematicFile( wxEmptyString, &m_schematic, nullptr, &properties );
    BOOST_REQUIRE( rootSheet );

    const std::vector<SCH_SHEET*> topLevelSheets = m_schematic.GetTopLevelSheets();
    BOOST_REQUIRE_EQUAL( topLevelSheets.size(), 1 );
    BOOST_CHECK_EQUAL( topLevelSheets.front()->GetName(), wxT( "Repeated_Schematic" ) );

    std::optional<SCH_SHEET_PATH> topLevelPath;
    std::map<wxString, SCH_SHEET_PATH> channelPaths;

    for( const SCH_SHEET_PATH& sheetPath : m_schematic.Hierarchy() )
    {
        SCH_SHEET* sheet = sheetPath.Last();

        if( sheet && sheet->GetName() == wxT( "Repeated_Schematic" ) )
            topLevelPath = sheetPath;
        else if( sheet && sheet->GetName().StartsWith( wxT( "CH" ) ) )
            channelPaths.emplace( sheet->GetName(), sheetPath );
    }

    BOOST_REQUIRE( topLevelPath );
    BOOST_CHECK_EQUAL( topLevelPath->GetPageNumber(), wxT( "1" ) );

    BOOST_REQUIRE_EQUAL( channelPaths.size(), 3 );
    BOOST_CHECK_EQUAL( channelPaths.at( wxT( "CH1" ) ).GetPageNumber(), wxT( "2" ) );
    BOOST_CHECK_EQUAL( channelPaths.at( wxT( "CH2" ) ).GetPageNumber(), wxT( "3" ) );
    BOOST_CHECK_EQUAL( channelPaths.at( wxT( "CH3" ) ).GetPageNumber(), wxT( "4" ) );

    std::set<wxString> ledReferences;
    std::set<wxString> resistorReferences;

    for( const auto& [channelName, sheetPath] : channelPaths )
    {
        for( SCH_ITEM* item : sheetPath.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            wxString    ref = symbol->GetRef( &sheetPath );

            if( ref.StartsWith( wxT( "LED" ) ) )
                ledReferences.insert( ref );
            else if( ref.StartsWith( wxT( "R" ) ) )
                resistorReferences.insert( ref );
        }
    }

    BOOST_CHECK( ledReferences == std::set<wxString>( { wxT( "LED1_CH1" ), wxT( "LED1_CH2" ),
                                                        wxT( "LED1_CH3" ) } ) );
    BOOST_CHECK( resistorReferences == std::set<wxString>( { wxT( "R1_CH1" ), wxT( "R1_CH2" ),
                                                             wxT( "R1_CH3" ) } ) );
}


// Support ticket #1303: an OrCad-derived Altium sheet symbol references several source files
// through a single semicolon-separated filename ("pagea.SchDoc;pageb.SchDoc"), the two pages of one
// multi-page block. The pages cross-reference each other (and one references itself). Every page
// must be merged into the one sub-sheet screen, and the cyclic cross-references must not create a
// recursive hierarchy that trips SCH_SHEET_LIST::BuildSheetList.
BOOST_AUTO_TEST_CASE( Ticket1303_MultiPageBlock )
{
    SCH_IO_ALTIUM plugin;

    SCH_SHEET* rootSheet = plugin.LoadSchematicFile( ticket1303DataFile( wxT( "overview.SchDoc" ) ),
                                                     &m_schematic, nullptr, nullptr );
    BOOST_REQUIRE( rootSheet );
    BOOST_REQUIRE( rootSheet->GetScreen() );

    // Walking the hierarchy must not trip the recursion guard in SCH_SHEET_LIST::BuildSheetList.
    // Before the fix this raised a wxASSERT (the pages resolved to sub-sheets whose remapped
    // filenames collided in the ancestry).
    SCH_SHEET_LIST hierarchy = m_schematic.Hierarchy();

    // Both pages of the block land in a single merged sub-sheet screen; a regression drops one (or
    // both), leaving the sheet empty. The screen also holds no residual sub-sheet symbols, since the
    // cross-page references were pruned rather than descended into.
    bool foundMergedScreen = false;

    for( const SCH_SHEET_PATH& sheetPath : hierarchy )
    {
        SCH_SCREEN* screen = sheetPath.LastScreen();

        if( !screen || screen == rootSheet->GetScreen() )
            continue;

        std::optional<VECTOR2I> posA;
        std::optional<VECTOR2I> posB;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_LABEL_T ) )
        {
            SCH_LABEL* label = static_cast<SCH_LABEL*>( item );

            if( label->GetText() == wxT( "ONLY_A" ) )
                posA = label->GetPosition();
            else if( label->GetText() == wxT( "ONLY_B" ) )
                posB = label->GetPosition();
        }

        if( !posA || !posB )
            continue;

        foundMergedScreen = true;

        // The cross-page references were pruned, not descended into: the merged screen holds no
        // residual sub-sheet symbols.
        int subSheetCount = 0;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SHEET_T ) )
        {
            (void) item;
            subSheetCount++;
        }

        BOOST_CHECK_EQUAL( subSheetCount, 0 );

        // Page B is tiled below page A rather than superimposed, even though both labels sit at the
        // same coordinates in their source files.
        BOOST_CHECK_MESSAGE( *posA != *posB, "Merged pages must be tiled, not overlapping" );

        // The pruned cross-reference sheet's pin was converted to a hierarchical label so the wire
        // that terminated on it still connects by name.
        bool foundSigLabel = false;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_HIER_LABEL_T ) )
        {
            if( static_cast<SCH_HIERLABEL*>( item )->GetText() == wxT( "SIG" ) )
                foundSigLabel = true;
        }

        BOOST_CHECK_MESSAGE( foundSigLabel,
                             "Pruned sheet pin must survive as a hierarchical label" );
    }

    BOOST_CHECK_MESSAGE( foundMergedScreen,
                         "Both pages of the multi-file sheet symbol must load into one screen" );
}


BOOST_AUTO_TEST_SUITE_END()
