/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007, 2008 Lubo Racko <developer@lura.sk>
 * Copyright (C) 2007, 2008, 2012-2013 Alexander Lunev <al.lunev@yahoo.com>
 * Copyright (C) 2012 KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <pcad/pcb_copper_pour.h>

#include <common.h>
#include <xnode.h>

#include <wx/gdicmn.h>
#include <wx/string.h>

namespace PCAD2KICAD {

PCB_COPPER_POUR::PCB_COPPER_POUR( PCB_CALLBACKS* aCallbacks, BOARD* aBoard, int aPCadLayer ) :
    PCB_POLYGON( aCallbacks, aBoard, aPCadLayer )
{
    m_filled = false;
}


PCB_COPPER_POUR::~PCB_COPPER_POUR()
{
}


bool PCB_COPPER_POUR::Parse( XNODE* aNode, const wxString& aDefaultUnits,
                             const wxString& aActualConversion )
{
    XNODE*          lNode;
    wxString        pourType, str, propValue;
    int             pourSpacing, thermalWidth;

    lNode = FindNode( aNode, wxT( "netNameRef" ) );

    if( lNode )
    {
        lNode->GetAttribute( wxT( "Name" ), &propValue );
        propValue.Trim( false );
        propValue.Trim( true );
        m_net = propValue;
        m_netCode = GetNetCode( m_net );
    }

    if( FindNode( aNode, wxT( "width" ) ) )
    {
        SetWidth( FindNode( aNode, wxT( "width" ) )->GetNodeContent(), aDefaultUnits, &m_width,
                  aActualConversion );
    }

    if( FindNode( aNode, wxT( "pourSpacing" ) ) )
    {
        SetWidth( FindNode( aNode, wxT( "pourSpacing" ) )->GetNodeContent(), aDefaultUnits,
                  &pourSpacing, aActualConversion );
    }

    if( FindNode( aNode, wxT( "thermalWidth" ) ) )
    {
        SetWidth( FindNode( aNode, wxT( "thermalWidth" ) )->GetNodeContent(), aDefaultUnits,
                  &thermalWidth, aActualConversion );
    }

    if( FindNode( aNode, wxT( "island" ) ) )
        m_filled = true;

    lNode = FindNode( aNode, wxT( "pcbPoly" ) );

    if( lNode )
    {
        // retrieve copper pour outline
        FormPolygon( lNode, &m_outline, aDefaultUnits, aActualConversion );

        m_positionX = m_outline[0]->x;
        m_positionY = m_outline[0]->y;
    }
    else
    {
        return false;
    }

    return true;
}

} // namespace PCAD2KICAD
