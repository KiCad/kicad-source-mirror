/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
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

/**
 * Main file for the pcbnew tests to be compiled
 */
#include <boost/test/unit_test.hpp>
#include <kiplatform/app.h>
#include <mock_pgm_base.h>
#include <settings/settings_manager.h>
#include <pcbnew_settings.h>
#include <qa_utils/error_handlers.h>
#include <pcbnew_utils/board_test_utils.h>

#include <wx/image.h>
#include <wx/init.h>
#include <wx/app.h>

using KI_TEST::STALE_DRU_SIDECAR_FIXTURE;

BOOST_TEST_GLOBAL_FIXTURE( KI_SIGNAL_HANDLER_FIXTURE );
BOOST_TEST_GLOBAL_FIXTURE( KI_STACK_TRACE_FIXTURE );
BOOST_TEST_GLOBAL_FIXTURE( STALE_DRU_SIDECAR_FIXTURE );

bool init_unit_test()
{
    KI_TEST::SetMockConfigDir();
    SetPgm( new MOCK_PGM_BASE() );
    KIPLATFORM::APP::Init();
    boost::unit_test::framework::master_test_suite().p_name.value = "Pcbnew module tests";

    wxApp::SetInstance( new wxAppConsole );

    bool ok = wxInitialize( boost::unit_test::framework::master_test_suite().argc,
                            boost::unit_test::framework::master_test_suite().argv );

    if( ok )
    {
        wxSetAssertHandler( &KI_TEST::wxAssertThrower );

        Pgm().InitPgm( true, true );
        Pgm().GetSettingsManager().RegisterSettings( new PCBNEW_SETTINGS, false );
        Pgm().GetSettingsManager().Load();

        const MOCK_PGM_BASE& program = static_cast<MOCK_PGM_BASE&>( Pgm() );
        Pgm().GetSettingsManager().LoadProject( "" );
    }

    return ok;
}


int main( int argc, char* argv[] )
{
    int ret = boost::unit_test::unit_test_main( &init_unit_test, argc, argv );

    // This causes some glib warnings on GTK3 (http://trac.wxwidgets.org/ticket/18274)
    // but without it, Valgrind notices a lot of leaks from WX
    wxUninitialize();

    return ret;
}
