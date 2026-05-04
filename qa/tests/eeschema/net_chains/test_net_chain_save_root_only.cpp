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
 */

#include <boost/test/unit_test.hpp>

#include <connection_graph.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <sch_netchain.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <locale_io.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <wx/ffile.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>


// Test backdoor to push a fully-formed committed net chain into the graph without
// needing a real RebuildNetChains() pass.  Friend-declared in connection_graph.h.
void boost_test_inject_committed_net_chain( CONNECTION_GRAPH& aGraph,
                                            std::unique_ptr<SCH_NETCHAIN> aChain )
{
    aGraph.m_committedNetChains.push_back( std::move( aChain ) );
}


struct NETCHAIN_SAVE_ROOT_FIXTURE
{
    NETCHAIN_SAVE_ROOT_FIXTURE() :
            m_settingsManager()
    {
        wxString tempDir = wxStandardPaths::Get().GetTempDir();
        wxString projectPath =
                tempDir + wxFileName::GetPathSeparator() + wxT( "test_netchain_save.kicad_pro" );
        m_tempFiles.push_back( projectPath );

        m_settingsManager.LoadProject( projectPath.ToStdString() );
        m_schematic = std::make_unique<SCHEMATIC>( nullptr );
        m_project = &m_settingsManager.Prj();
        m_schematic->SetProject( m_project );
    }

    ~NETCHAIN_SAVE_ROOT_FIXTURE()
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

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
    PROJECT*                   m_project;
    std::vector<wxString>      m_tempFiles;
};


/**
 * Regression: net_chain emission is schematic-level state owned by the connection graph.
 * It must be written exactly once, by the first top-level sheet's file.  Prior to the fix,
 * every per-sheet Format() call iterated GetCommittedNetChains() unconditionally, so a
 * multi-sheet save duplicated the (net_chain ...) block into every sub-sheet file and
 * broke round-trip stability.
 */
BOOST_FIXTURE_TEST_CASE( NetChainSavedOnlyOnRootSheetFile, NETCHAIN_SAVE_ROOT_FIXTURE )
{
    LOCALE_IO dummy;

    m_schematic->CreateDefaultScreens();

    std::vector<SCH_SHEET*> topSheets = m_schematic->GetTopLevelSheets();
    BOOST_REQUIRE( !topSheets.empty() );

    SCH_SHEET*  topSheet  = topSheets[0];
    SCH_SCREEN* topScreen = topSheet->GetScreen();
    BOOST_REQUIRE( topScreen );

    SCH_SHEET*  subSheet  = new SCH_SHEET( m_schematic.get() );
    SCH_SCREEN* subScreen = new SCH_SCREEN( m_schematic.get() );
    subSheet->SetName( "SubSheet" );
    subSheet->SetScreen( subScreen );
    subSheet->SetFileName( "subsheet.kicad_sch" );
    subScreen->SetFileName( "subsheet.kicad_sch" );
    topScreen->Append( subSheet );

    m_schematic->RefreshHierarchy();

    auto chain = std::make_unique<SCH_NETCHAIN>();
    chain->SetName( wxT( "TEST_CHAIN" ) );
    chain->AddNet( wxT( "/NET_A" ) );
    chain->AddNet( wxT( "/NET_B" ) );
    chain->SetTerminalRefs( wxT( "U1" ), wxT( "1" ), wxT( "U2" ), wxT( "2" ) );

    boost_test_inject_committed_net_chain( *m_schematic->ConnectionGraph(), std::move( chain ) );

    BOOST_REQUIRE_EQUAL( m_schematic->ConnectionGraph()->GetCommittedNetChains().size(), 1u );

    wxString topFileName = GetTempFileName( "netchain_main" );
    topFileName += ".kicad_sch";
    m_tempFiles.push_back( topFileName );

    wxString subFileName = GetTempFileName( "netchain_sub" );
    subFileName += ".kicad_sch";
    m_tempFiles.push_back( subFileName );

    SCH_IO_KICAD_SEXPR io;
    BOOST_CHECK_NO_THROW(
            io.SaveSchematicFile( topFileName, topSheet, m_schematic.get() ) );
    BOOST_CHECK_NO_THROW(
            io.SaveSchematicFile( subFileName, subSheet, m_schematic.get() ) );

    BOOST_REQUIRE( wxFileExists( topFileName ) );
    BOOST_REQUIRE( wxFileExists( subFileName ) );

    auto countOccurrences = []( const wxString& aHaystack, const wxString& aNeedle )
    {
        size_t count = 0;
        size_t pos = 0;

        while( ( pos = aHaystack.find( aNeedle, pos ) ) != wxString::npos )
        {
            ++count;
            pos += aNeedle.length();
        }

        return count;
    };

    wxString topContents;
    {
        wxFFile readback( topFileName, "rb" );
        BOOST_REQUIRE( readback.IsOpened() && readback.ReadAll( &topContents ) );
    }

    wxString subContents;
    {
        wxFFile readback( subFileName, "rb" );
        BOOST_REQUIRE( readback.IsOpened() && readback.ReadAll( &subContents ) );
    }

    BOOST_CHECK_EQUAL( countOccurrences( topContents, wxT( "(net_chain" ) ), 1u );
    BOOST_CHECK_MESSAGE( topContents.Contains( wxT( "TEST_CHAIN" ) ),
                         "Top-level sheet file is missing the chain name" );

    BOOST_CHECK_EQUAL( countOccurrences( subContents, wxT( "(net_chain" ) ), 0u );
    BOOST_CHECK_MESSAGE( !subContents.Contains( wxT( "TEST_CHAIN" ) ),
                         "Sub-sheet file unexpectedly contains the chain name" );
}
