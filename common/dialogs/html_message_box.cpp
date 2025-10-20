/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/clipbrd.h>
#include <wx/log.h>
#include <wx/textctrl.h>
#include <wx/uri.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <algorithm>
#include <string_utils.h>
#include <dialogs/html_message_box.h>
#include <build_version.h>


HTML_MESSAGE_BOX::HTML_MESSAGE_BOX( wxWindow* aParent, const wxString& aTitle,
                                    const wxPoint& aPosition, const wxSize& aSize ) :
    DIALOG_DISPLAY_HTML_TEXT_BASE( aParent, wxID_ANY, aTitle, aPosition, aSize ),
    m_currentMatch( 0 )
{
    m_htmlWindow->SetLayoutDirection( wxLayout_LeftToRight );
    ListClear();

    m_searchPanel = new wxPanel( this );
    wxBoxSizer* searchSizer = new wxBoxSizer( wxHORIZONTAL );
    m_matchCount = new wxStaticText( m_searchPanel, wxID_ANY, wxEmptyString );
    m_searchCtrl = new wxTextCtrl( m_searchPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    m_prevBtn = new wxButton( m_searchPanel, wxID_ANY, wxS( "∧" ), wxDefaultPosition, wxDefaultSize, wxBORDER_NONE );
    m_nextBtn = new wxButton( m_searchPanel, wxID_ANY, wxS( "∨" ), wxDefaultPosition, wxDefaultSize, wxBORDER_NONE );

    // Set minimum size for buttons to make them thinner
    m_prevBtn->SetMinSize( wxSize( 25, -1 ) );
    m_nextBtn->SetMinSize( wxSize( 25, -1 ) );

    // Set minimum width for match count to ensure it's visible
    m_matchCount->SetMinSize( wxSize( 60, -1 ) );

    searchSizer->Add( m_matchCount, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
    searchSizer->Add( m_searchCtrl, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
    searchSizer->Add( m_prevBtn, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 2 );
    searchSizer->Add( m_nextBtn, 0, wxALIGN_CENTER_VERTICAL );
    m_searchPanel->SetSizer( searchSizer );
    m_searchPanel->Hide();
    GetSizer()->Insert( 0, m_searchPanel, 0, wxALIGN_RIGHT|wxTOP|wxRIGHT, 5 );

    m_searchCtrl->Bind( wxEVT_TEXT, &HTML_MESSAGE_BOX::OnSearchText, this );
    m_searchCtrl->Bind( wxEVT_TEXT_ENTER, &HTML_MESSAGE_BOX::OnNext, this );
    m_prevBtn->Bind( wxEVT_BUTTON, &HTML_MESSAGE_BOX::OnPrev, this );
    m_nextBtn->Bind( wxEVT_BUTTON, &HTML_MESSAGE_BOX::OnNext, this );

    // Gives a default logical size (the actual size depends on the display definition)
    if( aSize != wxDefaultSize )
        setSizeInDU( aSize.x, aSize.y );

    Center();

    SetupStandardButtons();

    reload();

    Bind( wxEVT_SYS_COLOUR_CHANGED,
          wxSysColourChangedEventHandler( HTML_MESSAGE_BOX::onThemeChanged ), this );
}


HTML_MESSAGE_BOX::~HTML_MESSAGE_BOX()
{
    // Prevent wxWidgets bug which fails to release when closing the window on an <esc>.
    if( m_htmlWindow->HasCapture() )
        m_htmlWindow->ReleaseMouse();
}


void HTML_MESSAGE_BOX::reload()
{
    m_htmlWindow->SetPage( m_source );
}


void HTML_MESSAGE_BOX::onThemeChanged( wxSysColourChangedEvent &aEvent )
{
    reload();

    aEvent.Skip();
}


void HTML_MESSAGE_BOX::ListClear()
{
    m_source.clear();
    reload();
}


void HTML_MESSAGE_BOX::ListSet( const wxString& aList )
{
    wxArrayString strings_list;
    wxStringSplit( aList, strings_list, wxChar( '\n' ) );

    wxString msg = wxT( "<ul>" );

    for ( unsigned ii = 0; ii < strings_list.GetCount(); ii++ )
    {
        msg += wxT( "<li>" );
        msg += strings_list.Item( ii ) + wxT( "</li>" );
    }

    msg += wxT( "</ul>" );

    m_source += msg;
    reload();
}


void HTML_MESSAGE_BOX::ListSet( const wxArrayString& aList )
{
    wxString msg = wxT( "<ul>" );

    for( unsigned ii = 0; ii < aList.GetCount(); ii++ )
    {
        msg += wxT( "<li>" );
        msg += aList.Item( ii ) + wxT( "</li>" );
    }

    msg += wxT( "</ul>" );

    m_source += msg;
    reload();
}


void HTML_MESSAGE_BOX::MessageSet( const wxString& message )
{
    wxString message_value = wxString::Format( wxT( "<b>%s</b><br>" ), message );

    m_source += message_value;
    reload();
}


void HTML_MESSAGE_BOX::AddHTML_Text( const wxString& message )
{
    m_source += message;
    reload();
}


void HTML_MESSAGE_BOX::ShowModeless()
{
    reload();

    m_sdbSizer1->Show( false );
    Layout();

    Show( true );
}


void HTML_MESSAGE_BOX::OnHTMLLinkClicked( wxHtmlLinkEvent& event )
{
    wxString href = event.GetLinkInfo().GetHref();

    if( href.StartsWith( wxS( "https://go.kicad.org/docs" ) ) )
    {
        href.Replace( wxS( "GetMajorMinorVersion" ), GetMajorMinorVersion() );
    }

    wxURI uri( href );
    wxLaunchDefaultBrowser( uri.BuildURI() );
}


void HTML_MESSAGE_BOX::OnCharHook( wxKeyEvent& aEvent )
{
    if( m_searchPanel->IsShown() )
    {
        if( aEvent.GetKeyCode() == WXK_ESCAPE )
        {
            HideSearchBar();
            return;
        }
        else if( aEvent.GetKeyCode() == WXK_RETURN || aEvent.GetKeyCode() == WXK_NUMPAD_ENTER )
        {
            wxCommandEvent evt;
            OnNext( evt );
            return;
        }
        else if( aEvent.GetKeyCode() == WXK_BACK )
        {
            long from, to;
            m_searchCtrl->GetSelection( &from, &to );

            if( from == to )
                m_searchCtrl->Remove( std::max( 0L, from - 1 ), from );
            else
                m_searchCtrl->Remove( from, to );

            return;
        }
        else if( !aEvent.HasModifiers() && wxIsprint( aEvent.GetUnicodeKey() ) )
        {
            m_searchCtrl->AppendText( wxString( (wxChar) aEvent.GetUnicodeKey() ) );
            return;
        }
    }
    else if( !aEvent.HasModifiers() && wxIsprint( aEvent.GetUnicodeKey() ) )
    {
        ShowSearchBar();
        m_searchCtrl->SetValue( wxString( (wxChar) aEvent.GetUnicodeKey() ) );
        m_currentMatch = 0;
        updateSearch();
        return;
    }

    if( aEvent.GetKeyCode() == WXK_ESCAPE )
    {
        wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
        return;
    }
    else if( aEvent.GetModifiers() == wxMOD_CONTROL && aEvent.GetKeyCode() == 'A' )
    {
        m_htmlWindow->SelectAll();
        return;
    }
    else if( aEvent.GetModifiers() == wxMOD_CONTROL && aEvent.GetKeyCode() == 'C' )
    {
        wxLogNull doNotLog; // disable logging of failed clipboard actions

        if( wxTheClipboard->Open() )
        {
            wxTheClipboard->SetData( new wxTextDataObject( m_htmlWindow->SelectionToText() ) );
            wxTheClipboard->Flush(); // Allow data to be available after closing KiCad
            wxTheClipboard->Close();
        }

        return;
    }

    aEvent.Skip();
}

void HTML_MESSAGE_BOX::OnSearchText( wxCommandEvent& aEvent )
{
    m_currentMatch = 0;
    updateSearch();
}

void HTML_MESSAGE_BOX::OnNext( wxCommandEvent& aEvent )
{
    if( !m_matchPos.empty() )
    {
        m_currentMatch = ( m_currentMatch + 1 ) % m_matchPos.size();
        updateSearch();
    }
}

void HTML_MESSAGE_BOX::OnPrev( wxCommandEvent& aEvent )
{
    if( !m_matchPos.empty() )
    {
        m_currentMatch = ( m_currentMatch + m_matchPos.size() - 1 ) % m_matchPos.size();
        updateSearch();
    }
}

void HTML_MESSAGE_BOX::ShowSearchBar()
{
    if( !m_searchPanel->IsShown() )
    {
        m_originalSource = m_source;
        m_searchPanel->Show();
        Layout();
        m_searchCtrl->SetFocus();
    }
}

void HTML_MESSAGE_BOX::HideSearchBar()
{
    if( m_searchPanel->IsShown() )
    {
        m_searchPanel->Hide();
        Layout();
        m_source = m_originalSource;
        reload();

        // Refocus on the main HTML window so user can press Escape again to close the dialog
        m_htmlWindow->SetFocus();
    }
}

void HTML_MESSAGE_BOX::updateSearch()
{
    wxString term = m_searchCtrl->GetValue();

    if( term.IsEmpty() )
    {
        m_source = m_originalSource;
        reload();
        m_matchPos.clear();
        m_matchCount->SetLabel( wxEmptyString );
        return;
    }

    m_matchPos.clear();

    // Search only in text content, not in HTML tags
    wxString termLower = term.Lower();
    size_t pos = 0;
    bool insideTag = false;

    while( pos < m_originalSource.length() )
    {
        wxChar ch = m_originalSource[pos];

        if( ch == '<' )
        {
            insideTag = true;
        }
        else if( ch == '>' )
        {
            insideTag = false;
            pos++;
            continue;
        }

        // Only search for matches when we're not inside an HTML tag
        if( !insideTag )
        {
            // Check if we have a match starting at this position
            if( pos + termLower.length() <= m_originalSource.length() )
            {
                wxString candidate = m_originalSource.Mid( pos, termLower.length() ).Lower();
                if( candidate == termLower )
                {
                    // Verify that this match doesn't span into an HTML tag
                    bool validMatch = true;
                    for( size_t i = 0; i < termLower.length(); i++ )
                    {
                        if( pos + i < m_originalSource.length() && m_originalSource[pos + i] == '<' )
                        {
                            validMatch = false;
                            break;
                        }
                    }

                    if( validMatch )
                    {
                        m_matchPos.push_back( pos );
                    }
                }
            }
        }

        pos++;
    }

    if( m_matchPos.empty() )
    {
        m_source = m_originalSource;
        reload();
        m_matchCount->SetLabel( wxS( "0/0" ) );
        return;
    }

    if( m_currentMatch >= (int) m_matchPos.size() )
        m_currentMatch = 0;

    wxString out;
    size_t start = 0;

    for( size_t i = 0; i < m_matchPos.size(); ++i )
    {
        size_t idx = m_matchPos[i];
        out += m_originalSource.Mid( start, idx - start );
        wxString matchStr = m_originalSource.Mid( idx, term.length() );

        // HTML-escape the match string to prevent HTML parsing issues
        wxString escapedMatchStr = matchStr;
        escapedMatchStr.Replace( wxS( "&" ), wxS( "&amp;" ) );
        escapedMatchStr.Replace( wxS( "<" ), wxS( "&lt;" ) );
        escapedMatchStr.Replace( wxS( ">" ), wxS( "&gt;" ) );
        escapedMatchStr.Replace( wxS( "\"" ), wxS( "&quot;" ) );

        if( (int) i == m_currentMatch )
        {
            // Use a unique anchor name for each search to avoid conflicts
            wxString anchorName = wxString::Format( wxS( "kicad_search_%d" ), m_currentMatch );
            out += wxString::Format( wxS( "<a name=\"%s\"></a><span style=\"background-color:#DDAAFF;\">%s</span>" ),
                                   anchorName, escapedMatchStr );
        }
        else
        {
            out += wxString::Format( wxS( "<span style=\"background-color:#FFFFAA;\">%s</span>" ), escapedMatchStr );
        }

        start = idx + term.length();
    }

    out += m_originalSource.Mid( start );
    m_source = out;
    reload();

    // Use CallAfter to ensure the HTML is fully loaded before scrolling
    // Only scroll if we have matches and a valid current match
    if( !m_matchPos.empty() && m_currentMatch >= 0 && m_currentMatch < (int)m_matchPos.size() )
    {
        CallAfter( [this]()
        {
            // Try to scroll to the anchor, with fallback if it fails
            wxString anchorName = wxString::Format( wxS( "kicad_search_%d" ), m_currentMatch );
            if( !m_htmlWindow->ScrollToAnchor( anchorName ) )
            {
                // If anchor scrolling fails, try to scroll to approximate position
                // Calculate approximate scroll position based on match location
                if( !m_matchPos.empty() && m_currentMatch < (int)m_matchPos.size() )
                {
                    size_t matchPos = m_matchPos[m_currentMatch];
                    size_t totalLength = m_originalSource.length();
                    if( totalLength > 0 )
                    {
                        // Scroll to approximate percentage of document
                        double ratio = (double)matchPos / (double)totalLength;
                        int scrollPos = (int)(ratio * m_htmlWindow->GetScrollRange( wxVERTICAL ));
                        m_htmlWindow->Scroll( 0, scrollPos );
                    }
                }
            }
        } );
    }

    m_matchCount->SetLabel( wxString::Format( wxS( "%d/%zu" ), m_currentMatch + 1, m_matchPos.size() ) );
}
