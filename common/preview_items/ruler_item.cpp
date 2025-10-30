/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <gal/painter.h>
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
 * Draw labeled ticks on a line. Ticks are spaced according to a
 * maximum density. Minor ticks are not labeled.
 *
 * @param aGal the GAL to draw on
 * @param aOrigin start of line to draw ticks on
 * @param aLine line vector
 * @param aMinorTickLen length of minor ticks in IU
 */
void drawTicksAlongLine( KIGFX::VIEW* aView, const VECTOR2D& aOrigin, const VECTOR2D& aLine,
                         double aMinorTickLen, const EDA_IU_SCALE& aIuScale, EDA_UNITS aUnits,
                         bool aDrawingDropShadows )
{
    KIGFX::GAL*   gal = aView->GetGAL();
    KIFONT::FONT* font = KIFONT::FONT::GetFont();
    double        tickSpace;
    TICK_FORMAT   tickFormat = getTickFormatForScale( gal->GetWorldScale(), tickSpace, aUnits );
    double        majorTickLen = aMinorTickLen * ( majorTickLengthFactor + 1 );
    VECTOR2D      tickLine = aLine;

    RotatePoint( tickLine, ANGLE_90 );

    // number of ticks in whole ruler
    int           numTicks = (int) std::ceil( aLine.EuclideanNorm() / tickSpace );

    // work out which way up the tick labels go
    TEXT_DIMS     labelDims = GetConstantGlyphHeight( gal, -1 );
    EDA_ANGLE     labelAngle = - EDA_ANGLE( tickLine );
    VECTOR2I      labelOffset = tickLine.Resize( majorTickLen );

    // text is left (or right) aligned, so shadow text need a small offset to be draw
    // around the basic text
    int shadowXoffset = 0;

    if( aDrawingDropShadows )
    {
        labelDims.StrokeWidth += 2 * labelDims.ShadowWidth;
        shadowXoffset = labelDims.ShadowWidth;

        // Due to the fact a shadow text is drawn left or right aligned,
        // it needs an offset = shadowXoffset to be drawn at the same place as normal text
        // But for some reason we need to slightly modify this offset
        // for a better look for KiCad font (better alignment of shadow shape)
        const float adjust = 1.2f;      // Value chosen after tests
        shadowXoffset *= adjust;
    }

    if( aView->IsMirroredX() )
    {
        labelOffset = -labelOffset;
        shadowXoffset = -shadowXoffset;
    }

    TEXT_ATTRIBUTES labelAttrs;
    labelAttrs.m_Size = labelDims.GlyphSize;
    labelAttrs.m_StrokeWidth = labelDims.StrokeWidth;
    labelAttrs.m_Mirrored = aView->IsMirroredX();   // Prevent text mirrored when view is mirrored

    if( EDA_ANGLE( aLine ) > ANGLE_0 )
    {
        labelAttrs.m_Halign = GR_TEXT_H_ALIGN_LEFT;
        labelAttrs.m_Angle = labelAngle;

        // Adjust the text position of the shadow shape:
        labelOffset.x -= shadowXoffset * labelAttrs.m_Angle.Cos();;
        labelOffset.y += shadowXoffset * labelAttrs.m_Angle.Sin();;
    }
    else
    {
        labelAttrs.m_Halign = GR_TEXT_H_ALIGN_RIGHT;
        labelAttrs.m_Angle = labelAngle + ANGLE_180;

        // Adjust the text position of the shadow shape:
        labelOffset.x += shadowXoffset * labelAttrs.m_Angle.Cos();;
        labelOffset.y -= shadowXoffset * labelAttrs.m_Angle.Sin();;
    }

    BOX2D viewportD = aView->GetViewport();
    BOX2I viewport( VECTOR2I( viewportD.GetPosition() ), VECTOR2I( viewportD.GetSize() ) );

    viewport.Inflate( majorTickLen * 2 );   // Doesn't have to be accurate, just big enough not
                                            // to exclude anything that should be partially drawn

    int isign = aView->IsMirroredX() ? -1 : 1;

    for( int i = 0; i < numTicks; ++i )
    {
        const VECTOR2D tickPos = aOrigin + aLine.Resize( tickSpace * i );

        if( !viewport.Contains( tickPos ) )
            continue;

        double length = aMinorTickLen;
        bool   drawLabel = false;

        if( i % tickFormat.majorStep == 0 )
        {
            drawLabel = true;
            length *= majorTickLengthFactor;
        }
        else if( tickFormat.midStep && i % tickFormat.midStep == 0 )
        {
            drawLabel = true;
            length *= midTickLengthFactor;
        }

        gal->SetLineWidth( labelAttrs.m_StrokeWidth / 2 );
        gal->DrawLine( tickPos, tickPos + tickLine.Resize( length*isign ) );

        if( drawLabel )
        {
            wxString label = DimensionLabel( "", tickSpace * i, aIuScale, aUnits, false );
            font->Draw( gal, label, tickPos + labelOffset, labelAttrs, KIFONT::METRICS::Default() );
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
    KIGFX::GAL*  gal = aView->GetGAL();
    TEXT_DIMS    textDims = GetConstantGlyphHeight( gal, -1 );
    const double backTickSpace = aLine.EuclideanNorm() / aNumDivisions;
    VECTOR2D     backTickVec = aLine;
    int isign                = aView->IsMirroredX() ? -1 : 1;

    RotatePoint( backTickVec, -ANGLE_90 );
    backTickVec = backTickVec.Resize( aTickLen * isign );

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


RULER_ITEM::RULER_ITEM( const TWO_POINT_GEOMETRY_MANAGER& aGeomMgr, const EDA_IU_SCALE& aIuScale,
                        EDA_UNITS userUnits, bool aFlipX, bool aFlipY )
        : EDA_ITEM( NOT_USED ), // Never added to anything - just a preview
          m_geomMgr( aGeomMgr ),
          m_userUnits( userUnits ),
          m_iuScale( aIuScale ),
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


std::vector<int> RULER_ITEM::ViewGetLayers() const
{
    std::vector<int> layers{ LAYER_SELECT_OVERLAY, LAYER_GP_OVERLAY };
    return layers;
}


void RULER_ITEM::ViewDraw( int aLayer, KIGFX::VIEW* aView ) const
{
    KIGFX::GAL*      gal = aView->GetGAL();
    RENDER_SETTINGS* rs = aView->GetPainter()->GetSettings();
    bool             drawingDropShadows = ( aLayer == getShadowLayer( gal ) );

    GAL_SCOPED_ATTRS scopedAttrs( *gal, GAL_SCOPED_ATTRS::ALL_ATTRS );
    gal->SetLayerDepth( gal->GetMinDepth() );

    VECTOR2D origin = m_geomMgr.GetOrigin();
    VECTOR2D end = m_geomMgr.GetEnd();

    gal->SetIsStroke( true );
    gal->SetIsFill( false );
    gal->SetTextMirrored( false );

    if( m_color )
        gal->SetStrokeColor( *m_color );
    else
        gal->SetStrokeColor( rs->GetLayerColor( LAYER_AUX_ITEMS ) );

    if( drawingDropShadows )
        gal->SetStrokeColor( GetShadowColor( gal->GetStrokeColor() ) );

    gal->ResetTextAttributes();
    TEXT_DIMS textDims = GetConstantGlyphHeight( gal );

    // draw the main line from the origin to cursor
    gal->SetLineWidth( getTickLineWidth( textDims, drawingDropShadows ) );
    gal->DrawLine( origin, end );

    VECTOR2D rulerVec( end - origin );

    wxArrayString cursorStrings = GetDimensionStrings();

    // Choose a text quadrant that keeps the measurement text on-screen while avoiding
    // overlapping the ruler geometry.  Start with the preferred direction (away from the
    // origin) and fall back to other quadrants as needed to keep the label visible.
    int      prefX = rulerVec.y < 0.0 ? -1 : 1;
    int      prefY = rulerVec.x < 0.0 ? 1 : -1;

    double      scale = gal->GetWorldScale();

    TEXT_DIMS     dims = GetConstantGlyphHeight( gal );
    KIFONT::FONT* font = KIFONT::FONT::GetFont();
    double        width = 0.0;

    for( const wxString& s : cursorStrings )
    {
        VECTOR2I extents = font->StringBoundaryLimits( s, dims.GlyphSize, dims.StrokeWidth, false, false,
                                                       KIFONT::METRICS::Default() );
        width = std::max( width, (double) extents.x );
    }

    double height = dims.LinePitch * cursorStrings.size();

    // Convert to screen coordinates for visibility checks
    VECTOR2D cursorScreen = gal->ToScreen( end );
    VECTOR2I screenSize = gal->GetScreenPixelSize();
    double   offsetX = 15.0;                   // same as DrawTextNextToCursor()
    double   offsetY = dims.LinePitch * scale; // vertical spacing from cursor

    auto fits =
            [&]( int sx, int sy )
            {
                double left, right, top, bottom;
                double xStart = cursorScreen.x + ( sx < 0 ? offsetX : -offsetX );

                if( sx < 0 )
                {
                    left = xStart;
                    right = left + width * scale;
                }
                else
                {
                    right = xStart;
                    left = right - width * scale;
                }

                if( sy > 0 ) // above cursor
                {
                    bottom = cursorScreen.y - offsetY;
                    top = bottom - height * scale;
                }
                else // below cursor
                {
                    top = cursorScreen.y + offsetY;
                    bottom = top + height * scale;
                }

                return left >= 0 && right <= screenSize.x && top >= 0 && bottom <= screenSize.y;
            };

    std::vector<VECTOR2I> candidates = { { prefX, prefY }, { -prefX, prefY },
                                         { prefX, -prefY }, { -prefX, -prefY } };

    VECTOR2I  chosen = candidates[0];
    double    bestDot = -1.0;

    for( const VECTOR2I& c : candidates )
    {
        double dot = c.x * prefX + c.y * prefY;

        if( dot >= 0 && fits( c.x, c.y ) )
        {
            if( dot > bestDot )
            {
                bestDot = dot;
                chosen = c;
            }
        }
    }

    VECTOR2D quadrant( chosen.x, chosen.y );
    DrawTextNextToCursor( aView, end, quadrant, cursorStrings, drawingDropShadows );

    // basic tick size
    double minorTickLen = 5.0 / gal->GetWorldScale();
    double majorTickLen = minorTickLen * majorTickLengthFactor;

    minorTickLen = std::min( minorTickLen, (double) INT_MAX / 2.0 );
    majorTickLen = std::min( majorTickLen, (double) INT_MAX / 2.0 );

    if( m_showTicks )
    {
        drawTicksAlongLine( aView, origin, rulerVec, minorTickLen, m_iuScale, m_userUnits, drawingDropShadows );
        drawBacksideTicks( aView, origin, rulerVec, majorTickLen, 2, drawingDropShadows );
    }

    if( m_showEndArrowHead )
    {
        const EDA_ANGLE arrowAngle{ 30.0 };
        VECTOR2D        arrowHead = rulerVec;
        RotatePoint( arrowHead, arrowAngle );
        arrowHead = arrowHead.Resize( majorTickLen );

        gal->DrawLine( end, end - arrowHead );

        arrowHead = rulerVec;
        RotatePoint( arrowHead, -arrowAngle );
        arrowHead = arrowHead.Resize( majorTickLen );

        gal->DrawLine( end, end - arrowHead );
    }
    else
    {
        // draw the back of the origin "crosshair"
        gal->DrawLine( origin, origin + rulerVec.Resize( -minorTickLen * midTickLengthFactor ) );
    }
}


wxArrayString RULER_ITEM::GetDimensionStrings() const
{
    const VECTOR2D rulerVec = m_geomMgr.GetEnd() - m_geomMgr.GetOrigin();
    VECTOR2D       temp = rulerVec;

    if( m_flipX )
        temp.x = -temp.x;

    if( m_flipY )
        temp.y = -temp.y;

    wxArrayString cursorStrings;

    cursorStrings.push_back( DimensionLabel( "x", temp.x, m_iuScale, m_userUnits ) );
    cursorStrings.push_back( DimensionLabel( "y", temp.y, m_iuScale, m_userUnits ) );

    cursorStrings.push_back( DimensionLabel( "r", rulerVec.EuclideanNorm(), m_iuScale, m_userUnits ) );

    EDA_ANGLE angle = -EDA_ANGLE( rulerVec );
    cursorStrings.push_back( DimensionLabel( wxString::FromUTF8( "θ" ), angle.AsDegrees(), m_iuScale,
                                             EDA_UNITS::DEGREES ) );
    return cursorStrings;
}
