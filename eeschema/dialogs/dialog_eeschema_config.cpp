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
#include <schframe.h>
#include <invoke_sch_dialog.h>
#include <kiface_i.h>

#include <general.h>
#include <netlist.h>
#include <libeditframe.h>
#include <viewlib_frame.h>
#include <wildcards_and_files_ext.h>
#include <wx/tokenzr.h>
#include <dialog_eeschema_config_fbp.h>
#include <eeschema_config.h>


class SCH_EDIT_FRAME;
class EDA_DRAW_FRAME;


class DIALOG_EESCHEMA_CONFIG : public DIALOG_EESCHEMA_CONFIG_FBP
{
public:
    DIALOG_EESCHEMA_CONFIG( wxWindow* aParent,
            wxString* aCallersProjectSpecificLibPaths, wxArrayString* aCallersLibNames );

private:
    wxConfigBase*   m_config;
    wxString*       m_callers_project_specific_lib_paths;
    wxArrayString*  m_callers_lib_names;

    bool            m_lib_list_changed;
    bool            m_lib_path_changed;

    //------ event handlers, overiding the fbp handlers --------------

    void OnCloseWindow( wxCloseEvent& event );

    /* Remove a library to the library list.
     * The real list (m_Parent->m_ComponentLibFiles) is not changed, so the change can be canceled
     */
    void OnRemoveLibClick( wxCommandEvent& event );

    /* Insert or add a library to the library list:
     *   The new library is put in list before (insert button) the selection,
     *   or added (add button) to end of list
     * The real list (m_Parent->m_ComponentLibFiles) is not changed, so the change
     * can be canceled
     */
    void OnAddOrInsertLibClick( wxCommandEvent& event );

    void OnAddOrInsertPath( wxCommandEvent& event );
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );
    void OnRemoveUserPath( wxCommandEvent& event );
    void OnButtonUpClick( wxCommandEvent& event );
    void OnButtonDownClick( wxCommandEvent& event );
};


DIALOG_EESCHEMA_CONFIG::DIALOG_EESCHEMA_CONFIG( wxWindow* aParent,
            wxString* aCallersProjectSpecificLibPaths, wxArrayString* aCallersLibNames ) :
    DIALOG_EESCHEMA_CONFIG_FBP( aParent ),
    m_callers_project_specific_lib_paths( aCallersProjectSpecificLibPaths ),
    m_callers_lib_names( aCallersLibNames ),
    m_lib_list_changed( false ),
    m_lib_path_changed( false )
{
    m_ListLibr->InsertItems( *aCallersLibNames, 0 );

    // Load user libs paths:
    wxArrayString paths;

    SEARCH_STACK::Split( &paths, *aCallersProjectSpecificLibPaths );

    for( unsigned i=0; i<paths.GetCount();  ++i )
    {
        wxString path = paths[i];

        if( wxFileName::DirExists( Prj().AbsolutePath( path ) ) )
            m_listUserPaths->Append( path );
    }

    // Display actual library paths which come in part from KIFACE::KifaceSearch()
    // along with aCallersProjectSpecificLibPaths at the front.
    SEARCH_STACK* libpaths = Prj().SchSearchS();

    DBG( libpaths->Show( __func__ ); )

    for( unsigned ii = 0; ii < libpaths->GetCount(); ii++ )
    {
        m_DefaultLibraryPathslistBox->Append( (*libpaths)[ii] );
    }

    // select the first path after the current project's path
    if( libpaths->GetCount() > 1 )
        m_DefaultLibraryPathslistBox->Select( 1 );

    // Load setting for cache rescue
    m_config = Kiface().KifaceSettings();
    bool rescueNeverShow = false;
    m_config->Read( RESCUE_NEVER_SHOW_KEY, &rescueNeverShow, false );
    m_cbRescue->SetValue( !rescueNeverShow );

    wxString msg = wxString::Format( _(
        "Project '%s'" ),
        GetChars( Prj().GetProjectFullName() )
        );

    SetTitle( msg );

    if( GetSizer() )
        GetSizer()->SetSizeHints( this );

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
        std::swap( libnames[jj],  libnames[jj-1]);
    }

    m_ListLibr->Set(libnames);

    // Reselect previously selected names
    for( size_t ii = 0; ii < selections.GetCount(); ii++ )
    {
        int jj = selections[ii];
        m_ListLibr->SetSelection(jj-1);
    }

    m_lib_list_changed = true;
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
        std::swap( libnames[jj],  libnames[jj+1]);
    }

    m_ListLibr->Set( libnames );

    // Reselect previously selected names
    for( size_t ii = 0; ii < selections.GetCount(); ii++ )
    {
        int jj = selections[ii];
        m_ListLibr->SetSelection(jj+1);
    }

    m_lib_list_changed = true;
}


void DIALOG_EESCHEMA_CONFIG::OnCancelClick( wxCommandEvent& event )
{
    EndModal( wxID_CANCEL );
}


void DIALOG_EESCHEMA_CONFIG::OnOkClick( wxCommandEvent& event )
{
    // Give caller the changed paths
    if( m_lib_path_changed )
    {
        wxString paths;

        for( unsigned ii = 0; ii < m_listUserPaths->GetCount(); ii++ )
        {
            if( ii > 0 )
                paths += wxT( ';' );

            paths += m_listUserPaths->GetString( ii );
        }

        *m_callers_project_specific_lib_paths = paths;
    }

    // Update caller's lib_names if changed.
    if( m_lib_list_changed )
    {
        wxArrayString list;

        for( unsigned ii = 0; ii < m_ListLibr->GetCount(); ii ++ )
            list.Add( m_ListLibr->GetString( ii ) );

        // Recreate lib list
        *m_callers_lib_names = list;
    }

    m_config->Write( RESCUE_NEVER_SHOW_KEY, ! m_cbRescue->GetValue() );

    EndModal( wxID_OK );
}


void DIALOG_EESCHEMA_CONFIG::OnCloseWindow( wxCloseEvent& event )
{
    EndModal( wxID_CANCEL );
}


void DIALOG_EESCHEMA_CONFIG::OnRemoveLibClick( wxCommandEvent& event )
{
    wxArrayInt selections;

    m_ListLibr->GetSelections( selections );

    for( int ii = selections.GetCount()-1; ii >= 0; ii-- )
    {
        m_ListLibr->Delete( selections[ii] );
        m_lib_list_changed = true;
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


void DIALOG_EESCHEMA_CONFIG::OnAddOrInsertLibClick( wxCommandEvent& event )
{
    int             ii;
    wxString        libfilename;
    wxArrayInt      selections;

    PROJECT&        prj = Prj();

    m_ListLibr->GetSelections( selections );

    ii = selections.GetCount();

    if( ii > 0 )
        ii = selections[0];
    else
        ii = 0;

    wxString selection = m_DefaultLibraryPathslistBox->GetStringSelection();
    wxString libpath   = Prj().AbsolutePath( selection );

    if( !libpath )
    {
        libpath = prj.GetRString( PROJECT::SCH_LIB_PATH );
    }

    wxFileDialog filesDialog( this, _( "Library files:" ), libpath,
                              wxEmptyString, SchematicLibraryFileWildcard,
                              wxFD_DEFAULT_STYLE | wxFD_MULTIPLE );

    if( filesDialog.ShowModal() != wxID_OK )
        return;

    wxArrayString   filenames;

    filesDialog.GetPaths( filenames );

    wxFileName      fn;

    // Build libs paths, to find later a relative path:
    wxArrayString paths;

    for( unsigned ll=0; ll < m_DefaultLibraryPathslistBox->GetCount(); ++ll )
        paths.Add( m_DefaultLibraryPathslistBox->GetString( ll ) );

    for( unsigned ll=0; ll < m_listUserPaths->GetCount(); ++ll )
        paths.Add( m_listUserPaths->GetString( ll ) );

    for( unsigned jj = 0; jj < filenames.GetCount(); jj++ )
    {
        fn = filenames[jj];

        if( jj == 0 )
            prj.SetRString( PROJECT::SCH_LIB_PATH, fn.GetPath() );

        // Extension is not stored, so remove extension:
        fn.SetExt( wxEmptyString );

        // Try to use relative path:
        for( unsigned ll = 0; ll < paths.GetCount(); ll++ )
        {
            wxFileName relfn = fn;
            relfn.MakeRelativeTo( paths[ll] );

            if( relfn.GetPath()[0] != '.' )
            {
                fn = relfn;
                break;
            }
        }

        libfilename = fn.GetFullPath();

        // Add or insert new library name, if not already in list
        if( m_ListLibr->FindString( libfilename, fn.IsCaseSensitive() ) == wxNOT_FOUND )
        {
            m_lib_list_changed = true;

            if( event.GetId() == ID_ADD_LIB )
                m_ListLibr->Append( libfilename );
            else
                m_ListLibr->Insert( libfilename, ii++ );
        }
        else
        {
            wxString msg = wxString::Format( _(
                "'%s' : library already in use" ),
                GetChars( libfilename )
                );
            DisplayError( this, msg );
        }
    }
}


void DIALOG_EESCHEMA_CONFIG::OnAddOrInsertPath( wxCommandEvent& event )
{
    PROJECT&        prj = Prj();
    wxString        abs_path = prj.GetRString( PROJECT::SCH_LIB_PATH );
    wxString        path;

    bool select = EDA_DirectorySelector( _( "Default Path for Libraries" ),
                                         abs_path, wxDD_DEFAULT_STYLE,
                                         this, wxDefaultPosition );

    if( !select )
        return;

    if( !wxFileName::DirExists( abs_path ) )    // Should not occur
        return;

    // Add or insert path if not already in list
    if( m_listUserPaths->FindString( abs_path ) == wxNOT_FOUND )
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
            wxFileName fn = abs_path;
            fn.MakeRelativeTo( wxPathOnly( Prj().GetProjectFullName() ) );
            path = fn.GetPathWithSep() + fn.GetFullName();
        }
        else
            path = abs_path;

        m_listUserPaths->Insert( path, ipos );
        m_lib_path_changed = true;

        m_DefaultLibraryPathslistBox->InsertItems( 1, &path, ipos+1 );
    }
    else
    {
        DisplayError( this, _("Path already in use") );
    }

    prj.SetRString( PROJECT::SCH_LIB_PATH, abs_path );
}


static void remove_from_listbox( wxListBox* aListBox, const wxString& aText )
{
    wxArrayString   a;

    for( int i=0, cnt = aListBox->GetCount();  i<cnt;  ++i )
    {
        wxString item = aListBox->GetString( i );

        if( item != aText )
            a.Add( item );
    }

    aListBox->Clear();

    aListBox->InsertItems( a, 0 );
}


void DIALOG_EESCHEMA_CONFIG::OnRemoveUserPath( wxCommandEvent& event )
{
    int ii = m_listUserPaths->GetSelection();

    if( ii < 0 )
        ii = m_listUserPaths->GetCount()-1;

    if( ii >= 0 )
    {
        wxString sel = m_listUserPaths->GetStringSelection();

        remove_from_listbox( m_DefaultLibraryPathslistBox, sel );

        m_listUserPaths->Delete( ii );
        m_lib_path_changed = true;
    }
}


bool InvokeEeschemaConfig( wxWindow* aParent,
        wxString* aCallersProjectSpecificLibPaths, wxArrayString* aCallersLibNames )
{
    DIALOG_EESCHEMA_CONFIG  dlg( aParent,
            aCallersProjectSpecificLibPaths, aCallersLibNames );

    int ret = dlg.ShowModal();

    return wxID_OK == ret;
}
