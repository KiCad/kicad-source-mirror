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

#ifndef _APP_SETTINGS_H
#define _APP_SETTINGS_H

#include <gal/color4d.h>
#include <nlohmann/json_fwd.hpp>
#include <settings/json_settings.h>
#include <settings/grid_settings.h>

/**
 * Cross-probing behavior.
 */
struct KICOMMON_API CROSS_PROBING_SETTINGS
{
    bool on_selection;    ///< Synchronize the selection for multiple items too.
    bool center_on_items; ///< Automatically pan to cross-probed items.
    bool zoom_to_fit;     ///< Zoom to fit items (ignored if center_on_items is off).
    bool auto_highlight;  ///< Automatically turn on highlight mode in the target frame.
    bool flash_selection; ///< Flash newly cross-probed selection (visual attention aid).
};

namespace KIGFX
{
    enum class CROSS_HAIR_MODE : int;
}

/**
 * Common cursor settings, available to every frame.
 */
struct KICOMMON_API CURSOR_SETTINGS
{
    bool always_show_cursor;
    KIGFX::CROSS_HAIR_MODE cross_hair_mode;
};

/**
* Settings for arc editing. Used by pcbnew and footprint editor
*/
enum class ARC_EDIT_MODE
{
    /**
     * When editing endpoints, the angle and radius are adjusted.
     * When editing the midpoint, the radius is adjusted.
     * When editing the center, the arcs is moved
     */
    KEEP_CENTER_ADJUST_ANGLE_RADIUS,
    /**
     * Whe editing endpoints, the other end remains in place.
     * When editing the midpoint, both endpoints remain in place.
     * When editing the center, the endpoints remain in place.
     */
    KEEP_ENDPOINTS_OR_START_DIRECTION,
    /**
     * When editing endpoints, only the angle is adjusted.
     * The midpoint adjusts only radius.
     * When editing the center, the arc is moved.
     */
    KEEP_CENTER_ENDS_ADJUST_ANGLE,
};

/**
* Store the window positioning/state.
*/
struct KICOMMON_API WINDOW_STATE
{
    bool maximized;
    int size_x;
    int size_y;
    int pos_x;
    int pos_y;
    unsigned int display;
};

/**
 * Store the common settings that are saved and loaded for each window / frame.
 */
struct KICOMMON_API WINDOW_SETTINGS
{
    WINDOW_STATE state;
    wxString mru_path;
    wxString perspective;
    nlohmann::json aui_state;
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
class KICOMMON_API APP_SETTINGS_BASE : public JSON_SETTINGS
{
public:
    struct FIND_REPLACE
    {
        wxString              find_string;
        std::vector<wxString> find_history;
        wxString              replace_string;
        std::vector<wxString> replace_history;

        bool search_and_replace;

        bool match_case;
        int match_mode;
    };

    struct SEARCH_PANE
    {
        enum class SELECTION_ZOOM
        {
            NONE,
            PAN,
            ZOOM,
        };

        SELECTION_ZOOM selection_zoom;
        bool           search_hidden_fields;
        bool           search_metadata;
    };

    struct GRAPHICS
    {
        float highlight_factor;         ///< How much to brighten highlighted objects by.
        float select_factor;            ///< How much to brighten selected objects by.
    };

    struct COLOR_PICKER
    {
        int default_tab;
    };

    struct LIB_TREE
    {
        std::vector<wxString> columns;         ///< Ordered list of visible columns in the tree.
        std::map<wxString, int> column_widths; ///< Column widths, keyed by header name.
        std::vector<wxString>   open_libs;     ///< list of libraries the user has open in the tree.
    };

    struct PANEL_DESIGN_BLOCK_CHOOSER
    {
        int  sash_pos_h;
        int  sash_pos_v;
        int  width;
        int  height;
        int  sort_mode;
        bool repeated_placement;
        bool place_as_sheet;
        bool place_as_group;
        bool keep_annotations;

        // For saving tree columns and widths
        LIB_TREE tree;
    };

    struct PRINTING
    {
        bool             background;             ///< Whether or not to print background color.
        bool             monochrome;             ///< Whether or not to print in monochrome.
        double           scale;                  ///< Printout scale.
        bool             use_theme;              ///< If false, display color theme will be used.
        wxString         color_theme;            ///< Color theme to use for printing.
        bool             title_block;            ///< Whether or not to print title block.
        std::vector<int> layers;                 ///< List of enabled layers for printing.
        bool             mirror;                 ///< Print mirrored.
        int              drill_marks;            ///< Drill marks type (0=none, 1=small, 2=real).
        int              pagination;             ///< 0=all layers on one page, 1=one page per layer.
        bool             edge_cuts_on_all_pages; ///< Print board edges on all pages.
        bool             as_item_checkboxes;     ///< Honor checkboxes in appearance manager.
    };

    struct SYSTEM
    {
        bool                  first_run_shown; //@todo RFB remove? - not used
        int                   max_undo_items;
        std::vector<wxString> file_history;
        int                   units;
        int                   last_metric_units;
        int                   last_imperial_units;

        /// Stored value for "show import issues" when importing non-KiCad designs to this
        /// application.
        bool                  show_import_issues;
    };

    struct PLUGINS
    {
        /// Ordered list of plugin actions mapped to whether or not they are shown in the toolbar.
        std::vector<std::pair<wxString, bool>> actions;
    };

    APP_SETTINGS_BASE( const std::string& aFilename, int aSchemaVersion );

    virtual ~APP_SETTINGS_BASE() {}

    virtual bool MigrateFromLegacy( wxConfigBase* aCfg ) override;

    const std::vector<GRID> DefaultGridSizeList() const;

    const std::vector<double> DefaultZoomList() const;

public:
    CROSS_PROBING_SETTINGS m_CrossProbing;

    FIND_REPLACE m_FindReplace;

    PANEL_DESIGN_BLOCK_CHOOSER m_DesignBlockChooserPanel;

    GRAPHICS m_Graphics;

    COLOR_PICKER m_ColorPicker;

    LIB_TREE m_LibTree;

    PRINTING m_Printing;

    SEARCH_PANE m_SearchPane;

    SYSTEM m_System;

    PLUGINS m_Plugins;

    WINDOW_SETTINGS m_Window;

    /// Active color theme name.
    wxString m_ColorTheme;

    /// Use custom toolbars
    bool m_CustomToolbars;


    ///! Local schema version for common app settings.
    int m_appSettingsSchemaVersion;

protected:

    virtual std::string getLegacyFrameName() const { return std::string(); }

    ///! Migrates the find/replace history string list.s
    void migrateFindReplace( wxConfigBase* aCfg );

    /**
     * Migrate legacy window settings into the JSON document.
     *
     * @param aCfg is the wxConfig object to read from
     * @param aFrameName is the prefix for window settings in the legacy config file
     * @param aJsonPath is the prefix for storing window settings in the JSON file
     * @return true if all settings were migrated
     */
    bool migrateWindowConfig( wxConfigBase* aCfg, const std::string& aFrameName,
                              const std::string& aJsonPath );

    /**
     * Add parameters for the given window object.
     *
     * @param aWindow is the target window settings object.
     * @param aJsonPath is the path to read parameters from.
     */
    void addParamsForWindow( WINDOW_SETTINGS* aWindow, const std::string& aJsonPath,
                             int aDefaultWidth = 0, int aDefaultHeight = 0 );

    /**
     * Migrate the library tree width setting from a single column (Item) to multi-column.
     */
    bool migrateLibTreeWidth();
};

#endif
