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
 * @file test_issue22620_group_annotation.cpp
 * Test for issue #22620: Symbols are not automatically annotated when placing design blocks
 *
 * When placing a design block as a group with auto-annotation enabled and "Keep Annotations"
 * disabled, symbols in the group should be automatically annotated. The root cause was that
 * getInferredSymbols() in annotate.cpp did not handle SCH_GROUP_T items, so symbols inside
 * groups were not found and thus not annotated.
 *
 * This test verifies that SCH_GROUP::RunOnChildren correctly iterates through group members,
 * which is the mechanism used by the fix.
 */

#include <boost/test/unit_test.hpp>

#include <sch_group.h>
#include <sch_screen.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <settings/settings_manager.h>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <wx/filename.h>
#include <wx/stdpaths.h>


struct GROUP_ANNOTATION_FIXTURE
{
    GROUP_ANNOTATION_FIXTURE() :
            m_settingsManager()
    {
        wxString tempDir = wxStandardPaths::Get().GetTempDir();
        wxString projectPath = tempDir + wxFileName::GetPathSeparator()
                               + wxT( "test_group_annotation.kicad_pro" );
        m_tempFiles.push_back( projectPath );

        m_settingsManager.LoadProject( projectPath.ToStdString() );
        m_schematic = std::make_unique<SCHEMATIC>( nullptr );
        m_project = &m_settingsManager.Prj();
        m_schematic->SetProject( m_project );
    }

    ~GROUP_ANNOTATION_FIXTURE()
    {
        for( const wxString& file : m_tempFiles )
        {
            if( wxFileExists( file ) )
                wxRemoveFile( file );
        }

        m_schematic.reset();
    }

    SETTINGS_MANAGER m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
    PROJECT* m_project;
    std::vector<wxString> m_tempFiles;
};


BOOST_FIXTURE_TEST_SUITE( Issue22620GroupAnnotation, GROUP_ANNOTATION_FIXTURE )


/**
 * Test that SCH_GROUP::RunOnChildren correctly finds symbols inside a group.
 * This verifies the mechanism used by the fix for issue #22620.
 */
BOOST_AUTO_TEST_CASE( TestGroupRunOnChildrenFindsSymbols )
{
    m_schematic->CreateDefaultScreens();

    std::vector<SCH_SHEET*> topSheets = m_schematic->GetTopLevelSheets();
    BOOST_REQUIRE( !topSheets.empty() );

    SCH_SCREEN* screen = topSheets[0]->GetScreen();
    BOOST_REQUIRE( screen != nullptr );

    // Create two simple symbols
    SCH_SYMBOL* symbol1 = new SCH_SYMBOL();
    symbol1->SetPosition( VECTOR2I( 0, 0 ) );
    screen->Append( symbol1 );

    SCH_SYMBOL* symbol2 = new SCH_SYMBOL();
    symbol2->SetPosition( VECTOR2I( 1000000, 0 ) );
    screen->Append( symbol2 );

    // Create a group containing both symbols
    SCH_GROUP* group = new SCH_GROUP( screen );
    group->SetName( wxT( "DesignBlock" ) );
    group->AddItem( symbol1 );
    group->AddItem( symbol2 );
    screen->Append( group );

    // Use RunOnChildren to find symbols in the group
    std::unordered_set<SCH_SYMBOL*> foundSymbols;

    group->RunOnChildren(
            [&foundSymbols]( SCH_ITEM* aChild )
            {
                if( aChild->Type() == SCH_SYMBOL_T )
                    foundSymbols.insert( static_cast<SCH_SYMBOL*>( aChild ) );
            },
            RECURSE_MODE::RECURSE );

    // Verify both symbols were found
    BOOST_CHECK_EQUAL( foundSymbols.size(), 2 );
    BOOST_CHECK( foundSymbols.count( symbol1 ) == 1 );
    BOOST_CHECK( foundSymbols.count( symbol2 ) == 1 );

    BOOST_TEST_MESSAGE( "Test passed: RunOnChildren correctly finds symbols in groups" );
}


/**
 * Test that SCH_GROUP::RunOnChildren handles nested groups correctly.
 */
BOOST_AUTO_TEST_CASE( TestGroupRunOnChildrenWithNestedGroups )
{
    m_schematic->CreateDefaultScreens();

    std::vector<SCH_SHEET*> topSheets = m_schematic->GetTopLevelSheets();
    BOOST_REQUIRE( !topSheets.empty() );

    SCH_SCREEN* screen = topSheets[0]->GetScreen();
    BOOST_REQUIRE( screen != nullptr );

    // Create symbols
    SCH_SYMBOL* symbol1 = new SCH_SYMBOL();
    symbol1->SetPosition( VECTOR2I( 0, 0 ) );
    screen->Append( symbol1 );

    SCH_SYMBOL* symbol2 = new SCH_SYMBOL();
    symbol2->SetPosition( VECTOR2I( 1000000, 0 ) );
    screen->Append( symbol2 );

    SCH_SYMBOL* symbol3 = new SCH_SYMBOL();
    symbol3->SetPosition( VECTOR2I( 2000000, 0 ) );
    screen->Append( symbol3 );

    // Create an inner group with symbol1
    SCH_GROUP* innerGroup = new SCH_GROUP( screen );
    innerGroup->SetName( wxT( "InnerGroup" ) );
    innerGroup->AddItem( symbol1 );
    screen->Append( innerGroup );

    // Create an outer group containing the inner group and symbol2
    SCH_GROUP* outerGroup = new SCH_GROUP( screen );
    outerGroup->SetName( wxT( "OuterGroup" ) );
    outerGroup->AddItem( innerGroup );
    outerGroup->AddItem( symbol2 );
    screen->Append( outerGroup );

    // Use RunOnChildren with RECURSE to find all symbols in nested groups
    std::unordered_set<SCH_SYMBOL*> foundSymbols;

    outerGroup->RunOnChildren(
            [&foundSymbols]( SCH_ITEM* aChild )
            {
                if( aChild->Type() == SCH_SYMBOL_T )
                    foundSymbols.insert( static_cast<SCH_SYMBOL*>( aChild ) );
            },
            RECURSE_MODE::RECURSE );

    // Verify both symbol1 (in nested group) and symbol2 were found
    BOOST_CHECK_EQUAL( foundSymbols.size(), 2 );
    BOOST_CHECK( foundSymbols.count( symbol1 ) == 1 );
    BOOST_CHECK( foundSymbols.count( symbol2 ) == 1 );
    BOOST_CHECK( foundSymbols.count( symbol3 ) == 0 ); // symbol3 is not in any group

    BOOST_TEST_MESSAGE( "Test passed: RunOnChildren correctly handles nested groups" );
}


BOOST_AUTO_TEST_SUITE_END()
