/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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
#include <wx/filename.h>

#include <api/api_plugin_manager.h>
#include <api/python_manager.h>
#include <footprint_wizard.h>
#include <mock_pgm_base.h>
#include <pcbnew_utils/board_file_utils.h>
#include <reporter.h>
#include <settings/common_settings.h>
#include <wx/ffile.h>

BOOST_AUTO_TEST_SUITE( ApiWizards )


// A cold cache builds a venv and installs kicad-python before the plugin is ready
static constexpr int COLD_ENV_TIMEOUT_MS = 120000;


static bool drainPluginManager( int aTimeoutMs )
{
    API_PLUGIN_MANAGER& manager = Pgm().GetPluginManager();

    for( int elapsed = 0; elapsed < aTimeoutMs && manager.Busy(); elapsed += 100 )
    {
        wxSafeYield();
        wxMilliSleep( 100 );
    }

    return !manager.Busy();
}


BOOST_AUTO_TEST_CASE( QueryWizards )
{
    wxFileName qaPath( KI_TEST::GetPcbnewTestDataDir() );
    qaPath.AppendDir( "wizards" );

    API_PLUGIN_MANAGER& manager = Pgm().GetPluginManager();
    manager.ReloadPlugins( qaPath.GetFullPath() );

    BOOST_TEST_MESSAGE( "Waiting for plugins to load..." );

    const bool drained = drainPluginManager( COLD_ENV_TIMEOUT_MS );

    if( Pgm().GetCommonSettings()->m_Api.python_interpreter.IsEmpty() )
        BOOST_TEST_WARN( drained, "Timed out waiting for manager to finish loading plugins" );
    else
        BOOST_TEST( drained, "Timed out waiting for manager to finish loading plugins" );

    std::vector<const PLUGIN_ACTION*> actions = manager.GetActionsForScope( PLUGIN_ACTION_SCOPE::FOOTPRINT_WIZARD );

    BOOST_TEST_WARN( !actions.empty(), "No available wizards to test with (maybe Python or kicad-python unavailable?); skipping tests" );

    if( actions.empty() )
        return;

    for( auto& action : actions )
    {
        BOOST_TEST_CONTEXT( action->identifier )
        {
            // QA test actions should have real metadata
            BOOST_TEST_REQUIRE( !action->description.IsEmpty() );
            BOOST_TEST_REQUIRE( !action->entrypoint.IsEmpty() );
            BOOST_TEST_REQUIRE( action->scopes.contains( PLUGIN_ACTION_SCOPE::FOOTPRINT_WIZARD ) );

            FOOTPRINT_WIZARD wizard;
            wizard.SetIdentifier( action->identifier );
            BOOST_TEST_REQUIRE( FOOTPRINT_WIZARD_MANAGER::RefreshInfo( &wizard ) );

            BOOST_TEST_REQUIRE( wizard.Info().meta.identifier.Matches( action->identifier ) );
        }
    }
}


struct EMPTY_INTERPRETER_FIXTURE
{
    EMPTY_INTERPRETER_FIXTURE() :
            m_interpreter( Pgm().GetCommonSettings()->m_Api.python_interpreter ),
            m_saved( m_interpreter )
    {
        // Jobs left by an earlier case would hold the plugin busy and skip the code under test
        BOOST_REQUIRE_MESSAGE( drainPluginManager( COLD_ENV_TIMEOUT_MS ), "The plugin manager never went idle" );

        wxString cacheHome;
        BOOST_REQUIRE( wxGetEnv( wxS( "KICAD_CACHE_HOME" ), &cacheHome ) && !cacheHome.IsEmpty() );

        std::optional<wxString> env = PYTHON_MANAGER::GetPythonEnvironment( WIZARD_ID );
        BOOST_REQUIRE( env );
        wxFileName::Rmdir( *env, wxPATH_RMDIR_RECURSIVE );

        m_interpreter.Clear();
    }

    ~EMPTY_INTERPRETER_FIXTURE()
    {
        drainPluginManager( COLD_ENV_TIMEOUT_MS );
        m_interpreter = m_saved;
    }

    void LoadWizards( const std::shared_ptr<REPORTER>& aReporter = nullptr )
    {
        wxFileName qaPath( KI_TEST::GetPcbnewTestDataDir() );
        qaPath.AppendDir( "wizards" );

        API_PLUGIN_MANAGER& manager = Pgm().GetPluginManager();
        manager.ReloadPlugins( qaPath.GetFullPath(), aReporter );

        BOOST_REQUIRE( drainPluginManager( COLD_ENV_TIMEOUT_MS ) );
        BOOST_REQUIRE_MESSAGE( manager.GetAction( wxS( "org.kicad.generators.qfp.qfp" ) ),
                               "The wizard plugin did not load" );
    }

    /// Give the wizard an environment whose interpreter differs from the configured one
    static wxFileName PlantVenv()
    {
        std::optional<wxString> env = PYTHON_MANAGER::GetPythonEnvironment( WIZARD_ID );
        BOOST_REQUIRE( env );

        wxFileName cfg( *env, wxS( "pyvenv.cfg" ) );
        wxFFile    file( cfg.GetFullPath(), wxS( "w" ) );
        BOOST_REQUIRE( file.IsOpened() );
        file.Write( wxString::Format( wxS( "home = %s\n" ), cfg.GetPath() ) );
        return cfg;
    }

    static constexpr const wxChar* WIZARD_ID = wxS( "org.kicad.generators.qfp" );

    wxString&      m_interpreter;
    const wxString m_saved;
};


BOOST_FIXTURE_TEST_CASE( EmptyInterpreterReportsMissingPython, EMPTY_INTERPRETER_FIXTURE )
{
    wxFileName venvCfg = PlantVenv();
    auto       reporter = std::make_shared<WX_STRING_REPORTER>();

    LoadWizards( reporter );

    BOOST_TEST_INFO( reporter->GetMessages() );
    BOOST_CHECK( reporter->GetMessages().Contains( wxS( "No Python interpreter is configured" ) ) );
    BOOST_CHECK( !reporter->GetMessages().Contains( wxS( "Could not create plugin environment" ) ) );
    BOOST_CHECK( venvCfg.FileExists() );
}


BOOST_FIXTURE_TEST_CASE( EmptyInterpreterKeepsVenvOnRecreate, EMPTY_INTERPRETER_FIXTURE )
{
    LoadWizards();
    wxFileName venvCfg = PlantVenv();

    BOOST_CHECK( !Pgm().GetPluginManager().RecreatePluginEnvironment( WIZARD_ID ) );
    BOOST_CHECK( venvCfg.FileExists() );
}


BOOST_FIXTURE_TEST_CASE( SettingInterpreterBuildsSkippedEnvironment, EMPTY_INTERPRETER_FIXTURE )
{
    LoadWizards();
    BOOST_REQUIRE( !Pgm().GetPluginManager().Busy() );

    // Any path will do; the job only has to be scheduled
    m_interpreter = wxS( "kicad-qa-no-such-python" );
    Pgm().GetPluginManager().HandlePythonInterpreterChanged();

    BOOST_CHECK( Pgm().GetPluginManager().Busy() );
}


BOOST_AUTO_TEST_CASE( EmptyInterpreterNeverExecutes )
{
    PYTHON_MANAGER manager( wxEmptyString );
    auto           error = std::make_shared<wxString>();

    long pid = manager.Execute( { wxS( "--version" ) },
                                [error]( int, const wxString&, const wxString& aError )
                                {
                                    *error = aError;
                                } );

    BOOST_CHECK( manager.GetInterpreterPath().IsEmpty() );
    BOOST_CHECK_EQUAL( pid, 0 );
    BOOST_CHECK( error->Contains( wxS( "No Python interpreter is configured" ) ) );
}


BOOST_AUTO_TEST_SUITE_END()
