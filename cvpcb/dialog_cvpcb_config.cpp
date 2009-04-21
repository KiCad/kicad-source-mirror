/////////////////////////////////////////////////////////////////////////////

// Name:        dialog_cvpcb_config.cpp
// Author:      jean-pierre Charras
// Licence:     gpl
/////////////////////////////////////////////////////////////////////////////

#include "fctsys.h"
#include <wx/tokenzr.h>
#include "appl_wxstruct.h"
#include "common.h"
#include "confirm.h"
#include "gestfich.h"

#include "cvpcb.h"
#include "cvstruct.h"

#include "protos.h"

#include "dialog_cvpcb_config.h"


DIALOG_CVPCB_CONFIG::DIALOG_CVPCB_CONFIG( WinEDA_CvpcbFrame* parent ) :
    DIALOG_CVPCB_CONFIG_FBP( parent )
{
    wxString title;

    m_Parent   = parent;
    m_Config = wxGetApp().m_EDA_CommonConfig;
 
    Init( );
    title = _( "from " ) + wxGetApp().m_CurrentOptionFile;
    SetTitle( title );
    if( GetSizer() )
    {
        GetSizer()->SetSizeHints( this );
    }
}


/*************************************/
void DIALOG_CVPCB_CONFIG::Init()
/*************************************/
{
    wxString msg;

    SetFont( *g_DialogFont );
    SetFocus();

    m_LibListChanged = false;
    m_LibPathChanged = false;
    m_UserLibDirBufferImg = g_UserLibDirBuffer;         // Save the original lib path

    // Display current files extension (info)
    msg = m_InfoCmpFileExt->GetLabel() + ComponentFileExtension;
    m_InfoCmpFileExt->SetLabel( msg );

    msg = m_InfoLibFileExt->GetLabel() + ModuleFileExtension;
    m_InfoLibFileExt->SetLabel( msg );

    msg = m_InfoNetlistFileExt->GetLabel() + g_NetlistFileExtension;
    m_InfoNetlistFileExt->SetLabel( msg );

    msg = m_InfoEquivFileExt->GetLabel() + EquivFileExtension;
    m_InfoEquivFileExt->SetLabel( msg );

    msg = m_InfoRetroannotFileExt->GetLabel() + RetroFileExtension;
    m_InfoRetroannotFileExt->SetLabel( msg );

    m_ListLibr->InsertItems( g_LibName_List, 0 );
    m_ListEquiv->InsertItems( g_ListName_Equ, 0 );

    // Display current modules doc file:
    m_Config->Read( DOC_FOOTPRINTS_LIST_KEY, g_DocModulesFileName );
    m_TextHelpModulesFileName->SetValue( g_DocModulesFileName );

    // Load user libs paths:
    wxStringTokenizer Token( m_UserLibDirBufferImg, wxT( ";\n\r" ) );
    while( Token.HasMoreTokens() )
    {
        wxString path = Token.GetNextToken();
        if( wxFileName::DirExists( path ) )
            m_listUserPaths->Append( path );
    }

    // Display actual libraries paths:
    wxPathList libpaths = wxGetApp().GetLibraryPathList();
    for( unsigned ii = 0; ii < libpaths.GetCount(); ii++ )
    {
        m_DefaultLibraryPathslistBox->Append( libpaths[ii] );
    }

    // select the first path afer the current path project
    if( libpaths.GetCount() > 1 )
        m_DefaultLibraryPathslistBox->Select( 1 );
    
    m_radioBoxCloseOpt->SetSelection ( g_KeepCvpcbOpen ? 1 : 0 );
}


/******************************************************************/
void DIALOG_CVPCB_CONFIG::OnCancelClick( wxCommandEvent& event )
/******************************************************************/
{
    // Recreate the user lib path
    if( m_LibPathChanged )
    {
        for( unsigned ii = 0; ii < m_ListLibr->GetCount(); ii++ )
            wxGetApp().RemoveLibraryPath( m_listUserPaths->GetString( ii ) );

        wxGetApp().InsertLibraryPath( g_UserLibDirBuffer, 1 );
    }
    EndModal( -1 );
}


/**************************************************************/
void DIALOG_CVPCB_CONFIG::OnOkClick( wxCommandEvent& event )
/**************************************************************/
{
    g_KeepCvpcbOpen = m_radioBoxCloseOpt->GetSelection( ) ? true : false;
    m_Config->Write( CLOSE_OPTION_KEY, (long) g_KeepCvpcbOpen );

    m_Config->Write( DOC_FOOTPRINTS_LIST_KEY,
                    m_TextHelpModulesFileName->GetValue() );

    // Recreate the user lib path
    if( m_LibPathChanged )
    {
        g_UserLibDirBuffer.Empty();
        for( unsigned ii = 0; ii < m_listUserPaths->GetCount(); ii++ )
        {
            if( ii > 0 )
                g_UserLibDirBuffer << wxT( ";" );
            g_UserLibDirBuffer << m_listUserPaths->GetString( ii );
        }
    }


    // Set new active library list if the lib list of if default path list was modified
    if( m_LibListChanged || m_LibPathChanged )
    {
        // Recreate lib list
        g_LibName_List.Clear();
        for( unsigned ii = 0; ii < m_ListLibr->GetCount(); ii++ )
            g_LibName_List.Add( m_ListLibr->GetString( ii ) );

        // Recreate equ list
        g_ListName_Equ.Clear();
        for( unsigned ii = 0; ii < m_ListEquiv->GetCount(); ii++ )
            g_ListName_Equ.Add( m_ListEquiv->GetString( ii ) );

        listlib();
        m_Parent->BuildFootprintListBox();
    }
    if( event.GetId() != ID_SAVE_CFG )
        EndModal( 0 );
}


/**************************************************************/
void DIALOG_CVPCB_CONFIG::OnCloseWindow( wxCloseEvent& event )
/**************************************************************/
{
    EndModal( 0 );
}


/*********************************************************************/
void DIALOG_CVPCB_CONFIG::OnRemoveLibClick( wxCommandEvent& event )
/*********************************************************************/

/* Remove a library to the library list.
 * The real list (g_LibName_List) is not changed, so the change can be cancelled
 */
{
    int        ii;

    wxListBox * list = m_ListEquiv;

    if( event.GetId() == ID_REMOVE_LIB )
        list = m_ListLibr;

    ii = list->GetSelection();
    if( ii < 0 )
        return;

    list->Delete( ii );
    m_LibListChanged = TRUE;
}


/**************************************************************************/
void DIALOG_CVPCB_CONFIG::OnAddOrInsertLibClick( wxCommandEvent& event )
/**************************************************************************/

/* Insert or add a library to the library list:
 *   The new library is put in list before (insert button) the selection,
 *   or added (add button) to end of list
 * The real list (g_LibName_List) is not changed, so the change can be cancelled
 */
{
    int        ii;
    wxString   libfilename, wildcard;
    wxFileName fn;

    bool       insert = false;

    if( (event.GetId() == ID_INSERT_EQU) || (event.GetId() == ID_INSERT_LIB) )
        insert = true;

    wildcard = EquivFileWildcard;
    wxListBox * list = m_ListEquiv;
    if( (event.GetId() == ID_ADD_LIB) || (event.GetId() == ID_INSERT_LIB) )
    {
        list = m_ListLibr;
        wildcard = g_FootprintLibFileWildcard;
    }

    ii = list->GetSelection();
    if( ii == wxNOT_FOUND )
        ii = 0;

    wxString libpath;
    libpath = m_DefaultLibraryPathslistBox->GetStringSelection();
    if( libpath.IsEmpty() )
        libpath = wxGetApp().ReturnLastVisitedLibraryPath();

    wxFileDialog FilesDialog( this, _( "Footprint library files:" ), libpath,
                              wxEmptyString, wildcard,
                              wxFD_DEFAULT_STYLE | wxFD_MULTIPLE );

    if( FilesDialog.ShowModal() != wxID_OK )
        return;

    wxArrayString Filenames;
    FilesDialog.GetPaths( Filenames );

    for( unsigned jj = 0; jj < Filenames.GetCount(); jj++ )
    {
        fn = Filenames[jj];
        if( jj == 0 )
            wxGetApp().SaveLastVisitedLibraryPath( fn.GetPath() );

        /* If the library path is already in the library search paths
         * list, just add the library name to the list.  Otherwise, add
         * the library name with the full or relative path.
         * the relative path, when possible is preferable,
         * because it preserve use of default libraries paths, when the path is a sub path of these default paths
         *
         */
        if( wxGetApp().GetLibraryPathList().Index( fn.GetPath() ) != wxNOT_FOUND )  // Ok, trivial case
            libfilename = fn.GetName();
        else                                                                        // not in the default, : see if this file is in a subpath:
        {
            libfilename = fn.GetPathWithSep() + fn.GetName();
            for( unsigned kk = 0; kk < wxGetApp().GetLibraryPathList().GetCount(); kk++ )
            {
                if( fn.MakeRelativeTo( wxGetApp().GetLibraryPathList()[kk] ) )
                {
                    libfilename = fn.GetPathWithSep() + fn.GetName();
                    break;
                }
            }
        }

        //Add or insert new library name, if not already in list
        if( list->FindString( libfilename, fn.IsCaseSensitive() ) == wxNOT_FOUND )
        {
            m_LibListChanged = TRUE;
            if( ! insert )
                list->Append( libfilename );
            else
                list->Insert( libfilename, ii++ );
        }
        else
        {
            wxString msg = wxT( "<" ) + libfilename + wxT( "> : " ) +
                           _( "Library already in use" );
            DisplayError( this, msg );
        }
    }
}


/*******************************************************************/
void DIALOG_CVPCB_CONFIG::OnSaveCfgClick( wxCommandEvent& event )
/*******************************************************************/
{
    OnOkClick( event );
    Save_Config( this, m_Parent->m_NetlistFileName.GetFullPath() );
}


/***********************************************************************/
void DIALOG_CVPCB_CONFIG::OnAddOrInsertPath( wxCommandEvent& event )
/***********************************************************************/
{
    wxString path = wxGetApp().ReturnLastVisitedLibraryPath();

    bool     select = EDA_DirectorySelector( _( "Default Path for Libraries" ),     /* Titre de la fenetre */
                                             path,                                  /* Chemin par defaut */
                                             wxDD_DEFAULT_STYLE,
                                             this,                                  /* parent frame */
                                             wxDefaultPosition );

    if( !select )
        return;

    if( !wxFileName::DirExists( path ) )     // Should not occurs
        return;

    // Add or insert path if not already in list
    if( m_listUserPaths->FindString( path ) == wxNOT_FOUND )
    {
        int ipos = m_listUserPaths->GetCount();
        if( event.GetId() == ID_INSERT_PATH )
        {
            if( ipos  )
                ipos--;
            int jj = m_listUserPaths->GetSelection();
            if( jj >= 0 )
                ipos = jj;
        }
        m_listUserPaths->Insert( path, ipos );
        m_LibPathChanged = true;
        wxGetApp().InsertLibraryPath( path, ipos + 1 );

        // Display actual libraries paths:
        wxPathList libpaths = wxGetApp().GetLibraryPathList();
        m_DefaultLibraryPathslistBox->Clear();
        for( unsigned ii = 0; ii < libpaths.GetCount(); ii++ )
        {
            m_DefaultLibraryPathslistBox->Append( libpaths[ii] );
        }
    }
    else
        DisplayError( this, _( "Path already in use" ) );

    wxGetApp().SaveLastVisitedLibraryPath( path );
}


/***********************************************************************/
void DIALOG_CVPCB_CONFIG::OnRemoveUserPath( wxCommandEvent& event )
/***********************************************************************/
{
    int ii = m_listUserPaths->GetSelection();

    if( ii < 0 )
        ii = m_listUserPaths->GetCount() - 1;
    if( ii >= 0 )
    {
        wxGetApp().RemoveLibraryPath( m_listUserPaths->GetStringSelection() );
        m_listUserPaths->Delete( ii );
        m_LibPathChanged = true;
    }

    // Display actual libraries paths:
    wxPathList libpaths = wxGetApp().GetLibraryPathList();
    m_DefaultLibraryPathslistBox->Clear();
    for( unsigned ii = 0; ii < libpaths.GetCount(); ii++ )
    {
        m_DefaultLibraryPathslistBox->Append( libpaths[ii] );
    }
}


/**************************************************************************/
void DIALOG_CVPCB_CONFIG::OnBrowseModDocFile( wxCommandEvent& event )
/**************************************************************************/
{
    wxString FullFileName, mask;
    wxString docpath, filename;

    docpath = wxGetApp().ReturnLastVisitedLibraryPath( wxT( "doc" ) );

    mask = wxT( "*.pdf" );

    wxFileDialog FilesDialog( this, _( "Footprint document file:" ), docpath,
                              wxEmptyString, mask,
                              wxFD_DEFAULT_STYLE  );

    if( FilesDialog.ShowModal() != wxID_OK )
        return;

    FullFileName = FilesDialog.GetPath();

    /* If the path is already in the library search paths
     * list, just add the library name to the list.  Otherwise, add
     * the library name with the full or relative path.
     * the relative path, when possible is preferable,
     * because it preserve use of default libraries paths, when the path is a sub path of these default paths
     */
    wxFileName fn = FullFileName;
    wxGetApp().SaveLastVisitedLibraryPath( fn.GetPath() );

    filename = wxGetApp().ReturnFilenameWithRelativePathInLibPath( FullFileName );
    m_TextHelpModulesFileName->SetValue( filename );
}
