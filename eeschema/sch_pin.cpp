/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 CERN
 * Copyright (C) 2019-2024 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <base_units.h>
#include <lib_pin.h>
#include <sch_symbol.h>
#include <sch_pin.h>
#include <schematic.h>
#include <schematic_settings.h>
#include <sch_sheet_path.h>
#include <sch_edit_frame.h>
#include "string_utils.h"

SCH_PIN::SCH_PIN( LIB_PIN* aLibPin, SCH_SYMBOL* aParentSymbol ) :
    SCH_ITEM( aParentSymbol, SCH_PIN_T )
{
    wxASSERT( aParentSymbol );
    m_layer = LAYER_PIN;
    m_alt = wxEmptyString;
    m_number = aLibPin->GetNumber();
    m_libPin = aLibPin;
    SetPosition( aLibPin->GetPosition() );
    m_isDangling = true;
}


/**
 * Create a proxy pin from an alternate pin designation.
 * The LIB_PIN data will be filled in when the pin is resolved (see SCH_SYMBOL::UpdatePins).
 */
SCH_PIN::SCH_PIN( SCH_SYMBOL* aParentSymbol, const wxString& aNumber, const wxString& aAlt ) :
    SCH_ITEM( aParentSymbol, SCH_PIN_T )
{
    wxASSERT( aParentSymbol );
    m_layer = LAYER_PIN;
    m_alt = aAlt;
    m_number = aNumber;
    m_libPin = nullptr;
    m_isDangling = true;
}


SCH_PIN::SCH_PIN( const SCH_PIN& aPin ) :
        SCH_ITEM( aPin )
{
    m_layer = aPin.m_layer;
    m_alt = aPin.m_alt;
    m_number = aPin.m_number;
    m_libPin = aPin.m_libPin;
    m_position = aPin.m_position;
    m_isDangling = aPin.m_isDangling;
}


SCH_PIN& SCH_PIN::operator=( const SCH_PIN& aPin )
{
    SCH_ITEM::operator=( aPin );

    m_alt = aPin.m_alt;
    m_number = aPin.m_number;
    m_libPin = aPin.m_libPin;
    m_position = aPin.m_position;
    m_isDangling = aPin.m_isDangling;

    return *this;
}


bool SCH_PIN::IsVisible() const
{
    wxCHECK( m_libPin, false );

    return m_libPin->IsVisible();
}


wxString SCH_PIN::GetName() const
{
    if( !m_alt.IsEmpty() )
        return m_alt;

    return m_libPin ? m_libPin->GetName() : wxString( "??" );
}


wxString SCH_PIN::GetShownName() const
{
    wxString name = m_libPin ? m_libPin->GetName() : wxString( "??" );

    if( !m_alt.IsEmpty() )
        name = m_alt;

    if( name == wxS( "~" ) )
        return wxEmptyString;
    else
        return name;
}


wxString SCH_PIN::GetShownNumber() const
{
    if( m_number == wxS( "~" ) )
        return wxEmptyString;
    else
        return m_number;
}


ELECTRICAL_PINTYPE SCH_PIN::GetType() const
{
    if( !m_libPin )
        return ELECTRICAL_PINTYPE::PT_NC;

    if( !m_alt.IsEmpty() )
        return m_libPin->GetAlt( m_alt ).m_Type;

    return m_libPin->GetType();
}


GRAPHIC_PINSHAPE SCH_PIN::GetShape() const
{
    if( !m_libPin )
        return GRAPHIC_PINSHAPE::LINE;

    if( !m_alt.IsEmpty() )
        return m_libPin->GetAlt( m_alt ).m_Shape;

    return m_libPin->GetShape();
}


PIN_ORIENTATION SCH_PIN::GetOrientation() const
{
    if( !m_libPin )
        return PIN_ORIENTATION::PIN_RIGHT;

    return m_libPin->GetOrientation();
}


int SCH_PIN::GetLength() const
{
    if( !m_libPin )
        return 0;

    return m_libPin->GetLength();
}


const BOX2I SCH_PIN::ViewBBox() const
{
    return GetBoundingBox( false, true, true );
}


void SCH_PIN::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount     = 4;
    aLayers[0] = LAYER_DANGLING;
    aLayers[1] = LAYER_DEVICE;
    aLayers[2] = LAYER_SELECTION_SHADOWS;
    aLayers[3] = LAYER_OP_CURRENTS;
}


bool SCH_PIN::Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxDat ) const
{
    const SCH_SEARCH_DATA& schSearchData =
            dynamic_cast<const SCH_SEARCH_DATA&>( aSearchData );

    if( !schSearchData.searchAllPins )
        return false;

    return EDA_ITEM::Matches( GetName(), aSearchData )
                || EDA_ITEM::Matches( GetNumber(), aSearchData );
}


bool SCH_PIN::Replace( const EDA_SEARCH_DATA& aSearchData, void* aAuxData )
{
    bool isReplaced = false;

    /* TODO: waiting on a way to override pins in the schematic...
    isReplaced |= EDA_ITEM::Replace( aSearchData, m_name );
    isReplaced |= EDA_ITEM::Replace( aSearchData, m_number );
     */

    return isReplaced;
}


SCH_SYMBOL* SCH_PIN::GetParentSymbol() const
{
    return static_cast<SCH_SYMBOL*>( GetParent() );
}


wxString SCH_PIN::GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const
{
    LIB_PIN::ALT  localStorage;
    LIB_PIN::ALT* alt = nullptr;

    if( !m_alt.IsEmpty() && m_libPin )
    {
        localStorage = m_libPin->GetAlt( m_alt );
        alt = &localStorage;
    }

    wxString itemDesc = m_libPin ? m_libPin->GetItemDescription( aUnitsProvider, alt ) :
                                   wxString( wxS( "Undefined library pin." ) );
    return wxString::Format( "Symbol %s %s",
                             UnescapeString( GetParentSymbol()->GetField( REFERENCE_FIELD )->GetText() ),
                             itemDesc );
}


void SCH_PIN::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString    msg;
    SCH_SYMBOL* symbol = GetParentSymbol();

    aList.emplace_back( _( "Type" ), _( "Pin" ) );

    if( LIB_SYMBOL* libSymbol = symbol->GetLibSymbolRef().get() )
    {
        if( libSymbol->GetUnitCount() )
        {
            msg = m_libPin ? LIB_ITEM::GetUnitDescription( m_libPin->GetUnit() ) :
                             wxString( "Undefined library pin." );
            aList.emplace_back( _( "Unit" ), msg );
        }

        if( libSymbol->HasAlternateBodyStyle() )
        {
            msg = m_libPin ? LIB_ITEM::GetBodyStyleDescription( m_libPin->GetBodyStyle() ) :
                             wxString( "Undefined library pin." );
            aList.emplace_back( _( "Body Style" ), msg );
        }
    }

    aList.emplace_back( _( "Name" ), GetShownName() );
    aList.emplace_back( _( "Number" ), GetShownNumber() );
    aList.emplace_back( _( "Type" ), ElectricalPinTypeGetText( GetType() ) );
    aList.emplace_back( _( "Style" ), PinShapeGetText( GetShape() ) );

    aList.emplace_back( _( "Visible" ), IsVisible() ? _( "Yes" ) : _( "No" ) );

    aList.emplace_back( _( "Length" ), aFrame->MessageTextFromValue( GetLength() ), true );

    aList.emplace_back( _( "Orientation" ), PinOrientationName( GetOrientation() ) );

    SCH_EDIT_FRAME* schframe = dynamic_cast<SCH_EDIT_FRAME*>( aFrame );
    SCH_SHEET_PATH* currentSheet = schframe ? &schframe->GetCurrentSheet() : nullptr;

    // Don't use GetShownText(); we want to see the variable references here
    aList.emplace_back( symbol->GetRef( currentSheet ),
                        UnescapeString( symbol->GetField( VALUE_FIELD )->GetText() ) );

#if defined(DEBUG)
    if( !IsConnectivityDirty() && dynamic_cast<SCH_EDIT_FRAME*>( aFrame ) )
    {
        SCH_CONNECTION* conn = Connection();

        if( conn )
            conn->AppendInfoToMsgPanel( aList );
    }
#endif

}


bool SCH_PIN::IsStacked( const SCH_PIN* aPin ) const
{
    bool isConnectableType_a = GetType() == ELECTRICAL_PINTYPE::PT_PASSIVE
                            || GetType() == ELECTRICAL_PINTYPE::PT_NIC;
    bool isConnectableType_b = aPin->GetType() == ELECTRICAL_PINTYPE::PT_PASSIVE
                            || aPin->GetType() == ELECTRICAL_PINTYPE::PT_NIC;

    return m_parent == aPin->GetParent()
           && GetTransformedPosition() == aPin->GetTransformedPosition()
           && GetName() == aPin->GetName()
           && ( ( GetType() == aPin->GetType() )
                || isConnectableType_a || isConnectableType_b );
}


bool SCH_PIN::IsGlobalPower() const
{
    if( !m_libPin )
        return false;

    return m_libPin->IsGlobalPower();
}

void SCH_PIN::ClearDefaultNetName( const SCH_SHEET_PATH* aPath )
{
    std::lock_guard<std::recursive_mutex> lock( m_netmap_mutex );

    if( aPath )
        m_net_name_map.erase( *aPath );
    else
        m_net_name_map.clear();
}


wxString SCH_PIN::GetDefaultNetName( const SCH_SHEET_PATH& aPath, bool aForceNoConnect )
{
    // Need to check for parent as power symbol to make sure we aren't dealing
    // with legacy global power pins on non-power symbols
    if( IsGlobalPower() )
    {
        if( GetLibPin()->GetParent()->IsPower() )
        {
            return EscapeString( GetParentSymbol()->GetValueFieldText( true, &aPath, false ),
                                 CTX_NETNAME );
        }
        else
        {
            wxString tmp = m_libPin ? m_libPin->GetName() : wxString( "??" );

            return EscapeString( tmp, CTX_NETNAME );
        }
    }

    std::lock_guard<std::recursive_mutex> lock( m_netmap_mutex );

    auto it = m_net_name_map.find( aPath );

    if( it != m_net_name_map.end() )
    {
        if( it->second.second == aForceNoConnect )
            return it->second.first;
    }

    wxString name = "Net-(";
    bool unconnected = false;

    if( aForceNoConnect || GetType() == ELECTRICAL_PINTYPE::PT_NC )
    {
        unconnected = true;
        name = ( "unconnected-(" );
    }

    bool annotated = true;

    std::vector<SCH_PIN*> pins = GetParentSymbol()->GetPins( &aPath );
    bool has_multiple = false;

    for( SCH_PIN* pin : pins )
    {
        if( pin->GetShownName() == GetShownName()
                && pin->GetShownNumber() != GetShownNumber()
                && unconnected == ( pin->GetType() == ELECTRICAL_PINTYPE::PT_NC ) )
        {
            has_multiple = true;
            break;
        }
    }

    wxString libPinShownName = m_libPin ? m_libPin->GetShownName() : wxString( "??" );
    wxString libPinShownNumber = m_libPin ? m_libPin->GetShownNumber() : wxString( "??" );

    // Use timestamp for unannotated symbols
    if( GetParentSymbol()->GetRef( &aPath, false ).Last() == '?' )
    {
        name << GetParentSymbol()->m_Uuid.AsString();

        wxString libPinNumber = m_libPin ? m_libPin->GetNumber() : wxString( "??" );
        name << "-Pad" << libPinNumber << ")";
        annotated = false;
    }
    else if( !libPinShownName.IsEmpty() && ( libPinShownName != libPinShownNumber ) )
    {
        // Pin names might not be unique between different units so we must have the
        // unit token in the reference designator
        name << GetParentSymbol()->GetRef( &aPath, true );
        name << "-" << EscapeString( libPinShownName, CTX_NETNAME );

        if( unconnected || has_multiple )
            name << "-Pad" << EscapeString( libPinShownNumber, CTX_NETNAME );

        name << ")";
    }
    else
    {
        // Pin numbers are unique, so we skip the unit token
        name << GetParentSymbol()->GetRef( &aPath, false );
        name << "-Pad" << EscapeString( libPinShownNumber, CTX_NETNAME ) << ")";
    }

    if( annotated )
        m_net_name_map[ aPath ] = std::make_pair( name, aForceNoConnect );

    return name;
}


VECTOR2I SCH_PIN::GetTransformedPosition() const
{
    TRANSFORM t = GetParentSymbol()->GetTransform();
    return t.TransformCoordinate( GetLocalPosition() ) + GetParentSymbol()->GetPosition();
}


const BOX2I SCH_PIN::GetBoundingBox( bool aIncludeInvisiblePins, bool aIncludeNameAndNumber,
                                     bool aIncludeElectricalType ) const
{
    TRANSFORM t = GetParentSymbol()->GetTransform();
    BOX2I     r;

    if( m_libPin )
        r = m_libPin->GetBoundingBox( aIncludeInvisiblePins, aIncludeNameAndNumber,
                                      aIncludeElectricalType );

    r.RevertYAxis();

    r = t.TransformCoordinate( r );
    r.Offset( GetParentSymbol()->GetPosition() );
    r.Normalize();

    return r;
}


bool SCH_PIN::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    // When looking for an "exact" hit aAccuracy will be 0 which works poorly if the pin has
    // no pin number or name.  Give it a floor.
    if( Schematic() )
        aAccuracy = std::max( aAccuracy, Schematic()->Settings().m_PinSymbolSize / 4 );

    BOX2I rect = GetBoundingBox( false, true, m_flags & SHOW_ELEC_TYPE );
    return rect.Inflate( aAccuracy ).Contains( aPosition );
}


bool SCH_PIN::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    BOX2I sel = aRect;

    if( aAccuracy )
        sel.Inflate( aAccuracy );

    if( aContained )
        return sel.Contains( GetBoundingBox( false, false, false ) );

    return sel.Intersects( GetBoundingBox( false, true, m_flags & SHOW_ELEC_TYPE ) );
}


EDA_ITEM* SCH_PIN::Clone() const
{
    return new SCH_PIN( *this );
}


bool SCH_PIN::HasConnectivityChanges( const SCH_ITEM* aItem,
                                      const SCH_SHEET_PATH* aInstance ) const
{
    // Do not compare to ourself.
    if( aItem == this )
        return false;

    const SCH_PIN* pin = dynamic_cast<const SCH_PIN*>( aItem );

    // Don't compare against a different SCH_ITEM.
    wxCHECK( pin, false );

    if( GetPosition() != pin->GetPosition() )
        return true;

    return GetNumber() != pin->GetNumber();
}


bool SCH_PIN::ConnectionPropagatesTo( const EDA_ITEM* aItem ) const
{
    if( !m_libPin )
        return false;

    // Reciprocal checking is done in CONNECTION_GRAPH anyway
    return !( m_libPin->GetType() == ELECTRICAL_PINTYPE::PT_NC );
}


bool SCH_PIN::operator==( const SCH_ITEM& aOther ) const
{
    if( aOther.Type() != SCH_PIN_T )
        return false;

    const SCH_PIN& other = static_cast<const SCH_PIN&>( aOther );

    if( m_number != other.m_number )
        return false;

    if( m_position != other.m_position )
        return false;

    if( !m_libPin )
        return false;

    return m_libPin == other.m_libPin;
}


double SCH_PIN::Similarity( const SCH_ITEM& aOther ) const
{
    if( m_Uuid == aOther.m_Uuid )
        return 1.0;

    if( aOther.Type() != SCH_PIN_T )
        return 0.0;

    const SCH_PIN& other = static_cast<const SCH_PIN&>( aOther );

    if( m_number != other.m_number )
        return 0.0;

    if( m_position != other.m_position )
        return 0.0;

    return m_libPin ? m_libPin->Similarity( *other.m_libPin ) : 0.0;
}


static struct SCH_PIN_DESC
{
    SCH_PIN_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( SCH_PIN );
        propMgr.InheritsAfter( TYPE_HASH( SCH_PIN ), TYPE_HASH( SCH_ITEM ) );

        propMgr.AddProperty( new PROPERTY<SCH_PIN, wxString>( _HKI( "Pin Name" ),
                NO_SETTER( SCH_PIN, wxString ), &SCH_PIN::GetName ) );

        propMgr.AddProperty( new PROPERTY<SCH_PIN, wxString>( _HKI( "Pin Number" ),
                NO_SETTER( SCH_PIN, wxString ), &SCH_PIN::GetNumber ) );

        propMgr.AddProperty( new PROPERTY<SCH_PIN, int>( _HKI( "Length" ),
                NO_SETTER( SCH_PIN, int ), &SCH_PIN::GetLength, PROPERTY_DISPLAY::PT_SIZE ) );
    }
} _SCH_PIN_DESC;
