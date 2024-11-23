/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
 * Author: SYSUEric <jzzhuang666@gmail.com>.
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


#include <base_units.h>
#include <board_stackup_manager/stackup_predefined_prms.h>
#include <build_version.h>
#include <callback_gal.h>
#include <connectivity/connectivity_data.h>
#include <connectivity/connectivity_algo.h>
#include <convert_basic_shapes_to_polygon.h>
#include <font/font.h>
#include <footprint.h>
#include <hash_eda.h>
#include <pad.h>
#include <pcb_dimension.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <pcb_textbox.h>
#include <pcb_track.h>
#include <pcbnew_settings.h>
#include <board_design_settings.h>
#include <pgm_base.h>
#include <progress_reporter.h>
#include <settings/settings_manager.h>
#include <wx_fstream_progress.h>

#include <geometry/shape_circle.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_poly_set.h>
#include <geometry/shape_segment.h>

#include <wx/log.h>
#include <wx/numformatter.h>
#include <wx/mstream.h>

#include "odb_attribute.h"
#include "odb_entity.h"
#include "odb_defines.h"
#include "odb_feature.h"
#include "odb_util.h"
#include "pcb_io_odbpp.h"


bool ODB_ENTITY_BASE::CreateDirectoryTree( ODB_TREE_WRITER& writer )
{
    try
    {
        writer.CreateEntityDirectory( writer.GetRootPath(), GetEntityName() );
        return true;
    }
    catch( const std::exception& e )
    {
        std::cerr << e.what() << std::endl;
        return false;
    }
}


ODB_MISC_ENTITY::ODB_MISC_ENTITY()
{
    m_info = { { wxS( ODB_JOB_NAME ), wxS( "job" ) },
               { wxS( ODB_UNITS ), PCB_IO_ODBPP::m_unitsStr },
               { wxS( "ODB_VERSION_MAJOR" ), wxS( "8" ) },
               { wxS( "ODB_VERSION_MINOR" ), wxS( "1" ) },
               { wxS( "ODB_SOURCE" ), wxS( "KiCad EDA" ) },
               { wxS( "CREATION_DATE" ), wxDateTime::Now().FormatISOCombined() },
               { wxS( "SAVE_DATE" ), wxDateTime::Now().FormatISOCombined() },
               { wxS( "SAVE_APP" ), wxString::Format( wxS( "KiCad EDA %s" ), GetBuildVersion() ) } };
}


void ODB_MISC_ENTITY::GenerateFiles( ODB_TREE_WRITER& writer )
{
    auto fileproxy = writer.CreateFileProxy( "info" );

    ODB_TEXT_WRITER twriter( fileproxy.GetStream() );

    for( auto& info : m_info )
    {
        twriter.WriteEquationLine( info.first, info.second );
    }
}


void ODB_MATRIX_ENTITY::AddStep( const wxString& aStepName )
{
    m_matrixSteps.emplace( aStepName.Upper(), m_col++ );
}


void ODB_MATRIX_ENTITY::InitEntityData()
{
    AddStep( "PCB" );

    InitMatrixLayerData();
}


void ODB_MATRIX_ENTITY::InitMatrixLayerData()
{
    BOARD_DESIGN_SETTINGS& dsnSettings = m_board->GetDesignSettings();
    BOARD_STACKUP&         stackup = dsnSettings.GetStackupDescriptor();
    stackup.SynchronizeWithBoard( &dsnSettings );

    std::vector<BOARD_STACKUP_ITEM*> layers = stackup.GetList();
    std::set<PCB_LAYER_ID>           added_layers;

    AddCOMPMatrixLayer( F_Cu );

    for( int i = 0; i < stackup.GetCount(); i++ )
    {
        BOARD_STACKUP_ITEM* stackup_item = layers.at( i );

        for( int sublayer_id = 0; sublayer_id < stackup_item->GetSublayersCount(); sublayer_id++ )
        {
            wxString ly_name = stackup_item->GetLayerName();

            if( ly_name.IsEmpty() )
            {
                if( IsValidLayer( stackup_item->GetBrdLayerId() ) )
                    ly_name = m_board->GetLayerName( stackup_item->GetBrdLayerId() );

                if( ly_name.IsEmpty() && stackup_item->GetType() == BS_ITEM_TYPE_DIELECTRIC )
                    ly_name = wxString::Format( "DIELECTRIC_%d",
                                                stackup_item->GetDielectricLayerId() );
            }

            MATRIX_LAYER matrix( m_row++, ly_name );

            if( stackup_item->GetType() == BS_ITEM_TYPE_DIELECTRIC )
            {
                if( stackup_item->GetTypeName() == KEY_CORE )
                    matrix.m_diType.emplace( ODB_DIELECTRIC_TYPE::CORE );
                else
                    matrix.m_diType.emplace( ODB_DIELECTRIC_TYPE::PREPREG );

                matrix.m_type = ODB_TYPE::DIELECTRIC;
                matrix.m_context = ODB_CONTEXT::BOARD;
                matrix.m_polarity = ODB_POLARITY::POSITIVE;
                m_matrixLayers.push_back( matrix );
                m_plugin->GetLayerNameList().emplace_back(
                        std::make_pair( PCB_LAYER_ID::UNDEFINED_LAYER, matrix.m_layerName ) );

                continue;
            }
            else
            {
                added_layers.insert( stackup_item->GetBrdLayerId() );
                AddMatrixLayerField( matrix, stackup_item->GetBrdLayerId() );
            }
        }
    }

    LSEQ layer_seq = m_board->GetEnabledLayers().Seq();

    for( PCB_LAYER_ID layer : layer_seq )
    {
        if( added_layers.find( layer ) != added_layers.end() )
            continue;

        MATRIX_LAYER matrix( m_row++, m_board->GetLayerName( layer ) );
        added_layers.insert( layer );
        AddMatrixLayerField( matrix, layer );
    }

    AddDrillMatrixLayer();

    AddCOMPMatrixLayer( B_Cu );
}


void ODB_MATRIX_ENTITY::AddMatrixLayerField( MATRIX_LAYER& aMLayer, PCB_LAYER_ID aLayer )
{
    aMLayer.m_polarity = ODB_POLARITY::POSITIVE;
    aMLayer.m_context = ODB_CONTEXT::BOARD;
    switch( aLayer )
    {
    case F_Paste:
    case B_Paste: aMLayer.m_type = ODB_TYPE::SOLDER_PASTE; break;
    case F_SilkS:
    case B_SilkS: aMLayer.m_type = ODB_TYPE::SILK_SCREEN; break;
    case F_Mask:
    case B_Mask: aMLayer.m_type = ODB_TYPE::SOLDER_MASK; break;
    case B_CrtYd:
    case F_CrtYd:
    case Edge_Cuts:
    case B_Fab:
    case F_Fab:
    case F_Adhes:
    case B_Adhes:
    case Dwgs_User:
    case Cmts_User:
    case Eco1_User:
    case Eco2_User:
    case Margin:
    case User_1:
    case User_2:
    case User_3:
    case User_4:
    case User_5:
    case User_6:
    case User_7:
    case User_8:
    case User_9:
        aMLayer.m_context = ODB_CONTEXT::MISC;
        aMLayer.m_type = ODB_TYPE::DOCUMENT;
        break;

    default:
        if( IsCopperLayer( aLayer ) )
        {
            aMLayer.m_type = ODB_TYPE::SIGNAL;
        }
        else
        {
            // Do not handle other layers :
            aMLayer.m_type = ODB_TYPE::UNDEFINED;
            m_row--;
        }

        break;
    }

    if( aMLayer.m_type != ODB_TYPE::UNDEFINED )
    {
        m_matrixLayers.push_back( aMLayer );
        m_plugin->GetLayerNameList().emplace_back( std::make_pair( aLayer, aMLayer.m_layerName ) );
    }
}


void ODB_MATRIX_ENTITY::AddDrillMatrixLayer()
{
    std::map<std::pair<PCB_LAYER_ID, PCB_LAYER_ID>, std::vector<BOARD_ITEM*>>& drill_layers =
            m_plugin->GetDrillLayerItemsMap();

    std::map<std::pair<PCB_LAYER_ID, PCB_LAYER_ID>, std::vector<BOARD_ITEM*>>& slot_holes =
            m_plugin->GetSlotHolesMap();

    bool has_pth_layer = false;
    bool has_npth_layer = false;

    for( BOARD_ITEM* item : m_board->Tracks() )
    {
        if( item->Type() == PCB_VIA_T )
        {
            PCB_VIA* via = static_cast<PCB_VIA*>( item );
            drill_layers[std::make_pair( via->TopLayer(), via->BottomLayer() )].push_back( via );
        }
    }

    for( FOOTPRINT* fp : m_board->Footprints() )
    {
        // std::shared_ptr<FOOTPRINT> fp( static_cast<FOOTPRINT*>( it_fp->Clone() ) );

        if( fp->IsFlipped() )
        {
            m_hasBotComp = true;
        }

        for( PAD* pad : fp->Pads() )
        {
            if( !has_pth_layer && pad->GetAttribute() == PAD_ATTRIB::PTH )
                has_pth_layer = true;
            if( !has_npth_layer && pad->GetAttribute() == PAD_ATTRIB::NPTH )
                has_npth_layer = true;

            if( pad->HasHole() && pad->GetDrillSizeX() != pad->GetDrillSizeY() )
                slot_holes[std::make_pair( F_Cu, B_Cu )].push_back( pad );
            else if( pad->HasHole() )
                drill_layers[std::make_pair( F_Cu, B_Cu )].push_back( pad );
        }

        // m_plugin->GetLoadedFootprintList().push_back( std::move( fp ) );
    }

    auto InitDrillMatrix =
            [&]( const wxString& aHasPlated, std::pair<PCB_LAYER_ID, PCB_LAYER_ID> aLayerPair )
    {
        wxString     dLayerName = wxString::Format( "drill_%s_%s-%s", aHasPlated,
                                                    m_board->GetLayerName( aLayerPair.first ),
                                                    m_board->GetLayerName( aLayerPair.second ) );
        MATRIX_LAYER matrix( m_row++, dLayerName );

        matrix.m_type = ODB_TYPE::DRILL;
        matrix.m_context = ODB_CONTEXT::BOARD;
        matrix.m_polarity = ODB_POLARITY::POSITIVE;
        matrix.m_span.emplace( std::make_pair(
                ODB::GenLegalEntityName( m_board->GetLayerName( aLayerPair.first ) ),
                ODB::GenLegalEntityName( m_board->GetLayerName( aLayerPair.second ) ) ) );
        m_matrixLayers.push_back( matrix );
        m_plugin->GetLayerNameList().emplace_back(
                std::make_pair( PCB_LAYER_ID::UNDEFINED_LAYER, matrix.m_layerName ) );
    };

    if( drill_layers.find( std::make_pair( F_Cu, B_Cu ) ) != drill_layers.end()
        || !slot_holes.empty() )
    {
        // for pad has hole
        if( has_pth_layer )
            InitDrillMatrix( "plated", std::make_pair( F_Cu, B_Cu ) );
        if( has_npth_layer )
            InitDrillMatrix( "non-plated", std::make_pair( F_Cu, B_Cu ) );
    }

    for( const auto& [layer_pair, vec] : drill_layers )
    {
        if( layer_pair != std::make_pair( F_Cu, B_Cu ) ) // pad has initialized above
            InitDrillMatrix( "plated", layer_pair );     // for via
    }
}


void ODB_MATRIX_ENTITY::AddCOMPMatrixLayer( PCB_LAYER_ID aCompSide )
{
    MATRIX_LAYER matrix( m_row++, "COMP_+_TOP" );
    matrix.m_type = ODB_TYPE::COMPONENT;
    matrix.m_context = ODB_CONTEXT::BOARD;

    if( aCompSide == F_Cu )
    {
        m_matrixLayers.push_back( matrix );
        m_plugin->GetLayerNameList().emplace_back(
                std::make_pair( PCB_LAYER_ID::UNDEFINED_LAYER, matrix.m_layerName ) );
    }

    if( aCompSide == B_Cu && m_hasBotComp )
    {
        matrix.m_layerName = ODB::GenLegalEntityName( "COMP_+_BOT" );
        m_matrixLayers.push_back( matrix );
        m_plugin->GetLayerNameList().emplace_back(
                std::make_pair( PCB_LAYER_ID::UNDEFINED_LAYER, matrix.m_layerName ) );
    }
}


void ODB_MATRIX_ENTITY::GenerateFiles( ODB_TREE_WRITER& writer )
{
    auto fileproxy = writer.CreateFileProxy( "matrix" );

    ODB_TEXT_WRITER twriter( fileproxy.GetStream() );

    for( const auto& [step_name, column] : m_matrixSteps )
    {
        const auto array_proxy = twriter.MakeArrayProxy( "STEP" );
        twriter.WriteEquationLine( "COL", column );
        twriter.WriteEquationLine( "NAME", step_name );
    }

    for( const MATRIX_LAYER& layer : m_matrixLayers )
    {
        const auto array_proxy = twriter.MakeArrayProxy( "LAYER" );
        twriter.WriteEquationLine( "ROW", layer.m_rowNumber );
        twriter.write_line_enum( "CONTEXT", layer.m_context );
        twriter.write_line_enum( "TYPE", layer.m_type );

        if( layer.m_addType.has_value() )
        {
            twriter.write_line_enum( "ADD_TYPE", layer.m_addType.value() );
        }

        twriter.WriteEquationLine( "NAME", layer.m_layerName.Upper() );
        twriter.WriteEquationLine( "OLD_NAME", wxEmptyString );
        twriter.write_line_enum( "POLARITY", layer.m_polarity );

        if( layer.m_diType.has_value() )
        {
            twriter.write_line_enum( "DIELECTRIC_TYPE", layer.m_diType.value() );
            // twriter.WriteEquationLine( "DIELECTRIC_NAME", wxEmptyString );

            // Can be used with DIELECTRIC_TYPE=CORE
            // twriter.WriteEquationLine( "CU_TOP", wxEmptyString );
            // twriter.WriteEquationLine( "CU_BOTTOM", wxEmptyString );
        }

        // Only applies to: soldermask, silkscreen, solderpaste and specifies the relevant cu layer
        // twriter.WriteEquationLine( "REF", wxEmptyString );

        if( layer.m_span.has_value() )
        {
            twriter.WriteEquationLine( "START_NAME", layer.m_span->first.Upper() );
            twriter.WriteEquationLine( "END_NAME", layer.m_span->second.Upper() );
        }

        twriter.WriteEquationLine( "COLOR", "0" );
    }
}


ODB_LAYER_ENTITY::ODB_LAYER_ENTITY( BOARD* aBoard, PCB_IO_ODBPP* aPlugin,
                                    std::map<int, std::vector<BOARD_ITEM*>>& aMap,
                                    const PCB_LAYER_ID& aLayerID, const wxString& aLayerName ) :
        ODB_ENTITY_BASE( aBoard, aPlugin ), m_layerItems( aMap ), m_layerID( aLayerID ),
        m_matrixLayerName( aLayerName )
{
    m_featuresMgr = std::make_unique<FEATURES_MANAGER>( aBoard, aPlugin, aLayerName );
}


void ODB_LAYER_ENTITY::InitEntityData()
{
    if( m_matrixLayerName.Contains( "drill" ) )
    {
        InitDrillData();
        InitFeatureData();
        return;
    }

    if( m_layerID != PCB_LAYER_ID::UNDEFINED_LAYER )
    {
        InitFeatureData();
    }
}

void ODB_LAYER_ENTITY::InitFeatureData()
{
    if( m_layerItems.empty() )
        return;

    const NETINFO_LIST& nets = m_board->GetNetInfo();

    for( const NETINFO_ITEM* net : nets )
    {
        std::vector<BOARD_ITEM*>& vec = m_layerItems[net->GetNetCode()];

        std::stable_sort( vec.begin(), vec.end(),
                          []( BOARD_ITEM* a, BOARD_ITEM* b )
                          {
                              if( a->GetParentFootprint() == b->GetParentFootprint() )
                                  return a->Type() < b->Type();

                              return a->GetParentFootprint() < b->GetParentFootprint();
                          } );

        if( vec.empty() )
            continue;

        m_featuresMgr->InitFeatureList( m_layerID, vec );
    }
}


ODB_COMPONENT& ODB_LAYER_ENTITY::InitComponentData( const FOOTPRINT*         aFp,
                                                    const EDA_DATA::PACKAGE& aPkg )
{
    if( m_matrixLayerName == "COMP_+_BOT" )
    {
        if( !m_compBot.has_value() )
        {
            m_compBot.emplace();
        }
        return m_compBot.value().AddComponent( aFp, aPkg );
    }
    else
    {
        if( !m_compTop.has_value() )
        {
            m_compTop.emplace();
        }

        return m_compTop.value().AddComponent( aFp, aPkg );
    }
}


void ODB_LAYER_ENTITY::InitDrillData()
{
    std::map<std::pair<PCB_LAYER_ID, PCB_LAYER_ID>, std::vector<BOARD_ITEM*>>& drill_layers =
            m_plugin->GetDrillLayerItemsMap();

    std::map<std::pair<PCB_LAYER_ID, PCB_LAYER_ID>, std::vector<BOARD_ITEM*>>& slot_holes =
            m_plugin->GetSlotHolesMap();

    if( !m_layerItems.empty() )
    {
        m_layerItems.clear();
    }

    m_tools.emplace( PCB_IO_ODBPP::m_unitsStr );

    bool     is_npth_layer = false;
    wxString plated_name = "plated";

    if( m_matrixLayerName.Contains( "non-plated" ) )
    {
        is_npth_layer = true;
        plated_name = "non-plated";
    }


    for( const auto& [layer_pair, vec] : slot_holes )
    {
        wxString dLayerName = wxString::Format( "drill_%s_%s-%s", plated_name,
                                                m_board->GetLayerName( layer_pair.first ),
                                                m_board->GetLayerName( layer_pair.second ) );

        if( ODB::GenLegalEntityName( dLayerName ) == m_matrixLayerName )
        {
            for( BOARD_ITEM* item : vec )
            {
                if( item->Type() == PCB_PAD_T )
                {
                    PAD* pad = static_cast<PAD*>( item );

                    if( ( is_npth_layer && pad->GetAttribute() == PAD_ATTRIB::PTH )
                        || ( !is_npth_layer && pad->GetAttribute() == PAD_ATTRIB::NPTH ) )
                    {
                        continue;
                    }

                    m_tools.value().AddDrillTools(
                            pad->GetAttribute() == PAD_ATTRIB::PTH ? "PLATED" : "NON_PLATED",
                            ODB::SymDouble2String(
                                    std::min( pad->GetDrillSizeX(), pad->GetDrillSizeY() ) ) );

                    // for drill features
                    m_layerItems[pad->GetNetCode()].push_back( item );
                }
            }

            break;
        }
    }

    for( const auto& [layer_pair, vec] : drill_layers )
    {
        wxString dLayerName = wxString::Format( "drill_%s_%s-%s", plated_name,
                                                m_board->GetLayerName( layer_pair.first ),
                                                m_board->GetLayerName( layer_pair.second ) );

        if( ODB::GenLegalEntityName( dLayerName ) == m_matrixLayerName )
        {
            for( BOARD_ITEM* item : vec )
            {
                if( item->Type() == PCB_VIA_T && !is_npth_layer )
                {
                    PCB_VIA* via = static_cast<PCB_VIA*>( item );

                    m_tools.value().AddDrillTools( "VIA",
                                                   ODB::SymDouble2String( via->GetDrillValue() ) );

                    // for drill features
                    m_layerItems[via->GetNetCode()].push_back( item );
                }
                else if( item->Type() == PCB_PAD_T )
                {
                    PAD* pad = static_cast<PAD*>( item );

                    if( ( is_npth_layer && pad->GetAttribute() == PAD_ATTRIB::PTH )
                        || ( !is_npth_layer && pad->GetAttribute() == PAD_ATTRIB::NPTH ) )
                    {
                        continue;
                    }

                    m_tools.value().AddDrillTools(
                            pad->GetAttribute() == PAD_ATTRIB::PTH ? "PLATED" : "NON_PLATED",
                            ODB::SymDouble2String( pad->GetDrillSizeX() ) );

                    // for drill features
                    m_layerItems[pad->GetNetCode()].push_back( item );
                }
            }

            break;
        }
    }
}


void ODB_STEP_ENTITY::InitEntityData()
{
    MakeLayerEntity();

    InitEdaData();

    // Init Layer Entity Data
    for( const auto& [layerName, layer_entity_ptr] : m_layerEntityMap )
    {
        layer_entity_ptr->InitEntityData();
    }
}


void ODB_LAYER_ENTITY::GenerateFiles( ODB_TREE_WRITER& writer )
{
    GenAttrList( writer );

    GenFeatures( writer );

    if( m_compTop.has_value() || m_compBot.has_value() )
    {
        GenComponents( writer );
    }

    if( m_tools.has_value() )
    {
        GenTools( writer );
    }
}


void ODB_LAYER_ENTITY::GenComponents( ODB_TREE_WRITER& writer )
{
    auto fileproxy = writer.CreateFileProxy( "components" );

    if( m_compTop.has_value() )
    {
        m_compTop->Write( fileproxy.GetStream() );
    }
    else if( m_compBot.has_value() )
    {
        m_compBot->Write( fileproxy.GetStream() );
    }
}


void ODB_LAYER_ENTITY::GenFeatures( ODB_TREE_WRITER& writer )
{
    auto fileproxy = writer.CreateFileProxy( "features" );

    m_featuresMgr->GenerateFeatureFile( fileproxy.GetStream() );
}


void ODB_LAYER_ENTITY::GenAttrList( ODB_TREE_WRITER& writer )
{
    auto fileproxy = writer.CreateFileProxy( "attrlist" );
}


void ODB_LAYER_ENTITY::GenTools( ODB_TREE_WRITER& writer )
{
    auto fileproxy = writer.CreateFileProxy( "tools" );

    m_tools.value().GenerateFile( fileproxy.GetStream() );
}


void ODB_STEP_ENTITY::InitEdaData()
{
    //InitPackage
    for( const FOOTPRINT* fp : m_board->Footprints() )
    {
        m_edaData.AddPackage( fp );
    }

    // for NET
    const NETINFO_LIST& nets = m_board->GetNetInfo();

    for( const NETINFO_ITEM* net : nets )
    {
        m_edaData.AddNET( net );
    }

    // for CMP
    size_t j = 0;

    for( const FOOTPRINT* fp : m_board->Footprints() )
    {
        wxString compName = ODB::GenLegalEntityName( "COMP_+_TOP" );
        if( fp->IsFlipped() )
            compName = ODB::GenLegalEntityName( "COMP_+_BOT" );

        auto iter = m_layerEntityMap.find( compName );

        if( iter == m_layerEntityMap.end() )
        {
            wxLogError( _( "Failed to add component data" ) );
            return;
        }

        // ODBPP only need unique PACKAGE in PKG record in eda/data file.
        // the PKG index can repeat to be ref in CMP record in component file.
        std::shared_ptr<FOOTPRINT> fp_pkg = m_edaData.GetEdaFootprints().at( j );
        ++j;

        const EDA_DATA::PACKAGE& eda_pkg =
                m_edaData.GetPackage( hash_fp_item( fp_pkg.get(), HASH_POS | REL_COORD ) );

        ODB_COMPONENT& comp = iter->second->InitComponentData( fp, eda_pkg );

        for( int i = 0; i < fp->Pads().size(); ++i )
        {
            PAD*  pad = fp->Pads()[i];
            auto& eda_net = m_edaData.GetNet( pad->GetNetCode() );

            auto& subnet = eda_net.AddSubnet<EDA_DATA::SUB_NET_TOEPRINT>(
                    &m_edaData,
                    fp->IsFlipped() ? EDA_DATA::SUB_NET_TOEPRINT::SIDE::BOTTOM
                                    : EDA_DATA::SUB_NET_TOEPRINT::SIDE::TOP,
                    comp.m_index, comp.m_toeprints.size() );

            m_plugin->GetPadSubnetMap().emplace( pad, &subnet );

            const std::shared_ptr<EDA_DATA::PIN> pin = eda_pkg.GetEdaPkgPin( i );
            const EDA_DATA::PIN&                 pin_ref = *pin;
            auto&                                toep = comp.m_toeprints.emplace_back( pin_ref );

            toep.m_net_num = eda_net.m_index;
            toep.m_subnet_num = subnet.m_index;

            toep.m_center = ODB::AddXY( pad->GetPosition() );

            toep.m_rot = ODB::Double2String(
                    ( ANGLE_360 - pad->GetOrientation() ).Normalize().AsDegrees() );

            if( pad->IsFlipped() )
                toep.m_mirror = wxT( "M" );
            else
                toep.m_mirror = wxT( "N" );
        }
    }

    for( PCB_TRACK* track : m_board->Tracks() )
    {
        auto&              eda_net = m_edaData.GetNet( track->GetNetCode() );
        EDA_DATA::SUB_NET* subnet = nullptr;

        if( track->Type() == PCB_VIA_T )
            subnet = &( eda_net.AddSubnet<EDA_DATA::SUB_NET_VIA>( &m_edaData ) );
        else
            subnet = &( eda_net.AddSubnet<EDA_DATA::SUB_NET_TRACE>( &m_edaData ) );

        m_plugin->GetViaTraceSubnetMap().emplace( track, subnet );
    }

    for( ZONE* zone : m_board->Zones() )
    {
        for( PCB_LAYER_ID layer : zone->GetLayerSet().Seq() )
        {
            auto& eda_net = m_edaData.GetNet( zone->GetNetCode() );
            auto& subnet = eda_net.AddSubnet<EDA_DATA::SUB_NET_PLANE>(
                    &m_edaData, EDA_DATA::SUB_NET_PLANE::FILL_TYPE::SOLID,
                    EDA_DATA::SUB_NET_PLANE::CUTOUT_TYPE::EXACT, 0 );
            m_plugin->GetPlaneSubnetMap().emplace( std::piecewise_construct,
                                                   std::forward_as_tuple( layer, zone ),
                                                   std::forward_as_tuple( &subnet ) );
        }
    }
}


void ODB_STEP_ENTITY::GenerateFiles( ODB_TREE_WRITER& writer )
{
    wxString step_root = writer.GetCurrentPath();

    writer.CreateEntityDirectory( step_root, "layers" );
    GenerateLayerFiles( writer );

    writer.CreateEntityDirectory( step_root, "eda" );
    GenerateEdaFiles( writer );

    writer.CreateEntityDirectory( step_root, "netlists/cadnet" );
    GenerateNetlistsFiles( writer );

    writer.SetCurrentPath( step_root );
    GenerateProfileFile( writer );

    GenerateStepHeaderFile( writer );

    //TODO: system attributes
    // GenerateAttrListFile( writer );
}


void ODB_STEP_ENTITY::GenerateProfileFile( ODB_TREE_WRITER& writer )
{
    auto fileproxy = writer.CreateFileProxy( "profile" );

    m_profile = std::make_unique<FEATURES_MANAGER>( m_board, m_plugin, wxEmptyString );

    SHAPE_POLY_SET board_outline;

    if( !m_board->GetBoardPolygonOutlines( board_outline ) )
    {
        wxLogError( "Failed to get board outline" );
    }

    if( !m_profile->AddContour( board_outline, 0 ) )
    {
        wxLogError( "Failed to add polygon to profile" );
    }

    m_profile->GenerateProfileFeatures( fileproxy.GetStream() );
}


void ODB_STEP_ENTITY::GenerateStepHeaderFile( ODB_TREE_WRITER& writer )
{
    auto fileproxy = writer.CreateFileProxy( "stephdr" );

    m_stephdr = {
        { ODB_UNITS, PCB_IO_ODBPP::m_unitsStr },
        { "X_DATUM", "0" },
        { "Y_DATUM", "0" },
        { "X_ORIGIN", "0" },
        { "Y_ORIGIN", "0" },
        { "TOP_ACTIVE", "0" },
        { "BOTTOM_ACTIVE", "0" },
        { "RIGHT_ACTIVE", "0" },
        { "LEFT_ACTIVE", "0" },
        { "AFFECTING_BOM", "" },
        { "AFFECTING_BOM_CHANGED", "0" },
    };

    ODB_TEXT_WRITER twriter( fileproxy.GetStream() );

    for( const auto& [key, value] : m_stephdr )
    {
        twriter.WriteEquationLine( key, value );
    }
}


void ODB_STEP_ENTITY::GenerateLayerFiles( ODB_TREE_WRITER& writer )
{
    wxString layers_root = writer.GetCurrentPath();

    for( auto& [layerName, layerEntity] : m_layerEntityMap )
    {
        writer.CreateEntityDirectory( layers_root, layerName );

        layerEntity->GenerateFiles( writer );
    }
}


void ODB_STEP_ENTITY::GenerateEdaFiles( ODB_TREE_WRITER& writer )
{
    auto fileproxy = writer.CreateFileProxy( "data" );

    m_edaData.Write( fileproxy.GetStream() );
}


void ODB_STEP_ENTITY::GenerateNetlistsFiles( ODB_TREE_WRITER& writer )
{
    auto fileproxy = writer.CreateFileProxy( "netlist" );

    m_netlist.Write( fileproxy.GetStream() );
}


bool ODB_STEP_ENTITY::CreateDirectoryTree( ODB_TREE_WRITER& writer )
{
    try
    {
        writer.CreateEntityDirectory( writer.GetRootPath(), "steps" );
        writer.CreateEntityDirectory( writer.GetCurrentPath(), GetEntityName() );
        return true;
    }
    catch( const std::exception& e )
    {
        std::cerr << e.what() << std::endl;
        return false;
    }
}


void ODB_STEP_ENTITY::MakeLayerEntity()
{
    LSEQ                layers = m_board->GetEnabledLayers().Seq();
    const NETINFO_LIST& nets = m_board->GetNetInfo();

    // To avoid the overhead of repeatedly cycling through the layers and nets,
    // we pre-sort the board items into a map of layer -> net -> items
    std::map<PCB_LAYER_ID, std::map<int, std::vector<BOARD_ITEM*>>>& elements =
            m_plugin->GetLayerElementsMap();

    std::for_each( m_board->Tracks().begin(), m_board->Tracks().end(),
                   [&layers, &elements]( PCB_TRACK* aTrack )
                   {
                       if( aTrack->Type() == PCB_VIA_T )
                       {
                           PCB_VIA* via = static_cast<PCB_VIA*>( aTrack );

                           for( PCB_LAYER_ID layer : layers )
                           {
                               if( via->FlashLayer( layer ) )
                                   elements[layer][via->GetNetCode()].push_back( via );
                           }
                       }
                       else
                       {
                           elements[aTrack->GetLayer()][aTrack->GetNetCode()].push_back( aTrack );
                       }
                   } );

    std::for_each( m_board->Zones().begin(), m_board->Zones().end(),
                   [&elements]( ZONE* zone )
                   {
                       LSEQ zone_layers = zone->GetLayerSet().Seq();

                       for( PCB_LAYER_ID layer : zone_layers )
                       {
                           elements[layer][zone->GetNetCode()].push_back( zone );
                       }
                   } );

    for( BOARD_ITEM* item : m_board->Drawings() )
    {
        if( BOARD_CONNECTED_ITEM* conn_it = dynamic_cast<BOARD_CONNECTED_ITEM*>( item ) )
            elements[conn_it->GetLayer()][conn_it->GetNetCode()].push_back( conn_it );
        else
            elements[item->GetLayer()][0].push_back( item );
    }

    for( FOOTPRINT* fp : m_board->Footprints() )
    {
        for( PCB_FIELD* field : fp->GetFields() )
            elements[field->GetLayer()][0].push_back( field );

        for( BOARD_ITEM* item : fp->GraphicalItems() )
            elements[item->GetLayer()][0].push_back( item );

        for( PAD* pad : fp->Pads() )
        {
            LSEQ     pad_layers = pad->GetLayerSet().Seq();
            VECTOR2I margin;

            for( PCB_LAYER_ID layer : pad_layers )
            {
                bool onCopperLayer = ( LSET::AllCuMask() & LSET( { layer } ) ).any();
                bool onSolderMaskLayer = ( LSET( { F_Mask, B_Mask } ) & LSET( { layer } ) ).any();
                bool onSolderPasteLayer =
                        ( LSET( { F_Paste, B_Paste } ) & LSET( { layer } ) ).any();

                if( onSolderMaskLayer )
                    margin.x = margin.y = pad->GetSolderMaskExpansion( PADSTACK::ALL_LAYERS );

                if( onSolderPasteLayer )
                    margin = pad->GetSolderPasteMargin( PADSTACK::ALL_LAYERS );

                VECTOR2I padPlotsSize = pad->GetSize( PADSTACK::ALL_LAYERS ) + margin * 2;

                if( onCopperLayer && !pad->IsOnCopperLayer() )
                    continue;

                if( onCopperLayer && !pad->FlashLayer( layer ) )
                    continue;

                if( pad->GetShape( PADSTACK::ALL_LAYERS ) != PAD_SHAPE::CUSTOM
                    && ( padPlotsSize.x <= 0 || padPlotsSize.y <= 0 ) )
                    continue;

                elements[layer][pad->GetNetCode()].push_back( pad );
            }
        }
    }

    for( const auto& [layerID, layerName] : m_plugin->GetLayerNameList() )
    {
        std::shared_ptr<ODB_LAYER_ENTITY> layer_entity_ptr = std::make_shared<ODB_LAYER_ENTITY>(
                m_board, m_plugin, elements[layerID], layerID, layerName );

        m_layerEntityMap.emplace( layerName, layer_entity_ptr );
    }
}
