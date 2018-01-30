/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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


#include <widgets/widget_net_selector.h>

#include <class_board.h>
#include <netinfo.h>
#include <wx/arrstr.h>

WIDGET_NET_SELECTOR::WIDGET_NET_SELECTOR( wxWindow *parent, wxWindowID id,
    const wxString &value, const wxPoint &pos, const wxSize &size,
    int n, const wxString choices[], long style,
    const wxValidator &validator, const wxString &name ) :
    wxComboBox( parent, id, value, pos, size, n, choices, style, validator, name ),
    m_multiple( false )
{
}


WIDGET_NET_SELECTOR::~WIDGET_NET_SELECTOR()
{
}


void WIDGET_NET_SELECTOR::SetMultiple( bool aMultiple )
{
    if ( aMultiple )
    {
        m_multiple = true;

        int k = Append( wxT("<multiple nets>") );
        SetSelection( k );
    }
}


void WIDGET_NET_SELECTOR::SetSelectedNet ( int aNetcode )
{
    for( const auto& net : m_nets )
    {
        if( net.m_Code == aNetcode )
        {
            SetSelection( net.m_Pos );
            return;
        }
    }
}


int WIDGET_NET_SELECTOR::GetSelectedNet()
{
    int pos = GetSelection();

    for( const auto& net : m_nets )
    {
        if( net.m_Pos == pos )
            return net.m_Code;
    }

    return 0;
}


bool WIDGET_NET_SELECTOR::IsUniqueNetSelected() const
{
    if( m_multiple && ( GetSelection() == ( (int)GetCount() - 1 ) ) )
        return false;

    return true;
}


void WIDGET_NET_SELECTOR::SetBoard( BOARD* aBoard )
{
    auto& netinfo = aBoard->GetNetInfo();

    for( unsigned i = 1; i < netinfo.GetNetCount(); i++ )
    {
        NETINFO_ITEM* ni = netinfo.GetNetItem( i );
        NET           net;
        net.m_Name = ni->GetNetname();
        net.m_Code = i;
        m_nets.push_back( net );
    }

    std::sort( m_nets.begin(), m_nets.end() );

    // Add the list of selectable nets to the wxComboBox.
    // Using a wxArrayString is much faster than adding each name separately
    wxArrayString netnames;

    netnames.Add( wxT( "<no net>" ) );    // Always on top of the list

    for( auto& net : m_nets )
    {
        net.m_Pos = netnames.Add( net.m_Name );
    }

    Append( netnames );
}
