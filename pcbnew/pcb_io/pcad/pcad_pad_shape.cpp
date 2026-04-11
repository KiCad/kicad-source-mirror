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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <pcad/pcad_pad_shape.h>

#include <common.h>
#include <xnode.h>

#include <wx/string.h>

namespace PCAD2KICAD {

PCAD_PAD_SHAPE::PCAD_PAD_SHAPE( PCAD_CALLBACKS* aCallbacks, BOARD* aBoard ) :
        PCAD_PCB_COMPONENT( aCallbacks, aBoard )
{
    m_Shape     = wxEmptyString;
    m_Width     = 0;
    m_Height    = 0;
}


PCAD_PAD_SHAPE::~PCAD_PAD_SHAPE()
{
}


void PCAD_PAD_SHAPE::Parse( XNODE* aNode, const wxString& aDefaultUnits,
                            const wxString& aActualConversion )
{
    wxString    str, s;
    long        num;
    int         minX, maxX, minY, maxY, x, y;
    XNODE*  lNode;

    lNode = FindNode( aNode, wxT( "padShapeType" ) );

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

    if( m_Shape == wxT( "Oval" )
        || m_Shape == wxT( "Rect" )
        || m_Shape == wxT( "Ellipse" )
        || m_Shape == wxT( "MtHole" )
        || m_Shape == wxT( "RndRect" ) )
    {
        lNode = FindNode( aNode, wxT( "shapeWidth" ) );

        if( lNode )
            SetWidth( lNode->GetNodeContent(), aDefaultUnits, &m_Width, aActualConversion );

        lNode = FindNode( aNode, wxT( "shapeHeight" ) );

        if( lNode )
            SetWidth( lNode->GetNodeContent(), aDefaultUnits, &m_Height, aActualConversion );
    }
    else if( m_Shape == wxT( "Polygon" ) )
    {
        // approximation to simpler pad shape .....
        lNode = FindNode( aNode, wxT( "shapeOutline" ) );

        if( lNode )
            lNode = FindNode( lNode, wxT( "pt" ) );

        minX    = 0;
        maxX    = 0;
        minY    = 0;
        maxY    = 0;

        while( lNode )
        {
            s = lNode->GetNodeContent();
            SetPosition( s, aDefaultUnits, &x, &y, aActualConversion );

            if( minX > x )
                minX = x;

            if( maxX < x )
                maxX = x;

            if( minY > y )
                minY = y;

            if( maxY < y )
                maxY = y;

            lNode = lNode->GetNext();
        }

        m_Width     = maxX - minX;
        m_Height    = maxY - minY;
    }
}


} // namespace PCAD2KICAD
