/*
 * This code comes from wxWebView sample webview.cpp and is modified to be used in Kicad
 * the wxWidgets sample code webview.cpp is under wxWindows licence (author Marianne Gagnon)
 *
 * Webviewer runs in a dialog to allows an user to select a list of URLs which are .pretty library
 * folder accessible from http or https using the Github plugin.
 *
 * The Web viewer just return a list of URLs selected from the context menu (command copy link)
 * when mouse cursor in on a http link.
 * URLs strings are filtered
 * Only URLs starting by "http" and ending by ".pretty" can be currently selected and stored in fp lib list
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

/*
 * wxWidgets gives very few info about wxwebkit. For more info and more comments:
 * see https://forums.wxwidgets.org/viewtopic.php?f=1&t=1119#
 */

#include "wx/wx.h"

#if !wxUSE_WEBVIEW_WEBKIT && !wxUSE_WEBVIEW_IE
#error "wxWidgets must be built with wxWebView support enabled. Please rebuild wxWidgets"
#endif

#include "wx/artprov.h"
#include "wx/cmdline.h"
#include "wx/notifmsg.h"
#include "wx/settings.h"
#include "wx/webview.h"
#include "wx/webviewarchivehandler.h"
#include "wx/webviewfshandler.h"
#include "wx/infobar.h"
#include "wx/filesys.h"
#include "wx/fs_arc.h"
#include <wx/clipbrd.h>
#include <wx/uri.h>
#if wxUSE_STC
#include "wx/stc/stc.h"
#else
#error "wxWidgets must be built with wxStyledTextControl enabled. Please rebuild wxWidgets"
#endif

#include <dialog_shim.h>
#include <bitmaps.h>
#include "html_link_parser.h"

// We map menu items to their history items
WX_DECLARE_HASH_MAP( int, wxSharedPtr<wxWebViewHistoryItem>,
        wxIntegerHash, wxIntegerEqual, wxMenuHistoryMap );


class WEB_NAVIGATOR : public DIALOG_SHIM
{
    wxArrayString* urlListSelection;

public:
    WEB_NAVIGATOR( wxWindow* aParent,
            const wxString& aUrlOnStart,
            wxArrayString* aUrlListSelection = NULL );
    virtual ~WEB_NAVIGATOR();

    void InitNavigator( const wxString& aUrlOnStart );

private:
    void    buildToolBar();
    void    buildToolMenu();
    void    CreateFindToolBar( wxWindow* aParent );

    void UpdateState();

    // wxWebViewEvents:
    /** Callback invoked when a navigation request was accepted
     */
    void OnNavigationComplete( wxWebViewEvent& evt )
    {
        UpdateState();
    }

    /** Callback invoked when a page is finished loading
     */
    void OnDocumentLoaded( wxWebViewEvent& evt )
    {
        UpdateState();
    }

    /** On new window, we veto to stop extra windows appearing
     */
    void OnNewWindow( wxWebViewEvent& evt )
    {
        UpdateState();
    }

    void OnTitleChanged( wxWebViewEvent& evt )
    {
        SetTitle( evt.GetString() );
    }

    // event functions:
    void    OnIdle( wxIdleEvent& evt );
    void    OnUrl( wxCommandEvent& evt );
    void    OnBack( wxCommandEvent& evt );
    void    OnForward( wxCommandEvent& evt );
    void    OnStop( wxCommandEvent& evt );
    void    OnReload( wxCommandEvent& evt );
    void    OnClearHistory( wxCommandEvent& evt );
    void    OnNavigationRequest( wxWebViewEvent& evt );
    void    OnViewSourceRequest( wxCommandEvent& evt );
    void    OnToolsClicked( wxCommandEvent& evt );
    void    OnSetZoom( wxCommandEvent& evt );
    void    OnError( wxWebViewEvent& evt );

    void    OnPrint( wxCommandEvent& evt )
    {
        m_browser->Print();
    }

    void OnZoomLayout( wxCommandEvent& evt );

    void    OnHistory( wxCommandEvent& evt )
    {
        m_browser->LoadHistoryItem( m_histMenuItems[evt.GetId()] );
    }

    void    OnFind( wxCommandEvent& evt );
    void    OnFindDone( wxCommandEvent& evt );
    void    OnFindText( wxCommandEvent& evt );
    void    OnFindOptions( wxCommandEvent& evt );

    // only for fp lib wizard called if there is a non null
    // aUrlListSelection from idle event, to see if a valid
    // footprint lib was selected by user, from the context menu
    // (command copy link)
    void TestAndStoreFootprintLibLink();
    // Alternate way to list and select fp libraries:
    void    OnListPrettyLibURLs( wxCommandEvent& event );

private:
    wxTextCtrl* m_url;
    wxWebView*  m_browser;

    wxToolBar* m_toolbar;
    wxToolBarToolBase*  m_toolbar_back;
    wxToolBarToolBase*  m_toolbar_forward;
    wxToolBarToolBase*  m_toolbar_stop;
    wxToolBarToolBase*  m_toolbar_reload;
    wxToolBarToolBase*  m_toolbar_tools;
    wxToolBarToolBase*  m_toolbar_ListLibs;

    wxToolBarToolBase*  m_find_toolbar_done;
    wxToolBarToolBase*  m_find_toolbar_next;
    wxToolBarToolBase*  m_find_toolbar_previous;
    wxToolBarToolBase*  m_find_toolbar_options;

    wxMenuItem* m_find_toolbar_wrap;
    wxMenuItem* m_find_toolbar_highlight;
    wxMenuItem* m_find_toolbar_matchcase;
    wxMenuItem* m_find_toolbar_wholeword;

    wxMenu* m_tools_menu;
    wxMenuItem* m_tools_viewSource;
    wxMenuItem* m_tools_print;
    wxMenu* m_tools_history_menu;
    wxMenuItem* m_tools_clearhist;
    wxMenuItem* m_tools_layout;
    wxMenuItem* m_tools_tiny;
    wxMenuItem* m_tools_small;
    wxMenuItem* m_tools_medium;
    wxMenuItem* m_tools_large;
    wxMenuItem* m_tools_largest;
    wxMenuItem* m_find;

    wxInfoBar* m_info;
    wxStaticText*   m_info_text;
    wxTextCtrl*     m_find_ctrl;
    wxToolBar* m_find_toolbar;

    wxMenuHistoryMap m_histMenuItems;
    wxString m_findText;
    int m_findFlags, m_findCount;
};

class SourceViewDialog : public wxDialog
{
public:
    SourceViewDialog( wxWindow* parent, wxString source );
};

// Helper function to run the web viewer from an other dialog or frame:
int RunWebViewer( wxWindow* aParent, const wxString& aUrlOnStart, wxArrayString* aUrlListSelection )
{
    WEB_NAVIGATOR dlg( aParent, aUrlOnStart, aUrlListSelection );
    return dlg.ShowModal();
}


WEB_NAVIGATOR::WEB_NAVIGATOR( wxWindow* aParent,
        const wxString& aUrlOnStart, wxArrayString* aUrlListSelection ) :
    DIALOG_SHIM( aParent, wxID_ANY, "Web Viewer", wxDefaultPosition, wxDefaultSize,
            wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER )
{
    urlListSelection = aUrlListSelection;
    InitNavigator( aUrlOnStart );

    SetMinSize( wxSize( 700, 500 ) );

    GetSizer()->Fit( this );
    Centre();
}


void WEB_NAVIGATOR::InitNavigator( const wxString& aUrlOnStart )
{
    // To collect URLs, the Web Viewer can use the clipboard.
    // Clear it before running the viewer
    // For an unknown reason, wxTheClipboard->Clear() does not work,
    // so use a trick
    if( wxTheClipboard->Open() )
    {
        wxTheClipboard->SetData( new wxTextDataObject( "" ) );
        wxTheClipboard->Close();
    }

    wxBoxSizer* topsizer = new wxBoxSizer( wxVERTICAL );

    // Create the main toolbar
    m_toolbar = new wxToolBar( this, wxID_ANY );
    buildToolBar();
    topsizer->Add( m_toolbar );

    // Shows the tool to choose and select fp libs only if a selected URL list
    // exists
    if( urlListSelection == NULL )
        m_toolbar_ListLibs->Enable( false );

    // Set find parameters and create the find toolbar.
    m_findFlags = wxWEBVIEW_FIND_DEFAULT;
    m_findCount = 0;
    CreateFindToolBar( this );
    topsizer->Add( m_find_toolbar, wxSizerFlags().Expand() );
    m_find_toolbar->Hide();

    // Create the info panel
    m_info = new wxInfoBar( this );
    topsizer->Add( m_info, wxSizerFlags().Expand() );

    // Create the webview engine
    if( aUrlOnStart.IsEmpty() )     // Start on a blank page:
        m_browser = wxWebView::New( this, wxID_ANY, wxT( "about:blank")  );
    else
        m_browser = wxWebView::New( this, wxID_ANY, aUrlOnStart,
                                    wxDefaultPosition, wxSize( 900, 600 ) );

    m_browser->EnableHistory( true );

    topsizer->Add( m_browser, wxSizerFlags().Expand().Proportion( 1 ) );

    SetSizer( topsizer );

    // Create the Tools menu
    buildToolMenu();

    if( !m_browser->CanSetZoomType( wxWEBVIEW_ZOOM_TYPE_LAYOUT ) )
        m_tools_layout->Enable( false );

    // Connect the toolbar events
    Connect( m_toolbar_back->GetId(), wxEVT_TOOL,
            wxCommandEventHandler( WEB_NAVIGATOR::OnBack ), NULL, this );
    Connect( m_toolbar_forward->GetId(), wxEVT_TOOL,
            wxCommandEventHandler( WEB_NAVIGATOR::OnForward ), NULL, this );
    Connect( m_toolbar_stop->GetId(), wxEVT_TOOL,
            wxCommandEventHandler( WEB_NAVIGATOR::OnStop ), NULL, this );
    Connect( m_toolbar_reload->GetId(), wxEVT_TOOL,
            wxCommandEventHandler( WEB_NAVIGATOR::OnReload ), NULL, this );
    Connect( m_toolbar_tools->GetId(), wxEVT_TOOL,
            wxCommandEventHandler( WEB_NAVIGATOR::OnToolsClicked ), NULL, this );

    Connect( m_toolbar_ListLibs->GetId(), wxEVT_TOOL,
            wxCommandEventHandler( WEB_NAVIGATOR::OnListPrettyLibURLs ), NULL, this );

    Connect( m_url->GetId(), wxEVT_TEXT_ENTER,
            wxCommandEventHandler( WEB_NAVIGATOR::OnUrl ), NULL, this );

    // Connect find toolbar events.
    Connect( m_find_toolbar_done->GetId(), wxEVT_TOOL,
            wxCommandEventHandler( WEB_NAVIGATOR::OnFindDone ), NULL, this );
    Connect( m_find_toolbar_next->GetId(), wxEVT_TOOL,
            wxCommandEventHandler( WEB_NAVIGATOR::OnFindText ), NULL, this );
    Connect( m_find_toolbar_previous->GetId(), wxEVT_TOOL,
            wxCommandEventHandler( WEB_NAVIGATOR::OnFindText ), NULL, this );

    // Connect find control events.
    Connect( m_find_ctrl->GetId(), wxEVT_TEXT,
            wxCommandEventHandler( WEB_NAVIGATOR::OnFindText ), NULL, this );
    Connect( m_find_ctrl->GetId(), wxEVT_TEXT_ENTER,
            wxCommandEventHandler( WEB_NAVIGATOR::OnFindText ), NULL, this );

    // Connect the webview events
    Connect( m_browser->GetId(), wxEVT_WEBVIEW_NAVIGATING,
            wxWebViewEventHandler( WEB_NAVIGATOR::OnNavigationRequest ), NULL, this );
    Connect( m_browser->GetId(), wxEVT_WEBVIEW_NAVIGATED,
            wxWebViewEventHandler( WEB_NAVIGATOR::OnNavigationComplete ), NULL, this );
    Connect( m_browser->GetId(), wxEVT_WEBVIEW_LOADED,
            wxWebViewEventHandler( WEB_NAVIGATOR::OnDocumentLoaded ), NULL, this );
    Connect( m_browser->GetId(), wxEVT_WEBVIEW_ERROR,
            wxWebViewEventHandler( WEB_NAVIGATOR::OnError ), NULL, this );
    Connect( m_browser->GetId(), wxEVT_WEBVIEW_NEWWINDOW,
            wxWebViewEventHandler( WEB_NAVIGATOR::OnNewWindow ), NULL, this );
    Connect( m_browser->GetId(), wxEVT_WEBVIEW_TITLE_CHANGED,
            wxWebViewEventHandler( WEB_NAVIGATOR::OnTitleChanged ), NULL, this );

    // Connect the menu events
    Connect( m_tools_viewSource->GetId(), wxEVT_MENU,
            wxCommandEventHandler( WEB_NAVIGATOR::OnViewSourceRequest ), NULL, this );
    Connect( m_tools_print->GetId(), wxEVT_MENU,
            wxCommandEventHandler( WEB_NAVIGATOR::OnPrint ), NULL, this );
    Connect( m_tools_layout->GetId(), wxEVT_MENU,
            wxCommandEventHandler( WEB_NAVIGATOR::OnZoomLayout ), NULL, this );
    Connect( m_tools_tiny->GetId(), wxEVT_MENU,
            wxCommandEventHandler( WEB_NAVIGATOR::OnSetZoom ), NULL, this );
    Connect( m_tools_small->GetId(), wxEVT_MENU,
            wxCommandEventHandler( WEB_NAVIGATOR::OnSetZoom ), NULL, this );
    Connect( m_tools_medium->GetId(), wxEVT_MENU,
            wxCommandEventHandler( WEB_NAVIGATOR::OnSetZoom ), NULL, this );
    Connect( m_tools_large->GetId(), wxEVT_MENU,
            wxCommandEventHandler( WEB_NAVIGATOR::OnSetZoom ), NULL, this );
    Connect( m_tools_largest->GetId(), wxEVT_MENU,
            wxCommandEventHandler( WEB_NAVIGATOR::OnSetZoom ), NULL, this );
    Connect( m_tools_clearhist->GetId(), wxEVT_MENU,
            wxCommandEventHandler( WEB_NAVIGATOR::OnClearHistory ), NULL, this );
    Connect( m_find->GetId(), wxEVT_MENU,
            wxCommandEventHandler( WEB_NAVIGATOR::OnFind ), NULL, this );

    // Connect the idle events
    Connect( wxID_ANY, wxEVT_IDLE, wxIdleEventHandler( WEB_NAVIGATOR::OnIdle ), NULL, this );
}


WEB_NAVIGATOR::~WEB_NAVIGATOR()
{
    delete m_tools_menu;
}


void WEB_NAVIGATOR::buildToolMenu()
{
    m_tools_menu = new wxMenu();

    m_tools_print = m_tools_menu->Append( wxID_ANY, _( "Print" ) );
    m_tools_viewSource = m_tools_menu->Append( wxID_ANY, _( "View Source" ) );
    m_tools_menu->AppendSeparator();
    m_tools_layout = m_tools_menu->AppendCheckItem( wxID_ANY, _( "Use Layout Zoom" ) );
    m_tools_tiny    = m_tools_menu->AppendCheckItem( wxID_ANY, _( "Tiny" ) );
    m_tools_small   = m_tools_menu->AppendCheckItem( wxID_ANY, _( "Small" ) );
    m_tools_medium  = m_tools_menu->AppendCheckItem( wxID_ANY, _( "Medium" ) );
    m_tools_large   = m_tools_menu->AppendCheckItem( wxID_ANY, _( "Large" ) );
    m_tools_largest = m_tools_menu->AppendCheckItem( wxID_ANY, _( "Largest" ) );
    m_tools_menu->AppendSeparator();

    // Find
    m_find = m_tools_menu->Append( wxID_ANY, _( "Find" ) );
    m_tools_menu->AppendSeparator();

    // History menu
    m_tools_history_menu = new wxMenu();
    m_tools_clearhist = m_tools_history_menu->Append( wxID_ANY, _( "Clear History" ) );
    m_tools_history_menu->AppendSeparator();

    m_tools_menu->AppendSubMenu( m_tools_history_menu, "History" );
}


void WEB_NAVIGATOR::buildToolBar()
{
    // Populate the toolbar
    m_toolbar->SetToolBitmapSize( wxSize( 26, 26 ) );
    m_toolbar->SetMargins( 3, 3 );

    // The tool menu (one cannot have a menubar in a dialog, so uise a tool to display a menu
    m_toolbar_tools = m_toolbar->AddTool( wxID_ANY, _( "Menu" ), KiBitmap( tools_xpm ),
                                          _("Access to some options") );
    m_toolbar->AddSeparator();

    // The navigation tools
    m_toolbar_back = m_toolbar->AddTool( wxID_ANY, _( "Back" ), KiBitmap( left_xpm ) );
    m_toolbar_forward = m_toolbar->AddTool( wxID_ANY, _( "Forward" ), KiBitmap( right_xpm ) );
    m_toolbar_stop = m_toolbar->AddTool( wxID_ANY, _( "Stop" ), KiBitmap( red_xpm ) );
    m_toolbar_reload = m_toolbar->AddTool( wxID_ANY, _( "Reload" ), KiBitmap( reload2_xpm ) );

    m_toolbar->AddSeparator();
    m_url = new wxTextCtrl( m_toolbar, wxID_ANY, wxT( "" ), wxDefaultPosition, wxSize( 400,
                    -1 ), wxTE_PROCESS_ENTER );
    m_toolbar->AddControl( m_url, _( "URL" ) );
    m_toolbar->AddSeparator();

    // The Kicad footprint libraries extractor/selector tool:
    m_toolbar_ListLibs =
        m_toolbar->AddTool( wxID_ANY, _( "KicadLibs" ), KiBitmap( library_browse_xpm ),
                _("List .pretty kicad footprint libraries and add selected libraries\n"
                  "to the footprint library table") );

    m_toolbar->Realize();
}


void WEB_NAVIGATOR::CreateFindToolBar( wxWindow* aParent )
{
    m_find_toolbar = new wxToolBar( aParent,
            wxID_ANY, wxDefaultPosition, wxDefaultSize,
            wxTB_HORIZONTAL | wxTB_TEXT | wxTB_HORZ_LAYOUT );

    // Create find control.
    m_find_ctrl = new wxTextCtrl( m_find_toolbar,
            wxID_ANY, wxEmptyString, wxDefaultPosition,
            wxSize( 140, -1 ), wxTE_PROCESS_ENTER );

    // Find options menu
    wxMenu* findmenu = new wxMenu;
    m_find_toolbar_wrap = findmenu->AppendCheckItem( wxID_ANY, "Wrap" );
    m_find_toolbar_matchcase    = findmenu->AppendCheckItem( wxID_ANY, "Match Case" );
    m_find_toolbar_wholeword    = findmenu->AppendCheckItem( wxID_ANY, "Entire Word" );
    m_find_toolbar_highlight    = findmenu->AppendCheckItem( wxID_ANY, "Highlight" );
    m_find_toolbar_highlight->Check( true );

    // Add find toolbar tools.
    m_find_toolbar->SetToolSeparation( 7 );
    m_find_toolbar_done = m_find_toolbar->AddTool( wxID_ANY, "Close",
            wxArtProvider::GetBitmap( wxART_CROSS_MARK ) );
    m_find_toolbar->AddSeparator();
    m_find_toolbar->AddControl( m_find_ctrl, "Find" );
    m_find_toolbar->AddSeparator();
    m_find_toolbar_next = m_find_toolbar->AddTool( wxID_ANY, "Next",
            wxArtProvider::GetBitmap( wxART_GO_DOWN, wxART_TOOLBAR, wxSize( 16, 16 ) ) );
    m_find_toolbar_previous = m_find_toolbar->AddTool( wxID_ANY, "Previous",
            wxArtProvider::GetBitmap( wxART_GO_UP, wxART_TOOLBAR, wxSize( 16, 16 ) ) );
    m_find_toolbar->AddSeparator();
    m_find_toolbar_options = m_find_toolbar->AddTool( wxID_ANY, "Options",
            wxArtProvider::GetBitmap( wxART_PLUS, wxART_TOOLBAR, wxSize( 16, 16 ) ),
            "", wxITEM_DROPDOWN );
    m_find_toolbar_options->SetDropdownMenu( findmenu );
    m_find_toolbar->Realize();
}


// A helper function to try to validate urls names
// read in github repos.
// a valid .pretty github library name ( on github )
// is expected ending with .pretty, and to be a path
// relative github url
static bool urlFilter( const wxString& aText )
{
    if( aText.Length() < 8 ) // unlikely a valid .pretty name
        return false;

    if( !aText.EndsWith( wxT( ".pretty" ) ) )
        return false;

    wxURI uri( aText );

    if( !uri.GetQuery().IsEmpty() ) // the link is a request, not good
        return false;

    return true;
}


void WEB_NAVIGATOR::OnListPrettyLibURLs( wxCommandEvent& event )
{
    if( m_browser->IsBusy() ) // a page loading is currently in progress
        return;

    wxString source = m_browser->GetPageSource();
    wxArrayString urls;

    HTML_LINK_PARSER parser( source, urls );
    parser.ParseLinks( urlFilter );

    // Create library list.
    // From github, the links can be relative to the github server URL
    // In this case, make url absolute
    wxString reposUrl( m_browser->GetCurrentURL() );
    wxURI reposUri( reposUrl );
    wxString reposName( reposUri.GetScheme() + wxT( "://" ) + reposUri.GetServer() );

    for( unsigned ii = 0; ii < urls.GetCount(); ii++ )
    {
        wxString url = urls[ii];
        wxURI currUri( url );

        if( !currUri.HasServer() )
        {
            if( url.StartsWith( wxT( "/" ) ) )  // path relative to the server name
                url.Prepend( reposName );
            else                                // path relative to the server currently open path
                url.Prepend( reposUrl );

            urls[ii] = url;
        }
    }

    // Remove duplicates:
    // A very basic test, not optimized, but usually we have only few urls
    for( unsigned ii = 0; ii < urls.GetCount(); ii++ )
    {
        for( unsigned jj = urls.GetCount() - 1; jj > ii; jj-- )
        {
            if( urls[ii] == urls[jj] ) // Duplicate found
                urls.RemoveAt( jj );
        }
    }

    wxArrayInt choices;
    wxString msg( _( "Urls detected as footprint .pretty libraries.\n"
                     "Selected urls will be added to the current footprint library list,\n"
                     "when closing the web viewer") );

    if( wxGetSelectedChoices( choices, msg,
                _( "Footprint libraries" ), urls, this ) <= 0 )
        return;

    // Add selected fp list in list
    for( unsigned ii = 0; ii < choices.GetCount(); ii++ )
    {
        wxString& url = urls[choices[ii]];
        urlListSelection->Add( url );
    }
}


/**
 * Method that retrieves the current state from the web control and updates the GUI
 * the reflect this current state.
 */
void WEB_NAVIGATOR::UpdateState()
{
    m_toolbar->EnableTool( m_toolbar_back->GetId(), m_browser->CanGoBack() );
    m_toolbar->EnableTool( m_toolbar_forward->GetId(), m_browser->CanGoForward() );

    if( m_browser->IsBusy() )
    {
        m_toolbar->EnableTool( m_toolbar_stop->GetId(), true );
    }
    else
    {
        m_toolbar->EnableTool( m_toolbar_stop->GetId(), false );
    }

    SetTitle( m_browser->GetCurrentTitle() );
    m_url->SetValue( m_browser->GetCurrentURL() );
}


void WEB_NAVIGATOR::OnIdle( wxIdleEvent& WXUNUSED( evt ) )
{
    if( m_browser->IsBusy() )
    {
        wxSetCursor( wxCURSOR_ARROWWAIT );
        m_toolbar->EnableTool( m_toolbar_stop->GetId(), true );
    }
    else
    {
        wxSetCursor( wxNullCursor );
        m_toolbar->EnableTool( m_toolbar_stop->GetId(), false );

        if( urlListSelection )
            TestAndStoreFootprintLibLink();
    }
}


// only for fp lib wizard called if there is a non null
// aUrlListSelection from idle event, to see if a valid
// footprint lib was selected by user, from the context menu
// (command copy link)
void WEB_NAVIGATOR::TestAndStoreFootprintLibLink()
{
    if( !urlListSelection )
        return;

    if( wxTheClipboard->Open() )
    {
        if( wxTheClipboard->IsSupported( wxDF_TEXT ) )
        {
            wxTextDataObject data;

            if( wxTheClipboard->GetData( data ) )
            {
                // A valid text data is found
                const wxString& text = data.GetText();

                // Be sure it is a valid data for us, i.e. a valid
                // kicad url
                if( urlFilter( text ) && ( text.StartsWith( "http" ) ) )
                {
                    urlListSelection->Add( text );
                    wxTheClipboard->SetData( new wxTextDataObject( wxEmptyString ) );
                }
            }

            wxTheClipboard->Close();
        }
    }
}


/**
 * Callback invoked when user entered an URL and pressed enter
 */
void WEB_NAVIGATOR::OnUrl( wxCommandEvent& WXUNUSED( evt ) )
{
    m_browser->LoadURL( m_url->GetValue() );
    m_browser->SetFocus();
    UpdateState();
}


/**
 * Callback invoked when user pressed the "back" button
 */
void WEB_NAVIGATOR::OnBack( wxCommandEvent& WXUNUSED( evt ) )
{
    m_browser->GoBack();
    UpdateState();
}


/**
 * Callback invoked when user pressed the "forward" button
 */
void WEB_NAVIGATOR::OnForward( wxCommandEvent& WXUNUSED( evt ) )
{
    m_browser->GoForward();
    UpdateState();
}


/**
 * Callback invoked when user pressed the "stop" button
 */
void WEB_NAVIGATOR::OnStop( wxCommandEvent& WXUNUSED( evt ) )
{
    m_browser->Stop();
    UpdateState();
}


/**
 * Callback invoked when user pressed the "reload" button
 */
void WEB_NAVIGATOR::OnReload( wxCommandEvent& WXUNUSED( evt ) )
{
    m_browser->Reload();
    UpdateState();
}


void WEB_NAVIGATOR::OnClearHistory( wxCommandEvent& WXUNUSED( evt ) )
{
    m_browser->ClearHistory();
    UpdateState();
}


void WEB_NAVIGATOR::OnFind( wxCommandEvent& WXUNUSED( evt ) )
{
    wxString value = m_browser->GetSelectedText();

    if( value.Len() > 150 )
    {
        value.Truncate( 150 );
    }

    m_find_ctrl->SetValue( value );

    if( !m_find_toolbar->IsShown() )
    {
        m_find_toolbar->Show( true );
        SendSizeEvent();
    }

    m_find_ctrl->SelectAll();
}


void WEB_NAVIGATOR::OnFindDone( wxCommandEvent& WXUNUSED( evt ) )
{
    m_browser->Find( "" );
    m_find_toolbar->Show( false );
    SendSizeEvent();
}


void WEB_NAVIGATOR::OnFindText( wxCommandEvent& evt )
{
    int flags = 0;

    if( m_find_toolbar_wrap->IsChecked() )
        flags |= wxWEBVIEW_FIND_WRAP;

    if( m_find_toolbar_wholeword->IsChecked() )
        flags |= wxWEBVIEW_FIND_ENTIRE_WORD;

    if( m_find_toolbar_matchcase->IsChecked() )
        flags |= wxWEBVIEW_FIND_MATCH_CASE;

    if( m_find_toolbar_highlight->IsChecked() )
        flags |= wxWEBVIEW_FIND_HIGHLIGHT_RESULT;

    if( m_find_toolbar_previous->GetId() == evt.GetId() )
        flags |= wxWEBVIEW_FIND_BACKWARDS;

    wxString find_text = m_find_ctrl->GetValue();

    long count = wxNOT_FOUND;

    // On windows, for an unknwon reason (bug ?) some texts in some
    // html pages hang the search.
    // Waiting for 2 chars before starting a search reduces the risk
    // (but the risk still exists)
#ifdef __WINDOWS__
    #define MIN_CHAR_CNT 2
    if( find_text.Length() >= MIN_CHAR_CNT )
        count = m_browser->Find( find_text, flags );
    else // Reset search
        m_browser->Find( wxEmptyString, flags );
#else
    #define MIN_CHAR_CNT 1
    count = m_browser->Find( find_text, flags );
#endif

    if( m_findText != find_text )
    {
        m_findCount = count;
        m_findText  = find_text;
    }

    if( count != wxNOT_FOUND || find_text.Length() < MIN_CHAR_CNT )
        m_find_ctrl->SetBackgroundColour( *wxWHITE );
    else
        m_find_ctrl->SetBackgroundColour( wxColour( 255, 101, 101 ) );

    m_find_ctrl->Refresh();
}


/**
 * Callback invoked when there is a request to load a new page (for instance
 * when the user clicks a link)
 */
void WEB_NAVIGATOR::OnNavigationRequest( wxWebViewEvent& evt )
{
    if( m_info->IsShown() )
        m_info->Dismiss();

    wxASSERT( m_browser->IsBusy() );

    UpdateState();
}


/**
 * Invoked when user selects the "View Source" menu item
 */
void WEB_NAVIGATOR::OnViewSourceRequest( wxCommandEvent& WXUNUSED( evt ) )
{
    SourceViewDialog dlg( this, m_browser->GetPageSource() );

    dlg.ShowModal();
}


/**
 * Invoked when user selects the "Menu" item
 */
void WEB_NAVIGATOR::OnToolsClicked( wxCommandEvent& WXUNUSED( evt ) )
{
    if( m_browser->GetCurrentURL() == "" )
        return;

    m_tools_tiny->Check( false );
    m_tools_small->Check( false );
    m_tools_medium->Check( false );
    m_tools_large->Check( false );
    m_tools_largest->Check( false );

    wxWebViewZoom zoom = m_browser->GetZoom();

    switch( zoom )
    {
    case wxWEBVIEW_ZOOM_TINY:
        m_tools_tiny->Check();
        break;

    case wxWEBVIEW_ZOOM_SMALL:
        m_tools_small->Check();
        break;

    case wxWEBVIEW_ZOOM_MEDIUM:
        m_tools_medium->Check();
        break;

    case wxWEBVIEW_ZOOM_LARGE:
        m_tools_large->Check();
        break;

    case wxWEBVIEW_ZOOM_LARGEST:
        m_tools_largest->Check();
        break;
    }

    // Firstly we clear the existing menu items, then we add the current ones
    wxMenuHistoryMap::const_iterator it;

    for( it = m_histMenuItems.begin(); it != m_histMenuItems.end(); ++it )
    {
        m_tools_history_menu->Destroy( it->first );
    }

    m_histMenuItems.clear();

    wxVector<wxSharedPtr<wxWebViewHistoryItem> >    back = m_browser->GetBackwardHistory();
    wxVector<wxSharedPtr<wxWebViewHistoryItem> >    forward = m_browser->GetForwardHistory();

    wxMenuItem* item;

    unsigned int i;

    for( i = 0; i < back.size(); i++ )
    {
        item = m_tools_history_menu->AppendRadioItem( wxID_ANY, back[i]->GetTitle() );
        m_histMenuItems[item->GetId()] = back[i];
        Connect( item->GetId(), wxEVT_MENU,
                wxCommandEventHandler( WEB_NAVIGATOR::OnHistory ), NULL, this );
    }

    wxString title = m_browser->GetCurrentTitle();

    if( title.empty() )
        title = "(untitled)";

    item = m_tools_history_menu->AppendRadioItem( wxID_ANY, title );
    item->Check();

    // No need to connect the current item
    m_histMenuItems[item->GetId()] =
        wxSharedPtr<wxWebViewHistoryItem>( new wxWebViewHistoryItem( m_browser->GetCurrentURL(),
                        m_browser->GetCurrentTitle() ) );

    for( i = 0; i < forward.size(); i++ )
    {
        item = m_tools_history_menu->AppendRadioItem( wxID_ANY, forward[i]->GetTitle() );
        m_histMenuItems[item->GetId()] = forward[i];
        Connect( item->GetId(), wxEVT_TOOL,
                wxCommandEventHandler( WEB_NAVIGATOR::OnHistory ), NULL, this );
    }

    wxPoint position = ScreenToClient( wxGetMousePosition() );
    PopupMenu( m_tools_menu, position.x, position.y );
}


/**
 * Invoked when user selects the zoom size in the menu
 */
void WEB_NAVIGATOR::OnSetZoom( wxCommandEvent& evt )
{
    if( evt.GetId() == m_tools_tiny->GetId() )
        m_browser->SetZoom( wxWEBVIEW_ZOOM_TINY );
    else if( evt.GetId() == m_tools_small->GetId() )
        m_browser->SetZoom( wxWEBVIEW_ZOOM_SMALL );
    else if( evt.GetId() == m_tools_medium->GetId() )
        m_browser->SetZoom( wxWEBVIEW_ZOOM_MEDIUM );
    else if( evt.GetId() == m_tools_large->GetId() )
        m_browser->SetZoom( wxWEBVIEW_ZOOM_LARGE );
    else if( evt.GetId() == m_tools_largest->GetId() )
        m_browser->SetZoom( wxWEBVIEW_ZOOM_LARGEST );
    else
        wxFAIL;
}


void WEB_NAVIGATOR::OnZoomLayout( wxCommandEvent& WXUNUSED( evt ) )
{
    if( m_tools_layout->IsChecked() )
        m_browser->SetZoomType( wxWEBVIEW_ZOOM_TYPE_LAYOUT );
    else
        m_browser->SetZoomType( wxWEBVIEW_ZOOM_TYPE_TEXT );
}


/**
 * Callback invoked when a loading error occurs
 */
void WEB_NAVIGATOR::OnError( wxWebViewEvent& evt )
{
#define WX_ERROR_CASE( type ) case type: \
    category = # type; break;

    wxString category;

    switch( evt.GetInt() )
    {
        WX_ERROR_CASE( wxWEBVIEW_NAV_ERR_CONNECTION );
        WX_ERROR_CASE( wxWEBVIEW_NAV_ERR_CERTIFICATE );
        WX_ERROR_CASE( wxWEBVIEW_NAV_ERR_AUTH );
        WX_ERROR_CASE( wxWEBVIEW_NAV_ERR_SECURITY );
        WX_ERROR_CASE( wxWEBVIEW_NAV_ERR_NOT_FOUND );
        WX_ERROR_CASE( wxWEBVIEW_NAV_ERR_REQUEST );
        WX_ERROR_CASE( wxWEBVIEW_NAV_ERR_USER_CANCELLED );
        WX_ERROR_CASE( wxWEBVIEW_NAV_ERR_OTHER );
    }

    // Show the info bar with an error
    wxString msg;
    msg.Printf( _( "An error occurred loading %s\n'%s'" ), evt.GetURL().GetData(),
            category.GetData() );
    m_info->ShowMessage( msg, wxICON_ERROR );

    UpdateState();
}


SourceViewDialog::SourceViewDialog( wxWindow* parent, wxString source ) :
    wxDialog( parent, wxID_ANY, "Source Code",
            wxDefaultPosition, wxSize( 700, 500 ),
            wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER )
{
    wxStyledTextCtrl* text = new wxStyledTextCtrl( this, wxID_ANY );

    text->SetMarginWidth( 1, 30 );
    text->SetMarginType( 1, wxSTC_MARGIN_NUMBER );
    text->SetText( source );

    text->StyleClearAll();
    text->SetLexer( wxSTC_LEX_HTML );
    text->StyleSetForeground( wxSTC_H_DOUBLESTRING, wxColour( 255, 0, 0 ) );
    text->StyleSetForeground( wxSTC_H_SINGLESTRING, wxColour( 255, 0, 0 ) );
    text->StyleSetForeground( wxSTC_H_ENTITY, wxColour( 255, 0, 0 ) );
    text->StyleSetForeground( wxSTC_H_TAG, wxColour( 0, 150, 0 ) );
    text->StyleSetForeground( wxSTC_H_TAGUNKNOWN, wxColour( 0, 150, 0 ) );
    text->StyleSetForeground( wxSTC_H_ATTRIBUTE, wxColour( 0, 0, 150 ) );
    text->StyleSetForeground( wxSTC_H_ATTRIBUTEUNKNOWN, wxColour( 0, 0, 150 ) );
    text->StyleSetForeground( wxSTC_H_COMMENT, wxColour( 150, 150, 150 ) );

    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
    sizer->Add( text, 1, wxEXPAND );
    SetSizer( sizer );
}
