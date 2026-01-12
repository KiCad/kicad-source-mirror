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


#include "widgets/filter_combobox.h"

#include <wx/textctrl.h>
#include <wx/listbox.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/valtext.h>

#include <kiplatform/ui.h>
#include <widgets/ui_common.h>

wxDEFINE_EVENT( FILTERED_ITEM_SELECTED, wxCommandEvent );

#if defined( __WXOSX_MAC__ )
    #define POPUP_PADDING 2
    #define LIST_ITEM_PADDING 2
    #define LIST_PADDING 7
#elif defined( __WXMSW__ )
    #define POPUP_PADDING 0
    #define LIST_ITEM_PADDING 2
    #define LIST_PADDING 5
#else
    #define POPUP_PADDING 0
    #define LIST_ITEM_PADDING 6
    #define LIST_PADDING 5
#endif


FILTER_COMBOPOPUP::FILTER_COMBOPOPUP() :
        m_filterValidator( nullptr ),
        m_filterCtrl( nullptr ),
        m_listBox( nullptr ),
        m_minPopupWidth( -1 ),
        m_maxPopupHeight( 1000 ),
        m_focusHandler( nullptr )
{
}


bool FILTER_COMBOPOPUP::Create( wxWindow* aParent )
{
    wxPanel::Create( aParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSIMPLE_BORDER );

    wxBoxSizer* mainSizer;
    mainSizer = new wxBoxSizer( wxVERTICAL );

    wxStaticText* filterLabel = new wxStaticText( this, wxID_ANY, _( "Filter:" ) );
    mainSizer->Add( filterLabel, 0, wxEXPAND, 0 );

    m_filterCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                   wxTE_PROCESS_ENTER );
    m_filterValidator = new wxTextValidator( wxFILTER_EXCLUDE_CHAR_LIST );
    m_filterValidator->SetCharExcludes( " " );
    m_filterCtrl->SetValidator( *m_filterValidator );
    mainSizer->Add( m_filterCtrl, 0, wxEXPAND, 0 );

    m_listBox = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr,
                               wxLB_SINGLE | wxLB_NEEDED_SB );
    mainSizer->Add( m_listBox, 1, wxEXPAND | wxTOP, 2 );

    SetSizer( mainSizer );
    Layout();

    Connect( wxEVT_IDLE, wxIdleEventHandler( FILTER_COMBOPOPUP::onIdle ), nullptr, this );
    Connect( wxEVT_CHAR_HOOK, wxKeyEventHandler( FILTER_COMBOPOPUP::onKeyDown ), nullptr, this );
    Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( FILTER_COMBOPOPUP::onMouseClick ), nullptr, this );
    m_listBox->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( FILTER_COMBOPOPUP::onMouseClick ), nullptr, this );
    m_filterCtrl->Connect( wxEVT_TEXT, wxCommandEventHandler( FILTER_COMBOPOPUP::onFilterEdit ), nullptr, this );
    m_filterCtrl->Connect( wxEVT_TEXT_ENTER, wxCommandEventHandler( FILTER_COMBOPOPUP::onEnter ), nullptr, this );

    // <enter> in a ListBox comes in as a double-click on GTK
    m_listBox->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( FILTER_COMBOPOPUP::onEnter ),
                        nullptr, this );

    return true;
}


void FILTER_COMBOPOPUP::SetStringList( const wxArrayString& aStringList )
{
    m_stringList = aStringList;
    m_stringList.Sort();
    rebuildList();
}


wxString FILTER_COMBOPOPUP::GetStringValue() const
{
    return m_selectedString;
}


void FILTER_COMBOPOPUP::SetStringValue( const wxString& aNetName )
{
    if( GetWindowStyleFlag() & wxCB_READONLY )
        /* shouldn't be here (combo is read-only) */;
    else
        wxComboPopup::SetStringValue( aNetName );
}


void FILTER_COMBOPOPUP::SetSelectedString( const wxString& aString )
{
    m_selectedString = aString;
    GetComboCtrl()->SetValue( m_selectedString );
}


void FILTER_COMBOPOPUP::OnPopup()
{
    // While it can sometimes be useful to keep the filter, it's always unexpected.
    // Better to clear it.
    m_filterCtrl->Clear();

    m_listBox->SetStringSelection( GetStringValue() );
    m_filterCtrl->SetFocus();

    // The updateSize() call in GetAdjustedSize() leaves the height off-by-one for
    // some reason, so do it again.
    updateSize();
}


void FILTER_COMBOPOPUP::OnStartingKey( wxKeyEvent& aEvent )
{
    doSetFocus( m_filterCtrl );
    doStartingKey( aEvent );
}


void FILTER_COMBOPOPUP::Accept()
{
    wxString selectedString = getSelectedValue().value_or( wxEmptyString );

    Dismiss();

    // No update on empty
    if( !selectedString.IsEmpty() && selectedString != m_selectedString )
    {
        m_selectedString = selectedString;
        GetComboCtrl()->SetValue( m_selectedString );

        wxCommandEvent changeEvent( FILTERED_ITEM_SELECTED );
        wxPostEvent( GetComboCtrl(), changeEvent );
    }
}


wxSize FILTER_COMBOPOPUP::GetAdjustedSize( int aMinWidth, int aPrefHeight, int aMaxHeight )
{
    // Called when the popup is first shown.  Stash the minWidth and maxHeight so we
    // can use them later when refreshing the sizes after filter changes.
    m_minPopupWidth = aMinWidth;
    m_maxPopupHeight = aMaxHeight;

    return updateSize();
}


void FILTER_COMBOPOPUP::getListContent( wxArrayString& aListContent )
{
    const wxString filterString = getFilterValue();

    // Simple substring, case-insensitive search
    for( const wxString& str : m_stringList )
    {
        if( filterString.IsEmpty() || str.Lower().Contains( filterString.Lower() ) )
            aListContent.push_back( str );
    }
}


void FILTER_COMBOPOPUP::rebuildList()
{
    wxArrayString newList;
    getListContent( newList );

    m_listBox->Set( newList );
}


std::optional<wxString> FILTER_COMBOPOPUP::getSelectedValue() const
{
    int selection = m_listBox->GetSelection();

    if( selection >= 0 )
        return m_listBox->GetString( selection );

    return std::nullopt;
}


wxString FILTER_COMBOPOPUP::getFilterValue() const
{
    return m_filterCtrl->GetValue().Trim( true ).Trim( false );
}


wxSize FILTER_COMBOPOPUP::updateSize()
{
    int listTop    = m_listBox->GetRect().y;
    int itemHeight = KIUI::GetTextSize( wxT( "Xy" ), this ).y + LIST_ITEM_PADDING;
    int listHeight = ( (int) m_listBox->GetCount() * itemHeight ) + ( LIST_PADDING * 3 );

    if( listTop + listHeight >= m_maxPopupHeight )
        listHeight = m_maxPopupHeight - listTop - 1;

    int listWidth = m_minPopupWidth;

    for( size_t i = 0; i < m_listBox->GetCount(); ++i )
    {
        int itemWidth = KIUI::GetTextSize( m_listBox->GetString( i ), m_listBox ).x;
        listWidth = std::max( listWidth, itemWidth + LIST_PADDING * 2 );
    }

    wxSize listSize( listWidth, listHeight );
    wxSize popupSize( listWidth, listTop + listHeight );

    SetSize( popupSize );               // us
    GetParent()->SetSize( popupSize );  // the window that wxComboCtrl put us in

    m_listBox->SetMinSize( listSize );
    m_listBox->SetSize( listSize );

    return popupSize;
}


void FILTER_COMBOPOPUP::onIdle( wxIdleEvent& aEvent )
{
    // Only process when the popup is actually visible to avoid ClientToScreen warnings
    if( !IsShown() )
        return;

    // Generate synthetic (but reliable) MouseMoved events
    static wxPoint lastPos;
    wxPoint screenPos = KIPLATFORM::UI::GetMousePosition();

    if( screenPos != lastPos )
    {
        lastPos = screenPos;
        onMouseMoved( screenPos );
    }

    if( m_focusHandler )
    {
        m_filterCtrl->PushEventHandler( m_focusHandler );
        m_focusHandler = nullptr;
    }
}


// Hot-track the mouse (for focus and listbox selection)
void FILTER_COMBOPOPUP::onMouseMoved( const wxPoint aScreenPos )
{
    if( m_listBox->GetScreenRect().Contains( aScreenPos ) )
    {
        doSetFocus( m_listBox );

        wxPoint relativePos = m_listBox->ScreenToClient( aScreenPos );
        int     item = m_listBox->HitTest( relativePos );

        if( item >= 0 )
            m_listBox->SetSelection( item );
    }
    else if( m_filterCtrl->GetScreenRect().Contains( aScreenPos ) )
    {
        doSetFocus( m_filterCtrl );
    }
}


void FILTER_COMBOPOPUP::onMouseClick( wxMouseEvent& aEvent )
{
    // Accept a click event from anywhere.  Different platform implementations have
    // different foibles with regard to transient popups and their children.
    if( aEvent.GetEventObject() == m_listBox )
    {
        m_listBox->SetSelection( m_listBox->HitTest( aEvent.GetPosition() ) );
        Accept();
        return;
    }

    wxWindow* window = dynamic_cast<wxWindow*>( aEvent.GetEventObject() );

    if( window )
    {
        wxPoint screenPos = window->ClientToScreen( aEvent.GetPosition() );

        if( m_listBox->GetScreenRect().Contains( screenPos ) )
        {
            wxPoint localPos = m_listBox->ScreenToClient( screenPos );

            m_listBox->SetSelection( m_listBox->HitTest( localPos ) );
            Accept();
        }
    }
}


void FILTER_COMBOPOPUP::onKeyDown( wxKeyEvent& aEvent )
{
    switch( aEvent.GetKeyCode() )
    {
    // Control keys go to the parent combobox
    case WXK_TAB:
        Dismiss();

        m_parent->NavigateIn( ( aEvent.ShiftDown() ? 0 : wxNavigationKeyEvent::IsForward ) |
                                ( aEvent.ControlDown() ? wxNavigationKeyEvent::WinChange : 0 ) );
        break;

    case WXK_ESCAPE:
        Dismiss();
        break;

    case WXK_RETURN:
    case WXK_NUMPAD_ENTER:
        Accept();
        break;

    // Arrows go to the list box
    case WXK_DOWN:
    case WXK_NUMPAD_DOWN:
        doSetFocus( m_listBox );
        m_listBox->SetSelection( std::min( m_listBox->GetSelection() + 1,
                                           (int) m_listBox->GetCount() - 1 ) );
        break;

    case WXK_UP:
    case WXK_NUMPAD_UP:
        doSetFocus( m_listBox );
        m_listBox->SetSelection( std::max( m_listBox->GetSelection() - 1, 0 ) );
        break;

    // Everything else goes to the filter textbox
    default:
        if( !m_filterCtrl->HasFocus() )
        {
            doSetFocus( m_filterCtrl );

            // Because we didn't have focus we missed our chance to have the native widget
            // handle the keystroke.  We'll have to do the first character ourselves.
            doStartingKey( aEvent );
        }
        else
        {
            // On some platforms a wxComboFocusHandler will have been pushed which
            // unhelpfully gives the event right back to the popup.  Make sure the filter
            // control is going to get the event.
            if( m_filterCtrl->GetEventHandler() != m_filterCtrl )
                m_focusHandler = m_filterCtrl->PopEventHandler();

            aEvent.Skip();
        }
        break;
    }
}


void FILTER_COMBOPOPUP::onEnter( wxCommandEvent& aEvent )
{
    Accept();
}


void FILTER_COMBOPOPUP::onFilterEdit( wxCommandEvent& aEvent )
{
    rebuildList();
    updateSize();

    if( m_listBox->GetCount() > 0 )
        m_listBox->SetSelection( 0 );
}


void FILTER_COMBOPOPUP::doStartingKey( wxKeyEvent& aEvent )
{
    if( aEvent.GetKeyCode() == WXK_BACK )
    {
        const long pos = m_filterCtrl->GetLastPosition();
        m_filterCtrl->Remove( pos - 1, pos );
    }
    else
    {
        bool isPrintable;
        int  ch = aEvent.GetUnicodeKey();

        if( ch != WXK_NONE )
            isPrintable = true;
        else
        {
            ch = aEvent.GetKeyCode();
            isPrintable = ch > WXK_SPACE && ch < WXK_START;
        }

        if( isPrintable )
        {
            wxString text( static_cast<wxChar>( ch ) );

            // wxCHAR_HOOK chars have been converted to uppercase.
            if( !aEvent.ShiftDown() )
                text.MakeLower();

            m_filterCtrl->AppendText( text );
        }
    }
}


void FILTER_COMBOPOPUP::doSetFocus( wxWindow* aWindow )
{
#ifndef __WXGTK__ // Cannot focus in non-toplevel windows on GTK
    KIPLATFORM::UI::ForceFocus( aWindow );
#endif
}


FILTER_COMBOBOX::FILTER_COMBOBOX( wxWindow *parent, wxWindowID id, const wxPoint &pos,
                                  const wxSize &size, long style ) :
        wxComboCtrl( parent, id, wxEmptyString, pos, size, style|wxTE_PROCESS_ENTER ),
        m_filterPopup( nullptr )
{
    UseAltPopupWindow();
    Connect( wxEVT_CHAR_HOOK, wxKeyEventHandler( FILTER_COMBOBOX::onKeyDown ), nullptr, this );

#ifdef __WXMSW__
    // On Windows the listbox background doesn't have the right colour in dark mode
    if( KIPLATFORM::UI::IsDarkTheme() )
        SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
    else
#endif
    SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_LISTBOX ) );
}


FILTER_COMBOBOX::FILTER_COMBOBOX( wxWindow* parent, wxWindowID id, const wxString& value,
                                  const wxPoint& pos, const wxSize& size,
                                  int count, wxString strings[], long style ) :
        FILTER_COMBOBOX( parent, id, pos, size, style )
{
    // These arguments are just to match wxFormBuilder's expected API; they are not supported
    wxASSERT( value.IsEmpty() && count == 0 && strings == nullptr );

    m_filterPopup = new FILTER_COMBOPOPUP();
    setFilterPopup( m_filterPopup );
}


FILTER_COMBOBOX::~FILTER_COMBOBOX()
{
    Disconnect( wxEVT_CHAR_HOOK, wxKeyEventHandler( FILTER_COMBOBOX::onKeyDown ), nullptr, this );
}


void FILTER_COMBOBOX::SetStringList( const wxArrayString& aList )
{
    m_filterPopup->SetStringList( aList );
}


void FILTER_COMBOBOX::SetSelectedString( const wxString& aString )
{
    m_filterPopup->SetSelectedString( aString );
}


void FILTER_COMBOBOX::setFilterPopup( FILTER_COMBOPOPUP* aPopup )
{
    m_filterPopup = aPopup;
    SetPopupControl( aPopup );
}


void FILTER_COMBOBOX::onKeyDown( wxKeyEvent& aEvt )
{
    int key = aEvt.GetKeyCode();

    if( IsPopupShown() )
    {
        // If the popup is shown then it's CHAR_HOOK should be eating these before they
        // even get to us.  But just to be safe, we go ahead and skip.
        aEvt.Skip();
    }
    // Shift-return accepts dialog
    else if( ( key == WXK_RETURN || key == WXK_NUMPAD_ENTER ) && aEvt.ShiftDown() )
    {
        wxPostEvent( m_parent, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
    }
    // Return, arrow-down and space-bar all open popup
    else if( key == WXK_RETURN || key == WXK_NUMPAD_ENTER || key == WXK_DOWN
             || key == WXK_NUMPAD_DOWN || key == WXK_SPACE )
    {
        Popup();
    }
    // Non-control characters go to filterbox in popup for read-only controls
    else if( key > WXK_SPACE && key < WXK_START && ( GetWindowStyleFlag() & wxCB_READONLY ) )
    {
        Popup();
        m_filterPopup->OnStartingKey( aEvt );
    }
    else
    {
        aEvt.Skip();
    }
}
