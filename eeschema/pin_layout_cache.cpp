/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "pin_layout_cache.h"

#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <sch_symbol.h>
#include <eeschema_settings.h>

// small margin in internal units between the pin text and the pin line
#define PIN_TEXT_MARGIN 4

struct EXTENTS_CACHE
{
    KIFONT::FONT* m_Font = nullptr;
    int           m_FontSize = 0;
    VECTOR2I      m_Extents;
};

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


void PIN_LAYOUT_CACHE::recomputeExtentsCache( bool aDefinitelyDirty, KIFONT::FONT* aFont, int aSize,
                                              const wxString&        aText,
                                              const KIFONT::METRICS& aFontMetrics,
                                              TEXT_EXTENTS_CACHE&    aCache )
{
    // Even if not definitely dirty, verify no font changes
    if( !aDefinitelyDirty && aCache.m_Font == aFont && aCache.m_FontSize == aSize )
    {
        return;
    }

    aCache.m_Font = aFont;
    aCache.m_FontSize = aSize;

    VECTOR2D fontSize( aSize, aSize );
    int      penWidth = GetPenSizeForNormal( aSize );

    aCache.m_Extents = aFont->StringBoundaryLimits( aText, fontSize, penWidth, false, false,
                                                     aFontMetrics);
}


PIN_LAYOUT_CACHE::PIN_LAYOUT_CACHE( const SCH_PIN& aPin ) :
        m_pin( aPin ), m_dirtyFlags( DIRTY_FLAGS::ALL )
{
}


void PIN_LAYOUT_CACHE::MarkDirty( int aDirtyFlags )
{
    m_dirtyFlags |= aDirtyFlags;
}


BOX2I PIN_LAYOUT_CACHE::GetPinBoundingBox( bool aIncludeLabelsOnInvisiblePins,
                                           bool aIncludeNameAndNumber, bool aIncludeElectricalType )
{
    if( const SCH_SYMBOL* symbol = dynamic_cast<const SCH_SYMBOL*>( m_pin.GetParentSymbol() ) )
    {
        SCH_PIN* const libPin = m_pin.GetLibPin();
        wxCHECK( libPin, BOX2I() );

        BOX2I r = libPin->GetBoundingBox( aIncludeLabelsOnInvisiblePins, aIncludeNameAndNumber,
                                          aIncludeElectricalType );

        r = symbol->GetTransform().TransformCoordinate( r );
        r.Offset( symbol->GetPosition() );
        r.Normalize();

        return r;
    }

    EESCHEMA_SETTINGS* cfg = Pgm().GetSettingsManager().GetAppSettings<EESCHEMA_SETTINGS>();
    KIFONT::FONT*      font = KIFONT::FONT::GetFont( cfg->m_Appearance.default_font );

    VECTOR2I begin;
    VECTOR2I end;
    int      pinNameOffset = 0;
    int      nameTextLength = 0;
    int      nameTextHeight = 0;
    int      numberTextLength = 0;
    int      numberTextHeight = 0;
    int      typeTextLength = 0;
    bool     includeName = aIncludeNameAndNumber && !m_pin.GetShownName().IsEmpty();
    bool     includeNumber = aIncludeNameAndNumber && !m_pin.GetShownNumber().IsEmpty();
    bool     includeType = aIncludeElectricalType;
    int      minsizeV = TARGET_PIN_RADIUS;

    if( !aIncludeLabelsOnInvisiblePins && !m_pin.IsVisible() )
    {
        includeName = false;
        includeNumber = false;
        includeType = false;
    }

    if( const SYMBOL* parentSymbol = m_pin.GetParentSymbol() )
    {
        if( parentSymbol->GetShowPinNames() )
            pinNameOffset = parentSymbol->GetPinNameOffset();
        else
            includeName = false;

        if( !parentSymbol->GetShowPinNumbers() )
            includeNumber = false;
    }

    const KIFONT::METRICS& metrics = m_pin.GetFontMetrics();

    if( includeNumber )
    {
        recomputeExtentsCache( isDirty( DIRTY_FLAGS::NUMBER ), font, m_pin.GetNumberTextSize(),
                               m_pin.GetShownNumber(), metrics, m_numExtentsCache );
        setClean( DIRTY_FLAGS::NUMBER );

        numberTextLength = m_numExtentsCache.m_Extents.x;
        numberTextHeight = m_numExtentsCache.m_Extents.y;
    }

    if( includeName )
    {
        recomputeExtentsCache( isDirty( DIRTY_FLAGS::NAME ), font, m_pin.GetNameTextSize(),
                               m_pin.GetShownName(), metrics, m_nameExtentsCache );
        setClean( DIRTY_FLAGS::NAME );

        nameTextLength = m_nameExtentsCache.m_Extents.x + pinNameOffset;
        nameTextHeight = m_nameExtentsCache.m_Extents.y + schIUScale.MilsToIU( PIN_TEXT_MARGIN );
    }

    if( includeType )
    {
        // TODO: cache this
        double   fontSize = std::max( m_pin.GetNameTextSize() * 3 / 4, schIUScale.mmToIU( 0.7 ) );
        double   stroke = fontSize / 8.0;
        VECTOR2I typeTextSize = font->StringBoundaryLimits( m_pin.GetElectricalTypeName(),
                                                            VECTOR2D( fontSize, fontSize ),
                                                            KiROUND( stroke ), false, false,
                                                            metrics );

        typeTextLength = typeTextSize.x + schIUScale.MilsToIU( PIN_TEXT_MARGIN ) + TARGET_PIN_RADIUS;
        minsizeV = std::max( minsizeV, typeTextSize.y / 2 );
    }

    // First, calculate boundary box corners position
    if( m_pin.GetShape() == GRAPHIC_PINSHAPE::INVERTED || m_pin.GetShape() == GRAPHIC_PINSHAPE::INVERTED_CLOCK )
        minsizeV = std::max( TARGET_PIN_RADIUS, externalPinDecoSize( nullptr, m_pin ) );

    const int pinLength = m_pin.GetLength();

    // Calculate topLeft & bottomRight corner positions for the default pin orientation (PIN_RIGHT)
    if( pinNameOffset || !includeName )
    {
        // pin name is inside the body (or invisible)
        // pin number is above the line
        begin.y = std::min( -minsizeV, -numberTextHeight );
        begin.x = std::min( -typeTextLength, pinLength - ( numberTextLength / 2 ) );

        end.x = pinLength + nameTextLength;
        end.y = std::max( minsizeV, nameTextHeight / 2 );
    }
    else
    {
        // pin name is above pin line
        // pin number is below line
        begin.y = std::min( -minsizeV, -nameTextHeight );
        begin.x = -typeTextLength;
        begin.x = std::min( begin.x, ( pinLength - numberTextLength ) / 2 );
        begin.x = std::min( begin.x, ( pinLength - nameTextLength ) / 2 );

        end.x = pinLength;
        end.x = std::max( end.x, ( pinLength + nameTextLength ) / 2 );
        end.x = std::max( end.x, ( pinLength + numberTextLength ) / 2 );
        end.y = std::max( minsizeV, numberTextHeight );
    }

    // Now, calculate boundary box corners position for the actual pin orientation
    switch( m_pin.PinDrawOrient( DefaultTransform ) )
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

    BOX2I bbox = BOX2I::ByCorners( begin, end );
    bbox.Move( m_pin.GetPosition() );

    if( m_pin.IsDangling() )
    {
        // Not much point caching this
        const CIRCLE c = GetDanglingIndicator();

        BOX2I cBox = BOX2I::ByCenter( c.Center, { c.Radius * 2, c.Radius * 2 } );
        // TODO: need some way to find the thickness...?
        // cBox.Inflate( ??? );

        bbox.Merge( cBox );
    }

    bbox.Normalize();
    bbox.Inflate( ( m_pin.GetPenWidth() / 2 ) + 1 );

    return bbox;
}


CIRCLE PIN_LAYOUT_CACHE::GetDanglingIndicator() const
{
    return CIRCLE{
        m_pin.GetPosition(),
        TARGET_PIN_RADIUS,
    };
}