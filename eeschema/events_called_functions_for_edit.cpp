/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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

/*
 * @file events_called_functions.cpp
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <class_drawpanel.h>
#include <general.h>
#include <kicad_device_context.h>
#include <wxEeschemaStruct.h>
#include <sch_component.h>
#include <sch_text.h>


void SCH_EDIT_FRAME::OnCopySchematicItemRequest( wxCommandEvent& event )
{
    SCH_ITEM * curr_item = GetScreen()->GetCurItem();

    if( !curr_item || curr_item->GetFlags() )
        return;

    INSTALL_UNBUFFERED_DC( dc, m_canvas );

    switch( curr_item->Type() )
    {
    case SCH_COMPONENT_T:
    {
        SCH_COMPONENT* newitem;
        newitem = new SCH_COMPONENT( *( (SCH_COMPONENT*) curr_item ) );
        newitem->SetTimeStamp( GetNewTimeStamp() );
        newitem->ClearAnnotation( NULL );
        newitem->SetFlags( IS_NEW );
        // Draw the new part, MoveItem() expects it to be already on screen.
        newitem->Draw( m_canvas, &dc, wxPoint( 0, 0 ), g_XorMode );
        MoveItem( newitem, &dc );
    }
    break;

    case SCH_TEXT_T:
    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIERARCHICAL_LABEL_T:
    {
        SCH_TEXT* newitem = (SCH_TEXT*) curr_item->Clone();
        newitem->SetFlags( IS_NEW );
        // Draw the new item, MoveItem() expects it to be already on screen.
        newitem->Draw( m_canvas, &dc, wxPoint( 0, 0 ), g_XorMode );
        MoveItem( newitem, &dc );
    }
        break;

    default:
        break;
    }
}
