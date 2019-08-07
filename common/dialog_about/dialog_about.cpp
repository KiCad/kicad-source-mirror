/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Rafael Sokolowski <Rafael.Sokolowski@web.de>
 * Copyright (C) 2017-2019 KiCad Developers, see CHANGELOG.TXT for contributors.
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
#include <string>

// kicad_curl.h must be included before wx headers, to avoid
// conflicts for some defines, at least on Windows
#ifdef BUILD_GITHUB_PLUGIN
// kicad_curl.h can create conflicts for some defines, at least on Windows
// so we are using here 2 proxy functions to know Curl version to avoid
// including kicad_curl.h to know Curl version
extern std::string GetKicadCurlVersion();
extern std::string GetCurlLibVersion();
#endif
#if defined( KICAD_USE_OCC ) | defined( KICAD_USE_OCE )
#include <Standard_Version.hxx>
#endif

#include <boost/version.hpp>
#include <wx/clipbrd.h>
#include <wx/msgdlg.h>
#include <wx/hyperlink.h>

/* All KiCad icons are linked into shared library 'libbitmaps.a'.
 *  Icons:
 *  preference_xpm;    // Icon for 'Developers' tab
 *  editor_xpm;        // Icon for 'Doc Writers' tab
 *  palette_xpm;       // Icon for 'Artists' tab
 *  language_xpm;      // Icon for 'Translators' tab
 *  right_xpm;         // Right arrow icon for list items
 *  info_xpm;          // Bulb for description tab
 *  tools_xpm;         // Sheet of paper icon for license info tab
 */
#include <bitmaps.h>
#include <build_version.h>
#include <html_messagebox.h>

#include "dialog_about.h"


/*
 * Class DIALOG_ABOUT methods
 */

DIALOG_ABOUT::DIALOG_ABOUT( EDA_BASE_FRAME *aParent, ABOUT_APP_INFO& aAppInfo )
    : DIALOG_ABOUT_BASE( aParent ), m_info( aAppInfo )
{
    wxASSERT( aParent != nullptr );

    m_picInformation = KiBitmap( info_xpm );
    m_picDevelopers  = KiBitmap( preference_xpm );
    m_picDocWriters  = KiBitmap( editor_xpm );
    m_picArtists     = KiBitmap( palette_xpm );
    m_picTranslators = KiBitmap( language_xpm );
    m_picLicense     = KiBitmap( tools_xpm );
    m_picPackagers   = KiBitmap( zip_xpm );

    if( m_info.GetAppIcon().IsOk() )
    {
        SetIcon( m_info.GetAppIcon() );
        m_bitmapApp->SetBitmap( m_info.GetAppIcon() );
    }
    else
    {
        wxIcon icon;
        icon.CopyFromBitmap( KiBitmap( icon_kicad_xpm ) );
        SetIcon( icon );
        m_bitmapApp->SetBitmap( icon );
    }

    m_titleName = aParent->GetAboutTitle();
    m_staticTextAppTitle->SetLabel( m_titleName );
    m_staticTextCopyright->SetLabel( m_info.GetCopyright() );
    m_staticTextBuildVersion->SetLabel( "Version: " + m_info.GetBuildVersion() );
    m_staticTextLibVersion->SetLabel( m_info.GetLibVersion() );

    SetTitle( wxString::Format( _( "About %s" ), m_titleName ) );
    createNotebooks();

    GetSizer()->SetSizeHints( this );
    m_auiNotebook->Update();
    SetFocus();
    Centre();
}


DIALOG_ABOUT::~DIALOG_ABOUT()
{
}


wxFlexGridSizer* DIALOG_ABOUT::createFlexGridSizer()
{
    // three columns with vertical and horizontal extra space of two pixels
    wxFlexGridSizer* fgSizer = new wxFlexGridSizer( 3, 2, 2 );
    fgSizer->SetFlexibleDirection( wxHORIZONTAL );
    fgSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    return fgSizer;
}


void DIALOG_ABOUT::createNotebooks()
{
    createNotebookHtmlPage( m_auiNotebook, _( "Information" ), m_picInformation,
                            m_info.GetDescription() );

    createNotebookPage( m_auiNotebook, _( "Developers" ) , m_picDevelopers,
                        m_info.GetDevelopers() );
    createNotebookPage( m_auiNotebook, _( "Doc Writers" ), m_picDocWriters,
                        m_info.GetDocWriters() );

    createNotebookPageByCategory( m_auiNotebook, _( "Artists" ), m_picArtists,
                                  m_info.GetArtists() );
    createNotebookPageByCategory( m_auiNotebook, _( "Translators" ), m_picTranslators,
                                  m_info.GetTranslators() );
    createNotebookPageByCategory( m_auiNotebook, _( "Packagers" ), m_picPackagers,
                                  m_info.GetPackagers() );

    createNotebookHtmlPage( m_auiNotebook, _( "License" ), m_picLicense, m_info.GetLicense() );
}

void DIALOG_ABOUT::createNotebookPage( wxAuiNotebook* aParent, const wxString& aCaption,
                                       const wxBitmap& aIcon, const CONTRIBUTORS& aContributors )
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

    wxFlexGridSizer* fgSizer1 = createFlexGridSizer();

    for( size_t i=0; i<aContributors.GetCount(); ++i )
    {
        CONTRIBUTOR* contributor = &aContributors.Item( i );

        // Icon at first column
        wxStaticBitmap* m_bitmap1 = createStaticBitmap( m_scrolledWindow1, contributor->GetIcon() );
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
            wxStaticText* hyperlink = wxStaticTextMail( m_scrolledWindow1,
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


void DIALOG_ABOUT::createNotebookPageByCategory(wxAuiNotebook* aParent, const wxString& aCaption,
                                                const wxBitmap& aIcon,
                                                const CONTRIBUTORS& aContributors)
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

    wxFlexGridSizer* fgSizer1 = createFlexGridSizer();

    for( size_t i=0; i < aContributors.GetCount(); ++i )
    {
        CONTRIBUTOR* contributor = &aContributors.Item( i );

        wxBitmap* icon = contributor->GetIcon();
        wxString category = contributor->GetCategory();

        /* to construct the next row we expect to have
         * a category and a contributor that was not considered up to now
         */
        if( ( category != wxEmptyString ) && !( contributor->IsChecked() ) )
        {
            // Icon at first column
            wxStaticBitmap* m_bitmap1 = createStaticBitmap( m_scrolledWindow1, icon );
            fgSizer1->Add( m_bitmap1, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT, 5 );

            // Category name at second column
            wxStaticText* m_staticText1 = new wxStaticText( m_scrolledWindow1, wxID_ANY,
                                                            contributor->GetCategory() + wxT( ":" ),
                                                            wxDefaultPosition, wxDefaultSize, 0 );
            m_staticText1->SetFont( wxFont( -1, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                            wxFONTWEIGHT_BOLD, false,
                                            wxEmptyString ) ); // bold font
            m_staticText1->Wrap( -1 );
            fgSizer1->Add( m_staticText1, 0, wxALIGN_LEFT|wxBOTTOM, 2 );

            // Nothing at third column
            fgSizer1->AddSpacer( 5 );

            // Now, all contributors of the same category will follow
            for( size_t j=0; j < aContributors.GetCount(); ++j )
            {
                CONTRIBUTOR* sub_contributor = &aContributors.Item( j );

                if ( sub_contributor->GetCategory() == category )
                {
                    // First column is empty
                    fgSizer1->AddSpacer(5);

                    wxControl* ctrl;

                    // No URL supplied, display normal text control
                    if( sub_contributor->GetUrl().IsEmpty() )
                    {
                        ctrl = new wxStaticText( m_scrolledWindow1, wxID_ANY,
                                                 wxT( "  • " ) + sub_contributor->GetName(),
                                                 wxDefaultPosition,
                                                 wxDefaultSize, 0 );
                    }
                    else
                    {
                        // Display a hyperlink control instead
                        ctrl = new wxHyperlinkCtrl( m_scrolledWindow1, wxID_ANY,
                                                    wxT( "• " ) + sub_contributor->GetName(),
                                                    sub_contributor->GetUrl(),
                                                    wxDefaultPosition,
                                                    wxDefaultSize, wxHL_ALIGN_LEFT );
                    }

                    m_staticText1->Wrap( -1 );

                    fgSizer1->Add( ctrl, 0, wxALIGN_LEFT|wxBOTTOM, 2 );

                    // Email address of contributor at third column
                    if( sub_contributor->GetEMail() != wxEmptyString )
                    {
                        wxStaticText* mail = wxStaticTextMail( m_scrolledWindow1,
                                                               sub_contributor->GetEMail() );
                        fgSizer1->Add( mail, 0, wxALIGN_LEFT|wxBOTTOM, 2 );
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
        CONTRIBUTOR* contributor = &aContributors.Item( k );

        if ( contributor->IsChecked() )
            continue;

        // Icon at first column
        wxStaticBitmap* m_bitmap1 = createStaticBitmap( m_scrolledWindow1, contributor->GetIcon() );
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
            wxStaticText* mail = wxStaticTextMail( m_scrolledWindow1,
                                                   contributor->GetEMail() );
            fgSizer1->Add( mail, 0, wxALIGN_LEFT|wxBOTTOM, 2 );
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


void DIALOG_ABOUT::createNotebookHtmlPage( wxAuiNotebook* aParent, const wxString& aCaption,
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
                         wxHtmlLinkEventHandler( DIALOG_ABOUT::onHtmlLinkClicked ), NULL, this );

    // no additional space around the HTML window as it is also the case by the other notebook pages
    bSizer->Add( htmlWindow, 1, wxALL|wxEXPAND, 0 );
    panel->SetSizer( bSizer );
    panel->Layout();
    bSizer->Fit( panel );
    aParent->AddPage( panel, aCaption, false, aIcon );
}


wxStaticText* DIALOG_ABOUT::wxStaticTextMail(wxScrolledWindow* aParent, const wxString& aEmail)
{
    wxStaticText* text = new wxStaticText( aParent, wxID_ANY,
                                           wxT( "<" ) + aEmail + wxT( ">" ) );

    return text;
}


wxStaticBitmap* DIALOG_ABOUT::createStaticBitmap(wxScrolledWindow* aParent, wxBitmap* aIcon)
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


void DIALOG_ABOUT::onHtmlLinkClicked( wxHtmlLinkEvent& event )
{
    ::wxLaunchDefaultBrowser( event.GetLinkInfo().GetHref() );
}


void DIALOG_ABOUT::buildVersionInfoData( wxString& aMsg, bool aFormatHtml )
{
    // DO NOT translate information in the msg_version string

    wxString eol = aFormatHtml ? "<br>" : "\n";
    wxString indent4 = aFormatHtml ? "&nbsp;&nbsp;&nbsp;&nbsp;" : "    ";

    #define ON "ON" << eol
    #define OFF "OFF" << eol

    wxPlatformInfo platform;
    aMsg << "Application: " << m_titleName << eol;
    aMsg << "Version: " << m_info.GetBuildVersion() << eol;
    aMsg << "Libraries:" << eol;
    aMsg << indent4 << wxGetLibraryVersionInfo().GetVersionString() << eol;

#ifdef BUILD_GITHUB_PLUGIN
    aMsg << indent4 << GetKicadCurlVersion() << eol;
#endif
    aMsg << "Platform: " << wxGetOsDescription() << ", "
         << platform.GetArchName() << ", "
         << platform.GetEndiannessName() << ", "
         << platform.GetPortIdName() << eol;

    aMsg << "Build Info:" << eol;
    aMsg << indent4 << "wxWidgets: " << wxVERSION_NUM_DOT_STRING << " (";
    aMsg << __WX_BO_UNICODE __WX_BO_STL __WX_BO_WXWIN_COMPAT_2_8 ")";

    // Get the GTK+ version where possible.
#ifdef __WXGTK__
    int major, minor;

    major = wxPlatformInfo().Get().GetToolkitMajorVersion();
    minor = wxPlatformInfo().Get().GetToolkitMinorVersion();
    aMsg << " GTK+ " <<  major << "." << minor;
#endif

    aMsg << eol;

    aMsg << indent4 << "Boost: " << ( BOOST_VERSION / 100000 ) << wxT( "." )
                      << ( BOOST_VERSION / 100 % 1000 ) << wxT( "." )
                      << ( BOOST_VERSION % 100 ) << eol;

#ifdef KICAD_USE_OCC
    aMsg << indent4 << "OpenCASCADE Technology: " << OCC_VERSION_COMPLETE << eol;
#endif

#ifdef KICAD_USE_OCE
    aMsg << indent4 << "OpenCASCADE Community Edition: " << OCC_VERSION_COMPLETE << eol;
#endif

#ifdef BUILD_GITHUB_PLUGIN
    aMsg << indent4 << "Curl: " << GetCurlLibVersion() << eol;
#endif

    aMsg << indent4 << "Compiler: ";
#if defined(__clang__)
    aMsg << "Clang " << __clang_major__ << "." << __clang_minor__ << "." << __clang_patchlevel__;
#elif defined(__GNUG__)
    aMsg << "GCC " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__;
#elif defined(_MSC_VER)
    aMsg << "Visual C++ " << _MSC_VER;
#elif defined(__INTEL_COMPILER)
    aMsg << "Intel C++ " << __INTEL_COMPILER;
#else
    aMsg << "Other Compiler ";
#endif

#if defined(__GXX_ABI_VERSION)
    aMsg << " with C++ ABI " << __GXX_ABI_VERSION << eol;
#else
    aMsg << " without C++ ABI";
#endif

    aMsg << eol;

    // Add build settings config (build options):
    aMsg << "Build settings:" << eol;

    aMsg << indent4 << "KICAD_SCRIPTING=";
#ifdef KICAD_SCRIPTING
    aMsg << ON;
#else
    aMsg << OFF;
#endif

    aMsg << indent4 << "KICAD_SCRIPTING_MODULES=";
#ifdef KICAD_SCRIPTING_MODULES
    aMsg << ON;
#else
    aMsg << OFF;
#endif

    aMsg << indent4 << "KICAD_SCRIPTING_PYTHON3=";
#ifdef KICAD_SCRIPTING_PYTHON3
    aMsg << ON;
#else
    aMsg << OFF;
#endif

    aMsg << indent4 << "KICAD_SCRIPTING_WXPYTHON=";
#ifdef KICAD_SCRIPTING_WXPYTHON
    aMsg << ON;
#else
    aMsg << OFF;
#endif

    aMsg << indent4 << "KICAD_SCRIPTING_WXPYTHON_PHOENIX=";
#ifdef KICAD_SCRIPTING_WXPYTHON_PHOENIX
    aMsg << ON;
#else
    aMsg << OFF;
#endif

    aMsg << indent4 << "KICAD_SCRIPTING_ACTION_MENU=";
#ifdef KICAD_SCRIPTING_ACTION_MENU
    aMsg << ON;
#else
    aMsg << OFF;
#endif

    aMsg << indent4 << "BUILD_GITHUB_PLUGIN=";
#ifdef BUILD_GITHUB_PLUGIN
    aMsg << ON;
#else
    aMsg << OFF;
#endif

    aMsg << indent4 << "KICAD_USE_OCE=";
#ifdef KICAD_USE_OCE
    aMsg << ON;
#else
    aMsg << OFF;
#endif

    aMsg << indent4 << "KICAD_USE_OCC=";
#ifdef KICAD_USE_OCC
    aMsg << ON;
#else
    aMsg << OFF;
#endif

    aMsg << indent4 << "KICAD_SPICE=";
#ifdef KICAD_SPICE
    aMsg << ON;
#else
    aMsg << OFF;
#endif

#ifndef NDEBUG
        aMsg << indent4 << "KICAD_STDLIB_DEBUG=";
    #ifdef KICAD_STDLIB_DEBUG
        aMsg << ON;
    #else
        aMsg << OFF;
        aMsg << indent4 << "KICAD_STDLIB_LIGHT_DEBUG=";
        #ifdef KICAD_STDLIB_LIGHT_DEBUG
            aMsg << ON;
        #else
            aMsg << OFF;
        #endif
    #endif

        aMsg << indent4 << "KICAD_SANITIZE=";
    #ifdef KICAD_SANITIZE
        aMsg << ON;
    #else
        aMsg << OFF;
    #endif
#endif
}


void DIALOG_ABOUT::onShowVersionInfo( wxCommandEvent& event )
{
    wxString msg_version;
    buildVersionInfoData( msg_version, true );

    HTML_MESSAGE_BOX dlg( this, _( "Version Info" ));
    dlg.SetDialogSizeInDU( 400, 350 );

    dlg.AddHTML_Text( msg_version );
    dlg.ShowModal();
}


void DIALOG_ABOUT::onCopyVersionInfo( wxCommandEvent& event )
{
    if( !wxTheClipboard->Open() )
    {
        wxMessageBox( _( "Could not open clipboard to write version information." ),
                      _( "Clipboard Error" ), wxOK | wxICON_EXCLAMATION, this );
        return;
    }

    wxString msg_version;
    buildVersionInfoData( msg_version, false );

    wxTheClipboard->SetData( new wxTextDataObject( msg_version ) );
    wxTheClipboard->Close();
    m_btCopyVersionInfo->SetLabel( _( "Copied..." ) );
}
