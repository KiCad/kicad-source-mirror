/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007, 2008 Lubo Racko <developer@lura.sk>
 * Copyright (C) 2007, 2008, 2012 Alexander Lunev <al.lunev@yahoo.com>
 * Copyright (C) 2012-2020 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <pcad/pcb_via.h>
#include <pcad/pcb_via_shape.h>

#include <xnode.h>

#include <wx/string.h>
#include <wx/translation.h>

namespace PCAD2KICAD {

PCB_VIA::PCB_VIA( PCB_CALLBACKS* aCallbacks, BOARD* aBoard ) :
    PCB_PAD( aCallbacks, aBoard )
{
    m_objType = wxT( 'V' );
}


PCB_VIA::~PCB_VIA()
{
}


void PCB_VIA::Parse( XNODE* aNode, const wxString& aDefaultUnits,
                     const wxString& aActualConversion )
{
    XNODE*          lNode, * tNode;
    wxString        propValue;
    PCB_VIA_SHAPE*  viaShape;

    m_rotation = 0;
    lNode = FindNode( aNode, wxT( "viaStyleRef" ) );

    if( lNode )
    {
        lNode->GetAttribute( wxT( "Name" ), &propValue );
        propValue.Trim( false );
        propValue.Trim( true );
        m_name.text = propValue;
    }

    lNode = FindNode( aNode, wxT( "pt" ) );

    if( lNode )
    {
        SetPosition( lNode->GetNodeContent(), aDefaultUnits, &m_positionX, &m_positionY,
                     aActualConversion );
    }

    lNode = FindNode( aNode, wxT( "netNameRef" ) );

    if( lNode )
    {
        lNode->GetAttribute( wxT( "Name" ), &propValue );
        propValue.Trim( false );
        propValue.Trim( true );
        m_net = propValue;
        m_netCode = GetNetCode( m_net );
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

        if( propValue.IsSameAs( m_name.text, false ) )
            break;

        lNode = lNode->GetNext();
    }

    if ( !lNode )
        THROW_IO_ERROR( wxString::Format( _( "Unable to find viaStyleDef %s." ), m_name.text ) );

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
                    viaShape = new PCB_VIA_SHAPE( m_callbacks, m_board );
                    viaShape->Parse( lNode, aDefaultUnits, aActualConversion );
                    m_Shapes.Add( viaShape );
                }
            }

            lNode = lNode->GetNext();
        }
    }
}

} // namespace PCAD2KICAD
