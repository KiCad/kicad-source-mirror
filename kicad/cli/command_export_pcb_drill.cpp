/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "command_export_pcb_drill.h"
#include <cli/exit_codes.h>
#include "jobs/job_export_pcb_drill.h"
#include <kiface_base.h>
#include <layer_ids.h>
#include <wx/crt.h>

#include <macros.h>
#include <wx/tokenzr.h>

#define ARG_FORMAT "--format"
#define ARG_EXCELLON_MIRRORY "--excellon-mirror-y"
#define ARG_EXCELLON_MINIMALHEAD "--excellon-min-header"
#define ARG_EXCELLON_SEPARATE_TH "--excellon-separate-th"
#define ARG_EXCELLON_ZEROS_FORMAT "--excellon-zeros-format"
#define ARG_GERBER_PRECISION "--gerber-precision"
#define ARG_EXCELLON_UNITS "--excellon-units"
#define ARG_GENERATE_MAP "--generate-map"
#define ARG_MAP_FORMAT "--map-format"
#define ARG_DRILL_ORIGIN "--drill-origin"

CLI::EXPORT_PCB_DRILL_COMMAND::EXPORT_PCB_DRILL_COMMAND() : EXPORT_PCB_BASE_COMMAND( "drill" )
{
    m_argParser.add_argument( ARG_FORMAT )
            .default_value( std::string( "excellon" ) )
            .help( UTF8STDSTR( _( "Valid options excellon, gerber." ) ) );

    m_argParser.add_argument( ARG_DRILL_ORIGIN )
            .default_value( std::string( "absolute" ) )
            .help( UTF8STDSTR( _( "Valid options are: absolute,plot" ) ) );

    m_argParser.add_argument( ARG_EXCELLON_ZEROS_FORMAT )
            .default_value( std::string( "decimal" ) )
            .help( UTF8STDSTR(
                    _( "Valid options are: decimal,suppressleading,suppresstrailing,keep." ) ) );

    m_argParser.add_argument( "-u", ARG_EXCELLON_UNITS )
            .default_value( std::string( "mm" ) )
            .help( UTF8STDSTR( _( "Output units, valid options:in,mm" ) ) );

    m_argParser.add_argument( ARG_EXCELLON_MIRRORY )
            .help( UTF8STDSTR( _( "Mirror Y axis" ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( ARG_EXCELLON_MINIMALHEAD )
            .help( UTF8STDSTR( _( "Minimal header" ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( ARG_EXCELLON_SEPARATE_TH )
            .help( UTF8STDSTR( _( "Generate independent files for NPTH and PTH holes" ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( ARG_GENERATE_MAP )
            .help( UTF8STDSTR( _( "Generate map / summary of drill hits" ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( ARG_MAP_FORMAT )
            .default_value( std::string( "pdf" ) )
            .help( UTF8STDSTR( _( "Valid options: pdf,gerberx2,ps,dxf,svg" ) ) );

    m_argParser.add_argument( ARG_GERBER_PRECISION )
            .help( UTF8STDSTR( _( "Precision of gerber coordinates (5 or 6)" ) ) )
            .default_value( 6 );
}


int CLI::EXPORT_PCB_DRILL_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_EXPORT_PCB_DRILL> drillJob( new JOB_EXPORT_PCB_DRILL( true ) );

    drillJob->m_filename = FROM_UTF8( m_argParser.get<std::string>( ARG_INPUT ).c_str() );
    drillJob->m_outputDir = FROM_UTF8( m_argParser.get<std::string>( ARG_OUTPUT ).c_str() );

    if( !drillJob->m_outputDir.IsEmpty() )
    {
        wxFileName fn( drillJob->m_outputDir );
        if( !fn.IsDir() )
        {
            wxFprintf( stderr, _( "Output must be a directory\n" ) );
            return EXIT_CODES::ERR_ARGS;
        }
    }

    wxString format = FROM_UTF8( m_argParser.get<std::string>( ARG_FORMAT ).c_str() );
    if( format == "excellon" )
    {
        drillJob->m_format = JOB_EXPORT_PCB_DRILL::DRILL_FORMAT::EXCELLON;
    }
    else if( format == "gerber" )
    {
        drillJob->m_format = JOB_EXPORT_PCB_DRILL::DRILL_FORMAT::GERBER;
    }
    else
    {
        wxFprintf( stderr, _( "Invalid drill format\n" ) );
        return EXIT_CODES::ERR_ARGS;
    }

    wxString units = FROM_UTF8( m_argParser.get<std::string>( ARG_EXCELLON_UNITS ).c_str() );

    if( units == wxS( "mm" ) )
    {
        drillJob->m_drillUnits = JOB_EXPORT_PCB_DRILL::DRILL_UNITS::MILLIMETERS;
    }
    else if( units == wxS( "in" ) )
    {
        drillJob->m_drillUnits = JOB_EXPORT_PCB_DRILL::DRILL_UNITS::INCHES;
    }
    else
    {
        wxFprintf( stderr, _( "Invalid units specified\n" ) );
        return EXIT_CODES::ERR_ARGS;
    }

    wxString zeroFormat = FROM_UTF8( m_argParser.get<std::string>( ARG_EXCELLON_ZEROS_FORMAT ).c_str() );

    if( zeroFormat == wxS( "decimal" ) )
    {
        drillJob->m_zeroFormat = JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT::DECIMAL;
    }
    else if( zeroFormat == wxS( "suppressleading" ) )
    {
        drillJob->m_zeroFormat = JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT::SUPPRESS_LEADING;
    }
    else if( zeroFormat == wxS( "suppresstrailing" ) )
    {
        drillJob->m_zeroFormat = JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT::SUPPRESS_TRAILING;
    }
    else if( zeroFormat == wxS( "keep" ) )
    {
        drillJob->m_zeroFormat = JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT::KEEP_ZEROS;
    }
    else
    {
        wxFprintf( stderr, _( "Invalid zeros format specified\n" ) );
        return EXIT_CODES::ERR_ARGS;
    }

    wxString mapFormat = FROM_UTF8( m_argParser.get<std::string>( ARG_MAP_FORMAT ).c_str() );

    if( mapFormat == wxS( "pdf" ) )
    {
        drillJob->m_mapFormat = JOB_EXPORT_PCB_DRILL::MAP_FORMAT::PDF;
    }
    else if( mapFormat == wxS( "ps" ) )
    {
        drillJob->m_mapFormat = JOB_EXPORT_PCB_DRILL::MAP_FORMAT::POSTSCRIPT;
    }
    else if( mapFormat == wxS( "gerberx2" ) )
    {
        drillJob->m_mapFormat = JOB_EXPORT_PCB_DRILL::MAP_FORMAT::GERBER_X2;
    }
    else if( mapFormat == wxS( "dxf" ) )
    {
        drillJob->m_mapFormat = JOB_EXPORT_PCB_DRILL::MAP_FORMAT::DXF;
    }
    else if( mapFormat == wxS( "svg" ) )
    {
        drillJob->m_mapFormat = JOB_EXPORT_PCB_DRILL::MAP_FORMAT::SVG;
    }
    else
    {
        wxFprintf( stderr, _( "Invalid map format specified\n" ) );
        return EXIT_CODES::ERR_ARGS;
    }

    wxString origin = FROM_UTF8( m_argParser.get<std::string>( ARG_DRILL_ORIGIN ).c_str() );

    if( origin == wxS( "absolute" ) )
    {
        drillJob->m_drillOrigin = JOB_EXPORT_PCB_DRILL::DRILL_ORIGIN::ABSOLUTE;
    }
    else if( origin == wxS( "plot" ) )
    {
        drillJob->m_drillOrigin = JOB_EXPORT_PCB_DRILL::DRILL_ORIGIN::PLOT;
    }
    else
    {
        wxFprintf( stderr, _( "Invalid origin mode specified\n" ) );
        return EXIT_CODES::ERR_ARGS;
    }

    drillJob->m_excellonMirrorY = m_argParser.get<bool>( ARG_EXCELLON_MIRRORY );
    drillJob->m_excellonMinimalHeader = m_argParser.get<bool>( ARG_EXCELLON_MINIMALHEAD );
    drillJob->m_excellonCombinePTHNPTH = !m_argParser.get<bool>( ARG_EXCELLON_SEPARATE_TH );
    drillJob->m_generateMap = m_argParser.get<bool>( ARG_GENERATE_MAP );

    if( drillJob->m_gerberPrecision != 5 && drillJob->m_gerberPrecision != 6 )
    {
        wxFprintf( stderr, _( "Gerber coordinate precision should be either 5 or 6\n" ) );
        return EXIT_CODES::ERR_ARGS;
    }

    int exitCode = aKiway.ProcessJob( KIWAY::FACE_PCB, drillJob.get() );

    return exitCode;
}
