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
#include <class_board.h>
#include <class_board_item.h>
#include <pcb_expr_evaluator.h>


/*
 * Rule tokens:
 *     disallow
 *     constraint
 *     condition
 *
 * Disallow types:
 *     track
 *     via
 *     micro_via
 *     blind_via
 *     pad
 *     zone
 *     text
 *     graphic
 *     hole
 *
 * Constraint types:
 *     clearance
 *     annulus_width
 *     track_width
 *     hole
 *
 *
 *     (rule "HV"
 *        (constraint clearance (min 200))
 *        (condition "A.Netclass == 'HV' || B.Netclass == 'HV'")
 *     )
 *
 *     (rule "HV_external"
 *        (constraint clearance (min 400))
 *        (condition "(A.Netclass == 'HV' && (A.onLayer('F.Cu') || A.onLayer('B.Cu'))
 *                     || (B.Netclass == 'HV' && (B.onLayer('F.Cu') || B.onLayer('B.Cu'))")
 *     )
 *
 *     (rule "HV2HV" (constraint clearance (min 200)))
 *     (rule "HV2HV_external" (constraint clearance (min 500)))
 *     (rule "pad2padHV" (constraint clearance (min 500)))
 *
 *     (rule "signal" (constraint clearance (min 20)))
 *     (rule "neckdown" (constraint clearance (min 15)))
 *
 *     (rule "disallowMicrovias" (disallow micro_via))
 *
 *
 */

DRC_RULE* GetRule( const BOARD_ITEM* aItem, const BOARD_ITEM* bItem, int aConstraint )
{
    BOARD* board = aItem->GetBoard();

    if( !board )
        return nullptr;

    for( DRC_RULE* rule : board->GetDesignSettings().m_DRCRules )
    {
        if( ( rule->m_ConstraintFlags & aConstraint ) > 0 )
        {
            if( rule->m_Condition.EvaluateFor( aItem, bItem ) )
                return rule;

            if( bItem && rule->m_Condition.EvaluateFor( bItem, aItem ) )
                return rule;
        }
    }

    return nullptr;
}


DRC_RULE_CONDITION::DRC_RULE_CONDITION()
{
    m_ucode = nullptr;
}


DRC_RULE_CONDITION::~DRC_RULE_CONDITION()
{
    delete m_ucode;
}


bool DRC_RULE_CONDITION::EvaluateFor( const BOARD_ITEM* aItemA, const BOARD_ITEM* aItemB )
{
    BOARD_ITEM* a = const_cast<BOARD_ITEM*>( aItemA );
    BOARD_ITEM* b = aItemB ? const_cast<BOARD_ITEM*>( aItemB ) : DELETED_BOARD_ITEM::GetInstance();

    PCB_EXPR_CONTEXT ctx;
    ctx.SetItems( a, b );

    return m_ucode->Run( &ctx )->AsDouble() != 0.0;
}


bool DRC_RULE_CONDITION::Compile()
{
    PCB_EXPR_COMPILER compiler;

    if (!m_ucode)
        m_ucode = new PCB_EXPR_UCODE;

    PCB_EXPR_CONTEXT preflightContext;

    bool ok = compiler.Compile( (const char*) m_Expression.c_str(), m_ucode, &preflightContext );

    if( ok )
        return true;

    m_compileError = compiler.GetErrorStatus();

    return false;
}


LIBEVAL::ERROR_STATUS DRC_RULE_CONDITION::GetCompilationError()
{
    return m_compileError;
}
