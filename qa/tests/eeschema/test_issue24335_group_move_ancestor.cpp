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

/**
 * @file test_issue24335_group_move_ancestor.cpp
 * Test for issue #24335: Moving of grouped components breaks wire positions
 *
 * Root cause: getConnectedDragItems() in sch_move_tool.cpp used a plain IsSelected() check on
 * candidate items to decide whether to skip them. Items that move as members of a selected group
 * do not carry the SELECTED flag themselves, so they were treated as fixed anchors. When an
 * outside wire endpoint touched a symbol pin belonging to such a group member, a spurious stub
 * wire was synthesised at the group boundary, producing the broken diagonal wires observed in the
 * report.
 *
 * The fix introduces EDA_ITEM::HasSelectedAncestorGroup() which walks the parent-group chain and
 * returns true if any ancestor group is selected. This test exercises that helper for the
 * scenarios used inside getConnectedDragItems.
 */

#include <boost/test/unit_test.hpp>

#include <eda_item.h>
#include <sch_field.h>
#include <sch_group.h>
#include <sch_line.h>
#include <sch_screen.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <settings/settings_manager.h>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <wx/filename.h>
#include <wx/stdpaths.h>


struct GROUP_MOVE_ANCESTOR_FIXTURE
{
    GROUP_MOVE_ANCESTOR_FIXTURE() :
            m_settingsManager()
    {
        wxString tempDir = wxStandardPaths::Get().GetTempDir();
        wxString projectPath = tempDir + wxFileName::GetPathSeparator()
                               + wxT( "test_issue24335.kicad_pro" );
        m_tempFiles.push_back( projectPath );

        m_settingsManager.LoadProject( projectPath.ToStdString() );
        m_schematic = std::make_unique<SCHEMATIC>( nullptr );
        m_schematic->SetProject( &m_settingsManager.Prj() );
        m_schematic->CreateDefaultScreens();
    }

    ~GROUP_MOVE_ANCESTOR_FIXTURE()
    {
        m_schematic.reset();

        for( const wxString& file : m_tempFiles )
        {
            if( wxFileExists( file ) )
                wxRemoveFile( file );
        }
    }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
    std::vector<wxString>      m_tempFiles;
};


BOOST_FIXTURE_TEST_SUITE( Issue24335GroupMoveAncestor, GROUP_MOVE_ANCESTOR_FIXTURE )


BOOST_AUTO_TEST_CASE( SymbolInsideSelectedGroupReportsAncestor )
{
    SCH_SCREEN* screen = m_schematic->GetTopLevelSheets()[0]->GetScreen();
    BOOST_REQUIRE( screen );

    SCH_SYMBOL* symbol = new SCH_SYMBOL();
    symbol->SetPosition( VECTOR2I( 0, 0 ) );
    screen->Append( symbol );

    SCH_GROUP* group = new SCH_GROUP( screen );
    group->AddItem( symbol );
    screen->Append( group );

    // Baseline: nothing is selected.
    BOOST_CHECK( !symbol->IsSelected() );
    BOOST_CHECK( !symbol->HasSelectedAncestorGroup() );

    // Selecting the group must propagate to "ancestor is selected" for the symbol even though the
    // symbol itself does not carry the SELECTED flag (group selection replaces members in the
    // collector).
    group->SetSelected();

    BOOST_CHECK( !symbol->IsSelected() );
    BOOST_CHECK( symbol->HasSelectedAncestorGroup() );

    group->ClearSelected();
    BOOST_CHECK( !symbol->HasSelectedAncestorGroup() );
}


BOOST_AUTO_TEST_CASE( NestedGroupAncestorIsDetected )
{
    SCH_SCREEN* screen = m_schematic->GetTopLevelSheets()[0]->GetScreen();
    BOOST_REQUIRE( screen );

    SCH_SYMBOL* symbol = new SCH_SYMBOL();
    symbol->SetPosition( VECTOR2I( 0, 0 ) );
    screen->Append( symbol );

    SCH_GROUP* inner = new SCH_GROUP( screen );
    inner->AddItem( symbol );
    screen->Append( inner );

    SCH_GROUP* outer = new SCH_GROUP( screen );
    outer->AddItem( inner );
    screen->Append( outer );

    BOOST_REQUIRE( symbol->GetParentGroup() == inner );
    BOOST_REQUIRE( inner->GetParentGroup() == outer );

    // Selecting only the outer group must still be picked up by the symbol's ancestor walk.
    outer->SetSelected();
    BOOST_CHECK( symbol->HasSelectedAncestorGroup() );
    BOOST_CHECK( inner->HasSelectedAncestorGroup() );

    outer->ClearSelected();
    inner->SetSelected();
    BOOST_CHECK( symbol->HasSelectedAncestorGroup() );
    BOOST_CHECK( !inner->HasSelectedAncestorGroup() );
}


BOOST_AUTO_TEST_CASE( SymbolFieldInheritsAncestorGroupViaParent )
{
    SCH_SCREEN* screen = m_schematic->GetTopLevelSheets()[0]->GetScreen();
    BOOST_REQUIRE( screen );

    // Symbol fields are children of the symbol (GetParent() == symbol). When the symbol's group
    // is selected, the field must also be recognised as moving with the group via the parent walk.
    SCH_SYMBOL* symbol = new SCH_SYMBOL();
    symbol->SetPosition( VECTOR2I( 0, 0 ) );
    screen->Append( symbol );

    SCH_GROUP* group = new SCH_GROUP( screen );
    group->AddItem( symbol );
    screen->Append( group );

    SCH_FIELD* field = symbol->GetField( FIELD_T::REFERENCE );
    BOOST_REQUIRE( field );
    BOOST_REQUIRE( field->GetParent() == symbol );
    BOOST_REQUIRE( field->GetParentGroup() == nullptr );

    group->SetSelected();

    BOOST_CHECK( field->HasSelectedAncestorGroup() );
}


BOOST_AUTO_TEST_CASE( WireInsideSelectedGroupReportsAncestor )
{
    SCH_SCREEN* screen = m_schematic->GetTopLevelSheets()[0]->GetScreen();
    BOOST_REQUIRE( screen );

    SCH_LINE* wire = new SCH_LINE( VECTOR2I( 0, 0 ), LAYER_WIRE );
    wire->SetEndPoint( VECTOR2I( 100000, 0 ) );
    screen->Append( wire );

    SCH_GROUP* group = new SCH_GROUP( screen );
    group->AddItem( wire );
    screen->Append( group );

    group->SetSelected();

    // The connectivity walker relies on this signal to avoid double-moving in-group wires.
    BOOST_CHECK( wire->HasSelectedAncestorGroup() );
}


BOOST_AUTO_TEST_SUITE_END()
