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

#include <memory>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <lib_symbol.h>
#include <pgm_base.h>
#include <project.h>
#include <reporter.h>
#include <sch_screen.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <settings/settings_manager.h>

#include <link_imported_footprints.h>


BOOST_AUTO_TEST_SUITE( LinkImportedFootprints_ )


// Two unannotated symbols share R?, so only the annotated one may take the board footprint
BOOST_AUTO_TEST_CASE( UnannotatedReferencesAreNotLinked )
{
    Pgm().GetSettingsManager().LoadProject( wxS( "" ) );
    PROJECT& project = Pgm().GetSettingsManager().Prj();

    std::unique_ptr<SCHEMATIC> schematic = std::make_unique<SCHEMATIC>( &project );
    schematic->Reset();

    BOOST_REQUIRE_EQUAL( schematic->Hierarchy().size(), 1U );

    SCH_SHEET_PATH path = schematic->Hierarchy().front();
    SCH_SCREEN*    screen = path.LastScreen();
    LIB_SYMBOL     lib( wxS( "R" ), nullptr );

    std::vector<SCH_SYMBOL*> symbols;

    for( const wxString& ref : { wxS( "R?" ), wxS( "R?" ), wxS( "R1" ) } )
    {
        SCH_SYMBOL* symbol = new SCH_SYMBOL( lib, lib.GetLibId(), &path, 1 );
        symbol->SetRef( &path, ref );
        screen->Append( symbol );
        symbols.push_back( symbol );
    }

    std::map<wxString, wxString> footprints = { { wxS( "R?" ), wxS( "lib:R_0603" ) },
                                                { wxS( "R1" ), wxS( "lib:R_0805" ) } };

    BOOST_CHECK_EQUAL( LinkImportedFootprints( *schematic, footprints, NULL_REPORTER::GetInstance() ), 1 );

    BOOST_CHECK( symbols[0]->GetField( FIELD_T::FOOTPRINT )->GetText().IsEmpty() );
    BOOST_CHECK( symbols[1]->GetField( FIELD_T::FOOTPRINT )->GetText().IsEmpty() );
    BOOST_CHECK_EQUAL( symbols[2]->GetField( FIELD_T::FOOTPRINT )->GetText(), wxS( "lib:R_0805" ) );
}


// The import cache renames a footprint two libraries supply differently, which only the board knows, but a field in
// another library is the user's choice
BOOST_AUTO_TEST_CASE( CacheRenamesReachTheFields )
{
    Pgm().GetSettingsManager().LoadProject( wxS( "" ) );
    PROJECT& project = Pgm().GetSettingsManager().Prj();

    std::unique_ptr<SCHEMATIC> schematic = std::make_unique<SCHEMATIC>( &project );
    schematic->Reset();

    SCH_SHEET_PATH path = schematic->Hierarchy().front();
    SCH_SCREEN*    screen = path.LastScreen();
    LIB_SYMBOL     lib( wxS( "R" ), nullptr );

    std::vector<SCH_SYMBOL*> symbols;

    for( const wxString& ref : { wxS( "R1" ), wxS( "R2" ) } )
    {
        SCH_SYMBOL* symbol = new SCH_SYMBOL( lib, lib.GetLibId(), &path, 1 );
        symbol->SetRef( &path, ref );
        screen->Append( symbol );
        symbols.push_back( symbol );
    }

    symbols[0]->SetFootprintFieldText( wxS( "cache:SOIC8" ) );
    symbols[1]->SetFootprintFieldText( wxS( "mine:SOIC8" ) );

    std::map<wxString, wxString> footprints = { { wxS( "R1" ), wxS( "cache:SOIC8_1" ) },
                                                { wxS( "R2" ), wxS( "cache:SOIC8_1" ) } };

    BOOST_CHECK_EQUAL( LinkImportedFootprints( *schematic, footprints, NULL_REPORTER::GetInstance() ), 1 );

    BOOST_CHECK_EQUAL( symbols[0]->GetField( FIELD_T::FOOTPRINT )->GetText(), wxS( "cache:SOIC8_1" ) );
    BOOST_CHECK_EQUAL( symbols[1]->GetField( FIELD_T::FOOTPRINT )->GetText(), wxS( "mine:SOIC8" ) );
}


// A sheet used twice shares its symbols, so a rename that reaches only one of the two parts must leave the field
BOOST_AUTO_TEST_CASE( SharedSheetInstancesMustAgree )
{
    SETTINGS_MANAGER           settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "issue23840/BusAndVectors", schematic );

    std::vector<SCH_SHEET_PATH> paths;

    for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
    {
        if( path.LastScreen()->GetFileName().EndsWith( "LEDs.kicad_sch" ) )
            paths.push_back( path );
    }

    BOOST_REQUIRE_EQUAL( paths.size(), 2 );

    SCH_SYMBOL* shared = nullptr;

    for( SCH_ITEM* item : paths[0].LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        if( symbol->GetRef( &paths[0] ) != symbol->GetRef( &paths[1] ) )
        {
            shared = symbol;
            break;
        }
    }

    BOOST_REQUIRE( shared );

    const wxString first = shared->GetRef( &paths[0] );
    const wxString second = shared->GetRef( &paths[1] );

    shared->SetFootprintFieldText( wxS( "cache:SOIC8" ) );

    std::map<wxString, wxString> split = { { first, wxS( "cache:SOIC8_1" ) }, { second, wxS( "mine:SOIC8" ) } };

    BOOST_CHECK_EQUAL( LinkImportedFootprints( *schematic, split, NULL_REPORTER::GetInstance() ), 0 );
    BOOST_CHECK_EQUAL( shared->GetField( FIELD_T::FOOTPRINT )->GetText(), wxS( "cache:SOIC8" ) );

    std::map<wxString, wxString> agreed = { { first, wxS( "cache:SOIC8_1" ) }, { second, wxS( "cache:SOIC8_1" ) } };

    BOOST_CHECK_GT( LinkImportedFootprints( *schematic, agreed, NULL_REPORTER::GetInstance() ), 0 );
    BOOST_CHECK_EQUAL( shared->GetField( FIELD_T::FOOTPRINT )->GetText(), wxS( "cache:SOIC8_1" ) );
}


BOOST_AUTO_TEST_SUITE_END()
