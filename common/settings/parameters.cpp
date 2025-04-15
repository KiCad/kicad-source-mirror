/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Jon Evans <jon@craftyjon.com>
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

#include <wx/string.h>

#include <json_common.h>

#include <project/project_file.h>
#include <settings/parameters.h>

void PARAM_PATH_LIST::Store( JSON_SETTINGS* aSettings ) const
{
    nlohmann::json js = nlohmann::json::array();

    for( const auto& el : *m_ptr )
        js.push_back( toFileFormat( el ) );

    aSettings->Set<nlohmann::json>( m_path, js );
}


bool PARAM_PATH_LIST::MatchesFile( const JSON_SETTINGS& aSettings ) const
{
    if( std::optional<nlohmann::json> js = aSettings.GetJson( m_path ) )
    {
        if( js->is_array() )
        {
            std::vector<wxString> val;

            for( const auto& el : js->items() )
                val.emplace_back( fromFileFormat( el.value().get<wxString>() ) );

            return val == *m_ptr;
        }
    }

    return false;
}


void PARAM_WXSTRING_MAP::Load( const JSON_SETTINGS& aSettings, bool aResetIfMissing ) const
{
    if( m_readOnly )
        return;

    if( std::optional<nlohmann::json> js = aSettings.GetJson( m_path ) )
    {
        if( js->is_object() )
        {
            m_ptr->clear();

            for( const auto& el : js->items() )
                ( *m_ptr )[wxString( el.key().c_str(), wxConvUTF8 )] = el.value().get<wxString>();
        }
    }
    else if( aResetIfMissing )
    {
        *m_ptr = m_default;
    }
}


void PARAM_WXSTRING_MAP::Store( JSON_SETTINGS* aSettings ) const
{
    nlohmann::json js( {} );

    for( const auto& el : *m_ptr )
    {
        std::string key( el.first.ToUTF8() );
        js[key] = el.second;
    }

    aSettings->Set<nlohmann::json>( m_path, js );
}


bool PARAM_WXSTRING_MAP::MatchesFile( const JSON_SETTINGS& aSettings ) const
{
    if( std::optional<nlohmann::json> js = aSettings.GetJson( m_path ) )
    {
        if( js->is_object() )
        {
            if( m_ptr->size() != js->size() )
                return false;

            std::map<wxString, wxString> val;

            for( const auto& el : js->items() )
            {
                wxString key( el.key().c_str(), wxConvUTF8 );
                val[key] = el.value().get<wxString>();
            }

            return val == *m_ptr;
        }
    }

    return false;
}

#if !defined( __MINGW32__ )
// Instantiate all required templates here and export
template class KICOMMON_API PARAM_LAMBDA<bool>;
template class KICOMMON_API PARAM_LAMBDA<int>;
template class KICOMMON_API PARAM_LAMBDA<nlohmann::json>;
template class KICOMMON_API PARAM_LAMBDA<std::string>;

template class KICOMMON_API PARAM_LIST<bool>;
template class KICOMMON_API PARAM_LIST<int>;
template class KICOMMON_API PARAM_LIST<double>;
template class KICOMMON_API PARAM_LIST<wxString>;
template class KICOMMON_API PARAM_LIST<KIGFX::COLOR4D>;
//template KICOMMON_API class PARAM_LIST<FILE_INFO_PAIR>;
template class KICOMMON_API PARAM_LIST<GRID>;

template class KICOMMON_API PARAM_SET<wxString>;

template class KICOMMON_API PARAM_MAP<int>;
template class KICOMMON_API PARAM_MAP<double>;
template class KICOMMON_API PARAM_MAP<bool>;
#endif