/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <pcad/pcad_plane.h>

#include <common.h>
#include <xnode.h>

#include <wx/string.h>

namespace PCAD2KICAD {

PCAD_PLANE::PCAD_PLANE( PCAD_CALLBACKS* aCallbacks, BOARD* aBoard, int aPCadLayer ) :
        PCAD_POLYGON( aCallbacks, aBoard, aPCadLayer )
{
    m_Priority = 1;
}


PCAD_PLANE::~PCAD_PLANE()
{
}


bool PCAD_PLANE::Parse( XNODE* aNode, const wxString& aDefaultUnits,
                        const wxString& aActualConversion )
{
    XNODE*          lNode;
    wxString        pourType, str, propValue;

    lNode = FindNode( aNode, wxT( "netNameRef" ) );

    if( lNode )
    {
        lNode->GetAttribute( wxT( "Name" ), &propValue );
        propValue.Trim( false );
        propValue.Trim( true );
        m_Net = propValue;
        m_NetCode = GetNetCode( m_Net );
    }

    if( FindNode( aNode, wxT( "width" ) ) )
    {
        SetWidth( FindNode( aNode, wxT( "width" ) )->GetNodeContent(), aDefaultUnits, &m_Width,
                  aActualConversion );
    }

    lNode = FindNode( aNode, wxT( "pcbPoly" ) );

    if( !lNode )
        lNode = FindNode( aNode, wxT( "planeOutline" ) );

    if( lNode )
    {
        // retrieve plane outline
        FormPolygon( lNode, &m_Outline, aDefaultUnits, aActualConversion );

        m_PositionX = m_Outline[0]->x;
        m_PositionY = m_Outline[0]->y;
    }
    else
    {
        return false;
    }

    return true;
}

} // namespace PCAD2KICAD
