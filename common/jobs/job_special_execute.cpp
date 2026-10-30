/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Mark Roszko <mark.roszko@gmail.com>
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

#include <jobs/job_special_execute.h>
#include <jobs/job_registry.h>
#include <common.h>
#include <i18n_utility.h>
#include <paths.h>
#include <project.h>
#include <project/project_file.h>
#include <wildcards_and_files_ext.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/utils.h>

JOB_SPECIAL_EXECUTE::JOB_SPECIAL_EXECUTE() :
        JOB( "special_execute", false ),
        m_command(),
        m_ignoreExitcode( false ),
        m_recordOutput( true )
{
    m_params.emplace_back( new JOB_PARAM<wxString>( "command", &m_command, m_command ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "ignore_exit_code", &m_ignoreExitcode, m_ignoreExitcode ) );
    m_params.emplace_back(
            new JOB_PARAM<bool>( "record_output", &m_recordOutput, m_recordOutput ) );
}

wxString JOB_SPECIAL_EXECUTE::GetDefaultDescription() const
{
    return wxString( "Execute command" );
}


wxString FindBundledPython( const std::vector<wxString>& aToolDirs )
{
#ifdef __WXMSW__
    const wxChar* names[] = { wxS( "python.exe" ) };
#else
    // A framework bin directory ships python3 only
    const wxChar* names[] = { wxS( "python3" ), wxS( "python" ) };
#endif

    for( const wxString& dir : aToolDirs )
    {
        for( const wxChar* name : names )
        {
            wxFileName candidate( dir, name );

            if( candidate.FileExists() )
                return candidate.GetFullPath();
        }
    }

    return wxEmptyString;
}


// After a rename the only file of its kind in the project folder is the one the user means
static wxString onlyFileWithExt( const wxString& aDir, const wxString& aExt )
{
    wxArrayString files;
    wxDir::GetAllFiles( aDir, &files, wxS( "*." ) + aExt, wxDIR_FILES );

    return files.size() == 1 ? files[0] : wxString();
}


static wxString projectBoard( const PROJECT& aProject )
{
    wxString   dir = aProject.GetProjectPath();
    wxString   ext = wxString::FromUTF8( FILEEXT::KiCadPcbFileExtension );
    wxFileName board( dir, aProject.GetProjectName(), ext );

    if( !board.FileExists() )
    {
        wxString only = onlyFileWithExt( dir, ext );

        if( !only.IsEmpty() )
            return only;
    }

    return board.GetFullPath();
}


static wxString projectSchematic( const PROJECT& aProject )
{
    wxString dir = aProject.GetProjectPath();
    wxString ext = wxString::FromUTF8( FILEEXT::KiCadSchematicFileExtension );

    for( const TOP_LEVEL_SHEET_INFO& sheet : aProject.GetProjectFile().GetTopLevelSheets() )
    {
        wxFileName top( sheet.filename );
        top.MakeAbsolute( dir );

        if( top.FileExists() )
            return top.GetFullPath();
    }

    wxFileName schematic( dir, aProject.GetProjectName(), ext );

    if( !schematic.FileExists() )
    {
        wxString only = onlyFileWithExt( dir, ext );

        if( !only.IsEmpty() )
            return only;
    }

    return schematic.GetFullPath();
}


std::map<wxString, wxString> GetExecuteBuiltinVariables( const PROJECT* aProject )
{
    std::map<wxString, wxString> vars;

    wxString python = FindBundledPython( PATHS::GetToolDirectories( PATHS::GetExecutableDir() ) );

    if( python.IsEmpty() )
    {
#ifdef __WXMSW__
        python = wxS( "python" );
#else
        python = wxS( "python3" );
#endif
    }

    vars[EXEC_PYTHON_VAR_NAME] = python;

    if( aProject && !aProject->GetProjectFullName().IsEmpty() )
    {
        vars[EXEC_PROJECT_PCB_VAR_NAME] = projectBoard( *aProject );
        vars[EXEC_PROJECT_SCH_VAR_NAME] = projectSchematic( *aProject );
    }

    return vars;
}


std::map<wxString, wxString> GetExecuteBuiltinVariableHelp()
{
    return {
        { EXEC_PYTHON_VAR_NAME, _( "Python interpreter shipped with KiCad" ) },
        { EXEC_PROJECT_PCB_VAR_NAME, _( "Board file of the project" ) },
        { EXEC_PROJECT_SCH_VAR_NAME, _( "Top-level schematic file of the project" ) },
    };
}


wxString ExpandExecuteCommand( const wxString& aCommand, const PROJECT* aProject )
{
    wxString command = ExpandEnvVarSubstitutions( aCommand, aProject );

    // Text substitution because other threads expand variables, so the environment stays untouched
    // Done last so a user variable can refer to a built-in
    for( const auto& [name, value] : GetExecuteBuiltinVariables( aProject ) )
    {
        wxString existing = name;

        if( wxGetEnv( name, nullptr ) || ( aProject && aProject->TextVarResolver( &existing ) ) )
            continue;

        for( const wxString& form : { wxS( "${" ) + name + wxS( "}" ), wxS( "$(" ) + name + wxS( ")" ) } )
        {
            size_t pos = 0;

            while( ( pos = command.find( form, pos ) ) != wxString::npos )
            {
                if( pos > 0 && command[pos - 1] == wxS( '\\' ) )
                {
                    pos += form.length();
                    continue;
                }

                command.replace( pos, form.length(), value );
                pos += value.length();
            }
        }
    }

    return command;
}


REGISTER_JOB( special_execute, _HKI( "Special: Execute Command" ), KIWAY::KIWAY_FACE_COUNT,
              JOB_SPECIAL_EXECUTE );