/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
* Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <kiplatform/policy.h>

#include <wx/string.h>
#include <wx/msw/registry.h>

#define POLICY_KEY_ROOT "Software\\Policies\\KiCad\\KiCad"


KIPLATFORM::POLICY::STATE KIPLATFORM::POLICY::GetPolicyState( const wxString& aKey )
{
    wxRegKey key( wxRegKey::HKLM, POLICY_KEY_ROOT );
    if( key.Exists() )
    {
        long value;
        if( key.QueryValue( aKey, &value ) )
        {
            if( value == 1 )
                return POLICY::STATE::ENABLED;
            else
                return POLICY::STATE::DISABLED;
        }
    }

    return STATE::NOT_CONFIGURED;
}