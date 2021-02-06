/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2021 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file cadstar_pcb_archive_loader.cpp
 * @brief Loads a cpa file into a KiCad BOARD object
 */

#include <cadstar_pcb_archive_loader.h>

#include <board_stackup_manager/stackup_predefined_prms.h> // KEY_COPPER, KEY_CORE, KEY_PREPREG
#include <board.h>
#include <dimension.h>
#include <pcb_shape.h>
#include <fp_shape.h>
#include <footprint.h>
#include <pcb_text.h>
#include <project.h>
#include <track.h>
#include <zone.h>
#include <convert_basic_shapes_to_polygon.h>
#include <trigo.h>

#include <limits> // std::numeric_limits


void CADSTAR_PCB_ARCHIVE_LOADER::Load( ::BOARD* aBoard, ::PROJECT* aProject )
{
    mBoard = aBoard;
    mProject = aProject;

    Parse();

    LONGPOINT designLimit = Assignments.Technology.DesignLimit;

    //Note: can't use getKiCadPoint() due wxPoint being int - need long long to make the check
    long long designSizeXkicad = (long long) designLimit.x * KiCadUnitMultiplier;
    long long designSizeYkicad = (long long) designLimit.y * KiCadUnitMultiplier;

    // Max size limited by the positive dimension of wxPoint (which is an int)
    long long maxDesignSizekicad = std::numeric_limits<int>::max();

    if( designSizeXkicad > maxDesignSizekicad || designSizeYkicad > maxDesignSizekicad )
    {
        THROW_IO_ERROR( wxString::Format(
                _( "The design is too large and cannot be imported into KiCad. \n"
                   "Please reduce the maximum design size in CADSTAR by navigating to: \n"
                   "Design Tab -> Properties -> Design Options -> Maximum Design Size. \n"
                   "Current Design size: %.2f, %.2f millimeters. \n"
                   "Maximum permitted design size: %.2f, %.2f millimeters.\n" ),
                (double) designSizeXkicad / PCB_IU_PER_MM,
                (double) designSizeYkicad / PCB_IU_PER_MM,
                (double) maxDesignSizekicad / PCB_IU_PER_MM,
                (double) maxDesignSizekicad / PCB_IU_PER_MM ) );
    }

    mDesignCenter =
            ( Assignments.Technology.DesignArea.first + Assignments.Technology.DesignArea.second )
            / 2;

    if( Layout.NetSynch == NETSYNCH::WARNING )
    {
        wxLogWarning(
                _( "The selected file indicates that nets might be out of synchronisation "
                   "with the schematic. It is recommended that you carry out an 'Align Nets' "
                   "procedure in CADSTAR and re-import, to avoid inconsistencies between the "
                   "PCB and the schematic. " ) );
    }

    loadBoardStackup();
    remapUnsureLayers();
    loadDesignRules();
    loadComponentLibrary();
    loadGroups();
    loadBoards();
    loadFigures();
    loadTexts();
    loadDimensions();
    loadAreas();
    loadComponents();
    loadDocumentationSymbols();
    loadTemplates();
    loadCoppers();
    calculateZonePriorities();
    loadNets();
    loadTextVariables();

    if( Layout.Trunks.size() > 0 )
    {
        wxLogWarning(
                _( "The CADSTAR design contains Trunk routing elements, which have no KiCad "
                   "equivalent. These elements were not loaded." ) );
    }

    if( Layout.VariantHierarchy.Variants.size() > 0 )
    {
        wxLogWarning( wxString::Format(
                _( "The CADSTAR design contains variants which has no KiCad equivalent. Only "
                   "the variant '%s' was loaded." ),
                Layout.VariantHierarchy.Variants.begin()->second.Name ) );
    }

    if( Layout.ReuseBlocks.size() > 0 )
    {
        wxLogWarning(
                _( "The CADSTAR design contains re-use blocks which has no KiCad equivalent. The "
                   "re-use block information has been discarded during the import." ) );
    }

    wxLogMessage(
            _( "The CADSTAR design has been imported successfully.\n"
               "Please review the import errors and warnings (if any)." ) );
}

std::vector<FOOTPRINT*> CADSTAR_PCB_ARCHIVE_LOADER::GetLoadedLibraryFootpints() const
{
    std::vector<FOOTPRINT*> retval;

    for( std::pair<SYMDEF_ID, FOOTPRINT*> fpPair : mLibraryMap )
    {
        retval.push_back( static_cast<FOOTPRINT*>( fpPair.second->Clone() ) );
    }

    return retval;
}


void CADSTAR_PCB_ARCHIVE_LOADER::logBoardStackupWarning(
        const wxString& aCadstarLayerName,
                                                         const PCB_LAYER_ID& aKiCadLayer )
{
    if( mLogLayerWarnings )
    {
        wxLogWarning( wxString::Format(
                _( "The CADSTAR layer '%s' has no KiCad equivalent. All elements on this "
                   "layer have been mapped to KiCad layer '%s' instead." ),
                aCadstarLayerName, LSET::Name( aKiCadLayer ) ) );
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::logBoardStackupMessage( const wxString& aCadstarLayerName,
                                                         const PCB_LAYER_ID& aKiCadLayer )
{
    if( mLogLayerWarnings )
    {
        wxLogMessage( wxString::Format(
                _( "The CADSTAR layer '%s' has been assumed to be a technical layer. All "
                   "elements on this layer have been mapped to KiCad layer '%s'." ),
                aCadstarLayerName, LSET::Name( aKiCadLayer ) ) );
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::initStackupItem( const LAYER&          aCadstarLayer,
                                                  ::BOARD_STACKUP_ITEM* aKiCadItem,
                                                  int aDielectricSublayer )
{
    if( !aCadstarLayer.MaterialId.IsEmpty() )
    {
        MATERIAL material = Assignments.Layerdefs.Materials.at( aCadstarLayer.MaterialId );

        aKiCadItem->SetMaterial( material.Name, aDielectricSublayer );
        aKiCadItem->SetEpsilonR( material.Permittivity.GetDouble(), aDielectricSublayer );
        aKiCadItem->SetLossTangent( material.LossTangent.GetDouble(), aDielectricSublayer );
        //TODO add Resistivity when KiCad supports it
    }

    aKiCadItem->SetLayerName( aCadstarLayer.Name );
    aKiCadItem->SetThickness( getKiCadLength( aCadstarLayer.Thickness ), aDielectricSublayer );
}


void CADSTAR_PCB_ARCHIVE_LOADER::loadBoardStackup()
{
    // Structure describing an electrical layer with optional dielectric layers below it
    // (construction layers in CADSTAR)
    struct LAYER_BLOCK
    {
        LAYER_ID ElecLayerID = wxEmptyString;     // Normally not empty, but could be empty if the
                                                  // first layer in the stackup is a construction
                                                  // layer
        std::vector<LAYER_ID> ConstructionLayers; // Normally empty for the last electrical layer
                                                  // but it is possible to build a board in CADSTAR
                                                  // with no construction layers or with the bottom
                                                  // layer being a construction layer

        bool IsInitialised() { return !ElecLayerID.IsEmpty() || ConstructionLayers.size() > 0; };
    };

    std::vector<LAYER_BLOCK> cadstarBoardStackup;
    LAYER_BLOCK              currentBlock;

    // Find the electrical and construction (dielectric) layers in the stackup
    for( LAYER_ID cadstarLayerID : Assignments.Layerdefs.LayerStack )
    {
        LAYER cadstarLayer = Assignments.Layerdefs.Layers.at( cadstarLayerID );

        if( cadstarLayer.Type == LAYER_TYPE::JUMPERLAYER ||
            cadstarLayer.Type == LAYER_TYPE::POWER ||
            cadstarLayer.Type == LAYER_TYPE::ELEC )
        {
            if( currentBlock.IsInitialised() )
            {
                cadstarBoardStackup.push_back( currentBlock );
                currentBlock = LAYER_BLOCK(); // reset the block
            }

            currentBlock.ElecLayerID = cadstarLayerID;
        }
        else if( cadstarLayer.Type == LAYER_TYPE::CONSTRUCTION )
        {
            currentBlock.ConstructionLayers.push_back( cadstarLayerID );
        }
    }

    if( currentBlock.IsInitialised() )
        cadstarBoardStackup.push_back( currentBlock );

    int totalCopperLayers = cadstarBoardStackup.size();

    // Special case: last layer in the stackup is a construction layer, we need to use B.Cu as a
    // dummy layer
    if( cadstarBoardStackup.back().ConstructionLayers.size() > 0 )
    {
        cadstarBoardStackup.push_back( LAYER_BLOCK() ); //Add dummy layer at the end
        ++totalCopperLayers;
    }

    // Make sure it is an even number of layers (KiCad doesn't yet support unbalanced stack-ups)
    if( ( totalCopperLayers % 2 ) != 0 )
    {
        LAYER_BLOCK bottomLayer = cadstarBoardStackup.back();
        cadstarBoardStackup.pop_back();

        LAYER_BLOCK secondToLastLayer = cadstarBoardStackup.back();
        cadstarBoardStackup.pop_back();

        LAYER_BLOCK dummyLayer;
        LAYER_ID           lastConstruction = secondToLastLayer.ConstructionLayers.back();

        if( secondToLastLayer.ConstructionLayers.size() > 1 )
        {
            // At least two construction layers, lets remove it here and use it in the dummy layer
            secondToLastLayer.ConstructionLayers.pop_back();
        }
        else
        {
            // There is only one construction layer, lets halve its thickness so it is split evenly
            // between this layer and the dummy layer
            Assignments.Layerdefs.Layers.at( lastConstruction ).Thickness /= 2;
        }

        dummyLayer.ConstructionLayers.push_back( lastConstruction );
        cadstarBoardStackup.push_back( secondToLastLayer );
        cadstarBoardStackup.push_back( dummyLayer );
        cadstarBoardStackup.push_back( bottomLayer );
        ++totalCopperLayers;
    }

    wxASSERT( totalCopperLayers == cadstarBoardStackup.size() );
    wxASSERT( cadstarBoardStackup.back().ConstructionLayers.size() == 0 );


    // Create a new stackup from default stackup list
    BOARD_STACKUP& stackup = mBoard->GetDesignSettings().GetStackupDescriptor();
    stackup.RemoveAll();
    mBoard->SetEnabledLayers( LSET::AllLayersMask() );
    mBoard->SetVisibleLayers( LSET::AllLayersMask() );
    mBoard->SetCopperLayerCount( totalCopperLayers );
    stackup.BuildDefaultStackupList( &mBoard->GetDesignSettings(), totalCopperLayers );

    size_t stackIndex = 0;

    for( BOARD_STACKUP_ITEM* item : stackup.GetList() )
    {
        if( item->GetType() == BOARD_STACKUP_ITEM_TYPE::BS_ITEM_TYPE_COPPER )
        {
            LAYER_ID layerID = cadstarBoardStackup.at( stackIndex ).ElecLayerID;

            if( layerID.IsEmpty() )
            {
                // Loading a dummy layer. Make zero thickness so it doesn't affect overall stackup
                item->SetThickness( 0 );
            }
            else
            {
                LAYER copperLayer = Assignments.Layerdefs.Layers.at( layerID );
                initStackupItem( copperLayer, item, 0 );
                LAYER_T copperType = LAYER_T::LT_SIGNAL;

                switch( copperLayer.Type )
                {
                case LAYER_TYPE::JUMPERLAYER:
                    copperType = LAYER_T::LT_JUMPER;
                    break;

                case LAYER_TYPE::ELEC:
                    copperType = LAYER_T::LT_SIGNAL;
                    break;

                case LAYER_TYPE::POWER:
                    copperType = LAYER_T::LT_POWER;
                    mPowerPlaneLayers.push_back( copperLayer.ID ); //need to add a Copper zone
                    break;

                default:
                    wxFAIL_MSG( "Unexpected Layer type. Was expecting an electrical type" );
                    break;
                }

                mBoard->SetLayerType( item->GetBrdLayerId(), copperType );
                mBoard->SetLayerName( item->GetBrdLayerId(), item->GetLayerName() );
                mCopperLayers.insert( { copperLayer.PhysicalLayer, copperLayer.ID } );
                mLayermap.insert( { copperLayer.ID, item->GetBrdLayerId() } );
            }
        }
        else if( item->GetType() == BOARD_STACKUP_ITEM_TYPE::BS_ITEM_TYPE_DIELECTRIC )
        {
            LAYER_BLOCK layerBlock = cadstarBoardStackup.at( stackIndex );
            LAYER_BLOCK layerBlockBelow = cadstarBoardStackup.at( stackIndex + 1 );

            // We should have made sure all layer blocks have at least one construction layer
            wxASSERT( layerBlock.ConstructionLayers.size() > 0 );

            int dielectricId = stackIndex + 1;
            // item->SetBrdLayerId();
            item->SetDielectricLayerId( dielectricId );

            //Prepreg or core?
            //Look at CADSTAR layer embedding (see LAYER->Embedding) to check whether the electrical
            //layer embeds above and below to decide if current layer is prepreg or core
            if( layerBlock.ElecLayerID.IsEmpty() )
            {
                //Dummy electrical layer, assume prepreg
                item->SetTypeName( KEY_PREPREG );
            }
            else
            {
                LAYER copperLayer = Assignments.Layerdefs.Layers.at( layerBlock.ElecLayerID );

                if( layerBlockBelow.ElecLayerID.IsEmpty() )
                {
                    // Dummy layer below, just use current layer to decide

                    if( copperLayer.Embedding == EMBEDDING::ABOVE )
                        item->SetTypeName( KEY_CORE );
                    else
                        item->SetTypeName( KEY_PREPREG );
                }
                else
                {
                    LAYER copperLayerBelow =
                            Assignments.Layerdefs.Layers.at( layerBlockBelow.ElecLayerID );

                    if( copperLayer.Embedding == EMBEDDING::ABOVE )
                    {
                        // Need to check layer below is embedding downwards
                        if( copperLayerBelow.Embedding == EMBEDDING::BELOW )
                            item->SetTypeName( KEY_CORE );
                        else
                            item->SetTypeName( KEY_PREPREG );
                    }
                    else
                    {
                        item->SetTypeName( KEY_PREPREG );
                    }
                }
            }

            int dielectricSublayer = 0;

            for( LAYER_ID constructionLaID : layerBlock.ConstructionLayers )
            {
                LAYER dielectricLayer = Assignments.Layerdefs.Layers.at( constructionLaID );

                if( dielectricSublayer )
                    item->AddDielectricPrms( dielectricSublayer );

                initStackupItem( dielectricLayer, item, dielectricSublayer );
                mBoard->SetLayerName( item->GetBrdLayerId(), item->GetLayerName() );
                mLayermap.insert( { dielectricLayer.ID, item->GetBrdLayerId() } );
                ++dielectricSublayer;
            }

            ++stackIndex;
        }
    }

    int numElecAndPowerLayers = 0;

    for( LAYER_ID cadstarLayerID : Assignments.Layerdefs.LayerStack )
    {
        LAYER        curLayer = Assignments.Layerdefs.Layers.at( cadstarLayerID );
        PCB_LAYER_ID kicadLayerID = PCB_LAYER_ID::UNDEFINED_LAYER;
        wxString     layerName = curLayer.Name.Lower();

        enum class LOG_LEVEL
        {
            NONE,
            MSG,
            WARN
        };

        auto selectLayerID =
            [&]( PCB_LAYER_ID aFront, PCB_LAYER_ID aBack, LOG_LEVEL aLogType )
            {
                if( numElecAndPowerLayers > 0 )
                    kicadLayerID = aBack;
                else
                    kicadLayerID = aFront;

                switch( aLogType )
                {
                case LOG_LEVEL::NONE:
                    break;

                case LOG_LEVEL::MSG:
                    logBoardStackupMessage( curLayer.Name, kicadLayerID );
                    break;

                case LOG_LEVEL::WARN:
                    logBoardStackupMessage( curLayer.Name, kicadLayerID );
                    break;
                }
            };

        switch( curLayer.Type )
        {
        case LAYER_TYPE::ALLDOC:
        case LAYER_TYPE::ALLELEC:
        case LAYER_TYPE::ALLLAYER:
        case LAYER_TYPE::ASSCOMPCOPP:
        case LAYER_TYPE::NOLAYER:
            //Shouldn't be here if CPA file is correctly parsed and not corrupt
            THROW_IO_ERROR( wxString::Format(
                    _( "Unexpected layer '%s' in layer stack." ), curLayer.Name ) );
            break;

        case LAYER_TYPE::JUMPERLAYER:
        case LAYER_TYPE::ELEC:
        case LAYER_TYPE::POWER:
            ++numElecAndPowerLayers;
            KI_FALLTHROUGH;
        case LAYER_TYPE::CONSTRUCTION:
            //Already dealt with these when loading board stackup
            break;

        case LAYER_TYPE::DOC:
            selectLayerID( PCB_LAYER_ID::Dwgs_User, PCB_LAYER_ID::Cmts_User, LOG_LEVEL::WARN );
            break;

        case LAYER_TYPE::NONELEC:
            switch( curLayer.SubType )
            {
            case LAYER_SUBTYPE::LAYERSUBTYPE_ASSEMBLY:
                selectLayerID( PCB_LAYER_ID::F_Fab, PCB_LAYER_ID::B_Fab, LOG_LEVEL::NONE );
                break;

            case LAYER_SUBTYPE::LAYERSUBTYPE_PLACEMENT:
                selectLayerID( PCB_LAYER_ID::F_CrtYd, PCB_LAYER_ID::B_CrtYd, LOG_LEVEL::NONE );
                break;

            case LAYER_SUBTYPE::LAYERSUBTYPE_NONE:
                // Generic Non-electrical layer (older CADSTAR versions).
                // Attempt to detect technical layers by string matching.
                if( layerName.Contains( "glue" ) || layerName.Contains( "adhesive" ) )
                {
                    selectLayerID( PCB_LAYER_ID::F_Adhes, PCB_LAYER_ID::B_Adhes, LOG_LEVEL::MSG );
                }
                else if( layerName.Contains( "silk" ) || layerName.Contains( "legend" ) )
                {
                    selectLayerID( PCB_LAYER_ID::F_SilkS, PCB_LAYER_ID::B_SilkS, LOG_LEVEL::MSG );
                }
                else if( layerName.Contains( "assembly" ) || layerName.Contains( "fabrication" ) )
                {
                    selectLayerID( PCB_LAYER_ID::F_Fab, PCB_LAYER_ID::B_Fab, LOG_LEVEL::MSG );
                }
                else if( layerName.Contains( "resist" ) || layerName.Contains( "mask" ) )
                {
                    selectLayerID( PCB_LAYER_ID::F_Mask, PCB_LAYER_ID::B_Mask, LOG_LEVEL::MSG );
                }
                else if( layerName.Contains( "paste" ) )
                {
                    selectLayerID( PCB_LAYER_ID::F_Paste, PCB_LAYER_ID::B_Paste, LOG_LEVEL::MSG );
                }
                else
                {
                    // Does not appear to be a technical layer - Map to Eco layers for now.
                    selectLayerID( PCB_LAYER_ID::Eco1_User, PCB_LAYER_ID::Eco2_User,
                                   LOG_LEVEL::WARN );
                }
                break;

            case LAYER_SUBTYPE::LAYERSUBTYPE_PASTE:
                selectLayerID( PCB_LAYER_ID::F_Paste, PCB_LAYER_ID::B_Paste, LOG_LEVEL::MSG );
                break;

            case LAYER_SUBTYPE::LAYERSUBTYPE_SILKSCREEN:
                selectLayerID( PCB_LAYER_ID::F_SilkS, PCB_LAYER_ID::B_SilkS, LOG_LEVEL::MSG );
                break;

            case LAYER_SUBTYPE::LAYERSUBTYPE_SOLDERRESIST:
                selectLayerID( PCB_LAYER_ID::F_Mask, PCB_LAYER_ID::B_Mask, LOG_LEVEL::MSG );
                break;

            case LAYER_SUBTYPE::LAYERSUBTYPE_ROUT:
            case LAYER_SUBTYPE::LAYERSUBTYPE_CLEARANCE:
                //Unsure what these layer types are used for. Map to Eco layers for now.
                selectLayerID( PCB_LAYER_ID::Eco1_User, PCB_LAYER_ID::Eco2_User, LOG_LEVEL::WARN );
                break;

            default:
                wxFAIL_MSG( "Unknown CADSTAR Layer Sub-type" );
                break;
            }
            break;

        default:
            wxFAIL_MSG( "Unknown CADSTAR Layer Type" );
            break;
        }

        mLayermap.insert( { curLayer.ID, kicadLayerID } );
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::remapUnsureLayers()
{
    LSET enabledLayers        = mBoard->GetEnabledLayers();
    LSET validRemappingLayers = enabledLayers    | LSET::AllBoardTechMask() |
                                LSET::UserMask() | LSET::UserDefinedLayers();

    std::vector<INPUT_LAYER_DESC> inputLayers;
    std::map<wxString, LAYER_ID>  cadstarLayerNameMap;

    for( std::pair<LAYER_ID, PCB_LAYER_ID> layerPair : mLayermap )
    {
        LAYER* curLayer = &Assignments.Layerdefs.Layers.at( layerPair.first );

        //Only remap layers that we aren't sure about
        if( curLayer->Type == LAYER_TYPE::DOC
                || ( curLayer->Type == LAYER_TYPE::NONELEC
                        && curLayer->SubType == LAYER_SUBTYPE::LAYERSUBTYPE_NONE )
                || ( curLayer->Type == LAYER_TYPE::NONELEC
                        && curLayer->SubType == LAYER_SUBTYPE::LAYERSUBTYPE_ROUT )
                || ( curLayer->Type == LAYER_TYPE::NONELEC
                        && curLayer->SubType == LAYER_SUBTYPE::LAYERSUBTYPE_CLEARANCE ) )
        {
            INPUT_LAYER_DESC iLdesc;
            iLdesc.Name            = curLayer->Name;
            iLdesc.PermittedLayers = validRemappingLayers;
            iLdesc.AutoMapLayer    = layerPair.second;

            inputLayers.push_back( iLdesc );
            cadstarLayerNameMap.insert( { curLayer->Name, curLayer->ID } );
        }
    }

    if( inputLayers.size() == 0 )
        return;

    // Callback:
    std::map<wxString, PCB_LAYER_ID> reMappedLayers = mLayerMappingHandler( inputLayers );

    for( std::pair<wxString, PCB_LAYER_ID> layerPair : reMappedLayers )
    {
        if( layerPair.second == PCB_LAYER_ID::UNDEFINED_LAYER )
        {
            wxFAIL_MSG( "Unexpected Layer ID" );
            continue;
        }

        LAYER_ID cadstarLayerID        = cadstarLayerNameMap.at( layerPair.first );
        mLayermap.at( cadstarLayerID ) = layerPair.second;
        enabledLayers |= LSET( layerPair.second );
    }

    mBoard->SetEnabledLayers( enabledLayers );
    mBoard->SetVisibleLayers( enabledLayers );
}


void CADSTAR_PCB_ARCHIVE_LOADER::loadDesignRules()
{
    BOARD_DESIGN_SETTINGS&                 ds           = mBoard->GetDesignSettings();
    std::map<SPACINGCODE_ID, SPACINGCODE>& spacingCodes = Assignments.Codedefs.SpacingCodes;

    auto applyRule =
            [&]( wxString aID, int* aVal )
            {
                if( spacingCodes.find( aID ) == spacingCodes.end() )
                    wxLogWarning( _( "Design rule %s was not found. This was ignored." ) );
                else
                    *aVal = getKiCadLength( spacingCodes.at( aID ).Spacing );
            };

    //Note: for details on the different spacing codes see SPACINGCODE::ID

    applyRule( "T_T", &ds.m_MinClearance );
    applyRule( "C_B", &ds.m_CopperEdgeClearance );
    applyRule( "H_H", &ds.m_HoleToHoleMin );

    ds.m_TrackMinWidth = Assignments.Technology.MinRouteWidth;

    auto applyNetClassRule = [&]( wxString aID, ::NETCLASS* aNetClassPtr,
                                     void ( ::NETCLASS::*aFunc )( int ) ) {
        int value = -1;
        applyRule( aID, &value );

        if( value != -1 )
            ( aNetClassPtr->*aFunc )( value );
    };

    applyNetClassRule( "T_T", ds.GetDefault(), &::NETCLASS::SetClearance );

    mBoard->m_LegacyNetclassesLoaded = true;

    wxLogWarning( _( "KiCad design rules are different from CADSTAR ones. Only the compatible "
                     "design rules were imported. It is recommended that you review the design "
                     "rules that have been applied." ) );
}


void CADSTAR_PCB_ARCHIVE_LOADER::loadComponentLibrary()
{
    for( std::pair<SYMDEF_ID, SYMDEF_PCB> symPair : Library.ComponentDefinitions )
    {
        SYMDEF_ID  key = symPair.first;
        SYMDEF_PCB component = symPair.second;
        wxString   fpName = component.ReferenceName + ( ( component.Alternate.size() > 0 ) ?
                                              ( wxT( " (" ) + component.Alternate + wxT( ")" ) ) :
                                              wxT( "" ) );
        FOOTPRINT* footprint = new FOOTPRINT( mBoard );
        footprint->SetPosition( getKiCadPoint( component.Origin ) );

        LIB_ID libID;
        libID.Parse( fpName, true );

        footprint->SetFPID( libID );
        loadLibraryFigures( component, footprint );
        loadLibraryCoppers( component, footprint );
        loadLibraryAreas( component, footprint );
        loadLibraryPads( component, footprint );

        mLibraryMap.insert( std::make_pair( key, footprint ) );
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::loadLibraryFigures( const SYMDEF_PCB& aComponent,
                                                     FOOTPRINT* aFootprint )
{
    for( std::pair<FIGURE_ID, FIGURE> figPair : aComponent.Figures )
    {
        FIGURE& fig = figPair.second;

        drawCadstarShape( fig.Shape, getKiCadLayer( fig.LayerID ),
                          getLineThickness( fig.LineCodeID ),
                          wxString::Format( "Component %s:%s -> Figure %s",
                                            aComponent.ReferenceName,
                                            aComponent.Alternate,
                                            fig.ID ),
                          aFootprint );
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::loadLibraryCoppers( const SYMDEF_PCB& aComponent, FOOTPRINT* aFootprint )
{
    for( COMPONENT_COPPER compCopper : aComponent.ComponentCoppers )
    {
        int lineThickness = getKiCadLength( getCopperCode( compCopper.CopperCodeID ).CopperWidth );

        drawCadstarShape( compCopper.Shape, getKiCadLayer( compCopper.LayerID ), lineThickness,
                          wxString::Format( "Component %s:%s -> Copper element",
                                            aComponent.ReferenceName,
                                            aComponent.Alternate ),
                          aFootprint );
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::loadLibraryAreas( const SYMDEF_PCB& aComponent,
                                                   FOOTPRINT* aFootprint )
{
    for( std::pair<COMP_AREA_ID, COMPONENT_AREA> areaPair : aComponent.ComponentAreas )
    {
        COMPONENT_AREA& area = areaPair.second;

        if( area.NoVias || area.NoTracks )
        {
            ZONE* zone = getZoneFromCadstarShape( area.Shape, getLineThickness( area.LineCodeID ),
                                                  aFootprint );

            aFootprint->Add( zone, ADD_MODE::APPEND );

            if( isLayerSet( area.LayerID ) )
                zone->SetLayerSet( getKiCadLayerSet( area.LayerID ) );
            else
                zone->SetLayer( getKiCadLayer( area.LayerID ) );

            zone->SetIsRuleArea( true );      //import all CADSTAR areas as Keepout zones
            zone->SetDoNotAllowPads( false ); //no CADSTAR equivalent
            zone->SetZoneName( area.ID );

            //There is no distinction between tracks and copper pours in CADSTAR Keepout zones
            zone->SetDoNotAllowTracks( area.NoTracks );
            zone->SetDoNotAllowCopperPour( area.NoTracks );

            zone->SetDoNotAllowVias( area.NoVias );
        }
        else
        {
            wxString libName = aComponent.ReferenceName;

            if( !aComponent.Alternate.IsEmpty() )
                libName << wxT( " (" ) << aComponent.Alternate << wxT( ")" );

            wxLogError(
                    wxString::Format( _( "The CADSTAR area '%s' in library component '%s' does not "
                                         "have a KiCad equivalent. The area is neither a via or"
                                         "route keepout area. The area was not imported." ),
                            area.ID, libName ) );
        }
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::loadLibraryPads( const SYMDEF_PCB& aComponent,
                                                  FOOTPRINT* aFootprint )
{
    for( std::pair<PAD_ID, COMPONENT_PAD> padPair : aComponent.ComponentPads )
    {
        PAD* pad = getKiCadPad( padPair.second, aFootprint );
        aFootprint->Add( pad, ADD_MODE::INSERT ); // insert so that we get correct behaviour
                                                  // when finding pads by PAD_ID - see loadNets()
    }
}


PAD* CADSTAR_PCB_ARCHIVE_LOADER::getKiCadPad( const COMPONENT_PAD& aCadstarPad, FOOTPRINT* aParent )
{
    PADCODE csPadcode = getPadCode( aCadstarPad.PadCodeID );

    PAD* pad = new PAD( aParent );

    switch( aCadstarPad.Side )
    {
    case PAD_SIDE::MAXIMUM: //Bottom side
        pad->SetAttribute( PAD_ATTR_T::PAD_ATTRIB_SMD );
        pad->SetLayerSet( LSET( 3, B_Cu, B_Paste, B_Mask ) );
        break;

    case PAD_SIDE::MINIMUM: //TOP side
        pad->SetAttribute( PAD_ATTR_T::PAD_ATTRIB_SMD );
        pad->SetLayerSet( LSET( 3, F_Cu, F_Paste, F_Mask ) );
        break;

    case PAD_SIDE::THROUGH_HOLE:

        if( csPadcode.Plated )
            pad->SetAttribute( PAD_ATTR_T::PAD_ATTRIB_PTH );
        else
            pad->SetAttribute( PAD_ATTR_T::PAD_ATTRIB_NPTH );

        pad->SetLayerSet( pad->PTHMask() ); // for now we will assume no paste layers
        //TODO: We need to read the csPad->Reassigns vector to make sure no paste
        break;

    default:
        wxFAIL_MSG( "Unknown Pad type" );
    }

    pad->SetName( aCadstarPad.Identifier.IsEmpty() ?
                          wxString::Format( wxT( "%ld" ), aCadstarPad.ID ) :
                          aCadstarPad.Identifier );

    if( csPadcode.Shape.Size == 0 )
        // zero sized pads seems to break KiCad so lets make it very small instead
        csPadcode.Shape.Size = 1;

    wxPoint padOffset = { 0, 0 }; // offset of the pad origin (before rotating)
    wxPoint drillOffset = { 0, 0 }; // offset of the drill origin w.r.t. the pad (before rotating)

    switch( csPadcode.Shape.ShapeType )
    {
    case PAD_SHAPE_TYPE::ANNULUS:
        //todo fix: use custom shape instead (Donught shape, i.e. a circle with a hole)
        pad->SetShape( PAD_SHAPE_T::PAD_SHAPE_CIRCLE );
        pad->SetSize( { getKiCadLength( csPadcode.Shape.Size ),
                getKiCadLength( csPadcode.Shape.Size ) } );
        break;

    case PAD_SHAPE_TYPE::BULLET:
        pad->SetShape( PAD_SHAPE_T::PAD_SHAPE_CHAMFERED_RECT );
        pad->SetSize( { getKiCadLength( (long long) csPadcode.Shape.Size
                                        + (long long) csPadcode.Shape.LeftLength
                                        + (long long) csPadcode.Shape.RightLength ),
                getKiCadLength( csPadcode.Shape.Size ) } );
        pad->SetChamferPositions( RECT_CHAMFER_POSITIONS::RECT_CHAMFER_BOTTOM_LEFT
                                  | RECT_CHAMFER_POSITIONS::RECT_CHAMFER_TOP_LEFT );
        pad->SetRoundRectRadiusRatio( 0.5 );
        pad->SetChamferRectRatio( 0.0 );

        padOffset.x = getKiCadLength( ( (long long) csPadcode.Shape.LeftLength / 2 ) -
                                      ( (long long) csPadcode.Shape.RightLength / 2 ) );
        break;

    case PAD_SHAPE_TYPE::CIRCLE:
        pad->SetShape( PAD_SHAPE_T::PAD_SHAPE_CIRCLE );
        pad->SetSize( { getKiCadLength( csPadcode.Shape.Size ),
                getKiCadLength( csPadcode.Shape.Size ) } );
        break;

    case PAD_SHAPE_TYPE::DIAMOND:
    {
        // Cadstar diamond shape is a square rotated 45 degrees
        // We convert it in KiCad to a square with chamfered edges
        int sizeOfSquare = (double) getKiCadLength( csPadcode.Shape.Size ) * sqrt(2.0);
        pad->SetShape( PAD_SHAPE_T::PAD_SHAPE_RECT );
        pad->SetChamferRectRatio( 0.5 );
        pad->SetSize( { sizeOfSquare, sizeOfSquare } );

        padOffset.x = getKiCadLength( ( (long long) csPadcode.Shape.LeftLength / 2 ) -
                                      ( (long long) csPadcode.Shape.RightLength / 2 ) );
    }
        break;

    case PAD_SHAPE_TYPE::FINGER:
        pad->SetShape( PAD_SHAPE_T::PAD_SHAPE_OVAL );
        pad->SetSize( { getKiCadLength( (long long) csPadcode.Shape.Size
                                        + (long long) csPadcode.Shape.LeftLength
                                        + (long long) csPadcode.Shape.RightLength ),
                getKiCadLength( csPadcode.Shape.Size ) } );

        padOffset.x = getKiCadLength( ( (long long) csPadcode.Shape.LeftLength / 2 ) -
                                      ( (long long) csPadcode.Shape.RightLength / 2 ) );
        break;

    case PAD_SHAPE_TYPE::OCTAGON:
        pad->SetShape( PAD_SHAPE_CHAMFERED_RECT );
        pad->SetChamferPositions( RECT_CHAMFER_POSITIONS::RECT_CHAMFER_ALL );
        pad->SetChamferRectRatio( 0.25 );
        pad->SetSize( { getKiCadLength( csPadcode.Shape.Size ),
                getKiCadLength( csPadcode.Shape.Size ) } );
        break;

    case PAD_SHAPE_TYPE::RECTANGLE:
        pad->SetShape( PAD_SHAPE_T::PAD_SHAPE_RECT );
        pad->SetSize( { getKiCadLength( (long long) csPadcode.Shape.Size
                                        + (long long) csPadcode.Shape.LeftLength
                                        + (long long) csPadcode.Shape.RightLength ),
                getKiCadLength( csPadcode.Shape.Size ) } );

        padOffset.x = getKiCadLength( ( (long long) csPadcode.Shape.LeftLength / 2 ) -
                                      ( (long long) csPadcode.Shape.RightLength / 2 ) );
        break;

    case PAD_SHAPE_TYPE::ROUNDED_RECT:
        pad->SetShape( PAD_SHAPE_T::PAD_SHAPE_RECT );
        pad->SetRoundRectCornerRadius( getKiCadLength( csPadcode.Shape.InternalFeature ) );
        pad->SetSize( { getKiCadLength( (long long) csPadcode.Shape.Size
                                        + (long long) csPadcode.Shape.LeftLength
                                        + (long long) csPadcode.Shape.RightLength ),
                getKiCadLength( csPadcode.Shape.Size ) } );

        padOffset.x = getKiCadLength( ( (long long) csPadcode.Shape.LeftLength / 2 ) -
                                      ( (long long) csPadcode.Shape.RightLength / 2 ) );
        break;


    case PAD_SHAPE_TYPE::SQUARE:
        pad->SetShape( PAD_SHAPE_T::PAD_SHAPE_RECT );
        pad->SetSize( { getKiCadLength( csPadcode.Shape.Size ),
                getKiCadLength( csPadcode.Shape.Size ) } );
        break;

    default:
        wxFAIL_MSG( "Unknown Pad Shape" );
    }

    if( csPadcode.ReliefClearance != UNDEFINED_VALUE )
        pad->SetThermalGap( getKiCadLength( csPadcode.ReliefClearance ) );

    if( csPadcode.ReliefWidth != UNDEFINED_VALUE )
        pad->SetThermalSpokeWidth( getKiCadLength( csPadcode.ReliefWidth ) );


    if( csPadcode.DrillDiameter != UNDEFINED_VALUE )
    {
        if( csPadcode.SlotLength != UNDEFINED_VALUE )
        {
            pad->SetDrillShape( PAD_DRILL_SHAPE_T::PAD_DRILL_SHAPE_OBLONG );
            pad->SetDrillSize( { getKiCadLength( (long long) csPadcode.SlotLength +
                                                 (long long) csPadcode.DrillDiameter ),
                                 getKiCadLength( csPadcode.DrillDiameter ) } );
        }
        else
        {
            pad->SetDrillShape( PAD_DRILL_SHAPE_T::PAD_DRILL_SHAPE_CIRCLE );
            pad->SetDrillSize( { getKiCadLength( csPadcode.DrillDiameter ),
                    getKiCadLength( csPadcode.DrillDiameter ) } );
        }

        drillOffset.x = -getKiCadLength( csPadcode.DrillXoffset );
        drillOffset.y = getKiCadLength( csPadcode.DrillYoffset );
    }


    if( csPadcode.SlotOrientation != 0 )
    {
        LSET lset = pad->GetLayerSet();
        lset &= LSET::AllCuMask();

        if( lset.size() > 0 )
        {
            SHAPE_POLY_SET padOutline;
            PCB_LAYER_ID   layer = lset.Seq().at( 0 );
            int            maxError = mBoard->GetDesignSettings().m_MaxError;

            pad->SetPosition( { 0, 0 } );
            pad->SetPos0( { 0, 0 } );
            pad->TransformShapeWithClearanceToPolygon( padOutline, layer, 0, maxError,
                                                       ERROR_LOC::ERROR_INSIDE );

            PCB_SHAPE* padShape = new PCB_SHAPE;
            padShape->SetShape( S_POLYGON );
            padShape->SetFilled( true );
            padShape->SetPolyShape( padOutline );
            padShape->SetWidth( 0 );
            padShape->Move( padOffset - drillOffset );
            padShape->Rotate( wxPoint( 0, 0 ), 1800.0 - getAngleTenthDegree( csPadcode.SlotOrientation ) );


            SHAPE_POLY_SET editedPadOutline = padShape->GetPolyShape();

            if( editedPadOutline.Contains( { 0, 0 } ) )
            {
                pad->SetAnchorPadShape( PAD_SHAPE_T::PAD_SHAPE_RECT );
                pad->SetSize( wxSize( { 4, 4 } ) );
                pad->SetShape( PAD_SHAPE_T::PAD_SHAPE_CUSTOM );
                pad->AddPrimitive( padShape );
                padOffset   = { 0, 0 };
            }
            else
            {
                // The CADSTAR pad has the hole shape outside the pad shape
                // Lets just put the hole in the center of the pad instead
                csPadcode.SlotOrientation = 0;
                drillOffset               = { 0, 0 };

                if( mPadcodesTested.find( csPadcode.ID ) == mPadcodesTested.end() )
                {
                    wxLogError( wxString::Format(
                            _( "The CADSTAR pad definition '%s' has the hole shape outside the "
                               "pad shape. The hole has been moved to the center of the pad." ),
                            csPadcode.Name ) );

                    mPadcodesTested.insert( csPadcode.ID );
                }
            }

        }
        else
        {
            wxFAIL_MSG( "No copper layers defined in the pad?" );
            csPadcode.SlotOrientation = 0;
            pad->SetOffset( drillOffset );
        }
    }
    else
    {
        pad->SetOffset( drillOffset );
    }

    double padOrientation = getAngleTenthDegree( aCadstarPad.OrientAngle )
                            + getAngleTenthDegree( csPadcode.Shape.OrientAngle );

    RotatePoint( &padOffset, padOrientation );
    RotatePoint( &drillOffset, padOrientation );
    pad->SetPos0( getKiCadPoint( aCadstarPad.Position ) - aParent->GetPosition() - padOffset
                  - drillOffset );
    pad->SetOrientation( padOrientation + getAngleTenthDegree( csPadcode.SlotOrientation ) );

    //TODO handle csPadcode.Reassigns when KiCad supports full padstacks

    return pad;
}


void CADSTAR_PCB_ARCHIVE_LOADER::loadGroups()
{
    for( std::pair<GROUP_ID, GROUP> groupPair : Layout.Groups )
    {
        GROUP& csGroup = groupPair.second;

        PCB_GROUP* kiGroup = new PCB_GROUP( mBoard );

        mBoard->Add( kiGroup );
        kiGroup->SetName( csGroup.Name );
        kiGroup->SetLocked( csGroup.Fixed );

        mGroupMap.insert( { csGroup.ID, kiGroup } );
    }

    //now add any groups to their parent group
    for( std::pair<GROUP_ID, GROUP> groupPair : Layout.Groups )
    {
        GROUP& csGroup = groupPair.second;

        if( !csGroup.GroupID.IsEmpty() )
        {
            if( mGroupMap.find( csGroup.ID ) == mGroupMap.end() )
            {
                THROW_IO_ERROR( wxString::Format(
                        _( "The file appears to be corrupt. Unable to find group ID %s "
                           "in the group definitions." ),
                        csGroup.ID ) );
            }
            else if( mGroupMap.find( csGroup.ID ) == mGroupMap.end() )
            {
                THROW_IO_ERROR( wxString::Format(
                        _( "The file appears to be corrupt. Unable to find sub group %s "
                           "in the group map (parent group ID=%s, Name=%s)." ),
                        csGroup.GroupID, csGroup.ID, csGroup.Name ) );
            }
            else
            {
                PCB_GROUP* kiCadGroup  = mGroupMap.at( csGroup.ID );
                PCB_GROUP* parentGroup = mGroupMap.at( csGroup.GroupID );
                parentGroup->AddItem( kiCadGroup );
            }
        }
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::loadBoards()
{
    for( std::pair<BOARD_ID, BOARD> boardPair : Layout.Boards )
    {
        BOARD&   board      = boardPair.second;
        GROUP_ID boardGroup = createUniqueGroupID( wxT( "Board" ) );
        drawCadstarShape( board.Shape, PCB_LAYER_ID::Edge_Cuts,
                getLineThickness( board.LineCodeID ), wxString::Format( "BOARD %s", board.ID ),
                mBoard, boardGroup );

        if( !board.GroupID.IsEmpty() )
        {
            addToGroup( board.GroupID, getKiCadGroup( boardGroup ) );
        }

        //TODO process board attributes when KiCad supports them
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::loadFigures()
{
    for( std::pair<FIGURE_ID, FIGURE> figPair : Layout.Figures )
    {
        FIGURE& fig = figPair.second;
        drawCadstarShape( fig.Shape, getKiCadLayer( fig.LayerID ),
                getLineThickness( fig.LineCodeID ), wxString::Format( "FIGURE %s", fig.ID ), mBoard,
                fig.GroupID );

        //TODO process "swaprule" (doesn't seem to apply to Layout Figures?)
        //TODO process re-use block when KiCad Supports it
        //TODO process attributes when KiCad Supports attributes in figures
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::loadTexts()
{
    for( std::pair<TEXT_ID, TEXT> txtPair : Layout.Texts )
    {
        TEXT& csTxt = txtPair.second;
        drawCadstarText( csTxt, mBoard );
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::loadDimensions()
{
    for( std::pair<DIMENSION_ID, DIMENSION> dimPair : Layout.Dimensions )
    {
        DIMENSION& csDim = dimPair.second;

        switch( csDim.Type )
        {
        case DIMENSION::TYPE::LINEARDIM:
            switch( csDim.Subtype )
            {
            case DIMENSION::SUBTYPE::ANGLED:
                wxLogWarning( wxString::Format( _( "Dimension ID %s is an angled dimension, which "
                                                   "has no KiCad equivalent. An aligned dimension "
                                                   "was loaded instead." ),
                                                csDim.ID ) );
                KI_FALLTHROUGH;
            case DIMENSION::SUBTYPE::DIRECT:
            case DIMENSION::SUBTYPE::ORTHOGONAL:
            {
                if( csDim.Line.Style == DIMENSION::LINE::STYLE::EXTERNAL )
                {
                    wxLogWarning( wxString::Format(
                            _( "Dimension ID %s has 'External' style in CADSTAR. External "
                               "dimension styles are not yet supported in KiCad. The dimension "
                               "object was imported with an internal dimension style instead." ),
                            csDim.ID ) );
                }

                ::ALIGNED_DIMENSION* dimension = nullptr;

                if( csDim.Subtype == DIMENSION::SUBTYPE::ORTHOGONAL )
                {
                    dimension = new ::ORTHOGONAL_DIMENSION( mBoard );
                    ORTHOGONAL_DIMENSION* orDim = static_cast<ORTHOGONAL_DIMENSION*>( dimension );

                    if( csDim.ExtensionLineParams.Start.x == csDim.Line.Start.x )
                        orDim->SetOrientation( ORTHOGONAL_DIMENSION::DIR::HORIZONTAL );
                    else
                        orDim->SetOrientation( ORTHOGONAL_DIMENSION::DIR::VERTICAL );
                }
                else
                {
                    dimension = new ::ALIGNED_DIMENSION( mBoard );
                }

                mBoard->Add( dimension, ADD_MODE::APPEND );
                applyDimensionSettings( csDim, dimension );

                dimension->SetExtensionHeight(
                        getKiCadLength( csDim.ExtensionLineParams.Overshoot ) );

                // Calculate height:
                wxPoint  crossbarStart = getKiCadPoint( csDim.Line.Start );
                wxPoint  crossbarEnd = getKiCadPoint( csDim.Line.End );
                VECTOR2I crossbarVector = crossbarEnd - crossbarStart;
                VECTOR2I heightVector = crossbarStart - dimension->GetStart();
                double   height = 0.0;

                if( csDim.Subtype == DIMENSION::SUBTYPE::ORTHOGONAL )
                {
                    if( csDim.ExtensionLineParams.Start.x == csDim.Line.Start.x )
                        height = heightVector.y;
                    else
                        height = heightVector.x;
                }
                else
                {
                    double angle = crossbarVector.Angle() + ( M_PI / 2 );
                    height = heightVector.x * cos( angle ) + heightVector.y * sin( angle );
                }

                dimension->SetHeight( height );
            }
            break;

            default:
                // Radius and diameter dimensions are LEADERDIM (even if not actually leader)
                // Angular dimensions are always ANGLEDIM
                wxLogError( wxString::Format(
                        _( "Unexpected Dimension type (ID %s). This was not imported" ),
                        csDim.ID ) );
                continue;
            }
            break;

        case DIMENSION::TYPE::LEADERDIM:
            //TODO: update import when KiCad supports radius and diameter dimensions

            if( csDim.Line.Style == DIMENSION::LINE::STYLE::INTERNAL )
            {
                // "internal" is a simple double sided arrow from start to end (no extension lines)
                ::ALIGNED_DIMENSION* dimension = new ::ALIGNED_DIMENSION( mBoard );
                mBoard->Add( dimension, ADD_MODE::APPEND );
                applyDimensionSettings( csDim, dimension );

                // Lets set again start/end:
                dimension->SetStart( getKiCadPoint( csDim.Line.Start ) );
                dimension->SetEnd( getKiCadPoint( csDim.Line.End ) );

                // Do not use any extension lines:
                dimension->SetExtensionOffset( 0 );
                dimension->SetExtensionHeight( 0 );
                dimension->SetHeight( 0 );
            }
            else
            {
                // "external" is a "leader" style dimension
                ::LEADER* leaderDim = new ::LEADER( mBoard );
                mBoard->Add( leaderDim, ADD_MODE::APPEND );

                applyDimensionSettings( csDim, leaderDim );
                leaderDim->SetStart( getKiCadPoint( csDim.Line.End ) );

                /*
                 * In CADSTAR, the resulting shape orientation of the leader dimension depends on
                 * on the positions of the #Start (S) and #End (E) points as shown below. In the
                 * diagrams below, the leader angle (angRad) is represented by HEV
                 *
                 * Orientation 1: (orientX = -1,  |     Orientation 2: (orientX = 1,
                 *                 orientY = 1)   |                     orientY = 1)
                 *                                |
                 * --------V                      |               V----------
                 *          \                     |              /
                 *           \                    |             /
                 * H         _E/                  |           \E_           H
                 *                                |
                 *                     S          |     S
                 *                                |
                 *
                 * Orientation 3: (orientX = -1,  |     Orientation 4: (orientX = 1,
                 *                 orientY = -1)  |                     orientY = -1)
                 *                                |
                 *                     S          |     S
                 *             _                  |            _
                 *  H           E\                |          /E             H
                 *             /                  |            \
                 *            /                   |             \
                 * ----------V                    |              V-----------
                 *                                |
                 *
                 * Corner cases:
                 *
                 * It is not possible to generate a leader object with start and end point being
                 * identical. Assume Orientation 2 if start and end points are identical.
                 *
                 * If start and end points are aligned vertically (i.e. S.x == E.x):
                 * - If E.y > S.y - Orientation 2
                 * - If E.y < S.y - Orientation 4
                 *
                 * If start and end points are aligned horitontally (i.e. S.y == E.y):
                 * - If E.x > S.x - Orientation 2
                 * - If E.x < S.x - Orientation 1
                 */
                double  angRad = DEG2RAD( getAngleDegrees( csDim.Line.LeaderAngle ) );

                double orientX = 1;
                double orientY = 1;

                if( csDim.Line.End.x >= csDim.Line.Start.x )
                {
                    if( csDim.Line.End.y >= csDim.Line.Start.y )
                    {
                        //Orientation 2
                        orientX = 1;
                        orientY = 1;
                    }
                    else
                    {
                        //Orientation 4
                        orientX = 1;
                        orientY = -1;
                    }
                }
                else
                {
                    if( csDim.Line.End.y >= csDim.Line.Start.y )
                    {
                        //Orientation 1
                        orientX = -1;
                        orientY = 1;
                    }
                    else
                    {
                        //Orientation 3
                        orientX = -1;
                        orientY = -1;
                    }
                }

                wxPoint endOffset( csDim.Line.LeaderLineLength * cos( angRad ) * orientX,
                                   csDim.Line.LeaderLineLength * sin( angRad ) * orientY );

                wxPoint endPoint = csDim.Line.End + endOffset;
                wxPoint txtPoint( endPoint.x + ( csDim.Line.LeaderLineExtensionLength * orientX ),
                                  endPoint.y );

                leaderDim->SetEnd( getKiCadPoint( endPoint ) );
                leaderDim->Text().SetTextPos( getKiCadPoint( txtPoint ) );
                leaderDim->SetText( ParseTextFields( csDim.Text.Text, &mContext ) );
                leaderDim->SetPrefix( wxEmptyString );
                leaderDim->SetSuffix( wxEmptyString );
                leaderDim->SetUnitsFormat( DIM_UNITS_FORMAT::NO_SUFFIX );

                if( orientX == 1 )
                    leaderDim->Text().SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
                else
                    leaderDim->Text().SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );

                leaderDim->SetExtensionOffset( 0 );
            }
            break;

        case DIMENSION::TYPE::ANGLEDIM:
            //TODO: update import when KiCad supports angular dimensions
            wxLogError( wxString::Format(
                    _( "Dimension ID %s is an angular dimension which has no KiCad equivalent. "
                       "The object was not imported." ),
                    csDim.ID ) );
            break;
        }
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::loadAreas()
{
    for( std::pair<AREA_ID, AREA> areaPair : Layout.Areas )
    {
        AREA& area = areaPair.second;

        if( area.NoVias || area.NoTracks || area.Keepout || area.Routing )
        {
            ZONE* zone = getZoneFromCadstarShape( area.Shape, getLineThickness( area.LineCodeID ),
                                                  mBoard );

            mBoard->Add( zone, ADD_MODE::APPEND );

            if( isLayerSet( area.LayerID ) )
                zone->SetLayerSet( getKiCadLayerSet( area.LayerID ) );
            else
                zone->SetLayer( getKiCadLayer( area.LayerID ) );

            zone->SetIsRuleArea( true );      //import all CADSTAR areas as Keepout zones
            zone->SetDoNotAllowPads( false ); //no CADSTAR equivalent
            zone->SetZoneName( area.Name );

            zone->SetDoNotAllowFootprints( area.Keepout );

            zone->SetDoNotAllowTracks( area.NoTracks );
            zone->SetDoNotAllowCopperPour( area.NoTracks );

            zone->SetDoNotAllowVias( area.NoVias );

            if( area.Placement )
                wxLogWarning( wxString::Format(
                        _( "The CADSTAR area '%s' is marked as a placement area in CADSTAR. "
                           "Placement areas are not supported in KiCad. Only the supported "
                           "elements for the area were imported." ),
                        area.Name ) );
        }
        else
        {
            wxLogError(
                    wxString::Format( _( "The CADSTAR area '%s' does not have a KiCad equivalent. "
                                         "Pure Placement areas are not supported." ),
                            area.Name ) );
        }

        //todo Process area.AreaHeight when KiCad supports 3D design rules
        //TODO process attributes
        //TODO process addition to a group
        //TODO process "swaprule"
        //TODO process re-use block
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::loadComponents()
{
    for( std::pair<COMPONENT_ID, COMPONENT> compPair : Layout.Components )
    {
        COMPONENT& comp = compPair.second;

        if( !comp.VariantID.empty() && comp.VariantParentComponentID != comp.ID )
            continue; // Only load master Variant

        auto fpIter = mLibraryMap.find( comp.SymdefID );

        if( fpIter == mLibraryMap.end() )
        {
            THROW_IO_ERROR( wxString::Format( _( "Unable to find component '%s' in the library"
                                                 "(Symdef ID: '%s')" ),
                    comp.Name, comp.SymdefID ) );
        }

        FOOTPRINT* libFootprint = fpIter->second;

        // Use Duplicate() to ensure unique KIID for all objects
        FOOTPRINT* footprint = static_cast<FOOTPRINT*>( libFootprint->Duplicate() );

        mBoard->Add( footprint, ADD_MODE::APPEND );

        // First lets fix the pad names on the footprint.
        // CADSTAR defines the pad name in the PART definition and the SYMDEF (i.e. the PCB
        // footprint definition) uses a numerical sequence. COMP is the only object that has
        // visibility of both the SYMDEF and PART.
        if( Parts.PartDefinitions.find( comp.PartID ) != Parts.PartDefinitions.end() )
        {
            PART part = Parts.PartDefinitions.at( comp.PartID );

            // Only do this when the number of pins in the part definition equals the number of
            // pads in the footprint.
            if( part.Definition.Pins.size() == footprint->Pads().size() )
            {
                for( std::pair<PART_DEFINITION_PIN_ID, PART::DEFINITION::PIN> pinPair :
                        part.Definition.Pins )
                {
                    PART::DEFINITION::PIN pin = pinPair.second;
                    wxString              pinName = pin.Name;

                    if( pinName.empty() )
                        pinName = pin.Identifier;

                    if( pinName.empty() )
                        pinName = wxString::Format( wxT( "%ld" ), pin.ID );

                    footprint->Pads().at( pin.ID - (long long) 1 )->SetName( pinName );
                }
            }
        }

        //Override pads with pad exceptions
        if( comp.PadExceptions.size() > 0 )
        {
            SYMDEF_PCB fpLibEntry = Library.ComponentDefinitions.at( comp.SymdefID );

            for( std::pair<PAD_ID, PADEXCEPTION> padPair : comp.PadExceptions )
            {
                PADEXCEPTION& padEx = padPair.second;
                COMPONENT_PAD csPad = fpLibEntry.ComponentPads.at( padPair.first );

                if( !padEx.PadCode.IsEmpty() )
                    csPad.PadCodeID = padEx.PadCode;

                if( padEx.OverrideExits )
                    csPad.Exits = padEx.Exits;

                if( padEx.OverrideOrientation )
                    csPad.OrientAngle = padEx.OrientAngle;

                if( padEx.OverrideSide )
                    csPad.Side = padEx.Side;

                // Find the pad in the footprint definition
                PAD* kiPad = footprint->Pads().at( padEx.ID - (long long) 1 );
                wxString padName = kiPad->GetName();

                if( kiPad )
                    delete kiPad;

                kiPad = getKiCadPad( csPad, footprint );
                kiPad->SetName( padName );
                footprint->Pads().at( padEx.ID - (long long) 1 ) = kiPad;
            }
        }

        //set to empty string to avoid duplication when loading attributes:
        footprint->SetValue( wxEmptyString );

        footprint->SetPosition( getKiCadPoint( comp.Origin ) );
        footprint->SetOrientation( getAngleTenthDegree( comp.OrientAngle ) );
        footprint->SetReference( comp.Name );

        if( comp.Mirror )
        {
            double mirroredAngle = - getAngleTenthDegree( comp.OrientAngle );
            NORMALIZE_ANGLE_180( mirroredAngle );
            footprint->SetOrientation( mirroredAngle );
            footprint->Flip( getKiCadPoint( comp.Origin ), true );
        }

        loadComponentAttributes( comp, footprint );

        if( !comp.PartID.IsEmpty() && comp.PartID != wxT( "NO_PART" ) )
            footprint->SetDescription( getPart( comp.PartID ).Definition.Name );

        mComponentMap.insert( { comp.ID, footprint } );
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::loadDocumentationSymbols()
{
    //No KiCad equivalent. Loaded as graphic and text elements instead

    for( std::pair<DOCUMENTATION_SYMBOL_ID, DOCUMENTATION_SYMBOL> docPair :
            Layout.DocumentationSymbols )
    {
        DOCUMENTATION_SYMBOL& docSymInstance = docPair.second;


        auto docSymIter = Library.ComponentDefinitions.find( docSymInstance.SymdefID );

        if( docSymIter == Library.ComponentDefinitions.end() )
        {
            THROW_IO_ERROR( wxString::Format( _( "Unable to find documentation symbol in the "
                                                 "library (Symdef ID: '%s')" ),
                    docSymInstance.SymdefID ) );
        }

        SYMDEF_PCB& docSymDefinition = ( *docSymIter ).second;
        wxPoint     moveVector =
                getKiCadPoint( docSymInstance.Origin ) - getKiCadPoint( docSymDefinition.Origin );
        double rotationAngle = getAngleTenthDegree( docSymInstance.OrientAngle );
        double scalingFactor = (double) docSymInstance.ScaleRatioNumerator
                               / (double) docSymInstance.ScaleRatioDenominator;
        wxPoint centreOfTransform = getKiCadPoint( docSymDefinition.Origin );
        bool    mirrorInvert      = docSymInstance.Mirror;

        //create a group to store the items in
        wxString groupName = docSymDefinition.ReferenceName;

        if( !docSymDefinition.Alternate.IsEmpty() )
            groupName += wxT( " (" ) + docSymDefinition.Alternate + wxT( ")" );

        GROUP_ID groupID = createUniqueGroupID( groupName );

        LSEQ layers = getKiCadLayerSet( docSymInstance.LayerID ).Seq();

        for( PCB_LAYER_ID layer : layers )
        {
            for( std::pair<FIGURE_ID, FIGURE> figPair : docSymDefinition.Figures )
            {
                FIGURE fig = figPair.second;
                drawCadstarShape( fig.Shape, layer, getLineThickness( fig.LineCodeID ),
                        wxString::Format( "DOCUMENTATION SYMBOL %s, FIGURE %s",
                                docSymDefinition.ReferenceName, fig.ID ),
                        mBoard, groupID, moveVector, rotationAngle, scalingFactor,
                        centreOfTransform, mirrorInvert );
            }
        }

        for( std::pair<TEXT_ID, TEXT> textPair : docSymDefinition.Texts )
        {
            TEXT txt = textPair.second;
            drawCadstarText( txt, mBoard, groupID, docSymInstance.LayerID, moveVector,
                    rotationAngle, scalingFactor, centreOfTransform, mirrorInvert );
        }
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::loadTemplates()
{
    for( std::pair<TEMPLATE_ID, TEMPLATE> tempPair : Layout.Templates )
    {
        TEMPLATE& csTemplate = tempPair.second;

        int zonelinethickness = 0; // The line thickness in CADSTAR is only for display purposes but
                                   // does not affect the end copper result.
        ZONE* zone = getZoneFromCadstarShape( csTemplate.Shape, zonelinethickness, mBoard );

        mBoard->Add( zone, ADD_MODE::APPEND );

        zone->SetZoneName( csTemplate.Name );
        zone->SetLayer( getKiCadLayer( csTemplate.LayerID ) );
        zone->SetPriority( 1 ); // initially 1, we will increase in calculateZonePriorities

        if( !( csTemplate.NetID.IsEmpty() || csTemplate.NetID == wxT( "NONE" ) ) )
            zone->SetNet( getKiCadNet( csTemplate.NetID ) );

        if( csTemplate.Pouring.AllowInNoRouting )
            wxLogError( wxString::Format(
                    _( "The CADSTAR template '%s' has the setting 'Allow in No Routing Areas' "
                       "enabled. This setting has no KiCad equivalent, so it has been ignored." ),
                    csTemplate.Name ) );

        if( csTemplate.Pouring.BoxIsolatedPins )
            wxLogError( wxString::Format(
                    _( "The CADSTAR template '%s' has the setting 'Box Isolated Pins' "
                       "enabled. This setting has no KiCad equivalent, so it has been ignored." ),
                    csTemplate.Name ) );

        if( csTemplate.Pouring.AutomaticRepour )
            wxLogWarning( wxString::Format(
                    _( "The CADSTAR template '%s' has the setting 'Automatic Repour' "
                       "enabled. This setting has no KiCad equivalent, so it has been ignored." ),
                    csTemplate.Name ) );

        // Sliver width has different behaviour to KiCad Zone's minimum thickness
        // In Cadstar 'Sliver width' has to be greater than the Copper thickness, whereas in
        // Kicad it is the opposite.
        if( csTemplate.Pouring.SliverWidth != 0 )
            wxLogError( wxString::Format(
                    _( "The CADSTAR template '%s' has a non-zero value defined for the "
                       "'Sliver Width' setting. There is no KiCad equivalent for "
                       "this, so this setting was ignored." ),
                    csTemplate.Name ) );


        if( csTemplate.Pouring.MinIsolatedCopper != csTemplate.Pouring.MinDisjointCopper )
            wxLogError( wxString::Format(
                    _( "The CADSTAR template '%s' has different settings for 'Retain Poured Copper "
                       "- Disjoint' and 'Retain Poured Copper - Isolated'. KiCad does not "
                       "distinguish between these two settings. The setting for disjoint copper "
                       "has been applied as the minimum island area of the KiCad Zone." ),
                    csTemplate.Name ) );

        if( csTemplate.Pouring.MinDisjointCopper < 0 )
            zone->SetMinIslandArea( -1 );
        else
            zone->SetMinIslandArea(
                    (long long) getKiCadLength( csTemplate.Pouring.MinDisjointCopper )
                    * (long long) getKiCadLength( csTemplate.Pouring.MinDisjointCopper ) );

        zone->SetLocalClearance( getKiCadLength( csTemplate.Pouring.AdditionalIsolation ) );

        if( csTemplate.Pouring.FillType == TEMPLATE::POURING::COPPER_FILL_TYPE::HATCHED )
        {
            zone->SetFillMode( ZONE_FILL_MODE::HATCH_PATTERN );
            zone->SetHatchGap( getKiCadHatchCodeGap( csTemplate.Pouring.HatchCodeID ) );
            zone->SetHatchThickness( getKiCadHatchCodeThickness( csTemplate.Pouring.HatchCodeID ) );
            zone->SetHatchOrientation( getHatchCodeAngleDegrees( csTemplate.Pouring.HatchCodeID ) );
        }
        else
        {
            zone->SetFillMode( ZONE_FILL_MODE::POLYGONS );
        }

        if( csTemplate.Pouring.ThermalReliefOnPads != csTemplate.Pouring.ThermalReliefOnVias
                || csTemplate.Pouring.ThermalReliefPadsAngle
                           != csTemplate.Pouring.ThermalReliefViasAngle )
            wxLogWarning( wxString::Format(
                    _( "The CADSTAR template '%s' has different settings for thermal relief "
                       "in pads and vias. KiCad only supports one single setting for both. The "
                       "setting for pads has been applied." ),
                    csTemplate.Name ) );

        if( csTemplate.Pouring.ThermalReliefOnPads )
        {
            zone->SetThermalReliefGap( getKiCadLength( csTemplate.Pouring.ClearanceWidth ) );
            zone->SetThermalReliefSpokeWidth( getKiCadLength(
                    getCopperCode( csTemplate.Pouring.ReliefCopperCodeID ).CopperWidth ) );
            zone->SetPadConnection( ZONE_CONNECTION::THERMAL );
        }
        else
            zone->SetPadConnection( ZONE_CONNECTION::FULL );

        mLoadedTemplates.insert( { csTemplate.ID, zone } );
    }

    //Now create power plane layers:
    for( LAYER_ID layer : mPowerPlaneLayers )
    {
        wxASSERT(
                Assignments.Layerdefs.Layers.find( layer ) != Assignments.Layerdefs.Layers.end() );

        //The net name will equal the layer name
        wxString powerPlaneLayerName = Assignments.Layerdefs.Layers.at( layer ).Name;
        NET_ID   netid               = wxEmptyString;

        for( std::pair<NET_ID, NET_PCB> netPair : Layout.Nets )
        {
            NET_PCB net = netPair.second;

            if( net.Name == powerPlaneLayerName )
            {
                netid = net.ID;
                break;
            }
        }

        if( netid.IsEmpty() )
        {
            wxLogError( wxString::Format(
                    _( "The CADSTAR layer '%s' is defined as a power plane layer. However no "
                       "net with such name exists. The layer has been loaded but no copper zone "
                       "was created." ),
                    powerPlaneLayerName ) );
        }
        else
        {
            for( std::pair<BOARD_ID, BOARD> boardPair : Layout.Boards )
            {
                //create a zone in each board shape
                BOARD_DESIGN_SETTINGS& bds = mBoard->GetDesignSettings();
                BOARD& board = boardPair.second;
                int    defaultLineThicknesss = bds.GetLineThickness( PCB_LAYER_ID::Edge_Cuts );
                ZONE*  zone = getZoneFromCadstarShape( board.Shape, defaultLineThicknesss, mBoard );

                mBoard->Add( zone, ADD_MODE::APPEND );

                zone->SetZoneName( powerPlaneLayerName );
                zone->SetLayer( getKiCadLayer( layer ) );
                zone->SetFillMode( ZONE_FILL_MODE::POLYGONS );
                zone->SetPadConnection( ZONE_CONNECTION::FULL );
                zone->SetMinIslandArea( -1 );
                zone->SetPriority( 0 ); // Priority always 0 (lowest priority) for implied power planes.
                zone->SetNet( getKiCadNet( netid ) );
            }
        }
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::loadCoppers()
{
    for( std::pair<COPPER_ID, COPPER> copPair : Layout.Coppers )
    {
        COPPER& csCopper = copPair.second;

        if( !csCopper.PouredTemplateID.IsEmpty() )
        {
            ZONE* pouredZone = mLoadedTemplates.at( csCopper.PouredTemplateID );

            SHAPE_POLY_SET rawPolys = getPolySetFromCadstarShape( csCopper.Shape, -1 );

            int copperWidth = getKiCadLength( getCopperCode( csCopper.CopperCodeID ).CopperWidth );
            int zoneWidth = pouredZone->GetMinThickness();

            if( zoneWidth > copperWidth )
                rawPolys.Deflate( ( zoneWidth - copperWidth ) / 2, 32 );
            else
                rawPolys.Inflate( ( copperWidth - zoneWidth ) / 2, 32 );

            if( pouredZone->HasFilledPolysForLayer( getKiCadLayer( csCopper.LayerID ) ) )
                rawPolys.BooleanAdd( pouredZone->RawPolysList( getKiCadLayer( csCopper.LayerID )),
                                     SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );


            SHAPE_POLY_SET finalPolys = rawPolys;
            finalPolys.Fracture( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );

            pouredZone->SetRawPolysList( getKiCadLayer( csCopper.LayerID ), rawPolys );
            pouredZone->SetFilledPolysList( getKiCadLayer( csCopper.LayerID ), finalPolys );
            pouredZone->SetIsFilled( true );
            pouredZone->SetNeedRefill( false );
            continue;
        }

        // For now we are going to load coppers to a KiCad zone however this isn't perfect
        //TODO: Load onto a graphical polygon with a net (when KiCad has this feature)

        if( !mDoneCopperWarning )
        {
            wxLogWarning(
                    _( "The CADSTAR design contains COPPER elements, which have no direct KiCad "
                       "equivalent. These have been imported as a KiCad Zone if solid or hatch "
                       "filled, or as a KiCad Track if the shape was an unfilled outline (open or "
                       "closed)." ) );
            mDoneCopperWarning = true;
        }


        if( csCopper.Shape.Type == SHAPE_TYPE::OPENSHAPE
                || csCopper.Shape.Type == SHAPE_TYPE::OUTLINE )
        {
            std::vector<PCB_SHAPE*> outlineSegments =
                    getDrawSegmentsFromVertices( csCopper.Shape.Vertices );

            std::vector<TRACK*> outlineTracks = makeTracksFromDrawsegments( outlineSegments, mBoard,
                    getKiCadNet( csCopper.NetRef.NetID ), getKiCadLayer( csCopper.LayerID ),
                    getKiCadLength( getCopperCode( csCopper.CopperCodeID ).CopperWidth ) );

            //cleanup
            for( PCB_SHAPE* seg : outlineSegments )
                delete seg;

            for( CUTOUT cutout : csCopper.Shape.Cutouts )
            {
                std::vector<PCB_SHAPE*> cutoutSeg =
                        getDrawSegmentsFromVertices( cutout.Vertices );

                std::vector<TRACK*> cutoutTracks = makeTracksFromDrawsegments( cutoutSeg, mBoard,
                        getKiCadNet( csCopper.NetRef.NetID ), getKiCadLayer( csCopper.LayerID ),
                        getKiCadLength( getCopperCode( csCopper.CopperCodeID ).CopperWidth ) );

                //cleanup
                for( PCB_SHAPE* seg : cutoutSeg )
                    delete seg;
            }
        }
        else
        {
            ZONE* zone = getZoneFromCadstarShape( csCopper.Shape,
                                                  getKiCadLength( getCopperCode( csCopper.CopperCodeID ).CopperWidth ),
                                                  mBoard );

            mBoard->Add( zone, ADD_MODE::APPEND );

            zone->SetZoneName( csCopper.ID );
            zone->SetLayer( getKiCadLayer( csCopper.LayerID ) );

            if( csCopper.Shape.Type == SHAPE_TYPE::HATCHED )
            {
                zone->SetFillMode( ZONE_FILL_MODE::HATCH_PATTERN );
                zone->SetHatchGap( getKiCadHatchCodeGap( csCopper.Shape.HatchCodeID ) );
                zone->SetHatchThickness( getKiCadHatchCodeThickness( csCopper.Shape.HatchCodeID ) );
                zone->SetHatchOrientation( getHatchCodeAngleDegrees( csCopper.Shape.HatchCodeID ) );
            }
            else
            {
                zone->SetFillMode( ZONE_FILL_MODE::POLYGONS );
            }

            zone->SetPadConnection( ZONE_CONNECTION::FULL );
            zone->SetNet( getKiCadNet( csCopper.NetRef.NetID ) );
            zone->SetPriority( mLoadedTemplates.size() + 1 ); // Highest priority (always fill first)
            zone->SetRawPolysList( getKiCadLayer( csCopper.LayerID ), *zone->Outline() );

            SHAPE_POLY_SET fillePolys( *zone->Outline() );
            fillePolys.Fracture( SHAPE_POLY_SET::POLYGON_MODE::PM_STRICTLY_SIMPLE );

            zone->SetFilledPolysList( getKiCadLayer( csCopper.LayerID ), fillePolys );
        }
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::loadNets()
{
    for( std::pair<NET_ID, NET_PCB> netPair : Layout.Nets )
    {
        NET_PCB  net                      = netPair.second;
        wxString netnameForErrorReporting = net.Name;

        if( netnameForErrorReporting.IsEmpty() )
            netnameForErrorReporting = wxString::Format( "$%ld", net.SignalNum );

        for( NET_PCB::CONNECTION_PCB connection : net.Connections )
        {
            if( !connection.Unrouted )
                loadNetTracks( net.ID, connection.Route );

            //TODO: all other elements
        }

        for( std::pair<NETELEMENT_ID, NET_PCB::VIA> viaPair : net.Vias )
        {
            NET_PCB::VIA via = viaPair.second;
            loadNetVia( net.ID, via );
        }

        for( std::pair<NETELEMENT_ID, NET_PCB::PIN> pinPair : net.Pins )
        {
            NET_PCB::PIN pin = pinPair.second;
            FOOTPRINT*   footprint = getFootprintFromCadstarID( pin.ComponentID );

            if( footprint == nullptr )
            {
                wxLogWarning( wxString::Format(
                        _( "The net '%s' references component ID '%s' which does not exist. "
                           "This has been ignored." ),
                        netnameForErrorReporting, pin.ComponentID ) );
            }
            else if( ( pin.PadID - (long) 1 ) > footprint->Pads().size() )
            {
                wxLogWarning( wxString::Format( _( "The net '%s' references non-existent pad index"
                                                   " '%d' in component '%s'. This has been ignored." ),
                                                netnameForErrorReporting,
                                                pin.PadID,
                                                footprint->GetReference() ) );
            }
            else
            {
                // The below works because we have added the pads in the correct order to the
                // footprint and the PAD_ID in Cadstar is a sequential, numerical ID
                footprint->Pads().at( pin.PadID - (long) 1 )->SetNet( getKiCadNet( net.ID ) );
            }
        }
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::loadTextVariables()
{
    auto findAndReplaceTextField =
        [&]( TEXT_FIELD_NAME aField, wxString aValue )
        {
            if( mContext.TextFieldToValuesMap.find( aField ) !=
                mContext.TextFieldToValuesMap.end() )
            {
                if( mContext.TextFieldToValuesMap.at( aField ) != aValue )
                {
                    mContext.TextFieldToValuesMap.at( aField ) = aValue;
                    mContext.InconsistentTextFields.insert( aField );
                    return false;
                }
            }
            else
            {
                mContext.TextFieldToValuesMap.insert( { aField, aValue } );
            }

            return true;
        };

    if( mProject )
    {
        std::map<wxString, wxString>& txtVars = mProject->GetTextVars();

        // Most of the design text fields can be derived from other elements
        if( Layout.VariantHierarchy.Variants.size() > 0 )
        {
            VARIANT loadedVar = Layout.VariantHierarchy.Variants.begin()->second;

            findAndReplaceTextField( TEXT_FIELD_NAME::VARIANT_NAME, loadedVar.Name );
            findAndReplaceTextField( TEXT_FIELD_NAME::VARIANT_DESCRIPTION, loadedVar.Description );
        }

        findAndReplaceTextField( TEXT_FIELD_NAME::DESIGN_TITLE, Header.JobTitle );

        for( std::pair<TEXT_FIELD_NAME, wxString> txtvalue : mContext.TextFieldToValuesMap )
        {
            wxString varName  = CadstarToKicadFieldsMap.at( txtvalue.first );
            wxString varValue = txtvalue.second;

            txtVars.insert( { varName, varValue } );
        }

        for( std::pair<wxString, wxString> txtvalue : mContext.FilenamesToTextMap )
        {
            wxString varName  = txtvalue.first;
            wxString varValue = txtvalue.second;

            txtVars.insert( { varName, varValue } );
        }
    }
    else
    {
        wxLogError( _( "Text Variables could not be set as there is no project loaded." ) );
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::loadComponentAttributes( const COMPONENT& aComponent,
                                                          FOOTPRINT* aFootprint )
{
    for( std::pair<ATTRIBUTE_ID, ATTRIBUTE_VALUE> attrPair : aComponent.AttributeValues )
    {
        ATTRIBUTE_VALUE& attrval = attrPair.second;

        if( attrval.HasLocation ) //only import attributes with location. Ignore the rest
        {
            addAttribute( attrval.AttributeLocation, attrval.AttributeID, aFootprint,
                          attrval.Value );
        }
    }

    for( std::pair<ATTRIBUTE_ID, TEXT_LOCATION> textlocPair : aComponent.TextLocations )
    {
        TEXT_LOCATION& textloc = textlocPair.second;
        wxString       attrval;

        if( textloc.AttributeID == COMPONENT_NAME_ATTRID )
        {
            attrval = wxEmptyString; // Designator is loaded separately
        }
        else if( textloc.AttributeID == COMPONENT_NAME_2_ATTRID )
        {
            attrval = wxT( "${REFERENCE}" );
        }
        else if( textloc.AttributeID == PART_NAME_ATTRID )
        {
            attrval = getPart( aComponent.PartID ).Name;
        }
        else
            attrval = getAttributeValue( textloc.AttributeID, aComponent.AttributeValues );

        addAttribute( textloc, textloc.AttributeID, aFootprint, attrval );
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::loadNetTracks(
        const NET_ID& aCadstarNetID, const NET_PCB::ROUTE& aCadstarRoute )
{
    std::vector<PCB_SHAPE*> shapes;

    POINT prevEnd = aCadstarRoute.StartPoint;

    for( const NET_PCB::ROUTE_VERTEX& v : aCadstarRoute.RouteVertices )
    {
        PCB_SHAPE* shape = getDrawSegmentFromVertex( prevEnd, v.Vertex );
        shape->SetLayer( getKiCadLayer( aCadstarRoute.LayerID ) );
        shape->SetWidth( getKiCadLength( v.RouteWidth ) );
        shapes.push_back( shape );
        prevEnd = v.Vertex.End;
    }

    //Todo add real netcode to the tracks
    std::vector<TRACK*> tracks =
            makeTracksFromDrawsegments( shapes, mBoard, getKiCadNet( aCadstarNetID ) );

    //cleanup
    for( PCB_SHAPE* shape : shapes )
        delete shape;
}


void CADSTAR_PCB_ARCHIVE_LOADER::loadNetVia(
        const NET_ID& aCadstarNetID, const NET_PCB::VIA& aCadstarVia )
{
    VIA* via = new VIA( mBoard );
    mBoard->Add( via, ADD_MODE::APPEND );

    VIACODE   csViaCode   = getViaCode( aCadstarVia.ViaCodeID );
    LAYERPAIR csLayerPair = getLayerPair( aCadstarVia.LayerPairID );

    via->SetPosition( getKiCadPoint( aCadstarVia.Location ) );
    via->SetDrill( getKiCadLength( csViaCode.DrillDiameter ) );
    via->SetLocked( aCadstarVia.Fixed );

    if( csViaCode.Shape.ShapeType != PAD_SHAPE_TYPE::CIRCLE )
        wxLogError( wxString::Format(
                _( "The CADSTAR via code '%s' has different shape from a circle defined. "
                   "KiCad only supports circular vias so this via type has been changed to "
                   "be a via with circular shape of %.2f mm diameter." ),
                csViaCode.Name,
                (double) ( (double) getKiCadLength( csViaCode.Shape.Size ) / 1E6 ) ) );

    via->SetWidth( getKiCadLength( csViaCode.Shape.Size ) );

    bool start_layer_outside =
            csLayerPair.PhysicalLayerStart == 1
            || csLayerPair.PhysicalLayerStart == Assignments.Technology.MaxPhysicalLayer;
    bool end_layer_outside =
            csLayerPair.PhysicalLayerEnd == 1
            || csLayerPair.PhysicalLayerEnd == Assignments.Technology.MaxPhysicalLayer;

    if( start_layer_outside && end_layer_outside )
    {
        via->SetViaType( VIATYPE::THROUGH );
    }
    else if( ( !start_layer_outside ) && ( !end_layer_outside ) )
    {
        via->SetViaType( VIATYPE::BLIND_BURIED );
    }
    else
    {
        via->SetViaType( VIATYPE::MICROVIA );
    }

    via->SetLayerPair( getKiCadCopperLayerID( csLayerPair.PhysicalLayerStart ),
            getKiCadCopperLayerID( csLayerPair.PhysicalLayerEnd ) );
    via->SetNet( getKiCadNet( aCadstarNetID ) );
    ///todo add netcode to the via
}


void CADSTAR_PCB_ARCHIVE_LOADER::drawCadstarText( const TEXT& aCadstarText,
        BOARD_ITEM_CONTAINER* aContainer, const GROUP_ID& aCadstarGroupID,
        const LAYER_ID& aCadstarLayerOverride, const wxPoint& aMoveVector,
        const double& aRotationAngle, const double& aScalingFactor, const wxPoint& aTransformCentre,
        const bool& aMirrorInvert )
{
    PCB_TEXT* txt = new PCB_TEXT( aContainer );
    aContainer->Add( txt );
    txt->SetText( aCadstarText.Text );

    wxPoint rotatedTextPos = getKiCadPoint( aCadstarText.Position );
    RotatePoint( &rotatedTextPos, aTransformCentre, aRotationAngle );
    rotatedTextPos.x =
            KiROUND( (double) ( rotatedTextPos.x - aTransformCentre.x ) * aScalingFactor );
    rotatedTextPos.y =
            KiROUND( (double) ( rotatedTextPos.y - aTransformCentre.y ) * aScalingFactor );
    rotatedTextPos += aTransformCentre;
    txt->SetTextPos( rotatedTextPos );
    txt->SetPosition( rotatedTextPos );

    txt->SetTextAngle( getAngleTenthDegree( aCadstarText.OrientAngle ) + aRotationAngle );

    if( aCadstarText.Mirror != aMirrorInvert ) // If mirroring, invert angle to match CADSTAR
        txt->SetTextAngle( -txt->GetTextAngle() );

    txt->SetMirrored( aCadstarText.Mirror );

    TEXTCODE tc = getTextCode( aCadstarText.TextCodeID );

    txt->SetTextThickness( getKiCadLength( tc.LineWidth ) );

    wxSize unscaledTextSize;
    unscaledTextSize.x = getKiCadLength( tc.Width );

    // The width is zero for all non-cadstar fonts. Using a width equal to the height seems
    // to work well for most fonts.
    if( unscaledTextSize.x == 0 )
        unscaledTextSize.x = getKiCadLength( tc.Height );

    unscaledTextSize.y = KiROUND( TXT_HEIGHT_RATIO * (double) getKiCadLength( tc.Height ) );
    txt->SetTextSize( unscaledTextSize );

    switch( aCadstarText.Alignment )
    {
    case ALIGNMENT::NO_ALIGNMENT: // Default for Single line text is Bottom Left
    case ALIGNMENT::BOTTOMLEFT:
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        break;

    case ALIGNMENT::BOTTOMCENTER:
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
        break;

    case ALIGNMENT::BOTTOMRIGHT:
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        break;

    case ALIGNMENT::CENTERLEFT:
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        break;

    case ALIGNMENT::CENTERCENTER:
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
        break;

    case ALIGNMENT::CENTERRIGHT:
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        break;

    case ALIGNMENT::TOPLEFT:
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        break;

    case ALIGNMENT::TOPCENTER:
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
        break;

    case ALIGNMENT::TOPRIGHT:
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        break;

    default:
        wxFAIL_MSG( "Unknown Aligment - needs review!" );
    }

    if( aMirrorInvert )
    {
        txt->Flip( aTransformCentre, true );
    }

    //scale it after flipping:
    if( aScalingFactor != 1.0 )
    {
        wxSize scaledTextSize;
        scaledTextSize.x = KiROUND( (double) unscaledTextSize.x * aScalingFactor );
        scaledTextSize.y = KiROUND( (double) unscaledTextSize.y * aScalingFactor );
        txt->SetTextSize( scaledTextSize );
        txt->SetTextThickness(
                KiROUND( (double) getKiCadLength( tc.LineWidth ) * aScalingFactor ) );
    }

    txt->Move( aMoveVector );

    if( aCadstarText.Alignment == ALIGNMENT::NO_ALIGNMENT )
        FixTextPositionNoAlignment( txt );

    LAYER_ID layersToDrawOn = aCadstarLayerOverride;

    if( layersToDrawOn.IsEmpty() )
        layersToDrawOn = aCadstarText.LayerID;

    if( isLayerSet( layersToDrawOn ) )
    {
        //Make a copy on each layer

        LSEQ      layers = getKiCadLayerSet( layersToDrawOn ).Seq();
        PCB_TEXT* newtxt;

        for( PCB_LAYER_ID layer : layers )
        {
            txt->SetLayer( layer );
            newtxt = static_cast<PCB_TEXT*>( txt->Duplicate() );
            mBoard->Add( newtxt, ADD_MODE::APPEND );

            if( !aCadstarGroupID.IsEmpty() )
                addToGroup( aCadstarGroupID, newtxt );
        }

        mBoard->Remove( txt );
        delete txt;
    }
    else
    {
        txt->SetLayer( getKiCadLayer( layersToDrawOn ) );

        if( !aCadstarGroupID.IsEmpty() )
            addToGroup( aCadstarGroupID, txt );
    }
    //TODO Handle different font types when KiCad can support it.
}


void CADSTAR_PCB_ARCHIVE_LOADER::drawCadstarShape( const SHAPE& aCadstarShape,
                                                   const PCB_LAYER_ID& aKiCadLayer,
                                                   const int& aLineThickness,
                                                   const wxString& aShapeName,
                                                   BOARD_ITEM_CONTAINER* aContainer,
                                                   const GROUP_ID& aCadstarGroupID,
                                                   const wxPoint& aMoveVector,
                                                   const double& aRotationAngle,
                                                   const double& aScalingFactor,
                                                   const wxPoint& aTransformCentre,
                                                   const bool& aMirrorInvert )
{
    switch( aCadstarShape.Type )
    {
    case SHAPE_TYPE::OPENSHAPE:
    case SHAPE_TYPE::OUTLINE:
        ///TODO update this when Polygons in KiCad can be defined with no fill
        drawCadstarVerticesAsSegments( aCadstarShape.Vertices, aKiCadLayer, aLineThickness,
                aContainer, aCadstarGroupID, aMoveVector, aRotationAngle, aScalingFactor,
                aTransformCentre, aMirrorInvert );
        drawCadstarCutoutsAsSegments( aCadstarShape.Cutouts, aKiCadLayer, aLineThickness,
                aContainer, aCadstarGroupID, aMoveVector, aRotationAngle, aScalingFactor,
                aTransformCentre, aMirrorInvert );
        break;

    case SHAPE_TYPE::HATCHED:
        ///TODO update this when Polygons in KiCad can be defined with hatch fill
        wxLogWarning( wxString::Format(
                _( "The shape for '%s' is Hatch filled in CADSTAR, which has no KiCad equivalent. "
                   "Using solid fill instead." ),
                aShapeName ) );

    case SHAPE_TYPE::SOLID:
    {
        PCB_SHAPE* shape;

        if( isFootprint( aContainer ) )
        {
            shape = new FP_SHAPE( (FOOTPRINT*) aContainer, S_POLYGON );
        }
        else
        {
            shape = new PCB_SHAPE( aContainer );
            shape->SetShape( S_POLYGON );
        }

        shape->SetFilled( true );

        SHAPE_POLY_SET shapePolys = getPolySetFromCadstarShape(
                aCadstarShape, -1, aContainer, aMoveVector, aRotationAngle, aScalingFactor,
                aTransformCentre, aMirrorInvert );

        shapePolys.Fracture( SHAPE_POLY_SET::POLYGON_MODE::PM_STRICTLY_SIMPLE );

        shape->SetPolyShape( shapePolys );
        shape->SetWidth( aLineThickness );
        shape->SetLayer( aKiCadLayer );
        aContainer->Add( shape, ADD_MODE::APPEND );

        if( !aCadstarGroupID.IsEmpty() )
            addToGroup( aCadstarGroupID, shape );
    }
    break;
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::drawCadstarCutoutsAsSegments( const std::vector<CUTOUT>& aCutouts,
                                                               const PCB_LAYER_ID& aKiCadLayer,
                                                               const int& aLineThickness,
                                                               BOARD_ITEM_CONTAINER* aContainer,
                                                               const GROUP_ID& aCadstarGroupID,
                                                               const wxPoint& aMoveVector,
                                                               const double& aRotationAngle,
                                                               const double& aScalingFactor,
                                                               const wxPoint& aTransformCentre,
                                                               const bool& aMirrorInvert )
{
    for( CUTOUT cutout : aCutouts )
    {
        drawCadstarVerticesAsSegments( cutout.Vertices, aKiCadLayer, aLineThickness, aContainer,
                                       aCadstarGroupID, aMoveVector, aRotationAngle, aScalingFactor,
                                       aTransformCentre, aMirrorInvert );
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::drawCadstarVerticesAsSegments(
        const std::vector<VERTEX>& aCadstarVertices, const PCB_LAYER_ID& aKiCadLayer,
        const int& aLineThickness, BOARD_ITEM_CONTAINER* aContainer,
        const GROUP_ID& aCadstarGroupID, const wxPoint& aMoveVector, const double& aRotationAngle,
        const double& aScalingFactor, const wxPoint& aTransformCentre, const bool& aMirrorInvert )
{
    std::vector<PCB_SHAPE*> drawSegments =
            getDrawSegmentsFromVertices( aCadstarVertices, aContainer, aCadstarGroupID, aMoveVector,
                    aRotationAngle, aScalingFactor, aTransformCentre, aMirrorInvert );

    for( PCB_SHAPE* ds : drawSegments )
    {
        ds->SetWidth( aLineThickness );
        ds->SetLayer( aKiCadLayer );
        ds->SetParent( aContainer );
        aContainer->Add( ds, ADD_MODE::APPEND );
    }
}


std::vector<PCB_SHAPE*> CADSTAR_PCB_ARCHIVE_LOADER::getDrawSegmentsFromVertices(
        const std::vector<VERTEX>& aCadstarVertices, BOARD_ITEM_CONTAINER* aContainer,
        const GROUP_ID& aCadstarGroupID, const wxPoint& aMoveVector, const double& aRotationAngle,
        const double& aScalingFactor, const wxPoint& aTransformCentre, const bool& aMirrorInvert )
{
    std::vector<PCB_SHAPE*> drawSegments;

    if( aCadstarVertices.size() < 2 )
        //need at least two points to draw a segment! (unlikely but possible to have only one)
        return drawSegments;

    const VERTEX* prev = &aCadstarVertices.at( 0 ); // first one should always be a point vertex
    const VERTEX* cur;

    for( size_t i = 1; i < aCadstarVertices.size(); i++ )
    {
        cur = &aCadstarVertices.at( i );
        drawSegments.push_back(
                getDrawSegmentFromVertex( prev->End, *cur, aContainer, aCadstarGroupID, aMoveVector,
                        aRotationAngle, aScalingFactor, aTransformCentre, aMirrorInvert ) );
        prev = cur;
    }

    return drawSegments;
}


PCB_SHAPE* CADSTAR_PCB_ARCHIVE_LOADER::getDrawSegmentFromVertex( const POINT& aCadstarStartPoint,
                                                                 const VERTEX& aCadstarVertex,
                                                                 BOARD_ITEM_CONTAINER* aContainer,
                                                                 const GROUP_ID& aCadstarGroupID,
                                                                 const wxPoint& aMoveVector,
                                                                 const double& aRotationAngle,
                                                                 const double& aScalingFactor,
                                                                 const wxPoint& aTransformCentre,
                                                                 const bool& aMirrorInvert )
{
    PCB_SHAPE* ds = nullptr;
    bool       cw = false;
    double     arcStartAngle, arcEndAngle, arcAngle;

    wxPoint startPoint = getKiCadPoint( aCadstarStartPoint );
    wxPoint endPoint   = getKiCadPoint( aCadstarVertex.End );
    wxPoint centerPoint;

    if( aCadstarVertex.Type == VERTEX_TYPE::ANTICLOCKWISE_SEMICIRCLE
            || aCadstarVertex.Type == VERTEX_TYPE::CLOCKWISE_SEMICIRCLE )
        centerPoint = ( startPoint + endPoint ) / 2;
    else
        centerPoint = getKiCadPoint( aCadstarVertex.Center );

    switch( aCadstarVertex.Type )
    {

    case VERTEX_TYPE::POINT:

        if( isFootprint( aContainer ) )
        {
            ds = new FP_SHAPE( static_cast<FOOTPRINT*>( aContainer ), S_SEGMENT );
        }
        else
        {
            ds = new PCB_SHAPE( aContainer );
            ds->SetShape( S_SEGMENT );
        }

        ds->SetStart( startPoint );
        ds->SetEnd( endPoint );
        break;

    case VERTEX_TYPE::CLOCKWISE_SEMICIRCLE:
    case VERTEX_TYPE::CLOCKWISE_ARC:
        cw = true;
        KI_FALLTHROUGH;

    case VERTEX_TYPE::ANTICLOCKWISE_SEMICIRCLE:
    case VERTEX_TYPE::ANTICLOCKWISE_ARC:

        if( isFootprint( aContainer ) )
        {
            ds = new FP_SHAPE((FOOTPRINT*) aContainer, S_ARC );
        }
        else
        {
            ds = new PCB_SHAPE( aContainer );
            ds->SetShape( S_ARC );
        }

        ds->SetArcStart( startPoint );
        ds->SetCenter( centerPoint );

        arcStartAngle = getPolarAngle( startPoint - centerPoint );
        arcEndAngle   = getPolarAngle( endPoint - centerPoint );
        arcAngle      = arcEndAngle - arcStartAngle;
        //TODO: detect if we are supposed to draw a circle instead (i.e. two SEMICIRCLEs
        // with opposite start/end points and same centre point)

        if( cw )
            ds->SetAngle( NormalizeAnglePos( arcAngle ) );
        else
            ds->SetAngle( NormalizeAngleNeg( arcAngle ) );

        break;
    }

    //Apply transforms
    if( aMirrorInvert )
        ds->Flip( aTransformCentre, true );

    if( aScalingFactor != 1.0 )
    {
        ds->Move( -aTransformCentre );
        ds->Scale( aScalingFactor );
        ds->Move( aTransformCentre );
    }

    if( aRotationAngle != 0.0 )
        ds->Rotate( aTransformCentre, aRotationAngle );

    if( aMoveVector != wxPoint{ 0, 0 } )
        ds->Move( aMoveVector );

    if( isFootprint( aContainer ) && ds != nullptr )
        static_cast<FP_SHAPE*>( ds )->SetLocalCoord();

    if( !aCadstarGroupID.IsEmpty() )
        addToGroup( aCadstarGroupID, ds );

    return ds;
}


ZONE* CADSTAR_PCB_ARCHIVE_LOADER::getZoneFromCadstarShape( const SHAPE& aCadstarShape,
                                                           const int& aLineThickness,
                                                           BOARD_ITEM_CONTAINER* aParentContainer )
{
    ZONE* zone = new ZONE( aParentContainer, isFootprint( aParentContainer ) );

    if( aCadstarShape.Type == SHAPE_TYPE::HATCHED )
    {
        zone->SetFillMode( ZONE_FILL_MODE::HATCH_PATTERN );
        zone->SetHatchStyle( ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_FULL );
    }
    else
    {
        zone->SetHatchStyle( ZONE_BORDER_DISPLAY_STYLE::NO_HATCH );
    }

    SHAPE_POLY_SET polygon = getPolySetFromCadstarShape( aCadstarShape, aLineThickness );

    zone->AddPolygon( polygon.COutline( 0 ) );

    for( int i = 0; i < polygon.HoleCount( 0 ); i++ )
        zone->AddPolygon( polygon.CHole( 0, i ) );

    return zone;
}


SHAPE_POLY_SET CADSTAR_PCB_ARCHIVE_LOADER::getPolySetFromCadstarShape( const SHAPE& aCadstarShape,
                                                                       const int& aLineThickness,
                                                                       BOARD_ITEM_CONTAINER* aContainer,
                                                                       const wxPoint& aMoveVector,
                                                                       const double& aRotationAngle,
                                                                       const double& aScalingFactor,
                                                                       const wxPoint& aTransformCentre,
                                                                       const bool& aMirrorInvert )
{
    GROUP_ID noGroup = wxEmptyString;

    std::vector<PCB_SHAPE*> outlineSegments = getDrawSegmentsFromVertices( aCadstarShape.Vertices,
                                                                           aContainer, noGroup,
                                                                           aMoveVector,
                                                                           aRotationAngle,
                                                                           aScalingFactor,
                                                                           aTransformCentre,
                                                                           aMirrorInvert );

    SHAPE_POLY_SET polySet( getLineChainFromDrawsegments( outlineSegments ) );

    //cleanup
    for( PCB_SHAPE* seg : outlineSegments )
            delete seg;

    for( CUTOUT cutout : aCadstarShape.Cutouts )
    {
        std::vector<PCB_SHAPE*> cutoutSeg = getDrawSegmentsFromVertices( cutout.Vertices,
                                                                         aContainer, noGroup,
                                                                         aMoveVector,
                                                                         aRotationAngle,
                                                                         aScalingFactor,
                                                                         aTransformCentre,
                                                                         aMirrorInvert );

        polySet.AddHole( getLineChainFromDrawsegments( cutoutSeg ) );

        //cleanup
        for( PCB_SHAPE* seg : cutoutSeg )
            delete seg;
    }

    if( aLineThickness > 0 )
        polySet.Inflate( aLineThickness / 2, 32, SHAPE_POLY_SET::CORNER_STRATEGY::ROUND_ALL_CORNERS );

#ifdef DEBUG
    for( int i = 0; i < polySet.OutlineCount(); ++i )
    {
        wxASSERT( polySet.Outline( i ).PointCount() > 2 );

        for( int j = 0; j < polySet.HoleCount( i ); ++j )
        {
            wxASSERT( polySet.Hole( i, j ).PointCount() > 2 );
        }
    }
#endif

    return polySet;
}


SHAPE_LINE_CHAIN CADSTAR_PCB_ARCHIVE_LOADER::getLineChainFromDrawsegments( const std::vector<PCB_SHAPE*> aDrawsegments )
{
    SHAPE_LINE_CHAIN lineChain;

    for( PCB_SHAPE* ds : aDrawsegments )
    {
        switch( ds->GetShape() )
        {
        case S_ARC:
        {
            if( ds->GetClass() == wxT( "MGRAPHIC" ) )
            {
                FP_SHAPE* em = (FP_SHAPE*) ds;
                SHAPE_ARC arc( em->GetStart0(), em->GetEnd0(), (double) em->GetAngle() / 10.0 );
                lineChain.Append( arc );
            }
            else
            {
                SHAPE_ARC arc( ds->GetCenter(), ds->GetArcStart(), (double) ds->GetAngle() / 10.0 );
                lineChain.Append( arc );
            }
        }
        break;
        case S_SEGMENT:
            if( ds->GetClass() == wxT( "MGRAPHIC" ) )
            {
                FP_SHAPE* em = (FP_SHAPE*) ds;
                lineChain.Append( em->GetStart0().x, em->GetStart0().y );
                lineChain.Append( em->GetEnd0().x, em->GetEnd0().y );
            }
            else
            {
                lineChain.Append( ds->GetStartX(), ds->GetStartY() );
                lineChain.Append( ds->GetEndX(), ds->GetEndY() );
            }
            break;

        default:
            wxFAIL_MSG( "Drawsegment type is unexpected. Ignored." );
        }
    }

    lineChain.SetClosed( true ); //todo check if it is closed

    wxASSERT( lineChain.PointCount() > 2 );

    return lineChain;
}


std::vector<TRACK*> CADSTAR_PCB_ARCHIVE_LOADER::makeTracksFromDrawsegments(
                                                  const std::vector<PCB_SHAPE*> aDrawsegments,
                                                  BOARD_ITEM_CONTAINER* aParentContainer,
                                                  NETINFO_ITEM* aNet, PCB_LAYER_ID aLayerOverride,
                                                  int aWidthOverride )
{
    std::vector<TRACK*> tracks;

    for( PCB_SHAPE* ds : aDrawsegments )
    {
        TRACK* track;

        switch( ds->GetShape() )
        {
        case S_ARC:
            if( ds->GetClass() == wxT( "MGRAPHIC" ) )
            {
                FP_SHAPE* em = (FP_SHAPE*) ds;
                SHAPE_ARC arc( em->GetStart0(), em->GetEnd0(), (double) em->GetAngle() / 10.0 );
                track = new ARC( aParentContainer, &arc );
            }
            else
            {
                SHAPE_ARC arc( ds->GetCenter(), ds->GetArcStart(), (double) ds->GetAngle() / 10.0 );
                track = new ARC( aParentContainer, &arc );
            }
            break;
        case S_SEGMENT:
            if( ds->GetClass() == wxT( "MGRAPHIC" ) )
            {
                FP_SHAPE* em = (FP_SHAPE*) ds;
                track = new TRACK( aParentContainer );
                track->SetStart( em->GetStart0() );
                track->SetEnd( em->GetEnd0() );
            }
            else
            {
                track = new TRACK( aParentContainer );
                track->SetStart( ds->GetStart() );
                track->SetEnd( ds->GetEnd() );
            }
            break;

        default:
            wxFAIL_MSG( "Drawsegment type is unexpected. Ignored." );
            continue;
        }

        if( aWidthOverride == -1 )
            track->SetWidth( ds->GetWidth() );
        else
            track->SetWidth( aWidthOverride );

        if( aLayerOverride == PCB_LAYER_ID::UNDEFINED_LAYER )
            track->SetLayer( ds->GetLayer() );
        else
            track->SetLayer( aLayerOverride );

        if( aNet != nullptr )
            track->SetNet( aNet );

        tracks.push_back( track );
        aParentContainer->Add( track, ADD_MODE::APPEND );
    }

    return tracks;
}


void CADSTAR_PCB_ARCHIVE_LOADER::addAttribute( const ATTRIBUTE_LOCATION& aCadstarAttrLoc,
                                               const ATTRIBUTE_ID& aCadstarAttributeID,
                                               FOOTPRINT* aFootprint,
                                               const wxString& aAttributeValue )
{
    FP_TEXT* txt;

    if( aCadstarAttributeID == COMPONENT_NAME_ATTRID )
    {
        txt = &aFootprint->Reference(); //text should be set outside this function
    }
    else if( aCadstarAttributeID == PART_NAME_ATTRID )
    {
        if( aFootprint->Value().GetText().IsEmpty() )
        {
            // Use PART_NAME_ATTRID as the value is value field is blank
            aFootprint->SetValue( aAttributeValue );
            txt = &aFootprint->Value();
        }
        else
        {
            txt = new FP_TEXT( aFootprint );
            aFootprint->Add( txt );
            txt->SetText( aAttributeValue );
        }
        txt->SetVisible( false ); //make invisible to avoid clutter.
    }
    else if( aCadstarAttributeID != COMPONENT_NAME_2_ATTRID
             && getAttributeName( aCadstarAttributeID ) == wxT( "Value" ) )
    {
        if( !aFootprint->Value().GetText().IsEmpty() )
        {
            //copy the object
            aFootprint->Add( aFootprint->Value().Duplicate() );
        }

        aFootprint->SetValue( aAttributeValue );
        txt = &aFootprint->Value();
        txt->SetVisible( false ); //make invisible to avoid clutter.
    }
    else
    {
        txt = new FP_TEXT( aFootprint );
        aFootprint->Add( txt );
        txt->SetText( aAttributeValue );
        txt->SetVisible( false ); //make all user attributes invisible to avoid clutter.
        //TODO: Future improvement - allow user to decide what to do with attributes
    }

    wxPoint rotatedTextPos = getKiCadPoint( aCadstarAttrLoc.Position ) - aFootprint->GetPosition();
    RotatePoint( &rotatedTextPos, -aFootprint->GetOrientation() );

    txt->SetTextPos( getKiCadPoint( aCadstarAttrLoc.Position ) );
    txt->SetPos0( rotatedTextPos );
    txt->SetLayer( getKiCadLayer( aCadstarAttrLoc.LayerID ) );
    txt->SetMirrored( aCadstarAttrLoc.Mirror );
    txt->SetTextAngle(
            getAngleTenthDegree( aCadstarAttrLoc.OrientAngle ) - aFootprint->GetOrientation() );

    if( aCadstarAttrLoc.Mirror ) // If mirroring, invert angle to match CADSTAR
        txt->SetTextAngle( -txt->GetTextAngle() );

    TEXTCODE tc = getTextCode( aCadstarAttrLoc.TextCodeID );

    txt->SetTextThickness( getKiCadLength( tc.LineWidth ) );

    wxSize txtSize;
    txtSize.x = getKiCadLength( tc.Width );

    // The width is zero for all non-cadstar fonts. Using a width equal to the height seems
    // to work well for most fonts.
    if( txtSize.x == 0 )
        txtSize.x = getKiCadLength( tc.Height );

    txtSize.y = KiROUND( TXT_HEIGHT_RATIO * (double) getKiCadLength( tc.Height ) );
    txt->SetTextSize( txtSize );
    txt->SetKeepUpright( false ); //Keeping it upright seems to result in incorrect orientation

    switch( aCadstarAttrLoc.Alignment )
    {
    case ALIGNMENT::NO_ALIGNMENT: // Default for Single line text is Bottom Left
        FixTextPositionNoAlignment( txt );
        KI_FALLTHROUGH;
    case ALIGNMENT::BOTTOMLEFT:
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        break;

    case ALIGNMENT::BOTTOMCENTER:
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
        break;

    case ALIGNMENT::BOTTOMRIGHT:
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        break;

    case ALIGNMENT::CENTERLEFT:
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        break;

    case ALIGNMENT::CENTERCENTER:
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
        break;

    case ALIGNMENT::CENTERRIGHT:
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        break;

    case ALIGNMENT::TOPLEFT:
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        break;

    case ALIGNMENT::TOPCENTER:
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
        break;

    case ALIGNMENT::TOPRIGHT:
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        break;

    default:
        wxFAIL_MSG( "Unknown Aligment - needs review!" );
    }

    //TODO Handle different font types when KiCad can support it.
}


int CADSTAR_PCB_ARCHIVE_LOADER::getLineThickness( const LINECODE_ID& aCadstarLineCodeID )
{
    wxCHECK( Assignments.Codedefs.LineCodes.find( aCadstarLineCodeID )
                     != Assignments.Codedefs.LineCodes.end(),
            mBoard->GetDesignSettings().GetLineThickness( PCB_LAYER_ID::Edge_Cuts ) );

    return getKiCadLength( Assignments.Codedefs.LineCodes.at( aCadstarLineCodeID ).Width );
}


CADSTAR_PCB_ARCHIVE_LOADER::COPPERCODE CADSTAR_PCB_ARCHIVE_LOADER::getCopperCode(
        const COPPERCODE_ID& aCadstaCopperCodeID )
{
    wxCHECK( Assignments.Codedefs.CopperCodes.find( aCadstaCopperCodeID )
                     != Assignments.Codedefs.CopperCodes.end(),
            COPPERCODE() );

    return Assignments.Codedefs.CopperCodes.at( aCadstaCopperCodeID );
}


CADSTAR_PCB_ARCHIVE_LOADER::TEXTCODE CADSTAR_PCB_ARCHIVE_LOADER::getTextCode(
        const TEXTCODE_ID& aCadstarTextCodeID )
{
    wxCHECK( Assignments.Codedefs.TextCodes.find( aCadstarTextCodeID )
                     != Assignments.Codedefs.TextCodes.end(),
            TEXTCODE() );

    return Assignments.Codedefs.TextCodes.at( aCadstarTextCodeID );
}


CADSTAR_PCB_ARCHIVE_LOADER::PADCODE CADSTAR_PCB_ARCHIVE_LOADER::getPadCode(
        const PADCODE_ID& aCadstarPadCodeID )
{
    wxCHECK( Assignments.Codedefs.PadCodes.find( aCadstarPadCodeID )
                     != Assignments.Codedefs.PadCodes.end(),
            PADCODE() );

    return Assignments.Codedefs.PadCodes.at( aCadstarPadCodeID );
}


CADSTAR_PCB_ARCHIVE_LOADER::VIACODE CADSTAR_PCB_ARCHIVE_LOADER::getViaCode(
        const VIACODE_ID& aCadstarViaCodeID )
{
    wxCHECK( Assignments.Codedefs.ViaCodes.find( aCadstarViaCodeID )
                     != Assignments.Codedefs.ViaCodes.end(),
            VIACODE() );

    return Assignments.Codedefs.ViaCodes.at( aCadstarViaCodeID );
}


CADSTAR_PCB_ARCHIVE_LOADER::LAYERPAIR CADSTAR_PCB_ARCHIVE_LOADER::getLayerPair(
        const LAYERPAIR_ID& aCadstarLayerPairID )
{
    wxCHECK( Assignments.Codedefs.LayerPairs.find( aCadstarLayerPairID )
                     != Assignments.Codedefs.LayerPairs.end(),
            LAYERPAIR() );

    return Assignments.Codedefs.LayerPairs.at( aCadstarLayerPairID );
}


wxString CADSTAR_PCB_ARCHIVE_LOADER::getAttributeName( const ATTRIBUTE_ID& aCadstarAttributeID )
{
    wxCHECK( Assignments.Codedefs.AttributeNames.find( aCadstarAttributeID )
                     != Assignments.Codedefs.AttributeNames.end(),
            wxEmptyString );

    return Assignments.Codedefs.AttributeNames.at( aCadstarAttributeID ).Name;
}


wxString CADSTAR_PCB_ARCHIVE_LOADER::getAttributeValue( const ATTRIBUTE_ID& aCadstarAttributeID,
        const std::map<ATTRIBUTE_ID, ATTRIBUTE_VALUE>&                      aCadstarAttributeMap )
{
    wxCHECK( aCadstarAttributeMap.find( aCadstarAttributeID ) != aCadstarAttributeMap.end(),
            wxEmptyString );

    return aCadstarAttributeMap.at( aCadstarAttributeID ).Value;
}


CADSTAR_PCB_ARCHIVE_LOADER::PART CADSTAR_PCB_ARCHIVE_LOADER::getPart(
        const PART_ID& aCadstarPartID )
{
    wxCHECK( Parts.PartDefinitions.find( aCadstarPartID ) != Parts.PartDefinitions.end(), PART() );

    return Parts.PartDefinitions.at( aCadstarPartID );
}


CADSTAR_PCB_ARCHIVE_LOADER::ROUTECODE CADSTAR_PCB_ARCHIVE_LOADER::getRouteCode(
        const ROUTECODE_ID& aCadstarRouteCodeID )
{
    wxCHECK( Assignments.Codedefs.RouteCodes.find( aCadstarRouteCodeID )
                     != Assignments.Codedefs.RouteCodes.end(),
            ROUTECODE() );

    return Assignments.Codedefs.RouteCodes.at( aCadstarRouteCodeID );
}


CADSTAR_PCB_ARCHIVE_LOADER::HATCHCODE CADSTAR_PCB_ARCHIVE_LOADER::getHatchCode(
        const HATCHCODE_ID& aCadstarHatchcodeID )
{
    wxCHECK( Assignments.Codedefs.HatchCodes.find( aCadstarHatchcodeID )
                     != Assignments.Codedefs.HatchCodes.end(),
            HATCHCODE() );

    return Assignments.Codedefs.HatchCodes.at( aCadstarHatchcodeID );
}


double CADSTAR_PCB_ARCHIVE_LOADER::getHatchCodeAngleDegrees(
        const HATCHCODE_ID& aCadstarHatchcodeID )
{
    checkAndLogHatchCode( aCadstarHatchcodeID );
    HATCHCODE hcode = getHatchCode( aCadstarHatchcodeID );

    if( hcode.Hatches.size() < 1 )
        return mBoard->GetDesignSettings().GetDefaultZoneSettings().m_HatchOrientation;
    else
        return getAngleDegrees( hcode.Hatches.at( 0 ).OrientAngle );
}


int CADSTAR_PCB_ARCHIVE_LOADER::getKiCadHatchCodeThickness(
        const HATCHCODE_ID& aCadstarHatchcodeID )
{
    checkAndLogHatchCode( aCadstarHatchcodeID );
    HATCHCODE hcode = getHatchCode( aCadstarHatchcodeID );

    if( hcode.Hatches.size() < 1 )
        return mBoard->GetDesignSettings().GetDefaultZoneSettings().m_HatchThickness;
    else
        return getKiCadLength( hcode.Hatches.at( 0 ).LineWidth );
}


int CADSTAR_PCB_ARCHIVE_LOADER::getKiCadHatchCodeGap( const HATCHCODE_ID& aCadstarHatchcodeID )
{
    checkAndLogHatchCode( aCadstarHatchcodeID );
    HATCHCODE hcode = getHatchCode( aCadstarHatchcodeID );

    if( hcode.Hatches.size() < 1 )
        return mBoard->GetDesignSettings().GetDefaultZoneSettings().m_HatchGap;
    else
        return getKiCadLength( hcode.Hatches.at( 0 ).Step );
}


PCB_GROUP* CADSTAR_PCB_ARCHIVE_LOADER::getKiCadGroup( const GROUP_ID& aCadstarGroupID )
{
    wxCHECK( mGroupMap.find( aCadstarGroupID ) != mGroupMap.end(), nullptr );

    return mGroupMap.at( aCadstarGroupID );
}


void CADSTAR_PCB_ARCHIVE_LOADER::checkAndLogHatchCode( const HATCHCODE_ID& aCadstarHatchcodeID )
{
    if( mHatchcodesTested.find( aCadstarHatchcodeID ) != mHatchcodesTested.end() )
    {
        return; //already checked
    }
    else
    {
        HATCHCODE hcode = getHatchCode( aCadstarHatchcodeID );

        if( hcode.Hatches.size() != 2 )
        {
            wxLogWarning( wxString::Format(
                    _( "The CADSTAR Hatching code '%s' has %d hatches defined. "
                       "KiCad only supports 2 hatches (crosshatching) 90 degrees apart. "
                       "The imported hatching is crosshatched." ),
                    hcode.Name, (int) hcode.Hatches.size() ) );
        }
        else
        {
            if( hcode.Hatches.at( 0 ).LineWidth != hcode.Hatches.at( 1 ).LineWidth )
            {
                wxLogWarning( wxString::Format(
                        _( "The CADSTAR Hatching code '%s' has different line widths for each "
                           "hatch. KiCad only supports one width for the haching. The imported "
                           "hatching uses the width defined in the first hatch definition, i.e. "
                           "%.2f mm." ),
                        hcode.Name,
                        (double) ( (double) getKiCadLength( hcode.Hatches.at( 0 ).LineWidth ) )
                                / 1E6 ) );
            }

            if( hcode.Hatches.at( 0 ).Step != hcode.Hatches.at( 1 ).Step )
            {
                wxLogWarning( wxString::Format(
                        _( "The CADSTAR Hatching code '%s' has different step sizes for each "
                           "hatch. KiCad only supports one step size for the haching. The imported "
                           "hatching uses the step size defined in the first hatching definition, "
                           "i.e. %.2f mm." ),
                        hcode.Name,
                        (double) ( (double) getKiCadLength( hcode.Hatches.at( 0 ).Step ) )
                                / 1E6 ) );
            }

            if( abs( hcode.Hatches.at( 0 ).OrientAngle - hcode.Hatches.at( 1 ).OrientAngle )
                    != 90000 )
            {
                wxLogWarning( wxString::Format(
                        _( "The hatches in CADSTAR Hatching code '%s' have an angle  "
                           "difference of %.1f degrees. KiCad only supports hatching 90 "
                           "degrees apart.  The imported hatching has two hatches 90 "
                           "degrees apart, oriented %.1f degrees from horizontal." ),
                        hcode.Name,
                        getAngleDegrees( abs( hcode.Hatches.at( 0 ).OrientAngle
                                              - hcode.Hatches.at( 1 ).OrientAngle ) ),
                        getAngleDegrees( hcode.Hatches.at( 0 ).OrientAngle ) ) );
            }
        }

        mHatchcodesTested.insert( aCadstarHatchcodeID );
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::applyDimensionSettings( const DIMENSION&  aCadstarDim,
                                                         ::DIMENSION_BASE* aKiCadDim )
{
    UNITS dimensionUnits = aCadstarDim.LinearUnits;
    TEXTCODE txtCode = getTextCode( aCadstarDim.Text.TextCodeID );
    int correctedHeight = KiROUND( TXT_HEIGHT_RATIO * (double) getKiCadLength( txtCode.Height ) );
    wxSize   txtSize( getKiCadLength( txtCode.Width ), correctedHeight );
    LINECODE linecode = Assignments.Codedefs.LineCodes.at( aCadstarDim.Line.LineCodeID );

    aKiCadDim->SetLayer( getKiCadLayer( aCadstarDim.LayerID ) );
    aKiCadDim->SetPrecision( aCadstarDim.Precision );
    aKiCadDim->SetStart( getKiCadPoint( aCadstarDim.ExtensionLineParams.Start ) );
    aKiCadDim->SetEnd( getKiCadPoint( aCadstarDim.ExtensionLineParams.End ) );
    aKiCadDim->SetExtensionOffset( getKiCadLength( aCadstarDim.ExtensionLineParams.Offset ) );
    aKiCadDim->SetLineThickness( getKiCadLength( linecode.Width ) );
    aKiCadDim->Text().SetTextThickness( getKiCadLength( txtCode.LineWidth ) );
    aKiCadDim->Text().SetTextSize( txtSize );

    // Find prefix and suffix:
    wxString prefix = wxEmptyString;
    wxString suffix = wxEmptyString;
    size_t   startpos = aCadstarDim.Text.Text.Find( wxT( "<@DISTANCE" ) );

    if( startpos != wxNOT_FOUND )
    {
        prefix = ParseTextFields( aCadstarDim.Text.Text.SubString( 0, startpos - 1 ), &mContext );
        wxString remainingStr = aCadstarDim.Text.Text.Mid( startpos );
        size_t   endpos = remainingStr.Find( "@>" );
        suffix = ParseTextFields( remainingStr.Mid( endpos + 2 ), &mContext );
    }

    if( suffix.StartsWith( "mm" ) )
    {
        aKiCadDim->SetUnitsFormat( DIM_UNITS_FORMAT::BARE_SUFFIX );
        suffix = suffix.Mid( 2 );
    }
    else
    {
        aKiCadDim->SetUnitsFormat( DIM_UNITS_FORMAT::NO_SUFFIX );
    }

    aKiCadDim->SetPrefix( prefix );
    aKiCadDim->SetSuffix( suffix );

    if( aCadstarDim.LinearUnits == UNITS::DESIGN )
    {
        // For now we will hardcode the units as per the original CADSTAR design.
        // TODO: update this when KiCad supports design units
        aKiCadDim->SetPrecision( Assignments.Technology.UnitDisplPrecision );
        dimensionUnits = Assignments.Technology.Units;
    }

    switch( dimensionUnits )
    {
    case UNITS::METER:
    case UNITS::CENTIMETER:
    case UNITS::MICROMETRE:
        wxLogWarning( wxString::Format( _( "Dimension ID %s uses a type of unit that "
                                           "is not supported in KiCad. Milimetres were "
                                           "applied instead." ),
                                        aCadstarDim.ID ) );
        KI_FALLTHROUGH;
    case UNITS::MM:
        aKiCadDim->SetUnitsMode( DIM_UNITS_MODE::MILLIMETRES );
        break;

    case UNITS::INCH:
        aKiCadDim->SetUnitsMode( DIM_UNITS_MODE::INCHES );
        break;

    case UNITS::THOU:
        aKiCadDim->SetUnitsMode( DIM_UNITS_MODE::MILS );
        break;

    case UNITS::DESIGN:
        wxFAIL_MSG( "We should have handled design units before coming here!" );
        break;
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::calculateZonePriorities()
{
    std::map<TEMPLATE_ID, std::set<TEMPLATE_ID>> winningOverlaps;

    // Calculate the intesection between aPolygon and the outline of aZone
    auto intersectionArea = []( SHAPE_POLY_SET aPolygon, ZONE* aZone ) -> double
                            {
                                SHAPE_POLY_SET intersectShape( *aZone->Outline() );

                                intersectShape.BooleanIntersection( aPolygon,
                                                                    SHAPE_POLY_SET::POLYGON_MODE::PM_FAST );
                                double intersectionArea = 0.0;

                                for( int i = 0; i < intersectShape.OutlineCount(); i++ )
                                {
                                    intersectionArea += intersectShape.Outline( i ).Area();

                                    for( int j = 0; j < intersectShape.HoleCount( i ); j++ )
                                        intersectionArea -= intersectShape.Hole( i, j ).Area();
                                }

                                return intersectionArea;
                            };

    for( std::map<TEMPLATE_ID, ZONE*>::iterator it1 = mLoadedTemplates.begin();
         it1 != mLoadedTemplates.end(); ++it1 )
    {
        TEMPLATE     thisTemplate = Layout.Templates.at( it1->first );
        PCB_LAYER_ID thisLayer = getKiCadLayer( thisTemplate.LayerID );
        ZONE*        thisZone = it1->second;

        for( std::map<TEMPLATE_ID, ZONE*>::iterator it2 = it1;
            it2 != mLoadedTemplates.end(); ++it2 )
        {
            TEMPLATE     otherTemplate = Layout.Templates.at( it2->first );
            PCB_LAYER_ID otherLayer = getKiCadLayer( otherTemplate.LayerID );
            ZONE*        otherZone = it2->second;

            if( thisTemplate.ID == otherTemplate.ID )
                continue;

            if( thisLayer != otherLayer )
                continue;

            // Intersect the filled polygons of thisZone with the *outline* of otherZone
            SHAPE_POLY_SET thisZonePolyFill = thisZone->GetFilledPolysList( thisLayer );
            double         areaThis = intersectionArea( thisZonePolyFill, otherZone );

            // Viceversa
            SHAPE_POLY_SET otherZonePolyFill = otherZone->GetFilledPolysList( otherLayer );
            double         areaOther = intersectionArea( otherZonePolyFill, thisZone );

            // Best effort: Compare Areas
            // If thisZone's fill polygons overlap otherZone's outline *and* the opposite
            // is true: otherZone's fill polygons overlap thisZone's outline then compare the
            // intersection areas to decide which of the two zones should have higher priority
            if( areaThis > areaOther )
                winningOverlaps[thisTemplate.ID].insert( otherTemplate.ID );
            else if( areaOther > 0 )
                winningOverlaps[otherTemplate.ID].insert( thisTemplate.ID );
        }
    }

    // Build a set of unique TEMPLATE_IDs of all the zones that intersect with another one
    std::set<TEMPLATE_ID> intersectingIDs;

    for( const std::pair<TEMPLATE_ID, std::set<TEMPLATE_ID>>& idPair : winningOverlaps )
    {
        intersectingIDs.insert( idPair.first );
        intersectingIDs.insert( idPair.second.begin(), idPair.second.end() );
    }

    // Now store them in a vector
    std::vector<TEMPLATE_ID> sortedIDs;

    for( const TEMPLATE_ID& id : intersectingIDs )
    {
        sortedIDs.push_back( id );
    }

    // Lambda to determine if the zone with template ID 'a' is lower priority than 'b'
    auto isLowerPriority = [&]( const TEMPLATE_ID& a, const TEMPLATE_ID& b ) -> bool
                         {
                             return winningOverlaps[b].count( a ) > 0;
                         };

    // sort by priority
    std::sort( sortedIDs.begin(), sortedIDs.end(), isLowerPriority );

    TEMPLATE_ID prevID = wxEmptyString;

    for( const TEMPLATE_ID& id : sortedIDs )
    {
        if( prevID.IsEmpty() )
        {
            prevID = id;
            continue;
        }

        wxASSERT( !isLowerPriority( id, prevID ) );

        int newPriority = mLoadedTemplates.at( prevID )->GetPriority();

        // Only increase priority of the current zone
        if( isLowerPriority( prevID, id ) )
            newPriority++;

        mLoadedTemplates.at( id )->SetPriority( newPriority );
        prevID = id;
    }

}


FOOTPRINT* CADSTAR_PCB_ARCHIVE_LOADER::getFootprintFromCadstarID( const COMPONENT_ID& aCadstarComponentID )
{
    if( mComponentMap.find( aCadstarComponentID ) == mComponentMap.end() )
        return nullptr;
    else
        return mComponentMap.at( aCadstarComponentID );
}


wxPoint CADSTAR_PCB_ARCHIVE_LOADER::getKiCadPoint( wxPoint aCadstarPoint )
{
    wxPoint retval;

    retval.x = ( aCadstarPoint.x - mDesignCenter.x ) * KiCadUnitMultiplier;
    retval.y = -( aCadstarPoint.y - mDesignCenter.y ) * KiCadUnitMultiplier;

    return retval;
}


double CADSTAR_PCB_ARCHIVE_LOADER::getPolarAngle( wxPoint aPoint )
{

    return NormalizeAnglePos( ArcTangente( aPoint.y, aPoint.x ) );
}


NETINFO_ITEM* CADSTAR_PCB_ARCHIVE_LOADER::getKiCadNet( const NET_ID& aCadstarNetID )
{
    if( aCadstarNetID.IsEmpty() )
        return nullptr;
    else if( mNetMap.find( aCadstarNetID ) != mNetMap.end() )
    {
        return mNetMap.at( aCadstarNetID );
    }
    else
    {
        wxCHECK( Layout.Nets.find( aCadstarNetID ) != Layout.Nets.end(), nullptr );

        NET_PCB  csNet   = Layout.Nets.at( aCadstarNetID );
        wxString newName = csNet.Name;

        if( csNet.Name.IsEmpty() )
        {
            if( csNet.Pins.size() > 0 )
            {
                // Create default KiCad net naming:

                NET_PCB::PIN firstPin = ( *csNet.Pins.begin() ).second;
                //we should have already loaded the component with loadComponents() :
                FOOTPRINT* m = getFootprintFromCadstarID( firstPin.ComponentID );
                newName   = wxT( "Net-(" );
                newName << m->Reference().GetText();
                newName << "-Pad" << wxString::Format( "%ld", firstPin.PadID ) << ")";
            }
            else
            {
                wxFAIL_MSG( "A net with no pins associated?" );
                newName = wxT( "csNet-" );
                newName << wxString::Format( "%i", csNet.SignalNum );
            }
        }

        if( !mDoneNetClassWarning && !csNet.NetClassID.IsEmpty()
                && csNet.NetClassID != wxT( "NONE" ) )
        {
            wxLogMessage(
                    _( "The CADSTAR design contains nets with a 'Net Class' assigned. KiCad does "
                       "not have an equivalent to CADSTAR's Net Class so these elements were not "
                       "imported. Note: KiCad's version of 'Net Class' is closer to CADSTAR's "
                       "'Net Route Code' (which has been imported for all nets)." ) );
            mDoneNetClassWarning = true;
        }

        if( !mDoneSpacingClassWarning && !csNet.SpacingClassID.IsEmpty()
                && csNet.SpacingClassID != wxT( "NONE" ) )
        {
            wxLogWarning( _( "The CADSTAR design contains nets with a 'Spacing Class' assigned. "
                             "KiCad does not have an equivalent to CADSTAR's Spacing Class so "
                             "these elements were not imported. Please review the design rules as "
                             "copper pours will affected by this." ) );
            mDoneSpacingClassWarning = true;
        }

        NETINFO_ITEM* netInfo = new NETINFO_ITEM( mBoard, newName, ++mNumNets );
        mBoard->Add( netInfo, ADD_MODE::APPEND );

        if( mNetClassMap.find( csNet.RouteCodeID ) != mNetClassMap.end() )
        {
            NETCLASSPTR netclass = mNetClassMap.at( csNet.RouteCodeID );
            netInfo->SetNetClass( netclass );
        }
        else
        {
            ROUTECODE   rc = getRouteCode( csNet.RouteCodeID );
            NETCLASSPTR netclass( new ::NETCLASS( rc.Name ) );
            netclass->SetTrackWidth( getKiCadLength( rc.OptimalWidth ) );
            netInfo->SetNetClass( netclass );
            mNetClassMap.insert( { csNet.RouteCodeID, netclass } );
        }

        mNetMap.insert( { aCadstarNetID, netInfo } );
        return netInfo;
    }

    return nullptr;
}


PCB_LAYER_ID CADSTAR_PCB_ARCHIVE_LOADER::getKiCadCopperLayerID( unsigned int aLayerNum, bool aDetectMaxLayer )
{
    if( aDetectMaxLayer && aLayerNum == Assignments.Technology.MaxPhysicalLayer )
        return PCB_LAYER_ID::B_Cu;

    switch( aLayerNum )
    {
    case 1:   return PCB_LAYER_ID::F_Cu;
    case 2:   return PCB_LAYER_ID::In1_Cu;
    case 3:   return PCB_LAYER_ID::In2_Cu;
    case 4:   return PCB_LAYER_ID::In3_Cu;
    case 5:   return PCB_LAYER_ID::In4_Cu;
    case 6:   return PCB_LAYER_ID::In5_Cu;
    case 7:   return PCB_LAYER_ID::In6_Cu;
    case 8:   return PCB_LAYER_ID::In7_Cu;
    case 9:   return PCB_LAYER_ID::In8_Cu;
    case 10:  return PCB_LAYER_ID::In9_Cu;
    case 11:  return PCB_LAYER_ID::In10_Cu;
    case 12:  return PCB_LAYER_ID::In11_Cu;
    case 13:  return PCB_LAYER_ID::In12_Cu;
    case 14:  return PCB_LAYER_ID::In13_Cu;
    case 15:  return PCB_LAYER_ID::In14_Cu;
    case 16:  return PCB_LAYER_ID::In15_Cu;
    case 17:  return PCB_LAYER_ID::In16_Cu;
    case 18:  return PCB_LAYER_ID::In17_Cu;
    case 19:  return PCB_LAYER_ID::In18_Cu;
    case 20:  return PCB_LAYER_ID::In19_Cu;
    case 21:  return PCB_LAYER_ID::In20_Cu;
    case 22:  return PCB_LAYER_ID::In21_Cu;
    case 23:  return PCB_LAYER_ID::In22_Cu;
    case 24:  return PCB_LAYER_ID::In23_Cu;
    case 25:  return PCB_LAYER_ID::In24_Cu;
    case 26:  return PCB_LAYER_ID::In25_Cu;
    case 27:  return PCB_LAYER_ID::In26_Cu;
    case 28:  return PCB_LAYER_ID::In27_Cu;
    case 29:  return PCB_LAYER_ID::In28_Cu;
    case 30:  return PCB_LAYER_ID::In29_Cu;
    case 31:  return PCB_LAYER_ID::In30_Cu;
    case 32:  return PCB_LAYER_ID::B_Cu;
    }

    return PCB_LAYER_ID::UNDEFINED_LAYER;
}

bool CADSTAR_PCB_ARCHIVE_LOADER::isLayerSet( const LAYER_ID& aCadstarLayerID )
{
    wxCHECK( Assignments.Layerdefs.Layers.find( aCadstarLayerID )
                     != Assignments.Layerdefs.Layers.end(),
            false );

    LAYER& layer = Assignments.Layerdefs.Layers.at( aCadstarLayerID );

    switch( layer.Type )
    {
    case LAYER_TYPE::ALLDOC:
    case LAYER_TYPE::ALLELEC:
    case LAYER_TYPE::ALLLAYER:
        return true;

    default:
        return false;
    }

    return false;
}


PCB_LAYER_ID CADSTAR_PCB_ARCHIVE_LOADER::getKiCadLayer( const LAYER_ID& aCadstarLayerID )
{
    if( Assignments.Layerdefs.Layers.find( aCadstarLayerID ) != Assignments.Layerdefs.Layers.end() )
    {
        if( Assignments.Layerdefs.Layers.at( aCadstarLayerID ).Type == LAYER_TYPE::NOLAYER )
            //The "no layer" is common for CADSTAR documentation symbols
            //map it to undefined layer for later processing
            return PCB_LAYER_ID::UNDEFINED_LAYER;
    }

    wxCHECK( mLayermap.find( aCadstarLayerID ) != mLayermap.end(), PCB_LAYER_ID::UNDEFINED_LAYER );

    return mLayermap.at( aCadstarLayerID );
}


LSET CADSTAR_PCB_ARCHIVE_LOADER::getKiCadLayerSet( const LAYER_ID& aCadstarLayerID )
{
    LAYER& layer = Assignments.Layerdefs.Layers.at( aCadstarLayerID );

    switch( layer.Type )
    {
    case LAYER_TYPE::ALLDOC:
        return LSET( 4, PCB_LAYER_ID::Dwgs_User, PCB_LAYER_ID::Cmts_User, PCB_LAYER_ID::Eco1_User,
                PCB_LAYER_ID::Eco2_User );

    case LAYER_TYPE::ALLELEC:
        return LSET::AllCuMask();

    case LAYER_TYPE::ALLLAYER:
        return LSET::AllLayersMask();

    default:
        return LSET( getKiCadLayer( aCadstarLayerID ) );
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::addToGroup(
        const GROUP_ID& aCadstarGroupID, BOARD_ITEM* aKiCadItem )
{
    wxCHECK( mGroupMap.find( aCadstarGroupID ) != mGroupMap.end(), );

    PCB_GROUP* parentGroup = mGroupMap.at( aCadstarGroupID );
    parentGroup->AddItem( aKiCadItem );
}


CADSTAR_PCB_ARCHIVE_LOADER::GROUP_ID CADSTAR_PCB_ARCHIVE_LOADER::createUniqueGroupID(
        const wxString& aName )
{
    wxString groupName = aName;
    int      num       = 0;

    while( mGroupMap.find( groupName ) != mGroupMap.end() )
    {
        groupName = aName + wxT( "_" ) + wxString::Format( "%i", ++num );
    }

    PCB_GROUP* docSymGroup = new PCB_GROUP( mBoard );
    mBoard->Add( docSymGroup );
    docSymGroup->SetName( groupName );
    GROUP_ID groupID( groupName );
    mGroupMap.insert( { groupID, docSymGroup } );

    return groupID;
}
