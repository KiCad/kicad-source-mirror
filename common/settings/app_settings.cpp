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

#include <class_draw_panel_gal.h>
#include <common.h>
#include <layers_id_colors_and_visibility.h>
#include <pgm_base.h>
#include <settings/app_settings.h>
#include <settings/common_settings.h>
#include <settings/parameters.h>
#include <base_units.h>

///! Update the schema version whenever a migration is required
const int appSettingsSchemaVersion = 0;


APP_SETTINGS_BASE::APP_SETTINGS_BASE( const std::string& aFilename, int aSchemaVersion ) :
        JSON_SETTINGS( aFilename, SETTINGS_LOC::USER, aSchemaVersion ),
        m_CrossProbing(),
        m_FindReplace(),
        m_Graphics(),
        m_ColorPicker(),
        m_LibTree(),
        m_Printing(),
        m_System(),
        m_Window(),
        m_appSettingsSchemaVersion( aSchemaVersion )
{
    // Make Coverity happy:
    m_LibTree.column_width = 360;
    m_Graphics.canvas_type = EDA_DRAW_PANEL_GAL::GAL_FALLBACK;

    // Build parameters list:
    m_params.emplace_back( new PARAM<int>( "find_replace.flags", &m_FindReplace.flags, 1 ) );

    m_params.emplace_back( new PARAM<wxString>( "find_replace.find_string",
            &m_FindReplace.find_string, "" ) );

    m_params.emplace_back( new PARAM_LIST<wxString>( "find_replace.find_history",
            &m_FindReplace.find_history, {} ) );

    m_params.emplace_back( new PARAM<wxString>( "find_replace.replace_string",
            &m_FindReplace.replace_string, "" ) );

    m_params.emplace_back( new PARAM_LIST<wxString>( "find_replace.replace_history",
            &m_FindReplace.replace_history, {} ) );

    m_params.emplace_back( new PARAM<int>( "graphics.canvas_type",
            &m_Graphics.canvas_type, EDA_DRAW_PANEL_GAL::GAL_FALLBACK ) );

    m_params.emplace_back( new PARAM<float>(
            "graphics.highlight_factor", &m_Graphics.highlight_factor, 0.5f, 0.0, 1.0f ) );

    m_params.emplace_back( new PARAM<float>(
            "graphics.select_factor", &m_Graphics.select_factor, 0.75f, 0.0, 1.0f ) );

    m_params.emplace_back( new PARAM<float>(
            "graphics.high_contrast_factor", &m_Graphics.high_contrast_factor, 0.35f, 0.0, 1.0f ) );

    m_params.emplace_back( new PARAM<int>( "color_picker.default_tab",
            &m_ColorPicker.default_tab, 0 ) );

    m_params.emplace_back( new PARAM<int>( "lib_tree.column_width",
            &m_LibTree.column_width, 360 ) );

    m_params.emplace_back( new PARAM<bool>( "printing.background",
            &m_Printing.background, false ) );

    m_params.emplace_back( new PARAM<bool>( "printing.monochrome",
            &m_Printing.monochrome, true ) );

    m_params.emplace_back( new PARAM<double>( "printing.scale",
            &m_Printing.scale, 1.0 ) );

    m_params.emplace_back( new PARAM<bool>( "printing.use_theme",
            &m_Printing.use_theme, false ) );

    m_params.emplace_back( new PARAM<wxString>( "printing.color_theme",
            &m_Printing.color_theme, "" ) );

    m_params.emplace_back( new PARAM<bool>( "printing.title_block",
            &m_Printing.title_block, false ) );

    m_params.emplace_back( new PARAM_LIST<int>( "printing.layers",
            &m_Printing.layers, {} ) );

    m_params.emplace_back( new PARAM<bool>( "system.first_run_shown",
            &m_System.first_run_shown, false ) );

    m_params.emplace_back( new PARAM<int>( "system.max_undo_items",
            &m_System.max_undo_items, 0 ) );


    m_params.emplace_back( new PARAM_LIST<wxString>( "system.file_history",
            &m_System.file_history, {} ) );

    m_params.emplace_back( new PARAM<int>( "system.units",
            &m_System.units, static_cast<int>( EDA_UNITS::MILLIMETRES ) ) );

    m_params.emplace_back( new PARAM<int>( "system.last_metric_units",
            &m_System.last_metric_units, static_cast<int>( EDA_UNITS::MILLIMETRES ) ) );

    m_params.emplace_back( new PARAM<int>( "system.last_imperial_units",
            &m_System.last_imperial_units, static_cast<int>( EDA_UNITS::INCHES ) ) );

    m_params.emplace_back( new PARAM<wxString>( "appearance.color_theme",
            &m_ColorTheme, "_builtin_default" ) );

    addParamsForWindow( &m_Window, "window" );

    m_params.emplace_back( new PARAM<bool>( "cross_probing.center_on_items",
            &m_CrossProbing.center_on_items, true ) );

    m_params.emplace_back( new PARAM<bool>( "cross_probing.zoom_to_fit",
            &m_CrossProbing.zoom_to_fit, true ) );

    m_params.emplace_back( new PARAM<bool>( "cross_probing.auto_highlight",
            &m_CrossProbing.auto_highlight, true ) );
}


bool APP_SETTINGS_BASE::MigrateFromLegacy( wxConfigBase* aCfg )
{
    bool ret = true;

    const std::string f = getLegacyFrameName();

    ret &= fromLegacyString(   aCfg, "LastFindString",      "find_replace.find_string" );
    ret &= fromLegacyString(   aCfg, "LastReplaceString",   "find_replace.replace_string" );

    migrateFindReplace( aCfg );

    ret &= fromLegacy<int>(    aCfg, "canvas_type",         "graphics.canvas_type" );

    ret &= fromLegacy<int>(    aCfg, "P22LIB_TREE_MODEL_ADAPTERSelectorColumnWidth",
                                                            "lib_tree.column_width" );

    ret &= fromLegacy<bool>(   aCfg, "PrintMonochrome",     "printing.monochrome" );
    ret &= fromLegacy<double>( aCfg, "PrintScale",          "printing.scale" );
    ret &= fromLegacy<bool>(   aCfg, "PrintPageFrame",      "printing.title_block" );

    {
        nlohmann::json js = nlohmann::json::array();
        wxString       key;
        bool           val = false;

        for( unsigned i = 0; i < PCB_LAYER_ID_COUNT; ++i )
        {
            key.Printf( wxT( "PlotLayer_%d" ), i );

            if( aCfg->Read( key, &val ) && val )
                js.push_back( i );
        }

        ( *this )[PointerFromString( "printing.layers" ) ] = js;
    }

    ret &= fromLegacy<bool>(   aCfg, f + "FirstRunShown",       "system.first_run_shown" );
    ret &= fromLegacy<int>(    aCfg, f + "DevelMaxUndoItems",   "system.max_undo_items" );
    ret &= fromLegacy<int>(    aCfg, f + "Units",               "system.units" );

    {
        int            max_history_size = Pgm().GetCommonSettings()->m_System.file_history_size;
        wxString       file, key;
        nlohmann::json js = nlohmann::json::array();

        for( int i = 1; i <= max_history_size; i++ )
        {
            key.Printf( "file%d", i );
            file = aCfg->Read( key, wxEmptyString );

            if( !file.IsEmpty() )
                js.push_back( file.ToStdString() );
        }

        ( *this )[PointerFromString( "system.file_history" )] = js;
    }

    ret &= migrateWindowConfig( aCfg, f, "window" );

    return ret;
}


void APP_SETTINGS_BASE::migrateFindReplace( wxConfigBase* aCfg )
{
    const int find_replace_history_size = 10;
    nlohmann::json find_history         = nlohmann::json::array();
    nlohmann::json replace_history      = nlohmann::json::array();
    wxString tmp, find_key, replace_key;

    for( int i = 0; i < find_replace_history_size; ++i )
    {
        find_key.Printf( "FindStringHistoryList%d", i );
        replace_key.Printf( "ReplaceStringHistoryList%d", i );

        if( aCfg->Read( find_key, &tmp ) )
            find_history.push_back( tmp.ToStdString() );

        if( aCfg->Read( replace_key, &tmp ) )
            replace_history.push_back( tmp.ToStdString() );
    }

    ( *this )[PointerFromString( "find_replace.find_history" )] = find_history;
    ( *this )[PointerFromString( "find_replace.replace_history" )] = replace_history;
}


bool APP_SETTINGS_BASE::migrateWindowConfig( wxConfigBase* aCfg, const std::string& aFrame,
                                             const std::string& aJsonPath )
{
    bool ret = true;

    const std::string frameGDO = aFrame + "GalDisplayOptions";
    const std::string cursorPath = aJsonPath + ".cursor";
    const std::string gridPath = aJsonPath + ".grid";

    ret &= fromLegacy<bool>( aCfg, aFrame + "Maximized",            aJsonPath + ".maximized" );
    ret &= fromLegacyString( aCfg, aFrame + "MostRecentlyUsedPath", aJsonPath + ".mru_path" );
    ret &= fromLegacy<int>(  aCfg, aFrame + "Size_x",               aJsonPath + ".size_x" );
    ret &= fromLegacy<int>(  aCfg, aFrame + "Size_y",               aJsonPath + ".size_y" );
    ret &= fromLegacyString( aCfg, aFrame + "Perspective",          aJsonPath + ".perspective" );
    ret &= fromLegacy<int>(  aCfg, aFrame + "Pos_x",                aJsonPath + ".pos_x" );
    ret &= fromLegacy<int>(  aCfg, aFrame + "Pos_y",                aJsonPath + ".pos_y" );

    ret &= fromLegacy<bool>( aCfg, frameGDO + "ForceDisplayCursor", cursorPath + ".always_show_cursor" );
    ret &= fromLegacy<bool>( aCfg, frameGDO + "CursorFullscreen",   cursorPath + ".fullscreen_cursor" );

    ret &= fromLegacy<int>(  aCfg, aFrame + "_LastGridSize",        gridPath + ".last_size" );

    ret &= fromLegacy<int>(  aCfg, aFrame + "FastGrid1",            gridPath + ".fast_grid_1" );
    ret &= fromLegacy<int>(  aCfg, aFrame + "FastGrid2",            gridPath + ".fast_grid_2" );

    ret &= fromLegacy<bool>(   aCfg, frameGDO + "GridAxesEnabled",  gridPath + ".axes_enabled" );
    ret &= fromLegacy<double>( aCfg, frameGDO + "GridLineWidth",    gridPath + ".line_width" );
    ret &= fromLegacy<double>( aCfg, frameGDO + "GridMaxDensity",   gridPath + ".min_spacing" );
    ret &= fromLegacy<bool>(   aCfg, frameGDO + "ShowGrid",         gridPath + ".show" );
    ret &= fromLegacy<int>(    aCfg, frameGDO + "GridStyle",        gridPath + ".style" );
    ret &= fromLegacyColor(    aCfg, frameGDO + "GridColor",        gridPath + ".color" );

    return ret;
}


void APP_SETTINGS_BASE::addParamsForWindow( WINDOW_SETTINGS* aWindow, const std::string& aJsonPath )
{
    m_params.emplace_back( new PARAM<bool>( aJsonPath + ".maximized",
            &aWindow->state.maximized, false ) );

    m_params.emplace_back( new PARAM<wxString>( aJsonPath + ".mru_path",
            &aWindow->mru_path, "" ) );

    m_params.emplace_back( new PARAM<int>( aJsonPath + ".size_x", &aWindow->state.size_x, 0 ) );

    m_params.emplace_back( new PARAM<int>( aJsonPath + ".size_y", &aWindow->state.size_y, 0 ) );

    m_params.emplace_back( new PARAM<wxString>( aJsonPath + ".perspective",
            &aWindow->perspective, "" ) );

    m_params.emplace_back( new PARAM<int>( aJsonPath + ".pos_x", &aWindow->state.pos_x, 0 ) );

    m_params.emplace_back( new PARAM<int>( aJsonPath + ".pos_y", &aWindow->state.pos_y, 0 ) );

    m_params.emplace_back( new PARAM<unsigned int>( aJsonPath + ".display", &aWindow->state.display, 0 ) );

    m_params.emplace_back( new PARAM_LIST<double>( aJsonPath + ".zoom_factors",
            &aWindow->zoom_factors, {} ) );

    m_params.emplace_back( new PARAM<bool>( aJsonPath + ".grid.axes_enabled",
            &aWindow->grid.axes_enabled, false ) );

    m_params.emplace_back( new PARAM_LIST<wxString>( aJsonPath + ".grid.sizes",
            &aWindow->grid.sizes, DefaultGridSizeList() ) );

    // pcbnew default grid doesn't matter much, but eeschema does, so default to the index
    // of the 50mil grid in eeschema
    m_params.emplace_back( new PARAM<int>( aJsonPath + ".grid.last_size",
            &aWindow->grid.last_size_idx, 1 ) );

    m_params.emplace_back( new PARAM<int>( aJsonPath + ".grid.fast_grid_1",
            &aWindow->grid.fast_grid_1, 1 ) );

    m_params.emplace_back( new PARAM<int>( aJsonPath + ".grid.fast_grid_2",
            &aWindow->grid.fast_grid_2, 2 ) );

    // for grid user, use a default value compatible with eeschema and pcbnew (10 mils)
    m_params.emplace_back( new PARAM<wxString>( aJsonPath + ".grid.user_grid_x",
            &aWindow->grid.user_grid_x, "10 mil" ) );
    m_params.emplace_back( new PARAM<wxString>( aJsonPath + ".grid.user_grid_y",
            &aWindow->grid.user_grid_y, "10 mil" ) );

    m_params.emplace_back( new PARAM<double>( aJsonPath + ".grid.line_width",
            &aWindow->grid.line_width, 1.0 ) );

    m_params.emplace_back( new PARAM<double>( aJsonPath + ".grid.min_spacing",
            &aWindow->grid.min_spacing, 10 ) );

    m_params.emplace_back( new PARAM<bool>( aJsonPath + ".grid.show",
            &aWindow->grid.show, true ) );

    m_params.emplace_back( new PARAM<int>( aJsonPath + ".grid.style",
            &aWindow->grid.style, 0 ) );

    m_params.emplace_back( new PARAM<int>( aJsonPath + ".grid.snap",
            &aWindow->grid.snap, 0 ) );

    m_params.emplace_back( new PARAM<bool>( aJsonPath + ".cursor.always_show_cursor",
            &aWindow->cursor.always_show_cursor, true ) );

    m_params.emplace_back( new PARAM<bool>( aJsonPath + ".cursor.fullscreen_cursor",
            &aWindow->cursor.fullscreen_cursor, false ) );
}
