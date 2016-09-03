/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Rafael Sokolowski <Rafael.Sokolowski@web.de>
 * Copyright (C) 2016 KiCad Developers, see CHANGELOG.TXT for contributors.
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


#include <config.h>

// kicad_curl.h must be included before wx headers, to avoid
// conflicts for some defines, at least on Windows
#ifdef BUILD_GITHUB_PLUGIN
#include <curl/curlver.h>
#include <kicad_curl/kicad_curl.h>
#endif

#include <boost/version.hpp>
#include <wx/clipbrd.h>
#include <wx/msgdlg.h>

/* Pixel information of icons in XPM format.
 * All KiCad icons are linked into shared library 'libbitmaps.a'.
 *  Icons:
 *  preference_xpm[];    // Icon for 'Developers' tab
 *  editor_xpm[];        // Icon for 'Doc Writers' tab
 *  palette_xpm[];       // Icon for 'Artists' tab
 *  language_xpm[];      // Icon for 'Translators' tab
 *  right_xpm[];         // Right arrow icon for list items
 *  info_xpm[];          // Bulb for description tab
 *  tools_xpm[];         // Sheet of paper icon for license info tab
 */
#include <bitmaps.h>
#include <build_version.h>

#include "dialog_about.h"


/*
 * Class dialog_about methods
 */

dialog_about::dialog_about(wxWindow *aParent, AboutAppInfo& appInfo)
    : dialog_about_base(aParent), info(appInfo)
{
    picInformation = KiBitmap( info_xpm );
    picDevelopers  = KiBitmap( preference_xpm );
    picDocWriters  = KiBitmap( editor_xpm );
    picArtists     = KiBitmap( palette_xpm );
    picTranslators = KiBitmap( language_xpm );
    picLicense     = KiBitmap( tools_xpm );
    picPackagers   = KiBitmap( zip_xpm );

    m_bitmapApp->SetBitmap( info.GetIcon() );

    m_staticTextAppTitle->SetLabel( info.GetAppName() );
    m_staticTextCopyright->SetLabel( info.GetCopyright() );
    m_staticTextBuildVersion->SetLabel( "Version: " + info.GetBuildVersion() );
    m_staticTextLibVersion->SetLabel( info.GetLibVersion() );

    DeleteNotebooks();
    CreateNotebooks();

    GetSizer()->SetSizeHints( this );
    m_auiNotebook->Update();
    SetFocus();
    Centre();

    Connect( wxID_COPY, wxEVT_COMMAND_BUTTON_CLICKED,
             wxCommandEventHandler( dialog_about::OnCopyVersionInfo ) );
}


dialog_about::~dialog_about()
{
}


wxFlexGridSizer* dialog_about::CreateFlexGridSizer()
{
    // three columns with vertical and horizontal extra space of two pixels
    wxFlexGridSizer* fgSizer1 = new wxFlexGridSizer( 3, 2, 2 );
    fgSizer1->SetFlexibleDirection( wxHORIZONTAL );
    fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    return fgSizer1;
}


void dialog_about::DeleteNotebooks()
{
    for( size_t i=0; i<m_auiNotebook->GetPageCount(); ++i )
        m_auiNotebook->DeletePage( i );
}


void dialog_about::CreateNotebooks()
{
    CreateNotebookHtmlPage( m_auiNotebook, _( "Information" ), picInformation,
                            info.GetDescription() );

    CreateNotebookPage( m_auiNotebook, _( "Developers" ) , picDevelopers, info.GetDevelopers() );
    CreateNotebookPage( m_auiNotebook, _( "Doc Writers" ), picDocWriters, info.GetDocWriters() );

    CreateNotebookPageByCategory( m_auiNotebook, _( "Artists" ), picArtists, info.GetArtists() );
    CreateNotebookPageByCategory( m_auiNotebook, _( "Translators" ), picTranslators,
                                  info.GetTranslators() );
    CreateNotebookPageByCategory( m_auiNotebook, _( "Packagers" ), picPackagers,
                                  info.GetPackagers() );

    CreateNotebookHtmlPage( m_auiNotebook, _( "License" ), picLicense, info.GetLicense() );
}

void dialog_about::CreateNotebookPage( wxAuiNotebook* aParent, const wxString& aCaption,
                                       const wxBitmap& aIcon, const Contributors& aContributors )
{
    wxBoxSizer* bSizer = new wxBoxSizer( wxHORIZONTAL );

    wxScrolledWindow* m_scrolledWindow1 = new wxScrolledWindow( aParent, wxID_ANY,
                                                                wxDefaultPosition,
                                                                wxDefaultSize,
                                                                wxHSCROLL|wxVSCROLL );
    m_scrolledWindow1->SetScrollRate( 5, 5 );

    /* Panel for additional space at the left,
     * but can also be used to show an additional bitmap.
     */
    wxPanel* panel1 = new wxPanel( m_scrolledWindow1 );

    wxFlexGridSizer* fgSizer1 = CreateFlexGridSizer();

    for( size_t i=0; i<aContributors.GetCount(); ++i )
    {
        Contributor* contributor = &aContributors.Item( i );

        // Icon at first column
        wxStaticBitmap* m_bitmap1 = CreateStaticBitmap( m_scrolledWindow1, contributor->GetIcon() );
        fgSizer1->Add( m_bitmap1, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT, 5 );

        // Name of contributor at second column
        if ( contributor->GetName() != wxEmptyString )
        {
            wxStaticText* m_staticText1 = new wxStaticText( m_scrolledWindow1, wxID_ANY,
                                                            contributor->GetName(),
                                                            wxDefaultPosition, wxDefaultSize, 0 );
            m_staticText1->Wrap( -1 );
            fgSizer1->Add( m_staticText1, 0, wxALIGN_LEFT|wxBOTTOM, 2 );
        }
        else
        {
            fgSizer1->AddSpacer( 5 );
        }

        // Email address of contributor at third column
        if ( contributor->GetEMail() != wxEmptyString )
        {
            wxHyperlinkCtrl* hyperlink = CreateHyperlink( m_scrolledWindow1,
                                                          contributor->GetEMail() );
            fgSizer1->Add( hyperlink, 0, wxALIGN_LEFT|wxBOTTOM, 2 );
        }
        else
        {
            fgSizer1->AddSpacer( 5 );
        }
    }

    bSizer->Add( panel1, 1, wxEXPAND|wxALL, 10 );
    bSizer->Add( fgSizer1, 7, wxEXPAND|wxALL, 10 ); // adjust width of panel with first int value
    m_scrolledWindow1->SetSizer( bSizer );
    m_scrolledWindow1->Layout();
    bSizer->Fit( m_scrolledWindow1 );
    aParent->AddPage( m_scrolledWindow1, aCaption, false, aIcon );
}


void dialog_about::CreateNotebookPageByCategory(wxAuiNotebook* aParent, const wxString& aCaption,
                                                const wxBitmap& aIcon,
                                                const Contributors& aContributors)
{
    wxBoxSizer* bSizer = new wxBoxSizer( wxHORIZONTAL );

    wxScrolledWindow* m_scrolledWindow1 = new wxScrolledWindow( aParent, wxID_ANY,
                                                                wxDefaultPosition,
                                                                wxDefaultSize,
                                                                wxHSCROLL|wxVSCROLL );
    m_scrolledWindow1->SetScrollRate( 5, 5 );

    /* Panel for additional space at the left,
     * but can also be used to show an additional bitmap.
     */
    wxPanel* panel1 = new wxPanel( m_scrolledWindow1 );

    wxFlexGridSizer* fgSizer1 = CreateFlexGridSizer();

    for( size_t i=0; i < aContributors.GetCount(); ++i )
    {
        Contributor* contributor = &aContributors.Item( i );

        wxBitmap* icon = contributor->GetIcon();
        wxString category = contributor->GetCategory();

        /* to construct the next row we expect to have
         * a category and a contributor that was not considered up to now
         */
        if( ( category != wxEmptyString ) && !( contributor->IsChecked() ) )
        {
            // Icon at first column
            wxStaticBitmap* m_bitmap1 = CreateStaticBitmap( m_scrolledWindow1, icon );
            fgSizer1->Add( m_bitmap1, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT, 5 );

            // Category name at second column
            wxStaticText* m_staticText1 = new wxStaticText( m_scrolledWindow1, wxID_ANY,
                                                            contributor->GetCategory() + wxT( ":" ),
                                                            wxDefaultPosition, wxDefaultSize, 0 );
            m_staticText1->SetFont( wxFont( -1, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false,
                                            wxEmptyString ) ); // bold font
            m_staticText1->Wrap( -1 );
            fgSizer1->Add( m_staticText1, 0, wxALIGN_LEFT|wxBOTTOM, 2 );

            // Nothing at third column
            fgSizer1->AddSpacer( 5 );

            // Now, all contributors of the same category will follow
            for( size_t j=0; j < aContributors.GetCount(); ++j )
            {
                Contributor* sub_contributor = &aContributors.Item( j );

                if ( sub_contributor->GetCategory() == category )
                {
                    // First column is empty
                    fgSizer1->AddSpacer(5);

                    // Name of contributor at second column
                    wxStaticText* m_staticText2 = new wxStaticText( m_scrolledWindow1, wxID_ANY,
                                                                    wxT(" • ") + sub_contributor->GetName(),
                                                                    wxDefaultPosition,
                                                                    wxDefaultSize, 0 );
                    m_staticText1->Wrap( -1 );
                    fgSizer1->Add( m_staticText2, 0, wxALIGN_LEFT|wxBOTTOM, 2 );

                    // Email address of contributor at third column
                    if( sub_contributor->GetEMail() != wxEmptyString )
                    {
                        wxHyperlinkCtrl* hyperlink = CreateHyperlink( m_scrolledWindow1,
                                                                      sub_contributor->GetEMail() );
                        fgSizer1->Add( hyperlink, 0, wxALIGN_LEFT|wxBOTTOM, 2 );
                    }
                    else
                    {
                        fgSizer1->AddSpacer( 5 );
                    }

                    /* this contributor was added to the GUI,
                     * thus can be ignored next time
                     */
                    sub_contributor->SetChecked( true );
                }
            }
        }
        else
        {
            continue;
        }
    }

    /* Now, lets list the remaining contributors that have not been considered
     * because they were not assigned to any category.
     */
    for ( size_t k=0; k < aContributors.GetCount(); ++k )
    {
        Contributor* contributor = &aContributors.Item( k );

        if ( contributor->IsChecked() )
            continue;

        // Icon at first column
        wxStaticBitmap* m_bitmap1 = CreateStaticBitmap( m_scrolledWindow1, contributor->GetIcon() );
        fgSizer1->Add( m_bitmap1, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT, 5 );

        // Name of contributor at second column
        if( contributor->GetName() != wxEmptyString )
        {
            wxStaticText* m_staticText1 = new wxStaticText( m_scrolledWindow1, wxID_ANY,
                                                            contributor->GetName(),
                                                            wxDefaultPosition, wxDefaultSize, 0 );
            m_staticText1->Wrap( -1 );
            fgSizer1->Add( m_staticText1, 0, wxALIGN_LEFT|wxBOTTOM, 2 );
        }
        else
        {
            fgSizer1->AddSpacer( 5 );
        }

        // Email address of contributor at third column
        if ( contributor->GetEMail() != wxEmptyString )
        {
            wxHyperlinkCtrl* hyperlink = CreateHyperlink( m_scrolledWindow1,
                                                          contributor->GetEMail() );
            fgSizer1->Add( hyperlink, 0, wxALIGN_LEFT|wxBOTTOM, 2 );
        }
        else
        {
            fgSizer1->AddSpacer( 5 );
        }
    }

    bSizer->Add( panel1, 1, wxEXPAND|wxALL, 10 );
    bSizer->Add( fgSizer1, 7, wxEXPAND|wxALL, 10 ); // adjust width of panel with first int value
    m_scrolledWindow1->SetSizer( bSizer );
    m_scrolledWindow1->Layout();
    bSizer->Fit( m_scrolledWindow1 );
    aParent->AddPage( m_scrolledWindow1, aCaption, false, aIcon );
}


void dialog_about::CreateNotebookHtmlPage( wxAuiNotebook* aParent, const wxString& aCaption,
                                           const wxBitmap& aIcon, const wxString& html )
{
    wxPanel* panel = new wxPanel( aParent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                  wxTAB_TRAVERSAL );

    wxBoxSizer* bSizer = new wxBoxSizer( wxVERTICAL );

    wxString htmlPage = wxEmptyString, htmlContent = html;

    // to have a unique look background color for HTML pages is set to the default as it is
    // used for all the other widgets
    wxString htmlColor = ( this->GetBackgroundColour() ).GetAsString( wxC2S_HTML_SYNTAX );

    // beginning of HTML structure
    htmlPage.Append( wxT( "<html><body bgcolor='" ) + htmlColor + wxT( "'>" ) );

    htmlPage.Append( htmlContent );

    // end of HTML structure indicated by closing tags
    htmlPage.Append( wxT( "</body></html>" ) );

    // the HTML page is going to be created with previously created HTML content
    wxHtmlWindow* htmlWindow = new wxHtmlWindow( panel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                                 wxHW_SCROLLBAR_AUTO|wxHW_NO_SELECTION );

    // HTML font set to font properties as they are used for widgets to have an unique look
    // under different platforms with HTML
    wxFont font = this->GetFont();
    htmlWindow->SetStandardFonts( font.GetPointSize(), font.GetFaceName(), font.GetFaceName() );
    htmlWindow->SetPage( htmlPage );

    // the HTML window shall not be used to open external links, thus this task is delegated
    // to users default browser
    htmlWindow->Connect( wxEVT_COMMAND_HTML_LINK_CLICKED,
                         wxHtmlLinkEventHandler( dialog_about::OnHtmlLinkClicked ), NULL, this );

    // no additional space around the HTML window as it is also the case by the other notebook pages
    bSizer->Add( htmlWindow, 1, wxALL|wxEXPAND, 0 );
    panel->SetSizer( bSizer );
    panel->Layout();
    bSizer->Fit( panel );
    aParent->AddPage( panel, aCaption, false, aIcon );
}


wxHyperlinkCtrl* dialog_about::CreateHyperlink(wxScrolledWindow* aParent, const wxString& email)
{
    wxHyperlinkCtrl* hyperlink = new wxHyperlinkCtrl(
                                        aParent, wxID_ANY,
                                        wxT( "<" ) + email + wxT( ">" ), /* the label */
                                        wxT( "mailto:" ) + email
                                        + wxT( "?subject=KiCad - " )
                                        + info.GetBuildVersion()
                                        + wxT( " ,  " ) + info.GetLibVersion()
                                        ); /* the url */

    return hyperlink;
}


wxStaticBitmap* dialog_about::CreateStaticBitmap(wxScrolledWindow* aParent, wxBitmap* aIcon)
{
    wxStaticBitmap* bitmap = new wxStaticBitmap( aParent, wxID_ANY, wxNullBitmap,
                                                 wxDefaultPosition, wxDefaultSize, 0 );

    if( aIcon )
    {
        bitmap->SetBitmap( *aIcon );
    }
    else
    {
        bitmap->SetBitmap( KiBitmap( right_xpm ) );
    }

    return bitmap;
}


///////////////////////////////////////////////////////////////////////////////
/// Event handlers
///////////////////////////////////////////////////////////////////////////////

void dialog_about::OnClose( wxCloseEvent &event )
{
    Destroy();
}


void dialog_about::OnOkClick( wxCommandEvent &event )
{
    Destroy();
}


void dialog_about::OnHtmlLinkClicked( wxHtmlLinkEvent& event )
{
    ::wxLaunchDefaultBrowser( event.GetLinkInfo().GetHref() );
}

void dialog_about::OnCopyVersionInfo( wxCommandEvent& event )
{
    if( !wxTheClipboard->Open() )
    {
        wxMessageBox( _( "Could not open clipboard to write version information." ),
                      _( "Clipboard Error" ), wxOK | wxICON_EXCLAMATION, this );
        return;
    }

    wxPlatformInfo platform;

    // DO NOT translate information in the msg_version string
    wxString msg_version;
    msg_version << "Application: " << info.GetAppName() << "\n";
    msg_version << "Version: " << info.GetBuildVersion() << "\n";
    msg_version << "Libraries: " << wxGetLibraryVersionInfo().GetVersionString() << "\n";
#ifdef BUILD_GITHUB_PLUGIN
    msg_version << "           " << KICAD_CURL::GetVersion() << "\n";
#endif
    msg_version << "Platform: " << wxGetOsDescription() << ", "
                                << platform.GetArchName() << ", "
                                << platform.GetEndiannessName() << ", "
                                << platform.GetPortIdName() << "\n";

    msg_version << "- Build Info -\n";
    msg_version << "wxWidgets: " << wxVERSION_NUM_DOT_STRING << " (";
    msg_version << __WX_BO_UNICODE __WX_BO_STL __WX_BO_WXWIN_COMPAT_2_8 ")\n";

    msg_version << "Boost: " << ( BOOST_VERSION / 100000 ) << wxT( "." )
                             << ( BOOST_VERSION / 100 % 1000 ) << wxT( "." )
                             << ( BOOST_VERSION % 100 ) << wxT( "\n" );

#ifdef BUILD_GITHUB_PLUGIN
    msg_version << "Curl: " << LIBCURL_VERSION << "\n";
#endif

    msg_version << "KiCad - Compiler: ";
#if defined(__clang__)
    msg_version << "Clang " << __clang_major__ << "." << __clang_minor__ << "." << __clang_patchlevel__;
#elif defined(__GNUG__)
    msg_version << "GCC " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__;
#elif defined(_MSC_VER)
    msg_version << "Visual C++ " << _MSC_VER;
#elif defined(__INTEL_COMPILER)
    msg_version << "Intel C++ " << __INTEL_COMPILER;
#else
    msg_version << "Other Compiler ";
#endif

#if defined(__GXX_ABI_VERSION)
    msg_version << " with C++ ABI " << __GXX_ABI_VERSION << "\n";
#else
    msg_version << " without C++ ABI\n";
#endif

    msg_version << "        Settings: ";

    #define ON "ON\n"
    #define OFF "OFF\n"

    msg_version << "USE_WX_GRAPHICS_CONTEXT=";
#ifdef USE_WX_GRAPHICS_CONTEXT
    msg_version << ON;
#else
    msg_version << OFF;
#endif

    msg_version << "                  USE_WX_OVERLAY=";
#ifdef USE_WX_OVERLAY
    msg_version << ON;
#else
    msg_version << OFF;
#endif

    msg_version << "                  KICAD_SCRIPTING=";
#ifdef KICAD_SCRIPTING
    msg_version << ON;
#else
    msg_version << OFF;
#endif

    msg_version << "                  KICAD_SCRIPTING_MODULES=";
#ifdef KICAD_SCRIPTING_MODULES
    msg_version << ON;
#else
    msg_version << OFF;
#endif

    msg_version << "                  KICAD_SCRIPTING_WXPYTHON=";
#ifdef KICAD_SCRIPTING_WXPYTHON
    msg_version << ON;
#else
    msg_version << OFF;
#endif

    msg_version << "                  BUILD_GITHUB_PLUGIN=";
#ifdef BUILD_GITHUB_PLUGIN
    msg_version << ON;
#else
    msg_version << OFF;
#endif

    msg_version << "                  KICAD_USE_SCH_IO_MANAGER=";
#ifdef KICAD_USE_SCH_IO_MANAGER
    msg_version << ON;
#else
    msg_version << OFF;
#endif

    msg_version << "                  KICAD_USE_OCE=";
#ifdef KICAD_USE_OCE
    msg_version << ON;
#else
    msg_version << OFF;
#endif

    wxTheClipboard->SetData( new wxTextDataObject( msg_version ) );
    wxTheClipboard->Close();
    copyVersionInfo->SetLabel( _( "Copied..." ) );
}
