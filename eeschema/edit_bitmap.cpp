/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 jean-pierre.charras
 * Copyright (C) 2012-2019 KiCad Developers, see change_log.txt for contributors.
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
#include <sch_draw_panel.h>
#include <sch_view.h>
#include <sch_edit_frame.h>
#include <sch_bitmap.h>
#include <dialog_image_editor.h>

#include <view/view_group.h>


bool SCH_EDIT_FRAME::EditImage( SCH_BITMAP* aItem )
{
    // TODO: change image scale or more
    DIALOG_IMAGE_EDITOR dlg( this, aItem->GetImage() );

    if( dlg.ShowModal() != wxID_OK )
        return false;

    // save old image in undo list if not already in edit
    if( aItem->GetEditFlags() == 0 )
        SaveCopyInUndoList( aItem, UR_CHANGED );

    dlg.TransfertToImage( aItem->GetImage() );

    RefreshItem( aItem );
    OnModify();
    return true;
}
