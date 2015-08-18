/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014-2015 KiCad Developers, see CHANGELOG.TXT for contributors.
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
#include <3d_viewer.h>
#include <info3d_visu.h>

class DIALOG_3D_VIEW_OPTIONS : public DIALOG_3D_VIEW_OPTIONS_BASE
{
public:
    DIALOG_3D_VIEW_OPTIONS( EDA_3D_FRAME* parent );

private:
    EDA_3D_FRAME* m_parent;
    INFO3D_VISU & m_3Dprms;

    void initDialog();

    // Event functions:
    void OnShowAllClick( wxCommandEvent& event );
    void OnShowNoneClick( wxCommandEvent& event );
    void OnOKClick( wxCommandEvent& event );
	void OnCheckRealisticMode( wxCommandEvent& event );
};


void EDA_3D_FRAME::Install_3D_ViewOptionDialog( wxCommandEvent& event )
{
    DIALOG_3D_VIEW_OPTIONS dlg( this );

    if( dlg.ShowModal() == wxID_OK )
    {
        SetMenuBarOptionsState();
        NewDisplay();
    }
}


DIALOG_3D_VIEW_OPTIONS::DIALOG_3D_VIEW_OPTIONS( EDA_3D_FRAME* parent )
     :DIALOG_3D_VIEW_OPTIONS_BASE( parent ), m_3Dprms( g_Parm_3D_Visu )
{
    m_parent = parent;

    initDialog();

    SetDefaultItem( (wxWindow*) m_sdbSizerOK );
	Layout();
    GetSizer()->SetSizeHints( this );
	Centre();
}


void DIALOG_3D_VIEW_OPTIONS::initDialog()
{
    m_bitmapRealisticMode->SetBitmap( KiBitmap( use_3D_copper_thickness_xpm ) );
    m_bitmapCuThickness->SetBitmap( KiBitmap( use_3D_copper_thickness_xpm ) );
    m_bitmap3Dshapes->SetBitmap( KiBitmap( shape_3d_xpm ) );
    m_bitmapAreas->SetBitmap( KiBitmap( add_zone_xpm ) );
    m_bitmapSilkscreen->SetBitmap( KiBitmap( add_text_xpm ) );
    m_bitmapSolderMask->SetBitmap( KiBitmap( pads_mask_layers_xpm ) );
    m_bitmapSolderPaste->SetBitmap( KiBitmap( pads_mask_layers_xpm ) );
    m_bitmapAdhesive->SetBitmap( KiBitmap( tools_xpm ) );
    m_bitmapComments->SetBitmap( KiBitmap( edit_sheet_xpm ) );
    m_bitmapECO->SetBitmap( KiBitmap( edit_sheet_xpm ) );

    // Check/uncheck checkboxes
    m_checkBoxRealisticMode->SetValue( m_3Dprms.GetFlag( FL_USE_REALISTIC_MODE ) );
    m_checkBoxCuThickness->SetValue( m_3Dprms.GetFlag( FL_USE_COPPER_THICKNESS ) );
    m_checkBox3Dshapes->SetValue( m_3Dprms.GetFlag( FL_MODULE ) );
    m_checkBoxAreas->SetValue( m_3Dprms.GetFlag( FL_ZONE ) );
    m_checkBoxSilkscreen->SetValue( m_3Dprms.GetFlag( FL_SILKSCREEN ) );
    m_checkBoxSolderMask->SetValue( m_3Dprms.GetFlag( FL_SOLDERMASK ) );
    m_checkBoxSolderpaste->SetValue( m_3Dprms.GetFlag( FL_SOLDERPASTE ) );
    m_checkBoxAdhesive->SetValue( m_3Dprms.GetFlag( FL_ADHESIVE ) );

    m_checkBoxComments->SetValue( m_3Dprms.GetFlag( FL_COMMENTS ) );
    m_checkBoxECO->SetValue( m_3Dprms.GetFlag( FL_ECO ) );
    bool enable = !m_3Dprms.GetFlag( FL_USE_REALISTIC_MODE );
    m_checkBoxComments->Enable( enable );
    m_checkBoxECO->Enable( enable );
}

void DIALOG_3D_VIEW_OPTIONS::OnCheckRealisticMode( wxCommandEvent& event )
{
    bool enable = !m_checkBoxRealisticMode->GetValue();
    m_checkBoxComments->Enable( enable );
    m_checkBoxECO->Enable( enable );
}

void DIALOG_3D_VIEW_OPTIONS::OnShowAllClick( wxCommandEvent& event )
{
    bool state = true;
    m_checkBoxCuThickness->SetValue( state );
    m_checkBox3Dshapes->SetValue( state );
    m_checkBoxAreas->SetValue( state );
    m_checkBoxSilkscreen->SetValue( state );
    m_checkBoxSolderMask->SetValue( state );
    m_checkBoxSolderpaste->SetValue( state );
    m_checkBoxAdhesive->SetValue( state );
    m_checkBoxComments->SetValue( state );
    m_checkBoxECO->SetValue( state );
}


void DIALOG_3D_VIEW_OPTIONS::OnShowNoneClick( wxCommandEvent& event )
{
    bool state = false;
    m_checkBoxCuThickness->SetValue( state );
    m_checkBox3Dshapes->SetValue( state );
    m_checkBoxAreas->SetValue( state );
    m_checkBoxSilkscreen->SetValue( state );
    m_checkBoxSolderMask->SetValue( state );
    m_checkBoxSolderpaste->SetValue( state );
    m_checkBoxAdhesive->SetValue( state );
    m_checkBoxComments->SetValue( state );
    m_checkBoxECO->SetValue( state );
}


void DIALOG_3D_VIEW_OPTIONS::OnOKClick( wxCommandEvent& event )
{
    m_3Dprms.SetFlag( FL_USE_REALISTIC_MODE, m_checkBoxRealisticMode->GetValue() );
    m_3Dprms.SetFlag( FL_USE_COPPER_THICKNESS, m_checkBoxCuThickness->GetValue() );
    m_3Dprms.SetFlag( FL_MODULE, m_checkBox3Dshapes->GetValue() );
    m_3Dprms.SetFlag( FL_ZONE, m_checkBoxAreas->GetValue() );
    m_3Dprms.SetFlag( FL_SILKSCREEN, m_checkBoxSilkscreen->GetValue() );
    m_3Dprms.SetFlag( FL_SOLDERMASK, m_checkBoxSolderMask->GetValue() );
    m_3Dprms.SetFlag( FL_SOLDERPASTE, m_checkBoxSolderpaste->GetValue() );
    m_3Dprms.SetFlag( FL_ADHESIVE, m_checkBoxAdhesive->GetValue() );
    m_3Dprms.SetFlag( FL_COMMENTS, m_checkBoxComments->GetValue() );
    m_3Dprms.SetFlag( FL_ECO, m_checkBoxECO->GetValue( ) );

    EndModal( wxID_OK );
}
