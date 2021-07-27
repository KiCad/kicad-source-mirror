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

#include <paths.h>
#include <search_stack.h>
#include <settings/common_settings.h>
#include <settings/json_settings_internals.h>
#include <settings/parameters.h>
#include <systemdirsappend.h>
#include <trace_helpers.h>
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

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "environment.vars",
            [&]() -> nlohmann::json
            {
                nlohmann::json ret = {};

                for( const std::pair<wxString, ENV_VAR_ITEM> entry : m_Env.vars )
                {
                    const ENV_VAR_ITEM& var = entry.second;

                    wxASSERT( entry.first == var.GetKey() );

                    // Default values are never persisted
                    if( var.IsDefault() )
                    {
                        wxLogTrace( traceEnvVars,
                                    "COMMON_SETTINGS: Env var %s skipping save (default)",
                                    var.GetKey() );
                        continue;
                    }

                    wxString value = var.GetValue();

                    // Vars that existed in JSON are persisted, but if they were overridden
                    // externally, we persist the old value (i.e. the one that was loaded from JSON)
                    if( var.GetDefinedExternally() )
                    {
                        if( var.GetDefinedInSettings() )
                        {
                            wxLogTrace( traceEnvVars,
                                        "COMMON_SETTINGS: Env var %s was overridden externally, "
                                        "saving previously-loaded value %s",
                                        var.GetKey(), var.GetSettingsValue() );
                            value = var.GetSettingsValue();
                        }
                        else
                        {
                            wxLogTrace( traceEnvVars,
                                        "COMMON_SETTINGS: Env var %s skipping save (external)",
                                        var.GetKey() );
                            continue;
                        }
                    }

                    wxLogTrace( traceEnvVars,
                                "COMMON_SETTINGS: Saving env var %s = %s",
                                var.GetKey(), value);

                    std::string key( var.GetKey().ToUTF8() );
                    ret[key] = value;
                }

                return ret;
            },
            [&]( const nlohmann::json& aJson )
            {
                if( !aJson.is_object() )
                    return;

                for( const auto& entry : aJson.items() )
                {
                    wxString key = wxString( entry.key().c_str(), wxConvUTF8 );
                    wxString val = entry.value().get<wxString>();

                    if( m_Env.vars.count( key ) )
                    {
                        if( m_Env.vars[key].GetDefinedExternally() )
                        {
                            wxLogTrace( traceEnvVars, "COMMON_SETTINGS: %s is defined externally",
                                        key );
                            m_Env.vars[key].SetDefinedInSettings();
                            m_Env.vars[key].SetSettingsValue( val );
                            continue;
                        }
                        else
                        {
                            wxLogTrace( traceEnvVars, "COMMON_SETTINGS: Updating %s: %s -> %s",
                                        key, m_Env.vars[key].GetValue(), val );
                            m_Env.vars[key].SetValue( val );
                        }
                    }
                    else
                    {
                        wxLogTrace( traceEnvVars, "COMMON_SETTINGS: Loaded new var: %s = %s",
                                    key, val );
                        m_Env.vars[key] = ENV_VAR_ITEM( key, val );
                    }

                    m_Env.vars[key].SetDefinedInSettings();
                    m_Env.vars[key].SetSettingsValue( val );
                }
            },
            {} ) );

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

    m_params.emplace_back( new PARAM<bool>( "input.zoom_acceleration",
            &m_Input.zoom_acceleration, false ) );

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
            &m_Graphics.opengl_aa_mode, 0, 0, 2 ) );

    m_params.emplace_back( new PARAM<int>( "graphics.cairo_antialiasing_mode",
            &m_Graphics.cairo_aa_mode, 0, 0, 2 ) );

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

    m_params.emplace_back( new PARAM<bool>( "do_not_show_again.zone_fill_warning",
            &m_DoNotShowAgain.zone_fill_warning, false ) );

    m_params.emplace_back( new PARAM<bool>( "do_not_show_again.env_var_overwrite_warning",
            &m_DoNotShowAgain.env_var_overwrite_warning, false ) );

    m_params.emplace_back( new PARAM<bool>( "do_not_show_again.scaled_3d_models_warning",
            &m_DoNotShowAgain.scaled_3d_models_warning, false ) );

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
        mwp = m_internals->at( mwp_pointer );
        m_internals->At( "input" ).erase( "mousewheel_pan" );
    }
    catch( ... )
    {
        wxLogTrace( traceSettings, "COMMON_SETTINGS::Migrate 0->1: mousewheel_pan not found" );
    }

    if( mwp )
    {
        ( *m_internals )[nlohmann::json::json_pointer( "/input/horizontal_pan" )] = true;

        ( *m_internals )[nlohmann::json::json_pointer( "/input/scroll_modifier_pan_h" )] = WXK_SHIFT;
        ( *m_internals )[nlohmann::json::json_pointer( "/input/scroll_modifier_pan_v" )] = 0;
        ( *m_internals )[nlohmann::json::json_pointer( "/input/scroll_modifier_zoom" )]  = WXK_CONTROL;
    }
    else
    {
        ( *m_internals )[nlohmann::json::json_pointer( "/input/horizontal_pan" )] = false;

        ( *m_internals )[nlohmann::json::json_pointer( "/input/scroll_modifier_pan_h" )] = WXK_CONTROL;
        ( *m_internals )[nlohmann::json::json_pointer( "/input/scroll_modifier_pan_v" )] = WXK_SHIFT;
        ( *m_internals )[nlohmann::json::json_pointer( "/input/scroll_modifier_zoom" )]  = 0;
    }

    return true;
}


bool COMMON_SETTINGS::migrateSchema1to2()
{
    nlohmann::json::json_pointer v1_pointer( "/input/prefer_select_to_drag"_json_pointer );

    bool prefer_selection = false;

    try
    {
        prefer_selection = m_internals->at( v1_pointer );
        m_internals->at( nlohmann::json::json_pointer( "/input"_json_pointer ) ).erase( "prefer_select_to_drag" );
    }
    catch( ... )
    {
        wxLogTrace( traceSettings, "COMMON_SETTINGS::Migrate 1->2: prefer_select_to_drag not found" );
    }

    if( prefer_selection )
        ( *m_internals )[nlohmann::json::json_pointer( "/input/mouse_left" )] = MOUSE_DRAG_ACTION::SELECT;
    else
        ( *m_internals )[nlohmann::json::json_pointer( "/input/mouse_left" )] = MOUSE_DRAG_ACTION::DRAG_ANY;

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
    Set( "appearance.canvas_scale", 0.0 );
#endif

    ret &= fromLegacy<bool>( aCfg, "ShowEnvVarWarningDialog", "environment.show_warning_dialog" );

    auto load_env_vars = [&] () {
          wxString key, value;
          long index = 0;
          nlohmann::json::json_pointer ptr = m_internals->PointerFromString( "environment.vars" );

          aCfg->SetPath( "EnvironmentVariables" );
          ( *m_internals )[ptr] = nlohmann::json( {} );

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
                  ( *m_internals )[ptr] = value.ToUTF8();

                  ptr.pop_back();
              }
          }

          aCfg->SetPath( ".." );
    };

    load_env_vars();

    bool mousewheel_pan = false;

    if( aCfg->Read( "MousewheelPAN", &mousewheel_pan ) && mousewheel_pan )
    {
        Set( "input.horizontal_pan", true );
        Set( "input.scroll_modifier_pan_h", static_cast<int>( WXK_SHIFT ) );
        Set( "input.scroll_modifier_pan_v", 0 );
        Set( "input.scroll_modifier_zoom",  static_cast<int>( WXK_CONTROL ) );
    }

    ret &= fromLegacy<bool>( aCfg, "AutoPAN",                   "input.auto_pan" );
    ret &= fromLegacy<bool>( aCfg, "ImmediateActions",          "input.immediate_actions" );
    ret &= fromLegacy<bool>( aCfg, "PreferSelectionToDragging", "input.prefer_select_to_drag" );
    ret &= fromLegacy<bool>( aCfg, "MoveWarpsCursor",           "input.warp_mouse_on_move" );
    ret &= fromLegacy<bool>( aCfg, "ZoomNoCenter",              "input.center_on_zoom" );

    // This was stored inverted in legacy config
    if( OPT<bool> value = Get<bool>( "input.center_on_zoom" ) )
        Set( "input.center_on_zoom", !( *value ) );

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


void COMMON_SETTINGS::InitializeEnvironment()
{
    auto addVar =
        [&]( const wxString& aKey, const wxString& aDefault )
        {
            m_Env.vars[aKey] = ENV_VAR_ITEM( aKey, aDefault, aDefault );

            wxString envValue;

            if( wxGetEnv( aKey, &envValue ) == true && !envValue.IsEmpty() )
            {
                m_Env.vars[aKey].SetValue( envValue );
                m_Env.vars[aKey].SetDefinedExternally();
                wxLogTrace( traceEnvVars,
                            "InitializeEnvironment: Entry %s defined externally as %s", aKey,
                            envValue );
            }
            else
            {
                wxLogTrace( traceEnvVars, "InitializeEnvironment: Setting entry %s to default %s",
                            aKey, aDefault );
            }
        };

    wxFileName basePath( PATHS::GetStockEDALibraryPath(), wxEmptyString );

    wxFileName path( basePath );
    path.AppendDir( wxT( "modules" ) );
    addVar( wxT( "KICAD6_FOOTPRINT_DIR" ), path.GetFullPath() );

    path = basePath;
    path.AppendDir( wxT( "3dmodels" ) );
    addVar( wxT( "KICAD6_3DMODEL_DIR" ), path.GetFullPath() );

    // We don't have just one default template path, so use this logic that originally was in
    // PGM_BASE::InitPgm to determine the best default template path
    {
        // Attempt to find the best default template path.
        SEARCH_STACK bases;
        SEARCH_STACK templatePaths;

        SystemDirsAppend( &bases );

        for( unsigned i = 0; i < bases.GetCount(); ++i )
        {
            wxFileName fn( bases[i], wxEmptyString );

            // Add KiCad template file path to search path list.
            fn.AppendDir( "template" );

            // Only add path if exists and can be read by the user.
            if( fn.DirExists() && fn.IsDirReadable() )
            {
                wxLogTrace( tracePathsAndFiles, "Checking template path '%s' exists",
                            fn.GetPath() );
                templatePaths.AddPaths( fn.GetPath() );
            }
        }

        if( templatePaths.IsEmpty() )
        {
            path = basePath;
            path.AppendDir( "template" );
        }
        else
        {
            // Take the first one.  There may be more but this will likely be the best option.
            path.AssignDir( templatePaths[0] );
        }

        addVar( wxT( "KICAD6_TEMPLATE_DIR" ), path.GetFullPath() );
    }

    addVar( wxT( "KICAD_USER_TEMPLATE_DIR" ), PATHS::GetUserTemplatesPath() );

    path = basePath;
    path.AppendDir( wxT( "library" ) );
    addVar( wxT( "KICAD6_SYMBOL_DIR" ), path.GetFullPath() );
}
