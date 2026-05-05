/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers
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
 * along with this program; if not, you may find one at
 * http://www.gnu.org/licenses/
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <connection_graph.h>
#include <netlist_exporter_xml.h>
#include <sch_netchain.h>
#include <sch_sheet.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <locale_io.h>

#include <wx/filename.h>
#include <wx/xml/xml.h>


struct XML_NETCHAIN_TERMINALS_FIXTURE
{
    XML_NETCHAIN_TERMINALS_FIXTURE() = default;

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
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


BOOST_FIXTURE_TEST_CASE( NetlistExporterXML_NetChainTerminalPinsRoundTrip,
                         XML_NETCHAIN_TERMINALS_FIXTURE )
{
    LOCALE_IO         dummy;
    KI_TEST::LoadSchematic( m_settingsManager, wxT( "net_chains_four_nets" ), m_schematic );

    SCH_SHEET_LIST    sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
    graph->Recalculate( sheets, /*aUnconditional=*/true );

    SCH_NETCHAIN* candidate = nullptr;

    for( const auto& sig : graph->GetPotentialNetChains() )
    {
        if( sig && sig->GetNets().size() == 4 )
        {
            candidate = sig.get();
            break;
        }
    }

    BOOST_REQUIRE_MESSAGE( candidate, "fixture must produce a 4-net potential chain" );
    BOOST_REQUIRE_MESSAGE( !candidate->GetTerminalRef( 0 ).IsEmpty()
                                   && !candidate->GetTerminalRef( 1 ).IsEmpty(),
                           "potential chain must have terminal refs populated" );

    SCH_NETCHAIN* committed = graph->CreateNetChainFromPotential( candidate, wxT( "SIG_TEST" ) );
    BOOST_REQUIRE( committed );

    const wxString expectRefA = committed->GetTerminalRef( 0 );
    const wxString expectPinA = committed->GetTerminalPinNum( 0 );
    const wxString expectRefB = committed->GetTerminalRef( 1 );
    const wxString expectPinB = committed->GetTerminalPinNum( 1 );

    wxFileName netFile = m_schematic->Project().GetProjectFullName();
    netFile.SetName( netFile.GetName() + wxT( "_terminal_pins_test" ) );
    netFile.SetExt( wxT( "xml" ) );

    if( wxFileExists( netFile.GetFullPath() ) )
        wxRemoveFile( netFile.GetFullPath() );

    WX_STRING_REPORTER                    reporter;
    std::unique_ptr<NETLIST_EXPORTER_XML> exporter =
            std::make_unique<NETLIST_EXPORTER_XML>( m_schematic.get() );

    // Net chains are gated behind the KiCad-internal flag, so the public XML format
    // does not leak them to schema-validating consumers.
    BOOST_REQUIRE( exporter->WriteNetlist( netFile.GetFullPath(), GNL_OPT_KICAD, reporter )
                   && reporter.GetMessages().IsEmpty() );

    wxXmlDocument xdoc;
    BOOST_REQUIRE( xdoc.Load( netFile.GetFullPath() ) );

    wxXmlNode* root = xdoc.GetRoot();
    BOOST_REQUIRE( root );

    wxXmlNode* netChains = find_child( root, wxT( "net_chains" ) );
    BOOST_REQUIRE( netChains );

    wxXmlNode* targetChain = nullptr;

    for( wxXmlNode* xchain = netChains->GetChildren(); xchain; xchain = xchain->GetNext() )
    {
        if( xchain->GetName() != wxT( "net_chain" ) )
            continue;

        if( xchain->GetAttribute( wxT( "name" ), wxEmptyString ) == wxT( "SIG_TEST" ) )
        {
            targetChain = xchain;
            break;
        }
    }

    BOOST_REQUIRE_MESSAGE( targetChain, "expected committed chain SIG_TEST in netlist" );

    wxXmlNode* xterminals = find_child( targetChain, wxT( "terminal_pins" ) );
    BOOST_REQUIRE_MESSAGE( xterminals, "expected terminal_pins block under net_chain" );

    std::vector<std::pair<wxString, wxString>> emittedTerms;

    for( wxXmlNode* xt = xterminals->GetChildren(); xt; xt = xt->GetNext() )
    {
        if( xt->GetName() != wxT( "terminal_pin" ) )
            continue;

        emittedTerms.emplace_back( xt->GetAttribute( wxT( "ref" ), wxEmptyString ),
                                   xt->GetAttribute( wxT( "pin" ), wxEmptyString ) );
    }

    BOOST_CHECK_EQUAL( emittedTerms.size(), 2u );

    auto matches = []( const std::vector<std::pair<wxString, wxString>>& aTerms,
                       const wxString& aRef, const wxString& aPin )
    {
        for( const auto& t : aTerms )
        {
            if( t.first == aRef && t.second == aPin )
                return true;
        }

        return false;
    };

    BOOST_CHECK( matches( emittedTerms, expectRefA, expectPinA ) );
    BOOST_CHECK( matches( emittedTerms, expectRefB, expectPinB ) );

    wxRemoveFile( netFile.GetFullPath() );
}
