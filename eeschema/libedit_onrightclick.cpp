/**
 *  @file libedit_onrightclick.cpp
 */

/* , In library editor, create the pop menu when clicking on mouse right button
 */

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "macros.h"
#include "confirm.h"
#include "bitmaps.h"
#include "eeschema_id.h"
#include "hotkeys.h"
#include "class_drawpanel.h"
#include "class_sch_screen.h"

#include "general.h"
#include "protos.h"
#include "libeditframe.h"
#include "class_libentry.h"
#include "lib_pin.h"
#include "lib_polyline.h"


/* functions to add commands and submenus depending on the item */
static void AddMenusForBlock( wxMenu* PopMenu, LIB_EDIT_FRAME* frame );
static void AddMenusForPin( wxMenu* PopMenu, LIB_PIN* Pin, LIB_EDIT_FRAME* frame );


bool LIB_EDIT_FRAME::OnRightClick( const wxPoint& aPosition, wxMenu* PopMenu )
{
    LIB_ITEM* item = GetDrawItem();
    bool BlockActive = GetScreen()->IsBlockActive();

    if( BlockActive )
    {
        AddMenusForBlock( PopMenu, this );
        PopMenu->AppendSeparator();
        return true;
    }

    if( m_component == NULL )
        return true;

    //  If Command in progress, put menu "cancel"
    if( item && item->GetFlags() )
    {
        ADD_MENUITEM( PopMenu, ID_POPUP_LIBEDIT_CANCEL_EDITING, _( "Cancel" ), cancel_xpm );
        PopMenu->AppendSeparator();
    }
    else
    {
        item = LocateItemUsingCursor( aPosition );

        // If the clarify item selection context menu is aborted, don't show the context menu.
        if( item == NULL && DrawPanel->m_AbortRequest )
        {
            DrawPanel->m_AbortRequest = false;
            return false;
        }

        if( GetToolId() != ID_NO_TOOL_SELECTED )
        {
            // If a tool is active, put menu "end tool"
            ADD_MENUITEM( PopMenu, ID_POPUP_LIBEDIT_CANCEL_EDITING, _( "End Tool" ),
                          cancel_tool_xpm );
            PopMenu->AppendSeparator();
        }
    }

    if( item )
        item->DisplayInfo( this );
    else
        return true;

    m_drawItem = item;
    wxString msg;

    switch( item->Type() )
    {
    case LIB_PIN_T:
        AddMenusForPin( PopMenu, (LIB_PIN*) item, this );
        break;

    case LIB_ARC_T:
        if( item->GetFlags() == 0 )
        {
            msg = AddHotkeyName( _( "Move Arc" ), s_Libedit_Hokeys_Descr,
                                 HK_LIBEDIT_MOVE_GRAPHIC_ITEM );
            ADD_MENUITEM( PopMenu, ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST, msg, move_arc_xpm );
            msg = AddHotkeyName( _( "Drag Arc Size" ), s_Libedit_Hokeys_Descr, HK_DRAG );
            ADD_MENUITEM( PopMenu, ID_POPUP_LIBEDIT_MODIFY_ITEM, msg, move_arc_xpm );
        }

        msg = AddHotkeyName( _( "Edit Arc Options" ), s_Libedit_Hokeys_Descr, HK_EDIT );
        ADD_MENUITEM( PopMenu, ID_POPUP_LIBEDIT_BODY_EDIT_ITEM, msg, options_arc_xpm );

        if( item->GetFlags() == 0 )
        {
            msg = AddHotkeyName( _( "Delete Arc" ), s_Libedit_Hokeys_Descr, HK_DELETE );
            ADD_MENUITEM( PopMenu, ID_POPUP_LIBEDIT_DELETE_ITEM, msg, delete_arc_xpm );
        }
        break;

    case LIB_CIRCLE_T:
        if( item->GetFlags() == 0 )
        {
            msg = AddHotkeyName( _( "Move Circle" ), s_Libedit_Hokeys_Descr,
                                 HK_LIBEDIT_MOVE_GRAPHIC_ITEM );
            ADD_MENUITEM( PopMenu, ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST, msg, move_circle_xpm );
        }

        if( item->GetFlags() == 0 )
        {
            msg = AddHotkeyName( _( "Drag Circle Outline" ), s_Libedit_Hokeys_Descr, HK_DRAG );
            ADD_MENUITEM( PopMenu, ID_POPUP_LIBEDIT_MODIFY_ITEM, msg, move_rectangle_xpm );
        }

        msg = AddHotkeyName( _( "Edit Circle Options" ), s_Libedit_Hokeys_Descr, HK_EDIT );
        ADD_MENUITEM( PopMenu, ID_POPUP_LIBEDIT_BODY_EDIT_ITEM, msg, options_circle_xpm );

        if( item->GetFlags() == 0 )
        {
            msg = AddHotkeyName( _( "Delete Circle" ), s_Libedit_Hokeys_Descr, HK_DELETE );
            ADD_MENUITEM( PopMenu, ID_POPUP_LIBEDIT_DELETE_ITEM, msg, delete_circle_xpm );
        }
        break;

    case LIB_RECTANGLE_T:
        if( item->GetFlags() == 0 )
        {
            msg = AddHotkeyName( _( "Move Rectangle" ), s_Libedit_Hokeys_Descr,
                                 HK_LIBEDIT_MOVE_GRAPHIC_ITEM );
            ADD_MENUITEM( PopMenu, ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST, msg, move_rectangle_xpm );
        }

        msg = AddHotkeyName( _( "Edit Rectangle Options" ), s_Libedit_Hokeys_Descr, HK_EDIT );
        ADD_MENUITEM( PopMenu, ID_POPUP_LIBEDIT_BODY_EDIT_ITEM, msg, options_rectangle_xpm );

        if( item->GetFlags() == 0 )
        {
            msg = AddHotkeyName( _( "Drag Rectangle Edge" ), s_Libedit_Hokeys_Descr, HK_DRAG );
            ADD_MENUITEM( PopMenu, ID_POPUP_LIBEDIT_MODIFY_ITEM, msg, move_rectangle_xpm );
        }

        if( item->GetFlags() == 0 )
        {
            msg = AddHotkeyName( _( "Delete Rectangle" ), s_Libedit_Hokeys_Descr, HK_DELETE );
            ADD_MENUITEM( PopMenu, ID_POPUP_LIBEDIT_DELETE_ITEM, msg, delete_rectangle_xpm );
        }

        break;

    case LIB_TEXT_T:
        if( item->GetFlags() == 0 )
        {
            msg = AddHotkeyName( _( "Move Text" ), s_Libedit_Hokeys_Descr,
                                 HK_LIBEDIT_MOVE_GRAPHIC_ITEM );
            ADD_MENUITEM( PopMenu, ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST, msg, move_text_xpm );
        }

        msg = AddHotkeyName( _( "Edit Text" ), s_Libedit_Hokeys_Descr, HK_EDIT );
        ADD_MENUITEM( PopMenu, ID_POPUP_LIBEDIT_BODY_EDIT_ITEM, msg, edit_text_xpm );

        msg = AddHotkeyName( _( "Rotate Text" ), s_Libedit_Hokeys_Descr, HK_ROTATE );
        ADD_MENUITEM( PopMenu, ID_LIBEDIT_ROTATE_ITEM, msg, edit_text_xpm );

        if( item->GetFlags() == 0 )
        {
            msg = AddHotkeyName( _( "Delete Text" ), s_Libedit_Hokeys_Descr, HK_DELETE );
            ADD_MENUITEM( PopMenu, ID_POPUP_LIBEDIT_DELETE_ITEM, msg, delete_text_xpm );
        }
        break;

    case LIB_POLYLINE_T:
        if( item->GetFlags() == 0 )
        {
            msg = AddHotkeyName( _( "Move Line" ), s_Libedit_Hokeys_Descr,
                                 HK_LIBEDIT_MOVE_GRAPHIC_ITEM );
            ADD_MENUITEM( PopMenu, ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST, msg, move_line_xpm );
            msg = AddHotkeyName( _( "Drag Edge Point" ), s_Libedit_Hokeys_Descr, HK_DRAG );
            ADD_MENUITEM( PopMenu, ID_POPUP_LIBEDIT_MODIFY_ITEM, msg, move_line_xpm );
        }

        if( item->IsNew() )
        {
            ADD_MENUITEM( PopMenu, ID_POPUP_LIBEDIT_END_CREATE_ITEM, _( "Line End" ), apply_xpm );
        }

        msg = AddHotkeyName( _( "Edit Line Options" ), s_Libedit_Hokeys_Descr, HK_EDIT );
        ADD_MENUITEM( PopMenu, ID_POPUP_LIBEDIT_BODY_EDIT_ITEM, msg, options_segment_xpm );

        if( item->GetFlags() == 0 )
        {
            msg = AddHotkeyName( _( "Delete Line " ), s_Libedit_Hokeys_Descr, HK_DELETE );
            ADD_MENUITEM( PopMenu, ID_POPUP_LIBEDIT_DELETE_ITEM, msg, delete_segment_xpm );
        }
        else if( item->IsNew() )
        {
            if( ( (LIB_POLYLINE*) item )->GetCornerCount() > 2 )
            {
                msg = AddHotkeyName( _( "Delete Segment" ), s_Libedit_Hokeys_Descr, HK_DELETE );
                ADD_MENUITEM( PopMenu, ID_POPUP_LIBEDIT_DELETE_CURRENT_POLY_SEGMENT,
                              msg, delete_segment_xpm );
            }
        }

        break;

    case LIB_FIELD_T:
        if( item->GetFlags() == 0 )
        {
            msg = AddHotkeyName( _( "Move Field" ), s_Libedit_Hokeys_Descr,
                                 HK_LIBEDIT_MOVE_GRAPHIC_ITEM );
            ADD_MENUITEM( PopMenu, ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST,  msg, move_field_xpm );
        }
        msg = AddHotkeyName( _( "Field Rotate" ), s_Libedit_Hokeys_Descr, HK_ROTATE );
        ADD_MENUITEM( PopMenu, ID_LIBEDIT_ROTATE_ITEM, msg, rotate_field_xpm );
        msg = AddHotkeyName( _( "Field Edit" ), s_Libedit_Hokeys_Descr, HK_EDIT );
        ADD_MENUITEM( PopMenu, ID_POPUP_LIBEDIT_FIELD_EDIT_ITEM, msg, edit_text_xpm );
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


void AddMenusForPin( wxMenu* PopMenu, LIB_PIN* Pin, LIB_EDIT_FRAME* frame )
{
    bool selected    = (Pin->m_Selected & IS_SELECTED) != 0;
    bool not_in_move = (Pin->GetFlags() == 0);
    wxString msg;

    if( not_in_move )
    {
        msg = AddHotkeyName( _( "Move Pin " ), s_Libedit_Hokeys_Descr,
                                 HK_LIBEDIT_MOVE_GRAPHIC_ITEM );
        ADD_MENUITEM( PopMenu, ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST, msg, move_xpm );
    }

    msg = AddHotkeyName( _( "Edit Pin " ), s_Libedit_Hokeys_Descr, HK_EDIT);
    ADD_MENUITEM( PopMenu, ID_LIBEDIT_EDIT_PIN, msg, edit_xpm );

    msg = AddHotkeyName( _( "Rotate Pin " ), s_Libedit_Hokeys_Descr, HK_ROTATE );
    ADD_MENUITEM( PopMenu, ID_LIBEDIT_ROTATE_ITEM, msg, rotate_pin_xpm );

    if( not_in_move )
    {
        msg = AddHotkeyName( _( "Delete Pin " ), s_Libedit_Hokeys_Descr, HK_DELETE );
        ADD_MENUITEM( PopMenu, ID_POPUP_LIBEDIT_DELETE_ITEM, msg, delete_pin_xpm );
    }

    wxMenu* global_pin_change = new wxMenu;
    ADD_MENUITEM_WITH_SUBMENU( PopMenu, global_pin_change,
                               ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_ITEM,
                               _( "Global" ), pin_to_xpm );
    ADD_MENUITEM( global_pin_change,
                  ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINSIZE_ITEM,
                  selected ? _( "Pin Size to selected pins" ) :
                  _( "Pin Size to Others" ), pin_size_to_xpm );
    ADD_MENUITEM( global_pin_change,
                  ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINNAMESIZE_ITEM,
                  selected ? _( "Pin Name Size to selected pin" ) :
                  _( "Pin Name Size to Others" ), pin_name_to_xpm );
    ADD_MENUITEM( global_pin_change,
                  ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINNUMSIZE_ITEM,
                  selected ? _( "Pin Num Size to selected pin" ) :
                  _( "Pin Num Size to Others" ), pin_number_to_xpm );
}


/* Add menu commands for block */

void AddMenusForBlock( wxMenu* PopMenu, LIB_EDIT_FRAME* frame )
{
    ADD_MENUITEM( PopMenu, ID_POPUP_LIBEDIT_CANCEL_EDITING, _( "Cancel Block" ), cancel_xpm );

    if( frame->GetScreen()->m_BlockLocate.m_Command == BLOCK_MOVE )
        ADD_MENUITEM( PopMenu, ID_POPUP_ZOOM_BLOCK,
                      _( "Zoom Block (drag middle mouse)" ),
                      zoom_area_xpm );

    PopMenu->AppendSeparator();

    ADD_MENUITEM( PopMenu, ID_POPUP_PLACE_BLOCK, _( "Place Block" ), apply_xpm );

    if( frame->GetScreen()->m_BlockLocate.m_Command == BLOCK_MOVE )
    {
        ADD_MENUITEM( PopMenu, ID_POPUP_SELECT_ITEMS_BLOCK, _( "Select Items" ), green_xpm );
        ADD_MENUITEM( PopMenu, ID_POPUP_COPY_BLOCK, _( "Copy Block" ), copyblock_xpm );
        ADD_MENUITEM( PopMenu, ID_POPUP_MIRROR_Y_BLOCK, _( "Mirror Block ||" ), mirror_H_xpm );
        ADD_MENUITEM( PopMenu, ID_POPUP_MIRROR_X_BLOCK, _( "Mirror Block --" ), mirror_V_xpm );
        ADD_MENUITEM( PopMenu, ID_POPUP_ROTATE_BLOCK, _( "Rotate Block ccw" ), rotate_pos_xpm );
        ADD_MENUITEM( PopMenu, ID_POPUP_DELETE_BLOCK, _( "Delete Block" ), delete_xpm );
    }
}
