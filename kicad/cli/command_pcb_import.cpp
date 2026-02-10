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

#include "command_pcb_import.h"
#include <cli/exit_codes.h>
#include <jobs/job_pcb_import.h>
#include <kiface_base.h>
#include <string_utils.h>
#include <macros.h>
#include <wx/crt.h>
#include <wx/filename.h>


#define ARG_FORMAT "--format"
#define ARG_LAYER_MAP "--layer-map"
#define ARG_AUTO_MAP "--auto-map"
#define ARG_REPORT_FORMAT "--report-format"
#define ARG_REPORT_FILE "--report-file"


CLI::PCB_IMPORT_COMMAND::PCB_IMPORT_COMMAND() : COMMAND( "import" )
{
    addCommonArgs( true, true, IO_TYPE::FILE, IO_TYPE::FILE );

    m_argParser.add_description(
            UTF8STDSTR( _( "Import a non-KiCad PCB file to KiCad format" ) ) );

    m_argParser.add_argument( ARG_FORMAT )
            .default_value( std::string( "auto" ) )
            .help( UTF8STDSTR( _( "Input format hint: auto, pads, altium, eagle, cadstar, "
                                  "fabmaster, pcad, solidworks (default: auto)" ) ) )
            .metavar( "FORMAT" );

    m_argParser.add_argument( ARG_LAYER_MAP )
            .default_value( std::string( "" ) )
            .help( UTF8STDSTR( _( "JSON file with layer name mappings" ) ) )
            .metavar( "FILE" );

    m_argParser.add_argument( ARG_AUTO_MAP )
            .help( UTF8STDSTR( _( "Use automatic layer mapping (default behavior)" ) ) )
            .flag();

    m_argParser.add_argument( ARG_REPORT_FORMAT )
            .default_value( std::string( "none" ) )
            .help( UTF8STDSTR( _( "Import report format: none, json, text (default: none)" ) ) )
            .metavar( "FORMAT" );

    m_argParser.add_argument( ARG_REPORT_FILE )
            .default_value( std::string( "" ) )
            .help( UTF8STDSTR( _( "File path for import report (default: stdout)" ) ) )
            .metavar( "FILE" );
}


int CLI::PCB_IMPORT_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_PCB_IMPORT> importJob = std::make_unique<JOB_PCB_IMPORT>();

    importJob->m_inputFile = m_argInput;
    importJob->SetConfiguredOutputPath( m_argOutput );

    wxString format = From_UTF8( m_argParser.get<std::string>( ARG_FORMAT ).c_str() );

    if( format == wxS( "auto" ) )
    {
        importJob->m_format = JOB_PCB_IMPORT::FORMAT::AUTO;
    }
    else if( format == wxS( "pads" ) )
    {
        importJob->m_format = JOB_PCB_IMPORT::FORMAT::PADS;
    }
    else if( format == wxS( "altium" ) )
    {
        importJob->m_format = JOB_PCB_IMPORT::FORMAT::ALTIUM;
    }
    else if( format == wxS( "eagle" ) )
    {
        importJob->m_format = JOB_PCB_IMPORT::FORMAT::EAGLE;
    }
    else if( format == wxS( "cadstar" ) )
    {
        importJob->m_format = JOB_PCB_IMPORT::FORMAT::CADSTAR;
    }
    else if( format == wxS( "fabmaster" ) )
    {
        importJob->m_format = JOB_PCB_IMPORT::FORMAT::FABMASTER;
    }
    else if( format == wxS( "pcad" ) )
    {
        importJob->m_format = JOB_PCB_IMPORT::FORMAT::PCAD;
    }
    else if( format == wxS( "solidworks" ) )
    {
        importJob->m_format = JOB_PCB_IMPORT::FORMAT::SOLIDWORKS;
    }
    else
    {
        wxFprintf( stderr, _( "Invalid format: %s\n" ), format );
        return EXIT_CODES::ERR_ARGS;
    }

    wxString layerMapFile = From_UTF8( m_argParser.get<std::string>( ARG_LAYER_MAP ).c_str() );
    importJob->m_layerMapFile = layerMapFile;

    importJob->m_autoMap = m_argParser.get<bool>( ARG_AUTO_MAP ) || layerMapFile.IsEmpty();

    wxString reportFormat = From_UTF8( m_argParser.get<std::string>( ARG_REPORT_FORMAT ).c_str() );

    if( reportFormat == wxS( "none" ) )
    {
        importJob->m_reportFormat = JOB_PCB_IMPORT::REPORT_FORMAT::NONE;
    }
    else if( reportFormat == wxS( "json" ) )
    {
        importJob->m_reportFormat = JOB_PCB_IMPORT::REPORT_FORMAT::JSON;
    }
    else if( reportFormat == wxS( "text" ) )
    {
        importJob->m_reportFormat = JOB_PCB_IMPORT::REPORT_FORMAT::TEXT;
    }
    else
    {
        wxFprintf( stderr, _( "Invalid report format: %s\n" ), reportFormat );
        return EXIT_CODES::ERR_ARGS;
    }

    wxString reportFile = From_UTF8( m_argParser.get<std::string>( ARG_REPORT_FILE ).c_str() );
    importJob->m_reportFile = reportFile;

    int exitCode = aKiway.ProcessJob( KIWAY::FACE_PCB, importJob.get() );

    return exitCode;
}
