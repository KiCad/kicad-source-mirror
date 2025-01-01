/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
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

#include <kiplatform/policy.h>

#include <wx/string.h>
#include <wx/tokenzr.h>
#include <wx/msw/registry.h>

#include <memory>

#define POLICY_KEY_ROOT "Software\\Policies\\KiCad\\KiCad"


static wxRegKey* GetPolicyRegKey( wxString& aKey )
{
    wxString  key = aKey;
    wxRegKey* keyToUse = nullptr;

    wxString keyPath = POLICY_KEY_ROOT;

    wxStringTokenizer tokenizer( aKey, "\\" );
    while( tokenizer.HasMoreTokens() )
    {
        wxString token = tokenizer.GetNextToken();

        if( tokenizer.HasMoreTokens() )
        {
            keyPath.Append( "\\" );
            keyPath.Append( token );
        }
        else
            key = token;
    }

    std::unique_ptr<wxRegKey> userKey = std::make_unique<wxRegKey>( wxRegKey::HKCU, keyPath );

    // we have user level policies take precedence over computer level policies
    if( userKey->Exists() && userKey->HasValue( key ) )
    {
        keyToUse = userKey.release();
    }
    else
    {
        std::unique_ptr<wxRegKey> compKey = std::make_unique<wxRegKey>( wxRegKey::HKLM, keyPath );

        if( compKey->Exists() && compKey->HasValue( key ) )
        {
            keyToUse = compKey.release();
        }
    }

    aKey = key;
    return keyToUse;
}


KIPLATFORM::POLICY::PBOOL KIPLATFORM::POLICY::GetPolicyBool( const wxString& aKey )
{
    wxString  key = aKey;
    std::unique_ptr<wxRegKey> keyToUse( GetPolicyRegKey( key ) );

    if( keyToUse != nullptr )
    {
        long value;
        if( keyToUse->QueryValue( key, &value ) )
        {
            if( value == 1 )
                return POLICY::PBOOL::ENABLED;
            else
                return POLICY::PBOOL::DISABLED;
        }
    }

    return PBOOL::NOT_CONFIGURED;
}


std::uint32_t KIPLATFORM::POLICY::GetPolicyEnumUInt( const wxString& aKey )
{
    wxString  key = aKey;
    std::unique_ptr<wxRegKey> keyToUse( GetPolicyRegKey( key ) );

    if( keyToUse != nullptr )
    {
        long value;
        if( keyToUse->QueryValue( key, &value ) )
        {
            return value;
        }
    }

    return 0;
}