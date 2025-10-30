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

#include "dialogs/dialog_export_odbpp.h"

#include <set>
#include <vector>
#include <thread_pool.h>

#include <wx/dir.h>
#include <wx/filedlg.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include <wx/tarstrm.h>
#include <wx/zstream.h>

#include <board.h>
#include <confirm.h>
#include <footprint.h>
#include <kidialog.h>
#include <kiway_holder.h>
#include <paths.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <pgm_base.h>
#include <progress_reporter.h>
#include <project.h>
#include <project/project_file.h>
#include <settings/settings_manager.h>
#include <string_utils.h>
#include <widgets/std_bitmap_button.h>
#include <io/io_mgr.h>
#include <jobs/job_export_pcb_odb.h>
#include <pcb_io/pcb_io_mgr.h>



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


void DIALOG_EXPORT_ODBPP::GenerateODBPPFiles( const JOB_EXPORT_PCB_ODB& aJob, BOARD* aBoard,
                                              PCB_EDIT_FRAME* aParentFrame, PROGRESS_REPORTER* aProgressReporter,
                                              REPORTER* aReporter )
{
    wxCHECK( aBoard, /* void */ );
    wxString outputPath = aJob.GetFullOutputPath( aBoard->GetProject() );

    if( outputPath.IsEmpty() )
        outputPath = wxFileName( aJob.m_filename ).GetPath();

    wxFileName outputFn( outputPath );

    // Write through symlinks, don't replace them
    WX_FILENAME::ResolvePossibleSymlinks( outputFn );

    if( outputFn.GetPath().IsEmpty() && outputFn.HasName() )
        outputFn.MakeAbsolute();

    bool     outputIsSingleFile = aJob.m_compressionMode != JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::NONE;
    wxString msg;

    if( !PATHS::EnsurePathExists( outputFn.GetFullPath(), outputIsSingleFile ) )
    {
        msg.Printf( _( "Cannot create output directory '%s'." ), outputFn.GetFullPath() );

        if( aReporter )
            aReporter->Report( msg, RPT_SEVERITY_ERROR );

        return;
    }

    if( outputFn.IsDir() && !outputFn.IsDirWritable() )
    {
        msg.Printf( _( "Insufficient permissions to folder '%s'." ), outputFn.GetPath() );

        if( aReporter )
            aReporter->Report( msg, RPT_SEVERITY_ERROR );

        return;
    }

    if( outputIsSingleFile )
    {
        bool writeable = outputFn.FileExists() ? outputFn.IsFileWritable() : outputFn.IsDirWritable();

        if( !writeable )
        {
            msg.Printf( _( "Insufficient permissions to save file '%s'." ), outputFn.GetFullPath() );

            if( aReporter )
                aReporter->Report( msg, RPT_SEVERITY_ERROR );

            return;
        }
    }

    wxFileName tempFile( outputFn.GetFullPath() );

    if( outputIsSingleFile )
    {
        if( outputFn.Exists() )
        {
            if( aParentFrame )
            {
                msg = wxString::Format( _( "Output files '%s' already exists. Do you want to overwrite it?" ),
                                        outputFn.GetFullPath() );

                KIDIALOG errorDlg( aParentFrame, msg, _( "Confirmation" ), wxOK | wxCANCEL | wxICON_WARNING );
                errorDlg.SetOKLabel( _( "Overwrite" ) );

                if( errorDlg.ShowModal() != wxID_OK )
                    return;

                if( !wxRemoveFile( outputFn.GetFullPath() ) )
                {
                    msg.Printf( _( "Cannot remove existing output file '%s'." ), outputFn.GetFullPath() );
                    DisplayErrorMessage( aParentFrame, msg );
                    return;
                }
            }
            else
            {
                msg = wxString::Format( _( "Output file '%s' already exists." ), outputFn.GetFullPath() );

                if( aReporter )
                    aReporter->Report( msg, RPT_SEVERITY_ERROR );

                return;
            }
        }

        tempFile.AssignDir( wxFileName::GetTempDir() );
        tempFile.AppendDir( "kicad" );
        tempFile.AppendDir( "odb" );

        if( !wxFileName::Mkdir( tempFile.GetFullPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
        {
            msg.Printf( _( "Cannot create temporary output directory." ) );

            if( aReporter )
                aReporter->Report( msg, RPT_SEVERITY_ERROR );

            return;
        }
    }
    else
    {
        // Test for the output directory of tempFile
        wxDir testDir( tempFile.GetFullPath() );

        if( testDir.IsOpened() && ( testDir.HasFiles() || testDir.HasSubDirs() ) )
        {
            if( aParentFrame )
            {
                msg = wxString::Format( _( "Output directory '%s' already exists and is not empty. "
                                           "Do you want to overwrite it?" ),
                                        tempFile.GetFullPath() );

                KIDIALOG errorDlg( aParentFrame, msg, _( "Confirmation" ), wxOK | wxCANCEL | wxICON_WARNING );
                errorDlg.SetOKLabel( _( "Overwrite" ) );

                if( errorDlg.ShowModal() != wxID_OK )
                    return;

                if( !tempFile.Rmdir( wxPATH_RMDIR_RECURSIVE ) )
                {
                    msg.Printf( _( "Cannot remove existing output directory '%s'." ), tempFile.GetFullPath() );
                    DisplayErrorMessage( aParentFrame, msg );
                    return;
                }
            }
            else
            {
                msg = wxString::Format( _( "Output directory '%s' already exists." ), tempFile.GetFullPath() );

                if( aReporter )
                    aReporter->Report( msg, RPT_SEVERITY_ERROR );

                return;
            }
        }
    }

    std::map<std::string, UTF8> props;

    props["units"] = aJob.m_units == JOB_EXPORT_PCB_ODB::ODB_UNITS::MM ? "mm" : "inch";
    props["sigfig"] = wxString::Format( "%d", aJob.m_precision );

    auto saveFile =
            [&]() -> bool
            {
                try
                {
                    IO_RELEASER<PCB_IO> pi( PCB_IO_MGR::FindPlugin( PCB_IO_MGR::ODBPP ) );
                    pi->SetReporter( aReporter );
                    pi->SetProgressReporter( aProgressReporter );
                    pi->SaveBoard( tempFile.GetFullPath(), aBoard, &props );
                    return true;
                }
                catch( const IO_ERROR& ioe )
                {
                    if( aReporter )
                    {
                        msg = wxString::Format( _( "Error generating ODBPP files '%s'.\n%s" ),
                                                tempFile.GetFullPath(), ioe.What() );
                        aReporter->Report( msg, RPT_SEVERITY_ERROR );
                    }

                    // In case we started a file but didn't fully write it, clean up
                    wxFileName::Rmdir( tempFile.GetFullPath() );
                    return false;
                }
            };

    thread_pool& tp = GetKiCadThreadPool();
    auto         ret = tp.submit_task( saveFile );

    std::future_status status = ret.wait_for( std::chrono::milliseconds( 250 ) );

    while( status != std::future_status::ready )
    {
        if( aProgressReporter )
            aProgressReporter->KeepRefreshing();

        status = ret.wait_for( std::chrono::milliseconds( 250 ) );
    }

    try
    {
        if( !ret.get() )
            return;
    }
    catch( const std::exception& e )
    {
        if( aReporter )
        {
            aReporter->Report( wxString::Format( "Exception in ODB++ generation: %s", e.what() ),
                               RPT_SEVERITY_ERROR );
        }

        return;
    }

    if( aJob.m_compressionMode == JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::ZIP )
    {
        if( aProgressReporter )
            aProgressReporter->AdvancePhase( _( "Compressing output" ) );

        wxFFileOutputStream fnout( outputFn.GetFullPath() );
        wxZipOutputStream   zipStream( fnout );

        std::function<void( const wxString&, const wxString& )> addDirToZip =
                [&]( const wxString& dirPath, const wxString& parentPath )
                {
                    wxDir    dir( dirPath );
                    wxString fileName;

                    bool cont = dir.GetFirst( &fileName, wxEmptyString, wxDIR_DEFAULT );

                    while( cont )
                    {
                        wxFileName fileInZip( dirPath, fileName );
                        wxString   relativePath = fileName;

                        if( !parentPath.IsEmpty() )
                            relativePath = parentPath + wxString( wxFileName::GetPathSeparator() ) + fileName;

                        if( wxFileName::DirExists( fileInZip.GetFullPath() ) )
                        {
                            zipStream.PutNextDirEntry( relativePath );
                            addDirToZip( fileInZip.GetFullPath(), relativePath );
                        }
                        else
                        {
                            wxFFileInputStream fileStream( fileInZip.GetFullPath() );
                            zipStream.PutNextEntry( relativePath );
                            fileStream.Read( zipStream );
                        }
                        cont = dir.GetNext( &fileName );
                    }
                };

        addDirToZip( tempFile.GetFullPath(), wxEmptyString );

        zipStream.Close();
        fnout.Close();

        tempFile.Rmdir( wxPATH_RMDIR_RECURSIVE );
    }
    else if( aJob.m_compressionMode == JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::TGZ )
    {
        wxFFileOutputStream fnout( outputFn.GetFullPath() );
        wxZlibOutputStream  zlibStream( fnout, -1, wxZLIB_GZIP );
        wxTarOutputStream   tarStream( zlibStream );

        std::function<void( const wxString&, const wxString& )> addDirToTar =
                [&]( const wxString& dirPath, const wxString& parentPath )
                {
                    wxDir    dir( dirPath );
                    wxString fileName;

                    bool cont = dir.GetFirst( &fileName, wxEmptyString, wxDIR_DEFAULT );
                    while( cont )
                    {
                        wxFileName fileInTar( dirPath, fileName );
                        wxString   relativePath = fileName;

                        if( !parentPath.IsEmpty() )
                            relativePath = parentPath + wxString( wxFileName::GetPathSeparator() ) + fileName;

                        if( wxFileName::DirExists( fileInTar.GetFullPath() ) )
                        {
                            tarStream.PutNextDirEntry( relativePath );
                            addDirToTar( fileInTar.GetFullPath(), relativePath );
                        }
                        else
                        {
                            wxFFileInputStream fileStream( fileInTar.GetFullPath() );
                            tarStream.PutNextEntry( relativePath, wxDateTime::Now(), fileStream.GetLength() );
                            fileStream.Read( tarStream );
                        }
                        cont = dir.GetNext( &fileName );
                    }
                };

        addDirToTar( tempFile.GetFullPath(),
                     tempFile.GetPath( wxPATH_NO_SEPARATOR ).AfterLast( tempFile.GetPathSeparator() ) );

        tarStream.Close();
        zlibStream.Close();
        fnout.Close();

        tempFile.Rmdir( wxPATH_RMDIR_RECURSIVE );
    }

    if( aProgressReporter )
        aProgressReporter->SetCurrentProgress( 1 );
}
