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

#include <pcad/pcad_via_shape.h>

#include <common.h>
#include <xnode.h>

#include <wx/string.h>

namespace PCAD2KICAD {

PCAD_VIA_SHAPE::PCAD_VIA_SHAPE( PCAD_CALLBACKS* aCallbacks, BOARD* aBoard ) :
        PCAD_PAD_SHAPE( aCallbacks, aBoard )
{
}


PCAD_VIA_SHAPE::~PCAD_VIA_SHAPE()
{
}


void PCAD_VIA_SHAPE::Parse( XNODE* aNode, const wxString& aDefaultUnits,
                           const wxString& aActualConversion )
{
    XNODE*      lNode;
    wxString    str;
    long        num;

    lNode = FindNode( aNode, wxT( "viaShapeType" ) );

    if( lNode )
    {
        str = lNode->GetNodeContent();
        str.Trim( false );
        m_Shape = str;
    }

    lNode = FindNode( aNode, wxT( "layerNumRef" ) );

    if( lNode )
    {
        lNode->GetNodeContent().ToLong( &num );
        m_PCadLayer = (int) num;
    }

    m_KiCadLayer = GetKiCadLayer();
    lNode = FindNode( aNode, wxT( "shapeWidth" ) );

    if( lNode )
        SetWidth( lNode->GetNodeContent(), aDefaultUnits, &m_Width, aActualConversion );

    lNode = FindNode( aNode, wxT( "shapeHeight" ) );

    if( lNode )
        SetWidth( lNode->GetNodeContent(), aDefaultUnits, &m_Height, aActualConversion );

}

} // namespace PCAD2KICAD
