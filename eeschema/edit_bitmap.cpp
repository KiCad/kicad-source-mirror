/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 jean-pierre.charras
 * Copyright (C) 2012 KiCad Developers, see change_log.txt for contributors.
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
 * @file edit_bitmap.cpp
 */

#include <fctsys.h>
#include <class_drawpanel.h>

#include <schframe.h>
#include <sch_bitmap.h>
#include <dialog_image_editor.h>


static void abortMoveBitmap( EDA_DRAW_PANEL* aPanel, wxDC* aDC )
{
    SCH_SCREEN*     screen = (SCH_SCREEN*) aPanel->GetScreen();
    SCH_BITMAP*     item   = (SCH_BITMAP*) screen->GetCurItem();
    SCH_EDIT_FRAME* parent = (SCH_EDIT_FRAME*) aPanel->GetParent();

    parent->SetRepeatItem( NULL );

    if( item == NULL )  /* no current item */
        return;

    if( item->IsNew() )
    {
        delete item;
        item = NULL;
    }
    else    // Move command on an existing text item, restore the data of the original.
    {
        item->ClearFlags();

        SCH_BITMAP * olditem = (SCH_BITMAP*) parent->GetUndoItem();

        wxCHECK_RET( olditem != NULL && item->Type() == olditem->Type() &&
                     item->Type() == SCH_BITMAP_T,
                     wxT( "Cannot restore undefined last text item." ) );

        // Never delete existing item, because it can be referenced by an undo/redo command
        // Just restore its data
        item->SwapData( olditem );
        parent->SetUndoItem( NULL );
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

        STATUS_FLAGS flgs = image->GetFlags();
        image->ClearFlags();
        aPanel->RefreshDrawingRect( dirty );
        image->SetFlags( flgs );
        aPanel->SetMouseCapture( moveBitmap, abortMoveBitmap );
    }

    // Draw the bitmap at it's new position.
    image->SetPosition( aPanel->GetParent()->GetCrossHairPosition() );
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

    wxPoint     pos = GetCrossHairPosition();

    SCH_BITMAP* image = new SCH_BITMAP( pos );

    if( !image->ReadImageFile( fullFilename ) )
    {
        wxMessageBox( _( "Couldn't load image from <%s>" ), GetChars( fullFilename ) );
        delete image;
        return NULL;
    }


    image->SetFlags( IS_NEW | IS_MOVED );
    image->Draw( m_canvas, aDC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );

    m_canvas->SetMouseCapture( moveBitmap, abortMoveBitmap );
    GetScreen()->SetCurItem( image );

    OnModify();
    return image;
}

void SCH_EDIT_FRAME::MoveImage( SCH_BITMAP* aImageItem, wxDC* aDC )
{
    aImageItem->SetFlags( IS_MOVED );

    m_canvas->SetMouseCapture( moveBitmap, abortMoveBitmap );
    GetScreen()->SetCurItem( aImageItem );
    SetRepeatItem( NULL );

    SetUndoItem( aImageItem );

    m_canvas->CrossHairOff( aDC );
    SetCrossHairPosition( aImageItem->GetPosition() );
    m_canvas->MoveCursorToCrossHair();
    m_canvas->CrossHairOn( aDC );

    OnModify();
}

void SCH_EDIT_FRAME::RotateImage( SCH_BITMAP* aItem )
{
    if( aItem->GetFlags( ) == 0 )
        SaveCopyInUndoList( aItem, UR_ROTATED, aItem->GetPosition() );

    aItem->Rotate( aItem->GetPosition() );
    OnModify();
    m_canvas->Refresh();
}

void SCH_EDIT_FRAME::MirrorImage( SCH_BITMAP* aItem, bool Is_X_axis )
{
    if( aItem->GetFlags( ) == 0 )
        SaveCopyInUndoList( aItem, UR_CHANGED );

    if( Is_X_axis )
        aItem->MirrorX( aItem->GetPosition().y );
    else
        aItem->MirrorY( aItem->GetPosition().x );

    OnModify();
    m_canvas->Refresh();
}

void SCH_EDIT_FRAME::EditImage( SCH_BITMAP* aItem )
{
    // TODO: change image scale or more
    DIALOG_IMAGE_EDITOR dlg( this, aItem->m_Image );
    if( dlg.ShowModal() != wxID_OK )
        return;

    // save old image in undo list if not already in edit
    // or the image to be edited is part of a block
    if( aItem->GetFlags() == 0 ||
        GetScreen()->m_BlockLocate.GetState() != STATE_NO_BLOCK )
        SaveCopyInUndoList( aItem, UR_CHANGED );

    dlg.TransfertToImage(aItem->m_Image);
    OnModify();
    m_canvas->Refresh();
}
