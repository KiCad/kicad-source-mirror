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
#include <wx/evtloop.h>
#include <wx/display.h>

wxDEFINE_EVENT( NET_SELECTED, wxCommandEvent );

#define LIST_ITEM_PADDING 5    // these are probably going to be platform-specific...
#define LIST_PADDING 5

#define NO_NET _( "<no net>" )


class NET_SELECTOR_POPUP : public wxPanel
{
public:
    NET_SELECTOR_POPUP( wxWindow* aParent, const wxPoint& aPos, const wxSize& aSize,
                        NETINFO_LIST* aNetInfoList ) :
            wxPanel( aParent, wxID_ANY, aPos, aSize ),
            m_popupWidth( -1 ),
            m_maxPopupHeight( 1000 ),
            m_netinfoList( aNetInfoList ),
            m_filterCtrl( nullptr ),
            m_netListBox( nullptr ),
            m_cancelled( false ),
            m_selected( false ),
            m_selectedNet( 0 )
    {
        m_popupWidth = aSize.x;
        m_maxPopupHeight = aSize.y;

        wxBoxSizer* mainSizer;
        mainSizer = new wxBoxSizer( wxVERTICAL );

        m_filterCtrl = new wxTextCtrl( this, wxID_ANY );
        m_filterCtrl->SetHint( _( "Filter" ) );
        mainSizer->Add( m_filterCtrl, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 0 );

        m_netListBox = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, 0,
                                      wxLB_SINGLE|wxLB_NEEDED_SB );
        mainSizer->Add( m_netListBox, 0, wxALL|wxEXPAND, 0 );

        SetSizer( mainSizer );
        Layout();

        Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( NET_SELECTOR_POPUP::onCapturedMouseClick ), NULL, this );
        m_netListBox->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( NET_SELECTOR_POPUP::onListBoxMouseClick ), NULL, this );
        m_filterCtrl->Connect( wxEVT_TEXT, wxCommandEventHandler( NET_SELECTOR_POPUP::onFilterEdit ), NULL, this );

        rebuildList();
    }

    void SetSelectedNetcode( int aNetcode ) { m_selectedNet = aNetcode; }
    int GetSelectedNetcode() { return m_selectedNet; }

    bool DoPopup()
    {
        Show( true );

        wxGUIEventLoop eventLoop;

        while ( eventLoop.Pending() )
        {
            wxPoint screenPos = wxGetMousePosition();

            if( m_netListBox->GetScreenRect().Contains( screenPos ) )
            {
                if( HasCapture() )
                    ReleaseMouse();

#ifdef __WXOSX_MAC__
                m_netListBox->OSXForceFocus();
#else
                m_netListBox->SetFocus();
#endif

                wxPoint relativePos = m_netListBox->ScreenToClient( screenPos );
                int     item = m_netListBox->HitTest( relativePos );

                if( item >= 0 )
                    m_netListBox->SetSelection( item );
            }
            else if( m_filterCtrl->GetScreenRect().Contains( screenPos ) )
            {
                if( HasCapture() )
                    ReleaseMouse();

                m_filterCtrl->SetFocus();
            }
            else if( !HasCapture() )
            {
                CaptureMouse();
            }

            eventLoop.Dispatch();

            if( m_cancelled || m_selected )
                break;
        }

        if( HasCapture() )
            ReleaseMouse();

        Show( false );

        return m_selected;
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

    void onFilterEdit( wxCommandEvent& aEvent )
    {
        rebuildList();
    }

    // Accecpt single-click closure from m_netListBox
    void onListBoxMouseClick( wxMouseEvent& aEvent )
    {
        wxPoint relativePos = m_netListBox->ScreenToClient( wxGetMousePosition() );
        int     item = m_netListBox->HitTest( relativePos );

        if( item >= 0 )
        {
            wxString selectedNetName = m_netListBox->GetString( (unsigned) item );

            if( selectedNetName.IsEmpty() )
                m_selectedNet = -1;
            else if( selectedNetName == NO_NET )
                m_selectedNet = 0;
            else
                m_selectedNet = m_netinfoList->GetNetItem( selectedNetName )->GetNet();

            m_selected = true;
        }

        aEvent.Skip();
    }

    // Cancel popup for clicks outside it
    void onCapturedMouseClick( wxMouseEvent& aEvent )
    {
        wxPoint mousePos = aEvent.GetPosition();

        if( !GetRect().Contains( mousePos ) )
            m_cancelled = true;

        aEvent.Skip();
    }

protected:
    int           m_popupWidth;
    int           m_maxPopupHeight;
    NETINFO_LIST* m_netinfoList;

    wxTextCtrl*   m_filterCtrl;
    wxListBox*    m_netListBox;

    bool          m_cancelled;
    bool          m_selected;
    int           m_selectedNet;
};


NET_SELECTOR::NET_SELECTOR( wxWindow *parent, wxWindowID id,
                            const wxPoint &pos, const wxSize &size, long style ) :
        wxComboCtrl( parent, id, wxEmptyString, pos, size, style|wxCB_READONLY ),
        m_netinfoList( nullptr ),
        m_netSelectorPopup( nullptr )
{ }

void NET_SELECTOR::DoSetPopupControl( wxComboPopup* aPopup )
{
    m_popup = nullptr;
}

void NET_SELECTOR::OnButtonClick()
{
    wxRect    comboRect = GetScreenRect();
    wxPoint   popupPos( comboRect.x + 2, comboRect.y + comboRect.height );
    wxWindow* dlg = m_parent;

    while( !dlg->IsTopLevel() && dlg->GetParent() )
        dlg = dlg->GetParent();

    popupPos = dlg->ScreenToClient( popupPos );

    wxSize popupSize( comboRect.width - 4, dlg->GetRect().height - popupPos.y - 24 );
    auto   popup = new NET_SELECTOR_POPUP( dlg, popupPos, popupSize, m_netinfoList );

    popup->SetSelectedNetcode( m_netcode );

    if( popup->DoPopup() )
        SetSelectedNetcode( popup->GetSelectedNetcode() );
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

