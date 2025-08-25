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
#include <vector>


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

    PCBEXPR_CONTEXT ctx( aConstraint, aLayer );

    if( aReporter )
    {
        ctx.SetErrorCallback(
                [&]( const wxString& aMessage, int aOffset )
                {
                    aReporter->Report( _( "ERROR:" ) + wxS( " " ) + aMessage );
                } );
    }

    BOARD_ITEM* a = const_cast<BOARD_ITEM*>( aItemA );
    BOARD_ITEM* b = const_cast<BOARD_ITEM*>( aItemB );

    ctx.SetItems( a, b );

    if( m_ucode->Run( &ctx )->AsDouble() != 0.0 )
    {
        return true;
    }
    else if( aItemB )   // Conditions are commutative
    {
        ctx.SetItems( b, a );

        if( m_ucode->Run( &ctx )->AsDouble() != 0.0 )
            return true;
    }

    return false;
}


bool DRC_RULE_CONDITION::Compile( REPORTER* aReporter, int aSourceLine, int aSourceOffset )
{
    static std::vector<wxString> propNames;

    if( propNames.empty() )
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();

        for( const PROPERTY_MANAGER::CLASS_INFO& cls : propMgr.GetAllClasses() )
        {
            for( PROPERTY_BASE* prop : cls.properties )
            {
                if( prop->IsHiddenFromRulesEditor() )
                    continue;

                wxString name = prop->Name();
                name.Replace( wxT( " " ), wxT( "_" ) );
                propNames.push_back( name.Lower() );
            }
        }

        for( const wxString& sig : PCBEXPR_BUILTIN_FUNCTIONS::Instance().GetSignatures() )
        {
            wxString name = sig.BeforeFirst( '(' );
            propNames.push_back( name.Lower() );
        }
    }

    wxString exprLower = m_expression.Lower();

    auto isIdent = []( wxChar c )
    {
        return ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) ||
                 ( c >= '0' && c <= '9' ) || c == '_' );
    };

    for( const wxString& name : propNames )
    {
        size_t pos = 0;

        while( ( pos = exprLower.find( name, pos ) ) != exprLower.npos )
        {
            size_t end = pos + name.Length();

            bool boundaryStart = ( pos == 0 ) || !isIdent( exprLower[ pos - 1 ] );
            bool boundaryEnd = ( end == exprLower.Length() ) || !isIdent( exprLower[ end ] );

            if( boundaryStart && boundaryEnd )
            {
                bool needPrefix = true;

                if( pos >= 2 && m_expression[ pos - 1 ] == '.'
                        && ( m_expression[ pos - 2 ] == 'A' || m_expression[ pos - 2 ] == 'B' ) )
                {
                    needPrefix = false;
                }

                if( needPrefix )
                {
                    if( aReporter )
                    {
                        wxString msg = wxString::Format(
                                _( "The '%s' property requires an 'A.' or 'B.' prefix; assuming 'A.'" ),
                                m_expression.Mid( pos, name.Length() ) );

                        aReporter->Report( msg, RPT_SEVERITY_WARNING );
                    }

                    m_expression.insert( pos, wxT( "A." ) );
                    exprLower.insert( pos, wxT( "a." ) );
                    pos += name.Length() + 2;
                    continue;
                }
            }

            pos += name.Length();
        }
    }

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


