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

#include "command_pcb_export_odb.h"
#include <cli/exit_codes.h>
#include "jobs/job_export_pcb_odb.h"
#include <kiface_base.h>
#include <string_utils.h>
#include <wx/crt.h>

#include <macros.h>
#include <locale_io.h>

#define ARG_COMPRESS "--compression"
#define ARG_UNITS "--units"

CLI::PCB_EXPORT_ODB_COMMAND::PCB_EXPORT_ODB_COMMAND() :
        PCB_EXPORT_BASE_COMMAND( "odb" )
{
    addDrawingSheetArg();
    addDefineArg();

    m_argParser.add_description( std::string( "Export the PCB in ODB++ format" ) );

    m_argParser.add_argument( ARG_PRECISION )
            .help( std::string( "Precision" ) )
            .scan<'i', int>()
            .default_value( 2 )
            .metavar( "PRECISION" );

    m_argParser.add_argument( ARG_COMPRESS )
            .default_value( std::string( "zip" ) )
            .help( std::string( "Compression mode" ) )
            .choices( "none", "zip", "tgz" );

    m_argParser.add_argument( ARG_UNITS )
            .default_value( std::string( "mm" ) )
            .help( std::string( "Units" ) )
            .choices( "mm", "in" );
}


int CLI::PCB_EXPORT_ODB_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_EXPORT_PCB_ODB> job( new JOB_EXPORT_PCB_ODB() );

    job->m_filename = m_argInput;
    job->SetConfiguredOutputPath( m_argOutput );
    job->m_drawingSheet = m_argDrawingSheet;
    job->SetVarOverrides( m_argDefineVars );

    if( !wxFile::Exists( job->m_filename ) )
    {
        wxFprintf( stderr, _( "Board file does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    job->m_precision = m_argParser.get<int>( ARG_PRECISION );

    wxString units = From_UTF8( m_argParser.get<std::string>( ARG_UNITS ).c_str() );

    if( units == "mm" )
        job->m_units = JOB_EXPORT_PCB_ODB::ODB_UNITS::MM;
    else if( units == "in" )
        job->m_units = JOB_EXPORT_PCB_ODB::ODB_UNITS::INCH;

    wxString compression = From_UTF8( m_argParser.get<std::string>( ARG_COMPRESS ).c_str() ).Lower();

    if( compression == "zip" )
        job->m_compressionMode = JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::ZIP;
    else if( compression == "tgz" )
        job->m_compressionMode = JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::TGZ;
    else if( compression == "none" )
        job->m_compressionMode = JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::NONE;

    LOCALE_IO dummy;
    return aKiway.ProcessJob( KIWAY::FACE_PCB, job.get() );
}
