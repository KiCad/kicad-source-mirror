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

#include <connection_graph.h>
#include <erc/erc.h>
#include <lib_id.h>
#include <schematic.h>
#include <sch_io/altium/sch_io_altium.h>
#include <sch_screen.h>
#include <sch_sheet.h>
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

    wxString issue16903DataFile( const wxString& aName ) const
    {
        return wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir()
                                   + "/plugins/altium/issue16903/" )
               + aName;
    }

    wxString eDPAdapterDataFile( const wxString& aName ) const
    {
        return wxString::FromUTF8( KI_TEST::GetTestDataRootDir()
                                   + "pcbnew/plugins/altium/eDP_adapter_dvt1_source/" )
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


// https://gitlab.com/kicad/code/kicad/-/issues/24843
// Altium encodes symbol rotation as quarter turns that map one-for-one onto KiCad's
// SYM_ORIENT_* angles. The importer must store the same angle Altium shows so a later
// "Update Symbols from Library" against a canonical upright symbol does not rotate the
// placement. The reference symbols below are non-mirrored so the stored angle reads back
// directly, without GetOrientation()'s mirror normalization.
BOOST_AUTO_TEST_CASE( Issue24843_SymbolOrientationMatchesAltium )
{
    SCH_IO_ALTIUM plugin;

    SCH_SHEET* rootSheet = plugin.LoadSchematicFile( eDPAdapterDataFile( "power.SchDoc" ),
                                                     &m_schematic );
    BOOST_REQUIRE( rootSheet );

    m_schematic.RefreshHierarchy();

    // Reference designator -> Altium orientation angle read straight from the source records.
    const std::map<wxString, SYMBOL_ORIENTATION_PROP> expected = {
        { wxT( "L10P" ), SYMBOL_ORIENTATION_PROP::SYMBOL_ANGLE_0 },   // Altium ORIENTATION 0
        { wxT( "C10P" ), SYMBOL_ORIENTATION_PROP::SYMBOL_ANGLE_90 },  // Altium ORIENTATION 1
        { wxT( "R10P" ), SYMBOL_ORIENTATION_PROP::SYMBOL_ANGLE_180 }, // Altium ORIENTATION 2
        { wxT( "C13P" ), SYMBOL_ORIENTATION_PROP::SYMBOL_ANGLE_270 }, // Altium ORIENTATION 3
    };

    std::map<wxString, SYMBOL_ORIENTATION_PROP> actual;

    for( const SCH_SHEET_PATH& sheetPath : m_schematic.Hierarchy() )
    {
        for( SCH_ITEM* item : sheetPath.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            wxString    ref = symbol->GetRef( &sheetPath );

            if( expected.count( ref ) )
                actual[ref] = symbol->GetOrientationProp();
        }
    }

    for( const auto& [ref, angle] : expected )
    {
        BOOST_REQUIRE_MESSAGE( actual.count( ref ), "Symbol '" << ref << "' not found on import" );
        BOOST_CHECK_MESSAGE( actual.at( ref ) == angle,
                             "Symbol '" << ref << "' imported with orientation " << actual.at( ref )
                                        << ", expected " << angle );
    }
}


// https://gitlab.com/kicad/code/kicad/-/issues/16903
// Every connectable coordinate in this design sits on a 50 mil grid in Altium's own frame, so the
// importer's Y flip must preserve that grid phase rather than shift the sheet off grid.
BOOST_AUTO_TEST_CASE( Issue16903_ImportKeepsGeometryOnGrid )
{
    SCH_IO_ALTIUM plugin;

    SCH_SHEET* rootSheet =
            plugin.LoadSchematicFile( issue16903DataFile( wxT( "hierarchical_schematic_top.SchDoc" ) ),
                                      &m_schematic );
    BOOST_REQUIRE( rootSheet );

    m_schematic.RefreshHierarchy();

    ERC_TESTER tester( &m_schematic );
    BOOST_CHECK_EQUAL( tester.TestOffGridEndpoints(), 0 );
}


BOOST_AUTO_TEST_SUITE_END()
