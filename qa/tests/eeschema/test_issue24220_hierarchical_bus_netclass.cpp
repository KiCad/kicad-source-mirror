/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file test_issue24220_hierarchical_bus_netclass.cpp
 *
 * Tests for issue 24220: Inconsistency between schematic netclass coloring and
 * XML netlist export in hierarchical buses.
 *
 * When a netclass pattern matches a bus member net (e.g. pattern "*AD.1*" against
 * net "/AD.1"), the schematic editor uses NET_SETTINGS::GetEffectiveNetClass() and
 * sees the matching netclass.  The XML netlist exporter previously resolved the
 * netclass via subgraph->GetDriver()->GetEffectiveNetClass(), which fails for
 * bus-member subgraphs (no driver SCH_ITEM is set) and silently falls back to
 * the default netclass.  The exporter should produce the same netclass that the
 * painter shows.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <wx/filename.h>
#include <wx/xml/xml.h>

#include <netlist_exporter_xml.h>
#include <project/net_settings.h>
#include <project/project_file.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <string_utils.h>


struct ISSUE24220_FIXTURE
{
    ISSUE24220_FIXTURE() {}

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


BOOST_FIXTURE_TEST_SUITE( Issue24220HierarchicalBusNetclass, ISSUE24220_FIXTURE )


// The schematic ships with patterns "*AD.1*", "*AD.2*", "*AD*" all assigned to the
// "ADI" netclass.  Verify that the live NET_SETTINGS lookup (the same one the
// schematic painter uses) returns the expected ADI class for the bus member nets.
BOOST_AUTO_TEST_CASE( NetSettingsResolvesBusMemberToAdi )
{
    KI_TEST::LoadSchematic( m_settingsManager, wxT( "netlists/issue24220/issue24220" ),
                            m_schematic );

    std::shared_ptr<NET_SETTINGS>& netSettings =
            m_schematic->Project().GetProjectFile().m_NetSettings;

    BOOST_REQUIRE( netSettings );

    // The "ADI" netclass in the project only specifies some parameters, so
    // GetEffectiveNetClass composites it with the Default netclass to fill in
    // the rest.  The composite's name leads with "ADI" so users still see the
    // matching class in netlist/UI output.
    BOOST_CHECK( netSettings->GetEffectiveNetClass( wxT( "/AD.1" ) )
                         ->GetName()
                         .StartsWith( wxT( "ADI" ) ) );
    BOOST_CHECK( netSettings->GetEffectiveNetClass( wxT( "/AD.2" ) )
                         ->GetName()
                         .StartsWith( wxT( "ADI" ) ) );
}


// Regression for the reported bug:  the XML exporter must emit class="ADI" for
// the bus member nets, matching what the schematic editor displays.  Before the
// fix, the exporter emitted class="Default" because the bus-member subgraph's
// driver SCH_ITEM is null and the lookup was falling back via that path.
BOOST_AUTO_TEST_CASE( XmlExportClassMatchesNetSettings )
{
    KI_TEST::LoadSchematic( m_settingsManager, wxT( "netlists/issue24220/issue24220" ),
                            m_schematic );

    std::shared_ptr<NET_SETTINGS>& netSettings =
            m_schematic->Project().GetProjectFile().m_NetSettings;

    BOOST_REQUIRE( netSettings );

    wxFileName netFile = m_schematic->Project().GetProjectFullName();
    netFile.SetName( netFile.GetName() + wxT( "_issue24220_test" ) );
    netFile.SetExt( wxT( "xml" ) );

    if( wxFileExists( netFile.GetFullPath() ) )
        wxRemoveFile( netFile.GetFullPath() );

    WX_STRING_REPORTER                    reporter;
    std::unique_ptr<NETLIST_EXPORTER_XML> exporter =
            std::make_unique<NETLIST_EXPORTER_XML>( m_schematic.get() );

    bool success = exporter->WriteNetlist( netFile.GetFullPath(), 0, reporter );
    BOOST_REQUIRE( success );

    wxXmlDocument xdoc;
    BOOST_REQUIRE( xdoc.Load( netFile.GetFullPath() ) );

    wxXmlNode* root = xdoc.GetRoot();
    BOOST_REQUIRE( root );

    wxXmlNode* nets = nullptr;

    for( wxXmlNode* child = root->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == wxT( "nets" ) )
        {
            nets = child;
            break;
        }
    }

    BOOST_REQUIRE( nets );

    bool matchedAd1 = false;
    bool matchedAd2 = false;

    for( wxXmlNode* net = nets->GetChildren(); net; net = net->GetNext() )
    {
        if( net->GetName() != wxT( "net" ) )
            continue;

        wxString netName  = net->GetAttribute( wxT( "name" ), wxEmptyString );
        wxString netClass = net->GetAttribute( wxT( "class" ), wxEmptyString );

        if( netName != wxT( "/AD.1" ) && netName != wxT( "/AD.2" ) )
            continue;

        std::shared_ptr<NETCLASS> expected =
                netSettings->GetEffectiveNetClass( netName );

        BOOST_REQUIRE( expected );
        BOOST_TEST_INFO( "net=" << netName );
        BOOST_CHECK_EQUAL( netClass, UnescapeString( expected->GetName() ) );
        // The painter shows the ADI color; the exporter should now agree.
        BOOST_CHECK_NE( netClass, wxString( wxT( "Default" ) ) );
        BOOST_CHECK( netClass.StartsWith( wxT( "ADI" ) ) );

        if( netName == wxT( "/AD.1" ) )
            matchedAd1 = true;
        else if( netName == wxT( "/AD.2" ) )
            matchedAd2 = true;
    }

    BOOST_CHECK( matchedAd1 );
    BOOST_CHECK( matchedAd2 );

    wxRemoveFile( netFile.GetFullPath() );
}


BOOST_AUTO_TEST_SUITE_END()
