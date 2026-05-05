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
#include <schematic.h>
#include <settings/settings_manager.h>
#include <locale_io.h>

#include <wx/filename.h>
#include <wx/xml/xml.h>


// Verifies that the KiCad-internal <net_chains> element is gated behind GNL_OPT_KICAD.
// External XML netlist consumers (KiCost, custom BOM scripts, schema validators) must
// not see this extension under the public netlist version "E".

struct XML_NETCHAIN_GATING_FIXTURE
{
    XML_NETCHAIN_GATING_FIXTURE() = default;

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


static bool writeAndLoad( SCHEMATIC* aSch, const wxString& aSuffix, unsigned aOptions,
                          wxXmlDocument& aDoc )
{
    wxFileName netFile = aSch->Project().GetProjectFullName();
    netFile.SetName( netFile.GetName() + aSuffix );
    netFile.SetExt( wxT( "xml" ) );

    if( wxFileExists( netFile.GetFullPath() ) )
        wxRemoveFile( netFile.GetFullPath() );

    WX_STRING_REPORTER                    reporter;
    std::unique_ptr<NETLIST_EXPORTER_XML> exporter =
            std::make_unique<NETLIST_EXPORTER_XML>( aSch );

    bool ok = exporter->WriteNetlist( netFile.GetFullPath(), aOptions, reporter )
              && reporter.GetMessages().IsEmpty();

    if( !ok )
        return false;

    bool loaded = aDoc.Load( netFile.GetFullPath() );
    wxRemoveFile( netFile.GetFullPath() );
    return loaded;
}


BOOST_FIXTURE_TEST_CASE( NetlistExporterXML_NetChainsAbsentForGenericFormat,
                         XML_NETCHAIN_GATING_FIXTURE )
{
    // Schematic with committed chains; generic-XML export must NOT include <net_chains>.
    LOCALE_IO dummy;
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
    BOOST_REQUIRE( graph->CreateNetChainFromPotential( candidate, wxT( "SIG_GATING" ) ) );

    // Generic XML (no GNL_OPT_KICAD) must not emit <net_chains>.
    {
        wxXmlDocument xdoc;
        BOOST_REQUIRE( writeAndLoad( m_schematic.get(), wxT( "_generic" ), 0, xdoc ) );
        BOOST_REQUIRE( xdoc.GetRoot() );
        BOOST_CHECK_MESSAGE( find_child( xdoc.GetRoot(), wxT( "net_chains" ) ) == nullptr,
                             "Generic XML export must not contain <net_chains>" );
    }

    // KiCad-internal export must emit <net_chains> with the committed chain.
    {
        wxXmlDocument xdoc;
        BOOST_REQUIRE( writeAndLoad( m_schematic.get(), wxT( "_kicad" ), GNL_OPT_KICAD, xdoc ) );
        BOOST_REQUIRE( xdoc.GetRoot() );

        wxXmlNode* netChains = find_child( xdoc.GetRoot(), wxT( "net_chains" ) );
        BOOST_REQUIRE_MESSAGE( netChains,
                               "KiCad-internal export must contain <net_chains>" );

        bool foundCommitted = false;

        for( wxXmlNode* xchain = netChains->GetChildren(); xchain; xchain = xchain->GetNext() )
        {
            if( xchain->GetName() != wxT( "net_chain" ) )
                continue;

            if( xchain->GetAttribute( wxT( "name" ), wxEmptyString ) == wxT( "SIG_GATING" ) )
            {
                foundCommitted = true;
                break;
            }
        }

        BOOST_CHECK_MESSAGE( foundCommitted,
                             "KiCad-internal export must list the committed chain" );
    }
}


BOOST_FIXTURE_TEST_CASE( NetlistExporterXML_NoNetChainsElementWhenEmpty,
                         XML_NETCHAIN_GATING_FIXTURE )
{
    // A schematic with no committed chains must not emit a stray <net_chains/> wrapper,
    // even under the KiCad-internal flag.
    LOCALE_IO dummy;
    KI_TEST::LoadSchematic( m_settingsManager, wxT( "net_chains_four_nets" ), m_schematic );

    SCH_SHEET_LIST    sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
    graph->Recalculate( sheets, /*aUnconditional=*/true );

    BOOST_REQUIRE_MESSAGE( graph->GetCommittedNetChains().empty(),
                           "fresh fixture must have no committed chains" );

    wxXmlDocument xdocGeneric;
    BOOST_REQUIRE( writeAndLoad( m_schematic.get(), wxT( "_empty_generic" ), 0, xdocGeneric ) );
    BOOST_REQUIRE( xdocGeneric.GetRoot() );
    BOOST_CHECK( find_child( xdocGeneric.GetRoot(), wxT( "net_chains" ) ) == nullptr );

    wxXmlDocument xdocKicad;
    BOOST_REQUIRE( writeAndLoad( m_schematic.get(), wxT( "_empty_kicad" ),
                                 GNL_OPT_KICAD, xdocKicad ) );
    BOOST_REQUIRE( xdocKicad.GetRoot() );
    BOOST_CHECK_MESSAGE( find_child( xdocKicad.GetRoot(), wxT( "net_chains" ) ) == nullptr,
                         "Empty chain set must not produce a stray <net_chains/> element" );
}
