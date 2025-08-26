/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialogs/dialog_reference_image_properties.h>
#include <dialogs/panel_image_editor.h>

#include <pcb_base_edit_frame.h>
#include <pcb_reference_image.h>
#include <tool/tool_manager.h>
#include <tool/actions.h>
#include <board.h>
#include <pcb_layer_box_selector.h>


DIALOG_REFERENCE_IMAGE_PROPERTIES::DIALOG_REFERENCE_IMAGE_PROPERTIES( PCB_BASE_FRAME* aParent,
                                                                      PCB_REFERENCE_IMAGE& aBitmap ) :
        DIALOG_REFERENCE_IMAGE_PROPERTIES_BASE( aParent ),
        m_frame( aParent ),
        m_bitmap( aBitmap ),
        m_posX( aParent, m_XPosLabel, m_ModPositionX, m_XPosUnit ),
        m_posY( aParent, m_YPosLabel, m_ModPositionY, m_YPosUnit ),
        m_width( aParent, m_WidthLabel, m_ModWidth, m_WidthUnit ),
        m_height( aParent, m_HeightLabel, m_ModHeight, m_HeightUnit )
{
    // Create the image editor page
    m_imageEditor = new PANEL_IMAGE_EDITOR( aParent, this, aBitmap.GetReferenceImage().MutableImage() );

    m_imageSizer->Add( m_imageEditor, 1, wxEXPAND | wxALL, 5 );

    m_posX.SetCoordType( ORIGIN_TRANSFORMS::ABS_X_COORD );
    m_posY.SetCoordType( ORIGIN_TRANSFORMS::ABS_Y_COORD );
    m_width.SetCoordType( ORIGIN_TRANSFORMS::ABS_X_COORD );
    m_height.SetCoordType( ORIGIN_TRANSFORMS::ABS_Y_COORD );

    m_ModWidth->Bind( wxEVT_TEXT, &DIALOG_REFERENCE_IMAGE_PROPERTIES::onWidthChanged, this );
    m_ModHeight->Bind( wxEVT_TEXT, &DIALOG_REFERENCE_IMAGE_PROPERTIES::onHeightChanged, this );
    m_imageEditor->GetScaleCtrl()->Bind( wxEVT_TEXT, &DIALOG_REFERENCE_IMAGE_PROPERTIES::onScaleChanged, this );

    // Only show unactivated board layers if the bitmap is on one of them
    if( !m_frame->GetBoard()->IsLayerEnabled( m_bitmap.GetLayer() ) )
        m_LayerSelectionCtrl->ShowNonActivatedLayers( true );

    m_LayerSelectionCtrl->SetLayersHotkeys( false );
    m_LayerSelectionCtrl->SetBoardFrame( m_frame );
    m_LayerSelectionCtrl->Resync();

    SetupStandardButtons();

    finishDialogSettings();
}


void PCB_BASE_EDIT_FRAME::ShowReferenceImagePropertiesDialog( BOARD_ITEM* aBitmap )
{
    PCB_REFERENCE_IMAGE&              bitmap = static_cast<PCB_REFERENCE_IMAGE&>( *aBitmap );
    DIALOG_REFERENCE_IMAGE_PROPERTIES dlg( this, bitmap );

    if( dlg.ShowModal() == wxID_OK )
    {
        // The bitmap is cached in Opengl: clear the cache in case it has become invalid
        GetCanvas()->GetView()->RecacheAllItems();
        m_toolManager->PostEvent( EVENTS::SelectedItemsModified );
        OnModify();
    }
}


bool DIALOG_REFERENCE_IMAGE_PROPERTIES::TransferDataToWindow()
{
    m_posX.SetValue( m_bitmap.GetPosition().x );
    m_posY.SetValue( m_bitmap.GetPosition().y );

    m_LayerSelectionCtrl->SetLayerSelection( m_bitmap.GetLayer() );

    m_cbLocked->SetValue( m_bitmap.IsLocked() );
    m_cbLocked->SetToolTip( _( "Locked items cannot be freely moved and oriented on the canvas and can only be "
                               "selected when the 'Locked items' checkbox is checked in the selection filter." ) );

    m_imageEditor->TransferDataToWindow();
    VECTOR2I size = m_imageEditor->GetImageSize();
    m_width.SetValue( size.x );
    m_height.SetValue( size.y );

    return true;
}


bool DIALOG_REFERENCE_IMAGE_PROPERTIES::TransferDataFromWindow()
{
    if( m_imageEditor->TransferDataFromWindow() )
    {
        // Save old image in undo list if not already in edit
        if( m_bitmap.GetEditFlags() == 0 )
            m_frame->SaveCopyInUndoList( &m_bitmap, UNDO_REDO::CHANGED );

        // Update our bitmap from the editor
        m_imageEditor->TransferToImage( m_bitmap.GetReferenceImage().MutableImage() );

        // Set position, etc.
        m_bitmap.SetPosition( VECTOR2I( m_posX.GetIntValue(), m_posY.GetIntValue() ) );
        m_bitmap.SetLayer( ToLAYER_ID( m_LayerSelectionCtrl->GetLayerSelection() ) );
        m_bitmap.SetLocked( m_cbLocked->GetValue() );

        return true;
    }

    return false;
}


void DIALOG_REFERENCE_IMAGE_PROPERTIES::onWidthChanged( wxCommandEvent& aEvent )
{
    double newWidth = m_width.GetDoubleValue();
    VECTOR2I size = m_imageEditor->GetImageSize();

    if( size.x > 0 )
    {
        double scale = m_imageEditor->GetScale() * newWidth / size.x;
        m_imageEditor->SetScale( scale );
        VECTOR2I newSize = m_imageEditor->GetImageSize();
        m_height.ChangeDoubleValue( newSize.y );
    }
}


void DIALOG_REFERENCE_IMAGE_PROPERTIES::onHeightChanged( wxCommandEvent& aEvent )
{
    double newHeight = m_height.GetDoubleValue();
    VECTOR2I size = m_imageEditor->GetImageSize();

    if( size.y > 0 )
    {
        double scale = m_imageEditor->GetScale() * newHeight / size.y;
        m_imageEditor->SetScale( scale );
        VECTOR2I newSize = m_imageEditor->GetImageSize();
        m_width.ChangeDoubleValue( newSize.x );
    }
}


void DIALOG_REFERENCE_IMAGE_PROPERTIES::onScaleChanged( wxCommandEvent& aEvent )
{
    double scale = m_imageEditor->GetScale();

    if( scale <= 0 )
        return;

    m_imageEditor->SetScale( scale );
    VECTOR2I size = m_imageEditor->GetImageSize();
    m_width.ChangeDoubleValue( size.x );
    m_height.ChangeDoubleValue( size.y );
}
