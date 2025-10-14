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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <jobs/jobs_output_folder.h>
#include <wx/filename.h>
#include <gestfich.h>
#include <common.h>

JOBS_OUTPUT_FOLDER::JOBS_OUTPUT_FOLDER() :
    JOBS_OUTPUT_HANDLER()
{

}


bool JOBS_OUTPUT_FOLDER::HandleOutputs( const wxString&                baseTempPath,
                                        PROJECT*                       aProject,
                                        const std::vector<JOB_OUTPUT>& aOutputsToHandle,
                                        std::optional<wxString>&       aResolvedOutputPath )
{
    aResolvedOutputPath.reset();

    wxString outputPath = ExpandTextVars( m_outputPath, aProject );
    outputPath = ExpandEnvVarSubstitutions( outputPath, aProject );

    if( outputPath.StartsWith( "~" ) )
        outputPath.Replace( "~", wxGetHomeDir(), false );

    if( !wxFileName::DirExists( outputPath ) )
    {
        if( !wxFileName::Mkdir( outputPath, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
        {
            aResolvedOutputPath.reset();
            return false;
        }
    }

    wxString errors;

    if( !CopyDirectory( baseTempPath, outputPath, errors ) )
    {
        aResolvedOutputPath.reset();
        return false;
    }

    aResolvedOutputPath = outputPath;

    return true;
}


bool JOBS_OUTPUT_FOLDER::OutputPrecheck()
{
    if( m_outputPath.IsEmpty() )
        return false;

    return true;
}


void JOBS_OUTPUT_FOLDER::FromJson( const nlohmann::json& j )
{
    m_outputPath = j.value( "output_path", "" );
}


void JOBS_OUTPUT_FOLDER::ToJson( nlohmann::json& j ) const
{
    j["output_path"] = m_outputPath;
}


wxString JOBS_OUTPUT_FOLDER::GetDefaultDescription() const
{
    return _( "Folder" );
}
