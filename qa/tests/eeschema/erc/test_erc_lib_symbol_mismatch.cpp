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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>
#include <schematic_utils/schematic_file_util.h>
#include <settings/settings_manager.h>
#include <locale_io.h>
#include <lib_symbol.h>
#include <sch_screen.h>
#include <sch_symbol.h>
#include <reporter.h>
#include <sch_io/kicad_legacy/sch_io_kicad_legacy_lib_cache.h>
#include <qa_utils/wx_utils/unit_test_utils.h>
#include <wx/string.h>
#include <advanced_config.h>
#include <erc/erc.h>
#include <erc/erc_exclusion.h>
#include <api/schematic/schematic_rules.pb.h>
#include <set>
#include <pgm_base.h>
#include <project_sch.h>
#include <libraries/library_manager.h>
#include <libraries/library_table.h>
#include <libraries/symbol_library_adapter.h>
#include <wx/filename.h>
#include <wx/file.h>
#include <sch_marker.h>
#include <sch_sheet.h>
#include <scoped_set_reset.h>

struct ERC_LIB_SYMBOL_MISMATCH_FIXTURE
{
    ERC_LIB_SYMBOL_MISMATCH_FIXTURE() {}

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


namespace
{
struct SCOPED_NATIVE_LIBRARY
{
    struct TABLE_RESTORE
    {
        LIBRARY_MANAGER& manager;
        wxString         directory;
        ~TABLE_RESTORE() { manager.LoadProjectTables( directory, { LIBRARY_TABLE_TYPE::SYMBOL } ); }
    };

    static wxString PreviousDirectory( SYMBOL_LIBRARY_ADAPTER* aAdapter )
    {
        BOOST_REQUIRE( aAdapter );
        const auto table = aAdapter->ProjectTable();
        return table && *table ? wxFileName( ( *table )->Path() ).GetPath() : wxString();
    }

    explicit SCOPED_NATIVE_LIBRARY( PROJECT& aProject ) :
            adapter( PROJECT_SCH::SymbolLibAdapter( &aProject ) ),
            restore{ Pgm().GetLibraryManager(), PreviousDirectory( adapter ) }
    {
        const wxString directory = wxFileName::GetTempDir() + "/library-erc-" + KIID().AsString();
        BOOST_REQUIRE( wxFileName::Mkdir( directory, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );
        wxFile tableFile( directory + "/sym-lib-table", wxFile::write );
        BOOST_REQUIRE( tableFile.IsOpened() );
        BOOST_REQUIRE( tableFile.Write( "(sym_lib_table (version 7))\n" ) );
        tableFile.Close();
        restore.manager.LoadProjectTables( directory, { LIBRARY_TABLE_TYPE::SYMBOL } );
        LIBRARY_TABLE* table = adapter->ProjectTable().value_or( nullptr );
        BOOST_REQUIRE( table );
        row = &table->InsertRow();
        row->SetNickname( "NativeCapturedLibrary" );
        row->SetURI( wxString( KI_TEST::GetEeschemaTestDataDir() ) + "libs/4xxx.kicad_sym" );
        row->SetType( "KiCad" );
        row->SetScope( LIBRARY_TABLE_SCOPE::PROJECT );
        adapter->LoadOne( row->Nickname() );
        BOOST_REQUIRE( adapter->IsLibraryLoaded( row->Nickname() ) );
        external = adapter->LoadSymbol( row->Nickname(), "4001" );
        BOOST_REQUIRE( external );
    }

    SYMBOL_LIBRARY_ADAPTER* adapter;
    TABLE_RESTORE           restore;
    LIBRARY_TABLE_ROW*      row = nullptr;
    LIB_SYMBOL*             external = nullptr;
};
}


BOOST_AUTO_TEST_SUITE( ERCLibSymbolMismatch )

BOOST_FIXTURE_TEST_CASE( LibraryIssuesPreserveSharedPathsAndExclusions, ERC_LIB_SYMBOL_MISMATCH_FIXTURE )
{
    LOCALE_IO locale;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );
    KI_TEST::LoadSchematic( m_settingsManager, "issue23840/BusAndVectors", m_schematic );
    std::vector<SCH_SHEET_PATH> paths;

    for( const auto& path : m_schematic->Hierarchy() )
    {
        if( path.LastScreen()->GetFileName().EndsWith( "LEDs.kicad_sch" ) )
            paths.push_back( path );
    }

    BOOST_REQUIRE_EQUAL( paths.size(), 2u );
    SCH_SCREEN* screen = paths.front().LastScreen();
    BOOST_REQUIRE( screen == paths.back().LastScreen() );
    auto symbols = screen->Items().OfType( SCH_SYMBOL_T );
    BOOST_REQUIRE( symbols.begin() != symbols.end() );
    auto* symbol = static_cast<SCH_SYMBOL*>( *symbols.begin() );
    BOOST_REQUIRE( symbol->GetLibSymbolRef() );
    SCOPED_NATIVE_LIBRARY library( m_schematic->Project() );
    const auto native = library.external->Flatten();
    symbol->SetLibSymbol( new LIB_SYMBOL( *native ) );
    symbol->GetLibSymbolRef()->SetKeyWords( "Native shared mismatch" );
    m_schematic->ErcSettings().SetSeverity( ERCE_LIB_SYMBOL_ISSUES, RPT_SEVERITY_WARNING );
    auto& exclusions = m_schematic->ErcSettings().m_ErcExclusions;
    const auto markers = [&]( int aCode )
    {
        std::vector<SCH_MARKER*> result;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_MARKER_T ) )
        {
            auto* marker = static_cast<SCH_MARKER*>( item );
            const auto error = marker->GetRCItem();

            if( error->GetErrorCode() == aCode && error->GetMainItemID() == symbol->m_Uuid )
                result.push_back( marker );
        }

        return result;
    };

    for( int code : { ERCE_LIB_SYMBOL_ISSUES, ERCE_LIB_SYMBOL_MISMATCH } )
    {
        symbol->SetLibId( code == ERCE_LIB_SYMBOL_ISSUES ? LIB_ID( "CapturedMissingLibrary", "NativeSymbol" )
                                                       : LIB_ID( library.row->Nickname(), "4001" ) );
        m_schematic->ErcSettings().SetSeverity( code, RPT_SEVERITY_WARNING );
        for( bool backend : { true, false } )
        {
            enabled = backend;
            m_schematic->RebuildConnectivity();

            for( bool historical : { false, true } )
            {
                exclusions.clear();
                ERC_TESTER tester( m_schematic.get() );
                tester.TestLibSymbolIssues();
                BOOST_REQUIRE_EQUAL( markers( code ).size(), 2u );
                std::set<KIID_PATH> seen;

                for( SCH_MARKER* marker : markers( code ) )
                {
                    const auto error = std::static_pointer_cast<ERC_ITEM>( marker->GetRCItem() );
                    BOOST_REQUIRE( error->IsSheetSpecific() );
                    BOOST_REQUIRE( error->MainItemHasSheetPath() );
                    const auto& markerPath = error->GetSpecificSheetPath().PathRef();
                    BOOST_CHECK( error->GetMainItemSheetPath().PathRef() == markerPath );
                    BOOST_CHECK( seen.insert( markerPath ).second );

                    if( markerPath == paths.front().PathRef() )
                    {
                        auto proto = ERC_EXCLUSION::FromMarker( *marker ).ToProto();

                        if( historical )
                        {
                            proto.mutable_marker()->clear_sheet_specific_path();
                            proto.mutable_marker()->clear_main_item_sheet_path();
                        }

                        auto exclusion = ERC_EXCLUSION::FromProto( proto );
                        exclusion.SetComment( "Retained library exclusion" );
                        exclusions.insert( exclusion );
                    }

                    screen->DeleteItem( marker );
                }

                const std::set<KIID_PATH> expectedPaths{ paths.front().PathRef(), paths.back().PathRef() };
                BOOST_CHECK( seen == expectedPaths );
                BOOST_REQUIRE_EQUAL( exclusions.size(), 1u );
                tester.TestLibSymbolIssues();
                m_schematic->ResolveERCExclusionsPostUpdate();
                BOOST_REQUIRE_EQUAL( markers( code ).size(), 2u );

                for( SCH_MARKER* marker : markers( code ) )
                {
                    const auto error = std::static_pointer_cast<ERC_ITEM>( marker->GetRCItem() );
                    const bool expected = historical
                                          || error->GetSpecificSheetPath().PathRef() == paths.front().PathRef();
                    BOOST_CHECK_EQUAL( marker->IsExcluded(), expected );
                    BOOST_CHECK_EQUAL( marker->GetComment(), expected ? wxString( "Retained library exclusion" )
                                                                     : wxString() );
                    screen->DeleteItem( marker );
                }
            }
        }
    }

}


BOOST_FIXTURE_TEST_CASE( VariantExclusionsWithoutItemSheetPathStillApply, ERC_LIB_SYMBOL_MISMATCH_FIXTURE )
{
    LOCALE_IO locale;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, false );
    KI_TEST::LoadSchematic( m_settingsManager, "issue23840/BusAndVectors", m_schematic );
    std::vector<SCH_SHEET_PATH> paths;

    for( const auto& path : m_schematic->Hierarchy() )
    {
        if( path.LastScreen()->GetFileName().EndsWith( "LEDs.kicad_sch" ) )
            paths.push_back( path );
    }

    BOOST_REQUIRE_EQUAL( paths.size(), 2u );
    SCH_SCREEN* screen = paths.front().LastScreen();
    BOOST_REQUIRE( screen == paths.back().LastScreen() );
    auto symbols = screen->Items().OfType( SCH_SYMBOL_T );
    BOOST_REQUIRE( symbols.begin() != symbols.end() );
    auto* symbol = static_cast<SCH_SYMBOL*>( *symbols.begin() );
    SCOPED_NATIVE_LIBRARY library( m_schematic->Project() );
    const auto native = library.external->Flatten();
    symbol->SetLibSymbol( new LIB_SYMBOL( *native ) );
    symbol->SetLibId( LIB_ID( "Embedded", "Base" ) );
    const LIB_ID alternate( library.row->Nickname(), "4001" );
    const LIB_ID missing( library.row->Nickname(), "MissingVariantSymbol" );
    const wxString variant = wxS( "Captured variant" );
    m_schematic->AddVariant( variant );
    symbol->SetVariantSymbolOverride( paths[0], variant, alternate );
    symbol->SetVariantSymbolOverride( paths[1], variant, missing );
    m_schematic->ErcSettings().SetSeverity( ERCE_VARIANT_SYMBOL_INVALID, RPT_SEVERITY_ERROR );
    m_schematic->ErcSettings().SetSeverity( ERCE_VARIANT_SYMBOL_INCOMPATIBLE, RPT_SEVERITY_ERROR );
    m_schematic->RebuildConnectivity();

    symbol->GetLibSymbolRef()->SetUnitCount( native->GetUnitCount() + 1, false );

    // Exclusions saved before variant markers carried the item's sheet path must still apply
    ERC_TESTER tester( m_schematic.get() );
    const int count = tester.TestVariantSymbols();
    BOOST_REQUIRE_GT( count, 1 );
    std::vector<SCH_MARKER*> legacy;

    for( SCH_ITEM* item : screen->Items().OfType( SCH_MARKER_T ) )
        legacy.push_back( static_cast<SCH_MARKER*>( item ) );

    for( SCH_MARKER* marker : legacy )
    {
        auto proto = ERC_EXCLUSION::FromMarker( *marker ).ToProto();
        proto.mutable_marker()->clear_main_item_sheet_path();
        m_schematic->ErcSettings().m_ErcExclusions.insert( ERC_EXCLUSION::FromProto( proto ) );
        screen->DeleteItem( marker );
    }

    BOOST_REQUIRE_EQUAL( tester.TestVariantSymbols(), count );
    m_schematic->ResolveERCExclusionsPostUpdate();
    int excluded = 0;

    for( SCH_ITEM* item : screen->Items().OfType( SCH_MARKER_T ) )
    {
        BOOST_CHECK( static_cast<SCH_MARKER*>( item )->IsExcluded() );
        ++excluded;
    }

    BOOST_CHECK_EQUAL( excluded, count );
}


BOOST_FIXTURE_TEST_CASE( Issue22371LegacyLibrary, ERC_LIB_SYMBOL_MISMATCH_FIXTURE )
{
    LOCALE_IO dummy;

    KI_TEST::LoadSchematic( m_settingsManager, wxString( "issue22371/issue22371" ), m_schematic );

    std::string dataDir = KI_TEST::GetEeschemaTestDataDir();
    wxString legacyLibPath = wxString( dataDir + "issue22371/lib.sch/74xx.lib" );

    SCH_IO_KICAD_LEGACY_LIB_CACHE legacyCache( legacyLibPath );
    legacyCache.Load();

    const LIB_SYMBOL_MAP& symbols = legacyCache.GetSymbolMap();
    auto it = symbols.find( "74LS00" );
    BOOST_REQUIRE_MESSAGE( it != symbols.end(), "74LS00 not found in legacy library" );

    std::unique_ptr<LIB_SYMBOL> flattenedLibSymbol = it->second->Flatten();

    SCH_SCREENS screens( m_schematic->Root() );

    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
    {
        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            LIB_SYMBOL* libSymbolInSchematic = symbol->GetLibSymbolRef().get();

            if( !libSymbolInSchematic )
                continue;

            if( symbol->GetLibId().GetLibItemName() != "74LS00" )
                continue;

            int flags = ~SCH_ITEM::COMPARE_FLAGS::UUID;
            flags &= ~SCH_ITEM::COMPARE_FLAGS::UNIT;
            flags &= ~SCH_ITEM::COMPARE_FLAGS::IDENTITY;

            int result = flattenedLibSymbol->Compare( *libSymbolInSchematic, flags );

            BOOST_CHECK_EQUAL( result, 0 );

            // The default Compare Symbol settings should also not find changes
            SCHEMATIC_SETTINGS defaultSettings( nullptr, "empty" );
            int defaultFlags = defaultSettings.SymbolCompareFlags();

            result = flattenedLibSymbol->Compare( *libSymbolInSchematic, defaultFlags );

            BOOST_CHECK_EQUAL( result, 0 );
            return;
        }
    }

    BOOST_FAIL( "No 74LS00 symbol found in schematic" );
}


BOOST_AUTO_TEST_SUITE_END()
