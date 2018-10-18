/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 jean-pierre.charras
 * Copyright (C) 2012-2016 KiCad Developers, see change_log.txt for contributors.
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
#include <sch_draw_panel.h>
#include <sch_view.h>
#include <sch_edit_frame.h>
#include <sch_bitmap.h>
#include <dialog_image_editor.h>

#include <view/view_group.h>

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
    auto panel = static_cast<SCH_DRAW_PANEL*> ( aPanel );
    SCH_SCREEN* screen = (SCH_SCREEN*) aPanel->GetScreen();
    SCH_BITMAP* image  = (SCH_BITMAP*) screen->GetCurItem();
    auto preview = panel->GetView()->GetPreview();

    if ( ! image )
        return;

    // Draw the bitmap at it's new position.
    image->SetPosition( aPanel->GetParent()->GetCrossHairPosition() - image->GetStoredPos() );

    auto view = panel->GetView();

    view->ClearPreview();
    view->AddToPreview( image, false );
    view->SetVisible( preview, true );
    view->Update( preview );

    view->MarkTargetDirty( KIGFX::TARGET_NONCACHED );
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
        wxMessageBox( _( "Couldn't load image from \"%s\"" ), GetChars( fullFilename ) );
        return NULL;
    }

    wxPoint     pos = GetCrossHairPosition();

    SCH_BITMAP* image = new SCH_BITMAP( pos );

    if( !image->ReadImageFile( fullFilename ) )
    {
        wxMessageBox( _( "Couldn't load image from \"%s\"" ), GetChars( fullFilename ) );
        delete image;
        return NULL;
    }

    image->SetFlags( IS_NEW | IS_MOVED );

    auto view = static_cast<SCH_DRAW_PANEL*>( m_canvas )->GetView();

    view->ClearPreview();
    view->AddToPreview( image, false );


    m_canvas->SetMouseCapture( moveBitmap, abortMoveBitmap );
    GetScreen()->SetCurItem( image );

    OnModify();
    return image;
}


void SCH_EDIT_FRAME::RotateImage( SCH_BITMAP* aItem )
{
    if( aItem->GetFlags( ) == 0 )
        SaveCopyInUndoList( aItem, UR_ROTATED, false, aItem->GetPosition() );

    aItem->Rotate( aItem->GetPosition() );

    RefreshItem( aItem );
    OnModify();
}


void SCH_EDIT_FRAME::MirrorImage( SCH_BITMAP* aItem, bool Is_X_axis )
{
    if( aItem->GetFlags( ) == 0 )
        SaveCopyInUndoList( aItem, UR_CHANGED );

    if( Is_X_axis )
        aItem->MirrorX( aItem->GetPosition().y );
    else
        aItem->MirrorY( aItem->GetPosition().x );

    RefreshItem( aItem );
    OnModify();
}


void SCH_EDIT_FRAME::EditImage( SCH_BITMAP* aItem )
{
    // TODO: change image scale or more
    DIALOG_IMAGE_EDITOR dlg( this, aItem->GetImage() );

    if( dlg.ShowModal() != wxID_OK )
        return;

    // save old image in undo list if not already in edit
    // or the image to be edited is part of a block
    int mask = EDA_ITEM_ALL_FLAGS - ( SELECTED | HIGHLIGHTED | BRIGHTENED );
    if( ( aItem->GetFlags() & mask ) == 0
        || GetScreen()->m_BlockLocate.GetState() != STATE_NO_BLOCK )
    {
        SaveCopyInUndoList( aItem, UR_CHANGED );
    }

    dlg.TransfertToImage( aItem->GetImage() );

    RefreshItem( aItem );
    OnModify();
}
