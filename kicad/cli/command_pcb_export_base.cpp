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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "command_pcb_export_base.h"
#include <bitset>
#include <jobs/job_export_pcb_fab.h>
#include <string_utils.h>

#include <magic_enum.hpp>


CLI::PCB_EXPORT_BASE_COMMAND::PCB_EXPORT_BASE_COMMAND( const std::string& aName, IO_TYPE aInputType,
                                                       IO_TYPE aOutputType ) :
        COMMAND( aName )
{
    addCommonArgs( true, true, aInputType, aOutputType );
}


void CLI::PCB_EXPORT_BASE_COMMAND::addLayerArg()
{
    m_argParser.add_argument( "-l", ARG_LAYERS )
            .default_value( std::string() )
            .help( UTF8STDSTR( _( "Comma separated list of untranslated layer names to include "
                                  "such as F.Cu,B.Cu" ) ) )
            .metavar( "LAYER_LIST" );
}


void CLI::PCB_EXPORT_BASE_COMMAND::addCommonLayersArg()
{
    m_argParser.add_argument( "--cl", ARG_COMMON_LAYERS )
            .default_value( std::string() )
            .help( UTF8STDSTR( _( "Layers to include on each plot, comma separated list of "
                                  "untranslated layer names to include such as F.Cu,B.Cu" ) ) )
            .metavar( "COMMON_LAYER_LIST" );
}


void CLI::PCB_EXPORT_BASE_COMMAND::addFabExportArgs( const JOB_EXPORT_PCB_FAB& aJob,
                                                    const std::string&        aDataSetAlias )
{
    m_argParser.add_argument( ARG_BOM_COL_MFG_PN )
            .default_value( std::string() )
            .help( std::string( "Name of the part field to use for the Bill of Material "
                                "Manufacturer Part Number Column" ) )
            .metavar( "FIELD_NAME" );

    m_argParser.add_argument( ARG_PRECISION )
            .help( std::string( "Precision" ) )
            .scan<'i', int>()
            .default_value( 6 )
            .metavar( "PRECISION" );

    m_argParser.add_argument( ARG_FAB_UNITS )
            .default_value( std::string( "mm" ) )
            .help( std::string( "Units" ) )
            .choices( "mm", "in" );

    m_argParser.add_argument( ARG_CHECK_ZONES ).help( UTF8STDSTR( _( ARG_CHECK_ZONES_DESC ) ) ).flag();

    std::string dataSetHelp = "Data set to export:";

    for( JOB_EXPORT_PCB_FAB::DATA_SET dataSet : magic_enum::enum_values<JOB_EXPORT_PCB_FAB::DATA_SET>() )
    {
        if( aJob.SupportsDataSet( dataSet ) )
            dataSetHelp += " " + JOB_EXPORT_PCB_FAB::DataSetToken( dataSet );
    }

    argparse::Argument& dataSetArg = aDataSetAlias.empty() ? m_argParser.add_argument( ARG_DATA_SET )
                                                           : m_argParser.add_argument( ARG_DATA_SET, aDataSetAlias );

    dataSetArg.default_value( std::string( "userdef" ) ).help( dataSetHelp ).metavar( "DATA_SET" );

    m_argParser.add_argument( ARG_SECTIONS )
            .default_value( std::string() )
            .help( std::string( "Override the optional sections of the chosen data set, as "
                                "IPC-2581 section key letters" ) )
            .metavar( "SECTION_KEY" );

    m_argParser.add_argument( ARG_NET_NAMES )
            .default_value( std::string( "include" ) )
            .help( std::string( "Export net names as authored or as anonymous names that preserve "
                                "connectivity" ) )
            .choices( "include", "anonymize" );
}


void CLI::PCB_EXPORT_BASE_COMMAND::applyFabExportArgs( JOB_EXPORT_PCB_FAB& aJob )
{
    aJob.m_filename = m_argInput;
    aJob.m_drawingSheet = m_argDrawingSheet;
    aJob.m_variantNames = m_argVariantNames;
    aJob.m_precision = m_argParser.get<int>( ARG_PRECISION );
    aJob.m_checkZonesBeforeExport = m_argParser.get<bool>( ARG_CHECK_ZONES );
    aJob.m_colMfgPn = From_UTF8( m_argParser.get<std::string>( ARG_BOM_COL_MFG_PN ).c_str() );
    std::string units = m_argParser.get<std::string>( ARG_FAB_UNITS );
    aJob.m_units = units == "in" ? JOB_EXPORT_PCB_FAB::UNITS::INCH : JOB_EXPORT_PCB_FAB::UNITS::MM;

    aJob.m_dataSet = JOB_EXPORT_PCB_FAB::DataSetFromToken(
            From_UTF8( m_argParser.get<std::string>( ARG_DATA_SET ).c_str() ) );
    aJob.m_sections = From_UTF8( m_argParser.get<std::string>( ARG_SECTIONS ).c_str() );
    aJob.m_netNames = m_argParser.get<std::string>( ARG_NET_NAMES ) == "anonymize"
                              ? JOB_EXPORT_PCB_FAB::NET_NAMES::ANONYMIZE
                              : JOB_EXPORT_PCB_FAB::NET_NAMES::INCLUDE;
}
