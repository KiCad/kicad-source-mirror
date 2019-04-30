 /*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2017 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <kiway.h>
#include <gr_basic.h>
#include <sch_draw_panel.h>
#include <confirm.h>
#include <eda_doc.h>
#include <sch_edit_frame.h>
#include <hotkeys_basic.h>
#include <general.h>
#include <eeschema_id.h>
#include <list_operations.h>
#include <class_library.h>
#include <sch_bus_entry.h>
#include <sch_marker.h>
#include <sch_component.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_view.h>
#include <simulation_cursors.h>
#include <tool/tool_manager.h>
#include <tools/sch_actions.h>
#include <tools/sch_selection_tool.h>

 void SCH_EDIT_FRAME::Process_Special_Functions( wxCommandEvent& event )
{
    int         id = event.GetId();
    wxPoint     pos;
    SCH_SCREEN* screen = GetScreen();
    SCH_ITEM*   item = screen->GetCurItem();

    pos = wxGetMousePosition();

    pos.y += 20;

    // If needed, stop the current command and deselect current tool
    switch( id )
    {
    case ID_POPUP_CANCEL_CURRENT_COMMAND:
    case ID_POPUP_SCH_CLEANUP_SHEET:
    case ID_POPUP_IMPORT_HLABEL_TO_SHEETPIN:
    case ID_POPUP_SCH_EDIT_CONVERT_CMP:
    case ID_POPUP_SCH_ADD_JUNCTION:
    case ID_POPUP_SCH_ADD_LABEL:
        /* At this point: Do nothing. these commands do not need to stop the
         * current command (mainly a block command) or reset the current state
         * They will be executed later, in next switch structure.
         */
        break;

    default:
        // Stop the current command and deselect the current tool
        SetNoToolSelected();
        break;
    }

    item = screen->GetCurItem();    // Can be modified by previous calls.

    switch( id )
    {
    case ID_HIERARCHY:
        InstallHierarchyFrame( pos );
        SetRepeatItem( NULL );
        break;

    case ID_POPUP_CANCEL_CURRENT_COMMAND:
        if( m_canvas->IsMouseCaptured() )
        {
            m_canvas->EndMouseCapture();
            SetToolID( GetToolId(), GetGalCanvas()->GetCurrentCursor(), wxEmptyString );
        }
        else
        {
            SetNoToolSelected();
        }
        break;

    case ID_POPUP_SCH_CLEANUP_SHEET:
        if( item != NULL && item->Type() == SCH_SHEET_T )
        {
            SCH_SHEET* sheet = (SCH_SHEET*) item;

            if( !sheet->HasUndefinedPins() )
            {
                DisplayInfoMessage( this, _( "There are no undefined labels in this sheet to clean up." ) );
                return;
            }

            if( !IsOK( this, _( "Do you wish to cleanup this sheet?" ) ) )
                return;

            /* Save sheet in undo list before cleaning up unreferenced hierarchical labels. */
            SaveCopyInUndoList( sheet, UR_CHANGED );
            sheet->CleanupSheet();
            SyncView();
            GetCanvas()->Refresh();
            OnModify();
        }
        break;

    case ID_POPUP_SCH_EDIT_CONVERT_CMP:
        // Ensure the struct is a component (could be a struct of a component, like Field, text..)
        if( item && item->Type() == SCH_COMPONENT_T )
        {
            m_canvas->MoveCursorToCrossHair();
            ConvertPart( (SCH_COMPONENT*) item );
        }
        break;

    default:        // Log error:
        wxFAIL_MSG( wxString::Format( "Cannot process command event ID %d", event.GetId() ) );
        break;
    }

    if( GetToolId() == ID_NO_TOOL_SELECTED )
        SetRepeatItem( NULL );
}


void SCH_EDIT_FRAME::OnUnfoldBus( wxCommandEvent& event )
{
    wxMenuItem* item = static_cast<wxMenuItem*>( event.GetEventUserData() );
    wxString net = item->GetItemLabelText();

    GetToolManager()->RunAction( SCH_ACTIONS::unfoldBus, true, &net );

    // Now that we have handled the chosen bus unfold, disconnect all  the events so they can be
    // recreated with updated data on the next unfold
    Unbind( wxEVT_COMMAND_MENU_SELECTED, &SCH_EDIT_FRAME::OnUnfoldBus, this );
}


void SCH_EDIT_FRAME::OnCancelCurrentCommand( wxCommandEvent& aEvent )
{
    SCH_SCREEN* screen = GetScreen();

    if( screen->IsBlockActive() )
    {
        GetCanvas()->SetCursor( (wxStockCursor) GetGalCanvas()->GetDefaultCursor() );
        screen->ClearBlockCommand();

        // Stop the current command (if any) but keep the current tool
        m_canvas->EndMouseCapture();
    }
    else
    {
        if( m_canvas->IsMouseCaptured() ) // Stop the current command but keep the current tool
            m_canvas->EndMouseCapture();
        else                              // Deselect current tool
            m_canvas->EndMouseCapture( ID_NO_TOOL_SELECTED, GetGalCanvas()->GetDefaultCursor() );
     }

     GetCanvas()->GetView()->ClearHiddenFlags();
     GetCanvas()->GetView()->ClearPreview();
     GetCanvas()->GetView()->ShowPreview( false );
}


void SCH_EDIT_FRAME::OnUpdateSelectTool( wxUpdateUIEvent& aEvent )
{
    if( aEvent.GetEventObject() == m_drawToolBar || aEvent.GetEventObject() == m_mainToolBar )
        aEvent.Check( GetToolId() == aEvent.GetId() );
}


void SCH_EDIT_FRAME::OnUnfoldBusHotkey( wxCommandEvent& aEvent )
{
    SCH_SELECTION_TOOL*     selTool = GetToolManager()->GetTool<SCH_SELECTION_TOOL>();
    EDA_HOTKEY_CLIENT_DATA* data = (EDA_HOTKEY_CLIENT_DATA*) aEvent.GetClientObject();
    SCH_ITEM*               item = GetScreen()->GetCurItem();

    wxCHECK_RET( data != NULL, wxT( "Invalid hot key client object." ) );

    if( item == NULL )
    {
        // If we didn't get here by a hot key, then something has gone wrong.
        if( aEvent.GetInt() == 0 )
            return;

        item = selTool->SelectPoint( data->GetPosition(), SCH_COLLECTOR::EditableItems );

        // Exit if no item found at the current location or the item is already being edited.
        if( item == NULL || item->GetEditFlags() != 0 )
            return;
    }

    if( item->Type() != SCH_LINE_T )
        return;

    wxMenu* bus_unfold_menu = GetUnfoldBusMenu( static_cast<SCH_LINE*>( item ) );

    if( bus_unfold_menu )
    {
        auto controls = GetCanvas()->GetViewControls();
        auto vmp = controls->GetMousePosition( false );
        wxPoint mouse_pos( (int) vmp.x, (int) vmp.y );

        GetGalCanvas()->PopupMenu( bus_unfold_menu, mouse_pos );
    }
}


wxMenu* SCH_EDIT_FRAME::GetUnfoldBusMenu( SCH_LINE* aBus )
{
    auto connection = aBus->Connection( *g_CurrentSheet );

    if( !connection ||  !connection->IsBus() || connection->Members().empty() )
        return nullptr;

    int idx = 0;
    wxMenu* bus_unfolding_menu = new wxMenu;

    for( const auto& member : connection->Members() )
    {
        int id = ID_POPUP_SCH_UNFOLD_BUS + ( idx++ );
        wxString name = member->Name( true );

        if( member->Type() == CONNECTION_BUS )
        {
            wxMenu* submenu = new wxMenu;
            bus_unfolding_menu->AppendSubMenu( submenu, _( name ) );

            for( const auto& sub_member : member->Members() )
            {
                id = ID_POPUP_SCH_UNFOLD_BUS + ( idx++ );

                submenu->Append( id, sub_member->Name( true ), wxEmptyString );

                // See comment in else clause below
                auto sub_item_clone = new wxMenuItem();
                sub_item_clone->SetItemLabel( sub_member->Name( true ) );

                Bind( wxEVT_COMMAND_MENU_SELECTED, &SCH_EDIT_FRAME::OnUnfoldBus, this, id, id,
                        sub_item_clone );
            }
        }
        else
        {
            bus_unfolding_menu->Append( id, name, wxEmptyString );

            // Because Bind() takes ownership of the user data item, we
            // make a new menu item here and set its label.  Why create a
            // menu item instead of just a wxString or something? Because
            // Bind() requires a pointer to wxObject rather than a void
            // pointer.  Maybe at some point I'll think of a better way...
            auto item_clone = new wxMenuItem();
            item_clone->SetItemLabel( name );

            Bind( wxEVT_COMMAND_MENU_SELECTED, &SCH_EDIT_FRAME::OnUnfoldBus, this, id, id,
                    item_clone );
        }
    }

    return bus_unfolding_menu;
}
