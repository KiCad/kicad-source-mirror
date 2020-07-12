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

    for( ; tempNode && ( tempNode->GetName() != wxT( "ASSIGNMENTS" ) );
            tempNode = tempNode->GetNext() )
        ;

    if( !tempNode )
        THROW_MISSING_NODE_IO_ERROR( wxT( "ASSIGNMENTS" ), wxT( "CADSTARPCB" ) );

    tempNode = tempNode->GetChildren();

    for( ; tempNode && ( tempNode->GetName() != wxT( "LAYERDEFS" ) );
            tempNode = tempNode->GetNext() )
        ;

    if( !tempNode )
        THROW_MISSING_NODE_IO_ERROR( wxT( "LAYERDEFS" ), wxT( "ASSIGNMENTS" ) );

    Assignments.Layerdefs.Parse( tempNode );

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
    XNODE *         iNode, *cNode;
    wxXmlAttribute* xmlAttribute = NULL;

    iNode = aNode->GetChildren();

    if( !iNode )
        THROW_MISSING_PARAMETER_IO_ERROR( wxT( "LAYERSTACK" ), wxT( "LAYERDEFS" ) );

    for( ; iNode; iNode = iNode->GetNext() )
    {
        wxString nodeName = iNode->GetName();

        if( nodeName == wxT( "LAYERSTACK" ) )
        {
            xmlAttribute = iNode->GetAttributes();

            for( ; xmlAttribute; xmlAttribute = xmlAttribute->GetNext() )
            {
                LayerStack.push_back( xmlAttribute->GetValue() );
            }

            if( cNode = iNode->GetChildren() ) //Shouldn't have any children
            {
                THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), nodeName );
            }
        }
        else if( nodeName == wxT( "MATERIAL" ) )
        {
            CPA_MATERIAL material;
            material.Parse( iNode );
            Materials.insert( std::make_pair( material.ID, material ) );
        }
        else if( nodeName == wxT( "LAYER" ) )
        {
            CPA_LAYER layer;
            layer.Parse( iNode );
            Layers.insert( std::make_pair( layer.ID, layer ) );
        }
        else if( nodeName == wxT( "SWAPPAIR" ) )
        {
            wxString layerId     = CADSTAR_COMMON::GetAttributeID( iNode, 0 );
            wxString swapLayerId = CADSTAR_COMMON::GetAttributeID( iNode, 1 );

            if( layerId.IsEmpty() || swapLayerId.IsEmpty() )
            {
                THROW_MISSING_PARAMETER_IO_ERROR(
                        wxT( "ID" ), wxString::Format( "SWAPPAIR %s,%s", layerId, swapLayerId ) );
            }

            Layers[layerId].SwapLayerID = swapLayerId;
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( nodeName, aNode->GetName() );
        }
    }
}


void CPAParseEValue( XNODE* aNode, CPA_EVALUE& aValue, wxString location )
{
    if( aNode->GetChildren()->GetName() == wxT( "E" ) )
    {
        aValue.Parse( aNode->GetChildren() );
    }
    else
    {
        THROW_UNKNOWN_NODE_IO_ERROR( aNode->GetChildren()->GetName(), location );
    }
}


void CPAParseNameAndID( XNODE* aNode, wxString& aName, wxString& aID )
{
    aID   = CADSTAR_COMMON::GetAttributeID( aNode, 0 );
    aName = CADSTAR_COMMON::GetAttributeID( aNode, 1 );

    if( aID.IsEmpty() )
        THROW_MISSING_PARAMETER_IO_ERROR(
                wxT( "ID" ), wxString::Format( "%s %s", aNode->GetName(), aName ) );

    if( aName.IsEmpty() )
        THROW_MISSING_PARAMETER_IO_ERROR(
                wxT( "Name" ), wxString::Format( "%s %s", aNode->GetName(), aID ) );
}

void CPA_MATERIAL::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "MATERIAL" ) );

    //Process Name & ID
    CPAParseNameAndID( aNode, Name, ID );

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
        wxString location = wxString::Format( "MATERIAL %s->%s", Name, nodeName );

        if( nodeName == wxT( "RELPERMIT" ) )
        {
            CPAParseEValue( iNode, Permittivity, location );
        }
        else if( nodeName == wxT( "LOSSTANGENT" ) )
        {
            CPAParseEValue( iNode, LossTangent, location );
        }
        else if( nodeName == wxT( "RESISTIVITY" ) )
        {
            CPAParseEValue( iNode, Resistivity, location );
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
    CPAParseNameAndID( aNode, Name, ID );

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

                if( MaterialId.IsEmpty() )
                    THROW_PARSING_IO_ERROR(
                            wxT( "Material ID" ), wxString::Format( "LAYER %s->MAKE", Name ) );

                if( !CADSTAR_COMMON::GetAttributeID( tempNode, 1 ).ToLong( &Thickness ) )
                    THROW_PARSING_IO_ERROR(
                            wxT( "Thickness" ), wxString::Format( "LAYER %s->MAKE", Name ) );

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

void CPA_EVALUE::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "E" ) );

    if( ( !CADSTAR_COMMON::GetAttributeID( aNode, 0 ).ToLong( &Base ) )
            || ( !CADSTAR_COMMON::GetAttributeID( aNode, 1 ).ToLong( &Exponent ) ) )
        THROW_PARSING_IO_ERROR( wxT( "Base and Exponent" ),
                wxString::Format(
                        "%s->%s", aNode->GetParent()->GetName(), aNode->GetParent()->GetName() ) );
}

double CPA_EVALUE::GetDouble()
{
    return Base * std::pow( 10.0, Exponent );
}
