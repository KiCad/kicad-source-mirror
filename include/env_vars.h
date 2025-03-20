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

/**
 * @file env_vars.h
 *
 * Functions related to environment variables, including help functions.
 */

#ifndef ENV_VARS_H
#define ENV_VARS_H

#include <kicommon.h>
#include <wx/string.h>
#include <wx/arrstr.h>
#include <map>
#include <vector>
#include <optional>

class ENV_VAR_ITEM;

namespace ENV_VAR
{
    /**
     * Determine if an environment variable is "predefined", i.e. if the
     * name of the variable is special to KiCad, and isn't just a user-specified
     * substitution name.
     *
     * @param  aEnvVar the variable to check.
     * @return         true if predefined.
     */
    KICOMMON_API bool IsEnvVarImmutable( const wxString& aEnvVar );

    /**
     * Get the list of pre-defined environment variables.
     */
    KICOMMON_API const std::vector<wxString>& GetPredefinedEnvVars();

    /**
     * Return autocomplete tokens for environment variables for Scintilla.
     */
    KICOMMON_API void GetEnvVarAutocompleteTokens( wxArrayString* aVars );

    /**
     * Construct a versioned environment variable based on this KiCad major version.
     *
     * @param aBaseName is the suffix, like TEMPLATE_DIR.
     * @return an environment variable name, like KICAD8_TEMPLATE_DIR.
     */
    KICOMMON_API wxString GetVersionedEnvVarName( const wxString& aBaseName );

    /**
     * Attempt to retrieve the value of a versioned environment variable, such as
     * KICAD8_TEMPLATE_DIR.
     *
     * If this value exists in the map, it will be returned.  If not, the map will be searched
     * for keys matching KICAD*_<aBaseName>, and the first match's value will be returned.  If
     * there are no matches, std::nullopt will be returned.
     *
     * @param aMap is an #ENV_VAR_MAP (@see environment.h).
     * @param aBaseName is the suffix for the environment variable (@see GetVersionedEnvVarName).
     */
    KICOMMON_API std::optional<wxString>
                 GetVersionedEnvVarValue( const std::map<wxString, ENV_VAR_ITEM>& aMap,
                                          const wxString&                         aBaseName );

    /**
     * Look up long-form help text for a given environment variable.
     *
     * This is intended for use in more verbose help resources (as opposed to tooltip text).
     *
     * @param  aEnvVar The variable to look up.
     * @return         A string with help for that variable. Empty if
     *                 no help available for this variable.
     */
    KICOMMON_API wxString LookUpEnvVarHelp( const wxString& aEnvVar );

    /**
     * Get an environment variable as a specific type, if set correctly.
     *
     * @param aEnvVarName the name of the environment variable.
     * @return an std::optional containing the value, if set and parseable, otherwise empty.
     */
    template <typename VAL_TYPE>
    KICOMMON_API std::optional<VAL_TYPE> GetEnvVar( const wxString& aEnvVarName );

    /**
     * Get a string environment variable, if it is set.
     *
     * @param aEnvVarName the name of the environment variable
     * @return an std::optional containing the value, if set, otherwise empty.
     */
    template<>
    KICOMMON_API std::optional<wxString> GetEnvVar( const wxString& aEnvVarName );

    /**
     * Get a double from an environment variable, if set.
     *
     * @param aEnvVarName the name of the environment variable
     * @return an std::optional containing the value, if set and parseable as a double,
     * otherwise empty.
     */
    template <>
    KICOMMON_API std::optional<double> GetEnvVar( const wxString& aEnvVarName );
};

#endif /* ENV_VARS_H */
