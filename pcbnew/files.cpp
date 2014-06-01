/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2010 Jean-Pierre Charras, jean-pierre.charras@gpisa-lab.inpg.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2010 KiCad Developers, see change_log.txt for contributors.
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
#include <pcbcommon.h>
#include <3d_viewer.h>
#include <richio.h>
#include <filter_reader.h>
#include <pgm_base.h>
#include <msgpanel.h>
#include <fp_lib_table.h>

#include <pcbnew.h>
#include <pcbnew_id.h>
#include <io_mgr.h>
#include <wildcards_and_files_ext.h>

#include <class_board.h>
#include <build_version.h>      // LEGACY_BOARD_FILE_VERSION
#include <module_editor_frame.h>
#include <modview_frame.h>


//#define     USE_INSTRUMENTATION     true
#define     USE_INSTRUMENTATION     false


static const wxChar backupSuffix[]  = wxT( "-bak" );
static const wxChar autosavePrefix[]= wxT( "_autosave-" );


void PCB_EDIT_FRAME::OnFileHistory( wxCommandEvent& event )
{
    wxString fn = GetFileFromHistory( event.GetId(), _( "Printed circuit board" ) );

    if( !!fn )
    {
        int open_ctl = 0;

        m_canvas->EndMouseCapture( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor() );
        ::wxSetWorkingDirectory( ::wxPathOnly( fn ) );

        // LoadOnePcbFile( fn, bool aAppend = false,  bool aForceFileDialog = false );
        if( !wxFileName::IsFileReadable( fn ) )
        {
            if( !AskBoardFileName( this, &open_ctl, &fn ) )
                return;
        }

        OpenProjectFiles( std::vector<wxString>( 1, fn ), open_ctl );
    }
}


void PCB_EDIT_FRAME::Files_io( wxCommandEvent& event )
{
    int        id = event.GetId();
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
            wxString    fileName = GetBoard()->GetFileName();

            if( !AskBoardFileName( this, &open_ctl, &fileName ) )
                return;

            OpenProjectFiles( std::vector<wxString>( 1, fileName ), open_ctl );
        }
        break;

    case ID_MENU_READ_BOARD_BACKUP_FILE:
    case ID_MENU_RECOVER_BOARD_AUTOSAVE:
        {
            wxFileName currfn = GetBoard()->GetFileName();
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
            // LoadOnePcbFile( wxEmptyString, aAppend = true, aForceFileDialog=false );
            int         open_ctl;
            wxString    fileName;

            if( !AskBoardFileName( this, &open_ctl, &fileName ) )
                break;

            OpenProjectFiles( std::vector<wxString>( 1, fileName ), open_ctl | KICTL_OPEN_APPEND );
        }
        break;

    case ID_NEW_BOARD:
        {
            if( ! Clear_Pcb( true ) )
                break;

            // Clear footprint library table for the new board.
            Prj().PcbFootprintLibs()->Clear();

            wxFileName fn;

            fn.AssignCwd();
            fn.SetName( wxT( "noname" ) );

            Prj().SetProjectFullName( fn.GetFullPath() );

            fn.SetExt( PcbFileExtension );

            GetBoard()->SetFileName( fn.GetFullPath() );
            UpdateTitle();
            ReCreateLayerBox();
        }
        break;

    case ID_SAVE_BOARD:
        SavePcbFile( GetBoard()->GetFileName() );
        break;

    case ID_SAVE_BOARD_AS:
        SavePcbFile( wxEmptyString );
        break;

    default:
        DisplayError( this, wxT( "File_io Internal Error" ) ); break;
    }
}


bool AskBoardFileName( wxWindow* aParent, int* aCtl, wxString* aFileName )
{
    // This is a subset of all PLUGINs which are trusted to be able to
    // load a BOARD.  Order is subject to change as KICAD plugin matures.
    // User may occasionally use the wrong plugin to load a *.brd file,
    // (since both legacy and eagle use *.brd extension),
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

    for( unsigned i=0;  i<DIM( loaders );  ++i )
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
        path = wxGetCwd();
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


bool PCB_EDIT_FRAME::OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl )
{
    wxASSERT( aFileSet.size() == 1 );

    bool        doAppend = aCtl & KICTL_OPEN_APPEND;
    wxFileName  fileName( aFileSet[0] );

    // Make filename absolute, to avoid issues when the filename is relative,
    // for instance when stored in history list without path, and when building
    // the config filename ( which should have a path )
    if( fileName.IsRelative() )
        fileName.MakeAbsolute();

    if( GetScreen()->IsModify() && !doAppend )
    {
        int response = YesNoCancelDialog( this, _(
            "The current board has been modified.  Do "
            "you wish to save the changes?" ),
            wxEmptyString,
            _( "Save and Load" ),
            _( "Load Without Saving" )
            );

        if( response == wxID_CANCEL )
            return false;
        else if( response == wxID_YES )
            SavePcbFile( GetBoard()->GetFileName(), true );
    }

    if( doAppend )
    {
        GetBoard()->SetFileName( wxEmptyString );
        OnModify();
        GetBoard()->m_Status_Pcb = 0;
    }

    // The KIWAY_PLAYER::OpenProjectFiles() API knows nothing about plugins, so
    // determine how to load the BOARD here, with minor assistance from KICTL_EAGLE_BRD
    // bit flag.

    IO_MGR::PCB_FILE_T  pluginType;

    if( fileName.GetExt() == IO_MGR::GetFileExtension( IO_MGR::LEGACY ) )
    {
        // both legacy and eagle share a common file extension.
        pluginType = ( aCtl & KICTL_EAGLE_BRD ) ? IO_MGR::EAGLE : IO_MGR::LEGACY;
    }
    else if( fileName.GetExt() == IO_MGR::GetFileExtension( IO_MGR::LEGACY ) + backupSuffix )
    {
        pluginType = IO_MGR::LEGACY;
    }
    else if( fileName.GetExt() == IO_MGR::GetFileExtension( IO_MGR::IO_MGR::PCAD ) )
    {
        pluginType = IO_MGR::PCAD;
    }
    else
        pluginType = IO_MGR::KICAD;

    PLUGIN::RELEASER pi( IO_MGR::PluginFind( pluginType ) );

    if( !doAppend )
    {
        if( !Pgm().LockFile( fileName.GetFullPath() ) )
        {
            DisplayError( this, _( "This file is already open." ) );
            return false;
        }
        Clear_Pcb( false );     // pass false since we prompted above for a modified board
    }

    CheckForAutoSaveFile( fileName, fileName.GetExt() );

    GetBoard()->SetFileName( fileName.GetFullPath() );

    if( !doAppend )
    {
        // Update the option toolbar
        m_DisplayPcbTrackFill = DisplayOpt.DisplayPcbTrackFill;
        m_DisplayModText = DisplayOpt.DisplayModText;
        m_DisplayModEdge = DisplayOpt.DisplayModEdge;
        m_DisplayPadFill = DisplayOpt.DisplayPadFill;
        m_DisplayViaFill = DisplayOpt.DisplayViaFill;

        // load project settings before BOARD, in case BOARD file has overrides.
        LoadProjectSettings( GetBoard()->GetFileName() );
    }
    else
    {
        GetDesignSettings().m_NetClasses.Clear();
    }

    BOARD* loadedBoard = 0;   // it will be set to non-NULL if loaded OK

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

        // load or append either:
        loadedBoard = pi->Load( GetBoard()->GetFileName(), doAppend ? GetBoard() : NULL, &props );

#if USE_INSTRUMENTATION
        unsigned stopTime = GetRunningMicroSecs();
        printf( "PLUGIN::Load(): %u usecs\n", stopTime - startTime );
#endif

        // the Load plugin method makes a 'fresh' board, so we need to
        // set its own name
        GetBoard()->SetFileName( fileName.GetFullPath() );

        if( !doAppend )
        {
            if( pluginType == IO_MGR::LEGACY &&
                loadedBoard->GetFileFormatVersionAtLoad() < LEGACY_BOARD_FILE_VERSION )
            {
                DisplayInfoMessage( this,
                    _(  "This file was created by an older version of Pcbnew.\n"
                        "It will be stored in the new file format when you save this file again." ) );
            }

            SetBoard( loadedBoard );
        }
    }
    catch( const IO_ERROR& ioe )
    {
        wxString msg = wxString::Format( _( "Error loading board.\n%s" ),
                                         ioe.errorText.GetData() );
        wxMessageBox( msg, _( "Open Board File" ), wxOK | wxICON_ERROR );
    }

    if( loadedBoard )
    {
        // we should not ask PLUGINs to do these items:
        loadedBoard->BuildListOfNets();
        loadedBoard->SynchronizeNetsAndNetClasses();

        SetStatusText( wxEmptyString );
        BestZoom();

        // update the layer names in the listbox
        ReCreateLayerBox( false );
    }

    GetScreen()->ClrModify();

    if( doAppend )
    {
        // change the initial board name to <oldname>-append.brd
        wxString new_filename = GetBoard()->GetFileName().BeforeLast( '.' );

        if( !new_filename.EndsWith( wxT( "-append" ) ) )
            new_filename += wxT( "-append" );

        new_filename += wxT( "." ) + PcbFileExtension;

        OnModify();
        GetBoard()->SetFileName( new_filename );
    }

    // Fix the directory separator on Windows and
    // force the new file format for not Kicad boards,
    // to ensure the right format when saving the board
    bool converted =  pluginType != IO_MGR::LEGACY && pluginType != IO_MGR::KICAD;
    wxString fn;

    if( converted )
        fn = GetBoard()->GetFileName().BeforeLast( '.' );
    else
        fn = GetBoard()->GetFileName();

    fn.Replace( WIN_STRING_DIR_SEP, UNIX_STRING_DIR_SEP );

    if( converted )
        fn += wxT( "." ) + PcbFileExtension;

    GetBoard()->SetFileName( fn );

    UpdateTitle();

    if( !converted )
        UpdateFileHistory( GetBoard()->GetFileName() );

    // Rebuild the new pad list (for drc and ratsnet control ...)
    GetBoard()->m_Status_Pcb = 0;

    // Dick 5-Feb-2012: I do not agree with this.  The layer widget will show what
    // is visible or not, and it would be nice for the board to look like it
    // did when I saved it, immediately after loading.
#if 0
    /* Reset the items visibility flag when loading a new config
     * Because it could creates SERIOUS mistakes for the user,
     * if board items are not visible after loading a board...
     * Grid and ratsnest can be left to their previous state
     */
    bool showGrid = IsElementVisible( GRID_VISIBLE );
    bool showRats = IsElementVisible( RATSNEST_VISIBLE );

    SetVisibleAlls();

    SetElementVisibility( GRID_VISIBLE, showGrid );
    SetElementVisibility( RATSNEST_VISIBLE, showRats );
#endif

    // Update info shown by the horizontal toolbars
    GetDesignSettings().SetCurrentNetClass( NETCLASS::Default );
    ReFillLayerWidget();
    ReCreateLayerBox();

    // upate the layer widget to match board visibility states, both layers and render columns.
    syncLayerVisibilities();
    syncLayerWidgetLayer();
    syncRenderStates();

    // Update the RATSNEST items, which were not loaded at the time
    // BOARD::SetVisibleElements() was called from within any PLUGIN.
    // See case RATSNEST_VISIBLE: in BOARD::SetElementVisibility()
    GetBoard()->SetVisibleElements( GetBoard()->GetVisibleElements() );

    updateTraceWidthSelectBox();
    updateViaSizeSelectBox();

    // Display the loaded board:
    Zoom_Automatique( false );

    // Compile ratsnest and displays net info
    {
        wxBusyCursor dummy;    // Displays an Hourglass while building connectivity
        Compile_Ratsnest( NULL, true );
    }

    SetMsgPanel( GetBoard() );

    // Refresh the 3D view, if any
    if( m_Draw3DFrame )
        m_Draw3DFrame->NewDisplay();

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


bool PCB_EDIT_FRAME::SavePcbFile( const wxString& aFileName, bool aCreateBackupFile )
{
    wxFileName  backupFileName;
    wxFileName  pcbFileName;
    wxString    upperTxt;
    wxString    lowerTxt;
    wxString    msg;
    bool        saveok = true;
    bool        isSaveAs = false;

    IO_MGR::PCB_FILE_T pluginType;

    if( aFileName == wxEmptyString )
    {
        wxString    wildcard;
        wildcard << wxGetTranslation( PcbFileWildcard ) << wxChar( '|' ) <<
                    wxGetTranslation( LegacyPcbFileWildcard );

        isSaveAs = true;
        pcbFileName = GetBoard()->GetFileName();

        if( pcbFileName.GetName() == wxEmptyString )
        {
            pcbFileName.SetName( _( "Unnamed file" ) );
        }

        // Match the default wildcard filter choice, with the inital file extension shown.
        // That'll be the extension unless user changes filter dropdown listbox.
        pcbFileName.SetExt( KiCadPcbFileExtension );

        wxFileDialog dlg(   this, _( "Save Board File As" ), pcbFileName.GetPath(),
                            pcbFileName.GetFullName(),
                            wildcard, wxFD_SAVE
                            /* wxFileDialog is not equipped to handle multiple wildcards and
                                wxFD_OVERWRITE_PROMPT both together.
                                | wxFD_OVERWRITE_PROMPT
                            */
                            );

        if( dlg.ShowModal() != wxID_OK )
            return false;

        int filterNdx = dlg.GetFilterIndex();

        pluginType = ( filterNdx == 1 ) ? IO_MGR::LEGACY : IO_MGR::KICAD;

        // Note: on Linux wxFileDialog is not reliable for noticing a changed filename.
        // We probably need to file a bug report or implement our own derivation.
        pcbFileName = dlg.GetPath();

        // enforce file extension, must match plugin's policy.
        pcbFileName.SetExt( IO_MGR::GetFileExtension( pluginType ) );

        // Since the file overwrite test was removed from wxFileDialog because it doesn't work
        // when multiple wildcards are defined, we have to check it ourselves to prevent an
        // existing board file from silently being over written.
        if( pcbFileName.FileExists()
          && !IsOK( this, wxString::Format( _( "The file '%s' already exists.\n\nDo you want "
                                               "to overwrite it?" ),
                                            GetChars( pcbFileName.GetFullPath() ) )) )
            return false;

        // Save the project specific footprint library table.
        if( !Prj().PcbFootprintLibs()->IsEmpty( false ) )
        {
            wxString fp_lib_tbl = Prj().FootprintLibTblName();

            if( wxFileName::FileExists( fp_lib_tbl )
              && IsOK( this, _( "A footprint library table already exists in this path.\n\nDo "
                                "you want to overwrite it?" ) ) )
            {
                try
                {
                    Prj().PcbFootprintLibs()->Save( fp_lib_tbl );
                }
                catch( const IO_ERROR& ioe )
                {
                    wxString msg = wxString::Format( _(
                        "An error occurred attempting to save the "
                        "footprint library table '%s'\n\n%s" ),
                        GetChars( fp_lib_tbl ),
                        GetChars( ioe.errorText )
                        );
                    DisplayError( this, msg );
                }
            }
        }
    }
    else
    {
        pcbFileName = aFileName;

        if( pcbFileName.GetExt() == LegacyPcbFileExtension )
            pluginType = IO_MGR::LEGACY;
        else
        {
            pluginType = IO_MGR::KICAD;
            pcbFileName.SetExt( KiCadPcbFileExtension );
        }
    }

    if( !IsWritable( pcbFileName ) )
        return false;

    if( aCreateBackupFile )
    {
        // Get the backup file name
        backupFileName = pcbFileName;
        backupFileName.SetExt( pcbFileName.GetExt() + backupSuffix );

        // If an old backup file exists, delete it.  If an old board file exists, rename
        // it to the backup file name.
        if( pcbFileName.FileExists() )
        {
            // Remove the old file xxx.000 if it exists.
            if( backupFileName.FileExists() )
                wxRemoveFile( backupFileName.GetFullPath() );

            // Rename the "old" file" from xxx.kicad_pcb to xxx.000
            if( !wxRenameFile( pcbFileName.GetFullPath(), backupFileName.GetFullPath() ) )
            {
                msg = _( "Warning: unable to create backup file " ) + backupFileName.GetFullPath();
                DisplayError( this, msg );
                saveok = false;
            }
        }
        else
        {
            backupFileName.Clear();
        }
    }

    GetBoard()->m_Status_Pcb &= ~CONNEXION_OK;

    GetBoard()->SynchronizeNetsAndNetClasses();

    // Select default Netclass before writing file.
    // Useful to save default values in headers
    GetDesignSettings().SetCurrentNetClass( NETCLASS::Default );

    try
    {
        PLUGIN::RELEASER    pi( IO_MGR::PluginFind( pluginType ) );

        /*
        if( (PLUGIN*)pi == NULL )
            THROW_IO_ERROR( wxString::Format( _( "cannot find file plug in for file format '%s'" ),
                                              GetChars( pcbFileName.GetExt() ) ) );
        */

        pi->Save( pcbFileName.GetFullPath(), GetBoard(), NULL );
    }
    catch( const IO_ERROR& ioe )
    {
        wxString msg = wxString::Format( _( "Error saving board.\n%s" ),
                                         ioe.errorText.GetData() );
        wxMessageBox( msg, _( "Save Board File" ), wxICON_ERROR | wxOK );
        saveok = false;
    }

    if( saveok )
    {
        GetBoard()->SetFileName( pcbFileName.GetFullPath() );
        UpdateTitle();

        // Put the saved file in File History, unless aCreateBackupFile
        // is false.
        // aCreateBackupFile == false is mainly used to write autosave files
        // and not need to have an autosave file in file history
        if( aCreateBackupFile )
            UpdateFileHistory( GetBoard()->GetFileName() );

        // It's possible that the save as wrote over an existing board file that was part of a
        // project so attempt reload the projects settings.
        if( isSaveAs )
            LoadProjectSettings( pcbFileName.GetFullPath() );
    }

    // Display the file names:
    m_messagePanel->EraseMsgBox();

    if( saveok )
    {
        // Delete auto save file on successful save.
        wxFileName autoSaveFileName = pcbFileName;

        autoSaveFileName.SetName( wxString( autosavePrefix ) + pcbFileName.GetName() );

        if( autoSaveFileName.FileExists() )
            wxRemoveFile( autoSaveFileName.GetFullPath() );

        upperTxt = _( "Backup file: " ) + backupFileName.GetFullPath();
    }

    if( saveok )
        lowerTxt = _( "Wrote board file: " );
    else
        lowerTxt = _( "Failed to create " );

    lowerTxt += pcbFileName.GetFullPath();

    ClearMsgPanel();
    AppendMsgPanel( upperTxt, lowerTxt, CYAN );

    GetScreen()->ClrSave();
    GetScreen()->ClrModify();
    return true;
}


bool PCB_EDIT_FRAME::doAutoSave()
{
    wxFileName tmpFileName = GetBoard()->GetFileName();
    wxFileName fn = tmpFileName;

    // Auto save file name is the normal file name prepended with
    // autosaveFilePrefix string.
    fn.SetName( wxString( autosavePrefix ) + fn.GetName() );

    wxLogTrace( traceAutoSave,
                wxT( "Creating auto save file <" + fn.GetFullPath() ) + wxT( ">" ) );

    if( SavePcbFile( fn.GetFullPath(), NO_BACKUP_FILE ) )
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
