/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2019 CERN
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
using namespace std::placeholders;
#include <tool/tool_manager.h>
#include <view/view_controls.h>
#include <gal/graphics_abstraction_layer.h>
#include <geometry/seg.h>
#include <confirm.h>
#include "pcb_actions.h"
#include "selection_tool.h"
#include "point_editor.h"
#include "grid_helper.h"
#include <board_commit.h>
#include <bitmaps.h>
#include <status_popup.h>
#include <pcb_edit_frame.h>
#include <class_edge_mod.h>
#include <class_dimension.h>
#include <class_zone.h>
#include <connectivity/connectivity_data.h>
#include <widgets/progress_reporter.h>

// Few constants to avoid using bare numbers for point indices
enum SEG_POINTS
{
    SEG_START, SEG_END
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
    DIM_CROSSBARO,
    DIM_CROSSBARF,
    DIM_FEATUREGO,
    DIM_FEATUREDO,
};

class EDIT_POINTS_FACTORY
{
private:

    static void buildForPolyOutline( std::shared_ptr<EDIT_POINTS> points,
                                     const SHAPE_POLY_SET* aOutline, KIGFX::GAL* aGal )
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
            {
                points->AddLine( points->Point( i ),
                        points->Point( points->GetContourStartIdx( i ) ) );
            }
            else
            {
                points->AddLine( points->Point( i ), points->Point( i + 1 ) );
            }

            points->Line( i ).SetConstraint( new EC_SNAPLINE( points->Line( i ),
                            std::bind( &KIGFX::GAL::GetGridPoint, aGal, _1 ) ) );
        }

        // The last missing line, connecting the last and the first polygon point
        points->AddLine( points->Point( cornersCount - 1 ),
                points->Point( points->GetContourStartIdx( cornersCount - 1 ) ) );

        points->Line( points->LinesSize() - 1 ).SetConstraint(
                new EC_SNAPLINE( points->Line( points->LinesSize() - 1 ),
                        std::bind( &KIGFX::GAL::GetGridPoint, aGal, _1 ) ) );
    }

public:
    static std::shared_ptr<EDIT_POINTS> Make( EDA_ITEM* aItem, KIGFX::GAL* aGal )
    {
        std::shared_ptr<EDIT_POINTS> points = std::make_shared<EDIT_POINTS>( aItem );

        if( !aItem )
            return points;

        // Generate list of edit points basing on the item type
        switch( aItem->Type() )
        {
            case PCB_LINE_T:
            case PCB_MODULE_EDGE_T:
            {
                const DRAWSEGMENT* segment = static_cast<const DRAWSEGMENT*>( aItem );

                switch( segment->GetShape() )
                {
                case S_SEGMENT:
                    points->AddPoint( segment->GetStart() );
                    points->AddPoint( segment->GetEnd() );
                    break;

                case S_ARC:
                    points->AddPoint( segment->GetCenter() );
                    points->AddPoint( segment->GetArcStart() );
                    points->AddPoint( segment->GetArcMid() );
                    points->AddPoint( segment->GetArcEnd() );

                    // Set constraints
                    // Arc end has to stay at the same radius as the start
                    points->Point( ARC_END ).SetConstraint( new EC_CIRCLE( points->Point( ARC_END ),
                                                                           points->Point( ARC_CENTER ),
                                                                           points->Point( ARC_START ) ) );

                    points->Point( ARC_MID ).SetConstraint( new EC_LINE( points->Point( ARC_MID ),
                                                                         points->Point( ARC_CENTER ) ) );
                    break;

                case S_CIRCLE:
                    points->AddPoint( segment->GetCenter() );
                    points->AddPoint( segment->GetEnd() );

                    break;

                case S_POLYGON:
                    buildForPolyOutline( points, &segment->GetPolyShape(), aGal );
                    break;

                case S_CURVE:
                    points->AddPoint( segment->GetStart() );
                    points->AddPoint( segment->GetBezControl1() );
                    points->AddPoint( segment->GetBezControl2() );
                    points->AddPoint( segment->GetEnd() );
                    break;

                default:        // suppress warnings
                    break;
                }

                break;
            }

            case PCB_ZONE_AREA_T:
            {
                auto zone = static_cast<const ZONE_CONTAINER*>( aItem );
                buildForPolyOutline( points, zone->Outline(), aGal );
                break;
            }

            case PCB_DIMENSION_T:
            {
                const DIMENSION* dimension = static_cast<const DIMENSION*>( aItem );

                points->AddPoint( dimension->m_crossBarO );
                points->AddPoint( dimension->m_crossBarF );
                points->AddPoint( dimension->m_featureLineGO );
                points->AddPoint( dimension->m_featureLineDO );

                // Dimension height setting - edit points should move only along the feature lines
                points->Point( DIM_CROSSBARO ).SetConstraint( new EC_LINE( points->Point( DIM_CROSSBARO ),
                                                                           points->Point( DIM_FEATUREGO ) ) );
                points->Point( DIM_CROSSBARF ).SetConstraint( new EC_LINE( points->Point( DIM_CROSSBARF ),
                                                                           points->Point( DIM_FEATUREDO ) ) );

                break;
            }

            default:
                points.reset();
                break;
        }

        return points;
    }

private:
    EDIT_POINTS_FACTORY() {};
};


POINT_EDITOR::POINT_EDITOR() :
    PCB_TOOL_BASE( "pcbnew.PointEditor" ),
    m_selectionTool( NULL ),
    m_editedPoint( NULL ),
    m_original( VECTOR2I( 0, 0 ) ),
    m_altConstrainer( VECTOR2I( 0, 0 ) ),
    m_refill( false )
{
}


void POINT_EDITOR::Reset( RESET_REASON aReason )
{
    m_refill = false;
    m_editPoints.reset();
    m_altConstraint.reset();
    getViewControls()->SetAutoPan( false );

    m_statusPopup.reset( new STATUS_TEXT_POPUP( getEditFrame<PCB_BASE_EDIT_FRAME>() ) );
    m_statusPopup->SetTextColor( wxColour( 255, 0, 0 ) );
    m_statusPopup->SetText( _( "Self-intersecting polygons are not allowed." ) );
}


bool POINT_EDITOR::Init()
{
    // Find the selection tool, so they can cooperate
    m_selectionTool = m_toolMgr->GetTool<SELECTION_TOOL>();

    wxASSERT_MSG( m_selectionTool, _( "pcbnew.InteractiveSelection tool is not available" ) );

    auto& menu = m_selectionTool->GetToolMenu().GetMenu();
    menu.AddItem( PCB_ACTIONS::pointEditorAddCorner, POINT_EDITOR::addCornerCondition );
    menu.AddItem( PCB_ACTIONS::pointEditorRemoveCorner,
                  std::bind( &POINT_EDITOR::removeCornerCondition, this, _1 ) );

    return true;
}


void POINT_EDITOR::updateEditedPoint( const TOOL_EVENT& aEvent )
{
    EDIT_POINT* point;

    if( aEvent.IsMotion() )
    {
        point = m_editPoints->FindPoint( aEvent.Position(), getView() );
    }
    else if( aEvent.IsDrag( BUT_LEFT ) )
    {
        point = m_editPoints->FindPoint( aEvent.DragOrigin(), getView() );
    }
    else
    {
        point = m_editPoints->FindPoint( getViewControls()->GetCursorPosition(), getView() );
    }

    if( m_editedPoint != point )
        setEditedPoint( point );
}


int POINT_EDITOR::OnSelectionChange( const TOOL_EVENT& aEvent )
{
    if( !m_selectionTool )
        return 0;

    const PCBNEW_SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.Size() != 1 || selection.Front()->GetEditFlags() )
        return 0;

    Activate();

    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    KIGFX::VIEW* view = getView();
    PCB_BASE_EDIT_FRAME* editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();

    controls->ShowCursor( true );

    GRID_HELPER grid( editFrame );
    BOARD_ITEM* item = static_cast<BOARD_ITEM*>( selection.Front() );

    if( !item )
        return 0;

    m_editPoints = EDIT_POINTS_FACTORY::Make( item, getView()->GetGAL() );

    if( !m_editPoints )
        return 0;

    view->Add( m_editPoints.get() );
    setEditedPoint( nullptr );
    updateEditedPoint( aEvent );
    m_refill = false;
    bool inDrag = false;

    BOARD_COMMIT commit( editFrame );
    LSET snapLayers = item->GetLayerSet();

    if( item->Type() == PCB_DIMENSION_T )
        snapLayers = LSET::AllLayersMask();

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( !evt->Modifier( MD_ALT ) );
        controls->SetSnapping( !evt->Modifier( MD_ALT ) );

        if( !m_editPoints || evt->IsSelectionEvent() )
            break;

        if ( !inDrag )
            updateEditedPoint( *evt );

        if( evt->IsDrag( BUT_LEFT ) && m_editedPoint )
        {
            if( !inDrag )
            {
                commit.StageItems( selection, CHT_MODIFY );

                controls->ForceCursorPosition( false );
                m_original = *m_editedPoint;    // Save the original position
                controls->SetAutoPan( true );
                inDrag = true;
                grid.SetAuxAxes( true, m_original.GetPosition() );
            }

            //TODO: unify the constraints to solve simultaneously instead of sequentially
            m_editedPoint->SetPosition( grid.BestSnapAnchor( evt->Position(),
                                                             snapLayers, { item } ) );
            bool enableAltConstraint = !!evt->Modifier( MD_CTRL );

            if( enableAltConstraint != (bool) m_altConstraint )  // alternative constraint
                setAltConstraint( enableAltConstraint );

            if( m_altConstraint )
                m_altConstraint->Apply();
            else
                m_editedPoint->ApplyConstraint();

            m_editedPoint->SetPosition( grid.BestSnapAnchor( m_editedPoint->GetPosition(),
                                                             snapLayers, { item } ) );

            updateItem();
            updatePoints();
        }

        else if( inDrag && evt->IsMouseUp( BUT_LEFT ) )
        {
            controls->SetAutoPan( false );
            setAltConstraint( false );

            commit.Push( _( "Drag a corner" ) );
            inDrag = false;
            m_refill = true;
        }

        else if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            if( inDrag )      // Restore the last change
                commit.Revert();
            else if( evt->IsCancelInteractive() )
                break;

            if( evt->IsActivate() && !evt->IsMoveTool() )
                break;
        }

        else
            evt->SetPassEvent();
    }

    if( m_editPoints )
    {
        view->Remove( m_editPoints.get() );

        finishItem();
        m_editPoints.reset();
    }

    frame()->UpdateMsgPanel();

    return 0;
}


void POINT_EDITOR::updateItem() const
{
    EDA_ITEM* item = m_editPoints->GetParent();

    if( !item )
        return;

    switch( item->Type() )
    {
    case PCB_LINE_T:
    case PCB_MODULE_EDGE_T:
    {
        DRAWSEGMENT* segment = static_cast<DRAWSEGMENT*>( item );
        switch( segment->GetShape() )
        {
        case S_SEGMENT:
            if( isModified( m_editPoints->Point( SEG_START ) ) )
                segment->SetStart( wxPoint( m_editPoints->Point( SEG_START ).GetPosition().x,
                                            m_editPoints->Point( SEG_START ).GetPosition().y ) );

            else if( isModified( m_editPoints->Point( SEG_END ) ) )
                segment->SetEnd( wxPoint( m_editPoints->Point( SEG_END ).GetPosition().x,
                                          m_editPoints->Point( SEG_END ).GetPosition().y ) );

            break;

        case S_ARC:
        {
            VECTOR2I center = m_editPoints->Point( ARC_CENTER ).GetPosition();
            VECTOR2I mid = m_editPoints->Point( ARC_MID ).GetPosition();
            VECTOR2I start = m_editPoints->Point( ARC_START ).GetPosition();
            VECTOR2I end = m_editPoints->Point( ARC_END ).GetPosition();

            if( center != segment->GetCenter() )
            {
                wxPoint moveVector = wxPoint( center.x, center.y ) - segment->GetCenter();
                segment->Move( moveVector );

                m_editPoints->Point( ARC_START ).SetPosition( segment->GetArcStart() );
                m_editPoints->Point( ARC_END ).SetPosition( segment->GetArcEnd() );
                m_editPoints->Point( ARC_MID ).SetPosition( segment->GetArcMid() );
            }
            else
            {
                if( mid != segment->GetArcMid() )
                {
                    center = GetArcCenter( start, mid, end );
                    segment->SetCenter( wxPoint( center.x, center.y ) );
                    m_editPoints->Point( ARC_CENTER ).SetPosition( center );
                }

                segment->SetArcStart( wxPoint( start.x, start.y ) );

                VECTOR2D startLine = start - center;
                VECTOR2D endLine = end - center;
                double newAngle = RAD2DECIDEG( endLine.Angle() - startLine.Angle() );

                // Adjust the new angle to (counter)clockwise setting
                bool clockwise = ( segment->GetAngle() > 0 );

                if( clockwise && newAngle < 0.0 )
                    newAngle += 3600.0;
                else if( !clockwise && newAngle > 0.0 )
                    newAngle -= 3600.0;

                segment->SetAngle( newAngle );
            }

            break;
        }

        case S_CIRCLE:
        {
            const VECTOR2I& center = m_editPoints->Point( CIRC_CENTER ).GetPosition();
            const VECTOR2I& end = m_editPoints->Point( CIRC_END ).GetPosition();

            if( isModified( m_editPoints->Point( CIRC_CENTER ) ) )
            {
                wxPoint moveVector = wxPoint( center.x, center.y ) - segment->GetCenter();
                segment->Move( moveVector );
            }
            else
            {
                segment->SetEnd( wxPoint( end.x, end.y ) );
            }

            break;
        }

        case S_POLYGON:
        {
            SHAPE_POLY_SET& outline = segment->GetPolyShape();

            for( int i = 0; i < outline.TotalVertices(); ++i )
                outline.Vertex( i ) = m_editPoints->Point( i ).GetPosition();

            validatePolygon( outline );
            break;
        }

        case S_CURVE:
            if( isModified( m_editPoints->Point( BEZIER_CURVE_START ) ) )
                segment->SetStart( wxPoint( m_editPoints->Point( BEZIER_CURVE_START ).GetPosition().x,
                                            m_editPoints->Point( BEZIER_CURVE_START ).GetPosition().y ) );
            else if( isModified( m_editPoints->Point( BEZIER_CURVE_CONTROL_POINT1 ) ) )
                segment->SetBezControl1( wxPoint( m_editPoints->Point( BEZIER_CURVE_CONTROL_POINT1 ).GetPosition().x,
                                          m_editPoints->Point( BEZIER_CURVE_CONTROL_POINT1 ).GetPosition().y ) );
            else if( isModified( m_editPoints->Point( BEZIER_CURVE_CONTROL_POINT2 ) ) )
                segment->SetBezControl2( wxPoint( m_editPoints->Point( BEZIER_CURVE_CONTROL_POINT2 ).GetPosition().x,
                                            m_editPoints->Point( BEZIER_CURVE_CONTROL_POINT2 ).GetPosition().y ) );
            else if( isModified( m_editPoints->Point( BEZIER_CURVE_END ) ) )
                segment->SetEnd( wxPoint( m_editPoints->Point( BEZIER_CURVE_END ).GetPosition().x,
                                          m_editPoints->Point( BEZIER_CURVE_END ).GetPosition().y ) );

            segment->RebuildBezierToSegmentsPointsList( segment->GetWidth() );
            break;

        default:        // suppress warnings
            break;
        }

        // Update relative coordinates for module edges
        if( EDGE_MODULE* edge = dyn_cast<EDGE_MODULE*>( item ) )
            edge->SetLocalCoord();

        break;
    }

    case PCB_ZONE_AREA_T:
    {
        ZONE_CONTAINER* zone = static_cast<ZONE_CONTAINER*>( item );
        zone->ClearFilledPolysList();
        SHAPE_POLY_SET& outline = *zone->Outline();

        for( int i = 0; i < outline.TotalVertices(); ++i )
        {
            if( outline.Vertex( i ) != m_editPoints->Point( i ).GetPosition() )
                zone->SetNeedRefill( true );

            outline.Vertex( i ) = m_editPoints->Point( i ).GetPosition();
        }

        validatePolygon( outline );
        zone->Hatch();
        break;
    }

    case PCB_DIMENSION_T:
    {
        DIMENSION* dimension = static_cast<DIMENSION*>( item );

        // Check which point is currently modified and updated dimension's points respectively
        if( isModified( m_editPoints->Point( DIM_CROSSBARO ) ) )
        {
            VECTOR2D featureLine( m_editedPoint->GetPosition() - dimension->GetOrigin() );
            VECTOR2D crossBar( dimension->GetEnd() - dimension->GetOrigin() );

            if( featureLine.Cross( crossBar ) > 0 )
                dimension->SetHeight( -featureLine.EuclideanNorm() );
            else
                dimension->SetHeight( featureLine.EuclideanNorm() );
        }

        else if( isModified( m_editPoints->Point( DIM_CROSSBARF ) ) )
        {
            VECTOR2D featureLine( m_editedPoint->GetPosition() - dimension->GetEnd() );
            VECTOR2D crossBar( dimension->GetEnd() - dimension->GetOrigin() );

            if( featureLine.Cross( crossBar ) > 0 )
                dimension->SetHeight( -featureLine.EuclideanNorm() );
            else
                dimension->SetHeight( featureLine.EuclideanNorm() );
        }

        else if( isModified( m_editPoints->Point( DIM_FEATUREGO ) ) )
        {
            dimension->SetOrigin( wxPoint( m_editedPoint->GetPosition().x, m_editedPoint->GetPosition().y ) );
            m_editPoints->Point( DIM_CROSSBARO ).SetConstraint( new EC_LINE( m_editPoints->Point( DIM_CROSSBARO ),
                                                                             m_editPoints->Point( DIM_FEATUREGO ) ) );
            m_editPoints->Point( DIM_CROSSBARF ).SetConstraint( new EC_LINE( m_editPoints->Point( DIM_CROSSBARF ),
                                                                             m_editPoints->Point( DIM_FEATUREDO ) ) );
        }

        else if( isModified( m_editPoints->Point( DIM_FEATUREDO ) ) )
        {
            dimension->SetEnd( wxPoint( m_editedPoint->GetPosition().x, m_editedPoint->GetPosition().y ) );
            m_editPoints->Point( DIM_CROSSBARO ).SetConstraint( new EC_LINE( m_editPoints->Point( DIM_CROSSBARO ),
                                                                             m_editPoints->Point( DIM_FEATUREGO ) ) );
            m_editPoints->Point( DIM_CROSSBARF ).SetConstraint( new EC_LINE( m_editPoints->Point( DIM_CROSSBARF ),
                                                                             m_editPoints->Point( DIM_FEATUREDO ) ) );
        }

        break;
    }

    default:
        break;
    }

    if( frame() )
        frame()->SetMsgPanel( item );
}


void POINT_EDITOR::finishItem()
{
    auto item = m_editPoints->GetParent();

    if( !item )
        return;

    if( item->Type() == PCB_ZONE_AREA_T )
    {
        auto zone = static_cast<ZONE_CONTAINER*>( item );

        if( zone->IsFilled() && m_refill && zone->NeedRefill() )
            m_toolMgr->RunAction( PCB_ACTIONS::zoneFill, true, zone );
    }
}


bool POINT_EDITOR::validatePolygon( SHAPE_POLY_SET& aPoly ) const
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


void POINT_EDITOR::updatePoints()
{
    if( !m_editPoints )
        return;

    EDA_ITEM* item = m_editPoints->GetParent();

    if( !item )
        return;

    switch( item->Type() )
    {
    case PCB_LINE_T:
    case PCB_MODULE_EDGE_T:
    {
        const DRAWSEGMENT* segment = static_cast<const DRAWSEGMENT*>( item );

        switch( segment->GetShape() )
        {
        case S_SEGMENT:
            m_editPoints->Point( SEG_START ).SetPosition( segment->GetStart() );
            m_editPoints->Point( SEG_END ).SetPosition( segment->GetEnd() );
            break;

        case S_ARC:
            m_editPoints->Point( ARC_CENTER ).SetPosition( segment->GetCenter() );
            m_editPoints->Point( ARC_START ).SetPosition( segment->GetArcStart() );
            m_editPoints->Point( ARC_MID ).SetPosition( segment->GetArcMid() );
            m_editPoints->Point( ARC_END ).SetPosition( segment->GetArcEnd() );
            break;

        case S_CIRCLE:
            m_editPoints->Point( CIRC_CENTER ).SetPosition( segment->GetCenter() );
            m_editPoints->Point( CIRC_END ).SetPosition( segment->GetEnd() );
            break;

        case S_POLYGON:
        {
            const auto& points = segment->BuildPolyPointsList();

            if( m_editPoints->PointsSize() != (unsigned) points.size() )
            {
                getView()->Remove( m_editPoints.get() );
                m_editedPoint = nullptr;
                m_editPoints = EDIT_POINTS_FACTORY::Make( item, getView()->GetGAL() );
                getView()->Add( m_editPoints.get() );
            }
            else
            {
                for( unsigned i = 0; i < points.size(); i++ )
                    m_editPoints->Point( i ).SetPosition( points[i] );
            }
            break;
        }

        case S_CURVE:
            m_editPoints->Point( BEZIER_CURVE_START ).SetPosition( segment->GetStart() );
            m_editPoints->Point( BEZIER_CURVE_CONTROL_POINT1 ).SetPosition( segment->GetBezControl1() );
            m_editPoints->Point( BEZIER_CURVE_CONTROL_POINT2 ).SetPosition( segment->GetBezControl2() );
            m_editPoints->Point( BEZIER_CURVE_END ).SetPosition( segment->GetEnd() );
            break;

        default:        // suppress warnings
            break;
        }

        break;
    }

    case PCB_ZONE_AREA_T:
    {
        ZONE_CONTAINER* zone = static_cast<ZONE_CONTAINER*>( item );
        const SHAPE_POLY_SET* outline = zone->Outline();

        if( m_editPoints->PointsSize() != (unsigned) outline->TotalVertices() )
        {
            getView()->Remove( m_editPoints.get() );
            m_editedPoint = nullptr;
            m_editPoints = EDIT_POINTS_FACTORY::Make( item, getView()->GetGAL() );
            getView()->Add( m_editPoints.get() );
        }
        else
        {
            for( int i = 0; i < outline->TotalVertices(); ++i )
                m_editPoints->Point( i ).SetPosition( outline->CVertex( i ) );
        }

        break;
    }

    case PCB_DIMENSION_T:
    {
        const DIMENSION* dimension = static_cast<const DIMENSION*>( item );

        m_editPoints->Point( DIM_CROSSBARO ).SetPosition( dimension->m_crossBarO );
        m_editPoints->Point( DIM_CROSSBARF ).SetPosition( dimension->m_crossBarF );
        m_editPoints->Point( DIM_FEATUREGO ).SetPosition( dimension->m_featureLineGO );
        m_editPoints->Point( DIM_FEATUREDO ).SetPosition( dimension->m_featureLineDO );
        break;
    }

    default:
        break;
    }

    getView()->Update( m_editPoints.get() );
}


void POINT_EDITOR::setEditedPoint( EDIT_POINT* aPoint )
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();

    if( aPoint )
    {
        frame()->GetCanvas()->SetCurrentCursor( wxCURSOR_ARROW );
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


void POINT_EDITOR::setAltConstraint( bool aEnabled )
{
    if( aEnabled )
    {
        EDIT_LINE* line = dynamic_cast<EDIT_LINE*>( m_editedPoint );

        if( line && m_editPoints->GetParent()->Type() == PCB_ZONE_AREA_T )
        {
            m_altConstraint.reset( (EDIT_CONSTRAINT<EDIT_POINT>*)( new EC_CONVERGING( *line, *m_editPoints ) ) );
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


EDIT_POINT POINT_EDITOR::get45DegConstrainer() const
{
    EDA_ITEM* item = m_editPoints->GetParent();

    switch( item->Type() )
    {
    case PCB_LINE_T:
    case PCB_MODULE_EDGE_T:
    {
        const DRAWSEGMENT* segment = static_cast<const DRAWSEGMENT*>( item );
        {
            switch( segment->GetShape() )
            {
            case S_SEGMENT:
                return *( m_editPoints->Next( *m_editedPoint ) );     // select the other end of line

            case S_ARC:
            case S_CIRCLE:
                return m_editPoints->Point( CIRC_CENTER );

            default:        // suppress warnings
                break;
            }
        }

        break;
    }

    case PCB_DIMENSION_T:
    {
        // Constraint for crossbar
        if( isModified( m_editPoints->Point( DIM_FEATUREGO ) ) )
            return m_editPoints->Point( DIM_FEATUREDO );

        else if( isModified( m_editPoints->Point( DIM_FEATUREDO ) ) )
            return m_editPoints->Point( DIM_FEATUREGO );

        else
            return EDIT_POINT( m_editedPoint->GetPosition() );      // no constraint

        break;
    }

    default:
        break;
    }

    // In any other case we may align item to its original position
    return m_original;
}


bool POINT_EDITOR::canAddCorner( const EDA_ITEM& aItem )
{
    const auto type = aItem.Type();

    // Works only for zones and line segments
    return type == PCB_ZONE_AREA_T ||
           ( ( type == PCB_LINE_T || type == PCB_MODULE_EDGE_T ) &&
             ( static_cast<const DRAWSEGMENT&>( aItem ).GetShape() == S_SEGMENT  ||
               static_cast<const DRAWSEGMENT&>( aItem ).GetShape() == S_POLYGON ) );
}


bool POINT_EDITOR::addCornerCondition( const SELECTION& aSelection )
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

        if( aPolySet.Vertex( vertexIdx ) == aPoint.GetPosition() )
            return std::make_pair( true, vertexIdx );
    }

    return std::make_pair( false, SHAPE_POLY_SET::VERTEX_INDEX() );
}


bool POINT_EDITOR::removeCornerCondition( const SELECTION& )
{
    if( !m_editPoints || !m_editedPoint )
        return false;

    EDA_ITEM* item = m_editPoints->GetParent();

    if( !item || !( item->Type() == PCB_ZONE_AREA_T ||
            ( ( item->Type() == PCB_MODULE_EDGE_T || item->Type() == PCB_LINE_T ) &&
                   static_cast<DRAWSEGMENT*>( item )->GetShape() == S_POLYGON ) ) )
        return false;

    SHAPE_POLY_SET *polyset;

    if( item->Type() == PCB_ZONE_AREA_T )
        polyset = static_cast<ZONE_CONTAINER*>( item )->Outline();
    else
        polyset = &static_cast<DRAWSEGMENT*>( item )->GetPolyShape();

    auto vertex = findVertex( *polyset, *m_editedPoint );

    if( !vertex.first )
        return false;

    const auto& vertexIdx = vertex.second;

    // Check if there are enough vertices so one can be removed without
    // degenerating the polygon.
    // The first condition allows one to remove all corners from holes (when
    // there are only 2 vertices left, a hole is removed).
    if( vertexIdx.m_contour == 0 && polyset->Polygon( vertexIdx.m_polygon )[vertexIdx.m_contour].PointCount() <= 3 )
        return false;

    // Remove corner does not work with lines
    if( dynamic_cast<EDIT_LINE*>( m_editedPoint ) )
        return false;

    return m_editedPoint != NULL;
}


int POINT_EDITOR::addCorner( const TOOL_EVENT& aEvent )
{
    if( !m_editPoints )
        return 0;

    EDA_ITEM* item = m_editPoints->GetParent();
    PCB_BASE_EDIT_FRAME* frame = getEditFrame<PCB_BASE_EDIT_FRAME>();
    const VECTOR2I& cursorPos = getViewControls()->GetCursorPosition();

    // called without an active edited polygon
    if( !item || !canAddCorner( *item ) )
        return 0;

    DRAWSEGMENT* graphicItem = dynamic_cast<DRAWSEGMENT*>( item );
    BOARD_COMMIT commit( frame );

    if( item->Type() == PCB_ZONE_AREA_T ||
            ( graphicItem && graphicItem->GetShape() == S_POLYGON ) )
    {
        unsigned int nearestIdx = 0;
        unsigned int nextNearestIdx = 0;
        unsigned int nearestDist = INT_MAX;
        unsigned int firstPointInContour = 0;
        SHAPE_POLY_SET* zoneOutline;

        if( item->Type() == PCB_ZONE_AREA_T )
        {
            ZONE_CONTAINER* zone = static_cast<ZONE_CONTAINER*>( item );
            zoneOutline = zone->Outline();
            zone->SetNeedRefill( true );
        }
        else
            zoneOutline = &( graphicItem->GetPolyShape() );

        commit.Modify( item );

        // Search the best outline segment to add a new corner
        // and therefore break this segment into two segments

        // Object to iterate through the corners of the outlines (main contour and its holes)
        SHAPE_POLY_SET::ITERATOR iterator = zoneOutline->Iterate( 0,
                zoneOutline->OutlineCount()-1, /* IterateHoles */ true );
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

            SEG curr_segment( zoneOutline->Vertex( curr_idx ), zoneOutline->Vertex( jj ) );

            unsigned int distance = curr_segment.Distance( cursorPos );

            if( distance < nearestDist )
            {
                nearestDist = distance;
                nearestIdx = curr_idx;
                nextNearestIdx = jj;
            }
        }

        // Find the point on the closest segment
        VECTOR2I sideOrigin = zoneOutline->Vertex( nearestIdx );
        VECTOR2I sideEnd = zoneOutline->Vertex( nextNearestIdx );
        SEG nearestSide( sideOrigin, sideEnd );
        VECTOR2I nearestPoint = nearestSide.NearestPoint( cursorPos );

        // Do not add points that have the same coordinates as ones that already belong to polygon
        // instead, add a point in the middle of the side
        if( nearestPoint == sideOrigin || nearestPoint == sideEnd )
            nearestPoint = ( sideOrigin + sideEnd ) / 2;

        zoneOutline->InsertVertex( nextNearestIdx, nearestPoint );

        // We re-hatch the filled zones but not polygons
        if( item->Type() == PCB_ZONE_AREA_T )
            static_cast<ZONE_CONTAINER*>( item )->Hatch();


        commit.Push( _( "Add a zone corner" ) );
    }

    else if( graphicItem && graphicItem->GetShape() == S_SEGMENT )
    {
        commit.Modify( graphicItem );

        SEG seg( graphicItem->GetStart(), graphicItem->GetEnd() );
        VECTOR2I nearestPoint = seg.NearestPoint( cursorPos );

        // Move the end of the line to the break point..
        graphicItem->SetEnd( wxPoint( nearestPoint.x, nearestPoint.y ) );

        // and add another one starting from the break point
        DRAWSEGMENT* newSegment;

        if( item->Type() == PCB_MODULE_EDGE_T )
        {
            EDGE_MODULE* edge = static_cast<EDGE_MODULE*>( graphicItem );
            assert( edge->GetParent()->Type() == PCB_MODULE_T );
            newSegment = new EDGE_MODULE( *edge );
        }
        else
        {
            newSegment = new DRAWSEGMENT( *graphicItem );
        }

        newSegment->ClearSelected();
        newSegment->SetStart( wxPoint( nearestPoint.x, nearestPoint.y ) );
        newSegment->SetEnd( wxPoint( seg.B.x, seg.B.y ) );

        commit.Add( newSegment );
        commit.Push( _( "Split segment" ) );
    }

    updatePoints();
    return 0;
}


int POINT_EDITOR::removeCorner( const TOOL_EVENT& aEvent )
{
    if( !m_editPoints || !m_editedPoint )
        return 0;

    EDA_ITEM* item = m_editPoints->GetParent();

    if( !item )
        return 0;

    SHAPE_POLY_SET* polygon = nullptr;

    if( item->Type() == PCB_ZONE_AREA_T)
    {
        auto zone = static_cast<ZONE_CONTAINER*>( item );
        polygon = zone->Outline();
        zone->SetNeedRefill( true );
    }
    else if( (item->Type() == PCB_MODULE_EDGE_T ) || ( item->Type() == PCB_LINE_T ) )
    {
        auto ds = static_cast<DRAWSEGMENT*>( item );

        if( ds->GetShape() == S_POLYGON )
            polygon = &ds->GetPolyShape();
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
        if( item->Type() == PCB_ZONE_AREA_T)
            static_cast<ZONE_CONTAINER*>( item )->Hatch();

        updatePoints();
    }

    return 0;
}


int POINT_EDITOR::modifiedSelection( const TOOL_EVENT& aEvent )
{
    updatePoints();
    return 0;
}


void POINT_EDITOR::setTransitions()
{
    Go( &POINT_EDITOR::OnSelectionChange, ACTIONS::activatePointEditor.MakeEvent() );
    Go( &POINT_EDITOR::addCorner,         PCB_ACTIONS::pointEditorAddCorner.MakeEvent() );
    Go( &POINT_EDITOR::removeCorner,      PCB_ACTIONS::pointEditorRemoveCorner.MakeEvent() );
    Go( &POINT_EDITOR::modifiedSelection, EVENTS::SelectedItemsModified );
    Go( &POINT_EDITOR::OnSelectionChange, EVENTS::SelectedEvent );
    Go( &POINT_EDITOR::OnSelectionChange, EVENTS::UnselectedEvent );
}
