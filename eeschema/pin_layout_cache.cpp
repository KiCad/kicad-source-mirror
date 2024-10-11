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
#include <schematic_settings.h>

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
static int externalPinDecoSize( const SCHEMATIC_SETTINGS* aSettings, const SCH_PIN& aPin )
{
    if( aSettings && aSettings->m_PinSymbolSize )
        return aSettings->m_PinSymbolSize;

    return aPin.GetNumberTextSize() / 2;
}


static int internalPinDecoSize( const SCHEMATIC_SETTINGS* aSettings, const SCH_PIN& aPin )
{
    if( aSettings && aSettings->m_PinSymbolSize > 0 )
        return aSettings->m_PinSymbolSize;

    return aPin.GetNameTextSize() != 0 ? aPin.GetNameTextSize() / 2 : aPin.GetNumberTextSize() / 2;
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

    aCache.m_Extents =
            aFont->StringBoundaryLimits( aText, fontSize, penWidth, false, false, aFontMetrics );
}


PIN_LAYOUT_CACHE::PIN_LAYOUT_CACHE( const SCH_PIN& aPin ) :
        m_pin( aPin ), m_schSettings( nullptr ), m_dirtyFlags( DIRTY_FLAGS::ALL )
{
    // Resolve the schematic (can be null, e.g. in previews)
    const SCHEMATIC* schematic = aPin.Schematic();

    if( schematic )
    {
        m_schSettings = &schematic->Settings();
    }
}


void PIN_LAYOUT_CACHE::MarkDirty( int aDirtyFlags )
{
    m_dirtyFlags |= aDirtyFlags;
}


void PIN_LAYOUT_CACHE::recomputeCaches()
{
    EESCHEMA_SETTINGS*     cfg = Pgm().GetSettingsManager().GetAppSettings<EESCHEMA_SETTINGS>();
    KIFONT::FONT*          font = KIFONT::FONT::GetFont( cfg->m_Appearance.default_font );
    const KIFONT::METRICS& metrics = m_pin.GetFontMetrics();

    {
        recomputeExtentsCache( isDirty( DIRTY_FLAGS::NUMBER ), font, m_pin.GetNumberTextSize(),
                               m_pin.GetShownNumber(), metrics, m_numExtentsCache );
        setClean( DIRTY_FLAGS::NUMBER );
    }

    {
        recomputeExtentsCache( isDirty( DIRTY_FLAGS::NAME ), font, m_pin.GetNameTextSize(),
                               m_pin.GetShownName(), metrics, m_nameExtentsCache );
        setClean( DIRTY_FLAGS::NAME );
    }

    {
        double fontSize = std::max( m_pin.GetNameTextSize() * 3 / 4, schIUScale.mmToIU( 0.7 ) );
        recomputeExtentsCache( isDirty( DIRTY_FLAGS::ELEC_TYPE ), font, fontSize,
                               m_pin.GetElectricalTypeName(), metrics, m_typeExtentsCache );
        setClean( DIRTY_FLAGS::ELEC_TYPE );
    }
}


void PIN_LAYOUT_CACHE::transformBoxForPin( BOX2I& aBox ) const
{
    // Now, calculate boundary box corners position for the actual pin orientation
    switch( m_pin.PinDrawOrient( DefaultTransform ) )
    {
    case PIN_ORIENTATION::PIN_UP:
    {
        // Pin is rotated and texts positions are mirrored
        VECTOR2I c1{ aBox.GetLeft(), aBox.GetTop() };
        VECTOR2I c2{ aBox.GetRight(), aBox.GetBottom() };

        RotatePoint( c1, VECTOR2I( 0, 0 ), ANGLE_90 );
        RotatePoint( c2, VECTOR2I( 0, 0 ), ANGLE_90 );

        aBox = BOX2I::ByCorners( c1, c2 );
        break;
    }
    case PIN_ORIENTATION::PIN_DOWN:
    {
        VECTOR2I c1{ aBox.GetLeft(), aBox.GetTop() };
        VECTOR2I c2{ aBox.GetRight(), aBox.GetBottom() };

        RotatePoint( c1, VECTOR2I( 0, 0 ), -ANGLE_90 );
        RotatePoint( c2, VECTOR2I( 0, 0 ), -ANGLE_90 );

        c1.x = -c1.x;
        c2.x = -c2.x;

        aBox = BOX2I::ByCorners( c1, c2 );
        break;
    }
    case PIN_ORIENTATION::PIN_LEFT:
        // Flip it around
        aBox.Move( { -aBox.GetCenter().x * 2, 0 } );
        break;

    default:
    case PIN_ORIENTATION::PIN_RIGHT:
        // Already in this form
        break;
    }

    aBox.Move( m_pin.GetPosition() );
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

    VECTOR2I begin;
    VECTOR2I end;
    bool     includeName = aIncludeNameAndNumber && !m_pin.GetShownName().IsEmpty();
    bool     includeNumber = aIncludeNameAndNumber && !m_pin.GetShownNumber().IsEmpty();
    bool     includeType = aIncludeElectricalType;

    if( !aIncludeLabelsOnInvisiblePins && !m_pin.IsVisible() )
    {
        includeName = false;
        includeNumber = false;
        includeType = false;
    }

    if( const SYMBOL* parentSymbol = m_pin.GetParentSymbol() )
    {
        if( !parentSymbol->GetShowPinNames() )
            includeName = false;

        if( !parentSymbol->GetShowPinNumbers() )
            includeNumber = false;
    }

    recomputeCaches();

    const int pinLength = m_pin.GetLength();
    BOX2I     bbox;

    // Creating and merging all the boxes is pretty quick, if cached we'd have
    // to track many variables here, which is possible, but unlikely to be worth it.

    // Untranformed pin box
    {
        BOX2I pinBox = BOX2I::ByCorners( { 0, 0 }, { pinLength, 0 } );
        pinBox.Inflate( m_pin.GetPenWidth() / 2 );
        bbox.Merge( pinBox );
    }

    if( OPT_BOX2I decoBox = getUntransformedDecorationBox() )
    {
        bbox.Merge( *decoBox );
    }

    if( includeName )
    {
        if( OPT_BOX2I nameBox = getUntransformedPinNameBox() )
        {
            bbox.Merge( *nameBox );
        }
    }

    if( includeNumber )
    {
        if( OPT_BOX2I numBox = getUntransformedPinNumberBox( includeName ) )
        {
            bbox.Merge( *numBox );
        }
    }

    if( includeType )
    {
        bbox.Merge( getUntransformedPinTypeBox() );
    }

    transformBoxForPin( bbox );

    if( m_pin.IsDangling() )
    {
        // Not much point caching this, but we could
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


OPT_BOX2I PIN_LAYOUT_CACHE::getUntransformedPinNameBox() const
{
    int pinNameOffset = 0;
    if( const SYMBOL* parentSymbol = m_pin.GetParentSymbol() )
    {
        if( parentSymbol->GetShowPinNames() )
            pinNameOffset = parentSymbol->GetPinNameOffset();
    }

    // We're considering the PIN_RIGHT scenario
    //      TEXT
    //   X-------|  TEXT
    //      TEXT
    //
    // We'll rotate it later.

    OPT_BOX2I box;
    const int pinLength = m_pin.GetLength();

    if( pinNameOffset > 0 )
    {
        // This means name inside the pin
        box = BOX2I::ByCenter( { pinLength, 0 }, m_nameExtentsCache.m_Extents );

        // Bump over to be left aligned just inside the pin
        box->Move( { m_nameExtentsCache.m_Extents.x / 2 + pinNameOffset, 0 } );
    }
    else
    {
        // The pin name is always over the pin
        box = BOX2I::ByCenter( { pinLength / 2, 0 }, m_nameExtentsCache.m_Extents );

        float offsetRatio =
                m_schSettings ? m_schSettings->m_TextOffsetRatio : DEFAULT_TEXT_OFFSET_RATIO;

        const int offset = schIUScale.MilsToIU( KiROUND( 24 * offsetRatio ) );

        // Bump it up
        box->Move( { 0, -m_nameExtentsCache.m_Extents.y / 2 - offset } );
    }

    return box;
}


OPT_BOX2I PIN_LAYOUT_CACHE::getUntransformedPinNumberBox( bool aIncludeName ) const
{
    int pinNameOffset = 0;
    if( const SYMBOL* parentSymbol = m_pin.GetParentSymbol() )
    {
        if( parentSymbol->GetShowPinNames() )
            pinNameOffset = parentSymbol->GetPinNameOffset();
    }

    const int pinLength = m_pin.GetLength();

    // The pin name is always over the pin
    OPT_BOX2I box = BOX2I::ByCenter( { pinLength / 2, 0 }, m_numExtentsCache.m_Extents );

    float offsetRatio =
            m_schSettings ? m_schSettings->m_TextOffsetRatio : DEFAULT_TEXT_OFFSET_RATIO;
    const int offset = schIUScale.MilsToIU( KiROUND( 24 * offsetRatio ) );
    int       textPos = -m_numExtentsCache.m_Extents.y / 2 - offset;

    // The number goes below, if there is a name outside
    if( pinNameOffset == 0 && aIncludeName )
        textPos *= -1;

    // Bump it up (or down)
    box->Move( { 0, textPos } );

    return box;
}


BOX2I PIN_LAYOUT_CACHE::getUntransformedPinTypeBox() const
{
    BOX2I box{
        { -m_typeExtentsCache.m_Extents.x, -m_typeExtentsCache.m_Extents.y / 2 },
        m_typeExtentsCache.m_Extents,
    };

    // Jog left
    box.Move( { -schIUScale.MilsToIU( PIN_TEXT_MARGIN ) - TARGET_PIN_RADIUS, 0 } );

    return box;
}


OPT_BOX2I PIN_LAYOUT_CACHE::getUntransformedDecorationBox() const
{
    const GRAPHIC_PINSHAPE shape = m_pin.GetShape();
    const int              decoSize = externalPinDecoSize( m_schSettings, m_pin );
    const int              intDecoSize = internalPinDecoSize( m_schSettings, m_pin );

    const auto makeInvertBox = [&]()
    {
        return BOX2I::ByCenter( { -decoSize, 0 }, { decoSize * 2, decoSize * 2 } );
    };

    const auto makeLowBox = [&]()
    {
        return BOX2I::ByCorners( { -decoSize * 2, -decoSize * 2 }, { 0, 0 } );
    };

    const auto makeClockBox = [&]()
    {
        return BOX2I::ByCorners( { 0, -intDecoSize }, { intDecoSize, intDecoSize } );
    };

    OPT_BOX2I box;

    switch( shape )
    {
    case GRAPHIC_PINSHAPE::INVERTED:
    {
        box = makeInvertBox();
        break;
    }
    case GRAPHIC_PINSHAPE::CLOCK:
    {
        box = makeClockBox();
        break;
    }
    case GRAPHIC_PINSHAPE::INVERTED_CLOCK:
    {
        box = makeInvertBox();
        box->Merge( makeClockBox() );
        break;
    }
    case GRAPHIC_PINSHAPE::INPUT_LOW:
    {
        box = makeLowBox();
        break;
    }
    case GRAPHIC_PINSHAPE::FALLING_EDGE_CLOCK:
    case GRAPHIC_PINSHAPE::CLOCK_LOW:
    {
        box = makeLowBox();
        box->Merge( makeClockBox() );
        break;
    }
    case GRAPHIC_PINSHAPE::NONLOGIC:
    {
        box = BOX2I::ByCenter( { 0, 0 }, { decoSize * 2, decoSize * 2 } );
        break;
    }
    case GRAPHIC_PINSHAPE::LINE:
    default:
    {
        // No decoration
        break;
    }
    }

    if( box )
    {
        // Put the box at the root of the pin
        box->Move( { m_pin.GetLength(), 0 } );
        box->Inflate( m_pin.GetPenWidth() / 2 );
    }

    return box;
}


OPT_BOX2I PIN_LAYOUT_CACHE::GetPinNameBBox()
{
    recomputeCaches();
    OPT_BOX2I box = getUntransformedPinNameBox();

    if( box )
        transformBoxForPin( *box );

    return box;
}


OPT_BOX2I PIN_LAYOUT_CACHE::GetPinNumberBBox()
{
    recomputeCaches();
    OPT_BOX2I box = getUntransformedPinNumberBox( true );

    if( box )
        transformBoxForPin( *box );

    return box;
}