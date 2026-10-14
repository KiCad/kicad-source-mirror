/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include "qa_utils/mock_utils.h"

#include <wx/filename.h>
#include <wx/utils.h>

#include <qa_utils/wx_utils/unit_test_utils.h>


void KI_TEST::SetMockConfigDir()
{
    if( !wxGetEnv( wxT( "KICAD_CONFIG_HOME" ), nullptr ) )
    {
        wxString path( GetTestDataRootDir() );
        path += wxT( "/config/" );
        wxSetEnv( wxT( "KICAD_CONFIG_HOME" ), path );
        wxSetEnv( wxT( "KICAD_CONFIG_HOME_IS_QA" ), wxT( "1" ) );

        if( wxFileName fn( path ); fn.DirExists() )
        {
            // Under normal circumstances, don't let QA test runs write back to the config dir
            // to avoid churn.  Disable this if you want to update the QA schema.
            wxSetEnv( wxT( "KICAD_INHIBIT_SETTINGS_WRITES" ), wxT( "1" ) );
        }
    }
}
