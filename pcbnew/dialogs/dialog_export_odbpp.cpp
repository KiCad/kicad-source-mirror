/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "dialogs/dialog_export_odbpp.h"

#include <wx/filedlg.h>
#include <kiplatform/ui.h>

#include <confirm.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <pgm_base.h>
#include <project.h>
#include <string_utils.h>
#include <widgets/std_bitmap_button.h>
#include <jobs/job_export_pcb_odb.h>


DIALOG_EXPORT_ODBPP::DIALOG_EXPORT_ODBPP( PCB_EDIT_FRAME* aParent ) :
        DIALOG_EXPORT_ODBPP_BASE( aParent ),
        m_parent( aParent ),
        m_job( nullptr )
{
    m_browseButton->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );

    SetupStandardButtons();

    // DIALOG_SHIM needs a unique hash_key because classname will be the same for both job and
    // non-job versions.
    m_hash_key = TO_UTF8( GetTitle() );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


DIALOG_EXPORT_ODBPP::DIALOG_EXPORT_ODBPP( JOB_EXPORT_PCB_ODB* aJob, PCB_EDIT_FRAME* aEditFrame,
                                          wxWindow* aParent ) :
        DIALOG_EXPORT_ODBPP_BASE( aParent ),
        m_parent( aEditFrame ),
        m_job( aJob )
{
    m_browseButton->Hide();

    SetupStandardButtons();

    // DIALOG_SHIM needs a unique hash_key because classname will be the same for both job and
    // non-job versions.
    m_hash_key = TO_UTF8( GetTitle() );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


bool DIALOG_EXPORT_ODBPP::TransferDataToWindow()
{
    if( !m_job )
    {
        if( m_outputFileName->GetValue().IsEmpty() )
        {
            wxFileName brdFile( m_parent->GetBoard()->GetFileName() );
            wxFileName odbFile( brdFile.GetPath(), wxString::Format( wxS( "%s-odb" ), brdFile.GetName() ),
                                FILEEXT::ArchiveFileExtension );

            m_outputFileName->SetValue( odbFile.GetFullPath() );
            OnFmtChoiceOptionChanged();
        }
    }
    else
    {
        SetTitle( m_job->GetSettingsDialogTitle() );

        m_choiceUnits->SetSelection( static_cast<int>( m_job->m_units ) );
        m_precision->SetValue( m_job->m_precision );
        m_choiceCompress->SetSelection( static_cast<int>( m_job->m_compressionMode ) );
        m_outputFileName->SetValue( m_job->GetConfiguredOutputPath() );
    }

    return true;
}


void DIALOG_EXPORT_ODBPP::onBrowseClicked( wxCommandEvent& event )
{
    // clang-format off
    wxString filter = _( "zip files" )
                      + AddFileExtListToFilter( { FILEEXT::ArchiveFileExtension } ) + "|"
                      + _( "tgz files" )
                      + AddFileExtListToFilter( { "tgz" } );
    // clang-format on

    // Build the absolute path of current output directory to preselect it in the file browser.
    wxString   path = ExpandEnvVarSubstitutions( m_outputFileName->GetValue(), &Prj() );
    wxFileName fn( Prj().AbsolutePath( path ) );

    wxFileName brdFile( m_parent->GetBoard()->GetFileName() );

    wxString fileDialogName( wxString::Format( wxS( "%s-odb" ), brdFile.GetName() ) );

    wxFileDialog dlg( this, _( "Export ODB++ File" ), fn.GetPath(), fileDialogName, filter, wxFD_SAVE );

    KIPLATFORM::UI::AllowNetworkFileSystems( &dlg );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    path = dlg.GetPath();

    fn = wxFileName( path );

    if( fn.GetExt().Lower() == "zip" )
    {
        m_choiceCompress->SetSelection( static_cast<int>( JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::ZIP ) );
    }
    else if( fn.GetExt().Lower() == "tgz" )
    {
        m_choiceCompress->SetSelection( static_cast<int>( JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::TGZ ) );
    }
    else if( path.EndsWith( "/" ) || path.EndsWith( "\\" ) )
    {
        m_choiceCompress->SetSelection( static_cast<int>( JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::NONE ) );
    }
    else
    {
        DisplayErrorMessage( this, _( "The selected output file name is not a supported archive format." ) );
        return;
    }

    m_outputFileName->SetValue( path );
}


void DIALOG_EXPORT_ODBPP::onFormatChoice( wxCommandEvent& event )
{
    OnFmtChoiceOptionChanged();
}


void DIALOG_EXPORT_ODBPP::OnFmtChoiceOptionChanged()
{
    wxString fn = m_outputFileName->GetValue();

    wxFileName fileName( fn );

    auto compressionMode = static_cast<JOB_EXPORT_PCB_ODB::ODB_COMPRESSION>( m_choiceCompress->GetSelection() );

    int sepIdx = std::max( fn.Find( '/', true ), fn.Find( '\\', true ) );
    int dotIdx = fn.Find( '.', true );

    if( fileName.IsDir() )
        fn = fn.Mid( 0, sepIdx );
    else if( sepIdx < dotIdx )
        fn = fn.Mid( 0, dotIdx );

    switch( compressionMode )
    {
    case JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::ZIP:
        fn = fn + '.' + FILEEXT::ArchiveFileExtension;
        break;
    case JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::TGZ:
        fn += ".tgz";
        break;
    case JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::NONE:
        fn = wxFileName( fn, "" ).GetFullPath();
        break;
    default:
        break;
    };

    m_outputFileName->SetValue( fn );
}


void DIALOG_EXPORT_ODBPP::onOKClick( wxCommandEvent& event )
{
    if( !m_job )
    {
        wxString fn = m_outputFileName->GetValue();

        if( fn.IsEmpty() )
        {
            DisplayErrorMessage( this, _( "Output file name cannot be empty." ) );
            return;
        }

        auto compressionMode = static_cast<JOB_EXPORT_PCB_ODB::ODB_COMPRESSION>( m_choiceCompress->GetSelection() );

        wxFileName fileName( fn );
        bool       isDirectory = fileName.IsDir();
        wxString   extension = fileName.GetExt();

        if( ( compressionMode == JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::NONE && !isDirectory )
            || ( compressionMode == JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::ZIP && extension != "zip" )
            || ( compressionMode == JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::TGZ && extension != "tgz" ) )
        {
            DisplayErrorMessage( this, _( "The output file name conflicts with the selected compression format." ) );
            return;
        }
    }

    event.Skip();
}


bool DIALOG_EXPORT_ODBPP::TransferDataFromWindow()
{
    if( m_job )
    {
        m_job->SetConfiguredOutputPath( m_outputFileName->GetValue() );

        m_job->m_precision = m_precision->GetValue();
        m_job->m_units = static_cast<JOB_EXPORT_PCB_ODB::ODB_UNITS>( m_choiceUnits->GetSelection() );
        m_job->m_compressionMode = static_cast<JOB_EXPORT_PCB_ODB::ODB_COMPRESSION>( m_choiceCompress->GetSelection() );
    }

    return true;
}
