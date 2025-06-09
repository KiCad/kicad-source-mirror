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

#include <deque>
#include <memory>
#include <set>

#include <wx/event.h>

#include <api/api_plugin.h>
#include <json_schema_validator.h>
#include <kicommon.h>

class wxTimer;

/// Internal event used for handling async tasks
wxDECLARE_EVENT( EDA_EVT_PLUGIN_MANAGER_JOB_FINISHED, wxCommandEvent );

/// Notifies other parts of KiCad when plugin availability changes
extern const KICOMMON_API wxEventTypeTag<wxCommandEvent> EDA_EVT_PLUGIN_AVAILABILITY_CHANGED;

/**
 * Responsible for loading plugin definitions for API-based plugins (ones that do not run inside
 * KiCad itself, but instead are launched as external processes by KiCad)
 */
class KICOMMON_API API_PLUGIN_MANAGER : public wxEvtHandler
{
public:
    API_PLUGIN_MANAGER( wxEvtHandler* aParent );

    void ReloadPlugins();

    void RecreatePluginEnvironment( const wxString& aIdentifier );

    void InvokeAction( const wxString& aIdentifier );

    std::optional<const PLUGIN_ACTION*> GetAction( const wxString& aIdentifier );

    std::vector<const PLUGIN_ACTION*> GetActionsForScope( PLUGIN_ACTION_SCOPE aScope );

    std::map<int, wxString>& ButtonBindings() { return m_buttonBindings; }

    std::map<int, wxString>& MenuBindings() { return m_menuBindings; }

private:
    void processPluginDependencies();

    void processNextJob( wxCommandEvent& aEvent );

    wxEvtHandler* m_parent;

    std::set<std::unique_ptr<API_PLUGIN>, CompareApiPluginIdentifiers> m_plugins;

    std::map<wxString, const API_PLUGIN*> m_pluginsCache;

    std::map<wxString, const PLUGIN_ACTION*> m_actionsCache;

    /// Map of plugin identifier to a path for the plugin's virtual environment, if it has one
    std::map<wxString, wxString> m_environmentCache;

    /// Map of button wx item id to action identifier
    std::map<int, wxString> m_buttonBindings;

    /// Map of menu wx item id to action identifier
    std::map<int, wxString> m_menuBindings;

    std::set<wxString> m_readyPlugins;

    std::set<wxString> m_busyPlugins;

    enum class JOB_TYPE
    {
        CREATE_ENV,
        SETUP_ENV,
        INSTALL_REQUIREMENTS
    };

    struct JOB
    {
        JOB_TYPE type;
        wxString identifier;
        wxString plugin_path;
        wxString env_path;
    };

    std::deque<JOB> m_jobs;

    std::unique_ptr<JSON_SCHEMA_VALIDATOR> m_schema_validator;

    [[maybe_unused]] long     m_lastPid;
    [[maybe_unused]] wxTimer* m_raiseTimer;
};
