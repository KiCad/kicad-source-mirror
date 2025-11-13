/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


#include <board.h>
#include <board_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_rule_condition.h>


DRC_RULE::DRC_RULE() :
        m_Unary( false ),
        m_ImplicitItemId( 0 ),
        m_LayerCondition( LSET::AllLayersMask() ),
        m_Condition( nullptr ),
        m_Severity( RPT_SEVERITY_UNDEFINED ),
        m_implicitSource( DRC_IMPLICIT_SOURCE::NONE )
{
}


DRC_RULE::DRC_RULE( const wxString& aName ) :
        m_Unary( false ),
        m_ImplicitItemId( 0 ),
        m_Name( aName ),
        m_LayerCondition( LSET::AllLayersMask() ),
        m_Condition( nullptr ),
        m_Severity( RPT_SEVERITY_UNDEFINED ),
        m_implicitSource( DRC_IMPLICIT_SOURCE::NONE )
{
}


DRC_RULE::~DRC_RULE()
{
    delete m_Condition;
}


void DRC_RULE::AddConstraint( DRC_CONSTRAINT& aConstraint )
{
    aConstraint.SetParentRule( this );
    m_Constraints.push_back( aConstraint );
}


std::optional<DRC_CONSTRAINT> DRC_RULE::FindConstraint( DRC_CONSTRAINT_T aType )
{
    for( DRC_CONSTRAINT& c : m_Constraints)
    {
        if( c.m_Type == aType )
            return c;
    }

    return std::optional<DRC_CONSTRAINT>();
}