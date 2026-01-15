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

#include "dialog_destination.h"
#include "dialog_copyfiles_job_settings.h"
#include <wx/aui/auibook.h>
#include <jobs/jobset.h>
#include <jobs/job_registry.h>
#include <wx/checkbox.h>
#include <wx/menu.h>
#include <bitmaps.h>
#include <jobs/jobs_output_archive.h>
#include <kicad_manager_frame.h>
#include <vector>

#include <wx/dirdlg.h>
#include <wx/filedlg.h>
#include <wildcards_and_files_ext.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/wx_grid.h>
#include <widgets/grid_text_button_helpers.h>
#include <confirm.h>


extern KICOMMON_API
std::map<JOBSET_DESTINATION_T, JOBSET_DESTINATION_T_INFO> JobsetDestinationTypeInfos;


DIALOG_DESTINATION::DIALOG_DESTINATION( wxWindow* aParent, JOBSET* aJobsFile,
                                        JOBSET_DESTINATION* aDestination ) :
        DIALOG_DESTINATION_BASE( aParent ),
        m_jobsFile( aJobsFile ),
        m_destination( aDestination )
{
    // prevent someone from failing to add the type info in the future
    wxASSERT( JobsetDestinationTypeInfos.contains( m_destination->m_type ) );

    SetTitle( wxString::Format( _( "%s Destination" ),
                                m_destination->m_outputHandler->GetDefaultDescription() ) );

    if( m_destination->m_type != JOBSET_DESTINATION_T::ARCHIVE )
    {
        m_textArchiveFormat->Hide();
        m_choiceArchiveformat->Hide();
    }

    m_buttonOutputPath->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );

    SetupStandardButtons();
}


void DIALOG_DESTINATION::onOutputPathBrowseClicked(wxCommandEvent& event)
{
    bool isFolder = false;
    wxString fileWildcard = "";
    isFolder = JobsetDestinationTypeInfos[m_destination->m_type].outputPathIsFolder;
    fileWildcard = JobsetDestinationTypeInfos[m_destination->m_type].fileWildcard;

    if( isFolder )
    {
        wxFileName fn;
        fn.AssignDir( m_textCtrlOutputPath->GetValue() );
        fn.Normalize( FN_NORMALIZE_FLAGS | wxPATH_NORM_ENV_VARS );
        wxString currPath = fn.GetFullPath();

        wxDirDialog dirDialog( this, _( "Select output directory" ), currPath,
                               wxDD_DEFAULT_STYLE );

        if( dirDialog.ShowModal() != wxID_OK )
            return;

        wxFileName dirName = wxFileName::DirName( dirDialog.GetPath() );

        m_textCtrlOutputPath->SetValue( dirName.GetFullPath() );
    }
    else
    {
        wxFileName fname( m_textCtrlOutputPath->GetValue() );

        wxFileDialog dlg( this, _( "Select output path" ), fname.GetPath(), fname.GetFullName(),
                          fileWildcard,
                          wxFD_OVERWRITE_PROMPT | wxFD_SAVE );

        if( dlg.ShowModal() != wxID_OK )
            return;

        wxString path = dlg.GetPath();

        if( m_destination->m_type == JOBSET_DESTINATION_T::ARCHIVE )
        {
            wxFileName fn( path );

            if( fn.GetExt().IsEmpty() )
                fn.SetExt( FILEEXT::ArchiveFileExtension );

            path = fn.GetFullPath();
        }

        m_textCtrlOutputPath->SetValue( path );
    }

}

bool DIALOG_DESTINATION::TransferDataFromWindow()
{
    wxString outputPath = m_textCtrlOutputPath->GetValue().Trim().Trim( false );

    if( outputPath.IsEmpty() )
    {
        DisplayErrorMessage( this, _( "Output path cannot be empty" ) );
        return false;
    }

    wxArrayInt selectedItems;
    m_includeJobs->GetCheckedItems( selectedItems );

    // Update the only job map
    m_destination->m_only.clear();

    if( selectedItems.size() < m_includeJobs->GetCount() )
    {
        for( int i : selectedItems )
        {
            if( m_onlyMap.contains( i ) )
                m_destination->m_only.emplace_back( m_onlyMap[i] );
        }
    }

    m_destination->m_outputHandler->SetOutputPath( outputPath );

    if( m_destination->m_type == JOBSET_DESTINATION_T::ARCHIVE )
    {
        JOBS_OUTPUT_ARCHIVE* archive =
                static_cast<JOBS_OUTPUT_ARCHIVE*>( m_destination->m_outputHandler.get() );

        archive->SetFormat( JOBS_OUTPUT_ARCHIVE::FORMAT::ZIP );
    }

    m_destination->SetDescription( m_textCtrlDescription->GetValue() );

    return true;
}


bool DIALOG_DESTINATION::TransferDataToWindow()
{
    m_textCtrlDescription->SetValue( m_destination->GetDescription() );
    m_textCtrlOutputPath->SetValue( m_destination->m_outputHandler->GetOutputPath() );

    wxArrayString    arrayStr;
    std::vector<int> selectedList;

    for( JOBSET_JOB& job : m_jobsFile->GetJobs() )
    {
        arrayStr.Add( wxString::Format( wxT( "%d.  %s" ),
                                        (int) arrayStr.size() + 1,
                                        job.GetDescription() ) );

        auto it = std::find_if( m_destination->m_only.begin(), m_destination->m_only.end(),
                                [&]( const wxString& only )
                                {
                                    if( only == job.m_id )
                                        return true;

                                    return false;
                                } );

        if( it != m_destination->m_only.end() )
            selectedList.emplace_back( arrayStr.size() - 1 );

        m_onlyMap.emplace( arrayStr.size() - 1, job.m_id );
    }

    if( arrayStr.size() != 0 )
    {
        m_includeJobs->InsertItems( arrayStr, 0 );

        if( selectedList.size() )
        {
            for( int idx : selectedList )
                m_includeJobs->Check( idx );
        }
        else
        {
            for( size_t idx = 0; idx < m_includeJobs->GetCount(); ++idx )
                m_includeJobs->Check( idx );
        }
    }

    m_choiceArchiveformat->AppendString( _( "Zip" ) );
    m_choiceArchiveformat->SetSelection( 0 );

    return true;
}
