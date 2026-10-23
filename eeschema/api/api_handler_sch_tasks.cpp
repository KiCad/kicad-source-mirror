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

#include <api/api_handler_sch.h>
#include <api/api_task_scheduler.h>
#include <api/api_task_gui_reporter.h>

#include <connection_graph.h>
#include <drawing_sheet/ds_proxy_view_item.h>
#include <erc/erc.h>
#include <erc/erc_item.h>
#include <erc/erc_settings.h>
#include <fmt.h>
#include <kiway.h>
#include <layer_ids.h>
#include <libraries/symbol_library_adapter.h>
#include <project_sch.h>
#include <sch_edit_frame.h>
#include <sch_marker.h>
#include <sch_screen.h>
#include <schematic.h>
#include <string_utils.h>
#include <widgets/report_severity.h>
#include <widgets/wx_progress_reporters.h>

#include <api/api_enums.h>
#include <api/api_utils.h>
#include <api/common/tasks.pb.h>
#include <api/schematic/schematic_tasks.pb.h>

#include <algorithm>
#include <memory>

using kiapi::common::tasks::StartTaskResponse;
using kiapi::common::tasks::TaskState;
using kiapi::common::tasks::TaskStatusResponse;
using kiapi::common::tasks::TS_CANCELLED;
using kiapi::common::tasks::TS_FAILED;
using kiapi::common::tasks::TS_RUNNING;
using kiapi::common::tasks::TS_UNKNOWN;
using kiapi::schematic::tasks::ErcTaskResult;
using kiapi::schematic::tasks::GetErcTaskResult;
using kiapi::schematic::tasks::StartErcTask;


class API_HANDLER_SCH::ERC_TASK : public API_TASK
{
public:
    ERC_TASK( API_HANDLER_SCH* aHandler, SCHEMATIC* aSchematic ) :
            m_handler( aHandler ),
            m_schematic( aSchematic )
    {
    }

    void Run() override
    {
        SCH_EDIT_FRAME* frame = m_handler->frame();

        if( frame )
            frame->ClearErcMarkers();

        SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( &m_schematic->Project() );
        adapter->AsyncLoad();
        adapter->BlockUntilLoaded();

        ERC_TESTER ercTester( m_schematic );

        DS_PROXY_VIEW_ITEM* drawingSheetRaw = new DS_PROXY_VIEW_ITEM(
                schIUScale, &m_schematic->RootScreen()->GetPageSettings(),
                &m_schematic->Project(), &m_schematic->RootScreen()->GetTitleBlock(),
                m_schematic->GetProperties() );
        drawingSheetRaw->SetPageNumber( TO_UTF8( m_schematic->RootScreen()->GetPageNumber() ) );
        drawingSheetRaw->SetSheetCount( m_schematic->RootScreen()->GetPageCount() );
        drawingSheetRaw->SetFileName( TO_UTF8( m_schematic->RootScreen()->GetFileName() ) );
        drawingSheetRaw->SetColorLayer( LAYER_SCHEMATIC_DRAWINGSHEET );
        drawingSheetRaw->SetPageBorderColorLayer( LAYER_SCHEMATIC_PAGE_LIMITS );
        drawingSheetRaw->SetIsFirstPage( m_schematic->RootScreen()->GetVirtualPageNumber() == 1 );

        wxString variant = m_schematic->GetCurrentVariant();
        drawingSheetRaw->SetVariantName( TO_UTF8( variant ) );
        drawingSheetRaw->SetVariantDesc( TO_UTF8( m_schematic->GetVariantDescription( variant ) ) );

        drawingSheetRaw->SetSheetName( "" );
        drawingSheetRaw->SetSheetPath( "" );
        std::unique_ptr<DS_PROXY_VIEW_ITEM> drawingSheet( drawingSheetRaw );

        KIWAY* kiway = m_handler->context()->GetKiway();

        std::unique_ptr<API_TASK_GUI_PROGRESS_REPORTER> reporter;

        if( frame )
            reporter = std::make_unique<API_TASK_GUI_PROGRESS_REPORTER>( frame, _( "Running ERC" ), 1, 0 );

        ercTester.RunTests( drawingSheet.get(), frame,
                            kiway ? kiway->KiFACE( KIWAY::FACE_CVPCB ) : nullptr,
                            &m_schematic->Project(), reporter.get() );

        reporter.reset();

        if( frame )
            frame->RefreshErcMarkers();

        buildResult();
    }

    void FillStatus( TaskStatusResponse& aStatus ) const override
    {
        if( m_state != TS_UNKNOWN )
            aStatus.set_state( m_state );

        if( !m_errorMessage.empty() )
            aStatus.set_error_message( m_errorMessage );
    }

    ErcTaskResult Result() const
    {
        return m_result;
    }

private:
    void buildResult()
    {
        m_result.Clear();

        std::vector<SCH_MARKER*> markers;

        SCH_SCREENS screens( m_schematic->Root() );

        for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
        {
            for( SCH_ITEM* item : screen->Items() )
            {
                if( item->Type() == SCH_MARKER_T )
                    markers.push_back( static_cast<SCH_MARKER*>( item ) );
            }
        }

        std::stable_sort( markers.begin(), markers.end(),
                          []( const SCH_MARKER* a, const SCH_MARKER* b )
                          {
                              auto rank = []( SEVERITY aSeverity )
                              {
                                  return aSeverity == RPT_SEVERITY_IGNORE
                                                 || aSeverity == RPT_SEVERITY_DEBUG
                                         ? 0
                                         : static_cast<int>( aSeverity );
                              };

                              return rank( a->GetSeverity() ) > rank( b->GetSeverity() );
                          } );

        for( SCH_MARKER* marker : markers )
        {
            if( marker->GetSeverity() == RPT_SEVERITY_EXCLUSION )
                m_result.set_excluded_count( m_result.excluded_count() + 1 );

            kiapi::schematic::ErcMarker markerMsg;
            SCH_MARKER::ToProto( markerMsg, *marker );
            *m_result.add_violations() = std::move( markerMsg );
        }

        for( const std::reference_wrapper<RC_ITEM>& item : ERC_ITEM::GetItemsWithSeverities() )
        {
            int code = item.get().GetErrorCode();

            if( code > 0 && m_schematic->ErcSettings().GetSeverity( code ) == RPT_SEVERITY_IGNORE )
                m_result.set_ignored_count( m_result.ignored_count() + 1 );
        }
    }

    API_HANDLER_SCH* m_handler;
    SCHEMATIC*       m_schematic;

    TaskState   m_state = TS_UNKNOWN;
    std::string m_errorMessage;

    ErcTaskResult      m_result;
};


HANDLER_RESULT<StartTaskResponse> API_HANDLER_SCH::handleStartErcTask( const HANDLER_CONTEXT<StartErcTask>& aCtx )
{
    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.document() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    SCHEMATIC* schematic = context()->GetSchematic();

    if( !schematic )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "no schematic is open" );
        return tl::unexpected( e );
    }

    auto task = std::make_shared<API_HANDLER_SCH::ERC_TASK>( this, schematic );

    std::optional<std::string> token = API_TASK_SCHEDULER::Instance().StartTask( task, wxS( "ERC" ) );

    if( !token.has_value() )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BUSY );
        e.set_error_message( "another task is already running" );
        return tl::unexpected( e );
    }

    StartTaskResponse response;
    response.mutable_token()->set_value( *token );
    return response;
}


HANDLER_RESULT<ErcTaskResult> API_HANDLER_SCH::handleGetErcTaskResult( const HANDLER_CONTEXT<GetErcTaskResult>& aCtx )
{
    if( !aCtx.Request.has_token() || aCtx.Request.token().value().empty() )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "no task token provided" );
        return tl::unexpected( e );
    }

    std::optional<TaskStatusResponse> status = API_TASK_SCHEDULER::Instance().GetStatus( aCtx.Request.token().value() );

    if( !status )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( fmt::format( "unknown task token {}", aCtx.Request.token().value() ) );
        return tl::unexpected( e );
    }

    if( status->state() == TS_RUNNING )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BUSY );
        e.set_error_message( "the ERC task is still running" );
        return tl::unexpected( e );
    }

    if( status->state() == TS_FAILED || status->state() == TS_CANCELLED )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_INTERNAL_ERROR );
        e.set_error_message( status->error_message().empty() ? std::string( "ERC task failed" )
                                                             : status->error_message() );
        return tl::unexpected( e );
    }

    std::shared_ptr<API_TASK> task = API_TASK_SCHEDULER::Instance().GetFinishedTask( aCtx.Request.token().value() );

    if( !task )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( fmt::format( "no completed task with token {}", aCtx.Request.token().value() ) );
        return tl::unexpected( e );
    }

    auto* ercTask = dynamic_cast<API_HANDLER_SCH::ERC_TASK*>( task.get() );

    if( !ercTask )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "task is not an ERC task" );
        return tl::unexpected( e );
    }

    return ercTask->Result();
}
