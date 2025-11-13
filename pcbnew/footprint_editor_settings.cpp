/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
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

#include "footprint_editor_settings.h"

#include <common.h>
#include <layer_ids.h>
#include <lset.h>
#include <pgm_base.h>
#include <eda_text.h>
#include <pcb_dimension.h>
#include <settings/common_settings.h>
#include <settings/json_settings_internals.h>
#include <settings/parameters.h>
#include <settings/settings_manager.h>
#include <base_units.h>

#include <wx/config.h>
#include <wx/log.h>


///! Update the schema version whenever a migration is required
const int fpEditSchemaVersion = 5;


FOOTPRINT_EDITOR_SETTINGS::FOOTPRINT_EDITOR_SETTINGS() :
        PCB_VIEWERS_SETTINGS_BASE( "fpedit", fpEditSchemaVersion ),
        m_DesignSettings( nullptr, "fpedit.settings" ),
        m_MagneticItems(),
        m_Display(),
        m_UserGrid(),
        m_PolarCoords( false ),
        m_DisplayInvertXAxis( false ),
        m_DisplayInvertYAxis( false ),
        m_RotationAngle( ANGLE_90 ),
        m_AngleSnapMode( LEADER_MODE::DEG45 ),
        m_ArcEditMode( ARC_EDIT_MODE::KEEP_CENTER_ADJUST_ANGLE_RADIUS ),
        m_LibWidth( 250 ),
        m_LastExportPath()
{
    m_MagneticItems.pads      = MAGNETIC_OPTIONS::CAPTURE_ALWAYS;
    m_MagneticItems.tracks    = MAGNETIC_OPTIONS::NO_EFFECT;
    m_MagneticItems.graphics  = true;
    m_MagneticItems.allLayers = false;

    m_AuiPanels.appearance_panel_tab = 0;
    m_AuiPanels.right_panel_width = -1;
    m_AuiPanels.show_layer_manager = true;

    m_params.emplace_back( new PARAM<int>( "window.lib_width",
            &m_LibWidth, 250 ) );

    m_params.emplace_back( new PARAM<bool>( "aui.show_layer_manager",
            &m_AuiPanels.show_layer_manager, true ) );

    m_params.emplace_back( new PARAM<int>( "aui.right_panel_width",
            &m_AuiPanels.right_panel_width, -1 ) );

    m_params.emplace_back( new PARAM<int>( "aui.appearance_panel_tab",
            &m_AuiPanels.appearance_panel_tab, 0, 0, 2 ) );

    m_params.emplace_back( new PARAM<int>( "aui.properties_panel_width",
            &m_AuiPanels.properties_panel_width, -1 ) );

    m_params.emplace_back( new PARAM<float>( "aui.properties_splitter_proportion",
            &m_AuiPanels.properties_splitter, 0.5f ) );

    m_params.emplace_back( new PARAM<bool>( "aui.show_properties",
            &m_AuiPanels.show_properties, false ) );

    m_params.emplace_back( new PARAM<int>( "library.sort_mode",
            &m_LibrarySortMode, 0 ) );

    m_params.emplace_back( new PARAM<wxString>( "system.last_import_export_path",
            &m_LastExportPath, "" ) );

    m_params.emplace_back( new PARAM<bool>( "pcb_display.graphics_fill",
            &m_ViewersDisplay.m_DisplayGraphicsFill, true ) );

    m_params.emplace_back( new PARAM<bool>( "pcb_display.text_fill",
            &m_ViewersDisplay.m_DisplayTextFill, true ) );

    m_params.emplace_back( new PARAM<bool>( "pcb_display.pad_fill",
            &m_ViewersDisplay.m_DisplayPadFill, true ) );

    m_params.emplace_back( new PARAM<bool>( "pcb_display.pad_numbers",
            &m_ViewersDisplay.m_DisplayPadNumbers, true ) );

    m_params.emplace_back( new PARAM<int>( "editing.magnetic_pads",
            reinterpret_cast<int*>( &m_MagneticItems.pads ),
            static_cast<int>( MAGNETIC_OPTIONS::CAPTURE_ALWAYS ) ) );

    m_params.emplace_back( new PARAM<bool>( "editing.magnetic_graphics",
            &m_MagneticItems.graphics, true ) );

    m_params.emplace_back( new PARAM<bool>( "editing.magnetic_all_layers",
            &m_MagneticItems.allLayers, false ) );

    m_params.emplace_back( new PARAM<bool>( "editing.polar_coords",
            &m_PolarCoords, false ) );

    m_params.emplace_back( new PARAM<bool>( "origin_invert_x_axis",
            &m_DisplayInvertXAxis, false ) );

    m_params.emplace_back( new PARAM<bool>( "origin_invert_y_axis",
            &m_DisplayInvertYAxis, false ) );

    m_params.emplace_back( new PARAM_LAMBDA<int>( "editing.rotation_angle",
            [this] () -> int
            {
                return m_RotationAngle.AsTenthsOfADegree();
            },
            [this] ( int aVal )
            {
                if( aVal )
                    m_RotationAngle = EDA_ANGLE( aVal, TENTHS_OF_A_DEGREE_T );
            },
            900 ) );

    m_params.emplace_back( new PARAM<int>( "editing.fp_angle_snap_mode",
            reinterpret_cast<int*>( &m_AngleSnapMode ),
            static_cast<int>( LEADER_MODE::DEG45 ) ) );

    m_params.emplace_back( new PARAM_LAYER_PRESET( "pcb_display.layer_presets", &m_LayerPresets ) );

    m_params.emplace_back( new PARAM<wxString>( "pcb_display.active_layer_preset",
            &m_ActiveLayerPreset, "" ) );

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>(
            "design_settings.default_footprint_text_items",
            [&] () -> nlohmann::json
            {
                nlohmann::json js = nlohmann::json::array();

                for( const TEXT_ITEM_INFO& item : m_DesignSettings.m_DefaultFPTextItems )
                {
                    js.push_back( nlohmann::json( { item.m_Text.ToUTF8(),
                                                    item.m_Visible,
                                                    LSET::Name( item.m_Layer ) } ) );
                }

                return js;
            },
            [&] ( const nlohmann::json& aObj )
            {
                m_DesignSettings.m_DefaultFPTextItems.clear();

                if( !aObj.is_array() )
                    return;

                for( const nlohmann::json& entry : aObj )
                {
                    if( entry.empty() || !entry.is_array() )
                        continue;

                    TEXT_ITEM_INFO textInfo( wxT( "" ), true, F_SilkS );

                    textInfo.m_Text = entry.at(0).get<wxString>();
                    textInfo.m_Visible = entry.at(1).get<bool>();
                    wxString layerName = entry.at(2).get<wxString>();
                    int candidateLayer = LSET::NameToLayer( layerName );
                    textInfo.m_Layer = candidateLayer >= 0
                                           ? static_cast<PCB_LAYER_ID>(candidateLayer)
                                           : F_SilkS;

                    m_DesignSettings.m_DefaultFPTextItems.push_back( std::move( textInfo ) );
                }
            },
            nlohmann::json::array( {
                                       { "REF**", true, LSET::Name( F_SilkS ) },
                                       { "", true, LSET::Name( F_Fab ) },
                                       { "${REFERENCE}", true, LSET::Name( F_Fab ) }
                                   } ) ) );

    m_params.emplace_back( new PARAM_MAP<wxString>( "design_settings.default_footprint_layer_names",
                                                    &m_DesignSettings.m_UserLayerNames, {} ) );

    int minTextSize = pcbIUScale.mmToIU( TEXT_MIN_SIZE_MM );
    int maxTextSize = pcbIUScale.mmToIU( TEXT_MAX_SIZE_MM );
    int minStroke = 1;
    int maxStroke = pcbIUScale.mmToIU( 100 );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.silk_line_width",
            &m_DesignSettings.m_LineThickness[ LAYER_CLASS_SILK ],
            pcbIUScale.mmToIU( DEFAULT_SILK_LINE_WIDTH ), minStroke, maxStroke, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.silk_text_size_h",
            &m_DesignSettings.m_TextSize[ LAYER_CLASS_SILK ].x,
            pcbIUScale.mmToIU( DEFAULT_SILK_TEXT_SIZE ), minTextSize, maxTextSize, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.silk_text_size_v",
            &m_DesignSettings.m_TextSize[ LAYER_CLASS_SILK ].y,
            pcbIUScale.mmToIU( DEFAULT_SILK_TEXT_SIZE ), minTextSize, maxTextSize, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.silk_text_thickness",
            &m_DesignSettings.m_TextThickness[ LAYER_CLASS_SILK ],
            pcbIUScale.mmToIU( DEFAULT_SILK_TEXT_WIDTH ), 1, maxTextSize, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM<bool>( "design_settings.silk_text_italic",
            &m_DesignSettings.m_TextItalic[ LAYER_CLASS_SILK ], false ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.copper_line_width",
            &m_DesignSettings.m_LineThickness[ LAYER_CLASS_COPPER ],
            pcbIUScale.mmToIU( DEFAULT_COPPER_LINE_WIDTH ), minStroke, maxStroke, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.copper_text_size_h",
            &m_DesignSettings.m_TextSize[ LAYER_CLASS_COPPER ].x,
            pcbIUScale.mmToIU( DEFAULT_COPPER_TEXT_SIZE ), minTextSize, maxTextSize, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.copper_text_size_v",
            &m_DesignSettings.m_TextSize[ LAYER_CLASS_COPPER ].y,
            pcbIUScale.mmToIU( DEFAULT_COPPER_TEXT_SIZE ), minTextSize, maxTextSize, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.copper_text_thickness",
            &m_DesignSettings.m_TextThickness[ LAYER_CLASS_COPPER ],
            pcbIUScale.mmToIU( DEFAULT_COPPER_TEXT_WIDTH ), minStroke, maxStroke, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM<bool>( "design_settings.copper_text_italic",
            &m_DesignSettings.m_TextItalic[ LAYER_CLASS_COPPER ], false ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.edge_line_width",
            &m_DesignSettings.m_LineThickness[ LAYER_CLASS_EDGES ],
            pcbIUScale.mmToIU( DEFAULT_EDGE_WIDTH ), minStroke, maxStroke, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.courtyard_line_width",
            &m_DesignSettings.m_LineThickness[ LAYER_CLASS_COURTYARD ],
            pcbIUScale.mmToIU( DEFAULT_COURTYARD_WIDTH ), minStroke, maxStroke, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.fab_line_width",
           &m_DesignSettings.m_LineThickness[ LAYER_CLASS_FAB ],
           pcbIUScale.mmToIU( DEFAULT_LINE_WIDTH ), minStroke, maxStroke, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.fab_text_size_h",
           &m_DesignSettings.m_TextSize[ LAYER_CLASS_FAB ].x,
           pcbIUScale.mmToIU( DEFAULT_TEXT_SIZE ), minTextSize, maxTextSize, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.fab_text_size_v",
           &m_DesignSettings.m_TextSize[ LAYER_CLASS_FAB ].y,
           pcbIUScale.mmToIU( DEFAULT_TEXT_SIZE ), minTextSize, maxTextSize, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.fab_text_thickness",
           &m_DesignSettings.m_TextThickness[ LAYER_CLASS_FAB ],
           pcbIUScale.mmToIU( DEFAULT_TEXT_WIDTH ), 1, maxTextSize, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM<bool>( "design_settings.fab_text_italic",
           &m_DesignSettings.m_TextItalic[ LAYER_CLASS_FAB ], false ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.others_line_width",
           &m_DesignSettings.m_LineThickness[ LAYER_CLASS_OTHERS ],
           pcbIUScale.mmToIU( DEFAULT_LINE_WIDTH ), minStroke, maxStroke, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.others_text_size_h",
           &m_DesignSettings.m_TextSize[ LAYER_CLASS_OTHERS ].x,
           pcbIUScale.mmToIU( DEFAULT_TEXT_SIZE ), minTextSize, maxTextSize, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.others_text_size_v",
           &m_DesignSettings.m_TextSize[ LAYER_CLASS_OTHERS ].y,
           pcbIUScale.mmToIU( DEFAULT_TEXT_SIZE ), minTextSize, maxTextSize, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.others_text_thickness",
           &m_DesignSettings.m_TextThickness[ LAYER_CLASS_OTHERS ],
           pcbIUScale.mmToIU( DEFAULT_TEXT_WIDTH ), 1, maxTextSize, pcbIUScale.MM_PER_IU ) );

    m_params.emplace_back( new PARAM<bool>( "design_settings.others_text_italic",
            &m_DesignSettings.m_TextItalic[ LAYER_CLASS_OTHERS ], false ) );


    // ---------------------------------------------------------------------------------------------
    // Dimension settings

    m_params.emplace_back( new PARAM_ENUM<DIM_UNITS_MODE>( "design_settings.dimensions.units",
            &m_DesignSettings.m_DimensionUnitsMode, DIM_UNITS_MODE::AUTOMATIC, DIM_UNITS_MODE::INCH,
            DIM_UNITS_MODE::AUTOMATIC ) );

    m_params.emplace_back( new PARAM_ENUM<DIM_PRECISION>( "design_settings.dimensions.precision",
            &m_DesignSettings.m_DimensionPrecision, DIM_PRECISION::X_XXXX, DIM_PRECISION::X, DIM_PRECISION::V_VVVVV ) );

    m_params.emplace_back( new PARAM_ENUM<DIM_UNITS_FORMAT>( "design_settings.dimensions.units_format",
            &m_DesignSettings.m_DimensionUnitsFormat, DIM_UNITS_FORMAT::NO_SUFFIX, DIM_UNITS_FORMAT::NO_SUFFIX,
            DIM_UNITS_FORMAT::PAREN_SUFFIX ) );

    m_params.emplace_back( new PARAM<bool>( "design_settings.dimensions.suppress_zeroes",
            &m_DesignSettings.m_DimensionSuppressZeroes, true ) );

    // NOTE: excluding DIM_TEXT_POSITION::MANUAL from the valid range here
    m_params.emplace_back( new PARAM_ENUM<DIM_TEXT_POSITION>( "design_settings.dimensions.text_position",
            &m_DesignSettings.m_DimensionTextPosition, DIM_TEXT_POSITION::OUTSIDE, DIM_TEXT_POSITION::OUTSIDE,
            DIM_TEXT_POSITION::INLINE ) );

    m_params.emplace_back( new PARAM<bool>( "design_settings.dimensions.keep_text_aligned",
            &m_DesignSettings.m_DimensionKeepTextAligned, true ) );

    m_params.emplace_back( new PARAM<int>( "design_settings.dimensions.arrow_length",
            &m_DesignSettings.m_DimensionArrowLength,
            pcbIUScale.MilsToIU( DEFAULT_DIMENSION_ARROW_LENGTH ) ) );

    m_params.emplace_back( new PARAM<int>( "design_settings.dimensions.extension_offset",
            &m_DesignSettings.m_DimensionExtensionOffset,
            pcbIUScale.mmToIU( DEFAULT_DIMENSION_EXTENSION_OFFSET ) ) );

    // ---------------------------------------------------------------------------------------------

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "editing.selection_filter",
            [&]() -> nlohmann::json
            {
                nlohmann::json ret;

                ret["lockedItems"] = m_SelectionFilter.lockedItems;
                ret["footprints"]  = m_SelectionFilter.footprints;
                ret["text"]        = m_SelectionFilter.text;
                ret["tracks"]      = m_SelectionFilter.tracks;
                ret["vias"]        = m_SelectionFilter.vias;
                ret["pads"]        = m_SelectionFilter.pads;
                ret["graphics"]    = m_SelectionFilter.graphics;
                ret["zones"]       = m_SelectionFilter.zones;
                ret["keepouts"]    = m_SelectionFilter.keepouts;
                ret["dimensions"]  = m_SelectionFilter.dimensions;
                ret["points"]      = m_SelectionFilter.points;
                ret["otherItems"]  = m_SelectionFilter.otherItems;

                return ret;
            },
            [&]( const nlohmann::json& aVal )
            {
                if( aVal.empty() || !aVal.is_object() )
                    return;

                SetIfPresent( aVal, "lockedItems", m_SelectionFilter.lockedItems );
                SetIfPresent( aVal, "footprints", m_SelectionFilter.footprints );
                SetIfPresent( aVal, "text", m_SelectionFilter.text );
                SetIfPresent( aVal, "tracks", m_SelectionFilter.tracks );
                SetIfPresent( aVal, "vias", m_SelectionFilter.vias );
                SetIfPresent( aVal, "pads", m_SelectionFilter.pads );
                SetIfPresent( aVal, "graphics", m_SelectionFilter.graphics );
                SetIfPresent( aVal, "zones", m_SelectionFilter.zones );
                SetIfPresent( aVal, "keepouts", m_SelectionFilter.keepouts );
                SetIfPresent( aVal, "dimensions", m_SelectionFilter.dimensions );
                SetIfPresent( aVal, "points", m_SelectionFilter.points );
                SetIfPresent( aVal, "otherItems", m_SelectionFilter.otherItems );
            },
            {
                { "lockedItems", false },
                { "footprints", true },
                { "text", true },
                { "tracks", true },
                { "vias", true },
                { "pads", true },
                { "graphics", true },
                { "zones", true },
                { "keepouts", true },
                { "dimensions", true },
                { "points", true },
                { "otherItems", true }
            } ) );

    registerMigration( 0, 1, std::bind( &FOOTPRINT_EDITOR_SETTINGS::migrateSchema0to1, this ) );

    registerMigration( 1, 2,
                       [&]() -> bool
                       {
                           // This is actually a migration for APP_SETTINGS_BASE::m_LibTree
                           return migrateLibTreeWidth();
                       } );

    registerMigration( 2, 3, std::bind( &FOOTPRINT_EDITOR_SETTINGS::migrateSchema2To3, this ) );
    registerMigration( 3, 4, std::bind( &FOOTPRINT_EDITOR_SETTINGS::migrateSchema3To4, this ) );
    registerMigration( 4, 5, std::bind( &FOOTPRINT_EDITOR_SETTINGS::migrateSchema4To5, this ) );
}


bool FOOTPRINT_EDITOR_SETTINGS::MigrateFromLegacy( wxConfigBase* aCfg )
{
    bool ret = APP_SETTINGS_BASE::MigrateFromLegacy( aCfg );

    //
    // NOTE: there's no value in line-wrapping these; it just makes the table unreadable.
    //
    ret &= fromLegacy<int>(  aCfg, "ModeditLibWidth",              "window.lib_width" );
    ret &= fromLegacyString( aCfg, "import_last_path",             "system.last_import_export_path" );

    ret &= fromLegacy<int>(  aCfg, "FpEditorMagneticPads",               "editing.magnetic_pads" );
    ret &= fromLegacy<bool>( aCfg, "FpEditorDisplayPolarCoords",         "editing.polar_coords" );
    ret &= fromLegacy<int>(  aCfg, "FpEditorUse45DegreeGraphicSegments", "editing.use_45_degree_graphic_segments" );

    ret &= fromLegacy<bool>(  aCfg, "FpEditorGraphicLinesDisplayMode", "pcb_display.graphic_items_fill" );
    ret &= fromLegacy<bool>(  aCfg, "FpEditorPadDisplayMode",          "pcb_display.pad_fill" );
    ret &= fromLegacy<bool>(  aCfg, "FpEditorTextsDisplayMode",        "pcb_display.footprint_text" );

    ret &= fromLegacy<double>( aCfg, "FpEditorSilkLineWidth",       "design_settings.silk_line_width" );
    ret &= fromLegacy<double>( aCfg, "FpEditorSilkTextSizeH",       "design_settings.silk_text_size_h" );
    ret &= fromLegacy<double>( aCfg, "FpEditorSilkTextSizeV",       "design_settings.silk_text_size_v" );
    ret &= fromLegacy<double>( aCfg, "FpEditorSilkTextThickness",   "design_settings.silk_text_thickness" );
    ret &= fromLegacy<bool>(   aCfg, "FpEditorSilkTextItalic",      "design_settings.silk_text_italic" );
    ret &= fromLegacy<double>( aCfg, "FpEditorCopperLineWidth",     "design_settings.copper_line_width" );
    ret &= fromLegacy<double>( aCfg, "FpEditorCopperTextSizeH",     "design_settings.copper_text_size_h" );
    ret &= fromLegacy<double>( aCfg, "FpEditorCopperTextSizeV",     "design_settings.copper_text_size_v" );
    ret &= fromLegacy<double>( aCfg, "FpEditorCopperTextThickness", "design_settings.copper_text_thickness" );
    ret &= fromLegacy<bool>(   aCfg, "FpEditorCopperTextItalic",    "design_settings.copper_text_italic" );
    ret &= fromLegacy<double>( aCfg, "FpEditorEdgeCutLineWidth",    "design_settings.edge_line_width" );
    ret &= fromLegacy<double>( aCfg, "FpEditorCourtyardLineWidth",  "design_settings.courtyard_line_width" );
    ret &= fromLegacy<double>( aCfg, "FpEditorOthersLineWidth",     "design_settings.others_line_width" );
    ret &= fromLegacy<double>( aCfg, "FpEditorOthersTextSizeH",     "design_settings.others_text_size_h" );
    ret &= fromLegacy<double>( aCfg, "FpEditorOthersTextSizeV",     "design_settings.others_text_size_v" );
    ret &= fromLegacy<double>( aCfg, "FpEditorOthersTextThickness", "design_settings.others_text_thickness" );
    ret &= fromLegacy<bool>(   aCfg, "FpEditorOthersTextItalic",    "design_settings.others_text_italic" );

    nlohmann::json textItems = nlohmann::json::array( {
                                                          { "REF**", true, F_SilkS },
                                                          { "", true, F_Fab }
                                                      } );

    Set( "design_settings.default_footprint_text_items", std::move( textItems ) );

    ret &= fromLegacyString( aCfg, "FpEditorRefDefaultText",         "design_settings.default_footprint_text_items.0.0" );
    ret &= fromLegacy<bool>( aCfg, "FpEditorRefDefaultVisibility",   "design_settings.default_footprint_text_items.0.1" );
    ret &= fromLegacy<int>(  aCfg, "FpEditorRefDefaultLayer",        "design_settings.default_footprint_text_items.0.2" );
    ret &= fromLegacyString( aCfg, "FpEditorValueDefaultText",       "design_settings.default_footprint_text_items.1.0" );
    ret &= fromLegacy<bool>( aCfg, "FpEditorValueDefaultVisibility", "design_settings.default_footprint_text_items.1.1" );
    ret &= fromLegacy<int>( aCfg,  "FpEditorValueDefaultLayer",      "design_settings.default_footprint_text_items.1.2" );


    std::string f = "ModEdit";

    // Migrate color settings that were stored in the pcbnew config file
    // We create a copy of the user scheme for the footprint editor context

    SETTINGS_MANAGER& manager = Pgm().GetSettingsManager();
    COLOR_SETTINGS* cs = manager.AddNewColorSettings( "user_footprints" );

    cs->SetName( wxT( "User (Footprints)" ) );
    manager.Save( cs );

    auto migrateLegacyColor = [&] ( const std::string& aKey, int aLayerId )
                              {
                                  wxString str;

                                  if( aCfg->Read( aKey, &str ) )
                                      cs->SetColor( aLayerId, COLOR4D( str ) );
                              };

    for( int i = 0; i < PCB_LAYER_ID_COUNT; ++i )
    {
        wxString layer = LSET::Name( PCB_LAYER_ID( i ) );
        migrateLegacyColor( f + "Color4DPCBLayer_" + layer.ToStdString(), PCB_LAYER_ID( i ) );
    }

    migrateLegacyColor( f + "Color4DAnchorEx",           LAYER_ANCHOR );
    migrateLegacyColor( f + "Color4DAuxItems",           LAYER_AUX_ITEMS );
    migrateLegacyColor( f + "Color4DGrid",               LAYER_GRID );
    migrateLegacyColor( f + "Color4DNonPlatedEx",        LAYER_NON_PLATEDHOLES );
    migrateLegacyColor( f + "Color4DPCBBackground",      LAYER_PCB_BACKGROUND );
    migrateLegacyColor( f + "Color4DPCBCursor",          LAYER_CURSOR );
    migrateLegacyColor( f + "Color4DRatsEx",             LAYER_RATSNEST );
    migrateLegacyColor( f + "Color4DViaBBlindEx",        LAYER_VIA_BLIND );
    migrateLegacyColor( f + "Color4DViaBBlindEx",        LAYER_VIA_BURIED );
    migrateLegacyColor( f + "Color4DViaMicroEx",         LAYER_VIA_MICROVIA );
    migrateLegacyColor( f + "Color4DViaThruEx",          LAYER_VIA_THROUGH );
    migrateLegacyColor( f + "Color4DWorksheet",          LAYER_DRAWINGSHEET );

    manager.SaveColorSettings( cs, "board" );

    ( *m_internals )[m_internals->PointerFromString( "appearance.color_theme" )] = "user_footprints";

    double x = 0, y = 0;
    f = "ModEditFrame";

    if( aCfg->Read( f + "PcbUserGrid_X", &x ) && aCfg->Read( f + "PcbUserGrid_Y", &y ) )
    {
        EDA_UNITS u = static_cast<EDA_UNITS>( aCfg->ReadLong( f + "PcbUserGrid_Unit",
                                                              static_cast<long>( EDA_UNITS::INCH ) ) );

        // Convert to internal units
        x = EDA_UNIT_UTILS::UI::FromUserUnit( pcbIUScale, u, x );
        y = EDA_UNIT_UTILS::UI::FromUserUnit( pcbIUScale, u, y );

        Set( "window.grid.user_grid_x", EDA_UNIT_UTILS::UI::StringFromValue( pcbIUScale, u, x ) );
        Set( "window.grid.user_grid_y", EDA_UNIT_UTILS::UI::StringFromValue( pcbIUScale, u, y ) );
    }

    return ret;
}


bool FOOTPRINT_EDITOR_SETTINGS::migrateSchema0to1()
{
    /**
     * Schema version 0 to 1:
     *
     * - Check to see if a footprints version of the currently selected theme exists.
     * - If so, select it
     */

    if( !m_manager )
    {
        wxLogTrace( traceSettings,
                    wxT( "Error: FOOTPRINT_EDITOR_SETTINGS migration cannot run unmanaged!" ) );
        return false;
    }

    std::string theme_ptr( "appearance.color_theme" );

    if( !Contains( theme_ptr ) )
        return true;

    wxString selected = At( theme_ptr ).get<wxString>();
    wxString search   = selected + wxT( "_footprints" );

    for( COLOR_SETTINGS* settings : Pgm().GetSettingsManager().GetColorSettingsList() )
    {
        if( settings->GetFilename() == search )
        {
            wxLogTrace( traceSettings, wxT( "Updating footprint editor theme from %s to %s" ),
                        selected, search );
            Set( theme_ptr, search );
            return true;
        }
    }

    return true;
}


/**
 * Schema version 2: Bump for KiCad 9 layer numbering changes
 * Migrate layer presets to use new enum values for copper layers
 */
bool FOOTPRINT_EDITOR_SETTINGS::migrateSchema2To3()
{
    auto p( "/pcb_display/layer_presets"_json_pointer );

    if( !m_internals->contains( p ) || !m_internals->at( p ).is_array() )
        return true;

    nlohmann::json& presets = m_internals->at( p );

    for( nlohmann::json& entry : presets )
        PARAM_LAYER_PRESET::MigrateToV9Layers( entry );

    return true;
}


/**
 * Schema version 4: move layer presets to use named render layers
 */
bool FOOTPRINT_EDITOR_SETTINGS::migrateSchema3To4()
{
    auto p( "/pcb_display/layer_presets"_json_pointer );

    if( !m_internals->contains( p ) || !m_internals->at( p ).is_array() )
        return true;

    nlohmann::json& presets = m_internals->at( p );

    for( nlohmann::json& entry : presets )
        PARAM_LAYER_PRESET::MigrateToNamedRenderLayers( entry );

    return true;
}


/**
 * Schema version 5: move text defaults to used named layers
 */
bool FOOTPRINT_EDITOR_SETTINGS::migrateSchema4To5   ()
{
    auto p( "/design_settings/default_footprint_text_items"_json_pointer );

    if( !m_internals->contains( p ) || !m_internals->at( p ).is_array() )
        return true;

    nlohmann::json& defaults = m_internals->at( p );

    bool reset = false;

    for( nlohmann::json& entry : defaults )
    {
        TEXT_ITEM_INFO textInfo( wxT( "" ), true, F_SilkS );

        textInfo.m_Text = entry.at(0).get<wxString>();
        textInfo.m_Visible = entry.at(1).get<bool>();
        textInfo.m_Layer = static_cast<PCB_LAYER_ID>( entry.at(2).get<int>() );

        if( textInfo.m_Layer == Rescue || textInfo.m_Layer >= User_5 )
        {
            // KiCad pre-9.0 nightlies would write buggy preferences out with invalid layers.
            // If we detect that, reset to defaults
            reset = true;
        }
        else
        {
            // Coming from 8.0 or earlier, just migrate to named layers
            entry.at(2) = LSET::Name( textInfo.m_Layer );
        }
    }

    if( reset )
    {
        defaults = nlohmann::json::array( {
                                       { "REF**", true, LSET::Name( F_SilkS ) },
                                       { "", true, LSET::Name( F_Fab ) },
                                       { "${REFERENCE}", true, LSET::Name( F_Fab ) }
                                   } );
    }

    return true;
}
