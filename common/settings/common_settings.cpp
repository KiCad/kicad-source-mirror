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

#include <set>
#include <fstream>
#include <sstream>

#include <class_draw_panel_gal.h>
#include <env_vars.h>
#include <paths.h>
#include <search_stack.h>
#include <settings/settings_manager.h>
#include <settings/common_settings.h>
#include <settings/json_settings.h>
#include <settings/parameters.h>
#include <systemdirsappend.h>
#include <trace_helpers.h>
#include <wx/config.h>
#include <wx/log.h>
#include <wx/regex.h>
#include <wx/tokenzr.h>


///! The following environment variables will never be migrated from a previous version
const wxRegEx versionedEnvVarRegex( wxS( "KICAD[0-9]+_[A-Z0-9_]+(_DIR)?" ) );

///! Update the schema version whenever a migration is required
const int commonSchemaVersion = 4;

COMMON_SETTINGS::COMMON_SETTINGS() :
        JSON_SETTINGS( "kicad_common", SETTINGS_LOC::USER, commonSchemaVersion ),
        m_Appearance(),
        m_Backup(),
        m_Env(),
        m_Input(),
        m_SpaceMouse(),
        m_Graphics(),
        m_Session(),
        m_System(),
        m_DoNotShowAgain(),
        m_PackageManager(),
        m_Api()
{
    /*
     * Automatic dark mode detection works fine on Mac.
     */
#if defined( __WXGTK__ ) || defined( __WXMSW__ )
    m_params.emplace_back( new PARAM_ENUM<ICON_THEME>( "appearance.icon_theme",
            &m_Appearance.icon_theme, ICON_THEME::AUTO, ICON_THEME::LIGHT, ICON_THEME::AUTO ) );
#else
    m_Appearance.icon_theme = ICON_THEME::AUTO;
#endif

    /*
   	 * Automatic canvas scaling works fine on all supported platforms, so it's no longer exposed as
     * a configuration option.
     */
    m_Appearance.canvas_scale = 0.0;

    /*
     * Menu icons are off by default on OSX and on for all other platforms.
     */
#ifdef __WXMAC__
    m_params.emplace_back( new PARAM<bool>( "appearance.use_icons_in_menus",
            &m_Appearance.use_icons_in_menus, false ) );
#else
    m_params.emplace_back( new PARAM<bool>( "appearance.use_icons_in_menus",
            &m_Appearance.use_icons_in_menus, true ) );
#endif

    /*
     * Font scaling hacks are only needed on GTK under wxWidgets 3.0.
     */
    m_Appearance.apply_icon_scale_to_fonts = false;

    m_params.emplace_back( new PARAM<bool>( "appearance.show_scrollbars",
            &m_Appearance.show_scrollbars, false ) );

    m_params.emplace_back( new PARAM<double>( "appearance.hicontrast_dimming_factor",
            &m_Appearance.hicontrast_dimming_factor, 0.8f ) );

    m_params.emplace_back( new PARAM<int>( "appearance.text_editor_zoom",
            &m_Appearance.text_editor_zoom, 0 ) );

    m_params.emplace_back( new PARAM<int>( "appearance.toolbar_icon_size",
            &m_Appearance.toolbar_icon_size, 24, 16, 64 ) );

    m_params.emplace_back( new PARAM<bool>( "appearance.grid_striping",
            &m_Appearance.grid_striping, false ) );

    m_params.emplace_back( new PARAM<bool>( "appearance.use_custom_cursors",
            &m_Appearance.use_custom_cursors, true ) );

    m_Appearance.zoom_correction_factor = 1.0;
    m_params.emplace_back( new PARAM<double>( "appearance.zoom_correction_factor",
            &m_Appearance.zoom_correction_factor, 1.0, 0.1, 10.0 ) );

    m_params.emplace_back( new PARAM<bool>( "auto_backup.enabled", &m_Backup.enabled, true ) );

    m_params.emplace_back( new PARAM<unsigned long long>( "auto_backup.limit_total_size",
            &m_Backup.limit_total_size, 104857600 ) );

    auto envVarsParam = m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "environment.vars",
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
                                    wxS( "COMMON_SETTINGS: Env var %s skipping save (default)" ),
                                    var.GetKey() );
                        continue;
                    }

                    wxString value = var.GetValue();

                    value.Trim( true ).Trim( false ); // Trim from both sides

                    // Vars that existed in JSON are persisted, but if they were overridden
                    // externally, we persist the old value (i.e. the one that was loaded from JSON)
                    if( var.GetDefinedExternally() )
                    {
                        if( var.GetDefinedInSettings() )
                        {
                            wxLogTrace( traceEnvVars,
                                        wxS( "COMMON_SETTINGS: Env var %s was overridden "
                                             "externally, saving previously-loaded value %s" ),
                                        var.GetKey(), var.GetSettingsValue() );
                            value = var.GetSettingsValue();
                        }
                        else
                        {
                            wxLogTrace( traceEnvVars,
                                        wxS( "COMMON_SETTINGS: Env var %s skipping save "
                                             "(external)" ),
                                        var.GetKey() );
                            continue;
                        }
                    }

                    wxLogTrace( traceEnvVars,
                                wxS( "COMMON_SETTINGS: Saving env var %s = %s" ),
                                var.GetKey(), value);

                    std::string key( var.GetKey().Trim( true ).Trim( false ).ToUTF8() );
                    ret[ std::move( key ) ] = value;
                }

                return ret;
            },
            [&]( const nlohmann::json& aJson )
            {
                if( !aJson.is_object() )
                    return;

                for( const auto& entry : aJson.items() )
                {
                    wxString key = wxString( entry.key().c_str(), wxConvUTF8 ).Trim( true ).Trim( false );
                    wxString val = entry.value().get<wxString>().Trim( true ).Trim( false );

                    if( m_Env.vars.count( key ) )
                    {
                        if( m_Env.vars[key].GetDefinedExternally() )
                        {
                            wxLogTrace( traceEnvVars,
                                        wxS( "COMMON_SETTINGS: %s is defined externally" ),
                                        key );
                            m_Env.vars[key].SetDefinedInSettings();
                            m_Env.vars[key].SetSettingsValue( val );
                            continue;
                        }
                        else
                        {
                            wxLogTrace( traceEnvVars,
                                        wxS( "COMMON_SETTINGS: Updating %s: %s -> %s"),
                                        key, m_Env.vars[key].GetValue(), val );
                            m_Env.vars[key].SetValue( val );
                        }
                    }
                    else
                    {
                        wxLogTrace( traceEnvVars,
                                    wxS( "COMMON_SETTINGS: Loaded new var: %s = %s" ),
                                    key, val );
                        m_Env.vars[key] = ENV_VAR_ITEM( key, val );
                    }

                    m_Env.vars[key].SetDefinedInSettings();
                    m_Env.vars[key].SetSettingsValue( val );
                }
            },
            {} ) );
    envVarsParam->SetClearUnknownKeys();

    m_params.emplace_back( new PARAM<bool>( "input.focus_follow_sch_pcb",
            &m_Input.focus_follow_sch_pcb, false ) );

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

    m_params.emplace_back( new PARAM<bool>( "input.hotkey_feedback",
            &m_Input.hotkey_feedback, true ) );

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

    m_params.emplace_back( new PARAM<int>( "input.motion_pan_modifier",
            &m_Input.motion_pan_modifier, 0 ) );

    m_params.emplace_back( new PARAM<bool>( "input.reverse_scroll_zoom",
            &m_Input.reverse_scroll_zoom, false ) );

    m_params.emplace_back( new PARAM<bool>( "input.reverse_scroll_pan_h",
            &m_Input.reverse_scroll_pan_h, false ) );

    m_params.emplace_back( new PARAM_ENUM<MOUSE_DRAG_ACTION>( "input.mouse_left",
            &m_Input.drag_left, MOUSE_DRAG_ACTION::DRAG_SELECTED, MOUSE_DRAG_ACTION::DRAG_ANY,
            MOUSE_DRAG_ACTION::SELECT ) );

    m_params.emplace_back( new PARAM_ENUM<MOUSE_DRAG_ACTION>( "input.mouse_middle",
            &m_Input.drag_middle, MOUSE_DRAG_ACTION::PAN, MOUSE_DRAG_ACTION::SELECT,
            MOUSE_DRAG_ACTION::NONE ) );

    m_params.emplace_back( new PARAM_ENUM<MOUSE_DRAG_ACTION>( "input.mouse_right",
            &m_Input.drag_right, MOUSE_DRAG_ACTION::PAN, MOUSE_DRAG_ACTION::SELECT,
            MOUSE_DRAG_ACTION::NONE ) );

    m_params.emplace_back( new PARAM<int>( "spacemouse.rotate_speed",
            &m_SpaceMouse.rotate_speed, 5, 1, 10 ) );

    m_params.emplace_back( new PARAM<int>( "spacemouse.pan_speed",
            &m_SpaceMouse.pan_speed, 5, 1, 10 ) );

    m_params.emplace_back( new PARAM<bool>( "spacemouse.reverse_rotate",
            &m_SpaceMouse.reverse_rotate, false ) );

    m_params.emplace_back( new PARAM<bool>( "spacemouse.reverse_pan_x",
            &m_SpaceMouse.reverse_pan_x, false ) );

    m_params.emplace_back( new PARAM<bool>( "spacemouse.reverse_pan_y",
            &m_SpaceMouse.reverse_pan_y, false ) );

    m_params.emplace_back( new PARAM<bool>( "spacemouse.reverse_zoom",
            &m_SpaceMouse.reverse_zoom, false ) );

    m_params.emplace_back( new PARAM<int>( "graphics.canvas_type",
            &m_Graphics.canvas_type, EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL ) );

    m_params.emplace_back( new PARAM<int>( "graphics.antialiasing_mode",
            &m_Graphics.aa_mode, 2, 0, 2 ) );

    m_params.emplace_back( new PARAM<bool>( "system.local_history_enabled",
            &m_System.local_history_enabled, true ) );
    m_params.emplace_back( new PARAM<int>( "system.local_history_debounce",
            &m_System.local_history_debounce, 5, 0, 100000 ) );

#ifdef __WXMAC__
    m_params.emplace_back( new PARAM<wxString>( "system.text_editor",
            &m_System.text_editor, wxS( "/usr/bin/open -e" ) ) );
#else
    m_params.emplace_back( new PARAM<wxString>( "system.text_editor",
            &m_System.text_editor, wxS( "" ) ) );
#endif

#if defined( __WINDOWS__ )
    m_params.emplace_back( new PARAM<wxString>( "system.file_explorer",
            &m_System.file_explorer, wxS( "explorer.exe /n,/select,%F" ) ) );
#else
    m_params.emplace_back( new PARAM<wxString>( "system.file_explorer",
            &m_System.file_explorer, wxS( "" ) ) );
#endif

    m_params.emplace_back( new PARAM<int>( "system.file_history_size",
            &m_System.file_history_size, 9 ) );

    m_params.emplace_back( new PARAM<wxString>( "system.language",
            &m_System.language, wxS( "Default" ) ) );

    m_params.emplace_back( new PARAM<wxString>( "system.pdf_viewer_name",
            &m_System.pdf_viewer_name, wxS( "" ) ) );

    m_params.emplace_back( new PARAM<bool>( "system.use_system_pdf_viewer",
            &m_System.use_system_pdf_viewer, true ) );

    m_params.emplace_back( new PARAM<wxString>( "system.working_dir",
            &m_System.working_dir, wxS( "" ) ) );

    m_params.emplace_back( new PARAM<int>( "system.clear_3d_cache_interval",
            &m_System.clear_3d_cache_interval, 30 ) );

    m_params.emplace_back( new PARAM<bool>( "do_not_show_again.zone_fill_warning",
            &m_DoNotShowAgain.zone_fill_warning, false ) );

    m_params.emplace_back( new PARAM<bool>( "do_not_show_again.env_var_overwrite_warning",
            &m_DoNotShowAgain.env_var_overwrite_warning, false ) );

    m_params.emplace_back( new PARAM<bool>( "do_not_show_again.scaled_3d_models_warning",
            &m_DoNotShowAgain.scaled_3d_models_warning, false ) );

    m_params.emplace_back( new PARAM<bool>( "do_not_show_again.data_collection_prompt",
            &m_DoNotShowAgain.data_collection_prompt, false ) );

    m_params.emplace_back( new PARAM<bool>( "do_not_show_again.update_check_prompt",
            &m_DoNotShowAgain.update_check_prompt, false ) );

    m_params.emplace_back( new PARAM<bool>( "session.remember_open_files",
            &m_Session.remember_open_files, false ) );

    m_params.emplace_back( new PARAM_LIST<wxString>( "session.pinned_symbol_libs",
            &m_Session.pinned_symbol_libs, {} ) );

    m_params.emplace_back( new PARAM_LIST<wxString>( "session.pinned_fp_libs",
            &m_Session.pinned_fp_libs, {} ) );

    m_params.emplace_back( new PARAM_LIST<wxString>( "session.pinned_design_block_libs",
            &m_Session.pinned_design_block_libs, {} ) );

    m_params.emplace_back( new PARAM<int>( "package_manager.sash_pos",
            &m_PackageManager.sash_pos, 380 ) );

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "git.repositories",
            [&]() -> nlohmann::json
            {
                nlohmann::json ret = {};

                for( const GIT_REPOSITORY& repo : m_Git.repositories )
                {
                    nlohmann::json repoJson = {};

                    repoJson["name"] = repo.name;
                    repoJson["path"] = repo.path;
                    repoJson["authType"] = repo.authType;
                    repoJson["username"] = repo.username;
                    repoJson["ssh_path"] = repo.ssh_path;
                    repoJson["active"] = repo.active;

                    ret.push_back( repoJson );
                }

                return ret;
            },
            [&]( const nlohmann::json& aJson )
            {
                if( !aJson.is_array() )
                    return;

                m_Git.repositories.clear();

                for( const auto& repoJson : aJson )
                {
                    GIT_REPOSITORY repo;

                    repo.name = repoJson["name"].get<wxString>();
                    repo.path = repoJson["path"].get<wxString>();
                    repo.authType = repoJson["authType"].get<wxString>();
                    repo.username = repoJson["username"].get<wxString>();
                    repo.ssh_path = repoJson["ssh_path"].get<wxString>();
                    repo.active = repoJson["active"].get<bool>();
                    repo.checkValid = true;

                    m_Git.repositories.push_back( repo );
                }
            },
            {} ) );

    m_params.emplace_back( new PARAM<wxString>( "git.authorName",
            &m_Git.authorName, wxS( "" ) ) );

    m_params.emplace_back( new PARAM<wxString>( "git.authorEmail",
            &m_Git.authorEmail, wxS( "" ) ) );

    m_params.emplace_back( new PARAM<bool>( "git.useDefaultAuthor",
            &m_Git.useDefaultAuthor, true ) );

    m_params.emplace_back( new PARAM<bool>( "git.enableGit",
            &m_Git.enableGit, true ) );

    m_params.emplace_back( new PARAM<int>( "git.updatInterval",
            &m_Git.updatInterval, 5 ) );

    m_params.emplace_back( new PARAM<wxString>( "api.interpreter_path",
            &m_Api.python_interpreter, wxS( "" ) ) );

    m_params.emplace_back( new PARAM<bool>( "api.enable_server",
            &m_Api.enable_server, false ) );

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "dialog.controls",
            [&]() -> nlohmann::json
            {
                nlohmann::json ret = nlohmann::json::object();

                for( const auto& dlg : m_dialogControlValues )
                    ret[ dlg.first ] = dlg.second;

                return ret;
            },
            [&]( const nlohmann::json& aVal )
            {
                m_dialogControlValues.clear();

                if( !aVal.is_object() )
                    return;

                for( auto& [dlgKey, dlgVal] : aVal.items() )
                {
                    if( !dlgVal.is_object() )
                        continue;

                    for( auto& [ctrlKey, ctrlVal] : dlgVal.items() )
                        m_dialogControlValues[ dlgKey ][ ctrlKey ] = ctrlVal;
                }
            },
            nlohmann::json::object() ) );


    registerMigration( 0, 1, std::bind( &COMMON_SETTINGS::migrateSchema0to1, this ) );
    registerMigration( 1, 2, std::bind( &COMMON_SETTINGS::migrateSchema1to2, this ) );
    registerMigration( 2, 3, std::bind( &COMMON_SETTINGS::migrateSchema2to3, this ) );
    registerMigration( 3, 4, std::bind( &COMMON_SETTINGS::migrateSchema3to4, this ) );
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
        wxLogTrace( traceSettings,
                    wxT( "COMMON_SETTINGS::Migrate 0->1: mousewheel_pan not found" ) );
    }

    if( mwp )
    {
        ( *m_internals )[nlohmann::json::json_pointer( "/input/horizontal_pan" )] = true;
        ( *m_internals )[nlohmann::json::json_pointer( "/input/scroll_modifier_pan_h" )] = WXK_SHIFT;
        ( *m_internals )[nlohmann::json::json_pointer( "/input/scroll_modifier_pan_v" )] = 0;
        ( *m_internals )[nlohmann::json::json_pointer( "/input/scroll_modifier_zoom" )] = WXK_CONTROL;
    }
    else
    {
        ( *m_internals )[nlohmann::json::json_pointer( "/input/horizontal_pan" )] = false;
        ( *m_internals )[nlohmann::json::json_pointer( "/input/scroll_modifier_pan_h" )] = WXK_CONTROL;
        ( *m_internals )[nlohmann::json::json_pointer( "/input/scroll_modifier_pan_v" )] = WXK_SHIFT;
        ( *m_internals )[nlohmann::json::json_pointer( "/input/scroll_modifier_zoom" )] = 0;
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
        m_internals->at( nlohmann::json::json_pointer( "/input"_json_pointer ) )
                .erase( "prefer_select_to_drag" );
    }
    catch( ... )
    {
        wxLogTrace( traceSettings,
                    wxT( "COMMON_SETTINGS::Migrate 1->2: prefer_select_to_drag not found" ) );
    }

    if( prefer_selection )
        ( *m_internals )[nlohmann::json::json_pointer( "/input/mouse_left" )] = MOUSE_DRAG_ACTION::SELECT;
    else
        ( *m_internals )[nlohmann::json::json_pointer( "/input/mouse_left" )] = MOUSE_DRAG_ACTION::DRAG_ANY;

    return true;
}


bool COMMON_SETTINGS::migrateSchema2to3()
{
    wxFileName cfgpath;
    cfgpath.AssignDir( PATHS::GetUserSettingsPath() );
    cfgpath.AppendDir( wxT( "3d" ) );
    cfgpath.SetFullName( wxS( "3Dresolver.cfg" ) );
    cfgpath.MakeAbsolute();

    std::vector<LEGACY_3D_SEARCH_PATH> legacyPaths;
    readLegacy3DResolverCfg( cfgpath.GetFullPath(), legacyPaths );

    // env variables have a limited allowed character set for names
    wxRegEx nonValidCharsRegex( wxS( "[^A-Z0-9_]+" ), wxRE_ADVANCED );

    for( const LEGACY_3D_SEARCH_PATH& path : legacyPaths )
    {
        wxString key = path.m_Alias;
        const wxString& val = path.m_Pathvar;

        // The 3d alias config didn't use the same naming restrictions as real env variables
        // We need to sanitize them

        // upper case only
        key.MakeUpper();

        // logically swap - with _
        key.Replace( wxS( "-" ), wxS( "_" ) );

        // remove any other chars
        nonValidCharsRegex.Replace( &key, wxEmptyString );

        if( !m_Env.vars.count( key ) )
        {
            wxLogTrace( traceEnvVars, wxS( "COMMON_SETTINGS: Loaded new var: %s = %s" ), key, val );
            m_Env.vars[key] = ENV_VAR_ITEM( key, val );
        }
    }

    if( cfgpath.FileExists() )
    {
        wxRemoveFile( cfgpath.GetFullPath() );
    }

    return true;
}


bool COMMON_SETTINGS::migrateSchema3to4()
{
    // >= 10 = add 1
    try
    {
        // Update netclass panel shown columns for eeschema
        const nlohmann::json::json_pointer v3_pointer_eeschema( "/netclass_panel/eeschema_shown_columns"_json_pointer );
        wxString                           eeSchemaColumnList_old = m_internals->at( v3_pointer_eeschema );

        wxStringTokenizer eeSchemaShownTokens( eeSchemaColumnList_old, " \t\r\n" );
        wxString          eeSchemaColumnList_new;

        while( eeSchemaShownTokens.HasMoreTokens() )
        {
            long colNumber;
            eeSchemaShownTokens.GetNextToken().ToLong( &colNumber );

            if( colNumber >= 10 )
                ++colNumber;

            eeSchemaColumnList_new += wxString::Format( wxT( "%ld " ), colNumber );
        }

        eeSchemaColumnList_new.Trim( true );
        eeSchemaColumnList_new.Trim( false );

        m_internals->at( v3_pointer_eeschema ) = eeSchemaColumnList_new.ToUTF8();

        // Update netclass panel shown columns for pcbnew
        const nlohmann::json::json_pointer v3_pointer_pcbnew( "/netclass_panel/pcbnew_shown_columns"_json_pointer );
        wxString                           pcbnewColumnList_old = m_internals->at( v3_pointer_pcbnew );

        wxStringTokenizer pcbnewShownTokens( pcbnewColumnList_old, " \t\r\n" );
        wxString          pcbnewColumnList_new;

        while( pcbnewShownTokens.HasMoreTokens() )
        {
            long colNumber;
            pcbnewShownTokens.GetNextToken().ToLong( &colNumber );

            if( colNumber >= 10 )
                ++colNumber;

            pcbnewColumnList_new += wxString::Format( wxT( "%ld " ), colNumber );
        }

        pcbnewColumnList_new.Trim( true );
        pcbnewColumnList_new.Trim( false );

        m_internals->at( v3_pointer_pcbnew ) = pcbnewColumnList_new.ToUTF8();
    }
    catch( ... )
    {
        wxLogTrace( traceSettings, wxT( "COMMON_SETTINGS::Migrate 3->4: /netclass_panel/shown_columns not found" ) );
    }

    return true;
}


bool COMMON_SETTINGS::MigrateFromLegacy( wxConfigBase* aCfg )
{
    bool ret = true;

    ret &= fromLegacy<double>( aCfg, "CanvasScale",             "appearance.canvas_scale" );
    ret &= fromLegacy<int>(    aCfg, "IconScale",               "appearance.icon_scale" );
    ret &= fromLegacy<bool>(   aCfg, "UseIconsInMenus",         "appearance.use_icons_in_menus" );
    ret &= fromLegacy<bool>(   aCfg, "ShowEnvVarWarningDialog", "environment.show_warning_dialog" );

    auto load_env_vars =
            [&]()
            {
                wxString key, value;
                long index = 0;
                nlohmann::json::json_pointer ptr = m_internals->PointerFromString( "environment.vars" );

                aCfg->SetPath( "EnvironmentVariables" );
                ( *m_internals )[ptr] = nlohmann::json( {} );

                while( aCfg->GetNextEntry( key, index ) )
                {
                    if( versionedEnvVarRegex.Matches( key ) )
                    {
                        wxLogTrace( traceSettings,
                                    wxT( "Migrate Env: %s is blacklisted; skipping." ), key );
                        continue;
                    }

                    value = aCfg->Read( key, wxEmptyString );

                    if( !value.IsEmpty() )
                    {
                        ptr.push_back( key.ToStdString() );

                        wxLogTrace( traceSettings, wxT( "Migrate Env: %s=%s" ),
                                    ptr.to_string(), value );
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
    if( std::optional<bool> value = Get<bool>( "input.center_on_zoom" ) )
        Set( "input.center_on_zoom", !( *value ) );

    ret &= fromLegacy<int>( aCfg, "OpenGLAntialiasingMode", "graphics.opengl_antialiasing_mode" );
    ret &= fromLegacy<int>( aCfg, "CairoAntialiasingMode",  "graphics.cairo_antialiasing_mode" );

    ret &= fromLegacy<int>(  aCfg, "AutoSaveInterval",        "system.local_history_debounce" );
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
                            wxS( "InitializeEnvironment: Entry %s defined externally as %s" ), aKey,
                            envValue );
            }
            else
            {
                wxLogTrace( traceEnvVars, wxS( "InitializeEnvironment: Setting entry %s to "
                                               "default %s" ),
                            aKey, aDefault );
            }
        };

    wxFileName basePath( PATHS::GetStockEDALibraryPath(), wxEmptyString );

    wxFileName path( basePath );
    path.AppendDir( wxT( "footprints" ) );
    addVar( ENV_VAR::GetVersionedEnvVarName( wxS( "FOOTPRINT_DIR" ) ), path.GetFullPath() );

    path = basePath;
    path.AppendDir( wxT( "3dmodels" ) );
    addVar( ENV_VAR::GetVersionedEnvVarName( wxS( "3DMODEL_DIR" ) ), path.GetFullPath() );

    addVar( ENV_VAR::GetVersionedEnvVarName( wxS( "TEMPLATE_DIR" ) ),
            PATHS::GetStockTemplatesPath() );

    addVar( wxT( "KICAD_USER_TEMPLATE_DIR" ), PATHS::GetUserTemplatesPath() );

    addVar( ENV_VAR::GetVersionedEnvVarName( wxS( "3RD_PARTY" ) ),
            PATHS::GetDefault3rdPartyPath() );

    path = basePath;
    path.AppendDir( wxT( "symbols" ) );
    addVar( ENV_VAR::GetVersionedEnvVarName( wxS( "SYMBOL_DIR" ) ), path.GetFullPath() );

    path = basePath;
    path.AppendDir( wxT( "blocks" ) );
    addVar( ENV_VAR::GetVersionedEnvVarName( wxS( "DESIGN_BLOCK_DIR" ) ), path.GetFullPath() );
}


bool COMMON_SETTINGS::readLegacy3DResolverCfg( const wxString&                   path,
                                               std::vector<LEGACY_3D_SEARCH_PATH>& aSearchPaths )
{
    wxFileName cfgpath( path );

    // This should be the same as wxWidgets 3.0 wxPATH_NORM_ALL which is deprecated in 3.1.
    // There are known issues with environment variable expansion so maybe we should be using
    // our own ExpandEnvVarSubstitutions() here instead.
    cfgpath.Normalize( FN_NORMALIZE_FLAGS | wxPATH_NORM_ENV_VARS );
    wxString cfgname = cfgpath.GetFullPath();

    std::ifstream cfgFile;
    std::string   cfgLine;

    if( !wxFileName::Exists( cfgname ) )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = "no 3D configuration file";
        ostr << " * " << errmsg.ToUTF8() << " '";
        ostr << cfgname.ToUTF8() << "'";
        wxLogTrace( traceSettings, "%s\n", ostr.str().c_str() );
        return false;
    }

    cfgFile.open( cfgname.ToUTF8() );

    if( !cfgFile.is_open() )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = wxS( "Could not open configuration file" );
        ostr << " * " << errmsg.ToUTF8() << " '" << cfgname.ToUTF8() << "'";
        wxLogTrace( traceSettings, wxS( "%s\n" ), ostr.str().c_str() );
        return false;
    }

    int         lineno = 0;
    LEGACY_3D_SEARCH_PATH al;
    size_t      idx;
    int         vnum = 0; // version number

    while( cfgFile.good() )
    {
        cfgLine.clear();
        std::getline( cfgFile, cfgLine );
        ++lineno;

        if( cfgLine.empty() )
        {
            if( cfgFile.eof() )
                break;

            continue;
        }

        if( 1 == lineno && cfgLine.compare( 0, 2, "#V" ) == 0 )
        {
            // extract the version number and parse accordingly
            if( cfgLine.size() > 2 )
            {
                std::istringstream istr;
                istr.str( cfgLine.substr( 2 ) );
                istr >> vnum;
            }

            continue;
        }

        idx = 0;

        if( !getLegacy3DHollerith( cfgLine, idx, al.m_Alias ) )
            continue;

        // Don't add KICADn_3DMODEL_DIR, one of its legacy equivalents, or KIPRJMOD from a
        // config file.  They're system variables which are defined at runtime.
        wxString versionedPath = wxString::Format( wxS( "${%s}" ),
                                       ENV_VAR::GetVersionedEnvVarName( wxS( "3DMODEL_DIR" ) ) );

        if( al.m_Alias == versionedPath || al.m_Alias == wxS( "${KIPRJMOD}" )
            || al.m_Alias == wxS( "$(KIPRJMOD)" ) || al.m_Alias == wxS( "${KISYS3DMOD}" )
            || al.m_Alias == wxS( "$(KISYS3DMOD)" ) )
        {
            continue;
        }

        if( !getLegacy3DHollerith( cfgLine, idx, al.m_Pathvar ) )
            continue;

        if( !getLegacy3DHollerith( cfgLine, idx, al.m_Description ) )
            continue;

        aSearchPaths.push_back( al );
    }

    cfgFile.close();

    return true;
}


bool COMMON_SETTINGS::getLegacy3DHollerith( const std::string& aString, size_t& aIndex,
                                            wxString& aResult )
{
    aResult.clear();

    if( aIndex >= aString.size() )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = wxS( "bad Hollerith string on line" );
        ostr << " * " << errmsg.ToUTF8() << "\n'" << aString << "'";
        wxLogTrace( traceSettings, wxS( "%s\n" ), ostr.str().c_str() );

        return false;
    }

    size_t i2 = aString.find( '"', aIndex );

    if( std::string::npos == i2 )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = wxS( "missing opening quote mark in config file" );
        ostr << " * " << errmsg.ToUTF8() << "\n'" << aString << "'";
        wxLogTrace( traceSettings, wxS( "%s\n" ), ostr.str().c_str() );

        return false;
    }

    ++i2;

    if( i2 >= aString.size() )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = wxS( "invalid entry (unexpected end of line)" );
        ostr << " * " << errmsg.ToUTF8() << "\n'" << aString << "'";
        wxLogTrace( traceSettings, wxS( "%s\n" ), ostr.str().c_str() );

        return false;
    }

    std::string tnum;

    while( aString[i2] >= '0' && aString[i2] <= '9' )
        tnum.append( 1, aString[i2++] );

    if( tnum.empty() || aString[i2++] != ':' )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = wxS( "bad Hollerith string on line" );
        ostr << " * " << errmsg.ToUTF8() << "\n'" << aString << "'";
        wxLogTrace( traceSettings, wxS( "%s\n" ), ostr.str().c_str() );

        return false;
    }

    std::istringstream istr;
    istr.str( tnum );
    size_t nchars;
    istr >> nchars;

    if( ( i2 + nchars ) >= aString.size() )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = wxS( "invalid entry (unexpected end of line)" );
        ostr << " * " << errmsg.ToUTF8() << "\n'" << aString << "'";
        wxLogTrace( traceSettings, wxS( "%s\n" ), ostr.str().c_str() );

        return false;
    }

    if( nchars > 0 )
    {
        aResult = wxString::FromUTF8( aString.substr( i2, nchars ).c_str() );
        i2 += nchars;
    }

    if( i2 >= aString.size() || aString[i2] != '"' )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = wxS( "missing closing quote mark in config file" );
        ostr << " * " << errmsg.ToUTF8() << "\n'" << aString << "'";
        wxLogTrace( traceSettings, wxS( "%s\n" ), ostr.str().c_str() );

        return false;
    }

    aIndex = i2 + 1;
    return true;
}
