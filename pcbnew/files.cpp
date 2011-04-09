/***************************************/
/* files.cpp: read / write board files */
/***************************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "kicad_string.h"
#include "gestfich.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "protos.h"
#include "pcbnew_id.h"
#include "3d_viewer.h"
#include "richio.h"
#include "filter_reader.h"

#define BACKUP_FILE_EXT wxT( "000" )


void PCB_EDIT_FRAME::OnFileHistory( wxCommandEvent& event )
{
    wxString fn;

    fn = GetFileFromHistory( event.GetId(), _( "Printed circuit board" ) );

    if( fn != wxEmptyString )
    {
        DrawPanel->EndMouseCapture( ID_NO_TOOL_SELECTED, DrawPanel->GetDefaultCursor() );
        ::wxSetWorkingDirectory( ::wxPathOnly( fn ) );
        LoadOnePcbFile( fn );
    }
}


/* Handle the read/write file commands
 */
void PCB_EDIT_FRAME::Files_io( wxCommandEvent& event )
{
    int        id = event.GetId();
    wxString   msg;

    // If an edition is in progress, stop it.
    // For something else than save, get rid of current tool.
    if( id == ID_SAVE_BOARD )
        DrawPanel->EndMouseCapture( -1, DrawPanel->GetDefaultCursor() );
    else
        DrawPanel->EndMouseCapture( ID_NO_TOOL_SELECTED, DrawPanel->GetDefaultCursor() );

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
            fn = wxFileName( wxEmptyString, g_SaveFileName, PcbFileExtension );
        }
        else
        {
            fn = GetScreen()->GetFileName();
            fn.SetExt( BACKUP_FILE_EXT );
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
        SetTitle( GetScreen()->GetFileName() );
        break;
    }

    case ID_APPEND_FILE:
        LoadOnePcbFile( wxEmptyString, true );
        break;

    case ID_NEW_BOARD:
        Clear_Pcb( true );
        GetScreen()->GetFileName().Printf( wxT( "%s%cnoname%s" ),
                                           GetChars( wxGetCwd() ), DIR_SEP,
                                           GetChars( PcbFileExtension ) );
        SetTitle( GetScreen()->GetFileName() );
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


/**
 * Function LoadOnePcbFile
 *  Load a Kicad board (.brd) file.
 *
 *  @param aFileName - File name including path. If empty, a file dialog will
 *                     be displayed.
 *  @param aAppend - Append board file aFileName to the currently loaded file if true.
 *                   Default = false.
 *  @param aForceFileDialog - Display the file open dialog even if aFullFileName is
 *                            valid if true; Default = false.
 *
 *  @return False if file load fails or is cancelled by the user, otherwise true.
 */
bool PCB_EDIT_FRAME::LoadOnePcbFile( const wxString& aFileName, bool aAppend,
                                     bool aForceFileDialog )
{
    FILE*    source;
    wxString msg;

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

    wxFileName fileName = aFileName;

    if( !fileName.IsOk() || !fileName.FileExists() || aForceFileDialog )
    {
        wxString name;
        wxString path = wxGetCwd();

        if( aForceFileDialog && fileName.FileExists() )
        {
            path = fileName.GetPath();
            name = fileName.GetFullName();
        }

        wxFileDialog dlg( this, _( "Open Board File" ), path, name, PcbFileWildcard,
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        fileName = dlg.GetPath();

        if( !fileName.HasExt() )
            fileName.SetExt( PcbFileExtension );
    }

    if( !aAppend )
        Clear_Pcb( false );     // pass false since we prompted above for a modified board

    GetScreen()->SetFileName( fileName.GetFullPath() );

    /* Start read PCB file
    */

    source = wxFopen( GetScreen()->GetFileName(), wxT( "rt" ) );
    if( source == NULL )
    {
        msg.Printf( _( "File <%s> not found" ), GetChars( GetScreen()->GetFileName() ) );
        DisplayError( this, msg );
        return false;
    }

    FILE_LINE_READER fileReader( source, GetScreen()->GetFileName() );

    FILTER_READER reader( fileReader );

    /* Read header and TEST if it is a PCB file format */
    reader.ReadLine();
    if( strncmp( reader.Line(), "PCBNEW-BOARD", 12 ) != 0 )
    {
        DisplayError( this, wxT( "Unknown file type" ) );
        return false;
    }

    int ver;
    sscanf( reader.Line() , "PCBNEW-BOARD Version %d date", &ver );
    if ( ver > g_CurrentVersionPCB )
    {
        DisplayInfoMessage( this, _( "This file was created by a more recent \
version of PCBnew and may not load correctly. Please consider updating!" ) );
    }
    else if ( ver < g_CurrentVersionPCB )
    {
        DisplayInfoMessage( this, _( "This file was created by an older \
version of PCBnew. It will be stored in the new file format when you save \
this file again." ) );
    }

    // Reload the corresponding configuration file:
    wxSetWorkingDirectory( wxPathOnly( GetScreen()->GetFileName() ) );

    if( aAppend )
        ReadPcbFile( &reader, true );
    else
    {
        // Update the option toolbar
        m_DisplayPcbTrackFill = DisplayOpt.DisplayPcbTrackFill;
        m_DisplayModText = DisplayOpt.DisplayModText;
        m_DisplayModEdge = DisplayOpt.DisplayModEdge;
        m_DisplayPadFill = DisplayOpt.DisplayPadFill;
        m_DisplayViaFill = DisplayOpt.DisplayViaFill;

        ReadPcbFile( &reader, false );
        LoadProjectSettings( GetScreen()->GetFileName() );
    }

    GetScreen()->ClrModify();

    /* If append option: change the initial board name to <oldname>-append.brd */
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

    SetTitle( GetScreen()->GetFileName() );
    UpdateFileHistory( GetScreen()->GetFileName() );

    /* Rebuild the new pad list (for drc and ratsnet control ...) */
    GetBoard()->m_Status_Pcb = 0;

    /* Reset the items visibility flag when loading a new config
     *  Because it could creates SERIOUS mistakes for the user,
     * if board items are not visible after loading a board...
     * Grid and ratsnest can be left to their previous state
     */
    bool showGrid = IsElementVisible( GRID_VISIBLE );
    bool showRats = IsElementVisible( RATSNEST_VISIBLE );
    SetVisibleAlls();
    SetElementVisibility( GRID_VISIBLE, showGrid );
    SetElementVisibility( RATSNEST_VISIBLE, showRats );

    // Update info shown by the horizontal toolbars
    GetBoard()->SetCurrentNetClass( NETCLASS::Default );
    ReFillLayerWidget();

    ReCreateLayerBox( NULL );
    syncLayerWidget();

    updateTraceWidthSelectBox();
    updateViaSizeSelectBox();

    // Display the loaded board:
    Zoom_Automatique( false );

    // Compile ratsnest and displays net info
    wxBusyCursor dummy;    // Displays an Hourglass while building connectivity
    Compile_Ratsnest( NULL, true );
    GetBoard()->DisplayInfo( this );

    /* reset the auto save timer */
    g_SaveTime = time( NULL );

    // Refresh the 3D view, if any
    if( m_Draw3DFrame )
        m_Draw3DFrame->NewDisplay();

#if 0 && defined(DEBUG)
    // note this freezes up pcbnew when run under the kicad project
    // manager.  runs fine from command prompt.  This is because the kicad
    // project manager redirects stdout of the child pcbnew process to itself,
    // but never reads from that pipe, and that in turn eventually blocks
    // the pcbnew program when the pipe it is writing to gets full.

    // Output the board object tree to stdout, but please run from command prompt:
    GetBoard()->Show( 0, std::cout );
#endif

    return true;
}


/* Write the board file
 */
bool PCB_EDIT_FRAME::SavePcbFile( const wxString& FileName )
{
    wxFileName  backupFileName;
    wxFileName  pcbFileName;
    wxString    upperTxt;
    wxString    lowerTxt;
    wxString    msg;

    bool        saveok = true;
    FILE*       dest;

    if( FileName == wxEmptyString )
    {
        wxFileDialog dlg( this, _( "Save Board File" ), wxEmptyString,
                          GetScreen()->GetFileName(), PcbFileWildcard,
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        GetScreen()->SetFileName( dlg.GetPath() );
    }
    else
        GetScreen()->SetFileName( FileName );

    /* If changes are made, update the board date */
    if( GetScreen()->IsModify() )
    {
        GetScreen()->m_Date = GenDate();
    }

    pcbFileName = GetScreen()->GetFileName();

    /* Get the backup file name */
    backupFileName = pcbFileName;
    backupFileName.SetExt( BACKUP_FILE_EXT );

    /* If an old backup file exists, delete it.
    if an old board file exists, rename it to the backup file name
    */
    if( pcbFileName.FileExists() )
    {
        /* rename the "old" file" from xxx.brd to xxx.000 */
        if( backupFileName.FileExists() )    /* Remove the old file xxx.000 (if exists) */
            wxRemoveFile( backupFileName.GetFullPath() );
        if( !wxRenameFile( pcbFileName.GetFullPath(), backupFileName.GetFullPath() ) )
        {
            msg = _( "Warning: unable to create backup file " ) + backupFileName.GetFullPath();
            DisplayError( this, msg, 15 );
            saveok = false;
        }
    }
    else
    {
        backupFileName.Clear();
        saveok = false;
    }

    /* Create the file */
    dest = wxFopen( pcbFileName.GetFullPath(), wxT( "wt" ) );

    if( dest == 0 )
    {
        msg = _( "Unable to create " ) + pcbFileName.GetFullPath();
        DisplayError( this, msg );
        saveok = false;
    }

    if( dest )
    {
        GetScreen()->SetFileName( pcbFileName.GetFullPath() );
        SetTitle( GetScreen()->GetFileName() );

        SavePcbFormatAscii( dest );
        fclose( dest );
    }

    /* Display the file names: */
    MsgPanel->EraseMsgBox();

    if( saveok )
    {
        upperTxt = _( "Backup file: " ) + backupFileName.GetFullPath();
    }

    if( dest )
        lowerTxt = _( "Wrote board file: " );
    else
        lowerTxt = _( "Failed to create " );
    lowerTxt += pcbFileName.GetFullPath();

    ClearMsgPanel();
    AppendMsgPanel( upperTxt, lowerTxt, CYAN );

    g_SaveTime = time( NULL );    /* Reset timer for the automatic saving */

    GetScreen()->ClrModify();
    return true;
}
