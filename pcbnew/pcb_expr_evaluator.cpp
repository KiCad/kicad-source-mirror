/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2020 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <cstdio>
#include <boost/algorithm/string/case_conv.hpp>
#include <memory>
#include "class_board.h"
#include "pcb_expr_evaluator.h"



static void onLayer( LIBEVAL::CONTEXT* aCtx, void *self )
{
    LIBEVAL::VALUE* arg = aCtx->Pop();
    LIBEVAL::VALUE* result = aCtx->AllocValue();

    result->Set( 0.0 );
    aCtx->Push( result );

    if( !arg )
    {
        aCtx->ReportError( wxString::Format( _( "Missing argument to '%s'" ),
                                             wxT( "onLayer()" ) ) );
        return;
    }

    wxString     layerName = arg->AsString();
    PCB_LAYER_ID layer = ENUM_MAP<PCB_LAYER_ID>::Instance().ToEnum( layerName );

    if( layer == UNDEFINED_LAYER )
    {
        aCtx->ReportError( wxString::Format( _( "Unrecognized layer '%s' " ), layerName ) );
        return;
    }

    PCB_EXPR_VAR_REF* vref = static_cast<PCB_EXPR_VAR_REF*>( self );
    BOARD_ITEM*       item = vref ? vref->GetObject( aCtx ) : nullptr;

    if( item && item->IsOnLayer( layer ) )
        result->Set( 1.0 );
}


static void isPlated( LIBEVAL::CONTEXT* aCtx, void* self )
{
    LIBEVAL::VALUE* result = aCtx->AllocValue();

    result->Set( 0.0 );
    aCtx->Push( result );

    PCB_EXPR_VAR_REF* vref = static_cast<PCB_EXPR_VAR_REF*>( self );
    BOARD_ITEM*       item = vref ? vref->GetObject( aCtx ) : nullptr;
    D_PAD*            pad = dynamic_cast<D_PAD*>( item );

    if( pad && pad->GetAttribute() == PAD_ATTRIB_STANDARD )
        result->Set( 1.0 );
}


static void insideCourtyard( LIBEVAL::CONTEXT* aCtx, void* self )
{
    LIBEVAL::VALUE* arg = aCtx->Pop();
    LIBEVAL::VALUE* result = aCtx->AllocValue();

    result->Set( 0.0 );
    aCtx->Push( result );

    if( !arg )
    {
        aCtx->ReportError( wxString::Format( _( "Missing argument to '%s'" ),
                                             wxT( "insideCourtyard()" ) ) );
        return;
    }

    wxString          footprintRef = arg->AsString();
    PCB_EXPR_VAR_REF* vref = static_cast<PCB_EXPR_VAR_REF*>( self );
    BOARD_ITEM*       item = vref ? vref->GetObject( aCtx ) : nullptr;

    if( item )
    {
        for( MODULE* footprint : item->GetBoard()->Modules() )
        {
            if( footprint->GetReference() == footprintRef )
            {
                SHAPE_POLY_SET footprintCourtyard;

                if( footprint->IsFlipped() )
                    footprintCourtyard = footprint->GetPolyCourtyardBack();
                else
                    footprintCourtyard = footprint->GetPolyCourtyardFront();

                SHAPE_POLY_SET testPoly;

                item->TransformShapeWithClearanceToPolygon( testPoly, 0 );
                testPoly.BooleanIntersection( footprintCourtyard, SHAPE_POLY_SET::PM_FAST );

                if( testPoly.OutlineCount() )
                    result->Set( 1.0 );

                break;
            }
        }
    }
}


PCB_EXPR_BUILTIN_FUNCTIONS::PCB_EXPR_BUILTIN_FUNCTIONS()
{
    auto registerFunc = [&]( const wxString& funcSignature, FPTR funcPtr )
                        {
                            wxString funcName = funcSignature.BeforeFirst( '(' );
                            m_funcs[ std::string( funcName.Lower() ) ] = std::move( funcPtr );
                            m_funcSigs.Add( funcSignature );
                        };

    registerFunc( "onLayer('x')", onLayer );
    registerFunc( "isPlated()", isPlated );
    registerFunc( "insideCourtyard('x')", insideCourtyard );
}


BOARD_ITEM* PCB_EXPR_VAR_REF::GetObject( LIBEVAL::CONTEXT* aCtx ) const
{
    const PCB_EXPR_CONTEXT* ctx = static_cast<const PCB_EXPR_CONTEXT*>( aCtx );
    BOARD_ITEM*             item  = ctx->GetItem( m_itemIndex );
    return item;
}


LIBEVAL::VALUE PCB_EXPR_VAR_REF::GetValue( LIBEVAL::CONTEXT* aCtx )
{
    BOARD_ITEM* item  = const_cast<BOARD_ITEM*>( GetObject( aCtx ) );
    auto        it = m_matchingTypes.find( TYPE_HASH( *item ) );

    if( it == m_matchingTypes.end() )
    {
        // Don't force user to type "A.Type == 'via' && A.Via_Type == 'buried'" when the
        // simplier "A.Via_Type == 'buried'" is perfectly clear.  Instead, return an undefined
        // value when the property doesn't appear on a particular object.

        return LIBEVAL::VALUE( "UNDEFINED" );
    }
    else
    {
        if( m_type == LIBEVAL::VT_NUMERIC )
            return LIBEVAL::VALUE( (double) item->Get<int>( it->second ) );
        else
        {
            wxString str;

            if( !m_isEnum )
            {
                //printf("item %p Get string '%s'\n", item, (const char*) it->second->Name().c_str() );
                str = item->Get<wxString>( it->second );
            }
            else
            {
                const wxAny& any = item->Get( it->second );
                any.GetAs<wxString>( &str );
                //printf("item %p get enum: '%s'\n", item , (const char*) str.c_str() );
            }

            return LIBEVAL::VALUE( str );
        }
    }
}


LIBEVAL::UCODE::FUNC_PTR PCB_EXPR_UCODE::createFuncCall( LIBEVAL::COMPILER* aCompiler,
                                                         const char* aName )
{
    PCB_EXPR_BUILTIN_FUNCTIONS& registry = PCB_EXPR_BUILTIN_FUNCTIONS::Instance();

    std::string lowerName( aName );
    boost::to_lower( lowerName );

    return registry.Get( lowerName );
}


LIBEVAL::VAR_REF* PCB_EXPR_UCODE::createVarRef( LIBEVAL::COMPILER *aCompiler, const char* aVar,
                                                const char* aField )
{
    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
    PCB_EXPR_VAR_REF* vref = nullptr;

    if( *aVar == 'A' )
    {
        vref = new PCB_EXPR_VAR_REF( 0 );
    }
    else if( *aVar == 'B' )
    {
        vref = new PCB_EXPR_VAR_REF( 1 );
    }
    else
    {
        aCompiler->ReportError( "var" );
        return vref;
    }

    if( strlen( aField ) == 0 ) // return reference to base object
        return vref;

    wxString field = wxString::FromUTF8( aField );
    field.Replace( "_",  " " );

    for( const PROPERTY_MANAGER::CLASS_INFO& cls : propMgr.GetAllClasses() )
    {
        if( propMgr.IsOfType( cls.type, TYPE_HASH( BOARD_ITEM ) ) )
        {
            PROPERTY_BASE* prop = propMgr.GetProperty( cls.type, field );

            if( prop )
            {
                //printf("Field '%s' class %s ptr %p haschoices %d typeid %s\n", field.c_str(), (const char *) cls.name.c_str(), prop, !!prop->HasChoices(), typeid(*prop).name() );
                vref->AddAllowedClass( cls.type, prop );

                if( prop->TypeHash() == TYPE_HASH( int ) )
                {
                    vref->SetType( LIBEVAL::VT_NUMERIC );
                }
                else if( prop->TypeHash() == TYPE_HASH( wxString ) )
                {
                    vref->SetType( LIBEVAL::VT_STRING );
                }
                else if ( prop->HasChoices() )
                {   // it's an enum, we treat it as string
                    vref->SetType( LIBEVAL::VT_STRING );
                    vref->SetIsEnum ( true );
                }
                else
                {
                    (void) 0; // should we do anything here?
                    //msg.Printf("Unrecognized type for property '%s'", field.c_str() );
                    //aCompiler->ReportError( (const char*) msg.c_str() );
                }
            }
        }
    }

    if( vref->GetType() == LIBEVAL::VT_UNDEFINED )
        aCompiler->ReportError( "field" );

    return vref;
}


class PCB_UNIT_RESOLVER : public LIBEVAL::UNIT_RESOLVER
{
public:
    virtual ~PCB_UNIT_RESOLVER()
    {
    }

    virtual const std::vector<std::string>& GetSupportedUnits() const override
    {
        static const std::vector<std::string> pcbUnits = { "mil", "mm", "in" };

        return pcbUnits;
    }

    virtual double Convert( const std::string& aString, int unitId ) const override
    {
        double v = atof( aString.c_str() );

        switch( unitId )
        {
        case 0:  return Mils2iu( v );
        case 1:  return Millimeter2iu( v );
        case 2:  return Mils2iu( v * 1000.0 );
        default: return v;
        }
    };
};


PCB_EXPR_COMPILER::PCB_EXPR_COMPILER()
{
    m_unitResolver = std::make_unique<PCB_UNIT_RESOLVER>();
}


PCB_EXPR_EVALUATOR::PCB_EXPR_EVALUATOR()
{
    m_result = 0;
}

PCB_EXPR_EVALUATOR::~PCB_EXPR_EVALUATOR()
{
}


bool PCB_EXPR_EVALUATOR::Evaluate( const wxString& aExpr )
{
    PCB_EXPR_UCODE   ucode;
    LIBEVAL::CONTEXT preflightContext;

    if( !m_compiler.Compile( aExpr.ToUTF8().data(), &ucode, &preflightContext ) )
    {
        m_errorStatus = m_compiler.GetErrorStatus();
        return false;
    }

// fixme: handle error conditions
    LIBEVAL::CONTEXT ctx;
    LIBEVAL::VALUE*  result = ucode.Run( &ctx );

    if( result->GetType() == LIBEVAL::VT_NUMERIC )
        m_result = KiROUND( result->AsDouble() );

    return true;
}

