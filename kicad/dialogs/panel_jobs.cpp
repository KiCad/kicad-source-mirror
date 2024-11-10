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

#include "panel_jobs.h"
#include <wx/aui/auibook.h>
#include <jobs/jobset.h>
#include <jobs/job_registry.h>
#include <eda_list_dialog.h>
#include <dialogs/dialog_job_config_base.h>
#include <wx/checkbox.h>
#include <wx/menu.h>
#include <bitmaps.h>
#include <i18n_utility.h>
#include <jobs_runner.h>
#include <widgets/wx_progress_reporters.h>
#include <jobs/jobs_output_archive.h>
#include <jobs/jobs_output_folder.h>
#include <kicad_manager_frame.h>
#include <vector>

#include <wx/dirdlg.h>
#include <wx/filedlg.h>
#include <wildcards_and_files_ext.h>
#include <widgets/std_bitmap_button.h>

#include <confirm.h>

#include <jobs/job_special_execute.h>
#include <dialogs/dialog_special_execute.h>


struct JOB_TYPE_INFO
{
    wxString    name;
    BITMAPS     bitmap;
    bool        outputPathIsFolder;
    wxString    fileWildcard;
};


static std::map<JOBSET_OUTPUT_TYPE, JOB_TYPE_INFO> jobTypeInfos = {
    { JOBSET_OUTPUT_TYPE::FOLDER,
        { _HKI( "Folder" ), BITMAPS::small_folder, true, "" } },
    { JOBSET_OUTPUT_TYPE::ARCHIVE,
        { _HKI( "Archive" ), BITMAPS::zip, false, FILEEXT::ZipFileWildcard() } },
};


class DIALOG_JOB_OUTPUT : public DIALOG_JOB_OUTPUT_BASE
{
public:
    DIALOG_JOB_OUTPUT( wxWindow* aParent, JOBSET* aJobsFile, JOBSET_OUTPUT* aOutput ) :
            DIALOG_JOB_OUTPUT_BASE( aParent ), m_jobsFile( aJobsFile ), m_output( aOutput )
    {
        SetAffirmativeId( wxID_SAVE );

        // prevent someone from failing to add the type info in the future
        wxASSERT( jobTypeInfos.contains( m_output->m_type ) );

        SetTitle( wxString::Format( _( "%s Output Options" ), jobTypeInfos[m_output->m_type].name ) );

        if( m_output->m_type != JOBSET_OUTPUT_TYPE::ARCHIVE )
        {
            m_textArchiveFormat->Hide();
            m_choiceArchiveformat->Hide();
        }

        m_buttonOutputPath->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );
    }


    virtual void onOutputPathBrowseClicked(wxCommandEvent& event) override
    {
        bool isFolder = false;
        wxString fileWildcard = "";
        isFolder = jobTypeInfos[m_output->m_type].outputPathIsFolder;
        fileWildcard = jobTypeInfos[m_output->m_type].fileWildcard;

        if( isFolder )
        {
            wxFileName fn;
            fn.AssignDir( m_textCtrlOutputPath->GetValue() );
            fn.Normalize( FN_NORMALIZE_FLAGS | wxPATH_NORM_ENV_VARS );
            wxString currPath = fn.GetFullPath();

            wxDirDialog dirDialog( this, _( "Select Templates Directory" ), currPath,
                                   wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST );

            if( dirDialog.ShowModal() != wxID_OK )
                return;

            wxFileName dirName = wxFileName::DirName( dirDialog.GetPath() );

            m_textCtrlOutputPath->SetValue( dirName.GetFullPath() );
        }
        else
        {
            wxFileDialog dlg( this, _( "Select output path" ), m_textCtrlOutputPath->GetValue(),
                              "test.zip",
                              fileWildcard,
                              wxFD_OVERWRITE_PROMPT | wxFD_SAVE );

            if( dlg.ShowModal() != wxID_OK )
            {
                return;
            }

            wxFileName fname_log( dlg.GetPath() );
        }

    }

    bool TransferDataFromWindow() override
    {
        wxArrayInt selectedItems;
        m_listBoxOnly->GetSelections( selectedItems );

        // Update the only job map
        m_output->m_only.clear();
        for( int i : selectedItems )
        {
            if( m_onlyMap.contains( i ) )
            {
                m_output->m_only.emplace_back( m_onlyMap[i] );
            }
        }

        m_output->m_outputHandler->SetOutputPath( m_textCtrlOutputPath->GetValue() );

        if( m_output->m_type == JOBSET_OUTPUT_TYPE::ARCHIVE )
        {
            JOBS_OUTPUT_ARCHIVE* archive =
                    static_cast<JOBS_OUTPUT_ARCHIVE*>( m_output->m_outputHandler );

            archive->SetFormat( JOBS_OUTPUT_ARCHIVE::FORMAT::ZIP );
        }


        return true;
    }


    bool TransferDataToWindow() override
    {
        wxArrayString arrayStr;
        std::vector<int>    selectedList;

        std::vector<JOBSET_JOB> jobs = m_jobsFile->GetJobs();
        int                        i = 0;
        for( JOBSET_JOB& job : jobs )
        {
            arrayStr.Add( job.m_job->GetDescription() );

            auto it = std::find_if( m_output->m_only.begin(), m_output->m_only.end(),
                                    [&]( const wxString& only )
                                    {
                                        if( only == job.m_id )
                                            return true;

                                        return false;
                                    } );

            if( it != m_output->m_only.end() )
            {
                selectedList.emplace_back( i );
            }

            m_onlyMap.emplace( i, job.m_id );
            i++;
        }

        m_listBoxOnly->InsertItems( arrayStr, 0 );

        for( int idx : selectedList )
        {
            m_listBoxOnly->SetSelection( idx );
        }

        m_choiceArchiveformat->AppendString( _( "Zip" ) );
        m_choiceArchiveformat->SetSelection( 0 );

        return true;
    }


private:
    JOBSET*        m_jobsFile;
    JOBSET_OUTPUT* m_output;
    std::map<int, wxString> m_onlyMap;
};


class PANEL_JOB_OUTPUT : public PANEL_JOB_OUTPUT_BASE
{
public:
    PANEL_JOB_OUTPUT( wxWindow* aParent, PANEL_JOBS* aPanelParent, KICAD_MANAGER_FRAME* aFrame,
                     JOBSET* aFile, JOBSET_OUTPUT* aOutput ) :
            PANEL_JOB_OUTPUT_BASE( aParent ),
            m_jobsFile( aFile ),
            m_output( aOutput ),
            m_frame( aFrame ),
            m_panelParent( aPanelParent )
    {
        m_buttonOutputRun->SetBitmap( KiBitmapBundle( BITMAPS::sim_run ) );
        m_buttonOutputOptions->SetBitmap( KiBitmapBundle( BITMAPS::preference ) );

        m_buttonOutputOptions->Connect( wxEVT_MENU,
                                        wxCommandEventHandler( PANEL_JOB_OUTPUT::onMenu ), nullptr, this );

        if( jobTypeInfos.contains( aOutput->m_type ) )
        {
            JOB_TYPE_INFO& jobTypeInfo = jobTypeInfos[aOutput->m_type];
            m_textOutputType->SetLabel( wxGetTranslation( jobTypeInfo.name ) );
            m_bitmapOutputType->SetBitmap( KiBitmapBundle( jobTypeInfo.bitmap ) );
        }
    }


    ~PANEL_JOB_OUTPUT()
    {
        m_buttonOutputOptions->Disconnect(
                wxEVT_MENU, wxCommandEventHandler( PANEL_JOB_OUTPUT::onMenu ), nullptr, this );
    }


    virtual void OnOutputRunClick( wxCommandEvent& event ) override
    {
        CallAfter(
                [this]()
                {
                    PROJECT&      project = m_frame->Kiway().Prj();
                    m_panelParent->EnsurePcbSchFramesOpen();

                    wxFileName fn = project.GetProjectFullName();
                    wxSetWorkingDirectory( fn.GetPath() );

                    JOBS_RUNNER jobRunner( &( m_frame->Kiway() ), m_jobsFile );

                    WX_PROGRESS_REPORTER* progressReporter =
                            new WX_PROGRESS_REPORTER( m_frame, _( "Running jobs" ), 1 );

                    jobRunner.RunJobsForOutput( m_output );

                    delete progressReporter;
                } );
    }

    virtual void OnOutputOptionsClick( wxCommandEvent& event ) override
    {
        wxMenu menu;
        menu.Append( wxID_EDIT, _( "Edit..." ) );
        menu.Append( wxID_DELETE, _( "Delete" ) );

        m_buttonOutputOptions->PopupMenu( &menu );
    }


private:
    void onMenu( wxCommandEvent& aEvent )
    {
        switch( aEvent.GetId() )
        {
            case wxID_EDIT:
            {
                DIALOG_JOB_OUTPUT dialog( m_frame, m_jobsFile, m_output );

                dialog.ShowModal();
            }
                break;

            case wxID_DELETE:
                m_panelParent->RemoveOutput( m_output );
                break;

            default:
                wxFAIL_MSG( wxT( "Unknown ID in context menu event" ) );
        }
    }

    JOBSET*           m_jobsFile;
    JOBSET_OUTPUT*    m_output;
    KICAD_MANAGER_FRAME* m_frame;
    PANEL_JOBS*          m_panelParent;
};


PANEL_JOBS::PANEL_JOBS( wxAuiNotebook* aParent, KICAD_MANAGER_FRAME* aFrame,
                        std::unique_ptr<JOBSET> aJobsFile ) :
	PANEL_JOBS_BASE( aParent ),
	m_parentBook( aParent ),
    m_frame( aFrame ),
	m_jobsFile( std::move( aJobsFile ) )
{
    int jobNoColId = m_jobList->AppendColumn( _( "No." ) );
    int jobDescColId = m_jobList->AppendColumn( _( "Job Description" ) );
    m_jobList->SetColumnWidth( jobNoColId, wxLIST_AUTOSIZE_USEHEADER );
    m_jobList->SetColumnWidth( jobDescColId, wxLIST_AUTOSIZE_USEHEADER );

    m_buttonAddJob->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_buttonUp->SetBitmap( KiBitmapBundle( BITMAPS::small_up ) );
    m_buttonDown->SetBitmap( KiBitmapBundle( BITMAPS::small_down ) );
    m_buttonOutputAdd->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );

    m_jobList->Connect( wxEVT_MENU, wxCommandEventHandler( PANEL_JOBS::onJobListMenu ),
                                nullptr, this );

    rebuildJobList();
    buildOutputList();

    m_buttonRunAllOutputs->Enable( !m_jobsFile->GetOutputs().empty() );
}


PANEL_JOBS::~PANEL_JOBS()
{
    m_jobList->Disconnect( wxEVT_MENU, wxCommandEventHandler( PANEL_JOBS::onJobListMenu ),
                            nullptr, this );
}


void PANEL_JOBS::RemoveOutput( JOBSET_OUTPUT* aOutput )
{
    auto it = m_outputPanelMap.find( aOutput );
    if( it != m_outputPanelMap.end() )
    {
        PANEL_JOB_OUTPUT* panel = m_outputPanelMap[aOutput];
        m_outputListSizer->Detach( panel );
        panel->Destroy();

        m_outputPanelMap.erase( it );

        // ensure the window contents get shifted as needed
        m_outputList->Layout();
        Layout();
    }

    m_jobsFile->RemoveOutput( aOutput );

    m_buttonRunAllOutputs->Enable( !m_jobsFile->GetOutputs().empty() );
}


void PANEL_JOBS::rebuildJobList()
{
    m_jobList->DeleteAllItems();

    int num = 1;
    for( auto& job : m_jobsFile->GetJobs() )
    {
        long itemIndex =
                m_jobList->InsertItem( m_jobList->GetItemCount(), wxString::Format( "%d", num++ ) );
        m_jobList->SetItem( itemIndex, 1, job.m_job->GetDescription() );
    }

    updateTitle();
}


void PANEL_JOBS::updateTitle()
{
    wxString tabName = m_jobsFile->GetFullName();
    if( m_jobsFile->GetDirty() )
    {
        tabName = wxS( "*" ) + tabName;
    }
    int pageIdx = m_parentBook->FindPage( this );
    m_parentBook->SetPageText( pageIdx, tabName );
}


void PANEL_JOBS::addJobOutputPanel( JOBSET_OUTPUT* aOutput )
{
    PANEL_JOB_OUTPUT* outputPanel =
            new PANEL_JOB_OUTPUT( m_outputList, this, m_frame, m_jobsFile.get(), aOutput );
    m_outputListSizer->Add( outputPanel, 0, wxEXPAND | wxALL, 5 );

    m_outputPanelMap[aOutput] = outputPanel;

    m_outputList->Layout();
}


void PANEL_JOBS::buildOutputList()
{
    Freeze();
    m_outputPanelMap.clear();

    for( auto& job : m_jobsFile->GetOutputs() )
    {
        addJobOutputPanel( &job );
    }

    // ensure the window contents get shifted as needed
    Layout();
    Thaw();
}


void PANEL_JOBS::openJobOptionsForListItem( size_t aItemIndex )
{
    JOBSET_JOB& job = m_jobsFile->GetJobs()[aItemIndex];

    KIWAY::FACE_T iface = JOB_REGISTRY::GetKifaceType( job.m_type );

    if( iface < KIWAY::KIWAY_FACE_COUNT )
    {
        EnsurePcbSchFramesOpen();

        m_frame->Kiway().ProcessJobConfigDialog( iface, job.m_job.get(), m_frame );
	}
    else
    {
        // special jobs
        if( job.m_job->GetType() == "special_execute" )
        {
            JOB_SPECIAL_EXECUTE* specialJob = static_cast<JOB_SPECIAL_EXECUTE*>( job.m_job.get() );

            DIALOG_SPECIAL_EXECUTE dialog( m_frame, specialJob );
            dialog.ShowModal();
        }
    }
}


void PANEL_JOBS::OnJobListDoubleClicked( wxListEvent& aEvent )
{
    long item = aEvent.GetIndex();

    openJobOptionsForListItem( item );
}


void PANEL_JOBS::OnJobListItemRightClick( wxListEvent& event )
{
    wxMenu menu;
    menu.Append( wxID_EDIT, _( "Edit..." ) );
    menu.Append( wxID_DELETE, _( "Delete" ) );

    m_jobList->PopupMenu( &menu, event.GetPoint() );
}


void PANEL_JOBS::onJobListMenu( wxCommandEvent& aEvent )
{
    switch( aEvent.GetId() )
    {
    case wxID_EDIT:
    {
        long item = m_jobList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );

        if( item == -1 )
            return;

        openJobOptionsForListItem( item );
    }
    break;

    case wxID_DELETE:
    {
        long item = m_jobList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );

        if( item == -1 )
            return;

        m_jobsFile->RemoveJob( item );
        rebuildJobList();
        break;
    }

    default: wxFAIL_MSG( wxT( "Unknown ID in context menu event" ) );
    }
}


void PANEL_JOBS::OnSaveButtonClick( wxCommandEvent& aEvent )
{
    m_jobsFile->SaveToFile();
    updateTitle();
}


void PANEL_JOBS::OnAddJobClick( wxCommandEvent& aEvent )
{
    wxArrayString              headers;
    std::vector<wxArrayString> items;

    headers.Add( _( "Job Types" ) );

    const JOB_REGISTRY::REGISTRY_MAP_T& jobMap =  JOB_REGISTRY::GetRegistry();

    for( const std::pair<const wxString, JOB_REGISTRY_ENTRY>& entry : jobMap )
    {
        wxArrayString item;
        item.Add( wxGetTranslation( entry.second.title ) );
        items.emplace_back( item );
    }

    EDA_LIST_DIALOG dlg( this, _( "Add New Job" ), headers, items );
    dlg.SetListLabel( _( "Select job type:" ) );

    if( dlg.ShowModal() == wxID_OK )
    {
        wxString selectedString = dlg.GetTextSelection();

        wxString jobKey;
        if( !selectedString.IsEmpty() )
        {
            for( const std::pair<const wxString, JOB_REGISTRY_ENTRY>& entry : jobMap )
            {
                if( wxGetTranslation( entry.second.title ) == selectedString )
                {
                    jobKey = entry.first;
                    break;
                }
            }
        }

        if( !jobKey.IsEmpty() )
        {
            JOB* job = JOB_REGISTRY::CreateInstance<JOB>( jobKey );
            m_jobsFile->AddNewJob( jobKey, job );

            rebuildJobList();
        }
    }
}


void PANEL_JOBS::OnAddOutputClick( wxCommandEvent& aEvent )
{
    wxArrayString              headers;
    std::vector<wxArrayString> items;

    headers.Add( _( "Job Types" ) );

    for( const std::pair<const JOBSET_OUTPUT_TYPE, JOB_TYPE_INFO>& jobType : jobTypeInfos )
    {
        wxArrayString item;
        item.Add( wxGetTranslation( jobType.second.name ) );
        items.emplace_back( item );
    }

    EDA_LIST_DIALOG dlg( this, _( "Add New Output" ), headers, items );
    dlg.SetListLabel( _( "Select output type:" ) );
    dlg.HideFilter();

    if( dlg.ShowModal() == wxID_OK )
    {
        wxString selectedString = dlg.GetTextSelection();

        for( const std::pair<const JOBSET_OUTPUT_TYPE, JOB_TYPE_INFO>& jobType : jobTypeInfos )
        {
            if( wxGetTranslation( jobType.second.name ) == selectedString )
            {
                JOBSET_OUTPUT output = m_jobsFile->AddNewJobOutput( jobType.first, nullptr );

                Freeze();
                addJobOutputPanel( &output );
                m_buttonRunAllOutputs->Enable( !m_jobsFile->GetOutputs().empty() );
                Thaw();
            }
        }
    }
}


bool PANEL_JOBS::GetCanClose()
{
    if( m_jobsFile->GetDirty() )
    {
        wxFileName fileName = m_jobsFile->GetFullFilename();
        wxString   msg = _( "Save changes to '%s' before closing?" );

        if( !HandleUnsavedChanges( this, wxString::Format( msg, fileName.GetFullName() ),
                                   [&]() -> bool
                                   {
                                        return m_jobsFile->SaveToFile();
                                   } ) )
        {
            return false;
        }
    }

    return true;
}


void PANEL_JOBS::EnsurePcbSchFramesOpen()
{
    PROJECT&      project = m_frame->Kiway().Prj();
    KIWAY_PLAYER* frame = m_frame->Kiway().Player( FRAME_PCB_EDITOR, false );

    if( !frame )
    {
        frame = m_frame->Kiway().Player( FRAME_PCB_EDITOR, true );

        // frame can be null if Cvpcb cannot be run. No need to show a warning
        // Kiway() generates the error messages
        if( !frame )
            return;

        wxFileName boardfn = project.GetProjectFullName();
        boardfn.SetExt( FILEEXT::PcbFileExtension );

        frame->OpenProjectFiles( std::vector<wxString>( 1, boardfn.GetFullPath() ) );
    }

    frame = m_frame->Kiway().Player( FRAME_SCH, false );

    if( !frame )
    {
        frame = m_frame->Kiway().Player( FRAME_SCH, true );

        // frame can be null if Cvpcb cannot be run. No need to show a warning
        // Kiway() generates the error messages
        if( !frame )
            return;

        wxFileName schFn = project.GetProjectFullName();
        schFn.SetExt( FILEEXT::KiCadSchematicFileExtension );

        frame->OpenProjectFiles( std::vector<wxString>( 1, schFn.GetFullPath() ) );
    }
}


void PANEL_JOBS::OnJobButtonUp( wxCommandEvent& aEvent )
{
    long item = m_jobList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );

    if( item == -1 )
        return;

    if( item == 0 )
        return;

    m_jobsFile->MoveJobUp( item );

    rebuildJobList();

    m_jobList->SetItemState( item - 1, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
}


void PANEL_JOBS::OnJobButtonDown( wxCommandEvent& aEvent )
{
    long item = m_jobList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );

    if( item == -1 )
        return;

    if( item == m_jobList->GetItemCount() - 1 )
        return;

    m_jobsFile->MoveJobDown( item );

    rebuildJobList();

    m_jobList->SetItemState( item + 1, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
}

void PANEL_JOBS::OnRunAllJobsClick( wxCommandEvent& event )
{
    // sanity
    if( m_jobsFile->GetOutputs().empty() )
	{
		DisplayError( this, _( "No outputs defined" ) );
		return;
	}

	CallAfter(
			[this]()
			{
				PROJECT&      project = m_frame->Kiway().Prj();
				EnsurePcbSchFramesOpen();

				wxFileName fn = project.GetProjectFullName();
				wxSetWorkingDirectory( fn.GetPath() );

                JOBS_RUNNER jobRunner( &( m_frame->Kiway() ), m_jobsFile.get() );

				WX_PROGRESS_REPORTER* progressReporter =
						new WX_PROGRESS_REPORTER( m_frame, _( "Running jobs" ), 1 );

				jobRunner.RunJobsAllOutputs();

				delete progressReporter;
			} );
}