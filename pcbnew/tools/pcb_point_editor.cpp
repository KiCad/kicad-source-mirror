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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
#include <preview_items/simple_overlay_item.h>
#include <gal/painter.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <tools/pcb_point_editor.h>
#include <tools/pcb_grid_helper.h>
#include <tools/block_drag_capability.h>
#include <tools/block_drag_dependents.h>
#include <tools/block_drag_displacement.h>
#include <tools/block_drag_pick.h>
#include <tools/block_drag_validation.h>
#include <tools/generator_tool.h>
#include <tools/zone_filler_tool.h>
#include <router/router_tool.h>
#include <router/pns_router.h>
#include <router/pns_block_dragger.h>
#include <router/router_status_view_item.h>
#include <router/pns_drag_algo.h>
#include <router/pns_kicad_iface.h>
#include <dialogs/dialog_unit_entry.h>
#include <board_commit.h>
#include <pcb_edit_frame.h>
#include <pcb_reference_image.h>
#include <pcb_generator.h>
#include <pcb_group.h>
#include <pcb_dimension.h>
#include <pcb_barcode.h>
#include <pcb_grid_item.h>
#include <pcb_textbox.h>
#include <pcb_tablecell.h>
#include <pcb_table.h>
#include <pad.h>
#include <zone.h>
#include <zone_filler.h>
#include <footprint.h>
#include <board.h>
#include <constraints/board_constraint_adapter.h>
#include <constraints/constraint_builder.h>
#include <constraints/pcb_constraint.h>
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

static std::vector<VECTOR2I> getConstraintDirections( EDIT_RELATION* aRelation )
{
    std::vector<VECTOR2I> directions;

    if( !aRelation )
        return directions;

    switch( aRelation->Kind() )
    {
    case EDIT_RELATION_KIND::ANGLE_STEP_90:
        appendDirection( directions, VECTOR2I( 1, 0 ) );
        appendDirection( directions, VECTOR2I( 0, 1 ) );
        break;

    case EDIT_RELATION_KIND::ANGLE_STEP_45:
        appendDirection( directions, VECTOR2I( 1, 0 ) );
        appendDirection( directions, VECTOR2I( 0, 1 ) );
        appendDirection( directions, VECTOR2I( 1, 1 ) );
        appendDirection( directions, VECTOR2I( 1, -1 ) );
        break;

    case EDIT_RELATION_KIND::SAME_X:
        appendDirection( directions, VECTOR2I( 0, 1 ) );
        break;

    case EDIT_RELATION_KIND::SAME_Y:
        appendDirection( directions, VECTOR2I( 1, 0 ) );
        break;

    case EDIT_RELATION_KIND::POINT_ON_LINE:
        appendDirection( directions, aRelation->Direction() );
        break;

    default:
        break;
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


static std::optional<CONSTRAINT_MEMBER> constraintMemberForEditPoint(
        PCB_SHAPE* aShape, EDIT_POINTS* aPoints, EDIT_POINT* aEditedPoint )
{
    if( !aShape || !aPoints || !aEditedPoint )
        return std::nullopt;

    SHAPE_T type = aShape->GetShape();
    VECTOR2I position = aEditedPoint->GetPosition();

    if( type == SHAPE_T::SEGMENT || type == SHAPE_T::ARC || type == SHAPE_T::BEZIER )
    {
        if( position == aShape->GetStart() )
            return CONSTRAINT_MEMBER( aShape->m_Uuid, CONSTRAINT_ANCHOR::START );

        if( position == aShape->GetEnd() )
            return CONSTRAINT_MEMBER( aShape->m_Uuid, CONSTRAINT_ANCHOR::END );

        if( type == SHAPE_T::ARC && position == aShape->GetCenter() )
            return CONSTRAINT_MEMBER( aShape->m_Uuid, CONSTRAINT_ANCHOR::CENTER );
    }
    else if( type == SHAPE_T::CIRCLE && position == aShape->GetCenter() )
    {
        return CONSTRAINT_MEMBER( aShape->m_Uuid, CONSTRAINT_ANCHOR::CENTER );
    }
    else if( type == SHAPE_T::RECTANGLE && aPoints->PointsSize() >= RECT_MAX_POINTS )
    {
        for( unsigned i = RECT_TOP_LEFT; i <= RECT_BOT_LEFT; ++i )
        {
            if( aEditedPoint == &aPoints->Point( i ) )
                return CONSTRAINT_MEMBER( aShape->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, i );
        }

        for( unsigned i = 0; i < aPoints->LinesSize() && i < 4; ++i )
        {
            if( aEditedPoint == &aPoints->Line( i ) )
                return CONSTRAINT_MEMBER( aShape->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, i );
        }
    }
    else if( type == SHAPE_T::POLY && ConstraintPolygonIsModelable( aShape ) )
    {
        for( unsigned i = 0; i < aPoints->PointsSize(); ++i )
        {
            if( aEditedPoint == &aPoints->Point( i ) )
                return CONSTRAINT_MEMBER( aShape->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, i );
        }

        for( unsigned i = 0; i < aPoints->LinesSize(); ++i )
        {
            if( aEditedPoint == &aPoints->Line( i ) )
                return CONSTRAINT_MEMBER( aShape->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, i );
        }
    }

    return std::nullopt;
}


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


/// Text boxes have different point counts depending on their orientation.
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

        // Neither is a corner the caller can place. The centre moves the whole rectangle and
        // the radius rounds it, both read from the shape as it stands
        aPoints.Point( RECT_CENTER ).SetDrivesWholeShape();
        aPoints.Point( RECT_RADIUS ).SetDrivesWholeShape();

        aPoints.AddLine( aPoints.Point( RECT_TOP_LEFT ), aPoints.Point( RECT_TOP_RIGHT ) );
        aPoints.Line( RECT_TOP ).SetRelation(
                EDIT_RELATION::PerpendicularTranslation( aPoints.Line( RECT_TOP ) ) );
        aPoints.AddLine( aPoints.Point( RECT_TOP_RIGHT ), aPoints.Point( RECT_BOT_RIGHT ) );
        aPoints.Line( RECT_RIGHT ).SetRelation(
                EDIT_RELATION::PerpendicularTranslation( aPoints.Line( RECT_RIGHT ) ) );
        aPoints.AddLine( aPoints.Point( RECT_BOT_RIGHT ), aPoints.Point( RECT_BOT_LEFT ) );
        aPoints.Line( RECT_BOT ).SetRelation(
                EDIT_RELATION::PerpendicularTranslation( aPoints.Line( RECT_BOT ) ) );
        aPoints.AddLine( aPoints.Point( RECT_BOT_LEFT ), aPoints.Point( RECT_TOP_LEFT ) );
        aPoints.Line( RECT_LEFT ).SetRelation(
                EDIT_RELATION::PerpendicularTranslation( aPoints.Line( RECT_LEFT ) ) );
    }

    static void UpdateItem( PCB_SHAPE& aRectangle, const EDIT_POINT& aEditedPoint,
                            EDIT_POINTS& aPoints, const VECTOR2I& aMinSize = { 0, 0 } )
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

        PinEditedCorner( aEditedPoint, aPoints, topLeft, topRight, botLeft, botRight,
                         { 0, 0 }, { 0, 0 }, aMinSize );

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
                aPoints.Line( i ).SetRelation(
                        EDIT_RELATION::PerpendicularTranslation( aPoints.Line( i ) ) );
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
     * @param aEditedPoint
     * @param aEditPoints the points list
     * @param[in,out] aTopLeft is the RECT_TOPLEFT to constraint
     * @param[in,out] aTopRight is the RECT_TOPRIGHT to constraint
     * @param[in,out] aBotLeft is the RECT_BOTLEFT to constraint
     * @param[in,out] aBotRight is the RECT_BOTRIGHT to constraint
     * @param aHole the location of the pad's hole
     * @param aHoleSize the pad's hole size (or {0,0} if it has no hole)
     * @param aMinSize
     */
    static void PinEditedCorner( const EDIT_POINT& aEditedPoint, const EDIT_POINTS& aEditPoints,
                                VECTOR2I& aTopLeft, VECTOR2I& aTopRight, VECTOR2I& aBotLeft, VECTOR2I& aBotRight,
                                const VECTOR2I& aHole = { 0, 0 }, const VECTOR2I& aHoleSize = { 0, 0 },
                                const VECTOR2I& aMinSize = { 0, 0 } )
    {
        int minWidth = std::max( EDA_UNIT_UTILS::Mils2IU( pcbIUScale, 1 ), aMinSize.x );
        int minHeight = std::max( EDA_UNIT_UTILS::Mils2IU( pcbIUScale, 1 ), aMinSize.y );

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


/**
 * Looks the outline up on every call instead of holding a reference to it, because
 * undoing a commit gives the zone a new SHAPE_POLY_SET while this object lives on.
 */
class ZONE_POINT_EDIT_BEHAVIOR : public POINT_EDIT_BEHAVIOR
{
public:
    ZONE_POINT_EDIT_BEHAVIOR( ZONE& aZone ) :
            m_zone( aZone )
    {}

    void MakePoints( EDIT_POINTS& aPoints ) override
    {
        POLYGON_POINT_EDIT_BEHAVIOR::BuildForPolyOutline( aPoints, *m_zone.Outline() );
    }

    bool UpdatePoints( EDIT_POINTS& aPoints ) override
    {
        POLYGON_POINT_EDIT_BEHAVIOR::UpdatePointsFromOutline( *m_zone.Outline(), aPoints );
        return true;
    }

    void UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                     std::vector<EDA_ITEM*>& aUpdatedItems ) override
    {
        m_zone.UnFill();
        POLYGON_POINT_EDIT_BEHAVIOR::UpdateOutlineFromPoints( *m_zone.Outline(), aEditedPoint, aPoints );
        m_zone.HatchBorder();
    }

    bool UpdateItemAppliesAllPoints() const override { return true; }

    void FinalizeItem( EDIT_POINTS& aPoints, COMMIT& aCommit ) override
    {
        POLYGON_POINT_EDIT_BEHAVIOR::RemoveNullSegmentsIfPolygonSurvives( *m_zone.Outline() );
    }

private:
    ZONE& m_zone;
};


class GENERATOR_POLY_POINT_EDIT_BEHAVIOR : public POLYGON_POINT_EDIT_BEHAVIOR
{
public:
    GENERATOR_POLY_POINT_EDIT_BEHAVIOR( PCB_GENERATOR_POLY& aGen ) :
            POLYGON_POINT_EDIT_BEHAVIOR( aGen.Outline() ),
            m_gen( aGen )
    {
    }

    void UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                     std::vector<EDA_ITEM*>& aUpdatedItems ) override
    {
        // Defer to the base class to update the polygon
        POLYGON_POINT_EDIT_BEHAVIOR::UpdateItem( aEditedPoint, aPoints, aCommit, aUpdatedItems );

        // Flag the generator has to be updated based on potential outline change
        m_gen.MarkDirty();
    }

private:
    PCB_GENERATOR_POLY& m_gen;
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
                    aPoints.Point( a ).SetRelation( EDIT_RELATION::Angle45( aPoints.Point( b ) ) );
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


class GRID_POINT_EDIT_BEHAVIOR : public POINT_EDIT_BEHAVIOR
{
public:
    GRID_POINT_EDIT_BEHAVIOR( PCB_GRID_ITEM& aGridItem ) :
            m_gridItem( aGridItem )
    {
    }

    void MakePoints( EDIT_POINTS& aPoints ) override
    {
        // Index 0: centre (drag = translate).
        aPoints.AddPoint( m_gridItem.GetPosition() );

        switch( m_gridItem.GetGridItemType() )
        {
        case PCB_GRID_TYPE::POLAR:
            aPoints.AddPoint( toWorld( radiusEndLocal() ) );
            aPoints.AddPoint( toWorld( arcEndLocal() ) );
            aPoints.AddPoint( toWorld( arcMidLocal() ) );
            break;

        case PCB_GRID_TYPE::CARTESIAN:
            aPoints.AddPoint( toWorld( cornerLocal( 0, 0 ) ) );
            aPoints.AddPoint( toWorld( cornerLocal( 1, 0 ) ) );
            aPoints.AddPoint( toWorld( cornerLocal( 1, 1 ) ) );
            aPoints.AddPoint( toWorld( cornerLocal( 0, 1 ) ) );
            break;

        default:
            wxFAIL_MSG( wxT( "MakePoints: unhandled PCB_GRID_TYPE" ) );
            break;
        }
    }

    bool UpdatePoints( EDIT_POINTS& aPoints ) override
    {
        switch( m_gridItem.GetGridItemType() )
        {
        case PCB_GRID_TYPE::POLAR:
            if( aPoints.PointsSize() != 4 )
                return false;

            aPoints.Point( 0 ).SetPosition( m_gridItem.GetPosition() );
            aPoints.Point( 1 ).SetPosition( toWorld( radiusEndLocal() ) );
            aPoints.Point( 2 ).SetPosition( toWorld( arcEndLocal() ) );
            aPoints.Point( 3 ).SetPosition( toWorld( arcMidLocal() ) );
            return true;

        case PCB_GRID_TYPE::CARTESIAN:
            if( aPoints.PointsSize() != 5 )
                return false;

            aPoints.Point( 0 ).SetPosition( m_gridItem.GetPosition() );
            aPoints.Point( 1 ).SetPosition( toWorld( cornerLocal( 0, 0 ) ) );
            aPoints.Point( 2 ).SetPosition( toWorld( cornerLocal( 1, 0 ) ) );
            aPoints.Point( 3 ).SetPosition( toWorld( cornerLocal( 1, 1 ) ) );
            aPoints.Point( 4 ).SetPosition( toWorld( cornerLocal( 0, 1 ) ) );
            return true;

        default:
            wxFAIL_MSG( wxT( "UpdatePoints: unhandled PCB_GRID_TYPE" ) );
            return false;
        }
    }

    void UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                     std::vector<EDA_ITEM*>& aUpdatedItems ) override
    {
        // Centre handle: translate the whole grid.
        if( isModified( aEditedPoint, aPoints.Point( 0 ) ) )
        {
            m_gridItem.SetPosition( aEditedPoint.GetPosition() );
            return;
        }

        const VECTOR2I local = toLocal( aEditedPoint.GetPosition() );

        switch( m_gridItem.GetGridItemType() )
        {
        case PCB_GRID_TYPE::POLAR:
        {
            // World offset from grid centre.
            const VECTOR2I worldOff = aEditedPoint.GetPosition() - m_gridItem.GetPosition();
            const double   dragAngle = std::atan2( worldOff.y, worldOff.x );
            const int      newRadius = std::max( 1, KiROUND( std::hypot( worldOff.x, worldOff.y ) ) );
            const double   orientOld = m_gridItem.GetOrientation().AsRadians();
            const double   phiMaxOld = m_gridItem.GetPhiExtent().AsRadians();

            // KiCad screen rotation: a local point at phi lands at world math-angle
            // (phi - orient).  Each handle drag pins exactly one endpoint and follows
            // the (already-snapped) cursor; the unmoved endpoint stays put.
            const auto wrap2pi = []( double a )
            {
                while( a < 0 )
                    a += 2 * M_PI;
                while( a >= 2 * M_PI )
                    a -= 2 * M_PI;
                return a;
            };

            if( isModified( aEditedPoint, aPoints.Point( 1 ) ) ) // radius end (phi=0)
            {
                // phi=0 follows the cursor; the phi=phiMax end stays at world angle
                // (phiMaxOld - orientOld).
                const double newOrient = -dragAngle;
                const double newPhiMax = wrap2pi( phiMaxOld - orientOld - dragAngle );

                m_gridItem.SetOrientation( EDA_ANGLE( newOrient, RADIANS_T ) );
                m_gridItem.SetPhiExtentDegrees( newPhiMax * 180.0 / M_PI );
            }
            else if( isModified( aEditedPoint, aPoints.Point( 2 ) ) ) // arc end (phi=phiMax)
            {
                // phi=phiMax follows the cursor; the phi=0 end (orient direction) stays.
                const double newPhiMax = wrap2pi( dragAngle + orientOld );

                m_gridItem.SetPhiExtentDegrees( newPhiMax * 180.0 / M_PI );
            }
            else if( isModified( aEditedPoint, aPoints.Point( 3 ) ) ) // arc mid (bisector)
            {
                // Bisector follows the cursor; phiMax unchanged (whole wedge rotates).
                const double newOrient = phiMaxOld / 2.0 - dragAngle;

                m_gridItem.SetOrientation( EDA_ANGLE( newOrient, RADIANS_T ) );
            }

            m_gridItem.SetRadiusExtent( newRadius );
            break;
        }

        case PCB_GRID_TYPE::CARTESIAN:
        {
            // Dragging a corner resizes symmetrically about the centre.
            const int newHalfX = std::max( 1, std::abs( local.x ) );
            const int newHalfY = std::max( 1, std::abs( local.y ) );

            // SetExtent takes size/2 directly, bypassing the property panel's
            // total-width 2x / /2 accessor dance.
            m_gridItem.SetExtent( VECTOR2I( newHalfX, newHalfY ) );
            break;
        }

        default:
            wxFAIL_MSG( wxT( "UpdateItem: unhandled PCB_GRID_TYPE" ) );
            break;
        }
    }

private:
    VECTOR2I toLocal( const VECTOR2I& aWorld ) const
    {
        VECTOR2I local = aWorld - m_gridItem.GetPosition();
        RotatePoint( local, -m_gridItem.GetOrientation() );
        return local;
    }

    VECTOR2I toWorld( const VECTOR2I& aLocal ) const
    {
        VECTOR2I world = aLocal;
        RotatePoint( world, m_gridItem.GetOrientation() );
        return m_gridItem.GetPosition() + world;
    }

    // aX / aY in {0, 1}: 0 = negative half, 1 = positive half.
    VECTOR2I cornerLocal( int aX, int aY ) const
    {
        const int sx = ( aX == 0 ) ? -1 : +1;
        const int sy = ( aY == 0 ) ? -1 : +1;
        return VECTOR2I( sx * m_gridItem.GetExtent().x, sy * m_gridItem.GetExtent().y );
    }

    VECTOR2I radiusEndLocal() const { return VECTOR2I( m_gridItem.GetRadiusExtent(), 0 ); }

    VECTOR2I arcEndLocal() const
    {
        const double phi = m_gridItem.GetPhiExtent().AsRadians();
        const int    r = m_gridItem.GetRadiusExtent();
        return VECTOR2I( KiROUND( r * std::cos( phi ) ), KiROUND( r * std::sin( phi ) ) );
    }

    VECTOR2I arcMidLocal() const
    {
        const double phi = m_gridItem.GetPhiExtent().AsRadians() / 2.0;
        const int    r = m_gridItem.GetRadiusExtent();
        return VECTOR2I( KiROUND( r * std::cos( phi ) ), KiROUND( r * std::sin( phi ) ) );
    }

    PCB_GRID_ITEM& m_gridItem;
};


class PCB_TABLECELL_POINT_EDIT_BEHAVIOR : public EDA_TABLECELL_POINT_EDIT_BEHAVIOR
{
public:
    PCB_TABLECELL_POINT_EDIT_BEHAVIOR( PCB_TABLECELL& aCell ) :
            EDA_TABLECELL_POINT_EDIT_BEHAVIOR( aCell ),
            m_cell( aCell )
    {}

    void MakePoints( EDIT_POINTS& aPoints ) override
    {
        aPoints.AddPoint( edgeMidpoint( COL_WIDTH ) );
        aPoints.AddPoint( edgeMidpoint( ROW_HEIGHT ) );
    }

    bool UpdatePoints( EDIT_POINTS& aPoints ) override
    {
        wxCHECK( aPoints.PointsSize() == TABLECELL_MAX_POINTS, false );

        aPoints.Point( COL_WIDTH ).SetPosition( edgeMidpoint( COL_WIDTH ) );
        aPoints.Point( ROW_HEIGHT ).SetPosition( edgeMidpoint( ROW_HEIGHT ) );

        return true;
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
            m_cell.SetEnd( VECTOR2I( unturned( aPoints.Point( COL_WIDTH ).GetPosition() ).x, m_cell.GetEndY() ) );

            int colWidth = m_cell.GetRectangleWidth();

            for( int ii = 0; ii < m_cell.GetColSpan() - 1; ++ii )
                colWidth -= table.GetColWidth( m_cell.GetColumn() + ii );

            table.SetColWidth( m_cell.GetColumn() + m_cell.GetColSpan() - 1, colWidth );
        }
        else if( isModified( aEditedPoint, aPoints.Point( ROW_HEIGHT ) ) )
        {
            m_cell.SetEnd( VECTOR2I( m_cell.GetEndX(), unturned( aPoints.Point( ROW_HEIGHT ).GetPosition() ).y ) );

            int rowHeight = m_cell.GetRectangleHeight();

            for( int ii = 0; ii < m_cell.GetRowSpan() - 1; ++ii )
                rowHeight -= table.GetRowHeight( m_cell.GetRow() + ii );

            table.SetRowHeight( m_cell.GetRow() + m_cell.GetRowSpan() - 1, rowHeight );
        }

        table.Normalize();
    }

private:
    // A cell keeps its rectangle square to the board and carries the turn in its text angle, so
    // the handles belong on the edges it is drawn with, not on the stored rectangle.
    VECTOR2I edgeMidpoint( TABLECELL_POINTS aPoint ) const
    {
        std::vector<VECTOR2I> corners = m_cell.GetCorners();

        return aPoint == COL_WIDTH ? ( corners[1] + corners[2] ) / 2 : ( corners[2] + corners[3] ) / 2;
    }

    // Bring a dragged handle back into the frame the rectangle is stored in.
    VECTOR2I unturned( const VECTOR2I& aPoint ) const
    {
        BOX2I box;
        box.Merge( m_cell.GetStart() );
        box.Merge( m_cell.GetEnd() );

        VECTOR2I pt = aPoint;
        RotatePoint( pt, box.GetCenter(), -m_cell.GetDrawRotation() );

        return pt;
    }

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
            aPoints.Point( DIM_CROSSBARSTART )
                    .SetRelation( EDIT_RELATION::PointOnLine(
                            aPoints.Point( DIM_CROSSBARSTART ), aPoints.Point( DIM_START ) ) );
            aPoints.Point( DIM_CROSSBAREND )
                    .SetRelation( EDIT_RELATION::PointOnLine(
                            aPoints.Point( DIM_CROSSBAREND ), aPoints.Point( DIM_END ) ) );
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

            aPoints.Point( DIM_CROSSBARSTART )
                    .SetRelation( EDIT_RELATION::PointOnLine(
                            aPoints.Point( DIM_CROSSBARSTART ), aPoints.Point( DIM_START ) ) );
            aPoints.Point( DIM_CROSSBAREND )
                    .SetRelation( EDIT_RELATION::PointOnLine(
                            aPoints.Point( DIM_CROSSBAREND ), aPoints.Point( DIM_END ) ) );
        }
        else if( isModified( aEditedPoint, aPoints.Point( DIM_END ) ) )
        {
            m_dimension.SetEnd( aEditedPoint.GetPosition() );
            m_dimension.Update();

            aPoints.Point( DIM_CROSSBARSTART )
                    .SetRelation( EDIT_RELATION::PointOnLine(
                            aPoints.Point( DIM_CROSSBARSTART ), aPoints.Point( DIM_START ) ) );
            aPoints.Point( DIM_CROSSBAREND )
                    .SetRelation( EDIT_RELATION::PointOnLine(
                            aPoints.Point( DIM_CROSSBAREND ), aPoints.Point( DIM_END ) ) );
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
            VECTOR2I direction = directionA.SquaredEuclideanNorm() < directionB.SquaredEuclideanNorm() ? directionA
                                                                                                      : directionB;

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

        aPoints.Point( DIM_END ).SetRelation( EDIT_RELATION::Angle45( aPoints.Point( DIM_START ) ) );
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

        aPoints.Point( DIM_KNEE )
                .SetRelation( EDIT_RELATION::PointOnLine( aPoints.Point( DIM_START ),
                                                          aPoints.Point( DIM_END ) ) );
        aPoints.Point( DIM_KNEE ).SetSnapConstraint( IGNORE_SNAPS );

        aPoints.Point( DIM_TEXT ).SetRelation( EDIT_RELATION::Angle45( aPoints.Point( DIM_KNEE ) ) );
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

            aPoints.Point( DIM_KNEE )
                    .SetRelation( EDIT_RELATION::PointOnLine( aPoints.Point( DIM_START ),
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

            aPoints.Point( DIM_KNEE )
                    .SetRelation( EDIT_RELATION::PointOnLine( aPoints.Point( DIM_START ),
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

        aPoints.Point( DIM_TEXT ).SetRelation( EDIT_RELATION::Angle45( aPoints.Point( DIM_END ) ) );
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
        {
            m_textbox.ClearBoundingBoxCache();
            VECTOR2I minSize = m_textbox.GetMinSize();
            RECTANGLE_POINT_EDIT_BEHAVIOR::UpdateItem( m_textbox, aEditedPoint, aPoints, minSize );
        }
    }

private:
    PCB_TEXTBOX& m_textbox;
};

class SHAPE_GROUP_POINT_EDIT_BEHAVIOR : public POINT_EDIT_BEHAVIOR
{
public:
    SHAPE_GROUP_POINT_EDIT_BEHAVIOR( PCB_GROUP& aGroup ) :
            m_group( &aGroup ),
            m_parent( &aGroup ),
            m_pinned( std::nullopt )
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

    SHAPE_GROUP_POINT_EDIT_BEHAVIOR( std::vector<PCB_SHAPE*> aShapes, BOARD_ITEM* aParent,
                                     const std::optional<VECTOR2I>& aPinned ) :
            m_group( nullptr ),
            m_shapes( std::move( aShapes ) ),
            m_parent( aParent ),
            m_pinned( aPinned )
    {
        for( PCB_SHAPE* shape : m_shapes )
            m_originalWidths[shape] = static_cast<double>( shape->GetWidth() );
    }

    // Gap in board units that keeps the box handles clear of the shapes' own handles
    void SetInflation( int aInflation ) { m_inflation = aInflation; }

    void MakePoints( EDIT_POINTS& aPoints ) override
    {
        BOX2I bbox = getBoundingBox();
        bbox.Inflate( m_inflation );
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
        bbox.Inflate( m_inflation );
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
            if( m_pinned )
            {
                UpdatePoints( aPoints );
                return;
            }

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

        // Take the gap back out before pinning, so the minimum size is measured on the
        // real box and a corner the user did not touch scales by exactly one
        tl += VECTOR2I( m_inflation, m_inflation );
        tr += VECTOR2I( -m_inflation, m_inflation );
        bl += VECTOR2I( m_inflation, -m_inflation );
        br -= VECTOR2I( m_inflation, m_inflation );

        RECTANGLE_POINT_EDIT_BEHAVIOR::PinEditedCorner( aEditedPoint, aPoints, tl, tr, bl, br );

        VECTOR2I origin = oldCenter;
        double   scale;

        // A scale holds exactly one point still, so a pinned selection has to scale about the pin
        if( m_pinned )
        {
            VECTOR2I oldCorner( oldBox.GetLeft(), oldBox.GetTop() );
            VECTOR2I newCorner = tl;

            if( isModified( aEditedPoint, aPoints.Point( RECT_TOP_RIGHT ) ) )
            {
                oldCorner = VECTOR2I( oldBox.GetRight(), oldBox.GetTop() );
                newCorner = tr;
            }
            else if( isModified( aEditedPoint, aPoints.Point( RECT_BOT_RIGHT ) ) )
            {
                oldCorner = VECTOR2I( oldBox.GetRight(), oldBox.GetBottom() );
                newCorner = br;
            }
            else if( isModified( aEditedPoint, aPoints.Point( RECT_BOT_LEFT ) ) )
            {
                oldCorner = VECTOR2I( oldBox.GetLeft(), oldBox.GetBottom() );
                newCorner = bl;
            }

            // Below this share of the selection diagonal a corner is too near the pin to scale by
            const double MIN_ARM_RATIO = 0.1;

            VECTOR2I oldArm = oldCorner - *m_pinned;
            VECTOR2I newArm = newCorner - *m_pinned;
            VECTOR2I diagonal( oldBox.GetWidth(), oldBox.GetHeight() );
            double   span = static_cast<double>( oldArm.SquaredEuclideanNorm() );
            double   floor = static_cast<double>( diagonal.SquaredEuclideanNorm() ) * MIN_ARM_RATIO * MIN_ARM_RATIO;

            // A corner on the pin has no leverage, so any drag of it would scale wildly
            if( span < floor )
            {
                UpdatePoints( aPoints );
                return;
            }

            // Measured along the arm, so a corner level with the pin stays controllable
            origin = *m_pinned;
            scale = static_cast<double>( oldArm.Dot( newArm ) ) / span;
        }
        else
        {
            double sx = static_cast<double>( br.x - tl.x ) / static_cast<double>( oldBox.GetWidth() );
            double sy = static_cast<double>( br.y - tl.y ) / static_cast<double>( oldBox.GetHeight() );

            scale = ( sx + sy ) / 2.0;
        }

        // Prevent scaling below a minimum threshold to avoid precision loss when shapes
        // are scaled to near-zero size. Also prevent negative scaling which would flip
        // shapes when dragging past the center point.
        const double MIN_SCALE = 0.01;

        if( scale < MIN_SCALE )
            scale = MIN_SCALE;

        for( PCB_SHAPE* shape : m_shapes )
        {
            aCommit.Modify( shape );
            shape->Move( -origin );
            shape->Scale( scale );
            shape->Move( origin );

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

    const std::vector<PCB_SHAPE*>& GetShapes() const { return m_shapes; }

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
    std::optional<VECTOR2I>                m_pinned;
    std::unordered_map<PCB_SHAPE*, double> m_originalWidths;
    int                                    m_inflation = 0;
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
        m_angleSnapActive( false ),
        m_blockHoveredPoint( nullptr ),
        m_blockHoveredOwner( nullptr )
{}


void PCB_POINT_EDITOR::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<PCB_BASE_FRAME>();

    // Undo a running gesture first. ResetTools runs before a board reload deletes the
    // old board, so the items are still here. Move the callback out before calling it,
    // so a second reset during the undo cannot call it again
    if( m_blockDragAbort )
    {
        std::function<void()> abort = std::move( m_blockDragAbort );
        m_blockDragAbort = nullptr;
        abort();
    }

    if( KIGFX::VIEW* view = getView() )
    {
        if( m_angleItem && view->HasItem( m_angleItem.get() ) )
            view->Remove( m_angleItem.get() );

        if( m_editPoints && view->HasItem( m_editPoints.get() ) )
            view->Remove( m_editPoints.get() );

        if( view->HasItem( &m_preview ) )
            view->Remove( &m_preview );
    }

    m_editedPoint = nullptr;
    m_hoveredPoint = nullptr;
    clearBlockHandles();

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

    // An edge handle is only pickable in its own right with the feature on, so only then does
    // it show that it is under the cursor or picked
    points->SetLineStateVisible( blockAnchorEditingEnabled() );

    if( !aItem )
        return points;

    bool rejected = false;
    m_editorBehavior = makeBehavior( aItem, rejected );

    if( rejected )
        points.reset();

    if( m_editorBehavior && points )
        m_editorBehavior->MakePoints( *points );

    return points;
}


std::unique_ptr<POINT_EDIT_BEHAVIOR> PCB_POINT_EDITOR::makeBehavior( EDA_ITEM* aItem, bool& aRejected )
{
    std::unique_ptr<POINT_EDIT_BEHAVIOR> behavior;

    switch( aItem->Type() )
    {
    case PCB_REFERENCE_IMAGE_T:
    {
        PCB_REFERENCE_IMAGE& refImage = static_cast<PCB_REFERENCE_IMAGE&>( *aItem );
        behavior = std::make_unique<REFERENCE_IMAGE_POINT_EDIT_BEHAVIOR>( refImage.GetReferenceImage(),
                                                                          pcbIUScale.mmToIU( 1.0 ) );
        break;
    }
    case PCB_BARCODE_T:
    {
        PCB_BARCODE& barcode = static_cast<PCB_BARCODE&>( *aItem );
        behavior = std::make_unique<BARCODE_POINT_EDIT_BEHAVIOR>( barcode );
        break;
    }
    case PCB_GRID_ITEM_T:
    {
        PCB_GRID_ITEM& grid = static_cast<PCB_GRID_ITEM&>( *aItem );
        behavior = std::make_unique<GRID_POINT_EDIT_BEHAVIOR>( grid );
        break;
    }
    case PCB_TEXTBOX_T:
    {
        PCB_TEXTBOX& textbox = static_cast<PCB_TEXTBOX&>( *aItem );
        behavior = std::make_unique<TEXTBOX_POINT_EDIT_BEHAVIOR>( textbox );
        break;
    }
    case PCB_SHAPE_T:
    {
        PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( aItem );

        switch( shape->GetShape() )
        {
        case SHAPE_T::SEGMENT: behavior = std::make_unique<EDA_SEGMENT_POINT_EDIT_BEHAVIOR>( *shape ); break;

        case SHAPE_T::RECTANGLE: behavior = std::make_unique<RECTANGLE_POINT_EDIT_BEHAVIOR>( *shape ); break;

        case SHAPE_T::ARC:
            behavior = std::make_unique<EDA_ARC_POINT_EDIT_BEHAVIOR>( *shape, m_arcEditMode, *getViewControls(),
                                                                      pcbIUScale );
            break;

        case SHAPE_T::CIRCLE: behavior = std::make_unique<EDA_CIRCLE_POINT_EDIT_BEHAVIOR>( *shape ); break;

        case SHAPE_T::POLY: behavior = std::make_unique<EDA_POLYGON_POINT_EDIT_BEHAVIOR>( *shape ); break;

        case SHAPE_T::BEZIER:
            behavior = std::make_unique<EDA_BEZIER_POINT_EDIT_BEHAVIOR>( *shape, shape->GetMaxError() );
            break;

        case SHAPE_T::ELLIPSE:
        case SHAPE_T::ELLIPSE_ARC: behavior = std::make_unique<EDA_ELLIPSE_POINT_EDIT_BEHAVIOR>( *shape ); break;

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
        {
            std::unique_ptr<SHAPE_GROUP_POINT_EDIT_BEHAVIOR> groupBehavior =
                    std::make_unique<SHAPE_GROUP_POINT_EDIT_BEHAVIOR>( *group );
            groupBehavior->SetInflation( bboxHandleInflation() );
            behavior = std::move( groupBehavior );
        }
        else
        {
            aRejected = true;
        }

        break;
    }

    case PCB_TABLECELL_T:
    {
        PCB_TABLECELL* cell = static_cast<PCB_TABLECELL*>( aItem );

        // No support for point-editing of a rotated table
        if( cell->GetShape() == SHAPE_T::RECTANGLE )
            behavior = std::make_unique<PCB_TABLECELL_POINT_EDIT_BEHAVIOR>( *cell );

        break;
    }

    case PCB_PAD_T:
    {
        // Pad edit only for the footprint editor
        if( m_isFootprintEditor )
        {
            PAD& pad = static_cast<PAD&>( *aItem );
            PCB_LAYER_ID activeLayer = m_frame ? m_frame->GetActiveLayer() : PADSTACK::TEMP_ALL_LAYERS;

            // Point editor only handles copper shape changes
            if( !IsCopperLayer( activeLayer ) )
                activeLayer = IsFrontLayer( activeLayer ) ? F_Cu : B_Cu;

            behavior = std::make_unique<PAD_POINT_EDIT_BEHAVIOR>( pad, activeLayer );
        }
        break;
    }

    case PCB_ZONE_T:
    {
        ZONE& zone = static_cast<ZONE&>( *aItem );
        behavior = std::make_unique<ZONE_POINT_EDIT_BEHAVIOR>( zone );
        break;
    }

    case PCB_GENERATOR_T:
    {
        if( dynamic_cast<PCB_GENERATOR_POLY*>( aItem ) )
        {
            PCB_GENERATOR_POLY* generator = static_cast<PCB_GENERATOR_POLY*>( aItem );
            behavior = std::make_unique<GENERATOR_POLY_POINT_EDIT_BEHAVIOR>( *generator );
            break;
        }
        else
        {
            PCB_GENERATOR* generator = static_cast<PCB_GENERATOR*>( aItem );
            behavior = std::make_unique<GENERATOR_POINT_EDIT_BEHAVIOR>( *generator );
        }
        break;
    }

    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_ORTHOGONAL_T:
    {
        PCB_DIM_ALIGNED& dimension = static_cast<PCB_DIM_ALIGNED&>( *aItem );
        behavior = std::make_unique<ALIGNED_DIMENSION_POINT_EDIT_BEHAVIOR>( dimension );
        break;
    }

    case PCB_DIM_CENTER_T:
    {
        PCB_DIM_CENTER& dimension = static_cast<PCB_DIM_CENTER&>( *aItem );
        behavior = std::make_unique<DIM_CENTER_POINT_EDIT_BEHAVIOR>( dimension );
        break;
    }

    case PCB_DIM_RADIAL_T:
    {
        PCB_DIM_RADIAL& dimension = static_cast<PCB_DIM_RADIAL&>( *aItem );
        behavior = std::make_unique<DIM_RADIAL_POINT_EDIT_BEHAVIOR>( dimension );
        break;
    }

    case PCB_DIM_LEADER_T:
    {
        PCB_DIM_LEADER& dimension = static_cast<PCB_DIM_LEADER&>( *aItem );
        behavior = std::make_unique<DIM_LEADER_POINT_EDIT_BEHAVIOR>( dimension );
        break;
    }

    default: aRejected = true; break;
    }

    return behavior;
}


int PCB_POINT_EDITOR::bboxHandleInflation() const
{
    // The gap only matters when the shapes show their own handles next to the box
    if( !blockAnchorEditingEnabled() )
        return 0;

    // About 10 screen pixels of clearance at any zoom
    return KiROUND( getView()->ToWorld( 10.0 ) );
}


bool PCB_POINT_EDITOR::blockAnchorEditingEnabled() const
{
    return BlockDragAnchorEditingEnabled( frame() );
}


void PCB_POINT_EDITOR::buildBlockHandles( const PCB_SELECTION& aSelection )
{
    clearBlockHandles();

    std::vector<BOARD_ITEM*> items;

    for( EDA_ITEM* selItem : aSelection )
    {
        if( selItem->IsBOARD_ITEM() )
            items.push_back( static_cast<BOARD_ITEM*>( selItem ) );
    }

    std::vector<BOARD_ITEM*> expanded = BlockDragExpandSelection( items );

    m_expandedSelection = expanded;

    // A lone object that is not a group already shows its handles through m_editPoints
    if( aSelection.Size() == 1 && aSelection.Front()->Type() != PCB_GROUP_T )
        return;

    for( BOARD_ITEM* item : expanded )
    {
        if( BlockDragCapability( *item ) != BLOCK_DRAG_CAPABILITY::DEFORMABLE )
            continue;

        bool                                 rejected = false;
        std::unique_ptr<POINT_EDIT_BEHAVIOR> behavior = makeBehavior( item, rejected );

        if( !behavior )
            continue;

        BLOCK_HANDLE_ENTRY entry;
        entry.behavior = std::move( behavior );
        entry.points = std::make_shared<EDIT_POINTS>( item );
        entry.points->SetLineStateVisible( true );
        entry.behavior->MakePoints( *entry.points );
        getView()->Add( entry.points.get() );
        m_blockHandles.push_back( std::move( entry ) );
    }
}


void PCB_POINT_EDITOR::clearBlockHandles()
{
    if( KIGFX::VIEW* view = getView() )
    {
        for( BLOCK_HANDLE_ENTRY& entry : m_blockHandles )
        {
            if( view->HasItem( entry.points.get() ) )
                view->Remove( entry.points.get() );
        }
    }

    m_blockHandles.clear();
    m_blockHoveredPoint = nullptr;
    m_blockHoveredOwner = nullptr;

    // No repaint, the collections these keys point at are already gone
    m_pickedHandles.clear();
    m_expandedSelection.clear();
}


bool PCB_POINT_EDITOR::HasBlockHandle() const
{
    return m_blockHoveredPoint && m_blockHoveredOwner
           && BlockDragHandlePickable( *m_blockHoveredOwner, *m_blockHoveredPoint );
}


std::vector<EDIT_POINTS*> PCB_POINT_EDITOR::pickableCollections()
{
    std::vector<EDIT_POINTS*> collections;

    for( BLOCK_HANDLE_ENTRY& entry : m_blockHandles )
        collections.push_back( entry.points.get() );

    // The scaling box borrows a member shape as its parent, so it is not pickable
    if( m_editPoints && m_editPoints->GetParent()
        && !dynamic_cast<SHAPE_GROUP_POINT_EDIT_BEHAVIOR*>( m_editorBehavior.get() ) )
    {
        collections.push_back( m_editPoints.get() );
    }

    return collections;
}


bool PCB_POINT_EDITOR::pickHandleAt( const VECTOR2I& aPosition, bool aAccumulate )
{
    for( EDIT_POINTS* points : pickableCollections() )
    {
        EDIT_POINT* point = points->FindPoint( aPosition, getView() );

        if( !point )
            continue;

        // A pickable handle at the same spot always wins
        if( !BlockDragHandlePickable( *points, *point ) )
            continue;

        std::optional<PCB_HANDLE_KEY> key = PCB_HANDLE_KEY::Make( *points, point );

        if( !key )
            return false;

        std::set<PCB_HANDLE_KEY> picked = m_pickedHandles;

        BlockDragApplyPick( picked, *key, aAccumulate );
        setPickedHandles( std::move( picked ) );
        return true;
    }

    return false;
}


void PCB_POINT_EDITOR::clearPickedHandles()
{
    if( m_pickedHandles.empty() )
        return;

    setPickedHandles( {} );
}


void PCB_POINT_EDITOR::setPickedHandles( std::set<PCB_HANDLE_KEY> aPicked )
{
    m_pickedHandles = std::move( aPicked );
    applyPickedFlags();
}


void PCB_POINT_EDITOR::applyPickedFlags()
{
    // Nothing can be picked with the feature off, so leave the plain point editor alone
    if( m_pickedHandles.empty() && !blockAnchorEditingEnabled() )
        return;

    std::vector<EDIT_POINTS*> collections = pickableCollections();

    for( EDIT_POINTS* points : collections )
    {
        for( unsigned ii = 0; ii < points->PointsSize(); ++ii )
            points->Point( ii ).SetSelected( false );

        for( unsigned ii = 0; ii < points->LinesSize(); ++ii )
            points->Line( ii ).SetSelected( false );
    }

    for( auto it = m_pickedHandles.begin(); it != m_pickedHandles.end(); )
    {
        EDIT_POINT* resolved = nullptr;

        for( EDIT_POINTS* points : collections )
        {
            resolved = it->Resolve( *points );

            if( resolved )
                break;
        }

        if( resolved )
        {
            resolved->SetSelected();
            ++it;
        }
        else
        {
            it = m_pickedHandles.erase( it );
        }
    }

    for( EDIT_POINTS* points : collections )
        getView()->Update( points );
}


void PCB_POINT_EDITOR::updateBlockHandlePoints()
{
    for( BLOCK_HANDLE_ENTRY& entry : m_blockHandles )
    {
        // A collection can be rebuilt in place, so a stored hover pointer goes stale
        if( m_blockHoveredOwner == entry.points.get() )
        {
            if( m_blockHoveredPoint )
                m_blockHoveredPoint->SetHover( false );

            m_blockHoveredPoint = nullptr;
            m_blockHoveredOwner = nullptr;
        }

        if( !entry.behavior->UpdatePoints( *entry.points ) )
        {
            EDA_ITEM* parent = entry.points->GetParent();

            if( getView()->HasItem( entry.points.get() ) )
                getView()->Remove( entry.points.get() );

            entry.points = std::make_shared<EDIT_POINTS>( parent );
            entry.points->SetLineStateVisible( true );
            entry.behavior->MakePoints( *entry.points );
            getView()->Add( entry.points.get() );
        }
        else
        {
            getView()->Update( entry.points.get() );
        }
    }
}


void PCB_POINT_EDITOR::updateBlockHandleHover( const VECTOR2I& aPosition )
{
    EDIT_POINT*  hovered = nullptr;
    EDIT_POINTS* owner = nullptr;

    for( BLOCK_HANDLE_ENTRY& entry : m_blockHandles )
    {
        EDIT_POINT* point = entry.points->FindPoint( aPosition, getView() );

        // Light only what a click would pick, the same test pickHandleAt() makes
        if( point && BlockDragHandlePickable( *entry.points, *point ) )
        {
            hovered = point;
            owner = entry.points.get();
            break;
        }
    }

    if( hovered == m_blockHoveredPoint )
        return;

    if( m_blockHoveredPoint )
    {
        m_blockHoveredPoint->SetHover( false );

        if( m_blockHoveredOwner )
            getView()->Update( m_blockHoveredOwner );
    }

    m_blockHoveredPoint = hovered;
    m_blockHoveredOwner = owner;

    if( m_blockHoveredPoint )
    {
        m_blockHoveredPoint->SetHover();
        getView()->Update( m_blockHoveredOwner );
    }
}


void PCB_POINT_EDITOR::updateEditedPoint( const TOOL_EVENT& aEvent )
{
    if( aEvent.IsMotion() )
        updateBlockHandleHover( aEvent.Position() );

    if( !m_editPoints )
        return;

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


// Pass on only navigation, the way the selection tool's own band loops do
static const TOOL_ACTION* const blockDragNavigationActions[] = {
    &ACTIONS::panUp,        &ACTIONS::panDown,        &ACTIONS::panLeft,        &ACTIONS::panRight,
    &ACTIONS::cursorUp,     &ACTIONS::cursorDown,     &ACTIONS::cursorLeft,     &ACTIONS::cursorRight,
    &ACTIONS::cursorUpFast, &ACTIONS::cursorDownFast, &ACTIONS::cursorLeftFast, &ACTIONS::cursorRightFast,
    &ACTIONS::zoomIn,       &ACTIONS::zoomOut,        &ACTIONS::zoomInCenter,   &ACTIONS::zoomOutCenter,
    &ACTIONS::zoomCenter,   &ACTIONS::zoomFitScreen,  &ACTIONS::zoomFitObjects, nullptr
};


static void blockDragPassNavigation( TOOL_EVENT* aEvent )
{
    for( int ii = 0; blockDragNavigationActions[ii]; ++ii )
    {
        if( aEvent->IsAction( blockDragNavigationActions[ii] ) )
        {
            aEvent->SetPassEvent();
            return;
        }
    }
}


// Commands that leave the board alone and so may run while a gesture holds its staged commit
static const TOOL_ACTION* const blockDragSafeActions[] = {
    &ACTIONS::updateMenu,        &ACTIONS::refreshPreview,      &ACTIONS::zoomRedraw,
    &ACTIONS::cursorClick,       &ACTIONS::gridFast1,           &ACTIONS::gridFast2,
    &ACTIONS::gridFastCycle,     &ACTIONS::gridNext,            &ACTIONS::gridPrev,
    &ACTIONS::gridPreset,        &ACTIONS::toggleGrid,          &ACTIONS::toggleGridOverrides,
    &ACTIONS::toggleCursor,      &ACTIONS::togglePolarCoords,   &PCB_ACTIONS::drag45Degree,
    &PCB_ACTIONS::dragFreeAngle, nullptr
};


static bool blockDragAllowsCommand( const TOOL_EVENT& aEvent )
{
    if( aEvent.IsCancelInteractive() || aEvent.IsActivate() )
        return true;

    for( const TOOL_ACTION* const* list : { blockDragNavigationActions, blockDragSafeActions } )
    {
        for( int ii = 0; list[ii]; ++ii )
        {
            if( aEvent.IsAction( list[ii] ) )
                return true;
        }
    }

    return false;
}


namespace
{

// Draw the band in the handle highlight color, so it reads as a handle gesture
class HANDLE_PICK_AREA : public KIGFX::PREVIEW::SIMPLE_OVERLAY_ITEM
{
public:
    void              SetPoly( const SHAPE_LINE_CHAIN& aPoly ) { m_poly = aPoly; }
    SHAPE_LINE_CHAIN& GetPoly() { return m_poly; }

    const BOX2I ViewBBox() const override { return m_poly.BBox(); }

private:
    void drawPreviewShape( KIGFX::VIEW* aView ) const override
    {
        if( m_poly.PointCount() < 2 )
            return;

        KIGFX::GAL& gal = *aView->GetGAL();

        gal.SetIsFill( false );
        gal.SetIsStroke( true );
        gal.SetLineWidth( 0.0 );
        gal.DrawPolygon( m_poly );

        gal.SetIsFill( true );
        gal.SetIsStroke( false );
        gal.DrawPolygon( m_poly );
    }

    SHAPE_LINE_CHAIN m_poly;
};

} // namespace


// The solver can move shapes that share a constraint with the block but were never dragged
static void blockDragAppendConstraintCoMembers( BOARD* aBoard, std::vector<BOARD_ITEM*>& aItems )
{
    std::set<KIID> known;
    bool           grew = true;

    for( BOARD_ITEM* item : aItems )
    {
        known.insert( item->m_Uuid );

        // A carried footprint's pads reach further clusters
        if( item->Type() == PCB_FOOTPRINT_T )
        {
            item->RunOnChildren( [&]( BOARD_ITEM* aChild )
                                 {
                                     known.insert( aChild->m_Uuid );
                                 },
                                 RECURSE_MODE::RECURSE );
        }
    }

    auto include = [&]( BOARD_ITEM* aItem )
    {
        if( known.insert( aItem->m_Uuid ).second )
        {
            aItems.push_back( aItem );
            grew = true;
        }
    };

    const std::vector<PCB_CONSTRAINT*> constraints = CollectResolvedConstraints( aBoard );

    while( grew )
    {
        grew = false;

        for( const PCB_CONSTRAINT* constraint : constraints )
        {
            bool touches = false;

            for( const CONSTRAINT_MEMBER& member : constraint->GetMembers() )
            {
                if( known.count( member.m_item ) )
                {
                    touches = true;
                    break;
                }
            }

            if( !touches )
                continue;

            for( const CONSTRAINT_MEMBER& member : constraint->GetMembers() )
            {
                if( known.count( member.m_item ) )
                    continue;

                BOARD_ITEM* item = aBoard->ResolveItem( member.m_item, true );

                if( !item )
                    continue;

                include( item );

                // On a board the solver moves a constrained child's whole footprint
                if( FOOTPRINT* footprint = item->GetParentFootprint(); footprint && !aBoard->IsFootprintHolder() )
                {
                    include( footprint );
                    footprint->RunOnChildren( include, RECURSE_MODE::RECURSE );
                }
            }
        }
    }
}


/// The release refuses a DRC violation unless the user forces it, and says so the same way
/// wherever it is refused
static wxString blockDragDrcRefusal()
{
    return wxString::Format( _( "The move violates DRC. (%s to commit anyway.)" ),
                             KeyNameFromKeyCode( MD_CTRL + PSEUDO_WXK_CLICK ) );
}


/**
 * The block drag gesture, with the state it holds while it runs.
 *
 * Lives on the stack for one run of PCB_POINT_EDITOR::OnSelectionChange and borrows that
 * call's frame, grid helper and commit. The editor makes it a friend, because the gesture
 * works on the same handle collections, picked set and expanded selection the editor builds,
 * and its area picks run their own event loops through the editor's Wait().
 *
 * The editor's loop decides which event goes where. Everything the gesture then does to the
 * board is behind the calls below.
 */
class BLOCK_DRAG_SESSION
{
public:
    BLOCK_DRAG_SESSION( PCB_POINT_EDITOR& aEditor, PCB_BASE_EDIT_FRAME* aEditFrame, PCB_GRID_HELPER& aGrid,
                        BOARD_COMMIT& aCommit ) :
            m_editor( aEditor ),
            m_editFrame( aEditFrame ),
            m_grid( aGrid ),
            m_commit( aCommit )
    {
    }

    /// Whether a drag is running.
    bool IsActive() const { return m_active; }

    /// Start a drag from the handle at aOrigin. aPressInFlight says a mouse button is down, so a
    /// refusal has the rest of that press to ignore and the drag follows the button. Without it the
    /// drag runs from the keyboard instead, with the cursor free.
    bool Begin( const VECTOR2I& aOrigin, bool aPressInFlight );

    /// Whether this drag runs from the keyboard, so motion moves it and a click places it.
    bool KeyboardDrag() const { return m_keyboardDrag; }

    /// Place one frame of the drag. True when the editor has to leave its loop.
    bool ApplyCursor( const TOOL_EVENT& aEvent );

    /// Finish the drag and push what it changed. True when the editor has to leave its loop.
    bool Release( const TOOL_EVENT& aEvent );

    /// Undo the drag, keeping the picks. True when the editor has to leave its loop.
    bool Cancel( TOOL_EVENT& aEvent );

    /// Undo a drag that is still running, for the exits that miss the release and cancel paths.
    /// Reset() runs it as well, while the items are still here.
    void Abort();

    /// Whether a drag starting on empty canvas should pick handles by area. False with the feature
    /// off, with a handle under the drag origin, with nothing pickable, or on a selected object.
    bool AreaPickStarts( const TOOL_EVENT& aEvent );

    /// Run the lasso or the box, whichever the selection mode asks for. True when the editor
    /// has to leave its loop.
    bool RunAreaPick( const TOOL_EVENT& aEvent );

    /// Whether the last start was refused, in which case the gesture has already said why.
    bool Refused() const { return m_refused; }
    void ClearRefused() { m_refused = false; }

    /// Whether the rest of a swallowed button press is still to be ignored.
    bool PressConsumed() const { return m_pressConsumed; }
    void ClearPressConsumed() { m_pressConsumed = false; }

private:
    /// How one frame of the gesture turned out
    enum class FRAME_RESULT
    {
        APPLIED,       ///< The frame's geometry went onto the board
        LOST_IDENTITY, ///< A picked handle has gone, so the gesture has to stop
        CONSTRAINED,   ///< A constraint blocks this frame, and the board is untouched
    };

    /// Write one frame. First the reshaped items, then the ones that travel whole, then the joined
    /// endpoints that follow, then the constraint solve over all of them. Undoing a frame is this
    /// same call with the last offset the gesture accepted.
    FRAME_RESULT applyFrame( const VECTOR2I& aDelta, std::vector<EDA_ITEM*>& aUpdated );

    /// Pair every collection that holds a moved point with its behavior. Looked up again on each
    /// use, because updatePoints() can rebuild a collection in place.
    bool collectTargets();

    /// Put the view, the flags and the router back, and drop the gesture's state.
    void releaseState();

    /// Undo the gesture. True when the editor has to leave its loop and start again from the live
    /// selection: in the board editor a footprint child is staged through its footprint, and undoing
    /// that gives the footprint fresh children, so every pointer held here to such a child is dead.
    /// The move tool restarts the point editor the same way after its own undo. Once the carried
    /// changes have been pushed, the gesture's undo entry is popped off instead.
    bool revert( bool aPushed );

    /// End the gesture and rebuild the handles.
    void endDrag( bool aKeepPicks );

    /// Say why the release was refused and put the board back, keeping the picks so the user can
    /// try again from the same handles. True when the editor has to leave its loop.
    bool refuseRelease( const wxString& aMessage );

    /// Band colour for the area picks, kept clear of the canvas colour.
    KIGFX::COLOR4D bandColor() const;

    /// Put the pick band on the view and take the cursor for it, and take it off again.
    void showBand( HANDLE_PICK_AREA& aArea );
    void hideBand( HANDLE_PICK_AREA& aArea );

    /// Pick handles with a lasso, the way the object lasso works. True when the band ended on an
    /// event the editor's loop has to handle itself.
    bool runLasso( const TOOL_EVENT& aEvent );

    /// Pick handles with a box, which finishes on release like normal box selection. True when the
    /// band ended on an event the editor's loop has to handle itself.
    bool runBox( const TOOL_EVENT& aEvent );

    // Borrowed from the editor's call for as long as the gesture runs, never owned
    PCB_POINT_EDITOR&    m_editor;
    PCB_BASE_EDIT_FRAME* m_editFrame;
    PCB_GRID_HELPER&     m_grid;
    BOARD_COMMIT&        m_commit;

    bool m_active = false;         ///< A drag is running.
    bool m_keyboardDrag = false;   ///< Started with D, so motion moves it and a click places it.
    bool m_pressConsumed = false;  ///< The rest of a swallowed button press is still to be ignored.
    bool m_refused = false;        ///< The last start was declined and said why itself.
    bool m_revertRestarts = false; ///< Undo will hand out fresh objects, so the editor has to start
                                   ///< again from the live selection.
    bool m_routerActive = false;   ///< A router drag session is carrying the block's copper.

    /// Where each picked handle stood when the gesture began. Every frame is placed from these,
    /// never from the current position, so a long drag cannot drift.
    std::map<PCB_HANDLE_KEY, VECTOR2I> m_origins;

    /// Each collection holding a picked handle, with the behavior that rebuilds its item.
    std::vector<std::pair<EDIT_POINTS*, POINT_EDIT_BEHAVIOR*>> m_targets;

    /// Unselected ends that sit on a moved joint and follow it, with the handle drawn at each one.
    /// The two line up index by index.
    std::vector<BLOCK_DRAG_ENDPOINT>          m_dependents;
    std::vector<std::shared_ptr<EDIT_POINTS>> m_dependentDisplays;

    /// Selected items with no picked handle, which travel whole as they would under the move tool.
    std::vector<BOARD_ITEM*> m_rigidItems;

    /// Everything that moves with the cursor, kept out of the snap targets so the block cannot
    /// chase its own geometry. Also the list the release clears the moving flags from.
    std::vector<BOARD_ITEM*> m_moving;

    /// Copper whose selection overlay is hidden while the router preview stands in for it, by id
    /// because fixing the route can replace a carried segment.
    std::vector<KIID> m_overlayHidden;

    /// What the solve holds fixed: the anchors the gesture placed itself, and the items it carried
    /// whole, whose offset is pinned so the solve cannot pull them back out.
    std::vector<std::pair<PCB_SHAPE*, CONSTRAINT_MEMBER>> m_constraintAnchors;
    std::set<KIID>                                        m_constraintTranslated;

    std::optional<BOARD_CONSTRAINT_BLOCK_SESSION> m_constraintSession;
    bool                                          m_constraintWarned = false; ///< Said once, not per frame.

    /// Neighbours the solve pulled along, brightened so the user can see what moved with the block.
    std::set<PCB_SHAPE*> m_solverBrightened;

    /// The baseline the zone and board outline checks compare the released geometry against.
    std::optional<BLOCK_DRAG_VALIDATION> m_validation;

    /// The last offset written to the board, which is what a refused frame is undone with.
    VECTOR2I m_appliedDelta;

    /// The grabbed handle's snapping context, taken once at the start.
    BOARD_ITEM*          m_refParent = nullptr;
    GRID_HELPER_GRIDS    m_refGrid = GRID_CURRENT;
    SNAP_CONSTRAINT_TYPE m_snapConstraint = OBJECT_LAYERS;
    VECTOR2I             m_grabStart;
};


bool BLOCK_DRAG_SESSION::RunAreaPick( const TOOL_EVENT& aEvent )
{
    SELECTION_MODE mode = m_editor.m_selectionTool->GetSelectionMode();

    if( mode == SELECTION_MODE::INSIDE_LASSO || mode == SELECTION_MODE::TOUCHING_LASSO )
        return runLasso( aEvent );

    return runBox( aEvent );
}


void BLOCK_DRAG_SESSION::releaseState()
{
    // The DRC status lives in the view's preview, which nothing else in this editor
    // uses, so every exit from the gesture clears it
    m_editor.getView()->ClearPreview();

    for( BOARD_ITEM* moving : m_moving )
    {
        if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( moving ) )
        {
            shape->ClearFlags( IS_MOVING );
            shape->UpdateHatching();
        }
    }

    for( std::shared_ptr<EDIT_POINTS>& display : m_dependentDisplays )
    {
        if( m_editor.getView()->HasItem( display.get() ) )
            m_editor.getView()->Remove( display.get() );
    }

    for( PCB_SHAPE* neighbor : m_solverBrightened )
    {
        neighbor->ClearBrightened();
        m_editor.getView()->Update( neighbor );
    }

    m_solverBrightened.clear();

    if( m_editor.m_editedPoint )
        m_editor.m_editedPoint->SetActive( false );

    if( m_routerActive )
    {
        ROUTER_TOOL* routerTool = m_editor.m_toolMgr->GetTool<ROUTER_TOOL>();

        if( routerTool && routerTool->Router() )
        {
            if( routerTool->Router()->RoutingInProgress() )
                routerTool->Router()->StopRouting();

            routerTool->GetInterface()->SetIndependentPads( {} );
        }

        m_routerActive = false;
    }

    // Look items up by id, because fixing the route can replace a carried segment.
    // The selection tool hides a selected item from the board pass so the overlay can
    // draw it alone, and clearing the overlay flag clears that hiding too, so put it back
    for( const KIID& hidden : m_overlayHidden )
    {
        if( BOARD_ITEM* item = m_editor.board()->ResolveItem( hidden, true ) )
        {
            m_editor.getView()->Hide( item, false );

            if( item->IsSelected() )
                m_editor.getView()->Hide( item, true );
        }
    }

    m_overlayHidden.clear();

    m_dependentDisplays.clear();
    m_dependents.clear();
    m_targets.clear();
    m_origins.clear();
    m_rigidItems.clear();
    m_moving.clear();
    m_constraintAnchors.clear();
    m_constraintTranslated.clear();
    m_constraintSession.reset();
    m_constraintWarned = false;
    m_validation.reset();
    m_appliedDelta = VECTOR2I( 0, 0 );
    m_revertRestarts = false;
    m_active = false;
    m_keyboardDrag = false;
    m_editor.m_blockDragAbort = nullptr;
    m_grid.SetPointEditProfile( true );
    m_editor.getViewControls()->ForceCursorPosition( false );
    m_editor.getViewControls()->SetAutoPan( false );
    m_editor.frame()->UndoRedoBlock( false );
}


void BLOCK_DRAG_SESSION::Abort()
{
    if( !m_active )
        return;

    // Release the items before undoing, because the undo can replace them
    releaseState();
    m_commit.Revert();

    // No repaint, the undo can replace the items these keys point at
    m_editor.m_pickedHandles.clear();
}


bool BLOCK_DRAG_SESSION::revert( bool aPushed )
{
    bool restart = m_revertRestarts;

    releaseState();

    if( aPushed )
        m_editFrame->RollbackFromUndo();
    else
        m_commit.Revert();

    if( restart )
        m_editor.m_toolMgr->PostEvent( EVENTS::SelectedEvent );

    return restart;
}


void BLOCK_DRAG_SESSION::endDrag( bool aKeepPicks )
{
    if( m_active )
        releaseState();

    if( !aKeepPicks )
        m_editor.clearPickedHandles();

    m_editor.updatePoints();
}


bool BLOCK_DRAG_SESSION::collectTargets()
{
    m_targets.clear();

    for( EDIT_POINTS* candidate : m_editor.pickableCollections() )
    {
        EDA_ITEM* parent = candidate->GetParent();

        if( !parent )
            continue;

        bool hasMovedPoint = false;

        for( const auto& [key, origin] : m_origins )
        {
            if( key.Parent() == parent->m_Uuid )
            {
                hasMovedPoint = true;
                break;
            }
        }

        if( !hasMovedPoint )
            continue;

        POINT_EDIT_BEHAVIOR* behavior = nullptr;

        if( candidate == m_editor.m_editPoints.get() )
        {
            behavior = m_editor.m_editorBehavior.get();
        }
        else
        {
            for( PCB_POINT_EDITOR::BLOCK_HANDLE_ENTRY& entry : m_editor.m_blockHandles )
            {
                if( entry.points.get() == candidate )
                {
                    behavior = entry.behavior.get();
                    break;
                }
            }
        }

        if( !behavior )
            return false;

        m_targets.push_back( { candidate, behavior } );
    }

    return true;
}


// Write one frame. First the reshaped items, then the ones that travel whole, then the
// joined endpoints that follow, then the constraint solve over all of them. Undoing a
// frame is this same call with the last offset the gesture accepted
BLOCK_DRAG_SESSION::FRAME_RESULT BLOCK_DRAG_SESSION::applyFrame( const VECTOR2I&         aDelta,
                                                                 std::vector<EDA_ITEM*>& aUpdated )
{
    if( !BlockDragApplyDisplacement( m_editor.pickableCollections(), m_origins, aDelta, m_grid ) || !collectTargets() )
        return FRAME_RESULT::LOST_IDENTITY;

    for( auto& [targetPoints, behavior] : m_targets )
    {
        EDA_ITEM* parent = targetPoints->GetParent();
        aUpdated.push_back( parent );

        for( const auto& [key, origin] : m_origins )
        {
            if( key.Parent() != parent->m_Uuid )
                continue;

            if( EDIT_POINT* moved = key.Resolve( *targetPoints ) )
            {
                behavior->UpdateItem( *moved, *targetPoints, m_commit, aUpdated );

                // A polygon rebuilds its whole outline from the point list, so it has just
                // taken every picked handle with it. Calling it again per handle repeats
                // the same work, which is quadratic in the number of points picked
                if( behavior->UpdateItemAppliesAllPoints() )
                    break;
            }
        }
    }

    VECTOR2I frameStep = aDelta - m_appliedDelta;

    if( frameStep != VECTOR2I( 0, 0 ) )
    {
        for( BOARD_ITEM* rigidItem : m_rigidItems )
        {
            rigidItem->Move( frameStep );
            aUpdated.push_back( rigidItem );
        }
    }

    m_appliedDelta = aDelta;

    for( size_t ii = 0; ii < m_dependents.size(); ++ii )
    {
        const BLOCK_DRAG_ENDPOINT& dep = m_dependents[ii];
        VECTOR2I                   target = dep.position + aDelta;

        if( dep.follow == BLOCK_DRAG_FOLLOW::WHOLE )
        {
            // Move it by the distance its joint has left to go, so a long drag cannot
            // drift. Setting one endpoint instead would leave an arc's centre behind
            VECTOR2I at = dep.isStart ? dep.shape->GetStart() : dep.shape->GetEnd();

            dep.shape->Move( target - at );
        }
        else if( dep.isStart )
        {
            dep.shape->SetStart( target );
        }
        else
        {
            dep.shape->SetEnd( target );
        }

        aUpdated.push_back( dep.shape );

        // The two vectors line up index by index, so they are always filtered together
        m_dependentDisplays[ii]->Point( 0 ).SetPosition( target );
        m_editor.getView()->Update( m_dependentDisplays[ii].get() );
    }

    // The picked handles and the carried offset are fixed. Everything else in their
    // constraint clusters moves as little as the relations allow
    if( m_constraintSession && frameStep != VECTOR2I( 0, 0 ) )
    {
        std::vector<std::pair<CONSTRAINT_MEMBER, VECTOR2I>> pins;
        pins.reserve( m_constraintAnchors.size() );

        const PCB_SHAPE*                     anchorShape = nullptr;
        std::vector<CONSTRAINT_ANCHOR_POINT> anchors;

        // Take the position the behavior actually used, not the one the cursor asked
        // for, so a drag the behavior clamped stays clamped through the solve
        for( const auto& [shape, member] : m_constraintAnchors )
        {
            if( shape != anchorShape )
            {
                anchors = ConstraintShapeAnchors( shape );
                anchorShape = shape;
            }

            for( const CONSTRAINT_ANCHOR_POINT& anchor : anchors )
            {
                if( anchor.anchor == member.m_anchor && anchor.index == member.m_index )
                {
                    pins.emplace_back( member, anchor.pos );
                    break;
                }
            }
        }

        std::vector<PCB_SHAPE*> solverMoved;

        auto stageNeighbor = [&]( BOARD_ITEM* aItem )
        {
            m_commit.Modify( aItem );

            // A dimension that only re-measured is not reported as moved, so refresh
            // it here or it looks frozen until the drag ends
            if( aItem->Type() != PCB_SHAPE_T )
                aUpdated.push_back( aItem );
        };

        CONSTRAINT_BLOCK_RESULT solveResult =
                m_constraintSession->Solve( pins, m_constraintTranslated, aDelta, &solverMoved, stageNeighbor );

        if( solveResult == CONSTRAINT_BLOCK_RESULT::REFUSED )
            return FRAME_RESULT::CONSTRAINED;

        for( PCB_SHAPE* neighbor : solverMoved )
        {
            aUpdated.push_back( neighbor );

            // A shape the gesture carries or reshapes is already drawn with the block,
            // so only mark the ones the solve pulled along
            if( !neighbor->IsMoving() && !neighbor->IsBrightened() )
            {
                neighbor->SetBrightened();
                m_solverBrightened.insert( neighbor );
            }
        }
    }

    return FRAME_RESULT::APPLIED;
}


bool BLOCK_DRAG_SESSION::Begin( const VECTOR2I& aOrigin, bool aPressInFlight )
{
    EDIT_POINTS* grabCollection = nullptr;
    EDIT_POINT*  grabPoint = nullptr;

    for( EDIT_POINTS* candidate : m_editor.pickableCollections() )
    {
        if( EDIT_POINT* found = candidate->FindPoint( aOrigin, m_editor.getView() ) )
        {
            if( !BlockDragHandlePickable( *candidate, *found ) )
                continue;

            grabCollection = candidate;
            grabPoint = found;
            break;
        }
    }

    if( !grabPoint )
        return false;

    std::optional<PCB_HANDLE_KEY> grabKey = PCB_HANDLE_KEY::Make( *grabCollection, grabPoint );

    if( !grabKey )
        return false;

    std::set<PCB_HANDLE_KEY> priorPicks = m_editor.m_pickedHandles;
    bool                     grabPicked = m_editor.m_pickedHandles.count( *grabKey ) > 0;

    // Put the picks back if the start is declined
    auto restorePicks = [&]() -> bool
    {
        m_editor.setPickedHandles( priorPicks );
        return false;
    };

    auto refuse = [&]( const wxString& aMessage ) -> bool
    {
        m_editor.frame()->ShowInfoBarError( aMessage );
        m_refused = true;

        // Only a mouse press has the rest of a button press left to ignore. A
        // keyboard start sets no flag here, so a refusal cannot eat the next click
        if( aPressInFlight )
            m_pressConsumed = true;

        return restorePicks();
    };

    if( grabCollection == m_editor.m_editPoints.get() )
    {
        // A lone object's handles keep the normal drag, unless the handle grabbed is
        // part of a bigger picked set
        if( !grabPicked || m_editor.m_pickedHandles.size() < 2 )
            return false;
    }
    else if( !grabPicked )
    {
        // Dragging a handle that is not picked picks just that one first
        std::set<PCB_HANDLE_KEY> picked = m_editor.m_pickedHandles;

        BlockDragApplyPick( picked, *grabKey, false );
        m_editor.setPickedHandles( std::move( picked ) );
    }

    // From here on the selection tool sees none of this drag, whether the gesture
    // starts or is refused, so stop its long press timer here
    m_editor.m_selectionTool->StopDisambiguation();

    // Refuse the whole gesture, so no selected item is left out without the user knowing
    std::vector<BOARD_ITEM*> routerItems;

    for( BOARD_ITEM* selectedItem : m_editor.m_expandedSelection )
    {
        BLOCK_DRAG_CAPABILITY capability = BlockDragCapability( *selectedItem );

        if( capability == BLOCK_DRAG_CAPABILITY::UNSUPPORTED )
        {
            return refuse( wxString::Format( _( "%s cannot be dragged." ),
                                             selectedItem->GetItemDescription( m_editor.frame(), false ) ) );
        }

        if( capability == BLOCK_DRAG_CAPABILITY::ROUTER )
            routerItems.push_back( selectedItem );
    }

    m_origins = BlockDragMoveOrigins( m_editor.pickableCollections(), m_editor.m_pickedHandles );

    if( m_origins.empty() || !collectTargets() )
        return restorePicks();

    bool overrideLocksNow = m_editFrame->GetOverrideLocks();

    for( auto& [targetPoints, behavior] : m_targets )
    {
        BOARD_ITEM* parent = static_cast<BOARD_ITEM*>( targetPoints->GetParent() );

        if( ConstraintItemIsLocked( parent ) && !overrideLocksNow )
            return refuse( wxString::Format( _( "%s is locked. Enable 'Override locks' to move it." ),
                                             parent->GetItemDescription( m_editor.frame(), false ) ) );
    }

    // A selected item with no picked handle travels whole, as it would under the move
    // tool. Picking says which handles stay with the cursor, selecting says what moves
    m_rigidItems.clear();

    // The items taking part in the gesture. The dependent search skips these
    std::unordered_set<const BOARD_ITEM*> blockParticipants;

    for( auto& [targetPoints, behavior] : m_targets )
        blockParticipants.insert( static_cast<BOARD_ITEM*>( targetPoints->GetParent() ) );

    for( BOARD_ITEM* selectedItem : m_editor.m_expandedSelection )
    {
        // An item with a picked handle reshapes instead, and its lock was checked above
        if( blockParticipants.count( selectedItem ) )
            continue;

        if( ConstraintItemIsLocked( selectedItem ) && !overrideLocksNow )
        {
            return refuse( wxString::Format( _( "%s is locked. Enable 'Override locks' to move it." ),
                                             selectedItem->GetItemDescription( m_editor.frame(), false ) ) );
        }

        blockParticipants.insert( selectedItem );

        // Copper travels in the router session. A footprint also travels whole on
        // the board side, and only its copper becomes router solids
        if( BlockDragCapability( *selectedItem ) == BLOCK_DRAG_CAPABILITY::ROUTER )
        {
            if( selectedItem->Type() == PCB_FOOTPRINT_T )
                m_rigidItems.push_back( selectedItem );

            continue;
        }

        m_rigidItems.push_back( selectedItem );
    }

    // The ends of everything the block moves are the starting points for finding
    // graphics that should follow. A carried arc seeds from both of its ends
    std::vector<BLOCK_DRAG_ENDPOINT> movedEndpoints;

    for( auto& [targetPoints, behavior] : m_targets )
    {
        PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( targetPoints->GetParent() );

        if( !shape || shape->GetShape() != SHAPE_T::SEGMENT )
            continue;

        for( const auto& [key, origin] : m_origins )
        {
            if( key.Parent() != shape->m_Uuid || key.IsLine() )
                continue;

            if( key.Index() == 0 || key.Index() == 1 )
                movedEndpoints.push_back( { shape, key.Index() == 0, origin } );
        }
    }

    for( BOARD_ITEM* rigidItem : m_rigidItems )
    {
        PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( rigidItem );

        if( !shape )
            continue;

        // A carried arc joins the chain at both of its ends, so selecting the arc of
        // a filleted corner keeps its far joint together
        if( shape->GetShape() != SHAPE_T::SEGMENT && shape->GetShape() != SHAPE_T::ARC )
            continue;

        movedEndpoints.push_back( { shape, true, shape->GetStart(), BLOCK_DRAG_FOLLOW::WHOLE } );
        movedEndpoints.push_back( { shape, false, shape->GetEnd(), BLOCK_DRAG_FOLLOW::WHOLE } );
    }

    m_dependents.clear();

    if( !movedEndpoints.empty() )
    {
        std::vector<PCB_SHAPE*> candidates;

        // An item the gesture already moves must not also be pulled by one endpoint
        auto addCandidate = [&]( BOARD_ITEM* aItem )
        {
            if( aItem->Type() != PCB_SHAPE_T )
                return;

            if( blockParticipants.count( aItem ) )
                return;

            candidates.push_back( static_cast<PCB_SHAPE*>( aItem ) );
        };

        if( m_editor.m_isFootprintEditor )
        {
            if( FOOTPRINT* footprint = m_editor.board()->GetFirstFootprint() )
            {
                for( BOARD_ITEM* drawing : footprint->GraphicalItems() )
                    addCandidate( drawing );
            }
        }
        else
        {
            for( BOARD_ITEM* drawing : m_editor.board()->Drawings() )
                addCandidate( drawing );
        }

        if( BlockDragFindDependents( candidates, movedEndpoints, overrideLocksNow, m_dependents )
            == BLOCK_DRAG_DEPENDENT_RESULT::LOCKED )
        {
            return refuse( _( "A locked connected item prevents the drag. Enable 'Override locks' to move it." ) );
        }
    }

    // Undoing swaps in fresh footprint children, so a pad selected together with
    // its footprint gets a new object
    bool padOfCarriedFootprint = false;

    // Start the router session after every refusal, so no refusal has to unwind it
    if( !routerItems.empty() )
    {
        ROUTER_TOOL* routerTool = m_editor.m_toolMgr->GetTool<ROUTER_TOOL>();

        if( !routerTool || !routerTool->Router() || routerTool->IsToolActive()
            || routerTool->Router()->RoutingInProgress() )
        {
            return refuse( _( "The router is busy." ) );
        }

        // The router tool only rebinds its interface when it is reset to run, so after
        // a board reload the borrowed interface still points at the old board
        routerTool->GetInterface()->SetBoard( m_editor.board() );
        routerTool->GetInterface()->SetView( m_editor.getView() );

        PNS::ROUTER* router = routerTool->Router();
        router->SyncWorld();

        PNS::NODE*                     world = router->GetWorld();
        PNS::ITEM_SET                  routerSet;
        std::set<PAD*>                 independentPads;
        std::unordered_set<FOOTPRINT*> carriedFootprints;

        for( BOARD_ITEM* routerItem : routerItems )
        {
            if( routerItem->Type() == PCB_FOOTPRINT_T )
                carriedFootprints.insert( static_cast<FOOTPRINT*>( routerItem ) );
        }

        for( BOARD_ITEM* routerItem : routerItems )
        {
            if( routerItem->Type() == PCB_FOOTPRINT_T )
            {
                FOOTPRINT* footprint = static_cast<FOOTPRINT*>( routerItem );

                // A footprint with no copper is not in the router world, so it
                // travels on the board side only
                for( PAD* pad : footprint->Pads() )
                {
                    for( PNS::ITEM* solid : world->FindItemsByParent( pad ) )
                        routerSet.Add( solid );
                }

                for( ZONE* zone : footprint->Zones() )
                {
                    for( PNS::ITEM* solid : world->FindItemsByParent( zone ) )
                        routerSet.Add( solid );
                }

                for( BOARD_ITEM* graphic : footprint->GraphicalItems() )
                {
                    if( graphic->GetLayer() == Edge_Cuts || graphic->GetLayer() == Margin
                        || IsCopperLayer( graphic->GetLayer() ) )
                    {
                        for( PNS::ITEM* solid : world->FindItemsByParent( graphic ) )
                            routerSet.Add( solid );
                    }
                }

                continue;
            }

            if( routerItem->Type() == PCB_PAD_T )
            {
                PAD* pad = static_cast<PAD*>( routerItem );

                if( carriedFootprints.count( pad->GetParentFootprint() ) )
                {
                    padOfCarriedFootprint = true;
                    continue;
                }

                // A normal move turns a lone pad into its whole footprint unless free
                // pads are allowed, and a block cannot carry an unselected footprint
                if( !m_editor.frame()->GetPcbNewSettings()->m_AllowFreePads )
                {
                    return refuse( wxString::Format( _( "%s cannot be dragged." ),
                                                     pad->GetItemDescription( m_editor.frame(), false ) ) );
                }

                // The solver only moves a pad with its footprint, so it cannot follow one the router moves alone
                if( BlockDragIsConstraintMember( *pad ) )
                {
                    return refuse( wxString::Format( _( "%s cannot be dragged." ),
                                                     pad->GetItemDescription( m_editor.frame(), false ) ) );
                }

                // Keeps the router from moving the pad's whole footprint
                independentPads.insert( pad );
            }

            std::vector<PNS::ITEM*> pnsItems = world->FindItemsByParent( routerItem );

            if( pnsItems.empty() )
            {
                return refuse( wxString::Format( _( "%s cannot be dragged." ),
                                                 routerItem->GetItemDescription( m_editor.frame(), false ) ) );
            }

            for( PNS::ITEM* pnsItem : pnsItems )
                routerSet.Add( pnsItem );
        }

        // A generator moves its copper on the board side, so the router world has to
        // move the same copper. Otherwise the router reroutes stale copies over it
        for( BOARD_ITEM* rigidItem : m_rigidItems )
        {
            if( rigidItem->Type() != PCB_GENERATOR_T )
                continue;

            for( BOARD_ITEM* member : static_cast<PCB_GENERATOR*>( rigidItem )->GetBoardItems() )
            {
                for( PNS::ITEM* pnsItem : world->FindItemsByParent( member ) )
                    routerSet.Add( pnsItem );
            }
        }

        if( routerSet.Size() > 0 )
        {
            routerTool->GetInterface()->SetIndependentPads( independentPads );

            if( !router->StartDragging( grabPoint->GetPosition(), routerSet, PNS::DM_BLOCK ) )
            {
                routerTool->GetInterface()->SetIndependentPads( {} );
                return refuse( _( "The copper in the selection cannot be dragged." ) );
            }

            m_routerActive = true;

            // The dragger leaves an arc with both ends pinned where it is. Name those for
            // the user, the way the router tool's own drag does, and keep them for the
            // hiding below
            std::set<KIID> blockAnchoredCopper;

            if( auto* blockDragger = dynamic_cast<PNS::BLOCK_DRAGGER*>( router->GetDragger() ) )
            {
                const std::vector<PNS::LINE>& held = blockDragger->AnchoredLines();

                if( !held.empty() )
                {
                    m_editor.frame()->ShowInfoBarWarning( wxString::Format(
                            wxPLURAL( "%zu selected arc cannot move because both ends are fixed.",
                                      "%zu selected arcs cannot move because both ends are fixed.", held.size() ),
                            held.size() ) );
                }

                for( const PNS::LINE& line : held )
                {
                    for( const PNS::LINKED_ITEM* link : line.Links() )
                    {
                        if( const BOARD_ITEM* parent = link->Parent() )
                            blockAnchoredCopper.insert( parent->m_Uuid );
                    }
                }
            }

            // The router keeps carried copper at its board position until the route is
            // fixed, and the selection overlay paints a selected item from that position.
            // Hide the overlay copy while the router preview stands in for it. The overlay
            // flag only takes effect while the item is still visible, and the first move
            // below hides it
            for( BOARD_ITEM* routerItem : routerItems )
            {
                if( routerItem->Type() == PCB_FOOTPRINT_T )
                    continue;

                // An arc the dragger anchored gets no preview, so hiding it would leave
                // it drawn by nobody until the gesture ends
                if( blockAnchoredCopper.count( routerItem->m_Uuid ) )
                    continue;

                m_editor.getView()->Hide( routerItem, true, true );
                m_overlayHidden.push_back( routerItem->m_Uuid );
            }

            // The first move hands the carried copper over to the router preview, so
            // the first snap cannot land on copper that is about to move
            router->Move( grabPoint->GetPosition(), nullptr );
        }
    }

    m_refParent = static_cast<BOARD_ITEM*>( grabCollection->GetParent() );
    m_refGrid = m_grid.GetItemGrid( m_refParent );
    m_snapConstraint = grabPoint->GetSnapConstraint();
    m_grabStart = grabPoint->GetPosition();

    // Keep everything that moves with the cursor out of the snap targets, the
    // way the move tool does, or the block chases its own geometry
    m_moving = m_rigidItems;

    for( auto& [targetPoints, behavior] : m_targets )
    {
        EDA_ITEM* parent = targetPoints->GetParent();
        m_moving.push_back( static_cast<BOARD_ITEM*>( parent ) );
        m_commit.Modify( parent );
    }

    for( BOARD_ITEM* rigidItem : m_rigidItems )
    {
        // A generator moves its members with it, so each member needs its own undo copy
        m_commit.Modify( rigidItem, nullptr, RECURSE_MODE::RECURSE );
    }

    m_dependentDisplays.clear();

    for( const BLOCK_DRAG_ENDPOINT& dep : m_dependents )
    {
        m_commit.Modify( dep.shape );

        // A segment joined to the block at both ends shows up twice
        if( std::find( m_moving.begin(), m_moving.end(), dep.shape ) == m_moving.end() )
            m_moving.push_back( dep.shape );

        std::shared_ptr<EDIT_POINTS> display = std::make_shared<EDIT_POINTS>( dep.shape );
        display->AddPoint( dep.position );
        display->Point( 0 ).SetDependent();
        m_editor.getView()->Add( display.get() );
        m_dependentDisplays.push_back( std::move( display ) );
    }

    // Flag them from the same list the release clears, so the two always match
    for( BOARD_ITEM* moving : m_moving )
    {
        if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( moving ) )
        {
            shape->SetFlags( IS_MOVING );
            shape->UpdateHatching();
        }
    }

    // The solver can move an item that shares a constraint but was never dragged, so
    // validation has to watch it. Keep it out of the moving set, which is what lets it
    // stay a snap target
    std::vector<BOARD_ITEM*> validationScope = m_moving;

    // Everything the gesture writes is fixed for the solver. A picked handle ends where
    // the user put it and a carried item ends at its own offset, while the rest of their
    // constraint clusters is free to move
    if( BoardHasConstraints( m_editor.board() ) )
    {
        std::vector<BOARD_ITEM*>              blockConstrainedItems;
        std::unordered_set<const BOARD_ITEM*> seeded;

        auto seedConstrained = [&]( BOARD_ITEM* aItem )
        {
            if( !IsConstrainableItem( aItem ) || !seeded.insert( aItem ).second )
                return;

            if( BlockDragIsConstraintMember( *aItem ) )
                blockConstrainedItems.push_back( aItem );
        };

        for( auto& [targetPoints, behavior] : m_targets )
        {
            PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( targetPoints->GetParent() );

            if( !shape )
                continue;

            for( const auto& [key, origin] : m_origins )
            {
                if( key.Parent() != shape->m_Uuid )
                    continue;

                EDIT_POINT* picked = key.Resolve( *targetPoints );

                if( !picked )
                    continue;

                if( std::optional<CONSTRAINT_MEMBER> member =
                            constraintMemberForEditPoint( shape, targetPoints, picked ) )
                {
                    m_constraintAnchors.emplace_back( shape, *member );
                }
            }

            seedConstrained( shape );
        }

        // A carried container moves its graphics with it, so pin their offset too, or
        // the solve pulls them back out of the container
        std::unordered_set<BOARD_ITEM*> walked;

        std::function<void( BOARD_ITEM* )> carryWhole = [&]( BOARD_ITEM* aItem )
        {
            if( !aItem || !walked.insert( aItem ).second )
                return;

            m_constraintTranslated.insert( aItem->m_Uuid );
            seedConstrained( aItem );
            aItem->RunOnChildren( carryWhole, RECURSE_MODE::NO_RECURSE );
        };

        for( BOARD_ITEM* rigidItem : m_rigidItems )
            carryWhole( rigidItem );

        for( const BLOCK_DRAG_ENDPOINT& dep : m_dependents )
        {
            // The gesture moves a whole arc by the block offset, so the solver must carry all of it
            if( dep.follow == BLOCK_DRAG_FOLLOW::WHOLE )
            {
                carryWhole( dep.shape );
                continue;
            }

            // The gesture already placed it, so it pins its joint and does not move again
            CONSTRAINT_ANCHOR anchor = dep.isStart ? CONSTRAINT_ANCHOR::START : CONSTRAINT_ANCHOR::END;

            m_constraintAnchors.emplace_back( dep.shape, CONSTRAINT_MEMBER( dep.shape->m_Uuid, anchor ) );
            seedConstrained( dep.shape );
        }

        if( !blockConstrainedItems.empty() )
        {
            m_constraintSession.emplace();

            if( !m_constraintSession->Build( m_editor.board(), blockConstrainedItems ) )
                m_constraintSession.reset();
        }
    }

    blockDragAppendConstraintCoMembers( m_editor.board(), validationScope );

    m_validation.emplace( *m_editor.board() );
    m_validation->CaptureBaseline( validationScope );

    m_appliedDelta = VECTOR2I( 0, 0 );
    m_revertRestarts = padOfCarriedFootprint;

    if( !m_editor.m_isFootprintEditor )
    {
        // Use the wider scope, because the solver reaches items the gesture never
        // touched, and undoing a footprint child gives that footprint all new
        // children, which leaves every pointer held here dead
        for( BOARD_ITEM* moving : validationScope )
        {
            if( moving->GetParentFootprint() )
                m_revertRestarts = true;
        }
    }

    // The block is not a single point edit, so the grabbed item's own geometry
    // must not act as a snap target
    m_grid.SetPointEditProfile( false );
    m_editor.frame()->UndoRedoBlock( true );
    m_editor.getViewControls()->SetAutoPan( true );
    m_active = true;

    // A start with no button down is the keyboard one, which the cursor then follows freely
    m_keyboardDrag = !aPressInFlight;
    m_editor.m_blockDragAbort = [this]()
    {
        Abort();
    };
    return true;
}


bool BLOCK_DRAG_SESSION::AreaPickStarts( const TOOL_EVENT& aEvent )
{
    if( !m_editor.blockAnchorEditingEnabled() )
        return false;

    int modifiers = aEvent.Modifier( MD_SHIFT | MD_CTRL | MD_ALT );

    if( modifiers != 0 && modifiers != MD_SHIFT )
        return false;

    bool anyPickable = false;

    for( EDIT_POINTS* candidate : m_editor.pickableCollections() )
    {
        if( candidate->FindPoint( aEvent.DragOrigin(), m_editor.getView() ) )
            return false;

        if( anyPickable )
            continue;

        for( unsigned ii = 0; ii < candidate->PointsSize(); ++ii )
        {
            if( BlockDragHandlePickable( *candidate, candidate->Point( ii ) ) )
            {
                anyPickable = true;
                break;
            }
        }
    }

    if( !anyPickable )
        return false;

    // Only empty canvas starts the gesture. A drag on an item still moves it
    return !m_editor.m_selectionTool->SelectionContains( aEvent.DragOrigin() );
}


KIGFX::COLOR4D BLOCK_DRAG_SESSION::bandColor() const
{
    KIGFX::RENDER_SETTINGS* settings = m_editor.getView()->GetPainter()->GetSettings();
    KIGFX::COLOR4D          color = settings->GetLayerColor( LAYER_AUX_ITEMS );

    if( m_editor.getView()->GetGAL()->GetClearColor().Distance( color ) < 0.5 )
        color.Invert();

    return color;
}


void BLOCK_DRAG_SESSION::showBand( HANDLE_PICK_AREA& aArea )
{
    // The selection tool never sees this drag, so stop its long press timer here
    m_editor.m_selectionTool->StopDisambiguation();

    KIGFX::COLOR4D color = bandColor();

    aArea.SetStrokeColor( color.WithAlpha( 0.9 ) );
    aArea.SetFillColor( color.WithAlpha( 0.15 ) );
    m_editor.getView()->Add( &aArea );
    m_editor.getView()->SetVisible( &aArea, true );
    m_editor.getViewControls()->SetAutoPan( true );
}


void BLOCK_DRAG_SESSION::hideBand( HANDLE_PICK_AREA& aArea )
{
    m_editor.getViewControls()->SetAutoPan( false );
    m_editor.getView()->SetVisible( &aArea, false );
    m_editor.getView()->Remove( &aArea );
}


bool BLOCK_DRAG_SESSION::runLasso( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW*     view = m_editor.getView();
    HANDLE_PICK_AREA area;

    showBand( area );

    // aEvent points at the tool's one event slot, which every Wait() below overwrites
    bool accumulate = aEvent.Modifier( MD_SHIFT ) != 0;
    bool finished = false;
    bool exitLoop = false;

    SHAPE_LINE_CHAIN points;
    points.SetClosed( true );
    points.Append( aEvent.DragOrigin() );

    while( TOOL_EVENT* lassoEvt = m_editor.Wait() )
    {
        if( lassoEvt->IsCancelInteractive() )
        {
            lassoEvt->SetPassEvent( false );
            break;
        }
        else if( lassoEvt->IsActivate() )
        {
            // Pass it on so the new tool starts, as a normal handle drag does
            exitLoop = !lassoEvt->IsMoveTool();
            break;
        }
        else if( lassoEvt->IsSelectionEvent() || lassoEvt->Matches( EVENTS::InhibitSelectionEditing ) )
        {
            exitLoop = true;
            break;
        }
        else if( lassoEvt->IsDrag( BUT_LEFT ) || lassoEvt->IsClick( BUT_LEFT )
                 || lassoEvt->IsAction( &ACTIONS::cursorClick ) )
        {
            points.Append( lassoEvt->Position() );
        }
        else if( lassoEvt->IsDblClick( BUT_LEFT ) || lassoEvt->IsAction( &ACTIONS::cursorDblClick )
                 || lassoEvt->IsAction( &ACTIONS::finishInteractive ) )
        {
            finished = true;
            lassoEvt->SetPassEvent( false );
            break;
        }
        else if( lassoEvt->IsAction( &ACTIONS::deleteLastPoint ) || lassoEvt->IsAction( &ACTIONS::doDelete )
                 || lassoEvt->IsAction( &ACTIONS::undo ) )
        {
            if( points.GetPointCount() > 0 )
            {
                m_editor.getViewControls()->SetCursorPosition( points.CLastPoint() );
                points.Remove( points.GetPointCount() - 1 );
            }
        }
        else
        {
            // Never pass a mouse down on, it would arm the long press timer again
            blockDragPassNavigation( lassoEvt );
        }

        area.SetPoly( points );
        area.GetPoly().Append( m_editor.m_toolMgr->GetMousePosition() );
        view->Update( &area );
    }

    hideBand( area );

    if( finished )
    {
        std::set<PCB_HANDLE_KEY> picked = m_editor.m_pickedHandles;

        BlockDragLassoPick( m_editor.pickableCollections(), points, accumulate, picked );
        m_editor.setPickedHandles( std::move( picked ) );
    }

    return exitLoop;
}


bool BLOCK_DRAG_SESSION::runBox( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW* view = m_editor.getView();

    // aEvent points at the tool's one event slot, which every Wait() below
    // overwrites, so copy out everything needed from it now
    VECTOR2I origin = aEvent.DragOrigin();
    VECTOR2I end = aEvent.Position();
    bool     accumulate = aEvent.Modifier( MD_SHIFT ) != 0;
    bool     released = false;
    bool     exitLoop = false;

    HANDLE_PICK_AREA area;
    area.SetPoly( BlockDragPickRect( origin, end ) );

    showBand( area );

    while( TOOL_EVENT* boxEvt = m_editor.Wait() )
    {
        if( boxEvt->IsCancelInteractive() )
        {
            boxEvt->SetPassEvent( false );
            break;
        }
        else if( boxEvt->IsActivate() )
        {
            exitLoop = !boxEvt->IsMoveTool();
            break;
        }
        else if( boxEvt->IsSelectionEvent() || boxEvt->Matches( EVENTS::InhibitSelectionEditing ) )
        {
            exitLoop = true;
            break;
        }
        else if( boxEvt->IsDrag( BUT_LEFT ) )
        {
            end = boxEvt->Position();
            area.SetPoly( BlockDragPickRect( origin, end ) );
            view->Update( &area );
        }
        else if( boxEvt->IsMouseUp( BUT_LEFT ) )
        {
            released = true;
            break;
        }
        else
        {
            // Never pass a mouse down on, it would arm the long press timer again
            blockDragPassNavigation( boxEvt );
        }
    }

    hideBand( area );

    if( released )
    {
        std::set<PCB_HANDLE_KEY> picked = m_editor.m_pickedHandles;

        BlockDragLassoPick( m_editor.pickableCollections(), BlockDragPickRect( origin, end ), accumulate, picked );
        m_editor.setPickedHandles( std::move( picked ) );
    }

    return exitLoop;
}


bool BLOCK_DRAG_SESSION::ApplyCursor( const TOOL_EVENT& aEvent )
{
    VECTOR2I pos = GetClampedCoords<double, int>( aEvent.Position(), PCB_POINT_EDITOR::COORDS_PADDING );
    LSET     snapLayers;

    switch( m_snapConstraint )
    {
    case IGNORE_SNAPS: break;
    case OBJECT_LAYERS: snapLayers = m_refParent->GetLayerSet(); break;
    case ALL_LAYERS: snapLayers = LSET::AllLayersMask(); break;
    }

    VECTOR2I snapped =
            m_grid.ResolveSnap( pos, snapLayers, m_refGrid, m_moving, m_grabStart + m_appliedDelta ).position;
    VECTOR2I delta = snapped - m_grabStart;
    VECTOR2I accepted = m_appliedDelta;

    // Ctrl while moving does what it does in a single point drag, which is to
    // change how a polygon or zone edge behaves. Ctrl at the release is the
    // router's force placement, so the two are read at different times
    POLYGON_LINE_MODE edgeMode =
            aEvent.Modifier( MD_CTRL ) ? POLYGON_LINE_MODE::FIXED_LENGTH : POLYGON_LINE_MODE::CONVERGING;

    for( const auto& [key, origin] : m_origins )
    {
        if( !key.IsLine() )
            continue;

        for( EDIT_POINTS* collection : m_editor.pickableCollections() )
        {
            EDIT_LINE* line = dynamic_cast<EDIT_LINE*>( key.Resolve( *collection ) );

            if( !line )
                continue;

            if( POLYGON_EDGE_DRAG_POLICY* policy = line->GetDragPolicy() )
            {
                if( policy->GetMode() != edgeMode )
                    policy->SetMode( edgeMode );
            }

            break;
        }
    }

    std::vector<EDA_ITEM*> updatedItems;
    FRAME_RESULT           frameResult = applyFrame( delta, updatedItems );

    if( frameResult == FRAME_RESULT::CONSTRAINED )
    {
        // Say it once, not once per frame, because the block keeps resisting for as
        // long as the cursor stays somewhere with no solution
        if( !m_constraintWarned )
        {
            m_editor.frame()->ShowInfoBarError( _( "The move would break a geometric constraint." ) );
            m_constraintWarned = true;
        }

        m_grid.ClearSnapFeedback();

        frameResult = applyFrame( accepted, updatedItems ) == FRAME_RESULT::LOST_IDENTITY ? FRAME_RESULT::LOST_IDENTITY
                                                                                          : FRAME_RESULT::APPLIED;
    }

    if( frameResult == FRAME_RESULT::LOST_IDENTITY )
    {
        // A picked handle has gone mid gesture. Put the geometry back and let the
        // rest of this button press pass, so the user does not start a new gesture
        // from an empty pick set. Read the flag before the teardown clears it
        bool pressInFlight = !m_keyboardDrag;

        if( revert( false ) )
            return true;

        endDrag( false );
        m_pressConsumed = pressInFlight;
    }
    else
    {
        for( EDA_ITEM* updatedItem : updatedItems )
            m_editor.getView()->Update( updatedItem );

        if( m_routerActive && m_appliedDelta != accepted )
        {
            if( ROUTER_TOOL* routerTool = m_editor.m_toolMgr->GetTool<ROUTER_TOOL>() )
                routerTool->Router()->Move( m_grabStart + m_appliedDelta, nullptr );
        }

        // Say it while the cursor is still down, so the user hears why before the
        // release throws the gesture away
        if( m_routerActive )
        {
            ROUTER_TOOL*    routerTool = m_editor.m_toolMgr->GetTool<ROUTER_TOOL>();
            PNS::DRAG_ALGO* dragger = routerTool ? routerTool->Router()->GetDragger() : nullptr;
            bool            dragStatus = true;

            // When the router is set to allow violations the release takes them anyway,
            // so a DRC warning here would ask the user for a Ctrl they do not need
            bool allowed = routerTool && routerTool->Router()->Settings().AllowDRCViolations();

            m_editor.getView()->ClearPreview();

            if( !allowed && dragger && dragger->GetForceMarkObstaclesMode( &dragStatus ) && !dragStatus )
            {
                wxString hint;
                hint.Printf( _( "(%s to commit anyway.)" ), KeyNameFromKeyCode( MD_CTRL + PSEUDO_WXK_CLICK ) );

                ROUTER_STATUS_VIEW_ITEM* statusItem = new ROUTER_STATUS_VIEW_ITEM();
                statusItem->SetMessage( _( "The move violates DRC." ) );
                statusItem->SetHint( hint );
                statusItem->SetPosition( m_editor.m_toolMgr->GetMousePosition() );
                m_editor.getView()->AddToPreview( statusItem );
            }
        }

        m_editor.getViewControls()->ForceCursorPosition( true, m_grabStart + m_appliedDelta );
        m_editor.updatePoints();
    }

    return false;
}


bool BLOCK_DRAG_SESSION::refuseRelease( const wxString& aMessage )
{
    m_editor.frame()->ShowInfoBarError( aMessage );

    if( revert( false ) )
        return true;

    endDrag( true );
    return false;
}


bool BLOCK_DRAG_SESSION::Release( const TOOL_EVENT& aEvent )
{
    // This click stops here, so the selection tool only ever sees the press that
    // armed its long press timer
    m_editor.m_selectionTool->StopDisambiguation();

    if( m_appliedDelta == VECTOR2I( 0, 0 ) )
    {
        // A gesture that changed nothing leaves no undo entry
        if( revert( false ) )
            return true;

        endDrag( true );
    }
    else
    {
        auto originResolves = [&]( const std::pair<const PCB_HANDLE_KEY, VECTOR2I>& aOrigin )
        {
            return std::ranges::any_of( m_targets,
                                        [&]( const auto& aTarget )
                                        {
                                            return aOrigin.first.Resolve( *aTarget.first ) != nullptr;
                                        } );
        };

        // A picked handle gone by now would leave part of the block unfinalized
        if( !collectTargets() || !std::ranges::all_of( m_origins, originResolves ) )
        {
            if( revert( false ) )
                return true;

            endDrag( false );
            return false;
        }

        // Finalize first, so the geometry that was checked is the geometry that goes in
        for( auto& [targetPoints, behavior] : m_targets )
            behavior->FinalizeItem( *targetPoints, m_commit );

        BLOCK_DRAG_VALIDATION_RESULT validationResult = m_validation->Validate();

        std::vector<ZONE*> zonesToFill;

        auto appendFillZone = [&]( EDA_ITEM* aItem )
        {
            if( aItem->Type() != PCB_ZONE_T )
                return;

            // A rule area has no fill, and filling nothing still rebuilds connectivity
            // and every zone's caches
            if( !static_cast<ZONE*>( aItem )->GetIsRuleArea() )
                zonesToFill.push_back( static_cast<ZONE*>( aItem ) );
        };

        // The collections were just resolved, and the footprint editor does no zone filling
        if( !m_editor.m_isFootprintEditor )
        {
            for( auto& [targetPoints, behavior] : m_targets )
                appendFillZone( targetPoints->GetParent() );

            for( BOARD_ITEM* rigidItem : m_rigidItems )
                appendFillZone( rigidItem );
        }

        bool routerSessionOk = true;
        bool routerStatusOk = true;
        bool forceCommit = aEvent.Modifier( MD_CTRL );

        if( m_routerActive )
        {
            ROUTER_TOOL*    routerTool = m_editor.m_toolMgr->GetTool<ROUTER_TOOL>();
            PNS::DRAG_ALGO* dragger = routerTool ? routerTool->Router()->GetDragger() : nullptr;

            if( !dragger )
                routerSessionOk = false;
            else if( routerTool->Router()->Settings().AllowDRCViolations() )
                routerStatusOk = true;
            else
                dragger->GetForceMarkObstaclesMode( &routerStatusOk );
        }

        if( validationResult != BLOCK_DRAG_VALIDATION_RESULT::OK )
        {
            if( refuseRelease( validationResult == BLOCK_DRAG_VALIDATION_RESULT::INVALID_ZONE
                                       ? _( "The move would break a zone outline." )
                                       : _( "The move would break the board outline." ) ) )
            {
                return true;
            }
        }
        else if( !routerSessionOk || ( !routerStatusOk && !forceCommit ) )
        {
            // Forced placement skips the collision refusal and nothing else
            if( refuseRelease( blockDragDrcRefusal() ) )
                return true;
        }
        else
        {
            wxString label;

            if( m_routerActive || !m_rigidItems.empty() || !m_dependents.empty() || m_targets.size() > 1 )
                label = _( "Drag block" );
            else
                label = m_origins.size() > 1 ? _( "Move Points" ) : _( "Move Point" );

            // The carried changes name the undo entry, and the router and the fill add to
            // it. Only the gesture's last push may arm the refill scheduler. An earlier one
            // would run a fill, a connectivity rebuild and maybe a progress dialog inside
            // that push, while the router drag is still live
            m_commit.Push( label, m_routerActive ? ZONE_FILL_OP : 0 );

            bool routerCommitted = true;

            if( m_routerActive )
            {
                ROUTER_TOOL* routerTool = m_editor.m_toolMgr->GetTool<ROUTER_TOOL>();
                routerCommitted = false;

                if( routerTool )
                {
                    routerTool->GetInterface()->SetCommitFlags( APPEND_UNDO );
                    routerCommitted = routerTool->Router()->FixRoute( VECTOR2I(), nullptr, false, forceCommit );
                    routerTool->GetInterface()->SetCommitFlags( 0 );
                }
            }

            if( !routerCommitted )
            {
                m_editor.frame()->ShowInfoBarError( blockDragDrcRefusal() );

                if( revert( true ) )
                    return true;
            }
            else if( !zonesToFill.empty() && m_routerActive && m_editFrame->GetPcbNewSettings()->m_AutoRefillZones )
            {
                // Dragging an outline leaves its zone unfilled. The router suppresses the
                // automatic refill, so do it here instead. With the refill off, nothing refills
                BOARD_COMMIT fillCommit( m_editFrame );
                ZONE_FILLER  filler( m_editor.board(), &fillCommit );

                if( filler.Fill( zonesToFill ) )
                {
                    m_editor.board()->OnZonesFilled( zonesToFill );

                    if( ZONE_FILLER_TOOL* zoneFillerTool = m_editor.m_toolMgr->GetTool<ZONE_FILLER_TOOL>() )
                        zoneFillerTool->RegenerateDirtyGenerators( fillCommit );

                    fillCommit.Push( label, APPEND_UNDO | ZONE_FILL_OP );
                }
                else
                {
                    // The gesture's undo entry holds a copy of every zone the fill touched from
                    // before the drag, so one undo takes the fill out along with the move
                    m_editor.frame()->ShowInfoBarError( _( "Zone fill failed, the move was undone." ) );

                    if( revert( true ) )
                        return true;
                }
            }

            // A reshaped generator keeps its members where they were while the drag
            // runs and re-places them once the outline is final, as Move() leaves them
            // for the next Update(). It comes after the router's push, so the router
            // never meets a via this replaces, and joins the same undo entry. A revert
            // above has already ended the gesture
            if( GENERATOR_TOOL* genTool = m_active ? m_editor.m_toolMgr->GetTool<GENERATOR_TOOL>() : nullptr )
            {
                BOARD_COMMIT genCommit( m_editFrame );
                bool         regenerated = false;

                // A reshaped one is dirty from its handles, a carried one from Move()
                for( BOARD_ITEM* moving : m_moving )
                {
                    if( moving->Type() != PCB_GENERATOR_T )
                        continue;

                    PCB_GENERATOR* gen = static_cast<PCB_GENERATOR*>( moving );

                    if( !gen->IsDirty() )
                        continue;

                    gen->EditStart( genTool, m_editor.board(), &genCommit );
                    gen->Update( genTool, m_editor.board(), &genCommit );
                    gen->EditFinish( genTool, m_editor.board(), &genCommit );
                    regenerated = true;
                }

                if( regenerated )
                    genCommit.Push( label, APPEND_UNDO | ZONE_FILL_OP );
            }

            endDrag( true );
        }
    }

    return false;
}


bool BLOCK_DRAG_SESSION::Cancel( TOOL_EVENT& aEvent )
{
    // Escape cancels the gesture and nothing else. The picks and the object
    // selection stay, and the rest of this button press is ignored. A tool
    // switch and the other ways of cancelling keep their usual effect
    bool escape = aEvent.IsCancel() && aEvent.GetCommandId() == (int) WXK_ESCAPE;

    // Read the flag before the teardown clears it
    bool pressInFlight = !m_keyboardDrag;

    if( escape )
        aEvent.SetPassEvent( false );

    if( revert( false ) )
        return true;

    endDrag( escape );

    if( escape )
        m_pressConsumed = pressInFlight;

    return false;
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
    bool        overrideLocks = editFrame->GetOverrideLocks();

    // A locked item at the front of the selection would hide the handles of everything
    // else. With block handles the per item test refuses the drag by name, which tells
    // the user which item is in the way
    bool blockHandlesPossible = selection.Size() > 1 && blockAnchorEditingEnabled();

    if( !item || ( item->IsLocked() && !overrideLocks && !blockHandlesPossible ) )
        return 0;

    Activate();
    // Must be done after Activate() so that it gets set into the correct context
    getViewControls()->ShowCursor( true );

    PCB_GRID_HELPER grid( m_toolMgr, editFrame->GetMagneticItemsSettings() );
    grid.SetPointEditProfile( true );
    m_constraintDragSession.reset();

    // Use the original object as a construction item
    std::vector<std::unique_ptr<BOARD_ITEM>> clones;

    m_editorBehavior.reset();

    if( selection.Size() > 1 )
    {
        // Multi-selection: check if all items are shapes
        std::vector<PCB_SHAPE*> shapes;
        std::set<KIID>          shapeIds;
        bool                    allShapes = true;
        bool                    anyLocked = false;

        for( EDA_ITEM* selItem : selection )
        {
            if( selItem->Type() == PCB_SHAPE_T )
            {
                PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( selItem );
                shapes.push_back( shape );
                shapeIds.insert( shape->m_Uuid );

                if( shape->IsLocked() )
                    anyLocked = true;
            }
            else
            {
                allShapes = false;
            }
        }

        std::vector<VECTOR2I> pinned;

        if( allShapes && shapes.size() > 1 )
            pinned = ConstraintPinnedPositions( board(), shapeIds );

        // Two pins in different places leave a scale no fixed point to work from
        if( allShapes && shapes.size() > 1 && ( !anyLocked || overrideLocks ) && pinned.size() < 2 )
        {
            std::optional<VECTOR2I> pin;

            if( !pinned.empty() )
                pin = pinned.front();

            std::unique_ptr<SHAPE_GROUP_POINT_EDIT_BEHAVIOR> groupBehavior =
                    std::make_unique<SHAPE_GROUP_POINT_EDIT_BEHAVIOR>( std::move( shapes ), item, pin );
            groupBehavior->SetInflation( bboxHandleInflation() );
            m_editorBehavior = std::move( groupBehavior );
            m_editPoints = std::make_shared<EDIT_POINTS>( item );
            m_editPoints->SetLineStateVisible( blockAnchorEditingEnabled() );
            m_editorBehavior->MakePoints( *m_editPoints );
        }
    }
    else
    {
        // Single selection: use existing makePoints logic
        m_editPoints = makePoints( item );
    }

    if( blockAnchorEditingEnabled() )
        buildBlockHandles( selection );

    if( !m_editPoints && m_blockHandles.empty() )
        return 0;

    PCB_SHAPE* graphicItem = dynamic_cast<PCB_SHAPE*>( item );

    // Only add the angle_item if we are editing a polygon or zone
    if( m_editPoints && ( item->Type() == PCB_ZONE_T || ( graphicItem && graphicItem->GetShape() == SHAPE_T::POLY ) ) )
    {
        m_angleItem = std::make_unique<KIGFX::PREVIEW::ANGLE_ITEM>( m_editPoints );
    }

    m_preview.FreeItems();
    m_radiusHelper = nullptr;
    getView()->Add( &m_preview );

    m_radiusHelper = new RECT_RADIUS_TEXT_ITEM( pcbIUScale, editFrame->GetUserUnits() );
    m_preview.Add( m_radiusHelper );

    if( m_editPoints )
        getView()->Add( m_editPoints.get() );

    if( m_angleItem )
        getView()->Add( m_angleItem.get() );

    setEditedPoint( nullptr );
    updateEditedPoint( aEvent );
    bool inDrag = false;
    bool isConstrained = false;
    bool haveSnapLineDirections = false;

    auto updateSnapLineDirections = [&]()
    {
        std::vector<VECTOR2I> directions;

        if( inDrag && m_editedPoint )
        {
            EDIT_RELATION* relation = nullptr;

            if( m_altConstraint )
                relation = m_altConstraint.get();
            else if( m_editedPoint->IsConstrained() )
                relation = m_editedPoint->GetRelation();

            directions = getConstraintDirections( relation );
        }

        if( directions.empty() )
        {
            grid.SetSnapLineDirections( {} );
            grid.SetSnapLineEnd( std::nullopt );
            haveSnapLineDirections = false;
        }
        else
        {
            VECTOR2I origin = m_altConstraint ? m_altConstrainer.GetPosition() : m_original.GetPosition();

            grid.SetSnapLineDirections( directions );
            grid.SetSnapLineOrigin( origin );
            grid.SetSnapLineEnd( std::nullopt );
            haveSnapLineDirections = true;
        }
    };

    BOARD_COMMIT commit( editFrame );

    auto installFeasibilityCallback = [&]()
    {
        grid.SetFeasibilityCallback( {} );

        if( !m_constraintDragSession || dynamic_cast<EDIT_LINE*>( m_editedPoint ) )
            return;

        std::shared_ptr<BOARD_CONSTRAINT_DRAG_SESSION> session = m_constraintDragSession;

        grid.SetFeasibilityCallback(
                [session]( const SNAP_SOURCE_CONTEXT& aContext, const std::vector<SNAP_CANDIDATE>& aCandidates )
                {
                    return session->ResolveCandidates( aContext, aCandidates );
                } );
    };

    BLOCK_DRAG_SESSION block( *this, editFrame, grid, commit );


    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );
        installFeasibilityCallback();

        if( editFrame->IsType( FRAME_PCB_EDITOR ) )
            m_arcEditMode = editFrame->GetPcbNewSettings()->m_ArcEditMode;
        else
            m_arcEditMode = editFrame->GetFootprintEditorSettings()->m_ArcEditMode;

        if( ( !m_editPoints && m_blockHandles.empty() ) || evt->IsSelectionEvent()
            || evt->Matches( EVENTS::InhibitSelectionEditing ) )
        {
            break;
        }

        EDIT_POINT* prevHover = m_hoveredPoint;

        if( !inDrag && !block.IsActive() )
            updateEditedPoint( *evt );

        if( prevHover != m_hoveredPoint && m_editPoints )
        {
            getView()->Update( m_editPoints.get() );

            if( m_angleItem )
                getView()->Update( m_angleItem.get() );
        }

        // A running gesture holds a staged commit, and with copper in it a live router
        // session. Refuse any command that could edit the board underneath it, because a
        // commit pushed now would record the gesture's half-dragged geometry as the state
        // its own undo returns to. Undo is already held off while the gesture runs
        if( block.IsActive() && evt->Category() == TC_COMMAND && !blockDragAllowsCommand( *evt ) )
        {
            wxBell();
            continue;
        }

        // D over a picked handle starts the gesture from the keyboard, with the cursor
        // free instead of held down. Every other use of D belongs to the edit tool
        if( evt->IsAction( &PCB_ACTIONS::drag45Degree ) || evt->IsAction( &PCB_ACTIONS::dragFreeAngle ) )
        {
            // An edit already running owns D. Passing it on would start the edit tool's own
            // drag of the same selection on top of this one, and that tool refilters the
            // selection, which ends this gesture through its selection event. With the
            // feature off the key is not ours to swallow
            if( blockAnchorEditingEnabled() && ( inDrag || block.IsActive() ) )
                continue;

            block.ClearRefused();

            if( blockAnchorEditingEnabled() && block.Begin( getViewControls()->GetCursorPosition( false ), false ) )
            {
                // Handled here, so the loop's catch-all does not pass it on as well
                continue;
            }

            // The gesture already said why it refused, so keep the key here. Passed on, the
            // edit tool would replace that message with one of its own
            if( block.Refused() )
                continue;

            evt->SetPassEvent();
        }

        if( evt->IsDrag( BUT_LEFT ) && !inDrag && !block.PressConsumed() && !block.IsActive()
            && blockAnchorEditingEnabled() )
        {
            block.Begin( evt->DragOrigin(), true );
        }

        if( ( ( evt->IsDrag( BUT_LEFT ) && !block.PressConsumed() ) || ( block.KeyboardDrag() && evt->IsMotion() ) )
            && !inDrag && block.IsActive() )
        {
            if( block.ApplyCursor( *evt ) )
                break;
        }
        else if( block.IsActive()
                 && ( evt->IsMouseUp( BUT_LEFT ) || ( block.KeyboardDrag() && evt->IsClick( BUT_LEFT ) ) ) )
        {
            if( block.Release( *evt ) )
                break;
        }
        // Comes before the click pick and the catch-all, which do not test this flag
        else if( block.PressConsumed() && evt->IsDrag( BUT_LEFT ) )
        {
            // The press we swallowed keeps the pointer until the release
        }
        else if( block.PressConsumed() && ( evt->IsMouseUp( BUT_LEFT ) || evt->IsClick( BUT_LEFT ) ) )
        {
            // The swallowed press ends here, and a later drag may try again
            block.ClearPressConsumed();
        }
        else if( evt->IsDrag( BUT_LEFT ) && !inDrag && !block.PressConsumed() && !m_editedPoint )
        {
            if( block.AreaPickStarts( *evt ) )
            {
                if( block.RunAreaPick( *evt ) )
                    break;
            }
            else
            {
                // A refused start goes to the selection tool, which selects as usual
                evt->SetPassEvent();
            }
        }
        else if( evt->IsDrag( BUT_LEFT ) && m_editedPoint && !block.PressConsumed() )
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

                if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( item ) )
                {
                    if( std::optional<CONSTRAINT_MEMBER> member =
                                constraintMemberForEditPoint( shape, m_editPoints.get(), m_editedPoint ) )
                    {
                        m_constraintDragSession = std::make_shared<BOARD_CONSTRAINT_DRAG_SESSION>();

                        if( !m_constraintDragSession->Build( board(), *member ) )
                            m_constraintDragSession.reset();

                        installFeasibilityCallback();
                    }
                }

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

            EDIT_LINE* line = dynamic_cast<EDIT_LINE*>( m_editedPoint );
            bool       ctrlHeld = evt->Modifier( MD_CTRL );

            bool need_constraint = ( Is45Limited() || Is90Limited() ) && !ctrlHeld;

            if( isConstrained != need_constraint )
            {
                setAltConstraint( need_constraint );
                isConstrained = need_constraint;
                updateSnapLineDirections();
            }

            if( need_constraint )
            {
                VECTOR2I origin = m_altConstraint ? m_altConstrainer.GetPosition()
                                                  : m_original.GetPosition();
                grid.SetAngleRestriction( origin, Is45Limited() ? 45.0 : 90.0 );
            }
            else
            {
                grid.SetAngleRestriction( std::nullopt, 0.0 );
            }

            // For polygon lines, Ctrl temporarily toggles between CONVERGING and FIXED_LENGTH modes

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
                    POLYGON_EDGE_DRAG_POLICY* policy = line->GetDragPolicy();

                    if( policy )
                    {
                        POLYGON_LINE_MODE targetMode = ctrlHeld ? POLYGON_LINE_MODE::FIXED_LENGTH
                                                                : POLYGON_LINE_MODE::CONVERGING;

                        if( policy->GetMode() != targetMode )
                            policy->SetMode( targetMode );
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
                    POLYGON_EDGE_DRAG_POLICY* dragPolicy =
                            line ? line->GetDragPolicy() : nullptr;

                    bool snappedAlongPerp = false;

                    if( dragPolicy )
                    {
                        // For a polygon edge, the line moves only perpendicular to itself.
                        // Snapping pos.x and pos.y independently to the axis-aligned grid
                        // produces inconsistent perpendicular displacements when the edge is
                        // tilted (different magnitudes depending on which axis crossed the
                        // half-grid threshold first), causing the rendered edge to flicker
                        // between two positions. Quantize the perpendicular displacement
                        // directly so each grid step produces one stable line position.
                        const VECTOR2I& origCenter = dragPolicy->GetOriginalCenter();
                        const VECTOR2I& perpVec = dragPolicy->GetPerpVector();
                        double perpLen = VECTOR2D( perpVec ).EuclideanNorm();

                        if( perpLen > 0 )
                        {
                            VECTOR2D perpUnit = VECTOR2D( perpVec ) / perpLen;
                            VECTOR2D gridSize = grid.GetGridSize( grid.GetItemGrid( item ) );

                            // Effective grid spacing along the perpendicular direction. For an
                            // axis-aligned edge this reduces to the grid pitch on that axis.
                            double step = std::hypot( gridSize.x * perpUnit.x,
                                                      gridSize.y * perpUnit.y );

                            if( step > 0 )
                            {
                                double offset = VECTOR2D( pos - origCenter ).Dot( perpUnit );
                                double snapped = std::round( offset / step ) * step;
                                VECTOR2D snappedPt = VECTOR2D( origCenter ) + perpUnit * snapped;
                                pos = VECTOR2I( KiROUND( snappedPt.x ), KiROUND( snappedPt.y ) );
                                snappedAlongPerp = true;
                            }
                        }
                    }

                    if( !snappedAlongPerp )
                    {
                        VECTOR2I gridPt =
                                grid.ResolveSnap( pos, {}, grid.GetItemGrid( item ), { item } )
                                        .position;

                        VECTOR2I last = m_editedPoint->GetPosition();
                        VECTOR2I delta = pos - last;
                        VECTOR2I deltaGrid =
                                gridPt
                                - grid.ResolveSnap( last, {}, grid.GetItemGrid( item ), { item } )
                                          .position;

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

            bool isFreePolygon =
                    item->Type() == PCB_ZONE_T
                    || ( item->Type() == PCB_SHAPE_T && static_cast<PCB_SHAPE*>( item )->GetShape() == SHAPE_T::POLY );

            if( isFreePolygon && !m_angleSnapActive && m_editPoints->PointsSize() > 2 && !evt->Modifier( MD_SHIFT ) )
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
                        VECTOR2I snapped = snapCorner( prev, next, pos, snapAng );

                        if( m_editedPoint->GetGridConstraint() == SNAP_TO_GRID && grid.GetSnap() )
                        {
                            VECTOR2I gridded =
                                    grid.ResolveSnap( snapped, {}, grid.GetItemGrid( item ), { item } )
                                            .position;
                            double   griddedAng = SEG( gridded, prev ).Angle( SEG( gridded, next ) ).AsDegrees();

                            snapped = std::abs( griddedAng - snapAng ) < 2.0 ? gridded : pos;
                        }

                        if( snapped != pos )
                        {
                            m_angleSnapPos = snapped;
                            m_angleSnapActive = true;
                            m_stickyDisplacement = evt->Position() - m_angleSnapPos;
                            pos = m_angleSnapPos;
                        }
                    }
                }
            }

            bool constraintSnapped = false;
            std::vector<VECTOR2I> stationarySelfPoints;
            std::vector<SEG>      stationarySelfSegments;

            if( item->Type() == PCB_ZONE_T
                || ( item->Type() == PCB_SHAPE_T
                     && static_cast<PCB_SHAPE*>( item )->GetShape() == SHAPE_T::POLY ) )
            {
                const EDIT_LINE* editedLine = dynamic_cast<const EDIT_LINE*>( m_editedPoint );

                for( unsigned i = 0; i < m_editPoints->PointsSize(); ++i )
                {
                    const EDIT_POINT& point = m_editPoints->Point( i );

                    if( &point != m_editedPoint
                        && ( !editedLine
                             || ( &point != &editedLine->GetOrigin()
                                  && &point != &editedLine->GetEnd() ) ) )
                    {
                        stationarySelfPoints.push_back( point.GetPosition() );
                    }
                }

                for( unsigned i = 0; i < m_editPoints->LinesSize(); ++i )
                {
                    const EDIT_LINE& stationaryLine = m_editPoints->Line( i );

                    if( &stationaryLine != editedLine
                        && &stationaryLine.GetOrigin() != m_editedPoint
                        && &stationaryLine.GetEnd() != m_editedPoint
                        && ( !editedLine
                             || ( &stationaryLine.GetOrigin() != &editedLine->GetOrigin()
                                  && &stationaryLine.GetOrigin() != &editedLine->GetEnd()
                                  && &stationaryLine.GetEnd() != &editedLine->GetOrigin()
                                  && &stationaryLine.GetEnd() != &editedLine->GetEnd() ) ) )
                    {
                        stationarySelfSegments.emplace_back(
                                stationaryLine.GetOrigin().GetPosition(),
                                stationaryLine.GetEnd().GetPosition() );
                    }
                }
            }
            else if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( item ) )
            {
                int editedIndex = getEditedPointIndex();

                if( shape->GetShape() == SHAPE_T::RECTANGLE && m_editPoints->PointsSize() >= RECT_MAX_POINTS
                    && editedIndex >= RECT_TOP_LEFT && editedIndex <= RECT_BOT_LEFT )
                {
                    int opposite = ( editedIndex + 2 ) % 4;
                    stationarySelfPoints.push_back( m_editPoints->Point( opposite ).GetPosition() );
                }
                else if( shape->GetShape() == SHAPE_T::RECTANGLE && m_editPoints->LinesSize() >= 4 )
                {
                    for( unsigned i = 0; i < m_editPoints->LinesSize() && i < 4; ++i )
                    {
                        if( m_editedPoint != &m_editPoints->Line( i ) )
                            continue;

                        const EDIT_LINE& opposite = m_editPoints->Line( ( i + 2 ) % 4 );
                        stationarySelfPoints.push_back( opposite.GetOrigin().GetPosition() );
                        stationarySelfPoints.push_back( opposite.GetEnd().GetPosition() );
                        stationarySelfSegments.emplace_back( opposite.GetOrigin().GetPosition(),
                                                             opposite.GetEnd().GetPosition() );
                        break;
                    }
                }
                else if( shape->GetShape() == SHAPE_T::ARC )
                {
                    constexpr int arcStart = 0;
                    constexpr int arcMid = 1;
                    constexpr int arcEnd = 2;
                    constexpr int arcCenter = 3;

                    if( m_arcEditMode == ARC_EDIT_MODE::KEEP_ENDPOINTS_OR_START_DIRECTION )
                    {
                        if( editedIndex != arcStart )
                            stationarySelfPoints.push_back( m_editPoints->Point( arcStart ).GetPosition() );

                        if( editedIndex != arcEnd )
                            stationarySelfPoints.push_back( m_editPoints->Point( arcEnd ).GetPosition() );
                    }
                    else if( editedIndex == arcStart || editedIndex == arcMid || editedIndex == arcEnd )
                    {
                        stationarySelfPoints.push_back( m_editPoints->Point( arcCenter ).GetPosition() );
                    }
                }
            }

            grid.SetStationarySelfGeometry( std::move( stationarySelfPoints ),
                                            std::move( stationarySelfSegments ) );

            // Apply 45 degree or other constraints
            if( !m_angleSnapActive && m_altConstraint )
            {
                m_editedPoint->SetPosition(
                        grid.ResolveSnap( pos, snapLayers, grid.GetItemGrid( item ), { item } )
                                .position );
                constraintSnapped = true;
            }
            else if( !m_angleSnapActive && m_editedPoint->IsConstrained() )
            {
                m_editedPoint->SetPosition( pos );
                m_editedPoint->ApplyRelation( grid );
                constraintSnapped = true;

                // For constrained lines (like zone edges), try to snap to nearby anchors
                // that lie on the constraint line. First get the constrained position, then
                // look for snap anchors and verify they're on the constraint line.
                if( grid.GetSnap() && !snapLayers.empty() )
                {
                    VECTOR2I constrainedPos = m_editedPoint->GetPosition();
                    VECTOR2I snapPos =
                            grid.ResolveSnap( constrainedPos, snapLayers, grid.GetItemGrid( item ),
                                              { item } )
                                    .position;

                    // Require the relation to preserve the discrete anchor exactly.
                    if( snapPos != constrainedPos )
                    {
                        m_editedPoint->SetPosition( snapPos );
                        m_editedPoint->ApplyRelation( grid );
                        VECTOR2I projectedPos = m_editedPoint->GetPosition();

                        if( projectedPos != snapPos )
                            m_editedPoint->SetPosition( constrainedPos );
                    }
                }
            }
            else if( !m_angleSnapActive && m_editedPoint->GetGridConstraint() == SNAP_TO_GRID )
            {
                m_editedPoint->SetPosition(
                        grid.ResolveSnap( pos, snapLayers, grid.GetItemGrid( item ), { item } )
                                .position );
            }
            else
            {
                m_editedPoint->SetPosition( pos );
            }

            if( haveSnapLineDirections )
            {
                VECTOR2I snapOrigin = m_altConstraint ? m_altConstrainer.GetPosition() : m_original.GetPosition();
                grid.SetSnapLineOrigin( snapOrigin );

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
            m_constraintDragSession.reset();
            frame()->UndoRedoBlock( false );
            updateSnapLineDirections();

            m_toolMgr->PostAction<EDA_ITEM*>( ACTIONS::reselectItem, item ); // FIXME: Needed for generators
        }
        else if( !inDrag && !block.IsActive() && evt->IsClick( BUT_LEFT ) && blockAnchorEditingEnabled() )
        {
            bool accumulate = evt->Modifier( MD_SHIFT ) != 0;
            bool consumed = pickHandleAt( evt->Position(), accumulate );

            // A plain click on nothing just clears the picks. A Shift click on nothing
            // keeps them and goes to the selection tool, which adds to the selection
            if( !consumed && !accumulate && !m_pickedHandles.empty() )
            {
                clearPickedHandles();
                consumed = true;
            }

            if( consumed )
            {
                // The selection tool never sees a click we swallow, so stop its long press
                // timer here
                m_selectionTool->StopDisambiguation();
            }
            else
            {
                evt->SetPassEvent();
            }
        }
        else if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            if( block.IsActive() )
            {
                if( block.Cancel( *evt ) )
                    break;
            }
            else if( inDrag ) // Restore the last change
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
                m_constraintDragSession.reset();
                frame()->UndoRedoBlock( false );
                updateSnapLineDirections();
            }
            else if( evt->IsCancelInteractive() && !m_pickedHandles.empty() )
            {
                clearPickedHandles();

                // Cancel events go to every tool by default. The first Escape only drops
                // the picks, so stop it before it clears the selection too. A tool switch
                // and the other ways of cancelling keep their usual effect
                if( evt->IsCancel() && evt->GetCommandId() == (int) WXK_ESCAPE )
                    evt->SetPassEvent( false );
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
            if( m_editPoints && item->Type() == PCB_PAD_T && m_isFootprintEditor )
            {
                if( getView()->HasItem( m_editPoints.get() ) )
                    getView()->Remove( m_editPoints.get() );

                if( m_angleItem && getView()->HasItem( m_angleItem.get() ) )
                    getView()->Remove( m_angleItem.get() );

                m_editedPoint = nullptr;
                m_hoveredPoint = nullptr;
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

    // IS_MOVING is only still set if the loop broke mid-drag, and a null m_editPoints means
    // Reset() ran while we were suspended, so the board and item have already been destroyed
    if( inDrag && m_editPoints )
    {
        if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( m_editPoints->GetParent() ) )
        {
            shape->ClearFlags( IS_MOVING );
            shape->UpdateHatching();
        }
    }

    // Leaving the loop without going through the cancel branch undoes the drag
    // the same way Escape does. Reset() does this before the collections clear,
    // while the items are still here.
    block.Abort();
    m_blockDragAbort = nullptr;

    m_preview.FreeItems();
    m_radiusHelper = nullptr;
    m_constraintDragSession.reset();

    clearBlockHandles();

    if( getView()->HasItem( &m_preview ) )
        getView()->Remove( &m_preview );

    if( m_editPoints )
    {
        if( getView()->HasItem( m_editPoints.get() ) )
            getView()->Remove( m_editPoints.get() );

        if( m_angleItem && getView()->HasItem( m_angleItem.get() ) )
            getView()->Remove( m_angleItem.get() );

        m_hoveredPoint = nullptr;
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

    // Re-derive any geometry constrained to the dragged segment endpoint (issue #2329).  The
    // behavior above has already moved the dragged point to the cursor, so its current endpoint
    // position is the pin target for the solver.
    if( auto* groupBehavior = dynamic_cast<SHAPE_GROUP_POINT_EDIT_BEHAVIOR*>( m_editorBehavior.get() );
        groupBehavior && BoardHasConstraints( board() ) )
    {
        // A group edit writes its own shapes, so the neighbours still need re-solving
        std::vector<PCB_SHAPE*> modified;

        ReSolveShapeClustersHoldingEdited( board(), groupBehavior->GetShapes(), &modified,
                                           [&]( BOARD_ITEM* aNeighbor )
                                           {
                                               aCommit.Modify( aNeighbor );
                                           } );

        for( PCB_SHAPE* neighbor : modified )
            updatedItems.push_back( neighbor );
    }
    else if( item->Type() == PCB_SHAPE_T && m_editedPoint && BoardHasConstraints( board() ) )
    {
        PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( item );
        SHAPE_T    type = shape->GetShape();
        VECTOR2I   cursor = m_editedPoint->GetPosition();

        std::optional<CONSTRAINT_MEMBER>                      member;
        std::optional<std::pair<CONSTRAINT_MEMBER, VECTOR2I>> coDragged;

        // Match dragged point to a constraint anchor segment bezier and arc expose endpoints
        // arc and circle also expose a centre bezier has none so centre test stays arc only
        if( type == SHAPE_T::SEGMENT || type == SHAPE_T::ARC || type == SHAPE_T::BEZIER )
        {
            if( cursor == shape->GetStart() )
                member = CONSTRAINT_MEMBER( shape->m_Uuid, CONSTRAINT_ANCHOR::START );
            else if( cursor == shape->GetEnd() )
                member = CONSTRAINT_MEMBER( shape->m_Uuid, CONSTRAINT_ANCHOR::END );
            else if( type == SHAPE_T::ARC && cursor == shape->GetCenter() )
                member = CONSTRAINT_MEMBER( shape->m_Uuid, CONSTRAINT_ANCHOR::CENTER );
        }
        else if( type == SHAPE_T::CIRCLE )
        {
            if( cursor == shape->GetCenter() )
                member = CONSTRAINT_MEMBER( shape->m_Uuid, CONSTRAINT_ANCHOR::CENTER );
        }
        else if( type == SHAPE_T::RECTANGLE && m_editPoints->PointsSize() >= RECT_MAX_POINTS )
        {
            // Only corner handles map to vertex anchors centre radius and side handles reshape whole rect
            // corners kept in min max order so ordinal equals vertex index solve target reads post clamp position
            for( unsigned i = RECT_TOP_LEFT; i <= RECT_BOT_LEFT; ++i )
            {
                if( isModified( m_editPoints->Point( i ) ) )
                {
                    if( std::optional<CONSTRAINT_ANCHOR_POINT> corner = ConstraintShapeVertex( shape, (int) i ) )
                    {
                        member = CONSTRAINT_MEMBER( shape->m_Uuid, corner->anchor, corner->index );
                        cursor = corner->pos;
                    }

                    break;
                }
            }

            // Side handle drags one edge param aliased by both corners side i runs corner i to i plus 1 mod 4
            // pinning corner i covers dragged plus one perpendicular param opposite corner hold covers the rest
            if( !member.has_value() )
            {
                for( unsigned i = 0; i < m_editPoints->LinesSize() && i < 4; ++i )
                {
                    if( isModified( m_editPoints->Line( i ) ) )
                    {
                        if( std::optional<CONSTRAINT_ANCHOR_POINT> corner = ConstraintShapeVertex( shape, (int) i ) )
                        {
                            member = CONSTRAINT_MEMBER( shape->m_Uuid, corner->anchor, corner->index );
                            cursor = corner->pos;
                        }

                        break;
                    }
                }
            }
        }
        else if( type == SHAPE_T::POLY && ConstraintPolygonIsModelable( shape ) )
        {
            // Editor builds one edit point per outline vertex in order so dragged ordinal is vertex index
            // holds even when a drag lands a vertex on top of another where position match would be ambiguous
            for( unsigned i = 0; i < m_editPoints->PointsSize(); ++i )
            {
                if( isModified( m_editPoints->Point( i ) ) )
                {
                    if( std::optional<CONSTRAINT_ANCHOR_POINT> vertex = ConstraintShapeVertex( shape, (int) i ) )
                    {
                        member = CONSTRAINT_MEMBER( shape->m_Uuid, vertex->anchor, vertex->index );
                        cursor = vertex->pos;
                    }

                    break;
                }
            }

            // Edge handle moves two adjacent vertices one member cannot express both so second vertex
            // rides as a co dragged pin line i runs vertex i to i plus 1 mod count positions read back post move
            if( !member.has_value() )
            {
                for( unsigned i = 0; i < m_editPoints->LinesSize(); ++i )
                {
                    if( isModified( m_editPoints->Line( i ) ) )
                    {
                        int next = ( (int) i + 1 ) % (int) m_editPoints->PointsSize();

                        std::optional<CONSTRAINT_ANCHOR_POINT> v0 = ConstraintShapeVertex( shape, (int) i );
                        std::optional<CONSTRAINT_ANCHOR_POINT> v1 = ConstraintShapeVertex( shape, next );

                        if( v0 && v1 )
                        {
                            member = CONSTRAINT_MEMBER( shape->m_Uuid, v0->anchor, v0->index );
                            cursor = v0->pos;
                            coDragged = { CONSTRAINT_MEMBER( shape->m_Uuid, v1->anchor, v1->index ), v1->pos };
                        }

                        break;
                    }
                }
            }
        }

        bool isCurve = type == SHAPE_T::CIRCLE || type == SHAPE_T::ARC || type == SHAPE_T::ELLIPSE
                       || type == SHAPE_T::ELLIPSE_ARC;

        std::vector<PCB_SHAPE*> modified;

        // Failed or diverged solve leaves neighbors untouched below so nothing is half moved this frame
        // moved shapes report in modified moved dimensions do not so refresh view here or they freeze
        auto stageNeighbor = [&]( BOARD_ITEM* aItem )
        {
            aCommit.Modify( aItem );

            if( aItem->Type() != PCB_SHAPE_T )
                updatedItems.push_back( aItem );
        };

        if( member.has_value() )
        {
            if( !m_constraintDragSession || !m_constraintDragSession->Matches( *member ) )
            {
                m_constraintDragSession = std::make_shared<BOARD_CONSTRAINT_DRAG_SESSION>();

                if( !m_constraintDragSession->Build( board(), *member ) )
                    m_constraintDragSession.reset();
            }

            if( m_constraintDragSession )
            {
                m_constraintDragSession->Solve( cursor, &modified, stageNeighbor,
                                                /* aIncludeDragged */ false,
                                                /* aStabilize */ false, {}, coDragged );
            }
            else
            {
                SolveCluster( board(), member.value(), cursor, &modified, stageNeighbor,
                              /* aIncludeDragged */ false, /* aStabilize */ false, {}, coDragged );
            }
        }
        else if( isCurve )
        {
            // Radius or axis handle not a constrained point resize hold keeps new radius but yields
            // to a real radius constraint full dof hold below would fight it so curves use this instead
            ReSolveAfterShapeResize( board(), shape, &modified, stageNeighbor );
        }
        else if( type == SHAPE_T::RECTANGLE || ( type == SHAPE_T::POLY && ConstraintPolygonIsModelable( shape ) ) )
        {
            // Only whole shape handles centre and corner radius grips land here side corner and edge
            // drags pinned members above already treated like a properties edit new geometry wins neighbors follow
            ReSolveShapeClustersHoldingEdited( board(), { shape }, &modified, stageNeighbor );
        }

        for( PCB_SHAPE* neighbor : modified )
            updatedItems.push_back( neighbor );
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
    updateBlockHandlePoints();

    // Done here too, because the three early returns below skip the refresh at the end
    applyPickedFlags();

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

    if( SHAPE_GROUP_POINT_EDIT_BEHAVIOR* groupBehavior =
                dynamic_cast<SHAPE_GROUP_POINT_EDIT_BEHAVIOR*>( m_editorBehavior.get() ) )
    {
        groupBehavior->SetInflation( bboxHandleInflation() );
    }

    if( !m_editorBehavior->UpdatePoints( *m_editPoints ) )
    {
        if( getView()->HasItem( m_editPoints.get() ) )
            getView()->Remove( m_editPoints.get() );

        m_hoveredPoint = nullptr;
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

    applyPickedFlags();

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
            POLYGON_EDGE_DRAG_POLICY* policy = line->GetDragPolicy();

            if( policy )
                policy->SetMode( POLYGON_LINE_MODE::FIXED_LENGTH );

            // Don't set m_altConstraint - we're modifying the line's own constraint
        }
        else
        {
            // Find a proper constraining point for angle snapping mode
            m_altConstrainer = get45DegConstrainer();

            if( Is90Limited() )
                m_altConstraint = EDIT_RELATION::Angle90( m_altConstrainer );
            else
                m_altConstraint = EDIT_RELATION::Angle45( m_altConstrainer );
        }
    }
    else
    {
        if( line && isPoly )
        {
            // Restore the line's constraint to CONVERGING mode
            POLYGON_EDGE_DRAG_POLICY* policy = line->GetDragPolicy();

            if( policy )
                policy->SetMode( POLYGON_LINE_MODE::CONVERGING );
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

        // Zones cannot carry constraint members but shape polygons can insertion shifts ordinals at or
        // past new vertex issue 2329 members only exist on hole free polys index past outline count is a hole vertex
        if( graphicItem && nextNearestIdx < (unsigned) zoneOutline->COutline( 0 ).PointCount() )
        {
            RemapPolygonVertexMembers( frame->GetBoard(), graphicItem->m_Uuid, (int) nextNearestIdx, 1,
                                       [&]( BOARD_ITEM* aConstraint ) { commit.Modify( aConstraint ); },
                                       [&]( BOARD_ITEM* aConstraint ) { commit.Remove( aConstraint ); } );
        }

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

            // Members only exist on hole free shape polygons where contour relative ordinal is member
            // index removed vertex member retires its constraint later ordinals shift down issue 2329
            if( item->Type() == PCB_SHAPE_T && vertexIdx.m_contour == 0 )
            {
                RemapPolygonVertexMembers( frame->GetBoard(), item->m_Uuid, vertexIdx.m_vertex, -1,
                                           [&]( BOARD_ITEM* aConstraint ) { commit.Modify( aConstraint ); },
                                           [&]( BOARD_ITEM* aConstraint ) { commit.Remove( aConstraint ); } );
            }
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

            // Chamfered corner constraints retire members past it net one higher insert pass must run
            // first threshold nearestIdx plus 1 deleting first nets negative one instead issue 2329
            if( item->Type() == PCB_SHAPE_T && nearestIdx < (unsigned) polygon->COutline( 0 ).PointCount() )
            {
                auto modify = [&]( BOARD_ITEM* aConstraint ) { commit.Modify( aConstraint ); };
                auto remove = [&]( BOARD_ITEM* aConstraint ) { commit.Remove( aConstraint ); };

                RemapPolygonVertexMembers( frame->GetBoard(), item->m_Uuid, (int) nearestIdx + 1, 2, modify, remove );
                RemapPolygonVertexMembers( frame->GetBoard(), item->m_Uuid, (int) nearestIdx, -1, modify, remove );
            }
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
