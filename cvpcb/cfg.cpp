/***************************************/
/** cfg.cpp : configuration de CVPCB  **/
/***************************************/

/* lit ou met a jour la configuration de CVPCB */

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "gestfich.h"
#include "cvpcb.h"
#include "protos.h"
#include "cvstruct.h"
#include "cfg.h"

/* Routines Locales */
/**/


/**************************************************/
void Read_Config( const wxString& FileName )
/**************************************************/

/* lit la configuration
 * 1 - lit cvpcb.cnf
 * 2 - si non trouve lit <chemin de cvpcb.exe>/cvpcb.cnf
 * 3 - si non trouve: init des variables aux valeurs par defaut
 *
 * Remarque:
 * le chemin de l'executable cvpcb.exe doit etre dans BinDir
 */
{
    wxFileName fn = FileName;

    /* Init des valeurs par defaut */
    g_LibName_List.Clear();
    g_ListName_Equ.Clear();

    if( fn.GetExt() != ProjectFileExtension )
        fn.SetExt( ProjectFileExtension );

    if( wxGetApp().GetLibraryPathList().Index( g_UserLibDirBuffer ) != wxNOT_FOUND )
    {
        wxLogDebug( wxT( "Removing path <%s> to library path search list." ),
                    g_UserLibDirBuffer.c_str() );
        wxGetApp().GetLibraryPathList().Remove( g_UserLibDirBuffer );
    }

    wxGetApp().ReadProjectConfig( fn.GetFullPath(),
                                  GROUP, ParamCfgList, FALSE );

    if( g_NetlistFileExtension.IsEmpty() )
        g_NetlistFileExtension = wxT( "net" );

    if( wxFileName::DirExists( g_UserLibDirBuffer )
        && wxGetApp().GetLibraryPathList().Index( g_UserLibDirBuffer ) == wxNOT_FOUND )
    {
        wxLogDebug( wxT( "Adding path <%s> to library path search list." ),
                    g_UserLibDirBuffer.c_str() );
        wxGetApp().GetLibraryPathList().Add( g_UserLibDirBuffer );
    }
}


/************************************************************/
void WinEDA_CvpcbFrame::Update_Config( wxCommandEvent& event )
/************************************************************/

/* fonction relai d'appel a Save_Config,
 * la vraie fonction de sauvegarde de la config
 */
{
    Save_Config( this, m_NetlistFileName.GetFullPath() );
}


void Save_Config( wxWindow* parent, const wxString& fileName )
{
    wxFileName fn = fileName;

    fn.SetExt( ProjectFileExtension );

    wxFileDialog dlg( parent, _( "Save Project File" ), fn.GetPath(),
                      fn.GetFullName(), ProjectFileWildcard, wxFD_SAVE );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    /* ecriture de la configuration */
    wxGetApp().WriteProjectConfig( dlg.GetPath(), GROUP, ParamCfgList );
}
