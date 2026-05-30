/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Joshua Redstone redstone at gmail.com
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
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <bitset>
#include <filesystem>
#include <string>

#include <board.h>
#include <footprint.h>
#include <pcb_generator.h>
#include <pcb_group.h>
#include <pcb_text.h>
#include <pcb_track.h>
#include <generators/pcb_tuning_pattern.h>
#include <common.h>
#include <pcbnew_utils/board_construction_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

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
    REMOVED_TEXT,   // Text not added to board
    GROUP0,
    GROUP1,
    GROUP2,
    NAME_GROUP3,
    NAME_GROUP4,
    NAME_GROUP3_DUP, // Group with name identical to NAME_GROUP3
    REMOVED_GROUP,   // Group not added to board
    NUM_ITEMS
};

// The objects associated with item REMOVED_TEXT and REMOVED_GROUP are not added to the board,
// so they are not cleaned up when the board is deleted. These pointers stores the objects
// so they can be deleted once they are done being used.
static PCB_TEXT*  s_removedText  = nullptr;
static PCB_GROUP* s_removedGroup = nullptr;


/*
 * Takes a vector of group specifications for groups to create.
 * Each group is a vector of which ItemTypes to put in the group.
 * The first group corresponds to GROUP0, the second to GROUP1, and os on.
 */
std::unique_ptr<BOARD> createBoard( const std::vector<std::vector<ItemType>>& spec, bool aAllowInvalidGroups = false )
{
    std::unique_ptr<BOARD>   board = std::make_unique<BOARD>();
    std::vector<BOARD_ITEM*> items;

    // Create text items and add to board.
    for( int idx = 0; idx <= REMOVED_TEXT; idx++ )
    {
        PCB_TEXT* textItem = new PCB_TEXT( board.get() );
        textItem->SetText( wxString::Format( _( "some text-%d" ), idx ) );

        // Don't add REMOVED_TEXT to the board
        if( idx < REMOVED_TEXT )
            board->Add( textItem );

        items.push_back( textItem );
    }

    // Create groups
    for( int idx = 0; idx < ( NUM_ITEMS - GROUP0 ); idx++ )
    {
        PCB_GROUP* gr = new PCB_GROUP( board.get() );

        if( idx >= ( NAME_GROUP3 - GROUP0 ) )
        {
            wxString name = wxString::Format( _( "group-%d" ),
                                               ( idx == ( NAME_GROUP3_DUP - GROUP0 ) ) ? 3 : idx );
            gr->SetName( name );
            BOOST_CHECK_EQUAL( gr->GetName(), name );
        }

        items.push_back( gr );
    }

    std::bitset<NUM_ITEMS> used;

    // Populate groups based on spec
    for( int offset = 0; offset < ( NUM_ITEMS - GROUP0 ); offset++ )
    {
        int groupIdx = GROUP0 + offset;

        PCB_GROUP* group = static_cast<PCB_GROUP*>( items[groupIdx] );

        if( offset < spec.size() )
        {
            const std::vector<ItemType>& groupSpec = spec[offset];

            for( ItemType item : groupSpec )
            {
                used.set( static_cast<size_t>( item ) );

                if( aAllowInvalidGroups )
                {
                    // The invalid-group tests intentionally build graphs that AddItem()
                    // rejects so GroupsSanityCheck() can verify diagnostics.
                    group->GetItems().insert( items[item] );
                    items[item]->SetParentGroup( group );
                }
                else
                {
                    group->AddItem( items[item] );
                }
            }

            BOOST_CHECK_EQUAL( group->GetItems().size(), groupSpec.size() );
            board->Add( group );
        }
        else if( groupIdx != REMOVED_GROUP && used.test( groupIdx ) )
        {
            // This group is used in another group, so it must be on the board
            board->Add( group );
        }
        else if( groupIdx != REMOVED_GROUP )
        {
            // If the group isn't used, delete it
            delete group;
        }
    }

    // Delete the removed text item if it isn't used
    if( used.test( REMOVED_TEXT ) )
        s_removedText = static_cast<PCB_TEXT*>( items[REMOVED_TEXT] );
    else
        delete items[REMOVED_TEXT];

    // Delete the removed group item if it isn't used
    if( used.test( REMOVED_GROUP ) )
        s_removedGroup = static_cast<PCB_GROUP*>( items[REMOVED_GROUP] );
    else
        delete items[REMOVED_GROUP];

    BOOST_TEST_CHECKPOINT( "Returning fresh board" );
    return board;
}


// Check if two groups are identical by comparing the fields (by Uuid).
void testGroupEqual( const PCB_GROUP& group1, const PCB_GROUP& group2 )
{
    BOOST_CHECK_EQUAL( group1.m_Uuid.AsString(), group2.m_Uuid.AsString() );
    BOOST_CHECK_EQUAL( group1.GetName(), group2.GetName() );

    const std::unordered_set<EDA_ITEM*>& items1 = group1.GetItems();
    const std::unordered_set<EDA_ITEM*>& items2 = group2.GetItems();

    BOOST_CHECK_EQUAL( items1.size(), items2.size() );

    // Test that the sets items1 and items2 are identical, by checking m_Uuid
    for( EDA_ITEM* item1 : items1 )
    {
        auto cmp = [&]( EDA_ITEM* elem )
                   {
                       return elem->m_Uuid.AsString() == item1->m_Uuid.AsString();
                   };

        auto item2 = std::find_if( items2.begin(), items2.end(), cmp );

        BOOST_CHECK( item2 != items2.end() );
    }
}


// Check if two GROUPS are identical by comparing the groups in each of them.
void testGroupsEqual( const GROUPS& groups1, const GROUPS& groups2 )
{
    BOOST_CHECK_EQUAL( groups1.size(), groups2.size() );

    for( PCB_GROUP* group1 : groups1 )
    {
        auto cmp = [&]( BOARD_ITEM* elem )
                   {
                       return elem->m_Uuid.AsString() == group1->m_Uuid.AsString();
                   };

        auto group2 = std::find_if( groups2.begin(), groups2.end(), cmp );

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
    std::unique_ptr<BOARD> board1 = createBoard( spec );
    auto path = std::filesystem::temp_directory_path() / "group_saveload_tst.kicad_pcb";
    ::KI_TEST::DumpBoardToFile( *board1, path.string() );

    std::unique_ptr<BOARD> board2 = ::KI_TEST::ReadBoardFromFileOrStream( path.string() );
    testGroupsEqual( board1->Groups(), board2->Groups() );
}


// Test saving & loading of a few configurations.
// Groups with fewer than 2 members are not saved, so all round-trip tests
// use groups with at least 2 members.
BOOST_AUTO_TEST_CASE( HealthyGroups )
{
    // Test board with no groups
    testSaveLoad( {} );

    // Single group with 2 members
    testSaveLoad( { { TEXT0, TEXT1 } } );

    // Two groups
    testSaveLoad( { { TEXT0, TEXT1 }, { TEXT2, TEXT3 } } );
    testSaveLoad( { { TEXT0, TEXT1 }, { TEXT2, GROUP0 } } );

    // Subgroups with 2+ members each
    testSaveLoad( { { TEXT0, TEXT1, GROUP1 }, { TEXT2, TEXT3 }, { TEXT4, GROUP0 } } );
    testSaveLoad( { { TEXT0, TEXT1 }, { TEXT2, TEXT3 }, { GROUP1, GROUP0 } } );
    testSaveLoad( { { TEXT0, TEXT1 }, { TEXT2, TEXT3 }, { TEXT4, NAME_GROUP3 }, { TEXT5, TEXT6 } } );
}


BOOST_AUTO_TEST_CASE( SingleMemberGroupsSaved )
{
    std::unique_ptr<BOARD> board1 = createBoard( { { TEXT0 } } );
    auto path = std::filesystem::temp_directory_path() / "group_saveload_tst.kicad_pcb";
    ::KI_TEST::DumpBoardToFile( *board1, path.string() );

    std::unique_ptr<BOARD> board2 = ::KI_TEST::ReadBoardFromFileOrStream( path.string() );
    BOOST_CHECK_EQUAL( board2->Groups().size(), 1u );
}


// TODO: this is *probably* not needed any more as long as nothing is bypassing AddItem's
// check for cyclic group membership, but it doesn't hurt to have it as a sanity check for the groups graph.
BOOST_AUTO_TEST_CASE( InvalidGroups )
{
    // A cycle
    std::unique_ptr<BOARD> board1 = createBoard( { { TEXT0, GROUP1 }, { TEXT2, GROUP0 } }, true );
    BOOST_CHECK_EQUAL( board1->GroupsSanityCheck(), "Cycle detected in group membership" );

    // More complex cycle
    board1 = createBoard(
            { { TEXT0, GROUP1 }, { TEXT1 }, { TEXT2, NAME_GROUP4 }, { TEXT3, GROUP2 }, { TEXT4, NAME_GROUP3 } }, true );
    BOOST_CHECK_EQUAL( board1->GroupsSanityCheck(), "Cycle detected in group membership" );

    // Delete the removed group since the test is over
    board1.reset( nullptr );
    delete s_removedGroup;
    s_removedGroup = nullptr;

    // Delete the removed text since the test is over
    board1.reset( nullptr );
    delete s_removedText;
    s_removedText = nullptr;
}


/**
 * Verify that PCB_GROUP::DeepClone produces a group whose m_items reference
 * the cloned children rather than the originals, and that the cloned group
 * round-trips through save/load with correct membership.
 */
BOOST_AUTO_TEST_CASE( DeepCloneGroupMembership )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    PCB_TEXT* text0 = new PCB_TEXT( board.get() );
    text0->SetText( wxT( "child-0" ) );
    board->Add( text0 );

    PCB_TEXT* text1 = new PCB_TEXT( board.get() );
    text1->SetText( wxT( "child-1" ) );
    board->Add( text1 );

    PCB_GROUP* group = new PCB_GROUP( board.get() );
    group->SetName( wxT( "TestGroup" ) );
    group->AddItem( text0 );
    group->AddItem( text1 );
    board->Add( group );

    BOOST_CHECK_EQUAL( group->GetItems().size(), 2 );

    PCB_GROUP* deepCopy = group->DeepClone();

    // DeepClone preserves the UUID
    BOOST_CHECK_EQUAL( deepCopy->m_Uuid.AsString(), group->m_Uuid.AsString() );
    BOOST_CHECK_EQUAL( deepCopy->GetName(), group->GetName() );
    BOOST_CHECK_EQUAL( deepCopy->GetItems().size(), 2 );

    // The cloned group's children must be different objects from the originals
    for( EDA_ITEM* clonedChild : deepCopy->GetItems() )
    {
        BOOST_CHECK( clonedChild != text0 );
        BOOST_CHECK( clonedChild != text1 );
    }

    // Children must NOT be the same pointers as the original group's children
    for( EDA_ITEM* clonedChild : deepCopy->GetItems() )
    {
        bool foundInOriginal = group->GetItems().count( clonedChild ) > 0;
        BOOST_CHECK_MESSAGE( !foundInOriginal,
                             "DeepClone child should not be in original group's m_items" );
    }

    // Round-trip: add the deep clone and its children to a temp board, save, reload
    std::unique_ptr<BOARD> tempBoard = std::make_unique<BOARD>();
    tempBoard->Add( deepCopy );

    deepCopy->RunOnChildren(
            [&]( BOARD_ITEM* child )
            {
                tempBoard->Add( child, ADD_MODE::APPEND, false );
            },
            RECURSE_MODE::RECURSE );

    auto path = std::filesystem::temp_directory_path() / "group_deepclone_tst.kicad_pcb";
    ::KI_TEST::DumpBoardToFile( *tempBoard, path.string() );

    std::unique_ptr<BOARD> reloaded = ::KI_TEST::ReadBoardFromFileOrStream( path.string() );

    BOOST_CHECK_EQUAL( reloaded->Groups().size(), 1 );

    if( !reloaded->Groups().empty() )
    {
        PCB_GROUP* loadedGroup = static_cast<PCB_GROUP*>( reloaded->Groups().front() );
        BOOST_CHECK_EQUAL( loadedGroup->GetItems().size(), 2 );
        BOOST_CHECK_EQUAL( loadedGroup->GetName(), wxT( "TestGroup" ) );
    }
}


/**
 * Verify that PCB_GENERATOR::DeepClone correctly recurses into nested generators,
 * not just nested groups. Without this, a generator containing another generator
 * would leave the inner generator's m_items pointing at original board items.
 */
BOOST_AUTO_TEST_CASE( DeepCloneNestedGeneratorMembership )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    PCB_TEXT* text = new PCB_TEXT( board.get() );
    text->SetText( wxT( "generated-child" ) );
    board->Add( text );

    PCB_TUNING_PATTERN* nested = new PCB_TUNING_PATTERN( board.get() );
    nested->AddItem( text );
    board->Add( nested );

    PCB_TUNING_PATTERN* root = new PCB_TUNING_PATTERN( board.get() );
    root->AddItem( nested );
    board->Add( root );

    PCB_GENERATOR* deepCopy = root->DeepClone();

    BOOST_CHECK_EQUAL( deepCopy->GetItems().size(), 1 );

    EDA_ITEM* clonedNestedRaw = *deepCopy->GetItems().begin();
    BOOST_CHECK( clonedNestedRaw != nested );
    BOOST_CHECK_EQUAL( clonedNestedRaw->Type(), PCB_GENERATOR_T );

    PCB_GENERATOR* clonedNested = static_cast<PCB_GENERATOR*>( clonedNestedRaw );
    BOOST_CHECK_EQUAL( clonedNested->GetItems().size(), 1 );

    for( EDA_ITEM* member : clonedNested->GetItems() )
    {
        BOOST_CHECK( member != text );
    }

    // Clean up the deep copy tree (not owned by any board)
    EDA_ITEM* clonedText = *clonedNested->GetItems().begin();
    delete clonedText;
    delete clonedNested;
    delete deepCopy;
}


BOOST_AUTO_TEST_SUITE_END()
