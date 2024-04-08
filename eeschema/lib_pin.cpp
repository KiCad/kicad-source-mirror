/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2024 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <lib_pin.h>
#include <settings/settings_manager.h>
#include <symbol_editor/symbol_editor_settings.h>
#include <trigo.h>
#include <string_utils.h>
#include "sch_painter.h"
#include "plotters/plotter.h"


// small margin in internal units between the pin text and the pin line
#define PIN_TEXT_MARGIN 4

const wxString LIB_PIN::GetCanonicalElectricalTypeName( ELECTRICAL_PINTYPE aType )
{
    // These strings are the canonical name of the electrictal type
    // Not translated, no space in name, only ASCII chars.
    // to use when the string name must be known and well defined
    // must have same order than enum ELECTRICAL_PINTYPE (see lib_pin.h)
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

static int internalPinDecoSize( const RENDER_SETTINGS* aSettings, const LIB_PIN &aPin )
{
    const SCH_RENDER_SETTINGS* settings = static_cast<const SCH_RENDER_SETTINGS*>( aSettings );

    if( settings && settings->m_PinSymbolSize )
        return settings->m_PinSymbolSize;

    return aPin.GetNameTextSize() != 0 ? aPin.GetNameTextSize() / 2 : aPin.GetNumberTextSize() / 2;
}

/// Utility for getting the size of the 'external' pin decorators (as a radius)
// i.e. the negation circle, the polarity 'slopes' and the nonlogic
// marker
static int externalPinDecoSize( const RENDER_SETTINGS* aSettings, const LIB_PIN &aPin )
{
    const SCH_RENDER_SETTINGS* settings = static_cast<const SCH_RENDER_SETTINGS*>( aSettings );

    if( settings && settings->m_PinSymbolSize )
        return settings->m_PinSymbolSize;

    return aPin.GetNumberTextSize() / 2;
}


LIB_PIN::LIB_PIN( SCH_ITEM* aParent ) :
        SCH_ITEM( aParent, LIB_PIN_T, 0, 0 ),
        m_orientation( PIN_ORIENTATION::PIN_RIGHT ),
        m_shape( GRAPHIC_PINSHAPE::LINE ),
        m_type( ELECTRICAL_PINTYPE::PT_UNSPECIFIED ),
        m_attributes( 0 )
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
}


LIB_PIN::LIB_PIN( SCH_ITEM* aParent, const wxString& aName, const wxString& aNumber,
                  PIN_ORIENTATION aOrientation, ELECTRICAL_PINTYPE aPinType, int aLength,
                  int aNameTextSize, int aNumTextSize, int aBodyStyle, const VECTOR2I& aPos,
                  int aUnit ) :
        SCH_ITEM( aParent, LIB_PIN_T, aUnit, aBodyStyle ),
        m_position( aPos ),
        m_length( aLength ),
        m_orientation( aOrientation ),
        m_shape( GRAPHIC_PINSHAPE::LINE ),
        m_type( aPinType ),
        m_attributes( 0 ),
        m_numTextSize( aNumTextSize ),
        m_nameTextSize( aNameTextSize )
{
    SetName( aName );
    SetNumber( aNumber );
}


bool LIB_PIN::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    BOX2I rect = GetBoundingBox( false, true, m_flags & SHOW_ELEC_TYPE );

    return rect.Inflate( aAccuracy ).Contains( aPosition );
}


bool LIB_PIN::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
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


int LIB_PIN::GetPenWidth() const
{
    return 0;
}


wxString LIB_PIN::GetShownName() const
{
    if( m_name == wxS( "~" ) )
        return wxEmptyString;
    else
        return m_name;
}


VECTOR2I LIB_PIN::GetPinRoot() const
{
    switch( m_orientation )
    {
    default:
    case PIN_ORIENTATION::PIN_RIGHT: return VECTOR2I( m_position.x + m_length, -( m_position.y ) );
    case PIN_ORIENTATION::PIN_LEFT:  return VECTOR2I( m_position.x - m_length, -( m_position.y ) );
    case PIN_ORIENTATION::PIN_UP:    return VECTOR2I( m_position.x, -( m_position.y + m_length ) );
    case PIN_ORIENTATION::PIN_DOWN:  return VECTOR2I( m_position.x, -( m_position.y - m_length ) );
    }
}


void LIB_PIN::Print( const SCH_RENDER_SETTINGS* aSettings, int aUnit, int aBodyStyle,
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


void LIB_PIN::printPinSymbol( const SCH_RENDER_SETTINGS* aSettings, const VECTOR2I& aPos,
                              PIN_ORIENTATION aOrient, bool aDimmed )
{
    wxDC*   DC = aSettings->GetPrintDC();
    int     MapX1, MapY1, x1, y1;
    int     width = GetEffectivePenWidth( aSettings );
    int     posX = aPos.x, posY = aPos.y, len = m_length;
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

    switch( aOrient )
    {
    case PIN_ORIENTATION::PIN_UP:     y1 = posY - len;  MapY1 = 1;   break;
    case PIN_ORIENTATION::PIN_DOWN:   y1 = posY + len;  MapY1 = -1;  break;
    case PIN_ORIENTATION::PIN_LEFT:   x1 = posX - len;  MapX1 = 1;   break;
    case PIN_ORIENTATION::PIN_RIGHT:  x1 = posX + len;  MapX1 = -1;  break;
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


void LIB_PIN::printPinTexts( const RENDER_SETTINGS* aSettings, VECTOR2I& aPinPos,
                             PIN_ORIENTATION aPinOrient, int aTextInside, bool aDrawPinNum,
                             bool aDrawPinName, bool aDimmed )
{
    if( !aDrawPinName && !aDrawPinNum )
        return;

    KIFONT::FONT* font = KIFONT::FONT::GetFont( aSettings->GetDefaultFont(), false, false );
    wxString      name = GetShownName();
    wxString      number = GetShownNumber();
    VECTOR2I      nameSize( m_nameTextSize, m_nameTextSize );
    VECTOR2I      numSize( m_numTextSize, m_numTextSize );
    int           nameWidth = std::max( Clamp_Text_PenSize( GetPenWidth(), m_nameTextSize, true ),
                                           aSettings->GetDefaultPenWidth() );
    int           numWidth = std::max( Clamp_Text_PenSize( GetPenWidth(), m_numTextSize, true ),
                                          aSettings->GetDefaultPenWidth() );
    int           name_offset = schIUScale.MilsToIU( PIN_TEXT_MARGIN ) + nameWidth;
    int           num_offset = schIUScale.MilsToIU( PIN_TEXT_MARGIN ) + numWidth;

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
    case PIN_ORIENTATION::PIN_UP:    y1 -= m_length; break;
    case PIN_ORIENTATION::PIN_DOWN:  y1 += m_length; break;
    case PIN_ORIENTATION::PIN_LEFT:  x1 -= m_length; break;
    case PIN_ORIENTATION::PIN_RIGHT: x1 += m_length; break;
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
                             nameSize, hAlign, vAlign, nameWidth, false, false, font, GetFontMetrics() );
            };

    auto printNum =
            [&]( int x, int y, const EDA_ANGLE& angle, enum GR_TEXT_H_ALIGN_T hAlign,
                 enum GR_TEXT_V_ALIGN_T vAlign )
            {
                GRPrintText( aSettings->GetPrintDC(), VECTOR2I( x, y ), numColor, number, angle,
                             numSize, hAlign, vAlign, numWidth, false, false, font, GetFontMetrics() );
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


void LIB_PIN::printPinElectricalTypeName( const RENDER_SETTINGS* aSettings, VECTOR2I& aPosition,
                                          PIN_ORIENTATION aOrientation, bool aDimmed )
{
    wxDC*       DC = aSettings->GetPrintDC();
    wxString    typeName = GetElectricalTypeName();

    // Use a reasonable (small) size to draw the text
    int         textSize = ( m_nameTextSize * 3 ) / 4;

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
    }

    GRPrintText( DC, txtpos, color, typeName, orient, VECTOR2I( textSize, textSize ), hjustify,
                 GR_TEXT_V_ALIGN_CENTER, pensize, false, false, font, GetFontMetrics() );
}


void LIB_PIN::PlotPinType( PLOTTER *aPlotter, const VECTOR2I &aPosition,
                           PIN_ORIENTATION aOrientation, bool aDimmed ) const
{
    int                  MapX1, MapY1, x1, y1;
    SCH_RENDER_SETTINGS* renderSettings = getRenderSettings( aPlotter );
    COLOR4D              color = renderSettings->GetLayerColor( LAYER_PIN );
    COLOR4D              bg = renderSettings->GetBackgroundColor();
    int                  penWidth = GetEffectivePenWidth( renderSettings );

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
    case PIN_ORIENTATION::PIN_UP:     y1 = aPosition.y - m_length;  MapY1 = 1;   break;
    case PIN_ORIENTATION::PIN_DOWN:   y1 = aPosition.y + m_length;  MapY1 = -1;  break;
    case PIN_ORIENTATION::PIN_LEFT:   x1 = aPosition.x - m_length;  MapX1 = 1;   break;
    case PIN_ORIENTATION::PIN_RIGHT:  x1 = aPosition.x + m_length;  MapX1 = -1;  break;
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


void LIB_PIN::PlotPinTexts( PLOTTER *aPlotter, const VECTOR2I &aPinPos, PIN_ORIENTATION aPinOrient,
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

    int     namePenWidth = std::max( Clamp_Text_PenSize( GetPenWidth(), m_nameTextSize, true ),
                                     settings->GetDefaultPenWidth() );
    int     numPenWidth  = std::max( Clamp_Text_PenSize( GetPenWidth(), m_numTextSize, true ),
                                     settings->GetDefaultPenWidth() );
    int     name_offset = schIUScale.MilsToIU( PIN_TEXT_MARGIN ) + namePenWidth;
    int     num_offset  = schIUScale.MilsToIU( PIN_TEXT_MARGIN ) + numPenWidth;

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
    case PIN_ORIENTATION::PIN_UP:     y1 -= m_length;  break;
    case PIN_ORIENTATION::PIN_DOWN:   y1 += m_length;  break;
    case PIN_ORIENTATION::PIN_LEFT:   x1 -= m_length;  break;
    case PIN_ORIENTATION::PIN_RIGHT:  x1 += m_length;  break;
    }

    auto plotName =
            [&]( int x, int y, const EDA_ANGLE& angle, GR_TEXT_H_ALIGN_T hJustify,
                 GR_TEXT_V_ALIGN_T vJustify )
            {
                TEXT_ATTRIBUTES attrs;
                attrs.m_StrokeWidth = namePenWidth;
                attrs.m_Angle = angle;
                attrs.m_Size = VECTOR2I( m_nameTextSize, m_nameTextSize );
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
                attrs.m_Size = VECTOR2I( m_numTextSize, m_numTextSize );
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


PIN_ORIENTATION LIB_PIN::PinDrawOrient( const TRANSFORM& aTransform ) const
{
    PIN_ORIENTATION orient;
    VECTOR2I end; // position of pin end starting at 0,0 according to its orientation, length = 1

    switch( m_orientation )
    {
    case PIN_ORIENTATION::PIN_UP:     end.y = 1;   break;
    case PIN_ORIENTATION::PIN_DOWN:   end.y = -1;  break;
    case PIN_ORIENTATION::PIN_LEFT:   end.x = -1;  break;
    case PIN_ORIENTATION::PIN_RIGHT:  end.x = 1;   break;
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


EDA_ITEM* LIB_PIN::Clone() const
{
    //return new LIB_PIN( *this );
    SCH_ITEM* newPin = new LIB_PIN( *this );
    wxASSERT( newPin->GetUnit() == m_unit && newPin->GetBodyStyle() == m_bodyStyle );
    return newPin;
}


int LIB_PIN::compare( const SCH_ITEM& aOther, int aCompareFlags ) const
{
    wxASSERT( aOther.Type() == LIB_PIN_T );

    int retv = SCH_ITEM::compare( aOther, aCompareFlags );

    if( retv )
        return retv;

    const LIB_PIN* tmp = (LIB_PIN*) &aOther;

    // When comparing units, we do not compare the part numbers.  If everything else is
    // identical, then we can just renumber the parts for the inherited symbol.
    if( !( aCompareFlags & SCH_ITEM::COMPARE_FLAGS::UNIT ) && m_number != tmp->m_number )
        return m_number.Cmp( tmp->m_number );

    int result = m_name.Cmp( tmp->m_name );

    if( result )
        return result;

    if( m_position.x != tmp->m_position.x )
        return m_position.x - tmp->m_position.x;

    if( m_position.y != tmp->m_position.y )
        return m_position.y - tmp->m_position.y;

    if( m_length != tmp->m_length )
        return m_length - tmp->m_length;

    if( m_orientation != tmp->m_orientation )
        return static_cast<int>( m_orientation ) - static_cast<int>( tmp->m_orientation );

    if( m_shape != tmp->m_shape )
        return static_cast<int>( m_shape ) - static_cast<int>( tmp->m_shape );

    if( m_type != tmp->m_type )
        return static_cast<int>( m_type ) - static_cast<int>( tmp->m_type );

    if( m_attributes != tmp->m_attributes )
        return m_attributes - tmp->m_attributes;

    if( m_numTextSize != tmp->m_numTextSize )
        return m_numTextSize - tmp->m_numTextSize;

    if( m_nameTextSize != tmp->m_nameTextSize )
        return m_nameTextSize - tmp->m_nameTextSize;

    if( m_alternates.size() != tmp->m_alternates.size() )
        return m_alternates.size() - tmp->m_alternates.size();

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

    return 0;
}

void LIB_PIN::ChangeLength( int aLength )
{
    int lengthChange = m_length - aLength;
    int offsetX = 0;
    int offsetY = 0;

    switch( m_orientation )
    {
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

    VECTOR2I offset = VECTOR2I( offsetX, offsetY );
    Move( offset );

    m_length = aLength;
}

void LIB_PIN::Move( const VECTOR2I& aOffset )
{
    m_position += aOffset;
}


void LIB_PIN::MirrorHorizontally( int aCenter )
{
    m_position.x -= aCenter;
    m_position.x *= -1;
    m_position.x += aCenter;

    if( m_orientation == PIN_ORIENTATION::PIN_RIGHT )
        m_orientation = PIN_ORIENTATION::PIN_LEFT;
    else if( m_orientation == PIN_ORIENTATION::PIN_LEFT )
        m_orientation = PIN_ORIENTATION::PIN_RIGHT;
}


void LIB_PIN::MirrorVertically( int aCenter )
{
    m_position.y -= aCenter;
    m_position.y *= -1;
    m_position.y += aCenter;

    if( m_orientation == PIN_ORIENTATION::PIN_UP )
        m_orientation = PIN_ORIENTATION::PIN_DOWN;
    else if( m_orientation == PIN_ORIENTATION::PIN_DOWN )
        m_orientation = PIN_ORIENTATION::PIN_UP;
}


void LIB_PIN::Rotate( const VECTOR2I& aCenter, bool aRotateCCW )
{
    EDA_ANGLE rot_angle = aRotateCCW ? -ANGLE_90 : ANGLE_90;

    RotatePoint( m_position, aCenter, rot_angle );

    if( aRotateCCW )
    {
        switch( m_orientation )
        {
        case PIN_ORIENTATION::PIN_RIGHT: m_orientation = PIN_ORIENTATION::PIN_UP;    break;
        case PIN_ORIENTATION::PIN_UP:    m_orientation = PIN_ORIENTATION::PIN_LEFT;  break;
        case PIN_ORIENTATION::PIN_LEFT:  m_orientation = PIN_ORIENTATION::PIN_DOWN;  break;
        case PIN_ORIENTATION::PIN_DOWN:  m_orientation = PIN_ORIENTATION::PIN_RIGHT; break;
        }
    }
    else
    {
        switch( m_orientation )
        {
        case PIN_ORIENTATION::PIN_RIGHT: m_orientation = PIN_ORIENTATION::PIN_DOWN;  break;
        case PIN_ORIENTATION::PIN_UP:    m_orientation = PIN_ORIENTATION::PIN_RIGHT; break;
        case PIN_ORIENTATION::PIN_LEFT:  m_orientation = PIN_ORIENTATION::PIN_UP;    break;
        case PIN_ORIENTATION::PIN_DOWN:  m_orientation = PIN_ORIENTATION::PIN_LEFT;  break;
        }
    }
}


void LIB_PIN::Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts,
                    int aUnit, int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed )
{
    if( !IsVisible() || aBackground )
        return;

    SCH_RENDER_SETTINGS* renderSettings = getRenderSettings( aPlotter );
    const SYMBOL*        part = GetParentSymbol();
    PIN_ORIENTATION      orient = PinDrawOrient( renderSettings->m_Transform );

    VECTOR2I pos = renderSettings->TransformCoordinate( m_position ) + aOffset;

    PlotPinType( aPlotter, pos, orient, aDimmed );
    PlotPinTexts( aPlotter, pos, orient, part->GetPinNameOffset(), part->GetShowPinNumbers(),
                  part->GetShowPinNames(), aDimmed );
}


void LIB_PIN::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    getSymbolEditorMsgPanelInfo( aFrame, aList );

    aList.emplace_back( _( "Name" ), UnescapeString( GetShownName() ) );
    aList.emplace_back( _( "Number" ), GetShownNumber() );
    aList.emplace_back( _( "Type" ), ElectricalPinTypeGetText( m_type ) );
    aList.emplace_back( _( "Style" ), PinShapeGetText( m_shape ) );

    aList.emplace_back( _( "Style" ), IsVisible() ? _( "Yes" ) : _( "No" ) );

    // Display pin length
    aList.emplace_back( _( "Length" ), aFrame->MessageTextFromValue( m_length, true ) );

    aList.emplace_back( _( "Orientation" ), PinOrientationName( m_orientation ) );

    VECTOR2I pinpos = GetPosition();
    pinpos.y = -pinpos.y;   // Display coords are top to bottom; lib item coords are bottom to top

    aList.emplace_back( _( "Pos X" ), aFrame->MessageTextFromValue( pinpos.x, true ) );
    aList.emplace_back( _( "Pos Y" ), aFrame->MessageTextFromValue( pinpos.y, true ) );

    #if 0   // For debug purpose only
    aList.emplace_back( _( "Flags" ), wxString::Format( "%8.8X", (long)GetFlags() ) );
    #endif
}


const BOX2I LIB_PIN::ViewBBox() const
{
    return GetBoundingBox( false, true, true );
}


void LIB_PIN::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount     = 6;
    aLayers[0] = LAYER_DANGLING;    // We don't really show dangling vs non-dangling (since there
                                    // are no connections in the symbol editor), but it's still
                                    // a good visual indication of which end of the pin is which.
    aLayers[1] = LAYER_DEVICE;
    aLayers[2] = LAYER_SELECTION_SHADOWS;
    aLayers[3] = LAYER_OP_CURRENTS;
    aLayers[4] = LAYER_PINNAM;
    aLayers[5] = LAYER_PINNUM;
}


void LIB_PIN::validateExtentsCache( KIFONT::FONT* aFont, int aSize, const wxString& aText,
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


const BOX2I LIB_PIN::GetBoundingBox( bool aIncludeInvisiblePins, bool aIncludeNameAndNumber,
                                     bool aIncludeElectricalType ) const
{
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

    if( !aIncludeInvisiblePins && !IsVisible() )
    {
        includeName = false;
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
        validateExtentsCache( font, m_numTextSize, GetShownNumber(), &m_numExtentsCache );
        numberTextLength = m_numExtentsCache.m_Extents.x;
        numberTextHeight = m_numExtentsCache.m_Extents.y;
    }

    if( includeName )
    {
        validateExtentsCache( font, m_nameTextSize, GetShownName(), &m_nameExtentsCache );
        nameTextLength = m_nameExtentsCache.m_Extents.x + pinNameOffset;
        nameTextHeight = m_nameExtentsCache.m_Extents.y + schIUScale.MilsToIU( PIN_TEXT_MARGIN );
    }

    if( includeType )
    {
        double   fontSize = std::max( m_nameTextSize * 3 / 4, schIUScale.mmToIU( 0.7 ) );
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
        begin.y = std::max( minsizeV, numberTextHeight );
        begin.x = std::min( -typeTextLength, m_length - ( numberTextLength / 2 ) );

        end.x = m_length + nameTextLength;
        end.y = std::min( -minsizeV, -nameTextHeight / 2 );
    }
    else
    {
        // pin name is above pin line
        // pin number is below line
        begin.y = std::max( minsizeV, nameTextHeight );
        begin.x = -typeTextLength;
        begin.x = std::min( begin.x, ( m_length - numberTextLength ) / 2 );
        begin.x = std::min( begin.x, ( m_length - nameTextLength ) / 2 );

        end.x = m_length;
        end.x = std::max( end.x, ( m_length + nameTextLength ) / 2 );
        end.x = std::max( end.x, ( m_length + numberTextLength ) / 2 );
        end.y = std::min( -minsizeV, -numberTextHeight );
    }

    // Now, calculate boundary box corners position for the actual pin orientation
    PIN_ORIENTATION orient = PinDrawOrient( DefaultTransform );

    /* Calculate the pin position */
    switch( orient )
    {
    case PIN_ORIENTATION::PIN_UP:
        // Pin is rotated and texts positions are mirrored
        RotatePoint( begin, VECTOR2I( 0, 0 ), -ANGLE_90 );
        RotatePoint( end, VECTOR2I( 0, 0 ), -ANGLE_90 );
        break;

    case PIN_ORIENTATION::PIN_DOWN:
        RotatePoint( begin, VECTOR2I( 0, 0 ), ANGLE_90 );
        RotatePoint( end, VECTOR2I( 0, 0 ), ANGLE_90 );
        begin.x = -begin.x;
        end.x = -end.x;
        break;

    case PIN_ORIENTATION::PIN_LEFT:
        begin.x = -begin.x;
        end.x = -end.x;
        break;

    case PIN_ORIENTATION::PIN_RIGHT:
        break;
    }

    begin += m_position;
    end += m_position;

    bbox.SetOrigin( begin );
    bbox.SetEnd( end );
    bbox.Normalize();
    bbox.Inflate( ( GetPenWidth() / 2 ) + 1 );

    // Draw Y axis is reversed in schematic:
    bbox.RevertYAxis();

    return bbox;
}


BITMAPS LIB_PIN::GetMenuImage() const
{
    return ElectricalPinTypeGetBitmap( m_type );
}


wxString LIB_PIN::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, ALT* aAlt ) const
{
    return getItemDescription( aAlt );
}


wxString LIB_PIN::GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const
{
    return getItemDescription( nullptr );
}


wxString LIB_PIN::getItemDescription( ALT* aAlt ) const
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


bool LIB_PIN::operator==( const SCH_ITEM& aOther ) const
{
    if( aOther.Type() != LIB_PIN_T )
        return false;

    const LIB_PIN* other = static_cast<const LIB_PIN*>( &aOther );

    if( m_name != other->m_name )
        return false;

    if( m_number != other->m_number )
        return false;

    if( m_position != other->m_position )
        return false;

    if( m_length != other->m_length )
        return false;

    if( m_orientation != other->m_orientation )
        return false;

    if( m_shape != other->m_shape )
        return false;

    if( m_type != other->m_type )
        return false;

    if( m_attributes != other->m_attributes )
        return false;

    if( m_numTextSize != other->m_numTextSize )
        return false;

    if( m_nameTextSize != other->m_nameTextSize )
        return false;

    if( m_alternates.size() != other->m_alternates.size() )
        return false;

    auto lhsItem = m_alternates.begin();
    auto rhsItem = other->m_alternates.begin();

    while( lhsItem != m_alternates.end() )
    {
        if( rhsItem == other->m_alternates.end() )
            return false;

        const ALT& lhsAlt = lhsItem->second;
        const ALT& rhsAlt = rhsItem->second;

        if( lhsAlt.m_Name != rhsAlt.m_Name )
            return false;

        if( lhsAlt.m_Type != rhsAlt.m_Type )
            return false;

        if( lhsAlt.m_Shape != rhsAlt.m_Shape )
            return false;

        ++lhsItem;
        ++rhsItem;
    }

    return rhsItem == other->m_alternates.end();
}


double LIB_PIN::Similarity( const SCH_ITEM& aOther ) const
{
    if( aOther.m_Uuid == m_Uuid )
        return 1.0;

    if( aOther.Type() != LIB_PIN_T )
        return 0.0;

    const LIB_PIN* other = static_cast<const LIB_PIN*>( &aOther );

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

    if( m_attributes != other->m_attributes )
        similarity *= 0.9;

    if( m_numTextSize != other->m_numTextSize )
        similarity *= 0.9;

    if( m_nameTextSize != other->m_nameTextSize )
        similarity *= 0.9;

    if( m_alternates.size() != other->m_alternates.size() )
        similarity *= 0.9;

    return similarity;
}


std::ostream& LIB_PIN::operator<<( std::ostream& aStream )
{
    aStream << "LIB_PIN:" << std::endl
            << "  Name: \"" << m_name << "\"" << std::endl
            << "  Number: \"" << m_number << "\"" << std::endl
            << "  Position: " << m_position << std::endl
            << "  Length: " << m_length << std::endl
            << "  Orientation: " << PinOrientationName( m_orientation ) << std::endl
            << "  Shape: " << PinShapeGetText( m_shape ) << std::endl
            << "  Type: " << ElectricalPinTypeGetText( m_type ) << std::endl
            << "  Name Text Size: " << m_nameTextSize << std::endl
            << "  Number Text Size: " << m_numTextSize << std::endl;

    return aStream;
}


#if defined(DEBUG)

void LIB_PIN::Show( int nestLevel, std::ostream& os ) const
{
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str()
                                 << " num=\"" << m_number.mb_str()
                                 << '"' << "/>\n";

//    NestedSpace( nestLevel, os ) << "</" << GetClass().Lower().mb_str() << ">\n";
}

#endif

void LIB_PIN::CalcEdit( const VECTOR2I& aPosition )
{
    if( IsMoving() )
        SetPosition( aPosition );
}


static struct LIB_PIN_DESC
{
    LIB_PIN_DESC()
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

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( LIB_PIN );
        propMgr.AddTypeCast( new TYPE_CAST<LIB_PIN, SCH_ITEM> );
        propMgr.InheritsAfter( TYPE_HASH( LIB_PIN ), TYPE_HASH( SCH_ITEM ) );

        propMgr.AddProperty( new PROPERTY<LIB_PIN, wxString>( _HKI( "Pin Name" ),
                &LIB_PIN::SetName, &LIB_PIN::GetName ) );

        propMgr.AddProperty( new PROPERTY<LIB_PIN, wxString>( _HKI( "Pin Number" ),
                &LIB_PIN::SetNumber, &LIB_PIN::GetNumber ) );

        propMgr.AddProperty( new PROPERTY_ENUM<LIB_PIN, ELECTRICAL_PINTYPE>(
                _HKI( "Electrical Type" ),
                &LIB_PIN::SetType, &LIB_PIN::GetType ) );

        propMgr.AddProperty( new PROPERTY_ENUM<LIB_PIN, GRAPHIC_PINSHAPE>(
                _HKI( "Graphic Style" ),
                &LIB_PIN::SetShape, &LIB_PIN::GetShape ) );

        propMgr.AddProperty( new PROPERTY<LIB_PIN, int>( _HKI( "Position X" ),
                &LIB_PIN::SetX, &LIB_PIN::GetX, PROPERTY_DISPLAY::PT_COORD ) );

        propMgr.AddProperty( new PROPERTY<LIB_PIN, int>( _HKI( "Position Y" ),
                &LIB_PIN::SetY, &LIB_PIN::GetY, PROPERTY_DISPLAY::PT_COORD ) );

        propMgr.AddProperty( new PROPERTY_ENUM<LIB_PIN, PIN_ORIENTATION>( _HKI( "Orientation" ),
                &LIB_PIN::SetOrientation, &LIB_PIN::GetOrientation ) );

        propMgr.AddProperty( new PROPERTY<LIB_PIN, int>( _HKI( "Length" ),
                &LIB_PIN::SetLength, &LIB_PIN::GetLength,
                PROPERTY_DISPLAY::PT_SIZE ) );

        propMgr.AddProperty( new PROPERTY<LIB_PIN, int>( _HKI( "Name Text Size" ),
                &LIB_PIN::SetNameTextSize, &LIB_PIN::GetNameTextSize,
                PROPERTY_DISPLAY::PT_SIZE ) );

        propMgr.AddProperty( new PROPERTY<LIB_PIN, int>( _HKI( "Number Text Size" ),
                &LIB_PIN::SetNumberTextSize, &LIB_PIN::GetNumberTextSize,
                PROPERTY_DISPLAY::PT_SIZE ) );

        propMgr.AddProperty( new PROPERTY<LIB_PIN, bool>( _HKI( "Visible" ),
                &LIB_PIN::SetVisible, &LIB_PIN::IsVisible ) );
    }
} _LIB_PIN_DESC;


ENUM_TO_WXANY( PIN_ORIENTATION )
ENUM_TO_WXANY( GRAPHIC_PINSHAPE )
ENUM_TO_WXANY( ELECTRICAL_PINTYPE )
