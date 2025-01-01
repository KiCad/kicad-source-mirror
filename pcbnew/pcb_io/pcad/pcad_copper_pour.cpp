/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007, 2008 Lubo Racko <developer@lura.sk>
 * Copyright (C) 2007, 2008, 2012-2013 Alexander Lunev <al.lunev@yahoo.com>
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

#include <pcad/pcad_copper_pour.h>

#include <common.h>
#include <xnode.h>

#include <wx/string.h>

namespace PCAD2KICAD {

PCAD_COPPER_POUR::PCAD_COPPER_POUR( PCAD_CALLBACKS* aCallbacks, BOARD* aBoard, int aPCadLayer ) :
        PCAD_POLYGON( aCallbacks, aBoard, aPCadLayer )
{
    m_filled = false;
}


PCAD_COPPER_POUR::~PCAD_COPPER_POUR()
{
}


bool PCAD_COPPER_POUR::Parse( XNODE* aNode, const wxString& aDefaultUnits,
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
        m_Net = propValue;
        m_NetCode = GetNetCode( m_Net );
    }

    if( FindNode( aNode, wxT( "width" ) ) )
    {
        SetWidth( FindNode( aNode, wxT( "width" ) )->GetNodeContent(), aDefaultUnits, &m_Width,
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

    // If the pour doesn't have the newer `pcbPoly` tag, check for the older `pourOutline` tag
    if( !lNode )
        lNode = FindNode( aNode, wxT( "pourOutline" ) );

    if( lNode )
    {
        // retrieve copper pour outline
        FormPolygon( lNode, &m_Outline, aDefaultUnits, aActualConversion );

        if( m_Outline.GetCount() <= 0 )
        {
            // empty polygon may have been in the file
            return false;
        }

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
