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

#include <3d_enums.h>
#include <common_ogl/ogl_attr_list.h>
#include <settings/parameters.h>
#include <wx/config.h>

#include "eda_3d_viewer_settings.h"

using KIGFX::COLOR4D;

///! Update the schema version whenever a migration is required
const int viewer3dSchemaVersion = 0;


EDA_3D_VIEWER_SETTINGS::EDA_3D_VIEWER_SETTINGS()
        : APP_SETTINGS_BASE( "3d_viewer", viewer3dSchemaVersion ),
          m_Render(),
          m_Camera()
{
    m_params.emplace_back( new PARAM<int>( "render.engine", &m_Render.engine,
            static_cast<int>( RENDER_ENGINE::OPENGL_LEGACY ),
            static_cast<int>( RENDER_ENGINE::OPENGL_LEGACY ),
            static_cast<int>( RENDER_ENGINE::RAYTRACING ) ) );

    m_params.emplace_back( new PARAM<int>( "render.grid_type", &m_Render.grid_type,
            static_cast<int>( GRID3D_TYPE::NONE ),
            static_cast<int>( GRID3D_TYPE::NONE ),
            static_cast<int>( GRID3D_TYPE::GRID_10MM ) ) );

    m_params.emplace_back( new PARAM<int>( "render.material_mode", &m_Render.material_mode,
            static_cast<int>( MATERIAL_MODE::NORMAL ),
            static_cast<int>( MATERIAL_MODE::NORMAL ),
            static_cast<int>( MATERIAL_MODE::CAD_MODE ) ) );

    m_params.emplace_back( new PARAM<int>( "render.opengl_AA_mode", &m_Render.opengl_AA_mode,
           static_cast<int>( ANTIALIASING_MODE::AA_8X ),
           static_cast<int>( ANTIALIASING_MODE::AA_NONE ),
           static_cast<int>( ANTIALIASING_MODE::AA_8X ) ) );

    m_params.emplace_back( new PARAM<COLOR4D>( "render.opengl_selection_color",
                                               &m_Render.opengl_selection_color,
                                               COLOR4D( 0.0, 1.0, 0.0, 1.0 ) ) );

    // OpenGL options
    m_params.emplace_back( new PARAM<bool>(
            "render.opengl_copper_thickness", &m_Render.opengl_copper_thickness, true ) );
    m_params.emplace_back( new PARAM<bool>(
            "render.opengl_show_model_bbox", &m_Render.opengl_show_model_bbox, false ) );
    m_params.emplace_back( new PARAM<bool>(
            "render.opengl_highlight_on_rollover", &m_Render.opengl_highlight_on_rollover, true ) );
    m_params.emplace_back( new PARAM<bool>(
            "render.opengl_AA_disableOnMove", &m_Render.opengl_AA_disableOnMove, false ) );
    m_params.emplace_back( new PARAM<bool>(
            "render.opengl_thickness_disableOnMove", &m_Render.opengl_thickness_disableOnMove, false ) );
    m_params.emplace_back( new PARAM<bool>(
            "render.opengl_vias_disableOnMove", &m_Render.opengl_vias_disableOnMove, false ) );
    m_params.emplace_back( new PARAM<bool>(
            "render.opengl_holes_disableOnMove", &m_Render.opengl_holes_disableOnMove, false ) );
    m_params.emplace_back( new PARAM<bool>(
            "render.opengl_render_bbox_only_OnMove", &m_Render.opengl_render_bbox_only_OnMove, false ) );

    // Raytracing options
    m_params.emplace_back( new PARAM<bool>( "render.raytrace_anti_aliasing",
            &m_Render.raytrace_anti_aliasing, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.raytrace_backfloor",
            &m_Render.raytrace_backfloor, false ) );
    m_params.emplace_back( new PARAM<bool>( "render.raytrace_post_processing",
            &m_Render.raytrace_post_processing, true ) );
    m_params.emplace_back(  new PARAM<bool>( "render.raytrace_procedural_textures",
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
                                                    default_colors ) );

    const std::vector<int> default_elevation =
    {
        67,  67,  67,  67, -67, -67, -67, -67,
    };

    m_params.emplace_back( new PARAM_LIST<int>( "render.raytrace_lightElevation",
                                                &m_Render.raytrace_lightElevation,
                                                default_elevation ) );

    const std::vector<int> default_azimuth =
    {
        45, 135, 225, 315, 45, 135, 225, 315,
    };

    m_params.emplace_back( new PARAM_LIST<int>( "render.raytrace_lightAzimuth",
                                                &m_Render.raytrace_lightAzimuth,
                                                default_azimuth ) );

    m_params.emplace_back( new PARAM<bool>( "render.realistic", &m_Render.realistic, true ) );
    m_params.emplace_back(
            new PARAM<bool>( "render.show_adhesive", &m_Render.show_adhesive, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.show_axis", &m_Render.show_axis, true ) );
    m_params.emplace_back(
            new PARAM<bool>( "render.show_board_body", &m_Render.show_board_body, true ) );
    m_params.emplace_back(
            new PARAM<bool>( "render.show_comments", &m_Render.show_comments, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.show_eco", &m_Render.show_eco, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.show_footprints_insert",
            &m_Render.show_footprints_insert, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.show_footprints_normal",
            &m_Render.show_footprints_normal, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.show_footprints_virtual",
            &m_Render.show_footprints_virtual, true ) );
    m_params.emplace_back(
            new PARAM<bool>( "render.show_silkscreen", &m_Render.show_silkscreen, true ) );
    m_params.emplace_back(
            new PARAM<bool>( "render.show_soldermask", &m_Render.show_soldermask, true ) );
    m_params.emplace_back(
            new PARAM<bool>( "render.show_solderpaste", &m_Render.show_solderpaste, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.show_zones", &m_Render.show_zones, true ) );
    m_params.emplace_back( new PARAM<bool>( "render.subtract_mask_from_silk",
            &m_Render.subtract_mask_from_silk, false ) );
    m_params.emplace_back( new PARAM<bool>( "render.clip_silk_on_via_annulus",
            &m_Render.clip_silk_on_via_annulus, false ) );
    m_params.emplace_back( new PARAM<bool>( "render.plated_and_bare_copper",
            &m_Render.renderPlatedPadsAsPlated, false ) );
    m_params.emplace_back( new PARAM<bool>( "camera.animation_enabled",
            &m_Camera.animation_enabled, true ) );
    m_params.emplace_back( new PARAM<int>( "camera.moving_speed_multiplier",
            &m_Camera.moving_speed_multiplier, 3 ) );
    m_params.emplace_back( new PARAM<double>( "camera.rotation_increment",
            &m_Camera.rotation_increment, 10.0 ) );
    m_params.emplace_back( new PARAM<int>( "camera.projection_mode",
            &m_Camera.projection_mode, 1 ) );

}


bool EDA_3D_VIEWER_SETTINGS::MigrateFromLegacy( wxConfigBase* aCfg )
{
    bool ret = APP_SETTINGS_BASE::MigrateFromLegacy( aCfg );

    ret &= fromLegacy<int>( aCfg, "RenderEngine",       "render.engine" );
    ret &= fromLegacy<int>( aCfg, "ShowGrid3D",         "render.grid_type" );
    ret &= fromLegacy<int>( aCfg, "Render_Material",    "render.material_mode" );
    ret &= fromLegacy<bool>(
            aCfg, "Render_OGL_ShowCopperThickness",     "render.opengl_copper_thickness" );
    ret &= fromLegacy<bool>(
            aCfg, "Render_OGL_ShowModelBoudingBoxes", "render.opengl_show_model_bbox" );
    ret &= fromLegacy<bool>( aCfg, "Render_RAY_AntiAliasing",   "render.raytrace_anti_aliasing" );
    ret &= fromLegacy<bool>( aCfg, "Render_RAY_Backfloor",      "render.raytrace_backfloor" );
    ret &= fromLegacy<bool>( aCfg, "Render_RAY_PostProcess",    "render.raytrace_post_processing" );
    ret &= fromLegacy<bool>(
            aCfg, "Render_RAY_ProceduralTextures", "render.raytrace_procedural_textures" );
    ret &= fromLegacy<bool>( aCfg, "Render_RAY_Reflections",    "render.raytrace_reflections" );
    ret &= fromLegacy<bool>( aCfg, "Render_RAY_Refractions",    "render.raytrace_refractions" );
    ret &= fromLegacy<bool>( aCfg, "Render_RAY_Shadows",        "render.raytrace_shadows" );
    ret &= fromLegacy<bool>( aCfg, "ShowRealisticMode",         "render.realistic" );
    ret &= fromLegacy<bool>( aCfg, "ShowAdhesiveLayers",        "render.show_adhesive" );
    ret &= fromLegacy<bool>( aCfg, "ShowAxis",                  "render.show_axis" );
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

    auto migrate_color =
            [&] ( const std::string& k_r, const std::string& k_g, const std::string& k_b,
                  std::string destKey, double alpha = 1.0 )
            {
                COLOR4D color( 1, 1, 1, alpha );

                if( aCfg->Read( k_r, &color.r ) &&
                    aCfg->Read( k_g, &color.g ) && aCfg->Read( k_b, &color.b ) )
                {
                    Set( destKey, color );
                }
            };

    migrate_color( "BgColor_Red", "BgColor_Green", "BgColor_Blue", "colors.background_bottom" );
    migrate_color(
            "BgColor_Red_Top", "BgColor_Green_Top", "BgColor_Blue_Top", "colors.background_top" );
    migrate_color(
            "BoardBodyColor_Red", "BoardBodyColor_Green", "BoardBodyColor_Blue", "colors.board" );
    migrate_color( "CopperColor_Red", "CopperColor_Green", "CopperColor_Blue", "colors.copper" );
    migrate_color(
            "SilkColor_Red", "SilkColor_Green", "SilkColor_Blue", "colors.silkscreen_bottom" );
    migrate_color( "SilkColor_Red", "SilkColor_Green", "SilkColor_Blue", "colors.silkscreen_top" );
    migrate_color( "SMaskColor_Red", "SMaskColor_Green", "SMaskColor_Blue", "colors.soldermask", 0.83 );
    migrate_color(
            "SPasteColor_Red", "SPasteColor_Green", "SPasteColor_Blue", "colors.solderpaste" );

    return ret;
}
