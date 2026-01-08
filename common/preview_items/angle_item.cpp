/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2024 The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <preview_items/angle_item.h>
#include <tool/edit_points.h>
#include <gal/graphics_abstraction_layer.h>
#include <gal/painter.h>
#include <view/view.h>
#include <geometry/seg.h>
#include <preview_items/preview_utils.h>
#include <font/font.h>
#include <wx/string.h>
#include <algorithm>
#include <map>

using namespace KIGFX::PREVIEW;

ANGLE_ITEM::ANGLE_ITEM( const std::shared_ptr<EDIT_POINTS>& aPoints ) :
        SIMPLE_OVERLAY_ITEM(),
        m_points( aPoints )
{
}

const BOX2I ANGLE_ITEM::ViewBBox() const
{
    std::shared_ptr<EDIT_POINTS> points = m_points.lock();

    if( points )
        return points->ViewBBox();

    return BOX2I();
}

void ANGLE_ITEM::drawPreviewShape( KIGFX::VIEW* aView ) const
{
    std::shared_ptr<EDIT_POINTS> points = m_points.lock();

    if( !points )
        return;

    KIGFX::GAL*             gal = aView->GetGAL();
    KIGFX::RENDER_SETTINGS* settings = aView->GetPainter()->GetSettings();
    KIGFX::COLOR4D          drawColor = settings->GetLayerColor( LAYER_AUX_ITEMS );

    double size       = aView->ToWorld( EDIT_POINT::POINT_SIZE ) / 2.0;
    double borderSize = aView->ToWorld( EDIT_POINT::BORDER_SIZE );
    double radius     = size * 10.0;

    gal->SetStrokeColor( drawColor );
    gal->SetFillColor( drawColor );
    gal->SetIsFill( false );
    gal->SetLineWidth( borderSize * 2.0 );
    gal->SetGlyphSize( VECTOR2I( radius / 2, radius / 2 ) );

    std::vector<const EDIT_POINT*> anglePoints;

    for( unsigned i = 0; i < points->PointsSize(); ++i )
    {
        const EDIT_POINT& point = points->Point( i );

        if( point.IsActive() || point.IsHover() )
        {
            anglePoints.push_back( &point );

            EDIT_POINT* prev = points->Previous( point );
            EDIT_POINT* next = points->Next( point );

            if( prev )
                anglePoints.push_back( prev );

            if( next )
                anglePoints.push_back( next );
        }
    }

    std::sort( anglePoints.begin(), anglePoints.end() );
    anglePoints.erase( std::unique( anglePoints.begin(), anglePoints.end() ), anglePoints.end() );

    // First pass: collect all angles and identify congruent ones
    struct AngleInfo
    {
        const EDIT_POINT* point;
        EDA_ANGLE angle;
        VECTOR2D center;
        VECTOR2D v1, v2;
        EDA_ANGLE start;
        EDA_ANGLE sweep;
        EDA_ANGLE mid;
        bool isRightAngle;
    };

    std::vector<AngleInfo> angles;
    std::map<int, int> angleCount; // Map angle (in tenths of degree) to count

    for( const EDIT_POINT* pt : anglePoints )
    {
        EDIT_POINT* prev = points->Previous( *pt );
        EDIT_POINT* next = points->Next( *pt );

        if( !( prev && next ) )
            continue;

        SEG seg1( pt->GetPosition(), prev->GetPosition() );
        SEG seg2( pt->GetPosition(), next->GetPosition() );

        // Calculate the interior angle (0-180 degrees) instead of the smallest angle
        VECTOR2I c = pt->GetPosition();
        VECTOR2I v1 = prev->GetPosition() - c;
        VECTOR2I v2 = next->GetPosition() - c;
        EDA_ANGLE angle = seg2.Angle( seg1 );

        EDA_ANGLE start = EDA_ANGLE( VECTOR2D( v1 ) );
        EDA_ANGLE sweep = ( EDA_ANGLE( VECTOR2D( v2 ) ) - start ).Normalize180();
        EDA_ANGLE mid = start + sweep / 2.0;
        VECTOR2D center( c );

        AngleInfo info;
        info.point = pt;
        info.angle = angle;
        info.center = center;
        info.v1 = v1;
        info.v2 = v2;
        info.start = start;
        info.sweep = sweep;
        info.mid = mid;
        info.isRightAngle = ( angle.AsTenthsOfADegree() == 900 );

        angles.push_back( info );
        angleCount[angle.AsTenthsOfADegree()]++;
    }

    // Second pass: draw the angles with congruence markings
    for( const AngleInfo& angleInfo : angles )
    {
        bool isCongruent = angleCount[angleInfo.angle.AsTenthsOfADegree()] > 1;

        if( angleInfo.isRightAngle )
        {
            VECTOR2D u1 = VECTOR2D( angleInfo.v1 ).Resize( radius );
            VECTOR2D u2 = VECTOR2D( angleInfo.v2 ).Resize( radius );
            VECTOR2D p1 = angleInfo.center + u1;
            VECTOR2D p2 = angleInfo.center + u2;
            VECTOR2D corner = angleInfo.center + u1 + u2;

            // Draw the primary right angle marker
            gal->DrawLine( p1, corner );
            gal->DrawLine( p2, corner );

            // Draw congruence marking for right angles
            if( isCongruent )
            {
                double innerRadius = radius * 0.6;
                VECTOR2D u1_inner = VECTOR2D( angleInfo.v1 ).Resize( innerRadius );
                VECTOR2D u2_inner = VECTOR2D( angleInfo.v2 ).Resize( innerRadius );
                VECTOR2D p1_inner = angleInfo.center + u1_inner;
                VECTOR2D p2_inner = angleInfo.center + u2_inner;
                VECTOR2D corner_inner = angleInfo.center + u1_inner + u2_inner;

                gal->DrawLine( p1_inner, corner_inner );
                gal->DrawLine( p2_inner, corner_inner );
            }
        }
        else
        {
            // Draw the primary arc
            gal->DrawArc( angleInfo.center, radius, angleInfo.start, angleInfo.sweep );

            // Draw congruence marking for non-right angles
            if( isCongruent )
            {
                double innerRadius = radius * 0.7;
                gal->DrawArc( angleInfo.center, innerRadius, angleInfo.start, angleInfo.sweep );
            }
        }

        VECTOR2D textDir( angleInfo.mid.Cos(), angleInfo.mid.Sin() );
        wxString label = wxString::Format( wxT( "%.1f°" ), angleInfo.angle.AsDegrees() );

        // Calculate actual text dimensions to ensure proper clearance
        KIFONT::FONT* font = KIFONT::FONT::GetFont();
        VECTOR2I textSize = font->StringBoundaryLimits( label, gal->GetGlyphSize(), 0, false, false,
                                                        KIFONT::METRICS::Default() );

        // Calculate offset based on text direction - use width for horizontal, height for vertical
        double absX = std::abs( textDir.x );
        double absY = std::abs( textDir.y );
        double textClearance = ( absX * textSize.x + absY * textSize.y ) / 2.0;
        double textOffset = radius + borderSize + textClearance;
        VECTOR2I textPos = angleInfo.center + VECTOR2I( textDir * textOffset );
        gal->BitmapText( label, textPos, ANGLE_HORIZONTAL );
    }
}
