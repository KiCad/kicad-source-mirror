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

#include <dialogs/dialog_image_properties.h>
#include <dialogs/panel_image_editor.h>

#include <sch_edit_frame.h>
#include <sch_bitmap.h>
#include <sch_commit.h>


DIALOG_IMAGE_PROPERTIES::DIALOG_IMAGE_PROPERTIES( SCH_EDIT_FRAME* aParent, SCH_BITMAP& aBitmap ) :
        DIALOG_IMAGE_PROPERTIES_BASE( aParent ),
        m_frame( aParent ),
        m_bitmap( aBitmap ),
        m_posX( aParent, m_XPosLabel, m_ModPositionX, m_XPosUnit ),
        m_posY( aParent, m_YPosLabel, m_ModPositionY, m_YPosUnit )
{
    // Create the image editor page
    const REFERENCE_IMAGE& refImage = aBitmap.GetReferenceImage();
    m_imageEditor = new PANEL_IMAGE_EDITOR( aParent, this, refImage.GetImage() );

    m_imageSizer->Add( m_imageEditor, 1, wxEXPAND | wxALL, 5 );

    m_posX.SetCoordType( ORIGIN_TRANSFORMS::ABS_X_COORD );
    m_posY.SetCoordType( ORIGIN_TRANSFORMS::ABS_Y_COORD );

    SetupStandardButtons();

    finishDialogSettings();
}


bool DIALOG_IMAGE_PROPERTIES::TransferDataToWindow()
{
    m_posX.SetValue( m_bitmap.GetPosition().x );
    m_posY.SetValue( m_bitmap.GetPosition().y );

    return m_imageEditor->TransferDataToWindow();
}


bool DIALOG_IMAGE_PROPERTIES::TransferDataFromWindow()
{
    REFERENCE_IMAGE& refImage = m_bitmap.GetReferenceImage();

    if( m_imageEditor->TransferDataFromWindow() )
    {
        SCH_COMMIT commit( m_frame );

        // Save old image in undo list if not already in edit
        if( m_bitmap.GetEditFlags() == 0 )
            commit.Modify( &m_bitmap, m_frame->GetScreen() );

        // Update our bitmap from the editor
        m_imageEditor->TransferToImage( refImage.MutableImage() );

        m_bitmap.SetPosition( VECTOR2I( m_posX.GetIntValue(), m_posY.GetIntValue() ) );

        if( !commit.Empty() )
            commit.Push( _( "Image Properties" ) );

        return true;
    }

    return false;
}
