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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file test_issue24870_top_level_sheet_delete.cpp
 *
 * Test for issue #24870: deleting a top-level sheet blanked the page that was left behind.
 *
 * The schematic editor draws whatever screen the current sheet path points at, so removing
 * the displayed top-level sheet has to leave that path on a surviving page.  These tests
 * cover the model half of the contract: the current sheet is retargeted onto a page that is
 * still in the hierarchy, the surviving page keeps every item it had, and the survivor still
 * round trips through disk.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <schematic.h>
#include <schematic_file_util.h>
#include <sch_line.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_text.h>
#include <settings/settings_manager.h>
#include <project.h>
#include <locale_io.h>

#include <wx/filename.h>

#include <fstream>


struct ISSUE24870_FIXTURE
{
    ISSUE24870_FIXTURE()
    {
        m_settingsManager.LoadProject( "" );
        m_schematic = std::make_unique<SCHEMATIC>( &m_settingsManager.Prj() );
        m_schematic->Reset();

        // Reset() leaves one empty top-level sheet behind; that is the reporter's first page
        m_page1 = m_schematic->GetTopLevelSheet( 0 );
        BOOST_REQUIRE( m_page1 );
        BOOST_REQUIRE( m_page1->GetScreen() );

        m_page1->GetScreen()->SetFileName( wxT( "page1.kicad_sch" ) );

        SCH_LINE* wire = new SCH_LINE( VECTOR2I( 0, 0 ), LAYER_WIRE );
        wire->SetEndPoint( VECTOR2I( 10000, 0 ) );
        m_page1->GetScreen()->Append( wire );

        SCH_TEXT* text = new SCH_TEXT( VECTOR2I( 0, 5000 ), wxT( "PAGE1 CONTENT" ) );
        m_page1->GetScreen()->Append( text );
    }

    /// Add a second top-level sheet the way HIERARCHY_PANE's "New Top-Level Sheet" does
    SCH_SHEET* addSecondPage()
    {
        SCH_SHEET*  page2 = new SCH_SHEET( m_schematic.get() );
        SCH_SCREEN* screen = new SCH_SCREEN( m_schematic.get() );

        page2->SetScreen( screen );
        page2->SetName( wxT( "Untitled" ) );
        screen->SetFileName( wxT( "untitled.kicad_sch" ) );

        m_schematic->AddTopLevelSheet( page2 );

        SCH_SHEET_PATH page2Path;
        page2Path.push_back( page2 );
        page2Path.SetPageNumber( wxT( "2" ) );

        return page2;
    }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
    SCH_SHEET*                 m_page1 = nullptr;
};


BOOST_FIXTURE_TEST_SUITE( Issue24870TopLevelSheetDelete, ISSUE24870_FIXTURE )


/**
 * Deleting the top-level sheet that is being displayed must retarget the current sheet onto a
 * page that is still in the hierarchy, otherwise the editor keeps drawing the removed screen.
 */
BOOST_AUTO_TEST_CASE( DeleteDisplayedPageRetargetsCurrentSheet )
{
    SCH_SHEET*  page2 = addSecondPage();
    SCH_SCREEN* page1Screen = m_page1->GetScreen();
    size_t      itemCount = page1Screen->Items().size();

    // The reporter deletes the second page while looking at it
    SCH_SHEET_PATH page2Path;
    page2Path.push_back( page2 );
    m_schematic->SetCurrentSheet( page2Path );

    BOOST_REQUIRE( m_schematic->RemoveTopLevelSheet( page2 ) );

    BOOST_REQUIRE( !m_schematic->CurrentSheet().empty() );
    BOOST_CHECK( m_schematic->IsTopLevelSheet( m_schematic->CurrentSheet().at( 0 ) ) );
    BOOST_CHECK( m_schematic->CurrentSheet().LastScreen() == page1Screen );

    // The path the editor is about to draw has to be one the hierarchy still knows about
    BOOST_CHECK( m_schematic->Hierarchy().HasPath( m_schematic->CurrentSheet().Path() ) );

    BOOST_CHECK_EQUAL( page1Screen->Items().size(), itemCount );
}


/**
 * The surviving page keeps its content, both in memory and through a save/reload round trip.
 */
BOOST_AUTO_TEST_CASE( DeletePageKeepsOtherPageContentOnDisk )
{
    LOCALE_IO   dummy;
    SCH_SHEET*  page2 = addSecondPage();
    SCH_SCREEN* page1Screen = m_page1->GetScreen();

    SCH_SHEET_PATH page2Path;
    page2Path.push_back( page2 );
    m_schematic->SetCurrentSheet( page2Path );

    BOOST_REQUIRE( m_schematic->RemoveTopLevelSheet( page2 ) );

    BOOST_CHECK_EQUAL( page1Screen->Items().size(), size_t( 2 ) );

    wxString    tmpName = wxFileName::CreateTempFileName( wxT( "issue24870" ) );
    std::string path = ( tmpName + wxT( ".kicad_sch" ) ).ToStdString();

    KI_TEST::DumpSchematicToFile( *m_schematic, *m_page1, path );

    std::ifstream stream( path );
    BOOST_REQUIRE( stream.is_open() );

    std::unique_ptr<SCHEMATIC> reloaded =
            KI_TEST::ReadSchematicFromStream( stream, &m_settingsManager.Prj() );

    BOOST_REQUIRE( reloaded );
    BOOST_REQUIRE( reloaded->RootScreen() );

    int wires = 0;
    int texts = 0;

    for( SCH_ITEM* item : reloaded->RootScreen()->Items() )
    {
        if( item->Type() == SCH_LINE_T && static_cast<SCH_LINE*>( item )->IsWire() )
            wires++;
        else if( item->Type() == SCH_TEXT_T )
            texts++;
    }

    BOOST_CHECK_EQUAL( wires, 1 );
    BOOST_CHECK_EQUAL( texts, 1 );

    stream.close();
    wxRemoveFile( tmpName );
    wxRemoveFile( path );
}


BOOST_AUTO_TEST_SUITE_END()
