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

#include "dialogs/dialog_export_2581.h"

#include <wx/choicdlg.h>
#include <wx/filedlg.h>
#include <kiplatform/ui.h>

#include <board.h>
#include <pcb_edit_frame.h>
#include <project.h>
#include <project/project_file.h>
#include <pcb_io/ipc2581/ipc2581_export_job.h>
#include <pcb_io/ipc2581/pcb_io_ipc2581.h>
#include <widgets/wx_html_report_panel.h>
#include <widgets/wx_progress_reporters.h>
#include <tools/zone_filler_tool.h>
#include <tool/tool_manager.h>
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

    // The messages panel uses a negative min width so it doesn't drive the dialog width.
    // Ensure the dialog is at least wide enough for the standard buttons and the messages
    // panel's internal controls (filter checkboxes and Save button).
    int btnWidth = m_stdButtons->GetMinSize().GetWidth() + 10;
    int panelWidth = m_messagesPanel->GetBestSize().GetWidth() + 10;
    int minWidth = std::max( btnWidth, panelWidth );
    wxSize dialogMin = GetMinSize();

    if( dialogMin.GetWidth() < minWidth )
    {
        SetMinSize( wxSize( minWidth, dialogMin.GetHeight() ) );
        SetSize( wxSize( std::max( GetSize().GetWidth(), minWidth ), GetSize().GetHeight() ) );
    }
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

    // The messages panel uses a negative min width so it doesn't drive the dialog width.
    // Ensure the dialog is at least wide enough for the standard buttons and the messages
    // panel's internal controls (filter checkboxes and Save button).
    int btnWidth = m_stdButtons->GetMinSize().GetWidth() + 10;
    int panelWidth = m_messagesPanel->GetBestSize().GetWidth() + 10;
    int minWidth = std::max( btnWidth, panelWidth );
    wxSize dialogMin = GetMinSize();

    if( dialogMin.GetWidth() < minWidth )
    {
        SetMinSize( wxSize( minWidth, dialogMin.GetHeight() ) );
        SetSize( wxSize( std::max( GetSize().GetWidth(), minWidth ), GetSize().GetHeight() ) );
    }
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

    KIPLATFORM::UI::AllowNetworkFileSystems( &dlg );

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


void DIALOG_EXPORT_2581::onOKClick( wxCommandEvent& event )
{
    if( m_job )
    {
        if( TransferDataFromWindow() )
            EndModal( wxID_OK );

        return;
    }

    saveToProject();

    JOB_EXPORT_PCB_IPC2581 job;
    m_job = &job;

    TransferDataFromWindow();

    m_job = nullptr;

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

    m_parent->GetToolManager()->GetTool<ZONE_FILLER_TOOL>()->CheckAllZones( this );

    WX_PROGRESS_REPORTER progress( this, _( "Generate IPC-2581 File" ),
                                   PCB_IO_IPC2581::EXPORT_PHASES, PR_CAN_ABORT );

    if( !GenerateIpc2581File( job, m_parent->GetBoard(), &progress, &reporter ) )
        return;

    reporter.Report( _( "IPC-2581 file generated successfully." ), RPT_SEVERITY_ACTION );
}


void DIALOG_EXPORT_2581::init()
{
    m_contentPanel->Configure( FAB_CONTENT_FORMAT::IPC2581, m_parent->GetBoard() );
}


bool DIALOG_EXPORT_2581::TransferDataToWindow()
{
    IPC2581_BOM_FIELDS bomFields;

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
        m_choiceUnits->SetSelection( m_job->m_units == JOB_EXPORT_PCB_FAB::UNITS::MM ? 0 : 1 );
        m_precision->SetValue( static_cast<int>( m_job->m_precision ) );
        m_versionChoice->SetSelection( m_job->m_version == JOB_EXPORT_PCB_IPC2581::IPC2581_VERSION::B ? 0 : 1 );
        m_cbCompress->SetValue( m_job->m_compress );
        m_outputFileName->SetValue( m_job->GetConfiguredOutputPath() );
    }

    wxCommandEvent dummy;
    onCompressCheck( dummy );

    PROJECT_FILE& prj = Prj().GetProjectFile();

    if( !m_job )
    {
        bomFields.m_internalId = prj.m_IP2581Bom.id;
        bomFields.m_mfgPn = prj.m_IP2581Bom.MPN;
        bomFields.m_mfg = prj.m_IP2581Bom.mfg;
        bomFields.m_distPn = prj.m_IP2581Bom.distPN;
        bomFields.m_dist = prj.m_IP2581Bom.dist;
        bomFields.m_revision = prj.m_IP2581Bom.bomRev.IsEmpty() ? prj.m_IP2581Bom.schRevision : prj.m_IP2581Bom.bomRev;

        if( std::optional<IPC2581::MODE> mode = IPC2581::ModeFromToken( prj.m_IP2581Bom.mode ) )
            m_contentPanel->SetDataSet( *mode );

        m_contentPanel->SetSectionKey( prj.m_IP2581Bom.sections );

        m_contentPanel->SetNetNames( prj.m_IP2581Bom.netNames == wxS( "anonymize" )
                                             ? JOB_EXPORT_PCB_FAB::NET_NAMES::ANONYMIZE
                                             : JOB_EXPORT_PCB_FAB::NET_NAMES::INCLUDE );
        m_contentPanel->SetRefDes( prj.m_IP2581Bom.refDes == wxS( "omit" )
                                           ? JOB_EXPORT_PCB_IPC2581::REF_DES::OMIT
                                           : JOB_EXPORT_PCB_IPC2581::REF_DES::INCLUDE );
    }
    else
    {
        bomFields.m_internalId = m_job->m_colInternalId;
        bomFields.m_mfgPn = m_job->m_colMfgPn;
        bomFields.m_mfg = m_job->m_colMfg;
        bomFields.m_distPn = m_job->m_colDistPn;
        bomFields.m_dist = m_job->m_colDist;
        bomFields.m_revision = m_job->m_bomRev;

        m_contentPanel->SetDataSet( m_job->m_dataSet );

        m_contentPanel->SetSectionKey( m_job->m_sections );

        m_contentPanel->SetNetNames( m_job->m_netNames );
        m_contentPanel->SetRefDes( m_job->m_refDes );
        m_contentPanel->SetVariantNames( m_job->m_variantNames );
    }

    m_contentPanel->SetBomFields( bomFields );

    return true;
}


void DIALOG_EXPORT_2581::saveToProject()
{
    PROJECT_FILE& prj = Prj().GetProjectFile();
    const IPC2581_BOM_FIELDS& bomFields = m_contentPanel->GetBomFields();

    prj.m_IP2581Bom.id = bomFields.m_internalId;
    prj.m_IP2581Bom.mfg = bomFields.m_mfg;
    prj.m_IP2581Bom.MPN = bomFields.m_mfgPn;
    prj.m_IP2581Bom.distPN = bomFields.m_distPn;
    prj.m_IP2581Bom.dist = bomFields.m_dist;
    prj.m_IP2581Bom.bomRev = bomFields.m_revision;
    prj.m_IP2581Bom.mode = IPC2581::ModeToken( m_contentPanel->GetDataSet() );
    prj.m_IP2581Bom.sections = m_contentPanel->GetSectionKey();
    prj.m_IP2581Bom.netNames = m_contentPanel->GetNetNames() == JOB_EXPORT_PCB_FAB::NET_NAMES::ANONYMIZE
                                       ? wxS( "anonymize" ) : wxS( "include" );
    prj.m_IP2581Bom.refDes = m_contentPanel->GetRefDes() == JOB_EXPORT_PCB_IPC2581::REF_DES::OMIT
                                     ? wxS( "omit" ) : wxS( "include" );
}


bool DIALOG_EXPORT_2581::TransferDataFromWindow()
{
    if( m_job )
    {
        const IPC2581_BOM_FIELDS& bomFields = m_contentPanel->GetBomFields();
        m_job->SetConfiguredOutputPath( m_outputFileName->GetValue() );

        m_job->m_colInternalId = bomFields.m_internalId;
        m_job->m_colDist = bomFields.m_dist;
        m_job->m_colDistPn = bomFields.m_distPn;
        m_job->m_colMfg = bomFields.m_mfg;
        m_job->m_colMfgPn = bomFields.m_mfgPn;
        m_job->m_bomRev = bomFields.m_revision;

        m_job->m_version = GetVersion() == 'B' ? JOB_EXPORT_PCB_IPC2581::IPC2581_VERSION::B
											   : JOB_EXPORT_PCB_IPC2581::IPC2581_VERSION::C;
        m_job->m_units = GetUnitsString() == wxT( "mm" ) ? JOB_EXPORT_PCB_FAB::UNITS::MM
                                                        : JOB_EXPORT_PCB_FAB::UNITS::INCH;
        m_job->m_precision = m_precision->GetValue();
        m_job->m_compress = GetCompress();
        m_job->m_dataSet = m_contentPanel->GetDataSet();
        m_job->m_netNames = m_contentPanel->GetNetNames();
        m_job->m_refDes = m_contentPanel->GetRefDes();

        m_job->m_sections = m_contentPanel->GetSectionKey();
        m_job->m_variantNames = m_contentPanel->GetVariantNames();
    }

    return true;
}
