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
#include <class_board_item.h>
#include <reporter.h>
#include <drc/drc_rule_condition.h>
#include <pcb_expr_evaluator.h>


DRC_RULE_CONDITION::DRC_RULE_CONDITION( const wxString& aExpression ) :
    m_expression( aExpression ),
    m_ucode ( nullptr )
{
}


DRC_RULE_CONDITION::~DRC_RULE_CONDITION()
{
}


bool DRC_RULE_CONDITION::EvaluateFor( const BOARD_ITEM* aItemA, const BOARD_ITEM* aItemB,
                                      PCB_LAYER_ID aLayer, REPORTER* aReporter )
{
#define REPORT( s ) { if( aReporter ) { aReporter->Report( s ); } }

    if( GetExpression().IsEmpty() )
    {
        REPORT( _( "Unconditional constraint." ) );

        return true;
    }

    REPORT( _( "Checking rule condition \"" + GetExpression() + "\"." ) );

    if( !m_ucode )
    {
        REPORT( _( "ERROR in expression." ) );

        return false;
    }

    BOARD_ITEM* a = const_cast<BOARD_ITEM*>( aItemA );
    BOARD_ITEM* b = aItemB ? const_cast<BOARD_ITEM*>( aItemB ) : DELETED_BOARD_ITEM::GetInstance();

    PCB_EXPR_CONTEXT ctx( aLayer );
    ctx.SetItems( a, b );
    ctx.SetErrorCallback( [&]( const wxString& aMessage, int aOffset )
                          {
                              REPORT( _( "ERROR: " ) + aMessage );
                          } );

    return m_ucode->Run( &ctx )->AsDouble() != 0.0;

#undef REPORT
}


bool DRC_RULE_CONDITION::Compile( REPORTER* aReporter, int aSourceLine, int aSourceOffset )
{
    auto errorHandler = [&]( const wxString& aMessage, int aOffset )
    {
        wxString rest;
        wxString first = aMessage.BeforeFirst( '|', &rest );
        wxString msg = wxString::Format( _( "ERROR: <a href='%d:%d'>%s</a>%s" ),
                                         aSourceLine,
                                         aSourceOffset + aOffset,
                                         first,
                                         rest );

        aReporter->Report( msg, RPT_SEVERITY_ERROR );
    };

    PCB_EXPR_COMPILER compiler;
    compiler.SetErrorCallback( errorHandler );

    m_ucode = std::make_unique<PCB_EXPR_UCODE>();

    PCB_EXPR_CONTEXT preflightContext( F_Cu );

    bool ok = compiler.Compile( GetExpression().ToUTF8().data(), m_ucode.get(), &preflightContext );
    return ok;
}


