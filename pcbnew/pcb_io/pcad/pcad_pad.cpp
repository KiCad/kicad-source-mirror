/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007, 2008 Lubo Racko <developer@lura.sk>
 * Copyright (C) 2007, 2008, 2012-2013 Alexander Lunev <al.lunev@yahoo.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pcad/pcad_pad.h>
#include <pcad/pcad_pad_shape.h>

#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <trigo.h>
#include <xnode.h>

#include <wx/string.h>

namespace PCAD2KICAD {


PCAD_PAD::PCAD_PAD( PCAD_CALLBACKS* aCallbacks, BOARD* aBoard ) :
        PCAD_PCB_COMPONENT( aCallbacks, aBoard )
{
    m_ObjType = wxT( 'P' );
    m_Number        = 0;
    m_Hole          = 0;
    m_IsHolePlated  = true;
    m_defaultPinDes = wxEmptyString;
}


PCAD_PAD::~PCAD_PAD()
{
    int i;

    for( i = 0; i < (int) m_Shapes.GetCount(); i++ )
        delete m_Shapes[i];
}


void PCAD_PAD::Parse( XNODE* aNode, const wxString& aDefaultUnits,
                      const wxString& aActualConversion )
{
    XNODE*          lNode;
    XNODE*          cNode;
    long            num;
    wxString        propValue, str, emsg;
    PCAD_PAD_SHAPE* padShape;

    m_Rotation = ANGLE_0;
    lNode = FindNode( aNode, wxT( "padNum" ) );

    if( lNode )
    {
        lNode->GetNodeContent().ToLong( &num );
        m_Number = (int) num;
    }

    lNode = FindNode( aNode, wxT( "padStyleRef" ) );

    if( lNode )
    {
        lNode->GetAttribute( wxT( "Name" ), &propValue );
        propValue.Trim( false );
        m_Name.text = propValue;
    }

    lNode = FindNode( aNode, wxT( "pt" ) );

    if( lNode )
    {
        SetPosition( lNode->GetNodeContent(), aDefaultUnits, &m_PositionX, &m_PositionY,
                     aActualConversion );
    }

    lNode = FindNode( aNode, wxT( "rotation" ) );

    if( lNode )
    {
        str = lNode->GetNodeContent();
        str.Trim( false );
        m_Rotation = EDA_ANGLE( StrToInt1Units( str ), TENTHS_OF_A_DEGREE_T );
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

        if( propValue.IsSameAs( m_Name.text, false) )
            break;

        lNode = lNode->GetNext();
    }

    if ( !lNode )
        THROW_IO_ERROR( wxString::Format( wxT( "Unable to find padStyleDef " ) + m_Name.text ) );

    cNode = FindNode( lNode, wxT( "holeDiam" ) );

    if( cNode )
        SetWidth( cNode->GetNodeContent(), aDefaultUnits, &m_Hole, aActualConversion );

    if( FindNodeGetContent( lNode, wxT( "isHolePlated" ) ).IsSameAs( wxT( "False" ), false ) )
        m_IsHolePlated = false;

    cNode   = FindNode( lNode, wxT( "padShape" ) );

    while( cNode )
    {
        if( cNode->GetName().IsSameAs( wxT( "padShape" ), false ) )
        {
            // we support only Pads on specific layers......
            // we do not support pads on "Plane", "NonSignal" , "Signal" ... layerr
            if( FindNode( cNode, wxT( "layerNumRef" ) ) )
            {
                padShape = new PCAD_PAD_SHAPE( m_callbacks, m_board );
                padShape->Parse( cNode, aDefaultUnits, aActualConversion );
                m_Shapes.Add( padShape );
            }
        }

        cNode = cNode->GetNext();
    }
}


void PCAD_PAD::Flip()
{
    int i;

    PCAD_PCB_COMPONENT::Flip();

    if( m_ObjType == wxT( 'P' ) )
        m_Rotation = -m_Rotation;

    for( i = 0; i < (int)m_Shapes.GetCount(); i++ )
        m_Shapes[i]->m_KiCadLayer = m_board->FlipLayer( m_Shapes[i]->m_KiCadLayer );
}


void PCAD_PAD::AddToFootprint( FOOTPRINT* aFootprint, const EDA_ANGLE& aRotation,
                              bool aEncapsulatedPad )
{
    PCAD_PAD_SHAPE*  padShape;
    wxString        padShapeName = wxT( "Ellipse" );
    PAD_ATTRIB      padType;
    int             i;
    int             width = 0;
    int             height = 0;

    PAD* pad = new PAD( aFootprint );

    if( !m_IsHolePlated && m_Hole )
    {
        // mechanical hole
        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
        pad->SetAttribute( PAD_ATTRIB::NPTH );

        pad->SetDrillShape( PAD_DRILL_SHAPE::CIRCLE );
        pad->SetDrillSize( VECTOR2I( m_Hole, m_Hole ) );
        pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( m_Hole, m_Hole ) );

        // Mounting Hole: Solder Mask Margin from Top Layer Width size.
        // Used the default zone clearance (simplify)
        if( m_Shapes.GetCount() && m_Shapes[0]->m_Shape.IsSameAs( wxT( "MtHole" ), false ) )
        {
            int sm_margin = ( m_Shapes[0]->m_Width - m_Hole ) / 2;
            pad->SetLocalSolderMaskMargin( sm_margin );
            pad->SetLocalClearance( sm_margin + pcbIUScale.mmToIU( 0.254 ) );
        }

        pad->SetLayerSet( LSET::AllCuMask() | LSET( { B_Mask, F_Mask } ) );
    }
    else
    {
        ( m_Hole ) ? padType = PAD_ATTRIB::PTH : padType = PAD_ATTRIB::SMD;

        // form layer mask
        for( i = 0; i < (int) m_Shapes.GetCount(); i++ )
        {
            padShape = m_Shapes[i];

            if( padShape->m_Width > 0 && padShape->m_Height > 0 )
            {
                if( padShape->m_KiCadLayer == F_Cu ||
                    padShape->m_KiCadLayer == B_Cu )
                {
                    padShapeName = padShape->m_Shape;
                    width = padShape->m_Width;
                    height = padShape->m_Height;

                    // assume this is SMD pad
                    if( padShape->m_KiCadLayer == F_Cu )
                        pad->SetLayerSet( LSET( { F_Cu, F_Paste, F_Mask } ) );
                    else
                        pad->SetLayerSet( LSET( { B_Cu, B_Paste, B_Mask } ) );

                    break;
                }
            }
        }

        if( width == 0 || height == 0 )
        {
            delete pad;
            return;
        }

        if( padType == PAD_ATTRIB::PTH )
            // actually this is a thru-hole pad
            pad->SetLayerSet( LSET::AllCuMask() | LSET( { B_Mask, F_Mask } ) );

        pad->SetNumber( m_Name.text );

        if( padShapeName.IsSameAs( wxT( "Oval" ), false )
            || padShapeName.IsSameAs( wxT( "Ellipse" ), false )
            || padShapeName.IsSameAs( wxT( "MtHole" ), false ) )
        {
            if( width != height )
                pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::OVAL );
            else
                pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
        }
        else if( padShapeName.IsSameAs( wxT( "Rect" ), false ) )
        {
            pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );
        }
        else if(  padShapeName.IsSameAs( wxT( "RndRect" ), false ) )
        {
            pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::ROUNDRECT );
        }
        else if( padShapeName.IsSameAs( wxT( "Polygon" ), false ) )
        {
            pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE ); // approximation
        }

        pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( width, height ) );
        pad->SetDelta( PADSTACK::ALL_LAYERS, VECTOR2I( 0, 0 ) );
        pad->SetOrientation( m_Rotation + aRotation );

        pad->SetDrillShape( PAD_DRILL_SHAPE::CIRCLE );
        pad->SetOffset( PADSTACK::ALL_LAYERS, VECTOR2I( 0, 0 ) );
        pad->SetDrillSize( VECTOR2I( m_Hole, m_Hole ) );

        pad->SetAttribute( padType );

        // Set the proper net code
        NETINFO_ITEM* netinfo = m_board->FindNet( m_Net );

        if( netinfo == nullptr )   // I believe this should not happen, but just in case
        {
            // It is a new net
            netinfo = new NETINFO_ITEM( m_board, m_Net );
            m_board->Add( netinfo );
        }

        pad->SetNetCode( netinfo->GetNetCode() );
    }

    if( !aEncapsulatedPad )
    {
        VECTOR2I padpos( m_PositionX, m_PositionY );
        RotatePoint( padpos, aFootprint->GetOrientation() );
        pad->SetPosition( padpos + aFootprint->GetPosition() );
    }

    aFootprint->Add( pad );
}


void PCAD_PAD::AddToBoard( FOOTPRINT* aFootprint )
{
    PCAD_PAD_SHAPE*  padShape;
    int             i;
    int             width = 0;
    int             height = 0;

    if( m_ObjType == wxT( 'V' ) ) // via
    {
        // choose one of the shapes
        for( i = 0; i < (int) m_Shapes.GetCount(); i++ )
        {
            padShape = m_Shapes[i];

            if( padShape->m_Width > 0 && padShape->m_Height > 0 )
            {
                if( padShape->m_KiCadLayer == F_Cu
                    || padShape->m_KiCadLayer == B_Cu )
                {
                    width = padShape->m_Width;
                    height = padShape->m_Height;

                    break;
                }
            }
        }

        if( width == 0 || height == 0 )
            return;

        if( IsCopperLayer( m_KiCadLayer ) )
        {
            PCB_VIA* via = new PCB_VIA( m_board );
            m_board->Add( via );

            via->SetPosition( VECTOR2I( m_PositionX, m_PositionY ) );
            via->SetEnd( VECTOR2I( m_PositionX, m_PositionY ) );

            via->SetWidth( PADSTACK::ALL_LAYERS, height );
            via->SetViaType( VIATYPE::THROUGH );
            via->SetLayerPair( F_Cu, B_Cu );
            via->SetDrill( m_Hole );

            via->SetLayer( m_KiCadLayer );
            via->SetNetCode( m_NetCode );
        }
    }
    else // pad
    {
        if( !aFootprint )
        {
            aFootprint = new FOOTPRINT( m_board );
            m_board->Add( aFootprint, ADD_MODE::APPEND );
            aFootprint->SetPosition( VECTOR2I( m_PositionX, m_PositionY ) );
        }

        m_Name.text = m_defaultPinDes;

        AddToFootprint( aFootprint, ANGLE_0, true );
    }
}

} // namespace PCAD2KICAD
