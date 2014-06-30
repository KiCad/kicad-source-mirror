/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007, 2008 Lubo Racko <developer@lura.sk>
 * Copyright (C) 2007, 2008, 2012-2013 Alexander Lunev <al.lunev@yahoo.com>
 * Copyright (C) 2012 KiCad Developers, see CHANGELOG.TXT for contributors.
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

/**
 * @file pcb_pad.cpp
 */

#include <wx/wx.h>
#include <wx/config.h>
#include <trigo.h>

#include <pcb_pad.h>

namespace PCAD2KICAD {

PCB_PAD::PCB_PAD( PCB_CALLBACKS* aCallbacks, BOARD* aBoard ) : PCB_COMPONENT( aCallbacks, aBoard )
{
    m_objType       = wxT( 'P' );
    m_number        = 0;
    m_hole          = 0;
    m_isHolePlated  = true;
    m_defaultPinDes = wxEmptyString;
}


PCB_PAD::~PCB_PAD()
{
    int i;

    for( i = 0; i < (int) m_shapes.GetCount(); i++ )
    {
        delete m_shapes[i];
    }
}


void PCB_PAD::Parse( XNODE*   aNode, wxString aDefaultMeasurementUnit,
                     wxString aActualConversion )
{
    XNODE*          lNode, *cNode;
    long            num;
    wxString        propValue, str, emsg;
    PCB_PAD_SHAPE*  padShape;

    m_rotation = 0;
    lNode = FindNode( aNode, wxT( "padNum" ) );

    if( lNode )
    {
        lNode->GetNodeContent().ToLong( &num );
        m_number = (int) num;
    }

    lNode = FindNode( aNode, wxT( "padStyleRef" ) );

    if( lNode )
    {
        lNode->GetAttribute( wxT( "Name" ), &propValue );
        propValue.Trim( false );
        m_name.text = propValue;
    }

    lNode = FindNode( aNode, wxT( "pt" ) );

    if( lNode )
        SetPosition( lNode->GetNodeContent(), aDefaultMeasurementUnit,
                     &m_positionX, &m_positionY, aActualConversion );

    lNode = FindNode( aNode, wxT( "rotation" ) );

    if( lNode )
    {
        str = lNode->GetNodeContent();
        str.Trim( false );
        m_rotation = StrToInt1Units( str );
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

    lNode = FindNode( aNode, wxT( "defaultPinDes" ) );

    if( lNode )
    {
        lNode->GetAttribute( wxT( "Name" ), &propValue );
        //propValue.Trim( false );
        m_defaultPinDes = propValue;
    }

    lNode = aNode;

    while( lNode && lNode->GetName() != wxT( "www.lura.sk" ) )
        lNode = lNode->GetParent();

    lNode   = FindNode( lNode, wxT( "library" ) );
    if ( !lNode )
        THROW_IO_ERROR( wxT( "Unable to find library section" ) );

    lNode   = FindNode( lNode, wxT( "padStyleDef" ) );

    while( lNode )
    {
        lNode->GetAttribute( wxT( "Name" ), &propValue );

        if( propValue.IsSameAs( m_name.text, false) )
            break;

        lNode = lNode->GetNext();
    }

    if ( !lNode )
        THROW_IO_ERROR( wxString::Format( wxT( "Unable to find padStyleDef " ) + m_name.text ) );

    cNode = FindNode( lNode, wxT( "holeDiam" ) );

    if( cNode )
        SetWidth( cNode->GetNodeContent(), aDefaultMeasurementUnit, &m_hole, aActualConversion );

    if( FindNodeGetContent( lNode, wxT( "isHolePlated" ) ) == wxT( "False" ) )
        m_isHolePlated = false;

    cNode   = FindNode( lNode, wxT( "padShape" ) );

    while( cNode )
    {
        if( cNode->GetName() == wxT( "padShape" ) )
        {
            // we support only Pads on specific layers......
            // we do not support pads on "Plane", "NonSignal" , "Signal" ... layerr
            if( FindNode( cNode, wxT( "layerNumRef" ) ) )
            {
                padShape = new PCB_PAD_SHAPE( m_callbacks, m_board );
                padShape->Parse( cNode, aDefaultMeasurementUnit, aActualConversion );
                m_shapes.Add( padShape );
            }
        }

        cNode = cNode->GetNext();
    }
}


void PCB_PAD::Flip()
{
    int i;

    PCB_COMPONENT::Flip();

    if( m_objType == wxT( 'P' ) )
        m_rotation = -m_rotation;

    for( i = 0; i < (int)m_shapes.GetCount(); i++ )
        m_shapes[i]->m_KiCadLayer = FlipLayer( m_shapes[i]->m_KiCadLayer );
}


void PCB_PAD::AddToModule( MODULE* aModule, int aRotation, bool aEncapsulatedPad )
{
    PCB_PAD_SHAPE*  padShape;
    wxString        padShapeName = wxT( "Ellipse" );
    PAD_ATTR_T      padType;
    int             i;
    int             width = 0;
    int             height = 0;

    D_PAD* pad = new D_PAD( aModule );

    aModule->Pads().PushBack( pad );

    if( !m_isHolePlated && m_hole )
    {
        // mechanical hole
        pad->SetShape( PAD_CIRCLE );
        pad->SetAttribute( PAD_HOLE_NOT_PLATED );

        pad->SetDrillShape( PAD_DRILL_CIRCLE );
        pad->SetDrillSize( wxSize( m_hole, m_hole ) );
        pad->SetSize( wxSize( m_hole, m_hole ) );

        pad->SetLayerSet( LSET::AllCuMask() | LSET( 3, B_Mask, F_Mask ) );
    }
    else
    {
        ( m_hole ) ? padType = PAD_STANDARD : padType = PAD_SMD;

        // form layer mask
        for( i = 0; i < (int) m_shapes.GetCount(); i++ )
        {
            padShape = m_shapes[i];

            if( padShape->m_width > 0 && padShape->m_height > 0 )
            {
                if( padShape->m_KiCadLayer == F_Cu ||
                    padShape->m_KiCadLayer == B_Cu )
                {
                    padShapeName = padShape->m_shape;
                    width = padShape->m_width;
                    height = padShape->m_height;

                    // assume this is SMD pad
                    if( padShape->m_KiCadLayer == F_Cu )
                        pad->SetLayerSet( LSET( 3, F_Cu, F_Paste, F_Mask ) );
                    else
                        pad->SetLayerSet( LSET( 3, B_Cu, B_Paste, B_Mask ) );
                    break;
                }
            }
        }

        if( padType == PAD_STANDARD )
            // actually this is a thru-hole pad
            pad->SetLayerSet( LSET::AllCuMask() | LSET( 2, B_Mask, F_Mask ) );

        if( width == 0 || height == 0 )
            THROW_IO_ERROR( wxT( "pad with zero size" ) );

        pad->SetPadName( m_name.text );

        if( padShapeName == wxT( "Oval" )
            || padShapeName == wxT( "Ellipse" )
            || padShapeName == wxT( "MtHole" ) )
        {
            if( width != height )
                pad->SetShape( PAD_OVAL );
            else
                pad->SetShape( PAD_CIRCLE );
        }
        else if( padShapeName == wxT( "Rect" )
                 || padShapeName == wxT( "RndRect" ) )
            pad->SetShape( PAD_RECT );
        else if( padShapeName == wxT( "Polygon" ) )
            pad->SetShape( PAD_RECT ); // approximation

        pad->SetSize( wxSize( width, height ) );
        pad->SetDelta( wxSize( 0, 0 ) );
        pad->SetOrientation( m_rotation + aRotation );

        pad->SetDrillShape( PAD_DRILL_CIRCLE );
        pad->SetOffset( wxPoint( 0, 0 ) );
        pad->SetDrillSize( wxSize( m_hole, m_hole ) );

        pad->SetAttribute( padType );

        // Set the proper net code
        NETINFO_ITEM* netinfo = m_board->FindNet( m_net );
        if( netinfo == NULL )   // I believe this should not happen, but just in case
        {
            // It is a new net
            netinfo = new NETINFO_ITEM( m_board, m_net );
            m_board->AppendNet( netinfo );
        }

        pad->SetNetCode( netinfo->GetNet() );
    }

    if( !aEncapsulatedPad )
    {
        // pad's "Position" is not relative to the module's,
        // whereas Pos0 is relative to the module's but is the unrotated coordinate.
        wxPoint padpos( m_positionX, m_positionY );
        pad->SetPos0( padpos );
        RotatePoint( &padpos, aModule->GetOrientation() );
        pad->SetPosition( padpos + aModule->GetPosition() );
    }
}


void PCB_PAD::AddToBoard()
{
    PCB_PAD_SHAPE*  padShape;
    int             i;
    int             width = 0;
    int             height = 0;

    if( m_objType == wxT( 'V' ) ) // via
    {
        // choose one of the shapes
        for( i = 0; i < (int) m_shapes.GetCount(); i++ )
        {
            padShape = m_shapes[i];

            if( padShape->m_width > 0 && padShape->m_height > 0 )
            {
                if( padShape->m_KiCadLayer == F_Cu
                    || padShape->m_KiCadLayer == B_Cu )
                {
                    width = padShape->m_width;
                    height = padShape->m_height;

                    break;
                }
            }
        }

        if( width == 0 || height == 0 )
            THROW_IO_ERROR( wxT( "pad or via with zero size" ) );

        if( IsCopperLayer( m_KiCadLayer ) )
        {
            VIA* via = new VIA( m_board );
            m_board->m_Track.Append( via );

            via->SetTimeStamp( 0 );

            via->SetPosition( wxPoint( m_positionX, m_positionY ) );
            via->SetEnd( wxPoint( m_positionX, m_positionY ) );

            via->SetWidth( height );
            via->SetViaType( VIA_THROUGH );
            via->SetLayerPair( F_Cu, B_Cu );
            via->SetDrill( m_hole );

            via->SetLayer( m_KiCadLayer );
            via->SetNetCode( m_netCode );
        }
    }
    else // pad
    {
        MODULE* module = new MODULE( m_board );
        m_board->Add( module, ADD_APPEND );

        m_name.text = m_defaultPinDes;

        module->SetPosition( wxPoint( m_positionX, m_positionY ) );
        AddToModule( module, 0, true );

    }
}

} // namespace PCAD2KICAD
