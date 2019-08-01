/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2017 CERN
 * Copyright (C) 2018-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "drawing_tool.h"
#include "pcb_actions.h"

#include <pcb_edit_frame.h>
#include <project.h>
#include <id.h>
#include <confirm.h>
#include <import_gfx/dialog_import_gfx.h>
#include <view/view_controls.h>
#include <view/view.h>
#include <gal/graphics_abstraction_layer.h>
#include <tool/tool_manager.h>
#include <geometry/geometry_utils.h>
#include <ratsnest_data.h>
#include <board_commit.h>
#include <scoped_set_reset.h>
#include <bitmaps.h>
#include <painter.h>
#include <status_popup.h>
#include "grid_helper.h"
#include "point_editor.h"
#include <dialogs/dialog_text_properties.h>
#include <preview_items/arc_assistant.h>

#include <class_board.h>
#include <class_edge_mod.h>
#include <class_pcb_text.h>
#include <class_dimension.h>
#include <class_zone.h>
#include <class_module.h>

#include <tools/selection_tool.h>
#include <tools/tool_event_utils.h>
#include <tools/zone_create_helper.h>

using SCOPED_DRAW_MODE = SCOPED_SET_RESET<DRAWING_TOOL::MODE>;


DRAWING_TOOL::DRAWING_TOOL() :
    PCB_TOOL_BASE( "pcbnew.InteractiveDrawing" ),
    m_view( nullptr ), m_controls( nullptr ),
    m_board( nullptr ), m_frame( nullptr ), m_mode( MODE::NONE ),
    m_lineWidth( 1 )
{
}


DRAWING_TOOL::~DRAWING_TOOL()
{
}


bool DRAWING_TOOL::Init()
{
    auto activeToolFunctor = [ this ] ( const SELECTION& aSel ) {
                                 return m_mode != MODE::NONE;
                             };

    // some interactive drawing tools can undo the last point
    auto canUndoPoint = [ this ] ( const SELECTION& aSel ) {
                            return m_mode == MODE::ARC || m_mode == MODE::ZONE;
                        };

    // functor for zone-only actions
    auto zoneActiveFunctor = [this ] ( const SELECTION& aSel ) {
                                 return m_mode == MODE::ZONE;
                             };

    auto& ctxMenu = m_menu.GetMenu();

    // cancel current tool goes in main context menu at the top if present
    ctxMenu.AddItem( ACTIONS::cancelInteractive, activeToolFunctor, 1 );
    ctxMenu.AddSeparator( 1 );

    // tool-specific actions
    ctxMenu.AddItem( PCB_ACTIONS::closeZoneOutline, zoneActiveFunctor, 200 );
    ctxMenu.AddItem( PCB_ACTIONS::deleteLastPoint, canUndoPoint, 200 );

    ctxMenu.AddSeparator( 500 );

    // Type-specific sub-menus will be added for us by other tools
    // For example, zone fill/unfill is provided by the PCB control tool

    // Finally, add the standard zoom/grid items
    getEditFrame<PCB_BASE_FRAME>()->AddStandardSubMenus( m_menu );

    return true;
}


void DRAWING_TOOL::Reset( RESET_REASON aReason )
{
    // Init variables used by every drawing tool
    m_view = getView();
    m_controls = getViewControls();
    m_board = getModel<BOARD>();
    m_frame = getEditFrame<PCB_BASE_EDIT_FRAME>();
}


DRAWING_TOOL::MODE DRAWING_TOOL::GetDrawingMode() const
{
    return m_mode;
}


int DRAWING_TOOL::DrawLine( const TOOL_EVENT& aEvent )
{
    if( m_editModules && !m_frame->GetModel() )
        return 0;

    MODULE*          module = dynamic_cast<MODULE*>( m_frame->GetModel() );
    DRAWSEGMENT*     line = m_editModules ? new EDGE_MODULE( module ) : new DRAWSEGMENT;
    BOARD_COMMIT     commit( m_frame );
    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::LINE );
    OPT<VECTOR2D>    startingPoint = boost::make_optional<VECTOR2D>( false, VECTOR2D( 0, 0 ) );

    line->SetFlags( IS_NEW );

    if( aEvent.HasPosition() )
        startingPoint = getViewControls()->GetCursorPosition( !aEvent.Modifier( MD_ALT ) );

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();

    while( drawSegment( tool, S_SEGMENT, line, startingPoint ) )
    {
        if( line )
        {
            if( m_editModules )
                static_cast<EDGE_MODULE*>( line )->SetLocalCoord();

            commit.Add( line );
            commit.Push( _( "Draw a line segment" ) );
            startingPoint = VECTOR2D( line->GetEnd() );
        }
        else
        {
            startingPoint = NULLOPT;
        }

        line = m_editModules ? new EDGE_MODULE( module ) : new DRAWSEGMENT;
        line->SetFlags( IS_NEW );
    }

    return 0;
}


int DRAWING_TOOL::DrawCircle( const TOOL_EVENT& aEvent )
{
    if( m_editModules && !m_frame->GetModel() )
        return 0;

    MODULE*          module = dynamic_cast<MODULE*>( m_frame->GetModel() );
    DRAWSEGMENT*     circle = m_editModules ? new EDGE_MODULE( module ) : new DRAWSEGMENT;
    BOARD_COMMIT     commit( m_frame );
    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::CIRCLE );
    OPT<VECTOR2D>    startingPoint = boost::make_optional<VECTOR2D>( false, VECTOR2D( 0, 0 ) );

    circle->SetFlags( IS_NEW );

    if( aEvent.HasPosition() )
        startingPoint = getViewControls()->GetCursorPosition( !aEvent.Modifier( MD_ALT ) );

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();

    while( drawSegment( tool, S_CIRCLE, circle, startingPoint ) )
    {
        if( circle )
        {
            if( m_editModules )
                static_cast<EDGE_MODULE*>( circle )->SetLocalCoord();

            commit.Add( circle );
            commit.Push( _( "Draw a circle" ) );

            m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, circle );
        }

        circle = m_editModules ? new EDGE_MODULE( module ) : new DRAWSEGMENT;
        circle->SetFlags( IS_NEW );
        startingPoint = NULLOPT;
    }

    return 0;
}


int DRAWING_TOOL::DrawArc( const TOOL_EVENT& aEvent )
{
    if( m_editModules && !m_frame->GetModel() )
        return 0;

    MODULE*          module = dynamic_cast<MODULE*>( m_frame->GetModel() );
    DRAWSEGMENT*     arc = m_editModules ? new EDGE_MODULE( module ) : new DRAWSEGMENT;
    BOARD_COMMIT     commit( m_frame );
    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::ARC );
    bool             immediateMode = aEvent.HasPosition();

    arc->SetFlags( IS_NEW );

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();

    while( drawArc( tool, arc, immediateMode ) )
    {
        if( arc )
        {
            if( m_editModules )
                static_cast<EDGE_MODULE*>( arc )->SetLocalCoord();

            commit.Add( arc );
            commit.Push( _( "Draw an arc" ) );

            m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, arc );
        }

        arc = m_editModules ? new EDGE_MODULE( module ) : new DRAWSEGMENT;
        arc->SetFlags( IS_NEW );
        immediateMode = false;
    }

    return 0;
}


int DRAWING_TOOL::PlaceText( const TOOL_EVENT& aEvent )
{
    if( m_editModules && !m_frame->GetModel() )
        return 0;

    BOARD_ITEM* text = NULL;
    const BOARD_DESIGN_SETTINGS& dsnSettings = m_frame->GetDesignSettings();
    BOARD_COMMIT commit( m_frame );

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );
    // do not capture or auto-pan until we start placing some text

    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::TEXT );

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();

    bool reselect = false;

    // Prime the pump
    if( aEvent.HasPosition() )
        m_toolMgr->RunAction( ACTIONS::cursorClick );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        m_frame->GetCanvas()->SetCurrentCursor( text ? wxCURSOR_ARROW : wxCURSOR_PENCIL );
        VECTOR2I cursorPos = m_controls->GetCursorPosition();

        if( reselect && text )
            m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, text );

        auto cleanup = [&] () {
            m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
            m_controls->ForceCursorPosition( false );
            m_controls->ShowCursor( true );
            m_controls->SetAutoPan( false );
            m_controls->CaptureCursor( false );
            delete text;
            text = NULL;
        };

        if( evt->IsCancelInteractive() )
        {
            if( text )
                cleanup();
            else
            {
                m_frame->PopTool( tool );
                break;
            }
        }
        else if( evt->IsActivate() )
        {
            if( text )
                cleanup();

            if( evt->IsMoveTool() )
            {
                // leave ourselves on the stack so we come back after the move
                break;
            }
            else
            {
                m_frame->PopTool( tool );
                break;
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu( selection() );
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            bool placing = text != nullptr;

            if( !text )
            {
                m_controls->ForceCursorPosition( true, m_controls->GetCursorPosition() );
                PCB_LAYER_ID layer = m_frame->GetActiveLayer();

                // Init the new item attributes
                if( m_editModules )
                {
                    TEXTE_MODULE* textMod = new TEXTE_MODULE( (MODULE*) m_frame->GetModel() );

                    textMod->SetLayer( layer );
                    textMod->SetTextSize( dsnSettings.GetTextSize( layer ) );
                    textMod->SetThickness( dsnSettings.GetTextThickness( layer ) );
                    textMod->SetItalic( dsnSettings.GetTextItalic( layer ) );
                    textMod->SetKeepUpright( dsnSettings.GetTextUpright( layer ) );
                    textMod->SetTextPos( (wxPoint) cursorPos );

                    text = textMod;

                    DIALOG_TEXT_PROPERTIES textDialog( m_frame, textMod );
                    bool cancelled;

                    RunMainStack([&]() {
                        cancelled = !textDialog.ShowModal() || textMod->GetText().IsEmpty();
                    } );

                    if( cancelled )
                    {
                        delete text;
                        text = nullptr;
                    }
                    else if( textMod->GetTextPos() != (wxPoint) cursorPos )
                    {
                        // If the user modified the location then go ahead and place it there.
                        // Otherwise we'll drag.
                        placing = true;
                    }
                }
                else
                {
                    TEXTE_PCB* textPcb = new TEXTE_PCB( m_frame->GetModel() );
                    // TODO we have to set IS_NEW, otherwise InstallTextPCB.. creates an undo entry :| LEGACY_CLEANUP
                    textPcb->SetFlags( IS_NEW );

                    textPcb->SetLayer( layer );

                    // Set the mirrored option for layers on the BACK side of the board
                    if( IsBackLayer( layer ) )
                        textPcb->SetMirrored( true );

                    textPcb->SetTextSize( dsnSettings.GetTextSize( layer ) );
                    textPcb->SetThickness( dsnSettings.GetTextThickness( layer ) );
                    textPcb->SetItalic( dsnSettings.GetTextItalic( layer ) );
                    textPcb->SetTextPos( (wxPoint) cursorPos );

                    RunMainStack([&]() {
                        m_frame->InstallTextOptionsFrame( textPcb );
                    } );

                    if( textPcb->GetText().IsEmpty() )
                        delete textPcb;
                    else
                        text = textPcb;
                }

                if( text )
                {
                    m_controls->WarpCursor( text->GetPosition(), true );
                    m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, text );
                }
            }

            if( placing )
            {
                text->ClearFlags();
                m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

                commit.Add( text );
                commit.Push( _( "Place a text" ) );

                m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, text );

                text = nullptr;
            }

            m_controls->ForceCursorPosition( false );
            m_controls->ShowCursor( true );
            m_controls->CaptureCursor( text != nullptr );
            m_controls->SetAutoPan( text != nullptr );
        }
        else if( text && evt->IsMotion() )
        {
            text->SetPosition( (wxPoint) cursorPos );
            selection().SetReferencePoint( cursorPos );
            m_view->Update( &selection() );
            frame()->SetMsgPanel( text );
        }

        else if( text && evt->IsAction( &PCB_ACTIONS::properties ) )
        {
            // Calling 'Properties' action clears the selection, so we need to restore it
            reselect = true;
        }

        else
            evt->SetPassEvent();
    }

    frame()->SetMsgPanel( board() );
    return 0;
}


void DRAWING_TOOL::constrainDimension( DIMENSION* dimension )
{
    const VECTOR2I lineVector{ dimension->GetEnd() - dimension->GetOrigin() };

    dimension->SetEnd( wxPoint(
        VECTOR2I( dimension->GetOrigin() ) + GetVectorSnapped45( lineVector ) ) );
}


int DRAWING_TOOL::DrawDimension( const TOOL_EVENT& aEvent )
{
    if( m_editModules && !m_frame->GetModel() )
        return 0;

    POINT_EDITOR* pointEditor = m_toolMgr->GetTool<POINT_EDITOR>();
    DIMENSION*    dimension = NULL;
    BOARD_COMMIT  commit( m_frame );
    GRID_HELPER  grid( m_frame );

    // Add a VIEW_GROUP that serves as a preview for the new item
    PCBNEW_SELECTION preview;

    m_view->Add( &preview );

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );

    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::DIMENSION );

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();

    enum DIMENSION_STEPS
    {
        SET_ORIGIN = 0,
        SET_END,
        SET_HEIGHT,
        FINISHED
    };
    int step = SET_ORIGIN;

    // Prime the pump
    m_toolMgr->RunAction( ACTIONS::refreshPreview );

    if( aEvent.HasPosition() )
        m_toolMgr->RunAction( ACTIONS::cursorClick );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        if( !pointEditor->HasPoint() )
            m_frame->GetCanvas()->SetCurrentCursor( wxCURSOR_PENCIL );

        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( !evt->Modifier( MD_ALT ) );
        m_controls->SetSnapping( !evt->Modifier( MD_ALT ) );
        VECTOR2I cursorPos = grid.BestSnapAnchor( m_controls->GetMousePosition(), nullptr );
        m_controls->ForceCursorPosition( true, cursorPos );

        auto cleanup = [&] () {
            preview.Clear();
            delete dimension;
            dimension = nullptr;
            step = SET_ORIGIN;
        };

        if( evt->IsCancelInteractive() )
        {
            m_controls->SetAutoPan( false );

            if( step != SET_ORIGIN )    // start from the beginning
                cleanup();
            else
            {
                m_frame->PopTool( tool );
                break;
            }
        }
        else if( evt->IsActivate() )
        {
            if( step != SET_ORIGIN )
                cleanup();

            if( evt->IsPointEditor() )
            {
                // don't exit (the point editor runs in the background)
            }
            else if( evt->IsMoveTool() )
            {
                // leave ourselves on the stack so we come back after the move
                break;
            }
            else
            {
                m_frame->PopTool( tool );
                break;
            }
        }
        else if( evt->IsAction( &PCB_ACTIONS::incWidth ) && step != SET_ORIGIN )
        {
            m_lineWidth += WIDTH_STEP;
            dimension->SetWidth( m_lineWidth );
            m_view->Update( &preview );
            frame()->SetMsgPanel( dimension );
        }
        else if( evt->IsAction( &PCB_ACTIONS::decWidth ) && step != SET_ORIGIN )
        {
            if( m_lineWidth > WIDTH_STEP )
            {
                m_lineWidth -= WIDTH_STEP;
                dimension->SetWidth( m_lineWidth );
                m_view->Update( &preview );
                frame()->SetMsgPanel( dimension );
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu( selection() );
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            switch( step )
            {
            case SET_ORIGIN:
            {
                m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

                PCB_LAYER_ID layer = getDrawingLayer();
                const BOARD_DESIGN_SETTINGS& boardSettings = m_board->GetDesignSettings();

                if( layer == Edge_Cuts )        // dimensions are not allowed on EdgeCuts
                    layer = Dwgs_User;

                // Init the new item attributes
                dimension = new DIMENSION( m_board );
                dimension->SetLayer( layer );
                dimension->SetOrigin( (wxPoint) cursorPos );
                dimension->SetEnd( (wxPoint) cursorPos );
                dimension->Text().SetTextSize( boardSettings.GetTextSize( layer ) );
                dimension->Text().SetThickness( boardSettings.GetTextThickness( layer ) );
                dimension->Text().SetItalic( boardSettings.GetTextItalic( layer ) );
                dimension->SetWidth( boardSettings.GetLineThickness( layer ) );
                dimension->SetUnits( m_frame->GetUserUnits(), false );
                dimension->AdjustDimensionDetails();

                preview.Add( dimension );
                frame()->SetMsgPanel( dimension );

                m_controls->SetAutoPan( true );
                m_controls->CaptureCursor( true );
            }
            break;

            case SET_END:
                dimension->SetEnd( (wxPoint) cursorPos );

                if( !!evt->Modifier( MD_CTRL ) )
                    constrainDimension( dimension );

                // Dimensions that have origin and end in the same spot are not valid
                if( dimension->GetOrigin() == dimension->GetEnd() )
                    --step;

                break;

            case SET_HEIGHT:
            {
                if( (wxPoint) cursorPos != dimension->GetPosition() )
                {
                    assert( dimension->GetOrigin() != dimension->GetEnd() );
                    assert( dimension->GetWidth() > 0 );

                    preview.Remove( dimension );

                    commit.Add( dimension );
                    commit.Push( _( "Draw a dimension" ) );

                    m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, dimension );
                }
            }
            break;
            }

            if( ++step == FINISHED )
            {
                step = SET_ORIGIN;
                m_controls->SetAutoPan( false );
                m_controls->CaptureCursor( false );
            }
        }
        else if( evt->IsMotion() )
        {
            switch( step )
            {
            case SET_END:
                dimension->SetEnd( (wxPoint) cursorPos );

                if( !!evt->Modifier( MD_CTRL ) )
                    constrainDimension( dimension );

                break;

            case SET_HEIGHT:
            {
                // Calculating the direction of travel perpendicular to the selected axis
                double angle = dimension->GetAngle() + ( M_PI / 2 );

                wxPoint delta( (wxPoint) cursorPos - dimension->m_featureLineDO );
                double  height = ( delta.x * cos( angle ) ) + ( delta.y * sin( angle ) );
                dimension->SetHeight( height );
            }
            break;
            }

            // Show a preview of the item
            m_view->Update( &preview );
            if( step )
                frame()->SetMsgPanel( dimension );
            else
                frame()->SetMsgPanel( board() );
        }
        else
            evt->SetPassEvent();
    }

    if( step != SET_ORIGIN )
        delete dimension;

    m_controls->SetAutoPan( false );
    m_controls->ForceCursorPosition( false );

    m_view->Remove( &preview );
    frame()->SetMsgPanel( board() );
    return 0;
}


int DRAWING_TOOL::PlaceImportedGraphics( const TOOL_EVENT& aEvent )
{
    if( !m_frame->GetModel() )
        return 0;

    // Note: PlaceImportedGraphics() will convert PCB_LINE_T and PCB_TEXT_T to module graphic
    // items if needed
    DIALOG_IMPORT_GFX dlg( m_frame, m_editModules );
    int dlgResult = dlg.ShowModal();

    auto& list = dlg.GetImportedItems();

    if( dlgResult != wxID_OK )
        return 0;

    // Ensure the list is not empty:
    if( list.empty() )
    {
        wxMessageBox( _( "No graphic items found in file to import") );
        return 0;
    }

    m_toolMgr->RunAction( ACTIONS::cancelInteractive, true );

    // Add a VIEW_GROUP that serves as a preview for the new item
    PCBNEW_SELECTION preview;
    BOARD_COMMIT commit( m_frame );

    // Build the undo list & add items to the current view
    for( auto& ptr : list)
    {
        EDA_ITEM* item = ptr.get();

        if( m_editModules )
            wxASSERT( item->Type() == PCB_MODULE_EDGE_T || item->Type() == PCB_MODULE_TEXT_T );
        else
            wxASSERT( item->Type() == PCB_LINE_T || item->Type() == PCB_TEXT_T );

        if( dlg.IsPlacementInteractive() )
            preview.Add( item );
        else
            commit.Add( item );

        ptr.release();
    }

    if( !dlg.IsPlacementInteractive() )
    {
        commit.Push( _( "Place a DXF_SVG drawing" ) );
        return 0;
    }

    BOARD_ITEM* firstItem = static_cast<BOARD_ITEM*>( preview.Front() );
    m_view->Add( &preview );

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );
    m_controls->ForceCursorPosition( false );

    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::DXF );

    // Now move the new items to the current cursor position:
    VECTOR2I cursorPos = m_controls->GetCursorPosition();
    VECTOR2I delta = cursorPos - firstItem->GetPosition();

    for( EDA_ITEM* item : preview )
        static_cast<BOARD_ITEM*>( item )->Move( (wxPoint) delta );

    m_view->Update( &preview );

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        m_frame->GetCanvas()->SetCurrentCursor( wxCURSOR_ARROW );
        cursorPos = m_controls->GetCursorPosition();

        if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            preview.FreeItems();

            m_frame->PopTool( tool );
            break;
        }
        else if( evt->IsMotion() )
        {
            delta = cursorPos - firstItem->GetPosition();

            for( auto item : preview )
                static_cast<BOARD_ITEM*>( item )->Move( (wxPoint) delta );

            m_view->Update( &preview );
        }
        else if( evt->Category() == TC_COMMAND )
        {
            // TODO it should be handled by EDIT_TOOL, so add items and select?
            if( TOOL_EVT_UTILS::IsRotateToolEvt( *evt ) )
            {
                const auto rotationPoint = (wxPoint) cursorPos;
                const auto rotationAngle = TOOL_EVT_UTILS::GetEventRotationAngle( *m_frame, *evt );

                for( auto item : preview )
                    static_cast<BOARD_ITEM*>( item )->Rotate( rotationPoint, rotationAngle );

                m_view->Update( &preview );
            }
            else if( evt->IsAction( &PCB_ACTIONS::flip ) )
            {
                bool leftRight = m_frame->Settings().m_FlipLeftRight;

                for( auto item : preview )
                    static_cast<BOARD_ITEM*>( item )->Flip( (wxPoint) cursorPos, leftRight);

                m_view->Update( &preview );
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu( selection() );
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            // Place the imported drawings
            for( auto item : preview )
                commit.Add( item );

            commit.Push( _( "Place a DXF_SVG drawing" ) );
            break;
        }
        else
            evt->SetPassEvent();
    }

    preview.Clear();
    m_view->Remove( &preview );
    return 0;
}


int DRAWING_TOOL::SetAnchor( const TOOL_EVENT& aEvent )
{
    assert( m_editModules );

    if( !m_frame->GetModel() )
        return 0;

    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::ANCHOR );

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );
    m_controls->SetAutoPan( true );
    m_controls->CaptureCursor( false );

    while( TOOL_EVENT* evt = Wait() )
    {
        m_frame->GetCanvas()->SetCurrentCursor( wxCURSOR_BULLSEYE );

        if( evt->IsClick( BUT_LEFT ) )
        {
            MODULE* module = (MODULE*) m_frame->GetModel();
            BOARD_COMMIT commit( m_frame );
            commit.Modify( module );

            // set the new relative internal local coordinates of footprint items
            VECTOR2I    cursorPos = m_controls->GetCursorPosition();
            wxPoint     moveVector = module->GetPosition() - (wxPoint) cursorPos;
            module->MoveAnchorPosition( moveVector );

            commit.Push( _( "Move the footprint reference anchor" ) );

            // Usually, we do not need to change twice the anchor position,
            // so deselect the active tool
            m_frame->PopTool( tool );
            break;
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu( selection() );
        }
        else if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            m_frame->PopTool( tool );
            break;
        }
        else
            evt->SetPassEvent();
    }

    return 0;
}


bool DRAWING_TOOL::drawSegment( const std::string& aTool, int aShape, DRAWSEGMENT*& aGraphic,
                                OPT<VECTOR2D> aStartingPoint )
{
    // Only two shapes are currently supported
    assert( aShape == S_SEGMENT || aShape == S_CIRCLE );
    GRID_HELPER   grid( m_frame );
    POINT_EDITOR* pointEditor = m_toolMgr->GetTool<POINT_EDITOR>();

    m_lineWidth = getSegmentWidth( getDrawingLayer() );
    m_frame->SetActiveLayer( getDrawingLayer() );

    // Add a VIEW_GROUP that serves as a preview for the new item
    PCBNEW_SELECTION preview;
    m_view->Add( &preview );

    m_controls->ShowCursor( true );

    bool     direction45 = false;    // 45 degrees only mode
    bool     started = false;
    bool     cancelled = false;
    bool     isLocalOriginSet = ( m_frame->GetScreen()->m_LocalOrigin != VECTOR2D( 0, 0 ) );
    VECTOR2I cursorPos = m_controls->GetMousePosition();

    // Prime the pump
    m_toolMgr->RunAction( ACTIONS::refreshPreview );

    if( aStartingPoint )
        m_toolMgr->RunAction( ACTIONS::cursorClick );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        if( !pointEditor->HasPoint() )
            m_frame->GetCanvas()->SetCurrentCursor( wxCURSOR_PENCIL );

        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( !evt->Modifier( MD_ALT ) );
        m_controls->SetSnapping( !evt->Modifier( MD_ALT ) );
        cursorPos = grid.BestSnapAnchor( m_controls->GetMousePosition(), getDrawingLayer() );
        m_controls->ForceCursorPosition( true, cursorPos );

        // 45 degree angle constraint enabled with an option and toggled with Ctrl
        bool limit45 = frame()->Settings().m_Use45DegreeGraphicSegments;

        if( evt->Modifier( MD_CTRL ) )
            limit45 = !limit45;

        if( direction45 != limit45 && started && aShape == S_SEGMENT )
        {
            direction45 = limit45;

            if( direction45 )
            {
                const VECTOR2I lineVector( cursorPos - VECTOR2I( aGraphic->GetStart() ) );

                // get a restricted 45/H/V line from the last fixed point to the cursor
                auto newEnd = GetVectorSnapped45( lineVector );
                aGraphic->SetEnd( aGraphic->GetStart() + (wxPoint) newEnd );
                m_controls->ForceCursorPosition( true, VECTOR2I( aGraphic->GetEnd() ) );
            }
            else
            {
                aGraphic->SetEnd( (wxPoint) cursorPos );
            }

            m_view->Update( &preview );
            frame()->SetMsgPanel( aGraphic );
        }

        auto cleanup = [&] () {
            preview.Clear();
            m_view->Update( &preview );
            delete aGraphic;
            aGraphic = nullptr;

            if( !isLocalOriginSet )
                m_frame->GetScreen()->m_LocalOrigin = VECTOR2D( 0, 0 );
        };

        if( evt->IsCancelInteractive() )
        {
            if( started )
                cleanup();
            else
            {
                m_frame->PopTool( aTool );
                cancelled = true;
            }

            break;
        }
        else if( evt->IsActivate() )
        {
            if( evt->IsPointEditor() )
            {
                // don't exit (the point editor runs in the background)
            }
            else if( evt->IsMoveTool() )
            {
                if( started )
                    cleanup();

                // leave ourselves on the stack so we come back after the move
                cancelled = true;
                break;
            }
            else
            {
                if( started )
                    cleanup();

                m_frame->PopTool( aTool );
                cancelled = true;
                break;
            }
        }
        else if( evt->IsAction( &PCB_ACTIONS::layerChanged ) )
        {
            m_lineWidth = getSegmentWidth( getDrawingLayer() );
            aGraphic->SetLayer( getDrawingLayer() );
            aGraphic->SetWidth( m_lineWidth );
            m_view->Update( &preview );
            frame()->SetMsgPanel( aGraphic );
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu( selection() );
        }
        else if( evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT ) )
        {
            if( !started )
            {
                m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

                if( aStartingPoint )
                {
                    cursorPos = aStartingPoint.get();
                    aStartingPoint = NULLOPT;
                }

                m_lineWidth = getSegmentWidth( getDrawingLayer() );

                // Init the new item attributes
                aGraphic->SetShape( (STROKE_T) aShape );
                aGraphic->SetWidth( m_lineWidth );
                aGraphic->SetStart( (wxPoint) cursorPos );
                aGraphic->SetEnd( (wxPoint) cursorPos );
                aGraphic->SetLayer( getDrawingLayer() );

                if( !isLocalOriginSet )
                    m_frame->GetScreen()->m_LocalOrigin = cursorPos;

                preview.Add( aGraphic );
                frame()->SetMsgPanel( aGraphic );
                m_controls->SetAutoPan( true );
                m_controls->CaptureCursor( true );

                started = true;
            }
            else
            {
                auto snapItem = dyn_cast<DRAWSEGMENT*>( grid.GetSnapped() );

                if( aGraphic->GetEnd() == aGraphic->GetStart()
                    || ( evt->IsDblClick( BUT_LEFT ) && aShape == S_SEGMENT )
                    || snapItem )
                // User has clicked twice in the same spot
                //  or clicked on the end of an existing segment (closing a path)
                {
                    BOARD_COMMIT commit( m_frame );

                    // If the user clicks on an existing snap point from a drawsegment
                    //  we finish the segment as they are likely closing a path
                    if( snapItem && aGraphic->GetLength() > 0.0 )
                    {
                        commit.Add( aGraphic );
                        commit.Push( _( "Draw a line segment" ) );
                        m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, aGraphic );
                    }
                    else
                    {
                        delete aGraphic;
                    }

                    aGraphic = nullptr;
                }

                preview.Clear();
                break;
            }
        }
        else if( evt->IsMotion() )
        {
            // 45 degree lines
            if( direction45 && aShape == S_SEGMENT )
            {
                const VECTOR2I lineVector( cursorPos - VECTOR2I( aGraphic->GetStart() ) );

                // get a restricted 45/H/V line from the last fixed point to the cursor
                auto newEnd = GetVectorSnapped45( lineVector );
                aGraphic->SetEnd( aGraphic->GetStart() + (wxPoint) newEnd );
                m_controls->ForceCursorPosition( true, VECTOR2I( aGraphic->GetEnd() ) );
            }
            else
                aGraphic->SetEnd( (wxPoint) cursorPos );

            m_view->Update( &preview );

            if( started )
                frame()->SetMsgPanel( aGraphic );
            else
                frame()->SetMsgPanel( board() );
        }
        else if( evt->IsAction( &PCB_ACTIONS::incWidth ) )
        {
            m_lineWidth += WIDTH_STEP;
            aGraphic->SetWidth( m_lineWidth );
            m_view->Update( &preview );
            frame()->SetMsgPanel( aGraphic );
        }
        else if( evt->IsAction( &PCB_ACTIONS::decWidth ) && ( m_lineWidth > WIDTH_STEP ) )
        {
            m_lineWidth -= WIDTH_STEP;
            aGraphic->SetWidth( m_lineWidth );
            m_view->Update( &preview );
            frame()->SetMsgPanel( aGraphic );
        }
        else if( evt->IsAction( &ACTIONS::resetLocalCoords ) )
        {
            isLocalOriginSet = true;
        }
        else
            evt->SetPassEvent();
    }

    if( !isLocalOriginSet ) // reset the relative coordinte if it was not set before
        m_frame->GetScreen()->m_LocalOrigin = VECTOR2D( 0, 0 );

    m_view->Remove( &preview );
    frame()->SetMsgPanel( board() );
    m_controls->SetAutoPan( false );
    m_controls->CaptureCursor( false );
    m_controls->ForceCursorPosition( false );

    return !cancelled;
}


/**
 * Update an arc DRAWSEGMENT from the current state
 * of an Arc Geometry Manager
 */
static void updateArcFromConstructionMgr( const KIGFX::PREVIEW::ARC_GEOM_MANAGER& aMgr,
                                          DRAWSEGMENT& aArc )
{
    auto vec = aMgr.GetOrigin();

    aArc.SetCenter( { vec.x, vec.y } );

    vec = aMgr.GetStartRadiusEnd();
    aArc.SetArcStart( { vec.x, vec.y } );

    aArc.SetAngle( RAD2DECIDEG( -aMgr.GetSubtended() ) );
}


bool DRAWING_TOOL::drawArc( const std::string& aTool, DRAWSEGMENT*& aGraphic, bool aImmediateMode )
{
    m_lineWidth = getSegmentWidth( getDrawingLayer() );

    // Arc geometric construction manager
    KIGFX::PREVIEW::ARC_GEOM_MANAGER arcManager;

    // Arc drawing assistant overlay
    KIGFX::PREVIEW::ARC_ASSISTANT arcAsst( arcManager, m_frame->GetUserUnits() );

    // Add a VIEW_GROUP that serves as a preview for the new item
    PCBNEW_SELECTION preview;
    m_view->Add( &preview );
    m_view->Add( &arcAsst );
    GRID_HELPER grid( m_frame );

    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );

    bool firstPoint = false;
    bool cancelled = false;

    // Prime the pump
    m_toolMgr->RunAction( ACTIONS::refreshPreview );

    if( aImmediateMode )
        m_toolMgr->RunAction( ACTIONS::cursorClick );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        PCB_LAYER_ID layer = getDrawingLayer();
        aGraphic->SetLayer( layer );

        m_frame->GetCanvas()->SetCurrentCursor( wxCURSOR_PENCIL );
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( !evt->Modifier( MD_ALT ) );
        m_controls->SetSnapping( !evt->Modifier( MD_ALT ) );
        VECTOR2I cursorPos = grid.BestSnapAnchor( m_controls->GetMousePosition(), aGraphic );
        m_controls->ForceCursorPosition( true, cursorPos );

        auto cleanup = [&] () {
            preview.Clear();
            delete aGraphic;
            aGraphic = nullptr;
        };

        if( evt->IsCancelInteractive() )
        {
            if( firstPoint )
                cleanup();
            else
            {
                m_frame->PopTool( aTool );
                cancelled = true;
            }

            break;
        }
        else if( evt->IsActivate() )
        {
            if( evt->IsPointEditor() )
            {
                // don't exit (the point editor runs in the background)
            }
            else if( evt->IsMoveTool() )
            {
                if( firstPoint )
                    cleanup();

                // leave ourselves on the stack so we come back after the move
                cancelled = true;
                break;
            }
            else
            {
                if( firstPoint )
                    cleanup();

                m_frame->PopTool( aTool );
                cancelled = true;
                break;
            }
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( !firstPoint )
            {
                m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

                m_controls->SetAutoPan( true );
                m_controls->CaptureCursor( true );

                m_lineWidth = getSegmentWidth( getDrawingLayer() );

                // Init the new item attributes
                // (non-geometric, those are handled by the manager)
                aGraphic->SetShape( S_ARC );
                aGraphic->SetWidth( m_lineWidth );

                preview.Add( aGraphic );
                firstPoint = true;
            }

            arcManager.AddPoint( cursorPos, true );
        }
        else if( evt->IsAction( &PCB_ACTIONS::deleteLastPoint ) )
        {
            arcManager.RemoveLastPoint();
        }
        else if( evt->IsMotion() )
        {
            // set angle snap
            arcManager.SetAngleSnap( evt->Modifier( MD_CTRL ) );

            // update, but don't step the manager state
            arcManager.AddPoint( cursorPos, false );
        }
        else if( evt->IsAction( &PCB_ACTIONS::layerChanged ) )
        {
            m_lineWidth = getSegmentWidth( getDrawingLayer() );
            aGraphic->SetLayer( getDrawingLayer() );
            aGraphic->SetWidth( m_lineWidth );
            m_view->Update( &preview );
            frame()->SetMsgPanel( aGraphic );
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu( selection() );
        }
        else if( evt->IsAction( &PCB_ACTIONS::incWidth ) )
        {
            m_lineWidth += WIDTH_STEP;
            aGraphic->SetWidth( m_lineWidth );
            m_view->Update( &preview );
            frame()->SetMsgPanel( aGraphic );
        }
        else if( evt->IsAction( &PCB_ACTIONS::decWidth ) && m_lineWidth > WIDTH_STEP )
        {
            m_lineWidth -= WIDTH_STEP;
            aGraphic->SetWidth( m_lineWidth );
            m_view->Update( &preview );
            frame()->SetMsgPanel( aGraphic );
        }
        else if( evt->IsAction( &PCB_ACTIONS::arcPosture ) )
        {
            arcManager.ToggleClockwise();
        }
        else
            evt->SetPassEvent();

        if( arcManager.IsComplete() )
        {
            break;
        }
        else if( arcManager.HasGeometryChanged() )
        {
            updateArcFromConstructionMgr( arcManager, *aGraphic );
            m_view->Update( &preview );
            m_view->Update( &arcAsst );

            if( firstPoint )
                frame()->SetMsgPanel( aGraphic );
            else
                frame()->SetMsgPanel( board() );
        }
    }

    preview.Remove( aGraphic );
    m_view->Remove( &arcAsst );
    m_view->Remove( &preview );
    frame()->SetMsgPanel( board() );
    m_controls->SetAutoPan( false );
    m_controls->CaptureCursor( false );
    m_controls->ForceCursorPosition( false );

    return !cancelled;
}


bool DRAWING_TOOL::getSourceZoneForAction( ZONE_MODE aMode, ZONE_CONTAINER*& aZone )
{
    bool clearSelection = false;
    aZone = nullptr;

    // not an action that needs a source zone
    if( aMode == ZONE_MODE::ADD || aMode == ZONE_MODE::GRAPHIC_POLYGON )
        return true;

    SELECTION_TOOL*         selTool = m_toolMgr->GetTool<SELECTION_TOOL>();
    const PCBNEW_SELECTION& selection = selTool->GetSelection();

    if( selection.Empty() )
    {
        clearSelection = true;
        m_toolMgr->RunAction( PCB_ACTIONS::selectionCursor, true );
    }

    // we want a single zone
    if( selection.Size() == 1 )
        aZone = dyn_cast<ZONE_CONTAINER*>( selection[0] );

    // expected a zone, but didn't get one
    if( !aZone )
    {
        if( clearSelection )
            m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

        return false;
    }

    return true;
}

int DRAWING_TOOL::DrawZone( const TOOL_EVENT& aEvent )
{
    if( m_editModules && !m_frame->GetModel() )
        return 0;

    ZONE_MODE zoneMode = aEvent.Parameter<ZONE_MODE>();
    MODE      drawMode = MODE::ZONE;

    if( aEvent.IsAction( &PCB_ACTIONS::drawZoneKeepout ) )
        drawMode = MODE::KEEPOUT;

    if( aEvent.IsAction( &PCB_ACTIONS::drawPolygon ) )
        drawMode = MODE::GRAPHIC_POLYGON;

    SCOPED_DRAW_MODE scopedDrawMode( m_mode, drawMode );

    // get a source zone, if we need one. We need it for:
    // ZONE_MODE::CUTOUT (adding a hole to the source zone)
    // ZONE_MODE::SIMILAR (creating a new zone using settings of source zone
    ZONE_CONTAINER* sourceZone = nullptr;

    if( !getSourceZoneForAction( zoneMode, sourceZone ) )
        return 0;

    ZONE_CREATE_HELPER::PARAMS params;

    params.m_keepout = drawMode == MODE::KEEPOUT;
    params.m_mode = zoneMode;
    params.m_sourceZone = sourceZone;

    if( zoneMode == ZONE_MODE::GRAPHIC_POLYGON )
        params.m_layer = getDrawingLayer();
    else if( zoneMode == ZONE_MODE::SIMILAR )
        params.m_layer = sourceZone->GetLayer();
    else
        params.m_layer = m_frame->GetActiveLayer();

    ZONE_CREATE_HELPER zoneTool( *this, params );

    // the geometry manager which handles the zone geometry, and
    // hands the calculated points over to the zone creator tool
    POLYGON_GEOM_MANAGER polyGeomMgr( zoneTool );

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();    // register for events

    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );

    bool    started     = false;
    GRID_HELPER grid( m_frame );
    STATUS_TEXT_POPUP status( m_frame );
    status.SetTextColor( wxColour( 255, 0, 0 ) );
    status.SetText( _( "Self-intersecting polygons are not allowed" ) );

    // Prime the pump
    if( aEvent.HasPosition() )
        m_toolMgr->RunAction( ACTIONS::cursorClick );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        m_frame->GetCanvas()->SetCurrentCursor( wxCURSOR_PENCIL );
        LSET layers( m_frame->GetActiveLayer() );
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( !evt->Modifier( MD_ALT ) );
        m_controls->SetSnapping( !evt->Modifier( MD_ALT ) );
        VECTOR2I cursorPos = grid.BestSnapAnchor( m_controls->GetMousePosition(), layers );
        m_controls->ForceCursorPosition( true, cursorPos );

        auto cleanup = [&] () {
            polyGeomMgr.Reset();
            started = false;
            m_controls->SetAutoPan( false );
            m_controls->CaptureCursor( false );
        };

        if( evt->IsCancelInteractive())
        {
            if( polyGeomMgr.IsPolygonInProgress() )
                cleanup();
            else
            {
                m_frame->PopTool( tool );
                break;
            }
        }
        else if( evt->IsActivate() )
        {
            if( polyGeomMgr.IsPolygonInProgress() )
                cleanup();

            if( evt->IsPointEditor() )
            {
                // don't exit (the point editor runs in the background)
            }
            else if( evt->IsMoveTool() )
            {
                // leave ourselves on the stack so we come back after the move
                break;
            }
            else
            {
                m_frame->PopTool( tool );
                break;
            }
        }
        else if( evt->IsAction( &PCB_ACTIONS::layerChanged ) )
        {
            if( zoneMode == ZONE_MODE::GRAPHIC_POLYGON )
                params.m_layer = getDrawingLayer();
            else if( zoneMode == ZONE_MODE::ADD || zoneMode == ZONE_MODE::CUTOUT )
                params.m_layer = frame()->GetActiveLayer();
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu( selection() );
        }
        // events that lock in nodes
        else if( evt->IsClick( BUT_LEFT )
                 || evt->IsDblClick( BUT_LEFT )
                 || evt->IsAction( &PCB_ACTIONS::closeZoneOutline ) )
        {
            // Check if it is double click / closing line (so we have to finish the zone)
            const bool endPolygon = evt->IsDblClick( BUT_LEFT )
                                    || evt->IsAction( &PCB_ACTIONS::closeZoneOutline )
                                    || polyGeomMgr.NewPointClosesOutline( cursorPos );

            if( endPolygon )
            {
                polyGeomMgr.SetFinished();
                polyGeomMgr.Reset();

                started = false;
                m_controls->SetAutoPan( false );
                m_controls->CaptureCursor( false );
            }

            // adding a corner
            else if( polyGeomMgr.AddPoint( cursorPos ) )
            {
                if( !started )
                {
                    started = true;
                    m_controls->SetAutoPan( true );
                    m_controls->CaptureCursor( true );
                }
            }
        }
        else if( evt->IsAction( &PCB_ACTIONS::deleteLastPoint ) )
        {
            polyGeomMgr.DeleteLastCorner();

            if( !polyGeomMgr.IsPolygonInProgress() )
            {
                // report finished as an empty shape
                polyGeomMgr.SetFinished();

                // start again
                started = false;
                m_controls->SetAutoPan( false );
                m_controls->CaptureCursor( false );
            }
        }
        else if( polyGeomMgr.IsPolygonInProgress()
                 && ( evt->IsMotion() || evt->IsDrag( BUT_LEFT ) ) )
        {
            polyGeomMgr.SetCursorPosition( cursorPos, evt->Modifier( MD_CTRL )
                                                      ? POLYGON_GEOM_MANAGER::LEADER_MODE::DEG45
                                                      : POLYGON_GEOM_MANAGER::LEADER_MODE::DIRECT );

            if( polyGeomMgr.IsSelfIntersecting( true ) )
            {
                wxPoint p = wxGetMousePosition() + wxPoint( 20, 20 );
                status.Move( p );
                status.PopupFor( 1500 );
            }
            else
            {
                status.Hide();
            }
        }
        else
            evt->SetPassEvent();

    }    // end while

    m_controls->ForceCursorPosition( false );
    return 0;
}


int DRAWING_TOOL::DrawVia( const TOOL_EVENT& aEvent )
{
    struct VIA_PLACER : public INTERACTIVE_PLACER_BASE
    {
        GRID_HELPER m_gridHelper;

        VIA_PLACER( PCB_BASE_EDIT_FRAME* aFrame ) : m_gridHelper( aFrame )
        {}

        TRACK* findTrack( VIA* aVia )
        {
            const LSET lset = aVia->GetLayerSet();
            wxPoint position = aVia->GetPosition();
            BOX2I bbox = aVia->GetBoundingBox();

            std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> items;
            auto view = m_frame->GetCanvas()->GetView();
            std::vector<TRACK*> possible_tracks;

            view->Query( bbox, items );

            for( auto it : items )
            {
                BOARD_ITEM* item = static_cast<BOARD_ITEM*>( it.first );

                if( !(item->GetLayerSet() & lset ).any() )
                    continue;

                if( auto track = dyn_cast<TRACK*>( item ) )
                {
                    if( TestSegmentHit( position, track->GetStart(), track->GetEnd(),
                                        ( track->GetWidth() + aVia->GetWidth() ) / 2 ) )
                        possible_tracks.push_back( track );
                }
            }

            TRACK* return_track = nullptr;
            int min_d = std::numeric_limits<int>::max();
            for( auto track : possible_tracks )
            {
                SEG test( track->GetStart(), track->GetEnd() );
                auto dist = ( test.NearestPoint( position ) - position ).EuclideanNorm();

                if( dist < min_d )
                {
                    min_d = dist;
                    return_track = track;
                }
            }

            return return_track;
        }


        bool hasDRCViolation( VIA* aVia )
        {
            const LSET lset = aVia->GetLayerSet();
            wxPoint position = aVia->GetPosition();
            int drillRadius = aVia->GetDrillValue() / 2;
            BOX2I bbox = aVia->GetBoundingBox();

            std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> items;
            int net = 0;
            int clearance = 0;
            auto view = m_frame->GetCanvas()->GetView();
            int holeToHoleMin = m_frame->GetBoard()->GetDesignSettings().m_HoleToHoleMin;

            view->Query( bbox, items );

            for( auto it : items )
            {
                BOARD_ITEM* item = static_cast<BOARD_ITEM*>( it.first );

                if( !(item->GetLayerSet() & lset ).any() )
                    continue;

                if( auto track = dyn_cast<TRACK*>( item ) )
                {
                    int max_clearance = std::max( clearance, track->GetClearance() );

                    if( TestSegmentHit( position, track->GetStart(), track->GetEnd(),
                            ( track->GetWidth() + aVia->GetWidth() ) / 2  + max_clearance ) )
                    {
                        if( net && track->GetNetCode() != net )
                            return true;

                        net = track->GetNetCode();
                        clearance = track->GetClearance();
                    }
                }

                if( auto via = dyn_cast<VIA*>( item ) )
                {
                    int dist = KiROUND( GetLineLength( position, via->GetPosition() ) );

                    if( dist < drillRadius + via->GetDrillValue() / 2 + holeToHoleMin )
                        return true;
                }

                if( auto mod = dyn_cast<MODULE*>( item ) )
                {
                    for( D_PAD* pad : mod->Pads() )
                    {
                        int max_clearance = std::max( clearance, pad->GetClearance() );

                        if( pad->HitTest( aVia->GetBoundingBox(), false, max_clearance ) )
                        {
                            if( net && pad->GetNetCode() != net )
                                return true;

                            net = pad->GetNetCode();
                            clearance = pad->GetClearance();
                        }

                        if( pad->GetDrillSize().x && pad->GetDrillShape() == PAD_DRILL_SHAPE_CIRCLE )
                        {
                            int dist = KiROUND( GetLineLength( position, pad->GetPosition() ) );

                            if( dist < drillRadius + pad->GetDrillSize().x / 2 + holeToHoleMin )
                                return true;
                        }
                    }
                }
            }

            return false;
        }


        int findStitchedZoneNet( VIA* aVia )
        {
            const wxPoint position = aVia->GetPosition();
            const LSET    lset = aVia->GetLayerSet();

            for( auto mod : m_board->Modules() )
            {
                for( D_PAD* pad : mod->Pads() )
                {
                    if( pad->HitTest( position ) && ( pad->GetLayerSet() & lset ).any() )
                        return -1;
                }
            }

            std::vector<ZONE_CONTAINER*> foundZones;

            for( ZONE_CONTAINER* zone : m_board->Zones() )
            {
                if( zone->HitTestFilledArea( position ) )
                    foundZones.push_back( zone );
            }

            std::sort( foundZones.begin(), foundZones.end(),
                [] ( const ZONE_CONTAINER* a, const ZONE_CONTAINER* b )
                {
                    return a->GetLayer() < b->GetLayer();
                } );

            // first take the net of the active layer
            for( ZONE_CONTAINER* z : foundZones )
            {
                if( m_frame->GetActiveLayer() == z->GetLayer() )
                    return z->GetNetCode();
            }

            // none? take the topmost visible layer
            for( ZONE_CONTAINER* z : foundZones )
            {
                if( m_board->IsLayerVisible( z->GetLayer() ) )
                    return z->GetNetCode();
            }

            return -1;
        }

        void SnapItem( BOARD_ITEM *aItem ) override
        {
            // If you place a Via on a track but not on its centerline, the current
            // connectivity algorithm will require us to put a kink in the track when
            // we break it (so that each of the two segments ends on the via center).
            // That's not ideal, and is in fact probably worse than forcing snap in
            // this situation.

            m_gridHelper.SetSnap( !( m_modifiers & MD_SHIFT ) );
            auto    via = static_cast<VIA*>( aItem );
            wxPoint position = via->GetPosition();
            TRACK*  track = findTrack( via );

            if( track )
            {
                SEG      trackSeg( track->GetStart(), track->GetEnd() );
                VECTOR2I snap = m_gridHelper.AlignToSegment( position, trackSeg );

                aItem->SetPosition( (wxPoint) snap );
            }
        }

        bool PlaceItem( BOARD_ITEM* aItem, BOARD_COMMIT& aCommit ) override
        {
            VIA*    via = static_cast<VIA*>( aItem );
            wxPoint viaPos = via->GetPosition();
            int     newNet;
            TRACK*  track = findTrack( via );

            if( hasDRCViolation( via ) )
                return false;

            if( track )
            {
                if( viaPos != track->GetStart() && viaPos != track->GetEnd() )
                {
                    aCommit.Modify( track );
                    TRACK* newTrack = dynamic_cast<TRACK*>( track->Clone() );
                    track->SetEnd( viaPos );
                    newTrack->SetStart( viaPos );
                    aCommit.Add( newTrack );
                }

                newNet = track->GetNetCode();
            }
            else
                newNet = findStitchedZoneNet( via );

            if( newNet > 0 )
                via->SetNetCode( newNet );

            aCommit.Add( aItem );
            return true;
        }

        std::unique_ptr<BOARD_ITEM> CreateItem() override
        {
            auto&   ds = m_board->GetDesignSettings();
            VIA*    via = new VIA( m_board );

            via->SetNetCode( 0 );
            via->SetViaType( ds.m_CurrentViaType );

            // for microvias, the size and hole will be changed later.
            via->SetWidth( ds.GetCurrentViaSize() );
            via->SetDrill( ds.GetCurrentViaDrill() );

            // Usual via is from copper to component.
            // layer pair is B_Cu and F_Cu.
            via->SetLayerPair( B_Cu, F_Cu );

            PCB_LAYER_ID    first_layer = m_frame->GetActiveLayer();
            PCB_LAYER_ID    last_layer;

            // prepare switch to new active layer:
            if( first_layer != m_frame->GetScreen()->m_Route_Layer_TOP )
                last_layer = m_frame->GetScreen()->m_Route_Layer_TOP;
            else
                last_layer = m_frame->GetScreen()->m_Route_Layer_BOTTOM;

            // Adjust the actual via layer pair
            switch( via->GetViaType() )
            {
            case VIA_BLIND_BURIED:
                via->SetLayerPair( first_layer, last_layer );
                break;

            case VIA_MICROVIA:    // from external to the near neighbor inner layer
            {
                PCB_LAYER_ID last_inner_layer =
                    ToLAYER_ID( ( m_board->GetCopperLayerCount() - 2 ) );

                if( first_layer == B_Cu )
                    last_layer = last_inner_layer;
                else if( first_layer == F_Cu )
                    last_layer = In1_Cu;
                else if( first_layer == last_inner_layer )
                    last_layer = B_Cu;
                else if( first_layer == In1_Cu )
                    last_layer = F_Cu;

                // else error: will be removed later
                via->SetLayerPair( first_layer, last_layer );

                // Update diameter and hole size, which where set previously
                // for normal vias
                NETINFO_ITEM* net = via->GetNet();

                if( net )
                {
                    via->SetWidth( net->GetMicroViaSize() );
                    via->SetDrill( net->GetMicroViaDrillSize() );
                }
            }
            break;

            default:
                break;
            }

            return std::unique_ptr<BOARD_ITEM>( via );
        }
    };

    VIA_PLACER placer( frame() );

    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::VIA );

    doInteractiveItemPlacement( aEvent.GetCommandStr().get(), &placer, _( "Place via" ),
                                IPO_REPEAT | IPO_SINGLE_CLICK );

    return 0;
}


int DRAWING_TOOL::getSegmentWidth( PCB_LAYER_ID aLayer ) const
{
    assert( m_board );
    return m_board->GetDesignSettings().GetLineThickness( aLayer );
}


PCB_LAYER_ID DRAWING_TOOL::getDrawingLayer() const
{
    PCB_LAYER_ID layer = m_frame->GetActiveLayer();

    if( ( GetDrawingMode() == MODE::DIMENSION || GetDrawingMode() == MODE::GRAPHIC_POLYGON )
        && IsCopperLayer( layer ) )
    {
        if( layer == F_Cu )
            layer = F_SilkS;
        else if( layer == B_Cu )
            layer = B_SilkS;
        else
            layer = Dwgs_User;

        m_frame->SetActiveLayer( layer );
    }

    return layer;
}


const unsigned int DRAWING_TOOL::WIDTH_STEP = Millimeter2iu( 0.1 );


void DRAWING_TOOL::setTransitions()
{
    Go( &DRAWING_TOOL::DrawLine,              PCB_ACTIONS::drawLine.MakeEvent() );
    Go( &DRAWING_TOOL::DrawZone,              PCB_ACTIONS::drawPolygon.MakeEvent() );
    Go( &DRAWING_TOOL::DrawCircle,            PCB_ACTIONS::drawCircle.MakeEvent() );
    Go( &DRAWING_TOOL::DrawArc,               PCB_ACTIONS::drawArc.MakeEvent() );
    Go( &DRAWING_TOOL::DrawDimension,         PCB_ACTIONS::drawDimension.MakeEvent() );
    Go( &DRAWING_TOOL::DrawZone,              PCB_ACTIONS::drawZone.MakeEvent() );
    Go( &DRAWING_TOOL::DrawZone,              PCB_ACTIONS::drawZoneKeepout.MakeEvent() );
    Go( &DRAWING_TOOL::DrawZone,              PCB_ACTIONS::drawZoneCutout.MakeEvent() );
    Go( &DRAWING_TOOL::DrawZone,              PCB_ACTIONS::drawSimilarZone.MakeEvent() );
    Go( &DRAWING_TOOL::DrawVia,               PCB_ACTIONS::drawVia.MakeEvent() );
    Go( &DRAWING_TOOL::PlaceText,             PCB_ACTIONS::placeText.MakeEvent() );
    Go( &DRAWING_TOOL::PlaceImportedGraphics, PCB_ACTIONS::placeImportedGraphics.MakeEvent() );
    Go( &DRAWING_TOOL::SetAnchor,             PCB_ACTIONS::setAnchor.MakeEvent() );
}
