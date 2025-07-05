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
