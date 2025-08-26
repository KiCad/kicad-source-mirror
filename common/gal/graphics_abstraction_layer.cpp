/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <advanced_config.h>
#include <gal/graphics_abstraction_layer.h>
#include <gal/definitions.h>
#include <font/font.h>

#include <math/util.h>      // for KiROUND

#include <cmath>

using namespace KIGFX;


GAL::GAL( GAL_DISPLAY_OPTIONS& aDisplayOptions ) :
        m_options( aDisplayOptions ),

        // m_currentNativeCursor is initialized with KICURSOR::DEFAULT value to avoid
        // if comparison with uninitialized value on SetNativeCursorStyle method.
        // Some classes inheriting from GAL has different SetNativeCursorStyle method
        // implementation and therefore it's called also on constructor
        // to change the value from DEFAULT to KICURSOR::ARROW
        m_currentNativeCursor( KICURSOR::DEFAULT )
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
    SetScreenDPI( ADVANCED_CFG::GetCfg().m_ScreenDPI );
    SetDepthRange( VECTOR2D( GAL::MIN_DEPTH, GAL::MAX_DEPTH ) );
    SetLayerDepth( 0.0 );
    SetFlip( false, false );
    SetLineWidth( 1.0f );
    SetMinLineWidth( 1.0f );
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
    m_crossHairMode = CROSS_HAIR_MODE::SMALL_CROSS;
    m_forceDisplayCursor = false;
    SetCursorEnabled( false );

    // Initialize the native widget to an arrow cursor
    SetNativeCursorStyle( KICURSOR::ARROW, false );

    // Initialize text properties
    ResetTextAttributes();

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
        m_gridStyle = m_options.m_gridStyle;
        refresh = true;
    }

    if( m_options.m_gridLineWidth != m_gridLineWidth )
    {
        m_gridLineWidth = m_options.m_scaleFactor * m_options.m_gridLineWidth + 0.25;
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

    if( m_options.GetCursorMode() != m_crossHairMode )
    {
        m_crossHairMode = m_options.GetCursorMode();
        refresh = true;
    }

    // tell the derived class if the base class needs an update or not
    return refresh;
}


void GAL::ResetTextAttributes()
{
     // Tiny but non-zero - this will always need setting
     // there is no built-in default
    SetGlyphSize( { 1, 1 } );

    SetHorizontalJustify( GR_TEXT_H_ALIGN_CENTER );
    SetVerticalJustify( GR_TEXT_V_ALIGN_CENTER );

    SetFontBold( false );
    SetFontItalic( false );
    SetFontUnderlined( false );
    SetTextMirrored( false );
}


void GAL::ComputeWorldScreenMatrix()
{
    computeWorldScale();

    MATRIX3x3D translation;
    translation.SetIdentity();
    // We're deliberately dividing integers to avoid fractional pixel offsets.
    translation.SetTranslation( VECTOR2D( m_screenSize.x/2, m_screenSize.y/2 ) );

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


BOX2D GAL::GetVisibleWorldExtents() const
{
    const MATRIX3x3D& matrix = GetScreenWorldMatrix();

    VECTOR2D halfSize = VECTOR2D( matrix.GetScale().x * m_screenSize.x * 0.5,
                                  matrix.GetScale().y * m_screenSize.y * 0.5 );

    BOX2D extents;
    extents.SetOrigin( GetLookAtPoint() - halfSize );
    extents.SetSize( halfSize * 2 );

    return extents;
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
    return VECTOR2D( KiROUND( ( aPoint.x - m_gridOffset.x ) / m_gridSize.x ) *
                     m_gridSize.x + m_gridOffset.x,
                     KiROUND( ( aPoint.y - m_gridOffset.y ) / m_gridSize.y ) *
                     m_gridSize.y + m_gridOffset.y );
#else
    // if grid size == 0.0 there is no grid, so use aPoint as grid reference position
    double cx = m_gridSize.x > 0.0 ? KiROUND( ( aPoint.x - m_gridOffset.x ) / m_gridSize.x ) *
                m_gridSize.x + m_gridOffset.x
                                   : aPoint.x;
    double cy = m_gridSize.y > 0.0 ? KiROUND( ( aPoint.y - m_gridOffset.y ) / m_gridSize.y ) *
                                     m_gridSize.y + m_gridOffset.y
                                   : aPoint.y;

    return VECTOR2D( cx, cy );
#endif
}


// MIN_DEPTH must be set to be - (VIEW::VIEW_MAX_LAYERS + abs(VIEW::TOP_LAYER_MODIFIER))
// MAX_DEPTH must be set to be VIEW::VIEW_MAX_LAYERS + abs(VIEW::TOP_LAYER_MODIFIER) -1
// VIEW_MAX_LAYERS and TOP_LAYER_MODIFIER are defined in view.h.
// TOP_LAYER_MODIFIER is set as -VIEW_MAX_LAYERS
// Currently KIGFX::VIEW::VIEW_MAX_LAYERS = MAX_LAYERS_FOR_VIEW
const int GAL::MIN_DEPTH = -2*MAX_LAYERS_FOR_VIEW;
const int GAL::MAX_DEPTH = 2*MAX_LAYERS_FOR_VIEW - 1;
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


void GAL::BitmapText( const wxString& aText, const VECTOR2I& aPosition, const EDA_ANGLE& aAngle )
{
    KIFONT::FONT* font = KIFONT::FONT::GetFont();

    if( aText.IsEmpty() )
        return;

    TEXT_ATTRIBUTES attrs = m_attributes;
    attrs.m_Angle = aAngle;
    attrs.m_Mirrored = m_globalFlipX;    // Prevent text flipping when view is flipped

    // Bitmap font has different metrics than the stroke font so we compensate a bit before
    // stroking
    attrs.m_Size = VECTOR2I( m_attributes.m_Size.x, m_attributes.m_Size.y * 0.95 );
    attrs.m_StrokeWidth = GetLineWidth() * 0.74;

    font->Draw( this, aText, aPosition, attrs, KIFONT::METRICS::Default() );
}


bool GAL::SetNativeCursorStyle( KICURSOR aCursor, bool aHiDPI )
{
    if( m_currentNativeCursor == aCursor )
        return false;

    m_currentNativeCursor = aCursor;

    return true;
}
