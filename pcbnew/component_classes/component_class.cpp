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


#include <component_classes/component_class.h>
#include <wx/intl.h>
#include <algorithm>


void COMPONENT_CLASS::AddConstituentClass( COMPONENT_CLASS* componentClass )
{
    m_constituentClasses.push_back( componentClass );
}


const COMPONENT_CLASS* COMPONENT_CLASS::GetConstituentClass( const wxString& className ) const
{
    const auto itr = std::ranges::find_if( m_constituentClasses,
                                           [&className]( const COMPONENT_CLASS* testClass )
                                           {
                                               return testClass->GetName() == className;
                                           } );

    if( itr != m_constituentClasses.end() )
        return *itr;

    return nullptr;
}


bool COMPONENT_CLASS::ContainsClassName( const wxString& className ) const
{
    return GetConstituentClass( className ) != nullptr;
}


wxString COMPONENT_CLASS::GetHumanReadableName() const
{
    if( m_constituentClasses.size() == 0 )
        return wxT( "<None>" );

    if( m_constituentClasses.size() == 1 )
        return m_name;

    wxASSERT( m_constituentClasses.size() >= 2 );

    wxString name;

    if( m_constituentClasses.size() == 2 )
    {
        name.Printf( _( "%s and %s" ), m_constituentClasses[0]->GetName(),
                     m_constituentClasses[1]->GetName() );
    }
    else if( m_constituentClasses.size() == 3 )
    {
        name.Printf( _( "%s, %s and %s" ), m_constituentClasses[0]->GetName(),
                     m_constituentClasses[1]->GetName(), m_constituentClasses[2]->GetName() );
    }
    else if( m_constituentClasses.size() > 3 )
    {
        name.Printf( _( "%s, %s and %d more" ), m_constituentClasses[0]->GetName(),
                     m_constituentClasses[1]->GetName(),
                     static_cast<int>( m_constituentClasses.size() - 2 ) );
    }

    return name;
}


bool COMPONENT_CLASS::IsEmpty() const
{
    return m_constituentClasses.empty();
}


bool COMPONENT_CLASS::operator==( const COMPONENT_CLASS& aComponent ) const
{
    return GetName() == aComponent.GetName();
}


bool COMPONENT_CLASS::operator!=( const COMPONENT_CLASS& aComponent ) const
{
    return !( *this == aComponent );
}