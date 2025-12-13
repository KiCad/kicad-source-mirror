/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <base_units.h>
#include <cadstar_pcb_archive_parser.h>
#include <macros.h>
#include <progress_reporter.h>
#include <wx/translation.h>


void CADSTAR_PCB_ARCHIVE_PARSER::Parse( bool aLibrary )
{
    if( m_progressReporter )
        m_progressReporter->BeginPhase( 0 ); // Read file

    m_rootNode = LoadArchiveFile( Filename, wxT( "CADSTARPCB" ), m_progressReporter );

    if( m_progressReporter )
    {
        m_progressReporter->BeginPhase( 1 ); // Parse File

        std::vector<wxString> subNodeChildrenToCount = { wxT( "LIBRARY" ), wxT( "PARTS" ),
                                                         wxT( "LAYOUT" ) };

        long numOfSteps = GetNumberOfStepsForReporting( m_rootNode, subNodeChildrenToCount );
        m_progressReporter->SetMaxProgress( numOfSteps );
    }

    m_context.CheckPointCallback = [&](){ checkPoint(); };

    XNODE* cNode = m_rootNode->GetChildren();

    if( !cNode )
        THROW_MISSING_NODE_IO_ERROR( wxT( "HEADER" ), wxT( "CADSTARPCB" ) );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( cNode->GetName() == wxT( "HEADER" ) )
        {
            Header.Parse( cNode, &m_context );

            switch( Header.Resolution )
            {
            case RESOLUTION::HUNDREDTH_MICRON:
                KiCadUnitMultiplier = PCB_IU_PER_MM / 1e5;
                break;

            default:
                wxASSERT_MSG( true, wxT( "Unknown File Resolution" ) );
                break;
            }

            if( aLibrary && Header.Format.Type != wxT( "LIBRARY" ) )
            {
                THROW_IO_ERROR( wxT( "The selected file is not a valid CADSTAR library file." ) );
            }
            else if( !aLibrary && Header.Format.Type != wxT( "LAYOUT" ) )
            {
                if( Header.Format.Type == wxT( "LIBRARY" ) )
                {
                    THROW_IO_ERROR(
                            wxT( "The selected file is a CADSTAR library file (as opposed "
                                 "to a layout file). You can import this library by adding it "
                                 "to the library table." ) );
                }
                else
                {
                    THROW_IO_ERROR( wxT( "The selected file is an unknown CADSTAR format so "
                                         "cannot be imported into KiCad." ) );
                }
            }
        }
        else if( cNode->GetName() == wxT( "ASSIGNMENTS" ) )
        {
            Assignments.Parse( cNode, &m_context );
        }
        else if( cNode->GetName() == wxT( "LIBRARY" ) )
        {
            Library.Parse( cNode, &m_context );
        }
        else if( cNode->GetName() == wxT( "DEFAULTS" ) )
        {
            // No design information here (no need to parse)
            // Only contains CADSTAR configuration data such as default shapes, text and units
            // In future some of this could be converted to KiCad but limited value
        }
        else if( cNode->GetName() == wxT( "PARTS" ) )
        {
            Parts.Parse( cNode, &m_context );
        }
        else if( cNode->GetName() == wxT( "LAYOUT" ) )
        {
            Layout.Parse( cNode, &m_context );
        }
        else if( cNode->GetName() == wxT( "DISPLAY" ) )
        {
            // No design information here (no need to parse)
            // Contains CADSTAR Display settings such as layer/element colours and visibility.
            // In the future these settings could be converted to KiCad
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), wxT( "[root]" ) );
        }

        checkPoint();
    }

    delete m_rootNode;
    m_rootNode = nullptr;
}


void CADSTAR_PCB_ARCHIVE_PARSER::ASSIGNMENTS::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "ASSIGNMENTS" ) );

    XNODE* cNode = aNode->GetChildren();

    if( !cNode )
        THROW_MISSING_NODE_IO_ERROR( wxT( "TECHNOLOGY" ), wxT( "ASSIGNMENTS" ) );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( cNode->GetName() == wxT( "LAYERDEFS" ) )
            Layerdefs.Parse( cNode, aContext );
        else if( cNode->GetName() == wxT( "CODEDEFS" ) )
            Codedefs.Parse( cNode, aContext );
        else if( cNode->GetName() == wxT( "TECHNOLOGY" ) )
            Technology.Parse( cNode, aContext );
        else if( cNode->GetName() == wxT( "GRIDS" ) )
            Grids.Parse( cNode, aContext );
        else if( cNode->GetName() == wxT( "NETCLASSEDITATTRIBSETTINGS" ) )
            NetclassEditAttributeSettings = true;
        else if( cNode->GetName() == wxT( "SPCCLASSEDITATTRIBSETTINGS" ) )
            SpacingclassEditAttributeSettings = true;
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), aNode->GetName() );
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::LAYERDEFS::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "LAYERDEFS" ) );

    wxXmlAttribute* xmlAttribute = nullptr;

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
                if( !IsValidAttribute( xmlAttribute ) )
                    continue;
                else
                    LayerStack.push_back( (LAYER_ID) xmlAttribute->GetValue() );
            }

            CheckNoChildNodes( cNode );
        }
        else if( nodeName == wxT( "MATERIAL" ) )
        {
            MATERIAL material;
            material.Parse( cNode, aContext );
            Materials.insert( std::make_pair( material.ID, material ) );
        }
        else if( nodeName == wxT( "LAYER" ) )
        {
            LAYER layer;
            layer.Parse( cNode, aContext );
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


void CADSTAR_PCB_ARCHIVE_PARSER::RULESET::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "RULESET" ) );

    ID   = GetXmlAttributeIDString( aNode, 0 );
    Name = GetXmlAttributeIDString( aNode, 1 );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString nodeName = cNode->GetName();

        if( nodeName == wxT( "ROUCODEREF" ) )
        {
            AreaRouteCodeID = GetXmlAttributeIDString( cNode, 0 );
        }
        else if( nodeName == wxT( "VIACODEREF" ) )
        {
            AreaViaCodeID = GetXmlAttributeIDString( cNode, 0 );
        }
        else if( nodeName == wxT( "SPACINGCODE" ) )
        {
            SPACINGCODE spacingcode;
            spacingcode.Parse( cNode, aContext );
            SpacingCodes.insert( std::make_pair( spacingcode.ID, spacingcode ) );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( nodeName, aNode->GetName() );
        }
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::CODEDEFS_PCB::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "CODEDEFS" ) );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString nodeName = cNode->GetName();

        if( ParseSubNode( cNode, aContext ) ) // in CADSTAR_ARCHIVE_PARSER::CODEDEFS
        {
        }
        else if( nodeName == wxT( "COPPERCODE" ) )
        {
            COPPERCODE coppercode;
            coppercode.Parse( cNode, aContext );
            CopperCodes.insert( std::make_pair( coppercode.ID, coppercode ) );
        }
        else if( nodeName == wxT( "SPACINGCODE" ) )
        {
            SPACINGCODE spacingcode;
            spacingcode.Parse( cNode, aContext );
            SpacingCodes.insert( std::make_pair( spacingcode.ID, spacingcode ) );
        }
        else if( nodeName == wxT( "RULESET" ) )
        {
            RULESET ruleset;
            ruleset.Parse( cNode, aContext );
            Rulesets.insert( std::make_pair( ruleset.ID, ruleset ) );
        }
        else if( nodeName == wxT( "PADCODE" ) )
        {
            PADCODE padcode;
            padcode.Parse( cNode, aContext );
            PadCodes.insert( std::make_pair( padcode.ID, padcode ) );
        }
        else if( nodeName == wxT( "VIACODE" ) )
        {
            VIACODE viacode;
            viacode.Parse( cNode, aContext );
            ViaCodes.insert( std::make_pair( viacode.ID, viacode ) );
        }
        else if( nodeName == wxT( "LAYERPAIR" ) )
        {
            LAYERPAIR layerpair;
            layerpair.Parse( cNode, aContext );
            LayerPairs.insert( std::make_pair( layerpair.ID, layerpair ) );
        }
        else if( nodeName == wxT( "SPCCLASSSPACE" ) )
        {
            SPCCLASSSPACE spcclassspace;
            spcclassspace.Parse( cNode, aContext );
            SpacingClasses.push_back( spcclassspace );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( nodeName, aNode->GetName() );
        }
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::MATERIAL::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "MATERIAL" ) );

    ID   = GetXmlAttributeIDString( aNode, 0 );
    Name = GetXmlAttributeIDString( aNode, 1 );

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
        THROW_UNKNOWN_PARAMETER_IO_ERROR( sType, wxString::Format( wxT( "MATERIAL %s" ), Name ) );
    }

    XNODE* iNode = aNode->GetChildren();

    if( !iNode )
    {
        THROW_MISSING_PARAMETER_IO_ERROR( wxT( "RESISTIVITY" ),
                                          wxString::Format( wxT( "MATERIAL %s" ), Name ) );
    }

    for( ; iNode; iNode = iNode->GetNext() )
    {
        wxString nodeName = iNode->GetName();

        if( nodeName == wxT( "RELPERMIT" ) )
        {
            ParseChildEValue( iNode, aContext, Permittivity );
        }
        else if( nodeName == wxT( "LOSSTANGENT" ) )
        {
            ParseChildEValue( iNode, aContext, LossTangent );
        }
        else if( nodeName == wxT( "RESISTIVITY" ) )
        {
            ParseChildEValue( iNode, aContext, Resistivity );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( nodeName, wxString::Format( wxT( "MATERIAL %s" ), Name ) );
        }
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::LAYER::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "LAYER" ) );

    ID   = GetXmlAttributeIDString( aNode, 0 );
    Name = GetXmlAttributeIDString( aNode, 1 );

    XNODE* cNode                       = aNode->GetChildren();
    auto   processLayerMaterialDetails = [&]() {
        XNODE* tempNode = cNode->GetChildren();
        for( ; tempNode; tempNode = tempNode->GetNext() )
        {
            wxString tempNodeName = tempNode->GetName();

            if( tempNodeName == wxT( "MAKE" ) || tempNodeName == wxT( "LAYERHEIGHT" ) )
            {
                if( tempNodeName == wxT( "LAYERHEIGHT" ) )
                {
                    Thickness = GetXmlAttributeIDLong( tempNode, 0 );
                }
                else
                {
                    MaterialId = GetXmlAttributeIDString( tempNode, 0 );
                    Thickness  = GetXmlAttributeIDLong( tempNode, 1 );
                }

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
                            THROW_UNKNOWN_PARAMETER_IO_ERROR( embedsValue,
                                                              wxString::Format( wxT( "LAYER %s -> EMBEDS" ),
                                                                                Name ) );
                        }
                    }
                    else
                    {
                        THROW_UNKNOWN_NODE_IO_ERROR( childOfTempNode->GetName(),
                                                     wxString::Format( wxT( "LAYER %s->MAKE" ),
                                                                       Name ) );
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
                    THROW_UNKNOWN_PARAMETER_IO_ERROR( bias,
                                                      wxString::Format( wxT( "LAYER %s -> BIAS" ),
                                                                        Name ) );
                }
            }
            else
            {
                THROW_UNKNOWN_NODE_IO_ERROR( tempNodeName, wxString::Format( wxT( "LAYER %s" ),
                                                                             Name ) );
            }
        }
    };

    for( ; cNode; cNode = cNode->GetNext() )
    {
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
        else if( cNodeName == wxT( "DESCRIPTION" ) )
        {
            Description = GetXmlAttributeIDString( cNode, 0 );
        }
        else if( cNodeName == wxT( "REFPLANE" ) )
        {
            ReferencePlane = true;
        }
        else if( cNodeName == wxT( "VLAYER" ) )
        {
            VariantLayer = true;
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
            else if( sSubType == wxT( "LAYERSUBTYPE_CLEARANCE" ) )
            {
                this->SubType = LAYER_SUBTYPE::LAYERSUBTYPE_CLEARANCE;
            }
            else if( sSubType == wxT( "LAYERSUBTYPE_ROUT" ) )
            {
                this->SubType = LAYER_SUBTYPE::LAYERSUBTYPE_ROUT;
            }
            else
            {
                THROW_UNKNOWN_PARAMETER_IO_ERROR( sSubType, wxString::Format( wxT( "LAYER %s %s" ),
                                                                              Name, cNodeName ) );
            }
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, wxString::Format( wxT( "LAYER %s" ), Name ) );
        }
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::COPREASSIGN::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "COPREASSIGN" ) );

    LayerID = GetXmlAttributeIDString( aNode, 0 );

    CopperWidth = GetXmlAttributeIDLong( aNode, 1 );
}


void CADSTAR_PCB_ARCHIVE_PARSER::COPPERCODE::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "COPPERCODE" ) );

    ID   = GetXmlAttributeIDString( aNode, 0 );
    Name = GetXmlAttributeIDString( aNode, 1 );

    CopperWidth = GetXmlAttributeIDLong( aNode, 2 );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( cNode->GetName() == wxT( "COPREASSIGN" ) )
        {
            CADSTAR_PCB_ARCHIVE_PARSER::COPREASSIGN reassign;
            reassign.Parse( cNode, aContext );
            Reassigns.push_back( reassign );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), aNode->GetName() );
        }
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::SPACINGCODE::REASSIGN::Parse( XNODE* aNode,
                                                               PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "SPACEREASSIGN" ) );

    LayerID = GetXmlAttributeIDString( aNode, 0 );
    Spacing = GetXmlAttributeIDLong( aNode, 1 );

    CheckNoChildNodes( aNode );
}


void CADSTAR_PCB_ARCHIVE_PARSER::SPACINGCODE::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "SPACINGCODE" ) );

    ID      = GetXmlAttributeIDString( aNode, 0 );
    Spacing = GetXmlAttributeIDLong( aNode, 1 );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "SPACEREASSIGN" ) )
        {
            REASSIGN reassign;
            reassign.Parse( cNode, aContext );
            Reassigns.push_back( reassign );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }
}


bool CADSTAR_PCB_ARCHIVE_PARSER::CADSTAR_PAD_SHAPE::IsPadShape( XNODE* aNode )
{
    wxString aNodeName = aNode->GetName();

    if( aNodeName == wxT( "ANNULUS" ) || aNodeName == wxT( "BULLET" ) || aNodeName == wxT( "ROUND" )
            || aNodeName == wxT( "DIAMOND" ) || aNodeName == wxT( "FINGER" )
            || aNodeName == wxT( "OCTAGON" ) || aNodeName == wxT( "RECTANGLE" )
            || aNodeName == wxT( "ROUNDED" ) || aNodeName == wxT( "SQUARE" ) )
    {
        return true;
    }
    else
    {
        return false;
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::CADSTAR_PAD_SHAPE::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
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
        KI_FALLTHROUGH;

    case PAD_SHAPE_TYPE::BULLET:
    case PAD_SHAPE_TYPE::FINGER:
    case PAD_SHAPE_TYPE::RECTANGLE:
        RightLength = GetXmlAttributeIDLong( aNode, 2 );
        LeftLength  = GetXmlAttributeIDLong( aNode, 1 );
        KI_FALLTHROUGH;

    case PAD_SHAPE_TYPE::DIAMOND:
    case PAD_SHAPE_TYPE::OCTAGON:
    case PAD_SHAPE_TYPE::SQUARE:
        if( aNode->GetChildren() )
        {
            if( aNode->GetChildren()->GetName() == wxT( "ORIENT" ) )
            {
                OrientAngle = GetXmlAttributeIDLong( aNode->GetChildren(), 0 );
            }
            else
            {
                THROW_UNKNOWN_NODE_IO_ERROR( aNode->GetChildren()->GetName(), aNode->GetName() );
            }

            CheckNoNextNodes( aNode->GetChildren() );
        }
        KI_FALLTHROUGH;

    case PAD_SHAPE_TYPE::CIRCLE:
        Size = GetXmlAttributeIDLong( aNode, 0 );
        break;
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::PADREASSIGN::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "PADREASSIGN" ) );

    LayerID = GetXmlAttributeIDString( aNode, 0 );

    if( CADSTAR_PAD_SHAPE::IsPadShape( aNode->GetChildren() ) )
        Shape.Parse( aNode->GetChildren(), aContext );
    else
        THROW_UNKNOWN_NODE_IO_ERROR( aNode->GetChildren()->GetName(), aNode->GetName() );

    CheckNoNextNodes( aNode->GetChildren() );
}


void CADSTAR_PCB_ARCHIVE_PARSER::PADCODE::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "PADCODE" ) );

    ID   = GetXmlAttributeIDString( aNode, 0 );
    Name = GetXmlAttributeIDString( aNode, 1 );

    XNODE*   cNode    = aNode->GetChildren();
    wxString location = wxString::Format( wxT( "PADCODE -> %s" ), Name );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( CADSTAR_PAD_SHAPE::IsPadShape( cNode ) )
        {
            Shape.Parse( cNode, aContext );
        }
        else if( cNodeName == wxT( "CLEARANCE" ) )
        {
            ReliefClearance = GetXmlAttributeIDLong( cNode, 0 );
        }
        else if( cNodeName == wxT( "RELIEFWIDTH" ) )
        {
            ReliefWidth = GetXmlAttributeIDLong( cNode, 0 );
        }
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
        {
            SlotLength = GetXmlAttributeIDLong( cNode, 0 );
        }
        else if( cNodeName == wxT( "DRILLORIENTATION" ) )
        {
            SlotOrientation = GetXmlAttributeIDLong( cNode, 0 );
        }
        else if( cNodeName == wxT( "DRILLXOFFSET" ) )
        {
            DrillXoffset = GetXmlAttributeIDLong( cNode, 0 );
        }
        else if( cNodeName == wxT( "DRILLYOFFSET" ) )
        {
            DrillYoffset = GetXmlAttributeIDLong( cNode, 0 );
        }
        else if( cNodeName == wxT( "PADREASSIGN" ) )
        {
            PADREASSIGN reassign;
            reassign.Parse( cNode, aContext );
            Reassigns.insert( std::make_pair( reassign.LayerID, reassign.Shape ) );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, location );
        }
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::VIAREASSIGN::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "VIAREASSIGN" ) );

    LayerID = GetXmlAttributeIDString( aNode, 0 );

    if( CADSTAR_PAD_SHAPE::IsPadShape( aNode->GetChildren() ) )
        Shape.Parse( aNode->GetChildren(), aContext );
    else
        THROW_UNKNOWN_NODE_IO_ERROR( aNode->GetChildren()->GetName(), aNode->GetName() );

    CheckNoNextNodes( aNode->GetChildren() );
}


void CADSTAR_PCB_ARCHIVE_PARSER::VIACODE::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "VIACODE" ) );

    ID   = GetXmlAttributeIDString( aNode, 0 );
    Name = GetXmlAttributeIDString( aNode, 1 );

    XNODE*   cNode    = aNode->GetChildren();
    wxString location = wxString::Format( wxT( "VIACODE -> %s" ), Name );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( CADSTAR_PAD_SHAPE::IsPadShape( cNode ) )
        {
            Shape.Parse( cNode, aContext );
        }
        else if( cNodeName == wxT( "CLEARANCE" ) )
        {
            ReliefClearance = GetXmlAttributeIDLong( cNode, 0 );
        }
        else if( cNodeName == wxT( "RELIEFWIDTH" ) )
        {
            ReliefWidth = GetXmlAttributeIDLong( cNode, 0 );
        }
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
            VIAREASSIGN reassign;
            reassign.Parse( cNode, aContext );
            Reassigns.insert( std::make_pair( reassign.LayerID, reassign.Shape ) );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, location );
        }
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::LAYERPAIR::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "LAYERPAIR" ) );

    ID   = GetXmlAttributeIDString( aNode, 0 );
    Name = GetXmlAttributeIDString( aNode, 1 );

    PhysicalLayerStart = GetXmlAttributeIDLong( aNode, 2 );
    PhysicalLayerEnd   = GetXmlAttributeIDLong( aNode, 3 );

    wxString location = wxString::Format( wxT( "LAYERPAIR -> %s" ), Name );

    if( aNode->GetChildren() )
    {
        if( aNode->GetChildren()->GetName() == wxT( "VIACODEREF" ) )
        {
            ViacodeID = GetXmlAttributeIDString( aNode->GetChildren(), 0 );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( aNode->GetChildren()->GetName(), location );
        }

        CheckNoNextNodes( aNode->GetChildren() );
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::SPCCLASSSPACE::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "SPCCLASSSPACE" ) );

    SpacingClassID1 = GetXmlAttributeIDString( aNode, 0 );
    SpacingClassID2 = GetXmlAttributeIDString( aNode, 1 );
    LayerID         = GetXmlAttributeIDString( aNode, 2 );
    Spacing         = GetXmlAttributeIDLong( aNode, 3 );
}


void CADSTAR_PCB_ARCHIVE_PARSER::TECHNOLOGY_SECTION::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "TECHNOLOGY" ) );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( ParseSubNode( cNode, aContext ) ) //CADSTAR_ARCHIVE_PARSER::SETTINGS
        {
        }
        else if( cNodeName == wxT( "MINROUTEWIDTH" ) )
        {
            MinRouteWidth = GetXmlAttributeIDLong( cNode, 0 );
        }
        else if( cNodeName == wxT( "MINNECKED" ) )
        {
            MinNeckedLength = GetXmlAttributeIDLong( cNode, 0 );
        }
        else if( cNodeName == wxT( "MINUNNECKED" ) )
        {
            MinUnneckedLength = GetXmlAttributeIDLong( cNode, 0 );
        }
        else if( cNodeName == wxT( "MINMITER" ) )
        {
            MinMitre = GetXmlAttributeIDLong( cNode, 0 );
        }
        else if( cNodeName == wxT( "MAXMITER" ) )
        {
            MaxMitre = GetXmlAttributeIDLong( cNode, 0 );
        }
        else if( cNodeName == wxT( "MAXPHYSLAYER" ) )
        {
            MaxPhysicalLayer = GetXmlAttributeIDLong( cNode, 0 );
        }
        else if( cNodeName == wxT( "TRACKGRID" ) )
        {
            TrackGrid = GetXmlAttributeIDLong( cNode, 0 );
        }
        else if( cNodeName == wxT( "VIAGRID" ) )
        {
            ViaGrid = GetXmlAttributeIDLong( cNode, 0 );
        }
        else if( cNodeName == wxT( "BACKOFFJCTS" ) )
        {
            BackOffJunctions = true;
        }
        else if( cNodeName == wxT( "BCKOFFWIDCHANGE" ) )
        {
            BackOffWidthChange = true;
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, wxT( "TECHNOLOGY" ) );
        }
    }
}


CADSTAR_PCB_ARCHIVE_PARSER::PAD_SIDE CADSTAR_PCB_ARCHIVE_PARSER::GetPadSide(
        const wxString& aPadSideString )
{
    if( aPadSideString == wxT( "THRU" ) )
        return PAD_SIDE::THROUGH_HOLE;
    else if( aPadSideString == wxT( "BOTTOM" ) )
        return PAD_SIDE::MAXIMUM;
    else if( aPadSideString == wxT( "TOP" ) )
        return PAD_SIDE::MINIMUM;
    else
        return PAD_SIDE::THROUGH_HOLE; // Assume through hole as default
}


void CADSTAR_PCB_ARCHIVE_PARSER::COMPONENT_COPPER::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "COMPCOPPER" ) );

    CopperCodeID = GetXmlAttributeIDString( aNode, 0 );
    LayerID      = GetXmlAttributeIDString( aNode, 1 );

    XNODE*   cNode              = aNode->GetChildren();
    bool     shapeIsInitialised = false; // Stop more than one Shape Object
    wxString location           = wxT( "COMPCOPPER" );

    if( !cNode )
        THROW_MISSING_NODE_IO_ERROR( wxT( "Shape" ), location );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( !shapeIsInitialised && Shape.IsShape( cNode ) )
        {
            Shape.Parse( cNode, aContext );
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
                if( !IsValidAttribute( xmlAttribute ) )
                    continue;

                long padId;

                if( !xmlAttribute->GetValue().ToLong( &padId ) )
                    THROW_PARSING_IO_ERROR( wxT( "ASSOCPIN" ), location );

                AssociatedPadIDs.push_back( (PAD_ID) padId );
            }

            CheckNoChildNodes( cNode );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, location );
        }
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::COMPONENT_AREA::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "COMPAREA" ) );

    ID         = GetXmlAttributeIDString( aNode, 0 );
    LineCodeID = GetXmlAttributeIDString( aNode, 1 );
    LayerID    = GetXmlAttributeIDString( aNode, 3 );

    XNODE*   cNode              = aNode->GetChildren();
    bool     shapeIsInitialised = false; // Stop more than one Shape Object
    wxString location           = wxString::Format( wxT( "COMPAREA %s" ), ID );

    if( !cNode )
        THROW_MISSING_NODE_IO_ERROR( wxT( "Shape" ), location );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( !shapeIsInitialised && SHAPE::IsShape( cNode ) )
        {
            Shape.Parse( cNode, aContext );
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
                if( !IsValidAttribute( xmlAttribute ) )
                    continue;

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
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, location );
        }
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::PAD_EXITS::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "EXITS" ) );

    wxXmlAttribute* xmlAttribute = aNode->GetAttributes();

    for( ; xmlAttribute; xmlAttribute = xmlAttribute->GetNext() )
    {
        if( !IsValidAttribute( xmlAttribute ) )
            continue;

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


void CADSTAR_PCB_ARCHIVE_PARSER::COMPONENT_PAD::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "PAD" ) );

    ID        = GetXmlAttributeIDLong( aNode, 0 );
    PadCodeID = GetXmlAttributeIDString( aNode, 2 );
    Side      = GetPadSide( GetXmlAttributeIDString( aNode, 3 ) );

    XNODE*   cNode    = aNode->GetChildren();
    wxString location = wxString::Format( wxT( "PAD %ld" ), ID );

    if( !cNode )
        THROW_MISSING_NODE_IO_ERROR( wxT( "PT" ), location );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "ORIENT" ) )
            OrientAngle = GetXmlAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "FIRSTPAD" ) )
            FirstPad = true;
        else if( cNodeName == wxT( "EXITS" ) )
            Exits.Parse( cNode, aContext );
        else if( cNodeName == wxT( "PADIDENTIFIER" ) )
            Identifier = GetXmlAttributeIDString( cNode, 0 );
        else if( cNodeName == wxT( "PCBONLYPAD" ) )
            PCBonlyPad = true;
        else if( cNodeName == wxT( "PT" ) )
            Position.Parse( cNode, aContext );
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, location );
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::DIMENSION::ARROW::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
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


void CADSTAR_PCB_ARCHIVE_PARSER::DIMENSION::TEXTFORMAT::Parse( XNODE* aNode,
                                                               PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "DIMTEXT" ) );

    TextGap    = GetXmlAttributeIDLong( aNode, 1 );
    TextOffset = GetXmlAttributeIDLong( aNode, 2 );

    XNODE* cNode = aNode->GetChildren();

    if( !cNode || cNode->GetName() != wxT( "TXTSTYLE" ) )
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


void CADSTAR_PCB_ARCHIVE_PARSER::DIMENSION::EXTENSION_LINE::Parse( XNODE* aNode,
                                                                   PARSER_CONTEXT* aContext )
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
                Start.Parse( cNode, aContext );
            else
                End.Parse( cNode, aContext );
        }
        else if( cNodeName == wxT( "SUPPRESSFIRST" ) )
        {
            SuppressFirst = true;
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, wxT( "EXTLINE" ) );
        }
    }

    if( noOfPoints != 2 )
        THROW_MISSING_PARAMETER_IO_ERROR( wxT( "PT" ), wxT( "EXTLINE" ) );
}


bool CADSTAR_PCB_ARCHIVE_PARSER::DIMENSION::LINE::IsLine( XNODE* aNode )
{
    if( aNode->GetName() == wxT( "LEADERLINE" ) || aNode->GetName() == wxT( "LINEARLINE" )
            || aNode->GetName() == wxT( "ANGULARLINE" ) )
    {
        return true;
    }
    else
    {
        return false;
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::DIMENSION::LINE::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( IsLine( aNode ) );

    if( aNode->GetName() == wxT( "LINEARLINE" ) )
        Type = TYPE::LINEARLINE;
    else if( aNode->GetName() == wxT( "LEADERLINE" ) )
        Type = TYPE::LEADERLINE;
    else if( aNode->GetName() == wxT( "ANGULARLINE" ) )
        Type = TYPE::ANGULARLINE;
    else
        wxASSERT_MSG( true, wxT( "Not a valid type. What happened to the node Name?" ) );

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
                Start.Parse( cNode, aContext );
            else if( noOfPoints == 2 )
                End.Parse( cNode, aContext );
            else
                Centre.Parse( cNode, aContext );
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
    {
        return true;
    }
    else
    {
        return false;
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::DIMENSION::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( IsDimension( aNode ) );

    std::map<wxString, TYPE> typeMap = { { wxT( "LINEARDIM" ), TYPE::LINEARDIM },
        { wxT( "LEADERDIM" ), TYPE::LEADERDIM }, { wxT( "ANGLEDIM" ), TYPE::ANGLEDIM } };

    //make sure aNode is valid TYPE
    wxASSERT_MSG( typeMap.find( aNode->GetName() ) != typeMap.end(),
                  wxT( "Not a valid type. What happened to the node Name?" ) );

    Type                = typeMap[aNode->GetName()];
    LayerID             = GetXmlAttributeIDString( aNode, 1 );
    wxString subTypeStr = GetXmlAttributeIDString( aNode, 2 );

    std::map<wxString, SUBTYPE> subTypeMap = {
        { wxT( "DIMENSION_ORTHOGONAL" ), SUBTYPE::ORTHOGONAL },
        { wxT( "DIMENSION_DIRECT" ),     SUBTYPE::DIRECT },
        { wxT( "DIMENSION_ANGLED" ),     SUBTYPE::ANGLED },
        { wxT( "DIMENSION_DIAMETER" ),   SUBTYPE::DIAMETER },
        { wxT( "DIMENSION_RADIUS" ),     SUBTYPE::RADIUS },
        { wxT( "DIMENSION_ANGULAR" ),    SUBTYPE::ANGULAR } };

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
            Arrow.Parse( cNode, aContext );
            arrowParsed = true;
        }
        else if( !textFormatParsed && cNodeName == wxT( "DIMTEXT" ) )
        {
            TextParams.Parse( cNode, aContext );
            textFormatParsed = true;
        }
        else if( !extLineParsed && cNodeName == wxT( "EXTLINE" ) )
        {
            ExtensionLineParams.Parse( cNode, aContext );
            extLineParsed = true;
        }
        else if( !lineParsed && LINE::IsLine( cNode ) )
        {
            Line.Parse( cNode, aContext );
            lineParsed = true;
        }
        else if( !textParsed && cNodeName == wxT( "TEXT" ) )
        {
            // Do not parse the fields in dimension text (will be done when loading, if required)
            Text.Parse( cNode, aContext, false );
            textParsed = true;
        }
        else if( cNodeName == wxT( "FIX" ) )
        {
            Fixed = true;
        }
        else if( cNodeName == wxT( "GROUPREF" ) )
        {
            GroupID = GetXmlAttributeIDString( cNode, 0 );
        }
        else if( cNodeName == wxT( "REUSEBLOCKREF" ) )
        {
            ReuseBlockRef.Parse( cNode, aContext );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::SYMDEF_PCB::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "SYMDEF" ) );

    ParseIdentifiers( aNode, aContext );

    wxString rest;

    if( ReferenceName.StartsWith( wxT( "JUMPERNF" ), &rest ) )
        Type = SYMDEF_TYPE::JUMPER;
    else if( ReferenceName.StartsWith( wxT( "STARPOINTNF" ), &rest ) )
        Type = SYMDEF_TYPE::STARPOINT;
    else if( ReferenceName == wxT( "TESTPOINT" ) )
        Type = SYMDEF_TYPE::TESTPOINT;
    else
        Type = SYMDEF_TYPE::COMPONENT;

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( ParseSubNode( cNode, aContext ) )
        {
            continue;
        }
        else if( cNodeName == wxT( "SYMHEIGHT" ) )
        {
            SymHeight = GetXmlAttributeIDLong( cNode, 0 );
        }
        else if( cNodeName == wxT( "COMPCOPPER" ) )
        {
            COMPONENT_COPPER compcopper;
            compcopper.Parse( cNode, aContext );
            ComponentCoppers.push_back( compcopper );
        }
        else if( cNodeName == wxT( "COMPAREA" ) )
        {
            COMPONENT_AREA area;
            area.Parse( cNode, aContext );
            ComponentAreas.insert( std::make_pair( area.ID, area ) );
        }
        else if( cNodeName == wxT( "PAD" ) )
        {
            COMPONENT_PAD pad;
            pad.Parse( cNode, aContext );
            ComponentPads.insert( std::make_pair( pad.ID, pad ) );
        }
        else if( cNodeName == wxT( "DIMENSIONS" ) )
        {
            XNODE* dimensionNode = cNode->GetChildren();

            for( ; dimensionNode; dimensionNode = dimensionNode->GetNext() )
            {
                if( DIMENSION::IsDimension( dimensionNode ) )
                {
                    DIMENSION dim;
                    dim.Parse( dimensionNode, aContext );
                    Dimensions.insert( std::make_pair( dim.ID, dim ) );
                }
                else
                {
                    THROW_UNKNOWN_NODE_IO_ERROR( dimensionNode->GetName(), cNodeName );
                }
            }
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }

    if( !Stub && ( Origin.x == UNDEFINED_VALUE || Origin.y == UNDEFINED_VALUE ) )
        THROW_MISSING_PARAMETER_IO_ERROR( wxT( "PT" ), aNode->GetName() );
}


void CADSTAR_PCB_ARCHIVE_PARSER::LIBRARY::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "LIBRARY" ) );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "SYMDEF" ) )
        {
            SYMDEF_PCB symdef;
            symdef.Parse( cNode, aContext );
            ComponentDefinitions.insert( std::make_pair( symdef.ID, symdef ) );
        }
        else if( cNodeName == wxT( "HIERARCHY" ) )
        {
            // Ignore for now
            //
            // This node doesn't have any equivalent in KiCad so for now we ignore it. In
            // future, we could parse it in detail, to obtain the tree-structure of
            // footprints in a cadstar library
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }

        aContext->CheckPointCallback();
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::CADSTAR_BOARD::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "BOARD" ) );

    ID         = GetXmlAttributeIDString( aNode, 0 );
    LineCodeID = GetXmlAttributeIDString( aNode, 1 );

    XNODE*   cNode              = aNode->GetChildren();
    bool     shapeIsInitialised = false; // Stop more than one Shape Object
    wxString location           = wxString::Format( wxT( "BOARD %s" ), ID );

    if( !cNode )
        THROW_MISSING_NODE_IO_ERROR( wxT( "Shape" ), location );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( !shapeIsInitialised && SHAPE::IsShape( cNode ) )
        {
            Shape.Parse( cNode, aContext );
            shapeIsInitialised = true;
        }
        else if( cNodeName == wxT( "ATTR" ) )
        {
            ATTRIBUTE_VALUE attr;
            attr.Parse( cNode, aContext );
            AttributeValues.insert( std::make_pair( attr.AttributeID, attr ) );
        }
        else if( cNodeName == wxT( "FIX" ) )
        {
            Fixed = true;
        }
        else if( cNodeName == wxT( "GROUPREF" ) )
        {
            GroupID = GetXmlAttributeIDString( cNode, 0 );
        }
        else if( cNodeName == wxT( "REUSEBLOCKREF" ) )
        {
            ReuseBlockRef.Parse( cNode, aContext );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, location );
        }
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::AREA::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "AREA" ) );

    ID         = GetXmlAttributeIDString( aNode, 0 );
    LineCodeID = GetXmlAttributeIDString( aNode, 1 );
    Name       = GetXmlAttributeIDString( aNode, 2 );
    LayerID    = GetXmlAttributeIDString( aNode, 4 );

    XNODE*   cNode              = aNode->GetChildren();
    bool     shapeIsInitialised = false; // Stop more than one Shape Object
    wxString location           = wxString::Format( wxT( "AREA %s" ), ID );

    if( !cNode )
        THROW_MISSING_NODE_IO_ERROR( wxT( "Shape" ), location );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( !shapeIsInitialised && SHAPE::IsShape( cNode ) )
        {
            Shape.Parse( cNode, aContext );
            shapeIsInitialised = true;
        }
        else if( cNodeName == wxT( "FIX" ) )
        {
            Fixed = true;
        }
        else if( cNodeName == wxT( "USAGE" ) )
        {
            wxXmlAttribute* xmlAttribute = cNode->GetAttributes();

            for( ; xmlAttribute; xmlAttribute = xmlAttribute->GetNext() )
            {
                if( !IsValidAttribute( xmlAttribute ) )
                    continue;

                if( xmlAttribute->GetValue() == wxT( "PLACEMENT" ) )
                    Placement = true;
                else if( xmlAttribute->GetValue() == wxT( "ROUTING" ) )
                    Routing = true;
                else if( xmlAttribute->GetValue() == wxT( "KEEPOUT" ) )
                    Keepout = true;
                else if( xmlAttribute->GetValue() == wxT( "NO_TRACKS" ) )
                    NoTracks = true;
                else if( xmlAttribute->GetValue() == wxT( "NO_VIAS" ) )
                    NoVias = true;
                else
                    THROW_UNKNOWN_PARAMETER_IO_ERROR( xmlAttribute->GetValue(), location );
            }

            CheckNoChildNodes( cNode );
        }
        else if( cNodeName == wxT( "AREAHEIGHT" ) )
        {
            AreaHeight = GetXmlAttributeIDLong( cNode, 0 );
        }
        else if( cNodeName == wxT( "GROUPREF" ) )
        {
            GroupID = GetXmlAttributeIDString( cNode, 0 );
        }
        else if( cNodeName == wxT( "REUSEBLOCKREF" ) )
        {
            ReuseBlockRef.Parse( cNode, aContext );
        }
        else if( cNodeName == wxT( "ATTR" ) )
        {
            ATTRIBUTE_VALUE attr;
            attr.Parse( cNode, aContext );
            AttributeValues.insert( std::make_pair( attr.AttributeID, attr ) );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, location );
        }
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::PIN_ATTRIBUTE::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "PINATTR" ) );

    Pin = GetXmlAttributeIDLong( aNode, 0 );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "ATTR" ) )
        {
            ATTRIBUTE_VALUE attrVal;
            attrVal.Parse( cNode, aContext );
            AttributeValues.insert( std::make_pair( attrVal.AttributeID, attrVal ) );
        }
        else if( cNodeName == wxT( "TESTLAND" ) )
        {
            TestlandSide = ParseTestlandSide( cNode );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::PADEXCEPTION::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "PADEXCEPTION" ) );

    ID = GetXmlAttributeIDLong( aNode, 0 );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "PADCODEREF" ) )
        {
            PadCode = GetXmlAttributeIDString( cNode, 0 );
        }
        else if( cNodeName == wxT( "EXITS" ) )
        {
            OverrideExits = true;
            Exits.Parse( cNode, aContext );
        }
        else if( cNodeName == wxT( "SIDE" ) )
        {
            OverrideSide = true;
            Side         = GetPadSide( GetXmlAttributeIDString( cNode, 0 ) );
        }
        else if( cNodeName == wxT( "ORIENT" ) )
        {
            OverrideOrientation = true;
            OrientAngle         = GetXmlAttributeIDLong( cNode, 0 );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::COMPONENT::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "COMP" ) );

    ID       = GetXmlAttributeIDString( aNode, 0 );
    Name     = GetXmlAttributeIDString( aNode, 1 );
    PartID   = GetXmlAttributeIDString( aNode, 2 );
    SymdefID = GetXmlAttributeIDString( aNode, 3 );

    XNODE* cNode        = aNode->GetChildren();
    bool   originParsed = false;

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( !originParsed && cNodeName == wxT( "PT" ) )
        {
            Origin.Parse( cNode, aContext );
            originParsed = true;
        }
        else if( cNodeName == wxT( "GROUPREF" ) )
        {
            GroupID = GetXmlAttributeIDString( cNode, 0 );
        }
        else if( cNodeName == wxT( "REUSEBLOCKREF" ) )
        {
            ReuseBlockRef.Parse( cNode, aContext );
        }
        else if( cNodeName == wxT( "TESTPOINT" ) )
        {
            TestPoint = true;
        }
        else if( cNodeName == wxT( "FIX" ) )
        {
            Fixed = true;
        }
        else if( cNodeName == wxT( "MIRROR" ) )
        {
            Mirror = true;
        }
        else if( cNodeName == wxT( "READABILITY" ) )
        {
            Readability = ParseReadability( cNode );
        }
        else if( cNodeName == wxT( "ORIENT" ) )
        {
            OrientAngle = GetXmlAttributeIDLong( cNode, 0 );
        }
        else if( cNodeName == wxT( "VCOMPMASTER" ) )
        {
            VariantParentComponentID = GetXmlAttributeIDString( cNode, 0 );
            VariantID                = GetXmlAttributeIDString( cNode, 1 );
        }
        else if( cNodeName == wxT( "TEXTLOC" ) )
        {
            TEXT_LOCATION textloc;
            textloc.Parse( cNode, aContext );
            TextLocations.insert( std::make_pair( textloc.AttributeID, textloc ) );
        }
        else if( cNodeName == wxT( "ATTR" ) )
        {
            ATTRIBUTE_VALUE attrVal;
            attrVal.Parse( cNode, aContext );
            AttributeValues.insert( std::make_pair( attrVal.AttributeID, attrVal ) );
        }
        else if( cNodeName == wxT( "PINATTR" ) )
        {
            PIN_ATTRIBUTE pinAttr;
            pinAttr.Parse( cNode, aContext );
            PinAttributes.insert( std::make_pair( pinAttr.Pin, pinAttr ) );
        }
        else if( cNodeName == wxT( "COMPPINLABEL" ) )
        {
            PART_DEFINITION_PIN_ID pinID    = GetXmlAttributeIDLong( cNode, 0 );
            wxString               pinLabel = GetXmlAttributeIDString( cNode, 1 );
            PinLabels.insert( std::make_pair( pinID, pinLabel ) );
        }
        else if( cNodeName == wxT( "PADEXCEPTION" ) )
        {
            PADEXCEPTION padExcept;
            padExcept.Parse( cNode, aContext );
            PadExceptions.insert( std::make_pair( padExcept.ID, padExcept ) );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }

    if( !originParsed )
        THROW_MISSING_PARAMETER_IO_ERROR( wxT( "PT" ), aNode->GetName() );
}


CADSTAR_PCB_ARCHIVE_PARSER::TESTLAND_SIDE CADSTAR_PCB_ARCHIVE_PARSER::ParseTestlandSide(
        XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "TESTLAND" ) );

    wxString side = GetXmlAttributeIDString( aNode, 0 );

    if( side == wxT( "MIN_SIDE" ) )
        return TESTLAND_SIDE::MIN;
    else if( side == wxT( "MAX_SIDE" ) )
        return TESTLAND_SIDE::MAX;
    else if( side == wxT( "BOTH_SIDES" ) )
        return TESTLAND_SIDE::BOTH;
    else
        THROW_UNKNOWN_PARAMETER_IO_ERROR( side, aNode->GetName() );

    return TESTLAND_SIDE::NONE;
}


void CADSTAR_PCB_ARCHIVE_PARSER::TRUNK::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "TRUNK" ) );

    ID         = GetXmlAttributeIDString( aNode, 0 );
    Definition = GetXmlAttributeIDString( aNode, 1 );
}


void CADSTAR_PCB_ARCHIVE_PARSER::NET_PCB::PIN::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "PIN" ) );

    ID          = GetXmlAttributeIDString( aNode, 0 );
    ComponentID = GetXmlAttributeIDString( aNode, 1 );
    PadID       = GetXmlAttributeIDLong( aNode, 2 );
    CheckNoChildNodes( aNode );
}


void CADSTAR_PCB_ARCHIVE_PARSER::NET_PCB::JUNCTION_PCB::Parse( XNODE* aNode,
                                                               PARSER_CONTEXT* aContext )
{
    ParseIdentifiers( aNode, aContext );
    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( ParseSubNode( cNode, aContext ) )
            continue;
        else if( cNode->GetName() == wxT( "TRUNKREF" ) )
            TrunkID = GetXmlAttributeIDString( cNode, 0 );
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), aNode->GetName() );
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::NET_PCB::VIA::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "VIA" ) );

    ID          = GetXmlAttributeIDString( aNode, 0 );
    ViaCodeID   = GetXmlAttributeIDString( aNode, 1 );
    LayerPairID = GetXmlAttributeIDString( aNode, 2 );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "PT" ) )
            Location.Parse( cNode, aContext );
        else if( cNodeName == wxT( "FIX" ) )
            Fixed = true;
        else if( cNodeName == wxT( "GROUPREF" ) )
            GroupID = GetXmlAttributeIDString( cNode, 0 );
        else if( cNodeName == wxT( "REUSEBLOCKREF" ) )
            ReuseBlockRef.Parse( cNode, aContext );
        else if( cNodeName == wxT( "TESTLAND" ) )
            TestlandSide = ParseTestlandSide( cNode );
        else if( cNode->GetName() == wxT( "TRUNKREF" ) )
            TrunkID = GetXmlAttributeIDString( cNode, 0 );
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::NET_PCB::COPPER_TERMINAL::Parse( XNODE* aNode,
                                                                  PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "COPTERM" ) );

    ID            = GetXmlAttributeIDString( aNode, 0 );
    CopperID      = GetXmlAttributeIDString( aNode, 1 );
    CopperTermNum = GetXmlAttributeIDLong( aNode, 2 );
}


XNODE* CADSTAR_PCB_ARCHIVE_PARSER::NET_PCB::ROUTE_VERTEX::Parse( XNODE* aNode,
                                                                 PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "ROUTEWIDTH" ) );

    RouteWidth      = GetXmlAttributeIDLong( aNode, 0 );
    XNODE* prevNode = aNode;
    XNODE* nextNode = aNode->GetNext();

    for( ; nextNode; nextNode = nextNode->GetNext() )
    {
        if( nextNode->GetName() == wxT( "FIX" ) )
        {
            Fixed = true;
        }
        else if( nextNode->GetName() == wxT( "TDROPATSTART" ) )
        {
            TeardropAtStart = true;
            TeardropAtStartAngle = GetXmlAttributeIDLong( nextNode, 0 );
        }
        else if( nextNode->GetName() == wxT( "TDROPATEND" ) )
        {
            TeardropAtEnd = true;
            TeardropAtEndAngle = GetXmlAttributeIDLong( nextNode, 0 );
        }
        else if( VERTEX::IsVertex( nextNode ) )
        {
            Vertex.Parse( nextNode, aContext );
        }
        else if( nextNode->GetName() == wxT( "ROUTEWIDTH" ) )
        {
            return prevNode;
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( nextNode->GetName(), wxT( "ROUTE" ) );
        }

        prevNode = nextNode;
    }

    return prevNode;
}


void CADSTAR_PCB_ARCHIVE_PARSER::NET_PCB::ROUTE::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "ROUTE" ) );

    LayerID = GetXmlAttributeIDString( aNode, 0 );

    //Parse child nodes
    XNODE* cNode            = aNode->GetChildren();
    bool   startPointParsed = false;

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( !startPointParsed && cNodeName == wxT( "PT" ) )
        {
            startPointParsed = true;
            StartPoint.Parse( cNode, aContext );
        }
        else if( cNodeName == wxT( "ROUTEWIDTH" ) )
        {
            ROUTE_VERTEX rtVert;
            cNode = rtVert.Parse( cNode, aContext );
            RouteVertices.push_back( rtVert );

            assert( cNode != nullptr );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, wxT( "ROUTE" ) );
        }
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::NET_PCB::CONNECTION_PCB::Parse( XNODE* aNode,
                                                                 PARSER_CONTEXT* aContext )
{
    ParseIdentifiers( aNode, aContext );

    //Parse child nodes
    XNODE* cNode       = aNode->GetChildren();
    bool   routeParsed = false; //assume only one route per connection

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( ParseSubNode( cNode, aContext ) )
        {
            continue;
        }
        else if( !Unrouted && !routeParsed && cNodeName == wxT( "ROUTE" ) )
        {
            Route.Parse( cNode, aContext );
            routeParsed = true;
        }
        else if( !routeParsed && cNodeName == wxT( "UNROUTE" ) )
        {
            Unrouted       = true;
            UnrouteLayerID = GetXmlAttributeIDString( cNode, 0 );
        }
        else if( cNode->GetName() == wxT( "TRUNKREF" ) )
        {
            TrunkID = GetXmlAttributeIDString( cNode, 0 );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, wxT( "CONN" ) );
        }
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::NET_PCB::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    ParseIdentifiers( aNode, aContext );

    //Parse child nodes
    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "JPT" ) )
        {
            JUNCTION_PCB jpt;
            jpt.Parse( cNode, aContext );
            Junctions.insert( std::make_pair( jpt.ID, jpt ) );
        }
        else if( ParseSubNode( cNode, aContext ) )
        {
            continue;
        }
        else if( cNodeName == wxT( "PIN" ) )
        {
            PIN pin;
            pin.Parse( cNode, aContext );
            Pins.insert( std::make_pair( pin.ID, pin ) );
        }
        else if( cNodeName == wxT( "VIA" ) )
        {
            VIA via;
            via.Parse( cNode, aContext );
            Vias.insert( std::make_pair( via.ID, via ) );
        }
        else if( cNodeName == wxT( "COPTERM" ) )
        {
            COPPER_TERMINAL cterm;
            cterm.Parse( cNode, aContext );
            CopperTerminals.insert( std::make_pair( cterm.ID, cterm ) );
        }
        else if( cNodeName == wxT( "CONN" ) )
        {
            CONNECTION_PCB conn;
            conn.Parse( cNode, aContext );
            Connections.push_back( conn );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, wxT( "NET" ) );
        }
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::TEMPLATE::POURING::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "POURING" ) );

    CopperCodeID           = GetXmlAttributeIDString( aNode, 0 );
    ReliefCopperCodeID     = GetXmlAttributeIDString( aNode, 1 );
    ClearanceWidth         = GetXmlAttributeIDLong( aNode, 2 );
    SliverWidth            = GetXmlAttributeIDLong( aNode, 3 );
    AdditionalIsolation    = GetXmlAttributeIDLong( aNode, 4 );
    ThermalReliefPadsAngle = GetXmlAttributeIDLong( aNode, 5 );
    ThermalReliefViasAngle = GetXmlAttributeIDLong( aNode, 6 );

    wxString MinIsolCopStr = GetXmlAttributeIDString( aNode, 7 );

    if( MinIsolCopStr == wxT( "NONE" ) )
        MinIsolatedCopper = UNDEFINED_VALUE;
    else
        MinIsolatedCopper = GetXmlAttributeIDLong( aNode, 7 );

    wxString MinDisjCopStr = GetXmlAttributeIDString( aNode, 8 );

    if( MinDisjCopStr == wxT( "NONE" ) )
        MinDisjointCopper = UNDEFINED_VALUE;
    else
        MinDisjointCopper = GetXmlAttributeIDLong( aNode, 8 );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "NOPINRELIEF" ) )
        {
            ThermalReliefOnPads = false;
        }
        else if( cNodeName == wxT( "NOVIARELIEF" ) )
        {
            ThermalReliefOnVias = false;
        }
        else if( cNodeName == wxT( "IGNORETRN" ) )
        {
            AllowInNoRouting = true;
        }
        else if( cNodeName == wxT( "BOXPINS" ) )
        {
            BoxIsolatedPins = true;
        }
        else if( cNodeName == wxT( "REGENERATE" ) )
        {
            AutomaticRepour = true;
        }
        else if( cNodeName == wxT( "AUTOROUTETARGET" ) )
        {
            TargetForAutorouting = true;
        }
        else if( cNodeName == wxT( "THERMALCUTOUT" ) )
        {
            ReliefType = RELIEF_TYPE::CUTOUTS;
        }
        else if( cNodeName == wxT( "FILLED" ) )
        {
            FillType = COPPER_FILL_TYPE::FILLED;
        }
        else if( cNodeName == wxT( "HATCHCODEREF" ) )
        {
            FillType    = COPPER_FILL_TYPE::HATCHED;
            HatchCodeID = GetXmlAttributeIDString( cNode, 0 );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, wxT( "POURING" ) );
        }
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::TEMPLATE::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "TEMPLATE" ) );

    ID         = GetXmlAttributeIDString( aNode, 0 );
    LineCodeID = GetXmlAttributeIDString( aNode, 1 );
    Name       = GetXmlAttributeIDString( aNode, 2 );
    NetID      = GetXmlAttributeIDString( aNode, 3 );
    LayerID    = GetXmlAttributeIDString( aNode, 4 );

    XNODE* cNode         = aNode->GetChildren();
    bool   shapeParsed   = false;
    bool   pouringParsed = false;

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( !shapeParsed && SHAPE::IsShape( cNode ) )
        {
            Shape.Parse( cNode, aContext );
            shapeParsed = true;
        }
        else if( !pouringParsed && cNodeName == wxT( "POURING" ) )
        {
            Pouring.Parse( cNode, aContext );
            pouringParsed = true;
        }
        else if( cNodeName == wxT( "FIX" ) )
        {
            Fixed = true;
        }
        else if( cNodeName == wxT( "GROUPREF" ) )
        {
            GroupID = GetXmlAttributeIDString( cNode, 0 );
        }
        else if( cNodeName == wxT( "REUSEBLOCKREF" ) )
        {
            ReuseBlockRef.Parse( cNode, aContext );
        }
        else if( cNodeName == wxT( "ATTR" ) )
        {
            ATTRIBUTE_VALUE attr;
            attr.Parse( cNode, aContext );
            AttributeValues.insert( std::make_pair( attr.AttributeID, attr ) );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, wxT( "TEMPLATE" ) );
        }
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::COPPER::NETREF::COPPER_TERM::Parse( XNODE* aNode,
                                                                     PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "TERM" ) );

    ID = GetXmlAttributeIDLong( aNode, 0 );

    XNODE* cNode          = aNode->GetChildren();
    bool   locationParsed = false;

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( !locationParsed && cNodeName == wxT( "PT" ) )
        {
            Location.Parse( cNode, aContext );
            locationParsed = true;
        }
        else if( cNodeName == wxT( "FIX" ) )
        {
            Fixed = true;
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::COPPER::NETREF::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "NETREF" ) );

    NetID = GetXmlAttributeIDString( aNode, 0 );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "TERM" ) )
        {
            COPPER_TERM term;
            term.Parse( cNode, aContext );
            CopperTerminals.insert( std::make_pair( term.ID, term ) );
        }
        else if( cNodeName == wxT( "FIX" ) )
        {
            Fixed = true;
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, wxT( "NETREF" ) );
        }
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::COPPER::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "COPPER" ) );

    ID           = GetXmlAttributeIDString( aNode, 0 );
    CopperCodeID = GetXmlAttributeIDString( aNode, 1 );
    LayerID      = GetXmlAttributeIDString( aNode, 2 );

    XNODE* cNode        = aNode->GetChildren();
    bool   shapeParsed  = false;
    bool   netRefParsed = false;

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( !shapeParsed && SHAPE::IsShape( cNode ) )
        {
            Shape.Parse( cNode, aContext );
            shapeParsed = true;
        }
        else if( !netRefParsed && cNodeName == wxT( "NETREF" ) )
        {
            NetRef.Parse( cNode, aContext );
            netRefParsed = true;
        }
        else if( cNodeName == wxT( "FIX" ) )
        {
            Fixed = true;
        }
        else if( cNodeName == wxT( "GROUPREF" ) )
        {
            GroupID = GetXmlAttributeIDString( cNode, 0 );
        }
        else if( cNodeName == wxT( "REUSEBLOCKREF" ) )
        {
            ReuseBlockRef.Parse( cNode, aContext );
        }
        else if( cNodeName == wxT( "POURED" ) )
        {
            PouredTemplateID = GetXmlAttributeIDString( cNode, 0 );
        }
        else if( cNodeName == wxT( "ATTR" ) )
        {
            ATTRIBUTE_VALUE attr;
            attr.Parse( cNode, aContext );
            AttributeValues.insert( std::make_pair( attr.AttributeID, attr ) );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, wxT( "TEMPLATE" ) );
        }
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::DRILL_TABLE::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "DRILLTABLE" ) );

    ID      = GetXmlAttributeIDString( aNode, 0 );
    LayerID = GetXmlAttributeIDString( aNode, 1 );

    XNODE* cNode          = aNode->GetChildren();
    bool   positionParsed = false;

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( !positionParsed && cNodeName == wxT( "PT" ) )
        {
            Position.Parse( cNode, aContext );
            positionParsed = true;
        }
        else if( cNodeName == wxT( "ORIENT" ) )
        {
            OrientAngle = GetXmlAttributeIDLong( cNode, 0 );
        }
        else if( cNodeName == wxT( "MIRROR" ) )
        {
            Mirror = true;
        }
        else if( cNodeName == wxT( "FIX" ) )
        {
            Fixed = true;
        }
        else if( cNodeName == wxT( "READABILITY" ) )
        {
            Readability = ParseReadability( cNode );
        }
        else if( cNodeName == wxT( "GROUPREF" ) )
        {
            GroupID = GetXmlAttributeIDString( cNode, 0 );
        }
        else if( cNodeName == wxT( "REUSEBLOCKREF" ) )
        {
            ReuseBlockRef.Parse( cNode, aContext );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }
}


void CADSTAR_PCB_ARCHIVE_PARSER::LAYOUT::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "LAYOUT" ) );

    XNODE* cNode            = aNode->GetChildren();
    bool   netSynchParsed   = false;
    bool   dimensionsParsed = false;

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( !netSynchParsed && cNodeName == wxT( "NETSYNCH" ) )
        {
            std::map<wxString, NETSYNCH> netSynchMap = { { wxT( "WARNING" ), NETSYNCH::WARNING },
                { wxT( "FULL" ), NETSYNCH::FULL } };

            wxString nsString = GetXmlAttributeIDString( cNode, 0 );

            if( netSynchMap.find( nsString ) == netSynchMap.end() )
                THROW_UNKNOWN_PARAMETER_IO_ERROR( nsString, aNode->GetName() );

            NetSynch       = netSynchMap[nsString];
            netSynchParsed = true;
        }
        else if( cNodeName == wxT( "GROUP" ) )
        {
            GROUP group;
            group.Parse( cNode, aContext );
            Groups.insert( std::make_pair( group.ID, group ) );
        }
        else if( cNodeName == wxT( "REUSEBLOCK" ) )
        {
            REUSEBLOCK reuseblock;
            reuseblock.Parse( cNode, aContext );
            ReuseBlocks.insert( std::make_pair( reuseblock.ID, reuseblock ) );
        }
        else if( cNodeName == wxT( "BOARD" ) )
        {
            CADSTAR_BOARD board;
            board.Parse( cNode, aContext );
            Boards.insert( std::make_pair( board.ID, board ) );
        }
        else if( cNodeName == wxT( "FIGURE" ) )
        {
            FIGURE figure;
            figure.Parse( cNode, aContext );
            Figures.insert( std::make_pair( figure.ID, figure ) );
        }
        else if( cNodeName == wxT( "AREA" ) )
        {
            AREA area;
            area.Parse( cNode, aContext );
            Areas.insert( std::make_pair( area.ID, area ) );
        }
        else if( cNodeName == wxT( "COMP" ) )
        {
            COMPONENT comp;
            comp.Parse( cNode, aContext );
            Components.insert( std::make_pair( comp.ID, comp ) );
        }
        else if( cNodeName == wxT( "TRUNK" ) )
        {
            TRUNK trunk;
            trunk.Parse( cNode, aContext );
            Trunks.insert( std::make_pair( trunk.ID, trunk ) );
        }
        else if( cNodeName == wxT( "NET" ) )
        {
            NET_PCB net;
            net.Parse( cNode, aContext );
            Nets.insert( std::make_pair( net.ID, net ) );
        }
        else if( cNodeName == wxT( "TEMPLATE" ) )
        {
            TEMPLATE temp;
            temp.Parse( cNode, aContext );
            Templates.insert( std::make_pair( temp.ID, temp ) );
        }
        else if( cNodeName == wxT( "COPPER" ) )
        {
            COPPER copper;
            copper.Parse( cNode, aContext );
            Coppers.insert( std::make_pair( copper.ID, copper ) );
        }
        else if( cNodeName == wxT( "TEXT" ) )
        {
            TEXT txt;
            txt.Parse( cNode, aContext );
            Texts.insert( std::make_pair( txt.ID, txt ) );
        }
        else if( cNodeName == wxT( "DOCSYMBOL" ) )
        {
            DOCUMENTATION_SYMBOL docsym;
            docsym.Parse( cNode, aContext );
            DocumentationSymbols.insert( std::make_pair( docsym.ID, docsym ) );
        }
        else if( !dimensionsParsed && cNodeName == wxT( "DIMENSIONS" ) )
        {
            XNODE* dimensionNode = cNode->GetChildren();

            for( ; dimensionNode; dimensionNode = dimensionNode->GetNext() )
            {
                if( DIMENSION::IsDimension( dimensionNode ) )
                {
                    DIMENSION dim;
                    dim.Parse( dimensionNode, aContext );
                    Dimensions.insert( std::make_pair( dim.ID, dim ) );
                }
                else
                {
                    THROW_UNKNOWN_NODE_IO_ERROR( dimensionNode->GetName(), cNodeName );
                }
            }

            dimensionsParsed = true;
        }
        else if( cNodeName == wxT( "DRILLTABLE" ) )
        {
            DRILL_TABLE drilltable;
            drilltable.Parse( cNode, aContext );
            DrillTables.insert( std::make_pair( drilltable.ID, drilltable ) );
        }
        else if( cNodeName == wxT( "VHIERARCHY" ) )
        {
            VariantHierarchy.Parse( cNode, aContext );
        }
        else if( cNodeName == wxT( "ERRORMARK" ) )
        {
            //ignore (this is a DRC error marker in cadstar)
            continue;
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }

        aContext->CheckPointCallback();
    }
}
