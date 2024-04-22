/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <ee_grid_helper.h>
#include <tool/tool_manager.h>
#include <sch_commit.h>
#include <view/view_controls.h>
#include <gal/graphics_abstraction_layer.h>
#include <geometry/seg.h>
#include <tools/ee_actions.h>
#include <tools/ee_selection_tool.h>
#include <sch_edit_frame.h>
#include <sch_line.h>
#include <sch_bitmap.h>
#include <sch_sheet.h>
#include <sch_textbox.h>
#include <sch_table.h>
#include <sch_tablecell.h>
#include <sch_shape.h>
#include <symbol_edit_frame.h>


// Few constants to avoid using bare numbers for point indices
enum ARC_POINTS
{
    ARC_START, ARC_END, ARC_CENTER
};


enum CIRCLE_POINTS
{
    CIRC_CENTER, CIRC_END
};


enum RECTANGLE_POINTS
{
    RECT_TOPLEFT, RECT_TOPRIGHT, RECT_BOTLEFT, RECT_BOTRIGHT
};


enum RECTANGLE_LINES
{
    RECT_TOP, RECT_RIGHT, RECT_BOT, RECT_LEFT
};

enum TABLECELL_POINTS
{
    COL_WIDTH, ROW_HEIGHT
};

enum LINE_POINTS
{
    LINE_START, LINE_END
};


enum BEZIER_POINTS
{
    BEZIER_START,
    BEZIER_CTRL_PT1,
    BEZIER_CTRL_PT2,
    BEZIER_END
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
        case SCH_SHAPE_T:
        {
            SCH_SHAPE* shape = static_cast<SCH_SHAPE*>( aItem );

            switch( shape->GetShape() )
            {
            case SHAPE_T::ARC:
                points->AddPoint( shape->GetPosition() );
                points->AddPoint( shape->GetStart() );
                points->AddPoint( shape->GetEnd() );
                break;

            case SHAPE_T::CIRCLE:
                points->AddPoint( shape->GetPosition() );
                points->AddPoint( shape->GetEnd() );
                break;

            case SHAPE_T::RECTANGLE:
            {
                shape->Normalize();

                VECTOR2I topLeft = shape->GetPosition();
                VECTOR2I botRight = shape->GetEnd();

                points->AddPoint( topLeft );
                points->AddPoint( VECTOR2I( botRight.x, topLeft.y ) );
                points->AddPoint( VECTOR2I( topLeft.x, botRight.y ) );
                points->AddPoint( botRight );

                points->AddLine( points->Point( RECT_TOPLEFT ), points->Point( RECT_TOPRIGHT ) );
                points->Line( RECT_TOP ).SetConstraint( new EC_PERPLINE( points->Line( RECT_TOP ) ) );
                points->AddLine( points->Point( RECT_TOPRIGHT ), points->Point( RECT_BOTRIGHT ) );
                points->Line( RECT_RIGHT ).SetConstraint( new EC_PERPLINE( points->Line( RECT_RIGHT ) ) );
                points->AddLine( points->Point( RECT_BOTRIGHT ), points->Point( RECT_BOTLEFT ) );
                points->Line( RECT_BOT ).SetConstraint( new EC_PERPLINE( points->Line( RECT_BOT ) ) );
                points->AddLine( points->Point( RECT_BOTLEFT ), points->Point( RECT_TOPLEFT ) );
                points->Line( RECT_LEFT ).SetConstraint( new EC_PERPLINE( points->Line( RECT_LEFT ) ) );

                break;
            }

            case SHAPE_T::POLY:
                for( const VECTOR2I& pt : shape->GetPolyShape().Outline( 0 ).CPoints() )
                    points->AddPoint( pt );

                break;

            case SHAPE_T::BEZIER:
                points->AddPoint( shape->GetStart() );
                points->AddPoint( shape->GetBezierC1() );
                points->AddPoint( shape->GetBezierC2() );
                points->AddPoint( shape->GetEnd() );
                break;

            default:
                UNIMPLEMENTED_FOR( shape->SHAPE_T_asString() );
            }

            break;
        }

        case SCH_TEXTBOX_T:
        {
            SCH_TEXTBOX* textbox = static_cast<SCH_TEXTBOX*>( aItem );

            textbox->Normalize();

            VECTOR2I topLeft = textbox->GetPosition();
            VECTOR2I botRight = textbox->GetEnd();

            points->AddPoint( topLeft );
            points->AddPoint( VECTOR2I( botRight.x, topLeft.y ) );
            points->AddPoint( VECTOR2I( topLeft.x, botRight.y ) );
            points->AddPoint( botRight );

            points->AddLine( points->Point( RECT_TOPLEFT ), points->Point( RECT_TOPRIGHT ) );
            points->Line( RECT_TOP ).SetConstraint( new EC_PERPLINE( points->Line( RECT_TOP ) ) );
            points->AddLine( points->Point( RECT_TOPRIGHT ), points->Point( RECT_BOTRIGHT ) );
            points->Line( RECT_RIGHT ).SetConstraint( new EC_PERPLINE( points->Line( RECT_RIGHT ) ) );
            points->AddLine( points->Point( RECT_BOTRIGHT ), points->Point( RECT_BOTLEFT ) );
            points->Line( RECT_BOT ).SetConstraint( new EC_PERPLINE( points->Line( RECT_BOT ) ) );
            points->AddLine( points->Point( RECT_BOTLEFT ), points->Point( RECT_TOPLEFT ) );
            points->Line( RECT_LEFT ).SetConstraint( new EC_PERPLINE( points->Line( RECT_LEFT ) ) );

            break;
        }

        case SCH_TABLECELL_T:
        {
            SCH_TABLECELL* cell = static_cast<SCH_TABLECELL*>( aItem );
            points->AddPoint( cell->GetEnd() - VECTOR2I( 0, cell->GetRectangleHeight() / 2 ) );
            points->AddPoint( cell->GetEnd() - VECTOR2I( cell->GetRectangleWidth() / 2, 0 ) );
            break;
        }

        case SCH_SHEET_T:
        {
            SCH_SHEET* sheet = (SCH_SHEET*) aItem;
            VECTOR2I   topLeft = sheet->GetPosition();
            VECTOR2I   botRight = sheet->GetPosition() + sheet->GetSize();

            points->AddPoint( topLeft );
            points->AddPoint( VECTOR2I( botRight.x, topLeft.y ) );
            points->AddPoint( VECTOR2I( topLeft.x, botRight.y ) );
            points->AddPoint( botRight );

            points->AddLine( points->Point( RECT_TOPLEFT ), points->Point( RECT_TOPRIGHT ) );
            points->Line( RECT_TOP ).SetConstraint( new EC_PERPLINE( points->Line( RECT_TOP ) ) );
            points->AddLine( points->Point( RECT_TOPRIGHT ), points->Point( RECT_BOTRIGHT ) );
            points->Line( RECT_RIGHT ).SetConstraint( new EC_PERPLINE( points->Line( RECT_RIGHT ) ) );
            points->AddLine( points->Point( RECT_BOTRIGHT ), points->Point( RECT_BOTLEFT ) );
            points->Line( RECT_BOT ).SetConstraint( new EC_PERPLINE( points->Line( RECT_BOT ) ) );
            points->AddLine( points->Point( RECT_BOTLEFT ), points->Point( RECT_TOPLEFT ) );
            points->Line( RECT_LEFT ).SetConstraint( new EC_PERPLINE( points->Line( RECT_LEFT ) ) );

            break;
        }

        case SCH_BITMAP_T:
        {
            SCH_BITMAP* bitmap = (SCH_BITMAP*) aItem;
            VECTOR2I    topLeft = bitmap->GetPosition() - bitmap->GetSize() / 2;
            VECTOR2I    botRight = bitmap->GetPosition() + bitmap->GetSize() / 2;

            points->AddPoint( topLeft );
            points->AddPoint( VECTOR2I( botRight.x, topLeft.y ) );
            points->AddPoint( VECTOR2I( topLeft.x, botRight.y ) );
            points->AddPoint( botRight );
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
    m_editedPoint( nullptr ),
    m_inPointEditor( false )
{
}


void EE_POINT_EDITOR::Reset( RESET_REASON aReason )
{
    EE_TOOL_BASE::Reset( aReason );

    m_editPoints.reset();
    m_editedPoint = nullptr;
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


int EE_POINT_EDITOR::clearEditedPoints( const TOOL_EVENT& aEvent )
{
    setEditedPoint( nullptr );

    return 0;
}


void EE_POINT_EDITOR::updateEditedPoint( const TOOL_EVENT& aEvent )
{
    EDIT_POINT* point = m_editedPoint;

    if( !m_editPoints )
    {
        point = nullptr;
    }
    else if( aEvent.IsMotion() )
    {
        point = m_editPoints->FindPoint( aEvent.Position(), getView() );
    }
    else if( aEvent.IsDrag( BUT_LEFT ) )
    {
        point = m_editPoints->FindPoint( aEvent.DragOrigin(), getView() );
    }
    else
    {
        point = m_editPoints->FindPoint( getViewControls()->GetCursorPosition( false ), getView() );
    }

    if( m_editedPoint != point )
        setEditedPoint( point );
}


int EE_POINT_EDITOR::Main( const TOOL_EVENT& aEvent )
{
    if( !m_selectionTool )
        return 0;

    if( m_inPointEditor )
        return 0;

    REENTRANCY_GUARD guard( &m_inPointEditor );

    if( m_isSymbolEditor )
    {
        SYMBOL_EDIT_FRAME* editor = getEditFrame<SYMBOL_EDIT_FRAME>();

        if( !editor->IsSymbolEditable() || editor->IsSymbolAlias() )
            return 0;
    }

    const EE_SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.Size() != 1 || !selection.Front()->IsType( { SCH_SHAPE_T,
                                                               SCH_TEXTBOX_T,
                                                               SCH_TABLECELL_T,
                                                               SCH_SHEET_T,
                                                               SCH_ITEM_LOCATE_GRAPHIC_LINE_T,
                                                               SCH_BITMAP_T } ) )
    {
        return 0;
    }

    // Wait till drawing tool is done
    if( selection.Front()->IsNew() )
        return 0;

    Activate();

    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    EE_GRID_HELPER*       grid = new EE_GRID_HELPER( m_toolMgr );
    VECTOR2I              cursorPos;
    KIGFX::VIEW*          view = getView();
    EDA_ITEM*             item = selection.Front();
    SCH_COMMIT            commit( m_toolMgr );

    controls->ShowCursor( true );

    m_editPoints = EDIT_POINTS_FACTORY::Make( item, m_frame );
    view->Add( m_editPoints.get() );
    setEditedPoint( nullptr );
    updateEditedPoint( aEvent );
    bool inDrag = false;

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        if( grid )
        {
            grid->SetSnap( !evt->Modifier( MD_SHIFT ) );
            grid->SetUseGrid( getView()->GetGAL()->GetGridSnapping()
                              && !evt->DisableGridSnapping() );
        }
        else
        {
            // This check is based on the assumption that the grid object must be valid.
            // If this assumption is wrong, please fix the code above.
            wxCHECK( false, 0 );
        }

        if( !m_editPoints || evt->IsSelectionEvent() )
            break;

        if ( !inDrag )
            updateEditedPoint( *evt );

        if( evt->IsDrag( BUT_LEFT ) && m_editedPoint )
        {
            if( !inDrag )
            {
                commit.Modify( m_editPoints->GetParent(), m_frame->GetScreen() );

                if( m_editPoints->GetParent()->Type() == SCH_LINE_T )
                {
                    std::pair<EDA_ITEM*, int> connected = m_editPoints->Point( LINE_START ).GetConnected();

                    if( connected.first )
                        commit.Modify( connected.first, m_frame->GetScreen() );

                    connected = m_editPoints->Point( LINE_END ).GetConnected();

                    if( connected.first )
                        commit.Modify( connected.first, m_frame->GetScreen() );
                }
                else if( m_editPoints->GetParent()->Type() == SCH_TABLECELL_T )
                {
                    SCH_TABLECELL* cell = static_cast<SCH_TABLECELL*>( m_editPoints->GetParent() );
                    SCH_TABLE*     table = static_cast<SCH_TABLE*>( cell->GetParent() );

                    commit.Modify( table, m_frame->GetScreen() );
                }

                inDrag = true;
            }

            bool snap = !evt->DisableGridSnapping();

            cursorPos =
                    grid->Align( controls->GetMousePosition(), GRID_HELPER_GRIDS::GRID_GRAPHICS );
            controls->ForceCursorPosition( true, cursorPos );

            m_editedPoint->SetPosition( controls->GetCursorPosition( snap ) );

            updateParentItem( snap );
            updatePoints();
        }
        else if( inDrag && evt->IsMouseUp( BUT_LEFT ) )
        {
            if( !commit.Empty() )
                commit.Push( _( "Move Point" ) );

            controls->SetAutoPan( false );
            inDrag = false;
        }
        else if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            if( inDrag )      // Restore the last change
            {
                // Currently we are manually managing the lifetime of the grid
                // helpers because there is a bug in the tool stack that adds
                // the point editor again when commit.Revert() rebuilds the selection.
                // We remove this grid here so the its destructor is called before it
                // is added again.
                if( grid )
                {
                    delete grid;
                    grid = nullptr;
                }

                commit.Revert();
                inDrag = false;
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
    setEditedPoint( nullptr );

    if( m_editPoints )
    {
        view->Remove( m_editPoints.get() );

        m_editPoints.reset();
        m_frame->GetCanvas()->Refresh();
    }

    delete grid;

    return 0;
}

/**
 * Update the coordinates of 4 corners of a rectangle, according to constraints
 * and the moved corner
 * @param minWidth is the minimal width constraint
 * @param minHeight is the minimal height constraint
 * @param topLeft is the RECT_TOPLEFT to constraint
 * @param topRight is the RECT_TOPRIGHT to constraint
 * @param botLeft is the RECT_BOTLEFT to constraint
 * @param botRight is the RECT_BOTRIGHT to constraint
 */
void EE_POINT_EDITOR::pinEditedCorner( int minWidth, int minHeight, VECTOR2I& topLeft,
                                       VECTOR2I& topRight, VECTOR2I& botLeft, VECTOR2I& botRight,
                                       EE_GRID_HELPER* aGrid ) const
{
    if( isModified( m_editPoints->Point(  RECT_TOPLEFT ) ) )
    {
        // pin edited point within opposite corner
        topLeft.x = std::min( topLeft.x, botRight.x - minWidth );
        topLeft.y = std::min( topLeft.y, botRight.y - minHeight );

        if( aGrid->GetSnap() )
            topLeft = aGrid->AlignGrid( topLeft, GRID_HELPER_GRIDS::GRID_GRAPHICS );

        // push edited point edges to adjacent corners
        topRight.y = topLeft.y;
        botLeft.x = topLeft.x;
    }
    else if( isModified( m_editPoints->Point( RECT_TOPRIGHT ) ) )
    {
        // pin edited point within opposite corner
        topRight.x = std::max( topRight.x, botLeft.x + minWidth );
        topRight.y = std::min( topRight.y, botLeft.y - minHeight );

        if( aGrid->GetSnap() )
            topRight = aGrid->AlignGrid( topRight, GRID_HELPER_GRIDS::GRID_GRAPHICS );

        // push edited point edges to adjacent corners
        topLeft.y = topRight.y;
        botRight.x = topRight.x;
    }
    else if( isModified( m_editPoints->Point( RECT_BOTLEFT ) ) )
    {
        // pin edited point within opposite corner
        botLeft.x = std::min( botLeft.x, topRight.x - minWidth );
        botLeft.y = std::max( botLeft.y, topRight.y + minHeight );

        if( aGrid->GetSnap() )
            botLeft = aGrid->AlignGrid( botLeft, GRID_HELPER_GRIDS::GRID_GRAPHICS );

        // push edited point edges to adjacent corners
        botRight.y = botLeft.y;
        topLeft.x = botLeft.x;
    }
    else if( isModified( m_editPoints->Point( RECT_BOTRIGHT ) ) )
    {
        // pin edited point within opposite corner
        botRight.x = std::max( botRight.x, topLeft.x + minWidth );
        botRight.y = std::max( botRight.y, topLeft.y + minHeight );

        if( aGrid->GetSnap() )
            botRight = aGrid->AlignGrid( botRight, GRID_HELPER_GRIDS::GRID_GRAPHICS );

        // push edited point edges to adjacent corners
        botLeft.y = botRight.y;
        topRight.x = botRight.x;
    }
    else if( isModified( m_editPoints->Line( RECT_TOP ) ) )
    {
        topLeft.y = std::min( topLeft.y, botRight.y - minHeight );

        if( aGrid->GetSnap() )
            topLeft = aGrid->AlignGrid( topLeft, GRID_HELPER_GRIDS::GRID_GRAPHICS );
    }
    else if( isModified( m_editPoints->Line( RECT_LEFT ) ) )
    {
        topLeft.x = std::min( topLeft.x, botRight.x - minWidth );

        if( aGrid->GetSnap() )
            topLeft = aGrid->AlignGrid( topLeft, GRID_HELPER_GRIDS::GRID_GRAPHICS );
    }
    else if( isModified( m_editPoints->Line( RECT_BOT ) ) )
    {
        botRight.y = std::max( botRight.y, topLeft.y + minHeight );

        if( aGrid->GetSnap() )
            botRight = aGrid->AlignGrid( botRight, GRID_HELPER_GRIDS::GRID_GRAPHICS );
    }
    else if( isModified( m_editPoints->Line( RECT_RIGHT ) ) )
    {
        botRight.x = std::max( botRight.x, topLeft.x + minWidth );

        if( aGrid->GetSnap() )
            botRight = aGrid->AlignGrid( botRight, GRID_HELPER_GRIDS::GRID_GRAPHICS );
    }
}


void EE_POINT_EDITOR::updateParentItem( bool aSnapToGrid ) const
{
    EDA_ITEM* item = m_editPoints->GetParent();

    if( !item )
        return;

    switch( item->Type() )
    {
    case SCH_SHAPE_T:
    {
        SCH_SHAPE* shape = static_cast<SCH_SHAPE*>( item );

        switch( shape->GetShape() )
        {
        case SHAPE_T::ARC:
            if( getEditedPointIndex() == ARC_CENTER )
            {
                shape->SetEditState( 4 );
                shape->CalcEdit( m_editPoints->Point( ARC_CENTER ).GetPosition() );
            }
            else if( getEditedPointIndex() == ARC_START )
            {
                shape->SetEditState( 2 );
                shape->CalcEdit( m_editPoints->Point( ARC_START ).GetPosition() );
            }
            else if( getEditedPointIndex() == ARC_END )
            {
                shape->SetEditState( 3 );
                shape->CalcEdit( m_editPoints->Point( ARC_END ).GetPosition() );
            }
            break;

        case SHAPE_T::CIRCLE:
            shape->SetPosition( m_editPoints->Point( CIRC_CENTER ).GetPosition() );
            shape->SetEnd( m_editPoints->Point( CIRC_END ).GetPosition() );
            break;

        case SHAPE_T::POLY:
            shape->GetPolyShape().RemoveAllContours();
            shape->GetPolyShape().NewOutline();

            for( unsigned i = 0; i < m_editPoints->PointsSize(); ++i )
            {
                VECTOR2I pt = m_editPoints->Point( i ).GetPosition();
                shape->GetPolyShape().Append( pt.x, pt.y, -1, -1, true );
            }

            break;

        case SHAPE_T::RECTANGLE:
        {
            EE_GRID_HELPER gridHelper( m_toolMgr );
            VECTOR2I       topLeft = m_editPoints->Point( RECT_TOPLEFT ).GetPosition();
            VECTOR2I       topRight = m_editPoints->Point( RECT_TOPRIGHT ).GetPosition();
            VECTOR2I       botLeft = m_editPoints->Point( RECT_BOTLEFT ).GetPosition();
            VECTOR2I       botRight = m_editPoints->Point( RECT_BOTRIGHT ).GetPosition();

            gridHelper.SetSnap( aSnapToGrid );

            pinEditedCorner( schIUScale.MilsToIU( 1 ), schIUScale.MilsToIU( 1 ), topLeft, topRight,
                             botLeft, botRight, &gridHelper );

            if( isModified( m_editPoints->Point( RECT_TOPLEFT ) )
                    || isModified( m_editPoints->Point( RECT_TOPRIGHT ) )
                    || isModified( m_editPoints->Point( RECT_BOTRIGHT ) )
                    || isModified( m_editPoints->Point( RECT_BOTLEFT ) ) )
            {
                shape->SetPosition( topLeft );
                shape->SetEnd( botRight );
            }
            else if( isModified( m_editPoints->Line( RECT_TOP ) ) )
            {
                shape->SetStartY( topLeft.y );
            }
            else if( isModified( m_editPoints->Line( RECT_LEFT ) ) )
            {
                shape->SetStartX( topLeft.x );
            }
            else if( isModified( m_editPoints->Line( RECT_BOT ) ) )
            {
                shape->SetEndY( botRight.y );
            }
            else if( isModified( m_editPoints->Line( RECT_RIGHT ) ) )
            {
                shape->SetEndX( botRight.x );
            }

            for( unsigned i = 0; i < m_editPoints->LinesSize(); ++i )
            {
                if( !isModified( m_editPoints->Line( i ) ) )
                {
                    m_editPoints->Line( i ).SetConstraint(
                            new EC_PERPLINE( m_editPoints->Line( i ) ) );
                }
            }

            break;
        }

        case SHAPE_T::BEZIER:
            shape->SetStart( m_editPoints->Point( BEZIER_START ).GetPosition() );
            shape->SetBezierC1( m_editPoints->Point( BEZIER_CTRL_PT1 ).GetPosition() );
            shape->SetBezierC2( m_editPoints->Point( BEZIER_CTRL_PT2 ).GetPosition() );
            shape->SetEnd( m_editPoints->Point( BEZIER_END ).GetPosition() );

            shape->RebuildBezierToSegmentsPointsList( shape->GetWidth() );
            break;

        default:
            UNIMPLEMENTED_FOR( shape->SHAPE_T_asString() );
        }

        break;
    }

    case SCH_TEXTBOX_T:
    {
        SCH_TEXTBOX*   textbox = static_cast<SCH_TEXTBOX*>( item );
        EE_GRID_HELPER gridHelper( m_toolMgr );
        VECTOR2I       topLeft = m_editPoints->Point( RECT_TOPLEFT ).GetPosition();
        VECTOR2I       topRight = m_editPoints->Point( RECT_TOPRIGHT ).GetPosition();
        VECTOR2I       botLeft = m_editPoints->Point( RECT_BOTLEFT ).GetPosition();
        VECTOR2I       botRight = m_editPoints->Point( RECT_BOTRIGHT ).GetPosition();

        gridHelper.SetSnap( aSnapToGrid );

        pinEditedCorner( schIUScale.MilsToIU( 1 ), schIUScale.MilsToIU( 1 ), topLeft, topRight,
                         botLeft, botRight, &gridHelper );

        if( isModified( m_editPoints->Point( RECT_TOPLEFT ) )
                || isModified( m_editPoints->Point( RECT_TOPRIGHT ) )
                || isModified( m_editPoints->Point( RECT_BOTRIGHT ) )
                || isModified( m_editPoints->Point( RECT_BOTLEFT ) ) )
        {
            textbox->SetPosition( topLeft );
            textbox->SetEnd( botRight );
        }
        else if( isModified( m_editPoints->Line( RECT_TOP ) ) )
        {
            textbox->SetStartY( topLeft.y );
        }
        else if( isModified( m_editPoints->Line( RECT_LEFT ) ) )
        {
            textbox->SetStartX( topLeft.x );
        }
        else if( isModified( m_editPoints->Line( RECT_BOT ) ) )
        {
            textbox->SetEndY( botRight.y );
        }
        else if( isModified( m_editPoints->Line( RECT_RIGHT ) ) )
        {
            textbox->SetEndX( botRight.x );
        }

        for( unsigned i = 0; i < m_editPoints->LinesSize(); ++i )
        {
            if( !isModified( m_editPoints->Line( i ) ) )
            {
                m_editPoints->Line( i ).SetConstraint(
                        new EC_PERPLINE( m_editPoints->Line( i ) ) );
            }
        }

        textbox->ClearRenderCache();
        break;
    }

    case SCH_TABLECELL_T:
    {
        SCH_TABLECELL* cell = static_cast<SCH_TABLECELL*>( item );
        SCH_TABLE*     table = static_cast<SCH_TABLE*>( cell->GetParent() );

        if( isModified( m_editPoints->Point( COL_WIDTH ) ) )
        {
            cell->SetEnd( VECTOR2I( m_editPoints->Point( 0 ).GetX(), cell->GetEndY() ) );

            int colWidth = cell->GetRectangleWidth();

            for( int ii = 0; ii < cell->GetColSpan() - 1; ++ii )
                colWidth -= table->GetColWidth( cell->GetColumn() + ii );

            table->SetColWidth( cell->GetColumn() + cell->GetColSpan() - 1, colWidth );
            table->Normalize();
        }
        else if( isModified( m_editPoints->Point( ROW_HEIGHT ) ) )
        {
            cell->SetEnd( VECTOR2I( cell->GetEndX(), m_editPoints->Point( 1 ).GetY() ) );

            int rowHeight = cell->GetRectangleHeight();

            for( int ii = 0; ii < cell->GetRowSpan() - 1; ++ii )
                rowHeight -= table->GetRowHeight( cell->GetRow() + ii );

            table->SetRowHeight( cell->GetRow() + cell->GetRowSpan() - 1, rowHeight );
            table->Normalize();
        }

        break;
    }

    case SCH_BITMAP_T:
    {
        EE_GRID_HELPER gridHelper( m_toolMgr );
        SCH_BITMAP*    bitmap = (SCH_BITMAP*) item;
        VECTOR2I       topLeft = m_editPoints->Point( RECT_TOPLEFT ).GetPosition();
        VECTOR2I       topRight = m_editPoints->Point( RECT_TOPRIGHT ).GetPosition();
        VECTOR2I       botLeft = m_editPoints->Point( RECT_BOTLEFT ).GetPosition();
        VECTOR2I       botRight = m_editPoints->Point( RECT_BOTRIGHT ).GetPosition();

        gridHelper.SetSnap( aSnapToGrid );

        pinEditedCorner( schIUScale.MilsToIU( 50 ), schIUScale.MilsToIU( 50 ), topLeft, topRight,
                         botLeft, botRight, &gridHelper );

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
        SCH_SHEET*     sheet = (SCH_SHEET*) item;
        EE_GRID_HELPER gridHelper( m_toolMgr );
        VECTOR2I       topLeft = m_editPoints->Point( RECT_TOPLEFT ).GetPosition();
        VECTOR2I       topRight = m_editPoints->Point( RECT_TOPRIGHT ).GetPosition();
        VECTOR2I       botLeft = m_editPoints->Point( RECT_BOTLEFT ).GetPosition();
        VECTOR2I       botRight = m_editPoints->Point( RECT_BOTRIGHT ).GetPosition();
        VECTOR2I       sheetNewPos = sheet->GetPosition();
        VECTOR2I       sheetNewSize = sheet->GetSize();

        gridHelper.SetSnap( aSnapToGrid );

        int edited = getEditedPointIndex();

        if( isModified( m_editPoints->Line( RECT_RIGHT ) ) )
            edited = RECT_TOPRIGHT;
        else if( isModified( m_editPoints->Line( RECT_BOT ) ) )
            edited = RECT_BOTLEFT;

        gridHelper.SetSnap( aSnapToGrid );

        pinEditedCorner( sheet->GetMinWidth( edited == RECT_TOPRIGHT || edited == RECT_BOTRIGHT ),
                         sheet->GetMinHeight( edited == RECT_BOTLEFT || edited == RECT_BOTRIGHT ),
                         topLeft, topRight, botLeft, botRight, &gridHelper );

        if( isModified( m_editPoints->Point( RECT_TOPLEFT ) )
                || isModified( m_editPoints->Point( RECT_TOPRIGHT ) )
                || isModified( m_editPoints->Point( RECT_BOTRIGHT ) )
                || isModified( m_editPoints->Point( RECT_BOTLEFT ) ) )
        {
            sheetNewPos = topLeft;
            sheetNewSize = VECTOR2I( botRight.x - topLeft.x, botRight.y - topLeft.y );
        }
        else if( isModified( m_editPoints->Line( RECT_TOP ) ) )
        {
            sheetNewPos = VECTOR2I( sheet->GetPosition().x, topLeft.y );
            sheetNewSize = VECTOR2I( sheet->GetSize().x, botRight.y - topLeft.y );
        }
        else if( isModified( m_editPoints->Line( RECT_LEFT ) ) )
        {
            sheetNewPos = VECTOR2I( topLeft.x, sheet->GetPosition().y );
            sheetNewSize = VECTOR2I( botRight.x - topLeft.x, sheet->GetSize().y );
        }
        else if( isModified( m_editPoints->Line( RECT_BOT ) ) )
        {
            sheetNewSize = VECTOR2I( sheet->GetSize().x, botRight.y - topLeft.y );
        }
        else if( isModified( m_editPoints->Line( RECT_RIGHT ) ) )
        {
            sheetNewSize = VECTOR2I( botRight.x - topLeft.x, sheet->GetSize().y );
        }

        for( unsigned i = 0; i < m_editPoints->LinesSize(); ++i )
        {
            if( !isModified( m_editPoints->Line( i ) ) )
            {
                m_editPoints->Line( i ).SetConstraint(
                        new EC_PERPLINE( m_editPoints->Line( i ) ) );
            }
        }

        if( sheet->GetPosition() != sheetNewPos )
            sheet->SetPositionIgnoringPins( sheetNewPos );

        if( sheet->GetSize() != sheetNewSize )
            sheet->Resize( sheetNewSize );

        break;
    }

    case SCH_LINE_T:
    {
        SCH_LINE* line = (SCH_LINE*) item;

        line->SetStartPoint( m_editPoints->Point( LINE_START ).GetPosition() );
        line->SetEndPoint( m_editPoints->Point( LINE_END ).GetPosition() );

        std::pair<EDA_ITEM*, int> connected = m_editPoints->Point( LINE_START ).GetConnected();

        if( connected.first )
        {
            if( connected.second == STARTPOINT )
                static_cast<SCH_LINE*>( connected.first )->SetStartPoint( line->GetStartPoint() );
            else if( connected.second == ENDPOINT )
                static_cast<SCH_LINE*>( connected.first )->SetEndPoint( line->GetStartPoint() );

            updateItem( connected.first, true );
        }

        connected = m_editPoints->Point( LINE_END ).GetConnected();

        if( connected.first )
        {
            if( connected.second == STARTPOINT )
                static_cast<SCH_LINE*>( connected.first )->SetStartPoint( line->GetEndPoint() );
            else if( connected.second == ENDPOINT )
                static_cast<SCH_LINE*>( connected.first )->SetEndPoint( line->GetEndPoint() );

            updateItem( connected.first, true );
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
    case SCH_SHAPE_T:
    {
        SCH_SHAPE* shape = static_cast<SCH_SHAPE*>( item );

        switch( shape->GetShape() )
        {
        case SHAPE_T::ARC:
            m_editPoints->Point( ARC_CENTER ).SetPosition( shape->GetPosition() );
            m_editPoints->Point( ARC_START ).SetPosition( shape->GetStart() );
            m_editPoints->Point( ARC_END ).SetPosition( shape->GetEnd() );
            break;

        case SHAPE_T::CIRCLE:
            m_editPoints->Point( CIRC_CENTER ).SetPosition( shape->GetPosition() );
            m_editPoints->Point( CIRC_END ).SetPosition( shape->GetEnd() );
            break;

        case SHAPE_T::POLY:
        {
            if( (int) m_editPoints->PointsSize() != shape->GetPointCount() )
            {
                getView()->Remove( m_editPoints.get() );
                m_editedPoint = nullptr;
                m_editPoints = EDIT_POINTS_FACTORY::Make( item, m_frame );
                getView()->Add( m_editPoints.get() );
            }
            else
            {
                int ii = 0;

                for( const VECTOR2I& pt : shape->GetPolyShape().Outline( 0 ).CPoints() )
                    m_editPoints->Point( ii++ ).SetPosition( pt );
            }

            break;
        }

        case SHAPE_T::RECTANGLE:
        {
            // point editor works only with rectangles having width and height > 0
            // Some symbols can have rectangles with width or height < 0
            // So normalize the size:
            BOX2I dummy;
            dummy.SetOrigin( shape->GetPosition() );
            dummy.SetEnd( shape->GetEnd() );
            dummy.Normalize();
            VECTOR2I topLeft = dummy.GetPosition();
            VECTOR2I botRight = dummy.GetEnd();

            m_editPoints->Point( RECT_TOPLEFT ).SetPosition( topLeft );
            m_editPoints->Point( RECT_TOPRIGHT ).SetPosition( VECTOR2I( botRight.x, topLeft.y ) );
            m_editPoints->Point( RECT_BOTLEFT ).SetPosition( VECTOR2I( topLeft.x, botRight.y ) );
            m_editPoints->Point( RECT_BOTRIGHT ).SetPosition( botRight );
            break;
        }

        case SHAPE_T::BEZIER:
            m_editPoints->Point( BEZIER_START ).SetPosition( shape->GetStart() );
            m_editPoints->Point( BEZIER_CTRL_PT1 ).SetPosition( shape->GetBezierC1() );
            m_editPoints->Point( BEZIER_CTRL_PT2 ).SetPosition( shape->GetBezierC2() );
            m_editPoints->Point( BEZIER_END ).SetPosition( shape->GetEnd() );
            break;

        default:
            UNIMPLEMENTED_FOR( shape->SHAPE_T_asString() );
        }

        break;
    }

    case SCH_TEXTBOX_T:
    {
        SCH_TEXTBOX* textbox = static_cast<SCH_TEXTBOX*>( item );

        // point editor works only with rectangles having width and height > 0
        // Some symbols can have rectangles with width or height < 0
        // So normalize the size:
        BOX2I dummy;
        dummy.SetOrigin( textbox->GetPosition() );
        dummy.SetEnd( textbox->GetEnd() );
        dummy.Normalize();
        VECTOR2I topLeft = dummy.GetPosition();
        VECTOR2I botRight = dummy.GetEnd();

        m_editPoints->Point( RECT_TOPLEFT ).SetPosition( topLeft );
        m_editPoints->Point( RECT_TOPRIGHT ).SetPosition( VECTOR2I( botRight.x, topLeft.y ) );
        m_editPoints->Point( RECT_BOTLEFT ).SetPosition( VECTOR2I( topLeft.x, botRight.y ) );
        m_editPoints->Point( RECT_BOTRIGHT ).SetPosition( botRight );
        break;
    }

    case SCH_TABLECELL_T:
    {
        SCH_TABLECELL* cell = static_cast<SCH_TABLECELL*>( item );

        m_editPoints->Point( 0 ).SetPosition( cell->GetEndX(),
                                              cell->GetEndY() - cell->GetRectangleHeight() / 2 );
        m_editPoints->Point( 1 ).SetPosition( cell->GetEndX() - cell->GetRectangleWidth() / 2,
                                              cell->GetEndY() );
        break;
    }

    case SCH_BITMAP_T:
    {
        SCH_BITMAP* bitmap = (SCH_BITMAP*) item;
        VECTOR2I    topLeft = bitmap->GetPosition() - bitmap->GetSize() / 2;
        VECTOR2I    botRight = bitmap->GetPosition() + bitmap->GetSize() / 2;

        m_editPoints->Point( RECT_TOPLEFT ).SetPosition( topLeft );
        m_editPoints->Point( RECT_TOPRIGHT ).SetPosition( botRight.x, topLeft.y );
        m_editPoints->Point( RECT_BOTLEFT ).SetPosition( topLeft.x, botRight.y );
        m_editPoints->Point( RECT_BOTRIGHT ).SetPosition( botRight );
        break;
    }

    case SCH_SHEET_T:
    {
        SCH_SHEET* sheet = (SCH_SHEET*) item;
        VECTOR2I   topLeft = sheet->GetPosition();
        VECTOR2I   botRight = sheet->GetPosition() + sheet->GetSize();

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
    if( !m_editPoints || !m_editedPoint || m_editPoints->GetParent()->Type() != SCH_SHAPE_T )
        return false;

    SCH_SHAPE* shape = static_cast<SCH_SHAPE*>( m_editPoints->GetParent() );

    if( shape->GetPolyShape().IsEmpty() )
        return false;

    SHAPE_LINE_CHAIN& poly = shape->GetPolyShape().Outline( 0 );

    if( poly.GetPointCount() < 3 )
        return false;

    for( const VECTOR2I& pt : poly.CPoints() )
    {
        if( pt == m_editedPoint->GetPosition() )
            return true;
    }

    return false;
}


bool EE_POINT_EDITOR::addCornerCondition( const SELECTION& )
{
    if( !m_editPoints || m_editPoints->GetParent()->Type() != SCH_SHAPE_T )
        return false;

    SCH_SHAPE* shape = static_cast<SCH_SHAPE*>( m_editPoints->GetParent() );

    if( shape->GetShape() != SHAPE_T::POLY )
        return false;

    VECTOR2I cursorPos = getViewControls()->GetCursorPosition( false );
    double   threshold = getView()->ToWorld( EDIT_POINT::POINT_SIZE );

    return shape->HitTest( cursorPos, (int) threshold );
}


int EE_POINT_EDITOR::addCorner( const TOOL_EVENT& aEvent )
{
    if( !m_editPoints || m_editPoints->GetParent()->Type() != SCH_SHAPE_T )
        return 0;

    SCH_SHAPE*        shape = static_cast<SCH_SHAPE*>( m_editPoints->GetParent() );
    SHAPE_LINE_CHAIN& poly = shape->GetPolyShape().Outline( 0 );
    SCH_COMMIT        commit( m_toolMgr );

    commit.Modify( shape, m_frame->GetScreen() );

    VECTOR2I cursor = getViewControls()->GetCursorPosition( !aEvent.DisableGridSnapping() );
    int      currentMinDistance = INT_MAX;
    int      closestLineStart = 0;

    for( unsigned i = 0; i < poly.GetPointCount() - 1; ++i )
    {
        int distance = (int) DistanceLinePoint( poly.CPoint( i ),
                                                poly.CPoint( i + 1 ), cursor );

        if( distance < currentMinDistance )
        {
            currentMinDistance = distance;
            closestLineStart = i;
        }
    }

    poly.Insert( closestLineStart + 1, cursor );

    updateItem( shape, true );
    updatePoints();

    commit.Push( _( "Add Corner" ) );
    return 0;
}


int EE_POINT_EDITOR::removeCorner( const TOOL_EVENT& aEvent )
{
    if( !m_editPoints || !m_editedPoint || m_editPoints->GetParent()->Type() != SCH_SHAPE_T )
        return 0;

    SCH_SHAPE*        shape = static_cast<SCH_SHAPE*>( m_editPoints->GetParent() );
    SHAPE_LINE_CHAIN& poly = shape->GetPolyShape().Outline( 0 );
    SCH_COMMIT        commit( m_toolMgr );

    commit.Modify( shape, m_frame->GetScreen() );

    if( poly.GetPointCount() < 3 )
        return 0;

    poly.Remove( getEditedPointIndex() );

    updateItem( shape, true );
    updatePoints();

    commit.Push( _( "Remove Corner" ) );
    return 0;
}


int EE_POINT_EDITOR::modifiedSelection( const TOOL_EVENT& aEvent )
{
    updatePoints();
    return 0;
}


void EE_POINT_EDITOR::setTransitions()
{
    Go( &EE_POINT_EDITOR::Main,              EVENTS::PointSelectedEvent );
    Go( &EE_POINT_EDITOR::Main,              EVENTS::SelectedEvent );
    Go( &EE_POINT_EDITOR::Main,              ACTIONS::activatePointEditor.MakeEvent() );
    Go( &EE_POINT_EDITOR::addCorner,         EE_ACTIONS::pointEditorAddCorner.MakeEvent() );
    Go( &EE_POINT_EDITOR::removeCorner,      EE_ACTIONS::pointEditorRemoveCorner.MakeEvent() );
    Go( &EE_POINT_EDITOR::modifiedSelection, EVENTS::SelectedItemsModified );
    Go( &EE_POINT_EDITOR::clearEditedPoints, EVENTS::ClearedEvent );
}


