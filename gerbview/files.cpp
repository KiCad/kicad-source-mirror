/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include <wx/debug.h>
#include <wx/filedlg.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include <reporter.h>
#include <dialogs/html_message_box.h>
#include <gerbview_frame.h>
#include <gerbview_id.h>
#include <gerber_file_image.h>
#include <gerber_file_image_list.h>
#include <excellon_image.h>
#include <lset.h>
#include <wildcards_and_files_ext.h>
#include <view/view.h>
#include <widgets/wx_progress_reporters.h>
#include "widgets/gerbview_layer_widget.h"
#include <tool/tool_manager.h>

// HTML Messages used more than one time:
#define MSG_NO_MORE_LAYER _( "<b>No more available layers</b> in GerbView to load files" )
#define MSG_NOT_LOADED _( "<b>Not loaded:</b> <i>%s</i>" )
#define MSG_OOM _( "<b>Memory was exhausted reading:</b> <i>%s</i>" )


void GERBVIEW_FRAME::OnGbrFileHistory( wxCommandEvent& event )
{
    wxString filename = GetFileFromHistory( event.GetId(), _( "Gerber files" ) );

    if( !filename.IsEmpty() )
        LoadGerberFiles( filename );
}

void GERBVIEW_FRAME::OnClearGbrFileHistory( wxCommandEvent& aEvent )
{
    ClearFileHistory();
}


void GERBVIEW_FRAME::OnDrlFileHistory( wxCommandEvent& event )
{
    wxString filename = GetFileFromHistory( event.GetId(), _( "Drill files" ), &m_drillFileHistory );

    if( !filename.IsEmpty() )
        LoadExcellonFiles( filename );
}


void GERBVIEW_FRAME::OnClearDrlFileHistory( wxCommandEvent& aEvent )
{
    m_drillFileHistory.ClearFileHistory();

    if( GetMenuBar() )
    {
        ReCreateMenuBar();
        GetMenuBar()->Refresh();
    }
}


void GERBVIEW_FRAME::OnZipFileHistory( wxCommandEvent& event )
{
    wxString filename = GetFileFromHistory( event.GetId(), _( "Zip files" ), &m_zipFileHistory );

    if( !filename.IsEmpty() )
        LoadZipArchiveFile( filename );
}


void GERBVIEW_FRAME::OnClearZipFileHistory( wxCommandEvent& aEvent )
{
    m_zipFileHistory.ClearFileHistory();

    if( GetMenuBar() )
    {
        ReCreateMenuBar();
        GetMenuBar()->Refresh();
    }
}


void GERBVIEW_FRAME::OnJobFileHistory( wxCommandEvent& event )
{
    wxString filename = GetFileFromHistory( event.GetId(), _( "Job files" ), &m_jobFileHistory );

    if( !filename.IsEmpty() )
        LoadGerberJobFile( filename );
}


void GERBVIEW_FRAME::OnClearJobFileHistory( wxCommandEvent& aEvent )
{
    m_jobFileHistory.ClearFileHistory();

    if( GetMenuBar() )
    {
        ReCreateMenuBar();
        GetMenuBar()->Refresh();
    }
}


bool GERBVIEW_FRAME::LoadFileOrShowDialog( const wxString& aFileName,
                                           const wxString& dialogFiletypes,
                                           const wxString& dialogTitle, const int filetype )
{
    static int lastGerberFileWildcard = 0;
    wxArrayString filenamesList;
    wxFileName    filename = aFileName;
    wxString currentPath;

    if( !filename.IsOk() )
    {
        // Use the current working directory if the file name path does not exist.
        if( filename.DirExists() )
            currentPath = filename.GetPath();
        else
        {
            currentPath = m_mruPath;

            // On wxWidgets 3.1 (bug?) the path in wxFileDialog is ignored when
            // finishing by the dir separator. Remove it if any:
            if( currentPath.EndsWith( '\\' ) || currentPath.EndsWith( '/' ) )
                currentPath.RemoveLast();
        }

        wxFileDialog dlg( this, dialogTitle, currentPath, filename.GetFullName(), dialogFiletypes,
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE | wxFD_CHANGE_DIR );

        wxArrayString dummy1, dummy2;
        const int nWildcards = wxParseCommonDialogsFilter( dialogFiletypes, dummy1, dummy2 );

        if( lastGerberFileWildcard >= 0 && lastGerberFileWildcard < nWildcards )
            dlg.SetFilterIndex( lastGerberFileWildcard );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        lastGerberFileWildcard = dlg.GetFilterIndex();
        dlg.GetPaths( filenamesList );
        m_mruPath = currentPath = dlg.GetDirectory();
    }
    else
    {
        filenamesList.Add( aFileName );
        currentPath = filename.GetPath();
        m_mruPath = currentPath;
    }

    // Set the busy cursor
    wxBusyCursor wait;

    bool isFirstFile = GetImagesList()->GetLoadedImageCount() == 0;

    std::vector<int> fileTypesVec( filenamesList.Count(), filetype );
    bool success = LoadListOfGerberAndDrillFiles( currentPath, filenamesList, &fileTypesVec );

    // Auto zoom / sort is only applied when no other files have been loaded
    if( isFirstFile )
    {
        int ly = GetActiveLayer();

        SortLayersByFileExtension();
        Zoom_Automatique( false );

        // Ensure the initial active graphic layer is updated after sorting.
        SetActiveLayer( ly, true );
    }

    return success;
}


bool GERBVIEW_FRAME::LoadAutodetectedFiles( const wxString& aFileName )
{
    // 2 = autodetect files
    return LoadFileOrShowDialog( aFileName, FILEEXT::AllFilesWildcard(), _( "Open Autodetected File(s)" ),
                                 2 );
}


bool GERBVIEW_FRAME::LoadGerberFiles( const wxString& aFileName )
{
    wxString   filetypes;
    wxFileName filename = aFileName;

    /* Standard gerber filetypes
     * (See http://en.wikipedia.org/wiki/Gerber_File)
     * The .gbr (.pho in legacy files) extension is the default used in Pcbnew; however
     * there are a lot of other extensions used for gerber files.  Because the first letter
     * is usually g, we accept g* as extension.
     * (Mainly internal copper layers do not have specific extension, and filenames are like
     * *.g1, *.g2 *.gb1 ...)
     * Now (2014) Ucamco (the company which manages the Gerber format) encourages use of .gbr
     * only and the Gerber X2 file format.
     */
    filetypes = _( "Gerber files" ) + AddFileExtListToFilter( { "g*", "pho" } ) + wxT( "|" );

    /* Special gerber filetypes */
    filetypes += _( "Top layer" ) + AddFileExtListToFilter( { "gtl" } ) + wxT( "|" );
    filetypes += _( "Bottom layer" ) + AddFileExtListToFilter( { "gbl" } ) + wxT( "|" );
    filetypes += _( "Bottom solder resist" ) + AddFileExtListToFilter( { "gbs" } ) + wxT( "|" );
    filetypes += _( "Top solder resist" ) + AddFileExtListToFilter( { "gts" } ) + wxT( "|" );
    filetypes += _( "Bottom overlay" ) + AddFileExtListToFilter( { "gbo" } ) + wxT( "|" );
    filetypes += _( "Top overlay" ) + AddFileExtListToFilter( { "gto" } ) + wxT( "|" );
    filetypes += _( "Bottom paste" ) + AddFileExtListToFilter( { "gbp" } ) + wxT( "|" );
    filetypes += _( "Top paste" ) + AddFileExtListToFilter( { "gtp" } ) + wxT( "|" );
    filetypes += _( "Keep-out layer" ) + AddFileExtListToFilter( { "gko" } ) + wxT( "|" );
    filetypes += _( "Mechanical layers" )
                 + AddFileExtListToFilter(
                         { "gm1", "gm2", "gm3", "gm4", "gm5", "gm6", "gm7", "gm8", "gm9" } )
                 + wxT( "|" );
    filetypes += _( "Top Pad Master" ) + AddFileExtListToFilter( { "gpt" } ) + wxT( "|" );
    filetypes += _( "Bottom Pad Master" ) + AddFileExtListToFilter( { "gpb" } ) + wxT( "|" );

    // All filetypes
    filetypes += FILEEXT::AllFilesWildcard();

    // 0 = gerber files
    return LoadFileOrShowDialog( aFileName, filetypes, _( "Open Gerber File(s)" ), 0 );
}


bool GERBVIEW_FRAME::LoadExcellonFiles( const wxString& aFileName )
{
    wxString filetypes = FILEEXT::DrillFileWildcard();
    filetypes << wxT( "|" );
    filetypes += FILEEXT::AllFilesWildcard();

    // 1 = drill files
    return LoadFileOrShowDialog( aFileName, filetypes, _( "Open NC (Excellon) Drill File(s)" ), 1 );
}


bool GERBVIEW_FRAME::LoadListOfGerberAndDrillFiles( const wxString&      aPath,
                                                    const wxArrayString& aFilenameList,
                                                    std::vector<int>*    aFileType )
{
    wxCHECK_MSG( aFilenameList.Count() == aFileType->size(), false,
                 "Mismatch in file names and file types count" );

    wxFileName filename;

    // Read gerber files: each file is loaded on a new GerbView layer
    bool success = true;
    int layer = GetActiveLayer();
    int  firstLoadedLayer = NO_AVAILABLE_LAYERS;
    LSET visibility = GetVisibleLayers();

    // Manage errors when loading files
    WX_STRING_REPORTER reporter;

    // Create progress dialog (only used if more than 1 file to load
    std::unique_ptr<WX_PROGRESS_REPORTER> progress = nullptr;

    for( unsigned ii = 0; ii < aFilenameList.GetCount(); ii++ )
    {
        filename = aFilenameList[ii];

        if( !filename.IsAbsolute() )
            filename.SetPath( aPath );

        // Check for non existing files, to avoid creating broken or useless data
        // and report all in one error list:
        if( !filename.FileExists() )
        {
            wxString warning;
            warning << wxT( "<b>" ) << _( "File not found:" ) << wxT( "</b><br>" )
                    << filename.GetFullPath() << wxT( "<br>" );
            reporter.Report( warning, RPT_SEVERITY_WARNING );
            success = false;
            continue;
        }

        if( filename.GetExt() == FILEEXT::GerberJobFileExtension.c_str() )
        {
            //We cannot read a gerber job file as a gerber plot file: skip it
            wxString txt;
            txt.Printf( _( "<b>A gerber job file cannot be loaded as a plot file</b> "
                           "<i>%s</i>" ),
                        filename.GetFullName() );
            success = false;
            reporter.Report( txt, RPT_SEVERITY_ERROR );
            continue;
        }


        m_lastFileName = filename.GetFullPath();

        if( !progress && ( aFilenameList.GetCount() > 1 ) )
        {
            progress = std::make_unique<WX_PROGRESS_REPORTER>( this, _( "Load Files" ), 1, PR_CAN_ABORT );
            progress->SetMaxProgress( aFilenameList.GetCount() - 1 );
            progress->Report( wxString::Format( _("Loading %u/%zu %s..." ),
                                                ii+1,
                                                aFilenameList.GetCount(),
                                                m_lastFileName ) );
        }
        else if( progress )
        {
            progress->Report( wxString::Format( _("Loading %u/%zu %s..." ),
                                                ii+1,
                                                aFilenameList.GetCount(),
                                                m_lastFileName ) );
            progress->KeepRefreshing();
        }


        // Make sure we have a layer available to load into
        layer = getNextAvailableLayer();

        if( layer == NO_AVAILABLE_LAYERS )
        {
            success = false;
            reporter.Report( MSG_NO_MORE_LAYER, RPT_SEVERITY_ERROR );

            // Report the name of not loaded files:
            while( ii < aFilenameList.GetCount() )
            {
                filename = aFilenameList[ii++];
                wxString txt = wxString::Format( MSG_NOT_LOADED, filename.GetFullName() );
                reporter.Report( txt, RPT_SEVERITY_ERROR );
            }
            break;
        }

        SetActiveLayer( layer, false );
        visibility[ layer ] = true;

        try
        {
            // 2 = Autodetect
            if( ( *aFileType )[ii] == 2 )
            {
                if( EXCELLON_IMAGE::TestFileIsExcellon( filename.GetFullPath() ) )
                    ( *aFileType )[ii] = 1;
                else if( GERBER_FILE_IMAGE::TestFileIsRS274( filename.GetFullPath() ) )
                    ( *aFileType )[ii] = 0;
            }

            switch( ( *aFileType )[ii] )
            {
            case 0:

                if( Read_GERBER_File( filename.GetFullPath() ) )
                {
                    UpdateFileHistory( filename.GetFullPath() );

                    if( firstLoadedLayer == NO_AVAILABLE_LAYERS )
                    {
                        firstLoadedLayer = layer;
                    }
                }

                break;

            case 1:

                if( Read_EXCELLON_File( filename.GetFullPath() ) )
                {
                    UpdateFileHistory( filename.GetFullPath(), &m_drillFileHistory );

                    // Select the first added layer by default when done loading
                    if( firstLoadedLayer == NO_AVAILABLE_LAYERS )
                    {
                        firstLoadedLayer = layer;
                    }
                }

                break;
            default:
                wxString txt = wxString::Format( MSG_NOT_LOADED, filename.GetFullName() );
                reporter.Report( txt, RPT_SEVERITY_ERROR );
            }
        }
        catch( const std::bad_alloc& )
        {
            wxString txt = wxString::Format( MSG_OOM, filename.GetFullName() );
            reporter.Report( txt, RPT_SEVERITY_ERROR );
            success = false;
            continue;
        }

        if( progress )
            progress->AdvanceProgress();
    }

    if( !success )
    {
        wxSafeYield();  // Allows slice of time to redraw the screen
                        // to refresh widgets, before displaying messages
        HTML_MESSAGE_BOX mbox( this, _( "Errors" ) );
        mbox.ListSet( reporter.GetMessages() );
        mbox.ShowModal();
    }

    SetVisibleLayers( visibility );

    if( firstLoadedLayer != NO_AVAILABLE_LAYERS )
        SetActiveLayer( firstLoadedLayer, true );

    // Synchronize layers tools with actual active layer:
    ReFillLayerWidget();

    m_LayersManager->UpdateLayerIcons();
    syncLayerBox( true );

    GetCanvas()->Refresh();

    return success;
}


bool GERBVIEW_FRAME::unarchiveFiles( const wxString& aFullFileName, REPORTER* aReporter )
{
    bool     foundX2Gerbers = false;
    wxString msg;
    int      firstLoadedLayer = NO_AVAILABLE_LAYERS;
    LSET     visibility = GetVisibleLayers();

    // Extract the path of aFullFileName. We use it to store temporary files
    wxFileName fn( aFullFileName );
    wxString   unzipDir = fn.GetPath();

    wxFFileInputStream zipFile( aFullFileName );

    if( !zipFile.IsOk() )
    {
        if( aReporter )
        {
            msg.Printf( _( "Zip file '%s' cannot be opened." ), aFullFileName );
            aReporter->Report( msg, RPT_SEVERITY_ERROR );
        }

        return false;
    }

    // Update the list of recent zip files.
    UpdateFileHistory( aFullFileName, &m_zipFileHistory );

    // The unzipped file in only a temporary file. Give it a filename
    // which cannot conflict with an usual filename.
    // TODO: make Read_GERBER_File() and Read_EXCELLON_File() able to
    // accept a stream, and avoid using a temp file.
    wxFileName temp_fn( "$tempfile.tmp" );
    temp_fn.MakeAbsolute( unzipDir );
    wxString unzipped_tempfile = temp_fn.GetFullPath();


    bool             success = true;
    wxZipInputStream zipArchive( zipFile );
    wxZipEntry*      entry;
    bool             reported_no_more_layer = false;
    KIGFX::VIEW*     view = GetCanvas()->GetView();

    while( ( entry = zipArchive.GetNextEntry() ) != nullptr )
    {
        if( entry->IsDir() )
            continue;

        wxString   fname = entry->GetName();
        wxFileName uzfn = fname;
        wxString   curr_ext = uzfn.GetExt().Lower();

        // The archive contains Gerber and/or Excellon drill files. Use the right loader.
        // However it can contain a few other files (reports, pdf files...),
        // which will be skipped.
        if( curr_ext == FILEEXT::GerberJobFileExtension.c_str() )
        {
            //We cannot read a gerber job file as a gerber plot file: skip it
            if( aReporter )
            {
                msg.Printf( _( "Skipped file '%s' (gerber job file)." ), entry->GetName() );
                aReporter->Report( msg, RPT_SEVERITY_WARNING );
            }

            continue;
        }

        wxString               matchedExt;
        enum GERBER_ORDER_ENUM order;
        GERBER_FILE_IMAGE_LIST::GetGerberLayerFromFilename( fname, order, matchedExt );

        int layer = getNextAvailableLayer();

        if( layer == NO_AVAILABLE_LAYERS )
        {
            success = false;

            if( aReporter )
            {
                if( !reported_no_more_layer )
                    aReporter->Report( MSG_NO_MORE_LAYER,  RPT_SEVERITY_ERROR );

                reported_no_more_layer = true;

                // Report the name of not loaded files:
                msg.Printf( MSG_NOT_LOADED, entry->GetName() );
                aReporter->Report( msg, RPT_SEVERITY_ERROR );
            }

            delete entry;
            continue;
        }

        SetActiveLayer( layer, false );

        // Create the unzipped temporary file:
        {
            wxFFileOutputStream temporary_ofile( unzipped_tempfile );

            if( temporary_ofile.Ok() )
                temporary_ofile.Write( zipArchive );
            else
            {
                success = false;

                if( aReporter )
                {
                    msg.Printf( _( "<b>Unable to create temporary file '%s'.</b>" ),
                                unzipped_tempfile );
                    aReporter->Report( msg, RPT_SEVERITY_ERROR );
                }
            }
        }

        bool read_ok = true;

        // Try to parse files if we can't tell from file extension
        if( order == GERBER_ORDER_ENUM::GERBER_LAYER_UNKNOWN )
        {
            if( EXCELLON_IMAGE::TestFileIsExcellon( unzipped_tempfile ) )
            {
                order = GERBER_ORDER_ENUM::GERBER_DRILL;
            }
            else if( GERBER_FILE_IMAGE::TestFileIsRS274( unzipped_tempfile ) )
            {
                // If we have no way to know what layer it is, just guess
                order = GERBER_ORDER_ENUM::GERBER_TOP_COPPER;
            }
            else
            {
                if( aReporter )
                {
                    msg.Printf( _( "Skipped file '%s' (unknown type)." ), entry->GetName() );
                    aReporter->Report( msg, RPT_SEVERITY_WARNING );
                }
            }
        }

        if( order == GERBER_ORDER_ENUM::GERBER_DRILL )
        {
            read_ok = Read_EXCELLON_File( unzipped_tempfile );
        }
        else if( order != GERBER_ORDER_ENUM::GERBER_LAYER_UNKNOWN )
        {
            // Read gerber files: each file is loaded on a new GerbView layer
            read_ok = Read_GERBER_File( unzipped_tempfile );

            if( read_ok )
            {
                if( GERBER_FILE_IMAGE* gbrImage = GetGbrImage( layer ) )
                    view->SetLayerHasNegatives( GERBER_DRAW_LAYER( layer ), gbrImage->HasNegativeItems() );
            }
        }

        // Select the first added layer by default when done loading
        if( read_ok && firstLoadedLayer == NO_AVAILABLE_LAYERS )
        {
            firstLoadedLayer = layer;
        }

        delete entry;

        // The unzipped file is only a temporary file, delete it.
        wxRemoveFile( unzipped_tempfile );

        if( !read_ok )
        {
            success = false;

            if( aReporter )
            {
                msg.Printf( _( "<b>unzipped file %s read error</b>" ), unzipped_tempfile );
                aReporter->Report( msg, RPT_SEVERITY_ERROR );
            }
        }
        else
        {
            GERBER_FILE_IMAGE* gerber_image = GetGbrImage( layer );
            visibility[ layer ] = true;

            if( gerber_image )
            {
                gerber_image->m_FileName = fname;
                if( gerber_image->m_IsX2_file )
                    foundX2Gerbers = true;
            }

            layer = getNextAvailableLayer();
            SetActiveLayer( layer, false );
        }
    }

    if( foundX2Gerbers )
        SortLayersByX2Attributes();
    else
        SortLayersByFileExtension();

    SetVisibleLayers( visibility );

    // Select the first layer loaded so we don't show another layer on top after
    if( firstLoadedLayer != NO_AVAILABLE_LAYERS )
        SetActiveLayer( firstLoadedLayer, true );

    return success;
}


bool GERBVIEW_FRAME::LoadZipArchiveFile( const wxString& aFullFileName )
{
#define ZipFileExtension "zip"

    wxFileName filename = aFullFileName;
    wxString currentPath;

    if( !filename.IsOk() )
    {
        // Use the current working directory if the file name path does not exist.
        if( filename.DirExists() )
            currentPath = filename.GetPath();
        else
            currentPath = m_mruPath;

        wxFileDialog dlg( this, _( "Open Zip File" ), currentPath, filename.GetFullName(),
                          FILEEXT::ZipFileWildcard(),
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_CHANGE_DIR );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        filename = dlg.GetPath();
        currentPath = wxGetCwd();
        m_mruPath = currentPath;
    }
    else
    {
        currentPath = filename.GetPath();
        m_mruPath = currentPath;
    }

    WX_STRING_REPORTER reporter;

    if( filename.IsOk() )
        unarchiveFiles( filename.GetFullPath(), &reporter );

    Zoom_Automatique( false );

    // Synchronize layers tools with actual active layer:
    ReFillLayerWidget();
    SetActiveLayer( GetActiveLayer() );
    m_LayersManager->UpdateLayerIcons();
    syncLayerBox();

    if( reporter.HasMessage() )
    {
        wxSafeYield();  // Allows slice of time to redraw the screen
                        // to refresh widgets, before displaying messages
        HTML_MESSAGE_BOX mbox( this, _( "Messages" ) );
        mbox.ListSet( reporter.GetMessages() );
        mbox.ShowModal();
    }

    return true;
}


void GERBVIEW_FRAME::ClearFileHistory()
{
    m_drillFileHistory.ClearFileHistory();
    m_zipFileHistory.ClearFileHistory();
    m_jobFileHistory.ClearFileHistory();

    EDA_DRAW_FRAME::ClearFileHistory();
}


void GERBVIEW_FRAME::DoWithAcceptedFiles()
{
    wxString gerbFn; // param to be sent with action event.

    for( const wxFileName& file : m_AcceptedFiles )
    {
        if( file.GetExt() == FILEEXT::ArchiveFileExtension )
        {
            wxString fn = file.GetFullPath();
            // Open zip archive in editor
            m_toolManager->RunAction<wxString*>( *m_acceptedExts.at( FILEEXT::ArchiveFileExtension ), &fn );
        }
        else
        {
            // Store FileName in variable to open later
            gerbFn += '"' + file.GetFullPath() + '"';
        }
    }

    // Open files in editor
    if( !gerbFn.IsEmpty() )
        m_toolManager->RunAction<wxString*>( *m_acceptedExts.at( FILEEXT::GerberFileExtension ), &gerbFn );
}
