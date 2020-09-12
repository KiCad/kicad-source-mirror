/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see change_log.txt for contributors.
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


#include <fctsys.h>
#include <drc/drc_rule.h>
#include <drc/drc_rule_condition.h>
#include <drc/drc_engine.h>


const DRC_CONSTRAINT* GetConstraint( const BOARD_ITEM* aItem, const BOARD_ITEM* bItem,
                                     int aConstraint, PCB_LAYER_ID aLayer, wxString* aRuleName )
{
    BOARD* board = aItem->GetBoard();

    if( !board )
        return nullptr;

    for( DRC_RULE* rule : board->GetDesignSettings().m_DRCRules )
    {
        if( !rule->m_LayerCondition.test( aLayer ) )
            continue;

        const DRC_CONSTRAINT* constraint = nullptr;

        for( const DRC_CONSTRAINT& candidate : rule->m_Constraints )
        {
            if( candidate.m_Type == aConstraint )
            {
                constraint = &candidate;
                break;
            }
        }

        if( constraint )
        {
            if( rule->m_Condition->EvaluateFor( aItem, bItem, aLayer ) )
            {
                if( aRuleName )
                    *aRuleName = rule->m_Name;

                return constraint;
            }

            if( bItem && rule->m_Condition->EvaluateFor( bItem, aItem, aLayer ) )
            {
                if( aRuleName )
                    *aRuleName = rule->m_Name;

                return constraint;
            }
        }
    }

    return nullptr;
}


DRC_RULE::DRC_RULE() :
        m_Unary( false ),
        m_LayerCondition( LSET::AllLayersMask() ),
        m_Condition( nullptr ),
        m_Priority( 0 )
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


