/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <sim/sim_value.h>
#include <locale_io.h>
#include <complex>


std::unique_ptr<SIM_VALUE_BASE> SIM_VALUE_BASE::Create( TYPE aType, wxString aString )
{
    std::unique_ptr<SIM_VALUE_BASE> value = SIM_VALUE_BASE::Create( aType );
    value->FromString( aString );
    return value;
}


std::unique_ptr<SIM_VALUE_BASE> SIM_VALUE_BASE::Create( TYPE aType )
{
    switch( aType )
    {
    case TYPE::BOOL:           return std::make_unique<SIM_VALUE<bool>>();
    case TYPE::INT:            return std::make_unique<SIM_VALUE<long>>();
    case TYPE::FLOAT:          return std::make_unique<SIM_VALUE<double>>();
    case TYPE::COMPLEX:        return std::make_unique<SIM_VALUE<std::complex<double>>>();
    case TYPE::STRING:         return std::make_unique<SIM_VALUE<wxString>>();
    case TYPE::BOOL_VECTOR:    return std::make_unique<SIM_VALUE<bool>>();
    case TYPE::INT_VECTOR:     return std::make_unique<SIM_VALUE<long>>();
    case TYPE::FLOAT_VECTOR:   return std::make_unique<SIM_VALUE<double>>();
    case TYPE::COMPLEX_VECTOR: return std::make_unique<SIM_VALUE<std::complex<double>>>();
    }
    
    wxFAIL_MSG( "Unknown SIM_VALUE type" );
    return nullptr;
}


void SIM_VALUE_BASE::operator=( const wxString& aString )
{
    FromString( aString );
}


template <typename T>
SIM_VALUE<T>::SIM_VALUE( const T& aValue ) : m_value(aValue)
{
}


template <typename T>
void SIM_VALUE<T>::FromString( const wxString& aString )
{
    LOCALE_IO toggle;
}


template <typename T>
wxString SIM_VALUE<T>::ToString() const
{
    static_assert( std::is_same<T, std::vector<T>>::value );

    wxString string = "";

    for( auto it = m_value.cbegin(); it != m_value.cend(); it++ )
    {
        string += SIM_VALUE<T>( *it ).ToString();
        string += ",";
    }

    return string;
}


template <>
wxString SIM_VALUE<bool>::ToString() const
{
    LOCALE_IO toggle;
    return wxString::Format( "%d", m_value );
}


template <>
wxString SIM_VALUE<long>::ToString() const
{
    LOCALE_IO toggle;
    return wxString::Format( "%d", m_value );
}


template <>
wxString SIM_VALUE<double>::ToString() const
{
    LOCALE_IO toggle;
    return wxString::Format( "%f", m_value );
}


template <>
wxString SIM_VALUE<std::complex<double>>::ToString() const
{
    LOCALE_IO toggle;
    return wxString::Format( "%f+%fi", m_value.real(), m_value.imag() );
}


template <>
wxString SIM_VALUE<wxString>::ToString() const
{
    LOCALE_IO toggle;
    return m_value;
}


template <typename T>
bool SIM_VALUE<T>::operator==( const SIM_VALUE_BASE& aOther ) const
{
    const SIM_VALUE* otherNumber = dynamic_cast<const SIM_VALUE*>( &aOther );

    if( otherNumber )
        return m_value == otherNumber->m_value;

    return false;
}
