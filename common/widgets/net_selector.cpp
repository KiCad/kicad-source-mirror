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

#include "widgets/net_selector.h"

#include <string_utils.h>

#include <board.h>
#include <netinfo.h>


#define NO_NET _( "<no net>" )
#define CREATE_NET _( "<create net>" )


class NET_SELECTOR_COMBOPOPUP : public FILTER_COMBOPOPUP
{
public:
    NET_SELECTOR_COMBOPOPUP() :
            m_netinfoList( nullptr ),
            m_selectedNetcode( 0 )
    { }

    wxString GetStringValue() const override
    {
        if( !m_netinfoList || ( m_selectedNetcode == -1 ) )
            return m_indeterminateLabel;

        NETINFO_ITEM* netInfo = m_netinfoList->GetNetItem( m_selectedNetcode );

        if( netInfo && netInfo->GetNetCode() > 0 )
            return netInfo->GetNetname();

        return NO_NET;
    }

    void SetNetInfo( const NETINFO_LIST* aNetInfoList )
    {
        m_netinfoList = aNetInfoList;
        rebuildList();
    }

    void SetIndeterminateLabel( const wxString& aIndeterminateLabel )
    {
        m_indeterminateLabel = aIndeterminateLabel;
        rebuildList();
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

    void Accept() override
    {
        wxString  escapedNetName;
        wxString  remainingName;
        wxString  selectedNetName = getSelectedValue().value_or( wxEmptyString );

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
        else if( m_netinfoList && escapedNetName.StartsWith( CREATE_NET, &remainingName ) && !remainingName.IsEmpty() )
        {
            // Remove the first character ':' and all whitespace
            remainingName = remainingName.Mid( 1 ).Trim().Trim( false );

            if( BOARD* board = m_netinfoList->GetParent() )
            {
                NETINFO_ITEM *newnet = new NETINFO_ITEM( board, remainingName, 0 );

                board->Add( newnet );
                rebuildList();

                if( newnet->GetNetCode() > 0 )
                {
                    m_selectedNetcode = newnet->GetNetCode();
                    GetComboCtrl()->SetValue( UnescapeString( remainingName ) );
                }
                else
                {
                    // This indicates that the NETINFO_ITEM was not successfully appended to the
                    // list for unknown reasons
                    board->Remove( newnet );
                    delete newnet;
                }
            }
        }
        else if( m_netinfoList )
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

        wxCommandEvent changeEvent( FILTERED_ITEM_SELECTED );
        wxPostEvent( GetComboCtrl(), changeEvent );
    }

protected:
    void getListContent( wxArrayString& aNetnames ) override
    {
        if( !m_netinfoList )
            return;

        wxString      netstring = getFilterValue();
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
                    aNetnames.push_back( netname );
                    m_unescapedNetNameMap[ netname ] = netinfo->GetNetname();
                }
            }
        }

        std::sort( aNetnames.begin(), aNetnames.end(),
                []( const wxString& lhs, const wxString& rhs )
                {
                    return StrNumCmp( lhs, rhs, true /* ignore case */ ) < 0;
                } );

        // Special handling for <no net>
        if( filter.IsEmpty() || wxString( NO_NET ).MakeLower().Matches( filter ) )
            aNetnames.insert( aNetnames.begin(), NO_NET );

        if( !filter.IsEmpty() && !m_netinfoList->GetNetItem( netstring ) )
        {
            wxString newnet = wxString::Format( "%s: %s", CREATE_NET, netstring );
            aNetnames.insert( aNetnames.end(), newnet );
        }

        if( !m_indeterminateLabel.IsEmpty() )
            aNetnames.push_back( m_indeterminateLabel );
    }

protected:
    const NETINFO_LIST*          m_netinfoList;
    wxString                     m_indeterminateLabel;
    int                          m_selectedNetcode;

    std::map<wxString, wxString> m_unescapedNetNameMap;
};


NET_SELECTOR::NET_SELECTOR( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size,
                            long style ) :
        FILTER_COMBOBOX( parent, id, pos, size, style|wxCB_READONLY )
{
    m_netSelectorPopup = new NET_SELECTOR_COMBOPOPUP();
    setFilterPopup( m_netSelectorPopup );
}


void NET_SELECTOR::SetNetInfo( const NETINFO_LIST* aNetInfoList )
{
    m_netSelectorPopup->SetNetInfo( aNetInfoList );
}


void NET_SELECTOR::SetIndeterminateString( const wxString& aString )
{
    m_indeterminateString = aString;
    m_netSelectorPopup->SetIndeterminateLabel( aString );
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

