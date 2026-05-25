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
 */

// Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24409

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <memory>
#include <set>

#include <connection_graph.h>
#include <eeschema_helpers.h>
#include <erc/erc.h>
#include <erc/erc_item.h>
#include <erc/erc_report.h>
#include <erc/erc_settings.h>
#include <locale_io.h>
#include <sch_marker.h>
#include <sch_pin.h>
#include <sch_reference_list.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <schematic.h>


// The reproduction case has three sheet instances of one sub-sheet with two
// resistors R1/R2 (no wires).  Per-instance annotations make this R1+R2,
// R3+R4 and R5+R6.  RunERC dedups markers that share a driver pin so each
// pin produces exactly one pin_not_connected marker, but the dedup must
// pick the marker on the *first* sheet path (by page number) so that
// kicad-cli produces the same report a user sees in the GUI.  Before the
// fix the unordered iteration of SCH_ITEM::m_connection_map caused
// subgraphs to be created in hash-bucket order, so the marker landed on
// whichever instance happened to surface first (R5/R6 on this layout).
BOOST_AUTO_TEST_CASE( Issue24409CliHierarchicalInstances )
{
    LOCALE_IO dummy;

    wxString schPath = wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() ) +
                       wxS( "issue24409/issue24409.kicad_sch" );

    // Use the CLI/headless loader directly so the regression covers the path
    // exercised by kicad-cli sch erc / sch export bom / sch export netlist.
    std::unique_ptr<SCHEMATIC> sch( EESCHEMA_HELPERS::LoadSchematic( schPath, true, false ) );
    BOOST_REQUIRE( sch != nullptr );

    SCH_SHEET_LIST sheets = sch->Hierarchy();

    // Sanity: three sub-sheet instances + the root sheet.
    BOOST_REQUIRE_EQUAL( sheets.size(), 4u );

    // The two resistors get annotated R1..R6 once instances are resolved.
    SCH_REFERENCE_LIST refs;
    sheets.GetSymbols( refs, SYMBOL_FILTER_NON_POWER, false );
    BOOST_REQUIRE_EQUAL( refs.GetCount(), 6u );

    std::set<wxString> seenRefs;

    for( size_t i = 0; i < refs.GetCount(); ++i )
        seenRefs.insert( refs[i].GetRef() );

    BOOST_CHECK( seenRefs.count( wxS( "R1" ) ) );
    BOOST_CHECK( seenRefs.count( wxS( "R2" ) ) );
    BOOST_CHECK( seenRefs.count( wxS( "R3" ) ) );
    BOOST_CHECK( seenRefs.count( wxS( "R4" ) ) );
    BOOST_CHECK( seenRefs.count( wxS( "R5" ) ) );
    BOOST_CHECK( seenRefs.count( wxS( "R6" ) ) );

    // Now run ERC against the loaded schematic and verify that every pin on
    // every sheet instance has been reported.  Disable the library-mismatch
    // and sim-model checks because the reproduction project ships without
    // installed libraries.
    ERC_SETTINGS& settings = sch->ErcSettings();
    settings.m_ERCSeverities[ERCE_LIB_SYMBOL_ISSUES] = RPT_SEVERITY_IGNORE;
    settings.m_ERCSeverities[ERCE_LIB_SYMBOL_MISMATCH] = RPT_SEVERITY_IGNORE;
    settings.m_ERCSeverities[ERCE_SIMULATION_MODEL] = RPT_SEVERITY_IGNORE;

    sch->ConnectionGraph()->RunERC();

    // Walk every sheet's screen and count pin_not_connected markers.
    int unconnectedMarkers = 0;
    std::set<wxString> markedRefs;

    std::set<const SCH_SCREEN*> visitedScreens;

    for( const SCH_SHEET_PATH& sheet : sheets )
    {
        SCH_SCREEN* screen = sheet.LastScreen();

        // Each underlying screen carries its own markers; visit each only once.
        if( !visitedScreens.insert( screen ).second )
            continue;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_MARKER_T ) )
        {
            SCH_MARKER* marker = static_cast<SCH_MARKER*>( item );
            std::shared_ptr<ERC_ITEM> ercItem =
                    std::dynamic_pointer_cast<ERC_ITEM>( marker->GetRCItem() );

            if( !ercItem || ercItem->GetErrorCode() != ERCE_PIN_NOT_CONNECTED )
                continue;

            ++unconnectedMarkers;

            const SCH_SHEET_PATH& markerSheet = ercItem->IsSheetSpecific()
                                                        ? ercItem->GetSpecificSheetPath()
                                                        : sheet;
            EDA_ITEM* erred = sch->ResolveItem( ercItem->GetMainItemID(), nullptr, true );

            if( SCH_PIN* pin = dynamic_cast<SCH_PIN*>( erred ) )
            {
                if( SCH_SYMBOL* parent = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() ) )
                    markedRefs.insert( parent->GetRef( &markerSheet ) );
            }
        }
    }

    // 2 resistors per sub-sheet * 2 pins per resistor = 4 logical pins; the
    // RunERC dedup collapses the three instances of each pin down to one
    // marker apiece so we expect 4 markers in total.
    BOOST_CHECK_MESSAGE( unconnectedMarkers == 4,
                         "Expected 4 deduplicated pin_not_connected markers, got "
                         << unconnectedMarkers );

    // The dedup must keep the marker on the first sheet instance by page
    // number (Untitled Sheet, page 2, annotated R1/R2) so that kicad-cli
    // matches the GUI.  Without the fix this is R5/R6 because the unordered
    // iteration of m_connection_map surfaces the last sheet first.
    for( const wxString& ref : { wxS( "R1" ), wxS( "R2" ) } )
    {
        BOOST_CHECK_MESSAGE( markedRefs.count( ref ),
                             "Missing pin_not_connected marker for " << ref
                             << " on the first sheet instance" );
    }

    for( const wxString& ref : { wxS( "R3" ), wxS( "R4" ), wxS( "R5" ), wxS( "R6" ) } )
    {
        BOOST_CHECK_MESSAGE( !markedRefs.count( ref ),
                             "Unexpected pin_not_connected marker for " << ref
                             << "; dedup should keep only the first instance" );
    }

    // Also exercise the text report writer end-to-end so the regression catches
    // the case where the marker is on the right sheet but the printed reference
    // is wrong (i.e. the symbol's REFERENCE field text was left on the last
    // sheet's annotation by the helpers' UpdateAllScreenReferences loop).
    std::shared_ptr<SHEETLIST_ERC_ITEMS_PROVIDER> markersProvider =
            std::make_shared<SHEETLIST_ERC_ITEMS_PROVIDER>( sch.get() );
    markersProvider->SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );

    ERC_REPORT reportWriter( sch.get(), EDA_UNITS::MM, markersProvider );
    wxString   report = reportWriter.GetTextReport();

    for( const wxString& ref : { wxS( "R1" ), wxS( "R2" ) } )
    {
        BOOST_CHECK_MESSAGE( report.Contains( wxS( "Symbol " ) + ref + wxS( " " ) ),
                             "ERC text report missing pin_not_connected for "
                             << ref << "\n" << report );
    }

    for( const wxString& ref : { wxS( "R3" ), wxS( "R4" ), wxS( "R5" ), wxS( "R6" ) } )
    {
        BOOST_CHECK_MESSAGE( !report.Contains( wxS( "Symbol " ) + ref + wxS( " " ) ),
                             "ERC text report unexpectedly mentions "
                             << ref << "\n" << report );
    }
}
