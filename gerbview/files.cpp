/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <wx/fs_zip.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include <common.h>
#include <reporter.h>
#include <html_messagebox.h>
#include <gerbview_frame.h>
#include <gerbview_id.h>
#include <gerber_file_image.h>
#include <gerber_file_image_list.h>
#include <excellon_image.h>
#include <gerbview_layer_widget.h>
#include <wildcards_and_files_ext.h>
#include <widgets/progress_reporter.h>

// HTML Messages used more than one time:
#define MSG_NO_MORE_LAYER\
    _( "<b>No more available free graphic layer</b> in Gerbview to load files" )
#define MSG_NOT_LOADED _( "\n<b>Not loaded:</b> <i>%s</i>" )


void GERBVIEW_FRAME::OnGbrFileHistory( wxCommandEvent& event )
{
    wxString fn;

    fn = GetFileFromHistory( event.GetId(), _( "Gerber files" ) );

    if( !fn.IsEmpty() )
    {
        Erase_Current_DrawLayer( false );
        LoadGerberFiles( fn );
    }
}


void GERBVIEW_FRAME::OnDrlFileHistory( wxCommandEvent& event )
{
    wxString fn;

    fn = GetFileFromHistory( event.GetId(), _( "Drill files" ), &m_drillFileHistory );

    if( !fn.IsEmpty() )
    {
        Erase_Current_DrawLayer( false );
        LoadExcellonFiles( fn );
    }
}


void GERBVIEW_FRAME::OnZipFileHistory( wxCommandEvent& event )
{
    wxString filename;
    filename = GetFileFromHistory( event.GetId(), _( "Zip files" ), &m_zipFileHistory );

    if( !filename.IsEmpty() )
    {
        Erase_Current_DrawLayer( false );
        LoadZipArchiveFile( filename );
    }
}


void GERBVIEW_FRAME::OnJobFileHistory( wxCommandEvent& event )
{
    wxString filename = GetFileFromHistory( event.GetId(), _( "Job files" ), &m_jobFileHistory );

    if( !filename.IsEmpty() )
        LoadGerberJobFile( filename );
}


/* File commands. */
void GERBVIEW_FRAME::Files_io( wxCommandEvent& event )
{
    int        id = event.GetId();

    switch( id )
    {
    case wxID_FILE:
        LoadGerberFiles( wxEmptyString );
        break;

    case ID_GERBVIEW_ERASE_ALL:
        Clear_DrawLayers( false );
        Zoom_Automatique( false );
        GetCanvas()->Refresh();
        ClearMsgPanel();
        break;

    case ID_GERBVIEW_RELOAD_ALL:
    {
        // Store filenames
        wxArrayString listOfGerberFiles;
        std::vector<int> fileType;

        for( unsigned i = 0; i < GetImagesList()->ImagesMaxCount(); i++ )
        {
            if( GetImagesList()->GetGbrImage( i ) == nullptr )
                continue;

            if( !GetImagesList()->GetGbrImage( i )->m_InUse )
                continue;

            auto* drill_file = dynamic_cast<EXCELLON_IMAGE*>( GetImagesList()->GetGbrImage( i ) );

            if( drill_file )
                fileType.push_back( 1 );
            else
                fileType.push_back( 0 );

            listOfGerberFiles.Add( GetImagesList()->GetGbrImage( i )->m_FileName );
        }

        // Clear all layers
        Clear_DrawLayers( false );
        Zoom_Automatique( false );
        GetCanvas()->Refresh();
        ClearMsgPanel();

        // Load the layers from stored paths
        wxBusyCursor wait;
        loadListOfGerberAndDrillFiles( wxEmptyString, listOfGerberFiles, &fileType );
    }
    break;

    case ID_GERBVIEW_LOAD_DRILL_FILE:
        LoadExcellonFiles( wxEmptyString );
        GetCanvas()->Refresh();
        break;

    case ID_GERBVIEW_LOAD_ZIP_ARCHIVE_FILE:
        LoadZipArchiveFile( wxEmptyString );
        GetCanvas()->Refresh();
        break;

    case ID_GERBVIEW_LOAD_JOB_FILE:
        LoadGerberJobFile( wxEmptyString );
        GetCanvas()->Refresh();
        break;

    default:
        wxFAIL_MSG( "File_io: unexpected command id" );
        break;
    }
}


bool GERBVIEW_FRAME::LoadGerberFiles( const wxString& aFullFileName )
{
    static int lastGerberFileWildcard = 0;
    wxString   filetypes;
    wxArrayString filenamesList;
    wxFileName filename = aFullFileName;
    wxString currentPath;

    if( !filename.IsOk() )
    {
        /* Standard gerber filetypes
         * (See http://en.wikipedia.org/wiki/Gerber_File)
         * the .gbr (.pho in legacy files) extension is the default used in Pcbnew
         * However there are a lot of other extensions used for gerber files
         * Because the first letter is usually g, we accept g* as extension
         * (Mainly internal copper layers do not have specific extension,
         *  and filenames are like *.g1, *.g2 *.gb1 ...).
         * Now (2014) Ucamco (the company which manages the Gerber format) encourages
         * use of .gbr only and the Gerber X2 file format.
         */
        filetypes = _( "Gerber files (.g* .lgr .pho)" );
        filetypes << wxT("|");
        filetypes += wxT("*.g*;*.G*;*.pho;*.PHO" );
        filetypes << wxT("|");

        /* Special gerber filetypes */
        filetypes += _( "Top layer (*.GTL)|*.GTL;*.gtl|" );
        filetypes += _( "Bottom layer (*.GBL)|*.GBL;*.gbl|" );
        filetypes += _( "Bottom solder resist (*.GBS)|*.GBS;*.gbs|" );
        filetypes += _( "Top solder resist (*.GTS)|*.GTS;*.gts|" );
        filetypes += _( "Bottom overlay (*.GBO)|*.GBO;*.gbo|" );
        filetypes += _( "Top overlay (*.GTO)|*.GTO;*.gto|" );
        filetypes += _( "Bottom paste (*.GBP)|*.GBP;*.gbp|" );
        filetypes += _( "Top paste (*.GTP)|*.GTP;*.gtp|" );
        filetypes += _( "Keep-out layer (*.GKO)|*.GKO;*.gko|" );
        filetypes += _( "Mechanical layers (*.GMx)|*.GM1;*.gm1;*.GM2;*.gm2;*.GM3;*.gm3|" );
        filetypes += _( "Top Pad Master (*.GPT)|*.GPT;*.gpt|" );
        filetypes += _( "Bottom Pad Master (*.GPB)|*.GPB;*.gpb|" );

        // All filetypes
        filetypes += AllFilesWildcard();

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

        wxFileDialog dlg( this, _( "Open Gerber File(s)" ),
                          currentPath,
                          filename.GetFullName(),
                          filetypes,
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE | wxFD_CHANGE_DIR );
        dlg.SetFilterIndex( lastGerberFileWildcard );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        lastGerberFileWildcard = dlg.GetFilterIndex();
        dlg.GetPaths( filenamesList );
        m_mruPath = currentPath = dlg.GetDirectory();
    }
    else
    {
        filenamesList.Add( aFullFileName );
        m_mruPath = currentPath = filename.GetPath();
    }

    Erase_Current_DrawLayer( false );

    // Set the busy cursor
    wxBusyCursor wait;

    return loadListOfGerberAndDrillFiles( currentPath, filenamesList );
}


bool GERBVIEW_FRAME::loadListOfGerberAndDrillFiles( const wxString& aPath,
                                            const wxArrayString& aFilenameList,
                                            const std::vector<int>* aFileType )
{
    wxFileName filename;

    // Read gerber files: each file is loaded on a new GerbView layer
    bool success = true;
    int layer = GetActiveLayer();
    LSET visibility = GetVisibleLayers();

    // Manage errors when loading files
    wxString msg;
    WX_STRING_REPORTER reporter( &msg );

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
            warning << "<b>" << _( "File not found:" ) << "</b><br>"
                    << filename.GetFullPath() << "<br>";
            reporter.Report( warning, REPORTER::RPT_WARNING );
            success = false;
            continue;
        }

        m_lastFileName = filename.GetFullPath();

        if( !progress && ( aFilenameList.GetCount() > 1 ) )
        {
            progress = std::make_unique<WX_PROGRESS_REPORTER>( this,
                            _( "Loading Gerber files..." ), 1, false );
            progress->SetMaxProgress( aFilenameList.GetCount() - 1 );
            progress->Report( wxString::Format( _("Loading %u/%zu %s" ), ii+1,
                                            aFilenameList.GetCount(), m_lastFileName ) );
        }
        else if( progress )
        {
            progress->Report( wxString::Format( _("Loading %u/%zu %s" ), ii+1,
                                            aFilenameList.GetCount(), m_lastFileName ) );
            progress->KeepRefreshing();
        }

        SetActiveLayer( layer, false );

        visibility[ layer ] = true;

        if( aFileType && (*aFileType)[ii] == 1 )
        {
            LoadExcellonFiles( filename.GetFullPath() );
            layer = GetActiveLayer();   // Loading NC drill file changes the active layer
        }
        else
        {
            if( Read_GERBER_File( filename.GetFullPath() ) )
            {
                UpdateFileHistory( m_lastFileName );

                layer = getNextAvailableLayer( layer );

                if( layer == NO_AVAILABLE_LAYERS && ii < aFilenameList.GetCount()-1 )
                {
                    success = false;
                    reporter.Report( MSG_NO_MORE_LAYER, REPORTER::RPT_ERROR );

                    // Report the name of not loaded files:
                    ii += 1;
                    while( ii < aFilenameList.GetCount() )
                    {
                        filename = aFilenameList[ii++];
                        wxString txt = wxString::Format( MSG_NOT_LOADED, filename.GetFullName() );
                        reporter.Report( txt, REPORTER::RPT_ERROR );
                    }
                    break;
                }

                SetActiveLayer( layer, false );
            }
        }

        if( progress )
            progress->AdvanceProgress();
    }

    if( !success )
    {
        wxSafeYield();  // Allows slice of time to redraw the screen
                        // to refresh widgets, before displaying messages
        HTML_MESSAGE_BOX mbox( this, _( "Errors" ) );
        mbox.ListSet( msg );
        mbox.ShowModal();
    }

    SetVisibleLayers( visibility );

    Zoom_Automatique( false );

    // Synchronize layers tools with actual active layer:
    ReFillLayerWidget();

    // TODO: it would be nice if we could set the active layer to one of the
    // ones that was just loaded, but to maintain the previous user experience
    // we need to set it to a blank layer in case they load another file.
    // We can't start with the next available layer when loading files because
    // some users expect the behavior of overwriting the active layer on load.
    SetActiveLayer( getNextAvailableLayer( layer ), true );

    m_LayersManager->UpdateLayerIcons();
    syncLayerBox( true );

    GetCanvas()->Refresh();

    return success;
}


bool GERBVIEW_FRAME::LoadExcellonFiles( const wxString& aFullFileName )
{
    wxString   filetypes;
    wxArrayString filenamesList;
    wxFileName filename = aFullFileName;
    wxString currentPath;

    if( !filename.IsOk() )
    {
        filetypes = DrillFileWildcard();
        filetypes << wxT( "|" );

        /* All filetypes */
        filetypes += AllFilesWildcard();

        /* Use the current working directory if the file name path does not exist. */
        if( filename.DirExists() )
            currentPath = filename.GetPath();
        else
            currentPath = m_mruPath;

        wxFileDialog dlg( this, _( "Open NC (Excellon) Drill File(s)" ),
                          currentPath, filename.GetFullName(), filetypes,
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE | wxFD_CHANGE_DIR );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        dlg.GetPaths( filenamesList );
        currentPath = wxGetCwd();
        m_mruPath = currentPath;
    }
    else
    {
        filenamesList.Add( aFullFileName );
        currentPath = filename.GetPath();
        m_mruPath = currentPath;
    }

    // Read Excellon drill files: each file is loaded on a new GerbView layer
    bool success = true;
    int layer = GetActiveLayer();

    // Manage errors when loading files
    wxString msg;
    WX_STRING_REPORTER reporter( &msg );

    for( unsigned ii = 0; ii < filenamesList.GetCount(); ii++ )
    {
        filename = filenamesList[ii];

        if( !filename.IsAbsolute() )
            filename.SetPath( currentPath );

        m_lastFileName = filename.GetFullPath();

        SetActiveLayer( layer, false );

        if( Read_EXCELLON_File( filename.GetFullPath() ) )
        {
            // Update the list of recent drill files.
            UpdateFileHistory( filename.GetFullPath(), &m_drillFileHistory );

            layer = getNextAvailableLayer( layer );

            if( layer == NO_AVAILABLE_LAYERS && ii < filenamesList.GetCount()-1 )
            {
                success = false;
                reporter.Report( MSG_NO_MORE_LAYER, REPORTER::RPT_ERROR );

                // Report the name of not loaded files:
                ii += 1;
                while( ii < filenamesList.GetCount() )
                {
                    filename = filenamesList[ii++];
                    wxString txt = wxString::Format( MSG_NOT_LOADED, filename.GetFullName() );
                    reporter.Report( txt, REPORTER::RPT_ERROR );
                }
                break;
            }

            SetActiveLayer( layer, false );
        }
    }

    if( !success )
    {
        HTML_MESSAGE_BOX mbox( this, _( "Errors" ) );
        mbox.ListSet( msg );
        mbox.ShowModal();
    }

    Zoom_Automatique( false );

    // Synchronize layers tools with actual active layer:
    ReFillLayerWidget();
    SetActiveLayer( GetActiveLayer() );
    m_LayersManager->UpdateLayerIcons();
    syncLayerBox();

    return success;
}


bool GERBVIEW_FRAME::unarchiveFiles( const wxString& aFullFileName, REPORTER* aReporter )
{
    wxString msg;

    // Extract the path of aFullFileName. We use it to store temporary files
    wxFileName fn( aFullFileName );
    wxString unzipDir = fn.GetPath();

    wxFFileInputStream zipFile( aFullFileName );

    if( !zipFile.IsOk() )
    {
        if( aReporter )
        {
            msg.Printf( _( "Zip file \"%s\" cannot be opened" ), GetChars( aFullFileName ) );
            aReporter->Report( msg, REPORTER::RPT_ERROR );
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


    bool success = true;
    wxZipInputStream zipArchive( zipFile );
    wxZipEntry* entry;
    bool reported_no_more_layer = false;

    while( ( entry = zipArchive.GetNextEntry() ) )
    {
        wxString fname = entry->GetName();
        wxFileName uzfn = fname;
        wxString curr_ext = uzfn.GetExt().Lower();

        // The archive contains Gerber and/or Excellon drill files. Use the right loader.
        // However it can contain a few other files (reports, pdf files...),
        // which will be skipped.
        // Gerber files ext is usually "gbr", but can be also another value, starting by "g"
        // old gerber files ext from kicad is .pho
        // drill files do not have a well defined ext
        // It is .drl in kicad, but .txt in Altium for instance
        // Allows only .drl for drill files.
        if( curr_ext[0] != 'g' && curr_ext != "pho" && curr_ext != "drl" )
        {
            if( aReporter )
            {
                msg.Printf( _( "Info: skip file <i>\"%s\"</i> (unknown type)\n" ),
                            GetChars( entry->GetName() ) );
                aReporter->Report( msg, REPORTER::RPT_WARNING );
            }

            continue;
        }

        int layer = GetActiveLayer();

        if( layer == NO_AVAILABLE_LAYERS )
        {
            success = false;

            if( aReporter )
            {
                if( !reported_no_more_layer )
                    aReporter->Report( MSG_NO_MORE_LAYER, REPORTER::RPT_ERROR );

                reported_no_more_layer = true;

                // Report the name of not loaded files:
                msg.Printf( MSG_NOT_LOADED, GetChars( entry->GetName() ) );
                aReporter->Report( msg, REPORTER::RPT_ERROR );
            }

            delete entry;
            continue;
        }

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
                    msg.Printf( _( "<b>Unable to create temporary file \"%s\"</b>\n"),
                                GetChars( unzipped_tempfile ) );
                    aReporter->Report( msg, REPORTER::RPT_ERROR );
                }
            }
        }

        bool read_ok = true;

        if( curr_ext[0] == 'g' || curr_ext == "pho" )
        {
            // Read gerber files: each file is loaded on a new GerbView layer
            read_ok = Read_GERBER_File( unzipped_tempfile );
        }
        else // if( curr_ext == "drl" )
        {
            read_ok = Read_EXCELLON_File( unzipped_tempfile );
        }

        delete entry;

        // The unzipped file is only a temporary file, delete it.
        wxRemoveFile( unzipped_tempfile );

        if( !read_ok )
        {
            success = false;

            if( aReporter )
            {
                msg.Printf( _("<b>unzipped file %s read error</b>\n"),
                            GetChars( unzipped_tempfile ) );
                aReporter->Report( msg, REPORTER::RPT_ERROR );
            }
        }
        else
        {
            GERBER_FILE_IMAGE* gerber_image = GetGbrImage( layer );

            if( gerber_image )
                gerber_image->m_FileName = fname;

            layer = getNextAvailableLayer( layer );
            SetActiveLayer( layer, false );
        }
    }

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

        wxFileDialog dlg( this,
                          _( "Open Zip File" ),
                          currentPath,
                          filename.GetFullName(),
                          ZipFileWildcard(),
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

    wxString msg;
    WX_STRING_REPORTER reporter( &msg );

    if( filename.IsOk() )
        unarchiveFiles( filename.GetFullPath(), &reporter );

    Zoom_Automatique( false );

    // Synchronize layers tools with actual active layer:
    ReFillLayerWidget();
    SetActiveLayer( GetActiveLayer() );
    m_LayersManager->UpdateLayerIcons();
    syncLayerBox();

    if( !msg.IsEmpty() )
    {
        wxSafeYield();  // Allows slice of time to redraw the screen
                        // to refresh widgets, before displaying messages
        HTML_MESSAGE_BOX mbox( this, _( "Messages" ) );
        mbox.ListSet( msg );
        mbox.ShowModal();
    }

    return true;
}
