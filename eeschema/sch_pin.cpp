/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2018 CERN
 * Copyright (C) 1992-2024 KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <base_units.h>
#include <pgm_base.h>
#include <sch_draw_panel.h>
#include <sch_edit_frame.h>
#include <symbol_edit_frame.h>
#include <sch_pin.h>
#include <settings/settings_manager.h>
#include <symbol_editor/symbol_editor_settings.h>
#include <trigo.h>
#include <string_utils.h>
#include <plotters/plotter.h>


// small margin in internal units between the pin text and the pin line
#define PIN_TEXT_MARGIN 4

wxString SCH_PIN::GetCanonicalElectricalTypeName( ELECTRICAL_PINTYPE aType )
{
    // These strings are the canonical name of the electrictal type
    // Not translated, no space in name, only ASCII chars.
    // to use when the string name must be known and well defined
    // must have same order than enum ELECTRICAL_PINTYPE (see sch_pin.h)
    static const wxChar* msgPinElectricType[] =
    {
        wxT( "input" ),
        wxT( "output" ),
        wxT( "bidirectional" ),
        wxT( "tri_state" ),
        wxT( "passive" ),
        wxT( "free" ),
        wxT( "unspecified" ),
        wxT( "power_in" ),
        wxT( "power_out" ),
        wxT( "open_collector" ),
        wxT( "open_emitter" ),
        wxT( "no_connect" )
    };

    return msgPinElectricType[static_cast<int>( aType )];
}


/// Utility for getting the size of the 'internal' pin decorators (as a radius)
// i.e. the clock symbols (falling clock is actually external but is of
// the same kind)

static int internalPinDecoSize( const RENDER_SETTINGS* aSettings, const SCH_PIN &aPin )
{
    const SCH_RENDER_SETTINGS* settings = static_cast<const SCH_RENDER_SETTINGS*>( aSettings );

    if( settings && settings->m_PinSymbolSize )
        return settings->m_PinSymbolSize;

    return aPin.GetNameTextSize() != 0 ? aPin.GetNameTextSize() / 2 : aPin.GetNumberTextSize() / 2;
}

/// Utility for getting the size of the 'external' pin decorators (as a radius)
// i.e. the negation circle, the polarity 'slopes' and the nonlogic
// marker
static int externalPinDecoSize( const RENDER_SETTINGS* aSettings, const SCH_PIN &aPin )
{
    const SCH_RENDER_SETTINGS* settings = static_cast<const SCH_RENDER_SETTINGS*>( aSettings );

    if( settings && settings->m_PinSymbolSize )
        return settings->m_PinSymbolSize;

    return aPin.GetNumberTextSize() / 2;
}


SCH_PIN::SCH_PIN( LIB_SYMBOL* aParentSymbol ) :
        SCH_ITEM( aParentSymbol, SCH_PIN_T, 0, 0 ),
        m_libPin( nullptr ),
        m_position( { 0, 0 } ),
        m_orientation( PIN_ORIENTATION::PIN_RIGHT ),
        m_shape( GRAPHIC_PINSHAPE::LINE ),
        m_type( ELECTRICAL_PINTYPE::PT_UNSPECIFIED ),
        m_hidden( false ),
        m_isDangling( true )
{
    // Use the application settings for pin sizes if exists.
    // pgm can be nullptr when running a shared lib from a script, not from a kicad appl
    PGM_BASE* pgm  = PgmOrNull();

    if( pgm )
    {
        auto* settings = pgm->GetSettingsManager().GetAppSettings<SYMBOL_EDITOR_SETTINGS>();
        m_length       = schIUScale.MilsToIU( settings->m_Defaults.pin_length );
        m_numTextSize  = schIUScale.MilsToIU( settings->m_Defaults.pin_num_size );
        m_nameTextSize = schIUScale.MilsToIU( settings->m_Defaults.pin_name_size );
    }
    else    // Use hardcoded eeschema defaults: symbol_editor settings are not existing.
    {
        m_length       = schIUScale.MilsToIU( DEFAULT_PIN_LENGTH );
        m_numTextSize  = schIUScale.MilsToIU( DEFAULT_PINNUM_SIZE );
        m_nameTextSize = schIUScale.MilsToIU( DEFAULT_PINNAME_SIZE );
    }

    m_layer = LAYER_DEVICE;
}


SCH_PIN::SCH_PIN( LIB_SYMBOL* aParentSymbol, const wxString& aName, const wxString& aNumber,
                  PIN_ORIENTATION aOrientation, ELECTRICAL_PINTYPE aPinType, int aLength,
                  int aNameTextSize, int aNumTextSize, int aBodyStyle, const VECTOR2I& aPos,
                  int aUnit ) :
        SCH_ITEM( aParentSymbol, SCH_PIN_T, aUnit, aBodyStyle ),
        m_libPin( nullptr ),
        m_position( aPos ),
        m_length( aLength ),
        m_orientation( aOrientation ),
        m_shape( GRAPHIC_PINSHAPE::LINE ),
        m_type( aPinType ),
        m_hidden( false ),
        m_numTextSize( aNumTextSize ),
        m_nameTextSize( aNameTextSize ),
        m_isDangling( true )
{
    SetName( aName );
    SetNumber( aNumber );

    m_layer = LAYER_DEVICE;
}


SCH_PIN::SCH_PIN( SCH_SYMBOL* aParentSymbol, SCH_PIN* aLibPin ) :
        SCH_ITEM( aParentSymbol, SCH_PIN_T, 0, 0 ),
        m_libPin( aLibPin ),
        m_orientation( PIN_ORIENTATION::INHERIT ),
        m_shape( GRAPHIC_PINSHAPE::INHERIT ),
        m_type( ELECTRICAL_PINTYPE::PT_INHERIT ),
        m_isDangling( true )
{
    wxASSERT( aParentSymbol );

    SetName( m_libPin->GetName() );
    SetNumber( m_libPin->GetNumber() );
    m_position = m_libPin->GetPosition();

    m_layer = LAYER_PIN;
}


/**
 * Create a proxy pin from an alternate pin designation.
 * The SCH_PIN data will be filled in when the pin is resolved (see SCH_SYMBOL::UpdatePins).
 */
SCH_PIN::SCH_PIN( SCH_SYMBOL* aParentSymbol, const wxString& aNumber, const wxString& aAlt,
                  const KIID& aUuid ) :
        SCH_ITEM( aParentSymbol, SCH_PIN_T ),
        m_libPin( nullptr ),
        m_orientation( PIN_ORIENTATION::INHERIT ),
        m_shape( GRAPHIC_PINSHAPE::INHERIT ),
        m_type( ELECTRICAL_PINTYPE::PT_INHERIT ),
        m_number( aNumber ),
        m_alt( aAlt ),
        m_isDangling( true )
{
    wxASSERT( aParentSymbol );

    const_cast<KIID&>( m_Uuid ) = aUuid;
    m_layer = LAYER_PIN;
}


SCH_PIN::SCH_PIN( const SCH_PIN& aPin ) :
        SCH_ITEM( aPin ),
        m_libPin( aPin.m_libPin ),
        m_alternates( aPin.m_alternates ),
        m_position( aPin.m_position ),
        m_length( aPin.m_length ),
        m_orientation( aPin.m_orientation ),
        m_shape( aPin.m_shape ),
        m_type( aPin.m_type ),
        m_hidden( aPin.m_hidden ),
        m_numTextSize( aPin.m_numTextSize ),
        m_nameTextSize( aPin.m_nameTextSize ),
        m_alt( aPin.m_alt ),
        m_isDangling( aPin.m_isDangling )
{
    SetName( aPin.m_name );
    SetNumber( aPin.m_number );

    m_layer = aPin.m_layer;
}


SCH_PIN& SCH_PIN::operator=( const SCH_PIN& aPin )
{
    SCH_ITEM::operator=( aPin );

    m_libPin = aPin.m_libPin;
    m_alternates = aPin.m_alternates;
    m_alt = aPin.m_alt;
    m_name = aPin.m_name;
    m_number = aPin.m_number;
    m_position = aPin.m_position;
    m_length = aPin.m_length;
    m_orientation = aPin.m_orientation;
    m_shape = aPin.m_shape;
    m_type = aPin.m_type;
    m_hidden = aPin.m_hidden;
    m_numTextSize = aPin.m_numTextSize;
    m_nameTextSize = aPin.m_nameTextSize;
    m_isDangling = aPin.m_isDangling;

    return *this;
}


VECTOR2I SCH_PIN::GetPosition() const
{
    if( const SCH_SYMBOL* symbol = dynamic_cast<const SCH_SYMBOL*>( GetParentSymbol() ) )
        return symbol->GetTransform().TransformCoordinate( m_position ) + symbol->GetPosition();
    else
        return m_position;
}

PIN_ORIENTATION SCH_PIN::GetOrientation() const
{
    if( m_orientation == PIN_ORIENTATION::INHERIT )
    {
        wxCHECK_MSG( m_libPin, PIN_ORIENTATION::PIN_RIGHT, wxS( "Can't inherit without a libPin!" ) );

        return m_libPin->GetOrientation();
    }

    return m_orientation;
}


GRAPHIC_PINSHAPE SCH_PIN::GetShape() const
{
    if( !m_alt.IsEmpty() )
    {
        wxCHECK_MSG( m_libPin, GRAPHIC_PINSHAPE::LINE, wxS( "Can't specify alternate without a "
                                                            "libPin!" ) );

        return m_libPin->GetAlt( m_alt ).m_Shape;
    }
    else if( m_shape == GRAPHIC_PINSHAPE::INHERIT )
    {
        wxCHECK_MSG( m_libPin, GRAPHIC_PINSHAPE::LINE, wxS( "Can't inherit without a libPin!" ) );

        return m_libPin->GetShape();
    }

    return m_shape;
}


int SCH_PIN::GetLength() const
{
    if( !m_length.has_value() )
    {
        wxCHECK_MSG( m_libPin, 0, wxS( "Can't inherit without a libPin!" ) );

        return m_libPin->GetLength();
    }

    return m_length.value();
}


ELECTRICAL_PINTYPE SCH_PIN::GetType() const
{
    if( !m_alt.IsEmpty() )
    {
        wxCHECK_MSG( m_libPin, ELECTRICAL_PINTYPE::PT_UNSPECIFIED, wxS( "Can't specify alternate "
                                                                        "without a libPin!" ) );

        return m_libPin->GetAlt( m_alt ).m_Type;
    }
    else if( m_type == ELECTRICAL_PINTYPE::PT_INHERIT )
    {
        wxCHECK_MSG( m_libPin, ELECTRICAL_PINTYPE::PT_UNSPECIFIED, wxS( "Can't inherit without a "
                                                                        "libPin!" ) );

        return m_libPin->GetType();
    }

    return m_type;
}


wxString SCH_PIN::GetCanonicalElectricalTypeName() const
{
    if( m_type == ELECTRICAL_PINTYPE::PT_INHERIT )
    {
        wxCHECK_MSG( m_libPin, GetCanonicalElectricalTypeName( ELECTRICAL_PINTYPE::PT_UNSPECIFIED ),
                     wxS( "Can't inherit without a m_libPin!" ) );

        return m_libPin->GetCanonicalElectricalTypeName();
    }

    return GetCanonicalElectricalTypeName( m_type );
}


wxString SCH_PIN::GetElectricalTypeName() const
{
    if( m_type == ELECTRICAL_PINTYPE::PT_INHERIT )
    {
        wxCHECK_MSG( m_libPin, ElectricalPinTypeGetText( ELECTRICAL_PINTYPE::PT_UNSPECIFIED ),
                     wxS( "Can't inherit without a m_libPin!" ) );

        return m_libPin->GetElectricalTypeName();
    }

    return ElectricalPinTypeGetText( m_type );
}


bool SCH_PIN::IsVisible() const
{
    if( !m_hidden.has_value() )
    {
        wxCHECK_MSG( m_libPin, true, wxS( "Can't inherit without a libPin!" ) );

        return m_libPin->IsVisible();
    }

    return !m_hidden.value();
}


const wxString& SCH_PIN::GetName() const
{
    if( !m_alt.IsEmpty() )
        return m_alt;

    return GetBaseName();
}


const wxString& SCH_PIN::GetBaseName() const
{
    if( m_libPin )
        return m_libPin->GetBaseName();

    return m_name;
}


void SCH_PIN::SetName( const wxString& aName )
{
    m_name = aName;

    // pin name string does not support spaces
    m_name.Replace( wxT( " " ), wxT( "_" ) );
    m_nameExtentsCache.m_Extents = VECTOR2I();
}


bool SCH_PIN::IsStacked( const SCH_PIN* aPin ) const
{
    bool isConnectableType_a = GetType() == ELECTRICAL_PINTYPE::PT_PASSIVE
                            || GetType() == ELECTRICAL_PINTYPE::PT_NIC;
    bool isConnectableType_b = aPin->GetType() == ELECTRICAL_PINTYPE::PT_PASSIVE
                            || aPin->GetType() == ELECTRICAL_PINTYPE::PT_NIC;

    return m_parent == aPin->GetParent()
           && GetPosition() == aPin->GetPosition()
           && GetName() == aPin->GetName()
           && ( GetType() == aPin->GetType() || isConnectableType_a || isConnectableType_b );
}


bool SCH_PIN::Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) const
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
    if( m_flags & (STRUCT_DELETED | SKIP_STRUCT ) )
        return false;

    BOX2I sel = aRect;

    if ( aAccuracy )
        sel.Inflate( aAccuracy );

    if( aContained )
        return sel.Contains( GetBoundingBox( false, false, false ) );

    return sel.Intersects( GetBoundingBox( false, true, m_flags & SHOW_ELEC_TYPE ) );
}


wxString SCH_PIN::GetShownName() const
{
    if( !m_alt.IsEmpty() )
        return m_alt;
    else if( m_libPin )
        return m_libPin->GetShownName();

    if( m_name == wxS( "~" ) )
        return wxEmptyString;
    else
        return m_name;
}


wxString SCH_PIN::GetShownNumber() const
{
    if( m_number == wxS( "~" ) )
        return wxEmptyString;
    else
        return m_number;
}


void SCH_PIN::SetNumber( const wxString& aNumber )
{
    m_number = aNumber;

    // pin number string does not support spaces
    m_number.Replace( wxT( " " ), wxT( "_" ) );
    m_numExtentsCache.m_Extents = VECTOR2I();
}


int SCH_PIN::GetNameTextSize() const
{
    if( !m_nameTextSize.has_value() )
    {
        wxCHECK_MSG( m_libPin, schIUScale.MilsToIU( DEFAULT_PINNAME_SIZE ),
                     wxS( "Can't inherit without a libPin!" ) );

        return m_libPin->GetNameTextSize();
    }
    wxASSERT( !m_libPin );

    return m_nameTextSize.value();
}


void SCH_PIN::SetNameTextSize( int aSize )
{
    m_nameTextSize = aSize;
    m_nameExtentsCache.m_Extents = VECTOR2I();
}


int SCH_PIN::GetNumberTextSize() const
{
    if( !m_numTextSize.has_value() )
    {
        wxCHECK_MSG( m_libPin, schIUScale.MilsToIU( DEFAULT_PINNUM_SIZE ),
                     wxS( "Can't inherit without a libPin!" ) );

        return m_libPin->GetNumberTextSize();
    }
    wxASSERT( !m_libPin );

    return m_numTextSize.value();
}


void SCH_PIN::SetNumberTextSize( int aSize )
{
    m_numTextSize = aSize;
    m_numExtentsCache.m_Extents = VECTOR2I();
}


VECTOR2I SCH_PIN::GetPinRoot() const
{
    if( const SCH_SYMBOL* symbol = dynamic_cast<const SCH_SYMBOL*>( GetParentSymbol() ) )
    {
        const TRANSFORM& t = symbol->GetTransform();
        wxCHECK( m_libPin, GetPosition() );
        return t.TransformCoordinate( m_libPin->GetPinRoot() ) + symbol->GetPosition();
    }

    switch( GetOrientation() )
    {
    default:
    case PIN_ORIENTATION::PIN_RIGHT: return m_position + VECTOR2I(  GetLength(), 0 );
    case PIN_ORIENTATION::PIN_LEFT:  return m_position + VECTOR2I( -GetLength(), 0 );
    case PIN_ORIENTATION::PIN_UP:    return m_position + VECTOR2I( 0, -GetLength() );
    case PIN_ORIENTATION::PIN_DOWN:  return m_position + VECTOR2I( 0,  GetLength() );
    }
}


void SCH_PIN::Print( const SCH_RENDER_SETTINGS* aSettings, int aUnit, int aBodyStyle,
                     const VECTOR2I& aOffset, bool aForceNoFill, bool aDimmed )
{
    LIB_SYMBOL* part = dynamic_cast<LIB_SYMBOL*>( GetParentSymbol() );

    wxCHECK( part && aSettings, /* void */ );

    /* Calculate pin orient taking in account the symbol orientation. */
    PIN_ORIENTATION orient = PinDrawOrient( aSettings->m_Transform );

    /* Calculate the pin position */
    VECTOR2I pos1 = aSettings->TransformCoordinate( m_position ) + aOffset;

    if( IsVisible() || aSettings->m_ShowHiddenFields )
    {
        printPinSymbol( aSettings, pos1, orient, aDimmed );

        printPinTexts( aSettings, pos1, orient, part->GetPinNameOffset(),
                       aSettings->m_ShowPinNumbers || part->GetShowPinNumbers(),
                       aSettings->m_ShowPinNames || part->GetShowPinNames(),
                       aDimmed );

        if( aSettings->m_ShowPinElectricalTypes )
            printPinElectricalTypeName( aSettings, pos1, orient, aDimmed );

        if( aSettings->m_ShowConnectionPoints
                && m_type != ELECTRICAL_PINTYPE::PT_NC
                && m_type != ELECTRICAL_PINTYPE::PT_NIC )
        {
            wxDC* DC = aSettings->GetPrintDC();
            COLOR4D color = aSettings->GetLayerColor( IsVisible() ? LAYER_PIN : LAYER_HIDDEN );

            COLOR4D bg = aSettings->GetBackgroundColor();

            if( bg == COLOR4D::UNSPECIFIED || GetGRForceBlackPenState() )
                bg = COLOR4D::WHITE;

            if( aDimmed )
            {
                color.Desaturate( );
                color = color.Mix( bg, 0.5f );
            }

            GRCircle( DC, pos1, TARGET_PIN_RADIUS, 0, color );
        }
    }
}


void SCH_PIN::printPinSymbol( const SCH_RENDER_SETTINGS* aSettings, const VECTOR2I& aPos,
                              PIN_ORIENTATION aOrientation, bool aDimmed )
{
    wxDC*   DC = aSettings->GetPrintDC();
    int     MapX1, MapY1, x1, y1;
    int     width = GetEffectivePenWidth( aSettings );
    int     posX = aPos.x;
    int     posY = aPos.y;
    int     len = GetLength();
    COLOR4D color = aSettings->GetLayerColor( IsVisible() ? LAYER_PIN : LAYER_HIDDEN );
    COLOR4D bg = aSettings->GetBackgroundColor();

    if( bg == COLOR4D::UNSPECIFIED || GetGRForceBlackPenState() )
        bg = COLOR4D::WHITE;

    if( !IsVisible() )
        bg = aSettings->GetLayerColor( LAYER_HIDDEN );

    if( aDimmed )
    {
        color.Desaturate( );
        color = color.Mix( bg, 0.5f );
    }

    MapX1 = MapY1 = 0;
    x1    = posX;
    y1    = posY;

    switch( aOrientation )
    {
    case PIN_ORIENTATION::PIN_UP:    y1 = posY - len;  MapY1 = 1;                           break;
    case PIN_ORIENTATION::PIN_DOWN:  y1 = posY + len;  MapY1 = -1;                          break;
    case PIN_ORIENTATION::PIN_LEFT:  x1 = posX - len;  MapX1 = 1;                           break;
    case PIN_ORIENTATION::PIN_RIGHT: x1 = posX + len;  MapX1 = -1;                          break;
    case PIN_ORIENTATION::INHERIT:   wxFAIL_MSG( wxS( "aOrientation must be resolved!" ) ); break;
    }

    if( m_shape == GRAPHIC_PINSHAPE::INVERTED || m_shape == GRAPHIC_PINSHAPE::INVERTED_CLOCK )
    {
        const int radius = externalPinDecoSize( aSettings, *this );
        GRCircle( DC, VECTOR2I( MapX1 * radius + x1, MapY1 * radius + y1 ), radius, width, color );

        GRMoveTo( MapX1 * radius * 2 + x1, MapY1 * radius * 2 + y1 );
        GRLineTo( DC, posX, posY, width, color );
    }
    else
    {
        GRMoveTo( x1, y1 );
        GRLineTo( DC, posX, posY, width, color );
    }

    // Draw the clock shape (>)inside the symbol
    if( m_shape == GRAPHIC_PINSHAPE::CLOCK
            || m_shape == GRAPHIC_PINSHAPE::INVERTED_CLOCK
            || m_shape == GRAPHIC_PINSHAPE::FALLING_EDGE_CLOCK
            || m_shape == GRAPHIC_PINSHAPE::CLOCK_LOW )
    {
        const int clock_size = internalPinDecoSize( aSettings, *this );
        if( MapY1 == 0 ) /* MapX1 = +- 1 */
        {
            GRMoveTo( x1, y1 + clock_size );
            GRLineTo( DC, x1 - MapX1 * clock_size * 2, y1, width, color );
            GRLineTo( DC, x1, y1 - clock_size, width, color );
        }
        else    /* MapX1 = 0 */
        {
            GRMoveTo( x1 + clock_size, y1 );
            GRLineTo( DC, x1, y1 - MapY1 * clock_size * 2, width, color );
            GRLineTo( DC, x1 - clock_size, y1, width, color );
        }
    }

    // Draw the active low (or H to L active transition)
    if( m_shape == GRAPHIC_PINSHAPE::INPUT_LOW
            || m_shape == GRAPHIC_PINSHAPE::FALLING_EDGE_CLOCK
            || m_shape == GRAPHIC_PINSHAPE::CLOCK_LOW )
    {
        const int deco_size = externalPinDecoSize( aSettings, *this );
        if( MapY1 == 0 )            /* MapX1 = +- 1 */
        {
            GRMoveTo( x1 + MapX1 * deco_size * 2, y1 );
            GRLineTo( DC, x1 + MapX1 * deco_size * 2, y1 - deco_size * 2, width, color );
            GRLineTo( DC, x1, y1, width, color );
        }
        else    /* MapX1 = 0 */
        {
            GRMoveTo( x1, y1 + MapY1 * deco_size * 2 );
            GRLineTo( DC, x1 - deco_size * 2, y1 + MapY1 * deco_size * 2, width, color );
            GRLineTo( DC, x1, y1, width, color );
        }
    }

    if( m_shape == GRAPHIC_PINSHAPE::OUTPUT_LOW ) /* IEEE symbol "Active Low Output" */
    {
        const int deco_size = externalPinDecoSize( aSettings, *this );
        if( MapY1 == 0 )            /* MapX1 = +- 1 */
        {
            GRMoveTo( x1, y1 - deco_size * 2 );
            GRLineTo( DC, x1 + MapX1 * deco_size * 2, y1, width, color );
        }
        else    /* MapX1 = 0 */
        {
            GRMoveTo( x1 - deco_size * 2, y1 );
            GRLineTo( DC, x1, y1 + MapY1 * deco_size * 2, width, color );
        }
    }
    else if( m_shape == GRAPHIC_PINSHAPE::NONLOGIC ) /* NonLogic pin symbol */
    {
        const int deco_size = externalPinDecoSize( aSettings, *this );
        GRMoveTo( x1 - (MapX1 + MapY1) * deco_size, y1 - (MapY1 - MapX1) * deco_size );
        GRLineTo( DC, x1 + (MapX1 + MapY1) * deco_size, y1 + ( MapY1 - MapX1 ) * deco_size, width,
                  color );
        GRMoveTo( x1 - (MapX1 - MapY1) * deco_size, y1 - (MapY1 + MapX1) * deco_size );
        GRLineTo( DC, x1 + (MapX1 - MapY1) * deco_size, y1 + ( MapY1 + MapX1 ) * deco_size, width,
                  color );
    }

    if( m_type == ELECTRICAL_PINTYPE::PT_NC ) // Draw a N.C. symbol
    {
        const int deco_size = TARGET_PIN_RADIUS;
        GRLine( DC, posX - deco_size, posY - deco_size, posX + deco_size, posY + deco_size, width,
                color );
        GRLine( DC, posX + deco_size, posY - deco_size, posX - deco_size, posY + deco_size, width,
                color );
    }
}


void SCH_PIN::printPinTexts( const RENDER_SETTINGS* aSettings, const VECTOR2I& aPinPos,
                             PIN_ORIENTATION aPinOrient, int aTextInside, bool aDrawPinNum,
                             bool aDrawPinName, bool aDimmed )
{
    if( !aDrawPinName && !aDrawPinNum )
        return;

    KIFONT::FONT* font = KIFONT::FONT::GetFont( aSettings->GetDefaultFont(), false, false );
    wxString      name = GetShownName();
    wxString      number = GetShownNumber();
    VECTOR2I      nameSize( GetNameTextSize(), GetNameTextSize() );
    VECTOR2I      numSize( GetNumberTextSize(), GetNumberTextSize() );
    int           name_offset = schIUScale.MilsToIU( PIN_TEXT_MARGIN );
    int           num_offset = schIUScale.MilsToIU( PIN_TEXT_MARGIN );

    /* Get the num and name colors */
    COLOR4D nameColor = aSettings->GetLayerColor( IsVisible() ? LAYER_PINNAM : LAYER_HIDDEN );
    COLOR4D numColor  = aSettings->GetLayerColor( IsVisible() ? LAYER_PINNUM : LAYER_HIDDEN );
    COLOR4D bg = aSettings->GetBackgroundColor();

    if( bg == COLOR4D::UNSPECIFIED || GetGRForceBlackPenState() )
        bg = COLOR4D::WHITE;

    if( !IsVisible() )
        bg = aSettings->GetLayerColor( LAYER_HIDDEN );

    if( aDimmed )
    {
        nameColor.Desaturate();
        numColor.Desaturate();
        nameColor = nameColor.Mix( bg, 0.5f );
        numColor = numColor.Mix( bg, 0.5f );
    }

    int x1 = aPinPos.x;
    int y1 = aPinPos.y;

    switch( aPinOrient )
    {
    case PIN_ORIENTATION::PIN_UP:    y1 -= GetLength();                                   break;
    case PIN_ORIENTATION::PIN_DOWN:  y1 += GetLength();                                   break;
    case PIN_ORIENTATION::PIN_LEFT:  x1 -= GetLength();                                   break;
    case PIN_ORIENTATION::PIN_RIGHT: x1 += GetLength();                                   break;
    case PIN_ORIENTATION::INHERIT:   wxFAIL_MSG( wxT( "aPinOrient must be resolved!" ) ); break;
    }

    if( name.IsEmpty() || m_nameTextSize == 0 )
        aDrawPinName = false;

    if( number.IsEmpty() || m_numTextSize == 0 )
        aDrawPinNum = false;

    auto printName =
            [&]( int x, int y, const EDA_ANGLE& angle, enum GR_TEXT_H_ALIGN_T hAlign,
                 enum GR_TEXT_V_ALIGN_T vAlign )
            {
                GRPrintText( aSettings->GetPrintDC(), VECTOR2I( x, y ), nameColor, name, angle,
                             nameSize, hAlign, vAlign, 0, false, false, font, GetFontMetrics() );
            };

    auto printNum =
            [&]( int x, int y, const EDA_ANGLE& angle, enum GR_TEXT_H_ALIGN_T hAlign,
                 enum GR_TEXT_V_ALIGN_T vAlign )
            {
                GRPrintText( aSettings->GetPrintDC(), VECTOR2I( x, y ), numColor, number, angle,
                             numSize, hAlign, vAlign, 0, false, false, font, GetFontMetrics() );
            };

    if( aTextInside )  // Draw the text inside, but the pin numbers outside.
    {
        if( ( aPinOrient == PIN_ORIENTATION::PIN_LEFT )
            || ( aPinOrient == PIN_ORIENTATION::PIN_RIGHT ) )
        {
            // It is an horizontal line
            if( aDrawPinName )
            {
                if( aPinOrient == PIN_ORIENTATION::PIN_RIGHT )
                {
                    printName( x1 + aTextInside, y1, ANGLE_HORIZONTAL,
                               GR_TEXT_H_ALIGN_LEFT, GR_TEXT_V_ALIGN_CENTER );
                }
                else    // Orient == PIN_LEFT
                {
                    printName( x1 - aTextInside, y1, ANGLE_HORIZONTAL,
                               GR_TEXT_H_ALIGN_RIGHT, GR_TEXT_V_ALIGN_CENTER );
                }
            }

            if( aDrawPinNum )
            {
                printNum( ( x1 + aPinPos.x ) / 2, y1 - num_offset, ANGLE_HORIZONTAL,
                          GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_BOTTOM );
            }
        }
        else            /* Its a vertical line. */
        {
            // Text is drawn from bottom to top (i.e. to negative value for Y axis)
            if( aPinOrient == PIN_ORIENTATION::PIN_DOWN )
            {
                if( aDrawPinName )
                {
                    printName( x1, y1 + aTextInside, ANGLE_VERTICAL,
                               GR_TEXT_H_ALIGN_RIGHT, GR_TEXT_V_ALIGN_CENTER );
                }

                if( aDrawPinNum )
                {
                    printNum( x1 - num_offset, ( y1 + aPinPos.y ) / 2, ANGLE_VERTICAL,
                              GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_BOTTOM );
                }
            }
            else        /* PIN_UP */
            {
                if( aDrawPinName )
                {
                    printName( x1, y1 - aTextInside, ANGLE_VERTICAL,
                               GR_TEXT_H_ALIGN_LEFT, GR_TEXT_V_ALIGN_CENTER );
                }

                if( aDrawPinNum )
                {
                    printNum( x1 - num_offset, ( y1 + aPinPos.y) / 2, ANGLE_VERTICAL,
                              GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_BOTTOM );
                }
            }
        }
    }
    else     /**** Draw num & text pin outside  ****/
    {
        if( ( aPinOrient == PIN_ORIENTATION::PIN_LEFT )
            || ( aPinOrient == PIN_ORIENTATION::PIN_RIGHT ) )
        {
            /* Its an horizontal line. */
            if( aDrawPinName && aDrawPinNum )
            {
                printName( ( x1 + aPinPos.x ) / 2, y1 - name_offset, ANGLE_HORIZONTAL,
                           GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_BOTTOM );

                printNum( ( x1 + aPinPos.x ) / 2, y1 + num_offset, ANGLE_HORIZONTAL,
                          GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_TOP );
            }
            else if( aDrawPinName )
            {
                printName( ( x1 + aPinPos.x ) / 2, y1 - name_offset, ANGLE_HORIZONTAL,
                           GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_BOTTOM );
            }
            else if( aDrawPinNum )
            {
                printNum( ( x1 + aPinPos.x ) / 2, y1 - num_offset, ANGLE_HORIZONTAL,
                          GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_BOTTOM );
            }
        }
        else     /* Its a vertical line. */
        {
            if( aDrawPinName && aDrawPinNum )
            {
                printName( x1 - name_offset, ( y1 + aPinPos.y ) / 2, ANGLE_VERTICAL,
                           GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_BOTTOM );

                printNum( x1 + num_offset, ( y1 + aPinPos.y ) / 2, ANGLE_VERTICAL,
                          GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_TOP );
            }
            else if( aDrawPinName )
            {
                printName( x1 - name_offset, ( y1 + aPinPos.y ) / 2, ANGLE_VERTICAL,
                           GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_BOTTOM );
            }
            else if( aDrawPinNum )
            {
                printNum( x1 - num_offset, ( y1 + aPinPos.y ) / 2, ANGLE_VERTICAL,
                          GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_BOTTOM );
            }
        }
    }
}


void SCH_PIN::printPinElectricalTypeName( const RENDER_SETTINGS* aSettings,
                                          const VECTOR2I& aPosition, PIN_ORIENTATION aOrientation,
                                          bool aDimmed )
{
    wxDC*       DC = aSettings->GetPrintDC();
    wxString    typeName = GetElectricalTypeName();

    // Use a reasonable (small) size to draw the text
    int         textSize = ( GetNameTextSize() * 3 ) / 4;

    #define ETXT_MAX_SIZE schIUScale.mmToIU( 0.7 )

    if( textSize > ETXT_MAX_SIZE )
        textSize = ETXT_MAX_SIZE;

    // Use a reasonable pen size to draw the text
    int pensize = textSize/6;

    // Get a suitable color
    COLOR4D color = aSettings->GetLayerColor( IsVisible() ? LAYER_PRIVATE_NOTES : LAYER_HIDDEN );
    COLOR4D bg = aSettings->GetBackgroundColor();

    if( bg == COLOR4D::UNSPECIFIED || GetGRForceBlackPenState() )
        bg = COLOR4D::WHITE;

    if( !IsVisible() )
        bg = aSettings->GetLayerColor( LAYER_HIDDEN );

    if( aDimmed )
    {
        color.Desaturate( );
        color = color.Mix( bg, 0.5f );
    }

    VECTOR2I          txtpos = aPosition;
    int               offset = schIUScale.mmToIU( 0.4 );
    GR_TEXT_H_ALIGN_T hjustify = GR_TEXT_H_ALIGN_LEFT;
    EDA_ANGLE         orient = ANGLE_HORIZONTAL;
    KIFONT::FONT*     font = KIFONT::FONT::GetFont( aSettings->GetDefaultFont(), false, false );

    switch( aOrientation )
    {
    case PIN_ORIENTATION::PIN_UP:
        txtpos.y += offset;
        orient = ANGLE_VERTICAL;
        hjustify = GR_TEXT_H_ALIGN_RIGHT;
        break;

    case PIN_ORIENTATION::PIN_DOWN:
        txtpos.y -= offset;
        orient = ANGLE_VERTICAL;
        break;

    case PIN_ORIENTATION::PIN_LEFT:
        txtpos.x += offset;
        break;

    case PIN_ORIENTATION::PIN_RIGHT:
        txtpos.x -= offset;
        hjustify = GR_TEXT_H_ALIGN_RIGHT;
        break;

    case PIN_ORIENTATION::INHERIT:
        wxFAIL_MSG( wxS( "aOrientation must be resolved!" ) );
        break;
    }

    GRPrintText( DC, txtpos, color, typeName, orient, VECTOR2I( textSize, textSize ), hjustify,
                 GR_TEXT_V_ALIGN_CENTER, pensize, false, false, font, GetFontMetrics() );
}


void SCH_PIN::PlotPinType( PLOTTER *aPlotter, const VECTOR2I &aPosition,
                           PIN_ORIENTATION aOrientation, bool aDimmed ) const
{
    int                  MapX1, MapY1, x1, y1;
    SCH_RENDER_SETTINGS* renderSettings = getRenderSettings( aPlotter );
    COLOR4D              color = renderSettings->GetLayerColor( LAYER_PIN );
    COLOR4D              bg = renderSettings->GetBackgroundColor();
    int                  penWidth = GetEffectivePenWidth( renderSettings );
    int                  pinLength = GetLength();

    if( bg == COLOR4D::UNSPECIFIED || !aPlotter->GetColorMode() )
        bg = COLOR4D::WHITE;

    if( aDimmed )
    {
        color.Desaturate( );
        color = color.Mix( bg, 0.5f );
    }

    aPlotter->SetColor( color );
    aPlotter->SetCurrentLineWidth( penWidth );

    MapX1 = MapY1 = 0;
    x1 = aPosition.x; y1 = aPosition.y;

    switch( aOrientation )
    {
    case PIN_ORIENTATION::PIN_UP:     y1 = aPosition.y - pinLength;  MapY1 = 1;              break;
    case PIN_ORIENTATION::PIN_DOWN:   y1 = aPosition.y + pinLength;  MapY1 = -1;             break;
    case PIN_ORIENTATION::PIN_LEFT:   x1 = aPosition.x - pinLength;  MapX1 = 1;              break;
    case PIN_ORIENTATION::PIN_RIGHT:  x1 = aPosition.x + pinLength;  MapX1 = -1;             break;
    case PIN_ORIENTATION::INHERIT:    wxFAIL_MSG( wxS( "aOrientation must be resolved!" ) ); break;
    }

    if( m_shape == GRAPHIC_PINSHAPE::INVERTED || m_shape == GRAPHIC_PINSHAPE::INVERTED_CLOCK )
    {
        const int radius = externalPinDecoSize( aPlotter->RenderSettings(), *this );
        aPlotter->Circle( VECTOR2I( MapX1 * radius + x1, MapY1 * radius + y1 ), radius * 2,
                          FILL_T::NO_FILL, penWidth );

        aPlotter->MoveTo( VECTOR2I( MapX1 * radius * 2 + x1, MapY1 * radius * 2 + y1 ) );
        aPlotter->FinishTo( aPosition );
    }
    else if( m_shape == GRAPHIC_PINSHAPE::FALLING_EDGE_CLOCK )
    {
        const int deco_size = internalPinDecoSize( aPlotter->RenderSettings(), *this );
        if( MapY1 == 0 ) /* MapX1 = +- 1 */
        {
            aPlotter->MoveTo( VECTOR2I( x1, y1 + deco_size ) );
            aPlotter->LineTo( VECTOR2I( x1 + MapX1 * deco_size * 2, y1 ) );
            aPlotter->FinishTo( VECTOR2I( x1, y1 - deco_size ) );
        }
        else    /* MapX1 = 0 */
        {
            aPlotter->MoveTo( VECTOR2I( x1 + deco_size, y1 ) );
            aPlotter->LineTo( VECTOR2I( x1, y1 + MapY1 * deco_size * 2 ) );
            aPlotter->FinishTo( VECTOR2I( x1 - deco_size, y1 ) );
        }

        aPlotter->MoveTo( VECTOR2I( MapX1 * deco_size * 2 + x1, MapY1 * deco_size * 2 + y1 ) );
        aPlotter->FinishTo( aPosition );
    }
    else
    {
        aPlotter->MoveTo( VECTOR2I( x1, y1 ) );
        aPlotter->FinishTo( aPosition );
    }

    if( m_shape == GRAPHIC_PINSHAPE::CLOCK
            || m_shape == GRAPHIC_PINSHAPE::INVERTED_CLOCK
            || m_shape == GRAPHIC_PINSHAPE::CLOCK_LOW )
    {
        const int deco_size = internalPinDecoSize( aPlotter->RenderSettings(), *this );
        if( MapY1 == 0 ) /* MapX1 = +- 1 */
        {
            aPlotter->MoveTo( VECTOR2I( x1, y1 + deco_size ) );
            aPlotter->LineTo( VECTOR2I( x1 - MapX1 * deco_size * 2, y1 ) );
            aPlotter->FinishTo( VECTOR2I( x1, y1 - deco_size ) );
        }
        else    /* MapX1 = 0 */
        {
            aPlotter->MoveTo( VECTOR2I( x1 + deco_size, y1 ) );
            aPlotter->LineTo( VECTOR2I( x1, y1 - MapY1 * deco_size * 2 ) );
            aPlotter->FinishTo( VECTOR2I( x1 - deco_size, y1 ) );
        }
    }

    if( m_shape == GRAPHIC_PINSHAPE::INPUT_LOW
            || m_shape == GRAPHIC_PINSHAPE::CLOCK_LOW ) /* IEEE symbol "Active Low Input" */
    {
        const int deco_size = externalPinDecoSize( aPlotter->RenderSettings(), *this );

        if( MapY1 == 0 )        /* MapX1 = +- 1 */
        {
            aPlotter->MoveTo( VECTOR2I( x1 + MapX1 * deco_size * 2, y1 ) );
            aPlotter->LineTo( VECTOR2I( x1 + MapX1 * deco_size * 2, y1 - deco_size * 2 ) );
            aPlotter->FinishTo( VECTOR2I( x1, y1 ) );
        }
        else    /* MapX1 = 0 */
        {
            aPlotter->MoveTo( VECTOR2I( x1, y1 + MapY1 * deco_size * 2 ) );
            aPlotter->LineTo( VECTOR2I( x1 - deco_size * 2, y1 + MapY1 * deco_size * 2 ) );
            aPlotter->FinishTo( VECTOR2I( x1, y1 ) );
        }
    }

    if( m_shape == GRAPHIC_PINSHAPE::OUTPUT_LOW ) /* IEEE symbol "Active Low Output" */
    {
        const int symbol_size = externalPinDecoSize( aPlotter->RenderSettings(), *this );

        if( MapY1 == 0 )        /* MapX1 = +- 1 */
        {
            aPlotter->MoveTo( VECTOR2I( x1, y1 - symbol_size * 2 ) );
            aPlotter->FinishTo( VECTOR2I( x1 + MapX1 * symbol_size * 2, y1 ) );
        }
        else    /* MapX1 = 0 */
        {
            aPlotter->MoveTo( VECTOR2I( x1 - symbol_size * 2, y1 ) );
            aPlotter->FinishTo( VECTOR2I( x1, y1 + MapY1 * symbol_size * 2 ) );
        }
    }
    else if( m_shape == GRAPHIC_PINSHAPE::NONLOGIC ) /* NonLogic pin symbol */
    {
        const int deco_size = externalPinDecoSize( aPlotter->RenderSettings(), *this );
        aPlotter->MoveTo( VECTOR2I( x1 - ( MapX1 + MapY1 ) * deco_size,
                                    y1 - ( MapY1 - MapX1 ) * deco_size ) );
        aPlotter->FinishTo( VECTOR2I( x1 + ( MapX1 + MapY1 ) * deco_size,
                                      y1 + ( MapY1 - MapX1 ) * deco_size ) );
        aPlotter->MoveTo( VECTOR2I( x1 - ( MapX1 - MapY1 ) * deco_size,
                                    y1 - ( MapY1 + MapX1 ) * deco_size ) );
        aPlotter->FinishTo( VECTOR2I( x1 + ( MapX1 - MapY1 ) * deco_size,
                                      y1 + ( MapY1 + MapX1 ) * deco_size ) );
    }

    if( m_type == ELECTRICAL_PINTYPE::PT_NC ) // Draw a N.C. symbol
    {
        const int deco_size = TARGET_PIN_RADIUS;
        const int ex1 = aPosition.x;
        const int ey1 = aPosition.y;
        aPlotter->MoveTo( VECTOR2I( ex1 - deco_size, ey1 - deco_size ) );
        aPlotter->FinishTo( VECTOR2I( ex1 + deco_size, ey1 + deco_size ) );
        aPlotter->MoveTo( VECTOR2I( ex1 + deco_size, ey1 - deco_size ) );
        aPlotter->FinishTo( VECTOR2I( ex1 - deco_size, ey1 + deco_size ) );
    }
}


void SCH_PIN::PlotPinTexts( PLOTTER *aPlotter, const VECTOR2I &aPinPos, PIN_ORIENTATION aPinOrient,
                            int aTextInside, bool aDrawPinNum, bool aDrawPinName,
                            bool aDimmed ) const
{
    RENDER_SETTINGS* settings = aPlotter->RenderSettings();
    KIFONT::FONT*    font = KIFONT::FONT::GetFont( settings->GetDefaultFont(), false, false );
    wxString         name = GetShownName();
    wxString         number = GetShownNumber();

    if( name.IsEmpty() || m_nameTextSize == 0 )
        aDrawPinName = false;

    if( number.IsEmpty() || m_numTextSize == 0 )
        aDrawPinNum = false;

    if( !aDrawPinNum && !aDrawPinName )
        return;

    int namePenWidth = settings->GetDefaultPenWidth();
    int numPenWidth  = settings->GetDefaultPenWidth();
    int name_offset = schIUScale.MilsToIU( PIN_TEXT_MARGIN ) + namePenWidth;
    int num_offset  = schIUScale.MilsToIU( PIN_TEXT_MARGIN ) + numPenWidth;

    /* Get the num and name colors */
    COLOR4D nameColor = settings->GetLayerColor( LAYER_PINNAM );
    COLOR4D numColor  = settings->GetLayerColor( LAYER_PINNUM );
    COLOR4D bg = settings->GetBackgroundColor();

    if( bg == COLOR4D::UNSPECIFIED || !aPlotter->GetColorMode() )
        bg = COLOR4D::WHITE;

    if( aDimmed )
    {
        nameColor.Desaturate( );
        numColor.Desaturate( );
        nameColor = nameColor.Mix( bg, 0.5f );
        numColor = numColor.Mix( bg, 0.5f );
    }

    int x1 = aPinPos.x;
    int y1 = aPinPos.y;

    switch( aPinOrient )
    {
    case PIN_ORIENTATION::PIN_UP:    y1 -= GetLength();                                   break;
    case PIN_ORIENTATION::PIN_DOWN:  y1 += GetLength();                                   break;
    case PIN_ORIENTATION::PIN_LEFT:  x1 -= GetLength();                                   break;
    case PIN_ORIENTATION::PIN_RIGHT: x1 += GetLength();                                   break;
    case PIN_ORIENTATION::INHERIT:   wxFAIL_MSG( wxS( "aPinOrient must be resolved!" ) ); break;
    }

    auto plotName =
            [&]( int x, int y, const EDA_ANGLE& angle, GR_TEXT_H_ALIGN_T hJustify,
                 GR_TEXT_V_ALIGN_T vJustify )
            {
                TEXT_ATTRIBUTES attrs;
                attrs.m_StrokeWidth = namePenWidth;
                attrs.m_Angle = angle;
                attrs.m_Size = VECTOR2I( GetNameTextSize(), GetNameTextSize() );
                attrs.m_Halign = hJustify;
                attrs.m_Valign = vJustify;
                attrs.m_Multiline = false;

                aPlotter->PlotText( VECTOR2I( x, y ), nameColor, name, attrs, font, GetFontMetrics() );
            };

    auto plotNum =
            [&]( int x, int y, const EDA_ANGLE& angle, GR_TEXT_H_ALIGN_T hJustify,
                 GR_TEXT_V_ALIGN_T vJustify )
            {
                TEXT_ATTRIBUTES attrs;
                attrs.m_StrokeWidth = numPenWidth;
                attrs.m_Angle = angle;
                attrs.m_Size = VECTOR2I( GetNumberTextSize(), GetNumberTextSize() );
                attrs.m_Halign = hJustify;
                attrs.m_Valign = vJustify;
                attrs.m_Multiline = false;

                aPlotter->PlotText( VECTOR2I( x, y ), numColor, number, attrs, font, GetFontMetrics() );
            };

    /* Draw the text inside, but the pin numbers outside. */
    if( aTextInside )
    {
        if( ( aPinOrient == PIN_ORIENTATION::PIN_LEFT )
            || ( aPinOrient == PIN_ORIENTATION::PIN_RIGHT ) ) /* Its an horizontal line. */
        {
            if( aDrawPinName )
            {
                if( aPinOrient == PIN_ORIENTATION::PIN_RIGHT )
                {
                    plotName( x1 + aTextInside, y1, ANGLE_HORIZONTAL,
                              GR_TEXT_H_ALIGN_LEFT, GR_TEXT_V_ALIGN_CENTER );
                }
                else    // orient == PIN_LEFT
                {
                    plotName( x1 - aTextInside, y1, ANGLE_HORIZONTAL,
                              GR_TEXT_H_ALIGN_RIGHT, GR_TEXT_V_ALIGN_CENTER );
                }
            }

            if( aDrawPinNum )
            {
                plotNum( ( x1 + aPinPos.x) / 2, y1 - num_offset, ANGLE_HORIZONTAL,
                         GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_BOTTOM );
            }
        }
        else         /* Its a vertical line. */
        {
            if( aPinOrient == PIN_ORIENTATION::PIN_DOWN )
            {
                if( aDrawPinName )
                {
                    plotName( x1, y1 + aTextInside, ANGLE_VERTICAL,
                              GR_TEXT_H_ALIGN_RIGHT, GR_TEXT_V_ALIGN_CENTER );
                }

                if( aDrawPinNum )
                {
                    plotNum( x1 - num_offset, ( y1 + aPinPos.y) / 2, ANGLE_VERTICAL,
                             GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_BOTTOM );
                }
            }
            else        /* PIN_UP */
            {
                if( aDrawPinName )
                {
                    plotName( x1, y1 - aTextInside, ANGLE_VERTICAL,
                              GR_TEXT_H_ALIGN_LEFT, GR_TEXT_V_ALIGN_CENTER );
                }

                if( aDrawPinNum )
                {
                    plotNum( x1 - num_offset, ( y1 + aPinPos.y) / 2, ANGLE_VERTICAL,
                             GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_BOTTOM );
                }
            }
        }
    }
    else     /* Draw num & text pin outside */
    {
        if( ( aPinOrient == PIN_ORIENTATION::PIN_LEFT )
            || ( aPinOrient == PIN_ORIENTATION::PIN_RIGHT ) )
        {
            /* Its an horizontal line. */
            if( aDrawPinName && aDrawPinNum )
            {
                plotName( ( x1 + aPinPos.x) / 2, y1 - name_offset, ANGLE_HORIZONTAL,
                          GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_BOTTOM );

                plotNum( ( x1 + aPinPos.x) / 2, y1 + num_offset, ANGLE_HORIZONTAL,
                          GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_TOP );
            }
            else if( aDrawPinName )
            {
                plotName( ( x1 + aPinPos.x) / 2, y1 - name_offset, ANGLE_HORIZONTAL,
                          GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_BOTTOM );
            }
            else if( aDrawPinNum )
            {
                plotNum( ( x1 + aPinPos.x) / 2, y1 - name_offset, ANGLE_HORIZONTAL,
                          GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_BOTTOM );
            }
        }
        else
        {
            /* Its a vertical line. */
            if( aDrawPinName && aDrawPinNum )
            {
                plotName( x1 - name_offset, ( y1 + aPinPos.y ) / 2, ANGLE_VERTICAL,
                          GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_BOTTOM );

                plotNum( x1 + num_offset, ( y1 + aPinPos.y ) / 2, ANGLE_VERTICAL,
                          GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_TOP );
            }
            else if( aDrawPinName )
            {
                plotName( x1 - name_offset, ( y1 + aPinPos.y ) / 2, ANGLE_VERTICAL,
                          GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_BOTTOM );
            }
            else if( aDrawPinNum )
            {
                plotNum( x1 - num_offset, ( y1 + aPinPos.y ) / 2, ANGLE_VERTICAL,
                          GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_BOTTOM );
            }
        }
    }
}


PIN_ORIENTATION SCH_PIN::PinDrawOrient( const TRANSFORM& aTransform ) const
{
    PIN_ORIENTATION orient;
    VECTOR2I end; // position of pin end starting at 0,0 according to its orientation, length = 1

    switch( GetOrientation() )
    {
    default:
    case PIN_ORIENTATION::PIN_RIGHT:  end.x = 1;   break;
    case PIN_ORIENTATION::PIN_UP:     end.y = -1;  break;
    case PIN_ORIENTATION::PIN_DOWN:   end.y = 1;   break;
    case PIN_ORIENTATION::PIN_LEFT:   end.x = -1;  break;
    }

    // = pos of end point, according to the symbol orientation.
    end    = aTransform.TransformCoordinate( end );
    orient = PIN_ORIENTATION::PIN_UP;

    if( end.x == 0 )
    {
        if( end.y > 0 )
            orient = PIN_ORIENTATION::PIN_DOWN;
    }
    else
    {
        orient = PIN_ORIENTATION::PIN_RIGHT;

        if( end.x < 0 )
            orient = PIN_ORIENTATION::PIN_LEFT;
    }

    return orient;
}


EDA_ITEM* SCH_PIN::Clone() const
{
    //return new SCH_PIN( *this );
    SCH_ITEM* newPin = new SCH_PIN( *this );
    wxASSERT( newPin->GetUnit() == m_unit && newPin->GetBodyStyle() == m_bodyStyle );
    return newPin;
}


void SCH_PIN::ChangeLength( int aLength )
{
    int lengthChange = GetLength() - aLength;
    int offsetX = 0;
    int offsetY = 0;

    switch( m_orientation )
    {
    default:
    case PIN_ORIENTATION::PIN_RIGHT:
        offsetX = lengthChange;
        break;
    case PIN_ORIENTATION::PIN_LEFT:
        offsetX = -1 * lengthChange;
        break;
    case PIN_ORIENTATION::PIN_UP:
        offsetY = lengthChange;
        break;
    case PIN_ORIENTATION::PIN_DOWN:
        offsetY = -1 * lengthChange;
        break;
    }

    m_position += VECTOR2I( offsetX, offsetY );
    m_length = aLength;
}


void SCH_PIN::Move( const VECTOR2I& aOffset )
{
    m_position += aOffset;
}


void SCH_PIN::MirrorHorizontallyPin( int aCenter )
{
    m_position.x -= aCenter;
    m_position.x *= -1;
    m_position.x += aCenter;

    if( m_orientation == PIN_ORIENTATION::PIN_RIGHT )
        m_orientation = PIN_ORIENTATION::PIN_LEFT;
    else if( m_orientation == PIN_ORIENTATION::PIN_LEFT )
        m_orientation = PIN_ORIENTATION::PIN_RIGHT;
}


void SCH_PIN::MirrorHorizontally( int aCenter )
{
    if( dynamic_cast<LIB_SYMBOL*>( GetParentSymbol() ) )
        MirrorHorizontallyPin( aCenter );
}


void SCH_PIN::MirrorVerticallyPin( int aCenter )
{
    m_position.y -= aCenter;
    m_position.y *= -1;
    m_position.y += aCenter;

    if( m_orientation == PIN_ORIENTATION::PIN_UP )
        m_orientation = PIN_ORIENTATION::PIN_DOWN;
    else if( m_orientation == PIN_ORIENTATION::PIN_DOWN )
        m_orientation = PIN_ORIENTATION::PIN_UP;
}


void SCH_PIN::MirrorVertically( int aCenter )
{
    if( dynamic_cast<LIB_SYMBOL*>( GetParentSymbol() ) )
        MirrorVerticallyPin( aCenter );
}


void SCH_PIN::RotatePin( const VECTOR2I& aCenter, bool aRotateCCW )
{
    if( aRotateCCW )
    {
        RotatePoint( m_position, aCenter, ANGLE_90 );

        switch( GetOrientation() )
        {
        default:
        case PIN_ORIENTATION::PIN_RIGHT: m_orientation = PIN_ORIENTATION::PIN_UP;    break;
        case PIN_ORIENTATION::PIN_UP:    m_orientation = PIN_ORIENTATION::PIN_LEFT;  break;
        case PIN_ORIENTATION::PIN_LEFT:  m_orientation = PIN_ORIENTATION::PIN_DOWN;  break;
        case PIN_ORIENTATION::PIN_DOWN:  m_orientation = PIN_ORIENTATION::PIN_RIGHT; break;
        }
    }
    else
    {
        RotatePoint( m_position, aCenter, -ANGLE_90 );

        switch( GetOrientation() )
        {
        default:
        case PIN_ORIENTATION::PIN_RIGHT: m_orientation = PIN_ORIENTATION::PIN_DOWN;  break;
        case PIN_ORIENTATION::PIN_UP:    m_orientation = PIN_ORIENTATION::PIN_RIGHT; break;
        case PIN_ORIENTATION::PIN_LEFT:  m_orientation = PIN_ORIENTATION::PIN_UP;    break;
        case PIN_ORIENTATION::PIN_DOWN:  m_orientation = PIN_ORIENTATION::PIN_LEFT;  break;
        }
    }
}


void SCH_PIN::Rotate( const VECTOR2I& aCenter, bool aRotateCCW )
{
    if( dynamic_cast<LIB_SYMBOL*>( GetParentSymbol() ) )
        RotatePin( aCenter, aRotateCCW );
}


void SCH_PIN::Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts,
                    int aUnit, int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed )
{
    if( aBackground )
        return;

    SCH_RENDER_SETTINGS* renderSettings = getRenderSettings( aPlotter );

    if( !IsVisible() && !renderSettings->m_ShowHiddenPins )
        return;

    const SYMBOL*   part = GetParentSymbol();
    PIN_ORIENTATION orient = PinDrawOrient( renderSettings->m_Transform );
    VECTOR2I        pos = renderSettings->TransformCoordinate( m_position ) + aOffset;

    PlotPinType( aPlotter, pos, orient, aDimmed );
    PlotPinTexts( aPlotter, pos, orient, part->GetPinNameOffset(), part->GetShowPinNumbers(),
                  part->GetShowPinNames(), aDimmed );
}


void SCH_PIN::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString msg;
    SYMBOL*  symbol = GetParentSymbol();

    if( dynamic_cast<LIB_SYMBOL*>( symbol ) )
    {
        getSymbolEditorMsgPanelInfo( aFrame, aList );
    }
    else
    {
        aList.emplace_back( _( "Type" ), _( "Pin" ) );

        if( symbol->GetUnitCount() )
        {
            msg = m_libPin ? GetUnitDescription( m_libPin->GetUnit() ) :
                             wxString( "Undefined library pin." );
            aList.emplace_back( _( "Unit" ), msg );
        }

        if( symbol->HasAlternateBodyStyle() )
        {
            msg = m_libPin ? GetBodyStyleDescription( m_libPin->GetBodyStyle() ) :
                             wxString( "Undefined library pin." );
            aList.emplace_back( _( "Body Style" ), msg );
        }
    }

    aList.emplace_back( _( "Name" ), UnescapeString( GetShownName() ) );
    aList.emplace_back( _( "Number" ), GetShownNumber() );
    aList.emplace_back( _( "Type" ), ElectricalPinTypeGetText( GetType() ) );
    aList.emplace_back( _( "Style" ), PinShapeGetText( GetShape() ) );

    aList.emplace_back( _( "Visible" ), IsVisible() ? _( "Yes" ) : _( "No" ) );

    // Display pin length
    aList.emplace_back( _( "Length" ), aFrame->MessageTextFromValue( GetLength(), true ) );

    aList.emplace_back( _( "Orientation" ), PinOrientationName( GetOrientation() ) );

    if( dynamic_cast<LIB_SYMBOL*>( symbol ) )
    {
        aList.emplace_back( _( "Pos X" ), aFrame->MessageTextFromValue( GetPosition().x, true ) );
        aList.emplace_back( _( "Pos Y" ), aFrame->MessageTextFromValue( GetPosition().y, true ) );
    }
    else
    {
        SCH_EDIT_FRAME* schframe = dynamic_cast<SCH_EDIT_FRAME*>( aFrame );
        SCH_SHEET_PATH* currentSheet = schframe ? &schframe->GetCurrentSheet() : nullptr;
        SCH_SYMBOL*     schsymbol = dynamic_cast<SCH_SYMBOL*>( symbol );

        // Don't use GetShownText(); we want to see the variable references here
        aList.emplace_back( symbol->GetRef( currentSheet ),
                            UnescapeString( schsymbol->GetField( VALUE_FIELD )->GetText() ) );
    }

#if defined(DEBUG)
    if( !IsConnectivityDirty() && dynamic_cast<SCH_EDIT_FRAME*>( aFrame ) )
    {
        SCH_CONNECTION* conn = Connection();

        if( conn )
            conn->AppendInfoToMsgPanel( aList );
    }
#endif
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
    const SCH_SYMBOL* symbol = static_cast<const SCH_SYMBOL*>( GetParentSymbol() );

    // Need to check for parent as power symbol to make sure we aren't dealing
    // with legacy global power pins on non-power symbols
    if( IsGlobalPower() )
    {
        if( GetLibPin()->GetParentSymbol()->IsPower() )
        {
            return EscapeString( symbol->GetValue( true, &aPath, false ), CTX_NETNAME );
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

    std::vector<SCH_PIN*> pins = symbol->GetPins( &aPath );
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
    if( symbol->GetRef( &aPath, false ).Last() == '?' )
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
        name << symbol->GetRef( &aPath, true );
        name << "-" << EscapeString( libPinShownName, CTX_NETNAME );

        if( unconnected || has_multiple )
            name << "-Pad" << EscapeString( libPinShownNumber, CTX_NETNAME );

        name << ")";
    }
    else
    {
        // Pin numbers are unique, so we skip the unit token
        name << symbol->GetRef( &aPath, false );
        name << "-Pad" << EscapeString( libPinShownNumber, CTX_NETNAME ) << ")";
    }

    if( annotated )
        m_net_name_map[ aPath ] = std::make_pair( name, aForceNoConnect );

    return name;
}


const BOX2I SCH_PIN::ViewBBox() const
{
    return GetBoundingBox( false, true, m_flags & SHOW_ELEC_TYPE );
}


void SCH_PIN::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount     = 6;
    aLayers[0] = LAYER_DANGLING;
    aLayers[1] = LAYER_DEVICE;
    aLayers[2] = LAYER_SELECTION_SHADOWS;
    aLayers[3] = LAYER_OP_CURRENTS;
    aLayers[4] = LAYER_PINNAM;
    aLayers[5] = LAYER_PINNUM;
}


void SCH_PIN::validateExtentsCache( KIFONT::FONT* aFont, int aSize, const wxString& aText,
                                    EXTENTS_CACHE* aCache ) const
{
    if( aCache->m_Font == aFont
        && aCache->m_FontSize == aSize
        && aCache->m_Extents != VECTOR2I() )
    {
        return;
    }

    aCache->m_Font = aFont;
    aCache->m_FontSize = aSize;

    VECTOR2D fontSize( aSize, aSize );
    int      penWidth = GetPenSizeForNormal( aSize );

    aCache->m_Extents = aFont->StringBoundaryLimits( aText, fontSize, penWidth, false, false,
                                                       GetFontMetrics() );
}


BOX2I SCH_PIN::GetBoundingBox( bool aIncludeLabelsOnInvisiblePins, bool aIncludeNameAndNumber,
                               bool aIncludeElectricalType ) const
{
    if( const SCH_SYMBOL* symbol = dynamic_cast<const SCH_SYMBOL*>( GetParentSymbol() ) )
    {
        wxCHECK( m_libPin, BOX2I() );

        BOX2I r = m_libPin->GetBoundingBox( aIncludeLabelsOnInvisiblePins, aIncludeNameAndNumber,
                                            aIncludeElectricalType );

        r = symbol->GetTransform().TransformCoordinate( r );
        r.Offset( symbol->GetPosition() );
        r.Normalize();

        return r;
    }

    EESCHEMA_SETTINGS* cfg = Pgm().GetSettingsManager().GetAppSettings<EESCHEMA_SETTINGS>();
    KIFONT::FONT*      font = KIFONT::FONT::GetFont( cfg->m_Appearance.default_font );

    BOX2I    bbox;
    VECTOR2I begin;
    VECTOR2I end;
    int      pinNameOffset = 0;
    int      nameTextLength = 0;
    int      nameTextHeight = 0;
    int      numberTextLength = 0;
    int      numberTextHeight = 0;
    int      typeTextLength = 0;
    bool     includeName = aIncludeNameAndNumber && !GetShownName().IsEmpty();
    bool     includeNumber = aIncludeNameAndNumber && !GetShownNumber().IsEmpty();
    bool     includeType = aIncludeElectricalType;
    int      minsizeV = TARGET_PIN_RADIUS;

    if( !aIncludeLabelsOnInvisiblePins && !IsVisible() )
    {
        includeName = false;
        includeNumber = false;
        includeType = false;
    }

    if( const SYMBOL* parentSymbol = GetParentSymbol() )
    {
        if( parentSymbol->GetShowPinNames() )
            pinNameOffset = parentSymbol->GetPinNameOffset();
        else
            includeName = false;

        if( !parentSymbol->GetShowPinNumbers() )
            includeNumber = false;
    }

    if( includeNumber )
    {
        validateExtentsCache( font, GetNumberTextSize(), GetShownNumber(), &m_numExtentsCache );
        numberTextLength = m_numExtentsCache.m_Extents.x;
        numberTextHeight = m_numExtentsCache.m_Extents.y;
    }

    if( includeName )
    {
        validateExtentsCache( font, GetNameTextSize(), GetShownName(), &m_nameExtentsCache );
        nameTextLength = m_nameExtentsCache.m_Extents.x + pinNameOffset;
        nameTextHeight = m_nameExtentsCache.m_Extents.y + schIUScale.MilsToIU( PIN_TEXT_MARGIN );
    }

    if( includeType )
    {
        double   fontSize = std::max( GetNameTextSize() * 3 / 4, schIUScale.mmToIU( 0.7 ) );
        double   stroke = fontSize / 8.0;
        VECTOR2I typeTextSize = font->StringBoundaryLimits( GetElectricalTypeName(),
                                                            VECTOR2D( fontSize, fontSize ),
                                                            KiROUND( stroke ), false, false,
                                                            GetFontMetrics() );

        typeTextLength = typeTextSize.x + schIUScale.MilsToIU( PIN_TEXT_MARGIN ) + TARGET_PIN_RADIUS;
        minsizeV = std::max( minsizeV, typeTextSize.y / 2 );
    }

    // First, calculate boundary box corners position
    if( m_shape == GRAPHIC_PINSHAPE::INVERTED || m_shape == GRAPHIC_PINSHAPE::INVERTED_CLOCK )
        minsizeV = std::max( TARGET_PIN_RADIUS, externalPinDecoSize( nullptr, *this ) );

    // Calculate topLeft & bottomRight corner positions for the default pin orientation (PIN_RIGHT)
    if( pinNameOffset || !includeName )
    {
        // pin name is inside the body (or invisible)
        // pin number is above the line
        begin.y = std::min( -minsizeV, -numberTextHeight );
        begin.x = std::min( -typeTextLength, GetLength() - ( numberTextLength / 2 ) );

        end.x = GetLength() + nameTextLength;
        end.y = std::max( minsizeV, nameTextHeight / 2 );
    }
    else
    {
        // pin name is above pin line
        // pin number is below line
        begin.y = std::min( -minsizeV, -nameTextHeight );
        begin.x = -typeTextLength;
        begin.x = std::min( begin.x, ( GetLength() - numberTextLength ) / 2 );
        begin.x = std::min( begin.x, ( GetLength() - nameTextLength ) / 2 );

        end.x = GetLength();
        end.x = std::max( end.x, ( GetLength() + nameTextLength ) / 2 );
        end.x = std::max( end.x, ( GetLength() + numberTextLength ) / 2 );
        end.y = std::max( minsizeV, numberTextHeight );
    }

    // Now, calculate boundary box corners position for the actual pin orientation
    switch( PinDrawOrient( DefaultTransform ) )
    {
    case PIN_ORIENTATION::PIN_UP:
        // Pin is rotated and texts positions are mirrored
        RotatePoint( begin, VECTOR2I( 0, 0 ), ANGLE_90 );
        RotatePoint( end, VECTOR2I( 0, 0 ), ANGLE_90 );
        break;

    case PIN_ORIENTATION::PIN_DOWN:
        RotatePoint( begin, VECTOR2I( 0, 0 ), -ANGLE_90 );
        RotatePoint( end, VECTOR2I( 0, 0 ), -ANGLE_90 );
        begin.x = -begin.x;
        end.x = -end.x;
        break;

    case PIN_ORIENTATION::PIN_LEFT:
        begin.x = -begin.x;
        end.x = -end.x;
        break;

    default:
    case PIN_ORIENTATION::PIN_RIGHT:
        break;
    }

    begin += m_position;
    end += m_position;

    bbox.SetOrigin( begin );
    bbox.SetEnd( end );
    bbox.Normalize();
    bbox.Inflate( ( GetPenWidth() / 2 ) + 1 );

    return bbox;
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

    if( GetNumber() != pin->GetNumber() )
        return true;

    return GetName() != pin->GetName();
}


bool SCH_PIN::ConnectionPropagatesTo( const EDA_ITEM* aItem ) const
{
    wxCHECK( m_libPin, false );

    // Reciprocal checking is done in CONNECTION_GRAPH anyway
    return m_libPin->GetType() != ELECTRICAL_PINTYPE::PT_NC;
}


BITMAPS SCH_PIN::GetMenuImage() const
{
    return ElectricalPinTypeGetBitmap( m_type );
}


wxString SCH_PIN::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, ALT* aAlt ) const
{
    return getItemDescription( aAlt );
}


wxString SCH_PIN::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const
{
    if( m_libPin )
    {
        SCH_PIN::ALT  localStorage;
        SCH_PIN::ALT* alt = nullptr;

        if( !m_alt.IsEmpty() )
        {
            localStorage = m_libPin->GetAlt( m_alt );
            alt = &localStorage;
        }

        wxString itemDesc = m_libPin ? m_libPin->GetItemDescription( aUnitsProvider, alt )
                                     : wxString( wxS( "Undefined library pin." ) );

        const SCH_SYMBOL* symbol = static_cast<const SCH_SYMBOL*>( GetParentSymbol() );

        return wxString::Format( "Symbol %s %s",
                                 UnescapeString( symbol->GetField( REFERENCE_FIELD )->GetText() ),
                                 itemDesc );
    }

    return getItemDescription( nullptr );
}


wxString SCH_PIN::getItemDescription( ALT* aAlt ) const
{
    // This code previously checked "m_name.IsEmpty()" to choose the correct
    // formatting path, but that check fails if the pin is called "~" which is
    // the default for an empty pin name.  Instead we get the final display string
    // that will be shown and check if it's empty.

    wxString name = UnescapeString( aAlt ? aAlt->m_Name : GetShownName() );
    wxString electricalTypeName = ElectricalPinTypeGetText( aAlt ? aAlt->m_Type : m_type );
    wxString pinShapeName = PinShapeGetText( aAlt ? aAlt->m_Shape : m_shape );

    if( IsVisible() )
    {
        if ( !name.IsEmpty() )
        {
            return wxString::Format( _( "Pin %s [%s, %s, %s]" ),
                                     GetShownNumber(),
                                     name,
                                     electricalTypeName,
                                     pinShapeName );
        }
        else
        {
            return wxString::Format( _( "Pin %s [%s, %s]" ),
                                     GetShownNumber(),
                                     electricalTypeName,
                                     pinShapeName );
        }
    }
    else
    {
        if( !name.IsEmpty() )
        {
            return wxString::Format( _( "Hidden pin %s [%s, %s, %s]" ),
                                     GetShownNumber(),
                                     name,
                                     electricalTypeName,
                                     pinShapeName );
        }
        else
        {
            return wxString::Format( _( "Hidden pin %s [%s, %s]" ),
                                     GetShownNumber(),
                                     electricalTypeName,
                                     pinShapeName );
        }
    }
}




int SCH_PIN::compare( const SCH_ITEM& aOther, int aCompareFlags ) const
{
    // Ignore the UUID here.
    int retv = SCH_ITEM::compare( aOther, aCompareFlags | SCH_ITEM::COMPARE_FLAGS::EQUALITY );

    if( retv )
        return retv;

    const SCH_PIN* tmp = static_cast<const SCH_PIN*>( &aOther );

    wxCHECK( tmp, -1 );

    // When comparing units, we do not compare the part numbers.  If everything else is
    // identical, then we can just renumber the parts for the inherited symbol.
    // if( !( aCompareFlags & SCH_ITEM::COMPARE_FLAGS::UNIT ) && m_number != tmp->m_number )
    //     return m_number.Cmp( tmp->m_number );

    // int result = m_name.Cmp( tmp->m_name );

    // if( result )
    //     return result;

    // if( m_position.x != tmp->m_position.x )
    //     return m_position.x - tmp->m_position.x;

    // if( m_position.y != tmp->m_position.y )
    //     return m_position.y - tmp->m_position.y;

    // if( m_length != tmp->m_length )
    //     return m_length.value_or( 0 ) - tmp->m_length.value_or( 0 );

    // if( m_orientation != tmp->m_orientation )
    //     return static_cast<int>( m_orientation ) - static_cast<int>( tmp->m_orientation );

    // if( m_shape != tmp->m_shape )
    //     return static_cast<int>( m_shape ) - static_cast<int>( tmp->m_shape );

    // if( m_type != tmp->m_type )
    //     return static_cast<int>( m_type ) - static_cast<int>( tmp->m_type );

    // if( m_hidden != tmp->m_hidden )
    //     return m_hidden.value_or( false ) - tmp->m_hidden.value_or( false );

    // if( m_numTextSize != tmp->m_numTextSize )
    //     return m_numTextSize.value_or( 0 ) - tmp->m_numTextSize.value_or( 0 );

    // if( m_nameTextSize != tmp->m_nameTextSize )
    //     return m_nameTextSize.value_or( 0 ) - tmp->m_nameTextSize.value_or( 0 );

    // if( m_alternates.size() != tmp->m_alternates.size() )
    //     return static_cast<int>( m_alternates.size() - tmp->m_alternates.size() );

    // auto lhsItem = m_alternates.begin();
    // auto rhsItem = tmp->m_alternates.begin();

    // while( lhsItem != m_alternates.end() )
    // {
    //     const ALT& lhsAlt = lhsItem->second;
    //     const ALT& rhsAlt = rhsItem->second;

    //     int retv = lhsAlt.m_Name.Cmp( rhsAlt.m_Name );

    //     if( retv )
    //         return retv;

    //     if( lhsAlt.m_Type != rhsAlt.m_Type )
    //         return static_cast<int>( lhsAlt.m_Type ) - static_cast<int>( rhsAlt.m_Type );

    //     if( lhsAlt.m_Shape != rhsAlt.m_Shape )
    //         return static_cast<int>( lhsAlt.m_Shape ) - static_cast<int>( rhsAlt.m_Shape );

    //     ++lhsItem;
    //     ++rhsItem;
    // }

    if( m_number != tmp->m_number )
        return m_number.Cmp( tmp->m_number );

    if( m_position.x != tmp->m_position.x )
        return m_position.x - tmp->m_position.x;

    if( m_position.y != tmp->m_position.y )
        return m_position.y - tmp->m_position.y;

    if( dynamic_cast<const SCH_SYMBOL*>( GetParentSymbol() ) )
    {
        wxCHECK( m_libPin && tmp->m_libPin, -1 );

        retv = m_libPin->compare( *tmp->m_libPin );

        if( retv )
            return retv;

        retv = m_alt.Cmp( tmp->m_alt );

        if( retv )
            return retv;
    }

    if( dynamic_cast<const LIB_SYMBOL*>( GetParentSymbol() ) )
    {
        if( m_length != tmp->m_length )
            return m_length.value_or( 0 ) - tmp->m_length.value_or( 0 );

        if( m_orientation != tmp->m_orientation )
            return static_cast<int>( m_orientation ) - static_cast<int>( tmp->m_orientation );

        if( m_shape != tmp->m_shape )
            return static_cast<int>( m_shape ) - static_cast<int>( tmp->m_shape );

        if( m_type != tmp->m_type )
            return static_cast<int>( m_type ) - static_cast<int>( tmp->m_type );

        if( m_hidden != tmp->m_hidden )
            return m_hidden.value_or( false ) - tmp->m_hidden.value_or( false );

        if( m_numTextSize != tmp->m_numTextSize )
            return m_numTextSize.value_or( 0 ) - tmp->m_numTextSize.value_or( 0 );

        if( m_nameTextSize != tmp->m_nameTextSize )
            return m_nameTextSize.value_or( 0 ) - tmp->m_nameTextSize.value_or( 0 );

        if( m_alternates.size() != tmp->m_alternates.size() )
            return static_cast<int>( m_alternates.size() - tmp->m_alternates.size() );

        auto lhsItem = m_alternates.begin();
        auto rhsItem = tmp->m_alternates.begin();

        while( lhsItem != m_alternates.end() )
        {
            const ALT& lhsAlt = lhsItem->second;
            const ALT& rhsAlt = rhsItem->second;

            retv = lhsAlt.m_Name.Cmp( rhsAlt.m_Name );

            if( retv )
                return retv;

            if( lhsAlt.m_Type != rhsAlt.m_Type )
                return static_cast<int>( lhsAlt.m_Type ) - static_cast<int>( rhsAlt.m_Type );

            if( lhsAlt.m_Shape != rhsAlt.m_Shape )
                return static_cast<int>( lhsAlt.m_Shape ) - static_cast<int>( rhsAlt.m_Shape );

            ++lhsItem;
            ++rhsItem;
        }
    }

    return 0;
}


double SCH_PIN::Similarity( const SCH_ITEM& aOther ) const
{
    if( aOther.m_Uuid == m_Uuid )
        return 1.0;

    if( aOther.Type() != SCH_PIN_T )
        return 0.0;

    const SCH_PIN* other = static_cast<const SCH_PIN*>( &aOther );

    if( m_libPin )
    {
        if( m_number != other->m_number )
            return 0.0;

        if( m_position != other->m_position )
            return 0.0;

        return m_libPin->Similarity( *other->m_libPin );
    }

    double similarity = SimilarityBase( aOther );

    if( m_name != other->m_name )
        similarity *= 0.9;

    if( m_number != other->m_number )
        similarity *= 0.9;

    if( m_position != other->m_position )
        similarity *= 0.9;

    if( m_length != other->m_length )
        similarity *= 0.9;

    if( m_orientation != other->m_orientation )
        similarity *= 0.9;

    if( m_shape != other->m_shape )
        similarity *= 0.9;

    if( m_type != other->m_type )
        similarity *= 0.9;

    if( m_hidden != other->m_hidden )
        similarity *= 0.9;

    if( m_numTextSize != other->m_numTextSize )
        similarity *= 0.9;

    if( m_nameTextSize != other->m_nameTextSize )
        similarity *= 0.9;

    if( m_alternates.size() != other->m_alternates.size() )
        similarity *= 0.9;

    return similarity;
}


std::ostream& SCH_PIN::operator<<( std::ostream& aStream )
{
    aStream << "SCH_PIN:" << std::endl
            << "  Name: \"" << m_name << "\"" << std::endl
            << "  Number: \"" << m_number << "\"" << std::endl
            << "  Position: " << m_position << std::endl
            << "  Length: " << GetLength() << std::endl
            << "  Orientation: " << PinOrientationName( m_orientation ) << std::endl
            << "  Shape: " << PinShapeGetText( m_shape ) << std::endl
            << "  Type: " << ElectricalPinTypeGetText( m_type ) << std::endl
            << "  Name Text Size: " << GetNameTextSize() << std::endl
            << "  Number Text Size: " << GetNumberTextSize() << std::endl;

    return aStream;
}


#if defined(DEBUG)

void SCH_PIN::Show( int nestLevel, std::ostream& os ) const
{
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str()
                                 << " num=\"" << m_number.mb_str()
                                 << '"' << "/>\n";

//    NestedSpace( nestLevel, os ) << "</" << GetClass().Lower().mb_str() << ">\n";
}

#endif

void SCH_PIN::CalcEdit( const VECTOR2I& aPosition )
{
    if( IsMoving() )
        SetPosition( aPosition );
}


static struct SCH_PIN_DESC
{
    SCH_PIN_DESC()
    {
        auto& pinTypeEnum = ENUM_MAP<ELECTRICAL_PINTYPE>::Instance();

        if( pinTypeEnum.Choices().GetCount() == 0 )
        {
            pinTypeEnum.Map( ELECTRICAL_PINTYPE::PT_INPUT,         _HKI( "Input" ) )
                       .Map( ELECTRICAL_PINTYPE::PT_OUTPUT,        _HKI( "Output" ) )
                       .Map( ELECTRICAL_PINTYPE::PT_BIDI,          _HKI( "Bidirectional" ) )
                       .Map( ELECTRICAL_PINTYPE::PT_TRISTATE,      _HKI( "Tri-state" ) )
                       .Map( ELECTRICAL_PINTYPE::PT_PASSIVE,       _HKI( "Passive" ) )
                       .Map( ELECTRICAL_PINTYPE::PT_NIC,           _HKI( "Free" ) )
                       .Map( ELECTRICAL_PINTYPE::PT_UNSPECIFIED,   _HKI( "Unspecified" ) )
                       .Map( ELECTRICAL_PINTYPE::PT_POWER_IN,      _HKI( "Power input" ) )
                       .Map( ELECTRICAL_PINTYPE::PT_POWER_OUT,     _HKI( "Power output" ) )
                       .Map( ELECTRICAL_PINTYPE::PT_OPENCOLLECTOR, _HKI( "Open collector" ) )
                       .Map( ELECTRICAL_PINTYPE::PT_OPENEMITTER,   _HKI( "Open emitter" ) )
                       .Map( ELECTRICAL_PINTYPE::PT_NC,            _HKI( "Unconnected" ) );
        }

        auto& pinShapeEnum = ENUM_MAP<GRAPHIC_PINSHAPE>::Instance();

        if( pinShapeEnum.Choices().GetCount() == 0 )
        {
            pinShapeEnum.Map( GRAPHIC_PINSHAPE::LINE,               _HKI( "Line" ) )
                        .Map( GRAPHIC_PINSHAPE::INVERTED,           _HKI( "Inverted" ) )
                        .Map( GRAPHIC_PINSHAPE::CLOCK,              _HKI( "Clock" ) )
                        .Map( GRAPHIC_PINSHAPE::INVERTED_CLOCK,     _HKI( "Inverted clock" ) )
                        .Map( GRAPHIC_PINSHAPE::INPUT_LOW,          _HKI( "Input low" ) )
                        .Map( GRAPHIC_PINSHAPE::CLOCK_LOW,          _HKI( "Clock low" ) )
                        .Map( GRAPHIC_PINSHAPE::OUTPUT_LOW,         _HKI( "Output low" ) )
                        .Map( GRAPHIC_PINSHAPE::FALLING_EDGE_CLOCK, _HKI( "Falling edge clock" ) )
                        .Map( GRAPHIC_PINSHAPE::NONLOGIC,           _HKI( "NonLogic" ) );
        }

        auto& orientationEnum = ENUM_MAP<PIN_ORIENTATION>::Instance();

        if( orientationEnum.Choices().GetCount() == 0 )
        {
            orientationEnum.Map( PIN_ORIENTATION::PIN_RIGHT, _( "Right" ) )
                           .Map( PIN_ORIENTATION::PIN_LEFT,  _( "Left" ) )
                           .Map( PIN_ORIENTATION::PIN_UP,    _( "Up" ) )
                           .Map( PIN_ORIENTATION::PIN_DOWN,  _( "Down" ) );
        }

        auto isSymbolEditor =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( SCH_PIN* pin = dynamic_cast<SCH_PIN*>( aItem ) )
                        return dynamic_cast<LIB_SYMBOL*>( pin->GetParentSymbol() ) != nullptr;

                    return false;
                };

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( SCH_PIN );
        propMgr.AddTypeCast( new TYPE_CAST<SCH_PIN, SCH_ITEM> );
        propMgr.InheritsAfter( TYPE_HASH( SCH_PIN ), TYPE_HASH( SCH_ITEM ) );

        propMgr.AddProperty( new PROPERTY<SCH_PIN, wxString>( _HKI( "Pin Name" ),
                    &SCH_PIN::SetName, &SCH_PIN::GetName ) )
                .SetWriteableFunc( isSymbolEditor );

        propMgr.AddProperty( new PROPERTY<SCH_PIN, wxString>( _HKI( "Pin Number" ),
                    &SCH_PIN::SetNumber, &SCH_PIN::GetNumber ) )
                .SetWriteableFunc( isSymbolEditor );

        propMgr.AddProperty( new PROPERTY_ENUM<SCH_PIN, ELECTRICAL_PINTYPE>(
                    _HKI( "Electrical Type" ),
                    &SCH_PIN::SetType, &SCH_PIN::GetType ) )
                .SetWriteableFunc( isSymbolEditor );

        propMgr.AddProperty( new PROPERTY_ENUM<SCH_PIN, GRAPHIC_PINSHAPE>(
                    _HKI( "Graphic Style" ),
                    &SCH_PIN::SetShape, &SCH_PIN::GetShape ) )
                .SetWriteableFunc( isSymbolEditor );

        propMgr.AddProperty( new PROPERTY<SCH_PIN, int>( _HKI( "Position X" ),
                    &SCH_PIN::SetX, &SCH_PIN::GetX, PROPERTY_DISPLAY::PT_COORD ) )
                .SetAvailableFunc( isSymbolEditor );

        propMgr.AddProperty( new PROPERTY<SCH_PIN, int>( _HKI( "Position Y" ),
                    &SCH_PIN::SetY, &SCH_PIN::GetY, PROPERTY_DISPLAY::PT_COORD ) )
                .SetAvailableFunc( isSymbolEditor );

        propMgr.AddProperty( new PROPERTY_ENUM<SCH_PIN, PIN_ORIENTATION>( _HKI( "Orientation" ),
                    &SCH_PIN::SetOrientation, &SCH_PIN::GetOrientation ) )
                .SetWriteableFunc( isSymbolEditor );

        propMgr.AddProperty( new PROPERTY<SCH_PIN, int>( _HKI( "Length" ),
                    &SCH_PIN::SetLength, &SCH_PIN::GetLength,
                    PROPERTY_DISPLAY::PT_SIZE ) )
                .SetWriteableFunc( isSymbolEditor );

        propMgr.AddProperty( new PROPERTY<SCH_PIN, int>( _HKI( "Name Text Size" ),
                    &SCH_PIN::SetNameTextSize, &SCH_PIN::GetNameTextSize,
                    PROPERTY_DISPLAY::PT_SIZE ) )
                .SetAvailableFunc( isSymbolEditor );

        propMgr.AddProperty( new PROPERTY<SCH_PIN, int>( _HKI( "Number Text Size" ),
                    &SCH_PIN::SetNumberTextSize, &SCH_PIN::GetNumberTextSize,
                    PROPERTY_DISPLAY::PT_SIZE ) )
                .SetAvailableFunc( isSymbolEditor );

        propMgr.AddProperty( new PROPERTY<SCH_PIN, bool>( _HKI( "Visible" ),
                    &SCH_PIN::SetVisible, &SCH_PIN::IsVisible ) )
                .SetAvailableFunc( isSymbolEditor );

    }
} _SCH_PIN_DESC;


ENUM_TO_WXANY( PIN_ORIENTATION )
ENUM_TO_WXANY( GRAPHIC_PINSHAPE )
ENUM_TO_WXANY( ELECTRICAL_PINTYPE )
