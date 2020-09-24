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
#include <memory>
#include <reporter.h>
#include <class_board.h>
#include <class_track.h>
#include <pcb_expr_evaluator.h>


static void onLayer( LIBEVAL::CONTEXT* aCtx, void *self )
{
    PCB_EXPR_VAR_REF* vref = static_cast<PCB_EXPR_VAR_REF*>( self );
    BOARD_ITEM*       item = vref ? vref->GetObject( aCtx ) : nullptr;

    LIBEVAL::VALUE*   arg = aCtx->Pop();
    LIBEVAL::VALUE*   result = aCtx->AllocValue();

    result->Set( 0.0 );
    aCtx->Push( result );

    if( !item )
        return;

    if( !arg )
    {
        aCtx->ReportError( wxString::Format( _( "Missing argument to '%s'" ),
                                             wxT( "onLayer()" ) ) );
        return;
    }

    wxString     layerName = arg->AsString();
    wxPGChoices& layerMap = ENUM_MAP<PCB_LAYER_ID>::Instance().Choices();
    bool         anyMatch = false;

    for( unsigned ii = 0; ii < layerMap.GetCount(); ++ii )
    {
        wxPGChoiceEntry& entry = layerMap[ii];

        if( entry.GetText().Matches( layerName ) )
        {
            anyMatch = true;

            if( item->IsOnLayer( ToLAYER_ID( entry.GetValue() ) ) )
            {
                result->Set( 1.0 );
                return;
            }
        }
    }

    if( !anyMatch )
        aCtx->ReportError( wxString::Format( _( "Unrecognized layer '%s'" ), layerName ) );
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
    PCB_EXPR_CONTEXT* context = static_cast<PCB_EXPR_CONTEXT*>( aCtx );
    LIBEVAL::VALUE*   arg = aCtx->Pop();
    LIBEVAL::VALUE*   result = aCtx->AllocValue();

    result->Set( 0.0 );
    aCtx->Push( result );

    if( !arg )
    {
        aCtx->ReportError( wxString::Format( _( "Missing argument to '%s'" ),
                                             wxT( "insideCourtyard()" ) ) );
        return;
    }

    PCB_EXPR_VAR_REF* vref = static_cast<PCB_EXPR_VAR_REF*>( self );
    BOARD_ITEM*       item = vref ? vref->GetObject( aCtx ) : nullptr;
    MODULE*           footprint = nullptr;

    if( !item )
        return;

    if( arg->AsString() == "A" )
    {
        footprint = dynamic_cast<MODULE*>( context->GetItem( 0 ) );
    }
    else if( arg->AsString() == "B" )
    {
        footprint = dynamic_cast<MODULE*>( context->GetItem( 1 ) );
    }
    else
    {
        for( MODULE* candidate : item->GetBoard()->Modules() )
        {
            if( candidate->GetReference().Matches( arg->AsString() ) )
            {
                footprint = candidate;
                break;
            }
        }
    }

    if( footprint )
    {
        SHAPE_POLY_SET footprintCourtyard;

        if( footprint->IsFlipped() )
            footprintCourtyard = footprint->GetPolyCourtyardBack();
        else
            footprintCourtyard = footprint->GetPolyCourtyardFront();

        SHAPE_POLY_SET testPoly;

        item->TransformShapeWithClearanceToPolygon( testPoly, context->GetLayer(), 0 );
        testPoly.BooleanIntersection( footprintCourtyard, SHAPE_POLY_SET::PM_FAST );

        if( testPoly.OutlineCount() )
            result->Set( 1.0 );
    }
}


static void insideArea( LIBEVAL::CONTEXT* aCtx, void* self )
{
    PCB_EXPR_CONTEXT* context = static_cast<PCB_EXPR_CONTEXT*>( aCtx );
    LIBEVAL::VALUE*   arg = aCtx->Pop();
    LIBEVAL::VALUE*   result = aCtx->AllocValue();

    result->Set( 0.0 );
    aCtx->Push( result );

    if( !arg )
    {
        aCtx->ReportError( wxString::Format( _( "Missing argument to '%s'" ),
                                             wxT( "insideArea()" ) ) );
        return;
    }

    PCB_EXPR_VAR_REF* vref = static_cast<PCB_EXPR_VAR_REF*>( self );
    BOARD_ITEM*       item = vref ? vref->GetObject( aCtx ) : nullptr;
    ZONE_CONTAINER*   zone = nullptr;

    if( !item )
        return;

    if( arg->AsString() == "A" )
    {
        zone = dynamic_cast<ZONE_CONTAINER*>( context->GetItem( 0 ) );
    }
    else if( arg->AsString() == "B" )
    {
        zone = dynamic_cast<ZONE_CONTAINER*>( context->GetItem( 1 ) );
    }
    else
    {
        for( ZONE_CONTAINER* candidate : item->GetBoard()->Zones() )
        {
            if( candidate->GetZoneName().Matches( arg->AsString() ) )
            {
                zone = candidate;
                break;
            }
        }
    }

    if( zone )
    {
        SHAPE_POLY_SET testPoly;

        item->TransformShapeWithClearanceToPolygon( testPoly, context->GetLayer(), 0 );
        testPoly.BooleanIntersection( *zone->Outline(), SHAPE_POLY_SET::PM_FAST );

        if( testPoly.OutlineCount() )
            result->Set( 1.0 );
    }
}


static void memberOf( LIBEVAL::CONTEXT* aCtx, void* self )
{
    LIBEVAL::VALUE* arg = aCtx->Pop();
    LIBEVAL::VALUE* result = aCtx->AllocValue();

    result->Set( 0.0 );
    aCtx->Push( result );

    if( !arg )
    {
        aCtx->ReportError( wxString::Format( _( "Missing argument to '%s'" ),
                                             wxT( "memberOf()" ) ) );
        return;
    }

    PCB_EXPR_VAR_REF* vref = static_cast<PCB_EXPR_VAR_REF*>( self );
    BOARD_ITEM*       item = vref ? vref->GetObject( aCtx ) : nullptr;

    if( !item )
        return;

    BOARD*     board = item->GetBoard();
    PCB_GROUP* group = board->ParentGroup( item );

    if( !group && item->GetParent() && item->GetParent()->Type() == PCB_MODULE_T )
        group = board->ParentGroup( item->GetParent() );

    while( group )
    {
        if( group->GetName().Matches( arg->AsString() ) )
        {
            result->Set( 1.0 );
            return;
        }

        group = board->ParentGroup( group );
    }
}


static void isMicroVia( LIBEVAL::CONTEXT* aCtx, void* self )
{
    PCB_EXPR_VAR_REF* vref = static_cast<PCB_EXPR_VAR_REF*>( self );
    BOARD_ITEM*       item = vref ? vref->GetObject( aCtx ) : nullptr;
    LIBEVAL::VALUE*   result = aCtx->AllocValue();

    result->Set( 0.0 );
    aCtx->Push( result );

    auto via = dyn_cast<VIA*>( item );

    if( via && via->GetViaType() == VIATYPE::MICROVIA )
    {
        result->Set ( 1.0 );
    }
}


static void isBlindBuriedVia( LIBEVAL::CONTEXT* aCtx, void* self )
{
    PCB_EXPR_VAR_REF* vref = static_cast<PCB_EXPR_VAR_REF*>( self );
    BOARD_ITEM*       item = vref ? vref->GetObject( aCtx ) : nullptr;
    LIBEVAL::VALUE*   result = aCtx->AllocValue();

    result->Set( 0.0 );
    aCtx->Push( result );

    auto via = dyn_cast<VIA*>( item );

    if( via && via->GetViaType() == VIATYPE::BLIND_BURIED )
    {
        result->Set ( 1.0 );
    }

}


PCB_EXPR_BUILTIN_FUNCTIONS::PCB_EXPR_BUILTIN_FUNCTIONS()
{
    auto registerFunc = [&]( const wxString& funcSignature,  LIBEVAL::FUNC_CALL_REF funcPtr )
                        {
                            wxString funcName = funcSignature.BeforeFirst( '(' );
                            m_funcs[ std::string( funcName.Lower() ) ] = std::move( funcPtr );
                            m_funcSigs.Add( funcSignature );
                        };

    registerFunc( "onLayer('x')", onLayer );
    registerFunc( "isPlated()", isPlated );
    registerFunc( "insideCourtyard('x')", insideCourtyard );
    registerFunc( "insideArea('x')", insideArea );
    registerFunc( "memberOf('x')", memberOf );
    registerFunc( "isMicroVia()", isMicroVia );
    registerFunc( "isBlindBuriedVia()", isBlindBuriedVia );
}


BOARD_ITEM* PCB_EXPR_VAR_REF::GetObject( LIBEVAL::CONTEXT* aCtx ) const
{
    wxASSERT( dynamic_cast<PCB_EXPR_CONTEXT*>( aCtx ) );

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
                str = item->Get<wxString>( it->second );
            }
            else
            {
                const wxAny& any = item->Get( it->second );
                any.GetAs<wxString>( &str );
            }

            return LIBEVAL::VALUE( str );
        }
    }
}


LIBEVAL::FUNC_CALL_REF PCB_EXPR_UCODE::CreateFuncCall( const wxString& aName )
{
    PCB_EXPR_BUILTIN_FUNCTIONS& registry = PCB_EXPR_BUILTIN_FUNCTIONS::Instance();

    return registry.Get( aName.Lower() );
}


std::unique_ptr<LIBEVAL::VAR_REF> PCB_EXPR_UCODE::CreateVarRef( const wxString& aVar, const wxString& aField )
{
    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
    std::unique_ptr<PCB_EXPR_VAR_REF> vref;;

    if( aVar == "A" )
    {
        vref.reset( new PCB_EXPR_VAR_REF( 0 ) );
    }
    else if( aVar == "B" )
    {
        vref.reset( new PCB_EXPR_VAR_REF( 1 ) );
    }
    else
    {
        return nullptr;
    }

    if( aField.length() == 0 ) // return reference to base object
    {
        return std::move( vref );
    }

    wxString field( aField );
    field.Replace( "_",  " " );

    for( const PROPERTY_MANAGER::CLASS_INFO& cls : propMgr.GetAllClasses() )
    {
        if( propMgr.IsOfType( cls.type, TYPE_HASH( BOARD_ITEM ) ) )
        {
            PROPERTY_BASE* prop = propMgr.GetProperty( cls.type, field );

            if( prop )
            {
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
                    wxFAIL_MSG( "PCB_EXPR_UCODE::createVarRef: Unknown property type." );
                }
            }
        }
    }

    if( vref->GetType() == LIBEVAL::VT_UNDEFINED )
        vref->SetType( LIBEVAL::VT_PARSE_ERROR );

    return std::move( vref );
}


class PCB_UNIT_RESOLVER : public LIBEVAL::UNIT_RESOLVER
{
public:
    virtual ~PCB_UNIT_RESOLVER()
    {
    }

    virtual const std::vector<wxString>& GetSupportedUnits() const override
    {
        static const std::vector<wxString> pcbUnits = { "mil", "mm", "in" };

        return pcbUnits;
    }

    virtual double Convert( const wxString& aString, int unitId ) const override
    {
        double v = wxAtof( aString );

        switch( unitId )
        {
        case 0:  return DoubleValueFromString( EDA_UNITS::INCHES, aString, true );
        case 1:  return DoubleValueFromString( EDA_UNITS::MILLIMETRES, aString );
        case 2:  return DoubleValueFromString( EDA_UNITS::INCHES, aString, false );
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
    PCB_EXPR_CONTEXT preflightContext( F_Cu );

    m_compiler.Compile( aExpr.ToUTF8().data(), &ucode, &preflightContext );

    PCB_EXPR_CONTEXT evaluationContext( F_Cu );
    LIBEVAL::VALUE*  result = ucode.Run( &evaluationContext );

    if( result->GetType() == LIBEVAL::VT_NUMERIC )
        m_result = KiROUND( result->AsDouble() );

    return true;
}

