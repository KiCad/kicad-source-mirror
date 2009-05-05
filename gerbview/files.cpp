/******************************************************/
/* Files.cp: Lecture / Sauvegarde des fichiers gerber */
/******************************************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "gestfich.h"

#include "gerbview.h"
#include "pcbplot.h"
#include "protos.h"
#include "id.h"


/* Routines locales */
static void LoadDCodeFile( WinEDA_GerberFrame* frame, const wxString& FullFileName, wxDC* DC );


void WinEDA_GerberFrame::OnFileHistory( wxCommandEvent& event )
{
    wxString fn;

    fn = GetFileFromHistory( event.GetId(), _( "Printed circuit board" ) );

    if( fn != wxEmptyString && Clear_Pcb( true ) )
    {
        wxClientDC dc( DrawPanel );
        DrawPanel->CursorOff( &dc );
        LoadOneGerberFile( fn, &dc, false );
        DrawPanel->MouseToCursorSchema();
        DrawPanel->CursorOn( &dc );
    }
}

/***************
***************************************/
void WinEDA_GerberFrame::Files_io( wxCommandEvent& event )
/********************************************************/

/* Gestion generale  des commandes de lecture de fichiers
 */
{
    int id = event.GetId();

    wxClientDC dc( DrawPanel );

    DrawPanel->CursorOff( &dc );

    switch( id )
    {
    case ID_MENU_LOAD_FILE:
    case ID_LOAD_FILE:
        if( Clear_Pcb( TRUE ) )
        {
            LoadOneGerberFile( wxEmptyString, &dc, 0 );
        }
        break;

    case ID_MENU_INC_LAYER_AND_APPEND_FILE:
    case ID_INC_LAYER_AND_APPEND_FILE:
        {
            int origLayer = GetScreen()->m_Active_Layer;

            GetScreen()->m_Active_Layer++;

            if( !LoadOneGerberFile( wxEmptyString, &dc, 0 ) )
                GetScreen()->m_Active_Layer = origLayer;

            SetToolbars();
        }
        break;

    case ID_MENU_APPEND_FILE:
    case ID_APPEND_FILE:
        LoadOneGerberFile( wxEmptyString, &dc, 0 );
        break;

    case ID_MENU_NEW_BOARD:
    case ID_NEW_BOARD:
        Clear_Pcb( TRUE );
        Zoom_Automatique( FALSE );
        GetScreen()->SetRefreshReq();
        break;

    case ID_GERBVIEW_LOAD_DRILL_FILE:
        DisplayError( this, _( "Not yet available..." ) );
        break;

    case ID_GERBVIEW_LOAD_DCODE_FILE:
        LoadDCodeFile( this, wxEmptyString, &dc );
        break;

    case ID_SAVE_BOARD:
    case ID_MENU_SAVE_BOARD:
        SaveGerberFile( GetScreen()->m_FileName, &dc );
        break;

    case ID_MENU_SAVE_BOARD_AS:
        SaveGerberFile( wxEmptyString, &dc );
        break;

    default:
        DisplayError( this, wxT( "File_io Internal Error" ) );
        break;
    }

    DrawPanel->MouseToCursorSchema();
    DrawPanel->CursorOn( &dc );
}


/*******************************************************************************************/
bool WinEDA_GerberFrame::LoadOneGerberFile( const wxString& FullFileName,
                                            wxDC* DC, int mode )
/*******************************************************************************************/

/*
 *  Lecture d'un fichier PCB, le nom etant dans PcbNameBuffer.s
 *  retourne:
 *  0 si fichier non lu ( annulation de commande ... )
 *  1 si OK
 */
{
    wxString wildcard;
    wxFileName fn = FullFileName;

    ActiveScreen = GetScreen();

    if( !fn.IsOk() )
    {
        wildcard.Printf( _( "Gerber files (.%s .gbr .gbx .lgr .ger .pho)|"  \
                            "*.%s;*.gbr;*.gbx;*.lgr;*.ger;*.pho|" ),
                         g_PenFilenameExt.c_str(), g_PenFilenameExt.c_str());
        wildcard += AllFilesWildcard;

        wxString currpath = fn.GetPath();
        if ( currpath.IsEmpty() )
            currpath = wxGetCwd();
        wxFileDialog dlg( this, _( "Open Gerber File" ), currpath,
                          fn.GetFullName(), wildcard,
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        fn = dlg.GetPath();
    }

    GetScreen()->m_FileName = fn.GetFullPath();
    wxSetWorkingDirectory( fn.GetPath() );
    fn.SetExt( g_PenFilenameExt );

    if( Read_GERBER_File( DC, GetScreen()->m_FileName, fn.GetFullPath() ) )
        SetLastProject( GetScreen()->m_FileName );

    Zoom_Automatique( FALSE );
    GetScreen()->SetRefreshReq();
    g_SaveTime = time( NULL );

    return true;
}


/**********************************************************************************************/
static void LoadDCodeFile( WinEDA_GerberFrame* frame, const wxString& FullFileName, wxDC* DC )
/**********************************************************************************************/

/*
 *  Lecture d'un fichier PCB, le nom etant dans PcbNameBuffer.s
 *  retourne:
 *  0 si fichier non lu ( annulation de commande ... )
 *  1 si OK
 */
{
    wxString wildcard;
    wxFileName fn = FullFileName;

    ActiveScreen = frame->GetScreen();

    if( !fn.IsOk() )
    {
        wildcard.Printf( _( "Gerber DCODE files (%s)|*.%s" ),
                         g_PenFilenameExt.c_str(), g_PenFilenameExt.c_str());
        wildcard += AllFilesWildcard;
        fn = frame->GetScreen()->m_FileName;
        fn.SetExt( g_PenFilenameExt );
        wxFileDialog dlg( ( wxWindow* )frame, _( "Load GERBER DCODE File" ),
                          fn.GetPath(), fn.GetFullName(), wildcard,
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST );

        if( dlg.ShowModal() == wxID_CANCEL )
            return;

        fn = dlg.GetPath();
    }

    frame->Read_D_Code_File( fn.GetFullPath() );
    frame->CopyDCodesSizeToItems();
    frame->GetScreen()->SetRefreshReq();
}


/*******************************************************************************/
bool WinEDA_GerberFrame::SaveGerberFile( const wxString& FullFileName, wxDC* DC )
/*******************************************************************************/

/* Sauvegarde du fichier PCB en format ASCII
 */
{
    wxString wildcard;
    wxFileName fn = FullFileName;

    if( !fn.IsOk() )
    {
        fn = GetScreen()->m_FileName;

        wildcard.Printf( _( "Gerber DCODE files (%s)|*.%s" ),
                         g_PenFilenameExt.c_str(), g_PenFilenameExt.c_str());

        wxFileDialog dlg( this, _( "Save Gerber File" ), fn.GetPath(),
                          fn.GetFullName(), wildcard,
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        fn = dlg.GetPath();
    }

    GetScreen()->m_FileName = fn.GetFullPath();

// TODO

    return true;
}
