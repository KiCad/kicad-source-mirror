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
#include "dialog_destination.h"
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
#include <wx/dcclient.h>

#include <wildcards_and_files_ext.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/wx_grid.h>
#include <widgets/grid_text_button_helpers.h>
#include <kiplatform/ui.h>
#include <confirm.h>
#include <launch_ext.h>
#include <wx/filename.h>

#include <jobs/job_special_execute.h>
#include <jobs/job_special_copyfiles.h>
#include <dialogs/dialog_executecommand_job_settings.h>
#include <common.h>


extern KICOMMON_API
std::map<JOBSET_DESTINATION_T, JOBSET_DESTINATION_T_INFO> JobsetDestinationTypeInfos;


class DIALOG_JOBSET_RUN_LOG : public DIALOG_JOBSET_RUN_LOG_BASE
{
public:
    DIALOG_JOBSET_RUN_LOG( wxWindow* aParent, JOBSET* aJobsFile, JOBSET_DESTINATION* aDestination ) :
            DIALOG_JOBSET_RUN_LOG_BASE( aParent ),
            m_jobsFile( aJobsFile ),
            m_destination( aDestination ),
            m_lastWidth( -1 ),
            m_marginsWidth( -1 )
    {
        m_staticTextOutputName->SetLabel( wxString::Format( _( "Destination: %s" ),
                                                            aDestination->GetDescription() ) );

        int jobBmpColId = m_jobList->AppendColumn( wxT( "" ) );
        int jobNoColId = m_jobList->AppendColumn( _( "No." ) );
        int jobDescColId = m_jobList->AppendColumn( _( "Job Description" ) );
        int jobSourceColId = m_jobList->AppendColumn( _( "Source" ) );
        m_jobList->SetColumnWidth( jobBmpColId, 26 );
        m_jobList->SetColumnWidth( jobNoColId, GetTextExtent( wxT( "XXXX" ) ).GetWidth() );
        m_jobList->SetColumnWidth( jobSourceColId, GetTextExtent( wxT( "XXXXXX" ) ).GetWidth() );

        wxImageList* imageList = new wxImageList( 16, 16, true, 3 );
        imageList->Add( KiBitmapBundle( BITMAPS::ercerr ).GetBitmap( wxSize( 16, 16 ) ) );
        imageList->Add( KiBitmapBundle( BITMAPS::checked_ok ).GetBitmap( wxSize( 16, 16 ) ) );
        m_jobList->SetImageList( imageList, wxIMAGE_LIST_SMALL );

        int num = 1;

        for( JOBSET_JOB& job : aJobsFile->GetJobsForDestination( aDestination ) )
        {
            int imageIdx = -1;

            if( aDestination->m_lastRunSuccessMap.contains( job.m_id ) )
            {
                if( aDestination->m_lastRunSuccessMap[job.m_id].value() )
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
                    source = wxT( "PCB" );
                else if( iface == KIWAY::FACE_SCH )
					source = wxT( "SCH" );
            }

            m_jobList->SetItem( itemIndex, jobSourceColId, source );
        }

        SetupStandardButtons( { { wxID_OK, _( "Close" ) } } );
        finishDialogSettings();
    }

    virtual void OnUpdateUI( wxUpdateUIEvent& event ) override
    {
        if( GetSize().GetWidth() != m_lastWidth )
        {
            m_lastWidth = GetSize().GetWidth();

            if( m_marginsWidth < 0 )
                m_marginsWidth = m_lastWidth - ( m_jobList->GetSize().GetWidth() * 2 );

            int width = ( m_lastWidth / 2 );
            width -= m_marginsWidth;
            width -= m_jobList->GetColumnWidth( 0 );
            width -= m_jobList->GetColumnWidth( 1 );
            width -= m_jobList->GetColumnWidth( 3 );

            m_jobList->SetColumnWidth( 2, width );
        }
    }

    void OnJobListItemSelected( wxListEvent& event ) override
    {
        int itemIndex = event.GetIndex();

        // The index could be negative (it is default -1)
        if( itemIndex < 0 )
            return;

        std::vector<JOBSET_JOB> jobs = m_jobsFile->GetJobsForDestination( m_destination );

        if( static_cast<size_t>( itemIndex ) < jobs.size() )
        {
            wxString jobId = jobs[itemIndex].m_id;

            if( m_destination->m_lastRunReporters.contains( jobId ) )
                m_textCtrlOutput->SetValue( m_destination->m_lastRunReporters.at( jobId )->GetMessages() );
            else
                m_textCtrlOutput->SetValue( _( "No output messages" ) );
        }
    }

private:
    JOBSET*             m_jobsFile;
    JOBSET_DESTINATION* m_destination;

    int                 m_lastWidth;
    int                 m_marginsWidth;
};


class PANEL_DESTINATION : public PANEL_DESTINATION_BASE
{
public:
    PANEL_DESTINATION( wxWindow* aParent, PANEL_JOBSET* aPanelParent, KICAD_MANAGER_FRAME* aFrame,
                       JOBSET* aFile, JOBSET_DESTINATION* aDestination ) :
            PANEL_DESTINATION_BASE( aParent ),
            m_jobsFile( aFile ),
            m_destinationId( aDestination->m_id ),
            m_frame( aFrame ),
            m_panelParent( aPanelParent )
    {
        m_buttonProperties->SetBitmap( KiBitmapBundle( BITMAPS::config ) );
        m_buttonDelete->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
        m_buttonOpenOutput->SetBitmap( KiBitmapBundle( BITMAPS::small_new_window ) );

#if  _WIN32
        // BORDER_RAISED/SUNKEN look pretty on every platform but Windows
        long style = GetWindowStyleFlag();
        style &= ~wxBORDER_MASK;
        style |= wxBORDER_SIMPLE;
        SetWindowStyleFlag( style );
#endif //  _WIN32

        Connect( wxEVT_MENU, wxCommandEventHandler( PANEL_DESTINATION::onMenu ), nullptr, this );

        if( JobsetDestinationTypeInfos.contains( aDestination->m_type ) )
        {
            JOBSET_DESTINATION_T_INFO& jobTypeInfo = JobsetDestinationTypeInfos[aDestination->m_type];
            m_textOutputType->SetLabel( aDestination->GetDescription() );
            m_bitmapOutputType->SetBitmap( KiBitmapBundle( jobTypeInfo.bitmap ) );
        }

        m_pathInfo->SetFont( KIUI::GetSmallInfoFont( this ) );
        UpdatePathInfo( aDestination->GetPathInfo() );
        UpdateStatus();
    }

    ~PANEL_DESTINATION()
    {
        Disconnect( wxEVT_MENU, wxCommandEventHandler( PANEL_DESTINATION::onMenu ), nullptr, this );
    }

    void ClearStatus()
    {
        JOBSET_DESTINATION* destination = GetDestination();
        wxCHECK( destination, /*void*/ );

        destination->m_lastRunSuccess = std::nullopt;
        destination->m_lastResolvedOutputPath = std::nullopt;
        m_statusBitmap->SetBitmap( wxNullBitmap );
    }

    void UpdateStatus()
    {
        JOBSET_DESTINATION* destination = GetDestination();
        wxCHECK( destination, /*void*/ );

        if( destination->m_lastRunSuccess.has_value() )
        {
            if( destination->m_lastRunSuccess.value() )
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

        m_buttonGenerate->Enable( !m_jobsFile->GetJobsForDestination( destination ).empty() );
    }

    void UpdatePathInfo( const wxString& aMsg )
    {
        wxClientDC dc( this );
        int        width = GetSize().GetWidth();

        m_pathInfo->SetLabel( wxControl::Ellipsize( aMsg, dc, wxELLIPSIZE_MIDDLE, width ) );
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

                    {
                        JOBS_PROGRESS_REPORTER progressReporter( m_frame, _( "Running Jobs" ) );
                        JOBS_RUNNER            jobRunner( &m_frame->Kiway(), m_jobsFile, &project,
                                                          NULL_REPORTER::GetInstance(), &progressReporter );

                        if( JOBSET_DESTINATION* destination = GetDestination() )
                            jobRunner.RunJobsForDestination( destination );

                        UpdateStatus();
                    }

                    // Bring the Kicad manager frame back to the front
                    m_frame->Raise();
                } );
    }

    void OnOpenOutput( wxCommandEvent& aEvent ) override
    {
        JOBSET_DESTINATION* destination = GetDestination();
        wxCHECK( destination, /*void*/ );

        wxString resolvedPath;

        if( destination->m_lastResolvedOutputPath.has_value() )
        {
            resolvedPath = destination->m_lastResolvedOutputPath.value();
        }
        else
        {
            resolvedPath = ExpandTextVars( destination->GetPathInfo(), &m_frame->Prj() );
            resolvedPath = ExpandEnvVarSubstitutions( resolvedPath, &m_frame->Prj() );

            if( resolvedPath.StartsWith( "~" ) )
                resolvedPath.Replace( "~", wxGetHomeDir(), false );
        }

        if( resolvedPath.IsEmpty() )
            return;

        if( !LaunchExternal( resolvedPath ) )
            DisplayError( this, wxString::Format( _( "Failed to open '%s'." ), resolvedPath ) );
    }

    void OnLastStatusClick( wxMouseEvent& aEvent ) override
    {
        JOBSET_DESTINATION* destination = GetDestination();
        wxCHECK( destination, /*void*/ );

        DIALOG_JOBSET_RUN_LOG dialog( m_frame, m_jobsFile, destination );
        dialog.ShowModal();
    }

    void OnRightDown( wxMouseEvent& aEvent ) override
    {
        JOBSET_DESTINATION* destination = GetDestination();
        wxCHECK( destination, /*void*/ );

        wxMenu menu;
        menu.Append( wxID_EDIT, _( "Edit Destination Options..." ) );
        menu.Append( wxID_DELETE, _( "Delete Destination" ) );

        menu.AppendSeparator();
        menu.Append( wxID_VIEW_DETAILS, _( "View Last Run Log..." ) );
        menu.Append( wxID_OPEN, _( "Open Output" ) );

        menu.Enable( wxID_VIEW_DETAILS, destination->m_lastRunSuccess.has_value() );
        menu.Enable( wxID_OPEN, m_buttonOpenOutput->IsEnabled() );

        PopupMenu( &menu );
    }

    void OnProperties( wxCommandEvent& aEvent ) override
    {
        JOBSET_DESTINATION* destination = GetDestination();
        wxCHECK( destination, /*void*/ );

        DIALOG_DESTINATION dialog( m_frame, m_jobsFile, destination );

        if( dialog.ShowModal() == wxID_OK )
        {
            m_textOutputType->SetLabel( destination->GetDescription() );
            UpdatePathInfo( destination->GetPathInfo() );
            m_jobsFile->SetDirty();
            m_panelParent->UpdateTitle();
            ClearStatus();
        }
    }

    virtual void OnDelete( wxCommandEvent& aEvent ) override
    {
        m_panelParent->RemoveDestination( this );
    }

    JOBSET_DESTINATION* GetDestination()
    {
        for( JOBSET_DESTINATION& destination : m_jobsFile->GetDestinations() )
        {
            if( destination.m_id == m_destinationId )
                return &destination;
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

                case wxID_OPEN:
                {
                    wxCommandEvent dummy;
                    OnOpenOutput( dummy );
                }
                break;

                default: wxFAIL_MSG( wxT( "Unknown ID in context menu event" ) );
                }
    }

private:
    JOBSET*              m_jobsFile;
    wxString             m_destinationId;
    KICAD_MANAGER_FRAME* m_frame;
    PANEL_JOBSET*        m_panelParent;
};


JOBS_GRID_TRICKS::JOBS_GRID_TRICKS( PANEL_JOBSET* aParent, WX_GRID* aGrid ) :
        GRID_TRICKS( aGrid ),
        m_parent( aParent ),
        m_doubleClickRow( -1 )
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
    menu.Append( GRIDTRICKS_ID_COPY, _( "Copy" ) + "\tCtrl+C", _( "Copy selected cells to clipboard" ) );
    menu.Append( GRIDTRICKS_ID_DELETE, _( "Delete" ) + "\tDel", _( "Delete selected jobs" ) );
    menu.Append( GRIDTRICKS_ID_SELECT, _( "Select All" ) + "\tCtrl+A", _( "Select all jobs" ) );

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

    m_jobsGrid->OverrideMinSize( 0.6, 0.3 );
    m_jobsGrid->SetSelectionMode( wxGrid::wxGridSelectRows );

    // 'm' for margins
    m_jobsGrid->SetColSize( COL_NUMBER, GetTextExtent( wxT( "99m" ) ).x );
    m_jobsGrid->SetColSize( COL_SOURCE, GetTextExtent( wxT( "PCBm" ) ).x );

    m_buttonAddJob->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_buttonUp->SetBitmap( KiBitmapBundle( BITMAPS::small_up ) );
    m_buttonDown->SetBitmap( KiBitmapBundle( BITMAPS::small_down ) );
    m_buttonDelete->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_buttonAddDestination->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );

    rebuildJobList();
    buildDestinationList();
}


PANEL_JOBSET::~PANEL_JOBSET()
{
    // Delete the GRID_TRICKS.
    m_jobsGrid->PopEventHandler( true );
}


void PANEL_JOBSET::RemoveDestination( PANEL_DESTINATION* aPanel )
{
    JOBSET_DESTINATION* output = aPanel->GetDestination();

    m_destinationListSizer->Detach( aPanel );
    aPanel->Destroy();

    // ensure the window contents get shifted as needed
    m_destinationList->Layout();
    Layout();

    m_jobsFile->RemoveDestination( output );
}


void PANEL_JOBSET::rebuildJobList()
{
    m_jobsGrid->ClearRows();
    m_jobsGrid->AppendRows( (int) m_jobsFile->GetJobs().size() );

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
                source = wxT( "PCB" );
            else if( iface == KIWAY::FACE_SCH )
                source = wxT( "SCH" );
        }

        m_jobsGrid->SetCellValue( num - 1, COL_SOURCE, source );

        num++;
    }

    UpdateTitle();

    // Ensure the outputs get their Run-ability status updated
    for( PANEL_DESTINATION* panel : GetDestinationPanels() )
        panel->UpdateStatus();
}


void PANEL_JOBSET::UpdateTitle()
{
    wxString tabName = m_jobsFile->GetFullName();

    if( m_jobsFile->GetDirty() )
        tabName = wxS( "*" ) + tabName;

    int pageIdx = m_parentBook->FindPage( this );

    if( pageIdx >= 0 )
        m_parentBook->SetPageText( pageIdx, tabName );
}


void PANEL_JOBSET::addDestinationPanel( JOBSET_DESTINATION* aOutput )
{
    PANEL_DESTINATION* destinationPanel = new PANEL_DESTINATION( m_destinationList, this, m_frame,
                                                                 m_jobsFile.get(), aOutput );

#if __OSX__
    m_outputListSizer->Add( destinationPanel, 0, wxEXPAND, 5 );
#else
    m_destinationListSizer->Add( destinationPanel, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 5 );
#endif

    m_destinationList->Layout();
}


std::vector<PANEL_DESTINATION*> PANEL_JOBSET::GetDestinationPanels()
{
    std::vector<PANEL_DESTINATION*> panels;

    for( const wxSizerItem* item : m_destinationListSizer->GetChildren() )
    {
        if( PANEL_DESTINATION* panel = dynamic_cast<PANEL_DESTINATION*>( item->GetWindow() ) )
            panels.push_back( panel );
    }

    return panels;
}


void PANEL_JOBSET::buildDestinationList()
{
    Freeze();

    for( JOBSET_DESTINATION& job : m_jobsFile->GetDestinations() )
        addDestinationPanel( &job );

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

            // QuasiModal for Scintilla autocomplete
            if( dialog.ShowQuasiModal() == wxID_OK )
                success = true;
        }
        else if( job.m_job->GetType() == "special_copyfiles" )
        {
            JOB_SPECIAL_COPYFILES* specialJob = static_cast<JOB_SPECIAL_COPYFILES*>( job.m_job.get() );
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
        if( entry.second.deprecated )
            continue;

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
                if( entry.second.deprecated )
                    continue;

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
    m_jobsGrid->OnDeleteRows(
            [&]( int row )
            {
                m_jobsFile->RemoveJob( row );
                m_jobsGrid->DeleteRows( row, 1 );
            } );
}


void PANEL_JOBSET::OnAddDestinationClick( wxCommandEvent& aEvent )
{
    wxArrayString              headers;
    std::vector<wxArrayString> items;

    headers.Add( _( "Destination Types" ) );

    for( const auto& [destinationType, destinationTypeInfo] : JobsetDestinationTypeInfos )
    {
        wxArrayString item;
        item.Add( wxGetTranslation( destinationTypeInfo.name ) );
        items.emplace_back( item );
    }

    EDA_LIST_DIALOG dlg( this, _( "Add New Destination" ), headers, items );
    dlg.SetListLabel( _( "Select destination type:" ) );
    dlg.HideFilter();

    if( dlg.ShowModal() == wxID_OK )
    {
        wxString selectedString = dlg.GetTextSelection();

        for( const auto& [destinationType, destinationTypeInfo] : JobsetDestinationTypeInfos )
        {
            if( wxGetTranslation( destinationTypeInfo.name ) == selectedString )
            {
                JOBSET_DESTINATION* destination = m_jobsFile->AddNewDestination( destinationType );

                DIALOG_DESTINATION dialog( m_frame, m_jobsFile.get(), destination );

                if (dialog.ShowModal() == wxID_OK)
                {
                    Freeze();
                    addDestinationPanel( destination );
                    Thaw();
                }
                else
                {
                    // canceled
                    m_jobsFile->RemoveDestination( destination );
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
                                       return m_jobsFile->SaveToFile( wxEmptyString, true );
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


void PANEL_JOBSET::OnGenerateAllDestinationsClick( wxCommandEvent& event )
{
    if( !m_jobsGrid->CommitPendingChanges() )
        return;

    // sanity
    if( m_jobsFile->GetDestinations().empty() )
	{
		DisplayError( this, _( "No destinations defined" ) );
		return;
	}

    for( PANEL_DESTINATION* panel : GetDestinationPanels() )
        panel->ClearStatus();

    Refresh();

	CallAfter(
			[this]()
			{
				PROJECT& project = m_frame->Kiway().Prj();
				EnsurePcbSchFramesOpen();

				wxFileName fn = project.GetProjectFullName();
				wxSetWorkingDirectory( fn.GetPath() );

                {
                    JOBS_PROGRESS_REPORTER progressReporter( m_frame, _( "Running Jobs" ) );
                    JOBS_RUNNER            jobRunner( &m_frame->Kiway(), m_jobsFile.get(), &project,
                                                      NULL_REPORTER::GetInstance(), &progressReporter );

                    jobRunner.RunJobsAllDestinations();

                    for( PANEL_DESTINATION* panel : GetDestinationPanels() )
                        panel->UpdateStatus();
                }

                // Bring the Kicad manager frame back to the front
                m_frame->Raise();
			} );
}


void PANEL_JOBSET::OnSizeGrid( wxSizeEvent& aEvent )
{
    m_jobsGrid->SetColSize( COL_DESCR, m_jobsGrid->GetSize().x - m_jobsGrid->GetColSize( COL_SOURCE )
                                                               - m_jobsGrid->GetColSize( COL_NUMBER ) );

    // Always propagate for a grid repaint (needed if the height changes, as well as width)
    aEvent.Skip();
}

