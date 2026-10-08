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

#include "command_sch_downgrade.h"
#include "jobs/job_sch_downgrade.h"
#include "cli/exit_codes.h"
#include <wx/crt.h>
#include <wx/file.h>
#include <wx/filename.h>

#define ARG_TARGET "--target"
#define ARG_FORCE "--force"
#define ARG_DRY_RUN "--dry-run"
#define ARG_LIBRARIES "--libraries"
#define ARG_VARIANT "--variant"
#define ARG_DROP_APPROXIMATIONS "--drop-approximations"

CLI::SCH_DOWNGRADE_COMMAND::SCH_DOWNGRADE_COMMAND() :
        COMMAND( "downgrade" )
{
    addCommonArgs( true, true, IO_TYPE::FILE, IO_TYPE::DIRECTORY );
    m_argParser.add_description( UTF8STDSTR( _( "Export a copy of the schematic readable by an older KiCad version. "
                                                "A new --output folder is required except for --dry-run." ) ) );

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

    m_argParser.add_argument( ARG_LIBRARIES )
            .default_value( std::string() )
            .help( UTF8STDSTR(
                    _( "Copy this symbol-library folder into the output folder and downgrade the copies" ) ) );

    m_argParser.add_argument( ARG_VARIANT )
            .default_value( std::string() )
            .help( UTF8STDSTR( _( "Flatten this variant into the export. Default is the base design." ) ) );
}

int CLI::SCH_DOWNGRADE_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_SCH_DOWNGRADE> job = std::make_unique<JOB_SCH_DOWNGRADE>();

    job->m_filename = m_argInput;
    job->m_outputDir = m_argOutput;
    job->m_target = wxString::FromUTF8( m_argParser.get<std::string>( ARG_TARGET ) );
    job->m_force = m_argParser.get<bool>( ARG_FORCE );
    job->m_dryRun = m_argParser.get<bool>( ARG_DRY_RUN );
    job->m_dropInsteadOfApproximate = m_argParser.get<bool>( ARG_DROP_APPROXIMATIONS );
    job->m_libraryDir = wxString::FromUTF8( m_argParser.get<std::string>( ARG_LIBRARIES ) );
    job->m_variant = wxString::FromUTF8( m_argParser.get<std::string>( ARG_VARIANT ) );

    if( !wxFile::Exists( job->m_filename ) )
    {
        wxFprintf( stderr, _( "Schematic file does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    if( !job->m_dryRun && job->m_outputDir.IsEmpty() )
    {
        wxFprintf( stderr, _( "An output folder is required. Use --output.\n" ) );
        return EXIT_CODES::ERR_ARGS;
    }

    if( !job->m_outputDir.IsEmpty() && !job->m_dryRun )
    {
        wxFileName inFn( job->m_filename );
        wxFileName outDir( job->m_outputDir, wxEmptyString );
        inFn.MakeAbsolute();
        outDir.MakeAbsolute();
        job->m_outputDir = outDir.GetPath();

        wxFileName inDir( inFn.GetPath(), wxEmptyString );

        if( outDir.SameAs( inDir ) )
        {
            wxFprintf( stderr, _( "The output folder must differ from the schematic folder.\n" ) );
            return EXIT_CODES::ERR_ARGS;
        }
    }

    return aKiway.ProcessJob( KIWAY::FACE_SCH, job.get() );
}
