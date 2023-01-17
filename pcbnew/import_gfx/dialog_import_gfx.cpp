/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <base_units.h>
#include <kiface_base.h>
#include <locale_io.h>
#include <pcb_layer_box_selector.h>
#include <wildcards_and_files_ext.h>
#include <bitmaps.h>
#include <widgets/std_bitmap_button.h>
#include <map>
#include "dxf_import_plugin.h"
#include <wx/filedlg.h>
#include <wx/msgdlg.h>

#include <memory>

// Static members of DIALOG_IMPORT_GFX, to remember the user's choices during the session
bool   DIALOG_IMPORT_GFX::m_placementInteractive = true;
bool   DIALOG_IMPORT_GFX::m_shouldGroupItems     = true;
double DIALOG_IMPORT_GFX::m_importScale          = 1.0;   // Do not change the imported items size


const std::map<DXF_IMPORT_UNITS, wxString> dxfUnitsMap = {
    { DXF_IMPORT_UNITS::INCHES,      _( "Inches" ) },
    { DXF_IMPORT_UNITS::MILLIMETERS, _( "Millimeters" ) },
    { DXF_IMPORT_UNITS::MILS,        _( "Mils" ) },
    { DXF_IMPORT_UNITS::CENTIMETERS, _( "Centimeter" ) },
    { DXF_IMPORT_UNITS::FEET,        _( "Feet" ) },
};


DIALOG_IMPORT_GFX::DIALOG_IMPORT_GFX( PCB_BASE_FRAME* aParent, bool aImportAsFootprintGraphic ) :
        DIALOG_IMPORT_GFX_BASE( aParent ),
        m_parent( aParent ),
        m_xOrigin( aParent, m_xLabel, m_xCtrl, m_xUnits ),
        m_yOrigin( aParent, m_yLabel, m_yCtrl, m_yUnits ),
        m_defaultLineWidth( aParent, m_lineWidthLabel, m_lineWidthCtrl, m_lineWidthUnits )
{
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

    PCBNEW_SETTINGS* cfg = m_parent->GetPcbNewSettings();

    m_placementInteractive = cfg->m_ImportGraphics.interactive_placement;

    m_xOrigin.SetValue( cfg->m_ImportGraphics.origin_x * pcbIUScale.IU_PER_MM );
    m_yOrigin.SetValue( cfg->m_ImportGraphics.origin_y * pcbIUScale.IU_PER_MM );
    m_defaultLineWidth.SetValue( cfg->m_ImportGraphics.dxf_line_width * pcbIUScale.IU_PER_MM );

    m_textCtrlFileName->SetValue( cfg->m_ImportGraphics.last_file );
    m_rbInteractivePlacement->SetValue( m_placementInteractive );
    m_rbAbsolutePlacement->SetValue( !m_placementInteractive );
    m_groupItems->SetValue( m_shouldGroupItems );

    m_importScaleCtrl->SetValue( wxString::Format( wxT( "%f" ), m_importScale ) );

    // Configure the layers list selector
    m_SelLayerBox->SetLayersHotkeys( false );    // Do not display hotkeys
    m_SelLayerBox->SetBoardFrame( m_parent );
    m_SelLayerBox->Resync();

    if( m_SelLayerBox->SetLayerSelection( cfg->m_ImportGraphics.layer ) < 0 )
        m_SelLayerBox->SetLayerSelection( Dwgs_User );

    for( const std::pair<const DXF_IMPORT_UNITS, wxString>& unitEntry : dxfUnitsMap )
        m_choiceDxfUnits->Append( unitEntry.second );

    m_choiceDxfUnits->SetSelection( cfg->m_ImportGraphics.dxf_units );

    m_browseButton->SetBitmap( KiBitmap( BITMAPS::small_folder ) );

    wxCommandEvent dummy;
    onFilename( dummy );

    SetInitialFocus( m_textCtrlFileName );
    SetupStandardButtons();

    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
    Centre();

    m_textCtrlFileName->Connect( wxEVT_COMMAND_TEXT_UPDATED,
                                 wxCommandEventHandler( DIALOG_IMPORT_GFX::onFilename ),
                                 nullptr, this );
}


DIALOG_IMPORT_GFX::~DIALOG_IMPORT_GFX()
{
    PCBNEW_SETTINGS* cfg = m_parent->GetPcbNewSettings();

    cfg->m_ImportGraphics.layer                 = m_SelLayerBox->GetLayerSelection();
    cfg->m_ImportGraphics.interactive_placement = m_placementInteractive;
    cfg->m_ImportGraphics.last_file             = m_textCtrlFileName->GetValue();
    cfg->m_ImportGraphics.dxf_line_width        = pcbIUScale.IUTomm( m_defaultLineWidth.GetValue() );
    cfg->m_ImportGraphics.origin_x              = pcbIUScale.IUTomm( m_xOrigin.GetValue() );
    cfg->m_ImportGraphics.origin_y              = pcbIUScale.IUTomm( m_yOrigin.GetValue() );
    cfg->m_ImportGraphics.dxf_units             = m_choiceDxfUnits->GetSelection();

    m_importScale = EDA_UNIT_UTILS::UI::DoubleValueFromString( m_importScaleCtrl->GetValue() );

    m_textCtrlFileName->Disconnect( wxEVT_COMMAND_TEXT_UPDATED,
                                    wxCommandEventHandler( DIALOG_IMPORT_GFX::onFilename ),
                                    nullptr, this );
}


void DIALOG_IMPORT_GFX::onFilename( wxCommandEvent& event )
{
    bool     enableDXFControls = true;
    wxString ext = wxFileName( m_textCtrlFileName->GetValue() ).GetExt();

    if( std::unique_ptr<GRAPHICS_IMPORT_PLUGIN> plugin = m_gfxImportMgr->GetPluginByExt( ext ) )
        enableDXFControls = dynamic_cast<DXF_IMPORT_PLUGIN*>( plugin.get() ) != nullptr;

    m_defaultLineWidth.Enable( enableDXFControls );

    m_staticTextLineWidth1->Enable( enableDXFControls );
    m_choiceDxfUnits->Enable( enableDXFControls );
}


void DIALOG_IMPORT_GFX::onBrowseFiles( wxCommandEvent& event )
{
    wxString path;
    wxString filename = m_textCtrlFileName->GetValue();

    if( !filename.IsEmpty() )
    {
        wxFileName fn( filename );
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

    wildcardsDesc = _( "All supported formats" ) + wxT( "|" ) + allWildcards + wildcardsDesc;

    wxFileDialog dlg( m_parent, _( "Open File" ), path, filename, wildcardsDesc,
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_OK && !dlg.GetPath().IsEmpty() )
        m_textCtrlFileName->SetValue( dlg.GetPath() );
}


bool DIALOG_IMPORT_GFX::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    if( m_textCtrlFileName->GetValue().IsEmpty() )
    {
        wxMessageBox( _( "No file selected!" ) );
        return false;
    }

    if( m_SelLayerBox->GetLayerSelection() < 0 )
    {
        wxMessageBox( _( "Please select a valid layer." ) );
        return false;
    }

    PCBNEW_SETTINGS* cfg = m_parent->GetPcbNewSettings();
    wxString ext = wxFileName( m_textCtrlFileName->GetValue() ).GetExt();
    double   scale = EDA_UNIT_UTILS::UI::DoubleValueFromString( m_importScaleCtrl->GetValue() );
    double           xscale = scale;
    double           yscale = scale;

    if( cfg->m_Display.m_DisplayInvertXAxis )
    {
        xscale *= -1.0;
    }

    if( cfg->m_Display.m_DisplayInvertYAxis )
    {
        yscale *= -1.0;
    }

    VECTOR2D origin( m_xOrigin.GetValue() / xscale, m_yOrigin.GetValue() / yscale );

    if( std::unique_ptr<GRAPHICS_IMPORT_PLUGIN> plugin = m_gfxImportMgr->GetPluginByExt( ext ) )
    {
        if( DXF_IMPORT_PLUGIN* dxfPlugin = dynamic_cast<DXF_IMPORT_PLUGIN*>( plugin.get() ) )
        {
            auto it = dxfUnitsMap.begin();
            std::advance( it, m_choiceDxfUnits->GetSelection() );

            if( it == dxfUnitsMap.end() )
                dxfPlugin->SetUnit( DXF_IMPORT_UNITS::DEFAULT );
            else
                dxfPlugin->SetUnit( it->first );

            m_importer->SetLineWidthMM( pcbIUScale.IUTomm( m_defaultLineWidth.GetValue() ) );
        }
        else
        {
            m_importer->SetLineWidthMM( 0.0 );
        }

        m_importer->SetPlugin( std::move( plugin ) );
        m_importer->SetLayer( PCB_LAYER_ID( m_SelLayerBox->GetLayerSelection() ) );
        m_importer->SetImportOffsetMM( { pcbIUScale.IUTomm( origin.x ), pcbIUScale.IUTomm( origin.y ) } );

        LOCALE_IO dummy;    // Ensure floats can be read.

        if( m_importer->Load( m_textCtrlFileName->GetValue() ) )
            m_importer->Import( scale );

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

        return true;
    }
    else
    {
        wxMessageBox( _( "There is no plugin to handle this file type." ) );
        return false;
    }
}


void DIALOG_IMPORT_GFX::originOptionOnUpdateUI( wxUpdateUIEvent& event )
{
    if( m_rbInteractivePlacement->GetValue() != m_placementInteractive )
        m_rbInteractivePlacement->SetValue( m_placementInteractive );

    if( m_rbAbsolutePlacement->GetValue() == m_placementInteractive )
        m_rbAbsolutePlacement->SetValue( !m_placementInteractive );

    m_xOrigin.Enable( !m_placementInteractive );
    m_yOrigin.Enable( !m_placementInteractive );
}


