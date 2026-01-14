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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "dialogs/dialog_export_2581.h"

#include <set>
#include <map>
#include <vector>

#include <wx/filedlg.h>
#include <wx/filefn.h>

#include <board.h>
#include <footprint.h>
#include <kiway_holder.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <pgm_base.h>
#include <project.h>
#include <project/project_file.h>
#include <pcb_io/pcb_io_mgr.h>
#include <widgets/wx_html_report_panel.h>
#include <widgets/wx_progress_reporters.h>
#include <settings/settings_manager.h>
#include <string_utils.h>
#include <widgets/std_bitmap_button.h>
#include <jobs/job_export_pcb_ipc2581.h>
#include <wx_filename.h>



DIALOG_EXPORT_2581::DIALOG_EXPORT_2581( PCB_EDIT_FRAME* aParent ) :
        DIALOG_EXPORT_2581_BASE( aParent ),
        m_parent( aParent ),
        m_job( nullptr )
{
    m_browseButton->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );

    SetupStandardButtons( { { wxID_OK,     _( "Export" ) },
                            { wxID_CANCEL, _( "Close" )  } } );

    // DIALOG_SHIM needs a unique hash_key because classname will be the same for both job and
    // non-job versions.
    m_hash_key = TO_UTF8( GetTitle() );

    init();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


DIALOG_EXPORT_2581::DIALOG_EXPORT_2581( JOB_EXPORT_PCB_IPC2581* aJob, PCB_EDIT_FRAME* aEditFrame,
                                        wxWindow* aParent ) :
        DIALOG_EXPORT_2581_BASE( aParent ),
        m_parent( aEditFrame ),
        m_job( aJob )
{
    m_browseButton->Hide();

    SetupStandardButtons();

    SetTitle( m_job->GetSettingsDialogTitle() );

    // DIALOG_SHIM needs a unique hash_key because classname will be the same for both job and
    // non-job versions.
    m_hash_key = TO_UTF8( GetTitle() );

    init();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


void DIALOG_EXPORT_2581::onBrowseClicked( wxCommandEvent& event )
{
    // Build the absolute path of current output directory to preselect it in the file browser.
    wxString     path = ExpandEnvVarSubstitutions( m_outputFileName->GetValue(), &Prj() );
    wxFileName   fn( Prj().AbsolutePath( path ) );
    wxString     ipc_files = _( "IPC-2581 Files (*.xml)|*.xml" );
    wxString     compressed_files = _( "IPC-2581 Compressed Files (*.zip)|*.zip" );

    wxFileDialog dlg( this, _( "Export IPC-2581 File" ), fn.GetPath(), fn.GetFullName(),
                      m_cbCompress->IsChecked() ? compressed_files : ipc_files,
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    m_outputFileName->SetValue( dlg.GetPath() );

}

void DIALOG_EXPORT_2581::onCompressCheck( wxCommandEvent& event )
{
    if( m_cbCompress->GetValue() )
    {
        wxFileName fn = m_outputFileName->GetValue();

        fn.SetExt( "zip" );
        m_outputFileName->SetValue( fn.GetFullPath() );
    }
    else
    {
        wxFileName fn = m_outputFileName->GetValue();

        fn.SetExt( "xml" );
        m_outputFileName->SetValue( fn.GetFullPath() );
    }
}


void DIALOG_EXPORT_2581::onMfgPNChange( wxCommandEvent& event )
{
    if( event.GetSelection() == 0 )
    {
        m_choiceMfg->Enable( false );
    }
    else
    {
        m_choiceMfg->Enable( true );

        // Don't try to guess the manufacturer if the user has already selected one
        if( m_choiceMfg->GetSelection() > 0 )
            return;

        int it = 0;

        if( it = m_choiceMfg->FindString( wxT( "manufacturer" ) ); it != wxNOT_FOUND )
            m_choiceMfg->Select( it );
        else if( it = m_choiceMfg->FindString( _( "manufacturer" ) ); it != wxNOT_FOUND )
            m_choiceMfg->Select( it );
        else if( it = m_choiceMfg->FindString( wxT( "mfg" ) ); it != wxNOT_FOUND )
            m_choiceMfg->Select( it );
        else if( it = m_choiceMfg->FindString( _( "mfg" ) ); it != wxNOT_FOUND )
            m_choiceMfg->Select( it );
    }
}


void DIALOG_EXPORT_2581::onDistPNChange( wxCommandEvent& event )
{
    if( event.GetSelection() == 0 )
    {
        m_textDistributor->Enable( false );
        m_textDistributor->SetValue( _( "N/A" ) );
    }
    else
    {
        m_textDistributor->Enable( true );

        // Don't try to guess the distributor if the user has already selected one
        if( m_textDistributor->GetValue() != _( "N/A" ) )
            return;

        wxString dist = m_choiceDistPN->GetStringSelection();
        dist.MakeUpper();

        // Try to guess the distributor from the part number column

        if( dist.Contains( wxT( "DIGIKEY" ) ) )
        {
            m_textDistributor->SetValue( wxT( "Digi-Key" ) );
        }
        else if( dist.Contains( wxT( "DIGI-KEY" ) ) )
        {
            m_textDistributor->SetValue( wxT( "Digi-Key" ) );
        }
        else if( dist.Contains( wxT( "MOUSER" ) ) )
        {
            m_textDistributor->SetValue( wxT( "Mouser" ) );
        }
        else if( dist.Contains( wxT( "NEWARK" ) ) )
        {
            m_textDistributor->SetValue( wxT( "Newark" ) );
        }
        else if( dist.Contains( wxT( "RS COMPONENTS" ) ) )
        {
            m_textDistributor->SetValue( wxT( "RS Components" ) );
        }
        else if( dist.Contains( wxT( "FARNELL" ) ) )
        {
            m_textDistributor->SetValue( wxT( "Farnell" ) );
        }
        else if( dist.Contains( wxT( "ARROW" ) ) )
        {
            m_textDistributor->SetValue( wxT( "Arrow" ) );
        }
        else if( dist.Contains( wxT( "AVNET" ) ) )
        {
            m_textDistributor->SetValue( wxT( "Avnet" ) );
        }
        else if( dist.Contains( wxT( "TME" ) ) )
        {
            m_textDistributor->SetValue( wxT( "TME" ) );
        }
        else if( dist.Contains( wxT( "LCSC" ) ) )
        {
            m_textDistributor->SetValue( wxT( "LCSC" ) );
        }
    }
}


void DIALOG_EXPORT_2581::onOKClick( wxCommandEvent& event )
{
    if( m_job )
    {
        if( TransferDataFromWindow() )
            EndModal( wxID_OK );

        return;
    }

    if( !TransferDataFromWindow() )
        return;

    m_messagesPanel->Clear();

    REPORTER& reporter = m_messagesPanel->Reporter();

    wxFileName pcbFileName = GetOutputPath();
    WX_FILENAME::ResolvePossibleSymlinks( pcbFileName );

    if( pcbFileName.GetName().empty() )
    {
        reporter.Report( _( "The board must be saved before generating IPC-2581 file." ),
                         RPT_SEVERITY_ERROR );
        return;
    }

    if( !m_parent->IsWritable( pcbFileName ) )
    {
        reporter.Report( wxString::Format( _( "Insufficient permissions to write file '%s'." ),
                                           pcbFileName.GetFullPath() ),
                         RPT_SEVERITY_ERROR );
        return;
    }

    wxString tempFile = wxFileName::CreateTempFileName( wxS( "pcbnew_ipc" ) );

    WX_PROGRESS_REPORTER progress( this, _( "Generate IPC-2581 File" ), 5, PR_CAN_ABORT );

    std::map<std::string, UTF8> props;

    props[ "units" ] = TO_UTF8( GetUnitsString() );
    props[ "sigfig" ] = TO_UTF8( GetPrecision() );
    props[ "version" ] = TO_UTF8( wxString( GetVersion() ) );
    props[ "OEMRef" ] = TO_UTF8( GetOEM() );
    props[ "mpn" ] = TO_UTF8( GetMPN() );
    props[ "mfg" ] = TO_UTF8( GetMfg() );
    props[ "dist" ] = TO_UTF8( GetDist() );
    props[ "distpn" ] = TO_UTF8( GetDistPN() );

    try
    {
        IO_RELEASER<PCB_IO> pi( PCB_IO_MGR::FindPlugin( PCB_IO_MGR::IPC2581 ) );
        pi->SetProgressReporter( &progress );
        pi->SetReporter( &reporter );
        pi->SaveBoard( tempFile, m_parent->GetBoard(), &props );
    }
    catch( const IO_ERROR& ioe )
    {
        reporter.Report( wxString::Format( _( "Error generating IPC-2581 file '%s'.\n%s" ),
                                           pcbFileName.GetFullPath(), ioe.What() ),
                         RPT_SEVERITY_ERROR );

        wxRemoveFile( tempFile );
        return;
    }

    if( wxFileExists( pcbFileName.GetFullPath() ) )
        wxRemoveFile( pcbFileName.GetFullPath() );

    if( !wxRenameFile( tempFile, pcbFileName.GetFullPath() ) )
    {
        reporter.Report( wxString::Format( _( "Failed to create file '%s'." ), pcbFileName.GetFullPath() ),
                         RPT_SEVERITY_ERROR );
        wxRemoveFile( tempFile );
        return;
    }

    reporter.Report( _( "IPC-2581 file generated successfully." ), RPT_SEVERITY_ACTION );
}


void DIALOG_EXPORT_2581::init()
{
    m_textDistributor->SetSize( m_choiceDistPN->GetSize() );

    std::set<wxString> options;

    for( FOOTPRINT* fp : m_parent->GetBoard()->Footprints() )
    {
        for( PCB_FIELD* field : fp->GetFields() )
        {
            wxCHECK2( field, continue );

            options.insert( field->GetName() );
        }
    }

    std::vector<wxString> items( options.begin(), options.end() );
    m_oemRef->Append( items );
    m_choiceMPN->Append( items );
    m_choiceMfg->Append( items );
    m_choiceDistPN->Append( items );
}


bool DIALOG_EXPORT_2581::TransferDataToWindow()
{
    if( !m_job )
    {
        wxString path = m_outputFileName->GetValue();

        if( path.IsEmpty() )
        {
            wxFileName brdFile( m_parent->GetBoard()->GetFileName() );
            brdFile.SetExt( wxT( "xml" ) );
            path = brdFile.GetFullPath();
            m_outputFileName->SetValue( path );
        }
    }
    else
    {
        m_choiceUnits->SetSelection( m_job->m_units == JOB_EXPORT_PCB_IPC2581::IPC2581_UNITS::MM ? 0 : 1 );
        m_precision->SetValue( static_cast<int>( m_job->m_precision ) );
        m_versionChoice->SetSelection( m_job->m_version == JOB_EXPORT_PCB_IPC2581::IPC2581_VERSION::B ? 0 : 1 );
        m_cbCompress->SetValue( m_job->m_compress );
        m_outputFileName->SetValue( m_job->GetConfiguredOutputPath() );
    }

    wxCommandEvent dummy;
    onCompressCheck( dummy );

    PROJECT_FILE& prj = Prj().GetProjectFile();

    wxString internalIdCol;
    wxString mpnCol;
    wxString distPnCol;
    wxString mfgCol;
    wxString distCol;

    if( !m_job )
    {
        internalIdCol = prj.m_IP2581Bom.id;
        mpnCol = prj.m_IP2581Bom.MPN;
        distPnCol = prj.m_IP2581Bom.distPN;
        mfgCol = prj.m_IP2581Bom.mfg;
        distCol = prj.m_IP2581Bom.dist;
    }
    else
    {
        internalIdCol = m_job->m_colInternalId;
        mpnCol = m_job->m_colMfgPn;
        distPnCol = m_job->m_colDistPn;
        mfgCol = m_job->m_colMfg;
        distCol = m_job->m_colDist;
    }

    if( !m_choiceMPN->SetStringSelection( internalIdCol ) )
        m_choiceMPN->SetSelection( 0 );

    if( m_choiceMPN->SetStringSelection( mpnCol ) )
    {
        m_choiceMfg->Enable( true );

        if( !m_choiceMfg->SetStringSelection( mfgCol ) )
            m_choiceMfg->SetSelection( 0 );
    }
    else
    {
        m_choiceMPN->SetSelection( 0 );
        m_choiceMfg->SetSelection( 0 );
        m_choiceMfg->Enable( false );
    }

    if( m_choiceDistPN->SetStringSelection( distPnCol ) )
    {
        m_textDistributor->Enable( true );

        // The combo box selection can be fixed, so any value can be entered
        if( !prj.m_IP2581Bom.distPN.empty() )
        {
            m_textDistributor->SetValue( distCol );
        }
        else
        {
            wxCommandEvent evt;
            onDistPNChange( evt );
        }
    }
    else
    {
        m_choiceDistPN->SetSelection( 0 );
        m_textDistributor->SetValue( _( "N/A" ) );
        m_textDistributor->Enable( false );
    }

    return true;
}


bool DIALOG_EXPORT_2581::TransferDataFromWindow()
{
    if( !m_job )
    {
        PROJECT_FILE& prj = Prj().GetProjectFile();

        prj.m_IP2581Bom.id = GetOEM();
        prj.m_IP2581Bom.mfg = GetMfg();
        prj.m_IP2581Bom.MPN = GetMPN();
        prj.m_IP2581Bom.distPN = GetDistPN();
        prj.m_IP2581Bom.dist = GetDist();
    }
    else
    {
        m_job->SetConfiguredOutputPath( m_outputFileName->GetValue() );

        m_job->m_colInternalId = GetOEM();
        m_job->m_colDist = GetDist();
        m_job->m_colDistPn = GetDistPN();
        m_job->m_colMfg = GetMfg();
        m_job->m_colMfgPn = GetMPN();

        m_job->m_version = GetVersion() == 'B' ? JOB_EXPORT_PCB_IPC2581::IPC2581_VERSION::B
											   : JOB_EXPORT_PCB_IPC2581::IPC2581_VERSION::C;
        m_job->m_units = GetUnitsString() == wxT( "mm" ) ? JOB_EXPORT_PCB_IPC2581::IPC2581_UNITS::MM
														 : JOB_EXPORT_PCB_IPC2581::IPC2581_UNITS::INCH;
        m_job->m_precision = m_precision->GetValue();
        m_job->m_compress = GetCompress();
    }

    return true;
}
