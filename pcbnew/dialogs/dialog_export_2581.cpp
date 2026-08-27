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

#include <set>
#include <map>
#include <vector>

#include <wx/choicdlg.h>
#include <wx/filedlg.h>
#include <wx/filefn.h>
#include <kiplatform/ui.h>

#include <board.h>
#include <footprint.h>
#include <kiway_holder.h>
#include <paths.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <pgm_base.h>
#include <project.h>
#include <project/project_file.h>
#include <pcb_io/ipc2581/pcb_io_ipc2581.h>
#include <pcb_io/pcb_io_mgr.h>
#include <widgets/wx_html_report_panel.h>
#include <widgets/wx_progress_reporters.h>
#include <settings/settings_manager.h>
#include <string_utils.h>
#include <widgets/std_bitmap_button.h>
#include <jobs/job_export_pcb_ipc2581.h>
#include <kiplatform/io.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
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

    WX_PROGRESS_REPORTER progress( this, _( "Generate IPC-2581 File" ),
                                   PCB_IO_IPC2581::EXPORT_PHASES, PR_CAN_ABORT );

    if( !GenerateFile( job, m_parent->GetBoard(), &progress, &reporter ) )
        return;

    reporter.Report( _( "IPC-2581 file generated successfully." ), RPT_SEVERITY_ACTION );
}


bool DIALOG_EXPORT_2581::GenerateFile( JOB_EXPORT_PCB_IPC2581& aJob, BOARD* aBoard,
                                       PROGRESS_REPORTER* aProgressReporter, REPORTER* aReporter )
{
    wxCHECK( aBoard, false );
    wxString outPath = aJob.GetFullOutputPath( aBoard->GetProject() );

    if( !PATHS::EnsurePathExists( outPath, true ) )
    {
        if( aReporter )
            aReporter->Report( _( "Failed to create output directory\n" ), RPT_SEVERITY_ERROR );

        return false;
    }

    std::map<std::string, UTF8> props;
    props["units"] = aJob.m_units == JOB_EXPORT_PCB_IPC2581::IPC2581_UNITS::MM ? "mm" : "inch";
    props["sigfig"] = wxString::Format( "%d", aJob.m_precision );
    props["version"] = aJob.m_version == JOB_EXPORT_PCB_IPC2581::IPC2581_VERSION::C ? "C" : "B";
    props["OEMRef"] = aJob.m_colInternalId;
    props["mpn"] = aJob.m_colMfgPn;
    props["mfg"] = aJob.m_colMfg;
    props["dist"] = aJob.m_colDist;
    props["distpn"] = aJob.m_colDistPn;

    if( !aJob.m_mode.IsEmpty() )
        props["mode"] = aJob.m_mode;

    if( !aJob.m_sections.IsEmpty() )
        props["sections"] = aJob.m_sections;

    if( !aJob.m_netNamePolicy.IsEmpty() )
        props["netnames"] = aJob.m_netNamePolicy;

    if( !aJob.m_refDesPolicy.IsEmpty() )
        props["refdes"] = aJob.m_refDesPolicy;

    wxString bomRev = aJob.m_bomRev;

    if( bomRev.IsEmpty() && aBoard->GetProject() )
    {
        const IP2581_BOM& bomSettings = aBoard->GetProject()->GetProjectFile().m_IP2581Bom;
        bomRev = bomSettings.bomRev;

        if( bomRev.IsEmpty() )
            bomRev = bomSettings.schRevision;
    }

    if( !bomRev.IsEmpty() )
        props["bomrev"] = bomRev;

    wxString tempFile = wxFileName::CreateTempFileName( wxS( "pcbnew_ipc" ) );

    try
    {
        IO_RELEASER<PCB_IO> pi( PCB_IO_MGR::FindPlugin( PCB_IO_MGR::IPC2581 ) );
        pi->SetProgressReporter( aProgressReporter );
        pi->SetReporter( aReporter );
        pi->SaveBoard( tempFile, aBoard, &props );
    }
    catch( const IO_ERROR& ioe )
    {
        if( aReporter )
        {
            aReporter->Report( wxString::Format( _( "Error generating IPC-2581 file '%s'.\n%s" ),
                                                  aJob.m_filename,
                                                  ioe.What() ),
                                RPT_SEVERITY_ERROR );
        }

        wxRemoveFile( tempFile );

        return false;
    }

    if( aJob.m_compress )
    {
        wxFileName tempfn = outPath;
        tempfn.SetExt( FILEEXT::Ipc2581FileExtension );
        wxFileName zipfn = tempFile;
        zipfn.SetExt( "zip" );

        {
            wxFFileOutputStream fnout( zipfn.GetFullPath() );

            // Use a large I/O buffer to improve compatibility with cloud-synced folders.
            // See KIPLATFORM::IO::CLOUD_SYNC_BUFFER_SIZE comment for details.
            if( FILE* fp = fnout.GetFile()->fp() )
                setvbuf( fp, nullptr, _IOFBF, KIPLATFORM::IO::CLOUD_SYNC_BUFFER_SIZE );

            wxZipOutputStream   zip( fnout );
            wxFFileInputStream  fnin( tempFile );

            zip.PutNextEntry( tempfn.GetFullName() );
            fnin.Read( zip );
        }

        wxRemoveFile( tempFile );
        tempFile = zipfn.GetFullPath();
    }

    // If save succeeded, replace the original with what we just wrote
    if( !wxRenameFile( tempFile, outPath ) )
    {
        if( aReporter )
        {
            aReporter->Report( wxString::Format( _( "Error generating IPC-2581 file '%s'.\n"
                                                     "Failed to rename temporary file '%s." ),
                                                  outPath,
                                                  tempFile ),
                                RPT_SEVERITY_ERROR );
        }

        return false;
    }

    aJob.AddOutput( outPath );
    return true;
}


void DIALOG_EXPORT_2581::init()
{
    updateContentSummary();
}


IPC2581::SECTION_SET DIALOG_EXPORT_2581::resolvedSections() const
{
    IPC2581::MODE mode = GetDataSet();
    IPC2581::SECTION_SET requested;

    if( !m_sectionKey || !IPC2581::SectionSetFromKeyString( *m_sectionKey, requested ) )
        requested = IPC2581::RecommendedOptionalSections( mode );

    // Table 4 by itself only gives the schema sections
    return IPC2581::ResolveSections( IPC2581::REVISION::C, mode, requested ).m_included;
}


std::vector<std::pair<IPC2581::SECTION, wxString>> DIALOG_EXPORT_2581::sectionLabels()
{
    return {
        { IPC2581::SECTION::BOM_AVL, _( "BOM" ) },
        { IPC2581::SECTION::PACKAGES, _( "packages" ) },
        { IPC2581::SECTION::COMPONENTS, _( "components" ) },
        { IPC2581::SECTION::PADSTACKS, _( "padstacks" ) },
        { IPC2581::SECTION::STACKUP, _( "stackup" ) },
        { IPC2581::SECTION::PROFILE, _( "board profile" ) },
        { IPC2581::SECTION::SOLDERMASK, _( "solder mask" ) },
        { IPC2581::SECTION::SOLDERPASTE, _( "solder paste" ) },
        { IPC2581::SECTION::SILKSCREEN, _( "silkscreen" ) },
        { IPC2581::SECTION::DRILL_ROUT, _( "drill and router" ) },
        { IPC2581::SECTION::DOCUMENTATION, _( "documentation" ) },
        { IPC2581::SECTION::OUTER_COPPER, _( "outer copper" ) },
        { IPC2581::SECTION::INNER_COPPER, _( "inner copper" ) },
        { IPC2581::SECTION::DIELECTRIC, _( "dielectric" ) },
        { IPC2581::SECTION::MISC_FAB, _( "misc fab layers" ) },
        { IPC2581::SECTION::LOGICAL_NET, _( "logical netlist" ) },
        { IPC2581::SECTION::PHYSICAL_NET, _( "physical netlist" ) },
    };
}


void DIALOG_EXPORT_2581::updateContentSummary()
{
    IPC2581::SECTION_SET sections = resolvedSections();

    wxString summary;

    for( const auto& [section, label] : sectionLabels() )
    {
        if( !sections.Contains( section ) )
            continue;

        if( !summary.IsEmpty() )
            summary << wxT( ", " );

        summary << label;
    }

    m_lblIncludes->SetLabel( wxString::Format( _( "Includes: %s" ), summary ) );
    m_lblIncludes->Wrap( 280 );

    m_btnBomFields->Enable( sections.Contains( IPC2581::SECTION::BOM_AVL ) );

    Layout();
}


void DIALOG_EXPORT_2581::onDataSetChange( wxCommandEvent& event )
{
    // A new function mode removes the section key of the previous function mode
    m_sectionKey.reset();
    updateContentSummary();
}


void DIALOG_EXPORT_2581::onCustomizeClick( wxCommandEvent& event )
{
    IPC2581::MODE mode = GetDataSet();
    IPC2581::SECTION_SET optional = IPC2581::OptionalSections( mode );
    IPC2581::SECTION_SET current = resolvedSections();

    // Some pre-sets have 'optional' items.  If you've chosen a preset, then
    // you can only change the optional ones, not the required or forbidden ones
    std::vector<IPC2581::SECTION> offered;
    wxArrayString                 labels;
    wxArrayInt                    selected;

    for( const auto& [section, label] : sectionLabels() )
    {
        if( !optional.Contains( section ) )
            continue;

        if( current.Contains( section ) )
            selected.Add( static_cast<int>( offered.size() ) );

        offered.push_back( section );
        labels.Add( label );
    }

    if( offered.empty() )
    {
        wxMessageBox( _( "This data set has no optional sections to choose from." ),
                      _( "Customize Content" ), wxOK | wxICON_INFORMATION, this );
        return;
    }

    wxMultiChoiceDialog dlg( this, _( "Choose the optional sections to include." ),
                             _( "Customize Content" ), labels );
    dlg.SetSelections( selected );

    if( dlg.ShowModal() != wxID_OK )
        return;

    IPC2581::SECTION_SET chosen;

    for( int index : dlg.GetSelections() )
        chosen.Set( offered[index] );

    m_sectionKey = IPC2581::SectionKeyString( chosen );
    updateContentSummary();
}


void DIALOG_EXPORT_2581::onBomFieldsClick( wxCommandEvent& event )
{
    DIALOG_EXPORT_2581_BOM dlg( this, m_parent->GetBoard(), m_bomFields );

    if( dlg.ShowModal() == wxID_OK )
        m_bomFields = dlg.GetFields();
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

    if( !m_job )
    {
        m_bomFields.m_internalId = prj.m_IP2581Bom.id;
        m_bomFields.m_mfgPn = prj.m_IP2581Bom.MPN;
        m_bomFields.m_mfg = prj.m_IP2581Bom.mfg;
        m_bomFields.m_distPn = prj.m_IP2581Bom.distPN;
        m_bomFields.m_dist = prj.m_IP2581Bom.dist;
        m_bomFields.m_revision = prj.m_IP2581Bom.bomRev.IsEmpty() ? prj.m_IP2581Bom.schRevision
                                                                  : prj.m_IP2581Bom.bomRev;

        if( std::optional<IPC2581::MODE> mode = IPC2581::ModeFromToken( prj.m_IP2581Bom.mode ) )
            m_choiceDataSet->SetSelection( static_cast<int>( *mode ) );

        if( !prj.m_IP2581Bom.sections.IsEmpty() )
            m_sectionKey = prj.m_IP2581Bom.sections;

        m_choiceNetNames->SetSelection( prj.m_IP2581Bom.netNames == wxT( "anonymize" ) ? 1 : 0 );
        m_choiceRefDes->SetSelection( prj.m_IP2581Bom.refDes == wxT( "omit" ) ? 1 : 0 );
    }
    else
    {
        m_bomFields.m_internalId = m_job->m_colInternalId;
        m_bomFields.m_mfgPn = m_job->m_colMfgPn;
        m_bomFields.m_mfg = m_job->m_colMfg;
        m_bomFields.m_distPn = m_job->m_colDistPn;
        m_bomFields.m_dist = m_job->m_colDist;
        m_bomFields.m_revision = m_job->m_bomRev;

        if( std::optional<IPC2581::MODE> mode = IPC2581::ModeFromToken( m_job->m_mode ) )
            m_choiceDataSet->SetSelection( static_cast<int>( *mode ) );

        if( !m_job->m_sections.IsEmpty() )
            m_sectionKey = m_job->m_sections;

        m_choiceNetNames->SetSelection( m_job->m_netNamePolicy == wxT( "anonymize" ) ? 1 : 0 );
        m_choiceRefDes->SetSelection( m_job->m_refDesPolicy == wxT( "omit" ) ? 1 : 0 );
    }

    updateContentSummary();

    return true;
}


void DIALOG_EXPORT_2581::saveToProject()
{
    PROJECT_FILE& prj = Prj().GetProjectFile();

    prj.m_IP2581Bom.id = m_bomFields.m_internalId;
    prj.m_IP2581Bom.mfg = m_bomFields.m_mfg;
    prj.m_IP2581Bom.MPN = m_bomFields.m_mfgPn;
    prj.m_IP2581Bom.distPN = m_bomFields.m_distPn;
    prj.m_IP2581Bom.dist = m_bomFields.m_dist;
    prj.m_IP2581Bom.bomRev = m_bomFields.m_revision;
    prj.m_IP2581Bom.mode = IPC2581::ModeToken( GetDataSet() );
    prj.m_IP2581Bom.sections = m_sectionKey.value_or( wxString() );
    prj.m_IP2581Bom.netNames = GetNetNamePolicy();
    prj.m_IP2581Bom.refDes = GetRefDesPolicy();
}


bool DIALOG_EXPORT_2581::TransferDataFromWindow()
{
    if( m_job )
    {
        m_job->SetConfiguredOutputPath( m_outputFileName->GetValue() );

        m_job->m_colInternalId = m_bomFields.m_internalId;
        m_job->m_colDist = m_bomFields.m_dist;
        m_job->m_colDistPn = m_bomFields.m_distPn;
        m_job->m_colMfg = m_bomFields.m_mfg;
        m_job->m_colMfgPn = m_bomFields.m_mfgPn;
        m_job->m_bomRev = m_bomFields.m_revision;

        m_job->m_version = GetVersion() == 'B' ? JOB_EXPORT_PCB_IPC2581::IPC2581_VERSION::B
											   : JOB_EXPORT_PCB_IPC2581::IPC2581_VERSION::C;
        m_job->m_units = GetUnitsString() == wxT( "mm" ) ? JOB_EXPORT_PCB_IPC2581::IPC2581_UNITS::MM
														 : JOB_EXPORT_PCB_IPC2581::IPC2581_UNITS::INCH;
        m_job->m_precision = m_precision->GetValue();
        m_job->m_compress = GetCompress();
        m_job->m_mode = IPC2581::ModeToken( GetDataSet() );
        m_job->m_netNamePolicy = GetNetNamePolicy();
        m_job->m_refDesPolicy = GetRefDesPolicy();

        m_job->m_sections = m_sectionKey.value_or( wxString() );
    }

    return true;
}
