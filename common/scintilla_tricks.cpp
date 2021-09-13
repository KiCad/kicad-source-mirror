/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2021 KiCad Developers, see change_log.txt for contributors.
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


#include <string_utils.h>
#include <scintilla_tricks.h>
#include <wx/stc/stc.h>
#include <gal/color4d.h>
#include <dialog_shim.h>
#include <wx/clipbrd.h>
#include <wx/log.h>
#include <wx/settings.h>
#include <confirm.h>

SCINTILLA_TRICKS::SCINTILLA_TRICKS( wxStyledTextCtrl* aScintilla, const wxString& aBraces,
                                    bool aSingleLine, std::function<void()> aReturnCallback ) :
        m_te( aScintilla ),
        m_braces( aBraces ),
        m_lastCaretPos( -1 ),
        m_suppressAutocomplete( false ),
        m_singleLine( aSingleLine ),
        m_returnCallback( aReturnCallback )
{
    // A hack which causes Scintilla to auto-size the text editor canvas
    // See: https://github.com/jacobslusser/ScintillaNET/issues/216
    m_te->SetScrollWidth( 1 );
    m_te->SetScrollWidthTracking( true );

    setupStyles();

    // Set up autocomplete
    m_te->AutoCompSetIgnoreCase( true );
    m_te->AutoCompSetFillUps( m_braces[1] );
    m_te->AutoCompSetMaxHeight( 20 );

    // Hook up events
    m_te->Bind( wxEVT_STC_UPDATEUI, &SCINTILLA_TRICKS::onScintillaUpdateUI, this );

    // Dispatch command-keys in Scintilla control.
    m_te->Bind( wxEVT_CHAR_HOOK, &SCINTILLA_TRICKS::onCharHook, this );

    m_te->Bind( wxEVT_SYS_COLOUR_CHANGED,
                wxSysColourChangedEventHandler( SCINTILLA_TRICKS::onThemeChanged ), this );
}


void SCINTILLA_TRICKS::onThemeChanged( wxSysColourChangedEvent &aEvent )
{
    setupStyles();

    aEvent.Skip();
}


void SCINTILLA_TRICKS::setupStyles()
{
    wxTextCtrl dummy( m_te->GetParent(), wxID_ANY );
    wxColour   foreground    = dummy.GetForegroundColour();
    wxColour   background    = dummy.GetBackgroundColour();
    wxColour   highlight     = wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT );
   	wxColour   highlightText = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT );

    m_te->StyleSetForeground( wxSTC_STYLE_DEFAULT, foreground );
    m_te->StyleSetBackground( wxSTC_STYLE_DEFAULT, background );
    m_te->StyleClearAll();

    m_te->SetSelForeground( true, highlightText );
    m_te->SetSelBackground( true, highlight );

    if( !m_singleLine )
    {
        // Set a monospace font with a tab width of 4.  This is the closest we can get to having
        // Scintilla mimic the stroke font's tab positioning.
        wxFont fixedFont = KIUI::GetMonospacedUIFont();

        for( size_t i = 0; i < wxSTC_STYLE_MAX; ++i )
            m_te->StyleSetFont( i, fixedFont );

        m_te->SetTabWidth( 4 );
    }

    // Set up the brace highlighting
   	unsigned char r = highlight.Red();
    unsigned char g = highlight.Green();
    unsigned char b = highlight.Blue();
   	wxColour::MakeGrey( &r, &g, &b );
   	highlight.Set( r, g, b );

    m_te->StyleSetForeground( wxSTC_STYLE_BRACELIGHT, highlightText );
    m_te->StyleSetBackground( wxSTC_STYLE_BRACELIGHT, highlight );
    m_te->StyleSetForeground( wxSTC_STYLE_BRACEBAD, *wxRED );
}


bool isCtrlSlash( wxKeyEvent& aEvent )
{
    if( !aEvent.ControlDown() || aEvent.MetaDown() )
        return false;

    if( aEvent.GetUnicodeKey() == '/' )
        return true;

    // OK, now the wxWidgets hacks start.
    // (We should abandon these if https://trac.wxwidgets.org/ticket/18911 gets resolved.)

    // Many Latin America and European keyboars have have the / over the 7.  We know that
    // wxWidgets messes this up and returns Shift+7 through GetUnicodeKey().  However, other
    // keyboards (such as France and Belgium) have 7 in the shifted position, so a Shift+7
    // *could* be legitimate.

    // However, we *are* checking Ctrl, so to assume any Shift+7 is a Ctrl-/ really only
    // disallows Ctrl+Shift+7 from doing something else, which is probably OK.  (This routine
    // is only used in the Scintilla editor, not in the rest of Kicad.)

    // The other main shifted loation of / is over : (France and Belgium), so we'll sacrifice
    // Ctrl+Shift+: too.

    if( aEvent.ShiftDown() && ( aEvent.GetUnicodeKey() == '7' || aEvent.GetUnicodeKey() == ':' ) )
        return true;

    // A few keyboards have / in an Alt position.  Since we're expressly not checking Alt for
    // up or down, those should work.  However, if they don't, there's room below for yet
    // another hack....

    return false;
}


void SCINTILLA_TRICKS::onCharHook( wxKeyEvent& aEvent )
{
    wxString c = aEvent.GetUnicodeKey();

    if( !isalpha( aEvent.GetKeyCode() ) )
        m_suppressAutocomplete = false;

    if( aEvent.GetKeyCode() == WXK_RETURN && ( m_singleLine || aEvent.ShiftDown() ) )
    {
        m_returnCallback();
    }
    else if( ConvertSmartQuotesAndDashes( &c ) )
    {
        m_te->AddText( c );
    }
    else if( aEvent.GetKeyCode() == WXK_TAB )
    {
        if( aEvent.ControlDown() )
        {
            int flags = 0;

            if( !aEvent.ShiftDown() )
                flags |= wxNavigationKeyEvent::IsForward;

            wxWindow* parent = m_te->GetParent();

            while( parent && dynamic_cast<DIALOG_SHIM*>( parent ) == nullptr )
                parent = parent->GetParent();

            if( parent )
                parent->NavigateIn( flags );
        }
        else
        {
            m_te->Tab();
        }
    }
    else if( aEvent.GetModifiers() == wxMOD_CONTROL && aEvent.GetKeyCode() == 'Z' )
    {
        m_te->Undo();
    }
    else if( ( aEvent.GetModifiers() == wxMOD_SHIFT+wxMOD_CONTROL && aEvent.GetKeyCode() == 'Z' )
            || ( aEvent.GetModifiers() == wxMOD_CONTROL && aEvent.GetKeyCode() == 'Y' ) )
    {
        m_te->Redo();
    }
    else if( aEvent.GetModifiers() == wxMOD_CONTROL && aEvent.GetKeyCode() == 'A' )
    {
        m_te->SelectAll();
    }
    else if( aEvent.GetModifiers() == wxMOD_CONTROL && aEvent.GetKeyCode() == 'X' )
    {
        m_te->Cut();
    }
    else if( aEvent.GetModifiers() == wxMOD_CONTROL && aEvent.GetKeyCode() == 'C' )
    {
        m_te->Copy();
    }
    else if( aEvent.GetModifiers() == wxMOD_CONTROL && aEvent.GetKeyCode() == 'V' )
    {
        if( m_te->GetSelectionEnd() > m_te->GetSelectionStart() )
            m_te->DeleteBack();

        wxLogNull doNotLog; // disable logging of failed clipboard actions

        if( wxTheClipboard->Open() )
        {
            if( wxTheClipboard->IsSupported( wxDF_TEXT ) )
            {
                wxTextDataObject data;
                wxString         str;

                wxTheClipboard->GetData( data );
                str = data.GetText();

                ConvertSmartQuotesAndDashes( &str );
                m_te->AddText( str );
            }

            wxTheClipboard->Close();
        }
    }
    else if( aEvent.GetKeyCode() == WXK_BACK )
    {
        m_te->DeleteBack();
    }
    else if( aEvent.GetKeyCode() == WXK_DELETE )
    {
        if( m_te->GetSelectionEnd() == m_te->GetSelectionStart() )
            m_te->CharRightExtend();

        if( m_te->GetSelectionEnd() > m_te->GetSelectionStart() )
            m_te->DeleteBack();
    }
    else if( aEvent.GetKeyCode() == WXK_ESCAPE )
    {
        if( m_te->AutoCompActive() )
        {
            m_te->AutoCompCancel();
            m_suppressAutocomplete = true; // Don't run autocomplete again on the next char...
        }
        else
        {
            aEvent.Skip();
        }
    }
    else if( isCtrlSlash( aEvent ) )
    {
        int  startLine = m_te->LineFromPosition( m_te->GetSelectionStart() );
        int  endLine = m_te->LineFromPosition( m_te->GetSelectionEnd() );
        bool comment = firstNonWhitespace( startLine ) != '#';
        int  whitespaceCount;

        m_te->BeginUndoAction();

        for( int ii = startLine; ii <= endLine; ++ii )
        {
            if( comment )
                m_te->InsertText( m_te->PositionFromLine( ii ), "#" );
            else if( firstNonWhitespace( ii, &whitespaceCount ) == '#' )
                m_te->DeleteRange( m_te->PositionFromLine( ii ) + whitespaceCount, 1 );
        }

        m_te->SetSelection( m_te->PositionFromLine( startLine ),
                            m_te->PositionFromLine( endLine ) + m_te->GetLineLength( endLine ) );

        m_te->EndUndoAction();
    }
#ifdef __WXMAC__
    else if( aEvent.GetModifiers() == wxMOD_RAW_CONTROL && aEvent.GetKeyCode() == 'A' )
    {
        m_te->LineEndWrap();
    }
    else if( aEvent.GetModifiers() == wxMOD_RAW_CONTROL && aEvent.GetKeyCode() == 'E' )
    {
        m_te->HomeWrap();
    }
#endif
    else if( aEvent.GetKeyCode() == WXK_SPECIAL20 )
    {
        // Proxy for a wxSysColourChangedEvent
        setupStyles();
    }
    else
    {
        aEvent.Skip();
    }
}


int SCINTILLA_TRICKS::firstNonWhitespace( int aLine, int* aWhitespaceCharCount )
{
    int lineStart = m_te->PositionFromLine( aLine );

    if( aWhitespaceCharCount )
        *aWhitespaceCharCount = 0;

    for( int ii = 0; ii < m_te->GetLineLength( aLine ); ++ii )
    {
        int c = m_te->GetCharAt( lineStart + ii );

        if( c == ' ' || c == '\t' )
        {
            if( aWhitespaceCharCount )
                *aWhitespaceCharCount += 1;

            continue;
        }
        else
        {
            return c;
        }
    }

    return '\r';
}


void SCINTILLA_TRICKS::onScintillaUpdateUI( wxStyledTextEvent& aEvent )
{
    auto isBrace = [this]( int c ) -> bool
                   {
                       return m_braces.Find( (wxChar) c ) >= 0;
                   };

    // Has the caret changed position?
    int caretPos = m_te->GetCurrentPos();

    if( m_lastCaretPos != caretPos )
    {
        m_lastCaretPos = caretPos;
        int bracePos1 = -1;
        int bracePos2 = -1;

        // Is there a brace to the left or right?
        if( caretPos > 0 && isBrace( m_te->GetCharAt( caretPos-1 ) ) )
            bracePos1 = ( caretPos - 1 );
        else if( isBrace( m_te->GetCharAt( caretPos ) ) )
            bracePos1 = caretPos;

        if( bracePos1 >= 0 )
        {
            // Find the matching brace
            bracePos2 = m_te->BraceMatch( bracePos1 );

            if( bracePos2 == -1 )
            {
                m_te->BraceBadLight( bracePos1 );
                m_te->SetHighlightGuide( 0 );
            }
            else
            {
                m_te->BraceHighlight( bracePos1, bracePos2 );
                m_te->SetHighlightGuide( m_te->GetColumn( bracePos1 ) );
            }
        }
        else
        {
            // Turn off brace matching
            m_te->BraceHighlight( -1, -1 );
            m_te->SetHighlightGuide( 0 );
        }
    }
}


void SCINTILLA_TRICKS::DoAutocomplete( const wxString& aPartial, const wxArrayString& aTokens )
{
    if( m_suppressAutocomplete )
        return;

    wxArrayString matchedTokens;

    wxString filter = wxT( "*" ) + aPartial.Lower() + wxT( "*" );

    for( const wxString& token : aTokens )
    {
        if( token.Lower().Matches( filter ) )
            matchedTokens.push_back( token );
    }

    if( matchedTokens.size() > 0 )
    {
        // NB: tokens MUST be in alphabetical order because the Scintilla engine is going
        // to do a binary search on them
        matchedTokens.Sort( []( const wxString& first, const wxString& second ) -> int
                            {
                                return first.CmpNoCase( second );
                            });

        m_te->AutoCompShow( aPartial.size(), wxJoin( matchedTokens, ' ' ) );
    }
}


