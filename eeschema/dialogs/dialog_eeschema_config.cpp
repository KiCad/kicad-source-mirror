/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2006-2013 KiCad Developers, see change_log.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file dialog_eeschema_config.cpp
 */

#include <fctsys.h>
#include <pgm_base.h>
#include <kiway.h>
#include <confirm.h>
#include <gestfich.h>
#include <wxEeschemaStruct.h>
#include <invoke_sch_dialog.h>

#include <general.h>
#include <netlist.h>
#include <libeditframe.h>
#include <viewlib_frame.h>
#include <wildcards_and_files_ext.h>

#include <wx/tokenzr.h>


#include <dialog_eeschema_config_fbp.h>

class SCH_EDIT_FRAME;
class EDA_DRAW_FRAME;

class DIALOG_EESCHEMA_CONFIG : public DIALOG_EESCHEMA_CONFIG_FBP
{
public:
    DIALOG_EESCHEMA_CONFIG( SCH_EDIT_FRAME* parent, wxFrame* activeWindow );

private:
    SCH_EDIT_FRAME* m_Parent;
    bool            m_LibListChanged;
    bool            m_LibPathChanged;
    wxString        m_UserLibDirBufferImg;

    // event handlers, overiding the fbp handlers
    void Init();
    void OnCloseWindow( wxCloseEvent& event );
    void OnRemoveLibClick( wxCommandEvent& event );
    void OnAddOrInsertLibClick( wxCommandEvent& event );
    void OnAddOrInsertPath( wxCommandEvent& event );
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );
    void OnRemoveUserPath( wxCommandEvent& event );
    void OnButtonUpClick( wxCommandEvent& event );
    void OnButtonDownClick( wxCommandEvent& event );
};


DIALOG_EESCHEMA_CONFIG::DIALOG_EESCHEMA_CONFIG( SCH_EDIT_FRAME* aSchFrame,
        wxFrame* aParent ) :
    DIALOG_EESCHEMA_CONFIG_FBP( aParent )
{
    m_Parent = aSchFrame;

    Init();

    wxString msg = wxString::Format(
        _( "from '%s'" ),
        GetChars( Prj().GetProjectFullName() ) );

    SetTitle( msg );

    if( GetSizer() )
        GetSizer()->SetSizeHints( this );
}


void DIALOG_EESCHEMA_CONFIG::Init()
{
    wxString msg;

    SetFocus();

    m_LibListChanged = false;
    m_LibPathChanged = false;
    m_UserLibDirBufferImg = m_Parent->GetUserLibraryPath();

    m_ListLibr->InsertItems( m_Parent->GetComponentLibraries(), 0 );

    // Load user libs paths:
    wxStringTokenizer tokenizer( m_UserLibDirBufferImg, wxT( ";\n\r" ) );

    while( tokenizer.HasMoreTokens() )
    {
        wxString path = tokenizer.GetNextToken();

        if( wxFileName::DirExists( path ) )
            m_listUserPaths->Append( path );
    }

    // Display actual libraries paths:
    SEARCH_STACK& libpaths = Prj().SchSearchS();

    for( unsigned ii = 0; ii < libpaths.GetCount(); ii++ )
    {
        m_DefaultLibraryPathslistBox->Append( libpaths[ii] );
    }

    // select the first path after the current path project
    if ( libpaths.GetCount() > 1 )
        m_DefaultLibraryPathslistBox->Select( 1 );

    m_sdbSizer1OK->SetDefault();
}


void DIALOG_EESCHEMA_CONFIG::OnButtonUpClick( wxCommandEvent& event )
{
    wxArrayInt selections;

    m_ListLibr->GetSelections( selections );

    if ( selections.GetCount() <= 0 )   // No selection.
        return;

    if( selections[0] == 0 )            // The first lib is selected. cannot move up it
        return;

    wxArrayString libnames = m_ListLibr->GetStrings();

    for( size_t ii = 0; ii < selections.GetCount(); ii++ )
    {
        int jj = selections[ii];
        EXCHG( libnames[jj],  libnames[jj-1]);
    }

    m_ListLibr->Set(libnames);

    // Reselect previously selected names
    for( size_t ii = 0; ii < selections.GetCount(); ii++ )
    {
        int jj = selections[ii];
        m_ListLibr->SetSelection(jj-1);
    }

    m_LibListChanged = true;
}


void DIALOG_EESCHEMA_CONFIG::OnButtonDownClick( wxCommandEvent& event )
{
    wxArrayInt selections;

    m_ListLibr->GetSelections(selections);

    if ( selections.GetCount() <= 0 )   // No selection.
        return;

    // The last lib is selected. cannot move down it
    if( selections.Last() == (int)(m_ListLibr->GetCount()-1) )
        return;

    wxArrayString libnames = m_ListLibr->GetStrings();

    for( int ii = selections.GetCount()-1; ii >= 0; ii-- )
    {
        int jj = selections[ii];
        EXCHG( libnames[jj],  libnames[jj+1]);
    }

    m_ListLibr->Set( libnames );

    // Reselect previously selected names
    for( size_t ii = 0; ii < selections.GetCount(); ii++ )
    {
        int jj = selections[ii];
        m_ListLibr->SetSelection(jj+1);
    }

    m_LibListChanged = true;
}


void DIALOG_EESCHEMA_CONFIG::OnCancelClick( wxCommandEvent& event )
{
    SEARCH_STACK&    lib_search = Prj().SchSearchS();

    // Recreate the user lib path
    if( m_LibPathChanged )
    {
        for( unsigned ii = 0; ii < m_listUserPaths->GetCount(); ii++ )
            lib_search.RemovePaths( m_listUserPaths->GetString(ii) );

        lib_search.AddPaths( m_Parent->GetUserLibraryPath(), 1 );
    }

    EndModal( wxID_CANCEL );
}


void DIALOG_EESCHEMA_CONFIG::OnOkClick( wxCommandEvent& event )
{
    // Recreate the user lib path
    if( m_LibPathChanged )
    {
        wxString path;

        for( unsigned ii = 0; ii < m_listUserPaths->GetCount(); ii++ )
        {
            if( ii > 0 )
                path << wxT( ";" );

            path << m_listUserPaths->GetString( ii );
        }

        m_Parent->SetUserLibraryPath( path );
    }

    /* Set new active library list if the lib list of if default path list
     * was modified
     */
    if( m_LibListChanged || m_LibPathChanged )
    {
        wxArrayString list;

        for( unsigned ii = 0; ii < m_ListLibr->GetCount(); ii ++ )
            list.Add( m_ListLibr->GetString( ii ) );

        // Recreate lib list
        m_Parent->SetComponentLibraries( list );

        // take new list in account
        m_Parent->LoadLibraries();

        // Clear (if needed) the current active library in libedit because it could be
        // removed from memory
        LIB_EDIT_FRAME::EnsureActiveLibExists();
    }

    m_Parent->SaveProjectSettings( false );

    EndModal( wxID_OK );
}


void DIALOG_EESCHEMA_CONFIG::OnCloseWindow( wxCloseEvent& event )
{
    EndModal( wxID_CANCEL );
}


/* Remove a library to the library list.
 * The real list (m_Parent->m_ComponentLibFiles) is not changed, so the change can be canceled
 */
void DIALOG_EESCHEMA_CONFIG::OnRemoveLibClick( wxCommandEvent& event )
{
    wxArrayInt selections;

    m_ListLibr->GetSelections( selections );

    for( int ii = selections.GetCount()-1; ii >= 0; ii-- )
    {
        m_ListLibr->Delete( selections[ii] );
        m_LibListChanged = true;
    }

    // Select next item after deleted in m_ListLibr
    if( m_ListLibr->GetCount() > 0 && selections.GetCount() > 0 )
    {
        int pos = selections[selections.GetCount()-1];

        if( pos == (int)m_ListLibr->GetCount() )
            pos = m_ListLibr->GetCount() - 1;

        m_ListLibr->SetSelection( pos );
    }
}


/* Insert or add a library to the library list:
 *   The new library is put in list before (insert button) the selection,
 *   or added (add button) to end of list
 * The real list (m_Parent->m_ComponentLibFiles) is not changed, so the change
 * can be canceled
 */
void DIALOG_EESCHEMA_CONFIG::OnAddOrInsertLibClick( wxCommandEvent& event )
{
    int             ii;
    wxString        libfilename;
    wxFileName      fn;
    wxArrayInt      selections;

    PROJECT&        prj = Prj();
    SEARCH_STACK&   search = prj.SchSearchS();

    m_ListLibr->GetSelections( selections );

    ii = selections.GetCount();

    if( ii > 0 )
        ii = selections[0];
    else
        ii = 0;

    wxString libpath = m_DefaultLibraryPathslistBox->GetStringSelection();

    if( !libpath )
    {
        libpath = prj.GetRString( PROJECT::SCH_LIB_PATH );
        if( !libpath )
            libpath = search.LastVisitedPath();
    }

    wxFileDialog filesDialog( this, _( "Library files:" ), libpath,
                              wxEmptyString, SchematicLibraryFileWildcard,
                              wxFD_DEFAULT_STYLE | wxFD_MULTIPLE );

    if( filesDialog.ShowModal() != wxID_OK )
        return;

    wxArrayString filenames;

    filesDialog.GetPaths( filenames );

    for( unsigned jj = 0; jj < filenames.GetCount(); jj++ )
    {
        fn = filenames[jj];

        if( jj == 0 )
            prj.SetRString( PROJECT::SCH_LIB_PATH, fn.GetPath() );

        /* If the library path is already in the library search paths
         * list, just add the library name to the list.  Otherwise, add
         * the library name with the full or relative path.
         * the relative path, when possible is preferable,
         * because it preserve use of default libraries paths, when the path
         * is a sub path of these default paths
         */
        libfilename = search.FilenameWithRelativePathInSearchList( fn.GetFullPath() );

        // Remove extension:
        fn = libfilename;
        fn.SetExt( wxEmptyString );
        libfilename = fn.GetFullPath();

        // Add or insert new library name, if not already in list
        if( m_ListLibr->FindString( libfilename, fn.IsCaseSensitive() ) == wxNOT_FOUND )
        {
            m_LibListChanged = true;

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


void DIALOG_EESCHEMA_CONFIG::OnAddOrInsertPath( wxCommandEvent& event )
{
    PROJECT&        prj = Prj();
    SEARCH_STACK&   search = Prj().SchSearchS();
    wxString        path = prj.GetRString( PROJECT::SCH_LIB_PATH );

    if( !path )
        path = search.LastVisitedPath();

    bool select = EDA_DirectorySelector( _( "Default Path for Libraries" ),
                                         path, wxDD_DEFAULT_STYLE,
                                         this, wxDefaultPosition );

    if( !select )
        return;

    if( !wxFileName::DirExists( path ) )    // Should not occurs
        return;

    // Add or insert path if not already in list
    if( m_listUserPaths->FindString( path ) == wxNOT_FOUND )
    {
        int ipos = m_listUserPaths->GetCount();

        if( event.GetId() == wxID_INSERT_PATH )
        {
            if( ipos  )
                ipos--;

            int jj = m_listUserPaths->GetSelection();

            if( jj >= 0 )
                ipos = jj;
        }

        // Ask the user if this is a relative path
       int diag = wxMessageBox( _( "Use a relative path?" ), _( "Path type" ),
                                wxYES_NO | wxICON_QUESTION, this );

        if( diag == wxYES )
        {
            // Make it relative
            wxFileName fn = path;
            fn.MakeRelativeTo( wxT(".") );
            path = fn.GetPathWithSep() + fn.GetFullName();
        }

        m_listUserPaths->Insert(path, ipos);
        m_LibPathChanged = true;

        search.AddPaths( path, ipos+1 );

        // Display actual libraries paths:
        m_DefaultLibraryPathslistBox->Clear();

        for( unsigned ii = 0; ii < search.GetCount(); ii++ )
        {
            m_DefaultLibraryPathslistBox->Append( search[ii] );
        }
    }
    else
    {
        DisplayError( this, _("Path already in use") );
    }

    prj.SetRString( PROJECT::SCH_LIB_PATH, path );
}


void DIALOG_EESCHEMA_CONFIG::OnRemoveUserPath( wxCommandEvent& event )
{
    SEARCH_STACK&    lib_search = Prj().SchSearchS();

    int ii = m_listUserPaths->GetSelection();

    if( ii < 0 )
        ii = m_listUserPaths->GetCount()-1;

    if( ii >= 0 )
    {
        lib_search.RemovePaths( m_listUserPaths->GetStringSelection() );

        m_listUserPaths->Delete( ii );
        m_LibPathChanged = true;
    }

    // Display actual libraries paths:
    m_DefaultLibraryPathslistBox->Clear();

    for( unsigned ii = 0; ii < lib_search.GetCount(); ii++ )
    {
        m_DefaultLibraryPathslistBox->Append( lib_search[ii] );
    }
}


int InvokeEeschemaConfig( SCH_EDIT_FRAME* aEditFrame, wxFrame* aParent )
{
    DIALOG_EESCHEMA_CONFIG  dlg( aEditFrame, aParent );

    dlg.ShowModal();

    return 1;
}
