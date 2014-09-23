/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2013 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file libedit_onleftclick.cpp
 * @brief Eeschema library editor event handler for a mouse left button single or double click.
 */

#include <fctsys.h>
#include <class_drawpanel.h>
#include <eeschema_id.h>
#include <msgpanel.h>

#include <general.h>
#include <libeditframe.h>
#include <class_libentry.h>


void LIB_EDIT_FRAME::OnLeftClick( wxDC* DC, const wxPoint& aPosition )
{
    LIB_ITEM*   item = m_drawItem;
    bool        item_in_edit = item && item->InEditMode();
    bool        no_item_edited = !item_in_edit;

    LIB_PART*      part = GetCurPart();

    if( !part )         // No component loaded !
        return;

    if( ( GetToolId() == ID_NO_TOOL_SELECTED ) && no_item_edited )
    {
        item = LocateItemUsingCursor( aPosition );

        if( item )
        {
            MSG_PANEL_ITEMS items;
            item->GetMsgPanelInfo( items );
            SetMsgPanel( items );
        }
        else
        {
            DisplayCmpDoc();

            if( m_canvas->GetAbortRequest() )
                m_canvas->SetAbortRequest( false );
        }
    }

    switch( GetToolId() )
    {
    case ID_NO_TOOL_SELECTED:
        // If an item is currently in edit, finish edit
        if( item_in_edit )
        {
            switch( item->Type() )
            {
            case LIB_PIN_T:
                PlacePin();
                break;

            default:
                EndDrawGraphicItem( DC );
                break;
            }
        }
        break;

    case ID_LIBEDIT_PIN_BUTT:
        if( no_item_edited )
            CreatePin( DC );
        else
            PlacePin();
        break;

    case ID_LIBEDIT_BODY_LINE_BUTT:
    case ID_LIBEDIT_BODY_ARC_BUTT:
    case ID_LIBEDIT_BODY_CIRCLE_BUTT:
    case ID_LIBEDIT_BODY_RECT_BUTT:
    case ID_LIBEDIT_BODY_TEXT_BUTT:
        if( no_item_edited )
            m_drawItem = CreateGraphicItem( part, DC );
        else if( m_drawItem )
        {
            if( m_drawItem->IsNew() )
                GraphicItemBeginDraw( DC );
            else
                EndDrawGraphicItem( DC );
        }
        break;

    case ID_LIBEDIT_DELETE_ITEM_BUTT:
        m_drawItem = LocateItemUsingCursor( aPosition );

        if( m_drawItem )
            deleteItem( DC );
        else
            DisplayCmpDoc();

        break;

    case ID_LIBEDIT_ANCHOR_ITEM_BUTT:
        SaveCopyInUndoList( part );
        PlaceAnchor();
        SetToolID( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor(), wxEmptyString );
        break;

    default:
        wxFAIL_MSG( wxString::Format( wxT( "Unhandled command ID %d" ), GetToolId() ) );
        SetToolID( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor(), wxEmptyString );
        break;
    }
}


/*
 * Called on a double click:
 *  If an editable item  (field, pin, graphic):
 *      Call the suitable dialog editor.
 */
void LIB_EDIT_FRAME::OnLeftDClick( wxDC* DC, const wxPoint& aPosition )
{
    LIB_PART*      part = GetCurPart();

    if( !part )
        return;

    if( !m_drawItem || !m_drawItem->InEditMode() )
    {   // We can locate an item
        m_drawItem = LocateItemUsingCursor( aPosition );

        if( m_drawItem == NULL )
        {
            wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );

            cmd.SetId( ID_LIBEDIT_GET_FRAME_EDIT_PART );
            GetEventHandler()->ProcessEvent( cmd );
        }
    }

    if( m_drawItem )
        SetMsgPanel( m_drawItem );
    else
        return;

    m_canvas->SetIgnoreMouseEvents( true );
    bool not_edited = !m_drawItem->InEditMode();

    switch( m_drawItem->Type() )
    {
    case LIB_PIN_T:
        if( not_edited )
        {
            wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );

            cmd.SetId( ID_LIBEDIT_EDIT_PIN );
            GetEventHandler()->ProcessEvent( cmd );
        }
        break;

    case LIB_ARC_T:
    case LIB_CIRCLE_T:
    case LIB_RECTANGLE_T:
        if( not_edited )
            EditGraphicSymbol( DC, m_drawItem );
        break;

    case LIB_POLYLINE_T:
        if( not_edited )
            EditGraphicSymbol( DC, m_drawItem );
        else if( m_drawItem->IsNew() )
            EndDrawGraphicItem( DC );
        break;

    case LIB_TEXT_T:
        if( not_edited )
            EditSymbolText( DC, m_drawItem );
        break;

    case LIB_FIELD_T:
        if( not_edited )
            EditField( (LIB_FIELD*) m_drawItem );
        break;

    default:
        wxFAIL_MSG( wxT( "Unhandled item <" ) + m_drawItem->GetClass() + wxT( ">" ) );
        break;
    }

    m_canvas->MoveCursorToCrossHair();
    m_canvas->SetIgnoreMouseEvents( false );
}
