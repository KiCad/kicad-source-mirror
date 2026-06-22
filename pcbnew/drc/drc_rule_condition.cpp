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


#include <board_item.h>
#include <reporter.h>
#include <drc/drc_rule_condition.h>
#include <pcbexpr_evaluator.h>
#include <zone.h>

DRC_RULE_CONDITION::DRC_RULE_CONDITION( const wxString& aExpression ) :
    m_expression( aExpression ),
    m_ucode ( nullptr )
{
}


DRC_RULE_CONDITION::~DRC_RULE_CONDITION()
{
}


bool DRC_RULE_CONDITION::EvaluateFor( const BOARD_ITEM* aItemA, const BOARD_ITEM* aItemB,
                                      int aConstraint, PCB_LAYER_ID aLayer, REPORTER* aReporter )
{
    if( GetExpression().IsEmpty() )
        return true;

    if( !m_ucode )
    {
        if( aReporter )
            aReporter->Report( _( "ERROR in expression." ) );

        return false;
    }

    // The no-reporter path is the DRC hot path, so reuse a per-thread context rather than
    // constructing and tearing one down for each of the millions of condition evaluations.  The
    // reporter path is rare and keeps a throwaway context so its error-callback captures stay
    // self-contained.  A re-entrant evaluation on the same thread (should a predicate ever
    // evaluate a rule itself) must not reset the outer evaluation's in-flight context, so only the
    // outermost call reuses the per-thread context; a nested one falls back to a throwaway.
    thread_local PCBEXPR_CONTEXT     tlsCtx;
    thread_local bool                tlsCtxBusy = false;

    struct TLS_RELEASE
    {
        bool* busy = nullptr;
        ~TLS_RELEASE() { if( busy ) *busy = false; }
    } tlsRelease;

    std::unique_ptr<PCBEXPR_CONTEXT> ownedCtx;
    PCBEXPR_CONTEXT*                 ctx;

    if( aReporter || tlsCtxBusy )
    {
        ownedCtx = std::make_unique<PCBEXPR_CONTEXT>( aConstraint, aLayer );
        ctx = ownedCtx.get();

        if( aReporter )
        {
            ctx->SetErrorCallback(
                    [&]( const wxString& aMessage, int aOffset )
                    {
                        aReporter->Report( _( "ERROR:" ) + wxS( " " ) + aMessage );
                    } );
        }
    }
    else
    {
        tlsCtxBusy = true;
        tlsRelease.busy = &tlsCtxBusy;
        tlsCtx.Reset();
        tlsCtx.SetConstraint( aConstraint );
        tlsCtx.SetLayer( aLayer );
        ctx = &tlsCtx;
    }

    BOARD_ITEM* a = const_cast<BOARD_ITEM*>( aItemA );
    BOARD_ITEM* b = const_cast<BOARD_ITEM*>( aItemB );

    // Treat teardrop areas as tracks for DRC rule matching
    if( a && a->Type() == PCB_ZONE_T && static_cast<ZONE*>( a )->IsTeardropArea() )
        ctx->SetTypeOverride( a, PCB_TRACE_T );

    if( b && b->Type() == PCB_ZONE_T && static_cast<ZONE*>( b )->IsTeardropArea() )
        ctx->SetTypeOverride( b, PCB_TRACE_T );

    ctx->SetItems( a, b );

    if( m_ucode->Run( ctx )->AsDouble() != 0.0 )
    {
        return true;
    }
    else if( aItemB )   // Conditions are commutative
    {
        ctx->SetItems( b, a );

        if( m_ucode->Run( ctx )->AsDouble() != 0.0 )
            return true;
    }

    return false;
}


bool DRC_RULE_CONDITION::Compile( REPORTER* aReporter, int aSourceLine, int aSourceOffset )
{
    PCBEXPR_COMPILER compiler( new PCBEXPR_UNIT_RESOLVER() );

    if( aReporter )
    {
        compiler.SetErrorCallback(
                [&]( const wxString& aMessage, int aOffset )
                {
                    wxString rest;
                    wxString first = aMessage.BeforeFirst( '|', &rest );
                    wxString msg = wxString::Format( _( "ERROR: <a href='%d:%d'>%s</a>%s" ),
                                                     aSourceLine,
                                                     aSourceOffset + aOffset,
                                                     first,
                                                     rest );

                    aReporter->Report( msg, RPT_SEVERITY_ERROR );
                } );
    }

    m_ucode = std::make_unique<PCBEXPR_UCODE>();

    PCBEXPR_CONTEXT preflightContext( 0, F_Cu );

    bool ok = compiler.Compile( GetExpression().ToUTF8().data(), m_ucode.get(), &preflightContext );
    return ok;
}


bool DRC_RULE_CONDITION::HasGeometryDependentFunctions() const
{
    return m_ucode && m_ucode->HasGeometryDependentFunctions();
}


