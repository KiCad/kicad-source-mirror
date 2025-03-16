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

#include "command_pcb_export_ipc2581.h"
#include <cli/exit_codes.h>
#include "jobs/job_export_pcb_ipc2581.h"
#include <kiface_base.h>
#include <string_utils.h>
#include <wx/crt.h>

#include <macros.h>
#include <wx/tokenzr.h>

#include <locale_io.h>

#define ARG_COMPRESS "--compress"

#define ARG_BOM_COL_INT_ID "--bom-col-int-id"
#define ARG_BOM_COL_MFG_PN "--bom-col-mfg-pn"
#define ARG_BOM_COL_MFG "--bom-col-mfg"
#define ARG_BOM_COL_DIST_PN "--bom-col-dist-pn"
#define ARG_BOM_COL_DIST "--bom-col-dist"
#define ARG_UNITS "--units"

CLI::PCB_EXPORT_IPC2581_COMMAND::PCB_EXPORT_IPC2581_COMMAND() :
        PCB_EXPORT_BASE_COMMAND( "ipc2581" )
{
    addDrawingSheetArg();
    addDefineArg();

    m_argParser.add_description( std::string( "Export the PCB in IPC-2581 format" ) );

    m_argParser.add_argument( ARG_PRECISION )
            .help( std::string( "Precision" ) )
            .scan<'i', int>()
            .default_value( 6 )
            .metavar( "PRECISION" );

    m_argParser.add_argument( ARG_COMPRESS )
            .help( std::string( "Compress the output" ) )
            .flag();

    m_argParser.add_argument( ARG_VERSION )
            .default_value( std::string( "C" ) )
            .help( std::string( "IPC-2581 standard version" ) )
            .choices( "B", "C" );

    m_argParser.add_argument( ARG_UNITS )
            .default_value( std::string( "mm" ) )
            .help( std::string( "Units" ) )
            .choices( "mm", "in" );

    m_argParser.add_argument( ARG_BOM_COL_INT_ID )
            .default_value( std::string() )
            .help( std::string(
                    "Name of the part field to use for the Bill of Material Internal Id Column" ) )
            .metavar( "FIELD_NAME" );

    m_argParser.add_argument( ARG_BOM_COL_MFG_PN )
            .default_value( std::string() )
            .help( std::string( "Name of the part field to use for the Bill of "
                                "Material Manufacturer Part Number Column" ) )
            .metavar( "FIELD_NAME" );

    m_argParser.add_argument( ARG_BOM_COL_MFG )
            .default_value( std::string() )
            .help( std::string( "Name of the part field to use for the Bill of "
                                "Material Manufacturer Column" ) )
            .metavar( "FIELD_NAME" );

    m_argParser.add_argument( ARG_BOM_COL_DIST_PN )
            .default_value( std::string() )
            .help( std::string( "Name of the part field to use for the Bill of "
                                "Material Distributor Part Number Column" ) )
            .metavar( "FIELD_NAME" );

    m_argParser.add_argument( ARG_BOM_COL_DIST )
            .default_value( std::string() )
            .help( std::string( "Name to insert into Bill of "
                                "Material Distributor Column" ) )
            .metavar( "FIELD_NAME" );

}


int CLI::PCB_EXPORT_IPC2581_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_EXPORT_PCB_IPC2581> ipc2581Job( new JOB_EXPORT_PCB_IPC2581() );

    ipc2581Job->m_filename = m_argInput;
    ipc2581Job->SetConfiguredOutputPath( m_argOutput );
    ipc2581Job->m_drawingSheet = m_argDrawingSheet;
    ipc2581Job->SetVarOverrides( m_argDefineVars );

    if( !wxFile::Exists( ipc2581Job->m_filename ) )
    {
        wxFprintf( stderr, _( "Board file does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    ipc2581Job->m_compress = m_argParser.get<bool>( ARG_COMPRESS );
    ipc2581Job->m_precision = m_argParser.get<int>( ARG_PRECISION );

    wxString version = From_UTF8( m_argParser.get<std::string>( ARG_VERSION ).c_str() );
    if( version == 'B' )
        ipc2581Job->m_version = JOB_EXPORT_PCB_IPC2581::IPC2581_VERSION::B;
    else if( version == 'C' )
        ipc2581Job->m_version = JOB_EXPORT_PCB_IPC2581::IPC2581_VERSION::C;

    wxString units = From_UTF8( m_argParser.get<std::string>( ARG_UNITS ).c_str() );
    if( units == "mm" )
        ipc2581Job->m_units = JOB_EXPORT_PCB_IPC2581::IPC2581_UNITS::MM;
    else if( units == "in" )
        ipc2581Job->m_units = JOB_EXPORT_PCB_IPC2581::IPC2581_UNITS::INCH;

    ipc2581Job->m_colInternalId =
            From_UTF8( m_argParser.get<std::string>( ARG_BOM_COL_INT_ID ).c_str() );
    ipc2581Job->m_colMfgPn =
            From_UTF8( m_argParser.get<std::string>( ARG_BOM_COL_MFG_PN ).c_str() );
    ipc2581Job->m_colMfg = From_UTF8( m_argParser.get<std::string>( ARG_BOM_COL_MFG ).c_str() );
    ipc2581Job->m_colDistPn =
            From_UTF8( m_argParser.get<std::string>( ARG_BOM_COL_DIST_PN ).c_str() );
    ipc2581Job->m_colDist =
            From_UTF8( m_argParser.get<std::string>( ARG_BOM_COL_DIST ).c_str() );

    LOCALE_IO dummy;
    return aKiway.ProcessJob( KIWAY::FACE_PCB, ipc2581Job.get() );
}
