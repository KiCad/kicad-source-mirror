/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

/*
 * Regression test for https://gitlab.com/kicad/code/kicad/-/work_items/24766
 *
 * The XML netlist must list a nested child group's uuid among its parent group's members.
 *
 * Fixture nested_groups.kicad_sch has group OUTER containing symbol R3 and group INNER.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <netlist_exporter_xml.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <locale_io.h>

#include <set>

#include <wx/filename.h>
#include <wx/xml/xml.h>


struct XML_NESTED_GROUPS_FIXTURE
{
    XML_NESTED_GROUPS_FIXTURE() = default;

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


static wxXmlNode* find_child( wxXmlNode* aParent, const wxString& aName )
{
    for( wxXmlNode* child = aParent->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == aName )
            return child;
    }

    return nullptr;
}


BOOST_FIXTURE_TEST_CASE( NetlistExporterXML_NestedGroupMembershipPreserved, XML_NESTED_GROUPS_FIXTURE )
{
    LOCALE_IO dummy;
    KI_TEST::LoadSchematic( m_settingsManager, wxT( "nested_groups" ), m_schematic );

    wxFileName netFile = m_schematic->Project().GetProjectFullName();
    netFile.SetName( netFile.GetName() + wxT( "_nested_groups_test" ) );
    netFile.SetExt( wxT( "xml" ) );

    if( wxFileExists( netFile.GetFullPath() ) )
        wxRemoveFile( netFile.GetFullPath() );

    WX_STRING_REPORTER                    reporter;
    std::unique_ptr<NETLIST_EXPORTER_XML> exporter = std::make_unique<NETLIST_EXPORTER_XML>( m_schematic.get() );

    // Groups are only emitted for the KiCad-internal netlist.
    BOOST_REQUIRE( exporter->WriteNetlist( netFile.GetFullPath(), GNL_OPT_KICAD, reporter ) );

    wxXmlDocument xdoc;
    BOOST_REQUIRE( xdoc.Load( netFile.GetFullPath() ) );

    wxXmlNode* root = xdoc.GetRoot();
    BOOST_REQUIRE( root );

    wxXmlNode* xgroups = find_child( root, wxT( "groups" ) );
    BOOST_REQUIRE_MESSAGE( xgroups, "expected a <groups> block in the netlist" );

    std::set<wxString>              groupUuids;
    std::vector<std::set<wxString>> groupMembers;

    for( wxXmlNode* xgroup = xgroups->GetChildren(); xgroup; xgroup = xgroup->GetNext() )
    {
        if( xgroup->GetName() != wxT( "group" ) )
            continue;

        groupUuids.insert( xgroup->GetAttribute( wxT( "uuid" ), wxEmptyString ) );

        std::set<wxString> members;

        if( wxXmlNode* xmembers = find_child( xgroup, wxT( "members" ) ) )
        {
            for( wxXmlNode* xm = xmembers->GetChildren(); xm; xm = xm->GetNext() )
            {
                if( xm->GetName() == wxT( "member" ) )
                    members.insert( xm->GetAttribute( wxT( "uuid" ), wxEmptyString ) );
            }
        }

        groupMembers.push_back( std::move( members ) );
    }

    BOOST_REQUIRE_MESSAGE( groupUuids.size() >= 2, "fixture must contain a parent and a nested child group" );

    bool nestedReferenceFound = false;

    for( const std::set<wxString>& members : groupMembers )
    {
        for( const wxString& member : members )
        {
            if( groupUuids.count( member ) )
            {
                nestedReferenceFound = true;
                break;
            }
        }

        if( nestedReferenceFound )
            break;
    }

    BOOST_CHECK_MESSAGE( nestedReferenceFound, "no group lists another group's UUID as a member; nested group "
                                               "membership was dropped during XML export" );

    wxRemoveFile( netFile.GetFullPath() );
}
