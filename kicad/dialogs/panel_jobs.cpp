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
#include <wx/combo.h>
#include <wildcards_and_files_ext.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/wx_grid.h>
#include <widgets/grid_text_button_helpers.h>
#include <kiplatform/ui.h>
#include <confirm.h>

#include <jobs/job_special_execute.h>
#include <dialogs/dialog_special_execute.h>
#include <wx/textdlg.h>


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

        SetTitle( wxString::Format( _( "%s Output Options" ), m_output->m_outputHandler->GetDefaultDescription() ) );

        if( m_output->m_type != JOBSET_OUTPUT_TYPE::ARCHIVE )
        {
            m_textArchiveFormat->Hide();
            m_choiceArchiveformat->Hide();
        }

        m_textCtrlOutputPath->SetValue( m_output->m_outputHandler->GetOutputPath() );
        m_buttonOutputPath->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );
        m_textCtrlDescription->SetValue( m_output->GetDescription() );
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

            wxDirDialog dirDialog( this, _( "Select output directory" ), currPath,
                                   wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST );

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
            {
                return;
            }

            m_textCtrlOutputPath->SetValue( dlg.GetPath() );
        }

    }

    bool TransferDataFromWindow() override
    {
        wxString outputPath = m_textCtrlOutputPath->GetValue().Trim().Trim( false );
        if( outputPath == wxEmptyString )
        {
            DisplayErrorMessage( this, _( "Output path cannot be empty" ) );
            return false;
        }

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

        m_output->m_outputHandler->SetOutputPath( outputPath );

        if( m_output->m_type == JOBSET_OUTPUT_TYPE::ARCHIVE )
        {
            JOBS_OUTPUT_ARCHIVE* archive =
                    static_cast<JOBS_OUTPUT_ARCHIVE*>( m_output->m_outputHandler );

            archive->SetFormat( JOBS_OUTPUT_ARCHIVE::FORMAT::ZIP );
        }

        m_output->SetDescription( m_textCtrlDescription->GetValue() );

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
            arrayStr.Add( job.GetDescription() );

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

        if( arrayStr.size() != 0 )
        {
            m_listBoxOnly->InsertItems( arrayStr, 0 );

            for( int idx : selectedList )
            {
                m_listBoxOnly->SetSelection( idx );
            }
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


class DIALOG_OUTPUT_RUN_RESULTS : public DIALOG_OUTPUT_RUN_RESULTS_BASE
{
public:
    DIALOG_OUTPUT_RUN_RESULTS( wxWindow* aParent,
                                JOBSET* aJobsFile,
                                JOBSET_OUTPUT* aOutput ) :
        DIALOG_OUTPUT_RUN_RESULTS_BASE( aParent ),
        m_jobsFile( aJobsFile ),
        m_output( aOutput )
    {
        m_staticTextOutputName->SetLabel( aOutput->m_outputHandler->GetOutputPath() );

        int jobBmpColId = m_jobList->AppendColumn( wxT( "" ) );
        int jobNoColId = m_jobList->AppendColumn( _( "No." ) );
        int jobDescColId = m_jobList->AppendColumn( _( "Job Description" ) );
        m_jobList->SetColumnWidth( jobBmpColId, wxLIST_AUTOSIZE_USEHEADER );
        m_jobList->SetColumnWidth( jobNoColId, wxLIST_AUTOSIZE_USEHEADER );
        m_jobList->SetColumnWidth( jobDescColId, wxLIST_AUTOSIZE_USEHEADER );

        wxImageList* imageList = new wxImageList( 16, 16, true, 3 );
        imageList->Add( KiBitmapBundle( BITMAPS::ercerr ).GetBitmap( wxSize( 16, 16 ) ) );
        imageList->Add( KiBitmapBundle( BITMAPS::checked_ok ).GetBitmap( wxSize( 16, 16 ) ) );
        m_jobList->SetImageList( imageList, wxIMAGE_LIST_SMALL );

        int num = 1;
        for( auto& job : aJobsFile->GetJobsForOutput( aOutput ) )
        {
            int imageIdx = -1;
            if( aOutput->m_lastRunSuccessMap.contains( job.m_id ) )
            {
                if( aOutput->m_lastRunSuccessMap[job.m_id].value() )
                {
                    imageIdx = 1;
                }
                else
                {
                    imageIdx = 0;
                }
            }

            long itemIndex = m_jobList->InsertItem( m_jobList->GetItemCount(), imageIdx );

            m_jobList->SetItem( itemIndex, jobNoColId, wxString::Format( "%d", num++ ) );
            m_jobList->SetItem( itemIndex, jobDescColId, job.GetDescription() );
        }
    }


    virtual void OnJobListItemSelected( wxListEvent& event ) override
    {
        int itemIndex = event.GetIndex();

        std::vector<JOBSET_JOB> jobs = m_jobsFile->GetJobsForOutput( m_output );

        if( itemIndex < jobs.size() )
        {
            JOBSET_JOB& job = jobs[itemIndex];
            if( m_output->m_lastRunReporters.contains( job.m_id ) )
            {
                WX_STRING_REPORTER* reporter =
                        static_cast<WX_STRING_REPORTER*>( m_output->m_lastRunReporters[job.m_id] );
                m_textCtrlOutput->SetValue( reporter->GetMessages() );
            }
            else
            {
                m_textCtrlOutput->SetValue( _( "No output available" ) );
            }
        }

    }

private:
    JOBSET*        m_jobsFile;
    JOBSET_OUTPUT* m_output;
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
            m_textOutputType->SetLabel( aOutput->GetDescription() );
            m_bitmapOutputType->SetBitmap( KiBitmapBundle( jobTypeInfo.bitmap ) );
        }

        UpdateStatus();
    }


    ~PANEL_JOB_OUTPUT()
    {
        m_buttonOutputOptions->Disconnect(
                wxEVT_MENU, wxCommandEventHandler( PANEL_JOB_OUTPUT::onMenu ), nullptr, this );
    }

    void UpdateStatus()
    {
        if( m_output->m_lastRunSuccess.has_value() )
        {
            if( m_output->m_lastRunSuccess.value() )
            {
                m_statusBitmap->SetBitmap( KiBitmapBundle( BITMAPS::checked_ok ) );
                m_statusBitmap->Show();
                m_statusBitmap->SetToolTip( _( "Last run successful" ) );
            }
            else
            {
                m_statusBitmap->SetBitmap( KiBitmapBundle( BITMAPS::ercerr ) );
                m_statusBitmap->Show();
                m_statusBitmap->SetToolTip( _( "Last run failed" ) );
            }
        }
        else
        {
            m_statusBitmap->Hide();
        }

        if( !m_jobsFile->GetJobsForOutput( m_output ).empty() )
        {
            m_buttonOutputRun->Enable();
        }
        else
        {
            m_buttonOutputRun->Disable();
        }
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

                    JOBS_RUNNER jobRunner( &( m_frame->Kiway() ), m_jobsFile, &project );

                    WX_PROGRESS_REPORTER* progressReporter =
                            new WX_PROGRESS_REPORTER( m_frame, _( "Running jobs" ), 1 );

                    jobRunner.RunJobsForOutput( m_output );
                    UpdateStatus();

                    delete progressReporter;
                } );
    }

    virtual void OnLastStatusClick(wxMouseEvent& event) override
    {
        DIALOG_OUTPUT_RUN_RESULTS dialog( m_frame, m_jobsFile, m_output );
        dialog.ShowModal();
    }

    virtual void OnOutputOptionsClick( wxCommandEvent& event ) override
    {
        wxMenu menu;
        menu.Append( wxID_EDIT, _( "Edit..." ) );
        menu.Append( wxID_DELETE, _( "Delete" ) );

        if( m_output->m_lastRunSuccess.has_value() )
        {
            menu.AppendSeparator();
            menu.Append( wxID_VIEW_DETAILS, _( "View last run results..." ) );
        }

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
                if( dialog.ShowModal() == wxID_SAVE )
                {
                    m_textOutputType->SetLabel( m_output->GetDescription() );
                    m_jobsFile->SetDirty();
                    m_panelParent->UpdateTitle();
                }
            }
                break;

            case wxID_DELETE:
                m_panelParent->RemoveOutput( m_output );
                break;

            case wxID_VIEW_DETAILS:
            {
                DIALOG_OUTPUT_RUN_RESULTS dialog( m_frame, m_jobsFile, m_output );
                dialog.ShowModal();
            }
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


/* ---------- support for jobs grid ---------- */

JOBS_GRID_TRICKS::JOBS_GRID_TRICKS( PANEL_JOBS* aParent, WX_GRID* aGrid ) :
        GRID_TRICKS( aGrid ),
        m_parent( aParent )
{
    m_multiCellEditEnabled = false;
}


class TEXT_BUTTON_JOB_PROPERTIES : public wxComboCtrl
{
public:
    TEXT_BUTTON_JOB_PROPERTIES( wxWindow* aParent, PANEL_JOBS* aController, WX_GRID* aGrid ) :
            wxComboCtrl( aParent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                         wxTE_PROCESS_ENTER | wxBORDER_NONE ),
            m_controller( aController ),
            m_grid( aGrid )
    {
        SetButtonBitmaps( KiBitmapBundle( BITMAPS::edit ) );

        // win32 fix, avoids drawing the "native dropdown caret"
        Customize( wxCC_IFLAG_HAS_NONSTANDARD_BUTTON );
    }

protected:
    void DoSetPopupControl( wxComboPopup* popup ) override
    {
        m_popup = nullptr;
    }

    void OnButtonClick() override
    {
        int row = m_grid->GetGridCursorRow();

        if( row >= 0 && row < (int) m_controller->GetJobsFile()->GetJobs().size() )
            m_controller->OpenJobOptionsForListItem( m_grid->GetGridCursorRow() );
    }

    PANEL_JOBS* m_controller;
    WX_GRID*    m_grid;
};


class GRID_CELL_JOB_PROPERTIES_EDITOR : public GRID_CELL_TEXT_BUTTON
{
public:
    GRID_CELL_JOB_PROPERTIES_EDITOR( PANEL_JOBS* aController, WX_GRID* aGrid ) :
            m_controller( aController ),
            m_grid( aGrid )
    { }

    wxGridCellEditor* Clone() const override
    {
        return new GRID_CELL_JOB_PROPERTIES_EDITOR( m_controller, m_grid );
    }

    void Create( wxWindow* aParent, wxWindowID aId, wxEvtHandler* aEventHandler ) override
    {
        m_control = new TEXT_BUTTON_JOB_PROPERTIES( aParent, m_controller, m_grid );
        WX_GRID::CellEditorSetMargins( Combo() );

        wxGridCellEditor::Create( aParent, aId, aEventHandler );
    }

protected:
    PANEL_JOBS* m_controller;
    WX_GRID*    m_grid;
};

/* ---------- end of support for jobs grid ---------- */


PANEL_JOBS::PANEL_JOBS( wxAuiNotebook* aParent, KICAD_MANAGER_FRAME* aFrame,
                        std::unique_ptr<JOBSET> aJobsFile ) :
	PANEL_JOBS_BASE( aParent ),
	m_parentBook( aParent ),
    m_frame( aFrame ),
	m_jobsFile( std::move( aJobsFile ) )
{
    m_jobsGrid->PushEventHandler( new JOBS_GRID_TRICKS( this, m_jobsGrid ) );

    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetEditor( new GRID_CELL_JOB_PROPERTIES_EDITOR( this, m_jobsGrid ) );
    m_jobsGrid->SetColAttr( 1, attr );

    m_buttonAddJob->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_buttonUp->SetBitmap( KiBitmapBundle( BITMAPS::small_up ) );
    m_buttonDown->SetBitmap( KiBitmapBundle( BITMAPS::small_down ) );
    m_buttonDelete->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_buttonOutputAdd->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );

    rebuildJobList();
    buildOutputList();

    m_buttonRunAllOutputs->Enable( !m_jobsFile->GetOutputs().empty()
                                        && !m_jobsFile->GetJobs().empty() );
}


PANEL_JOBS::~PANEL_JOBS()
{
    // Delete the GRID_TRICKS.
    m_jobsGrid->PopEventHandler( true );
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
    if( m_jobsGrid->GetNumberRows() )
        m_jobsGrid->DeleteRows( 0, m_jobsGrid->GetNumberRows() );

    m_jobsGrid->AppendRows( m_jobsFile->GetJobs().size() );

    int num = 1;

    for( JOBSET_JOB& job : m_jobsFile->GetJobs() )
    {
        m_jobsGrid->SetCellValue( num - 1, 0, wxString::Format( "%d", num ) );
        m_jobsGrid->SetReadOnly( num - 1, 0 );

        m_jobsGrid->SetCellValue( num - 1, 1, job.GetDescription() );

        num++;
    }

    UpdateTitle();

    if( m_jobsFile->GetJobs().empty() )
    {
        m_buttonUp->Disable();
        m_buttonDown->Disable();
        m_buttonRunAllOutputs->Disable();
    }
    else
    {
        m_buttonUp->Enable();
        m_buttonDown->Enable();
        m_buttonRunAllOutputs->Enable();
    }

    // Ensure the outputs get their Run-ability status updated
    for( auto& output : m_jobsFile->GetOutputs() )
    {
        if( m_outputPanelMap.contains( &output ) )
        {
            PANEL_JOB_OUTPUT* panel = m_outputPanelMap[&output];
            panel->UpdateStatus();
        }
    }
}


void PANEL_JOBS::UpdateTitle()
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
    PANEL_JOB_OUTPUT* outputPanel = new PANEL_JOB_OUTPUT( m_outputList, this, m_frame,
                                                          m_jobsFile.get(), aOutput );
    m_outputListSizer->Add( outputPanel, 0, wxEXPAND, 5 );

    m_outputPanelMap[aOutput] = outputPanel;

    m_outputList->Layout();
}


void PANEL_JOBS::buildOutputList()
{
    Freeze();
    m_outputPanelMap.clear();

    for( JOBSET_OUTPUT& job : m_jobsFile->GetOutputs() )
    {
        addJobOutputPanel( &job );
    }

    // ensure the window contents get shifted as needed
    Layout();
    Thaw();
}


void PANEL_JOBS::OpenJobOptionsForListItem( size_t aItemIndex )
{
    JOBSET_JOB& job = m_jobsFile->GetJobs()[aItemIndex];

    KIWAY::FACE_T iface = JOB_REGISTRY::GetKifaceType( job.m_type );

    if( iface < KIWAY::KIWAY_FACE_COUNT )
    {
        EnsurePcbSchFramesOpen();

        bool changes = m_frame->Kiway().ProcessJobConfigDialog( iface, job.m_job.get(), m_frame );
        if( changes )
        {
            m_jobsFile->SetDirty();
            UpdateTitle();
        }
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


void PANEL_JOBS::OnSaveButtonClick( wxCommandEvent& aEvent )
{
    m_jobsFile->SaveToFile( wxEmptyString, true );
    UpdateTitle();
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

    headers.Add( _( "Output Types" ) );

    for( const std::pair<const JOBSET_OUTPUT_TYPE, JOB_TYPE_INFO>& outputType : jobTypeInfos )
    {
        wxArrayString item;
        item.Add( wxGetTranslation( outputType.second.name ) );
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
                JOBSET_OUTPUT* output = m_jobsFile->AddNewJobOutput( jobType.first );

                DIALOG_JOB_OUTPUT dialog( m_frame, m_jobsFile.get(), output );
                if (dialog.ShowModal() == wxID_SAVE)
                {
                    Freeze();
                    addJobOutputPanel( output );
                    m_buttonRunAllOutputs->Enable( !m_jobsFile->GetOutputs().empty() );
                    Thaw();
                }
                else
                {
                    // canceled
                    m_jobsFile->RemoveOutput( output );
                }
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

        // Prevent our window from being closed during the open process
        wxEventBlocker blocker( this );

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

        wxEventBlocker blocker( this );

        frame->OpenProjectFiles( std::vector<wxString>( 1, schFn.GetFullPath() ) );
    }
}


wxString PANEL_JOBS::GetFilePath() const
{
    return m_jobsFile->GetFullFilename();
}


void PANEL_JOBS::OnJobButtonUp( wxCommandEvent& aEvent )
{
    if( !m_jobsGrid->CommitPendingChanges() )
        return;

    int item = m_jobsGrid->GetGridCursorRow();

    if( item > 0 )
    {
        m_jobsFile->MoveJobUp( item );

        rebuildJobList();

        m_jobsGrid->SelectRow( item - 1 );
    }
    else
    {
        wxBell();
    }
}


void PANEL_JOBS::OnJobButtonDown( wxCommandEvent& aEvent )
{
    if( !m_jobsGrid->CommitPendingChanges() )
        return;

    int item = m_jobsGrid->GetGridCursorRow();

    if( item < m_jobsGrid->GetNumberRows() - 1 )
    {
        m_jobsFile->MoveJobDown( item );

        rebuildJobList();

        m_jobsGrid->SelectRow( item + 1 );
    }
    else
    {
        wxBell();
    }
}


void PANEL_JOBS::OnJobButtonDelete( wxCommandEvent& aEvent )
{
    wxArrayInt selectedRows = m_jobsGrid->GetSelectedRows();

    if( selectedRows.empty() && m_jobsGrid->GetGridCursorRow() >= 0 )
        selectedRows.push_back( m_jobsGrid->GetGridCursorRow() );

    if( selectedRows.empty() )
        return;

    m_jobsGrid->CommitPendingChanges( true /* quiet mode */ );
    m_jobsGrid->ClearSelection();

    // Reverse sort so deleting a row doesn't change the indexes of the other rows.
    selectedRows.Sort( []( int* first, int* second ) { return *second - *first; } );

    for( int row : selectedRows )
        m_jobsFile->RemoveJob( row );

    rebuildJobList();
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

                JOBS_RUNNER jobRunner( &( m_frame->Kiway() ), m_jobsFile.get(), &project );

				WX_PROGRESS_REPORTER* progressReporter =
						new WX_PROGRESS_REPORTER( m_frame, _( "Running jobs" ), 1 );

				jobRunner.RunJobsAllOutputs();

                for( auto& output : m_jobsFile->GetOutputs() )
                {
                    PANEL_JOB_OUTPUT* panel = m_outputPanelMap[&output];
                    panel->UpdateStatus();
                }

				delete progressReporter;
			} );
}


void PANEL_JOBS::adjustGridColumns()
{
    // Account for scroll bars
    int width = KIPLATFORM::UI::GetUnobscuredSize( m_jobsGrid ).x;

    m_jobsGrid->SetColSize( 1, width - m_jobsGrid->GetColSize( 0 ) );
}


void PANEL_JOBS::OnSizeGrid( wxSizeEvent& aEvent )
{
    adjustGridColumns();

    // Always propagate for a grid repaint (needed if the height changes, as well as width)
    aEvent.Skip();
}

