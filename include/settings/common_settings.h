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

#ifndef _COMMON_SETTINGS_H
#define _COMMON_SETTINGS_H

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


class COMMON_SETTINGS : public JSON_SETTINGS
{
public:
    struct APPEARANCE
    {
        double     canvas_scale;
        int        icon_scale;
        ICON_THEME icon_theme;
        bool       use_icons_in_menus;
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
        bool auto_pan;
        int  auto_pan_acceleration;
        bool center_on_zoom;
        bool immediate_actions;
        bool warp_mouse_on_move;
        bool horizontal_pan;

        bool zoom_acceleration;
        int  zoom_speed;
        bool zoom_speed_auto;

        int scroll_modifier_zoom;
        int scroll_modifier_pan_h;
        int scroll_modifier_pan_v;

        MOUSE_DRAG_ACTION drag_left;
        MOUSE_DRAG_ACTION drag_middle;
        MOUSE_DRAG_ACTION drag_right;
    };

    struct GRAPHICS
    {
        int cairo_aa_mode;
        int opengl_aa_mode;
    };

    struct SESSION
    {
        bool remember_open_files;
    };

    struct SYSTEM
    {
        int autosave_interval;
        wxString editor_name;
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
    };

    struct NETCLASS_PANEL
    {
        int sash_pos;
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

public:
    APPEARANCE m_Appearance;

    AUTO_BACKUP m_Backup;

    ENVIRONMENT m_Env;

    INPUT m_Input;

    GRAPHICS m_Graphics;

    SESSION m_Session;

    SYSTEM m_System;

    DO_NOT_SHOW_AGAIN m_DoNotShowAgain;

    NETCLASS_PANEL m_NetclassPanel;
};

#endif
