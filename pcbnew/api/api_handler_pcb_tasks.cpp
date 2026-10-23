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

#include <api/api_handler_pcb.h>
#include <api/api_task_scheduler.h>
#include <api/api_task_gui_reporter.h>

#include <board.h>
#include <board_commit.h>
#include <pcb_edit_frame.h>
#include <tool/tool_manager.h>
#include <tools/zone_filler_tool.h>
#include <widgets/wx_progress_reporters.h>
#include <zone.h>
#include <zone_filler.h>

#include <api/board/board_tasks.pb.h>
#include <api/common/tasks.pb.h>

#include <memory>

using kiapi::board::tasks::StartRefillZonesTask;
using kiapi::common::tasks::StartTaskResponse;
using kiapi::common::tasks::TaskState;
using kiapi::common::tasks::TaskStatusResponse;
using kiapi::common::tasks::TS_CANCELLED;
using kiapi::common::tasks::TS_FAILED;
using kiapi::common::tasks::TS_UNKNOWN;


class API_HANDLER_PCB::REFILL_ZONES_TASK : public API_TASK
{
public:
    REFILL_ZONES_TASK( API_HANDLER_PCB* aHandler, std::vector<ZONE*>&& aZones ) :
            m_handler( aHandler ),
            m_board( aHandler->board() ),
            m_zones( std::move( aZones ) )
    {
    }

    void Run() override
    {
        PCB_EDIT_FRAME* frame = m_handler->frame();
        TOOL_MANAGER*   mgr = m_handler->toolManager();

        if( !mgr->FindTool( ZONE_FILLER_TOOL_NAME ) )
            mgr->RegisterTool( new ZONE_FILLER_TOOL );

        ZONE_FILLER_TOOL* zoneFillerTool = mgr->GetTool<ZONE_FILLER_TOOL>();

        if( !zoneFillerTool )
        {
            m_zones.clear();
            m_state = TS_FAILED;
            m_errorMessage = "zone filler tool is not available";
            return;
        }

        std::unique_ptr<API_TASK_GUI_PROGRESS_REPORTER> reporter;

        if( frame )
            reporter = std::make_unique<API_TASK_GUI_PROGRESS_REPORTER>( frame, _( "Fill Zone(s)" ), 1, PR_CAN_ABORT );

        bool filled = false;

        if( m_zones.empty() )
        {
            filled = zoneFillerTool->FillAllZones( frame, reporter.get(), frame == nullptr );
        }
        else
        {
            std::unique_ptr<COMMIT> commit = m_handler->createCommit();
            ZONE_FILLER             filler( m_board, commit.get() );

            if( reporter )
                filler.SetProgressReporter( reporter.get() );

            try
            {
                filled = filler.Fill( m_zones );
            }
            catch( ... )
            {
                commit->Revert();
                throw;
            }

            if( filled )
                commit->Push( _( "Refill zones from API task" ), SKIP_CONNECTIVITY | ZONE_FILL_OP );
            else
                commit->Revert();

            if( filled )
                zoneFillerTool->PostFillRefresh( frame == nullptr );
        }

        m_zones.clear();

        if( !filled )
        {
            if( reporter && reporter->IsCancelled() )
            {
                m_state = TS_CANCELLED;
                m_errorMessage = "zone fill cancelled by user";
            }
            else
            {
                m_state = TS_FAILED;
                m_errorMessage = "zone fill failed";
            }
        }
        else
        {
            m_handler->onModified();
        }
    }

    void FillStatus( TaskStatusResponse& aStatus ) const override
    {
        if( m_state != TS_UNKNOWN )
            aStatus.set_state( m_state );

        if( !m_errorMessage.empty() )
            aStatus.set_error_message( m_errorMessage );
    }

private:
    API_HANDLER_PCB*   m_handler;
    BOARD*             m_board;
    std::vector<ZONE*> m_zones;
    TaskState          m_state = TS_UNKNOWN;
    std::string        m_errorMessage;
};


HANDLER_RESULT<StartTaskResponse>
API_HANDLER_PCB::handleStartRefillZonesTask( const HANDLER_CONTEXT<StartRefillZonesTask>& aCtx )
{
    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.board() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    HANDLER_RESULT<std::vector<ZONE*>> toFill = resolveZonesToFill( aCtx.Request.zones() );

    if( !toFill )
        return tl::unexpected( toFill.error() );

    auto task = std::make_shared<API_HANDLER_PCB::REFILL_ZONES_TASK>( this, std::move( *toFill ) );

    std::optional<std::string> token = API_TASK_SCHEDULER::Instance().StartTask( task, wxS( "RefillZones" ) );

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
