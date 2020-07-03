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

#include <fctsys.h>
#include <common.h>
#include <class_board.h>
#include <class_track.h>
#include <layers_id_colors_and_visibility.h>
#include <kiface_i.h>
#include <pcbnew.h>
#include <board_design_settings.h>
#include <drc/drc.h>
#include <widgets/ui_common.h>
#include <drc/drc_rule.h>
#include <settings/parameters.h>
#include <project/project_file.h>


const int bdsSchemaVersion = 0;


BOARD_DESIGN_SETTINGS::BOARD_DESIGN_SETTINGS( JSON_SETTINGS* aParent, const std::string& aPath ) :
        NESTED_SETTINGS( "board_design_settings", bdsSchemaVersion, aParent, aPath ),
        m_Pad_Master( NULL )

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

    m_DimensionUnits = 0;       // Inches
    m_DimensionPrecision = 1;   // 0.001mm / 0.1 mil

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
    m_HoleToHoleMin       = Millimeter2iu( DEFAULT_HOLETOHOLEMIN );

    for( int errorCode = DRCE_FIRST; errorCode <= DRCE_LAST; ++errorCode )
        m_DRCSeverities[ errorCode ] = RPT_SEVERITY_ERROR;

    m_DRCSeverities[ DRCE_MISSING_COURTYARD ] = RPT_SEVERITY_IGNORE;
    m_DRCSeverities[ DRCE_PTH_IN_COURTYARD ] = RPT_SEVERITY_IGNORE;
    m_DRCSeverities[ DRCE_NPTH_IN_COURTYARD ] = RPT_SEVERITY_IGNORE;

    m_DRCSeverities[ DRCE_DANGLING_TRACK ] = RPT_SEVERITY_WARNING;
    m_DRCSeverities[ DRCE_DANGLING_VIA ] = RPT_SEVERITY_WARNING;

    m_DRCSeverities[ DRCE_MISSING_FOOTPRINT ] = RPT_SEVERITY_WARNING;
    m_DRCSeverities[ DRCE_DUPLICATE_FOOTPRINT ] = RPT_SEVERITY_WARNING;
    m_DRCSeverities[ DRCE_EXTRA_FOOTPRINT ] = RPT_SEVERITY_WARNING;

    m_MaxError = ARC_HIGH_DEF;
    m_ZoneUseNoOutlineInFill = false;   // Use compatibility mode by default

    // Global mask margins:
    m_SolderMaskMargin  = Millimeter2iu( DEFAULT_SOLDERMASK_CLEARANCE );
    m_SolderMaskMinWidth = Millimeter2iu( DEFAULT_SOLDERMASK_MIN_WIDTH );
    m_SolderPasteMargin = 0;                    // Solder paste margin absolute value
    m_SolderPasteMarginRatio = 0.0;             // Solder paste margin as a ratio of pad size
                                                // The final margin is the sum of these 2 values
                                                // Usually < 0 because the mask is smaller than pad
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

    m_params.emplace_back(
            new PARAM<bool>( "rules.allow_blind_buried_vias", &m_BlindBuriedViaAllowed, false ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "rules.min_clearance", &m_MinClearance,
            Millimeter2iu( DEFAULT_MINCLEARANCE ), Millimeter2iu( 0.01 ), Millimeter2iu( 25.0 ),
            MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "rules.min_track_width", &m_TrackMinWidth,
            Millimeter2iu( DEFAULT_TRACKMINWIDTH ), Millimeter2iu( 0.01 ), Millimeter2iu( 25.0 ),
            MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "rules.min_via_annulus", &m_ViasMinAnnulus,
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

    // Note: a clearance of -0.01 is a flag indicating we should use the legacy (pre-6.0) method
    // based on the edge cut thicknesses.
    m_params.emplace_back( new PARAM_SCALED<int>( "rules.min_copper_edge_clearance",
            &m_CopperEdgeClearance, Millimeter2iu( LEGACY_COPPEREDGECLEARANCE ),
            Millimeter2iu( -0.01 ), Millimeter2iu( 25.0 ), MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "rules.solder_mask_clearance",
            &m_SolderMaskMargin, Millimeter2iu( DEFAULT_SOLDERMASK_CLEARANCE ),
            Millimeter2iu( -1.0 ), Millimeter2iu( 1.0 ), MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "rules.solder_mask_min_width",
            &m_SolderMaskMinWidth, Millimeter2iu( DEFAULT_SOLDERMASK_MIN_WIDTH ), 0,
            Millimeter2iu( 1.0 ), MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "rules.solder_paste_clearance",
            &m_SolderPasteMargin, Millimeter2iu( DEFAULT_SOLDERPASTE_CLEARANCE ),
            Millimeter2iu( -1.0 ), Millimeter2iu( 1.0 ), MM_PER_IU ) );

    m_params.emplace_back( new PARAM<double>( "rules.solder_paste_margin_ratio",
            &m_SolderPasteMarginRatio, DEFAULT_SOLDERPASTE_RATIO, -0.5, 1.0 ) );

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "rule_severities",
            [&]() -> nlohmann::json
            {
                nlohmann::json ret = {};

                for( const RC_ITEM& item : DRC_ITEM::GetItemsWithSeverities() )
                {
                    int code = item.GetErrorCode();

                    if( !m_DRCSeverities.count( code ) )
                        continue;

                    wxString name = item.GetSettingsKey();

                    ret[std::string( name.ToUTF8() )] =
                            SeverityToString( static_cast<SEVERITY>( m_DRCSeverities[code] ) );
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

                    int width   = Millimeter2iu( entry["width"].get<int>() );
                    int gap     = Millimeter2iu( entry["gap"].get<int>() );
                    int via_gap = Millimeter2iu( entry["via_gap"].get<int>() );

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

    m_params.emplace_back( new PARAM<bool>(
            "defaults.silk_text_italic", &m_TextItalic[LAYER_CLASS_SILK], false ) );

    m_params.emplace_back( new PARAM<bool>(
            "defaults.silk_text_upright", &m_TextUpright[ LAYER_CLASS_SILK ], true ) );

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

    m_params.emplace_back( new PARAM<bool>(
            "defaults.copper_text_italic", &m_TextItalic[LAYER_CLASS_COPPER], false ) );

    m_params.emplace_back( new PARAM<bool>(
            "defaults.copper_text_upright", &m_TextUpright[LAYER_CLASS_COPPER], true ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.board_outline_line_width",
            &m_LineThickness[LAYER_CLASS_EDGES], Millimeter2iu( DEFAULT_SILK_LINE_WIDTH ),
            Millimeter2iu( 0.01 ), Millimeter2iu( 5.0 ), MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.courtyard_line_width",
            &m_LineThickness[LAYER_CLASS_COURTYARD], Millimeter2iu( DEFAULT_SILK_LINE_WIDTH ),
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

    m_params.emplace_back(
            new PARAM<bool>( "defaults.fab_text_italic", &m_TextItalic[LAYER_CLASS_FAB], false ) );

    m_params.emplace_back(
            new PARAM<bool>( "defaults.fab_text_upright", &m_TextUpright[LAYER_CLASS_FAB], true ) );

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

    m_params.emplace_back( new PARAM<bool>(
            "defaults.other_text_italic", &m_TextItalic[LAYER_CLASS_OTHERS], false ) );

    m_params.emplace_back( new PARAM<bool>(
            "defaults.other_text_upright", &m_TextUpright[LAYER_CLASS_OTHERS], true ) );

    m_params.emplace_back(
            new PARAM<int>( "defaults.dimension_units", &m_DimensionUnits, 0, 0, 2 ) );

    m_params.emplace_back(
            new PARAM<int>( "defaults.dimension_precision", &m_DimensionPrecision, 1, 0, 2 ) );

    m_params.emplace_back( new PARAM<bool>(
            "defaults.zones.45_degree_only", &m_defaultZoneSettings.m_Zone_45_Only, false ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.zones.min_clearance",
            &m_defaultZoneSettings.m_ZoneClearance, Mils2iu( ZONE_CLEARANCE_MIL ),
            Millimeter2iu( 0.0 ), Millimeter2iu( 25.0 ), MM_PER_IU ) );

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "defaults.pads",
            [&]() -> nlohmann::json
            {
                nlohmann::json ret =
                        {
                            { "width",  Iu2Millimeter( m_Pad_Master.GetSize().x ) },
                            { "height", Iu2Millimeter( m_Pad_Master.GetSize().y ) },
                            { "drill",  Iu2Millimeter( m_Pad_Master.GetDrillSize().x ) }
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

                    m_Pad_Master.SetSize( sz );

                    int drill = Millimeter2iu( aJson["drill"].get<double>() );

                    m_Pad_Master.SetDrillSize( wxSize( drill, drill ) );
                }
            }, {} ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "rules.max_error", &m_MaxError, ARC_HIGH_DEF,
            Millimeter2iu( 0.0001 ), Millimeter2iu( 1.0 ), MM_PER_IU ) );

    m_params.emplace_back(
            new PARAM<bool>( "zones_use_no_outline", &m_ZoneUseNoOutlineInFill, false ) );
}


BOARD_DESIGN_SETTINGS::~BOARD_DESIGN_SETTINGS()
{
    if( m_parent )
    {
        m_parent->ReleaseNestedSettings( this );
        m_parent = nullptr;
    }
}


BOARD_DESIGN_SETTINGS& BOARD_DESIGN_SETTINGS::operator=( const BOARD_DESIGN_SETTINGS& aOther )
{
    // Copy of NESTED_SETTINGS around is not allowed, so let's just update the params.
    m_TrackWidthList         = aOther.m_TrackWidthList;
    m_ViasDimensionsList     = aOther.m_ViasDimensionsList;
    m_DiffPairDimensionsList = aOther.m_DiffPairDimensionsList;
    m_DRCRuleSelectors       = aOther.m_DRCRuleSelectors;
    m_DRCRules               = aOther.m_DRCRules;
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
    m_HoleToHoleMin          = aOther.m_HoleToHoleMin;
    m_DRCSeverities          = aOther.m_DRCSeverities;
    m_DrcExclusions          = aOther.m_DrcExclusions;
    m_ZoneUseNoOutlineInFill = aOther.m_ZoneUseNoOutlineInFill;
    m_MaxError               = aOther.m_MaxError;
    m_SolderMaskMargin       = aOther.m_SolderMaskMargin;
    m_SolderMaskMinWidth     = aOther.m_SolderMaskMinWidth;
    m_SolderPasteMarginRatio = aOther.m_SolderPasteMarginRatio;
    m_DefaultFPTextItems     = aOther.m_DefaultFPTextItems;
    m_DimensionUnits         = aOther.m_DimensionUnits;
    m_DimensionPrecision     = aOther.m_DimensionPrecision;
    m_AuxOrigin              = aOther.m_AuxOrigin;
    m_GridOrigin             = aOther.m_GridOrigin;

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

    return *this;
}


bool BOARD_DESIGN_SETTINGS::LoadFromFile( const std::string& aDirectory )
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
                DRC_ITEM* item = DRC_ITEM::Create( aCode );
                wxString name = item->GetSettingsKey();
                delete item;
                return std::string( name.ToUTF8() );
            };

    std::string bp = "board.design_settings.rule_severities.";
    std::string rs = "rule_severities.";

    if( OPT<bool> v =
                    project->Get<bool>( PointerFromString( bp + "legacy_no_courtyard_defined" ) ) )
    {
        if( *v )
            ( *this )[PointerFromString( rs + drcName( DRCE_MISSING_COURTYARD ) )] = "error";
        else
            ( *this )[PointerFromString( rs + drcName( DRCE_MISSING_COURTYARD ) )] = "ignore";

        project->erase( PointerFromString( bp + "legacy_no_courtyard_defined" ) );
        migrated = true;
    }

    if( OPT<bool> v = project->Get<bool>( PointerFromString( bp + "legacy_courtyards_overlap" ) ) )
    {
        if( *v )
            ( *this )[PointerFromString( rs + drcName( DRCE_OVERLAPPING_FOOTPRINTS ) )] = "error";
        else
            ( *this )[PointerFromString( rs + drcName( DRCE_OVERLAPPING_FOOTPRINTS ) )] = "ignore";

        project->erase( PointerFromString( bp + "legacy_courtyards_overlap" ) );
        migrated = true;
    }

    if( project->contains( "legacy" ) )
        project->at( "legacy" ).erase( "pcbnew" );

    // Now that we have everything, we need to load again
    if( migrated )
        Load();

    return ret;
}


int BOARD_DESIGN_SETTINGS::GetSeverity( int aDRCErrorCode )
{
    return m_DRCSeverities[ aDRCErrorCode ];
}


bool BOARD_DESIGN_SETTINGS::Ignore( int aDRCErrorCode )
{
    return m_DRCSeverities[ aDRCErrorCode ] == RPT_SEVERITY_IGNORE;
}


bool BOARD_DESIGN_SETTINGS::SetCurrentNetClass( const wxString& aNetClassName )
{
    NETCLASSPTR netClass = GetNetClasses().Find( aNetClassName );
    bool        lists_sizes_modified = false;

    // if not found (should not happen) use the default
    if( !netClass )
        netClass = GetNetClasses().GetDefault();

    m_currentNetClassName = netClass->GetName();

    // Initialize others values:
    if( m_TrackWidthList.size() == 0 )
    {
        lists_sizes_modified = true;
        m_TrackWidthList.push_back( 0 );
    }

    if( m_ViasDimensionsList.size() == 0 )
    {
        lists_sizes_modified = true;
        m_ViasDimensionsList.emplace_back( VIA_DIMENSION() );
    }

    if( m_DiffPairDimensionsList.size() == 0 )
    {
        lists_sizes_modified = true;
        m_DiffPairDimensionsList.emplace_back( DIFF_PAIR_DIMENSION() );
    }

    /* note the m_ViasDimensionsList[0] and m_TrackWidthList[0] values
     * are always the Netclass values
     */
    if( m_TrackWidthList[0] != netClass->GetTrackWidth() )
    {
        lists_sizes_modified = true;
        m_TrackWidthList[0] = netClass->GetTrackWidth();
    }

    if( m_ViasDimensionsList[0].m_Diameter != netClass->GetViaDiameter() )
    {
        lists_sizes_modified = true;
        m_ViasDimensionsList[0].m_Diameter = netClass->GetViaDiameter();
    }

    if( m_ViasDimensionsList[0].m_Drill != netClass->GetViaDrill() )
    {
        lists_sizes_modified = true;
        m_ViasDimensionsList[0].m_Drill = netClass->GetViaDrill();
    }

    if( m_DiffPairDimensionsList[0].m_Width != netClass->GetDiffPairWidth() )
    {
        lists_sizes_modified = true;
        m_DiffPairDimensionsList[0].m_Width = netClass->GetDiffPairWidth();
    }

    if( m_DiffPairDimensionsList[0].m_Gap != netClass->GetDiffPairGap() )
    {
        lists_sizes_modified = true;
        m_DiffPairDimensionsList[0].m_Gap = netClass->GetDiffPairGap();
    }

    if( m_DiffPairDimensionsList[0].m_ViaGap != netClass->GetDiffPairViaGap() )
    {
        lists_sizes_modified = true;
        m_DiffPairDimensionsList[0].m_ViaGap = netClass->GetDiffPairViaGap();
    }

    if( GetViaSizeIndex() >= m_ViasDimensionsList.size() )
        SetViaSizeIndex( m_ViasDimensionsList.size() );

    if( GetTrackWidthIndex() >= m_TrackWidthList.size() )
        SetTrackWidthIndex( m_TrackWidthList.size() );

    if( GetDiffPairIndex() >= m_DiffPairDimensionsList.size() )
        SetDiffPairIndex( m_DiffPairDimensionsList.size() );

    return lists_sizes_modified;
}


int BOARD_DESIGN_SETTINGS::GetBiggestClearanceValue()
{
    int clearance = GetDefault()->GetClearance();

    for( const std::pair<const wxString, NETCLASSPTR>& netclass : GetNetClasses().NetClasses() )
        clearance = std::max( clearance, netclass.second->GetClearance() );

    for( const DRC_RULE* rule : m_DRCRules )
        clearance = std::max( clearance, rule->m_Clearance.Min );

    return clearance;
}


int BOARD_DESIGN_SETTINGS::GetSmallestClearanceValue()
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


int BOARD_DESIGN_SETTINGS::GetCurrentViaDrill() const
{
    int drill;

    if( m_useCustomTrackVia )
        drill = m_customViaSize.m_Drill;
    else
        drill = m_ViasDimensionsList[m_viaSizeIndex].m_Drill;

    return drill > 0 ? drill : -1;
}


void BOARD_DESIGN_SETTINGS::SetTrackWidthIndex( unsigned aIndex )
{
    m_trackWidthIndex = std::min( aIndex, (unsigned) m_TrackWidthList.size() );
    m_useCustomTrackVia = false;
}


void BOARD_DESIGN_SETTINGS::SetDiffPairIndex( unsigned aIndex )
{
    m_diffPairIndex = std::min( aIndex, (unsigned) 8 );
    m_useCustomDiffPair = false;
}


void BOARD_DESIGN_SETTINGS::SetMinHoleSeparation( int aDistance )
{
    m_HoleToHoleMin = aDistance;
}


void BOARD_DESIGN_SETTINGS::SetCopperEdgeClearance( int aDistance )
{
    m_CopperEdgeClearance = aDistance;
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


