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
 * @file pcb_module.cpp
 */

#include <wx/wx.h>
#include <wx/config.h>

#include <common.h>

#include <pcb_arc.h>
#include <pcb_copper_pour.h>
#include <pcb_cutout.h>
#include <pcb_plane.h>
#include <pcb_line.h>
#include <pcb_module.h>
#include <pcb_pad.h>
#include <pcb_polygon.h>
#include <pcb_text.h>
#include <pcb_via.h>

namespace PCAD2KICAD {

PCB_MODULE::PCB_MODULE( PCB_CALLBACKS* aCallbacks, BOARD* aBoard ) : PCB_COMPONENT( aCallbacks,
                                                                                    aBoard )
{
    InitTTextValue( &m_value );
    m_mirror = 0;
    m_objType = wxT( 'M' );    // MODULE
    m_KiCadLayer = F_SilkS;  // default
}


PCB_MODULE::~PCB_MODULE()
{
    int i;

    for( i = 0; i < (int) m_moduleObjects.GetCount(); i++ )
    {
        delete m_moduleObjects[i];
    }
}


XNODE* PCB_MODULE::FindModulePatternDefName( XNODE* aNode, wxString aName )
{
    XNODE*      result, * lNode;
    wxString    propValue1, propValue2;

    result  = NULL;
    lNode   = FindNode( aNode, wxT( "patternDef" ) );

    while( lNode )
    {
        if( lNode->GetName() == wxT( "patternDef" ) )
        {
            lNode->GetAttribute( wxT( "Name" ), &propValue1 );
            FindNode( lNode,
                      wxT( "originalName" ) )->GetAttribute( wxT( "Name" ), &propValue2 );

            if( ValidateName( propValue1 ) == aName
                || ValidateName( propValue2 ) == aName )
            {
                result  = lNode;
                lNode   = NULL;
            }
        }

        if( lNode )
            lNode = lNode->GetNext();
    }

    if( result == NULL )
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
                    lNode   = NULL;
                }
            }

            if( lNode )
                lNode = lNode->GetNext();
        }
    }

    return result;
}


XNODE* PCB_MODULE::FindPatternMultilayerSection( XNODE* aNode, wxString* aPatGraphRefName )
{
    XNODE*      result, * pNode, * lNode;
    wxString    propValue, patName;

    result  = NULL;
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

    lNode = NULL;

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
        else
            lNode = FindNode( pNode, wxT( "patternGraphicsDef" ) );

        if( *aPatGraphRefName == wxEmptyString )    // no patern delection, the first is actual...
        {
            if( lNode )
            {
                result  = FindNode( lNode, wxT( "multiLayer" ) );
                lNode   = NULL;
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
                    lNode   = NULL;
                }
                else
                    lNode = lNode->GetNext();
            }
            else
                lNode = lNode->GetNext();
        }
    }

    return result;
}


void PCB_MODULE::DoLayerContentsObjects( XNODE*                 aNode,
                                         PCB_MODULE*            aPCBModule,
                                         PCB_COMPONENTS_ARRAY*  aList,
                                         wxStatusBar*           aStatusBar,
                                         wxString               aDefaultMeasurementUnit,
                                         wxString               aActualConversion )
{
    PCB_ARC*            arc;
    PCB_POLYGON*        polygon;
    PCB_POLYGON         *plane_layer = NULL;
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
        plane_layer->SetOutline( &m_boardOutline );
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

            if( propValue == wxT( "Type" ) )
            {
                tNode = FindNode( lNode, wxT( "textStyleRef" ) );

                if( tNode && aPCBModule )
                {
                    // TODO: to understand and may be repair
                    // Alexander Lunev: originally in Delphi version of the project there was
                    // a strange access to pcbModule->m_name (it was global variable). This access
                    // is necessary when the function DoLayerContentsObjects() is called from
                    // function CreatePCBModule(). However it is not clear whether the access is
                    // required when the function DoLayerContentsObjects() is called from
                    // function ProcessXMLtoPCBLib().
                    SetFontProperty( tNode,
                                     &aPCBModule->m_name,
                                     aDefaultMeasurementUnit,
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
                plane_layer->FormPolygon( lNode, plane_layer_polygon, aDefaultMeasurementUnit, aActualConversion );
                plane_layer->m_cutouts.Add( plane_layer_polygon );
            }
            else
            {
                polygon = new PCB_POLYGON( m_callbacks, m_board, PCadLayer );
                if( polygon->Parse( lNode,
                                    aDefaultMeasurementUnit,
                                    aActualConversion,
                                    aStatusBar ) )
                    aList->Add( polygon );
                else
                    delete polygon;
            }
        }

        if( lNode->GetName() == wxT( "copperPour95" ) )
        {
            copperPour = new PCB_COPPER_POUR( m_callbacks, m_board, PCadLayer );

            if( copperPour->Parse( lNode, aDefaultMeasurementUnit, aActualConversion,
                                   aStatusBar ) )
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

            if( plane->Parse( lNode, aDefaultMeasurementUnit, aActualConversion,
                              aStatusBar ) )
                aList->Add( plane );
            else
                delete plane;
        }

        lNode = lNode->GetNext();
    }
}


void PCB_MODULE::SetPadName( wxString aPin, wxString aName )
{
    int     i;
    long    num;

    aPin.ToLong( &num );

    for( i = 0; i < (int) m_moduleObjects.GetCount(); i++ )
    {
        if( m_moduleObjects[i]->m_objType == wxT( 'P' ) )
            if( ( (PCB_PAD*) m_moduleObjects[i] )->m_number == num )
                ( (PCB_PAD*) m_moduleObjects[i] )->m_name.text = aName;


    }
}


void PCB_MODULE::Parse( XNODE*   aNode, wxStatusBar* aStatusBar,
                        wxString aDefaultMeasurementUnit, wxString aActualConversion )
{
    XNODE*      lNode, * tNode, * mNode;
    PCB_PAD*    pad;
    PCB_VIA*    via;
    wxString    propValue, str;

    FindNode( aNode, wxT( "originalName" ) )->GetAttribute( wxT( "Name" ),
                                                            &propValue );
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
                m_moduleObjects.Add( pad );
            }

            if( tNode->GetName() == wxT( "via" ) )
            {
                via = new PCB_VIA( m_callbacks, m_board );
                via->Parse( tNode, aDefaultMeasurementUnit, aActualConversion );
                m_moduleObjects.Add( via );
            }

            tNode = tNode->GetNext();
        }
    }

    lNode   = lNode->GetParent();
    lNode   = FindNode( lNode, wxT( "layerContents" ) );

    while( lNode )
    {
        if( lNode->GetName() == wxT( "layerContents" ) )
            DoLayerContentsObjects( lNode, this, &m_moduleObjects, aStatusBar,
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
                SetPadName( str, propValue );
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


wxString PCB_MODULE::ModuleLayer( int aMirror )
{
    wxString result;

    // ///NOT !   {IntToStr(KiCadLayer)}    NOT !
    // /  MODULES ARE HARD PLACED ON COMPONENT OR COPPER LAYER.
    // /  IsFLIPPED--> MIRROR attribute is decision Point!!!

    if( aMirror == 0 )
        result = wxT( "15" );   // Components side
    else
        result = wxT( "0" );    // Copper side

    return result;
}


void PCB_MODULE::AddToBoard()
{
    int i;

    // transform text positions
    CorrectTextPosition( &m_name, m_rotation );
    CorrectTextPosition( &m_value, m_rotation );

    MODULE* module = new MODULE( m_board );
    m_board->Add( module, ADD_APPEND );

    module->SetPosition( wxPoint( m_positionX, m_positionY ) );
    module->SetLayer( m_mirror ? B_Cu : F_Cu );
    module->SetOrientation( m_rotation );
    module->SetTimeStamp( 0 );
    module->SetLastEditTime( 0 );

    module->SetFPID( FPID( m_compRef ) );

    module->SetAttributes( MOD_DEFAULT | MOD_CMS );

    // reference text
    TEXTE_MODULE* ref_text = &module->Reference();

    ref_text->SetText( m_name.text );
    ref_text->SetType( TEXTE_MODULE::TEXT_is_REFERENCE );

    ref_text->SetPos0( wxPoint( m_name.correctedPositionX, m_name.correctedPositionY ) );
    ref_text->SetSize( wxSize( KiROUND( m_name.textHeight / 2 ),
                               KiROUND( m_name.textHeight / 1.5 ) ) );

    ref_text->SetOrientation( m_name.textRotation );
    ref_text->SetThickness( m_name.textstrokeWidth );

    ref_text->SetMirrored( m_name.mirror );
    ref_text->SetVisible( m_name.textIsVisible );

    ref_text->SetLayer( m_KiCadLayer );

    // Calculate the actual position.
    ref_text->SetDrawCoord();

    // value text
    TEXTE_MODULE* val_text = &module->Value();

    val_text->SetText( m_value.text );
    val_text->SetType( TEXTE_MODULE::TEXT_is_VALUE );

    val_text->SetPos0( wxPoint( m_value.correctedPositionX, m_value.correctedPositionY ) );
    val_text->SetSize( wxSize( KiROUND( m_value.textHeight / 2 ),
                               KiROUND( m_value.textHeight / 1.5 ) ) );

    val_text->SetOrientation( m_value.textRotation );
    val_text->SetThickness( m_value.textstrokeWidth );

    val_text->SetMirrored( m_value.mirror );
    val_text->SetVisible( m_value.textIsVisible );

    val_text->SetLayer( m_KiCadLayer );

    // Calculate the actual position.
    val_text->SetDrawCoord();

    // TEXTS
    for( i = 0; i < (int) m_moduleObjects.GetCount(); i++ )
    {
        if( m_moduleObjects[i]->m_objType == wxT( 'T' ) )
        {
            ( (PCB_TEXT*) m_moduleObjects[i] )->m_tag = i + 2;
            m_moduleObjects[i]->AddToModule( module );
        }
    }

    // MODULE LINES
    for( i = 0; i < (int) m_moduleObjects.GetCount(); i++ )
    {
        if( m_moduleObjects[i]->m_objType == wxT( 'L' ) )
            m_moduleObjects[i]->AddToModule( module );
    }

    // MODULE Arcs
    for( i = 0; i < (int) m_moduleObjects.GetCount(); i++ )
    {
        if( m_moduleObjects[i]->m_objType == wxT( 'A' ) )
            m_moduleObjects[i]->AddToModule( module );
    }

    // PADS
    for( i = 0; i < (int) m_moduleObjects.GetCount(); i++ )
    {
        if( m_moduleObjects[i]->m_objType == wxT( 'P' ) )
            ( (PCB_PAD*) m_moduleObjects[i] )->AddToModule( module, m_rotation, false );
    }

    // VIAS
    for( i = 0; i < (int) m_moduleObjects.GetCount(); i++ )
    {
        if( m_moduleObjects[i]->m_objType == wxT( 'V' ) )
            ( (PCB_VIA*) m_moduleObjects[i] )->AddToModule( module, m_rotation, false );
    }

    module->CalculateBoundingBox();
}


void PCB_MODULE::Flip()
{
    int i;

    if( m_mirror == 1 )
    {
        // Flipped
        m_KiCadLayer    = FlipLayer( m_KiCadLayer );
        m_rotation      = -m_rotation;
        m_name.textPositionX = -m_name.textPositionX;
        m_name.mirror = m_mirror;
        m_value.textPositionX = -m_value.textPositionX;
        m_value.mirror = m_mirror;

        for( i = 0; i < (int) m_moduleObjects.GetCount(); i++ )
        {
            if( m_moduleObjects[i]->m_objType == wxT( 'L' ) || // lines
                m_moduleObjects[i]->m_objType == wxT( 'A' ) || // arcs
                m_moduleObjects[i]->m_objType == wxT( 'P' ) || // pads
                m_moduleObjects[i]->m_objType == wxT( 'V' ) )  // vias
            {
                m_moduleObjects[i]->Flip();
            }
        }
    }
}

} // namespace PCAD2KICAD
