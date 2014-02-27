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
    static EDIT_POINTS Make( EDA_ITEM* aItem )
    {
        // TODO generate list of points basing on the type
        EDIT_POINTS points( aItem );

        switch( aItem->Type() )
        {
            case PCB_LINE_T:
            {
                DRAWSEGMENT* segment = static_cast<DRAWSEGMENT*>( aItem );

                switch( segment->GetShape() )
                {
                case S_SEGMENT:
                    points.Add( segment->GetStart() );
                    points.Add( segment->GetEnd() );

                    break;

                case S_ARC:
                    points.Add( segment->GetCenter() );         // points[0]
                    points.Add( segment->GetArcStart() );       // points[1]
                    points.Add( segment->GetArcEnd() );         // points[2]

                    // Set constraints
                    // Arc end has to stay at the same radius as the start
                    points[2].SetConstraint( new EPC_CIRCLE( &points[2], points[0], points[1] ) );
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


int POINT_EDITOR::OnSelected( TOOL_EVENT& aEvent )
{
    std::cout << "point editor activated" << std::endl;
    std::cout << aEvent.Format() << std::endl;

    const SELECTION_TOOL::SELECTION& selection = m_selectionTool->GetSelection();
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    m_dragPoint = NULL;

    if( selection.Size() == 1 )
    {
        Activate();

        EDA_ITEM* item = selection.items.GetPickedItem( 0 );
        EDIT_POINTS editPoints = EDIT_POINTS_FACTORY::Make( item );
        m_toolMgr->GetView()->Add( &editPoints );

        // Main loop: keep receiving events
        while( OPT_TOOL_EVENT evt = Wait() )
        {
            if( evt->IsCancel() ||
                evt->Matches( m_selectionTool->ClearedEvent ) ||
                evt->Matches( m_selectionTool->DeselectedEvent ) ||
                evt->Matches( m_selectionTool->SelectedEvent ) )
            {
                m_toolMgr->PassEvent();
                break;
            }

            else if( evt->IsMotion() )
            {
                EDIT_POINT* point = editPoints.FindPoint( evt->Position() );

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

            else if( evt->IsDrag( BUT_LEFT ) )
            {
                if( m_dragPoint )
                {
                    m_dragPoint->SetPosition( controls->GetCursorPosition() );
                    m_dragPoint->ApplyConstraint();
                    updateItem( item, editPoints );
                    updatePoints( item, editPoints );

                    editPoints.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
                }
                else
                {
                    m_toolMgr->PassEvent();
                }
            }

            else if( evt->IsClick( BUT_LEFT ) )
            {
                m_toolMgr->PassEvent();
            }
        }

        m_toolMgr->GetView()->Remove( &editPoints );
        item->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
    }

    controls->ShowCursor( false );
    controls->SetAutoPan( false );
    controls->SetSnapping( false );

    setTransitions();

    return 0;
}


void POINT_EDITOR::setTransitions()
{
    Go( &POINT_EDITOR::OnSelected, m_selectionTool->SelectedEvent );
}


void POINT_EDITOR::updateItem( EDA_ITEM* aItem, EDIT_POINTS& aPoints ) const
{
    switch( aItem->Type() )
    {
    case PCB_LINE_T:
    {
        DRAWSEGMENT* segment = static_cast<DRAWSEGMENT*>( aItem );
        switch( segment->GetShape() )
        {
        case S_SEGMENT:
            if( &aPoints[0] == m_dragPoint )
                segment->SetStart( wxPoint( aPoints[0].GetPosition().x, aPoints[0].GetPosition().y ) );
            else if( &aPoints[1] == m_dragPoint )
                segment->SetEnd( wxPoint( aPoints[1].GetPosition().x, aPoints[1].GetPosition().y ) );

            break;

        case S_ARC:
        {
            const VECTOR2I& center = aPoints[0].GetPosition();
            const VECTOR2I& start = aPoints[1].GetPosition();
            const VECTOR2I& end = aPoints[2].GetPosition();

            if( center != segment->GetCenter() )
            {
                wxPoint moveVector = wxPoint( center.x, center.y ) - segment->GetCenter();
                segment->Move( moveVector );

                aPoints[1].SetPosition( segment->GetArcStart() );
                aPoints[2].SetPosition( segment->GetArcEnd() );
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


void POINT_EDITOR::updatePoints( const EDA_ITEM* aItem, EDIT_POINTS& aPoints ) const
{
    switch( aItem->Type() )
    {
    case PCB_LINE_T:
    {
        const DRAWSEGMENT* segment = static_cast<const DRAWSEGMENT*>( aItem );
        {
            switch( segment->GetShape() )
            {
            case S_SEGMENT:
                aPoints[0].SetPosition( segment->GetStart() );
                aPoints[1].SetPosition( segment->GetEnd() );
                break;

            case S_ARC:
                aPoints[0].SetPosition( segment->GetCenter() );
                aPoints[1].SetPosition( segment->GetArcStart() );
                aPoints[2].SetPosition( segment->GetArcEnd() );
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
