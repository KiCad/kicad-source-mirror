/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "dialog_3D_view_option_base.h"
#include <3d_viewer/eda_3d_viewer.h>
#include <3d_canvas/3d_settings.h>
#include <bitmaps.h>

class DIALOG_3D_VIEW_OPTIONS : public DIALOG_3D_VIEW_OPTIONS_BASE
{
public:
    explicit DIALOG_3D_VIEW_OPTIONS( EDA_3D_VIEWER* parent );

private:
    EDA_3D_VIEWER*   m_parent;
    EDA_3D_SETTINGS& m_settings;

    void initDialog();

    /// Automatically called when clicking on the OK button
    bool TransferDataFromWindow() override;

    /// Automatically called after creating the dialog
    bool TransferDataToWindow() override;
};


void EDA_3D_VIEWER::Install3DViewOptionDialog( wxCommandEvent& event )
{
    DIALOG_3D_VIEW_OPTIONS dlg( this );

    if( dlg.ShowModal() == wxID_OK )
    {
        NewDisplay( true );
    }
}


DIALOG_3D_VIEW_OPTIONS::DIALOG_3D_VIEW_OPTIONS( EDA_3D_VIEWER* parent )
     : DIALOG_3D_VIEW_OPTIONS_BASE( parent ), m_settings( *parent->GetSettings() )
{
    m_parent = parent;

    initDialog();

    m_sdbSizerOK->SetDefault();

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
}


void DIALOG_3D_VIEW_OPTIONS::initDialog()
{
    m_bitmapRealisticMode->SetBitmap( KiBitmap( use_3D_copper_thickness_xpm ) );
    m_bitmapBoardBody->SetBitmap( KiBitmap( ortho_xpm ) );
    m_bitmapCuThickness->SetBitmap( KiBitmap( use_3D_copper_thickness_xpm ) );
    m_bitmap3DshapesTH->SetBitmap( KiBitmap( shape_3d_xpm ) );
    m_bitmap3DshapesSMD->SetBitmap( KiBitmap( shape_3d_xpm ) );
    m_bitmap3DshapesVirtual->SetBitmap( KiBitmap( shape_3d_xpm ) );
    m_bitmapBoundingBoxes->SetBitmap( KiBitmap( axis3d_xpm ) );
    m_bitmapAreas->SetBitmap( KiBitmap( add_zone_xpm ) );
    m_bitmapSilkscreen->SetBitmap( KiBitmap( text_xpm ) );
    m_bitmapSolderMask->SetBitmap( KiBitmap( pads_mask_layers_xpm ) );
    m_bitmapSolderPaste->SetBitmap( KiBitmap( pads_mask_layers_xpm ) );
    m_bitmapAdhesive->SetBitmap( KiBitmap( tools_xpm ) );
    m_bitmapComments->SetBitmap( KiBitmap( editor_xpm ) );
    m_bitmapECO->SetBitmap( KiBitmap( editor_xpm ) );
    m_bitmapSubtractMaskFromSilk->SetBitmap( KiBitmap( use_3D_copper_thickness_xpm ) );
}


bool DIALOG_3D_VIEW_OPTIONS::TransferDataToWindow()
{
    // Check/uncheck checkboxes
    m_checkBoxRealisticMode->SetValue( m_settings.GetFlag( FL_USE_REALISTIC_MODE ) );
    m_checkBoxBoardBody->SetValue( m_settings.GetFlag( FL_SHOW_BOARD_BODY ) );
    m_checkBoxCuThickness->SetValue( m_settings.GetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS ) );
    m_checkBoxAreas->SetValue( m_settings.GetFlag( FL_ZONE ) );
    m_checkBoxBoundingBoxes->SetValue( m_settings.GetFlag( FL_RENDER_OPENGL_SHOW_MODEL_BBOX ) );

    m_checkBox3DshapesTH->SetValue( m_settings.GetFlag( FL_MODULE_ATTRIBUTES_NORMAL ) );
    m_checkBox3DshapesSMD->SetValue( m_settings.GetFlag( FL_MODULE_ATTRIBUTES_NORMAL_INSERT ) );
    m_checkBox3DshapesVirtual->SetValue( m_settings.GetFlag( FL_MODULE_ATTRIBUTES_VIRTUAL ) );

    m_checkBoxSilkscreen->SetValue( m_settings.GetFlag( FL_SILKSCREEN ) );
    m_checkBoxSolderMask->SetValue( m_settings.GetFlag( FL_SOLDERMASK ) );
    m_checkBoxSolderpaste->SetValue( m_settings.GetFlag( FL_SOLDERPASTE ) );
    m_checkBoxAdhesive->SetValue( m_settings.GetFlag( FL_ADHESIVE ) );
    m_checkBoxComments->SetValue( m_settings.GetFlag( FL_COMMENTS ) );
    m_checkBoxECO->SetValue( m_settings.GetFlag( FL_ECO ) );
    m_checkBoxSubtractMaskFromSilk->SetValue( m_settings.GetFlag( FL_SUBTRACT_MASK_FROM_SILK ) );

    return true;
}


bool DIALOG_3D_VIEW_OPTIONS::TransferDataFromWindow()
{
    // Set render mode
    m_settings.SetFlag( FL_USE_REALISTIC_MODE, m_checkBoxRealisticMode->GetValue() );

    // Set visibility of items
    m_settings.SetFlag( FL_SHOW_BOARD_BODY, m_checkBoxBoardBody->GetValue() );
    m_settings.SetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS, m_checkBoxCuThickness->GetValue() );
    m_settings.SetFlag( FL_ZONE, m_checkBoxAreas->GetValue() );
    m_settings.SetFlag( FL_RENDER_OPENGL_SHOW_MODEL_BBOX, m_checkBoxBoundingBoxes->GetValue() );
    m_settings.SetFlag( FL_SUBTRACT_MASK_FROM_SILK, m_checkBoxSubtractMaskFromSilk->GetValue() );

    // Set 3D shapes visibility
    m_settings.SetFlag( FL_MODULE_ATTRIBUTES_NORMAL, m_checkBox3DshapesTH->GetValue() );
    m_settings.SetFlag( FL_MODULE_ATTRIBUTES_NORMAL_INSERT, m_checkBox3DshapesSMD->GetValue() );
    m_settings.SetFlag( FL_MODULE_ATTRIBUTES_VIRTUAL, m_checkBox3DshapesVirtual->GetValue() );

    // Set Layer visibility
    m_settings.SetFlag( FL_SILKSCREEN, m_checkBoxSilkscreen->GetValue() );
    m_settings.SetFlag( FL_SOLDERMASK, m_checkBoxSolderMask->GetValue() );
    m_settings.SetFlag( FL_SOLDERPASTE, m_checkBoxSolderpaste->GetValue() );
    m_settings.SetFlag( FL_ADHESIVE, m_checkBoxAdhesive->GetValue() );
    m_settings.SetFlag( FL_COMMENTS, m_checkBoxComments->GetValue() );
    m_settings.SetFlag( FL_ECO, m_checkBoxECO->GetValue( ) );

    return true;
}
