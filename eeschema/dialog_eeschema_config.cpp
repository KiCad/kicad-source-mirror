/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_eeschema_config.cpp
// Purpose:
// Author:      jean-pierre Charras
// Created:     17/02/2006 21:14:46
// Copyright:   Kicad Team
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "confirm.h"
#include "gestfich.h"

#include "program.h"
#include "general.h"
#include "protos.h"
#include "netlist.h"
#include "libeditfrm.h"
#include "libviewfrm.h"

#include <wx/tokenzr.h>

#include "dialog_eeschema_config_fbp.h"

class DIALOG_EESCHEMA_CONFIG : public DIALOG_EESCHEMA_CONFIG_FBP
{
private:
    WinEDA_SchematicFrame* m_Parent;
    bool m_LibListChanged;
    bool m_LibPathChanged;
    wxString m_UserLibDirBufferImg;      // Copy of original g_UserLibDirBuffer

private:

    // event handlers, overiding the fbp handlers
    void Init();
    void OnCloseWindow( wxCloseEvent& event );
    void OnRemoveLibClick( wxCommandEvent& event );
    void OnAddOrInsertLibClick( wxCommandEvent& event );
    void OnAddOrInsertPath( wxCommandEvent& event );
	void OnOkClick( wxCommandEvent& event );
	void OnCancelClick( wxCommandEvent& event );
    void OnRemoveUserPath( wxCommandEvent& event );


public:
    DIALOG_EESCHEMA_CONFIG( WinEDA_SchematicFrame * parent );
    ~DIALOG_EESCHEMA_CONFIG() {};
};


/******************************************************************/
void WinEDA_SchematicFrame::InstallConfigFrame( const wxPoint& pos )
/******************************************************************/
{
    DIALOG_EESCHEMA_CONFIG CfgFrame( this );

    CfgFrame.ShowModal();
}


/*******************************************************************************/
DIALOG_EESCHEMA_CONFIG::DIALOG_EESCHEMA_CONFIG( WinEDA_SchematicFrame* parent )
    : DIALOG_EESCHEMA_CONFIG_FBP( parent )
/*******************************************************************************/
{
    wxString msg;

    m_Parent = parent;

    Init();

    msg = _( "from " ) + wxGetApp().m_CurrentOptionFile;
    SetTitle( msg );

    if( GetSizer() )
        GetSizer()->SetSizeHints( this );
}


/***********************************/
void DIALOG_EESCHEMA_CONFIG::Init()
/***********************************/
{
    wxString msg;

    SetFocus();

    m_LibListChanged = false;
    m_LibPathChanged = false;
    m_UserLibDirBufferImg = m_Parent->m_UserLibraryPath;


    m_ListLibr->InsertItems( m_Parent->m_ComponentLibFiles, 0 );

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
void DIALOG_EESCHEMA_CONFIG::OnCancelClick( wxCommandEvent& event )
/******************************************************************/
{
    // Recreate the user lib path
    if ( m_LibPathChanged )
    {
        for ( unsigned ii = 0; ii < m_ListLibr->GetCount(); ii ++ )
            wxGetApp().RemoveLibraryPath( m_listUserPaths->GetString(ii)) ;
        wxGetApp().InsertLibraryPath( m_Parent->m_UserLibraryPath, 1);
    }

    EndModal( wxID_CANCEL );
}


/**************************************************************/
void DIALOG_EESCHEMA_CONFIG::OnOkClick( wxCommandEvent& event )
/**************************************************************/
{
    // Recreate the user lib path
    if ( m_LibPathChanged )
    {
        m_Parent->m_UserLibraryPath.Empty();
        for ( unsigned ii = 0; ii < m_listUserPaths->GetCount(); ii ++ )
        {
            if ( ii > 0 )
                m_Parent->m_UserLibraryPath << wxT(";");
            m_Parent->m_UserLibraryPath << m_listUserPaths->GetString(ii);
        }
    }

    /* Set new active library list if the lib list of if default path list
     * was modified
     */
    if( m_LibListChanged || m_LibPathChanged )
    {
        // Recreate lib list
        m_Parent->m_ComponentLibFiles.Clear();
        for ( unsigned ii = 0; ii < m_ListLibr->GetCount(); ii ++ )
            m_Parent->m_ComponentLibFiles.Add(m_ListLibr->GetString(ii) );

        // take new list in account
        m_Parent->LoadLibraries();
        // Clear (if needed) the current active library in libedit because it could be
        // removed from memory
        WinEDA_LibeditFrame::EnsureActiveLibExists();
    }

    m_Parent->SaveProjectFile( this, false );
    EndModal( wxID_OK );
}

/**************************************************************/
void DIALOG_EESCHEMA_CONFIG::OnCloseWindow( wxCloseEvent& event )
/**************************************************************/
{
    EndModal( wxID_CANCEL );
}



/*********************************************************************/
void DIALOG_EESCHEMA_CONFIG::OnRemoveLibClick( wxCommandEvent& event )
/*********************************************************************/
/* Remove a library to the library list.
 * The real list (m_Parent->m_ComponentLibFiles) is not changed, so the change can be cancelled
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
void DIALOG_EESCHEMA_CONFIG::OnAddOrInsertLibClick( wxCommandEvent& event )
/**************************************************************************/

/* Insert or add a library to the library list:
 *   The new library is put in list before (insert button) the selection,
 *   or added (add button) to end of list
 * The real list (m_Parent->m_ComponentLibFiles) is not changed, so the change
 * can be cancelled
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

    wxFileDialog FilesDialog( this, _( "Library files:" ), libpath,
                              wxEmptyString, CompLibFileWildcard,
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
         * because it preserve use of default libraries paths, when the path
         * is a sub path of these default paths
         */
        libfilename = wxGetApp().ReturnFilenameWithRelativePathInLibPath( fn.GetFullPath() );
        // Remove extension:
        fn = libfilename;
        fn.SetExt(wxEmptyString);
        libfilename = fn.GetFullPath();

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



/***********************************************************************/
void DIALOG_EESCHEMA_CONFIG::OnAddOrInsertPath( wxCommandEvent& event )
/***********************************************************************/
{
    wxString path = wxGetApp().ReturnLastVisitedLibraryPath();

    bool select = EDA_DirectorySelector( _( "Default Path for Libraries" ),
                                         path, wxDD_DEFAULT_STYLE,
                                         this, wxDefaultPosition );

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

        // Ask the user if this is a relative path
        int diag = wxMessageBox(
            _( "Use a relative path?" ),
            _( "Path type" ),
            wxYES_NO | wxICON_QUESTION, this );

        if( diag == wxYES )
        {   // Make it relative
            wxFileName fn = path;
            fn.MakeRelativeTo( wxT(".") );
            path = fn.GetPathWithSep() + fn.GetFullName();
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
void DIALOG_EESCHEMA_CONFIG::OnRemoveUserPath( wxCommandEvent& event )
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
