/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2021 Kicad Developers, see change_log.txt for contributors.
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
#include <layer_ids.h>
#include <painter.h>
#include <view/view.h>
#include <trigo.h>

using namespace KIGFX::PREVIEW;

static const double maxTickDensity = 10.0;       // min pixels between tick marks
static const double midTickLengthFactor = 1.5;
static const double majorTickLengthFactor = 2.5;


/*
 * It would be nice to know why Cairo seems to have an opposite layer order from GAL, but
 * only when drawing RULER_ITEMs (the TWO_POINT_ASSISTANT and ARC_ASSISTANT are immune from
 * this issue).
 *
 * Until then, this egregious hack...
 */
static int getShadowLayer( KIGFX::GAL* aGal )
{
    if( aGal->IsCairoEngine() )
        return LAYER_SELECT_OVERLAY;
    else
        return LAYER_GP_OVERLAY;
}


static void drawCursorStrings( KIGFX::VIEW* aView, const VECTOR2D& aCursor,
                               const VECTOR2D& aRulerVec, EDA_UNITS aUnits,
                               bool aDrawingDropShadows, bool aFlipX, bool aFlipY )
{
    // draw the cursor labels
    std::vector<wxString> cursorStrings;

    VECTOR2D temp = aRulerVec;

    if( aFlipX )
        temp.x = -temp.x;

    if( aFlipY )
        temp.y = -temp.y;

    cursorStrings.push_back( DimensionLabel( "x", temp.x, aUnits ) );
    cursorStrings.push_back( DimensionLabel( "y", temp.y, aUnits ) );

    cursorStrings.push_back( DimensionLabel( "r", aRulerVec.EuclideanNorm(), aUnits ) );

    double degs = RAD2DECIDEG( -aRulerVec.Angle() );
    cursorStrings.push_back( DimensionLabel( wxString::FromUTF8( "θ" ), degs,
                                             EDA_UNITS::DEGREES ) );

    temp = aRulerVec;
    DrawTextNextToCursor( aView, aCursor, -temp, cursorStrings, aDrawingDropShadows );
}


static double getTickLineWidth( const TEXT_DIMS& textDims, bool aDrawingDropShadows )
{
    double width = textDims.StrokeWidth * 0.8;

    if( aDrawingDropShadows )
        width += textDims.ShadowWidth;

    return width;
}


/**
 * Description of a "tick format" for a scale factor - how many ticks there are
 * between medium/major ticks and how each scale relates to the last one
 */
struct TICK_FORMAT
{
    double divisionBase;    ///< multiple from the last scale
    int majorStep;          ///< ticks between major ticks
    int midStep;            ///< ticks between medium ticks (0 if no medium ticks)
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

    // Convert to a round (mod-10) number of mils for imperial units
    if( EDA_UNIT_UTILS::IsImperialUnit( aUnits ) )
        aTickSpace *= 2.54;

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
 * maximum density. Minor ticks are not labelled.
 *
 * @param aGal the GAL to draw on
 * @param aOrigin start of line to draw ticks on
 * @param aLine line vector
 * @param aMinorTickLen length of minor ticks in IU
 */
void drawTicksAlongLine( KIGFX::VIEW* aView, const VECTOR2D& aOrigin, const VECTOR2D& aLine,
                         double aMinorTickLen, EDA_UNITS aUnits, bool aDrawingDropShadows )
{
    KIGFX::GAL* gal = aView->GetGAL();
    VECTOR2D    tickLine = aLine.Rotate( -M_PI_2 );
    double      tickSpace;
    TICK_FORMAT tickF = getTickFormatForScale( gal->GetWorldScale(), tickSpace, aUnits );

    // number of ticks in whole ruler
    int         numTicks = (int) std::ceil( aLine.EuclideanNorm() / tickSpace );

    // work out which way up the tick labels go
    TEXT_DIMS   textDims = SetConstantGlyphHeight( gal, -1 );
    double      textThickness = textDims.StrokeWidth;
    double      labelAngle = -tickLine.Angle();
    double      textOffset = 0;

    if( aDrawingDropShadows )
    {
        textOffset = textDims.ShadowWidth;
        textThickness += 2 * textDims.ShadowWidth;
    }

    double majorTickLen = aMinorTickLen * ( majorTickLengthFactor + 1 );
    VECTOR2D labelOffset = tickLine.Resize( majorTickLen - textOffset );

    if( aView->IsMirroredX() )
    {
        textOffset = -textOffset;
        labelOffset = -labelOffset;
    }

    if( aLine.Angle() > 0 )
    {
        gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_LEFT );
    }
    else
    {
        gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_RIGHT );
        labelAngle += M_PI;
    }

    BOX2D viewportD = aView->GetViewport();
    BOX2I viewport( VECTOR2I( viewportD.GetPosition() ), VECTOR2I( viewportD.GetSize() ) );

    viewport.Inflate( majorTickLen * 2 );   // Doesn't have to be accurate, just big enough not
                                            // to exclude anything that should be partially drawn

    for( int i = 0; i < numTicks; ++i )
    {
        const VECTOR2D tickPos = aOrigin + aLine.Resize( tickSpace * i );

        if( !viewport.Contains( tickPos ) )
            continue;

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

        gal->SetLineWidth( textThickness / 2 );
        gal->DrawLine( tickPos, tickPos + tickLine.Resize( length ) );

        if( drawLabel )
        {
            wxString label = DimensionLabel( "", tickSpace * i, aUnits, false );
            gal->SetLineWidth( textThickness );
            gal->StrokeText( label, tickPos + labelOffset, labelAngle );
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
void drawBacksideTicks( KIGFX::VIEW* aView, const VECTOR2D& aOrigin, const VECTOR2D& aLine,
                        double aTickLen, int aNumDivisions, bool aDrawingDropShadows )
{
    KIGFX::GAL*    gal = aView->GetGAL();
    const double   backTickSpace = aLine.EuclideanNorm() / aNumDivisions;
    const VECTOR2D backTickVec = aLine.Rotate( M_PI_2 ).Resize( aTickLen );
    TEXT_DIMS      textDims = SetConstantGlyphHeight( gal, -1 );

    BOX2D viewportD = aView->GetViewport();
    BOX2I viewport( VECTOR2I( viewportD.GetPosition() ), VECTOR2I( viewportD.GetSize() ) );

    viewport.Inflate( aTickLen * 4 );   // Doesn't have to be accurate, just big enough not to
                                        // exclude anything that should be partially drawn

    for( int i = 0; i < aNumDivisions + 1; ++i )
    {
        const VECTOR2D backTickPos = aOrigin + aLine.Resize( backTickSpace * i );

        if( !viewport.Contains( backTickPos ) )
            continue;

        gal->SetLineWidth( getTickLineWidth( textDims, aDrawingDropShadows ) );
        gal->DrawLine( backTickPos, backTickPos + backTickVec );
    }
}


RULER_ITEM::RULER_ITEM( const TWO_POINT_GEOMETRY_MANAGER& aGeomMgr, EDA_UNITS userUnits,
        bool aFlipX, bool aFlipY )
        : EDA_ITEM( NOT_USED ), // Never added to anything - just a preview
          m_geomMgr( aGeomMgr ),
          m_userUnits( userUnits ),
          m_flipX( aFlipX ),
          m_flipY( aFlipY )
{
}


const BOX2I RULER_ITEM::ViewBBox() const
{
    BOX2I tmp;

    if( m_geomMgr.GetOrigin() == m_geomMgr.GetEnd() )
        return tmp;

    // this is an edit-time artefact; no reason to try and be smart with the bounding box
    // (besides, we can't tell the text extents without a view to know what the scale is)
    tmp.SetMaximum();
    return tmp;
}


void RULER_ITEM::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aLayers[0] = LAYER_SELECT_OVERLAY;
    aLayers[1] = LAYER_GP_OVERLAY;
    aCount = 2;
}


void RULER_ITEM::ViewDraw( int aLayer, KIGFX::VIEW* aView ) const
{
    KIGFX::GAL*      gal = aView->GetGAL();
    RENDER_SETTINGS* rs = aView->GetPainter()->GetSettings();
    bool             drawingDropShadows = ( aLayer == getShadowLayer( gal ) );

    gal->PushDepth();
    gal->SetLayerDepth( gal->GetMinDepth() );

    VECTOR2D origin = m_geomMgr.GetOrigin();
    VECTOR2D end = m_geomMgr.GetEnd();

    gal->SetIsStroke( true );
    gal->SetIsFill( false );

    gal->SetTextMirrored( false );
    gal->SetStrokeColor( rs->GetLayerColor( LAYER_AUX_ITEMS ) );

    if( drawingDropShadows )
        gal->SetStrokeColor( GetShadowColor( gal->GetStrokeColor() ) );

    gal->ResetTextAttributes();
    TEXT_DIMS textDims = SetConstantGlyphHeight( gal );

    // draw the main line from the origin to cursor
    gal->SetLineWidth( getTickLineWidth( textDims, drawingDropShadows ) );
    gal->DrawLine( origin, end );

    VECTOR2D rulerVec( end - origin );

    drawCursorStrings( aView, end, rulerVec, m_userUnits, drawingDropShadows, m_flipX, m_flipY );

    // basic tick size
    const double minorTickLen = 5.0 / gal->GetWorldScale();
    const double majorTickLen = minorTickLen * majorTickLengthFactor;

    drawTicksAlongLine( aView, origin, rulerVec, minorTickLen, m_userUnits, drawingDropShadows );

    drawBacksideTicks( aView, origin, rulerVec, majorTickLen, 2, drawingDropShadows );

    // draw the back of the origin "crosshair"
    gal->DrawLine( origin, origin + rulerVec.Resize( -minorTickLen * midTickLengthFactor ) );
    gal->PopDepth();
}
