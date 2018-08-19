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


wxDEFINE_EVENT( NET_SELECTED, wxCommandEvent );

#define LIST_ITEM_PADDING 5    // these are probably going to be platform-specific...
#define LIST_PADDING 5

#define NO_NET _( "<no net>" )


class NET_SELECTOR_COMBOPOPUP : public wxPanel, public wxComboPopup
{
public:
    NET_SELECTOR_COMBOPOPUP() :
            m_filterCtrl( nullptr ),
            m_netListBox( nullptr ),
            m_popupWidth( -1 ),
            m_maxPopupHeight( 1000 ),
            m_netinfoList( nullptr ),
            m_selectedNet( 0 )
    { }

    bool Create(wxWindow* aParent) override
    {
        wxPanel::Create( aParent );

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

        // wxPopupTransientWindow's mouse capture strategy is an absolute nightmare.  We can't
        // tell where mouse-down events will come from, so we have to accept them from either
        // ourselves (the popup) or our child (the net listbox).  Mouse-move events are even
        // worse as the above strategy doesn't even work -- so we process them on idle.
        Connect( wxEVT_IDLE, wxIdleEventHandler( NET_SELECTOR_COMBOPOPUP::onIdle ), NULL, this );
        Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( NET_SELECTOR_COMBOPOPUP::onMouseClick ), NULL, this );
        m_netListBox->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( NET_SELECTOR_COMBOPOPUP::onMouseClick ), NULL, this );
        m_filterCtrl->Connect( wxEVT_TEXT, wxCommandEventHandler( NET_SELECTOR_COMBOPOPUP::onFilterEdit ), NULL, this );

        return true;
    }

    wxWindow *GetControl() override { return this; }

    void SetStringValue( const wxString& aNetName ) override
    {
        // shouldn't be here (combo is read-only)
    }

    wxString GetStringValue() const override
    {
        NETINFO_ITEM* netInfo = m_netinfoList->GetNetItem( m_selectedNet );

        if( netInfo && netInfo->GetNet() > 0 )
            return netInfo->GetNetname();

        return NO_NET;
    }

    void SetNetInfo( NETINFO_LIST* aNetInfoList )
    {
        m_netinfoList = aNetInfoList;
        rebuildList();
    }

    void SetIndeterminate() { m_selectedNet = -1; }
    bool IsIndeterminate() { return m_selectedNet == -1; }

    void SetSelectedNetcode( int aNetcode ) { m_selectedNet = aNetcode; }
    int GetSelectedNetcode() { return m_selectedNet; }

    wxSize GetAdjustedSize( int aMinWidth, int aPrefHeight, int aMaxHeight ) override
    {
        // Called when the popup is first shown.  Stash the width and maxHeight so we
        // can use them later when refreshing the sizes after filter changes.
        m_popupWidth = aMinWidth;
        m_maxPopupHeight = aMaxHeight;

        return updateSize();
    }

    void OnPopup() override
    {
        // The updateSize() call in GetAdjustedSize() leaves the height off-by-one for
        // some reason, so do it again.
        updateSize();
    }

protected:
    wxSize updateSize()
    {
        wxSize popupSize( m_popupWidth, m_maxPopupHeight );
        int    listTop = m_netListBox->GetRect().y;
        int    itemHeight = GetTextSize( wxT( "Xy" ), this ).y + LIST_ITEM_PADDING;
        int    listHeight = m_netListBox->GetCount() * itemHeight + LIST_PADDING;

        if( listTop + listHeight >= m_maxPopupHeight )
            listHeight = m_maxPopupHeight - listTop - 1;

        popupSize.y = listTop + listHeight;
        SetSize( popupSize );               // us
        GetParent()->SetSize( popupSize );  // the window that wxComboCtrl put us in

        m_netListBox->SetSize( wxSize( m_popupWidth, listHeight ) );
        m_netListBox->Refresh();

        return popupSize;
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
    }

    // Hot-track mouse.
    void onIdle( wxIdleEvent& aEvent )
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
    }

    // Accecpt single-click closure from m_netListBox
    void onMouseClick( wxMouseEvent& aEvent )
    {
        wxPoint relativePos = m_netListBox->ScreenToClient( wxGetMousePosition() );
        int     item = m_netListBox->HitTest( relativePos );

        if( item >= 0 )
        {
            wxString selectedNetName = m_netListBox->GetString( (unsigned) item );

            if( selectedNetName.IsEmpty() )
            {
                m_selectedNet = -1;
                GetComboCtrl()->SetValue( INDETERMINATE );
            }
            else if( selectedNetName == NO_NET )
            {
                m_selectedNet = 0;
                GetComboCtrl()->SetValue( NO_NET );
            }
            else
            {
                m_selectedNet = m_netinfoList->GetNetItem( selectedNetName )->GetNet();
                GetComboCtrl()->SetValue( selectedNetName );
            }

            wxCommandEvent changeEvent( NET_SELECTED );
            wxPostEvent( GetComboCtrl(), changeEvent );

            Dismiss();
        }
        aEvent.Skip();
    }

    void onFilterEdit( wxCommandEvent& aEvent )
    {
        rebuildList();
        updateSize();
    }

protected:
    wxTextCtrl*   m_filterCtrl;
    wxListBox*    m_netListBox;
    int           m_popupWidth;
    int           m_maxPopupHeight;

    NETINFO_LIST* m_netinfoList;

    int           m_selectedNet;
};


NET_SELECTOR::NET_SELECTOR( wxWindow *parent, wxWindowID id,
                            const wxPoint &pos, const wxSize &size, long style ) :
        wxComboCtrl( parent, id, wxEmptyString, pos, size, style|wxCB_READONLY )
{
    m_netSelectorPopup = new NET_SELECTOR_COMBOPOPUP();
    SetPopupControl( m_netSelectorPopup );
}

void NET_SELECTOR::SetNetInfo( NETINFO_LIST* aNetInfoList )
{
    m_netSelectorPopup->SetNetInfo( aNetInfoList );
}

void NET_SELECTOR::SetSelectedNetcode( int aNetcode )
{
    m_netSelectorPopup->SetSelectedNetcode( aNetcode );
    SetValue( m_netSelectorPopup->GetStringValue() );
}

void NET_SELECTOR::SetIndeterminate()
{
    m_netSelectorPopup->SetIndeterminate();
    SetValue( INDETERMINATE );
}

bool NET_SELECTOR::IsIndeterminate()
{
    return m_netSelectorPopup->IsIndeterminate();
}

int NET_SELECTOR::GetSelectedNetcode()
{
    return m_netSelectorPopup->GetSelectedNetcode();
}

