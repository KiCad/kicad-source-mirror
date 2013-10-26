/**
 * @file dialog_dxf_import.cpp
 * @brief Dialog to import a dxf file on a given board layer.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2013 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dxf2brd_items.h>
#include <wxPcbStruct.h>
#include <convert_from_iu.h>
#include <dialog_dxf_import_base.h>
#include <class_pcb_layer_box_selector.h>

class DIALOG_DXF_IMPORT : public DIALOG_DXF_IMPORT_BASE
{
private:
    PCB_EDIT_FRAME * m_parent;
    static wxString m_dxfFilename;
    static int m_offsetSelection;
    static LAYER_NUM m_layer;

public:

    DIALOG_DXF_IMPORT( PCB_EDIT_FRAME* aParent );
    ~DIALOG_DXF_IMPORT();

private:
    // Virtual event handlers
    void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
    void OnOKClick( wxCommandEvent& event );
	void OnBrowseDxfFiles( wxCommandEvent& event );
};

// Static members of DIALOG_DXF_IMPORT, to remember
// the user's choices during the session
wxString DIALOG_DXF_IMPORT::m_dxfFilename;
int DIALOG_DXF_IMPORT::m_offsetSelection = 4;
LAYER_NUM DIALOG_DXF_IMPORT::m_layer = DRAW_N;


DIALOG_DXF_IMPORT::DIALOG_DXF_IMPORT( PCB_EDIT_FRAME* aParent )
    : DIALOG_DXF_IMPORT_BASE(  aParent )
{
    m_parent = aParent;
    m_textCtrlFileName->SetValue( m_dxfFilename );
    m_rbOffsetOption->SetSelection( m_offsetSelection );

    // Configure the layers list selector
    m_SelLayerBox->SetLayersHotkeys( false );       // Do not display hotkeys
    m_SelLayerBox->SetLayerMask( ALL_CU_LAYERS );   // Do not use copper layers
    m_SelLayerBox->SetBoardFrame( m_parent );
    m_SelLayerBox->Resync();
    if( m_SelLayerBox->SetLayerSelection( m_layer ) < 0 )
    {
        m_layer = DRAW_N;
        m_SelLayerBox->SetLayerSelection( m_layer );
    }

    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
    Centre();
}


DIALOG_DXF_IMPORT::~DIALOG_DXF_IMPORT()
{
    m_offsetSelection = m_rbOffsetOption->GetSelection();
    m_layer = m_SelLayerBox->GetLayerSelection();
}


void DIALOG_DXF_IMPORT::OnBrowseDxfFiles( wxCommandEvent& event )
{
    wxFileDialog dlg( m_parent,
                      wxT( "Open File" ),
                      wxEmptyString, wxEmptyString,
                      wxT( "dxf Files (*.dxf)|*.dxf|*.DXF" ),
                      wxFD_OPEN|wxFD_FILE_MUST_EXIST );
    dlg.ShowModal();

    wxString fileName = dlg.GetPath();

    if( fileName.IsEmpty() )
        return;

    m_dxfFilename = fileName;
    m_textCtrlFileName->SetValue( fileName );
}

void DIALOG_DXF_IMPORT::OnOKClick( wxCommandEvent& event )
{
    m_dxfFilename = m_textCtrlFileName->GetValue();

    if( m_dxfFilename.IsEmpty() )
        return;

    double offsetX = 0;
    double offsetY = 0;

    m_offsetSelection = m_rbOffsetOption->GetSelection();
    switch( m_offsetSelection )
    {
        case 0:
            break;

        case 1:
            offsetY = m_parent->GetPageSizeIU().y * MM_PER_IU / 2;
            break;

        case 2:
            offsetX = m_parent->GetPageSizeIU().x * MM_PER_IU / 2;
            offsetY = m_parent->GetPageSizeIU().y * MM_PER_IU / 2;
            break;

        case 3:
            offsetY = m_parent->GetPageSizeIU().y * MM_PER_IU;
            break;
    }

    BOARD * brd = m_parent->GetBoard();
    DXF2BRD_CONVERTER dxf_importer;

    // Set coordinates offset for import (offset is given in mm)
    dxf_importer.SetOffset( offsetX, offsetY );
    m_layer = m_SelLayerBox->GetLayerSelection();
    dxf_importer.SetBrdLayer( m_layer );
    dxf_importer.ImportDxfFile( m_dxfFilename, brd );

    EndModal( wxID_OK );
}


bool InvokeDXFDialogImport( PCB_EDIT_FRAME* aCaller )
{
    DIALOG_DXF_IMPORT dlg( aCaller );
    bool success = dlg.ShowModal() == wxID_OK;

    if( success )
        aCaller->OnModify();

    return success;
}
