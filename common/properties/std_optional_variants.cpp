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

#include <properties/std_optional_variants.h>


STD_OPTIONAL_INT_VARIANT_DATA::STD_OPTIONAL_INT_VARIANT_DATA() :
        wxVariantData()
{}


STD_OPTIONAL_INT_VARIANT_DATA::STD_OPTIONAL_INT_VARIANT_DATA( std::optional<int> aValue ) :
        wxVariantData(),
        m_value( aValue )
{}


bool STD_OPTIONAL_INT_VARIANT_DATA::Eq( wxVariantData& aOther ) const
{
    try
    {
        return dynamic_cast<STD_OPTIONAL_INT_VARIANT_DATA&>( aOther ).m_value == m_value;
    }
    catch( std::bad_cast& )
    {
        return false;
    }
}


STD_OPTIONAL_DOUBLE_VARIANT_DATA::STD_OPTIONAL_DOUBLE_VARIANT_DATA() :
        wxVariantData()
{}


STD_OPTIONAL_DOUBLE_VARIANT_DATA::STD_OPTIONAL_DOUBLE_VARIANT_DATA( std::optional<double> aValue ) :
        wxVariantData(),
        m_value( aValue )
{}


bool STD_OPTIONAL_DOUBLE_VARIANT_DATA::Eq( wxVariantData& aOther ) const
{
    try
    {
        return dynamic_cast<STD_OPTIONAL_DOUBLE_VARIANT_DATA&>( aOther ).m_value == m_value;
    }
    catch( std::bad_cast& )
    {
        return false;
    }
}
