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
#include <mutex>
#include <board.h>
#include <lset.h>
#include <board_connected_item.h>
#include <pcbexpr_evaluator.h>
#include <drc/drc_engine.h>

/* --------------------------------------------------------------------------------------------
 * Specialized Expression References
 */

BOARD_ITEM* PCBEXPR_VAR_REF::GetObject( const LIBEVAL::CONTEXT* aCtx ) const
{
    wxASSERT( dynamic_cast<const PCBEXPR_CONTEXT*>( aCtx ) );

    const PCBEXPR_CONTEXT* ctx = static_cast<const PCBEXPR_CONTEXT*>( aCtx );
    BOARD_ITEM*            item  = ctx->GetItem( m_itemIndex );
    return item;
}


class PCBEXPR_LAYER_VALUE : public LIBEVAL::VALUE
{
public:
    PCBEXPR_LAYER_VALUE( PCB_LAYER_ID aLayer ) :
        LIBEVAL::VALUE( LayerName( aLayer ) ),
        m_layer( aLayer )
    {};

    virtual bool EqualTo( LIBEVAL::CONTEXT* aCtx, const VALUE* b ) const override
    {
        // For boards with user-defined layer names there will be 2 entries for each layer
        // in the ENUM_MAP: one for the canonical layer name and one for the user layer name.
        // We need to check against both.

        wxPGChoices&    layerMap = ENUM_MAP<PCB_LAYER_ID>::Instance().Choices();
        const wxString& layerName = b->AsString();
        BOARD*          board = static_cast<PCBEXPR_CONTEXT*>( aCtx )->GetBoard();

        {
            std::shared_lock<std::shared_mutex> readLock( board->m_CachesMutex );

            auto i = board->m_LayerExpressionCache.find( layerName );

            if( i != board->m_LayerExpressionCache.end() )
                return i->second.Contains( m_layer );
        }

        LSET mask;

        for( unsigned ii = 0; ii < layerMap.GetCount(); ++ii )
        {
            wxPGChoiceEntry& entry = layerMap[ii];

            if( entry.GetText().Matches( layerName ) )
                mask.set( ToLAYER_ID( entry.GetValue() ) );
        }

        {
            std::unique_lock<std::shared_mutex> writeLock( board->m_CachesMutex );
            board->m_LayerExpressionCache[ layerName ] = mask;
        }

        return mask.Contains( m_layer );
    }

protected:
    PCB_LAYER_ID m_layer;
};


class PCBEXPR_PINTYPE_VALUE : public LIBEVAL::VALUE
{
public:
    PCBEXPR_PINTYPE_VALUE( const wxString& aPinTypeName ) :
            LIBEVAL::VALUE( aPinTypeName )
    {};

    bool EqualTo( LIBEVAL::CONTEXT* aCtx, const VALUE* b ) const override
    {
        const wxString& thisStr = AsString();
        const wxString& otherStr = b->AsString();

        if( thisStr.IsSameAs( otherStr, false ) )
            return true;

        // Handle cases where the netlist token is different from the EEschema token
        if( thisStr == wxT( "tri_state" ) )
            return otherStr.IsSameAs( wxT( "Tri-state" ), false );

        if( thisStr == wxT( "power_in" ) )
            return otherStr.IsSameAs( wxT( "Power input" ), false );

        if( thisStr == wxT( "power_out" ) )
            return otherStr.IsSameAs( wxT( "Power output" ), false );

        if( thisStr == wxT( "no_connect" ) )
            return otherStr.IsSameAs( wxT( "Unconnected" ), false );

        return false;
    }
};


class PCBEXPR_NETCLASS_VALUE : public LIBEVAL::VALUE
{
public:
    PCBEXPR_NETCLASS_VALUE( BOARD_CONNECTED_ITEM* aItem ) :
            LIBEVAL::VALUE( wxEmptyString ),
            m_item( aItem )
    {};

    const wxString& AsString() const override
    {
        const_cast<PCBEXPR_NETCLASS_VALUE*>( this )->Set(
                m_item->GetEffectiveNetClass()->GetVariableSubstitutionName() );
        return LIBEVAL::VALUE::AsString();
    }

    bool EqualTo( LIBEVAL::CONTEXT* aCtx, const VALUE* b ) const override
    {
        if( const PCBEXPR_NETCLASS_VALUE* bValue = dynamic_cast<const PCBEXPR_NETCLASS_VALUE*>( b ) )
            return *( m_item->GetEffectiveNetClass() )
                   == *( bValue->m_item->GetEffectiveNetClass() );
        else
            return LIBEVAL::VALUE::EqualTo( aCtx, b );
    }

    bool NotEqualTo( LIBEVAL::CONTEXT* aCtx, const LIBEVAL::VALUE* b ) const override
    {
        if( const PCBEXPR_NETCLASS_VALUE* bValue = dynamic_cast<const PCBEXPR_NETCLASS_VALUE*>( b ) )
            return m_item->GetEffectiveNetClass() != bValue->m_item->GetEffectiveNetClass();
        else
            return LIBEVAL::VALUE::NotEqualTo( aCtx, b );
    }

protected:
    BOARD_CONNECTED_ITEM* m_item;
};


class PCBEXPR_NET_VALUE : public LIBEVAL::VALUE
{
public:
    PCBEXPR_NET_VALUE( BOARD_CONNECTED_ITEM* aItem ) :
            LIBEVAL::VALUE( wxEmptyString ),
            m_item( aItem )
    {};

    const wxString& AsString() const override
    {
        const_cast<PCBEXPR_NET_VALUE*>( this )->Set( m_item->GetNetname() );
        return LIBEVAL::VALUE::AsString();
    }

    bool EqualTo( LIBEVAL::CONTEXT* aCtx, const VALUE* b ) const override
    {
        if( const PCBEXPR_NET_VALUE* bValue = dynamic_cast<const PCBEXPR_NET_VALUE*>( b ) )
            return m_item->GetNetCode() == bValue->m_item->GetNetCode();
        else
            return LIBEVAL::VALUE::EqualTo( aCtx, b );
    }

    bool NotEqualTo( LIBEVAL::CONTEXT* aCtx, const LIBEVAL::VALUE* b ) const override
    {
        if( const PCBEXPR_NET_VALUE* bValue = dynamic_cast<const PCBEXPR_NET_VALUE*>( b ) )
            return m_item->GetNetCode() != bValue->m_item->GetNetCode();
        else
            return LIBEVAL::VALUE::NotEqualTo( aCtx, b );
    }

protected:
    BOARD_CONNECTED_ITEM* m_item;
};


LIBEVAL::VALUE* PCBEXPR_VAR_REF::GetValue( LIBEVAL::CONTEXT* aCtx )
{
    PCBEXPR_CONTEXT* context = static_cast<PCBEXPR_CONTEXT*>( aCtx );

    if( m_itemIndex == 2 )
        return new PCBEXPR_LAYER_VALUE( context->GetLayer() );

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
        {
            return new LIBEVAL::VALUE( (double) item->Get<int>( it->second ) );
        }
        else
        {
            wxString str;

            if( !m_isEnum )
            {
                str = item->Get<wxString>( it->second );

                if( it->second->Name() == wxT( "Pin Type" ) )
                    return new PCBEXPR_PINTYPE_VALUE( str );
                else
                    return new LIBEVAL::VALUE( str );
            }
            else
            {
                const wxAny& any = item->Get( it->second );
                PCB_LAYER_ID layer;

                if( it->second->Name() == wxT( "Layer" )
                        || it->second->Name() == wxT( "Layer Top" )
                        || it->second->Name() == wxT( "Layer Bottom" ) )
                {
                    if( any.GetAs<PCB_LAYER_ID>( &layer ) )
                        return new PCBEXPR_LAYER_VALUE( layer );
                    else if( any.GetAs<wxString>( &str ) )
                        return new PCBEXPR_LAYER_VALUE( context->GetBoard()->GetLayerID( str ) );
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


LIBEVAL::VALUE* PCBEXPR_NETCLASS_REF::GetValue( LIBEVAL::CONTEXT* aCtx )
{
    BOARD_CONNECTED_ITEM* item = dynamic_cast<BOARD_CONNECTED_ITEM*>( GetObject( aCtx ) );

    if( !item )
        return new LIBEVAL::VALUE();

    return new PCBEXPR_NETCLASS_VALUE( item );
}


LIBEVAL::VALUE* PCBEXPR_NETNAME_REF::GetValue( LIBEVAL::CONTEXT* aCtx )
{
    BOARD_CONNECTED_ITEM* item = dynamic_cast<BOARD_CONNECTED_ITEM*>( GetObject( aCtx ) );

    if( !item )
        return new LIBEVAL::VALUE();

    return new PCBEXPR_NET_VALUE( item );
}


LIBEVAL::VALUE* PCBEXPR_TYPE_REF::GetValue( LIBEVAL::CONTEXT* aCtx )
{
    BOARD_ITEM* item = GetObject( aCtx );

    if( !item )
        return new LIBEVAL::VALUE();

    return new LIBEVAL::VALUE( ENUM_MAP<KICAD_T>::Instance().ToString( item->Type() ) );
}


LIBEVAL::FUNC_CALL_REF PCBEXPR_UCODE::CreateFuncCall( const wxString& aName )
{
    PCBEXPR_BUILTIN_FUNCTIONS& registry = PCBEXPR_BUILTIN_FUNCTIONS::Instance();

    return registry.Get( aName.Lower() );
}


std::unique_ptr<LIBEVAL::VAR_REF> PCBEXPR_UCODE::CreateVarRef( const wxString& aVar,
                                                                const wxString& aField )
{
    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
    std::unique_ptr<PCBEXPR_VAR_REF> vref;

    // Check for a couple of very common cases and compile them straight to "object code".

    if( aField.CmpNoCase( wxT( "NetClass" ) ) == 0 )
    {
        if( aVar == wxT( "A" ) )
            return std::make_unique<PCBEXPR_NETCLASS_REF>( 0 );
        else if( aVar == wxT( "B" ) )
            return std::make_unique<PCBEXPR_NETCLASS_REF>( 1 );
        else
            return nullptr;
    }
    else if( aField.CmpNoCase( wxT( "NetName" ) ) == 0 )
    {
        if( aVar == wxT( "A" ) )
            return std::make_unique<PCBEXPR_NETNAME_REF>( 0 );
        else if( aVar == wxT( "B" ) )
            return std::make_unique<PCBEXPR_NETNAME_REF>( 1 );
        else
            return nullptr;
    }
    else if( aField.CmpNoCase( wxT( "Type" ) ) == 0 )
    {
        if( aVar == wxT( "A" ) )
            return std::make_unique<PCBEXPR_TYPE_REF>( 0 );
        else if( aVar == wxT( "B" ) )
            return std::make_unique<PCBEXPR_TYPE_REF>( 1 );
        else
            return nullptr;
    }

    if( aVar == wxT( "A" ) || aVar == wxT( "AB" ) )
        vref = std::make_unique<PCBEXPR_VAR_REF>( 0 );
    else if( aVar == wxT( "B" ) )
        vref = std::make_unique<PCBEXPR_VAR_REF>( 1 );
    else if( aVar == wxT( "L" ) )
        vref = std::make_unique<PCBEXPR_VAR_REF>( 2 );
    else
        return nullptr;

    if( aField.length() == 0 ) // return reference to base object
    {
        return vref;
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
                else if( prop->TypeHash() == TYPE_HASH( bool ) )
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
                    wxFAIL_MSG( wxT( "PCBEXPR_UCODE::createVarRef: Unknown property type." ) );
                }
            }
        }
    }

    if( vref->GetType() == LIBEVAL::VT_UNDEFINED )
        vref->SetType( LIBEVAL::VT_PARSE_ERROR );

    return vref;
}


BOARD* PCBEXPR_CONTEXT::GetBoard() const
{
    if( m_items[0] )
        return m_items[0]->GetBoard();

    return nullptr;
}


/* --------------------------------------------------------------------------------------------
 * Unit Resolvers
 */

const std::vector<wxString>& PCBEXPR_UNIT_RESOLVER::GetSupportedUnits() const
{
    static const std::vector<wxString> pcbUnits = { wxT( "mil" ), wxT( "mm" ), wxT( "in" ) };

    return pcbUnits;
}


wxString PCBEXPR_UNIT_RESOLVER::GetSupportedUnitsMessage() const
{
    return _( "must be mm, in, or mil" );
}


double PCBEXPR_UNIT_RESOLVER::Convert( const wxString& aString, int unitId ) const
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


const std::vector<wxString>& PCBEXPR_UNITLESS_RESOLVER::GetSupportedUnits() const
{
    static const std::vector<wxString> emptyUnits;

    return emptyUnits;
}


double PCBEXPR_UNITLESS_RESOLVER::Convert( const wxString& aString, int unitId ) const
{
    return wxAtof( aString );
};


PCBEXPR_COMPILER::PCBEXPR_COMPILER( LIBEVAL::UNIT_RESOLVER* aUnitResolver )
{
    m_unitResolver.reset( aUnitResolver );
}


/* --------------------------------------------------------------------------------------------
 * PCB Expression Evaluator
 */

PCBEXPR_EVALUATOR::PCBEXPR_EVALUATOR( LIBEVAL::UNIT_RESOLVER* aUnitResolver ) :
    m_result( 0 ),
    m_compiler( aUnitResolver ),
    m_ucode(),
    m_errorStatus()
{
}


PCBEXPR_EVALUATOR::~PCBEXPR_EVALUATOR()
{
}


bool PCBEXPR_EVALUATOR::Evaluate( const wxString& aExpr )
{
    PCBEXPR_UCODE    ucode;
    PCBEXPR_CONTEXT  preflightContext( NULL_CONSTRAINT, F_Cu );

    if( !m_compiler.Compile( aExpr.ToUTF8().data(), &ucode, &preflightContext ) )
        return false;

    PCBEXPR_CONTEXT  evaluationContext( NULL_CONSTRAINT, F_Cu );
    LIBEVAL::VALUE*  result = ucode.Run( &evaluationContext );

    if( result->GetType() == LIBEVAL::VT_NUMERIC )
        m_result = KiROUND( result->AsDouble() );

    return true;
}

