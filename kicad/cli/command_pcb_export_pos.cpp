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

#include "command_pcb_export_pos.h"
#include <cli/exit_codes.h>
#include "jobs/job_export_pcb_pos.h"
#include <kiface_base.h>
#include <string_utils.h>
#include <wx/crt.h>

#include <macros.h>

#include <locale_io.h>

#define ARG_SIDE "--side"
#define ARG_FORMAT "--format"
#define ARG_UNITS "--units"
#define ARG_NEGATE_BOTTOM_X "--bottom-negate-x"
#define ARG_USE_DRILL_FILE_ORIGIN "--use-drill-file-origin"
#define ARG_SMD_ONLY "--smd-only"
#define ARG_EXCLUDE_FOOTPRINTS_TH "--exclude-fp-th"
#define ARG_EXCLUDE_DNP "--exclude-dnp"
#define ARG_GERBER_BOARD_EDGE "--gerber-board-edge"
#define ARG_VARIANT "--variant"


CLI::PCB_EXPORT_POS_COMMAND::PCB_EXPORT_POS_COMMAND() :
        PCB_EXPORT_BASE_COMMAND( "pos" )
{
    m_argParser.add_description( UTF8STDSTR( _( "Generate Position File" ) ) );

    m_argParser.add_argument( ARG_SIDE )
            .default_value( std::string( "both" ) )
            .help( UTF8STDSTR( _( "Valid options: front,back,both. Gerber format only supports "
                                  "\"front\" or \"back\"." ) ) );

    m_argParser.add_argument( ARG_FORMAT )
            .default_value( std::string( "ascii" ) )
            .help( UTF8STDSTR( _( "Valid options: ascii,csv,gerber" ) ) )
            .metavar( "FORMAT" );

    m_argParser.add_argument( ARG_UNITS )
            .default_value( std::string( "in" ) )
            .help( UTF8STDSTR( _( "Output units; ascii or csv format only; valid options: "
                                  "in,mm" ) ) )
            .metavar( "UNITS" );

    m_argParser.add_argument( ARG_NEGATE_BOTTOM_X )
            .help( UTF8STDSTR( _( "Use negative X coordinates for footprints on bottom layer "
                                "(ascii or csv formats only)" ) ) )
            .flag();

    m_argParser.add_argument( ARG_USE_DRILL_FILE_ORIGIN )
            .help( UTF8STDSTR( _( "Use drill/place file origin (ascii or csv only)" ) ) )
            .flag();

    m_argParser.add_argument( ARG_SMD_ONLY )
            .help( UTF8STDSTR( _( "Include only SMD footprints (ascii or csv only)" ) ) )
            .flag();

    m_argParser.add_argument( ARG_EXCLUDE_FOOTPRINTS_TH )
            .help( UTF8STDSTR(
                    _( "Exclude all footprints with through-hole pads (ascii or csv only)" ) ) )
            .flag();

    m_argParser.add_argument( ARG_EXCLUDE_DNP )
            .help( UTF8STDSTR( _( "Exclude all footprints with the Do Not Populate flag set" ) ) )
            .flag();

    m_argParser.add_argument( ARG_GERBER_BOARD_EDGE )
            .help( UTF8STDSTR( _( "Include board edge layer (Gerber only)" ) ) )
            .flag();

    m_argParser.add_argument( ARG_VARIANT )
            .default_value( std::string( "" ) )
            .help( UTF8STDSTR( _( "Board variant for variant-aware filtering (DNP, BOM, position "
                                  "file exclusions)" ) ) )
            .metavar( "VARIANT" );
}


int CLI::PCB_EXPORT_POS_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_EXPORT_PCB_POS> aPosJob( new JOB_EXPORT_PCB_POS() );

    aPosJob->m_filename = m_argInput;
    aPosJob->SetConfiguredOutputPath( m_argOutput );

    if( !wxFile::Exists( aPosJob->m_filename ) )
    {
        wxFprintf( stderr, _( "Board file does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    aPosJob->m_negateBottomX = m_argParser.get<bool>( ARG_NEGATE_BOTTOM_X );
    aPosJob->m_singleFile = true;
    aPosJob->m_nakedFilename = true;
    aPosJob->m_smdOnly = m_argParser.get<bool>( ARG_SMD_ONLY );
    aPosJob->m_excludeFootprintsWithTh = m_argParser.get<bool>( ARG_EXCLUDE_FOOTPRINTS_TH );
    aPosJob->m_useDrillPlaceFileOrigin = m_argParser.get<bool>( ARG_USE_DRILL_FILE_ORIGIN );
    aPosJob->m_excludeDNP = m_argParser.get<bool>( ARG_EXCLUDE_DNP );
    aPosJob->m_gerberBoardEdge = m_argParser.get<bool>( ARG_GERBER_BOARD_EDGE );
    aPosJob->m_variant = From_UTF8( m_argParser.get<std::string>( ARG_VARIANT ).c_str() );

    wxString format = From_UTF8( m_argParser.get<std::string>( ARG_FORMAT ).c_str() );

    if( format == wxS( "ascii" ) )
    {
        aPosJob->m_format = JOB_EXPORT_PCB_POS::FORMAT::ASCII;
    }
    else if( format == wxS( "csv" ) )
    {
        aPosJob->m_format = JOB_EXPORT_PCB_POS::FORMAT::CSV;
    }
    else if( format == wxS( "gerber" ) )
    {
        aPosJob->m_format = JOB_EXPORT_PCB_POS::FORMAT::GERBER;
    }
    else
    {
        wxFprintf( stderr, _( "Invalid format\n" ) );
        return EXIT_CODES::ERR_ARGS;
    }

    wxString units = From_UTF8( m_argParser.get<std::string>( ARG_UNITS ).c_str() );

    if( units == wxS( "mm" ) )
    {
        aPosJob->m_units = JOB_EXPORT_PCB_POS::UNITS::MM;
    }
    else if( units == wxS( "in" ) )
    {
        aPosJob->m_units = JOB_EXPORT_PCB_POS::UNITS::INCH;
    }
    else
    {
        wxFprintf( stderr, _( "Invalid units specified\n" ) );
        return EXIT_CODES::ERR_ARGS;
    }

    wxString side = From_UTF8( m_argParser.get<std::string>( ARG_SIDE ).c_str() );

    if( side == wxS( "both" ) )
    {
        if( aPosJob->m_format == JOB_EXPORT_PCB_POS::FORMAT::GERBER )
        {
            wxFprintf( stderr, _( "\"both\" not supported for Gerber format\n" ) );
            return EXIT_CODES::ERR_ARGS;
        }

        aPosJob->m_side = JOB_EXPORT_PCB_POS::SIDE::BOTH;
    }
    else if( side == wxS( "front" ) )
    {
        aPosJob->m_side = JOB_EXPORT_PCB_POS::SIDE::FRONT;
    }
    else if( side == wxS( "back" ) )
    {
        aPosJob->m_side = JOB_EXPORT_PCB_POS::SIDE::BACK;
    }
    else
    {
        wxFprintf( stderr, _( "Invalid side specified\n" ) );
        return EXIT_CODES::ERR_ARGS;
    }

    LOCALE_IO dummy;
    return aKiway.ProcessJob( KIWAY::FACE_PCB, aPosJob.get() );
}
