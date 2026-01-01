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
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file test_design_block_duplicate.cpp
 * Test for issue #22060: Multiple identical design blocks break connections
 *
 * When placing multiple instances of the same design block in the same sheet,
 * connections should be preserved after save/reload. This test verifies that
 * groups and their members maintain correct associations.
 */

#include <boost/test/unit_test.hpp>
#include <eeschema_test_utils.h>

#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_group.h>
#include <sch_line.h>
#include <sch_junction.h>
#include <schematic.h>
#include <kiid.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <wx/filename.h>
#include <wx/stdpaths.h>


struct DESIGN_BLOCK_FIXTURE
{
    DESIGN_BLOCK_FIXTURE() :
            m_settingsManager()
    {
        wxString tempDir = wxStandardPaths::Get().GetTempDir();
        wxString projectPath = tempDir + wxFileName::GetPathSeparator()
                               + wxT( "test_design_block.kicad_pro" );
        m_tempFiles.push_back( projectPath );

        m_settingsManager.LoadProject( projectPath.ToStdString() );
        m_schematic = std::make_unique<SCHEMATIC>( nullptr );
        m_project = &m_settingsManager.Prj();
        m_schematic->SetProject( m_project );
    }

    ~DESIGN_BLOCK_FIXTURE()
    {
        for( const wxString& file : m_tempFiles )
        {
            if( wxFileExists( file ) )
                wxRemoveFile( file );
        }

        m_schematic.reset();
    }

    wxString GetTempFileName( const wxString& aPrefix )
    {
        wxString tempDir = wxStandardPaths::Get().GetTempDir();
        wxString fileName = wxFileName::CreateTempFileName(
                tempDir + wxFileName::GetPathSeparator() + aPrefix );
        m_tempFiles.push_back( fileName );
        return fileName;
    }

    /**
     * Create a simple design block content: a wire and junction in a group
     */
    void CreateDesignBlockContent( SCH_SCREEN* aScreen, SCH_GROUP** aOutGroup,
                                   const VECTOR2I& aOffset )
    {
        // Create a wire
        SCH_LINE* wire = new SCH_LINE( aOffset + VECTOR2I( 0, 0 ), LAYER_WIRE );
        wire->SetEndPoint( aOffset + VECTOR2I( 1000000, 0 ) ); // 10mm wire
        aScreen->Append( wire );

        // Create a junction at the wire endpoint
        SCH_JUNCTION* junction = new SCH_JUNCTION( aOffset + VECTOR2I( 1000000, 0 ) );
        aScreen->Append( junction );

        // Create a group containing both items
        SCH_GROUP* group = new SCH_GROUP( aScreen );
        group->SetName( "DesignBlock" );
        group->AddItem( wire );
        group->AddItem( junction );
        aScreen->Append( group );

        if( aOutGroup )
            *aOutGroup = group;
    }

    SETTINGS_MANAGER m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
    PROJECT* m_project;
    std::vector<wxString> m_tempFiles;
};


BOOST_FIXTURE_TEST_SUITE( DesignBlockDuplicate, DESIGN_BLOCK_FIXTURE )


/**
 * Test that multiple design block instances maintain correct group membership after save/reload
 * This is a regression test for issue #22060
 */
BOOST_AUTO_TEST_CASE( TestMultipleDesignBlocksGroupIntegrity )
{
    // Create a simple schematic
    m_schematic->CreateDefaultScreens();

    std::vector<SCH_SHEET*> topSheets = m_schematic->GetTopLevelSheets();
    BOOST_REQUIRE( !topSheets.empty() );

    SCH_SCREEN* screen = topSheets[0]->GetScreen();
    BOOST_REQUIRE( screen != nullptr );

    screen->SetFileName( "test_design_block.kicad_sch" );

    // Create first "design block" instance
    SCH_GROUP* group1 = nullptr;
    CreateDesignBlockContent( screen, &group1, VECTOR2I( 0, 0 ) );
    BOOST_REQUIRE( group1 != nullptr );

    KIID group1Uuid = group1->m_Uuid;
    size_t group1MemberCount = group1->GetItems().size();
    BOOST_CHECK_EQUAL( group1MemberCount, 2 ); // Wire + junction

    // Create second "design block" instance at a different position
    SCH_GROUP* group2 = nullptr;
    CreateDesignBlockContent( screen, &group2, VECTOR2I( 5000000, 0 ) ); // 50mm offset
    BOOST_REQUIRE( group2 != nullptr );

    KIID group2Uuid = group2->m_Uuid;
    size_t group2MemberCount = group2->GetItems().size();
    BOOST_CHECK_EQUAL( group2MemberCount, 2 ); // Wire + junction

    // Verify both groups exist and are distinct
    BOOST_CHECK( group1Uuid != group2Uuid );

    // Count total groups before save
    int groupCountBefore = 0;

    for( SCH_ITEM* item : screen->Items().OfType( SCH_GROUP_T ) )
    {
        groupCountBefore++;
        SCH_GROUP* group = static_cast<SCH_GROUP*>( item );
        BOOST_TEST_MESSAGE( "Group before save: " << group->GetName().ToStdString()
                            << " UUID: " << group->m_Uuid.AsString().ToStdString()
                            << " Members: " << group->GetItems().size() );
    }

    BOOST_CHECK_EQUAL( groupCountBefore, 2 );

    // Save the schematic
    wxString fileName = GetTempFileName( "test_design_block" );
    fileName += ".kicad_sch";
    m_tempFiles.push_back( fileName );

    SCH_IO_KICAD_SEXPR io;
    BOOST_CHECK_NO_THROW( io.SaveSchematicFile( fileName, topSheets[0], m_schematic.get() ) );
    BOOST_CHECK( wxFileExists( fileName ) );

    // Reset and reload
    m_schematic->Reset();
    SCH_SHEET* defaultSheet = m_schematic->GetTopLevelSheet( 0 );
    SCH_SHEET* loadedSheet = nullptr;

    BOOST_CHECK_NO_THROW( loadedSheet = io.LoadSchematicFile( fileName, m_schematic.get() ) );
    BOOST_REQUIRE( loadedSheet != nullptr );

    m_schematic->AddTopLevelSheet( loadedSheet );
    m_schematic->RemoveTopLevelSheet( defaultSheet );
    delete defaultSheet;

    SCH_SCREEN* loadedScreen = loadedSheet->GetScreen();
    BOOST_REQUIRE( loadedScreen != nullptr );

    // Verify groups after reload
    int groupCountAfter = 0;
    int totalMembersAfter = 0;

    for( SCH_ITEM* item : loadedScreen->Items().OfType( SCH_GROUP_T ) )
    {
        groupCountAfter++;
        SCH_GROUP* group = static_cast<SCH_GROUP*>( item );

        BOOST_TEST_MESSAGE( "Group after load: " << group->GetName().ToStdString()
                            << " UUID: " << group->m_Uuid.AsString().ToStdString()
                            << " Members: " << group->GetItems().size() );

        // Each group should still have exactly 2 members
        BOOST_CHECK_EQUAL( group->GetItems().size(), 2 );
        totalMembersAfter += group->GetItems().size();

        // Verify members are actual items on the screen
        for( EDA_ITEM* member : group->GetItems() )
        {
            bool found = false;

            for( SCH_ITEM* screenItem : loadedScreen->Items() )
            {
                if( screenItem == member )
                {
                    found = true;
                    break;
                }
            }

            BOOST_CHECK_MESSAGE( found, "Group member should exist on screen" );
        }
    }

    // CRITICAL: We should have 2 groups, each with 2 members
    BOOST_CHECK_EQUAL( groupCountAfter, 2 );
    BOOST_CHECK_EQUAL( totalMembersAfter, 4 );

    BOOST_TEST_MESSAGE( "Test passed: Groups maintained integrity after save/reload" );
}


/**
 * Test that ReplaceDuplicateTimeStamps correctly handles ALL item types
 * (wires, junctions, groups), not just hierarchical items (symbols, sheets, labels).
 * This is the core fix for issue #22060.
 */
BOOST_AUTO_TEST_CASE( TestDesignBlockDuplicateUuidHandling )
{
    m_schematic->CreateDefaultScreens();

    std::vector<SCH_SHEET*> topSheets = m_schematic->GetTopLevelSheets();
    BOOST_REQUIRE( !topSheets.empty() );

    SCH_SCREEN* mainScreen = topSheets[0]->GetScreen();
    BOOST_REQUIRE( mainScreen != nullptr );
    mainScreen->SetFileName( "test_duplicate_uuid.kicad_sch" );

    // To properly simulate design block placement, we simulate what happens when
    // the same design block file is loaded twice: groups are created pointing to
    // the items they were loaded with, then items are moved to the target screen.
    //
    // We'll create two sets of items with the SAME UUIDs (as if loaded from
    // the same design block file), with groups properly pointing to their own items.

    // Fixed UUIDs to simulate loading from same source file
    KIID wireUuid1( "11111111-1111-1111-1111-111111111111" );
    KIID junctionUuid1( "22222222-2222-2222-2222-222222222222" );
    KIID groupUuid1( "33333333-3333-3333-3333-333333333333" );

    // First "design block placement" - create items at origin
    SCH_LINE* wire1 = new SCH_LINE( VECTOR2I( 0, 0 ), LAYER_WIRE );
    wire1->SetEndPoint( VECTOR2I( 1000000, 0 ) );
    const_cast<KIID&>( wire1->m_Uuid ) = wireUuid1;
    mainScreen->Append( wire1 );

    SCH_JUNCTION* junction1 = new SCH_JUNCTION( VECTOR2I( 1000000, 0 ) );
    const_cast<KIID&>( junction1->m_Uuid ) = junctionUuid1;
    mainScreen->Append( junction1 );

    SCH_GROUP* group1 = new SCH_GROUP( mainScreen );
    group1->SetName( "DesignBlock" );
    const_cast<KIID&>( group1->m_Uuid ) = groupUuid1;
    group1->AddItem( wire1 );
    group1->AddItem( junction1 );
    mainScreen->Append( group1 );

    // Second "design block placement" - create items at offset with SAME UUIDs
    // (simulating loading same design block file again)
    SCH_LINE* wire2 = new SCH_LINE( VECTOR2I( 5000000, 0 ), LAYER_WIRE );
    wire2->SetEndPoint( VECTOR2I( 6000000, 0 ) );
    const_cast<KIID&>( wire2->m_Uuid ) = wireUuid1;  // SAME UUID!
    mainScreen->Append( wire2 );

    SCH_JUNCTION* junction2 = new SCH_JUNCTION( VECTOR2I( 6000000, 0 ) );
    const_cast<KIID&>( junction2->m_Uuid ) = junctionUuid1;  // SAME UUID!
    mainScreen->Append( junction2 );

    SCH_GROUP* group2 = new SCH_GROUP( mainScreen );
    group2->SetName( "DesignBlock" );
    const_cast<KIID&>( group2->m_Uuid ) = groupUuid1;  // SAME UUID!
    group2->AddItem( wire2 );  // Points to wire2, not wire1
    group2->AddItem( junction2 );  // Points to junction2, not junction1
    mainScreen->Append( group2 );

    // Verify we have duplicate UUIDs
    std::map<KIID, int> uuidCounts;

    for( SCH_ITEM* item : mainScreen->Items() )
        uuidCounts[item->m_Uuid]++;

    int duplicatesFound = 0;

    for( const auto& pair : uuidCounts )
    {
        if( pair.second > 1 )
        {
            duplicatesFound += pair.second - 1;
            BOOST_TEST_MESSAGE( "Found duplicate UUID: " << pair.first.AsString().ToStdString()
                                << " count: " << pair.second );
        }
    }

    // Should have 3 duplicate pairs: wire, junction, group
    BOOST_CHECK_EQUAL( duplicatesFound, 3 );

    // This is the critical test: ReplaceDuplicateTimeStamps should now handle
    // ALL item types, not just hierarchical items
    SCH_SCREENS screens( topSheets[0] );
    int replaced = screens.ReplaceDuplicateTimeStamps();

    BOOST_TEST_MESSAGE( "ReplaceDuplicateTimeStamps replaced: " << replaced );

    // Should replace exactly 3 items (one from each duplicate pair)
    BOOST_CHECK_EQUAL( replaced, 3 );

    // Verify UUIDs are now unique
    uuidCounts.clear();

    for( SCH_ITEM* item : mainScreen->Items() )
        uuidCounts[item->m_Uuid]++;

    for( const auto& pair : uuidCounts )
    {
        BOOST_CHECK_MESSAGE( pair.second == 1,
                             "UUID should be unique after ReplaceDuplicateTimeStamps: "
                             << pair.first.AsString().ToStdString() );
    }

    // Verify both groups still have their correct members
    int groupCount = 0;

    for( SCH_ITEM* item : mainScreen->Items().OfType( SCH_GROUP_T ) )
    {
        groupCount++;
        SCH_GROUP* group = static_cast<SCH_GROUP*>( item );

        BOOST_TEST_MESSAGE( "Group: " << group->GetName().ToStdString()
                            << " UUID: " << group->m_Uuid.AsString().ToStdString()
                            << " Members: " << group->GetItems().size() );

        // Each group should have exactly 2 members
        BOOST_CHECK_EQUAL( group->GetItems().size(), 2 );

        // Verify all members are on the screen
        for( EDA_ITEM* member : group->GetItems() )
        {
            bool memberOnScreen = false;

            for( SCH_ITEM* screenItem : mainScreen->Items() )
            {
                if( screenItem == member )
                {
                    memberOnScreen = true;
                    break;
                }
            }

            BOOST_CHECK_MESSAGE( memberOnScreen,
                                 "Group member should be on screen. Member UUID: "
                                 << member->m_Uuid.AsString().ToStdString() );
        }
    }

    BOOST_CHECK_EQUAL( groupCount, 2 );

    // Save and reload to test full round-trip
    wxString fileName = GetTempFileName( "test_dup_uuid" );
    fileName += ".kicad_sch";
    m_tempFiles.push_back( fileName );

    SCH_IO_KICAD_SEXPR io;
    BOOST_CHECK_NO_THROW( io.SaveSchematicFile( fileName, topSheets[0], m_schematic.get() ) );

    m_schematic->Reset();
    SCH_SHEET* defaultSheet = m_schematic->GetTopLevelSheet( 0 );
    SCH_SHEET* loadedSheet = io.LoadSchematicFile( fileName, m_schematic.get() );
    BOOST_REQUIRE( loadedSheet != nullptr );

    m_schematic->AddTopLevelSheet( loadedSheet );
    m_schematic->RemoveTopLevelSheet( defaultSheet );
    delete defaultSheet;

    SCH_SCREEN* loadedScreen = loadedSheet->GetScreen();

    // Verify groups after reload - each should have 2 members
    groupCount = 0;
    int membersWithValidPointers = 0;

    for( SCH_ITEM* item : loadedScreen->Items().OfType( SCH_GROUP_T ) )
    {
        groupCount++;
        SCH_GROUP* group = static_cast<SCH_GROUP*>( item );

        BOOST_TEST_MESSAGE( "Loaded group: " << group->GetName().ToStdString()
                            << " Members: " << group->GetItems().size() );

        // CRITICAL CHECK: Each group should have 2 members after reload
        BOOST_CHECK_EQUAL( group->GetItems().size(), 2 );

        for( EDA_ITEM* member : group->GetItems() )
        {
            bool memberOnScreen = false;

            for( SCH_ITEM* screenItem : loadedScreen->Items() )
            {
                if( screenItem == member )
                {
                    memberOnScreen = true;
                    membersWithValidPointers++;
                    break;
                }
            }

            BOOST_CHECK_MESSAGE( memberOnScreen,
                                 "Loaded group member should be on screen" );
        }
    }

    BOOST_CHECK_EQUAL( groupCount, 2 );
    BOOST_CHECK_EQUAL( membersWithValidPointers, 4 ); // 2 groups * 2 members each

    BOOST_TEST_MESSAGE( "Test passed: Duplicate UUIDs correctly handled and groups preserved" );
}


BOOST_AUTO_TEST_SUITE_END()
