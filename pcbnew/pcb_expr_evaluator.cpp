/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2022 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <board.h>
#include <board_connected_item.h>
#include <pcb_expr_evaluator.h>
#include <drc/drc_engine.h>

/* --------------------------------------------------------------------------------------------
 * Specialized Expression References
 */

BOARD_ITEM* PCB_EXPR_VAR_REF::GetObject( const LIBEVAL::CONTEXT* aCtx ) const
{
    wxASSERT( dynamic_cast<const PCB_EXPR_CONTEXT*>( aCtx ) );

    const PCB_EXPR_CONTEXT* ctx = static_cast<const PCB_EXPR_CONTEXT*>( aCtx );
    BOARD_ITEM*             item  = ctx->GetItem( m_itemIndex );
    return item;
}


class PCB_LAYER_VALUE : public LIBEVAL::VALUE
{
public:
    PCB_LAYER_VALUE( PCB_LAYER_ID aLayer ) :
        LIBEVAL::VALUE( LayerName( aLayer ) ),
        m_layer( aLayer )
    {};

    virtual bool EqualTo( LIBEVAL::CONTEXT* aCtx, const VALUE* b ) const override
    {
        // For boards with user-defined layer names there will be 2 entries for each layer
        // in the ENUM_MAP: one for the canonical layer name and one for the user layer name.
        // We need to check against both.

        wxPGChoices&                 layerMap = ENUM_MAP<PCB_LAYER_ID>::Instance().Choices();
        const wxString&              layerName = b->AsString();
        BOARD*                       board = static_cast<PCB_EXPR_CONTEXT*>( aCtx )->GetBoard();
        std::unique_lock<std::mutex> cacheLock( board->m_CachesMutex );
        auto                         i = board->m_LayerExpressionCache.find( layerName );
        LSET                         mask;

        if( i == board->m_LayerExpressionCache.end() )
        {
            for( unsigned ii = 0; ii < layerMap.GetCount(); ++ii )
            {
                wxPGChoiceEntry& entry = layerMap[ii];

                if( entry.GetText().Matches( layerName ) )
                    mask.set( ToLAYER_ID( entry.GetValue() ) );
            }

            board->m_LayerExpressionCache[ layerName ] = mask;
        }
        else
        {
            mask = i->second;
        }

        return mask.Contains( m_layer );
    }

protected:
    PCB_LAYER_ID m_layer;
};


LIBEVAL::VALUE* PCB_EXPR_VAR_REF::GetValue( LIBEVAL::CONTEXT* aCtx )
{
    PCB_EXPR_CONTEXT* context = static_cast<PCB_EXPR_CONTEXT*>( aCtx );

    if( m_itemIndex == 2 )
        return new PCB_LAYER_VALUE( context->GetLayer() );

    BOARD_ITEM* item = GetObject( aCtx );

    if( !item )
        return new LIBEVAL::VALUE();

    auto it = m_matchingTypes.find( TYPE_HASH( *item ) );

    if( it == m_matchingTypes.end() )
    {
        // Don't force user to type "A.Type == 'via' && A.Via_Type == 'buried'" when the
        // simpler "A.Via_Type == 'buried'" is perfectly clear.  Instead, return an undefined
        // value when the property doesn't appear on a particular object.

        return new LIBEVAL::VALUE();
    }
    else
    {
        if( m_type == LIBEVAL::VT_NUMERIC )
            return new LIBEVAL::VALUE( (double) item->Get<int>( it->second ) );
        else
        {
            wxString str;

            if( !m_isEnum )
            {
                str = item->Get<wxString>( it->second );
                return new LIBEVAL::VALUE( str );
            }
            else
            {
                const wxAny& any = item->Get( it->second );
                PCB_LAYER_ID layer;

                if( it->second->Name() == wxT( "Layer" ) )
                {
                    if( any.GetAs<PCB_LAYER_ID>( &layer ) )
                        return new PCB_LAYER_VALUE( layer );
                    else if( any.GetAs<wxString>( &str ) )
                        return new PCB_LAYER_VALUE( context->GetBoard()->GetLayerID( str ) );
                }
                else
                {
                    if( any.GetAs<wxString>( &str ) )
                        return new LIBEVAL::VALUE( str );
                }
            }

            return new LIBEVAL::VALUE();
        }
    }
}


LIBEVAL::VALUE* PCB_EXPR_NETCLASS_REF::GetValue( LIBEVAL::CONTEXT* aCtx )
{
    BOARD_CONNECTED_ITEM* item = dynamic_cast<BOARD_CONNECTED_ITEM*>( GetObject( aCtx ) );

    if( !item )
        return new LIBEVAL::VALUE();

    return new LIBEVAL::VALUE( item->GetEffectiveNetClass()->GetName() );
}


LIBEVAL::VALUE* PCB_EXPR_NETNAME_REF::GetValue( LIBEVAL::CONTEXT* aCtx )
{
    BOARD_CONNECTED_ITEM* item = dynamic_cast<BOARD_CONNECTED_ITEM*>( GetObject( aCtx ) );

    if( !item )
        return new LIBEVAL::VALUE();

    return new LIBEVAL::VALUE( item->GetNetname() );
}


LIBEVAL::VALUE* PCB_EXPR_TYPE_REF::GetValue( LIBEVAL::CONTEXT* aCtx )
{
    BOARD_ITEM* item = GetObject( aCtx );

    if( !item )
        return new LIBEVAL::VALUE();

    return new LIBEVAL::VALUE( ENUM_MAP<KICAD_T>::Instance().ToString( item->Type() ) );
}


LIBEVAL::FUNC_CALL_REF PCB_EXPR_UCODE::CreateFuncCall( const wxString& aName )
{
    PCB_EXPR_BUILTIN_FUNCTIONS& registry = PCB_EXPR_BUILTIN_FUNCTIONS::Instance();

    return registry.Get( aName.Lower() );
}


std::unique_ptr<LIBEVAL::VAR_REF> PCB_EXPR_UCODE::CreateVarRef( const wxString& aVar,
                                                                const wxString& aField )
{
    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
    std::unique_ptr<PCB_EXPR_VAR_REF> vref;

    // Check for a couple of very common cases and compile them straight to "object code".

    if( aField.CmpNoCase( wxT( "NetClass" ) ) == 0 )
    {
        if( aVar == wxT( "A" ) )
            return std::make_unique<PCB_EXPR_NETCLASS_REF>( 0 );
        else if( aVar == wxT( "B" ) )
            return std::make_unique<PCB_EXPR_NETCLASS_REF>( 1 );
        else
            return nullptr;
    }
    else if( aField.CmpNoCase( wxT( "NetName" ) ) == 0 )
    {
        if( aVar == wxT( "A" ) )
            return std::make_unique<PCB_EXPR_NETNAME_REF>( 0 );
        else if( aVar == wxT( "B" ) )
            return std::make_unique<PCB_EXPR_NETNAME_REF>( 1 );
        else
            return nullptr;
    }
    else if( aField.CmpNoCase( wxT( "Type" ) ) == 0 )
    {
        if( aVar == wxT( "A" ) )
            return std::make_unique<PCB_EXPR_TYPE_REF>( 0 );
        else if( aVar == wxT( "B" ) )
            return std::make_unique<PCB_EXPR_TYPE_REF>( 1 );
        else
            return nullptr;
    }

    if( aVar == wxT( "A" ) || aVar == wxT( "AB" ) )
        vref = std::make_unique<PCB_EXPR_VAR_REF>( 0 );
    else if( aVar == wxT( "B" ) )
        vref = std::make_unique<PCB_EXPR_VAR_REF>( 1 );
    else if( aVar == wxT( "L" ) )
        vref = std::make_unique<PCB_EXPR_VAR_REF>( 2 );
    else
        return nullptr;

    if( aField.length() == 0 ) // return reference to base object
    {
        return std::move( vref );
    }

    wxString field( aField );
    field.Replace( wxT( "_" ),  wxT( " " ) );

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
                    wxFAIL_MSG( wxT( "PCB_EXPR_UCODE::createVarRef: Unknown property type." ) );
                }
            }
        }
    }

    if( vref->GetType() == LIBEVAL::VT_UNDEFINED )
        vref->SetType( LIBEVAL::VT_PARSE_ERROR );

    return std::move( vref );
}


BOARD* PCB_EXPR_CONTEXT::GetBoard() const
{
    if( m_items[0] )
        return m_items[0]->GetBoard();

    return nullptr;
}


/* --------------------------------------------------------------------------------------------
 * Unit Resolvers
 */

const std::vector<wxString>& PCB_UNIT_RESOLVER::GetSupportedUnits() const
{
    static const std::vector<wxString> pcbUnits = { wxT( "mil" ), wxT( "mm" ), wxT( "in" ) };

    return pcbUnits;
}


wxString PCB_UNIT_RESOLVER::GetSupportedUnitsMessage() const
{
    return _( "must be mm, in, or mil" );
}


double PCB_UNIT_RESOLVER::Convert( const wxString& aString, int unitId ) const
{
    double v = wxAtof( aString );

    switch( unitId )
    {
    case 0: return EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, EDA_UNITS::MILS, aString );
    case 1: return EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, EDA_UNITS::MILLIMETRES, aString );
    case 2: return EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, EDA_UNITS::INCHES, aString );
    default: return v;
    }
};


const std::vector<wxString>& PCB_UNITLESS_RESOLVER::GetSupportedUnits() const
{
    static const std::vector<wxString> emptyUnits;

    return emptyUnits;
}


double PCB_UNITLESS_RESOLVER::Convert( const wxString& aString, int unitId ) const
{
    return wxAtof( aString );
};


PCB_EXPR_COMPILER::PCB_EXPR_COMPILER( LIBEVAL::UNIT_RESOLVER* aUnitResolver )
{
    m_unitResolver.reset( aUnitResolver );
}


/* --------------------------------------------------------------------------------------------
 * PCB Expression Evaluator
 */

PCB_EXPR_EVALUATOR::PCB_EXPR_EVALUATOR( LIBEVAL::UNIT_RESOLVER* aUnitResolver ) :
    m_result( 0 ),
    m_compiler( aUnitResolver ),
    m_ucode(),
    m_errorStatus()
{
}


PCB_EXPR_EVALUATOR::~PCB_EXPR_EVALUATOR()
{
}


bool PCB_EXPR_EVALUATOR::Evaluate( const wxString& aExpr )
{
    PCB_EXPR_UCODE   ucode;
    PCB_EXPR_CONTEXT preflightContext( NULL_CONSTRAINT, F_Cu );

    if( !m_compiler.Compile( aExpr.ToUTF8().data(), &ucode, &preflightContext ) )
        return false;

    PCB_EXPR_CONTEXT evaluationContext( NULL_CONSTRAINT, F_Cu );
    LIBEVAL::VALUE*  result = ucode.Run( &evaluationContext );

    if( result->GetType() == LIBEVAL::VT_NUMERIC )
        m_result = KiROUND( result->AsDouble() );

    return true;
}

