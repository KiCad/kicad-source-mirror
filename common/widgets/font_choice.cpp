/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <widgets/font_choice.h>
#include <kiplatform/ui.h>
#include <font/fontconfig.h>
#include <pgm_base.h>

#include <wx/dc.h>
#include <wx/event.h>
#include <wx/fontenum.h>
#include <wx/settings.h>
#include <wx/textctrl.h>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <algorithm>
#include <cwctype>

// The "official" name of the building Kicad stroke font (always existing)
#include <font/kicad_font_name.h>

class FONT_LIST_MANAGER : public wxEvtHandler
{
public:
    static FONT_LIST_MANAGER& Get();
    wxArrayString GetFonts() const;
    void Register( FONT_CHOICE* aCtrl );
    void Unregister( FONT_CHOICE* aCtrl );

private:
    FONT_LIST_MANAGER();
    ~FONT_LIST_MANAGER();
    void Poll();
    void UpdateFonts();

    std::thread               m_thread;
    mutable std::mutex        m_mutex;
    std::condition_variable   m_cv;
    wxArrayString             m_fonts;
    std::vector<FONT_CHOICE*> m_controls;
    std::atomic<bool>         m_quit;
};

FONT_LIST_MANAGER& FONT_LIST_MANAGER::Get()
{
    static FONT_LIST_MANAGER mgr;
    return mgr;
}

wxArrayString FONT_LIST_MANAGER::GetFonts() const
{
    std::lock_guard<std::mutex> lock( m_mutex );
    return m_fonts;
}

void FONT_LIST_MANAGER::Register( FONT_CHOICE* aCtrl )
{
    std::lock_guard<std::mutex> lock( m_mutex );
    m_controls.push_back( aCtrl );
}

void FONT_LIST_MANAGER::Unregister( FONT_CHOICE* aCtrl )
{
    std::lock_guard<std::mutex> lock( m_mutex );
    auto it = std::find( m_controls.begin(), m_controls.end(), aCtrl );

    if( it != m_controls.end() )
        m_controls.erase( it );
}

FONT_LIST_MANAGER::FONT_LIST_MANAGER()
{
    m_quit = false;
    UpdateFonts();

// It appears that the polling mechanism does not work correctly
// for mingw (hangs on exit)
#ifndef __MINGW32__
    m_thread = std::thread( &FONT_LIST_MANAGER::Poll, this );
#endif
}

FONT_LIST_MANAGER::~FONT_LIST_MANAGER()
{
    {
        std::lock_guard<std::mutex> lock( m_mutex );
        m_quit = true;
    }

#ifndef __MINGW32__
    m_cv.notify_one();

    if( m_thread.joinable() )
        m_thread.join();
#endif
}

void FONT_LIST_MANAGER::Poll()
{
    std::unique_lock<std::mutex> lock( m_mutex );

    while( !m_quit )
    {
        // N.B. wait_for will unlock the mutex while waiting but lock it before continuing
        // so we need to relock before continuing in the loop
        m_cv.wait_for( lock, std::chrono::seconds( 30 ), [&] { return m_quit.load(); } );

        if( !m_quit )
        {
            lock.unlock();
            UpdateFonts();
            lock.lock();
        }
    }
}

void FONT_LIST_MANAGER::UpdateFonts()
{
    std::vector<std::string> fontNames;
    Fontconfig()->ListFonts( fontNames, std::string( Pgm().GetLanguageTag().utf8_str() ) );

    wxArrayString menuList;

    for( const std::string& name : fontNames )
        menuList.Add( wxString( name ) );

    menuList.Sort();

    // Check if fonts changed and update controls
    {
        std::lock_guard<std::mutex> lock( m_mutex );

        if( menuList == m_fonts )
            return;

        m_fonts = menuList;
    }

    CallAfter( [this]() {
        std::vector<FONT_CHOICE*> controlsCopy;

        // Copy controls list under lock protection
        {
            std::lock_guard<std::mutex> lock( m_mutex );
            controlsCopy = m_controls;
        }

        // Update controls without holding lock
        for( FONT_CHOICE* ctrl : controlsCopy )
        {
            if( ctrl && !ctrl->IsShownOnScreen() )
                ctrl->RefreshFonts();
        }
    } );
}


FONT_CHOICE::FONT_CHOICE( wxWindow* aParent, int aId, wxPoint aPosition, wxSize aSize,
                          int nChoices, wxString* aChoices, int aStyle ) :
        wxOwnerDrawnComboBox( aParent, aId, wxEmptyString, aPosition, aSize, 0, nullptr, aStyle )
{
    m_systemFontCount = nChoices;
    m_notFound = wxS( " " ) + _( "<not found>" );
    m_isFiltered = false;
    m_lastText = wxEmptyString;
    m_originalSelection = wxEmptyString;

    FONT_LIST_MANAGER::Get().Register( this );
    RefreshFonts();

    // Bind only essential events to restore functionality
    Bind( wxEVT_KEY_DOWN, &FONT_CHOICE::OnKeyDown, this );
    Bind( wxEVT_CHAR_HOOK, &FONT_CHOICE::OnCharHook, this );
    Bind( wxEVT_COMMAND_TEXT_UPDATED, &FONT_CHOICE::OnTextCtrl, this );
    Bind( wxEVT_COMBOBOX_DROPDOWN, &FONT_CHOICE::OnDropDown, this );
    Bind( wxEVT_COMBOBOX_CLOSEUP, &FONT_CHOICE::OnCloseUp, this );
    Bind( wxEVT_SET_FOCUS, &FONT_CHOICE::OnSetFocus, this );
    Bind( wxEVT_KILL_FOCUS, &FONT_CHOICE::OnKillFocus, this );
}


FONT_CHOICE::~FONT_CHOICE()
{
    FONT_LIST_MANAGER::Get().Unregister( this );
}


void FONT_CHOICE::clearList()
{
    // Do the same as wxOwnerDrawnComboBox::Clear().
    // But, on MSW, Clear() has 2 issues:
    // - it generate wxWidgets alerts
    // - it generate a OnTextCtrl event, creating also recursions, not so easy to fix.
    // We use wxOwnerDrawnComboBox::Delete that do not have these issues and can do the same job

#if defined( __WXMSW__ )
    while( GetCount() )
        Delete( GetCount() - 1 );
#else
    Clear();
#endif
}


void FONT_CHOICE::RefreshFonts()
{
    wxArrayString menuList = FONT_LIST_MANAGER::Get().GetFonts();

    wxString selection = GetValue();

    // Store the full font list for filtering
    m_fullFontList.Clear();

    if( m_systemFontCount > 1 )
        m_fullFontList.Add( _( "Default Font" ) );

    m_fullFontList.Add( KICAD_FONT_NAME );

    for( const wxString& font : menuList )
        m_fullFontList.Add( font );

    Freeze();
    clearList();

    if( m_systemFontCount > 1 )
        Append( _( "Default Font" ) );

    Append( KICAD_FONT_NAME );
    m_systemFontCount = GetCount();

    Append( menuList );

    if( !selection.IsEmpty() )
        SetStringSelection( selection );

    m_isFiltered = false;
    Thaw();
}


void FONT_CHOICE::SetFontSelection( KIFONT::FONT* aFont, bool aSilentMode )
{
    if( !aFont )
    {
        SetSelection( 0 );
    }
    else
    {
        bool result = SetStringSelection( aFont->GetName() );

        if( !result )
        {
            Append( aFont->GetName() + m_notFound );
            SetSelection( GetCount() - 1 );
        }
    }
}


bool FONT_CHOICE::HaveFontSelection() const
{
    int sel = GetSelection();

    if( sel < 0 )
        return false;

    if( GetString( sel ).EndsWith( m_notFound ) )
        return false;

    return true;
}


KIFONT::FONT* FONT_CHOICE::GetFontSelection( bool aBold, bool aItalic, bool aForDrawingSheet ) const
{
    if( GetSelection() <= 0 )
    {
        return nullptr;
    }
    else if( GetSelection() == 1 && m_systemFontCount == 2 )
    {
        return KIFONT::FONT::GetFont( KICAD_FONT_NAME, aBold, aItalic );
    }
    else
    {
        return KIFONT::FONT::GetFont( GetValue(), aBold, aItalic, nullptr,
                                      aForDrawingSheet );
    }
}


wxCoord FONT_CHOICE::OnMeasureItem( size_t aItem ) const
{
    wxString name = GetString( aItem );

    // Get default font extent
    int sysW = 0, sysH = 0;
    GetTextExtent( name, &sysW, &sysH );

    return sysH + FromDIP( 6 );
}


void FONT_CHOICE::OnDrawItem( wxDC& aDc, const wxRect& aRect, int aItem, int aFlags ) const
{
    static const wxString c_sampleString = wxS( "AaBbCcDd123456" );

    if( aItem == wxNOT_FOUND )
        return;

    wxString name = GetString( aItem );

    aDc.SetFont( wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT ) );

    // Get default font extent
    int sysW = 0, sysH = 0, sysDescent = 0;
    aDc.GetTextExtent( name, &sysW, &sysH, &sysDescent );

    // Draw the font name vertically centered
    wxRect nameRect = wxRect( aRect.x + 2, aRect.y, sysW, sysH ).CenterIn( aRect, wxVERTICAL );
    aDc.DrawText( name, nameRect.GetTopLeft() );

    if( aItem >= m_systemFontCount )
    {
        wxFont sampleFont( wxFontInfo( aDc.GetFont().GetPointSize() ).FaceName( name ) );
        aDc.SetFont( sampleFont );

        if( aFlags & wxODCB_PAINTING_SELECTED )
            aDc.SetTextForeground( wxSystemSettings::GetColour( wxSYS_COLOUR_LISTBOXHIGHLIGHTTEXT ) );
        else
            aDc.SetTextForeground( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );

        // Get sample font extent
        int sampleW = 0, sampleH = 0, sampleDescent = 0;
        aDc.GetTextExtent( name, &sampleW, &sampleH, &sampleDescent );

        // Align the baselines vertically
        aDc.DrawText( c_sampleString, nameRect.GetRight() + 15,
                      nameRect.GetBottom() - sysDescent - sampleH + sampleDescent + 1 );
    }
}


wxString FONT_CHOICE::GetStringSelection() const
{
    return GetValue();
}


void FONT_CHOICE::OnKeyDown( wxKeyEvent& aEvent )
{
    int keyCode = aEvent.GetKeyCode();

    if( keyCode == WXK_RETURN || keyCode == WXK_NUMPAD_ENTER || keyCode == WXK_ESCAPE )
    {
        if( IsPopupShown() )
        {
            // Accept current text and close popup
            Dismiss();
            return;
        }
    }
    else if( keyCode == WXK_BACK && !IsPopupShown() )
    {
        // Handle backspace when popup is not shown
        // This allows normal character-by-character deletion instead of selecting all text

        wxString currentText = GetValue();
        long selStart, selEnd;
        GetSelection( &selStart, &selEnd );

        if( selStart != selEnd )
        {
            // There's a selection - delete the selected text
            wxString newText = currentText.Left( selStart ) + currentText.Mid( selEnd );
            m_lastText = newText;  // Prevent recursive calls
            ChangeValue( newText );
            SetInsertionPoint( selStart );
            return; // Don't skip this event
        }
        else if( selStart > 0 )
        {
            // No selection - delete character before cursor
            wxString newText = currentText.Left( selStart - 1 ) + currentText.Mid( selStart );
            m_lastText = newText;  // Prevent recursive calls
            ChangeValue( newText );
            SetInsertionPoint( selStart - 1 );
            return; // Don't skip this event
        }
        // If at beginning of text, let default behavior handle it
    }

    aEvent.Skip();
}


void FONT_CHOICE::OnCharHook( wxKeyEvent& aEvent )
{
    int keyCode = aEvent.GetUnicodeKey();
    wchar_t wc = static_cast<wchar_t>( keyCode );

    // When popup is not shown, let normal text processing handle printable characters
    // The OnTextCtrl method will handle filtering and autocomplete
    if( !IsPopupShown() )
    {
        aEvent.Skip();
        return;
    }

    if( std::iswprint( wc ) && !std::iswcntrl( wc ) )
    {
        // Get current text and check if there's a selection
        wxString currentText = GetValue();
        long selStart, selEnd;
        GetSelection( &selStart, &selEnd );

        wxChar newChar = (wxChar)keyCode;
        wxString newText;

        if( selStart != selEnd )
        {
            // There's a selection - replace it with the new character
            newText = currentText.Left( selStart ) + newChar + currentText.Mid( selEnd );
        }
        else
        {
            // No selection - append to current insertion point
            long insertionPoint = GetInsertionPoint();
            newText = currentText.Left( insertionPoint ) + newChar + currentText.Mid( insertionPoint );
        }

        // Update the text control
        m_lastText = newText;  // Prevent recursive calls
        ChangeValue( newText );
        SetInsertionPoint( selStart + 1 );  // Position cursor after new character

        // Filter the font list based on new text (will handle trimming internally)
        FilterFontList( newText );

        // Try autocomplete
        DoAutoComplete( newText );

        return; // Don't skip this event
    }

    switch (keyCode)
    {
    case WXK_BACK:
    {
        wxString currentText = GetValue();
        long selStart, selEnd;
        GetSelection( &selStart, &selEnd );

        wxString newText;
        long newInsertionPoint;

        if( selStart != selEnd )
        {
            // There's a selection - delete the selected text
            newText = currentText.Left( selStart ) + currentText.Mid( selEnd );
            newInsertionPoint = selStart;
        }
        else if( selStart > 0 )
        {
            // No selection - delete character before cursor
            newText = currentText.Left( selStart - 1 ) + currentText.Mid( selStart );
            newInsertionPoint = selStart - 1;
        }
        else
        {
            return; // At beginning, can't delete
        }

        m_lastText = newText;  // Prevent recursive calls
        ChangeValue( newText );
        SetInsertionPoint( newInsertionPoint );

        // Check if trimmed text is empty
        wxString trimmedNewText = newText;
        trimmedNewText.Trim().Trim( false );

        if( trimmedNewText.IsEmpty() )
        {
            RestoreFullFontList();
        }
        else
        {
            FilterFontList( newText );
            // Don't call DoAutoComplete for backspace to avoid the loop
        }

        return; // Don't skip this event
    }
    case WXK_RETURN:
    case WXK_NUMPAD_ENTER:
    {
        Dismiss();
        return;
    }
    break;

    case WXK_ESCAPE:
    {
        // Restore to original selection or default font if original doesn't exist
        if( !m_originalSelection.IsEmpty() && FindBestMatch( m_originalSelection ) != wxNOT_FOUND )
        {
            SetStringSelection( m_originalSelection );
            m_lastText = m_originalSelection;
        }
        else
        {
            // Original font doesn't exist anymore, select default font
            wxString defaultFont = GetDefaultFontName();
            SetStringSelection( defaultFont );
            m_lastText = defaultFont;
        }

        // Restore full font list if filtered
        if( m_isFiltered )
        {
            RestoreFullFontList();
        }

        // Only dismiss if popup is actually shown
        if( IsPopupShown() )
        {
            Dismiss();
        }
        return;
    }

    default:
        break;
    }

    aEvent.Skip();
}


void FONT_CHOICE::OnTextCtrl( wxCommandEvent& aEvent )
{
    wxString currentText = GetValue();

    // Avoid recursive calls
    if( currentText == m_lastText )
    {
        aEvent.Skip();
        return;
    }

    m_lastText = currentText;

    // If popup is shown, OnCharHook handles the text input, so just skip
    if( IsPopupShown() )
    {
        aEvent.Skip();
        return;
    }

    // Trim whitespace for processing
    wxString trimmedText = currentText;
    trimmedText.Trim().Trim(false);

    // If text is empty or all whitespace, restore full list
    if( trimmedText.IsEmpty() )
    {
        RestoreFullFontList();
        aEvent.Skip();
        return;
    }

    // Filter the font list based on the text input
    FilterFontList( currentText );

    // Try to find a match for autocomplete (only when popup is not shown)
    int bestMatch = FindBestMatch( trimmedText );

    if( bestMatch != wxNOT_FOUND )
    {
        DoAutoComplete( trimmedText );
    }

    aEvent.Skip();
}


void FONT_CHOICE::OnDropDown( wxCommandEvent& aEvent )
{
    // Store the original selection when dropdown opens
    m_originalSelection = GetValue();
    aEvent.Skip();
}


void FONT_CHOICE::OnCloseUp( wxCommandEvent& aEvent )
{
    // When dropdown closes, we should only restore the full font list
    // but NOT change the current text value unless explicitly selected
    // The OnKillFocus handler will handle text validation when focus is lost

    // Reset to full font list if filtered
    if( m_isFiltered )
    {
        RestoreFullFontList();
    }

    aEvent.Skip();
}


void FONT_CHOICE::OnSetFocus( wxFocusEvent& aEvent )
{
    // When the control gains focus, select all text so user can quickly replace it
    // Only do this if we're not already showing the popup (which would indicate
    // the user is actively interacting with the dropdown)
    if( !GetValue().IsEmpty() && !IsPopupShown() )
    {
        // Use CallAfter to ensure the focus event is fully processed first
        CallAfter( [this]() {
            if( HasFocus() && !IsPopupShown() )
                SelectAll();
        } );
    }

    aEvent.Skip();
}


void FONT_CHOICE::OnKillFocus( wxFocusEvent& aEvent )
{
    // When losing focus, deselect text and validate/correct the font name

    // First, deselect any selected text
    if( GetInsertionPoint() != GetLastPosition() )
    {
        SetInsertionPointEnd();
    }

    // Get current text and trim whitespace
    wxString currentText = GetValue();
    currentText.Trim().Trim(false);

    // If text is empty, set to default font
    if( currentText.IsEmpty() )
    {
        wxString defaultFont = GetDefaultFontName();
        SetStringSelection( defaultFont );
        m_lastText = defaultFont;
        aEvent.Skip();
        return;
    }

    // Try to find exact match first
    if( FindBestMatch( currentText ) != wxNOT_FOUND )
    {
        // Exact match found, keep the current text but ensure it's properly set
        SetStringSelection( currentText );
        m_lastText = currentText;
        aEvent.Skip();
        return;
    }

    // No exact match, try to find best partial match
    wxString partialMatch = FindBestPartialMatch( currentText );

    if( !partialMatch.IsEmpty() )
    {
        SetStringSelection( partialMatch );
        m_lastText = partialMatch;
    }
    else
    {
        // No decent partial match, fall back to default font
        wxString defaultFont = GetDefaultFontName();
        SetStringSelection( defaultFont );
        m_lastText = defaultFont;
    }

    // Ensure we restore full font list if it was filtered
    if( m_isFiltered )
    {
        RestoreFullFontList();
    }

    aEvent.Skip();
}


void FONT_CHOICE::DoAutoComplete( const wxString& aText )
{
    if( aText.IsEmpty() )
        return;

    // Find the best matching font
    int bestMatch = FindBestMatch( aText );

    if( bestMatch == wxNOT_FOUND )
        return;


    wxString matchText = GetString( bestMatch );

    // Only do autocomplete if the match is longer than what we typed
    if( matchText.Length() > aText.Length() && matchText.Lower().StartsWith( aText.Lower() ) )
    {
        // Set the text with the autocompleted portion selected
        m_lastText = matchText;  // Update to prevent recursive calls
        ChangeValue( matchText );
        SetInsertionPoint( aText.Length() );
        SetSelection( aText.Length(), matchText.Length() );

        if( IsPopupShown() )
        {
            SetSelection( bestMatch );
        }
    }
}


void FONT_CHOICE::FilterFontList( const wxString& aFilter )
{
    // Trim whitespace from filter
    wxString trimmedFilter = aFilter;
    trimmedFilter.Trim().Trim(false);

    if( trimmedFilter.IsEmpty() )
    {
        RestoreFullFontList();
        return;
    }

    wxArrayString filteredList;

    // Add system fonts first
    for( int i = 0; i < m_systemFontCount; i++ )
    {
        wxString fontName = m_fullFontList[i];
        filteredList.Add( fontName );
    }

    // Add matching fonts from the full list
    for( size_t i = m_systemFontCount; i < m_fullFontList.GetCount(); i++ )
    {
        wxString fontName = m_fullFontList[i];

        if( fontName.Lower().StartsWith( trimmedFilter.Lower() ) )
            filteredList.Add( fontName );
    }

    // Preserve the current text value
    wxString currentText = GetValue();

    // Check if we had items before and now have none - this indicates we need to force refresh
    bool hadItemsBefore = GetCount() > 0;
    bool haveItemsNow = filteredList.GetCount() > 0;
    bool needsPopupRefresh = hadItemsBefore && !haveItemsNow && IsPopupShown();

    // Update the combo box with filtered list (even if empty)
    Freeze();
    clearList();

    if( haveItemsNow )
    {
        Append( filteredList );
    }
    // If no matches, leave the dropdown empty

    m_isFiltered = true;

    // Restore the text value after filtering
    if( !currentText.IsEmpty() )
    {
        ChangeValue( currentText );
        SetInsertionPointEnd();
    }

    Thaw();

    // Handle popup display
    if( needsPopupRefresh )
    {
        // We had items before but now have none - dismiss the popup
        Dismiss();
    }
    else if( !IsPopupShown() && haveItemsNow )
    {
        // Only show popup if we have items to display and control has focus
        // This prevents popup from showing during programmatic text changes
        if( HasFocus() )
        {
            Popup();
        }
    }
    else if( IsPopupShown() && !haveItemsNow )
    {
        // If popup is shown but we have no items, dismiss it
        Dismiss();
    }

    // Force a refresh to ensure the popup displays correctly only if it's shown and has items
    if( IsPopupShown() && haveItemsNow )
    {
        Update();
        Refresh();
    }
}


void FONT_CHOICE::RestoreFullFontList()
{
    if( !m_isFiltered )
        return;

    wxString selection = GetValue();

    Freeze();
    clearList();
    Append( m_fullFontList );
    m_isFiltered = false;

    if( !selection.IsEmpty() )
    {
        ChangeValue( selection );
        SetInsertionPointEnd();
    }

    Thaw();
}


int FONT_CHOICE::FindBestMatch( const wxString& aText )
{
    if( aText.IsEmpty() )
        return wxNOT_FOUND;

    // Trim whitespace from search text
    wxString trimmedText = aText;
    trimmedText.Trim().Trim(false);

    if( trimmedText.IsEmpty() )
        return wxNOT_FOUND;

    wxString lowerText = trimmedText.Lower();

    // Search in the full font list to find matches, then map to current list
    for( size_t i = 0; i < m_fullFontList.GetCount(); i++ )
    {
        wxString itemText = m_fullFontList[i].Lower();

        if( itemText.StartsWith( lowerText ) )
        {
            // Find this font in the current displayed list
            wxString fullFontName = m_fullFontList[i];

            for( unsigned int j = 0; j < GetCount(); j++ )
            {
                if( GetString( j ) == fullFontName )
                    return j;
            }
        }
    }

    return wxNOT_FOUND;
}


wxString FONT_CHOICE::FindBestPartialMatch( const wxString& aText )
{
    if( aText.IsEmpty() )
        return wxEmptyString;

    // Trim whitespace from search text
    wxString trimmedText = aText;
    trimmedText.Trim().Trim(false);

    if( trimmedText.IsEmpty() )
        return wxEmptyString;

    wxString testText = trimmedText;

    // Try progressively shorter versions of the text by removing characters from the end
    // Don't go below a minimum length of 2 characters for meaningful partial matches
    while( testText.Length() >= 2 )
    {
        wxString lowerTestText = testText.Lower();

        // Search in the full font list for a match
        for( size_t i = 0; i < m_fullFontList.GetCount(); i++ )
        {
            wxString itemText = m_fullFontList[i].Lower();

            if( itemText.StartsWith( lowerTestText ) )
            {
                // Found a match, return the full font name
                return m_fullFontList[i];
            }
        }

        // Remove the last character and try again
        testText = testText.Left( testText.Length() - 1 );
    }

    // No decent partial match found (need at least 2 characters for a meaningful match)
    return wxEmptyString;
}


wxString FONT_CHOICE::GetDefaultFontName() const
{
    // Return KiCad font name as the default
    return KICAD_FONT_NAME;
}