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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "pcbexpr_evaluator.h"

#include <inspectable_impl.h>

#include <cstdio>
#include <memory>
#include <mutex>

#include <board.h>
#include <footprint.h>
#include <gal/color4d.h>
#include <geometry/eda_angle.h>
#include <lset.h>
#include <board_connected_item.h>
#include <drc/drc_engine.h>
#include <component_classes/component_class.h>
#include <string_utils.h>


/* --------------------------------------------------------------------------------------------
 * Specialized Expression References
 */

BOARD_ITEM* PCBEXPR_VAR_REF::GetObject( const LIBEVAL::CONTEXT* aCtx ) const
{
    wxASSERT( dynamic_cast<const PCBEXPR_CONTEXT*>( aCtx ) );

    const PCBEXPR_CONTEXT* ctx = static_cast<const PCBEXPR_CONTEXT*>( aCtx );
    BOARD_ITEM*            item  = ctx->GetItem( m_itemIndex );

    for( PCBEXPR_NAV_STEP step : m_navigation )
    {
        if( !item )
            break;

        switch( step )
        {
        // The direct parent, matching the semantics of the "Parent" string property so that
        // "A.Parent" used as an object refers to the same item as "A.Parent" used as a string.
        case PCBEXPR_NAV_STEP::PARENT: item = item->GetParent(); break;
        }
    }

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

        // Case insensitive
        if( thisStr.IsSameAs( otherStr, false ) )
            return true;

        // Wildcards
        if( thisStr.Matches( otherStr ) )
            return true;

        // Handle cases where the netlist token is different from the EEschema token
        wxString altStr;

        if( thisStr == wxT( "tri_state" ) )
            altStr = wxT( "Tri-state" );
        else if( thisStr == wxT( "power_in" ) )
            altStr = wxT( "Power input" );
        else if( thisStr == wxT( "power_out" ) )
            altStr = wxT( "Power output" );
        else if( thisStr == wxT( "no_connect" ) )
            altStr = wxT( "Unconnected" );

        if( !altStr.IsEmpty() )
        {
            // Case insensitive
            if( altStr.IsSameAs( otherStr, false ) )
                return true;

            // Wildcards
            if( altStr.Matches( otherStr ) )
                return true;
        }

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
        const_cast<PCBEXPR_NETCLASS_VALUE*>( this )->Set( m_item->GetEffectiveNetClass()->GetName() );
        return LIBEVAL::VALUE::AsString();
    }

    bool EqualTo( LIBEVAL::CONTEXT* aCtx, const VALUE* b ) const override
    {
        if( const PCBEXPR_NETCLASS_VALUE* bValue = dynamic_cast<const PCBEXPR_NETCLASS_VALUE*>( b ) )
            return *( m_item->GetEffectiveNetClass() ) == *( bValue->m_item->GetEffectiveNetClass() );

        if( b->GetType() == LIBEVAL::VT_STRING )
        {
            // Test constituent net class names. The effective net class name (e.g. CLASS1,CLASS2,OTHER_CLASS) is
            // tested in the fallthrough condition.
            for( const NETCLASS* nc : m_item->GetEffectiveNetClass()->GetConstituentNetclasses() )
            {
                const wxString& ncName = nc->GetName();

                if( b->StringIsWildcard() )
                {
                    if( WildCompareString( b->AsString(), ncName, false ) )
                        return true;
                }
                else
                {
                    if( ncName.IsSameAs( b->AsString(), false ) )
                        return true;
                }
            }
        }

        return LIBEVAL::VALUE::EqualTo( aCtx, b );
    }

    bool NotEqualTo( LIBEVAL::CONTEXT* aCtx, const LIBEVAL::VALUE* b ) const override
    {
        if( const PCBEXPR_NETCLASS_VALUE* bValue = dynamic_cast<const PCBEXPR_NETCLASS_VALUE*>( b ) )
            return *( m_item->GetEffectiveNetClass() ) != *( bValue->m_item->GetEffectiveNetClass() );

        if( b->GetType() == LIBEVAL::VT_STRING )
        {
            // Test constituent net class names
            bool isInConstituents = false;

            for( const NETCLASS* nc : m_item->GetEffectiveNetClass()->GetConstituentNetclasses() )
            {
                const wxString& ncName = nc->GetName();

                if( b->StringIsWildcard() )
                {
                    if( WildCompareString( b->AsString(), ncName, false ) )
                    {
                        isInConstituents = true;
                        break;
                    }
                }
                else
                {
                    if( ncName.IsSameAs( b->AsString(), false ) )
                    {
                        isInConstituents = true;
                        break;
                    }
                }
            }

            // Test effective net class name
            const bool isFullName = LIBEVAL::VALUE::EqualTo( aCtx, b );

            return !isInConstituents && !isFullName;
        }

        return LIBEVAL::VALUE::NotEqualTo( aCtx, b );
    }

protected:
    BOARD_CONNECTED_ITEM* m_item;
};


class PCBEXPR_COMPONENT_CLASS_VALUE : public LIBEVAL::VALUE
{
public:
    PCBEXPR_COMPONENT_CLASS_VALUE( BOARD_ITEM* aItem ) :
            LIBEVAL::VALUE( wxEmptyString ),
            m_item( dynamic_cast<FOOTPRINT*>( aItem ) )
    {};

    const wxString& AsString() const override
    {
        if( !m_item )
            return LIBEVAL::VALUE::AsString();

        if( const COMPONENT_CLASS* compClass = m_item->GetComponentClass() )
            const_cast<PCBEXPR_COMPONENT_CLASS_VALUE*>( this )->Set( compClass->GetName() );

        return LIBEVAL::VALUE::AsString();
    }

    bool EqualTo( LIBEVAL::CONTEXT* aCtx, const VALUE* b ) const override
    {
        if( const PCBEXPR_COMPONENT_CLASS_VALUE* bValue = dynamic_cast<const PCBEXPR_COMPONENT_CLASS_VALUE*>( b ) )
        {
            if( !m_item || !bValue->m_item )
                return LIBEVAL::VALUE::EqualTo( aCtx, b );

            const COMPONENT_CLASS* aClass = m_item->GetComponentClass();
            const COMPONENT_CLASS* bClass = bValue->m_item->GetComponentClass();

            return *aClass == *bClass;
        }

        if( b->GetType() == LIBEVAL::VT_STRING )
        {
            // Test constituent component class names. The effective component class name
            // (e.g. CLASS1,CLASS2,OTHER_CLASS) is tested in the fallthrough condition.
            for( const COMPONENT_CLASS* cc : m_item->GetComponentClass()->GetConstituentClasses() )
            {
                const wxString& ccName = cc->GetName();

                if( b->StringIsWildcard() )
                {
                    if( WildCompareString( b->AsString(), ccName, false ) )
                        return true;
                }
                else
                {
                    if( ccName.IsSameAs( b->AsString(), false ) )
                        return true;
                }
            }
        }

        return LIBEVAL::VALUE::EqualTo( aCtx, b );
    }

    bool NotEqualTo( LIBEVAL::CONTEXT* aCtx, const LIBEVAL::VALUE* b ) const override
    {
        if( const PCBEXPR_COMPONENT_CLASS_VALUE* bValue = dynamic_cast<const PCBEXPR_COMPONENT_CLASS_VALUE*>( b ) )
        {
            if( !m_item || !bValue->m_item )
                return LIBEVAL::VALUE::NotEqualTo( aCtx, b );

            const COMPONENT_CLASS* aClass = m_item->GetComponentClass();
            const COMPONENT_CLASS* bClass = bValue->m_item->GetComponentClass();

            return *aClass != *bClass;
        }

        if( b->GetType() == LIBEVAL::VT_STRING )
        {
            // Test constituent component class names
            bool isInConstituents = false;

            for( const COMPONENT_CLASS* cc : m_item->GetComponentClass()->GetConstituentClasses() )
            {
                const wxString& ccName = cc->GetName();

                if( b->StringIsWildcard() )
                {
                    if( WildCompareString( b->AsString(), ccName, false ) )
                    {
                        isInConstituents = true;
                        break;
                    }
                }
                else
                {
                    if( ccName.IsSameAs( b->AsString(), false ) )
                    {
                        isInConstituents = true;
                        break;
                    }
                }
            }

            // Test effective component class name
            const bool isFullName = LIBEVAL::VALUE::EqualTo( aCtx, b );

            return !isInConstituents && !isFullName;
        }

        return LIBEVAL::VALUE::NotEqualTo( aCtx, b );
    }

protected:
    FOOTPRINT* m_item;
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


KICAD_T PCBEXPR_CONTEXT::GetEffectiveType( const BOARD_ITEM* aItem ) const
{
    auto it = m_typeOverrides.find( aItem );
    return it != m_typeOverrides.end() ? it->second : aItem->Type();
}


PCBEXPR_PROPERTY_KIND PCBEXPR_VAR_REF::ClassifyProperty( const PROPERTY_BASE* aProperty )
{
    const TYPE_ID type = aProperty->TypeHash();

    if( type == TYPE_HASH( int ) )
        return PCBEXPR_PROPERTY_KIND::INT;
    else if( type == TYPE_HASH( std::optional<int> ) )
        return PCBEXPR_PROPERTY_KIND::OPTIONAL_INT;
    else if( type == TYPE_HASH( unsigned ) )
        return PCBEXPR_PROPERTY_KIND::UNSIGNED;
    else if( type == TYPE_HASH( long long int ) )
        return PCBEXPR_PROPERTY_KIND::LONG_LONG;
    else if( type == TYPE_HASH( double ) )
        return PCBEXPR_PROPERTY_KIND::DOUBLE;
    else if( type == TYPE_HASH( std::optional<double> ) )
        return PCBEXPR_PROPERTY_KIND::OPTIONAL_DOUBLE;
    else if( type == TYPE_HASH( bool ) )
        return PCBEXPR_PROPERTY_KIND::BOOL;
    else if( type == TYPE_HASH( wxString ) )
        return PCBEXPR_PROPERTY_KIND::STRING;
    else if( aProperty->HasChoices() )
        return PCBEXPR_PROPERTY_KIND::ENUM;
    else if( type == TYPE_HASH( EDA_ANGLE ) )
        return PCBEXPR_PROPERTY_KIND::ANGLE;
    else if( type == TYPE_HASH( COLOR4D ) )
        return PCBEXPR_PROPERTY_KIND::COLOR;

    return PCBEXPR_PROPERTY_KIND::UNSUPPORTED;
}


LIBEVAL::VAR_TYPE_T PCBEXPR_VAR_REF::ExpressionType( PCBEXPR_PROPERTY_KIND aKind )
{
    switch( aKind )
    {
    case PCBEXPR_PROPERTY_KIND::INT:
    case PCBEXPR_PROPERTY_KIND::OPTIONAL_INT:
    case PCBEXPR_PROPERTY_KIND::UNSIGNED:
    case PCBEXPR_PROPERTY_KIND::LONG_LONG:
    case PCBEXPR_PROPERTY_KIND::BOOL: return LIBEVAL::VT_NUMERIC;

    case PCBEXPR_PROPERTY_KIND::DOUBLE:
    case PCBEXPR_PROPERTY_KIND::OPTIONAL_DOUBLE:
    case PCBEXPR_PROPERTY_KIND::ANGLE: return LIBEVAL::VT_NUMERIC_DOUBLE;

    case PCBEXPR_PROPERTY_KIND::STRING:
    case PCBEXPR_PROPERTY_KIND::ENUM:
    case PCBEXPR_PROPERTY_KIND::COLOR: return LIBEVAL::VT_STRING;

    case PCBEXPR_PROPERTY_KIND::UNSUPPORTED: return LIBEVAL::VT_PARSE_ERROR;
    }

    return LIBEVAL::VT_PARSE_ERROR;
}


LIBEVAL::VALUE* PCBEXPR_VAR_REF::GetValue( LIBEVAL::CONTEXT* aCtx )
{
    PCBEXPR_CONTEXT* context = static_cast<PCBEXPR_CONTEXT*>( aCtx );

    if( m_type == LIBEVAL::VT_NULL )
        return LIBEVAL::VALUE::MakeNullValue();

    if( m_itemIndex == 2 )
        return new PCBEXPR_LAYER_VALUE( context->GetLayer() );

    BOARD_ITEM* item = GetObject( aCtx );

    if( !item )
        return new LIBEVAL::VALUE();

    auto it = m_matchingTypes.find( TYPE_HASH( *item ) );

    if( it == m_matchingTypes.end() )
    {
        // If the property isn't defined on the item itself but is defined on its parent
        // footprint (e.g. Reference, Value), resolve against the parent so that conditions
        // like "A.Reference == 'J1'" match pads and graphics belonging to J1.
        if( FOOTPRINT* parentFp = item->GetParentFootprint() )
        {
            auto parentIt = m_matchingTypes.find( TYPE_HASH( *parentFp ) );

            if( parentIt != m_matchingTypes.end() )
            {
                item = parentFp;
                it = parentIt;
            }
        }
    }

    if( it == m_matchingTypes.end() )
    {
        // Don't force user to type "A.Type == 'via' && A.Via_Type == 'buried'" when the
        // simpler "A.Via_Type == 'buried'" is perfectly clear.  Instead, return an undefined
        // value when the property doesn't appear on a particular object.

        return new LIBEVAL::VALUE();
    }
    else
    {
        switch( it->second.kind )
        {
        case PCBEXPR_PROPERTY_KIND::INT:
            return new LIBEVAL::VALUE( static_cast<double>( item->Get<int>( it->second.property ) ) );

        case PCBEXPR_PROPERTY_KIND::OPTIONAL_INT:
        {
            std::optional<int> val = item->Get<std::optional<int>>( it->second.property );

            if( val.has_value() )
                return new LIBEVAL::VALUE( static_cast<double>( val.value() ) );

            return LIBEVAL::VALUE::MakeNullValue();
        }

        case PCBEXPR_PROPERTY_KIND::UNSIGNED:
            return new LIBEVAL::VALUE( static_cast<double>( item->Get<unsigned>( it->second.property ) ) );

        case PCBEXPR_PROPERTY_KIND::LONG_LONG:
            return new LIBEVAL::VALUE( static_cast<double>( item->Get<long long int>( it->second.property ) ) );

        case PCBEXPR_PROPERTY_KIND::DOUBLE: return new LIBEVAL::VALUE( item->Get<double>( it->second.property ) );

        case PCBEXPR_PROPERTY_KIND::OPTIONAL_DOUBLE:
        {
            std::optional<double> val = item->Get<std::optional<double>>( it->second.property );

            if( val.has_value() )
                return new LIBEVAL::VALUE( val.value() );

            return LIBEVAL::VALUE::MakeNullValue();
        }

        case PCBEXPR_PROPERTY_KIND::BOOL:
            return new LIBEVAL::VALUE( static_cast<double>( item->Get<bool>( it->second.property ) ) );

        case PCBEXPR_PROPERTY_KIND::STRING:
        {
            wxString str = item->Get<wxString>( it->second.property );

            if( it->second.property->Name() == wxT( "Pin Type" ) )
                return new PCBEXPR_PINTYPE_VALUE( str );

            // If it quacks like a duck, it is a duck
            double doubleVal;

            if( EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, str, doubleVal ) )
                return new LIBEVAL::VALUE( doubleVal );

            return new LIBEVAL::VALUE( str );
        }

        case PCBEXPR_PROPERTY_KIND::ENUM:
        {
            wxString str;

            if( it->second.property->Name() == wxT( "Layer" ) || it->second.property->Name() == wxT( "Layer Top" )
                || it->second.property->Name() == wxT( "Layer Bottom" ) )
            {
                const wxAny& any = item->Get( it->second.property );
                PCB_LAYER_ID layer;

                if( any.GetAs<PCB_LAYER_ID>( &layer ) )
                    return new PCBEXPR_LAYER_VALUE( layer );
                else if( any.GetAs<wxString>( &str ) )
                    return new PCBEXPR_LAYER_VALUE( context->GetBoard()->GetLayerID( str ) );
            }
            else
            {
                const wxAny& any = item->Get( it->second.property );

                if( any.GetAs<wxString>( &str ) )
                    return new LIBEVAL::VALUE( str );
            }

            return new LIBEVAL::VALUE();
        }

        case PCBEXPR_PROPERTY_KIND::ANGLE:
            return new LIBEVAL::VALUE( item->Get<EDA_ANGLE>( it->second.property ).AsDegrees() );

        case PCBEXPR_PROPERTY_KIND::COLOR:
            return new LIBEVAL::VALUE( item->Get<COLOR4D>( it->second.property ).ToCSSString() );

        case PCBEXPR_PROPERTY_KIND::UNSUPPORTED: return new LIBEVAL::VALUE();
        }
    }

    return new LIBEVAL::VALUE();
}


LIBEVAL::VALUE* PCBEXPR_NETCLASS_REF::GetValue( LIBEVAL::CONTEXT* aCtx )
{
    BOARD_CONNECTED_ITEM* item = dynamic_cast<BOARD_CONNECTED_ITEM*>( GetObject( aCtx ) );

    if( !item )
        return new LIBEVAL::VALUE();

    return new PCBEXPR_NETCLASS_VALUE( item );
}


LIBEVAL::VALUE* PCBEXPR_COMPONENT_CLASS_REF::GetValue( LIBEVAL::CONTEXT* aCtx )
{
    BOARD_ITEM* item = dynamic_cast<BOARD_ITEM*>( GetObject( aCtx ) );

    if( !item )
        return new LIBEVAL::VALUE();

    // Resolve component class via the parent footprint so that conditions like
    // "A.ComponentClass == 'X'" match pads and graphics inside the footprint.
    if( item->Type() != PCB_FOOTPRINT_T )
        item = item->GetParentFootprint();

    if( !item )
        return new LIBEVAL::VALUE();

    return new PCBEXPR_COMPONENT_CLASS_VALUE( item );
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

    PCBEXPR_CONTEXT* ctx = static_cast<PCBEXPR_CONTEXT*>( aCtx );
    KICAD_T          type = ctx->GetEffectiveType( item );

    return new LIBEVAL::VALUE( ENUM_MAP<KICAD_T>::Instance().ToString( type ) );
}


LIBEVAL::FUNC_CALL_REF PCBEXPR_UCODE::CreateFuncCall( const wxString& aName )
{
    wxString nameLower = aName.Lower();

    PCBEXPR_BUILTIN_FUNCTIONS& registry = PCBEXPR_BUILTIN_FUNCTIONS::Instance();

    if( registry.IsGeometryDependent( nameLower ) )
        m_hasGeometryDependentFunctions = true;

    return registry.Get( nameLower );
}


std::unique_ptr<LIBEVAL::VAR_REF> PCBEXPR_UCODE::CreateVarRef( const wxString& aVar,
                                                               const wxString& aField )
{
    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
    std::unique_ptr<PCBEXPR_VAR_REF> vref;

    if( aVar.IsSameAs( wxT( "null" ), false ) )
    {
        vref = std::make_unique<PCBEXPR_VAR_REF>( 0 );
        vref->SetType( LIBEVAL::VT_NULL );
        return vref;
    }

    // The receiver may be a navigation chain such as "A.Parent".  Split off the base variable
    // and translate each remaining segment into a navigation step.  Only "Parent" is supported.

    wxString                      baseVar = aVar;
    std::vector<PCBEXPR_NAV_STEP> navigation;

    if( aVar.Contains( wxT( "." ) ) )
    {
        wxArrayString tokens = wxSplit( aVar, '.' );
        baseVar = tokens.IsEmpty() ? wxString() : tokens[0];

        for( size_t i = 1; i < tokens.GetCount(); ++i )
        {
            if( tokens[i].CmpNoCase( wxT( "Parent" ) ) == 0 )
                navigation.push_back( PCBEXPR_NAV_STEP::PARENT );
            else
                return nullptr;
        }

        // Navigation is only meaningful for the item operands; the layer pseudo-item "L" has
        // no parent.
        if( baseVar != wxT( "A" ) && baseVar != wxT( "AB" ) && baseVar != wxT( "B" ) )
            return nullptr;
    }

    auto withNav =
            [&navigation]( std::unique_ptr<PCBEXPR_VAR_REF> aRef ) -> std::unique_ptr<PCBEXPR_VAR_REF>
            {
                if( aRef && !navigation.empty() )
                    aRef->SetNavigation( navigation );

                return aRef;
            };

    // Check for a couple of very common cases and compile them straight to "object code".

    if( aField.CmpNoCase( wxT( "NetClass" ) ) == 0 )
    {
        if( baseVar == wxT( "A" ) )
            return withNav( std::make_unique<PCBEXPR_NETCLASS_REF>( 0 ) );
        else if( baseVar == wxT( "B" ) )
            return withNav( std::make_unique<PCBEXPR_NETCLASS_REF>( 1 ) );
        else
            return nullptr;
    }
    else if( aField.CmpNoCase( wxT( "ComponentClass" ) ) == 0 )
    {
        if( baseVar == wxT( "A" ) )
            return withNav( std::make_unique<PCBEXPR_COMPONENT_CLASS_REF>( 0 ) );
        else if( baseVar == wxT( "B" ) )
            return withNav( std::make_unique<PCBEXPR_COMPONENT_CLASS_REF>( 1 ) );
        else
            return nullptr;
    }
    else if( aField.CmpNoCase( wxT( "NetName" ) ) == 0 )
    {
        if( baseVar == wxT( "A" ) )
            return withNav( std::make_unique<PCBEXPR_NETNAME_REF>( 0 ) );
        else if( baseVar == wxT( "B" ) )
            return withNav( std::make_unique<PCBEXPR_NETNAME_REF>( 1 ) );
        else
            return nullptr;
    }
    else if( aField.CmpNoCase( wxT( "Type" ) ) == 0 )
    {
        if( baseVar == wxT( "A" ) )
            return withNav( std::make_unique<PCBEXPR_TYPE_REF>( 0 ) );
        else if( baseVar == wxT( "B" ) )
            return withNav( std::make_unique<PCBEXPR_TYPE_REF>( 1 ) );
        else
            return nullptr;
    }

    if( baseVar == wxT( "A" ) || baseVar == wxT( "AB" ) )
        vref = std::make_unique<PCBEXPR_VAR_REF>( 0 );
    else if( baseVar == wxT( "B" ) )
        vref = std::make_unique<PCBEXPR_VAR_REF>( 1 );
    else if( baseVar == wxT( "L" ) )
        vref = std::make_unique<PCBEXPR_VAR_REF>( 2 );
    else
        return nullptr;

    vref->SetNavigation( navigation );

    if( aField.length() == 0 ) // return reference to base object
        return vref;

    wxString field( aField );
    field.Replace( wxT( "_" ),  wxT( " " ) );

    for( const PROPERTY_MANAGER::CLASS_INFO& cls : propMgr.GetAllClasses() )
    {
        if( propMgr.IsOfType( cls.type, TYPE_HASH( BOARD_ITEM ) ) )
        {
            PROPERTY_BASE* prop = propMgr.GetProperty( cls.type, field );

            if( prop )
            {
                PCBEXPR_PROPERTY_KIND kind = PCBEXPR_VAR_REF::ClassifyProperty( prop );
                LIBEVAL::VAR_TYPE_T   expressionType = PCBEXPR_VAR_REF::ExpressionType( kind );

                if( expressionType == LIBEVAL::VT_PARSE_ERROR
                    || ( vref->GetType() != LIBEVAL::VT_UNDEFINED && vref->GetType() != expressionType ) )
                {
                    vref->SetType( LIBEVAL::VT_PARSE_ERROR );
                    return vref;
                }

                vref->AddAllowedClass( cls.type, prop, kind );
                vref->SetType( expressionType );
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
    static const std::vector<wxString> pcbUnits = { wxT( "mil" ), wxT( "mm" ), wxT( "in" ),
                                                    wxT( "deg" ), wxT( "fs" ), wxT( "ps" ) };


    return pcbUnits;
}


wxString PCBEXPR_UNIT_RESOLVER::GetSupportedUnitsMessage() const
{
    return _( "must be mm, in, mil, deg, fs, or ps" );
}


const std::vector<EDA_UNITS>& PCBEXPR_UNIT_RESOLVER::GetSupportedUnitsTypes() const
{
    static const std::vector<EDA_UNITS> pcbUnits = { EDA_UNITS::MILS,    EDA_UNITS::MM, EDA_UNITS::INCH,
                                                     EDA_UNITS::DEGREES, EDA_UNITS::FS, EDA_UNITS::PS };

    return pcbUnits;
}


double PCBEXPR_UNIT_RESOLVER::Convert( const wxString& aString, int unitId ) const
{
    double v = wxAtof( aString );

    switch( unitId )
    {
    case 0: return EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, EDA_UNITS::MILS, aString );
    case 1: return EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, EDA_UNITS::MM, aString );
    case 2: return EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, EDA_UNITS::INCH, aString );
    case 3: return v;
    case 4: return EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, EDA_UNITS::FS, aString );
    case 5: return EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, EDA_UNITS::PS, aString );
    default: return v;
    }
};


const std::vector<wxString>& PCBEXPR_UNITLESS_RESOLVER::GetSupportedUnits() const
{
    static const std::vector<wxString> emptyUnits;

    return emptyUnits;
}


const std::vector<EDA_UNITS>& PCBEXPR_UNITLESS_RESOLVER::GetSupportedUnitsTypes() const
{
    static const std::vector<EDA_UNITS> emptyUnits;

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
    m_units( EDA_UNITS::MM ),
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
    {
        m_result = KiROUND( result->AsDouble() );
        m_units = result->GetUnits();
    }

    return true;
}
