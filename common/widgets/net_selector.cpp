/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <widgets/net_selector.h>

#include <class_board.h>
#include <netinfo.h>
#include <wx/arrstr.h>
#include <wx/display.h>

wxDEFINE_EVENT( NET_SELECTED, wxCommandEvent );

#if defined( __WXOSX_MAC__ )
    #define POPUP_PADDING 2
    #define LIST_ITEM_PADDING 5
    #define LIST_PADDING 5
#elif defined( __WXMSW__ )
    #define POPUP_PADDING 0
    #define LIST_ITEM_PADDING 2
    #define LIST_PADDING 5
#else
    #define POPUP_PADDING 0
    #define LIST_ITEM_PADDING 2
    #define LIST_PADDING 5
#endif

#define NO_NET _( "<no net>" )


class POPUP_EVENTFILTER : public wxEventFilter
{
public:
    POPUP_EVENTFILTER( wxDialog* aPopup ) :
            m_popup( aPopup )
    {
        wxEvtHandler::AddFilter( this );
    }

    ~POPUP_EVENTFILTER() override
    {
        wxEvtHandler::RemoveFilter( this );
    }

    int FilterEvent( wxEvent& aEvent ) override
    {
        // Click outside popup cancels
        if( aEvent.GetEventType() == wxEVT_LEFT_DOWN
            && !m_popup->GetScreenRect().Contains( wxGetMousePosition() ) )
        {
            m_popup->EndModal( wxID_CANCEL );
            return Event_Processed;
        }

        return Event_Skip;
    }

private:
    wxDialog* m_popup;
};


class NET_SELECTOR_POPUP : public wxDialog
{
public:
    NET_SELECTOR_POPUP( wxWindow* aParent, const wxPoint& aPos, const wxSize& aSize,
                        NETINFO_LIST* aNetInfoList ) :
            wxDialog( aParent, wxID_ANY, wxEmptyString, aPos, aSize, wxSIMPLE_BORDER|wxWANTS_CHARS ),
            m_popupWidth( -1 ),
            m_maxPopupHeight( 1000 ),
            m_netinfoList( aNetInfoList ),
            m_filterCtrl( nullptr ),
            m_netListBox( nullptr ),
            m_initialized( false ),
            m_selectedNet( 0 ),
            m_retCode( 0 )
    {
        SetExtraStyle( wxWS_EX_BLOCK_EVENTS|wxWS_EX_PROCESS_IDLE );

        m_popupWidth = aSize.x;
        m_maxPopupHeight = aSize.y;

        wxBoxSizer* mainSizer;
        mainSizer = new wxBoxSizer( wxVERTICAL );

        wxStaticText* title = new wxStaticText( this, wxID_ANY, _( "Filter:" ) );
        mainSizer->Add( title, 0, wxLEFT|wxRIGHT, 2 );

        m_filterCtrl = new wxTextCtrl( this, wxID_ANY );
        mainSizer->Add( m_filterCtrl, 0, wxEXPAND|wxLEFT|wxRIGHT, 2 );

        m_netListBox = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, 0, wxLB_NEEDED_SB );
        mainSizer->Add( m_netListBox, 0, wxALL|wxEXPAND, 2 );

        SetSizer( mainSizer );
        Layout();

        Connect( wxEVT_IDLE, wxIdleEventHandler( NET_SELECTOR_POPUP::onIdle ), NULL, this );
        Connect( wxEVT_CHAR_HOOK, wxKeyEventHandler( NET_SELECTOR_POPUP::onKeyDown ), NULL, this );
        m_netListBox->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( NET_SELECTOR_POPUP::onListBoxMouseClick ), NULL, this );
        m_filterCtrl->Connect( wxEVT_TEXT, wxCommandEventHandler( NET_SELECTOR_POPUP::onFilterEdit ), NULL, this );

        rebuildList();
    }

    ~NET_SELECTOR_POPUP()
    {
        Disconnect( wxEVT_IDLE, wxIdleEventHandler( NET_SELECTOR_POPUP::onIdle ), NULL, this );
        Disconnect( wxEVT_CHAR_HOOK, wxKeyEventHandler( NET_SELECTOR_POPUP::onKeyDown ), NULL, this );
        m_netListBox->Disconnect( wxEVT_LEFT_DOWN, wxMouseEventHandler( NET_SELECTOR_POPUP::onListBoxMouseClick ), NULL, this );
        m_filterCtrl->Disconnect( wxEVT_TEXT, wxCommandEventHandler( NET_SELECTOR_POPUP::onFilterEdit ), NULL, this );
    }

    void SetSelectedNetcode( int aNetcode )
    {
        m_selectedNet = aNetcode;
        m_netListBox->SetFocus();
    }

    int GetSelectedNetcode()
    {
        return m_selectedNet;
    }

    // While we act like a modal our implementation is not modal.  This is done to allow us
    // to catch mouse and key events outside our window.
    int ShowModal() override
    {
        POPUP_EVENTFILTER filter( this );

        Show( true );

        while( !m_retCode )
            wxYield();

        return m_retCode;
    }

    void EndModal( int aReason ) override
    {
        if( IsShown() )
            Show( false );

        if( !m_retCode )
            m_retCode = aReason;
    }

protected:
    void updateSize()
    {
        wxSize popupSize( m_popupWidth, m_maxPopupHeight );
        int    listTop = m_netListBox->GetRect().y;
        int    itemHeight = GetTextSize( wxT( "Xy" ), this ).y + LIST_ITEM_PADDING;
        int    listHeight = m_netListBox->GetCount() * itemHeight + LIST_PADDING;

        if( listTop + listHeight >= m_maxPopupHeight )
            listHeight = m_maxPopupHeight - listTop - 1;

        wxSize listSize( m_popupWidth, listHeight );

        m_netListBox->SetMinSize( listSize );
        m_netListBox->SetSize( listSize );

        popupSize.y = listTop + listHeight;
        SetSize( popupSize );
    }

    void rebuildList()
    {
        wxArrayString netNames;
        wxString      filter = m_filterCtrl->GetValue().MakeLower();

        if( !filter.IsEmpty() )
            filter = wxT( "*" ) + filter + wxT( "*" );

        for( NETINFO_ITEM* netinfo : *m_netinfoList )
        {
            if( netinfo->GetNet() == 0 )
                continue;  // we'll insert NO_NET after sorting

            if( filter.IsEmpty() || wxString( netinfo->GetNetname() ).MakeLower().Matches( filter ) )
                netNames.push_back( netinfo->GetNetname() );
        }
        std::sort( netNames.begin(), netNames.end() );

        if( filter.IsEmpty() || wxString( NO_NET ).MakeLower().Matches( filter ) )
            netNames.insert( netNames.begin(), NO_NET );

        m_netListBox->Set( netNames );

        updateSize();
        m_netListBox->Refresh();
    }

    void onIdle( wxIdleEvent& aEvent )
    {
        // Generate synthetic (but reliable) MouseMoved events
        static wxPoint lastPos;
        wxPoint screenPos = wxGetMousePosition();

        if( screenPos != lastPos )
        {
            lastPos = screenPos;
            onMouseMoved( screenPos );
        }

#ifndef __WXGTK__
        // Check for loss of focus.  This will indicate that a window manager processed
        // an activate event without fully involving wxWidgets (and thus our EventFilter
        // never got notified of the click).  This is possibly an OSX-only issue, but it
        // doesn't seem to cause any harm on MSW.
        // Note: don't try to do this with KillFocus events; the event ordering is too
        // platform-dependant.
        wxWindow* focus = wxWindow::FindFocus();

        if( m_initialized && focus != this && focus != m_netListBox && focus != m_filterCtrl )
            EndModal( wxID_CANCEL );
#endif
    }

    // Hot-track the mouse (for focus and listbox selection)
    void onMouseMoved( const wxPoint aScreenPos )
    {
        if( m_netListBox->GetScreenRect().Contains( aScreenPos ) )
        {
            doSetFocus( m_netListBox );

            wxPoint relativePos = m_netListBox->ScreenToClient( aScreenPos );
            int     item = m_netListBox->HitTest( relativePos );

            if( item >= 0 )
                m_netListBox->SetSelection( item );
        }
        else if( m_filterCtrl->GetScreenRect().Contains( aScreenPos ) )
        {
            doSetFocus( m_filterCtrl );
        }
        else if( !m_initialized )
        {
            doSetFocus( m_netListBox );
            m_initialized = true;
        }
    }

    void onKeyDown( wxKeyEvent& aEvent )
    {
        switch( aEvent.GetKeyCode() )
        {
        case WXK_TAB:
            EndModal( wxID_CANCEL );

            m_parent->NavigateIn( ( aEvent.ShiftDown() ? 0 : wxNavigationKeyEvent::IsForward ) |
                                  ( aEvent.ControlDown() ? wxNavigationKeyEvent::WinChange : 0 ) );
            break;

        case WXK_ESCAPE:
            EndModal( wxID_CANCEL );
            break;

        case WXK_RETURN:
            doSelect( m_netListBox->GetSelection() );
            break;

        case WXK_DOWN:
        case WXK_NUMPAD_DOWN:
            doSetFocus( m_netListBox );
            m_netListBox->SetSelection( std::min( m_netListBox->GetSelection() + 1, (int) m_netListBox->GetCount() - 1 ) );
            break;

        case WXK_UP:
        case WXK_NUMPAD_UP:
            doSetFocus( m_netListBox );
            m_netListBox->SetSelection( std::max( m_netListBox->GetSelection() - 1, 0 ) );
            break;

        default:
            aEvent.Skip();
            break;
        }
    }

    void onFilterEdit( wxCommandEvent& aEvent )
    {
        rebuildList();
    }

    void onListBoxMouseClick( wxMouseEvent& aEvent )
    {
        doSelect( m_netListBox->HitTest( aEvent.GetPosition()));
    }

    void doSetFocus( wxWindow* aWindow )
    {
#ifdef __WXOSX_MAC__
        aWindow->OSXForceFocus();
#else
        aWindow->SetFocus();
#endif
    }

    void doSelect( int aItem )
    {
        if( aItem >= 0 )
        {
            wxString selectedNetName = m_netListBox->GetString( (unsigned) aItem );

            if( selectedNetName.IsEmpty() )
                m_selectedNet = -1;
            else if( selectedNetName == NO_NET )
                m_selectedNet = 0;
            else
                m_selectedNet = m_netinfoList->GetNetItem( selectedNetName )->GetNet();

            EndModal( wxID_OK );
        }
        else
            EndModal( wxID_CANCEL );
    }

protected:
    int               m_popupWidth;
    int               m_maxPopupHeight;
    NETINFO_LIST*     m_netinfoList;

    wxTextCtrl*       m_filterCtrl;
    wxListBox*        m_netListBox;

    bool              m_initialized;

    int               m_selectedNet;
    int               m_retCode;
};


NET_SELECTOR::NET_SELECTOR( wxWindow *parent, wxWindowID id,
                            const wxPoint &pos, const wxSize &size, long style ) :
        wxComboCtrl( parent, id, wxEmptyString, pos, size, style|wxCB_READONLY ),
        m_netinfoList( nullptr ),
        m_netcode( -1 ),
        m_netSelectorPopup( nullptr )
{ }


NET_SELECTOR::~NET_SELECTOR()
{
    delete m_netSelectorPopup;
}


void NET_SELECTOR::DoSetPopupControl( wxComboPopup* aPopup )
{
    m_popup = nullptr;
}


void NET_SELECTOR::OnButtonClick()
{
    // Guard against clicks during show or hide
    if( m_netSelectorPopup )
        return;

    // Allow button to process mouse-up event
    wxYield();

    wxRect    comboRect = GetScreenRect();
    wxPoint   popupPos( comboRect.x + POPUP_PADDING, comboRect.y + comboRect.height );
    wxDisplay display( (unsigned) wxDisplay::GetFromWindow( this ) );

    wxSize popupSize( comboRect.width - ( POPUP_PADDING * 2 ),
                      display.GetClientArea().y + display.GetClientArea().height - popupPos.y );

    m_netSelectorPopup = new NET_SELECTOR_POPUP( m_parent, popupPos, popupSize, m_netinfoList );

    m_netSelectorPopup->SetSelectedNetcode( m_netcode );

    if( m_netSelectorPopup->ShowModal() == wxID_OK )
        SetSelectedNetcode( m_netSelectorPopup->GetSelectedNetcode() );

    SetFocus();

    delete m_netSelectorPopup;
    m_netSelectorPopup = nullptr;
}


void NET_SELECTOR::SetNetInfo( NETINFO_LIST* aNetInfoList )
{
    m_netinfoList = aNetInfoList;
}


void NET_SELECTOR::SetSelectedNetcode( int aNetcode )
{
    m_netcode = aNetcode;

    wxASSERT( m_netinfoList );

    if( m_netcode == -1 )
        SetValue( INDETERMINATE );
    else if( m_netinfoList )
    {
        NETINFO_ITEM* netInfo = m_netinfoList->GetNetItem( m_netcode );

        if( netInfo && netInfo->GetNet() > 0 )
            SetValue( netInfo->GetNetname() );
        else
            SetValue( NO_NET );
    }
}


void NET_SELECTOR::SetIndeterminate()
{
    m_netcode = -1;
    SetValue( INDETERMINATE );
}


bool NET_SELECTOR::IsIndeterminate()
{
    return m_netcode == -1;
}


int NET_SELECTOR::GetSelectedNetcode()
{
    return m_netcode;
}

