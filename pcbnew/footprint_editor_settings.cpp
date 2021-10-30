/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <common.h>
#include <footprint_editor_settings.h>
#include <layer_ids.h>
#include <pgm_base.h>
#include <settings/common_settings.h>
#include <settings/json_settings_internals.h>
#include <settings/parameters.h>
#include <settings/settings_manager.h>
#include <wx/config.h>
#include <base_units.h>
#include <widgets/ui_common.h>
#include <wx/log.h>
#include <pcbnew.h>


///! Update the schema version whenever a migration is required
const int fpEditSchemaVersion = 1;


FOOTPRINT_EDITOR_SETTINGS::FOOTPRINT_EDITOR_SETTINGS() :
        APP_SETTINGS_BASE( "fpedit", fpEditSchemaVersion ),
        m_DesignSettings( nullptr, "fpedit.settings" ),
        m_MagneticItems(),
        m_Display(),
        m_UserGrid(),
        m_PolarCoords( false ),
        m_Use45Limit( true ),
        m_LibWidth( 250 ),
        m_LastImportExportPath(),
        m_FootprintTextShownColumns()
{
    m_MagneticItems.pads     = MAGNETIC_OPTIONS::CAPTURE_ALWAYS;
    m_MagneticItems.tracks   = MAGNETIC_OPTIONS::NO_EFFECT;
    m_MagneticItems.graphics = true;

    m_params.emplace_back( new PARAM<int>( "window.lib_width",
            &m_LibWidth, 250 ) );

    m_params.emplace_back( new PARAM<bool>( "aui.show_layer_manager",
            &m_AuiPanels.show_layer_manager, true ) );

    m_params.emplace_back( new PARAM<int>( "aui.right_panel_width",
            &m_AuiPanels.right_panel_width, -1 ) );

    m_params.emplace_back( new PARAM<int>( "aui.appearance_panel_tab",
            &m_AuiPanels.appearance_panel_tab, 0, 0, 2 ) );

    m_params.emplace_back( new PARAM<wxString>( "system.last_import_export_path",
            &m_LastImportExportPath, "" ) );

    m_params.emplace_back( new PARAM<wxString>( "window.footprint_text_shown_columns",
            &m_FootprintTextShownColumns, "0 1 2 3 4 5 6" ) );

    m_params.emplace_back( new PARAM<int>( "editing.magnetic_pads",
            reinterpret_cast<int*>( &m_MagneticItems.pads ),
            static_cast<int>( MAGNETIC_OPTIONS::CAPTURE_ALWAYS ) ) );

    m_params.emplace_back( new PARAM<bool>( "editing.magnetic_graphics",
            &m_MagneticItems.graphics, true ) );

    m_params.emplace_back( new PARAM<bool>( "editing.polar_coords", &m_PolarCoords, false ) );

    m_params.emplace_back( new PARAM<bool>( "editing.use_45_degree_limit",
            &m_Use45Limit, false ) );

    m_params.emplace_back( new PARAM<bool>( "pcb_display.text_fill",
            &m_Display.m_DisplayTextFill, true ) );

    m_params.emplace_back( new PARAM<bool>( "pcb_display.graphic_items_fill",
            &m_Display.m_DisplayGraphicsFill, true ) );

    m_params.emplace_back( new PARAM<bool>( "pcb_display.pad_fill",
            &m_Display.m_DisplayPadFill, true ) );

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
                                                    item.m_Layer } ) );
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
                    textInfo.m_Layer = entry.at(2).get<int>();

                    m_DesignSettings.m_DefaultFPTextItems.push_back( std::move( textInfo ) );
                }
            },
            nlohmann::json::array( {
                                       { "REF**", true, F_SilkS },
                                       { "", true, F_Fab },
                                       { "${REFERENCE}", true, F_Fab }
                                   } ) ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.silk_line_width",
            &m_DesignSettings.m_LineThickness[ LAYER_CLASS_SILK ],
            Millimeter2iu( DEFAULT_SILK_LINE_WIDTH ), Millimeter2iu( 0.01 ), Millimeter2iu( 100.0 ),
            MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.silk_text_size_h",
            &m_DesignSettings.m_TextSize[ LAYER_CLASS_SILK ].x,
            Millimeter2iu( DEFAULT_SILK_TEXT_SIZE ), TEXTS_MIN_SIZE, TEXTS_MAX_SIZE, MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.silk_text_size_v",
            &m_DesignSettings.m_TextSize[ LAYER_CLASS_SILK ].y,
            Millimeter2iu( DEFAULT_SILK_TEXT_SIZE ), TEXTS_MIN_SIZE, TEXTS_MAX_SIZE, MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.silk_text_thickness",
            &m_DesignSettings.m_TextThickness[ LAYER_CLASS_SILK ],
            Millimeter2iu( DEFAULT_SILK_TEXT_WIDTH ), 1, TEXTS_MAX_WIDTH, MM_PER_IU ) );

    m_params.emplace_back( new PARAM<bool>( "design_settings.silk_text_italic",
            &m_DesignSettings.m_TextItalic[ LAYER_CLASS_SILK ], false ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.copper_line_width",
            &m_DesignSettings.m_LineThickness[ LAYER_CLASS_COPPER ],
            Millimeter2iu( DEFAULT_COPPER_LINE_WIDTH ), Millimeter2iu( 0.01 ), Millimeter2iu( 5.0 ),
            MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.copper_text_size_h",
            &m_DesignSettings.m_TextSize[ LAYER_CLASS_COPPER ].x,
            Millimeter2iu( DEFAULT_COPPER_TEXT_SIZE ), TEXTS_MIN_SIZE, TEXTS_MAX_SIZE,
            MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.copper_text_size_v",
            &m_DesignSettings.m_TextSize[ LAYER_CLASS_COPPER ].y,
            Millimeter2iu( DEFAULT_COPPER_TEXT_SIZE ), TEXTS_MIN_SIZE, TEXTS_MAX_SIZE,
            MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.copper_text_thickness",
            &m_DesignSettings.m_TextThickness[ LAYER_CLASS_COPPER ],
            Millimeter2iu( DEFAULT_COPPER_TEXT_WIDTH ), Millimeter2iu( 0.01 ), Millimeter2iu( 5.0 ),
            MM_PER_IU ) );

    m_params.emplace_back( new PARAM<bool>( "design_settings.copper_text_italic",
            &m_DesignSettings.m_TextItalic[ LAYER_CLASS_COPPER ], false ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.edge_line_width",
            &m_DesignSettings.m_LineThickness[ LAYER_CLASS_EDGES ],
            Millimeter2iu( DEFAULT_SILK_LINE_WIDTH ), Millimeter2iu( 0.01 ), Millimeter2iu( 5.0 ),
            MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.courtyard_line_width",
            &m_DesignSettings.m_LineThickness[ LAYER_CLASS_COURTYARD ],
            Millimeter2iu( DEFAULT_COURTYARD_WIDTH ), Millimeter2iu( 0.01 ), Millimeter2iu( 5.0 ),
            MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.fab_line_width",
           &m_DesignSettings.m_LineThickness[ LAYER_CLASS_FAB ],
           Millimeter2iu( DEFAULT_LINE_WIDTH ), Millimeter2iu( 0.01 ), Millimeter2iu( 5.0 ),
           MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.fab_text_size_h",
           &m_DesignSettings.m_TextSize[ LAYER_CLASS_FAB ].x,
           Millimeter2iu( DEFAULT_TEXT_SIZE ), TEXTS_MIN_SIZE, TEXTS_MAX_SIZE, MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.fab_text_size_v",
           &m_DesignSettings.m_TextSize[ LAYER_CLASS_FAB ].y,
           Millimeter2iu( DEFAULT_TEXT_SIZE ), TEXTS_MIN_SIZE, TEXTS_MAX_SIZE, MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.fab_text_thickness",
           &m_DesignSettings.m_TextThickness[ LAYER_CLASS_FAB ],
           Millimeter2iu( DEFAULT_TEXT_WIDTH ), 1, TEXTS_MAX_WIDTH, MM_PER_IU ) );

    m_params.emplace_back( new PARAM<bool>( "design_settings.fab_text_italic",
            &m_DesignSettings.m_TextItalic[ LAYER_CLASS_FAB ], false ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.others_line_width",
           &m_DesignSettings.m_LineThickness[ LAYER_CLASS_OTHERS ],
           Millimeter2iu( DEFAULT_SILK_LINE_WIDTH ), Millimeter2iu( 0.01 ), Millimeter2iu( 5.0 ),
           MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.others_text_size_h",
           &m_DesignSettings.m_TextSize[ LAYER_CLASS_OTHERS ].x,
           Millimeter2iu( DEFAULT_TEXT_SIZE ), TEXTS_MIN_SIZE, TEXTS_MAX_SIZE, MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.others_text_size_v",
           &m_DesignSettings.m_TextSize[ LAYER_CLASS_OTHERS ].y,
           Millimeter2iu( DEFAULT_TEXT_SIZE ), TEXTS_MIN_SIZE, TEXTS_MAX_SIZE, MM_PER_IU ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "design_settings.others_text_thickness",
           &m_DesignSettings.m_TextThickness[ LAYER_CLASS_OTHERS ],
           Millimeter2iu( DEFAULT_TEXT_WIDTH ), 1, TEXTS_MAX_WIDTH, MM_PER_IU ) );

    m_params.emplace_back( new PARAM<bool>( "design_settings.others_text_italic",
            &m_DesignSettings.m_TextItalic[ LAYER_CLASS_OTHERS ], false ) );

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
                SetIfPresent( aVal, "otherItems", m_SelectionFilter.otherItems );
            },
            {
                { "lockedItems", true },
                { "footprints", true },
                { "text", true },
                { "tracks", true },
                { "vias", true },
                { "pads", true },
                { "graphics", true },
                { "zones", true },
                { "keepouts", true },
                { "dimensions", true },
                { "otherItems", true }
            } ) );

    registerMigration( 0, 1, std::bind( &FOOTPRINT_EDITOR_SETTINGS::migrateSchema0to1, this ) );
}


bool FOOTPRINT_EDITOR_SETTINGS::MigrateFromLegacy( wxConfigBase* aCfg )
{
    bool ret = APP_SETTINGS_BASE::MigrateFromLegacy( aCfg );

    //
    // NOTE: there's no value in line-wrapping these; it just makes the table unreadable.
    //
    ret &= fromLegacy<int>(  aCfg, "ModeditLibWidth",              "window.lib_width" );
    ret &= fromLegacyString( aCfg, "import_last_path",             "system.last_import_export_path" );
    ret &= fromLegacyString( aCfg, "LibFootprintTextShownColumns", "window.footprint_text_shown_columns" );

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

    Set( "design_settings.default_footprint_text_items", textItems );

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
    migrateLegacyColor( f + "Color4DNoNetPadMarker",     LAYER_NO_CONNECTS );
    migrateLegacyColor( f + "Color4DNonPlatedEx",        LAYER_NON_PLATEDHOLES );
    migrateLegacyColor( f + "Color4DPadThruHoleEx",      LAYER_PADS_TH );
    migrateLegacyColor( f + "Color4DPCBBackground",      LAYER_PCB_BACKGROUND );
    migrateLegacyColor( f + "Color4DPCBCursor",          LAYER_CURSOR );
    migrateLegacyColor( f + "Color4DRatsEx",             LAYER_RATSNEST );
    migrateLegacyColor( f + "Color4DTxtInvisEx",         LAYER_MOD_TEXT_INVISIBLE );
    migrateLegacyColor( f + "Color4DViaBBlindEx",        LAYER_VIA_BBLIND );
    migrateLegacyColor( f + "Color4DViaMicroEx",         LAYER_VIA_MICROVIA );
    migrateLegacyColor( f + "Color4DViaThruEx",          LAYER_VIA_THROUGH );
    migrateLegacyColor( f + "Color4DWorksheet",          LAYER_DRAWINGSHEET );

    manager.SaveColorSettings( cs, "board" );

    ( *m_internals )[m_internals->PointerFromString( "appearance.color_theme" )] = "user_footprints";

    double x, y;
    f = "ModEditFrame";

    if( aCfg->Read( f + "PcbUserGrid_X", &x ) && aCfg->Read( f + "PcbUserGrid_Y", &y ) )
    {
        EDA_UNITS u = static_cast<EDA_UNITS>( aCfg->ReadLong( f + "PcbUserGrid_Unit",
                static_cast<long>( EDA_UNITS::INCHES ) ) );

        // Convert to internal units
        x = From_User_Unit( u, x );
        y = From_User_Unit( u, y );

        Set( "window.grid.user_grid_x", StringFromValue( u, x ) );
        Set( "window.grid.user_grid_y", StringFromValue( u, y ) );
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
                    "Error: FOOTPRINT_EDITOR_SETTINGS migration cannot run unmanaged!" );
        return false;
    }

    std::string theme_ptr( "appearance.color_theme" );

    if( !Count( theme_ptr ) )
        return true;

    wxString selected = At( theme_ptr ).get<wxString>();
    wxString search   = selected + wxT( "_footprints" );

    for( COLOR_SETTINGS* settings : Pgm().GetSettingsManager().GetColorSettingsList() )
    {
        if( settings->GetFilename() == search )
        {
            wxLogTrace( traceSettings, "Updating footprint editor theme from %s to %s",
                        selected, search );
            Set( theme_ptr, search );
            return true;
        }
    }

    return true;
}
