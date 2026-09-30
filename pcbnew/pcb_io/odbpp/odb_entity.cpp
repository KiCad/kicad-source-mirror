/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include <base_units.h>
#include <optional>
#include <board.h>
#include <board_stackup_manager/stackup_predefined_prms.h>
#include <build_version.h>
#include <callback_gal.h>
#include <connectivity/connectivity_data.h>
#include <connectivity/connectivity_algo.h>
#include <convert_basic_shapes_to_polygon.h>
#include <exporters/fab_model/fab_item_order.h>
#include <font/font.h>
#include <footprint.h>
#include <hash_eda.h>
#include <drill/drill_enumerator.h>
#include <pad.h>
#include <padstack.h>
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
#include <trace_helpers.h>


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


ODB_MISC_ENTITY::ODB_MISC_ENTITY( BOARD* aBoard, PCB_IO_ODBPP* aPlugin ) :
        ODB_ENTITY_BASE( aBoard, aPlugin )
{
    m_info = { { wxS( ODB_JOB_NAME ), wxS( "job" ) },
               { wxS( ODB_UNITS ), m_plugin->GetFormat().m_unitsStr },
               { wxS( "ODB_VERSION_MAJOR" ), wxS( "8" ) },
               { wxS( "ODB_VERSION_MINOR" ), wxS( "1" ) },
               { wxS( "ODB_SOURCE" ), wxS( "KiCad EDA" ) },
               { wxS( "CREATION_DATE" ), wxDateTime::Now().Format( "%Y%m%d.%H%M%S" ) },
               { wxS( "SAVE_DATE" ), wxDateTime::Now().Format( "%Y%m%d.%H%M%S" ) },
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
    m_matrixSteps.emplace( ODB::GenLegalEntityName( aStepName ), m_col++ );
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

    for( const FOOTPRINT* fp : m_board->Footprints() )
        ( fp->IsFlipped() ? m_hasBotComp : m_hasTopComp ) = true;

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
                    ly_name = wxString::Format( "DIELECTRIC_%d", stackup_item->GetDielectricLayerId() );
            }

            MATRIX_LAYER matrix( m_row++, ly_name );
            matrix.m_info.m_stackupItem = stackup_item;
            matrix.m_info.m_sublayer = sublayer_id;

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

                continue;
            }
            else
            {
                added_layers.insert( stackup_item->GetBrdLayerId() );
                AddMatrixLayerField( matrix, stackup_item->GetBrdLayerId() );
            }
        }
    }

    for( PCB_LAYER_ID layer : m_board->GetEnabledLayers().Seq() )
    {
        if( added_layers.find( layer ) != added_layers.end() )
            continue;

        MATRIX_LAYER matrix( m_row++, m_board->GetLayerName( layer ) );
        added_layers.insert( layer );
        AddMatrixLayerField( matrix, layer );
    }

    AddDrillMatrixLayer();

    AddAuxilliaryMatrixLayer();

    AddCOMPMatrixLayer( B_Cu );

    EnsureUniqueLayerNames();

    std::vector<ODB_LAYER_NAME>& names = m_plugin->GetLayerNameList();
    names.clear();
    names.reserve( m_matrixLayers.size() );

    for( const MATRIX_LAYER& layer : m_matrixLayers )
    {
        names.push_back( layer.m_info );
        names.back().m_name = layer.m_layerName;
    }
}


void ODB_MATRIX_ENTITY::AddMatrixLayerField( MATRIX_LAYER& aMLayer, PCB_LAYER_ID aLayer )
{
    aMLayer.m_info.m_layer = aLayer;
    aMLayer.m_polarity = ODB_POLARITY::POSITIVE;
    aMLayer.m_context = ODB_CONTEXT::BOARD;
    switch( aLayer )
    {
    case F_Paste:
    case B_Paste:
        aMLayer.m_type = ODB_TYPE::SOLDER_PASTE;
        break;

    case F_SilkS:
    case B_SilkS:
        aMLayer.m_type = ODB_TYPE::SILK_SCREEN;
        break;

    case F_Mask:
    case B_Mask:
        aMLayer.m_type = ODB_TYPE::SOLDER_MASK;
        break;

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
    case User_10:
    case User_11:
    case User_12:
    case User_13:
    case User_14:
    case User_15:
    case User_16:
    case User_17:
    case User_18:
    case User_19:
    case User_20:
    case User_21:
    case User_22:
    case User_23:
    case User_24:
    case User_25:
    case User_26:
    case User_27:
    case User_28:
    case User_29:
    case User_30:
    case User_31:
    case User_32:
    case User_33:
    case User_34:
    case User_35:
    case User_36:
    case User_37:
    case User_38:
    case User_39:
    case User_40:
    case User_41:
    case User_42:
    case User_43:
    case User_44:
    case User_45:
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
    }
}


void ODB_MATRIX_ENTITY::AddDrillMatrixLayer()
{
    std::map<ODB_DRILL_SPAN, std::vector<BOARD_ITEM*>>& drill_layers = m_plugin->GetDrillLayerItemsMap();

    std::map<std::pair<PCB_LAYER_ID, PCB_LAYER_ID>, std::vector<BOARD_ITEM*>>& slot_holes =
            m_plugin->GetSlotHolesMap();

    drill_layers.clear();
    slot_holes.clear();

    bool has_pth_layer = false;
    bool has_npth_layer = false;

    for( BOARD_ITEM* item : m_board->Tracks() )
    {
        if( item->Type() == PCB_VIA_T )
        {
            PCB_VIA* via = static_cast<PCB_VIA*>( item );
            has_pth_layer = true;

            // Start is the drilling side, so blind vias reaching only B.Cu start there
            bool           fromBack = via->BottomLayer() == B_Cu && via->TopLayer() != F_Cu;
            ODB_DRILL_SPAN platedSpan( fromBack ? via->BottomLayer() : via->TopLayer(),
                                       fromBack ? via->TopLayer() : via->BottomLayer(), false, false );
            drill_layers[platedSpan].push_back( via );

            std::set<ODB_DRILL_SPAN> addedBackdrillSpans;

            auto addBackdrillSpan = [&]( const PADSTACK::DRILL_PROPS& aDrill )
            {
                if( aDrill.start != UNDEFINED_LAYER && aDrill.end != UNDEFINED_LAYER
                        && ( aDrill.size.x > 0 || aDrill.size.y > 0 ) )
                {
                    ODB_DRILL_SPAN backSpan( aDrill.start, aDrill.end, true, true );

                    if( addedBackdrillSpans.insert( backSpan ).second )
                        drill_layers[backSpan].push_back( via );
                }
            };

            addBackdrillSpan( via->Padstack().SecondaryDrill() );
            addBackdrillSpan( via->Padstack().TertiaryDrill() );
        }
    }

    for( FOOTPRINT* fp : m_board->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            if( pad->GetAttribute() == PAD_ATTRIB::PTH )
                has_pth_layer = true;
            if( pad->GetAttribute() == PAD_ATTRIB::NPTH )
                has_npth_layer = true;

            // Shared with the drill writers and IPC-2581, which each used to decide this
            // separately and disagreed on a circular drill shape with unequal sizes
            if( IsDrillSlot( *pad ) )
                slot_holes[std::make_pair( F_Cu, B_Cu )].push_back( pad );
            else if( pad->HasHole() )
            {
                ODB_DRILL_SPAN padSpan( F_Cu, B_Cu, false, pad->GetAttribute() == PAD_ATTRIB::NPTH );
                drill_layers[padSpan].push_back( pad );
            }
        }
    }

    if( has_npth_layer )
    {
        ODB_DRILL_SPAN npthSpan( F_Cu, B_Cu, false, true );
        drill_layers[npthSpan];
    }

    if( has_pth_layer )
    {
        ODB_DRILL_SPAN platedSpan( F_Cu, B_Cu, false, false );
        drill_layers[platedSpan];
    }

    int backdrillIndex = 1;

    auto assignName = [&]( const ODB_DRILL_SPAN& aSpan )
    {
        wxString name;

        if( aSpan.m_IsBackdrill )
        {
            name.Printf( wxT( "drill%d" ), backdrillIndex++ );
        }
        else
        {
            wxString platedLabel = aSpan.m_IsNonPlated ? wxT( "non-plated" ) : wxT( "plated" );
            name.Printf( wxT( "drill_%s_%s-%s" ), platedLabel, m_board->GetLayerName( aSpan.TopLayer() ),
                         m_board->GetLayerName( aSpan.BottomLayer() ) );
        }

        return ODB::GenLegalEntityName( name );
    };

    auto InitDrillMatrix =
            [&]( const ODB_DRILL_SPAN& aSpan )
            {
                wxString dLayerName = assignName( aSpan );
                MATRIX_LAYER matrix( m_row++, dLayerName );

                matrix.m_type = ODB_TYPE::DRILL;
                matrix.m_info.m_role = aSpan.m_IsBackdrill ? ODB_LAYER_ROLE::BACKDRILL : ODB_LAYER_ROLE::DRILL;
                matrix.m_info.m_drillSpan = aSpan;
                matrix.m_context = ODB_CONTEXT::BOARD;
                matrix.m_polarity = ODB_POLARITY::POSITIVE;
                matrix.m_span.emplace(
                        std::make_pair( ODB::GenLegalEntityName( m_board->GetLayerName( aSpan.TopLayer() ) ),
                                        ODB::GenLegalEntityName( m_board->GetLayerName( aSpan.BottomLayer() ) ) ) );

                if( aSpan.m_IsBackdrill )
                    matrix.m_addType.emplace( ODB_SUBTYPE::BACKDRILL );

                m_matrixLayers.push_back( matrix );
            };

    for( const auto& entry : drill_layers )
        InitDrillMatrix( entry.first );
}


void ODB_MATRIX_ENTITY::AddCOMPMatrixLayer( PCB_LAYER_ID aCompSide )
{
    // A component layer must have a components file, so a side with no components has no layer
    if( aCompSide == F_Cu ? !m_hasTopComp : !m_hasBotComp )
        return;

    MATRIX_LAYER matrix( m_row++, aCompSide == F_Cu ? "COMP_+_TOP" : "COMP_+_BOT" );
    matrix.m_type = ODB_TYPE::COMPONENT;
    matrix.m_info.m_role = ODB_LAYER_ROLE::COMPONENT;
    matrix.m_context = ODB_CONTEXT::BOARD;
    m_matrixLayers.push_back( matrix );
}

void ODB_MATRIX_ENTITY::AddAuxilliaryMatrixLayer()
{
    auto& auxilliary_layers = m_plugin->GetAuxilliaryLayerItemsMap();

    for( BOARD_ITEM* item : m_board->Tracks() )
    {
        if( item->Type() == PCB_VIA_T )
        {
            PCB_VIA* via = static_cast<PCB_VIA*>( item );

            if( via->Padstack().IsFilled().value_or( false ) )
            {
                auxilliary_layers[std::make_tuple( ODB_AUX_LAYER_TYPE::FILLING, via->TopLayer(),
                                                   via->BottomLayer() )]
                        .push_back( via );
            }

            if( via->Padstack().IsCapped().value_or( false ) )
            {
                auxilliary_layers[std::make_tuple( ODB_AUX_LAYER_TYPE::CAPPING, via->TopLayer(),
                                                   via->BottomLayer() )]
                        .push_back( via );
            }

            for( PCB_LAYER_ID layer : { via->TopLayer(), via->BottomLayer() } )
            {
                if( via->Padstack().IsPlugged( layer ).value_or( false ) )
                {
                    auxilliary_layers[std::make_tuple( ODB_AUX_LAYER_TYPE::PLUGGING, layer,
                                                       PCB_LAYER_ID::UNDEFINED_LAYER )]
                            .push_back( via );
                }

                if( via->Padstack().IsCovered( layer ).value_or( false ) )
                {
                    auxilliary_layers[std::make_tuple( ODB_AUX_LAYER_TYPE::COVERING, layer,
                                                       PCB_LAYER_ID::UNDEFINED_LAYER )]
                            .push_back( via );
                }

                if( via->Padstack().IsTented( layer ).value_or( false ) )
                {
                    auxilliary_layers[std::make_tuple( ODB_AUX_LAYER_TYPE::TENTING, layer,
                                                       PCB_LAYER_ID::UNDEFINED_LAYER )]
                            .push_back( via );
                }
            }
        }
    }

    auto InitAuxMatrix = [&]( ODB_AUX_LAYER_KEY aLayerPair )
    {
        wxString featureName = "";
        switch( std::get<0>( aLayerPair ) )
        {
        case ODB_AUX_LAYER_TYPE::TENTING: featureName = "tenting"; break;
        case ODB_AUX_LAYER_TYPE::COVERING: featureName = "covering"; break;
        case ODB_AUX_LAYER_TYPE::PLUGGING: featureName = "plugging"; break;
        case ODB_AUX_LAYER_TYPE::FILLING: featureName = "filling"; break;
        case ODB_AUX_LAYER_TYPE::CAPPING: featureName = "capping"; break;
        default: return;
        }

        wxString dLayerName;

        if( std::get<2>( aLayerPair ) != PCB_LAYER_ID::UNDEFINED_LAYER )
        {
            dLayerName = wxString::Format( "%s_%s-%s", featureName,
                                           m_board->GetLayerName( std::get<1>( aLayerPair ) ),
                                           m_board->GetLayerName( std::get<2>( aLayerPair ) ) );
        }
        else
        {
            if( m_board->IsFrontLayer( std::get<1>( aLayerPair ) ) )
                dLayerName = wxString::Format( "%s_front", featureName );
            else if( m_board->IsBackLayer( std::get<1>( aLayerPair ) ) )
                dLayerName = wxString::Format( "%s_back", featureName );
            else
                return;
        }
        MATRIX_LAYER matrix( m_row++, dLayerName );

        matrix.m_type = ODB_TYPE::DOCUMENT;
        matrix.m_info.m_role = ODB_LAYER_ROLE::VIA_PROTECTION;
        matrix.m_info.m_auxType = std::get<0>( aLayerPair );
        matrix.m_info.m_auxKey = aLayerPair;
        matrix.m_info.m_layer = std::get<2>( aLayerPair ) == PCB_LAYER_ID::UNDEFINED_LAYER
                                        ? std::get<1>( aLayerPair )
                                        : PCB_LAYER_ID::UNDEFINED_LAYER;
        matrix.m_context = ODB_CONTEXT::BOARD;
        matrix.m_polarity = ODB_POLARITY::POSITIVE;

        if( std::get<2>( aLayerPair ) != PCB_LAYER_ID::UNDEFINED_LAYER )
        {
            matrix.m_span.emplace( std::make_pair(
                    ODB::GenLegalEntityName( m_board->GetLayerName( std::get<1>( aLayerPair ) ) ),
                    ODB::GenLegalEntityName(
                            m_board->GetLayerName( std::get<2>( aLayerPair ) ) ) ) );
        }

        m_matrixLayers.push_back( matrix );
    };

    for( const auto& [layer_pair, vec] : auxilliary_layers )
    {
        InitAuxMatrix( layer_pair );
    }
}


void ODB_MATRIX_ENTITY::EnsureUniqueLayerNames()
{
    // Track occurrences of each layer name to detect and handle duplicates
    std::map<wxString, std::vector<size_t>> name_to_indices;

    // First pass: collect all layer names and their indices
    for( size_t i = 0; i < m_matrixLayers.size(); ++i )
    {
        const wxString& layerName = m_matrixLayers[i].m_layerName;
        name_to_indices[layerName].push_back( i );
    }

    // Second pass: for any layer names that appear more than once, add suffixes
    for( auto& [layerName, indices] : name_to_indices )
    {
        if( indices.size() > 1 )
        {
            // Multiple layers have the same name, add suffixes to make them unique
            for( size_t count = 0; count < indices.size(); ++count )
            {
                size_t idx = indices[count];
                wxString newLayerName = wxString::Format( "%s_%zu", m_matrixLayers[idx].m_layerName, count + 1 );

                // Ensure the new name doesn't exceed the 64-character limit
                if( newLayerName.length() > 64 )
                {
                    // Truncate the base name if necessary to fit the suffix
                    wxString baseName = m_matrixLayers[idx].m_layerName;
                    size_t suffixLen = wxString::Format( "_%zu", count + 1 ).length();

                    if( suffixLen < baseName.length() )
                    {
                        baseName.Truncate( 64 - suffixLen );
                        newLayerName = wxString::Format( "%s_%zu", baseName, count + 1 );
                    }
                }

                m_matrixLayers[idx].m_layerName = std::move( newLayerName );
            }
        }
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

        twriter.WriteEquationLine( "NAME", layer.m_layerName );
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
            twriter.WriteEquationLine( "START_NAME", layer.m_span->first );
            twriter.WriteEquationLine( "END_NAME", layer.m_span->second );
        }

        twriter.WriteEquationLine( "COLOR", "0" );
    }
}


ODB_LAYER_ENTITY::ODB_LAYER_ENTITY( BOARD* aBoard, PCB_IO_ODBPP* aPlugin, std::map<int, std::vector<BOARD_ITEM*>>& aMap,
                                    const ODB_LAYER_NAME& aLayer ) :
        ODB_ENTITY_BASE( aBoard, aPlugin ),
        m_layerItems( aMap ),
        m_layer( aLayer )
{
    m_featuresMgr = std::make_unique<FEATURES_MANAGER>( aBoard, aPlugin, aLayer.m_name, aLayer.m_role, aLayer.m_auxType,
                                                        aLayer.m_drillSpan );
}


void ODB_LAYER_ENTITY::InitEntityData()
{
    if( m_layer.m_role == ODB_LAYER_ROLE::DRILL || m_layer.m_role == ODB_LAYER_ROLE::BACKDRILL )
    {
        InitDrillData();
        InitFeatureData();
        return;
    }

    if( m_layer.m_role == ODB_LAYER_ROLE::VIA_PROTECTION )
    {
        InitAuxilliaryData();
        InitFeatureData();
        return;
    }

    if( m_layer.m_layer != PCB_LAYER_ID::UNDEFINED_LAYER )
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

        if( vec.empty() )
            continue;

        std::stable_sort( vec.begin(), vec.end(), FabItemLess );

        m_featuresMgr->InitFeatureList( m_layer.m_layer, vec );
    }
}


ODB_COMPONENT& ODB_LAYER_ENTITY::InitComponentData( const FOOTPRINT*         aFp,
                                                    const EDA_DATA::PACKAGE& aPkg )
{
    if( m_layer.m_name == ODB::GenLegalEntityName( "COMP_+_BOT" ) )
    {
        if( !m_compBot.has_value() )
        {
            m_compBot.emplace( m_plugin );
        }
        return m_compBot.value().AddComponent( aFp, aPkg );
    }
    else
    {
        if( !m_compTop.has_value() )
        {
            m_compTop.emplace( m_plugin );
        }

        return m_compTop.value().AddComponent( aFp, aPkg );
    }
}


void ODB_LAYER_ENTITY::InitDrillData()
{
    std::map<ODB_DRILL_SPAN, std::vector<BOARD_ITEM*>>& drill_layers =
            m_plugin->GetDrillLayerItemsMap();

    std::map<std::pair<PCB_LAYER_ID, PCB_LAYER_ID>, std::vector<BOARD_ITEM*>>& slot_holes =
            m_plugin->GetSlotHolesMap();

    wxCHECK_RET( m_layer.m_drillSpan.has_value(), "Drill matrix row has no span" );
    const ODB_DRILL_SPAN& matchedSpan = *m_layer.m_drillSpan;

    if( !m_layerItems.empty() )
    {
        m_layerItems.clear();
    }

    m_tools.emplace( m_plugin->GetFormat().m_unitsStr );

    bool isBackdrillLayer = matchedSpan.m_IsBackdrill;
    bool isNonPlatedLayer = matchedSpan.m_IsNonPlated;
    bool isNPTHLayer = matchedSpan.m_IsNonPlated && !matchedSpan.m_IsBackdrill;

    if( !isBackdrillLayer )
    {
        // Slotted (oval) holes are routed to a separate map; emit them on the matching
        // plated or non-plated drill layer. Plated slots belong on the plated layer,
        // non-plated slots on the non-plated layer.
        auto slotIt = slot_holes.find( matchedSpan.Pair() );

        if( slotIt != slot_holes.end() )
        {
            for( BOARD_ITEM* item : slotIt->second )
            {
                if( item->Type() != PCB_PAD_T )
                    continue;

                PAD* pad = static_cast<PAD*>( item );

                bool padIsNPTH = pad->GetAttribute() == PAD_ATTRIB::NPTH;

                if( isNPTHLayer != padIsNPTH )
                    continue;

                m_tools.value().AddDrillTool( m_plugin->GetFormat(),
                                               padIsNPTH ? wxT( "NON_PLATED" ) : wxT( "PLATED" ),
                                               std::min( pad->GetDrillSizeX(), pad->GetDrillSizeY() ) );

                m_layerItems[pad->GetNetCode()].push_back( item );
            }
        }
    }

    auto drillIt = drill_layers.find( matchedSpan );

    if( drillIt != drill_layers.end() )
    {
        for( BOARD_ITEM* item : drillIt->second )
        {
            if( item->Type() == PCB_VIA_T )
            {
                PCB_VIA* via = static_cast<PCB_VIA*>( item );

                if( isBackdrillLayer )
                {
                    const PADSTACK::DRILL_PROPS* drill =
                            ODB::MatchBackdrill( *via, matchedSpan.m_StartLayer, matchedSpan.m_EndLayer );

                    if( !drill )
                        continue;

                    int diameter = ODB::BackdrillDiameter( *drill );

                    if( diameter <= 0 )
                        continue;

                    m_tools.value().AddDrillTool( m_plugin->GetFormat(), wxT( "NON_PLATED" ), diameter,
                                                   wxT( "BLIND" ) );
                }
                else if( isNonPlatedLayer )
                {
                    m_tools.value().AddDrillTool( m_plugin->GetFormat(), wxT( "NON_PLATED" ),
                                                   via->GetDrillValue() );
                }
                else
                {
                    m_tools.value().AddDrillTool( m_plugin->GetFormat(), wxT( "VIA" ), via->GetDrillValue() );
                }

                m_layerItems[via->GetNetCode()].push_back( item );
            }
            else if( item->Type() == PCB_PAD_T )
            {
                PAD* pad = static_cast<PAD*>( item );

                bool padIsNPTH = pad->GetAttribute() == PAD_ATTRIB::NPTH;

                if( isNPTHLayer && !padIsNPTH )
                    continue;

                if( !isNonPlatedLayer && padIsNPTH )
                    continue;

                int drillSize = pad->GetDrillSizeX();

                if( pad->GetDrillSizeX() != pad->GetDrillSizeY() )
                    drillSize = std::min( pad->GetDrillSizeX(), pad->GetDrillSizeY() );

                wxString typeLabel = ( padIsNPTH || isNonPlatedLayer ) ? wxT( "NON_PLATED" ) : wxT( "PLATED" );
                wxString type2 = isBackdrillLayer ? wxT( "BLIND" ) : wxT( "STANDARD" );

                m_tools.value().AddDrillTool( m_plugin->GetFormat(), typeLabel, drillSize, type2 );

                m_layerItems[pad->GetNetCode()].push_back( item );
            }
        }
    }
}

void ODB_LAYER_ENTITY::InitAuxilliaryData()
{
    auto& auxilliary_layers = m_plugin->GetAuxilliaryLayerItemsMap();

    wxCHECK_RET( m_layer.m_auxKey.has_value(), "Auxiliary matrix row has no key" );

    if( !m_layerItems.empty() )
    {
        m_layerItems.clear();
    }

    auto auxIt = auxilliary_layers.find( *m_layer.m_auxKey );

    if( auxIt == auxilliary_layers.end() )
        return;

    for( BOARD_ITEM* item : auxIt->second )
    {
        if( item->Type() == PCB_VIA_T )
        {
            PCB_VIA* via = static_cast<PCB_VIA*>( item );

            m_layerItems[via->GetNetCode()].push_back( item );
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

    std::ostream& ost = fileproxy.GetStream();

    if( m_layer.m_drillSpan
        && IsCopperLayerLowerThan( m_layer.m_drillSpan->m_StartLayer, m_layer.m_drillSpan->m_EndLayer ) )
    {
        ost << ".drill_layer_direction=bottom2top" << std::endl;
    }

    BOARD_DESIGN_SETTINGS& dsnSettings = m_board->GetDesignSettings();
    BOARD_STACKUP&         stackup = dsnSettings.GetStackupDescriptor();

    const BOARD_STACKUP_ITEM* stackupItem = nullptr;

    if( m_layer.m_layer != PCB_LAYER_ID::UNDEFINED_LAYER )
    {
        for( BOARD_STACKUP_ITEM* item : stackup.GetList() )
        {
            if( item->GetBrdLayerId() == m_layer.m_layer )
            {
                stackupItem = item;
                break;
            }
        }
    }
    else
    {
        stackupItem = m_layer.m_stackupItem;
    }

    if( stackupItem )
    {
        int sublayer = m_layer.m_sublayer;
        int thickness = stackupItem->GetThickness( sublayer );

        if( thickness > 0 )
        {
            ost << ".layer_dielectric=" << ODB::Data2String( m_plugin->GetFormat(), thickness ) << std::endl;
        }

        if( stackupItem->GetType() == BS_ITEM_TYPE_COPPER )
        {
            double copperThicknessMM = static_cast<double>( thickness ) / pcbIUScale.mmToIU( 1.0 );
            double thicknessOz = copperThicknessMM / 0.035;
            ost << ".copper_weight=" << ODB::Double2String( m_plugin->GetFormat(), thicknessOz ) << std::endl;
        }

        if( stackupItem->HasEpsilonRValue() )
        {
            double epsilonR = stackupItem->GetEpsilonR( sublayer );

            if( epsilonR > 0.0 )
            {
                ost << ".dielectric_constant=" << ODB::Double2String( m_plugin->GetFormat(), epsilonR ) << std::endl;
            }
        }

        if( stackupItem->HasLossTangentValue() )
        {
            double lossTangent = stackupItem->GetLossTangent( sublayer );

            if( lossTangent > 0.0 )
            {
                ost << ".loss_tangent=" << ODB::Double2String( m_plugin->GetFormat(), lossTangent ) << std::endl;
            }
        }

        wxString material = stackupItem->GetMaterial( sublayer );

        if( !material.IsEmpty() )
        {
            ost << ".material=" << ODB::GenLegalEntityName( material ).ToStdString() << std::endl;
        }
    }
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
        m_edaData.AddPackage( fp, m_plugin->GetFormat() );
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
            wxLogTrace( traceOdbppIo, wxT( "Failed to add component data" ) );
            return;
        }

        // ODBPP only need unique PACKAGE in PKG record in eda/data file.
        // the PKG index can repeat to be ref in CMP record in component file.
        std::shared_ptr<FOOTPRINT> fp_pkg = m_edaData.GetEdaFootprints().at( j );
        ++j;

        const EDA_DATA::PACKAGE& eda_pkg =
                m_edaData.GetPackage( hash_fp_item( fp_pkg.get(), HASH_POS | REL_COORD ) );

        if( fp->Pads().empty() )
            continue;

        ODB_COMPONENT& comp = iter->second->InitComponentData( fp, eda_pkg );

        for( int i = 0; i < (int) fp->Pads().size(); ++i )
        {
            PAD*           pad = fp->Pads()[i];
            EDA_DATA::NET& eda_net = m_edaData.GetNet( pad->GetNetCode() );

            EDA_DATA::SUB_NET_TOEPRINT& subnet = eda_net.AddSubnet<EDA_DATA::SUB_NET_TOEPRINT>(
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

            toep.m_center = ODB::AddXY( m_plugin->GetFormat(), pad->GetPosition() );

            toep.m_rot = ODB::Double2String( m_plugin->GetFormat(),
                                             ( ANGLE_360 - pad->GetOrientation() ).Normalize().AsDegrees() );

            if( pad->IsFlipped() )
                toep.m_mirror = wxT( "M" );
            else
                toep.m_mirror = wxT( "N" );
        }
    }

    for( PCB_TRACK* track : m_board->Tracks() )
    {
        EDA_DATA::NET&     eda_net = m_edaData.GetNet( track->GetNetCode() );
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
            EDA_DATA::NET&           eda_net = m_edaData.GetNet( zone->GetNetCode() );
            EDA_DATA::SUB_NET_PLANE& subnet = eda_net.AddSubnet<EDA_DATA::SUB_NET_PLANE>(
                    &m_edaData,
                    EDA_DATA::SUB_NET_PLANE::FILL_TYPE::SOLID,
                    EDA_DATA::SUB_NET_PLANE::CUTOUT_TYPE::EXACT,
                    0 );
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

    m_profile = std::make_unique<FEATURES_MANAGER>( m_board, m_plugin, wxEmptyString, ODB_LAYER_ROLE::BOARD_LAYER,
                                                    std::nullopt );

    SHAPE_POLY_SET board_outline;

    if( !m_board->GetBoardPolygonOutlines( board_outline, true ) )
        wxLogTrace( traceOdbppIo, "Failed to get board outline" );

    int    count = board_outline.OutlineCount();
    int    largest = 0;
    double largestArea = count > 0 ? board_outline.Outline( 0 ).Area() : 0.0;

    for( int ii = 1; ii < count; ++ii )
    {
        double area = board_outline.Outline( ii ).Area();

        if( area > largestArea )
        {
            largest = ii;
            largestArea = area;
        }
    }

    if( count > 1 )
    {
        m_plugin->Report( wxString::Format( _( "ODB++ profile uses the largest of %d board outline islands; "
                                               "other islands are omitted." ),
                                            count ),
                          RPT_SEVERITY_WARNING );
    }

    if( !m_profile->AddContour( board_outline, largest ) )
        wxLogTrace( traceOdbppIo, "Failed to add polygon to profile" );

    m_profile->GenerateProfileFeatures( fileproxy.GetStream() );
}


void ODB_STEP_ENTITY::GenerateStepHeaderFile( ODB_TREE_WRITER& writer )
{
    auto fileproxy = writer.CreateFileProxy( "stephdr" );

    m_stephdr = {
        { ODB_UNITS, m_plugin->GetFormat().m_unitsStr },
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

    m_edaData.Write( fileproxy.GetStream(), m_plugin->GetFormat() );
}


void ODB_STEP_ENTITY::GenerateNetlistsFiles( ODB_TREE_WRITER& writer )
{
    auto fileproxy = writer.CreateFileProxy( "netlist" );

    m_netlist.Write( fileproxy.GetStream(), m_plugin->GetFormat() );
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
    LSET layers = m_board->GetEnabledLayers();

    // To avoid the overhead of repeatedly cycling through the layers and nets,
    // we pre-sort the board items into a map of layer -> net -> items
    std::map<PCB_LAYER_ID, std::map<int, std::vector<BOARD_ITEM*>>>& elements = m_plugin->GetLayerElementsMap();

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
                       for( PCB_LAYER_ID layer : zone->GetLayerSet() )
                           elements[layer][zone->GetNetCode()].push_back( zone );
                   } );

    for( BOARD_ITEM* item : m_board->Drawings() )
    {
        if( BOARD_CONNECTED_ITEM* conn_it = dynamic_cast<BOARD_CONNECTED_ITEM*>( item ) )
        {
            for( PCB_LAYER_ID layer : conn_it->GetLayerSet() )
                elements[layer][conn_it->GetNetCode()].push_back( conn_it );
        }
        else
        {
            for( PCB_LAYER_ID layer : item->GetLayerSet() )
                elements[layer][0].push_back( item );
        }
    }

    for( FOOTPRINT* fp : m_board->Footprints() )
    {
        for( PCB_FIELD* field : fp->GetFields() )
            elements[field->GetLayer()][0].push_back( field );

        for( BOARD_ITEM* item : fp->GraphicalItems() )
        {
            for( PCB_LAYER_ID layer : item->GetLayerSet() )
                elements[layer][0].push_back( item );
        }

        for( PAD* pad : fp->Pads() )
        {
            VECTOR2I margin;

            for( PCB_LAYER_ID layer : pad->GetLayerSet() )
            {
                PCB_LAYER_ID padLayer = pad->Padstack().EffectiveLayerFor( layer );

                bool onCopperLayer = LSET::AllCuMask().test( layer );
                bool onSolderMaskLayer = LSET( { F_Mask, B_Mask } ).test( layer );
                bool onSolderPasteLayer = LSET( { F_Paste, B_Paste } ).test( layer );

                if( onSolderMaskLayer )
                    margin.x = margin.y = pad->GetSolderMaskExpansion( padLayer );

                if( onSolderPasteLayer )
                    margin = pad->GetSolderPasteMargin( padLayer );

                VECTOR2I padPlotsSize = pad->GetSize( padLayer ) + margin * 2;

                if( onCopperLayer && !pad->IsOnCopperLayer() )
                    continue;

                if( onCopperLayer && !pad->FlashLayer( layer ) )
                    continue;

                if( pad->GetShape( padLayer ) != PAD_SHAPE::CUSTOM
                    && ( padPlotsSize.x <= 0 || padPlotsSize.y <= 0 ) )
                {
                    continue;
                }

                elements[layer][pad->GetNetCode()].push_back( pad );
            }
        }
    }

    for( const ODB_LAYER_NAME& layer : m_plugin->GetLayerNameList() )
    {
        std::shared_ptr<ODB_LAYER_ENTITY> layer_entity_ptr =
                std::make_shared<ODB_LAYER_ENTITY>( m_board, m_plugin, elements[layer.m_layer], layer );

        m_layerEntityMap.emplace( layer.m_name, layer_entity_ptr );
    }
}
