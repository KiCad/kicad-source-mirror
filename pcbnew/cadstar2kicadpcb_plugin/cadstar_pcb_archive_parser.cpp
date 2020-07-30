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


void CADSTAR_PCB_ARCHIVE_PARSER::Parse()
{
    XNODE* fileRootNode = LoadArchiveFile( Filename, wxT( "CADSTARPCB" ) );

    XNODE* cNode = fileRootNode->GetChildren();

    if( !cNode )
        THROW_MISSING_NODE_IO_ERROR( wxT( "HEADER" ), wxT( "CADSTARPCB" ) );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( cNode->GetName() == wxT( "HEADER" ) )
        {
            //TODO try.. catch + throw again with more detailed error information
            Header.Parse( cNode );

            switch( Header.Resolution )
            {
            case RESOLUTION::HUNDREDTH_MICRON:
                KiCadUnitMultiplier = 10;
                break;

            default:
                wxASSERT_MSG( true, wxT( "Unknown File Resolution" ) );
                break;
            }
        }
        else if( cNode->GetName() == wxT( "ASSIGNMENTS" ) )
            Assignments.Parse( cNode );
        else if( cNode->GetName() == wxT( "LIBRARY" ) )
            Library.Parse( cNode );
        //TODO need to parse everything else!
    }


    //Todo - This is just for testing. Remove this block & delete fileRootNode.
    {
        wxXmlDocument doc;
        doc.SetRoot( fileRootNode );
        doc.Save( Filename + wxT( ".xml" ) );
    }

    //delete fileRootNode;
}


CADSTAR_PCB_ARCHIVE_PARSER::ALIGNMENT CADSTAR_PCB_ARCHIVE_PARSER::ParseAlignment( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "ALIGN" ) );

    wxString alignmentStr = GetXmlAttributeIDString( aNode, 0 );

    if( alignmentStr == wxT( "BOTTOMCENTER" ) )
        return ALIGNMENT::BOTTOMCENTER;
    else if( alignmentStr == wxT( "BOTTOMLEFT" ) )
        return ALIGNMENT::BOTTOMLEFT;
    else if( alignmentStr == wxT( "BOTTOMRIGHT" ) )
        return ALIGNMENT::BOTTOMRIGHT;
    else if( alignmentStr == wxT( "CENTERCENTER" ) )
        return ALIGNMENT::CENTERCENTER;
    else if( alignmentStr == wxT( "CENTERLEFT" ) )
        return ALIGNMENT::CENTERLEFT;
    else if( alignmentStr == wxT( "CENTERRIGHT" ) )
        return ALIGNMENT::CENTERRIGHT;
    else if( alignmentStr == wxT( "TOPCENTER" ) )
        return ALIGNMENT::TOPCENTER;
    else if( alignmentStr == wxT( "TOPLEFT" ) )
        return ALIGNMENT::TOPLEFT;
    else if( alignmentStr == wxT( "TOPRIGHT" ) )
        return ALIGNMENT::TOPRIGHT;
    else
        THROW_UNKNOWN_PARAMETER_IO_ERROR( alignmentStr, wxT( "ALIGN" ) );

    //shouldn't be here but avoids compiler warning
    return ALIGNMENT::NO_ALIGNMENT;
}


CADSTAR_PCB_ARCHIVE_PARSER::JUSTIFICATION CADSTAR_PCB_ARCHIVE_PARSER::ParseJustification(
        XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "JUSTIFICATION" ) );

    wxString justificationStr = GetXmlAttributeIDString( aNode, 0 );

    if( justificationStr == wxT( "LEFT" ) )
        return JUSTIFICATION::LEFT;
    else if( justificationStr == wxT( "RIGHT" ) )
        return JUSTIFICATION::RIGHT;
    else if( justificationStr == wxT( "CENTER" ) )
        return JUSTIFICATION::CENTER;
    else
        THROW_UNKNOWN_PARAMETER_IO_ERROR( justificationStr, wxT( "JUSTIFICATION" ) );

    return JUSTIFICATION::LEFT;
}

CADSTAR_PCB_ARCHIVE_PARSER::ANGUNITS CADSTAR_PCB_ARCHIVE_PARSER::ParseAngunits( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "ANGUNITS" ) );

    wxString angUnitStr = GetXmlAttributeIDString( aNode, 0 );

    if( angUnitStr == wxT( "DEGREES" ) )
        return ANGUNITS::DEGREES;
    else if( angUnitStr == wxT( "RADIANS" ) )
        return ANGUNITS::RADIANS;
    else
        THROW_UNKNOWN_PARAMETER_IO_ERROR( angUnitStr, aNode->GetName() );

    return ANGUNITS();
}


void CADSTAR_PCB_ARCHIVE_PARSER::ASSIGNMENTS::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "ASSIGNMENTS" ) );

    XNODE* cNode = aNode->GetChildren();

    if( !cNode )
        THROW_MISSING_NODE_IO_ERROR( wxT( "TECHNOLOGY" ), wxT( "ASSIGNMENTS" ) );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( cNode->GetName() == wxT( "LAYERDEFS" ) )
            //TODO try.. catch + throw again with more detailed error information
            Layerdefs.Parse( cNode );
        else if( cNode->GetName() == wxT( "CODEDEFS" ) )
            //TODO try.. catch + throw again with more detailed error information
            Codedefs.Parse( cNode );
        else if( cNode->GetName() == wxT( "TECHNOLOGY" ) )
            //TODO try.. catch + throw again with more detailed error information
            Technology.Parse( cNode );
        else if( cNode->GetName() == wxT( "GRIDS" ) )
            //TODO try.. catch + throw again with more detailed error information
            Grids.Parse( cNode );
        else if( cNode->GetName() == wxT( "NETCLASSEDITATTRIBSETTINGS" ) )
            NetclassEditAttributeSettings = true;
        else if( cNode->GetName() == wxT( "SPCCLASSEDITATTRIBSETTINGS" ) )
            SpacingclassEditAttributeSettings = true;
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), aNode->GetName() );
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::LAYERDEFS::Parse( XNODE* aNode )
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
                LayerStack.push_back( (LAYER_ID) xmlAttribute->GetValue() );
            }

            CheckNoChildNodes( cNode );
        }
        else if( nodeName == wxT( "MATERIAL" ) )
        {
            MATERIAL material;
            //TODO try.. catch + throw again with more detailed error information
            material.Parse( cNode );
            Materials.insert( std::make_pair( material.ID, material ) );
        }
        else if( nodeName == wxT( "LAYER" ) )
        {
            LAYER layer;
            //TODO try.. catch + throw again with more detailed error information
            layer.Parse( cNode );
            Layers.insert( std::make_pair( layer.ID, layer ) );
        }
        else if( nodeName == wxT( "SWAPPAIR" ) )
        {
            LAYER_ID layerId     = (LAYER_ID) GetXmlAttributeIDString( cNode, 0 );
            LAYER_ID swapLayerId = (LAYER_ID) GetXmlAttributeIDString( cNode, 1 );

            Layers[layerId].SwapLayerID = swapLayerId;
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( nodeName, aNode->GetName() );
        }
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::CODEDEFS::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "CODEDEFS" ) );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString nodeName = cNode->GetName();

        if( nodeName == wxT( "LINECODE" ) )
        {
            LINECODE linecode;
            //TODO try.. catch + throw again with more detailed error information
            linecode.Parse( cNode );
            LineCodes.insert( std::make_pair( linecode.ID, linecode ) );
        }
        else if( nodeName == wxT( "HATCHCODE" ) )
        {
            HATCHCODE hatchcode;
            //TODO try.. catch + throw again with more detailed error information
            hatchcode.Parse( cNode );
            HatchCodes.insert( std::make_pair( hatchcode.ID, hatchcode ) );
        }
        else if( nodeName == wxT( "TEXTCODE" ) )
        {
            TEXTCODE textcode;
            //TODO try.. catch + throw again with more detailed error information
            textcode.Parse( cNode );
            TextCodes.insert( std::make_pair( textcode.ID, textcode ) );
        }
        else if( nodeName == wxT( "ROUTECODE" ) )
        {
            ROUTECODE routecode;
            //TODO try.. catch + throw again with more detailed error information
            routecode.Parse( cNode );
            RouteCodes.insert( std::make_pair( routecode.ID, routecode ) );
        }
        else if( nodeName == wxT( "COPPERCODE" ) )
        {
            COPPERCODE coppercode;
            //TODO try.. catch + throw again with more detailed error information
            coppercode.Parse( cNode );
            CopperCodes.insert( std::make_pair( coppercode.ID, coppercode ) );
        }
        else if( nodeName == wxT( "SPACINGCODE" ) )
        {
            SPACINGCODE spacingcode;
            //TODO try.. catch + throw again with more detailed error information
            spacingcode.Parse( cNode );
            SpacingCodes.push_back( spacingcode );
        }
        else if( nodeName == wxT( "PADCODE" ) )
        {
            PADCODE padcode;
            //TODO try.. catch + throw again with more detailed error information
            padcode.Parse( cNode );
            PadCodes.insert( std::make_pair( padcode.ID, padcode ) );
        }
        else if( nodeName == wxT( "VIACODE" ) )
        {
            VIACODE viacode;
            //TODO try.. catch + throw again with more detailed error information
            viacode.Parse( cNode );
            ViaCodes.insert( std::make_pair( viacode.ID, viacode ) );
        }
        else if( nodeName == wxT( "LAYERPAIR" ) )
        {
            LAYERPAIR layerpair;
            //TODO try.. catch + throw again with more detailed error information
            layerpair.Parse( cNode );
            LayerPairs.insert( std::make_pair( layerpair.ID, layerpair ) );
        }
        else if( nodeName == wxT( "ATTRNAME" ) )
        {
            ATTRNAME attrname;
            //TODO try.. catch + throw again with more detailed error information
            attrname.Parse( cNode );
            AttributeNames.insert( std::make_pair( attrname.ID, attrname ) );
        }
        else if( nodeName == wxT( "NETCLASS" ) )
        {
            NETCLASS netclass;
            //TODO try.. catch + throw again with more detailed error information
            netclass.Parse( cNode );
            NetClasses.insert( std::make_pair( netclass.ID, netclass ) );
        }
        else if( nodeName == wxT( "SPCCLASSNAME" ) )
        {
            SPCCLASSNAME spcclassname;
            //TODO try.. catch + throw again with more detailed error information
            spcclassname.Parse( cNode );
            SpacingClassNames.insert( std::make_pair( spcclassname.ID, spcclassname ) );
        }
        else if( nodeName == wxT( "SPCCLASSSPACE" ) )
        {
            SPCCLASSSPACE spcclassspace;
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


void CADSTAR_PCB_ARCHIVE_PARSER::MATERIAL::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "MATERIAL" ) );

    //Process Name & ID
    ID   = GetXmlAttributeIDString( aNode, 0 );
    Name = GetXmlAttributeIDString( aNode, 1 );

    //Process Type
    wxString sType = GetXmlAttributeIDString( aNode, 2 );

    if( sType == wxT( "CONSTRUCTION" ) )
    {
        Type = MATERIAL_LAYER_TYPE::CONSTRUCTION;
    }
    else if( sType == wxT( "ELECTRICAL" ) )
    {
        Type = MATERIAL_LAYER_TYPE::ELECTRICAL;
    }
    else if( sType == wxT( "NONELEC" ) )
    {
        Type = MATERIAL_LAYER_TYPE::NON_ELECTRICAL;
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
            ParseChildEValue( iNode, Permittivity );
        }
        else if( nodeName == wxT( "LOSSTANGENT" ) )
        {
            //TODO try.. catch + throw again with more detailed error information
            ParseChildEValue( iNode, LossTangent );
        }
        else if( nodeName == wxT( "RESISTIVITY" ) )
        {
            //TODO try.. catch + throw again with more detailed error information
            ParseChildEValue( iNode, Resistivity );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( nodeName, wxString::Format( "MATERIAL %s", Name ) );
        }
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::LAYER::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "LAYER" ) );

    //Process Name & ID
    ID   = GetXmlAttributeIDString( aNode, 0 );
    Name = GetXmlAttributeIDString( aNode, 1 );

    XNODE* cNode                       = aNode->GetChildren();
    auto   processLayerMaterialDetails = [&]() {
        XNODE* tempNode = cNode->GetChildren();
        for( ; tempNode; tempNode = tempNode->GetNext() )
        {
            wxString tempNodeName = tempNode->GetName();

            if( tempNodeName == wxT( "MAKE" ) )
            {
                //Process material ID and layer width
                MaterialId = GetXmlAttributeIDString( tempNode, 0 );

                Thickness = GetXmlAttributeIDLong( tempNode, 1 );

                XNODE* childOfTempNode = tempNode->GetChildren();

                if( childOfTempNode )
                {
                    if( childOfTempNode->GetName() == wxT( "EMBEDS" ) )
                    {
                        wxString embedsValue = GetXmlAttributeIDString( childOfTempNode, 0 );

                        if( embedsValue == wxT( "UPWARDS" ) )
                        {
                            Embedding = EMBEDDING::ABOVE;
                        }
                        else if( embedsValue == wxT( "DOWNWARDS" ) )
                        {
                            Embedding = EMBEDDING::BELOW;
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
                wxString bias = GetXmlAttributeIDString( tempNode, 0 );

                if( bias == wxT( "X_BIASED" ) )
                {
                    RoutingBias = ROUTING_BIAS::X;
                }
                else if( bias == wxT( "Y_BIASED" ) )
                {
                    RoutingBias = ROUTING_BIAS::Y;
                }
                else if( bias == wxT( "ANTITRACK" ) )
                {
                    RoutingBias = ROUTING_BIAS::ANTI_ROUTE;
                }
                else if( bias == wxT( "OBSTACLE" ) )
                {
                    RoutingBias = ROUTING_BIAS::OBSTACLE;
                }
                else if( bias == wxT( "UNBIASED" ) )
                {
                    RoutingBias = ROUTING_BIAS::UNBIASED;
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
            Type = LAYER_TYPE::ALLDOC;
        }
        else if( cNodeName == wxT( "ALLELEC" ) )
        {
            Type = LAYER_TYPE::ALLELEC;
        }
        else if( cNodeName == wxT( "ALLLAYER" ) )
        {
            Type = LAYER_TYPE::ALLLAYER;
        }
        else if( cNodeName == wxT( "ASSCOMPCOPP" ) )
        {
            Type = LAYER_TYPE::ASSCOMPCOPP;
        }
        else if( cNodeName == wxT( "JUMPERLAYER" ) )
        {
            Type = LAYER_TYPE::JUMPERLAYER;
        }
        else if( cNodeName == wxT( "NOLAYER" ) )
        {
            Type = LAYER_TYPE::NOLAYER;
        }
        else if( cNodeName == wxT( "POWER" ) )
        {
            Type          = LAYER_TYPE::POWER;
            PhysicalLayer = GetXmlAttributeIDLong( cNode, 0 );
            processLayerMaterialDetails();
        }
        else if( cNodeName == wxT( "DOC" ) )
        {
            Type = LAYER_TYPE::DOC;
        }
        else if( cNodeName == wxT( "CONSTRUCTION" ) )
        {
            Type = LAYER_TYPE::CONSTRUCTION;
            processLayerMaterialDetails();
        }
        else if( cNodeName == wxT( "ELEC" ) )
        {
            Type          = LAYER_TYPE::ELEC;
            PhysicalLayer = GetXmlAttributeIDLong( cNode, 0 );
            processLayerMaterialDetails();
        }
        else if( cNodeName == wxT( "NONELEC" ) )
        {
            Type          = LAYER_TYPE::NONELEC;
            PhysicalLayer = GetXmlAttributeIDLong( cNode, 0 );
            processLayerMaterialDetails();
        }
        else if( cNodeName == wxT( "LASUBTYP" ) )
        {
            //Process subtype
            wxString sSubType = GetXmlAttributeIDString( cNode, 0 );

            if( sSubType == wxT( "LAYERSUBTYPE_ASSEMBLY" ) )
            {
                this->SubType = LAYER_SUBTYPE::LAYERSUBTYPE_ASSEMBLY;
            }
            else if( sSubType == wxT( "LAYERSUBTYPE_PASTE" ) )
            {
                this->SubType = LAYER_SUBTYPE::LAYERSUBTYPE_PASTE;
            }
            else if( sSubType == wxT( "LAYERSUBTYPE_PLACEMENT" ) )
            {
                this->SubType = LAYER_SUBTYPE::LAYERSUBTYPE_PLACEMENT;
            }
            else if( sSubType == wxT( "LAYERSUBTYPE_SILKSCREEN" ) )
            {
                this->SubType = LAYER_SUBTYPE::LAYERSUBTYPE_SILKSCREEN;
            }
            else if( sSubType == wxT( "LAYERSUBTYPE_SOLDERRESIST" ) )
            {
                this->SubType = LAYER_SUBTYPE::LAYERSUBTYPE_SOLDERRESIST;
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


void CADSTAR_PCB_ARCHIVE_PARSER::FORMAT::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "FORMAT" ) );

    Type    = GetXmlAttributeIDString( aNode, 0 );
    SomeInt = GetXmlAttributeIDLong( aNode, 1 );
    Version = GetXmlAttributeIDLong( aNode, 2 );
}


void CADSTAR_PCB_ARCHIVE_PARSER::TIMESTAMP::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "TIMESTAMP" ) );

    if( !GetXmlAttributeIDString( aNode, 0 ).ToLong( &Year )
            || !GetXmlAttributeIDString( aNode, 1 ).ToLong( &Month )
            || !GetXmlAttributeIDString( aNode, 2 ).ToLong( &Day )
            || !GetXmlAttributeIDString( aNode, 3 ).ToLong( &Hour )
            || !GetXmlAttributeIDString( aNode, 4 ).ToLong( &Minute )
            || !GetXmlAttributeIDString( aNode, 5 ).ToLong( &Second ) )
        THROW_PARSING_IO_ERROR( wxT( "TIMESTAMP" ), wxString::Format( "HEADER" ) );
}


void CADSTAR_PCB_ARCHIVE_PARSER::HEADER::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "HEADER" ) );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString nodeName = cNode->GetName();

        if( nodeName == wxT( "FORMAT" ) )
        { //TODO try.. catch + throw again with more detailed error information
            Format.Parse( cNode );

            if( Format.Type != wxT( "LAYOUT" ) )
                THROW_IO_ERROR( "Not a CADSTAR PCB Layout file!" );
        }
        else if( nodeName == wxT( "JOBFILE" ) )
            JobFile = GetXmlAttributeIDString( cNode, 0 );
        else if( nodeName == wxT( "JOBTITLE" ) )
            JobTitle = GetXmlAttributeIDString( cNode, 0 );
        else if( nodeName == wxT( "GENERATOR" ) )
            Generator = GetXmlAttributeIDString( cNode, 0 );
        else if( nodeName == wxT( "RESOLUTION" ) )
        {
            XNODE* subNode = cNode->GetChildren();
            if( ( subNode->GetName() == wxT( "METRIC" ) )
                    && ( GetXmlAttributeIDString( subNode, 0 ) == wxT( "HUNDREDTH" ) )
                    && ( GetXmlAttributeIDString( subNode, 1 ) == wxT( "MICRON" ) ) )
            {
                Resolution = RESOLUTION::HUNDREDTH_MICRON;
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


void CADSTAR_PCB_ARCHIVE_PARSER::LINECODE::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "LINECODE" ) );

    //Process Name & ID
    ID   = GetXmlAttributeIDString( aNode, 0 );
    Name = GetXmlAttributeIDString( aNode, 1 );

    if( !GetXmlAttributeIDString( aNode, 2 ).ToLong( &Width ) )
        THROW_PARSING_IO_ERROR( wxT( "Line Width" ), wxString::Format( "LINECODE -> %s", Name ) );

    XNODE* cNode = aNode->GetChildren();

    if( cNode->GetName() != wxT( "STYLE" ) )
        THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), wxString::Format( "LINECODE -> %s", Name ) );

    wxString styleStr = GetXmlAttributeIDString( cNode, 0 );

    if( styleStr == wxT( "SOLID" ) )
        Style = LINESTYLE::SOLID;
    else if( styleStr == wxT( "DASH" ) )
        Style = LINESTYLE::DASH;
    else if( styleStr == wxT( "DASHDOT" ) )
        Style = LINESTYLE::DASHDOT;
    else if( styleStr == wxT( "DASHDOTDOT" ) )
        Style = LINESTYLE::DASHDOTDOT;
    else if( styleStr == wxT( "DOT" ) )
        Style = LINESTYLE::DOT;
    else
        THROW_UNKNOWN_PARAMETER_IO_ERROR( wxString::Format( "STYLE %s", styleStr ),
                wxString::Format( "LINECODE -> %s", Name ) );
}


void CADSTAR_PCB_ARCHIVE_PARSER::HATCH::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "HATCH" ) );

    Step      = GetXmlAttributeIDLong( aNode, 0 );
    LineWidth = GetXmlAttributeIDLong( aNode, 2 );

    XNODE* cNode = aNode->GetChildren();

    if( !cNode || cNode->GetName() != wxT( "ORIENT" ) )
        THROW_MISSING_NODE_IO_ERROR( wxT( "ORIENT" ), wxT( "HATCH" ) );

    OrientAngle = GetXmlAttributeIDLong( cNode, 0 );
}


void CADSTAR_PCB_ARCHIVE_PARSER::HATCHCODE::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "HATCHCODE" ) );

    //Process Name & ID
    ID   = GetXmlAttributeIDString( aNode, 0 );
    Name = GetXmlAttributeIDString( aNode, 1 );

    XNODE*   cNode    = aNode->GetChildren();
    wxString location = wxString::Format( "HATCHCODE -> %s", Name );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( cNode->GetName() != wxT( "HATCH" ) )
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), location );

        HATCH hatch;
        //TODO try.. catch + throw again with more detailed error information
        hatch.Parse( cNode );
        Hatches.push_back( hatch );
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::FONT::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "FONT" ) );

    Name      = GetXmlAttributeIDString( aNode, 0 );
    Modifier1 = GetXmlAttributeIDLong( aNode, 1 );
    Modifier2 = GetXmlAttributeIDLong( aNode, 2 );

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

void CADSTAR_PCB_ARCHIVE_PARSER::TEXTCODE::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "TEXTCODE" ) );

    //Process Name & ID
    ID   = GetXmlAttributeIDString( aNode, 0 );
    Name = GetXmlAttributeIDString( aNode, 1 );

    LineWidth = GetXmlAttributeIDLong( aNode, 2 );
    Height    = GetXmlAttributeIDLong( aNode, 3 );
    Width     = GetXmlAttributeIDLong( aNode, 4 );

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


void CADSTAR_PCB_ARCHIVE_PARSER::ROUTECODE::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "ROUTECODE" ) );

    //Process Name & ID
    ID   = GetXmlAttributeIDString( aNode, 0 );
    Name = GetXmlAttributeIDString( aNode, 1 );

    OptimalWidth = GetXmlAttributeIDLong( aNode, 2 );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "NECKWIDTH" ) )
            NeckedWidth = GetXmlAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "MINWIDTH" ) )
            MinWidth = GetXmlAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "MAXWIDTH" ) )
            MaxWidth = GetXmlAttributeIDLong( cNode, 0 );
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::COPREASSIGN::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "COPREASSIGN" ) );

    LayerID = GetXmlAttributeIDString( aNode, 0 );

    CopperWidth = GetXmlAttributeIDLong( aNode, 1 );
}


void CADSTAR_PCB_ARCHIVE_PARSER::COPPERCODE::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "COPPERCODE" ) );

    //Process Name & ID
    ID   = GetXmlAttributeIDString( aNode, 0 );
    Name = GetXmlAttributeIDString( aNode, 1 );

    CopperWidth = GetXmlAttributeIDLong( aNode, 2 );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( cNode->GetName() == wxT( "COPREASSIGN" ) )
        {
            //TODO try.. catch + throw again with more detailed error information
            CADSTAR_PCB_ARCHIVE_PARSER::COPREASSIGN reassign;
            reassign.Parse( cNode );
            Reassigns.push_back( reassign );
        }
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), aNode->GetName() );
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::SPACINGCODE::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "SPACINGCODE" ) );

    Code = GetXmlAttributeIDString( aNode, 0 );

    Spacing = GetXmlAttributeIDLong( aNode, 1 );
}


bool CADSTAR_PCB_ARCHIVE_PARSER::PAD_SHAPE::IsPadShape( XNODE* aNode )
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


void CADSTAR_PCB_ARCHIVE_PARSER::PAD_SHAPE::Parse( XNODE* aNode )
{
    wxASSERT( IsPadShape( aNode ) );

    wxString aNodeName = aNode->GetName();

    if( aNodeName == wxT( "ANNULUS" ) )
        ShapeType = PAD_SHAPE_TYPE::ANNULUS;
    else if( aNodeName == wxT( "BULLET" ) )
        ShapeType = PAD_SHAPE_TYPE::BULLET;
    else if( aNodeName == wxT( "ROUND" ) )
        ShapeType = PAD_SHAPE_TYPE::CIRCLE;
    else if( aNodeName == wxT( "DIAMOND" ) )
        ShapeType = PAD_SHAPE_TYPE::DIAMOND;
    else if( aNodeName == wxT( "FINGER" ) )
        ShapeType = PAD_SHAPE_TYPE::FINGER;
    else if( aNodeName == wxT( "OCTAGON" ) )
        ShapeType = PAD_SHAPE_TYPE::OCTAGON;
    else if( aNodeName == wxT( "RECTANGLE" ) )
        ShapeType = PAD_SHAPE_TYPE::RECTANGLE;
    else if( aNodeName == wxT( "ROUNDED" ) )
        ShapeType = PAD_SHAPE_TYPE::ROUNDED_RECT;
    else if( aNodeName == wxT( "SQUARE" ) )
        ShapeType = PAD_SHAPE_TYPE::SQUARE;
    else
        wxASSERT( true );

    switch( ShapeType )
    {
    case PAD_SHAPE_TYPE::ANNULUS:
        Size            = GetXmlAttributeIDLong( aNode, 0 );
        InternalFeature = GetXmlAttributeIDLong( aNode, 1 );
        break;

    case PAD_SHAPE_TYPE::ROUNDED_RECT:
        InternalFeature = GetXmlAttributeIDLong( aNode, 3 );
        //Fall through
    case PAD_SHAPE_TYPE::BULLET:
    case PAD_SHAPE_TYPE::FINGER:
    case PAD_SHAPE_TYPE::RECTANGLE:
        RightLength = GetXmlAttributeIDLong( aNode, 2 );
        LeftLength  = GetXmlAttributeIDLong( aNode, 1 );
        //Fall through
    case PAD_SHAPE_TYPE::SQUARE:

        if( aNode->GetChildren() )
        {
            if( aNode->GetChildren()->GetName() == wxT( "ORIENT" ) )
            {
                OrientAngle = GetXmlAttributeIDLong( aNode->GetChildren(), 0 );
            }
            else
                THROW_UNKNOWN_NODE_IO_ERROR( aNode->GetChildren()->GetName(), aNode->GetName() );

            CheckNoNextNodes( aNode->GetChildren() );
        }
        //Fall through
    case PAD_SHAPE_TYPE::CIRCLE:
        Size = GetXmlAttributeIDLong( aNode, 0 );
        break;
    }
}

void CADSTAR_PCB_ARCHIVE_PARSER::PADREASSIGN::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "PADREASSIGN" ) );

    LayerID = GetXmlAttributeIDString( aNode, 0 );

    if( PAD_SHAPE::IsPadShape( aNode->GetChildren() ) )
        //TODO try.. catch + throw again with more detailed error information
        Shape.Parse( aNode->GetChildren() );
    else
        THROW_UNKNOWN_NODE_IO_ERROR( aNode->GetChildren()->GetName(), aNode->GetName() );

    CheckNoNextNodes( aNode->GetChildren() );
}


void CADSTAR_PCB_ARCHIVE_PARSER::PADCODE::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "PADCODE" ) );

    //Process Name & ID
    ID   = GetXmlAttributeIDString( aNode, 0 );
    Name = GetXmlAttributeIDString( aNode, 1 );

    XNODE*   cNode    = aNode->GetChildren();
    wxString location = wxString::Format( "PADCODE -> %s", Name );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( PAD_SHAPE::IsPadShape( cNode ) )
            //TODO try.. catch + throw again with more detailed error information
            Shape.Parse( cNode );
        else if( cNodeName == wxT( "CLEARANCE" ) )
            ReliefClearance = GetXmlAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "RELIEFWIDTH" ) )
            ReliefWidth = GetXmlAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "DRILL" ) )
        {
            DrillDiameter  = GetXmlAttributeIDLong( cNode, 0 );
            XNODE* subNode = cNode->GetChildren();

            for( ; subNode; subNode = subNode->GetNext() )
            {
                wxString subNodeName = subNode->GetName();

                if( subNodeName == wxT( "NONPLATED" ) )
                    Plated = false;
                else if( subNodeName == wxT( "OVERSIZE" ) )
                    DrillOversize = GetXmlAttributeIDLong( subNode, 0 );
                else
                    THROW_UNKNOWN_NODE_IO_ERROR( subNode->GetName(), location );
            }
        }
        else if( cNodeName == wxT( "DRILLLENGTH" ) )
            SlotLength = GetXmlAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "DRILLORIENTATION" ) )
            SlotOrientation = GetXmlAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "DRILLXOFFSET" ) )
            DrillXoffset = GetXmlAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "DRILLYOFFSET" ) )
            DrillYoffset = GetXmlAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "PADREASSIGN" ) )
        {
            //TODO try.. catch + throw again with more detailed error information
            PADREASSIGN reassign;
            reassign.Parse( cNode );
            Reassigns.insert( std::make_pair( reassign.LayerID, reassign.Shape ) );
        }
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, location );
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::VIAREASSIGN::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "VIAREASSIGN" ) );

    LayerID = GetXmlAttributeIDString( aNode, 0 );

    if( PAD_SHAPE::IsPadShape( aNode->GetChildren() ) )
        //TODO try.. catch + throw again with more detailed error information
        Shape.Parse( aNode->GetChildren() );
    else
        THROW_UNKNOWN_NODE_IO_ERROR( aNode->GetChildren()->GetName(), aNode->GetName() );

    CheckNoNextNodes( aNode->GetChildren() );
}


void CADSTAR_PCB_ARCHIVE_PARSER::VIACODE::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "VIACODE" ) );

    //Process Name & ID
    ID   = GetXmlAttributeIDString( aNode, 0 );
    Name = GetXmlAttributeIDString( aNode, 1 );

    XNODE*   cNode    = aNode->GetChildren();
    wxString location = wxString::Format( "VIACODE -> %s", Name );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( PAD_SHAPE::IsPadShape( cNode ) )
            //TODO try.. catch + throw again with more detailed error information
            Shape.Parse( cNode );
        else if( cNodeName == wxT( "CLEARANCE" ) )
            ReliefClearance = GetXmlAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "RELIEFWIDTH" ) )
            ReliefWidth = GetXmlAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "DRILL" ) )
        {
            DrillDiameter  = GetXmlAttributeIDLong( cNode, 0 );
            XNODE* subNode = cNode->GetChildren();

            for( ; subNode; subNode = subNode->GetNext() )
            {
                wxString subNodeName = subNode->GetName();

                if( subNodeName == wxT( "OVERSIZE" ) )
                    DrillOversize = GetXmlAttributeIDLong( subNode, 0 );
                else
                    THROW_UNKNOWN_NODE_IO_ERROR( subNode->GetName(), location );
            }
        }
        else if( cNodeName == wxT( "VIAREASSIGN" ) )
        {
            //TODO try.. catch + throw again with more detailed error information
            VIAREASSIGN reassign;
            reassign.Parse( cNode );
            Reassigns.insert( std::make_pair( reassign.LayerID, reassign.Shape ) );
        }
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, location );
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::LAYERPAIR::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "LAYERPAIR" ) );

    //Process Name & ID
    ID   = GetXmlAttributeIDString( aNode, 0 );
    Name = GetXmlAttributeIDString( aNode, 1 );

    PhysicalLayerStart = GetXmlAttributeIDLong( aNode, 2 );
    PhysicalLayerEnd   = GetXmlAttributeIDLong( aNode, 3 );

    wxString location = wxString::Format( "LAYERPAIR -> %s", Name );

    if( aNode->GetChildren() )
    {
        if( aNode->GetChildren()->GetName() == wxT( "VIACODEREF" ) )
        {
            ViacodeID = GetXmlAttributeIDString( aNode->GetChildren(), 0 );
        }
        else
            THROW_UNKNOWN_NODE_IO_ERROR( aNode->GetChildren()->GetName(), location );

        CheckNoNextNodes( aNode->GetChildren() );
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::ATTRNAME::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "ATTRNAME" ) );

    //Process Name & ID
    ID   = GetXmlAttributeIDString( aNode, 0 );
    Name = GetXmlAttributeIDString( aNode, 1 );

    XNODE*   cNode    = aNode->GetChildren();
    wxString location = wxString::Format( "ATTRNAME -> %s", Name );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "ATTROWNER" ) )
        {
            wxString attOwnerVal = GetXmlAttributeIDString( cNode, 0 );

            if( attOwnerVal == wxT( "ALL_ITEMS" ) )
                AttributeOwner = ATTROWNER::ALL_ITEMS;
            else if( attOwnerVal == wxT( "AREA" ) )
                AttributeOwner = ATTROWNER::AREA;
            else if( attOwnerVal == wxT( "BOARD" ) )
                AttributeOwner = ATTROWNER::BOARD;
            else if( attOwnerVal == wxT( "COMPONENT" ) )
                AttributeOwner = ATTROWNER::COMPONENT;
            else if( attOwnerVal == wxT( "CONNECTION" ) )
                AttributeOwner = ATTROWNER::CONNECTION;
            else if( attOwnerVal == wxT( "COPPER" ) )
                AttributeOwner = ATTROWNER::COPPER;
            else if( attOwnerVal == wxT( "DOCSYMBOL" ) )
                AttributeOwner = ATTROWNER::DOCSYMBOL;
            else if( attOwnerVal == wxT( "FIGURE" ) )
                AttributeOwner = ATTROWNER::FIGURE;
            else if( attOwnerVal == wxT( "NET" ) )
                AttributeOwner = ATTROWNER::NET;
            else if( attOwnerVal == wxT( "NETCLASS" ) )
                AttributeOwner = ATTROWNER::NETCLASS;
            else if( attOwnerVal == wxT( "PART" ) )
                AttributeOwner = ATTROWNER::PART;
            else if( attOwnerVal == wxT( "PART_DEFINITION" ) )
                AttributeOwner = ATTROWNER::PART_DEFINITION;
            else if( attOwnerVal == wxT( "PIN" ) )
                AttributeOwner = ATTROWNER::PIN;
            else if( attOwnerVal == wxT( "SYMDEF" ) )
                AttributeOwner = ATTROWNER::SYMDEF;
            else if( attOwnerVal == wxT( "TEMPLATE" ) )
                AttributeOwner = ATTROWNER::TEMPLATE;
            else if( attOwnerVal == wxT( "TESTPOINT" ) )
                AttributeOwner = ATTROWNER::TESTPOINT;
            else
                THROW_UNKNOWN_PARAMETER_IO_ERROR( attOwnerVal, location );
        }
        else if( cNodeName == wxT( "ATTRUSAGE" ) )
        {
            wxString attUsageVal = GetXmlAttributeIDString( cNode, 0 );

            if( attUsageVal == wxT( "BOTH" ) )
                AttributeUsage = ATTRUSAGE::BOTH;
            else if( attUsageVal == wxT( "COMPONENT" ) )
                AttributeUsage = ATTRUSAGE::COMPONENT;
            else if( attUsageVal == wxT( "PART_DEFINITION" ) )
                AttributeUsage = ATTRUSAGE::PART_DEFINITION;
            else if( attUsageVal == wxT( "PART_LIBRARY" ) )
                AttributeUsage = ATTRUSAGE::PART_LIBRARY;
            else if( attUsageVal == wxT( "SYMBOL" ) )
                AttributeUsage = ATTRUSAGE::SYMBOL;
            else
                THROW_UNKNOWN_PARAMETER_IO_ERROR( attUsageVal, location );
        }
        else if( cNodeName == wxT( "NOTRANSFER" ) )
            NoTransfer = true;
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, location );
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::ATTRIBUTE_VALUE::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "ATTR" ) );

    AttributeID = GetXmlAttributeIDString( aNode, 0 );
    Value       = GetXmlAttributeIDString( aNode, 1 );
}


void CADSTAR_PCB_ARCHIVE_PARSER::NETCLASS::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "NETCLASS" ) );

    //Process Name & ID
    ID   = GetXmlAttributeIDString( aNode, 0 );
    Name = GetXmlAttributeIDString( aNode, 1 );

    XNODE*   cNode    = aNode->GetChildren();
    wxString location = wxString::Format( "NETCLASS -> %s", Name );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "ATTR" ) )
        {
            ATTRIBUTE_VALUE attribute_val;
            //TODO try.. catch + throw again with more detailed error information
            attribute_val.Parse( cNode );
            Attributes.push_back( attribute_val );
        }
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, location );
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::SPCCLASSNAME::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "SPCCLASSNAME" ) );

    //Process Name & ID
    ID   = GetXmlAttributeIDString( aNode, 0 );
    Name = GetXmlAttributeIDString( aNode, 1 );
}


void CADSTAR_PCB_ARCHIVE_PARSER::SPCCLASSSPACE::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "SPCCLASSSPACE" ) );

    SpacingClassID1 = GetXmlAttributeIDString( aNode, 0 );
    SpacingClassID2 = GetXmlAttributeIDString( aNode, 1 );
    LayerID         = GetXmlAttributeIDString( aNode, 2 );
    Spacing         = GetXmlAttributeIDLong( aNode, 3 );
}


CADSTAR_PCB_ARCHIVE_PARSER::UNITS CADSTAR_PCB_ARCHIVE_PARSER::ParseUnits( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "UNITS" ) );

    wxString unit = GetXmlAttributeIDString( aNode, 0 );

    if( unit == wxT( "CENTIMETER" ) )
        return UNITS::CENTIMETER;
    else if( unit == wxT( "INCH" ) )
        return UNITS::INCH;
    else if( unit == wxT( "METER" ) )
        return UNITS::METER;
    else if( unit == wxT( "MICROMETRE" ) )
        return UNITS::MICROMETRE;
    else if( unit == wxT( "MM" ) )
        return UNITS::MM;
    else if( unit == wxT( "THOU" ) )
        return UNITS::THOU;
    else
        THROW_UNKNOWN_PARAMETER_IO_ERROR( unit, wxT( "UNITS" ) );

    return UNITS();
}


void CADSTAR_PCB_ARCHIVE_PARSER::TECHNOLOGY_SECTION::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "TECHNOLOGY" ) );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "UNITS" ) )
            Units = ParseUnits( cNode );
        else if( cNodeName == wxT( "UNITSPRECISION" ) )
            UnitDisplPrecision = GetXmlAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "INTERLINEGAP" ) )
            InterlineGap = GetXmlAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "BARLINEGAP" ) )
            BarlineGap = GetXmlAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "ALLOWBARTEXT" ) )
            AllowBarredText = true;
        else if( cNodeName == wxT( "ANGULARPRECISION" ) )
            AngularPrecision = GetXmlAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "MINROUTEWIDTH" ) )
            MinRouteWidth = GetXmlAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "MINNECKED" ) )
            MinNeckedLength = GetXmlAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "MINUNNECKED" ) )
            MinUnneckedLength = GetXmlAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "MINMITER" ) )
            MinMitre = GetXmlAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "MAXMITER" ) )
            MaxMitre = GetXmlAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "MAXPHYSLAYER" ) )
            MaxPhysicalLayer = GetXmlAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "TRACKGRID" ) )
            TrackGrid = GetXmlAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "VIAGRID" ) )
            ViaGrid = GetXmlAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "DESIGNORIGIN" ) )
        {
            std::vector<POINT> pts = ParseAllChildPoints( cNode, true, 1 );
            DesignOrigin           = pts[0];
        }
        else if( cNodeName == wxT( "DESIGNAREA" ) )
        {
            std::vector<POINT> pts = ParseAllChildPoints( cNode, true, 2 );
            DesignArea             = std::make_pair( pts[0], pts[1] );
        }
        else if( cNodeName == wxT( "DESIGNREF" ) )
        {
            std::vector<POINT> pts = ParseAllChildPoints( cNode, true, 1 );
            DesignRef              = pts[0];
        }
        else if( cNodeName == wxT( "DESIGNLIMIT" ) )
        {
            std::vector<POINT> pts = ParseAllChildPoints( cNode, true, 1 );
            DesignLimit            = pts[0];
        }
        else if( cNodeName == wxT( "BACKOFFJCTS" ) )
            BackOffJunctions = true;
        else if( cNodeName == wxT( "BCKOFFWIDCHANGE" ) )
            BackOffWidthChange = true;
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, wxT( "TECHNOLOGY" ) );
    }
}


bool CADSTAR_PCB_ARCHIVE_PARSER::GRID::IsGrid( XNODE* aNode )
{
    wxString aNodeName = aNode->GetName();

    if( aNodeName == wxT( "FRACTIONALGRID" ) || aNodeName == wxT( "STEPGRID" ) )
        return true;
    else
        return false;
}


void CADSTAR_PCB_ARCHIVE_PARSER::GRID::Parse( XNODE* aNode )
{
    wxASSERT( IsGrid( aNode ) );

    wxString aNodeName = aNode->GetName();

    if( aNodeName == wxT( "FRACTIONALGRID" ) )
        Type = GRID_TYPE::FRACTIONALGRID;
    else if( aNodeName == wxT( "STEPGRID" ) )
        Type = GRID_TYPE::STEPGRID;
    else
        wxASSERT_MSG( true, wxT( "Unknown Grid Type" ) );

    Name   = GetXmlAttributeIDString( aNode, 0 );
    Param1 = GetXmlAttributeIDLong( aNode, 1 );
    Param2 = GetXmlAttributeIDLong( aNode, 2 );
}


void CADSTAR_PCB_ARCHIVE_PARSER::GRIDS::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "GRIDS" ) );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "WORKINGGRID" ) )
        {
            XNODE* workingGridNode = cNode->GetChildren();

            if( !GRID::IsGrid( workingGridNode ) )
                THROW_UNKNOWN_NODE_IO_ERROR(
                        workingGridNode->GetName(), wxT( "GRIDS -> WORKINGGRID" ) );
            else
                WorkingGrid.Parse( workingGridNode );
        }
        else if( cNodeName == wxT( "SCREENGRID" ) )
        {
            XNODE* screenGridNode = cNode->GetChildren();

            if( !GRID::IsGrid( screenGridNode ) )
                THROW_UNKNOWN_NODE_IO_ERROR(
                        screenGridNode->GetName(), wxT( "GRIDS -> SCREENGRID" ) );
            else
                ScreenGrid.Parse( screenGridNode );
        }
        else if( GRID::IsGrid( cNode ) )
        {
            GRID userGrid;
            userGrid.Parse( cNode );
            UserGrids.push_back( userGrid );
        }
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::FIGURE::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "FIGURE" ) );

    ID         = GetXmlAttributeIDString( aNode, 0 );
    LineCodeID = GetXmlAttributeIDString( aNode, 1 );
    LayerID    = GetXmlAttributeIDString( aNode, 2 );

    XNODE*   cNode              = aNode->GetChildren();
    bool     shapeIsInitialised = false; //< Stop more than one Shape Object
    wxString location           = wxString::Format( "Figure %s", ID );

    if( !cNode )
        THROW_MISSING_NODE_IO_ERROR( wxT( "Shape" ), location );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( !shapeIsInitialised && Shape.IsShape( cNode ) )
        {
            Shape.Parse( cNode );
            shapeIsInitialised = true;
        }
        else if( cNodeName == wxT( "SWAPRULE" ) )
        {
            SwapRule = ParseSwapRule( cNode );
        }
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, location );
    }
}


CADSTAR_PCB_ARCHIVE_PARSER::SWAP_RULE CADSTAR_PCB_ARCHIVE_PARSER::ParseSwapRule( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "SWAPRULE" ) );

    SWAP_RULE retval;
    wxString  swapRuleStr = GetXmlAttributeIDString( aNode, 0 );

    if( swapRuleStr == wxT( "NO_SWAP" ) )
        retval = SWAP_RULE::NO_SWAP;
    else if( swapRuleStr == wxT( "USE_SWAP_LAYER" ) )
        retval = SWAP_RULE::USE_SWAP_LAYER;
    else
        THROW_UNKNOWN_PARAMETER_IO_ERROR( swapRuleStr, wxT( "SWAPRULE" ) );

    return retval;
}


void CADSTAR_PCB_ARCHIVE_PARSER::COMPONENT_COPPER::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "COMPCOPPER" ) );

    CopperCodeID = GetXmlAttributeIDString( aNode, 0 );
    LayerID      = GetXmlAttributeIDString( aNode, 1 );

    XNODE*   cNode              = aNode->GetChildren();
    bool     shapeIsInitialised = false; //< Stop more than one Shape Object
    wxString location           = wxT( "COMPCOPPER" );

    if( !cNode )
        THROW_MISSING_NODE_IO_ERROR( wxT( "Shape" ), location );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( !shapeIsInitialised && Shape.IsShape( cNode ) )
        {
            Shape.Parse( cNode );
            shapeIsInitialised = true;
        }
        else if( cNodeName == wxT( "SWAPRULE" ) )
        {
            SwapRule = ParseSwapRule( cNode );
        }
        else if( cNodeName == wxT( "ASSOCPIN" ) )
        {
            wxXmlAttribute* xmlAttribute = cNode->GetAttributes();

            for( ; xmlAttribute; xmlAttribute = xmlAttribute->GetNext() )
            {
                long padId;

                if( !xmlAttribute->GetValue().ToLong( &padId ) )
                    THROW_PARSING_IO_ERROR( wxT( "ASSOCPIN" ), location );

                AssociatedPadIDs.push_back( (PAD_ID) padId );
            }

            CheckNoChildNodes( cNode );
        }
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, location );
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::COMPONENT_AREA::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "COMPAREA" ) );

    ID         = GetXmlAttributeIDString( aNode, 0 );
    LineCodeID = GetXmlAttributeIDString( aNode, 1 );
    LayerID    = GetXmlAttributeIDString( aNode, 3 );

    XNODE*   cNode              = aNode->GetChildren();
    bool     shapeIsInitialised = false; //< Stop more than one Shape Object
    wxString location           = wxString::Format( "COMPAREA %s", ID );

    if( !cNode )
        THROW_MISSING_NODE_IO_ERROR( wxT( "Shape" ), location );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( !shapeIsInitialised && SHAPE::IsShape( cNode ) )
        {
            Shape.Parse( cNode );
            shapeIsInitialised = true;
        }
        else if( cNodeName == wxT( "SWAPRULE" ) )
        {
            SwapRule = ParseSwapRule( cNode );
        }
        else if( cNodeName == wxT( "USAGE" ) )
        {
            wxXmlAttribute* xmlAttribute = cNode->GetAttributes();

            for( ; xmlAttribute; xmlAttribute = xmlAttribute->GetNext() )
            {
                if( xmlAttribute->GetValue() == wxT( "NO_TRACKS" ) )
                    NoTracks = true;
                else if( xmlAttribute->GetValue() == wxT( "NO_VIAS" ) )
                    NoVias = true;
                else
                    THROW_UNKNOWN_PARAMETER_IO_ERROR( xmlAttribute->GetValue(), location );
            }

            CheckNoChildNodes( cNode );
        }
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, location );
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::PAD_EXITS::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "EXITS" ) );

    wxXmlAttribute* xmlAttribute = aNode->GetAttributes();

    for( ; xmlAttribute; xmlAttribute = xmlAttribute->GetNext() )
    {
        if( xmlAttribute->GetValue() == wxT( "FREE" ) )
            FreeAngle = true;
        else if( xmlAttribute->GetValue() == wxT( "N" ) )
            North = true;
        else if( xmlAttribute->GetValue() == wxT( "S" ) )
            South = true;
        else if( xmlAttribute->GetValue() == wxT( "E" ) )
            East = true;
        else if( xmlAttribute->GetValue() == wxT( "W" ) )
            West = true;
        else if( xmlAttribute->GetValue() == wxT( "NE" ) )
            NorthEast = true;
        else if( xmlAttribute->GetValue() == wxT( "NW" ) )
            NorthWest = true;
        else if( xmlAttribute->GetValue() == wxT( "SE" ) )
            SouthEast = true;
        else if( xmlAttribute->GetValue() == wxT( "SW" ) )
            SouthWest = true;
        else
            THROW_UNKNOWN_PARAMETER_IO_ERROR( xmlAttribute->GetValue(), wxT( "EXITS" ) );
    }

    CheckNoChildNodes( aNode );
}


void CADSTAR_PCB_ARCHIVE_PARSER::PAD::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "PAD" ) );

    ID        = GetXmlAttributeIDLong( aNode, 0 );
    PadCodeID = GetXmlAttributeIDString( aNode, 2 );

    wxString padSideStr = GetXmlAttributeIDString( aNode, 3 );

    if( padSideStr == wxT( "THRU" ) )
        Side = PAD_SIDE::THROUGH_HOLE;
    else if( padSideStr == wxT( "BOTTOM" ) )
        Side = PAD_SIDE::MAXIMUM;
    else if( padSideStr == wxT( "TOP" ) )
        Side = PAD_SIDE::MINIMUM;

    XNODE*   cNode    = aNode->GetChildren();
    wxString location = wxString::Format( "PAD %d", ID );

    if( !cNode )
        THROW_MISSING_NODE_IO_ERROR( wxT( "PADIDENTIFIER" ), location );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "ORIENT" ) )
            OrientAngle = GetXmlAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "FIRSTPAD" ) )
            FirstPad = true;
        else if( cNodeName == wxT( "EXITS" ) )
            Exits.Parse( cNode );
        else if( cNodeName == wxT( "PADIDENTIFIER" ) )
            Identifier = GetXmlAttributeIDString( cNode, 0 );
        else if( cNodeName == wxT( "PCBONLYPAD" ) )
            PCBonlyPad = true;
        else if( cNodeName == wxT( "PT" ) )
            Position.Parse( cNode );
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, location );
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::TEXT_LOCATION::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "TEXTLOC" ) );

    wxString attributeStr     = GetXmlAttributeIDString( aNode, 0 );
    bool     attributeIDisSet = false;

    if( attributeStr == wxT( "PART_NAME" ) )
    {
        AttributeID      = PART_NAME_ATTRID;
        attributeIDisSet = true;
    }
    else if( attributeStr == wxT( "COMP_NAME" ) )
    {
        AttributeID      = COMPONENT_NAME_ATTRID;
        attributeIDisSet = true;
    }
    else if( attributeStr == wxT( "COMP_NAME2" ) )
    {
        AttributeID      = COMPONENT_NAME_2_ATTRID;
        attributeIDisSet = true;
    }
    else if( attributeStr == wxT( "ATTRREF" ) )
    {
        //We will initialise when we parse all child nodes
        attributeIDisSet = false;
    }
    else
    {
        THROW_UNKNOWN_PARAMETER_IO_ERROR( attributeStr, wxT( "TEXTLOC" ) );
    }

    TextCodeID = GetXmlAttributeIDString( aNode, 1 );
    LayerID    = GetXmlAttributeIDString( aNode, 2 );

    //Parse child nodes
    XNODE* cNode = aNode->GetChildren();

    if( !cNode )
        THROW_MISSING_NODE_IO_ERROR( wxT( "PT" ), wxT( "TEXTLOC" ) );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "PT" ) )
            Position.Parse( cNode );
        else if( !attributeIDisSet && cNodeName == wxT( "ATTRREF" ) )
        {
            AttributeID      = GetXmlAttributeIDString( cNode, 0 );
            attributeIDisSet = true;
        }
        else if( cNodeName == wxT( "ORIENT" ) )
            OrientAngle = GetXmlAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "MIRROR" ) )
            Mirror = true;
        else if( cNodeName == wxT( "ALIGN" ) )
            Alignment = ParseAlignment( cNode );
        else if( cNodeName == wxT( "JUSTIFICATION" ) )
            Justification = ParseJustification( cNode );
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, wxT( "TEXTLOC" ) );
    }
}

void CADSTAR_PCB_ARCHIVE_PARSER::TEXT::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "TEXT" ) );

    ID = GetXmlAttributeIDString( aNode, 0 );
    //TODO: Need to lex/parse "Text" to identify design fields (e.g "<@DESIGN_TITLE@>") and
    // hyperlinks (e.g. "<@HYPERLINK\"[link]\"[link text]@>")
    Text       = GetXmlAttributeIDString( aNode, 1 );
    TextCodeID = GetXmlAttributeIDString( aNode, 2 );
    LayerID    = GetXmlAttributeIDString( aNode, 3 );

    //Parse child nodes
    XNODE* cNode = aNode->GetChildren();

    if( !cNode )
        THROW_MISSING_NODE_IO_ERROR( wxT( "PT" ), wxT( "TEXT" ) );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "PT" ) )
            Position.Parse( cNode );
        else if( cNodeName == wxT( "ORIENT" ) )
            OrientAngle = GetXmlAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "MIRROR" ) )
            Mirror = true;
        else if( cNodeName == wxT( "SWAPRULE" ) )
            SwapRule = ParseSwapRule( cNode );
        else if( cNodeName == wxT( "ALIGN" ) )
            Alignment = ParseAlignment( cNode );
        else if( cNodeName == wxT( "JUSTIFICATION" ) )
            Justification = ParseJustification( cNode );
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, wxT( "TEXT" ) );
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::DIMENSION::ARROW::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "DIMARROW" ) );
    bool arrowStyleInitialised = false;
    bool upperAngleInitialised = false;
    bool lowerAngleInitialised = false;

    ArrowLength = GetXmlAttributeIDLong( aNode, 3 );

    XNODE* cNode = aNode->GetChildren();


    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "ARROWSTYLE" ) )
        {
            wxString arrowStyleStr = GetXmlAttributeIDString( cNode, 0 );
            arrowStyleInitialised  = true;

            if( arrowStyleStr == wxT( "DIMENSION_ARROWOPEN" ) )
                ArrowStyle = STYLE::OPEN;
            else if( arrowStyleStr == wxT( "DIMENSION_ARROWCLOSED" ) )
                ArrowStyle = STYLE::CLOSED;
            else if( arrowStyleStr == wxT( "DIMENSION_ARROWCLEAR" ) )
                ArrowStyle = STYLE::CLEAR;
            else if( arrowStyleStr == wxT( "DIMENSION_ARROWCLOSEDFILLED" ) )
                ArrowStyle = STYLE::CLOSED_FILLED;
            else
                THROW_UNKNOWN_PARAMETER_IO_ERROR( arrowStyleStr, cNodeName );
        }
        else if( cNodeName == wxT( "ARROWANGLEA" ) )
        {
            UpperAngle            = GetXmlAttributeIDLong( cNode, 0 );
            upperAngleInitialised = true;
        }
        else if( cNodeName == wxT( "ARROWANGLEB" ) )
        {
            UpperAngle            = GetXmlAttributeIDLong( cNode, 0 );
            lowerAngleInitialised = true;
        }
        else
        {
            THROW_UNKNOWN_PARAMETER_IO_ERROR( cNodeName, wxT( "DIMARROW" ) );
        }
    }

    if( !arrowStyleInitialised )
        THROW_MISSING_PARAMETER_IO_ERROR( wxT( "ARROWSTYLE" ), wxT( "DIMARROW" ) );

    if( !upperAngleInitialised )
        THROW_MISSING_PARAMETER_IO_ERROR( wxT( "ARROWANGLEA" ), wxT( "DIMARROW" ) );

    if( !lowerAngleInitialised )
        THROW_MISSING_PARAMETER_IO_ERROR( wxT( "ARROWANGLEB" ), wxT( "DIMARROW" ) );
}


void CADSTAR_PCB_ARCHIVE_PARSER::DIMENSION::TEXTFORMAT::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "DIMTEXT" ) );

    TextGap    = GetXmlAttributeIDLong( aNode, 1 );
    TextOffset = GetXmlAttributeIDLong( aNode, 2 );

    XNODE* cNode = aNode->GetChildren();

    if( cNode->GetName() != wxT( "TXTSTYLE" ) )
        THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), wxT( "DIMTEXT" ) );

    wxString styleStr = GetXmlAttributeIDString( cNode, 0 );

    if( styleStr == wxT( "DIMENSION_INTERNAL" ) )
        Style = STYLE::INSIDE;
    else if( styleStr == wxT( "DIMENSION_EXTERNAL" ) )
        Style = STYLE::OUTSIDE;
    else
        THROW_UNKNOWN_PARAMETER_IO_ERROR( styleStr, wxT( "TXTSTYLE" ) );

    CheckNoNextNodes( cNode );
}


void CADSTAR_PCB_ARCHIVE_PARSER::DIMENSION::EXTENSION_LINE::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "EXTLINE" ) );

    LineCodeID = GetXmlAttributeIDString( aNode, 0 );
    Overshoot  = GetXmlAttributeIDLong( aNode, 3 );
    Offset     = GetXmlAttributeIDLong( aNode, 4 );

    XNODE* cNode      = aNode->GetChildren();
    int    noOfPoints = 0;

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( noOfPoints < 2 && cNodeName == wxT( "PT" ) )
        {
            ++noOfPoints;

            if( noOfPoints == 1 )
                Start.Parse( cNode );
            else
                End.Parse( cNode );
        }
        else if( cNodeName == wxT( "SUPPRESSFIRST" ) )
            SuppressFirst = true;
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, wxT( "EXTLINE" ) );
    }

    if( noOfPoints != 2 )
        THROW_MISSING_PARAMETER_IO_ERROR( wxT( "PT" ), wxT( "EXTLINE" ) );
}


bool CADSTAR_PCB_ARCHIVE_PARSER::DIMENSION::LINE::IsLine( XNODE* aNode )
{
    if( aNode->GetName() == wxT( "LEADERLINE" ) || aNode->GetName() == wxT( "LINEARLINE" )
            || aNode->GetName() == wxT( "ANGULARLINE" ) )
        return true;
    else
        return false;
}


void CADSTAR_PCB_ARCHIVE_PARSER::DIMENSION::LINE::Parse( XNODE* aNode )
{
    wxASSERT( IsLine( aNode ) );

    if( aNode->GetName() == wxT( "LINEARLINE" ) )
        Type = TYPE::LINEARLINE;
    else if( aNode->GetName() == wxT( "LEADERLINE" ) )
        Type = TYPE::LEADERLINE;
    else if( aNode->GetName() == wxT( "ANGULARLINE" ) )
        Type = TYPE::ANGULARLINE;
    else
        wxASSERT_MSG( true, "Not a valid type. What happened to the node Name?" );

    LineCodeID = GetXmlAttributeIDString( aNode, 0 );

    if( Type == TYPE::LEADERLINE )
    {
        LeaderLineLength          = GetXmlAttributeIDLong( aNode, 5 );
        LeaderLineExtensionLength = GetXmlAttributeIDLong( aNode, 6 );
    }

    XNODE* cNode              = aNode->GetChildren();
    int    noOfPoints         = 0;
    int    requiredNoOfPoints = 2;

    if( Type == TYPE::ANGULARLINE )
        requiredNoOfPoints = 3;

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "DIMLINETYPE" ) )
        {
            wxString styleStr = GetXmlAttributeIDString( cNode, 0 );

            if( styleStr == wxT( "DIMENSION_INTERNAL" ) )
                Style = STYLE::INTERNAL;
            else if( styleStr == wxT( "DIMENSION_EXTERNAL" ) )
                Style = STYLE::EXTERNAL;
            else
                THROW_UNKNOWN_PARAMETER_IO_ERROR( styleStr, cNodeName );
        }
        else if( noOfPoints < requiredNoOfPoints && cNodeName == wxT( "PT" ) )
        {
            ++noOfPoints;

            if( noOfPoints == 1 )
                Start.Parse( cNode );
            else if( noOfPoints == 2 )
                End.Parse( cNode );
            else
                Centre.Parse( cNode );
        }
        else if( Type == TYPE::LEADERLINE && cNodeName == wxT( "LEADERANG" ) )
        {
            LeaderAngle = GetXmlAttributeIDLong( cNode, 0 );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }

    if( noOfPoints != requiredNoOfPoints )
        THROW_MISSING_PARAMETER_IO_ERROR( wxT( "PT" ), aNode->GetName() );
}


bool CADSTAR_PCB_ARCHIVE_PARSER::DIMENSION::IsDimension( XNODE* aNode )
{
    if( aNode->GetName() == wxT( "LINEARDIM" ) || aNode->GetName() == wxT( "LEADERDIM" )
            || aNode->GetName() == wxT( "ANGLEDIM" ) )
        return true;
    else
        return false;
}

void CADSTAR_PCB_ARCHIVE_PARSER::DIMENSION::Parse( XNODE* aNode )
{
    wxASSERT( IsDimension( aNode ) );

    std::map<wxString, TYPE> typeMap = {
        { wxT( "LINEARDIM" ), TYPE::LINEARDIM }, /////////////////////////////////////////////
        { wxT( "LEADERDIM" ), TYPE::LEADERDIM }, /////////////////////////////////////////////
        { wxT( "ANGLEDIM" ), TYPE::ANGLEDIM }
    };

    //make sure aNode is valid TYPE
    wxASSERT_MSG( typeMap.find( aNode->GetName() ) != typeMap.end(),
            "Not a valid type. What happened to the node Name?" );

    Type                = typeMap[aNode->GetName()];
    LayerID             = GetXmlAttributeIDString( aNode, 1 );
    wxString subTypeStr = GetXmlAttributeIDString( aNode, 2 );

    std::map<wxString, SUBTYPE> subTypeMap = {
        { wxT( "DIMENSION_ORTHOGONAL" ), SUBTYPE::ORTHOGONAL }, //////////////////////////////
        { wxT( "DIMENSION_DIRECT" ), SUBTYPE::DIRECT },         //////////////////////////////
        { wxT( "DIMENSION_ANGLED" ), SUBTYPE::ANGLED },         //////////////////////////////
        { wxT( "DIMENSION_DIAMETER" ), SUBTYPE::DIAMETER },     //////////////////////////////
        { wxT( "DIMENSION_RADIUS" ), SUBTYPE::RADIUS },         //////////////////////////////
        { wxT( "DIMENSION_ANGULAR" ), SUBTYPE::ANGULAR }
    };

    if( subTypeMap.find( subTypeStr ) == subTypeMap.end() )
        THROW_UNKNOWN_PARAMETER_IO_ERROR( subTypeStr, aNode->GetName() );

    Subtype   = subTypeMap[subTypeStr];
    Precision = GetXmlAttributeIDLong( aNode, 3 );

    XNODE* cNode = aNode->GetChildren();

    bool idParsed         = false;
    bool unitsParsed      = false; //UNITS or ANGUNITS
    bool arrowParsed      = false;
    bool textFormatParsed = false;
    bool extLineParsed    = false;
    bool lineParsed       = false;
    bool textParsed       = false;

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( !idParsed && cNodeName == wxT( "DIMREF" ) )
        {
            ID       = GetXmlAttributeIDString( cNode, 0 );
            idParsed = true;
        }
        else if( !unitsParsed && cNodeName == wxT( "UNITS" ) )
        {
            LinearUnits = ParseUnits( cNode );
            unitsParsed = true;
        }
        else if( !unitsParsed && cNodeName == wxT( "ANGUNITS" ) )
        {
            AngularUnits = ParseAngunits( cNode );
            unitsParsed  = true;
        }
        else if( !arrowParsed && cNodeName == wxT( "DIMARROW" ) )
        {
            Arrow.Parse( cNode );
            arrowParsed = true;
        }
        else if( !textFormatParsed && cNodeName == wxT( "DIMTEXT" ) )
        {
            TextParams.Parse( cNode );
            textFormatParsed = true;
        }
        else if( !extLineParsed && cNodeName == wxT( "EXTLINE" ) )
        {
            ExtensionLineParams.Parse( cNode );
            extLineParsed = true;
        }
        else if( !lineParsed && LINE::IsLine( cNode ) )
        {
            Line.Parse( cNode );
            lineParsed = true;
        }
        else if( !textParsed && cNodeName == wxT( "TEXT" ) )
        {
            Text.Parse( cNode );
            textParsed = true;
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }
}

void CADSTAR_PCB_ARCHIVE_PARSER::SYMDEF::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "SYMDEF" ) );

    ID            = GetXmlAttributeIDString( aNode, 0 );
    ReferenceName = GetXmlAttributeIDString( aNode, 1 );
    wxString rest;

    if( ReferenceName.StartsWith( wxT( "JUMPERNF" ), &rest ) )
        Type = SYMDEF_TYPE::JUMPER;
    else if( ReferenceName.StartsWith( wxT( "STARPOINTNF" ), &rest ) )
        Type = SYMDEF_TYPE::STARPOINT;
    else if( ReferenceName == wxT( "TESTPOINT" ) )
        Type = SYMDEF_TYPE::TESTPOINT;
    else
        Type = SYMDEF_TYPE::COMPONENT;

    Alternate = GetXmlAttributeIDString( aNode, 2 );

    XNODE* cNode            = aNode->GetChildren();
    bool   originParsed     = false;
    bool   symHeightParsed  = false;
    bool   vesionParsed     = false;
    bool   dimensionsParsed = false;

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( !originParsed && cNodeName == wxT( "PT" ) )
        {
            Origin.Parse( cNode );
            originParsed = true;
        }
        else if( cNodeName == wxT( "STUB" ) )
        {
            Stub = true;
        }
        else if( !symHeightParsed && cNodeName == wxT( "SYMHEIGHT" ) )
        {
            SymHeight       = GetXmlAttributeIDLong( cNode, 0 );
            symHeightParsed = true;
        }
        else if( !vesionParsed && cNodeName == wxT( "VERSION" ) )
        {
            Version      = GetXmlAttributeIDLong( cNode, 0 );
            vesionParsed = true;
        }
        else if( cNodeName == wxT( "FIGURE" ) )
        {
            FIGURE figure;
            //TODO try.. catch + throw again with more detailed error information
            figure.Parse( cNode );
            Figures.insert( std::make_pair( figure.ID, figure ) );
        }
        else if( cNodeName == wxT( "COMPCOPPER" ) )
        {
            COMPONENT_COPPER compcopper;
            //TODO try.. catch + throw again with more detailed error information
            compcopper.Parse( cNode );
            ComponentCoppers.push_back( compcopper );
        }
        else if( cNodeName == wxT( "COMPAREA" ) )
        {
            COMPONENT_AREA area;
            //TODO try.. catch + throw again with more detailed error information
            area.Parse( cNode );
            ComponentAreas.insert( std::make_pair( area.ID, area ) );
        }
        else if( cNodeName == wxT( "TEXT" ) )
        {
            TEXT txt;
            //TODO try.. catch + throw again with more detailed error information
            txt.Parse( cNode );
            Texts.insert( std::make_pair( txt.ID, txt ) );
        }
        else if( cNodeName == wxT( "PAD" ) )
        {
            PAD pad;
            //TODO try.. catch + throw again with more detailed error information
            pad.Parse( cNode );
            Pads.insert( std::make_pair( pad.ID, pad ) );
        }
        else if( cNodeName == wxT( "TEXTLOC" ) )
        {
            TEXT_LOCATION textloc;
            //TODO try.. catch + throw again with more detailed error information
            textloc.Parse( cNode );
            TextLocations.insert( std::make_pair( textloc.AttributeID, textloc ) );
        }
        else if( cNodeName == wxT( "ATTR" ) )
        {
            ATTRIBUTE_VALUE attrVal;
            //TODO try.. catch + throw again with more detailed error information
            attrVal.Parse( cNode );
            AttributeValues.insert( std::make_pair( attrVal.AttributeID, attrVal ) );
        }
        else if( !dimensionsParsed && cNodeName == wxT( "DIMENSIONS" ) )
        {
            XNODE* dimensionNode = cNode->GetChildren();

            for( ; dimensionNode; dimensionNode = dimensionNode->GetNext() )
            {
                if( DIMENSION::IsDimension( dimensionNode ) )
                {
                    DIMENSION dim;
                    //TODO try.. catch + throw again with more detailed error information
                    dim.Parse( dimensionNode );
                    Dimensions.insert( std::make_pair( dim.ID, dim ) );
                }
                else
                    THROW_UNKNOWN_NODE_IO_ERROR( dimensionNode->GetName(), cNodeName );
            }

            dimensionsParsed = true;
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }

    if( !originParsed )
        THROW_MISSING_PARAMETER_IO_ERROR( wxT( "PT" ), aNode->GetName() );
}


void CADSTAR_PCB_ARCHIVE_PARSER::LIBRARY::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "LIBRARY" ) );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "SYMDEF" ) )
        {
            SYMDEF symdef;
            //TODO try.. catch + throw again with more detailed error information
            symdef.Parse( cNode );
            ComponentDefinitions.insert( std::make_pair( symdef.ID, symdef ) );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }
}
