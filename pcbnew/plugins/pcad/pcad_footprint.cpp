/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007, 2008 Lubo Racko <developer@lura.sk>
 * Copyright (C) 2007, 2008, 2012-2013 Alexander Lunev <al.lunev@yahoo.com>
 * Copyright (C) 2012-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pcad/pcad_arc.h>
#include <pcad/pcad_copper_pour.h>
#include <pcad/pcad_cutout.h>
#include <pcad/pcad_plane.h>
#include <pcad/pcad_line.h>
#include <pcad/pcad_footprint.h>
#include <pcad/pcad_pad.h>
#include <pcad/pcad_polygon.h>
#include <pcad/pcad_text.h>
#include <pcad/pcad_via.h>

#include <board.h>
#include <footprint.h>
#include <pcb_text.h>
#include <trigo.h>
#include <xnode.h>

#include <wx/string.h>

namespace PCAD2KICAD {


PCAD_FOOTPRINT::PCAD_FOOTPRINT( PCAD_CALLBACKS* aCallbacks, BOARD* aBoard ) :
        PCAD_PCB_COMPONENT( aCallbacks, aBoard )
{
    InitTTextValue( &m_Value );
    m_Mirror = 0;
    m_ObjType = wxT( 'M' );  // FOOTPRINT
    m_KiCadLayer = F_SilkS;  // default
}


PCAD_FOOTPRINT::~PCAD_FOOTPRINT()
{
    int i;

    for( i = 0; i < (int) m_FootprintItems.GetCount(); i++ )
    {
        delete m_FootprintItems[i];
    }
}


XNODE* PCAD_FOOTPRINT::FindModulePatternDefName( XNODE* aNode, const wxString& aName )
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


XNODE* PCAD_FOOTPRINT::FindPatternMultilayerSection( XNODE* aNode, wxString* aPatGraphRefName )
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


void PCAD_FOOTPRINT::DoLayerContentsObjects( XNODE* aNode, PCAD_FOOTPRINT* aFootprint,
                                             PCAD_COMPONENTS_ARRAY* aList, wxStatusBar* aStatusBar,
                                             const wxString& aDefaultMeasurementUnit,
                                             const wxString& aActualConversion )
{
    PCAD_ARC*         arc;
    PCAD_POLYGON*     polygon;
    PCAD_POLYGON*     plane_layer = nullptr;
    PCAD_COPPER_POUR* copperPour;
    PCAD_CUTOUT*      cutout;
    PCAD_PLANE*       plane;
    VERTICES_ARRAY*   plane_layer_polygon;
    PCAD_LINE*        line;
    PCAD_TEXT*        text;
    XNODE*            lNode;
    XNODE*            tNode;
    wxString          propValue;
    long long         i;
    int               PCadLayer;
    long              num = 0;

    i = 0;

    // aStatusBar->SetStatusText( wxT( "Processing LAYER CONTENT OBJECTS " ) );
    if( FindNode( aNode, wxT( "layerNumRef" ) ) )
        FindNode( aNode, wxT( "layerNumRef" ) )->GetNodeContent().ToLong( &num );

    PCadLayer = (int) num;

    if( m_callbacks->GetLayerType( PCadLayer ) == LAYER_TYPE_PLANE )
    {
        plane_layer = new PCAD_POLYGON( m_callbacks, m_board, PCadLayer );
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
            line = new PCAD_LINE( m_callbacks, m_board );
            line->Parse( lNode, PCadLayer, aDefaultMeasurementUnit, aActualConversion );
            aList->Add( line );
        }

        if( lNode->GetName() == wxT( "text" ) )
        {
            text = new PCAD_TEXT( m_callbacks, m_board );
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
                    // a strange access to pcbModule->m_Name (it was global variable). This access
                    // is necessary when the function DoLayerContentsObjects() is called from
                    // function CreatePCBModule(). However it is not clear whether the access is
                    // required when the function DoLayerContentsObjects() is called from
                    // function ProcessXMLtoPCBLib().
                    SetFontProperty( tNode, &aFootprint->m_Name, aDefaultMeasurementUnit,
                                     aActualConversion );
                }
            }
        }

        // added  as Sergeys request 02/2008
        if( lNode->GetName() == wxT( "arc" ) || lNode->GetName() == wxT( "triplePointArc" ) )
        {
            arc = new PCAD_ARC( m_callbacks, m_board );
            arc->Parse( lNode, PCadLayer, aDefaultMeasurementUnit, aActualConversion );
            aList->Add( arc );
        }

        if( lNode->GetName() == wxT( "pcbPoly" ) )
        {
            if( m_callbacks->GetLayerType( PCadLayer ) == LAYER_TYPE_PLANE )
            {
                plane_layer_polygon = new VERTICES_ARRAY;
                plane_layer->FormPolygon( lNode, plane_layer_polygon, aDefaultMeasurementUnit,
                                          aActualConversion );
                plane_layer->m_Cutouts.Add( plane_layer_polygon );
            }
            else
            {
                polygon = new PCAD_POLYGON( m_callbacks, m_board, PCadLayer );

                if( polygon->Parse( lNode, aDefaultMeasurementUnit, aActualConversion ) )
                    aList->Add( polygon );
                else
                    delete polygon;
            }
        }

        if( lNode->GetName() == wxT( "copperPour95" ) )
        {
            copperPour = new PCAD_COPPER_POUR( m_callbacks, m_board, PCadLayer );

            if( copperPour->Parse( lNode, aDefaultMeasurementUnit, aActualConversion ) )
                aList->Add( copperPour );
            else
                delete copperPour;
        }

        if( lNode->GetName() == wxT( "polyCutOut" ) )
        {
            cutout = new PCAD_CUTOUT( m_callbacks, m_board, PCadLayer );

            if( cutout->Parse( lNode, aDefaultMeasurementUnit, aActualConversion ) )
                aList->Add( cutout );
            else
                delete cutout;
        }

        if( lNode->GetName() == wxT( "planeObj" ) )
        {
            plane = new PCAD_PLANE( m_callbacks, m_board, PCadLayer );

            if( plane->Parse( lNode, aDefaultMeasurementUnit, aActualConversion ) )
                aList->Add( plane );
            else
                delete plane;
        }

        lNode = lNode->GetNext();
    }
}


void PCAD_FOOTPRINT::SetName( const wxString& aPin, const wxString& aName )
{
    int     i;
    long    num;

    aPin.ToLong( &num );

    for( i = 0; i < (int) m_FootprintItems.GetCount(); i++ )
    {
        if( m_FootprintItems[i]->m_ObjType == wxT( 'P' ) )
        {
            if( ( (PCAD_PAD*) m_FootprintItems[i] )->m_Number == num )
                ( (PCAD_PAD*) m_FootprintItems[i] )->m_Name.text = aName;
        }
    }
}


void PCAD_FOOTPRINT::Parse( XNODE* aNode, wxStatusBar* aStatusBar,
                           const wxString& aDefaultMeasurementUnit,
                           const wxString& aActualConversion )
{
    XNODE*      lNode, * tNode, * mNode;
    PCAD_PAD*    pad;
    PCAD_VIA*    via;
    wxString    propValue, str;

    FindNode( aNode, wxT( "originalName" ) )->GetAttribute( wxT( "Name" ), &propValue );
    propValue.Trim( false );
    m_Name.text = propValue;

    // aStatusBar->SetStatusText( wxT( "Creating Component : " ) + m_Name.text );
    lNode   = aNode;
    lNode   = FindPatternMultilayerSection( lNode, &m_PatGraphRefName );

    if( lNode )
    {
        tNode   = lNode;
        tNode   = tNode->GetChildren();

        while( tNode )
        {
            if( tNode->GetName() == wxT( "pad" ) )
            {
                pad = new PCAD_PAD( m_callbacks, m_board );
                pad->Parse( tNode, aDefaultMeasurementUnit, aActualConversion );
                m_FootprintItems.Add( pad );
            }

            if( tNode->GetName() == wxT( "via" ) )
            {
                via = new PCAD_VIA( m_callbacks, m_board );
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


void PCAD_FOOTPRINT::AddToBoard( FOOTPRINT* aFootprint )
{
    // No nested footprints....
    wxCHECK( aFootprint == nullptr, /* void */ );

    int i;
    EDA_ANGLE r;

    // transform text positions
    CorrectTextPosition( &m_Name );
    RotatePoint( &m_Name.correctedPositionX, &m_Name.correctedPositionY, -m_Rotation );

    CorrectTextPosition( &m_Value );
    RotatePoint( &m_Value.correctedPositionX, &m_Value.correctedPositionY, -m_Rotation );

    FOOTPRINT* footprint = new FOOTPRINT( m_board );
    m_board->Add( footprint, ADD_MODE::APPEND );

    footprint->SetPosition( VECTOR2I( m_PositionX, m_PositionY ) );
    footprint->SetLayer( m_Mirror ? B_Cu : F_Cu );
    footprint->SetOrientation( m_Rotation );

    LIB_ID fpID;
    fpID.Parse( m_CompRef, true );
    footprint->SetFPID( fpID );

    // reference text
    PCB_TEXT* ref_text = &footprint->Reference();

    ref_text->SetText( ValidateReference( m_Name.text ) );
    ref_text->SetType( PCB_TEXT::TEXT_is_REFERENCE );

    ref_text->SetFPRelativePosition( VECTOR2I( m_Name.correctedPositionX,
                                               m_Name.correctedPositionY ) );

    if( m_Name.isTrueType )
        SetTextSizeFromTrueTypeFontHeight( ref_text, m_Name.textHeight );
    else
        SetTextSizeFromStrokeFontHeight( ref_text, m_Name.textHeight );

    r = m_Name.textRotation - m_Rotation;
    ref_text->SetTextAngle( r );
    ref_text->SetKeepUpright( false );

    ref_text->SetItalic( m_Name.isItalic );
    ref_text->SetTextThickness( m_Name.textstrokeWidth );

    ref_text->SetMirrored( m_Name.mirror );
    ref_text->SetVisible( m_Name.textIsVisible );

    ref_text->SetLayer( m_Mirror ? FlipLayer( m_KiCadLayer ) : m_KiCadLayer );

    // value text
    PCB_TEXT* val_text = &footprint->Value();

    val_text->SetText( m_Value.text );
    val_text->SetType( PCB_TEXT::TEXT_is_VALUE );

    val_text->SetFPRelativePosition( VECTOR2I( m_Value.correctedPositionX,
                                               m_Value.correctedPositionY ) );

    if( m_Value.isTrueType )
        SetTextSizeFromTrueTypeFontHeight( val_text, m_Value.textHeight );
    else
        SetTextSizeFromStrokeFontHeight( val_text, m_Value.textHeight );

    r = m_Value.textRotation - m_Rotation;
    val_text->SetTextAngle( r );
    val_text->SetKeepUpright( false );

    val_text->SetItalic( m_Value.isItalic );
    val_text->SetTextThickness( m_Value.textstrokeWidth );

    val_text->SetMirrored( m_Value.mirror );
    val_text->SetVisible( m_Value.textIsVisible );

    val_text->SetLayer( m_Value.mirror ? FlipLayer( m_KiCadLayer ) : m_KiCadLayer );

    // TEXTS
    for( i = 0; i < (int) m_FootprintItems.GetCount(); i++ )
    {
        if( m_FootprintItems[i]->m_ObjType == wxT( 'T' ) )
            m_FootprintItems[ i ]->AddToBoard( footprint );
    }

    // FOOTPRINT LINES
    for( i = 0; i < (int) m_FootprintItems.GetCount(); i++ )
    {
        if( m_FootprintItems[i]->m_ObjType == wxT( 'L' ) )
            m_FootprintItems[ i ]->AddToBoard( footprint );
    }

    // FOOTPRINT ARCS
    for( i = 0; i < (int) m_FootprintItems.GetCount(); i++ )
    {
        if( m_FootprintItems[i]->m_ObjType == wxT( 'A' ) )
            m_FootprintItems[ i ]->AddToBoard( footprint );
    }

    // FOOTPRINT POLYGONS
    for( i = 0; i < (int) m_FootprintItems.GetCount(); i++ )
    {
        if( m_FootprintItems[i]->m_ObjType == wxT( 'Z' ) )
            m_FootprintItems[ i ]->AddToBoard( footprint );
    }

    // PADS
    for( i = 0; i < (int) m_FootprintItems.GetCount(); i++ )
    {
        if( m_FootprintItems[i]->m_ObjType == wxT( 'P' ) )
            ((PCAD_PAD*) m_FootprintItems[ i ] )->AddToFootprint( footprint, m_Rotation, false );
    }

    // VIAS
    for( i = 0; i < (int) m_FootprintItems.GetCount(); i++ )
    {
        if( m_FootprintItems[i]->m_ObjType == wxT( 'V' ) )
            ((PCAD_VIA*) m_FootprintItems[ i ] )->AddToFootprint( footprint, m_Rotation, false );
    }
}


void PCAD_FOOTPRINT::Flip()
{
    int i;

    if( m_Mirror == 1 )
    {
        m_Rotation = -m_Rotation;

        for( i = 0; i < (int) m_FootprintItems.GetCount(); i++ )
        {
            if( m_FootprintItems[i]->m_ObjType == wxT( 'L' ) || // lines
                m_FootprintItems[i]->m_ObjType == wxT( 'A' ) || // arcs
                m_FootprintItems[i]->m_ObjType == wxT( 'Z' ) || // polygons
                m_FootprintItems[i]->m_ObjType == wxT( 'P' ) || // pads
                m_FootprintItems[i]->m_ObjType == wxT( 'V' ) )  // vias
            {
                m_FootprintItems[i]->Flip();
            }
        }
    }
}

} // namespace PCAD2KICAD
