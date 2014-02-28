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
#include <confirm.h>

#include "common_actions.h"
#include "selection_tool.h"
#include "point_editor.h"

#include <class_drawsegment.h>

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
        // TODO generate list of points basing on the type
        boost::shared_ptr<EDIT_POINTS> points = boost::make_shared<EDIT_POINTS>( aItem );

        switch( aItem->Type() )
        {
            case PCB_LINE_T:
            {
                DRAWSEGMENT* segment = static_cast<DRAWSEGMENT*>( aItem );

                switch( segment->GetShape() )
                {
                case S_SEGMENT:
                    points->Add( segment->GetStart() );
                    points->Add( segment->GetEnd() );

                    break;

                case S_ARC:
                    points->Add( segment->GetCenter() );         // points[0]
                    points->Add( segment->GetArcStart() );       // points[1]
                    points->Add( segment->GetArcEnd() );         // points[2]

                    // Set constraints
                    // Arc end has to stay at the same radius as the start
                    (*points)[2].SetConstraint( new EPC_CIRCLE( (*points)[2], (*points)[0], (*points)[1] ) );
                    break;

                default:        // suppress warnings
                    break;
                }
            }
            break;

            default:
                break;
        }

        return points;
    }

private:
    EDIT_POINTS_FACTORY() {};
};


POINT_EDITOR::POINT_EDITOR() :
    TOOL_INTERACTIVE( "pcbnew.PointEditor" ), m_selectionTool( NULL )
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
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    m_dragPoint = NULL;

    if( selection.Size() == 1 )
    {
        Activate();

        EDA_ITEM* item = selection.items.GetPickedItem( 0 );
        m_editPoints = EDIT_POINTS_FACTORY::Make( item );
        m_toolMgr->GetView()->Add( m_editPoints.get() );

        // Main loop: keep receiving events
        while( OPT_TOOL_EVENT evt = Wait() )
        {
            if( !m_editPoints || evt->IsCancel() ||
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
                    }
                    else
                    {
                        controls->ShowCursor( false );
                        controls->SetAutoPan( false );
                        controls->SetSnapping( false );
                    }
                }

                m_dragPoint = point;
            }

            else if( evt->IsDrag( BUT_LEFT ) && m_dragPoint )
            {
                m_dragPoint->SetPosition( controls->GetCursorPosition() );
                m_dragPoint->ApplyConstraint();
                updateItem();
                updatePoints();

                m_editPoints->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            }

            else if( evt->IsAction( &COMMON_ACTIONS::pointEditorUpdate ) )
            {
                updatePoints();
            }

            else
            {
                m_toolMgr->PassEvent();
            }
        }

        if( m_editPoints )
        {
            item->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            m_toolMgr->GetView()->Remove( m_editPoints.get() );
            m_editPoints.reset();
        }
    }

    controls->ShowCursor( false );
    controls->SetAutoPan( false );
    controls->SetSnapping( false );

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
            if( &(*m_editPoints)[0] == m_dragPoint )
                segment->SetStart( wxPoint( (*m_editPoints)[0].GetPosition().x, (*m_editPoints)[0].GetPosition().y ) );
            else if( &(*m_editPoints)[1] == m_dragPoint )
                segment->SetEnd( wxPoint( (*m_editPoints)[1].GetPosition().x, (*m_editPoints)[1].GetPosition().y ) );

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
        }
        break;

        default:        // suppress warnings
            break;
        }
        break;
    }

    default:
        break;
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

            default:        // suppress warnings
                break;
            }
        }
        break;
    }

    default:
        break;
    }
}
