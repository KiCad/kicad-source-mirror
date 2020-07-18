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
 * @file cadstar_pcb.cpp
 * @brief Converts a CPA_FILE object into a KiCad BOARD object
 */

#include <board_stackup_manager/stackup_predefined_prms.h> //KEY_COPPER, KEY_CORE, KEY_PREPREG
#include <cadstar_pcb.h>


void CADSTAR_PCB::Load( CPA_FILE* aCPAfile )
{
    loadBoardStackup( aCPAfile );

    //TODO: process all other items
}


void CADSTAR_PCB::loadBoardStackup( CPA_FILE* aCPAfile )
{
    std::map<CPA_ID, CPA_LAYER>&    cpaLayers     = aCPAfile->Assignments.Layerdefs.Layers;
    std::map<CPA_ID, CPA_MATERIAL>& cpaMaterials  = aCPAfile->Assignments.Layerdefs.Materials;
    std::vector<CPA_ID>&            cpaLayerStack = aCPAfile->Assignments.Layerdefs.LayerStack;
    unsigned                        numElectricalAndPowerLayers = 0;
    BOARD_DESIGN_SETTINGS&          designSettings              = mBoard->GetDesignSettings();
    BOARD_STACKUP&                  stackup                = designSettings.GetStackupDescriptor();
    int                             noOfKiCadStackupLayers = 0;
    int                             lastElectricalLayerIndex = 0;
    int                             dielectricSublayer       = 0;
    int                             numDielectricLayers      = 0;
    bool                            prevWasDielectric        = false;
    BOARD_STACKUP_ITEM*             tempKiCadLayer;
    std::vector<PCB_LAYER_ID>       layerIDs;

    //Remove all layers except required ones
    stackup.RemoveAll();
    layerIDs.push_back( PCB_LAYER_ID::F_CrtYd );
    layerIDs.push_back( PCB_LAYER_ID::B_CrtYd );
    layerIDs.push_back( PCB_LAYER_ID::Margin );
    layerIDs.push_back( PCB_LAYER_ID::Edge_Cuts );
    designSettings.SetEnabledLayers( LSET( &layerIDs[0], layerIDs.size() ) );

    for( auto it = cpaLayerStack.begin(); it != cpaLayerStack.end(); ++it )
    {
        CPA_LAYER               curLayer       = cpaLayers[*it];
        BOARD_STACKUP_ITEM_TYPE kicadLayerType = BOARD_STACKUP_ITEM_TYPE::BS_ITEM_TYPE_UNDEFINED;
        LAYER_T                 copperType     = LAYER_T::LT_UNDEFINED;
        PCB_LAYER_ID            kicadLayerID   = PCB_LAYER_ID::UNDEFINED_LAYER;
        wxString                layerTypeName  = wxEmptyString;

        if( cpaLayers.count( *it ) == 0 )
            wxASSERT_MSG( true, wxT( "Unable to find layer index" ) );

        if( prevWasDielectric && ( curLayer.Type != CPA_LAYER_TYPE::CONSTRUCTION ) )
        {
            stackup.Add( tempKiCadLayer ); //only add dielectric layers here after all are done
            dielectricSublayer = 0;
            prevWasDielectric  = false;
            noOfKiCadStackupLayers++;
        }

        switch( curLayer.Type )
        {
        case CPA_LAYER_TYPE::ALLDOC:
        case CPA_LAYER_TYPE::ALLELEC:
        case CPA_LAYER_TYPE::ALLLAYER:
        case CPA_LAYER_TYPE::ASSCOMPCOPP:
        case CPA_LAYER_TYPE::NOLAYER:
            //Shouldn't be here if CPA file is correctly parsed and not corrupt
            THROW_IO_ERROR( wxString::Format(
                    _( "Unexpected layer '%s' in layer stack." ), curLayer.Name ) );
            continue;
        case CPA_LAYER_TYPE::JUMPERLAYER:
            copperType     = LAYER_T::LT_JUMPER;
            kicadLayerID   = getKiCadCopperLayerID( numElectricalAndPowerLayers++ );
            kicadLayerType = BOARD_STACKUP_ITEM_TYPE::BS_ITEM_TYPE_COPPER;
            layerTypeName  = KEY_COPPER;
            break;
        case CPA_LAYER_TYPE::ELEC:
            copperType     = LAYER_T::LT_SIGNAL;
            kicadLayerID   = getKiCadCopperLayerID( numElectricalAndPowerLayers++ );
            kicadLayerType = BOARD_STACKUP_ITEM_TYPE::BS_ITEM_TYPE_COPPER;
            layerTypeName  = KEY_COPPER;
            break;
        case CPA_LAYER_TYPE::POWER:
            copperType     = LAYER_T::LT_POWER;
            kicadLayerID   = getKiCadCopperLayerID( numElectricalAndPowerLayers++ );
            kicadLayerType = BOARD_STACKUP_ITEM_TYPE::BS_ITEM_TYPE_COPPER;
            layerTypeName  = KEY_COPPER;
            break;
        case CPA_LAYER_TYPE::CONSTRUCTION:
            kicadLayerID      = PCB_LAYER_ID::UNDEFINED_LAYER;
            kicadLayerType    = BOARD_STACKUP_ITEM_TYPE::BS_ITEM_TYPE_DIELECTRIC;
            prevWasDielectric = true;
            layerTypeName     = KEY_PREPREG;
            //TODO handle KEY_CORE and KEY_PREPREG
            //will need to look at CADSTAR layer embedding (CPA_LAYER->Embedding)
            //check electrical layers above and below to decide if current layer is prepreg
            // or core
            break;
        case CPA_LAYER_TYPE::DOC:
            //TODO find out a suitable KiCad Layer alternative for this CADSTAR type
            continue; //ignore
        case CPA_LAYER_TYPE::NONELEC:
            switch( curLayer.SubType )
            {
            case CPA_LAYER_SUBTYPE::LAYERSUBTYPE_ASSEMBLY:
            case CPA_LAYER_SUBTYPE::LAYERSUBTYPE_NONE:
            case CPA_LAYER_SUBTYPE::LAYERSUBTYPE_PLACEMENT:
                //TODO find out a suitable KiCad Layer alternative for these CADSTAR types
                continue; //ignore these layer types for now
            case CPA_LAYER_SUBTYPE::LAYERSUBTYPE_PASTE:
                kicadLayerType = BOARD_STACKUP_ITEM_TYPE::BS_ITEM_TYPE_SOLDERPASTE;
                if( numElectricalAndPowerLayers > 0 )
                {
                    kicadLayerID  = PCB_LAYER_ID::F_Paste;
                    layerTypeName = _HKI( "Top Solder Paste" );
                }
                else
                {
                    kicadLayerID  = PCB_LAYER_ID::B_Paste;
                    layerTypeName = _HKI( "Bottom Solder Paste" );
                }
                break;
            case CPA_LAYER_SUBTYPE::LAYERSUBTYPE_SILKSCREEN:
                kicadLayerType = BOARD_STACKUP_ITEM_TYPE::BS_ITEM_TYPE_SILKSCREEN;
                if( numElectricalAndPowerLayers > 0 )
                {
                    kicadLayerID  = PCB_LAYER_ID::F_SilkS;
                    layerTypeName = _HKI( "Top Silk Screen" );
                }
                else
                {
                    kicadLayerID  = PCB_LAYER_ID::B_SilkS;
                    layerTypeName = _HKI( "Bottom Silk Screen" );
                }
                break;
            case CPA_LAYER_SUBTYPE::LAYERSUBTYPE_SOLDERRESIST:
                kicadLayerType = BOARD_STACKUP_ITEM_TYPE::BS_ITEM_TYPE_SOLDERMASK;
                if( numElectricalAndPowerLayers > 0 )
                {
                    kicadLayerID  = PCB_LAYER_ID::F_Mask;
                    layerTypeName = _HKI( "Top Solder Mask" );
                }
                else
                {
                    kicadLayerID  = PCB_LAYER_ID::B_Mask;
                    layerTypeName = _HKI( "Bottom Solder Mask" );
                }
                break;
            default:
                wxASSERT_MSG( true, wxT( "Unknown CADSTAR Layer Sub-type" ) );
                break;
            }
            break;
        default:
            wxASSERT_MSG( true, wxT( "Unknown CADSTAR Layer Type" ) );
            break;
        }

        if( dielectricSublayer == 0 )
            tempKiCadLayer = new BOARD_STACKUP_ITEM( kicadLayerType );

        tempKiCadLayer->SetLayerName( curLayer.Name );
        tempKiCadLayer->SetBrdLayerId( kicadLayerID );

        if( prevWasDielectric )
        {
            wxASSERT_MSG( kicadLayerID == PCB_LAYER_ID::UNDEFINED_LAYER,
                    wxT( "Error Processing Dielectric Layer. "
                         "Expected to have undefined layer type" ) );

            if( dielectricSublayer == 0 )
                tempKiCadLayer->SetDielectricLayerId( ++numDielectricLayers );
            else
                tempKiCadLayer->AddDielectricPrms( dielectricSublayer );
        }

        if( !curLayer.MaterialId.IsEmpty() )
        {
            tempKiCadLayer->SetMaterial(
                    cpaMaterials[curLayer.MaterialId].Name, dielectricSublayer );
            tempKiCadLayer->SetEpsilonR( cpaMaterials[curLayer.MaterialId].Permittivity.GetDouble(),
                    dielectricSublayer );
            tempKiCadLayer->SetLossTangent(
                    cpaMaterials[curLayer.MaterialId].LossTangent.GetDouble(), dielectricSublayer );
            //TODO add Resistivity when KiCad supports it
        }

        int unitMultiplier = aCPAfile->KiCadUnitMultiplier;
        tempKiCadLayer->SetThickness( curLayer.Thickness * unitMultiplier, dielectricSublayer );

        wxASSERT( layerTypeName != wxEmptyString );
        tempKiCadLayer->SetTypeName( layerTypeName );

        if( !prevWasDielectric )
        {
            stackup.Add( tempKiCadLayer ); //only add non-dielectric layers here
            ++noOfKiCadStackupLayers;
            layerIDs.push_back( tempKiCadLayer->GetBrdLayerId() );
            designSettings.SetEnabledLayers( LSET( &layerIDs[0], layerIDs.size() ) );
        }
        else
            ++dielectricSublayer;

        if( copperType != LAYER_T::LT_UNDEFINED )
        {
            wxASSERT( mBoard->SetLayerType( tempKiCadLayer->GetBrdLayerId(),
                    copperType ) ); //move to outside, need to enable layer in board first
            lastElectricalLayerIndex = noOfKiCadStackupLayers - 1;
            wxASSERT( mBoard->SetLayerName(
                    tempKiCadLayer->GetBrdLayerId(), tempKiCadLayer->GetLayerName() ) );
            //TODO set layer names for other CADSTAR layers when KiCad supports custom
            //layer names on non-copper layers
            mCopperLayers.insert( std::make_pair( curLayer.PhysicalLayer, curLayer.ID ) );
        }
        //TODO map kicad layer to CADSTAR layer in mLayermap
    }

    //change last copper layer to be B_Cu instead of an inner layer
    PCB_LAYER_ID lastElecBrdId =
            stackup.GetStackupLayer( lastElectricalLayerIndex )->GetBrdLayerId();
    std::remove( layerIDs.begin(), layerIDs.end(), lastElecBrdId );
    layerIDs.push_back( PCB_LAYER_ID::B_Cu );
    tempKiCadLayer = stackup.GetStackupLayer( lastElectricalLayerIndex );
    tempKiCadLayer->SetBrdLayerId( PCB_LAYER_ID::B_Cu );
    wxASSERT( mBoard->SetLayerName(
            tempKiCadLayer->GetBrdLayerId(), tempKiCadLayer->GetLayerName() ) );

    //make all layers enabled and visible
    mBoard->SetEnabledLayers( LSET( &layerIDs[0], layerIDs.size() ) );
    mBoard->SetVisibleLayers( LSET( &layerIDs[0], layerIDs.size() ) );

    mBoard->SetCopperLayerCount( numElectricalAndPowerLayers );
}


PCB_LAYER_ID CADSTAR_PCB::getKiCadCopperLayerID( unsigned int aLayerNum )
{
    switch( aLayerNum )
    {
    case 0:
        return PCB_LAYER_ID::F_Cu;
    case 1:
        return PCB_LAYER_ID::In1_Cu;
    case 2:
        return PCB_LAYER_ID::In2_Cu;
    case 3:
        return PCB_LAYER_ID::In3_Cu;
    case 4:
        return PCB_LAYER_ID::In4_Cu;
    case 5:
        return PCB_LAYER_ID::In5_Cu;
    case 6:
        return PCB_LAYER_ID::In6_Cu;
    case 7:
        return PCB_LAYER_ID::In7_Cu;
    case 8:
        return PCB_LAYER_ID::In8_Cu;
    case 9:
        return PCB_LAYER_ID::In9_Cu;
    case 10:
        return PCB_LAYER_ID::In10_Cu;
    case 11:
        return PCB_LAYER_ID::In11_Cu;
    case 12:
        return PCB_LAYER_ID::In12_Cu;
    case 13:
        return PCB_LAYER_ID::In13_Cu;
    case 14:
        return PCB_LAYER_ID::In14_Cu;
    case 15:
        return PCB_LAYER_ID::In15_Cu;
    case 16:
        return PCB_LAYER_ID::In16_Cu;
    case 17:
        return PCB_LAYER_ID::In17_Cu;
    case 18:
        return PCB_LAYER_ID::In18_Cu;
    case 19:
        return PCB_LAYER_ID::In19_Cu;
    case 20:
        return PCB_LAYER_ID::In20_Cu;
    case 21:
        return PCB_LAYER_ID::In21_Cu;
    case 22:
        return PCB_LAYER_ID::In22_Cu;
    case 23:
        return PCB_LAYER_ID::In23_Cu;
    case 24:
        return PCB_LAYER_ID::In24_Cu;
    case 25:
        return PCB_LAYER_ID::In25_Cu;
    case 26:
        return PCB_LAYER_ID::In26_Cu;
    case 27:
        return PCB_LAYER_ID::In27_Cu;
    case 28:
        return PCB_LAYER_ID::In28_Cu;
    case 29:
        return PCB_LAYER_ID::In29_Cu;
    case 30:
        return PCB_LAYER_ID::In30_Cu;
    case 31:
        return PCB_LAYER_ID::B_Cu;
    }
    return PCB_LAYER_ID::UNDEFINED_LAYER;
}
