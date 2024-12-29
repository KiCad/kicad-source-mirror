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
#include <confirm.h>
#include <footprint.h>
#include <kidialog.h>
#include <kiway_holder.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <pgm_base.h>
#include <progress_reporter.h>
#include <project.h>
#include <project/board_project_settings.h>
#include <project/project_file.h>
#include <settings/settings_manager.h>
#include <widgets/std_bitmap_button.h>
#include <jobs/job_export_pcb_odb.h>

#include <set>
#include <vector>
#include <core/thread_pool.h>
#include <io/io_mgr.h>
#include <jobs/job_export_pcb_odb.h>
#include <pcb_io/pcb_io_mgr.h>
#include <wx/dir.h>
#include <wx/filedlg.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include <wx/tarstrm.h>
#include <wx/zstream.h>

static wxString s_oemColumn = wxEmptyString;

DIALOG_EXPORT_ODBPP::DIALOG_EXPORT_ODBPP( PCB_EDIT_FRAME* aParent ) :
        DIALOG_EXPORT_ODBPP_BASE( aParent ), m_parent( aParent ), m_job( nullptr )
{
    m_browseButton->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );

    SetupStandardButtons( { { wxID_OK, _( "Export" ) }, { wxID_CANCEL, _( "Close" ) } } );

    wxString path = m_parent->GetLastPath( LAST_PATH_ODBPP );

    if( path.IsEmpty() )
    {
        wxFileName brdFile( m_parent->GetBoard()->GetFileName() );
        wxFileName odbFile( brdFile.GetPath(),
                            wxString::Format( wxS( "%s-odb" ), brdFile.GetName() ),
                            FILEEXT::ArchiveFileExtension );
        path = odbFile.GetFullPath();
    }

    m_outputFileName->SetValue( path );

    // Fill wxChoice (and others) items with data before calling finishDialogSettings()
    // to calculate suitable widgets sizes
    Init();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


DIALOG_EXPORT_ODBPP::DIALOG_EXPORT_ODBPP( JOB_EXPORT_PCB_ODB* aJob, PCB_EDIT_FRAME* aEditFrame,
                                          wxWindow* aParent ) :
        DIALOG_EXPORT_ODBPP_BASE( aParent ), m_parent( aEditFrame ), m_job( aJob )
{
    m_browseButton->Hide();

    SetupStandardButtons( { { wxID_OK, _( "Save" ) }, { wxID_CANCEL, _( "Close" ) } } );

    m_outputFileName->SetValue( m_job->GetOutputPath() );

    // Fill wxChoice (and others) items with data before calling finishDialogSettings()
    // to calculate suitable widgets sizes
    Init();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
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

    wxFileDialog dlg( this, _( "Export ODB++ File" ), fn.GetPath(), fileDialogName, filter,
                      wxFD_SAVE );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    path = dlg.GetPath();

    fn = wxFileName( path );

    if( fn.GetExt().Lower() == "zip" )
    {
        m_choiceCompress->SetSelection(
                static_cast<int>( JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::ZIP ) );
    }
    else if( fn.GetExt().Lower() == "tgz" )
    {
        m_choiceCompress->SetSelection(
                static_cast<int>( JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::TGZ ) );
    }
    else if( path.EndsWith( "/" ) || path.EndsWith( "\\" ) )
    {
        m_choiceCompress->SetSelection(
                static_cast<int>( JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::NONE ) );
    }
    else
    {
        wxString msg;
        msg.Printf( _( "The selected output file name is not a supported archive format." ) );
        DisplayErrorMessage( this, msg );
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

    auto compressionMode =
            static_cast<JOB_EXPORT_PCB_ODB::ODB_COMPRESSION>( m_choiceCompress->GetSelection() );

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
    case JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::TGZ: fn += ".tgz"; break;
    case JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::NONE: fn = wxFileName( fn, "" ).GetFullPath(); break;
    default: break;
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
            wxString msg;
            msg.Printf( _( "Output file name cannot be empty." ) );
            DisplayErrorMessage( this, msg );
            return;
        }

        auto compressionMode = static_cast<JOB_EXPORT_PCB_ODB::ODB_COMPRESSION>(
                m_choiceCompress->GetSelection() );

        wxFileName fileName( fn );
        bool       isDirectory = fileName.IsDir();
        wxString   extension = fileName.GetExt();

        if( ( compressionMode == JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::NONE && !isDirectory )
            || ( compressionMode == JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::ZIP && extension != "zip" )
            || ( compressionMode == JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::TGZ
                 && extension != "tgz" ) )
        {
            wxString msg;
            msg.Printf(
                    _( "The output file name conflicts with the selected compression format." ) );
            DisplayErrorMessage( this, msg );
            return;
        }

        m_parent->SetLastPath( LAST_PATH_ODBPP, m_outputFileName->GetValue() );
    }

    event.Skip();
}


bool DIALOG_EXPORT_ODBPP::Init()
{
    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
    PCBNEW_SETTINGS*  cfg = mgr.GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" );

    if( !m_job )
    {
        m_choiceUnits->SetSelection( cfg->m_ExportODBPP.units );
        m_precision->SetValue( cfg->m_ExportODBPP.precision );
        m_choiceCompress->SetSelection( cfg->m_ExportODBPP.compressFormat );
    }
    else
    {
        m_choiceUnits->SetSelection( static_cast<int>( m_job->m_units ) );
        m_precision->SetValue( m_job->m_precision );
        m_choiceCompress->SetSelection( static_cast<int>( m_job->m_compressionMode ) );
    }

    return true;
}


bool DIALOG_EXPORT_ODBPP::TransferDataFromWindow()
{
    if( !m_job )
    {
        SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
        PCBNEW_SETTINGS*  cfg = mgr.GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" );

        cfg->m_ExportODBPP.units = m_choiceUnits->GetSelection();
        cfg->m_ExportODBPP.precision = m_precision->GetValue();
        cfg->m_ExportODBPP.compressFormat = m_choiceCompress->GetSelection();
    }
    else
    {
        m_job->SetOutputPath( m_outputFileName->GetValue() );

        m_job->m_precision = m_precision->GetValue();
        m_job->m_units =
                static_cast<JOB_EXPORT_PCB_ODB::ODB_UNITS>( m_choiceUnits->GetSelection() );
        m_job->m_compressionMode = static_cast<JOB_EXPORT_PCB_ODB::ODB_COMPRESSION>(
                m_choiceCompress->GetSelection() );
    }

    return true;
}


void DIALOG_EXPORT_ODBPP::GenerateODBPPFiles( const JOB_EXPORT_PCB_ODB& aJob, BOARD* aBoard,
                                              PCB_EDIT_FRAME*    aParentFrame,
                                              PROGRESS_REPORTER* aProgressReporter,
                                              REPORTER*          aReporter )
{
    wxCHECK( aBoard, /* void */ );
    wxString outputPath = aJob.GetOutputPath();

    if( outputPath.IsEmpty() )
        outputPath = wxFileName( aJob.m_filename ).GetPath();

    wxFileName pcbFileName( outputPath );

    // Write through symlinks, don't replace them
    WX_FILENAME::ResolvePossibleSymlinks( pcbFileName );

    if( pcbFileName.GetPath().IsEmpty() && pcbFileName.HasName() )
        pcbFileName.MakeAbsolute();

    wxString msg;

    if( !wxFileName::DirExists( pcbFileName.GetPath() ) )
    {
        // Make every directory provided when the provided path doesn't exist
        if( !wxFileName::Mkdir( pcbFileName.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
        {
            msg.Printf( _( "Cannot create output directory '%s'." ), pcbFileName.GetFullPath() );

            if( aReporter )
                aReporter->Report( msg, RPT_SEVERITY_ERROR );

            return;
        }
    }

    if( pcbFileName.IsDir() && !pcbFileName.IsDirWritable() )
    {
        msg.Printf( _( "Insufficient permissions to folder '%s'." ), pcbFileName.GetPath() );
    }
    else if( !pcbFileName.FileExists() && !pcbFileName.IsDirWritable() )
    {
        msg.Printf( _( "Insufficient permissions to save file '%s'." ), pcbFileName.GetFullPath() );
    }
    else if( pcbFileName.FileExists() && !pcbFileName.IsFileWritable() )
    {
        msg.Printf( _( "Insufficient permissions to save file '%s'." ), pcbFileName.GetFullPath() );
    }

    if( !msg.IsEmpty() )
    {
        if( aReporter )
            aReporter->Report( msg, RPT_SEVERITY_ERROR );

        return;
    }

    wxFileName tempFile( pcbFileName.GetFullPath() );

    if( aJob.m_compressionMode != JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::NONE )
    {
        if( pcbFileName.Exists() )
        {
            if( aParentFrame )
            {
                msg = wxString::Format( _( "Output files '%s' already exists. "
                                           "Do you want to overwrite it?" ),
                                        pcbFileName.GetFullPath() );

                KIDIALOG errorDlg( aParentFrame, msg, _( "Confirmation" ),
                                   wxOK | wxCANCEL | wxICON_WARNING );
                errorDlg.SetOKLabel( _( "Overwrite" ) );

                if( errorDlg.ShowModal() != wxID_OK )
                    return;

                if( !wxRemoveFile( pcbFileName.GetFullPath() ) )
                {
                    msg.Printf( _( "Cannot remove existing output file '%s'." ),
                                pcbFileName.GetFullPath() );
                    DisplayErrorMessage( aParentFrame, msg );
                    return;
                }
            }
            else
            {
                msg = wxString::Format( _( "Output file '%s' already exists." ),
                                        pcbFileName.GetFullPath() );

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

                KIDIALOG errorDlg( aParentFrame, msg, _( "Confirmation" ),
                                   wxOK | wxCANCEL | wxICON_WARNING );
                errorDlg.SetOKLabel( _( "Overwrite" ) );

                if( errorDlg.ShowModal() != wxID_OK )
                    return;

                if( !tempFile.Rmdir( wxPATH_RMDIR_RECURSIVE ) )
                {
                    msg.Printf( _( "Cannot remove existing output directory '%s'." ),
                                tempFile.GetFullPath() );
                    DisplayErrorMessage( aParentFrame, msg );
                    return;
                }
            }
            else
            {
                msg = wxString::Format( _( "Output directory '%s' already exists." ),
                                        tempFile.GetFullPath() );

                if( aReporter )
                    aReporter->Report( msg, RPT_SEVERITY_ERROR );

                return;
            }
        }
    }

    wxString                    upperTxt;
    wxString                    lowerTxt;
    std::map<std::string, UTF8> props;

    props["units"] = aJob.m_units == JOB_EXPORT_PCB_ODB::ODB_UNITS::MILLIMETERS ? "mm" : "inch";
    props["sigfig"] = wxString::Format( "%d", aJob.m_precision );

    auto saveFile = [&]() -> bool
    {
        try
        {
            IO_RELEASER<PCB_IO> pi( PCB_IO_MGR::PluginFind( PCB_IO_MGR::ODBPP ) );
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
    auto         ret = tp.submit( saveFile );

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

        wxFFileOutputStream fnout( pcbFileName.GetFullPath() );
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
                wxString   relativePath =
                        parentPath.IsEmpty()
                                  ? fileName
                                  : parentPath + wxString( wxFileName::GetPathSeparator() )
                                          + fileName;

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
        wxFFileOutputStream fnout( pcbFileName.GetFullPath() );
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
                wxString   relativePath =
                        parentPath.IsEmpty()
                                  ? fileName
                                  : parentPath + wxString( wxFileName::GetPathSeparator() )
                                          + fileName;

                if( wxFileName::DirExists( fileInTar.GetFullPath() ) )
                {
                    tarStream.PutNextDirEntry( relativePath );
                    addDirToTar( fileInTar.GetFullPath(), relativePath );
                }
                else
                {
                    wxFFileInputStream fileStream( fileInTar.GetFullPath() );
                    tarStream.PutNextEntry( relativePath, wxDateTime::Now(),
                                            fileStream.GetLength() );
                    fileStream.Read( tarStream );
                }
                cont = dir.GetNext( &fileName );
            }
        };

        addDirToTar(
                tempFile.GetFullPath(),
                tempFile.GetPath( wxPATH_NO_SEPARATOR ).AfterLast( tempFile.GetPathSeparator() ) );

        tarStream.Close();
        zlibStream.Close();
        fnout.Close();

        tempFile.Rmdir( wxPATH_RMDIR_RECURSIVE );
    }

    if( aProgressReporter )
        aProgressReporter->SetCurrentProgress( 1 );
}
