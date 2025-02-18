/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Author: SYSUEric <jzzhuang666@gmail.com>.
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

#include "odb_attribute.h"
#include <sstream>
#include <iomanip>


size_t ATTR_MANAGER::GetTextIndex( std::unordered_map<std::string, size_t>&     aMap,
                                   std::vector<std::pair<size_t, std::string>>& aVec,
                                   const std::string&                           aText )
{
    if( aMap.count( aText ) )
    {
        return aMap.at( aText );
    }
    else
    {
        auto index = aMap.size();
        aMap.emplace( aText, index );
        aVec.emplace_back( index, aText );

        return index;
    }
}


size_t ATTR_MANAGER::GetAttrNameNumber( const wxString& aName )
{
    return GetTextIndex( m_attrNames, m_attrNameVec, aName.Lower().ToStdString() );
}


size_t ATTR_MANAGER::GetAttrTextNumber( const wxString& aText )
{
    return GetTextIndex( m_attrTexts, m_attrTextVec, aText.Upper().ToStdString() );
}


void ATTR_RECORD_WRITER::WriteAttributes( std::ostream& ost ) const
{
    ODB::CHECK_ONCE once;

    ost << " ";

    for( const auto& attr : m_ODBattributes )
    {
        if( once() )
            ost << ";";
        else
            ost << ",";
        ost << attr.first;
        if( attr.second.size() )
            ost << "=" << attr.second;
    }
}


void ATTR_MANAGER::WriteAttributesName( std::ostream& ost, const std::string& prefix ) const
{
    for( const auto& [n, name] : m_attrNameVec )
    {
        ost << prefix << "@" << n << " " << name << std::endl;
    }
}


void ATTR_MANAGER::WriteAttributesText( std::ostream& ost, const std::string& prefix ) const
{
    for( const auto& [n, name] : m_attrTextVec )
    {
        ost << prefix << "&" << n << " " << name << std::endl;
    }
}


void ATTR_MANAGER::WriteAttributes( std::ostream& ost, const std::string& prefix ) const
{
    ost << std::endl << "#\n#Feature attribute names\n#" << std::endl;
    WriteAttributesName( ost );

    ost << std::endl << "#\n#Feature attribute text strings\n#" << std::endl;
    WriteAttributesText( ost );
}
