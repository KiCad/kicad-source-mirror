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

#include <boost/test/unit_test.hpp>

#include <mdw/client/mdw_client.h>
#include <mdw/client/mdw_library_sources.h>
#include <mdw/mdw_framework.h>
#include <mdw/mdw_session.h>
#include <project.h>
#include <project/project_file.h>
#include <project/project_local_settings.h>
#include <settings/settings_manager.h>

#include <qa_utils/file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include "mdw_git_test_utils.h"

#include <json_common.h>

#include <fstream>
#include <iterator>

namespace
{
std::string slurp( const fs::path& aFile )
{
    std::ifstream in( aFile, std::ios::binary );

    return std::string( std::istreambuf_iterator<char>( in ), std::istreambuf_iterator<char>() );
}


bool disabledInPrl( const fs::path& aFile )
{
    nlohmann::json prl = nlohmann::json::parse( slurp( aFile ) );

    return prl.contains( "workspace" ) && prl["workspace"].value( "disabled_here", false );
}
} // namespace


BOOST_AUTO_TEST_SUITE( MdwDisconnectHere )


// A project bound by the team, left on one computer, must not be unbound for the rest of the team
BOOST_AUTO_TEST_CASE( DisconnectHereKeepsSharedBinding )
{
    SCOPED_DATA_DIR          dataDir;
    SCOPED_PGM               program;
    KI_TEST::SCOPED_TEMP_DIR temp( wxS( "mdw_disconnect_here" ) );

    Pgm().SetMdwEnabled( true );

    const fs::path projectFile = temp.Path() / "amp-board.kicad_pro";
    const fs::path localFile = temp.Path() / "amp-board.kicad_prl";

    fs::copy_file( fs::path( KI_TEST::GetTestDataRootDir() ) / "common/git/mdw_ctrl_board/ctrl-board.kicad_pro",
                   projectFile );

    SETTINGS_MANAGER manager;
    const wxString   projectPath = wxString::FromUTF8( projectFile.string() );

    // Inactive, since the program here has no library manager for an active project to tell
    BOOST_REQUIRE( manager.LoadProject( projectPath, false ) );

    PROJECT& project = *manager.GetProject( projectPath );

    project.GetProjectFile().m_WorkspaceBackend = wxS( "org.kicad.mdw.git" );
    BOOST_REQUIRE( manager.SaveProject( projectPath, &project ) );

    const std::string shared = slurp( projectFile );

    BOOST_REQUIRE( shared.find( "org.kicad.mdw.git" ) != std::string::npos );
    BOOST_REQUIRE( MDW_CLIENT::Get().ProjectConnection( project ).status.phase != MDW_CONNECTION::UNBOUND );

    MDW_SETUP::DisconnectHere( project );

    BOOST_CHECK( slurp( projectFile ) == shared );
    BOOST_CHECK( project.GetProjectFile().m_WorkspaceBackend == wxS( "org.kicad.mdw.git" ) );
    BOOST_REQUIRE( fs::exists( localFile ) );
    BOOST_CHECK( disabledInPrl( localFile ) );
    BOOST_CHECK( MDW_CLIENT::Get().ProjectConnection( project ).status.phase == MDW_CONNECTION::UNBOUND );

    MDW_SETUP::ReconnectHere( project );

    BOOST_CHECK( !disabledInPrl( localFile ) );
    BOOST_CHECK( MDW_CLIENT::Get().ProjectConnection( project ).status.phase != MDW_CONNECTION::UNBOUND );
    BOOST_CHECK( slurp( projectFile ) == shared );
}


// The session stays open when the choice cannot be saved, since the next open would undo it
BOOST_AUTO_TEST_CASE( DisconnectHereFailsWhenItCannotBeSaved,
                      *boost::unit_test::precondition( KI_TEST::WriteBitsBind ) )
{
    SCOPED_DATA_DIR          dataDir;
    SCOPED_PGM               program;
    KI_TEST::SCOPED_TEMP_DIR temp( wxS( "mdw_disconnect_here_ro" ) );

    Pgm().SetMdwEnabled( true );

    const fs::path projectFile = temp.Path() / "amp-board.kicad_pro";
    const fs::path localFile = temp.Path() / "amp-board.kicad_prl";

    fs::copy_file( fs::path( KI_TEST::GetTestDataRootDir() ) / "common/git/mdw_ctrl_board/ctrl-board.kicad_pro",
                   projectFile );

    SETTINGS_MANAGER manager;
    const wxString   projectPath = wxString::FromUTF8( projectFile.string() );

    BOOST_REQUIRE( manager.LoadProject( projectPath, false ) );

    PROJECT& project = *manager.GetProject( projectPath );

    project.GetProjectFile().m_WorkspaceBackend = wxS( "org.kicad.mdw.git" );
    BOOST_REQUIRE( manager.SaveProject( projectPath, &project ) );
    BOOST_REQUIRE( project.GetLocalSettings().SaveToFile( project.GetProjectPath(), true ) );

    fs::permissions( localFile, fs::perms::owner_write | fs::perms::group_write | fs::perms::others_write,
                     fs::perm_options::remove );

    const bool disconnected = MDW_SETUP::DisconnectHere( project );

    fs::permissions( localFile, fs::perms::owner_write, fs::perm_options::add );

    BOOST_CHECK( !disconnected );
    BOOST_CHECK( !project.GetLocalSettings().m_WorkspaceDisabledHere );
    BOOST_CHECK( MDW_CLIENT::Get().ProjectConnection( project ).status.phase != MDW_CONNECTION::UNBOUND );
}


// The target the search picks lands in the locator under its own key
BOOST_AUTO_TEST_CASE( LocatorTakesTheChosenTarget )
{
    BOOST_CHECK( MDW_SETUP::LocatorWithItem( wxS( "https://odoo.example.com/?db=acme" ), wxS( "project=PCBA-AMP-001" ) )
                 == wxS( "https://odoo.example.com/?db=acme&project=PCBA-AMP-001" ) );
    BOOST_CHECK( MDW_SETUP::LocatorWithItem( wxS( "https://odoo.example.com/?project=OLD&db=acme" ),
                                             wxS( "project=A%20B" ) )
                 == wxS( "https://odoo.example.com/?project=A%20B&db=acme" ) );
    BOOST_CHECK( MDW_SETUP::LocatorWithItem( wxS( "https://odoo.example.com" ), wxS( "project=X" ) )
                 == wxS( "https://odoo.example.com?project=X" ) );
}


// A backend that lists nothing to bind to answers not supported, which the dialog reads as no search
BOOST_AUTO_TEST_CASE( BackendWithoutBindingTargetsIsNotSupported )
{
    SCOPED_DATA_DIR dataDir;
    SCOPED_PGM      program;

    Pgm().SetMdwEnabled( true );

    MDW_FRAMEWORK* framework = Pgm().MdwFrameworkOrNull();
    BOOST_REQUIRE( framework );

    offerRealmBackend( *framework, dataDir );

    std::shared_ptr<MDW_SESSION> session = framework->OpenSession(
            wxString::FromUTF8( ( dataDir.Path() / "board.kicad_pro" ).string() ), realmBinding(), 8000 );

    BOOST_REQUIRE( session );

    auto [result, targets] = MDW_SETUP::FindBindingTargets( session, wxEmptyString );

    BOOST_CHECK( result.error == MDW_ERROR_CODE::NOT_SUPPORTED );
    BOOST_CHECK( targets.empty() );
}


BOOST_AUTO_TEST_SUITE_END()
