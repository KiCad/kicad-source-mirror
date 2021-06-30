/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/translation.h>
#include <wx/utils.h>

using STRING_MAP = std::map<wxString, wxString>;

/*
 * List of pre-defined environment variables
 *
 * TODO - Instead of defining these values here,
 * extract them from elsewhere in the program
 * (where they are originally defined)
 */
static const ENV_VAR::ENV_VAR_LIST predefinedEnvVars = {
    "KIPRJMOD",
    "KICAD6_SYMBOL_DIR",
    "KICAD6_3DMODEL_DIR",
    "KICAD6_FOOTPRINT_DIR",
    "KICAD6_TEMPLATE_DIR",
    "KICAD_USER_TEMPLATE_DIR",
    "KICAD_PTEMPLATES",
};


bool ENV_VAR::IsEnvVarImmutable( const wxString& aEnvVar )
{
    for( const auto& s : predefinedEnvVars )
    {
        if( s == aEnvVar )
            return true;
    }

    return false;
}


const ENV_VAR::ENV_VAR_LIST& ENV_VAR::GetPredefinedEnvVars()
{
    return predefinedEnvVars;
}


static void initialiseEnvVarHelp( STRING_MAP& aMap )
{
    // Set up dynamically, as we want to be able to use _() translations,
    // which can't be done statically
    aMap["KICAD6_FOOTPRINT_DIR"] =
        _( "The base path of locally installed system "
            "footprint libraries (.pretty folders).");
    aMap["KICAD6_3DMODEL_DIR"] =
        _( "The base path of system footprint 3D shapes (.3Dshapes folders).");
    aMap["KICAD6_SYMBOL_DIR"] =
        _( "The base path of the locally installed symbol libraries.");
    aMap["KICAD6_TEMPLATE_DIR"] =
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
    aMap["KICAD6_SCRIPTING_DIR"] =
        _( "A directory containing system-wide scripts installed with KiCad" );
    aMap["KICAD6_USER_SCRIPTING_DIR"] =
        _( "A directory containing user-specific scripts installed with KiCad" );

    // Deprecated vars
    aMap["KICAD_PTEMPLATES"] =
        _( "Deprecated version of KICAD_TEMPLATE_DIR.");
    aMap["KISYS3DMOD"] =
        _( "Deprecated version of KICAD6_3DMODEL_DIR." );
    aMap["KISYSMOD"] =
        _( "Deprecated version of KICAD6_FOOTPRINT_DIR." );
    aMap["KICAD_SYMBOL_DIR"] =
        _( "Deprecated version of KICAD_SYMBOL_DIR.");
}


wxString ENV_VAR::LookUpEnvVarHelp( const wxString& aEnvVar )
{
    static STRING_MAP envVarHelpText;

    if( envVarHelpText.size() == 0 )
        initialiseEnvVarHelp( envVarHelpText );

    return envVarHelpText[ aEnvVar ];
}


template<>
OPT<double> ENV_VAR::GetEnvVar( const wxString& aEnvVarName )
{
    OPT<double> optValue = NULLOPT;

    wxString env;

    if( wxGetEnv( aEnvVarName, &env ) )
    {
        double value;

        if( env.ToDouble( &value ) )
        {
            optValue = value;
        }
    }

    return optValue;
}


template<>
OPT<wxString> ENV_VAR::GetEnvVar( const wxString& aEnvVarName )
{
    OPT<wxString> optValue;

    wxString env;

    if( wxGetEnv( aEnvVarName, &env ) )
    {
        optValue = env;
    }

    return optValue;
}
