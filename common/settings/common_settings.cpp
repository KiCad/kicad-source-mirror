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

#include <settings/common_settings.h>
#include <settings/parameters.h>
#include <wx/config.h>
#include <wx/log.h>


extern const char* traceSettings;


///! Update the schema version whenever a migration is required
const int commonSchemaVersion = 0;

COMMON_SETTINGS::COMMON_SETTINGS() :
        JSON_SETTINGS( "kicad_common", SETTINGS_LOC::USER, commonSchemaVersion ),
        m_Appearance(), m_Env(), m_Input(), m_Graphics(), m_System()
{
    // This only effect the first time KiCad is run.  The user's setting will be used for all
    // subsequent runs.  Menu icons are off by default on OSX and on for all other platforms.
#if defined( __WXMAC__ )
    bool defaultUseIconsInMenus = false;
#else
    bool defaultUseIconsInMenus = true;
#endif

    m_params.emplace_back( new PARAM<double>( "appearance.canvas_scale",
            &m_Appearance.canvas_scale, 1.0 ) );

    m_params.emplace_back( new PARAM<int>( "appearance.icon_scale",
            &m_Appearance.icon_scale, 0 ) );

    m_params.emplace_back( new PARAM<bool>( "appearance.use_icons_in_menus",
            &m_Appearance.use_icons_in_menus, defaultUseIconsInMenus ) );

    m_params.emplace_back( new PARAM<bool>( "environment.show_warning_dialog",
                                         &m_Env.show_warning_dialog, false ) );

    m_params.emplace_back( new PARAM_MAP<wxString>( "environment.vars", &m_Env.vars, {} ) );

    m_params.emplace_back( new PARAM<bool>( "input.auto_pan", &m_Input.auto_pan, false ) );

    m_params.emplace_back(
            new PARAM<bool>( "input.center_on_zoom", &m_Input.center_on_zoom, true ) );

    m_params.emplace_back( new PARAM<bool>( "input.immediate_actions",
            &m_Input.immediate_actions, true ) );

    m_params.emplace_back(
            new PARAM<bool>( "input.mousewheel_pan", &m_Input.mousewheel_pan, false ) );

    m_params.emplace_back( new PARAM<bool>( "input.prefer_select_to_drag",
            &m_Input.prefer_select_to_drag, true ) );

    m_params.emplace_back( new PARAM<bool>( "input.warp_mouse_on_move",
            &m_Input.warp_mouse_on_move, true ) );

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
}


bool COMMON_SETTINGS::MigrateFromLegacy( wxConfigBase* aCfg )
{
    bool ret = true;

    ret &= fromLegacy<double>( aCfg, "CanvasScale",     "appearance.canvas_scale" );
    ret &= fromLegacy<int>( aCfg,    "IconScale",       "appearance.icon_scale" );
    ret &= fromLegacy<bool>( aCfg,   "UseIconsInMenus", "appearance.use_icons_in_menus" );

    ret &= fromLegacy<bool>( aCfg, "ShowEnvVarWarningDialog", "environment.show_warning_dialog" );

    auto load_env_vars = [&] () {
          wxString key, value;
          long index = 0;
          nlohmann::json::json_pointer ptr = PointerFromString( "environment.vars" );

          aCfg->SetPath( "EnvironmentVariables" );
          ( *this )[ptr] = nlohmann::json( {} );

          while( aCfg->GetNextEntry( key, index ) )
          {
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

    ret &= fromLegacy<bool>( aCfg, "AutoPAN",                   "input.auto_pan" );
    ret &= fromLegacy<bool>( aCfg, "ImmediateActions",          "input.immediate_actions" );
    ret &= fromLegacy<bool>( aCfg, "MousewheelPAN",             "input.mousewheel_pan" );
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