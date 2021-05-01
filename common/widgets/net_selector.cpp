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

#include <kicad_string.h>
#include <kiplatform/ui.h>

#include <widgets/net_selector.h>

#include <board.h>
#include <netinfo.h>
#include <wx/arrstr.h>
#include <wx/display.h>
#include <wx/valtext.h>
#include <wx/listbox.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>


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
    #define LIST_ITEM_PADDING 6
    #define LIST_PADDING 5
#endif

#define NO_NET _( "<no net>" )
#define CREATE_NET _( "<create net>" )


class NET_SELECTOR_COMBOPOPUP : public wxPanel, public wxComboPopup
{
public:
    NET_SELECTOR_COMBOPOPUP() :
            m_filterValidator( nullptr ),
            m_filterCtrl( nullptr ),
            m_listBox( nullptr ),
            m_minPopupWidth( -1 ),
            m_maxPopupHeight( 1000 ),
            m_netinfoList( nullptr ),
            m_board( nullptr ),
            m_selectedNetcode( 0 ),
            m_focusHandler( nullptr )
    { }

    bool Create(wxWindow* aParent) override
    {
        wxPanel::Create( aParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSIMPLE_BORDER );

        wxBoxSizer* mainSizer;
        mainSizer = new wxBoxSizer( wxVERTICAL );

        wxStaticText* filterLabel = new wxStaticText( this, wxID_ANY, _( "Filter:" ) );
        mainSizer->Add( filterLabel, 0, wxEXPAND, 0 );

        m_filterCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                       wxDefaultSize, wxTE_PROCESS_ENTER );
        m_filterValidator = new wxTextValidator( wxFILTER_EXCLUDE_CHAR_LIST );
        m_filterValidator->SetCharExcludes( " " );
        m_filterCtrl->SetValidator( *m_filterValidator );
        mainSizer->Add( m_filterCtrl, 0, wxEXPAND, 0 );

        m_listBox = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr,
                                   wxLB_SINGLE|wxLB_NEEDED_SB );
        mainSizer->Add( m_listBox, 0, wxEXPAND|wxTOP, 2 );

        SetSizer( mainSizer );
        Layout();

        Connect( wxEVT_IDLE, wxIdleEventHandler( NET_SELECTOR_COMBOPOPUP::onIdle ), nullptr, this );
        Connect( wxEVT_CHAR_HOOK, wxKeyEventHandler( NET_SELECTOR_COMBOPOPUP::onKeyDown ), nullptr, this );
        Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( NET_SELECTOR_COMBOPOPUP::onMouseClick ), nullptr, this );
        m_listBox->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( NET_SELECTOR_COMBOPOPUP::onMouseClick ), nullptr, this );
        m_filterCtrl->Connect( wxEVT_TEXT, wxCommandEventHandler( NET_SELECTOR_COMBOPOPUP::onFilterEdit ), nullptr, this );
        m_filterCtrl->Connect( wxEVT_TEXT_ENTER, wxCommandEventHandler( NET_SELECTOR_COMBOPOPUP::onEnter ), nullptr, this );

        // <enter> in a ListBox comes in as a double-click on GTK
        m_listBox->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( NET_SELECTOR_COMBOPOPUP::onEnter ), nullptr, this );

        return true;
    }

    wxWindow *GetControl() override { return this; }

    void SetStringValue( const wxString& aNetName ) override
    {
        // shouldn't be here (combo is read-only)
    }

    wxString GetStringValue() const override
    {
        if( m_selectedNetcode == -1 )
            return m_indeterminateLabel;

        NETINFO_ITEM* netInfo = m_netinfoList->GetNetItem( m_selectedNetcode );

        if( netInfo && netInfo->GetNetCode() > 0 )
            return netInfo->GetNetname();

        return NO_NET;
    }

    void SetNetInfo( NETINFO_LIST* aNetInfoList )
    {
        m_netinfoList = aNetInfoList;
        rebuildList();
    }

    void SetIndeterminateLabel( const wxString& aIndeterminateLabel )
    {
        m_indeterminateLabel = aIndeterminateLabel;
        rebuildList();
    }

    void SetBoard( BOARD* aBoard )
    {
        m_board = aBoard;
    }

    void SetIndeterminate() { m_selectedNetcode = -1; }
    bool IsIndeterminate() { return m_selectedNetcode == -1; }

    void SetSelectedNetcode( int aNetcode ) { m_selectedNetcode = aNetcode; }
    int GetSelectedNetcode() { return m_selectedNetcode; }

    void SetSelectedNet( const wxString& aNetname )
    {
        if( m_netinfoList && m_netinfoList->GetNetItem( aNetname ) )
            m_selectedNetcode = m_netinfoList->GetNetItem( aNetname )->GetNetCode();
    }

    wxString GetSelectedNetname()
    {
        if( m_netinfoList && m_netinfoList->GetNetItem( m_selectedNetcode ) )
            return m_netinfoList->GetNetItem( m_selectedNetcode )->GetNetname();
        else
            return wxEmptyString;
    }

    wxSize GetAdjustedSize( int aMinWidth, int aPrefHeight, int aMaxHeight ) override
    {
        // Called when the popup is first shown.  Stash the minWidth and maxHeight so we
        // can use them later when refreshing the sizes after filter changes.
        m_minPopupWidth = aMinWidth;
        m_maxPopupHeight = aMaxHeight;

        return updateSize();
    }

    void OnPopup() override
    {
        // While it can sometimes be useful to keep the filter, it's always unexpected.
        // Better to clear it.
        m_filterCtrl->Clear();

        // The updateSize() call in GetAdjustedSize() leaves the height off-by-one for
        // some reason, so do it again.
        updateSize();
    }

    void OnStartingKey( wxKeyEvent& aEvent )
    {
        doSetFocus( m_filterCtrl );
        doStartingKey( aEvent );
    }

    void Accept()
    {
        wxString  selectedNetName;
        wxString  escapedNetName;
        wxString  remainingName;
        int       selection     = m_listBox->GetSelection();
        wxString  prefix        = CREATE_NET;

        if( selection >= 0 )
            selectedNetName = m_listBox->GetString( (unsigned) selection );

        auto it = m_unescapedNetNameMap.find( selectedNetName );

        if( it != m_unescapedNetNameMap.end() )
            escapedNetName = it->second;
        else    // shouldn't happen....
            escapedNetName = selectedNetName;

        Dismiss();

        if( escapedNetName.IsEmpty() || escapedNetName == m_indeterminateLabel )
        {
            m_selectedNetcode = -1;
            GetComboCtrl()->SetValue( m_indeterminateLabel );
        }
        else if( escapedNetName == NO_NET )
        {
            m_selectedNetcode = 0;
            GetComboCtrl()->SetValue( NO_NET );
        }
        else if( escapedNetName.StartsWith( CREATE_NET, &remainingName ) &&
                !remainingName.IsEmpty() )
        {
            // Remove the first character ':' and all whitespace
            remainingName = remainingName.Mid( 1 ).Trim().Trim( false );

            BOARD*        board  = m_netinfoList->GetParent();
            NETINFO_ITEM *newnet = new NETINFO_ITEM( m_board, remainingName, 0 );

            // add the new netinfo through the board's function so that
            // board listeners get notified and things stay in sync.
            if( board != nullptr )
                board->Add( newnet );
            else
                m_netinfoList->AppendNet( newnet );

            rebuildList();

            if( newnet->GetNetCode() > 0 )
            {
                m_selectedNetcode = newnet->GetNetCode();
                GetComboCtrl()->SetValue( UnescapeString( remainingName ) );
            }
            else
            {
                // This indicates that the NETINFO_ITEM was not successfully appended
                // to the list for unknown reasons
                if( board != nullptr )
                    board->Remove( newnet );
                else
                    m_netinfoList->RemoveNet( newnet );

                delete newnet;
            }
        }
        else
        {
            NETINFO_ITEM* netInfo = m_netinfoList->GetNetItem( escapedNetName );

            if( netInfo == nullptr || netInfo->GetNetCode() == 0 )
            {
                m_selectedNetcode = 0;
                GetComboCtrl()->SetValue( NO_NET );
            }
            else
            {
                m_selectedNetcode = netInfo->GetNetCode();
                GetComboCtrl()->SetValue( UnescapeString( escapedNetName ) );
            }
        }

        wxCommandEvent changeEvent( NET_SELECTED );
        wxPostEvent( GetComboCtrl(), changeEvent );
    }

protected:
    wxSize updateSize()
    {
        int listTop    = m_listBox->GetRect().y;
        int itemHeight = KIUI::GetTextSize( wxT( "Xy" ), this ).y + LIST_ITEM_PADDING;
        int listHeight = m_listBox->GetCount() * itemHeight + LIST_PADDING;

        if( listTop + listHeight >= m_maxPopupHeight )
            listHeight = m_maxPopupHeight - listTop - 1;

        int    listWidth = m_minPopupWidth;

        for( size_t i = 0; i < m_listBox->GetCount(); ++i )
        {
            int itemWidth = KIUI::GetTextSize( m_listBox->GetString( i ), m_listBox ).x;
            listWidth = std::max( listWidth, itemWidth + LIST_PADDING * 3 );
        }

        wxSize listSize( listWidth, listHeight );
        wxSize popupSize( listWidth, listTop + listHeight );

        SetSize( popupSize );               // us
        GetParent()->SetSize( popupSize );  // the window that wxComboCtrl put us in

        m_listBox->SetMinSize( listSize );
        m_listBox->SetSize( listSize );

        return popupSize;
    }

    void rebuildList()
    {
        wxArrayString netNames;
        wxString      netstring = m_filterCtrl->GetValue().Trim().Trim( false );
        wxString      filter = netstring.Lower();

        m_unescapedNetNameMap.clear();

        if( !filter.IsEmpty() )
            filter = wxT( "*" ) + filter + wxT( "*" );

        for( NETINFO_ITEM* netinfo : *m_netinfoList )
        {
            if( netinfo->GetNetCode() > 0 && netinfo->IsCurrent() )
            {
                wxString netname = UnescapeString( netinfo->GetNetname() );

                if( filter.IsEmpty() || wxString( netname ).MakeLower().Matches( filter ) )
                {
                    netNames.push_back( netname );
                    m_unescapedNetNameMap[ netname ] = netinfo->GetNetname();
                }
            }
        }

        std::sort( netNames.begin(), netNames.end() );

        // Special handling for <no net>
        if( filter.IsEmpty() || wxString( NO_NET ).MakeLower().Matches( filter ) )
            netNames.insert( netNames.begin(), NO_NET );

        if( !filter.IsEmpty() && !m_netinfoList->GetNetItem( netstring ) )
        {
            wxString newnet = wxString::Format( "%s: %s", CREATE_NET, netstring );
            netNames.insert( netNames.end(), newnet );
        }

        if( !m_indeterminateLabel.IsEmpty() )
            netNames.push_back( m_indeterminateLabel );

        m_listBox->Set( netNames );
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

        if( m_focusHandler )
        {
            m_filterCtrl->PushEventHandler( m_focusHandler );
            m_focusHandler = nullptr;
        }
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
    }

    void onMouseClick( wxMouseEvent& aEvent )
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

    void onKeyDown( wxKeyEvent& aEvent )
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
            Accept();
            break;

        // Arrows go to the list box
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

    void onEnter( wxCommandEvent& aEvent )
    {
        Accept();
    }

    void onFilterEdit( wxCommandEvent& aEvent )
    {
        rebuildList();
        updateSize();

        if( m_listBox->GetCount() > 0 )
            m_listBox->SetSelection( 0 );
    }

    void doStartingKey( wxKeyEvent& aEvent )
    {
        if( aEvent.GetKeyCode() == WXK_BACK )
        {
            const long pos = m_filterCtrl->GetLastPosition();
            m_filterCtrl->Remove( pos - 1, pos );
        }
        else
        {
            bool isPrintable;
            int ch = aEvent.GetUnicodeKey();

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

    void doSetFocus( wxWindow* aWindow )
    {
        KIPLATFORM::UI::ForceFocus( aWindow );
    }

protected:
    wxTextValidator* m_filterValidator;
    wxTextCtrl*      m_filterCtrl;
    wxListBox*       m_listBox;
    int              m_minPopupWidth;
    int              m_maxPopupHeight;

    NETINFO_LIST*    m_netinfoList;
    wxString         m_indeterminateLabel;
    BOARD*           m_board;

    int              m_selectedNetcode;

    std::map<wxString, wxString> m_unescapedNetNameMap;

    wxEvtHandler*    m_focusHandler;
};


NET_SELECTOR::NET_SELECTOR( wxWindow *parent, wxWindowID id, const wxPoint &pos,
                            const wxSize &size, long style ) :
        wxComboCtrl( parent, id, wxEmptyString, pos, size, style|wxCB_READONLY|wxTE_PROCESS_ENTER )
{
    UseAltPopupWindow();

    m_netSelectorPopup = new NET_SELECTOR_COMBOPOPUP();
    SetPopupControl( m_netSelectorPopup );

    Connect( wxEVT_CHAR_HOOK, wxKeyEventHandler( NET_SELECTOR::onKeyDown ), nullptr, this );
}


NET_SELECTOR::~NET_SELECTOR()
{
    Disconnect( wxEVT_CHAR_HOOK, wxKeyEventHandler( NET_SELECTOR::onKeyDown ), nullptr, this );
}


void NET_SELECTOR::onKeyDown( wxKeyEvent& aEvt )
{
    int key = aEvt.GetKeyCode();

    if( IsPopupShown() )
    {
        // If the popup is shown then it's CHAR_HOOK should be eating these before they
        // even get to us.  But just to be safe, we go ahead and skip.
        aEvt.Skip();
    }

    // Shift-return accepts dialog
    else if( key == WXK_RETURN && aEvt.ShiftDown() )
    {
        wxPostEvent( m_parent, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
    }

    // Return, arrow-down and space-bar all open popup
    else if( key == WXK_RETURN || key == WXK_DOWN || key == WXK_NUMPAD_DOWN || key == WXK_SPACE )
    {
        Popup();
    }

    // Non-control characters go to filterbox in popup
    else if( key > WXK_SPACE && key < WXK_START )
    {
        Popup();
        m_netSelectorPopup->OnStartingKey( aEvt );
    }

    else
    {
        aEvt.Skip();
    }
}


void NET_SELECTOR::SetNetInfo( NETINFO_LIST* aNetInfoList )
{
    m_netSelectorPopup->SetNetInfo( aNetInfoList );
}


void NET_SELECTOR::SetIndeterminateString( const wxString& aString )
{
    m_indeterminateString = aString;
    m_netSelectorPopup->SetIndeterminateLabel( aString );
}


void NET_SELECTOR::SetBoard( BOARD* aBoard )
{
    m_netSelectorPopup->SetBoard( aBoard );
}


void NET_SELECTOR::SetSelectedNetcode( int aNetcode )
{
    m_netSelectorPopup->SetSelectedNetcode( aNetcode );
    SetValue( UnescapeString( m_netSelectorPopup->GetStringValue() ) );
}


void NET_SELECTOR::SetSelectedNet( const wxString& aNetname )
{
    m_netSelectorPopup->SetSelectedNet( aNetname );
    SetValue( UnescapeString( m_netSelectorPopup->GetStringValue() ) );
}


wxString NET_SELECTOR::GetSelectedNetname()
{
    return m_netSelectorPopup->GetSelectedNetname();
}


void NET_SELECTOR::SetIndeterminate()
{
    m_netSelectorPopup->SetIndeterminate();
    SetValue( m_indeterminateString  );
}


bool NET_SELECTOR::IsIndeterminate()
{
    return m_netSelectorPopup->IsIndeterminate();
}


int NET_SELECTOR::GetSelectedNetcode()
{
    return m_netSelectorPopup->GetSelectedNetcode();
}

