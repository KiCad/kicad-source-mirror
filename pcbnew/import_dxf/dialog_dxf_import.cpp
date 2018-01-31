/**
 * @file dialog_dxf_import.cpp
 * @brief Dialog to import a dxf file on a given board layer.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <convert_to_biu.h>
#include <pcb_layer_box_selector.h>
#include <wildcards_and_files_ext.h>

#include <class_board.h>
#include <class_module.h>
#include <class_edge_mod.h>
#include <class_text_mod.h>
#include <class_pcb_text.h>

// Keys to store setup in config
#define DXF_IMPORT_LAYER_OPTION_KEY     "DxfImportBrdLayer"
#define DXF_IMPORT_COORD_ORIGIN_KEY     "DxfImportCoordOrigin"
#define DXF_IMPORT_LAST_FILE_KEY        "DxfImportLastFile"
#define DXF_IMPORT_IMPORT_UNITS_KEY     "DxfImportOffsetUnits"
#define DXF_IMPORT_IMPORT_OFFSET_X_KEY  "DxfImportOffsetX"
#define DXF_IMPORT_IMPORT_OFFSET_Y_KEY  "DxfImportOffsetY"
#define DXF_IMPORT_LINEWIDTH_UNITS_KEY  "DxfImportLineWidthUnits"
#define DXF_IMPORT_LINEWIDTH_KEY        "DxfImportLineWidth"


// Static members of DIALOG_DXF_IMPORT, to remember
// the user's choices during the session
wxString DIALOG_DXF_IMPORT::m_dxfFilename;
int DIALOG_DXF_IMPORT::m_offsetSelection = 0;
LAYER_NUM DIALOG_DXF_IMPORT::m_layer = Dwgs_User;


DIALOG_DXF_IMPORT::DIALOG_DXF_IMPORT( PCB_BASE_FRAME* aParent, bool aImportAsFootprintGraphic )
    : DIALOG_DXF_IMPORT_BASE( aParent )
{
    m_parent = aParent;
    m_dxfImporter.ImportAsFootprintGraphic( aImportAsFootprintGraphic );
    m_config = Kiface().KifaceSettings();
    m_PcbImportUnits = 0;
    m_PcbImportOffsetX = 0.0;   // always in mm
    m_PcbImportOffsetY = 0.0;   // always in mm
    m_PCBdefaultLineWidth = 0.2; // in mm
    m_PCBLineWidthUnits = 0;

    if( m_config )
    {
        m_layer = m_config->Read( DXF_IMPORT_LAYER_OPTION_KEY, (long)Dwgs_User );
        m_offsetSelection = m_config->Read( DXF_IMPORT_COORD_ORIGIN_KEY, (long)0 );
        m_dxfFilename =  m_config->Read( DXF_IMPORT_LAST_FILE_KEY, wxEmptyString );
        m_config->Read( DXF_IMPORT_IMPORT_UNITS_KEY, &m_PcbImportUnits, 0 );
        m_config->Read( DXF_IMPORT_IMPORT_OFFSET_X_KEY, &m_PcbImportOffsetX, 0.0 );
        m_config->Read( DXF_IMPORT_IMPORT_OFFSET_Y_KEY, &m_PcbImportOffsetY, 0.0 );
        m_config->Read( DXF_IMPORT_LINEWIDTH_UNITS_KEY, &m_PCBLineWidthUnits, 0 );
        m_config->Read( DXF_IMPORT_LINEWIDTH_KEY, &m_PCBdefaultLineWidth, 0.2 );
    }

    m_choiceUnitLineWidth->SetSelection( m_PCBLineWidthUnits );
    showPCBdefaultLineWidth();
    m_dxfImporter.SetDefaultLineWidthMM( m_PCBdefaultLineWidth );

    m_DxfPcbPositionUnits->SetSelection( m_PcbImportUnits );
    showPcbImportOffsets();

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

    m_sdbSizerOK->SetDefault();
    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
    Centre();
}


DIALOG_DXF_IMPORT::~DIALOG_DXF_IMPORT()
{
    m_offsetSelection = m_rbOffsetOption->GetSelection();
    getPcbImportOffsets();
    m_layer = m_SelLayerBox->GetLayerSelection();

    if( m_config )
    {
        m_config->Write( DXF_IMPORT_LAYER_OPTION_KEY, (long)m_layer );
        m_config->Write( DXF_IMPORT_COORD_ORIGIN_KEY, m_offsetSelection );
        m_config->Write( DXF_IMPORT_LAST_FILE_KEY, m_dxfFilename );

        m_config->Write( DXF_IMPORT_IMPORT_UNITS_KEY, m_PcbImportUnits );
        m_config->Write( DXF_IMPORT_IMPORT_OFFSET_X_KEY, m_PcbImportOffsetX );
        m_config->Write( DXF_IMPORT_IMPORT_OFFSET_Y_KEY, m_PcbImportOffsetY );

        m_config->Write( DXF_IMPORT_LINEWIDTH_UNITS_KEY, m_PCBLineWidthUnits );
        m_PCBLineWidthUnits = getPCBdefaultLineWidthMM();
        m_config->Write( DXF_IMPORT_LINEWIDTH_KEY, m_PCBdefaultLineWidth );
    }
}


void DIALOG_DXF_IMPORT::DIALOG_DXF_IMPORT::onUnitPositionSelection( wxCommandEvent& event )
{
    // Collect last entered values:
    getPcbImportOffsets();

    m_PcbImportUnits = m_DxfPcbPositionUnits->GetSelection();;
    showPcbImportOffsets();
}


double DIALOG_DXF_IMPORT::getPCBdefaultLineWidthMM()
{
    double value = DoubleValueFromString( UNSCALED_UNITS, m_textCtrlLineWidth->GetValue() );

    switch( m_PCBLineWidthUnits )
    {
        default:
        case 0:     // display units = mm
            break;

        case 1:     // display units = mil
            value *= 25.4 / 1000;
            break;

        case 2:     // display units = inch
            value *= 25.4;
            break;
    }

    return value;   // value is in mm
}


void DIALOG_DXF_IMPORT::onUnitWidthSelection( wxCommandEvent& event )
{
    m_PCBdefaultLineWidth = getPCBdefaultLineWidthMM();

    // Switch to new units
    m_PCBLineWidthUnits = m_choiceUnitLineWidth->GetSelection();
    showPCBdefaultLineWidth();
}


void DIALOG_DXF_IMPORT::showPcbImportOffsets()
{
    // Display m_PcbImportOffsetX and m_PcbImportOffsetY values according to
    // the unit selection:
    double xoffset = m_PcbImportOffsetX;
    double yoffset = m_PcbImportOffsetY;

    if( m_PcbImportUnits )   // Units are inches
    {
        xoffset /= 25.4;
        yoffset /= 25.4;
    }

    m_DxfPcbXCoord->SetValue( wxString::Format( "%f", xoffset ) );
    m_DxfPcbYCoord->SetValue( wxString::Format( "%f", yoffset ) );

}


void DIALOG_DXF_IMPORT::showPCBdefaultLineWidth()
{
    double value;

    switch( m_PCBLineWidthUnits )
    {
        default:
        case 0:     // display units = mm
            value = m_PCBdefaultLineWidth;
            break;

        case 1:     // display units = mil
            value = m_PCBdefaultLineWidth / 25.4 * 1000;
            break;

        case 2:     // display units = inch
            value = m_PCBdefaultLineWidth / 25.4;
            break;
    }

    m_textCtrlLineWidth->SetValue( wxString::Format( "%f", value ) );
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
                      DxfFileWildcard(),
                      wxFD_OPEN|wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() != wxID_OK )
        return;

    wxString fileName = dlg.GetPath();

    if( fileName.IsEmpty() )
        return;

    m_dxfFilename = fileName;
    m_textCtrlFileName->SetValue( fileName );
}


bool DIALOG_DXF_IMPORT::TransferDataFromWindow()
{
    m_dxfFilename = m_textCtrlFileName->GetValue();

    if( m_dxfFilename.IsEmpty() )
    {
        wxMessageBox( _( "Error: No DXF filename!" ) );
        return false;
    }

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
        getPcbImportOffsets();
        offsetX = m_PcbImportOffsetX;
        offsetY = m_PcbImportOffsetY;
        break;
    }

    // Set coordinates offset for import (offset is given in mm)
    m_dxfImporter.SetOffset( offsetX, offsetY );
    m_layer = m_SelLayerBox->GetLayerSelection();
    m_dxfImporter.SetBrdLayer( m_layer );
    m_PCBdefaultLineWidth = getPCBdefaultLineWidthMM();
    m_dxfImporter.SetDefaultLineWidthMM( m_PCBdefaultLineWidth );

    // Read dxf file:
    m_dxfImporter.ImportDxfFile( m_dxfFilename );

   return true;
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

    DIALOG_DXF_IMPORT dlg( aCaller, true );
    bool success = ( dlg.ShowModal() == wxID_OK );

    if( success )
    {
        const std::list<BOARD_ITEM*>& list = dlg.GetImportedItems();

        aCaller->SaveCopyInUndoList( aModule, UR_CHANGED );
        aCaller->OnModify();

        std::list<BOARD_ITEM*>::const_iterator it, itEnd;

        for( it = list.begin(), itEnd = list.end(); it != itEnd; ++it )
        {
            aModule->Add( *it );
        }
    }

    return success;
}


void DIALOG_DXF_IMPORT::OriginOptionOnUpdateUI( wxUpdateUIEvent& event )
{
    bool enable = m_rbOffsetOption->GetSelection() == 4;

    m_DxfPcbPositionUnits->Enable( enable );
    m_DxfPcbXCoord->Enable( enable );
    m_DxfPcbYCoord->Enable( enable );
}


void DIALOG_DXF_IMPORT::getPcbImportOffsets()
{
    m_PcbImportOffsetX = DoubleValueFromString( UNSCALED_UNITS, m_DxfPcbXCoord->GetValue() );
    m_PcbImportOffsetY = DoubleValueFromString( UNSCALED_UNITS, m_DxfPcbYCoord->GetValue() );

    if( m_PcbImportUnits )   // Units are inches
    {
        m_PcbImportOffsetX *= 25.4;
        m_PcbImportOffsetY *= 25.4;
    }

    return;
}
