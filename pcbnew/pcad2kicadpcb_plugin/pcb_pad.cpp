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
        m_shapes[i]->m_KiCadLayer = FlipLayers( m_shapes[i]->m_KiCadLayer );
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
    aModule->m_Pads.PushBack( pad );

    if( !m_isHolePlated && m_hole )
    {
        // mechanical hole
        pad->SetShape( PAD_CIRCLE );
        pad->SetAttribute( PAD_HOLE_NOT_PLATED );

        pad->SetDrillShape( PAD_CIRCLE );
        pad->SetDrillSize( wxSize( m_hole, m_hole ) );
        pad->SetSize( wxSize( m_hole, m_hole ) );

        pad->SetLayerMask( ALL_CU_LAYERS  | SOLDERMASK_LAYER_BACK | SOLDERMASK_LAYER_FRONT );
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
                if( padShape->m_KiCadLayer == LAYER_N_FRONT
                    || padShape->m_KiCadLayer == LAYER_N_BACK )
                {
                    padShapeName = padShape->m_shape;
                    width = padShape->m_width;
                    height = padShape->m_height;

                    // assume this is SMD pad
                    if( padShape->m_KiCadLayer == LAYER_N_FRONT )
                        pad->SetLayerMask( LAYER_FRONT | SOLDERPASTE_LAYER_FRONT |
                                           SOLDERMASK_LAYER_FRONT );
                    else
                        pad->SetLayerMask( LAYER_BACK | SOLDERPASTE_LAYER_BACK |
                                           SOLDERMASK_LAYER_BACK );

                    break;
                }
            }
        }

        if( padType == PAD_STANDARD )
            // actually this is a thru-hole pad
            pad->SetLayerMask( ALL_CU_LAYERS  | SOLDERMASK_LAYER_BACK | SOLDERMASK_LAYER_FRONT );

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

        pad->SetDrillShape( PAD_CIRCLE );
        pad->SetOffset( wxPoint( 0, 0 ) );
        pad->SetDrillSize( wxSize( m_hole, m_hole ) );

        pad->SetAttribute( padType );

        pad->SetNet( 0 );
        pad->SetNetname( m_net );
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
                if( padShape->m_KiCadLayer == LAYER_N_FRONT
                    || padShape->m_KiCadLayer == LAYER_N_BACK )
                {
                    width = padShape->m_width;
                    height = padShape->m_height;

                    break;
                }
            }
        }

        if( width == 0 || height == 0 )
            THROW_IO_ERROR( wxT( "pad or via with zero size" ) );

        if( IsValidCopperLayerIndex( m_KiCadLayer ) )
        {
            SEGVIA* via = new SEGVIA( m_board );
            m_board->m_Track.Append( via );

            via->SetTimeStamp( 0 );

            via->SetPosition( wxPoint( m_positionX, m_positionY ) );
            via->SetEnd( wxPoint( m_positionX, m_positionY ) );

            via->SetWidth( height );
            via->SetShape( VIA_THROUGH );
            ( (SEGVIA*) via )->SetLayerPair( LAYER_N_FRONT, LAYER_N_BACK );
            via->SetDrill( m_hole );

            via->SetLayer( m_KiCadLayer );
            via->SetNet( m_netCode );
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
