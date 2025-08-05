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

#ifndef EDA_3D_VIEWER_SETTINGS_H_
#define EDA_3D_VIEWER_SETTINGS_H_

#include <3d_enums.h>
#include <plugins/3dapi/xv3d_types.h>
#include <settings/app_settings.h>
#include <settings/parameters.h>
#include <project/board_project_settings.h>
#include "render_settings.h"

#define FOLLOW_PCB           wxT( "follow_pcb_editor" )
#define FOLLOW_PLOT_SETTINGS wxT( "follow_plot_settings" )
#define LEGACY_PRESET_FLAG   wxT( "legacy_preset_flag" )

enum class ANTIALIASING_MODE;


struct LAYER_PRESET_3D
{
    LAYER_PRESET_3D( const wxString& aName = wxEmptyString );

    LAYER_PRESET_3D( const wxString& aName, const std::bitset<LAYER_3D_END>& aLayers,
                     const std::map<int, KIGFX::COLOR4D>& aColors ) :
            name( aName ),
            layers( aLayers ),
            colors( aColors )
    {
    }

    wxString                      name;          ///< A name for this layer set
    std::bitset<LAYER_3D_END>     layers;
    std::map<int, KIGFX::COLOR4D> colors;
};


class PARAM_LAYER_PRESET_3D : public PARAM_LAMBDA<nlohmann::json>
{
public:
    PARAM_LAYER_PRESET_3D( const std::string& aPath, std::vector<LAYER_PRESET_3D>* aPresetList );

private:
    nlohmann::json presetsToJson();

    void jsonToPresets( const nlohmann::json& aJson );

private:
    std::vector<LAYER_PRESET_3D>* m_presets;

    std::map<int, wxString>       m_layerToLayerNameMap;
    std::map<wxString, int>       m_layerNameToLayerMap;
};


class EDA_3D_VIEWER_SETTINGS : public APP_SETTINGS_BASE
{
public:
    struct AUI_PANELS
    {
        int  right_panel_width;
        bool show_layer_manager;
    };

    struct RENDER_SETTINGS
    {
        RENDER_ENGINE     engine;
        GRID3D_TYPE       grid_type;
        ANTIALIASING_MODE opengl_AA_mode;
        MATERIAL_MODE     material_mode;

        bool opengl_AA_disableOnMove;
        bool opengl_thickness_disableOnMove;
        bool opengl_microvias_disableOnMove;
        bool opengl_holes_disableOnMove;
        bool opengl_render_bbox_only_OnMove;
        bool opengl_copper_thickness;
        bool show_model_bbox;
        bool show_off_board_silk;
        bool highlight_on_rollover;
        KIGFX::COLOR4D opengl_selection_color;

        bool raytrace_anti_aliasing;
        bool raytrace_backfloor;
        bool raytrace_post_processing;
        bool raytrace_procedural_textures;
        bool raytrace_reflections;
        bool raytrace_refractions;
        bool raytrace_shadows;

        int raytrace_nrsamples_shadows;
        int raytrace_nrsamples_reflections;
        int raytrace_nrsamples_refractions;

        float raytrace_spread_shadows;
        float raytrace_spread_reflections;
        float raytrace_spread_refractions;

        int raytrace_recursivelevel_reflections;
        int raytrace_recursivelevel_refractions;

        KIGFX::COLOR4D raytrace_lightColorCamera;
        KIGFX::COLOR4D raytrace_lightColorTop;
        KIGFX::COLOR4D raytrace_lightColorBottom;
        std::vector<KIGFX::COLOR4D> raytrace_lightColor;
        std::vector<int> raytrace_lightElevation;   // -90 .. 90
        std::vector<int> raytrace_lightAzimuth;     // 0 .. 359

        bool show_adhesive;
        bool show_navigator;
        bool show_board_body;
        bool show_plated_barrels;
        bool show_comments;
        bool show_drawings;
        bool show_eco1;
        bool show_eco2;
        bool show_user[45];
        bool show_footprints_insert;
        bool show_footprints_normal;
        bool show_footprints_virtual;
        bool show_footprints_not_in_posfile;
        bool show_footprints_dnp;
        bool show_silkscreen_top;
        bool show_silkscreen_bottom;
        bool show_soldermask_top;
        bool show_soldermask_bottom;
        bool show_solderpaste;
        bool show_copper_top;
        bool show_copper_bottom;
        bool show_zones;
        bool show_fp_references;
        bool show_fp_values;
        bool show_fp_text;
        bool subtract_mask_from_silk;
        bool clip_silk_on_via_annuli;
        bool differentiate_plated_copper;
        bool use_board_editor_copper_colors;    // OpenGL only
        bool preview_show_board_body;

        /**
         * return true if platted copper aeras and non platted copper areas must be drawn
         * using a different color
         */
        bool DifferentiatePlatedCopper()
        {
            if( engine == RENDER_ENGINE::OPENGL && use_board_editor_copper_colors )
                return false;

            return differentiate_plated_copper;
        }
    };

    struct CAMERA_SETTINGS
    {
        bool   animation_enabled;
        int    moving_speed_multiplier;
        double rotation_increment;
        int    projection_mode;
    };

    EDA_3D_VIEWER_SETTINGS();

    virtual ~EDA_3D_VIEWER_SETTINGS() {}

    LAYER_PRESET_3D* FindPreset( const wxString& aName );

    virtual bool MigrateFromLegacy( wxConfigBase* aLegacyConfig ) override;

public:
    AUI_PANELS      m_AuiPanels;
    RENDER_SETTINGS m_Render;
    CAMERA_SETTINGS m_Camera;

    bool                         m_UseStackupColors;
    std::vector<LAYER_PRESET_3D> m_LayerPresets;
    wxString                     m_CurrentPreset;

protected:
    virtual std::string getLegacyFrameName() const override { return "Viewer3DFrameName"; }

private:
    bool migrateSchema0to1();
};


#endif
