/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "tools/position_relative_tool.h"

#include <board_commit.h>
#include <collectors.h>
#include <dialogs/dialog_position_relative.h>
#include <dialogs/dialog_set_offset.h>
#include <footprint.h>
#include <footprint_editor_settings.h>
#include <gal/graphics_abstraction_layer.h>
#include <kiplatform/ui.h>
#include <pad.h>
#include <pcb_group.h>
#include <preview_items/two_point_assistant.h>
#include <preview_items/two_point_geom_manager.h>
#include <pcb_painter.h>
#include <pgm_base.h>
#include <preview_items/ruler_item.h>
#include <render_settings.h>
#include <settings/settings_manager.h>
#include <status_popup.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_grid_helper.h>
#include <tools/pcb_selection_tool.h>
#include <tools/pcb_picker_tool.h>
#include <view/view_controls.h>


POSITION_RELATIVE_TOOL::POSITION_RELATIVE_TOOL() :
    PCB_TOOL_BASE( "pcbnew.PositionRelative" ),
    m_dialog( nullptr ),
    m_selectionTool( nullptr )
{
}


void POSITION_RELATIVE_TOOL::Reset( RESET_REASON aReason )
{
    if( aReason != RUN )
        m_commit = std::make_unique<BOARD_COMMIT>( this );
}


bool POSITION_RELATIVE_TOOL::Init()
{
    // Find the selection tool, so they can cooperate
    m_selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    return m_selectionTool != nullptr;
}


int POSITION_RELATIVE_TOOL::PositionRelative( const TOOL_EVENT& aEvent )
{
    PCB_BASE_FRAME* editFrame = getEditFrame<PCB_BASE_FRAME>();

    const auto& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                sTool->FilterCollectorForHierarchy( aCollector, true );
                sTool->FilterCollectorForMarkers( aCollector );
            },
            !m_isFootprintEditor /* prompt user regarding locked items */ );

    if( selection.Empty() )
        return 0;

    m_selection = selection;

    // We prefer footprints, then pads, then anything else here.
    EDA_ITEM* preferredItem = m_selection.GetTopLeftItem( true );

    if( !preferredItem && m_selection.HasType( PCB_PAD_T ) )
    {
        PCB_SELECTION padsOnly = m_selection;
        std::deque<EDA_ITEM*>& items = padsOnly.Items();
        items.erase( std::remove_if( items.begin(), items.end(),
                                     []( const EDA_ITEM* aItem )
                                     {
                                         return aItem->Type() != PCB_PAD_T;
                                     } ), items.end() );

        preferredItem = padsOnly.GetTopLeftItem();
    }

    if( preferredItem )
        m_selectionAnchor = preferredItem->GetPosition();
    else
        m_selectionAnchor = m_selection.GetTopLeftItem()->GetPosition();

    // The dialog is not modal and not deleted between calls.
    // It means some options can have changed since the last call.
    // Therefore we need to rebuild it in case UI units have changed since the last call.
    if( m_dialog && m_dialog->GetUserUnits() != editFrame->GetUserUnits() )
    {
        m_dialog->Destroy();
        m_dialog = nullptr;
    }

    if( !m_dialog )
        m_dialog = new DIALOG_POSITION_RELATIVE( editFrame );

    m_dialog->Show( true );

    return 0;
}


int POSITION_RELATIVE_TOOL::PositionRelativeInteractively( const TOOL_EVENT& aEvent )
{
    // First, acquire the selection that we will be moving after
    // we have the new offset vector.
    const auto& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                sTool->FilterCollectorForHierarchy( aCollector, true );
                sTool->FilterCollectorForMarkers( aCollector );
            },
            !m_isFootprintEditor /* prompt user regarding locked items */ );

    if( m_isFootprintEditor && !frame()->GetModel() )
        return 0;

    if( frame()->IsCurrentTool( ACTIONS::measureTool ) )
        return 0;

    auto& view = *getView();
    auto& controls = *getViewControls();

    frame()->PushTool( aEvent );

    bool invertXAxis = displayOptions().m_DisplayInvertXAxis;
    bool invertYAxis = displayOptions().m_DisplayInvertYAxis;

    if( m_isFootprintEditor )
    {
        invertXAxis = frame()->GetFootprintEditorSettings()->m_DisplayInvertXAxis;
        invertYAxis = frame()->GetFootprintEditorSettings()->m_DisplayInvertYAxis;
    }

    KIGFX::PREVIEW::TWO_POINT_GEOMETRY_MANAGER twoPtMgr;
    PCB_GRID_HELPER            grid( m_toolMgr, frame()->GetMagneticItemsSettings() );
    bool                       originSet = false;
    EDA_UNITS                  units = frame()->GetUserUnits();
    KIGFX::PREVIEW::RULER_ITEM ruler( twoPtMgr, pcbIUScale, units, invertXAxis, invertYAxis );
    STATUS_TEXT_POPUP          statusPopup( frame() );

    // Some colour to make it obviously not just a ruler
    ruler.SetColor( view.GetPainter()->GetSettings()->GetLayerColor( LAYER_ANCHOR ) );
    ruler.SetShowTicks( false );
    ruler.SetShowOriginArrowHead( true );

    view.Add( &ruler );
    view.SetVisible( &ruler, false );

    auto setCursor =
            [&]()
            {
                frame()->GetCanvas()->SetCurrentCursor( KICURSOR::MEASURE );
            };

    auto cleanup =
            [&] ()
            {
                view.SetVisible( &ruler, false );
                controls.SetAutoPan( false );
                controls.CaptureCursor( false );
                controls.ForceCursorPosition( false );
                originSet = false;
            };

    const auto applyVector = [&]( const VECTOR2I& aMoveVec )
    {
        BOARD_COMMIT commit( frame() );

        for( EDA_ITEM* item : selection )
        {
            if( !item->IsBOARD_ITEM() )
                continue;

            BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( item );

            // Don't move a pad by itself unless editing the footprint
            if( boardItem->Type() == PCB_PAD_T
                && !frame()->GetPcbNewSettings()->m_AllowFreePads
                && frame()->IsType( FRAME_PCB_EDITOR ) )
                continue;

            commit.Modify( boardItem );
            boardItem->Move( aMoveVec );
        }

        commit.Push( _( "Set Relative Position Interactively" ) );
    };

    Activate();
    // Must be done after Activate() so that it gets set into the correct context
    controls.ShowCursor( true );
    controls.SetAutoPan( false );
    controls.CaptureCursor( false );
    controls.ForceCursorPosition( false );

    // Set initial cursor
    setCursor();

    const auto setInitialMsg = [&]()
    {
        statusPopup.SetText( _( "Select the reference point on the item to move." ) );
    };

    const auto setDragMsg = [&]()
    {
        statusPopup.SetText( _( "Select the point to define the new offset from." ) );
    };

    const auto setPopupPosition = [&]()
    {
        statusPopup.Move( KIPLATFORM::UI::GetMousePosition() + wxPoint( 20, -50 ) );
    };

    setInitialMsg();

    setPopupPosition();
    statusPopup.Popup();
    canvas()->SetStatusPopup( statusPopup.GetPanel() );

    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( view.GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );
        VECTOR2I cursorPos = evt->HasPosition() ? evt->Position() : controls.GetMousePosition();
        cursorPos = grid.BestSnapAnchor( cursorPos, nullptr );
        controls.ForceCursorPosition( true, cursorPos );
        setPopupPosition();

        if( evt->IsCancelInteractive() )
        {
            if( originSet )
            {
                cleanup();
            }
            else
            {
                frame()->PopTool( aEvent );
                break;
            }
        }
        else if( evt->IsActivate() )
        {
            if( originSet )
                cleanup();

            frame()->PopTool( aEvent );
            break;
        }
        // click or drag starts
        else if( !originSet && ( evt->IsDrag( BUT_LEFT ) || evt->IsClick( BUT_LEFT ) ) )
        {
            twoPtMgr.SetOrigin( cursorPos );
            twoPtMgr.SetEnd( cursorPos );

            setDragMsg();

            controls.CaptureCursor( true );
            controls.SetAutoPan( true );

            originSet = true;
        }
        // second click or mouse up after drag ends
        else if( originSet && ( evt->IsClick( BUT_LEFT ) || evt->IsMouseUp( BUT_LEFT ) ) )
        {
            // The reverse vector, as (I think) it's clearer if the arrow points to
            // the thing that's going to move, and it's more natural to start drawing
            // at the item you just selected (which is the one that will move)
            const VECTOR2I    origVector = twoPtMgr.GetOrigin() - twoPtMgr.GetEnd();
            VECTOR2I          offsetVector = origVector;
            DIALOG_SET_OFFSET dlg( *frame(), offsetVector, false );

            int ret = dlg.ShowModal();

            if( ret == wxID_OK )
            {
                const VECTOR2I move = offsetVector - origVector;

                applyVector( move );

                // Leave the arrow in place but update it
                twoPtMgr.SetOrigin( twoPtMgr.GetOrigin() + move );
                view.Update( &ruler, KIGFX::GEOMETRY );
                canvas()->Refresh();
            }

            originSet = false;

            setInitialMsg();

            controls.SetAutoPan( false );
            controls.CaptureCursor( false );
        }
        // move or drag when origin set updates rules
        else if( originSet && ( evt->IsMotion() || evt->IsDrag( BUT_LEFT ) ) )
        {
            SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
            bool              force45Deg;

            if( frame()->IsType( FRAME_PCB_EDITOR ) )
                force45Deg = mgr.GetAppSettings<PCBNEW_SETTINGS>()->m_Use45DegreeLimit;
            else
                force45Deg = mgr.GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>()->m_Use45Limit;

            twoPtMgr.SetAngleSnap( force45Deg );
            twoPtMgr.SetEnd( cursorPos );

            view.SetVisible( &ruler, true );
            view.Update( &ruler, KIGFX::GEOMETRY );
        }
        else if( evt->IsAction( &ACTIONS::updateUnits ) )
        {
            if( frame()->GetUserUnits() != units )
            {
                units = frame()->GetUserUnits();
                ruler.SwitchUnits( units );
                view.Update( &ruler, KIGFX::GEOMETRY );
                canvas()->ForceRefresh();
            }

            evt->SetPassEvent();
        }
        else if( evt->IsAction( &ACTIONS::updatePreferences ) )
        {
            invertXAxis = displayOptions().m_DisplayInvertXAxis;
            invertYAxis = displayOptions().m_DisplayInvertYAxis;

            if( m_isFootprintEditor )
            {
                invertXAxis = frame()->GetFootprintEditorSettings()->m_DisplayInvertXAxis;
                invertYAxis = frame()->GetFootprintEditorSettings()->m_DisplayInvertYAxis;
            }

            ruler.UpdateDir( invertXAxis, invertYAxis );

            view.Update( &ruler, KIGFX::GEOMETRY );
            canvas()->Refresh();
            evt->SetPassEvent();
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // TODO: This does not work
            PCB_SELECTION    dummy;
            PCB_PICKER_TOOL* picker = m_toolMgr->GetTool<PCB_PICKER_TOOL>();
            picker->GetToolMenu().ShowContextMenu( dummy );
        }
        else
        {
            evt->SetPassEvent();
        }
    }

    view.SetVisible( &ruler, false );
    view.Remove( &ruler );

    frame()->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    controls.SetAutoPan( false );
    controls.CaptureCursor( false );
    controls.ForceCursorPosition( false );

    canvas()->SetStatusPopup( nullptr );
    return 0;
}


int POSITION_RELATIVE_TOOL::RelativeItemSelectionMove( const VECTOR2I& aPosAnchor,
                                                       const VECTOR2I& aTranslation )
{
    VECTOR2I aggregateTranslation = aPosAnchor + aTranslation - GetSelectionAnchorPosition();

    for( EDA_ITEM* item : m_selection )
    {
        if( !item->IsBOARD_ITEM() )
            continue;

        BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( item );

        // Don't move a pad by itself unless editing the footprint
        if( boardItem->Type() == PCB_PAD_T
            && !frame()->GetPcbNewSettings()->m_AllowFreePads
            && frame()->IsType( FRAME_PCB_EDITOR ) )
        {
            boardItem = boardItem->GetParent();
        }

        m_commit->Modify( boardItem );
        boardItem->Move( aggregateTranslation );
    }

    m_commit->Push( _( "Position Relative" ) );

    if( m_selection.IsHover() )
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear );

    m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );

    canvas()->Refresh();
    return 0;
}


void POSITION_RELATIVE_TOOL::setTransitions()
{
    // clang-format off
    Go( &POSITION_RELATIVE_TOOL::PositionRelative,              PCB_ACTIONS::positionRelative.MakeEvent() );
    Go( &POSITION_RELATIVE_TOOL::PositionRelativeInteractively, PCB_ACTIONS::positionRelativeInteractively.MakeEvent() );
    // clang-format on
}
