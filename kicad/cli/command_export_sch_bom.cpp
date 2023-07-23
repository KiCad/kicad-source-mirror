/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mike Williams <mike@mikebwilliams.com>
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

#include "command_export_sch_bom.h"
#include <cli/exit_codes.h>
#include "jobs/job_export_sch_bom.h"
#include <kiface_base.h>
#include <layer_ids.h>
#include <wx/crt.h>
#include <schematic_settings.h>

#include <macros.h>


CLI::EXPORT_SCH_BOM_COMMAND::EXPORT_SCH_BOM_COMMAND() : EXPORT_PCB_BASE_COMMAND( "bom" )
{
    // Field output options
    m_argParser.add_argument( ARG_FIELDS )
            .help( UTF8STDSTR( _( ARG_FIELDS_DESC ) ) )
            .default_value( std::string( "Reference,Value,Footprint,Quantity,${DNP}" ) );

    m_argParser.add_argument( ARG_LABELS )
            .help( UTF8STDSTR( _( ARG_LABELS_DESC ) ) )
            .default_value( std::string( "Refs,Value,Footprint,Qty,DNP" ) );

    m_argParser.add_argument( ARG_GROUP_BY )
            .help( UTF8STDSTR( _( ARG_GROUP_BY_DESC ) ) )
            .default_value( std::string( "" ) );

    m_argParser.add_argument( ARG_SORT_FIELD )
            .help( UTF8STDSTR( _( ARG_SORT_FIELD_DESC ) ) )
            .default_value( std::string( "Reference" ) );

    m_argParser.add_argument( ARG_SORT_ASC )
            .help( UTF8STDSTR( _( ARG_SORT_ASC_DESC ) ) )
            .implicit_value( true )
            .default_value( true );

    m_argParser.add_argument( ARG_FILTER )
            .help( UTF8STDSTR( _( ARG_FILTER_DESC ) ) )
            .default_value( std::string( "" ) );

    m_argParser.add_argument( ARG_EXCLUDE_DNP )
            .help( UTF8STDSTR( _( ARG_EXCLUDE_DNP_DESC ) ) )
            .implicit_value( true )
            .default_value( false );

    // Output formatting options
    m_argParser.add_argument( ARG_FIELD_DELIMITER )
            .help( UTF8STDSTR( _( ARG_FIELD_DELIMITER_DESC ) ) )
            .default_value( std::string( "," ) );

    m_argParser.add_argument( ARG_STRING_DELIMITER )
            .help( UTF8STDSTR( _( ARG_STRING_DELIMITER_DESC ) ) )
            .default_value( std::string( "\"" ) );

    m_argParser.add_argument( ARG_REF_DELIMITER )
            .help( UTF8STDSTR( _( ARG_REF_DELIMITER_DESC ) ) )
            .default_value( std::string( "," ) );

    m_argParser.add_argument( ARG_REF_RANGE_DELIMITER )
            .help( UTF8STDSTR( _( ARG_REF_RANGE_DELIMITER_DESC ) ) )
            .default_value( std::string( "-" ) );

    m_argParser.add_argument( ARG_KEEP_TABS )
            .help( UTF8STDSTR( _( ARG_KEEP_TABS_DESC ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( ARG_KEEP_LINE_BREAKS )
            .help( UTF8STDSTR( _( ARG_KEEP_LINE_BREAKS_DESC ) ) )
            .implicit_value( true )
            .default_value( false );
}


std::vector<wxString> CLI::EXPORT_SCH_BOM_COMMAND::convertStringList( const wxString& aList )
{
    std::vector<wxString> v;

    if( !aList.IsEmpty() )
    {
        wxStringTokenizer layerTokens( aList, "," );

        while( layerTokens.HasMoreTokens() )
            v.emplace_back( layerTokens.GetNextToken() );
    }

    return v;
}

int CLI::EXPORT_SCH_BOM_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_EXPORT_SCH_BOM> bomJob = std::make_unique<JOB_EXPORT_SCH_BOM>( true );

    // Basic options
    bomJob->m_filename = FROM_UTF8( m_argParser.get<std::string>( ARG_INPUT ).c_str() );
    bomJob->m_outputFile = FROM_UTF8( m_argParser.get<std::string>( ARG_OUTPUT ).c_str() );

    // Format options
    bomJob->m_fieldDelimiter =
            FROM_UTF8( m_argParser.get<std::string>( ARG_FIELD_DELIMITER ).c_str() );
    bomJob->m_stringDelimiter =
            FROM_UTF8( m_argParser.get<std::string>( ARG_STRING_DELIMITER ).c_str() );
    bomJob->m_refDelimiter = FROM_UTF8( m_argParser.get<std::string>( ARG_REF_DELIMITER ).c_str() );
    bomJob->m_refRangeDelimiter =
            FROM_UTF8( m_argParser.get<std::string>( ARG_REF_RANGE_DELIMITER ).c_str() );
    bomJob->m_keepTabs = m_argParser.get<bool>( ARG_KEEP_TABS );
    bomJob->m_keepLineBreaks = m_argParser.get<bool>( ARG_KEEP_LINE_BREAKS );

    // Output fields options
    bomJob->m_fieldsOrdered =
            convertStringList( FROM_UTF8( m_argParser.get<std::string>( ARG_FIELDS ).c_str() ) );
    bomJob->m_fieldsLabels =
            convertStringList( FROM_UTF8( m_argParser.get<std::string>( ARG_LABELS ).c_str() ) );
    bomJob->m_fieldsGroupBy =
            convertStringList( FROM_UTF8( m_argParser.get<std::string>( ARG_GROUP_BY ).c_str() ) );
    bomJob->m_sortField = FROM_UTF8( m_argParser.get<std::string>( ARG_SORT_FIELD ).c_str() );
    bomJob->m_sortAsc = m_argParser.get<bool>( ARG_SORT_ASC );
    bomJob->m_filterString = FROM_UTF8( m_argParser.get<std::string>( ARG_FILTER ).c_str() );
    bomJob->m_excludeDNP = m_argParser.get<bool>( ARG_EXCLUDE_DNP );

    if( !wxFile::Exists( bomJob->m_filename ) )
    {
        wxFprintf( stderr, _( "Schematic file does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    int exitCode = aKiway.ProcessJob( KIWAY::FACE_SCH, bomJob.get() );

    return exitCode;
}
