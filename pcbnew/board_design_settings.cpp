/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <lset.h>
#include <kiface_base.h>
#include <pad.h>
#include <board_design_settings.h>
#include <drc/drc_item.h>
#include <drc/drc_engine.h>
#include <settings/json_settings_internals.h>
#include <settings/parameters.h>
#include <project/project_file.h>
#include <advanced_config.h>

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
    SetDefaultMasterPad();

    LSET all_set = LSET().set();
    m_enabledLayers = all_set;     // All layers enabled at first.
                                   // SetCopperLayerCount() will adjust this.

    // Default design is a double layer board with 4 user defined layers
    SetCopperLayerCount( 2 );
    SetUserDefinedLayerCount( 4 );

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

    m_StyleFPFields = false;
    m_StyleFPText = false;
    m_StyleFPShapes = false;
    m_StyleFPDimensions = false;
    m_StyleFPBarcodes = false;

    m_DimensionPrecision       = DIM_PRECISION::X_XXXX;
    m_DimensionUnitsMode       = DIM_UNITS_MODE::AUTOMATIC;
    m_DimensionUnitsFormat     = DIM_UNITS_FORMAT::NO_SUFFIX;
    m_DimensionSuppressZeroes  = true;
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
    m_MinGrooveWidth      = pcbIUScale.mmToIU( DEFAULT_MINGROOVEWIDTH );

    for( int errorCode = DRCE_FIRST; errorCode <= DRCE_LAST; ++errorCode )
        m_DRCSeverities[ errorCode ] = RPT_SEVERITY_ERROR;

    m_DRCSeverities[ DRCE_DRILLED_HOLES_COLOCATED ] = RPT_SEVERITY_WARNING;
    m_DRCSeverities[ DRCE_DRILLED_HOLES_TOO_CLOSE ] = RPT_SEVERITY_WARNING;

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
    m_DRCSeverities[ DRCE_SCHEMATIC_PARITY ] = RPT_SEVERITY_WARNING;
    m_DRCSeverities[ DRCE_FOOTPRINT_FILTERS ] = RPT_SEVERITY_IGNORE;
    m_DRCSeverities[ DRCE_SCHEMATIC_FIELDS_PARITY ] = RPT_SEVERITY_WARNING;

    m_DRCSeverities[ DRCE_SILK_CLEARANCE ] = RPT_SEVERITY_WARNING;
    m_DRCSeverities[ DRCE_SILK_MASK_CLEARANCE ] = RPT_SEVERITY_WARNING;
    m_DRCSeverities[ DRCE_SILK_EDGE_CLEARANCE ] = RPT_SEVERITY_WARNING;
    m_DRCSeverities[ DRCE_TEXT_HEIGHT ] = RPT_SEVERITY_WARNING;
    m_DRCSeverities[ DRCE_TEXT_THICKNESS ] = RPT_SEVERITY_WARNING;

    m_DRCSeverities[ DRCE_FOOTPRINT_TYPE_MISMATCH ] = RPT_SEVERITY_IGNORE;

    m_DRCSeverities[ DRCE_LIB_FOOTPRINT_ISSUES ] = RPT_SEVERITY_WARNING;
    m_DRCSeverities[ DRCE_LIB_FOOTPRINT_MISMATCH ] = RPT_SEVERITY_WARNING;

    m_DRCSeverities[ DRCE_CONNECTION_WIDTH ] = RPT_SEVERITY_WARNING;

    m_DRCSeverities[DRCE_MIRRORED_TEXT_ON_FRONT_LAYER] = RPT_SEVERITY_WARNING;
    m_DRCSeverities[DRCE_NONMIRRORED_TEXT_ON_BACK_LAYER] = RPT_SEVERITY_WARNING;

    m_DRCSeverities[DRCE_MISSING_TUNING_PROFILE] = RPT_SEVERITY_WARNING;
    m_DRCSeverities[DRCE_TUNING_PROFILE_IMPLICIT_RULES] = RPT_SEVERITY_IGNORE;

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
    m_TentViasFront = true;
    m_TentViasBack = true;

    m_CoverViasFront = false;
    m_CoverViasBack = false;

    m_PlugViasFront = false;
    m_PlugViasBack = false;

    m_CapVias = false;

    m_FillVias = false;

    // Layer thickness for 3D viewer
    m_boardThickness = pcbIUScale.mmToIU( DEFAULT_BOARD_THICKNESS_MM );

    // Default spacing for meanders
    m_SingleTrackMeanderSettings.m_spacing = pcbIUScale.mmToIU( DEFAULT_MEANDER_SPACING );
    m_SkewMeanderSettings.m_spacing = pcbIUScale.mmToIU( DEFAULT_MEANDER_SPACING );
    m_DiffPairMeanderSettings.m_spacing = pcbIUScale.mmToIU( DEFAULT_DP_MEANDER_SPACING );

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

    m_params.emplace_back( new PARAM_SCALED<int>( "rules.min_groove_width",
            &m_MinGrooveWidth, pcbIUScale.mmToIU( DEFAULT_MINGROOVEWIDTH ),
            pcbIUScale.mmToIU( 0.00 ), pcbIUScale.mmToIU( 25.0 ), pcbIUScale.MM_PER_IU ) );

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
            &m_CopperEdgeClearance, pcbIUScale.mmToIU( DEFAULT_COPPEREDGECLEARANCE ),
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

                // Load V8 'hole_near_hole' token first (if present).  Any current 'hole_to_hole' token
                // found will then overwrite it.
                // We can't use the migration architecture because we forgot to bump the version number
                // when the change was made.  But this is a one-off as any future deprecations should
                // bump the version number and use registerMigration().
                if( aJson.contains( "hole_near_hole" ) )
                    m_DRCSeverities[DRCE_DRILLED_HOLES_TOO_CLOSE] = SeverityFromString( aJson["hole_near_hole"] );

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

                for( const wxString& entry : m_DrcExclusions )
                    js.push_back( { entry, m_DrcExclusionComments[ entry ] } );

                return js;
            },
            [&]( const nlohmann::json& aObj )
            {
                m_DrcExclusions.clear();

                if( !aObj.is_array() )
                    return;

                for( const nlohmann::json& entry : aObj )
                {
                    if( entry.is_array() )
                    {
                        wxString serialized = entry[0].get<wxString>();
                        m_DrcExclusions.insert( serialized );
                        m_DrcExclusionComments[ serialized ] = entry[1].get<wxString>();
                    }
                    else if( entry.is_string() )
                    {
                        m_DrcExclusions.insert( entry.get<wxString>() );
                    }
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

                    if( !entry.contains( "width" )
                            || !entry.contains( "gap" )
                            || !entry.contains( "via_gap" ) )
                    {
                        continue;
                    }

                    int width   = pcbIUScale.mmToIU( entry["width"].get<double>() );
                    int gap     = pcbIUScale.mmToIU( entry["gap"].get<double>() );
                    int via_gap = pcbIUScale.mmToIU( entry["via_gap"].get<double>() );

                    m_DiffPairDimensionsList.emplace_back( DIFF_PAIR_DIMENSION( width, gap, via_gap ) );
                }
            },
            {} ) );

    // Handle options for teardrops (targets and some others):
    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "teardrop_options",
            [&]() -> nlohmann::json
            {
                nlohmann::json js = nlohmann::json::array();
                nlohmann::json entry = {};

                entry["td_onvia"]  = m_TeardropParamsList.m_TargetVias;
                entry["td_onpthpad"]  = m_TeardropParamsList.m_TargetPTHPads;
                entry["td_onsmdpad"]  = m_TeardropParamsList.m_TargetSMDPads;
                entry["td_ontrackend"]  = m_TeardropParamsList.m_TargetTrack2Track;
                entry["td_onroundshapesonly"]  = m_TeardropParamsList.m_UseRoundShapesOnly;

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

                    if( entry.contains( "td_onvia" ) )
                        m_TeardropParamsList.m_TargetVias = entry["td_onvia"].get<bool>();

                    if( entry.contains( "td_onpthpad" ) )
                        m_TeardropParamsList.m_TargetPTHPads = entry["td_onpthpad"].get<bool>();

                    if( entry.contains( "td_onsmdpad" ) )
                        m_TeardropParamsList.m_TargetSMDPads = entry["td_onsmdpad"].get<bool>();

                    if( entry.contains( "td_ontrackend" ) )
                        m_TeardropParamsList.m_TargetTrack2Track = entry["td_ontrackend"].get<bool>();

                    if( entry.contains( "td_onroundshapesonly" ) )
                        m_TeardropParamsList.m_UseRoundShapesOnly = entry["td_onroundshapesonly"].get<bool>();

                    // Legacy settings
                    for( int ii = 0; ii < 3; ++ii )
                    {
                        TEARDROP_PARAMETERS* td_prm = m_TeardropParamsList.GetParameters( (TARGET_TD)ii );

                        if( entry.contains( "td_allow_use_two_tracks" ) )
                            td_prm->m_AllowUseTwoTracks = entry["td_allow_use_two_tracks"].get<bool>();

                        if( entry.contains( "td_curve_segcount" ) )
                        {
                            if( entry["td_curve_segcount"].get<int>() > 0 )
                                td_prm->m_CurvedEdges = true;
                        }

                        if( entry.contains( "td_on_pad_in_zone" ) )
                            td_prm->m_TdOnPadsInZones = entry["td_on_pad_in_zone"].get<bool>();
                    }
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
                    entry["td_maxheight"]  = pcbIUScale.IUTomm( td_prm->m_TdMaxWidth );
                    entry["td_length_ratio"]  = td_prm->m_BestLengthRatio;
                    entry["td_height_ratio"]  = td_prm->m_BestWidthRatio;
                    entry["td_curve_segcount"]  = td_prm->m_CurvedEdges ? 1 : 0;
                    entry["td_width_to_size_filter_ratio"] = td_prm->m_WidthtoSizeFilterRatio;
                    entry["td_allow_use_two_tracks"] = td_prm->m_AllowUseTwoTracks;
                    entry["td_on_pad_in_zone"]  = td_prm->m_TdOnPadsInZones;

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
                            td_prm->m_TdMaxWidth = pcbIUScale.mmToIU( entry["td_maxheight"].get<double>() );

                        if( entry.contains( "td_length_ratio" ) )
                            td_prm->m_BestLengthRatio = entry["td_length_ratio"].get<double>();

                        if( entry.contains( "td_height_ratio" ) )
                            td_prm->m_BestWidthRatio = entry["td_height_ratio"].get<double>();

                        if( entry.contains( "td_curve_segcount" ) )
                        {
                            if( entry["td_curve_segcount"].get<int>() > 0 )
                                td_prm->m_CurvedEdges = true;
                        }

                        if( entry.contains( "td_width_to_size_filter_ratio" ) )
                            td_prm->m_WidthtoSizeFilterRatio = entry["td_width_to_size_filter_ratio"].get<double>();

                        if( entry.contains( "td_allow_use_two_tracks" ) )
                            td_prm->m_AllowUseTwoTracks = entry["td_allow_use_two_tracks"].get<bool>();

                        if( entry.contains( "td_on_pad_in_zone" ) )
                            td_prm->m_TdOnPadsInZones = entry["td_on_pad_in_zone"].get<bool>();
                    }
                }
            },
            {} ) );

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "tuning_pattern_settings",
            [&]() -> nlohmann::json
            {
                nlohmann::json js = {};

                auto make_settings =
                        []( const PNS::MEANDER_SETTINGS& aSettings )
                        {
                            nlohmann::json entry = {};

                            entry["min_amplitude"] = pcbIUScale.IUTomm( aSettings.m_minAmplitude );
                            entry["max_amplitude"] = pcbIUScale.IUTomm( aSettings.m_maxAmplitude );
                            entry["spacing"] = pcbIUScale.IUTomm( aSettings.m_spacing );
                            entry["corner_style"] = aSettings.m_cornerStyle == PNS::MEANDER_STYLE_CHAMFER ? 0 : 1;
                            entry["corner_radius_percentage"] = aSettings.m_cornerRadiusPercentage;
                            entry["single_sided"] = aSettings.m_singleSided;

                            return entry;
                        };

                js["single_track_defaults"] = make_settings( m_SingleTrackMeanderSettings );
                js["diff_pair_defaults"] = make_settings( m_DiffPairMeanderSettings );
                js["diff_pair_skew_defaults"] = make_settings( m_SkewMeanderSettings );

                return js;
            },
            [&]( const nlohmann::json& aObj )
            {
                auto read_settings =
                        []( const nlohmann::json& entry ) -> PNS::MEANDER_SETTINGS
                        {
                            PNS::MEANDER_SETTINGS settings;

                            if( entry.contains( "min_amplitude" ) )
                                settings.m_minAmplitude = pcbIUScale.mmToIU( entry["min_amplitude"].get<double>() );

                            if( entry.contains( "max_amplitude" ) )
                                settings.m_maxAmplitude = pcbIUScale.mmToIU( entry["max_amplitude"].get<double>() );

                            if( entry.contains( "spacing" ) )
                                settings.m_spacing = pcbIUScale.mmToIU( entry["spacing"].get<double>() );

                            if( entry.contains( "corner_style" ) )
                            {
                                settings.m_cornerStyle = entry["corner_style"] == 0 ? PNS::MEANDER_STYLE_CHAMFER
                                                                                    : PNS::MEANDER_STYLE_ROUND;
                            }

                            if( entry.contains( "corner_radius_percentage" ) )
                                settings.m_cornerRadiusPercentage = entry["corner_radius_percentage"].get<int>();

                            if( entry.contains( "single_sided" ) )
                                settings.m_singleSided = entry["single_sided"].get<bool>();

                            return settings;
                        };

                if( aObj.contains( "single_track_defaults" ) )
                    m_SingleTrackMeanderSettings = read_settings( aObj["single_track_defaults"] );

                if( aObj.contains( "diff_pair_defaults" ) )
                    m_DiffPairMeanderSettings = read_settings( aObj["diff_pair_defaults"] );

                if( aObj.contains( "diff_pair_skew_defaults" ) )
                    m_SkewMeanderSettings = read_settings( aObj["diff_pair_skew_defaults"] );
            },
            {} ) );

    int minTextSize = pcbIUScale.mmToIU( TEXT_MIN_SIZE_MM );
    int maxTextSize = pcbIUScale.mmToIU( TEXT_MAX_SIZE_MM );
    int minStroke = 1;
    int maxStroke = pcbIUScale.mmToIU( 100 );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.silk_line_width",
            &m_LineThickness[LAYER_CLASS_SILK], pcbIUScale.mmToIU( DEFAULT_SILK_LINE_WIDTH ),
            minStroke, maxStroke, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.silk_text_size_v",
            &m_TextSize[LAYER_CLASS_SILK].y, pcbIUScale.mmToIU( DEFAULT_SILK_TEXT_SIZE ),
            minTextSize, maxTextSize, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.silk_text_size_h",
            &m_TextSize[LAYER_CLASS_SILK].x, pcbIUScale.mmToIU( DEFAULT_SILK_TEXT_SIZE ),
            minTextSize, maxTextSize, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.silk_text_thickness",
            &m_TextThickness[LAYER_CLASS_SILK], pcbIUScale.mmToIU( DEFAULT_SILK_TEXT_WIDTH ),
            minStroke, maxStroke, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM<bool>( "defaults.silk_text_italic",
            &m_TextItalic[LAYER_CLASS_SILK], false ) );

    m_params.emplace_back( new PARAM<bool>( "defaults.silk_text_upright",
            &m_TextUpright[ LAYER_CLASS_SILK ], true ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.copper_line_width",
            &m_LineThickness[LAYER_CLASS_COPPER], pcbIUScale.mmToIU( DEFAULT_COPPER_LINE_WIDTH ),
            minStroke, maxStroke, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.copper_text_size_v",
            &m_TextSize[LAYER_CLASS_COPPER].y, pcbIUScale.mmToIU( DEFAULT_COPPER_TEXT_SIZE ),
            minTextSize, maxTextSize, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.copper_text_size_h",
            &m_TextSize[LAYER_CLASS_COPPER].x, pcbIUScale.mmToIU( DEFAULT_COPPER_TEXT_SIZE ),
            minTextSize, maxTextSize, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.copper_text_thickness",
            &m_TextThickness[LAYER_CLASS_COPPER], pcbIUScale.mmToIU( DEFAULT_COPPER_TEXT_WIDTH ),
            minStroke, maxStroke, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM<bool>( "defaults.copper_text_italic",
            &m_TextItalic[LAYER_CLASS_COPPER], false ) );

    m_params.emplace_back( new PARAM<bool>( "defaults.copper_text_upright",
            &m_TextUpright[LAYER_CLASS_COPPER], true ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.board_outline_line_width",
            &m_LineThickness[LAYER_CLASS_EDGES], pcbIUScale.mmToIU( DEFAULT_EDGE_WIDTH ),
            minStroke, maxStroke, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.courtyard_line_width",
            &m_LineThickness[LAYER_CLASS_COURTYARD], pcbIUScale.mmToIU( DEFAULT_COURTYARD_WIDTH ),
            minStroke, maxStroke, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.fab_line_width",
            &m_LineThickness[LAYER_CLASS_FAB], pcbIUScale.mmToIU( DEFAULT_LINE_WIDTH ),
            minStroke, maxStroke, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.fab_text_size_v",
            &m_TextSize[LAYER_CLASS_FAB].y, pcbIUScale.mmToIU( DEFAULT_TEXT_SIZE ),
            minTextSize, maxTextSize, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.fab_text_size_h",
            &m_TextSize[LAYER_CLASS_FAB].x, pcbIUScale.mmToIU( DEFAULT_TEXT_SIZE ),
            minTextSize, maxTextSize, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.fab_text_thickness",
            &m_TextThickness[LAYER_CLASS_FAB], pcbIUScale.mmToIU( DEFAULT_TEXT_WIDTH ),
            minStroke, maxStroke, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM<bool>( "defaults.fab_text_italic",
            &m_TextItalic[LAYER_CLASS_FAB], false ) );

    m_params.emplace_back( new PARAM<bool>( "defaults.fab_text_upright",
            &m_TextUpright[LAYER_CLASS_FAB], true ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.other_line_width",
            &m_LineThickness[LAYER_CLASS_OTHERS], pcbIUScale.mmToIU( DEFAULT_LINE_WIDTH ),
            minStroke, maxStroke, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.other_text_size_v",
            &m_TextSize[LAYER_CLASS_OTHERS].y, pcbIUScale.mmToIU( DEFAULT_TEXT_SIZE ),
            minTextSize, maxTextSize, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.other_text_size_h",
            &m_TextSize[LAYER_CLASS_OTHERS].x, pcbIUScale.mmToIU( DEFAULT_TEXT_SIZE ),
            minTextSize, maxTextSize, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.other_text_thickness",
            &m_TextThickness[LAYER_CLASS_OTHERS], pcbIUScale.mmToIU( DEFAULT_TEXT_WIDTH ),
            minStroke, maxStroke, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM<bool>( "defaults.other_text_italic",
            &m_TextItalic[LAYER_CLASS_OTHERS], false ) );

    m_params.emplace_back( new PARAM<bool>( "defaults.other_text_upright",
            &m_TextUpright[LAYER_CLASS_OTHERS], true ) );

    m_params.emplace_back( new PARAM_ENUM<DIM_UNITS_MODE>( "defaults.dimension_units",
            &m_DimensionUnitsMode, DIM_UNITS_MODE::AUTOMATIC, DIM_UNITS_MODE::INCH,
            DIM_UNITS_MODE::AUTOMATIC ) );

    m_params.emplace_back( new PARAM_ENUM<DIM_PRECISION>( "defaults.dimension_precision",
            &m_DimensionPrecision, DIM_PRECISION::X_XXXX, DIM_PRECISION::X, DIM_PRECISION::V_VVVVV ) );

    m_params.emplace_back( new PARAM_ENUM<DIM_UNITS_FORMAT>( "defaults.dimensions.units_format",
            &m_DimensionUnitsFormat, DIM_UNITS_FORMAT::NO_SUFFIX, DIM_UNITS_FORMAT::NO_SUFFIX,
            DIM_UNITS_FORMAT::PAREN_SUFFIX ) );

    m_params.emplace_back( new PARAM<bool>( "defaults.dimensions.suppress_zeroes",
            &m_DimensionSuppressZeroes, true ) );

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

    m_params.emplace_back( new PARAM<bool>( "defaults.apply_defaults_to_fp_fields",
            &m_StyleFPFields, false ) );
    m_params.emplace_back( new PARAM<bool>( "defaults.apply_defaults_to_fp_text",
            &m_StyleFPText, false ) );
    m_params.emplace_back( new PARAM<bool>( "defaults.apply_defaults_to_fp_shapes",
            &m_StyleFPShapes, false ) );
    m_params.emplace_back( new PARAM<bool>( "defaults.apply_defaults_to_fp_dimensions",
            &m_StyleFPDimensions, false ) );
    m_params.emplace_back( new PARAM<bool>( "defaults.apply_defaults_to_fp_barcodes",
            &m_StyleFPBarcodes, false ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "defaults.zones.min_clearance",
            &m_defaultZoneSettings.m_ZoneClearance, pcbIUScale.mmToIU( ZONE_CLEARANCE_MM ),
            pcbIUScale.mmToIU( 0.0 ), pcbIUScale.mmToIU( 25.0 ), pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "defaults.pads",
            [&]() -> nlohmann::json
            {
                nlohmann::json ret =
                        {
                            { "width",  pcbIUScale.IUTomm( m_Pad_Master->GetSize( PADSTACK::ALL_LAYERS ).x ) },
                            { "height", pcbIUScale.IUTomm( m_Pad_Master->GetSize( PADSTACK::ALL_LAYERS ).y ) },
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

                    m_Pad_Master->SetSize( PADSTACK::ALL_LAYERS, sz );

                    int drill = pcbIUScale.mmToIU( aJson["drill"].get<double>() );

                    m_Pad_Master->SetDrillSize( VECTOR2I( drill, drill ) );
                }
            }, {} ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "rules.max_error",
            &m_MaxError, ARC_HIGH_DEF,
            pcbIUScale.mmToIU( 0.0001 ), pcbIUScale.mmToIU( 1.0 ), pcbIUScale.MM_PER_IU ) );

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
    m_TrackWidthList              = aOther.m_TrackWidthList;
    m_ViasDimensionsList          = aOther.m_ViasDimensionsList;
    m_DiffPairDimensionsList      = aOther.m_DiffPairDimensionsList;
    m_CurrentViaType              = aOther.m_CurrentViaType;
    m_UseConnectedTrackWidth      = aOther.m_UseConnectedTrackWidth;
    m_TempOverrideTrackWidth      = aOther.m_TempOverrideTrackWidth;
    m_MinClearance                = aOther.m_MinClearance;
    m_MinGrooveWidth              = aOther.m_MinGrooveWidth;
    m_MinConn                     = aOther.m_MinConn;
    m_TrackMinWidth               = aOther.m_TrackMinWidth;
    m_ViasMinAnnularWidth         = aOther.m_ViasMinAnnularWidth;
    m_ViasMinSize                 = aOther.m_ViasMinSize;
    m_MinThroughDrill             = aOther.m_MinThroughDrill;
    m_MicroViasMinSize            = aOther.m_MicroViasMinSize;
    m_MicroViasMinDrill           = aOther.m_MicroViasMinDrill;
    m_CopperEdgeClearance         = aOther.m_CopperEdgeClearance;
    m_HoleClearance               = aOther.m_HoleClearance;
    m_HoleToHoleMin               = aOther.m_HoleToHoleMin;
    m_SilkClearance               = aOther.m_SilkClearance;
    m_MinResolvedSpokes           = aOther.m_MinResolvedSpokes;
    m_MinSilkTextHeight           = aOther.m_MinSilkTextHeight;
    m_MinSilkTextThickness        = aOther.m_MinSilkTextThickness;
    m_DRCSeverities               = aOther.m_DRCSeverities;
    m_DrcExclusions               = aOther.m_DrcExclusions;
    m_DrcExclusionComments        = aOther.m_DrcExclusionComments;
    m_ZoneKeepExternalFillets     = aOther.m_ZoneKeepExternalFillets;
    m_MaxError                    = aOther.m_MaxError;
    m_SolderMaskExpansion         = aOther.m_SolderMaskExpansion;
    m_SolderMaskMinWidth          = aOther.m_SolderMaskMinWidth;
    m_SolderMaskToCopperClearance = aOther.m_SolderMaskToCopperClearance;
    m_SolderPasteMargin           = aOther.m_SolderPasteMargin;
    m_SolderPasteMarginRatio      = aOther.m_SolderPasteMarginRatio;
    m_AllowSoldermaskBridgesInFPs = aOther.m_AllowSoldermaskBridgesInFPs;
    m_TentViasFront               = aOther.m_TentViasFront;
    m_TentViasBack                = aOther.m_TentViasBack;
    m_CoverViasFront              = aOther.m_CoverViasFront;
    m_CoverViasBack               = aOther.m_CoverViasBack;
    m_PlugViasFront               = aOther.m_PlugViasFront;
    m_PlugViasBack                = aOther.m_PlugViasBack;
    m_CapVias                     = aOther.m_CapVias;
    m_FillVias                    = aOther.m_FillVias;
    m_DefaultFPTextItems          = aOther.m_DefaultFPTextItems;
    m_UserLayerNames              = aOther.m_UserLayerNames;

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

    m_auxOrigin                = aOther.m_auxOrigin;
    m_gridOrigin               = aOther.m_gridOrigin;
    m_HasStackup               = aOther.m_HasStackup;
    m_UseHeightForLengthCalcs  = aOther.m_UseHeightForLengthCalcs;

    m_trackWidthIndex     = aOther.m_trackWidthIndex;
    m_viaSizeIndex        = aOther.m_viaSizeIndex;
    m_diffPairIndex       = aOther.m_diffPairIndex;
    m_useCustomTrackVia   = aOther.m_useCustomTrackVia;
    m_customTrackWidth    = aOther.m_customTrackWidth;
    m_customViaSize       = aOther.m_customViaSize;
    m_useCustomDiffPair   = aOther.m_useCustomDiffPair;
    m_customDiffPair      = aOther.m_customDiffPair;
    m_copperLayerCount    = aOther.m_copperLayerCount;
    m_userDefinedLayerCount = aOther.m_userDefinedLayerCount;
    m_enabledLayers       = aOther.m_enabledLayers;
    m_boardThickness      = aOther.m_boardThickness;
    m_currentNetClassName = aOther.m_currentNetClassName;
    m_stackup             = aOther.m_stackup;
    m_NetSettings         = aOther.m_NetSettings;
    m_Pad_Master          = std::make_unique<PAD>( *aOther.m_Pad_Master );
    m_defaultZoneSettings = aOther.m_defaultZoneSettings;

    m_StyleFPFields       = aOther.m_StyleFPFields;
    m_StyleFPText         = aOther.m_StyleFPText;
    m_StyleFPShapes       = aOther.m_StyleFPShapes;
    m_StyleFPDimensions   = aOther.m_StyleFPDimensions;
    m_StyleFPBarcodes     = aOther.m_StyleFPBarcodes;
}


bool BOARD_DESIGN_SETTINGS::operator==( const BOARD_DESIGN_SETTINGS& aOther ) const
{
    if( m_TrackWidthList         != aOther.m_TrackWidthList ) return false;
    if( m_ViasDimensionsList     != aOther.m_ViasDimensionsList ) return false;
    if( m_DiffPairDimensionsList != aOther.m_DiffPairDimensionsList ) return false;
    if( m_CurrentViaType         != aOther.m_CurrentViaType ) return false;
    if( m_UseConnectedTrackWidth != aOther.m_UseConnectedTrackWidth ) return false;
    if( m_TempOverrideTrackWidth != aOther.m_TempOverrideTrackWidth ) return false;
    if( m_MinClearance           != aOther.m_MinClearance ) return false;
    if( m_MinGrooveWidth         != aOther.m_MinGrooveWidth ) return false;
    if( m_MinConn                != aOther.m_MinConn ) return false;
    if( m_TrackMinWidth          != aOther.m_TrackMinWidth ) return false;
    if( m_ViasMinAnnularWidth    != aOther.m_ViasMinAnnularWidth ) return false;
    if( m_ViasMinSize            != aOther.m_ViasMinSize ) return false;
    if( m_MinThroughDrill        != aOther.m_MinThroughDrill ) return false;
    if( m_MicroViasMinSize       != aOther.m_MicroViasMinSize ) return false;
    if( m_MicroViasMinDrill      != aOther.m_MicroViasMinDrill ) return false;
    if( m_CopperEdgeClearance    != aOther.m_CopperEdgeClearance ) return false;
    if( m_HoleClearance          != aOther.m_HoleClearance ) return false;
    if( m_HoleToHoleMin          != aOther.m_HoleToHoleMin ) return false;
    if( m_SilkClearance          != aOther.m_SilkClearance ) return false;
    if( m_MinResolvedSpokes      != aOther.m_MinResolvedSpokes ) return false;
    if( m_MinSilkTextHeight      != aOther.m_MinSilkTextHeight ) return false;
    if( m_MinSilkTextThickness   != aOther.m_MinSilkTextThickness ) return false;
    if( m_DRCSeverities          != aOther.m_DRCSeverities ) return false;
    if( m_DrcExclusions          != aOther.m_DrcExclusions ) return false;
    if( m_DrcExclusionComments   != aOther.m_DrcExclusionComments ) return false;
    if( m_ZoneKeepExternalFillets     != aOther.m_ZoneKeepExternalFillets ) return false;
    if( m_MaxError                    != aOther.m_MaxError ) return false;
    if( m_SolderMaskExpansion         != aOther.m_SolderMaskExpansion ) return false;
    if( m_SolderMaskMinWidth          != aOther.m_SolderMaskMinWidth ) return false;
    if( m_SolderMaskToCopperClearance != aOther.m_SolderMaskToCopperClearance ) return false;
    if( m_SolderPasteMargin           != aOther.m_SolderPasteMargin ) return false;
    if( m_SolderPasteMarginRatio      != aOther.m_SolderPasteMarginRatio ) return false;
    if( m_AllowSoldermaskBridgesInFPs != aOther.m_AllowSoldermaskBridgesInFPs ) return false;
    if( m_TentViasFront               != aOther.m_TentViasFront ) return false;
    if( m_TentViasBack                != aOther.m_TentViasBack ) return false;
    if( m_CoverViasFront              != aOther.m_CoverViasFront ) return false;
    if( m_CoverViasBack               != aOther.m_CoverViasBack ) return false;
    if( m_PlugViasFront               != aOther.m_PlugViasFront ) return false;
    if( m_PlugViasBack                != aOther.m_PlugViasBack ) return false;
    if( m_CapVias                     != aOther.m_CapVias ) return false;
    if( m_FillVias                    != aOther.m_FillVias ) return false;
    if( m_DefaultFPTextItems          != aOther.m_DefaultFPTextItems ) return false;
    if( m_UserLayerNames              != aOther.m_UserLayerNames ) return false;

    if( !std::equal( std::begin( m_LineThickness ), std::end( m_LineThickness ),
                     std::begin( aOther.m_LineThickness ) ) )
        return false;

    if( !std::equal( std::begin( m_TextSize ), std::end( m_TextSize ),
                     std::begin( aOther.m_TextSize ) ) )
        return false;

    if( !std::equal( std::begin( m_TextThickness ), std::end( m_TextThickness ),
                     std::begin( aOther.m_TextThickness ) ) )
        return false;

    if( !std::equal( std::begin( m_TextItalic ), std::end( m_TextItalic ),
                     std::begin( aOther.m_TextItalic ) ) )
        return false;

    if( !std::equal( std::begin( m_TextUpright ), std::end( m_TextUpright ),
                     std::begin( aOther.m_TextUpright ) ) )
        return false;

    if( m_DimensionUnitsMode       != aOther.m_DimensionUnitsMode ) return false;
    if( m_DimensionPrecision       != aOther.m_DimensionPrecision ) return false;
    if( m_DimensionUnitsFormat     != aOther.m_DimensionUnitsFormat ) return false;
    if( m_DimensionSuppressZeroes  != aOther.m_DimensionSuppressZeroes ) return false;
    if( m_DimensionTextPosition    != aOther.m_DimensionTextPosition ) return false;
    if( m_DimensionKeepTextAligned != aOther.m_DimensionKeepTextAligned ) return false;
    if( m_DimensionArrowLength     != aOther.m_DimensionArrowLength ) return false;
    if( m_DimensionExtensionOffset != aOther.m_DimensionExtensionOffset ) return false;
    if( m_auxOrigin                != aOther.m_auxOrigin ) return false;
    if( m_gridOrigin               != aOther.m_gridOrigin ) return false;
    if( m_HasStackup               != aOther.m_HasStackup ) return false;
    if( m_UseHeightForLengthCalcs  != aOther.m_UseHeightForLengthCalcs ) return false;
    if( m_trackWidthIndex          != aOther.m_trackWidthIndex ) return false;
    if( m_viaSizeIndex             != aOther.m_viaSizeIndex ) return false;
    if( m_diffPairIndex            != aOther.m_diffPairIndex ) return false;
    if( m_useCustomTrackVia        != aOther.m_useCustomTrackVia ) return false;
    if( m_customTrackWidth         != aOther.m_customTrackWidth ) return false;
    if( m_customViaSize            != aOther.m_customViaSize ) return false;
    if( m_useCustomDiffPair        != aOther.m_useCustomDiffPair ) return false;
    if( m_customDiffPair           != aOther.m_customDiffPair ) return false;
    if( m_copperLayerCount         != aOther.m_copperLayerCount ) return false;
    if( m_userDefinedLayerCount    != aOther.m_userDefinedLayerCount ) return false;
    if( m_enabledLayers            != aOther.m_enabledLayers ) return false;
    if( m_boardThickness           != aOther.m_boardThickness ) return false;
    if( m_currentNetClassName      != aOther.m_currentNetClassName ) return false;
    if( m_stackup                  != aOther.m_stackup ) return false;
    if( *m_NetSettings             != *aOther.m_NetSettings ) return false;
    if( *m_Pad_Master              != *aOther.m_Pad_Master ) return false;
    if( m_defaultZoneSettings      != aOther.m_defaultZoneSettings ) return false;

    if( m_StyleFPFields     != aOther.m_StyleFPFields ) return false;
    if( m_StyleFPText       != aOther.m_StyleFPText ) return false;
    if( m_StyleFPShapes     != aOther.m_StyleFPShapes ) return false;
    if( m_StyleFPDimensions != aOther.m_StyleFPDimensions ) return false;
    if( m_StyleFPBarcodes   != aOther.m_StyleFPBarcodes ) return false;

    return true;
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

    if( !( Contains( units_ptr )
            && Contains( precision_ptr )
            && At( units_ptr ).is_number_integer()
            && At( precision_ptr ).is_number_integer() ) )
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
    default:                 break;
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
                return std::string( DRC_ITEM::Create( aCode )->GetSettingsKey().ToUTF8() );
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
    int            biggest = std::max( m_MinClearance, m_HoleClearance );
    DRC_CONSTRAINT constraint;

    biggest = std::max( biggest, m_HoleToHoleMin );
    biggest = std::max( biggest, m_CopperEdgeClearance );

    if( m_DRCEngine )
    {
        m_DRCEngine->QueryWorstConstraint( CLEARANCE_CONSTRAINT, constraint );
        biggest = std::max( biggest, constraint.Value().Min() );

        m_DRCEngine->QueryWorstConstraint( PHYSICAL_CLEARANCE_CONSTRAINT, constraint );
        biggest = std::max( biggest, constraint.Value().Min() );

        m_DRCEngine->QueryWorstConstraint( HOLE_CLEARANCE_CONSTRAINT, constraint );
        biggest = std::max( biggest, constraint.Value().Min() );

        m_DRCEngine->QueryWorstConstraint( EDGE_CLEARANCE_CONSTRAINT, constraint );
        biggest = std::max( biggest, constraint.Value().Min() );

        m_DRCEngine->QueryWorstConstraint( HOLE_TO_HOLE_CONSTRAINT, constraint );
        biggest = std::max( biggest, constraint.Value().Min() );
    }

    // Clip to avoid integer overflows in subsequent calculations
    return std::min( biggest, MAXIMUM_CLEARANCE );
}


int BOARD_DESIGN_SETTINGS::GetSmallestClearanceValue() const
{
    int clearance = m_NetSettings->GetDefaultNetclass()->GetClearance();

    for( const auto& [name, netclass] : m_NetSettings->GetNetclasses() )
        clearance = std::min( clearance, netclass->GetClearance() );

    return clearance;
}


void BOARD_DESIGN_SETTINGS::SetViaSizeIndex( int aIndex )
{
    m_viaSizeIndex = std::min( aIndex, (int) m_ViasDimensionsList.size() - 1 );
    m_useCustomTrackVia = false;
}


int BOARD_DESIGN_SETTINGS::GetCurrentViaSize() const
{
    if( m_useCustomTrackVia )
        return m_customViaSize.m_Diameter;
    else if( m_viaSizeIndex <= 0 || m_viaSizeIndex >= (int) m_ViasDimensionsList.size() )
        return m_NetSettings->GetDefaultNetclass()->GetViaDiameter();
    else
        return m_ViasDimensionsList[ m_viaSizeIndex ].m_Diameter;
}


int BOARD_DESIGN_SETTINGS::GetCurrentViaDrill() const
{
    int drill;

    if( m_useCustomTrackVia )
        drill = m_customViaSize.m_Drill;
    else if( m_viaSizeIndex <= 0 || m_viaSizeIndex >= (int) m_ViasDimensionsList.size() )
        drill = m_NetSettings->GetDefaultNetclass()->GetViaDrill();
    else
        drill = m_ViasDimensionsList[ m_viaSizeIndex ].m_Drill;

    return drill > 0 ? drill : -1;
}


void BOARD_DESIGN_SETTINGS::SetTrackWidthIndex( int aIndex )
{
    m_trackWidthIndex = std::min( aIndex, (int) m_TrackWidthList.size() - 1 );
    m_useCustomTrackVia = false;
}


int BOARD_DESIGN_SETTINGS::GetCurrentTrackWidth() const
{
    if( m_useCustomTrackVia )
        return m_customTrackWidth;
    else if( m_trackWidthIndex <= 0 || m_trackWidthIndex >= (int) m_TrackWidthList.size() )
        return m_NetSettings->GetDefaultNetclass()->GetTrackWidth();
    else
        return m_TrackWidthList[ m_trackWidthIndex ];
}


void BOARD_DESIGN_SETTINGS::SetDiffPairIndex( int aIndex )
{
    if( !m_DiffPairDimensionsList.empty() )
        m_diffPairIndex = std::min( aIndex, (int) m_DiffPairDimensionsList.size() - 1 );

    m_useCustomDiffPair = false;
}


int BOARD_DESIGN_SETTINGS::GetCurrentDiffPairWidth() const
{
    if( m_useCustomDiffPair )
    {
        return m_customDiffPair.m_Width;
    }
    else if( m_diffPairIndex <= 0 || m_diffPairIndex >= (int) m_DiffPairDimensionsList.size() )
    {
        if( m_NetSettings->GetDefaultNetclass()->HasDiffPairWidth() )
            return m_NetSettings->GetDefaultNetclass()->GetDiffPairWidth();
        else
            return m_NetSettings->GetDefaultNetclass()->GetTrackWidth();
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
        if( m_NetSettings->GetDefaultNetclass()->HasDiffPairGap() )
            return m_NetSettings->GetDefaultNetclass()->GetDiffPairGap();
        else
            return m_NetSettings->GetDefaultNetclass()->GetClearance();
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
        if( m_NetSettings->GetDefaultNetclass()->HasDiffPairViaGap() )
            return m_NetSettings->GetDefaultNetclass()->GetDiffPairViaGap();
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
    m_enabledLayers.ClearCopperLayers();

    if( aNewLayerCount > 0 )
        m_enabledLayers |= LSET::AllCuMask( aNewLayerCount );
}


void BOARD_DESIGN_SETTINGS::SetUserDefinedLayerCount( int aNewLayerCount )
{
    m_userDefinedLayerCount = aNewLayerCount;

    m_enabledLayers.ClearUserDefinedLayers();

    if( aNewLayerCount > 0 )
        m_enabledLayers |= LSET::UserDefinedLayersMask( aNewLayerCount );
}


void BOARD_DESIGN_SETTINGS::SetEnabledLayers( const LSET& aMask )
{
    m_enabledLayers = aMask;

    // Ensures mandatory back and front layers are always enabled regardless of board file
    // configuration.
    m_enabledLayers.set( B_Cu ).set( F_Cu )
                   .set( B_CrtYd ).set( F_CrtYd )
                   .set( Edge_Cuts )
                   .set( Margin );

    // update layer counts to ensure their consistency with m_EnabledLayers
    LSET copperLayers = aMask;
    copperLayers.ClearNonCopperLayers();

    LSET userLayers = aMask & LSET::UserDefinedLayersMask();

    m_copperLayerCount      = (int) copperLayers.count();
    m_userDefinedLayerCount = (int) userLayers.count();
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

void BOARD_DESIGN_SETTINGS::SetDefaultMasterPad()
{
    m_Pad_Master->SetSizeX( pcbIUScale.mmToIU( DEFAULT_PAD_WIDTH_MM ) );
    m_Pad_Master->SetSizeY( pcbIUScale.mmToIU( DEFAULT_PAD_HEIGTH_MM ) );
    m_Pad_Master->SetDrillShape( PAD_DRILL_SHAPE::CIRCLE );
    m_Pad_Master->SetDrillSize( VECTOR2I( pcbIUScale.mmToIU( DEFAULT_PAD_DRILL_DIAMETER_MM ), 0 ) );
    m_Pad_Master->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::ROUNDRECT );

    constexpr double RR_RADIUS = DEFAULT_PAD_HEIGTH_MM * DEFAULT_PAD_RR_RADIUS_RATIO;
    m_Pad_Master->SetRoundRectCornerRadius( PADSTACK::ALL_LAYERS, pcbIUScale.mmToIU( RR_RADIUS ) );

    if( m_Pad_Master->GetFrontShape() == PAD_SHAPE::CIRCLE )
        m_Pad_Master->SetThermalSpokeAngle( ANGLE_45 );
    else
        m_Pad_Master->SetThermalSpokeAngle( ANGLE_90 );
}
