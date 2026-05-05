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


// Test backdoor declared in connection_graph.h.  Pushes a fully-formed committed chain
// into the graph so SaveSchematicFile has something to serialize.
void boost_test_inject_committed_net_chain( CONNECTION_GRAPH& aGraph,
                                            std::unique_ptr<SCH_NETCHAIN> aChain );


struct NETCHAIN_HIER_ROUNDTRIP_FIXTURE
{
    NETCHAIN_HIER_ROUNDTRIP_FIXTURE() :
            m_settingsManager()
    {
        m_workDir.AssignDir( wxStandardPaths::Get().GetTempDir() );
        m_workDir.AppendDir(
                wxString::Format( wxT( "kicad_qa_netchain_hier_%lu" ),
                                  static_cast<unsigned long>( wxGetProcessId() ) ) );

        wxFileName::Mkdir( m_workDir.GetFullPath(), 0755, wxPATH_MKDIR_FULL );

        wxString projectPath = m_workDir.GetFullPath() + wxT( "hier_roundtrip.kicad_pro" );
        m_tempFiles.push_back( projectPath );

        m_settingsManager.LoadProject( projectPath.ToStdString() );
        m_project = &m_settingsManager.Prj();
    }

    ~NETCHAIN_HIER_ROUNDTRIP_FIXTURE()
    {
        m_schematic.reset();

        for( const wxString& file : m_tempFiles )
        {
            if( wxFileExists( file ) )
                wxRemoveFile( file );
        }

        if( m_workDir.DirExists() )
            wxFileName::Rmdir( m_workDir.GetFullPath(), wxPATH_RMDIR_RECURSIVE );
    }

    wxString PathInWorkDir( const wxString& aLeaf )
    {
        wxString full = m_workDir.GetFullPath() + aLeaf;
        m_tempFiles.push_back( full );
        return full;
    }

    SETTINGS_MANAGER           m_settingsManager;
    PROJECT*                   m_project = nullptr;
    std::unique_ptr<SCHEMATIC> m_schematic;
    wxFileName                 m_workDir;
    std::vector<wxString>      m_tempFiles;
};


/**
 * Regression: net-chain override maps (class, color, terminal refs) are schematic-level
 * state owned by the connection graph.  They are written into the root sheet's file by
 * SaveSchematicFile, but loadFile/LoadContent run for every sheet in the hierarchy.
 * Without gating the setter calls on aSheet == m_rootSheet, every sub-sheet load
 * silently overwrote the override maps with the parser's empty defaults, wiping all
 * chain metadata as soon as a hierarchical schematic was opened.
 *
 * This test builds a two-sheet hierarchy with a committed chain that carries terminal
 * refs, a netclass override and a color override.  It saves the hierarchy through the
 * full IO plugin, reloads it through LoadSchematicFile (which exercises loadHierarchy
 * and therefore loadFile for both the root and the sub-sheet), and verifies the three
 * override maps survived the load.
 */
BOOST_FIXTURE_TEST_CASE( NetChainHierarchicalRoundTripPreservesOverrides,
                         NETCHAIN_HIER_ROUNDTRIP_FIXTURE )
{
    LOCALE_IO dummy;

    const wxString chainName = wxT( "TEST_HIER_CHAIN" );
    const wxString chainNetClass = wxT( "HighSpeed" );
    const KIGFX::COLOR4D chainColor( 0.5, 0.25, 0.75, 1.0 );

    m_schematic = std::make_unique<SCHEMATIC>( nullptr );
    m_schematic->SetProject( m_project );
    m_schematic->CreateDefaultScreens();

    std::vector<SCH_SHEET*> topSheets = m_schematic->GetTopLevelSheets();
    BOOST_REQUIRE( !topSheets.empty() );

    SCH_SHEET*  topSheet  = topSheets[0];
    SCH_SCREEN* topScreen = topSheet->GetScreen();
    BOOST_REQUIRE( topScreen );

    wxString rootFileName = PathInWorkDir( wxT( "hier_roundtrip.kicad_sch" ) );
    wxString subFileName  = PathInWorkDir( wxT( "hier_roundtrip_sub.kicad_sch" ) );

    topSheet->SetFileName( wxT( "hier_roundtrip.kicad_sch" ) );
    topScreen->SetFileName( rootFileName );

    SCH_SHEET*  subSheet  = new SCH_SHEET( m_schematic.get() );
    SCH_SCREEN* subScreen = new SCH_SCREEN( m_schematic.get() );
    subSheet->SetName( wxT( "SubSheet" ) );
    subSheet->SetFileName( wxT( "hier_roundtrip_sub.kicad_sch" ) );
    subSheet->SetScreen( subScreen );
    subScreen->SetFileName( subFileName );
    topScreen->Append( subSheet );

    m_schematic->RefreshHierarchy();

    auto chain = std::make_unique<SCH_NETCHAIN>();
    chain->SetName( chainName );
    chain->AddNet( wxT( "/NET_A" ) );
    chain->AddNet( wxT( "/NET_B" ) );
    chain->SetTerminalRefs( wxT( "U1" ), wxT( "1" ), wxT( "U2" ), wxT( "2" ) );
    chain->SetNetClass( chainNetClass );
    chain->SetColor( chainColor );

    boost_test_inject_committed_net_chain( *m_schematic->ConnectionGraph(), std::move( chain ) );

    BOOST_REQUIRE_EQUAL( m_schematic->ConnectionGraph()->GetCommittedNetChains().size(), 1u );

    SCH_IO_KICAD_SEXPR saver;
    BOOST_REQUIRE_NO_THROW( saver.SaveSchematicFile( rootFileName, topSheet, m_schematic.get() ) );
    BOOST_REQUIRE_NO_THROW( saver.SaveSchematicFile( subFileName, subSheet, m_schematic.get() ) );

    BOOST_REQUIRE( wxFileExists( rootFileName ) );
    BOOST_REQUIRE( wxFileExists( subFileName ) );

    // Drop the in-memory schematic and reload from disk through the full IO plugin so
    // that loadHierarchy walks both files via loadFile, exercising the gate.
    m_schematic.reset();

    auto reloaded = std::make_unique<SCHEMATIC>( nullptr );
    reloaded->SetProject( m_project );

    SCH_IO_KICAD_SEXPR loader;
    SCH_SHEET*         loadedRoot = nullptr;

    BOOST_REQUIRE_NO_THROW( loadedRoot = loader.LoadSchematicFile( rootFileName, reloaded.get() ) );
    BOOST_REQUIRE( loadedRoot );

    reloaded->SetTopLevelSheets( { loadedRoot } );
    reloaded->RefreshHierarchy();

    const auto& classOverrides    = reloaded->ConnectionGraph()->GetNetChainNetClassOverrides();
    const auto& colorOverrides    = reloaded->ConnectionGraph()->GetNetChainColorOverrides();
    const auto& terminalOverrides = reloaded->ConnectionGraph()->GetNetChainTerminalRefOverrides();

    BOOST_CHECK_MESSAGE(
            classOverrides.size() == 1u,
            "Net-chain netclass overrides were wiped during hierarchical load (size = "
                    << classOverrides.size() << ")" );

    BOOST_CHECK_MESSAGE(
            colorOverrides.size() == 1u,
            "Net-chain color overrides were wiped during hierarchical load (size = "
                    << colorOverrides.size() << ")" );

    BOOST_CHECK_MESSAGE(
            terminalOverrides.size() == 1u,
            "Net-chain terminal-ref overrides were wiped during hierarchical load (size = "
                    << terminalOverrides.size() << ")" );

    auto classIt = classOverrides.find( chainName );
    BOOST_REQUIRE( classIt != classOverrides.end() );
    BOOST_CHECK_EQUAL( classIt->second, chainNetClass );

    auto colorIt = colorOverrides.find( chainName );
    BOOST_REQUIRE( colorIt != colorOverrides.end() );

    // RGB channels are serialized as 0..255 integers and reconstructed by dividing by 255,
    // so an exact equality check is not appropriate.  Allow slightly more than 0.5/255 of
    // slack to absorb rounding in either direction.
    constexpr double colorTolerance = 1.0 / 255.0 + 1e-9;
    BOOST_CHECK_SMALL( colorIt->second.r - chainColor.r, colorTolerance );
    BOOST_CHECK_SMALL( colorIt->second.g - chainColor.g, colorTolerance );
    BOOST_CHECK_SMALL( colorIt->second.b - chainColor.b, colorTolerance );
    BOOST_CHECK_SMALL( colorIt->second.a - chainColor.a, 1e-3 );

    auto termIt = terminalOverrides.find( chainName );
    BOOST_REQUIRE( termIt != terminalOverrides.end() );
    BOOST_CHECK_EQUAL( termIt->second.first.ref, wxT( "U1" ) );
    BOOST_CHECK_EQUAL( termIt->second.first.pin, wxT( "1" ) );
    BOOST_CHECK_EQUAL( termIt->second.second.ref, wxT( "U2" ) );
    BOOST_CHECK_EQUAL( termIt->second.second.pin, wxT( "2" ) );

    reloaded.reset();
}
