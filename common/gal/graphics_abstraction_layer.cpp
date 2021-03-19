/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2012-2017 Kicad Developers, see change_log.txt for contributors.
 *
 * Graphics Abstraction Layer (GAL) - base class
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

#include <wx/log.h>

#include <gal/graphics_abstraction_layer.h>
#include <gal/definitions.h>

#include <math/util.h>      // for KiROUND

#include <cmath>

using namespace KIGFX;


GAL::GAL( GAL_DISPLAY_OPTIONS& aDisplayOptions ) :
        m_options( aDisplayOptions ),
        m_strokeFont( this )
{
    // Set the default values for the internal variables
    SetIsFill( false );
    SetIsStroke( true );
    SetFillColor( COLOR4D( 0.0, 0.0, 0.0, 0.0 ) );
    SetStrokeColor( COLOR4D( 1.0, 1.0, 1.0, 1.0 ) );
    SetLookAtPoint( VECTOR2D( 0, 0 ) );
    SetZoomFactor( 1.0 );
    SetRotation( 0.0 );
    // this value for SetWorldUnitLength is only suitable for Pcbnew.
    // Other editors/viewer must call SetWorldUnitLength with their internal units
    SetWorldUnitLength( 1e-9 /* 1 nm */ / 0.0254 /* 1 inch in meters */ );
    // wxDC::GetPPI() reports 96 DPI, but somehow this value
    // is the closest match to the legacy renderer
    SetScreenDPI( 91 );
    SetDepthRange( VECTOR2D( GAL::MIN_DEPTH, GAL::MAX_DEPTH ) );
    SetLayerDepth( 0.0 );
    SetFlip( false, false );
    SetLineWidth( 1.0f );
    computeWorldScale();
    SetAxesEnabled( false );

    // Set grid defaults
    SetGridVisibility( true );
    SetCoarseGrid( 10 );
    m_gridLineWidth = 0.5f;
    m_gridStyle = GRID_STYLE::LINES;
    m_gridMinSpacing = 10;

    // Initialize the cursor shape
    SetCursorColor( COLOR4D( 1.0, 1.0, 1.0, 1.0 ) );
    m_fullscreenCursor = false;
    m_forceDisplayCursor = false;
    SetCursorEnabled( false );

    // Initialize text properties
    ResetTextAttributes();

    m_strokeFont.LoadNewStrokeFont( newstroke_font, newstroke_font_bufsize );

    // subscribe for settings updates
    m_observerLink = m_options.Subscribe( this );
}


GAL::~GAL()
{
}


void GAL::OnGalDisplayOptionsChanged( const GAL_DISPLAY_OPTIONS& aOptions )
{
    // defer to the child class first
    updatedGalDisplayOptions( aOptions );

    // there is no refresh to do at this level
}


bool GAL::updatedGalDisplayOptions( const GAL_DISPLAY_OPTIONS& aOptions )
{
    bool refresh = false;

    if( m_options.m_gridStyle != m_gridStyle )
    {
        m_gridStyle = m_options.m_gridStyle ;
        refresh = true;
    }

    if( m_options.m_gridLineWidth != m_gridLineWidth )
    {
        m_gridLineWidth = std::floor( m_options.m_gridLineWidth + 0.5 );
        refresh = true;
    }

    if( m_options.m_gridMinSpacing != m_gridMinSpacing )
    {
        m_gridMinSpacing = m_options.m_gridMinSpacing;
        refresh = true;
    }

    if( m_options.m_axesEnabled != m_axesEnabled )
    {
        m_axesEnabled = m_options.m_axesEnabled;
        refresh = true;
    }

    if( m_options.m_forceDisplayCursor != m_forceDisplayCursor )
    {
        m_forceDisplayCursor = m_options.m_forceDisplayCursor;
        refresh = true;
    }

    if( m_options.m_fullscreenCursor != m_fullscreenCursor )
    {
        m_fullscreenCursor = m_options.m_fullscreenCursor;
        refresh = true;
    }

    // tell the derived class if the base class needs an update or not
    return refresh;
}


void GAL::SetTextAttributes( const EDA_TEXT* aText )
{
    SetGlyphSize( VECTOR2D( aText->GetTextSize() ) );
    SetHorizontalJustify( aText->GetHorizJustify() );
    SetVerticalJustify( aText->GetVertJustify() );
    SetFontBold( aText->IsBold() );
    SetFontItalic( aText->IsItalic() );
    SetFontUnderlined( false );
    SetTextMirrored( aText->IsMirrored() );
}


void GAL::ResetTextAttributes()
{
     // Tiny but non-zero - this will always need setting
     // there is no built-in default
    SetGlyphSize( { 1.0, 1.0 } );

    SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
    SetVerticalJustify( GR_TEXT_VJUSTIFY_CENTER );

    SetFontBold( false );
    SetFontItalic( false );
    SetFontUnderlined( false );
    SetTextMirrored( false );
}


VECTOR2D GAL::GetTextLineSize( const UTF8& aText ) const
{
    // Compute the X and Y size of a given text.
    // Because computeTextLineSize expects a one line text,
    // aText is expected to be only one line text.
    return m_strokeFont.computeTextLineSize( aText );
}


void GAL::ComputeWorldScreenMatrix()
{
    computeWorldScale();

    MATRIX3x3D translation;
    translation.SetIdentity();
    translation.SetTranslation( 0.5 * VECTOR2D( m_screenSize ) );

    MATRIX3x3D rotate;
    rotate.SetIdentity();
    rotate.SetRotation( m_rotation );

    MATRIX3x3D scale;
    scale.SetIdentity();
    scale.SetScale( VECTOR2D( m_worldScale, m_worldScale ) );

    MATRIX3x3D flip;
    flip.SetIdentity();
    flip.SetScale( VECTOR2D( m_globalFlipX ? -1.0 : 1.0, m_globalFlipY ? -1.0 : 1.0 ) );

    MATRIX3x3D lookat;
    lookat.SetIdentity();
    lookat.SetTranslation( -m_lookAtPoint );

    m_worldScreenMatrix = translation * rotate * flip * scale * lookat;
    m_screenWorldMatrix = m_worldScreenMatrix.Inverse();
}


double GAL::computeMinGridSpacing() const
{
    // just return the current value. This could be cleverer and take
    // into account other settings in future
    return m_gridMinSpacing;
}


VECTOR2D GAL::GetGridPoint( const VECTOR2D& aPoint ) const
{
#if 0
    // This old code expects a non zero grid size, which can be wrong here.
    return VECTOR2D( KiROUND( ( aPoint.x - m_gridOffset.x ) / m_gridSize.x ) * m_gridSize.x + m_gridOffset.x,
                     KiROUND( ( aPoint.y - m_gridOffset.y ) / m_gridSize.y ) * m_gridSize.y + m_gridOffset.y );
#else
    // if grid size == 0.0 there is no grid, so use aPoint as grid reference position
    double cx = m_gridSize.x > 0.0 ? KiROUND( ( aPoint.x - m_gridOffset.x ) / m_gridSize.x ) * m_gridSize.x + m_gridOffset.x
                                   : aPoint.x;
    double cy = m_gridSize.y > 0.0 ? KiROUND( ( aPoint.y - m_gridOffset.y ) / m_gridSize.y ) * m_gridSize.y + m_gridOffset.y
                                   : aPoint.y;

    return VECTOR2D( cx, cy );
#endif
}

const int GAL::MIN_DEPTH = -1024;
const int GAL::MAX_DEPTH = 1023;
const int GAL::GRID_DEPTH = MAX_DEPTH - 1;


COLOR4D GAL::getCursorColor() const
{
    COLOR4D color = m_cursorColor;

    // dim the cursor if it's only on because it was forced
    // (this helps to provide a hint for active tools)
    if( !m_isCursorEnabled )
        color.a = color.a * 0.5;

    return color;
}
