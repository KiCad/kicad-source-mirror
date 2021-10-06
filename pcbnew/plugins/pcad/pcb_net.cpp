/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007, 2008 Lubo Racko <developer@lura.sk>
 * Copyright (C) 2007, 2008, 2012 Alexander Lunev <al.lunev@yahoo.com>
 * Copyright (C) 2012-2020 KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <pcad/pcb_net.h>

#include <xnode.h>

#include <wx/string.h>

namespace PCAD2KICAD {

PCB_NET_NODE::PCB_NET_NODE()
{
    m_CompRef   = wxEmptyString;
    m_PinRef    = wxEmptyString;
}


PCB_NET_NODE::~PCB_NET_NODE()
{
}


PCB_NET::PCB_NET( int aNetCode ) : m_NetCode( aNetCode )
{
    m_Name = wxEmptyString;
}


PCB_NET::~PCB_NET()
{
    int i;

    for( i = 0; i < (int) m_NetNodes.GetCount(); i++ )
    {
        delete m_NetNodes[i];
    }
}


void PCB_NET::Parse( XNODE* aNode )
{
    wxString        propValue, s1, s2;
    PCB_NET_NODE*   netNode;
    XNODE*          lNode;

    aNode->GetAttribute( wxT( "Name" ), &propValue );
    propValue.Trim( false );
    propValue.Trim( true );
    m_Name = propValue;

    lNode = FindNode( aNode, wxT( "node" ) );

    while( lNode )
    {
        lNode->GetAttribute( wxT( "Name" ), &s2 );
        s2.Trim( false );
        s1 = wxEmptyString;

        while( s2.Len() > 0 && s2[0] != wxT( ' ' ) )
        {
            s1  = s1 + s2[0];
            s2  = s2.Mid( 1 );
        }

        netNode = new PCB_NET_NODE;
        s1.Trim( false );
        s1.Trim( true );
        netNode->m_CompRef = s1;

        s2.Trim( false );
        s2.Trim( true );
        netNode->m_PinRef = s2;
        m_NetNodes.Add( netNode );
        lNode = lNode->GetNext();
    }
}

} // namespace PCAD2KICAD
