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

#include <pcad/pcad_pcb.h>

#include <pcad/pcad_keepout.h>
#include <pcad/pcad_footprint.h>
#include <pcad/pcad_nets.h>
#include <pcad/pcad_pad.h>
#include <pcad/pcad_via.h>

#include <board.h>
#include <common.h>
#include <xnode.h>

#include <wx/string.h>

namespace PCAD2KICAD {


PCB_LAYER_ID PCAD_PCB::GetKiCadLayer( int aPCadLayer ) const
{
    auto it = m_LayersMap.find( aPCadLayer );

    if( it == m_LayersMap.end() )
        THROW_IO_ERROR( wxString::Format( _( "Unknown PCad layer %u" ), unsigned( aPCadLayer ) ) );

    return it->second.KiCadLayer;
}


LAYER_TYPE_T PCAD_PCB::GetLayerType( int aPCadLayer ) const
{
    auto it = m_LayersMap.find( aPCadLayer );

    if( it == m_LayersMap.end() )
        THROW_IO_ERROR( wxString::Format( _( "Unknown PCad layer %u" ), unsigned( aPCadLayer ) ) );

    return it->second.layerType;
}


wxString PCAD_PCB::GetLayerNetNameRef( int aPCadLayer ) const
{
    auto it = m_LayersMap.find( aPCadLayer );

    if( it == m_LayersMap.end() )
        THROW_IO_ERROR( wxString::Format( _( "Unknown PCad layer %u" ), unsigned( aPCadLayer ) ) );

    return it->second.netNameRef;
}


PCAD_PCB::PCAD_PCB( BOARD* aBoard ) : PCAD_FOOTPRINT( this, aBoard )
{
    m_DefaultMeasurementUnit = wxT( "mil" );

    // Fill layer map with PCAD defaults
    for( size_t i = 1; i < 12; ++i )
    {
        TLAYER layer;
        layer.KiCadLayer = User_1;               // default
        layer.layerType  = LAYER_TYPE_NONSIGNAL; // default
        layer.netNameRef = wxT( "" ); // default
        layer.hasContent = false;

        m_LayersMap.insert( std::make_pair( i, layer ) );
    }

    m_SizeX = 0;
    m_SizeY = 0;

    m_LayersMap[1].KiCadLayer = F_Cu;
    m_LayersMap[1].layerType = LAYER_TYPE_SIGNAL;

    m_LayersMap[2].KiCadLayer = B_Cu;
    m_LayersMap[2].layerType = LAYER_TYPE_SIGNAL;

    m_LayersMap[3].KiCadLayer = Edge_Cuts;

    m_LayersMap[4].KiCadLayer = F_Mask;

    m_LayersMap[5].KiCadLayer = B_Mask;

    m_LayersMap[6].KiCadLayer  = F_SilkS;

    m_LayersMap[7].KiCadLayer  = B_SilkS;

    m_LayersMap[8].KiCadLayer = F_Paste;

    m_LayersMap[9].KiCadLayer = B_Paste;

    m_LayersMap[10].KiCadLayer = F_Fab;

    m_LayersMap[11].KiCadLayer = B_Fab;
}


PCAD_PCB::~PCAD_PCB()
{
    int i;

    for( i = 0; i < (int) m_PcbComponents.GetCount(); i++ )
    {
        delete m_PcbComponents[i];
    }

    for( i = 0; i < (int) m_PcbNetlist.GetCount(); i++ )
    {
        delete m_PcbNetlist[i];
    }
}


int PCAD_PCB::GetNetCode( const wxString& aNetName ) const
{
    const PCAD_NET* net;

    for( int i = 0; i < (int) m_PcbNetlist.GetCount(); i++ )
    {
        net = m_PcbNetlist[i];

        if( net->m_Name == aNetName )
        {
            return net->m_NetCode;
        }
    }

    return 0;
}

XNODE* PCAD_PCB::FindCompDefName( XNODE* aNode, const wxString& aName ) const
{
    XNODE*      result = nullptr, * lNode;
    wxString    propValue;

    lNode = FindNode( aNode, wxT( "compDef" ) );

    while( lNode )
    {
        if( lNode->GetName().IsSameAs( wxT( "compDef" ), false ) )
        {
            lNode->GetAttribute( wxT( "Name" ), &propValue );

            if( propValue == aName )
            {
                result  = lNode;
                lNode   = nullptr;
            }
        }

        if( lNode )
            lNode = lNode->GetNext();
    }

    return result;
}


void PCAD_PCB::SetTextProperty( XNODE* aNode, TTEXTVALUE* aTextValue, const wxString& aPatGraphRefName,
                           const wxString& aXmlName, const wxString& aActualConversion )
{
    XNODE*      tNode, * t1Node;
    wxString    n, nnew, pn, propValue, str;

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
            if( tNode->GetName().IsSameAs( wxT( "patternGraphicsRef" ), false ) )
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
                        nnew    = n; // new file version
                        n       = n + wxT( ' ' ) + str; // old file version
                        tNode   = nullptr;
                    }
                }
            }

            if( tNode )
                tNode = tNode->GetNext();
        }
    }

    // old version and compatible for both from this point
    tNode = FindNode( t1Node, wxT( "attr" ) );

    while( tNode )
    {
        tNode->GetAttribute( wxT( "Name" ), &propValue );
        propValue.Trim( false );
        propValue.Trim( true );

        if( propValue == n || propValue == nnew )
            break;

        tNode = tNode->GetNext();
    }

    if( tNode )
        SetTextParameters( tNode, aTextValue, m_DefaultMeasurementUnit, aActualConversion );
}


void PCAD_PCB::DoPCBComponents( XNODE* aNode, wxXmlDocument* aXmlDoc, const wxString& aActualConversion,
                           wxStatusBar* aStatusBar )
{
    XNODE*         lNode, * tNode, * mNode;
    PCAD_FOOTPRINT* fp;
    PCAD_PAD*       pad;
    PCAD_VIA*       via;
    PCAD_KEEPOUT*   keepOut;
    wxString       cn, str, propValue;

    lNode = aNode->GetChildren();

    while( lNode )
    {
        fp = nullptr;

        if( lNode->GetName().IsSameAs( wxT( "pattern" ), false ) )
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
                    fp = new PCAD_FOOTPRINT( this, m_board );

                    mNode = FindNode( lNode, wxT( "patternGraphicsNameRef" ) );

                    if( mNode )
                        mNode->GetAttribute( wxT( "Name" ), &fp->m_PatGraphRefName );

                    fp->Parse( tNode, aStatusBar, m_DefaultMeasurementUnit, aActualConversion );
                }
            }

            if( fp )
            {
                fp->m_CompRef = cn;    // default - in new version of file it is updated later....
                tNode = FindNode( lNode, wxT( "refDesRef" ) );

                if( tNode )
                {
                    tNode->GetAttribute( wxT( "Name" ), &fp->m_Name.text );
                    SetTextProperty( lNode, &fp->m_Name, fp->m_PatGraphRefName, wxT( "RefDes" ),
                                     aActualConversion );
                    SetTextProperty( lNode, &fp->m_Value, fp->m_PatGraphRefName, wxT( "Value" ),
                                     aActualConversion );
                }

                tNode = FindNode( lNode, wxT( "pt" ) );

                if( tNode )
                {
                    SetPosition( tNode->GetNodeContent(), m_DefaultMeasurementUnit,
                                 &fp->m_PositionX, &fp->m_PositionY, aActualConversion );
                }

                tNode = FindNode( lNode, wxT( "rotation" ) );

                if( tNode )
                {
                    str = tNode->GetNodeContent();
                    str.Trim( false );
                    fp->m_Rotation = EDA_ANGLE( StrToInt1Units( str ), TENTHS_OF_A_DEGREE_T );
                }

                str = FindNodeGetContent( lNode, wxT( "isFlipped" ) );

                if( str.IsSameAs( wxT( "True" ), false ) )
                    fp->m_Mirror = 1;

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

                        if( propValue == fp->m_Name.text )
                        {
                            if( FindNode( tNode, wxT( "compValue" ) ) )
                            {
                                FindNode( tNode,
                                          wxT( "compValue" ) )->GetAttribute( wxT( "Name" ),
                                                                              &fp->m_Value.text );
                                fp->m_Value.text.Trim( false );
                                fp->m_Value.text.Trim( true );
                            }

                            if( FindNode( tNode, wxT( "compRef" ) ) )
                            {
                                FindNode( tNode,
                                          wxT( "compRef" ) )->GetAttribute( wxT( "Name" ),
                                                                            &fp->m_CompRef );
                                fp->m_CompRef.Trim( false );
                                fp->m_CompRef.Trim( true );
                            }

                            tNode = nullptr;
                        }
                        else
                        {
                            tNode = tNode->GetNext();
                        }
                    }
                }

                // map pins
                tNode   = FindNode( (XNODE *)aXmlDoc->GetRoot(), wxT( "library" ) );
                tNode   = FindCompDefName( tNode, fp->m_CompRef );

                if( tNode )
                {
                    tNode = FindPinMap( tNode );

                    if( tNode )
                    {
                        mNode = tNode->GetChildren();

                        while( mNode )
                        {
                            if( mNode->GetName().IsSameAs( wxT( "padNum" ), false ) )
                            {
                                str     = mNode->GetNodeContent();
                                mNode   = mNode->GetNext();

                                if( !mNode )
                                    break;

                                mNode->GetAttribute( wxT( "Name" ), &propValue );
                                fp->SetName( str, propValue );
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

                m_PcbComponents.Add( fp );
            }
        }
        else if( lNode->GetName().IsSameAs( wxT( "pad" ), false ) )
        {
            pad = new PCAD_PAD( this, m_board );
            pad->Parse( lNode, m_DefaultMeasurementUnit, aActualConversion );
            m_PcbComponents.Add( pad );
        }
        else if( lNode->GetName().IsSameAs( wxT( "via" ), false ) )
        {
            via = new PCAD_VIA( this, m_board );
            via->Parse( lNode, m_DefaultMeasurementUnit, aActualConversion );
            m_PcbComponents.Add( via );
        }
        else if( lNode->GetName().IsSameAs( wxT( "polyKeepOut" ), false ) )
        {
            keepOut = new PCAD_KEEPOUT( m_callbacks, m_board, 0 );

            if( keepOut->Parse( lNode, m_DefaultMeasurementUnit, aActualConversion ) )
                m_PcbComponents.Add( keepOut );
            else
                delete keepOut;
        }

        lNode = lNode->GetNext();
    }
}


void PCAD_PCB::ConnectPinToNet( const wxString& aCompRef, const wxString& aPinRef,
                           const wxString& aNetName )
{
    PCAD_FOOTPRINT* footprint;
    PCAD_PAD*       cp;
    int            i, j;

    for( i = 0; i < (int) m_PcbComponents.GetCount(); i++ )
    {
        footprint = (PCAD_FOOTPRINT*) m_PcbComponents[i];

        if( footprint->m_ObjType == wxT( 'M' ) && footprint->m_Name.text == aCompRef )
        {
            for( j = 0; j < (int) footprint->m_FootprintItems.GetCount(); j++ )
            {
                if( footprint->m_FootprintItems[j]->m_ObjType == wxT( 'P' ) )
                {
                    cp = (PCAD_PAD*) footprint->m_FootprintItems[j];

                    if( cp->m_Name.text == aPinRef )
                        cp->m_Net = aNetName;
                }
            }
        }
    }
}


int PCAD_PCB::FindLayer( const wxString& aLayerName ) const
{
    int  layerIndex = -1;
    long layerNum = -1;

    for( int i = 0; i < (int) m_layersStackup.size(); ++i )
    {
        if( m_layersStackup[i].first == aLayerName )
        {
            layerIndex = i;
            layerNum = m_layersStackup[i].second;
            break;
        }
    }

    switch( layerNum )
    {
    case -1: return -1;
    case 1: return F_Cu;
    case 2: return B_Cu;
    default: return ( layerIndex + 1 ) * 2;
    }
}


void PCAD_PCB::MapLayer( XNODE* aNode )
{
    wxString     lName, layerType;
    PCB_LAYER_ID KiCadLayer = UNDEFINED_LAYER;
    long         num = 0;

    aNode->GetAttribute( wxT( "Name" ), &lName );
    lName = lName.MakeUpper();

    if( lName == wxT( "TOP ASSY" ) )
    {
        KiCadLayer = F_Fab;
    }
    else if( lName == wxT( "TOP SILK" ) )
    {
        KiCadLayer = F_SilkS;
    }
    else if( lName == wxT( "TOP PASTE" ) )
    {
        KiCadLayer = F_Paste;
    }
    else if( lName == wxT( "TOP MASK" ) )
    {
        KiCadLayer = F_Mask;
    }
    else if( lName == wxT( "TOP" ) )
    {
        KiCadLayer = F_Cu;
    }
    else if( lName == wxT( "BOTTOM" ) )
    {
        KiCadLayer = B_Cu;
    }
    else if( lName == wxT( "BOT MASK" ) )
    {
        KiCadLayer = B_Mask;
    }
    else if( lName == wxT( "BOT PASTE" ) )
    {
        KiCadLayer = B_Paste;
    }
    else if( lName == wxT( "BOT SILK" ) )
    {
        KiCadLayer = B_SilkS;
    }
    else if( lName == wxT( "BOT ASSY" ) )
    {
        KiCadLayer = B_Fab;
    }
    else if( lName == wxT( "BOARD" ) )
    {
        KiCadLayer = Edge_Cuts;
    }
    else
    {
        int layernum = FindLayer( lName );

        if( layernum != -1 )
            KiCadLayer = ToLAYER_ID( layernum );
    }

    if( FindNode( aNode, wxT( "layerNum" ) ) )
        FindNode( aNode, wxT( "layerNum" ) )->GetNodeContent().ToLong( &num );

    if( num < 0 )
        THROW_IO_ERROR( wxString::Format( wxT( "layerNum = %ld is out of range" ), num ) );

    TLAYER newlayer;
    newlayer.KiCadLayer = KiCadLayer;
    newlayer.hasContent = false;

    if( KiCadLayer == UNDEFINED_LAYER )
    {
        newlayer.KiCadLayer = Dwgs_User; // default
    }

    if( FindNode( aNode, wxT( "layerType" ) ) )
    {
        layerType = FindNode( aNode, wxT( "layerType" ) )->GetNodeContent().Trim( false );

        if( layerType.IsSameAs( wxT( "NonSignal" ), false ) )
            newlayer.layerType = LAYER_TYPE_NONSIGNAL;

        if( layerType.IsSameAs( wxT( "Signal" ), false ) )
            newlayer.layerType = LAYER_TYPE_SIGNAL;

        if( layerType.IsSameAs( wxT( "Plane" ), false ) )
            newlayer.layerType = LAYER_TYPE_PLANE;
    }

    if( auto [it, success] = m_LayersMap.insert( std::make_pair( num, newlayer ) ); !success )
    {
        // If we can't assign layer by name or stackup, but it has been already assigned by default pcad id in constructor, we don't want to change the assignment
        if( KiCadLayer == UNDEFINED_LAYER )
        {
            newlayer.KiCadLayer = it->second.KiCadLayer;
        }
        it->second = newlayer;
    }

    if( FindNode( aNode, wxT( "netNameRef" ) ) )
    {
        FindNode( aNode, wxT( "netNameRef" ) )->GetAttribute( wxT( "Name" ),
                                                              &m_LayersMap[(int) num].netNameRef );
        m_LayersMap[(int) num].netNameRef.Trim( false );
        m_LayersMap[(int) num].netNameRef.Trim( true );
        m_LayersMap[(int) num].netNameRef = ConvertNetName( m_LayersMap[(int) num].netNameRef );
    }
}

int PCAD_PCB::FindOutlinePoint( const VERTICES_ARRAY* aOutline, wxRealPoint aPoint ) const
{
    int i;

    for( i = 0; i < (int) aOutline->GetCount(); i++ )
    {
        if( *((*aOutline)[i]) == aPoint )
            return i;
    }

    return -1;
}


double PCAD_PCB::GetDistance( const wxRealPoint* aPoint1, const wxRealPoint* aPoint2 ) const
{
    return sqrt(  ( aPoint1->x - aPoint2->x ) * ( aPoint1->x - aPoint2->x ) +
                  ( aPoint1->y - aPoint2->y ) * ( aPoint1->y - aPoint2->y ) );
}

void PCAD_PCB::ExtractOutlinePointsFromEnhancedPolygon( const wxString& aActualConversion, XNODE* lNode )
{
    XNODE* epNode = nullptr;
    int    x = 0;
    int    y = 0;

    epNode = FindNode( lNode, wxT( "enhancedPolygon" ) );

    if( !epNode )
        return;

    epNode = epNode->GetChildren();

    while( epNode )
    {
        if( epNode->GetName().IsSameAs( wxT( "polyPoint" ), false ) )
        {
            SetPosition( epNode->GetNodeContent(), m_DefaultMeasurementUnit, &x, &y, aActualConversion );

            if( FindOutlinePoint( &m_BoardOutline, wxRealPoint( x, y ) ) == -1 )
                m_BoardOutline.Add( new wxRealPoint( x, y ) );
        }

        epNode = epNode->GetNext();
    }
}

void PCAD_PCB::GetBoardOutline( wxXmlDocument* aXmlDoc, const wxString& aActualConversion )
{
    XNODE *      iNode, *lNode, *pNode, *epNode;
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
            if( iNode->GetName().IsSameAs( wxT( "layerContents" ), false ) )
            {
                if( FindNode( iNode, wxT( "layerNumRef" ) ) )
                    FindNode( iNode, wxT( "layerNumRef" ) )->GetNodeContent().ToLong( &PCadLayer );

                if( GetKiCadLayer( PCadLayer ) == Edge_Cuts )
                {
                    lNode = iNode->GetChildren();

                    while( lNode )
                    {
                        if( lNode->GetName().IsSameAs( wxT( "boardOutlineObj" ), false ) )
                        {
                            ExtractOutlinePointsFromEnhancedPolygon( aActualConversion, lNode );
                        }

                        if( lNode->GetName().IsSameAs( wxT( "line" ), false ) )
                        {
                            pNode = FindNode( lNode, wxT( "pt" ) );

                            if( pNode )
                            {
                                SetPosition( pNode->GetNodeContent(), m_DefaultMeasurementUnit,
                                             &x, &y, aActualConversion );

                                if( FindOutlinePoint( &m_BoardOutline, wxRealPoint( x, y) ) == -1 )
                                    m_BoardOutline.Add( new wxRealPoint( x, y ) );
                            }

                            if( pNode )
                                pNode = pNode->GetNext();

                            if( pNode )
                            {
                                SetPosition( pNode->GetNodeContent(), m_DefaultMeasurementUnit,
                                             &x, &y, aActualConversion );

                                if( FindOutlinePoint( &m_BoardOutline, wxRealPoint( x, y) ) == -1 )
                                    m_BoardOutline.Add( new wxRealPoint( x, y ) );
                            }
                        }

                        lNode = lNode->GetNext();
                    }

                    //m_boardOutline.Sort( cmpFunc );
                    // sort vertices according to the distances between them
                    if( m_BoardOutline.GetCount() > 3 )
                    {
                        for( i = 0; i < (int) m_BoardOutline.GetCount() - 1; i++ )
                        {
                            minDistance = GetDistance( m_BoardOutline[i], m_BoardOutline[ i + 1] );
                            targetInd = i + 1;

                            for( j = i + 2; j < (int) m_BoardOutline.GetCount(); j++ )
                            {
                                distance = GetDistance( m_BoardOutline[i], m_BoardOutline[j] );

                                if( distance < minDistance )
                                {
                                    minDistance = distance;
                                    targetInd = j;
                                }
                            }

                            xchgPoint = m_BoardOutline[ i + 1];
                            m_BoardOutline[ i + 1] = m_BoardOutline[targetInd];
                            m_BoardOutline[targetInd] = xchgPoint;
                        }
                    }

                    break;
                }
            }

            iNode = iNode->GetNext();
        }
    }
}


void PCAD_PCB::ExtractLayerStackup( wxXmlDocument* aXmlDoc )
{
    XNODE*   rootNode;
    XNODE*   aNode;
    XNODE*   aaNode;
    wxString layerName, layerType;
    bool     isSignalLayer;
    bool     stackupIncomplete = false;

    rootNode = FindNode( (XNODE*) aXmlDoc->GetRoot(), wxT( "pcbDesign" ) );

    if( rootNode )
    {
        aNode = FindNode( rootNode, wxT( "layersStackup" ) );

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
                        m_layersStackup.emplace_back( layerName,
                                                      -1 ); // pcad layer numbers to be fixed later
                    }
                }

                aNode = aNode->GetNext();
            }
        }

        // parse missing layers and numbers
        aNode = FindNode( rootNode, wxT( "layerDef" ) );

        while( aNode )
        {
            if( aNode->GetName().IsSameAs( wxT( "layerDef" ), false ) )
            {
                if( FindNode( aNode, wxT( "layerType" ) ) )
                {
                    long num = -1;

                    if( FindNode( aNode, wxT( "layerNum" ) ) )
                        FindNode( aNode, wxT( "layerNum" ) )->GetNodeContent().ToLong( &num );

                    layerType = FindNode( aNode, wxT( "layerType" ) )->GetNodeContent().Trim( false );
                    isSignalLayer =
                            layerType.IsSameAs( wxT( "Signal" ), false ) || layerType.IsSameAs( wxT( "Plane" ), false );

                    if( num > 0 )
                    {
                        aNode->GetAttribute( wxT( "Name" ), &layerName );
                        layerName = layerName.MakeUpper();

                        auto compare = [&layerName]( const auto& el )
                        {
                            return layerName.IsSameAs( el.first, false );
                        };

                        if( auto el = std::find_if( m_layersStackup.begin(), m_layersStackup.end(), compare );
                            el != m_layersStackup.end() )
                        {
                            if( isSignalLayer )
                            {
                                el->second = num;
                            }
                            else
                            {
                                // remove layer from stackup if it is not signal layer
                                m_layersStackup.erase( el );
                            }
                        }
                        else
                        {
                            if( isSignalLayer ) // ignore non signal layers
                            {
                                // Stackup defined inside pcad file is incomplete.
                                // We will ignore it and sort layers later preserving original imported behavior
                                stackupIncomplete = true;
                                m_layersStackup.emplace_back( layerName, num );
                            }
                        }
                    }
                }
            }

            aNode = aNode->GetNext();
        }


        if( stackupIncomplete )
        {
            // Stackup inside pcad file is incomplete, so we;ve added all missing signal layers at the end,
            // so now ensure that the layers are properly mapped to their order with the bottom
            // copper (layer 2 in PCAD) at the end
            std::sort( m_layersStackup.begin(), m_layersStackup.end(),
                       [&]( const std::pair<wxString, long>& a, const std::pair<wxString, long>& b )
                       {
                           long lhs = a.second == 2 ? std::numeric_limits<long>::max() : a.second;
                           long rhs = b.second == 2 ? std::numeric_limits<long>::max() : b.second;

                           return lhs < rhs;
                       } );
        }

        if( m_layersStackup.size() > 32 )
            THROW_IO_ERROR( _( "KiCad only supports 32 signal layers." ) );
    }
}

void PCAD_PCB::CreatePolygonsForEmptyPowerPlanes()
{
    PCAD_POLYGON* plane_layer = nullptr;

    for( const auto& [nbr, layer] : m_LayersMap )
    {
        if( layer.layerType == LAYER_TYPE_PLANE && layer.hasContent == false )
        {
            // fill the polygon with the same contour as its outline is
            plane_layer = new PCAD_POLYGON( m_callbacks, m_board, nbr );
            plane_layer->AssignNet( layer.netNameRef );
            plane_layer->SetOutline( &m_BoardOutline );
            m_PcbComponents.Add( plane_layer );
        }
    }
}

void PCAD_PCB::ProcessLayerContentsObjects( wxStatusBar* aStatusBar, const wxString& aActualConversion, XNODE* aNode )
{
    long num = 0;

    if( FindNode( aNode, wxT( "layerNumRef" ) ) )
        FindNode( aNode, wxT( "layerNumRef" ) )->GetNodeContent().ToLong( &num );

    if( num <= 0 )
        return;

    DoLayerContentsObjects( aNode, nullptr, &m_PcbComponents, aStatusBar, m_DefaultMeasurementUnit, aActualConversion );

    m_LayersMap[num].hasContent = true;
}

void PCAD_PCB::ParseBoard( wxStatusBar* aStatusBar, wxXmlDocument* aXmlDoc, const wxString& aActualConversion )
{
    XNODE*              aNode; //, *aaNode;
    PCAD_NET*           net;
    PCAD_PCB_COMPONENT* comp;
    PCAD_FOOTPRINT*     footprint;
    wxString            compRef, pinRef;
    int                 i, j, netCode;

    // Default measurement units
    aNode = FindNode( (XNODE*) aXmlDoc->GetRoot(), wxT( "asciiHeader" ) );

    if( aNode )
    {
        aNode = FindNode( aNode, wxT( "fileUnits" ) );

        if( aNode )
        {
            m_DefaultMeasurementUnit = aNode->GetNodeContent().Lower();
            m_DefaultMeasurementUnit.Trim( true );
            m_DefaultMeasurementUnit.Trim( false );
        }
    }

    // Determine layers stackup
    ExtractLayerStackup( aXmlDoc );

    // Layers mapping
    aNode = FindNode( (XNODE *)aXmlDoc->GetRoot(), wxT( "pcbDesign" ) );

    if( aNode )
    {
        aNode = FindNode( aNode, wxT( "layerDef" ) );

        while( aNode )
        {
            if( aNode->GetName().IsSameAs( wxT( "layerDef" ), false ) )
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
            net = new PCAD_NET( netCode++ );
            net->Parse( aNode );
            m_PcbNetlist.Add( net );

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
            // Components/footprints
            if( aNode->GetName().IsSameAs( wxT( "multiLayer" ), false ) )
                DoPCBComponents( aNode, aXmlDoc, aActualConversion, aStatusBar );

            // objects
            if( aNode->GetName().IsSameAs( wxT( "layerContents" ), false ) )
                ProcessLayerContentsObjects( aStatusBar, aActualConversion, aNode );

            aNode = aNode->GetNext();
        }

        CreatePolygonsForEmptyPowerPlanes();

        // POSTPROCESS -- SET NETLIST REFERENCES
        // aStatusBar->SetStatusText( wxT( "Processing NETLIST " ) );

        for( i = 0; i < (int) m_PcbNetlist.GetCount(); i++ )
        {
            net = m_PcbNetlist[i];

            for( j = 0; j < (int) net->m_NetNodes.GetCount(); j++ )
            {
                compRef = net->m_NetNodes[j]->m_CompRef;
                compRef.Trim( false );
                compRef.Trim( true );
                pinRef = net->m_NetNodes[j]->m_PinRef;
                pinRef.Trim( false );
                pinRef.Trim( true );
                ConnectPinToNet( compRef, pinRef, net->m_Name );
            }
        }

        // POSTPROCESS -- FLIP COMPONENTS
        for( i = 0; i < (int) m_PcbComponents.GetCount(); i++ )
        {
            if( m_PcbComponents[i]->m_ObjType == wxT( 'M' ) )
                ( (PCAD_FOOTPRINT*) m_PcbComponents[i] )->Flip();
        }

        // POSTPROCESS -- SET/OPTIMIZE NEW PCB POSITION
        // aStatusBar->SetStatusText( wxT( "Optimizing BOARD POSITION " ) );

        m_SizeX = 10000000;
        m_SizeY = 0;

        for( i = 0; i < (int) m_PcbComponents.GetCount(); i++ )
        {
            comp = m_PcbComponents[i];

            if( comp->m_PositionY < m_SizeY )
                m_SizeY = comp->m_PositionY; // max Y

            if( comp->m_PositionX < m_SizeX && comp->m_PositionX > 0 )
                m_SizeX = comp->m_PositionX; // Min X
        }

        m_SizeY -= 10000;
        m_SizeX -= 10000;
        // aStatusBar->SetStatusText( wxT( " POSITIONING POSTPROCESS " ) );

        for( i = 0; i < (int) m_PcbComponents.GetCount(); i++ )
            m_PcbComponents[i]->SetPosOffset( -m_SizeX, -m_SizeY );

        m_SizeX = 0;
        m_SizeY = 0;

        for( i = 0; i < (int) m_PcbComponents.GetCount(); i++ )
        {
            comp = m_PcbComponents[i];

            if( comp->m_PositionY < m_SizeY )
                m_SizeY = comp->m_PositionY; // max Y

            if( comp->m_PositionX > m_SizeX )
                m_SizeX = comp->m_PositionX; // Min X
        }

        // SHEET SIZE CALCULATION
        m_SizeY = -m_SizeY;    // it is in absolute units
        m_SizeX += 10000;
        m_SizeY += 10000;

        // A4 is minimum $Descr A4 11700 8267
        if( m_SizeX < 11700 )
            m_SizeX = 11700;

        if( m_SizeY < 8267 )
            m_SizeY = 8267;
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

                if( aNode->GetName().IsSameAs( wxT( "compDef" ), false ) )
                {
                    footprint = new PCAD_FOOTPRINT( this, m_board );
                    footprint->Parse( aNode, aStatusBar, m_DefaultMeasurementUnit,
                                      aActualConversion );
                    m_PcbComponents.Add( footprint );
                }

                aNode = aNode->GetNext();
            }
        }
    }
}


void PCAD_PCB::AddToBoard( FOOTPRINT* )
{
    int i;
    PCAD_NET* net;

    m_board->SetCopperLayerCount( m_layersStackup.size() );

    for( i = 0; i < (int) m_PcbNetlist.GetCount(); i++ )
    {
        net = m_PcbNetlist[i];

        m_board->Add( new NETINFO_ITEM( m_board, net->m_Name, net->m_NetCode ) );
    }

    for( i = 0; i < (int) m_PcbComponents.GetCount(); i++ )
    {
        m_PcbComponents[i]->AddToBoard();
    }
}

} // namespace PCAD2KICAD
