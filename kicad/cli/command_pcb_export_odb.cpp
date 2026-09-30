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
#define ARG_VARIANT_PACKAGING "--variant-packaging"
#define ARG_ORIGIN "--origin"
#define ARG_PRODUCT_NAME "--product-name"
#define ARG_DATA_SET "--data-set"
#define ARG_NET_NAMES "--net-names"

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

    m_argParser.add_argument( ARG_VARIANT_PACKAGING )
            .default_value( std::string( "separate" ) )
            .help( std::string( "Package variants separately or together" ) )
            .choices( "separate", "combined" );

    m_argParser.add_argument( ARG_ORIGIN )
            .default_value( std::string( "absolute" ) )
            .help( std::string( "Coordinate origin" ) )
            .choices( "absolute", "aux", "grid" );

    m_argParser.add_argument( ARG_PRODUCT_NAME )
            .default_value( std::string() )
            .help( std::string( "ODB++ product model name" ) )
            .metavar( "NAME" );

    m_argParser.add_argument( ARG_DATA_SET )
            .default_value( std::string( "all" ) )
            .help( std::string( "ODB++ data set" ) )
            .choices( "all", "fabrication", "assembly", "test", "stackup" );

    m_argParser.add_argument( ARG_NET_NAMES )
            .default_value( std::string( "include" ) )
            .help( std::string( "Export net names or anonymous names preserving connectivity" ) )
            .choices( "include", "anonymize" );

    addVariantsArg( false );
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

    if( m_argParser.get<std::string>( ARG_VARIANT_PACKAGING ) == "combined" )
        job->m_variantPackaging = JOB_EXPORT_PCB_ODB::VARIANT_PACKAGING::COMBINED;

    std::string origin = m_argParser.get<std::string>( ARG_ORIGIN );

    if( origin == "aux" )
        job->m_origin = JOB_EXPORT_PCB_ODB::ORIGIN::AUX;
    else if( origin == "grid" )
        job->m_origin = JOB_EXPORT_PCB_ODB::ORIGIN::GRID;

    job->m_productName = From_UTF8( m_argParser.get<std::string>( ARG_PRODUCT_NAME ).c_str() );

    std::string dataSet = m_argParser.get<std::string>( ARG_DATA_SET );

    if( dataSet == "fabrication" )
        job->m_dataSet = JOB_EXPORT_PCB_ODB::DATA_SET::FABRICATION;
    else if( dataSet == "assembly" )
        job->m_dataSet = JOB_EXPORT_PCB_ODB::DATA_SET::ASSEMBLY;
    else if( dataSet == "test" )
        job->m_dataSet = JOB_EXPORT_PCB_ODB::DATA_SET::TEST;
    else if( dataSet == "stackup" )
        job->m_dataSet = JOB_EXPORT_PCB_ODB::DATA_SET::STACKUP;

    job->m_netNamePolicy = From_UTF8( m_argParser.get<std::string>( ARG_NET_NAMES ).c_str() );

    LOCALE_IO dummy;
    return aKiway.ProcessJob( KIWAY::FACE_PCB, job.get() );
}
