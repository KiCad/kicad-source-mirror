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

#include <jobs/jobs_output_archive.h>
#include <wx/fs_zip.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include <gestfich.h>
#include <common.h>
#include <wildcards_and_files_ext.h>

JOBS_OUTPUT_ARCHIVE::JOBS_OUTPUT_ARCHIVE() :
    JOBS_OUTPUT_HANDLER(),
    m_format( FORMAT::ZIP )
{
}


bool JOBS_OUTPUT_ARCHIVE::OutputPrecheck()
{
    if( m_outputPath.IsEmpty() )
        return false;

    return true;
}


bool JOBS_OUTPUT_ARCHIVE::HandleOutputs( const wxString&                baseTempPath,
                                         PROJECT*                       aProject,
                                         const std::vector<JOB_OUTPUT>& aOutputsToHandle,
                                         std::optional<wxString>&       aResolvedOutputPath )
{
    bool success = true;
    aResolvedOutputPath.reset();

    wxString outputPath = ExpandTextVars( m_outputPath, aProject );
    outputPath = ExpandEnvVarSubstitutions( outputPath, aProject );

    if( outputPath.StartsWith( "~" ) )
        outputPath.Replace( "~", wxGetHomeDir(), false );

    outputPath = EnsureFileExtension( outputPath, FILEEXT::ArchiveFileExtension );

    wxFFileOutputStream ostream( outputPath );

    if( !ostream.IsOk() ) // issue to create the file. Perhaps not writable dir
    {
        //msg.Printf( _( "Failed to create file '%s'." ), aDestFile );
        //aReporter.Report( msg, RPT_SEVERITY_ERROR );
        aResolvedOutputPath.reset();
        return false;
    }

    wxZipOutputStream zipstream( ostream, -1, wxConvUTF8 );
    wxString          errors;

    if( !AddDirectoryToZip( zipstream, baseTempPath, errors ) )
        success = false;

    if( !zipstream.Close() )
        success = false;

    if( success )
        aResolvedOutputPath = outputPath;
    else
        aResolvedOutputPath.reset();

    return success;
}


void JOBS_OUTPUT_ARCHIVE::FromJson( const nlohmann::json& j )
{
	m_outputPath = j.value( "output_path", "" );
    m_format = FORMAT::ZIP;
}


void JOBS_OUTPUT_ARCHIVE::ToJson( nlohmann::json& j ) const
{
    j["output_path"] = m_outputPath;
	j["format"] = "zip";
}


wxString JOBS_OUTPUT_ARCHIVE::GetDefaultDescription() const
{
    return _( "Archive" );
}