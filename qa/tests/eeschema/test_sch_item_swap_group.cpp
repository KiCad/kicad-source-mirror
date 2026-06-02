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
 * @file test_sch_item_swap_group.cpp
 * Regression test: SCH_ITEM::SwapItemData must not change group membership.
 *
 * When a design block (group) move is undone or canceled, each changed item's data is restored
 * with SCH_ITEM::SwapItemData against a saved copy. That copy carries the item's data but no group
 * pointer, so swapping m_group used to null the live item's group, leaving it orphaned from the
 * group that still listed it as a member. The wire then behaved as a loose item and the block
 * could tear apart on a later move.
 *
 * Group membership is structural and is restored on its own (SCH_GROUP::swapData and the undo
 * group-pointer pass), so a plain data swap must leave it untouched.
 */

#include <boost/test/unit_test.hpp>

#include <sch_group.h>
#include <sch_line.h>
#include <sch_screen.h>
#include <schematic.h>
#include <settings/settings_manager.h>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <wx/filename.h>
#include <wx/stdpaths.h>

#include <memory>


struct SCH_ITEM_SWAP_GROUP_FIXTURE
{
    SCH_ITEM_SWAP_GROUP_FIXTURE()
    {
        wxString tempDir = wxStandardPaths::Get().GetTempDir();
        wxString projectPath = tempDir + wxFileName::GetPathSeparator() + wxT( "test_sch_item_swap_group.kicad_pro" );
        m_tempFiles.push_back( projectPath );

        m_settingsManager.LoadProject( projectPath.ToStdString() );
        m_schematic = std::make_unique<SCHEMATIC>( nullptr );
        m_schematic->SetProject( &m_settingsManager.Prj() );
        m_schematic->CreateDefaultScreens();
    }

    ~SCH_ITEM_SWAP_GROUP_FIXTURE()
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


BOOST_FIXTURE_TEST_SUITE( SchItemSwapGroup, SCH_ITEM_SWAP_GROUP_FIXTURE )


BOOST_AUTO_TEST_CASE( SwapItemDataKeepsGroupMembership )
{
    SCH_SCREEN* screen = m_schematic->GetTopLevelSheets()[0]->GetScreen();
    BOOST_REQUIRE( screen );

    SCH_LINE* wire = new SCH_LINE( VECTOR2I( 0, 0 ), LAYER_WIRE );
    wire->SetEndPoint( VECTOR2I( 100000, 0 ) );
    screen->Append( wire );

    SCH_GROUP* group = new SCH_GROUP( screen );
    group->AddItem( wire );
    screen->Append( group );

    BOOST_REQUIRE( wire->GetParentGroup() == group );
    BOOST_REQUIRE( group->GetItems().count( wire ) == 1 );

    // The undo/cancel machinery holds a copy of the item that carries its data but no group
    // membership.  Give it different geometry so we can confirm the data swap actually happened.
    std::unique_ptr<SCH_LINE> image( static_cast<SCH_LINE*>( wire->Clone() ) );
    image->SetParentGroup( nullptr );
    image->SetEndPoint( VECTOR2I( 200000, 0 ) );

    wire->SwapItemData( image.get() );

    // The item data must have been swapped in.
    BOOST_CHECK( wire->GetEndPoint() == VECTOR2I( 200000, 0 ) );

    // But the wire must still belong to its group.  Swapping m_group here orphaned it from the
    // group while the group still listed it, which is the corruption this test guards against.
    BOOST_CHECK( wire->GetParentGroup() == group );
    BOOST_CHECK( group->GetItems().count( wire ) == 1 );
}


BOOST_AUTO_TEST_SUITE_END()
