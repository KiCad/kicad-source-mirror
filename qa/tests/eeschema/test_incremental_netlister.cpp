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
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 * or you may search the http://www.gnu.org website for the version 32 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <connection_graph.h>
#include <schematic.h>
#include <sch_sheet.h>
#include <sch_screen.h>
#include <sch_symbol.h>
#include <sch_pin.h>
#include <lib_symbol.h>
#include <settings/settings_manager.h>
#include <locale_io.h>

struct CONNECTIVITY_TEST_FIXTURE
{
    CONNECTIVITY_TEST_FIXTURE()
    { }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};

BOOST_FIXTURE_TEST_CASE( RemoveAddItems, CONNECTIVITY_TEST_FIXTURE )
{
    LOCALE_IO dummy;

    // Check for Errors when using global labels
    std::vector<wxString> tests = {// "incremental_test",
        // "issue10430",
        // "issue10926_1",
        // "issue11926",
        // "issue12505",
        // "issue12814",
        // "issue13112",
        // "issue13162",
        // "issue13212",
        // "issue13431",
        // "issue13591",
        // "issue16223",
        // "issue6588",
        "issue7203"};//,
        //  "issue9367"};

    for( const wxString& test : tests )
    {
        KI_TEST::LoadSchematic( m_settingsManager, test, m_schematic );

        SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();

        for( const SCH_SHEET_PATH& path : sheets )
        {
            for( size_t ii = 0; ii < path.size(); ++ii )
            {
                const SCH_SHEET* sheet = path.GetSheet( ii );
                const SCH_SCREEN* screen = sheet->GetScreen();
                std::vector<SCH_ITEM*> items;

                for( SCH_ITEM* item : screen->Items() )
                {
                    if( !item->IsConnectable() )
                    {
                        continue;
                    }

                    if( item->Type() == SCH_SYMBOL_T )
                    {
                        for( SCH_PIN* pin : static_cast<SCH_SYMBOL*>( item )->GetPins() )
                        {
                            items.push_back( pin );
                        }
                    }
                    else
                    {
                        items.push_back( item );
                    }
                }

                for( SCH_ITEM* item : items )
                {
                    for( SCH_ITEM* check_item : items )
                    {
                        auto& conn_items = check_item->ConnectedItems( path );
                        auto conn = check_item->Connection();
                        std::string netname = conn ? conn->GetNetName().ToStdString() : "NoNet";
                        int subgraph = conn ? conn->SubgraphCode() : -1;
                        BOOST_TEST_MESSAGE( test.ToStdString() << ": Item "
                                                                << check_item->GetFriendlyName().ToStdString()
                                                                << " in net " << netname << " subgraph " << subgraph
                                                                << " has " << conn_items.size() << " connections" );
                    }
                    SCH_CONNECTION* connection = item->Connection();

                    if( !connection )
                        continue;

                    wxString netname = connection->GetNetName();

                    if( !item->IsConnectable() )
                        continue;

                    SCH_ITEM_VEC prev_items = item->ConnectedItems( path );
                    std::sort( prev_items.begin(), prev_items.end() );
                    alg::remove_duplicates( prev_items );


                    std::set<std::pair<SCH_SHEET_PATH, SCH_ITEM*>> all_items =
                            m_schematic->ConnectionGraph()->ExtractAffectedItems( { item } );
                    all_items.insert( { path, item } );
                    BOOST_TEST_MESSAGE( test.ToStdString() << ": Item "
                                                           << item->GetFriendlyName().ToStdString()
                                                           << " in net " << netname.ToStdString()
                                                           << " has " << all_items.size() << " affected items" );

                    CONNECTION_GRAPH new_graph( m_schematic.get() );

                    new_graph.SetLastCodes( m_schematic->ConnectionGraph() );

                    for( auto&[ path, item ] : all_items )
                    {
                        wxCHECK2( item, continue );
                        item->SetConnectivityDirty();
                    }

                    new_graph.Recalculate( sheets, false );
                    m_schematic->ConnectionGraph()->Merge( new_graph );

                    SCH_ITEM_VEC curr_items = item->ConnectedItems( path );
                    std::sort( curr_items.begin(), curr_items.end() );
                    alg::remove_duplicates( curr_items );

                    BOOST_CHECK_MESSAGE( prev_items == curr_items,
                                         test.ToStdString() << ": Item "
                                                            << item->GetFriendlyName().ToStdString()
                                                            << " in net " << netname.ToStdString()
                                                            << " changed from " << prev_items.size()
                                                            << " to " << curr_items.size() << " Location:" << item->GetPosition().x << "," << item->GetPosition().y );
                }

            }
        }
    }
}


// Reproducer for Sentry KICAD-4SJ / KICAD-10HY.  A pin on a shared (multi-instantiated) sheet is
// registered in one subgraph per sheet path, but the connection graph's item-to-subgraph map only
// remembers one of them.  When SCH_SYMBOL::UpdatePins() frees the pin after a library update that
// dropped it, ~SCH_ITEM only cleans the mapped subgraph and the other instance keeps a dangling
// driver, which a later recalculation hands to CONNECTION_SUBGRAPH::ResolveDrivers().
BOOST_FIXTURE_TEST_CASE( SharedSheetUpdatePinsNoDanglingDriver, CONNECTIVITY_TEST_FIXTURE )
{
    LOCALE_IO dummy;

    KI_TEST::LoadSchematic( m_settingsManager, wxS( "netlists/complex_hierarchy_shared/complex_hierarchy" ),
                            m_schematic );

    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
    SCH_SHEET_LIST    sheets = m_schematic->BuildSheetListSortedByPageNumbers();

    std::map<SCH_SCREEN*, int> instanceCount;

    for( const SCH_SHEET_PATH& path : sheets )
        instanceCount[path.LastScreen()]++;

    // Count how many retained subgraphs still reference an item.  Pointer identity only; the
    // pointer may be freed by the time this runs.
    auto countRefs =
            [&]( SCH_ITEM* aItem ) -> int
            {
                int refs = 0;

                for( const auto& [key, subgraphs] : graph->GetNetMap() )
                {
                    for( CONNECTION_SUBGRAPH* sg : subgraphs )
                    {
                        if( sg->GetItems().count( aItem ) )
                            refs++;
                    }
                }

                return refs;
            };

    // Find a pin on a shared screen that the initial full rebuild registered once per sheet path
    SCH_SYMBOL* symbol = nullptr;
    SCH_PIN*    victim = nullptr;

    for( auto& [screen, count] : instanceCount )
    {
        if( count < 2 )
            continue;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* candidate = static_cast<SCH_SYMBOL*>( item );

            if( !candidate->GetLibSymbolRef() || candidate->GetPins().size() < 2 )
                continue;

            for( SCH_PIN* pin : candidate->GetPins() )
            {
                if( countRefs( pin ) >= 2 )
                {
                    symbol = candidate;
                    victim = pin;
                    break;
                }
            }

            if( symbol )
                break;
        }

        if( symbol )
            break;
    }

    BOOST_REQUIRE_MESSAGE( symbol && victim, "No shared-sheet pin registered on multiple paths" );

    wxString              pinNumber = victim->GetNumber();
    std::vector<SCH_PIN*> doomedPins = symbol->GetPinsByNumber( pinNumber );

    BOOST_REQUIRE( !doomedPins.empty() );

    std::vector<SCH_ITEM*> danglingCandidates( doomedPins.begin(), doomedPins.end() );

    // Update the symbol from a library version that no longer has this pin.  This mirrors
    // Update Symbol from Library; SetLibSymbol() -> UpdatePins() frees the surplus SCH_PINs.
    std::unique_ptr<LIB_SYMBOL> updated = symbol->GetLibSymbolRef()->Flatten();

    for( SCH_PIN* libPin : updated->GetPinsByNumber( pinNumber ) )
        updated->RemoveDrawItem( libPin );

    symbol->SetLibSymbol( updated.release() );

    BOOST_REQUIRE( symbol->GetPinsByNumber( pinNumber ).empty() );

    // The freed pins must be gone from every retained subgraph, or the next incremental
    // recalculation will dereference them from a thread pool worker
    for( SCH_ITEM* freedPin : danglingCandidates )
    {
        BOOST_CHECK_MESSAGE( countRefs( freedPin ) == 0,
                             "Freed pin " << pinNumber.ToStdString()
                                          << " still referenced by a retained subgraph" );
    }
}
