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

#include "command_project_downgrade.h"
#include "cli/exit_codes.h"
#include "../project_downgrade.h"

#include <json_common.h>
#include <filesystem>
#include <kiplatform/io.h>
#include <downgrade_scan.h>

#include <wx/crt.h>
#include <wx/file.h>
#include <wx/ffile.h>
#include <wx/filename.h>

#include <downgrade_target.h>

#define ARG_TARGET "--target"
#define ARG_FORCE "--force"
#define ARG_DRY_RUN "--dry-run"
#define ARG_REPORT_JSON "--report-json"
#define ARG_VARIANT "--variant"
#define ARG_DROP_APPROXIMATIONS "--drop-approximations"

CLI::PROJECT_DOWNGRADE_COMMAND::PROJECT_DOWNGRADE_COMMAND() :
        COMMAND( "downgrade" )
{
    addCommonArgs( true, true, IO_TYPE::FILE, IO_TYPE::DIRECTORY );
    m_argParser.add_description( UTF8STDSTR( _( "Export a copy of the whole project readable by an older "
                                                "KiCad version" ) ) );

    m_argParser.add_argument( ARG_TARGET )
            .default_value( std::string() )
            .help( UTF8STDSTR(
                    _( "Target version name or id, e.g. \"KiCad 10.0\" or \"10.0\". Defaults to the newest." ) ) );

    m_argParser.add_argument( ARG_FORCE )
            .help( UTF8STDSTR( _( "Export even if some features must be dropped or approximated" ) ) )
            .flag();

    m_argParser.add_argument( ARG_DRY_RUN )
            .help( UTF8STDSTR( _( "Print the compatibility report without writing anything" ) ) )
            .flag();

    m_argParser.add_argument( ARG_DROP_APPROXIMATIONS )
            .help( UTF8STDSTR(
                    _( "Omit features instead of approximating them; may remove drawing or electrical data" ) ) )
            .flag();

    m_argParser.add_argument( ARG_REPORT_JSON )
            .default_value( std::string() )
            .help( UTF8STDSTR( _( "Also write the compatibility report as JSON to this file" ) ) );

    m_argParser.add_argument( ARG_VARIANT )
            .default_value( std::string() )
            .help( UTF8STDSTR( _( "Flatten this variant into the export. Default is the base design." ) ) );
}


static const char* bucketName( DOWNGRADE_BUCKET aBucket )
{
    switch( aBucket )
    {
    case DOWNGRADE_BUCKET::KEEP: return "keep";
    case DOWNGRADE_BUCKET::LOWER: return "lower";
    case DOWNGRADE_BUCKET::DROP: return "drop";
    case DOWNGRADE_BUCKET::BLOCK: return "block";
    }

    return "unknown";
}


int CLI::PROJECT_DOWNGRADE_COMMAND::doPerform( KIWAY& aKiway )
{
    if( !wxFile::Exists( m_argInput ) )
    {
        wxFprintf( stderr, _( "Project file does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    wxFileName proFn( m_argInput );
    proFn.MakeAbsolute();

    wxString projectDir = proFn.GetPath();
    wxString projectName = proFn.GetName();

    wxString targetName = wxString::FromUTF8( m_argParser.get<std::string>( ARG_TARGET ) );

    if( targetName.IsEmpty() )
        targetName = GetDowngradeTargets().front().m_name;

    const DOWNGRADE_TARGET* target = FindDowngradeTarget( targetName );

    if( !target )
    {
        wxFprintf( stderr, _( "Unknown target version '%s'. Valid targets: %s\n" ), targetName,
                   DowngradeTargetNames() );
        return EXIT_CODES::ERR_ARGS;
    }

    // Argument problems surface before the classification runs, not one rerun at a time.
    if( !m_argParser.get<bool>( ARG_DRY_RUN ) && m_argOutput.IsEmpty() )
    {
        wxFprintf( stderr, _( "An output folder is required. Use --output.\n" ) );
        return EXIT_CODES::ERR_ARGS;
    }

    wxString jsonPath = wxString::FromUTF8( m_argParser.get<std::string>( ARG_REPORT_JSON ) );

    if( !jsonPath.IsEmpty() )
    {
        wxFileName reportFile( jsonPath );
        reportFile.MakeAbsolute();
        jsonPath = reportFile.GetFullPath();
        wxArrayString sources;
        wxDir::GetAllFiles( projectDir, &sources );

        for( const wxString& source : sources )
        {
            if( !IsDowngradeDesignFile( source ) )
                continue;

            std::error_code error;
            bool sameFile = std::filesystem::equivalent( jsonPath.ToStdWstring(), source.ToStdWstring(), error );

            if( sameFile || reportFile.SameAs( wxFileName( source ) ) )
            {
                wxFprintf( stderr, _( "The report file must differ from the source project files.\n" ) );
                return EXIT_CODES::ERR_ARGS;
            }
        }
    }

    COMPATIBILITY_REPORT report;
    wxString             error;
    bool                 dropInsteadOfApproximate = m_argParser.get<bool>( ARG_DROP_APPROXIMATIONS );
    wxString             variant = wxString::FromUTF8( m_argParser.get<std::string>( ARG_VARIANT ) );

    if( !ClassifyProjectForDowngrade( aKiway, projectDir, projectName, targetName, report, error, nullptr,
                                      dropInsteadOfApproximate, variant ) )
    {
        wxFprintf( stderr, wxT( "%s\n" ), error );
        return EXIT_CODES::ERR_UNKNOWN;
    }

    // The same printer the pcb and sch downgrade commands use, so the vocabulary matches.
    report.Print( &CLI_REPORTER::GetInstance() );

    if( !jsonPath.IsEmpty() )
    {
        nlohmann::json js;
        js["target"] = std::string( target->m_name.ToUTF8() );
        js["blocked"] = report.IsBlocked();
        js["lossy"] = report.IsLossy();
        js["drop_approximations"] = dropInsteadOfApproximate;
        js["entries"] = nlohmann::json::array();

        for( const COMPAT_ENTRY& entry : report.Entries() )
        {
            js["entries"].push_back( { { "category", bucketName( entry.m_bucket ) },
                                       { "feature", std::string( entry.m_feature.ToUTF8() ) },
                                       { "detail", std::string( entry.m_detail.ToUTF8() ) },
                                       { "count", entry.m_count } } );
        }

        std::string text = js.dump( 2 ) + "\n";

        if( !KIPLATFORM::IO::AtomicWriteFile( jsonPath, text.data(), text.size(), &error ) )
        {
            wxFprintf( stderr, _( "Could not write the JSON report to '%s'\n" ), jsonPath );
            return EXIT_CODES::ERR_UNKNOWN;
        }
    }

    if( m_argParser.get<bool>( ARG_DRY_RUN ) )
        return EXIT_CODES::OK;

    if( report.IsBlocked() )
    {
        wxFprintf( stderr, _( "Cannot export: the project uses features the target cannot represent.\n" ) );
        return EXIT_CODES::ERR_UNKNOWN;
    }

    if( report.IsLossy() && !m_argParser.get<bool>( ARG_FORCE ) )
    {
        wxFprintf( stderr, _( "Export would lose data. Re-run with --force to export anyway.\n" ) );
        return EXIT_CODES::ERR_UNKNOWN;
    }

    wxFileName outDir( m_argOutput, wxEmptyString );
    outDir.MakeAbsolute();

    if( !ExportProjectForDowngrade( aKiway, projectDir, projectName, targetName, outDir.GetPath(), error, variant,
                                    dropInsteadOfApproximate ) )
    {
        wxFprintf( stderr, wxT( "%s\n" ), error );
        return EXIT_CODES::ERR_UNKNOWN;
    }

    wxPrintf( _( "Exported to '%s'\n" ), outDir.GetPath() );
    return EXIT_CODES::OK;
}
