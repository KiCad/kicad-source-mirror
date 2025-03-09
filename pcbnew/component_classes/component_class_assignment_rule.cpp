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


#include <component_classes/component_class_assignment_rule.h>
#include <footprint.h>


COMPONENT_CLASS_ASSIGNMENT_RULE::COMPONENT_CLASS_ASSIGNMENT_RULE(
        const wxString& aComponentClass, std::shared_ptr<DRC_RULE_CONDITION>&& aCondition ) :
        m_componentClass( aComponentClass ), m_condition( std::move( aCondition ) )
{
}


bool COMPONENT_CLASS_ASSIGNMENT_RULE::Matches( const FOOTPRINT* aFootprint ) const
{
    if( !m_condition )
        return true;

    return m_condition->EvaluateFor( aFootprint, nullptr, 0, aFootprint->GetSide(), nullptr );
}
