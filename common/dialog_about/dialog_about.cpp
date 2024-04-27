/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Rafael Sokolowski <Rafael.Sokolowski@web.de>
 * Copyright (C) 2017-2021 KiCad Developers, see AUTHORS.TXT for contributors.
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


#include <build_version.h>
#include <eda_base_frame.h>
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
#include <dialogs/html_message_box.h>
#include <tool/tool_manager.h>

#include "dialog_about.h"


DIALOG_ABOUT::DIALOG_ABOUT( EDA_BASE_FRAME *aParent, ABOUT_APP_INFO& aAppInfo ) :
        DIALOG_ABOUT_BASE( aParent ),
        m_images( nullptr ),
        m_info( aAppInfo )
{
    wxASSERT( aParent != nullptr );

    SetEvtHandlerEnabled( false );

#ifdef __WXMAC__
    // HiDPI-aware API; will be generally available in wxWidgets 3.4
    wxVector<wxBitmapBundle> images;

    images.push_back( KiBitmapBundle( BITMAPS::info ) );              // INFORMATION
    images.push_back( KiBitmapBundle( BITMAPS::recent ) );            // VERSION
    images.push_back( KiBitmapBundle( BITMAPS::preference ) );        // DEVELOPERS
    images.push_back( KiBitmapBundle( BITMAPS::editor ) );            // DOCWRITERS
    images.push_back( KiBitmapBundle( BITMAPS::library ) );           // LIBRARIANS
    images.push_back( KiBitmapBundle( BITMAPS::color_materials ) );   // ARTISTS
    images.push_back( KiBitmapBundle( BITMAPS::language ) );          // TRANSLATORS
    images.push_back( KiBitmapBundle( BITMAPS::zip ) );               // PACKAGERS
    images.push_back( KiBitmapBundle( BITMAPS::tools ) );             // LICENSE

    m_notebook->SetImages( images );
#else
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
#endif

    if( m_info.GetAppIcon().IsOk() )
    {
        SetIcon( m_info.GetAppIcon() );
        m_bitmapApp->SetBitmap( m_info.GetAppIcon() );
    }
    else
    {
        wxIcon icon;

        if( IsNightlyVersion() )
            icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_kicad_nightly ) );
        else
            icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_kicad ) );

        SetIcon( icon );
        m_bitmapApp->SetBitmap( icon );
    }

    m_titleName = aParent->GetAboutTitle();
    m_untranslatedTitleName = aParent->GetUntranslatedAboutTitle();
    m_staticTextAppTitle->SetLabel( m_titleName );
    m_staticTextCopyright->SetLabel( m_info.GetCopyright() );
    m_staticTextBuildVersion->SetLabel( wxS( "Version: " ) + m_info.GetBuildVersion() );
    m_staticTextLibVersion->SetLabel( m_info.GetLibVersion() );

    SetTitle( wxString::Format( _( "About %s" ), m_titleName ) );
    createNotebooks();

    SetEvtHandlerEnabled( true );
    GetSizer()->SetSizeHints( this );
    SetFocus();
    Centre();
}


DIALOG_ABOUT::~DIALOG_ABOUT()
{
#ifndef __WXMAC__
    delete m_images;
#endif
}


void DIALOG_ABOUT::createNotebooks()
{
    createNotebookHtmlPage( m_notebook, _( "About" ), IMAGES::INFORMATION,
                            m_info.GetDescription() );

    wxString version = GetVersionInfoData( m_untranslatedTitleName, true );

    createNotebookHtmlPage( m_notebook, _( "Version" ), IMAGES::VERSION, version, true );

    createNotebookPageByCategory( m_notebook, _( "Developers" ) , IMAGES::DEVELOPERS,
                                  m_info.GetDevelopers() );
    createNotebookPageByCategory( m_notebook, _( "Doc Writers" ), IMAGES::DOCWRITERS,
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

void DIALOG_ABOUT::createNotebookPageByCategory( wxNotebook* aParent, const wxString& aCaption,
                                                 IMAGES aIconIndex,
                                                 const CONTRIBUTORS& aContributors )
{
    wxString html;

    for( size_t i=0; i < aContributors.GetCount(); ++i )
    {
        CONTRIBUTOR* contributor = &aContributors.Item( i );
        wxString     category = contributor->GetCategory();

        // to construct the next row we expect to have a category and a contributor that was
        // not considered up to now
        if( category == wxEmptyString || contributor->IsChecked() )
            continue;

        html += wxString::Format( wxS( "<p><b><u>%s:</u></b><ul>" ),
                                  contributor->GetCategory() );

        // Now, all contributors of the same category will follow
        for( size_t j=0; j < aContributors.GetCount(); ++j )
        {
            CONTRIBUTOR* sub_contributor = &aContributors.Item( j );

            if ( sub_contributor->GetCategory() == category )
            {
                // No URL supplied, display normal text control
                if( sub_contributor->GetUrl().IsEmpty() )
                {
                    html += wxString::Format( wxS( "<li>%s</li>" ),
                                              sub_contributor->GetName() );
                }
                else
                {
                    html += wxString::Format( wxS( "<li><a href='%s'>%s</a></li>" ),
                                              sub_contributor->GetUrl(),
                                              sub_contributor->GetName() );
                }

                // this contributor was added to the GUI, thus can be ignored next time
                sub_contributor->SetChecked( true );
            }
        }

        html += wxS( "</ul></p>" );
    }

    createNotebookHtmlPage( aParent, aCaption, aIconIndex, html, true );
}


void DIALOG_ABOUT::createNotebookHtmlPage( wxNotebook* aParent, const wxString& aCaption,
                                           IMAGES aIconIndex, const wxString& html,
                                           bool aSelection )
{
    wxPanel* panel = new wxPanel( aParent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                  wxTAB_TRAVERSAL );

    wxBoxSizer* bSizer = new wxBoxSizer( wxVERTICAL );

    int flags = aSelection ? wxHW_SCROLLBAR_AUTO : ( wxHW_SCROLLBAR_AUTO | wxHW_NO_SELECTION );

    // the HTML page is going to be created with previously created HTML content
    HTML_WINDOW* htmlWindow = new HTML_WINDOW( panel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                               flags );

    // HTML font set to font properties as they are used for widgets to have an unique look
    // under different platforms with HTML
    wxFont font = GetFont();
    htmlWindow->SetStandardFonts( font.GetPointSize(), font.GetFaceName(), font.GetFaceName() );
    htmlWindow->SetPage( html );

    // the HTML window shall not be used to open external links, thus this task is delegated
    // to users default browser
    htmlWindow->Connect( wxEVT_COMMAND_HTML_LINK_CLICKED,
                         wxHtmlLinkEventHandler( DIALOG_ABOUT::onHtmlLinkClicked ), NULL, this );

    // no additional space around the HTML window as it is also the case by the other notebook
    // pages
    bSizer->Add( htmlWindow, 1, wxEXPAND, 0 );
    panel->SetSizer( bSizer );

    aParent->AddPage( panel, aCaption, false, static_cast<int>( aIconIndex ) );
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

    wxString msg_version = GetVersionInfoData( m_untranslatedTitleName );

    wxTheClipboard->SetData( new wxTextDataObject( msg_version ) );
    wxTheClipboard->Flush(); // Allow clipboard data to be available after KiCad closes
    wxTheClipboard->Close();
    m_btCopyVersionInfo->SetLabel( _( "Copied..." ) );
}


void DIALOG_ABOUT::onDonateClick( wxCommandEvent& event )
{
    if( TOOL_MANAGER* mgr = static_cast<EDA_BASE_FRAME*>( GetParent() )->GetToolManager() )
        mgr->RunAction( "common.SuiteControl.donate" );
}


void DIALOG_ABOUT::onReportBug( wxCommandEvent& event )
{
    if( TOOL_MANAGER* mgr = static_cast<EDA_BASE_FRAME*>( GetParent() )->GetToolManager() )
        mgr->RunAction( "common.SuiteControl.reportBug" );
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
