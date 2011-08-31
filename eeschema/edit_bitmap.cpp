/********************/
/* edit_bitmap.cpp */
/********************/

/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2011 jean-pierre.charras
 * Copyright (C) 2011 Kicad Developers, see change_log.txt for contributors.
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

#include "fctsys.h"
#include "gr_basic.h"
#include "macros.h"
#include "class_drawpanel.h"
#include "trigo.h"
#include "common.h"
#include "richio.h"
#include "plot_common.h"

#include "wxEeschemaStruct.h"
#include "general.h"
#include "sch_bitmap.h"
#include "dialog_image_editor.h"

static void abortMoveBitmap( EDA_DRAW_PANEL* aPanel, wxDC* aDC )
{
    SCH_SCREEN*     screen = (SCH_SCREEN*) aPanel->GetScreen();
    SCH_ITEM*       item   = screen->GetCurItem();
    SCH_EDIT_FRAME* parent = (SCH_EDIT_FRAME*) aPanel->GetParent();

    parent->SetRepeatItem( NULL );

    if( item == NULL )  /* no current item */
        return;

    if( item->IsNew() )
    {
        delete item;
        item = NULL;
    }
    else    // Move command on an existing text item, restore the copy of the original.
    {
        screen->RemoveFromDrawList( item );
        delete item;

        item = parent->GetUndoItem();

        wxCHECK_RET( item != NULL, wxT( "Cannot restore undefined last text item." ) );

        screen->AddToDrawList( item );

        // the owner of item is no more parent, this is the draw list of screen:
        parent->SetUndoItem( NULL );

        item->Draw( aPanel, aDC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
        item->ClearFlags();
    }

    screen->SetCurItem( item );
    aPanel->Refresh();
}

static void moveBitmap( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition, bool aErase )
{
    SCH_SCREEN* screen = (SCH_SCREEN*) aPanel->GetScreen();
    SCH_BITMAP* image  = (SCH_BITMAP*) screen->GetCurItem();

    if( aErase )
    {
        // Erase the current bitmap at its current position.
        // Note also items flagged IS_MOVING are not drawn,
        // and if image is new, it is not yet il draw list
        // so image is erased from screen
        EDA_RECT dirty = image->GetBoundingBox();
        dirty.Inflate( 4 );     // Give a margin
        aPanel->SetMouseCapture( NULL, NULL );  // Avoid loop in redraw panel

        int flgs = image->GetFlags();
        image->m_Flags = 0;
        aPanel->RefreshDrawingRect( dirty );
        image->SetFlags( flgs );
        aPanel->SetMouseCapture( moveBitmap, abortMoveBitmap );
    }

    // Draw the bitmap at it's new position.
    image->m_Pos = screen->GetCrossHairPosition();
    image->Draw( aPanel, aDC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
}


SCH_BITMAP* SCH_EDIT_FRAME::CreateNewImage( wxDC* aDC )
{
    wxFileDialog fileDlg( this, _( "Choose Image" ), wxEmptyString, wxEmptyString,
                          _( "Image Files " ) + wxImage::GetImageExtWildcard(),
                          wxFD_OPEN );
    int          diag = fileDlg.ShowModal();

    if( diag != wxID_OK )
        return NULL;

    wxString fullFilename = fileDlg.GetPath();

    if( !wxFileExists( fullFilename ) )
    {
        wxMessageBox( _( "Couldn't load image from <%s>" ), GetChars( fullFilename ) );
        return NULL;
    }

    wxPoint     pos = GetScreen()->GetCrossHairPosition();

    SCH_BITMAP* image = new SCH_BITMAP( pos );
    if( !image->ReadImageFile( fullFilename ) )
    {
        wxMessageBox( _( "Couldn't load image from <%s>" ), GetChars( fullFilename ) );
        delete image;
        return NULL;
    }


    image->SetFlags( IS_NEW | IS_MOVED );
    image->Draw( DrawPanel, aDC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );

    DrawPanel->SetMouseCapture( moveBitmap, abortMoveBitmap );
    GetScreen()->SetCurItem( image );

    OnModify();
    return image;
}

void SCH_EDIT_FRAME::MoveImage( SCH_BITMAP* aImageItem, wxDC* aDC )
{
    aImageItem->SetFlags( IS_MOVED );

    DrawPanel->SetMouseCapture( moveBitmap, abortMoveBitmap );
    GetScreen()->SetCurItem( aImageItem );
    m_itemToRepeat = NULL;

    SetUndoItem( aImageItem );

    DrawPanel->CrossHairOff( aDC );
    GetScreen()->SetCrossHairPosition( aImageItem->m_Pos );
    DrawPanel->MoveCursorToCrossHair();
    DrawPanel->CrossHairOn( aDC );

    OnModify();
}

void SCH_EDIT_FRAME::RotateImage( SCH_BITMAP* aItem )
{
    if( aItem->GetFlags( ) == 0 )
        SaveCopyInUndoList( aItem, UR_ROTATED, aItem->m_Pos );

    aItem->Rotate( aItem->m_Pos );
    OnModify();
    DrawPanel->Refresh();
}

void SCH_EDIT_FRAME::MirrorImage( SCH_BITMAP* aItem, bool Is_X_axis )
{
    if( aItem->GetFlags( ) == 0 )
        SaveCopyInUndoList( aItem, UR_CHANGED );

    if( Is_X_axis )
        aItem->Mirror_X( aItem->m_Pos.y );
    else
        aItem->Mirror_Y( aItem->m_Pos.x );

    OnModify();
    DrawPanel->Refresh();
}

void SCH_EDIT_FRAME::EditImage( SCH_BITMAP* aItem )
{
    // TODO: change image scale or more
    DIALOG_IMAGE_EDITOR dlg( this, aItem->m_Image );
    if( dlg.ShowModal() != wxID_OK )
        return;

    if( aItem->GetFlags( ) == 0 )
        SaveCopyInUndoList( aItem, UR_CHANGED );
    dlg.TransfertToImage(aItem->m_Image);
    OnModify();
    DrawPanel->Refresh();
}
