/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialogs/html_message_box.h>

#include "dialog_import_gfx.h"
#include <kiface_base.h>
#include <locale_io.h>
#include <pcb_layer_box_selector.h>
#include <wildcards_and_files_ext.h>
#include <board.h>
#include <bitmaps.h>
#include <map>
#include "dxf_import_plugin.h"
#include <wx/filedlg.h>
#include <wx/msgdlg.h>

#include <memory>

// Static members of DIALOG_IMPORT_GFX, to remember the user's choices during the session
wxString DIALOG_IMPORT_GFX::m_filename;
bool DIALOG_IMPORT_GFX::m_placementInteractive = true;
bool DIALOG_IMPORT_GFX::m_shouldGroupItems = true;
LAYER_NUM DIALOG_IMPORT_GFX::m_layer = Dwgs_User;
double DIALOG_IMPORT_GFX::m_scaleImport = 1.0;     // Do not change the imported items size
int DIALOG_IMPORT_GFX::m_originUnits = 0;          // millimeter
int DIALOG_IMPORT_GFX::m_dxfLineWidthUnits = 0;    // millimeter
int DIALOG_IMPORT_GFX::m_dxfUnits = 0;             // first entry in the dxfUnits map below

const std::map<DXF_IMPORT_UNITS, wxString> dxfUnitsMap = {
    { DXF_IMPORT_UNITS::INCHES, _( "Inches" ) },
    { DXF_IMPORT_UNITS::MILLIMETERS, _( "Millimeters" ) },
    { DXF_IMPORT_UNITS::MILS, _( "Mils" ) },
    { DXF_IMPORT_UNITS::CENTIMETERS, _( "Centimeter" ) },
    { DXF_IMPORT_UNITS::FEET, _( "Feet" ) },
};


DIALOG_IMPORT_GFX::DIALOG_IMPORT_GFX( PCB_BASE_FRAME* aParent, bool aImportAsFootprintGraphic )
    : DIALOG_IMPORT_GFX_BASE( aParent )
{
    m_parent = aParent;

    if( aImportAsFootprintGraphic )
        m_importer = std::make_unique<GRAPHICS_IMPORTER_FOOTPRINT>( m_parent->GetBoard()->GetFirstFootprint() );
    else
        m_importer = std::make_unique<GRAPHICS_IMPORTER_BOARD>( m_parent->GetBoard() );

    // construct an import manager with options from config
    {
        GRAPHICS_IMPORT_MGR::TYPE_LIST blacklist;
        // Currently: all types are allowed, so the blacklist is empty
        // (no GFX_FILE_T in the blacklist)
        // To disable SVG import, enable these 2 lines
        // if( !ADVANCED_CFG::GetCfg().m_enableSvgImport )
        //    blacklist.push_back( GRAPHICS_IMPORT_MGR::SVG );
        // The SVG import has currently a flaw: all SVG shapes are imported as curves and
        // converted to a lot of segments.  A better approach is to convert to polylines
        // (not yet existing in Pcbnew) and keep arcs and circles as primitives (not yet
        // possible with tinysvg library).

        m_gfxImportMgr = std::make_unique<GRAPHICS_IMPORT_MGR>( blacklist );
    }

    m_originUnits = 0;
    m_origin.x = 0.0;              // always in mm
    m_origin.y = 0.0;              // always in mm
    m_dxfLineWidth = 0.2;          // always in mm
    m_dxfLineWidthUnits = 0;

    auto cfg = m_parent->GetPcbNewSettings();

    m_layer                = cfg->m_ImportGraphics.layer;
    m_placementInteractive = cfg->m_ImportGraphics.interactive_placement;
    m_filename             = cfg->m_ImportGraphics.last_file;
    m_dxfLineWidth         = cfg->m_ImportGraphics.dxf_line_width;
    m_dxfLineWidthUnits    = cfg->m_ImportGraphics.dxf_line_width_units;
    m_originUnits          = cfg->m_ImportGraphics.origin_units;
    m_origin.x             = cfg->m_ImportGraphics.origin_x;
    m_origin.y             = cfg->m_ImportGraphics.origin_y;
    m_dxfUnits             = cfg->m_ImportGraphics.dxf_units;

    m_choiceUnitLineWidth->SetSelection( m_dxfLineWidthUnits );
    showDXFDefaultLineWidth();

    m_DxfPcbPositionUnits->SetSelection( m_originUnits );
    showPcbImportOffsets();

    m_textCtrlFileName->SetValue( m_filename );
    m_rbInteractivePlacement->SetValue( m_placementInteractive );
    m_rbAbsolutePlacement->SetValue( not m_placementInteractive );
    m_groupItems->SetValue( m_shouldGroupItems );

    m_textCtrlImportScale->SetValue( wxString::Format( wxT( "%f" ), m_scaleImport ) );

    // Configure the layers list selector
    m_SelLayerBox->SetLayersHotkeys( false );                    // Do not display hotkeys
    m_SelLayerBox->SetBoardFrame( m_parent );
    m_SelLayerBox->Resync();

    if( m_SelLayerBox->SetLayerSelection( m_layer ) < 0 )
    {
        m_layer = Dwgs_User;
        m_SelLayerBox->SetLayerSelection( m_layer );
    }

    for( auto& unitEntry : dxfUnitsMap )
        m_choiceDxfUnits->Append( unitEntry.second );

    m_choiceDxfUnits->SetSelection( m_dxfUnits );

    m_browseButton->SetBitmap( KiBitmap( BITMAPS::small_folder ) );

    wxCommandEvent dummy;
    onFilename( dummy );

    SetInitialFocus( m_textCtrlFileName );
    m_sdbSizerOK->SetDefault();
    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
    Centre();

    m_textCtrlFileName->Connect( wxEVT_COMMAND_TEXT_UPDATED,
                                 wxCommandEventHandler( DIALOG_IMPORT_GFX::onFilename ),
                                 NULL, this );
}


DIALOG_IMPORT_GFX::~DIALOG_IMPORT_GFX()
{
    auto cfg = m_parent->GetPcbNewSettings();

    cfg->m_ImportGraphics.layer                 = m_layer;
    cfg->m_ImportGraphics.interactive_placement = m_placementInteractive;
    cfg->m_ImportGraphics.last_file             = m_filename;
    cfg->m_ImportGraphics.dxf_line_width        = m_dxfLineWidth;
    cfg->m_ImportGraphics.dxf_line_width_units  = m_dxfLineWidthUnits;
    cfg->m_ImportGraphics.origin_units          = m_originUnits;
    cfg->m_ImportGraphics.origin_x              = m_origin.x;
    cfg->m_ImportGraphics.origin_y              = m_origin.y;
    cfg->m_ImportGraphics.dxf_units             = m_dxfUnits;

    m_textCtrlFileName->Disconnect( wxEVT_COMMAND_TEXT_UPDATED,
                                    wxCommandEventHandler( DIALOG_IMPORT_GFX::onFilename ),
                                    NULL, this );
}


void DIALOG_IMPORT_GFX::onFilename( wxCommandEvent& event )
{
    bool     enableDXFControls = true;
    wxString filename = m_textCtrlFileName->GetValue();

    if( auto plugin = m_gfxImportMgr->GetPluginByExt( wxFileName( filename ).GetExt() ) )
        enableDXFControls = dynamic_cast<DXF_IMPORT_PLUGIN*>( plugin.get() ) != nullptr;

    m_staticTextLineWidth->Enable( enableDXFControls );
    m_textCtrlLineWidth->Enable( enableDXFControls );
    m_choiceUnitLineWidth->Enable( enableDXFControls );

    m_staticTextLineWidth1->Enable( enableDXFControls );
    m_choiceDxfUnits->Enable( enableDXFControls );
}


void DIALOG_IMPORT_GFX::onUnitPositionSelection( wxCommandEvent& event )
{
    // Collect last entered values:
    updatePcbImportOffsets_mm();

    m_originUnits = m_DxfPcbPositionUnits->GetSelection();;
    showPcbImportOffsets();
}


double DIALOG_IMPORT_GFX::getDXFDefaultLineWidthMM()
{
    double value = DoubleValueFromString( EDA_UNITS::UNSCALED, m_textCtrlLineWidth->GetValue() );

    switch( m_dxfLineWidthUnits )
    {
    default:
    case 0:                       break;  // display units = mm
    case 1: value *= 25.4 / 1000; break;  // display units = mil
    case 2: value *= 25.4;        break;  // display units = inch
    }

    return value;   // value is in mm
}


void DIALOG_IMPORT_GFX::onUnitWidthSelection( wxCommandEvent& event )
{
    m_dxfLineWidth = getDXFDefaultLineWidthMM();

    // Switch to new units
    m_dxfLineWidthUnits = m_choiceUnitLineWidth->GetSelection();
    showDXFDefaultLineWidth();
}


void DIALOG_IMPORT_GFX::showPcbImportOffsets()
{
    // Display m_origin value according to the unit selection:
    VECTOR2D offset = m_origin;

    if( m_originUnits )   // Units are inches
        offset = m_origin / 25.4;

    m_DxfPcbXCoord->SetValue( wxString::Format( wxT( "%f" ), offset.x ) );
    m_DxfPcbYCoord->SetValue( wxString::Format( wxT( "%f" ), offset.y ) );

}


void DIALOG_IMPORT_GFX::showDXFDefaultLineWidth()
{
    double value;

    switch( m_dxfLineWidthUnits )
    {
    default:
    case 0: value = m_dxfLineWidth;               break;  // display units = mm
    case 1: value = m_dxfLineWidth / 25.4 * 1000; break;  // display units = mil
    case 2: value = m_dxfLineWidth / 25.4;        break;  // display units = inch
    }

    m_textCtrlLineWidth->SetValue( wxString::Format( wxT( "%f" ), value ) );
}


void DIALOG_IMPORT_GFX::onBrowseFiles( wxCommandEvent& event )
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

    for( GRAPHICS_IMPORT_MGR::GFX_FILE_T pluginType : m_gfxImportMgr->GetImportableFileTypes() )
    {
        std::unique_ptr<GRAPHICS_IMPORT_PLUGIN> plugin = m_gfxImportMgr->GetPlugin( pluginType );
        const std::vector<std::string>          extensions = plugin->GetFileExtensions();

        wildcardsDesc += wxT( "|" ) + plugin->GetName() + AddFileExtListToFilter( extensions );
        allWildcards += plugin->GetWildcards() + wxT( ";" );
    }

    wildcardsDesc = _( "All supported formats|" ) + allWildcards + wildcardsDesc;

    wxFileDialog dlg( m_parent, _( "Open File" ), path, filename, wildcardsDesc,
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() != wxID_OK )
        return;

    wxString fileName = dlg.GetPath();

    if( fileName.IsEmpty() )
        return;

    m_filename = fileName;
    m_textCtrlFileName->SetValue( fileName );
}


bool DIALOG_IMPORT_GFX::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    m_filename = m_textCtrlFileName->GetValue();

    if( m_filename.IsEmpty() )
    {
        wxMessageBox( _( "No file selected!" ) );
        return false;
    }

    m_originUnits = m_DxfPcbPositionUnits->GetSelection();
    updatePcbImportOffsets_mm();      // Update m_originX and m_originY;

    m_layer = m_SelLayerBox->GetLayerSelection();

    if( m_layer < 0 )
    {
        wxMessageBox( _( "Please select a valid layer." ) );
        return false;
    }

    m_dxfLineWidthUnits = m_choiceUnitLineWidth->GetSelection();
    m_dxfLineWidth = getDXFDefaultLineWidthMM();

    m_dxfUnits = m_choiceDxfUnits->GetSelection();

    m_importer->SetLayer( PCB_LAYER_ID( m_layer ) );

    if( auto plugin = m_gfxImportMgr->GetPluginByExt( wxFileName( m_filename ).GetExt() ) )
    {
        DXF_IMPORT_PLUGIN* dxfPlugin = dynamic_cast<DXF_IMPORT_PLUGIN*>( plugin.get() );

        if( dxfPlugin )
        {
            auto it = dxfUnitsMap.begin();
            std::advance( it, m_dxfUnits );

            if( it == dxfUnitsMap.end() )
                dxfPlugin->SetUnit( DXF_IMPORT_UNITS::DEFAULT );
            else
                dxfPlugin->SetUnit( it->first );

            m_importer->SetLineWidthMM( m_dxfLineWidth );
        }
        else
        {
            m_importer->SetLineWidthMM( 0.0 );
        }

        // Set coordinates offset for import (offset is given in mm)
        m_importer->SetImportOffsetMM( m_origin );
        m_scaleImport = DoubleValueFromString( EDA_UNITS::UNSCALED,
                                               m_textCtrlImportScale->GetValue() );

        m_importer->SetPlugin( std::move( plugin ) );

        LOCALE_IO dummy;    // Ensure floats can be read.

        if( m_importer->Load( m_filename ) )
            m_importer->Import( m_scaleImport );

        // Get warning messages:
        wxString warnings = m_importer->GetMessages();

        // This isn't a fatal error so allow the dialog to close with wxID_OK.
        if( !warnings.empty() )
        {
            HTML_MESSAGE_BOX dlg( this, _( "Warning" ) );
            dlg.MessageSet( _( "Items in the imported file could not be handled properly." ) );
            warnings.Replace( wxT( "\n" ), wxT( "<br/>" ) );
            dlg.AddHTML_Text( warnings );
            dlg.ShowModal();
        }
    }
    else
    {
        wxMessageBox( _( "There is no plugin to handle this file type." ) );
        return false;
    }

    return true;
}


void DIALOG_IMPORT_GFX::originOptionOnUpdateUI( wxUpdateUIEvent& event )
{
    if( m_rbInteractivePlacement->GetValue() != m_placementInteractive )
        m_rbInteractivePlacement->SetValue( m_placementInteractive );

    if( m_rbAbsolutePlacement->GetValue() == m_placementInteractive )
        m_rbAbsolutePlacement->SetValue( not m_placementInteractive );

    m_DxfPcbPositionUnits->Enable( not m_placementInteractive );
    m_DxfPcbXCoord->Enable( not m_placementInteractive );
    m_DxfPcbYCoord->Enable( not m_placementInteractive );
}


void DIALOG_IMPORT_GFX::updatePcbImportOffsets_mm()
{
    m_origin.x = DoubleValueFromString( EDA_UNITS::UNSCALED, m_DxfPcbXCoord->GetValue() );
    m_origin.y = DoubleValueFromString( EDA_UNITS::UNSCALED, m_DxfPcbYCoord->GetValue() );

    if( m_originUnits )   // Units are inches
        m_origin = m_origin * 25.4;
}


