/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_pcbnew_config_libs_and_paths.cpp
// Author:      jean-pierre Charras
// Created:     2009 apr 18
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

/* Handle the pcbnew library config (library list, and default lib path)
 */

#include "fctsys.h"
#include <wx/tokenzr.h>
#include "appl_wxstruct.h"
#include "common.h"
#include "confirm.h"
#include "gestfich.h"
#include "pcbnew.h"

#include "dialog_pcbnew_config_libs_and_paths.h"


/*****************************************************************/
void WinEDA_PcbFrame::InstallConfigFrame( const wxPoint& pos )
/*****************************************************************/
{
    DIALOG_PCBNEW_CONFIG_LIBS dialog( this );
    dialog.ShowModal();
}


DIALOG_PCBNEW_CONFIG_LIBS::DIALOG_PCBNEW_CONFIG_LIBS( WinEDA_PcbFrame* parent ):
    DIALOG_PCBNEW_CONFIG_LIBS_FBP(parent)
{
    m_Parent = parent;
    m_Config = wxGetApp().m_EDA_CommonConfig;

    Init( );

    wxString title = _( "from " ) + wxGetApp().m_CurrentOptionFile;
    SetTitle( title );

    if( GetSizer() )
    GetSizer()->SetSizeHints( this );

}

/*************************************/
void DIALOG_PCBNEW_CONFIG_LIBS::Init()
/*************************************/
{
    SetFont( *g_DialogFont );
    SetFocus();

    m_LibListChanged = false;
    m_LibPathChanged = false;
    m_UserLibDirBufferImg = g_UserLibDirBuffer;         // Save the original lib path

    // Display current files extension (info)
    wxString msg = m_InfoBoardFileExt->GetLabel() + PcbExtBuffer;
    m_InfoBoardFileExt->SetLabel( msg );

    msg = m_InfoCmpFileExt->GetLabel() + NetCmpExtBuffer;
    m_InfoCmpFileExt->SetLabel( msg );

    msg = m_InfoLibFileExt->GetLabel() + ModuleFileExtension;
    m_InfoLibFileExt->SetLabel( msg );

    msg = m_InfoNetlistFileExt->GetLabel() + NetExtBuffer;
    m_InfoNetlistFileExt->SetLabel( msg );

    m_ListLibr->InsertItems( g_LibName_List, 0 );

    // Display current modules doc file:
    m_Config->Read( wxT( "module_doc_file" ), g_DocModulesFileName );
    m_TextHelpModulesFileName->SetValue( g_DocModulesFileName );

    // Load user libs paths:
    wxStringTokenizer Token( m_UserLibDirBufferImg, wxT( ";\n\r" ) );
    while( Token.HasMoreTokens() )
    {
        wxString path = Token.GetNextToken();
        if( wxFileName::DirExists( path ) )
            m_listUserPaths->Append(path);
    }

    // Display actual libraries paths:
    wxPathList libpaths = wxGetApp().GetLibraryPathList();
    for( unsigned ii = 0; ii < libpaths.GetCount(); ii++ )
    {
        m_DefaultLibraryPathslistBox->Append( libpaths[ii]);
    }

    // select the first path afer the current path project
    if ( libpaths.GetCount() > 1 )
        m_DefaultLibraryPathslistBox->Select( 1 );
}


/******************************************************************/
void DIALOG_PCBNEW_CONFIG_LIBS::OnCancelClick( wxCommandEvent& event )
/******************************************************************/
{
    // Recreate the user lib path
    if ( m_LibPathChanged )
    {
        for ( unsigned ii = 0; ii < m_ListLibr->GetCount(); ii ++ )
            wxGetApp().RemoveLibraryPath( m_listUserPaths->GetString(ii)) ;
        wxGetApp().InsertLibraryPath( g_UserLibDirBuffer, 1);
    }
    EndModal( -1 );
}


/**************************************************************/
void DIALOG_PCBNEW_CONFIG_LIBS::OnOkClick( wxCommandEvent& event )
/**************************************************************/
{
     m_Config->Write( wxT( "module_doc_file" ),
                m_TextHelpModulesFileName->GetValue() );

    // Recreate the user lib path
    if ( m_LibPathChanged )
    {
        g_UserLibDirBuffer.Empty();
        for ( unsigned ii = 0; ii < m_listUserPaths->GetCount(); ii ++ )
        {
            if ( ii > 0 )
                g_UserLibDirBuffer << wxT(";");
            g_UserLibDirBuffer << m_listUserPaths->GetString(ii);
        }
    }


    // Set new active library list if the lib list of if default path list was modified
    if( m_LibListChanged || m_LibPathChanged )
    {
        // Recreate lib list
        g_LibName_List.Clear();
        for ( unsigned ii = 0; ii < m_ListLibr->GetCount(); ii ++ )
            g_LibName_List.Add(m_ListLibr->GetString(ii) );
    }
    if ( event.GetId() != ID_SAVE_CFG )
        EndModal( 0 );
}

/**************************************************************/
void DIALOG_PCBNEW_CONFIG_LIBS::OnCloseWindow( wxCloseEvent& event )
/**************************************************************/
{
    EndModal( 0 );
}



/*********************************************************************/
void DIALOG_PCBNEW_CONFIG_LIBS::OnRemoveLibClick( wxCommandEvent& event )
/*********************************************************************/
/* Remove a library to the library list.
 * The real list (g_LibName_List) is not changed, so the change can be cancelled
 */
{
    int ii;

    ii = m_ListLibr->GetSelection();
    if( ii < 0 )
        return;

    m_ListLibr->Delete(ii);
    m_LibListChanged = TRUE;
}


/**************************************************************************/
void DIALOG_PCBNEW_CONFIG_LIBS::OnAddOrInsertLibClick( wxCommandEvent& event )
/**************************************************************************/

/* Insert or add a library to the library list:
 *   The new library is put in list before (insert button) the selection,
 *   or added (add button) to end of list
 * The real list (g_LibName_List) is not changed, so the change can be cancelled
 */
{
    int        ii;
    wxString   libfilename;
    wxFileName fn;

    ii = m_ListLibr->GetSelection();
    if( ii == wxNOT_FOUND && event.GetId() != ID_ADD_LIB )
        ii = 0;

    wxString libpath;
    libpath = m_DefaultLibraryPathslistBox->GetStringSelection();
    if ( libpath.IsEmpty() )
        libpath = wxGetApp().ReturnLastVisitedLibraryPath();

    wxFileDialog FilesDialog( this, _( "Footprint library files:" ), libpath,
                              wxEmptyString, g_FootprintLibFileWildcard,
                              wxFD_DEFAULT_STYLE | wxFD_MULTIPLE );

    if( FilesDialog.ShowModal() != wxID_OK )
        return;

    wxArrayString Filenames;
    FilesDialog.GetPaths( Filenames );

     for( unsigned jj = 0; jj < Filenames.GetCount(); jj++ )
    {
        fn = Filenames[jj];
        if ( jj == 0 )
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
        else    // not in the default, : see if this file is in a subpath:
        {
            libfilename = fn.GetPathWithSep() + fn.GetName();
            for ( unsigned kk = 0; kk < wxGetApp().GetLibraryPathList().GetCount(); kk ++ )
            {
                if( fn.MakeRelativeTo(wxGetApp().GetLibraryPathList()[kk] ) )
                {
                    libfilename = fn.GetPathWithSep() + fn.GetName();
                    break;
                }
            }
        }

        //Add or insert new library name, if not already in list
        if( m_ListLibr->FindString( libfilename, fn.IsCaseSensitive() ) == wxNOT_FOUND )
        {
            m_LibListChanged = TRUE;
            if( event.GetId() == ID_ADD_LIB )
                m_ListLibr->Append( libfilename );
            else
                m_ListLibr->Insert( libfilename, ii++ );
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
void DIALOG_PCBNEW_CONFIG_LIBS::OnSaveCfgClick( wxCommandEvent& event )
/*******************************************************************/
{
    OnOkClick( event );
    m_Parent->Update_config( this );
}


/***********************************************************************/
void DIALOG_PCBNEW_CONFIG_LIBS::OnAddOrInsertPath( wxCommandEvent& event )
/***********************************************************************/
{
    wxString path = wxGetApp().ReturnLastVisitedLibraryPath();

    bool     select = EDA_DirectorySelector( _( "Default Path for Libraries" ),    /* Titre de la fenetre */
                                             path,                                  /* Chemin par defaut */
                                             wxDD_DEFAULT_STYLE,
                                             this,                                  /* parent frame */
                                             wxDefaultPosition );

    if( !select )
        return;

    if( ! wxFileName::DirExists( path ) )    // Should not occurs
        return;

    // Add or insert path if not already in list
    if( m_listUserPaths->FindString( path ) == wxNOT_FOUND )
    {
        int ipos = m_listUserPaths->GetCount();
        if ( event.GetId() == wxID_INSERT_PATH )
        {
            if ( ipos  ) ipos--;
            int jj = m_listUserPaths->GetSelection();
            if ( jj >= 0 )
                ipos = jj;
        }
        m_listUserPaths->Insert(path, ipos);
        m_LibPathChanged = true;
        wxGetApp().InsertLibraryPath( path, ipos+1 );

        // Display actual libraries paths:
        wxPathList libpaths = wxGetApp().GetLibraryPathList();
        m_DefaultLibraryPathslistBox->Clear();
        for( unsigned ii = 0; ii < libpaths.GetCount(); ii++ )
        {
            m_DefaultLibraryPathslistBox->Append( libpaths[ii]);
        }
    }

    else
        DisplayError(this, _("Path already in use") );

    wxGetApp().SaveLastVisitedLibraryPath( path );

}


/***********************************************************************/
void DIALOG_PCBNEW_CONFIG_LIBS::OnRemoveUserPath( wxCommandEvent& event )
/***********************************************************************/
{
    int ii = m_listUserPaths->GetSelection();
    if ( ii < 0 )
        ii = m_listUserPaths->GetCount()-1;
    if ( ii >= 0 )
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
        m_DefaultLibraryPathslistBox->Append( libpaths[ii]);
    }
}

/**************************************************************************/
void DIALOG_PCBNEW_CONFIG_LIBS::OnBrowseModDocFile( wxCommandEvent& event )
/**************************************************************************/
{
    wxString FullFileName, mask;
    wxString docpath, filename;

    docpath = wxGetApp().ReturnLastVisitedLibraryPath(wxT( "doc" ));

    mask = wxT( "*.pdf" );

    wxFileDialog FilesDialog( this, _( "Footprint document file:" ), docpath,
                              wxEmptyString, mask,
                              wxFD_DEFAULT_STYLE  );

    if( FilesDialog.ShowModal() != wxID_OK )
        return;

    FullFileName = FilesDialog.GetPath( );
    /* If the path is already in the library search paths
     * list, just add the library name to the list.  Otherwise, add
     * the library name with the full or relative path.
     * the relative path, when possible is preferable,
     * because it preserve use of default libraries paths, when the path is a sub path of these default paths
     */
    wxFileName fn = FullFileName;
    wxGetApp().SaveLastVisitedLibraryPath( fn.GetPath() );

    filename = wxGetApp().ReturnFilenameWithRelativePathInLibPath(FullFileName);
    m_TextHelpModulesFileName->SetValue( filename );
}
