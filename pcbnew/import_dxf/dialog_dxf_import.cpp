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

#include <dialog_dxf_import.h>
//#include <pgm_base.h>
#include <kiface_i.h>
#include <convert_from_iu.h>
#include <class_pcb_layer_box_selector.h>
#include <class_draw_panel_gal.h>

#include <class_board.h>
#include <class_module.h>
#include <class_edge_mod.h>
#include <class_text_mod.h>
#include <class_pcb_text.h>

// Keys to store setup in config
#define DXF_IMPORT_LAYER_OPTION_KEY wxT("DxfImportBrdLayer")
#define DXF_IMPORT_COORD_ORIGIN_KEY wxT("DxfImportCoordOrigin")
#define DXF_IMPORT_LAST_FILE_KEY wxT("DxfImportLastFile")

// Static members of DIALOG_DXF_IMPORT, to remember
// the user's choices during the session
wxString DIALOG_DXF_IMPORT::m_dxfFilename;
int DIALOG_DXF_IMPORT::m_offsetSelection = 4;
LAYER_NUM DIALOG_DXF_IMPORT::m_layer = Dwgs_User;


DIALOG_DXF_IMPORT::DIALOG_DXF_IMPORT( PCB_BASE_FRAME* aParent )
    : DIALOG_DXF_IMPORT_BASE( aParent )
{
    m_parent = aParent;
    m_config = Kiface().KifaceSettings();

    if( m_config )
    {
        m_layer = m_config->Read( DXF_IMPORT_LAYER_OPTION_KEY, (long)Dwgs_User );
        m_offsetSelection = m_config->Read( DXF_IMPORT_COORD_ORIGIN_KEY, 3 );
        m_dxfFilename =  m_config->Read( DXF_IMPORT_LAST_FILE_KEY, wxEmptyString );
    }

    m_textCtrlFileName->SetValue( m_dxfFilename );
    m_rbOffsetOption->SetSelection( m_offsetSelection );

    // Configure the layers list selector
    m_SelLayerBox->SetLayersHotkeys( false );           // Do not display hotkeys
    m_SelLayerBox->SetLayerSet( LSET::AllCuMask() );    // Do not use copper layers
    m_SelLayerBox->SetBoardFrame( m_parent );
    m_SelLayerBox->Resync();

    if( m_SelLayerBox->SetLayerSelection( m_layer ) < 0 )
    {
        m_layer = Dwgs_User;
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

    if( m_config )
    {
        m_config->Write( DXF_IMPORT_LAYER_OPTION_KEY, (long)m_layer );
        m_config->Write( DXF_IMPORT_COORD_ORIGIN_KEY, m_offsetSelection );
        m_config->Write( DXF_IMPORT_LAST_FILE_KEY, m_dxfFilename );
    }
}


void DIALOG_DXF_IMPORT::OnBrowseDxfFiles( wxCommandEvent& event )
{
    wxString path;
    wxString filename;

    if( !m_dxfFilename.IsEmpty() )
    {
        wxFileName fn( m_dxfFilename );
        path = fn.GetPath();
        filename = fn.GetFullName();
    }
    wxFileDialog dlg( m_parent,
                      wxT( "Open File" ),
                      path, filename,
                      wxT( "dxf Files (*.dxf)|*.dxf" ),
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

    // Set coordinates offset for import (offset is given in mm)
    m_dxfImporter.SetOffset( offsetX, offsetY );
    m_layer = m_SelLayerBox->GetLayerSelection();
    m_dxfImporter.SetBrdLayer( m_layer );

    // Read dxf file:
    m_dxfImporter.ImportDxfFile( m_dxfFilename );

    EndModal( wxID_OK );
}


bool InvokeDXFDialogBoardImport( PCB_BASE_FRAME* aCaller )
{
    DIALOG_DXF_IMPORT dlg( aCaller );
    bool success = ( dlg.ShowModal() == wxID_OK );

    if( success )
    {
        const std::list<BOARD_ITEM*>& list = dlg.GetImportedItems();
        PICKED_ITEMS_LIST picklist;

        BOARD* board = aCaller->GetBoard();
        KIGFX::VIEW* view = aCaller->GetGalCanvas()->GetView();

        std::list<BOARD_ITEM*>::const_iterator it, itEnd;
        for( it = list.begin(), itEnd = list.end(); it != itEnd; ++it )
        {
            BOARD_ITEM* item = *it;
            board->Add( item );

            ITEM_PICKER itemWrapper( item, UR_NEW );
            picklist.PushItem( itemWrapper );

            if( aCaller->IsGalCanvasActive() )
                view->Add( item );
        }

        aCaller->SaveCopyInUndoList( picklist, UR_NEW, wxPoint( 0, 0 ) );
        aCaller->OnModify();
    }

    return success;
}


bool InvokeDXFDialogModuleImport( PCB_BASE_FRAME* aCaller, MODULE* aModule )
{
    DIALOG_DXF_IMPORT dlg( aCaller );
    bool success = ( dlg.ShowModal() == wxID_OK );

    if( success )
    {
        const std::list<BOARD_ITEM*>& list = dlg.GetImportedItems();
        MODULE* module = aCaller->GetBoard()->m_Modules;
        KIGFX::VIEW* view = aCaller->GetGalCanvas()->GetView();

        aCaller->SaveCopyInUndoList( module, UR_MODEDIT );
        aCaller->OnModify();

        std::list<BOARD_ITEM*>::const_iterator it, itEnd;
        for( it = list.begin(), itEnd = list.end(); it != itEnd; ++it )
        {
            BOARD_ITEM* item = *it;
            BOARD_ITEM* converted = NULL;

            // Modules use different types for the same things,
            // so we need to convert imported items to appropriate classes.
            switch( item->Type() )
            {
            case PCB_LINE_T:
            {
                converted = new EDGE_MODULE( module );
                *static_cast<DRAWSEGMENT*>( converted ) = *static_cast<DRAWSEGMENT*>( item );
                module->Add( converted );
                static_cast<EDGE_MODULE*>( converted )->SetLocalCoord();
                delete item;
                break;
            }

            case PCB_TEXT_T:
            {
                converted = new TEXTE_MODULE( module );
                *static_cast<TEXTE_PCB*>( converted ) = *static_cast<TEXTE_PCB*>( item );
                module->Add( module );
                static_cast<TEXTE_MODULE*>( converted )->SetLocalCoord();
                delete item;
                break;
            }

            default:
                assert( false );    // there is a type that is currently not handled here
                break;
            }

            if( aCaller->IsGalCanvasActive() )
                view->Add( converted );
        }
    }

    return success;
}
