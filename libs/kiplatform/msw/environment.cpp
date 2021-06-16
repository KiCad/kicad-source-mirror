/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Ian McInerney <Ian.S.McInerney at ieee.org>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <kiplatform/environment.h>
#include <wx/intl.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/string.h>
#include <wx/app.h>

#include <Windows.h>
#include <shellapi.h>
#include <shlwapi.h>


void KIPLATFORM::ENV::Init()
{
    // No tasks for this platform
}


bool KIPLATFORM::ENV::MoveToTrash( const wxString& aPath, wxString& aError )
{
    // The filename field must be a double-null terminated string
    wxString temp = aPath + '\0';

    SHFILEOPSTRUCT fileOp;
    ::ZeroMemory( &fileOp, sizeof( fileOp ) );

    fileOp.hwnd   = NULL; // Set to null since there is no progress dialog
    fileOp.wFunc  = FO_DELETE;
    fileOp.pFrom  = temp.c_str();
    fileOp.pTo    = NULL; // Set to to NULL since we aren't moving the file
    fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOERRORUI | FOF_NOCONFIRMATION | FOF_SILENT;

    int eVal = SHFileOperation( &fileOp );

    if( eVal != 0 )
    {
        aError = wxString::Format( _( "Error code: %d" ), eVal );
        return false;
    }

    return true;
}


bool KIPLATFORM::ENV::IsNetworkPath( const wxString& aPath )
{
    return ::PathIsNetworkPathW( aPath.wc_str() );
}


wxString KIPLATFORM::ENV::GetDocumentsPath()
{
    // If called by a python script in stand-alone (outside kicad), wxStandardPaths::Get()
    // complains about not existing app. so use a dummy app
    if( wxTheApp ==  nullptr )
    {
        wxApp dummy;
        return wxStandardPaths::Get().GetDocumentsDir();
    }

    return wxStandardPaths::Get().GetDocumentsDir();
}


wxString KIPLATFORM::ENV::GetUserConfigPath()
{
    // If called by a python script in stand-alone (outside kicad), wxStandardPaths::Get()
    // complains about not existing app. so use a dummy app
    if( wxTheApp ==  nullptr )
    {
        wxApp dummy;
        return wxStandardPaths::Get().GetUserConfigDir();
    }

    return wxStandardPaths::Get().GetUserConfigDir();
}


wxString KIPLATFORM::ENV::GetUserCachePath()
{
    // Unfortunately AppData/Local is the closest analog to "Cache" directories of other platforms

    // Make sure we dont include the "appinfo" (appended app name)

    // If called by a python script in stand-alone (outside kicad), wxStandardPaths::Get()
    // complains about not existing app. so use a dummy app
    if( wxTheApp ==  nullptr )
    {
        wxApp dummy;
        wxStandardPaths::Get().UseAppInfo( wxStandardPaths::AppInfo_None );

        return wxStandardPaths::Get().GetUserLocalDataDir();
   }

    wxStandardPaths::Get().UseAppInfo( wxStandardPaths::AppInfo_None );

    return wxStandardPaths::Get().GetUserLocalDataDir();
}
