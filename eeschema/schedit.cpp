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
#include <sch_edit_frame.h>
#include <hotkeys_basic.h>
#include <general.h>
#include <eeschema_id.h>
#include <sch_bus_entry.h>
#include <sch_view.h>
#include <tool/tool_manager.h>
#include <tools/sch_actions.h>
#include <tools/sch_selection_tool.h>

void SCH_EDIT_FRAME::Process_Special_Functions( wxCommandEvent& event )
{
    if( event.GetId() == ID_HIERARCHY )
    {
        wxPoint pos = wxGetMousePosition();
        pos.y += 20;

        SetNoToolSelected();
        InstallHierarchyFrame( pos );
        SetRepeatItem( NULL );
    }
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
