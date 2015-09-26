/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * Code derived from "wizard_add_fplib.cpp" ( author Maciej Suminski <maciej.suminski@cern.ch> )
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
 * @brief Wizard for selecting and dowloading D shapes libraries of footprints
 * consisting of 3 steps:
 * - select source and destination (Github URL and local folder)
 * - pick and select libraries
 * - download files
 */

#include <wx/wx.h>
#include <wx/uri.h>
#include <wx/dir.h>
#include <wx/progdlg.h>

#include <pgm_base.h>
#include <project.h>
#include <wizard_3DShape_Libs_downloader.h>
#include <confirm.h>
#include <3d_viewer.h>

#include <../github/github_getliblist.h>

// a key to store the default Kicad Github 3D libs URL
#define KICAD_3DLIBS_URL_KEY wxT( "kicad_3Dlib_url" )
#define KICAD_3DLIBS_LAST_DOWNLOAD_DIR wxT( "kicad_3Dlib_last_download_dir" )

#define DEFAULT_GITHUB_3DSHAPES_LIBS_URL \
    wxT( "https://github.com/KiCad/kicad-library/tree/master/modules/packages3d" )

void Invoke3DShapeLibsDownloaderWizard( wxTopLevelWindow* aParent )
{
    WIZARD_3DSHAPE_LIBS_DOWNLOADER wizard( aParent );
    wizard.RunWizard( wizard.GetFirstPage() );
}


WIZARD_3DSHAPE_LIBS_DOWNLOADER::WIZARD_3DSHAPE_LIBS_DOWNLOADER( wxWindow* aParent ) :
    WIZARD_3DSHAPE_LIBS_DOWNLOADER_BASE( aParent )
{
    m_welcomeDlg = m_pages[0];
    m_githubListDlg = m_pages[1];
    m_reviewDlg = m_pages[2];

    // Initialize default download dir (local target folder of 3D shapes libs)
    wxString default_path;
    wxGetEnv( KISYS3DMOD, &default_path );

    wxConfigBase* cfg = Pgm().CommonSettings();
    wxString tmp;
    cfg->Read( KICAD_3DLIBS_LAST_DOWNLOAD_DIR, &tmp, default_path );
    setDownloadDir( tmp );

    // Restore the Github 3D shapes libs url
    wxString githubUrl;
    cfg->Read( KICAD_3DLIBS_URL_KEY, &githubUrl );

    if( githubUrl.IsEmpty() )
        githubUrl = DEFAULT_GITHUB_3DSHAPES_LIBS_URL;

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

    setupDialogOrder();
    updateGithubControls();

    // When starting m_textCtrlGithubURL has the focus, and the text is selected,
    // and not fully visible.
    // Forcing deselection does not work, at least on W7 with wxWidgets 3.0.2
    // So (and also because m_textCtrlGithubURL and m_downloadDir are rarely modified
    // the focus is given to an other widget.
    m_hyperlinkGithubKicad->SetFocus();

    Connect( wxEVT_RADIOBUTTON, wxCommandEventHandler( WIZARD_3DSHAPE_LIBS_DOWNLOADER::OnSourceCheck ), NULL, this );
    Connect( wxEVT_CHECKLISTBOX, wxCommandEventHandler( WIZARD_3DSHAPE_LIBS_DOWNLOADER::OnCheckGithubList ), NULL, this );
}


WIZARD_3DSHAPE_LIBS_DOWNLOADER::~WIZARD_3DSHAPE_LIBS_DOWNLOADER()
{
    // Use this if you want to store kicad lib URL in pcbnew/cvpcb section config:
    // wxConfigBase* cfg = Kiface().KifaceSettings();

    // Use this if you want to store kicad lib URL in common section config:
    wxConfigBase* cfg = Pgm().CommonSettings();
    cfg->Write( KICAD_3DLIBS_URL_KEY, GetGithubURL() );
    cfg->Write( KICAD_3DLIBS_LAST_DOWNLOAD_DIR, getDownloadDir() );
}



void WIZARD_3DSHAPE_LIBS_DOWNLOADER::OnPageChanged( wxWizardEvent& aEvent )
{
    SetBitmap( KiBitmap( wizard_add_fplib_icon_xpm ) );
    enableNext( true );

    if( GetCurrentPage() == m_githubListDlg )
        setupGithubList();
    else if( GetCurrentPage() == m_reviewDlg )
        setupReview();
}


void WIZARD_3DSHAPE_LIBS_DOWNLOADER::OnCheckGithubList( wxCommandEvent& aEvent )
{
    wxArrayInt dummy;

    enableNext( m_checkList3Dlibnames->GetCheckedItems( dummy ) > 0 );
}


void WIZARD_3DSHAPE_LIBS_DOWNLOADER::OnSourceCheck( wxCommandEvent& aEvent )
{
    updateGithubControls();
    setupDialogOrder();
}

void WIZARD_3DSHAPE_LIBS_DOWNLOADER::OnGridLibReviewSize( wxSizeEvent& event )
{
    // Adjust the width of the column 1 afo m_gridLibReview (library names) to the
    // max available width.
    int gridwidth = m_gridLibReview->GetClientSize().x;
    gridwidth -= m_gridLibReview->GetColSize( 0 ) + m_gridLibReview->GetColLabelSize();

    if( gridwidth < 200 )
        gridwidth = 200;

    m_gridLibReview->SetColSize( 1, gridwidth );

    event.Skip();
}


void WIZARD_3DSHAPE_LIBS_DOWNLOADER::setupReview()
{
    // Prepare the last page of the wizard.

    m_LocalFolderInfo->SetLabel( getDownloadDir() );

    wxArrayInt checkedIndices;
    m_checkList3Dlibnames->GetCheckedItems( checkedIndices );

    m_libraries.Clear();

    // populate m_libraries with the name of libraries, without the github path:
    for( unsigned int ii = 0; ii < checkedIndices.GetCount(); ++ii )
    {
        m_libraries.Add( m_checkList3Dlibnames->GetString( checkedIndices[ii] ).AfterLast( '/' ) );
    }

    // Adjust number of rows in m_gridLibReview:
    int delta = m_libraries.GetCount() - m_gridLibReview->GetNumberRows();

    if( delta < 0 )
        m_gridLibReview->DeleteRows( -delta );
    else if( delta > 0 )
        m_gridLibReview->AppendRows( delta );

    // For user info, verify the existence of these libs in local folder
    wxArrayString liblist;
    wxFileName fn;
    fn.AssignDir( getDownloadDir() );

    for( unsigned int ii = 0; ii < m_libraries.GetCount(); ++ii )
    {
        fn.SetName( m_libraries[ii] );

        wxDir dirs;
        bool isNew = ! dirs.Exists( fn.GetFullPath() );
        wxString info = isNew ? _( "New" ) : _( "Update" );

        liblist.Add( info + wxT("  ") + m_libraries[ii] );

        m_gridLibReview->SetCellValue( ii, 0, info );
        m_gridLibReview->SetCellValue( ii, 1, m_libraries[ii] );
    }

    m_gridLibReview->AutoSizeColumn( 0 );
}


void WIZARD_3DSHAPE_LIBS_DOWNLOADER::OnSelectAll3Dlibs( wxCommandEvent& aEvent )
{
    for( unsigned int i = 0; i < m_checkList3Dlibnames->GetCount(); ++i )
        m_checkList3Dlibnames->Check( i, true );

    // The list might be empty, e.g. in case of download error
    wxArrayInt dummy;
    enableNext( m_checkList3Dlibnames->GetCheckedItems( dummy ) > 0 );
}


void WIZARD_3DSHAPE_LIBS_DOWNLOADER::OnUnselectAll3Dlibs( wxCommandEvent& aEvent )
{
    for( unsigned int i = 0; i < m_checkList3Dlibnames->GetCount(); ++i )
        m_checkList3Dlibnames->Check( i, false );

    enableNext( false );
}


void WIZARD_3DSHAPE_LIBS_DOWNLOADER::OnChangeSearch( wxCommandEvent& aEvent )
{
    wxString searchPhrase = m_searchCtrl3Dlibs->GetValue().Lower();

    // Store the current selection
    wxArrayInt checkedIndices;
    m_checkList3Dlibnames->GetCheckedItems( checkedIndices );
    wxArrayString checkedStrings;

    for( unsigned int i = 0; i < checkedIndices.GetCount(); ++i )
        checkedStrings.Add( m_checkList3Dlibnames->GetString( checkedIndices[i] ).AfterLast( '/' ) );

    m_checkList3Dlibnames->Clear();

    // Rebuild the list, putting the matching entries on the top
    int matching = 0;   // number of entries matching the search phrase
    for( unsigned int i = 0; i < m_githubLibs.GetCount(); ++i )
    {
        const wxString& lib = m_githubLibs[i].AfterLast( '/' );
        bool wasChecked = ( checkedStrings.Index( lib ) != wxNOT_FOUND );
        int insertedIdx = -1;

        if( !searchPhrase.IsEmpty() && lib.Lower().Contains( searchPhrase ) )
        {
            insertedIdx = m_checkList3Dlibnames->Insert( lib, matching++ );
            m_checkList3Dlibnames->SetSelection( insertedIdx );
        }
        else
            insertedIdx = m_checkList3Dlibnames->Append( lib );

        if( wasChecked )
            m_checkList3Dlibnames->Check( insertedIdx );
    }

    if( !m_checkList3Dlibnames->IsEmpty() )
        m_checkList3Dlibnames->EnsureVisible( 0 );
}


void WIZARD_3DSHAPE_LIBS_DOWNLOADER::OnWizardFinished( wxWizardEvent& aEvent )
{
    // we download a localy copy of the libraries
    wxString error;

    if( !downloadGithubLibsFromList( m_libraries, &error ) )
    {
        DisplayError( GetParent(), error );
    }
}


void WIZARD_3DSHAPE_LIBS_DOWNLOADER::OnBrowseButtonClick( wxCommandEvent& aEvent )
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


void WIZARD_3DSHAPE_LIBS_DOWNLOADER::OnDefault3DPathButtonClick( wxCommandEvent& event )
{
    wxString default_path;
    wxGetEnv( KISYS3DMOD, &default_path );

    if( !default_path.IsEmpty() && wxDirExists( default_path ) )
    {
        setDownloadDir( default_path );
        updateGithubControls();
    }
    else
        wxMessageBox( _( "KISYS3DMOD path not defined , or not existing" ) );
}


void WIZARD_3DSHAPE_LIBS_DOWNLOADER::OnCheckSaveCopy( wxCommandEvent& aEvent )
{
    updateGithubControls();
}


void WIZARD_3DSHAPE_LIBS_DOWNLOADER::getLibsListGithub( wxArrayString& aList )
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
    getter.Get3DshapesLibsList( &aList, filter3dshapeslibraries );

    wxEndBusyCursor();
}


// Download the .pretty libraries folders found in aUrlList and store them on disk
// in a master folder
bool WIZARD_3DSHAPE_LIBS_DOWNLOADER::downloadGithubLibsFromList( wxArrayString& aUrlList,
                                                     wxString* aErrorMessage )
{
    // Display a progress bar to show the download state
    // The title is updated for each downloaded library.
    // the state will be updated by downloadOneLib() for each file.
    wxProgressDialog pdlg( _( "Downloading 3D libraries" ), wxEmptyString,
                           aUrlList.GetCount(), GetParent(),
                           wxPD_CAN_ABORT | wxPD_APP_MODAL | wxPD_AUTO_HIDE );

    wxString url_base = GetGithubURL();

    // Download libs:
    for( unsigned ii = 0; ii < aUrlList.GetCount(); ii++ )
    {
        wxString& libsrc_name = aUrlList[ii];

        // Extract the lib name from the full URL:
        wxString url = GetGithubURL() + wxT( "/" ) + libsrc_name;
        wxFileName fn( libsrc_name );
        // Set our local path
        fn.SetPath( getDownloadDir() );
        wxString libdst_name = fn.GetFullPath();

        // Display the name of the library to download in the wxProgressDialog
        pdlg.SetTitle( wxString::Format( wxT("%s [%d/%d]" ),
                       libsrc_name.AfterLast( '/' ).GetData(),
                       ii + 1, aUrlList.GetCount() ) );

        if( !wxDirExists( libdst_name ) )
            wxMkdir( libdst_name );

        if( !downloadOneLib( url, libdst_name, &pdlg, aErrorMessage ) )
            return false;
    }

    return true;
}


bool WIZARD_3DSHAPE_LIBS_DOWNLOADER::downloadOneLib( const wxString& aLibURL,
                const wxString& aLocalLibName, wxProgressDialog * aIndicator,
                wxString* aErrorMessage )
{
    wxArrayString fileslist;

    bool success;

    // Get the list of candidate files: with ext .wrl or .wings
    do
    {
        GITHUB_GETLIBLIST getter( aLibURL );
        success = getter.Get3DshapesLibsList( &fileslist, filter3dshapesfiles );
    } while( 0 );

    if( !success )
        return false;

    // Load each file in list:
    wxURI repo( aLibURL );

    wxString server = repo.GetServer();

    // Github gives the current url of files inside .3dshapes folders like:
    //  "https://github.com/KiCad/kicad-library/blob/master/modules/packages3d/Capacitors_SMD.3dshapes/C_0402.wrl"
    // which displays a html page showing the file in html form.
    //
    // the URL of the corresponding raw file is
    // "https://github.com/KiCad/kicad-library/raw/master/modules/packages3d/Capacitors_SMD.3dshapes/C_0402.wrl"
    //
    // However Github redirects this current url to raw.githubusercontent.com/fullfilename
    // when trying to download raw files.
    //  "https://github.com/KiCad/kicad-library/raw/master/modules/packages3d/Capacitors_SMD.3dshapes/C_0402.wrl"
    // would be redirected to:
    //  "https://raw.githubusercontent.com/KiCad/kicad-library/master/modules/packages3d/Capacitors_SMD.3dshapes/C_0402.wrl"
    // So use raw.githubusercontent.com instead of github.com
    // (and removes the "/raw" in path)  speed up the downloads (x2 faster).
    //
    // wxURI has no way to change the server name, so we need to use tricks to make the URL.
    //
    // Comment this next line to use the github.com URL
#define FORCE_GITHUB_RAW_URL

#ifdef FORCE_GITHUB_RAW_URL
    if( server.Cmp( wxT( "github.com" ) ) == 0 )
        server = wxT( "raw.githubusercontent.com" );
#endif

    wxString full_url_base = repo.GetScheme() + wxT( "://" ) + server;
    wxString target_full_url;

    for( unsigned ii = 0; ii < fileslist.GetCount(); ii++ )
    {
        target_full_url = full_url_base + fileslist[ii];

#ifdef FORCE_GITHUB_RAW_URL
        // Remove "blob/" in URL string to build the URL on "raw.githubusercontent.com"
        // server from "github.com" URL string:
        target_full_url.Replace( wxT( "blob/" ), wxT( "" ) );
#else
        // Replace "blob" by "raw" in URL to access the raw file itself, not the html page
        // on "github.com" server
        target_full_url.Replace( wxT( "blob" ), wxT( "raw" ) );
#endif
        aIndicator->SetRange( fileslist.GetCount() );
        bool abort = !aIndicator->Update( ii, target_full_url.AfterLast( '/' ) );

        if( abort )
        {
            if( aErrorMessage )
                *aErrorMessage << _( "Aborted by user" );
            return false;
        }

        // Download the current file.
        // Get3DshapesLibsList actually downloads and stores the target_full_url content.
        GITHUB_GETLIBLIST getter( target_full_url );
        success = getter.Get3DshapesLibsList( NULL, NULL );

        if( !success )
            break;

        wxFileName fn;
        fn.AssignDir( aLocalLibName );
        fn.SetFullName( fileslist[ii].AfterLast( '/' ) );

        // The entire downloaded file is stored in getter buffer
        const std::string& buffer = getter.GetBuffer();

        // Write is "as this". It can be a binary file.
        wxFile file(fn.GetFullPath(), wxFile::write);
        file.Write( &buffer[0], buffer.size() );
    }

    return success;
}


void WIZARD_3DSHAPE_LIBS_DOWNLOADER::setupGithubList()
{
    // Enable 'Next' only if there is at least one library selected
    wxArrayInt checkedIndices;
    m_checkList3Dlibnames->GetCheckedItems( checkedIndices );
    enableNext( checkedIndices.GetCount() > 0 );

    // Update only if necessary
    if( m_githubLibs.GetCount() == 0 )
        getLibsListGithub( m_githubLibs );

    m_searchCtrl3Dlibs->Clear();

    // Clear the review list so it will be reloaded
    m_libraries.clear();
}


void WIZARD_3DSHAPE_LIBS_DOWNLOADER::updateGithubControls()
{
    bool valid = wxFileName::IsDirWritable( getDownloadDir() );

    // Shows or not the warning text if the target 3d folder does not exist, or is not
    // writable.
    m_invalidDirWarningText->Show( !valid );
    m_bitmapDirWarn->Show( !valid );

    // If the dialog starts with m_invalidDirWarningText and m_bitmapDirWarn not shown
    // the size and position of the sizer containing these widgets can be incorrect,
    // until a wxSizeEvent is fired, and the widgets are not shown, or truncated,
    // at least on Windows. So fire a dummy wxSizeEvent if the size looks bad
    if( m_invalidDirWarningText->IsShown() && m_invalidDirWarningText->GetSize().x < 2 )
    {
        wxSizeEvent event( GetSize() );
        wxPostEvent( this, event );
    }

    // Allow to go further only if there is a valid target directory selected
    enableNext( valid );
}

// Called when the local folder name is edited.
void WIZARD_3DSHAPE_LIBS_DOWNLOADER::OnLocalFolderChange( wxCommandEvent& event )
{
    updateGithubControls();
}


void WIZARD_3DSHAPE_LIBS_DOWNLOADER::setupDialogOrder()
{
    m_welcomeDlg->SetNext( m_githubListDlg );
    m_githubListDlg->SetPrev( m_welcomeDlg );
    m_githubListDlg->SetNext( m_reviewDlg );
    m_reviewDlg->SetPrev( m_githubListDlg );
}

