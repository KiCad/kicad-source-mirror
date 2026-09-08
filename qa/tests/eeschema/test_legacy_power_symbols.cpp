/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <qa_utils/file_utils.h>
#include <schematic_utils/schematic_file_util.h>
#include <wx/filefn.h>

#include <connection_graph.h>
#include <eeschema_helpers.h>
#include <pgm_base.h>
#include <project/project_file.h>
#include <project/net_settings.h>
#include <schematic.h>
#include <sch_screen.h>
#include <sch_line.h>
#include <sch_io/kicad_legacy/sch_io_kicad_legacy.h>
#include <settings/settings_manager.h>
#include <locale_io.h>

struct LEGACY_POWER_SYMBOLS_TEST_FIXTURE
{
    LEGACY_POWER_SYMBOLS_TEST_FIXTURE()
    { }

    void CheckSymbols()
    {
        size_t checked = 0;

        for( SCH_ITEM* item : m_schematic->RootScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            // Fix pre-8.0 legacy power symbols with invisible pins
            // that have mismatched pin names and value fields
            if( symbol->GetLibSymbolRef()
                && symbol->GetLibSymbolRef()->IsGlobalPower()
                && symbol->GetAllLibPins().size() > 0
                && symbol->GetAllLibPins()[0]->IsGlobalPower()
                && !symbol->GetAllLibPins()[0]->IsVisible() )
            {
                ++checked;
                BOOST_CHECK_EQUAL( symbol->GetField( FIELD_T::VALUE )->GetText(),
                                   symbol->GetAllLibPins()[0]->GetName() );
            }
        }

        BOOST_CHECK_GT( checked, 0 );
    }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


BOOST_FIXTURE_TEST_CASE( LegacyPowerFixup, LEGACY_POWER_SYMBOLS_TEST_FIXTURE )
{
    KI_TEST::LoadSchematic( m_settingsManager, "netlists/legacy_power/legacy_power", m_schematic );

    CheckSymbols();
}


BOOST_FIXTURE_TEST_CASE( LegacyPower4HeadlessLoad, LEGACY_POWER_SYMBOLS_TEST_FIXTURE )
{
    const wxString path = KI_TEST::GetEeschemaTestDataDir() + "netlists/legacy_power4/legacy_power4.sch";
    m_schematic.reset( EESCHEMA_HELPERS::LoadSchematic( path, false, true ) );
    BOOST_REQUIRE( m_schematic );
    size_t symbols = 0;

    for( SCH_ITEM* item : m_schematic->RootScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        const auto* symbol = static_cast<SCH_SYMBOL*>( item );
        BOOST_REQUIRE( symbol->GetLibSymbolRef() );
        ++symbols;

        if( symbol->GetField( FIELD_T::REFERENCE )->GetText().StartsWith( "R" ) )
            BOOST_CHECK_EQUAL( symbol->GetAllLibPins().size(), 2 );
    }

    BOOST_CHECK_EQUAL( symbols, 6 );
    CheckSymbols();

    m_schematic.reset();
    KI_TEST::SCOPED_TEMP_DIR directory( "legacy-headless" );
    const wxString withoutCache = directory.PathStr() + "/legacy_power4.sch";
    BOOST_REQUIRE( wxCopyFile( path, withoutCache ) );
    m_schematic.reset( EESCHEMA_HELPERS::LoadSchematic( withoutCache, false, true ) );
    BOOST_CHECK( !m_schematic );
}


BOOST_AUTO_TEST_CASE( LegacyHeadlessLoadUsesItsProjectLibraries )
{
    KI_TEST::SCOPED_TEMP_DIR directory( "legacy-project-libraries" );
    const wxString source = KI_TEST::GetEeschemaTestDataDir() + "netlists/complex_hierarchy";
    std::filesystem::copy( std::filesystem::path( source.ToStdString() ), directory.Path(),
                           std::filesystem::copy_options::recursive );
    SETTINGS_MANAGER settings;
    BOOST_REQUIRE( settings.LoadProject( directory.PathStr() + "/complex_hierarchy.kicad_pro" ) );
    const auto defaults = settings.Prj().GetProjectFile().NetSettings()->GetDefaultNetclass();
    BOOST_REQUIRE( defaults );
    BOOST_CHECK( defaults->HasWireWidth() );
    BOOST_CHECK_GT( defaults->GetWireWidth(), 0 );
    BOOST_CHECK( defaults->HasBusWidth() );
    BOOST_CHECK_GT( defaults->GetBusWidth(), 0 );
    PROJECT* active = &Pgm().GetSettingsManager().Prj();
    const wxString activePath = active->GetProjectFullName();
    BOOST_REQUIRE( active != &settings.Prj() );
    std::unique_ptr<SCHEMATIC> schematic( EESCHEMA_HELPERS::LoadSchematic(
            directory.PathStr() + "/complex_hierarchy.sch", false, false, &settings.Prj() ) );
    BOOST_REQUIRE( schematic );
    BOOST_CHECK( &Pgm().GetSettingsManager().Prj() == active );
    BOOST_CHECK_EQUAL( active->GetProjectFullName(), activePath );
    BOOST_CHECK( &schematic->Project() == &settings.Prj() );
    size_t aliases = 0;
    SCH_SCREENS screens( schematic->Root() );

    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
    {
        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            auto* symbol = static_cast<SCH_SYMBOL*>( item );
            BOOST_REQUIRE( symbol->GetLibSymbolRef() );

            if( symbol->GetLibId().GetLibItemName() == UTF8( "LM358N" ) )
            {
                ++aliases;
                BOOST_CHECK_EQUAL( symbol->GetLibSymbolRef()->GetUnitCount(), 2 );
                BOOST_CHECK_EQUAL( symbol->GetAllLibPins().size(), 8 );
            }
        }
    }

    BOOST_CHECK_EQUAL( aliases, 2 );

    bool checkedOutput = false;

    for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            auto* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &path ) == wxS( "U2" ) )
            {
                SCH_PIN* pin = symbol->GetPin( "1" );
                BOOST_REQUIRE( pin );
                const VECTOR2I pos = pin->GetPosition();
                const auto wires = path.LastScreen()->GetBusesAndWires( pos, false );
                BOOST_REQUIRE_EQUAL( wires.size(), 1 );
                BOOST_CHECK_GT( wires.front()->GetPenWidth(), 0 );

                const auto name = pin->GetConnectionName( &path );
                BOOST_REQUIRE( name );
                BOOST_CHECK_EQUAL( *name, wxString( "VCC" ) );
                checkedOutput = true;
            }
        }
    }

    BOOST_CHECK( checkedOutput );
}


BOOST_FIXTURE_TEST_CASE( LegacyPower4Fixup, LEGACY_POWER_SYMBOLS_TEST_FIXTURE )
{
    const wxString path = KI_TEST::GetEeschemaTestDataDir() + "netlists/legacy_power4/legacy_power4";
    m_settingsManager.LoadProject( "" );
    m_schematic = std::make_unique<SCHEMATIC>( &m_settingsManager.Prj() );
    m_schematic->Reset();
    SCH_IO_KICAD_LEGACY io;
    SCH_SHEET* root = io.LoadSchematicFile( path + ".sch", m_schematic.get() );
    BOOST_REQUIRE( root );
    BOOST_REQUIRE_MESSAGE( io.GetError().IsEmpty(), io.GetError() );
    m_schematic->SetTopLevelSheets( { root } );
    BOOST_REQUIRE_LT( m_schematic->RootScreen()->GetFileFormatVersionAtLoad(), 20230221 );

    size_t mismatches = 0;

    for( SCH_ITEM* item : m_schematic->RootScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
        LIB_SYMBOL* lib = io.LoadSymbol( path + "-cache.lib", symbol->GetLibId().GetLibItemName() );
        BOOST_REQUIRE( lib );
        symbol->SetLibSymbol( lib->Flatten().release() );

        if( lib->IsGlobalPower() && symbol->GetField( FIELD_T::VALUE )->GetText() == "+3.3V" )
        {
            BOOST_REQUIRE_EQUAL( symbol->GetAllLibPins().size(), 1 );
            BOOST_REQUIRE( symbol->GetAllLibPins()[0]->IsGlobalPower() );
            BOOST_REQUIRE( !symbol->GetAllLibPins()[0]->IsVisible() );
            BOOST_CHECK_EQUAL( symbol->GetAllLibPins()[0]->GetName(), wxString( "VCC" ) );
            ++mismatches;
        }
    }

    BOOST_REQUIRE_GT( mismatches, 0 );
    m_schematic->RootScreen()->FixLegacyPowerSymbolMismatches();
    CheckSymbols();
}
