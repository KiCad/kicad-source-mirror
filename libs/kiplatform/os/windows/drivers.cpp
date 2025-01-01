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

#include <core/version_compare.h>
#include <kiplatform/drivers.h>

#include <wx/string.h>
#include <wx/msw/registry.h>

#define MIN_WIN_VERSION "10.7.2"

bool KIPLATFORM::DRIVERS::Valid3DConnexionDriverVersion()
{
    const wxString versionValName = wxT( "Version" );
    wxRegKey smKey( wxRegKey::HKLM, wxT( "Software\\3Dconnexion\\3DxSoftware" ) );

    if( !smKey.Exists() )
        return false;

    if( !smKey.HasValue( versionValName ) )
        return false;

    wxString versionStr;
    if( !smKey.QueryValue( versionValName, versionStr ) )
        return false;

    return !versionStr.empty()
           && compareVersionStrings( MIN_WIN_VERSION, versionStr.ToStdString() );
}