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

#include "panel_jobset.h"
#include "dialog_jobset_output_options.h"
#include "dialog_copyfiles_job_settings.h"
#include <wx/aui/auibook.h>
#include <jobs/jobset.h>
#include <jobs/job_registry.h>
#include <eda_list_dialog.h>
#include <wx/checkbox.h>
#include <wx/menu.h>
#include <bitmaps.h>
#include <i18n_utility.h>
#include <jobs_runner.h>
#include <widgets/wx_progress_reporters.h>
#include <kicad_manager_frame.h>
#include <vector>

#include <wildcards_and_files_ext.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/wx_grid.h>
#include <widgets/grid_text_button_helpers.h>
#include <kiplatform/ui.h>
#include <confirm.h>

#include <jobs/job_special_execute.h>
#include <jobs/job_special_copyfiles.h>
#include <dialogs/dialog_executecommand_job_settings.h>


extern KICOMMON_API std::map<JOBSET_OUTPUT_TYPE, JOBSET_OUTPUT_TYPE_INFO> JobsetOutputTypeInfos;


class DIALOG_OUTPUT_RUN_RESULTS : public DIALOG_OUTPUT_RUN_RESULTS_BASE
{
public:
    DIALOG_OUTPUT_RUN_RESULTS( wxWindow* aParent, JOBSET* aJobsFile, JOBSET_OUTPUT* aOutput ) :
            DIALOG_OUTPUT_RUN_RESULTS_BASE( aParent ),
            m_jobsFile( aJobsFile ),
            m_output( aOutput )
    {
        m_staticTextOutputName->SetLabel( wxString::Format( _( "Output: %s" ),
                                                            aOutput->GetDescription() ) );

        int jobBmpColId = m_jobList->AppendColumn( wxT( "" ) );
        int jobNoColId = m_jobList->AppendColumn( _( "No." ) );
        int jobDescColId = m_jobList->AppendColumn( _( "Job Description" ) );
        int jobSourceColId = m_jobList->AppendColumn( wxT( "Source" ) );
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
                    imageIdx = 1;
                else
                    imageIdx = 0;
            }

            long itemIndex = m_jobList->InsertItem( m_jobList->GetItemCount(), imageIdx );

            m_jobList->SetItem( itemIndex, jobNoColId, wxString::Format( "%d", num++ ) );
            m_jobList->SetItem( itemIndex, jobDescColId, job.GetDescription() );

            KIWAY::FACE_T iface = JOB_REGISTRY::GetKifaceType( job.m_type );
            wxString source = wxEmptyString;
            if( iface < KIWAY::KIWAY_FACE_COUNT )
            {
                
                if( iface == KIWAY::FACE_PCB )
                {
                    source = wxT( "PCB" );
                }
                else if( iface == KIWAY::FACE_SCH )
				{
					source = wxT( "SCH" );
				}
            }
            m_jobList->SetItem( itemIndex, jobSourceColId, source );
        }

        SetupStandardButtons( { { wxID_OK, _( "Close" ) } } );
        finishDialogSettings();
    }

    void onJobListSize( wxSizeEvent& event ) override
    {
        int width = m_jobList->GetSize().x;
        width -= m_jobList->GetColumnWidth( 0 );
        width -= m_jobList->GetColumnWidth( 1 );

        m_jobList->SetColumnWidth( 2, width );
    }

    void OnJobListItemSelected( wxListEvent& event ) override
    {
        int itemIndex = event.GetIndex();

        // The index could be negative (it is default -1)
        if( itemIndex < 0 )
            return;

        std::vector<JOBSET_JOB> jobs = m_jobsFile->GetJobsForOutput( m_output );

        if( static_cast<size_t>( itemIndex ) < jobs.size() )
        {
            JOBSET_JOB& job = jobs[itemIndex];

            if( m_output->m_lastRunReporters.contains( job.m_id ) )
            {
                WX_STRING_REPORTER* reporter =
                        static_cast<WX_STRING_REPORTER*>( m_output->m_lastRunReporters[job.m_id] );

                if( reporter )
                    m_textCtrlOutput->SetValue( reporter->GetMessages() );
            }
            else
            {
                m_textCtrlOutput->SetValue( _( "No output messages" ) );
            }
        }
    }

private:
    JOBSET*        m_jobsFile;
    JOBSET_OUTPUT* m_output;
};


class PANEL_JOBSET_OUTPUT : public PANEL_JOBSET_OUTPUT_BASE
{
public:
    PANEL_JOBSET_OUTPUT( wxWindow* aParent, PANEL_JOBSET* aPanelParent, KICAD_MANAGER_FRAME* aFrame,
                         JOBSET* aFile, JOBSET_OUTPUT* aOutput ) :
            PANEL_JOBSET_OUTPUT_BASE( aParent ),
            m_jobsFile( aFile ),
            m_outputId( aOutput->m_id ),
            m_frame( aFrame ),
            m_panelParent( aPanelParent )
    {
        m_buttonProperties->SetBitmap( KiBitmapBundle( BITMAPS::config ) );
        m_buttonDelete->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );

#if  _WIN32
        // BORDER_RAISED/SUNKEN look pretty on every platform but Windows
        long style = GetWindowStyleFlag();
        style &= ~wxBORDER_MASK;
        style |= wxBORDER_SIMPLE;
        SetWindowStyleFlag( style );
#endif //  _WIN32

        Connect( wxEVT_MENU, wxCommandEventHandler( PANEL_JOBSET_OUTPUT::onMenu ), nullptr, this );

        if( JobsetOutputTypeInfos.contains( aOutput->m_type ) )
        {
            JOBSET_OUTPUT_TYPE_INFO& jobTypeInfo = JobsetOutputTypeInfos[aOutput->m_type];
            m_textOutputType->SetLabel( aOutput->GetDescription() );
            m_bitmapOutputType->SetBitmap( KiBitmapBundle( jobTypeInfo.bitmap ) );
        }

        UpdateStatus();
    }


    ~PANEL_JOBSET_OUTPUT()
    {
        Disconnect( wxEVT_MENU, wxCommandEventHandler( PANEL_JOBSET_OUTPUT::onMenu ), nullptr, this );
    }

    void ClearStatus()
    {
        JOBSET_OUTPUT* output = GetOutput();
        wxCHECK( output, /*void*/ );

        output->m_lastRunSuccess = std::nullopt;
        m_statusBitmap->SetBitmap( wxNullBitmap );
    }

    void UpdateStatus()
    {
        JOBSET_OUTPUT* output = GetOutput();
        wxCHECK( output, /*void*/ );

        if( output->m_lastRunSuccess.has_value() )
        {
            if( output->m_lastRunSuccess.value() )
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
            m_statusBitmap->SetBitmap( wxNullBitmap );
        }

        m_buttonGenerate->Enable( !m_jobsFile->GetJobsForOutput( output ).empty() );
    }

    virtual void OnGenerate( wxCommandEvent& event ) override
    {
        ClearStatus();
        Refresh();

        CallAfter(
                [this]()
                {
                    PROJECT& project = m_frame->Kiway().Prj();
                    m_panelParent->EnsurePcbSchFramesOpen();

                    wxFileName fn = project.GetProjectFullName();
                    wxSetWorkingDirectory( fn.GetPath() );

                    JOBS_RUNNER jobRunner( &( m_frame->Kiway() ), m_jobsFile, &project );

                    WX_PROGRESS_REPORTER* progressReporter =
                            new WX_PROGRESS_REPORTER( m_frame, _( "Running jobs" ), 1 );

                    if( JOBSET_OUTPUT* output = GetOutput() )
                        jobRunner.RunJobsForOutput( output );

                    UpdateStatus();

                    delete progressReporter;

                    // Bring the Kicad manager frame back to the front
                    m_frame->Raise();
                } );
    }

    virtual void OnLastStatusClick( wxMouseEvent& aEvent ) override
    {
        JOBSET_OUTPUT* output = GetOutput();
        wxCHECK( output, /*void*/ );

        DIALOG_OUTPUT_RUN_RESULTS dialog( m_frame, m_jobsFile, output );
        dialog.ShowModal();
    }

    void OnRightDown( wxMouseEvent& aEvent ) override
    {
        JOBSET_OUTPUT* output = GetOutput();
        wxCHECK( output, /*void*/ );

        wxMenu menu;
        menu.Append( wxID_EDIT, _( "Edit Output Options..." ) );
        menu.Append( wxID_DELETE, _( "Delete Output" ) );

        menu.AppendSeparator();
        menu.Append( wxID_VIEW_DETAILS, _( "View Last Run Results..." ) );

        menu.Enable( wxID_VIEW_DETAILS, output->m_lastRunSuccess.has_value() );

        PopupMenu( &menu );
    }

    void OnProperties( wxCommandEvent& aEvent ) override
    {
        JOBSET_OUTPUT* output = GetOutput();
        wxCHECK( output, /*void*/ );

        DIALOG_JOBSET_OUTPUT_OPTIONS dialog( m_frame, m_jobsFile, output );

        if( dialog.ShowModal() == wxID_OK )
        {
            m_textOutputType->SetLabel( output->GetDescription() );
            m_jobsFile->SetDirty();
            m_panelParent->UpdateTitle();
        }
    }

    virtual void OnDelete( wxCommandEvent& aEvent ) override
    {
        m_panelParent->RemoveOutput( this );
    }

    JOBSET_OUTPUT* GetOutput()
    {
        for( JOBSET_OUTPUT& jobset : m_jobsFile->GetOutputs() )
        {
            if( jobset.m_id == m_outputId )
                return &jobset;
        }

        return nullptr;
    }

private:
    void onMenu( wxCommandEvent& aEvent )
    {
        switch( aEvent.GetId() )
        {
            case wxID_EDIT:
            {
                wxCommandEvent dummy;
                OnProperties( dummy );
            }
                break;

            case wxID_DELETE:
            {
                wxCommandEvent dummy;
                OnDelete( dummy );
            }
                break;

            case wxID_VIEW_DETAILS:
            {
                wxMouseEvent dummy;
                OnLastStatusClick( dummy );
            }
                break;

            default:
                wxFAIL_MSG( wxT( "Unknown ID in context menu event" ) );
        }
    }

private:
    JOBSET*              m_jobsFile;
    wxString             m_outputId;
    KICAD_MANAGER_FRAME* m_frame;
    PANEL_JOBSET*        m_panelParent;
};


JOBS_GRID_TRICKS::JOBS_GRID_TRICKS( PANEL_JOBSET* aParent, WX_GRID* aGrid ) :
        GRID_TRICKS( aGrid ),
        m_parent( aParent )
{
    m_enableSingleClickEdit = false;
    m_multiCellEditEnabled = false;
}


void JOBS_GRID_TRICKS::showPopupMenu( wxMenu& menu, wxGridEvent& aEvent )
{
    wxArrayInt selectedRows = m_grid->GetSelectedRows();

    menu.Append( JOB_DESCRIPTION, _( "Edit Job Description" ) );
    menu.Append( JOB_PROPERTIES, _( "Edit Job Settings..." ) );
    menu.AppendSeparator();
    menu.Append( GRIDTRICKS_ID_COPY, _( "Copy" ) + "\tCtrl+C",
                 _( "Copy selected cells to clipboard" ) );
    menu.Append( GRIDTRICKS_ID_DELETE, _( "Delete" ) + "\tDel",
                 _( "Delete selected jobs" ) );
    menu.Append( GRIDTRICKS_ID_SELECT, _( "Select All" ) + "\tCtrl+A",
                 _( "Select all jobs" ) );

    menu.Enable( JOB_DESCRIPTION, selectedRows.size() == 1 );
    menu.Enable( JOB_PROPERTIES, selectedRows.size() == 1 );
    menu.Enable( GRIDTRICKS_ID_DELETE, selectedRows.size() > 0 );

    m_grid->PopupMenu( &menu );
}


void JOBS_GRID_TRICKS::doPopupSelection( wxCommandEvent& event )
{
    wxArrayInt selectedRows = m_grid->GetSelectedRows();

    if( event.GetId() == JOB_DESCRIPTION )
    {
        if( selectedRows.size() > 0 )
        {
            m_grid->SetGridCursor( selectedRows[0], 2 );
            m_grid->EnableCellEditControl();
        }
    }
    else if( event.GetId() == JOB_PROPERTIES )
    {
        if( selectedRows.size() > 0 )
            m_parent->OpenJobOptionsForListItem( selectedRows[0] );
    }
    else if( event.GetId() == GRIDTRICKS_ID_DELETE )
    {
        wxCommandEvent dummy;
        m_parent->OnJobButtonDelete( dummy );
    }
    else
    {
        GRID_TRICKS::doPopupSelection( event );
    }
}


bool JOBS_GRID_TRICKS::handleDoubleClick( wxGridEvent& aEvent )
{
    m_grid->CancelShowEditorOnMouseUp();

    int curr_col = aEvent.GetCol();
    int curr_row = aEvent.GetRow();

    if( ( curr_col == COL_NUMBER || curr_col == COL_SOURCE || curr_col == COL_DESCR )
        && curr_row >= 0 && curr_row < (int) m_parent->GetJobsFile()->GetJobs().size() )
    {
        m_doubleClickRow = curr_row;
        m_grid->CancelPendingChanges();

        CallAfter(
                [this]()
                {
                    // Yes, again.  CancelShowEditorOnMouseUp() doesn't appear to be 100%
                    // reliable.
                    m_grid->CancelPendingChanges();
                    int row = m_doubleClickRow;
                    m_doubleClickRow = -1;

                    if( row >= 0 && row < (int) m_parent->GetJobsFile()->GetJobs().size() )
                        m_parent->OpenJobOptionsForListItem( row );
                } );

        return true;
    }

    return false;
}


PANEL_JOBSET::PANEL_JOBSET( wxAuiNotebook* aParent, KICAD_MANAGER_FRAME* aFrame,
                            std::unique_ptr<JOBSET> aJobsFile ) :
        PANEL_JOBSET_BASE( aParent ),
        m_parentBook( aParent ),
        m_frame( aFrame ),
        m_jobsFile( std::move( aJobsFile ) )
{
    m_jobsGrid->PushEventHandler( new JOBS_GRID_TRICKS( this, m_jobsGrid ) );

    m_jobsGrid->SetDefaultRowSize( m_jobsGrid->GetDefaultRowSize() + 4 );
    m_jobsGrid->OverrideMinSize( 0.6, 0.3 );
    m_jobsGrid->SetSelectionMode( wxGrid::wxGridSelectRows );

    // 'm' for margins
    m_jobsGrid->SetColSize( COL_NUMBER, GetTextExtent( wxT( "99m" ) ).x );
    m_jobsGrid->SetColSize( COL_SOURCE, GetTextExtent( wxT( "PCBm" ) ).x );

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


PANEL_JOBSET::~PANEL_JOBSET()
{
    // Delete the GRID_TRICKS.
    m_jobsGrid->PopEventHandler( true );
}


void PANEL_JOBSET::RemoveOutput( PANEL_JOBSET_OUTPUT* aPanel )
{
    JOBSET_OUTPUT* output = aPanel->GetOutput();

    m_outputListSizer->Detach( aPanel );
    aPanel->Destroy();

    // ensure the window contents get shifted as needed
    m_outputList->Layout();
    Layout();

    m_jobsFile->RemoveOutput( output );

    m_buttonRunAllOutputs->Enable( !m_jobsFile->GetOutputs().empty() );
}


void PANEL_JOBSET::rebuildJobList()
{
    if( m_jobsGrid->GetNumberRows() )
        m_jobsGrid->DeleteRows( 0, m_jobsGrid->GetNumberRows() );

    m_jobsGrid->AppendRows( m_jobsFile->GetJobs().size() );

    int num = 1;

    for( JOBSET_JOB& job : m_jobsFile->GetJobs() )
    {
        m_jobsGrid->SetCellValue( num - 1, COL_NUMBER, wxString::Format( "%d", num ) );
        m_jobsGrid->SetReadOnly( num - 1, COL_NUMBER );

        m_jobsGrid->SetCellValue( num - 1, COL_DESCR, job.GetDescription() );

        m_jobsGrid->SetReadOnly( num - 1, COL_SOURCE );

        KIWAY::FACE_T iface = JOB_REGISTRY::GetKifaceType( job.m_type );
        wxString      source = wxEmptyString;
        if( iface < KIWAY::KIWAY_FACE_COUNT )
        {
            if( iface == KIWAY::FACE_PCB )
            {
                source = wxT( "PCB" );
            }
            else if( iface == KIWAY::FACE_SCH )
            {
                source = wxT( "SCH" );
            }
        }

        m_jobsGrid->SetCellValue( num - 1, COL_SOURCE, source );

        num++;
    }

    UpdateTitle();

    // Ensure the outputs get their Run-ability status updated
    for( PANEL_JOBSET_OUTPUT* panel : GetOutputPanels() )
        panel->UpdateStatus();
}


void PANEL_JOBSET::UpdateTitle()
{
    wxString tabName = m_jobsFile->GetFullName();

    if( m_jobsFile->GetDirty() )
        tabName = wxS( "*" ) + tabName;

    int pageIdx = m_parentBook->FindPage( this );
    m_parentBook->SetPageText( pageIdx, tabName );
}


void PANEL_JOBSET::addJobOutputPanel( JOBSET_OUTPUT* aOutput )
{
    PANEL_JOBSET_OUTPUT* outputPanel = new PANEL_JOBSET_OUTPUT( m_outputList, this, m_frame,
                                                                m_jobsFile.get(), aOutput );

#if __OSX__
    m_outputListSizer->Add( outputPanel, 0, wxEXPAND, 5 );
#else
    m_outputListSizer->Add( outputPanel, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 5 );
#endif

    m_outputList->Layout();
}


std::vector<PANEL_JOBSET_OUTPUT*> PANEL_JOBSET::GetOutputPanels()
{
    std::vector<PANEL_JOBSET_OUTPUT*> panels;

    for( const wxSizerItem* item : m_outputListSizer->GetChildren() )
    {
        if( PANEL_JOBSET_OUTPUT* panel = dynamic_cast<PANEL_JOBSET_OUTPUT*>( item->GetWindow() ) )
            panels.push_back( panel );
    }

    return panels;
}


void PANEL_JOBSET::buildOutputList()
{
    Freeze();

    for( JOBSET_OUTPUT& job : m_jobsFile->GetOutputs() )
        addJobOutputPanel( &job );

    // ensure the window contents get shifted as needed
    Layout();
    Thaw();
}


bool PANEL_JOBSET::OpenJobOptionsForListItem( size_t aItemIndex )
{
    bool          success = false;
    JOBSET_JOB&   job = m_jobsFile->GetJobs()[aItemIndex];
    KIWAY::FACE_T iface = JOB_REGISTRY::GetKifaceType( job.m_type );

    if( iface < KIWAY::KIWAY_FACE_COUNT )
    {
        EnsurePcbSchFramesOpen();
        success = m_frame->Kiway().ProcessJobConfigDialog( iface, job.m_job.get(), m_frame );
    }
    else
    {
        // special jobs
        if( job.m_job->GetType() == "special_execute" )
        {
            JOB_SPECIAL_EXECUTE* specialJob = static_cast<JOB_SPECIAL_EXECUTE*>( job.m_job.get() );

            DIALOG_EXECUTECOMMAND_JOB_SETTINGS dialog( m_frame, specialJob );

            if( dialog.ShowModal() == wxID_OK )
                success = true;
        }
        else if( job.m_job->GetType() == "special_copyfiles" )
        {
            JOB_SPECIAL_COPYFILES* specialJob =
                    static_cast<JOB_SPECIAL_COPYFILES*>( job.m_job.get() );
            DIALOG_COPYFILES_JOB_SETTINGS dialog( m_frame, specialJob );

            if( dialog.ShowModal() == wxID_OK )
                success = true;
        }
    }

    if( success )
    {
        m_jobsFile->SetDirty();
        UpdateTitle();
    }

    // Bring the Kicad manager frame back to the front
    m_frame->Raise();

    return success;
}


void PANEL_JOBSET::OnGridCellChange( wxGridEvent& aEvent )
{
    int row = aEvent.GetRow();
    int col = aEvent.GetCol();

    if( col == COL_DESCR )
        m_jobsFile->GetJobs()[row].SetDescription( m_jobsGrid->GetCellValue( row, col ) );
}


void PANEL_JOBSET::OnSaveButtonClick( wxCommandEvent& aEvent )
{
    if( !m_jobsGrid->CommitPendingChanges() )
        return;

    m_jobsFile->SaveToFile( wxEmptyString, true );
    UpdateTitle();
}


void PANEL_JOBSET::OnAddJobClick( wxCommandEvent& aEvent )
{
    if( !m_jobsGrid->CommitPendingChanges() )
        return;

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
            int  row = m_jobsFile->GetJobs().size();
            bool wasDirty = m_jobsFile->GetDirty();
            JOB* job = JOB_REGISTRY::CreateInstance<JOB>( jobKey );

            m_jobsFile->AddNewJob( jobKey, job );

            if( OpenJobOptionsForListItem( row ) )
            {
                rebuildJobList();

                m_jobsGrid->SetGridCursor( row, 2 );
                m_jobsGrid->EnableCellEditControl();
            }
            else
            {
                m_jobsFile->RemoveJob( row );
                m_jobsFile->SetDirty( wasDirty );
            }
        }
    }
}


void PANEL_JOBSET::OnJobButtonDelete( wxCommandEvent& aEvent )
{
    if( !m_jobsGrid->CommitPendingChanges() )
        return;

    wxArrayInt selectedRows = m_jobsGrid->GetSelectedRows();

    if( selectedRows.empty() )
        return;

    m_jobsGrid->CommitPendingChanges( true /* quiet mode */ );
    m_jobsGrid->ClearSelection();

    // Reverse sort so deleting a row doesn't change the indexes of the other rows.
    selectedRows.Sort( []( int* first, int* second ) { return *second - *first; } );

    int select = selectedRows[0];

    for( int row : selectedRows )
        m_jobsFile->RemoveJob( row );

    rebuildJobList();

    if( m_jobsGrid->GetNumberRows() )
    {
        m_jobsGrid->MakeCellVisible( std::max( 0, select-1 ), m_jobsGrid->GetGridCursorCol() );
        m_jobsGrid->SetGridCursor( std::max( 0, select-1 ), m_jobsGrid->GetGridCursorCol() );
    }
}


void PANEL_JOBSET::OnAddOutputClick( wxCommandEvent& aEvent )
{
    wxArrayString              headers;
    std::vector<wxArrayString> items;

    headers.Add( _( "Output Types" ) );

    for( const std::pair<const JOBSET_OUTPUT_TYPE, JOBSET_OUTPUT_TYPE_INFO>& outputType : JobsetOutputTypeInfos )
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

        for( const std::pair<const JOBSET_OUTPUT_TYPE, JOBSET_OUTPUT_TYPE_INFO>& jobType : JobsetOutputTypeInfos )
        {
            if( wxGetTranslation( jobType.second.name ) == selectedString )
            {
                JOBSET_OUTPUT* output = m_jobsFile->AddNewJobOutput( jobType.first );

                DIALOG_JOBSET_OUTPUT_OPTIONS dialog( m_frame, m_jobsFile.get(), output );
                if (dialog.ShowModal() == wxID_OK)
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


bool PANEL_JOBSET::GetCanClose()
{
    if( m_jobsFile->GetDirty() )
    {
        wxFileName fileName = m_jobsFile->GetFullFilename();
        wxString   msg = _( "Save changes to '%s' before closing?" );

        if( !HandleUnsavedChanges( this, wxString::Format( msg, fileName.GetFullName() ),
                                   [&]() -> bool
                                   {
                                       return m_jobsFile->SaveToFile(wxEmptyString, true);
                                   } ) )
        {
            return false;
        }
    }

    return true;
}


void PANEL_JOBSET::EnsurePcbSchFramesOpen()
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

        if( !frame->IsVisible() )
            frame->Show( true );
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

        if( !frame->IsVisible() )
            frame->Show( true );
    }

    SetFocus();
}


wxString PANEL_JOBSET::GetFilePath() const
{
    return m_jobsFile->GetFullFilename();
}


void PANEL_JOBSET::OnJobButtonUp( wxCommandEvent& aEvent )
{
    if( !m_jobsGrid->CommitPendingChanges() )
        return;

    int item = m_jobsGrid->GetGridCursorRow();

    if( item > 0 )
    {
        m_jobsFile->MoveJobUp( item );

        rebuildJobList();

        m_jobsGrid->SelectRow( item - 1 );
        m_jobsGrid->SetGridCursor( item - 1, m_jobsGrid->GetGridCursorCol() );
    }
    else
    {
        wxBell();
    }
}


void PANEL_JOBSET::OnJobButtonDown( wxCommandEvent& aEvent )
{
    if( !m_jobsGrid->CommitPendingChanges() )
        return;

    int item = m_jobsGrid->GetGridCursorRow();

    if( item < m_jobsGrid->GetNumberRows() - 1 )
    {
        m_jobsFile->MoveJobDown( item );

        rebuildJobList();

        m_jobsGrid->SelectRow( item + 1 );
        m_jobsGrid->SetGridCursor( item + 1, m_jobsGrid->GetGridCursorCol() );
    }
    else
    {
        wxBell();
    }
}


void PANEL_JOBSET::OnGenerateAllOutputsClick( wxCommandEvent& event )
{
    if( !m_jobsGrid->CommitPendingChanges() )
        return;

    // sanity
    if( m_jobsFile->GetOutputs().empty() )
	{
		DisplayError( this, _( "No outputs defined" ) );
		return;
	}

    for( PANEL_JOBSET_OUTPUT* panel : GetOutputPanels() )
        panel->ClearStatus();

    Refresh();

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

                for( PANEL_JOBSET_OUTPUT* panel : GetOutputPanels() )
                    panel->UpdateStatus();

				delete progressReporter;

                // Bring the Kicad manager frame back to the front
                m_frame->Raise();
			} );
}


void PANEL_JOBSET::OnSizeGrid( wxSizeEvent& aEvent )
{
    m_jobsGrid->SetColSize( COL_DESCR, m_jobsGrid->GetSize().x
                                               - m_jobsGrid->GetColSize( COL_SOURCE )
                                               - m_jobsGrid->GetColSize( COL_NUMBER ) );

    // Always propagate for a grid repaint (needed if the height changes, as well as width)
    aEvent.Skip();
}

