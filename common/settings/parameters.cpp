/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <nlohmann/json.hpp>

#include <gal/color4d.h>
#include <project/project_file.h>
#include <settings/parameters.h>


template <typename ValueType>
void PARAM_LAMBDA<ValueType>::Load( JSON_SETTINGS* aSettings, bool aResetIfMissing ) const
{
    if( m_readOnly )
        return;

    if( std::is_same<ValueType, nlohmann::json>::value )
    {
        if( OPT<nlohmann::json> optval = aSettings->GetJson( m_path ) )
            m_setter( *optval );
        else
            m_setter( m_default );
    }
    else
    {
        if( OPT<ValueType> optval = aSettings->Get<ValueType>( m_path ) )
            m_setter( *optval );
        else
            m_setter( m_default );
    }
}


template <typename ValueType>
bool PARAM_LAMBDA<ValueType>::MatchesFile( JSON_SETTINGS* aSettings ) const
{
    if( std::is_same<ValueType, nlohmann::json>::value )
    {
        if( OPT<nlohmann::json> optval = aSettings->GetJson( m_path ) )
            return *optval == m_getter();
    }
    else
    {
        if( OPT<ValueType> optval = aSettings->Get<ValueType>( m_path ) )
            return *optval == m_getter();
    }

    // Not in file
    return false;
}


// Instantiate all required templates here to allow reducing scope of json.hpp
template class PARAM_LAMBDA<bool>;
template class PARAM_LAMBDA<int>;
template class PARAM_LAMBDA<nlohmann::json>;
template class PARAM_LAMBDA<std::string>;


template <typename ValueType>
void PARAM_LIST<ValueType>::Load( JSON_SETTINGS* aSettings, bool aResetIfMissing ) const
{
    if( m_readOnly )
        return;

    if( OPT<nlohmann::json> js = aSettings->GetJson( m_path ) )
    {
        std::vector<ValueType> val;

        if( js->is_array() )
        {
            for( const auto& el : js->items() )
                val.push_back( el.value().get<ValueType>() );
        }

        *m_ptr = val;
    }
    else if( aResetIfMissing )
        *m_ptr = m_default;
}


template <typename ValueType>
void PARAM_LIST<ValueType>::Store( JSON_SETTINGS* aSettings ) const
{
    nlohmann::json js = nlohmann::json::array();

    for( const auto& el : *m_ptr )
        js.push_back( el );

    aSettings->Set<nlohmann::json>( m_path, js );
}


template <typename ValueType>
bool PARAM_LIST<ValueType>::MatchesFile( JSON_SETTINGS* aSettings ) const
{
    if( OPT<nlohmann::json> js = aSettings->GetJson( m_path ) )
    {
        if( js->is_array() )
        {
            std::vector<ValueType> val;

            for( const auto& el : js->items() )
                val.emplace_back( el.value().get<ValueType>() );

            return val == *m_ptr;
        }
    }

    return false;
}


template class PARAM_LIST<int>;
template class PARAM_LIST<double>;
template class PARAM_LIST<wxString>;
template class PARAM_LIST<KIGFX::COLOR4D>;
template class PARAM_LIST<FILE_INFO_PAIR>;


void PARAM_PATH_LIST::Store( JSON_SETTINGS* aSettings ) const
{
    nlohmann::json js = nlohmann::json::array();

    for( const auto& el : *m_ptr )
        js.push_back( toFileFormat( el ) );

    aSettings->Set<nlohmann::json>( m_path, js );
}


bool PARAM_PATH_LIST::MatchesFile( JSON_SETTINGS* aSettings ) const
{
    if( OPT<nlohmann::json> js = aSettings->GetJson( m_path ) )
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


template <typename Value>
void PARAM_MAP<Value>::Load( JSON_SETTINGS* aSettings, bool aResetIfMissing ) const
{
    if( m_readOnly )
        return;

    if( OPT<nlohmann::json> js = aSettings->GetJson( m_path ) )
    {
        if( js->is_object() )
        {
            m_ptr->clear();

            for( const auto& el : js->items() )
                ( *m_ptr )[el.key()] = el.value().get<Value>();
        }
    }
    else if( aResetIfMissing )
        *m_ptr = m_default;
}


template <typename Value>
void PARAM_MAP<Value>::Store( JSON_SETTINGS* aSettings ) const
{
    nlohmann::json js( {} );

    for( const auto& el : *m_ptr )
        js[el.first] = el.second;

    aSettings->Set<nlohmann::json>( m_path, js );
}


template <typename Value>
bool PARAM_MAP<Value>::MatchesFile( JSON_SETTINGS* aSettings ) const
{
    if( OPT<nlohmann::json> js = aSettings->GetJson( m_path ) )
    {
        if( js->is_object() )
        {
            if( m_ptr->size() != js->size() )
                return false;

            std::map<std::string, Value> val;

            for( const auto& el : js->items() )
                val[el.key()] = el.value().get<Value>();

            return val == *m_ptr;
        }
    }

    return false;
}


template class PARAM_MAP<int>;
template class PARAM_MAP<double>;
template class PARAM_MAP<bool>;


void PARAM_WXSTRING_MAP::Load( JSON_SETTINGS* aSettings, bool aResetIfMissing ) const
{
    if( m_readOnly )
        return;

    if( OPT<nlohmann::json> js = aSettings->GetJson( m_path ) )
    {
        if( js->is_object() )
        {
            m_ptr->clear();

            for( const auto& el : js->items() )
            {
                ( *m_ptr )[wxString( el.key().c_str(), wxConvUTF8 )] = el.value().get<wxString>();
            }
        }
    }
    else if( aResetIfMissing )
        *m_ptr = m_default;
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


bool PARAM_WXSTRING_MAP::MatchesFile( JSON_SETTINGS* aSettings ) const
{
    if( OPT<nlohmann::json> js = aSettings->GetJson( m_path ) )
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
