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

#include "ipc2581_export_job.h"
#include "pcb_io_ipc2581.h"

#include <map>

#include <wx/filefn.h>
#include <wx/filename.h>

#include <board.h>
#include <core/utf8.h>
#include <gestfich.h>
#include <jobs/job_export_pcb_ipc2581.h>
#include <ki_exception.h>
#include <paths.h>
#include <project.h>
#include <project/project_file.h>
#include <reporter.h>
#include <wildcards_and_files_ext.h>


bool GenerateIpc2581File( JOB_EXPORT_PCB_IPC2581& aJob, BOARD* aBoard, PROGRESS_REPORTER* aProgressReporter,
                          REPORTER* aReporter )
{
    wxCHECK( aBoard, false );
    wxString outPath = aJob.GetFullOutputPath( aBoard->GetProject() );

    if( !PATHS::EnsurePathExists( outPath, true ) )
    {
        if( aReporter )
            aReporter->Report( _( "Failed to create output directory\n" ), RPT_SEVERITY_ERROR );

        return false;
    }

    std::map<std::string, UTF8> props;
    props["units"] = aJob.m_units == JOB_EXPORT_PCB_FAB::UNITS::MM ? "mm" : "inch";
    props["sigfig"] = wxString::Format( "%d", aJob.m_precision );
    props["version"] = aJob.m_version == JOB_EXPORT_PCB_IPC2581::IPC2581_VERSION::C ? "C" : "B";
    props["OEMRef"] = aJob.m_colInternalId;
    props["mpn"] = aJob.m_colMfgPn;
    props["mfg"] = aJob.m_colMfg;
    props["dist"] = aJob.m_colDist;
    props["distpn"] = aJob.m_colDistPn;

    if( !aJob.m_variantNames.empty() )
        props["variant"] = aJob.m_variantNames.front();

    if( !aJob.SupportsDataSet( aJob.m_dataSet ) )
    {
        if( aReporter )
            aReporter->Report( _( "Unsupported IPC-2581 data set." ), RPT_SEVERITY_ERROR );

        return false;
    }

    props["mode"] = IPC2581::ModeToken( aJob.m_dataSet );

    if( aJob.m_sections )
        props["sections"] = *aJob.m_sections;

    if( aJob.m_netNames == JOB_EXPORT_PCB_FAB::NET_NAMES::ANONYMIZE )
        props["netnames"] = "anonymize";

    if( aJob.m_refDes == JOB_EXPORT_PCB_IPC2581::REF_DES::OMIT )
        props["refdes"] = "omit";

    wxString bomRev = aJob.m_bomRev;

    if( bomRev.IsEmpty() && aBoard->GetProject() )
    {
        const IP2581_BOM& bomSettings = aBoard->GetProject()->GetProjectFile().m_IP2581Bom;
        bomRev = bomSettings.bomRev;

        if( bomRev.IsEmpty() )
            bomRev = bomSettings.schRevision;
    }

    if( !bomRev.IsEmpty() )
        props["bomrev"] = bomRev;

    SCOPED_TEMP_PATH temporary;
    wxString         tempFile;

    // A compressed export writes the file into a directory that becomes the archive
    if( aJob.m_compress )
    {
        wxFileName xmlName = outPath;
        xmlName.SetExt( FILEEXT::Ipc2581FileExtension );

        if( temporary.MakeDirectory( wxS( "pcbnew_ipc" ) ) )
            tempFile = wxFileName( temporary.Path(), xmlName.GetFullName() ).GetFullPath();
    }
    else if( temporary.MakeFile( wxS( "pcbnew_ipc" ) ) )
    {
        tempFile = temporary.Path();
    }

    if( tempFile.IsEmpty() )
    {
        if( aReporter )
            aReporter->Report( _( "Cannot create temporary IPC-2581 output." ), RPT_SEVERITY_ERROR );

        return false;
    }

    try
    {
        PCB_IO_IPC2581 plugin;
        plugin.SetProgressReporter( aProgressReporter );
        plugin.SetReporter( aReporter );
        plugin.SaveBoard( tempFile, *aBoard, &props );
    }
    catch( const IO_ERROR& ioe )
    {
        if( aReporter )
        {
            aReporter->Report( wxString::Format( _( "Error generating IPC-2581 file '%s'.\n%s" ),
                                                  outPath,
                                                  ioe.What() ),
                                RPT_SEVERITY_ERROR );
        }

        return false;
    }

    if( aJob.m_compress )
    {
        wxString error;

        if( !WriteDirectoryArchive( temporary.Path(), outPath, ARCHIVE_FORMAT::ZIP, wxEmptyString, &error ) )
        {
            if( aReporter )
            {
                aReporter->Report( wxString::Format( _( "Cannot write IPC-2581 archive '%s'.\n%s" ),
                                                     outPath, error ), RPT_SEVERITY_ERROR );
            }

            return false;
        }

        aJob.AddOutput( outPath );
        return true;
    }

    // If save succeeded, replace the original with what we just wrote
    if( !wxRenameFile( tempFile, outPath ) )
    {
        if( aReporter )
        {
            aReporter->Report( wxString::Format( _( "Error generating IPC-2581 file '%s'.\n"
                                                     "Failed to rename temporary file '%s'." ),
                                                  outPath,
                                                  tempFile ),
                                RPT_SEVERITY_ERROR );
        }

        return false;
    }

    aJob.AddOutput( outPath );
    return true;
}
