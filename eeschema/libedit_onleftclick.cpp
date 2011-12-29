/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2006 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2010 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "fctsys.h"
#include "gr_basic.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "eeschema_id.h"

#include "general.h"
#include "protos.h"
#include "libeditframe.h"
#include "class_libentry.h"


void LIB_EDIT_FRAME::OnLeftClick( wxDC* DC, const wxPoint& aPosition )
{
    LIB_ITEM* item = m_drawItem;

    if( m_component == NULL )   // No component loaded !
        return;

    if( item == NULL || item->GetFlags() == 0 )
    {
        item = LocateItemUsingCursor( aPosition );

        if( item )

        {
            item->DisplayInfo( this );
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
        if( item && item->GetFlags() )   // moved object
        {
            switch( item->Type() )
            {
            case LIB_PIN_T:
                PlacePin( DC );
                break;

            default:
                EndDrawGraphicItem( DC );
                break;
            }
        }
        break;

    case ID_LIBEDIT_PIN_BUTT:
        if( m_drawItem == NULL || m_drawItem->GetFlags() == 0 )
        {
            CreatePin( DC );
        }
        else
        {
            PlacePin( DC );
        }
        break;

    case ID_LIBEDIT_BODY_LINE_BUTT:
    case ID_LIBEDIT_BODY_ARC_BUTT:
    case ID_LIBEDIT_BODY_CIRCLE_BUTT:
    case ID_LIBEDIT_BODY_RECT_BUTT:
    case ID_LIBEDIT_BODY_TEXT_BUTT:
        if( m_drawItem == NULL || m_drawItem->GetFlags() == 0 )
        {
            m_drawItem = CreateGraphicItem( m_component, DC );
        }
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
        SaveCopyInUndoList( m_component );
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
    if( m_component == NULL )
        return;

    if( ( m_drawItem == NULL ) || ( m_drawItem->GetFlags() == 0 ) )
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
        m_drawItem->DisplayInfo( this );
    else
        return;

    m_canvas->SetIgnoreMouseEvents( true );

    switch( m_drawItem->Type() )
    {
    case LIB_PIN_T:
        if( m_drawItem->GetFlags() == 0 )
        {
            wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );
            cmd.SetId( ID_LIBEDIT_EDIT_PIN );
            GetEventHandler()->ProcessEvent( cmd );
        }
        break;

    case LIB_ARC_T:
    case LIB_CIRCLE_T:
    case LIB_RECTANGLE_T:
        if( m_drawItem->GetFlags() == 0 )
        {
            EditGraphicSymbol( DC, m_drawItem );
        }
        break;

    case LIB_POLYLINE_T:
        if( m_drawItem->GetFlags() == 0 )
        {
            EditGraphicSymbol( DC, m_drawItem );
        }
        else if( m_drawItem->IsNew() )
        {
            EndDrawGraphicItem( DC );
        }
        break;

    case LIB_TEXT_T:
        if( m_drawItem->GetFlags() == 0 )
        {
            EditSymbolText( DC, m_drawItem );
        }
        break;

    case LIB_FIELD_T:
        if( m_drawItem->GetFlags() == 0 )
        {
            EditField( DC, (LIB_FIELD*) m_drawItem );
        }
        break;

    default:
        wxFAIL_MSG( wxT( "Unhandled item <" ) + m_drawItem->GetClass() + wxT( ">" ) );
        break;
    }

    m_canvas->MoveCursorToCrossHair();
    m_canvas->SetIgnoreMouseEvents( false );
}
