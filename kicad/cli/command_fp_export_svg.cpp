/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
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

#include "command_fp_export_svg.h"
#include <cli/exit_codes.h>
#include "jobs/job_fp_export_svg.h"
#include <kiface_base.h>
#include <string_utils.h>
#include <wx/crt.h>
#include <wx/dir.h>

#include <macros.h>

#define ARG_FOOTPRINT "--footprint"

CLI::FP_EXPORT_SVG_COMMAND::FP_EXPORT_SVG_COMMAND() :
        PCB_EXPORT_BASE_COMMAND( "svg", true, true )
{
    m_argParser.add_description( UTF8STDSTR( _( "Exports the footprint or entire footprint "
                                                "library to SVG" ) ) );

    addLayerArg();
    addDefineArg();

    m_argParser.add_argument( "-t", ARG_THEME )
            .default_value( std::string() )
            .help( UTF8STDSTR( _( "Color theme to use (will default to footprint editor "
                                  "settings)" ) ) );

    m_argParser.add_argument( "--fp", ARG_FOOTPRINT )
            .default_value( std::string() )
            .help( UTF8STDSTR( _( "Specific footprint to export within the library" ) ) )
            .metavar( "FOOTPRINT_NAME" );

    m_argParser.add_argument( "--sp", ARG_SKETCH_PADS_ON_FAB_LAYERS )
            .help( UTF8STDSTR( _( ARG_SKETCH_PADS_ON_FAB_LAYERS_DESC ) ) )
            .flag();

    m_argParser.add_argument( "--hdnp", ARG_HIDE_DNP_FPS_ON_FAB_LAYERS )
            .help( UTF8STDSTR( _( ARG_HIDE_DNP_FPS_ON_FAB_LAYERS_DESC ) ) )
            .flag();

    m_argParser.add_argument( "--sdnp", ARG_SKETCH_DNP_FPS_ON_FAB_LAYERS )
            .help( UTF8STDSTR( _( ARG_SKETCH_DNP_FPS_ON_FAB_LAYERS_DESC ) ) )
            .flag();

    m_argParser.add_argument( "--cdnp", ARG_CROSSOUT_DNP_FPS_ON_FAB_LAYERS )
            .help( UTF8STDSTR( _( ARG_CROSSOUT_DNP_FPS_ON_FAB_LAYERS_DESC ) ) )
            .flag();

    m_argParser.add_argument( ARG_BLACKANDWHITE )
            .help( UTF8STDSTR( _( ARG_BLACKANDWHITE_DESC ) ) )
            .flag();
}


int CLI::FP_EXPORT_SVG_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_FP_EXPORT_SVG> svgJob = std::make_unique<JOB_FP_EXPORT_SVG>();

    svgJob->m_libraryPath = m_argInput;
    svgJob->SetConfiguredOutputPath( m_argOutput );
    svgJob->m_blackAndWhite = m_argParser.get<bool>( ARG_BLACKANDWHITE );
    svgJob->m_sketchPadsOnFabLayers = m_argParser.get<bool>( ARG_SKETCH_PADS_ON_FAB_LAYERS );
    svgJob->m_hideDNPFPsOnFabLayers = m_argParser.get<bool>( ARG_HIDE_DNP_FPS_ON_FAB_LAYERS );
    svgJob->m_sketchDNPFPsOnFabLayers = m_argParser.get<bool>( ARG_SKETCH_DNP_FPS_ON_FAB_LAYERS );
    svgJob->m_crossoutDNPFPsOnFabLayers = m_argParser.get<bool>( ARG_CROSSOUT_DNP_FPS_ON_FAB_LAYERS );
    svgJob->m_footprint = From_UTF8( m_argParser.get<std::string>( ARG_FOOTPRINT ).c_str() );
    svgJob->SetVarOverrides( m_argDefineVars );

    if( !wxDir::Exists( svgJob->m_libraryPath ) )
    {
        wxFprintf( stderr, _( "Footprint library does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    svgJob->m_colorTheme = From_UTF8( m_argParser.get<std::string>( ARG_THEME ).c_str() );

    svgJob->m_argLayers = From_UTF8( m_argParser.get<std::string>( ARG_LAYERS ).c_str() );

    return aKiway.ProcessJob( KIWAY::FACE_PCB, svgJob.get() );
}
