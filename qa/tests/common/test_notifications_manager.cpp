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
#include <qa_utils/file_utils.h>
#include <qa_utils/env_var_utils.h>

#include <ui_events.h>
#include <build_version.h>
#include <paths.h>
#include <wx/filename.h>
#include <wx/string.h>

#include <notifications_manager.h>

BOOST_AUTO_TEST_SUITE( Notifications )

BOOST_AUTO_TEST_CASE( CreateAndPersist )
{
    KI_TEST::SCOPED_TEMP_DIR        tempDir( "notifications_manager_test" );
    KI_TEST::SCOPED_PROCESS_ENV_VAR env( wxS( "KICAD_CACHE_HOME" ), tempDir.PathStr() );

    const wxString userCachePath = PATHS::GetUserCachePath();

    // Make srue this is in the temp dir so we don't pollute anything
    BOOST_REQUIRE_MESSAGE( userCachePath.StartsWith( tempDir.PathStr() ),
                           "KICAD_CACHE_HOME must be in a temp dir for this test" );

    wxFileName::Mkdir( userCachePath, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

    NOTIFICATIONS_MANAGER mgr;
    mgr.CreateOrUpdate( wxS( "key" ), wxS( "Title" ), wxS( "Desc" ) );
    mgr.Save();

    wxFileName fn( userCachePath, wxS( "notifications.json" ) );
    BOOST_CHECK( fn.FileExists() );
}


class TEST_HANDLER : public wxEvtHandler
{
public:
    bool triggered = false;
    void OnEvent( wxCommandEvent& ) { triggered = true; }
};

BOOST_AUTO_TEST_CASE( EventDispatch )
{
    TEST_HANDLER handler;
    handler.Bind( EDA_EVT_UNITS_CHANGED, &TEST_HANDLER::OnEvent, &handler );
    wxCommandEvent evt( EDA_EVT_UNITS_CHANGED );
    handler.ProcessEvent( evt );
    BOOST_CHECK( handler.triggered );
}

BOOST_AUTO_TEST_SUITE_END()
