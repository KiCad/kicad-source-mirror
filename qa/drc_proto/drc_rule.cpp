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
#include <board_design_settings.h>
#include <class_board.h>
#include <class_board_item.h>


#include <drc_proto/drc_rule.h>
#include <pcb_expr_evaluator.h>


test::DRC_RULE::DRC_RULE()
{

}

test::DRC_RULE::~DRC_RULE()
{

}

test::DRC_RULE_CONDITION::DRC_RULE_CONDITION()
{
    m_ucode = nullptr;
}

test::DRC_RULE_CONDITION::~DRC_RULE_CONDITION()
{
    if( m_ucode )
        delete m_ucode;
}

bool test::DRC_RULE_CONDITION::EvaluateFor( BOARD_ITEM* aItemA, BOARD_ITEM* aItemB )
{
    m_ucode->SetItems( aItemA, aItemB );

// fixme: handle error conditions
    return m_ucode->Run()->AsDouble() != 0.0;
}

bool test::DRC_RULE_CONDITION::Compile()
{
    PCB_EXPR_COMPILER compiler;
    if (!m_ucode)
        m_ucode = new PCB_EXPR_UCODE;
    
    bool ok = compiler.Compile( (const char*) m_Expression.c_str(), m_ucode );

    if( ok )
        return true;
    
    m_compileError = compiler.GetErrorStatus();

    printf("Fail: %s", m_compileError.Format().c_str() );

    return false;
}

LIBEVAL::ERROR_STATUS test::DRC_RULE_CONDITION::GetCompilationError()
{
    return m_compileError;
}
