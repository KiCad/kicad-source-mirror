/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Joshua Redstone redstone at gmail.com
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


#include <boost/filesystem.hpp>
#include <class_board.h>
#include <class_module.h>
#include <class_pcb_text.h>
#include <common.h>
#include <pcbnew_utils/board_construction_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <string>
#include <unit_test_utils/unit_test_utils.h>

BOOST_AUTO_TEST_SUITE( GroupSaveLoad )

// The tests below set up a test case with a spec for the set of groups to create.
// A group can contain members from this list of candidates.
enum ItemType
{
    TEXT0,
    TEXT1,
    TEXT2,
    TEXT3,
    TEXT4,
    TEXT5,
    TEXT6,
    TEXT7,
    TEXT8,
    REMOVED_TEXT, // not known to board
    GROUP0,
    GROUP1,
    GROUP2,
    NAME_GROUP3,
    NAME_GROUP4,
    NAME_GROUP3_DUP, // Group with name identical to NAME_GROUP3
    NUM_ITEMS
};

/*
 * Takes a vector of group specifications for groups to create.
 * Each group is a vector of which ItemTypes to put in the group.
 * The first group corresponds to GROUP0, the second to GROUP1, and os on.
 */
BOARD* createBoard( const std::vector<std::vector<ItemType>>& spec )
{
    BOARD*                  aBoard = new BOARD();
    std::vector<PCB_GROUP*>     groups;
    std::vector<TEXTE_PCB*> textItems;

    // Create groups
    for( int idx = 0; idx < 6; idx++ )
    {
        PCB_GROUP* gr = new PCB_GROUP( aBoard );
        if( idx >= ( NAME_GROUP3 - GROUP0 ) )
        {
            wxString name = wxString::Format(
                    _( "group-%d" ), ( idx == ( NAME_GROUP3_DUP - GROUP0 ) ) ? 3 : idx );
            gr->SetName( name );
            BOOST_CHECK_EQUAL( gr->GetName(), name );
        }
        groups.push_back( gr );
    }

    // Create text items and add to board.
    for( int idx = 0; idx < 10; idx++ )
    {
        auto textItem = new TEXTE_PCB( aBoard );
        textItem->SetText( wxString::Format( _( "some text-%d" ), idx ) );
        if( idx < 9 ) // don't add REMOVED_TEXT
        {
            aBoard->Add( textItem );
        }
        textItems.push_back( textItem );
    }

    // Populate groups based on spec
    for( int groupIdx = 0; groupIdx < spec.size(); groupIdx++ )
    {
        auto&  groupSpec = spec[groupIdx];
        PCB_GROUP* group     = groups[groupIdx];
        int    count     = 0;
        for( ItemType item : groupSpec )
        {
            if( item <= REMOVED_TEXT )
            {
                group->AddItem( textItems[item] );
                count++;
            }
            else // it's a group
            {
                group->AddItem( groups[item - GROUP0] );
                count++;
            }
        }
        BOOST_CHECK_EQUAL( group->GetItems().size(), count );
        aBoard->Add( group );
    }

    BOOST_TEST_CHECKPOINT( "Returning fresh board" );
    return aBoard;
}

// Check if two groups are identical by comparing the fields (by Uuid).
void testGroupEqual( const PCB_GROUP& group1, const PCB_GROUP& group2 )
{
    BOOST_CHECK_EQUAL( group1.m_Uuid.AsString(), group2.m_Uuid.AsString() );
    BOOST_CHECK_EQUAL( group1.GetName(), group2.GetName() );
    auto items1 = group1.GetItems();
    auto items2 = group2.GetItems();

    // Test that the sets items1 and items2 are identical, by checking m_Uuid
    BOOST_CHECK_EQUAL( items1.size(), items2.size() );
    for( auto item1 : items1 )
    {
        auto item2 = std::find_if( items2.begin(), items2.end(),
                [&]( auto elem ) { return elem->m_Uuid.AsString() == item1->m_Uuid.AsString(); } );
        BOOST_CHECK( item2 != items2.end() );
        // Could check other properties here...
    }
}

// Check if two GROUPS are identical by comparing the groups in each of them.
void testGroupsEqual( const GROUPS& groups1, const GROUPS& groups2 )
{
    BOOST_CHECK_EQUAL( groups1.size(), groups2.size() );
    for( auto group1 : groups1 )
    {
        auto group2 = std::find_if( groups2.begin(), groups2.end(),
                [&]( auto elem ) { return elem->m_Uuid.AsString() == group1->m_Uuid.AsString(); } );
        BOOST_CHECK( group2 != groups2.end() );
        testGroupEqual( *group1, **group2 );
    }
}

/*
 * Create board based on spec, save it to a file, load it, and make sure the
 * groups in the resulting board are the same as the groups we started with.
 */
void testSaveLoad( const std::vector<std::vector<ItemType>>& spec )
{
    BOARD* aBoard1 = createBoard( spec );
    auto path = boost::filesystem::temp_directory_path() / "group_saveload_tst.kicad_pcb";
    ::KI_TEST::DumpBoardToFile( *aBoard1, path.string() );
    auto aBoard2 = ::KI_TEST::ReadBoardFromFileOrStream( path.string() );
    testGroupsEqual( aBoard1->Groups(), aBoard2->Groups() );
}

// Test saving & loading of a few configurations
BOOST_AUTO_TEST_CASE( HealthyCases )
{
    //BOOST_TEST_CONTEXT( "happy" );

    // Test board with no groups
    testSaveLoad( {} );

    // Single group
    testSaveLoad( { { TEXT0 } } );
    testSaveLoad( { { TEXT0, TEXT1 } } );

    // Two groups
    testSaveLoad( { { TEXT0, TEXT1 }, { TEXT2, TEXT3 } } );
    testSaveLoad( { { TEXT0, TEXT1 }, { TEXT2, GROUP0 } } );

    // Subgroups by no cycle
    testSaveLoad( { { TEXT0, GROUP1 }, { TEXT2 }, { TEXT3, GROUP0 } } );
    testSaveLoad( { { TEXT0 }, { TEXT2 }, { GROUP1, GROUP0 } } );
    testSaveLoad( { { TEXT0 }, { TEXT1 }, { TEXT2, NAME_GROUP3 }, { TEXT3 } } );
    testSaveLoad( { { TEXT0 }, { TEXT1 }, { TEXT2, NAME_GROUP3 }, { TEXT3, GROUP0 } } );
    testSaveLoad( { { TEXT0 }, { TEXT1 }, { TEXT2 }, { TEXT3 }, { NAME_GROUP3, GROUP0 } } );
}

BOOST_AUTO_TEST_CASE( ErrorCases )
{
    // A cycle
    BOARD* aBoard1 = createBoard( { { TEXT0, GROUP1 }, { TEXT2, GROUP0 } } );
    BOOST_CHECK_EQUAL( aBoard1->GroupsSanityCheck(), "Cycle detected in group membership" );

    // More complex cycle
    aBoard1 = createBoard( { { TEXT0, GROUP1 }, { TEXT1 }, { TEXT2, NAME_GROUP4 },
                             { TEXT3, GROUP2 }, { TEXT4, NAME_GROUP3 } } );
    BOOST_CHECK_EQUAL( aBoard1->GroupsSanityCheck(), "Cycle detected in group membership" );

    // Reference group not on board
    aBoard1      = createBoard( { { TEXT0, GROUP1 } } );
    wxString res = aBoard1->GroupsSanityCheck();
    BOOST_CHECK_MESSAGE( res.find( "contains deleted item" ) != std::string::npos, res );

    // Single empty group
    aBoard1      = createBoard( { {} } );
    res          = aBoard1->GroupsSanityCheck();
    BOOST_CHECK_MESSAGE(
            res.find( "Group must have at least one member" ) != std::string::npos, res );

    // Duplicate group name
    aBoard1 = createBoard( { { TEXT0 }, { TEXT1 }, { TEXT2 }, { TEXT3 }, { TEXT4 }, { TEXT5 } } );
    res     = aBoard1->GroupsSanityCheck();
    BOOST_CHECK_MESSAGE( res.find( "Two groups of identical name" ) != std::string::npos, res );

    // Group references item that is not on board
    aBoard1 = createBoard( { { REMOVED_TEXT } } );
    res     = aBoard1->GroupsSanityCheck();
    BOOST_CHECK_MESSAGE( res.find( "contains deleted item" ) != std::string::npos, res );
}

BOOST_AUTO_TEST_SUITE_END()
