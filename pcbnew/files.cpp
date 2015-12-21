/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011-2015 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2015 KiCad Developers, see change_log.txt for contributors.
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

/**
 * @file pcbnew/files.cpp
 * @brief Read and write board files.
 */

#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <kicad_string.h>
#include <gestfich.h>
#include <wxPcbStruct.h>
#include <macros.h>
#include <3d_viewer.h>
#include <richio.h>
#include <filter_reader.h>
#include <pgm_base.h>
#include <msgpanel.h>
#include <fp_lib_table.h>
#include <ratsnest_data.h>

#include <pcbnew.h>
#include <pcbnew_id.h>
#include <io_mgr.h>
#include <wildcards_and_files_ext.h>

#include <class_board.h>
#include <build_version.h>      // LEGACY_BOARD_FILE_VERSION
#include <module_editor_frame.h>
#include <modview_frame.h>

#include <wx/stdpaths.h>


//#define     USE_INSTRUMENTATION     true
#define     USE_INSTRUMENTATION     false


static const wxChar backupSuffix[]   = wxT( "-bak" );
static const wxChar autosavePrefix[] = wxT( "_autosave-" );


/**
 * Function AskLoadBoardFileName
 * puts up a wxFileDialog asking for a BOARD filename to open.
 *
 * @param aParent is a wxFrame passed to wxFileDialog.
 * @param aCtl is where to put the OpenProjectFiles() control bits.
 *
 * @param aFileName on entry is a probable choice, on return is the chosen filename.
 * @param aKicadFilesOnly true to list kiacad pcb files plugins only, false to list all plugins.
 *
 * @return bool - true if chosen, else false if user aborted.
 */
bool AskLoadBoardFileName( wxWindow* aParent, int* aCtl, wxString* aFileName, bool aKicadFilesOnly = false )
{
    // This is a subset of all PLUGINs which are trusted to be able to
    // load a BOARD. User may occasionally use the wrong plugin to load a
    // *.brd file (since both legacy and eagle use *.brd extension),
    // but eventually *.kicad_pcb will be more common than legacy *.brd files.
    static const struct
    {
        const wxString&     filter;
        IO_MGR::PCB_FILE_T  pluginType;
    } loaders[] =
    {
        { PcbFileWildcard,          IO_MGR::KICAD },
        { LegacyPcbFileWildcard,    IO_MGR::LEGACY },
        { EaglePcbFileWildcard,     IO_MGR::EAGLE },
        { PCadPcbFileWildcard,      IO_MGR::PCAD },
    };

    wxFileName  fileName( *aFileName );
    wxString    fileFilters;

    unsigned pluginsCount = aKicadFilesOnly ? 2 : DIM( loaders );

    for( unsigned i=0; i < pluginsCount; ++i )
    {
        if( i > 0 )
            fileFilters += wxChar( '|' );

        fileFilters += wxGetTranslation( loaders[i].filter );
    }

    wxString    path;
    wxString    name;

    if( fileName.FileExists() )
    {
        path = fileName.GetPath();
        name = fileName.GetFullName();
    }
    else
    {
        path = wxStandardPaths::Get().GetDocumentsDir();
        // leave name empty
    }

    wxFileDialog dlg( aParent, _( "Open Board File" ), path, name, fileFilters,
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() != wxID_CANCEL )
    {
        int chosenFilter = dlg.GetFilterIndex();

        // if Eagle, tell OpenProjectFiles() to use Eagle plugin.  It's the only special
        // case because of the duplicate use of the *.brd file extension.  Other cases
        // are clear because of unique file extensions.
        *aCtl = chosenFilter == 2  ? KICTL_EAGLE_BRD : 0;
        *aFileName = dlg.GetPath();

        return true;
    }
    else
        return false;
}


/**
 * Function AskSaveBoardFileName
 * puts up a wxFileDialog asking for a BOARD filename to save.
 *
 * @param aParent is a wxFrame passed to wxFileDialog.
 * @param aFileName on entry is a probable choice, on return is the
 *  chosen full filename (includes path).
 *
 * @return bool - true if chosen, else false if user aborted.
 */
bool AskSaveBoardFileName( wxWindow* aParent, wxString* aFileName )
{
    wxString    wildcard =  wxGetTranslation( PcbFileWildcard );
    wxFileName  fn = *aFileName;

    fn.SetExt( KiCadPcbFileExtension );

    wxFileDialog dlg( aParent,
            _( "Save Board File As" ),
            fn.GetPath(),
            fn.GetFullName(),
            wildcard,
            wxFD_SAVE
            /* wxFileDialog is not equipped to handle multiple wildcards and
               wxFD_OVERWRITE_PROMPT both together.
               | wxFD_OVERWRITE_PROMPT
            */
            );

    if( dlg.ShowModal() != wxID_OK )
        return false;

    fn = dlg.GetPath();

    // always enforce filename extension, user may not have entered it.
    fn.SetExt( KiCadPcbFileExtension );

    // Since the file overwrite test was removed from wxFileDialog because it doesn't work
    // when multiple wildcards are defined, we have to check it ourselves to prevent an
    // existing board file from silently being over written.
    if( fn.FileExists() )
    {
        wxString ask = wxString::Format( _(
                "The file '%s' already exists.\n\n"
                "Do you want to overwrite it?" ),
                 GetChars( fn.GetFullPath() )
                 );

        if( !IsOK( aParent, ask ) )
        {
            return false;
        }
    }

    *aFileName = fn.GetFullPath();

    return true;
}


void PCB_EDIT_FRAME::OnFileHistory( wxCommandEvent& event )
{
    wxString fn = GetFileFromHistory( event.GetId(), _( "Printed circuit board" ) );

    if( !!fn )
    {
        int open_ctl = 0;

        m_canvas->EndMouseCapture( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor() );

        if( !wxFileName::IsFileReadable( fn ) )
        {
            if( !AskLoadBoardFileName( this, &open_ctl, &fn ) )
                return;
        }

        OpenProjectFiles( std::vector<wxString>( 1, fn ), open_ctl );
    }
}


void PCB_EDIT_FRAME::Files_io( wxCommandEvent& event )
{
    int        id = event.GetId();
    Files_io_from_id( id );
}

void PCB_EDIT_FRAME::Files_io_from_id( int id )
{
    wxString   msg;

    // If an edition is in progress, stop it.
    // For something else than save, get rid of current tool.
    if( id == ID_SAVE_BOARD )
        m_canvas->EndMouseCapture( -1, m_canvas->GetDefaultCursor() );
    else
        m_canvas->EndMouseCapture( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor() );

    switch( id )
    {
    case ID_LOAD_FILE:
        {
            // LoadOnePcbFile( GetBoard()->GetFileName(), append=false, aForceFileDialog=true );

            int         open_ctl;
            wxString    fileName = Prj().AbsolutePath( GetBoard()->GetFileName() );

            if( !AskLoadBoardFileName( this, &open_ctl, &fileName ) )
                return;

            OpenProjectFiles( std::vector<wxString>( 1, fileName ), open_ctl );
        }
        break;

    case ID_MENU_READ_BOARD_BACKUP_FILE:
    case ID_MENU_RECOVER_BOARD_AUTOSAVE:
        {
            wxFileName currfn = Prj().AbsolutePath( GetBoard()->GetFileName() );
            wxFileName fn = currfn;

            if( id == ID_MENU_RECOVER_BOARD_AUTOSAVE )
            {
                wxString rec_name = wxString( autosavePrefix ) + fn.GetName();
                fn.SetName( rec_name );
            }
            else
            {
                wxString backup_ext = fn.GetExt()+ backupSuffix;
                fn.SetExt( backup_ext );
            }

            if( !fn.FileExists() )
            {
                msg.Printf( _( "Recovery file '%s' not found." ),
                            GetChars( fn.GetFullPath() ) );
                DisplayInfoMessage( this, msg );
                break;
            }

            msg.Printf( _( "OK to load recovery or backup file '%s'" ),
                            GetChars(fn.GetFullPath() ) );

            if( !IsOK( this, msg ) )
                break;

            GetScreen()->ClrModify();    // do not prompt the user for changes

            // LoadOnePcbFile( fn.GetFullPath(), aAppend=false, aForceFileDialog=false );
            OpenProjectFiles( std::vector<wxString>( 1, fn.GetFullPath() ) );

            // Re-set the name since name or extension was changed
            GetBoard()->SetFileName( currfn.GetFullPath() );
            UpdateTitle();
        }
        break;

    case ID_APPEND_FILE:
        {
            int         open_ctl;
            wxString    fileName;

            if( !AskLoadBoardFileName( this, &open_ctl, &fileName, true ) )
                break;

            AppendBoardFile( fileName, open_ctl );

            m_canvas->Refresh();
        }
        break;

    case ID_NEW_BOARD:
    {
        if( !Clear_Pcb( true ) )
            break;

        wxFileName fn( wxStandardPaths::Get().GetDocumentsDir(), wxT( "noname" ),
                       ProjectFileExtension );

        Prj().SetProjectFullName( fn.GetFullPath() );

        fn.SetExt( PcbFileExtension );

        GetBoard()->SetFileName( fn.GetFullPath() );
        UpdateTitle();
        ReCreateLayerBox();
        break;
    }

    case ID_SAVE_BOARD:
        if( ! GetBoard()->GetFileName().IsEmpty() )
        {
            SavePcbFile( Prj().AbsolutePath( GetBoard()->GetFileName() ) );
            break;
        }
    // Fall through
    case ID_COPY_BOARD_AS:
    case ID_SAVE_BOARD_AS:
        {
            wxString    pro_dir = wxPathOnly( Prj().GetProjectFullName() );
            wxFileName  fn( pro_dir, _( "noname" ), KiCadPcbFileExtension );
            wxString    filename = fn.GetFullPath();

            if( AskSaveBoardFileName( this, &filename ) )
            {
                if( id == ID_COPY_BOARD_AS )
                    SavePcbCopy( filename );
                else
                    SavePcbFile( filename, NO_BACKUP_FILE );
            }
        }
        break;

    default:
        DisplayError( this, wxT( "File_io Internal Error" ) );
        break;
    }
}


// The KIWAY_PLAYER::OpenProjectFiles() API knows nothing about plugins, so
// determine how to load the BOARD here, with minor assistance from KICTL_EAGLE_BRD
// bit flag.
IO_MGR::PCB_FILE_T plugin_type( const wxString& aFileName, int aCtl )
{
    IO_MGR::PCB_FILE_T  pluginType;

    wxFileName fn = aFileName;

    if( fn.GetExt() == IO_MGR::GetFileExtension( IO_MGR::LEGACY ) )
    {
        // both legacy and eagle share a common file extension.
        pluginType = ( aCtl & KICTL_EAGLE_BRD ) ? IO_MGR::EAGLE : IO_MGR::LEGACY;
    }
    else if( fn.GetExt() == IO_MGR::GetFileExtension( IO_MGR::LEGACY ) + backupSuffix )
    {
        pluginType = IO_MGR::LEGACY;
    }
    else if( fn.GetExt() == IO_MGR::GetFileExtension( IO_MGR::IO_MGR::PCAD ) )
    {
        pluginType = IO_MGR::PCAD;
    }
    else
        pluginType = IO_MGR::KICAD;

    return pluginType;
}


bool PCB_EDIT_FRAME::OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl )
{
    // This is for python:
    if( aFileSet.size() != 1 )
    {
        UTF8 msg = StrPrintf( "Pcbnew:%s() takes only a single filename", __func__ );
        DisplayError( this, msg );
        return false;
    }

    wxString fullFileName( aFileSet[0] );

    // We insist on caller sending us an absolute path, if it does not, we say it's a bug.
    wxASSERT_MSG( wxFileName( fullFileName ).IsAbsolute(),
        wxT( "bug in single_top.cpp or project manager." ) );

    if( !LockFile( fullFileName ) )
    {
        wxString msg = wxString::Format( _(
                "PCB file '%s' is already open." ),
                GetChars( fullFileName )
                );
        DisplayError( this, msg );
        return false;
    }

    if( GetScreen()->IsModify() )
    {
        int response = YesNoCancelDialog( this, _(
            "The current board has been modified.  Do you wish to save the changes?" ),
            wxEmptyString,
            _( "Save and Load" ),
            _( "Load Without Saving" )
            );

        if( response == wxID_CANCEL )
            return false;
        else if( response == wxID_YES )
            SavePcbFile( GetBoard()->GetFileName(), CREATE_BACKUP_FILE );
        else
        {
            // response == wxID_NO, fall thru
        }
    }

    wxFileName pro = fullFileName;
    pro.SetExt( ProjectFileExtension );

    bool is_new = !wxFileName::IsFileReadable( fullFileName );

    // If its a non-existent schematic and caller thinks it exists
    if( is_new && !( aCtl & KICTL_CREATE ) )
    {
        // notify user that fullFileName does not exist, ask if user wants to create it.
        wxString ask = wxString::Format( _(
                "Board '%s' does not exist.  Do you wish to create it?" ),
                GetChars( fullFileName )
                );
        if( !IsOK( this, ask ) )
            return false;
    }

    Clear_Pcb( false );     // pass false since we prompted above for a modified board

    IO_MGR::PCB_FILE_T  pluginType = plugin_type( fullFileName, aCtl );

    bool converted =  pluginType != IO_MGR::LEGACY && pluginType != IO_MGR::KICAD;

    if( !converted )
    {
        // PROJECT::SetProjectFullName() is an impactful function.  It should only be
        // called under carefully considered circumstances.

        // The calling code should know not to ask me here to change projects unless
        // it knows what consequences that will have on other KIFACEs running and using
        // this same PROJECT.  It can be very harmful if that calling code is stupid.
        Prj().SetProjectFullName( pro.GetFullPath() );

        // load project settings before BOARD
        LoadProjectSettings();
    }

    if( is_new )
    {
        OnModify();
    }
    else
    {
        BOARD* loadedBoard = 0;   // it will be set to non-NULL if loaded OK

        PLUGIN::RELEASER pi( IO_MGR::PluginFind( pluginType ) );

        try
        {
            PROPERTIES  props;
            char        xbuf[30];
            char        ybuf[30];

            // EAGLE_PLUGIN can use this info to center the BOARD, but it does not yet.
            sprintf( xbuf, "%d", GetPageSizeIU().x );
            sprintf( ybuf, "%d", GetPageSizeIU().y );

            props["page_width"]  = xbuf;
            props["page_height"] = ybuf;

#if USE_INSTRUMENTATION
            // measure the time to load a BOARD.
            unsigned startTime = GetRunningMicroSecs();
#endif

            loadedBoard = pi->Load( fullFileName, NULL, &props );

#if USE_INSTRUMENTATION
            unsigned stopTime = GetRunningMicroSecs();
            printf( "PLUGIN::Load(): %u usecs\n", stopTime - startTime );
#endif
        }
        catch( const IO_ERROR& ioe )
        {
            wxString msg = wxString::Format( _(
                    "Error loading board.\n%s" ),
                    GetChars( ioe.errorText )
                    );
            DisplayError( this, msg );

            return false;
        }

        SetBoard( loadedBoard );

        // we should not ask PLUGINs to do these items:
        loadedBoard->BuildListOfNets();
        loadedBoard->SynchronizeNetsAndNetClasses();

        SetStatusText( wxEmptyString );
        BestZoom();

        // update the layer names in the listbox
        ReCreateLayerBox( false );

        GetScreen()->ClrModify();

        {
            wxFileName fn = fullFileName;
            CheckForAutoSaveFile( fullFileName, fn.GetExt() );
        }

        if( pluginType == IO_MGR::LEGACY &&
            loadedBoard->GetFileFormatVersionAtLoad() < LEGACY_BOARD_FILE_VERSION )
        {
            DisplayInfoMessage( this,
                _(  "This file was created by an older version of Pcbnew.\n"
                    "It will be stored in the new file format when you save this file again." ) );
        }
    }

    {
        wxFileName fn = fullFileName;

        if( converted )
            fn.SetExt( PcbFileExtension );

        wxString fname = fn.GetFullPath();

        fname.Replace( WIN_STRING_DIR_SEP, UNIX_STRING_DIR_SEP );

        GetBoard()->SetFileName( fname );
    }

    UpdateTitle();

    if( !converted )
        UpdateFileHistory( GetBoard()->GetFileName() );

    // Rebuild the new pad list (for drc and ratsnet control ...)
    GetBoard()->m_Status_Pcb = 0;

    // Update info shown by the horizontal toolbars
    SetCurrentNetClass( NETCLASS::Default );
    ReFillLayerWidget();
    ReCreateLayerBox();

    // upate the layer widget to match board visibility states, both layers and render columns.
    syncLayerVisibilities();
    syncLayerWidgetLayer();
    syncRenderStates();

    // Update the tracks / vias available sizes list:
    ReCreateAuxiliaryToolbar();

    // Update the RATSNEST items, which were not loaded at the time
    // BOARD::SetVisibleElements() was called from within any PLUGIN.
    // See case RATSNEST_VISIBLE: in BOARD::SetElementVisibility()
    GetBoard()->SetVisibleElements( GetBoard()->GetVisibleElements() );

    // Display the loaded board:
    Zoom_Automatique( false );

    // Compile ratsnest and displays net info
    {
        wxBusyCursor dummy;    // Displays an Hourglass while building connectivity
        Compile_Ratsnest( NULL, true );
        GetBoard()->GetRatsnest()->ProcessBoard();
    }

    SetMsgPanel( GetBoard() );

    // Refresh the 3D view, if any
    EDA_3D_FRAME* draw3DFrame = Get3DViewerFrame();

    if( draw3DFrame )
        draw3DFrame->NewDisplay();

#if 0 && defined(DEBUG)
    // Output the board object tree to stdout, but please run from command prompt:
    GetBoard()->Show( 0, std::cout );
#endif

    // from EDA_APPL which was first loaded BOARD only:
    {
        /* For an obscure reason the focus is lost after loading a board file
         * when starting up the process.
         * (seems due to the recreation of the layer manager after loading the file)
         * Give focus to main window and Drawpanel
         * must be done for these 2 windows (for an obscure reason ...)
         * Linux specific
         * This is more a workaround than a fix.
         */
        SetFocus();
        GetCanvas()->SetFocus();
    }

    return true;
}


static wxString create_backup_file( const wxString& aFileName )
{
    wxFileName  fn = aFileName;
    wxFileName  backupFileName = aFileName;

    backupFileName.SetExt( fn.GetExt() + backupSuffix );

    // If an old backup file exists, delete it.  If an old board file exists,
    // rename it to the backup file name.
    if( fn.FileExists() )
    {
        // Remove the old file xxx.000 if it exists.
        if( backupFileName.FileExists() )
            wxRemoveFile( backupFileName.GetFullPath() );

        // Rename the current file from <xxx>.kicad_pcb to <xxx>.kicad_pcb-bak
        if( !wxRenameFile( fn.GetFullPath(), backupFileName.GetFullPath() ) )
        {
            wxString msg = wxString::Format( _(
                    "Warning: unable to create backup file '%s'" ),
                    GetChars( backupFileName.GetFullPath() )
                    );
            DisplayError( NULL, msg );
        }
    }
    else
    {
        backupFileName.Clear();
    }

    return backupFileName.GetFullPath();
}


bool PCB_EDIT_FRAME::SavePcbFile( const wxString& aFileName, bool aCreateBackupFile )
{
    // please, keep it simple.  prompting goes elsewhere.

    wxFileName  pcbFileName = aFileName;

    if( pcbFileName.GetExt() == LegacyPcbFileExtension )
        pcbFileName.SetExt( KiCadPcbFileExtension );

    if( !IsWritable( pcbFileName ) )
    {
        wxString msg = wxString::Format( _(
            "No access rights to write to file '%s'" ),
            GetChars( pcbFileName.GetFullPath() )
            );

        DisplayError( this, msg );
        return false;
    }

    wxString backupFileName;

    // aCreateBackupFile == false is mainly used to write autosave files
    // or new files in save as... command
    if( aCreateBackupFile )
    {
        backupFileName = create_backup_file( aFileName );
    }

    GetBoard()->m_Status_Pcb &= ~CONNEXION_OK;

    GetBoard()->SynchronizeNetsAndNetClasses();

    // Select default Netclass before writing file.
    // Useful to save default values in headers
    SetCurrentNetClass( NETCLASS::Default );

    ClearMsgPanel();

    wxString    upperTxt;
    wxString    lowerTxt;

    try
    {
        PLUGIN::RELEASER    pi( IO_MGR::PluginFind( IO_MGR::KICAD ) );

        wxASSERT( pcbFileName.IsAbsolute() );

        pi->Save( pcbFileName.GetFullPath(), GetBoard(), NULL );
    }
    catch( const IO_ERROR& ioe )
    {
        wxString msg = wxString::Format( _(
                "Error saving board file '%s'.\n%s" ),
                GetChars( pcbFileName.GetFullPath() ),
                GetChars( ioe.errorText )
                );
        DisplayError( this, msg );

        lowerTxt.Printf( _( "Failed to create '%s'" ), GetChars( pcbFileName.GetFullPath() ) );

        AppendMsgPanel( upperTxt, lowerTxt, CYAN );

        return false;
    }

    GetBoard()->SetFileName( pcbFileName.GetFullPath() );
    UpdateTitle();

    // Put the saved file in File History, unless aCreateBackupFile
    // is false.
    // aCreateBackupFile == false is mainly used to write autosave files
    // and not need to have an autosave file in file history
    if( aCreateBackupFile )
        UpdateFileHistory( GetBoard()->GetFileName() );

    // Delete auto save file on successful save.
    wxFileName autoSaveFileName = pcbFileName;

    autoSaveFileName.SetName( wxString( autosavePrefix ) + pcbFileName.GetName() );

    if( autoSaveFileName.FileExists() )
        wxRemoveFile( autoSaveFileName.GetFullPath() );

    if( !!backupFileName )
        upperTxt.Printf( _( "Backup file: '%s'" ), GetChars( backupFileName ) );

    lowerTxt.Printf( _( "Wrote board file: '%s'" ), GetChars( pcbFileName.GetFullPath() ) );

    AppendMsgPanel( upperTxt, lowerTxt, CYAN );

    GetScreen()->ClrModify();
    GetScreen()->ClrSave();
    return true;
}


bool PCB_EDIT_FRAME::SavePcbCopy( const wxString& aFileName )
{
    wxFileName  pcbFileName = aFileName;

    // Ensure the file ext is the right ext:
    pcbFileName.SetExt( KiCadPcbFileExtension );

    if( !IsWritable( pcbFileName ) )
    {
        wxString msg = wxString::Format( _(
            "No access rights to write to file '%s'" ),
            GetChars( pcbFileName.GetFullPath() )
            );

        DisplayError( this, msg );
        return false;
    }

    GetBoard()->m_Status_Pcb &= ~CONNEXION_OK;
    GetBoard()->SynchronizeNetsAndNetClasses();

    // Select default Netclass before writing file.
    // Useful to save default values in headers
    SetCurrentNetClass( NETCLASS::Default );

    try
    {
        PLUGIN::RELEASER    pi( IO_MGR::PluginFind( IO_MGR::KICAD ) );

        wxASSERT( pcbFileName.IsAbsolute() );

        pi->Save( pcbFileName.GetFullPath(), GetBoard(), NULL );
    }
    catch( const IO_ERROR& ioe )
    {
        wxString msg = wxString::Format( _(
                "Error saving board file '%s'.\n%s" ),
                GetChars( pcbFileName.GetFullPath() ),
                GetChars( ioe.errorText )
                );
        DisplayError( this, msg );

        return false;
    }

    DisplayInfoMessage( this, wxString::Format( _( "Board copied to:\n'%s'" ),
                                GetChars( pcbFileName.GetFullPath() ) ) );

    return true;
}


bool PCB_EDIT_FRAME::doAutoSave()
{
    wxFileName tmpFileName = Prj().AbsolutePath( GetBoard()->GetFileName() );
    wxFileName fn = tmpFileName;

    // Auto save file name is the normal file name prepended with
    // autosaveFilePrefix string.
    fn.SetName( wxString( autosavePrefix ) + fn.GetName() );

    wxLogTrace( traceAutoSave,
                wxT( "Creating auto save file <" + fn.GetFullPath() ) + wxT( ">" ) );

    if( !fn.IsOk() )
        return false;
    else if( SavePcbFile( fn.GetFullPath(), NO_BACKUP_FILE ) )
    {
        GetScreen()->SetModify();
        GetBoard()->SetFileName( tmpFileName.GetFullPath() );
        UpdateTitle();
        m_autoSaveState = false;
        return true;
    }

    GetBoard()->SetFileName( tmpFileName.GetFullPath() );

    return false;
}
