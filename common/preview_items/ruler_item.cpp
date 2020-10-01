/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Kicad Developers, see change_log.txt for contributors.
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

#include <preview_items/ruler_item.h>

#include <preview_items/preview_utils.h>
#include <gal/graphics_abstraction_layer.h>
#include <layers_id_colors_and_visibility.h>
#include <painter.h>
#include <view/view.h>

#include <common.h>

using namespace KIGFX::PREVIEW;

static const double maxTickDensity = 10.0;       // min pixels between tick marks
static const double midTickLengthFactor = 1.5;
static const double majorTickLengthFactor = 2.5;

// We need a pair of layers for the graphics and drop-shadows, but it's probably not
// worth adding more layers to the enum.
#define LAYER_RULER LAYER_SELECT_OVERLAY
#define LAYER_RULER_SHADOWS LAYER_GP_OVERLAY


static void drawCursorStrings( int aLayer, KIGFX::VIEW* aView, const VECTOR2D& aCursor,
                               const VECTOR2D& aRulerVec, EDA_UNITS aUnits )
{
    // draw the cursor labels
    std::vector<wxString> cursorStrings;

    cursorStrings.push_back( DimensionLabel( "x", aRulerVec.x, aUnits ) );
    cursorStrings.push_back( DimensionLabel( "y", aRulerVec.y, aUnits ) );

    cursorStrings.push_back( DimensionLabel( "r", aRulerVec.EuclideanNorm(), aUnits ) );

    double degs = RAD2DECIDEG( -aRulerVec.Angle() );
    cursorStrings.push_back( DimensionLabel( wxString::FromUTF8( "θ" ), degs,
                                             EDA_UNITS::DEGREES ) );

    auto temp = aRulerVec;
    DrawTextNextToCursor( aView, aCursor, -temp, cursorStrings, aLayer == LAYER_RULER_SHADOWS );
}


static double getTickLineWidth( const TEXT_DIMS& textDims, int aLayer )
{
    double width = textDims.StrokeWidth * 0.8;

    if( aLayer == LAYER_RULER_SHADOWS )
        width += textDims.ShadowWidth;

    return width;
}


/**
 * Description of a "tick format" for a scale factor - how many ticks there are
 * between medium/major ticks and how each scale relates to the last one
 */
struct TICK_FORMAT
{
    double divisionBase;    ///> multiple from the last scale
    int majorStep;          ///> ticks between major ticks
    int midStep;            ///> ticks between medium ticks (0 if no medium ticks)
};


static TICK_FORMAT getTickFormatForScale( double aScale, double& aTickSpace, EDA_UNITS aUnits )
{
    // simple 1/2/5 scales per decade
    static std::vector<TICK_FORMAT> tickFormats =
    {
        { 2,    10,     5 },    // |....:....|
        { 2,     5,     0 },    // |....|
        { 2.5,   2,     0 },    // |.|.|
    };

    // could start at a set number of MM, but that's not available in common
    aTickSpace = 1;

    // convert to a round (mod-10) number of mils
    if( aUnits == EDA_UNITS::INCHES )
    {
        aTickSpace *= 2.54;
    }

    int tickFormat = 0;

    while( true )
    {
        const auto pixelSpace = aTickSpace * aScale;

        if( pixelSpace >= maxTickDensity )
            break;

        tickFormat = ( tickFormat + 1 ) % tickFormats.size();
        aTickSpace *= tickFormats[tickFormat].divisionBase;
    }

    return tickFormats[tickFormat];
}


/**
 * Draw labelled ticks on a line. Ticks are spaced according to a
 * maximum density. Miror ticks are not labelled.
 *
 * @param aGal the GAL to draw on
 * @param aOrigin start of line to draw ticks on
 * @param aLine line vector
 * @param aMinorTickLen length of minor ticks in IU
 */
void drawTicksAlongLine( int aLayer, KIGFX::GAL* aGal, const VECTOR2D& aOrigin,
                         const VECTOR2D& aLine, double aMinorTickLen, EDA_UNITS aUnits )
{
    VECTOR2D    tickLine = aLine.Rotate( -M_PI_2 );
    double      tickSpace;
    TICK_FORMAT tickF = getTickFormatForScale( aGal->GetWorldScale(), tickSpace, aUnits );

    // number of ticks in whole ruler
    int         numTicks = (int) std::ceil( aLine.EuclideanNorm() / tickSpace );

    // work out which way up the tick labels go
    TEXT_DIMS   textDims = SetConstantGlyphHeight( aGal, -1 );
    double      textThickness = textDims.StrokeWidth;
    double      labelAngle = -tickLine.Angle();
    double      textOffset = 0;

    if( aLayer == LAYER_RULER_SHADOWS )
    {
        // Drawing drop shadows
        textOffset = textDims.ShadowWidth;
        textThickness += 2 * textDims.ShadowWidth;
    }

    double majorTickLen = aMinorTickLen * ( majorTickLengthFactor + 1 );
    VECTOR2D labelOffset = tickLine.Resize( majorTickLen - textOffset );

    if( aLine.Angle() > 0 )
    {
        aGal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_LEFT );
    }
    else
    {
        aGal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_RIGHT );
        labelAngle += M_PI;
    }

    for( int i = 0; i < numTicks; ++i )
    {
        const auto tickPos = aOrigin + aLine.Resize( tickSpace * i );

        double length = aMinorTickLen;
        bool drawLabel = false;

        if( i % tickF.majorStep == 0 )
        {
            drawLabel = true;
            length *= majorTickLengthFactor;
        }
        else if( tickF.midStep && i % tickF.midStep == 0 )
        {
            drawLabel = true;
            length *= midTickLengthFactor;
        }

        aGal->SetLineWidth( textThickness / 2 );
        aGal->DrawLine( tickPos, tickPos + tickLine.Resize( length ) );

        if( drawLabel )
        {
            wxString label = DimensionLabel( "", tickSpace * i, aUnits, false );
            aGal->SetLineWidth( textThickness );
            aGal->StrokeText( label, tickPos + labelOffset, labelAngle );
        }
    }
}


/**
 * Draw simple ticks on the back of a line such that the line is
 * divided into n parts.
 *
 * @param aGal the GAL to draw on
 * @param aOrigin start of line to draw ticks on
 * @param aLine line vector
 * @param aTickLen length of ticks in IU
 * @param aNumDivisions number of parts to divide the line into
 */
void drawBacksideTicks( int aLayer, KIGFX::GAL* aGal, const VECTOR2D& aOrigin,
                        const VECTOR2D& aLine, double aTickLen, int aNumDivisions )
{
    const double   backTickSpace = aLine.EuclideanNorm() / aNumDivisions;
    const VECTOR2D backTickVec = aLine.Rotate( M_PI_2 ).Resize( aTickLen );
    TEXT_DIMS      textDims = SetConstantGlyphHeight( aGal, -1 );

    for( int i = 0; i < aNumDivisions + 1; ++i )
    {
        const VECTOR2D backTickPos = aOrigin + aLine.Resize( backTickSpace * i );
        aGal->SetLineWidth( getTickLineWidth( textDims, aLayer ) );
        aGal->DrawLine( backTickPos, backTickPos + backTickVec );
    }
}


RULER_ITEM::RULER_ITEM( const TWO_POINT_GEOMETRY_MANAGER& aGeomMgr, EDA_UNITS userUnits )
        : EDA_ITEM( NOT_USED ), // Never added to anything - just a preview
          m_geomMgr( aGeomMgr ),
          m_userUnits( userUnits )
{
}


const BOX2I RULER_ITEM::ViewBBox() const
{
    BOX2I tmp;

    tmp.SetOrigin( m_geomMgr.GetOrigin() );
    tmp.SetEnd( m_geomMgr.GetEnd() );
    tmp.Normalize();
    return tmp;
}


void RULER_ITEM::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aLayers[0] = LAYER_RULER;
    aLayers[1] = LAYER_RULER_SHADOWS;
    aCount = 2;
}


void RULER_ITEM::ViewDraw( int aLayer, KIGFX::VIEW* aView ) const
{
    KIGFX::GAL*      gal = aView->GetGAL();
    RENDER_SETTINGS* rs = aView->GetPainter()->GetSettings();

    gal->PushDepth();
    gal->SetLayerDepth( gal->GetMinDepth() );

    VECTOR2D origin = m_geomMgr.GetOrigin();
    VECTOR2D end = m_geomMgr.GetEnd();

    gal->SetIsStroke( true );
    gal->SetIsFill( false );

    gal->SetTextMirrored( false );
    gal->SetStrokeColor( rs->GetLayerColor( LAYER_AUX_ITEMS ) );

    if( aLayer == LAYER_RULER_SHADOWS )
        gal->SetStrokeColor( GetShadowColor( gal->GetStrokeColor() ) );

    gal->ResetTextAttributes();
    TEXT_DIMS textDims = SetConstantGlyphHeight( gal );

    // draw the main line from the origin to cursor
    gal->SetLineWidth( getTickLineWidth( textDims, aLayer ) );
    gal->DrawLine( origin, end );

    VECTOR2D rulerVec( end - origin );

    drawCursorStrings( aLayer, aView, end, rulerVec, m_userUnits );

    // basic tick size
    const double minorTickLen = 5.0 / gal->GetWorldScale();

    drawTicksAlongLine( aLayer, gal, origin, rulerVec, minorTickLen, m_userUnits );

    drawBacksideTicks( aLayer, gal, origin, rulerVec, minorTickLen * majorTickLengthFactor, 2 );

    // draw the back of the origin "crosshair"
    gal->DrawLine( origin, origin + rulerVec.Resize( -minorTickLen * midTickLengthFactor ) );
    gal->PopDepth();
}
