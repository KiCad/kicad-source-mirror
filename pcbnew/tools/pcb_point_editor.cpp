/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2021 CERN
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <tool/tool_manager.h>
#include <view/view_controls.h>
#include <geometry/seg.h>
#include <confirm.h>
#include "pcb_actions.h"
#include "pcb_selection_tool.h"
#include "pcb_point_editor.h"
#include "pcb_grid_helper.h"
#include <board_commit.h>
#include <bitmaps.h>
#include <status_popup.h>
#include <pcb_edit_frame.h>
#include <fp_shape.h>
#include <pcb_dimension.h>
#include <pad.h>
#include <zone.h>
#include <connectivity/connectivity_data.h>
#include <progress_reporter.h>


// Few constants to avoid using bare numbers for point indices
enum SEG_POINTS
{
    SEG_START, SEG_END
};


enum RECT_POINTS
{
    RECT_TOP_LEFT, RECT_TOP_RIGHT, RECT_BOT_RIGHT, RECT_BOT_LEFT
};


enum RECT_LINES
{
    RECT_TOP, RECT_RIGHT, RECT_BOT, RECT_LEFT
};


enum ARC_POINTS
{
    ARC_CENTER, ARC_START, ARC_MID, ARC_END
};


enum CIRCLE_POINTS
{
    CIRC_CENTER, CIRC_END
};


enum BEZIER_CURVE_POINTS
{
    BEZIER_CURVE_START,
    BEZIER_CURVE_CONTROL_POINT1,
    BEZIER_CURVE_CONTROL_POINT2,
    BEZIER_CURVE_END
};


enum DIMENSION_POINTS
{
    DIM_START,
    DIM_END,
    DIM_TEXT,
    DIM_CROSSBARSTART,
    DIM_CROSSBAREND,
};


PCB_POINT_EDITOR::PCB_POINT_EDITOR() :
    PCB_TOOL_BASE( "pcbnew.PointEditor" ),
    m_selectionTool( nullptr ),
    m_editedPoint( nullptr ),
    m_hoveredPoint( nullptr ),
    m_original( VECTOR2I( 0, 0 ) ),
    m_refill( false ),
    m_altEditMethod( false ),
    m_altConstrainer( VECTOR2I( 0, 0 ) )
{
}


void PCB_POINT_EDITOR::Reset( RESET_REASON aReason )
{
    m_refill = false;
    m_editPoints.reset();
    m_altConstraint.reset();
    getViewControls()->SetAutoPan( false );

    m_statusPopup = std::make_unique<STATUS_TEXT_POPUP>( getEditFrame<PCB_BASE_EDIT_FRAME>() );
    m_statusPopup->SetTextColor( wxColour( 255, 0, 0 ) );
    m_statusPopup->SetText( _( "Self-intersecting polygons are not allowed." ) );
}


bool PCB_POINT_EDITOR::Init()
{
    // Find the selection tool, so they can cooperate
    m_selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    wxASSERT_MSG( m_selectionTool, "pcbnew.InteractiveSelection tool is not available" );

    auto& menu = m_selectionTool->GetToolMenu().GetMenu();
    menu.AddItem( PCB_ACTIONS::pointEditorAddCorner, PCB_POINT_EDITOR::addCornerCondition );
    menu.AddItem( PCB_ACTIONS::pointEditorRemoveCorner,
                  std::bind( &PCB_POINT_EDITOR::removeCornerCondition, this, _1 ) );

    return true;
}


void PCB_POINT_EDITOR::buildForPolyOutline( std::shared_ptr<EDIT_POINTS> points,
                                            const SHAPE_POLY_SET* aOutline )
{
    int cornersCount = aOutline->TotalVertices();

    for( auto iterator = aOutline->CIterateWithHoles(); iterator; iterator++ )
    {
        points->AddPoint( *iterator );

        if( iterator.IsEndContour() )
            points->AddBreak();
    }

    // Lines have to be added after creating edit points,
    // as they use EDIT_POINT references
    for( int i = 0; i < cornersCount - 1; ++i )
    {
        if( points->IsContourEnd( i ) )
            points->AddLine( points->Point( i ), points->Point( points->GetContourStartIdx( i ) ) );
        else
            points->AddLine( points->Point( i ), points->Point( i + 1 ) );

        points->Line( i ).SetConstraint( new EC_PERPLINE( points->Line( i ) ) );
    }

    // The last missing line, connecting the last and the first polygon point
    points->AddLine( points->Point( cornersCount - 1 ),
                     points->Point( points->GetContourStartIdx( cornersCount - 1 ) ) );

    points->Line( points->LinesSize() - 1 )
            .SetConstraint( new EC_PERPLINE( points->Line( points->LinesSize() - 1 ) ) );
}


std::shared_ptr<EDIT_POINTS> PCB_POINT_EDITOR::makePoints( EDA_ITEM* aItem )
{
    std::shared_ptr<EDIT_POINTS> points = std::make_shared<EDIT_POINTS>( aItem );

    if( !aItem )
        return points;

    // Generate list of edit points basing on the item type
    switch( aItem->Type() )
    {
    case PCB_SHAPE_T:
    case PCB_FP_SHAPE_T:
    {
        const PCB_SHAPE* shape = static_cast<const PCB_SHAPE*>( aItem );

        switch( shape->GetShape() )
        {
        case SHAPE_T::SEGMENT:
            points->AddPoint( shape->GetStart() );
            points->AddPoint( shape->GetEnd() );
            break;

        case SHAPE_T::RECT:
            points->AddPoint( shape->GetStart() );
            points->AddPoint( wxPoint( shape->GetEnd().x, shape->GetStart().y ) );
            points->AddPoint( shape->GetEnd() );
            points->AddPoint( wxPoint( shape->GetStart().x, shape->GetEnd().y ) );

            points->AddLine( points->Point( RECT_TOP_LEFT ), points->Point( RECT_TOP_RIGHT ) );
            points->Line( RECT_TOP ).SetConstraint( new EC_PERPLINE( points->Line( RECT_TOP ) ) );
            points->AddLine( points->Point( RECT_TOP_RIGHT ), points->Point( RECT_BOT_RIGHT ) );
            points->Line( RECT_RIGHT ).SetConstraint( new EC_PERPLINE( points->Line( RECT_RIGHT ) ) );
            points->AddLine( points->Point( RECT_BOT_RIGHT ), points->Point( RECT_BOT_LEFT ) );
            points->Line( RECT_BOT ).SetConstraint( new EC_PERPLINE( points->Line( RECT_BOT ) ) );
            points->AddLine( points->Point( RECT_BOT_LEFT ), points->Point( RECT_TOP_LEFT ) );
            points->Line( RECT_LEFT ).SetConstraint( new EC_PERPLINE( points->Line( RECT_LEFT ) ) );

            break;

        case SHAPE_T::ARC:
            points->AddPoint( shape->GetCenter() );
            points->AddPoint( shape->GetStart() );
            points->AddPoint( shape->GetArcMid() );
            points->AddPoint( shape->GetEnd() );

            points->Point( ARC_MID ).SetGridConstraint( IGNORE_GRID );
            points->Point( ARC_START ).SetGridConstraint( SNAP_TO_GRID );
            points->Point( ARC_CENTER ).SetGridConstraint( SNAP_BY_GRID );
            points->Point( ARC_END ).SetGridConstraint( SNAP_TO_GRID );
            break;

        case SHAPE_T::CIRCLE:
            points->AddPoint( shape->GetCenter() );
            points->AddPoint( shape->GetEnd() );
            break;

        case SHAPE_T::POLY:
            buildForPolyOutline( points, &shape->GetPolyShape() );
            break;

        case SHAPE_T::BEZIER:
            points->AddPoint( shape->GetStart() );
            points->AddPoint( shape->GetBezierC1() );
            points->AddPoint( shape->GetBezierC2() );
            points->AddPoint( shape->GetEnd() );
            break;

        default:        // suppress warnings
            break;
        }

        break;
    }

    case PCB_PAD_T:
    {
        const PAD* pad = static_cast<const PAD*>( aItem );
        wxPoint    shapePos = pad->ShapePos();
        wxPoint    halfSize( pad->GetSize().x / 2, pad->GetSize().y / 2 );

        if( !m_isFootprintEditor || pad->IsLocked() )
            break;

        switch( pad->GetShape() )
        {
        case PAD_SHAPE::CIRCLE:
            points->AddPoint( wxPoint( shapePos.x + halfSize.x, shapePos.y ) );
            break;

        case PAD_SHAPE::OVAL:
        case PAD_SHAPE::TRAPEZOID:
        case PAD_SHAPE::RECT:
        case PAD_SHAPE::ROUNDRECT:
        case PAD_SHAPE::CHAMFERED_RECT:
        {
            if( (int) pad->GetOrientation() % 900 != 0 )
                break;

            if( pad->GetOrientation() == 900 || pad->GetOrientation() == 2700 )
                std::swap( halfSize.x, halfSize.y );

            points->AddPoint( shapePos - halfSize );
            points->AddPoint( wxPoint( shapePos.x + halfSize.x, shapePos.y - halfSize.y ) );
            points->AddPoint( shapePos + halfSize );
            points->AddPoint( wxPoint( shapePos.x - halfSize.x, shapePos.y + halfSize.y ) );
        }
            break;

        default:        // suppress warnings
            break;
        }
    }
        break;

    case PCB_FP_ZONE_T:
    case PCB_ZONE_T:
    {
        const ZONE* zone = static_cast<const ZONE*>( aItem );
        buildForPolyOutline( points, zone->Outline() );
    }
        break;

    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_ORTHOGONAL_T:
    {
        const PCB_DIM_ALIGNED* dimension = static_cast<const PCB_DIM_ALIGNED*>( aItem );

        points->AddPoint( dimension->GetStart() );
        points->AddPoint( dimension->GetEnd() );
        points->AddPoint( dimension->Text().GetPosition() );
        points->AddPoint( dimension->GetCrossbarStart() );
        points->AddPoint( dimension->GetCrossbarEnd() );

        if( aItem->Type() == PCB_DIM_ALIGNED_T )
        {
            // Dimension height setting - edit points should move only along the feature lines
            points->Point( DIM_CROSSBARSTART )
                    .SetConstraint( new EC_LINE( points->Point( DIM_CROSSBARSTART ),
                                                 points->Point( DIM_START ) ) );
            points->Point( DIM_CROSSBAREND )
                    .SetConstraint( new EC_LINE( points->Point( DIM_CROSSBAREND ),
                                                 points->Point( DIM_END ) ) );
        }

        break;
    }

    case PCB_DIM_CENTER_T:
    {
        const PCB_DIM_CENTER* dimension = static_cast<const PCB_DIM_CENTER*>( aItem );

        points->AddPoint( dimension->GetStart() );
        points->AddPoint( dimension->GetEnd() );

        points->Point( DIM_END ).SetConstraint( new EC_45DEGREE( points->Point( DIM_END ),
                                                                 points->Point( DIM_START ) ) );

        break;
    }

    case PCB_DIM_LEADER_T:
    {
        const PCB_DIM_LEADER* dimension = static_cast<const PCB_DIM_LEADER*>( aItem );

        points->AddPoint( dimension->GetStart() );
        points->AddPoint( dimension->GetEnd() );
        points->AddPoint( dimension->Text().GetPosition() );

        points->Point( DIM_TEXT ).SetConstraint( new EC_45DEGREE( points->Point( DIM_TEXT ),
                                                                  points->Point( DIM_END ) ) );

        break;
    }

    default:
        points.reset();
        break;
    }

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
    m_editPoints = makePoints( item );

    if( !m_editPoints )
        return 0;

    getView()->Add( m_editPoints.get() );
    setEditedPoint( nullptr );
    updateEditedPoint( aEvent );
    m_refill = false;
    bool inDrag = false;

    BOARD_COMMIT commit( editFrame );
    LSET snapLayers = item->GetLayerSet();

    if( BaseType( item->Type() ) == PCB_DIMENSION_T )
        snapLayers = LSET::AllLayersMask();

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );

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

                commit.StageItems( selection, CHT_MODIFY );

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
            }

            VECTOR2I pos = evt->Position();

            //TODO: unify the constraints to solve simultaneously instead of sequentially
            switch( m_editedPoint->GetGridConstraint() )
            {
            case IGNORE_GRID:
                m_editedPoint->SetPosition( pos );
                break;

            case SNAP_TO_GRID:
                m_editedPoint->SetPosition( grid.BestSnapAnchor( pos, snapLayers, { item } ) );
                break;

            case SNAP_BY_GRID:
            {
                if( grid.GetUseGrid() )
                {
                    VECTOR2I gridPt = grid.BestSnapAnchor( pos, {}, { item } );

                    VECTOR2I last = m_editedPoint->GetPosition();
                    VECTOR2I delta = pos - last;
                    VECTOR2I deltaGrid = gridPt - grid.BestSnapAnchor( last, {}, { item } );

                    if( abs( delta.x ) > grid.GetGrid().x / 2 )
                        pos.x = last.x + deltaGrid.x;
                    else
                        pos.x = last.x;

                    if( abs( delta.y ) > grid.GetGrid().y / 2 )
                        pos.y = last.y + deltaGrid.y;
                    else
                        pos.y = last.y;
                }

                m_editedPoint->SetPosition( pos );
                break;
            }
            }

            // The alternative constraint limits to 45 degrees
            if( Is45Limited() )
                m_altConstraint->Apply();
            else
                m_editedPoint->ApplyConstraint();

            if( m_editedPoint->GetGridConstraint() == SNAP_TO_GRID )
            {
                m_editedPoint->SetPosition( grid.BestSnapAnchor( m_editedPoint->GetPosition(),
                                                                 snapLayers, { item } ) );
            }

            updateItem();
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

            commit.Push( _( "Drag a corner" ) );
            inDrag = false;
            frame()->UndoRedoBlock( false );

            m_refill = true;
        }
        else if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            if( inDrag )      // Restore the last change
            {
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

    if( m_editPoints )
    {
        getView()->Remove( m_editPoints.get() );

        finishItem();
        m_editPoints.reset();
    }

    m_editedPoint = nullptr;

    frame()->UpdateMsgPanel();

    return 0;
}

void PCB_POINT_EDITOR::editArcEndpointKeepTangent( PCB_SHAPE* aArc, const VECTOR2I& aCenter,
                                                   const VECTOR2I& aStart, const VECTOR2I& aMid,
                                                   const VECTOR2I& aEnd,
                                                   const VECTOR2I& aCursor ) const
{
    VECTOR2I start = aStart;
    VECTOR2I end = aEnd;
    VECTOR2I center = aCenter;
    bool     movingStart;
    bool     arcValid = true;

    VECTOR2I p1, p2, p3;
    // p1 does not move, p2 does.

    if( aStart != aArc->GetStart() )
    {
        start       = aCursor;
        p1          = aEnd;
        p2          = aStart;
        p3          = aMid;
        movingStart = true;
    }
    else if( aEnd != aArc->GetEnd() )
    {
        end         = aCursor;
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

    VECTOR2D u1, u2, u3;

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
            aArc->SetCenter( (wxPoint) center );

            if( movingStart )
                aArc->SetStart( (wxPoint) start );
            else
                aArc->SetEnd( (wxPoint) end );
        }
    }
}


/**
 * Update the coordinates of 4 corners of a rectangle, according to pad constraints and the
 * moved corner
 * @param aEditedPointIndex is the corner id
 * @param aMinWidth is the minimal width constraint
 * @param aMinHeight is the minimal height constraint
 * @param aTopLeft [in/out] is the RECT_TOPLEFT to constraint
 * @param aTopRight [in/out] is the RECT_TOPRIGHT to constraint
 * @param aBotLeft [in/out] is the RECT_BOTLEFT to constraint
 * @param aBotRight [in/out] is the RECT_BOTRIGHT to constraint
 * @param aHole the location of the pad's hole
 * @param aHoleSize the pad's hole size (or {0,0} if it has no hole)
 */
static void pinEditedCorner( int aEditedPointIndex, int aMinWidth, int aMinHeight,
                             VECTOR2I& aTopLeft, VECTOR2I& aTopRight, VECTOR2I& aBotLeft,
                             VECTOR2I& aBotRight, VECTOR2I& aHole, VECTOR2I& aHoleSize )
{
    switch( aEditedPointIndex )
    {
    case RECT_TOP_LEFT:
        if( aHoleSize.x )
        {
            // pin edited point to the top/left of the hole
            aTopLeft.x = std::min( aTopLeft.x, aHole.x - aHoleSize.x / 2 - aMinWidth );
            aTopLeft.y = std::min( aTopLeft.y, aHole.y - aHoleSize.y / 2 - aMinHeight );
        }
        else
        {
            // pin edited point within opposite corner
            aTopLeft.x = std::min( aTopLeft.x, aBotRight.x - aMinWidth );
            aTopLeft.y = std::min( aTopLeft.y, aBotRight.y - aMinHeight );
        }

        // push edited point edges to adjacent corners
        aTopRight.y = aTopLeft.y;
        aBotLeft.x = aTopLeft.x;

        break;

    case RECT_TOP_RIGHT:
        if( aHoleSize.x )
        {
            // pin edited point to the top/right of the hole
            aTopRight.x = std::max( aTopRight.x, aHole.x + aHoleSize.x / 2 + aMinWidth );
            aTopRight.y = std::min( aTopRight.y, aHole.y - aHoleSize.y / 2 - aMinHeight );
        }
        else
        {
            // pin edited point within opposite corner
            aTopRight.x = std::max( aTopRight.x, aBotLeft.x + aMinWidth );
            aTopRight.y = std::min( aTopRight.y, aBotLeft.y - aMinHeight );
        }

        // push edited point edges to adjacent corners
        aTopLeft.y = aTopRight.y;
        aBotRight.x = aTopRight.x;

        break;

    case RECT_BOT_LEFT:
        if( aHoleSize.x )
        {
            // pin edited point to the bottom/left of the hole
            aBotLeft.x = std::min( aBotLeft.x, aHole.x - aHoleSize.x / 2 - aMinWidth );
            aBotLeft.y = std::max( aBotLeft.y, aHole.y + aHoleSize.y / 2 + aMinHeight );
        }
        else
        {
            // pin edited point within opposite corner
            aBotLeft.x = std::min( aBotLeft.x, aTopRight.x - aMinWidth );
            aBotLeft.y = std::max( aBotLeft.y, aTopRight.y + aMinHeight );
        }

        // push edited point edges to adjacent corners
        aBotRight.y = aBotLeft.y;
        aTopLeft.x = aBotLeft.x;

        break;

    case RECT_BOT_RIGHT:
        if( aHoleSize.x )
        {
            // pin edited point to the bottom/right of the hole
            aBotRight.x = std::max( aBotRight.x, aHole.x + aHoleSize.x / 2 + aMinWidth );
            aBotRight.y = std::max( aBotRight.y, aHole.y + aHoleSize.y / 2 + aMinHeight );
        }
        else
        {
            // pin edited point within opposite corner
            aBotRight.x = std::max( aBotRight.x, aTopLeft.x + aMinWidth );
            aBotRight.y = std::max( aBotRight.y, aTopLeft.y + aMinHeight );
        }

        // push edited point edges to adjacent corners
        aBotLeft.y = aBotRight.y;
        aTopRight.x = aBotRight.x;

        break;
    }
}


void PCB_POINT_EDITOR::editArcEndpointKeepCenter( PCB_SHAPE* aArc, const VECTOR2I& aCenter,
                                                  const VECTOR2I& aStart, const VECTOR2I& aMid,
                                                  const VECTOR2I& aEnd,
                                                  const VECTOR2I& aCursor ) const
{
    bool movingStart;

    VECTOR2I p1, p2;
    VECTOR2I  target;

    // p1 does not move, p2 does.

    if( aStart != aArc->GetStart() )
    {
        p1          = aEnd;
        p2          = aStart;
        movingStart = true;
    }
    else
    {
        p1          = aStart;
        p2          = aEnd;
        movingStart = false;
    }

    target = p2 - aCenter;

    double sqRadius = ( p1 - aCenter ).SquaredEuclideanNorm();

    p1 = p1 - aCenter;
    p2 = p2 - aCenter;

    // Circle : x^2 + y^2 = R ^ 2
    // In this coordinate system, the angular position of the cursor is (r, theta)
    // The line coming from the center of the circle is y = start.y / start.x * x
    // The intersection fulfills : x^2  = R^2 /  ( 1 + ( start.y / start.x ) ^ 2 )

    if( target.x == 0 )
    {
        p2.x = 0;
        p2.y = ( target.y > 0 ) ? sqrt( sqRadius ) : -sqrt( sqRadius );
    }
    else
    {
        double tan = target.y / static_cast<double>( target.x );

        // The divider is always greater than 1 ( cannot be 0 )
        double tmp = sqrt( sqRadius / ( 1.0 + tan * tan ) );

        // Move to the correct quadrant
        tmp   = target.x > 0 ? tmp : -tmp;
        p2.y = target.y / static_cast<double>( target.x ) * tmp;
        p2.x = tmp;
    }

    p1 = p1 + aCenter;
    p2 = p2 + aCenter;

    aArc->SetCenter( (wxPoint) aCenter );

    if( movingStart )
        aArc->SetStart( (wxPoint) aStart );
    else
        aArc->SetEnd( (wxPoint) aEnd );
}


void PCB_POINT_EDITOR::editArcMidKeepCenter( PCB_SHAPE* aArc, const VECTOR2I& aCenter,
                                             const VECTOR2I& aStart, const VECTOR2I& aMid,
                                             const VECTOR2I& aEnd, const VECTOR2I& aCursor ) const
{
    // Now, update the edit point position
    // Express the point in a circle-centered coordinate system.
    VECTOR2I start = aStart - aCenter;
    VECTOR2I end   = aEnd - aCenter;

    double sqRadius = ( aCursor - aCenter ).SquaredEuclideanNorm();

    // Special case, because the tangent would lead to +/- infinity
    if( start.x == 0 )
    {
        start.y = aCursor.y > 0 ? sqrt( sqRadius ) : -sqrt( sqRadius );
    }
    else
    {
        // Circle : x^2 + y^2 = R ^ 2
        // In this coordinate system, the angular position of the cursor is (r, theta)
        // The line coming from the center of the circle is y = start.y / start.x * x
        // The intersection fulfills : x^2  = R^2 /  ( 1 + ( start.y / start.x ) ^ 2 )

        double tan = aStart.y / static_cast<double>( start.x );
        double tmp = sqrt( sqRadius / ( 1.0 + tan * tan ) );

        // Move to the correct quadrant
        tmp      = start.x > 0 ? tmp : -tmp;
        start.y = start.y / static_cast<double>( start.x ) * tmp;
        start.x = tmp;
    }

    // Special case, because the tangent would lead to +/- infinity
    if( end.x == 0 )
    {
        end.y = aMid.y > 0 ? sqrt( sqRadius ) : -sqrt( sqRadius );
    }
    else
    {
        // Circle : x^2 + y^2 = R ^ 2
        // In this coordinate system, the angular position of the cursor is (r, theta)
        // The line coming from the center of the circle is y = start.y / start.x * x
        // The intersection fulfills : x^2  = R^2 /  ( 1 + ( start.y / start.x ) ^ 2 )

        double tan = end.y / static_cast<double>( end.x );
        double tmp = sqrt( sqRadius / ( 1.0 + tan * tan ) );

        // Move to the correct quadrant
        tmp    = end.x > 0 ? tmp : -tmp;
        end.y = end.y / static_cast<double>( end.x ) * tmp;
        end.x = tmp;
    }

    start = start + aCenter;
    end   = end + aCenter;

    aArc->SetStart( (wxPoint) start );
    aArc->SetEnd( (wxPoint) end );
}


void PCB_POINT_EDITOR::editArcMidKeepEndpoints( PCB_SHAPE* aArc, const VECTOR2I& aStart,
                                                const VECTOR2I& aEnd,
                                                const VECTOR2I& aCursor ) const
{
    // Let 'm' be the middle point of the chord between the start and end points
    VECTOR2I  m = ( aStart + aEnd ) / 2;

    // Legal midpoints lie on a vector starting just off the chord midpoint and extending out
    // past the existing midpoint.  We do not allow arc inflection while point editing.
    const int JUST_OFF = ( aStart - aEnd ).EuclideanNorm() / 100;
    VECTOR2I  v = (VECTOR2I) aArc->GetArcMid() - m;
    SEG       legal( m + v.Resize( JUST_OFF ), m + v.Resize( INT_MAX / 2 ) );
    VECTOR2I  mid = legal.NearestPoint( aCursor );

    aArc->SetArcGeometry( (wxPoint) aStart, (wxPoint) mid, (wxPoint) aEnd );
}


void PCB_POINT_EDITOR::updateItem() const
{
    EDA_ITEM* item = m_editPoints->GetParent();

    if( !item )
        return;

    switch( item->Type() )
    {
    case PCB_SHAPE_T:
    case PCB_FP_SHAPE_T:
    {
        PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( item );

        switch( shape->GetShape() )
        {
        case SHAPE_T::SEGMENT:
            if( isModified( m_editPoints->Point( SEG_START ) ) )
                shape->SetStart( (wxPoint) m_editPoints->Point( SEG_START ).GetPosition() );
            else if( isModified( m_editPoints->Point( SEG_END ) ) )
                shape->SetEnd( (wxPoint) m_editPoints->Point( SEG_END ).GetPosition() );

            break;

        case SHAPE_T::RECT:
        {
            if( isModified( m_editPoints->Point( RECT_TOP_LEFT ) ) )
            {
                shape->SetStart( (wxPoint) m_editPoints->Point( RECT_TOP_LEFT ).GetPosition() );
            }
            else if( isModified( m_editPoints->Point( RECT_TOP_RIGHT ) ) )
            {
                shape->SetStartY( m_editPoints->Point( RECT_TOP_RIGHT ).GetPosition().y );
                shape->SetEndX( m_editPoints->Point( RECT_TOP_RIGHT ).GetPosition().x );
            }
            else if( isModified( m_editPoints->Point( RECT_BOT_RIGHT ) ) )
            {
                shape->SetEnd( (wxPoint) m_editPoints->Point( RECT_BOT_RIGHT ).GetPosition() );
            }
            else if( isModified( m_editPoints->Point( RECT_BOT_LEFT ) ) )
            {
                shape->SetStartX( m_editPoints->Point( RECT_BOT_LEFT ).GetPosition().x );
                shape->SetEndY( m_editPoints->Point( RECT_BOT_LEFT ).GetPosition().y );
            }
            else if( isModified( m_editPoints->Line( RECT_TOP ) ) )
            {
                shape->SetStartY( m_editPoints->Point( RECT_TOP_LEFT ).GetPosition().y );
            }
            else if( isModified( m_editPoints->Line( RECT_LEFT ) ) )
            {
                shape->SetStartX( m_editPoints->Point( RECT_BOT_LEFT ).GetPosition().x );
            }
            else if( isModified( m_editPoints->Line( RECT_BOT ) ) )
            {
                shape->SetEndY( m_editPoints->Point( RECT_BOT_RIGHT ).GetPosition().y );
            }
            else if( isModified( m_editPoints->Line( RECT_RIGHT ) ) )
            {
                shape->SetEndX( m_editPoints->Point( RECT_TOP_RIGHT ).GetPosition().x );
            }

            for( unsigned i = 0; i < m_editPoints->LinesSize(); ++i )
            {
                if( !isModified( m_editPoints->Line( i ) ) )
                    m_editPoints->Line( i ).SetConstraint(
                            new EC_PERPLINE( m_editPoints->Line( i ) ) );
            }
        }
            break;

        case SHAPE_T::ARC:
        {
            VECTOR2I center = m_editPoints->Point( ARC_CENTER ).GetPosition();
            VECTOR2I mid = m_editPoints->Point( ARC_MID ).GetPosition();
            VECTOR2I start = m_editPoints->Point( ARC_START ).GetPosition();
            VECTOR2I end = m_editPoints->Point( ARC_END ).GetPosition();

            if( isModified( m_editPoints->Point( ARC_CENTER ) ) )
            {
                wxPoint moveVector = wxPoint( center.x, center.y ) - shape->GetCenter();
                shape->Move( moveVector );
            }
            else if( isModified( m_editPoints->Point( ARC_MID ) ) )
            {
                const VECTOR2I& cursorPos = getViewControls()->GetCursorPosition( false );

                if( m_altEditMethod )
                    editArcMidKeepCenter( shape, center, start, mid, end, cursorPos );
                else
                    editArcMidKeepEndpoints( shape, start, end, cursorPos );
            }
            else if( isModified( m_editPoints->Point( ARC_START ) )
                     || isModified( m_editPoints->Point( ARC_END ) ) )
            {
                const VECTOR2I& cursorPos = getViewControls()->GetCursorPosition();

                if( m_altEditMethod )
                    editArcEndpointKeepCenter( shape, center, start, mid, end, cursorPos );
                else
                    editArcEndpointKeepTangent( shape, center, start, mid, end, cursorPos );
            }
        }
            break;

        case SHAPE_T::CIRCLE:
        {
            const VECTOR2I& center = m_editPoints->Point( CIRC_CENTER ).GetPosition();
            const VECTOR2I& end = m_editPoints->Point( CIRC_END ).GetPosition();

            if( isModified( m_editPoints->Point( CIRC_CENTER ) ) )
            {
                wxPoint moveVector = wxPoint( center.x, center.y ) - shape->GetCenter();
                shape->Move( moveVector );
            }
            else
            {
                shape->SetEnd( wxPoint( end.x, end.y ) );
            }
        }
            break;

        case SHAPE_T::POLY:
        {
            SHAPE_POLY_SET& outline = shape->GetPolyShape();

            for( int i = 0; i < outline.TotalVertices(); ++i )
                outline.SetVertex( i, m_editPoints->Point( i ).GetPosition() );

            for( unsigned i = 0; i < m_editPoints->LinesSize(); ++i )
            {
                if( !isModified( m_editPoints->Line( i ) ) )
                    m_editPoints->Line( i ).SetConstraint(
                            new EC_PERPLINE( m_editPoints->Line( i ) ) );
            }

            validatePolygon( outline );
        }
            break;

        case SHAPE_T::BEZIER:
            if( isModified( m_editPoints->Point( BEZIER_CURVE_START ) ) )
                shape->SetStart( (wxPoint) m_editPoints->Point( BEZIER_CURVE_START ).
                                 GetPosition() );
            else if( isModified( m_editPoints->Point( BEZIER_CURVE_CONTROL_POINT1 ) ) )
                shape->SetBezierC1( (wxPoint) m_editPoints->Point( BEZIER_CURVE_CONTROL_POINT1 ).
                                    GetPosition() );
            else if( isModified( m_editPoints->Point( BEZIER_CURVE_CONTROL_POINT2 ) ) )
                shape->SetBezierC2( (wxPoint) m_editPoints->Point( BEZIER_CURVE_CONTROL_POINT2 ).
                                    GetPosition() );
            else if( isModified( m_editPoints->Point( BEZIER_CURVE_END ) ) )
                shape->SetEnd( (wxPoint) m_editPoints->Point( BEZIER_CURVE_END ).GetPosition() );

            shape->RebuildBezierToSegmentsPointsList( shape->GetWidth() );
            break;

        default:        // suppress warnings
            break;
        }

        // Update relative coordinates for footprint shapes
        if( FP_SHAPE* fpShape = dyn_cast<FP_SHAPE*>( item ) )
            fpShape->SetLocalCoord();

        break;
    }

    case PCB_PAD_T:
    {
        PAD* pad = static_cast<PAD*>( item );

        switch( pad->GetShape() )
        {
        case PAD_SHAPE::CIRCLE:
        {
            wxPoint end = (wxPoint) m_editPoints->Point( 0 ).GetPosition();
            int     diameter = (int) EuclideanNorm( end - pad->GetPosition() ) * 2;

            pad->SetSize( wxSize( diameter, diameter ) );
        }
            break;

        case PAD_SHAPE::OVAL:
        case PAD_SHAPE::TRAPEZOID:
        case PAD_SHAPE::RECT:
        case PAD_SHAPE::ROUNDRECT:
        case PAD_SHAPE::CHAMFERED_RECT:
        {
            VECTOR2I topLeft = m_editPoints->Point( RECT_TOP_LEFT ).GetPosition();
            VECTOR2I topRight = m_editPoints->Point( RECT_TOP_RIGHT ).GetPosition();
            VECTOR2I botLeft = m_editPoints->Point( RECT_BOT_LEFT ).GetPosition();
            VECTOR2I botRight = m_editPoints->Point( RECT_BOT_RIGHT ).GetPosition();
            VECTOR2I holeCenter = pad->GetPosition();
            VECTOR2I holeSize = pad->GetDrillSize();

            pinEditedCorner( getEditedPointIndex(), Mils2iu( 1 ), Mils2iu( 1 ), topLeft, topRight,
                             botLeft, botRight, holeCenter, holeSize );

            if( ( pad->GetOffset().x || pad->GetOffset().y )
                    || ( pad->GetDrillSize().x && pad->GetDrillSize().y ) )
            {
                // Keep hole pinned at the current location; adjust the pad around the hole

                wxPoint center = pad->GetPosition();
                int     dist[4];

                if( isModified( m_editPoints->Point( RECT_TOP_LEFT ) )
                        || isModified( m_editPoints->Point( RECT_BOT_RIGHT ) ) )
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

                wxSize padSize( dist[0] + dist[2], dist[1] + dist[3] );
                wxPoint deltaOffset( padSize.x / 2 - dist[2], padSize.y / 2 - dist[3] );

                if( pad->GetOrientation() == 900 || pad->GetOrientation() == 2700 )
                    std::swap( padSize.x, padSize.y );

                RotatePoint( &deltaOffset, -pad->GetOrientation() );

                pad->SetSize( padSize );
                pad->SetOffset( -deltaOffset );
            }
            else
            {
                // Keep pad position at the center of the pad shape

                int left, top, right, bottom;

                if( isModified( m_editPoints->Point( RECT_TOP_LEFT ) )
                        || isModified( m_editPoints->Point( RECT_BOT_RIGHT ) ) )
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

                wxSize padSize( abs( right - left ), abs( bottom - top ) );

                if( pad->GetOrientation() == 900 || pad->GetOrientation() == 2700 )
                    std::swap( padSize.x, padSize.y );

                pad->SetSize( padSize );
                pad->SetPosition( wxPoint( ( left + right ) / 2, ( top + bottom ) / 2 ) );
            }
        }
            break;

        default:        // suppress warnings
            break;
        }

        break;
    }

    case PCB_FP_ZONE_T:
    case PCB_ZONE_T:
    {
        ZONE* zone = static_cast<ZONE*>( item );
        zone->UnFill();
        SHAPE_POLY_SET& outline = *zone->Outline();

        for( int i = 0; i < outline.TotalVertices(); ++i )
        {
            if( outline.CVertex( i ) != m_editPoints->Point( i ).GetPosition() )
                zone->SetNeedRefill( true );

            outline.SetVertex( i, m_editPoints->Point( i ).GetPosition() );
        }

        for( unsigned i = 0; i < m_editPoints->LinesSize(); ++i )
        {
            if( !isModified( m_editPoints->Line( i ) ) )
                m_editPoints->Line( i ).SetConstraint( new EC_PERPLINE( m_editPoints->Line( i ) ) );
        }

        validatePolygon( outline );
        zone->HatchBorder();

        // TODO Refill zone when KiCad supports auto re-fill
        break;
    }

    case PCB_DIM_ALIGNED_T:
    {
        PCB_DIM_ALIGNED* dimension = static_cast<PCB_DIM_ALIGNED*>( item );

        // Check which point is currently modified and updated dimension's points respectively
        if( isModified( m_editPoints->Point( DIM_CROSSBARSTART ) ) )
        {
            VECTOR2D featureLine( m_editedPoint->GetPosition() - dimension->GetStart() );
            VECTOR2D crossBar( dimension->GetEnd() - dimension->GetStart() );

            if( featureLine.Cross( crossBar ) > 0 )
                dimension->SetHeight( -featureLine.EuclideanNorm() );
            else
                dimension->SetHeight( featureLine.EuclideanNorm() );

            dimension->Update();
        }
        else if( isModified( m_editPoints->Point( DIM_CROSSBAREND ) ) )
        {
            VECTOR2D featureLine( m_editedPoint->GetPosition() - dimension->GetEnd() );
            VECTOR2D crossBar( dimension->GetEnd() - dimension->GetStart() );

            if( featureLine.Cross( crossBar ) > 0 )
                dimension->SetHeight( -featureLine.EuclideanNorm() );
            else
                dimension->SetHeight( featureLine.EuclideanNorm() );

            dimension->Update();
        }
        else if( isModified( m_editPoints->Point( DIM_START ) ) )
        {
            dimension->SetStart( (wxPoint) m_editedPoint->GetPosition() );
            dimension->Update();

            m_editPoints->Point( DIM_CROSSBARSTART ).
                    SetConstraint( new EC_LINE( m_editPoints->Point( DIM_CROSSBARSTART ),
                                                m_editPoints->Point( DIM_START ) ) );
            m_editPoints->Point( DIM_CROSSBAREND ).
                    SetConstraint( new EC_LINE( m_editPoints->Point( DIM_CROSSBAREND ),
                                                m_editPoints->Point( DIM_END ) ) );
        }
        else if( isModified( m_editPoints->Point( DIM_END ) ) )
        {
            dimension->SetEnd( (wxPoint) m_editedPoint->GetPosition() );
            dimension->Update();

            m_editPoints->Point( DIM_CROSSBARSTART ).
                    SetConstraint( new EC_LINE( m_editPoints->Point( DIM_CROSSBARSTART ),
                                                m_editPoints->Point( DIM_START ) ) );
            m_editPoints->Point( DIM_CROSSBAREND ).
                    SetConstraint( new EC_LINE( m_editPoints->Point( DIM_CROSSBAREND ),
                                                m_editPoints->Point( DIM_END ) ) );
        }
        else if( isModified( m_editPoints->Point(DIM_TEXT ) ) )
        {
            // Force manual mode if we weren't already in it
            dimension->SetTextPositionMode( DIM_TEXT_POSITION::MANUAL );
            dimension->Text().SetPosition( wxPoint( m_editedPoint->GetPosition() ) );
            dimension->Update();
        }

        break;
    }

    case PCB_DIM_ORTHOGONAL_T:
    {
        PCB_DIM_ORTHOGONAL* dimension = static_cast<PCB_DIM_ORTHOGONAL*>( item );

        if( isModified( m_editPoints->Point( DIM_CROSSBARSTART ) ) ||
            isModified( m_editPoints->Point( DIM_CROSSBAREND ) ) )
        {
            BOX2I bounds( dimension->GetStart(), dimension->GetEnd() - dimension->GetStart() );

            const VECTOR2I& cursorPos = m_editedPoint->GetPosition();

            // Find vector from nearest dimension point to edit position
            VECTOR2I directionA( cursorPos - dimension->GetStart() );
            VECTOR2I directionB( cursorPos - dimension->GetEnd() );
            VECTOR2I direction = ( directionA < directionB ) ? directionA : directionB;

            bool     vert;
            VECTOR2D featureLine( cursorPos - dimension->GetStart() );

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

                dimension->SetOrientation( vert ? PCB_DIM_ORTHOGONAL::DIR::VERTICAL
                                                : PCB_DIM_ORTHOGONAL::DIR::HORIZONTAL );
            }
            else
            {
                vert = dimension->GetOrientation() == PCB_DIM_ORTHOGONAL::DIR::VERTICAL;
            }

            dimension->SetHeight( vert ? featureLine.x : featureLine.y );
        }
        else if( isModified( m_editPoints->Point( DIM_START ) ) )
        {
            dimension->SetStart( (wxPoint) m_editedPoint->GetPosition() );
        }
        else if( isModified( m_editPoints->Point( DIM_END ) ) )
        {
            dimension->SetEnd( (wxPoint) m_editedPoint->GetPosition() );
        }
        else if( isModified( m_editPoints->Point(DIM_TEXT ) ) )
        {
            // Force manual mode if we weren't already in it
            dimension->SetTextPositionMode( DIM_TEXT_POSITION::MANUAL );
            dimension->Text().SetPosition( wxPoint( m_editedPoint->GetPosition() ) );
        }

        dimension->Update();

        break;
    }

    case PCB_DIM_CENTER_T:
    {
        PCB_DIM_CENTER* dimension = static_cast<PCB_DIM_CENTER*>( item );

        if( isModified( m_editPoints->Point( DIM_START ) ) )
            dimension->SetStart( (wxPoint) m_editedPoint->GetPosition() );
        else if( isModified( m_editPoints->Point( DIM_END ) ) )
            dimension->SetEnd( (wxPoint) m_editedPoint->GetPosition() );

        dimension->Update();

        break;
    }

    case PCB_DIM_LEADER_T:
    {
        PCB_DIM_LEADER* dimension = static_cast<PCB_DIM_LEADER*>( item );

        if( isModified( m_editPoints->Point( DIM_START ) ) )
        {
            dimension->SetStart( (wxPoint) m_editedPoint->GetPosition() );
        }
        else if( isModified( m_editPoints->Point( DIM_END ) ) )
        {
            wxPoint newPoint( m_editedPoint->GetPosition() );
            wxPoint delta = newPoint - dimension->GetEnd();

            dimension->SetEnd( newPoint );
            dimension->Text().SetPosition( dimension->Text().GetPosition() + delta );
        }
        else if( isModified( m_editPoints->Point( DIM_TEXT ) ) )
        {
            dimension->Text().SetPosition( (wxPoint) m_editedPoint->GetPosition() );
        }

        dimension->Update();

        break;
    }

    default:
        break;
    }

    getView()->Update( item );
}


void PCB_POINT_EDITOR::finishItem()
{
    auto item = m_editPoints->GetParent();

    if( !item )
        return;

    // TODO Refill edited zones when KiCad supports auto re-fill
}


bool PCB_POINT_EDITOR::validatePolygon( SHAPE_POLY_SET& aPoly ) const
{
    bool valid = !aPoly.IsSelfIntersecting();

    if( m_statusPopup )
    {
        if( valid )
        {
            m_statusPopup->Hide();
        }
        else
        {
            wxPoint p = wxGetMousePosition() + wxPoint( 20, 20 );
            m_statusPopup->Move( p );
            m_statusPopup->PopupFor( 1500 );
        }
    }

    return valid;
}


void PCB_POINT_EDITOR::updatePoints()
{
    if( !m_editPoints )
        return;

    EDA_ITEM* item = m_editPoints->GetParent();

    if( !item )
        return;

    switch( item->Type() )
    {
    case PCB_SHAPE_T:
    case PCB_FP_SHAPE_T:
    {
        const PCB_SHAPE* shape = static_cast<const PCB_SHAPE*>( item );

        switch( shape->GetShape() )
        {
        case SHAPE_T::SEGMENT:
            m_editPoints->Point( SEG_START ).SetPosition( shape->GetStart() );
            m_editPoints->Point( SEG_END ).SetPosition( shape->GetEnd() );
            break;

        case SHAPE_T::RECT:
            m_editPoints->Point( RECT_TOP_LEFT ).SetPosition( shape->GetStart() );
            m_editPoints->Point( RECT_TOP_RIGHT ).SetPosition( shape->GetEnd().x,
                                                               shape->GetStart().y );
            m_editPoints->Point( RECT_BOT_RIGHT ).SetPosition( shape->GetEnd() );
            m_editPoints->Point( RECT_BOT_LEFT ).SetPosition( shape->GetStart().x,
                                                              shape->GetEnd().y );
            break;

        case SHAPE_T::ARC:
            m_editPoints->Point( ARC_CENTER ).SetPosition( shape->GetCenter() );
            m_editPoints->Point( ARC_START ).SetPosition( shape->GetStart() );
            m_editPoints->Point( ARC_MID ).SetPosition( shape->GetArcMid() );
            m_editPoints->Point( ARC_END ).SetPosition( shape->GetEnd() );
            break;

        case SHAPE_T::CIRCLE:
            m_editPoints->Point( CIRC_CENTER ).SetPosition( shape->GetCenter() );
            m_editPoints->Point( CIRC_END ).SetPosition( shape->GetEnd() );
            break;

        case SHAPE_T::POLY:
        {
            std::vector<wxPoint> points;
            shape->DupPolyPointsList( points );

            if( m_editPoints->PointsSize() != (unsigned) points.size() )
            {
                getView()->Remove( m_editPoints.get() );
                m_editedPoint = nullptr;

                m_editPoints = makePoints( item );

                if( m_editPoints )
                    getView()->Add( m_editPoints.get() );
            }
            else
            {
                for( unsigned i = 0; i < points.size(); i++ )
                    m_editPoints->Point( i ).SetPosition( points[i] );
            }

            break;
        }

        case SHAPE_T::BEZIER:
            m_editPoints->Point( BEZIER_CURVE_START ).SetPosition( shape->GetStart() );
            m_editPoints->Point( BEZIER_CURVE_CONTROL_POINT1 ).SetPosition( shape->GetBezierC1() );
            m_editPoints->Point( BEZIER_CURVE_CONTROL_POINT2 ).SetPosition( shape->GetBezierC2() );
            m_editPoints->Point( BEZIER_CURVE_END ).SetPosition( shape->GetEnd() );
            break;

        default:        // suppress warnings
            break;
        }

        break;
    }

    case PCB_PAD_T:
    {
        const PAD* pad = static_cast<const PAD*>( item );
        bool       locked = pad->GetParent() && pad->IsLocked();
        wxPoint    shapePos = pad->ShapePos();
        wxPoint    halfSize( pad->GetSize().x / 2, pad->GetSize().y / 2 );

        switch( pad->GetShape() )
        {
        case PAD_SHAPE::CIRCLE:
        {
            int target = locked ? 0 : 1;

            // Careful; pad shape is mutable...
            if( int( m_editPoints->PointsSize() ) != target )
            {
                getView()->Remove( m_editPoints.get() );
                m_editedPoint = nullptr;

                m_editPoints = makePoints( item );

                if( m_editPoints )
                    getView()->Add( m_editPoints.get() );
            }
            else if( target == 1 )
            {
                VECTOR2I vec = m_editPoints->Point( 0 ).GetPosition() - shapePos;
                vec.Resize( halfSize.x );

                m_editPoints->Point( 0 ).SetPosition( vec + shapePos );
            }
        }
            break;

        case PAD_SHAPE::OVAL:
        case PAD_SHAPE::TRAPEZOID:
        case PAD_SHAPE::RECT:
        case PAD_SHAPE::ROUNDRECT:
        case PAD_SHAPE::CHAMFERED_RECT:
        {
            // Careful; pad shape and orientation are mutable...
            int target = locked || (int) pad->GetOrientation() % 900 > 0 ? 0 : 4;

            if( int( m_editPoints->PointsSize() ) != target )
            {
                getView()->Remove( m_editPoints.get() );
                m_editedPoint = nullptr;

                m_editPoints = makePoints( item );

                if( m_editPoints )
                    getView()->Add( m_editPoints.get() );
            }
            else if( target == 4 )
            {
                if( pad->GetOrientation() == 900 || pad->GetOrientation() == 2700 )
                    std::swap( halfSize.x, halfSize.y );

                m_editPoints->Point( RECT_TOP_LEFT ).SetPosition( shapePos - halfSize );
                m_editPoints->Point( RECT_TOP_RIGHT ).SetPosition( wxPoint( shapePos.x + halfSize.x,
                                                                            shapePos.y - halfSize.y ) );
                m_editPoints->Point( RECT_BOT_RIGHT ).SetPosition( shapePos + halfSize );
                m_editPoints->Point( RECT_BOT_LEFT ).SetPosition( wxPoint( shapePos.x - halfSize.x,
                                                                           shapePos.y + halfSize.y ) );
            }

            break;
        }

        default:        // suppress warnings
            break;
        }
    }
        break;

    case PCB_FP_ZONE_T:
    case PCB_ZONE_T:
    {
        ZONE*                 zone = static_cast<ZONE*>( item );
        const SHAPE_POLY_SET* outline = zone->Outline();

        if( m_editPoints->PointsSize() != (unsigned) outline->TotalVertices() )
        {
            getView()->Remove( m_editPoints.get() );
            m_editedPoint = nullptr;

            m_editPoints = makePoints( item );

            if( m_editPoints )
                getView()->Add( m_editPoints.get() );
        }
        else
        {
            for( int i = 0; i < outline->TotalVertices(); ++i )
                m_editPoints->Point( i ).SetPosition( outline->CVertex( i ) );
        }

        break;
    }

    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_ORTHOGONAL_T:
    {
        const PCB_DIM_ALIGNED* dimension = static_cast<const PCB_DIM_ALIGNED*>( item );

        m_editPoints->Point( DIM_START ).SetPosition( dimension->GetStart() );
        m_editPoints->Point( DIM_END ).SetPosition( dimension->GetEnd() );
        m_editPoints->Point( DIM_TEXT ).SetPosition( dimension->Text().GetPosition() );
        m_editPoints->Point( DIM_CROSSBARSTART ).SetPosition( dimension->GetCrossbarStart() );
        m_editPoints->Point( DIM_CROSSBAREND ).SetPosition( dimension->GetCrossbarEnd() );
        break;
    }

    case PCB_DIM_CENTER_T:
    {
        const PCB_DIM_CENTER* dimension = static_cast<const PCB_DIM_CENTER*>( item );

        m_editPoints->Point( DIM_START ).SetPosition( dimension->GetStart() );
        m_editPoints->Point( DIM_END ).SetPosition( dimension->GetEnd() );
        break;
    }

    case PCB_DIM_LEADER_T:
    {
        const PCB_DIM_LEADER* dimension = static_cast<const PCB_DIM_LEADER*>( item );

        m_editPoints->Point( DIM_START ).SetPosition( dimension->GetStart() );
        m_editPoints->Point( DIM_END ).SetPosition( dimension->GetEnd() );
        m_editPoints->Point( DIM_TEXT ).SetPosition( dimension->Text().GetPosition() );
        break;
    }

    default:
        break;
    }

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
        case PCB_FP_ZONE_T:
            isPoly = true;
            break;

        case PCB_SHAPE_T:
        case PCB_FP_SHAPE_T:
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
    EDA_ITEM* item = m_editPoints->GetParent();

    switch( item->Type() )
    {
    case PCB_SHAPE_T:
    case PCB_FP_SHAPE_T:
        switch( static_cast<const PCB_SHAPE*>( item )->GetShape() )
        {
        case SHAPE_T::SEGMENT:
            return *( m_editPoints->Next( *m_editedPoint ) );     // select the other end of line

        case SHAPE_T::ARC:
        case SHAPE_T::CIRCLE:
            return m_editPoints->Point( CIRC_CENTER );

        default:        // suppress warnings
            break;
        }

        break;

    case PCB_DIM_ALIGNED_T:
    {
        // Constraint for crossbar
        if( isModified( m_editPoints->Point( DIM_START ) ) )
            return m_editPoints->Point( DIM_END );

        else if( isModified( m_editPoints->Point( DIM_END ) ) )
            return m_editPoints->Point( DIM_START );

        else
            return EDIT_POINT( m_editedPoint->GetPosition() );      // no constraint

        break;
    }

    case PCB_DIM_CENTER_T:
    {
        if( isModified( m_editPoints->Point( DIM_END ) ) )
            return m_editPoints->Point( DIM_START );

        break;
    }

    default:
        break;
    }

    // In any other case we may align item to its original position
    return m_original;
}


bool PCB_POINT_EDITOR::canAddCorner( const EDA_ITEM& aItem )
{
    const auto type = aItem.Type();

    // Works only for zones and line segments
    if( type == PCB_ZONE_T || type == PCB_FP_ZONE_T )
        return true;

    if( type == PCB_SHAPE_T || type == PCB_FP_SHAPE_T )
    {
        const PCB_SHAPE& shape = static_cast<const PCB_SHAPE&>( aItem );
        return shape.GetShape() == SHAPE_T::SEGMENT || shape.GetShape() == SHAPE_T::POLY;
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
    case PCB_FP_ZONE_T:
        polyset = static_cast<ZONE*>( item )->Outline();
        break;

    case PCB_SHAPE_T:
    case PCB_FP_SHAPE_T:
        if( static_cast<PCB_SHAPE*>( item )->GetShape() == SHAPE_T::POLY )
            polyset = &static_cast<PCB_SHAPE*>( item )->GetPolyShape();
        else
            return false;

        break;

    default:
        return false;
    }

    auto vertex = findVertex( *polyset, *m_editedPoint );

    if( !vertex.first )
        return false;

    const auto& vertexIdx = vertex.second;

    // Check if there are enough vertices so one can be removed without
    // degenerating the polygon.
    // The first condition allows one to remove all corners from holes (when
    // there are only 2 vertices left, a hole is removed).
    if( vertexIdx.m_contour == 0 &&
        polyset->Polygon( vertexIdx.m_polygon )[vertexIdx.m_contour].PointCount() <= 3 )
        return false;

    // Remove corner does not work with lines
    if( dynamic_cast<EDIT_LINE*>( m_editedPoint ) )
        return false;

    return m_editedPoint != nullptr;
}


int PCB_POINT_EDITOR::addCorner( const TOOL_EVENT& aEvent )
{
    if( !m_editPoints )
        return 0;

    EDA_ITEM* item = m_editPoints->GetParent();
    PCB_BASE_EDIT_FRAME* frame = getEditFrame<PCB_BASE_EDIT_FRAME>();
    const VECTOR2I& cursorPos = getViewControls()->GetCursorPosition();

    // called without an active edited polygon
    if( !item || !canAddCorner( *item ) )
        return 0;

    PCB_SHAPE* graphicItem = dynamic_cast<PCB_SHAPE*>( item );
    BOARD_COMMIT commit( frame );

    if( item->Type() == PCB_ZONE_T || item->Type() == PCB_FP_ZONE_T
        || ( graphicItem && graphicItem->GetShape() == SHAPE_T::POLY ) )
    {
        unsigned int nearestIdx = 0;
        unsigned int nextNearestIdx = 0;
        unsigned int nearestDist = INT_MAX;
        unsigned int firstPointInContour = 0;
        SHAPE_POLY_SET* zoneOutline;

        if( item->Type() == PCB_ZONE_T || item->Type() == PCB_FP_ZONE_T )
        {
            ZONE* zone = static_cast<ZONE*>( item );
            zoneOutline = zone->Outline();
            zone->SetNeedRefill( true );
        }
        else
            zoneOutline = &( graphicItem->GetPolyShape() );

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
        auto&    sideOrigin = zoneOutline->CVertex( nearestIdx );
        auto&    sideEnd = zoneOutline->CVertex( nextNearestIdx );
        SEG nearestSide( sideOrigin, sideEnd );
        VECTOR2I nearestPoint = nearestSide.NearestPoint( cursorPos );

        // Do not add points that have the same coordinates as ones that already belong to polygon
        // instead, add a point in the middle of the side
        if( nearestPoint == sideOrigin || nearestPoint == sideEnd )
            nearestPoint = ( sideOrigin + sideEnd ) / 2;

        zoneOutline->InsertVertex( nextNearestIdx, nearestPoint );

        // We re-hatch the filled zones but not polygons
        if( item->Type() == PCB_ZONE_T || item->Type() == PCB_FP_ZONE_T )
            static_cast<ZONE*>( item )->HatchBorder();


        commit.Push( _( "Add a zone corner" ) );
    }
    else if( graphicItem && graphicItem->GetShape() == SHAPE_T::SEGMENT )
    {
        commit.Modify( graphicItem );

        SEG seg( graphicItem->GetStart(), graphicItem->GetEnd() );
        VECTOR2I nearestPoint = seg.NearestPoint( cursorPos );

        // Move the end of the line to the break point..
        graphicItem->SetEnd( wxPoint( nearestPoint.x, nearestPoint.y ) );

        if( graphicItem->Type() == PCB_FP_SHAPE_T )
            static_cast<FP_SHAPE*>( graphicItem )->SetLocalCoord();

        // and add another one starting from the break point
        PCB_SHAPE* newSegment;

        if( item->Type() == PCB_FP_SHAPE_T )
        {
            FP_SHAPE* edge = static_cast<FP_SHAPE*>( graphicItem );
            assert( edge->GetParent()->Type() == PCB_FOOTPRINT_T );
            newSegment = new FP_SHAPE( *edge );
        }
        else
        {
            newSegment = new PCB_SHAPE( *graphicItem );
        }

        newSegment->ClearSelected();
        newSegment->SetStart( wxPoint( nearestPoint.x, nearestPoint.y ) );
        newSegment->SetEnd( wxPoint( seg.B.x, seg.B.y ) );

        if( newSegment->Type() == PCB_FP_SHAPE_T )
            static_cast<FP_SHAPE*>( newSegment )->SetLocalCoord();

        commit.Add( newSegment );
        commit.Push( _( "Split segment" ) );
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

    if( item->Type() == PCB_ZONE_T || item->Type() == PCB_FP_ZONE_T )
    {
        ZONE* zone = static_cast<ZONE*>( item );
        polygon = zone->Outline();
        zone->SetNeedRefill( true );
    }
    else if( item->Type() == PCB_FP_SHAPE_T || item->Type() == PCB_SHAPE_T )
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
            validatePolygon( *polygon );
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
                m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
                commit.Remove( item );
            }
        }

        setEditedPoint( nullptr );

        commit.Push( _( "Remove a zone/polygon corner" ) );

        // Refresh zone hatching
        if( item->Type() == PCB_ZONE_T || item->Type() == PCB_FP_ZONE_T )
            static_cast<ZONE*>( item )->HatchBorder();

        updatePoints();
    }

    return 0;
}


int PCB_POINT_EDITOR::modifiedSelection( const TOOL_EVENT& aEvent )
{
    updatePoints();
    return 0;
}


int PCB_POINT_EDITOR::changeEditMethod( const TOOL_EVENT& aEvent )
{
    m_altEditMethod = !m_altEditMethod;
    return 0;
}


void PCB_POINT_EDITOR::setTransitions()
{
    Go( &PCB_POINT_EDITOR::OnSelectionChange, ACTIONS::activatePointEditor.MakeEvent() );
    Go( &PCB_POINT_EDITOR::addCorner, PCB_ACTIONS::pointEditorAddCorner.MakeEvent() );
    Go( &PCB_POINT_EDITOR::removeCorner, PCB_ACTIONS::pointEditorRemoveCorner.MakeEvent() );
    Go( &PCB_POINT_EDITOR::modifiedSelection, EVENTS::SelectedItemsModified );
    Go( &PCB_POINT_EDITOR::OnSelectionChange, EVENTS::SelectedEvent );
    Go( &PCB_POINT_EDITOR::OnSelectionChange, EVENTS::UnselectedEvent );
    Go( &PCB_POINT_EDITOR::changeEditMethod, ACTIONS::changeEditMethod.MakeEvent() );
    Go( &PCB_POINT_EDITOR::OnSelectionChange, EVENTS::InhibitSelectionEditing );
    Go( &PCB_POINT_EDITOR::OnSelectionChange, EVENTS::UninhibitSelectionEditing );
}
