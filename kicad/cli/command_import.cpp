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

#include "command_import.h"
#include <cli/exit_codes.h>
#include <jobs/job_pcb_import.h>
#include <jobs/job_sch_import.h>
#include <jobs/job_import_utils.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <project.h>
#include <project/project_file.h>
#include <reporter.h>
#include <kiid.h>
#include <string_utils.h>
#include <macros.h>
#include <wildcards_and_files_ext.h>
#include <wx/crt.h>
#include <wx/filename.h>

#include <memory>
#include <vector>


#define ARG_LAYER_MAP "--layer-map"


CLI::IMPORT_COMMAND::IMPORT_COMMAND() : COMMAND( "import" )
{
    m_argParser.add_description( UTF8STDSTR(
            _( "Import non-KiCad board and/or schematic files into a new KiCad project" ) ) );

    m_argParser.add_argument( ARG_INPUT )
            .help( UTF8STDSTR( _( "Input file(s) to import; each is autodetected as a board or "
                                  "schematic" ) ) )
            .nargs( argparse::nargs_pattern::at_least_one )
            .metavar( "INPUT_FILES" );

    m_argParser.add_argument( "-o", ARG_OUTPUT )
            .default_value( std::string() )
            .help( UTF8STDSTR( _( "Output project path stem; produces <stem>.kicad_pro plus the "
                                  "imported <stem>.kicad_pcb and/or <stem>.kicad_sch beside it. "
                                  "Defaults to the first input's name in the current directory." ) ) )
            .metavar( "PROJECT" );

    m_argParser.add_argument( ARG_LAYER_MAP )
            .default_value( std::string( "" ) )
            .help( UTF8STDSTR( _( "JSON file mapping source layer names to KiCad layer names for "
                                  "imported boards; unmapped layers use the automatic best-guess" ) ) )
            .metavar( "FILE" );
}


int CLI::IMPORT_COMMAND::doPerform( KIWAY& aKiway )
{
    std::vector<std::string> rawInputs = m_argParser.get<std::vector<std::string>>( ARG_INPUT );
    wxString outputArg = From_UTF8( m_argParser.get<std::string>( ARG_OUTPUT ).c_str() );

    if( rawInputs.empty() )
    {
        wxFprintf( stderr, _( "At least one input file is required\n" ) );
        return EXIT_CODES::ERR_ARGS;
    }

    std::vector<wxString> inputFiles;

    for( const std::string& raw : rawInputs )
        inputFiles.push_back( From_UTF8( raw.c_str() ) );

    std::map<wxString, wxString> layerMap;
    wxString layerMapFile = From_UTF8( m_argParser.get<std::string>( ARG_LAYER_MAP ).c_str() );

    if( !layerMapFile.IsEmpty() )
    {
        wxString error;

        if( !LoadLayerMapFile( layerMapFile, layerMap, error ) )
        {
            wxFprintf( stderr, wxS( "%s\n" ), error );
            return EXIT_CODES::ERR_ARGS;
        }
    }

    wxFileName projectFn;

    if( !outputArg.IsEmpty() )
    {
        projectFn.Assign( outputArg );
    }
    else
    {
        wxFileName firstInput( inputFiles.front() );
        projectFn.AssignDir( wxFileName::GetCwd() );
        projectFn.SetName( firstInput.GetName() );
    }

    projectFn.SetExt( FILEEXT::ProjectFileExtension );
    projectFn.MakeAbsolute();

    if( !projectFn.DirExists()
        && !wxFileName::Mkdir( projectFn.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
    {
        wxFprintf( stderr, _( "Could not create output directory: %s\n" ), projectFn.GetPath() );
        return EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    wxString stem = projectFn.GetName();
    wxString dir = projectFn.GetPath();

    wxFileName boardFn( dir, stem, FILEEXT::KiCadPcbFileExtension );
    wxFileName schFn( dir, stem, FILEEXT::KiCadSchematicFileExtension );

    // LoadProject returns false for a not-yet-existing file but still registers a fully-defaulted
    // active project, so success is signalled by GetProject() returning it.
    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();

    mgr.LoadProject( projectFn.GetFullPath(), true );

    PROJECT* project = mgr.GetProject( projectFn.GetFullPath() );

    if( !project )
    {
        wxFprintf( stderr, _( "Could not create project: %s\n" ), projectFn.GetFullPath() );
        return EXIT_CODES::ERR_UNKNOWN;
    }
    REPORTER& reporter = CLI_REPORTER::GetInstance();

    bool haveBoard = false;
    bool haveSch = false;
    int  retCode = EXIT_CODES::SUCCESS;

    for( const wxString& input : inputFiles )
    {
        // Classify by trial dispatch: a face that does not recognize the file returns the
        // ERR_UNKNOWN_FILE_FORMAT sentinel before loading, so we fall through to the next face.
        // Probe a face only while its slot is open so a duplicate input is rejected without
        // overwriting an earlier output.
        if( !haveBoard )
        {
            std::unique_ptr<JOB_PCB_IMPORT> pcbJob = std::make_unique<JOB_PCB_IMPORT>();
            pcbJob->m_inputFile = input;
            pcbJob->m_format = JOB_PCB_IMPORT::FORMAT::AUTO;
            pcbJob->m_layerMap = layerMap;
            pcbJob->SetConfiguredOutputPath( boardFn.GetFullPath() );

            int pcbResult = aKiway.ProcessJob( KIWAY::FACE_PCB, pcbJob.get(), &reporter );

            if( pcbResult == EXIT_CODES::SUCCESS )
            {
                haveBoard = true;
                continue;
            }
            else if( pcbResult != EXIT_CODES::ERR_UNKNOWN_FILE_FORMAT )
            {
                retCode = pcbResult;
                break;
            }
        }

        if( !haveSch )
        {
            std::unique_ptr<JOB_SCH_IMPORT> schJob = std::make_unique<JOB_SCH_IMPORT>();
            schJob->m_inputFile = input;
            schJob->m_format = JOB_SCH_IMPORT::FORMAT::AUTO;
            schJob->SetConfiguredOutputPath( schFn.GetFullPath() );

            int schResult = aKiway.ProcessJob( KIWAY::FACE_SCH, schJob.get(), &reporter );

            if( schResult == EXIT_CODES::SUCCESS )
            {
                haveSch = true;
                continue;
            }
            else if( schResult != EXIT_CODES::ERR_UNKNOWN_FILE_FORMAT )
            {
                retCode = schResult;
                break;
            }
        }

        if( haveBoard && haveSch )
        {
            wxFprintf( stderr, _( "Could not import '%s': a board and a schematic have already "
                                  "been imported for this project\n" ), input );
        }
        else
        {
            wxFprintf( stderr, _( "No board or schematic importer recognizes the file: %s\n" ),
                       input );
        }

        retCode = EXIT_CODES::ERR_INVALID_INPUT_FILE;
        break;
    }

    if( retCode == EXIT_CODES::SUCCESS && !haveBoard && !haveSch )
    {
        wxFprintf( stderr, _( "No inputs could be imported\n" ) );
        retCode = EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    if( retCode == EXIT_CODES::SUCCESS && project )
    {
        // The schematic handler already registered any sheets; link the board here.
        if( haveBoard )
        {
            std::vector<FILE_INFO_PAIR>& boards = project->GetProjectFile().GetBoards();
            boards.clear();
            boards.emplace_back( std::make_pair( KIID(), boardFn.GetFullName() ) );
        }

        if( mgr.SaveProject( projectFn.GetFullPath() ) )
        {
            reporter.Report( wxString::Format( _( "Created project '%s'\n" ),
                                               projectFn.GetFullPath() ),
                             RPT_SEVERITY_INFO );
        }
        else
        {
            wxFprintf( stderr, _( "Failed to write project file: %s\n" ),
                       projectFn.GetFullPath() );
            retCode = EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
        }
    }

    mgr.UnloadProject( project, false );

    return retCode;
}
