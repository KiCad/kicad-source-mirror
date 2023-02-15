/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialogs/dialog_image_properties.h>
#include <dialogs/panel_image_editor.h>

#include <pcb_base_edit_frame.h>
#include <pcb_bitmap.h>
#include <tool/tool_manager.h>
#include <tool/actions.h>
#include <pcb_layer_box_selector.h>


DIALOG_IMAGE_PROPERTIES::DIALOG_IMAGE_PROPERTIES( PCB_BASE_FRAME* aParent, PCB_BITMAP* aBitmap ) :
        DIALOG_IMAGE_PROPERTIES_BASE( aParent ), m_frame( aParent ), m_bitmap( aBitmap ),
        m_posX( aParent, m_XPosLabel, m_ModPositionX, m_XPosUnit ),
        m_posY( aParent, m_YPosLabel, m_ModPositionY, m_YPosUnit )
{
    // Create the image editor page
    m_imageEditor = new PANEL_IMAGE_EDITOR( m_Notebook, aBitmap->MutableImage() );
    m_Notebook->AddPage( m_imageEditor, _( "Image" ), false );

    m_posX.SetCoordType( ORIGIN_TRANSFORMS::ABS_X_COORD );
    m_posY.SetCoordType( ORIGIN_TRANSFORMS::ABS_Y_COORD );

    // Only show unactivated board layers if the bitmap is on one of them
    if( !m_frame->GetBoard()->IsLayerEnabled( m_bitmap->GetLayer() ) )
        m_LayerSelectionCtrl->ShowNonActivatedLayers( true );

    m_LayerSelectionCtrl->SetLayersHotkeys( false );
    m_LayerSelectionCtrl->SetBoardFrame( m_frame );
    m_LayerSelectionCtrl->Resync();

    SetupStandardButtons();

    finishDialogSettings();
}


void PCB_BASE_EDIT_FRAME::ShowBitmapPropertiesDialog( BOARD_ITEM* aBitmap )
{
    PCB_BITMAP*             bitmap = static_cast<PCB_BITMAP*>( aBitmap );
    DIALOG_IMAGE_PROPERTIES dlg( this, bitmap );

    if( dlg.ShowModal() == wxID_OK )
    {
        // The bitmap is cached in Opengl: clear the cache in case it has become invalid
        GetCanvas()->GetView()->RecacheAllItems();
        m_toolManager->PostEvent( EVENTS::SelectedItemsModified );
        OnModify();
    }
}


bool DIALOG_IMAGE_PROPERTIES::TransferDataToWindow()
{
    m_posX.SetValue( m_bitmap->GetPosition().x );
    m_posY.SetValue( m_bitmap->GetPosition().y );

    m_LayerSelectionCtrl->SetLayerSelection( m_bitmap->GetLayer() );

    m_cbLocked->SetValue( m_bitmap->IsLocked() );
    m_cbLocked->SetToolTip( _( "Locked footprints cannot be freely moved and oriented on the "
                               "canvas and can only be selected when the 'Locked items' checkbox "
                               "is enabled in the selection filter." ) );

    return true;
}


bool DIALOG_IMAGE_PROPERTIES::TransferDataFromWindow()
{
    if( m_imageEditor->TransferDataFromWindow() )
    {
        // Save old image in undo list if not already in edit
        if( m_bitmap->GetEditFlags() == 0 )
            m_frame->SaveCopyInUndoList( m_bitmap, UNDO_REDO::CHANGED );

        // Update our bitmap from the editor
        m_imageEditor->TransferToImage( m_bitmap->MutableImage() );

        // Set position, etc.
        m_bitmap->SetPosition( VECTOR2I( m_posX.GetValue(), m_posY.GetValue() ) );
        m_bitmap->SetLayer( ToLAYER_ID( m_LayerSelectionCtrl->GetLayerSelection() ) );
        m_bitmap->SetLocked( m_cbLocked->GetValue() );

        return true;
    }

    return false;
}
