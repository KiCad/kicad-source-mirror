/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007, 2008 Lubo Racko <developer@lura.sk>
 * Copyright (C) 2007, 2008, 2012-2013 Alexander Lunev <al.lunev@yahoo.com>
 * Copyright (C) 2012-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pcad/pcb_arc.h>
#include <pcad/pcb_copper_pour.h>
#include <pcad/pcb_cutout.h>
#include <pcad/pcb_plane.h>
#include <pcad/pcb_line.h>
#include <pcad/pcb_footprint.h>
#include <pcad/pcb_pad.h>
#include <pcad/pcb_polygon.h>
#include <pcad/pcb_text.h>
#include <pcad/pcb_via.h>

#include <board.h>
#include <footprint.h>
#include <trigo.h>
#include <xnode.h>

#include <wx/gdicmn.h>
#include <wx/string.h>

namespace PCAD2KICAD {


PCB_FOOTPRINT::PCB_FOOTPRINT( PCB_CALLBACKS* aCallbacks, BOARD* aBoard ) :
        PCB_COMPONENT( aCallbacks, aBoard )
{
    InitTTextValue( &m_Value );
    m_Mirror = 0;
    m_objType = wxT( 'M' );  // FOOTPRINT
    m_KiCadLayer = F_SilkS;  // default
}


PCB_FOOTPRINT::~PCB_FOOTPRINT()
{
    int i;

    for( i = 0; i < (int) m_FootprintItems.GetCount(); i++ )
    {
        delete m_FootprintItems[i];
    }
}


XNODE* PCB_FOOTPRINT::FindModulePatternDefName( XNODE* aNode, const wxString& aName )
{
    XNODE*      result, * lNode;
    wxString    propValue1, propValue2;

    result  = nullptr;
    lNode   = FindNode( aNode, wxT( "patternDef" ) );

    while( lNode )
    {
        if( lNode->GetName() == wxT( "patternDef" ) )
        {
            lNode->GetAttribute( wxT( "Name" ), &propValue1 );
            FindNode( lNode,
                      wxT( "originalName" ) )->GetAttribute( wxT( "Name" ), &propValue2 );

            if( ValidateName( propValue1 ) == aName || ValidateName( propValue2 ) == aName )
            {
                result  = lNode;
                lNode   = nullptr;
            }
        }

        if( lNode )
            lNode = lNode->GetNext();
    }

    if( result == nullptr )
    {
        lNode = FindNode( aNode, wxT( "patternDefExtended" ) );  // New file format

        while( lNode )
        {
            if( lNode->GetName() == wxT( "patternDefExtended" ) )
            {
                lNode->GetAttribute( wxT( "Name" ), &propValue1 );

                if( ValidateName( propValue1 ) == aName )
                {
                    result  = lNode;
                    lNode   = nullptr;
                }
            }

            if( lNode )
                lNode = lNode->GetNext();
        }
    }

    return result;
}


XNODE* PCB_FOOTPRINT::FindPatternMultilayerSection( XNODE* aNode, wxString* aPatGraphRefName )
{
    XNODE*      result, * pNode, * lNode;
    wxString    propValue, patName;

    result  = nullptr;
    pNode   = aNode; // pattern;
    lNode   = aNode;

    // calling from library  conversion we need to find pattern
    if( lNode->GetName() == wxT( "compDef" ) )
    {
        lNode->GetAttribute( wxT( "Name" ), &propValue );
        propValue.Trim( false );
        patName = ValidateName( propValue );

        if( FindNode( lNode, wxT( "attachedPattern" ) ) )
        {
            FindNode( FindNode( lNode, wxT( "attachedPattern" ) ),
                      wxT( "patternName" ) )->GetAttribute( wxT( "Name" ), &propValue );
            propValue.Trim( false );
            propValue.Trim( true );
            patName = ValidateName( propValue );
        }

        lNode   = FindModulePatternDefName( lNode->GetParent(), patName );
        pNode   = lNode; // pattern;
    }

    lNode = nullptr;

    if( pNode )
        lNode = FindNode( pNode, wxT( "multiLayer" ) );  // Old file format

    if( lNode )
    {
        *aPatGraphRefName = wxEmptyString; // default
        result = lNode;
    }
    else
    {
        // New file format

        if( *aPatGraphRefName == wxEmptyString ) // default
        {
            if( FindNode( aNode, wxT( "patternGraphicsNameRef" ) ) )
            {
                FindNode( aNode,
                          wxT( "patternGraphicsNameRef" ) )->GetAttribute( wxT( "Name" ),
                                                                           aPatGraphRefName );
            }
        }

        if( FindNode( aNode, wxT( "patternGraphicsDef" ) ) )
            lNode = FindNode( aNode, wxT( "patternGraphicsDef" ) );
        else if( pNode )
            lNode = FindNode( pNode, wxT( "patternGraphicsDef" ) );

        if( *aPatGraphRefName == wxEmptyString )    // no pattern detection, the first is actual...
        {
            if( lNode )
            {
                result  = FindNode( lNode, wxT( "multiLayer" ) );
                lNode   = nullptr;
            }
        }

        while( lNode )     // selected by name
        {
            if( lNode->GetName() == wxT( "patternGraphicsDef" ) )
            {
                FindNode( lNode,
                          wxT( "patternGraphicsNameDef" ) )->GetAttribute( wxT( "Name" ),
                                                                           &propValue );

                if( propValue == *aPatGraphRefName )
                {
                    result  = FindNode( lNode, wxT( "multiLayer" ) );
                    lNode   = nullptr;
                }
                else
                {
                    lNode = lNode->GetNext();
                }
            }
            else
            {
                lNode = lNode->GetNext();
            }
        }
    }

    return result;
}


void PCB_FOOTPRINT::DoLayerContentsObjects( XNODE* aNode, PCB_FOOTPRINT* aFootprint,
                                            PCB_COMPONENTS_ARRAY* aList, wxStatusBar* aStatusBar,
                                            const wxString& aDefaultMeasurementUnit,
                                            const wxString& aActualConversion )
{
    PCB_ARC*            arc;
    PCB_POLYGON*        polygon;
    PCB_POLYGON         *plane_layer = nullptr;
    PCB_COPPER_POUR*    copperPour;
    PCB_CUTOUT*         cutout;
    PCB_PLANE*          plane;
    VERTICES_ARRAY*     plane_layer_polygon;
    PCB_LINE*           line;
    PCB_TEXT*           text;
    XNODE*              lNode, * tNode;
    wxString            propValue;
    long long           i;
    int PCadLayer;
    long                num = 0;

    i = 0;

    // aStatusBar->SetStatusText( wxT( "Processing LAYER CONTENT OBJECTS " ) );
    if( FindNode( aNode, wxT( "layerNumRef" ) ) )
        FindNode( aNode, wxT( "layerNumRef" ) )->GetNodeContent().ToLong( &num );

    PCadLayer = (int) num;

    if( m_callbacks->GetLayerType( PCadLayer ) == LAYER_TYPE_PLANE )
    {
        plane_layer = new PCB_POLYGON( m_callbacks, m_board, PCadLayer );
        plane_layer->AssignNet( m_callbacks->GetLayerNetNameRef( PCadLayer ) );
        plane_layer->SetOutline( &m_BoardOutline );
        aList->Add( plane_layer );

        // fill the polygon with the same contour as its outline is
        //plane_layer->AddIsland( &m_boardOutline );
    }

    lNode = aNode->GetChildren();

    while( lNode )
    {
        i++;
        // aStatusBar->SetStatusText( wxString::Format( "Processing LAYER CONTENT OBJECTS :%lld",
        // i ) );

        if( lNode->GetName() == wxT( "line" ) )
        {
            line = new PCB_LINE( m_callbacks, m_board );
            line->Parse( lNode, PCadLayer, aDefaultMeasurementUnit, aActualConversion );
            aList->Add( line );
        }

        if( lNode->GetName() == wxT( "text" ) )
        {
            text = new PCB_TEXT( m_callbacks, m_board );
            text->Parse( lNode, PCadLayer, aDefaultMeasurementUnit, aActualConversion );
            aList->Add( text );
        }

        // added  as Sergeys request 02/2008
        if( lNode->GetName() == wxT( "attr" ) )
        {
            // assign fonts to Module Name,Value,Type,....s
            lNode->GetAttribute( wxT( "Name" ), &propValue );
            propValue.Trim( false );
            propValue.Trim( true );

            if( propValue == wxT( "RefDes" ) )
            {
                tNode = FindNode( lNode, wxT( "textStyleRef" ) );

                if( tNode && aFootprint )
                {
                    // TODO: to understand and may be repair
                    // Alexander Lunev: originally in Delphi version of the project there was
                    // a strange access to pcbModule->m_name (it was global variable). This access
                    // is necessary when the function DoLayerContentsObjects() is called from
                    // function CreatePCBModule(). However it is not clear whether the access is
                    // required when the function DoLayerContentsObjects() is called from
                    // function ProcessXMLtoPCBLib().
                    SetFontProperty( tNode, &aFootprint->m_name, aDefaultMeasurementUnit,
                                     aActualConversion );
                }
            }
        }

        // added  as Sergeys request 02/2008
        if( lNode->GetName() == wxT( "arc" ) || lNode->GetName() == wxT( "triplePointArc" ) )
        {
            arc = new PCB_ARC( m_callbacks, m_board );
            arc->Parse( lNode, PCadLayer, aDefaultMeasurementUnit, aActualConversion );
            aList->Add( arc );
        }

        if( lNode->GetName() == wxT( "pcbPoly" ) )
        {
            if( m_callbacks->GetLayerType( PCadLayer ) == LAYER_TYPE_PLANE )
            {
                plane_layer_polygon = new VERTICES_ARRAY;
                wxASSERT( plane_layer );
                plane_layer->FormPolygon( lNode, plane_layer_polygon, aDefaultMeasurementUnit,
                                          aActualConversion );
                plane_layer->m_cutouts.Add( plane_layer_polygon );
            }
            else
            {
                polygon = new PCB_POLYGON( m_callbacks, m_board, PCadLayer );

                if( polygon->Parse( lNode, aDefaultMeasurementUnit, aActualConversion ) )
                    aList->Add( polygon );
                else
                    delete polygon;
            }
        }

        if( lNode->GetName() == wxT( "copperPour95" ) )
        {
            copperPour = new PCB_COPPER_POUR( m_callbacks, m_board, PCadLayer );

            if( copperPour->Parse( lNode, aDefaultMeasurementUnit, aActualConversion ) )
                aList->Add( copperPour );
            else
                delete copperPour;
        }

        if( lNode->GetName() == wxT( "polyCutOut" ) )
        {
            cutout = new PCB_CUTOUT( m_callbacks, m_board, PCadLayer );

            if( cutout->Parse( lNode, aDefaultMeasurementUnit, aActualConversion ) )
                aList->Add( cutout );
            else
                delete cutout;
        }

        if( lNode->GetName() == wxT( "planeObj" ) )
        {
            plane = new PCB_PLANE( m_callbacks, m_board, PCadLayer );

            if( plane->Parse( lNode, aDefaultMeasurementUnit, aActualConversion ) )
                aList->Add( plane );
            else
                delete plane;
        }

        lNode = lNode->GetNext();
    }
}


void PCB_FOOTPRINT::SetName( const wxString& aPin, const wxString& aName )
{
    int     i;
    long    num;

    aPin.ToLong( &num );

    for( i = 0; i < (int) m_FootprintItems.GetCount(); i++ )
    {
        if( m_FootprintItems[i]->m_objType == wxT( 'P' ) )
        {
            if( ( (PCB_PAD*) m_FootprintItems[i] )->m_Number == num )
                ( (PCB_PAD*) m_FootprintItems[i] )->m_name.text = aName;
        }
    }
}


void PCB_FOOTPRINT::Parse( XNODE* aNode, wxStatusBar* aStatusBar,
                           const wxString& aDefaultMeasurementUnit,
                           const wxString& aActualConversion )
{
    XNODE*      lNode, * tNode, * mNode;
    PCB_PAD*    pad;
    PCB_VIA*    via;
    wxString    propValue, str;

    FindNode( aNode, wxT( "originalName" ) )->GetAttribute( wxT( "Name" ), &propValue );
    propValue.Trim( false );
    m_name.text = propValue;

    // aStatusBar->SetStatusText( wxT( "Creating Component : " ) + m_name.text );
    lNode   = aNode;
    lNode   = FindPatternMultilayerSection( lNode, &m_patGraphRefName );

    if( lNode )
    {
        tNode   = lNode;
        tNode   = tNode->GetChildren();

        while( tNode )
        {
            if( tNode->GetName() == wxT( "pad" ) )
            {
                pad = new PCB_PAD( m_callbacks, m_board );
                pad->Parse( tNode, aDefaultMeasurementUnit, aActualConversion );
                m_FootprintItems.Add( pad );
            }

            if( tNode->GetName() == wxT( "via" ) )
            {
                via = new PCB_VIA( m_callbacks, m_board );
                via->Parse( tNode, aDefaultMeasurementUnit, aActualConversion );
                m_FootprintItems.Add( via );
            }

            tNode = tNode->GetNext();
        }

        lNode = lNode->GetParent();
    }

    if( lNode )
        lNode = FindNode( lNode, wxT( "layerContents" ) );

    while( lNode )
    {
        if( lNode->GetName() == wxT( "layerContents" ) )
            DoLayerContentsObjects( lNode, this, &m_FootprintItems, aStatusBar,
                                    aDefaultMeasurementUnit, aActualConversion );

        lNode = lNode->GetNext();
    }

    // map pins
    lNode = FindPinMap( aNode );

    if( lNode )
    {
        mNode = lNode->GetChildren();

        while( mNode )
        {
            if( mNode->GetName() == wxT( "padNum" ) )
            {
                str     = mNode->GetNodeContent();
                mNode   = mNode->GetNext();

                if( !mNode )
                    break;

                mNode->GetAttribute( wxT( "Name" ), &propValue );
                SetName( str, propValue );
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


wxString PCB_FOOTPRINT::ModuleLayer( int aMirror )
{
    wxString result;

    // ///NOT !   {IntToStr(KiCadLayer)}    NOT !
    // /  FOOTPRINTs ARE HARD PLACED ON COMPONENT OR COPPER LAYER.
    // /  IsFLIPPED--> MIRROR attribute is decision Point!!!

    if( aMirror == 0 )
        result = wxT( "15" );   // Components side
    else
        result = wxT( "0" );    // Copper side

    return result;
}


void PCB_FOOTPRINT::AddToBoard()
{
    int i;
    int r;

    // transform text positions
    CorrectTextPosition( &m_name );
    RotatePoint( &m_name.correctedPositionX, &m_name.correctedPositionY, (double) -m_rotation );

    CorrectTextPosition( &m_Value );
    RotatePoint( &m_Value.correctedPositionX, &m_Value.correctedPositionY, (double) -m_rotation );

    FOOTPRINT* footprint = new FOOTPRINT( m_board );
    m_board->Add( footprint, ADD_MODE::APPEND );

    footprint->SetPosition( wxPoint( m_positionX, m_positionY ) );
    footprint->SetLayer( m_Mirror ? B_Cu : F_Cu );
    footprint->SetOrientation( m_rotation );
    footprint->SetLastEditTime( 0 );

    LIB_ID fpID;
    fpID.Parse( m_compRef, true );
    footprint->SetFPID( fpID );

    // reference text
    FP_TEXT* ref_text = &footprint->Reference();

    ref_text->SetText( ValidateReference( m_name.text ) );
    ref_text->SetType( FP_TEXT::TEXT_is_REFERENCE );

    ref_text->SetPos0( wxPoint( m_name.correctedPositionX, m_name.correctedPositionY ) );

    if( m_name.isTrueType )
        SetTextSizeFromTrueTypeFontHeight( ref_text, m_name.textHeight );
    else
        SetTextSizeFromStrokeFontHeight( ref_text, m_name.textHeight );

    r = m_name.textRotation - m_rotation;
    ref_text->SetTextAngle( r );
    ref_text->SetKeepUpright( false );

    ref_text->SetItalic( m_name.isItalic );
    ref_text->SetTextThickness( m_name.textstrokeWidth );

    ref_text->SetMirrored( m_name.mirror );
    ref_text->SetVisible( m_name.textIsVisible );

    ref_text->SetLayer( m_name.mirror ? FlipLayer( m_KiCadLayer ) : m_KiCadLayer );

    // Calculate the actual position.
    ref_text->SetDrawCoord();

    // value text
    FP_TEXT* val_text = &footprint->Value();

    val_text->SetText( m_Value.text );
    val_text->SetType( FP_TEXT::TEXT_is_VALUE );

    val_text->SetPos0( wxPoint( m_Value.correctedPositionX, m_Value.correctedPositionY ) );

    if( m_Value.isTrueType )
        SetTextSizeFromTrueTypeFontHeight( val_text, m_Value.textHeight );
    else
        SetTextSizeFromStrokeFontHeight( val_text, m_Value.textHeight );

    r = m_Value.textRotation - m_rotation;
    val_text->SetTextAngle( r );
    val_text->SetKeepUpright( false );

    val_text->SetItalic( m_Value.isItalic );
    val_text->SetTextThickness( m_Value.textstrokeWidth );

    val_text->SetMirrored( m_Value.mirror );
    val_text->SetVisible( m_Value.textIsVisible );

    val_text->SetLayer( m_Value.mirror ? FlipLayer( m_KiCadLayer ) : m_KiCadLayer );

    // Calculate the actual position.
    val_text->SetDrawCoord();

    // TEXTS
    for( i = 0; i < (int) m_FootprintItems.GetCount(); i++ )
    {
        if( m_FootprintItems[i]->m_objType == wxT( 'T' ) )
        {
            ( (PCB_TEXT*) m_FootprintItems[i] )->m_tag = i + 2;
            m_FootprintItems[ i ]->AddToFootprint( footprint );
        }
    }

    // FOOTPRINT LINES
    for( i = 0; i < (int) m_FootprintItems.GetCount(); i++ )
    {
        if( m_FootprintItems[i]->m_objType == wxT( 'L' ) )
            m_FootprintItems[ i ]->AddToFootprint( footprint );
    }

    // FOOTPRINT ARCS
    for( i = 0; i < (int) m_FootprintItems.GetCount(); i++ )
    {
        if( m_FootprintItems[i]->m_objType == wxT( 'A' ) )
            m_FootprintItems[ i ]->AddToFootprint( footprint );
    }

    // FOOTPRINT POLYGONS
    for( i = 0; i < (int) m_FootprintItems.GetCount(); i++ )
    {
        if( m_FootprintItems[i]->m_objType == wxT( 'Z' ) )
            m_FootprintItems[ i ]->AddToFootprint( footprint );
    }

    // PADS
    for( i = 0; i < (int) m_FootprintItems.GetCount(); i++ )
    {
        if( m_FootprintItems[i]->m_objType == wxT( 'P' ) )
            ((PCB_PAD*) m_FootprintItems[ i ] )->AddToFootprint( footprint, m_rotation, false );
    }

    // VIAS
    for( i = 0; i < (int) m_FootprintItems.GetCount(); i++ )
    {
        if( m_FootprintItems[i]->m_objType == wxT( 'V' ) )
            ((PCB_VIA*) m_FootprintItems[ i ] )->AddToFootprint( footprint, m_rotation, false );
    }
}


void PCB_FOOTPRINT::Flip()
{
    int i;

    if( m_Mirror == 1 )
    {
        m_rotation = -m_rotation;

        for( i = 0; i < (int) m_FootprintItems.GetCount(); i++ )
        {
            if( m_FootprintItems[i]->m_objType == wxT( 'L' ) || // lines
                m_FootprintItems[i]->m_objType == wxT( 'A' ) || // arcs
                m_FootprintItems[i]->m_objType == wxT( 'Z' ) || // polygons
                m_FootprintItems[i]->m_objType == wxT( 'P' ) || // pads
                m_FootprintItems[i]->m_objType == wxT( 'V' ) )  // vias
            {
                m_FootprintItems[i]->Flip();
            }
        }
    }
}

} // namespace PCAD2KICAD
