/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "tools/ee_graphic_tool.h"

#include <wx/msgdlg.h>

#include <default_values.h>
#include <scoped_set_reset.h>

#include <dialogs/dialog_text_properties.h>
#include <eeschema_settings.h>
#include <import_gfx/dialog_import_gfx_sch.h>
#include <gal/graphics_abstraction_layer.h>
#include <sch_actions.h>
#include <sch_commit.h>
#include <sch_edit_frame.h>
#include <sch_shape.h>
#include <sch_textbox.h>
#include <schematic.h>
#include <symbol_edit_frame.h>
#include <symbol_editor/symbol_editor_settings.h>
#include <tool/arc_draw_behavior.h>
#include <tool/bezier_draw_behavior.h>
#include <tool/ellipse_draw_behavior.h>
#include <tools/ee_grid_helper.h>
#include <tools/sch_selection_tool.h>
#include <view/view_controls.h>
#include <view/view.h>


using SCOPED_DRAW_MODE = SCOPED_SET_RESET<EE_GRAPHIC_TOOL::MODE>;


EE_GRAPHIC_TOOL::EE_GRAPHIC_TOOL() :
        SCH_TOOL_BASE<SCH_BASE_FRAME>( "eeschema.InteractiveGraphicDrawing" ),
        m_lastFillStyle( FILL_T::NO_FILL ),
        m_lastFillColor( COLOR4D::UNSPECIFIED ),
        m_lastStroke( 0, LINE_STYLE::DEFAULT, COLOR4D::UNSPECIFIED ),
        m_lastTextboxFillStyle( FILL_T::NO_FILL ),
        m_lastTextboxFillColor( COLOR4D::UNSPECIFIED ),
        m_lastTextboxStroke( 0, LINE_STYLE::DEFAULT, COLOR4D::UNSPECIFIED ),
        m_lastTextBold( false ),
        m_lastTextItalic( false ),
        m_lastTextboxAngle( ANGLE_0 ),
        m_lastTextboxHJustify( GR_TEXT_H_ALIGN_LEFT ),
        m_lastTextboxVJustify( GR_TEXT_V_ALIGN_TOP ),
        m_mode( MODE::NONE ),
        m_inDrawingTool( false )
{
}


bool EE_GRAPHIC_TOOL::Init()
{
    SCH_TOOL_BASE::Init();

    const auto inDrawingArc =
            [this]( const SELECTION& aSel )
            {
                return m_mode == MODE::ARC;
            };

    const auto canUndoPoint = [this]( const SELECTION& aSel )
            {
                return m_mode == MODE::ARC || m_mode == MODE::BEZIER || m_mode == MODE::ELLIPSE_ARC;
            };

    CONDITIONAL_MENU& ctxMenu = m_menu->GetMenu();

    // clang-format off
    ctxMenu.AddItem( ACTIONS::arcPosture,          inDrawingArc,    200 );
    ctxMenu.AddItem( SCH_ACTIONS::deleteLastPoint, canUndoPoint,    200 );
    // clang-format on

    return true;
}


SCH_LAYER_ID EE_GRAPHIC_TOOL::getShapeLayer() const
{
    return IsSymbolEditor() ? LAYER_DEVICE : LAYER_NOTES;
}


int EE_GRAPHIC_TOOL::getDefaultTextSize() const
{
    if( IsSymbolEditor() )
    {
        const SYMBOL_EDITOR_SETTINGS* cfg = frame<SYMBOL_EDIT_FRAME>()->libeditconfig();

        return schIUScale.MilsToIU( cfg ? cfg->m_Defaults.text_size : DEFAULT_TEXT_SIZE );
    }

    return getModel<SCHEMATIC>()->Settings().m_DefaultTextSize;
}


void EE_GRAPHIC_TOOL::applySymbolEditorFlags( SCH_ITEM& aItem ) const
{
    if( !IsSymbolEditor() )
        return;

    SYMBOL_EDIT_FRAME* symFrame = frame<SYMBOL_EDIT_FRAME>();

    if( symFrame->GetDrawSpecificUnit() )
        aItem.SetUnit( symFrame->GetUnit() );

    if( symFrame->GetDrawSpecificBodyStyle() )
        aItem.SetBodyStyle( symFrame->GetBodyStyle() );
}


void EE_GRAPHIC_TOOL::commitItem( SCH_COMMIT& aCommit, std::unique_ptr<SCH_ITEM> aItem, const wxString& aDescription )
{
    aItem->ClearEditFlags();

    if( IsSymbolEditor() )
    {
        SYMBOL_EDIT_FRAME* symFrame = frame<SYMBOL_EDIT_FRAME>();
        LIB_SYMBOL*        symbol = symFrame->GetCurSymbol();

        aCommit.Modify( symbol, frame()->GetScreen() );
        symbol->AddDrawItem( aItem.release() );
        aCommit.Push( aDescription );
        symFrame->RebuildView();
    }
    else
    {
        aCommit.Add( aItem.release(), frame()->GetScreen() );
        aCommit.Push( aDescription );
    }
}


int EE_GRAPHIC_TOOL::DrawShape( const TOOL_EVENT& aEvent )
{
    EDA_ITEM* parent = getDrawParent();

    if( !parent )
        return 0;

    int        defaultTextSize = getDefaultTextSize();
    bool       isTextBox = aEvent.IsAction( &SCH_ACTIONS::drawTextBox )
                        || aEvent.IsAction( &SCH_ACTIONS::drawSymbolTextBox );
    SHAPE_T    type = isTextBox ? SHAPE_T::RECTANGLE : aEvent.Parameter<SHAPE_T>();

    if( m_inDrawingTool )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawingTool );

    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    EE_GRID_HELPER        grid( m_toolMgr );
    VECTOR2I              cursorPos;

    std::unique_ptr<SCH_SHAPE> item;

    // We might be running as the same shape in another co-routine.  Make sure that one
    // gets whacked.
    m_toolMgr->DeactivateTool();

    m_toolMgr->RunAction( ACTIONS::selectionClear );

    frame()->PushTool( aEvent );

    auto setCursor =
            [&]()
            {
                frame()->GetCanvas()->SetCurrentCursor( KICURSOR::PENCIL );
            };

    auto cleanup =
            [&] ()
            {
                m_toolMgr->RunAction( ACTIONS::selectionClear );
                m_view->ClearPreview();
                item.reset();
            };

    Activate();

    // Must be done after Activate() so that it gets set into the correct context
    getViewControls()->ShowCursor( true );

    // Set initial cursor
    setCursor();

    if( aEvent.HasPosition() )
        m_toolMgr->PrimeTool( aEvent.Position() );

    SCH_LAYER_ID shapeLayer = getShapeLayer();

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );

        cursorPos = grid.Align( controls->GetMousePosition(), GRID_HELPER_GRIDS::GRID_GRAPHICS );
        controls->ForceCursorPosition( true, cursorPos );

        // The tool hotkey is interpreted as a click when drawing
        bool isSyntheticClick = item && evt->IsActivate() && evt->HasPosition() && evt->Matches( aEvent );

        if( evt->IsCancelInteractive() || ( item && evt->IsAction( &ACTIONS::undo ) ) )
        {
            if( item )
            {
                cleanup();
            }
            else
            {
                frame()->PopTool( aEvent );
                break;
            }
        }
        else if( evt->IsActivate() && !isSyntheticClick )
        {
            if( item && evt->IsMoveTool() )
            {
                // we're already drawing our own item; ignore the move tool
                evt->SetPassEvent( false );
                continue;
            }

            if( item )
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
                frame()->PopTool( aEvent );
                break;
            }
        }
        else if( !item && (   evt->IsClick( BUT_LEFT )
                           || evt->IsAction( &ACTIONS::cursorClick ) ) )
        {
            m_toolMgr->RunAction( ACTIONS::selectionClear );

            if( isTextBox )
            {
                auto textbox = std::make_unique<SCH_TEXTBOX>( shapeLayer, 0, m_lastTextboxFillStyle );

                textbox->SetTextSize( VECTOR2I( defaultTextSize, defaultTextSize ) );

                // Must come after SetTextSize()
                textbox->SetBold( m_lastTextBold );
                textbox->SetItalic( m_lastTextItalic );

                textbox->SetTextAngle( m_lastTextboxAngle );
                textbox->SetHorizJustify( m_lastTextboxHJustify );
                textbox->SetVertJustify( m_lastTextboxVJustify );
                textbox->SetStroke( m_lastTextboxStroke );
                textbox->SetFillColor( m_lastTextboxFillColor );
                textbox->SetParent( parent );

                item = std::move( textbox );
            }
            else
            {
                item = std::make_unique<SCH_SHAPE>( type, shapeLayer, 0, m_lastFillStyle );

                item->SetStroke( m_lastStroke );
                item->SetFillColor( m_lastFillColor );
                item->SetParent( parent );
            }

            item->SetFlags( IS_NEW );

            applySymbolEditorFlags( *item );

            item->BeginEdit( cursorPos );

            m_view->ClearPreview();
            m_view->AddToPreview( item->Clone() );
        }
        else if( item && (   evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT )
                          || isSyntheticClick
                          || evt->IsAction( &ACTIONS::cursorClick ) || evt->IsAction( &ACTIONS::cursorDblClick )
                          || evt->IsAction( &ACTIONS::finishInteractive ) ) )
        {
            bool finished = false;

            if( evt->IsDblClick( BUT_LEFT )
                    || evt->IsAction( &ACTIONS::cursorDblClick )
                    || evt->IsAction( &ACTIONS::finishInteractive ) )
            {
                finished = true;
            }
            else
            {
                finished = !item->ContinueEdit( cursorPos );
            }

            if( finished )
            {
                item->EndEdit();
                item->SetFlags( IS_NEW );

                if( isTextBox )
                {
                    SCH_TEXTBOX*           textbox = static_cast<SCH_TEXTBOX*>( item.get() );
                    DIALOG_TEXT_PROPERTIES dlg( frame(), textbox );

                    getViewControls()->SetAutoPan( false );
                    getViewControls()->CaptureCursor( false );

                    // QuasiModal required for syntax help and Scintilla auto-complete
                    if( dlg.ShowQuasiModal() != wxID_OK )
                    {
                        cleanup();
                        continue;
                    }

                    m_lastTextBold = textbox->IsBold();
                    m_lastTextItalic = textbox->IsItalic();
                    m_lastTextboxAngle = textbox->GetTextAngle();
                    m_lastTextboxHJustify = textbox->GetHorizJustify();
                    m_lastTextboxVJustify = textbox->GetVertJustify();
                    m_lastTextboxStroke = textbox->GetStroke();
                    m_lastTextboxFillStyle = textbox->GetFillMode();
                    m_lastTextboxFillColor = textbox->GetFillColor();
                }
                else
                {
                    m_lastStroke = item->GetStroke();
                    m_lastFillStyle = item->GetFillMode();
                    m_lastFillColor = item->GetFillColor();
                }

                m_selectionTool->AddItemToSel( item.get() );

                SCH_COMMIT commit( m_toolMgr );
                commitItem( commit, std::move( item ), wxString::Format( _( "Draw %s" ), item->GetClass() ) );

                item = nullptr;

                m_view->ClearPreview();
                m_toolMgr->PostAction( ACTIONS::activatePointEditor );
            }
        }
        else if( evt->IsAction( &ACTIONS::duplicate )
                || evt->IsAction( &SCH_ACTIONS::repeatDrawItem )
                || evt->IsAction( &ACTIONS::paste ) )
        {
            if( item )
            {
                wxBell();
                continue;
            }

            // Exit.  The duplicate/repeat/paste will run in its own loop.
            frame()->PopTool( aEvent );
            evt->SetPassEvent();
            break;
        }
        else if( item && ( evt->IsAction( &ACTIONS::refreshPreview ) || evt->IsMotion() ) )
        {
            item->CalcEdit( cursorPos );
            m_view->ClearPreview();
            m_view->AddToPreview( item->Clone() );

            frame()->SetMsgPanel( item.get() );
        }
        else if( evt->IsDblClick( BUT_LEFT ) && !item )
        {
            m_toolMgr->RunAction( SCH_ACTIONS::properties );
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // Warp after context menu only if dragging...
            if( !item )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu->ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else if( item && evt->IsAction( &ACTIONS::redo ) )
        {
            wxBell();
        }
        else
        {
            evt->SetPassEvent();
        }

        // Enable autopanning and cursor capture only when there is a shape being drawn
        getViewControls()->SetAutoPan( item != nullptr );
        getViewControls()->CaptureCursor( item != nullptr );
    }

    getViewControls()->SetAutoPan( false );
    getViewControls()->CaptureCursor( false );
    frame()->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    return 0;
}


int EE_GRAPHIC_TOOL::DrawArc( const TOOL_EVENT& aEvent )
{
    if( m_inDrawingTool )
        return 0;

    EDA_ITEM* parent = getDrawParent();

    if( !parent )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawingTool );
    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::ARC );

    SCH_LAYER_ID          shapeLayer = getShapeLayer();
    std::vector<VECTOR2D> initialPts;

    const auto makeNewArc =
            [&]()
            {
                std::unique_ptr<SCH_SHAPE> arc = std::make_unique<SCH_SHAPE>( SHAPE_T::ARC, shapeLayer, 0, m_lastFillStyle );
                arc->SetStroke( m_lastStroke );
                arc->SetFillColor( m_lastFillColor );
                arc->SetParent( parent );
                arc->SetFlags( IS_NEW );

                applySymbolEditorFlags( *arc );

                return arc;
            };

    std::unique_ptr<SCH_SHAPE> arc = makeNewArc();

    arc->SetFlags( IS_NEW );

    m_frame->PushTool( aEvent );
    Activate();

    if( aEvent.HasPosition() )
        initialPts.push_back( aEvent.Position() );

    ARC_DRAW_BEHAVIOR arcBehavior( schIUScale, frame()->GetUserUnits() );

    while( drawManagedShape( aEvent, arc, arcBehavior, initialPts ) )
    {
        if( arc )
        {
            m_lastStroke = arc->GetStroke();
            m_lastFillStyle = arc->GetFillMode();
            m_lastFillColor = arc->GetFillColor();

            m_selectionTool->AddItemToSel( arc.get() );

            SCH_COMMIT commit( m_toolMgr );
            commitItem( commit, std::move( arc ), _( "Draw Arc" ) );

            m_toolMgr->PostAction( ACTIONS::activatePointEditor );
        }

        arc = makeNewArc();
        initialPts.clear();
    }

    return 0;
}


int EE_GRAPHIC_TOOL::DrawEllipseArc( const TOOL_EVENT& aEvent )
{
    if( m_inDrawingTool )
        return 0;

    EDA_ITEM* parent = getDrawParent();

    if( !parent )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawingTool );
    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::ELLIPSE_ARC );

    SCH_LAYER_ID          shapeLayer = getShapeLayer();
    std::vector<VECTOR2D> initialPts;

    const auto makeNewEllipseArc =
            [&]()
            {
                std::unique_ptr<SCH_SHAPE> arc = std::make_unique<SCH_SHAPE>(
                    SHAPE_T::ELLIPSE_ARC, shapeLayer, 0, m_lastFillStyle );
                arc->SetStroke( m_lastStroke );
                arc->SetFillColor( m_lastFillColor );
                arc->SetParent( parent );
                arc->SetFlags( IS_NEW );

                applySymbolEditorFlags( *arc );

                return arc;
            };

    std::unique_ptr<SCH_SHAPE> arc = makeNewEllipseArc();

    m_frame->PushTool( aEvent );
    Activate();

    if( aEvent.HasPosition() )
        initialPts.push_back( aEvent.Position() );

    ELLIPSE_ARC_DRAW_BEHAVIOR ellipseBehavior( schIUScale, frame()->GetUserUnits() );

    while( drawManagedShape( aEvent, arc, ellipseBehavior, initialPts ) )
    {
        if( arc )
        {
            m_lastStroke = arc->GetStroke();
            m_lastFillStyle = arc->GetFillMode();
            m_lastFillColor = arc->GetFillColor();

            m_selectionTool->AddItemToSel( arc.get() );

            SCH_COMMIT commit( m_toolMgr );
            commitItem( commit, std::move( arc ), _( "Draw Elliptical Arc" ) );

            m_toolMgr->PostAction( ACTIONS::activatePointEditor );
        }

        arc = makeNewEllipseArc();
        initialPts.clear();
    }

    return 0;
}


int EE_GRAPHIC_TOOL::DrawBezier( const TOOL_EVENT& aEvent )
{
    if( m_inDrawingTool )
        return 0;

    EDA_ITEM* parent = getDrawParent();

    if( !parent )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawingTool );
    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::BEZIER );

    SCH_LAYER_ID          shapeLayer = getShapeLayer();
    std::vector<VECTOR2D> initialPts;

    const auto makeNewBezier =
            [&]()
            {
                std::unique_ptr<SCH_SHAPE> bezier = std::make_unique<SCH_SHAPE>(
                    SHAPE_T::BEZIER, shapeLayer, 0, m_lastFillStyle );
                bezier->SetStroke( m_lastStroke );
                bezier->SetFillColor( m_lastFillColor );
                bezier->SetParent( parent );
                bezier->SetFlags( IS_NEW );

                applySymbolEditorFlags( *bezier );

                return bezier;
            };

    std::unique_ptr<SCH_SHAPE> bezier = makeNewBezier();

    m_frame->PushTool( aEvent );
    Activate();

    if( aEvent.HasPosition() )
        initialPts.push_back( aEvent.Position() );

    BEZIER_DRAW_BEHAVIOR bezierBehavior( schIUScale, frame()->GetUserUnits() );

    while( drawManagedShape( aEvent, bezier, bezierBehavior, initialPts ) )
    {
        if( bezier )
        {
            m_lastStroke = bezier->GetStroke();
            m_lastFillStyle = bezier->GetFillMode();
            m_lastFillColor = bezier->GetFillColor();

            // Chain: next bezier starts at the end of this one
            initialPts.clear();
            initialPts.push_back( bezier->GetEnd() );

            // If the last control arm is non-zero, mirror it for tangent continuity
            if( bezier->GetEnd() != bezier->GetBezierC2() )
            {
                VECTOR2D mirroredC1 = bezier->GetEnd() - ( bezier->GetBezierC2() - bezier->GetEnd() );
                initialPts.push_back( mirroredC1 );
            }

            m_selectionTool->AddItemToSel( bezier.get() );

            SCH_COMMIT commit( m_toolMgr );
            commitItem( commit, std::move( bezier ), _( "Draw Bezier" ) );

            m_toolMgr->PostAction( ACTIONS::activatePointEditor );
        }
        else
        {
            initialPts.clear();
        }

        bezier = makeNewBezier();
    }

    return 0;
}


bool EE_GRAPHIC_TOOL::drawManagedShape( const TOOL_EVENT& aTool, std::unique_ptr<SCH_SHAPE>& aShape,
                                        SHAPE_DRAW_BEHAVIOR& aBehavior,
                                        const std::vector<VECTOR2D>& aInitialPts )
{
    if( !aShape )
        return false;

    aBehavior.Reset();

    SELECTION             preview;
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    EE_GRID_HELPER        grid( m_toolMgr );
    VECTOR2I              cursorPos;
    EDA_ITEM*             parent = getDrawParent();

    m_view->Add( &preview );
    m_view->Add( &aBehavior.GetAssistant() );

    auto setCursor =
            [&]()
            {
                frame()->GetCanvas()->SetCurrentCursor( KICURSOR::PENCIL );
            };

    auto cleanup =
            [&]()
            {
                m_toolMgr->RunAction( ACTIONS::selectionClear );
                preview.Clear();
                aShape.reset();
            };

    controls->ShowCursor( true );
    setCursor();

    bool started = false;
    bool cancelled = false;

    m_toolMgr->PostAction( ACTIONS::refreshPreview );

    // Pre-load any initial points into the behaviour, advancing the construction
    // state machine so that the next user click adds the subsequent point.
    // The user can still undo these points if they want to change them.
    for( const VECTOR2D& pt : aInitialPts )
        aBehavior.AddPoint( pt );

    if( !aInitialPts.empty() )
    {
        m_toolMgr->RunAction( ACTIONS::selectionClear );

        controls->SetAutoPan( true );
        controls->CaptureCursor( true );

        preview.Add( aShape.get() );
        frame()->SetMsgPanel( aShape.get() );

        m_toolMgr->PrimeTool( aInitialPts.back() );

        started = true;
    }

    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );

        cursorPos = grid.Align( controls->GetMousePosition(), GRID_HELPER_GRIDS::GRID_GRAPHICS );
        controls->ForceCursorPosition( true, cursorPos );

        if( evt->IsCancelInteractive() )
        {
            cleanup();

            if( !started )
            {
                evt->SetPassEvent( false );
                m_frame->PopTool( aTool );
                cancelled = true;
            }

            break;
        }
        else if( started && evt->IsAction( &ACTIONS::undo ) )
        {
            cleanup();
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
                cleanup();
                cancelled = true;
                break;
            }
            else
            {
                cleanup();
                m_frame->PopTool( aTool );
                cancelled = true;
                break;
            }
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( !started )
            {
                m_toolMgr->RunAction( ACTIONS::selectionClear );

                controls->SetAutoPan( true );
                controls->CaptureCursor( true );

                preview.Add( aShape.get() );
                frame()->SetMsgPanel( aShape.get() );
                started = true;
            }

            aBehavior.AddPoint( cursorPos );
        }
        else if( evt->IsAction( &ACTIONS::arcPosture ) )
        {
            aBehavior.ToggleClockwise();
        }
        else if( evt->IsAction( &SCH_ACTIONS::deleteLastPoint ) )
        {
            aBehavior.RemoveLastPoint();
            grid.FullReset();
        }
        else if( evt->IsMotion() )
        {
            aBehavior.SetCursorPosition( cursorPos );
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            if( !aShape )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu->ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else if( evt->IsAction( &ACTIONS::updateUnits ) )
        {
            aBehavior.SetUnits( frame()->GetUserUnits() );
            m_view->Update( &aBehavior.GetAssistant() );
            evt->SetPassEvent();
        }
        else if( started && evt->IsAction( &ACTIONS::redo ) )
        {
            wxBell();
        }
        else
        {
            evt->SetPassEvent();
        }

        if( aBehavior.IsComplete() )
        {
            break;
        }
        else if( aBehavior.HasGeometryChanged() )
        {
            aBehavior.ApplyToShape( *aShape );
            m_view->Update( &preview );
            m_view->Update( &aBehavior.GetAssistant() );
            aBehavior.ClearGeometryChanged();

            if( started )
                frame()->SetMsgPanel( aShape.get() );
            else
                frame()->SetMsgPanel( parent );
        }
    }

    preview.Remove( aShape.get() );
    m_view->Remove( &aBehavior.GetAssistant() );
    m_view->Remove( &preview );

    if( !started )
        frame()->SetMsgPanel( parent );

    controls->SetAutoPan( false );
    controls->CaptureCursor( false );
    controls->ForceCursorPosition( false );
    frame()->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );

    if( cancelled )
    {
        aShape.reset();
    }

    return !cancelled;
}


int EE_GRAPHIC_TOOL::ImportGraphics( const TOOL_EVENT& aEvent )
{
    if( m_inDrawingTool )
        return 0;

    EDA_ITEM* parent = getDrawParent();

    if( !parent )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawingTool );

    DIALOG_IMPORT_GFX_SCH dlg( frame() );

    // Set filename on drag-and-drop
    if( aEvent.HasParameter() )
        dlg.SetFilenameOverride( *aEvent.Parameter<wxString*>() );

    int dlgResult = dlg.ShowModal();

    std::list<std::unique_ptr<EDA_ITEM>>& list = dlg.GetImportedItems();

    if( dlgResult != wxID_OK )
        return 0;

    // Ensure the list is not empty:
    if( list.empty() )
    {
        wxMessageBox( _( "No graphic items found in file." ) );
        return 0;
    }

    m_toolMgr->RunAction( ACTIONS::cancelInteractive );

    KIGFX::VIEW_CONTROLS*  controls = getViewControls();
    std::vector<SCH_ITEM*> newItems;
    std::vector<SCH_ITEM*> selectedItems;
    SCH_SELECTION          preview;
    SCH_COMMIT             commit( m_toolMgr );

    auto commitImport =
            [&]( const std::vector<SCH_ITEM*>& aItems )
            {
                if( IsSymbolEditor() )
                {
                    LIB_SYMBOL* symbol = static_cast<LIB_SYMBOL*>( parent );
                    commit.Modify( symbol, frame()->GetScreen() );

                    for( SCH_ITEM* item : aItems )
                    {
                        symbol->AddDrawItem( item );
                        item->ClearEditFlags();
                    }
                }
                else
                {
                    for( SCH_ITEM* item : aItems )
                        commit.Add( item, frame()->GetScreen() );
                }

                commit.Push( _( "Import Graphic" ) );

                if( IsSymbolEditor() )
                    frame<SYMBOL_EDIT_FRAME>()->RebuildView();
            };

    for( std::unique_ptr<EDA_ITEM>& ptr : list )
    {
        SCH_ITEM* item = dynamic_cast<SCH_ITEM*>( ptr.get() );
        wxCHECK2_MSG( item, continue, wxString::Format( "Bad item type: ", ptr->Type() ) );

        newItems.push_back( item );
        selectedItems.push_back( item );
        preview.Add( item );

        ptr.release();
    }

    if( !dlg.IsPlacementInteractive() )
    {
        commitImport( newItems );
        return 0;
    }

    m_view->Add( &preview );

    m_toolMgr->RunAction( ACTIONS::selectionClear );

    EDA_ITEMS selItems( selectedItems.begin(), selectedItems.end() );
    m_toolMgr->RunAction<EDA_ITEMS*>( ACTIONS::selectItems, &selItems );

    frame()->PushTool( aEvent );

    auto setCursor =
            [&]()
            {
                frame()->GetCanvas()->SetCurrentCursor( KICURSOR::MOVING );
            };

    Activate();
    controls->ShowCursor( true );
    controls->ForceCursorPosition( false );
    setCursor();

    EE_GRID_HELPER grid( m_toolMgr );

    VECTOR2I cursorPos = controls->GetCursorPosition( !aEvent.DisableGridSnapping() );
    VECTOR2I delta = cursorPos;
    VECTOR2I currentOffset;

    for( SCH_ITEM* item : selectedItems )
        item->Move( delta );

    currentOffset += delta;
    m_view->Update( &preview );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();

        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );

        cursorPos = grid.Align( controls->GetMousePosition(), GRID_GRAPHICS );
        controls->ForceCursorPosition( true, cursorPos );

        if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            m_toolMgr->RunAction( ACTIONS::selectionClear );

            for( SCH_ITEM* item : newItems )
                delete item;

            break;
        }
        else if( evt->IsMotion() )
        {
            delta = cursorPos - currentOffset;

            for( SCH_ITEM* item : selectedItems )
                item->Move( delta );

            currentOffset += delta;
            m_view->Update( &preview );
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu->ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else if( evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT )
                || evt->IsAction( &ACTIONS::cursorClick ) || evt->IsAction( &ACTIONS::cursorDblClick ) )
        {
            commitImport( newItems );
            break;
        }
        else
        {
            evt->SetPassEvent();
        }
    }

    preview.Clear();
    m_view->Remove( &preview );

    frame()->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    controls->ForceCursorPosition( false );
    frame()->PopTool( aEvent );

    return 0;
}


void EE_GRAPHIC_TOOL::setTransitions()
{
    // clang-format off
    Go( &EE_GRAPHIC_TOOL::DrawShape,        SCH_ACTIONS::drawRectangle.MakeEvent() );
    Go( &EE_GRAPHIC_TOOL::DrawShape,        SCH_ACTIONS::drawCircle.MakeEvent() );
    Go( &EE_GRAPHIC_TOOL::DrawShape,        SCH_ACTIONS::drawEllipse.MakeEvent() );
    Go( &EE_GRAPHIC_TOOL::DrawEllipseArc,   SCH_ACTIONS::drawEllipseArc.MakeEvent() );
    Go( &EE_GRAPHIC_TOOL::DrawArc,          SCH_ACTIONS::drawArc.MakeEvent() );
    Go( &EE_GRAPHIC_TOOL::DrawBezier,       SCH_ACTIONS::drawBezier.MakeEvent() );
    Go( &EE_GRAPHIC_TOOL::DrawShape,        SCH_ACTIONS::drawTextBox.MakeEvent() );
    Go( &EE_GRAPHIC_TOOL::DrawShape,        SCH_ACTIONS::drawSymbolLines.MakeEvent() );
    Go( &EE_GRAPHIC_TOOL::DrawShape,        SCH_ACTIONS::drawSymbolPolygon.MakeEvent() );
    Go( &EE_GRAPHIC_TOOL::DrawShape,        SCH_ACTIONS::drawSymbolTextBox.MakeEvent() );
    Go( &EE_GRAPHIC_TOOL::ImportGraphics,   SCH_ACTIONS::importGraphics.MakeEvent() );
    Go( &EE_GRAPHIC_TOOL::ImportGraphics,   SCH_ACTIONS::ddImportGraphics.MakeEvent() );
    // clang-format on
}
