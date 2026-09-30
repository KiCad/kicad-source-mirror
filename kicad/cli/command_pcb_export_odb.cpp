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

#include "command_pcb_export_odb.h"
#include <cli/exit_codes.h>
#include "jobs/job_export_pcb_odb.h"
#include <kiface_base.h>
#include <string_utils.h>
#include <wx/crt.h>

#include <macros.h>
#include <locale_io.h>

#define ARG_COMPRESS "--compression"

CLI::PCB_EXPORT_ODB_COMMAND::PCB_EXPORT_ODB_COMMAND() :
        PCB_EXPORT_BASE_COMMAND( "odb" )
{
    addDrawingSheetArg();
    addDefineArg();
    addFabExportArgs();

    m_argParser.add_description( std::string( "Export the PCB in ODB++ format" ) );

    m_argParser.add_argument( ARG_COMPRESS )
            .default_value( std::string( "zip" ) )
            .help( std::string( "Compression mode" ) )
            .choices( "none", "zip", "tgz" );

    addVariantsArg();
}


int CLI::PCB_EXPORT_ODB_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_EXPORT_PCB_ODB> job( new JOB_EXPORT_PCB_ODB() );

    applyFabExportArgs( *job );
    job->SetConfiguredOutputPath( m_argOutput );
    job->SetVarOverrides( m_argDefineVars );

    if( !wxFile::Exists( job->m_filename ) )
    {
        wxFprintf( stderr, _( "Board file does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

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
