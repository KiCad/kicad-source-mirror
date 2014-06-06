/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
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

#include <boost/make_shared.hpp>
#include <boost/bind.hpp>

#include <tool/tool_manager.h>
#include <view/view_controls.h>
#include <gal/graphics_abstraction_layer.h>
#include <geometry/seg.h>
#include <confirm.h>

#include "common_actions.h"
#include "selection_tool.h"
#include "point_editor.h"

#include <wxPcbStruct.h>
#include <class_drawsegment.h>
#include <class_dimension.h>
#include <class_zone.h>
#include <class_board.h>

// Few constants to avoid using bare numbers for point indices
enum SEG_POINTS
{
    SEG_START, SEG_END
};

enum ARC_POINTS
{
    ARC_CENTER, ARC_START, ARC_END
};

enum CIRCLE_POINTS
{
    CIRC_CENTER, CIRC_END
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
public:
    static boost::shared_ptr<EDIT_POINTS> Make( EDA_ITEM* aItem, KIGFX::GAL* aGal )
    {
        boost::shared_ptr<EDIT_POINTS> points = boost::make_shared<EDIT_POINTS>( aItem );

        // Generate list of edit points basing on the item type
        switch( aItem->Type() )
        {
            case PCB_LINE_T:
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
                    points->AddPoint( segment->GetArcEnd() );

                    // Set constraints
                    // Arc end has to stay at the same radius as the start
                    points->Point( ARC_END ).SetConstraint( new EC_CIRCLE( points->Point( ARC_END ),
                                                                           points->Point( ARC_CENTER ),
                                                                           points->Point( ARC_START ) ) );
                    break;

                case S_CIRCLE:
                    points->AddPoint( segment->GetCenter() );
                    points->AddPoint( segment->GetEnd() );
                    break;

                default:        // suppress warnings
                    break;
                }

                break;
            }

            case PCB_ZONE_AREA_T:
            {
                const CPolyLine* outline = static_cast<const ZONE_CONTAINER*>( aItem )->Outline();
                int cornersCount = outline->GetCornersCount();

                for( int i = 0; i < cornersCount; ++i )
                    points->AddPoint( outline->GetPos( i ) );

                // Lines have to be added after creating edit points,
                // as they use EDIT_POINT references
                for( int i = 0; i < cornersCount - 1; ++i )
                {
                    points->AddLine( points->Point( i ), points->Point( i + 1 ) );
                    points->Line( i ).SetConstraint(
                            new EC_SNAPLINE( points->Line( i ),
                            boost::bind( &KIGFX::GAL::GetGridPoint, aGal, _1 ) ) );
                }

                // The last missing line, connecting the last and the first polygon point
                points->AddLine( points->Point( cornersCount - 1 ), points->Point( 0 ) );
                points->Line( points->LinesSize() - 1 ).SetConstraint(
                        new EC_SNAPLINE( points->Line( points->LinesSize() - 1 ),
                        boost::bind( &KIGFX::GAL::GetGridPoint, aGal, _1 ) ) );
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
    TOOL_INTERACTIVE( "pcbnew.PointEditor" ), m_selectionTool( NULL ), m_dragPoint( NULL ),
    m_original( VECTOR2I( 0, 0 ) ), m_altConstrainer( VECTOR2I( 0, 0 ) )
{
}


void POINT_EDITOR::Reset( RESET_REASON aReason )
{
    m_editPoints.reset();
    m_altConstraint.reset();
}


bool POINT_EDITOR::Init()
{
    // Find the selection tool, so they can cooperate
    m_selectionTool = static_cast<SELECTION_TOOL*>( m_toolMgr->FindTool( "pcbnew.InteractiveSelection" ) );

    if( !m_selectionTool )
    {
        DisplayError( NULL, wxT( "pcbnew.InteractiveSelection tool is not available" ) );
        return false;
    }

    setTransitions();

    return true;
}


int POINT_EDITOR::OnSelectionChange( TOOL_EVENT& aEvent )
{
    const SELECTION_TOOL::SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.Size() == 1 )
    {
        Activate();

        KIGFX::VIEW_CONTROLS* controls = getViewControls();
        KIGFX::VIEW* view = getView();
        PCB_EDIT_FRAME* editFrame = getEditFrame<PCB_EDIT_FRAME>();
        EDA_ITEM* item = selection.items.GetPickedItem( 0 );

        m_editPoints = EDIT_POINTS_FACTORY::Make( item, getView()->GetGAL() );
        if( !m_editPoints )
        {
            setTransitions();
            return 0;
        }

        view->Add( m_editPoints.get() );
        m_dragPoint = NULL;
        bool modified = false;

        // Main loop: keep receiving events
        while( OPT_TOOL_EVENT evt = Wait() )
        {
            if( !m_editPoints ||
                evt->Matches( m_selectionTool->ClearedEvent ) ||
                evt->Matches( m_selectionTool->DeselectedEvent ) ||
                evt->Matches( m_selectionTool->SelectedEvent ) )
            {
                break;
            }

            if( evt->IsMotion() )
            {
                EDIT_POINT* point = m_editPoints->FindPoint( evt->Position() );

                if( m_dragPoint != point )
                {
                    if( point )
                    {
                        controls->ShowCursor( true );
                        controls->SetSnapping( true );
                        controls->ForceCursorPosition( true, point->GetPosition() );
                    }
                    else
                    {
                        controls->ShowCursor( false );
                        controls->SetSnapping( false );
                        controls->ForceCursorPosition( false );
                    }
                }

                m_dragPoint = point;
            }

            else if( evt->IsDblClick( BUT_LEFT ) )
            {
                breakOutline( controls->GetCursorPosition() );
            }

            else if( evt->IsDrag( BUT_LEFT ) && m_dragPoint )
            {
                if( !modified )
                {
                    // Save items, so changes can be undone
                    editFrame->OnModify();
                    editFrame->SaveCopyInUndoList( selection.items, UR_CHANGED );
                    controls->ForceCursorPosition( false );
                    m_original = *m_dragPoint;    // Save the original position
                    controls->SetAutoPan( true );
                    modified = true;
                }

                bool enableAltConstraint = !!evt->Modifier( MD_CTRL );
                if( enableAltConstraint != (bool) m_altConstraint )  // alternative constraint
                    setAltConstraint( enableAltConstraint );

                m_dragPoint->SetPosition( controls->GetCursorPosition() );

                if( m_altConstraint )
                    m_altConstraint->Apply();
                else
                    m_dragPoint->ApplyConstraint();

                updateItem();
                updatePoints();

                m_editPoints->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            }

            else if( evt->IsAction( &COMMON_ACTIONS::pointEditorUpdate ) )
            {
                updatePoints();
            }

            else if( evt->IsMouseUp( BUT_LEFT ) )
            {
                controls->SetAutoPan( false );
                setAltConstraint( false );
                modified = false;
            }

            else if( evt->IsCancel() )
            {
                if( modified )      // Restore the last change
                {
                    wxCommandEvent dummy;
                    editFrame->GetBoardFromUndoList( dummy );

                    updatePoints();
                    modified = false;
                }

                // Let the selection tool receive the event too
                m_toolMgr->PassEvent();

                break;
            }

            else
            {
                m_toolMgr->PassEvent();
            }
        }

        if( m_editPoints )
        {
            finishItem();
            item->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            view->Remove( m_editPoints.get() );
            m_editPoints.reset();
        }

        controls->ShowCursor( false );
        controls->SetAutoPan( false );
        controls->SetSnapping( false );
        controls->ForceCursorPosition( false );
    }

    setTransitions();

    return 0;
}


void POINT_EDITOR::updateItem() const
{
    EDA_ITEM* item = m_editPoints->GetParent();

    switch( item->Type() )
    {
    case PCB_LINE_T:
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
            const VECTOR2I& center = m_editPoints->Point( ARC_CENTER ).GetPosition();
            const VECTOR2I& start = m_editPoints->Point( ARC_START ).GetPosition();
            const VECTOR2I& end = m_editPoints->Point( ARC_END ).GetPosition();

            if( center != segment->GetCenter() )
            {
                wxPoint moveVector = wxPoint( center.x, center.y ) - segment->GetCenter();
                segment->Move( moveVector );

                m_editPoints->Point( ARC_START ).SetPosition( segment->GetArcStart() );
                m_editPoints->Point( ARC_END ).SetPosition( segment->GetArcEnd() );
            }

            else
            {
                segment->SetArcStart( wxPoint( start.x, start.y ) );

                VECTOR2D startLine = start - center;
                VECTOR2I endLine = end - center;
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

        default:        // suppress warnings
            break;
        }

        break;
    }

    case PCB_ZONE_AREA_T:
    {
        ZONE_CONTAINER* zone = static_cast<ZONE_CONTAINER*>( item );
        zone->ClearFilledPolysList();
        CPolyLine* outline = zone->Outline();

        for( int i = 0; i < outline->GetCornersCount(); ++i )
        {
            outline->SetX( i, m_editPoints->Point( i ).GetPosition().x );
            outline->SetY( i, m_editPoints->Point( i ).GetPosition().y );
        }

        break;
    }

    case PCB_DIMENSION_T:
    {
        DIMENSION* dimension = static_cast<DIMENSION*>( item );

        // Check which point is currently modified and updated dimension's points respectively
        if( isModified( m_editPoints->Point( DIM_CROSSBARO ) ) )
        {
            VECTOR2D featureLine( m_dragPoint->GetPosition() - dimension->GetOrigin() );
            VECTOR2D crossBar( dimension->GetEnd() - dimension->GetOrigin() );

            if( featureLine.Cross( crossBar ) > 0 )
                dimension->SetHeight( -featureLine.EuclideanNorm() );
            else
                dimension->SetHeight( featureLine.EuclideanNorm() );
        }

        else if( isModified( m_editPoints->Point( DIM_CROSSBARF ) ) )
        {
            VECTOR2D featureLine( m_dragPoint->GetPosition() - dimension->GetEnd() );
            VECTOR2D crossBar( dimension->GetEnd() - dimension->GetOrigin() );

            if( featureLine.Cross( crossBar ) > 0 )
                dimension->SetHeight( -featureLine.EuclideanNorm() );
            else
                dimension->SetHeight( featureLine.EuclideanNorm() );
        }

        else if( isModified( m_editPoints->Point( DIM_FEATUREGO ) ) )
        {
            dimension->SetOrigin( wxPoint( m_dragPoint->GetPosition().x, m_dragPoint->GetPosition().y ) );
            m_editPoints->Point( DIM_CROSSBARO ).SetConstraint( new EC_LINE( m_editPoints->Point( DIM_CROSSBARO ),
                                                                             m_editPoints->Point( DIM_FEATUREGO ) ) );
            m_editPoints->Point( DIM_CROSSBARF ).SetConstraint( new EC_LINE( m_editPoints->Point( DIM_CROSSBARF ),
                                                                             m_editPoints->Point( DIM_FEATUREDO ) ) );
        }

        else if( isModified( m_editPoints->Point( DIM_FEATUREDO ) ) )
        {
            dimension->SetEnd( wxPoint( m_dragPoint->GetPosition().x, m_dragPoint->GetPosition().y ) );
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
}


void POINT_EDITOR::finishItem() const
{
    EDA_ITEM* item = m_editPoints->GetParent();

    if( item->Type() == PCB_ZONE_AREA_T )
    {
        ZONE_CONTAINER* zone = static_cast<ZONE_CONTAINER*>( item );

        if( zone->IsFilled() )
            getEditFrame<PCB_EDIT_FRAME>()->Fill_Zone( zone );
    }
}


void POINT_EDITOR::updatePoints() const
{
    EDA_ITEM* item = m_editPoints->GetParent();

    switch( item->Type() )
    {
    case PCB_LINE_T:
    {
        const DRAWSEGMENT* segment = static_cast<const DRAWSEGMENT*>( item );
        {
            switch( segment->GetShape() )
            {
            case S_SEGMENT:
                m_editPoints->Point( SEG_START ).SetPosition( segment->GetStart() );
                m_editPoints->Point( SEG_END ).SetPosition( segment->GetEnd() );
                break;

            case S_ARC:
                m_editPoints->Point( ARC_CENTER ).SetPosition( segment->GetCenter() );
                m_editPoints->Point( ARC_START).SetPosition( segment->GetArcStart() );
                m_editPoints->Point( ARC_END ).SetPosition( segment->GetArcEnd() );
                break;

            case S_CIRCLE:
                m_editPoints->Point( CIRC_CENTER ).SetPosition( segment->GetCenter() );
                m_editPoints->Point( CIRC_END ).SetPosition( segment->GetEnd() );
                break;

            default:        // suppress warnings
                break;
            }

            break;
        }
    }

    case PCB_ZONE_AREA_T:
    {
        const ZONE_CONTAINER* zone = static_cast<const ZONE_CONTAINER*>( item );
        const CPolyLine* outline = zone->Outline();

        for( int i = 0; i < outline->GetCornersCount(); ++i )
            m_editPoints->Point( i ).SetPosition( outline->GetPos( i ) );

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
}


void POINT_EDITOR::setAltConstraint( bool aEnabled )
{
    if( aEnabled )
    {
        EDIT_LINE* line = dynamic_cast<EDIT_LINE*>( m_dragPoint );

        if( line )
        {
            if( m_editPoints->GetParent()->Type() == PCB_ZONE_AREA_T )
                m_altConstraint.reset( (EDIT_CONSTRAINT<EDIT_POINT>*)( new EC_CONVERGING( *line, *m_editPoints ) ) );
        }
        else
        {
            // Find a proper constraining point for 45 degrees mode
            m_altConstrainer = get45DegConstrainer();
            m_altConstraint.reset( new EC_45DEGREE( *m_dragPoint, m_altConstrainer ) );
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
    {
        const DRAWSEGMENT* segment = static_cast<const DRAWSEGMENT*>( item );
        {
            switch( segment->GetShape() )
            {
            case S_SEGMENT:
                return *( m_editPoints->Next( *m_dragPoint ) );     // select the other end of line

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
            return EDIT_POINT( m_dragPoint->GetPosition() );      // no constraint

        break;
    }

    default:
        break;
    }

    // In any other case we may align item to its original position
    return m_original;
}


void POINT_EDITOR::breakOutline( const VECTOR2I& aBreakPoint )
{
    EDA_ITEM* item = m_editPoints->GetParent();
    const SELECTION_TOOL::SELECTION& selection = m_selectionTool->GetSelection();

    if( item->Type() == PCB_ZONE_AREA_T )
    {
        getEditFrame<PCB_EDIT_FRAME>()->OnModify();
        getEditFrame<PCB_EDIT_FRAME>()->SaveCopyInUndoList( selection.items, UR_CHANGED );

        ZONE_CONTAINER* zone = static_cast<ZONE_CONTAINER*>( item );
        CPolyLine* outline = zone->Outline();

        // Handle the last segment, so other segments can be easily handled in a loop
        unsigned int nearestIdx = outline->GetCornersCount() - 1, nextNearestIdx = 0;
        SEG side( VECTOR2I( outline->GetPos( nearestIdx ) ),
                  VECTOR2I( outline->GetPos( nextNearestIdx ) ) );
        unsigned int nearestDist = side.Distance( aBreakPoint );

        for( int i = 0; i < outline->GetCornersCount() - 1; ++i )
        {
            side = SEG( VECTOR2I( outline->GetPos( i ) ), VECTOR2I( outline->GetPos( i + 1 ) ) );

            unsigned int distance = side.Distance( aBreakPoint );
            if( distance < nearestDist )
            {
                nearestDist = distance;
                nearestIdx = i;
                nextNearestIdx = i + 1;
            }
        }

        // Find the point on the closest segment
        VECTOR2I sideOrigin( outline->GetPos( nearestIdx ) );
        VECTOR2I sideEnd( outline->GetPos( nextNearestIdx ) );
        SEG nearestSide( sideOrigin, sideEnd );
        VECTOR2I nearestPoint = nearestSide.NearestPoint( aBreakPoint );

        // Do not add points that have the same coordinates as ones that already belong to polygon
        // instead, add a point in the middle of the side
        if( nearestPoint == sideOrigin || nearestPoint == sideEnd )
            nearestPoint = ( sideOrigin + sideEnd ) / 2;

        outline->InsertCorner( nearestIdx, nearestPoint.x, nearestPoint.y );
    }

    else if( item->Type() == PCB_LINE_T )
    {
        getEditFrame<PCB_EDIT_FRAME>()->OnModify();
        getEditFrame<PCB_EDIT_FRAME>()->SaveCopyInUndoList( selection.items, UR_CHANGED );

        DRAWSEGMENT* segment = static_cast<DRAWSEGMENT*>( item );

        if( segment->GetShape() == S_SEGMENT )
        {
            SEG seg( segment->GetStart(), segment->GetEnd() );
            VECTOR2I nearestPoint = seg.NearestPoint( aBreakPoint );

            // Move the end of the line to the break point..
            segment->SetEnd( wxPoint( nearestPoint.x, nearestPoint.y ) );

            // and add another one starting from the break point
            DRAWSEGMENT* newSegment = new DRAWSEGMENT( *segment );
            newSegment->ClearSelected();
            newSegment->SetStart( wxPoint( nearestPoint.x, nearestPoint.y ) );
            newSegment->SetEnd( wxPoint( seg.B.x, seg.B.y ) );

            getModel<BOARD>()->Add( newSegment );
            getView()->Add( newSegment );
        }
    }
}
