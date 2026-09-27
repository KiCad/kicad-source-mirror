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
#include <regex>
#include <magic_enum.hpp>

#include <cli/exit_codes.h>
#include <kiface_base.h>
#include <string_utils.h>
#include <wx/crt.h>

#include "command_pcb_export_idf.h"
#include "jobs/job_export_pcb_idf.h"

#define ARG_UNITS "--units"
#define ARG_ORIGIN "--origin"
#define ARG_USER_ORIGIN_X "--user-origin-x"
#define ARG_USER_ORIGIN_Y "--user-origin-y"
#define ARG_NO_UNSPECIFIED "--no-unspecified"
#define ARG_NO_DNP "--no-dnp"
#define ARG_EXPLICIT_HEIGHT "--explicit-height"
#define ARG_PART_NUMBER_FIELD "--part-number-field"


CLI::PCB_EXPORT_IDF_COMMAND::PCB_EXPORT_IDF_COMMAND() :
        PCB_EXPORT_BASE_COMMAND( "idf" )
{
    addDefineArg();

    m_argParser.add_description( UTF8STDSTR( _( "Export the PCB in IDFv3 format" ) ) );

    m_argParser.add_argument( ARG_UNITS )
            .default_value( std::string( "mm" ) )
            .help( UTF8STDSTR( _( "Output units" ) ) )
            .choices( "mm", "mils" );

    m_argParser.add_argument( ARG_ORIGIN )
            .default_value( std::string( "center" ) )
            .help( UTF8STDSTR( _( "Coordinate system origin for the board" ) ) )
            .choices( "drill", "grid", "user", "center" );

    m_argParser.add_argument( ARG_USER_ORIGIN_X )
            .default_value( 0.0 )
            .scan<'g', double>()
            .help( UTF8STDSTR( _( "User-specified board origin X" ) ) )
            .metavar( "VALUE" );

    m_argParser.add_argument( ARG_USER_ORIGIN_Y )
            .default_value( 0.0 )
            .scan<'g', double>()
            .help( UTF8STDSTR( _( "User-specified board origin Y" ) ) )
            .metavar( "VALUE" );

    m_argParser.add_argument( ARG_NO_UNSPECIFIED )
            .help( UTF8STDSTR( _( "Exclude components with 'Unspecified' footprint type" ) ) )
            .flag();

    m_argParser.add_argument( ARG_NO_DNP )
            .help( UTF8STDSTR( _( "Exclude components with 'Do not populate' attribute" ) ) )
            .flag();

    m_argParser.add_argument( ARG_EXPLICIT_HEIGHT )
            .help( UTF8STDSTR( _( "Only use explicitly-set footprint heights to create IDFv3 shapes" ) ) )
            .flag();

    m_argParser.add_argument( ARG_PART_NUMBER_FIELD )
            .default_value( std::string( "Value" ) )
            .help( UTF8STDSTR( _( "Footprint field to export as the part number to the IDF library" ) ) );
}


int CLI::PCB_EXPORT_IDF_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_EXPORT_PCB_IDF> idfJob( new JOB_EXPORT_PCB_IDF() );

    idfJob->m_filename = m_argInput;
    idfJob->SetConfiguredOutputPath( m_argOutput );
    idfJob->SetVarOverrides( m_argDefineVars );

    if( !wxFile::Exists( idfJob->m_filename ) )
    {
        wxFprintf( stderr, _( "Board file does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    idfJob->m_units = magic_enum::enum_cast<IDF_SETTINGS::UNITS>( m_argParser.get<std::string>( ARG_UNITS ),
                                                                  magic_enum::case_insensitive )
                              .value_or( IDF_SETTINGS::UNITS::MM );

    idfJob->m_originMode = magic_enum::enum_cast<IDF_SETTINGS::COORD_ORIGIN>(
                                   m_argParser.get<std::string>( ARG_ORIGIN ), magic_enum::case_insensitive )
                                   .value_or( IDF_SETTINGS::COORD_ORIGIN::CENTER );

    idfJob->m_userOrigin.x = m_argParser.get<double>( ARG_USER_ORIGIN_X );
    idfJob->m_userOrigin.y = m_argParser.get<double>( ARG_USER_ORIGIN_Y );

    idfJob->m_includeUnspecified = !m_argParser.get<bool>( ARG_NO_UNSPECIFIED );
    idfJob->m_includeDNP = !m_argParser.get<bool>( ARG_NO_DNP );
    idfJob->m_calculateHeightFromModels = !m_argParser.get<bool>( ARG_EXPLICIT_HEIGHT );
    idfJob->m_partNumberField = wxString::FromUTF8( m_argParser.get<std::string>( ARG_PART_NUMBER_FIELD ) );

    return aKiway.ProcessJob( KIWAY::FACE_PCB, idfJob.get() );
}
