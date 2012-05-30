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
#include <appl_wxstruct.h>

#include <pcbnew.h>
#include <protos.h>
#include <pcbnew_id.h>
#include <io_mgr.h>
#include <wildcards_and_files_ext.h>

#include <class_board.h>
#include <build_version.h>      // LEGACY_BOARD_FILE_VERSION


static const wxString pcbBackupFileExtension(  wxT( "000" ) );
static wxString saveFileName( wxT( "$savepcb" ) );


void PCB_EDIT_FRAME::OnFileHistory( wxCommandEvent& event )
{
    wxString fn;

    fn = GetFileFromHistory( event.GetId(), _( "Printed circuit board" ) );

    if( fn != wxEmptyString )
    {
        m_canvas->EndMouseCapture( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor() );
        ::wxSetWorkingDirectory( ::wxPathOnly( fn ) );
        LoadOnePcbFile( fn );
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
        LoadOnePcbFile( GetScreen()->GetFileName(), false, true );
        break;

    case ID_MENU_READ_LAST_SAVED_VERSION_BOARD:
    case ID_MENU_RECOVER_BOARD:
        {
            wxFileName fn;

            if( id == ID_MENU_RECOVER_BOARD )
            {
                fn = wxFileName( wxEmptyString, saveFileName, PcbFileExtension );
            }
            else
            {
                fn = GetScreen()->GetFileName();
                fn.SetExt( pcbBackupFileExtension );
            }

            if( !fn.FileExists() )
            {
                msg = _( "Recovery file " ) + fn.GetFullPath() + _( " not found." );
                DisplayInfoMessage( this, msg );
                break;
            }
            else
            {
                msg = _( "OK to load recovery file " ) + fn.GetFullPath();

                if( !IsOK( this, msg ) )
                    break;
            }

            LoadOnePcbFile( fn.GetFullPath(), false );
            fn.SetExt( PcbFileExtension );
            GetScreen()->SetFileName( fn.GetFullPath() );
            UpdateTitle();
        }
        break;

    case ID_APPEND_FILE:
        LoadOnePcbFile( wxEmptyString, true );
        break;

    case ID_NEW_BOARD:
        Clear_Pcb( true );
        GetScreen()->GetFileName().Printf( wxT( "%s%cnoname%s" ),
                                           GetChars( wxGetCwd() ), DIR_SEP,
                                           GetChars( PcbFileExtension ) );
        UpdateTitle();
        ReCreateLayerBox( NULL );
        break;

    case ID_SAVE_BOARD:
        SavePcbFile( GetScreen()->GetFileName() );
        break;

    case ID_SAVE_BOARD_AS:
        SavePcbFile( wxEmptyString );
        break;

    default:
        DisplayError( this, wxT( "File_io Internal Error" ) ); break;
    }
}


bool PCB_EDIT_FRAME::LoadOnePcbFile( const wxString& aFileName, bool aAppend,
                                     bool aForceFileDialog )
{
    if( GetScreen()->IsModify() && !aAppend )
    {
        if( !IsOK( this, _( "The current board has been modified.  Do you wish to discard \
the changes?" ) ) )
            return false;
    }

    if( aAppend )
    {
        GetScreen()->SetFileName( wxEmptyString );
        OnModify();
        GetBoard()->m_Status_Pcb = 0;
    }

    wxFileName  fileName = aFileName;

    IO_MGR::PCB_FILE_T  pluginType = IO_MGR::LEGACY;

    // This is a subset of all PLUGINs which are trusted to be able to
    // load a BOARD.  Order is subject to change as KICAD plugin matures.
    // User may occasionally use the wrong plugin to load a *.brd file,
    // but eventually *.kicad_pcb will be more common than legacy *.brd files.
    static const struct {
        const wxString&     filter;
        IO_MGR::PCB_FILE_T  pluginType;
    } loaders[] = {
        { LegacyPcbFileWildcard,    IO_MGR::LEGACY },
        { PcbFileWildcard,          IO_MGR::KICAD },
        { EaglePcbFileWildcard,     IO_MGR::EAGLE },
    };

    if( !fileName.IsOk() || !fileName.FileExists() || aForceFileDialog )
    {
        wxString name;
        wxString path = wxGetCwd();
        wxString fileFilters;

        for( unsigned i=0;  i<DIM( loaders );  ++i )
        {
            if( i > 0 )
                fileFilters += wxChar( '|' );

            fileFilters += wxGetTranslation( loaders[i].filter );
        }

        if( aForceFileDialog && fileName.FileExists() )
        {
            path = fileName.GetPath();
            name = fileName.GetFullName();
        }

        wxFileDialog dlg( this, _( "Open Board File" ), path, name, fileFilters,
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        fileName = dlg.GetPath();

        int chosenFilter = dlg.GetFilterIndex();
        pluginType = loaders[chosenFilter].pluginType;
    }

    PLUGIN::RELEASER pi( IO_MGR::PluginFind( pluginType ) );

    if( !fileName.HasExt() )
        fileName.SetExt( pi->GetFileExtension() );

    if( !aAppend )
        Clear_Pcb( false );     // pass false since we prompted above for a modified board

    CheckForAutoSaveFile( fileName, pcbBackupFileExtension );

    GetScreen()->SetFileName( fileName.GetFullPath() );

    if( !aAppend )
    {
        // Update the option toolbar
        m_DisplayPcbTrackFill = DisplayOpt.DisplayPcbTrackFill;
        m_DisplayModText = DisplayOpt.DisplayModText;
        m_DisplayModEdge = DisplayOpt.DisplayModEdge;
        m_DisplayPadFill = DisplayOpt.DisplayPadFill;
        m_DisplayViaFill = DisplayOpt.DisplayViaFill;

        // load project settings before BOARD, in case BOARD file has overrides.
        LoadProjectSettings( GetScreen()->GetFileName() );
    }
    else
    {
        GetBoard()->m_NetClasses.Clear();
    }

    BOARD* loadedBoard = 0;   // it will be set to non-NULL if loadedOK

    try
    {
        // load or append either:
        loadedBoard = pi->Load( GetScreen()->GetFileName(), aAppend ? GetBoard() : NULL, NULL );

        if( !aAppend )
        {
            if( pluginType == IO_MGR::LEGACY &&
                loadedBoard->GetFileFormatVersionAtLoad() < LEGACY_BOARD_FILE_VERSION )
            {
                DisplayInfoMessage( this, _( "This file was created by an older \
version of Pcbnew. It will be stored in the new file format when you save \
this file again." ) );
            }

            SetBoard( loadedBoard );
        }
    }
    catch( IO_ERROR ioe )
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
    }

    GetScreen()->ClrModify();

    // If append option: change the initial board name to <oldname>-append.brd
    if( aAppend )
    {
        wxString new_filename = GetScreen()->GetFileName().BeforeLast( '.' );

        if ( ! new_filename.EndsWith( wxT( "-append" ) ) )
            new_filename += wxT( "-append" );

        new_filename += wxT( "." ) + PcbFileExtension;

        OnModify();
        GetScreen()->SetFileName( new_filename );
    }

    GetScreen()->GetFileName().Replace( WIN_STRING_DIR_SEP, UNIX_STRING_DIR_SEP );

    UpdateTitle();
    UpdateFileHistory( GetScreen()->GetFileName() );

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
    GetBoard()->SetCurrentNetClass( NETCLASS::Default );
    ReFillLayerWidget();

    ReCreateLayerBox( NULL );

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
    wxBusyCursor dummy;    // Displays an Hourglass while building connectivity
    Compile_Ratsnest( NULL, true );
    GetBoard()->DisplayInfo( this );

    // Refresh the 3D view, if any
    if( m_Draw3DFrame )
        m_Draw3DFrame->NewDisplay();

#if 0 && defined(DEBUG)
    // note this freezes up Pcbnew when run under the KiCad project
    // manager.  runs fine from command prompt.  This is because the KiCad
    // project manager redirects stdout of the child Pcbnew process to itself,
    // but never reads from that pipe, and that in turn eventually blocks
    // the Pcbnew program when the pipe it is writing to gets full.

    // Output the board object tree to stdout, but please run from command prompt:
    GetBoard()->Show( 0, std::cout );
#endif

    return true;
}


bool PCB_EDIT_FRAME::SavePcbFile( const wxString& aFileName, bool aCreateBackupFile )
{
    wxFileName  backupFileName;
    wxFileName  pcbFileName;
    wxString    upperTxt;
    wxString    lowerTxt;
    wxString    msg;
    wxString    wildcard;

    int         wildcardIndex = 0;
    bool        saveok = true;

    wildcard = LegacyPcbFileWildcard;

#if defined( USE_PCBNEW_SEXPR_FILE_FORMAT )
    wildcard += wxT( "|" ) + PcbFileWildcard;
#endif

    if( aFileName == wxEmptyString )
    {
        wxFileDialog dlg( this, _( "Save Board File" ), wxEmptyString, GetScreen()->GetFileName(),
                          wildcard, wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

        if( dlg.ShowModal() != wxID_OK )
            return false;

        GetScreen()->SetFileName( dlg.GetPath() );
        wildcardIndex = dlg.GetFilterIndex();         // Legacy or s-expression file format.
    }
    else
    {
        GetScreen()->SetFileName( aFileName );
    }

    // If changes are made, update the board date
    if( GetScreen()->IsModify() )
    {
        TITLE_BLOCK tb = GetTitleBlock();

        tb.SetDate();
        SetTitleBlock( tb );
    }

    pcbFileName = GetScreen()->GetFileName();

    if( pcbFileName.GetExt().IsEmpty() )
        pcbFileName.SetExt( IO_MGR::GetFileExtension( (IO_MGR::PCB_FILE_T) wildcardIndex ) );

    if( !IsWritable( pcbFileName ) )
        return false;

    if( aCreateBackupFile )
    {
        // Get the backup file name
        backupFileName = pcbFileName;
        backupFileName.SetExt( pcbBackupFileExtension );

        // If an old backup file exists, delete it.  If an old board file exists, rename
        // it to the backup file name.
        if( pcbFileName.FileExists() )
        {
            // Remove the old file xxx.000 if it exists.
            if( backupFileName.FileExists() )
                wxRemoveFile( backupFileName.GetFullPath() );

            // Rename the "old" file" from xxx.brd to xxx.000
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
            saveok = false;
        }
    }

    GetBoard()->m_Status_Pcb &= ~CONNEXION_OK;

    GetBoard()->SynchronizeNetsAndNetClasses();

    // Select default Netclass before writing file.
    // Useful to save default values in headers
    GetBoard()->SetCurrentNetClass( GetBoard()->m_NetClasses.GetDefault()->GetName() );

    try
    {
        PROPERTIES props;
        PLUGIN* plugin = IO_MGR::PluginFind( ( IO_MGR::PCB_FILE_T ) wildcardIndex );

        if( plugin == NULL )
            THROW_IO_ERROR( wxString::Format( _( "cannot find file plug in for file format '%s'" ),
                                              GetChars( pcbFileName.GetExt() ) ) );

        wxString header = wxString::Format(
            wxT( "PCBNEW-BOARD Version %d date %s\n\n# Created by Pcbnew%s\n\n" ),
            LEGACY_BOARD_FILE_VERSION, DateAndTime().GetData(),
            GetBuildVersion().GetData() );

        props["header"] = header;

        plugin->Save( pcbFileName.GetFullPath(), GetBoard(), &props );
    }
    catch( IO_ERROR ioe )
    {
        wxString msg = wxString::Format( _( "Error saving board.\n%s" ),
                                         ioe.errorText.GetData() );
        wxMessageBox( msg, _( "Save Board File" ), wxICON_ERROR | wxOK );
        saveok = false;
    }

    if( saveok )
    {
        GetScreen()->SetFileName( pcbFileName.GetFullPath() );
        UpdateTitle();
    }

    // Display the file names:
    m_messagePanel->EraseMsgBox();

    if( saveok )
    {
        // Delete auto save file on successful save.
        wxFileName autoSaveFileName = pcbFileName;
        autoSaveFileName.SetName( wxT( "$" ) + pcbFileName.GetName() );

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
    wxFileName tmpFileName = GetScreen()->GetFileName();
    wxFileName fn = tmpFileName;

    // Auto save file name is the normal file name prepended with $.
    fn.SetName( wxT( "$" ) + fn.GetName() );

    wxLogTrace( traceAutoSave,
                wxT( "Creating auto save file <" + fn.GetFullPath() ) + wxT( ">" ) );

    if( SavePcbFile( fn.GetFullPath(), NO_BACKUP_FILE ) )
    {
        GetScreen()->SetModify();
        GetScreen()->SetFileName( tmpFileName.GetFullPath() );
        UpdateTitle();
        m_autoSaveState = false;
        return true;
    }

    GetScreen()->SetFileName( tmpFileName.GetFullPath() );

    return false;
}
