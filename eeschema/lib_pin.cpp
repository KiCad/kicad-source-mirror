/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pgm_base.h>
#include <sch_draw_panel.h>
#include <sch_edit_frame.h>
#include <base_units.h>
#include <widgets/msgpanel.h>
#include <general.h>
#include <symbol_edit_frame.h>
#include <class_libentry.h>
#include <lib_pin.h>
#include <settings/settings_manager.h>
#include <symbol_editor/symbol_editor_settings.h>
#include <trigo.h>
#include "sch_painter.h"

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
        wxT( "unspecified" ),
        wxT( "power_in" ),
        wxT( "power_out" ),
        wxT( "open_collector" ),
        wxT( "open_emitter" ),
        wxT( "unconnected" )
    };

    return msgPinElectricType[static_cast<int>( aType )];
}


/// Utility for getting the size of the 'internal' pin decorators (as a radius)
// i.e. the clock symbols (falling clock is actually external but is of
// the same kind)

static int internalPinDecoSize( const RENDER_SETTINGS* aSettings, const LIB_PIN &aPin )
{
    const KIGFX::SCH_RENDER_SETTINGS* settings = static_cast<const KIGFX::SCH_RENDER_SETTINGS*>( aSettings );

    if( settings && settings->m_PinSymbolSize )
        return settings->m_PinSymbolSize;

    return aPin.GetNameTextSize() != 0 ? aPin.GetNameTextSize() / 2 : aPin.GetNumberTextSize() / 2;
}

/// Utility for getting the size of the 'external' pin decorators (as a radius)
// i.e. the negation circle, the polarity 'slopes' and the nonlogic
// marker
static int externalPinDecoSize( const RENDER_SETTINGS* aSettings, const LIB_PIN &aPin )
{
    const KIGFX::SCH_RENDER_SETTINGS* settings = static_cast<const KIGFX::SCH_RENDER_SETTINGS*>( aSettings );

    if( settings && settings->m_PinSymbolSize )
        return settings->m_PinSymbolSize;

    return aPin.GetNumberTextSize() / 2;
}


LIB_PIN::LIB_PIN( LIB_PART* aParent ) :
        LIB_ITEM( LIB_PIN_T, aParent ),
        m_orientation( PIN_RIGHT ),
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
        m_length       = Mils2iu( settings->m_Defaults.pin_length );
        m_numTextSize  = Mils2iu( settings->m_Defaults.pin_num_size );
        m_nameTextSize = Mils2iu( settings->m_Defaults.pin_name_size );
    }
    else    // Use hardcoded eeschema defaults: symbol_editor settings are not existing.
    {
        m_length       = Mils2iu( DEFAULT_PIN_LENGTH );
        m_numTextSize  = Mils2iu( DEFAULT_PINNUM_SIZE );
        m_nameTextSize = Mils2iu( DEFAULT_PINNAME_SIZE );
    }
}


LIB_PIN::LIB_PIN( LIB_PART* aParent, const wxString& aName, const wxString& aNumber,
                  int aOrientation, ELECTRICAL_PINTYPE aPinType, int aLength, int aNameTextSize,
                  int aNumTextSize, int aConvert, const wxPoint& aPos, int aUnit ) :
        LIB_ITEM( LIB_PIN_T, aParent ),
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
    SetUnit( aUnit );
    SetConvert( aConvert );
}


bool LIB_PIN::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    EDA_RECT rect = GetBoundingBox();

    return rect.Inflate( aAccuracy ).Contains( aPosition );
}


bool LIB_PIN::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    if( m_flags & (STRUCT_DELETED | SKIP_STRUCT ) )
        return false;

    EDA_RECT sel = aRect;

    if ( aAccuracy )
        sel.Inflate( aAccuracy );

    if( aContained )
        return sel.Contains( GetBoundingBox( false, true ) );

    return sel.Intersects( GetBoundingBox( false, true ) );
}


int LIB_PIN::GetPenWidth() const
{
    return 1;
}


void LIB_PIN::print( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset, void* aData,
                     const TRANSFORM& aTransform )
{
    PART_DRAW_OPTIONS* opts = (PART_DRAW_OPTIONS*) aData;
    bool               drawHiddenFields = opts ? opts->draw_hidden_fields : false;
    bool               showPinType = opts ? opts->show_elec_type : false;

    LIB_PART* part = GetParent();

    /* Calculate pin orient taking in account the component orientation. */
    int orient = PinDrawOrient( aTransform );

    /* Calculate the pin position */
    wxPoint pos1 = aTransform.TransformCoordinate( m_position ) + aOffset;

    if( IsVisible() || drawHiddenFields )
    {
        printPinSymbol( aSettings, pos1, orient );

        printPinTexts( aSettings, pos1, orient, part->GetPinNameOffset(), part->ShowPinNumbers(),
                       part->ShowPinNames() );

        if( showPinType )
            printPinElectricalTypeName( aSettings, pos1, orient );
    }
}


void LIB_PIN::printPinSymbol( const RENDER_SETTINGS* aSettings, const wxPoint& aPos, int aOrient )
{
    wxDC*   DC = aSettings->GetPrintDC();
    int     MapX1, MapY1, x1, y1;
    int     width = std::max( GetPenWidth(), aSettings->GetDefaultPenWidth() );
    int     posX = aPos.x, posY = aPos.y, len = m_length;
    COLOR4D color = aSettings->GetLayerColor( IsVisible() ? LAYER_PIN : LAYER_HIDDEN );

    MapX1 = MapY1 = 0;
    x1    = posX;
    y1    = posY;

    switch( aOrient )
    {
    case PIN_UP:     y1 = posY - len;  MapY1 = 1;   break;
    case PIN_DOWN:   y1 = posY + len;  MapY1 = -1;  break;
    case PIN_LEFT:   x1 = posX - len;  MapX1 = 1;   break;
    case PIN_RIGHT:  x1 = posX + len;  MapX1 = -1;  break;
    }

    if( m_shape == GRAPHIC_PINSHAPE::INVERTED || m_shape == GRAPHIC_PINSHAPE::INVERTED_CLOCK )
    {
        const int radius = externalPinDecoSize( aSettings, *this );
        GRCircle( nullptr, DC, MapX1 * radius + x1, MapY1 * radius + y1, radius, width, color );

        GRMoveTo( MapX1 * radius * 2 + x1, MapY1 * radius * 2 + y1 );
        GRLineTo( nullptr, DC, posX, posY, width, color );
    }
    else
    {
        GRMoveTo( x1, y1 );
        GRLineTo( nullptr, DC, posX, posY, width, color );
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
            GRLineTo( nullptr, DC, x1 - MapX1 * clock_size * 2, y1, width, color );
            GRLineTo( nullptr, DC, x1, y1 - clock_size, width, color );
        }
        else    /* MapX1 = 0 */
        {
            GRMoveTo( x1 + clock_size, y1 );
            GRLineTo( nullptr, DC, x1, y1 - MapY1 * clock_size * 2, width, color );
            GRLineTo( nullptr, DC, x1 - clock_size, y1, width, color );
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
            GRLineTo( nullptr, DC, x1 + MapX1 * deco_size * 2, y1 - deco_size * 2, width, color );
            GRLineTo( nullptr, DC, x1, y1, width, color );
        }
        else    /* MapX1 = 0 */
        {
            GRMoveTo( x1, y1 + MapY1 * deco_size * 2 );
            GRLineTo( nullptr, DC, x1 - deco_size * 2, y1 + MapY1 * deco_size * 2, width, color );
            GRLineTo( nullptr, DC, x1, y1, width, color );
        }
    }

    if( m_shape == GRAPHIC_PINSHAPE::OUTPUT_LOW ) /* IEEE symbol "Active Low Output" */
    {
        const int deco_size = externalPinDecoSize( aSettings, *this );
        if( MapY1 == 0 )            /* MapX1 = +- 1 */
        {
            GRMoveTo( x1, y1 - deco_size * 2 );
            GRLineTo( nullptr, DC, x1 + MapX1 * deco_size * 2, y1, width, color );
        }
        else    /* MapX1 = 0 */
        {
            GRMoveTo( x1 - deco_size * 2, y1 );
            GRLineTo( nullptr, DC, x1, y1 + MapY1 * deco_size * 2, width, color );
        }
    }
    else if( m_shape == GRAPHIC_PINSHAPE::NONLOGIC ) /* NonLogic pin symbol */
    {
        const int deco_size = externalPinDecoSize( aSettings, *this );
        GRMoveTo( x1 - (MapX1 + MapY1) * deco_size, y1 - (MapY1 - MapX1) * deco_size );
        GRLineTo( nullptr, DC, x1 + (MapX1 + MapY1) * deco_size,
                  y1 + ( MapY1 - MapX1 ) * deco_size, width, color );
        GRMoveTo( x1 - (MapX1 - MapY1) * deco_size, y1 - (MapY1 + MapX1) * deco_size );
        GRLineTo( nullptr, DC, x1 + (MapX1 - MapY1) * deco_size,
                  y1 + ( MapY1 + MapX1 ) * deco_size, width, color );
    }

    if( m_type == ELECTRICAL_PINTYPE::PT_NC ) // Draw a N.C. symbol
    {
        const int deco_size = TARGET_PIN_RADIUS;
        GRLine( nullptr, DC, posX - deco_size, posY - deco_size, posX + deco_size,
                posY + deco_size, width, color );
        GRLine( nullptr, DC, posX + deco_size, posY - deco_size, posX - deco_size,
                posY + deco_size, width, color );
    }
}


void LIB_PIN::printPinTexts( const RENDER_SETTINGS* aSettings, wxPoint& aPinPos, int aPinOrient,
                             int aTextInside, bool aDrawPinNum, bool aDrawPinName )
{
    if( !aDrawPinName && !aDrawPinNum )
        return;

    int    x, y;
    wxDC*  DC = aSettings->GetPrintDC();
    wxSize PinNameSize( m_nameTextSize, m_nameTextSize );
    wxSize PinNumSize( m_numTextSize, m_numTextSize );

    int    namePenWidth = std::max( Clamp_Text_PenSize( GetPenWidth(), m_nameTextSize, false ),
                                    aSettings->GetDefaultPenWidth() );
    int    numPenWidth = std::max( Clamp_Text_PenSize( GetPenWidth(), m_numTextSize, false ),
                                   aSettings->GetDefaultPenWidth() );

    int    name_offset = Mils2iu( PIN_TEXT_MARGIN ) + namePenWidth;
    int    num_offset = Mils2iu( PIN_TEXT_MARGIN ) + numPenWidth;

    /* Get the num and name colors */
    COLOR4D NameColor = aSettings->GetLayerColor( IsVisible() ? LAYER_PINNAM : LAYER_HIDDEN );
    COLOR4D NumColor  = aSettings->GetLayerColor( IsVisible() ? LAYER_PINNUM : LAYER_HIDDEN );

    int x1 = aPinPos.x;
    int y1 = aPinPos.y;

    switch( aPinOrient )
    {
    case PIN_UP:    y1 -= m_length; break;
    case PIN_DOWN:  y1 += m_length; break;
    case PIN_LEFT:  x1 -= m_length; break;
    case PIN_RIGHT: x1 += m_length; break;
    }

    if( m_name.IsEmpty() )
        aDrawPinName = false;

    if( aTextInside )  // Draw the text inside, but the pin numbers outside.
    {
        if(( aPinOrient == PIN_LEFT) || ( aPinOrient == PIN_RIGHT) )
        {
            // It is an horizontal line
            if( aDrawPinName )
            {
                if( aPinOrient == PIN_RIGHT )
                {
                    x = x1 + aTextInside;
                    GRText( DC, wxPoint( x, y1 ), NameColor, m_name, TEXT_ANGLE_HORIZ,
                            PinNameSize, GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                            namePenWidth, false, false );
                }
                else    // Orient == PIN_LEFT
                {
                    x = x1 - aTextInside;
                    GRText( DC, wxPoint( x, y1 ), NameColor, m_name, TEXT_ANGLE_HORIZ,
                            PinNameSize, GR_TEXT_HJUSTIFY_RIGHT, GR_TEXT_VJUSTIFY_CENTER,
                            namePenWidth, false, false );
                }
            }

            if( aDrawPinNum )
            {
                GRText( DC, wxPoint(( x1 + aPinPos.x) / 2, y1 - num_offset ), NumColor, m_number,
                        TEXT_ANGLE_HORIZ, PinNumSize, GR_TEXT_HJUSTIFY_CENTER,
                        GR_TEXT_VJUSTIFY_BOTTOM, numPenWidth, false, false );
            }
        }
        else            /* Its a vertical line. */
        {
            // Text is drawn from bottom to top (i.e. to negative value for Y axis)
            if( aPinOrient == PIN_DOWN )
            {
                y = y1 + aTextInside;

                if( aDrawPinName )
                {
                    GRText( DC, wxPoint( x1, y ), NameColor, m_name, TEXT_ANGLE_VERT, PinNameSize,
                            GR_TEXT_HJUSTIFY_RIGHT, GR_TEXT_VJUSTIFY_CENTER, namePenWidth, false,
                            false );
                }

                if( aDrawPinNum )
                {
                    GRText( DC, wxPoint( x1 - num_offset, ( y1 + aPinPos.y) / 2 ), NumColor,
                            m_number, TEXT_ANGLE_VERT, PinNumSize, GR_TEXT_HJUSTIFY_CENTER,
                            GR_TEXT_VJUSTIFY_BOTTOM, numPenWidth, false, false );
                }
            }
            else        /* PIN_UP */
            {
                y = y1 - aTextInside;

                if( aDrawPinName )
                {
                    GRText( DC, wxPoint( x1, y ), NameColor, m_name, TEXT_ANGLE_VERT, PinNameSize,
                            GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER, namePenWidth, false,
                            false );
                }

                if( aDrawPinNum )
                {
                    GRText( DC, wxPoint( x1 - num_offset, ( y1 + aPinPos.y) / 2 ), NumColor,
                            m_number, TEXT_ANGLE_VERT, PinNumSize, GR_TEXT_HJUSTIFY_CENTER,
                            GR_TEXT_VJUSTIFY_BOTTOM, numPenWidth, false, false );
                }
            }
        }
    }
    else     /**** Draw num & text pin outside  ****/
    {
        if(( aPinOrient == PIN_LEFT) || ( aPinOrient == PIN_RIGHT) )
        {
            /* Its an horizontal line. */
            if( aDrawPinName )
            {
                x = ( x1 + aPinPos.x) / 2;
                GRText( DC, wxPoint( x, y1 - name_offset ), NameColor, m_name, TEXT_ANGLE_HORIZ,
                        PinNameSize, GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_BOTTOM,
                        namePenWidth, false, false );
            }
            if( aDrawPinNum )
            {
                x = ( x1 + aPinPos.x) / 2;
                GRText( DC, wxPoint( x, y1 + num_offset ), NumColor, m_number, TEXT_ANGLE_HORIZ,
                        PinNumSize, GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_TOP, numPenWidth,
                        false, false );
            }
        }
        else     /* Its a vertical line. */
        {
            if( aDrawPinName )
            {
                y = ( y1 + aPinPos.y) / 2;
                GRText( DC, wxPoint( x1 - name_offset, y ), NameColor, m_name, TEXT_ANGLE_VERT,
                        PinNameSize, GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_BOTTOM,
                        namePenWidth, false, false );
            }

            if( aDrawPinNum )
            {
                GRText( DC, wxPoint( x1 + num_offset, ( y1 + aPinPos.y) / 2 ), NumColor, m_number,
                        TEXT_ANGLE_VERT, PinNumSize, GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_TOP,
                        numPenWidth, false, false );
            }
        }
    }
}



void LIB_PIN::printPinElectricalTypeName( const RENDER_SETTINGS* aSettings, wxPoint& aPosition,
                                          int aOrientation )
{
    wxDC*       DC = aSettings->GetPrintDC();
    wxString    typeName = GetElectricalTypeName();

    // Use a reasonable (small) size to draw the text
    int         textSize = ( m_nameTextSize * 3 ) / 4;

    #define ETXT_MAX_SIZE Millimeter2iu( 0.7 )

    if( textSize > ETXT_MAX_SIZE )
        textSize = ETXT_MAX_SIZE;

    // Use a reasonable pen size to draw the text
    int pensize = textSize/6;

    // Get a suitable color
    COLOR4D color = aSettings->GetLayerColor( IsVisible() ? LAYER_NOTES : LAYER_HIDDEN );

    wxPoint txtpos = aPosition;
    int offset = Millimeter2iu( 0.4 );
    EDA_TEXT_HJUSTIFY_T hjustify = GR_TEXT_HJUSTIFY_LEFT;
    int orient = TEXT_ANGLE_HORIZ;

    switch( aOrientation )
    {
    case PIN_UP:
        txtpos.y += offset;
        orient = TEXT_ANGLE_VERT;
        hjustify = GR_TEXT_HJUSTIFY_RIGHT;
        break;

    case PIN_DOWN:
        txtpos.y -= offset;
        orient = TEXT_ANGLE_VERT;
        break;

    case PIN_LEFT:
        txtpos.x += offset;
        break;

    case PIN_RIGHT:
        txtpos.x -= offset;
        hjustify = GR_TEXT_HJUSTIFY_RIGHT;
        break;
    }

    GRText( DC, txtpos, color, typeName, orient, wxSize( textSize, textSize ), hjustify,
            GR_TEXT_VJUSTIFY_CENTER, pensize, false, false, 0 );
}


void LIB_PIN::PlotSymbol( PLOTTER* aPlotter, const wxPoint& aPosition, int aOrientation )
{
    int     MapX1, MapY1, x1, y1;
    COLOR4D color = aPlotter->RenderSettings()->GetLayerColor( LAYER_PIN );
    int     penWidth = std::max( GetPenWidth(), aPlotter->RenderSettings()->GetDefaultPenWidth() );

    aPlotter->SetColor( color );
    aPlotter->SetCurrentLineWidth( penWidth );

    MapX1 = MapY1 = 0;
    x1 = aPosition.x; y1 = aPosition.y;

    switch( aOrientation )
    {
    case PIN_UP:     y1 = aPosition.y - m_length;  MapY1 = 1;   break;
    case PIN_DOWN:   y1 = aPosition.y + m_length;  MapY1 = -1;  break;
    case PIN_LEFT:   x1 = aPosition.x - m_length;  MapX1 = 1;   break;
    case PIN_RIGHT:  x1 = aPosition.x + m_length;  MapX1 = -1;  break;
    }

    if( m_shape == GRAPHIC_PINSHAPE::INVERTED || m_shape == GRAPHIC_PINSHAPE::INVERTED_CLOCK )
    {
        const int radius = externalPinDecoSize( aPlotter->RenderSettings(), *this );
        aPlotter->Circle( wxPoint( MapX1 * radius + x1, MapY1 * radius + y1 ), radius * 2,
                          FILL_TYPE::NO_FILL, penWidth );

        aPlotter->MoveTo( wxPoint( MapX1 * radius * 2 + x1, MapY1 * radius * 2 + y1 ) );
        aPlotter->FinishTo( aPosition );
    }
    else if( m_shape == GRAPHIC_PINSHAPE::FALLING_EDGE_CLOCK )
    {
        const int deco_size = internalPinDecoSize( aPlotter->RenderSettings(), *this );
        if( MapY1 == 0 ) /* MapX1 = +- 1 */
        {
            aPlotter->MoveTo( wxPoint( x1, y1 + deco_size ) );
            aPlotter->LineTo( wxPoint( x1 + MapX1 * deco_size * 2, y1 ) );
            aPlotter->FinishTo( wxPoint( x1, y1 - deco_size ) );
        }
        else    /* MapX1 = 0 */
        {
            aPlotter->MoveTo( wxPoint( x1 + deco_size, y1 ) );
            aPlotter->LineTo( wxPoint( x1, y1 + MapY1 * deco_size * 2 ) );
            aPlotter->FinishTo( wxPoint( x1 - deco_size, y1 ) );
        }

        aPlotter->MoveTo( wxPoint( MapX1 * deco_size * 2 + x1, MapY1 * deco_size * 2 + y1 ) );
        aPlotter->FinishTo( aPosition );
    }
    else
    {
        aPlotter->MoveTo( wxPoint( x1, y1 ) );
        aPlotter->FinishTo( aPosition );
    }

    if( m_shape == GRAPHIC_PINSHAPE::CLOCK
            || m_shape == GRAPHIC_PINSHAPE::INVERTED_CLOCK
            || m_shape == GRAPHIC_PINSHAPE::CLOCK_LOW )
    {
        const int deco_size = internalPinDecoSize( aPlotter->RenderSettings(), *this );
        if( MapY1 == 0 ) /* MapX1 = +- 1 */
        {
            aPlotter->MoveTo( wxPoint( x1, y1 + deco_size ) );
            aPlotter->LineTo( wxPoint( x1 - MapX1 * deco_size * 2, y1 ) );
            aPlotter->FinishTo( wxPoint( x1, y1 - deco_size ) );
        }
        else    /* MapX1 = 0 */
        {
            aPlotter->MoveTo( wxPoint( x1 + deco_size, y1 ) );
            aPlotter->LineTo( wxPoint( x1, y1 - MapY1 * deco_size * 2 ) );
            aPlotter->FinishTo( wxPoint( x1 - deco_size, y1 ) );
        }
    }

    if( m_shape == GRAPHIC_PINSHAPE::INPUT_LOW
            || m_shape == GRAPHIC_PINSHAPE::CLOCK_LOW ) /* IEEE symbol "Active Low Input" */
    {
        const int deco_size = externalPinDecoSize( aPlotter->RenderSettings(), *this );

        if( MapY1 == 0 )        /* MapX1 = +- 1 */
        {
            aPlotter->MoveTo( wxPoint( x1 + MapX1 * deco_size * 2, y1 ) );
            aPlotter->LineTo( wxPoint( x1 + MapX1 * deco_size * 2, y1 - deco_size * 2 ) );
            aPlotter->FinishTo( wxPoint( x1, y1 ) );
        }
        else    /* MapX1 = 0 */
        {
            aPlotter->MoveTo( wxPoint( x1, y1 + MapY1 * deco_size * 2 ) );
            aPlotter->LineTo( wxPoint( x1 - deco_size * 2, y1 + MapY1 * deco_size * 2 ) );
            aPlotter->FinishTo( wxPoint( x1, y1 ) );
        }
    }

    if( m_shape == GRAPHIC_PINSHAPE::OUTPUT_LOW ) /* IEEE symbol "Active Low Output" */
    {
        const int symbol_size = externalPinDecoSize( aPlotter->RenderSettings(), *this );

        if( MapY1 == 0 )        /* MapX1 = +- 1 */
        {
            aPlotter->MoveTo( wxPoint( x1, y1 - symbol_size * 2 ) );
            aPlotter->FinishTo( wxPoint( x1 + MapX1 * symbol_size * 2, y1 ) );
        }
        else    /* MapX1 = 0 */
        {
            aPlotter->MoveTo( wxPoint( x1 - symbol_size * 2, y1 ) );
            aPlotter->FinishTo( wxPoint( x1, y1 + MapY1 * symbol_size * 2 ) );
        }
    }
    else if( m_shape == GRAPHIC_PINSHAPE::NONLOGIC ) /* NonLogic pin symbol */
    {
        const int deco_size = externalPinDecoSize( aPlotter->RenderSettings(), *this );
        aPlotter->MoveTo( wxPoint( x1 - (MapX1 + MapY1) * deco_size, y1 - (MapY1 - MapX1) * deco_size ) );
        aPlotter->FinishTo( wxPoint( x1 + (MapX1 + MapY1) * deco_size, y1 + (MapY1 - MapX1) * deco_size ) );
        aPlotter->MoveTo( wxPoint( x1 - (MapX1 - MapY1) * deco_size, y1 - (MapY1 + MapX1) * deco_size ) );
        aPlotter->FinishTo( wxPoint( x1 + (MapX1 - MapY1) * deco_size, y1 + (MapY1 + MapX1) * deco_size ) );
    }

    if( m_type == ELECTRICAL_PINTYPE::PT_NC ) // Draw a N.C. symbol
    {
        const int deco_size = TARGET_PIN_RADIUS;
        const int ex1 = aPosition.x;
        const int ey1 = aPosition.y;
        aPlotter->MoveTo( wxPoint( ex1 - deco_size, ey1 - deco_size ) );
        aPlotter->FinishTo( wxPoint( ex1 + deco_size, ey1 + deco_size ) );
        aPlotter->MoveTo( wxPoint( ex1 + deco_size, ey1 - deco_size ) );
        aPlotter->FinishTo( wxPoint( ex1 - deco_size, ey1 + deco_size ) );
    }
}


void LIB_PIN::PlotPinTexts( PLOTTER* aPlotter, wxPoint& aPinPos, int aPinOrient, int aTextInside,
                            bool aDrawPinNum, bool aDrawPinName )
{
    if( m_name.IsEmpty() || m_name == wxT( "~" ) )
        aDrawPinName = false;

    if( m_number.IsEmpty() )
        aDrawPinNum = false;

    if( !aDrawPinNum && !aDrawPinName )
        return;

    int     x, y;
    wxSize  pinNameSize = wxSize( m_nameTextSize, m_nameTextSize );
    wxSize  pinNumSize  = wxSize( m_numTextSize, m_numTextSize );

    int     namePenWidth = std::max( Clamp_Text_PenSize( GetPenWidth(), m_nameTextSize, false ),
                                     aPlotter->RenderSettings()->GetDefaultPenWidth() );
    int     numPenWidth =  std::max( Clamp_Text_PenSize( GetPenWidth(), m_numTextSize, false ),
                                     aPlotter->RenderSettings()->GetDefaultPenWidth() );

    int     name_offset = Mils2iu( PIN_TEXT_MARGIN ) + namePenWidth;
    int     num_offset = Mils2iu( PIN_TEXT_MARGIN ) + numPenWidth;

    /* Get the num and name colors */
    COLOR4D nameColor = aPlotter->RenderSettings()->GetLayerColor( LAYER_PINNAM );
    COLOR4D numColor  = aPlotter->RenderSettings()->GetLayerColor( LAYER_PINNUM );

    int x1 = aPinPos.x;
    int y1 = aPinPos.y;

    switch( aPinOrient )
    {
    case PIN_UP:     y1 -= m_length;  break;
    case PIN_DOWN:   y1 += m_length;  break;
    case PIN_LEFT:   x1 -= m_length;  break;
    case PIN_RIGHT:  x1 += m_length;  break;
    }

    /* Draw the text inside, but the pin numbers outside. */
    if( aTextInside )
    {
        if( ( aPinOrient == PIN_LEFT) || ( aPinOrient == PIN_RIGHT) ) /* Its an horizontal line. */
        {
            if( aDrawPinName )
            {
                if( aPinOrient == PIN_RIGHT )
                {
                    x = x1 + aTextInside;
                    aPlotter->Text( wxPoint( x, y1 ), nameColor, m_name, TEXT_ANGLE_HORIZ,
                                    pinNameSize, GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                                    namePenWidth, false, false );
                }
                else    // orient == PIN_LEFT
                {
                    x = x1 - aTextInside;

                    if( aDrawPinName )
                    {
                        aPlotter->Text( wxPoint( x, y1 ), nameColor, m_name, TEXT_ANGLE_HORIZ,
                                        pinNameSize, GR_TEXT_HJUSTIFY_RIGHT,
                                        GR_TEXT_VJUSTIFY_CENTER, namePenWidth, false, false );
                    }
                }
            }
            if( aDrawPinNum )
            {
                aPlotter->Text( wxPoint( ( x1 + aPinPos.x) / 2, y1 - num_offset ), numColor,
                                m_number, TEXT_ANGLE_HORIZ, pinNumSize, GR_TEXT_HJUSTIFY_CENTER,
                                GR_TEXT_VJUSTIFY_BOTTOM, numPenWidth, false, false );
            }
        }
        else         /* Its a vertical line. */
        {
            if( aPinOrient == PIN_DOWN )
            {
                y = y1 + aTextInside;

                if( aDrawPinName )
                    aPlotter->Text( wxPoint( x1, y ), nameColor, m_name, TEXT_ANGLE_VERT,
                                    pinNameSize, GR_TEXT_HJUSTIFY_RIGHT, GR_TEXT_VJUSTIFY_CENTER,
                                    namePenWidth, false, false );

                if( aDrawPinNum )
                {
                    aPlotter->Text( wxPoint( x1 - num_offset, ( y1 + aPinPos.y) / 2 ), numColor,
                                    m_number, TEXT_ANGLE_VERT, pinNumSize, GR_TEXT_HJUSTIFY_CENTER,
                                    GR_TEXT_VJUSTIFY_BOTTOM, numPenWidth, false, false );
                }
            }
            else        /* PIN_UP */
            {
                y = y1 - aTextInside;

                if( aDrawPinName )
                {
                    aPlotter->Text( wxPoint( x1, y ), nameColor, m_name, TEXT_ANGLE_VERT,
                                    pinNameSize, GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                                    namePenWidth, false, false );
                }

                if( aDrawPinNum )
                {
                    aPlotter->Text( wxPoint( x1 - num_offset,  ( y1 + aPinPos.y) / 2 ), numColor,
                                    m_number, TEXT_ANGLE_VERT, pinNumSize, GR_TEXT_HJUSTIFY_CENTER,
                                    GR_TEXT_VJUSTIFY_BOTTOM, numPenWidth, false, false );
                }
            }
        }
    }
    else     /* Draw num & text pin outside */
    {
        if(( aPinOrient == PIN_LEFT) || ( aPinOrient == PIN_RIGHT) )
        {
            /* Its an horizontal line. */
            if( aDrawPinName )
            {
                x = ( x1 + aPinPos.x) / 2;
                aPlotter->Text( wxPoint( x, y1 - name_offset ), nameColor, m_name,
                                TEXT_ANGLE_HORIZ, pinNameSize, GR_TEXT_HJUSTIFY_CENTER,
                                GR_TEXT_VJUSTIFY_BOTTOM, namePenWidth, false, false );
            }

            if( aDrawPinNum )
            {
                x = ( x1 + aPinPos.x ) / 2;
                aPlotter->Text( wxPoint( x, y1 + num_offset ), numColor, m_number,
                                TEXT_ANGLE_HORIZ, pinNumSize, GR_TEXT_HJUSTIFY_CENTER,
                                GR_TEXT_VJUSTIFY_TOP, numPenWidth, false, false );
            }
        }
        else     /* Its a vertical line. */
        {
            if( aDrawPinName )
            {
                y = ( y1 + aPinPos.y ) / 2;
                aPlotter->Text( wxPoint( x1 - name_offset, y ), nameColor, m_name,
                                TEXT_ANGLE_VERT, pinNameSize, GR_TEXT_HJUSTIFY_CENTER,
                                GR_TEXT_VJUSTIFY_BOTTOM, namePenWidth, false, false );
            }

            if( aDrawPinNum )
            {
                aPlotter->Text( wxPoint( x1 + num_offset, ( y1 + aPinPos.y ) / 2 ), numColor,
                                m_number, TEXT_ANGLE_VERT, pinNumSize, GR_TEXT_HJUSTIFY_CENTER,
                                GR_TEXT_VJUSTIFY_TOP, numPenWidth, false, false );
            }
        }
    }
}


int LIB_PIN::PinDrawOrient( const TRANSFORM& aTransform ) const
{
    int     orient;
    wxPoint end;   // position of pin end starting at 0,0 according to its orientation, length = 1

    switch( m_orientation )
    {
    case PIN_UP:     end.y = 1;   break;
    case PIN_DOWN:   end.y = -1;  break;
    case PIN_LEFT:   end.x = -1;  break;
    case PIN_RIGHT:  end.x = 1;   break;
    }

    // = pos of end point, according to the component orientation
    end    = aTransform.TransformCoordinate( end );
    orient = PIN_UP;

    if( end.x == 0 )
    {
        if( end.y > 0 )
            orient = PIN_DOWN;
    }
    else
    {
        orient = PIN_RIGHT;

        if( end.x < 0 )
            orient = PIN_LEFT;
    }

    return orient;
}


EDA_ITEM* LIB_PIN::Clone() const
{
    return new LIB_PIN( *this );
}


int LIB_PIN::compare( const LIB_ITEM& aOther, LIB_ITEM::COMPARE_FLAGS aCompareFlags ) const
{
    wxASSERT( aOther.Type() == LIB_PIN_T );

    int retv = LIB_ITEM::compare( aOther, aCompareFlags );

    if( retv )
        return retv;

    const LIB_PIN* tmp = (LIB_PIN*) &aOther;

    // When comparing units, we do not compare the part numbers.  If everything else is
    // identical, then we can just renumber the parts for the inherited symbol.
    if( !( aCompareFlags & COMPARE_FLAGS::UNIT ) && m_number != tmp->m_number )
        return m_number.Cmp( tmp->m_number );

    int result = m_name.CmpNoCase( tmp->m_name );

    if( result )
        return result;

    if( m_position.x != tmp->m_position.x )
        return m_position.x - tmp->m_position.x;

    if( m_position.y != tmp->m_position.y )
        return m_position.y - tmp->m_position.y;

    if( m_length != tmp->m_length )
        return m_length - tmp->m_length;

    if( m_orientation != tmp->m_orientation )
        return m_orientation - tmp->m_orientation;

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

    return 0;
}


void LIB_PIN::Offset( const wxPoint& aOffset )
{
    m_position += aOffset;
}


void LIB_PIN::MoveTo( const wxPoint& aNewPosition )
{
    if( m_position != aNewPosition )
    {
        m_position = aNewPosition;
        SetModified();
    }
}


void LIB_PIN::MirrorHorizontal( const wxPoint& aCenter )
{
    m_position.x -= aCenter.x;
    m_position.x *= -1;
    m_position.x += aCenter.x;

    if( m_orientation == PIN_RIGHT )
        m_orientation = PIN_LEFT;
    else if( m_orientation == PIN_LEFT )
        m_orientation = PIN_RIGHT;
}


void LIB_PIN::MirrorVertical( const wxPoint& aCenter )
{
    m_position.y -= aCenter.y;
    m_position.y *= -1;
    m_position.y += aCenter.y;

    if( m_orientation == PIN_UP )
        m_orientation = PIN_DOWN;
    else if( m_orientation == PIN_DOWN )
        m_orientation = PIN_UP;
}


void LIB_PIN::Rotate( const wxPoint& aCenter, bool aRotateCCW )
{
    int rot_angle = aRotateCCW ? -900 : 900;

    RotatePoint( &m_position, aCenter, rot_angle );

    if( aRotateCCW )
    {
        switch( m_orientation )
        {
        case PIN_RIGHT: m_orientation = PIN_UP;    break;
        case PIN_UP:    m_orientation = PIN_LEFT;  break;
        case PIN_LEFT:  m_orientation = PIN_DOWN;  break;
        case PIN_DOWN:  m_orientation = PIN_RIGHT; break;
        }
    }
    else
    {
        switch( m_orientation )
        {
        case PIN_RIGHT: m_orientation = PIN_DOWN;  break;
        case PIN_UP:    m_orientation = PIN_RIGHT; break;
        case PIN_LEFT:  m_orientation = PIN_UP;    break;
        case PIN_DOWN:  m_orientation = PIN_LEFT;  break;
        }
    }
}


void LIB_PIN::Plot( PLOTTER* aPlotter, const wxPoint& aPffset, bool aFill,
                    const TRANSFORM& aTransform )
{
    if( ! IsVisible() )
        return;

    int     orient = PinDrawOrient( aTransform );
    wxPoint pos = aTransform.TransformCoordinate( m_position ) + aPffset;

    PlotSymbol( aPlotter, pos, orient );
    PlotPinTexts( aPlotter, pos, orient, GetParent()->GetPinNameOffset(),
                  GetParent()->ShowPinNumbers(), GetParent()->ShowPinNames() );
}


void LIB_PIN::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, MSG_PANEL_ITEMS& aList )
{
    wxString text = m_number.IsEmpty() ? wxT( "?" ) : m_number;

    LIB_ITEM::GetMsgPanelInfo( aFrame, aList );

    aList.push_back( MSG_PANEL_ITEM( _( "Name" ), m_name ) );
    aList.push_back( MSG_PANEL_ITEM( _( "Number" ), text ) );
    aList.push_back( MSG_PANEL_ITEM( _( "Type" ), ElectricalPinTypeGetText( m_type ) ) );

    text = PinShapeGetText( m_shape );
    aList.push_back( MSG_PANEL_ITEM( _( "Style" ), text ) );

    text = IsVisible() ? _( "Yes" ) : _( "No" );
    aList.push_back( MSG_PANEL_ITEM( _( "Visible" ), text ) );

    // Display pin length
    text = StringFromValue( aFrame->GetUserUnits(), m_length );
    aList.push_back( MSG_PANEL_ITEM( _( "Length" ), text ) );

    text = PinOrientationName( (unsigned) PinOrientationIndex( m_orientation ) );
    aList.push_back( MSG_PANEL_ITEM( _( "Orientation" ), text ) );

    wxPoint pinpos = GetPosition();
    pinpos.y = -pinpos.y;   // Display coord are top to bottom
                            // lib items coord are bottom to top

    text = MessageTextFromValue( aFrame->GetUserUnits(), pinpos.x );
    aList.push_back( MSG_PANEL_ITEM( _( "Pos X" ), text ) );

    text = MessageTextFromValue( aFrame->GetUserUnits(), pinpos.y );
    aList.push_back( MSG_PANEL_ITEM( _( "Pos Y" ), text ) );
}


const EDA_RECT LIB_PIN::GetBoundingBox( bool aIncludeInvisibles, bool aPinOnly ) const
{
    EDA_RECT       bbox;
    wxPoint        begin;
    wxPoint        end;
    int            nameTextOffset = 0;
    bool           showName = !m_name.IsEmpty() && ( m_name != wxT( "~" ) );
    bool           showNum = !m_number.IsEmpty();
    int            minsizeV = TARGET_PIN_RADIUS;

    if( !aIncludeInvisibles && !IsVisible() )
        showName = false;

    if( GetParent() )
    {
        if( GetParent()->ShowPinNames() )
            nameTextOffset = GetParent()->GetPinNameOffset();
        else
            showName = false;

        if( !GetParent()->ShowPinNumbers() )
            showNum = false;
    }

    if( aPinOnly )
    {
        showName = false;
        showNum = false;
    }

    // First, calculate boundary box corners position
    int numberTextLength = showNum ? m_numTextSize * m_number.Len() : 0;

    // Actual text height is bigger than text size
    int numberTextHeight  = showNum ? KiROUND( m_numTextSize * 1.1 ) : 0;

    if( m_shape == GRAPHIC_PINSHAPE::INVERTED || m_shape == GRAPHIC_PINSHAPE::INVERTED_CLOCK )
        minsizeV = std::max( TARGET_PIN_RADIUS, externalPinDecoSize( nullptr, *this ) );

    // calculate top left corner position
    // for the default pin orientation (PIN_RIGHT)
    begin.y = std::max( minsizeV, numberTextHeight + Mils2iu( PIN_TEXT_MARGIN ) );
    begin.x = std::min( -TARGET_PIN_RADIUS, m_length - (numberTextLength / 2) );

    // calculate bottom right corner position and adjust top left corner position
    int nameTextLength = 0;
    int nameTextHeight = 0;

    if( showName )
    {
        int length = m_name.Len();

        // Don't count the line over text symbol.
        if( m_name.Left( 1 ) == wxT( "~" ) )
            length -= 1;

        nameTextLength = ( m_nameTextSize * length ) + nameTextOffset;

        // Actual text height are bigger than text size
        nameTextHeight = KiROUND( m_nameTextSize * 1.1 ) + Mils2iu( PIN_TEXT_MARGIN );
    }

    if( nameTextOffset )        // for values > 0, pin name is inside the body
    {
        end.x = m_length + nameTextLength + TARGET_PIN_RADIUS;
        end.y = std::min( -minsizeV, -nameTextHeight / 2 );
    }
    else        // if value == 0:
                // pin name is outside the body, and above the pin line
                // pin num is below the pin line
    {
        end.x   = std::max( m_length + TARGET_PIN_RADIUS, nameTextLength );
        end.y   = -begin.y;
        begin.y = std::max( minsizeV, nameTextHeight );
    }

    // Now, calculate boundary box corners position for the actual pin orientation
    int orient = PinDrawOrient( DefaultTransform );

    /* Calculate the pin position */
    switch( orient )
    {
    case PIN_UP:
        // Pin is rotated and texts positions are mirrored
        RotatePoint( &begin, wxPoint( 0, 0 ), -900 );
        RotatePoint( &end, wxPoint( 0, 0 ), -900 );
        break;

    case PIN_DOWN:
        RotatePoint( &begin, wxPoint( 0, 0 ), 900 );
        RotatePoint( &end, wxPoint( 0, 0 ), 900 );
        begin.x = -begin.x;
        end.x = -end.x;
        break;

    case PIN_LEFT:
        begin.x = -begin.x;
        end.x = -end.x;
        break;

    case PIN_RIGHT:
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


BITMAP_DEF LIB_PIN::GetMenuImage() const
{
    return ElectricalPinTypeGetBitmap( m_type );
}


wxString LIB_PIN::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    if( !m_name.IsEmpty() )
    {
        return wxString::Format( _( "Pin %s [%s, %s, %s]" ),
                                 m_number,
                                 m_name,
                                 GetElectricalTypeName(),
                                 PinShapeGetText( m_shape ) );
    }
    else
    {
        return wxString::Format( _( "Pin %s [%s, %s]" ),
                                 m_number,
                                 GetElectricalTypeName(),
                                 PinShapeGetText( m_shape ) );
    }
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

void LIB_PIN::CalcEdit( const wxPoint& aPosition )
{
    if( IsMoving() )
    {
        MoveTo( aPosition );
    }
}
