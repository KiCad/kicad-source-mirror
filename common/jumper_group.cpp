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

#include "jumper_group.h"

#include <utility>


JUMPER_GROUP::JUMPER_GROUP( std::set<wxString> aNames ) :
        m_names( std::move( aNames ) )
{
}


std::optional<JUMPER_GROUP> JUMPER_GROUP::Make( std::set<wxString> aNames )
{
    // A blank name can never resolve to a pin or pad.
    std::erase_if( aNames,
                   []( const wxString& aName )
                   {
                       return aName.IsEmpty();
                   } );

    if( aNames.empty() )
        return std::nullopt;

    return JUMPER_GROUP( std::move( aNames ) );
}


bool JUMPER_GROUP::Contains( const wxString& aName ) const
{
    return m_names.find( aName ) != m_names.end();
}


void JUMPER_GROUP_SET::Add( JUMPER_GROUP aGroup )
{
    m_groups.push_back( std::move( aGroup ) );
}


void JUMPER_GROUP_SET::Add( std::set<wxString> aNames )
{
    std::optional<JUMPER_GROUP> group = JUMPER_GROUP::Make( std::move( aNames ) );

    if( group )
        Add( std::move( *group ) );
}


const JUMPER_GROUP* JUMPER_GROUP_SET::FindContaining( const wxString& aName ) const
{
    for( const JUMPER_GROUP& group : m_groups )
    {
        if( group.Contains( aName ) )
            return &group;
    }

    return nullptr;
}
