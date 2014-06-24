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
 * @file pcb.cpp
 */

#include <wx/wx.h>
#include <wx/config.h>

#include <common.h>

#include <pcb.h>
#include <pcb_arc.h>
#include <pcb_copper_pour.h>
#include <pcb_cutout.h>
#include <pcb_keepout.h>
#include <pcb_line.h>
#include <pcb_module.h>
#include <pcb_pad_shape.h>
#include <pcb_via_shape.h>
#include <pcb_pad.h>
#include <pcb_text.h>
#include <pcb_via.h>
#include <s_expr_loader.h>

namespace PCAD2KICAD {

LAYER_ID PCB::GetKiCadLayer( int aPCadLayer )
{
    wxASSERT( aPCadLayer >= 0 && aPCadLayer < MAX_PCAD_LAYER_QTY );
    return m_layersMap[aPCadLayer].KiCadLayer;
}

LAYER_TYPE_T PCB::GetLayerType( int aPCadLayer )
{
    wxASSERT( aPCadLayer >= 0 && aPCadLayer < MAX_PCAD_LAYER_QTY );
    return m_layersMap[aPCadLayer].layerType;
}

wxString PCB::GetLayerNetNameRef( int aPCadLayer )
{
    wxASSERT( aPCadLayer >= 0 && aPCadLayer < MAX_PCAD_LAYER_QTY );
    return m_layersMap[aPCadLayer].netNameRef;
}

PCB::PCB( BOARD* aBoard ) : PCB_MODULE( this, aBoard )
{
    int i;

    m_defaultMeasurementUnit = wxT( "mil" );

    for( i = 0; i < MAX_PCAD_LAYER_QTY; i++ )
    {
        m_layersMap[i].KiCadLayer = F_Mask; // default
        m_layersMap[i].layerType  = LAYER_TYPE_NONSIGNAL; // default
        m_layersMap[i].netNameRef = wxT( "" ); // default
    }

    m_sizeX = 0;
    m_sizeY = 0;

    m_layersMap[1].KiCadLayer = F_Cu;
    m_layersMap[1].layerType  = LAYER_TYPE_SIGNAL;

    m_layersMap[2].KiCadLayer = B_Cu;
    m_layersMap[2].layerType  = LAYER_TYPE_SIGNAL;

    m_layersMap[3].KiCadLayer  = Eco2_User;
    m_layersMap[6].KiCadLayer  = F_SilkS;
    m_layersMap[7].KiCadLayer  = B_SilkS;
    m_timestamp_cnt = 0x10000000;
}


PCB::~PCB()
{
    int i;

    for( i = 0; i < (int) m_pcbComponents.GetCount(); i++ )
    {
        delete m_pcbComponents[i];
    }

    for( i = 0; i < (int) m_pcbNetlist.GetCount(); i++ )
    {
        delete m_pcbNetlist[i];
    }
}


int PCB::GetNewTimestamp()
{
    return m_timestamp_cnt++;
}

int PCB::GetNetCode( wxString aNetName )
{
    PCB_NET* net;

    for( int i = 0; i < (int) m_pcbNetlist.GetCount(); i++ )
    {
        net = m_pcbNetlist[i];

        if( net->m_name == aNetName )
        {
            return net->m_netCode;
        }
    }

    return 0;
}

XNODE* PCB::FindCompDefName( XNODE* aNode, wxString aName )
{
    XNODE*      result = NULL, * lNode;
    wxString    propValue;

    lNode = FindNode( aNode, wxT( "compDef" ) );

    while( lNode )
    {
        if( lNode->GetName() == wxT( "compDef" ) )
        {
            lNode->GetAttribute( wxT( "Name" ), &propValue );

            if( propValue == aName )
            {
                result  = lNode;
                lNode   = NULL;
            }
        }

        if( lNode )
            lNode = lNode->GetNext();
    }

    return result;
}


void PCB::SetTextProperty( XNODE*   aNode, TTEXTVALUE* aTextValue,
                           wxString aPatGraphRefName, wxString aXmlName,
                           wxString aActualConversion )
{
    XNODE*      tNode, * t1Node;
    wxString    n, pn, propValue, str;

    // aNode is pattern now
    tNode   = aNode;
    t1Node  = aNode;
    n = aXmlName;

    // new file format version
    if( FindNode( tNode, wxT( "patternGraphicsNameRef" ) ) )
    {
        FindNode( tNode,
                  wxT( "patternGraphicsNameRef" ) )->GetAttribute( wxT( "Name" ),
                                                                   &pn );
        pn.Trim( false );
        pn.Trim( true );
        tNode = FindNode( tNode, wxT( "patternGraphicsRef" ) );

        while( tNode )
        {
            if( tNode->GetName() == wxT( "patternGraphicsRef" ) )
            {
                if( FindNode( tNode, wxT( "patternGraphicsNameRef" ) ) )
                {
                    FindNode( tNode,
                              wxT( "patternGraphicsNameRef" ) )->GetAttribute( wxT( "Name" ),
                                                                               &propValue );

                    if( propValue == pn )
                    {
                        t1Node  = tNode; // find correct section with same name.
                        str     = aTextValue->text;
                        str.Trim( false );
                        str.Trim( true );
                        n       = n + wxT( ' ' ) + str; // changed in new file version.....
                        tNode   = NULL;
                    }
                }
            }

            if( tNode )
                tNode = tNode->GetNext();
        }
    }

    // old version and compatibile fr both from this point
    tNode = FindNode( t1Node, wxT( "attr" ) );

    while( tNode )
    {
        tNode->GetAttribute( wxT( "Name" ), &propValue );
        propValue.Trim( false );
        propValue.Trim( true );

        if( propValue == n )
            break;

        tNode = tNode->GetNext();
    }

    if( tNode )
        SetTextParameters( tNode, aTextValue, m_defaultMeasurementUnit, aActualConversion );
}


void PCB::DoPCBComponents( XNODE*           aNode,
                           wxXmlDocument*   aXmlDoc,
                           wxString         aActualConversion,
                           wxStatusBar*     aStatusBar )
{
    XNODE*        lNode, * tNode, * mNode;
    PCB_MODULE*   mc;
    PCB_PAD*      pad;
    PCB_VIA*      via;
    PCB_KEEPOUT*  keepOut;
    wxString      cn, str, propValue;

    lNode = aNode->GetChildren();

    while( lNode )
    {
        mc = NULL;

        if( lNode->GetName() == wxT( "pattern" ) )
        {
            FindNode( lNode, wxT( "patternRef" ) )->GetAttribute( wxT( "Name" ),
                                                                  &cn );
            cn      = ValidateName( cn );
            tNode   = FindNode( (XNODE *)aXmlDoc->GetRoot(), wxT( "library" ) );

            if( tNode && cn.Len() > 0 )
            {
                tNode = FindModulePatternDefName( tNode, cn );

                if( tNode )
                {
                    mc = new PCB_MODULE( this, m_board );

                    mNode = FindNode( lNode, wxT( "patternGraphicsNameRef" ) );
                    if( mNode )
                        mNode->GetAttribute( wxT( "Name" ), &mc->m_patGraphRefName );

                    mc->Parse( tNode, aStatusBar, m_defaultMeasurementUnit, aActualConversion );
                }
            }

            if( mc )
            {
                mc->m_compRef = cn;    // default - in new version of file it is updated later....
                tNode = FindNode( lNode, wxT( "refDesRef" ) );

                if( tNode )
                {
                    tNode->GetAttribute( wxT( "Name" ), &mc->m_name.text );
                    SetTextProperty( lNode, &mc->m_name, mc->m_patGraphRefName, wxT(
                                         "RefDes" ), aActualConversion );
                    SetTextProperty( lNode, &mc->m_value, mc->m_patGraphRefName, wxT(
                                         "Value" ), aActualConversion );
                }

                tNode = FindNode( lNode, wxT( "pt" ) );

                if( tNode )
                    SetPosition( tNode->GetNodeContent(),
                                 m_defaultMeasurementUnit,
                                 &mc->m_positionX,
                                 &mc->m_positionY,
                                 aActualConversion );

                tNode = FindNode( lNode, wxT( "rotation" ) );

                if( tNode )
                {
                    str = tNode->GetNodeContent();
                    str.Trim( false );
                    mc->m_rotation = StrToInt1Units( str );
                }

                str = FindNodeGetContent( lNode, wxT( "isFlipped" ) );

                if( str == wxT( "True" ) )
                    mc->m_mirror = 1;

                tNode = aNode;

                while( tNode->GetName() != wxT( "www.lura.sk" ) )
                    tNode = tNode->GetParent();

                tNode = FindNode( tNode, wxT( "netlist" ) );

                if( tNode )
                {
                    tNode = FindNode( tNode, wxT( "compInst" ) );

                    while( tNode )
                    {
                        tNode->GetAttribute( wxT( "Name" ), &propValue );

                        if( propValue == mc->m_name.text )
                        {
                            if( FindNode( tNode, wxT( "compValue" ) ) )
                            {
                                FindNode( tNode,
                                          wxT( "compValue" ) )->GetAttribute( wxT( "Name" ),
                                                                              &mc->m_value.text );
                                mc->m_value.text.Trim( false );
                                mc->m_value.text.Trim( true );
                            }

                            if( FindNode( tNode, wxT( "compRef" ) ) )
                            {
                                FindNode( tNode,
                                          wxT( "compRef" ) )->GetAttribute( wxT( "Name" ),
                                                                            &mc->m_compRef );
                                mc->m_compRef.Trim( false );
                                mc->m_compRef.Trim( true );
                            }

                            tNode = NULL;
                        }
                        else
                            tNode = tNode->GetNext();
                    }
                }

                // map pins
                tNode   = FindNode( (XNODE *)aXmlDoc->GetRoot(), wxT( "library" ) );
                tNode   = FindCompDefName( tNode, mc->m_compRef );

                if( tNode )
                {
                    tNode = FindPinMap( tNode );

                    if( tNode )
                    {
                        mNode = tNode->GetChildren();

                        while( mNode )
                        {
                            if( mNode->GetName() == wxT( "padNum" ) )
                            {
                                str     = mNode->GetNodeContent();
                                mNode   = mNode->GetNext();

                                if( !mNode )
                                    break;

                                mNode->GetAttribute( wxT( "Name" ), &propValue );
                                mc->SetPadName( str, propValue );
                                mNode = mNode->GetNext();
                            }
                            else
                            {
                                mNode = mNode->GetNext();

                                if( !mNode )
                                    break;

                                mNode = mNode->GetNext();
                            }
                        }
                    }
                }

                m_pcbComponents.Add( mc );
            }
        }
        else if( lNode->GetName() == wxT( "pad" ) )
        {
            pad = new PCB_PAD( this, m_board );
            pad->Parse( lNode, m_defaultMeasurementUnit, aActualConversion );
            m_pcbComponents.Add( pad );
        }
        else if( lNode->GetName() == wxT( "via" ) )
        {
            via = new PCB_VIA( this, m_board );
            via->Parse( lNode, m_defaultMeasurementUnit, aActualConversion );
            m_pcbComponents.Add( via );
        }
        else if( lNode->GetName() == wxT( "polyKeepOut" ) )
        {
            keepOut = new PCB_KEEPOUT( m_callbacks, m_board, 0 );

            if( keepOut->Parse( lNode, m_defaultMeasurementUnit, aActualConversion ) )
                m_pcbComponents.Add( keepOut );
            else
                delete keepOut;
        }

        lNode = lNode->GetNext();
    }
}


void PCB::ConnectPinToNet( wxString aCompRef, wxString aPinRef, wxString aNetName )
{
    PCB_MODULE* module;
    PCB_PAD*    cp;
    int         i, j;

    for( i = 0; i < (int) m_pcbComponents.GetCount(); i++ )
    {
        module = (PCB_MODULE*) m_pcbComponents[i];

        if( module->m_objType == wxT( 'M' ) && module->m_name.text == aCompRef )
        {
            for( j = 0; j < (int) module->m_moduleObjects.GetCount(); j++ )
            {
                if( module->m_moduleObjects[j]->m_objType == wxT( 'P' ) )
                {
                    cp = (PCB_PAD*) module->m_moduleObjects[j];

                    if( cp->m_name.text == aPinRef )
                        cp->m_net = aNetName;
                }
            }
        }
    }
}


int PCB::FindLayer( wxString aLayerName )
{
    for( LAYER_NUM i = 0; i < (int)m_layersStackup.GetCount(); ++i )
    {
        if( m_layersStackup[i] == aLayerName )
            return i;
    }

    return -1;
}


/* KiCad layers
 *  0 Copper layer
 *  1 to 14   Inner layers
 *  15 Component layer
 *  16 Copper side adhesive layer    Technical layers
 *  17 Component side adhesive layer
 *  18 Copper side Solder paste layer
 *  19 Component Solder paste layer
 *  20 Copper side Silk screen layer
 *  21 Component Silk screen layer
 *  22 Copper side Solder mask layer
 *  23 Component Solder mask layer
 *  24 Draw layer (Used for general drawings)
 *  25 Comment layer (Other layer used for general drawings)
 *  26 ECO1 layer (Other layer used for general drawings)       // BUG
 *  26 ECO2 layer (Other layer used for general drawings)       // BUG      27
 *  27 Edge layer. Items on Edge layer are seen on all layers   // BUG     28
 */
void PCB::MapLayer( XNODE* aNode )
{
    wxString    lName, layerType;
    LAYER_ID    KiCadLayer;
    long        num = 0;

    aNode->GetAttribute( wxT( "Name" ), &lName );
    lName = lName.MakeUpper();

    if( lName == wxT( "TOP ASSY" ) )
        KiCadLayer = Cmts_User;
    else if( lName == wxT( "TOP SILK" ) )
        KiCadLayer = F_SilkS;
    else if( lName == wxT( "TOP PASTE" ) )
        KiCadLayer = F_Paste;
    else if( lName == wxT( "TOP MASK" ) )
        KiCadLayer = F_Mask;
    else if( lName == wxT( "TOP" ) )
        KiCadLayer = F_Cu;
    else if( lName == wxT( "BOTTOM" ) )
        KiCadLayer = B_Cu;
    else if( lName == wxT( "BOT MASK" ) )
        KiCadLayer = B_Mask;
    else if( lName == wxT( "BOT PASTE" ) )
        KiCadLayer = B_Paste;
    else if( lName == wxT( "BOT SILK" ) )
        KiCadLayer = B_SilkS;
    else if( lName == wxT( "BOT ASSY" ) )
        KiCadLayer = Dwgs_User;
    else if( lName == wxT( "BOARD" ) )
        KiCadLayer = Edge_Cuts;
    else
    {
        int layernum = FindLayer( lName );

        if( layernum == -1 )
            KiCadLayer = Dwgs_User;    // default
        else
#if 0 // was:
            KiCadLayer = FIRST_COPPER_LAYER + m_layersStackup.GetCount() - 1 - layernum;
#else
            KiCadLayer = LAYER_ID( layernum );
#endif
    }

    if( FindNode( aNode, wxT( "layerNum" ) ) )
        FindNode( aNode, wxT( "layerNum" ) )->GetNodeContent().ToLong( &num );

    if( num < 0 || num >= MAX_PCAD_LAYER_QTY )
        THROW_IO_ERROR( wxString::Format( wxT( "layerNum = %ld is out of range" ), num ) );

    m_layersMap[(int) num].KiCadLayer = KiCadLayer;

    if( FindNode( aNode, wxT( "layerType" ) ) )
    {
        layerType = FindNode( aNode, wxT( "layerType" ) )->GetNodeContent().Trim( false );

        if( layerType == wxT( "NonSignal" ) )
            m_layersMap[(int) num].layerType = LAYER_TYPE_NONSIGNAL;
        if( layerType == wxT( "Signal" ) )
            m_layersMap[(int) num].layerType = LAYER_TYPE_SIGNAL;
        if( layerType == wxT( "Plane" ) )
            m_layersMap[(int) num].layerType = LAYER_TYPE_PLANE;
    }

    if( FindNode( aNode, wxT( "netNameRef" ) ) )
    {
        FindNode( aNode, wxT( "netNameRef" ) )->GetAttribute( wxT( "Name" ),
                                                              &m_layersMap[(int) num].netNameRef );
    }
}

int PCB::FindOutlinePoint( VERTICES_ARRAY* aOutline, wxRealPoint aPoint )
{
    int i;

    for( i = 0; i < (int) aOutline->GetCount(); i++ )
        if( *((*aOutline)[i]) == aPoint )
            return i;

    return -1;
}

/*int cmpFunc( wxRealPoint **first, wxRealPoint **second )
{
    return sqrt( pow( (double) aPointA.x - (double) aPointB.x, 2 ) +
                 pow( (double) aPointA.y - (double) aPointB.y, 2 ) );

    return 0;
}*/
double PCB::GetDistance( wxRealPoint* aPoint1, wxRealPoint* aPoint2 )
{
    return sqrt(  ( aPoint1->x - aPoint2->x ) *
                  ( aPoint1->x - aPoint2->x ) +
                  ( aPoint1->y - aPoint2->y ) *
                  ( aPoint1->y - aPoint2->y ) );
}

void PCB::GetBoardOutline( wxXmlDocument* aXmlDoc, wxString aActualConversion )
{
    XNODE*       iNode, *lNode, *pNode;
    long         PCadLayer = 0;
    int          x, y, i, j, targetInd;
    wxRealPoint* xchgPoint;
    double       minDistance, distance;

    iNode = FindNode( (XNODE *)aXmlDoc->GetRoot(), wxT( "pcbDesign" ) );

    if( iNode )
    {
        // COMPONENTS AND OBJECTS
        iNode = iNode->GetChildren();

        while( iNode )
        {
            // objects
            if( iNode->GetName() == wxT( "layerContents" ) )
            {
                if( FindNode( iNode, wxT( "layerNumRef" ) ) )
                    FindNode( iNode, wxT( "layerNumRef" ) )->GetNodeContent().ToLong( &PCadLayer );

                if( GetKiCadLayer( PCadLayer ) == Edge_Cuts )
                {
                    lNode = iNode->GetChildren();
                    while( lNode )
                    {
                        if( lNode->GetName() == wxT( "line" ) )
                        {
                            pNode = FindNode( lNode, wxT( "pt" ) );

                            if( pNode )
                            {
                                SetPosition( pNode->GetNodeContent(), m_defaultMeasurementUnit,
                                             &x, &y, aActualConversion );

                                if( FindOutlinePoint( &m_boardOutline, wxRealPoint( x, y) ) == -1 )
                                    m_boardOutline.Add( new wxRealPoint( x, y ) );
                            }


                            pNode = pNode->GetNext();

                            if( pNode )
                            {
                                SetPosition( pNode->GetNodeContent(), m_defaultMeasurementUnit,
                                             &x, &y, aActualConversion );

                                if( FindOutlinePoint( &m_boardOutline, wxRealPoint( x, y) ) == -1 )
                                    m_boardOutline.Add( new wxRealPoint( x, y ) );
                            }
                        }

                        lNode = lNode->GetNext();
                    }

                    //m_boardOutline.Sort( cmpFunc );
                    // sort vertices according to the distances between them
                    if( m_boardOutline.GetCount() > 3 )
                    {
                        for( i = 0; i < (int) m_boardOutline.GetCount() - 1; i++ )
                        {
                            minDistance = GetDistance( m_boardOutline[i], m_boardOutline[i + 1] );
                            targetInd = i + 1;

                            for( j = i + 2; j < (int) m_boardOutline.GetCount(); j++ )
                            {
                                distance = GetDistance( m_boardOutline[i], m_boardOutline[j] );
                                if( distance < minDistance )
                                {
                                    minDistance = distance;
                                    targetInd = j;
                                }
                            }

                            xchgPoint = m_boardOutline[i + 1];
                            m_boardOutline[i + 1] = m_boardOutline[targetInd];
                            m_boardOutline[targetInd] = xchgPoint;
                        }
                    }

                    break;
                }
            }

            iNode = iNode->GetNext();
        }
    }
}

void PCB::Parse( wxStatusBar* aStatusBar, wxXmlDocument* aXmlDoc, wxString aActualConversion )
{
    XNODE*          aNode;//, *aaNode;
    PCB_NET*        net;
    PCB_COMPONENT*  comp;
    PCB_MODULE*     module;
    wxString        compRef, pinRef, layerName, layerType;
    int             i, j, netCode;

    // Defaut measurement units
    aNode = FindNode( (XNODE *)aXmlDoc->GetRoot(), wxT( "asciiHeader" ) );

    if( aNode )
    {
        aNode = FindNode( aNode, wxT( "fileUnits" ) );

        if( aNode )
        {
            m_defaultMeasurementUnit = aNode->GetNodeContent();
            m_defaultMeasurementUnit.Trim( true );
            m_defaultMeasurementUnit.Trim( false );
        }
    }

    // Determine layers stackup
    aNode = FindNode( (XNODE *)aXmlDoc->GetRoot(), wxT( "pcbDesign" ) );

    /*if( aNode )
    {
        aNode = FindNode( aNode, wxT( "layersStackup" ) );

        if( aNode )
        {
            aNode = FindNode( aNode, wxT( "layerStackupData" ) );

            while( aNode )
            {
                if( aNode->GetName() == wxT( "layerStackupData" ) )
                {
                    aaNode = FindNode( aNode, wxT( "layerStackupName" ) );

                    if( aaNode ) {
                        aaNode->GetAttribute( wxT( "Name" ), &layerName );
                        layerName = layerName.MakeUpper();
                        m_layersStackup.Add( layerName );
                    }
                }

                aNode = aNode->GetNext();
            }
        }
    }*/

    if( aNode )
    {
        aNode = FindNode( aNode, wxT( "layerDef" ) );

        while( aNode )
        {
            if( aNode->GetName() == wxT( "layerDef" ) )
            {
                if( FindNode( aNode, wxT( "layerType" ) ) )
                {
                    layerType = FindNode( aNode,
                                          wxT( "layerType" ) )->GetNodeContent().Trim( false );

                    if( layerType == wxT( "Signal" ) || layerType == wxT( "Plane" ) )
                    {
                        aNode->GetAttribute( wxT( "Name" ), &layerName );
                        layerName = layerName.MakeUpper();
                        m_layersStackup.Add( layerName );
                    }
                }
            }

            aNode = aNode->GetNext();
        }
    }

    // Layers mapping
    aNode = FindNode( (XNODE *)aXmlDoc->GetRoot(), wxT( "pcbDesign" ) );

    if( aNode )
    {
        aNode = FindNode( aNode, wxT( "layerDef" ) );

        while( aNode )
        {
            if( aNode->GetName() == wxT( "layerDef" ) )
                MapLayer( aNode );

            aNode = aNode->GetNext();
        }
    }

    GetBoardOutline( aXmlDoc, aActualConversion );

    // NETLIST
    // aStatusBar->SetStatusText( wxT( "Loading NETLIST " ) );

    aNode = FindNode( (XNODE *)aXmlDoc->GetRoot(), wxT( "netlist" ) );

    if( aNode )
    {
        aNode = FindNode( aNode, wxT( "net" ) );

        netCode = 1;

        while( aNode )
        {
            net = new PCB_NET( netCode++ );
            net->Parse( aNode );
            m_pcbNetlist.Add( net );

            aNode = aNode->GetNext();
        }
    }

    // BOARD FILE
    // aStatusBar->SetStatusText( wxT( "Loading BOARD DEFINITION " ) );

    aNode = FindNode( (XNODE *)aXmlDoc->GetRoot(), wxT( "pcbDesign" ) );

    if( aNode )
    {
        // COMPONENTS AND OBJECTS
        aNode = aNode->GetChildren();

        while( aNode )
        {
            // Components/modules
            if( aNode->GetName() == wxT( "multiLayer" ) )
                DoPCBComponents( aNode, aXmlDoc, aActualConversion, aStatusBar );

            // objects
            if( aNode->GetName() == wxT( "layerContents" ) )
                DoLayerContentsObjects( aNode, NULL, &m_pcbComponents, aStatusBar,
                                        m_defaultMeasurementUnit, aActualConversion );

            aNode = aNode->GetNext();
        }

        // POSTPROCESS -- SET NETLIST REFERENCES
        // aStatusBar->SetStatusText( wxT( "Processing NETLIST " ) );

        for( i = 0; i < (int) m_pcbNetlist.GetCount(); i++ )
        {
            net = m_pcbNetlist[i];

            for( j = 0; j < (int) net->m_netNodes.GetCount(); j++ )
            {
                compRef = net->m_netNodes[j]->m_compRef;
                compRef.Trim( false );
                compRef.Trim( true );
                pinRef = net->m_netNodes[j]->m_pinRef;
                pinRef.Trim( false );
                pinRef.Trim( true );
                ConnectPinToNet( compRef, pinRef, net->m_name );
            }
        }

        // POSTPROCESS -- FLIP COMPONENTS
        for( i = 0; i < (int) m_pcbComponents.GetCount(); i++ )
        {
            if( m_pcbComponents[i]->m_objType == wxT( 'M' ) )
                ( (PCB_MODULE*) m_pcbComponents[i] )->Flip();
        }

        // POSTPROCESS -- SET/OPTIMIZE NEW PCB POSITION
        // aStatusBar->SetStatusText( wxT( "Optimizing BOARD POSITION " ) );

        m_sizeX = 10000000;
        m_sizeY = 0;

        for( i = 0; i < (int) m_pcbComponents.GetCount(); i++ )
        {
            comp = m_pcbComponents[i];

            if( comp->m_positionY < m_sizeY )
                m_sizeY = comp->m_positionY; // max Y

            if( comp->m_positionX < m_sizeX && comp->m_positionX > 0 )
                m_sizeX = comp->m_positionX; // Min X
        }

        m_sizeY -= 10000;
        m_sizeX -= 10000;
        // aStatusBar->SetStatusText( wxT( " POSITIONING POSTPROCESS " ) );

        for( i = 0; i < (int) m_pcbComponents.GetCount(); i++ )
            m_pcbComponents[i]->SetPosOffset( -m_sizeX, -m_sizeY );

        m_sizeX = 0;
        m_sizeY = 0;

        for( i = 0; i < (int) m_pcbComponents.GetCount(); i++ )
        {
            comp = m_pcbComponents[i];

            if( comp->m_positionY < m_sizeY )
                m_sizeY = comp->m_positionY; // max Y

            if( comp->m_positionX > m_sizeX )
                m_sizeX = comp->m_positionX; // Min X
        }

        // SHEET SIZE CALCULATION
        m_sizeY = -m_sizeY;    // it is in absolute units
        m_sizeX += 10000;
        m_sizeY += 10000;

        // A4 is minimum $Descr A4 11700 8267
        if( m_sizeX < 11700 )
            m_sizeX = 11700;

        if( m_sizeY < 8267 )
            m_sizeY = 8267;
    }
    else
    {
        // LIBRARY FILE
        // aStatusBar->SetStatusText( wxT( "Processing LIBRARY FILE " ) );

        aNode = FindNode( (XNODE *)aXmlDoc->GetRoot(), wxT( "library" ) );

        if( aNode )
        {
            aNode = FindNode( aNode, wxT( "compDef" ) );

            while( aNode )
            {
                // aStatusBar->SetStatusText( wxT( "Processing COMPONENTS " ) );

                if( aNode->GetName() == wxT( "compDef" ) )
                {
                    module = new PCB_MODULE( this, m_board );
                    module->Parse( aNode, aStatusBar, m_defaultMeasurementUnit,
                                   aActualConversion );
                    m_pcbComponents.Add( module );
                }

                aNode = aNode->GetNext();
            }
        }
    }
}


void PCB::AddToBoard()
{
    int i;
    PCB_NET* net;

    m_board->SetCopperLayerCount( m_layersStackup.GetCount() );

    for( i = 0; i < (int) m_pcbNetlist.GetCount(); i++ )
    {
        net = m_pcbNetlist[i];

        m_board->AppendNet( new NETINFO_ITEM( m_board, net->m_name, net->m_netCode ) );
    }

    for( i = 0; i < (int) m_pcbComponents.GetCount(); i++ )
    {
        m_pcbComponents[i]->AddToBoard();
    }
}

} // namespace PCAD2KICAD
