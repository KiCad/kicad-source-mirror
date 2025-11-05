/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007, 2008 Lubo Racko <developer@lura.sk>
 * Copyright (C) 2007, 2008, 2012 Alexander Lunev <al.lunev@yahoo.com>
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include "string_utils.h"

#include <pcad/pcad_nets.h>

#include <xnode.h>

#include <wx/string.h>

namespace PCAD2KICAD {

wxString ConvertNetName( const wxString& aName )
{
    wxString retval;
    bool     negate = false;

    for( auto c = aName.begin(); c < aName.end(); c++ )
    {
        if( *c != '~' )
        {
            retval += *c;
        }
        else if( !negate )
        {
            retval += '~';
            retval += '{';
            negate = true;
        }
        else
        {
            retval += '}';
            negate = false;
        }
    }

    retval = EscapeString( retval, CTX_NETNAME );
    return retval;
}

PCAD_NET_NODE::PCAD_NET_NODE()
{
    m_CompRef   = wxEmptyString;
    m_PinRef    = wxEmptyString;
}


PCAD_NET_NODE::~PCAD_NET_NODE()
{
}


PCAD_NET::PCAD_NET( int aNetCode ) : m_NetCode( aNetCode )
{
    m_Name = wxEmptyString;
}


PCAD_NET::~PCAD_NET()
{
    int i;

    for( i = 0; i < (int) m_NetNodes.GetCount(); i++ )
    {
        delete m_NetNodes[i];
    }
}


void PCAD_NET::Parse( XNODE* aNode )
{
    wxString        propValue, s1, s2;
    PCAD_NET_NODE*   netNode;
    XNODE*          lNode;

    aNode->GetAttribute( wxT( "Name" ), &propValue );
    propValue.Trim( false );
    propValue.Trim( true );
    m_Name = ConvertNetName( propValue );

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

        netNode = new PCAD_NET_NODE;
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
