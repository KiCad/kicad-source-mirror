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

#include <pcad/pcad_via.h>
#include <pcad/pcad_via_shape.h>

#include <richio.h>
#include <xnode.h>

#include <wx/string.h>
#include <wx/translation.h>

namespace PCAD2KICAD {

PCAD_VIA::PCAD_VIA( PCAD_CALLBACKS* aCallbacks, BOARD* aBoard ) : PCAD_PAD( aCallbacks, aBoard )
{
    m_ObjType = wxT( 'V' );
}


PCAD_VIA::~PCAD_VIA()
{
}


void PCAD_VIA::Parse( XNODE* aNode, const wxString& aDefaultUnits,
                     const wxString& aActualConversion )
{
    XNODE*          lNode, * tNode;
    wxString        propValue;
    PCAD_VIA_SHAPE*  viaShape;

    m_Rotation = ANGLE_0;
    lNode = FindNode( aNode, wxT( "viaStyleRef" ) );

    if( lNode )
    {
        lNode->GetAttribute( wxT( "Name" ), &propValue );
        propValue.Trim( false );
        propValue.Trim( true );
        m_Name.text = propValue;
    }

    lNode = FindNode( aNode, wxT( "pt" ) );

    if( lNode )
    {
        SetPosition( lNode->GetNodeContent(), aDefaultUnits, &m_PositionX, &m_PositionY,
                     aActualConversion );
    }

    lNode = FindNode( aNode, wxT( "netNameRef" ) );

    if( lNode )
    {
        lNode->GetAttribute( wxT( "Name" ), &propValue );
        propValue.Trim( false );
        propValue.Trim( true );
        m_Net = propValue;
        m_NetCode = GetNetCode( m_Net );
    }

    lNode = aNode;

    while( lNode && lNode->GetName() != wxT( "www.lura.sk" ) )
        lNode = lNode->GetParent();

    lNode   = FindNode( lNode, wxT( "library" ) );

    if ( !lNode )
        THROW_IO_ERROR( _( "Unable to find library section." ) );

    lNode   = FindNode( lNode, wxT( "viaStyleDef" ) );

    while( lNode )
    {
        lNode->GetAttribute( wxT( "Name" ), &propValue );

        if( propValue.IsSameAs( m_Name.text, false ) )
            break;

        lNode = lNode->GetNext();
    }

    if ( !lNode )
        THROW_IO_ERROR( wxString::Format( _( "Unable to find viaStyleDef %s." ), m_Name.text ) );

    if( lNode )
    {
        tNode   = lNode;
        lNode   = FindNode( tNode, wxT( "holeDiam" ) );

        if( lNode )
            SetWidth( lNode->GetNodeContent(), aDefaultUnits, &m_Hole, aActualConversion );

        lNode = FindNode( tNode, wxT( "viaShape" ) );

        while( lNode )
        {
            if( lNode->GetName() == wxT( "viaShape" ) )
            {
                // we support only Vias on specific layers......
                // we do not support vias on "Plane", "NonSignal" , "Signal" ... layerr
                if( FindNode( lNode, wxT( "layerNumRef" ) ) )
                {
                    viaShape = new PCAD_VIA_SHAPE( m_callbacks, m_board );
                    viaShape->Parse( lNode, aDefaultUnits, aActualConversion );
                    m_Shapes.Add( viaShape );
                }
            }

            lNode = lNode->GetNext();
        }
    }
}

} // namespace PCAD2KICAD
