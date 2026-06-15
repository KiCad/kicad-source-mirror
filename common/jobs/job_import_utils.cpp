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

#include <jobs/job_import_utils.h>
#include <reporter.h>
#include <string_utils.h>
#include <wx/file.h>
#include <wx/ffile.h>
#include <wx/filename.h>
#include <wx/intl.h>


bool ParseImportReportFormat( const wxString& aText, IMPORT_REPORT_FORMAT& aFormat )
{
    if( aText == wxS( "none" ) )
        aFormat = IMPORT_REPORT_FORMAT::NONE;
    else if( aText == wxS( "json" ) )
        aFormat = IMPORT_REPORT_FORMAT::JSON;
    else if( aText == wxS( "text" ) )
        aFormat = IMPORT_REPORT_FORMAT::TEXT;
    else
        return false;

    return true;
}


wxString DefaultImportOutputPath( const wxString& aInputFile, const wxString& aKiCadExt )
{
    wxFileName fn( aInputFile );
    fn.SetExt( aKiCadExt );

    return fn.GetFullPath();
}


bool LoadLayerMapFile( const wxString& aFile, std::map<wxString, wxString>& aMap, wxString& aError )
{
    wxFFile file( aFile, wxS( "rb" ) );

    if( !file.IsOpened() )
    {
        aError = wxString::Format( _( "Could not open layer map file '%s'" ), aFile );
        return false;
    }

    wxString content;

    if( !file.ReadAll( &content ) )
    {
        aError = wxString::Format( _( "Could not read layer map file '%s'" ), aFile );
        return false;
    }

    nlohmann::json json;

    try
    {
        json = nlohmann::json::parse( content.ToStdString() );
    }
    catch( const nlohmann::json::exception& e )
    {
        aError = wxString::Format( _( "Layer map file '%s' is not valid JSON: %s" ), aFile,
                                   From_UTF8( e.what() ) );
        return false;
    }

    if( !json.is_object() )
    {
        aError = wxString::Format( _( "Layer map file '%s' must be a JSON object mapping source "
                                      "layer names to KiCad layer names" ), aFile );
        return false;
    }

    std::map<wxString, wxString> parsed;

    for( auto it = json.begin(); it != json.end(); ++it )
    {
        if( !it.value().is_string() )
        {
            aError = wxString::Format( _( "Layer map entry for '%s' must be a KiCad layer name "
                                          "string" ), From_UTF8( it.key().c_str() ) );
            return false;
        }

        parsed[From_UTF8( it.key().c_str() )] = From_UTF8( it.value().get<std::string>().c_str() );
    }

    aMap = std::move( parsed );
    return true;
}


void WriteImportReport( REPORTER* aReporter, IMPORT_REPORT_FORMAT aFormat,
                        const wxString& aReportFile, const IMPORT_REPORT_DATA& aData )
{
    if( aFormat == IMPORT_REPORT_FORMAT::NONE )
        return;

    wxString output;

    if( aFormat == IMPORT_REPORT_FORMAT::JSON )
    {
        nlohmann::json report;

        report["source_file"] = aData.m_sourceFile.ToStdString();
        report["source_format"] = aData.m_sourceFormat.ToStdString();
        report["output_file"] = aData.m_outputFile.ToStdString();

        nlohmann::json statistics = nlohmann::json::object();

        for( const auto& [key, value] : aData.m_statistics )
            statistics[key.ToStdString()] = value;

        report["statistics"] = statistics;

        for( auto it = aData.m_extraJson.begin(); it != aData.m_extraJson.end(); ++it )
            report[it.key()] = it.value();

        nlohmann::json warningsJson = nlohmann::json::array();

        for( const wxString& warning : aData.m_warnings )
            warningsJson.push_back( warning.ToStdString() );

        report["warnings"] = warningsJson;

        nlohmann::json errorsJson = nlohmann::json::array();

        for( const wxString& error : aData.m_errors )
            errorsJson.push_back( error.ToStdString() );

        report["errors"] = errorsJson;

        output = wxString::FromUTF8( report.dump( 2 ) );
    }
    else
    {
        output += wxS( "Import Report\n" );
        output += wxS( "=============\n\n" );
        output += wxString::Format( wxS( "Source file: %s\n" ), aData.m_sourceFile );
        output += wxString::Format( wxS( "Source format: %s\n" ), aData.m_sourceFormat );
        output += wxString::Format( wxS( "Output file: %s\n\n" ), aData.m_outputFile );
        output += wxS( "Statistics:\n" );

        for( const auto& [key, value] : aData.m_statistics )
        {
            wxString label = key.Left( 1 ).Upper() + key.Mid( 1 );
            output += wxString::Format( wxS( "  %s: %zu\n" ), label, value );
        }

        if( !aData.m_warnings.empty() )
        {
            output += wxS( "\nWarnings:\n" );

            for( const wxString& warning : aData.m_warnings )
                output += wxString::Format( wxS( "  - %s\n" ), warning );
        }
    }

    if( !aReportFile.IsEmpty() )
    {
        wxFile file( aReportFile, wxFile::write );

        if( file.IsOpened() )
        {
            file.Write( output );
            file.Close();
        }
        else if( aReporter )
        {
            aReporter->Report( wxString::Format( _( "Failed to write import report to '%s'\n" ),
                                                 aReportFile ),
                               RPT_SEVERITY_ERROR );
        }
    }
    else if( aReporter )
    {
        aReporter->Report( output + wxS( "\n" ), RPT_SEVERITY_INFO );
    }
}
