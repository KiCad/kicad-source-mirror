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
#include <dialogs/dialog_offset_item.h>
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


/**
 * Move each item in the selection by the given vector.
 *
 * If any pads are part of a footprint, the whole footprint is moved.
 */
static void moveSelectionBy( const PCB_SELECTION& aSelection, const VECTOR2I& aMoveVec,
                             BOARD_COMMIT& commit )
{
    for( EDA_ITEM* item : aSelection )
    {
        if( !item->IsBOARD_ITEM() )
            continue;

        BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( item );
        commit.Modify( boardItem, nullptr, RECURSE_MODE::RECURSE );
        boardItem->Move( aMoveVec );
    }
}


POSITION_RELATIVE_TOOL::POSITION_RELATIVE_TOOL() :
    PCB_TOOL_BASE( "pcbnew.PositionRelative" ),
    m_dialog( nullptr ),
    m_selectionTool( nullptr ),
    m_inInteractivePosition( false )
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
                sTool->FilterCollectorForFreePads( aCollector, false );
                sTool->FilterCollectorForLockedItems( aCollector );
            } );

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


int POSITION_RELATIVE_TOOL::InteractiveOffset( const TOOL_EVENT& aEvent )
{
    if( m_inInteractivePosition )
        return false;

    REENTRANCY_GUARD guard( &m_inInteractivePosition );

    // First, acquire the selection that we will be moving after we have the new offset vector.
    const PCB_SELECTION& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                sTool->FilterCollectorForHierarchy( aCollector, true );
                sTool->FilterCollectorForMarkers( aCollector );
                sTool->FilterCollectorForFreePads( aCollector, false );
                sTool->FilterCollectorForLockedItems( aCollector );
            } );

    if( selection.Empty() )
        return 0;

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
    ruler.SetShowEndArrowHead( true );

    view.Add( &ruler );
    view.SetVisible( &ruler, false );

    auto setCursor =
            [&]()
            {
                frame()->GetCanvas()->SetCurrentCursor( KICURSOR::MEASURE );
            };

    const auto setInitialMsg =
            [&]()
            {
                statusPopup.SetText( _( "Select the reference point on the item to move." ) );
            };

    const auto setDragMsg =
            [&]()
            {
                statusPopup.SetText( _( "Select the point to define the new offset from." ) );
            };

    const auto setPopupPosition =
            [&]()
            {
                statusPopup.Move( KIPLATFORM::UI::GetMousePosition() + wxPoint( 20, -50 ) );
            };

    auto cleanup =
            [&] ()
            {
                view.SetVisible( &ruler, false );
                controls.SetAutoPan( false );
                controls.CaptureCursor( false );
                controls.ForceCursorPosition( false );
                originSet = false;
                setInitialMsg();
            };

    const auto applyVector =
            [&]( const VECTOR2I& aMoveVec )
            {
                BOARD_COMMIT commit( frame() );
                moveSelectionBy( selection, aMoveVec, commit );
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
        setPopupPosition();

        if( !evt->IsActivate() && !evt->IsCancelInteractive() )
        {
            // If we are switching, the canvas may not be valid any more
            cursorPos = grid.BestSnapAnchor( cursorPos, nullptr );
            controls.ForceCursorPosition( true, cursorPos );
        }
        else
        {
            grid.FullReset();
        }

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
            // Hide the popup text so it doesn't get in the way
            statusPopup.Hide();

            // This is the forward vector from the ruler item
            VECTOR2I       offsetVector = twoPtMgr.GetEnd() - twoPtMgr.GetOrigin();
            const VECTOR2I toReferencePtVector = twoPtMgr.GetOrigin() - twoPtMgr.GetEnd();

            // Start with the value of that vector in the dialog (will match the rule HUD)
            DIALOG_OFFSET_ITEM dlg( *frame(), offsetVector );

            if( dlg.ShowModal() == wxID_OK )
            {
                const VECTOR2I move = toReferencePtVector + offsetVector;

                applyVector( move );

                // Leave the arrow in place but update it
                twoPtMgr.SetEnd( twoPtMgr.GetOrigin() + offsetVector );
                view.Update( &ruler, KIGFX::GEOMETRY );
                canvas()->Refresh();
            }

            originSet = false;

            setInitialMsg();

            controls.SetAutoPan( false );
            controls.CaptureCursor( false );

            statusPopup.Popup();
        }
        // move or drag when origin set updates rules
        else if( originSet && ( evt->IsMotion() || evt->IsDrag( BUT_LEFT ) ) )
        {
            auto snap = LEADER_MODE::DIRECT;

            if( frame()->IsType( FRAME_PCB_EDITOR ) )
                snap = GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" )->m_AngleSnapMode;
            else
                snap = GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" )->m_AngleSnapMode;

            twoPtMgr.SetAngleSnap( snap );
            // The end is fixed; we must update the origin
            twoPtMgr.SetOrigin( cursorPos );

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
        else if( !evt->IsMouseAction() )
        {
            // Often this will end up changing the items we just moved, so the ruler will be
            // in the wrong place. Clear it away and the user can restart
            twoPtMgr.Reset();
            view.SetVisible( &ruler, false );
            view.Update( &ruler, KIGFX::GEOMETRY );

            evt->SetPassEvent();
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
    moveSelectionBy( m_selection, aggregateTranslation, *m_commit );
    m_commit->Push( _( "Position Relative" ) );

    if( m_selection.IsHover() )
        m_toolMgr->RunAction( ACTIONS::selectionClear );

    m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );

    canvas()->Refresh();
    return 0;
}


void POSITION_RELATIVE_TOOL::setTransitions()
{
    Go( &POSITION_RELATIVE_TOOL::PositionRelative,  PCB_ACTIONS::positionRelative.MakeEvent() );
    Go( &POSITION_RELATIVE_TOOL::InteractiveOffset, PCB_ACTIONS::interactiveOffsetTool.MakeEvent() );
}
