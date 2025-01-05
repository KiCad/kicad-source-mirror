/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Jon Evans <jon@craftyjon.com>
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

#ifndef KICAD_API_PLUGIN_H
#define KICAD_API_PLUGIN_H

#include <memory>
#include <optional>
#include <set>
#include <nlohmann/json_fwd.hpp>
#include <wx/bmpbndl.h>
#include <wx/filename.h>
#include <wx/string.h>

#include <kicommon.h>


struct API_PLUGIN_CONFIG;
class API_PLUGIN;
class JSON_SCHEMA_VALIDATOR;


struct PLUGIN_DEPENDENCY
{
    wxString package_name;
    wxString version;
};


enum class PLUGIN_RUNTIME_TYPE
{
    INVALID,
    PYTHON,
    EXEC
};


enum class PLUGIN_ACTION_SCOPE
{
    INVALID,
    PCB,
    SCHEMATIC,
    FOOTPRINT,
    SYMBOL,
    PROJECT_MANAGER
};


struct PLUGIN_RUNTIME
{
    bool FromJson( const nlohmann::json& aJson );

    PLUGIN_RUNTIME_TYPE type;
    wxString min_version;
    std::vector<PLUGIN_DEPENDENCY> dependencies;
};


/**
 * An action performed by a plugin via the IPC API
 * (not to be confused with ACTION_PLUGIN, the old SWIG plugin system, which will be removed
 * in the future)
 */
struct PLUGIN_ACTION
{
    PLUGIN_ACTION( const API_PLUGIN& aPlugin ) :
            plugin( aPlugin )
    {}

    wxString identifier;
    wxString name;
    wxString description;
    bool show_button = false;
    wxString entrypoint;
    std::set<PLUGIN_ACTION_SCOPE> scopes;
    std::vector<wxString> args;
    wxBitmapBundle icon_light;
    wxBitmapBundle icon_dark;

    const API_PLUGIN& plugin;
};

/**
 * A plugin that is invoked by KiCad and runs as an external process; communicating with KiCad
 * via the IPC API.  The plugin metadata is read from a JSON file containing things like which
 * actions the plugin is capable of and how to invoke each one.
 */
class KICOMMON_API API_PLUGIN
{
public:
    API_PLUGIN( const wxFileName& aConfigFile, const JSON_SCHEMA_VALIDATOR& aValidator );

    ~API_PLUGIN();

    bool IsOk() const;

    static bool IsValidIdentifier( const wxString& aIdentifier );

    const wxString& Identifier() const;
    const wxString& Name() const;
    const wxString& Description() const;
    const PLUGIN_RUNTIME& Runtime() const;
    wxString BasePath() const;

    const std::vector<PLUGIN_ACTION>& Actions() const;

    wxString ActionSettingsKey( const PLUGIN_ACTION& aAction ) const;

private:
    friend struct API_PLUGIN_CONFIG;

    std::optional<PLUGIN_ACTION> createActionFromJson( const nlohmann::json& aJson );

    wxFileName m_configFile;

    std::unique_ptr<API_PLUGIN_CONFIG> m_config;
};

/**
 * Comparison functor for ensuring API_PLUGINs have unique identifiers
 */
struct CompareApiPluginIdentifiers
{
    bool operator()( const std::unique_ptr<API_PLUGIN>& item1,
                     const std::unique_ptr<API_PLUGIN>& item2 ) const
    {
        return item1->Identifier() < item2->Identifier();
    }
};

#endif //KICAD_API_PLUGIN_H
