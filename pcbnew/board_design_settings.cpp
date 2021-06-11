/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <layers_id_colors_and_visibility.h>
#include <kiface_i.h>
#include <pad.h>
#include <board_design_settings.h>
#include <drc/drc_item.h>
#include <drc/drc_engine.h>
#include <settings/json_settings_internals.h>
#include <settings/parameters.h>
#include <project/project_file.h>
#include <advanced_config.h>
#include <board_design_settings.h>
#include <pcbnew.h>

const int bdsSchemaVersion = 2;


BOARD_DESIGN_SETTINGS::BOARD_DESIGN_SETTINGS( JSON_SETTINGS* aParent, const std::string& aPath ) :
        NESTED_SETTINGS( "board_design_settings", bdsSchemaVersion, aParent, aPath )
{
    // We want to leave alone parameters that aren't found in the project JSON as they may be
    // initialized by the board file parser before NESTED_SETTINGS::LoadFromFile is called.
    m_resetParamsIfMissing = false;

    // Create a default NETCLASS list so that things don't break horribly if there's no project
    // loaded.  This also is used during file load for legacy boards that have netclasses stored
    // in the file.  After load, this information will be moved to the project and the pointer
    // updated.
    m_netClasses = &m_internalNetClasses;

    m_HasStackup = false;                   // no stackup defined by default

    m_Pad_Master = std::make_unique<PAD>( nullptr );

    LSET all_set = LSET().set();
    m_enabledLayers = all_set;              // All layers enabled at first.
                                            // SetCopperLayerCount() will adjust this.

    SetCopperLayerCount( 2 );               // Default design is a double sided board
    m_CurrentViaType = VIATYPE::THROUGH;

    // if true, when creating a new track starting on an existing track, use this track width
    m_UseConnectedTrackWidth = false;

    m_BlindBuriedViaAllowed = false;
    m_MicroViasAllowed  = false;

    // First is always the reference designator
    m_DefaultFPTextItems.emplace_back( wxT( "REF**" ), true, F_SilkS );
    // Second is always the value
    m_DefaultFPTextItems.emplace_back( wxT( "" ), true, F_Fab );
    // Any following ones are freebies
    m_DefaultFPTextItems.emplace_back( wxT( "${REF}" ), true, F_Fab );

    m_LineThickness[ LAYER_CLASS_SILK ] = Millimeter2iu( DEFAULT_SILK_LINE_WIDTH );
    m_TextSize[ LAYER_CLASS_SILK ] = wxSize( Millimeter2iu( DEFAULT_SILK_TEXT_SIZE ),
                                             Millimeter2iu( DEFAULT_SILK_TEXT_SIZE ) );
    m_TextThickness[ LAYER_CLASS_SILK ] = Millimeter2iu( DEFAULT_SILK_TEXT_WIDTH );
    m_TextItalic[ LAYER_CLASS_SILK ] = false;
    m_TextUpright[ LAYER_CLASS_SILK ] = false;

    m_LineThickness[ LAYER_CLASS_COPPER ] = Millimeter2iu( DEFAULT_COPPER_LINE_WIDTH );
    m_TextSize[ LAYER_CLASS_COPPER ] = wxSize( Millimeter2iu( DEFAULT_COPPER_TEXT_SIZE ),
                                               Millimeter2iu( DEFAULT_COPPER_TEXT_SIZE ) );
    m_TextThickness[ LAYER_CLASS_COPPER ] = Millimeter2iu( DEFAULT_COPPER_TEXT_WIDTH );
    m_TextItalic[ LAYER_CLASS_COPPER ] = false;
    m_TextUpright[ LAYER_CLASS_COPPER ] = false;

    // Edges & Courtyards; text properties aren't used but better to have them holding
    // reasonable values than not.
    m_LineThickness[ LAYER_CLASS_EDGES ] = Millimeter2iu( DEFAULT_EDGE_WIDTH );
    m_TextSize[ LAYER_CLASS_EDGES ] = wxSize( Millimeter2iu( DEFAULT_TEXT_SIZE ),
                                              Millimeter2iu( DEFAULT_TEXT_SIZE ) );
    m_TextThickness[ LAYER_CLASS_EDGES ] = Millimeter2iu( DEFAULT_TEXT_WIDTH );
    m_TextItalic[ LAYER_CLASS_EDGES ] = false;
    m_TextUpright[ LAYER_CLASS_EDGES ] = false;

    m_LineThickness[ LAYER_CLASS_COURTYARD ] = Millimeter2iu( DEFAULT_COURTYARD_WIDTH );
    m_TextSize[ LAYER_CLASS_COURTYARD ] = wxSize( Millimeter2iu( DEFAULT_TEXT_SIZE ),
                                                  Millimeter2iu( DEFAULT_TEXT_SIZE ) );
    m_TextThickness[ LAYER_CLASS_COURTYARD ] = Millimeter2iu( DEFAULT_TEXT_WIDTH );
    m_TextItalic[ LAYER_CLASS_COURTYARD ] = false;
    m_TextUpright[ LAYER_CLASS_COURTYARD ] = false;

    m_LineThickness[ LAYER_CLASS_FAB ] = Millimeter2iu( DEFAULT_LINE_WIDTH );
    m_TextSize[ LAYER_CLASS_FAB ] = wxSize( Millimeter2iu( DEFAULT_TEXT_SIZE ),
                                                  Millimeter2iu( DEFAULT_TEXT_SIZE ) );
    m_TextThickness[ LAYER_CLASS_FAB ] = Millimeter2iu( DEFAULT_TEXT_WIDTH );
    m_TextItalic[ LAYER_CLASS_FAB ] = false;
    m_TextUpright[ LAYER_CLASS_FAB ] = false;

    m_LineThickness[ LAYER_CLASS_OTHERS ] = Millimeter2iu( DEFAULT_LINE_WIDTH );
    m_TextSize[ LAYER_CLASS_OTHERS ] = wxSize( Millimeter2iu( DEFAULT_TEXT_SIZE ),
                                               Millimeter2iu( DEFAULT_TEXT_SIZE ) );
    m_TextThickness[ LAYER_CLASS_OTHERS ] = Millimeter2iu( DEFAULT_TEXT_WIDTH );
    m_TextItalic[ LAYER_CLASS_OTHERS ] = false;
    m_TextUpright[ LAYER_CLASS_OTHERS ] = false;

    m_DimensionPrecision       = 4;
    m_DimensionUnitsMode       = DIM_UNITS_MODE::AUTOMATIC;
    m_DimensionUnitsFormat     = DIM_UNITS_FORMAT::BARE_SUFFIX;
    m_DimensionSuppressZeroes  = false;
    m_DimensionTextPosition    = DIM_TEXT_POSITION::OUTSIDE;
    m_DimensionKeepTextAligned = true;
    m_DimensionArrowLength     = Mils2iu( DEFAULT_DIMENSION_ARROW_LENGTH );
    m_DimensionExtensionOffset = Millimeter2iu( DEFAULT_DIMENSION_EXTENSION_OFFSET );

    m_useCustomTrackVia = false;
    m_customTrackWidth  = Millimeter2iu( DEFAULT_CUSTOMTRACKWIDTH );
    m_customViaSize.m_Diameter = Millimeter2iu( DEFAULT_VIASMINSIZE );
    m_customViaSize.m_Drill = Millimeter2iu( DEFAULT_MINTHROUGHDRILL );

    m_useCustomDiffPair = false;
    m_customDiffPair.m_Width = Millimeter2iu( DEFAULT_CUSTOMDPAIRWIDTH );
    m_customDiffPair.m_Gap = Millimeter2iu( DEFAULT_CUSTOMDPAIRGAP );
    m_customDiffPair.m_ViaGap = Millimeter2iu( DEFAULT_CUSTOMDPAIRVIAGAP );

    m_MinClearance        = Millimeter2iu( DEFAULT_MINCLEARANCE );
    m_TrackMinWidth       = Millimeter2iu( DEFAULT_TRACKMINWIDTH );
    m_ViasMinAnnulus      = Millimeter2iu( DEFAULT_VIASMINSIZE - DEFAULT_MINTHROUGHDRILL ) / 2;
    m_ViasMinSize         = Millimeter2iu( DEFAULT_VIASMINSIZE );
    m_MinThroughDrill     = Millimeter2iu( DEFAULT_MINTHROUGHDRILL );
    m_MicroViasMinSize    = Millimeter2iu( DEFAULT_MICROVIASMINSIZE );
    m_MicroViasMinDrill   = Millimeter2iu( DEFAULT_MICROVIASMINDRILL );
    m_CopperEdgeClearance = Millimeter2iu( DEFAULT_COPPEREDGECLEARANCE );
    m_HoleClearance       = Millimeter2iu( DEFAULT_HOLECLEARANCE );
    m_HoleToHoleMin       = Millimeter2iu( DEFAULT_HOLETOHOLEMIN );
    m_SilkClearance       = Millimeter2iu( DEFAULT_SILKCLEARANCE );

    for( int errorCode = DRCE_FIRST; errorCode <= DRCE_LAST; ++errorCode )
        m_DRCSeverities[ errorCode ] = RPT_SEVERITY_ERROR;

    m_DRCSeverities[ DRCE_DRILLED_HOLES_COLOCATED ] = RPT_SEVERITY_WARNING;

    m_DRCSeverities[ DRCE_MISSING_COURTYARD ] = RPT_SEVERITY_IGNORE;
    m_DRCSeverities[ DRCE_PTH_IN_COURTYARD ] = RPT_SEVERITY_IGNORE;
    m_DRCSeverities[ DRCE_NPTH_IN_COURTYARD ] = RPT_SEVERITY_IGNORE;

    m_DRCSeverities[ DRCE_DANGLING_TRACK ] = RPT_SEVERITY_WARNING;
    m_DRCSeverities[ DRCE_DANGLING_VIA ] = RPT_SEVERITY_WARNING;

    m_DRCSeverities[ DRCE_MISSING_FOOTPRINT ] = RPT_SEVERITY_WARNING;
    m_DRCSeverities[ DRCE_DUPLICATE_FOOTPRINT ] = RPT_SEVERITY_WARNING;
    m_DRCSeverities[ DRCE_EXTRA_FOOTPRINT ] = RPT_SEVERITY_WARNING;
    m_DRCSeverities[ DRCE_NET_CONFLICT ] = RPT_SEVERITY_WARNING;

    m_MaxError = ARC_HIGH_DEF;
    m_ZoneFillVersion = 6;                      // Use new algo by default to fill zones
    m_ZoneKeepExternalFillets = false;          // Use new algo by default.  Legacy boards might
                                                // want to set it to true for old algo....
    m_UseHeightForLengthCalcs = true;

    // Global mask margins:
    m_SolderMaskMargin  = Millimeter2iu( DEFAULT_SOLDERMASK_CLEARANCE );
    m_SolderMaskMinWidth = Millimeter2iu( DEFAULT_SOLDERMASK_MIN_WIDTH );

    // Solder paste margin absolute value
    m_SolderPasteMargin = Millimeter2iu( DEFAULT_SOLDERPASTE_CLEARANCE );
    // Solder paste margin as a ratio of pad size
    // The final margin is the sum of these 2 values
    // Usually < 0 because the mask is smaller than pad
    m_SolderPasteMarginRatio = DEFAULT_SOLDERPASTE_RATIO;

    // Layer thickness for 3D viewer
    m_boardThickness = Millimeter2iu( DEFAULT_BOARD_THICKNESS_MM );

    m_viaSizeIndex = 0;
    m_trackWidthIndex = 0;
    m_diffPairIndex = 0;

    // Parameters stored in JSON in the project file

    // NOTE: Previously, BOARD_DESIGN_SETTINGS stored the basic board layer information (layer
    // names and enable/disable state) in the project file even though this information is also
    // stored in the board file.  This was implemented for importing these settings from another
    // project.  Going forward, the import feature will just import from other board files (since
    // we could have multi-board projects in the future anyway) so this functionality is dropped.

    m_params.emplace_back( new PARAM<bool>( "rules.allow_microvias", &m_MicroViasAllowed, false ) );

    m_params.emplace_back( new PARAM<bool>( "rules.allow_blind_buried_vias",
            &m_BlindBuriedViaAllowed, false ) );

    m_params.emplace_back( new PARAM<bool>( "rules.use_height_for_length_calcs",
            &m_UseHeightForLengthCalcs, true ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "rules.min_clearance", &m_MinClearance,
            Millimeter2iu( DEFAULT_MINCLEARANCE ), Millimeter2iu( 0.01 ), Millimeter2iu( 25.0 ),
            MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "rules.min_track_width", &m_TrackMinWidth,
            Millimeter2iu( DEFAULT_TRACKMINWIDTH ), Millimeter2iu( 0.01 ), Millimeter2iu( 25.0 ),
            MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "rules.min_via_annular_width", &m_ViasMinAnnulus,
            Millimeter2iu( DEFAULT_VIASMINSIZE ), Millimeter2iu( 0.01 ), Millimeter2iu( 25.0 ),
            MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "rules.min_via_diameter", &m_ViasMinSize,
            Millimeter2iu( DEFAULT_VIASMINSIZE ), Millimeter2iu( 0.01 ), Millimeter2iu( 25.0 ),
            MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "rules.min_through_hole_diameter",
            &m_MinThroughDrill, Millimeter2iu( DEFAULT_MINTHROUGHDRILL ), Millimeter2iu( 0.01 ),
            Millimeter2iu( 25.0 ), MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "rules.min_microvia_diameter",
            &m_MicroViasMinSize, Millimeter2iu( DEFAULT_MICROVIASMINSIZE ), Millimeter2iu( 0.01 ),
            Millimeter2iu( 10.0 ), MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "rules.min_microvia_drill", &m_MicroViasMinDrill,
            Millimeter2iu( DEFAULT_MICROVIASMINDRILL ), Millimeter2iu( 0.01 ),
            Millimeter2iu( 10.0 ), MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "rules.min_hole_to_hole", &m_HoleToHoleMin,
            Millimeter2iu( DEFAULT_HOLETOHOLEMIN ), Millimeter2iu( 0.00 ), Millimeter2iu( 10.0 ),
            MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "rules.min_hole_clearance", &m_HoleClearance,
            Millimeter2iu( DEFAULT_HOLECLEARANCE ), Millimeter2iu( 0.00 ), Millimeter2iu( 100.0 ),
            MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "rules.min_silk_clearance", &m_SilkClearance,
            Millimeter2iu( DEFAULT_SILKCLEARANCE ), Millimeter2iu( 0.00 ), Millimeter2iu( 100.0 ),
            MM_PER_IU ) );

    // Note: a clearance of -0.01 is a flag indicating we should use the legacy (pre-6.0) method
    // based on the edge cut thicknesses.
    m_params.emplace_back( new PARAM_SCALED<int>( "rules.min_copper_edge_clearance",
            &m_CopperEdgeClearance, Millimeter2iu( LEGACY_COPPEREDGECLEARANCE ),
            Millimeter2iu( -0.01 ), Millimeter2iu( 25.0 ), MM_PER_IU ) );

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
                    js.push_back( Iu2Millimeter( width ) );

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

                    m_TrackWidthList.emplace_back( Millimeter2iu( entry.get<double>() ) );
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

                    entry["diameter"] = Iu2Millimeter( via.m_Diameter );
                    entry["drill"]    = Iu2Millimeter( via.m_Drill );

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

                    int diameter = Millimeter2iu( entry["diameter"].get<double>() );
                    int drill    = Millimeter2iu( entry["drill"].get<double>() );

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

                    entry["width"]   = Iu2Millimeter( pair.m_Width );
                    entry["gap"]     = Iu2Millimeter( pair.m_Gap );
                    entry["via_gap"] = Iu2Millimeter( pair.m_ViaGap );

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

                    int width   = Millimeter2iu( entry["width"].get<double>() );
                    int gap     = Millimeter2iu( entry["gap"].get<double>() );
                    int via_gap = Millimeter2iu( entry["via_gap"].get<double>() );

                    m_DiffPairDimensionsList.emplace_back(
                            DIFF_PAIR_DIMENSION( width, gap, via_gap ) );
                }
            },
            {} ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.silk_line_width",
            &m_LineThickness[LAYER_CLASS_SILK], Millimeter2iu( DEFAULT_SILK_LINE_WIDTH ),
            Millimeter2iu( 0.01 ), Millimeter2iu( 5.0 ), MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.silk_text_size_v",
            &m_TextSize[LAYER_CLASS_SILK].y, Millimeter2iu( DEFAULT_SILK_TEXT_SIZE ),
            TEXTS_MIN_SIZE, TEXTS_MAX_SIZE, MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.silk_text_size_h",
            &m_TextSize[LAYER_CLASS_SILK].x, Millimeter2iu( DEFAULT_SILK_TEXT_SIZE ),
            TEXTS_MIN_SIZE, TEXTS_MAX_SIZE, MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.silk_text_thickness",
            &m_TextThickness[LAYER_CLASS_SILK], Millimeter2iu( DEFAULT_SILK_TEXT_WIDTH ), 1,
            TEXTS_MAX_WIDTH, MM_PER_IU ) );

    m_params.emplace_back( new PARAM<bool>( "defaults.silk_text_italic",
            &m_TextItalic[LAYER_CLASS_SILK], false ) );

    m_params.emplace_back( new PARAM<bool>( "defaults.silk_text_upright",
            &m_TextUpright[ LAYER_CLASS_SILK ], true ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.copper_line_width",
            &m_LineThickness[LAYER_CLASS_COPPER], Millimeter2iu( DEFAULT_SILK_LINE_WIDTH ),
            Millimeter2iu( 0.01 ), Millimeter2iu( 5.0 ), MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.copper_text_size_v",
            &m_TextSize[LAYER_CLASS_COPPER].y, Millimeter2iu( DEFAULT_COPPER_TEXT_SIZE ),
            TEXTS_MIN_SIZE, TEXTS_MAX_SIZE, MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.copper_text_size_h",
            &m_TextSize[LAYER_CLASS_COPPER].x, Millimeter2iu( DEFAULT_COPPER_TEXT_SIZE ),
            TEXTS_MIN_SIZE, TEXTS_MAX_SIZE, MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.copper_text_thickness",
            &m_TextThickness[LAYER_CLASS_COPPER], Millimeter2iu( DEFAULT_COPPER_TEXT_WIDTH ),
            Millimeter2iu( 0.01 ), Millimeter2iu( 5.0 ), MM_PER_IU ) );

    m_params.emplace_back( new PARAM<bool>( "defaults.copper_text_italic",
            &m_TextItalic[LAYER_CLASS_COPPER], false ) );

    m_params.emplace_back( new PARAM<bool>( "defaults.copper_text_upright",
            &m_TextUpright[LAYER_CLASS_COPPER], true ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.board_outline_line_width",
            &m_LineThickness[LAYER_CLASS_EDGES], Millimeter2iu( DEFAULT_SILK_LINE_WIDTH ),
            Millimeter2iu( 0.01 ), Millimeter2iu( 5.0 ), MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.courtyard_line_width",
            &m_LineThickness[LAYER_CLASS_COURTYARD], Millimeter2iu( DEFAULT_COURTYARD_WIDTH ),
            Millimeter2iu( 0.01 ), Millimeter2iu( 5.0 ), MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.fab_line_width",
            &m_LineThickness[LAYER_CLASS_FAB], Millimeter2iu( DEFAULT_LINE_WIDTH ),
            Millimeter2iu( 0.01 ), Millimeter2iu( 5.0 ), MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.fab_text_size_v",
            &m_TextSize[LAYER_CLASS_FAB].y, Millimeter2iu( DEFAULT_TEXT_SIZE ),
            TEXTS_MIN_SIZE, TEXTS_MAX_SIZE, MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.fab_text_size_h",
            &m_TextSize[LAYER_CLASS_FAB].x, Millimeter2iu( DEFAULT_TEXT_SIZE ),
            TEXTS_MIN_SIZE, TEXTS_MAX_SIZE, MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.fab_text_thickness",
            &m_TextThickness[LAYER_CLASS_FAB], Millimeter2iu( DEFAULT_TEXT_WIDTH ),
            Millimeter2iu( 0.01 ), Millimeter2iu( 5.0 ), MM_PER_IU ) );

    m_params.emplace_back( new PARAM<bool>( "defaults.fab_text_italic",
            &m_TextItalic[LAYER_CLASS_FAB], false ) );

    m_params.emplace_back( new PARAM<bool>( "defaults.fab_text_upright",
            &m_TextUpright[LAYER_CLASS_FAB], true ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.other_line_width",
            &m_LineThickness[LAYER_CLASS_OTHERS], Millimeter2iu( DEFAULT_LINE_WIDTH ),
            Millimeter2iu( 0.01 ), Millimeter2iu( 5.0 ), MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.other_text_size_v",
            &m_TextSize[LAYER_CLASS_OTHERS].y, Millimeter2iu( DEFAULT_TEXT_SIZE ), TEXTS_MIN_SIZE,
            TEXTS_MAX_SIZE, MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.other_text_size_h",
            &m_TextSize[LAYER_CLASS_OTHERS].x, Millimeter2iu( DEFAULT_TEXT_SIZE ), TEXTS_MIN_SIZE,
            TEXTS_MAX_SIZE, MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.other_text_thickness",
            &m_TextThickness[LAYER_CLASS_OTHERS], Millimeter2iu( DEFAULT_TEXT_WIDTH ),
            Millimeter2iu( 0.01 ), Millimeter2iu( 5.0 ), MM_PER_IU ) );

    m_params.emplace_back( new PARAM<bool>( "defaults.other_text_italic",
            &m_TextItalic[LAYER_CLASS_OTHERS], false ) );

    m_params.emplace_back( new PARAM<bool>( "defaults.other_text_upright",
            &m_TextUpright[LAYER_CLASS_OTHERS], true ) );

    m_params.emplace_back( new PARAM_ENUM<DIM_UNITS_MODE>( "defaults.dimension_units",
            &m_DimensionUnitsMode, DIM_UNITS_MODE::AUTOMATIC, DIM_UNITS_MODE::INCHES,
            DIM_UNITS_MODE::AUTOMATIC ) );

    m_params.emplace_back( new PARAM<int>( "defaults.dimension_precision",
            &m_DimensionPrecision, 4, 0, 5 ) );

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
            Mils2iu( DEFAULT_DIMENSION_ARROW_LENGTH ) ) );

    m_params.emplace_back( new PARAM<int>( "defaults.dimensions.extension_offset",
            &m_DimensionExtensionOffset,
            Millimeter2iu( DEFAULT_DIMENSION_EXTENSION_OFFSET ) ) );

    m_params.emplace_back( new PARAM<bool>( "defaults.zones.45_degree_only",
            &m_defaultZoneSettings.m_Zone_45_Only, false ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.zones.min_clearance",
            &m_defaultZoneSettings.m_ZoneClearance, Mils2iu( ZONE_CLEARANCE_MIL ),
            Millimeter2iu( 0.0 ), Millimeter2iu( 25.0 ), MM_PER_IU ) );

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "defaults.pads",
            [&]() -> nlohmann::json
            {
                nlohmann::json ret =
                        {
                            { "width",  Iu2Millimeter( m_Pad_Master->GetSize().x ) },
                            { "height", Iu2Millimeter( m_Pad_Master->GetSize().y ) },
                            { "drill",  Iu2Millimeter( m_Pad_Master->GetDrillSize().x ) }
                        };

                return ret;
            },
            [&]( const nlohmann::json& aJson )
            {
                if( aJson.contains( "width" ) && aJson.contains( "height" )
                        && aJson.contains( "drill" ) )
                {
                    wxSize sz;
                    sz.SetWidth( Millimeter2iu( aJson["width"].get<double>() ) );
                    sz.SetHeight( Millimeter2iu( aJson["height"].get<double>() ) );

                    m_Pad_Master->SetSize( sz );

                    int drill = Millimeter2iu( aJson["drill"].get<double>() );

                    m_Pad_Master->SetDrillSize( wxSize( drill, drill ) );
                }
            }, {} ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "rules.max_error", &m_MaxError, ARC_HIGH_DEF,
            Millimeter2iu( 0.0001 ), Millimeter2iu( 1.0 ), MM_PER_IU ) );

    // TODO: replace with zones_fill_version parameter and migrate zones_use_no_outline?
    m_params.emplace_back( new PARAM_LAMBDA<bool>( "zones_use_no_outline",
            [this]() -> bool
            {
                return m_ZoneFillVersion >= 6;
            },
            [this]( bool aVal )
            {
                m_ZoneFillVersion = aVal ? 6 : 5;
            },
            true ) );

    m_params.emplace_back( new PARAM<bool>( "zones_allow_external_fillets",
            &m_ZoneKeepExternalFillets, false ) );

    registerMigration( 0, 1, std::bind( &BOARD_DESIGN_SETTINGS::migrateSchema0to1, this ) );

    registerMigration( 1, 2,
            [&]() -> bool
            {
                // Schema 1 to 2: move mask and paste margin settings back to board.
                // The parameters are removed, so we just have to manually load them here and
                // they will get saved with the board
                if( OPT<double> optval = Get<double>( "rules.solder_mask_clearance" ) )
                    m_SolderMaskMargin = static_cast<int>( *optval * IU_PER_MM );

                if( OPT<double> optval = Get<double>( "rules.solder_mask_min_width" ) )
                    m_SolderMaskMinWidth = static_cast<int>( *optval * IU_PER_MM );

                if( OPT<double> optval = Get<double>( "rules.solder_paste_clearance" ) )
                    m_SolderPasteMargin = static_cast<int>( *optval * IU_PER_MM );

                if( OPT<double> optval = Get<double>( "rules.solder_paste_margin_ratio" ) )
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
    m_MicroViasAllowed       = aOther.m_MicroViasAllowed;
    m_BlindBuriedViaAllowed  = aOther.m_BlindBuriedViaAllowed;
    m_CurrentViaType         = aOther.m_CurrentViaType;
    m_UseConnectedTrackWidth = aOther.m_UseConnectedTrackWidth;
    m_MinClearance           = aOther.m_MinClearance;
    m_TrackMinWidth          = aOther.m_TrackMinWidth;
    m_ViasMinAnnulus         = aOther.m_ViasMinAnnulus;
    m_ViasMinSize            = aOther.m_ViasMinSize;
    m_MinThroughDrill        = aOther.m_MinThroughDrill;
    m_MicroViasMinSize       = aOther.m_MicroViasMinSize;
    m_MicroViasMinDrill      = aOther.m_MicroViasMinDrill;
    m_CopperEdgeClearance    = aOther.m_CopperEdgeClearance;
    m_HoleClearance          = aOther.m_HoleClearance;
    m_HoleToHoleMin          = aOther.m_HoleToHoleMin;
    m_SilkClearance          = aOther.m_SilkClearance;
    m_DRCSeverities          = aOther.m_DRCSeverities;
    m_DrcExclusions          = aOther.m_DrcExclusions;
    m_ZoneFillVersion        = aOther.m_ZoneFillVersion;
    m_ZoneKeepExternalFillets= aOther.m_ZoneKeepExternalFillets;
    m_MaxError               = aOther.m_MaxError;
    m_SolderMaskMargin       = aOther.m_SolderMaskMargin;
    m_SolderMaskMinWidth     = aOther.m_SolderMaskMinWidth;
    m_SolderPasteMargin      = aOther.m_SolderPasteMargin;
    m_SolderPasteMarginRatio = aOther.m_SolderPasteMarginRatio;
    m_DefaultFPTextItems     = aOther.m_DefaultFPTextItems;

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

    m_AuxOrigin              = aOther.m_AuxOrigin;
    m_GridOrigin             = aOther.m_GridOrigin;
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

    // Only take the pointer from the other if it isn't the default
    if( aOther.m_netClasses == &aOther.m_internalNetClasses )
        m_netClasses = &m_internalNetClasses;
    else
        m_netClasses = aOther.m_netClasses;

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
     * 2: Millimetres
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

    int units     = Get<int>( units_ptr ).value();
    int precision = Get<int>( precision_ptr ).value();

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

    std::string bp = "board.design_settings.rule_severities.";
    std::string rs = "rule_severities.";

    if( OPT<bool> v = project->Get<bool>( bp + "legacy_no_courtyard_defined" ) )
    {
        if( *v )
            Set( rs + drcName( DRCE_MISSING_COURTYARD ), "error" );
        else
            Set( rs + drcName( DRCE_MISSING_COURTYARD ), "ignore" );

        project->Internals()->erase( m_internals->PointerFromString( bp + "legacy_no_courtyard_defined" ) );
        migrated = true;
    }

    if( OPT<bool> v = project->Get<bool>( bp + "legacy_courtyards_overlap" ) )
    {
        if( *v )
            Set( rs + drcName( DRCE_OVERLAPPING_FOOTPRINTS ), "error" );
        else
            Set( rs + drcName( DRCE_OVERLAPPING_FOOTPRINTS ), "ignore" );

        project->Internals()->erase( JSON_SETTINGS_INTERNALS::PointerFromString( bp + "legacy_courtyards_overlap" ) );
        migrated = true;
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
    }

    return biggest;
}


int BOARD_DESIGN_SETTINGS::GetSmallestClearanceValue() const
{
    int clearance = GetDefault()->GetClearance();

    for( const std::pair<const wxString, NETCLASSPTR>& netclass : GetNetClasses().NetClasses() )
        clearance = std::min( clearance, netclass.second->GetClearance() );

    return clearance;
}


int BOARD_DESIGN_SETTINGS::GetCurrentMicroViaSize()
{
    NETCLASSPTR netclass = GetNetClasses().Find( m_currentNetClassName );

    return netclass->GetuViaDiameter();
}


int BOARD_DESIGN_SETTINGS::GetCurrentMicroViaDrill()
{
    NETCLASSPTR netclass = GetNetClasses().Find( m_currentNetClassName );

    return netclass->GetuViaDrill();
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
        return GetNetClasses().GetDefaultPtr()->GetViaDiameter();
    else
        return m_ViasDimensionsList[ m_viaSizeIndex ].m_Diameter;
}


int BOARD_DESIGN_SETTINGS::GetCurrentViaDrill() const
{
    int drill;

    if( m_useCustomTrackVia )
        drill = m_customViaSize.m_Drill;
    else if( m_viaSizeIndex == 0 )
        drill = GetNetClasses().GetDefaultPtr()->GetViaDrill();
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
        return GetNetClasses().GetDefaultPtr()->GetTrackWidth();
    else
        return m_TrackWidthList[ m_trackWidthIndex ];
}


void BOARD_DESIGN_SETTINGS::SetDiffPairIndex( unsigned aIndex )
{
    m_diffPairIndex = std::min( aIndex, (unsigned) 8 );
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
        if( GetNetClasses().GetDefaultPtr()->HasDiffPairWidth() )
            return GetNetClasses().GetDefaultPtr()->GetDiffPairWidth();
        else
            return GetNetClasses().GetDefaultPtr()->GetTrackWidth();
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
        if( GetNetClasses().GetDefaultPtr()->HasDiffPairGap() )
            return GetNetClasses().GetDefaultPtr()->GetDiffPairGap();
        else
            return GetNetClasses().GetDefaultPtr()->GetClearance();
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
        if( GetNetClasses().GetDefaultPtr()->HasDiffPairViaGap() )
            return GetNetClasses().GetDefaultPtr()->GetDiffPairViaGap();
        else
            return GetCurrentDiffPairGap();
    }
    else
    {
        return m_DiffPairDimensionsList[m_diffPairIndex].m_ViaGap;
    }
}


void BOARD_DESIGN_SETTINGS::SetMinHoleSeparation( int aDistance )
{
    m_HoleToHoleMin = aDistance;
}


void BOARD_DESIGN_SETTINGS::SetCopperEdgeClearance( int aDistance )
{
    m_CopperEdgeClearance = aDistance;
}


void BOARD_DESIGN_SETTINGS::SetSilkClearance( int aDistance )
{
    m_SilkClearance = aDistance;
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
    return Millimeter2iu( ADVANCED_CFG::GetCfg().m_DRCEpsilon );
}


int BOARD_DESIGN_SETTINGS::GetHolePlatingThickness() const
{
    return Millimeter2iu( ADVANCED_CFG::GetCfg().m_HoleWallThickness );
}


int BOARD_DESIGN_SETTINGS::GetLineThickness( PCB_LAYER_ID aLayer ) const
{
    return m_LineThickness[ GetLayerClass( aLayer ) ];
}


wxSize BOARD_DESIGN_SETTINGS::GetTextSize( PCB_LAYER_ID aLayer ) const
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


