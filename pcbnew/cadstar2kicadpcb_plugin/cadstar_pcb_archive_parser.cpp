/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Roberto Fernandez Bautista <@Qbort>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file cadstar_pcb_archive_parser.cpp
 * @brief Parses a CADSTAR PCB Archive file
 */

#include <cadstar_pcb_archive_parser.h>
#include <cmath> // pow()


void CPA_FILE::Parse()
{
    XNODE* fileRootNode =
            CADSTAR_COMMON::LoadArchiveFile( Filename, CADSTAR_COMMON::FILE_TYPE::PCB_ARCHIVE );

    XNODE* tempNode = fileRootNode->GetChildren();

    for( ; tempNode && ( tempNode->GetName() != wxT( "HEADER" ) ); tempNode = tempNode->GetNext() )
        ;

    if( !tempNode )
        THROW_MISSING_NODE_IO_ERROR( wxT( "HEADER" ), wxT( "CADSTARPCB" ) );

    //TODO try.. catch + throw again with more detailed error information
    Header.Parse( tempNode );

    switch( Header.Resolution )
    {
    case CPA_RESOLUTION::HUNDREDTH_MICRON:
        KiCadUnitMultiplier = 10;
        break;

    default:
        wxASSERT_MSG( true, wxT( "Unknown File Resolution" ) );
        break;
    }

    for( ; tempNode && ( tempNode->GetName() != wxT( "ASSIGNMENTS" ) );
            tempNode = tempNode->GetNext() )
        ;

    if( !tempNode )
        THROW_MISSING_NODE_IO_ERROR( wxT( "ASSIGNMENTS" ), wxT( "CADSTARPCB" ) );

    tempNode = tempNode->GetChildren();

    if( !tempNode )
        THROW_MISSING_NODE_IO_ERROR( wxT( "LAYERDEFS" ), wxT( "ASSIGNMENTS" ) );

    for( ; tempNode; tempNode = tempNode->GetNext() )
    {
        if( tempNode->GetName() == wxT( "LAYERDEFS" ) )
            //TODO try.. catch + throw again with more detailed error information
            Assignments.Layerdefs.Parse( tempNode );
        else if( tempNode->GetName() == wxT( "CODEDEFS" ) )
            Assignments.Codedefs.Parse( tempNode );
        //TODO parse technology, grids,etc
    }


    //Todo - This is just for testing. Remove this block & delete fileRootNode.
    {
        wxXmlDocument doc;
        doc.SetRoot( fileRootNode );
        doc.Save( Filename + wxT( ".xml" ) );
    }

    //delete fileRootNode;

    //TODO need to parse everything else!
}


void CPA_LAYERDEFS::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "LAYERDEFS" ) );

    wxXmlAttribute* xmlAttribute = NULL;

    XNODE* cNode = aNode->GetChildren();

    if( !cNode )
        THROW_MISSING_PARAMETER_IO_ERROR( wxT( "LAYERSTACK" ), wxT( "LAYERDEFS" ) );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString nodeName = cNode->GetName();

        if( nodeName == wxT( "LAYERSTACK" ) )
        {
            xmlAttribute = cNode->GetAttributes();

            for( ; xmlAttribute; xmlAttribute = xmlAttribute->GetNext() )
            {
                LayerStack.push_back( xmlAttribute->GetValue() );
            }

            CADSTAR_COMMON::CheckNoChildNodes( cNode );
        }
        else if( nodeName == wxT( "MATERIAL" ) )
        {
            CPA_MATERIAL material;
            //TODO try.. catch + throw again with more detailed error information
            material.Parse( cNode );
            Materials.insert( std::make_pair( material.ID, material ) );
        }
        else if( nodeName == wxT( "LAYER" ) )
        {
            CPA_LAYER layer;
            //TODO try.. catch + throw again with more detailed error information
            layer.Parse( cNode );
            Layers.insert( std::make_pair( layer.ID, layer ) );
        }
        else if( nodeName == wxT( "SWAPPAIR" ) )
        {
            wxString layerId     = CADSTAR_COMMON::GetAttributeID( cNode, 0 );
            wxString swapLayerId = CADSTAR_COMMON::GetAttributeID( cNode, 1 );

            Layers[layerId].SwapLayerID = swapLayerId;
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( nodeName, aNode->GetName() );
        }
    }
}


void CPA_CODEDEFS::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() != wxT( "CODEDEFS" ) );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString nodeName = cNode->GetName();

        if( nodeName == wxT( "LINECODE" ) )
        {
            CPA_LINECODE linecode;
            //TODO try.. catch + throw again with more detailed error information
            linecode.Parse( cNode );
            LineCodes.insert( std::make_pair( linecode.ID, linecode ) );
        }
        else if( nodeName == wxT( "HATCHCODE" ) )
        {
            CPA_HATCHCODE hatchcode;
            //TODO try.. catch + throw again with more detailed error information
            hatchcode.Parse( cNode );
            HatchCodes.insert( std::make_pair( hatchcode.ID, hatchcode ) );
        }
        else if( nodeName == wxT( "TEXTCODE" ) )
        {
            CPA_TEXTCODE textcode;
            //TODO try.. catch + throw again with more detailed error information
            textcode.Parse( cNode );
            TextCodes.insert( std::make_pair( textcode.ID, textcode ) );
        }
        else if( nodeName == wxT( "ROUTECODE" ) )
        {
            CPA_ROUTECODE routecode;
            //TODO try.. catch + throw again with more detailed error information
            routecode.Parse( cNode );
            RouteCodes.insert( std::make_pair( routecode.ID, routecode ) );
        }
        else if( nodeName == wxT( "COPPERCODE" ) )
        {
            CPA_COPPERCODE coppercode;
            //TODO try.. catch + throw again with more detailed error information
            coppercode.Parse( cNode );
            CopperCodes.insert( std::make_pair( coppercode.ID, coppercode ) );
        }
        else if( nodeName == wxT( "SPACINGCODE" ) )
        {
            CPA_SPACINGCODE spacingcode;
            //TODO try.. catch + throw again with more detailed error information
            spacingcode.Parse( cNode );
            SpacingCodes.push_back( spacingcode );
        }
        else if( nodeName == wxT( "PADCODE" ) )
        {
            CPA_PADCODE padcode;
            //TODO try.. catch + throw again with more detailed error information
            padcode.Parse( cNode );
            PadCodes.insert( std::make_pair( padcode.ID, padcode ) );
        }
        else if( nodeName == wxT( "VIACODE" ) )
        {
            CPA_VIACODE viacode;
            //TODO try.. catch + throw again with more detailed error information
            viacode.Parse( cNode );
            ViaCodes.insert( std::make_pair( viacode.ID, viacode ) );
        }
        else if( nodeName == wxT( "LAYERPAIR" ) )
        {
            CPA_LAYERPAIR layerpair;
            //TODO try.. catch + throw again with more detailed error information
            layerpair.Parse( cNode );
            LayerPairs.insert( std::make_pair( layerpair.ID, layerpair ) );
        }
        else if( nodeName == wxT( "ATTRNAME" ) )
        {
            CPA_ATTRNAME attrname;
            //TODO try.. catch + throw again with more detailed error information
            attrname.Parse( cNode );
            AttributeNames.insert( std::make_pair( attrname.ID, attrname ) );
        }
        else if( nodeName == wxT( "NETCLASS" ) )
        {
            CPA_NETCLASS netclass;
            //TODO try.. catch + throw again with more detailed error information
            netclass.Parse( cNode );
            NetClasses.insert( std::make_pair( netclass.ID, netclass ) );
        }
        else if( nodeName == wxT( "SPCCLASSNAME" ) )
        {
            CPA_SPCCLASSNAME spcclassname;
            //TODO try.. catch + throw again with more detailed error information
            spcclassname.Parse( cNode );
            SpacingClassNames.insert( std::make_pair( spcclassname.ID, spcclassname ) );
        }
        else if( nodeName == wxT( "SPCCLASSSPACE" ) )
        {
            CPA_SPCCLASSSPACE spcclassspace;
            //TODO try.. catch + throw again with more detailed error information
            spcclassspace.Parse( cNode );
            SpacingClasses.push_back( spcclassspace );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( nodeName, aNode->GetName() );
        }
    }
}


void CPA_MATERIAL::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "MATERIAL" ) );

    //Process Name & ID
    ID   = CADSTAR_COMMON::GetAttributeID( aNode, 0 );
    Name = CADSTAR_COMMON::GetAttributeID( aNode, 1 );

    //Process Type
    wxString sType = CADSTAR_COMMON::GetAttributeID( aNode, 2 );

    if( sType == wxT( "CONSTRUCTION" ) )
    {
        Type = CPA_MATERIAL_LAYER_TYPE::CONSTRUCTION;
    }
    else if( sType == wxT( "ELECTRICAL" ) )
    {
        Type = CPA_MATERIAL_LAYER_TYPE::ELECTRICAL;
    }
    else if( sType == wxT( "NONELEC" ) )
    {
        Type = CPA_MATERIAL_LAYER_TYPE::NON_ELECTRICAL;
    }
    else
    {
        THROW_UNKNOWN_PARAMETER_IO_ERROR( sType, wxString::Format( "MATERIAL %s", Name ) );
    }

    //Process electrical values
    XNODE* iNode = aNode->GetChildren();

    if( !iNode )
        THROW_MISSING_PARAMETER_IO_ERROR(
                wxT( "RESISTIVITY" ), wxString::Format( "MATERIAL %s", Name ) );

    for( ; iNode; iNode = iNode->GetNext() )
    {
        wxString nodeName = iNode->GetName();

        if( nodeName == wxT( "RELPERMIT" ) )
        {
            //TODO try.. catch + throw again with more detailed error information
            CADSTAR_COMMON::ParseChildEValue( iNode, Permittivity );
        }
        else if( nodeName == wxT( "LOSSTANGENT" ) )
        {
            //TODO try.. catch + throw again with more detailed error information
            CADSTAR_COMMON::ParseChildEValue( iNode, LossTangent );
        }
        else if( nodeName == wxT( "RESISTIVITY" ) )
        {
            //TODO try.. catch + throw again with more detailed error information
            CADSTAR_COMMON::ParseChildEValue( iNode, Resistivity );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( nodeName, wxString::Format( "MATERIAL %s", Name ) );
        }
    }
}


void CPA_LAYER::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "LAYER" ) );

    //Process Name & ID
    ID   = CADSTAR_COMMON::GetAttributeID( aNode, 0 );
    Name = CADSTAR_COMMON::GetAttributeID( aNode, 1 );

    XNODE* cNode                       = aNode->GetChildren();
    auto   processLayerMaterialDetails = [&]() {
        XNODE* tempNode = cNode->GetChildren();
        for( ; tempNode; tempNode = tempNode->GetNext() )
        {
            wxString tempNodeName = tempNode->GetName();

            if( tempNodeName == wxT( "MAKE" ) )
            {
                //Process material ID and layer width
                MaterialId = CADSTAR_COMMON::GetAttributeID( tempNode, 0 );

                Thickness = CADSTAR_COMMON::GetAttributeIDLong( tempNode, 1 );

                XNODE* childOfTempNode = tempNode->GetChildren();

                if( childOfTempNode )
                {
                    if( childOfTempNode->GetName() == wxT( "EMBEDS" ) )
                    {
                        // if( UPWARDS
                        wxString embedsValue = CADSTAR_COMMON::GetAttributeID( childOfTempNode, 0 );

                        if( embedsValue == wxT( "UPWARDS" ) )
                        {
                            Embedding = CPA_EMBEDDING::ABOVE;
                        }
                        else if( embedsValue == wxT( "DOWNWARDS" ) )
                        {
                            Embedding = CPA_EMBEDDING::BELOW;
                        }
                        else
                        {
                            THROW_UNKNOWN_PARAMETER_IO_ERROR(
                                    embedsValue, wxString::Format( "LAYER %s -> EMBEDS", Name ) );
                        }
                    }
                    else
                    {
                        THROW_UNKNOWN_NODE_IO_ERROR( childOfTempNode->GetName(),
                                wxString::Format( "LAYER %s->MAKE", Name ) );
                    }
                }
            }
            else if( tempNodeName == wxT( "BIAS" ) )
            {
                wxString bias = CADSTAR_COMMON::GetAttributeID( tempNode, 0 );

                if( bias == wxT( "X_BIASED" ) )
                {
                    RoutingBias = CPA_ROUTING_BIAS::X;
                }
                else if( bias == wxT( "Y_BIASED" ) )
                {
                    RoutingBias = CPA_ROUTING_BIAS::Y;
                }
                else if( bias == wxT( "ANTITRACK" ) )
                {
                    RoutingBias = CPA_ROUTING_BIAS::ANTI_ROUTE;
                }
                else if( bias == wxT( "OBSTACLE" ) )
                {
                    RoutingBias = CPA_ROUTING_BIAS::OBSTACLE;
                }
                else if( bias == wxT( "UNBIASED" ) )
                {
                    RoutingBias = CPA_ROUTING_BIAS::UNBIASED;
                }
                else
                {
                    THROW_UNKNOWN_PARAMETER_IO_ERROR(
                            bias, wxString::Format( "LAYER %s -> BIAS", Name ) );
                }
            }
            else
            {
                THROW_UNKNOWN_NODE_IO_ERROR( tempNodeName, wxString::Format( "LAYER %s", Name ) );
            }
        }
    };

    for( ; cNode; cNode = cNode->GetNext() )
    {
        //TODO ADD CHECK TO MAKE SURE THERE ARE NO CHILD NODES

        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "ALLDOC" ) )
        {
            Type = CPA_LAYER_TYPE::ALLDOC;
        }
        else if( cNodeName == wxT( "ALLELEC" ) )
        {
            Type = CPA_LAYER_TYPE::ALLELEC;
        }
        else if( cNodeName == wxT( "ALLLAYER" ) )
        {
            Type = CPA_LAYER_TYPE::ALLLAYER;
        }
        else if( cNodeName == wxT( "ASSCOMPCOPP" ) )
        {
            Type = CPA_LAYER_TYPE::ASSCOMPCOPP;
        }
        else if( cNodeName == wxT( "JUMPERLAYER" ) )
        {
            Type = CPA_LAYER_TYPE::JUMPERLAYER;
        }
        else if( cNodeName == wxT( "NOLAYER" ) )
        {
            Type = CPA_LAYER_TYPE::NOLAYER;
        }
        else if( cNodeName == wxT( "POWER" ) )
        {
            Type = CPA_LAYER_TYPE::POWER;

            if( !CADSTAR_COMMON::GetAttributeID( cNode, 0 ).ToLong( &PhysicalLayer ) )
                THROW_PARSING_IO_ERROR(
                        wxT( "Physical Layer" ), wxString::Format( "LAYER %s", Name ) );

            processLayerMaterialDetails();
        }
        else if( cNodeName == wxT( "DOC" ) )
        {
            Type = CPA_LAYER_TYPE::DOC;
        }
        else if( cNodeName == wxT( "CONSTRUCTION" ) )
        {
            Type = CPA_LAYER_TYPE::CONSTRUCTION;
            processLayerMaterialDetails();
        }
        else if( cNodeName == wxT( "ELEC" ) )
        {
            Type = CPA_LAYER_TYPE::ELEC;

            if( !CADSTAR_COMMON::GetAttributeID( cNode, 0 ).ToLong( &PhysicalLayer ) )
                THROW_PARSING_IO_ERROR(
                        wxT( "Physical Layer" ), wxString::Format( "LAYER %s", Name ) );

            processLayerMaterialDetails();
        }
        else if( cNodeName == wxT( "NONELEC" ) )
        {
            Type = CPA_LAYER_TYPE::NONELEC;

            if( !CADSTAR_COMMON::GetAttributeID( cNode, 0 ).ToLong( &PhysicalLayer ) )
                THROW_PARSING_IO_ERROR(
                        wxT( "Physical Layer" ), wxString::Format( "LAYER %s", Name ) );

            processLayerMaterialDetails();
        }
        else if( cNodeName == wxT( "LASUBTYP" ) )
        {
            //Process subtype
            wxString sSubType = CADSTAR_COMMON::GetAttributeID( cNode, 0 );

            if( sSubType == wxT( "LAYERSUBTYPE_ASSEMBLY" ) )
            {
                this->SubType = CPA_LAYER_SUBTYPE::LAYERSUBTYPE_ASSEMBLY;
            }
            else if( sSubType == wxT( "LAYERSUBTYPE_PASTE" ) )
            {
                this->SubType = CPA_LAYER_SUBTYPE::LAYERSUBTYPE_PASTE;
            }
            else if( sSubType == wxT( "LAYERSUBTYPE_PLACEMENT" ) )
            {
                this->SubType = CPA_LAYER_SUBTYPE::LAYERSUBTYPE_PLACEMENT;
            }
            else if( sSubType == wxT( "LAYERSUBTYPE_SILKSCREEN" ) )
            {
                this->SubType = CPA_LAYER_SUBTYPE::LAYERSUBTYPE_SILKSCREEN;
            }
            else if( sSubType == wxT( "LAYERSUBTYPE_SOLDERRESIST" ) )
            {
                this->SubType = CPA_LAYER_SUBTYPE::LAYERSUBTYPE_SOLDERRESIST;
            }
            else
            {
                THROW_UNKNOWN_PARAMETER_IO_ERROR(
                        sSubType, wxString::Format( "LAYER %s %s", Name, cNodeName ) );
            }
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, wxString::Format( "LAYER %s", Name ) );
        }
    }
}


void CPA_FORMAT::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "FORMAT" ) );

    Type    = CADSTAR_COMMON::GetAttributeID( aNode, 0 );
    SomeInt = CADSTAR_COMMON::GetAttributeIDLong( aNode, 1 );
    Version = CADSTAR_COMMON::GetAttributeIDLong( aNode, 2 );
}


void CPA_TIMESTAMP::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "TIMESTAMP" ) );

    if( !CADSTAR_COMMON::GetAttributeID( aNode, 0 ).ToLong( &Year )
            || !CADSTAR_COMMON::GetAttributeID( aNode, 1 ).ToLong( &Month )
            || !CADSTAR_COMMON::GetAttributeID( aNode, 2 ).ToLong( &Day )
            || !CADSTAR_COMMON::GetAttributeID( aNode, 3 ).ToLong( &Hour )
            || !CADSTAR_COMMON::GetAttributeID( aNode, 4 ).ToLong( &Minute )
            || !CADSTAR_COMMON::GetAttributeID( aNode, 5 ).ToLong( &Second ) )
        THROW_PARSING_IO_ERROR( wxT( "TIMESTAMP" ), wxString::Format( "HEADER" ) );
}


void CPA_HEADER::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "HEADER" ) );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString nodeName = cNode->GetName();

        if( nodeName == wxT( "FORMAT" ) )
            //TODO try.. catch + throw again with more detailed error information
            Format.Parse( cNode );
        else if( nodeName == wxT( "JOBFILE" ) )
            JobFile = CADSTAR_COMMON::GetAttributeID( cNode, 0 );
        else if( nodeName == wxT( "JOBTITLE" ) )
            JobTitle = CADSTAR_COMMON::GetAttributeID( cNode, 0 );
        else if( nodeName == wxT( "GENERATOR" ) )
            Generator = CADSTAR_COMMON::GetAttributeID( cNode, 0 );
        else if( nodeName == wxT( "RESOLUTION" ) )
        {
            XNODE* subNode = cNode->GetChildren();
            if( ( subNode->GetName() == wxT( "METRIC" ) )
                    && ( CADSTAR_COMMON::GetAttributeID( subNode, 0 ) == wxT( "HUNDREDTH" ) )
                    && ( CADSTAR_COMMON::GetAttributeID( subNode, 1 ) == wxT( "MICRON" ) ) )
            {
                Resolution = CPA_RESOLUTION::HUNDREDTH_MICRON;
            }
            else //TODO Need to find out if there are other possible resolutions. Logically
                    // there must be other base units that could be used, such as "IMPERIAL INCH"
                    // or "METRIC MM" but so far none of settings in CADSTAR generated a different
                    // output resolution to "HUNDREDTH MICRON"
                THROW_UNKNOWN_NODE_IO_ERROR( subNode->GetName(), wxT( "HEADER->RESOLUTION" ) );
        }
        else if( nodeName == wxT( "TIMESTAMP" ) )
            //TODO try.. catch + throw again with more detailed error information
            Timestamp.Parse( cNode );
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), wxT( "HEADER" ) );
    }
}


void CPA_LINECODE::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "LINECODE" ) );

    //Process Name & ID
    ID   = CADSTAR_COMMON::GetAttributeID( aNode, 0 );
    Name = CADSTAR_COMMON::GetAttributeID( aNode, 1 );

    if( !CADSTAR_COMMON::GetAttributeID( aNode, 2 ).ToLong( &Width ) )
        THROW_PARSING_IO_ERROR( wxT( "Line Width" ), wxString::Format( "LINECODE -> %s", Name ) );

    XNODE* cNode = aNode->GetChildren();

    if( cNode->GetName() != wxT( "STYLE" ) )
        THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), wxString::Format( "LINECODE -> %s", Name ) );

    wxString styleStr = CADSTAR_COMMON::GetAttributeID( cNode, 0 );

    if( styleStr == wxT( "SOLID" ) )
        Style = CPA_LINESTYLE::SOLID;
    else if( styleStr == wxT( "DASH" ) )
        Style = CPA_LINESTYLE::DASH;
    else if( styleStr == wxT( "DASHDOT" ) )
        Style = CPA_LINESTYLE::DASHDOT;
    else if( styleStr == wxT( "DASHDOTDOT" ) )
        Style = CPA_LINESTYLE::DASHDOTDOT;
    else if( styleStr == wxT( "DOT" ) )
        Style = CPA_LINESTYLE::DOT;
    else
        THROW_UNKNOWN_PARAMETER_IO_ERROR( wxString::Format( "STYLE %s", styleStr ),
                wxString::Format( "LINECODE -> %s", Name ) );
}


void CPA_HATCH::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "HATCH" ) );

    Step      = CADSTAR_COMMON::GetAttributeIDLong( aNode, 0 );
    LineWidth = CADSTAR_COMMON::GetAttributeIDLong( aNode, 2 );

    XNODE* cNode = aNode->GetChildren();

    if( !cNode || cNode->GetName() != wxT( "ORIENT" ) )
        THROW_MISSING_NODE_IO_ERROR( wxT( "ORIENT" ), wxT( "HATCH" ) );

    OrientAngle = CADSTAR_COMMON::GetAttributeIDLong( cNode, 0 );
}


void CPA_HATCHCODE::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "HATCHCODE" ) );

    //Process Name & ID
    ID   = CADSTAR_COMMON::GetAttributeID( aNode, 0 );
    Name = CADSTAR_COMMON::GetAttributeID( aNode, 1 );

    XNODE*   cNode    = aNode->GetChildren();
    wxString location = wxString::Format( "HATCHCODE -> %s", Name );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( cNode->GetName() != wxT( "HATCH" ) )
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), location );

        CPA_HATCH hatch;
        //TODO try.. catch + throw again with more detailed error information
        hatch.Parse( cNode );
        Hatches.push_back( hatch );
    }
}


void CPA_FONT::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "FONT" ) );

    Name      = CADSTAR_COMMON::GetAttributeID( aNode, 0 );
    Modifier1 = CADSTAR_COMMON::GetAttributeIDLong( aNode, 1 );
    Modifier2 = CADSTAR_COMMON::GetAttributeIDLong( aNode, 2 );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "ITALIC" ) )
            Italic = true;
        else if( cNodeName == wxT( "KERNING" ) )
            KerningPairs = true;
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
    }
}

void CPA_TEXTCODE::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "TEXTCODE" ) );

    //Process Name & ID
    ID   = CADSTAR_COMMON::GetAttributeID( aNode, 0 );
    Name = CADSTAR_COMMON::GetAttributeID( aNode, 1 );

    LineWidth = CADSTAR_COMMON::GetAttributeIDLong( aNode, 2 );
    Height    = CADSTAR_COMMON::GetAttributeIDLong( aNode, 3 );
    Width     = CADSTAR_COMMON::GetAttributeIDLong( aNode, 4 );

    XNODE* cNode = aNode->GetChildren();

    if( cNode )
    {
        if( cNode->GetName() == wxT( "FONT" ) )
            //TODO try.. catch + throw again with more detailed error information
            Font.Parse( cNode );
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), aNode->GetName() );
    }
}


void CPA_ROUTECODE::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "ROUTECODE" ) );

    //Process Name & ID
    ID   = CADSTAR_COMMON::GetAttributeID( aNode, 0 );
    Name = CADSTAR_COMMON::GetAttributeID( aNode, 1 );

    OptimalWidth = CADSTAR_COMMON::GetAttributeIDLong( aNode, 2 );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "NECKWIDTH" ) )
            NeckedWidth = CADSTAR_COMMON::GetAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "MINWIDTH" ) )
            MinWidth = CADSTAR_COMMON::GetAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "MAXWIDTH" ) )
            MaxWidth = CADSTAR_COMMON::GetAttributeIDLong( cNode, 0 );
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
    }
}


void CPA_COPREASSIGN::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "COPREASSIGN" ) );

    LayerID = CADSTAR_COMMON::GetAttributeID( aNode, 0 );

    CopperWidth = CADSTAR_COMMON::GetAttributeIDLong( aNode, 1 );
}


void CPA_COPPERCODE::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "COPPERCODE" ) );

    //Process Name & ID
    ID   = CADSTAR_COMMON::GetAttributeID( aNode, 0 );
    Name = CADSTAR_COMMON::GetAttributeID( aNode, 1 );

    CopperWidth = CADSTAR_COMMON::GetAttributeIDLong( aNode, 2 );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( cNode->GetName() == wxT( "COPREASSIGN" ) )
        {
            //TODO try.. catch + throw again with more detailed error information
            CPA_COPREASSIGN reassign;
            reassign.Parse( cNode );
            Reassigns.push_back( reassign );
        }
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), aNode->GetName() );
    }
}


void CPA_SPACINGCODE::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "SPACINGCODE" ) );

    Code = CADSTAR_COMMON::GetAttributeID( aNode, 0 );

    Spacing = CADSTAR_COMMON::GetAttributeIDLong( aNode, 1 );
}


bool CPA_PAD_SHAPE::IsShape( XNODE* aNode )
{
    wxString aNodeName = aNode->GetName();

    if( aNodeName == wxT( "ANNULUS" ) || aNodeName == wxT( "BULLET" ) || aNodeName == wxT( "ROUND" )
            || aNodeName == wxT( "DIAMOND" ) || aNodeName == wxT( "FINGER" )
            || aNodeName == wxT( "OCTAGON" ) || aNodeName == wxT( "RECTANGLE" )
            || aNodeName == wxT( "ROUNDED" ) || aNodeName == wxT( "SQUARE" ) )
        return true;
    else
        return false;
}


void CPA_PAD_SHAPE::Parse( XNODE* aNode )
{
    wxASSERT( IsShape( aNode ) );

    wxString aNodeName = aNode->GetName();

    if( aNodeName == wxT( "ANNULUS" ) )
        ShapeType = CPA_SHAPE_TYPE::ANNULUS;
    else if( aNodeName == wxT( "BULLET" ) )
        ShapeType = CPA_SHAPE_TYPE::BULLET;
    else if( aNodeName == wxT( "ROUND" ) )
        ShapeType = CPA_SHAPE_TYPE::CIRCLE;
    else if( aNodeName == wxT( "DIAMOND" ) )
        ShapeType = CPA_SHAPE_TYPE::DIAMOND;
    else if( aNodeName == wxT( "FINGER" ) )
        ShapeType = CPA_SHAPE_TYPE::FINGER;
    else if( aNodeName == wxT( "OCTAGON" ) )
        ShapeType = CPA_SHAPE_TYPE::OCTAGON;
    else if( aNodeName == wxT( "RECTANGLE" ) )
        ShapeType = CPA_SHAPE_TYPE::RECTANGLE;
    else if( aNodeName == wxT( "ROUNDED" ) )
        ShapeType = CPA_SHAPE_TYPE::ROUNDED_RECT;
    else if( aNodeName == wxT( "SQUARE" ) )
        ShapeType = CPA_SHAPE_TYPE::SQUARE;
    else
        wxASSERT( true );

    switch( ShapeType )
    {
    case CPA_SHAPE_TYPE::ANNULUS:
        Size            = CADSTAR_COMMON::GetAttributeIDLong( aNode, 0 );
        InternalFeature = CADSTAR_COMMON::GetAttributeIDLong( aNode, 1 );
        break;

    case CPA_SHAPE_TYPE::ROUNDED_RECT:
        InternalFeature = CADSTAR_COMMON::GetAttributeIDLong( aNode, 3 );
        //Fall through
    case CPA_SHAPE_TYPE::BULLET:
    case CPA_SHAPE_TYPE::FINGER:
    case CPA_SHAPE_TYPE::RECTANGLE:
        RightLength = CADSTAR_COMMON::GetAttributeIDLong( aNode, 2 );
        LeftLength  = CADSTAR_COMMON::GetAttributeIDLong( aNode, 1 );
        //Fall through
    case CPA_SHAPE_TYPE::SQUARE:

        if( aNode->GetChildren() )
        {
            if( aNode->GetChildren()->GetName() == wxT( "ORIENT" ) )
            {
                OrientAngle = CADSTAR_COMMON::GetAttributeIDLong( aNode->GetChildren(), 0 );
            }
            else
                THROW_UNKNOWN_NODE_IO_ERROR( aNode->GetChildren()->GetName(), aNode->GetName() );

            CADSTAR_COMMON::CheckNoNextNodes( aNode->GetChildren() );
        }
        //Fall through
    case CPA_SHAPE_TYPE::CIRCLE:
        Size = CADSTAR_COMMON::GetAttributeIDLong( aNode, 0 );
        break;
    }
}

void CPA_PADREASSIGN::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "PADREASSIGN" ) );

    LayerID = CADSTAR_COMMON::GetAttributeID( aNode, 0 );

    if( CPA_PAD_SHAPE::IsShape( aNode->GetChildren() ) )
        //TODO try.. catch + throw again with more detailed error information
        Shape.Parse( aNode->GetChildren() );
    else
        THROW_UNKNOWN_NODE_IO_ERROR( aNode->GetChildren()->GetName(), aNode->GetName() );

    CADSTAR_COMMON::CheckNoNextNodes( aNode->GetChildren() );
}


void CPA_PADCODE::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "PADCODE" ) );

    //Process Name & ID
    ID   = CADSTAR_COMMON::GetAttributeID( aNode, 0 );
    Name = CADSTAR_COMMON::GetAttributeID( aNode, 1 );

    XNODE*   cNode    = aNode->GetChildren();
    wxString location = wxString::Format( "PADCODE -> %s", Name );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( CPA_PAD_SHAPE::IsShape( cNode ) )
            //TODO try.. catch + throw again with more detailed error information
            Shape.Parse( cNode );
        else if( cNodeName == wxT( "CLEARANCE" ) )
            ReliefClearance = CADSTAR_COMMON::GetAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "RELIEFWIDTH" ) )
            ReliefWidth = CADSTAR_COMMON::GetAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "DRILL" ) )
        {
            DrillDiameter  = CADSTAR_COMMON::GetAttributeIDLong( cNode, 0 );
            XNODE* subNode = cNode->GetChildren();

            for( ; subNode; subNode = subNode->GetNext() )
            {
                wxString subNodeName = subNode->GetName();

                if( subNodeName == wxT( "NONPLATED" ) )
                    Plated = false;
                else if( subNodeName == wxT( "OVERSIZE" ) )
                    DrillOversize = CADSTAR_COMMON::GetAttributeIDLong( subNode, 0 );
                else
                    THROW_UNKNOWN_NODE_IO_ERROR( subNode->GetName(), location );
            }
        }
        else if( cNodeName == wxT( "DRILLLENGTH" ) )
            SlotLength = CADSTAR_COMMON::GetAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "DRILLORIENTATION" ) )
            SlotOrientation = CADSTAR_COMMON::GetAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "DRILLXOFFSET" ) )
            DrillXoffset = CADSTAR_COMMON::GetAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "DRILLYOFFSET" ) )
            DrillYoffset = CADSTAR_COMMON::GetAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "PADREASSIGN" ) )
        {
            //TODO try.. catch + throw again with more detailed error information
            CPA_PADREASSIGN reassign;
            reassign.Parse( cNode );
            Reassigns.push_back( reassign );
        }
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, location );
    }
}


void CPA_VIAREASSIGN::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "VIAREASSIGN" ) );

    LayerID = CADSTAR_COMMON::GetAttributeID( aNode, 0 );

    if( CPA_PAD_SHAPE::IsShape( aNode->GetChildren() ) )
        //TODO try.. catch + throw again with more detailed error information
        Shape.Parse( aNode->GetChildren() );
    else
        THROW_UNKNOWN_NODE_IO_ERROR( aNode->GetChildren()->GetName(), aNode->GetName() );

    CADSTAR_COMMON::CheckNoNextNodes( aNode->GetChildren() );
}


void CPA_VIACODE::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "VIACODE" ) );

    //Process Name & ID
    ID   = CADSTAR_COMMON::GetAttributeID( aNode, 0 );
    Name = CADSTAR_COMMON::GetAttributeID( aNode, 1 );

    XNODE*   cNode    = aNode->GetChildren();
    wxString location = wxString::Format( "VIACODE -> %s", Name );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( CPA_PAD_SHAPE::IsShape( cNode ) )
            //TODO try.. catch + throw again with more detailed error information
            Shape.Parse( cNode );
        else if( cNodeName == wxT( "CLEARANCE" ) )
            ReliefClearance = CADSTAR_COMMON::GetAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "RELIEFWIDTH" ) )
            ReliefWidth = CADSTAR_COMMON::GetAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "DRILL" ) )
        {
            DrillDiameter  = CADSTAR_COMMON::GetAttributeIDLong( cNode, 0 );
            XNODE* subNode = cNode->GetChildren();

            for( ; subNode; subNode = subNode->GetNext() )
            {
                wxString subNodeName = subNode->GetName();

                if( subNodeName == wxT( "OVERSIZE" ) )
                    DrillOversize = CADSTAR_COMMON::GetAttributeIDLong( subNode, 0 );
                else
                    THROW_UNKNOWN_NODE_IO_ERROR( subNode->GetName(), location );
            }
        }
        else if( cNodeName == wxT( "VIAREASSIGN" ) )
        {
            //TODO try.. catch + throw again with more detailed error information
            CPA_VIAREASSIGN reassign;
            reassign.Parse( cNode );
            Reassigns.push_back( reassign );
        }
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, location );
    }
}


void CPA_LAYERPAIR::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "LAYERPAIR" ) );

    //Process Name & ID
    ID   = CADSTAR_COMMON::GetAttributeID( aNode, 0 );
    Name = CADSTAR_COMMON::GetAttributeID( aNode, 1 );

    PhysicalLayerStart = CADSTAR_COMMON::GetAttributeIDLong( aNode, 2 );
    PhysicalLayerEnd   = CADSTAR_COMMON::GetAttributeIDLong( aNode, 3 );

    wxString location = wxString::Format( "LAYERPAIR -> %s", Name );

    if( aNode->GetChildren() )
    {
        if( aNode->GetChildren()->GetName() == wxT( "VIACODEREF" ) )
        {
            ViacodeID = CADSTAR_COMMON::GetAttributeID( aNode->GetChildren(), 0 );
        }
        else
            THROW_UNKNOWN_NODE_IO_ERROR( aNode->GetChildren()->GetName(), location );

        CADSTAR_COMMON::CheckNoNextNodes( aNode->GetChildren() );
    }
}


void CPA_ATTRNAME::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "ATTRNAME" ) );

    //Process Name & ID
    ID   = CADSTAR_COMMON::GetAttributeID( aNode, 0 );
    Name = CADSTAR_COMMON::GetAttributeID( aNode, 1 );

    XNODE*   cNode    = aNode->GetChildren();
    wxString location = wxString::Format( "ATTRNAME -> %s", Name );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "ATTROWNER" ) )
        {
            wxString attOwnerVal = CADSTAR_COMMON::GetAttributeID( cNode, 0 );

            if( attOwnerVal == wxT( "ALL_ITEMS" ) )
                AttributeOwner = CPA_ATTROWNER::ALL_ITEMS;
            else if( attOwnerVal == wxT( "AREA" ) )
                AttributeOwner = CPA_ATTROWNER::AREA;
            else if( attOwnerVal == wxT( "BOARD" ) )
                AttributeOwner = CPA_ATTROWNER::BOARD;
            else if( attOwnerVal == wxT( "COMPONENT" ) )
                AttributeOwner = CPA_ATTROWNER::COMPONENT;
            else if( attOwnerVal == wxT( "CONNECTION" ) )
                AttributeOwner = CPA_ATTROWNER::CONNECTION;
            else if( attOwnerVal == wxT( "COPPER" ) )
                AttributeOwner = CPA_ATTROWNER::COPPER;
            else if( attOwnerVal == wxT( "DOCSYMBOL" ) )
                AttributeOwner = CPA_ATTROWNER::DOCSYMBOL;
            else if( attOwnerVal == wxT( "FIGURE" ) )
                AttributeOwner = CPA_ATTROWNER::FIGURE;
            else if( attOwnerVal == wxT( "NET" ) )
                AttributeOwner = CPA_ATTROWNER::NET;
            else if( attOwnerVal == wxT( "NETCLASS" ) )
                AttributeOwner = CPA_ATTROWNER::NETCLASS;
            else if( attOwnerVal == wxT( "PART" ) )
                AttributeOwner = CPA_ATTROWNER::PART;
            else if( attOwnerVal == wxT( "PART_DEFINITION" ) )
                AttributeOwner = CPA_ATTROWNER::PART_DEFINITION;
            else if( attOwnerVal == wxT( "PIN" ) )
                AttributeOwner = CPA_ATTROWNER::PIN;
            else if( attOwnerVal == wxT( "SYMDEF" ) )
                AttributeOwner = CPA_ATTROWNER::SYMDEF;
            else if( attOwnerVal == wxT( "TEMPLATE" ) )
                AttributeOwner = CPA_ATTROWNER::TEMPLATE;
            else if( attOwnerVal == wxT( "TESTPOINT" ) )
                AttributeOwner = CPA_ATTROWNER::TESTPOINT;
            else
                THROW_UNKNOWN_PARAMETER_IO_ERROR( attOwnerVal, location );
        }
        else if( cNodeName == wxT( "ATTRUSAGE" ) )
        {
            wxString attUsageVal = CADSTAR_COMMON::GetAttributeID( cNode, 0 );

            if( attUsageVal == wxT( "BOTH" ) )
                AttributeUsage = CPA_ATTRUSAGE::BOTH;
            else if( attUsageVal == wxT( "COMPONENT" ) )
                AttributeUsage = CPA_ATTRUSAGE::COMPONENT;
            else if( attUsageVal == wxT( "PART_DEFINITION" ) )
                AttributeUsage = CPA_ATTRUSAGE::PART_DEFINITION;
            else if( attUsageVal == wxT( "PART_LIBRARY" ) )
                AttributeUsage = CPA_ATTRUSAGE::PART_LIBRARY;
            else if( attUsageVal == wxT( "SYMBOL" ) )
                AttributeUsage = CPA_ATTRUSAGE::SYMBOL;
            else
                THROW_UNKNOWN_PARAMETER_IO_ERROR( attUsageVal, location );
        }
        else if( cNodeName == wxT( "NOTRANSFER" ) )
            NoTransfer = true;
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, location );
    }
}


void CPA_ATTRIBUTE_VALUE::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "ATTR" ) );

    AttributeID = CADSTAR_COMMON::GetAttributeID( aNode, 0 );
    Value       = CADSTAR_COMMON::GetAttributeID( aNode, 1 );
}


void CPA_NETCLASS::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "NETCLASS" ) );

    //Process Name & ID
    ID   = CADSTAR_COMMON::GetAttributeID( aNode, 0 );
    Name = CADSTAR_COMMON::GetAttributeID( aNode, 1 );

    XNODE*   cNode    = aNode->GetChildren();
    wxString location = wxString::Format( "NETCLASS -> %s", Name );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "ATTR" ) )
        {
            //TODO try.. catch + throw again with more detailed error information
            CPA_ATTRIBUTE_VALUE attribute_val;
            attribute_val.Parse( cNode );
            Attributes.push_back( attribute_val );
        }
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, location );
    }
}


void CPA_SPCCLASSNAME::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "SPCCLASSNAME" ) );

    //Process Name & ID
    ID   = CADSTAR_COMMON::GetAttributeID( aNode, 0 );
    Name = CADSTAR_COMMON::GetAttributeID( aNode, 1 );
}


void CPA_SPCCLASSSPACE::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "SPCCLASSSPACE" ) );

    SpacingClassID1 = CADSTAR_COMMON::GetAttributeID( aNode, 0 );
    SpacingClassID2 = CADSTAR_COMMON::GetAttributeID( aNode, 1 );
    LayerID         = CADSTAR_COMMON::GetAttributeID( aNode, 2 );
    Spacing         = CADSTAR_COMMON::GetAttributeIDLong( aNode, 3 );
}
