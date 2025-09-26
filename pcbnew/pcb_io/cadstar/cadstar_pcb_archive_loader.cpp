/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2021 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
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
 * @file cadstar_pcb_archive_loader.cpp
 * @brief Loads a cpa file into a KiCad BOARD object
 */

#include <cadstar_pcb_archive_loader.h>

#include <board_stackup_manager/board_stackup.h>
#include <board_stackup_manager/stackup_predefined_prms.h> // KEY_COPPER, KEY_CORE, KEY_PREPREG
#include <board.h>
#include <board_design_settings.h>
#include <pcb_dimension.h>
#include <pcb_shape.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_group.h>
#include <pcb_text.h>
#include <project.h>
#include <pcb_track.h>
#include <progress_reporter.h>
#include <zone.h>
#include <convert_basic_shapes_to_polygon.h>
#include <trigo.h>
#include <macros.h>
#include <wx/debug.h>
#include <wx/log.h>

#include <limits> // std::numeric_limits


void CADSTAR_PCB_ARCHIVE_LOADER::Load( BOARD* aBoard, PROJECT* aProject )
{
    m_board = aBoard;
    m_project = aProject;

    if( m_progressReporter )
        m_progressReporter->SetNumPhases( 3 ); // (0) Read file, (1) Parse file, (2) Load file

    Parse();

    LONGPOINT designLimit = Assignments.Technology.DesignLimit;

    //Note: can't use getKiCadPoint() due wxPoint being int - need long long to make the check
    long long designSizeXkicad = (long long) designLimit.x * KiCadUnitMultiplier;
    long long designSizeYkicad = (long long) designLimit.y * KiCadUnitMultiplier;

    // Max size limited by the positive dimension of wxPoint (which is an int)
    long long maxDesignSizekicad = std::numeric_limits<int>::max();

    if( designSizeXkicad > maxDesignSizekicad || designSizeYkicad > maxDesignSizekicad )
    {
        // Note that we allow the floating point output here because this message is displayed to the user and should
        // be in their locale.
        THROW_IO_ERROR( wxString::Format(
                _( "The design is too large and cannot be imported into KiCad. \n"
                   "Please reduce the maximum design size in CADSTAR by navigating to: \n"
                   "Design Tab -> Properties -> Design Options -> Maximum Design Size. \n"
                   "Current Design size: %.2f, %.2f millimeters. \n"                //format:allow
                   "Maximum permitted design size: %.2f, %.2f millimeters.\n" ),    //format:allow
                (double) designSizeXkicad / PCB_IU_PER_MM,
                (double) designSizeYkicad / PCB_IU_PER_MM,
                (double) maxDesignSizekicad / PCB_IU_PER_MM,
                (double) maxDesignSizekicad / PCB_IU_PER_MM ) );
    }

    m_designCenter =
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

    if( m_progressReporter )
    {
        m_progressReporter->BeginPhase( 2 );

        // Significantly most amount of time spent loading coppers compared to all the other steps
        // (39 seconds vs max of 100ms in other steps). This is due to requirement of boolean
        // operations to join them together into a single polygon.
        long numSteps = Layout.Coppers.size();

        // A large amount is also spent calculating zone priorities
        numSteps += ( Layout.Templates.size() * Layout.Templates.size() ) / 2;

        m_progressReporter->SetMaxProgress( numSteps );
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
    loadCoppers(); // Progress reporting is here as significantly most amount of time spent

    for( PCB_LAYER_ID id : LSET::AllCuMask( m_numCopperLayers ).Seq() )
    {
        if( !calculateZonePriorities( id ) )
        {
            wxLogError( wxString::Format( _( "Unable to determine zone fill priorities for layer "
                                             "'%s'. A best attempt has been made but it is "
                                             "possible that DRC errors exist and that manual "
                                             "editing of the zone priorities is required." ),
                                          m_board->GetLayerName( id ) ) );
        }
    }

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

    wxLogWarning( _( "CADSTAR fonts are different to the ones in KiCad. This will likely result "
                     "in alignment issues that may cause DRC errors. Please review the imported "
                     "text elements carefully and correct manually if required." ) );

    wxLogMessage(
            _( "The CADSTAR design has been imported successfully.\n"
               "Please review the import errors and warnings (if any)." ) );
}

std::vector<FOOTPRINT*> CADSTAR_PCB_ARCHIVE_LOADER::GetLoadedLibraryFootpints() const
{
    std::vector<FOOTPRINT*> retval;

    for( std::pair<SYMDEF_ID, FOOTPRINT*> fpPair : m_libraryMap )
    {
        retval.push_back( static_cast<FOOTPRINT*>( fpPair.second->Clone() ) );
    }

    return retval;
}


std::vector<std::unique_ptr<FOOTPRINT>> CADSTAR_PCB_ARCHIVE_LOADER::LoadLibrary()
{
    // loading the library after parsing takes almost no time in comparison
    if( m_progressReporter )
        m_progressReporter->SetNumPhases( 2 ); // (0) Read file, (1) Parse file

    Parse( true /*aLibrary*/);

    // Some memory handling
    for( std::pair<SYMDEF_ID, FOOTPRINT*> libItem : m_libraryMap )
    {
        FOOTPRINT* footprint = libItem.second;

        if( footprint )
            delete footprint;
    }

    m_libraryMap.clear();

    if( m_board )
        delete m_board;

    m_board = new BOARD(); // dummy board for loading
    m_project = nullptr;
    m_designCenter = { 0, 0 }; // load footprints at 0,0

    loadBoardStackup();
    remapUnsureLayers();
    loadComponentLibrary();

    std::vector<std::unique_ptr<FOOTPRINT>> retval;

    for( auto& [id, footprint] : m_libraryMap )
    {
        footprint->SetParent( nullptr ); // remove association to m_board
        retval.emplace_back( footprint );
    }

    delete m_board;

    // Don't delete the generated footprints, but empty the map
    // (the destructor of this class would end up deleting them if not)
    m_libraryMap.clear();

    return retval;
}


void CADSTAR_PCB_ARCHIVE_LOADER::logBoardStackupWarning( const wxString& aCadstarLayerName,
                                                         const PCB_LAYER_ID& aKiCadLayer )
{
    if( m_logLayerWarnings )
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
    if( m_logLayerWarnings )
    {
        wxLogMessage( wxString::Format(
                _( "The CADSTAR layer '%s' has been assumed to be a technical layer. All "
                   "elements on this layer have been mapped to KiCad layer '%s'." ),
                aCadstarLayerName, LSET::Name( aKiCadLayer ) ) );
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::initStackupItem( const LAYER&          aCadstarLayer,
                                                  BOARD_STACKUP_ITEM* aKiCadItem,
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

    if( !aCadstarLayer.Name.IsEmpty() )
        aKiCadItem->SetLayerName( aCadstarLayer.Name );

    if( aCadstarLayer.Thickness != 0 )
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
    bool                     first = true;

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
            first = false;
        }
        else if( cadstarLayer.Type == LAYER_TYPE::CONSTRUCTION )
        {
            if( first )
            {
                wxLogWarning( wxString::Format( _( "The CADSTAR construction layer '%s' is on "
                                                   "the outer surface of the board. It has been "
                                                   "ignored." ),
                                                cadstarLayer.Name ) );
            }
            else
            {
                currentBlock.ConstructionLayers.push_back( cadstarLayerID );
            }
        }
    }

    if( currentBlock.IsInitialised() )
        cadstarBoardStackup.push_back( currentBlock );

    m_numCopperLayers = cadstarBoardStackup.size();

    // Special case: last layer in the stackup is a construction layer, drop it
    if( cadstarBoardStackup.back().ConstructionLayers.size() > 0 )
    {
        for( const LAYER_ID& layerID : cadstarBoardStackup.back().ConstructionLayers )
        {
            LAYER cadstarLayer = Assignments.Layerdefs.Layers.at( layerID );

            wxLogWarning( wxString::Format( _( "The CADSTAR construction layer '%s' is on "
                                               "the outer surface of the board. It has been "
                                               "ignored." ),
                                            cadstarLayer.Name ) );
        }

        cadstarBoardStackup.back().ConstructionLayers.clear();
    }

    // Make sure it is an even number of layers (KiCad doesn't yet support unbalanced stack-ups)
    if( ( m_numCopperLayers % 2 ) != 0 )
    {
        LAYER_BLOCK bottomLayer = cadstarBoardStackup.back();
        cadstarBoardStackup.pop_back();

        LAYER_BLOCK secondToLastLayer = cadstarBoardStackup.back();
        cadstarBoardStackup.pop_back();

        LAYER_BLOCK dummyLayer;

        if( secondToLastLayer.ConstructionLayers.size() > 0 )
        {
            LAYER_ID lastConstruction = secondToLastLayer.ConstructionLayers.back();

            if( secondToLastLayer.ConstructionLayers.size() > 1 )
            {
                // At least two construction layers, lets remove one here and use the
                // other in the dummy layer
                secondToLastLayer.ConstructionLayers.pop_back();
            }
            else
            {
                // There is only one construction layer, lets halve its thickness so it is split
                // evenly between this layer and the dummy layer
                Assignments.Layerdefs.Layers.at( lastConstruction ).Thickness /= 2;
            }

            dummyLayer.ConstructionLayers.push_back( lastConstruction );
        }

        cadstarBoardStackup.push_back( secondToLastLayer );
        cadstarBoardStackup.push_back( dummyLayer );
        cadstarBoardStackup.push_back( bottomLayer );
        ++m_numCopperLayers;
    }

    wxASSERT( m_numCopperLayers == cadstarBoardStackup.size() );
    wxASSERT( cadstarBoardStackup.back().ConstructionLayers.size() == 0 );

    // Create a new stackup from default stackup list
    BOARD_DESIGN_SETTINGS& boardDesignSettings = m_board->GetDesignSettings();
    BOARD_STACKUP&         stackup = boardDesignSettings.GetStackupDescriptor();
    stackup.RemoveAll();
    m_board->SetEnabledLayers( LSET::AllLayersMask() );
    m_board->SetVisibleLayers( LSET::AllLayersMask() );
    m_board->SetCopperLayerCount( m_numCopperLayers );
    stackup.BuildDefaultStackupList( &m_board->GetDesignSettings(), m_numCopperLayers );

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
                    m_powerPlaneLayers.push_back( copperLayer.ID ); //need to add a Copper zone
                    break;

                default:
                    wxFAIL_MSG( wxT( "Unexpected Layer type. Was expecting an electrical type" ) );
                    break;
                }

                m_board->SetLayerType( item->GetBrdLayerId(), copperType );
                m_board->SetLayerName( item->GetBrdLayerId(), item->GetLayerName() );
                m_layermap.insert( { copperLayer.ID, item->GetBrdLayerId() } );
            }
        }
        else if( item->GetType() == BOARD_STACKUP_ITEM_TYPE::BS_ITEM_TYPE_DIELECTRIC )
        {
            LAYER_BLOCK layerBlock = cadstarBoardStackup.at( stackIndex );
            LAYER_BLOCK layerBlockBelow = cadstarBoardStackup.at( stackIndex + 1 );

            if( layerBlock.ConstructionLayers.size() == 0 )
            {
                ++stackIndex;
                continue; // Older cadstar designs have no construction layers - use KiCad defaults
            }

            int dielectricId = stackIndex + 1;
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
                m_board->SetLayerName( item->GetBrdLayerId(), item->GetLayerName() );
                m_layermap.insert( { dielectricLayer.ID, item->GetBrdLayerId() } );
                ++dielectricSublayer;
            }

            ++stackIndex;
        }
        else if( item->GetType() == BOARD_STACKUP_ITEM_TYPE::BS_ITEM_TYPE_SILKSCREEN )
        {
            item->SetColor( wxT( "White" ) );
        }
        else if( item->GetType() == BOARD_STACKUP_ITEM_TYPE::BS_ITEM_TYPE_SOLDERMASK )
        {
            item->SetColor( wxT( "Green" ) );
        }
    }

    int thickness = stackup.BuildBoardThicknessFromStackup();
    boardDesignSettings.SetBoardThickness( thickness );
    boardDesignSettings.m_HasStackup = true;

    int numElecLayersProcessed = 0;

    // Map CADSTAR documentation layers to KiCad "User layers"
    int                       currentDocLayer = 0;
    std::vector<PCB_LAYER_ID> docLayers = { Dwgs_User, Cmts_User, User_1, User_2, User_3, User_4,
                                            User_5,    User_6,    User_7, User_8, User_9 };

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
                if( numElecLayersProcessed >= m_numCopperLayers )
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
                    logBoardStackupWarning( curLayer.Name, kicadLayerID );
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
            THROW_IO_ERROR( wxString::Format( _( "Unexpected layer '%s' in layer stack." ),
                                              curLayer.Name ) );
            break;

        case LAYER_TYPE::JUMPERLAYER:
        case LAYER_TYPE::ELEC:
        case LAYER_TYPE::POWER:
            ++numElecLayersProcessed;
            KI_FALLTHROUGH;
        case LAYER_TYPE::CONSTRUCTION:
            //Already dealt with these when loading board stackup
            break;

        case LAYER_TYPE::DOC:

            if( currentDocLayer >= docLayers.size() )
                currentDocLayer = 0;

            kicadLayerID = docLayers.at( currentDocLayer++ );
            logBoardStackupMessage( curLayer.Name, kicadLayerID );
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
                if( layerName.Contains( wxT( "glue" ) ) || layerName.Contains( wxT( "adhesive" ) ) )
                {
                    selectLayerID( PCB_LAYER_ID::F_Adhes, PCB_LAYER_ID::B_Adhes, LOG_LEVEL::MSG );
                }
                else if( layerName.Contains( wxT( "silk" ) ) || layerName.Contains( wxT( "legend" ) ) )
                {
                    selectLayerID( PCB_LAYER_ID::F_SilkS, PCB_LAYER_ID::B_SilkS, LOG_LEVEL::MSG );
                }
                else if( layerName.Contains( wxT( "assembly" ) ) || layerName.Contains( wxT( "fabrication" ) ) )
                {
                    selectLayerID( PCB_LAYER_ID::F_Fab, PCB_LAYER_ID::B_Fab, LOG_LEVEL::MSG );
                }
                else if( layerName.Contains( wxT( "resist" ) ) || layerName.Contains( wxT( "mask" ) ) )
                {
                    selectLayerID( PCB_LAYER_ID::F_Mask, PCB_LAYER_ID::B_Mask, LOG_LEVEL::MSG );
                }
                else if( layerName.Contains( wxT( "paste" ) ) )
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
                wxFAIL_MSG( wxT( "Unknown CADSTAR Layer Sub-type" ) );
                break;
            }
            break;

        default:
            wxFAIL_MSG( wxT( "Unknown CADSTAR Layer Type" ) );
            break;
        }

        m_layermap.insert( { curLayer.ID, kicadLayerID } );
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::remapUnsureLayers()
{
    LSET enabledLayers        = m_board->GetEnabledLayers();
    LSET validRemappingLayers = enabledLayers    | LSET::AllBoardTechMask() |
                                LSET::UserMask() | LSET::UserDefinedLayersMask();

    std::vector<INPUT_LAYER_DESC> inputLayers;
    std::map<wxString, LAYER_ID>  cadstarLayerNameMap;

    for( std::pair<LAYER_ID, PCB_LAYER_ID> layerPair : m_layermap )
    {
        LAYER* curLayer = &Assignments.Layerdefs.Layers.at( layerPair.first );

        //Only remap documentation and non-electrical layers
        if( curLayer->Type == LAYER_TYPE::NONELEC || curLayer->Type == LAYER_TYPE::DOC )
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
    std::map<wxString, PCB_LAYER_ID> reMappedLayers = m_layerMappingHandler( inputLayers );

    for( std::pair<wxString, PCB_LAYER_ID> layerPair : reMappedLayers )
    {
        if( layerPair.second == PCB_LAYER_ID::UNDEFINED_LAYER )
        {
            wxFAIL_MSG( wxT( "Unexpected Layer ID" ) );
            continue;
        }

        LAYER_ID cadstarLayerID        = cadstarLayerNameMap.at( layerPair.first );
        m_layermap.at( cadstarLayerID ) = layerPair.second;
        enabledLayers |= LSET( { layerPair.second } );
    }

    m_board->SetEnabledLayers( enabledLayers );
    m_board->SetVisibleLayers( enabledLayers );
}


void CADSTAR_PCB_ARCHIVE_LOADER::loadDesignRules()
{
    BOARD_DESIGN_SETTINGS&                 bds          = m_board->GetDesignSettings();
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

    applyRule( "T_T", &bds.m_MinClearance );
    applyRule( "C_B", &bds.m_CopperEdgeClearance );
    applyRule( "H_H", &bds.m_HoleToHoleMin );

    bds.m_TrackMinWidth = getKiCadLength( Assignments.Technology.MinRouteWidth );
    bds.m_ViasMinSize = bds.m_TrackMinWidth; // Not specified, assumed same as track width
    bds.m_ViasMinAnnularWidth = bds.m_TrackMinWidth / 2; // Not specified, assumed half track width
    bds.m_MinThroughDrill = PCB_IU_PER_MM * 0.0508; // CADSTAR does not specify a minimum hole size
                                                   // so set to minimum permitted in KiCad (2 mils)
    bds.m_HoleClearance = 0; // Testing suggests cadstar might not have a copper-to-hole clearance

    auto applyNetClassRule = [&]( wxString aID, const std::shared_ptr<NETCLASS>& aNetClassPtr )
    {
        int value = -1;
        applyRule( aID, &value );

        if( value != -1 )
            aNetClassPtr->SetClearance( value );
    };

    applyNetClassRule( "T_T", bds.m_NetSettings->GetDefaultNetclass() );

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

        // Check that we are not loading a documentation symbol.
        // Documentation symbols in CADSTAR are graphical "footprints" that can be assigned
        // to any layer. The definition in the library assigns all elements to an undefined layer.
        LAYER_ID componentLayer;

        if( component.Figures.size() > 0 )
        {
            FIGURE firstFigure = component.Figures.begin()->second;
            componentLayer = firstFigure.LayerID;
        }
        else if( component.Texts.size() > 0 )
        {
            TEXT firstText = component.Texts.begin()->second;
            componentLayer = firstText.LayerID;
        }

        if( !componentLayer.IsEmpty() && getLayerType( componentLayer ) == LAYER_TYPE::NOLAYER )
            continue; // don't process documentation symbols

        FOOTPRINT* footprint = new FOOTPRINT( m_board );
        footprint->SetPosition( getKiCadPoint( component.Origin ) );

        LIB_ID libID;
        libID.Parse( component.BuildLibName(), true );

        footprint->SetFPID( libID );
        loadLibraryFigures( component, footprint );
        loadLibraryAreas( component, footprint );
        loadLibraryPads( component, footprint );
        loadLibraryCoppers( component, footprint ); // Load coppers after pads to ensure correct
                                                    // ordering of pads in footprint->Pads()

        footprint->SetPosition( { 0, 0 } ); // KiCad expects library footprints at 0,0
        footprint->SetReference( wxT( "REF**" ) );
        footprint->SetValue( libID.GetLibItemName() );
        footprint->AutoPositionFields();
        m_libraryMap.insert( std::make_pair( key, footprint ) );
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::loadLibraryFigures( const SYMDEF_PCB& aComponent,
                                                     FOOTPRINT* aFootprint )
{
    for( std::pair<FIGURE_ID, FIGURE> figPair : aComponent.Figures )
    {
        FIGURE& fig = figPair.second;

        for( const PCB_LAYER_ID& layer : getKiCadLayerSet( fig.LayerID ).Seq() )
        {
            drawCadstarShape( fig.Shape,
                              layer,
                              getLineThickness( fig.LineCodeID ),
                              wxString::Format( wxT( "Component %s:%s -> Figure %s" ),
                                                aComponent.ReferenceName,
                                                aComponent.Alternate,
                                                fig.ID ),
                              aFootprint );
        }
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::loadLibraryCoppers( const SYMDEF_PCB& aComponent,
                                                     FOOTPRINT* aFootprint )
{
    bool compCopperError = false;

    for( COMPONENT_COPPER compCopper : aComponent.ComponentCoppers )
    {
        int lineThickness = getKiCadLength( getCopperCode( compCopper.CopperCodeID ).CopperWidth );
        LSET layers = getKiCadLayerSet( compCopper.LayerID );
        LSET copperLayers = LSET::AllCuMask() & layers;
        LSET remainingLayers = layers;

        if( !compCopperError && copperLayers.count() > 1 && compCopper.AssociatedPadIDs.size() > 0 )
        {
            // TODO: Fix when we have full padstacks
            wxLogError( _( "Footprint definition '%s' has component copper associated to a pad on "
                           "multiple layers. Custom padstacks are not supported in KiCad. The "
                           "copper items have been imported as graphical elements." ),
                        aComponent.BuildLibName() );

            compCopperError = true;
        }

        if( compCopper.AssociatedPadIDs.size() > 0 && copperLayers.count() == 1
            && compCopper.Shape.Type == SHAPE_TYPE::SOLID )
        {
            // The copper is associated with pads and in an electrical layer which means it can
            // have a net associated with it. Load as a pad instead.
            // Note: we can only handle SOLID copper shapes. If the copper shape is an outline or
            // hatched or outline, then we give up and load as a graphical shape instead.

            // Find the first non-PCB-only pad. If there are none, use the first one
            COMPONENT_PAD anchorPad;
            bool          found = false;

            for( PAD_ID padID : compCopper.AssociatedPadIDs )
            {
                anchorPad = aComponent.ComponentPads.at( padID );

                if( !anchorPad.PCBonlyPad )
                {
                    found = true;
                    break;
                }
            }

            if( !found )
                anchorPad = aComponent.ComponentPads.at( compCopper.AssociatedPadIDs.front() );

            std::unique_ptr<PAD> pad = std::make_unique<PAD>( aFootprint );
            pad->SetAttribute( PAD_ATTRIB::SMD );
            pad->SetLayerSet( copperLayers );
            pad->SetNumber( anchorPad.Identifier.IsEmpty()
                                  ? wxString::Format( wxT( "%ld" ), anchorPad.ID )
                                  : anchorPad.Identifier );

            // Custom pad shape with an anchor at the position of one of the associated
            // pads and same size as the pad. Shape circle as it fits inside a rectangle
            // but not the other way round
            PADCODE anchorpadcode = getPadCode( anchorPad.PadCodeID );
            int     anchorSize = getKiCadLength( anchorpadcode.Shape.Size );
            VECTOR2I anchorPos = getKiCadPoint( anchorPad.Position );

            if( anchorSize <= 0 )
                anchorSize = 1;

            pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CUSTOM );
            pad->SetAnchorPadShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
            pad->SetSize( PADSTACK::ALL_LAYERS, { anchorSize, anchorSize } );
            pad->SetPosition( anchorPos );

            SHAPE_POLY_SET shapePolys = getPolySetFromCadstarShape( compCopper.Shape, lineThickness,
                                                                    aFootprint );
            shapePolys.Move( -anchorPos );
            pad->AddPrimitivePoly( PADSTACK::ALL_LAYERS, shapePolys, 0, true );

            // Now renumber all the associated pads
            COMPONENT_PAD associatedPad;

            for( PAD_ID padID : compCopper.AssociatedPadIDs )
            {
                PAD* assocPad = getPadReference( aFootprint, padID );
                assocPad->SetNumber( pad->GetNumber() );
            }

            aFootprint->Add( pad.release(), ADD_MODE::APPEND ); // Append so that we get the correct behaviour
                                                      // when finding pads by PAD_ID. See loadNets()

            m_librarycopperpads[aComponent.ID][anchorPad.ID].push_back( aFootprint->Pads().size() );

            remainingLayers ^= copperLayers; // don't process copper layers again!
        }

        if( remainingLayers.any() )
        {
            for( const PCB_LAYER_ID& layer : remainingLayers.Seq() )
            {
                drawCadstarShape( compCopper.Shape, layer, lineThickness,
                                  wxString::Format( wxT( "Component %s:%s -> Copper element" ),
                                                    aComponent.ReferenceName, aComponent.Alternate ),
                                  aFootprint );
            }
        }
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
            int   lineThickness = 0; // CADSTAR areas only use the line width for display purpose
            ZONE* zone = getZoneFromCadstarShape( area.Shape, lineThickness, aFootprint );

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
            zone->SetDoNotAllowZoneFills( area.NoTracks );

            zone->SetDoNotAllowVias( area.NoVias );
        }
        else
        {
            wxString libName = aComponent.ReferenceName;

            if( !aComponent.Alternate.IsEmpty() )
                libName << wxT( " (" ) << aComponent.Alternate << wxT( ")" );

            wxLogError(
                    wxString::Format( _( "The CADSTAR area '%s' in library component '%s' does not "
                                         "have a KiCad equivalent. The area is neither a via nor "
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
        if( PAD* pad = getKiCadPad( padPair.second, aFootprint ) )
            aFootprint->Add( pad, ADD_MODE::APPEND ); // Append so that we get correct behaviour
                    // when finding pads by PAD_ID - see loadNets()
    }
}


PAD* CADSTAR_PCB_ARCHIVE_LOADER::getKiCadPad( const COMPONENT_PAD& aCadstarPad, FOOTPRINT* aParent )
{
    PADCODE csPadcode = getPadCode( aCadstarPad.PadCodeID );
    wxString errorMSG;

    std::unique_ptr<PAD> pad = std::make_unique<PAD>( aParent );
    LSET padLayerSet;

    switch( aCadstarPad.Side )
    {
    case PAD_SIDE::MAXIMUM: //Bottom side
        padLayerSet |= LSET( { B_Cu, B_Paste, B_Mask } );
        break;

    case PAD_SIDE::MINIMUM: //TOP side
        padLayerSet |= LSET( { F_Cu, F_Paste, F_Mask } );
        break;

    case PAD_SIDE::THROUGH_HOLE:
        padLayerSet = LSET::AllCuMask( m_numCopperLayers ) | LSET( { F_Mask, B_Mask, F_Paste, B_Paste } );
        break;

    default:
        wxFAIL_MSG( wxT( "Unknown Pad type" ) );
    }

    pad->SetAttribute( PAD_ATTRIB::SMD ); // assume SMD pad for now
    pad->SetLocalSolderMaskMargin( 0 );
    pad->SetLocalSolderPasteMargin( 0 );
    pad->SetLocalSolderPasteMarginRatio( 0.0 );
    bool complexPadErrorLogged = false;

    for( auto& [layer, shape] : csPadcode.Reassigns )
    {
        PCB_LAYER_ID kiLayer = getKiCadLayer( layer );

        if( shape.Size == 0 )
        {
            if( kiLayer > UNDEFINED_LAYER )
                padLayerSet.reset( kiLayer );
        }
        else
        {
            int newMargin = getKiCadLength( shape.Size - csPadcode.Shape.Size ) / 2;

            if( kiLayer == F_Mask || kiLayer == B_Mask )
            {
                std::optional<int> localMargin = pad->GetLocalSolderMaskMargin();

                if( !localMargin.has_value() )
                    pad->SetLocalSolderMaskMargin( newMargin );
                else if( std::abs( localMargin.value() ) < std::abs( newMargin ) )
                    pad->SetLocalSolderMaskMargin( newMargin );
            }
            else if( kiLayer == F_Paste || kiLayer == B_Paste )
            {
                std::optional<int> localMargin = pad->GetLocalSolderPasteMargin();

                if( !localMargin.has_value() )
                    pad->SetLocalSolderPasteMargin( newMargin );
                else if( std::abs( localMargin.value() ) < std::abs( newMargin ) )
                    pad->SetLocalSolderPasteMargin( newMargin );
            }
            else
            {
                //TODO fix properly when KiCad supports full padstacks

                if( !complexPadErrorLogged )
                {
                    complexPadErrorLogged = true;
                    errorMSG +=
                            wxT( "\n - " )
                            + wxString::Format(
                                    _( "The CADSTAR pad definition '%s' is a complex pad stack, "
                                       "which is not supported in KiCad. Please review the "
                                       "imported pads as they may require manual correction." ),
                                    csPadcode.Name );
                }
            }
        }
    }

    pad->SetLayerSet( padLayerSet );

    if( aCadstarPad.PCBonlyPad )
    {
        // PCB Only pads in CADSTAR do not have a representation in the schematic - they are
        // purely mechanical pads that have no net associated with them. Make the pad name
        // empty to avoid warnings when importing from the schematic
        pad->SetNumber( wxT( "" ) );
    }
    else
    {
        pad->SetNumber( aCadstarPad.Identifier.IsEmpty()
                                ? wxString::Format( wxT( "%ld" ), aCadstarPad.ID )
                                : aCadstarPad.Identifier );
    }

    if( csPadcode.Shape.Size == 0 )
    {
        if( csPadcode.DrillDiameter == UNDEFINED_VALUE
            && aCadstarPad.Side == PAD_SIDE::THROUGH_HOLE )
        {
            // Through-hole, zero sized pad?. Lets load this just on the F_Mask for now to
            // prevent DRC errors.
            // TODO: This could be a custom padstack, update when KiCad supports padstacks
            pad->SetAttribute( PAD_ATTRIB::SMD );
            pad->SetLayerSet( LSET( { F_Mask } ) );
        }

        // zero sized pads seems to break KiCad so lets make it very small instead
        csPadcode.Shape.Size = 1;
    }

    VECTOR2I padOffset = { 0, 0 };   // offset of the pad origin (before rotating)
    VECTOR2I drillOffset = { 0, 0 }; // offset of the drill origin w.r.t. the pad (before rotating)

    switch( csPadcode.Shape.ShapeType )
    {
    case PAD_SHAPE_TYPE::ANNULUS:
        //todo fix: use custom shape instead (Donught shape, i.e. a circle with a hole)
        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
        pad->SetSize( PADSTACK::ALL_LAYERS, { getKiCadLength( csPadcode.Shape.Size ),
                                              getKiCadLength( csPadcode.Shape.Size ) } );
        break;

    case PAD_SHAPE_TYPE::BULLET:
        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CHAMFERED_RECT );
        pad->SetSize( PADSTACK::ALL_LAYERS,
                      { getKiCadLength( (long long) csPadcode.Shape.Size
                                        + (long long) csPadcode.Shape.LeftLength
                                        + (long long) csPadcode.Shape.RightLength ),
                        getKiCadLength( csPadcode.Shape.Size ) } );
        pad->SetChamferPositions( PADSTACK::ALL_LAYERS,
                                  RECT_CHAMFER_POSITIONS::RECT_CHAMFER_BOTTOM_LEFT
                                          | RECT_CHAMFER_POSITIONS::RECT_CHAMFER_TOP_LEFT );
        pad->SetRoundRectRadiusRatio( PADSTACK::ALL_LAYERS, 0.5 );
        pad->SetChamferRectRatio( PADSTACK::ALL_LAYERS, 0.0 );

        padOffset.x = getKiCadLength( ( (long long) csPadcode.Shape.LeftLength / 2 ) -
                                      ( (long long) csPadcode.Shape.RightLength / 2 ) );
        break;

    case PAD_SHAPE_TYPE::CIRCLE:
        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
        pad->SetSize( PADSTACK::ALL_LAYERS, { getKiCadLength( csPadcode.Shape.Size ),
                                              getKiCadLength( csPadcode.Shape.Size ) } );
        break;

    case PAD_SHAPE_TYPE::DIAMOND:
    {
        // Cadstar diamond shape is a square rotated 45 degrees
        // We convert it in KiCad to a square with chamfered edges
        int sizeOfSquare = (double) getKiCadLength( csPadcode.Shape.Size ) * sqrt(2.0);
        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );
        pad->SetChamferRectRatio( PADSTACK::ALL_LAYERS, 0.5 );
        pad->SetSize( PADSTACK::ALL_LAYERS, { sizeOfSquare, sizeOfSquare } );

        padOffset.x = getKiCadLength( ( (long long) csPadcode.Shape.LeftLength / 2 ) -
                                      ( (long long) csPadcode.Shape.RightLength / 2 ) );
    }
        break;

    case PAD_SHAPE_TYPE::FINGER:
        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::OVAL );
        pad->SetSize( PADSTACK::ALL_LAYERS,
                      { getKiCadLength( (long long) csPadcode.Shape.Size
                                        + (long long) csPadcode.Shape.LeftLength
                                        + (long long) csPadcode.Shape.RightLength ),
                        getKiCadLength( csPadcode.Shape.Size ) } );

        padOffset.x = getKiCadLength( ( (long long) csPadcode.Shape.LeftLength / 2 ) -
                                      ( (long long) csPadcode.Shape.RightLength / 2 ) );
        break;

    case PAD_SHAPE_TYPE::OCTAGON:
        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CHAMFERED_RECT );
        pad->SetChamferPositions( PADSTACK::ALL_LAYERS, RECT_CHAMFER_POSITIONS::RECT_CHAMFER_ALL );
        pad->SetChamferRectRatio( PADSTACK::ALL_LAYERS, 0.25 );
        pad->SetSize( PADSTACK::ALL_LAYERS, { getKiCadLength( csPadcode.Shape.Size ),
                                              getKiCadLength( csPadcode.Shape.Size ) } );
        break;

    case PAD_SHAPE_TYPE::RECTANGLE:
        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );
        pad->SetSize( PADSTACK::ALL_LAYERS,
                      { getKiCadLength( (long long) csPadcode.Shape.Size
                                        + (long long) csPadcode.Shape.LeftLength
                                        + (long long) csPadcode.Shape.RightLength ),
                        getKiCadLength( csPadcode.Shape.Size ) } );

        padOffset.x = getKiCadLength( ( (long long) csPadcode.Shape.LeftLength / 2 ) -
                                      ( (long long) csPadcode.Shape.RightLength / 2 ) );
        break;

    case PAD_SHAPE_TYPE::ROUNDED_RECT:
        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::ROUNDRECT );
        pad->SetRoundRectCornerRadius( PADSTACK::ALL_LAYERS,
                                       getKiCadLength( csPadcode.Shape.InternalFeature ) );
        pad->SetSize( PADSTACK::ALL_LAYERS,
                      { getKiCadLength( (long long) csPadcode.Shape.Size
                                        + (long long) csPadcode.Shape.LeftLength
                                        + (long long) csPadcode.Shape.RightLength ),
                        getKiCadLength( csPadcode.Shape.Size ) } );

        padOffset.x = getKiCadLength( ( (long long) csPadcode.Shape.LeftLength / 2 ) -
                                      ( (long long) csPadcode.Shape.RightLength / 2 ) );
        break;


    case PAD_SHAPE_TYPE::SQUARE:
        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );
        pad->SetSize( PADSTACK::ALL_LAYERS, { getKiCadLength( csPadcode.Shape.Size ),
                                              getKiCadLength( csPadcode.Shape.Size ) } );
        break;

    default:
        wxFAIL_MSG( wxT( "Unknown Pad Shape" ) );
    }

    if( csPadcode.ReliefClearance != UNDEFINED_VALUE )
        pad->SetThermalGap( getKiCadLength( csPadcode.ReliefClearance ) );

    if( csPadcode.ReliefWidth != UNDEFINED_VALUE )
        pad->SetLocalThermalSpokeWidthOverride( getKiCadLength( csPadcode.ReliefWidth ) );

    if( csPadcode.DrillDiameter != UNDEFINED_VALUE )
    {
        if( csPadcode.SlotLength != UNDEFINED_VALUE )
        {
            pad->SetDrillShape( PAD_DRILL_SHAPE::OBLONG );
            pad->SetDrillSize( { getKiCadLength( (long long) csPadcode.SlotLength +
                                                 (long long) csPadcode.DrillDiameter ),
                                 getKiCadLength( csPadcode.DrillDiameter ) } );
        }
        else
        {
            pad->SetDrillShape( PAD_DRILL_SHAPE::CIRCLE );
            pad->SetDrillSize( { getKiCadLength( csPadcode.DrillDiameter ),
                    getKiCadLength( csPadcode.DrillDiameter ) } );
        }

        drillOffset.x = -getKiCadLength( csPadcode.DrillXoffset );
        drillOffset.y = getKiCadLength( csPadcode.DrillYoffset );

        if( csPadcode.Plated )
            pad->SetAttribute( PAD_ATTRIB::PTH );
        else
            pad->SetAttribute( PAD_ATTRIB::NPTH );
    }
    else
    {
        pad->SetDrillSize( { 0, 0 } );
    }

    if( csPadcode.SlotOrientation != 0 )
    {
        LSET lset = pad->GetLayerSet();
        lset &= LSET::AllCuMask();

        if( lset.size() > 0 )
        {
            SHAPE_POLY_SET padOutline;
            PCB_LAYER_ID   layer = lset.Seq().at( 0 );
            int            maxError = m_board->GetDesignSettings().m_MaxError;

            pad->SetPosition( { 0, 0 } );
            pad->TransformShapeToPolygon( padOutline, layer, 0, maxError, ERROR_INSIDE );

            PCB_SHAPE* padShape = new PCB_SHAPE;
            padShape->SetShape( SHAPE_T::POLY );
            padShape->SetFilled( true );
            padShape->SetPolyShape( padOutline );
            padShape->SetStroke( STROKE_PARAMS( 0 ) );
            padShape->Move( padOffset - drillOffset );
            padShape->Rotate( VECTOR2I( 0, 0 ), ANGLE_180 - getAngle( csPadcode.SlotOrientation ) );

            SHAPE_POLY_SET editedPadOutline = padShape->GetPolyShape();

            if( editedPadOutline.Contains( { 0, 0 } ) )
            {
                pad->SetAnchorPadShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );
                pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( { 4, 4 } ) );
                pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CUSTOM );
                pad->AddPrimitive( PADSTACK::ALL_LAYERS, padShape );
                padOffset   = { 0, 0 };
            }
            else
            {
                // The CADSTAR pad has the hole shape outside the pad shape
                // Lets just put the hole in the center of the pad instead
                csPadcode.SlotOrientation = 0;
                drillOffset               = { 0, 0 };

                errorMSG +=
                        wxT( "\n - " )
                        + wxString::Format(
                                _( "The CADSTAR pad definition '%s' has the hole shape outside the "
                                   "pad shape. The hole has been moved to the center of the pad." ),
                                csPadcode.Name );
            }
        }
        else
        {
            wxFAIL_MSG( wxT( "No copper layers defined in the pad?" ) );
            csPadcode.SlotOrientation = 0;
            pad->SetOffset( PADSTACK::ALL_LAYERS, drillOffset );
        }
    }
    else
    {
        pad->SetOffset( PADSTACK::ALL_LAYERS, drillOffset );
    }

    EDA_ANGLE padOrientation = getAngle( aCadstarPad.OrientAngle )
                                    + getAngle( csPadcode.Shape.OrientAngle );

    RotatePoint( padOffset, padOrientation );
    RotatePoint( drillOffset, padOrientation );
    pad->SetPosition( getKiCadPoint( aCadstarPad.Position ) - padOffset - drillOffset );
    pad->SetOrientation( padOrientation + getAngle( csPadcode.SlotOrientation ) );

    //TODO handle csPadcode.Reassigns when KiCad supports full padstacks

    //log warnings:
    if( m_padcodesTested.find( csPadcode.ID ) == m_padcodesTested.end() && !errorMSG.IsEmpty() )
    {
        wxLogError( _( "The CADSTAR pad definition '%s' has import errors: %s" ),
                    csPadcode.Name,
                    errorMSG );

        m_padcodesTested.insert( csPadcode.ID );
    }

    return pad.release();
}


PAD*& CADSTAR_PCB_ARCHIVE_LOADER::getPadReference( FOOTPRINT*   aFootprint,
                                                   const PAD_ID aCadstarPadID )
{
    size_t index = aCadstarPadID - (long) 1;

    if( !( index < aFootprint->Pads().size() ) )
    {
        THROW_IO_ERROR( wxString::Format( _( "Unable to find pad index '%d' in footprint '%s'." ),
                                          (long) aCadstarPadID,
                                          aFootprint->GetReference() ) );
    }

    return aFootprint->Pads().at( index );
}


void CADSTAR_PCB_ARCHIVE_LOADER::loadGroups()
{
    for( std::pair<GROUP_ID, GROUP> groupPair : Layout.Groups )
    {
        GROUP& csGroup = groupPair.second;

        PCB_GROUP* kiGroup = new PCB_GROUP( m_board );

        m_board->Add( kiGroup );
        kiGroup->SetName( csGroup.Name );
        kiGroup->SetLocked( csGroup.Fixed );

        m_groupMap.insert( { csGroup.ID, kiGroup } );
    }

    //now add any groups to their parent group
    for( std::pair<GROUP_ID, GROUP> groupPair : Layout.Groups )
    {
        GROUP& csGroup = groupPair.second;

        if( !csGroup.GroupID.IsEmpty() )
        {
            if( m_groupMap.find( csGroup.ID ) == m_groupMap.end() )
            {
                THROW_IO_ERROR( wxString::Format( _( "Unable to find group ID %s in the group "
                                                     "definitions." ),
                                                  csGroup.ID ) );
            }
            else if( m_groupMap.find( csGroup.ID ) == m_groupMap.end() )
            {
                THROW_IO_ERROR( wxString::Format( _( "Unable to find sub group %s in the group "
                                                     "map (parent group ID=%s, Name=%s)." ),
                                                  csGroup.GroupID,
                                                  csGroup.ID,
                                                  csGroup.Name ) );
            }
            else
            {
                PCB_GROUP* kiCadGroup  = m_groupMap.at( csGroup.ID );
                PCB_GROUP* parentGroup = m_groupMap.at( csGroup.GroupID );
                parentGroup->AddItem( kiCadGroup );
            }
        }
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::loadBoards()
{
    for( std::pair<BOARD_ID, CADSTAR_BOARD> boardPair : Layout.Boards )
    {
        CADSTAR_BOARD&   board      = boardPair.second;
        GROUP_ID boardGroup = createUniqueGroupID( wxT( "Board" ) );
        drawCadstarShape( board.Shape, PCB_LAYER_ID::Edge_Cuts,
                          getLineThickness( board.LineCodeID ),
                          wxString::Format( wxT( "BOARD %s" ), board.ID ),
                          m_board, boardGroup );

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

        for( const PCB_LAYER_ID& layer : getKiCadLayerSet( fig.LayerID ).Seq() )
        {
            drawCadstarShape( fig.Shape,
                              layer,
                              getLineThickness( fig.LineCodeID ),
                              wxString::Format( wxT( "FIGURE %s" ), fig.ID ),
                              m_board, fig.GroupID );
        }

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
        drawCadstarText( csTxt, m_board );
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

                PCB_DIM_ALIGNED* dimension = nullptr;

                if( csDim.Subtype == DIMENSION::SUBTYPE::ORTHOGONAL )
                {
                    dimension = new PCB_DIM_ORTHOGONAL( m_board );
                    PCB_DIM_ORTHOGONAL* orDim = static_cast<PCB_DIM_ORTHOGONAL*>( dimension );

                    if( csDim.ExtensionLineParams.Start.x == csDim.Line.Start.x )
                        orDim->SetOrientation( PCB_DIM_ORTHOGONAL::DIR::HORIZONTAL );
                    else
                        orDim->SetOrientation( PCB_DIM_ORTHOGONAL::DIR::VERTICAL );
                }
                else
                {
                    dimension = new PCB_DIM_ALIGNED( m_board, PCB_DIM_ALIGNED_T );
                }

                m_board->Add( dimension, ADD_MODE::APPEND );
                applyDimensionSettings( csDim, dimension );

                dimension->SetExtensionHeight(
                        getKiCadLength( csDim.ExtensionLineParams.Overshoot ) );

                // Calculate height:
                VECTOR2I crossbarStart = getKiCadPoint( csDim.Line.Start );
                VECTOR2I crossbarEnd = getKiCadPoint( csDim.Line.End );
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
                    EDA_ANGLE angle( crossbarVector );
                    angle += ANGLE_90;
                    height = heightVector.x * angle.Cos() + heightVector.y * angle.Sin();
                }

                dimension->SetHeight( height );
            }
            break;

            default:
                // Radius and diameter dimensions are LEADERDIM (even if not actually leader)
                // Angular dimensions are always ANGLEDIM
                wxLogError(  _( "Unexpected Dimension type (ID %s). This was not imported." ),
                             csDim.ID );
                continue;
            }
            break;

        case DIMENSION::TYPE::LEADERDIM:
            //TODO: update import when KiCad supports radius and diameter dimensions

            if( csDim.Line.Style == DIMENSION::LINE::STYLE::INTERNAL )
            {
                // "internal" is a simple double sided arrow from start to end (no extension lines)
                PCB_DIM_ALIGNED* dimension = new PCB_DIM_ALIGNED( m_board, PCB_DIM_ALIGNED_T );
                m_board->Add( dimension, ADD_MODE::APPEND );
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
                PCB_DIM_LEADER* leaderDim = new PCB_DIM_LEADER( m_board );
                m_board->Add( leaderDim, ADD_MODE::APPEND );

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

                VECTOR2I endOffset( csDim.Line.LeaderLineLength * cos( angRad ) * orientX,
                                    csDim.Line.LeaderLineLength * sin( angRad ) * orientY );

                VECTOR2I endPoint = VECTOR2I( csDim.Line.End ) + endOffset;
                VECTOR2I txtPoint( endPoint.x + ( csDim.Line.LeaderLineExtensionLength * orientX ),
                                   endPoint.y );

                leaderDim->SetEnd( getKiCadPoint( endPoint ) );
                leaderDim->SetTextPos( getKiCadPoint( txtPoint ) );
                leaderDim->SetOverrideText( ParseTextFields( csDim.Text.Text, &m_context ) );
                leaderDim->SetPrefix( wxEmptyString );
                leaderDim->SetSuffix( wxEmptyString );
                leaderDim->SetUnitsFormat( DIM_UNITS_FORMAT::NO_SUFFIX );

                if( orientX == 1 )
                    leaderDim->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
                else
                    leaderDim->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );

                leaderDim->SetExtensionOffset( 0 );
            }
            break;

        case DIMENSION::TYPE::ANGLEDIM:
            //TODO: update import when KiCad supports angular dimensions
            wxLogError( _( "Dimension %s is an angular dimension which has no KiCad equivalent. "
                           "The object was not imported." ),
                        csDim.ID );
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
            int   lineThickness = 0; // CADSTAR areas only use the line width for display purpose
            ZONE* zone = getZoneFromCadstarShape( area.Shape, lineThickness, m_board );

            m_board->Add( zone, ADD_MODE::APPEND );

            if( isLayerSet( area.LayerID ) )
                zone->SetLayerSet( getKiCadLayerSet( area.LayerID ) );
            else
                zone->SetLayer( getKiCadLayer( area.LayerID ) );

            zone->SetIsRuleArea( true );      //import all CADSTAR areas as Keepout zones
            zone->SetDoNotAllowPads( false ); //no CADSTAR equivalent
            zone->SetZoneName( area.Name );

            zone->SetDoNotAllowFootprints( area.Keepout );

            zone->SetDoNotAllowTracks( area.NoTracks );
            zone->SetDoNotAllowZoneFills( area.NoTracks );

            zone->SetDoNotAllowVias( area.NoVias );

            if( area.Placement )
            {
                wxLogWarning( wxString::Format( _( "The CADSTAR area '%s' is marked as a placement "
                                                   "area in CADSTAR. Placement areas are not "
                                                   "supported in KiCad. Only the supported elements "
                                                   "for the area were imported." ),
                                                area.Name ) );
            }
        }
        else
        {
            wxLogError( wxString::Format( _( "The CADSTAR area '%s' does not have a KiCad "
                                             "equivalent. Pure Placement areas are not supported." ),
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

        auto fpIter = m_libraryMap.find( comp.SymdefID );

        if( fpIter == m_libraryMap.end() )
        {
            THROW_IO_ERROR( wxString::Format( _( "Unable to find component '%s' in the library"
                                                 "(Symdef ID: '%s')" ),
                                              comp.Name,
                                              comp.SymdefID ) );
        }

        FOOTPRINT* libFootprint = fpIter->second;

        // Use Duplicate() to ensure unique KIID for all objects
        FOOTPRINT* footprint = static_cast<FOOTPRINT*>( libFootprint->Duplicate( IGNORE_PARENT_GROUP ) );

        m_board->Add( footprint, ADD_MODE::APPEND );

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

                    getPadReference( footprint, pin.ID )->SetNumber( pinName );
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

                // Reset the pad to be around 0,0
                csPad.Position -= fpLibEntry.Origin;
                csPad.Position += m_designCenter;

                if( !padEx.PadCode.IsEmpty() )
                    csPad.PadCodeID = padEx.PadCode;

                if( padEx.OverrideExits )
                    csPad.Exits = padEx.Exits;

                if( padEx.OverrideOrientation )
                    csPad.OrientAngle = padEx.OrientAngle;

                if( padEx.OverrideSide )
                    csPad.Side = padEx.Side;

                // Find the pad in the footprint definition
                PAD* kiPad = getPadReference( footprint, padEx.ID );

                wxString padNumber = kiPad->GetNumber();

                delete kiPad;

                if( ( kiPad = getKiCadPad( csPad, footprint ) ) )
                {
                    kiPad->SetNumber( padNumber );

                    // Change the pointer in the footprint to the newly created pad
                    getPadReference( footprint, padEx.ID ) = kiPad;
                }
            }
        }

        //set to empty string to avoid duplication when loading attributes:
        footprint->SetValue( wxEmptyString );

        footprint->SetPosition( getKiCadPoint( comp.Origin ) );
        footprint->SetOrientation( getAngle( comp.OrientAngle ) );
        footprint->SetReference( comp.Name );

        if( comp.Mirror )
        {
            EDA_ANGLE mirroredAngle = - getAngle( comp.OrientAngle );
            mirroredAngle.Normalize180();
            footprint->SetOrientation( mirroredAngle );
            footprint->Flip( getKiCadPoint( comp.Origin ), FLIP_DIRECTION::LEFT_RIGHT );
        }

        loadComponentAttributes( comp, footprint );

        if( !comp.PartID.IsEmpty() && comp.PartID != wxT( "NO_PART" ) )
            footprint->SetLibDescription( getPart( comp.PartID ).Definition.Name );

        m_componentMap.insert( { comp.ID, footprint } );
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
        VECTOR2I    moveVector =
                getKiCadPoint( docSymInstance.Origin ) - getKiCadPoint( docSymDefinition.Origin );
        double rotationAngle = getAngleTenthDegree( docSymInstance.OrientAngle );
        double scalingFactor = (double) docSymInstance.ScaleRatioNumerator
                               / (double) docSymInstance.ScaleRatioDenominator;
        VECTOR2I centreOfTransform = getKiCadPoint( docSymDefinition.Origin );
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
                                  wxString::Format( wxT( "DOCUMENTATION SYMBOL %s, FIGURE %s" ),
                                                    docSymDefinition.ReferenceName, fig.ID ),
                                  m_board, groupID, moveVector, rotationAngle, scalingFactor,
                                  centreOfTransform, mirrorInvert );
            }
        }

        for( std::pair<TEXT_ID, TEXT> textPair : docSymDefinition.Texts )
        {
            TEXT txt = textPair.second;
            drawCadstarText( txt, m_board, groupID, docSymInstance.LayerID, moveVector,
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
        ZONE* zone = getZoneFromCadstarShape( csTemplate.Shape, zonelinethickness, m_board );

        m_board->Add( zone, ADD_MODE::APPEND );

        zone->SetZoneName( csTemplate.Name );
        zone->SetLayer( getKiCadLayer( csTemplate.LayerID ) );
        zone->SetAssignedPriority( 1 ); // initially 1, we will increase in calculateZonePriorities

        if( !( csTemplate.NetID.IsEmpty() || csTemplate.NetID == wxT( "NONE" ) ) )
            zone->SetNet( getKiCadNet( csTemplate.NetID ) );

        if( csTemplate.Pouring.AllowInNoRouting )
        {
            wxLogWarning( wxString::Format(
                    _( "The CADSTAR template '%s' has the setting 'Allow in No Routing Areas' "
                       "enabled. This setting has no KiCad equivalent, so it has been ignored." ),
                    csTemplate.Name ) );
        }

        if( csTemplate.Pouring.BoxIsolatedPins )
        {
            wxLogWarning( wxString::Format(
                    _( "The CADSTAR template '%s' has the setting 'Box Isolated Pins' "
                       "enabled. This setting has no KiCad equivalent, so it has been ignored." ),
                    csTemplate.Name ) );
        }

        if( csTemplate.Pouring.AutomaticRepour )
        {
            wxLogWarning( wxString::Format(
                    _( "The CADSTAR template '%s' has the setting 'Automatic Repour' "
                       "enabled. This setting has no KiCad equivalent, so it has been ignored." ),
                    csTemplate.Name ) );
        }

        // Sliver width has different behaviour to KiCad Zone's minimum thickness
        // In Cadstar 'Sliver width' has to be greater than the Copper thickness, whereas in
        // Kicad it is the opposite.
        if( csTemplate.Pouring.SliverWidth != 0 )
        {
            wxLogWarning( wxString::Format(
                    _( "The CADSTAR template '%s' has a non-zero value defined for the "
                       "'Sliver Width' setting. There is no KiCad equivalent for "
                       "this, so this setting was ignored." ),
                    csTemplate.Name ) );
        }


        if( csTemplate.Pouring.MinIsolatedCopper != csTemplate.Pouring.MinDisjointCopper )
        {
            wxLogWarning( wxString::Format(
                    _( "The CADSTAR template '%s' has different settings for 'Retain Poured Copper "
                       "- Disjoint' and 'Retain Poured Copper - Isolated'. KiCad does not "
                       "distinguish between these two settings. The setting for disjoint copper "
                       "has been applied as the minimum island area of the KiCad Zone." ),
                    csTemplate.Name ) );
        }

        long long minIslandArea = -1;

        if( csTemplate.Pouring.MinDisjointCopper != UNDEFINED_VALUE )
        {
            minIslandArea = (long long) getKiCadLength( csTemplate.Pouring.MinDisjointCopper )
                            * (long long) getKiCadLength( csTemplate.Pouring.MinDisjointCopper );

            zone->SetIslandRemovalMode( ISLAND_REMOVAL_MODE::AREA );
        }
        else
        {
            zone->SetIslandRemovalMode( ISLAND_REMOVAL_MODE::ALWAYS );
        }

        zone->SetMinIslandArea( minIslandArea );

        // In cadstar zone clearance is in addition to the global clearance.
        // TODO: need to create custom rules for individual items: zone to pad, zone to track, etc.
        int clearance = getKiCadLength( csTemplate.Pouring.AdditionalIsolation );
        clearance += m_board->GetDesignSettings().m_MinClearance;

        zone->SetLocalClearance( clearance );

        COPPERCODE pouringCopperCode = getCopperCode( csTemplate.Pouring.CopperCodeID );
        int        minThickness = getKiCadLength( pouringCopperCode.CopperWidth );
        zone->SetMinThickness( minThickness );

        if( csTemplate.Pouring.FillType == TEMPLATE::POURING::COPPER_FILL_TYPE::HATCHED )
        {
            zone->SetFillMode( ZONE_FILL_MODE::HATCH_PATTERN );
            zone->SetHatchGap( getKiCadHatchCodeGap( csTemplate.Pouring.HatchCodeID ) );
            zone->SetHatchThickness( getKiCadHatchCodeThickness( csTemplate.Pouring.HatchCodeID ) );
            zone->SetHatchOrientation( getHatchCodeAngle( csTemplate.Pouring.HatchCodeID ) );
        }
        else
        {
            zone->SetFillMode( ZONE_FILL_MODE::POLYGONS );
        }

        if( csTemplate.Pouring.ThermalReliefOnPads != csTemplate.Pouring.ThermalReliefOnVias
            || csTemplate.Pouring.ThermalReliefPadsAngle
                       != csTemplate.Pouring.ThermalReliefViasAngle )
        {
            wxLogWarning( wxString::Format(
                    _( "The CADSTAR template '%s' has different settings for thermal relief "
                       "in pads and vias. KiCad only supports one single setting for both. The "
                       "setting for pads has been applied." ),
                    csTemplate.Name ) );
        }

        COPPERCODE reliefCopperCode = getCopperCode( csTemplate.Pouring.ReliefCopperCodeID );
        int        spokeWidth = getKiCadLength( reliefCopperCode.CopperWidth );
        int        reliefWidth = getKiCadLength( csTemplate.Pouring.ClearanceWidth );

        // Cadstar supports having a spoke width thinner than the minimum thickness of the zone, but
        // this is not permitted in KiCad. We load it as solid fill instead.
        if( csTemplate.Pouring.ThermalReliefOnPads && reliefWidth > 0 )
        {
            if( spokeWidth < minThickness )
            {
                wxLogWarning( wxString::Format(
                        _( "The CADSTAR template '%s' has thermal reliefs in the original design "
                           "but the spoke width (%.2f mm) is thinner than the minimum thickness of " //format:allow
                           "the zone (%.2f mm). KiCad requires the minimum thickness of the zone "   //format:allow
                           "to be preserved. Therefore the minimum thickness has been applied as "
                           "the new spoke width and will be applied next time the zones are "
                           "filled." ),
                        csTemplate.Name, (double) getKiCadLength( spokeWidth ) / 1E6,
                        (double) getKiCadLength( minThickness ) / 1E6 ) );

                spokeWidth = minThickness;
            }

            zone->SetThermalReliefGap( reliefWidth );
            zone->SetThermalReliefSpokeWidth( spokeWidth );
            zone->SetPadConnection( ZONE_CONNECTION::THERMAL );
        }
        else
        {
            zone->SetPadConnection( ZONE_CONNECTION::FULL );
        }

        m_zonesMap.insert( { csTemplate.ID, zone } );
    }

    //Now create power plane layers:
    for( LAYER_ID layer : m_powerPlaneLayers )
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
            wxLogError( _( "The CADSTAR layer '%s' is defined as a power plane layer. However no "
                           "net with such name exists. The layer has been loaded but no copper "
                           "zone was created." ),
                        powerPlaneLayerName );
        }
        else
        {
            for( std::pair<BOARD_ID, CADSTAR_BOARD> boardPair : Layout.Boards )
            {
                //create a zone in each board shape
                BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
                CADSTAR_BOARD& board = boardPair.second;
                int    defaultLineThicknesss = bds.GetLineThickness( PCB_LAYER_ID::Edge_Cuts );
                ZONE*  zone = getZoneFromCadstarShape( board.Shape, defaultLineThicknesss, m_board );

                m_board->Add( zone, ADD_MODE::APPEND );

                zone->SetZoneName( powerPlaneLayerName );
                zone->SetLayer( getKiCadLayer( layer ) );
                zone->SetFillMode( ZONE_FILL_MODE::POLYGONS );
                zone->SetPadConnection( ZONE_CONNECTION::FULL );
                zone->SetMinIslandArea( -1 );
                zone->SetAssignedPriority( 0 ); // Priority always 0 (lowest priority) for implied power planes.
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

        checkPoint();

        if( !csCopper.PouredTemplateID.IsEmpty() )
        {
            ZONE* pouredZone = m_zonesMap.at( csCopper.PouredTemplateID );
            SHAPE_POLY_SET fill;

            int copperWidth = getKiCadLength( getCopperCode( csCopper.CopperCodeID ).CopperWidth );

            if( csCopper.Shape.Type == SHAPE_TYPE::OPENSHAPE )
            {
                // This is usually for themal reliefs. They are lines of copper with a thickness.
                // We convert them to an oval in most cases, but handle also the possibility of
                // encountering arcs in here.

                std::vector<PCB_SHAPE*> outlineShapes = getShapesFromVertices( csCopper.Shape.Vertices );

                for( PCB_SHAPE* shape : outlineShapes )
                {
                    SHAPE_POLY_SET poly;

                    if( shape->GetShape() == SHAPE_T::ARC )
                    {
                        TransformArcToPolygon( poly, shape->GetStart(), shape->GetArcMid(),
                                               shape->GetEnd(), copperWidth, ARC_HIGH_DEF,
                                               ERROR_LOC::ERROR_INSIDE );
                    }
                    else
                    {
                        TransformOvalToPolygon( poly, shape->GetStart(), shape->GetEnd(),
                                                copperWidth, ARC_HIGH_DEF,
                                                ERROR_LOC::ERROR_INSIDE );
                    }

                    poly.ClearArcs();
                    fill.BooleanAdd( poly );
                }

            }
            else
            {
                fill = getPolySetFromCadstarShape( csCopper.Shape, -1 );
                fill.ClearArcs();
                fill.Inflate( copperWidth / 2, CORNER_STRATEGY::ROUND_ALL_CORNERS, ARC_HIGH_DEF );
            }

            if( pouredZone->HasFilledPolysForLayer( getKiCadLayer( csCopper.LayerID ) ) )
            {
                fill.BooleanAdd( *pouredZone->GetFill( getKiCadLayer( csCopper.LayerID ) ) );
            }

            fill.Fracture();

            pouredZone->SetFilledPolysList( getKiCadLayer( csCopper.LayerID ), fill );
            pouredZone->SetIsFilled( true );
            pouredZone->SetNeedRefill( false );
            continue;
        }

        // For now we are going to load coppers to a KiCad zone however this isn't perfect
        //TODO: Load onto a graphical polygon with a net (when KiCad has this feature)

        if( !m_doneCopperWarning )
        {
            wxLogWarning(
                    _( "The CADSTAR design contains COPPER elements, which have no direct KiCad "
                       "equivalent. These have been imported as a KiCad Zone if solid or hatch "
                       "filled, or as a KiCad Track if the shape was an unfilled outline (open or "
                       "closed)." ) );
            m_doneCopperWarning = true;
        }


        if( csCopper.Shape.Type == SHAPE_TYPE::OPENSHAPE
            || csCopper.Shape.Type == SHAPE_TYPE::OUTLINE )
        {
            std::vector<PCB_SHAPE*> outlineShapes = getShapesFromVertices( csCopper.Shape.Vertices );

            std::vector<PCB_TRACK*> outlineTracks = makeTracksFromShapes( outlineShapes, m_board,
                                                      getKiCadNet( csCopper.NetRef.NetID ),
                                                      getKiCadLayer( csCopper.LayerID ),
                                                      getKiCadLength( getCopperCode( csCopper.CopperCodeID ).CopperWidth ) );

            //cleanup
            for( PCB_SHAPE* shape : outlineShapes )
                delete shape;

            for( CUTOUT cutout : csCopper.Shape.Cutouts )
            {
                std::vector<PCB_SHAPE*> cutoutShapes = getShapesFromVertices( cutout.Vertices );

                std::vector<PCB_TRACK*> cutoutTracks = makeTracksFromShapes( cutoutShapes, m_board,
                                                         getKiCadNet( csCopper.NetRef.NetID ),
                                                         getKiCadLayer( csCopper.LayerID ),
                                                         getKiCadLength( getCopperCode( csCopper.CopperCodeID ).CopperWidth ));

                //cleanup
                for( PCB_SHAPE* shape : cutoutShapes )
                    delete shape;
            }
        }
        else
        {
            ZONE* zone = getZoneFromCadstarShape( csCopper.Shape,
                                                  getKiCadLength( getCopperCode( csCopper.CopperCodeID ).CopperWidth ),
                                                  m_board );

            m_board->Add( zone, ADD_MODE::APPEND );

            zone->SetZoneName( csCopper.ID );
            zone->SetLayer( getKiCadLayer( csCopper.LayerID ) );
            zone->SetHatchStyle( ZONE_BORDER_DISPLAY_STYLE::NO_HATCH );

            if( csCopper.Shape.Type == SHAPE_TYPE::HATCHED )
            {
                zone->SetFillMode( ZONE_FILL_MODE::HATCH_PATTERN );
                zone->SetHatchGap( getKiCadHatchCodeGap( csCopper.Shape.HatchCodeID ) );
                zone->SetHatchThickness( getKiCadHatchCodeThickness( csCopper.Shape.HatchCodeID ) );
                zone->SetHatchOrientation( getHatchCodeAngle( csCopper.Shape.HatchCodeID ) );
            }
            else
            {
                zone->SetFillMode( ZONE_FILL_MODE::POLYGONS );
            }

            zone->SetIslandRemovalMode( ISLAND_REMOVAL_MODE::NEVER );
            zone->SetPadConnection( ZONE_CONNECTION::FULL );
            zone->SetNet( getKiCadNet( csCopper.NetRef.NetID ) );
            zone->SetAssignedPriority( m_zonesMap.size() + 1 ); // Highest priority (always fill first)

            SHAPE_POLY_SET fill( *zone->Outline() );
            fill.Fracture();

            zone->SetFilledPolysList( getKiCadLayer( csCopper.LayerID ), fill );
        }
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::loadNets()
{
    for( std::pair<NET_ID, NET_PCB> netPair : Layout.Nets )
    {
        NET_PCB  net                      = netPair.second;
        wxString netnameForErrorReporting = net.Name;

        std::map<NETELEMENT_ID, long> netelementSizes;

        if( netnameForErrorReporting.IsEmpty() )
            netnameForErrorReporting = wxString::Format( wxT( "$%ld" ), net.SignalNum );

        for( std::pair<NETELEMENT_ID, NET_PCB::VIA> viaPair : net.Vias )
        {
            NET_PCB::VIA via = viaPair.second;

            // viasize is used for calculating route offset (as done in CADSTAR post processor)
            int viaSize = loadNetVia( net.ID, via );
            netelementSizes.insert( { viaPair.first, viaSize } );
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
                                                   " '%d' in component '%s'. This has been "
                                                   "ignored." ),
                                                netnameForErrorReporting,
                                                pin.PadID,
                                                footprint->GetReference() ) );
            }
            else
            {
                // The below works because we have added the pads in the correct order to the
                // footprint and the PAD_ID in Cadstar is a sequential, numerical ID
                PAD* pad = getPadReference( footprint, pin.PadID );
                pad->SetNet( getKiCadNet( net.ID ) );

                // also set the net to any copper pads (i.e. copper elements that we have imported
                // as pads instead:
                SYMDEF_ID symdefid = Layout.Components.at( pin.ComponentID ).SymdefID;

                if( m_librarycopperpads.find( symdefid ) != m_librarycopperpads.end() )
                {
                    ASSOCIATED_COPPER_PADS assocPads = m_librarycopperpads.at( symdefid );

                    if( assocPads.find( pin.PadID ) != assocPads.end() )
                    {
                        for( PAD_ID copperPadID : assocPads.at( pin.PadID ) )
                        {
                            PAD* copperpad = getPadReference( footprint, copperPadID );
                            copperpad->SetNet( getKiCadNet( net.ID ) );
                        }
                    }
                }

                // padsize is used for calculating route offset (as done in CADSTAR post processor)
                int padsize = std::min( pad->GetSizeX(), pad->GetSizeY() );
                netelementSizes.insert( { pinPair.first, padsize } );
            }
        }

        // For junction points we need to find out the biggest size of the other routes connecting
        // at the junction in order to correctly apply the same "route offset" operation that the
        // CADSTAR post processor applies when generating Manufacturing output. The only exception
        // is if there is just a single route at the junction point, we use that route width
        auto getJunctionSize =
            [&]( NETELEMENT_ID aJptNetElemId, const NET_PCB::CONNECTION_PCB& aConnectionToIgnore ) -> int
            {
                int jptsize = std::numeric_limits<int>::max();

                for( NET_PCB::CONNECTION_PCB connection : net.Connections )
                {
                    if( connection.Route.RouteVertices.size() == 0 )
                        continue;

                    if( connection.StartNode == aConnectionToIgnore.StartNode
                        && connection.EndNode == aConnectionToIgnore.EndNode )
                    {
                        continue;
                    }

                    if( connection.StartNode == aJptNetElemId )
                    {
                        int s = getKiCadLength( connection.Route.RouteVertices.front().RouteWidth );
                        jptsize = std::max( jptsize, s );
                    }
                    else if( connection.EndNode == aJptNetElemId )
                    {
                        int s = getKiCadLength( connection.Route.RouteVertices.back().RouteWidth );
                        jptsize = std::max( jptsize, s );
                    }
                }

                if( jptsize == std::numeric_limits<int>::max()
                    && !aConnectionToIgnore.Route.RouteVertices.empty() )
                {
                    // aConnectionToIgnore is actually the only one that has a route, so lets use that
                    // to determine junction size
                    NET_PCB::ROUTE_VERTEX vertex = aConnectionToIgnore.Route.RouteVertices.front();

                    if( aConnectionToIgnore.EndNode == aJptNetElemId )
                        vertex = aConnectionToIgnore.Route.RouteVertices.back();

                    jptsize = getKiCadLength( vertex.RouteWidth );
                }

                return jptsize;
            };

        for( NET_PCB::CONNECTION_PCB connection : net.Connections )
        {
            int startSize = std::numeric_limits<int>::max();
            int endSize = std::numeric_limits<int>::max();

            if( netelementSizes.find( connection.StartNode ) != netelementSizes.end() )
                startSize = netelementSizes.at( connection.StartNode );
            else if( net.Junctions.find( connection.StartNode ) != net.Junctions.end() )
                startSize = getJunctionSize( connection.StartNode, connection );

            if( netelementSizes.find( connection.EndNode ) != netelementSizes.end() )
                endSize = netelementSizes.at( connection.EndNode );
            else if( net.Junctions.find( connection.EndNode ) != net.Junctions.end() )
                endSize = getJunctionSize( connection.EndNode, connection );

            startSize /= KiCadUnitMultiplier;
            endSize /= KiCadUnitMultiplier;

            if( !connection.Unrouted )
                loadNetTracks( net.ID, connection.Route, startSize, endSize );
        }
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::loadTextVariables()
{
    auto findAndReplaceTextField =
        [&]( TEXT_FIELD_NAME aField, wxString aValue )
        {
            if( m_context.TextFieldToValuesMap.find( aField ) !=
                m_context.TextFieldToValuesMap.end() )
            {
                if( m_context.TextFieldToValuesMap.at( aField ) != aValue )
                {
                    m_context.TextFieldToValuesMap.at( aField ) = aValue;
                    m_context.InconsistentTextFields.insert( aField );
                    return false;
                }
            }
            else
            {
                m_context.TextFieldToValuesMap.insert( { aField, aValue } );
            }

            return true;
        };

    if( m_project )
    {
        std::map<wxString, wxString>& txtVars = m_project->GetTextVars();

        // Most of the design text fields can be derived from other elements
        if( Layout.VariantHierarchy.Variants.size() > 0 )
        {
            VARIANT loadedVar = Layout.VariantHierarchy.Variants.begin()->second;

            findAndReplaceTextField( TEXT_FIELD_NAME::VARIANT_NAME, loadedVar.Name );
            findAndReplaceTextField( TEXT_FIELD_NAME::VARIANT_DESCRIPTION, loadedVar.Description );
        }

        findAndReplaceTextField( TEXT_FIELD_NAME::DESIGN_TITLE, Header.JobTitle );

        for( std::pair<TEXT_FIELD_NAME, wxString> txtvalue : m_context.TextFieldToValuesMap )
        {
            wxString varName  = CADSTAR_TO_KICAD_FIELDS.at( txtvalue.first );
            wxString varValue = txtvalue.second;

            txtVars.insert( { varName, varValue } );
        }

        for( std::pair<wxString, wxString> txtvalue : m_context.FilenamesToTextMap )
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


void CADSTAR_PCB_ARCHIVE_LOADER::loadNetTracks( const NET_ID&         aCadstarNetID,
                                                const NET_PCB::ROUTE& aCadstarRoute,
                                                long aStartWidth, long aEndWidth )
{
    if( aCadstarRoute.RouteVertices.size() == 0 )
        return;

    std::vector<PCB_SHAPE*> shapes;
    std::vector<NET_PCB::ROUTE_VERTEX> routeVertices = aCadstarRoute.RouteVertices;

    // Add thin route at front so that route offsetting works as expected
    if( aStartWidth < routeVertices.front().RouteWidth )
    {
        NET_PCB::ROUTE_VERTEX newFrontVertex = aCadstarRoute.RouteVertices.front();
        newFrontVertex.RouteWidth = aStartWidth;
        newFrontVertex.Vertex.End = aCadstarRoute.StartPoint;
        routeVertices.insert( routeVertices.begin(), newFrontVertex );
    }

    // Add thin route at the back if required
    if( aEndWidth < routeVertices.back().RouteWidth )
    {
        NET_PCB::ROUTE_VERTEX newBackVertex = aCadstarRoute.RouteVertices.back();
        newBackVertex.RouteWidth = aEndWidth;
        routeVertices.push_back( newBackVertex );
    }

    POINT prevEnd = aCadstarRoute.StartPoint;

    for( const NET_PCB::ROUTE_VERTEX& v : routeVertices )
    {
        PCB_SHAPE* shape = getShapeFromVertex( prevEnd, v.Vertex );
        shape->SetLayer( getKiCadLayer( aCadstarRoute.LayerID ) );
        shape->SetStroke( STROKE_PARAMS( getKiCadLength( v.RouteWidth ), LINE_STYLE::SOLID ) );
        shape->SetLocked( v.Fixed );
        shapes.push_back( shape );
        prevEnd = v.Vertex.End;

        if( !m_doneTearDropWarning && ( v.TeardropAtEnd || v.TeardropAtStart ) )
        {
            // TODO: load teardrops
            wxLogError( _( "The CADSTAR design contains teardrops. This importer does not yet "
                           "support them, so the teardrops in the design have been ignored." ) );

            m_doneTearDropWarning = true;
        }
    }

    NETINFO_ITEM*           net = getKiCadNet( aCadstarNetID );
    std::vector<PCB_TRACK*> tracks = makeTracksFromShapes( shapes, m_board, net );

    //cleanup
    for( PCB_SHAPE* shape : shapes )
        delete shape;
}


int CADSTAR_PCB_ARCHIVE_LOADER::loadNetVia(
        const NET_ID& aCadstarNetID, const NET_PCB::VIA& aCadstarVia )
{
    PCB_VIA* via = new PCB_VIA( m_board );
    m_board->Add( via, ADD_MODE::APPEND );

    VIACODE   csViaCode   = getViaCode( aCadstarVia.ViaCodeID );
    LAYERPAIR csLayerPair = getLayerPair( aCadstarVia.LayerPairID );

    via->SetPosition( getKiCadPoint( aCadstarVia.Location ) );
    via->SetDrill( getKiCadLength( csViaCode.DrillDiameter ) );
    via->SetLocked( aCadstarVia.Fixed );

    if( csViaCode.Shape.ShapeType != PAD_SHAPE_TYPE::CIRCLE )
    {
        wxLogError( _( "The CADSTAR via code '%s' has different shape from a circle defined. "
                       "KiCad only supports circular vias so this via type has been changed to "
                       "be a via with circular shape of %.2f mm diameter." ),                   //format:allow
                    csViaCode.Name,
                    (double) ( (double) getKiCadLength( csViaCode.Shape.Size ) / 1E6 ) );
    }

    via->SetWidth( PADSTACK::ALL_LAYERS, getKiCadLength( csViaCode.Shape.Size ) );

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
        via->SetViaType( VIATYPE::BURIED );
    }
    else
    {
        via->SetViaType( VIATYPE::BLIND );
    }

    via->SetLayerPair( getKiCadCopperLayerID( csLayerPair.PhysicalLayerStart ),
            getKiCadCopperLayerID( csLayerPair.PhysicalLayerEnd ) );
    via->SetNet( getKiCadNet( aCadstarNetID ) );
    ///todo add netcode to the via

    return via->GetWidth( PADSTACK::ALL_LAYERS );
}


void CADSTAR_PCB_ARCHIVE_LOADER::drawCadstarText( const TEXT& aCadstarText,
                                                  BOARD_ITEM_CONTAINER* aContainer,
                                                  const GROUP_ID& aCadstarGroupID,
                                                  const LAYER_ID& aCadstarLayerOverride,
                                                  const VECTOR2I& aMoveVector,
                                                  double aRotationAngle, double aScalingFactor,
                                                  const VECTOR2I& aTransformCentre,
                                                  bool aMirrorInvert )
{
    PCB_TEXT* txt = new PCB_TEXT( aContainer );
    aContainer->Add( txt );
    txt->SetText( aCadstarText.Text );

    EDA_ANGLE rotationAngle( aRotationAngle, TENTHS_OF_A_DEGREE_T );
    VECTOR2I  rotatedTextPos = getKiCadPoint( aCadstarText.Position );
    RotatePoint( rotatedTextPos, aTransformCentre, rotationAngle );
    rotatedTextPos.x = KiROUND( ( rotatedTextPos.x - aTransformCentre.x ) * aScalingFactor );
    rotatedTextPos.y = KiROUND( ( rotatedTextPos.y - aTransformCentre.y ) * aScalingFactor );
    rotatedTextPos += aTransformCentre;
    txt->SetTextPos( rotatedTextPos );
    txt->SetPosition( rotatedTextPos );

    txt->SetTextAngle( getAngle( aCadstarText.OrientAngle ) + rotationAngle );

    txt->SetMirrored( aCadstarText.Mirror );

    applyTextCode( txt, aCadstarText.TextCodeID );

    switch( aCadstarText.Alignment )
    {
    case ALIGNMENT::NO_ALIGNMENT: // Default for Single line text is Bottom Left
    case ALIGNMENT::BOTTOMLEFT:
        txt->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
        txt->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        break;

    case ALIGNMENT::BOTTOMCENTER:
        txt->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
        txt->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        break;

    case ALIGNMENT::BOTTOMRIGHT:
        txt->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
        txt->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        break;

    case ALIGNMENT::CENTERLEFT:
        txt->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
        txt->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        break;

    case ALIGNMENT::CENTERCENTER:
        txt->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
        txt->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        break;

    case ALIGNMENT::CENTERRIGHT:
        txt->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
        txt->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        break;

    case ALIGNMENT::TOPLEFT:
        txt->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
        txt->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        break;

    case ALIGNMENT::TOPCENTER:
        txt->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
        txt->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        break;

    case ALIGNMENT::TOPRIGHT:
        txt->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
        txt->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        break;

    default:
        wxFAIL_MSG( wxT( "Unknown Alignment - needs review!" ) );
    }

    if( aMirrorInvert )
        txt->Flip( aTransformCentre, FLIP_DIRECTION::LEFT_RIGHT );

    //scale it after flipping:
    if( aScalingFactor != 1.0 )
    {
        VECTOR2I unscaledTextSize = txt->GetTextSize();
        int      unscaledThickness = txt->GetTextThickness();

        VECTOR2I scaledTextSize;
        scaledTextSize.x = KiROUND( (double) unscaledTextSize.x * aScalingFactor );
        scaledTextSize.y = KiROUND( (double) unscaledTextSize.y * aScalingFactor );

        txt->SetTextSize( scaledTextSize );
        txt->SetTextThickness( KiROUND( (double) unscaledThickness * aScalingFactor ) );
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
        for( PCB_LAYER_ID layer : getKiCadLayerSet( layersToDrawOn ).Seq() )
        {
            txt->SetLayer( layer );
            PCB_TEXT* newtxt = static_cast<PCB_TEXT*>( txt->Duplicate( IGNORE_PARENT_GROUP ) );
            m_board->Add( newtxt, ADD_MODE::APPEND );

            if( !aCadstarGroupID.IsEmpty() )
                addToGroup( aCadstarGroupID, newtxt );
        }

        m_board->Remove( txt );
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
                                                   int aLineThickness,
                                                   const wxString& aShapeName,
                                                   BOARD_ITEM_CONTAINER* aContainer,
                                                   const GROUP_ID& aCadstarGroupID,
                                                   const VECTOR2I& aMoveVector,
                                                   double aRotationAngle, double aScalingFactor,
                                                   const VECTOR2I& aTransformCentre,
                                                   bool aMirrorInvert )
{
    auto drawAsOutline = [&]()
    {
        drawCadstarVerticesAsShapes( aCadstarShape.Vertices, aKiCadLayer, aLineThickness,
                                     aContainer, aCadstarGroupID, aMoveVector, aRotationAngle,
                                     aScalingFactor, aTransformCentre, aMirrorInvert );
        drawCadstarCutoutsAsShapes( aCadstarShape.Cutouts, aKiCadLayer, aLineThickness, aContainer,
                                    aCadstarGroupID, aMoveVector, aRotationAngle, aScalingFactor,
                                    aTransformCentre, aMirrorInvert );
    };

    switch( aCadstarShape.Type )
    {
    case SHAPE_TYPE::OPENSHAPE:
    case SHAPE_TYPE::OUTLINE:
        ///TODO update this when Polygons in KiCad can be defined with no fill
        drawAsOutline();
        break;

    case SHAPE_TYPE::HATCHED:
        ///TODO update this when Polygons in KiCad can be defined with hatch fill
        wxLogWarning( wxString::Format(
                _( "The shape for '%s' is Hatch filled in CADSTAR, which has no KiCad equivalent. "
                   "Using solid fill instead." ),
                aShapeName ) );

    case SHAPE_TYPE::SOLID:
    {
        // Special case solid shapes that are effectively a single line
        if( aCadstarShape.Vertices.size() < 3 )
        {
            drawAsOutline();
            break;
        }

        PCB_SHAPE* shape = new PCB_SHAPE( aContainer, SHAPE_T::POLY );

        shape->SetFilled( true );

        SHAPE_POLY_SET shapePolys = getPolySetFromCadstarShape( aCadstarShape, -1, aContainer,
                                                                aMoveVector, aRotationAngle,
                                                                aScalingFactor, aTransformCentre,
                                                                aMirrorInvert );

        shapePolys.Fracture();

        shape->SetPolyShape( shapePolys );
        shape->SetStroke( STROKE_PARAMS( aLineThickness, LINE_STYLE::SOLID ) );
        shape->SetLayer( aKiCadLayer );
        aContainer->Add( shape, ADD_MODE::APPEND );

        if( !aCadstarGroupID.IsEmpty() )
            addToGroup( aCadstarGroupID, shape );
    }
    break;
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::drawCadstarCutoutsAsShapes( const std::vector<CUTOUT>& aCutouts,
                                                             const PCB_LAYER_ID& aKiCadLayer,
                                                             int aLineThickness,
                                                             BOARD_ITEM_CONTAINER* aContainer,
                                                             const GROUP_ID& aCadstarGroupID,
                                                             const VECTOR2I& aMoveVector,
                                                             double aRotationAngle,
                                                             double aScalingFactor,
                                                             const VECTOR2I& aTransformCentre,
                                                             bool aMirrorInvert )
{
    for( const CUTOUT& cutout : aCutouts )
    {
        drawCadstarVerticesAsShapes( cutout.Vertices, aKiCadLayer, aLineThickness, aContainer,
                                     aCadstarGroupID, aMoveVector, aRotationAngle, aScalingFactor,
                                     aTransformCentre, aMirrorInvert );
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::drawCadstarVerticesAsShapes( const std::vector<VERTEX>& aCadstarVertices,
                                                              const PCB_LAYER_ID& aKiCadLayer,
                                                              int aLineThickness,
                                                              BOARD_ITEM_CONTAINER* aContainer,
                                                              const GROUP_ID& aCadstarGroupID,
                                                              const VECTOR2I& aMoveVector,
                                                              double aRotationAngle,
                                                              double aScalingFactor,
                                                              const VECTOR2I& aTransformCentre,
                                                              bool aMirrorInvert )
{
    std::vector<PCB_SHAPE*> shapes = getShapesFromVertices( aCadstarVertices, aContainer,
                                                            aCadstarGroupID, aMoveVector,
                                                            aRotationAngle, aScalingFactor,
                                                            aTransformCentre, aMirrorInvert );

    for( PCB_SHAPE* shape : shapes )
    {
        shape->SetStroke( STROKE_PARAMS( aLineThickness, LINE_STYLE::SOLID ) );
        shape->SetLayer( aKiCadLayer );
        shape->SetParent( aContainer );
        aContainer->Add( shape, ADD_MODE::APPEND );
    }
}


std::vector<PCB_SHAPE*>
CADSTAR_PCB_ARCHIVE_LOADER::getShapesFromVertices( const std::vector<VERTEX>& aCadstarVertices,
                                                   BOARD_ITEM_CONTAINER* aContainer,
                                                   const GROUP_ID& aCadstarGroupID,
                                                   const VECTOR2I& aMoveVector,
                                                   double aRotationAngle, double aScalingFactor,
                                                   const VECTOR2I& aTransformCentre,
                                                   bool aMirrorInvert )
{
    std::vector<PCB_SHAPE*> shapes;

    if( aCadstarVertices.size() < 2 )
        //need at least two points to draw a segment! (unlikely but possible to have only one)
        return shapes;

    const VERTEX* prev = &aCadstarVertices.at( 0 ); // first one should always be a point vertex
    const VERTEX* cur;

    for( size_t i = 1; i < aCadstarVertices.size(); i++ )
    {
        cur = &aCadstarVertices.at( i );
        shapes.push_back( getShapeFromVertex( prev->End, *cur, aContainer, aCadstarGroupID,
                                              aMoveVector, aRotationAngle, aScalingFactor,
                                              aTransformCentre, aMirrorInvert ) );
        prev = cur;
    }

    return shapes;
}


PCB_SHAPE* CADSTAR_PCB_ARCHIVE_LOADER::getShapeFromVertex( const POINT& aCadstarStartPoint,
                                                           const VERTEX& aCadstarVertex,
                                                           BOARD_ITEM_CONTAINER* aContainer,
                                                           const GROUP_ID& aCadstarGroupID,
                                                           const VECTOR2I& aMoveVector,
                                                           double aRotationAngle,
                                                           double aScalingFactor,
                                                           const VECTOR2I& aTransformCentre,
                                                           bool aMirrorInvert )
{
    PCB_SHAPE* shape = nullptr;
    bool       cw = false;

    VECTOR2I startPoint = getKiCadPoint( aCadstarStartPoint );
    VECTOR2I endPoint = getKiCadPoint( aCadstarVertex.End );
    VECTOR2I centerPoint;

    if( aCadstarVertex.Type == VERTEX_TYPE::ANTICLOCKWISE_SEMICIRCLE
            || aCadstarVertex.Type == VERTEX_TYPE::CLOCKWISE_SEMICIRCLE )
    {
        centerPoint = ( startPoint + endPoint ) / 2;
    }
    else
    {
        centerPoint = getKiCadPoint( aCadstarVertex.Center );
    }

    switch( aCadstarVertex.Type )
    {

    case VERTEX_TYPE::VT_POINT:
        shape = new PCB_SHAPE( aContainer, SHAPE_T::SEGMENT );

        shape->SetStart( startPoint );
        shape->SetEnd( endPoint );
        break;

    case VERTEX_TYPE::CLOCKWISE_SEMICIRCLE:
    case VERTEX_TYPE::CLOCKWISE_ARC:
        cw = true;
        KI_FALLTHROUGH;

    case VERTEX_TYPE::ANTICLOCKWISE_SEMICIRCLE:
    case VERTEX_TYPE::ANTICLOCKWISE_ARC:
    {
        shape = new PCB_SHAPE( aContainer, SHAPE_T::ARC );

        shape->SetCenter( centerPoint );
        shape->SetStart( startPoint );

        EDA_ANGLE arcStartAngle( startPoint - centerPoint );
        EDA_ANGLE arcEndAngle( endPoint - centerPoint );
        EDA_ANGLE arcAngle = ( arcEndAngle - arcStartAngle ).Normalize();
        //TODO: detect if we are supposed to draw a circle instead (i.e. two SEMICIRCLEs
        // with opposite start/end points and same centre point)

        if( !cw )
            arcAngle.NormalizeNegative(); // anticlockwise arc

        shape->SetArcAngleAndEnd( arcAngle, true );

        break;
    }
    }

    //Apply transforms
    if( aMirrorInvert )
        shape->Flip( aTransformCentre, FLIP_DIRECTION::LEFT_RIGHT );

    if( aScalingFactor != 1.0 )
    {
        shape->Move( -1*aTransformCentre );
        shape->Scale( aScalingFactor );
        shape->Move( aTransformCentre );
    }

    if( aRotationAngle != 0.0 )
        shape->Rotate( aTransformCentre, EDA_ANGLE( aRotationAngle, TENTHS_OF_A_DEGREE_T ) );

    if( aMoveVector != VECTOR2I{ 0, 0 } )
        shape->Move( aMoveVector );

    if( !aCadstarGroupID.IsEmpty() )
        addToGroup( aCadstarGroupID, shape );

    return shape;
}


ZONE* CADSTAR_PCB_ARCHIVE_LOADER::getZoneFromCadstarShape( const SHAPE& aCadstarShape,
                                                           const int& aLineThickness,
                                                           BOARD_ITEM_CONTAINER* aParentContainer )
{
    ZONE* zone = new ZONE( aParentContainer );

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
                                                                       int aLineThickness,
                                                                       BOARD_ITEM_CONTAINER* aContainer,
                                                                       const VECTOR2I& aMoveVector,
                                                                       double aRotationAngle,
                                                                       double aScalingFactor,
                                                                       const VECTOR2I& aTransformCentre,
                                                                       bool aMirrorInvert )
{
    GROUP_ID noGroup = wxEmptyString;

    std::vector<PCB_SHAPE*> outlineShapes = getShapesFromVertices( aCadstarShape.Vertices,
                                                                   aContainer, noGroup, aMoveVector,
                                                                   aRotationAngle, aScalingFactor,
                                                                   aTransformCentre, aMirrorInvert );

    SHAPE_POLY_SET polySet( getLineChainFromShapes( outlineShapes ) );

    //cleanup
    for( PCB_SHAPE* shape : outlineShapes )
        delete shape;

    for( const CUTOUT& cutout : aCadstarShape.Cutouts )
    {
        std::vector<PCB_SHAPE*> cutoutShapes = getShapesFromVertices( cutout.Vertices, aContainer,
                                                                      noGroup, aMoveVector,
                                                                      aRotationAngle, aScalingFactor,
                                                                      aTransformCentre, aMirrorInvert );

        polySet.AddHole( getLineChainFromShapes( cutoutShapes ) );

        //cleanup
        for( PCB_SHAPE* shape : cutoutShapes )
            delete shape;
    }

    polySet.ClearArcs();

    if( aLineThickness > 0 )
        polySet.Inflate( aLineThickness / 2, CORNER_STRATEGY::ROUND_ALL_CORNERS, ARC_HIGH_DEF );

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


SHAPE_LINE_CHAIN CADSTAR_PCB_ARCHIVE_LOADER::getLineChainFromShapes( const std::vector<PCB_SHAPE*>& aShapes )
{
    SHAPE_LINE_CHAIN lineChain;

    for( PCB_SHAPE* shape : aShapes )
    {
        switch( shape->GetShape() )
        {
        case SHAPE_T::ARC:
        {
            SHAPE_ARC arc( shape->GetCenter(), shape->GetStart(), shape->GetArcAngle() );

            if( shape->EndsSwapped() )
                arc.Reverse();

            lineChain.Append( arc );
            break;
        }

        case SHAPE_T::SEGMENT:
            lineChain.Append( shape->GetStartX(), shape->GetStartY() );
            lineChain.Append( shape->GetEndX(), shape->GetEndY() );
            break;

        default:
            wxFAIL_MSG( wxT( "Drawsegment type is unexpected. Ignored." ) );
        }
    }

    // Shouldn't have less than 3 points to make a closed shape!
    wxASSERT( lineChain.PointCount() > 2 );

    // Check if it is closed
    if( lineChain.GetPoint( 0 ) != lineChain.GetPoint( lineChain.PointCount() - 1 ) )
    {
        lineChain.Append( lineChain.GetPoint( 0 ) );
    }

    lineChain.SetClosed( true );

    return lineChain;
}


std::vector<PCB_TRACK*> CADSTAR_PCB_ARCHIVE_LOADER::makeTracksFromShapes(
                                                  const std::vector<PCB_SHAPE*>& aShapes,
                                                  BOARD_ITEM_CONTAINER* aParentContainer,
                                                  NETINFO_ITEM* aNet, PCB_LAYER_ID aLayerOverride,
                                                  int aWidthOverride )
{
    std::vector<PCB_TRACK*> tracks;
    PCB_TRACK*              prevTrack = nullptr;
    PCB_TRACK*              track = nullptr;

    auto addTrack =
            [&]( PCB_TRACK* aTrack )
            {
                // Ignore zero length tracks in the same way as the CADSTAR postprocessor does
                // when generating gerbers. Note that CADSTAR reports these as "Route offset
                // errors" when running a DRC within CADSTAR, so we shouldn't be getting this in
                // general, however it is used to remove any synthetic points added to
                // aDrawSegments by the caller of this function.
                if( aTrack->GetLength() != 0 )
                {
                    tracks.push_back( aTrack );
                    aParentContainer->Add( aTrack, ADD_MODE::APPEND );
                }
                else
                {
                    delete aTrack;
                }
            };

    for( PCB_SHAPE* shape : aShapes )
    {
        switch( shape->GetShape() )
        {
        case SHAPE_T::ARC:
        {
            SHAPE_ARC arc( shape->GetStart(), shape->GetArcMid(), shape->GetEnd(), 0 );

            if( shape->EndsSwapped() )
                arc.Reverse();

            track = new PCB_ARC( aParentContainer, &arc );
            break;
        }

        case SHAPE_T::SEGMENT:
            track = new PCB_TRACK( aParentContainer );
            track->SetStart( shape->GetStart() );
            track->SetEnd( shape->GetEnd() );
            break;

        default:
            wxFAIL_MSG( wxT( "Drawsegment type is unexpected. Ignored." ) );
            continue;
        }

        if( aWidthOverride == -1 )
            track->SetWidth( shape->GetWidth() );
        else
            track->SetWidth( aWidthOverride );

        if( aLayerOverride == PCB_LAYER_ID::UNDEFINED_LAYER )
            track->SetLayer( shape->GetLayer() );
        else
            track->SetLayer( aLayerOverride );

        if( aNet != nullptr )
            track->SetNet( aNet );
        else
            track->SetNetCode( -1 );

        track->SetLocked( shape->IsLocked() );

        // Apply route offsetting, mimmicking the behaviour of the CADSTAR post processor
        if( prevTrack != nullptr )
        {
            int offsetAmount = ( track->GetWidth() / 2 ) - ( prevTrack->GetWidth() / 2 );

            if( offsetAmount > 0 )
            {
                // modify the start of the current track
                VECTOR2I newStart = track->GetStart();
                applyRouteOffset( &newStart, track->GetEnd(), offsetAmount );
                track->SetStart( newStart );
            }
            else if( offsetAmount < 0 )
            {
                // amend the end of the previous track
                VECTOR2I newEnd = prevTrack->GetEnd();
                applyRouteOffset( &newEnd, prevTrack->GetStart(), -offsetAmount );
                prevTrack->SetEnd( newEnd );
            } // don't do anything if offsetAmount == 0

            // Add a synthetic track of the thinnest width between the tracks
            // to ensure KiCad features works as expected on the imported design
            // (KiCad expects tracks are contiguous segments)
            if( track->GetStart() != prevTrack->GetEnd() )
            {
                int    minWidth = std::min( track->GetWidth(), prevTrack->GetWidth() );
                PCB_TRACK* synthTrack = new PCB_TRACK( aParentContainer );
                synthTrack->SetStart( prevTrack->GetEnd() );
                synthTrack->SetEnd( track->GetStart() );
                synthTrack->SetWidth( minWidth );
                synthTrack->SetLocked( track->IsLocked() );
                synthTrack->SetNet( track->GetNet() );
                synthTrack->SetLayer( track->GetLayer() );
                addTrack( synthTrack );
            }
        }

        if( prevTrack )
            addTrack( prevTrack );

        prevTrack = track;
    }

    if( track )
        addTrack( track );

    return tracks;
}


void CADSTAR_PCB_ARCHIVE_LOADER::addAttribute( const ATTRIBUTE_LOCATION& aCadstarAttrLoc,
                                               const ATTRIBUTE_ID& aCadstarAttributeID,
                                               FOOTPRINT* aFootprint,
                                               const wxString& aAttributeValue )
{
    PCB_FIELD* field;

    if( aCadstarAttributeID == COMPONENT_NAME_ATTRID )
    {
        field = &aFootprint->Reference(); //text should be set outside this function
    }
    else if( aCadstarAttributeID == PART_NAME_ATTRID )
    {
        if( aFootprint->Value().GetText().IsEmpty() )
        {
            // Use PART_NAME_ATTRID as the value is value field is blank
            aFootprint->SetValue( aAttributeValue );
            field = &aFootprint->Value();
        }
        else
        {
            field = new PCB_FIELD( aFootprint, FIELD_T::USER, aCadstarAttributeID );
            aFootprint->Add( field );
            field->SetText( aAttributeValue );
        }
        field->SetVisible( false ); //make invisible to avoid clutter.
    }
    else if( aCadstarAttributeID != COMPONENT_NAME_2_ATTRID
             && getAttributeName( aCadstarAttributeID ) == wxT( "Value" ) )
    {
        if( !aFootprint->Value().GetText().IsEmpty() )
        {
            //copy the object
            aFootprint->Add( aFootprint->Value().Duplicate( IGNORE_PARENT_GROUP ) );
        }

        aFootprint->SetValue( aAttributeValue );
        field = &aFootprint->Value();
        field->SetVisible( false ); //make invisible to avoid clutter.
    }
    else
    {
        field = new PCB_FIELD( aFootprint, FIELD_T::USER, aCadstarAttributeID );
        aFootprint->Add( field );
        field->SetText( aAttributeValue );
        field->SetVisible( false ); //make all user attributes invisible to avoid clutter.
        //TODO: Future improvement - allow user to decide what to do with attributes
    }

    field->SetPosition( getKiCadPoint( aCadstarAttrLoc.Position ) );
    field->SetLayer( getKiCadLayer( aCadstarAttrLoc.LayerID ) );
    field->SetMirrored( aCadstarAttrLoc.Mirror );
    field->SetTextAngle( getAngle( aCadstarAttrLoc.OrientAngle ) );

    if( aCadstarAttrLoc.Mirror ) // If mirroring, invert angle to match CADSTAR
        field->SetTextAngle( -field->GetTextAngle() );

    applyTextCode( field, aCadstarAttrLoc.TextCodeID );

    field->SetKeepUpright( false ); //Keeping it upright seems to result in incorrect orientation

    switch( aCadstarAttrLoc.Alignment )
    {
    case ALIGNMENT::NO_ALIGNMENT: // Default for Single line text is Bottom Left
        FixTextPositionNoAlignment( field );
        KI_FALLTHROUGH;
    case ALIGNMENT::BOTTOMLEFT:
        field->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
        field->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        break;

    case ALIGNMENT::BOTTOMCENTER:
        field->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
        field->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        break;

    case ALIGNMENT::BOTTOMRIGHT:
        field->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
        field->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        break;

    case ALIGNMENT::CENTERLEFT:
        field->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
        field->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        break;

    case ALIGNMENT::CENTERCENTER:
        field->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
        field->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        break;

    case ALIGNMENT::CENTERRIGHT:
        field->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
        field->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        break;

    case ALIGNMENT::TOPLEFT:
        field->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
        field->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        break;

    case ALIGNMENT::TOPCENTER:
        field->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
        field->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        break;

    case ALIGNMENT::TOPRIGHT:
        field->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
        field->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        break;

    default:
        wxFAIL_MSG( wxT( "Unknown Alignment - needs review!" ) );
    }

    //TODO Handle different font types when KiCad can support it.
}


void CADSTAR_PCB_ARCHIVE_LOADER::applyRouteOffset( VECTOR2I*      aPointToOffset,
                                                   const VECTOR2I& aRefPoint,
                                                   const long& aOffsetAmount )
{
    VECTOR2I v( *aPointToOffset - aRefPoint );
    int      newLength = v.EuclideanNorm() - aOffsetAmount;

    if( newLength > 0 )
    {
        VECTOR2I offsetted = v.Resize( newLength ) + VECTOR2I( aRefPoint );
        aPointToOffset->x = offsetted.x;
        aPointToOffset->y = offsetted.y;
    }
    else
    {
        *aPointToOffset = aRefPoint; // zero length track. Needs to be removed to mimmick
                                     // cadstar behaviour
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER:: applyTextCode( EDA_TEXT*          aKiCadText,
                                                 const TEXTCODE_ID& aCadstarTextCodeID )
{
    TEXTCODE tc = getTextCode( aCadstarTextCodeID );

    aKiCadText->SetTextThickness( getKiCadLength( tc.LineWidth ) );

    VECTOR2I textSize;
    textSize.x = getKiCadLength( tc.Width );

    // The width is zero for all non-cadstar fonts. Using a width equal to the height seems
    // to work well for most fonts.
    if( textSize.x == 0 )
        textSize.x = getKiCadLength( tc.Height );

    textSize.y = KiROUND( TXT_HEIGHT_RATIO * (double) getKiCadLength( tc.Height ) );

    if( textSize.x == 0 || textSize.y == 0 )
    {
        // Make zero sized text not visible

        aKiCadText->SetTextSize(
                VECTOR2I( EDA_UNIT_UTILS::Mils2IU( pcbIUScale, DEFAULT_SIZE_TEXT ),
                          EDA_UNIT_UTILS::Mils2IU( pcbIUScale, DEFAULT_SIZE_TEXT ) ) );
    }
    else
    {
        aKiCadText->SetTextSize( textSize );
    }
}


int CADSTAR_PCB_ARCHIVE_LOADER::getLineThickness( const LINECODE_ID& aCadstarLineCodeID )
{
    wxCHECK( Assignments.Codedefs.LineCodes.find( aCadstarLineCodeID )
                     != Assignments.Codedefs.LineCodes.end(),
            m_board->GetDesignSettings().GetLineThickness( PCB_LAYER_ID::Edge_Cuts ) );

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


CADSTAR_PCB_ARCHIVE_LOADER::LAYER_TYPE
CADSTAR_PCB_ARCHIVE_LOADER::getLayerType( const LAYER_ID aCadstarLayerID )
{
    if( Assignments.Layerdefs.Layers.find( aCadstarLayerID ) != Assignments.Layerdefs.Layers.end() )
    {
        return Assignments.Layerdefs.Layers.at( aCadstarLayerID ).Type;
    }

    return LAYER_TYPE::UNDEFINED;
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


EDA_ANGLE CADSTAR_PCB_ARCHIVE_LOADER::getHatchCodeAngle( const HATCHCODE_ID& aCadstarHatchcodeID )
{
    checkAndLogHatchCode( aCadstarHatchcodeID );
    HATCHCODE hcode = getHatchCode( aCadstarHatchcodeID );

    if( hcode.Hatches.size() < 1 )
        return m_board->GetDesignSettings().GetDefaultZoneSettings().m_HatchOrientation;
    else
        return getAngle( hcode.Hatches.at( 0 ).OrientAngle );
}


int CADSTAR_PCB_ARCHIVE_LOADER::getKiCadHatchCodeThickness(
        const HATCHCODE_ID& aCadstarHatchcodeID )
{
    checkAndLogHatchCode( aCadstarHatchcodeID );
    HATCHCODE hcode = getHatchCode( aCadstarHatchcodeID );

    if( hcode.Hatches.size() < 1 )
        return m_board->GetDesignSettings().GetDefaultZoneSettings().m_HatchThickness;
    else
        return getKiCadLength( hcode.Hatches.at( 0 ).LineWidth );
}


int CADSTAR_PCB_ARCHIVE_LOADER::getKiCadHatchCodeGap( const HATCHCODE_ID& aCadstarHatchcodeID )
{
    checkAndLogHatchCode( aCadstarHatchcodeID );
    HATCHCODE hcode = getHatchCode( aCadstarHatchcodeID );

    if( hcode.Hatches.size() < 1 )
        return m_board->GetDesignSettings().GetDefaultZoneSettings().m_HatchGap;
    else
        return getKiCadLength( hcode.Hatches.at( 0 ).Step );
}


PCB_GROUP* CADSTAR_PCB_ARCHIVE_LOADER::getKiCadGroup( const GROUP_ID& aCadstarGroupID )
{
    wxCHECK( m_groupMap.find( aCadstarGroupID ) != m_groupMap.end(), nullptr );

    return m_groupMap.at( aCadstarGroupID );
}


void CADSTAR_PCB_ARCHIVE_LOADER::checkAndLogHatchCode( const HATCHCODE_ID& aCadstarHatchcodeID )
{
    if( m_hatchcodesTested.find( aCadstarHatchcodeID ) != m_hatchcodesTested.end() )
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
                           "hatch. KiCad only supports one width for the hatching. The imported "
                           "hatching uses the width defined in the first hatch definition, i.e. "
                           "%.2f mm." ),    //format:allow
                        hcode.Name,
                        (double) ( (double) getKiCadLength( hcode.Hatches.at( 0 ).LineWidth ) )
                                / 1E6 ) );
            }

            if( hcode.Hatches.at( 0 ).Step != hcode.Hatches.at( 1 ).Step )
            {
                wxLogWarning( wxString::Format(
                        _( "The CADSTAR Hatching code '%s' has different step sizes for each "
                           "hatch. KiCad only supports one step size for the hatching. The imported "
                           "hatching uses the step size defined in the first hatching definition, "
                           "i.e. %.2f mm." ), //format:allow
                        hcode.Name,
                        (double) ( (double) getKiCadLength( hcode.Hatches.at( 0 ).Step ) )
                                / 1E6 ) );
            }

            if( abs( hcode.Hatches.at( 0 ).OrientAngle - hcode.Hatches.at( 1 ).OrientAngle )
                    != 90000 )
            {
                wxLogWarning( wxString::Format(
                        _( "The hatches in CADSTAR Hatching code '%s' have an angle  "
                           "difference of %.1f degrees. KiCad only supports hatching 90 "   //format:allow
                           "degrees apart.  The imported hatching has two hatches 90 "
                           "degrees apart, oriented %.1f degrees from horizontal." ),       //format:allow
                        hcode.Name,
                        getAngle( abs( hcode.Hatches.at( 0 ).OrientAngle
                                         - hcode.Hatches.at( 1 ).OrientAngle ) ).AsDegrees(),
                        getAngle( hcode.Hatches.at( 0 ).OrientAngle ).AsDegrees() ) );
            }
        }

        m_hatchcodesTested.insert( aCadstarHatchcodeID );
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::applyDimensionSettings( const DIMENSION&  aCadstarDim,
                                                         PCB_DIMENSION_BASE* aKiCadDim )
{
    UNITS dimensionUnits = aCadstarDim.LinearUnits;
    LINECODE linecode = Assignments.Codedefs.LineCodes.at( aCadstarDim.Line.LineCodeID );

    aKiCadDim->SetLayer( getKiCadLayer( aCadstarDim.LayerID ) );
    aKiCadDim->SetPrecision( static_cast<DIM_PRECISION>( aCadstarDim.Precision ) );
    aKiCadDim->SetStart( getKiCadPoint( aCadstarDim.ExtensionLineParams.Start ) );
    aKiCadDim->SetEnd( getKiCadPoint( aCadstarDim.ExtensionLineParams.End ) );
    aKiCadDim->SetExtensionOffset( getKiCadLength( aCadstarDim.ExtensionLineParams.Offset ) );
    aKiCadDim->SetLineThickness( getKiCadLength( linecode.Width ) );

    applyTextCode( aKiCadDim, aCadstarDim.Text.TextCodeID );

    // Find prefix and suffix:
    wxString prefix = wxEmptyString;
    wxString suffix = wxEmptyString;
    size_t   startpos = aCadstarDim.Text.Text.Find( wxT( "<@DISTANCE" ) );

    if( startpos != wxNOT_FOUND )
    {
        prefix = ParseTextFields( aCadstarDim.Text.Text.SubString( 0, startpos - 1 ), &m_context );
        wxString remainingStr = aCadstarDim.Text.Text.Mid( startpos );
        size_t   endpos = remainingStr.Find( "@>" );
        suffix = ParseTextFields( remainingStr.Mid( endpos + 2 ), &m_context );
    }

    if( suffix.StartsWith( wxT( "mm" ) ) )
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
        aKiCadDim->SetPrecision( static_cast<DIM_PRECISION>( Assignments.Technology.UnitDisplPrecision ) );
        dimensionUnits = Assignments.Technology.Units;
    }

    switch( dimensionUnits )
    {
    case UNITS::METER:
    case UNITS::CENTIMETER:
    case UNITS::MICROMETRE:
        wxLogWarning( wxString::Format( _( "Dimension ID %s uses a type of unit that "
                                           "is not supported in KiCad. Millimeters were "
                                           "applied instead." ),
                                        aCadstarDim.ID ) );
        KI_FALLTHROUGH;
    case UNITS::MM:
        aKiCadDim->SetUnitsMode( DIM_UNITS_MODE::MM );
        break;

    case UNITS::INCH:
        aKiCadDim->SetUnitsMode( DIM_UNITS_MODE::INCH );
        break;

    case UNITS::THOU:
        aKiCadDim->SetUnitsMode( DIM_UNITS_MODE::MILS );
        break;

    case UNITS::DESIGN:
        wxFAIL_MSG( wxT( "We should have handled design units before coming here!" ) );
        break;
    }
}

bool CADSTAR_PCB_ARCHIVE_LOADER::calculateZonePriorities( PCB_LAYER_ID& aLayer )
{
    std::map<TEMPLATE_ID, std::set<TEMPLATE_ID>> winningOverlaps;

    auto inflateValue =
        [&]( ZONE* aZoneA, ZONE* aZoneB )
        {
            int extra = getKiCadLength( Assignments.Codedefs.SpacingCodes.at( wxT( "C_C" ) ).Spacing )
                        - m_board->GetDesignSettings().m_MinClearance;

            int retval = std::max( aZoneA->GetLocalClearance().value(),
                                   aZoneB->GetLocalClearance().value() );

            retval += extra;

            return retval;
        };

    // Find the error in fill area when guessing that aHigherZone gets filled before aLowerZone
    auto errorArea =
        [&]( ZONE* aLowerZone, ZONE* aHigherZone ) -> double
        {
            SHAPE_POLY_SET intersectShape( *aHigherZone->Outline() );
            intersectShape.Inflate( inflateValue( aLowerZone, aHigherZone ),
                                    CORNER_STRATEGY::ROUND_ALL_CORNERS, ARC_HIGH_DEF );

            SHAPE_POLY_SET lowerZoneFill( *aLowerZone->GetFilledPolysList( aLayer ) );
            SHAPE_POLY_SET lowerZoneOutline( *aLowerZone->Outline() );

            lowerZoneOutline.BooleanSubtract( intersectShape );

            lowerZoneFill.BooleanSubtract( lowerZoneOutline );

            double leftOverArea = lowerZoneFill.Area();

            return leftOverArea;
        };

    auto intersectionAreaOfZoneOutlines =
        [&]( ZONE* aZoneA, ZONE* aZoneB ) -> double
        {
            SHAPE_POLY_SET outLineA( *aZoneA->Outline() );
            outLineA.Inflate( inflateValue( aZoneA, aZoneB ), CORNER_STRATEGY::ROUND_ALL_CORNERS,
                              ARC_HIGH_DEF );

            SHAPE_POLY_SET outLineB( *aZoneA->Outline() );
            outLineB.Inflate( inflateValue( aZoneA, aZoneB ), CORNER_STRATEGY::ROUND_ALL_CORNERS,
                              ARC_HIGH_DEF );

            outLineA.BooleanIntersection( outLineB );

            return outLineA.Area();
        };

    // Lambda to determine if the zone with template ID 'a' is lower priority than 'b'
    auto isLowerPriority =
        [&]( const TEMPLATE_ID& a, const TEMPLATE_ID& b ) -> bool
        {
            return winningOverlaps[b].count( a ) > 0;
        };

    for( std::map<TEMPLATE_ID, ZONE*>::iterator it1 = m_zonesMap.begin();
         it1 != m_zonesMap.end(); ++it1 )
    {
        TEMPLATE& thisTemplate = Layout.Templates.at( it1->first );
        ZONE*     thisZone = it1->second;

        if( !thisZone->GetLayerSet().Contains( aLayer ) )
            continue;

        for( std::map<TEMPLATE_ID, ZONE*>::iterator it2 = it1;
            it2 != m_zonesMap.end(); ++it2 )
        {
            TEMPLATE& otherTemplate = Layout.Templates.at( it2->first );
            ZONE*     otherZone = it2->second;

            if( thisTemplate.ID == otherTemplate.ID )
                continue;

            if( !otherZone->GetLayerSet().Contains( aLayer ) )
            {
                checkPoint();
                continue;
            }

            if( intersectionAreaOfZoneOutlines( thisZone, otherZone ) == 0 )
            {
                checkPoint();
                continue; // The zones do not interact in any way
            }

            SHAPE_POLY_SET thisZonePolyFill = *thisZone->GetFilledPolysList( aLayer );
            SHAPE_POLY_SET otherZonePolyFill = *otherZone->GetFilledPolysList( aLayer );

            if( thisZonePolyFill.Area() > 0.0 && otherZonePolyFill.Area() > 0.0 )
            {
                // Test if this zone were lower priority than other zone, what is the error?
                double areaThis = errorArea( thisZone, otherZone );
                // Vice-versa
                double areaOther = errorArea( otherZone, thisZone );

                if( areaThis > areaOther )
                {
                    // thisTemplate is filled before otherTemplate
                    winningOverlaps[thisTemplate.ID].insert( otherTemplate.ID );
                }
                else
                {
                    // thisTemplate is filled AFTER otherTemplate
                    winningOverlaps[otherTemplate.ID].insert( thisTemplate.ID );
                }
            }
            else if( thisZonePolyFill.Area() > 0.0 )
            {
                // The other template is not filled, this one wins
                winningOverlaps[thisTemplate.ID].insert( otherTemplate.ID );
            }
            else if( otherZonePolyFill.Area() > 0.0 )
            {
                // This template is not filled, the other one wins
                winningOverlaps[otherTemplate.ID].insert( thisTemplate.ID );
            }
            else
            {
                // Neither of the templates is poured - use zone outlines instead (bigger outlines
                // get a lower priority)
                if( intersectionAreaOfZoneOutlines( thisZone, otherZone ) != 0 )
                {
                    if( thisZone->Outline()->Area() > otherZone->Outline()->Area() )
                        winningOverlaps[otherTemplate.ID].insert( thisTemplate.ID );
                    else
                        winningOverlaps[thisTemplate.ID].insert( otherTemplate.ID );
                }
            }

            checkPoint();
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

        int newPriority = m_zonesMap.at( prevID )->GetAssignedPriority();

        // Only increase priority of the current zone
        if( isLowerPriority( prevID, id ) )
            newPriority++;

        m_zonesMap.at( id )->SetAssignedPriority( newPriority );
        prevID = id;
    }

    // Verify
    for( const std::pair<TEMPLATE_ID, std::set<TEMPLATE_ID>>& idPair : winningOverlaps )
    {
        const TEMPLATE_ID& winningID = idPair.first;

        for( const TEMPLATE_ID& losingID : idPair.second )
        {
            if( m_zonesMap.at( losingID )->GetAssignedPriority()
                > m_zonesMap.at( winningID )->GetAssignedPriority() )
            {
                return false;
            }
        }
    }

    return true;
}


FOOTPRINT* CADSTAR_PCB_ARCHIVE_LOADER::getFootprintFromCadstarID(
        const COMPONENT_ID& aCadstarComponentID )
{
    if( m_componentMap.find( aCadstarComponentID ) == m_componentMap.end() )
        return nullptr;
    else
        return m_componentMap.at( aCadstarComponentID );
}


VECTOR2I CADSTAR_PCB_ARCHIVE_LOADER::getKiCadPoint( const VECTOR2I& aCadstarPoint )
{
    VECTOR2I retval;

    retval.x = ( aCadstarPoint.x - m_designCenter.x ) * KiCadUnitMultiplier;
    retval.y = -( aCadstarPoint.y - m_designCenter.y ) * KiCadUnitMultiplier;

    return retval;
}


NETINFO_ITEM* CADSTAR_PCB_ARCHIVE_LOADER::getKiCadNet( const NET_ID& aCadstarNetID )
{
    if( aCadstarNetID.IsEmpty() )
    {
        return nullptr;
    }
    else if( m_netMap.find( aCadstarNetID ) != m_netMap.end() )
    {
        return m_netMap.at( aCadstarNetID );
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
                newName << wxT( "-Pad" ) << wxString::Format( wxT( "%ld" ), firstPin.PadID );
                newName << wxT( ")" );
            }
            else
            {
                wxFAIL_MSG( wxT( "A net with no pins associated?" ) );
                newName = wxT( "csNet-" );
                newName << wxString::Format( wxT( "%i" ), csNet.SignalNum );
            }
        }

        if( !m_doneNetClassWarning && !csNet.NetClassID.IsEmpty()
                && csNet.NetClassID != wxT( "NONE" ) )
        {
            wxLogMessage( _( "The CADSTAR design contains nets with a 'Net Class' assigned. KiCad "
                             "does not have an equivalent to CADSTAR's Net Class so these elements "
                             "were not imported. Note: KiCad's version of 'Net Class' is closer to "
                             "CADSTAR's 'Net Route Code' (which has been imported for all nets)." ) );
            m_doneNetClassWarning = true;
        }

        if( !m_doneSpacingClassWarning && !csNet.SpacingClassID.IsEmpty()
                && csNet.SpacingClassID != wxT( "NONE" ) )
        {
            wxLogWarning( _( "The CADSTAR design contains nets with a 'Spacing Class' assigned. "
                             "KiCad does not have an equivalent to CADSTAR's Spacing Class so "
                             "these elements were not imported. Please review the design rules as "
                             "copper pours will affected by this." ) );
            m_doneSpacingClassWarning = true;
        }

        std::shared_ptr<NET_SETTINGS>& netSettings = m_board->GetDesignSettings().m_NetSettings;
        NETINFO_ITEM*                  netInfo = new NETINFO_ITEM( m_board, newName, ++m_numNets );
        std::shared_ptr<NETCLASS>      netclass;

        std::tuple<ROUTECODE_ID, NETCLASS_ID, SPACING_CLASS_ID> key = { csNet.RouteCodeID,
                                                                        csNet.NetClassID,
                                                                        csNet.SpacingClassID };

        if( m_netClassMap.find( key ) != m_netClassMap.end() )
        {
            netclass = m_netClassMap.at( key );
        }
        else
        {
            wxString  netClassName;

            ROUTECODE rc = getRouteCode( csNet.RouteCodeID );
            netClassName += wxT( "Route code: " ) + rc.Name;

            if( !csNet.NetClassID.IsEmpty() )
            {
                CADSTAR_NETCLASS nc = Assignments.Codedefs.NetClasses.at( csNet.NetClassID );
                netClassName += wxT( " | Net class: " ) + nc.Name;
            }

            if( !csNet.SpacingClassID.IsEmpty() )
            {
                SPCCLASSNAME sp = Assignments.Codedefs.SpacingClassNames.at( csNet.SpacingClassID );
                netClassName += wxT( " | Spacing class: " ) + sp.Name;
            }

            netclass.reset( new NETCLASS( netClassName ) );
            netSettings->SetNetclass( netClassName, netclass );
            netclass->SetTrackWidth( getKiCadLength( rc.OptimalWidth ) );
            m_netClassMap.insert( { key, netclass } );
        }

        m_board->GetDesignSettings().m_NetSettings->SetNetclassPatternAssignment( newName, netclass->GetName() );

        netInfo->SetNetClass( netclass );
        m_board->Add( netInfo, ADD_MODE::APPEND );
        m_netMap.insert( { aCadstarNetID, netInfo } );
        return netInfo;
    }

    return nullptr;
}


PCB_LAYER_ID CADSTAR_PCB_ARCHIVE_LOADER::getKiCadCopperLayerID( unsigned int aLayerNum,
                                                                bool aDetectMaxLayer )
{
    if( aDetectMaxLayer && aLayerNum == m_numCopperLayers )
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
    if( getLayerType( aCadstarLayerID ) == LAYER_TYPE::NOLAYER )
    {
        //The "no layer" is common for CADSTAR documentation symbols
        //map it to undefined layer for later processing
        return PCB_LAYER_ID::UNDEFINED_LAYER;
    }

    wxCHECK( m_layermap.find( aCadstarLayerID ) != m_layermap.end(),
             PCB_LAYER_ID::UNDEFINED_LAYER );

    return m_layermap.at( aCadstarLayerID );
}


LSET CADSTAR_PCB_ARCHIVE_LOADER::getKiCadLayerSet( const LAYER_ID& aCadstarLayerID )
{
    LAYER_TYPE layerType = getLayerType( aCadstarLayerID );

    switch( layerType )
    {
    case LAYER_TYPE::ALLDOC:
        return LSET( { PCB_LAYER_ID::Dwgs_User,
                       PCB_LAYER_ID::Cmts_User,
                       PCB_LAYER_ID::Eco1_User,
                       PCB_LAYER_ID::Eco2_User } )
                | LSET::UserDefinedLayersMask();

    case LAYER_TYPE::ALLELEC:
        return LSET::AllCuMask( m_numCopperLayers );

    case LAYER_TYPE::ALLLAYER:
        return LSET::AllCuMask( m_numCopperLayers )
                | LSET( { PCB_LAYER_ID::Dwgs_User,
                          PCB_LAYER_ID::Cmts_User,
                          PCB_LAYER_ID::Eco1_User,
                          PCB_LAYER_ID::Eco2_User } )
                | LSET::UserDefinedLayersMask()
                | LSET::AllBoardTechMask();

    default:
        return LSET( { getKiCadLayer( aCadstarLayerID ) } );
    }
}


void CADSTAR_PCB_ARCHIVE_LOADER::addToGroup(
        const GROUP_ID& aCadstarGroupID, BOARD_ITEM* aKiCadItem )
{
    wxCHECK( m_groupMap.find( aCadstarGroupID ) != m_groupMap.end(), );

    PCB_GROUP* parentGroup = m_groupMap.at( aCadstarGroupID );
    parentGroup->AddItem( aKiCadItem );
}


CADSTAR_PCB_ARCHIVE_LOADER::GROUP_ID CADSTAR_PCB_ARCHIVE_LOADER::createUniqueGroupID(
        const wxString& aName )
{
    wxString groupName = aName;
    int      num       = 0;

    while( m_groupMap.find( groupName ) != m_groupMap.end() )
    {
        groupName = aName + wxT( "_" ) + wxString::Format( wxT( "%i" ), ++num );
    }

    PCB_GROUP* docSymGroup = new PCB_GROUP( m_board );
    m_board->Add( docSymGroup );
    docSymGroup->SetName( groupName );
    GROUP_ID groupID( groupName );
    m_groupMap.insert( { groupID, docSymGroup } );

    return groupID;
}
