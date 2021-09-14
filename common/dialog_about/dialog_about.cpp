/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Rafael Sokolowski <Rafael.Sokolowski@web.de>
 * Copyright (C) 2017-2021 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <wx/clipbrd.h>
#include <wx/msgdlg.h>
#include <wx/hyperlink.h>

/* All KiCad icons are linked into shared library 'libbitmaps.a'.
 *  Icons:
 *  preference_xpm;         // Icon for 'Developers' tab
 *  editor_xpm;             // Icon for 'Doc Writers' tab
 *  color_materials_xpm;    // Icon for 'Artists' tab
 *  language_xpm;           // Icon for 'Translators' tab
 *  right_xpm;              // Right arrow icon for list items
 *  info_xpm;               // Bulb for description tab
 *  tools_xpm;              // Sheet of paper icon for license info tab
 */
#include <bitmaps.h>
#include <build_version.h>
#include <dialogs/html_message_box.h>
#include <tool/tool_manager.h>

#include "dialog_about.h"


DIALOG_ABOUT::DIALOG_ABOUT( EDA_BASE_FRAME *aParent, ABOUT_APP_INFO& aAppInfo )
    : DIALOG_ABOUT_BASE( aParent ), m_info( aAppInfo )
{
    wxASSERT( aParent != nullptr );

    // TODO: Change these to 16x16 versions when available
    m_images = new wxImageList( 24, 24, false, 9 );

    m_images->Add( KiBitmap( BITMAPS::info ) );              // INFORMATION
    m_images->Add( KiBitmap( BITMAPS::recent ) );            // VERSION
    m_images->Add( KiBitmap( BITMAPS::preference ) );        // DEVELOPERS
    m_images->Add( KiBitmap( BITMAPS::editor ) );            // DOCWRITERS
    m_images->Add( KiBitmap( BITMAPS::library ) );           // LIBRARIANS
    m_images->Add( KiBitmap( BITMAPS::color_materials ) );   // ARTISTS
    m_images->Add( KiBitmap( BITMAPS::language ) );          // TRANSLATORS
    m_images->Add( KiBitmap( BITMAPS::zip ) );               // PACKAGERS
    m_images->Add( KiBitmap( BITMAPS::tools ) );             // LICENSE

    m_notebook->SetImageList( m_images );

    if( m_info.GetAppIcon().IsOk() )
    {
        SetIcon( m_info.GetAppIcon() );
        m_bitmapApp->SetBitmap( m_info.GetAppIcon() );
    }
    else
    {
        wxIcon icon;
        icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_kicad ) );
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
    SetFocus();
    Centre();
}


DIALOG_ABOUT::~DIALOG_ABOUT()
{
    delete m_images;
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
    createNotebookHtmlPage( m_notebook, _( "About" ), IMAGES::INFORMATION,
            m_info.GetDescription() );

    wxString version = GetVersionInfoData( m_titleName, true );

    createNotebookHtmlPage( m_notebook, _( "Version" ), IMAGES::VERSION, version, true );

    createNotebookPageByCategory( m_notebook, _( "Developers" ) , IMAGES::DEVELOPERS,
                        m_info.GetDevelopers() );
    createNotebookPage( m_notebook, _( "Doc Writers" ), IMAGES::DOCWRITERS,
                        m_info.GetDocWriters() );

    createNotebookPageByCategory( m_notebook, _( "Librarians" ), IMAGES::LIBRARIANS,
                                  m_info.GetLibrarians() );

    createNotebookPageByCategory( m_notebook, _( "Artists" ), IMAGES::ARTISTS,
                                  m_info.GetArtists() );
    createNotebookPageByCategory( m_notebook, _( "Translators" ), IMAGES::TRANSLATORS,
                                  m_info.GetTranslators() );
    createNotebookPageByCategory( m_notebook, _( "Packagers" ), IMAGES::PACKAGERS,
                                  m_info.GetPackagers() );

    createNotebookHtmlPage( m_notebook, _( "License" ), IMAGES::LICENSE, m_info.GetLicense() );
}

void DIALOG_ABOUT::createNotebookPage( wxNotebook* aParent, const wxString& aCaption,
                                       IMAGES aIconIndex, const CONTRIBUTORS& aContributors )
{
    wxPanel* outerPanel = new wxPanel( aParent );
    wxBoxSizer* outerSizer = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* bSizer = new wxBoxSizer( wxHORIZONTAL );

    wxScrolledWindow* m_scrolledWindow1 = new wxScrolledWindow( outerPanel, wxID_ANY,
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
        if ( contributor->GetExtra() != wxEmptyString )
        {
            wxStaticText* hyperlink = wxStaticTextRef( m_scrolledWindow1,
                                                        contributor->GetExtra() );
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

    outerSizer->Add( m_scrolledWindow1, 1, wxEXPAND, 0 );
    outerPanel->SetSizer( outerSizer );

    aParent->AddPage( outerPanel, aCaption, false, static_cast<int>( aIconIndex ) );
}


void DIALOG_ABOUT::createNotebookPageByCategory( wxNotebook* aParent, const wxString& aCaption,
                                                 IMAGES aIconIndex,
                                                 const CONTRIBUTORS& aContributors )
{
    // The left justification between wxStaticText and wxHyperlinkCtrl is different so
    // we must pad to make the alignment look decent.
    //
    // @todo Just make all of the contributor lists HTML so the alignment is consistent.
    wxString padding;

    // Of course the padding is different depending on the platform so we adjust the
    // padding accordingly.
#if defined( __WXGTK__ )
    padding += "      ";
#endif
    wxPanel* outerPanel = new wxPanel( aParent );
    wxBoxSizer* outerSizer = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* bSizer = new wxBoxSizer( wxHORIZONTAL );

    wxScrolledWindow* m_scrolledWindow1 = new wxScrolledWindow( outerPanel, wxID_ANY,
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
            m_staticText1->SetFont( m_staticText1->GetFont().Bold() );
            m_staticText1->Wrap( -1 );
            fgSizer1->Add( m_staticText1, 0, wxALIGN_LEFT|wxBOTTOM|wxEXPAND, 2 );

            // Nothing at third column
            fgSizer1->AddSpacer( 5 );

            // Now, all contributors of the same category will follow
            for( size_t j=0; j < aContributors.GetCount(); ++j )
            {
                CONTRIBUTOR* sub_contributor = &aContributors.Item( j );

                if ( sub_contributor->GetCategory() == category )
                {
                    // First column is empty
                    fgSizer1->AddSpacer( 5 );

                    wxControl* ctrl;

                    // No URL supplied, display normal text control
                    if( sub_contributor->GetUrl().IsEmpty() )
                    {
                        ctrl = new wxStaticText( m_scrolledWindow1, wxID_ANY,
                                                 padding + wxT( "• " ) + sub_contributor->GetName(),
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
                                                    wxDefaultSize,
                                                    wxBORDER_NONE | wxHL_CONTEXTMENU | wxHL_ALIGN_LEFT );
                    }

                    m_staticText1->Wrap( -1 );

                    fgSizer1->Add( ctrl, 0, wxALIGN_LEFT|wxBOTTOM, 2 );

                    // Email address of contributor at third column
                    if( sub_contributor->GetExtra() != wxEmptyString )
                    {
                        wxStaticText* mail = wxStaticTextRef( m_scrolledWindow1,
                                                               sub_contributor->GetExtra() );
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
        if ( contributor->GetExtra() != wxEmptyString )
        {
            wxStaticText* mail = wxStaticTextRef( m_scrolledWindow1,
                                                   contributor->GetExtra() );
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

    outerSizer->Add( m_scrolledWindow1, 1, wxEXPAND, 0 );
    outerPanel->SetSizer( outerSizer );

    aParent->AddPage( outerPanel, aCaption, false, static_cast<int>( aIconIndex ) );
}


void DIALOG_ABOUT::createNotebookHtmlPage( wxNotebook* aParent, const wxString& aCaption,
                                           IMAGES aIconIndex, const wxString& html,
                                           bool aSelection )
{
    wxPanel* panel = new wxPanel( aParent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                  wxTAB_TRAVERSAL );

    wxBoxSizer* bSizer = new wxBoxSizer( wxVERTICAL );

    wxString htmlPage = wxEmptyString, htmlContent = html;

    // to have a unique look background color for HTML pages is set to the default as it is
    // used for all the other widgets
    wxString htmlColor = ( GetBackgroundColour() ).GetAsString( wxC2S_HTML_SYNTAX );
    wxString textColor = GetForegroundColour().GetAsString( wxC2S_HTML_SYNTAX );
    wxString linkColor =
            wxSystemSettings::GetColour( wxSYS_COLOUR_HOTLIGHT ).GetAsString( wxC2S_HTML_SYNTAX );

    // beginning of HTML structure
    htmlPage.Append( wxString::Format( wxT( "<html><body bgcolor='%s' text='%s' link='%s'>" ),
                                       htmlColor, textColor, linkColor ) );

    htmlPage.Append( htmlContent );

    // end of HTML structure indicated by closing tags
    htmlPage.Append( wxT( "</body></html>" ) );

    int flags = aSelection ? wxHW_SCROLLBAR_AUTO : ( wxHW_SCROLLBAR_AUTO | wxHW_NO_SELECTION );

    // the HTML page is going to be created with previously created HTML content
    auto htmlWindow = new wxHtmlWindow( panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, flags );

    // HTML font set to font properties as they are used for widgets to have an unique look
    // under different platforms with HTML
    wxFont font = GetFont();
    htmlWindow->SetStandardFonts( font.GetPointSize(), font.GetFaceName(), font.GetFaceName() );
    htmlWindow->SetPage( htmlPage );

    // the HTML window shall not be used to open external links, thus this task is delegated
    // to users default browser
    htmlWindow->Connect( wxEVT_COMMAND_HTML_LINK_CLICKED,
                         wxHtmlLinkEventHandler( DIALOG_ABOUT::onHtmlLinkClicked ), NULL, this );

    // no additional space around the HTML window as it is also the case by the other notebook pages
    bSizer->Add( htmlWindow, 1, wxEXPAND, 0 );
    panel->SetSizer( bSizer );

    aParent->AddPage( panel, aCaption, false, static_cast<int>( aIconIndex ) );
}


wxStaticText* DIALOG_ABOUT::wxStaticTextRef( wxScrolledWindow* aParent, const wxString& aReference )
{
    wxStaticText* text = new wxStaticText( aParent, wxID_ANY,
                                           wxT( "(" ) + aReference + wxT( ")" ) );

    return text;
}


wxStaticBitmap* DIALOG_ABOUT::createStaticBitmap( wxScrolledWindow* aParent, wxBitmap* aIcon )
{
    wxStaticBitmap* bitmap = new wxStaticBitmap( aParent, wxID_ANY, wxNullBitmap,
                                                 wxDefaultPosition, wxDefaultSize, 0 );

    if( aIcon )
    {
        bitmap->SetBitmap( *aIcon );
    }
    else
    {
        bitmap->SetBitmap( KiBitmap( BITMAPS::right ) );
    }

    return bitmap;
}


void DIALOG_ABOUT::onHtmlLinkClicked( wxHtmlLinkEvent& event )
{
    ::wxLaunchDefaultBrowser( event.GetLinkInfo().GetHref() );
}


void DIALOG_ABOUT::onCopyVersionInfo( wxCommandEvent& event )
{
    wxLogNull doNotLog; // disable logging of failed clipboard actions

    if( !wxTheClipboard->Open() )
    {
        wxMessageBox( _( "Could not open clipboard to write version information." ),
                      _( "Clipboard Error" ), wxOK | wxICON_EXCLAMATION, this );
        return;
    }

    wxString msg_version = GetVersionInfoData( m_titleName );

    wxTheClipboard->SetData( new wxTextDataObject( msg_version ) );
    wxTheClipboard->Flush(); // Allow clipboard data to be available after KiCad closes
    wxTheClipboard->Close();
    m_btCopyVersionInfo->SetLabel( _( "Copied..." ) );
}


void DIALOG_ABOUT::onReportBug( wxCommandEvent& event )
{
    if( TOOL_MANAGER* mgr = static_cast<EDA_BASE_FRAME*>( GetParent() )->GetToolManager() )
        mgr->RunAction( "common.SuiteControl.reportBug", true );
}


void DIALOG_ABOUT::OnNotebookPageChanged( wxNotebookEvent& aEvent )
{
    // Work around wxMac issue where the notebook pages are blank
#ifdef __WXMAC__
    int page = aEvent.GetSelection();

    if( page >= 0 )
        m_notebook->ChangeSelection( static_cast<unsigned>( page ) );
#endif
}
