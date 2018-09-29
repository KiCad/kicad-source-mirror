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
    POPUP_EVENTFILTER( wxDialog* aPopup, wxComboCtrl* aCombobox ) :
            m_popup( aPopup ),
            m_combobox( aCombobox ),
            m_firstMouseUp( false )
    {
        wxEvtHandler::AddFilter( this );
    }

    ~POPUP_EVENTFILTER() override
    {
        wxEvtHandler::RemoveFilter( this );
    }

    int FilterEvent( wxEvent& aEvent ) override
    {
        if( aEvent.GetEventType() == wxEVT_LEFT_DOWN )
        {
            // Click outside popup cancels
            if( !m_popup->GetScreenRect().Contains( wxGetMousePosition() ) )
            {
                m_popup->EndModal( wxID_CANCEL );
                return Event_Processed;
            }
        }
        else if( aEvent.GetEventType() == wxEVT_LEFT_UP )
        {
            if( m_firstMouseUp )
            {
                // A first mouse-up inside the popup represents a drag-style menu selection
                if( m_popup->GetScreenRect().Contains( wxGetMousePosition() ) )
                    m_popup->EndModal( wxID_OK );

                // Otherwise the first mouse-up is sent back to the combox button
                else if( m_combobox->GetButton() )
                    m_combobox->GetButton()->GetEventHandler()->ProcessEvent( aEvent );

                m_firstMouseUp = false;
                return Event_Processed;
            }
        }

        return Event_Skip;
    }

private:
    wxDialog*     m_popup;
    wxComboCtrl*  m_combobox;
    bool          m_firstMouseUp;
};


class NET_SELECTOR_POPUP : public wxDialog
{
public:
    NET_SELECTOR_POPUP( wxComboCtrl* aParent, const wxPoint& aPos, const wxSize& aSize,
                        NETINFO_LIST* aNetInfoList ) :
            wxDialog( aParent->GetParent(), wxID_ANY, wxEmptyString, aPos, aSize,
                      wxWANTS_CHARS ),
            m_parentCombobox( aParent ),
            m_minPopupWidth( aSize.x ),
            m_maxPopupHeight( aSize.y ),
            m_netinfoList( aNetInfoList ),
            m_initialized( false ),
            m_selectedNetcode( 0 ),
            m_retCode( 0 )
    {
        SetExtraStyle( wxWS_EX_BLOCK_EVENTS|wxWS_EX_PROCESS_IDLE );

        auto mainSizer = new wxBoxSizer( wxVERTICAL );
        auto panelSizer = new wxBoxSizer( wxVERTICAL );

        auto panel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                  wxSIMPLE_BORDER );
        mainSizer->Add( panel, 1, wxEXPAND, 0 );

        wxStaticText* title = new wxStaticText( panel, wxID_ANY, _( "Filter:" ) );
        panelSizer->Add( title, 0, wxEXPAND, 0 );

        m_filterCtrl = new wxTextCtrl( panel, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                       wxDefaultSize, wxTE_PROCESS_ENTER );
        panelSizer->Add( m_filterCtrl, 0, wxEXPAND, 0 );

        m_listBox = new wxListBox( panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, 0,
                                   wxLB_NEEDED_SB );
        panelSizer->Add( m_listBox, 0, wxEXPAND|wxTOP, 2 );

        panel->SetSizer( panelSizer );
        this->SetSizer( mainSizer );
        Layout();

        Connect( wxEVT_IDLE, wxIdleEventHandler( NET_SELECTOR_POPUP::onIdle ), NULL, this );
        Connect( wxEVT_CHAR_HOOK, wxKeyEventHandler( NET_SELECTOR_POPUP::onKeyDown ), NULL, this );
        m_listBox->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( NET_SELECTOR_POPUP::onListBoxMouseClick ), NULL, this );
        m_filterCtrl->Connect( wxEVT_TEXT, wxCommandEventHandler( NET_SELECTOR_POPUP::onFilterEdit ), NULL, this );
        m_filterCtrl->Connect( wxEVT_TEXT_ENTER, wxCommandEventHandler( NET_SELECTOR_POPUP::onEnter ), NULL, this );

        // <enter> in a ListBox comes in as a double-click on GTK
        m_listBox->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( NET_SELECTOR_POPUP::onEnter ), NULL, this );

        rebuildList();
    }

    ~NET_SELECTOR_POPUP()
    {
        Disconnect( wxEVT_IDLE, wxIdleEventHandler( NET_SELECTOR_POPUP::onIdle ), NULL, this );
        Disconnect( wxEVT_CHAR_HOOK, wxKeyEventHandler( NET_SELECTOR_POPUP::onKeyDown ), NULL, this );
        m_listBox->Disconnect( wxEVT_LEFT_DOWN, wxMouseEventHandler( NET_SELECTOR_POPUP::onListBoxMouseClick ), NULL, this );
        m_filterCtrl->Disconnect( wxEVT_TEXT, wxCommandEventHandler( NET_SELECTOR_POPUP::onFilterEdit ), NULL, this );
        m_filterCtrl->Disconnect( wxEVT_TEXT_ENTER, wxCommandEventHandler( NET_SELECTOR_POPUP::onEnter ), NULL, this );

        m_listBox->Disconnect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( NET_SELECTOR_POPUP::onEnter ), NULL, this );
    }

    void SetSelectedNetcode( int aNetcode )
    {
        m_selectedNetcode = aNetcode;
        m_listBox->SetFocus();
    }

    int GetSelectedNetcode()
    {
        return m_selectedNetcode;
    }

    // While we act like a modal our implementation is not modal.  This is done to allow us
    // to catch mouse and key events outside our window.
    int ShowModal() override
    {
        POPUP_EVENTFILTER filter( this, m_parentCombobox );

        Show( true );
        doSetFocus( m_listBox );

        while( !m_retCode )
            wxYield();

        return m_retCode;
    }

    void EndModal( int aReason ) override
    {
        if( IsShown() )
            Show( false );

        if( !m_retCode )
        {
            if( aReason == wxID_OK )
            {
                wxString selectedNetName;
                int selection = m_listBox->GetSelection();

                if( selection >= 0 )
                    selectedNetName = m_listBox->GetString( (unsigned) selection );

                if( selectedNetName.IsEmpty() )
                    aReason = wxID_CANCEL;
                else if( selectedNetName == NO_NET )
                    m_selectedNetcode = 0;
                else
                    m_selectedNetcode = m_netinfoList->GetNetItem( selectedNetName )->GetNet();
            }

            m_retCode = aReason;
        }
    }

protected:
    void updateSize()
    {
        int    listTop = m_listBox->GetRect().y;
        int    itemHeight = GetTextSize( wxT( "Xy" ), this ).y + LIST_ITEM_PADDING;
        int    listHeight = m_listBox->GetCount() * itemHeight + LIST_PADDING;

        if( listTop + listHeight >= m_maxPopupHeight )
            listHeight = m_maxPopupHeight - listTop - 1;

        int    listWidth = m_minPopupWidth;

        for( size_t i = 0; i < m_listBox->GetCount(); ++i )
        {
            int itemWidth = GetTextSize( m_listBox->GetString( i ), m_listBox ).x;
            listWidth = std::max( listWidth, itemWidth + LIST_PADDING * 3 );
        }

        m_listBox->SetMinSize( wxSize( listWidth, listHeight ) );
        m_listBox->SetSize( wxSize( listWidth, listHeight ) );

        SetSize( wxSize( listWidth, listTop + listHeight ) );
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

        m_listBox->Set( netNames );

        updateSize();
        m_listBox->Refresh();
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
        // never got notified of the click).
        // Note: don't try to do this with KillFocus events; the event ordering is too
        // platform-dependant.
        if( m_initialized )
        {
            bool focusFound = false;

            for( wxWindow* w = wxWindow::FindFocus(); w && !focusFound; w = w->GetParent() )
                focusFound = ( w == this );

            if( !focusFound )
                EndModal( wxID_CANCEL );
        }
#endif
    }

    // Hot-track the mouse (for focus and listbox selection)
    void onMouseMoved( const wxPoint aScreenPos )
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
        else if( !m_initialized )
        {
            doSetFocus( m_listBox );
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
            EndModal( wxID_OK );
            break;

        case WXK_DOWN:
        case WXK_NUMPAD_DOWN:
            doSetFocus( m_listBox );
            m_listBox->SetSelection( std::min( m_listBox->GetSelection() + 1, (int) m_listBox->GetCount() - 1 ) );
            break;

        case WXK_UP:
        case WXK_NUMPAD_UP:
            doSetFocus( m_listBox );
            m_listBox->SetSelection( std::max( m_listBox->GetSelection() - 1, 0 ) );
            break;

        default:
            aEvent.Skip();
            break;
        }
    }

    void onEnter( wxCommandEvent& aEvent )
    {
        EndModal( wxID_OK );
    }

    void onFilterEdit( wxCommandEvent& aEvent )
    {
        rebuildList();
    }

    void onListBoxMouseClick( wxMouseEvent& aEvent )
    {
        m_listBox->SetSelection( m_listBox->HitTest( aEvent.GetPosition() ) );
        EndModal( wxID_OK );
    }

    void doSetFocus( wxWindow* aWindow )
    {
#ifdef __WXOSX_MAC__
        aWindow->OSXForceFocus();
#else
        aWindow->SetFocus();
#endif
    }

protected:
    wxComboCtrl*      m_parentCombobox;
    int               m_minPopupWidth;
    int               m_maxPopupHeight;
    NETINFO_LIST*     m_netinfoList;

    wxTextCtrl*       m_filterCtrl;
    wxListBox*        m_listBox;

    bool              m_initialized;

    int               m_selectedNetcode;
    int               m_retCode;
};


NET_SELECTOR::NET_SELECTOR( wxWindow *parent, wxWindowID id,
                            const wxPoint &pos, const wxSize &size, long style ) :
        wxComboCtrl( parent, id, wxEmptyString, pos, size, style|wxCB_READONLY ),
        m_netinfoList( nullptr ),
        m_netcode( -1 ),
        m_netSelectorPopup( nullptr )
{
    Connect( wxEVT_COMBOBOX_DROPDOWN, wxCommandEventHandler( NET_SELECTOR::onDropdown ), NULL, this );
}


NET_SELECTOR::~NET_SELECTOR()
{
    delete m_netSelectorPopup;
}


void NET_SELECTOR::DoSetPopupControl( wxComboPopup*  )
{
    m_popup = nullptr;
}


void NET_SELECTOR::onDropdown( wxCommandEvent&  )
{
    // Not all keyboard activation methods seem to get to OnButtonClick() by themselves,
    // so we pick them up here.
    OnButtonClick();
}


void NET_SELECTOR::OnButtonClick()
{
    // Guard against clicks during show or hide
    if( m_netSelectorPopup )
        return;

    wxRect    comboRect = GetScreenRect();
    wxPoint   popupPos( comboRect.x + POPUP_PADDING, comboRect.y + comboRect.height );
    wxDisplay display( (unsigned) wxDisplay::GetFromWindow( this ) );

    wxSize popupSize( comboRect.width - ( POPUP_PADDING * 2 ),
                      display.GetClientArea().y + display.GetClientArea().height - popupPos.y );

    m_netSelectorPopup = new NET_SELECTOR_POPUP( this, popupPos, popupSize, m_netinfoList );

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

