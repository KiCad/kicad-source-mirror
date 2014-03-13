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

#include <tool/tool_manager.h>
#include <view/view_controls.h>
#include <gal/graphics_abstraction_layer.h>
#include <confirm.h>

#include "common_actions.h"
#include "selection_tool.h"
#include "point_editor.h"

#include <wxPcbStruct.h>
#include <class_drawsegment.h>
#include <class_dimension.h>
#include <class_zone.h>

/**
 * Class POINT_EDITOR
 *
 * Tool that displays edit points allowing to modify items by dragging the points.
 */

class EDIT_POINTS_FACTORY
{
public:
    static boost::shared_ptr<EDIT_POINTS> Make( EDA_ITEM* aItem )
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
                    points->AddPoint( segment->GetStart() );          // points[0]
                    points->AddPoint( segment->GetEnd() );            // points[1]
                    break;

                case S_ARC:
                    points->AddPoint( segment->GetCenter() );         // points[0]
                    points->AddPoint( segment->GetArcStart() );       // points[1]
                    points->AddPoint( segment->GetArcEnd() );         // points[2]

                    // Set constraints
                    // Arc end has to stay at the same radius as the start
                    (*points)[2].SetConstraint( new EPC_CIRCLE( (*points)[2], (*points)[0], (*points)[1] ) );
                    break;

                case S_CIRCLE:
                    points->AddPoint( segment->GetCenter() );         // points[0]
                    points->AddPoint( segment->GetEnd() );            // points[1]
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

                // Lines have to be added after creating edit points, so they use EDIT_POINT references
                for( int i = 0; i < cornersCount - 1; ++i )
                    points->AddLine( (*points)[i], (*points)[i + 1] );

                // The last missing line, connecting the last and the first polygon point
                points->AddLine( (*points)[cornersCount - 1], (*points)[0] );
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
                (*points)[0].SetConstraint( new EPC_LINE( (*points)[0], (*points)[2] ) );
                (*points)[1].SetConstraint( new EPC_LINE( (*points)[1], (*points)[3] ) );

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
    m_original( VECTOR2I( 0, 0 ) )
{
}


void POINT_EDITOR::Reset( RESET_REASON aReason )
{
    m_editPoints.reset();
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
        EDIT_POINT constrainer( VECTOR2I( 0, 0 ) );
        boost::shared_ptr<EDIT_POINT_CONSTRAINT> degree45Constraint;

        m_editPoints = EDIT_POINTS_FACTORY::Make( item );
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
                        controls->SetAutoPan( true );
                        controls->SetSnapping( true );
                        controls->ForceCursorPosition( true, point->GetPosition() );
                    }
                    else
                    {
                        controls->ShowCursor( false );
                        controls->SetAutoPan( false );
                        controls->SetSnapping( false );
                        controls->ForceCursorPosition( false );
                    }
                }

                m_dragPoint = point;
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
                    modified = true;
                }

                if( !!evt->Modifier( MD_CTRL ) != (bool) degree45Constraint )      // 45 degrees mode
                {
                    if( !degree45Constraint )
                    {
                        // Find a proper constraining point for 45 degrees mode
                        constrainer = get45DegConstrainer();
                        degree45Constraint.reset( new EPC_45DEGREE( *m_dragPoint, constrainer ) );
                    }
                    else
                    {
                        degree45Constraint.reset();
                    }
                }

                m_dragPoint->SetPosition( controls->GetCursorPosition() );

                if( degree45Constraint )
                    degree45Constraint->Apply();
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
            if( isModified( (*m_editPoints)[0] ) )
                segment->SetStart( wxPoint( (*m_editPoints)[0].GetPosition().x,
                                            (*m_editPoints)[0].GetPosition().y ) );

            else if( isModified( (*m_editPoints)[1] ) )
                segment->SetEnd( wxPoint( (*m_editPoints)[1].GetPosition().x,
                                          (*m_editPoints)[1].GetPosition().y ) );

            break;

        case S_ARC:
        {
            const VECTOR2I& center = (*m_editPoints)[0].GetPosition();
            const VECTOR2I& start = (*m_editPoints)[1].GetPosition();
            const VECTOR2I& end = (*m_editPoints)[2].GetPosition();

            if( center != segment->GetCenter() )
            {
                wxPoint moveVector = wxPoint( center.x, center.y ) - segment->GetCenter();
                segment->Move( moveVector );

                (*m_editPoints)[1].SetPosition( segment->GetArcStart() );
                (*m_editPoints)[2].SetPosition( segment->GetArcEnd() );
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
            const VECTOR2I& center = (*m_editPoints)[0].GetPosition();
            const VECTOR2I& end = (*m_editPoints)[1].GetPosition();

            if( isModified( (*m_editPoints)[0] ) )
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
        CPolyLine* outline = zone->Outline();

        for( int i = 0; i < outline->GetCornersCount(); ++i )
        {
            outline->SetX( i, (*m_editPoints)[i].GetPosition().x );
            outline->SetY( i, (*m_editPoints)[i].GetPosition().y );
        }

        break;
    }

    case PCB_DIMENSION_T:
    {
        DIMENSION* dimension = static_cast<DIMENSION*>( item );

        // Check which point is currently modified and updated dimension's points respectively
        if( isModified( (*m_editPoints)[0] ) )
        {
            VECTOR2D featureLine( m_dragPoint->GetPosition() - dimension->GetOrigin() );
            VECTOR2D crossBar( dimension->GetEnd() - dimension->GetOrigin() );

            if( featureLine.Cross( crossBar ) > 0 )
                dimension->SetHeight( -featureLine.EuclideanNorm() );
            else
                dimension->SetHeight( featureLine.EuclideanNorm() );
        }

        else if( isModified( (*m_editPoints)[1] ) )
        {
            VECTOR2D featureLine( m_dragPoint->GetPosition() - dimension->GetEnd() );
            VECTOR2D crossBar( dimension->GetEnd() - dimension->GetOrigin() );

            if( featureLine.Cross( crossBar ) > 0 )
                dimension->SetHeight( -featureLine.EuclideanNorm() );
            else
                dimension->SetHeight( featureLine.EuclideanNorm() );
        }

        else if( isModified( (*m_editPoints)[2] ) )
        {
            dimension->SetOrigin( wxPoint( m_dragPoint->GetPosition().x, m_dragPoint->GetPosition().y ) );
            (*m_editPoints)[0].GetConstraint()->Update();
            (*m_editPoints)[1].GetConstraint()->Update();
        }

        else if( isModified( (*m_editPoints)[3] ) )
        {
            dimension->SetEnd( wxPoint( m_dragPoint->GetPosition().x, m_dragPoint->GetPosition().y ) );
            (*m_editPoints)[0].GetConstraint()->Update();
            (*m_editPoints)[1].GetConstraint()->Update();
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
                (*m_editPoints)[0].SetPosition( segment->GetStart() );
                (*m_editPoints)[1].SetPosition( segment->GetEnd() );
                break;

            case S_ARC:
                (*m_editPoints)[0].SetPosition( segment->GetCenter() );
                (*m_editPoints)[1].SetPosition( segment->GetArcStart() );
                (*m_editPoints)[2].SetPosition( segment->GetArcEnd() );
                break;

            case S_CIRCLE:
                (*m_editPoints)[0].SetPosition( segment->GetCenter() );
                (*m_editPoints)[1].SetPosition( segment->GetEnd() );
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
            (*m_editPoints)[i].SetPosition( outline->GetPos( i ) );

        break;
    }

    case PCB_DIMENSION_T:
    {
        const DIMENSION* dimension = static_cast<const DIMENSION*>( item );

        (*m_editPoints)[0].SetPosition( dimension->m_crossBarO );
        (*m_editPoints)[1].SetPosition( dimension->m_crossBarF );
        (*m_editPoints)[2].SetPosition( dimension->m_featureLineGO );
        (*m_editPoints)[3].SetPosition( dimension->m_featureLineDO );
        break;
    }

    default:
        break;
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
                return (*m_editPoints)[0];      // center

            default:        // suppress warnings
                break;
            }
        }

        break;
    }

    case PCB_DIMENSION_T:
    {
        // Constraint for crossbar
        if( isModified( (*m_editPoints)[2] ) )
            return (*m_editPoints)[3];

        else if( isModified( (*m_editPoints)[3] ) )
            return (*m_editPoints)[2];

        else
            return EDIT_POINT( m_dragPoint->GetPosition() );      // no constraint

        break;
    }

    default:
        break;
    }

    // In any other case we may align item to the current cursor position. TODO wrong desc
    return m_original;
}
