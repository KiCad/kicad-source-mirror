/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <env_vars.h>

#include <map>

#include <common.h>

#include <wx/utils.h>

using STRING_MAP = std::map<wxString, wxString>;

/*
 * List of pre-defined environment variables
 *
 * TODO - Instead of defining these values here,
 * extract them from elsewhere in the program
 * (where they are originally defined)
 */
static const ENV_VAR_LIST predefined_env_vars = {
    "KIPRJMOD",
    "KICAD_SYMBOL_DIR",
    "KISYS3DMOD",
    "KISYSMOD",
    "KIGITHUB",
    "KICAD_TEMPLATE_DIR",
    "KICAD_USER_TEMPLATE_DIR",
    "KICAD_PTEMPLATES",
};


bool IsEnvVarImmutable( const wxString& aEnvVar )
{
    for( const auto& s: predefined_env_vars )
    {
        if( s == aEnvVar )
            return true;
    }

    return false;
}


const ENV_VAR_LIST& GetPredefinedEnvVars()
{
    return predefined_env_vars;
}


void initialiseEnvVarHelp( STRING_MAP& aMap )
{
    // Set up dynamically, as we want to be able to use _() translations,
    // which can't be done statically
    aMap["KISYSMOD"] =
        _( "The base path of locally installed system "
            "footprint libraries (.pretty folders).");
    aMap["KISYS3DMOD"] =
        _( "The base path of system footprint 3D shapes (.3Dshapes folders).");
    aMap["KICAD_SYMBOL_DIR"] =
        _( "The base path of the locally installed symbol libraries.");
    aMap["KIGITHUB"] =
        _( "Used by KiCad to define the URL of the repository "
            "of the official KiCad footprint libraries.");
    aMap["KICAD_TEMPLATE_DIR"] =
        _( "A directory containing project templates installed with KiCad.");
    aMap["KICAD_USER_TEMPLATE_DIR"] =
        _( "Optional. Can be defined if you want to create your own project "
           "templates folder.");
    aMap["KIPRJMOD"] =
        _("Internally defined by KiCad (cannot be edited) and is set "
          "to the absolute path of the currently loaded project file.  This environment "
          "variable can be used to define files and paths relative to the currently loaded "
          "project.  For instance, ${KIPRJMOD}/libs/footprints.pretty can be defined as a "
          "folder containing a project specific footprint library named footprints.pretty." );

    // Deprecated vars
    aMap["KICAD_PTEMPLATES"] =
        _( "Deprecated version of KICAD_TEMPLATE_DIR.");
}


wxString LookUpEnvVarHelp( const wxString& aEnvVar )
{
    static STRING_MAP env_var_help_text;

    if( env_var_help_text.size() == 0 )
        initialiseEnvVarHelp( env_var_help_text );

    return env_var_help_text[aEnvVar];
}


template<>
OPT<double> GetEnvVar( const wxString& aEnvVarName )
{
    OPT<double> opt_value;

    wxString env;
    if( wxGetEnv( aEnvVarName, &env ) )
    {
        double value;
        if( env.ToDouble( &value ) )
        {
            opt_value = value;
        }
    }

    return opt_value;
}

template<>
OPT<wxString> GetEnvVar( const wxString& aEnvVarName )
{
    OPT<wxString> opt_value;

    wxString env;
    if( wxGetEnv( aEnvVarName, &env ) )
    {
        opt_value = env;
    }

    return opt_value;
}