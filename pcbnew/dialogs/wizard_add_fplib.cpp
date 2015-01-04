/**
 * @file wizard_add_fplib.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see AUTHORS.txt for contributors.
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

/** @brief this code creates a wizard to add entries in the footprint library table.
 * The wizard contains 3 pages:
 * The first is the selection of the type of libraries (plugin type):
 *  * Kicad .pretty library (actually a folder containing .kicad_mod footprint files)
 *  * Gihtub .pretty on line library, accessible via a internet connection
 *  * Legacy library (old .mod format file containing footprints desc)
 *  * Eagle xml V6 library (.lbr files)
 *  * Geda-PCB library (actually a folder containing .fp footprint files
 * The second is the selection of path management:
 *  use environment variable or absolute path.
 *  When using an environment variable, selec it
 * The third is the library list selection
 *  It allows entering entries by running a tool to select a set of libraries
 *  The tool depend on the type of lib (files/folder/urls):
 *    * multi files selector
 *    * multi folder selector
 *    * multi URL selector (via a web viewer)
 * The path and the plugin type comes from the selection, and a library nickname
 * is built from the path.
 */

#include <wx/wx.h>
#include <wx/url.h>
#include <pgm_base.h>
#include <kiface_i.h>
#include <dialog_helpers.h>
#include <project.h>        // For PROJECT_VAR_NAME definition
#include <io_mgr.h>
#include <wizard_add_fplib.h>
#include <dialog_select_dirlist_base.h>

// a key to store the default Kicad Github libs URL
#define KICAD_FPLIBS_URL_KEY wxT( "kicad_fplib_url" )

// key to store last options of th wizard
#define WIZARD_LAST_PLUGIN_KEY wxT( "wizard_plugin" )
#define WIZARD_LAST_PATHOPTION_KEY wxT( "wizard_path_option" )

// static members to store last choices during a session
int WIZARD_FPLIB_TABLE::m_last_plugin_choice = 0;
int WIZARD_FPLIB_TABLE::m_last_defaultpath_choice = 2;

WIZARD_FPLIB_TABLE::WIZARD_FPLIB_TABLE( wxWindow* aParent, wxArrayString& aEnvVariableList )
        : WIZARD_FPLIB_TABLE_BASE( aParent )
{
    initDlg( aEnvVariableList );

    // Allows github plugin selection only when the plugin is compiled:
#ifndef BUILD_GITHUB_PLUGIN
    m_rbFpLibFormat->Enable( GITHUB_PLUGIN, false );

    if( m_rbFpLibFormat->GetSelection() == GITHUB_PLUGIN )
        m_rbFpLibFormat->SetSelection( KICAD_PLUGIN );
#endif

    // Currently, I (JPC) do not know the best way to add/store
    // what is currently called env variables
    // So do not show tools to change them,
    // but do not remove the code, just in case
	m_buttonAddEV->Show( false );
	m_buttonRemoveEV->Show( false );


    // Gives a minimal size to the dialog, which allows displaying any page
    wxSize minsize;

    for( unsigned ii = 0; ii < m_pages.size(); ii++ )
    {
        wxSize size = m_pages[ii]->GetSizer()->CalcMin();
        minsize.x = std::max( minsize.x, size.x );
        minsize.y = std::max( minsize.y, size.y );
    }

    SetMinSize( minsize );
    SetPageSize( minsize );
    GetSizer()->SetSizeHints( this );
    Center();
}


WIZARD_FPLIB_TABLE::~WIZARD_FPLIB_TABLE()
{
    // Use this if you want to store kicad lib URL in pcbnew/cvpcb section config:
//    wxConfigBase* cfg = Kiface().KifaceSettings();
    // Use this if you want to store kicad lib URL in common section config:
    wxConfigBase* cfg = Pgm().CommonSettings();
    cfg->Write( KICAD_FPLIBS_URL_KEY, m_textCtrlGithubURL->GetValue() );

    m_last_plugin_choice = m_rbFpLibFormat->GetSelection();
    m_last_defaultpath_choice = m_rbPathManagement->GetSelection();

    cfg->Write( WIZARD_LAST_PLUGIN_KEY, m_last_plugin_choice );
    cfg->Write( WIZARD_LAST_PATHOPTION_KEY, m_last_defaultpath_choice );
}


void WIZARD_FPLIB_TABLE::initDlg( wxArrayString& aEnvVariableList )
{
    m_currLibDescr = NULL;

    SetBitmap( KiBitmap( wizard_add_fplib_icon_xpm ) );

    wxString msg;
    wxConfigBase* cfg = Pgm().CommonSettings();
    cfg->Read( KICAD_FPLIBS_URL_KEY, &msg );
    cfg->Read( WIZARD_LAST_PLUGIN_KEY, &m_last_plugin_choice );
    cfg->Read( WIZARD_LAST_PATHOPTION_KEY, &m_last_defaultpath_choice );

    if( msg.IsEmpty() )
        m_textCtrlGithubURL->SetValue( wxT( "http://github.com/KiCad/" ) );
    else
        m_textCtrlGithubURL->SetValue( msg );

    // KIGITHUB is frequently used (examples in docs, and other place)
    // So add it if it not yet in list, but if it is defined as env var
    // (note this env var is not hardcoded or existing in kicad sources)
    if( aEnvVariableList.Index( wxT("KIGITHUB"), false ) == wxNOT_FOUND )
    {
        //  Not yet in use in lib table, see in env. vars
        wxString evValue;
        wxGetEnv( wxT("KIGITHUB"), &evValue );

        //  Not yet in use in lib table, but it is defined in environment,
        // so add it, just in case
        if( ! evValue.IsEmpty() )
            aEnvVariableList.Add( wxT("KIGITHUB") );
    }

    m_rowPrjEnvVarPosition = 0;
    m_predefinedEnvVarCnt = aEnvVariableList.GetCount();

    for( int row = 0; row < m_predefinedEnvVarCnt; row++ )
    {
        if( GetEnvVarCount() <= row )
            m_gridEnvironmentVariablesList->AppendRows(1);

        m_gridEnvironmentVariablesList->SetCellValue( row, 0, aEnvVariableList[row] );

        if( aEnvVariableList[row] == PROJECT_VAR_NAME )
            m_rowPrjEnvVarPosition = row;

        wxString evValue;

        if( wxGetEnv( aEnvVariableList[row], &evValue ) )
            m_gridEnvironmentVariablesList->SetCellValue( row, 1, evValue );

        // All these env var are defined outside the wizard,
        // and cannot be modified in this dialog
        m_gridEnvironmentVariablesList->SetReadOnly( row, 0, true );
        m_gridEnvironmentVariablesList->SetReadOnly( row, 1, true );
    }

    m_gridEnvironmentVariablesList->Fit();

    m_buttonRemoveEV->Enable( GetEnvVarCount() > m_predefinedEnvVarCnt );
    m_gridEnvironmentVariablesList->AutoSizeColumns();

    m_rbFpLibFormat->SetSelection( m_last_plugin_choice );
    m_rbPathManagement->SetSelection( m_last_defaultpath_choice );

    wxCommandEvent event;
    updateFromPlugingChoice();
    OnPathManagementSelection( event );
}

int WIZARD_FPLIB_TABLE::HasGithubEnvVarCompatible()
{
    // Return true if at least one env var defines a url relative to github
    for( int row = 0; row < GetEnvVarCount(); row++ )
    {
        if( m_gridEnvironmentVariablesList->GetCellValue(
                wxGridCellCoords( row, 1 ) ).Lower().StartsWith( "http" ) )
            return row;
    }

    return -1;
}


bool WIZARD_FPLIB_TABLE::ValidateOptions()
{
    // Some choices can be conficting or do not work.
    // Warn the user when this is the case
    wxString msg;

    if( GetSelectedEnvVarValue().IsEmpty() )
    {
        // PROJECT_PATH option cannot be used with empty local path
        if( m_rbPathManagement->GetSelection() == PROJECT_PATH )
        {
            msg = _("The project path is empty and this option is not valid.\n"
                    "Looks like you are running the wizard outside a project.");
            wxMessageBox( msg );
            return false;
        }
        else if( m_rbPathManagement->GetSelection() != ABSOLUTE_PATH )
        {
            wxMessageBox( wxString::Format(
                _("The default path defined by env var \"%s\" is empty.\nCannot use it"),
                GetChars( GetSelectedEnvVar() ) ) );
            return false;
        }
    }
    else
    {
        if( IsGithubPlugin() )
        {
            // Github plugin cannot be used with local path; Need absolute path or valid URL
            if( !GetSelectedEnvVarValue().Lower().StartsWith( "http" ) )
            {
                msg = _("Github Plugin uses a valid Internet URL starting by http.\n"
                        "Cannot be used as URL");
                wxMessageBox( msg );
                return false;
            }
        }
        else
        {
            if( GetSelectedEnvVarValue().Lower().StartsWith( "http" ) )
            {
                msg = _("This default path looks strange.\n"
                        "Cannot be used for a file path");
                wxMessageBox( msg );
                return false;
            }
        }
    }

    // Other conficts: TODO

    return true;
}


void WIZARD_FPLIB_TABLE::OnPluginSelection( wxCommandEvent& event )
{
    updateFromPlugingChoice();
}

void WIZARD_FPLIB_TABLE::updateFromPlugingChoice()
{
    // update dialog options and widgets depending on a plugin choice
    // Project path has no sense for GITHUB_PLUGIN
    bool enablePrjPathOpt = not IsGithubPlugin();

    // Project path cannot be used if unknown
    if( m_gridEnvironmentVariablesList->GetCellValue(
                                wxGridCellCoords( m_rowPrjEnvVarPosition, 1 ) ).IsEmpty() )
        enablePrjPathOpt = false;

    m_rbPathManagement->Enable( PROJECT_PATH, enablePrjPathOpt );

    // Sometimes only the choice "absolute path" is allowed;
    // Force this choice, at least make it the default choice
    bool force_absolute_path = false;

    // For github plugin, the project path is not allowed
    if( ( m_rbPathManagement->GetSelection() == PROJECT_PATH ) && !enablePrjPathOpt )
        force_absolute_path = true;

    // For github plugin, at least one github compatible path must exist
    // If no github path, force absolute path
    int first_github_envvar = HasGithubEnvVarCompatible();
    if( IsGithubPlugin() )
    {
        if( first_github_envvar < 0 )
            force_absolute_path = true;
        else if( !GetSelectedEnvVarValue().StartsWith( "http" ) )
                m_gridEnvironmentVariablesList->SelectRow( first_github_envvar );

    }

    if( force_absolute_path )
        m_rbPathManagement->SetSelection( ABSOLUTE_PATH );
}


void WIZARD_FPLIB_TABLE::OnPathManagementSelection( wxCommandEvent& event )
{
    // Disable irrevant options, and enable others.
    int row_count = GetEnvVarCount();

    switch( m_rbPathManagement->GetSelection() )
    {
        case PROJECT_PATH:          // Choice = path relative to the project
            m_gridEnvironmentVariablesList->Enable( true );
            m_buttonAddEV->Enable( false );
            m_gridEnvironmentVariablesList->ShowRow( PROJECT_PATH );

            for( int row = 0; row < row_count; row++ )
            {
                if( row == PROJECT_PATH )
                    continue;

                m_gridEnvironmentVariablesList->HideRow( row );
            }
            break;

        case ENV_VAR_PATH:          // Choice = path relative to env var
            m_gridEnvironmentVariablesList->Enable( true );
            m_buttonAddEV->Enable( true );

            for( int row = 0; row < row_count; row++ )
                m_gridEnvironmentVariablesList->ShowRow( row );

            break;

        case ABSOLUTE_PATH:          // Choice = path relative to the project
            m_gridEnvironmentVariablesList->Enable( false );
            m_buttonAddEV->Enable( false );
            break;
    }
}

void WIZARD_FPLIB_TABLE::OnAddEVariable( wxCommandEvent& event )
{
    m_gridEnvironmentVariablesList->AppendRows( 1 );
    m_gridEnvironmentVariablesList->AutoSizeColumns();
    m_buttonRemoveEV->Enable( GetEnvVarCount() > m_predefinedEnvVarCnt );
    m_gridEnvironmentVariablesList->SetGridCursor( GetEnvVarCount()-1, 0 );
}

void WIZARD_FPLIB_TABLE::OnRemoveEVariable( wxCommandEvent& event )
{
    wxArrayInt selectedRows	= m_gridEnvironmentVariablesList->GetSelectedRows();
    int row_cursor = m_gridEnvironmentVariablesList->GetGridCursorRow();

    if( selectedRows.size() == 0 && row_cursor >= 0 )
        selectedRows.Add( row_cursor );

    std::sort( selectedRows.begin(), selectedRows.end() );

    for( int ii = selectedRows.GetCount()-1; ii >= 0; ii-- )
    {
        int row = selectedRows[ii];

        // don't remove them env var. which are already existing in lib table
        if( row > m_predefinedEnvVarCnt-1 )
            m_gridEnvironmentVariablesList->DeleteRows( row, 1 );
    }

    m_gridEnvironmentVariablesList->SelectRow( m_gridEnvironmentVariablesList->GetGridCursorRow() );
    m_buttonRemoveEV->Enable( GetEnvVarCount() > m_predefinedEnvVarCnt );
}

void WIZARD_FPLIB_TABLE::OnSelectEnvVarCell( wxGridEvent& event )
{
    // Ensure the selected row is also the row which have the focus.
    // useful when the user want to delete a row, and select it by the mouse
    m_gridEnvironmentVariablesList->SelectRow( event.GetRow() );
}

wxString WIZARD_FPLIB_TABLE::GetSelectedEnvVar()
{
    wxString envVar;
    wxArrayInt selectedRows	= m_gridEnvironmentVariablesList->GetSelectedRows();
    int row   = selectedRows.GetCount() ? selectedRows[0] :
                m_gridEnvironmentVariablesList->GetGridCursorRow();

    switch( m_rbPathManagement->GetSelection() )
    {
        case ENV_VAR_PATH:    // Choice = path relative to env var
            envVar = m_gridEnvironmentVariablesList->GetCellValue(
                                wxGridCellCoords( row, 0 ) );
            break;

        case PROJECT_PATH:    // Choice = path relative to the project
            envVar = PROJECT_VAR_NAME;
            break;

        case ABSOLUTE_PATH:    // Choice = absolute path
        default:
            break;
    }

    return envVar;
}


wxString WIZARD_FPLIB_TABLE::GetSelectedEnvVarValue()
{
    wxString envVarValue;
    wxArrayInt selectedRows	= m_gridEnvironmentVariablesList->GetSelectedRows();
    int row   = selectedRows.GetCount() ? selectedRows[0] :
                m_gridEnvironmentVariablesList->GetGridCursorRow();

    switch( m_rbPathManagement->GetSelection() )
    {
        case ENV_VAR_PATH:    // Choice = path relative tp env var
            envVarValue = m_gridEnvironmentVariablesList->GetCellValue(
                                wxGridCellCoords( row, 1 ) );
            break;

        case PROJECT_PATH:    // Choice = path relative to the project
            envVarValue = m_gridEnvironmentVariablesList->GetCellValue(
                                wxGridCellCoords( m_rowPrjEnvVarPosition, 1 ) );
            break;

        case ABSOLUTE_PATH:    // Choice = absolute path
        default:
            break;
    }

    return envVarValue;
}

void WIZARD_FPLIB_TABLE::OnPageChanged( wxWizardEvent& event )
{
    if( GetCurrentPage() == m_pages[2] )
        setLastPage();
    else if( GetCurrentPage() == m_pages[1] )
        setSecondPage();
}

void WIZARD_FPLIB_TABLE::OnPageChanging( wxWizardEvent& event )
{
    if( !( GetCurrentPage() == m_pages[1] && event.GetDirection() ) )
        return;

    if( ! ValidateOptions() )
    {
        event.Veto();
        return;
    }

    if( ( m_rbPathManagement->GetSelection() != ABSOLUTE_PATH ) &&
        ( IsGithubPlugin() ) )
    {
        wxURI uri( GetSelectedEnvVarValue() );

        // We cannot use wxURL to test the validity of the url, because
        // wxURL does not know https protocol we are using, and aways returns
        // error for url starting by https
        bool badurl = !uri.HasPath();

        if( badurl )
        {
            wxMessageBox( wxString::Format(
                _("The URL defined by env var \"%s\" is an incorrect URL.\nCannot use it"),
                GetChars( GetSelectedEnvVar() ) ) );
            event.Veto();
        }
    }
}

bool WIZARD_FPLIB_TABLE::setSecondPage()
{
    // Init parameters for the second wizard page: this is only
    // the current library description.
    updateFromPlugingChoice();

    delete m_currLibDescr;
    m_currLibDescr = NULL;

    switch( m_rbFpLibFormat->GetSelection() )
    {
        case 0:     // Kicad lib type
            m_currLibDescr = new LIB_DESCR_KICAD;
            break;

        case 1:     // Github lib type
            m_currLibDescr = new LIB_DESCR_GITHUB;
            break;

        case 2:     // Legacy lib type
            m_currLibDescr = new LIB_DESCR_LEGACY;
            break;

        case 3:     // Eagle V6 lib type
            m_currLibDescr = new LIB_DESCR_EAGLE;
            break;

        case 4:     // Geda lib type
            m_currLibDescr = new LIB_DESCR_GEDA;
            break;
    }

    return m_currLibDescr!= NULL;
}

bool WIZARD_FPLIB_TABLE::setLastPage()     // Init prms for the last wizard page
{
    // Update texts in last wizard page
    m_textPluginType->SetLabel( m_rbFpLibFormat->GetStringSelection() );

    switch( m_rbPathManagement->GetSelection() )
    {
        case ENV_VAR_PATH:    // Choice = path relative env var
        case PROJECT_PATH:    // Choice = path relative to the project
            m_currLibDescr->m_EnvVarName = GetSelectedEnvVar();
            m_currLibDescr->m_DefaultPath = GetSelectedEnvVarValue();
            m_currLibDescr->m_IsAbsolutePath = false;

            m_textOption->SetLabel( wxString::Format( wxT("%s (%s)"),
                m_rbPathManagement->GetStringSelection().GetData(),
                GetSelectedEnvVar().GetData() ) );

            m_textPath->SetLabel( GetSelectedEnvVarValue() );
            break;

        case ABSOLUTE_PATH:    // Choice = absolute path
            m_currLibDescr->m_IsAbsolutePath = true;

            m_textOption->SetLabel( m_rbPathManagement->GetStringSelection() );

            if( IsGithubPlugin() )
                m_textPath->SetLabel( _("Full URL") );
            else
                m_textPath->SetLabel( _("Full filename") );
            break;
    }

    return true;
}



void WIZARD_FPLIB_TABLE::OnAddFpLibs( wxCommandEvent& event )
{
    if( m_currLibDescr->m_IsFile )
        selectLibsFiles();
    else if( m_currLibDescr->m_IsGitHub )
        selectLibsGithub();
    else
        selectLibsFolders();

    m_gridFpListLibs->SetGridCursor( GetLibsCount()-1, 0 );
    m_gridFpListLibs->SelectRow( GetLibsCount()-1 );
}

void WIZARD_FPLIB_TABLE::selectLibsFiles()     // select a set of library files
{
    wxString msk = wxT("*.") + m_currLibDescr->m_Ext;

    wxFileDialog dlg( this, _("Select Library Files"), m_currLibDescr->m_DefaultPath,
                      wxEmptyString, msk,
                      wxFD_DEFAULT_STYLE|wxFD_FILE_MUST_EXIST|wxFD_MULTIPLE );

    dlg.ShowModal();

    wxArrayString filepaths;
    dlg.GetPaths( filepaths );

    // Create the nickname: currently make it from the filename
    wxArrayString nicknames;
    wxFileName fn;

    for( unsigned ii = 0; ii < filepaths.GetCount(); ii++ )
    {
        fn = filepaths[ii];
        nicknames.Add( fn.GetName() );

        if( m_currLibDescr->m_IsAbsolutePath || m_currLibDescr->m_DefaultPath.IsEmpty() )
        {
            filepaths[ii] = fn.GetPathWithSep();
        }
        else
        {
            if( ! fn.MakeRelativeTo( m_currLibDescr->m_DefaultPath ) )
                filepaths[ii] = fn.GetFullPath();
            else
                filepaths[ii].Printf( wxT("${%s}%c%s"),
                    GetChars( m_currLibDescr->m_EnvVarName ),
                    fn.GetPathSeparator(),
                    GetChars( fn.GetFullPath() ) );
        }
#ifdef __WINDOWS__
        // We store paths using Unix notation, which also works fine on Windows
        filepaths[ii].Replace( wxT("\\"), wxT("/") );
#endif
    }

    populateLibList( nicknames, filepaths, m_currLibDescr->m_PluginName );
    m_gridFpListLibs->AutoSizeColumns();
}


void WIZARD_FPLIB_TABLE::populateLibList( const wxArrayString& aNickNames,
                                          const wxArrayString& aPaths,
                                          const wxString& aPluginName )
{
    if( aPaths.GetCount() <= 0 )
        return;

    // Ensure there is room for selected libs
    int first_row = m_gridFpListLibs->GetTable()->GetRowsCount();
    m_gridFpListLibs->AppendRows( aPaths.GetCount() );

    // Populates the library list
    for( unsigned ii = 0; ii < aPaths.GetCount(); ii++ )
    {
        int jj = first_row + ii;
        // Add the nickname: currently make it from filename
        m_gridFpListLibs->SetCellValue( jj, 0, aNickNames[ii] );
        // Add the full path:
        m_gridFpListLibs->SetCellValue( jj, 1, aPaths[ii] );
        // Add the plugin name:
        m_gridFpListLibs->SetCellValue( jj, 2, aPluginName );
        m_gridFpListLibs->SetReadOnly( jj, 2, true );
    }

    m_gridFpListLibs->Fit();
}


// A helper dialog to show and select a set of directories
class DIALOG_SELECT_DIRLIST : public DIALOG_SELECT_DIRLIST_BASE
{
public:
    DIALOG_SELECT_DIRLIST( wxWindow* parent,
                              const wxString& aDefaultPath ):
                DIALOG_SELECT_DIRLIST_BASE( parent, wxID_ANY )
    {
        if( !aDefaultPath.IsEmpty() )
            m_dirCtrl->SetPath( aDefaultPath );

        Layout();
        GetSizer()->Fit( this );
        GetSizer()->SetSizeHints(this);
        Centre();
    }

    ~DIALOG_SELECT_DIRLIST() {};

    void GetPaths( wxArrayString& aPaths ) { m_dirCtrl->GetPaths( aPaths ); }
};

void WIZARD_FPLIB_TABLE::selectLibsFolders()   // select a set of library folders
{
    DIALOG_SELECT_DIRLIST dlg( this, m_currLibDescr->m_DefaultPath );

    if( dlg.ShowModal() != wxID_OK )
        return;

    wxArrayString filepaths;

    dlg.GetPaths( filepaths );

    // Create the nickname: currently make it from the filename
    wxArrayString nicknames;
    wxFileName fn;

    for( unsigned ii = 0; ii < filepaths.GetCount(); ii++ )
    {
        fn = filepaths[ii];
        nicknames.Add( fn.GetName() );

        fn.AssignDir( filepaths[ii] );

        if( m_currLibDescr->m_IsAbsolutePath || m_currLibDescr->m_DefaultPath.IsEmpty() )
        {
            filepaths[ii] = fn.GetFullPath();
        }
        else
        {
            if( ! fn.MakeRelativeTo( m_currLibDescr->m_DefaultPath ) )
                filepaths[ii] = fn.GetFullPath();
            else
                filepaths[ii].Printf( wxT("${%s}%c%s"),
                    GetChars( m_currLibDescr->m_EnvVarName ),
                    fn.GetPathSeparator(),
                    GetChars( fn.GetFullPath() ) );
        }
#ifdef __WINDOWS__
        // We store paths using Unix notation, which also works fine on Windows
        filepaths[ii].Replace( wxT("\\"), wxT("/") );
#endif
        // Remove trailing path separator, if any.
        if( filepaths[ii].EndsWith( wxT("/") ) )
            filepaths[ii].RemoveLast();
    }

    populateLibList( nicknames, filepaths, m_currLibDescr->m_PluginName );
}

#ifdef KICAD_USE_WEBKIT

// A helper function to run the wen viewer (see webviewer.cpp)
extern int RunWebViewer( wxWindow * aParent, const wxString& aUrlOnStart,
                         wxArrayString* aUrlListSelection = NULL );
#endif

void WIZARD_FPLIB_TABLE::selectLibsGithub()    // select a set of library on Github
{
    // A string array to store the URLs selected from the web viewer:
    wxArrayString urls;

    // Run the web viewer and open the default URL: the default path
    // or our github library repos
    wxString defaultURL = m_currLibDescr->m_DefaultPath;

    if( defaultURL.IsEmpty() )
        defaultURL = wxT( "https://github.com/KiCad" );
#ifdef KICAD_USE_WEBKIT
    RunWebViewer( this, defaultURL, &urls );
#else
    urls.Add( defaultURL + wxT("newlibname.pretty") );
#endif

    // Create the nickname: currently make it from the url
    wxArrayString filepaths;
    wxArrayString nicknames;

    for( unsigned ii = 0; ii < urls.GetCount(); ii++ )
    {
        wxString urlstring( urls[ii] );

        wxURI uri( urlstring );

        // We cannot use wxURL to test the validity of the url, because
        // wxURL does not know https protocol we are using, and aways returns
        // error for URLs starting by https. Hope this test is enough
        if( uri.HasPath() )
            nicknames.Add( uri.GetPath().AfterLast( '/').BeforeLast( '.' ) );
        else
            continue;   // Should not happen: bad URL

        if( m_currLibDescr->m_IsAbsolutePath ||
            m_currLibDescr->m_DefaultPath.IsEmpty() )
        {
            filepaths.Add( urls[ii] );  // use the full URL
        }
        else
        {
            wxString shortURI;
            if( urls[ii].Lower().StartsWith(
                        m_currLibDescr->m_DefaultPath.Lower(), &shortURI ) )
            {
                shortURI.Prepend( wxT("${") + m_currLibDescr->m_EnvVarName + wxT("}") );
                filepaths.Add( shortURI );
            }
            else    //  keep the full URL
                filepaths.Add( urls[ii] );   // use the full URL
        }
    }

    populateLibList( nicknames, filepaths, m_currLibDescr->m_PluginName );
}


void WIZARD_FPLIB_TABLE::OnRemoveFpLibs( wxCommandEvent& event )
{
    wxArrayInt selectedRows	= m_gridFpListLibs->GetSelectedRows();
    int row_cursor = m_gridFpListLibs->GetGridCursorRow();

    if( selectedRows.size() == 0 && row_cursor >= 0 )
        selectedRows.Add( row_cursor );

    std::sort( selectedRows.begin(), selectedRows.end() );

    for( int ii = selectedRows.GetCount()-1; ii >= 0; ii-- )
    {
        int row = selectedRows[ii];
        m_gridFpListLibs->DeleteRows( row, 1 );
    }

    m_gridFpListLibs->SelectRow( m_gridFpListLibs->GetGridCursorRow() );
}
