/**
 * @file dialog_cvpcb_config.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <wx/tokenzr.h>
#include <pgm_base.h>
#include <common.h>
#include <confirm.h>
#include <gestfich.h>
#include <id.h>
#include <macros.h>

#include <cvpcb.h>
#include <cvpcb_mainframe.h>

#include <dialog_cvpcb_config.h>
#include <wildcards_and_files_ext.h>


DIALOG_CVPCB_CONFIG::DIALOG_CVPCB_CONFIG( CVPCB_MAINFRAME* aParent ) :
    DIALOG_CVPCB_CONFIG_FBP( aParent )
{
    wxString    title;
    wxFileName  fn = aParent->m_NetlistFileName;

    fn.SetExt( ProjectFileExtension );

    m_Parent = aParent;
    m_Config = Pgm().CommonSettings();

    Init( );
    title.Format( _( "Project file: '%s'" ), GetChars( fn.GetFullPath() ) );
    SetTitle( title );

    if( GetSizer() )
    {
        GetSizer()->SetSizeHints( this );
    }

    m_sdbSizer2OK->SetDefault();
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
    wxPathList libpaths = Pgm().GetLibraryPathList();

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
            Pgm().RemoveLibraryPath( m_listUserPaths->GetString( ii ) );

        Pgm().InsertLibraryPath( m_Parent->m_UserLibraryPath, 1 );
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

    // Set new active library list if the lib list of if default path list was modified
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

        m_Parent->LoadFootprintFiles();
        m_Parent->BuildFOOTPRINTS_LISTBOX();
        m_Parent->BuildLIBRARY_LISTBOX();
    }

    wxCommandEvent evt( ID_SAVE_PROJECT );
    m_Parent->SaveProjectFile( evt );
    EndModal( wxID_OK );
}


void DIALOG_CVPCB_CONFIG::OnCloseWindow( wxCloseEvent& event )
{
    EndModal( 0 );
}


/********************************************************************/
void DIALOG_CVPCB_CONFIG::OnButtonUpClick( wxCommandEvent& event )
/********************************************************************/
{
    wxListBox * list = m_ListLibr;

    if( (event.GetId() == ID_EQU_UP) || (event.GetId() == ID_EQU_DOWN) )
    {
        list = m_ListEquiv;
    }

    wxArrayInt selections;

    list->GetSelections( selections );

    if ( selections.GetCount() <= 0 )   // No selection.
        return;

    if( selections[0] == 0 )            // The first lib is selected. cannot move up it
        return;

    wxArrayString libnames = list->GetStrings();

    for( size_t ii = 0; ii < selections.GetCount(); ii++ )
    {
        int jj = selections[ii];
        EXCHG( libnames[jj],  libnames[jj-1] );
    }

    list->Set( libnames );

    // Reselect previously selected names
    for( size_t ii = 0; ii < selections.GetCount(); ii++ )
    {
        int jj = selections[ii];
        list->SetSelection( jj-1 );
    }

    m_LibListChanged = true;
}


/*********************************************************************/
void DIALOG_CVPCB_CONFIG::OnButtonDownClick( wxCommandEvent& event )
/*********************************************************************/
{
    wxListBox * list = m_ListLibr;

    if( (event.GetId() == ID_EQU_UP) || (event.GetId() == ID_EQU_DOWN) )
    {
        list = m_ListEquiv;
    }

    wxArrayInt selections;

    list->GetSelections( selections );

    if ( selections.GetCount() <= 0 )   // No selection.
        return;

    // The last lib is selected. cannot move down it
    if( selections.Last() == (int)(list->GetCount()-1) )
        return;

    wxArrayString libnames = list->GetStrings();

    for( int ii = selections.GetCount()-1; ii >= 0; ii-- )
    {
        int jj = selections[ii];
        EXCHG( libnames[jj],  libnames[jj+1]);
    }

    list->Set( libnames );

    // Reselect previously selected names
    for( size_t ii = 0; ii < selections.GetCount(); ii++ )
    {
        int jj = selections[ii];
        list->SetSelection(jj+1);
    }

    m_LibListChanged = true;
}


/* Remove a library to the library list.
 * The real list (g_LibName_List) is not changed, so the change can be canceled
 */
void DIALOG_CVPCB_CONFIG::OnRemoveLibClick( wxCommandEvent& event )
{
    wxListBox * list = m_ListEquiv;

    if( event.GetId() == ID_REMOVE_LIB )
        list = m_ListLibr;

    wxArrayInt selections;

    list->GetSelections( selections );

    for( int ii = selections.GetCount()-1; ii >= 0; ii-- )
    {
        list->Delete(selections[ii] );
        m_LibListChanged = true;
    }
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

    wxListBox* list = m_ListEquiv;

    if( (event.GetId() == ID_ADD_LIB) || (event.GetId() == ID_INSERT_LIB) )
    {
        list = m_ListLibr;
        wildcard = LegacyFootprintLibPathWildcard;
    }

    wxArrayInt selections;
    list->GetSelections(selections);

    ii = selections.GetCount();

    if( ii > 0 )
        ii = selections[0];
    else
        ii = 0;

    wxString libpath;
    libpath = m_DefaultLibraryPathslistBox->GetStringSelection();

    if( libpath.IsEmpty() )
        libpath = Pgm().LastVisitedLibraryPath();

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
            Pgm().SaveLastVisitedLibraryPath( fn.GetPath() );

        /* If the library path is already in the library search paths
         * list, just add the library name to the list.  Otherwise, add
         * the library name with the full or relative path.
         * the relative path, when possible is preferable,
         * because it preserve use of default libraries paths, when the path
         * is a sub path of these default paths
         */
        libfilename = Pgm().FilenameWithRelativePathInSearchList( fn.GetFullPath() );

        // Remove extension:
        fn = libfilename;
        fn.SetExt( wxEmptyString );
        libfilename = fn.GetFullPath();

        // Add or insert new library name, if not already in list
        if( list->FindString( libfilename, fn.IsCaseSensitive() ) == wxNOT_FOUND )
        {
            m_LibListChanged = true;

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
    wxString path = Pgm().LastVisitedLibraryPath();

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

        // Ask the user if this is a relative path
        int diag = wxMessageBox( _( "Use a relative path?" ),
                                 _( "Path type" ),
                                 wxYES_NO | wxICON_QUESTION, this );

        if( diag == wxYES )
        {   // Make it relative
            wxFileName fn = path;
            fn.MakeRelativeTo( wxT( "." ) );
            path = fn.GetPathWithSep() + fn.GetFullName();
        }

        m_listUserPaths->Insert( path, ipos );
        m_LibPathChanged = true;
        Pgm().InsertLibraryPath( path, ipos + 1 );

        // Display actual libraries paths:
        wxPathList libpaths = Pgm().GetLibraryPathList();
        m_DefaultLibraryPathslistBox->Clear();

        for( unsigned ii = 0; ii < libpaths.GetCount(); ii++ )
        {
            m_DefaultLibraryPathslistBox->Append( libpaths[ii] );
        }
    }
    else
    {
        DisplayError( this, _( "Path already in use" ) );
    }

    Pgm().SaveLastVisitedLibraryPath( path );
}


void DIALOG_CVPCB_CONFIG::OnRemoveUserPath( wxCommandEvent& event )
{
    int ii = m_listUserPaths->GetSelection();

    if( ii < 0 )
        ii = m_listUserPaths->GetCount() - 1;

    if( ii >= 0 )
    {
        Pgm().RemoveLibraryPath( m_listUserPaths->GetStringSelection() );
        m_listUserPaths->Delete( ii );
        m_LibPathChanged = true;
    }

    // Display actual libraries paths:
    wxPathList libpaths = Pgm().GetLibraryPathList();
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

    docpath = Pgm().LastVisitedLibraryPath( wxT( "doc" ) );

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
    Pgm().SaveLastVisitedLibraryPath( fn.GetPath() );

    filename = Pgm().FilenameWithRelativePathInSearchList( FullFileName );
    m_TextHelpModulesFileName->SetValue( filename );
}
