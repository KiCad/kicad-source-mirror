/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <set>
#include <settings/common_settings.h>
#include <settings/parameters.h>
#include <wx/config.h>
#include <wx/log.h>


///! The following environment variables will never be migrated from a previous version
const std::set<wxString> envVarBlacklist =
        {
            wxT( "KICAD6_SYMBOL_DIR" ),
            wxT( "KICAD6_FOOTPRINT_DIR" ),
            wxT( "KICAD6_TEMPLATES_DIR" ),
            wxT( "KICAD6_3DMODEL_DIR" )
        };


///! Update the schema version whenever a migration is required
const int commonSchemaVersion = 2;

COMMON_SETTINGS::COMMON_SETTINGS() :
        JSON_SETTINGS( "kicad_common", SETTINGS_LOC::USER, commonSchemaVersion ),
        m_Appearance(),
        m_Backup(),
        m_Env(),
        m_Input(),
        m_Graphics(),
        m_Session(),
        m_System(),
        m_NetclassPanel()
{
    // This only effect the first time KiCad is run.  The user's setting will be used for all
    // subsequent runs.
    // Menu icons are off by default on OSX and on for all other platforms.
    // Use automatic canvas scaling on OSX, but not on the other platforms (their detection
    // isn't as good).
#if defined( __WXMAC__ )
    bool   defaultUseIconsInMenus = false;
    double canvasScale = 0.0;
#else
    bool   defaultUseIconsInMenus = true;
    double canvasScale = 1.0;
#endif

    m_params.emplace_back( new PARAM<double>( "appearance.canvas_scale",
            &m_Appearance.canvas_scale, canvasScale ) );

    m_params.emplace_back( new PARAM<int>( "appearance.icon_scale",
            &m_Appearance.icon_scale, 0 ) );

    m_params.emplace_back( new PARAM_ENUM<ICON_THEME>( "appearance.icon_theme",
            &m_Appearance.icon_theme, ICON_THEME::AUTO, ICON_THEME::LIGHT, ICON_THEME::AUTO ) );

    m_params.emplace_back( new PARAM<bool>( "appearance.use_icons_in_menus",
            &m_Appearance.use_icons_in_menus, defaultUseIconsInMenus ) );

    m_params.emplace_back( new PARAM<bool>( "auto_backup.enabled", &m_Backup.enabled, true ) );

    m_params.emplace_back( new PARAM<bool>( "auto_backup.backup_on_autosave",
            &m_Backup.backup_on_autosave, false ) );

    m_params.emplace_back( new PARAM<int>( "auto_backup.limit_total_files",
            &m_Backup.limit_total_files, 25 ) );

    m_params.emplace_back( new PARAM<unsigned long long>( "auto_backup.limit_total_size",
            &m_Backup.limit_total_size, 104857600 ) );

    m_params.emplace_back( new PARAM<int>( "auto_backup.limit_daily_files",
            &m_Backup.limit_daily_files, 5 ) );

    m_params.emplace_back( new PARAM<int>( "auto_backup.min_interval",
            &m_Backup.min_interval, 300 ) );

    m_params.emplace_back( new PARAM<bool>( "environment.show_warning_dialog",
            &m_Env.show_warning_dialog, false ) );

    m_params.emplace_back( new PARAM_MAP<wxString>( "environment.vars", &m_Env.vars, {} ) );

    m_params.emplace_back( new PARAM<bool>( "input.auto_pan", &m_Input.auto_pan, false ) );

    m_params.emplace_back( new PARAM<int>( "input.auto_pan_acceleration",
            &m_Input.auto_pan_acceleration, 5 ) );

    m_params.emplace_back( new PARAM<bool>( "input.center_on_zoom",
            &m_Input.center_on_zoom, true ) );

    m_params.emplace_back( new PARAM<bool>( "input.immediate_actions",
            &m_Input.immediate_actions, true ) );

    m_params.emplace_back( new PARAM<bool>( "input.warp_mouse_on_move",
            &m_Input.warp_mouse_on_move, true ) );

    m_params.emplace_back( new PARAM<bool>( "input.horizontal_pan",
            &m_Input.horizontal_pan, false ) );

#if defined(__WXMAC__) || defined(__WXGTK3__)
    bool default_zoom_acceleration = false;
#else
    bool default_zoom_acceleration = true;
#endif

    m_params.emplace_back( new PARAM<bool>( "input.zoom_acceleration",
            &m_Input.zoom_acceleration, default_zoom_acceleration ) );

#ifdef __WXMAC__
    int default_zoom_speed = 5;
#else
    int default_zoom_speed = 1;
#endif

    m_params.emplace_back( new PARAM<int>( "input.zoom_speed",
            &m_Input.zoom_speed, default_zoom_speed ) );

    m_params.emplace_back( new PARAM<bool>( "input.zoom_speed_auto",
            &m_Input.zoom_speed_auto, true ) );

    m_params.emplace_back( new PARAM<int>( "input.scroll_modifier_zoom",
            &m_Input.scroll_modifier_zoom, 0 ) );

    m_params.emplace_back( new PARAM<int>( "input.scroll_modifier_pan_h",
            &m_Input.scroll_modifier_pan_h, WXK_CONTROL ) );

    m_params.emplace_back( new PARAM<int>( "input.scroll_modifier_pan_v",
            &m_Input.scroll_modifier_pan_v, WXK_SHIFT ) );

    m_params.emplace_back( new PARAM_ENUM<MOUSE_DRAG_ACTION>( "input.mouse_left",
            &m_Input.drag_left, MOUSE_DRAG_ACTION::DRAG_SELECTED, MOUSE_DRAG_ACTION::DRAG_ANY,
            MOUSE_DRAG_ACTION::SELECT ) );

    m_params.emplace_back( new PARAM_ENUM<MOUSE_DRAG_ACTION>( "input.mouse_middle",
            &m_Input.drag_middle, MOUSE_DRAG_ACTION::PAN, MOUSE_DRAG_ACTION::SELECT,
            MOUSE_DRAG_ACTION::NONE ) );

    m_params.emplace_back( new PARAM_ENUM<MOUSE_DRAG_ACTION>( "input.mouse_right",
            &m_Input.drag_right, MOUSE_DRAG_ACTION::PAN, MOUSE_DRAG_ACTION::SELECT,
            MOUSE_DRAG_ACTION::NONE ) );

    m_params.emplace_back( new PARAM<int>( "graphics.opengl_antialiasing_mode",
            &m_Graphics.opengl_aa_mode, 0, 0, 4 ) );

    m_params.emplace_back( new PARAM<int>( "graphics.cairo_antialiasing_mode",
            &m_Graphics.cairo_aa_mode, 0, 0, 3 ) );

    m_params.emplace_back( new PARAM<int>( "system.autosave_interval",
            &m_System.autosave_interval, 600 ) );

    m_params.emplace_back( new PARAM<wxString>( "system.editor_name",
            &m_System.editor_name, "" ) );

    m_params.emplace_back( new PARAM<int>( "system.file_history_size",
            &m_System.file_history_size, 9 ) );

    m_params.emplace_back( new PARAM<wxString>( "system.language",
            &m_System.language, "Default" ) );

    m_params.emplace_back( new PARAM<wxString>( "system.pdf_viewer_name",
            &m_System.pdf_viewer_name, "" ) );

    m_params.emplace_back( new PARAM<bool>( "system.use_system_pdf_viewer",
            &m_System.use_system_pdf_viewer, true ) );

    m_params.emplace_back( new PARAM<wxString>( "system.working_dir",
            &m_System.working_dir, "" ) );

    m_params.emplace_back( new PARAM<int>( "system.clear_3d_cache_interval",
            &m_System.clear_3d_cache_interval, 30 ) );

    m_params.emplace_back( new PARAM<bool>( "session.remember_open_files",
            &m_Session.remember_open_files, false ) );

    m_params.emplace_back( new PARAM<int>( "netclass_panel.sash_pos",
            &m_NetclassPanel.sash_pos, 160 ) );

    registerMigration( 0, 1, std::bind( &COMMON_SETTINGS::migrateSchema0to1, this ) );
    registerMigration( 1, 2, std::bind( &COMMON_SETTINGS::migrateSchema1to2, this ) );
}


bool COMMON_SETTINGS::migrateSchema0to1()
{
    /**
     * Schema version 0 to 1:
     *
     * mousewheel_pan is replaced by explicit settings for scroll wheel behavior
     */

    nlohmann::json::json_pointer mwp_pointer( "/input/mousewheel_pan"_json_pointer );

    bool mwp = false;

    try
    {
        mwp = at( mwp_pointer );
        at( nlohmann::json::json_pointer( "/input"_json_pointer ) ).erase( "mousewheel_pan" );
    }
    catch( ... )
    {
        wxLogTrace( traceSettings, "COMMON_SETTINGS::Migrate 0->1: mousewheel_pan not found" );
    }

    if( mwp )
    {
        ( *this )[nlohmann::json::json_pointer( "/input/horizontal_pan" )] = true;

        ( *this )[nlohmann::json::json_pointer( "/input/scroll_modifier_pan_h" )] = WXK_SHIFT;
        ( *this )[nlohmann::json::json_pointer( "/input/scroll_modifier_pan_v" )] = 0;
        ( *this )[nlohmann::json::json_pointer( "/input/scroll_modifier_zoom" )]  = WXK_CONTROL;
    }
    else
    {
        ( *this )[nlohmann::json::json_pointer( "/input/horizontal_pan" )] = false;

        ( *this )[nlohmann::json::json_pointer( "/input/scroll_modifier_pan_h" )] = WXK_CONTROL;
        ( *this )[nlohmann::json::json_pointer( "/input/scroll_modifier_pan_v" )] = WXK_SHIFT;
        ( *this )[nlohmann::json::json_pointer( "/input/scroll_modifier_zoom" )]  = 0;
    }

    return true;
}


bool COMMON_SETTINGS::migrateSchema1to2()
{
    nlohmann::json::json_pointer v1_pointer( "/input/prefer_select_to_drag"_json_pointer );

    bool prefer_selection = false;

    try
    {
        prefer_selection = at( v1_pointer );
        at( nlohmann::json::json_pointer( "/input"_json_pointer ) ).erase( "prefer_select_to_drag" );
    }
    catch( ... )
    {
        wxLogTrace( traceSettings, "COMMON_SETTINGS::Migrate 1->2: prefer_select_to_drag not found" );
    }

    if( prefer_selection )
        ( *this )[nlohmann::json::json_pointer( "/input/mouse_left" )] = MOUSE_DRAG_ACTION::SELECT;
    else
        ( *this )[nlohmann::json::json_pointer( "/input/mouse_left" )] = MOUSE_DRAG_ACTION::DRAG_ANY;

    return true;
}


bool COMMON_SETTINGS::MigrateFromLegacy( wxConfigBase* aCfg )
{
    bool ret = true;

    ret &= fromLegacy<double>( aCfg, "CanvasScale",     "appearance.canvas_scale" );
    ret &= fromLegacy<int>( aCfg,    "IconScale",       "appearance.icon_scale" );
    ret &= fromLegacy<bool>( aCfg,   "UseIconsInMenus", "appearance.use_icons_in_menus" );

// Force OSX to automatically scale the canvas. Before v6, the user setting wasn't used on OSX
// and was set to 1.0. In v6, the setting is now used by OSX and should default to automatic
// scaling.
#ifdef __WXMAC__
    ( *this )[PointerFromString( "appearance.canvas_scale" )] = 0.0;
#endif

    ret &= fromLegacy<bool>( aCfg, "ShowEnvVarWarningDialog", "environment.show_warning_dialog" );

    auto load_env_vars = [&] () {
          wxString key, value;
          long index = 0;
          nlohmann::json::json_pointer ptr = PointerFromString( "environment.vars" );

          aCfg->SetPath( "EnvironmentVariables" );
          ( *this )[ptr] = nlohmann::json( {} );

          while( aCfg->GetNextEntry( key, index ) )
          {
              if( envVarBlacklist.count( key ) )
              {
                  wxLogTrace( traceSettings, "Migrate Env: %s is blacklisted; skipping.", key );
                  continue;
              }

              value = aCfg->Read( key, wxEmptyString );

              if( !value.IsEmpty() )
              {
                  ptr.push_back( key.ToStdString() );

                  wxLogTrace( traceSettings, "Migrate Env: %s=%s", ptr.to_string(), value );
                  ( *this )[ptr] = value.ToUTF8();

                  ptr.pop_back();
              }
          }

          aCfg->SetPath( ".." );
    };

    load_env_vars();

    bool mousewheel_pan = false;

    if( aCfg->Read( "MousewheelPAN", &mousewheel_pan ) && mousewheel_pan )
    {
        ( *this )[PointerFromString( "input.horizontal_pan" )] = true;

        ( *this )[PointerFromString( "input.scroll_modifier_pan_h" )] = WXK_SHIFT;
        ( *this )[PointerFromString( "input.scroll_modifier_pan_v" )] = 0;
        ( *this )[PointerFromString( "input.scroll_modifier_zoom" )]  = WXK_CONTROL;
    }

    ret &= fromLegacy<bool>( aCfg, "AutoPAN",                   "input.auto_pan" );
    ret &= fromLegacy<bool>( aCfg, "ImmediateActions",          "input.immediate_actions" );
    ret &= fromLegacy<bool>( aCfg, "PreferSelectionToDragging", "input.prefer_select_to_drag" );
    ret &= fromLegacy<bool>( aCfg, "MoveWarpsCursor",           "input.warp_mouse_on_move" );
    ret &= fromLegacy<bool>( aCfg, "ZoomNoCenter",              "input.center_on_zoom" );

    // This was stored inverted in legacy config
    if( ret )
    {
        auto p = PointerFromString( "input.center_on_zoom" );
        ( *this )[p] = !( *this )[p];
    }

    ret &= fromLegacy<int>( aCfg, "OpenGLAntialiasingMode", "graphics.opengl_antialiasing_mode" );
    ret &= fromLegacy<int>( aCfg, "CairoAntialiasingMode",  "graphics.cairo_antialiasing_mode" );

    ret &= fromLegacy<int>(  aCfg, "AutoSaveInterval",        "system.autosave_interval" );
    ret &= fromLegacyString( aCfg, "Editor",                  "system.editor_name" );
    ret &= fromLegacy<int>(  aCfg, "FileHistorySize",         "system.file_history_size" );
    ret &= fromLegacyString( aCfg, "LanguageID",              "system.language" );
    ret &= fromLegacyString( aCfg, "PdfBrowserName",          "system.pdf_viewer_name" );
    ret &= fromLegacy<bool>( aCfg, "UseSystemBrowser",        "system.use_system_pdf_viewer" );
    ret &= fromLegacyString( aCfg, "WorkingDir",              "system.working_dir" );

    return ret;
}
