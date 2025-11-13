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

#pragma once

#include <settings/environment.h>
#include <settings/json_settings.h>


enum class MOUSE_DRAG_ACTION
{
    // WARNING: these are encoded as integers in the file, so don't change their values.
    DRAG_ANY = -2,
    DRAG_SELECTED,
    SELECT,
    ZOOM,
    PAN,
    NONE
};

enum class ICON_THEME
{
    LIGHT,
    DARK,
    AUTO
};


class KICOMMON_API COMMON_SETTINGS : public JSON_SETTINGS
{
public:
    struct APPEARANCE
    {
        bool       show_scrollbars;
        double     canvas_scale;
        ICON_THEME icon_theme;
        bool       use_icons_in_menus;
        bool       apply_icon_scale_to_fonts;
        double     hicontrast_dimming_factor;
        int        text_editor_zoom;
        int        toolbar_icon_size;
        bool       grid_striping;
        bool       use_custom_cursors;
        double     zoom_correction_factor;
    };

    struct AUTO_BACKUP
    {
        bool   enabled;            ///< Automatically back up the project when files are saved
        bool   backup_on_autosave; ///< Trigger a backup on autosave
        int    limit_total_files;  ///< Maximum number of backup archives to retain
        int    limit_daily_files;  ///< Maximum files to keep per day, 0 for unlimited
        int    min_interval;       ///< Minimum time, in seconds, between subsequent backups

        /// Maximum total size of backups (bytes), 0 for unlimited
        unsigned long long limit_total_size;
    };

    struct ENVIRONMENT
    {
        ENV_VAR_MAP vars;
    };

    struct INPUT
    {
        bool focus_follow_sch_pcb;
        bool auto_pan;
        int  auto_pan_acceleration;
        bool center_on_zoom;
        bool immediate_actions;
        bool warp_mouse_on_move;
        bool horizontal_pan;
        bool hotkey_feedback;

        bool zoom_acceleration;
        int  zoom_speed;
        bool zoom_speed_auto;

        int scroll_modifier_zoom;
        int scroll_modifier_pan_h;
        int scroll_modifier_pan_v;

        int motion_pan_modifier;

        MOUSE_DRAG_ACTION drag_left;
        MOUSE_DRAG_ACTION drag_middle;
        MOUSE_DRAG_ACTION drag_right;

        bool reverse_scroll_zoom;
        bool reverse_scroll_pan_h;
    };

    struct SPACEMOUSE
    {
        int  rotate_speed;
        int  pan_speed;
        bool reverse_rotate;
        bool reverse_pan_x;
        bool reverse_pan_y;
        bool reverse_zoom;
    };

    struct GRAPHICS
    {
        int canvas_type; ///< EDA_DRAW_PANEL_GAL::GAL_TYPE_* value, see gal_options_panel.cpp
        int aa_mode;
    };

    struct SESSION
    {
        bool remember_open_files;
        std::vector<wxString> pinned_symbol_libs;
        std::vector<wxString> pinned_fp_libs;
        std::vector<wxString> pinned_design_block_libs;
    };

    struct SYSTEM
    {
        bool local_history_enabled;
        int local_history_debounce;
        wxString text_editor;
        wxString file_explorer;
        int file_history_size;
        wxString language;
        wxString pdf_viewer_name;
        bool use_system_pdf_viewer;
        wxString working_dir;
        int clear_3d_cache_interval;
    };

    struct DO_NOT_SHOW_AGAIN
    {
        bool zone_fill_warning;
        bool env_var_overwrite_warning;
        bool scaled_3d_models_warning;
        bool data_collection_prompt;
        bool update_check_prompt;
    };

    struct PACKAGE_MANAGER
    {
        int sash_pos;
    };

    struct GIT_REPOSITORY
    {
        wxString name;
        wxString path;
        wxString authType;
        wxString username;
        wxString ssh_path;
        bool     active;
        bool     checkValid;
    };

    struct GIT
    {
        std::vector<GIT_REPOSITORY> repositories;
        bool                        enableGit;
        int                         updatInterval;
        bool                        useDefaultAuthor;
        wxString                    authorName;
        wxString                    authorEmail;
    };

    struct API
    {
        wxString python_interpreter;
        bool enable_server;
    };

    COMMON_SETTINGS();

    virtual ~COMMON_SETTINGS() {}

    virtual bool MigrateFromLegacy( wxConfigBase* aLegacyConfig ) override;

    /**
     * Creates the built-in environment variables and sets their default values
     */
    void InitializeEnvironment();

private:
    bool migrateSchema0to1();
    bool migrateSchema1to2();
    bool migrateSchema2to3();
    bool migrateSchema3to4();

    struct LEGACY_3D_SEARCH_PATH
    {
        wxString m_Alias;       // alias to the base path
        wxString m_Pathvar;     // base path as stored in the config file
        wxString m_Pathexp;     // expanded base path
        wxString m_Description; // description of the aliased path
    };

    static bool getLegacy3DHollerith( const std::string& aString, size_t& aIndex,
                                      wxString& aResult );
    bool readLegacy3DResolverCfg( const wxString& aPath,
                                  std::vector<LEGACY_3D_SEARCH_PATH>& aSearchPaths );

public:
    APPEARANCE        m_Appearance;
    AUTO_BACKUP       m_Backup;
    ENVIRONMENT       m_Env;
    INPUT             m_Input;
    SPACEMOUSE        m_SpaceMouse;
    GRAPHICS          m_Graphics;
    SESSION           m_Session;
    SYSTEM            m_System;
    DO_NOT_SHOW_AGAIN m_DoNotShowAgain;
    PACKAGE_MANAGER   m_PackageManager;
    GIT               m_Git;
    API               m_Api;

    /// Persistent dialog control values
    std::map<std::string, std::map<std::string, nlohmann::json>> m_dialogControlValues;
};
