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
    wxFileName fn = parent->m_NetlistFileName;
    fn.SetExt( ProjectFileExtension );

    m_Parent   = parent;
    m_Config = wxGetApp().m_EDA_CommonConfig;

    Init( );
    title = _( "Project file: " ) + fn.GetFullPath();
    SetTitle( title );
    if( GetSizer() )
    {
        GetSizer()->SetSizeHints( this );
    }
}


void DIALOG_CVPCB_CONFIG::Init()
{
    wxString msg;

    SetFocus();

    m_LibListChanged = false;
    m_LibPathChanged = false;
    m_UserLibDirBufferImg = m_Parent->m_UserLibraryPath;

    m_ListLibr->InsertItems( m_Parent->m_ModuleLibNames, 0 );
    m_ListEquiv->InsertItems( m_Parent->m_AliasLibNames, 0 );

    m_TextHelpModulesFileName->SetValue( m_Parent->m_DocModulesFileName );

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

    // select the first path after the current path project
    if( libpaths.GetCount() > 1 )
        m_DefaultLibraryPathslistBox->Select( 1 );
}


void DIALOG_CVPCB_CONFIG::OnCancelClick( wxCommandEvent& event )
{
    // Recreate the user lib path
    if( m_LibPathChanged )
    {
        for( unsigned ii = 0; ii < m_ListLibr->GetCount(); ii++ )
            wxGetApp().RemoveLibraryPath( m_listUserPaths->GetString( ii ) );

        wxGetApp().InsertLibraryPath( m_Parent->m_UserLibraryPath, 1 );
    }

    EndModal( wxID_CANCEL );
}


void DIALOG_CVPCB_CONFIG::OnOkClick( wxCommandEvent& event )
{
    m_Parent->m_DocModulesFileName = m_TextHelpModulesFileName->GetValue();

    // Recreate the user lib path
    if( m_LibPathChanged )
    {
        m_Parent->m_UserLibraryPath.Empty();
        for( unsigned ii = 0; ii < m_listUserPaths->GetCount(); ii++ )
        {
            if( ii > 0 )
                m_Parent->m_UserLibraryPath << wxT( ";" );
            m_Parent->m_UserLibraryPath << m_listUserPaths->GetString( ii );
        }
    }

    // Set new active library list if the lib list of if default path list
    // was modified
    if( m_LibListChanged || m_LibPathChanged )
    {
        // Recreate lib list
        m_Parent->m_ModuleLibNames.Clear();
        for( unsigned ii = 0; ii < m_ListLibr->GetCount(); ii++ )
            m_Parent->m_ModuleLibNames.Add( m_ListLibr->GetString( ii ) );

        // Recreate equ list
        m_Parent->m_AliasLibNames.Clear();
        for( unsigned ii = 0; ii < m_ListEquiv->GetCount(); ii++ )
            m_Parent->m_AliasLibNames.Add( m_ListEquiv->GetString( ii ) );

        LoadFootprintFiles( m_Parent->m_ModuleLibNames,
                            m_Parent->m_footprints );
        m_Parent->BuildFOOTPRINTS_LISTBOX();
    }

    m_Parent->SaveProjectFile( m_Parent->m_NetlistFileName.GetFullPath() );
    EndModal( wxID_OK );
}


void DIALOG_CVPCB_CONFIG::OnCloseWindow( wxCloseEvent& event )
{
    EndModal( 0 );
}


/* Remove a library to the library list.
 * The real list (g_LibName_List) is not changed, so the change can be canceled
 */
void DIALOG_CVPCB_CONFIG::OnRemoveLibClick( wxCommandEvent& event )
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


/* Insert or add a library to the library list:
 *   The new library is put in list before (insert button) the selection,
 *   or added (add button) to end of list
 * The real list (g_LibName_List) is not changed, so the change can be canceled
 */
void DIALOG_CVPCB_CONFIG::OnAddOrInsertLibClick( wxCommandEvent& event )
{
    int        ii;
    wxString   libfilename, wildcard;
    wxFileName fn;

    bool       insert = false;

    if( (event.GetId() == ID_INSERT_EQU) || (event.GetId() == ID_INSERT_LIB) )
        insert = true;

    wildcard = FootprintAliasFileWildcard;
    wxListBox * list = m_ListEquiv;
    if( (event.GetId() == ID_ADD_LIB) || (event.GetId() == ID_INSERT_LIB) )
    {
        list = m_ListLibr;
        wildcard = ModuleFileWildcard;
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
         * because it preserve use of default libraries paths, when the path
         * is a sub path of these default paths
         */
        libfilename = wxGetApp().ReturnFilenameWithRelativePathInLibPath( fn.GetFullPath() );
        // Remove extension:
        fn = libfilename;
        fn.SetExt(wxEmptyString);
        libfilename = fn.GetFullPath();

        // Add or insert new library name, if not already in list
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


void DIALOG_CVPCB_CONFIG::OnAddOrInsertPath( wxCommandEvent& event )
{
    wxString path = wxGetApp().ReturnLastVisitedLibraryPath();

    bool     select = EDA_DirectorySelector( _( "Default Path for Libraries" ),
                                             path,
                                             wxDD_DEFAULT_STYLE,
                                             this,
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


void DIALOG_CVPCB_CONFIG::OnRemoveUserPath( wxCommandEvent& event )
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


void DIALOG_CVPCB_CONFIG::OnBrowseModDocFile( wxCommandEvent& event )
{
    wxString FullFileName;
    wxString docpath, filename;

    docpath = wxGetApp().ReturnLastVisitedLibraryPath( wxT( "doc" ) );

    wxFileDialog FilesDialog( this, _( "Footprint document file:" ), docpath,
                              wxEmptyString, PdfFileWildcard,
                              wxFD_DEFAULT_STYLE | wxFD_FILE_MUST_EXIST );

    if( FilesDialog.ShowModal() != wxID_OK )
        return;

    FullFileName = FilesDialog.GetPath();

    /* If the path is already in the library search paths
     * list, just add the library name to the list.  Otherwise, add
     * the library name with the full or relative path.
     * the relative path, when possible is preferable,
     * because it preserve use of default libraries paths, when the path is
     * a sub path of these default paths
     */
    wxFileName fn = FullFileName;
    wxGetApp().SaveLastVisitedLibraryPath( fn.GetPath() );

    filename = wxGetApp().ReturnFilenameWithRelativePathInLibPath( FullFileName );
    m_TextHelpModulesFileName->SetValue( filename );
}
