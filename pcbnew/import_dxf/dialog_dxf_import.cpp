/**
 * @file dialog_dxf_import.cpp
 * @brief Dialog to import a dxf file on a given board layer.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <kiface_i.h>
#include <convert_from_iu.h>
#include <class_pcb_layer_box_selector.h>

#include <class_board.h>
#include <class_module.h>
#include <class_edge_mod.h>
#include <class_text_mod.h>
#include <class_pcb_text.h>

// Keys to store setup in config
#define DXF_IMPORT_LAYER_OPTION_KEY wxT("DxfImportBrdLayer")
#define DXF_IMPORT_COORD_ORIGIN_KEY wxT("DxfImportCoordOrigin")
#define DXF_IMPORT_LAST_FILE_KEY wxT("DxfImportLastFile")
#define DXF_IMPORT_GRID_UNITS_KEY wxT("DxfImportGridUnits")
#define DXF_IMPORT_GRID_OFFSET_X_KEY wxT("DxfImportGridOffsetX")
#define DXF_IMPORT_GRID_OFFSET_Y_KEY wxT("DxfImportGridOffsetY")


// Static members of DIALOG_DXF_IMPORT, to remember
// the user's choices during the session
wxString DIALOG_DXF_IMPORT::m_dxfFilename;
int DIALOG_DXF_IMPORT::m_offsetSelection = 0;
LAYER_NUM DIALOG_DXF_IMPORT::m_layer = Dwgs_User;


DIALOG_DXF_IMPORT::DIALOG_DXF_IMPORT( PCB_BASE_FRAME* aParent )
    : DIALOG_DXF_IMPORT_BASE( aParent )
{
    m_parent = aParent;
    m_config = Kiface().KifaceSettings();
    m_PCBGridUnits = 0;
    m_PCBGridOffsetX = 0.0;
    m_PCBGridOffsetY = 0.0;

    if( m_config )
    {
        m_layer = m_config->Read( DXF_IMPORT_LAYER_OPTION_KEY, (long)Dwgs_User );
        m_offsetSelection = m_config->Read( DXF_IMPORT_COORD_ORIGIN_KEY, (long)0 );
        m_dxfFilename =  m_config->Read( DXF_IMPORT_LAST_FILE_KEY, wxEmptyString );
        m_config->Read( DXF_IMPORT_GRID_UNITS_KEY, &m_PCBGridUnits, 0 );
        m_config->Read( DXF_IMPORT_GRID_OFFSET_X_KEY, &m_PCBGridOffsetX, 0.0 );
        m_config->Read( DXF_IMPORT_GRID_OFFSET_Y_KEY, &m_PCBGridOffsetY, 0.0 );
    }

    m_DXFPCBGridUnits->SetSelection( m_PCBGridUnits );
    wxString tmpStr;
    tmpStr << m_PCBGridOffsetX;
    m_DXFPCBXCoord->SetValue( tmpStr );
    tmpStr = wxT( "" );
    tmpStr << m_PCBGridOffsetY;
    m_DXFPCBYCoord->SetValue( tmpStr );

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

    m_sdbSizer1OK->SetDefault();
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

        m_config->Write( DXF_IMPORT_GRID_UNITS_KEY, GetPCBGridUnits() );
        m_config->Write( DXF_IMPORT_GRID_OFFSET_X_KEY, m_DXFPCBXCoord->GetValue() );
        m_config->Write( DXF_IMPORT_GRID_OFFSET_Y_KEY, m_DXFPCBYCoord->GetValue() );
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
                      _( "Open File" ),
                      path, filename,
                      wxT( "DXF Files (*.dxf)|*.dxf" ),
                      wxFD_OPEN|wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() != wxID_OK )
        return;

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
        offsetX = m_parent->GetPageSizeIU().x * MM_PER_IU / 2;
        offsetY = m_parent->GetPageSizeIU().y * MM_PER_IU / 2;
        break;

    case 1:
        break;

    case 2:
        offsetY = m_parent->GetPageSizeIU().y * MM_PER_IU / 2;
        break;

    case 3:
        offsetY = m_parent->GetPageSizeIU().y * MM_PER_IU;
        break;

    case 4:
        GetPCBGridOffsets( offsetX, offsetY );

        if( GetPCBGridUnits() )
        {
            offsetX *= 25.4;
            offsetY *= 25.4;
        }
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

        std::list<BOARD_ITEM*>::const_iterator it, itEnd;
        for( it = list.begin(), itEnd = list.end(); it != itEnd; ++it )
        {
            BOARD_ITEM* item = *it;
            board->Add( item );

            ITEM_PICKER itemWrapper( item, UR_NEW );
            picklist.PushItem( itemWrapper );
        }

        aCaller->SaveCopyInUndoList( picklist, UR_NEW, wxPoint( 0, 0 ) );
        aCaller->OnModify();
    }

    return success;
}


bool InvokeDXFDialogModuleImport( PCB_BASE_FRAME* aCaller, MODULE* aModule )
{
    wxASSERT( aModule );

    DIALOG_DXF_IMPORT dlg( aCaller );
    bool success = ( dlg.ShowModal() == wxID_OK );

    if( success )
    {
        const std::list<BOARD_ITEM*>& list = dlg.GetImportedItems();

        aCaller->SaveCopyInUndoList( aModule, UR_MODEDIT );
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
                converted = new EDGE_MODULE( aModule );
                *static_cast<DRAWSEGMENT*>( converted ) = *static_cast<DRAWSEGMENT*>( item );
                aModule->Add( converted );
                static_cast<EDGE_MODULE*>( converted )->SetLocalCoord();
                delete item;
                break;
            }

            case PCB_TEXT_T:
            {
                converted = new TEXTE_MODULE( aModule );
                *static_cast<TEXTE_PCB*>( converted ) = *static_cast<TEXTE_PCB*>( item );
                aModule->Add( converted );
                static_cast<TEXTE_MODULE*>( converted )->SetLocalCoord();
                delete item;
                break;
            }

            default:
                wxLogDebug( wxT( "type %d currently not handled" ), item->Type() );
                break;
            }
        }
    }

    return success;
}


void DIALOG_DXF_IMPORT::OriginOptionOnUpdateUI( wxUpdateUIEvent& event )
{
    bool enable = m_rbOffsetOption->GetSelection() == 4;

    m_DXFPCBGridUnits->Enable( enable );
    m_DXFPCBXCoord->Enable( enable );
    m_DXFPCBYCoord->Enable( enable );
}


int  DIALOG_DXF_IMPORT::GetPCBGridUnits( void )
{
    return m_DXFPCBGridUnits->GetSelection();
}

void DIALOG_DXF_IMPORT::GetPCBGridOffsets( double &aXOffset, double &aYOffset )
{
    aXOffset = DoubleValueFromString( UNSCALED_UNITS, m_DXFPCBXCoord->GetValue() );
    aYOffset = DoubleValueFromString( UNSCALED_UNITS, m_DXFPCBYCoord->GetValue() );
    return;
}
