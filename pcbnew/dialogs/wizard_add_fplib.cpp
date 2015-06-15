/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 * Copyright (C) 2014-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @brief Wizard for selecting footprint libraries consisting of 4 steps:
 * - select source (Github/local files)
 * - pick libraries
 * - present a review of libraries (including validation)
 * - select scope (global/project)
 */

#include <wx/wx.h>
#include <wx/uri.h>
#include <wx/progdlg.h>

#include <pgm_base.h>
#include <project.h>
#include <wizard_add_fplib.h>
#include <fp_lib_table.h>
#include <confirm.h>

#include <class_module.h>

#ifdef BUILD_GITHUB_PLUGIN
#include <../github/github_getliblist.h>
#endif

// a key to store the default Kicad Github libs URL
#define KICAD_FPLIBS_URL_KEY wxT( "kicad_fplib_url" )

// Filters for the file picker
static const int FILTER_COUNT = 4;
static const struct
{
    wxString m_Description;
    wxString m_Extension;
    bool m_IsFile;
    IO_MGR::PCB_FILE_T m_Plugin;
} fileFilters[FILTER_COUNT] =
{
    { "KiCad (*.pretty folders)",      ".pretty", false,   IO_MGR::KICAD },
    { "Eagle 6.x (*.lbr)",             ".lbr",    true,    IO_MGR::EAGLE },
    { "KiCad legacy (*.mod)",          ".mod",    true,    IO_MGR::LEGACY },
    { "Geda (folder with *.fp files)", "",        false,   IO_MGR::GEDA_PCB },
};


// Returns the filter string for the file picker
static wxString getFilterString()
{
    wxString filterInit = _( "All supported library formats|" );
    wxString filter;

    for( int i = 0; i < FILTER_COUNT; ++i )
    {
        // Init part
        if( i != 0 )
            filterInit += ";";

        filterInit += "*" + fileFilters[i].m_Extension;

        // Rest of the filter string
        filter += "|" + fileFilters[i].m_Description + "|*" + fileFilters[i].m_Extension;
    }

    return filterInit + filter;
}


// Tries to guess the plugin type basing on the path
static boost::optional<IO_MGR::PCB_FILE_T> getPluginType( const wxString& aPath )
{
    if( ( aPath.StartsWith( "http://" ) || aPath.StartsWith( "https://" ) ) && aPath.EndsWith( ".pretty" ) )
        return boost::optional<IO_MGR::PCB_FILE_T>( IO_MGR::GITHUB );

    wxFileName file( aPath );

    for( int i = 0; i < FILTER_COUNT; ++i )
    {
        if( aPath.EndsWith( fileFilters[i].m_Extension ) &&
                file.FileExists() == fileFilters[i].m_IsFile )
            return boost::optional<IO_MGR::PCB_FILE_T>( fileFilters[i].m_Plugin );
    }

    return boost::optional<IO_MGR::PCB_FILE_T>();
}


// Checks if a filename fits specific filter
static bool passesFilter( const wxString& aFileName, int aFilterIndex )
{
    wxASSERT( aFilterIndex <= FILTER_COUNT );
    wxFileName file( aFileName );

    if( aFilterIndex == 0 )         // any supported library format
    {
        boost::optional<IO_MGR::PCB_FILE_T> result = getPluginType( aFileName );
        return ( result ? true : false );
    }
    else
    {
        if( aFileName.EndsWith( fileFilters[aFilterIndex - 1].m_Extension ) &&
                file.FileExists() == fileFilters[aFilterIndex - 1].m_IsFile )
            return true;
    }

    return false;
}


WIZARD_FPLIB_TABLE::LIBRARY::LIBRARY( const wxString& aPath, const wxString& aDescription ) :
    m_path( aPath ), m_description( aDescription ), m_status( NOT_CHECKED )
{
    m_plugin = getPluginType( aPath );
}


bool WIZARD_FPLIB_TABLE::LIBRARY::Test()
{
    if( !m_plugin )
    {
        m_status = LIBRARY::INVALID;
        return false;
    }

    PLUGIN* p = IO_MGR::PluginFind( *m_plugin );
    wxArrayString footprints;

    if( !p )
    {
        m_status = LIBRARY::INVALID;
        return false;
    }

    try
    {
        footprints = p->FootprintEnumerate( m_path );
    }
    catch( IO_ERROR& e )
    {
        m_status = LIBRARY::INVALID;
        return false;
    }

    if( footprints.GetCount() == 0 )
    {
        m_status = LIBRARY::INVALID;
        return false;
    }

    m_status = LIBRARY::OK;
    return true;
}


wxString WIZARD_FPLIB_TABLE::LIBRARY::GetPluginName() const
{
    if( !m_plugin )
        return _( "UNKNOWN" );

    switch( *m_plugin )
    {
        case IO_MGR::LEGACY:
            return wxT( "Legacy" );

        case IO_MGR::KICAD:
            return wxT( "KiCad" );

        case IO_MGR::EAGLE:
            return wxT( "Eagle" );

        case IO_MGR::GEDA_PCB:
            return wxT( "Geda-PCB" );

        case IO_MGR::GITHUB:
            return wxT( "Github" );

        default:
            return _( "UNKNOWN" );
    }

    /*PLUGIN* p = IO_MGR::PluginFind( *m_plugin );

    if( !p )
        return _( "UNKNOWN" );

    return p->PluginName();*/
}


wxString WIZARD_FPLIB_TABLE::LIBRARY::GetRelativePath( const wxString& aBase, const wxString& aSubstitution ) const
{
    wxFileName libPath( m_path );

    // Check if the library path belongs to the project folder
    if( libPath.MakeRelativeTo( aBase ) && !libPath.GetFullPath().StartsWith( ".." ) )
    {
        return wxString( aSubstitution + "/" + libPath.GetFullPath() );
    }

    // Probably on another drive, so the relative path will not work
    return wxEmptyString;
}


wxString WIZARD_FPLIB_TABLE::LIBRARY::GetAutoPath( LIB_SCOPE aScope ) const
{
    const wxString& global_env = FP_LIB_TABLE::GlobalPathEnvVariableName();
    const wxString& project_env = PROJECT_VAR_NAME;
    const wxString& github_env( "KIGITHUB" );

    wxString rel_path;

    // KISYSMOD check
    rel_path = replaceEnv( global_env );

    if( !rel_path.IsEmpty() )
        return rel_path;

    // KIGITHUB check
    rel_path = replaceEnv( github_env, false );

    if( !rel_path.IsEmpty() )
        return rel_path;

    // KIPRJMOD check
    if( aScope == PROJECT )
    {
        rel_path = replaceEnv( project_env );

        if( !rel_path.IsEmpty() )
            return rel_path;
    }

    // Return the full path
    return m_path;
}


wxString WIZARD_FPLIB_TABLE::LIBRARY::GetDescription() const
{
    if( !m_description.IsEmpty() )
        return m_description;

    wxFileName filename( m_path );
    return filename.GetName();
}


wxString WIZARD_FPLIB_TABLE::LIBRARY::replaceEnv( const wxString& aEnvVar, bool aFilePath ) const
{
    wxString env_path;

    if( !wxGetEnv( aEnvVar, &env_path ) )
        return wxEmptyString;

    //return GetRelativePath( m_path, wxString( "$(" + aEnvVar + ")" ) );

    wxString result( m_path );

    if( result.Replace( env_path, wxString( "$(" + aEnvVar + ")" ) ) )
        return result;

    return wxEmptyString;
}


WIZARD_FPLIB_TABLE::WIZARD_FPLIB_TABLE( wxWindow* aParent ) :
    WIZARD_FPLIB_TABLE_BASE( aParent ), m_welcomeDlg( m_pages[0] ),
    m_fileSelectDlg( m_pages[1] ), m_githubListDlg( m_pages[2] ),
    m_reviewDlg( m_pages[3] ), m_targetDlg( m_pages[4] ), m_selectedFilter( 0 )
{
    m_filePicker->SetFilter( getFilterString() );

    // Initialize default download dir
    wxString default_path;
    wxGetEnv( FP_LIB_TABLE::GlobalPathEnvVariableName(), &default_path );
    setDownloadDir( default_path );
    m_filePicker->SetPath( default_path );

    // Restore the Github url
    wxString githubUrl;
    wxConfigBase* cfg = Pgm().CommonSettings();
    cfg->Read( KICAD_FPLIBS_URL_KEY, &githubUrl );

    if( githubUrl.IsEmpty() )
        githubUrl = wxT( "https://github.com/KiCad" );

    SetGithubURL( githubUrl );

    // Give the minimal size to the dialog, which allows displaying any page
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

    if( !m_radioAddGithub->GetValue() && !m_radioAddLocal->GetValue() )
        m_radioAddLocal->SetValue( true );

    setupDialogOrder();
    updateGithubControls();

    Connect( wxEVT_RADIOBUTTON, wxCommandEventHandler( WIZARD_FPLIB_TABLE::OnSourceCheck ), NULL, this );
    Connect( wxEVT_DIRCTRL_SELECTIONCHANGED, wxCommandEventHandler( WIZARD_FPLIB_TABLE::OnSelectFiles ), NULL, this );
    Connect( wxEVT_CHECKLISTBOX, wxCommandEventHandler( WIZARD_FPLIB_TABLE::OnCheckGithubList ), NULL, this );
}


WIZARD_FPLIB_TABLE::~WIZARD_FPLIB_TABLE()
{
    // Use this if you want to store kicad lib URL in pcbnew/cvpcb section config:
//    wxConfigBase* cfg = Kiface().KifaceSettings();

    // Use this if you want to store kicad lib URL in common section config:
    wxConfigBase* cfg = Pgm().CommonSettings();
    cfg->Write( KICAD_FPLIBS_URL_KEY, GetGithubURL() );
}


WIZARD_FPLIB_TABLE::LIB_SOURCE WIZARD_FPLIB_TABLE::GetLibSource() const
{
    if( m_radioAddGithub->GetValue() )
        return GITHUB;

    wxASSERT( m_radioAddLocal->GetValue() );

    return LOCAL;
}


WIZARD_FPLIB_TABLE::LIB_SCOPE WIZARD_FPLIB_TABLE::GetLibScope() const
{
    if( m_radioGlobal->GetValue() )
        return GLOBAL;

    wxASSERT( m_radioProject->GetValue() );

    return PROJECT;
}


void WIZARD_FPLIB_TABLE::OnPageChanged( wxWizardEvent& aEvent )
{
    SetBitmap( KiBitmap( wizard_add_fplib_icon_xpm ) );
    enableNext( true );

#ifdef BUILD_GITHUB_PLUGIN
    if( GetCurrentPage() == m_githubListDlg )
        setupGithubList();
    else
#endif
    if( GetCurrentPage() == m_fileSelectDlg )
        setupFileSelect();
    else if( GetCurrentPage() == m_reviewDlg )
        setupReview();
}


void WIZARD_FPLIB_TABLE::OnSelectFiles( wxCommandEvent& aEvent )
{
    int filterIdx = m_filePicker->GetFilterIndex();

    if( m_selectedFilter != filterIdx )
    {
        m_selectedFilter = filterIdx;

        // Process the event again, as in the first iteration we cannot get the list of selected items
        wxCommandEvent ev( wxEVT_DIRCTRL_SELECTIONCHANGED );
        AddPendingEvent( ev );
        return;
    }

    enableNext( checkFiles() );
}


void WIZARD_FPLIB_TABLE::OnCheckGithubList( wxCommandEvent& aEvent )
{
    wxArrayInt dummy;

    enableNext( m_checkListGH->GetCheckedItems( dummy ) > 0 );
}


void WIZARD_FPLIB_TABLE::OnSourceCheck( wxCommandEvent& aEvent )
{
    updateGithubControls();
    setupDialogOrder();
}


void WIZARD_FPLIB_TABLE::OnSelectAllGH( wxCommandEvent& aEvent )
{
    for( unsigned int i = 0; i < m_checkListGH->GetCount(); ++i )
        m_checkListGH->Check( i, true );

    // The list might be empty, e.g. in case of download error
    wxArrayInt dummy;
    enableNext( m_checkListGH->GetCheckedItems( dummy ) > 0 );
}


void WIZARD_FPLIB_TABLE::OnUnselectAllGH( wxCommandEvent& aEvent )
{
    for( unsigned int i = 0; i < m_checkListGH->GetCount(); ++i )
        m_checkListGH->Check( i, false );

    enableNext( false );
}


void WIZARD_FPLIB_TABLE::OnChangeSearch( wxCommandEvent& aEvent )
{
    wxString searchPhrase = m_searchCtrlGH->GetValue().Lower();

    // Store the current selection
    wxArrayInt checkedIndices;
    m_checkListGH->GetCheckedItems( checkedIndices );
    wxArrayString checkedStrings;

    for( unsigned int i = 0; i < checkedIndices.GetCount(); ++i )
        checkedStrings.Add( m_checkListGH->GetString( checkedIndices[i] ).AfterLast( '/' ) );

    m_checkListGH->Clear();

    // Rebuild the list, putting the matching entries on the top
    int matching = 0;   // number of entries matching the search phrase
    for( unsigned int i = 0; i < m_githubLibs.GetCount(); ++i )
    {
        const wxString& lib = m_githubLibs[i].AfterLast( '/' );
        bool wasChecked = ( checkedStrings.Index( lib ) != wxNOT_FOUND );
        int insertedIdx = -1;

        if( !searchPhrase.IsEmpty() && lib.Lower().Contains( searchPhrase ) )
        {
            insertedIdx = m_checkListGH->Insert( lib, matching++ );
            m_checkListGH->SetSelection( insertedIdx );
        }
        else
            insertedIdx = m_checkListGH->Append( lib );

        if( wasChecked )
            m_checkListGH->Check( insertedIdx );
    }

    if( !m_checkListGH->IsEmpty() )
        m_checkListGH->EnsureVisible( 0 );
}


void WIZARD_FPLIB_TABLE::OnWizardFinished( wxWizardEvent& aEvent )
{
#ifdef BUILD_GITHUB_PLUGIN
    // Shall we download a localy copy of the libraries
    if( GetLibSource() == GITHUB && m_downloadGithub->GetValue() )
    {
        wxString error;
        wxArrayString libs;

        // Prepare a list of libraries to download
        for( std::vector<LIBRARY>::const_iterator it = m_libraries.begin();
                it != m_libraries.end(); ++it )
        {
            wxASSERT( it->GetPluginType() == IO_MGR::GITHUB );

            if( it->GetStatus() != LIBRARY::INVALID )
                libs.Add( it->GetAbsolutePath() );
        }

        if( !downloadGithubLibsFromList( libs, &error ) )
        {
            DisplayError( this, error );
            m_libraries.clear();
        }
        else
        {
            // Now libraries are stored locally, so update the paths to point to the download folder
            for( std::vector<LIBRARY>::iterator it = m_libraries.begin();
                    it != m_libraries.end(); ++it )
            {
                wxString path = it->GetAbsolutePath();
                path.Replace( GetGithubURL(), getDownloadDir() );
                it->setPath( path );
                it->setPluginType( IO_MGR::KICAD );
            }
        }
    }
#endif
}


void WIZARD_FPLIB_TABLE::OnBrowseButtonClick( wxCommandEvent& aEvent )
{
    wxString path = getDownloadDir();

    path = wxDirSelector( _("Choose a folder to save the downloaded libraries" ),
                             path, 0, wxDefaultPosition, this );

    if( !path.IsEmpty() && wxDirExists( path ) )
    {
        setDownloadDir( path );
        updateGithubControls();
    }
}


void WIZARD_FPLIB_TABLE::OnCheckSaveCopy( wxCommandEvent& aEvent )
{
    updateGithubControls();
}


bool WIZARD_FPLIB_TABLE::checkFiles() const
{
    // Get current selection (files & directories)
    wxArrayString candidates;
    m_filePicker->GetPaths( candidates );

    // Workaround, when you change filters "/" is automatically selected
    int slash_index = candidates.Index( "/", true, true );
    if( slash_index != wxNOT_FOUND )
        candidates.RemoveAt( slash_index, 1 );

    if( candidates.IsEmpty() )
        return false;

    // Verify all the files/folders comply to the selected library type filter
    for( unsigned int i = 0; i < candidates.GetCount(); ++i )
    {
        if( !passesFilter( candidates[i], m_filePicker->GetFilterIndex() ) )
            return false;
    }

    return true;
}


#ifdef BUILD_GITHUB_PLUGIN
void WIZARD_FPLIB_TABLE::getLibsListGithub( wxArrayString& aList )
{
    wxBeginBusyCursor();

    // Be sure there is no trailing '/' at the end of the repo name
    wxString git_url = m_textCtrlGithubURL->GetValue();

    if( git_url.EndsWith( wxT( "/" ) ) )
    {
        git_url.RemoveLast();
        m_textCtrlGithubURL->SetValue( git_url );
    }

    GITHUB_GETLIBLIST getter( git_url );
    getter.GetLibraryList( aList );

    wxEndBusyCursor();
}


// Download the .pretty libraries found in aUrlLis and store them on disk
// in a master folder
bool WIZARD_FPLIB_TABLE::downloadGithubLibsFromList( wxArrayString& aUrlList,
                                                     wxString* aErrorMessage )
{
    // Display a progress bar to show the downlaod state
    wxProgressDialog pdlg( _( "Downloading libraries" ), wxEmptyString, aUrlList.GetCount() );

    // Download libs:
    for( unsigned ii = 0; ii < aUrlList.GetCount(); ii++ )
    {
        wxString& libsrc_name = aUrlList[ii];
        wxString libdst_name;

        // Extract the lib name from the full URL:
        wxURI url( libsrc_name );
        wxFileName fn( url.GetPath() );
        // Set our local path
        fn.SetPath( getDownloadDir() );
        libdst_name = fn.GetFullPath();

        if( !wxDirExists( libdst_name ) )
            wxMkdir( libdst_name );

        pdlg.Update( ii, libsrc_name );

        try
        {
            PLUGIN::RELEASER src( IO_MGR::PluginFind( IO_MGR::GITHUB ) );
            PLUGIN::RELEASER dst( IO_MGR::PluginFind( IO_MGR::KICAD ) );

            wxArrayString footprints = src->FootprintEnumerate( libsrc_name );

            for( unsigned i = 0;  i < footprints.size();  ++i )
            {
                std::auto_ptr<MODULE> m( src->FootprintLoad( libsrc_name, footprints[i] ) );
                dst->FootprintSave( libdst_name, m.get() );
                // m is deleted here by auto_ptr.
            }
        }
        catch( const IO_ERROR& ioe )
        {
            if( aErrorMessage )
                aErrorMessage->Printf( _( "Error:\n'%s'\nwhile downloading library:\n'%s'" ),
                                       GetChars( ioe.errorText ), GetChars( libsrc_name ) );
            return false;
        }
    }

    return true;
}


void WIZARD_FPLIB_TABLE::setupGithubList()
{
    // Enable 'Next' only if there is at least one library selected
    wxArrayInt checkedIndices;
    m_checkListGH->GetCheckedItems( checkedIndices );
    enableNext( checkedIndices.GetCount() > 0 );

    // Update only if necessary
    if( m_githubLibs.GetCount() == 0 )
        getLibsListGithub( m_githubLibs );

    m_searchCtrlGH->Clear();

    // Clear the review list so it will be reloaded
    m_libraries.clear();
    m_listCtrlReview->DeleteAllItems();
}
#endif /* BUILD_GITHUB_PLUGIN */


void WIZARD_FPLIB_TABLE::updateGithubControls()
{
#ifndef BUILD_GITHUB_PLUGIN
    m_radioAddGithub->Enable( false );
#endif

    // Disable inputs that have no meaning for the selected source
    bool githubEnabled = ( GetLibSource() == GITHUB );
    m_textCtrlGithubURL->Enable( githubEnabled );
    m_downloadGithub->Enable( githubEnabled );
    m_downloadDir->Enable( githubEnabled && wantLocalCopy() );
    m_btnBrowse->Enable( githubEnabled && wantLocalCopy() );

    bool valid = !( githubEnabled && wantLocalCopy() ) || wxFileName::IsDirWritable( getDownloadDir() );

    // Do not allow to go further unless there is a valid directory selected
    m_invalidDir->Show( !valid );
    enableNext( valid );
}


void WIZARD_FPLIB_TABLE::updateLibraries()
{
    // No need to update, the review list is ready
    if( m_listCtrlReview->GetItemCount() != 0 )
        return;

    switch( GetLibSource() )
    {
    case LOCAL:
    {
        wxArrayString libs;
        m_filePicker->GetPaths( libs );

        // Workaround, when you change filters "/" is automatically selected
        int slash_index = libs.Index( "/", true, true );
        if( slash_index != wxNOT_FOUND )
            libs.RemoveAt( slash_index, 1 );

        m_libraries.reserve( libs.GetCount() );

        for( unsigned int i = 0; i < libs.GetCount(); ++i )
            m_libraries.push_back( libs[i] );
    }
    break;

    case GITHUB:
    {
        wxArrayInt checkedLibs;
        m_checkListGH->GetCheckedItems( checkedLibs );

        m_libraries.reserve( checkedLibs.GetCount() );

        for( unsigned int i = 0; i < checkedLibs.GetCount(); ++i )
            m_libraries.push_back( GetGithubURL() + "/" + m_checkListGH->GetString( checkedLibs[i] ) );
    }
    break;

    default:
        wxASSERT( false );
        break;
    }
}


void WIZARD_FPLIB_TABLE::setupDialogOrder()
{
    // Alternate the wizard pages flow depending on the selected option
    switch( GetLibSource() )
    {
    case LOCAL:
        m_welcomeDlg->SetNext( m_fileSelectDlg );
        m_fileSelectDlg->SetPrev( m_welcomeDlg );

        m_fileSelectDlg->SetNext( m_reviewDlg );
        m_reviewDlg->SetPrev( m_fileSelectDlg );
        break;

    case GITHUB:
        m_welcomeDlg->SetNext( m_githubListDlg );
        m_githubListDlg->SetPrev( m_welcomeDlg );

        m_githubListDlg->SetNext( m_reviewDlg );
        m_reviewDlg->SetPrev( m_githubListDlg );
        break;

    default:
        wxASSERT( false );
        break;
    }
}


void WIZARD_FPLIB_TABLE::setupFileSelect()
{
    // Disable the button until something is selected
    enableNext( checkFiles() );

    // Clear the review list so it will be reloaded
    m_libraries.clear();
    m_listCtrlReview->DeleteAllItems();
}


void WIZARD_FPLIB_TABLE::setupReview()
{
    wxBeginBusyCursor();
    updateLibraries();

    int libTotalCount = m_libraries.size();
    int libCount = 0;
    bool validate = true;
    wxProgressDialog progressDlg( _( "Please wait..." ), _( "Validating libraries" ),
                                  libTotalCount, this,
                                  wxPD_APP_MODAL | wxPD_CAN_ABORT | wxPD_AUTO_HIDE );

    m_dvLibName->SetWidth( 280 );

    // Prepare the review list
    m_listCtrlReview->DeleteAllItems();

    for( std::vector<LIBRARY>::iterator it = m_libraries.begin(); it != m_libraries.end(); ++it )
    {
        wxVector<wxVariant> row;
        LIBRARY::STATUS status = it->GetStatus();

        // Check if the library contents is valid
        if( status == LIBRARY::NOT_CHECKED && validate )
        {
            it->Test();
            status = it->GetStatus();
        }

        row.push_back( wxVariant( it->GetDescription() ) );

        switch( it->GetStatus() )
        {
            case LIBRARY::NOT_CHECKED:
                row.push_back( wxVariant( _( "NOT CHECKED" ) ) );
                break;

            case LIBRARY::OK:
                row.push_back( wxVariant( _( "OK" ) ) );
                break;

            case LIBRARY::INVALID:
                row.push_back( wxVariant( _( "INVALID" ) ) );
                break;
        }

        row.push_back( wxVariant( it->GetPluginName() ) );

        m_listCtrlReview->AppendItem( row );

        ++libCount;
        if( !progressDlg.Update( libCount, wxString::Format( _( "Validating libraries %d/%d" ),
                                 libCount, libTotalCount ) ) )
            validate = false;
    }

    // The list should never be empty, but who knows?
    enableNext( m_listCtrlReview->GetItemCount() > 0 );

    wxEndBusyCursor();
}
