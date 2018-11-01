/**
 * @file dialog_gfx_import.cpp
 * @brief Dialog to import a vector graphics file on a given board layer.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "dialog_import_gfx.h"
#include <kiface_i.h>
#include <convert_to_biu.h>
#include <confirm.h>

#include <pcb_base_frame.h>
#include <class_board.h>
#include <class_module.h>
#include <class_edge_mod.h>
#include <class_text_mod.h>
#include <class_pcb_text.h>
#include <pcb_layer_box_selector.h>

#include <import_gfx/graphics_importer_pcbnew.h>

// Keys to store setup in config
#define IMPORT_GFX_LAYER_OPTION_KEY  "GfxImportBrdLayer"
#define IMPORT_GFX_COORD_ORIGIN_KEY  "GfxImportCoordOrigin"
#define IMPORT_GFX_LAST_FILE_KEY     "GfxImportLastFile"
#define IMPORT_GFX_GRID_UNITS_KEY    "GfxImportGridUnits"
#define IMPORT_GFX_GRID_OFFSET_X_KEY "GfxImportGridOffsetX"
#define IMPORT_GFX_GRID_OFFSET_Y_KEY "GfxImportGridOffsetY"


// Static members of DIALOG_IMPORT_GFX, to remember the user's choices during the session
wxString DIALOG_IMPORT_GFX::m_filename;
int DIALOG_IMPORT_GFX::m_offsetSelection = 0;
LAYER_NUM DIALOG_IMPORT_GFX::m_layer = Dwgs_User;


DIALOG_IMPORT_GFX::DIALOG_IMPORT_GFX( PCB_BASE_FRAME* aParent, bool aUseModuleItems )
    : DIALOG_IMPORT_GFX_BASE( aParent )
{
    m_parent = aParent;

    if( aUseModuleItems )
        m_importer.reset( new GRAPHICS_IMPORTER_MODULE( m_parent->GetBoard()->m_Modules ) );
    else
        m_importer.reset( new GRAPHICS_IMPORTER_BOARD( m_parent->GetBoard() ) );

    m_config = Kiface().KifaceSettings();
    m_gridUnits = 0;
    m_gridOffsetX = 0.0;
    m_gridOffsetY = 0.0;

    if( m_config )
    {
        m_layer = m_config->Read( IMPORT_GFX_LAYER_OPTION_KEY, (long)Dwgs_User );
        m_offsetSelection = m_config->Read( IMPORT_GFX_COORD_ORIGIN_KEY, (long)0 );
        m_filename =  m_config->Read( IMPORT_GFX_LAST_FILE_KEY, wxEmptyString );
        m_config->Read( IMPORT_GFX_GRID_UNITS_KEY, &m_gridUnits, 0 );
        m_config->Read( IMPORT_GFX_GRID_OFFSET_X_KEY, &m_gridOffsetX, 0.0 );
        m_config->Read( IMPORT_GFX_GRID_OFFSET_Y_KEY, &m_gridOffsetY, 0.0 );
    }

    m_PCBGridUnits->SetSelection( m_gridUnits );
    wxString tmpStr;
    tmpStr << m_gridOffsetX;
    m_PCBXCoord->SetValue( tmpStr );
    tmpStr =  "";
    tmpStr << m_gridOffsetY;
    m_PCBYCoord->SetValue( tmpStr );

    m_textCtrlFileName->SetValue( m_filename );
    m_rbOffsetOption->SetSelection( m_offsetSelection );

    // Configure the layers list selector
    m_SelLayerBox->SetLayersHotkeys( false );           // Do not display hotkeys
    m_SelLayerBox->SetNotAllowedLayerSet( LSET::AllCuMask() );    // Do not use copper layers
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


DIALOG_IMPORT_GFX::~DIALOG_IMPORT_GFX()
{
    m_offsetSelection = m_rbOffsetOption->GetSelection();
    m_layer = m_SelLayerBox->GetLayerSelection();

    if( m_config )
    {
        m_config->Write( IMPORT_GFX_LAYER_OPTION_KEY, (long)m_layer );
        m_config->Write( IMPORT_GFX_COORD_ORIGIN_KEY, m_offsetSelection );
        m_config->Write( IMPORT_GFX_LAST_FILE_KEY, m_filename );

        m_config->Write( IMPORT_GFX_GRID_UNITS_KEY, GetPCBGridUnits() );
        m_config->Write( IMPORT_GFX_GRID_OFFSET_X_KEY, m_PCBXCoord->GetValue() );
        m_config->Write( IMPORT_GFX_GRID_OFFSET_Y_KEY, m_PCBYCoord->GetValue() );
    }
}


void DIALOG_IMPORT_GFX::OnBrowseFiles( wxCommandEvent& event )
{
    wxString path;
    wxString filename;

    if( !m_filename.IsEmpty() )
    {
        wxFileName fn( m_filename );
        path = fn.GetPath();
        filename = fn.GetFullName();
    }

    // Generate the list of handled file formats
    wxString wildcardsDesc;
    wxString allWildcards;

    for( auto pluginType : GRAPHICS_IMPORT_MGR::GFX_FILE_TYPES )
    {
        auto plugin = GRAPHICS_IMPORT_MGR::GetPlugin( pluginType );
        const auto wildcards = plugin->GetWildcards();

        wildcardsDesc += "|" + plugin->GetName() + " (" + wildcards + ")|" + wildcards;
        allWildcards += wildcards + ";";
    }

    wildcardsDesc = "All supported formats|" + allWildcards + wildcardsDesc;

    wxFileDialog dlg( m_parent, _( "Open File" ), path, filename,
                       wildcardsDesc, wxFD_OPEN|wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() != wxID_OK )
        return;

    wxString fileName = dlg.GetPath();

    if( fileName.IsEmpty() )
        return;

    m_filename = fileName;
    m_textCtrlFileName->SetValue( fileName );
}


void DIALOG_IMPORT_GFX::OnOKClick( wxCommandEvent& event )
{
    m_filename = m_textCtrlFileName->GetValue();

    if( m_filename.IsEmpty() )
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
    //m_importer.SetOffset( offsetX, offsetY );
    m_layer = m_SelLayerBox->GetLayerSelection();
    m_importer->SetLayer( PCB_LAYER_ID( m_layer ) );
    auto plugin = GRAPHICS_IMPORT_MGR::GetPluginByExt( wxFileName( m_filename ).GetExt() );

    if( plugin )
    {
        m_importer->SetScale( 1.0 /*1e6*/ );       // mm -> IU @todo: add a setting in the dialog and apply it here
        m_importer->SetLineWidth( 0.1 * 1e6 );   // @todo add a setting in the dialog and apply it here
        m_importer->SetPlugin( std::move( plugin ) );

        if( m_importer->Load( m_filename ) )
            m_importer->Import( 1.0, 1.0 );  // @todo

        EndModal( wxID_OK );
    }
    else
    {
        DisplayError( this, _( "There is no plugin to handle this file type" ) );
    }
}

void DIALOG_IMPORT_GFX::onChangeHeight( wxUpdateUIEvent &event)
{
    // @todo: implement scaling of Y
#if 0
    double heightInput = DoubleValueFromString(UNSCALED_UNITS,m_tcHeight->GetValue());

    if(m_cbKeepAspectRatio->GetValue())
    {
    }
#endif
}

#if 0
    // Must be reworked (perhaps removed) because this is not used in GAL canvases
    // only in legacy canvas.
bool InvokeDialogImportGfxBoard( PCB_BASE_FRAME* aCaller )
{
    DIALOG_IMPORT_GFX dlg( aCaller );
    bool success = ( dlg.ShowModal() == wxID_OK );

    if( success )
    {
        PICKED_ITEMS_LIST picklist;
        BOARD* board = aCaller->GetBoard();
        auto& items = dlg.GetImportedItems();

        for( auto it = items.begin(); it != items.end(); ++it )
        {
            BOARD_ITEM* item = static_cast<BOARD_ITEM*>( it->release() );
            board->Add( item );

            ITEM_PICKER itemWrapper( item, UR_NEW );
            picklist.PushItem( itemWrapper );
        }

        aCaller->SaveCopyInUndoList( picklist, UR_NEW, wxPoint( 0, 0 ) );
        aCaller->OnModify();
    }

    return success;
}


bool InvokeDialogImportGfxModule( PCB_BASE_FRAME* aCaller, MODULE* aModule )
{
    wxASSERT( aModule );

    DIALOG_IMPORT_GFX dlg( aCaller, true );
    bool success = ( dlg.ShowModal() == wxID_OK );

    if( success )
    {
        aCaller->SaveCopyInUndoList( aModule, UR_CHANGED );
        aCaller->OnModify();
        auto& list = dlg.GetImportedItems();

        for( auto it = list.begin(); it != list.end(); ++it )
        {
            aModule->Add( static_cast<BOARD_ITEM*>( it->release() ) );
        }
    }

    return success;
}
#endif

void DIALOG_IMPORT_GFX::OriginOptionOnUpdateUI( wxUpdateUIEvent& event )
{
    bool enable = m_rbOffsetOption->GetSelection() == 4;

    m_PCBGridUnits->Enable( enable );
    m_PCBXCoord->Enable( enable );
    m_PCBYCoord->Enable( enable );
}


int  DIALOG_IMPORT_GFX::GetPCBGridUnits( void )
{
    return m_PCBGridUnits->GetSelection();
}


void DIALOG_IMPORT_GFX::GetPCBGridOffsets( double &aXOffset, double &aYOffset )
{
    aXOffset = DoubleValueFromString( UNSCALED_UNITS, m_PCBXCoord->GetValue() );
    aYOffset = DoubleValueFromString( UNSCALED_UNITS, m_PCBYCoord->GetValue() );
    return;
}
