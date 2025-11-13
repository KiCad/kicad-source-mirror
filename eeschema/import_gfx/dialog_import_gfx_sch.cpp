/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include "dialog_import_gfx_sch.h"

#include <map>

#include <import_gfx/graphics_importer_lib_symbol.h>
#include <import_gfx/graphics_importer_sch.h>
#include <import_gfx/dxf_import_plugin.h>

#include <base_units.h>
#include <kiface_base.h>
#include <locale_io.h>
#include <bitmaps.h>
#include <wildcards_and_files_ext.h>

#include <eeschema_settings.h>
#include <sch_edit_frame.h>
#include <symbol_editor_settings.h>
#include <symbol_edit_frame.h>

#include <wx/filedlg.h>
#include <wx/msgdlg.h>

#include <dialogs/html_message_box.h>
#include <widgets/std_bitmap_button.h>

#include <memory>

// Static members of DIALOG_IMPORT_GFX_SCH, to remember the user's choices during the session
bool   DIALOG_IMPORT_GFX_SCH::m_placementInteractive = true;
double DIALOG_IMPORT_GFX_SCH::m_importScale          = 1.0;   // Do not change the imported items size


const std::map<DXF_IMPORT_UNITS, wxString> dxfUnitsMap = {
    { DXF_IMPORT_UNITS::INCH, _( "Inches" ) },
    { DXF_IMPORT_UNITS::MM,   _( "Millimeters" ) },
    { DXF_IMPORT_UNITS::MILS, _( "Mils" ) },
    { DXF_IMPORT_UNITS::CM,   _( "Centimeter" ) },
    { DXF_IMPORT_UNITS::FEET, _( "Feet" ) },
};


DIALOG_IMPORT_GFX_SCH::DIALOG_IMPORT_GFX_SCH( SCH_BASE_FRAME* aParent ) :
        DIALOG_IMPORT_GFX_SCH_BASE( aParent ),
        m_parent( aParent ),
        m_xOrigin( aParent, m_xLabel, m_xCtrl, m_xUnits ),
        m_yOrigin( aParent, m_yLabel, m_yCtrl, m_yUnits ),
        m_defaultLineWidth( aParent, m_lineWidthLabel, m_lineWidthCtrl, m_lineWidthUnits )
{
    m_browseButton->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );

    if( SYMBOL_EDIT_FRAME* symFrame = dynamic_cast<SYMBOL_EDIT_FRAME*>( aParent ) )
        m_importer = std::make_unique<GRAPHICS_IMPORTER_LIB_SYMBOL>( symFrame->GetCurSymbol(), symFrame->GetUnit() );
    else if( dynamic_cast<SCH_EDIT_FRAME*>( aParent ) )
        m_importer = std::make_unique<GRAPHICS_IMPORTER_SCH>();

    m_gfxImportMgr = std::make_unique<GRAPHICS_IMPORT_MGR>();

    wxCommandEvent dummy;
    onFilename( dummy );

    SetInitialFocus( m_textCtrlFileName );
    SetupStandardButtons();

    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
    Centre();

    m_textCtrlFileName->Connect( wxEVT_COMMAND_TEXT_UPDATED,
                                 wxCommandEventHandler( DIALOG_IMPORT_GFX_SCH::onFilename ), nullptr, this );
}


DIALOG_IMPORT_GFX_SCH::~DIALOG_IMPORT_GFX_SCH()
{
    m_textCtrlFileName->Disconnect( wxEVT_COMMAND_TEXT_UPDATED,
                                    wxCommandEventHandler( DIALOG_IMPORT_GFX_SCH::onFilename ), nullptr, this );
}


void DIALOG_IMPORT_GFX_SCH::SetFilenameOverride( const wxString& aFilenameOverride )
{
    m_filenameOverride = aFilenameOverride;
}


bool DIALOG_IMPORT_GFX_SCH::TransferDataToWindow()
{
    DIALOG_SHIM::TransferDataToWindow();

    // We have to set the filename field value here, otherwise it gets overwritten by state loading
    if( !m_filenameOverride.IsEmpty() )
        m_textCtrlFileName->SetValue( m_filenameOverride );

    return true;
}


void DIALOG_IMPORT_GFX_SCH::onFilename( wxCommandEvent& event )
{
    bool     enableDXFControls = true;
    wxString ext = wxFileName( m_textCtrlFileName->GetValue() ).GetExt();

    if( std::unique_ptr<GRAPHICS_IMPORT_PLUGIN> plugin = m_gfxImportMgr->GetPluginByExt( ext ) )
        enableDXFControls = dynamic_cast<DXF_IMPORT_PLUGIN*>( plugin.get() ) != nullptr;

    m_defaultLineWidth.Enable( enableDXFControls );

    m_staticTextLineWidth1->Enable( enableDXFControls );
    m_choiceDxfUnits->Enable( enableDXFControls );
}


void DIALOG_IMPORT_GFX_SCH::onBrowseFiles( wxCommandEvent& event )
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

    wxFileDialog dlg( m_parent, _( "Import Graphics" ), path, filename, wildcardsDesc,
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_OK && !dlg.GetPath().IsEmpty() )
        m_textCtrlFileName->SetValue( dlg.GetPath() );
}


bool DIALOG_IMPORT_GFX_SCH::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    if( m_textCtrlFileName->GetValue().IsEmpty() )
    {
        wxMessageBox( _( "No file selected!" ) );
        return false;
    }

    wxString ext = wxFileName( m_textCtrlFileName->GetValue() ).GetExt();
    double   scale = EDA_UNIT_UTILS::UI::DoubleValueFromString( m_importScaleCtrl->GetValue() );
    double           xscale = scale;
    double           yscale = scale;

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

            m_importer->SetLineWidthMM( schIUScale.IUTomm( m_defaultLineWidth.GetValue() ) );
        }
        else
        {
            m_importer->SetLineWidthMM( 0.0 );
        }

        m_importer->SetPlugin( std::move( plugin ) );
        m_importer->SetImportOffsetMM( { schIUScale.IUTomm( origin.x ), schIUScale.IUTomm( origin.y ) } );

        LOCALE_IO dummy;    // Ensure floats can be read.

        if( m_importer->Load( m_textCtrlFileName->GetValue() ) )
            m_importer->Import( VECTOR2D( xscale, yscale ) );

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


void DIALOG_IMPORT_GFX_SCH::originOptionOnUpdateUI( wxUpdateUIEvent& event )
{
    if( m_rbInteractivePlacement->GetValue() != m_placementInteractive )
        m_rbInteractivePlacement->SetValue( m_placementInteractive );

    if( m_rbAbsolutePlacement->GetValue() == m_placementInteractive )
        m_rbAbsolutePlacement->SetValue( !m_placementInteractive );

    m_xOrigin.Enable( !m_placementInteractive );
    m_yOrigin.Enable( !m_placementInteractive );
}


