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
#include <class_board.h>
#include <class_board_item.h>

#include <drc/drc_rule.h>
#include <pcb_expr_evaluator.h>


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

        for( const DRC_CONSTRAINT& constraint : rule->m_Constraints )
        {
            if( constraint.m_Type != aConstraint )
                continue;

            if( !rule->m_LayerCondition.test( aLayer ) )
                continue;

            if( rule->m_Condition.EvaluateFor( aItem, bItem, aLayer ) )
            {
                if( aRuleName )
                    *aRuleName = rule->m_Name;

                return &constraint;
            }

            if( bItem && rule->m_Condition.EvaluateFor( bItem, aItem, aLayer ) )
            {
                if( aRuleName )
                    *aRuleName = rule->m_Name;

                return &constraint;
            }
        }
    }

    return nullptr;
}


DRC_RULE::DRC_RULE() :
    m_LayerCondition( LSET::AllLayersMask() )
{
}


DRC_RULE::~DRC_RULE()
{
}


DRC_RULE_CONDITION::DRC_RULE_CONDITION()
{
    m_ucode = nullptr;
}


DRC_RULE_CONDITION::~DRC_RULE_CONDITION()
{
    delete m_ucode;
}


bool DRC_RULE_CONDITION::EvaluateFor( const BOARD_ITEM* aItemA, const BOARD_ITEM* aItemB,
                                      PCB_LAYER_ID aLayer )
{
    // An unconditional rule is always true
    if( m_Expression.IsEmpty() )
        return true;

    // A rule which failed to compile is always false
    if( !m_ucode )
        return false;

    BOARD_ITEM* a = const_cast<BOARD_ITEM*>( aItemA );
    BOARD_ITEM* b = aItemB ? const_cast<BOARD_ITEM*>( aItemB ) : DELETED_BOARD_ITEM::GetInstance();

    PCB_EXPR_CONTEXT ctx( aLayer );
    ctx.SetItems( a, b );

    return m_ucode->Run( &ctx )->AsDouble() != 0.0;
}


bool DRC_RULE_CONDITION::Compile( REPORTER* aReporter, int aSourceLine, int aSourceOffset )
{
    PCB_EXPR_COMPILER compiler;

    if (!m_ucode)
        m_ucode = new PCB_EXPR_UCODE;

    PCB_EXPR_CONTEXT preflightContext( F_Cu );

    bool ok = compiler.Compile( m_Expression.ToUTF8().data(), m_ucode, &preflightContext );
    return ok;
}


