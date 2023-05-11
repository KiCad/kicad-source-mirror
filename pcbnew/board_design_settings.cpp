/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <pcb_dimension.h>
#include <pcb_track.h>
#include <layer_ids.h>
#include <kiface_base.h>
#include <pad.h>
#include <board_design_settings.h>
#include <drc/drc_item.h>
#include <drc/drc_engine.h>
#include <settings/json_settings_internals.h>
#include <settings/parameters.h>
#include <project/project_file.h>
#include <advanced_config.h>
#include <pcbnew.h>

const int bdsSchemaVersion = 2;


BOARD_DESIGN_SETTINGS::BOARD_DESIGN_SETTINGS( JSON_SETTINGS* aParent, const std::string& aPath ) :
        NESTED_SETTINGS( "board_design_settings", bdsSchemaVersion, aParent, aPath )
{
    // We want to leave alone parameters that aren't found in the project JSON as they may be
    // initialized by the board file parser before NESTED_SETTINGS::LoadFromFile is called.
    m_resetParamsIfMissing = false;

    // Create a default NET_SETTINGS so that things don't break horribly if there's no project
    // loaded.  This also is used during file load for legacy boards that have netclasses stored
    // in the file.  After load, this information will be moved to the project and the pointer
    // updated.
    m_NetSettings = std::make_shared<NET_SETTINGS>( nullptr, "" );

    m_HasStackup = false;                   // no stackup defined by default

    m_Pad_Master = std::make_unique<PAD>( nullptr );

    LSET all_set = LSET().set();
    m_enabledLayers = all_set;              // All layers enabled at first.
                                            // SetCopperLayerCount() will adjust this.

    SetCopperLayerCount( 2 );               // Default design is a double sided board
    m_CurrentViaType = VIATYPE::THROUGH;

    // if true, when creating a new track starting on an existing track, use this track width
    m_UseConnectedTrackWidth = false;
    m_TempOverrideTrackWidth = false;

    // First is always the reference designator
    m_DefaultFPTextItems.emplace_back( wxT( "REF**" ), true, F_SilkS );
    // Second is always the value
    m_DefaultFPTextItems.emplace_back( wxT( "" ), true, F_Fab );
    // Any following ones are freebies
    m_DefaultFPTextItems.emplace_back( wxT( "${REFERENCE}" ), true, F_Fab );

    m_LineThickness[ LAYER_CLASS_SILK ] = pcbIUScale.mmToIU( DEFAULT_SILK_LINE_WIDTH );
    m_TextSize[ LAYER_CLASS_SILK ] = VECTOR2I( pcbIUScale.mmToIU( DEFAULT_SILK_TEXT_SIZE ),
                                               pcbIUScale.mmToIU( DEFAULT_SILK_TEXT_SIZE ) );
    m_TextThickness[ LAYER_CLASS_SILK ] = pcbIUScale.mmToIU( DEFAULT_SILK_TEXT_WIDTH );
    m_TextItalic[ LAYER_CLASS_SILK ] = false;
    m_TextUpright[ LAYER_CLASS_SILK ] = false;

    m_LineThickness[ LAYER_CLASS_COPPER ] = pcbIUScale.mmToIU( DEFAULT_COPPER_LINE_WIDTH );
    m_TextSize[ LAYER_CLASS_COPPER ] = VECTOR2I( pcbIUScale.mmToIU( DEFAULT_COPPER_TEXT_SIZE ),
                                                 pcbIUScale.mmToIU( DEFAULT_COPPER_TEXT_SIZE ) );
    m_TextThickness[ LAYER_CLASS_COPPER ] = pcbIUScale.mmToIU( DEFAULT_COPPER_TEXT_WIDTH );
    m_TextItalic[ LAYER_CLASS_COPPER ] = false;
    m_TextUpright[ LAYER_CLASS_COPPER ] = false;

    // Edges & Courtyards; text properties aren't used but better to have them holding
    // reasonable values than not.
    m_LineThickness[ LAYER_CLASS_EDGES ] = pcbIUScale.mmToIU( DEFAULT_EDGE_WIDTH );
    m_TextSize[ LAYER_CLASS_EDGES ] = VECTOR2I( pcbIUScale.mmToIU( DEFAULT_TEXT_SIZE ),
                                                pcbIUScale.mmToIU( DEFAULT_TEXT_SIZE ) );
    m_TextThickness[ LAYER_CLASS_EDGES ] = pcbIUScale.mmToIU( DEFAULT_TEXT_WIDTH );
    m_TextItalic[ LAYER_CLASS_EDGES ] = false;
    m_TextUpright[ LAYER_CLASS_EDGES ] = false;

    m_LineThickness[ LAYER_CLASS_COURTYARD ] = pcbIUScale.mmToIU( DEFAULT_COURTYARD_WIDTH );
    m_TextSize[ LAYER_CLASS_COURTYARD ] = VECTOR2I( pcbIUScale.mmToIU( DEFAULT_TEXT_SIZE ),
                                                    pcbIUScale.mmToIU( DEFAULT_TEXT_SIZE ) );
    m_TextThickness[ LAYER_CLASS_COURTYARD ] = pcbIUScale.mmToIU( DEFAULT_TEXT_WIDTH );
    m_TextItalic[ LAYER_CLASS_COURTYARD ] = false;
    m_TextUpright[ LAYER_CLASS_COURTYARD ] = false;

    m_LineThickness[ LAYER_CLASS_FAB ] = pcbIUScale.mmToIU( DEFAULT_LINE_WIDTH );
    m_TextSize[LAYER_CLASS_FAB] = VECTOR2I( pcbIUScale.mmToIU( DEFAULT_TEXT_SIZE ),
                                            pcbIUScale.mmToIU( DEFAULT_TEXT_SIZE ) );
    m_TextThickness[ LAYER_CLASS_FAB ] = pcbIUScale.mmToIU( DEFAULT_TEXT_WIDTH );
    m_TextItalic[ LAYER_CLASS_FAB ] = false;
    m_TextUpright[ LAYER_CLASS_FAB ] = false;

    m_LineThickness[ LAYER_CLASS_OTHERS ] = pcbIUScale.mmToIU( DEFAULT_LINE_WIDTH );
    m_TextSize[ LAYER_CLASS_OTHERS ] = VECTOR2I( pcbIUScale.mmToIU( DEFAULT_TEXT_SIZE ),
                                                 pcbIUScale.mmToIU( DEFAULT_TEXT_SIZE ) );
    m_TextThickness[ LAYER_CLASS_OTHERS ] = pcbIUScale.mmToIU( DEFAULT_TEXT_WIDTH );
    m_TextItalic[ LAYER_CLASS_OTHERS ] = false;
    m_TextUpright[ LAYER_CLASS_OTHERS ] = false;

    m_DimensionPrecision       = DIM_PRECISION::X_XXXX;
    m_DimensionUnitsMode       = DIM_UNITS_MODE::AUTOMATIC;
    m_DimensionUnitsFormat     = DIM_UNITS_FORMAT::BARE_SUFFIX;
    m_DimensionSuppressZeroes  = false;
    m_DimensionTextPosition    = DIM_TEXT_POSITION::OUTSIDE;
    m_DimensionKeepTextAligned = true;
    m_DimensionArrowLength     = pcbIUScale.MilsToIU( DEFAULT_DIMENSION_ARROW_LENGTH );
    m_DimensionExtensionOffset = pcbIUScale.mmToIU( DEFAULT_DIMENSION_EXTENSION_OFFSET );

    m_useCustomTrackVia = false;
    m_customTrackWidth  = pcbIUScale.mmToIU( DEFAULT_CUSTOMTRACKWIDTH );
    m_customViaSize.m_Diameter = pcbIUScale.mmToIU( DEFAULT_VIASMINSIZE );
    m_customViaSize.m_Drill = pcbIUScale.mmToIU( DEFAULT_MINTHROUGHDRILL );

    m_useCustomDiffPair = false;
    m_customDiffPair.m_Width = pcbIUScale.mmToIU( DEFAULT_CUSTOMDPAIRWIDTH );
    m_customDiffPair.m_Gap = pcbIUScale.mmToIU( DEFAULT_CUSTOMDPAIRGAP );
    m_customDiffPair.m_ViaGap = pcbIUScale.mmToIU( DEFAULT_CUSTOMDPAIRVIAGAP );

    m_MinClearance        = pcbIUScale.mmToIU( DEFAULT_MINCLEARANCE );
    m_MinConn             = pcbIUScale.mmToIU( DEFAULT_MINCONNECTION );
    m_TrackMinWidth       = pcbIUScale.mmToIU( DEFAULT_TRACKMINWIDTH );
    m_ViasMinAnnularWidth = pcbIUScale.mmToIU( DEFAULT_VIASMINSIZE - DEFAULT_MINTHROUGHDRILL ) / 2;
    m_ViasMinSize         = pcbIUScale.mmToIU( DEFAULT_VIASMINSIZE );
    m_MinThroughDrill     = pcbIUScale.mmToIU( DEFAULT_MINTHROUGHDRILL );
    m_MicroViasMinSize    = pcbIUScale.mmToIU( DEFAULT_MICROVIASMINSIZE );
    m_MicroViasMinDrill   = pcbIUScale.mmToIU( DEFAULT_MICROVIASMINDRILL );
    m_CopperEdgeClearance = pcbIUScale.mmToIU( DEFAULT_COPPEREDGECLEARANCE );
    m_HoleClearance       = pcbIUScale.mmToIU( DEFAULT_HOLECLEARANCE );
    m_HoleToHoleMin       = pcbIUScale.mmToIU( DEFAULT_HOLETOHOLEMIN );
    m_SilkClearance       = pcbIUScale.mmToIU( DEFAULT_SILKCLEARANCE );
    m_MinResolvedSpokes   = DEFAULT_MINRESOLVEDSPOKES;
    m_MinSilkTextHeight   = pcbIUScale.mmToIU( DEFAULT_SILK_TEXT_SIZE * 0.8 );
    m_MinSilkTextThickness= pcbIUScale.mmToIU( DEFAULT_SILK_TEXT_WIDTH * 0.8 );

    for( int errorCode = DRCE_FIRST; errorCode <= DRCE_LAST; ++errorCode )
        m_DRCSeverities[ errorCode ] = RPT_SEVERITY_ERROR;

    m_DRCSeverities[ DRCE_DRILLED_HOLES_COLOCATED ] = RPT_SEVERITY_WARNING;

    m_DRCSeverities[ DRCE_MISSING_COURTYARD ] = RPT_SEVERITY_IGNORE;
    m_DRCSeverities[ DRCE_PTH_IN_COURTYARD ] = RPT_SEVERITY_IGNORE;
    m_DRCSeverities[ DRCE_NPTH_IN_COURTYARD ] = RPT_SEVERITY_IGNORE;

    m_DRCSeverities[ DRCE_DANGLING_TRACK ] = RPT_SEVERITY_WARNING;
    m_DRCSeverities[ DRCE_DANGLING_VIA ] = RPT_SEVERITY_WARNING;

    m_DRCSeverities[ DRCE_COPPER_SLIVER ] = RPT_SEVERITY_WARNING;
    m_DRCSeverities[ DRCE_ISOLATED_COPPER ] = RPT_SEVERITY_WARNING;

    m_DRCSeverities[ DRCE_PADSTACK ] = RPT_SEVERITY_WARNING;

    m_DRCSeverities[ DRCE_MISSING_FOOTPRINT ] = RPT_SEVERITY_WARNING;
    m_DRCSeverities[ DRCE_DUPLICATE_FOOTPRINT ] = RPT_SEVERITY_WARNING;
    m_DRCSeverities[ DRCE_EXTRA_FOOTPRINT ] = RPT_SEVERITY_WARNING;
    m_DRCSeverities[ DRCE_NET_CONFLICT ] = RPT_SEVERITY_WARNING;

    m_DRCSeverities[ DRCE_OVERLAPPING_SILK ] = RPT_SEVERITY_WARNING;
    m_DRCSeverities[ DRCE_SILK_CLEARANCE ] = RPT_SEVERITY_WARNING;
    m_DRCSeverities[ DRCE_SILK_EDGE_CLEARANCE ] = RPT_SEVERITY_WARNING;
    m_DRCSeverities[ DRCE_TEXT_HEIGHT ] = RPT_SEVERITY_WARNING;
    m_DRCSeverities[ DRCE_TEXT_THICKNESS ] = RPT_SEVERITY_WARNING;

    m_DRCSeverities[ DRCE_FOOTPRINT_TYPE_MISMATCH ] = RPT_SEVERITY_IGNORE;

    m_DRCSeverities[ DRCE_LIB_FOOTPRINT_ISSUES ] = RPT_SEVERITY_WARNING;
    m_DRCSeverities[ DRCE_LIB_FOOTPRINT_MISMATCH ] = RPT_SEVERITY_WARNING;

    m_DRCSeverities[ DRCE_CONNECTION_WIDTH ] = RPT_SEVERITY_WARNING;

    m_MaxError = ARC_HIGH_DEF;
    m_ZoneKeepExternalFillets = false;
    m_UseHeightForLengthCalcs = true;

    // Global mask margins:
    m_SolderMaskExpansion = pcbIUScale.mmToIU( DEFAULT_SOLDERMASK_EXPANSION );
    m_SolderMaskMinWidth = pcbIUScale.mmToIU( DEFAULT_SOLDERMASK_MIN_WIDTH );
    m_SolderMaskToCopperClearance = pcbIUScale.mmToIU( DEFAULT_SOLDERMASK_TO_COPPER_CLEARANCE );

    // Solder paste margin absolute value
    m_SolderPasteMargin = pcbIUScale.mmToIU( DEFAULT_SOLDERPASTE_CLEARANCE );
    // Solder paste margin as a ratio of pad size
    // The final margin is the sum of these 2 values
    // Usually < 0 because the mask is smaller than pad
    m_SolderPasteMarginRatio = DEFAULT_SOLDERPASTE_RATIO;

    m_AllowSoldermaskBridgesInFPs = false;

    // Layer thickness for 3D viewer
    m_boardThickness = pcbIUScale.mmToIU( DEFAULT_BOARD_THICKNESS_MM );

    m_viaSizeIndex = 0;
    m_trackWidthIndex = 0;
    m_diffPairIndex = 0;

    // Parameters stored in JSON in the project file

    // NOTE: Previously, BOARD_DESIGN_SETTINGS stored the basic board layer information (layer
    // names and enable/disable state) in the project file even though this information is also
    // stored in the board file.  This was implemented for importing these settings from another
    // project.  Going forward, the import feature will just import from other board files (since
    // we could have multi-board projects in the future anyway) so this functionality is dropped.


    m_params.emplace_back( new PARAM<bool>( "rules.use_height_for_length_calcs",
            &m_UseHeightForLengthCalcs, true ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "rules.min_clearance",
            &m_MinClearance, pcbIUScale.mmToIU( DEFAULT_MINCLEARANCE ),
            pcbIUScale.mmToIU( 0.00 ), pcbIUScale.mmToIU( 25.0 ), pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "rules.min_connection",
            &m_MinConn, pcbIUScale.mmToIU( DEFAULT_MINCONNECTION ),
            pcbIUScale.mmToIU( 0.00 ), pcbIUScale.mmToIU( 100.0 ), pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "rules.min_track_width",
            &m_TrackMinWidth, pcbIUScale.mmToIU( DEFAULT_TRACKMINWIDTH ),
            pcbIUScale.mmToIU( 0.00 ), pcbIUScale.mmToIU( 25.0 ), pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "rules.min_via_annular_width",
            &m_ViasMinAnnularWidth, pcbIUScale.mmToIU( DEFAULT_VIASMINSIZE ),
            pcbIUScale.mmToIU( 0.00 ), pcbIUScale.mmToIU( 25.0 ), pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "rules.min_via_diameter",
            &m_ViasMinSize, pcbIUScale.mmToIU( DEFAULT_VIASMINSIZE ),
            pcbIUScale.mmToIU( 0.00 ), pcbIUScale.mmToIU( 25.0 ), pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "rules.min_through_hole_diameter",
            &m_MinThroughDrill, pcbIUScale.mmToIU( DEFAULT_MINTHROUGHDRILL ),
            pcbIUScale.mmToIU( 0.00 ), pcbIUScale.mmToIU( 25.0 ), pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "rules.min_microvia_diameter",
            &m_MicroViasMinSize, pcbIUScale.mmToIU( DEFAULT_MICROVIASMINSIZE ),
            pcbIUScale.mmToIU( 0.00 ), pcbIUScale.mmToIU( 10.0 ), pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "rules.min_microvia_drill",
            &m_MicroViasMinDrill, pcbIUScale.mmToIU( DEFAULT_MICROVIASMINDRILL ),
            pcbIUScale.mmToIU( 0.00 ), pcbIUScale.mmToIU( 10.0 ), pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "rules.min_hole_to_hole",
            &m_HoleToHoleMin, pcbIUScale.mmToIU( DEFAULT_HOLETOHOLEMIN ),
            pcbIUScale.mmToIU( 0.00 ), pcbIUScale.mmToIU( 10.0 ), pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "rules.min_hole_clearance",
            &m_HoleClearance, pcbIUScale.mmToIU( DEFAULT_HOLECLEARANCE ),
            pcbIUScale.mmToIU( 0.00 ), pcbIUScale.mmToIU( 100.0 ), pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "rules.min_silk_clearance",
            &m_SilkClearance, pcbIUScale.mmToIU( DEFAULT_SILKCLEARANCE ),
            pcbIUScale.mmToIU( 0.00 ), pcbIUScale.mmToIU( 100.0 ), pcbIUScale.MM_PER_IU ) );

    // While the maximum *effective* value is 4, we've had users interpret this as the count on
    // all layers, and enter something like 10.  They'll figure it out soon enough *unless* we
    // enforce a max of 4 (and therefore reset it back to the default of 2), at which point it
    // just looks buggy.
    m_params.emplace_back( new PARAM<int>( "rules.min_resolved_spokes",
            &m_MinResolvedSpokes, DEFAULT_MINRESOLVEDSPOKES, 0, 99 ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "rules.min_text_height",
            &m_MinSilkTextHeight, pcbIUScale.mmToIU( DEFAULT_SILK_TEXT_SIZE * 0.8 ),
            pcbIUScale.mmToIU( 0.00 ), pcbIUScale.mmToIU( 100.0 ), pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "rules.min_text_thickness",
            &m_MinSilkTextThickness, pcbIUScale.mmToIU( DEFAULT_SILK_TEXT_WIDTH * 0.8 ),
            pcbIUScale.mmToIU( 0.00 ), pcbIUScale.mmToIU( 25.0 ), pcbIUScale.MM_PER_IU ) );

    // Note: a clearance of -0.01 is a flag indicating we should use the legacy (pre-6.0) method
    // based on the edge cut thicknesses.
    m_params.emplace_back( new PARAM_SCALED<int>( "rules.min_copper_edge_clearance",
            &m_CopperEdgeClearance, pcbIUScale.mmToIU( LEGACY_COPPEREDGECLEARANCE ),
            pcbIUScale.mmToIU( -0.01 ), pcbIUScale.mmToIU( 25.0 ), pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "rule_severities",
            [&]() -> nlohmann::json
            {
                nlohmann::json ret = {};

                for( const RC_ITEM& item : DRC_ITEM::GetItemsWithSeverities() )
                {
                    wxString name = item.GetSettingsKey();
                    int      code = item.GetErrorCode();

                    if( name.IsEmpty() || m_DRCSeverities.count( code ) == 0 )
                        continue;

                    ret[std::string( name.ToUTF8() )] = SeverityToString( m_DRCSeverities[code] );
                }

                return ret;
            },
            [&]( const nlohmann::json& aJson )
            {
                if( !aJson.is_object() )
                    return;

                for( const RC_ITEM& item : DRC_ITEM::GetItemsWithSeverities() )
                {
                    wxString name = item.GetSettingsKey();
                    std::string key( name.ToUTF8() );

                    if( aJson.contains( key ) )
                        m_DRCSeverities[item.GetErrorCode()] = SeverityFromString( aJson[key] );
                }
            }, {} ) );

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "drc_exclusions",
            [&]() -> nlohmann::json
            {
                nlohmann::json js = nlohmann::json::array();

                for( const auto& entry : m_DrcExclusions )
                    js.push_back( entry );

                return js;
            },
            [&]( const nlohmann::json& aObj )
            {
                m_DrcExclusions.clear();

                if( !aObj.is_array() )
                    return;

                for( const nlohmann::json& entry : aObj )
                {
                    if( entry.empty() )
                        continue;

                    m_DrcExclusions.insert( entry.get<wxString>() );
                }
            },
            {} ) );

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "track_widths",
            [&]() -> nlohmann::json
            {
                nlohmann::json js = nlohmann::json::array();

                for( const int& width : m_TrackWidthList )
                    js.push_back( pcbIUScale.IUTomm( width ) );

                return js;
            },
            [&]( const nlohmann::json& aJson )
            {
                if( !aJson.is_array() )
                    return;

                m_TrackWidthList.clear();

                for( const nlohmann::json& entry : aJson )
                {
                    if( entry.empty() )
                        continue;

                    m_TrackWidthList.emplace_back( pcbIUScale.mmToIU( entry.get<double>() ) );
                }
            },
            {} ) );

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "via_dimensions",
            [&]() -> nlohmann::json
            {
                nlohmann::json js = nlohmann::json::array();

                for( const auto& via : m_ViasDimensionsList )
                {
                    nlohmann::json entry = {};

                    entry["diameter"] = pcbIUScale.IUTomm( via.m_Diameter );
                    entry["drill"]    = pcbIUScale.IUTomm( via.m_Drill );

                    js.push_back( entry );
                }

                return js;
            },
            [&]( const nlohmann::json& aObj )
            {
                if( !aObj.is_array() )
                    return;

                m_ViasDimensionsList.clear();

                for( const nlohmann::json& entry : aObj )
                {
                    if( entry.empty() || !entry.is_object() )
                        continue;

                    if( !entry.contains( "diameter" ) || !entry.contains( "drill" ) )
                        continue;

                    int diameter = pcbIUScale.mmToIU( entry["diameter"].get<double>() );
                    int drill    = pcbIUScale.mmToIU( entry["drill"].get<double>() );

                    m_ViasDimensionsList.emplace_back( VIA_DIMENSION( diameter, drill ) );
                }
            },
            {} ) );

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "diff_pair_dimensions",
            [&]() -> nlohmann::json
            {
                nlohmann::json js = nlohmann::json::array();

                for( const auto& pair : m_DiffPairDimensionsList )
                {
                    nlohmann::json entry = {};

                    entry["width"]   = pcbIUScale.IUTomm( pair.m_Width );
                    entry["gap"]     = pcbIUScale.IUTomm( pair.m_Gap );
                    entry["via_gap"] = pcbIUScale.IUTomm( pair.m_ViaGap );

                    js.push_back( entry );
                }

                return js;
            },
            [&]( const nlohmann::json& aObj )
            {
                if( !aObj.is_array() )
                    return;

                m_DiffPairDimensionsList.clear();

                for( const nlohmann::json& entry : aObj )
                {
                    if( entry.empty() || !entry.is_object() )
                        continue;

                    if( !entry.contains( "width" ) || !entry.contains( "gap" )
                            || !entry.contains( "via_gap" ) )
                        continue;

                    int width   = pcbIUScale.mmToIU( entry["width"].get<double>() );
                    int gap     = pcbIUScale.mmToIU( entry["gap"].get<double>() );
                    int via_gap = pcbIUScale.mmToIU( entry["via_gap"].get<double>() );

                    m_DiffPairDimensionsList.emplace_back(
                            DIFF_PAIR_DIMENSION( width, gap, via_gap ) );
                }
            },
            {} ) );

    // Handle options for teardrops (targets and some others):
    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "teardrop_options",
            [&]() -> nlohmann::json
            {
                nlohmann::json js = nlohmann::json::array();
                nlohmann::json entry = {};

                entry["td_onviapad"]  = m_TeardropParamsList.m_TargetViasPads;
                entry["td_onpadsmd"]  = m_TeardropParamsList.m_TargetPadsWithNoHole;
                entry["td_ontrackend"]  = m_TeardropParamsList.m_TargetTrack2Track;
                entry["td_onroundshapesonly"]  = m_TeardropParamsList.m_UseRoundShapesOnly;
                entry["td_allow_use_two_tracks"] = m_TeardropParamsList.m_AllowUseTwoTracks;
                entry["td_curve_segcount"]  = m_TeardropParamsList.m_CurveSegCount;
                entry["td_on_pad_in_zone"]  = m_TeardropParamsList.m_TdOnPadsInZones;

                js.push_back( entry );

                return js;
            },
            [&]( const nlohmann::json& aObj )
            {
                if( !aObj.is_array() )
                    return;

                for( const nlohmann::json& entry : aObj )
                {
                    if( entry.empty() || !entry.is_object() )
                        continue;

                    if( entry.contains( "td_onviapad" ) )
                        m_TeardropParamsList.m_TargetViasPads = entry["td_onviapad"].get<bool>();

                    if( entry.contains( "td_onpadsmd" ) )
                        m_TeardropParamsList.m_TargetPadsWithNoHole = entry["td_onpadsmd"].get<bool>();

                    if( entry.contains( "td_ontrackend" ) )
                        m_TeardropParamsList.m_TargetTrack2Track = entry["td_ontrackend"].get<bool>();

                    if( entry.contains( "td_onroundshapesonly" ) )
                        m_TeardropParamsList.m_UseRoundShapesOnly = entry["td_onroundshapesonly"].get<bool>();

                    if( entry.contains( "td_allow_use_two_tracks" ) )
                        m_TeardropParamsList.m_AllowUseTwoTracks = entry["td_allow_use_two_tracks"].get<bool>();

                    if( entry.contains( "td_curve_segcount" ) )
                        m_TeardropParamsList.m_CurveSegCount = entry["td_curve_segcount"].get<int>();

                    if( entry.contains( "td_on_pad_in_zone" ) )
                        m_TeardropParamsList.m_TdOnPadsInZones = entry["td_on_pad_in_zone"].get<bool>();
                }
            },
            {} ) );

    // Handle parameters (sizes, shape) for each type of teardrop:
    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "teardrop_parameters",
            [&]() -> nlohmann::json
            {
                nlohmann::json js = nlohmann::json::array();

                for( size_t ii = 0; ii < m_TeardropParamsList.GetParametersCount(); ii++ )
                {
                    nlohmann::json entry = {};
                    TEARDROP_PARAMETERS* td_prm = m_TeardropParamsList.GetParameters( (TARGET_TD)ii );

                    entry["td_target_name"]  = GetTeardropTargetCanonicalName( (TARGET_TD)ii );
                    entry["td_maxlen"]  = pcbIUScale.IUTomm( td_prm->m_TdMaxLen );
                    entry["td_maxheight"]  = pcbIUScale.IUTomm( td_prm->m_TdMaxHeight );
                    entry["td_length_ratio"]  = td_prm->m_LengthRatio;
                    entry["td_height_ratio"]  = td_prm->m_HeightRatio;
                    entry["td_curve_segcount"]  = td_prm->m_CurveSegCount;
                    entry["td_width_to_size_filter_ratio"] = td_prm->m_WidthtoSizeFilterRatio;

                    js.push_back( entry );
                }

                return js;
            },
            [&]( const nlohmann::json& aObj )
            {
                if( !aObj.is_array() )
                    return;

                for( const nlohmann::json& entry : aObj )
                {
                    if( entry.empty() || !entry.is_object() )
                        continue;

                    if( !entry.contains( "td_target_name" ) )
                        continue;

                    int idx = GetTeardropTargetTypeFromCanonicalName( entry["td_target_name"].get<std::string>() );

                    if( idx >= 0 && idx < 3 )
                    {
                        TEARDROP_PARAMETERS* td_prm = m_TeardropParamsList.GetParameters( (TARGET_TD)idx );

                        if( entry.contains( "td_maxlen" ) )
                            td_prm->m_TdMaxLen = pcbIUScale.mmToIU( entry["td_maxlen"].get<double>() );

                        if( entry.contains( "td_maxheight" ) )
                            td_prm->m_TdMaxHeight = pcbIUScale.mmToIU( entry["td_maxheight"].get<double>() );

                        if( entry.contains( "td_length_ratio" ) )
                            td_prm->m_LengthRatio = entry["td_length_ratio"].get<double>();

                        if( entry.contains( "td_height_ratio" ) )
                            td_prm->m_HeightRatio = entry["td_height_ratio"].get<double>();

                        if( entry.contains( "td_curve_segcount" ) )
                            td_prm->m_CurveSegCount = entry["td_curve_segcount"].get<int>();

                        if( entry.contains( "td_width_to_size_filter_ratio" ) )
                            td_prm->m_WidthtoSizeFilterRatio = entry["td_width_to_size_filter_ratio"].get<double>();
                    }
                }
            },
            {} ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.silk_line_width",
            &m_LineThickness[LAYER_CLASS_SILK], pcbIUScale.mmToIU( DEFAULT_SILK_LINE_WIDTH ),
            pcbIUScale.mmToIU( 0.01 ), pcbIUScale.mmToIU( 5.0 ), pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.silk_text_size_v",
            &m_TextSize[LAYER_CLASS_SILK].y, pcbIUScale.mmToIU( DEFAULT_SILK_TEXT_SIZE ),
            TEXTS_MIN_SIZE, TEXTS_MAX_SIZE, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.silk_text_size_h",
            &m_TextSize[LAYER_CLASS_SILK].x, pcbIUScale.mmToIU( DEFAULT_SILK_TEXT_SIZE ),
            TEXTS_MIN_SIZE, TEXTS_MAX_SIZE, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.silk_text_thickness",
            &m_TextThickness[LAYER_CLASS_SILK], pcbIUScale.mmToIU( DEFAULT_SILK_TEXT_WIDTH ), 1,
            TEXTS_MAX_WIDTH, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM<bool>( "defaults.silk_text_italic",
            &m_TextItalic[LAYER_CLASS_SILK], false ) );

    m_params.emplace_back( new PARAM<bool>( "defaults.silk_text_upright",
            &m_TextUpright[ LAYER_CLASS_SILK ], true ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.copper_line_width",
            &m_LineThickness[LAYER_CLASS_COPPER], pcbIUScale.mmToIU( DEFAULT_COPPER_LINE_WIDTH ),
            pcbIUScale.mmToIU( 0.01 ), pcbIUScale.mmToIU( 5.0 ), pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.copper_text_size_v",
            &m_TextSize[LAYER_CLASS_COPPER].y, pcbIUScale.mmToIU( DEFAULT_COPPER_TEXT_SIZE ),
            TEXTS_MIN_SIZE, TEXTS_MAX_SIZE, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.copper_text_size_h",
            &m_TextSize[LAYER_CLASS_COPPER].x, pcbIUScale.mmToIU( DEFAULT_COPPER_TEXT_SIZE ),
            TEXTS_MIN_SIZE, TEXTS_MAX_SIZE, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.copper_text_thickness",
            &m_TextThickness[LAYER_CLASS_COPPER], pcbIUScale.mmToIU( DEFAULT_COPPER_TEXT_WIDTH ),
            pcbIUScale.mmToIU( 0.01 ), pcbIUScale.mmToIU( 5.0 ), pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM<bool>( "defaults.copper_text_italic",
            &m_TextItalic[LAYER_CLASS_COPPER], false ) );

    m_params.emplace_back( new PARAM<bool>( "defaults.copper_text_upright",
            &m_TextUpright[LAYER_CLASS_COPPER], true ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.board_outline_line_width",
            &m_LineThickness[LAYER_CLASS_EDGES], pcbIUScale.mmToIU( DEFAULT_EDGE_WIDTH ),
            pcbIUScale.mmToIU( 0.01 ), pcbIUScale.mmToIU( 5.0 ), pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.courtyard_line_width",
            &m_LineThickness[LAYER_CLASS_COURTYARD], pcbIUScale.mmToIU( DEFAULT_COURTYARD_WIDTH ),
            pcbIUScale.mmToIU( 0.01 ), pcbIUScale.mmToIU( 5.0 ), pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.fab_line_width",
            &m_LineThickness[LAYER_CLASS_FAB], pcbIUScale.mmToIU( DEFAULT_LINE_WIDTH ),
            pcbIUScale.mmToIU( 0.01 ), pcbIUScale.mmToIU( 5.0 ), pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.fab_text_size_v",
            &m_TextSize[LAYER_CLASS_FAB].y, pcbIUScale.mmToIU( DEFAULT_TEXT_SIZE ),
            TEXTS_MIN_SIZE, TEXTS_MAX_SIZE, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.fab_text_size_h",
            &m_TextSize[LAYER_CLASS_FAB].x, pcbIUScale.mmToIU( DEFAULT_TEXT_SIZE ),
            TEXTS_MIN_SIZE, TEXTS_MAX_SIZE, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.fab_text_thickness",
            &m_TextThickness[LAYER_CLASS_FAB], pcbIUScale.mmToIU( DEFAULT_TEXT_WIDTH ),
            pcbIUScale.mmToIU( 0.01 ), pcbIUScale.mmToIU( 5.0 ), pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM<bool>( "defaults.fab_text_italic",
            &m_TextItalic[LAYER_CLASS_FAB], false ) );

    m_params.emplace_back( new PARAM<bool>( "defaults.fab_text_upright",
            &m_TextUpright[LAYER_CLASS_FAB], true ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.other_line_width",
            &m_LineThickness[LAYER_CLASS_OTHERS], pcbIUScale.mmToIU( DEFAULT_LINE_WIDTH ),
            pcbIUScale.mmToIU( 0.01 ), pcbIUScale.mmToIU( 5.0 ), pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.other_text_size_v",
            &m_TextSize[LAYER_CLASS_OTHERS].y, pcbIUScale.mmToIU( DEFAULT_TEXT_SIZE ), TEXTS_MIN_SIZE,
            TEXTS_MAX_SIZE, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.other_text_size_h",
            &m_TextSize[LAYER_CLASS_OTHERS].x, pcbIUScale.mmToIU( DEFAULT_TEXT_SIZE ), TEXTS_MIN_SIZE,
            TEXTS_MAX_SIZE, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.other_text_thickness",
            &m_TextThickness[LAYER_CLASS_OTHERS], pcbIUScale.mmToIU( DEFAULT_TEXT_WIDTH ),
            pcbIUScale.mmToIU( 0.01 ), pcbIUScale.mmToIU( 5.0 ), pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM<bool>( "defaults.other_text_italic",
            &m_TextItalic[LAYER_CLASS_OTHERS], false ) );

    m_params.emplace_back( new PARAM<bool>( "defaults.other_text_upright",
            &m_TextUpright[LAYER_CLASS_OTHERS], true ) );

    m_params.emplace_back( new PARAM_ENUM<DIM_UNITS_MODE>( "defaults.dimension_units",
            &m_DimensionUnitsMode, DIM_UNITS_MODE::AUTOMATIC, DIM_UNITS_MODE::INCHES,
            DIM_UNITS_MODE::AUTOMATIC ) );

    m_params.emplace_back( new PARAM_ENUM<DIM_PRECISION>( "defaults.dimension_precision",
            &m_DimensionPrecision, DIM_PRECISION::X_XXXX, DIM_PRECISION::X, DIM_PRECISION::V_VVVVV ) );

    m_params.emplace_back( new PARAM_ENUM<DIM_UNITS_FORMAT>( "defaults.dimensions.units_format",
            &m_DimensionUnitsFormat, DIM_UNITS_FORMAT::BARE_SUFFIX, DIM_UNITS_FORMAT::NO_SUFFIX,
            DIM_UNITS_FORMAT::PAREN_SUFFIX ) );

    m_params.emplace_back( new PARAM<bool>( "defaults.dimensions.suppress_zeroes",
            &m_DimensionSuppressZeroes, false ) );

    // NOTE: excluding DIM_TEXT_POSITION::MANUAL from the valid range here
    m_params.emplace_back( new PARAM_ENUM<DIM_TEXT_POSITION>( "defaults.dimensions.text_position",
            &m_DimensionTextPosition, DIM_TEXT_POSITION::OUTSIDE, DIM_TEXT_POSITION::OUTSIDE,
            DIM_TEXT_POSITION::INLINE ) );

    m_params.emplace_back( new PARAM<bool>( "defaults.dimensions.keep_text_aligned",
            &m_DimensionKeepTextAligned, true ) );

    m_params.emplace_back( new PARAM<int>( "defaults.dimensions.arrow_length",
            &m_DimensionArrowLength,
            pcbIUScale.MilsToIU( DEFAULT_DIMENSION_ARROW_LENGTH ) ) );

    m_params.emplace_back( new PARAM<int>( "defaults.dimensions.extension_offset",
            &m_DimensionExtensionOffset,
            pcbIUScale.mmToIU( DEFAULT_DIMENSION_EXTENSION_OFFSET ) ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.zones.min_clearance",
            &m_defaultZoneSettings.m_ZoneClearance, pcbIUScale.mmToIU( ZONE_CLEARANCE_MM ),
            pcbIUScale.mmToIU( 0.0 ), pcbIUScale.mmToIU( 25.0 ), pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "defaults.pads",
            [&]() -> nlohmann::json
            {
                nlohmann::json ret =
                        {
                            { "width",  pcbIUScale.IUTomm( m_Pad_Master->GetSize().x ) },
                            { "height", pcbIUScale.IUTomm( m_Pad_Master->GetSize().y ) },
                            { "drill",  pcbIUScale.IUTomm( m_Pad_Master->GetDrillSize().x ) }
                        };

                return ret;
            },
            [&]( const nlohmann::json& aJson )
            {
                if( aJson.contains( "width" ) && aJson.contains( "height" )
                        && aJson.contains( "drill" ) )
                {
                    VECTOR2I sz;
                    sz.x = pcbIUScale.mmToIU( aJson["width"].get<double>() );
                    sz.y = pcbIUScale.mmToIU( aJson["height"].get<double>() );

                    m_Pad_Master->SetSize( sz );

                    int drill = pcbIUScale.mmToIU( aJson["drill"].get<double>() );

                    m_Pad_Master->SetDrillSize( VECTOR2I( drill, drill ) );
                }
            }, {} ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "rules.max_error",
            &m_MaxError, ARC_HIGH_DEF, pcbIUScale.mmToIU( 0.0001 ), pcbIUScale.mmToIU( 1.0 ), pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "rules.solder_mask_to_copper_clearance",
            &m_SolderMaskToCopperClearance, pcbIUScale.mmToIU( DEFAULT_SOLDERMASK_TO_COPPER_CLEARANCE ),
            pcbIUScale.mmToIU( 0.0 ), pcbIUScale.mmToIU( 25.0 ), pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM<bool>( "zones_allow_external_fillets",
            &m_ZoneKeepExternalFillets, false ) );

    registerMigration( 0, 1, std::bind( &BOARD_DESIGN_SETTINGS::migrateSchema0to1, this ) );

    registerMigration( 1, 2,
            [&]() -> bool
            {
                // Schema 1 to 2: move mask and paste margin settings back to board.
                // The parameters are removed, so we just have to manually load them here and
                // they will get saved with the board
                if( std::optional<double> optval = Get<double>( "rules.solder_mask_clearance" ) )
                    m_SolderMaskExpansion = static_cast<int>( *optval * pcbIUScale.IU_PER_MM );

                if( std::optional<double> optval = Get<double>( "rules.solder_mask_min_width" ) )
                    m_SolderMaskMinWidth = static_cast<int>( *optval * pcbIUScale.IU_PER_MM );

                if( std::optional<double> optval = Get<double>( "rules.solder_paste_clearance" ) )
                    m_SolderPasteMargin = static_cast<int>( *optval * pcbIUScale.IU_PER_MM );

                if( std::optional<double> optval = Get<double>( "rules.solder_paste_margin_ratio" ) )
                    m_SolderPasteMarginRatio = *optval;

                try
                {
                    At( "rules" ).erase( "solder_mask_clearance" );
                    At( "rules" ).erase( "solder_mask_min_width" );
                    At( "rules" ).erase( "solder_paste_clearance" );
                    At( "rules" ).erase( "solder_paste_margin_ratio" );
                }
                catch( ... )
                {}

                return true;
            } );
}


BOARD_DESIGN_SETTINGS::~BOARD_DESIGN_SETTINGS()
{
    if( m_parent )
    {
        m_parent->ReleaseNestedSettings( this );
        m_parent = nullptr;
    }
}


BOARD_DESIGN_SETTINGS::BOARD_DESIGN_SETTINGS( const BOARD_DESIGN_SETTINGS& aOther ) :
        NESTED_SETTINGS( "board_design_settings", bdsSchemaVersion, aOther.m_parent,
                         aOther.m_path ),
        m_Pad_Master( nullptr )
{
    initFromOther( aOther );
}


BOARD_DESIGN_SETTINGS& BOARD_DESIGN_SETTINGS::operator=( const BOARD_DESIGN_SETTINGS& aOther )
{
    initFromOther( aOther );
    return *this;
}


void BOARD_DESIGN_SETTINGS::initFromOther( const BOARD_DESIGN_SETTINGS& aOther )
{
    // Copy of NESTED_SETTINGS around is not allowed, so let's just update the params.
    m_TrackWidthList         = aOther.m_TrackWidthList;
    m_ViasDimensionsList     = aOther.m_ViasDimensionsList;
    m_DiffPairDimensionsList = aOther.m_DiffPairDimensionsList;
    m_CurrentViaType         = aOther.m_CurrentViaType;
    m_UseConnectedTrackWidth = aOther.m_UseConnectedTrackWidth;
    m_MinClearance           = aOther.m_MinClearance;
    m_MinConn                = aOther.m_MinConn;
    m_TrackMinWidth          = aOther.m_TrackMinWidth;
    m_ViasMinAnnularWidth    = aOther.m_ViasMinAnnularWidth;
    m_ViasMinSize            = aOther.m_ViasMinSize;
    m_MinThroughDrill        = aOther.m_MinThroughDrill;
    m_MicroViasMinSize       = aOther.m_MicroViasMinSize;
    m_MicroViasMinDrill      = aOther.m_MicroViasMinDrill;
    m_CopperEdgeClearance    = aOther.m_CopperEdgeClearance;
    m_HoleClearance          = aOther.m_HoleClearance;
    m_HoleToHoleMin          = aOther.m_HoleToHoleMin;
    m_SilkClearance          = aOther.m_SilkClearance;
    m_MinSilkTextHeight      = aOther.m_MinSilkTextHeight;
    m_MinSilkTextThickness   = aOther.m_MinSilkTextThickness;
    m_DRCSeverities          = aOther.m_DRCSeverities;
    m_DrcExclusions          = aOther.m_DrcExclusions;
    m_ZoneKeepExternalFillets     = aOther.m_ZoneKeepExternalFillets;
    m_MaxError                    = aOther.m_MaxError;
    m_SolderMaskExpansion         = aOther.m_SolderMaskExpansion;
    m_SolderMaskMinWidth          = aOther.m_SolderMaskMinWidth;
    m_SolderMaskToCopperClearance = aOther.m_SolderMaskToCopperClearance;
    m_SolderPasteMargin           = aOther.m_SolderPasteMargin;
    m_SolderPasteMarginRatio      = aOther.m_SolderPasteMarginRatio;
    m_DefaultFPTextItems          = aOther.m_DefaultFPTextItems;

    std::copy( std::begin( aOther.m_LineThickness ), std::end( aOther.m_LineThickness ),
               std::begin( m_LineThickness ) );

    std::copy( std::begin( aOther.m_TextSize ), std::end( aOther.m_TextSize ),
               std::begin( m_TextSize ) );

    std::copy( std::begin( aOther.m_TextThickness ), std::end( aOther.m_TextThickness ),
               std::begin( m_TextThickness ) );

    std::copy( std::begin( aOther.m_TextItalic ), std::end( aOther.m_TextItalic ),
               std::begin( m_TextItalic ) );

    std::copy( std::begin( aOther.m_TextUpright ), std::end( aOther.m_TextUpright ),
               std::begin( m_TextUpright ) );

    m_DimensionUnitsMode       = aOther.m_DimensionUnitsMode;
    m_DimensionPrecision       = aOther.m_DimensionPrecision;
    m_DimensionUnitsFormat     = aOther.m_DimensionUnitsFormat;
    m_DimensionSuppressZeroes  = aOther.m_DimensionSuppressZeroes;
    m_DimensionTextPosition    = aOther.m_DimensionTextPosition;
    m_DimensionKeepTextAligned = aOther.m_DimensionKeepTextAligned;
    m_DimensionArrowLength     = aOther.m_DimensionArrowLength;
    m_DimensionExtensionOffset = aOther.m_DimensionExtensionOffset;

    m_auxOrigin              = aOther.m_auxOrigin;
    m_gridOrigin             = aOther.m_gridOrigin;
    m_HasStackup             = aOther.m_HasStackup;
    m_UseHeightForLengthCalcs= aOther.m_UseHeightForLengthCalcs;

    m_trackWidthIndex        = aOther.m_trackWidthIndex;
    m_viaSizeIndex           = aOther.m_viaSizeIndex;
    m_diffPairIndex          = aOther.m_diffPairIndex;
    m_useCustomTrackVia      = aOther.m_useCustomTrackVia;
    m_customTrackWidth       = aOther.m_customTrackWidth;
    m_customViaSize          = aOther.m_customViaSize;
    m_useCustomDiffPair      = aOther.m_useCustomDiffPair;
    m_customDiffPair         = aOther.m_customDiffPair;
    m_copperLayerCount       = aOther.m_copperLayerCount;
    m_enabledLayers          = aOther.m_enabledLayers;
    m_boardThickness         = aOther.m_boardThickness;
    m_currentNetClassName    = aOther.m_currentNetClassName;
    m_stackup                = aOther.m_stackup;
    m_NetSettings            = aOther.m_NetSettings;
    m_Pad_Master             = std::make_unique<PAD>( *aOther.m_Pad_Master );
    m_defaultZoneSettings    = aOther.m_defaultZoneSettings;
}


bool BOARD_DESIGN_SETTINGS::migrateSchema0to1()
{
    /**
     * Schema 0 to 1: default dimension precision changed in meaning.
     * Previously it was an enum with the following meaning:
     *
     * 0: 0.01mm / 1 mil / 0.001 in
     * 1: 0.001mm / 0.1 mil / 0.0001 in
     * 2: 0.0001mm / 0.01 mil / 0.00001 in
     *
     * Now it is independent of display units and is an integer meaning the number of digits
     * displayed after the decimal point, so we have to migrate based on the default units.
     *
     * The units is an integer with the following mapping:
     *
     * 0: Inches
     * 1: Mils
     * 2: Millimeters
     */
    std::string units_ptr( "defaults.dimension_units" );
    std::string precision_ptr( "defaults.dimension_precision" );

    if( !( Contains( units_ptr ) && Contains( precision_ptr ) &&
           At( units_ptr ).is_number_integer() &&
           At( precision_ptr ).is_number_integer() ) )
    {
        // if either is missing or invalid, migration doesn't make sense
        return true;
    }

    int units     = *Get<int>( units_ptr );
    int precision = *Get<int>( precision_ptr );

    // The enum maps directly to precision if the units is mils
    int extraDigits = 0;

    switch( units )
    {
    case 0: extraDigits = 3; break;
    case 2: extraDigits = 2; break;
    default: break;
    }

    precision += extraDigits;

    Set( precision_ptr, precision );

    return true;
}


bool BOARD_DESIGN_SETTINGS::LoadFromFile( const wxString& aDirectory )
{
    bool ret = NESTED_SETTINGS::LoadFromFile( aDirectory );

    // A number of things won't have been translated by the PROJECT_FILE migration because of
    // descoped objects required to decode this data.  So, it will be in the legacy.pcbnew
    // section and needs to be pulled out here

    PROJECT_FILE* project = dynamic_cast<PROJECT_FILE*>( GetParent() );

    if( !project )
        return ret;

    bool migrated = false;

    auto drcName =
            []( int aCode ) -> std::string
            {
                std::shared_ptr<DRC_ITEM> item = DRC_ITEM::Create( aCode );
                wxString name = item->GetSettingsKey();
                return std::string( name.ToUTF8() );
            };

    const std::string rs = "rule_severities.";
    const std::string no_courtyard_key = "legacy_no_courtyard_defined";
    const std::string courtyard_overlap_key = "legacy_courtyards_overlap";

    try
    {
        nlohmann::json& severities =
                project->Internals()->at( "/board/design_settings/rule_severities"_json_pointer );

        if( severities.contains( no_courtyard_key ) )
        {
            if( severities[no_courtyard_key].get<bool>() )
                Set( rs + drcName( DRCE_MISSING_COURTYARD ), "error" );
            else
                Set( rs + drcName( DRCE_MISSING_COURTYARD ), "ignore" );

            severities.erase( no_courtyard_key );
            migrated = true;
        }

        if( severities.contains( courtyard_overlap_key ) )
        {
            if( severities[courtyard_overlap_key].get<bool>() )
                Set( rs + drcName( DRCE_OVERLAPPING_FOOTPRINTS ), "error" );
            else
                Set( rs + drcName( DRCE_OVERLAPPING_FOOTPRINTS ), "ignore" );

            severities.erase( courtyard_overlap_key );
            migrated = true;
        }
    }
    catch( ... )
    {
    }
    if( Contains( "legacy" ) )
    {
        // This defaults to false for new boards, but version 5.1.x and prior kept the fillets
        // so we do the same for legacy boards.
        m_ZoneKeepExternalFillets = true;

        project->At( "legacy" ).erase( "pcbnew" );
    }

    // Now that we have everything, we need to load again
    if( migrated )
        Load();

    return ret;
}


SEVERITY BOARD_DESIGN_SETTINGS::GetSeverity( int aDRCErrorCode )
{
    return m_DRCSeverities[ aDRCErrorCode ];
}


bool BOARD_DESIGN_SETTINGS::Ignore( int aDRCErrorCode )
{
    return m_DRCSeverities[ aDRCErrorCode ] == RPT_SEVERITY_IGNORE;
}


int BOARD_DESIGN_SETTINGS::GetBiggestClearanceValue() const
{
    int            biggest = 0;
    DRC_CONSTRAINT constraint;

    if( m_DRCEngine )
    {
        m_DRCEngine->QueryWorstConstraint( CLEARANCE_CONSTRAINT, constraint );
        biggest = std::max( biggest, constraint.Value().Min() );

        m_DRCEngine->QueryWorstConstraint( HOLE_CLEARANCE_CONSTRAINT, constraint );
        biggest = std::max( biggest, constraint.Value().Min() );

        m_DRCEngine->QueryWorstConstraint( EDGE_CLEARANCE_CONSTRAINT, constraint );
        biggest = std::max( biggest, constraint.Value().Min() );

        m_DRCEngine->QueryWorstConstraint( HOLE_TO_HOLE_CONSTRAINT, constraint );
        biggest = std::max( biggest, constraint.Value().Min() );
    }

    return biggest;
}


int BOARD_DESIGN_SETTINGS::GetSmallestClearanceValue() const
{
    int clearance = m_NetSettings->m_DefaultNetClass->GetClearance();

    for( const auto& [ name, netclass ] : m_NetSettings->m_NetClasses )
        clearance = std::min( clearance, netclass->GetClearance() );

    return clearance;
}


void BOARD_DESIGN_SETTINGS::SetViaSizeIndex( unsigned aIndex )
{
    m_viaSizeIndex = std::min( aIndex, (unsigned) m_ViasDimensionsList.size() );
    m_useCustomTrackVia = false;
}


int BOARD_DESIGN_SETTINGS::GetCurrentViaSize() const
{
    if( m_useCustomTrackVia )
        return m_customViaSize.m_Diameter;
    else if( m_viaSizeIndex == 0 )
        return m_NetSettings->m_DefaultNetClass->GetViaDiameter();
    else
        return m_ViasDimensionsList[ m_viaSizeIndex ].m_Diameter;
}


int BOARD_DESIGN_SETTINGS::GetCurrentViaDrill() const
{
    int drill;

    if( m_useCustomTrackVia )
        drill = m_customViaSize.m_Drill;
    else if( m_viaSizeIndex == 0 )
        drill = m_NetSettings->m_DefaultNetClass->GetViaDrill();
    else
        drill = m_ViasDimensionsList[ m_viaSizeIndex ].m_Drill;

    return drill > 0 ? drill : -1;
}


void BOARD_DESIGN_SETTINGS::SetTrackWidthIndex( unsigned aIndex )
{
    m_trackWidthIndex = std::min( aIndex, (unsigned) m_TrackWidthList.size() );
    m_useCustomTrackVia = false;
}


int BOARD_DESIGN_SETTINGS::GetCurrentTrackWidth() const
{
    if( m_useCustomTrackVia )
        return m_customTrackWidth;
    else if( m_trackWidthIndex == 0 )
        return m_NetSettings->m_DefaultNetClass->GetTrackWidth();
    else
        return m_TrackWidthList[ m_trackWidthIndex ];
}


void BOARD_DESIGN_SETTINGS::SetDiffPairIndex( unsigned aIndex )
{
    if( !m_DiffPairDimensionsList.empty() )
    {
        m_diffPairIndex = std::min( aIndex,
                static_cast<unsigned>( m_DiffPairDimensionsList.size() ) - 1 );
    }

    m_useCustomDiffPair = false;
}


int BOARD_DESIGN_SETTINGS::GetCurrentDiffPairWidth() const
{
    if( m_useCustomDiffPair )
    {
        return m_customDiffPair.m_Width;
    }
    else if( m_diffPairIndex == 0 )
    {
        if( m_NetSettings->m_DefaultNetClass->HasDiffPairWidth() )
            return m_NetSettings->m_DefaultNetClass->GetDiffPairWidth();
        else
            return m_NetSettings->m_DefaultNetClass->GetTrackWidth();
    }
    else
    {
        return m_DiffPairDimensionsList[m_diffPairIndex].m_Width;
    }
}


int BOARD_DESIGN_SETTINGS::GetCurrentDiffPairGap() const
{
    if( m_useCustomDiffPair )
    {
        return m_customDiffPair.m_Gap;
    }
    else if( m_diffPairIndex == 0 )
    {
        if( m_NetSettings->m_DefaultNetClass->HasDiffPairGap() )
            return m_NetSettings->m_DefaultNetClass->GetDiffPairGap();
        else
            return m_NetSettings->m_DefaultNetClass->GetClearance();
    }
    else
    {
        return m_DiffPairDimensionsList[m_diffPairIndex].m_Gap;
    }
}


int BOARD_DESIGN_SETTINGS::GetCurrentDiffPairViaGap() const
{
    if( m_useCustomDiffPair )
    {
        return m_customDiffPair.m_ViaGap;
    }
    else if( m_diffPairIndex == 0 )
    {
        if( m_NetSettings->m_DefaultNetClass->HasDiffPairViaGap() )
            return m_NetSettings->m_DefaultNetClass->GetDiffPairViaGap();
        else
            return GetCurrentDiffPairGap();
    }
    else
    {
        return m_DiffPairDimensionsList[m_diffPairIndex].m_ViaGap;
    }
}


void BOARD_DESIGN_SETTINGS::SetCopperLayerCount( int aNewLayerCount )
{
    m_copperLayerCount = aNewLayerCount;

    // Update only enabled copper layers mask
    m_enabledLayers &= ~LSET::AllCuMask();

    if( aNewLayerCount > 0 )
        m_enabledLayers |= LSET::AllCuMask( aNewLayerCount );
}


void BOARD_DESIGN_SETTINGS::SetEnabledLayers( LSET aMask )
{
    // Back and front layers are always enabled.
    aMask.set( B_Cu ).set( F_Cu );

    m_enabledLayers = aMask;

    // update m_CopperLayerCount to ensure its consistency with m_EnabledLayers
    m_copperLayerCount = ( aMask & LSET::AllCuMask() ).count();
}


// Return the layer class index { silk, copper, edges & courtyards, fab, others } of the
// given layer.
int BOARD_DESIGN_SETTINGS::GetLayerClass( PCB_LAYER_ID aLayer ) const
{
    if( aLayer == F_SilkS || aLayer == B_SilkS )
        return LAYER_CLASS_SILK;
    else if( IsCopperLayer( aLayer ) )
        return LAYER_CLASS_COPPER;
    else if( aLayer == Edge_Cuts )
        return LAYER_CLASS_EDGES;
    else if( aLayer == F_CrtYd || aLayer == B_CrtYd )
        return LAYER_CLASS_COURTYARD;
    else if( aLayer == F_Fab || aLayer == B_Fab )
        return LAYER_CLASS_FAB;
    else
        return LAYER_CLASS_OTHERS;
}


int BOARD_DESIGN_SETTINGS::GetDRCEpsilon() const
{
    return pcbIUScale.mmToIU( ADVANCED_CFG::GetCfg().m_DRCEpsilon );
}


int BOARD_DESIGN_SETTINGS::GetHolePlatingThickness() const
{
    return pcbIUScale.mmToIU( ADVANCED_CFG::GetCfg().m_HoleWallThickness );
}


int BOARD_DESIGN_SETTINGS::GetLineThickness( PCB_LAYER_ID aLayer ) const
{
    return m_LineThickness[ GetLayerClass( aLayer ) ];
}


VECTOR2I BOARD_DESIGN_SETTINGS::GetTextSize( PCB_LAYER_ID aLayer ) const
{
    return m_TextSize[ GetLayerClass( aLayer ) ];
}


int BOARD_DESIGN_SETTINGS::GetTextThickness( PCB_LAYER_ID aLayer ) const
{
    return m_TextThickness[ GetLayerClass( aLayer ) ];
}


bool BOARD_DESIGN_SETTINGS::GetTextItalic( PCB_LAYER_ID aLayer ) const
{
    return m_TextItalic[ GetLayerClass( aLayer ) ];
}


bool BOARD_DESIGN_SETTINGS::GetTextUpright( PCB_LAYER_ID aLayer ) const
{
    return m_TextUpright[ GetLayerClass( aLayer ) ];
}


