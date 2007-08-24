/***************************************************/
/* Files.cpp: Lecture / Sauvegarde des fichiers PCB */
/***************************************************/

#include "fctsys.h"

#include "common.h"
#include "pcbnew.h"
#include "protos.h"
#include "id.h"


/****************************************************/
void WinEDA_PcbFrame::Files_io( wxCommandEvent& event )
/****************************************************/

/* Gestion generale  des commandes de lecture de fichiers
 */
{
    int        id = event.GetId();
    wxClientDC dc( DrawPanel );
    wxString   msg;

    DrawPanel->PrepareGraphicContext( &dc );

    // Arret des commandes en cours
    if( DrawPanel->ManageCurseur && DrawPanel->ForceCloseManageCurseur )
    {
        DrawPanel->ForceCloseManageCurseur( DrawPanel, &dc );
    }
    SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );

    switch( id )
    {
    case ID_MENU_LOAD_FILE:
    case ID_LOAD_FILE:
        Clear_Pcb( &dc, TRUE );
        LoadOnePcbFile( wxEmptyString, &dc, FALSE );
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
        Clear_Pcb( &dc, TRUE );
        LoadOnePcbFile( filename, &dc, FALSE );
        GetScreen()->m_FileName = oldfilename;
        SetTitle( GetScreen()->m_FileName );
        ReCreateAuxiliaryToolbar();
    }
        break;

    case ID_MENU_APPEND_FILE:
    case ID_APPEND_FILE:
        LoadOnePcbFile( wxEmptyString, &dc, TRUE );
        break;

    case ID_MENU_NEW_BOARD:
    case ID_NEW_BOARD:
        Clear_Pcb( &dc, TRUE );
        GetScreen()->m_FileName.Printf( wxT( "%s%cnoname%s" ),
                                       wxGetCwd().GetData(), DIR_SEP, PcbExtBuffer.GetData() );
        SetTitle( GetScreen()->m_FileName );
        break;

    case ID_LOAD_FILE_1:
    case ID_LOAD_FILE_2:
    case ID_LOAD_FILE_3:
    case ID_LOAD_FILE_4:
    case ID_LOAD_FILE_5:
    case ID_LOAD_FILE_6:
    case ID_LOAD_FILE_7:
    case ID_LOAD_FILE_8:
    case ID_LOAD_FILE_9:
    case ID_LOAD_FILE_10:
        Clear_Pcb( &dc, TRUE );
        wxSetWorkingDirectory( wxPathOnly( GetLastProject( id - ID_LOAD_FILE_1 ) ) );
        LoadOnePcbFile( GetLastProject( id - ID_LOAD_FILE_1 ).GetData(),
                        &dc, FALSE );
        ReCreateAuxiliaryToolbar();
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


/*****************************************************************************************/
int WinEDA_PcbFrame::LoadOnePcbFile( const wxString& FullFileName, wxDC* DC, bool Append )
/******************************************************************************************/

/*
 *  Lecture d'un fichier PCB, le nom etant dans PcbNameBuffer.s
 *  retourne:
 *  0 si fichier non lu ( annulation de commande ... )
 *  1 si OK
 */
{
    int      ii;
    FILE*    source;
    wxString msg;

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
        m_Pcb->m_Status_Pcb = 0;
    }

    if( FullFileName == wxEmptyString )
    {
        msg = wxT( "*" ) + PcbExtBuffer;
        wxString FileName =
            EDA_FileSelector( _( "Load board files:" ),
                              wxEmptyString,            /* Chemin par defaut */
                              GetScreen()->m_FileName,  /* nom fichier par defaut */
                              PcbExtBuffer,             /* extension par defaut */
                              msg,                      /* Masque d'affichage */
                              this,
                              wxFD_OPEN,
                              FALSE
            );
        if( FileName == wxEmptyString )
            return FALSE;
        GetScreen()->m_FileName = FileName;
    }
    else
        GetScreen()->m_FileName = FullFileName;

    /////////////////////////
    /* Lecture Fichier PCB */
    /////////////////////////

    source = wxFopen( GetScreen()->m_FileName, wxT( "rt" ) );
    if( source == NULL )
    {
        msg.Printf( _( "File <%s> not found" ), GetScreen()->m_FileName.GetData() );
        DisplayError( this, msg );
        return 0;
    }


    /* Lecture de l'entete et TEST si PCB format ASCII */
    GetLine( source, cbuf, &ii );
    if( strncmp( cbuf, "PCBNEW-BOARD", 12 ) != 0 )
    {
        fclose( source );
        DisplayError( this, wxT( "Unknown file type" ) );
        return 0;
    }

    SetTitle( GetScreen()->m_FileName );
    SetLastProject( GetScreen()->m_FileName );

    // Rechargement de la configuration:
    wxSetWorkingDirectory( wxPathOnly( GetScreen()->m_FileName ) );
    if( Append )
        ReadPcbFile( DC, source, TRUE );
    else
    {
        Read_Config( GetScreen()->m_FileName );

        // Mise a jour du toolbar d'options
        m_DisplayPcbTrackFill = DisplayOpt.DisplayPcbTrackFill;
        m_DisplayModText = DisplayOpt.DisplayModText;
        m_DisplayModEdge = DisplayOpt.DisplayModEdge;
        m_DisplayPadFill = DisplayOpt.DisplayPadFill;

        ReadPcbFile( DC, source, FALSE );
    }

    fclose( source );

    GetScreen()->ClrModify();

    if( Append )
    {
        GetScreen()->SetModify();
        GetScreen()->m_FileName.Printf( wxT( "%s%cnoname%s" ),
                  wxGetCwd().GetData(), DIR_SEP, PcbExtBuffer.GetData() );
    }

    /* liste des pads recalculee avec Affichage des messages d'erreur */
    build_liste_pads();

    m_Pcb->Display_Infos( this );

    g_SaveTime = time( NULL );

    
#if 0 && defined(DEBUG) 
    // note this seems to freeze up pcbnew when run under the kicad project 
    // manager.  runs fine from command prompt.
    // output the board object tree to stdout:
    m_Pcb->Show( 0, std::cout );
#endif
    
    return 1;
}


/***********************************************************/
bool WinEDA_PcbFrame::SavePcbFile( const wxString& FileName )
/************************************************************/

/* Sauvegarde du fichier PCB en format ASCII
 */
{
    wxString old_name, FullFileName, msg;
    bool     saveok = TRUE;
    FILE*    dest;

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

    /* mise a jour date si modifications */
    if( GetScreen()->IsModify() )
    {
        GetScreen()->m_Date = GenDate();
    }

    /* Calcul du nom du fichier a creer */
    FullFileName = MakeFileName( wxEmptyString, GetScreen()->m_FileName, PcbExtBuffer );

    /* Calcul du nom du fichier de sauvegarde */
    old_name = FullFileName;
    ChangeFileNameExt( old_name, wxT( ".000" ) );

    /* Changement du nom de l'ancien fichier s'il existe */
    if( wxFileExists( FullFileName ) )
    {
        /* conversion en *.000 de l'ancien fichier */
        wxRemoveFile( old_name ); /* S'il y a une ancienne sauvegarde */
        if( !wxRenameFile( FullFileName, old_name ) )
        {
            msg = _( "Warning: unable to create bakfile " ) + old_name;
            DisplayError( this, msg, 15 );
            saveok = FALSE;
        }
    }
    else
    {
        old_name = wxEmptyString; 
        saveok   = FALSE;
    }

    /* Sauvegarde de l'ancien fichier */
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

    /* Affichage des fichiers crees: */
    MsgPanel->EraseMsgBox();

    if( saveok )
    {
        msg = _( "Backup file: " ) + old_name;
        Affiche_1_Parametre( this, 1, msg, wxEmptyString, CYAN );
    }

    if( dest )
        msg = _( "Write Board file: " );
    else
        msg = _( "Failed to create " );
    msg += FullFileName;

    Affiche_1_Parametre( this, 1, wxEmptyString, msg, CYAN );
    g_SaveTime = time( NULL );    /* Reset delai pour sauvegarde automatique */
    GetScreen()->ClrModify();
    return TRUE;
}
