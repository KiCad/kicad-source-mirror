/***************************************/
/* files.cpp: read / write board files */
/***************************************/

#include "fctsys.h"

#include "common.h"
#include "pcbnew.h"
#include "protos.h"
#include "id.h"


void WinEDA_PcbFrame::OnFileHistory( wxCommandEvent& event )
{
    wxString fn;
    wxClientDC dc( DrawPanel );

    fn = GetFileFromHistory( event.GetId(), _( "Printed circuit board" ) );

    if( fn != wxEmptyString )
    {
        if( DrawPanel->ManageCurseur && DrawPanel->ForceCloseManageCurseur )
        {
            DrawPanel->PrepareGraphicContext( &dc );
            DrawPanel->ForceCloseManageCurseur( DrawPanel, &dc );
        }

        SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );
        ::wxSetWorkingDirectory( ::wxPathOnly( fn ) );
        LoadOnePcbFile( fn, false );
        ReCreateAuxiliaryToolbar();
        DrawPanel->MouseToCursorSchema();
        DrawPanel->CursorOn( &dc );
    }
}

/****************************************************/
void WinEDA_PcbFrame::Files_io( wxCommandEvent& event )
/****************************************************/

/* Handle the read/write file commands
 */
{
    int        id = event.GetId();
    wxString   msg;


    // If an edition is in progress, stop it
    if( DrawPanel->ManageCurseur && DrawPanel->ForceCloseManageCurseur )
    {
        wxClientDC dc( DrawPanel );
        DrawPanel->PrepareGraphicContext( &dc );
        DrawPanel->ForceCloseManageCurseur( DrawPanel, &dc );
    }
    SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );

    switch( id )
    {
    case ID_MENU_LOAD_FILE:
    case ID_LOAD_FILE:
        LoadOnePcbFile( wxEmptyString, false );
        ReCreateAuxiliaryToolbar();
        break;

    case ID_MENU_READ_LAST_SAVED_VERSION_BOARD:
    case ID_MENU_RECOVER_BOARD:
        {
            wxString filename, oldfilename = GetScreen()->m_FileName;
            if( id == ID_MENU_RECOVER_BOARD )
            {
                filename = g_SaveFileName + PcbExtBuffer;
            }
            else
            {
                filename = oldfilename;
                ChangeFileNameExt( filename, wxT( ".000" ) );
            }
            if( !wxFileExists( filename ) )
            {
                msg = _( "Recovery file " ) + filename + _( " not found" );
                DisplayInfo( this, msg );
                break;
            }
            else
            {
                msg = _( "Ok to load Recovery file " ) + filename;
                if( !IsOK( this, msg ) )
                    break;
            }
            LoadOnePcbFile( filename, false );
            GetScreen()->m_FileName = oldfilename;
            SetTitle( GetScreen()->m_FileName );
            ReCreateAuxiliaryToolbar();
        }
        break;

    case ID_MENU_APPEND_FILE:
    case ID_APPEND_FILE:
        LoadOnePcbFile( wxEmptyString, TRUE );
        break;

    case ID_MENU_NEW_BOARD:
    case ID_NEW_BOARD:
        Clear_Pcb( TRUE );
        GetScreen()->m_FileName.Printf( wxT( "%s%cnoname%s" ),
                                       wxGetCwd().GetData(), DIR_SEP, PcbExtBuffer.GetData() );
        SetTitle( GetScreen()->m_FileName );
        ReCreateLayerBox( NULL );
        break;

    case ID_SAVE_BOARD:
    case ID_MENU_SAVE_BOARD:
        SavePcbFile( GetScreen()->m_FileName );
        break;

    case ID_MENU_SAVE_BOARD_AS:
        SavePcbFile( wxEmptyString );
        break;

    case ID_PCB_GEN_CMP_FILE:
        RecreateCmpFileFromBoard();
        break;

    default:
        DisplayError( this, wxT( "File_io Internal Error" ) ); break;
    }
}


/*******************************************************************************/
int WinEDA_PcbFrame::LoadOnePcbFile( const wxString& FullFileName, bool Append )
/*******************************************************************************/

/**
 *  Read a board file
 *  @param FullFileName = file name. If empty, a file name will be asked
 *  @return 0 if fails or abort, 1 if OK
 */
{
    int      ii;
    FILE*    source;
    wxString msg;
    char cbuf[1024];

    ActiveScreen = GetScreen();

    if( GetScreen()->IsModify() &&  !Append )
    {
        if( !IsOK( this, _( "Board Modified: Continue ?" ) ) )
            return 0;
    }

    m_SelTrackWidthBox_Changed = TRUE;
    m_SelViaSizeBox_Changed    = TRUE;

    if( Append )
    {
        GetScreen()->m_FileName = wxEmptyString;
        GetScreen()->SetModify();
        GetBoard()->m_Status_Pcb = 0;
    }

    wxString fileName;

    if( FullFileName == wxEmptyString )
    {
        msg = wxT( "*" ) + PcbExtBuffer;
        fileName =
            EDA_FileSelector( _( "Load board files:" ),
                              wxEmptyString,            /* Chemin par defaut */
                              GetScreen()->m_FileName,  /* nom fichier par defaut */
                              PcbExtBuffer,             /* extension par defaut */
                              msg,                      /* Masque d'affichage */
                              this,
                              wxFD_OPEN,
                              FALSE
            );

        if( fileName == wxEmptyString )
            return FALSE;
    }
    else
        fileName = FullFileName;

    if( !Append )
        Clear_Pcb( false );     // pass false since we prompted above for a modified board

    GetScreen()->m_FileName = fileName;

    /* Start read PCB file
    */

    source = wxFopen( GetScreen()->m_FileName, wxT( "rt" ) );
    if( source == NULL )
    {
        msg.Printf( _( "File <%s> not found" ), GetScreen()->m_FileName.GetData() );
        DisplayError( this, msg );
        return 0;
    }


    /* Read header and TEST if it is a PCB file format */
    GetLine( source, cbuf, &ii );
    if( strncmp( cbuf, "PCBNEW-BOARD", 12 ) != 0 )
    {
        fclose( source );
        DisplayError( this, wxT( "Unknown file type" ) );
        return 0;
    }

    int ver;
    sscanf(cbuf, "PCBNEW-BOARD Version %d date", &ver );
    if ( ver > g_CurrentVersionPCB )
    {
        DisplayInfo( this, _( "This file was created by a more recent version of PCBnew and may not load correctly. Please consider updating!"));
    }
    else if ( ver < g_CurrentVersionPCB )
    {
        DisplayInfo( this, _( "This file was created by an older version of PCBnew. It will be stored in the new file format when you save this file again."));
    }

    // Reload the corresponding configuration file:
    wxSetWorkingDirectory( wxPathOnly( GetScreen()->m_FileName ) );
    if( Append )
        ReadPcbFile( source, true );
    else
    {
        Read_Config( GetScreen()->m_FileName );

        // Update the option toolbar
        m_DisplayPcbTrackFill = DisplayOpt.DisplayPcbTrackFill;
        m_DisplayModText = DisplayOpt.DisplayModText;
        m_DisplayModEdge = DisplayOpt.DisplayModEdge;
        m_DisplayPadFill = DisplayOpt.DisplayPadFill;

        ReadPcbFile( source, false );
    }

    fclose( source );

    GetScreen()->ClrModify();

    /* If append option: change the initial board name to <oldname>-append.brd */
    if( Append )
    {
        wxString new_filename = GetScreen()->m_FileName.BeforeLast('.');
        if ( ! new_filename.EndsWith(wxT("-append")) )
            new_filename += wxT("-append");

        new_filename += PcbExtBuffer;

        GetScreen()->SetModify();
        GetScreen()->m_FileName = new_filename;
    }

    GetScreen()->m_FileName.Replace( WIN_STRING_DIR_SEP, UNIX_STRING_DIR_SEP );

    SetTitle( GetScreen()->m_FileName );
    SetLastProject( GetScreen()->m_FileName );

    /* Rebuild the new pad list (for drc and ratsnet control ...) */
    build_liste_pads();

    GetBoard()->Display_Infos( this );
    DrawPanel->Refresh( true);

    /* reset the auto save timer */
    g_SaveTime = time( NULL );


#if 0 && defined(DEBUG)
    // note this freezes up pcbnew when run under the kicad project
    // manager.  runs fine from command prompt.  This is because the kicad
    // project manager redirects stdout of the child pcbnew process to itself,
    // but never reads from that pipe, and that in turn eventually blocks
    // the pcbnew program when the pipe it is writing to gets full.

    // Output the board object tree to stdout, but please run from command prompt:
    GetBoard()->Show( 0, std::cout );
#endif

    return 1;
}


/***********************************************************/
bool WinEDA_PcbFrame::SavePcbFile( const wxString& FileName )
/************************************************************/

/* Write the board file
 */
{
    wxString    BackupFileName;
    wxString    FullFileName;
    wxString    upperTxt;
    wxString    lowerTxt;
    wxString    msg;

    bool        saveok = TRUE;
    FILE*       dest;

    if( FileName == wxEmptyString )
    {
        msg = wxT( "*" ) + PcbExtBuffer;

        FullFileName = EDA_FileSelector( _( "Save board files:" ),
                                         wxEmptyString,             /* Chemin par defaut */
                                         GetScreen()->m_FileName,   /* nom fichier par defaut */
                                         PcbExtBuffer,              /* extension par defaut */
                                         msg,                       /* Masque d'affichage */
                                         this,
                                         wxFD_SAVE,
                                         FALSE
                       );
        if( FullFileName == wxEmptyString )
            return FALSE;

        GetScreen()->m_FileName = FullFileName;
    }
    else
        GetScreen()->m_FileName = FileName;

    /* If changes are made, update the board date */
    if( GetScreen()->IsModify() )
    {
        GetScreen()->m_Date = GenDate();
    }

    /* Get the filename */
    FullFileName = MakeFileName( wxEmptyString, GetScreen()->m_FileName, PcbExtBuffer );

    /* Get the backup file name */
    BackupFileName = FullFileName;
    ChangeFileNameExt( BackupFileName, wxT( ".000" ) );

    /* If an old backup file exists, delete it.
    if an old board file existes, rename it to the backup file name
    */
    if( wxFileExists( FullFileName ) )
    {
        /* rename the "old" file" from xxx.brd to xxx.000 */
        wxRemoveFile( BackupFileName ); /* Remove the old file xxx.000 (if exists) */
        if( !wxRenameFile( FullFileName, BackupFileName ) )
        {
            msg = _( "Warning: unable to create bakfile " ) + BackupFileName;
            DisplayError( this, msg, 15 );
            saveok = FALSE;
        }
    }
    else
    {
        BackupFileName = wxEmptyString;
        saveok   = FALSE;
    }

    /* Create the file */
    dest = wxFopen( FullFileName, wxT( "wt" ) );
    if( dest == 0 )
    {
        msg = _( "Unable to create " ) + FullFileName;
        DisplayError( this, msg );
        saveok = FALSE;
    }

    if( dest )
    {
        GetScreen()->m_FileName = FullFileName;
        SetTitle( GetScreen()->m_FileName );

        SavePcbFormatAscii( dest );
        fclose( dest );
    }

    /* Display the file names: */
    MsgPanel->EraseMsgBox();

    if( saveok )
    {
        upperTxt = _( "Backup file: " ) + BackupFileName;
    }

    if( dest )
        lowerTxt = _( "Wrote board file: " );
    else
        lowerTxt = _( "Failed to create " );
    lowerTxt += FullFileName;

    Affiche_1_Parametre( this, 1, upperTxt, lowerTxt, CYAN );

    g_SaveTime = time( NULL );    /* Reset timer for the automatic saving */

    GetScreen()->ClrModify();
    return TRUE;
}
