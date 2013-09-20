/**
 * @file pagelayout_editor/onleftclick.cpp
 * @brief functions called on left or double left click mouse event
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 CERN
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
#include <class_drawpanel.h>
#include <common.h>

#include <pl_editor_frame.h>
#include <pl_editor_id.h>
#include <design_tree_frame.h>
#include <properties_frame.h>
#include <dialog_helpers.h>
#include <class_worksheet_dataitem.h>

/* Process the command triggered by the left button of the mouse when a tool
 * is already selected.
 */
void PL_EDITOR_FRAME::OnLeftClick( wxDC* aDC, const wxPoint& aPosition )
{
    WORKSHEET_DATAITEM* item = GetScreen()->GetCurItem();

    if( item )     // An item is currently in edit: place it
    {
        PlaceItem( item );
        m_propertiesPagelayout->CopyPrmsFromItemToPanel( item );
        m_canvas->Refresh();
        return;
    }

    item = m_treePagelayout->GetPageLayoutSelectedItem();
    WORKSHEET_DATAITEM* newitem = Locate( aPosition );

    if( newitem == NULL )
        return;

    if( newitem != item )
    {
        item = newitem;
        m_treePagelayout->SelectCell( item );
        m_canvas->Refresh();
    }
}


/* Called on a double click of left mouse button.
 */
void PL_EDITOR_FRAME::OnLeftDClick( wxDC* aDC, const wxPoint& aPosition )
{
    // Currently: no nothing
}
