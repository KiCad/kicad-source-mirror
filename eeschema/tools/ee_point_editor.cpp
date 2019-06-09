/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "ee_point_editor.h"
#include <tool/tool_manager.h>
#include <view/view_controls.h>
#include <gal/graphics_abstraction_layer.h>
#include <geometry/seg.h>
#include <confirm.h>

#include <tools/ee_actions.h>
#include <tools/ee_selection_tool.h>
#include <bitmaps.h>
#include <status_popup.h>
#include <sch_edit_frame.h>
#include <sch_line.h>
#include <sch_sheet.h>
#include <lib_edit_frame.h>
#include <lib_arc.h>
#include <lib_circle.h>
#include <lib_rectangle.h>
#include <lib_polyline.h>
#include <eeschema_id.h>


// Point editor
TOOL_ACTION EE_ACTIONS::pointEditorAddCorner( "eeschema.PointEditor.addCorner",
        AS_GLOBAL, 0, "",
        _( "Create Corner" ), _( "Create a corner" ), 
        add_corner_xpm );

TOOL_ACTION EE_ACTIONS::pointEditorRemoveCorner( "eeschema.PointEditor.removeCorner",
        AS_GLOBAL, 0, "",
        _( "Remove Corner" ), _( "Remove corner" ), 
        delete_xpm );


// Few constants to avoid using bare numbers for point indices
enum ARC_POINTS
{
    ARC_CENTER, ARC_START, ARC_END
};

enum CIRCLE_POINTS
{
    CIRC_CENTER, CIRC_END
};

enum RECTANGLE_POINTS
{
    RECT_TOPLEFT, RECT_TOPRIGHT, RECT_BOTLEFT, RECT_BOTRIGHT
};

enum LINE_POINTS
{
    LINE_START, LINE_END
};

class EDIT_POINTS_FACTORY
{
public:
    static std::shared_ptr<EDIT_POINTS> Make( EDA_ITEM* aItem, SCH_BASE_FRAME* frame )
    {
        std::shared_ptr<EDIT_POINTS> points = std::make_shared<EDIT_POINTS>( aItem );

        if( !aItem )
            return points;

        // Generate list of edit points based on the item type
        switch( aItem->Type() )
        {
        case LIB_ARC_T:
        {
            LIB_ARC* arc = (LIB_ARC*) aItem;

            points->AddPoint( mapCoords( arc->GetPosition() ) );
            points->AddPoint( mapCoords( arc->GetStart() ) );
            points->AddPoint( mapCoords( arc->GetEnd() ) );
            break;
        }
        case LIB_CIRCLE_T:
        {
            LIB_CIRCLE* circle = (LIB_CIRCLE*) aItem;

            points->AddPoint( mapCoords( circle->GetPosition() ) );
            points->AddPoint( mapCoords( circle->GetEnd() ) );
            break;
        }
        case LIB_POLYLINE_T:
        {
            LIB_POLYLINE* lines = (LIB_POLYLINE*) aItem;
            const std::vector<wxPoint>& pts = lines->GetPolyPoints();

            for( wxPoint pt : pts )
                points->AddPoint( mapCoords( pt ) );

            break;
        }
        case LIB_RECTANGLE_T:
        {
            LIB_RECTANGLE* rect = (LIB_RECTANGLE*) aItem;
            wxPoint        topLeft = rect->GetPosition();
            wxPoint        botRight = rect->GetEnd();

            points->AddPoint( mapCoords( topLeft ) );
            points->AddPoint( mapCoords( botRight.x, topLeft.y ) );
            points->AddPoint( mapCoords( topLeft.x, botRight.y ) );
            points->AddPoint( mapCoords( botRight ) );
            break;
        }
        case SCH_SHEET_T:
        {
            SCH_SHEET* sheet = (SCH_SHEET*) aItem;
            wxPoint    topLeft = sheet->GetPosition();
            wxPoint    botRight = sheet->GetPosition() + sheet->GetSize();

            points->AddPoint( (wxPoint) topLeft );
            points->AddPoint( wxPoint( botRight.x, topLeft.y ) );
            points->AddPoint( wxPoint( topLeft.x, botRight.y ) );
            points->AddPoint( (wxPoint) botRight );
            break;
        }
        case SCH_LINE_T:
        {
            SCH_LINE* line = (SCH_LINE*) aItem;
            SCH_LINE* connectedStart = nullptr;
            SCH_LINE* connectedEnd = nullptr;

            for( SCH_ITEM* test = frame->GetScreen()->GetDrawItems(); test; test = test->Next() )
            {
                if( test->Type() != SCH_LINE_T || test->GetLayer() != LAYER_NOTES )
                    continue;

                if( test == aItem )
                    continue;

                SCH_LINE* testLine = (SCH_LINE*) test;
                testLine->ClearFlags( STARTPOINT | ENDPOINT );

                if( testLine->GetStartPoint() == line->GetStartPoint() )
                {
                    connectedStart = testLine;
                    testLine->SetFlags( STARTPOINT );
                }
                else if( testLine->GetEndPoint() == line->GetStartPoint() )
                {
                    connectedStart = testLine;
                    testLine->SetFlags( ENDPOINT );
                }
                else if( testLine->GetStartPoint() == line->GetEndPoint() )
                {
                    connectedEnd = testLine;
                    testLine->SetFlags( STARTPOINT );
                }
                else if( testLine->GetEndPoint() == line->GetEndPoint() )
                {
                    connectedEnd = testLine;
                    testLine->SetFlags( ENDPOINT );
                }
            }

            points->AddPoint( line->GetStartPoint(), connectedStart );
            points->AddPoint( line->GetEndPoint(), connectedEnd );
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


EE_POINT_EDITOR::EE_POINT_EDITOR() :
    EE_TOOL_BASE<SCH_BASE_FRAME>( "eeschema.PointEditor" ),
    m_editedPoint( nullptr )
{
}


void EE_POINT_EDITOR::Reset( RESET_REASON aReason )
{
    EE_TOOL_BASE::Reset( aReason );

    m_editPoints.reset();
}


bool EE_POINT_EDITOR::Init()
{
    EE_TOOL_BASE::Init();

    auto& menu = m_selectionTool->GetToolMenu().GetMenu();
    menu.AddItem( EE_ACTIONS::pointEditorAddCorner,
                              std::bind( &EE_POINT_EDITOR::addCornerCondition, this, _1 ) );
    menu.AddItem( EE_ACTIONS::pointEditorRemoveCorner,
                              std::bind( &EE_POINT_EDITOR::removeCornerCondition, this, _1 ) );

    return true;
}


void EE_POINT_EDITOR::updateEditedPoint( const TOOL_EVENT& aEvent )
{
    EDIT_POINT* point = m_editedPoint;

    if( aEvent.IsMotion() )
    {
        point = m_editPoints->FindPoint( aEvent.Position(), getView() );
    }
    else if( aEvent.IsDrag( BUT_LEFT ) )
    {
        point = m_editPoints->FindPoint( aEvent.DragOrigin(), getView() );
    }

    if( m_editedPoint != point )
        setEditedPoint( point );
}


int EE_POINT_EDITOR::Main( const TOOL_EVENT& aEvent )
{
    static KICAD_T pointTypes[] = { LIB_ARC_T, LIB_CIRCLE_T, LIB_POLYLINE_T, LIB_RECTANGLE_T,
                                    SCH_SHEET_T, SCH_LINE_LOCATE_GRAPHIC_LINE_T, EOT };

    if( !m_selectionTool )
        return 0;

    const EE_SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.Size() != 1 || !selection.Front()->IsType( pointTypes ) )
        return 0;

    // Wait till drawing tool is done
    if( selection.Front()->IsNew() )
        return 0;

    Activate();

    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    KIGFX::VIEW*          view = getView();
    int                   savedToolID = m_frame->GetToolId();
    wxString              savedToolMsg = m_frame->GetToolMsg();
    EDA_ITEM*             item = (EDA_ITEM*) selection.Front();

    controls->ShowCursor( true );

    m_editPoints = EDIT_POINTS_FACTORY::Make( item, m_frame );

    if( !m_editPoints )
        return 0;

    view->Add( m_editPoints.get() );
    setEditedPoint( nullptr );
    bool inDrag = false;
    bool modified = false;

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        if( !m_editPoints
             || evt->Matches( EVENTS::ClearedEvent )
             || evt->Matches( EVENTS::UnselectedEvent )
             || evt->Matches( EVENTS::SelectedEvent ) )
        {
            break;
        }

        if ( !inDrag )
            updateEditedPoint( *evt );

        if( evt->IsDrag( BUT_LEFT ) && m_editedPoint )
        {
            m_frame->SetToolID( ID_DRAG_POINT, -1, _( "Drag Point" ) );

            if( !inDrag )
            {
                saveItemsToUndo();
                controls->ForceCursorPosition( false );
                inDrag = true;
                modified = true;
            }

            m_editedPoint->SetPosition( controls->GetCursorPosition( !evt->Modifier( MD_ALT ) ) );

            updateItem();
            updatePoints();
        }

        else if( evt->IsMouseUp( BUT_LEFT ) )
        {
            controls->SetAutoPan( false );
            m_frame->SetToolID( savedToolID, -1, savedToolMsg );

            inDrag = false;

            m_toolMgr->PassEvent();
        }

        else if( evt->IsCancel() )
        {
            if( inDrag )      // Restore the last change
            {
                rollbackFromUndo();
                modified = false;
            }

            m_frame->SetToolID( savedToolID, -1, savedToolMsg );
            break;
        }

        else
        {
            m_toolMgr->PassEvent();
        }

        controls->SetAutoPan( inDrag );
        controls->CaptureCursor( inDrag );
    }

    controls->SetAutoPan( false );
    controls->CaptureCursor( false );
    m_frame->SetToolID( savedToolID, -1, savedToolMsg );

    if( m_editPoints )
    {
        view->Remove( m_editPoints.get() );

        if( modified )
            m_frame->OnModify();

        m_editPoints.reset();
        m_frame->GetCanvas()->Refresh();
    }

    return 0;
}


void pinEditedCorner( int editedPointIndex, int minWidth, int minHeight, VECTOR2I& topLeft,
                      VECTOR2I& topRight, VECTOR2I& botLeft, VECTOR2I& botRight )
{
    switch( editedPointIndex )
    {
    case RECT_TOPLEFT:
        // pin edited point within opposite corner
        topLeft.x = std::min( topLeft.x, botRight.x - minWidth );
        topLeft.y = std::min( topLeft.y, botRight.y - minHeight );

        // push edited point edges to adjacent corners
        topRight.y = topLeft.y;
        botLeft.x = topLeft.x;

        break;

    case RECT_TOPRIGHT:
        // pin edited point within opposite corner
        topRight.x = std::max( topRight.x, botLeft.x + minWidth );
        topRight.y = std::min( topRight.y, botLeft.y - minHeight );

        // push edited point edges to adjacent corners
        topLeft.y = topRight.y;
        botRight.x = topRight.x;

        break;

    case RECT_BOTLEFT:
        // pin edited point within opposite corner
        botLeft.x = std::min( botLeft.x, topRight.x - minWidth );
        botLeft.y = std::max( botLeft.y, topRight.y + minHeight );

        // push edited point edges to adjacent corners
        botRight.y = botLeft.y;
        topLeft.x = botLeft.x;

        break;

    case RECT_BOTRIGHT:
        // pin edited point within opposite corner
        botRight.x = std::max( botRight.x, topLeft.x + minWidth );
        botRight.y = std::max( botRight.y, topLeft.y + minHeight );

        // push edited point edges to adjacent corners
        botLeft.y = botRight.y;
        topRight.x = botRight.x;

        break;
    }
}


void EE_POINT_EDITOR::updateItem() const
{
    EDA_ITEM* item = m_editPoints->GetParent();

    if( !item )
        return;

    switch( item->Type() )
    {
    case LIB_ARC_T:
    {
        LIB_ARC* arc = (LIB_ARC*) item;

        arc->SetPosition( mapCoords( m_editPoints->Point( ARC_CENTER ).GetPosition() ) );
        arc->SetStart( mapCoords( m_editPoints->Point( ARC_START ).GetPosition() ) );
        arc->SetEnd( mapCoords( m_editPoints->Point( ARC_END ).GetPosition() ) );
        break;
    }

    case LIB_CIRCLE_T:
    {
        LIB_CIRCLE* circle = (LIB_CIRCLE*) item;

        circle->SetPosition( mapCoords( m_editPoints->Point( CIRC_CENTER ).GetPosition() ) );
        circle->SetEnd( mapCoords( m_editPoints->Point( CIRC_END ).GetPosition() ) );
        break;
    }

    case LIB_POLYLINE_T:
    {
        LIB_POLYLINE* lines = (LIB_POLYLINE*) item;

        lines->ClearPoints();

        for( unsigned i = 0; i < m_editPoints->PointsSize(); ++i )
            lines->AddPoint( mapCoords( m_editPoints->Point( i ).GetPosition() ) );

        break;
    }

    case LIB_RECTANGLE_T:
    {
        LIB_RECTANGLE* rect = (LIB_RECTANGLE*) item;
        VECTOR2I       topLeft = m_editPoints->Point( RECT_TOPLEFT ).GetPosition();
        VECTOR2I       topRight = m_editPoints->Point( RECT_TOPRIGHT ).GetPosition();
        VECTOR2I       botLeft = m_editPoints->Point( RECT_BOTLEFT ).GetPosition();
        VECTOR2I       botRight = m_editPoints->Point( RECT_BOTRIGHT ).GetPosition();

        pinEditedCorner( getEditedPointIndex(), Mils2iu( 1 ), Mils2iu( 1 ),
                         topLeft, topRight, botLeft, botRight );

        rect->SetPosition( mapCoords( topLeft ) );
        rect->SetEnd( mapCoords( botRight ) );
        break;
    }

    case SCH_SHEET_T:
    {
        SCH_SHEET* sheet = (SCH_SHEET*) item;
        VECTOR2I   topLeft = m_editPoints->Point( RECT_TOPLEFT ).GetPosition();
        VECTOR2I   topRight = m_editPoints->Point( RECT_TOPRIGHT ).GetPosition();
        VECTOR2I   botLeft = m_editPoints->Point( RECT_BOTLEFT ).GetPosition();
        VECTOR2I   botRight = m_editPoints->Point( RECT_BOTRIGHT ).GetPosition();

        pinEditedCorner( getEditedPointIndex(), sheet->GetMinWidth(), sheet->GetMinHeight(),
                         topLeft, topRight, botLeft, botRight );

        sheet->SetPosition( (wxPoint) topLeft );
        sheet->SetSize( wxSize( botRight.x - topLeft.x, botRight.y - topLeft.y ) );

        // Keep sheet pins attached to edges:
        for( SCH_SHEET_PIN& pin : sheet->GetPins() )
        {
            wxPoint pos = pin.GetPosition();

            switch( pin.GetEdge() )
            {
            case SHEET_LEFT_SIDE:      pos.x = topLeft.x;  break;
            case SHEET_RIGHT_SIDE:     pos.x = topRight.x; break;
            case SHEET_TOP_SIDE:       pos.y = topLeft.y;  break;
            case SHEET_BOTTOM_SIDE:    pos.y = botLeft.y;  break;
            case SHEET_UNDEFINED_SIDE: break;
            }

            pin.SetPosition( pos );
        }

        break;
    }

    case SCH_LINE_T:
    {
        SCH_LINE* line = (SCH_LINE*) item;

        line->SetStartPoint( (wxPoint) m_editPoints->Point( LINE_START ).GetPosition() );
        line->SetEndPoint( (wxPoint) m_editPoints->Point( LINE_END ).GetPosition() );

        SCH_LINE* connection = (SCH_LINE*) ( m_editPoints->Point( LINE_START ).GetConnection() );

        if( connection )
        {
            if( ( connection->GetFlags() & STARTPOINT ) != 0 )
                connection->SetStartPoint( line->GetPosition() );
            else if( ( connection->GetFlags() & ENDPOINT ) != 0 )
                connection->SetEndPoint( line->GetPosition() );

            getView()->Update( connection, KIGFX::GEOMETRY );
        }

        connection = (SCH_LINE*) ( m_editPoints->Point( LINE_END ).GetConnection() );

        if( connection )
        {
            if( ( connection->GetFlags() & STARTPOINT ) != 0 )
                connection->SetStartPoint( line->GetEndPoint() );
            else if( ( connection->GetFlags() & ENDPOINT ) != 0 )
                connection->SetEndPoint( line->GetEndPoint() );

            getView()->Update( connection, KIGFX::GEOMETRY );
        }

        break;
    }

    default:
        break;
    }

    updateView( item );
    m_frame->SetMsgPanel( item );
}


void EE_POINT_EDITOR::updatePoints()
{
    if( !m_editPoints )
        return;

    EDA_ITEM* item = m_editPoints->GetParent();

    if( !item )
        return;

    switch( item->Type() )
    {
    case LIB_ARC_T:
    {
        LIB_ARC* arc = (LIB_ARC*) item;

        m_editPoints->Point( ARC_CENTER ).SetPosition( mapCoords( arc->GetPosition() ) );
        m_editPoints->Point( ARC_START ).SetPosition( mapCoords( arc->GetStart() ) );
        m_editPoints->Point( ARC_END ).SetPosition( mapCoords( arc->GetEnd() ) );
        break;
    }

    case LIB_CIRCLE_T:
    {
        LIB_CIRCLE* circle = (LIB_CIRCLE*) item;

        m_editPoints->Point( CIRC_CENTER ).SetPosition( mapCoords( circle->GetPosition() ) );
        m_editPoints->Point( CIRC_END ).SetPosition( mapCoords( circle->GetEnd() ) );
        break;
    }

    case LIB_POLYLINE_T:
    {
        LIB_POLYLINE* lines = (LIB_POLYLINE*) item;
        const std::vector<wxPoint>& pts = lines->GetPolyPoints();

        if( m_editPoints->PointsSize() != (unsigned) pts.size() )
        {
            getView()->Remove( m_editPoints.get() );
            m_editedPoint = nullptr;
            m_editPoints = EDIT_POINTS_FACTORY::Make( item, m_frame );
            getView()->Add(m_editPoints.get() );
        }
        else
        {
            for( unsigned i = 0; i < pts.size(); i++ )
                m_editPoints->Point( i ).SetPosition( mapCoords( pts[i] ) );
        }

        break;
    }

    case LIB_RECTANGLE_T:
    {
        LIB_RECTANGLE* rect = (LIB_RECTANGLE*) item;
        wxPoint        topLeft = rect->GetPosition();
        wxPoint        botRight = rect->GetEnd();

        m_editPoints->Point( RECT_TOPLEFT ).SetPosition( mapCoords( topLeft ) );
        m_editPoints->Point( RECT_TOPRIGHT ).SetPosition( mapCoords( botRight.x, topLeft.y ) );
        m_editPoints->Point( RECT_BOTLEFT ).SetPosition( mapCoords( topLeft.x, botRight.y ) );
        m_editPoints->Point( RECT_BOTRIGHT ).SetPosition( mapCoords( botRight ) );
        break;
    }

    case SCH_SHEET_T:
    {
        SCH_SHEET* sheet = (SCH_SHEET*) item;
        wxPoint    topLeft = sheet->GetPosition();
        wxPoint    botRight = sheet->GetPosition() + sheet->GetSize();

        m_editPoints->Point( RECT_TOPLEFT ).SetPosition( topLeft );
        m_editPoints->Point( RECT_TOPRIGHT ).SetPosition( botRight.x, topLeft.y );
        m_editPoints->Point( RECT_BOTLEFT ).SetPosition( topLeft.x, botRight.y );
        m_editPoints->Point( RECT_BOTRIGHT ).SetPosition( botRight );
        break;
    }

    case SCH_LINE_T:
    {
        SCH_LINE* line = (SCH_LINE*) item;

        m_editPoints->Point( LINE_START ).SetPosition( line->GetStartPoint() );
        m_editPoints->Point( LINE_END ).SetPosition( line->GetEndPoint() );
        break;
    }

    default:
        break;
    }

    getView()->Update( m_editPoints.get() );
}


void EE_POINT_EDITOR::setEditedPoint( EDIT_POINT* aPoint )
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();

    if( aPoint )
    {
        controls->ForceCursorPosition( true, aPoint->GetPosition() );
        controls->ShowCursor( true );
    }
    else
    {
        controls->ShowCursor( false );
        controls->ForceCursorPosition( false );
    }

    m_editedPoint = aPoint;
}


bool EE_POINT_EDITOR::removeCornerCondition( const SELECTION& )
{
    if( !m_editPoints || !m_editedPoint )
        return false;

    LIB_POLYLINE* polyLine = dynamic_cast<LIB_POLYLINE*>( m_editPoints->GetParent() );

    if( !polyLine || polyLine->GetCornerCount() < 3 )
        return false;

    const std::vector<wxPoint>& pts = polyLine->GetPolyPoints();

    for( unsigned i = 0; i < polyLine->GetCornerCount(); ++i )
    {
        if( pts[i] == mapCoords( m_editedPoint->GetPosition() ) )
            return true;
    }

    return false;
}


bool EE_POINT_EDITOR::addCornerCondition( const SELECTION& )
{
    if( !m_editPoints || !m_editedPoint )
        return false;

    LIB_POLYLINE* polyLine = dynamic_cast<LIB_POLYLINE*>( m_editPoints->GetParent() );

    if( !polyLine )
        return false;

    VECTOR2I cursorPos = getViewControls()->GetCursorPosition();
    double   threshold = getView()->ToWorld( EDIT_POINT::POINT_SIZE );

    return polyLine->HitTest( mapCoords( cursorPos ), (int) threshold );
}


int EE_POINT_EDITOR::addCorner( const TOOL_EVENT& aEvent )
{
    if( !m_editPoints )
        return 0;

    LIB_POLYLINE* polyLine = dynamic_cast<LIB_POLYLINE*>( m_editPoints->GetParent() );

    if( !polyLine )
        return false;

    VECTOR2I cursorPos = getViewControls()->GetCursorPosition( !aEvent.Modifier( MD_ALT ) );
    polyLine->AddCorner( mapCoords( cursorPos ) );

    updateView( polyLine );
    updatePoints();

    return 0;
}


int EE_POINT_EDITOR::removeCorner( const TOOL_EVENT& aEvent )
{
    if( !m_editPoints || !m_editedPoint )
        return 0;

    LIB_POLYLINE* polyLine = dynamic_cast<LIB_POLYLINE*>( m_editPoints->GetParent() );

    if( !polyLine || polyLine->GetCornerCount() < 3 )
        return 0;

    polyLine->RemoveCorner( getEditedPointIndex() );

    updateView( polyLine );
    updatePoints();

    return 0;
}


int EE_POINT_EDITOR::modifiedSelection( const TOOL_EVENT& aEvent )
{
    updatePoints();
    return 0;
}


void EE_POINT_EDITOR::saveItemsToUndo()
{
    if( m_isLibEdit )
    {
        saveCopyInUndoList( m_editPoints->GetParent()->GetParent(), UR_LIBEDIT );
    }
    else
    {
        saveCopyInUndoList( (SCH_ITEM*) m_editPoints->GetParent(), UR_CHANGED );

        if( m_editPoints->GetParent()->Type() == SCH_LINE_T )
        {
            EDA_ITEM* connection = m_editPoints->Point( LINE_START ).GetConnection();

            if( connection )
                saveCopyInUndoList( (SCH_ITEM*) connection, UR_CHANGED, true );

            connection = m_editPoints->Point( LINE_END ).GetConnection();

            if( connection )
                saveCopyInUndoList( (SCH_ITEM*) connection, UR_CHANGED, true );
        }
    }
}


void EE_POINT_EDITOR::rollbackFromUndo()
{
    if( m_isLibEdit )
        static_cast<LIB_EDIT_FRAME*>( m_frame )->RollbackPartFromUndo();
    else
        static_cast<SCH_EDIT_FRAME*>( m_frame )->RollbackSchematicFromUndo();
}


void EE_POINT_EDITOR::setTransitions()
{
    Go( &EE_POINT_EDITOR::Main,              EVENTS::SelectedEvent );
    Go( &EE_POINT_EDITOR::Main,              ACTIONS::activatePointEditor.MakeEvent() );
    Go( &EE_POINT_EDITOR::addCorner,         EE_ACTIONS::pointEditorAddCorner.MakeEvent() );
    Go( &EE_POINT_EDITOR::removeCorner,      EE_ACTIONS::pointEditorRemoveCorner.MakeEvent() );
    Go( &EE_POINT_EDITOR::modifiedSelection, EVENTS::SelectedItemsModified );
}


