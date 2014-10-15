/**
 *  @file libedit_onrightclick.cpp
 *  @brief Library editor: create the pop menus when clicking on mouse right button
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2014 KiCad Developers, see change_log.txt for contributors.
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
#include <confirm.h>
#include <eeschema_id.h>
#include <hotkeys.h>
#include <class_drawpanel.h>
#include <class_sch_screen.h>
#include <msgpanel.h>

#include <general.h>
#include <libeditframe.h>
#include <class_libentry.h>
#include <lib_pin.h>
#include <lib_polyline.h>
#include <menus_helpers.h>


/* functions to add commands and submenus depending on the item */
static void AddMenusForBlock( wxMenu* PopMenu, LIB_EDIT_FRAME* frame );
static void AddMenusForPin( wxMenu* PopMenu, LIB_PIN* Pin, LIB_EDIT_FRAME* frame );


bool LIB_EDIT_FRAME::OnRightClick( const wxPoint& aPosition, wxMenu* PopMenu )
{
    LIB_ITEM*   item = GetDrawItem();
    bool        blockActive = GetScreen()->IsBlockActive();

    if( blockActive )
    {
        AddMenusForBlock( PopMenu, this );
        PopMenu->AppendSeparator();
        return true;
    }

    LIB_PART*      part = GetCurPart();

    if( !part )
        return true;

    //  If Command in progress, put menu "cancel"
    if( item && item->InEditMode() )
    {
        AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_CANCEL_EDITING, _( "Cancel" ),
                     KiBitmap( cancel_xpm ) );
        PopMenu->AppendSeparator();
    }
    else
    {
        item = LocateItemUsingCursor( aPosition );

        // If the clarify item selection context menu is aborted, don't show the context menu.
        if( item == NULL && m_canvas->GetAbortRequest() )
        {
            m_canvas->SetAbortRequest( false );
            return false;
        }

        if( GetToolId() != ID_NO_TOOL_SELECTED )
        {
            // If a tool is active, put menu "end tool"
            AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_CANCEL_EDITING, _( "End Tool" ),
                         KiBitmap( cursor_xpm ) );
            PopMenu->AppendSeparator();
        }
    }

    if( item )
    {
        MSG_PANEL_ITEMS items;
        item->GetMsgPanelInfo( items );
        SetMsgPanel( items );
    }
    else
    {
        return true;
    }

    m_drawItem = item;
    bool not_edited = !item->InEditMode();
    wxString msg;

    switch( item->Type() )
    {
    case LIB_PIN_T:
        AddMenusForPin( PopMenu, (LIB_PIN*) item, this );
        break;

    case LIB_ARC_T:
        if( not_edited )
        {
            msg = AddHotkeyName( _( "Move Arc" ), g_Libedit_Hokeys_Descr,
                                 HK_LIBEDIT_MOVE_GRAPHIC_ITEM );
            AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST, msg,
                         KiBitmap( move_arc_xpm ) );
            msg = AddHotkeyName( _( "Drag Arc Size" ), g_Libedit_Hokeys_Descr, HK_DRAG );
            AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_MODIFY_ITEM, msg, KiBitmap( move_arc_xpm ) );
        }

        msg = AddHotkeyName( _( "Edit Arc Options" ), g_Libedit_Hokeys_Descr, HK_EDIT );
        AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_BODY_EDIT_ITEM, msg, KiBitmap( options_arc_xpm ) );

        if( not_edited )
        {
            msg = AddHotkeyName( _( "Delete Arc" ), g_Libedit_Hokeys_Descr, HK_DELETE );
            AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_DELETE_ITEM, msg, KiBitmap( delete_arc_xpm ) );
        }
        break;

    case LIB_CIRCLE_T:
        if( not_edited )
        {
            msg = AddHotkeyName( _( "Move Circle" ), g_Libedit_Hokeys_Descr,
                                 HK_LIBEDIT_MOVE_GRAPHIC_ITEM );
            AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST, msg,
                         KiBitmap( move_circle_xpm ) );
            msg = AddHotkeyName( _( "Drag Circle Outline" ), g_Libedit_Hokeys_Descr, HK_DRAG );
            AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_MODIFY_ITEM, msg,
                         KiBitmap( move_rectangle_xpm ) );
        }

        msg = AddHotkeyName( _( "Edit Circle Options" ), g_Libedit_Hokeys_Descr, HK_EDIT );
        AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_BODY_EDIT_ITEM, msg,
                     KiBitmap( options_circle_xpm ) );

        if( not_edited )
        {
            msg = AddHotkeyName( _( "Delete Circle" ), g_Libedit_Hokeys_Descr, HK_DELETE );
            AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_DELETE_ITEM, msg,
                         KiBitmap( delete_circle_xpm ) );
        }
        break;

    case LIB_RECTANGLE_T:
        if( not_edited )
        {
            msg = AddHotkeyName( _( "Move Rectangle" ), g_Libedit_Hokeys_Descr,
                                 HK_LIBEDIT_MOVE_GRAPHIC_ITEM );
            AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST, msg,
                         KiBitmap( move_rectangle_xpm ) );
        }

        msg = AddHotkeyName( _( "Edit Rectangle Options" ), g_Libedit_Hokeys_Descr, HK_EDIT );
        AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_BODY_EDIT_ITEM, msg,
                     KiBitmap( options_rectangle_xpm ) );

        if( not_edited )
        {
            msg = AddHotkeyName( _( "Drag Rectangle Edge" ), g_Libedit_Hokeys_Descr, HK_DRAG );
            AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_MODIFY_ITEM, msg,
                         KiBitmap( move_rectangle_xpm ) );
            msg = AddHotkeyName( _( "Delete Rectangle" ), g_Libedit_Hokeys_Descr, HK_DELETE );
            AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_DELETE_ITEM, msg,
                         KiBitmap( delete_rectangle_xpm ) );
        }

        break;

    case LIB_TEXT_T:
        if( not_edited )
        {
            msg = AddHotkeyName( _( "Move Text" ), g_Libedit_Hokeys_Descr,
                                 HK_LIBEDIT_MOVE_GRAPHIC_ITEM );
            AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST, msg,
                         KiBitmap( move_text_xpm ) );
        }

        msg = AddHotkeyName( _( "Edit Text" ), g_Libedit_Hokeys_Descr, HK_EDIT );
        AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_BODY_EDIT_ITEM, msg, KiBitmap( edit_text_xpm ) );

        msg = AddHotkeyName( _( "Rotate Text" ), g_Libedit_Hokeys_Descr, HK_ROTATE );
        AddMenuItem( PopMenu, ID_LIBEDIT_ROTATE_ITEM, msg, KiBitmap( edit_text_xpm ) );

        if( not_edited )
        {
            msg = AddHotkeyName( _( "Delete Text" ), g_Libedit_Hokeys_Descr, HK_DELETE );
            AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_DELETE_ITEM, msg, KiBitmap( delete_text_xpm ) );
        }
        break;

    case LIB_POLYLINE_T:
        if( not_edited )
        {
            msg = AddHotkeyName( _( "Move Line" ), g_Libedit_Hokeys_Descr,
                                 HK_LIBEDIT_MOVE_GRAPHIC_ITEM );
            AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST, msg,
                         KiBitmap( move_line_xpm ) );
            msg = AddHotkeyName( _( "Drag Edge Point" ), g_Libedit_Hokeys_Descr, HK_DRAG );
            AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_MODIFY_ITEM, msg, KiBitmap( move_line_xpm ) );
        }

        if( item->IsNew() )
        {
            AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_END_CREATE_ITEM, _( "Line End" ),
                         KiBitmap( checked_ok_xpm ) );
        }

        msg = AddHotkeyName( _( "Edit Line Options" ), g_Libedit_Hokeys_Descr, HK_EDIT );
        AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_BODY_EDIT_ITEM, msg,
                     KiBitmap( options_segment_xpm ) );

        if( not_edited )
        {
            msg = AddHotkeyName( _( "Delete Line " ), g_Libedit_Hokeys_Descr, HK_DELETE );
            AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_DELETE_ITEM, msg,
                         KiBitmap( delete_segment_xpm ) );
        }

        if( item->IsNew() )
        {
            if( ( (LIB_POLYLINE*) item )->GetCornerCount() > 2 )
            {
                msg = AddHotkeyName( _( "Delete Segment" ), g_Libedit_Hokeys_Descr, HK_DELETE );
                AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_DELETE_CURRENT_POLY_SEGMENT,
                             msg, KiBitmap( delete_segment_xpm ) );
            }
        }

        break;

    case LIB_FIELD_T:
        if( not_edited )
        {
            msg = AddHotkeyName( _( "Move Field" ), g_Libedit_Hokeys_Descr,
                                 HK_LIBEDIT_MOVE_GRAPHIC_ITEM );
            AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST, msg,
                         KiBitmap( move_field_xpm ) );
        }

        msg = AddHotkeyName( _( "Field Rotate" ), g_Libedit_Hokeys_Descr, HK_ROTATE );
        AddMenuItem( PopMenu, ID_LIBEDIT_ROTATE_ITEM, msg, KiBitmap( rotate_field_xpm ) );
        msg = AddHotkeyName( _( "Field Edit" ), g_Libedit_Hokeys_Descr, HK_EDIT );
        AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_FIELD_EDIT_ITEM, msg, KiBitmap( edit_text_xpm ) );
        break;


    default:
        wxFAIL_MSG( wxString::Format( wxT( "Unknown library item type %d" ),
                                      item->Type() ) );
        m_drawItem = NULL;
        break;
    }

    PopMenu->AppendSeparator();
    return true;
}

// Add menu items for pin edition
void AddMenusForPin( wxMenu* PopMenu, LIB_PIN* Pin, LIB_EDIT_FRAME* frame )
{
    bool selected    = Pin->IsSelected();
    bool not_in_move = !Pin->IsMoving();
    wxString msg;

    if( not_in_move )
    {
        msg = AddHotkeyName( _( "Move Pin " ), g_Libedit_Hokeys_Descr,
                                 HK_LIBEDIT_MOVE_GRAPHIC_ITEM );
        AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST, msg, KiBitmap( move_xpm ) );
    }

    msg = AddHotkeyName( _( "Edit Pin " ), g_Libedit_Hokeys_Descr, HK_EDIT);
    AddMenuItem( PopMenu, ID_LIBEDIT_EDIT_PIN, msg, KiBitmap( edit_xpm ) );

    msg = AddHotkeyName( _( "Rotate Pin " ), g_Libedit_Hokeys_Descr, HK_ROTATE );
    AddMenuItem( PopMenu, ID_LIBEDIT_ROTATE_ITEM, msg, KiBitmap( rotate_pin_xpm ) );

    if( not_in_move )
    {
        msg = AddHotkeyName( _( "Delete Pin " ), g_Libedit_Hokeys_Descr, HK_DELETE );
        AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_DELETE_ITEM, msg, KiBitmap( delete_pin_xpm ) );
    }

    wxMenu* global_pin_change = new wxMenu;
    AddMenuItem( PopMenu, global_pin_change,
                 ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_ITEM,
                 _( "Global" ), KiBitmap( pin_to_xpm ) );
    AddMenuItem( global_pin_change,
                 ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINSIZE_ITEM,
                 selected ? _( "Pin Size to selected pins" ) :
                 _( "Pin Size to Others" ), KiBitmap( pin_size_to_xpm ) );
    AddMenuItem( global_pin_change,
                 ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINNAMESIZE_ITEM,
                 selected ? _( "Pin Name Size to selected pin" ) :
                 _( "Pin Name Size to Others" ), KiBitmap( pin_name_to_xpm ) );
    AddMenuItem( global_pin_change,
                 ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINNUMSIZE_ITEM,
                 selected ? _( "Pin Num Size to selected pin" ) :
                 _( "Pin Num Size to Others" ), KiBitmap( pin_number_to_xpm ) );
}


/* Add menu commands for block */

void AddMenusForBlock( wxMenu* PopMenu, LIB_EDIT_FRAME* frame )
{
    AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_CANCEL_EDITING, _( "Cancel Block" ),
                 KiBitmap( cancel_xpm ) );

    if( frame->GetScreen()->m_BlockLocate.GetCommand() == BLOCK_MOVE )
        AddMenuItem( PopMenu, ID_POPUP_ZOOM_BLOCK,
                     _( "Zoom Block (drag middle mouse)" ),
                     KiBitmap( zoom_area_xpm ) );

    PopMenu->AppendSeparator();

    AddMenuItem( PopMenu, ID_POPUP_PLACE_BLOCK, _( "Place Block" ), KiBitmap( checked_ok_xpm ) );

    if( frame->GetScreen()->m_BlockLocate.GetCommand() == BLOCK_MOVE )
    {
        AddMenuItem( PopMenu, ID_POPUP_SELECT_ITEMS_BLOCK, _( "Select Items" ),
                     KiBitmap( green_xpm ) );
        AddMenuItem( PopMenu, ID_POPUP_COPY_BLOCK, _( "Copy Block" ), KiBitmap( copyblock_xpm ) );
        AddMenuItem( PopMenu, ID_POPUP_MIRROR_Y_BLOCK, _( "Mirror Block ||" ),
                     KiBitmap( mirror_h_xpm ) );
        AddMenuItem( PopMenu, ID_POPUP_MIRROR_X_BLOCK, _( "Mirror Block --" ),
                     KiBitmap( mirror_v_xpm ) );
        AddMenuItem( PopMenu, ID_POPUP_ROTATE_BLOCK, _( "Rotate Block ccw" ),
                     KiBitmap( rotate_ccw_xpm ) );
        AddMenuItem( PopMenu, ID_POPUP_DELETE_BLOCK, _( "Delete Block" ), KiBitmap( delete_xpm ) );
    }
}
