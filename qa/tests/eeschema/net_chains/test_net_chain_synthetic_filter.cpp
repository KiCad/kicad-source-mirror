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
#include <netlist_exporter_xml.h>
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
#include <wx/xml/xml.h>


// Test backdoor declared in connection_graph.h.  Pushes a fully-formed committed chain
// into the graph so SaveSchematicFile / NETLIST_EXPORTER_XML have something to serialize.
void boost_test_inject_committed_net_chain( CONNECTION_GRAPH& aGraph,
                                            std::unique_ptr<SCH_NETCHAIN> aChain );


struct NETCHAIN_SYNTHETIC_FILTER_FIXTURE
{
    NETCHAIN_SYNTHETIC_FILTER_FIXTURE() :
            m_settingsManager()
    {
        m_workDir.AssignDir( wxStandardPaths::Get().GetTempDir() );
        m_workDir.AppendDir(
                wxString::Format( wxT( "kicad_qa_netchain_synth_%lu" ),
                                  static_cast<unsigned long>( wxGetProcessId() ) ) );

        wxFileName::Mkdir( m_workDir.GetFullPath(), 0755, wxPATH_MKDIR_FULL );

        wxString projectPath = m_workDir.GetFullPath() + wxT( "synth_filter.kicad_pro" );
        m_tempFiles.push_back( projectPath );

        m_settingsManager.LoadProject( projectPath.ToStdString() );
        m_project = &m_settingsManager.Prj();
    }

    ~NETCHAIN_SYNTHETIC_FILTER_FIXTURE()
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


static wxXmlNode* find_child( wxXmlNode* parent, const wxString& name )
{
    for( wxXmlNode* child = parent->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == name )
            return child;
    }

    return nullptr;
}


/**
 * Regression: synthetic per-run subgraph names (__SG_*) embed subgraph codes that are
 * not stable across reloads.  The sexpr writer filters them out at sch_io_kicad_sexpr.cpp,
 * but the XML netlist exporter previously emitted every member of the chain verbatim,
 * leaking unresolvable names into third-party netlist consumers (KiCost, custom BOM
 * pipelines).  Reloading the netlist could not match these synthetic strings against any
 * real BOARD net, so the chain assignment was silently dropped.
 *
 * This test injects a committed chain that mixes real net names with a __SG_* member,
 * runs both the XML exporter (KiCad-internal flag) and the sexpr writer over a single
 * fixture, and asserts that neither output contains the synthetic substring while the
 * real members survive.
 */
BOOST_FIXTURE_TEST_CASE( NetChainSyntheticNamesAreFilteredFromOutputs,
                         NETCHAIN_SYNTHETIC_FILTER_FIXTURE )
{
    LOCALE_IO dummy;

    const wxString chainName = wxT( "TEST_SYNTH_FILTER_CHAIN" );
    const wxString realNetA  = wxT( "/SIG_A" );
    const wxString realNetB  = wxT( "/SIG_B" );
    const wxString synthName = wxString( SCH_NETCHAIN::SYNTHETIC_NET_PREFIX )
                               + wxT( "0xdeadbeef" );

    m_schematic = std::make_unique<SCHEMATIC>( nullptr );
    m_schematic->SetProject( m_project );
    m_schematic->CreateDefaultScreens();

    std::vector<SCH_SHEET*> topSheets = m_schematic->GetTopLevelSheets();
    BOOST_REQUIRE( !topSheets.empty() );

    SCH_SHEET*  topSheet  = topSheets[0];
    SCH_SCREEN* topScreen = topSheet->GetScreen();
    BOOST_REQUIRE( topScreen );

    wxString rootFileName = PathInWorkDir( wxT( "synth_filter.kicad_sch" ) );
    topSheet->SetFileName( wxT( "synth_filter.kicad_sch" ) );
    topScreen->SetFileName( rootFileName );

    m_schematic->RefreshHierarchy();

    auto chain = std::make_unique<SCH_NETCHAIN>();
    chain->SetName( chainName );
    chain->AddNet( realNetA );
    chain->AddNet( realNetB );
    chain->AddNet( synthName );

    // Terminal refs are required by the sexpr writer; without them the chain is skipped
    // before the synthetic-net filter loop runs and this test would not exercise the filter.
    chain->SetTerminalRefs( wxT( "U1" ), wxT( "1" ), wxT( "U2" ), wxT( "2" ) );

    boost_test_inject_committed_net_chain( *m_schematic->ConnectionGraph(), std::move( chain ) );

    BOOST_REQUIRE_EQUAL( m_schematic->ConnectionGraph()->GetCommittedNetChains().size(), 1u );

    // 1. XML netlist exporter (KiCad-internal flag emits <net_chains>).
    wxFileName xmlFile( rootFileName );
    xmlFile.SetName( xmlFile.GetName() + wxT( "_netlist" ) );
    xmlFile.SetExt( wxT( "xml" ) );
    m_tempFiles.push_back( xmlFile.GetFullPath() );

    {
        WX_STRING_REPORTER                    reporter;
        std::unique_ptr<NETLIST_EXPORTER_XML> exporter =
                std::make_unique<NETLIST_EXPORTER_XML>( m_schematic.get() );

        BOOST_REQUIRE( exporter->WriteNetlist( xmlFile.GetFullPath(), GNL_OPT_KICAD,
                                               reporter ) );
        BOOST_REQUIRE( reporter.GetMessages().IsEmpty() );
    }

    BOOST_REQUIRE( wxFileExists( xmlFile.GetFullPath() ) );

    // Raw text scan catches the synthetic prefix anywhere in the document.
    {
        wxFFile rawXml( xmlFile.GetFullPath(), "rb" );
        BOOST_REQUIRE( rawXml.IsOpened() );

        wxString xmlText;
        rawXml.ReadAll( &xmlText );
        rawXml.Close();

        BOOST_CHECK_MESSAGE(
                xmlText.Find( wxString( SCH_NETCHAIN::SYNTHETIC_NET_PREFIX ) ) == wxNOT_FOUND,
                "XML netlist must not contain synthetic __SG_* net names" );
    }

    // Structural check: real nets remain in the chain's <members>; synthetic name does not.
    {
        wxXmlDocument xdoc;
        BOOST_REQUIRE( xdoc.Load( xmlFile.GetFullPath() ) );
        BOOST_REQUIRE( xdoc.GetRoot() );

        wxXmlNode* netChains = find_child( xdoc.GetRoot(), wxT( "net_chains" ) );
        BOOST_REQUIRE( netChains );

        wxXmlNode* targetChain = nullptr;

        for( wxXmlNode* xchain = netChains->GetChildren(); xchain; xchain = xchain->GetNext() )
        {
            if( xchain->GetName() != wxT( "net_chain" ) )
                continue;

            if( xchain->GetAttribute( wxT( "name" ), wxEmptyString ) == chainName )
            {
                targetChain = xchain;
                break;
            }
        }

        BOOST_REQUIRE_MESSAGE( targetChain, "Committed chain missing from XML output" );

        wxXmlNode* members = find_child( targetChain, wxT( "members" ) );
        BOOST_REQUIRE( members );

        std::set<wxString> emittedNets;

        for( wxXmlNode* xmem = members->GetChildren(); xmem; xmem = xmem->GetNext() )
        {
            if( xmem->GetName() != wxT( "member" ) )
                continue;

            emittedNets.insert( xmem->GetAttribute( wxT( "net" ), wxEmptyString ) );
        }

        BOOST_CHECK( emittedNets.count( realNetA ) == 1u );
        BOOST_CHECK( emittedNets.count( realNetB ) == 1u );
        BOOST_CHECK_MESSAGE( emittedNets.count( synthName ) == 0u,
                             "Synthetic net leaked into XML <member> list" );
    }

    // 2. sexpr writer must already filter synthetic names; reloading the file must
    //    yield a chain that resolves with the real members intact.
    {
        SCH_IO_KICAD_SEXPR saver;
        BOOST_REQUIRE_NO_THROW( saver.SaveSchematicFile( rootFileName, topSheet,
                                                        m_schematic.get() ) );
        BOOST_REQUIRE( wxFileExists( rootFileName ) );

        wxFFile rawSexpr( rootFileName, "rb" );
        BOOST_REQUIRE( rawSexpr.IsOpened() );

        wxString sexprText;
        rawSexpr.ReadAll( &sexprText );
        rawSexpr.Close();

        BOOST_CHECK_MESSAGE(
                sexprText.Find( wxString( SCH_NETCHAIN::SYNTHETIC_NET_PREFIX ) ) == wxNOT_FOUND,
                "kicad_sch must not contain synthetic __SG_* net names" );

        // Guard against the early-skip path in sch_io_kicad_sexpr.cpp: if terminal refs were
        // missing the writer would emit no net_chain section and the synthetic-prefix check
        // above would pass vacuously.
        BOOST_CHECK_MESSAGE( sexprText.Find( wxT( "(net_chain" ) ) != wxNOT_FOUND,
                             "kicad_sch must contain the committed net_chain section" );
        BOOST_CHECK_MESSAGE( sexprText.Find( chainName ) != wxNOT_FOUND,
                             "kicad_sch must reference the committed chain by name" );
        BOOST_CHECK_MESSAGE( sexprText.Find( realNetA ) != wxNOT_FOUND,
                             "kicad_sch must retain real net A in the chain's nets list" );
        BOOST_CHECK_MESSAGE( sexprText.Find( realNetB ) != wxNOT_FOUND,
                             "kicad_sch must retain real net B in the chain's nets list" );
    }

    // 3. Reload the saved file.  RebuildNetChains needs real schematic items to repopulate
    //    GetCommittedNetChains(), so instead we verify the parser-side member-net overrides
    //    that the IO layer hands to the connection graph during load.
    {
        SCH_IO_KICAD_SEXPR loader;
        SCHEMATIC          reloaded( nullptr );

        reloaded.SetProject( m_project );

        SCH_SHEET* loadedRoot = nullptr;
        BOOST_REQUIRE_NO_THROW(
                loadedRoot = loader.LoadSchematicFile( rootFileName, &reloaded ) );
        BOOST_REQUIRE( loadedRoot );

        const auto& overrides = reloaded.ConnectionGraph()->GetNetChainMemberNetOverrides();
        auto        it = overrides.find( chainName );
        BOOST_REQUIRE_MESSAGE( it != overrides.end(),
                               "Reloaded schematic missing chain member-net override" );

        const std::set<wxString>& reloadedNets = it->second;
        BOOST_CHECK( !reloadedNets.empty() );
        BOOST_CHECK( reloadedNets.count( realNetA ) == 1u );
        BOOST_CHECK( reloadedNets.count( realNetB ) == 1u );

        for( const wxString& n : reloadedNets )
        {
            BOOST_CHECK_MESSAGE(
                    !n.StartsWith( SCH_NETCHAIN::SYNTHETIC_NET_PREFIX ),
                    "Reloaded chain leaked a synthetic __SG_* member" );
        }
    }
}
