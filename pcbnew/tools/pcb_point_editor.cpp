/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2021 CERN
 * Copyright (C) 2018-2023 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include <functional>
#include <memory>

using namespace std::placeholders;
#include <advanced_config.h>
#include <kiplatform/ui.h>
#include <view/view_controls.h>
#include <gal/graphics_abstraction_layer.h>
#include <geometry/corner_operations.h>
#include <geometry/geometry_utils.h>
#include <geometry/seg.h>
#include <geometry/vector_utils.h>
#include <confirm.h>
#include <tool/tool_manager.h>
#include <tool/point_editor_behavior.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <tools/pcb_point_editor.h>
#include <tools/pcb_grid_helper.h>
#include <tools/generator_tool.h>
#include <dialogs/dialog_unit_entry.h>
#include <board_commit.h>
#include <pcb_edit_frame.h>
#include <pcb_reference_image.h>
#include <pcb_generator.h>
#include <pcb_dimension.h>
#include <pcb_textbox.h>
#include <pcb_tablecell.h>
#include <pcb_table.h>
#include <pad.h>
#include <zone.h>
#include <footprint.h>
#include <footprint_editor_settings.h>
#include <connectivity/connectivity_data.h>
#include <progress_reporter.h>

const unsigned int PCB_POINT_EDITOR::COORDS_PADDING = pcbIUScale.mmToIU( 20 );

// Few constants to avoid using bare numbers for point indices
enum RECT_POINTS
{
    RECT_TOP_LEFT,
    RECT_TOP_RIGHT,
    RECT_BOT_RIGHT,
    RECT_BOT_LEFT,
    RECT_CENTER,

    RECT_MAX_POINTS, // Must be last
};


enum RECT_LINES
{
    RECT_TOP, RECT_RIGHT, RECT_BOT, RECT_LEFT
};


enum DIMENSION_POINTS
{
    DIM_START,
    DIM_END,
    DIM_TEXT,
    DIM_CROSSBARSTART,
    DIM_CROSSBAREND,
    DIM_KNEE = DIM_CROSSBARSTART,

    DIM_ALIGNED_MAX = DIM_CROSSBAREND + 1,
    DIM_CENTER_MAX = DIM_END + 1,
    DIM_RADIAL_MAX = DIM_KNEE + 1,
    DIM_LEADER_MAX = DIM_TEXT + 1,
};


///< Text boxes have different point counts depending on their orientation.
enum TEXTBOX_POINT_COUNT
{
    WHEN_RECTANGLE = RECT_MAX_POINTS,
    WHEN_POLYGON = 0,
};


class RECTANGLE_POINT_EDIT_BEHAVIOR : public POINT_EDIT_BEHAVIOR
{
public:
    RECTANGLE_POINT_EDIT_BEHAVIOR( PCB_SHAPE& aRectangle ) : m_rectangle( aRectangle )
    {
        wxASSERT( m_rectangle.GetShape() == SHAPE_T::RECTANGLE );
    }

    /**
     * Standard rectangle points construction utility
     * (other shapes may use this as well)
     */
    static void MakePoints( const PCB_SHAPE& aRectangle, EDIT_POINTS& aPoints )
    {
        wxCHECK( aRectangle.GetShape() == SHAPE_T::RECTANGLE, /* void */ );

        VECTOR2I topLeft = aRectangle.GetTopLeft();
        VECTOR2I botRight = aRectangle.GetBotRight();

        aPoints.SetSwapX( topLeft.x > botRight.x );
        aPoints.SetSwapY( topLeft.y > botRight.y );

        if( aPoints.SwapX() )
            std::swap( topLeft.x, botRight.x );

        if( aPoints.SwapY() )
            std::swap( topLeft.y, botRight.y );

        aPoints.AddPoint( topLeft );
        aPoints.AddPoint( VECTOR2I( botRight.x, topLeft.y ) );
        aPoints.AddPoint( botRight );
        aPoints.AddPoint( VECTOR2I( topLeft.x, botRight.y ) );
        aPoints.AddPoint( aRectangle.GetCenter() );

        aPoints.AddLine( aPoints.Point( RECT_TOP_LEFT ), aPoints.Point( RECT_TOP_RIGHT ) );
        aPoints.Line( RECT_TOP ).SetConstraint( new EC_PERPLINE( aPoints.Line( RECT_TOP ) ) );
        aPoints.AddLine( aPoints.Point( RECT_TOP_RIGHT ), aPoints.Point( RECT_BOT_RIGHT ) );
        aPoints.Line( RECT_RIGHT ).SetConstraint( new EC_PERPLINE( aPoints.Line( RECT_RIGHT ) ) );
        aPoints.AddLine( aPoints.Point( RECT_BOT_RIGHT ), aPoints.Point( RECT_BOT_LEFT ) );
        aPoints.Line( RECT_BOT ).SetConstraint( new EC_PERPLINE( aPoints.Line( RECT_BOT ) ) );
        aPoints.AddLine( aPoints.Point( RECT_BOT_LEFT ), aPoints.Point( RECT_TOP_LEFT ) );
        aPoints.Line( RECT_LEFT ).SetConstraint( new EC_PERPLINE( aPoints.Line( RECT_LEFT ) ) );
    }

    static void UpdateItem( PCB_SHAPE& aRectangle, const EDIT_POINT& aEditedPoint,
                            EDIT_POINTS& aPoints )
    {
        // You can have more points if your item wants to have more points
        // (this class assumes the rect points come first, but that can be changed)
        CHECK_POINT_COUNT_GE( aPoints, RECT_MAX_POINTS );

        auto setLeft =
                [&]( int left )
                {
                    aPoints.SwapX() ? aRectangle.SetRight( left ) : aRectangle.SetLeft( left );
                };
        auto setRight =
                [&]( int right )
                {
                    aPoints.SwapX() ? aRectangle.SetLeft( right ) : aRectangle.SetRight( right );
                };
        auto setTop =
                [&]( int top )
                {
                    aPoints.SwapY() ? aRectangle.SetBottom( top ) : aRectangle.SetTop( top );
                };
        auto setBottom =
                [&]( int bottom )
                {
                    aPoints.SwapY() ? aRectangle.SetTop( bottom ) : aRectangle.SetBottom( bottom );
                };

        VECTOR2I topLeft = aPoints.Point( RECT_TOP_LEFT ).GetPosition();
        VECTOR2I topRight = aPoints.Point( RECT_TOP_RIGHT ).GetPosition();
        VECTOR2I botLeft = aPoints.Point( RECT_BOT_LEFT ).GetPosition();
        VECTOR2I botRight = aPoints.Point( RECT_BOT_RIGHT ).GetPosition();

        PinEditedCorner( aEditedPoint, aPoints, topLeft, topRight, botLeft, botRight );

        if( isModified( aEditedPoint, aPoints.Point( RECT_TOP_LEFT ) )
            || isModified( aEditedPoint, aPoints.Point( RECT_TOP_RIGHT ) )
            || isModified( aEditedPoint, aPoints.Point( RECT_BOT_RIGHT ) )
            || isModified( aEditedPoint, aPoints.Point( RECT_BOT_LEFT ) ) )
        {
            setTop( topLeft.y );
            setLeft( topLeft.x );
            setRight( botRight.x );
            setBottom( botRight.y );
        }
        else if( isModified( aEditedPoint, aPoints.Point( RECT_CENTER ) ) )
        {
            const VECTOR2I moveVector =
                    aPoints.Point( RECT_CENTER ).GetPosition() - aRectangle.GetCenter();
            aRectangle.Move( moveVector );
        }
        else if( isModified( aEditedPoint, aPoints.Line( RECT_TOP ) ) )
        {
            setTop( topLeft.y );
        }
        else if( isModified( aEditedPoint, aPoints.Line( RECT_LEFT ) ) )
        {
            setLeft( topLeft.x );
        }
        else if( isModified( aEditedPoint, aPoints.Line( RECT_BOT ) ) )
        {
            setBottom( botRight.y );
        }
        else if( isModified( aEditedPoint, aPoints.Line( RECT_RIGHT ) ) )
        {
            setRight( botRight.x );
        }

        for( unsigned i = 0; i < aPoints.LinesSize(); ++i )
        {
            if( !isModified( aEditedPoint, aPoints.Line( i ) ) )
            {
                aPoints.Line( i ).SetConstraint( new EC_PERPLINE( aPoints.Line( i ) ) );
            }
        }
    }

    static void UpdatePoints( PCB_SHAPE& aRectangle, EDIT_POINTS& aPoints )
    {
        wxCHECK( aPoints.PointsSize() >= RECT_MAX_POINTS, /* void */ );

        VECTOR2I topLeft = aRectangle.GetTopLeft();
        VECTOR2I botRight = aRectangle.GetBotRight();

        aPoints.SetSwapX( topLeft.x > botRight.x );
        aPoints.SetSwapY( topLeft.y > botRight.y );

        if( aPoints.SwapX() )
            std::swap( topLeft.x, botRight.x );

        if( aPoints.SwapY() )
            std::swap( topLeft.y, botRight.y );

        aPoints.Point( RECT_TOP_LEFT ).SetPosition( topLeft );
        aPoints.Point( RECT_TOP_RIGHT ).SetPosition( botRight.x, topLeft.y );
        aPoints.Point( RECT_BOT_RIGHT ).SetPosition( botRight );
        aPoints.Point( RECT_BOT_LEFT ).SetPosition( topLeft.x, botRight.y );
        aPoints.Point( RECT_CENTER ).SetPosition( aRectangle.GetCenter() );
    }

    void MakePoints( EDIT_POINTS& aPoints ) override
    {
        // Just call the static helper
        MakePoints( m_rectangle, aPoints );
    }

    void UpdatePoints( EDIT_POINTS& aPoints ) override
    {
        UpdatePoints( m_rectangle, aPoints );
    }

    void UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                     std::vector<EDA_ITEM*>& aUpdatedItems ) override
    {
        UpdateItem( m_rectangle, aEditedPoint, aPoints );
    }

    /**
     * Update the coordinates of 4 corners of a rectangle, according to pad constraints and the
     * moved corner
     *
     * @param aPoints the points list
     *
     * @param aTopLeft [in/out] is the RECT_TOPLEFT to constraint
     * @param aTopRight [in/out] is the RECT_TOPRIGHT to constraint
     * @param aBotLeft [in/out] is the RECT_BOTLEFT to constraint
     * @param aBotRight [in/out] is the RECT_BOTRIGHT to constraint
     * @param aHole the location of the pad's hole
     * @param aHoleSize the pad's hole size (or {0,0} if it has no hole)
     */
    static void PinEditedCorner( const EDIT_POINT& aEditedPoint, const EDIT_POINTS& aEditPoints,
                                VECTOR2I& aTopLeft, VECTOR2I& aTopRight, VECTOR2I& aBotLeft,
                                VECTOR2I& aBotRight, const VECTOR2I& aHole = { 0, 0 },
                                const VECTOR2I& aHoleSize = { 0, 0 } )
    {
        int minWidth = EDA_UNIT_UTILS::Mils2IU( pcbIUScale, 1 );
        int minHeight = EDA_UNIT_UTILS::Mils2IU( pcbIUScale, 1 );

        if( isModified( aEditedPoint, aEditPoints.Point( RECT_TOP_LEFT ) ) )
        {
            if( aHoleSize.x )
            {
                // pin edited point to the top/left of the hole
                aTopLeft.x = std::min( aTopLeft.x, aHole.x - aHoleSize.x / 2 - minWidth );
                aTopLeft.y = std::min( aTopLeft.y, aHole.y - aHoleSize.y / 2 - minHeight );
            }
            else
            {
                // pin edited point within opposite corner
                aTopLeft.x = std::min( aTopLeft.x, aBotRight.x - minWidth );
                aTopLeft.y = std::min( aTopLeft.y, aBotRight.y - minHeight );
            }

            // push edited point edges to adjacent corners
            aTopRight.y = aTopLeft.y;
            aBotLeft.x = aTopLeft.x;
        }
        else if( isModified( aEditedPoint, aEditPoints.Point( RECT_TOP_RIGHT ) ) )
        {
            if( aHoleSize.x )
            {
                // pin edited point to the top/right of the hole
                aTopRight.x = std::max( aTopRight.x, aHole.x + aHoleSize.x / 2 + minWidth );
                aTopRight.y = std::min( aTopRight.y, aHole.y - aHoleSize.y / 2 - minHeight );
            }
            else
            {
                // pin edited point within opposite corner
                aTopRight.x = std::max( aTopRight.x, aBotLeft.x + minWidth );
                aTopRight.y = std::min( aTopRight.y, aBotLeft.y - minHeight );
            }

            // push edited point edges to adjacent corners
            aTopLeft.y = aTopRight.y;
            aBotRight.x = aTopRight.x;
        }
        else if( isModified( aEditedPoint, aEditPoints.Point( RECT_BOT_LEFT ) ) )
        {
            if( aHoleSize.x )
            {
                // pin edited point to the bottom/left of the hole
                aBotLeft.x = std::min( aBotLeft.x, aHole.x - aHoleSize.x / 2 - minWidth );
                aBotLeft.y = std::max( aBotLeft.y, aHole.y + aHoleSize.y / 2 + minHeight );
            }
            else
            {
                // pin edited point within opposite corner
                aBotLeft.x = std::min( aBotLeft.x, aTopRight.x - minWidth );
                aBotLeft.y = std::max( aBotLeft.y, aTopRight.y + minHeight );
            }

            // push edited point edges to adjacent corners
            aBotRight.y = aBotLeft.y;
            aTopLeft.x = aBotLeft.x;
        }
        else if( isModified( aEditedPoint, aEditPoints.Point( RECT_BOT_RIGHT ) ) )
        {
            if( aHoleSize.x )
            {
                // pin edited point to the bottom/right of the hole
                aBotRight.x = std::max( aBotRight.x, aHole.x + aHoleSize.x / 2 + minWidth );
                aBotRight.y = std::max( aBotRight.y, aHole.y + aHoleSize.y / 2 + minHeight );
            }
            else
            {
                // pin edited point within opposite corner
                aBotRight.x = std::max( aBotRight.x, aTopLeft.x + minWidth );
                aBotRight.y = std::max( aBotRight.y, aTopLeft.y + minHeight );
            }

            // push edited point edges to adjacent corners
            aBotLeft.y = aBotRight.y;
            aTopRight.x = aBotRight.x;
        }
        else if( isModified( aEditedPoint, aEditPoints.Line( RECT_TOP ) ) )
        {
            aTopLeft.y = std::min( aTopLeft.y, aBotRight.y - minHeight );
        }
        else if( isModified( aEditedPoint, aEditPoints.Line( RECT_LEFT ) ) )
        {
            aTopLeft.x = std::min( aTopLeft.x, aBotRight.x - minWidth );
        }
        else if( isModified( aEditedPoint, aEditPoints.Line( RECT_BOT ) ) )
        {
            aBotRight.y = std::max( aBotRight.y, aTopLeft.y + minHeight );
        }
        else if( isModified( aEditedPoint, aEditPoints.Line( RECT_RIGHT ) ) )
        {
            aBotRight.x = std::max( aBotRight.x, aTopLeft.x + minWidth );
        }
    }

private:
    PCB_SHAPE& m_rectangle;
};


class ARC_POINT_EDIT_BEHAVIOR : public POINT_EDIT_BEHAVIOR
{
    enum ARC_POINTS
    {
        ARC_START,
        ARC_MID,
        ARC_END,
        ARC_CENTER,
    };

public:
    ARC_POINT_EDIT_BEHAVIOR( PCB_SHAPE& aArc, const ARC_EDIT_MODE& aArcEditMode,
                             KIGFX::VIEW_CONTROLS& aViewContols ) :
            m_arc( aArc ), m_arcEditMode( aArcEditMode ), m_viewControls( aViewContols )
    {
        wxASSERT( m_arc.GetShape() == SHAPE_T::ARC );
    }

    void MakePoints( EDIT_POINTS& aPoints ) override
    {
        aPoints.AddPoint( m_arc.GetStart() );
        aPoints.AddPoint( m_arc.GetArcMid() );
        aPoints.AddPoint( m_arc.GetEnd() );
        aPoints.AddPoint( m_arc.GetCenter() );

        aPoints.AddIndicatorLine( aPoints.Point( ARC_CENTER ), aPoints.Point( ARC_START ) );
        aPoints.AddIndicatorLine( aPoints.Point( ARC_CENTER ), aPoints.Point( ARC_END ) );
    }

    void UpdatePoints( EDIT_POINTS& aPoints ) override
    {
        CHECK_POINT_COUNT( aPoints, 4 );

        aPoints.Point( ARC_START ).SetPosition( m_arc.GetStart() );
        aPoints.Point( ARC_MID ).SetPosition( m_arc.GetArcMid() );
        aPoints.Point( ARC_END ).SetPosition( m_arc.GetEnd() );
        aPoints.Point( ARC_CENTER ).SetPosition( m_arc.GetCenter() );
    }

    void UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                     std::vector<EDA_ITEM*>& aUpdatedItems ) override
    {
        CHECK_POINT_COUNT( aPoints, 4 );

        VECTOR2I center = aPoints.Point( ARC_CENTER ).GetPosition();
        VECTOR2I mid = aPoints.Point( ARC_MID ).GetPosition();
        VECTOR2I start = aPoints.Point( ARC_START ).GetPosition();
        VECTOR2I end = aPoints.Point( ARC_END ).GetPosition();

        if( isModified( aEditedPoint, aPoints.Point( ARC_CENTER ) ) )
        {
            if( m_arcEditMode == ARC_EDIT_MODE::KEEP_ENDPOINTS_OR_START_DIRECTION )
            {
                editArcCenterKeepEndpoints( m_arc, center, start, mid, end );
            }
            else
            {
                VECTOR2I moveVector = VECTOR2I( center.x, center.y ) - m_arc.GetCenter();
                m_arc.Move( moveVector );
            }
        }
        else if( isModified( aEditedPoint, aPoints.Point( ARC_MID ) ) )
        {
            const VECTOR2I& cursorPos = m_viewControls.GetCursorPosition( false );

            if( m_arcEditMode == ARC_EDIT_MODE::KEEP_ENDPOINTS_OR_START_DIRECTION )
                editArcMidKeepEndpoints( m_arc, start, end, cursorPos );
            else
                editArcMidKeepCenter( m_arc, center, start, mid, end, cursorPos );
        }
        else if( isModified( aEditedPoint, aPoints.Point( ARC_START ) )
                 || isModified( aEditedPoint, aPoints.Point( ARC_END ) ) )
        {
            const VECTOR2I& cursorPos = m_viewControls.GetCursorPosition();

            if( m_arcEditMode == ARC_EDIT_MODE::KEEP_ENDPOINTS_OR_START_DIRECTION )
                editArcEndpointKeepTangent( m_arc, center, start, mid, end, cursorPos );
            else
                editArcEndpointKeepCenter( m_arc, center, start, mid, end, cursorPos );
        }
    }

    OPT_VECTOR2I Get45DegreeConstrainer( const EDIT_POINT& aEditedPoint,
                                         EDIT_POINTS&      aPoints ) const override
    {
        return aPoints.Point( ARC_CENTER ).GetPosition();
    }

private:
    // Note: these static arc functions don't have to be in here - we could ship them out
    // to a utils area for use by other behaviours (e.g. in eeschema, or for polygons with
    // radiused corners). For now this is the smaller conceptual delta with the old code.

    /**
     * Move an end point of the arc, while keeping the tangent at the other endpoint.
     */
    static void editArcEndpointKeepTangent( PCB_SHAPE& aArc, const VECTOR2I& aCenter,
                                            const VECTOR2I& aStart, const VECTOR2I& aMid,
                                            const VECTOR2I& aEnd, const VECTOR2I& aCursor )
    {
        VECTOR2I center = aCenter;
        bool     movingStart;
        bool     arcValid = true;

        VECTOR2I p1, p2, p3;
        // p1 does not move, p2 does.

        if( aStart != aArc.GetStart() )
        {
            p1          = aEnd;
            p2          = aStart;
            p3          = aMid;
            movingStart = true;
        }
        else if( aEnd != aArc.GetEnd() )
        {
            p1          = aStart;
            p2          = aEnd;
            p3          = aMid;
            movingStart = false;
        }
        else
        {
            return;
        }

        VECTOR2D v1, v2, v3, v4;

        // Move the coordinate system
        v1 = p1 - aCenter;
        v2 = p2 - aCenter;
        v3 = p3 - aCenter;

        VECTOR2D u1, u2;

        // A point cannot be both the center and on the arc.
        if( ( v1.EuclideanNorm() == 0 ) || ( v2.EuclideanNorm() == 0 ) )
            return;

        u1 = v1 / v1.EuclideanNorm();
        u2 = v3 - ( u1.x * v3.x + u1.y * v3.y ) * u1;
        u2 = u2 / u2.EuclideanNorm();

        // [ u1, u3 ] is a base centered on the circle with:
        //  u1 : unit vector toward the point that does not move
        //  u2 : unit vector toward the mid point.

        // Get vectors v1, and v2 in that coordinate system.

        double det  = u1.x * u2.y - u2.x * u1.y;

        // u1 and u2 are unit vectors, and perpendicular.
        // det should not be 0. In case it is, do not change the arc.
        if( det == 0 )
            return;

        double tmpx = v1.x * u2.y - v1.y * u2.x;
        double tmpy = -v1.x * u1.y + v1.y * u1.x;
        v1.x        = tmpx;
        v1.y        = tmpy;
        v1          = v1 / det;

        tmpx = v2.x * u2.y - v2.y * u2.x;
        tmpy = -v2.x * u1.y + v2.y * u1.x;
        v2.x = tmpx;
        v2.y = tmpy;
        v2   = v2 / det;

        double R               = v1.EuclideanNorm();
        bool   transformCircle = false;

        /*                 p2
        *                     X***
        *                         **  <---- This is the arc
        *            y ^            **
        *              |      R       *
        *              | <-----------> *
        *       x------x------>--------x p1
        *     C' <----> C      x
        *         delta
        *
        * p1 does not move, and the tangent at p1 remains the same.
        *  => The new center, C', will be on the C-p1 axis.
        * p2 moves
        *
        * The radius of the new circle is delta + R
        *
        * || C' p2 || = || C' P1 ||
        * is the same as :
        * ( delta + p2.x ) ^ 2 + p2.y ^ 2 = ( R + delta ) ^ 2
        *
        * delta = ( R^2  - p2.x ^ 2 - p2.y ^2 ) / ( 2 * p2.x - 2 * R )
        *
        * We can use this equation for any point p2 with p2.x < R
        */

        if( v2.x == R )
        {
            // Straight line, do nothing
        }
        else
        {
            if( v2.x > R )
            {
                // If we need to invert the curvature.
                // We modify the input so we can use the same equation
                transformCircle = true;
                v2.x            = 2 * R - v2.x;
            }

            // We can keep the tangent constraint.
            double delta = ( R * R - v2.x * v2.x - v2.y * v2.y ) / ( 2 * v2.x - 2 * R );

            // This is just to limit the radius, so nothing overflows later when drawing.
            if( abs( v2.y / ( R - v2.x ) ) > ADVANCED_CFG::GetCfg().m_DrawArcCenterMaxAngle )
                arcValid = false;

            // Never recorded a problem, but still checking.
            if( !std::isfinite( delta ) )
                arcValid = false;

            // v4 is the new center
            v4 = ( !transformCircle ) ? VECTOR2D( -delta, 0 ) : VECTOR2D( 2 * R + delta, 0 );

            tmpx = v4.x * u1.x + v4.y * u2.x;
            tmpy = v4.x * u1.y + v4.y * u2.y;
            v4.x = tmpx;
            v4.y = tmpy;

            center = v4 + aCenter;

            if( arcValid )
            {
                aArc.SetCenter( center );

                if( movingStart )
                    aArc.SetStart( aStart );
                else
                    aArc.SetEnd( aEnd );
            }
        }
    }

    /**
     * Move the arc center but keep endpoint locations.
     */
    static void editArcCenterKeepEndpoints( PCB_SHAPE& aArc, const VECTOR2I& aCenter,
                                            const VECTOR2I& aStart, const VECTOR2I& aMid,
                                            const VECTOR2I& aEnd )
    {
        const int c_snapEpsilon_sq = 4;

        VECTOR2I m = ( aStart / 2 + aEnd / 2 );
        VECTOR2I perp = ( aEnd - aStart ).Perpendicular().Resize( INT_MAX / 2 );

        SEG legal( m - perp, m + perp );

        const SEG testSegments[] = { SEG( aCenter, aCenter + VECTOR2( 1, 0 ) ),
                                     SEG( aCenter, aCenter + VECTOR2( 0, 1 ) ) };

        std::vector<VECTOR2I> points = { legal.A, legal.B };

        for( const SEG& seg : testSegments )
        {
            OPT_VECTOR2I vec = legal.IntersectLines( seg );

            if( vec && legal.SquaredDistance( *vec ) <= c_snapEpsilon_sq )
                points.push_back( *vec );
        }

        OPT_VECTOR2I nearest;
        SEG::ecoord  min_d_sq = VECTOR2I::ECOORD_MAX;

        // Snap by distance between cursor and intersections
        for( const VECTOR2I& pt : points )
        {
            SEG::ecoord d_sq = ( pt - aCenter ).SquaredEuclideanNorm();

            if( d_sq < min_d_sq - c_snapEpsilon_sq )
            {
                min_d_sq = d_sq;
                nearest = pt;
            }
        }

        if( nearest )
            aArc.SetCenter( *nearest );
    }

    /**
     * Move an end point of the arc around the circumference.
     */
    static void editArcEndpointKeepCenter( PCB_SHAPE& aArc, const VECTOR2I& aCenter,
                                           const VECTOR2I& aStart, const VECTOR2I& aMid,
                                           const VECTOR2I& aEnd, const VECTOR2I& aCursor )
    {
        int  minRadius = EDA_UNIT_UTILS::Mils2IU( pcbIUScale, 1 );
        bool movingStart;

        VECTOR2I p1, p2, prev_p1;

        // user is moving p1, we want to move p2 to the new radius.

        if( aStart != aArc.GetStart() )
        {
            prev_p1     = aArc.GetStart();
            p1          = aStart;
            p2          = aEnd;
            movingStart = true;
        }
        else
        {
            prev_p1     = aArc.GetEnd();
            p1          = aEnd;
            p2          = aStart;
            movingStart = false;
        }

        p1 = p1 - aCenter;
        p2 = p2 - aCenter;

        if( p1.x == 0 && p1.y == 0 )
            p1 = prev_p1 - aCenter;

        if( p2.x == 0 && p2.y == 0 )
            p2 = { 1, 0 };

        double radius = p1.EuclideanNorm();

        if( radius < minRadius )
            radius = minRadius;

        p1 = aCenter + p1.Resize( radius );
        p2 = aCenter + p2.Resize( radius );

        aArc.SetCenter( aCenter );

        if( movingStart )
        {
            aArc.SetStart( p1 );
            aArc.SetEnd( p2 );
        }
        else
        {
            aArc.SetStart( p2 );
            aArc.SetEnd( p1 );
        }
    }

    /**
     * Move the mid point of the arc, while keeping the two endpoints.
     */
    void editArcMidKeepCenter( PCB_SHAPE& aArc, const VECTOR2I& aCenter, const VECTOR2I& aStart,
                               const VECTOR2I& aMid, const VECTOR2I& aEnd, const VECTOR2I& aCursor )
    {
        int minRadius = EDA_UNIT_UTILS::Mils2IU( pcbIUScale, 1 );

        SEG chord( aStart, aEnd );

        // Now, update the edit point position
        // Express the point in a circle-centered coordinate system.
        VECTOR2I start = aStart - aCenter;
        VECTOR2I end = aEnd - aCenter;

        double radius = ( aCursor - aCenter ).EuclideanNorm();

        if( radius < minRadius )
            radius = minRadius;

        start = start.Resize( radius );
        end = end.Resize( radius );

        start = start + aCenter;
        end = end + aCenter;

        aArc.SetStart( start );
        aArc.SetEnd( end );
    }

    /**
     * Move the mid point of the arc, while keeping the angle.
     */
    static void editArcMidKeepEndpoints( PCB_SHAPE& aArc, const VECTOR2I& aStart,
                                         const VECTOR2I& aEnd, const VECTOR2I& aCursor )
    {
        // Let 'm' be the middle point of the chord between the start and end points
        VECTOR2I m = ( aStart + aEnd ) / 2;

        // Legal midpoints lie on a vector starting just off the chord midpoint and extending out
        // past the existing midpoint.  We do not allow arc inflection while point editing.
        const int JUST_OFF = ( aStart - aEnd ).EuclideanNorm() / 100;
        VECTOR2I  v = (VECTOR2I) aArc.GetArcMid() - m;
        SEG       legal( m + v.Resize( JUST_OFF ), m + v.Resize( INT_MAX / 2 ) );
        VECTOR2I  mid = legal.NearestPoint( aCursor );

        aArc.SetArcGeometry( aStart, mid, aEnd );
    }

    PCB_SHAPE& m_arc;
    // The arc edit mode, which is injected from the editor
    const ARC_EDIT_MODE&  m_arcEditMode;
    KIGFX::VIEW_CONTROLS& m_viewControls;
};


class ZONE_POINT_EDIT_BEHAVIOR : public POLYGON_POINT_EDIT_BEHAVIOR
{
public:
    ZONE_POINT_EDIT_BEHAVIOR( ZONE& aZone ) :
            POLYGON_POINT_EDIT_BEHAVIOR( *aZone.Outline() ), m_zone( aZone )
    {
    }

    void UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                     std::vector<EDA_ITEM*>& aUpdatedItems ) override
    {
        m_zone.UnFill();

        // Defer to the base class to update the polygon
        POLYGON_POINT_EDIT_BEHAVIOR::UpdateItem( aEditedPoint, aPoints, aCommit, aUpdatedItems );

        m_zone.HatchBorder();
    }

private:
    ZONE& m_zone;
};


class REFERENCE_IMAGE_POINT_EDIT_BEHAVIOR : public POINT_EDIT_BEHAVIOR
{
    enum REFIMG_POINTS
    {
        REFIMG_ORIGIN = RECT_CENTER, // Reuse the center point fo rthe transform origin

        REFIMG_MAX_POINTS,
    };

public:
    REFERENCE_IMAGE_POINT_EDIT_BEHAVIOR( PCB_REFERENCE_IMAGE& aRefImage ) : m_refImage( aRefImage )
    {
    }

    void MakePoints( EDIT_POINTS& aPoints ) override
    {
        REFERENCE_IMAGE& refImage = m_refImage.GetReferenceImage();

        const VECTOR2I topLeft = refImage.GetPosition() - refImage.GetSize() / 2;
        const VECTOR2I botRight = refImage.GetPosition() + refImage.GetSize() / 2;

        aPoints.AddPoint( topLeft );
        aPoints.AddPoint( VECTOR2I( botRight.x, topLeft.y ) );
        aPoints.AddPoint( botRight );
        aPoints.AddPoint( VECTOR2I( topLeft.x, botRight.y ) );

        aPoints.AddPoint( refImage.GetPosition() + refImage.GetTransformOriginOffset() );
    }

    void UpdatePoints( EDIT_POINTS& aPoints ) override
    {
        CHECK_POINT_COUNT( aPoints, REFIMG_MAX_POINTS );

        REFERENCE_IMAGE& refImage = m_refImage.GetReferenceImage();

        const VECTOR2I topLeft = refImage.GetPosition() - refImage.GetSize() / 2;
        const VECTOR2I botRight = refImage.GetPosition() + refImage.GetSize() / 2;

        aPoints.Point( RECT_TOP_LEFT ).SetPosition( topLeft );
        aPoints.Point( RECT_TOP_RIGHT ).SetPosition( botRight.x, topLeft.y );
        aPoints.Point( RECT_BOT_RIGHT ).SetPosition( botRight );
        aPoints.Point( RECT_BOT_LEFT ).SetPosition( topLeft.x, botRight.y );

        aPoints.Point( REFIMG_ORIGIN )
                .SetPosition( refImage.GetPosition() + refImage.GetTransformOriginOffset() );
    }

    void UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                     std::vector<EDA_ITEM*>& aUpdatedItems ) override
    {
        CHECK_POINT_COUNT( aPoints, REFIMG_MAX_POINTS );

        REFERENCE_IMAGE& refImage = m_refImage.GetReferenceImage();

        const VECTOR2I topLeft = aPoints.Point( RECT_TOP_LEFT ).GetPosition();
        const VECTOR2I topRight = aPoints.Point( RECT_TOP_RIGHT ).GetPosition();
        const VECTOR2I botRight = aPoints.Point( RECT_BOT_RIGHT ).GetPosition();
        const VECTOR2I botLeft = aPoints.Point( RECT_BOT_LEFT ).GetPosition();
        const VECTOR2I xfrmOrigin = aPoints.Point( REFIMG_ORIGIN ).GetPosition();

        if( isModified( aEditedPoint, aPoints.Point( REFIMG_ORIGIN ) ) )
        {
            // Moving the transform origin
            // As the other points didn't move, we can get the image extent from them
            const VECTOR2I newOffset = xfrmOrigin - ( topLeft + botRight ) / 2;
            refImage.SetTransformOriginOffset( newOffset );
        }
        else
        {
            const VECTOR2I oldOrigin =
                    m_refImage.GetPosition() + refImage.GetTransformOriginOffset();
            const VECTOR2I oldSize = refImage.GetSize();
            const VECTOR2I pos = refImage.GetPosition();

            OPT_VECTOR2I newCorner;
            VECTOR2I     oldCorner = pos;
            if( isModified( aEditedPoint, aPoints.Point( RECT_TOP_LEFT ) ) )
            {
                newCorner = topLeft;
                oldCorner -= oldSize / 2;
            }
            else if( isModified( aEditedPoint, aPoints.Point( RECT_TOP_RIGHT ) ) )
            {
                newCorner = topRight;
                oldCorner -= VECTOR2I( -oldSize.x, oldSize.y ) / 2;
            }
            else if( isModified( aEditedPoint, aPoints.Point( RECT_BOT_LEFT ) ) )
            {
                newCorner = botLeft;
                oldCorner -= VECTOR2I( oldSize.x, -oldSize.y ) / 2;
            }
            else if( isModified( aEditedPoint, aPoints.Point( RECT_BOT_RIGHT ) ) )
            {
                newCorner = botRight;
                oldCorner += oldSize / 2;
            }

            if( newCorner )
            {
                // Turn in the respective vectors from the origin
                *newCorner -= xfrmOrigin;
                oldCorner -= oldOrigin;

                // If we tried to cross the origin, clamp it to stop it
                if( sign( newCorner->x ) != sign( oldCorner.x )
                    || sign( newCorner->y ) != sign( oldCorner.y ) )
                {
                    *newCorner = VECTOR2I( 0, 0 );
                }

                const double newLength = newCorner->EuclideanNorm();
                const double oldLength = oldCorner.EuclideanNorm();

                double ratio = oldLength > 0 ? ( newLength / oldLength ) : 1.0;

                // Clamp the scaling to a minimum of 50 mils
                VECTOR2I newSize = oldSize * ratio;
                double newWidth = std::max( newSize.x, EDA_UNIT_UTILS::Mils2IU( pcbIUScale, 50 ) );
                double newHeight = std::max( newSize.y, EDA_UNIT_UTILS::Mils2IU( pcbIUScale, 50 ) );
                ratio = std::min( newWidth / oldSize.x, newHeight / oldSize.y );

                // Also handles the origin offset
                refImage.SetImageScale( refImage.GetImageScale() * ratio );
            }
        }
    }

private:
    PCB_REFERENCE_IMAGE& m_refImage;
};


class PCB_TABLECELL_POINT_EDIT_BEHAVIOR : public EDA_TABLECELL_POINT_EDIT_BEHAVIOR
{
public:
    PCB_TABLECELL_POINT_EDIT_BEHAVIOR( PCB_TABLECELL& aCell ) :
            EDA_TABLECELL_POINT_EDIT_BEHAVIOR( aCell ), m_cell( aCell )
    {
    }

    void UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                     std::vector<EDA_ITEM*>& aUpdatedItems ) override
    {
        CHECK_POINT_COUNT( aPoints, TABLECELL_MAX_POINTS );

        PCB_TABLE& table = static_cast<PCB_TABLE&>( *m_cell.GetParent() );
        aCommit.Modify( &table );
        aUpdatedItems.push_back( &table );

        if( isModified( aEditedPoint, aPoints.Point( COL_WIDTH ) ) )
        {
            m_cell.SetEnd( VECTOR2I( aPoints.Point( 0 ).GetX(), m_cell.GetEndY() ) );

            int colWidth = m_cell.GetRectangleWidth();

            for( int ii = 0; ii < m_cell.GetColSpan() - 1; ++ii )
                colWidth -= table.GetColWidth( m_cell.GetColumn() + ii );

            table.SetColWidth( m_cell.GetColumn() + m_cell.GetColSpan() - 1, colWidth );
        }
        else if( isModified( aEditedPoint, aPoints.Point( ROW_HEIGHT ) ) )
        {
            m_cell.SetEnd( VECTOR2I( m_cell.GetEndX(), aPoints.Point( 1 ).GetY() ) );

            int rowHeight = m_cell.GetRectangleHeight();

            for( int ii = 0; ii < m_cell.GetRowSpan() - 1; ++ii )
                rowHeight -= table.GetRowHeight( m_cell.GetRow() + ii );

            table.SetRowHeight( m_cell.GetRow() + m_cell.GetRowSpan() - 1, rowHeight );
        }

        table.Normalize();
    }

private:
    PCB_TABLECELL& m_cell;
};


class PAD_POINT_EDIT_BEHAVIOR : public POINT_EDIT_BEHAVIOR
{
public:
    PAD_POINT_EDIT_BEHAVIOR( PAD& aPad ) : m_pad( aPad ) {}

    void MakePoints( EDIT_POINTS& aPoints ) override
    {
        // TODO(JE) padstacks
        VECTOR2I shapePos = m_pad.ShapePos( PADSTACK::ALL_LAYERS );
        VECTOR2I halfSize( m_pad.GetSize( PADSTACK::ALL_LAYERS ).x / 2,
                           m_pad.GetSize( PADSTACK::ALL_LAYERS ).y / 2 );

        if( m_pad.IsLocked() )
            return;

        switch( m_pad.GetShape( PADSTACK::ALL_LAYERS ) )
        {
        case PAD_SHAPE::CIRCLE:
            aPoints.AddPoint( VECTOR2I( shapePos.x + halfSize.x, shapePos.y ) );
            break;

        case PAD_SHAPE::OVAL:
        case PAD_SHAPE::TRAPEZOID:
        case PAD_SHAPE::RECTANGLE:
        case PAD_SHAPE::ROUNDRECT:
        case PAD_SHAPE::CHAMFERED_RECT:
        {
            if( !m_pad.GetOrientation().IsCardinal() )
                break;

            if( m_pad.GetOrientation() == ANGLE_90 || m_pad.GetOrientation() == ANGLE_270 )
                std::swap( halfSize.x, halfSize.y );

            // It's important to fill these according to the RECT indices
            aPoints.AddPoint( shapePos - halfSize );
            aPoints.AddPoint( VECTOR2I( shapePos.x + halfSize.x, shapePos.y - halfSize.y ) );
            aPoints.AddPoint( shapePos + halfSize );
            aPoints.AddPoint( VECTOR2I( shapePos.x - halfSize.x, shapePos.y + halfSize.y ) );
        }
        break;

        default: // suppress warnings
            break;
        }
    }

    void UpdatePoints( EDIT_POINTS& aPoints ) override
    {
        // TODO(JE) padstacks
        bool     locked = m_pad.GetParent() && m_pad.IsLocked();
        VECTOR2I shapePos = m_pad.ShapePos( PADSTACK::ALL_LAYERS );
        VECTOR2I halfSize( m_pad.GetSize( PADSTACK::ALL_LAYERS ).x / 2,
                           m_pad.GetSize( PADSTACK::ALL_LAYERS ).y / 2 );

        switch( m_pad.GetShape( PADSTACK::ALL_LAYERS ) )
        {
        case PAD_SHAPE::CIRCLE:
        {
            int target = locked ? 0 : 1;

            // Careful; pad shape is mutable...
            if( int( aPoints.PointsSize() ) != target )
            {
                aPoints.Clear();
                MakePoints( aPoints );
            }
            else if( target == 1 )
            {
                shapePos.x += halfSize.x;
                aPoints.Point( 0 ).SetPosition( shapePos );
            }
        }
        break;

        case PAD_SHAPE::OVAL:
        case PAD_SHAPE::TRAPEZOID:
        case PAD_SHAPE::RECTANGLE:
        case PAD_SHAPE::ROUNDRECT:
        case PAD_SHAPE::CHAMFERED_RECT:
        {
            // Careful; pad shape and orientation are mutable...
            int target = locked || !m_pad.GetOrientation().IsCardinal() ? 0 : 4;

            if( int( aPoints.PointsSize() ) != target )
            {
                aPoints.Clear();
                MakePoints( aPoints );
            }
            else if( target == 4 )
            {
                if( m_pad.GetOrientation() == ANGLE_90 || m_pad.GetOrientation() == ANGLE_270 )
                    std::swap( halfSize.x, halfSize.y );

                aPoints.Point( RECT_TOP_LEFT ).SetPosition( shapePos - halfSize );
                aPoints.Point( RECT_TOP_RIGHT )
                        .SetPosition(
                                VECTOR2I( shapePos.x + halfSize.x, shapePos.y - halfSize.y ) );
                aPoints.Point( RECT_BOT_RIGHT ).SetPosition( shapePos + halfSize );
                aPoints.Point( RECT_BOT_LEFT )
                        .SetPosition(
                                VECTOR2I( shapePos.x - halfSize.x, shapePos.y + halfSize.y ) );
            }

            break;
        }

        default: // suppress warnings
            break;
        }
    }

    void UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                     std::vector<EDA_ITEM*>& aUpdatedItems ) override
    {
        // TODO(JE) padstacks
        switch( m_pad.GetShape( PADSTACK::ALL_LAYERS ) )
        {
        case PAD_SHAPE::CIRCLE:
        {
            VECTOR2I end = aPoints.Point( 0 ).GetPosition();
            int      diameter = 2 * ( end - m_pad.GetPosition() ).EuclideanNorm();

            m_pad.SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( diameter, diameter ) );
            break;
        }

        case PAD_SHAPE::OVAL:
        case PAD_SHAPE::TRAPEZOID:
        case PAD_SHAPE::RECTANGLE:
        case PAD_SHAPE::ROUNDRECT:
        case PAD_SHAPE::CHAMFERED_RECT:
        {
            VECTOR2I topLeft = aPoints.Point( RECT_TOP_LEFT ).GetPosition();
            VECTOR2I topRight = aPoints.Point( RECT_TOP_RIGHT ).GetPosition();
            VECTOR2I botLeft = aPoints.Point( RECT_BOT_LEFT ).GetPosition();
            VECTOR2I botRight = aPoints.Point( RECT_BOT_RIGHT ).GetPosition();
            VECTOR2I holeCenter = m_pad.GetPosition();
            VECTOR2I holeSize = m_pad.GetDrillSize();

            RECTANGLE_POINT_EDIT_BEHAVIOR::PinEditedCorner( aEditedPoint, aPoints, topLeft,
                                                            topRight, botLeft, botRight, holeCenter,
                                                            holeSize );

            if( ( m_pad.GetOffset( PADSTACK::ALL_LAYERS ).x
                  || m_pad.GetOffset( PADSTACK::ALL_LAYERS ).y )
                || ( m_pad.GetDrillSize().x && m_pad.GetDrillSize().y ) )
            {
                // Keep hole pinned at the current location; adjust the pad around the hole

                VECTOR2I center = m_pad.GetPosition();
                int      dist[4];

                if( isModified( aEditedPoint, aPoints.Point( RECT_TOP_LEFT ) )
                    || isModified( aEditedPoint, aPoints.Point( RECT_BOT_RIGHT ) ) )
                {
                    dist[0] = center.x - topLeft.x;
                    dist[1] = center.y - topLeft.y;
                    dist[2] = botRight.x - center.x;
                    dist[3] = botRight.y - center.y;
                }
                else
                {
                    dist[0] = center.x - botLeft.x;
                    dist[1] = center.y - topRight.y;
                    dist[2] = topRight.x - center.x;
                    dist[3] = botLeft.y - center.y;
                }

                VECTOR2I padSize( dist[0] + dist[2], dist[1] + dist[3] );
                VECTOR2I deltaOffset( padSize.x / 2 - dist[2], padSize.y / 2 - dist[3] );

                if( m_pad.GetOrientation() == ANGLE_90 || m_pad.GetOrientation() == ANGLE_270 )
                    std::swap( padSize.x, padSize.y );

                RotatePoint( deltaOffset, -m_pad.GetOrientation() );

                m_pad.SetSize( PADSTACK::ALL_LAYERS, padSize );
                m_pad.SetOffset( PADSTACK::ALL_LAYERS, -deltaOffset );
            }
            else
            {
                // Keep pad position at the center of the pad shape

                int left, top, right, bottom;

                if( isModified( aEditedPoint, aPoints.Point( RECT_TOP_LEFT ) )
                    || isModified( aEditedPoint, aPoints.Point( RECT_BOT_RIGHT ) ) )
                {
                    left = topLeft.x;
                    top = topLeft.y;
                    right = botRight.x;
                    bottom = botRight.y;
                }
                else
                {
                    left = botLeft.x;
                    top = topRight.y;
                    right = topRight.x;
                    bottom = botLeft.y;
                }

                VECTOR2I padSize( abs( right - left ), abs( bottom - top ) );

                if( m_pad.GetOrientation() == ANGLE_90 || m_pad.GetOrientation() == ANGLE_270 )
                    std::swap( padSize.x, padSize.y );

                m_pad.SetSize( PADSTACK::ALL_LAYERS, padSize );
                m_pad.SetPosition( VECTOR2I( ( left + right ) / 2, ( top + bottom ) / 2 ) );
            }
            break;
        }
        default: // suppress warnings
            break;
        }
    }

private:
    PAD& m_pad;
};


/**
 * Point editor behavior for the PCB_GENERATOR class.
 *
 * This just delegates to the PCB_GENERATOR's own methods.
 */
class GENERATOR_POINT_EDIT_BEHAVIOR : public POINT_EDIT_BEHAVIOR
{
public:
    GENERATOR_POINT_EDIT_BEHAVIOR( PCB_GENERATOR& aGenerator ) : m_generator( aGenerator ) {}

    void MakePoints( EDIT_POINTS& aPoints ) override
    {
        m_generator.MakeEditPoints( aPoints );
    }

    void UpdatePoints( EDIT_POINTS& aPoints ) override
    {
        m_generator.UpdateEditPoints( aPoints );
    }

    void UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                     std::vector<EDA_ITEM*>& aUpdatedItems ) override
    {
        m_generator.UpdateFromEditPoints( aPoints );
    }

private:
    PCB_GENERATOR& m_generator;
};


/**
 * Class to help update the text position of a dimension when the crossbar changes.
 *
 * Choosing the right way to update the text position requires some care, and
 * needs to hold some state from the original dimension position so the text can be placed
 * in a similar position relative to the new crossbar. This class handles that state
 * and the logic to find the new text position.
 */
class DIM_ALIGNED_TEXT_UPDATER
{
public:
    DIM_ALIGNED_TEXT_UPDATER( PCB_DIM_ALIGNED& aDimension ) :
            m_dimension( aDimension ), m_originalTextPos( aDimension.GetTextPos() ),
            m_oldCrossBar( SEG{ aDimension.GetCrossbarStart(), aDimension.GetCrossbarEnd() } )
    {
    }

    void UpdateTextAfterChange()
    {
        const SEG newCrossBar{ m_dimension.GetCrossbarStart(), m_dimension.GetCrossbarEnd() };

        if( newCrossBar == m_oldCrossBar )
        {
            // Crossbar didn't change, text doesn't need to change
            return;
        }

        const VECTOR2I newTextPos = getDimensionNewTextPosition();
        m_dimension.SetTextPos( newTextPos );

        const GR_TEXT_H_ALIGN_T oldJustify = m_dimension.GetHorizJustify();

        // We may need to update the justification if we go past vertical.
        if( oldJustify == GR_TEXT_H_ALIGN_T::GR_TEXT_H_ALIGN_LEFT
            || oldJustify == GR_TEXT_H_ALIGN_T::GR_TEXT_H_ALIGN_RIGHT )
        {
            const VECTOR2I oldProject = m_oldCrossBar.LineProject( m_originalTextPos );
            const VECTOR2I newProject = newCrossBar.LineProject( newTextPos );

            const VECTOR2I oldProjectedOffset =
                    oldProject - m_oldCrossBar.NearestPoint( oldProject );
            const VECTOR2I newProjectedOffset = newProject - newCrossBar.NearestPoint( newProject );

            const bool textWasLeftOf = oldProjectedOffset.x < 0
                                       || ( oldProjectedOffset.x == 0 && oldProjectedOffset.y > 0 );
            const bool textIsLeftOf = newProjectedOffset.x < 0
                                      || ( newProjectedOffset.x == 0 && newProjectedOffset.y > 0 );

            if( textWasLeftOf != textIsLeftOf )
            {
                // Flip whatever the user had set
                m_dimension.SetHorizJustify(
                        ( oldJustify == GR_TEXT_H_ALIGN_T::GR_TEXT_H_ALIGN_LEFT )
                                ? GR_TEXT_H_ALIGN_T::GR_TEXT_H_ALIGN_RIGHT
                                : GR_TEXT_H_ALIGN_T::GR_TEXT_H_ALIGN_LEFT );
            }
        }

        // Update the dimension (again) to ensure the text knockouts are correct
        m_dimension.Update();
    }

private:
    VECTOR2I getDimensionNewTextPosition()
    {
        const SEG newCrossBar{ m_dimension.GetCrossbarStart(), m_dimension.GetCrossbarEnd() };

        const EDA_ANGLE oldAngle = EDA_ANGLE( m_oldCrossBar.B - m_oldCrossBar.A );
        const EDA_ANGLE newAngle = EDA_ANGLE( newCrossBar.B - newCrossBar.A );
        const EDA_ANGLE rotation = oldAngle - newAngle;

        // There are two modes - when the text is between the crossbar points, and when it's not.
        if( !KIGEOM::PointProjectsOntoSegment( m_originalTextPos, m_oldCrossBar ) )
        {
            const VECTOR2I cbNearestEndToText =
                    KIGEOM::GetNearestEndpoint( m_oldCrossBar, m_originalTextPos );
            const VECTOR2I rotTextOffsetFromCbCenter =
                    GetRotated( m_originalTextPos - m_oldCrossBar.Center(), rotation );
            const VECTOR2I rotTextOffsetFromCbEnd =
                    GetRotated( m_originalTextPos - cbNearestEndToText, rotation );

            // Which of the two crossbar points is now in the right direction? They could be swapped over now.
            // If zero-length, doesn't matter, they're the same thing
            const bool startIsInOffsetDirection =
                    KIGEOM::PointIsInDirection( m_dimension.GetCrossbarStart(),
                                                rotTextOffsetFromCbCenter, newCrossBar.Center() );

            const VECTOR2I& newCbRefPt = startIsInOffsetDirection ? m_dimension.GetCrossbarStart()
                                                                  : m_dimension.GetCrossbarEnd();

            // Apply the new offset to the correct crossbar point
            return newCbRefPt + rotTextOffsetFromCbEnd;
        }

        // If the text was between the crossbar points, it should stay there, but we need to find a
        // good place for it. Keep it the same distance from the crossbar line, but rotated as needed.

        const VECTOR2I origTextPointProjected = m_oldCrossBar.NearestPoint( m_originalTextPos );
        const double   oldRatio =
                KIGEOM::GetLengthRatioFromStart( origTextPointProjected, m_oldCrossBar );

        // Perpendicular from the crossbar line to the text position
        // We need to keep this length constant
        const VECTOR2I rotCbNormalToText =
                GetRotated( m_originalTextPos - origTextPointProjected, rotation );

        const VECTOR2I newProjected = newCrossBar.A + ( newCrossBar.B - newCrossBar.A ) * oldRatio;
        return newProjected + rotCbNormalToText;
    }

    PCB_DIM_ALIGNED& m_dimension;
    const VECTOR2I   m_originalTextPos;
    const SEG        m_oldCrossBar;
};


/**
 * This covers both aligned and the orthogonal sub-type
 */
class ALIGNED_DIMENSION_POINT_EDIT_BEHAVIOR : public POINT_EDIT_BEHAVIOR
{
public:
    ALIGNED_DIMENSION_POINT_EDIT_BEHAVIOR( PCB_DIM_ALIGNED& aDimension ) : m_dimension( aDimension )
    {
    }

    void MakePoints( EDIT_POINTS& aPoints ) override
    {
        aPoints.AddPoint( m_dimension.GetStart() );
        aPoints.AddPoint( m_dimension.GetEnd() );
        aPoints.AddPoint( m_dimension.GetTextPos() );
        aPoints.AddPoint( m_dimension.GetCrossbarStart() );
        aPoints.AddPoint( m_dimension.GetCrossbarEnd() );

        aPoints.Point( DIM_START ).SetSnapConstraint( ALL_LAYERS );
        aPoints.Point( DIM_END ).SetSnapConstraint( ALL_LAYERS );

        if( m_dimension.Type() == PCB_DIM_ALIGNED_T )
        {
            // Dimension height setting - edit points should move only along the feature lines
            aPoints.Point( DIM_CROSSBARSTART )
                    .SetConstraint( new EC_LINE( aPoints.Point( DIM_CROSSBARSTART ),
                                                 aPoints.Point( DIM_START ) ) );
            aPoints.Point( DIM_CROSSBAREND )
                    .SetConstraint( new EC_LINE( aPoints.Point( DIM_CROSSBAREND ),
                                                 aPoints.Point( DIM_END ) ) );
        }
    }

    void UpdatePoints( EDIT_POINTS& aPoints ) override
    {
        CHECK_POINT_COUNT( aPoints, DIM_ALIGNED_MAX );

        aPoints.Point( DIM_START ).SetPosition( m_dimension.GetStart() );
        aPoints.Point( DIM_END ).SetPosition( m_dimension.GetEnd() );
        aPoints.Point( DIM_TEXT ).SetPosition( m_dimension.GetTextPos() );
        aPoints.Point( DIM_CROSSBARSTART ).SetPosition( m_dimension.GetCrossbarStart() );
        aPoints.Point( DIM_CROSSBAREND ).SetPosition( m_dimension.GetCrossbarEnd() );
    }

    void UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                     std::vector<EDA_ITEM*>& aUpdatedItems ) override
    {
        CHECK_POINT_COUNT( aPoints, DIM_ALIGNED_MAX );

        if( m_dimension.Type() == PCB_DIM_ALIGNED_T )
            updateAlignedDimension( aEditedPoint, aPoints );
        else
            updateOrthogonalDimension( aEditedPoint, aPoints );
    }

    OPT_VECTOR2I Get45DegreeConstrainer( const EDIT_POINT& aEditedPoint,
                                         EDIT_POINTS&      aPoints ) const override
    {
        // Constraint for crossbar
        if( isModified( aEditedPoint, aPoints.Point( DIM_START ) ) )
            return aPoints.Point( DIM_END ).GetPosition();

        else if( isModified( aEditedPoint, aPoints.Point( DIM_END ) ) )
            return aPoints.Point( DIM_START ).GetPosition();

        // No constraint
        return aEditedPoint.GetPosition();
    }

private:
    /**
     * Update non-orthogonal dimension points
     */
    void updateAlignedDimension( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints )
    {
        DIM_ALIGNED_TEXT_UPDATER textPositionUpdater( m_dimension );

        // Check which point is currently modified and updated dimension's points respectively
        if( isModified( aEditedPoint, aPoints.Point( DIM_CROSSBARSTART ) ) )
        {
            VECTOR2D featureLine( aEditedPoint.GetPosition() - m_dimension.GetStart() );
            VECTOR2D crossBar( m_dimension.GetEnd() - m_dimension.GetStart() );

            if( featureLine.Cross( crossBar ) > 0 )
                m_dimension.SetHeight( -featureLine.EuclideanNorm() );
            else
                m_dimension.SetHeight( featureLine.EuclideanNorm() );

            m_dimension.Update();
        }
        else if( isModified( aEditedPoint, aPoints.Point( DIM_CROSSBAREND ) ) )
        {
            VECTOR2D featureLine( aEditedPoint.GetPosition() - m_dimension.GetEnd() );
            VECTOR2D crossBar( m_dimension.GetEnd() - m_dimension.GetStart() );

            if( featureLine.Cross( crossBar ) > 0 )
                m_dimension.SetHeight( -featureLine.EuclideanNorm() );
            else
                m_dimension.SetHeight( featureLine.EuclideanNorm() );

            m_dimension.Update();
        }
        else if( isModified( aEditedPoint, aPoints.Point( DIM_START ) ) )
        {
            m_dimension.SetStart( aEditedPoint.GetPosition() );
            m_dimension.Update();

            aPoints.Point( DIM_CROSSBARSTART )
                    .SetConstraint( new EC_LINE( aPoints.Point( DIM_CROSSBARSTART ),
                                                 aPoints.Point( DIM_START ) ) );
            aPoints.Point( DIM_CROSSBAREND )
                    .SetConstraint( new EC_LINE( aPoints.Point( DIM_CROSSBAREND ),
                                                 aPoints.Point( DIM_END ) ) );
        }
        else if( isModified( aEditedPoint, aPoints.Point( DIM_END ) ) )
        {
            m_dimension.SetEnd( aEditedPoint.GetPosition() );
            m_dimension.Update();

            aPoints.Point( DIM_CROSSBARSTART )
                    .SetConstraint( new EC_LINE( aPoints.Point( DIM_CROSSBARSTART ),
                                                 aPoints.Point( DIM_START ) ) );
            aPoints.Point( DIM_CROSSBAREND )
                    .SetConstraint( new EC_LINE( aPoints.Point( DIM_CROSSBAREND ),
                                                 aPoints.Point( DIM_END ) ) );
        }
        else if( isModified( aEditedPoint, aPoints.Point( DIM_TEXT ) ) )
        {
            // Force manual mode if we weren't already in it
            m_dimension.SetTextPositionMode( DIM_TEXT_POSITION::MANUAL );
            m_dimension.SetTextPos( aEditedPoint.GetPosition() );
            m_dimension.Update();
        }

        textPositionUpdater.UpdateTextAfterChange();
    }

    /**
     * Update orthogonal dimension points
     */
    void updateOrthogonalDimension( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints )
    {
        DIM_ALIGNED_TEXT_UPDATER textPositionUpdater( m_dimension );
        PCB_DIM_ORTHOGONAL&      orthDimension = static_cast<PCB_DIM_ORTHOGONAL&>( m_dimension );

        if( isModified( aEditedPoint, aPoints.Point( DIM_CROSSBARSTART ) )
            || isModified( aEditedPoint, aPoints.Point( DIM_CROSSBAREND ) ) )
        {
            BOX2I bounds( m_dimension.GetStart(), m_dimension.GetEnd() - m_dimension.GetStart() );

            const VECTOR2I& cursorPos = aEditedPoint.GetPosition();

            // Find vector from nearest dimension point to edit position
            VECTOR2I directionA( cursorPos - m_dimension.GetStart() );
            VECTOR2I directionB( cursorPos - m_dimension.GetEnd() );
            VECTOR2I direction = ( directionA < directionB ) ? directionA : directionB;

            bool     vert;
            VECTOR2D featureLine( cursorPos - m_dimension.GetStart() );

            // Only change the orientation when we move outside the bounds
            if( !bounds.Contains( cursorPos ) )
            {
                // If the dimension is horizontal or vertical, set correct orientation
                // otherwise, test if we're left/right of the bounding box or above/below it
                if( bounds.GetWidth() == 0 )
                    vert = true;
                else if( bounds.GetHeight() == 0 )
                    vert = false;
                else if( cursorPos.x > bounds.GetLeft() && cursorPos.x < bounds.GetRight() )
                    vert = false;
                else if( cursorPos.y > bounds.GetTop() && cursorPos.y < bounds.GetBottom() )
                    vert = true;
                else
                    vert = std::abs( direction.y ) < std::abs( direction.x );

                orthDimension.SetOrientation( vert ? PCB_DIM_ORTHOGONAL::DIR::VERTICAL
                                                   : PCB_DIM_ORTHOGONAL::DIR::HORIZONTAL );
            }
            else
            {
                vert = orthDimension.GetOrientation() == PCB_DIM_ORTHOGONAL::DIR::VERTICAL;
            }

            m_dimension.SetHeight( vert ? featureLine.x : featureLine.y );
        }
        else if( isModified( aEditedPoint, aPoints.Point( DIM_START ) ) )
        {
            m_dimension.SetStart( aEditedPoint.GetPosition() );
        }
        else if( isModified( aEditedPoint, aPoints.Point( DIM_END ) ) )
        {
            m_dimension.SetEnd( aEditedPoint.GetPosition() );
        }
        else if( isModified( aEditedPoint, aPoints.Point( DIM_TEXT ) ) )
        {
            // Force manual mode if we weren't already in it
            m_dimension.SetTextPositionMode( DIM_TEXT_POSITION::MANUAL );
            m_dimension.SetTextPos( VECTOR2I( aEditedPoint.GetPosition() ) );
        }

        m_dimension.Update();

        // After recompute, find the new text position
        textPositionUpdater.UpdateTextAfterChange();
    }

    PCB_DIM_ALIGNED& m_dimension;
};


class DIM_CENTER_POINT_EDIT_BEHAVIOR : public POINT_EDIT_BEHAVIOR
{
public:
    DIM_CENTER_POINT_EDIT_BEHAVIOR( PCB_DIM_CENTER& aDimension ) : m_dimension( aDimension ) {}

    void MakePoints( EDIT_POINTS& aPoints ) override
    {
        aPoints.AddPoint( m_dimension.GetStart() );
        aPoints.AddPoint( m_dimension.GetEnd() );

        aPoints.Point( DIM_START ).SetSnapConstraint( ALL_LAYERS );

        aPoints.Point( DIM_END ).SetConstraint(
                new EC_45DEGREE( aPoints.Point( DIM_END ), aPoints.Point( DIM_START ) ) );
        aPoints.Point( DIM_END ).SetSnapConstraint( IGNORE_SNAPS );
    }

    void UpdatePoints( EDIT_POINTS& aPoints ) override
    {
        CHECK_POINT_COUNT( aPoints, DIM_CENTER_MAX );

        aPoints.Point( DIM_START ).SetPosition( m_dimension.GetStart() );
        aPoints.Point( DIM_END ).SetPosition( m_dimension.GetEnd() );
    }

    void UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                     std::vector<EDA_ITEM*>& aUpdatedItems ) override
    {
        CHECK_POINT_COUNT( aPoints, DIM_CENTER_MAX );

        if( isModified( aEditedPoint, aPoints.Point( DIM_START ) ) )
            m_dimension.SetStart( aEditedPoint.GetPosition() );
        else if( isModified( aEditedPoint, aPoints.Point( DIM_END ) ) )
            m_dimension.SetEnd( aEditedPoint.GetPosition() );

        m_dimension.Update();
    }

    OPT_VECTOR2I Get45DegreeConstrainer( const EDIT_POINT& aEditedPoint,
                                         EDIT_POINTS&      aPoints ) const override
    {
        if( isModified( aEditedPoint, aPoints.Point( DIM_END ) ) )
            return aPoints.Point( DIM_START ).GetPosition();

        return std::nullopt;
    }

private:
    PCB_DIM_CENTER& m_dimension;
};


class DIM_RADIAL_POINT_EDIT_BEHAVIOR : public POINT_EDIT_BEHAVIOR
{
public:
    DIM_RADIAL_POINT_EDIT_BEHAVIOR( PCB_DIM_RADIAL& aDimension ) : m_dimension( aDimension ) {}

    void MakePoints( EDIT_POINTS& aPoints ) override
    {
        aPoints.AddPoint( m_dimension.GetStart() );
        aPoints.AddPoint( m_dimension.GetEnd() );
        aPoints.AddPoint( m_dimension.GetTextPos() );
        aPoints.AddPoint( m_dimension.GetKnee() );

        aPoints.Point( DIM_START ).SetSnapConstraint( ALL_LAYERS );
        aPoints.Point( DIM_END ).SetSnapConstraint( ALL_LAYERS );

        aPoints.Point( DIM_KNEE )
                .SetConstraint(
                        new EC_LINE( aPoints.Point( DIM_START ), aPoints.Point( DIM_END ) ) );
        aPoints.Point( DIM_KNEE ).SetSnapConstraint( IGNORE_SNAPS );

        aPoints.Point( DIM_TEXT )
                .SetConstraint(
                        new EC_45DEGREE( aPoints.Point( DIM_TEXT ), aPoints.Point( DIM_KNEE ) ) );
        aPoints.Point( DIM_TEXT ).SetSnapConstraint( IGNORE_SNAPS );
    }

    void UpdatePoints( EDIT_POINTS& aPoints ) override
    {
        CHECK_POINT_COUNT( aPoints, DIM_RADIAL_MAX );

        aPoints.Point( DIM_START ).SetPosition( m_dimension.GetStart() );
        aPoints.Point( DIM_END ).SetPosition( m_dimension.GetEnd() );
        aPoints.Point( DIM_TEXT ).SetPosition( m_dimension.GetTextPos() );
        aPoints.Point( DIM_KNEE ).SetPosition( m_dimension.GetKnee() );
    }

    void UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                     std::vector<EDA_ITEM*>& aUpdatedItems ) override
    {
        CHECK_POINT_COUNT( aPoints, DIM_RADIAL_MAX );

        if( isModified( aEditedPoint, aPoints.Point( DIM_START ) ) )
        {
            m_dimension.SetStart( aEditedPoint.GetPosition() );
            m_dimension.Update();

            aPoints.Point( DIM_KNEE )
                    .SetConstraint(
                            new EC_LINE( aPoints.Point( DIM_START ), aPoints.Point( DIM_END ) ) );
        }
        else if( isModified( aEditedPoint, aPoints.Point( DIM_END ) ) )
        {
            VECTOR2I oldKnee = m_dimension.GetKnee();

            m_dimension.SetEnd( aEditedPoint.GetPosition() );
            m_dimension.Update();

            VECTOR2I kneeDelta = m_dimension.GetKnee() - oldKnee;
            m_dimension.SetTextPos( m_dimension.GetTextPos() + kneeDelta );
            m_dimension.Update();

            aPoints.Point( DIM_KNEE )
                    .SetConstraint(
                            new EC_LINE( aPoints.Point( DIM_START ), aPoints.Point( DIM_END ) ) );
        }
        else if( isModified( aEditedPoint, aPoints.Point( DIM_KNEE ) ) )
        {
            VECTOR2I oldKnee = m_dimension.GetKnee();
            VECTOR2I arrowVec = aPoints.Point( DIM_KNEE ).GetPosition()
                                - aPoints.Point( DIM_END ).GetPosition();

            m_dimension.SetLeaderLength( arrowVec.EuclideanNorm() );
            m_dimension.Update();

            VECTOR2I kneeDelta = m_dimension.GetKnee() - oldKnee;
            m_dimension.SetTextPos( m_dimension.GetTextPos() + kneeDelta );
            m_dimension.Update();
        }
        else if( isModified( aEditedPoint, aPoints.Point( DIM_TEXT ) ) )
        {
            m_dimension.SetTextPos( aEditedPoint.GetPosition() );
            m_dimension.Update();
        }
    }

    OPT_VECTOR2I Get45DegreeConstrainer( const EDIT_POINT& aEditedPoint,
                                         EDIT_POINTS&      aPoints ) const override
    {
        if( isModified( aEditedPoint, aPoints.Point( DIM_TEXT ) ) )
            return aPoints.Point( DIM_KNEE ).GetPosition();

        return std::nullopt;
    }

private:
    PCB_DIM_RADIAL& m_dimension;
};


class DIM_LEADER_POINT_EDIT_BEHAVIOR : public POINT_EDIT_BEHAVIOR
{
public:
    DIM_LEADER_POINT_EDIT_BEHAVIOR( PCB_DIM_LEADER& aDimension ) : m_dimension( aDimension ) {}

    void MakePoints( EDIT_POINTS& aPoints ) override
    {
        aPoints.AddPoint( m_dimension.GetStart() );
        aPoints.AddPoint( m_dimension.GetEnd() );
        aPoints.AddPoint( m_dimension.GetTextPos() );

        aPoints.Point( DIM_START ).SetSnapConstraint( ALL_LAYERS );
        aPoints.Point( DIM_END ).SetSnapConstraint( ALL_LAYERS );

        aPoints.Point( DIM_TEXT )
                .SetConstraint(
                        new EC_45DEGREE( aPoints.Point( DIM_TEXT ), aPoints.Point( DIM_END ) ) );
        aPoints.Point( DIM_TEXT ).SetSnapConstraint( IGNORE_SNAPS );
    }

    void UpdatePoints( EDIT_POINTS& aPoints ) override
    {
        CHECK_POINT_COUNT( aPoints, DIM_LEADER_MAX );

        aPoints.Point( DIM_START ).SetPosition( m_dimension.GetStart() );
        aPoints.Point( DIM_END ).SetPosition( m_dimension.GetEnd() );
        aPoints.Point( DIM_TEXT ).SetPosition( m_dimension.GetTextPos() );
    }

    void UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                     std::vector<EDA_ITEM*>& aUpdatedItems ) override
    {
        CHECK_POINT_COUNT( aPoints, DIM_LEADER_MAX );

        if( isModified( aEditedPoint, aPoints.Point( DIM_START ) ) )
        {
            m_dimension.SetStart( aEditedPoint.GetPosition() );
        }
        else if( isModified( aEditedPoint, aPoints.Point( DIM_END ) ) )
        {
            const VECTOR2I newPoint( aEditedPoint.GetPosition() );
            const VECTOR2I delta = newPoint - m_dimension.GetEnd();

            m_dimension.SetEnd( newPoint );
            m_dimension.SetTextPos( m_dimension.GetTextPos() + delta );
        }
        else if( isModified( aEditedPoint, aPoints.Point( DIM_TEXT ) ) )
        {
            m_dimension.SetTextPos( aEditedPoint.GetPosition() );
        }

        m_dimension.Update();
    }

private:
    PCB_DIM_LEADER& m_dimension;
};


/**
 * A textbox is edited as a rectnagle when it is orthogonally aligned
 */
class TEXTBOX_POINT_EDIT_BEHAVIOR : public POINT_EDIT_BEHAVIOR
{
public:
    TEXTBOX_POINT_EDIT_BEHAVIOR( PCB_TEXTBOX& aTextbox ) : m_textbox( aTextbox ) {}

    void MakePoints( EDIT_POINTS& aPoints ) override
    {
        if( m_textbox.GetShape() == SHAPE_T::RECTANGLE )
        {
            RECTANGLE_POINT_EDIT_BEHAVIOR::MakePoints( m_textbox, aPoints );
        }
        else
        {
            // Rotated textboxes are implemented as polygons and these
            // aren't currently editable.
        }
    }

    void UpdatePoints( EDIT_POINTS& aPoints ) override
    {
        // When textboxes are rotated, they act as polygons, not rectangles
        const unsigned target = m_textbox.GetShape() == SHAPE_T::RECTANGLE
                                        ? TEXTBOX_POINT_COUNT::WHEN_RECTANGLE
                                        : TEXTBOX_POINT_COUNT::WHEN_POLYGON;

        // Careful; textbox shape is mutable between cardinal and non-cardinal rotations...
        if( aPoints.PointsSize() != target )
        {
            aPoints.Clear();
            MakePoints( aPoints );
            return;
        }

        if( m_textbox.GetShape() == SHAPE_T::RECTANGLE )
        {
            // Dispatch to the rectangle behavior
            RECTANGLE_POINT_EDIT_BEHAVIOR::UpdatePoints( m_textbox, aPoints );
        }
        else if( m_textbox.GetShape() == SHAPE_T::POLY )
        {
            // Not currently editable while rotated.
        }
    }

    void UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                     std::vector<EDA_ITEM*>& aUpdatedItems ) override
    {
        if( m_textbox.GetShape() == SHAPE_T::RECTANGLE )
        {
            RECTANGLE_POINT_EDIT_BEHAVIOR::UpdateItem( m_textbox, aEditedPoint, aPoints );
        }
    }

private:
    PCB_TEXTBOX& m_textbox;
};

PCB_POINT_EDITOR::PCB_POINT_EDITOR() :
        PCB_TOOL_BASE( "pcbnew.PointEditor" ), m_selectionTool( nullptr ), m_editedPoint( nullptr ),
        m_hoveredPoint( nullptr ), m_original( VECTOR2I( 0, 0 ) ),
        m_arcEditMode( ARC_EDIT_MODE::KEEP_CENTER_ADJUST_ANGLE_RADIUS ),
        m_altConstrainer( VECTOR2I( 0, 0 ) ), m_inPointEditorTool( false )
{
}


void PCB_POINT_EDITOR::Reset( RESET_REASON aReason )
{
    m_editPoints.reset();
    m_altConstraint.reset();
    getViewControls()->SetAutoPan( false );
}


bool PCB_POINT_EDITOR::Init()
{
    // Find the selection tool, so they can cooperate
    m_selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    wxASSERT_MSG( m_selectionTool, wxT( "pcbnew.InteractiveSelection tool is not available" ) );

    auto& menu = m_selectionTool->GetToolMenu().GetMenu();
    menu.AddItem( PCB_ACTIONS::pointEditorAddCorner, PCB_POINT_EDITOR::addCornerCondition );
    menu.AddItem( PCB_ACTIONS::pointEditorRemoveCorner,
                  std::bind( &PCB_POINT_EDITOR::removeCornerCondition, this, _1 ) );
    menu.AddItem( PCB_ACTIONS::pointEditorChamferCorner, PCB_POINT_EDITOR::addCornerCondition );

    return true;
}


std::shared_ptr<EDIT_POINTS> PCB_POINT_EDITOR::makePoints( EDA_ITEM* aItem )
{
    std::shared_ptr<EDIT_POINTS> points = std::make_shared<EDIT_POINTS>( aItem );

    if( !aItem )
        return points;

    // Reset the behaviour and we'll make a new one
    m_editorBehavior = nullptr;

    switch( aItem->Type() )
    {
    case PCB_REFERENCE_IMAGE_T:
    {
        PCB_REFERENCE_IMAGE& refImage = static_cast<PCB_REFERENCE_IMAGE&>( *aItem );
        m_editorBehavior = std::make_unique<REFERENCE_IMAGE_POINT_EDIT_BEHAVIOR>( refImage );
        break;
    }
    case PCB_TEXTBOX_T:
    {
        PCB_TEXTBOX& textbox = static_cast<PCB_TEXTBOX&>( *aItem );
        m_editorBehavior = std::make_unique<TEXTBOX_POINT_EDIT_BEHAVIOR>( textbox );
        break;
    }
    case PCB_SHAPE_T:
    {
        PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( aItem );

        switch( shape->GetShape() )
        {
        case SHAPE_T::SEGMENT:
            m_editorBehavior = std::make_unique<EDA_SEGMENT_POINT_EDIT_BEHAVIOR>( *shape );
            break;
        case SHAPE_T::RECTANGLE:
            m_editorBehavior = std::make_unique<RECTANGLE_POINT_EDIT_BEHAVIOR>( *shape );
            break;
        case SHAPE_T::ARC:
            m_editorBehavior = std::make_unique<ARC_POINT_EDIT_BEHAVIOR>( *shape, m_arcEditMode,
                                                                          *getViewControls() );
            break;

        case SHAPE_T::CIRCLE:
            m_editorBehavior = std::make_unique<EDA_CIRCLE_POINT_EDIT_BEHAVIOR>( *shape );
            break;

        case SHAPE_T::POLY:
            m_editorBehavior = std::make_unique<EDA_POLYGON_POINT_EDIT_BEHAVIOR>( *shape );
            break;

        case SHAPE_T::BEZIER:
            m_editorBehavior = std::make_unique<EDA_BEZIER_POINT_EDIT_BEHAVIOR>( *shape );
            break;

        default:        // suppress warnings
            break;
        }

        break;
    }

    case PCB_TABLECELL_T:
    {
        PCB_TABLECELL* cell = static_cast<PCB_TABLECELL*>( aItem );
        m_editorBehavior = std::make_unique<PCB_TABLECELL_POINT_EDIT_BEHAVIOR>( *cell );
        break;
    }

    case PCB_PAD_T:
    {
        // Pad edit only for the footprint editor
        if( m_isFootprintEditor )
        {
            PAD& pad = static_cast<PAD&>( *aItem );
            m_editorBehavior = std::make_unique<PAD_POINT_EDIT_BEHAVIOR>( pad );
        }
        break;
    }

    case PCB_ZONE_T:
    {
        ZONE& zone = static_cast<ZONE&>( *aItem );
        m_editorBehavior = std::make_unique<ZONE_POINT_EDIT_BEHAVIOR>( zone );
        break;
    }

    case PCB_GENERATOR_T:
    {
        PCB_GENERATOR* generator = static_cast<PCB_GENERATOR*>( aItem );
        m_editorBehavior = std::make_unique<GENERATOR_POINT_EDIT_BEHAVIOR>( *generator );
        break;
    }

    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_ORTHOGONAL_T:
    {
        PCB_DIM_ALIGNED& dimension = static_cast<PCB_DIM_ALIGNED&>( *aItem );
        m_editorBehavior = std::make_unique<ALIGNED_DIMENSION_POINT_EDIT_BEHAVIOR>( dimension );
        break;
    }

    case PCB_DIM_CENTER_T:
    {
        PCB_DIM_CENTER& dimension = static_cast<PCB_DIM_CENTER&>( *aItem );
        m_editorBehavior = std::make_unique<DIM_CENTER_POINT_EDIT_BEHAVIOR>( dimension );
        break;
    }

    case PCB_DIM_RADIAL_T:
    {
        PCB_DIM_RADIAL& dimension = static_cast<PCB_DIM_RADIAL&>( *aItem );
        m_editorBehavior = std::make_unique<DIM_RADIAL_POINT_EDIT_BEHAVIOR>( dimension );
        break;
    }

    case PCB_DIM_LEADER_T:
    {
        PCB_DIM_LEADER& dimension = static_cast<PCB_DIM_LEADER&>( *aItem );
        m_editorBehavior = std::make_unique<DIM_LEADER_POINT_EDIT_BEHAVIOR>( dimension );
        break;
    }

    default:
        points.reset();
        break;
    }

    if( m_editorBehavior )
        m_editorBehavior->MakePoints( *points );

    return points;
}


void PCB_POINT_EDITOR::updateEditedPoint( const TOOL_EVENT& aEvent )
{
    EDIT_POINT* point;
    EDIT_POINT* hovered = nullptr;

    if( aEvent.IsMotion() )
    {
        point = m_editPoints->FindPoint( aEvent.Position(), getView() );
        hovered = point;
    }
    else if( aEvent.IsDrag( BUT_LEFT ) )
    {
        point = m_editPoints->FindPoint( aEvent.DragOrigin(), getView() );
    }
    else
    {
        point = m_editPoints->FindPoint( getViewControls()->GetCursorPosition(), getView() );
    }

    if( hovered )
    {
        if( m_hoveredPoint != hovered )
        {
            if( m_hoveredPoint )
                m_hoveredPoint->SetHover( false );

            m_hoveredPoint = hovered;
            m_hoveredPoint->SetHover();
        }
    }
    else if( m_hoveredPoint )
    {
        m_hoveredPoint->SetHover( false );
        m_hoveredPoint = nullptr;
    }

    if( m_editedPoint != point )
        setEditedPoint( point );
}


int PCB_POINT_EDITOR::OnSelectionChange( const TOOL_EVENT& aEvent )
{
    if( !m_selectionTool || aEvent.Matches( EVENTS::InhibitSelectionEditing ) )
        return 0;

    if( m_inPointEditorTool )
        return 0;

    REENTRANCY_GUARD guard( &m_inPointEditorTool );

    PCB_BASE_EDIT_FRAME* editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();
    const PCB_SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.Size() != 1 || selection.Front()->GetEditFlags() )
        return 0;

    BOARD_ITEM* item = static_cast<BOARD_ITEM*>( selection.Front() );

    if( !item || item->IsLocked() )
        return 0;

    Activate();
    // Must be done after Activate() so that it gets set into the correct context
    getViewControls()->ShowCursor( true );

    PCB_GRID_HELPER grid( m_toolMgr, editFrame->GetMagneticItemsSettings() );

    // Use the original object as a construction item
    std::unique_ptr<BOARD_ITEM> clone;

    m_editorBehavior.reset();
    // Will also make the edit behavior if supported
    m_editPoints = makePoints( item );

    if( !m_editPoints )
        return 0;

    m_preview.FreeItems();
    getView()->Add( &m_preview );

    getView()->Add( m_editPoints.get() );
    setEditedPoint( nullptr );
    updateEditedPoint( aEvent );
    bool inDrag = false;
    bool useAltContraint = false;

    BOARD_COMMIT commit( editFrame );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );

        if( editFrame->IsType( FRAME_PCB_EDITOR ) )
        {
            useAltContraint = editFrame->GetPcbNewSettings()->m_Use45DegreeLimit;
            m_arcEditMode = editFrame->GetPcbNewSettings()->m_ArcEditMode;
        }
        else
        {
            useAltContraint = editFrame->GetFootprintEditorSettings()->m_Use45Limit;
            m_arcEditMode = editFrame->GetFootprintEditorSettings()->m_ArcEditMode;
        }

        if( !m_editPoints || evt->IsSelectionEvent() ||
                evt->Matches( EVENTS::InhibitSelectionEditing ) )
        {
            break;
        }

        EDIT_POINT* prevHover = m_hoveredPoint;

        if( !inDrag )
            updateEditedPoint( *evt );

        if( prevHover != m_hoveredPoint )
            getView()->Update( m_editPoints.get() );

        if( evt->IsDrag( BUT_LEFT ) && m_editedPoint )
        {
            if( !inDrag )
            {
                frame()->UndoRedoBlock( true );

                if( item->Type() == PCB_GENERATOR_T )
                {
                    m_toolMgr->RunSynchronousAction( PCB_ACTIONS::genStartEdit, &commit,
                                                     static_cast<PCB_GENERATOR*>( item ) );
                }

                getViewControls()->ForceCursorPosition( false );
                m_original = *m_editedPoint;    // Save the original position
                getViewControls()->SetAutoPan( true );
                inDrag = true;

                if( m_editedPoint->GetGridConstraint() != SNAP_BY_GRID )
                    grid.SetAuxAxes( true, m_original.GetPosition() );

                setAltConstraint( true );
                m_editedPoint->SetActive();

                for( size_t ii = 0; ii < m_editPoints->PointsSize(); ++ii )
                {
                    EDIT_POINT& point = m_editPoints->Point( ii );

                    if( &point != m_editedPoint )
                        point.SetActive( false );
                }

                // When we start dragging, create a clone of the item to use as the original
                // reference geometry (e.g. for intersections and extensions)
                std::unique_ptr<BOARD_ITEM> oldClone = std::move( clone );
                clone.reset( static_cast<BOARD_ITEM*>( item->Clone() ) );
                grid.AddConstructionItems( { clone.get() }, false, true );
                // Now old clone can be safely deleted
            }

            // Keep point inside of limits with some padding
            VECTOR2I pos = GetClampedCoords<double, int>( evt->Position(), COORDS_PADDING );
            LSET     snapLayers;

            switch( m_editedPoint->GetSnapConstraint() )
            {
            case IGNORE_SNAPS:                                      break;
            case OBJECT_LAYERS: snapLayers = item->GetLayerSet();   break;
            case ALL_LAYERS:    snapLayers = LSET::AllLayersMask(); break;
            }

            if( m_editedPoint->GetGridConstraint() == SNAP_BY_GRID )
            {
                if( grid.GetUseGrid() )
                {
                    VECTOR2I gridPt = grid.BestSnapAnchor( pos, {}, grid.GetItemGrid( item ),
                                                           { item } );

                    VECTOR2I last = m_editedPoint->GetPosition();
                    VECTOR2I delta = pos - last;
                    VECTOR2I deltaGrid = gridPt - grid.BestSnapAnchor( last, {},
                                                                       grid.GetItemGrid( item ),
                                                                       { item } );

                    if( abs( delta.x ) > grid.GetGrid().x / 2 )
                        pos.x = last.x + deltaGrid.x;
                    else
                        pos.x = last.x;

                    if( abs( delta.y ) > grid.GetGrid().y / 2 )
                        pos.y = last.y + deltaGrid.y;
                    else
                        pos.y = last.y;
                }
            }

            m_editedPoint->SetPosition( pos );

            // The alternative constraint limits to 45 degrees
            if( useAltContraint )
            {
                m_altConstraint->Apply( grid );
            }
            else if( m_editedPoint->IsConstrained() )
            {
                m_editedPoint->ApplyConstraint( grid );
            }
            else if( m_editedPoint->GetGridConstraint() == SNAP_TO_GRID )
            {
                m_editedPoint->SetPosition( grid.BestSnapAnchor( m_editedPoint->GetPosition(),
                                                                 snapLayers,
                                                                 grid.GetItemGrid( item ),
                                                                 { item } ) );
            }

            updateItem( commit );
            getViewControls()->ForceCursorPosition( true, m_editedPoint->GetPosition() );
            updatePoints();
        }
        else if( m_editedPoint && evt->Action() == TA_MOUSE_DOWN && evt->Buttons() == BUT_LEFT )
        {
            m_editedPoint->SetActive();

            for( size_t ii = 0; ii < m_editPoints->PointsSize(); ++ii )
            {
                EDIT_POINT& point = m_editPoints->Point( ii );

                if( &point != m_editedPoint )
                    point.SetActive( false );
            }

            getView()->Update( m_editPoints.get() );
        }
        else if( inDrag && evt->IsMouseUp( BUT_LEFT ) )
        {
            if( m_editedPoint )
            {
                m_editedPoint->SetActive( false );
                getView()->Update( m_editPoints.get() );
            }

            getViewControls()->SetAutoPan( false );
            setAltConstraint( false );

            if( item->Type() == PCB_GENERATOR_T )
            {
                m_preview.FreeItems();
                m_toolMgr->RunSynchronousAction( PCB_ACTIONS::genPushEdit, &commit,
                                                 static_cast<PCB_GENERATOR*>( item ) );
            }
            else if( item->Type() == PCB_TABLECELL_T )
            {
                commit.Push( _( "Resize Table Cells" ) );
            }
            else
            {
                commit.Push( _( "Move Point" ) );
            }

            inDrag = false;
            frame()->UndoRedoBlock( false );

            m_toolMgr->PostAction<EDA_ITEM*>( PCB_ACTIONS::reselectItem,
                                              item ); // FIXME: Needed for generators
        }
        else if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            if( inDrag )      // Restore the last change
            {
                if( item->Type() == PCB_GENERATOR_T )
                {
                    m_toolMgr->RunSynchronousAction( PCB_ACTIONS::genRevertEdit, &commit,
                                                     static_cast<PCB_GENERATOR*>( item ) );
                }
                commit.Revert();

                inDrag = false;
                frame()->UndoRedoBlock( false );
            }

            // Only cancel point editor when activating a new tool
            // Otherwise, allow the points to persist when moving up the
            // tool stack
            if( evt->IsActivate() && !evt->IsMoveTool() )
                break;
        }
        else if( evt->Action() == TA_UNDO_REDO_POST )
        {
            break;
        }
        else
        {
            evt->SetPassEvent();
        }
    }

    m_preview.FreeItems();
    getView()->Remove( &m_preview );

    if( m_editPoints )
    {
        getView()->Remove( m_editPoints.get() );
        m_editPoints.reset();
    }

    m_editedPoint = nullptr;

    return 0;
}


int PCB_POINT_EDITOR::movePoint( const TOOL_EVENT& aEvent )
{
    if( !m_editPoints || !m_editPoints->GetParent() || !HasPoint() )
        return 0;

    PCB_BASE_EDIT_FRAME* editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();

    BOARD_COMMIT commit( editFrame );
    commit.Stage( m_editPoints->GetParent(), CHT_MODIFY );

    VECTOR2I pt = editFrame->GetOriginTransforms().ToDisplayAbs( m_editedPoint->GetPosition() );
    wxString title;
    wxString msg;

    if( dynamic_cast<EDIT_LINE*>( m_editedPoint ) )
    {
        title = _( "Move Midpoint to Location" );
        msg = _( "Move Midpoint" );
    }
    else
    {
        title = _( "Move Corner to Location" );
        msg = _( "Move Corner" );
    }

    WX_PT_ENTRY_DIALOG dlg( editFrame, title, _( "X:" ), _( "Y:" ), pt );

    if( dlg.ShowModal() == wxID_OK )
    {
        pt = editFrame->GetOriginTransforms().FromDisplayAbs( dlg.GetValue() );
        m_editedPoint->SetPosition( pt );
        updateItem( commit );
        commit.Push( msg );
    }

    return 0;
}


void PCB_POINT_EDITOR::updateItem( BOARD_COMMIT& aCommit )
{
    wxCHECK( m_editPoints, /* void */ );
    EDA_ITEM* item = m_editPoints->GetParent();

    if( !item )
        return;

    // item is always updated
    std::vector<EDA_ITEM*> updatedItems = { item };
    aCommit.Modify( item );

    if( m_editorBehavior )
    {
        wxCHECK( m_editedPoint, /* void */ );
        m_editorBehavior->UpdateItem( *m_editedPoint, *m_editPoints, aCommit, updatedItems );
    }

    // Perform any post-edit actions that the item may require

    switch( item->Type() )
    {
    case PCB_TEXTBOX_T:
    case PCB_SHAPE_T:
    {
        PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( item );

        if( shape->IsProxyItem() )
        {
            for( PAD* pad : shape->GetParentFootprint()->Pads() )
            {
                if( pad->IsEntered() )
                    view()->Update( pad );
            }
        }

        // Nuke outline font render caches
        if( PCB_TEXTBOX* textBox = dynamic_cast<PCB_TEXTBOX*>( item ) )
            textBox->ClearRenderCache();

        break;
    }
    case PCB_GENERATOR_T:
    {
        GENERATOR_TOOL* generatorTool = m_toolMgr->GetTool<GENERATOR_TOOL>();
        PCB_GENERATOR*  generatorItem = static_cast<PCB_GENERATOR*>( item );

        m_toolMgr->RunSynchronousAction( PCB_ACTIONS::genUpdateEdit, &aCommit, generatorItem );

        // Note: POINT_EDITOR::m_preview holds only the canvas-draw status "popup"; the meanders
        // themselves (ROUTER_PREVIEW_ITEMs) are owned by the router.

        m_preview.FreeItems();

        for( EDA_ITEM* previewItem : generatorItem->GetPreviewItems( generatorTool, frame(),
                                                                     STATUS_ITEMS_ONLY ) )
        {
            m_preview.Add( previewItem );
        }

        getView()->Update( &m_preview );
        break;
    }
    default:
        break;
    }

    // Update the item and any affected items
    for( EDA_ITEM* updatedItem : updatedItems )
    {
        getView()->Update( updatedItem );
    }

    frame()->SetMsgPanel( item );
}


void PCB_POINT_EDITOR::updatePoints()
{
    if( !m_editPoints )
        return;

    EDA_ITEM* item = m_editPoints->GetParent();

    if( !item )
        return;

    if( !m_editorBehavior )
        return;

    m_editorBehavior->UpdatePoints( *m_editPoints );
    getView()->Update( m_editPoints.get() );
}


void PCB_POINT_EDITOR::setEditedPoint( EDIT_POINT* aPoint )
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();

    if( aPoint )
    {
        frame()->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
        controls->ForceCursorPosition( true, aPoint->GetPosition() );
        controls->ShowCursor( true );
    }
    else
    {
        if( frame()->ToolStackIsEmpty() )
            controls->ShowCursor( false );

        controls->ForceCursorPosition( false );
    }

    m_editedPoint = aPoint;
}


void PCB_POINT_EDITOR::setAltConstraint( bool aEnabled )
{
    if( aEnabled )
    {
        EDA_ITEM*  parent = m_editPoints->GetParent();
        EDIT_LINE* line = dynamic_cast<EDIT_LINE*>( m_editedPoint );
        bool       isPoly;

        switch( parent->Type() )
        {
        case PCB_ZONE_T:
            isPoly = true;
            break;

        case PCB_SHAPE_T:
            isPoly = static_cast<PCB_SHAPE*>( parent )->GetShape() == SHAPE_T::POLY;
            break;

        default:
            isPoly = false;
            break;
        }

        if( line && isPoly )
        {
            EC_CONVERGING* altConstraint = new EC_CONVERGING( *line, *m_editPoints );
            m_altConstraint.reset( (EDIT_CONSTRAINT<EDIT_POINT>*) altConstraint );
        }
        else
        {
            // Find a proper constraining point for 45 degrees mode
            m_altConstrainer = get45DegConstrainer();
            m_altConstraint.reset( new EC_45DEGREE( *m_editedPoint, m_altConstrainer ) );
        }
    }
    else
    {
        m_altConstraint.reset();
    }
}


EDIT_POINT PCB_POINT_EDITOR::get45DegConstrainer() const
{
    // If there's a behaviour and it provides a constrainer, use that
    if( m_editorBehavior )
    {
        const OPT_VECTOR2I constrainer =
                m_editorBehavior->Get45DegreeConstrainer( *m_editedPoint, *m_editPoints );
        if( constrainer )
            return EDIT_POINT( *constrainer );
    }

    // In any other case we may align item to its original position
    return m_original;
}


bool PCB_POINT_EDITOR::canAddCorner( const EDA_ITEM& aItem )
{
    const auto type = aItem.Type();

    // Works only for zones and line segments
    if( type == PCB_ZONE_T )
        return true;

    if( type == PCB_SHAPE_T )
    {
        const PCB_SHAPE& shape = static_cast<const PCB_SHAPE&>( aItem );
        const SHAPE_T    shapeType = shape.GetShape();
        return shapeType == SHAPE_T::SEGMENT || shapeType == SHAPE_T::POLY
               || shapeType == SHAPE_T::ARC;
    }

    return false;
}


bool PCB_POINT_EDITOR::addCornerCondition( const SELECTION& aSelection )
{
    if( aSelection.Size() != 1 )
        return false;

    const EDA_ITEM* item = aSelection.Front();

    return ( item != nullptr ) && canAddCorner( *item );
}


// Finds a corresponding vertex in a polygon set
static std::pair<bool, SHAPE_POLY_SET::VERTEX_INDEX>
findVertex( SHAPE_POLY_SET& aPolySet, const EDIT_POINT& aPoint )
{
    for( auto it = aPolySet.IterateWithHoles(); it; ++it )
    {
        auto vertexIdx = it.GetIndex();

        if( aPolySet.CVertex( vertexIdx ) == aPoint.GetPosition() )
            return std::make_pair( true, vertexIdx );
    }

    return std::make_pair( false, SHAPE_POLY_SET::VERTEX_INDEX() );
}


bool PCB_POINT_EDITOR::removeCornerCondition( const SELECTION& )
{
    if( !m_editPoints || !m_editedPoint )
        return false;

    EDA_ITEM*       item = m_editPoints->GetParent();
    SHAPE_POLY_SET* polyset = nullptr;

    if( !item )
        return false;

    switch( item->Type() )
    {
    case PCB_ZONE_T:
        polyset = static_cast<ZONE*>( item )->Outline();
        break;

    case PCB_SHAPE_T:
        if( static_cast<PCB_SHAPE*>( item )->GetShape() == SHAPE_T::POLY )
            polyset = &static_cast<PCB_SHAPE*>( item )->GetPolyShape();
        else
            return false;

        break;

    default:
        return false;
    }

    std::pair<bool, SHAPE_POLY_SET::VERTEX_INDEX> vertex = findVertex( *polyset, *m_editedPoint );

    if( !vertex.first )
        return false;

    const SHAPE_POLY_SET::VERTEX_INDEX& vertexIdx = vertex.second;

    // Check if there are enough vertices so one can be removed without
    // degenerating the polygon.
    // The first condition allows one to remove all corners from holes (when
    // there are only 2 vertices left, a hole is removed).
    if( vertexIdx.m_contour == 0 &&
        polyset->Polygon( vertexIdx.m_polygon )[vertexIdx.m_contour].PointCount() <= 3 )
    {
        return false;
    }

    // Remove corner does not work with lines
    if( dynamic_cast<EDIT_LINE*>( m_editedPoint ) )
        return false;

    return m_editedPoint != nullptr;
}


int PCB_POINT_EDITOR::addCorner( const TOOL_EVENT& aEvent )
{
    if( !m_editPoints )
        return 0;

    EDA_ITEM*            item = m_editPoints->GetParent();
    PCB_BASE_EDIT_FRAME* frame = getEditFrame<PCB_BASE_EDIT_FRAME>();
    const VECTOR2I&      cursorPos = getViewControls()->GetCursorPosition();

    // called without an active edited polygon
    if( !item || !canAddCorner( *item ) )
        return 0;

    PCB_SHAPE* graphicItem = dynamic_cast<PCB_SHAPE*>( item );
    BOARD_COMMIT commit( frame );

    if( item->Type() == PCB_ZONE_T || ( graphicItem && graphicItem->GetShape() == SHAPE_T::POLY ) )
    {
        unsigned int nearestIdx = 0;
        unsigned int nextNearestIdx = 0;
        unsigned int nearestDist = INT_MAX;
        unsigned int firstPointInContour = 0;
        SHAPE_POLY_SET* zoneOutline;

        if( item->Type() == PCB_ZONE_T )
        {
            ZONE* zone = static_cast<ZONE*>( item );
            zoneOutline = zone->Outline();
            zone->SetNeedRefill( true );
        }
        else
        {
            zoneOutline = &( graphicItem->GetPolyShape() );
        }

        commit.Modify( item );

        // Search the best outline segment to add a new corner
        // and therefore break this segment into two segments

        // Object to iterate through the corners of the outlines (main contour and its holes)
        SHAPE_POLY_SET::ITERATOR iterator = zoneOutline->Iterate( 0, zoneOutline->OutlineCount()-1,
                                                                  /* IterateHoles */ true );
        int curr_idx = 0;

        // Iterate through all the corners of the outlines and search the best segment
        for( ; iterator; iterator++, curr_idx++ )
        {
            int jj = curr_idx+1;

            if( iterator.IsEndContour() )
            {   // We reach the last point of the current contour (main or hole)
                jj = firstPointInContour;
                firstPointInContour = curr_idx+1;     // Prepare next contour analysis
            }

            SEG curr_segment( zoneOutline->CVertex( curr_idx ), zoneOutline->CVertex( jj ) );

            unsigned int distance = curr_segment.Distance( cursorPos );

            if( distance < nearestDist )
            {
                nearestDist = distance;
                nearestIdx = curr_idx;
                nextNearestIdx = jj;
            }
        }

        // Find the point on the closest segment
        const VECTOR2I& sideOrigin = zoneOutline->CVertex( nearestIdx );
        const VECTOR2I& sideEnd = zoneOutline->CVertex( nextNearestIdx );
        SEG             nearestSide( sideOrigin, sideEnd );
        VECTOR2I        nearestPoint = nearestSide.NearestPoint( cursorPos );

        // Do not add points that have the same coordinates as ones that already belong to polygon
        // instead, add a point in the middle of the side
        if( nearestPoint == sideOrigin || nearestPoint == sideEnd )
            nearestPoint = ( sideOrigin + sideEnd ) / 2;

        zoneOutline->InsertVertex( nextNearestIdx, nearestPoint );

        // We re-hatch the filled zones but not polygons
        if( item->Type() == PCB_ZONE_T )
            static_cast<ZONE*>( item )->HatchBorder();

        commit.Push( _( "Add Zone Corner" ) );
    }
    else if( graphicItem )
    {
        switch( graphicItem->GetShape() )
        {
        case SHAPE_T::SEGMENT:
        {
            commit.Modify( graphicItem );

            SEG      seg( graphicItem->GetStart(), graphicItem->GetEnd() );
            VECTOR2I nearestPoint = seg.NearestPoint( cursorPos );

            // Move the end of the line to the break point..
            graphicItem->SetEnd( nearestPoint );

            // and add another one starting from the break point
            auto newSegment = std::make_unique<PCB_SHAPE>( *graphicItem );

            newSegment->ClearSelected();
            newSegment->SetStart( nearestPoint );
            newSegment->SetEnd( VECTOR2I( seg.B.x, seg.B.y ) );

            commit.Add( newSegment.release() );
            commit.Push( _( "Split Segment" ) );
            break;
        }
        case SHAPE_T::ARC:
        {
            commit.Modify( graphicItem );

            const SHAPE_ARC arc( graphicItem->GetStart(), graphicItem->GetArcMid(),
                                 graphicItem->GetEnd(), 0 );
            const VECTOR2I  nearestPoint = arc.NearestPoint( cursorPos );

            // Move the end of the arc to the break point..
            graphicItem->SetEnd( nearestPoint );

            // and add another one starting from the break point
            auto newArc = std::make_unique<PCB_SHAPE>( *graphicItem );

            newArc->ClearSelected();
            newArc->SetEnd( arc.GetP1() );
            newArc->SetStart( nearestPoint );

            commit.Add( newArc.release() );
            commit.Push( _( "Split Arc" ) );
            break;
        }
        default:
            // No split implemented for other shapes
            break;
        }
    }

    updatePoints();
    return 0;
}


int PCB_POINT_EDITOR::removeCorner( const TOOL_EVENT& aEvent )
{
    if( !m_editPoints || !m_editedPoint )
        return 0;

    EDA_ITEM* item = m_editPoints->GetParent();

    if( !item )
        return 0;

    SHAPE_POLY_SET* polygon = nullptr;

    if( item->Type() == PCB_ZONE_T )
    {
        ZONE* zone = static_cast<ZONE*>( item );
        polygon = zone->Outline();
        zone->SetNeedRefill( true );
    }
    else if( item->Type() == PCB_SHAPE_T )
    {
        PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( item );

        if( shape->GetShape() == SHAPE_T::POLY )
            polygon = &shape->GetPolyShape();
    }

    if( !polygon )
        return 0;

    PCB_BASE_FRAME* frame = getEditFrame<PCB_BASE_FRAME>();
    BOARD_COMMIT commit( frame );
    auto vertex = findVertex( *polygon, *m_editedPoint );

    if( vertex.first )
    {
        const auto& vertexIdx = vertex.second;
        auto& outline = polygon->Polygon( vertexIdx.m_polygon )[vertexIdx.m_contour];

        if( outline.PointCount() > 3 )
        {
            // the usual case: remove just the corner when there are >3 vertices
            commit.Modify( item );
            polygon->RemoveVertex( vertexIdx );
        }
        else
        {
            // either remove a hole or the polygon when there are <= 3 corners
            if( vertexIdx.m_contour > 0 )
            {
                // remove hole
                commit.Modify( item );
                polygon->RemoveContour( vertexIdx.m_contour );
            }
            else
            {
                m_toolMgr->RunAction( PCB_ACTIONS::selectionClear );
                commit.Remove( item );
            }
        }

        setEditedPoint( nullptr );

        if( item->Type() == PCB_ZONE_T )
            commit.Push( _( "Remove Zone Corner" ) );
        else
            commit.Push( _( "Remove Polygon Corner" ) );

        // Refresh zone hatching
        if( item->Type() == PCB_ZONE_T )
            static_cast<ZONE*>( item )->HatchBorder();

        updatePoints();
    }

    return 0;
}


int PCB_POINT_EDITOR::chamferCorner( const TOOL_EVENT& aEvent )
{
    if( !m_editPoints || !m_editedPoint )
        return 0;

    EDA_ITEM* item = m_editPoints->GetParent();

    if( !item )
        return 0;

    SHAPE_POLY_SET* polygon = nullptr;

    if( item->Type() == PCB_ZONE_T )
    {
        ZONE* zone = static_cast<ZONE*>( item );
        polygon = zone->Outline();
        zone->SetNeedRefill( true );
    }
    else if( item->Type() == PCB_SHAPE_T )
    {
        PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( item );

        if( shape->GetShape() == SHAPE_T::POLY )
            polygon = &shape->GetPolyShape();
    }

    if( !polygon )
        return 0;

    // Search the best outline corner to break

    PCB_BASE_FRAME* frame = getEditFrame<PCB_BASE_FRAME>();
    BOARD_COMMIT    commit( frame );
    const VECTOR2I& cursorPos = getViewControls()->GetCursorPosition();

    unsigned int nearestIdx = 0;
    unsigned int nearestDist = INT_MAX;

    int curr_idx = 0;
    // Object to iterate through the corners of the outlines (main contour and its holes)
    SHAPE_POLY_SET::ITERATOR iterator = polygon->Iterate( 0, polygon->OutlineCount() - 1,
                                                          /* IterateHoles */ true );

    // Iterate through all the corners of the outlines and search the best segment
    for( ; iterator; iterator++, curr_idx++ )
    {
        unsigned int distance = polygon->CVertex( curr_idx ).Distance( cursorPos );

        if( distance < nearestDist )
        {
            nearestDist = distance;
            nearestIdx = curr_idx;
        }
    }

    int prevIdx, nextIdx;
    if( polygon->GetNeighbourIndexes( nearestIdx, &prevIdx, &nextIdx ) )
    {
        const SEG segA{ polygon->CVertex( prevIdx ), polygon->CVertex( nearestIdx ) };
        const SEG segB{ polygon->CVertex( nextIdx ), polygon->CVertex( nearestIdx ) };

        // A plausible setback that won't consume a whole edge
        int setback = pcbIUScale.mmToIU( 5 );
        setback = std::min( setback, (int) ( segA.Length() * 0.8 ) );
        setback = std::min( setback, (int) ( segB.Length() * 0.8 ) );

        CHAMFER_PARAMS chamferParams{ setback, setback };

        std::optional<CHAMFER_RESULT> chamferResult =
                ComputeChamferPoints( segA, segB, chamferParams );

        if( chamferResult && chamferResult->m_updated_seg_a && chamferResult->m_updated_seg_b )
        {
            commit.Modify( item );
            polygon->RemoveVertex( nearestIdx );

            // The two end points of the chamfer are the new corners
            polygon->InsertVertex( nearestIdx, chamferResult->m_updated_seg_b->B );
            polygon->InsertVertex( nearestIdx, chamferResult->m_updated_seg_a->B );
        }
    }

    setEditedPoint( nullptr );

    if( item->Type() == PCB_ZONE_T )
        commit.Push( _( "Break Zone Corner" ) );
    else
        commit.Push( _( "Break Polygon Corner" ) );

    // Refresh zone hatching
    if( item->Type() == PCB_ZONE_T )
        static_cast<ZONE*>( item )->HatchBorder();

    updatePoints();

    return 0;
}


int PCB_POINT_EDITOR::modifiedSelection( const TOOL_EVENT& aEvent )
{
    updatePoints();
    return 0;
}


int PCB_POINT_EDITOR::changeArcEditMode( const TOOL_EVENT& aEvent )
{
    PCB_BASE_EDIT_FRAME* editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();

    if( aEvent.Matches( ACTIONS::cycleArcEditMode.MakeEvent() ) )
    {
        if( editFrame->IsType( FRAME_PCB_EDITOR ) )
            m_arcEditMode = editFrame->GetPcbNewSettings()->m_ArcEditMode;
        else
            m_arcEditMode = editFrame->GetFootprintEditorSettings()->m_ArcEditMode;

        switch( m_arcEditMode )
        {
        case ARC_EDIT_MODE::KEEP_CENTER_ADJUST_ANGLE_RADIUS:
            m_arcEditMode = ARC_EDIT_MODE::KEEP_ENDPOINTS_OR_START_DIRECTION;
            break;
        case ARC_EDIT_MODE::KEEP_ENDPOINTS_OR_START_DIRECTION:
            m_arcEditMode = ARC_EDIT_MODE::KEEP_CENTER_ADJUST_ANGLE_RADIUS;
            break;
        }
    }
    else
    {
        m_arcEditMode = aEvent.Parameter<ARC_EDIT_MODE>();
    }

    if( editFrame->IsType( FRAME_PCB_EDITOR ) )
        editFrame->GetPcbNewSettings()->m_ArcEditMode = m_arcEditMode;
    else
        editFrame->GetFootprintEditorSettings()->m_ArcEditMode = m_arcEditMode;

    return 0;
}


void PCB_POINT_EDITOR::setTransitions()
{
    Go( &PCB_POINT_EDITOR::OnSelectionChange, ACTIONS::activatePointEditor.MakeEvent() );
    Go( &PCB_POINT_EDITOR::movePoint,         PCB_ACTIONS::pointEditorMoveCorner.MakeEvent() );
    Go( &PCB_POINT_EDITOR::movePoint,         PCB_ACTIONS::pointEditorMoveMidpoint.MakeEvent() );
    Go( &PCB_POINT_EDITOR::addCorner,         PCB_ACTIONS::pointEditorAddCorner.MakeEvent() );
    Go( &PCB_POINT_EDITOR::removeCorner,      PCB_ACTIONS::pointEditorRemoveCorner.MakeEvent() );
    Go( &PCB_POINT_EDITOR::chamferCorner,     PCB_ACTIONS::pointEditorChamferCorner.MakeEvent() );
    Go( &PCB_POINT_EDITOR::changeArcEditMode, PCB_ACTIONS::pointEditorArcKeepCenter.MakeEvent() );
    Go( &PCB_POINT_EDITOR::changeArcEditMode, PCB_ACTIONS::pointEditorArcKeepEndpoint.MakeEvent() );
    Go( &PCB_POINT_EDITOR::changeArcEditMode, ACTIONS::cycleArcEditMode.MakeEvent() );
    Go( &PCB_POINT_EDITOR::modifiedSelection, EVENTS::SelectedItemsModified );
    Go( &PCB_POINT_EDITOR::modifiedSelection, EVENTS::SelectedItemsMoved );
    Go( &PCB_POINT_EDITOR::OnSelectionChange, EVENTS::PointSelectedEvent );
    Go( &PCB_POINT_EDITOR::OnSelectionChange, EVENTS::SelectedEvent );
    Go( &PCB_POINT_EDITOR::OnSelectionChange, EVENTS::UnselectedEvent );
    Go( &PCB_POINT_EDITOR::OnSelectionChange, EVENTS::InhibitSelectionEditing );
    Go( &PCB_POINT_EDITOR::OnSelectionChange, EVENTS::UninhibitSelectionEditing );
}
