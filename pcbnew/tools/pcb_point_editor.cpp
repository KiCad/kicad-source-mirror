/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2021 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <algorithm>
#include <limits>

using namespace std::placeholders;
#include <advanced_config.h>
#include <kiplatform/ui.h>
#include <view/view_controls.h>
#include <gal/graphics_abstraction_layer.h>
#include <geometry/corner_operations.h>
#include <geometry/geometry_utils.h>
#include <geometry/seg.h>
#include <geometry/vector_utils.h>
#include <math/util.h>
#include <confirm.h>
#include <tool/tool_manager.h>
#include <tool/point_editor_behavior.h>
#include <tool/selection_conditions.h>
#include <preview_items/angle_item.h>
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
#include <pcb_group.h>
#include <pcb_dimension.h>
#include <pcb_barcode.h>
#include <pcb_textbox.h>
#include <pcb_tablecell.h>
#include <pcb_table.h>
#include <pad.h>
#include <zone.h>
#include <footprint.h>
#include <footprint_editor_settings.h>
#include <connectivity/connectivity_data.h>
#include <progress_reporter.h>
#include <layer_ids.h>
#include <preview_items/preview_utils.h>

const unsigned int PCB_POINT_EDITOR::COORDS_PADDING = pcbIUScale.mmToIU( 20 );

static void appendDirection( std::vector<VECTOR2I>& aDirections, const VECTOR2I& aDirection )
{
    if( aDirection.x != 0 || aDirection.y != 0 )
        aDirections.push_back( aDirection );
}

static std::vector<VECTOR2I> getConstraintDirections( EDIT_CONSTRAINT<EDIT_POINT>* aConstraint )
{
    std::vector<VECTOR2I> directions;

    if( !aConstraint )
        return directions;

    if( dynamic_cast<EC_90DEGREE*>( aConstraint ) )
    {
        appendDirection( directions, VECTOR2I( 1, 0 ) );
        appendDirection( directions, VECTOR2I( 0, 1 ) );
    }
    else if( dynamic_cast<EC_45DEGREE*>( aConstraint ) )
    {
        appendDirection( directions, VECTOR2I( 1, 0 ) );
        appendDirection( directions, VECTOR2I( 0, 1 ) );
        appendDirection( directions, VECTOR2I( 1, 1 ) );
        appendDirection( directions, VECTOR2I( 1, -1 ) );
    }
    else if( dynamic_cast<EC_VERTICAL*>( aConstraint ) )
    {
        appendDirection( directions, VECTOR2I( 0, 1 ) );
    }
    else if( dynamic_cast<EC_HORIZONTAL*>( aConstraint ) )
    {
        appendDirection( directions, VECTOR2I( 1, 0 ) );
    }
    else if( EC_LINE* lineConstraint = dynamic_cast<EC_LINE*>( aConstraint ) )
    {
        appendDirection( directions, lineConstraint->GetLineVector() );
    }

    return directions;
}

// Few constants to avoid using bare numbers for point indices
enum RECT_POINTS
{
    RECT_TOP_LEFT,
    RECT_TOP_RIGHT,
    RECT_BOT_RIGHT,
    RECT_BOT_LEFT,
    RECT_CENTER,
    RECT_RADIUS,

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


class RECT_RADIUS_TEXT_ITEM : public EDA_ITEM
{
public:
    RECT_RADIUS_TEXT_ITEM( const EDA_IU_SCALE& aIuScale, EDA_UNITS aUnits ) :
            EDA_ITEM( NOT_USED ),
            m_iuScale( aIuScale ),
            m_units( aUnits ),
            m_radius( 0 ),
            m_corner(),
            m_quadrant( -1, 1 ),
            m_visible( false )
    {
    }

    const BOX2I ViewBBox() const override
    {
        BOX2I tmp;
        tmp.SetMaximum();
        return tmp;
    }

    std::vector<int> ViewGetLayers() const override
    {
        return { LAYER_SELECT_OVERLAY, LAYER_GP_OVERLAY };
    }

    void ViewDraw( int aLayer, KIGFX::VIEW* aView ) const override
    {
        if( !m_visible )
            return;

        wxArrayString strings;
        strings.push_back( KIGFX::PREVIEW::DimensionLabel( "r", m_radius, m_iuScale, m_units ) );
        KIGFX::PREVIEW::DrawTextNextToCursor( aView, m_corner, m_quadrant, strings,
                                              aLayer == LAYER_SELECT_OVERLAY );
    }

    void Set( int aRadius, const VECTOR2I& aCorner, const VECTOR2I& aQuadrant, EDA_UNITS aUnits )
    {
        m_radius = aRadius;
        m_corner = aCorner;
        m_quadrant = aQuadrant;
        m_units = aUnits;
        m_visible = true;
    }

    void Hide()
    {
        m_visible = false;
    }

    wxString GetClass() const override
    {
        return wxT( "RECT_RADIUS_TEXT_ITEM" );
    }

private:
    const EDA_IU_SCALE& m_iuScale;
    EDA_UNITS           m_units;
    int                 m_radius;
    VECTOR2I            m_corner;
    VECTOR2I            m_quadrant;
    bool                m_visible;
};


class RECTANGLE_POINT_EDIT_BEHAVIOR : public POINT_EDIT_BEHAVIOR
{
public:
    RECTANGLE_POINT_EDIT_BEHAVIOR( PCB_SHAPE& aRectangle ) :
            m_rectangle( aRectangle )
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
        aPoints.AddPoint( VECTOR2I( botRight.x - aRectangle.GetCornerRadius(), topLeft.y ) );
        aPoints.Point( RECT_RADIUS ).SetDrawCircle();

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
            const VECTOR2I moveVector = aPoints.Point( RECT_CENTER ).GetPosition() - aRectangle.GetCenter();
            aRectangle.Move( moveVector );
        }
        else if( isModified( aEditedPoint, aPoints.Point( RECT_RADIUS ) ) )
        {
            int width = std::abs( botRight.x - topLeft.x );
            int height = std::abs( botRight.y - topLeft.y );
            int maxRadius = std::min( width, height ) / 2;
            int x = aPoints.Point( RECT_RADIUS ).GetX();
            x = std::clamp( x, botRight.x - maxRadius, botRight.x );
            aPoints.Point( RECT_RADIUS ).SetPosition( x, topLeft.y );
            aRectangle.SetCornerRadius( botRight.x - x );
        }
        else if( isModified( aEditedPoint, aPoints.Line( RECT_TOP ) ) )
        {
            // Only top changes; keep others from previous full-local bbox
            setTop( topLeft.y );
        }
        else if( isModified( aEditedPoint, aPoints.Line( RECT_LEFT ) ) )
        {
            // Only left changes; keep others from previous full-local bbox
            setLeft( topLeft.x );
        }
        else if( isModified( aEditedPoint, aPoints.Line( RECT_BOT ) ) )
        {
            // Only bottom changes; keep others from previous full-local bbox
            setBottom( botRight.y );
        }
        else if( isModified( aEditedPoint, aPoints.Line( RECT_RIGHT ) ) )
        {
            // Only right changes; keep others from previous full-local bbox
            setRight( botRight.x );
        }

        for( unsigned i = 0; i < aPoints.LinesSize(); ++i )
        {
            if( !isModified( aEditedPoint, aPoints.Line( i ) ) )
                aPoints.Line( i ).SetConstraint( new EC_PERPLINE( aPoints.Line( i ) ) );
        }
    }

    static void UpdatePoints( const PCB_SHAPE& aRectangle, EDIT_POINTS& aPoints )
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
        aPoints.Point( RECT_RADIUS ).SetPosition( botRight.x - aRectangle.GetCornerRadius(), topLeft.y );
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

    bool UpdatePoints( EDIT_POINTS& aPoints ) override
    {
        // Careful; rectangle shape is mutable between cardinal and non-cardinal rotations...
        if( m_rectangle.GetShape() != SHAPE_T::RECTANGLE || aPoints.PointsSize() == 0 )
            return false;

        UpdatePoints( m_rectangle, aPoints );
        return true;
    }

    void UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                     std::vector<EDA_ITEM*>& aUpdatedItems ) override
    {
        UpdateItem( m_rectangle, aEditedPoint, aPoints );
    }

    /**
     * Update the coordinates of 4 corners of a rectangle, according to constraints and the
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
                                VECTOR2I& aTopLeft, VECTOR2I& aTopRight, VECTOR2I& aBotLeft, VECTOR2I& aBotRight,
                                const VECTOR2I& aHole = { 0, 0 }, const VECTOR2I& aHoleSize = { 0, 0 } )
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


class ZONE_POINT_EDIT_BEHAVIOR : public POLYGON_POINT_EDIT_BEHAVIOR
{
public:
    ZONE_POINT_EDIT_BEHAVIOR( ZONE& aZone ) :
            POLYGON_POINT_EDIT_BEHAVIOR( *aZone.Outline() ),
            m_zone( aZone )
    {}

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
    REFERENCE_IMAGE_POINT_EDIT_BEHAVIOR( PCB_REFERENCE_IMAGE& aRefImage ) :
            m_refImage( aRefImage )
    {}

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

    bool UpdatePoints( EDIT_POINTS& aPoints ) override
    {
        wxCHECK( aPoints.PointsSize() == REFIMG_MAX_POINTS, false );

        REFERENCE_IMAGE& refImage = m_refImage.GetReferenceImage();

        const VECTOR2I topLeft = refImage.GetPosition() - refImage.GetSize() / 2;
        const VECTOR2I botRight = refImage.GetPosition() + refImage.GetSize() / 2;

        aPoints.Point( RECT_TOP_LEFT ).SetPosition( topLeft );
        aPoints.Point( RECT_TOP_RIGHT ).SetPosition( VECTOR2I( botRight.x, topLeft.y ) );
        aPoints.Point( RECT_BOT_RIGHT ).SetPosition( botRight );
        aPoints.Point( RECT_BOT_LEFT ).SetPosition( VECTOR2I( topLeft.x, botRight.y ) );
        aPoints.Point( REFIMG_ORIGIN ).SetPosition( refImage.GetPosition() + refImage.GetTransformOriginOffset() );
        return true;
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
            const VECTOR2I oldOrigin = m_refImage.GetPosition() + refImage.GetTransformOriginOffset();
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
                if( sign( newCorner->x ) != sign( oldCorner.x ) || sign( newCorner->y ) != sign( oldCorner.y ) )
                {
                    *newCorner = VECTOR2I( 0, 0 );
                }

                const double newLength = newCorner->EuclideanNorm();
                const double oldLength = oldCorner.EuclideanNorm();

                double ratio = oldLength > 0 ? ( newLength / oldLength ) : 1.0;

                // Clamp the scaling to a minimum of 50 mils
                VECTOR2I newSize = oldSize * ratio;
                double   newWidth = std::max( newSize.x, EDA_UNIT_UTILS::Mils2IU( pcbIUScale, 50 ) );
                double   newHeight = std::max( newSize.y, EDA_UNIT_UTILS::Mils2IU( pcbIUScale, 50 ) );
                ratio = std::min( newWidth / oldSize.x, newHeight / oldSize.y );

                // Also handles the origin offset
                refImage.SetImageScale( refImage.GetImageScale() * ratio );
            }
        }
    }

private:
    PCB_REFERENCE_IMAGE& m_refImage;
};


class BARCODE_POINT_EDIT_BEHAVIOR : public POINT_EDIT_BEHAVIOR
{
public:
    BARCODE_POINT_EDIT_BEHAVIOR( PCB_BARCODE& aBarcode ) :
            m_barcode( aBarcode )
    {}

    PCB_SHAPE makeDummyRect()
    {
        PCB_SHAPE dummy( nullptr, SHAPE_T::RECTANGLE );
        dummy.SetStart( m_barcode.GetCenter() - VECTOR2I( m_barcode.GetWidth() / 2, m_barcode.GetHeight() / 2 ) );
        dummy.SetEnd( dummy.GetStart() + VECTOR2I( m_barcode.GetWidth(), m_barcode.GetHeight() ) );
        dummy.Rotate( m_barcode.GetPosition(), m_barcode.GetAngle() );
        return dummy;
    }

    void MakePoints( EDIT_POINTS& aPoints ) override
    {
        if( !m_barcode.GetAngle().IsCardinal() )
        {
            // Non-cardinal barcode point-editing isn't useful enough to support.
            return;
        }

        auto set45Constraint =
                [&]( int a, int b )
                {
                    aPoints.Point( a ).SetConstraint( new EC_45DEGREE( aPoints.Point( a ), aPoints.Point( b ) ) );
                };

        RECTANGLE_POINT_EDIT_BEHAVIOR::MakePoints( makeDummyRect(), aPoints );

        if( m_barcode.KeepSquare() )
        {
            set45Constraint( RECT_TOP_LEFT, RECT_BOT_RIGHT );
            set45Constraint( RECT_TOP_RIGHT, RECT_BOT_LEFT );
            set45Constraint( RECT_BOT_RIGHT, RECT_TOP_LEFT );
            set45Constraint( RECT_BOT_LEFT, RECT_TOP_RIGHT );
        }
    }

    bool UpdatePoints( EDIT_POINTS& aPoints ) override
    {
        const unsigned target = m_barcode.GetAngle().IsCardinal() ? RECT_MAX_POINTS : 0;

        if( aPoints.PointsSize() != target )
            return false;

        RECTANGLE_POINT_EDIT_BEHAVIOR::UpdatePoints( makeDummyRect(), aPoints );
        return true;
    }

    void UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                     std::vector<EDA_ITEM*>& aUpdatedItems ) override
    {
        if( m_barcode.GetAngle().IsCardinal() )
        {
            PCB_SHAPE dummy = makeDummyRect();
            RECTANGLE_POINT_EDIT_BEHAVIOR::UpdateItem( dummy, aEditedPoint, aPoints );
            dummy.Rotate( dummy.GetCenter(), -m_barcode.GetAngle() );

            m_barcode.SetPosition( dummy.GetCenter() );
            m_barcode.SetWidth( dummy.GetRectangleWidth() );
            m_barcode.SetHeight( dummy.GetRectangleHeight() );
            m_barcode.AssembleBarcode();
        }
    }

private:
    PCB_BARCODE& m_barcode;
};


class PCB_TABLECELL_POINT_EDIT_BEHAVIOR : public EDA_TABLECELL_POINT_EDIT_BEHAVIOR
{
public:
    PCB_TABLECELL_POINT_EDIT_BEHAVIOR( PCB_TABLECELL& aCell ) :
            EDA_TABLECELL_POINT_EDIT_BEHAVIOR( aCell ),
            m_cell( aCell )
    {}

    void UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                     std::vector<EDA_ITEM*>& aUpdatedItems ) override
    {
        CHECK_POINT_COUNT( aPoints, TABLECELL_MAX_POINTS );

        PCB_TABLE& table = static_cast<PCB_TABLE&>( *m_cell.GetParent() );
        aCommit.Modify( &table );
        aUpdatedItems.push_back( &table );

        if( !m_cell.GetTextAngle().IsHorizontal() )
        {
            if( isModified( aEditedPoint, aPoints.Point( ROW_HEIGHT ) ) )
            {
                m_cell.SetEnd( VECTOR2I( m_cell.GetEndX(), aPoints.Point( ROW_HEIGHT ).GetY() ) );

                int colWidth = std::abs( m_cell.GetRectangleHeight() );

                for( int ii = 0; ii < m_cell.GetColSpan() - 1; ++ii )
                    colWidth -= table.GetColWidth( m_cell.GetColumn() + ii );

                table.SetColWidth( m_cell.GetColumn() + m_cell.GetColSpan() - 1, colWidth );
            }
            else if( isModified( aEditedPoint, aPoints.Point( COL_WIDTH ) ) )
            {
                m_cell.SetEnd( VECTOR2I( aPoints.Point( COL_WIDTH ).GetX(), m_cell.GetEndY() ) );

                int rowHeight = m_cell.GetRectangleWidth();

                for( int ii = 0; ii < m_cell.GetRowSpan() - 1; ++ii )
                    rowHeight -= table.GetRowHeight( m_cell.GetRow() + ii );

                table.SetRowHeight( m_cell.GetRow() + m_cell.GetRowSpan() - 1, rowHeight );
            }
        }
        else
        {
            if( isModified( aEditedPoint, aPoints.Point( COL_WIDTH ) ) )
            {
                m_cell.SetEnd( VECTOR2I( aPoints.Point( COL_WIDTH ).GetX(), m_cell.GetEndY() ) );

                int colWidth = m_cell.GetRectangleWidth();

                for( int ii = 0; ii < m_cell.GetColSpan() - 1; ++ii )
                    colWidth -= table.GetColWidth( m_cell.GetColumn() + ii );

                table.SetColWidth( m_cell.GetColumn() + m_cell.GetColSpan() - 1, colWidth );
            }
            else if( isModified( aEditedPoint, aPoints.Point( ROW_HEIGHT ) ) )
            {
                m_cell.SetEnd( VECTOR2I( m_cell.GetEndX(), aPoints.Point( ROW_HEIGHT ).GetY() ) );

                int rowHeight = m_cell.GetRectangleHeight();

                for( int ii = 0; ii < m_cell.GetRowSpan() - 1; ++ii )
                    rowHeight -= table.GetRowHeight( m_cell.GetRow() + ii );

                table.SetRowHeight( m_cell.GetRow() + m_cell.GetRowSpan() - 1, rowHeight );
            }
        }

        table.Normalize();
    }

private:
    PCB_TABLECELL& m_cell;
};


class PAD_POINT_EDIT_BEHAVIOR : public POINT_EDIT_BEHAVIOR
{
public:
    PAD_POINT_EDIT_BEHAVIOR( PAD& aPad, PCB_LAYER_ID aLayer ) :
            m_pad( aPad ),
            m_layer( aLayer )
    {}

    void MakePoints( EDIT_POINTS& aPoints ) override
    {
        VECTOR2I shapePos = m_pad.ShapePos( m_layer );
        VECTOR2I halfSize( m_pad.GetSize( m_layer ).x / 2, m_pad.GetSize( m_layer ).y / 2 );

        if( m_pad.IsLocked() )
            return;

        switch( m_pad.GetShape( m_layer ) )
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

            if( m_pad.GetOrientation().IsVertical() )
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

    bool UpdatePoints( EDIT_POINTS& aPoints ) override
    {
        bool     locked = m_pad.GetParent() && m_pad.IsLocked();
        VECTOR2I shapePos = m_pad.ShapePos( m_layer );
        VECTOR2I halfSize( m_pad.GetSize( m_layer ).x / 2, m_pad.GetSize( m_layer ).y / 2 );

        switch( m_pad.GetShape( m_layer ) )
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
                if( m_pad.GetOrientation().IsVertical() )
                    std::swap( halfSize.x, halfSize.y );

                aPoints.Point( RECT_TOP_LEFT ).SetPosition( shapePos - halfSize );
                aPoints.Point( RECT_TOP_RIGHT ).SetPosition( VECTOR2I( shapePos.x + halfSize.x,
                                                                       shapePos.y - halfSize.y ) );
                aPoints.Point( RECT_BOT_RIGHT ).SetPosition( shapePos + halfSize );
                aPoints.Point( RECT_BOT_LEFT ).SetPosition( VECTOR2I( shapePos.x - halfSize.x,
                                                                      shapePos.y + halfSize.y ) );
            }

            break;
        }

        default: // suppress warnings
            break;
        }

        return true;
    }

    void UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                     std::vector<EDA_ITEM*>& aUpdatedItems ) override
    {
        switch( m_pad.GetShape( m_layer ) )
        {
        case PAD_SHAPE::CIRCLE:
        {
            VECTOR2I end = aPoints.Point( 0 ).GetPosition();
            int      diameter = 2 * ( end - m_pad.GetPosition() ).EuclideanNorm();

            m_pad.SetSize( m_layer, VECTOR2I( diameter, diameter ) );
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

            RECTANGLE_POINT_EDIT_BEHAVIOR::PinEditedCorner( aEditedPoint, aPoints, topLeft, topRight,
                                                            botLeft, botRight, holeCenter, holeSize );

            if( ( m_pad.GetOffset( m_layer ).x || m_pad.GetOffset( m_layer ).y )
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

                if( m_pad.GetOrientation().IsVertical() )
                    std::swap( padSize.x, padSize.y );

                RotatePoint( deltaOffset, -m_pad.GetOrientation() );

                m_pad.SetSize( m_layer, padSize );
                m_pad.SetOffset( m_layer, -deltaOffset );
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

                if( m_pad.GetOrientation().IsVertical() )
                    std::swap( padSize.x, padSize.y );

                m_pad.SetSize( m_layer, padSize );
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
    PCB_LAYER_ID m_layer;
};


/**
 * Point editor behavior for the PCB_GENERATOR class.
 *
 * This just delegates to the PCB_GENERATOR's own methods.
 */
class GENERATOR_POINT_EDIT_BEHAVIOR : public POINT_EDIT_BEHAVIOR
{
public:
    GENERATOR_POINT_EDIT_BEHAVIOR( PCB_GENERATOR& aGenerator ) :
            m_generator( aGenerator )
    {}

    void MakePoints( EDIT_POINTS& aPoints ) override
    {
        m_generator.MakeEditPoints( aPoints );
    }

    bool UpdatePoints( EDIT_POINTS& aPoints ) override
    {
        m_generator.UpdateEditPoints( aPoints );
        return true;
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
            m_dimension( aDimension ),
            m_originalTextPos( aDimension.GetTextPos() ),
            m_oldCrossBar( SEG{ aDimension.GetCrossbarStart(), aDimension.GetCrossbarEnd() } )
    {}

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
                m_dimension.SetHorizJustify( ( oldJustify == GR_TEXT_H_ALIGN_T::GR_TEXT_H_ALIGN_LEFT )
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
            const VECTOR2I cbNearestEndToText = KIGEOM::GetNearestEndpoint( m_oldCrossBar, m_originalTextPos );
            const VECTOR2I rotTextOffsetFromCbCenter = GetRotated( m_originalTextPos - m_oldCrossBar.Center(),
                                                                   rotation );
            const VECTOR2I rotTextOffsetFromCbEnd = GetRotated( m_originalTextPos - cbNearestEndToText, rotation );

            // Which of the two crossbar points is now in the right direction? They could be swapped over now.
            // If zero-length, doesn't matter, they're the same thing
            const bool startIsInOffsetDirection = KIGEOM::PointIsInDirection( m_dimension.GetCrossbarStart(),
                                                                              rotTextOffsetFromCbCenter,
                                                                              newCrossBar.Center() );

            const VECTOR2I& newCbRefPt = startIsInOffsetDirection ? m_dimension.GetCrossbarStart()
                                                                  : m_dimension.GetCrossbarEnd();

            // Apply the new offset to the correct crossbar point
            return newCbRefPt + rotTextOffsetFromCbEnd;
        }

        // If the text was between the crossbar points, it should stay there, but we need to find a
        // good place for it. Keep it the same distance from the crossbar line, but rotated as needed.

        const VECTOR2I origTextPointProjected = m_oldCrossBar.NearestPoint( m_originalTextPos );
        const double   oldRatio = KIGEOM::GetLengthRatioFromStart( origTextPointProjected, m_oldCrossBar );

        // Perpendicular from the crossbar line to the text position
        // We need to keep this length constant
        const VECTOR2I rotCbNormalToText = GetRotated( m_originalTextPos - origTextPointProjected, rotation );

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
    ALIGNED_DIMENSION_POINT_EDIT_BEHAVIOR( PCB_DIM_ALIGNED& aDimension ) :
            m_dimension( aDimension )
    {}

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
            aPoints.Point( DIM_CROSSBARSTART ).SetConstraint( new EC_LINE( aPoints.Point( DIM_CROSSBARSTART ),
                                                                           aPoints.Point( DIM_START ) ) );
            aPoints.Point( DIM_CROSSBAREND ).SetConstraint( new EC_LINE( aPoints.Point( DIM_CROSSBAREND ),
                                                                         aPoints.Point( DIM_END ) ) );
        }
    }

    bool UpdatePoints( EDIT_POINTS& aPoints ) override
    {
        wxCHECK( aPoints.PointsSize() == DIM_ALIGNED_MAX, false );

        aPoints.Point( DIM_START ).SetPosition( m_dimension.GetStart() );
        aPoints.Point( DIM_END ).SetPosition( m_dimension.GetEnd() );
        aPoints.Point( DIM_TEXT ).SetPosition( m_dimension.GetTextPos() );
        aPoints.Point( DIM_CROSSBARSTART ).SetPosition( m_dimension.GetCrossbarStart() );
        aPoints.Point( DIM_CROSSBAREND ).SetPosition( m_dimension.GetCrossbarEnd() );
        return true;
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

    OPT_VECTOR2I Get45DegreeConstrainer( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints ) const override
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

            aPoints.Point( DIM_CROSSBARSTART ).SetConstraint( new EC_LINE( aPoints.Point( DIM_CROSSBARSTART ),
                                                                           aPoints.Point( DIM_START ) ) );
            aPoints.Point( DIM_CROSSBAREND ).SetConstraint( new EC_LINE( aPoints.Point( DIM_CROSSBAREND ),
                                                                         aPoints.Point( DIM_END ) ) );
        }
        else if( isModified( aEditedPoint, aPoints.Point( DIM_END ) ) )
        {
            m_dimension.SetEnd( aEditedPoint.GetPosition() );
            m_dimension.Update();

            aPoints.Point( DIM_CROSSBARSTART ).SetConstraint( new EC_LINE( aPoints.Point( DIM_CROSSBARSTART ),
                                                                           aPoints.Point( DIM_START ) ) );
            aPoints.Point( DIM_CROSSBAREND ).SetConstraint( new EC_LINE( aPoints.Point( DIM_CROSSBAREND ),
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
    DIM_CENTER_POINT_EDIT_BEHAVIOR( PCB_DIM_CENTER& aDimension ) :
            m_dimension( aDimension )
    {}

    void MakePoints( EDIT_POINTS& aPoints ) override
    {
        aPoints.AddPoint( m_dimension.GetStart() );
        aPoints.AddPoint( m_dimension.GetEnd() );

        aPoints.Point( DIM_START ).SetSnapConstraint( ALL_LAYERS );

        aPoints.Point( DIM_END ).SetConstraint(new EC_45DEGREE( aPoints.Point( DIM_END ),
                                                                 aPoints.Point( DIM_START ) ) );
        aPoints.Point( DIM_END ).SetSnapConstraint( IGNORE_SNAPS );
    }

    bool UpdatePoints( EDIT_POINTS& aPoints ) override
    {
        wxCHECK( aPoints.PointsSize() == DIM_CENTER_MAX, false );

        aPoints.Point( DIM_START ).SetPosition( m_dimension.GetStart() );
        aPoints.Point( DIM_END ).SetPosition( m_dimension.GetEnd() );
        return true;
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

    OPT_VECTOR2I Get45DegreeConstrainer( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints ) const override
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
    DIM_RADIAL_POINT_EDIT_BEHAVIOR( PCB_DIM_RADIAL& aDimension ) :
            m_dimension( aDimension )
    {}

    void MakePoints( EDIT_POINTS& aPoints ) override
    {
        aPoints.AddPoint( m_dimension.GetStart() );
        aPoints.AddPoint( m_dimension.GetEnd() );
        aPoints.AddPoint( m_dimension.GetTextPos() );
        aPoints.AddPoint( m_dimension.GetKnee() );

        aPoints.Point( DIM_START ).SetSnapConstraint( ALL_LAYERS );
        aPoints.Point( DIM_END ).SetSnapConstraint( ALL_LAYERS );

        aPoints.Point( DIM_KNEE ).SetConstraint( new EC_LINE( aPoints.Point( DIM_START ),
                                                              aPoints.Point( DIM_END ) ) );
        aPoints.Point( DIM_KNEE ).SetSnapConstraint( IGNORE_SNAPS );

        aPoints.Point( DIM_TEXT ).SetConstraint( new EC_45DEGREE( aPoints.Point( DIM_TEXT ),
                                                                  aPoints.Point( DIM_KNEE ) ) );
        aPoints.Point( DIM_TEXT ).SetSnapConstraint( IGNORE_SNAPS );
    }

    bool UpdatePoints( EDIT_POINTS& aPoints ) override
    {
        wxCHECK( aPoints.PointsSize() == DIM_RADIAL_MAX, false );

        aPoints.Point( DIM_START ).SetPosition( m_dimension.GetStart() );
        aPoints.Point( DIM_END ).SetPosition( m_dimension.GetEnd() );
        aPoints.Point( DIM_TEXT ).SetPosition( m_dimension.GetTextPos() );
        aPoints.Point( DIM_KNEE ).SetPosition( m_dimension.GetKnee() );
        return true;
    }

    void UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                     std::vector<EDA_ITEM*>& aUpdatedItems ) override
    {
        CHECK_POINT_COUNT( aPoints, DIM_RADIAL_MAX );

        if( isModified( aEditedPoint, aPoints.Point( DIM_START ) ) )
        {
            m_dimension.SetStart( aEditedPoint.GetPosition() );
            m_dimension.Update();

            aPoints.Point( DIM_KNEE ).SetConstraint( new EC_LINE( aPoints.Point( DIM_START ),
                                                                  aPoints.Point( DIM_END ) ) );
        }
        else if( isModified( aEditedPoint, aPoints.Point( DIM_END ) ) )
        {
            VECTOR2I oldKnee = m_dimension.GetKnee();

            m_dimension.SetEnd( aEditedPoint.GetPosition() );
            m_dimension.Update();

            VECTOR2I kneeDelta = m_dimension.GetKnee() - oldKnee;
            m_dimension.SetTextPos( m_dimension.GetTextPos() + kneeDelta );
            m_dimension.Update();

            aPoints.Point( DIM_KNEE ).SetConstraint( new EC_LINE( aPoints.Point( DIM_START ),
                                                                  aPoints.Point( DIM_END ) ) );
        }
        else if( isModified( aEditedPoint, aPoints.Point( DIM_KNEE ) ) )
        {
            VECTOR2I oldKnee = m_dimension.GetKnee();
            VECTOR2I arrowVec = aPoints.Point( DIM_KNEE ).GetPosition() - aPoints.Point( DIM_END ).GetPosition();

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

    OPT_VECTOR2I Get45DegreeConstrainer( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints ) const override
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
    DIM_LEADER_POINT_EDIT_BEHAVIOR( PCB_DIM_LEADER& aDimension ) :
            m_dimension( aDimension )
    {}

    void MakePoints( EDIT_POINTS& aPoints ) override
    {
        aPoints.AddPoint( m_dimension.GetStart() );
        aPoints.AddPoint( m_dimension.GetEnd() );
        aPoints.AddPoint( m_dimension.GetTextPos() );

        aPoints.Point( DIM_START ).SetSnapConstraint( ALL_LAYERS );
        aPoints.Point( DIM_END ).SetSnapConstraint( ALL_LAYERS );

        aPoints.Point( DIM_TEXT ).SetConstraint( new EC_45DEGREE( aPoints.Point( DIM_TEXT ),
                                                                  aPoints.Point( DIM_END ) ) );
        aPoints.Point( DIM_TEXT ).SetSnapConstraint( IGNORE_SNAPS );
    }

    bool UpdatePoints( EDIT_POINTS& aPoints ) override
    {
        wxCHECK( aPoints.PointsSize() == DIM_LEADER_MAX, false );

        aPoints.Point( DIM_START ).SetPosition( m_dimension.GetStart() );
        aPoints.Point( DIM_END ).SetPosition( m_dimension.GetEnd() );
        aPoints.Point( DIM_TEXT ).SetPosition( m_dimension.GetTextPos() );
        return true;
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
    TEXTBOX_POINT_EDIT_BEHAVIOR( PCB_TEXTBOX& aTextbox ) :
            m_textbox( aTextbox )
    {}

    void MakePoints( EDIT_POINTS& aPoints ) override
    {
        if( m_textbox.GetShape() == SHAPE_T::RECTANGLE )
            RECTANGLE_POINT_EDIT_BEHAVIOR::MakePoints( m_textbox, aPoints );

        // Rotated textboxes are implemented as polygons and these aren't currently editable.
    }

    bool UpdatePoints( EDIT_POINTS& aPoints ) override
    {
        // Careful; textbox shape is mutable between cardinal and non-cardinal rotations...
        const unsigned target = m_textbox.GetShape() == SHAPE_T::RECTANGLE ? RECT_MAX_POINTS : 0;

        if( aPoints.PointsSize() != target )
            return false;

        RECTANGLE_POINT_EDIT_BEHAVIOR::UpdatePoints( m_textbox, aPoints );
        return true;
    }

    void UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                     std::vector<EDA_ITEM*>& aUpdatedItems ) override
    {
        if( m_textbox.GetShape() == SHAPE_T::RECTANGLE )
            RECTANGLE_POINT_EDIT_BEHAVIOR::UpdateItem( m_textbox, aEditedPoint, aPoints );
    }

private:
    PCB_TEXTBOX& m_textbox;
};

class SHAPE_GROUP_POINT_EDIT_BEHAVIOR : public POINT_EDIT_BEHAVIOR
{
public:
    SHAPE_GROUP_POINT_EDIT_BEHAVIOR( PCB_GROUP& aGroup ) :
            m_group( &aGroup ),
            m_parent( &aGroup )
    {
        for( BOARD_ITEM* item : aGroup.GetBoardItems() )
        {
            if( item->Type() == PCB_SHAPE_T )
            {
                PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( item );
                m_shapes.push_back( shape );
                m_originalWidths[shape] = static_cast<double>( shape->GetWidth() );
            }
        }
    }

    SHAPE_GROUP_POINT_EDIT_BEHAVIOR( std::vector<PCB_SHAPE*> aShapes, BOARD_ITEM* aParent ) :
            m_group( nullptr ),
            m_shapes( std::move( aShapes ) ),
            m_parent( aParent )
    {
        for( PCB_SHAPE* shape : m_shapes )
            m_originalWidths[shape] = static_cast<double>( shape->GetWidth() );
    }

    void MakePoints( EDIT_POINTS& aPoints ) override
    {
        BOX2I bbox = getBoundingBox();
        VECTOR2I tl = bbox.GetOrigin();
        VECTOR2I br = bbox.GetEnd();

        aPoints.AddPoint( tl );
        aPoints.AddPoint( VECTOR2I( br.x, tl.y ) );
        aPoints.AddPoint( br );
        aPoints.AddPoint( VECTOR2I( tl.x, br.y ) );
        aPoints.AddPoint( bbox.Centre() );

        aPoints.AddIndicatorLine( aPoints.Point( RECT_TOP_LEFT ), aPoints.Point( RECT_TOP_RIGHT ) );
        aPoints.AddIndicatorLine( aPoints.Point( RECT_TOP_RIGHT ), aPoints.Point( RECT_BOT_RIGHT ) );
        aPoints.AddIndicatorLine( aPoints.Point( RECT_BOT_RIGHT ), aPoints.Point( RECT_BOT_LEFT ) );
        aPoints.AddIndicatorLine( aPoints.Point( RECT_BOT_LEFT ), aPoints.Point( RECT_TOP_LEFT ) );
    }

    bool UpdatePoints( EDIT_POINTS& aPoints ) override
    {
        BOX2I bbox = getBoundingBox();
        VECTOR2I tl = bbox.GetOrigin();
        VECTOR2I br = bbox.GetEnd();

        aPoints.Point( RECT_TOP_LEFT ).SetPosition( tl );
        aPoints.Point( RECT_TOP_RIGHT ).SetPosition( br.x, tl.y );
        aPoints.Point( RECT_BOT_RIGHT ).SetPosition( br );
        aPoints.Point( RECT_BOT_LEFT ).SetPosition( tl.x, br.y );
        aPoints.Point( RECT_CENTER ).SetPosition( bbox.Centre() );
        return true;
    }

    void UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                     std::vector<EDA_ITEM*>& aUpdatedItems ) override
    {
        BOX2I oldBox = getBoundingBox();
        VECTOR2I oldCenter = oldBox.Centre();

        if( isModified( aEditedPoint, aPoints.Point( RECT_CENTER ) ) )
        {
            VECTOR2I delta = aPoints.Point( RECT_CENTER ).GetPosition() - oldCenter;

            if( m_group )
            {
                aCommit.Modify( m_group, nullptr, RECURSE_MODE::RECURSE );
                m_group->Move( delta );
            }
            else
            {
                for( PCB_SHAPE* shape : m_shapes )
                {
                    aCommit.Modify( shape );
                    shape->Move( delta );
                }
            }

            for( PCB_SHAPE* shape : m_shapes )
                aUpdatedItems.push_back( shape );

            UpdatePoints( aPoints );
            return;
        }

        VECTOR2I tl = aPoints.Point( RECT_TOP_LEFT ).GetPosition();
        VECTOR2I tr = aPoints.Point( RECT_TOP_RIGHT ).GetPosition();
        VECTOR2I bl = aPoints.Point( RECT_BOT_LEFT ).GetPosition();
        VECTOR2I br = aPoints.Point( RECT_BOT_RIGHT ).GetPosition();

        RECTANGLE_POINT_EDIT_BEHAVIOR::PinEditedCorner( aEditedPoint, aPoints, tl, tr, bl, br );

        double sx = static_cast<double>( br.x - tl.x ) / static_cast<double>( oldBox.GetWidth() );
        double sy = static_cast<double>( br.y - tl.y ) / static_cast<double>( oldBox.GetHeight() );
        double scale = ( sx + sy ) / 2.0;

        for( PCB_SHAPE* shape : m_shapes )
        {
            aCommit.Modify( shape );
            shape->Move( -oldCenter );
            shape->Scale( scale );
            shape->Move( oldCenter );

            if( auto shapeIt = m_originalWidths.find( shape ); shapeIt != m_originalWidths.end() )
            {
                shapeIt->second = shapeIt->second * scale;
                shape->SetWidth( KiROUND( shapeIt->second ) );
            }
            else
            {
                shape->SetWidth( KiROUND( shape->GetWidth() * scale ) );
            }

            aUpdatedItems.push_back( shape );
        }

        UpdatePoints( aPoints );
    }

    BOARD_ITEM* GetParent() const { return m_parent; }

private:
    BOX2I getBoundingBox() const
    {
        BOX2I bbox;

        for( const PCB_SHAPE* shape : m_shapes )
            bbox.Merge( shape->GetBoundingBox() );

        return bbox;
    }

private:
    PCB_GROUP*                             m_group;
    std::vector<PCB_SHAPE*>                m_shapes;
    BOARD_ITEM*                            m_parent;
    std::unordered_map<PCB_SHAPE*, double> m_originalWidths;
};


PCB_POINT_EDITOR::PCB_POINT_EDITOR() :
        PCB_TOOL_BASE( "pcbnew.PointEditor" ),
        m_frame( nullptr ),
        m_selectionTool( nullptr ),
        m_editedPoint( nullptr ),
        m_hoveredPoint( nullptr ),
        m_original( VECTOR2I( 0, 0 ) ),
        m_arcEditMode( ARC_EDIT_MODE::KEEP_CENTER_ADJUST_ANGLE_RADIUS ),
        m_radiusHelper( nullptr ),
        m_altConstrainer( VECTOR2I( 0, 0 ) ),
        m_inPointEditorTool( false ),
        m_angleSnapPos( VECTOR2I( 0, 0 ) ),
        m_stickyDisplacement( VECTOR2I( 0, 0 ) ),
        m_angleSnapActive( false )
{}


void PCB_POINT_EDITOR::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<PCB_BASE_FRAME>();

    if( KIGFX::VIEW* view = getView() )
    {
        if( m_angleItem && view->HasItem( m_angleItem.get() ) )
            view->Remove( m_angleItem.get() );

        if( m_editPoints && view->HasItem( m_editPoints.get() ) )
            view->Remove( m_editPoints.get() );

        if( view->HasItem( &m_preview ) )
            view->Remove( &m_preview );
    }

    m_angleItem.reset();
    m_editPoints.reset();
    m_altConstraint.reset();
    getViewControls()->SetAutoPan( false );
    m_angleSnapActive = false;
    m_stickyDisplacement = VECTOR2I( 0, 0 );
}


bool PCB_POINT_EDITOR::CanAddCorner( const EDA_ITEM& aItem )
{
    const KICAD_T type = aItem.Type();

    if( type == PCB_ZONE_T )
        return true;

    if( type == PCB_SHAPE_T )
    {
        const PCB_SHAPE& shape = static_cast<const PCB_SHAPE&>( aItem );
        const SHAPE_T    shapeType = shape.GetShape();
        return shapeType == SHAPE_T::SEGMENT || shapeType == SHAPE_T::POLY || shapeType == SHAPE_T::ARC;
    }

    return false;
}


bool PCB_POINT_EDITOR::CanChamferCorner( const EDA_ITEM& aItem )
{
    const auto type = aItem.Type();

    if( type == PCB_ZONE_T )
        return true;

    if( type == PCB_SHAPE_T )
    {
        const PCB_SHAPE& shape = static_cast<const PCB_SHAPE&>( aItem );
        const SHAPE_T    shapeType = shape.GetShape();
        return shapeType == SHAPE_T::POLY;
    }

    return false;
}


static VECTOR2I snapCorner( const VECTOR2I& aPrev, const VECTOR2I& aNext, const VECTOR2I& aGuess,
                            double aAngleDeg )
{
    double angleRad = aAngleDeg * M_PI / 180.0;
    VECTOR2D prev( aPrev );
    VECTOR2D next( aNext );
    double chord = ( next - prev ).EuclideanNorm();
    double sinA = sin( angleRad );

    if( chord == 0.0 || fabs( sinA ) < 1e-9 )
        return aGuess;

    double     radius = chord / ( 2.0 * sinA );
    VECTOR2D   mid = ( prev + next ) / 2.0;
    VECTOR2D   dir = next - prev;
    VECTOR2D   normal( -dir.y, dir.x );
    normal = normal.Resize( 1 );
    double h_sq = radius * radius - ( chord * chord ) / 4.0;
    double h = h_sq > 0.0 ? sqrt( h_sq ) : 0.0;

    VECTOR2D center1 = mid + normal * h;
    VECTOR2D center2 = mid - normal * h;

    auto project =
            [&]( const VECTOR2D& center )
            {
                VECTOR2D v = VECTOR2D( aGuess ) - center;

                if( v.EuclideanNorm() == 0.0 )
                    v = prev - center;

                v = v.Resize( 1 );
                VECTOR2D p = center + v * radius;
                return KiROUND( p );
            };

    VECTOR2I p1 = project( center1 );
    VECTOR2I p2 = project( center2 );

    double d1 = ( VECTOR2D( aGuess ) - VECTOR2D( p1 ) ).EuclideanNorm();
    double d2 = ( VECTOR2D( aGuess ) - VECTOR2D( p2 ) ).EuclideanNorm();

    return d1 < d2 ? p1 : p2;
}


bool PCB_POINT_EDITOR::Init()
{
    // Find the selection tool, so they can cooperate
    m_selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    wxASSERT_MSG( m_selectionTool, wxT( "pcbnew.InteractiveSelection tool is not available" ) );

    const auto arcIsEdited =
            []( const SELECTION& aSelection ) -> bool
            {
                const EDA_ITEM* item = aSelection.Front();
                return ( item != nullptr ) && ( item->Type() == PCB_SHAPE_T )
                       && static_cast<const PCB_SHAPE*>( item )->GetShape() == SHAPE_T::ARC;
            };

    using S_C = SELECTION_CONDITIONS;

    auto& menu = m_selectionTool->GetToolMenu().GetMenu();

    menu.AddItem( PCB_ACTIONS::cycleArcEditMode, S_C::Count( 1 ) && arcIsEdited );

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
    case PCB_BARCODE_T:
    {
        PCB_BARCODE& barcode = static_cast<PCB_BARCODE&>( *aItem );
        m_editorBehavior = std::make_unique<BARCODE_POINT_EDIT_BEHAVIOR>( barcode );
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
            m_editorBehavior = std::make_unique<EDA_ARC_POINT_EDIT_BEHAVIOR>( *shape, m_arcEditMode,
                                                                              *getViewControls() );
            break;

        case SHAPE_T::CIRCLE:
            m_editorBehavior = std::make_unique<EDA_CIRCLE_POINT_EDIT_BEHAVIOR>( *shape );
            break;

        case SHAPE_T::POLY:
            m_editorBehavior = std::make_unique<EDA_POLYGON_POINT_EDIT_BEHAVIOR>( *shape );
            break;

        case SHAPE_T::BEZIER:
            m_editorBehavior = std::make_unique<EDA_BEZIER_POINT_EDIT_BEHAVIOR>( *shape,
                                                                                 shape->GetMaxError() );
            break;

        default:        // suppress warnings
            break;
        }

        break;
    }

    case PCB_GROUP_T:
    {
        PCB_GROUP* group = static_cast<PCB_GROUP*>( aItem );
        bool shapesOnly = true;

        for( BOARD_ITEM* child : group->GetBoardItems() )
        {
            if( child->Type() != PCB_SHAPE_T )
            {
                shapesOnly = false;
                break;
            }
        }

        if( shapesOnly )
            m_editorBehavior = std::make_unique<SHAPE_GROUP_POINT_EDIT_BEHAVIOR>( *group );
        else
            points.reset();

        break;
    }

    case PCB_TABLECELL_T:
    {
        PCB_TABLECELL* cell = static_cast<PCB_TABLECELL*>( aItem );

        // No support for point-editing of a rotated table
        if( cell->GetShape() == SHAPE_T::RECTANGLE )
            m_editorBehavior = std::make_unique<PCB_TABLECELL_POINT_EDIT_BEHAVIOR>( *cell );

        break;
    }

    case PCB_PAD_T:
    {
        // Pad edit only for the footprint editor
        if( m_isFootprintEditor )
        {
            PAD& pad = static_cast<PAD&>( *aItem );
            PCB_LAYER_ID activeLayer = m_frame ? m_frame->GetActiveLayer() : PADSTACK::ALL_LAYERS;

            // Point editor only handles copper shape changes
            if( !IsCopperLayer( activeLayer ) )
                activeLayer = IsFrontLayer( activeLayer ) ? F_Cu : B_Cu;

            m_editorBehavior = std::make_unique<PAD_POINT_EDIT_BEHAVIOR>( pad, activeLayer );
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

    if( selection.Size() == 0 )
        return 0;

    for( EDA_ITEM* selItem : selection )
    {
        if( selItem->GetEditFlags() || !selItem->IsBOARD_ITEM() )
            return 0;
    }

    BOARD_ITEM* item = static_cast<BOARD_ITEM*>( selection.Front() );

    if( !item || item->IsLocked() )
        return 0;

    Activate();
    // Must be done after Activate() so that it gets set into the correct context
    getViewControls()->ShowCursor( true );

    PCB_GRID_HELPER grid( m_toolMgr, editFrame->GetMagneticItemsSettings() );

    // Use the original object as a construction item
    std::vector<std::unique_ptr<BOARD_ITEM>> clones;

    m_editorBehavior.reset();

    if( selection.Size() > 1 )
    {
        // Multi-selection: check if all items are shapes
        std::vector<PCB_SHAPE*> shapes;
        bool allShapes = true;
        bool anyLocked = false;

        for( EDA_ITEM* selItem : selection )
        {
            if( selItem->Type() == PCB_SHAPE_T )
            {
                PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( selItem );
                shapes.push_back( shape );

                if( shape->IsLocked() )
                    anyLocked = true;
            }
            else
            {
                allShapes = false;
            }
        }

        if( allShapes && shapes.size() > 1 && !anyLocked )
        {
            m_editorBehavior = std::make_unique<SHAPE_GROUP_POINT_EDIT_BEHAVIOR>(
                    std::move( shapes ), item );
            m_editPoints = std::make_shared<EDIT_POINTS>( item );
            m_editorBehavior->MakePoints( *m_editPoints );
        }
        else
        {
            return 0;
        }
    }
    else
    {
        // Single selection: use existing makePoints logic
        m_editPoints = makePoints( item );
    }

    if( !m_editPoints )
        return 0;

    PCB_SHAPE* graphicItem = dynamic_cast<PCB_SHAPE*>( item );

    // Only add the angle_item if we are editing a polygon or zone
    if( item->Type() == PCB_ZONE_T || ( graphicItem && graphicItem->GetShape() == SHAPE_T::POLY ) )
    {
        m_angleItem = std::make_unique<KIGFX::PREVIEW::ANGLE_ITEM>( m_editPoints );
    }

    m_preview.FreeItems();
    m_radiusHelper = nullptr;
    getView()->Add( &m_preview );

    m_radiusHelper = new RECT_RADIUS_TEXT_ITEM( pcbIUScale, editFrame->GetUserUnits() );
    m_preview.Add( m_radiusHelper );

    getView()->Add( m_editPoints.get() );

    if( m_angleItem )
        getView()->Add( m_angleItem.get() );

    setEditedPoint( nullptr );
    updateEditedPoint( aEvent );
    bool inDrag = false;
    bool isConstrained = false;
    bool haveSnapLineDirections = false;

    auto updateSnapLineDirections =
            [&]()
            {
                std::vector<VECTOR2I> directions;

                if( inDrag && m_editedPoint )
                {
                    EDIT_CONSTRAINT<EDIT_POINT>* constraint = nullptr;

                    if( m_altConstraint )
                        constraint = m_altConstraint.get();
                    else if( m_editedPoint->IsConstrained() )
                        constraint = m_editedPoint->GetConstraint();

                    directions = getConstraintDirections( constraint );
                }

                if( directions.empty() )
                {
                    grid.SetSnapLineDirections( {} );
                    grid.SetSnapLineEnd( std::nullopt );
                    haveSnapLineDirections = false;
                }
                else
                {
                    grid.SetSnapLineDirections( directions );
                    grid.SetSnapLineOrigin( m_original.GetPosition() );
                    grid.SetSnapLineEnd( std::nullopt );
                    haveSnapLineDirections = true;
                }
            };

    BOARD_COMMIT commit( editFrame );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );

        if( editFrame->IsType( FRAME_PCB_EDITOR ) )
            m_arcEditMode = editFrame->GetPcbNewSettings()->m_ArcEditMode;
        else
            m_arcEditMode = editFrame->GetFootprintEditorSettings()->m_ArcEditMode;

        if( !m_editPoints || evt->IsSelectionEvent() || evt->Matches( EVENTS::InhibitSelectionEditing ) )
        {
            break;
        }

        EDIT_POINT* prevHover = m_hoveredPoint;

        if( !inDrag )
            updateEditedPoint( *evt );

        if( prevHover != m_hoveredPoint )
        {
            getView()->Update( m_editPoints.get() );

            if( m_angleItem )
                getView()->Update( m_angleItem.get() );
        }

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

                m_editedPoint->SetActive();

                for( size_t ii = 0; ii < m_editPoints->PointsSize(); ++ii )
                {
                    EDIT_POINT& point = m_editPoints->Point( ii );

                    if( &point != m_editedPoint )
                        point.SetActive( false );
                }

                // When we start dragging, create a clone of the item to use as the original
                // reference geometry (e.g. for intersections and extensions)
                BOARD_ITEM* clone = static_cast<BOARD_ITEM*>( item->Clone() );
                clone->SetParent( nullptr );

                if( PCB_SHAPE* shape= dynamic_cast<PCB_SHAPE*>( item ) )
                {
                    shape->SetFlags( IS_MOVING );
                    shape->UpdateHatching();

                    static_cast<PCB_SHAPE*>( clone )->SetFillMode( FILL_T::NO_FILL );
                }

                clones.emplace_back( clone );
                grid.AddConstructionItems( { clone }, false, true );

                updateSnapLineDirections();
            }

            bool need_constraint = Is45Limited() || Is90Limited();

            if( isConstrained != need_constraint )
            {
                setAltConstraint( need_constraint );
                isConstrained = need_constraint;
                updateSnapLineDirections();
            }

            // For polygon lines, Ctrl temporarily toggles between CONVERGING and FIXED_LENGTH modes
            EDIT_LINE* line = dynamic_cast<EDIT_LINE*>( m_editedPoint );
            bool       ctrlHeld = evt->Modifier( MD_CTRL );

            if( line )
            {
                bool isPoly = false;

                switch( item->Type() )
                {
                case PCB_ZONE_T:
                    isPoly = true;
                    break;

                case PCB_SHAPE_T:
                    isPoly = static_cast<PCB_SHAPE*>( item )->GetShape() == SHAPE_T::POLY;
                    break;

                default:
                    break;
                }

                if( isPoly )
                {
                    EC_CONVERGING* constraint =
                            dynamic_cast<EC_CONVERGING*>( line->GetConstraint() );

                    if( constraint )
                    {
                        POLYGON_LINE_MODE targetMode = ctrlHeld ? POLYGON_LINE_MODE::FIXED_LENGTH
                                                                : POLYGON_LINE_MODE::CONVERGING;

                        if( constraint->GetMode() != targetMode )
                            constraint->SetMode( targetMode );
                    }
                }
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
                    VECTOR2I gridPt = grid.BestSnapAnchor( pos, {}, grid.GetItemGrid( item ), { item } );

                    VECTOR2I last = m_editedPoint->GetPosition();
                    VECTOR2I delta = pos - last;
                    VECTOR2I deltaGrid = gridPt - grid.BestSnapAnchor( last, {}, grid.GetItemGrid( item ),
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

            if( m_angleSnapActive )
            {
                m_stickyDisplacement = evt->Position() - m_angleSnapPos;
                int stickyLimit = KiROUND( getView()->ToWorld( 5 ) );

                if( m_stickyDisplacement.EuclideanNorm() > stickyLimit || evt->Modifier( MD_SHIFT ) )
                {
                    m_angleSnapActive = false;
                }
                else
                {
                    pos = m_angleSnapPos;
                }
            }

            if( !m_angleSnapActive && m_editPoints->PointsSize() > 2 && !evt->Modifier( MD_SHIFT ) )
            {
                int idx = getEditedPointIndex();

                if( idx != wxNOT_FOUND )
                {
                    int prevIdx = ( idx + m_editPoints->PointsSize() - 1 ) % m_editPoints->PointsSize();
                    int nextIdx = ( idx + 1 ) % m_editPoints->PointsSize();
                    VECTOR2I prev = m_editPoints->Point( prevIdx ).GetPosition();
                    VECTOR2I next = m_editPoints->Point( nextIdx ).GetPosition();
                    SEG      segA( pos, prev );
                    SEG      segB( pos, next );
                    double   ang = segA.Angle( segB ).AsDegrees();
                    double   snapAng = 45.0 * std::round( ang / 45.0 );

                    if( std::abs( ang - snapAng ) < 2.0 )
                    {
                        m_angleSnapPos = snapCorner( prev, next, pos, snapAng );
                        m_angleSnapActive = true;
                        m_stickyDisplacement = evt->Position() - m_angleSnapPos;
                        pos = m_angleSnapPos;
                    }
                }
            }

            bool constraintSnapped = false;

            // Apply 45 degree or other constraints
            if( !m_angleSnapActive && m_altConstraint )
            {
                m_editedPoint->SetPosition( pos );
                m_altConstraint->Apply( grid );
                constraintSnapped = true;

                // For constrained lines (like zone edges), try to snap to nearby anchors
                // that lie on the constraint line
                if( grid.GetSnap() && !snapLayers.empty() )
                {
                    VECTOR2I constrainedPos = m_editedPoint->GetPosition();
                    VECTOR2I snapPos = grid.BestSnapAnchor( constrainedPos, snapLayers,
                                                            grid.GetItemGrid( item ), { item } );

                    if( snapPos != constrainedPos )
                    {
                        m_editedPoint->SetPosition( snapPos );
                        m_altConstraint->Apply( grid );
                        VECTOR2I projectedPos = m_editedPoint->GetPosition();
                        const int snapTolerance = KiROUND( getView()->ToWorld( 5 ) );

                        if( ( projectedPos - snapPos ).EuclideanNorm() > snapTolerance )
                            m_editedPoint->SetPosition( constrainedPos );
                    }
                }
            }
            else if( !m_angleSnapActive && m_editedPoint->IsConstrained() )
            {
                m_editedPoint->SetPosition( pos );
                m_editedPoint->ApplyConstraint( grid );
                constraintSnapped = true;

                // For constrained lines (like zone edges), try to snap to nearby anchors
                // that lie on the constraint line. First get the constrained position, then
                // look for snap anchors and verify they're on the constraint line.
                if( grid.GetSnap() && !snapLayers.empty() )
                {
                    VECTOR2I constrainedPos = m_editedPoint->GetPosition();
                    VECTOR2I snapPos = grid.BestSnapAnchor( constrainedPos, snapLayers,
                                                            grid.GetItemGrid( item ), { item } );

                    // If we found a snap anchor different from the constrained position,
                    // check if setting the point there and reapplying the constraint
                    // results in a position close to the snap point
                    if( snapPos != constrainedPos )
                    {
                        m_editedPoint->SetPosition( snapPos );
                        m_editedPoint->ApplyConstraint( grid );
                        VECTOR2I projectedPos = m_editedPoint->GetPosition();

                        // If the projection is close to the snap anchor, use it
                        // Otherwise revert to the original constrained position
                        const int snapTolerance = KiROUND( getView()->ToWorld( 5 ) );

                        if( ( projectedPos - snapPos ).EuclideanNorm() > snapTolerance )
                            m_editedPoint->SetPosition( constrainedPos );
                    }
                }
            }
            else if( !m_angleSnapActive && m_editedPoint->GetGridConstraint() == SNAP_TO_GRID )
            {
                m_editedPoint->SetPosition( grid.BestSnapAnchor( pos, snapLayers, grid.GetItemGrid( item ),
                                                                 { item } ) );
            }
            else
            {
                m_editedPoint->SetPosition( pos );
            }

            if( haveSnapLineDirections )
            {
                if( constraintSnapped )
                    grid.SetSnapLineEnd( m_editedPoint->GetPosition() );
                else
                    grid.SetSnapLineEnd( std::nullopt );
            }

            updateItem( commit );
            getViewControls()->ForceCursorPosition( true, m_editedPoint->GetPosition() );
            updatePoints();

            if( m_radiusHelper )
            {
                if( m_editPoints->PointsSize() > RECT_RADIUS
                    && m_editedPoint == &m_editPoints->Point( RECT_RADIUS ) )
                {
                    if( PCB_SHAPE* rect = dynamic_cast<PCB_SHAPE*>( item ) )
                    {
                        int radius = rect->GetCornerRadius();
                        int offset = radius - M_SQRT1_2 * radius;
                        VECTOR2I topLeft = rect->GetTopLeft();
                        VECTOR2I botRight = rect->GetBotRight();
                        VECTOR2I topRight( botRight.x, topLeft.y );
                        VECTOR2I center( topRight.x - offset, topRight.y + offset );
                        m_radiusHelper->Set( radius, center, VECTOR2I( 1, -1 ), editFrame->GetUserUnits() );
                    }
                }
                else
                {
                    m_radiusHelper->Hide();
                }
            }

            getView()->Update( &m_preview );
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

            if( m_angleItem )
                getView()->Update( m_angleItem.get() );
        }
        else if( inDrag && evt->IsMouseUp( BUT_LEFT ) )
        {
            if( m_editedPoint )
            {
                m_editedPoint->SetActive( false );
                getView()->Update( m_editPoints.get() );

                if( m_angleItem )
                    getView()->Update( m_angleItem.get() );
            }

            if( m_radiusHelper )
                m_radiusHelper->Hide();

            getView()->Update( &m_preview );

            getViewControls()->SetAutoPan( false );
            setAltConstraint( false );
            updateSnapLineDirections();

            if( m_editorBehavior )
                m_editorBehavior->FinalizeItem( *m_editPoints, commit );

            if( item->Type() == PCB_GENERATOR_T )
            {
                PCB_GENERATOR* generator = static_cast<PCB_GENERATOR*>( item );

                m_preview.FreeItems();
                m_radiusHelper = nullptr;
                m_toolMgr->RunSynchronousAction( PCB_ACTIONS::genFinishEdit, &commit, generator );

                commit.Push( generator->GetCommitMessage() );
            }
            else if( item->Type() == PCB_TABLECELL_T )
            {
                commit.Push( _( "Resize Table Cells" ) );
            }
            else
            {
                commit.Push( _( "Move Point" ) );
            }

            if( PCB_SHAPE* shape= dynamic_cast<PCB_SHAPE*>( item ) )
            {
                shape->ClearFlags( IS_MOVING );
                shape->UpdateHatching();
            }

            inDrag = false;
            frame()->UndoRedoBlock( false );
            updateSnapLineDirections();

            m_toolMgr->PostAction<EDA_ITEM*>( ACTIONS::reselectItem, item ); // FIXME: Needed for generators
        }
        else if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            if( inDrag )      // Restore the last change
            {
                if( item->Type() == PCB_GENERATOR_T )
                {
                    m_toolMgr->RunSynchronousAction( PCB_ACTIONS::genCancelEdit, &commit,
                                                     static_cast<PCB_GENERATOR*>( item ) );
                }

                commit.Revert();

                if( PCB_SHAPE* shape= dynamic_cast<PCB_SHAPE*>( item ) )
                {
                    shape->ClearFlags( IS_MOVING );
                    shape->UpdateHatching();
                }

                inDrag = false;
                frame()->UndoRedoBlock( false );
                updateSnapLineDirections();
            }

            // Only cancel point editor when activating a new tool
            // Otherwise, allow the points to persist when moving up the
            // tool stack
            if( evt->IsActivate() && !evt->IsMoveTool() )
                break;
        }
        else if( evt->IsAction( &PCB_ACTIONS::layerChanged ) )
        {
            // Re-create the points for items which can have different behavior on different layers
            if( item->Type() == PCB_PAD_T && m_isFootprintEditor )
            {
                if( getView()->HasItem( m_editPoints.get() ) )
                    getView()->Remove( m_editPoints.get() );

                if( m_angleItem && getView()->HasItem( m_angleItem.get() ) )
                    getView()->Remove( m_angleItem.get() );

                m_editPoints = makePoints( item );

                if( m_angleItem )
                {
                    m_angleItem->SetEditPoints( m_editPoints );
                    getView()->Add( m_angleItem.get() );
                }

                getView()->Add( m_editPoints.get() );
            }
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

    if( PCB_SHAPE* shape= dynamic_cast<PCB_SHAPE*>( item ) )
    {
        shape->ClearFlags( IS_MOVING );
        shape->UpdateHatching();
    }

    m_preview.FreeItems();
    m_radiusHelper = nullptr;

    if( getView()->HasItem( &m_preview ) )
        getView()->Remove( &m_preview );

    if( m_editPoints )
    {
        if( getView()->HasItem( m_editPoints.get() ) )
            getView()->Remove( m_editPoints.get() );

        if( m_angleItem && getView()->HasItem( m_angleItem.get() ) )
            getView()->Remove( m_angleItem.get() );

        m_editPoints.reset();
        m_angleItem.reset();
    }

    m_editedPoint = nullptr;
    grid.SetSnapLineDirections( {} );

    return 0;
}


int PCB_POINT_EDITOR::movePoint( const TOOL_EVENT& aEvent )
{
    if( !m_editPoints || !m_editPoints->GetParent() || !HasPoint() )
        return 0;

    PCB_BASE_EDIT_FRAME* editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();

    BOARD_COMMIT commit( editFrame );
    commit.Stage( m_editPoints->GetParent(), CHT_MODIFY );

    VECTOR2I pt = m_editedPoint->GetPosition();
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

    WX_PT_ENTRY_DIALOG dlg( editFrame, title, _( "X:" ), _( "Y:" ), pt, false );

    if( dlg.ShowModal() == wxID_OK )
    {
        m_editedPoint->SetPosition( dlg.GetValue() );
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
        m_radiusHelper = nullptr;

        for( EDA_ITEM* previewItem : generatorItem->GetPreviewItems( generatorTool, frame(), STATUS_ITEMS_ONLY ) )
            m_preview.Add( previewItem );

        getView()->Update( &m_preview );
        break;
    }
    default:
        break;
    }

    // Update the item and any affected items
    for( EDA_ITEM* updatedItem : updatedItems )
        getView()->Update( updatedItem );

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

    int editedIndex = -1;
    bool editingLine = false;

    if( m_editedPoint )
    {
        // Check if we're editing a point (vertex)
        for( unsigned ii = 0; ii < m_editPoints->PointsSize(); ++ii )
        {
            if( &m_editPoints->Point( ii ) == m_editedPoint )
            {
                editedIndex = ii;
                break;
            }
        }

        // If not found in points, check if we're editing a line (midpoint)
        if( editedIndex == -1 )
        {
            for( unsigned ii = 0; ii < m_editPoints->LinesSize(); ++ii )
            {
                if( &m_editPoints->Line( ii ) == m_editedPoint )
                {
                    editedIndex = ii;
                    editingLine = true;
                    break;
                }
            }
        }
    }

    if( !m_editorBehavior->UpdatePoints( *m_editPoints ) )
    {
        if( getView()->HasItem( m_editPoints.get() ) )
            getView()->Remove( m_editPoints.get() );

        m_editPoints = makePoints( item );
        getView()->Add( m_editPoints.get() );
    }

    if( editedIndex >= 0 )
    {
        if( editingLine && editedIndex < (int) m_editPoints->LinesSize() )
            m_editedPoint = &m_editPoints->Line( editedIndex );
        else if( !editingLine && editedIndex < (int) m_editPoints->PointsSize() )
            m_editedPoint = &m_editPoints->Point( editedIndex );
        else
            m_editedPoint = nullptr;
    }
    else
    {
        m_editedPoint = nullptr;
    }

    getView()->Update( m_editPoints.get() );

    if( m_angleItem )
        getView()->Update( m_angleItem.get() );
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
    EDA_ITEM*  parent = m_editPoints ? m_editPoints->GetParent() : nullptr;
    EDIT_LINE* line = dynamic_cast<EDIT_LINE*>( m_editedPoint );
    bool       isPoly = false;

    if( parent )
    {
        switch( parent->Type() )
        {
        case PCB_ZONE_T:
            isPoly = true;
            break;

        case PCB_SHAPE_T:
            isPoly = static_cast<PCB_SHAPE*>( parent )->GetShape() == SHAPE_T::POLY;
            break;

        default:
            break;
        }
    }

    if( aEnabled )
    {
        if( line && isPoly )
        {
            // For polygon lines, toggle the mode on the existing constraint rather than
            // creating a new one. This preserves the original reference positions.
            EC_CONVERGING* constraint = dynamic_cast<EC_CONVERGING*>( line->GetConstraint() );

            if( constraint )
                constraint->SetMode( POLYGON_LINE_MODE::FIXED_LENGTH );

            // Don't set m_altConstraint - we're modifying the line's own constraint
        }
        else
        {
            // Find a proper constraining point for angle snapping mode
            m_altConstrainer = get45DegConstrainer();

            if( Is90Limited() )
                m_altConstraint.reset( new EC_90DEGREE( *m_editedPoint, m_altConstrainer ) );
            else
                m_altConstraint.reset( new EC_45DEGREE( *m_editedPoint, m_altConstrainer ) );
        }
    }
    else
    {
        if( line && isPoly )
        {
            // Restore the line's constraint to CONVERGING mode
            EC_CONVERGING* constraint = dynamic_cast<EC_CONVERGING*>( line->GetConstraint() );

            if( constraint )
                constraint->SetMode( POLYGON_LINE_MODE::CONVERGING );
        }

        m_altConstraint.reset();
    }
}


EDIT_POINT PCB_POINT_EDITOR::get45DegConstrainer() const
{
    // If there's a behaviour and it provides a constrainer, use that
    if( m_editorBehavior )
    {
        const OPT_VECTOR2I constrainer = m_editorBehavior->Get45DegreeConstrainer( *m_editedPoint, *m_editPoints );

        if( constrainer )
            return EDIT_POINT( *constrainer );
    }

    // In any other case we may align item to its original position
    return m_original;
}


// Finds a corresponding vertex in a polygon set
static std::pair<bool, SHAPE_POLY_SET::VERTEX_INDEX> findVertex( SHAPE_POLY_SET& aPolySet, const EDIT_POINT& aPoint )
{
    for( auto it = aPolySet.IterateWithHoles(); it; ++it )
    {
        auto vertexIdx = it.GetIndex();

        if( aPolySet.CVertex( vertexIdx ) == aPoint.GetPosition() )
            return std::make_pair( true, vertexIdx );
    }

    return std::make_pair( false, SHAPE_POLY_SET::VERTEX_INDEX() );
}


bool PCB_POINT_EDITOR::CanRemoveCorner( const SELECTION& )
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
    if( vertexIdx.m_contour == 0
            && polyset->Polygon( vertexIdx.m_polygon )[vertexIdx.m_contour].PointCount() <= 3 )
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
    if( !item || !CanAddCorner( *item ) )
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
            PCB_SHAPE* newSegment = static_cast<PCB_SHAPE*>( graphicItem->Duplicate( true, &commit ) );
            newSegment->ClearSelected();
            newSegment->SetStart( nearestPoint );
            newSegment->SetEnd( VECTOR2I( seg.B.x, seg.B.y ) );

            commit.Add( newSegment );
            commit.Push( _( "Split Segment" ) );
            break;
        }
        case SHAPE_T::ARC:
        {
            commit.Modify( graphicItem );

            const SHAPE_ARC arc( graphicItem->GetStart(), graphicItem->GetArcMid(), graphicItem->GetEnd(), 0 );
            const VECTOR2I  nearestPoint = arc.NearestPoint( cursorPos );

            // Move the end of the arc to the break point..
            graphicItem->SetEnd( nearestPoint );

            // and add another one starting from the break point
            PCB_SHAPE* newArc = static_cast<PCB_SHAPE*>( graphicItem->Duplicate( true, &commit ) );

            newArc->ClearSelected();
            newArc->SetEnd( arc.GetP1() );
            newArc->SetStart( nearestPoint );

            commit.Add( newArc );
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
                m_toolMgr->RunAction( ACTIONS::selectionClear );
                commit.Remove( item );
            }
        }

        setEditedPoint( nullptr );

        if( item->Type() == PCB_ZONE_T )
            commit.Push( _( "Remove Zone Corner" ) );
        else
            commit.Push( _( "Remove Polygon Corner" ) );

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
        setback = std::min( setback, (int) ( segA.Length() * 0.25 ) );
        setback = std::min( setback, (int) ( segB.Length() * 0.25 ) );

        CHAMFER_PARAMS chamferParams{ setback, setback };

        std::optional<CHAMFER_RESULT> chamferResult = ComputeChamferPoints( segA, segB, chamferParams );

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

        m_arcEditMode = IncrementArcEditMode( m_arcEditMode );
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
    Go( &PCB_POINT_EDITOR::changeArcEditMode, ACTIONS::pointEditorArcKeepCenter.MakeEvent() );
    Go( &PCB_POINT_EDITOR::changeArcEditMode, ACTIONS::pointEditorArcKeepEndpoint.MakeEvent() );
    Go( &PCB_POINT_EDITOR::changeArcEditMode, ACTIONS::pointEditorArcKeepRadius.MakeEvent() );
    Go( &PCB_POINT_EDITOR::changeArcEditMode, ACTIONS::cycleArcEditMode.MakeEvent() );
    Go( &PCB_POINT_EDITOR::modifiedSelection, EVENTS::SelectedItemsModified );
    Go( &PCB_POINT_EDITOR::modifiedSelection, EVENTS::SelectedItemsMoved );
    Go( &PCB_POINT_EDITOR::OnSelectionChange, EVENTS::PointSelectedEvent );
    Go( &PCB_POINT_EDITOR::OnSelectionChange, EVENTS::SelectedEvent );
    Go( &PCB_POINT_EDITOR::OnSelectionChange, EVENTS::UnselectedEvent );
    Go( &PCB_POINT_EDITOR::OnSelectionChange, EVENTS::InhibitSelectionEditing );
    Go( &PCB_POINT_EDITOR::OnSelectionChange, EVENTS::UninhibitSelectionEditing );
}
