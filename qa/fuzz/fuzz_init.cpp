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

#include <wx/init.h>
#include <wx/app.h>
#include <kiplatform/app.h>
#include <mock_pgm_base.h>
#include <settings/settings_manager.h>
#include <pcbnew_settings.h>


int fuzz_init( int* argc, char*** argv )
{
    KI_TEST::SetMockConfigDir();
    SetPgm( new MOCK_PGM_BASE() );
    KIPLATFORM::APP::Init();

    wxApp::SetInstance( new wxAppConsole );

    if( !wxInitialize( *argc, *argv ) )
    {
        return 1;
    }

    wxSetAssertHandler( &KI_TEST::wxAssertThrower );

    Pgm().InitPgm( true, true );

    Pgm().GetSettingsManager().RegisterSettings( new PCBNEW_SETTINGS, false );

    Pgm().GetSettingsManager().Load();
    Pgm().GetSettingsManager().LoadProject( "" );

    return 0;
}
