/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2023 CERN
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

#include <fmt/format.h>
#include <3d_enums.h>
#include <common_ogl/ogl_attr_list.h>
#include <settings/parameters.h>
#include <settings/json_settings_internals.h>
#include <3d_canvas/board_adapter.h>
#include <wx/config.h>

#include "eda_3d_viewer_settings.h"

using KIGFX::COLOR4D;

using namespace std::placeholders;


LAYER_PRESET_3D::LAYER_PRESET_3D( const wxString& aName ) :
        name( aName )
{
    layers.set( LAYER_3D_BOARD );
    layers.set( LAYER_3D_PLATED_BARRELS );
    layers.set( LAYER_3D_COPPER_TOP );
    layers.set( LAYER_3D_COPPER_BOTTOM );
    layers.set( LAYER_3D_SILKSCREEN_TOP );
    layers.set( LAYER_3D_SILKSCREEN_BOTTOM );
    layers.set( LAYER_3D_SOLDERMASK_TOP );
    layers.set( LAYER_3D_SOLDERMASK_BOTTOM );
    layers.set( LAYER_3D_SOLDERPASTE );
    layers.set( LAYER_3D_ADHESIVE );

    layers.set( LAYER_3D_TH_MODELS );
    layers.set( LAYER_3D_SMD_MODELS );

    layers.set( LAYER_FP_REFERENCES );
    layers.set( LAYER_FP_TEXT );

    layers.set( LAYER_GRID_AXES );

    // Preload colors vector so we don't have to worry about exceptions using colors.at()
    colors[ LAYER_3D_BACKGROUND_TOP ]    = BOARD_ADAPTER::g_DefaultBackgroundTop;
    colors[ LAYER_3D_BACKGROUND_BOTTOM ] = BOARD_ADAPTER::g_DefaultBackgroundBot;
    colors[ LAYER_3D_BOARD ]             = BOARD_ADAPTER::g_DefaultBoardBody;
    colors[ LAYER_3D_PLATED_BARRELS ]    = BOARD_ADAPTER::g_DefaultSurfaceFinish;
    colors[ LAYER_3D_COPPER_TOP ]        = BOARD_ADAPTER::g_DefaultSurfaceFinish;
    colors[ LAYER_3D_COPPER_BOTTOM ]     = BOARD_ADAPTER::g_DefaultSurfaceFinish;
    colors[ LAYER_3D_SILKSCREEN_TOP ]    = BOARD_ADAPTER::g_DefaultSilkscreen;
    colors[ LAYER_3D_SILKSCREEN_BOTTOM ] = BOARD_ADAPTER::g_DefaultSilkscreen;
    colors[ LAYER_3D_SOLDERMASK_TOP ]    = BOARD_ADAPTER::g_DefaultSolderMask;
    colors[ LAYER_3D_SOLDERMASK_BOTTOM ] = BOARD_ADAPTER::g_DefaultSolderMask;
    colors[ LAYER_3D_SOLDERPASTE ]       = BOARD_ADAPTER::g_DefaultSolderPaste;
    colors[ LAYER_3D_USER_DRAWINGS ]     = BOARD_ADAPTER::g_DefaultComments;
    colors[ LAYER_3D_USER_COMMENTS ]     = BOARD_ADAPTER::g_DefaultComments;
    colors[ LAYER_3D_USER_ECO1 ]         = BOARD_ADAPTER::g_DefaultECOs;
    colors[ LAYER_3D_USER_ECO2 ]         = BOARD_ADAPTER::g_DefaultECOs;
}


PARAM_LAYER_PRESET_3D::PARAM_LAYER_PRESET_3D( const std::string& aPath,
                                              std::vector<LAYER_PRESET_3D>* aPresetList ) :
        PARAM_LAMBDA<nlohmann::json>( aPath,
                                      std::bind( &PARAM_LAYER_PRESET_3D::presetsToJson, this ),
                                      std::bind( &PARAM_LAYER_PRESET_3D::jsonToPresets, this, _1 ),
                                      {} ),
        m_presets( aPresetList )
{
    wxASSERT( aPresetList );

#define LAYER( n, l ) m_layerToLayerNameMap[l] = n; m_layerNameToLayerMap[n] = l;

    LAYER( "fp_values",           LAYER_FP_VALUES            );
    LAYER( "fp_references",       LAYER_FP_REFERENCES        );
    LAYER( "fp_text",             LAYER_FP_TEXT              );
    LAYER( "background_bottom",   LAYER_3D_BACKGROUND_BOTTOM );
    LAYER( "background_top",      LAYER_3D_BACKGROUND_TOP    );
    LAYER( "board",               LAYER_3D_BOARD             );
    LAYER( "plated_barrels",      LAYER_3D_PLATED_BARRELS    );
    LAYER( "copper",              LAYER_3D_COPPER_TOP        );
    LAYER( "copper_bottom",       LAYER_3D_COPPER_BOTTOM     );
    LAYER( "silkscreen_bottom",   LAYER_3D_SILKSCREEN_BOTTOM );
    LAYER( "silkscreen_top",      LAYER_3D_SILKSCREEN_TOP    );
    LAYER( "soldermask_bottom",   LAYER_3D_SOLDERMASK_BOTTOM );
    LAYER( "soldermask_top",      LAYER_3D_SOLDERMASK_TOP    );
    LAYER( "solderpaste",         LAYER_3D_SOLDERPASTE       );
    LAYER( "adhesive",            LAYER_3D_ADHESIVE          );
    LAYER( "user_comments",       LAYER_3D_USER_COMMENTS     );
    LAYER( "user_drawings",       LAYER_3D_USER_DRAWINGS     );
    LAYER( "user_eco1",           LAYER_3D_USER_ECO1         );
    LAYER( "user_eco2",           LAYER_3D_USER_ECO2         );
    LAYER( "3d_navigator",        LAYER_3D_NAVIGATOR         );
    LAYER( "th_models",           LAYER_3D_TH_MODELS         );
    LAYER( "smd_models",          LAYER_3D_SMD_MODELS        );
    LAYER( "virtual_models",      LAYER_3D_VIRTUAL_MODELS    );
    LAYER( "non_pos_file_models", LAYER_3D_MODELS_NOT_IN_POS );
    LAYER( "dnp_models",          LAYER_3D_MODELS_MARKED_DNP );
    LAYER( "bounding_boxes",      LAYER_3D_BOUNDING_BOXES    );
    LAYER( "off_board_silk",      LAYER_3D_OFF_BOARD_SILK    );
}


nlohmann::json PARAM_LAYER_PRESET_3D::presetsToJson()
{
    nlohmann::json ret = nlohmann::json::array();

    for( const LAYER_PRESET_3D& preset : *m_presets )
    {
        nlohmann::json js = {
            { "name", preset.name }
        };

        nlohmann::json layers = nlohmann::json::array();

        for( int layer = 0; layer < LAYER_3D_END; ++layer )
        {
            if( preset.layers.test( layer ) )
                layers.push_back( m_layerToLayerNameMap[layer] );
        }

        js["layers"] = layers;

        nlohmann::json colors = nlohmann::json::array();

        for( const auto& [ layer, color ] : preset.colors )
        {
            nlohmann::json layerColor = {
                { "layer", m_layerToLayerNameMap[layer] },
                { "color", color.ToCSSString() }
            };

            colors.push_back( layerColor );
        }

        js["colors"] = colors;

        ret.push_back( js );
    }

    return ret;
}


void PARAM_LAYER_PRESET_3D::jsonToPresets( const nlohmann::json& aJson )
{
    if( aJson.empty() || !aJson.is_array() )
        return;

    m_presets->clear();

    for( const nlohmann::json& preset : aJson )
    {
        if( preset.contains( "name" ) )
        {
            LAYER_PRESET_3D p( preset.at( "name" ).get<wxString>() );

            if( preset.contains( "layers" ) && preset.at( "layers" ).is_array() )
            {
                p.layers.reset();

                for( const nlohmann::json& layer : preset.at( "layers" ) )
                {
                    if( layer.is_string() )
                        p.layers.set( m_layerNameToLayerMap[layer.get<wxString>()] );
                }
            }

            if( preset.contains( "colors" ) && preset.at( "colors" ).is_array() )
            {
                for( const nlohmann::json& entry : preset.at( "colors" ) )
                {
                    if( entry.contains( "layer" ) && entry.contains( "color" )
                        && entry.at( "layer" ).is_string() )
                    {
                        int layerNum = m_layerNameToLayerMap[entry.at( "layer" ).get<wxString>()];
                        p.colors[ layerNum ] = entry.at( "color" ).get<COLOR4D>();
                    }
                }
            }

            m_presets->emplace_back( p );
        }
    }
}


///! Update the schema version whenever a migration is required
const int viewer3dSchemaVersion = 4;


EDA_3D_VIEWER_SETTINGS::EDA_3D_VIEWER_SETTINGS() :
        APP_SETTINGS_BASE( "3d_viewer", viewer3dSchemaVersion ),
        m_Render(),
        m_Camera()
{
    m_params.emplace_back( new PARAM<bool>( "aui.show_layer_manager",
                                            &m_AuiPanels.show_layer_manager, true ) );

    m_params.emplace_back( new PARAM<int>( "aui.right_panel_width",
                                           &m_AuiPanels.right_panel_width, -1 ) );

    m_params.emplace_back( new PARAM_ENUM<RENDER_ENGINE>( "render.engine", &m_Render.engine,
                                                          RENDER_ENGINE::OPENGL,
                                                          RENDER_ENGINE::OPENGL,
                                                          RENDER_ENGINE::RAYTRACING ) );

    m_params.emplace_back( new PARAM_ENUM<GRID3D_TYPE>( "render.grid_type", &m_Render.grid_type,
                                                        GRID3D_TYPE::NONE,
                                                        GRID3D_TYPE::NONE,
                                                        GRID3D_TYPE::GRID_10MM ) );

    m_params.emplace_back( new PARAM_ENUM<MATERIAL_MODE>( "render.material_mode",
                                                          &m_Render.material_mode,
                                                          MATERIAL_MODE::NORMAL,
                                                          MATERIAL_MODE::NORMAL,
                                                          MATERIAL_MODE::CAD_MODE ) );

    m_params.emplace_back( new PARAM_ENUM<ANTIALIASING_MODE>( "render.opengl_AA_mode",
                                                              &m_Render.opengl_AA_mode,
                                                              ANTIALIASING_MODE::AA_8X,
                                                              ANTIALIASING_MODE::AA_NONE,
                                                              ANTIALIASING_MODE::AA_8X ) );

    m_params.emplace_back( new PARAM<COLOR4D>( "render.opengl_selection_color",
                                               &m_Render.opengl_selection_color,
                                               COLOR4D( 0.0, 1.0, 0.0, 1.0 ) ) );

    // OpenGL options
    m_params.emplace_back( new PARAM<bool>( "render.opengl_copper_thickness",
                                            &m_Render.opengl_copper_thickness, false ) );
    m_params.emplace_back( new PARAM<bool>( "render.opengl_show_model_bbox",
                                            &m_Render.show_model_bbox, false ) );
    m_params.emplace_back( new PARAM<bool>( "render.opengl_show_off_board_silk",
                                            &m_Render.show_off_board_silk, false ) );
    m_params.emplace_back( new PARAM<bool>( "render.opengl_highlight_on_rollover",
                                            &m_Render.highlight_on_rollover, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.opengl_AA_disableOnMove",
                                            &m_Render.opengl_AA_disableOnMove, false ) );
    m_params.emplace_back( new PARAM<bool>( "render.opengl_thickness_disableOnMove",
                                            &m_Render.opengl_thickness_disableOnMove, false ) );
    m_params.emplace_back( new PARAM<bool>( "render.opengl_vias_disableOnMove",
                                            &m_Render.opengl_microvias_disableOnMove, false ) );
    m_params.emplace_back( new PARAM<bool>( "render.opengl_holes_disableOnMove",
                                            &m_Render.opengl_holes_disableOnMove, false ) );
    m_params.emplace_back( new PARAM<bool>( "render.opengl_render_bbox_only_OnMove",
                                            &m_Render.opengl_render_bbox_only_OnMove, false ) );

    // Raytracing options
    m_params.emplace_back( new PARAM<bool>( "render.raytrace_anti_aliasing",
                                            &m_Render.raytrace_anti_aliasing, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.raytrace_backfloor",
                                            &m_Render.raytrace_backfloor, false ) );
    m_params.emplace_back( new PARAM<bool>( "render.raytrace_post_processing",
                                            &m_Render.raytrace_post_processing, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.raytrace_procedural_textures",
                                             &m_Render.raytrace_procedural_textures, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.raytrace_reflections",
                                            &m_Render.raytrace_reflections, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.raytrace_refractions",
                                            &m_Render.raytrace_refractions, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.raytrace_shadows",
                                            &m_Render.raytrace_shadows, true ) );

    m_params.emplace_back( new PARAM<int>( "render.raytrace_nrsamples_shadows",
                                           &m_Render.raytrace_nrsamples_shadows, 3 ) );
    m_params.emplace_back( new PARAM<int>( "render.raytrace_nrsamples_reflections",
                                           &m_Render.raytrace_nrsamples_reflections, 3 ) );
    m_params.emplace_back( new PARAM<int>( "render.raytrace_nrsamples_refractions",
                                           &m_Render.raytrace_nrsamples_refractions, 4 ) );

    m_params.emplace_back( new PARAM<int>( "render.raytrace_recursivelevel_reflections",
                                           &m_Render.raytrace_recursivelevel_reflections, 3 ) );
    m_params.emplace_back( new PARAM<int>( "render.raytrace_recursivelevel_refractions",
                                           &m_Render.raytrace_recursivelevel_refractions, 2 ) );

    m_params.emplace_back( new PARAM<float>( "render.raytrace_spread_shadows",
                                             &m_Render.raytrace_spread_shadows, 0.05f ) );
    m_params.emplace_back( new PARAM<float>( "render.raytrace_spread_reflections",
                                             &m_Render.raytrace_spread_reflections, 0.025f ) );
    m_params.emplace_back( new PARAM<float>( "render.raytrace_spread_refractions",
                                             &m_Render.raytrace_spread_refractions, 0.025f ) );

    m_params.emplace_back( new PARAM<COLOR4D>( "render.raytrace_lightColorCamera",
                                               &m_Render.raytrace_lightColorCamera,
                                               COLOR4D( 0.2, 0.2, 0.2, 1.0 ) ) );

    m_params.emplace_back( new PARAM<COLOR4D>( "render.raytrace_lightColorTop",
                                               &m_Render.raytrace_lightColorTop,
                                               COLOR4D( 0.247, 0.247, 0.247, 1.0 ) ) );

    m_params.emplace_back( new PARAM<COLOR4D>( "render.raytrace_lightColorBottom",
                                               &m_Render.raytrace_lightColorBottom,
                                               COLOR4D( 0.247, 0.247, 0.247, 1.0 ) ) );

    std::vector<COLOR4D> default_colors =
    {
            COLOR4D( 0.168, 0.168, 0.168, 1.0 ),
            COLOR4D( 0.168, 0.168, 0.168, 1.0 ),
            COLOR4D( 0.168, 0.168, 0.168, 1.0 ),
            COLOR4D( 0.168, 0.168, 0.168, 1.0 ),
            COLOR4D( 0.168, 0.168, 0.168, 1.0 ),
            COLOR4D( 0.168, 0.168, 0.168, 1.0 ),
            COLOR4D( 0.168, 0.168, 0.168, 1.0 ),
            COLOR4D( 0.168, 0.168, 0.168, 1.0 )
    };

    m_params.emplace_back( new PARAM_LIST<COLOR4D>( "render.raytrace_lightColor",
                                                    &m_Render.raytrace_lightColor,
                                                    std::move( default_colors ) ) );

    const std::vector<int> default_elevation =
    {
        67,  67,  67,  67, -67, -67, -67, -67,
    };

    m_params.emplace_back( new PARAM_LIST<int>( "render.raytrace_lightElevation",
                                                &m_Render.raytrace_lightElevation,
                                                std::move( default_elevation ) ) );

    const std::vector<int> default_azimuth =
    {
        45, 135, 225, 315, 45, 135, 225, 315,
    };

    m_params.emplace_back( new PARAM_LIST<int>( "render.raytrace_lightAzimuth",
                                                &m_Render.raytrace_lightAzimuth,
                                                std::move( default_azimuth ) ) );

    m_params.emplace_back( new PARAM<bool>( "render.show_adhesive",
                                            &m_Render.show_adhesive, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.show_navigator",
                                            &m_Render.show_navigator, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.show_board_body",
                                            &m_Render.show_board_body, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.show_plated_barrels",
                                            &m_Render.show_plated_barrels, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.show_comments",
                                            &m_Render.show_comments, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.show_drawings",
                                            &m_Render.show_drawings, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.show_eco1",
                                            &m_Render.show_eco1, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.show_eco2",
                                            &m_Render.show_eco2, true ) );

    for( int layer = 0; layer < 45; ++layer )
    {
        m_params.emplace_back( new PARAM<bool>( fmt::format( "render.show_user{}", layer + 1 ),
                                                &m_Render.show_user[layer], false ) );
    }

    m_params.emplace_back( new PARAM<bool>( "render.show_footprints_insert",
                                            &m_Render.show_footprints_insert, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.show_footprints_normal",
                                            &m_Render.show_footprints_normal, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.show_footprints_virtual",
                                            &m_Render.show_footprints_virtual, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.show_footprints_not_in_posfile",
                                            &m_Render.show_footprints_not_in_posfile, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.show_footprints_dnp",
                                            &m_Render.show_footprints_dnp, false ) );
    m_params.emplace_back( new PARAM<bool>( "render.show_silkscreen_top",
                                            &m_Render.show_silkscreen_top, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.show_silkscreen_bottom",
                                            &m_Render.show_silkscreen_bottom, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.show_soldermask_top",
                                            &m_Render.show_soldermask_top, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.show_soldermask_bottom",
                                            &m_Render.show_soldermask_bottom, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.show_solderpaste",
                                            &m_Render.show_solderpaste, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.show_copper_top",
                                            &m_Render.show_copper_bottom, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.show_copper_bottom",
                                            &m_Render.show_copper_top, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.show_zones",
                                            &m_Render.show_zones, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.show_fp_references",
                                            &m_Render.show_fp_references, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.show_fp_values",
                                            &m_Render.show_fp_values, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.show_fp_text",
                                            &m_Render.show_fp_text, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.subtract_mask_from_silk",
                                            &m_Render.subtract_mask_from_silk, false ) );
    m_params.emplace_back( new PARAM<bool>( "render.clip_silk_on_via_annulus",
                                            &m_Render.clip_silk_on_via_annuli, false ) );
    m_params.emplace_back( new PARAM<bool>( "render.plated_and_bare_copper",
                                            &m_Render.differentiate_plated_copper, false ) );
    m_params.emplace_back( new PARAM<bool>( "render.use_board_editor_copper_colors",
                                            &m_Render.use_board_editor_copper_colors, false ) );
    m_params.emplace_back( new PARAM<bool>( "render.preview_show_board_body",
                                            &m_Render.preview_show_board_body, true ) );
    m_params.emplace_back( new PARAM<bool>( "camera.animation_enabled",
                                            &m_Camera.animation_enabled, true ) );
    m_params.emplace_back( new PARAM<int>( "camera.moving_speed_multiplier",
                                           &m_Camera.moving_speed_multiplier, 3 ) );
    m_params.emplace_back( new PARAM<double>( "camera.rotation_increment",
                                              &m_Camera.rotation_increment, 10.0 ) );
    m_params.emplace_back( new PARAM<int>( "camera.projection_mode",
                                           &m_Camera.projection_mode, 1 ) );

    m_params.emplace_back( new PARAM<bool>( "use_stackup_colors",
                                            &m_UseStackupColors, true ) );
    m_params.emplace_back( new PARAM_LAYER_PRESET_3D( "layer_presets",
                                                      &m_LayerPresets ) );
    m_params.emplace_back( new PARAM<wxString>( "current_layer_preset",
                                                &m_CurrentPreset, LEGACY_PRESET_FLAG ) );

    registerMigration( 0, 1, std::bind( &EDA_3D_VIEWER_SETTINGS::migrateSchema0to1, this ) );

    registerMigration( 1, 2,
            [&]() -> bool
            {
                Set( "render.opengl_copper_thickness", false );
                return true;
            } );

    registerMigration( 2, 3,
            [&]() -> bool
            {
                if( std::optional<bool> optval = Get<bool>( "render.show_copper" ) )
                {
                    Set( "render.show_copper_top", *optval );
                    Set( "render.show_copper_bottom", *optval );
                }

                if( std::optional<bool> optval = Get<bool>( "render.show_silkscreen" ) )
                {
                    Set( "render.show_silkscreen_top", *optval );
                    Set( "render.show_silkscreen_bottom", *optval );
                }

                if( std::optional<bool> optval = Get<bool>( "render.show_soldermask" ) )
                {
                    Set( "render.show_soldermask_top", *optval );
                    Set( "render.show_soldermask_bottom", *optval );
                }

                if( std::optional<bool> optval = Get<bool>( "render.show_comments" ) )
                    Set( "render.show_drawings", *optval );

                if( std::optional<bool> optval = Get<bool>( "render.show_eco" ) )
                {
                    Set( "render.show_eco1", *optval );
                    Set( "render.show_eco2", *optval );
                }

                return true;
            } );

    registerMigration( 3, 4,
            [&]() -> bool
            {
                std::map<int, wxString> legacyColorMap;

                legacyColorMap[142] = "fp_values";
                legacyColorMap[143] = "fp_references";
                legacyColorMap[130] = "fp_text";
                legacyColorMap[466] = "background_bottom";
                legacyColorMap[467] = "background_top";
                legacyColorMap[468] = "board";
                legacyColorMap[469] = "copper";
                legacyColorMap[470] = "copper_bottom";
                legacyColorMap[471] = "silkscreen_bottom";
                legacyColorMap[472] = "silkscreen_top";
                legacyColorMap[473] = "soldermask_bottom";
                legacyColorMap[474] = "soldermask_top";
                legacyColorMap[475] = "solderpaste";
                legacyColorMap[476] = "adhesive";
                legacyColorMap[477] = "user_comments";
                legacyColorMap[478] = "user_drawings";
                legacyColorMap[479] = "user_eco1";
                legacyColorMap[480] = "user_eco2";
                legacyColorMap[481] = "th_models";
                legacyColorMap[482] = "smd_models";
                legacyColorMap[483] = "virtual_models";
                legacyColorMap[484] = "non_pos_file_models";
                legacyColorMap[485] = "dnp_models";
                legacyColorMap[486] = "3d_navigator";
                legacyColorMap[487] = "bounding_boxes";
                legacyColorMap[488] = "off_board_silk";

                if( !Contains( "layer_presets" ) || !At( "layer_presets" ).is_array() )
                    return true;

                for( nlohmann::json& preset : At( "layer_presets" ) )
                {
                    if( preset.contains( "colors" ) && preset.at( "colors" ).is_array() )
                    {
                        for( nlohmann::json& color : preset.at( "colors" ) )
                        {
                            if( color.contains( "layer" ) && color.at( "layer" ).is_number_integer() )
                                color["layer"] = legacyColorMap[color["layer"].get<int>()];
                        }
                    }

                    if( preset.contains( "layers" ) && preset.at( "layers" ).is_array() )
                    {
                        nlohmann::json mappedLayers = nlohmann::json::array();

                        for( const nlohmann::json& layer : preset.at( "layers" ) )
                        {
                            if( layer.is_number_integer() )
                                mappedLayers.push_back( legacyColorMap[layer.get<int>()] );
                        }

                        preset["layers"] = mappedLayers;
                    }
                }

                return true;
            } );
}


LAYER_PRESET_3D* EDA_3D_VIEWER_SETTINGS::FindPreset( const wxString& aName )
{
    for( LAYER_PRESET_3D& preset : m_LayerPresets )
    {
        if( preset.name == aName )
            return &preset;
    }

    return nullptr;
}


bool EDA_3D_VIEWER_SETTINGS::migrateSchema0to1()
{
    /**
     * Schema version 0 to 1:
     *
     * delete colors (they're now stored in the 'user' color theme.
     */
    try
    {
        if( m_internals->contains( "colors" ) )
            m_internals->erase( "colors" );
    }
    catch( ... )
    {
    }

    return true;
}


bool EDA_3D_VIEWER_SETTINGS::MigrateFromLegacy( wxConfigBase* aCfg )
{
    bool ret = APP_SETTINGS_BASE::MigrateFromLegacy( aCfg );

    ret &= fromLegacy<int>( aCfg,  "RenderEngine",              "render.engine" );
    ret &= fromLegacy<int>( aCfg,  "ShowGrid3D",                "render.grid_type" );
    ret &= fromLegacy<int>( aCfg,  "Render_Material",           "render.material_mode" );
    ret &= fromLegacy<bool>( aCfg, "Render_OGL_ShowCopperThickness",
                             "render.opengl_copper_thickness" );
    ret &= fromLegacy<bool>( aCfg, "Render_OGL_ShowModelBoudingBoxes",
                             "render.opengl_show_model_bbox" );
    ret &= fromLegacy<bool>( aCfg, "Render_RAY_AntiAliasing",   "render.raytrace_anti_aliasing" );
    ret &= fromLegacy<bool>( aCfg, "Render_RAY_Backfloor",      "render.raytrace_backfloor" );
    ret &= fromLegacy<bool>( aCfg, "Render_RAY_PostProcess",    "render.raytrace_post_processing" );
    ret &= fromLegacy<bool>( aCfg, "Render_RAY_ProceduralTextures",
                             "render.raytrace_procedural_textures" );
    ret &= fromLegacy<bool>( aCfg, "Render_RAY_Reflections",    "render.raytrace_reflections" );
    ret &= fromLegacy<bool>( aCfg, "Render_RAY_Refractions",    "render.raytrace_refractions" );
    ret &= fromLegacy<bool>( aCfg, "Render_RAY_Shadows",        "render.raytrace_shadows" );
    ret &= fromLegacy<bool>( aCfg, "ShowRealisticMode",         "render.realistic" );
    ret &= fromLegacy<bool>( aCfg, "ShowAdhesiveLayers",        "render.show_adhesive" );
    ret &= fromLegacy<bool>( aCfg, "ShowNavigator",             "render.show_navigator" );
    ret &= fromLegacy<bool>( aCfg, "ShowBoardBody",             "render.show_board_body" );
    ret &= fromLegacy<bool>( aCfg, "ShowCommentsLayers",        "render.show_comments" );
    ret &= fromLegacy<bool>( aCfg, "ShowEcoLayers",             "render.show_eco" );
    ret &= fromLegacy<bool>( aCfg, "ShowFootprints_Insert",     "render.show_footprints_insert" );
    ret &= fromLegacy<bool>( aCfg, "ShowFootprints_Normal",     "render.show_footprints_normal" );
    ret &= fromLegacy<bool>( aCfg, "ShowFootprints_Virtual",    "render.show_footprints_virtual" );
    ret &= fromLegacy<bool>( aCfg, "ShowSilkScreenLayers",      "render.show_silkscreen" );
    ret &= fromLegacy<bool>( aCfg, "ShowSolderMasLayers",       "render.show_soldermask" );
    ret &= fromLegacy<bool>( aCfg, "ShowSolderPasteLayers",     "render.show_solderpaste" );
    ret &= fromLegacy<bool>( aCfg, "ShowZones",                 "render.show_zones" );
    ret &= fromLegacy<bool>( aCfg, "SubtractMaskFromSilk",      "render.subtract_mask_from_silk" );

    auto do_color =
            [&] ( const std::string& key_r, const std::string& key_g, const std::string& key_b,
                  std::string key_dest, double alpha = 1.0 )
            {
                COLOR4D color( 1, 1, 1, alpha );

                if( aCfg->Read( key_r, &color.r )
                 && aCfg->Read( key_g, &color.g )
                 && aCfg->Read( key_b, &color.b ) )
                {
                    Set( key_dest, color );
                }
            };

    do_color( "BgColor_Red", "BgColor_Green", "BgColor_Blue", "colors.background_bottom" );
    do_color( "BgColor_Red_Top", "BgColor_Green_Top", "BgColor_Blue_Top", "colors.background_top" );
    do_color( "BoardBodyColor_Red", "BoardBodyColor_Green", "BoardBodyColor_Blue", "colors.board" );
    do_color( "CopperColor_Red", "CopperColor_Green", "CopperColor_Blue", "colors.copper" );
    do_color( "SilkColor_Red", "SilkColor_Green", "SilkColor_Blue", "colors.silkscreen_bottom" );
    do_color( "SilkColor_Red", "SilkColor_Green", "SilkColor_Blue", "colors.silkscreen_top" );
    do_color( "SMaskColor_Red", "SMaskColor_Green", "SMaskColor_Blue", "colors.soldermask", 0.83 );
    do_color( "SPasteColor_Red", "SPasteColor_Green", "SPasteColor_Blue", "colors.solderpaste" );

    return ret;
}
