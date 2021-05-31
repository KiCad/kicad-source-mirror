/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <geometry/seg.h>
#include <tools/ee_actions.h>
#include <tools/ee_selection_tool.h>
#include <bitmaps.h>
#include <sch_edit_frame.h>
#include <sch_line.h>
#include <sch_bitmap.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <symbol_edit_frame.h>
#include <lib_arc.h>
#include <lib_circle.h>
#include <lib_rectangle.h>
#include <lib_polyline.h>


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
            // point editor works only with rectangles having width and height > 0
            // Some symbols can have rectangles with width or height < 0
            // So normalize the size:
            BOX2I dummy;
            dummy.SetOrigin( mapCoords( rect->GetPosition() ) );
            dummy.SetEnd( mapCoords( rect->GetEnd() ) );
            dummy.Normalize();
            VECTOR2I topLeft = dummy.GetPosition();
            VECTOR2I botRight = dummy.GetEnd();

            points->AddPoint( topLeft );
            points->AddPoint( VECTOR2I( botRight.x, topLeft.y ) );
            points->AddPoint( VECTOR2I( topLeft.x, botRight.y ) );
            points->AddPoint( botRight );
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
        case SCH_BITMAP_T:
        {
            SCH_BITMAP* bitmap = (SCH_BITMAP*) aItem;
            wxPoint     topLeft = bitmap->GetPosition() - bitmap->GetSize() / 2;
            wxPoint     botRight = bitmap->GetPosition() + bitmap->GetSize() / 2;

            points->AddPoint( (wxPoint) topLeft );
            points->AddPoint( wxPoint( botRight.x, topLeft.y ) );
            points->AddPoint( wxPoint( topLeft.x, botRight.y ) );
            points->AddPoint( (wxPoint) botRight );
            break;
        }
        case SCH_LINE_T:
        {
            SCH_LINE* line = (SCH_LINE*) aItem;
            std::pair<EDA_ITEM*, int> connectedStart = { nullptr, STARTPOINT };
            std::pair<EDA_ITEM*, int> connectedEnd = { nullptr, STARTPOINT };

            for( SCH_ITEM* test : frame->GetScreen()->Items().OfType( SCH_LINE_T ) )
            {
                if( test->GetLayer() != LAYER_NOTES )
                    continue;

                if( test == aItem )
                    continue;

                SCH_LINE* testLine = static_cast<SCH_LINE*>( test );

                if( testLine->GetStartPoint() == line->GetStartPoint() )
                {
                    connectedStart = { testLine, STARTPOINT };
                }
                else if( testLine->GetEndPoint() == line->GetStartPoint() )
                {
                    connectedStart = { testLine, ENDPOINT };
                }
                else if( testLine->GetStartPoint() == line->GetEndPoint() )
                {
                    connectedEnd = { testLine, STARTPOINT };
                }
                else if( testLine->GetEndPoint() == line->GetEndPoint() )
                {
                    connectedEnd = { testLine, ENDPOINT };
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
    else
    {
        point = m_editPoints->FindPoint( getViewControls()->GetCursorPosition(), getView() );
    }

    if( m_editedPoint != point )
        setEditedPoint( point );
}


int EE_POINT_EDITOR::Main( const TOOL_EVENT& aEvent )
{
    static KICAD_T supportedTypes[] = {
        LIB_ARC_T,
        LIB_CIRCLE_T,
        LIB_POLYLINE_T,
        LIB_RECTANGLE_T,
        SCH_SHEET_T,
        SCH_LINE_LOCATE_GRAPHIC_LINE_T,
        SCH_BITMAP_T,
        EOT
    };

    if( !m_selectionTool )
        return 0;

    const EE_SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.Size() != 1 || !selection.Front()->IsType( supportedTypes ) )
        return 0;

    // Wait till drawing tool is done
    if( selection.Front()->IsNew() )
        return 0;

    Activate();

    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    KIGFX::VIEW*          view = getView();
    EDA_ITEM*             item = (EDA_ITEM*) selection.Front();

    controls->ShowCursor( true );

    m_editPoints = EDIT_POINTS_FACTORY::Make( item, m_frame );
    view->Add( m_editPoints.get() );
    setEditedPoint( nullptr );
    updateEditedPoint( aEvent );
    bool inDrag = false;
    bool modified = false;

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        if( !m_editPoints || evt->IsSelectionEvent() )
            break;

        if ( !inDrag )
            updateEditedPoint( *evt );

        if( evt->IsDrag( BUT_LEFT ) && m_editedPoint )
        {
            if( !inDrag )
            {
                saveItemsToUndo();
                controls->ForceCursorPosition( false );
                inDrag = true;
                modified = true;
            }

            bool snap = !evt->DisableGridSnapping();

            if( item->Type() == LIB_ARC_T && getEditedPointIndex() == ARC_CENTER )
                snap = false;

            m_editedPoint->SetPosition( controls->GetCursorPosition( snap ) );

            updateParentItem();
            updatePoints();
        }
        else if( inDrag && evt->IsMouseUp( BUT_LEFT ) )
        {
            if( modified )
            {
                m_frame->OnModify();
                modified = false;
            }

            controls->SetAutoPan( false );
            inDrag = false;
        }
        else if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            if( inDrag )      // Restore the last change
            {
                rollbackFromUndo();
                inDrag = false;
                modified = false;
                break;
            }
            else if( evt->IsCancelInteractive() )
            {
                break;
            }

            if( evt->IsActivate() )
                break;
        }
        else
        {
            evt->SetPassEvent();
        }

        controls->SetAutoPan( inDrag );
        controls->CaptureCursor( inDrag );
    }

    controls->SetAutoPan( false );
    controls->CaptureCursor( false );

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

/**
 * Update the coordinates of 4 corners of a rectangle, accordint to constraints
 * and the moved corner
 * @param aEditedPointIndex is the corner id
 * @param minWidth is the minimal width constraint
 * @param minHeight is the minimal height constraint
 * @param topLeft is the RECT_TOPLEFT to constraint
 * @param topRight is the RECT_TOPRIGHT to constraint
 * @param botLeft is the RECT_BOTLEFT to constraint
 * @param botRight is the RECT_BOTRIGHT to constraint
 * @param aGridSize is the a constraint: if > 1 new coordinates are on this grid
 */
static void pinEditedCorner( int aEditedPointIndex, int minWidth, int minHeight,
                             VECTOR2I& topLeft, VECTOR2I& topRight,
                             VECTOR2I& botLeft, VECTOR2I& botRight,
                             int aGridSize = 0 )
{
    auto alignToGrid =
            [&]( const VECTOR2I& aPoint ) -> VECTOR2I
            {
                return VECTOR2I( KiROUND( aPoint.x / aGridSize ) * aGridSize,
                                 KiROUND( aPoint.y / aGridSize ) * aGridSize );
            };

    switch( aEditedPointIndex )
    {
    case RECT_TOPLEFT:
        // pin edited point within opposite corner
        topLeft.x = std::min( topLeft.x, botRight.x - minWidth );
        topLeft.y = std::min( topLeft.y, botRight.y - minHeight );

        if( aGridSize > 1 )     // Keep point on specified grid size
            topLeft = alignToGrid( topLeft );

        // push edited point edges to adjacent corners
        topRight.y = topLeft.y;
        botLeft.x = topLeft.x;

        break;

    case RECT_TOPRIGHT:
        // pin edited point within opposite corner
        topRight.x = std::max( topRight.x, botLeft.x + minWidth );
        topRight.y = std::min( topRight.y, botLeft.y - minHeight );

        if( aGridSize > 1 )     // Keep point on specified grid size
            topRight = alignToGrid( topRight );

        // push edited point edges to adjacent corners
        topLeft.y = topRight.y;
        botRight.x = topRight.x;

        break;

    case RECT_BOTLEFT:
        // pin edited point within opposite corner
        botLeft.x = std::min( botLeft.x, topRight.x - minWidth );
        botLeft.y = std::max( botLeft.y, topRight.y + minHeight );

        if( aGridSize > 1 )     // Keep point on specified grid size
            botLeft = alignToGrid( botLeft );

        // push edited point edges to adjacent corners
        botRight.y = botLeft.y;
        topLeft.x = botLeft.x;

        break;

    case RECT_BOTRIGHT:
        // pin edited point within opposite corner
        botRight.x = std::max( botRight.x, topLeft.x + minWidth );
        botRight.y = std::max( botRight.y, topLeft.y + minHeight );

        if( aGridSize > 1 )     // Keep point on specified grid size
            botRight = alignToGrid( botRight );

        // push edited point edges to adjacent corners
        botLeft.y = botRight.y;
        topRight.x = botRight.x;

        break;
    }
}


void EE_POINT_EDITOR::updateParentItem() const
{
    EDA_ITEM* item = m_editPoints->GetParent();

    if( !item )
        return;

    switch( item->Type() )
    {
    case LIB_ARC_T:
    {
        LIB_ARC* arc = (LIB_ARC*) item;
        int      i = getEditedPointIndex();

        if( i == ARC_CENTER )
        {
            arc->SetEditState( 4 );
            arc->CalcEdit( mapCoords( m_editPoints->Point( ARC_CENTER ).GetPosition() ) );
        }
        else if( i == ARC_START )
        {
            arc->SetEditState( 2 );
            arc->CalcEdit( mapCoords( m_editPoints->Point( ARC_START ).GetPosition() ) );
        }
        else if( i == ARC_END )
        {
            arc->SetEditState( 3 );
            arc->CalcEdit( mapCoords( m_editPoints->Point( ARC_END ).GetPosition() ) );
        }

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
        VECTOR2I       topLeft = m_editPoints->Point( RECT_TOPLEFT ).GetPosition();
        VECTOR2I       topRight = m_editPoints->Point( RECT_TOPRIGHT ).GetPosition();
        VECTOR2I       botLeft = m_editPoints->Point( RECT_BOTLEFT ).GetPosition();
        VECTOR2I       botRight = m_editPoints->Point( RECT_BOTRIGHT ).GetPosition();

        pinEditedCorner( getEditedPointIndex(), Mils2iu( 1 ), Mils2iu( 1 ),
                         topLeft, topRight, botLeft, botRight );

        LIB_RECTANGLE* rect = (LIB_RECTANGLE*) item;
        rect->SetPosition( mapCoords( topLeft ) );
        rect->SetEnd( mapCoords( botRight ) );
        break;
    }

    case SCH_BITMAP_T:
    {
        SCH_BITMAP* bitmap = (SCH_BITMAP*) item;
        VECTOR2I    topLeft = m_editPoints->Point( RECT_TOPLEFT ).GetPosition();
        VECTOR2I    topRight = m_editPoints->Point( RECT_TOPRIGHT ).GetPosition();
        VECTOR2I    botLeft = m_editPoints->Point( RECT_BOTLEFT ).GetPosition();
        VECTOR2I    botRight = m_editPoints->Point( RECT_BOTRIGHT ).GetPosition();

        pinEditedCorner( getEditedPointIndex(), Mils2iu( 50 ), Mils2iu( 50 ),
                         topLeft, topRight, botLeft, botRight );

        double oldWidth = bitmap->GetSize().x;
        double newWidth = topRight.x - topLeft.x;
        double widthRatio = newWidth / oldWidth;

        double oldHeight = bitmap->GetSize().y;
        double newHeight = botLeft.y - topLeft.y;
        double heightRatio = newHeight / oldHeight;

        bitmap->SetImageScale( bitmap->GetImageScale() * std::min( widthRatio, heightRatio ) );
        break;
    }

    case SCH_SHEET_T:
    {
        SCH_SHEET* sheet = (SCH_SHEET*) item;
        VECTOR2I   topLeft = m_editPoints->Point( RECT_TOPLEFT ).GetPosition();
        VECTOR2I   topRight = m_editPoints->Point( RECT_TOPRIGHT ).GetPosition();
        VECTOR2I   botLeft = m_editPoints->Point( RECT_BOTLEFT ).GetPosition();
        VECTOR2I   botRight = m_editPoints->Point( RECT_BOTRIGHT ).GetPosition();

        // The grid size used to place connected items. because a sheet contains
        // connected items (sheet pins), keep corners coordinates on this grid.
        // Otherwise, some sheet pins can be moved off grid
        int grid_size = Mils2iu( 50 );
        int edited = getEditedPointIndex();

        pinEditedCorner( getEditedPointIndex(),
                         sheet->GetMinWidth( edited == RECT_TOPRIGHT || edited == RECT_BOTRIGHT ),
                         sheet->GetMinHeight( edited == RECT_BOTLEFT || edited == RECT_BOTRIGHT ),
                         topLeft, topRight, botLeft, botRight, grid_size );

        // Pin positions are relative to origin.  Attempt to leave them where they
        // are if the origin moves.
        wxPoint originDelta = sheet->GetPosition() - (wxPoint) topLeft;

        sheet->SetPosition( (wxPoint) topLeft );
        sheet->SetSize( wxSize( botRight.x - topLeft.x, botRight.y - topLeft.y ) );

        // Update the fields if we're in autoplace mode
        if( sheet->GetFieldsAutoplaced() == FIELDS_AUTOPLACED_AUTO )
            sheet->AutoplaceFields( /* aScreen */ NULL, /* aManual */ false );

        // Keep sheet pins attached to edges:
        for( SCH_SHEET_PIN* pin : sheet->GetPins() )
        {
            wxPoint pos = pin->GetPosition();

            pos += originDelta;

            switch( pin->GetEdge() )
            {
            case SHEET_SIDE::LEFT: pos.x = topLeft.x; break;
            case SHEET_SIDE::RIGHT: pos.x = topRight.x; break;
            case SHEET_SIDE::TOP: pos.y = topLeft.y; break;
            case SHEET_SIDE::BOTTOM: pos.y = botLeft.y; break;
            case SHEET_SIDE::UNDEFINED: break;
            }

            pin->SetPosition( pos );
        }

        break;
    }

    case SCH_LINE_T:
    {
        SCH_LINE* line = (SCH_LINE*) item;

        line->SetStartPoint( (wxPoint) m_editPoints->Point( LINE_START ).GetPosition() );
        line->SetEndPoint( (wxPoint) m_editPoints->Point( LINE_END ).GetPosition() );

        std::pair<EDA_ITEM*, int> connected = m_editPoints->Point( LINE_START ).GetConnected();

        if( connected.first )
        {
            if( connected.second == STARTPOINT )
                static_cast<SCH_LINE*>( connected.first )->SetStartPoint( line->GetPosition() );
            else if( connected.second == ENDPOINT )
                static_cast<SCH_LINE*>( connected.first )->SetEndPoint( line->GetPosition() );

            getView()->Update( connected.first, KIGFX::GEOMETRY );
        }

        connected = m_editPoints->Point( LINE_END ).GetConnected();

        if( connected.first )
        {
            if( connected.second == STARTPOINT )
                static_cast<SCH_LINE*>( connected.first )->SetStartPoint( line->GetEndPoint() );
            else if( connected.second == ENDPOINT )
                static_cast<SCH_LINE*>( connected.first )->SetEndPoint( line->GetEndPoint() );

            getView()->Update( connected.first, KIGFX::GEOMETRY );
        }

        break;
    }

    default:
        break;
    }

    updateItem( item, true );
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
        // point editor works only with rectangles having width and height > 0
        // Some symbols can have rectangles with width or height < 0
        // So normalize the size:
        BOX2I dummy;
        dummy.SetOrigin( mapCoords( rect->GetPosition() ) );
        dummy.SetEnd( mapCoords( rect->GetEnd() ) );
        dummy.Normalize();
        VECTOR2I topLeft = dummy.GetPosition();
        VECTOR2I botRight = dummy.GetEnd();

        m_editPoints->Point( RECT_TOPLEFT ).SetPosition( topLeft );
        m_editPoints->Point( RECT_TOPRIGHT ).SetPosition( VECTOR2I( botRight.x, topLeft.y ) );
        m_editPoints->Point( RECT_BOTLEFT ).SetPosition( VECTOR2I( topLeft.x, botRight.y ) );
        m_editPoints->Point( RECT_BOTRIGHT ).SetPosition( botRight );
        break;
    }

    case SCH_BITMAP_T:
    {
        SCH_BITMAP* bitmap = (SCH_BITMAP*) item;
        wxPoint        topLeft = bitmap->GetPosition() - bitmap->GetSize() / 2;
        wxPoint        botRight = bitmap->GetPosition() + bitmap->GetSize() / 2;

        m_editPoints->Point( RECT_TOPLEFT ).SetPosition( topLeft );
        m_editPoints->Point( RECT_TOPRIGHT ).SetPosition( botRight.x, topLeft.y );
        m_editPoints->Point( RECT_BOTLEFT ).SetPosition( topLeft.x, botRight.y );
        m_editPoints->Point( RECT_BOTRIGHT ).SetPosition( botRight );
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
        m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
        controls->ForceCursorPosition( true, aPoint->GetPosition() );
        controls->ShowCursor( true );
    }
    else
    {
        if( m_frame->ToolStackIsEmpty() )
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

    VECTOR2I cursorPos = getViewControls()->GetCursorPosition( !aEvent.DisableGridSnapping() );
    polyLine->AddCorner( mapCoords( cursorPos ) );

    updateItem( polyLine, true );
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

    updateItem( polyLine, true );
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
    if( m_isSymbolEditor )
    {
        saveCopyInUndoList( m_editPoints->GetParent()->GetParent(), UNDO_REDO::LIBEDIT );
    }
    else
    {
        saveCopyInUndoList( (SCH_ITEM*) m_editPoints->GetParent(), UNDO_REDO::CHANGED );

        if( m_editPoints->GetParent()->Type() == SCH_LINE_T )
        {
            std::pair<EDA_ITEM*, int> connected = m_editPoints->Point( LINE_START ).GetConnected();

            if( connected.first )
                saveCopyInUndoList( (SCH_ITEM*) connected.first, UNDO_REDO::CHANGED, true );

            connected = m_editPoints->Point( LINE_END ).GetConnected();

            if( connected.first )
                saveCopyInUndoList( (SCH_ITEM*) connected.first, UNDO_REDO::CHANGED, true );
        }
    }
}


void EE_POINT_EDITOR::rollbackFromUndo()
{
    if( m_isSymbolEditor )
        static_cast<SYMBOL_EDIT_FRAME*>( m_frame )->RollbackSymbolFromUndo();
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


