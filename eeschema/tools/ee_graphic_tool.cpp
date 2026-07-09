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

#include <scoped_set_reset.h>

#include <dialogs/dialog_text_properties.h>
#include <eeschema_settings.h>
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
                return m_mode == MODE::ARC;
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


void EE_GRAPHIC_TOOL::commitItem( SCH_COMMIT& aCommit, std::unique_ptr<SCH_ITEM> aItem, const wxString& aDescription )
{
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
    SCHEMATIC*          schematic = getModel<SCHEMATIC>();
    SCHEMATIC_SETTINGS& sch_settings = schematic->Settings();
    bool                isTextBox = aEvent.IsAction( &SCH_ACTIONS::drawTextBox );
    SHAPE_T             type = aEvent.Parameter<SHAPE_T>();
    wxString            description;

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

                textbox->SetTextSize( VECTOR2I( sch_settings.m_DefaultTextSize, sch_settings.m_DefaultTextSize ) );

                // Must come after SetTextSize()
                textbox->SetBold( m_lastTextBold );
                textbox->SetItalic( m_lastTextItalic );

                textbox->SetTextAngle( m_lastTextboxAngle );
                textbox->SetHorizJustify( m_lastTextboxHJustify );
                textbox->SetVertJustify( m_lastTextboxVJustify );
                textbox->SetStroke( m_lastTextboxStroke );
                textbox->SetFillColor( m_lastTextboxFillColor );
                textbox->SetParent( schematic );

                item = std::move( textbox );
                description = _( "Add Text Box" );
            }
            else
            {
                item = std::make_unique<SCH_SHAPE>( type, shapeLayer, 0, m_lastFillStyle );

                item->SetStroke( m_lastStroke );
                item->SetFillColor( m_lastFillColor );
                item->SetParent( schematic );
                description = wxString::Format( _( "Add %s" ), item->GetFriendlyName() );
            }

            item->SetFlags( IS_NEW );
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
                item->ClearEditFlags();
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

            if( type == SHAPE_T::ELLIPSE_ARC && item->GetEllipseMajorRadius() > 100
                && item->GetEllipseMinorRadius() > 100 )
            {
                const VECTOR2I  center = item->GetEllipseCenter();
                const double    a = item->GetEllipseMajorRadius();
                const double    b = item->GetEllipseMinorRadius();
                const EDA_ANGLE rot = item->GetEllipseRotation();
                const double    cosRot = rot.Cos();
                const double    sinRot = rot.Sin();

                const double dx = cursorPos.x - center.x;
                const double dy = cursorPos.y - center.y;
                const double lx = dx * cosRot + dy * sinRot;
                const double ly = -dx * sinRot + dy * cosRot;

                const EDA_ANGLE t( std::atan2( ly / b, lx / a ), RADIANS_T );
                const double    px = a * t.Cos();
                const double    py = b * t.Sin();

                VECTOR2I markerPos =
                        center + VECTOR2I( KiROUND( px * cosRot - py * sinRot ), KiROUND( px * sinRot + py * cosRot ) );

                SCH_SHAPE* dot = new SCH_SHAPE( SHAPE_T::CIRCLE, shapeLayer );
                int        radius = schIUScale.MilsToIU( 20 );
                dot->SetStart( markerPos );
                dot->SetEnd( markerPos + VECTOR2I( radius, 0 ) );
                dot->SetFillMode( FILL_T::FILLED_SHAPE );
                m_view->AddToPreview( dot );
            }

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

    SCHEMATIC* schematic = nullptr;

    if( !IsSymbolEditor() )
        schematic = getModel<SCHEMATIC>();

    REENTRANCY_GUARD guard( &m_inDrawingTool );
    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::ARC );

    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    EE_GRID_HELPER        grid( m_toolMgr );
    VECTOR2I              cursorPos;

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
            [&]()
            {
                m_toolMgr->RunAction( ACTIONS::selectionClear );
                m_view->ClearPreview();
            };

    Activate();

    // Must be done after Activate() so that it gets set into the correct context
    getViewControls()->ShowCursor( true );

    // Set initial cursor
    setCursor();

    ARC_DRAW_BEHAVIOR arcBehavior( schIUScale, frame()->GetUserUnits() );
    m_view->Add( &arcBehavior.GetAssistant() );

    SCH_LAYER_ID shapeLayer = getShapeLayer();

    // Create the shape up front, matching the PCB drawArc pattern.
    std::unique_ptr<SCH_SHAPE> arc = std::make_unique<SCH_SHAPE>( SHAPE_T::ARC, shapeLayer, 0, m_lastFillStyle );
    arc->SetStroke( m_lastStroke );
    arc->SetFillColor( m_lastFillColor );
    arc->SetParent( schematic );
    arc->SetFlags( IS_NEW );

    bool started = false;

    m_toolMgr->PostAction( ACTIONS::refreshPreview );

    if( aEvent.HasPosition() )
        m_toolMgr->PrimeTool( aEvent.Position() );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );

        cursorPos = grid.Align( controls->GetMousePosition(), GRID_HELPER_GRIDS::GRID_GRAPHICS );
        controls->ForceCursorPosition( true, cursorPos );

        if( evt->IsCancelInteractive() || ( started && evt->IsAction( &ACTIONS::undo ) ) )
        {
            if( !started )
            {
                // Consume the event so it does not propagate to other tools.
                evt->SetPassEvent( false );
                frame()->PopTool( aEvent );
            }
            else
            {
                cleanup();
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
                cleanup();
                // leave ourselves on the stack so we come back after the move
                break;
            }
            else
            {
                cleanup();
                frame()->PopTool( aEvent );
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

                m_view->ClearPreview();
                m_view->AddToPreview( arc->Clone() );
                frame()->SetMsgPanel( arc.get() );
                started = true;
            }

            arcBehavior.AddPoint( cursorPos );
        }
        else if( evt->IsAction( &ACTIONS::arcPosture ) )
        {
            arcBehavior.ToggleClockwise();
        }
        else if( evt->IsAction( &SCH_ACTIONS::deleteLastPoint ) )
        {
            arcBehavior.RemoveLastPoint();
            grid.FullReset();
        }
        else if( evt->IsMotion() )
        {
            arcBehavior.SetCursorPosition( cursorPos );
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            if( !started )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu->ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else if( evt->IsAction( &ACTIONS::updateUnits ) )
        {
            arcBehavior.SetUnits( frame()->GetUserUnits() );
            m_view->Update( &arcBehavior.GetAssistant() );
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

        // --- Geometry update and completion check (at bottom of loop) ---

        if( arcBehavior.IsComplete() )
        {
            // Apply the final geometry before committing.
            if( arcBehavior.HasGeometryChanged() )
            {
                arcBehavior.ApplyToShape( *arc );
                m_view->ClearPreview();
                m_view->AddToPreview( arc->Clone() );
                m_view->Update( &arcBehavior.GetAssistant() );
                arcBehavior.ClearGeometryChanged();
            }

            break;
        }
        else if( arcBehavior.HasGeometryChanged() )
        {
            arcBehavior.ApplyToShape( *arc );
            m_view->ClearPreview();
            m_view->AddToPreview( arc->Clone() );
            m_view->Update( &arcBehavior.GetAssistant() );
            arcBehavior.ClearGeometryChanged();

            if( started )
                frame()->SetMsgPanel( arc.get() );
            else
                frame()->SetMsgPanel( schematic );
        }
    }

    m_view->Remove( &arcBehavior.GetAssistant() );

    if( arcBehavior.IsComplete() )
    {
        m_lastStroke = arc->GetStroke();
        m_lastFillStyle = arc->GetFillMode();
        m_lastFillColor = arc->GetFillColor();

        SCH_SHAPE* committedArc = arc.get();

        SCH_COMMIT commit( m_toolMgr );
        commitItem( commit, std::move( arc ), _( "Draw Arc" ) );

        m_selectionTool->AddItemToSel( committedArc );

        m_view->ClearPreview();
        m_toolMgr->PostAction( ACTIONS::activatePointEditor );
    }
    else
    {
        m_view->ClearPreview();
    }

    if( !started )
        frame()->SetMsgPanel( schematic );

    controls->SetAutoPan( false );
    controls->CaptureCursor( false );
    controls->ForceCursorPosition( false );
    frame()->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );

    return 0;
}


void EE_GRAPHIC_TOOL::setTransitions()
{
    // clang-format off
    Go( &EE_GRAPHIC_TOOL::DrawShape,        SCH_ACTIONS::drawRectangle.MakeEvent() );
    Go( &EE_GRAPHIC_TOOL::DrawShape,        SCH_ACTIONS::drawCircle.MakeEvent() );
    Go( &EE_GRAPHIC_TOOL::DrawShape,        SCH_ACTIONS::drawEllipse.MakeEvent() );
    Go( &EE_GRAPHIC_TOOL::DrawShape,        SCH_ACTIONS::drawEllipseArc.MakeEvent() );
    Go( &EE_GRAPHIC_TOOL::DrawArc,          SCH_ACTIONS::drawArc.MakeEvent() );
    Go( &EE_GRAPHIC_TOOL::DrawShape,        SCH_ACTIONS::drawBezier.MakeEvent() );
    Go( &EE_GRAPHIC_TOOL::DrawShape,        SCH_ACTIONS::drawTextBox.MakeEvent() );
    // clang-format on
}
