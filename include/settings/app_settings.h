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

#ifndef _APP_SETTINGS_H
#define _APP_SETTINGS_H

#include <gal/color4d.h>
#include <settings/json_settings.h>

/**
 * Cross-probing behavior
 */
struct CROSS_PROBING_SETTINGS
{
    bool center_on_items; ///< Automatically pan to cross-probed items
    bool zoom_to_fit;     ///< Zoom to fit items (ignored if center_on_items is off)
    bool auto_highlight;  ///< Automatically turn on highlight mode in the target frame
};

/**
 * Common cursor settings, available to every frame
 */
struct CURSOR_SETTINGS
{
    bool always_show_cursor;
    bool fullscreen_cursor;
};

/**
 * Common grid settings, available to every frame
 */
struct GRID_SETTINGS
{
    bool axes_enabled;
    std::vector<wxString> sizes;
    wxString user_grid_x;
    wxString user_grid_y;
    int last_size_idx;
    int fast_grid_1;
    int fast_grid_2;
    double line_width;
    double min_spacing;
    bool show;
    int style;
    int snap;
};

/**
* Stores the window positioning/state
*/
struct WINDOW_STATE
{
    bool maximized;
    int size_x;
    int size_y;
    int pos_x;
    int pos_y;
    unsigned int display;
};

/**
 * Stores the common settings that are saved and loaded for each window / frame
 */
struct WINDOW_SETTINGS
{
    WINDOW_STATE state;
    wxString mru_path;
    wxString perspective;
    std::vector<double> zoom_factors;

    CURSOR_SETTINGS cursor;
    GRID_SETTINGS grid;
};

/**
 * APP_SETTINGS_BASE is a settings class that should be derived for each standalone KiCad
 * application.  It stores settings that should exist for every app, but may be different from
 * app to app depending on the user's preferences.
 *
 * COMMON_SETTINGS stores settings that are always the same across all applications.
 */
class APP_SETTINGS_BASE : public JSON_SETTINGS
{
public:


    struct FIND_REPLACE
    {
        int                   flags;
        wxString              find_string;
        std::vector<wxString> find_history;
        wxString              replace_string;
        std::vector<wxString> replace_history;
    };

    struct GRAPHICS
    {
        int   canvas_type;
        float highlight_factor;         ///< How much to brighten highlighted objects by
        float select_factor;            ///< How much to brighten selected objects by
        float high_contrast_factor;     ///< How much to darken inactive layers by
    };

    struct COLOR_PICKER
    {
        int default_tab;
    };

    struct LIB_TREE
    {
        int column_width;
    };

    struct PRINTING
    {
        bool             background;    ///< Whether or not to print background color
        bool             monochrome;    ///< Whether or not to print in monochrome
        double           scale;         ///< Printout scale
        bool             use_theme;     ///< If false, display color theme will be used
        wxString         color_theme;   ///< Color theme to use for printing
        bool             title_block;   ///< Whether or not to print title block
        std::vector<int> layers;        ///< List of enabled layers for printing
    };

    struct SYSTEM
    {
        bool                  first_run_shown;
        int                   max_undo_items;
        std::vector<wxString> file_history;
        int                   units;
        int                   last_metric_units;
        int                   last_imperial_units;
    };

    APP_SETTINGS_BASE( const std::string& aFilename, int aSchemaVersion );

    virtual ~APP_SETTINGS_BASE() {}

    virtual bool MigrateFromLegacy( wxConfigBase* aCfg ) override;

    /// Override in child classes to define the default for the window.grid.sizes parameter
    virtual const std::vector<wxString> DefaultGridSizeList() const { return {}; }

public:
    CROSS_PROBING_SETTINGS m_CrossProbing;

    FIND_REPLACE m_FindReplace;

    GRAPHICS m_Graphics;

    COLOR_PICKER m_ColorPicker;

    LIB_TREE m_LibTree;

    PRINTING m_Printing;

    SYSTEM m_System;

    WINDOW_SETTINGS m_Window;

    /// Active color theme name
    wxString m_ColorTheme;

    ///! Local schema version for common app settings
    int m_appSettingsSchemaVersion;

protected:

    virtual std::string getLegacyFrameName() const { return std::string(); }

    ///! Migrates the find/replace history string lists
    void migrateFindReplace( wxConfigBase* aCfg );

    /**
     * Migrates legacy window settings into the JSON document
     * @param aCfg is the wxConfig object to read from
     * @param aFrameName is the prefix for window settings in the legacy config file
     * @param aJsonPath is the prefix for storing window settings in the JSON file
     * @return true if all settings were migrated
     */
    bool migrateWindowConfig( wxConfigBase* aCfg, const std::string& aFrameName,
            const std::string& aJsonPath );

    /**
     * Adds parameters for the given window object
     * @param aWindow is the target window settings object
     * @param aJsonPath is the path to read parameters from
     */
    void addParamsForWindow( WINDOW_SETTINGS* aWindow, const std::string& aJsonPath );
};

#endif
