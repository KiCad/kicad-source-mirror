/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>
#include <magic_enum.hpp>
#include <import_export.h>
#include <qa_utils/wx_utils/wx_assert.h>
#include <qa_utils/api_test_utils.h>

// Common
#include <api/api_enums.h>
#include <api/board/board.pb.h>
#include <api/common/types/enums.pb.h>
#include <eda_shape.h>
#include <core/typeinfo.h>
#include <font/text_attributes.h>
#include <layer_ids.h>
#include <pin_type.h>
#include <stroke_params.h>
#include <widgets/report_severity.h>

// Board-specific
#include <api/board/board_types.pb.h>
#include <api/board/board_commands.pb.h>
#include <api/board/board_jobs.pb.h>
#include <api/schematic/schematic_jobs.pb.h>
#include <board_stackup_manager/board_stackup.h>
#include <constraints/pcb_constraint.h>
#include <jobs/job_export_sch_netlist.h>
#include <jobs/job_export_sch_plot.h>
#include <jobs/job_export_pcb_3d.h>
#include <jobs/job_export_pcb_dxf.h>
#include <jobs/job_export_pcb_drill.h>
#include <jobs/job_export_pcb_ipc2581.h>
#include <jobs/job_export_pcb_odb.h>
#include <jobs/job_export_pcb_pdf.h>
#include <jobs/job_export_pcb_pos.h>
#include <jobs/job_export_pcb_ps.h>
#include <jobs/job_export_pcb_stats.h>
#include <jobs/job_export_pcb_svg.h>
#include <jobs/job_pcb_render.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <padstack.h>
#include <pcb_dimension.h>
#include <pcb_track.h>
#include <plotprint_opts.h>
#include <project/board_project_settings.h>
#include <zones.h>
#include <zone_settings.h>

using namespace kiapi::common;

BOOST_AUTO_TEST_SUITE( ApiEnums )

BOOST_AUTO_TEST_CASE( HorizontalAlignment )
{
    testEnums<GR_TEXT_H_ALIGN_T, types::HorizontalAlignment>();
}

BOOST_AUTO_TEST_CASE( VerticalAlignment )
{
    testEnums<GR_TEXT_V_ALIGN_T, types::VerticalAlignment>();
}

BOOST_AUTO_TEST_CASE( StrokeLineStyle )
{
    testEnums<LINE_STYLE, types::StrokeLineStyle>();
}

BOOST_AUTO_TEST_CASE( GraphicFillType )
{
    testEnums<FILL_T, types::GraphicFillType>();
}

BOOST_AUTO_TEST_CASE( KiCadObjectType )
{
    testEnums<KICAD_T, types::KiCadObjectType>( true );
}

BOOST_AUTO_TEST_CASE( ElectricalPinType )
{
    testEnums<ELECTRICAL_PINTYPE, types::ElectricalPinType>( true );
}

BOOST_AUTO_TEST_CASE( BoardLayer )
{
    testEnums<PCB_LAYER_ID, kiapi::board::types::BoardLayer>( true );
}

BOOST_AUTO_TEST_CASE( PadStackShape )
{
    testEnums<PAD_SHAPE, kiapi::board::types::PadStackShape>();
}

BOOST_AUTO_TEST_CASE( ZoneConnectionStyle )
{
    testEnums<ZONE_CONNECTION, kiapi::board::types::ZoneConnectionStyle>();
}

BOOST_AUTO_TEST_CASE( PadType )
{
    testEnums<PAD_ATTRIB, kiapi::board::types::PadType>();
}

BOOST_AUTO_TEST_CASE( PadStackType )
{
    testEnums<PADSTACK::MODE, kiapi::board::types::PadStackType>();
}

BOOST_AUTO_TEST_CASE( DrillShape )
{
    testEnums<PAD_DRILL_SHAPE, kiapi::board::types::DrillShape>();
}

BOOST_AUTO_TEST_CASE( UnconnectedLayerRemoval )
{
    testEnums<UNCONNECTED_LAYER_MODE, kiapi::board::types::UnconnectedLayerRemoval>();
}

BOOST_AUTO_TEST_CASE( ViaType )
{
    // VIATYPE::NOT_DEFINED is not mapped
    testEnums<VIATYPE, kiapi::board::types::ViaType>( true );
}

BOOST_AUTO_TEST_CASE( IslandRemovalMode )
{
    testEnums<ISLAND_REMOVAL_MODE, kiapi::board::types::IslandRemovalMode>();
}

BOOST_AUTO_TEST_CASE( ZoneFillMode )
{
    testEnums<ZONE_FILL_MODE, kiapi::board::types::ZoneFillMode>();
}

BOOST_AUTO_TEST_CASE( ZoneBorderStyle )
{
    testEnums<ZONE_BORDER_DISPLAY_STYLE, kiapi::board::types::ZoneBorderStyle>();
}

BOOST_AUTO_TEST_CASE( PlacementRuleSourceType )
{
    testEnums<PLACEMENT_SOURCE_T, kiapi::board::types::PlacementRuleSourceType>();
}

BOOST_AUTO_TEST_CASE( TeardropType )
{
    testEnums<TEARDROP_TYPE, kiapi::board::types::TeardropType>();
}

BOOST_AUTO_TEST_CASE( TeardropTarget )
{
    testEnums<TARGET_TD, kiapi::board::TeardropTarget>( true );
}

BOOST_AUTO_TEST_CASE( DimensionTextBorderStyle )
{
    testEnums<DIM_TEXT_BORDER, kiapi::board::types::DimensionTextBorderStyle>();
}

BOOST_AUTO_TEST_CASE( DimensionUnitFormat )
{
    testEnums<DIM_UNITS_FORMAT, kiapi::board::types::DimensionUnitFormat>();
}

BOOST_AUTO_TEST_CASE( DimensionArrowDirection )
{
    testEnums<DIM_ARROW_DIRECTION, kiapi::board::types::DimensionArrowDirection>();
}

BOOST_AUTO_TEST_CASE( DimensionPrecision )
{
    testEnums<DIM_PRECISION, kiapi::board::types::DimensionPrecision>();
}

BOOST_AUTO_TEST_CASE( DimensionTextPosition )
{
    testEnums<DIM_TEXT_POSITION, kiapi::board::types::DimensionTextPosition>();
}

BOOST_AUTO_TEST_CASE( DimensionUnit )
{
    testEnums<DIM_UNITS_MODE, kiapi::board::types::DimensionUnit>();
}

BOOST_AUTO_TEST_CASE( InactiveLayerDisplayMode )
{
    testEnums<HIGH_CONTRAST_MODE, kiapi::board::commands::InactiveLayerDisplayMode>();
}

BOOST_AUTO_TEST_CASE( NetColorDisplayMode )
{
    testEnums<NET_COLOR_MODE, kiapi::board::commands::NetColorDisplayMode>();
}

BOOST_AUTO_TEST_CASE( RatsnestDisplayMode )
{
    testEnums<RATSNEST_MODE, kiapi::board::commands::RatsnestDisplayMode>();
}

BOOST_AUTO_TEST_CASE( BoardStackupLayerType )
{
    testEnums<BOARD_STACKUP_ITEM_TYPE, kiapi::board::BoardStackupLayerType>();
}

BOOST_AUTO_TEST_CASE( DrcSeverity )
{
    testEnums<SEVERITY, kiapi::board::commands::DrcSeverity>();
}

BOOST_AUTO_TEST_CASE( RuleSeverity )
{
    testEnums<SEVERITY, kiapi::common::types::RuleSeverity>();
}

BOOST_AUTO_TEST_CASE( ConstraintType )
{
    // UNDEFINED is an internal sentinel that is not exposed to the API.
    testEnums<PCB_CONSTRAINT_TYPE, kiapi::board::types::ConstraintType>( true );
}

BOOST_AUTO_TEST_CASE( ConstraintAnchor )
{
    testEnums<CONSTRAINT_ANCHOR, kiapi::board::types::ConstraintAnchor>();
}

BOOST_AUTO_TEST_CASE( DesignRuleType )
{
    using ProtoType = kiapi::board::DrcErrorType;

    for( PCB_DRC_CODE value : { DRCE_UNCONNECTED_ITEMS,
                                DRCE_SHORTING_ITEMS,
                                DRCE_ALLOWED_ITEMS,
                                DRCE_TEXT_ON_EDGECUTS,
                                DRCE_CLEARANCE,
                                DRCE_CREEPAGE,
                                DRCE_TRACKS_CROSSING,
                                DRCE_EDGE_CLEARANCE,
                                DRCE_ZONES_INTERSECT,
                                DRCE_ISOLATED_COPPER,
                                DRCE_STARVED_THERMAL,
                                DRCE_DANGLING_VIA,
                                DRCE_DANGLING_TRACK,
                                DRCE_DRILLED_HOLES_TOO_CLOSE,
                                DRCE_DRILLED_HOLES_COLOCATED,
                                DRCE_HOLE_CLEARANCE,
                                DRCE_CONNECTION_WIDTH,
                                DRCE_TRACK_WIDTH,
                                DRCE_TRACK_ANGLE,
                                DRCE_TRACK_SEGMENT_LENGTH,
                                DRCE_ANNULAR_WIDTH,
                                DRCE_DRILL_OUT_OF_RANGE,
                                DRCE_VIA_DIAMETER,
                                DRCE_PADSTACK,
                                DRCE_PADSTACK_INVALID,
                                DRCE_MICROVIA_DRILL_OUT_OF_RANGE,
                                DRCE_OVERLAPPING_FOOTPRINTS,
                                DRCE_MISSING_COURTYARD,
                                DRCE_MALFORMED_COURTYARD,
                                DRCE_PTH_IN_COURTYARD,
                                DRCE_NPTH_IN_COURTYARD,
                                DRCE_DISABLED_LAYER_ITEM,
                                DRCE_INVALID_OUTLINE,
                                DRCE_MISSING_FOOTPRINT,
                                DRCE_DUPLICATE_FOOTPRINT,
                                DRCE_NET_CONFLICT,
                                DRCE_EXTRA_FOOTPRINT,
                                DRCE_SCHEMATIC_PARITY,
                                DRCE_SCHEMATIC_FIELDS_PARITY,
                                DRCE_FOOTPRINT_FILTERS,
                                DRCE_LIB_FOOTPRINT_ISSUES,
                                DRCE_LIB_FOOTPRINT_MISMATCH,
                                DRCE_UNRESOLVED_VARIABLE,
                                DRCE_ASSERTION_FAILURE,
                                DRCE_GENERIC_WARNING,
                                DRCE_GENERIC_ERROR,
                                DRCE_COPPER_SLIVER,
                                DRCE_SILK_CLEARANCE,
                                DRCE_SILK_MASK_CLEARANCE,
                                DRCE_SILK_EDGE_CLEARANCE,
                                DRCE_SOLDERMASK_BRIDGE,
                                DRCE_TEXT_HEIGHT,
                                DRCE_TEXT_THICKNESS,
                                DRCE_LENGTH_OUT_OF_RANGE,
                                DRCE_SKEW_OUT_OF_RANGE,
                                DRCE_VIA_COUNT_OUT_OF_RANGE,
                                DRCE_DIFF_PAIR_GAP_OUT_OF_RANGE,
                                DRCE_DIFF_PAIR_UNCOUPLED_LENGTH_TOO_LONG,
                                DRCE_FOOTPRINT,
                                DRCE_FOOTPRINT_TYPE_MISMATCH,
                                DRCE_PAD_TH_WITH_NO_HOLE,
                                DRCE_MIRRORED_TEXT_ON_FRONT_LAYER,
                                DRCE_NONMIRRORED_TEXT_ON_BACK_LAYER,
                                DRCE_MISSING_TUNING_PROFILE,
                                DRCE_TRACK_ON_POST_MACHINED_LAYER,
                                DRCE_TRACK_NOT_CENTERED_ON_VIA } )
    {
        ProtoType proto = ToProtoEnum<PCB_DRC_CODE, ProtoType>( value );
        BOOST_REQUIRE( proto != ProtoType::DRCET_UNKNOWN );
        BOOST_CHECK( ( FromProtoEnum<PCB_DRC_CODE, ProtoType>( proto ) == value ) );
    }

    BOOST_CHECK( ( FromProtoEnum<PCB_DRC_CODE, ProtoType>( ProtoType::DRCET_UNKNOWN )
                   == static_cast<PCB_DRC_CODE>( 0 ) ) );
}

BOOST_AUTO_TEST_CASE( CustomRuleConstraintType )
{
    using ProtoType = kiapi::board::CustomRuleConstraintType;

    for( DRC_CONSTRAINT_T value : magic_enum::enum_values<DRC_CONSTRAINT_T>() )
    {
        if( value == NULL_CONSTRAINT )
            continue;

        ProtoType proto = ToProtoEnum<DRC_CONSTRAINT_T, ProtoType>( value );
        BOOST_REQUIRE( proto != ProtoType::CRCT_UNKNOWN );
        BOOST_CHECK( ( FromProtoEnum<DRC_CONSTRAINT_T, ProtoType>( proto ) == value ) );
    }

    BOOST_CHECK( ( FromProtoEnum<DRC_CONSTRAINT_T, ProtoType>( ProtoType::CRCT_UNKNOWN )
                   == NULL_CONSTRAINT ) );
}

BOOST_AUTO_TEST_CASE( CustomRuleConstraintOption )
{
    using ProtoType = kiapi::board::CustomRuleConstraintOption;

    for( DRC_CONSTRAINT::OPTIONS value : magic_enum::enum_values<DRC_CONSTRAINT::OPTIONS>() )
    {
        if( value == DRC_CONSTRAINT::OPTIONS::NUM_OPTIONS )
            continue;

        ProtoType proto = ToProtoEnum<DRC_CONSTRAINT::OPTIONS, ProtoType>( value );
        BOOST_REQUIRE( proto != ProtoType::CRCO_UNKNOWN );
        BOOST_CHECK( ( FromProtoEnum<DRC_CONSTRAINT::OPTIONS, ProtoType>( proto ) == value ) );
    }
}

BOOST_AUTO_TEST_CASE( CustomRuleDisallowType )
{
    using ProtoType = kiapi::board::CustomRuleDisallowType;

    for( DRC_DISALLOW_T value : { DRC_DISALLOW_THROUGH_VIAS, DRC_DISALLOW_MICRO_VIAS,
                                  DRC_DISALLOW_BLIND_VIAS, DRC_DISALLOW_BURIED_VIAS,
                                  DRC_DISALLOW_TRACKS, DRC_DISALLOW_PADS,
                                  DRC_DISALLOW_ZONES, DRC_DISALLOW_TEXTS,
                                  DRC_DISALLOW_GRAPHICS, DRC_DISALLOW_HOLES,
                                  DRC_DISALLOW_FOOTPRINTS } )
    {
        ProtoType proto = ToProtoEnum<DRC_DISALLOW_T, ProtoType>( value );
        BOOST_REQUIRE( proto != ProtoType::CRDT_UNKNOWN );
        BOOST_CHECK( ( FromProtoEnum<DRC_DISALLOW_T, ProtoType>( proto ) == value ) );
    }
}

BOOST_AUTO_TEST_CASE( PlotDrillMarks )
{
    testEnums<DRILL_MARKS, kiapi::board::jobs::PlotDrillMarks>();
}

BOOST_AUTO_TEST_CASE( Board3DFormat )
{
    testEnums<JOB_EXPORT_PCB_3D::FORMAT, kiapi::board::jobs::Board3DFormat>( true );
}

BOOST_AUTO_TEST_CASE( Board3DVRMLUnits )
{
    testEnums<JOB_EXPORT_PCB_3D::VRML_UNITS, kiapi::common::types::Units>( true );
}

BOOST_AUTO_TEST_CASE( RenderFormat )
{
    testEnums<JOB_PCB_RENDER::FORMAT, kiapi::board::jobs::RenderFormat>();
}

BOOST_AUTO_TEST_CASE( RenderQuality )
{
    testEnums<JOB_PCB_RENDER::QUALITY, kiapi::board::jobs::RenderQuality>();
}

BOOST_AUTO_TEST_CASE( RenderBackgroundStyle )
{
    testEnums<JOB_PCB_RENDER::BG_STYLE, kiapi::board::jobs::RenderBackgroundStyle>();
}

BOOST_AUTO_TEST_CASE( RenderSide )
{
    testEnums<JOB_PCB_RENDER::SIDE, kiapi::board::jobs::RenderSide>();
}

BOOST_AUTO_TEST_CASE( SvgPaginationMode )
{
    using Mode = kiapi::board::jobs::BoardJobPaginationMode;

    BOOST_CHECK( ( ToProtoEnum<JOB_EXPORT_PCB_SVG::GEN_MODE, Mode>( JOB_EXPORT_PCB_SVG::GEN_MODE::SINGLE )
                   == Mode::BJPM_ALL_LAYERS_ONE_PAGE ) );
    BOOST_CHECK( ( ToProtoEnum<JOB_EXPORT_PCB_SVG::GEN_MODE, Mode>( JOB_EXPORT_PCB_SVG::GEN_MODE::MULTI )
                   == Mode::BJPM_EACH_LAYER_OWN_FILE ) );

    BOOST_CHECK( ( FromProtoEnum<JOB_EXPORT_PCB_SVG::GEN_MODE, Mode>( Mode::BJPM_ALL_LAYERS_ONE_PAGE )
                   == JOB_EXPORT_PCB_SVG::GEN_MODE::SINGLE ) );
    BOOST_CHECK( ( FromProtoEnum<JOB_EXPORT_PCB_SVG::GEN_MODE, Mode>( Mode::BJPM_EACH_LAYER_OWN_FILE )
                   == JOB_EXPORT_PCB_SVG::GEN_MODE::MULTI ) );
    BOOST_CHECK( ( FromProtoEnum<JOB_EXPORT_PCB_SVG::GEN_MODE, Mode>( Mode::BJPM_EACH_LAYER_OWN_PAGE )
                   == JOB_EXPORT_PCB_SVG::GEN_MODE::SINGLE ) );
}

BOOST_AUTO_TEST_CASE( DxfPaginationMode )
{
    using Mode = kiapi::board::jobs::BoardJobPaginationMode;

    BOOST_CHECK( ( ToProtoEnum<JOB_EXPORT_PCB_DXF::GEN_MODE, Mode>( JOB_EXPORT_PCB_DXF::GEN_MODE::SINGLE )
                   == Mode::BJPM_ALL_LAYERS_ONE_PAGE ) );
    BOOST_CHECK( ( ToProtoEnum<JOB_EXPORT_PCB_DXF::GEN_MODE, Mode>( JOB_EXPORT_PCB_DXF::GEN_MODE::MULTI )
                   == Mode::BJPM_EACH_LAYER_OWN_FILE ) );

    BOOST_CHECK( ( FromProtoEnum<JOB_EXPORT_PCB_DXF::GEN_MODE, Mode>( Mode::BJPM_ALL_LAYERS_ONE_PAGE )
                   == JOB_EXPORT_PCB_DXF::GEN_MODE::SINGLE ) );
    BOOST_CHECK( ( FromProtoEnum<JOB_EXPORT_PCB_DXF::GEN_MODE, Mode>( Mode::BJPM_EACH_LAYER_OWN_FILE )
                   == JOB_EXPORT_PCB_DXF::GEN_MODE::MULTI ) );
    BOOST_CHECK( ( FromProtoEnum<JOB_EXPORT_PCB_DXF::GEN_MODE, Mode>( Mode::BJPM_EACH_LAYER_OWN_PAGE )
                   == JOB_EXPORT_PCB_DXF::GEN_MODE::SINGLE ) );
}

BOOST_AUTO_TEST_CASE( DxfUnits )
{
    testEnums<JOB_EXPORT_PCB_DXF::DXF_UNITS, kiapi::common::types::Units>( true );
}

BOOST_AUTO_TEST_CASE( PdfPaginationMode )
{
    testEnums<JOB_EXPORT_PCB_PDF::GEN_MODE, kiapi::board::jobs::BoardJobPaginationMode>();
}

BOOST_AUTO_TEST_CASE( PsPaginationMode )
{
    using Mode = kiapi::board::jobs::BoardJobPaginationMode;

    BOOST_CHECK( ( ToProtoEnum<JOB_EXPORT_PCB_PS::GEN_MODE, Mode>( JOB_EXPORT_PCB_PS::GEN_MODE::SINGLE )
                   == Mode::BJPM_ALL_LAYERS_ONE_PAGE ) );
    BOOST_CHECK( ( ToProtoEnum<JOB_EXPORT_PCB_PS::GEN_MODE, Mode>( JOB_EXPORT_PCB_PS::GEN_MODE::MULTI )
                   == Mode::BJPM_EACH_LAYER_OWN_FILE ) );

    BOOST_CHECK( ( FromProtoEnum<JOB_EXPORT_PCB_PS::GEN_MODE, Mode>( Mode::BJPM_ALL_LAYERS_ONE_PAGE )
                   == JOB_EXPORT_PCB_PS::GEN_MODE::SINGLE ) );
    BOOST_CHECK( ( FromProtoEnum<JOB_EXPORT_PCB_PS::GEN_MODE, Mode>( Mode::BJPM_EACH_LAYER_OWN_FILE )
                   == JOB_EXPORT_PCB_PS::GEN_MODE::MULTI ) );
    BOOST_CHECK( ( FromProtoEnum<JOB_EXPORT_PCB_PS::GEN_MODE, Mode>( Mode::BJPM_EACH_LAYER_OWN_PAGE )
                   == JOB_EXPORT_PCB_PS::GEN_MODE::SINGLE ) );
}

BOOST_AUTO_TEST_CASE( DrillFormat )
{
    testEnums<JOB_EXPORT_PCB_DRILL::DRILL_FORMAT, kiapi::board::jobs::DrillFormat>();
}

BOOST_AUTO_TEST_CASE( DrillOrigin )
{
    testEnums<JOB_EXPORT_PCB_DRILL::DRILL_ORIGIN, kiapi::board::jobs::DrillOrigin>();
}

BOOST_AUTO_TEST_CASE( DrillZerosFormat )
{
    testEnums<JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT, kiapi::board::jobs::DrillZerosFormat>();
}

BOOST_AUTO_TEST_CASE( DrillMapFormat )
{
    testEnums<JOB_EXPORT_PCB_DRILL::MAP_FORMAT, kiapi::board::jobs::DrillMapFormat>();
}

BOOST_AUTO_TEST_CASE( DrillUnits )
{
    testEnums<JOB_EXPORT_PCB_DRILL::DRILL_UNITS, kiapi::common::types::Units>( true );
}

BOOST_AUTO_TEST_CASE( PositionSide )
{
    testEnums<JOB_EXPORT_PCB_POS::SIDE, kiapi::board::jobs::PositionSide>();
}

BOOST_AUTO_TEST_CASE( PositionUnits )
{
    testEnums<JOB_EXPORT_PCB_POS::UNITS, kiapi::common::types::Units>( true );
}

BOOST_AUTO_TEST_CASE( PositionFormat )
{
    testEnums<JOB_EXPORT_PCB_POS::FORMAT, kiapi::board::jobs::PositionFormat>();
}

BOOST_AUTO_TEST_CASE( Ipc2581Units )
{
    testEnums<JOB_EXPORT_PCB_IPC2581::IPC2581_UNITS, kiapi::common::types::Units>( true );
}

BOOST_AUTO_TEST_CASE( Ipc2581Version )
{
    testEnums<JOB_EXPORT_PCB_IPC2581::IPC2581_VERSION, kiapi::board::jobs::Ipc2581Version>();
}

BOOST_AUTO_TEST_CASE( OdbUnits )
{
    testEnums<JOB_EXPORT_PCB_ODB::ODB_UNITS, kiapi::common::types::Units>( true );
}

BOOST_AUTO_TEST_CASE( OdbCompression )
{
    testEnums<JOB_EXPORT_PCB_ODB::ODB_COMPRESSION, kiapi::board::jobs::OdbCompression>();
}

BOOST_AUTO_TEST_CASE( StatsOutputFormat )
{
    testEnums<JOB_EXPORT_PCB_STATS::OUTPUT_FORMAT, kiapi::board::jobs::StatsOutputFormat>();
}

BOOST_AUTO_TEST_CASE( StatsUnits )
{
    testEnums<JOB_EXPORT_PCB_STATS::UNITS, kiapi::common::types::Units>( true );
}

BOOST_AUTO_TEST_CASE( SchematicJobPageSize )
{
    testEnums<JOB_PAGE_SIZE, kiapi::schematic::jobs::SchematicJobPageSize>();
}

BOOST_AUTO_TEST_CASE( SchematicNetlistFormat )
{
    testEnums<JOB_EXPORT_SCH_NETLIST::FORMAT, kiapi::schematic::jobs::SchematicNetlistFormat>();
}

BOOST_AUTO_TEST_SUITE_END()
