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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
     * Test whether a name is a versioned KiCad environment variable for the given base,
     * i.e. KICAD followed by one or more digits and then _<aBaseName> (KICAD8_FOOTPRINT_DIR).
     *
     * Unlike a glob such as "KICAD*_FOOTPRINT_DIR", this rejects user-defined names that merely
     * share the suffix, like KICAD_USER_FOOTPRINT_DIR, so they are never silently treated as a
     * built-in library path.
     *
     * @param aName is the variable name to test.
     * @param aBaseName is the suffix, like FOOTPRINT_DIR.
     */
    KICOMMON_API bool IsVersionedEnvVar( const wxString& aName, const wxString& aBaseName );

    /**
     * Attempt to retrieve the value of a versioned environment variable, such as
     * KICAD8_TEMPLATE_DIR.
     *
     * If this value exists in the map, it will be returned.  If not, the map will be searched
     * for keys that are versioned variants of <aBaseName> (@see IsVersionedEnvVar), and the
     * first match's value will be returned.  If there are no matches, std::nullopt will be
     * returned.
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
