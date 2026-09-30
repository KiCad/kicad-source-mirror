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


#include <algorithm>
#include <array>
#include <map>
#include <tuple>
#include <unordered_set>
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
#include <exporters/fab_model/fab_layer_index.h>
#include <exporters/fab_model/fab_drill.h>
#include <exporters/fab_model/fab_drill_model.h>
#include <exporters/fab_model/fab_stackup.h>
#include <exporters/fab_model/fab_pin.h>
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
#include <zone.h>
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
#include <wx/xml/xml.h>

#include "odb_attribute.h"
#include "odb_entity.h"
#include "odb_defines.h"
#include "odb_feature.h"
#include "odb_util.h"
#include "pcb_io_odbpp.h"
#include <trace_helpers.h>


namespace
{
enum class METADATA_SECTION
{
    HEADER,
    REQUIREMENTS,
    MANUFACTURING,
    ASSEMBLY
};


struct METADATA_FIELD
{
    const char*      m_name;
    const char*      m_display;
    METADATA_SECTION m_section;
    bool             m_distance = false;
};


constexpr std::array<METADATA_FIELD, 15> METADATA_FIELDS = { {
        { "counter_bore", "Counter Bore", METADATA_SECTION::HEADER },
        { "countersink", "Counter Sink", METADATA_SECTION::HEADER },
        { "edge_connectors", "Edge Connectors", METADATA_SECTION::HEADER },
        { "layer_count", "Layer Count", METADATA_SECTION::HEADER },
        { "board_thickness", "Board Thickness", METADATA_SECTION::REQUIREMENTS, true },
        { "bottom_legend_color", "Bottom Legend Color", METADATA_SECTION::REQUIREMENTS },
        { "bottom_soldermask_color", "Bottom Soldermask Color", METADATA_SECTION::REQUIREMENTS },
        { "legend_sides", "Legend Sides", METADATA_SECTION::REQUIREMENTS },
        { "plated_edge", "Plated Edge", METADATA_SECTION::REQUIREMENTS },
        { "plated_slots", "Plated Slots", METADATA_SECTION::REQUIREMENTS },
        { "soldermask_sides", "Soldermask Sides", METADATA_SECTION::REQUIREMENTS },
        { "top_legend_color", "Top Legend Color", METADATA_SECTION::REQUIREMENTS },
        { "top_soldermask_color", "Top Soldermask Color", METADATA_SECTION::REQUIREMENTS },
        { "surface_finish_pads", "Surface Finish Pads", METADATA_SECTION::MANUFACTURING },
        { "pressfit_technology", "Pressfit Technology", METADATA_SECTION::ASSEMBLY },
} };


const BOARD_STACKUP_ITEM* StackupItemForLayer( const FAB_STACKUP& aStackup, const ODB_LAYER_NAME& aLayer )
{
    return aLayer.m_layer == UNDEFINED_LAYER ? aLayer.m_stackupItem : aStackup.ItemForLayer( aLayer.m_layer );
}


void WriteXmlFile( ODB_TREE_WRITER& aWriter, const char* aName, wxXmlDocument& aDocument, const wxString& aError )
{
    wxMemoryOutputStream xml;

    if( !aDocument.Save( xml ) )
        THROW_IO_ERROR( aError );

    std::string output( xml.GetSize(), '\0' );
    xml.CopyTo( output.data(), output.size() );
    auto file = aWriter.CreateFileProxy( aName );
    file.GetStream().write( output.data(), output.size() );
}


double CopperWeightOz( int aThickness )
{
    double thicknessMm = static_cast<double>( aThickness ) / pcbIUScale.mmToIU( 1.0 );
    return thicknessMm / 0.035;
}
} // namespace


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

    if( !m_plugin->GetFormat().m_productModelName.IsEmpty() )
        m_info.emplace_back( wxS( "PRODUCT_MODEL_NAME" ), m_plugin->GetFormat().m_productModelName );
}


void ODB_MISC_ENTITY::GenerateFiles( ODB_TREE_WRITER& writer )
{
    auto fileproxy = writer.CreateFileProxy( "info" );

    ODB_TEXT_WRITER twriter( fileproxy.GetStream() );

    for( auto& info : m_info )
    {
        twriter.WriteEquationLine( info.first, info.second );
    }

    twriter.WriteEquationLine( "MAX_UID", static_cast<int>( m_plugin->MaxUid() ) );

    GenerateUserAttrFile( writer );

    if( m_plugin->GetFormat().m_boardMetadata )
        GenerateMetadataFile( writer );

    if( !m_board->GetVariantNames().empty() )
        GenerateAttrListFile( writer );
}


void ODB_MISC_ENTITY::GenerateAttrListFile( ODB_TREE_WRITER& writer )
{
    auto attrlist = writer.CreateFileProxy( "attrlist" );
    attrlist.GetStream() << "UNITS=" << m_plugin->GetFormat().m_unitsStr << '\n';

    if( m_plugin->GetFormat().m_variantNames.m_listsFit )
        attrlist.GetStream() << ".variant_list="
                             << m_plugin->GetFormat().m_variantNames.Join( m_board->GetVariantNames() ).ToStdString()
                             << '\n';
}


void ODB_MISC_ENTITY::GenerateUserAttrFile( ODB_TREE_WRITER& writer )
{
    auto userAttrs = writer.CreateFileProxy( "userattr" );
    std::ostream& attrs = userAttrs.GetStream();
    attrs << "UNITS=" << m_plugin->GetFormat().m_unitsStr << '\n';
    attrs << "TEXT {\nNAME=material\nENTITY=LAYER\nMIN_LEN=0\nMAX_LEN=64\nOPTIONS=\nGROUP=Product\n}\n";

    for( const char* side : { "top", "bottom" } )
    {
        attrs << "OPTION {\nNAME=post_machining_" << side
              << "\nENTITY=FEATURE\nOPTIONS=countersink;counterbore\nGROUP=Fabrication\n}\n";

        for( const char* dimension : { "diameter", "depth" } )
        {
            attrs << "FLOAT {\nNAME=post_machining_" << side << '_' << dimension
                  << "\nENTITY=FEATURE\nMIN_VAL=0.0\nMAX_VAL=100000.0"
                  << "\nUNITS=MIL_MICRON\nGROUP=Fabrication\n}\n";
        }

        attrs << "FLOAT {\nNAME=post_machining_" << side
              << "_angle\nENTITY=FEATURE\nMIN_VAL=0.0\nMAX_VAL=180.0\nGROUP=Fabrication\n}\n";
    }
}


void ODB_MISC_ENTITY::GenerateMetadataFile( ODB_TREE_WRITER& writer )
{
    const ODB_FORMAT&               format = m_plugin->GetFormat();
    const BOARD_STACKUP&            stackup = m_plugin->GetFabStackup().Stackup();
    const FAB_DRILL_MODEL&          drillModel = m_plugin->GetFabDrillModel();
    std::map<std::string, wxString> values;

    auto yesNo = []( bool aValue )
    {
        return aValue ? wxS( "yes" ) : wxS( "no" );
    };
    auto sides = []( bool aTop, bool aBottom, const wxString& aNone ) -> wxString
    {
        if( aTop )
            return aBottom ? wxS( "Both" ) : wxS( "Top" );

        if( aBottom )
            return wxS( "Bottom" );

        return aNone;
    };

    const FAB_DRILL_FACTS& drillFacts = drillModel.Facts();

    values["counter_bore"] = yesNo( drillFacts.m_counterbore );
    values["countersink"] = yesNo( drillFacts.m_countersink );
    values["edge_connectors"] = yesNo( stackup.m_EdgeConnectorConstraints != BS_EDGE_CONNECTOR_NONE );
    values["layer_count"] = wxString::Format( "%d", m_board->GetCopperLayerCount() );
    values["board_thickness"] = ODB::Data2String( format, m_plugin->GetFabStackup().Thickness() );
    values["plated_edge"] = yesNo( stackup.m_EdgePlating );
    values["plated_slots"] = yesNo( drillFacts.m_platedSlots );
    values["pressfit_technology"] = yesNo( drillFacts.m_pressfit );
    values["soldermask_sides"] =
            sides( m_board->IsLayerEnabled( F_Mask ), m_board->IsLayerEnabled( B_Mask ), wxS( "None" ) );

    values["legend_sides"] = sides( FabLayerHasVisibleItems( *m_board, F_SilkS ),
                                    FabLayerHasVisibleItems( *m_board, B_SilkS ), wxS( "none" ) );

    auto addColor = [this, &values]( const char* aName, PCB_LAYER_ID aLayer )
    {
        wxString color = m_plugin->GetFabStackup().NamedColor( aLayer );

        if( !color.IsEmpty() )
            values[aName] = color;
    };

    addColor( "top_soldermask_color", F_Mask );
    addColor( "bottom_soldermask_color", B_Mask );
    addColor( "top_legend_color", F_SilkS );
    addColor( "bottom_legend_color", B_SilkS );

    wxString finish = stackup.m_FinishType;

    if( IsPrmSpecified( finish ) && finish != wxS( "User defined" ) )
    {
        static const std::map<wxString, wxString> finishNames = {
            { wxS( "HAL SnPb" ), wxS( "HASL" ) },
            { wxS( "HAL lead-free" ), wxS( "Lead-Free HASL" ) },
            { wxS( "Immersion silver" ), wxS( "Immersion Silver" ) },
            { wxS( "Immersion tin" ), wxS( "White Tin" ) },
            { wxS( "OSP" ), wxS( "OSP (Entek)" ) },
            { wxS( "None" ), wxS( "Bare Cu" ) },
        };

        if( auto it = finishNames.find( finish ); it != finishNames.end() )
            finish = it->second;

        if( std::all_of( finish.begin(), finish.end(),
                         []( wxUniChar aChar )
                         {
                             return aChar >= 32 && aChar <= 126;
                         } ) )
            values["surface_finish_pads"] = finish;
    }

    wxXmlDocument document;
    wxXmlNode*    root = new wxXmlNode( wxXML_ELEMENT_NODE, wxS( "metadata" ) );
    document.SetRoot( root );

    auto addElement = []( wxXmlNode* aParent, const wxString& aName, const wxString& aDescription )
    {
        wxXmlNode* node = new wxXmlNode( wxXML_ELEMENT_NODE, aName );

        if( !aDescription.IsEmpty() )
            node->AddAttribute( wxS( "description" ), aDescription );

        aParent->AddChild( node );
        return node;
    };

    constexpr std::array<std::pair<const char*, const char*>, 4> sectionInfo = { {
            { "Header", "General Information" },
            { "Requirements", "Board Requirements" },
            { "Manufacturing", "Manufacturing Process" },
            { "Assembly", "Assembly" },
    } };
    std::array<wxXmlNode*, sectionInfo.size()>                   sections{};
    wxXmlNode*                                                   step = nullptr;

    auto sectionNode = [&]( METADATA_SECTION aSection )
    {
        size_t index = static_cast<size_t>( aSection );

        if( !sections[index] )
        {
            wxXmlNode* parent = root;

            if( index >= static_cast<size_t>( METADATA_SECTION::MANUFACTURING ) )
            {
                if( !step )
                {
                    wxXmlNode* steps = addElement( root, wxS( "Steps" ), wxS( "Steps" ) );
                    step = addElement( steps, wxS( "Step" ), wxString() );
                    step->AddAttribute( wxS( "name" ), wxString::FromUTF8( ODB_STEP_ENTITY::STEP_NAME ) );
                }

                parent = step;
            }

            sections[index] = addElement( parent, wxString::FromUTF8( sectionInfo[index].first ),
                                          wxString::FromUTF8( sectionInfo[index].second ) );
        }

        return sections[index];
    };

    for( const METADATA_FIELD& field : METADATA_FIELDS )
    {
        auto value = values.find( field.m_name );

        if( value == values.end() )
            continue;

        wxXmlNode* node = addElement( sectionNode( field.m_section ), wxS( "node" ), wxString() );
        node->AddAttribute( wxS( "name" ), wxString::FromUTF8( field.m_name ) );
        node->AddAttribute( wxS( "display" ), wxString::FromUTF8( field.m_display ) );
        node->AddAttribute( wxS( "value" ), value->second );

        if( field.m_distance )
            node->AddAttribute( wxS( "units" ), wxString::FromUTF8( format.m_unitsStr ) );
    }

    WriteXmlFile( writer, "metadata.xml", document, _( "Failed to generate ODB++ metadata." ) );
}


void ODB_MATRIX_ENTITY::AddStep( const wxString& aStepName )
{
    m_matrixSteps.emplace( ODB::GenLegalEntityName( aStepName ),
                           std::make_pair( m_col++, m_plugin->NewUid() ) );
}


void ODB_MATRIX_ENTITY::InitEntityData()
{
    AddStep( wxString::FromUTF8( ODB_STEP_ENTITY::STEP_NAME ) );

    InitMatrixLayerData();
}


void ODB_MATRIX_ENTITY::InitMatrixLayerData()
{
    const BOARD_STACKUP& stackup = m_plugin->GetFabStackup().Stackup();

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
                matrix.m_dielectricName = stackup_item->GetMaterial( sublayer_id );

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

    AddDrillMatrixLayer();

    AddRoutMatrixLayer();

    for( PCB_LAYER_ID layer : m_board->GetEnabledLayers().Seq() )
    {
        if( added_layers.find( layer ) != added_layers.end() )
            continue;

        MATRIX_LAYER matrix( m_row++, m_board->GetLayerName( layer ) );
        added_layers.insert( layer );
        AddMatrixLayerField( matrix, layer );
    }

    AddAuxilliaryMatrixLayer();

    AddCOMPMatrixLayer( B_Cu );

    const ODB_FORMAT& format = m_plugin->GetFormat();
    bool              filtered = format.m_sections.has_value() || !format.m_layerOverrides.empty();
    auto isCopper = []( const MATRIX_LAYER& aLayer )
    {
        return aLayer.m_type == ODB_TYPE::SIGNAL || aLayer.m_type == ODB_TYPE::POWER_GROUND
               || aLayer.m_type == ODB_TYPE::MIXED;
    };
    std::map<const BOARD_STACKUP_ITEM*, std::pair<PCB_LAYER_ID, PCB_LAYER_ID>> adjacentCopper;

    if( filtered )
    {
        for( size_t index = 0; index < m_matrixLayers.size(); ++index )
        {
            const MATRIX_LAYER& layer = m_matrixLayers[index];

            if( layer.m_diType != ODB_DIELECTRIC_TYPE::CORE )
                continue;

            auto& [top, bottom] = adjacentCopper[layer.m_info.m_stackupItem];
            top = UNDEFINED_LAYER;
            bottom = UNDEFINED_LAYER;

            for( size_t probe = index; probe-- > 0; )
            {
                const MATRIX_LAYER& neighbor = m_matrixLayers[probe];

                if( neighbor.m_info.m_stackupItem == layer.m_info.m_stackupItem )
                    continue;

                if( isCopper( neighbor ) )
                    top = neighbor.m_info.m_layer;

                break;
            }

            for( size_t probe = index + 1; probe < m_matrixLayers.size(); ++probe )
            {
                const MATRIX_LAYER& neighbor = m_matrixLayers[probe];

                if( neighbor.m_info.m_stackupItem == layer.m_info.m_stackupItem )
                    continue;

                if( isCopper( neighbor ) )
                    bottom = neighbor.m_info.m_layer;

                break;
            }
        }
    }

    if( filtered )
    {
        auto sectionFor = []( const MATRIX_LAYER& aLayer )
        {
            using SECTION = FAB::SECTION;

            if( aLayer.m_info.m_role == ODB_LAYER_ROLE::COMPONENT )
                return SECTION::COMPONENTS;

            if( aLayer.m_info.m_role == ODB_LAYER_ROLE::DRILL || aLayer.m_info.m_role == ODB_LAYER_ROLE::BACKDRILL
                || aLayer.m_info.m_role == ODB_LAYER_ROLE::ROUT )
            {
                return SECTION::DRILL_ROUT;
            }

            if( aLayer.m_info.m_role == ODB_LAYER_ROLE::VIA_PROTECTION )
                return SECTION::MISC_FAB;

            switch( aLayer.m_type )
            {
            case ODB_TYPE::SIGNAL:
            case ODB_TYPE::POWER_GROUND:
            case ODB_TYPE::MIXED:
                return IsExternalCopperLayer( aLayer.m_info.m_layer ) ? SECTION::OUTER_COPPER : SECTION::INNER_COPPER;
            case ODB_TYPE::DIELECTRIC: return SECTION::DIELECTRIC;
            case ODB_TYPE::SOLDER_MASK: return SECTION::SOLDERMASK;
            case ODB_TYPE::SOLDER_PASTE: return SECTION::SOLDERPASTE;
            case ODB_TYPE::SILK_SCREEN: return SECTION::SILKSCREEN;
            default: return SECTION::DOCUMENTATION;
            }
        };

        std::set<PCB_LAYER_ID> excluded;

        for( const ODB_LAYER_OVERRIDE& override : format.m_layerOverrides )
        {
            if( !override.m_include )
                excluded.insert( override.m_layer );
        }

        std::erase_if( m_matrixLayers,
                       [&]( const MATRIX_LAYER& aLayer )
                       {
                           return !format.Includes( sectionFor( aLayer ) )
                                  || excluded.contains( aLayer.m_info.m_layer );
                       } );

        // Overrides are checked against the final automatic names
        EnsureUniqueLayerNames();

        std::set<wxString> usedNames;

        for( const MATRIX_LAYER& layer : m_matrixLayers )
            usedNames.insert( layer.m_layerName );

        for( MATRIX_LAYER& layer : m_matrixLayers )
        {
            for( const ODB_LAYER_OVERRIDE& override : format.m_layerOverrides )
            {
                if( layer.m_info.m_role != ODB_LAYER_ROLE::BOARD_LAYER || layer.m_info.m_layer != override.m_layer )
                {
                    continue;
                }

                if( !override.m_odbName.IsEmpty() )
                {
                    wxString name = ODB::GenLegalEntityName( override.m_odbName );

                    if( name.IsEmpty() || ( name != layer.m_layerName && usedNames.contains( name ) ) )
                    {
                        m_plugin->Report( _( "ODB++ layer name override is empty or collides; using automatic name." ),
                                          RPT_SEVERITY_WARNING );
                    }
                    else
                    {
                        usedNames.erase( layer.m_layerName );
                        layer.m_layerName = name;
                        usedNames.insert( name );
                    }
                }

                if( !override.m_odbType.IsEmpty() )
                {
                    std::string           type = override.m_odbType.ToStdString();
                    std::vector<ODB_TYPE> allowed = ODB::OverridableLayerTypes( layer.m_info.m_layer );
                    auto                  it = std::find_if( allowed.begin(), allowed.end(),
                                                             [&]( ODB_TYPE aType )
                                                             {
                                                                 return ODB::Enum2String( aType ) == type;
                                                             } );

                    if( it != allowed.end() )
                        layer.m_type = *it;
                    else if( ODB::Enum2String( layer.m_type ) != type )
                        m_plugin->Report( _( "Invalid ODB++ layer type override; using automatic type." ),
                                          RPT_SEVERITY_WARNING );
                }
            }
        }
    }

    EnsureUniqueLayerNames();

    std::map<PCB_LAYER_ID, wxString> namesByLayer;

    for( const MATRIX_LAYER& layer : m_matrixLayers )
    {
        if( layer.m_info.m_role == ODB_LAYER_ROLE::BOARD_LAYER && layer.m_info.m_layer != UNDEFINED_LAYER )
        {
            namesByLayer[layer.m_info.m_layer] = layer.m_layerName;
        }
    }

    std::erase_if( m_matrixLayers,
                   [&]( MATRIX_LAYER& aLayer )
                   {
                       if( !aLayer.m_span )
                           return false;

                       PCB_LAYER_ID start = F_Cu;
                       PCB_LAYER_ID end = B_Cu;

                       if( aLayer.m_info.m_drillSpan )
                       {
                           start = aLayer.m_info.m_drillSpan->TopLayer();
                           end = aLayer.m_info.m_drillSpan->BottomLayer();
                       }
                       else if( aLayer.m_info.m_auxKey )
                       {
                           start = std::get<1>( *aLayer.m_info.m_auxKey );
                           end = std::get<2>( *aLayer.m_info.m_auxKey );
                       }

                       if( namesByLayer.contains( start ) && namesByLayer.contains( end ) )
                       {
                           aLayer.m_span = { namesByLayer.at( start ), namesByLayer.at( end ) };
                           return false;
                       }

                       wxCHECK_MSG( filtered, false, wxS( "ODB++ span layer has no matrix row" ) );

                       bool through =
                               aLayer.m_info.m_role == ODB_LAYER_ROLE::ROUT
                               || ( aLayer.m_info.m_role == ODB_LAYER_ROLE::DRILL && start == F_Cu && end == B_Cu );

                       if( through )
                       {
                           aLayer.m_span = { wxEmptyString, wxEmptyString };
                           return false;
                       }

                       m_plugin->Report( _( "ODB++ partial drill layer omitted because a span layer is excluded." ),
                                         RPT_SEVERITY_WARNING );
                       return true;
                   } );

    if( filtered )
    {
        for( size_t index = 0; index < m_matrixLayers.size(); ++index )
            m_matrixLayers[index].m_rowNumber = static_cast<uint32_t>( index + 1 );
    }

    std::map<PCB_LAYER_ID, uint32_t> copperIds;

    for( MATRIX_LAYER& layer : m_matrixLayers )
    {
        layer.m_uid = m_plugin->NewUid();

        if( isCopper( layer ) )
        {
            copperIds[layer.m_info.m_layer] = layer.m_uid;
        }
    }

    for( size_t i = 0; i < m_matrixLayers.size(); ++i )
    {
        MATRIX_LAYER& layer = m_matrixLayers[i];
        PCB_LAYER_ID brdLayer = layer.m_info.m_layer;

        // A misc layer retyped as mask, paste or silk has no physical side to reference
        if( layer.m_context == ODB_CONTEXT::BOARD
            && ( layer.m_type == ODB_TYPE::SOLDER_MASK || layer.m_type == ODB_TYPE::SOLDER_PASTE
                 || layer.m_type == ODB_TYPE::SILK_SCREEN ) )
        {
            PCB_LAYER_ID copper = m_board->IsFrontLayer( brdLayer ) ? F_Cu : B_Cu;

            if( auto it = copperIds.find( copper ); it != copperIds.end() )
                layer.m_ref = it->second;
        }

        if( layer.m_diType != ODB_DIELECTRIC_TYPE::CORE )
            continue;

        if( filtered )
        {
            auto [top, bottom] = adjacentCopper.at( layer.m_info.m_stackupItem );

            if( auto it = copperIds.find( top ); it != copperIds.end() )
                layer.m_cuTop = it->second;

            if( auto it = copperIds.find( bottom ); it != copperIds.end() )
                layer.m_cuBottom = it->second;

            continue;
        }

        for( size_t top = i; top-- > 0; )
        {
            if( isCopper( m_matrixLayers[top] ) )
            {
                layer.m_cuTop = m_matrixLayers[top].m_uid;
                break;
            }
        }

        for( size_t bottom = i + 1; bottom < m_matrixLayers.size(); ++bottom )
        {
            if( isCopper( m_matrixLayers[bottom] ) )
            {
                layer.m_cuBottom = m_matrixLayers[bottom].m_uid;
                break;
            }
        }
    }

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
            switch( m_board->GetLayerType( aLayer ) )
            {
            case LT_POWER: aMLayer.m_type = ODB_TYPE::POWER_GROUND; break;
            case LT_MIXED: aMLayer.m_type = ODB_TYPE::MIXED; break;
            default:       aMLayer.m_type = ODB_TYPE::SIGNAL; break;
            }
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
    std::map<DRILL_SPAN, std::vector<BOARD_ITEM*>>& drill_layers = m_plugin->GetDrillLayerItemsMap();

    std::map<std::pair<PCB_LAYER_ID, PCB_LAYER_ID>, std::vector<BOARD_ITEM*>>& slot_holes =
            m_plugin->GetSlotHolesMap();

    drill_layers.clear();
    slot_holes.clear();

    bool has_pth_layer = false;
    bool has_npth_layer = false;

    for( FOOTPRINT* fp : m_board->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            has_pth_layer |= pad->GetAttribute() == PAD_ATTRIB::PTH;
            has_npth_layer |= pad->GetAttribute() == PAD_ATTRIB::NPTH;
        }
    }

    const FAB_DRILL_MODEL& model = m_plugin->GetFabDrillModel();
    std::map<DRILL_SPAN, std::unordered_set<BOARD_ITEM*>> seen;

    for( const FAB_DRILL_LAYER& layer : model.Layers() )
    {
        auto addOperation = [&]( const DRILL_OPERATION& aOperation, bool aSlot )
        {
            BOARD_ITEM* item = aOperation.m_SourceItem;
            // Start is the drilling side, so blind vias reaching only B.Cu start there
            bool        fromBack = !layer.m_Span.m_IsBackdrill && item->Type() == PCB_VIA_T
                                   && aOperation.m_BottomLayer == B_Cu && aOperation.m_TopLayer != F_Cu;
            DRILL_SPAN span( fromBack ? layer.m_Span.BottomLayer() : layer.m_Span.DrillStartLayer(),
                             fromBack ? layer.m_Span.TopLayer() : layer.m_Span.DrillEndLayer(),
                             layer.m_Span.m_IsBackdrill, aOperation.m_NotPlated );

            if( aSlot && !span.m_IsBackdrill )
            {
                slot_holes[span.Pair()].push_back( item );
                return;
            }

            if( seen[span].insert( item ).second )
                drill_layers[span].push_back( item );
        };

        for( const DRILL_OPERATION& op : layer.m_Holes )
        {
            has_pth_layer |= !op.m_NotPlated && !layer.m_Span.m_IsBackdrill;
            addOperation( op, false );
        }

        for( const DRILL_OPERATION& op : layer.m_Slots )
            addOperation( op, true );
    }

    if( has_npth_layer )
    {
        DRILL_SPAN npthSpan( F_Cu, B_Cu, false, true );
        drill_layers[npthSpan];
    }

    if( has_pth_layer )
    {
        DRILL_SPAN platedSpan( F_Cu, B_Cu, false, false );
        drill_layers[platedSpan];
    }

    int backdrillIndex = 1;

    auto assignName = [&]( const DRILL_SPAN& aSpan )
    {
        wxString name;

        if( aSpan.m_IsBackdrill )
        {
            name.Printf( wxT( "drill%d" ), backdrillIndex++ );
        }
        else
        {
            wxString platedLabel = aSpan.m_IsNonPlatedFile ? wxT( "non-plated" ) : wxT( "plated" );
            name.Printf( wxT( "drill_%s_%s-%s" ), platedLabel, m_board->GetLayerName( aSpan.TopLayer() ),
                         m_board->GetLayerName( aSpan.BottomLayer() ) );
        }

        return ODB::GenLegalEntityName( name );
    };

    auto InitDrillMatrix = [&]( const DRILL_SPAN& aSpan )
    {
        wxString     dLayerName = assignName( aSpan );
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


void ODB_MATRIX_ENTITY::AddRoutMatrixLayer()
{
    MATRIX_LAYER matrix( m_row++, wxS( "rout" ) );
    matrix.m_type = ODB_TYPE::ROUT;
    matrix.m_info.m_role = ODB_LAYER_ROLE::ROUT;
    matrix.m_span.emplace( ODB::GenLegalEntityName( m_board->GetLayerName( F_Cu ) ),
                           ODB::GenLegalEntityName( m_board->GetLayerName( B_Cu ) ) );
    m_matrixLayers.push_back( matrix );
}


void ODB_MATRIX_ENTITY::AddCOMPMatrixLayer( PCB_LAYER_ID aCompSide )
{
    // A component layer must have a components file, so a side with no components has no layer
    if( aCompSide == F_Cu ? !m_hasTopComp : !m_hasBotComp )
        return;

    MATRIX_LAYER matrix( m_row++, aCompSide == F_Cu ? "COMP_+_TOP" : "COMP_+_BOT" );
    matrix.m_type = ODB_TYPE::COMPONENT;
    matrix.m_info.m_role = ODB_LAYER_ROLE::COMPONENT;
    matrix.m_info.m_componentSide = aCompSide;
    matrix.m_context = ODB_CONTEXT::BOARD;
    m_matrixLayers.push_back( matrix );
}

void ODB_MATRIX_ENTITY::AddAuxilliaryMatrixLayer()
{
    auto& auxilliary_layers = m_plugin->GetAuxilliaryLayerItemsMap();
    auxilliary_layers.clear();

    const FAB_DRILL_MODEL& model = m_plugin->GetFabDrillModel();

    for( BOARD_ITEM* item : m_board->Tracks() )
    {
        if( item->Type() == PCB_VIA_T )
        {
            PCB_VIA* via = static_cast<PCB_VIA*>( item );
            const DRILL_OPERATION* op = model.Find(
                    DRILL_SPAN( via->TopLayer(), via->BottomLayer(), false, false ), via );

            if( !op )
                continue;

            if( op->m_Filled )
            {
                auxilliary_layers[std::make_tuple( ODB_AUX_LAYER_TYPE::FILLING, via->TopLayer(),
                                                   via->BottomLayer() )]
                        .push_back( via );
            }

            if( op->m_Capped )
            {
                auxilliary_layers[std::make_tuple( ODB_AUX_LAYER_TYPE::CAPPING, via->TopLayer(),
                                                   via->BottomLayer() )]
                        .push_back( via );
            }

            for( PCB_LAYER_ID layer : { via->TopLayer(), via->BottomLayer() } )
            {
                bool top = layer == op->m_TopLayer;

                if( top ? op->m_TopPlugged : op->m_BottomPlugged )
                {
                    auxilliary_layers[std::make_tuple( ODB_AUX_LAYER_TYPE::PLUGGING, layer,
                                                       PCB_LAYER_ID::UNDEFINED_LAYER )]
                            .push_back( via );
                }

                if( top ? op->m_TopCovered : op->m_BottomCovered )
                {
                    auxilliary_layers[std::make_tuple( ODB_AUX_LAYER_TYPE::COVERING, layer,
                                                       PCB_LAYER_ID::UNDEFINED_LAYER )]
                            .push_back( via );
                }

                if( top ? op->m_TopTented : op->m_BottomTented )
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
    std::vector<wxString> bases;
    bases.reserve( m_matrixLayers.size() );

    for( const MATRIX_LAYER& layer : m_matrixLayers )
        bases.push_back( layer.m_layerName );

    std::vector<wxString> names = ODB::UniqueNames( bases, {}, 64 );

    for( size_t index = 0; index < names.size(); ++index )
        m_matrixLayers[index].m_layerName = std::move( names[index] );
}


void ODB_MATRIX_ENTITY::GenerateFiles( ODB_TREE_WRITER& writer )
{
    auto fileproxy = writer.CreateFileProxy( "matrix" );

    ODB_TEXT_WRITER twriter( fileproxy.GetStream() );

    for( const auto& [step_name, entry] : m_matrixSteps )
    {
        const auto array_proxy = twriter.MakeArrayProxy( "STEP" );
        twriter.WriteEquationLine( "COL", entry.first );
        twriter.WriteEquationLine( "ID", static_cast<int>( entry.second ) );
        twriter.WriteEquationLine( "NAME", step_name );
    }

    for( const MATRIX_LAYER& layer : m_matrixLayers )
    {
        const auto array_proxy = twriter.MakeArrayProxy( "LAYER" );
        twriter.WriteEquationLine( "ROW", layer.m_rowNumber );
        twriter.WriteEquationLine( "ID", static_cast<int>( layer.m_uid ) );
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

            if( !layer.m_dielectricName.IsEmpty() )
                twriter.WriteEquationLine( "DIELECTRIC_NAME", layer.m_dielectricName );

            if( layer.m_cuTop )
                twriter.WriteEquationLine( "CU_TOP", static_cast<int>( *layer.m_cuTop ) );

            if( layer.m_cuBottom )
                twriter.WriteEquationLine( "CU_BOTTOM", static_cast<int>( *layer.m_cuBottom ) );
        }

        if( layer.m_ref )
            twriter.WriteEquationLine( "REF", static_cast<int>( *layer.m_ref ) );

        if( layer.m_span.has_value() )
        {
            twriter.WriteEquationLine( "START_NAME", layer.m_span->first );
            twriter.WriteEquationLine( "END_NAME", layer.m_span->second );
        }

        twriter.WriteEquationLine( "COLOR", "0" );
    }

    if( m_plugin->GetFormat().Includes( FAB::SECTION::STACKUP ) )
        GenerateStackupFile( writer );
}


void ODB_MATRIX_ENTITY::GenerateStackupFile( ODB_TREE_WRITER& writer )
{
    const ODB_FORMAT&  format = m_plugin->GetFormat();
    const FAB_STACKUP& stackup = m_plugin->GetFabStackup();
    wxString           units = wxString::FromUTF8( format.m_unitsStr.c_str() );
    wxString           stepName = wxString::FromUTF8( ODB_STEP_ENTITY::STEP_NAME );
    wxXmlDocument      document;

    auto add = []( wxXmlNode* aParent, const wxString& aName )
    {
        wxXmlNode* node = new wxXmlNode( wxXML_ELEMENT_NODE, aName );

        if( aParent )
            aParent->AddChild( node );

        return node;
    };

    wxXmlNode* root = add( nullptr, wxS( "StackupFile" ) );
    document.SetRoot( root );
    root->AddAttribute( wxS( "Version" ), wxS( "8.1" ) );
    root->AddAttribute( wxS( "DefaultUnits" ), units );

    wxXmlNode* eda = add( root, wxS( "EdaData" ) );
    eda->AddAttribute( wxS( "CompanyName" ), m_board->GetTitleBlock().GetCompany() );
    wxXmlNode* specs = add( eda, wxS( "Specs" ) );
    wxXmlNode* spec = add( specs, wxS( "Spec" ) );
    spec->AddAttribute( wxS( "SpecName" ), stepName );
    wxXmlNode* stackupNode = add( eda, wxS( "Stackup" ) );
    stackupNode->AddAttribute( wxS( "StackupName" ), stepName );
    stackupNode->AddAttribute( wxS( "StackupThickness" ), ODB::Data2String( format, stackup.Thickness() ) );
    stackupNode->AddAttribute( wxS( "Units" ), units );
    bool maskThickness = false;

    for( PCB_LAYER_ID layer : { F_Mask, B_Mask } )
    {
        const BOARD_STACKUP_ITEM* item = stackup.ItemForLayer( layer );
        maskThickness |= item && item->IsEnabled() && item->GetThickness() > 0;
    }

    stackupNode->AddAttribute( wxS( "WhereMeasured" ), maskThickness ? wxS( "MASK" ) : wxS( "METAL" ) );
    wxXmlNode* group = add( stackupNode, wxS( "Group" ) );
    group->AddAttribute( wxS( "GroupName" ), stepName );

    for( const MATRIX_LAYER& row : m_matrixLayers )
    {
        if( row.m_context != ODB_CONTEXT::BOARD )
            continue;

        wxXmlNode* layer = add( group, wxS( "Layer" ) );
        layer->AddAttribute( wxS( "LayerName" ), row.m_layerName );
        layer->AddAttribute( wxS( "LayerType" ),
                             wxString::FromUTF8( ODB::EnumStringMap<ODB_TYPE>::GetMap().at( row.m_type ).c_str() ) );

        if( row.m_type == ODB_TYPE::DIELECTRIC )
            layer->AddAttribute( wxS( "Side" ), wxS( "INNER" ) );
        else if( m_board->IsFrontLayer( row.m_info.m_layer ) )
            layer->AddAttribute( wxS( "Side" ), wxS( "TOP" ) );
        else if( m_board->IsBackLayer( row.m_info.m_layer ) )
            layer->AddAttribute( wxS( "Side" ), wxS( "BOTTOM" ) );
        else if( IsCopperLayer( row.m_info.m_layer ) )
            layer->AddAttribute( wxS( "Side" ), wxS( "INNER" ) );

        if( row.m_span && ( row.m_type == ODB_TYPE::DRILL || row.m_type == ODB_TYPE::ROUT ) )
        {
            if( !row.m_span->first.IsEmpty() )
                layer->AddAttribute( wxS( "MechStartLayerName" ), row.m_span->first );

            if( !row.m_span->second.IsEmpty() )
                layer->AddAttribute( wxS( "MechEndLayerName" ), row.m_span->second );
        }

        const BOARD_STACKUP_ITEM* item = StackupItemForLayer( stackup, row.m_info );
        int                       sublayer = row.m_info.m_sublayer;

        if( !item || !item->IsThicknessEditable() || item->GetThickness( sublayer ) <= 0 )
            continue;

        wxXmlNode* material = add( spec, wxS( "Material" ) );
        material->AddAttribute( wxS( "MaterialName" ), row.m_layerName );

        if( item->GetType() == BS_ITEM_TYPE_COPPER )
        {
            wxXmlNode* conductor = add( material, wxS( "Conductor" ) );
            conductor->AddAttribute( wxS( "ConductorType" ), wxS( "COPPER" ) );
            conductor->AddAttribute( wxS( "CopperWeight_oz_ft2" ),
                                     ODB::Double2String( format, CopperWeightOz( item->GetThickness( sublayer ) ) ) );
        }
        else
        {
            wxXmlNode* dielectric = add( material, wxS( "Dielectric" ) );

            if( item->GetType() == BS_ITEM_TYPE_SOLDERMASK )
            {
                dielectric->AddAttribute( wxS( "DielectricType" ), wxS( "OTHER" ) );
                dielectric->AddAttribute( wxS( "OtherSubType" ), wxS( "SOLDER_MASK" ) );
            }
            else
            {
                dielectric->AddAttribute( wxS( "DielectricType" ),
                                          row.m_diType == ODB_DIELECTRIC_TYPE::CORE      ? wxS( "CORE" )
                                          : row.m_diType == ODB_DIELECTRIC_TYPE::PREPREG ? wxS( "PREPREG" )
                                                                                         : wxS( "UNDEFINED" ) );
            }

            wxString reference = item->GetMaterial( sublayer );

            if( IsPrmSpecified( reference ) )
                dielectric->AddAttribute( wxS( "MaterialReference" ), reference );

            bool hasDk = item->HasEpsilonRValue() && item->GetEpsilonR( sublayer ) > 0.0;
            bool hasDf = item->HasLossTangentValue() && item->GetLossTangent( sublayer ) > 0.0;

            if( hasDk || hasDf )
            {
                wxXmlNode* properties = add( dielectric, wxS( "Properties" ) );
                properties->AddAttribute( wxS( "PropertyName" ), wxS( "default" ) );
                wxXmlNode* property = add( properties, wxS( "Property" ) );
                double     frequency = item->GetSpecFreq( sublayer );

                if( frequency > 0.0 )
                {
                    property->AddAttribute( wxS( "FrequencyVal" ), ODB::Double2String( format, frequency / 1e9 ) );
                    property->AddAttribute( wxS( "Units" ), wxS( "GHz" ) );
                }

                if( hasDk )
                {
                    property->AddAttribute( wxS( "DielectricConstant_Dk" ),
                                            ODB::Double2String( format, item->GetEpsilonR( sublayer ) ) );
                }

                if( hasDf )
                {
                    property->AddAttribute( wxS( "LossTangent_Df" ),
                                            ODB::Double2String( format, item->GetLossTangent( sublayer ) ) );
                }
            }
        }

        wxXmlNode* thickness = add( material, wxS( "Default_Thickness" ) );
        thickness->AddAttribute( wxS( "Thickness" ), ODB::Data2String( format, item->GetThickness( sublayer ) ) );
        thickness->AddAttribute( wxS( "Units" ), units );

        wxXmlNode* ref = add( layer, wxS( "SpecRef" ) );
        ref->AddAttribute( wxS( "MaterialSpecName" ), stepName );
        wxXmlNode* named = add( ref, wxS( "Material" ) );
        named->AddAttribute( wxS( "MaterialName" ), row.m_layerName );
    }

    WriteXmlFile( writer, "stackup.xml", document, _( "Failed to generate ODB++ stackup." ) );
}


ODB_LAYER_ENTITY::ODB_LAYER_ENTITY( BOARD* aBoard, PCB_IO_ODBPP* aPlugin,
                                    const std::map<int, std::vector<BOARD_ITEM*>>& aMap,
                                    const ODB_LAYER_NAME& aLayer ) :
        ODB_ENTITY_BASE( aBoard, aPlugin ),
        m_layerItems( aMap ),
        m_layerID( aLayer.m_layer ),
        m_layer( aLayer )
{
    m_featuresMgr = std::make_unique<FEATURES_MANAGER>( aBoard, aPlugin, aLayer.m_name, aLayer.m_role, aLayer.m_auxType,
                                                        aLayer.m_drillSpan );
}


void ODB_LAYER_ENTITY::InitEntityData()
{
    if( m_layer.m_role == ODB_LAYER_ROLE::ROUT )
    {
        InitRoutData();
        return;
    }

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


void ODB_LAYER_ENTITY::InitRoutData()
{
    SHAPE_POLY_SET outlines;
    bool hasOutline = m_board->GetBoardPolygonOutlines( outlines, false, nullptr, true );

    if( !hasOutline || outlines.OutlineCount() == 0 )
    {
        m_plugin->Report( _( "Board outline is not closed; ODB++ rout layer has no board contour." ),
                          RPT_SEVERITY_WARNING );
    }

    int chain = 1;
    bool edgePlated = m_plugin->GetFabStackup().Stackup().m_EdgePlating;

    for( int island = 0; hasOutline && island < outlines.OutlineCount(); ++island )
    {
        m_featuresMgr->AddRoutContour( outlines.COutline( island ), chain++, edgePlated );

        for( int hole = 0; hole < outlines.HoleCount( island ); ++hole )
            m_featuresMgr->AddRoutContour( outlines.CHole( island, hole ), chain++, edgePlated );
    }

    for( const auto& [span, items] : m_plugin->GetSlotHolesMap() )
    {
        for( BOARD_ITEM* item : items )
        {
            if( item->Type() == PCB_PAD_T )
                m_featuresMgr->AddRoutSlot( *static_cast<PAD*>( item ), chain++ );
        }
    }
}


ODB_COMPONENT& ODB_LAYER_ENTITY::InitComponentData( const FOOTPRINT*         aFp,
                                                    const EDA_DATA::PACKAGE& aPkg )
{
    if( !m_components.has_value() )
        m_components.emplace( m_plugin );

    return m_components->AddComponent( aFp, aPkg );
}


void ODB_LAYER_ENTITY::InitDrillData()
{
    std::map<DRILL_SPAN, std::vector<BOARD_ITEM*>>& drill_layers = m_plugin->GetDrillLayerItemsMap();

    std::map<std::pair<PCB_LAYER_ID, PCB_LAYER_ID>, std::vector<BOARD_ITEM*>>& slot_holes =
            m_plugin->GetSlotHolesMap();

    wxCHECK_RET( m_layer.m_drillSpan.has_value(), "Drill matrix row has no span" );
    const DRILL_SPAN& matchedSpan = *m_layer.m_drillSpan;
    const FAB_DRILL_MODEL& model = m_plugin->GetFabDrillModel();

    if( !m_layerItems.empty() )
    {
        m_layerItems.clear();
    }

    m_tools.emplace( m_plugin->GetFormat().m_unitsStr );

    bool isBackdrillLayer = matchedSpan.m_IsBackdrill;
    bool isNonPlatedLayer = matchedSpan.m_IsNonPlatedFile;
    bool isNPTHLayer = matchedSpan.m_IsNonPlatedFile && !matchedSpan.m_IsBackdrill;

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
                                               std::min( pad->GetDrillSizeX(), pad->GetDrillSizeY() ),
                                               !padIsNPTH && GetFabPadRole( *pad ) == FAB_PAD_ROLE::PRESSFIT
                                                       ? wxT( "PRESS_FIT" ) : wxT( "STANDARD" ) );

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
                const DRILL_OPERATION* operation = model.Find( matchedSpan, via );

                if( !operation )
                    continue;

                if( isBackdrillLayer )
                {
                    m_tools.value().AddDrillTool( m_plugin->GetFormat(), wxT( "NON_PLATED" ),
                                                   operation->m_Diameter,
                                                   wxT( "BLIND" ) );
                }
                else if( isNonPlatedLayer )
                {
                    m_tools.value().AddDrillTool( m_plugin->GetFormat(), wxT( "NON_PLATED" ),
                                                   operation->m_Diameter );
                }
                else
                {
                    m_tools.value().AddDrillTool( m_plugin->GetFormat(), wxT( "VIA" ),
                                                   operation->m_Diameter,
                                                   via->GetViaType() == VIATYPE::MICROVIA ? wxT( "LASER" )
                                                                                        : wxT( "STANDARD" ) );
                }

                m_layerItems[via->GetNetCode()].push_back( item );
            }
            else if( item->Type() == PCB_PAD_T )
            {
                PAD* pad = static_cast<PAD*>( item );
                const DRILL_OPERATION* operation = model.Find( matchedSpan, pad );

                if( !operation )
                    continue;

                bool padIsNPTH = pad->GetAttribute() == PAD_ATTRIB::NPTH;

                if( isNPTHLayer && !padIsNPTH )
                    continue;

                if( !isNonPlatedLayer && padIsNPTH )
                    continue;

                int drillSize = operation->m_Diameter;

                wxString typeLabel = ( padIsNPTH || isNonPlatedLayer ) ? wxT( "NON_PLATED" ) : wxT( "PLATED" );
                wxString type2 = isBackdrillLayer ? wxT( "BLIND" ) : wxT( "STANDARD" );

                if( typeLabel == wxT( "PLATED" ) && GetFabPadRole( *pad ) == FAB_PAD_ROLE::PRESSFIT )
                    type2 = wxT( "PRESS_FIT" );

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

    if( m_components.has_value() )
        GenComponents( writer );

    if( m_tools.has_value() )
    {
        GenTools( writer );
    }
}


void ODB_LAYER_ENTITY::GenComponents( ODB_TREE_WRITER& writer )
{
    auto fileproxy = writer.CreateFileProxy( "components" );

    m_components->Write( fileproxy.GetStream() );
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
    ost << "UNITS=" << m_plugin->GetFormat().m_unitsStr << '\n';

    if( m_layer.m_drillSpan
        && IsCopperLayerLowerThan( m_layer.m_drillSpan->m_StartLayer, m_layer.m_drillSpan->m_EndLayer ) )
    {
        ost << ".drill_layer_direction=bottom2top" << std::endl;
    }

    if( m_layer.m_role == ODB_LAYER_ROLE::BACKDRILL && m_layer.m_drillSpan )
    {
        const DRILL_SPAN& span = *m_layer.m_drillSpan;
        PCB_LAYER_ID stop = FabBackdrillMustNotCut( m_board->GetEnabledLayers().CuStack(),
                                                    span.m_StartLayer, span.m_EndLayer );

        if( stop != UNDEFINED_LAYER )
        {
            const std::vector<ODB_LAYER_NAME>& layers = m_plugin->GetLayerNameList();
            auto row = std::find_if( layers.begin(), layers.end(),
                                     [stop]( const ODB_LAYER_NAME& aLayer )
                                     {
                                         return aLayer.m_role == ODB_LAYER_ROLE::BOARD_LAYER
                                                && aLayer.m_layer == stop;
                                     } );

            if( row != layers.end() )
                ost << ".backdrill_penetrate_stop_layer=" << row->m_name.ToStdString() << std::endl;
        }
    }

    const BOARD_STACKUP_ITEM* stackupItem = StackupItemForLayer( m_plugin->GetFabStackup(), m_layer );

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
            ost << ".copper_weight=" << ODB::Double2String( m_plugin->GetFormat(), CopperWeightOz( thickness ) )
                << std::endl;
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
            ost << "material=" << ODB::GenLegalEntityName( material ).ToStdString() << std::endl;
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
        m_edaData.AddNET( net, m_plugin->GetLegalNetName( net->GetNetCode() ) );
    }

    // for CMP
    size_t j = 0;
    bool   writeComponents = m_plugin->GetFormat().Includes( FAB::SECTION::COMPONENTS );
    std::array<wxString, 2> componentNames;

    for( const ODB_LAYER_NAME& layer : m_plugin->GetLayerNameList() )
    {
        if( layer.m_role == ODB_LAYER_ROLE::COMPONENT )
            componentNames[layer.m_componentSide == B_Cu ? 1 : 0] = layer.m_name;
    }

    for( const FOOTPRINT* fp : m_board->Footprints() )
    {
        // ODBPP only need unique PACKAGE in PKG record in eda/data file.
        // the PKG index can repeat to be ref in CMP record in component file.
        std::shared_ptr<FOOTPRINT> fp_pkg = m_edaData.GetEdaFootprints().at( j );
        ++j;

        const EDA_DATA::PACKAGE& eda_pkg =
                m_edaData.GetPackage( hash_fp_item( fp_pkg.get(), HASH_POS | REL_COORD ) );

        bool hasCourtyard = std::any_of( fp->GraphicalItems().begin(), fp->GraphicalItems().end(),
                                         []( const BOARD_ITEM* item )
                                         {
                                             return item->IsOnLayer( F_CrtYd ) || item->IsOnLayer( B_CrtYd );
                                         } );

        if( fp->Pads().empty() && !hasCourtyard && !ODB::HasShownModel( fp ) )
            continue;

        ODB_COMPONENT* comp = nullptr;

        if( writeComponents )
        {
            auto iter = m_layerEntityMap.find( componentNames[fp->IsFlipped() ? 1 : 0] );

            if( iter != m_layerEntityMap.end() )
                comp = &iter->second->InitComponentData( fp, eda_pkg );
        }

        for( int i = 0; i < (int) fp->Pads().size(); ++i )
        {
            PAD*           pad = fp->Pads()[i];
            EDA_DATA::NET& eda_net = m_edaData.GetNet( pad->GetNetCode() );

            EDA_DATA::SUB_NET_TOEPRINT& subnet = eda_net.AddSubnet<EDA_DATA::SUB_NET_TOEPRINT>(
                    &m_edaData,
                    fp->IsFlipped() ? EDA_DATA::SUB_NET_TOEPRINT::SIDE::BOTTOM : EDA_DATA::SUB_NET_TOEPRINT::SIDE::TOP,
                    comp ? comp->m_index : 0, comp ? comp->m_toeprints.size() : i );

            m_plugin->GetPadSubnetMap().emplace( pad, &subnet );

            if( !comp )
                continue;

            const std::shared_ptr<EDA_DATA::PIN> pin = eda_pkg.GetEdaPkgPin( i );
            const EDA_DATA::PIN&                 pin_ref = *pin;
            auto&                                toep = comp->m_toeprints.emplace_back( pin_ref );

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
            if( !IsCopperLayer( layer ) )
                continue;

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

    if( m_plugin->GetFormat().Includes( FAB::SECTION::PHYSICAL_NET ) )
    {
        writer.CreateEntityDirectory( step_root, "netlists/cadnet" );
        GenerateNetlistsFiles( writer );
    }

    writer.SetCurrentPath( step_root );
    GenerateProfileFile( writer );

    GenerateStepHeaderFile( writer );
    GenerateAttrListFile( writer );
}


void ODB_STEP_ENTITY::GenerateAttrListFile( ODB_TREE_WRITER& writer )
{
    auto attrlist = writer.CreateFileProxy( "attrlist" );
    int               thickness = m_plugin->GetFabStackup().Thickness();
    const ODB_FORMAT& format = m_plugin->GetFormat();
    attrlist.GetStream() << "UNITS=" << format.m_unitsStr << '\n';
    attrlist.GetStream() << ".board_thickness="
                         << ODB::Double2String( format, format.m_scale * thickness * 1000.0 ) << '\n';

    wxString selected = format.m_variantNames.SelectedName( format.m_variantName );

    if( !selected.IsEmpty() )
    {
        attrlist.GetStream() << ".current_variant=" << selected.ToStdString() << '\n';
    }
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


namespace
{
struct SHORT_SHAPE
{
    std::set<int> m_nets;
    wxString      m_layer;
    size_t        m_featureIndex;
};


std::vector<SHORT_SHAPE> CollectNetTieShorts( BOARD* aBoard, PCB_IO_ODBPP* aPlugin )
{
    std::vector<SHORT_SHAPE> shortShapes;

    for( const auto& [shapeLayer, feature] : aPlugin->GetNetTieFeatures() )
    {
        const auto* shape = static_cast<const PCB_SHAPE*>( shapeLayer.first );
        PCB_LAYER_ID layer = shapeLayer.second;
        const FOOTPRINT* footprint = shape->GetParentFootprint();
        std::map<wxString, int> groups = footprint->MapPadNumbersToNetTieGroups();
        auto copper = shape->GetEffectiveShape( layer );
        std::set<int> netCodes;
        int group = -1;
        bool invalidGroup = false;

        // The DRC net tie cache lists permitted nets, not physical pad contact
        for( const PAD* pad : footprint->Pads() )
        {
            if( pad->GetNetCode() <= 0 || !pad->GetLayerSet()[layer]
                || !pad->GetEffectiveShape( layer )->Collide( copper.get() ) )
            {
                continue;
            }

            auto padGroup = groups.find( pad->GetNumber() );
            int currentGroup = padGroup == groups.end() ? -1 : padGroup->second;

            if( currentGroup < 0 || ( group >= 0 && group != currentGroup ) )
                invalidGroup = true;

            group = currentGroup;
            netCodes.insert( pad->GetNetCode() );
        }

        if( invalidGroup )
        {
            wxLogTrace( traceOdbppIo, wxT( "Net tie shape touches pads outside one group" ) );
            continue;
        }

        if( netCodes.size() >= 2 )
            shortShapes.push_back( { std::move( netCodes ), feature.first, feature.second } );
    }

    for( const FOOTPRINT* footprint : aBoard->Footprints() )
    {
        if( !footprint->IsNetTie() )
            continue;

        bool hasCopperShape = std::any_of( footprint->GraphicalItems().begin(),
                                           footprint->GraphicalItems().end(),
                                           []( const BOARD_ITEM* item )
                                           {
                                               return item->Type() == PCB_SHAPE_T
                                                       && ( item->GetLayerSet() & LSET::AllCuMask() ).any();
                                           } );

        if( !hasCopperShape )
            wxLogTrace( traceOdbppIo, wxT( "Net tie has no copper shape for shortf" ) );
    }

    std::sort( shortShapes.begin(), shortShapes.end(), []( const SHORT_SHAPE& left, const SHORT_SHAPE& right )
               { return std::tie( left.m_layer, left.m_featureIndex )
                        < std::tie( right.m_layer, right.m_featureIndex ); } );

    return shortShapes;
}
}


void ODB_STEP_ENTITY::GenerateEdaFiles( ODB_TREE_WRITER& writer )
{
    std::vector<SHORT_SHAPE> shortShapes;

    if( m_plugin->GetFormat().m_writeEdaNets )
        shortShapes = CollectNetTieShorts( m_board, m_plugin );

    if( !shortShapes.empty() )
    {
        m_edaData.AssignNetUids( *m_plugin );

        for( const SHORT_SHAPE& shape : shortShapes )
            m_edaData.AddShort( shape.m_nets, shape.m_layer, shape.m_featureIndex, m_plugin->NewUid() );
    }

    auto fileproxy = writer.CreateFileProxy( "data" );

    m_edaData.Write( fileproxy.GetStream(), m_plugin->GetFormat() );

    if( m_edaData.HasShorts() )
    {
        auto shorts = writer.CreateFileProxy( "shortf" );
        m_edaData.WriteShortf( shorts.GetStream(), m_plugin->GetFormat() );
    }
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
    FAB_LAYER_INDEX index( *m_board );

    for( const ODB_LAYER_NAME& layer : m_plugin->GetLayerNameList() )
    {
        std::shared_ptr<ODB_LAYER_ENTITY> layerEntity =
                std::make_shared<ODB_LAYER_ENTITY>( m_board, m_plugin, index.Items( layer.m_layer ), layer );

        m_layerEntityMap.emplace( layer.m_name, layerEntity );
    }
}
