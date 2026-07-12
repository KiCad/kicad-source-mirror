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

#include "command_sch_import.h"
#include <cli/exit_codes.h>
#include <jobs/job_sch_import.h>
#include <jobs/job_import_utils.h>
#include <kiface_base.h>
#include <string_utils.h>
#include <macros.h>
#include <wx/crt.h>


#define ARG_FORMAT "--format"
#define ARG_REPORT_FORMAT "--report-format"
#define ARG_REPORT_FILE "--report-file"


CLI::SCH_IMPORT_COMMAND::SCH_IMPORT_COMMAND() : COMMAND( "import" )
{
    addCommonArgs( true, true, IO_TYPE::FILE, IO_TYPE::FILE );

    m_argParser.add_description(
            UTF8STDSTR( _( "Import a non-KiCad schematic file to KiCad format" ) ) );

    m_argParser.add_argument( ARG_FORMAT )
            .default_value( std::string( "auto" ) )
            .help( UTF8STDSTR( _( "Input format hint: auto, altium, eagle, cadstar, easyeda, "
                                  "easyedapro, ltspice, pads, diptrace, pcad, orcad "
                                  "(default: auto)" ) ) )
            .metavar( "FORMAT" );

    m_argParser.add_argument( ARG_REPORT_FORMAT )
            .default_value( std::string( "none" ) )
            .help( UTF8STDSTR( _( "Import report format: none, json, text (default: none)" ) ) )
            .metavar( "FORMAT" );

    m_argParser.add_argument( ARG_REPORT_FILE )
            .default_value( std::string( "" ) )
            .help( UTF8STDSTR( _( "File path for import report (default: stdout)" ) ) )
            .metavar( "FILE" );
}


int CLI::SCH_IMPORT_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_SCH_IMPORT> importJob = std::make_unique<JOB_SCH_IMPORT>();

    importJob->m_inputFile = m_argInput;
    importJob->SetConfiguredOutputPath( m_argOutput );

    wxString format = From_UTF8( m_argParser.get<std::string>( ARG_FORMAT ).c_str() );

    if( format == wxS( "auto" ) )
        importJob->m_format = JOB_SCH_IMPORT::FORMAT::AUTO;
    else if( format == wxS( "altium" ) )
        importJob->m_format = JOB_SCH_IMPORT::FORMAT::ALTIUM;
    else if( format == wxS( "eagle" ) )
        importJob->m_format = JOB_SCH_IMPORT::FORMAT::EAGLE;
    else if( format == wxS( "cadstar" ) )
        importJob->m_format = JOB_SCH_IMPORT::FORMAT::CADSTAR;
    else if( format == wxS( "easyeda" ) )
        importJob->m_format = JOB_SCH_IMPORT::FORMAT::EASYEDA;
    else if( format == wxS( "easyedapro" ) )
        importJob->m_format = JOB_SCH_IMPORT::FORMAT::EASYEDAPRO;
    else if( format == wxS( "ltspice" ) )
        importJob->m_format = JOB_SCH_IMPORT::FORMAT::LTSPICE;
    else if( format == wxS( "pads" ) )
        importJob->m_format = JOB_SCH_IMPORT::FORMAT::PADS;
    else if( format == wxS( "diptrace" ) )
        importJob->m_format = JOB_SCH_IMPORT::FORMAT::DIPTRACE;
    else if( format == wxS( "pcad" ) )
        importJob->m_format = JOB_SCH_IMPORT::FORMAT::PCAD;
    else if( format == wxS( "orcad" ) )
        importJob->m_format = JOB_SCH_IMPORT::FORMAT::ORCAD;
    else
    {
        wxFprintf( stderr, _( "Invalid format: %s\n" ), format );
        return EXIT_CODES::ERR_ARGS;
    }

    wxString reportFormat = From_UTF8( m_argParser.get<std::string>( ARG_REPORT_FORMAT ).c_str() );

    if( !ParseImportReportFormat( reportFormat, importJob->m_reportFormat ) )
    {
        wxFprintf( stderr, _( "Invalid report format: %s\n" ), reportFormat );
        return EXIT_CODES::ERR_ARGS;
    }

    importJob->m_reportFile = From_UTF8( m_argParser.get<std::string>( ARG_REPORT_FILE ).c_str() );

    int exitCode = aKiway.ProcessJob( KIWAY::FACE_SCH, importJob.get() );

    // The sentinel is internal to the top-level `import` classifier; standalone, it is invalid input.
    if( exitCode == EXIT_CODES::ERR_UNKNOWN_FILE_FORMAT )
        exitCode = EXIT_CODES::ERR_INVALID_INPUT_FILE;

    return exitCode;
}
