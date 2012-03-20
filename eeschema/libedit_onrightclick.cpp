/**
 *  @file libedit_onrightclick.cpp
 */

/* , In library editor, create the pop menu when clicking on mouse right button
 */

#include <fctsys.h>
#include <confirm.h>
#include <eeschema_id.h>
#include <hotkeys.h>
#include <class_drawpanel.h>
#include <class_sch_screen.h>

#include <general.h>
#include <protos.h>
#include <libeditframe.h>
#include <class_libentry.h>
#include <lib_pin.h>
#include <lib_polyline.h>


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
        AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_CANCEL_EDITING, _( "Cancel" ), KiBitmap( cancel_xpm ) );
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
            AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST, msg, KiBitmap( move_arc_xpm ) );
            msg = AddHotkeyName( _( "Drag Arc Size" ), s_Libedit_Hokeys_Descr, HK_DRAG );
            AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_MODIFY_ITEM, msg, KiBitmap( move_arc_xpm ) );
        }

        msg = AddHotkeyName( _( "Edit Arc Options" ), s_Libedit_Hokeys_Descr, HK_EDIT );
        AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_BODY_EDIT_ITEM, msg, KiBitmap( options_arc_xpm ) );

        if( item->GetFlags() == 0 )
        {
            msg = AddHotkeyName( _( "Delete Arc" ), s_Libedit_Hokeys_Descr, HK_DELETE );
            AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_DELETE_ITEM, msg, KiBitmap( delete_arc_xpm ) );
        }
        break;

    case LIB_CIRCLE_T:
        if( item->GetFlags() == 0 )
        {
            msg = AddHotkeyName( _( "Move Circle" ), s_Libedit_Hokeys_Descr,
                                 HK_LIBEDIT_MOVE_GRAPHIC_ITEM );
            AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST, msg, KiBitmap( move_circle_xpm ) );
        }

        if( item->GetFlags() == 0 )
        {
            msg = AddHotkeyName( _( "Drag Circle Outline" ), s_Libedit_Hokeys_Descr, HK_DRAG );
            AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_MODIFY_ITEM, msg, KiBitmap( move_rectangle_xpm ) );
        }

        msg = AddHotkeyName( _( "Edit Circle Options" ), s_Libedit_Hokeys_Descr, HK_EDIT );
        AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_BODY_EDIT_ITEM, msg, KiBitmap( options_circle_xpm ) );

        if( item->GetFlags() == 0 )
        {
            msg = AddHotkeyName( _( "Delete Circle" ), s_Libedit_Hokeys_Descr, HK_DELETE );
            AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_DELETE_ITEM, msg, KiBitmap( delete_circle_xpm ) );
        }
        break;

    case LIB_RECTANGLE_T:
        if( item->GetFlags() == 0 )
        {
            msg = AddHotkeyName( _( "Move Rectangle" ), s_Libedit_Hokeys_Descr,
                                 HK_LIBEDIT_MOVE_GRAPHIC_ITEM );
            AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST, msg, KiBitmap( move_rectangle_xpm ) );
        }

        msg = AddHotkeyName( _( "Edit Rectangle Options" ), s_Libedit_Hokeys_Descr, HK_EDIT );
        AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_BODY_EDIT_ITEM, msg, KiBitmap( options_rectangle_xpm ) );

        if( item->GetFlags() == 0 )
        {
            msg = AddHotkeyName( _( "Drag Rectangle Edge" ), s_Libedit_Hokeys_Descr, HK_DRAG );
            AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_MODIFY_ITEM, msg, KiBitmap( move_rectangle_xpm ) );
        }

        if( item->GetFlags() == 0 )
        {
            msg = AddHotkeyName( _( "Delete Rectangle" ), s_Libedit_Hokeys_Descr, HK_DELETE );
            AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_DELETE_ITEM, msg, KiBitmap( delete_rectangle_xpm ) );
        }

        break;

    case LIB_TEXT_T:
        if( item->GetFlags() == 0 )
        {
            msg = AddHotkeyName( _( "Move Text" ), s_Libedit_Hokeys_Descr,
                                 HK_LIBEDIT_MOVE_GRAPHIC_ITEM );
            AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST, msg, KiBitmap( move_text_xpm ) );
        }

        msg = AddHotkeyName( _( "Edit Text" ), s_Libedit_Hokeys_Descr, HK_EDIT );
        AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_BODY_EDIT_ITEM, msg, KiBitmap( edit_text_xpm ) );

        msg = AddHotkeyName( _( "Rotate Text" ), s_Libedit_Hokeys_Descr, HK_ROTATE );
        AddMenuItem( PopMenu, ID_LIBEDIT_ROTATE_ITEM, msg, KiBitmap( edit_text_xpm ) );

        if( item->GetFlags() == 0 )
        {
            msg = AddHotkeyName( _( "Delete Text" ), s_Libedit_Hokeys_Descr, HK_DELETE );
            AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_DELETE_ITEM, msg, KiBitmap( delete_text_xpm ) );
        }
        break;

    case LIB_POLYLINE_T:
        if( item->GetFlags() == 0 )
        {
            msg = AddHotkeyName( _( "Move Line" ), s_Libedit_Hokeys_Descr,
                                 HK_LIBEDIT_MOVE_GRAPHIC_ITEM );
            AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST, msg, KiBitmap( move_line_xpm ) );
            msg = AddHotkeyName( _( "Drag Edge Point" ), s_Libedit_Hokeys_Descr, HK_DRAG );
            AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_MODIFY_ITEM, msg, KiBitmap( move_line_xpm ) );
        }

        if( item->IsNew() )
        {
            AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_END_CREATE_ITEM, _( "Line End" ), KiBitmap( apply_xpm ) );
        }

        msg = AddHotkeyName( _( "Edit Line Options" ), s_Libedit_Hokeys_Descr, HK_EDIT );
        AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_BODY_EDIT_ITEM, msg, KiBitmap( options_segment_xpm ) );

        if( item->GetFlags() == 0 )
        {
            msg = AddHotkeyName( _( "Delete Line " ), s_Libedit_Hokeys_Descr, HK_DELETE );
            AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_DELETE_ITEM, msg, KiBitmap( delete_segment_xpm ) );
        }
        else if( item->IsNew() )
        {
            if( ( (LIB_POLYLINE*) item )->GetCornerCount() > 2 )
            {
                msg = AddHotkeyName( _( "Delete Segment" ), s_Libedit_Hokeys_Descr, HK_DELETE );
                AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_DELETE_CURRENT_POLY_SEGMENT,
                             msg, KiBitmap( delete_segment_xpm ) );
            }
        }

        break;

    case LIB_FIELD_T:
        if( item->GetFlags() == 0 )
        {
            msg = AddHotkeyName( _( "Move Field" ), s_Libedit_Hokeys_Descr,
                                 HK_LIBEDIT_MOVE_GRAPHIC_ITEM );
            AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST,  msg, KiBitmap( move_field_xpm ) );
        }

        msg = AddHotkeyName( _( "Field Rotate" ), s_Libedit_Hokeys_Descr, HK_ROTATE );
        AddMenuItem( PopMenu, ID_LIBEDIT_ROTATE_ITEM, msg, KiBitmap( rotate_field_xpm ) );
        msg = AddHotkeyName( _( "Field Edit" ), s_Libedit_Hokeys_Descr, HK_EDIT );
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


void AddMenusForPin( wxMenu* PopMenu, LIB_PIN* Pin, LIB_EDIT_FRAME* frame )
{
    bool selected    = Pin->IsSelected();
    bool not_in_move = !Pin->IsMoving();
    wxString msg;

    if( not_in_move )
    {
        msg = AddHotkeyName( _( "Move Pin " ), s_Libedit_Hokeys_Descr,
                                 HK_LIBEDIT_MOVE_GRAPHIC_ITEM );
        AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST, msg, KiBitmap( move_xpm ) );
    }

    msg = AddHotkeyName( _( "Edit Pin " ), s_Libedit_Hokeys_Descr, HK_EDIT);
    AddMenuItem( PopMenu, ID_LIBEDIT_EDIT_PIN, msg, KiBitmap( edit_xpm ) );

    msg = AddHotkeyName( _( "Rotate Pin " ), s_Libedit_Hokeys_Descr, HK_ROTATE );
    AddMenuItem( PopMenu, ID_LIBEDIT_ROTATE_ITEM, msg, KiBitmap( rotate_pin_xpm ) );

    if( not_in_move )
    {
        msg = AddHotkeyName( _( "Delete Pin " ), s_Libedit_Hokeys_Descr, HK_DELETE );
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
    AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_CANCEL_EDITING, _( "Cancel Block" ), KiBitmap( cancel_xpm ) );

    if( frame->GetScreen()->m_BlockLocate.m_Command == BLOCK_MOVE )
        AddMenuItem( PopMenu, ID_POPUP_ZOOM_BLOCK,
                     _( "Zoom Block (drag middle mouse)" ),
                     KiBitmap( zoom_area_xpm ) );

    PopMenu->AppendSeparator();

    AddMenuItem( PopMenu, ID_POPUP_PLACE_BLOCK, _( "Place Block" ), KiBitmap( apply_xpm ) );

    if( frame->GetScreen()->m_BlockLocate.m_Command == BLOCK_MOVE )
    {
        AddMenuItem( PopMenu, ID_POPUP_SELECT_ITEMS_BLOCK, _( "Select Items" ), KiBitmap( green_xpm ) );
        AddMenuItem( PopMenu, ID_POPUP_COPY_BLOCK, _( "Copy Block" ), KiBitmap( copyblock_xpm ) );
        AddMenuItem( PopMenu, ID_POPUP_MIRROR_Y_BLOCK, _( "Mirror Block ||" ), KiBitmap( mirror_h_xpm ) );
        AddMenuItem( PopMenu, ID_POPUP_MIRROR_X_BLOCK, _( "Mirror Block --" ), KiBitmap( mirror_v_xpm ) );
        AddMenuItem( PopMenu, ID_POPUP_ROTATE_BLOCK, _( "Rotate Block ccw" ), KiBitmap( rotate_ccw_xpm ) );
        AddMenuItem( PopMenu, ID_POPUP_DELETE_BLOCK, _( "Delete Block" ), KiBitmap( delete_xpm ) );
    }
}
