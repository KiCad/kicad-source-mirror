/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <dialogs/dialog_export_odbpp.h>

#include <board.h>
#include <footprint.h>
#include <kiway_holder.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <pgm_base.h>
#include <project.h>
#include <project/board_project_settings.h>
#include <project/project_file.h>
#include <settings/settings_manager.h>
#include <widgets/std_bitmap_button.h>

#include <set>
#include <vector>
#include <wx/dirdlg.h>

static wxString s_oemColumn = wxEmptyString;

DIALOG_EXPORT_ODBPP::DIALOG_EXPORT_ODBPP( PCB_EDIT_FRAME* aParent ) :
        DIALOG_EXPORT_ODBPP_BASE( aParent ), m_parent( aParent )
{
    m_browseButton->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );

    SetupStandardButtons( { { wxID_OK, _( "Export" ) }, { wxID_CANCEL, _( "Close" ) } } );

    wxString path = m_parent->GetLastPath( LAST_PATH_ODBPP );

    if( path.IsEmpty() )
    {
        wxFileName brdFile( m_parent->GetBoard()->GetFileName() );
        path = brdFile.GetPath();
    }

    m_outputFileName->SetValue( path );

    // Fill wxChoice (and others) items with data before calling finishDialogSettings()
    // to calculate suitable widgets sizes
    Init();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


void DIALOG_EXPORT_ODBPP::onBrowseClicked( wxCommandEvent& event )
{
    // Build the absolute path of current output directory to preselect it in the file browser.
    wxString   path = ExpandEnvVarSubstitutions( m_outputFileName->GetValue(), &Prj() );
    wxFileName fn( Prj().AbsolutePath( path ) );

    wxDirDialog dlg( this, _( "Export ODB++ File" ), fn.GetPath() );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    m_outputFileName->SetValue( dlg.GetPath() );
}


void DIALOG_EXPORT_ODBPP::onOKClick( wxCommandEvent& event )
{
    m_parent->SetLastPath( LAST_PATH_ODBPP, m_outputFileName->GetValue() );

    event.Skip();
}


bool DIALOG_EXPORT_ODBPP::Init()
{
    PCBNEW_SETTINGS* cfg = Pgm().GetSettingsManager().GetAppSettings<PCBNEW_SETTINGS>();

    m_choiceUnits->SetSelection( cfg->m_ExportODBPP.units );
    m_precision->SetValue( cfg->m_ExportODBPP.precision );
    m_cbCompress->SetValue( cfg->m_ExportODBPP.compress );

    return true;
}


bool DIALOG_EXPORT_ODBPP::TransferDataFromWindow()
{
    PCBNEW_SETTINGS* cfg = Pgm().GetSettingsManager().GetAppSettings<PCBNEW_SETTINGS>();

    cfg->m_ExportODBPP.units = m_choiceUnits->GetSelection();
    cfg->m_ExportODBPP.precision = m_precision->GetValue();
    cfg->m_ExportODBPP.compress = m_cbCompress->GetValue();

    return true;
}
