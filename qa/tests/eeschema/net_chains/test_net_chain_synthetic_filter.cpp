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

#include <boost/test/unit_test.hpp>

#include <connection_graph.h>
#include <advanced_config.h>
#include <scoped_set_reset.h>
#include <sch_symbol.h>
#include <schematic_utils/schematic_file_util.h>
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

        // Release the project lock while its file still exists
        m_settingsManager.UnloadProject( m_project, false );

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


// Keep native terminals and real nets while exercising transient member filtering.
BOOST_FIXTURE_TEST_CASE( NetChainSyntheticNamesAreFilteredFromOutputs,
                         NETCHAIN_SYNTHETIC_FILTER_FIXTURE )
{
    LOCALE_IO dummy;

    const wxString chainName = wxT( "TEST_SYNTH_FILTER_CHAIN" );
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );

    for( bool useEngine : { false, true } )
    {
        BOOST_TEST_CONTEXT( "engine=" << useEngine )
        {
            enabled = useEngine;
            KI_TEST::LoadSchematic( m_settingsManager, "net_chains_four_nets_labeled", m_schematic );
            m_project = &m_schematic->Project();
            auto& manager = m_schematic->NetChains();
            BOOST_REQUIRE( !manager.GetPotentialNetChains().empty() );
            BOOST_REQUIRE( !manager.GetPotentialNetChains().front()->GetSymbols().empty() );
            SCH_SYMBOL* driver = *manager.GetPotentialNetChains().front()->GetSymbols().begin();

            for( const SCH_SHEET_PATH& path : m_schematic->Hierarchy() )
            {
                for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
                    item->SetExcludedFromBoard( item != driver, &path );
            }

            m_schematic->RebuildConnectivity();
            BOOST_REQUIRE_EQUAL( manager.GetPotentialNetChains().size(), 1u );
            auto* potential = manager.GetPotentialNetChains().front().get();
            potential->AddNet( wxString( SCH_NETCHAIN::SYNTHETIC_NET_PREFIX ) + wxString( "filter-test" ) );
            std::vector<wxString> realNets;
            std::vector<wxString> syntheticNets;

            for( const wxString& net : potential->GetNets() )
            {
                if( net.StartsWith( SCH_NETCHAIN::SYNTHETIC_NET_PREFIX ) )
                    syntheticNets.push_back( net );
                else
                    realNets.push_back( net );
            }

            BOOST_REQUIRE_GE( realNets.size(), 2u );
            BOOST_REQUIRE( !syntheticNets.empty() );
            const wxString realNetA = realNets[0];
            const wxString realNetB = realNets[1];
            const wxString synthName = syntheticNets.front();
            SCH_NETCHAIN* committed = manager.CreateNetChainFromPotential( potential, chainName );
            BOOST_REQUIRE( committed );
            BOOST_REQUIRE_EQUAL( committed->GetNets().count( synthName ), 1u );
            SCH_SHEET* topSheet = m_schematic->GetTopLevelSheet();
            SCH_SCREEN* topScreen = topSheet->GetScreen();
            wxString rootFileName = PathInWorkDir( wxT( "synth_filter.kicad_sch" ) );
            topSheet->SetFileName( wxT( "synth_filter.kicad_sch" ) );
            topScreen->SetFileName( rootFileName );

            // 1. XML netlist exporter (KiCad-internal flag emits <net_chains>).
            wxFileName xmlFile( rootFileName );
            xmlFile.SetName( xmlFile.GetName() + wxT( "_netlist" ) );
            xmlFile.SetExt( wxT( "xml" ) );
            m_tempFiles.push_back( xmlFile.GetFullPath() );

            {
                struct FILTER_EXPORTER : NETLIST_EXPORTER_XML
                {
                    using NETLIST_EXPORTER_XML::NETLIST_EXPORTER_XML;
                    wxString transient;

                    bool writeNetlist( const wxString& aPath, unsigned aOptions, REPORTER& aReporter ) override
                    {
                        const auto& chains = m_schematic->NetChains().GetCommittedNetChains();

                        if( chains.size() != 1u )
                            return false;

                        // Export preparation rebuilds the chain before this serializer runs.
                        chains.front()->AddNet( transient );
                        return NETLIST_EXPORTER_XML::writeNetlist( aPath, aOptions, aReporter );
                    }
                };

                WX_STRING_REPORTER               reporter;
                std::unique_ptr<FILTER_EXPORTER> exporter = std::make_unique<FILTER_EXPORTER>( m_schematic.get(),
                                                                                               nullptr );
                exporter->transient = synthName;

                BOOST_REQUIRE( exporter->WriteNetlist( xmlFile.GetFullPath(), GNL_OPT_KICAD, reporter ) );
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

                BOOST_CHECK_MESSAGE( xmlText.Find( wxString( SCH_NETCHAIN::SYNTHETIC_NET_PREFIX ) ) == wxNOT_FOUND,
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
                BOOST_REQUIRE_NO_THROW( saver.SaveSchematicFile( rootFileName, topSheet, m_schematic.get() ) );
                BOOST_REQUIRE( wxFileExists( rootFileName ) );

                wxFFile rawSexpr( rootFileName, "rb" );
                BOOST_REQUIRE( rawSexpr.IsOpened() );

                wxString sexprText;
                rawSexpr.ReadAll( &sexprText );
                rawSexpr.Close();

                BOOST_CHECK_MESSAGE( sexprText.Find( wxString( SCH_NETCHAIN::SYNTHETIC_NET_PREFIX ) ) == wxNOT_FOUND,
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

            // Check the persisted member list independently of connectivity reconstruction.
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
                    BOOST_CHECK_MESSAGE( !n.StartsWith( SCH_NETCHAIN::SYNTHETIC_NET_PREFIX ),
                                         "Reloaded chain leaked a synthetic __SG_* member" );
                }
            }
        }
    }
}
