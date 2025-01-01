/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef KICAD_ENVIRONMENT_H
#define KICAD_ENVIRONMENT_H

#include <map>
#include <wx/string.h>

/**
 * KiCad uses environment variables internally for determining the base paths for libraries,
 * templates, and other assets that can be relocated by packagers or users.
 *
 * Because setting environment variables is not user-friendly on most platforms, KiCad supports two
 * backing stores for these internal variables: the system environment, and the settings system.
 *
 * We also want to make it possible to change the names and values of environment variables over
 * time with minimal impact to users.  Since most users do not customize these variables beyond any
 * customization provided by the packager for their platform, an easy way to get this possibility
 * with minimal user impact is to just not store environment variables if they match the internal
 * (compiled-in) default.
 *
 * The way environment variables are resolved is (highest to lowest priority):
 *
 * 1) Variables set at runtime via the Configure Paths dialog
 * 2) Variables set in the system environment
 * 3) Variables loaded from the settings system (stored in COMMON_SETTINGS)
 *
 * For all KiCad system variables, we allow users to change the values at runtime via the Configure
 * Paths dialog.  If these variables were set in the system environment, we do not persist any
 * changes made at runtime (and warn the user about this).  If the variables were not set in the
 * environment (meaning they were either the default value, or loaded from the settings system),
 * we persist the changes via the settings system.  Any variables that match the internal default
 * are not saved in the settings, so that the internal defaults can be changed and the change will
 * not be overridden by an old value cached in the settings file.
 */

/**
 * A simple helper class to store environment variable definitions and values.  This is used to
 * initialize the environment variables that are built-in to KiCad, and also to store any variables
 * created by the user.
 */
class ENV_VAR_ITEM
{
public:
    ENV_VAR_ITEM( const wxString& aValue = wxEmptyString, bool aIsDefinedExternally = false ) :
            m_value( aValue ),
            m_isBuiltin( true ),
            m_isDefinedExternally( aIsDefinedExternally ),
            m_isDefinedInSettings( false )
    {
    }

    ENV_VAR_ITEM( const wxString& aKey, const wxString& aValue,
                  const wxString& aDefaultValue = wxEmptyString ) :
            m_key( aKey ),
            m_value( aValue ),
            m_defaultValue( aDefaultValue ),
            m_isBuiltin( true ),
            m_isDefinedExternally( false ),
            m_isDefinedInSettings( false )
    {
    }

    ~ENV_VAR_ITEM() throw() {}    // tell SWIG no exception

    bool GetDefinedExternally() const { return m_isDefinedExternally; }
    void SetDefinedExternally( bool aIsDefinedExternally = true )
    {
        m_isDefinedExternally = aIsDefinedExternally;
    }

    bool GetDefinedInSettings() const { return m_isDefinedInSettings; }
    void SetDefinedInSettings( bool aDefined = true ) { m_isDefinedInSettings = aDefined; }

    wxString GetKey() const { return m_key; }

    const wxString& GetValue() const { return m_value; }
    void SetValue( const wxString& aValue ) { m_value = aValue; }

    wxString GetDefault() const { return m_defaultValue; }

    wxString GetSettingsValue() const { return m_settingsValue; }
    void SetSettingsValue( const wxString& aValue ) { m_settingsValue = aValue; }

    bool GetBuiltin() const { return m_isBuiltin; }

    /**
     * Checks if the variable matches its default value (always false for non-built-in vars)
     * @return true if a built-in variable matches its default
     */
    bool IsDefault() const
    {
        return m_isBuiltin && m_value == m_defaultValue;
    }

private:
    /// The environment variable string key.
    wxString m_key;

    /// The environment variable string value.
    wxString m_value;

    /// The default value, for built-in variables that are always defined.
    wxString m_defaultValue;

    /// The value that was originally loaded from JSON
    wxString m_settingsValue;

    /// Set to true for KiCad built-in variables that are always defined one way or another.
    bool m_isBuiltin;

    /// Flag to indicate if the environment variable was defined externally to the process.
    bool m_isDefinedExternally;

    /// Flag to indicate if the environment variable was defined in the settings file.
    bool m_isDefinedInSettings;
};

typedef std::map<wxString, ENV_VAR_ITEM>                 ENV_VAR_MAP;
typedef std::map<wxString, ENV_VAR_ITEM>::iterator       ENV_VAR_MAP_ITER;
typedef std::map<wxString, ENV_VAR_ITEM>::const_iterator ENV_VAR_MAP_CITER;

#endif // KICAD_ENVIRONMENT_H
