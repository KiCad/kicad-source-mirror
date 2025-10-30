/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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


#include <string_utils.h>
#include <scintilla_tricks.h>
#include <widgets/wx_grid.h>
#include <widgets/ui_common.h>
#include <wx/stc/stc.h>
#include <gal/color4d.h>
#include <dialog_shim.h>
#include <wx/clipbrd.h>
#include <wx/log.h>
#include <wx/settings.h>
#include <confirm.h>
#include <grid_tricks.h>

SCINTILLA_TRICKS::SCINTILLA_TRICKS( wxStyledTextCtrl* aScintilla, const wxString& aBraces,
                                    bool aSingleLine,
                                    std::function<void( wxKeyEvent& )> onAcceptFn,
                                    std::function<void( wxStyledTextEvent& )> onCharAddedFn ) :
        m_te( aScintilla ),
        m_braces( aBraces ),
        m_lastCaretPos( -1 ),
        m_lastSelStart( -1 ),
        m_lastSelEnd( -1 ),
        m_suppressAutocomplete( false ),
        m_singleLine( aSingleLine ),
        m_onAcceptFn( std::move( onAcceptFn ) ),
        m_onCharAddedFn( std::move( onCharAddedFn ) )
{
    // Always use LF as eol char, regardless the platform
    m_te->SetEOLMode( wxSTC_EOL_LF );

    // A hack which causes Scintilla to auto-size the text editor canvas
    // See: https://github.com/jacobslusser/ScintillaNET/issues/216
    m_te->SetScrollWidth( 1 );
    m_te->SetScrollWidthTracking( true );

    if( m_singleLine )
    {
        m_te->SetUseVerticalScrollBar( false );
        m_te->SetUseHorizontalScrollBar( false );
    }

    setupStyles();

    // Set up autocomplete
    m_te->AutoCompSetIgnoreCase( true );
    m_te->AutoCompSetMaxHeight( 20 );

    if( aBraces.Length() >= 2 )
        m_te->AutoCompSetFillUps( m_braces[1] );

    // Hook up events
    m_te->Bind( wxEVT_STC_UPDATEUI, &SCINTILLA_TRICKS::onScintillaUpdateUI, this );
    m_te->Bind( wxEVT_STC_MODIFIED, &SCINTILLA_TRICKS::onModified, this );

    // Handle autocomplete
    m_te->Bind( wxEVT_STC_CHARADDED, &SCINTILLA_TRICKS::onChar, this );
    m_te->Bind( wxEVT_STC_AUTOCOMP_CHAR_DELETED, &SCINTILLA_TRICKS::onChar, this );

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
    wxTextCtrl     dummy( m_te->GetParent(), wxID_ANY );
    KIGFX::COLOR4D foreground    = dummy.GetForegroundColour();
    KIGFX::COLOR4D background    = dummy.GetBackgroundColour();
    KIGFX::COLOR4D highlight     = wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT );
    KIGFX::COLOR4D highlightText = wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHTTEXT );

    m_te->StyleSetForeground( wxSTC_STYLE_DEFAULT, foreground.ToColour() );
    m_te->StyleSetBackground( wxSTC_STYLE_DEFAULT, background.ToColour() );
    m_te->StyleClearAll();

    // Scintilla doesn't handle alpha channel, which at least OSX uses in some highlight colours,
    // such as "graphite".
    highlight = highlight.Mix( background, highlight.a ).WithAlpha( 1.0 );
    highlightText = highlightText.Mix( background, highlightText.a ).WithAlpha( 1.0 );

    m_te->SetSelForeground( true, highlightText.ToColour() );
    m_te->SetSelBackground( true, highlight.ToColour() );
    m_te->SetCaretForeground( foreground.ToColour() );

    if( !m_singleLine )
    {
        // Set a monospace font with a tab width of 4.  This is the closest we can get to having
        // Scintilla mimic the stroke font's tab positioning.
        wxFont fixedFont = KIUI::GetMonospacedUIFont();

        for( size_t i = 0; i < wxSTC_STYLE_MAX; ++i )
            m_te->StyleSetFont( i, fixedFont );

        m_te->SetTabWidth( 4 );
    }

    // Set up the brace highlighting.  Scintilla doesn't handle alpha, so we construct our own
    // 20% wash by blending with the background.
    KIGFX::COLOR4D braceText = foreground;
    KIGFX::COLOR4D braceHighlight = braceText.Mix( background, 0.2 );

    m_te->StyleSetForeground( wxSTC_STYLE_BRACELIGHT, highlightText.ToColour() );
    m_te->StyleSetBackground( wxSTC_STYLE_BRACELIGHT, braceHighlight.ToColour() );
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

    // Many Latin America and European keyboards have have the / over the 7.  We know that
    // wxWidgets messes this up and returns Shift+7 through GetUnicodeKey().  However, other
    // keyboards (such as France and Belgium) have 7 in the shifted position, so a Shift+7
    // *could* be legitimate.

    // However, we *are* checking Ctrl, so to assume any Shift+7 is a Ctrl-/ really only
    // disallows Ctrl+Shift+7 from doing something else, which is probably OK.  (This routine
    // is only used in the Scintilla editor, not in the rest of KiCad.)

    // The other main shifted location of / is over : (France and Belgium), so we'll sacrifice
    // Ctrl+Shift+: too.

    if( aEvent.ShiftDown() && ( aEvent.GetUnicodeKey() == '7' || aEvent.GetUnicodeKey() == ':' ) )
        return true;

    // A few keyboards have / in an Alt position.  Since we're expressly not checking Alt for
    // up or down, those should work.  However, if they don't, there's room below for yet
    // another hack....

    return false;
}


void SCINTILLA_TRICKS::onChar( wxStyledTextEvent& aEvent )
{
    m_onCharAddedFn( aEvent );
}


void SCINTILLA_TRICKS::onModified( wxStyledTextEvent& aEvent )
{
    if( m_singleLine )
    {
        wxString curr_text = m_te->GetText();

        if( curr_text.Contains( wxS( "\n" ) ) || curr_text.Contains( wxS( "\r" ) ) )
        {
            // Scintilla won't allow us to call SetText() from within this event processor,
            // so we have to delay the processing.
            CallAfter( [this]()
                       {
                           wxString text = m_te->GetText();
                           int currpos = m_te->GetCurrentPos();

                           text.Replace( wxS( "\n" ), wxS( "" ) );
                           text.Replace( wxS( "\r" ), wxS( "" ) );
                           m_te->SetText( text );
                           m_te->GotoPos( currpos-1 );
                       } );
        }
    }

    if( m_singleLine || m_te->GetCurrentLine() == 0 )
    {
        // If the font is larger than the height of a single-line text box we can get issues
        // with the text disappearing every other character due to dodgy scrolling behaviour.
        CallAfter( [this]()
                   {
                       m_te->ScrollToStart();
                   } );
    }
}


void SCINTILLA_TRICKS::onCharHook( wxKeyEvent& aEvent )
{
    auto findGridTricks =
            [&]() -> GRID_TRICKS*
            {
                wxWindow* parent = m_te->GetParent();

                while( parent && !dynamic_cast<WX_GRID*>( parent ) )
                    parent = parent->GetParent();

                if( WX_GRID* grid = dynamic_cast<WX_GRID*>( parent ) )
                {
                    wxEvtHandler* handler = grid->GetEventHandler();

                    while( handler && !dynamic_cast<GRID_TRICKS*>( handler ) )
                        handler = handler->GetNextHandler();

                    if( GRID_TRICKS* gridTricks = dynamic_cast<GRID_TRICKS*>( handler ) )
                        return gridTricks;
                }

                return nullptr;
            };

    wxString c = aEvent.GetUnicodeKey();

    if( m_te->AutoCompActive() )
    {
        if( aEvent.GetKeyCode() == WXK_ESCAPE )
        {
            m_te->AutoCompCancel();
            m_suppressAutocomplete = true; // Don't run autocomplete again on the next char...
        }
        else if( aEvent.GetKeyCode() == WXK_RETURN || aEvent.GetKeyCode() == WXK_NUMPAD_ENTER )
        {
            int start = m_te->AutoCompPosStart();

            m_te->AutoCompComplete();

            int finish = m_te->GetCurrentPos();

            if( finish > start )
            {
                // Select the last substitution token (if any) in the autocompleted text

                int selStart = m_te->FindText( finish, start, "<" );
                int selEnd = m_te->FindText( finish, start, ">" );

                if( selStart > start && selEnd <= finish && selEnd > selStart )
                    m_te->SetSelection( selStart, selEnd + 1 );
            }
        }
        else
        {
            aEvent.Skip();
        }

        return;
    }

#ifdef __WXMAC__
    if( aEvent.GetModifiers() == wxMOD_RAW_CONTROL && aEvent.GetKeyCode() == WXK_SPACE )
#else
    if( aEvent.GetModifiers() == wxMOD_CONTROL && aEvent.GetKeyCode() == WXK_SPACE )
#endif
    {
        m_suppressAutocomplete = false;

        wxStyledTextEvent event;
        event.SetKey( ' ' );
        event.SetModifiers( wxMOD_CONTROL );
        m_onCharAddedFn( event );

        return;
    }

    if( !isalpha( aEvent.GetKeyCode() ) )
        m_suppressAutocomplete = false;

    if( ( aEvent.GetKeyCode() == WXK_RETURN || aEvent.GetKeyCode() == WXK_NUMPAD_ENTER )
        && ( m_singleLine || aEvent.ShiftDown() ) )
    {
        m_onAcceptFn( aEvent );
    }
    else if( ConvertSmartQuotesAndDashes( &c ) )
    {
        m_te->AddText( c );
    }
    else if( aEvent.GetKeyCode() == WXK_TAB )
    {
        wxWindow* ancestor = m_te->GetParent();

        while( ancestor && !dynamic_cast<WX_GRID*>( ancestor ) )
            ancestor = ancestor->GetParent();

        if( aEvent.ControlDown() )
        {
            int flags = 0;

            if( !aEvent.ShiftDown() )
                flags |= wxNavigationKeyEvent::IsForward;

            if( DIALOG_SHIM* dlg = dynamic_cast<DIALOG_SHIM*>( wxGetTopLevelParent( m_te ) ) )
                dlg->NavigateIn( flags );
        }
        else if( dynamic_cast<WX_GRID*>( ancestor ) )
        {
            WX_GRID* grid = static_cast<WX_GRID*>( ancestor );
            int      row = grid->GetGridCursorRow();
            int      col = grid->GetGridCursorCol();

            if( aEvent.ShiftDown() )
            {
                if( col > 0 )
                {
                    col--;
                }
                else if( row > 0 )
                {
                    col = (int) grid->GetNumberCols() - 1;
                    row--;
                }
            }
            else
            {
                if( col < (int) grid->GetNumberCols() - 1 )
                {
                    col++;
                }
                else if( row < grid->GetNumberRows() - 1 )
                {
                    col = 0;
                    row++;
                }
            }

            grid->SetGridCursor( row, col );
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

        if( wxTheClipboard->Open() )
        {
            wxTheClipboard->Flush(); // Allow data to be available after closing KiCad
            wxTheClipboard->Close();
        }
    }
    else if( aEvent.GetModifiers() == wxMOD_CONTROL && aEvent.GetKeyCode() == 'C' )
    {
        m_te->Copy();

        if( wxTheClipboard->Open() )
        {
            wxTheClipboard->Flush(); // Allow data to be available after closing KiCad
            wxTheClipboard->Close();
        }
    }
    else if( aEvent.GetModifiers() == wxMOD_CONTROL && aEvent.GetKeyCode() == 'V' )
    {
        if( m_te->GetSelectionEnd() > m_te->GetSelectionStart() )
            m_te->DeleteBack();

        GRID_TRICKS* gridTricks = nullptr;
        wxLogNull    doNotLog; // disable logging of failed clipboard actions

        if( wxTheClipboard->Open() )
        {
            if( wxTheClipboard->IsSupported( wxDF_TEXT ) ||
                wxTheClipboard->IsSupported( wxDF_UNICODETEXT ) )
            {
                wxTextDataObject data;
                wxString         str;

                wxTheClipboard->GetData( data );
                str = data.GetText();

                if( str.Contains( '\t' ) )
                    gridTricks = findGridTricks();

                if( !gridTricks )
                {
                    ConvertSmartQuotesAndDashes( &str );

                    if( m_singleLine )
                    {
                        str.Replace( wxS( "\n" ), wxEmptyString );
                        str.Replace( wxS( "\r" ), wxEmptyString );
                    }

                    m_te->BeginUndoAction();
                    m_te->AddText( str );
                    m_te->EndUndoAction();
                }
            }

            wxTheClipboard->Close();
        }

        if( gridTricks )
            gridTricks->onKeyDown( aEvent );
    }
    else if( aEvent.GetKeyCode() == WXK_BACK )
    {
        if( aEvent.GetModifiers() == wxMOD_CONTROL )
#ifdef __WXMAC__
            m_te->HomeExtend();
        else if( aEvent.GetModifiers() == wxMOD_ALT )
#endif
            m_te->WordLeftExtend();

        m_te->DeleteBack();
    }
    else if( aEvent.GetKeyCode() == WXK_DELETE )
    {
        if( m_te->GetSelectionEnd() == m_te->GetSelectionStart() )
        {
#ifndef __WXMAC__
            if( aEvent.GetModifiers() == wxMOD_CONTROL )
                m_te->WordRightExtend();
            else
#endif
                m_te->CharRightExtend();
        }

        if( m_te->GetSelectionEnd() > m_te->GetSelectionStart() )
            m_te->DeleteBack();
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
                m_te->InsertText( m_te->PositionFromLine( ii ), wxT( "#" ) );
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
        m_te->HomeWrap();
    }
    else if( aEvent.GetModifiers() == wxMOD_RAW_CONTROL && aEvent.GetKeyCode() == 'E' )
    {
        m_te->LineEndWrap();
    }
    else if( ( aEvent.GetModifiers() & wxMOD_RAW_CONTROL ) && aEvent.GetKeyCode() == 'B' )
    {
        if( aEvent.GetModifiers() & wxMOD_ALT )
            m_te->WordLeft();
        else
            m_te->CharLeft();
    }
    else if( ( aEvent.GetModifiers() & wxMOD_RAW_CONTROL ) && aEvent.GetKeyCode() == 'F' )
    {
        if( aEvent.GetModifiers() & wxMOD_ALT )
            m_te->WordRight();
        else
            m_te->CharRight();
    }
    else if( aEvent.GetModifiers() == wxMOD_RAW_CONTROL && aEvent.GetKeyCode() == 'D' )
    {
        if( m_te->GetSelectionEnd() == m_te->GetSelectionStart() )
            m_te->CharRightExtend();

        if( m_te->GetSelectionEnd() > m_te->GetSelectionStart() )
            m_te->DeleteBack();
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
    auto isBrace =
            [this]( int c ) -> bool
            {
                return m_braces.Find( (wxChar) c ) >= 0;
            };

    // Has the caret changed position?
    int caretPos = m_te->GetCurrentPos();
    int selStart = m_te->GetSelectionStart();
    int selEnd = m_te->GetSelectionEnd();

    if( m_lastCaretPos != caretPos || m_lastSelStart != selStart || m_lastSelEnd != selEnd )
    {
        m_lastCaretPos = caretPos;
        m_lastSelStart = selStart;
        m_lastSelEnd = selEnd;
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


void SCINTILLA_TRICKS::DoTextVarAutocomplete( const std::function<void( const wxString& xRef,
                                                                        wxArrayString* tokens )>& getTokensFn )
{
    wxArrayString autocompleteTokens;
    int           text_pos = m_te->GetCurrentPos();
    int           start = m_te->WordStartPosition( text_pos, true );
    wxString      partial;

    auto textVarRef =
            [&]( int pos )
            {
                return pos >= 2 && m_te->GetCharAt( pos-2 ) == '$'
                                && m_te->GetCharAt( pos-1 ) == '{';
            };

    // Check for cross-reference
    if( start > 1 && m_te->GetCharAt( start-1 ) == ':' )
    {
        int refStart = m_te->WordStartPosition( start-1, true );

        if( textVarRef( refStart ) )
        {
            partial = m_te->GetRange( start, text_pos );
            getTokensFn( m_te->GetRange( refStart, start-1 ), &autocompleteTokens );
        }
    }
    else if( textVarRef( start ) )
    {
        partial = m_te->GetTextRange( start, text_pos );
        getTokensFn( wxEmptyString, &autocompleteTokens );
    }

    DoAutocomplete( partial, autocompleteTokens );
    m_te->SetFocus();
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

        m_te->AutoCompSetSeparator( '\t' );
        m_te->AutoCompShow( aPartial.size(), wxJoin( matchedTokens, '\t' ) );
    }
}


void SCINTILLA_TRICKS::CancelAutocomplete()
{
    m_te->AutoCompCancel();
}

