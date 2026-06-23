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
#include <footprint_wizard.h>
#include <mock_pgm_base.h>
#include <pcbnew_utils/board_file_utils.h>

BOOST_AUTO_TEST_SUITE( ApiWizards )


BOOST_AUTO_TEST_CASE( QueryWizards )
{
    wxFileName qaPath( KI_TEST::GetPcbnewTestDataDir() );
    qaPath.AppendDir( "wizards" );

    API_PLUGIN_MANAGER& manager = Pgm().GetPluginManager();
    manager.ReloadPlugins( qaPath.GetFullPath() );

    constexpr int timeoutMs = 10000;
    constexpr int sleepMs = 100;
    int elapsed = 0;

    BOOST_TEST_MESSAGE( "Waiting for plugins to load..." );

    while( elapsed < timeoutMs )
    {
        wxSafeYield();
        wxMilliSleep( sleepMs );
        elapsed += sleepMs;

        if( !manager.Busy() )
            break;
    }

    BOOST_TEST_WARN( elapsed < timeoutMs, "Timed out waiting for manager to finish loading plugins" );

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

BOOST_AUTO_TEST_SUITE_END()
