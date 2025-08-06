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

#include <boost/test/unit_test.hpp>

#include <ui_events.h>
#include <build_version.h>
#include <paths.h>
#include <wx/filename.h>
#include <wx/string.h>

#include <notifications_manager.h>

BOOST_AUTO_TEST_SUITE( Notifications )

BOOST_AUTO_TEST_CASE( CreateAndPersist )
{
    wxFileName tmpDir( wxFileName::GetTempDir(), "" );
    wxString envPath = tmpDir.GetFullPath();

    wxSetEnv( wxS("KICAD_CACHE_HOME"), envPath );
    wxFileName::Mkdir( PATHS::GetUserCachePath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

    NOTIFICATIONS_MANAGER mgr;
    mgr.CreateOrUpdate( wxS("key"), wxS("Title"), wxS("Desc") );
    mgr.Save();

    wxFileName fn( PATHS::GetUserCachePath(), wxS("notifications.json") );
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

