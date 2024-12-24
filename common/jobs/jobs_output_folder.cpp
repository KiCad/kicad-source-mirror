/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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
                                        PROJECT* aProject,
                                        const std::vector<JOB_OUTPUT>& aOutputsToHandle )
{
    wxString outputPath = ExpandEnvVarSubstitutions( m_outputPath, aProject );

    if( wxFileName::DirExists( outputPath ) )
    {
        wxFileName::Rmdir( outputPath, wxPATH_RMDIR_RECURSIVE );
    }

    bool success = true;
    if( !wxFileName::Mkdir( outputPath, wxS_DIR_DEFAULT ) )
    {
        return false;
    }

    wxString errors;
    if( !CopyDirectory( baseTempPath, outputPath, errors ) )
    {
        success = false;
    }

    return success;
}


bool JOBS_OUTPUT_FOLDER::OutputPrecheck()
{
    if( m_outputPath.IsEmpty() )
    {
        return false;
    }

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