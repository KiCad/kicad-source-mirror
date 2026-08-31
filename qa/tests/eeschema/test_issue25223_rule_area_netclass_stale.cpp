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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file
 * Regression coverage for https://gitlab.com/kicad/code/kicad/-/issues/25223, using the
 * reporter's project. TP1 left a rule area assigning "WideClearance", yet
 * netclass_assignments still names it alongside TP2.
 */

#include <boost/test/unit_test.hpp>
#include <qa_utils/wx_utils/unit_test_utils.h>
#include <eeschema_test_utils.h>

#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>

#include <fstream>
#include <string>

#include <nlohmann/json.hpp>

#include <project.h>
#include <project/net_settings.h>
#include <project/project_file.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <wildcards_and_files_ext.h>


namespace
{

const std::string TP1_NET = "unconnected-(TP1-Pad1)";
const std::string TP2_NET = "unconnected-(TP2-Pad1)";


class RULE_AREA_NETCLASS_FIXTURE : public KI_TEST::SCHEMATIC_TEST_FIXTURE
{
public:
    RULE_AREA_NETCLASS_FIXTURE()
    {
        wxString reserved = wxFileName::CreateTempFileName(
                wxStandardPaths::Get().GetTempDir() + wxFileName::GetPathSeparator()
                + wxS( "kicad_qa_25223_" ) );

        BOOST_REQUIRE( !reserved.IsEmpty() );
        wxRemoveFile( reserved );

        m_tempDir.AssignDir( reserved );

        BOOST_REQUIRE( m_tempDir.Mkdir() );

        // The project gets saved during the test, so run against a copy
        wxFileName source( KI_TEST::GetEeschemaTestDataDir() );
        source.AppendDir( wxS( "issue25223" ) );

        for( const std::string& ext : { FILEEXT::ProjectFileExtension,
                                        FILEEXT::KiCadSchematicFileExtension } )
        {
            wxFileName from( source.GetPath(), wxS( "proj" ), ext );

            BOOST_REQUIRE( wxCopyFile( from.GetFullPath(), ProjFile( ext ).GetFullPath() ) );
        }
    }

    ~RULE_AREA_NETCLASS_FIXTURE()
    {
        m_schematic.reset();

        // Drop the project first so its lock file goes with it rather than after the directory
        SettingsManager().UnloadProject( &SettingsManager().Prj(), false );

        m_tempDir.Rmdir( wxPATH_RMDIR_RECURSIVE );
    }

    wxFileName ProjFile( const std::string& aExt ) const
    {
        return wxFileName( m_tempDir.GetPath(), wxS( "proj" ), aExt );
    }

    nlohmann::json ReadNetclassAssignments() const
    {
        wxFileName    path = ProjFile( FILEEXT::ProjectFileExtension );
        std::ifstream in( path.GetFullPath().fn_str(), std::ios::binary );

        BOOST_REQUIRE( in );

        nlohmann::json project = nlohmann::json::parse( in );

        BOOST_REQUIRE( project.contains( "net_settings" ) );
        BOOST_REQUIRE( project["net_settings"].contains( "netclass_assignments" ) );

        return project["net_settings"]["netclass_assignments"];
    }

    wxFileName m_tempDir;
};

} // namespace


BOOST_FIXTURE_TEST_SUITE( Issue25223RuleAreaNetclass, RULE_AREA_NETCLASS_FIXTURE )


BOOST_AUTO_TEST_CASE( SavedProjectDropsStaleAssignment )
{
    BOOST_REQUIRE( ReadNetclassAssignments().contains( TP1_NET ) );

    LoadSchematic( ProjFile( FILEEXT::KiCadSchematicFileExtension ) );

    NET_SETTINGS& netSettings = *m_schematic->Project().GetProjectFile().m_NetSettings;

    BOOST_REQUIRE( !netSettings.HasNetclassLabelAssignment( TP1_NET ) );
    BOOST_REQUIRE( netSettings.HasNetclassLabelAssignment( TP2_NET ) );

    SettingsManager().SaveProject();

    nlohmann::json assignments = ReadNetclassAssignments();

    BOOST_CHECK( !assignments.contains( TP1_NET ) );
    BOOST_CHECK( assignments.contains( TP2_NET ) );

    // Remove the assignment should now clear the existence
    netSettings.ClearNetclassLabelAssignments();
    SettingsManager().SaveProject();

    assignments = ReadNetclassAssignments();

    BOOST_CHECK( assignments.is_object() );
    BOOST_CHECK( assignments.empty() );
}


BOOST_AUTO_TEST_SUITE_END()
