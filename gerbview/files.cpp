/*************/
/* files.cpp */
/*************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "gestfich.h"

#include "gerbview.h"

static void LoadDCodeFile( WinEDA_GerberFrame* frame,
                           const wxString&     FullFileName );


/* Load a Gerber file selected from history list on current layer
 * Previous data is deleted
 */
void WinEDA_GerberFrame::OnFileHistory( wxCommandEvent& event )
{
    wxString fn;

    fn = GetFileFromHistory( event.GetId(), _( "Printed circuit board" ) );

    if( fn != wxEmptyString )
    {
        Erase_Current_Layer( false );
        LoadGerberFiles( fn );
    }
}


/* File commands. */
void WinEDA_GerberFrame::Files_io( wxCommandEvent& event )
{
    int        id = event.GetId();

    switch( id )
    {
    case wxID_FILE:
    {
        Erase_Current_Layer( false );
        LoadGerberFiles( wxEmptyString );
        break;
    }

    case ID_MENU_INC_LAYER_AND_APPEND_FILE:
    case ID_INC_LAYER_AND_APPEND_FILE:
    {
        int origLayer = getActiveLayer();
        if( origLayer < NB_LAYERS )
        {
            setActiveLayer(origLayer+1);
            Erase_Current_Layer( false );
            if( !LoadGerberFiles( wxEmptyString ) )
                setActiveLayer(origLayer);
            SetToolbars();
        }
        else
        {
            wxString msg;
            msg.Printf( _( "GerbView only supports a maximum of %d layers. You must first \
delete an existing layer to load any new layers." ), NB_LAYERS );
            wxMessageBox( msg );
        }
    }
    break;

    case ID_NEW_BOARD:
        Clear_Pcb( true );
        Zoom_Automatique( false );
        DrawPanel->Refresh();
        ClearMsgPanel();
        break;

    case ID_GERBVIEW_LOAD_DRILL_FILE:
        DisplayError( this, _( "Not yet available..." ) );
        break;

    case ID_GERBVIEW_LOAD_DCODE_FILE:
        LoadDCodeFile( this, wxEmptyString );
        break;

    default:
        DisplayError( this, wxT( "File_io Internal Error" ) );
        break;
    }
}


bool WinEDA_GerberFrame::LoadGerberFiles( const wxString& aFullFileName )
{
    wxString   filetypes;
    wxArrayString filenamesList;
    wxFileName filename = aFullFileName;
    wxString currentPath;

    if( ! filename.IsOk() )
    {
        /* Standard gerber filetypes
         * (See http://en.wikipedia.org/wiki/Gerber_File)
         * the .pho extension is the default used in Pcbnew
         * However there are a lot of other extensions used for gerber files
         * Because the first letter is usually g, we accept g* as extension
         * (Mainly internal copper layers do not have specific extention,
         *  and filenames are like *.g1, *.g2 *.gb1 ...).
         */
        filetypes = _( "Gerber files (.g* .lgr .pho)" );
        filetypes << wxT("|");
        filetypes += wxT("*.g*;*.G*;*.lgr;*.LGR;*.pho;*.PHO" );
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

        /* All filetypes */
        filetypes += AllFilesWildcard;

        /* Use the current working directory if the file name path does not exist. */
        if( filename.DirExists() )
            currentPath = filename.GetPath();
        else
            currentPath = wxGetCwd();

        wxFileDialog dlg( this,
                          _( "Open Gerber File" ),
                          currentPath,
                          filename.GetFullName(),
                          filetypes,
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE | wxFD_CHANGE_DIR );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        dlg.GetFilenames( filenamesList );
        currentPath = wxGetCwd();
    }
    else
    {
        wxFileName filename = aFullFileName;
        filenamesList.Add( aFullFileName );
        currentPath = filename.GetPath();
    }

    // Read gerber files: each file is loaded on a new gerbview layer
    int layer = getActiveLayer();

    for( unsigned ii = 0; ii < filenamesList.GetCount(); ii++ )
    {
        wxFileName filename = filenamesList[ii];
        filename.SetPath( currentPath );
        GetScreen()->SetFileName( filename.GetFullPath() );
        filename.SetExt( g_PenFilenameExt );

        setActiveLayer( layer, false );

        if( Read_GERBER_File( GetScreen()->GetFileName(), filename.GetFullPath() ) )
        {
            SetLastProject( GetScreen()->GetFileName() );
            layer++;
            if( layer >= NB_LAYERS )
                layer = 0;
        }
    }

    Zoom_Automatique( true );
    GetScreen()->SetRefreshReq();
    g_SaveTime = time( NULL );

    // Synchronize layers tools with actual active layer:
    setActiveLayer(getActiveLayer());
    syncLayerBox();

    return true;
}


/*
 * Read a DCode file (not used with RX274X files , just with RS274D old files).
 * Note: there is no standard for DCode file.
 * Just read a file format created by early versions of Pcbnew.
 * Returns:
 *   0 if file not read (cancellation of order ...)
 *   1 if OK
 */
static void LoadDCodeFile( WinEDA_GerberFrame* frame, const wxString& FullFileName )
{
    wxString   wildcard;
    wxFileName fn = FullFileName;

    if( !fn.IsOk() )
    {
        wildcard.Printf( _( "Gerber DCODE files (%s)|*.%s" ),
                         GetChars( g_PenFilenameExt ),
                         GetChars( g_PenFilenameExt ) );
        wildcard += AllFilesWildcard;
        fn = frame->GetScreen()->GetFileName();
        fn.SetExt( g_PenFilenameExt );
        wxFileDialog dlg( (wxWindow*) frame, _( "Load GERBER DCODE File" ),
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
