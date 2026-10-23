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

#include <base_units.h>
#include <board.h>
#include <board_commit.h>
#include <board_design_settings.h>
#include <dialogs/dialog_drc.h>
#include <drawing_sheet/ds_proxy_view_item.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <fmt.h>
#include <kiface_services.h>
#include <kiway.h>
#include <macros.h>
#include <netlist_reader/netlist_reader.h>
#include <netlist_reader/pcb_netlist.h>
#include <pcb_edit_frame.h>
#include <pcb_marker.h>
#include <richio.h>
#include <string_utils.h>
#include <tool/tool_manager.h>
#include <tools/drc_tool.h>
#include <tools/zone_filler_tool.h>
#include <widgets/report_severity.h>
#include <widgets/wx_progress_reporters.h>
#include <wildcards_and_files_ext.h>
#include <zone.h>
#include <zone_filler.h>

#include <api/api_enums.h>
#include <api/api_utils.h>
#include <api/board/board_tasks.pb.h>
#include <api/common/tasks.pb.h>

#include <algorithm>
#include <memory>

using kiapi::board::tasks::DrcTaskResult;
using kiapi::board::tasks::GetDrcTaskResult;
using kiapi::board::tasks::StartDrcTask;
using kiapi::board::tasks::StartRefillZonesTask;
using kiapi::common::tasks::StartTaskResponse;
using kiapi::common::tasks::TaskState;
using kiapi::common::tasks::TaskStatusResponse;
using kiapi::common::tasks::TS_CANCELLED;
using kiapi::common::tasks::TS_FAILED;
using kiapi::common::tasks::TS_RUNNING;
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


class API_HANDLER_PCB::DRC_TASK : public API_TASK
{
public:
    DRC_TASK( API_HANDLER_PCB* aHandler, bool aReportAllTrackErrors, bool aTestFootprints,
              bool aRequireZonesFilled ) :
            m_handler( aHandler ),
            m_board( aHandler->board() ),
            m_reportAllTrackErrors( aReportAllTrackErrors ),
            m_testFootprints( aTestFootprints ),
            m_requireZonesFilled( aRequireZonesFilled )
    {
    }

    void Run() override
    {
        if( m_requireZonesFilled && !checkZonesFilled() )
            return;

        std::shared_ptr<DRC_ENGINE> drcEngine = m_board->GetDesignSettings().m_DRCEngine;

        try
        {
            drcEngine->InitEngine( m_board->GetDesignRulesPath() );
        }
        catch( const PARSE_ERROR& )
        {
            m_state = TS_FAILED;
            m_errorMessage = "DRC incomplete: could not compile custom design rules";
            return;
        }

        if( m_testFootprints && !fetchNetlist() )
            return;

        EDA_UNITS units = EDA_UNITS::MM;
        PCB_EDIT_FRAME* frame = m_handler->frame();

        if( frame )
            units = frame->GetUserUnits();

        TOOL_MANAGER* mgr = m_handler->toolManager();
        DRC_TOOL*     drcTool = nullptr;

        if( frame )
        {
            if( !mgr->FindTool( DRC_TOOL_NAME ) )
                mgr->RegisterTool( new DRC_TOOL );

            drcTool = mgr->GetTool<DRC_TOOL>();

            if( !drcTool )
            {
                m_state = TS_FAILED;
                m_errorMessage = "DRC tool is not available";
                return;
            }

            if( drcTool->IsDRCRunning() )
            {
                m_state = TS_FAILED;
                m_errorMessage = "another DRC run is already in progress";
                return;
            }
        }

        if( m_testFootprints )
            drcEngine->SetSchematicNetlist( m_netlist.get() );

        m_commit = std::make_unique<BOARD_COMMIT>( mgr );

        drcEngine->SetViolationHandler(
                [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                     const std::function<void( PCB_MARKER* )>& aPathGenerator )
                {
                    PCB_MARKER* marker = new PCB_MARKER( aItem, aPos, aLayer );
                    aPathGenerator( marker );
                    m_commit->Add( marker );
                    m_violationMarkers.push_back( marker );
                } );

        m_board->RecordDRCExclusions();
        m_board->DeleteMARKERs( true, true );

        std::unique_ptr<API_TASK_GUI_PROGRESS_REPORTER> reporter;

        if( frame )
        {
            reporter = std::make_unique<API_TASK_GUI_PROGRESS_REPORTER>( frame, _( "Running DRC" ), 1,
                                                                        PR_CAN_ABORT );
            drcEngine->SetProgressReporter( reporter.get() );
        }

        std::unique_ptr<DS_PROXY_VIEW_ITEM> headlessDrawingSheet;

        if( frame )
        {
            drcEngine->SetDrawingSheet( frame->GetCanvas()->GetDrawingSheet() );
        }
        else
        {
            headlessDrawingSheet = std::make_unique<DS_PROXY_VIEW_ITEM>(
                    pcbIUScale, &m_board->GetPageSettings(), m_board->GetProject(), &m_board->GetTitleBlock(),
                    &m_board->GetProperties() );
            headlessDrawingSheet->SetSheetName( std::string() );
            headlessDrawingSheet->SetSheetPath( std::string() );
            headlessDrawingSheet->SetIsFirstPage( true );
            headlessDrawingSheet->SetFileName( TO_UTF8( m_board->GetFileName() ) );

            wxString variant = m_board->GetCurrentVariant();
            headlessDrawingSheet->SetVariantName( TO_UTF8( variant ) );
            headlessDrawingSheet->SetVariantDesc( TO_UTF8( m_board->GetVariantDescription( variant ) ) );

            drcEngine->SetDrawingSheet( headlessDrawingSheet.get() );
        }

        try
        {
            drcEngine->RunTests( units, m_reportAllTrackErrors, m_testFootprints, m_commit.get() );
        }
        catch( ... )
        {
            drcEngine->ClearViolationHandler();
            drcEngine->SetProgressReporter( nullptr );

            if( !frame )
                drcEngine->SetDrawingSheet( nullptr );

            headlessDrawingSheet.reset();
            reporter.reset();

            if( m_testFootprints )
                drcEngine->SetSchematicNetlist( nullptr );

            m_commit->Revert();
            m_commit.reset();
            m_violationMarkers.clear();
            m_netlist.reset();
            throw;
        }

        const bool cancelled = reporter && reporter->IsCancelled();

        drcEngine->ClearViolationHandler();
        drcEngine->SetProgressReporter( nullptr );

        if( !frame )
            drcEngine->SetDrawingSheet( nullptr );

        headlessDrawingSheet.reset();
        reporter.reset();

        if( m_testFootprints )
            drcEngine->SetSchematicNetlist( nullptr );

        m_netlist.reset();

        if( cancelled )
        {
            m_violationMarkers.clear();
            m_commit->Revert();
            m_commit.reset();

            std::unique_ptr<COMMIT> exclusionCommit = m_handler->createCommit();

            for( PCB_MARKER* marker : m_board->ResolveDRCExclusions( true ) )
            {
                if( frame && marker->GetMarkerType() == MARKER_BASE::MARKER_DRAWING_SHEET )
                    marker->GetRCItem()->SetItems( frame->GetCanvas()->GetDrawingSheet() );

                exclusionCommit->Add( marker );
            }

            exclusionCommit->Push( _( "DRC (cancelled)" ), SKIP_UNDO | SKIP_SET_DIRTY );
            m_board->UpdateRatsnestExclusions();

            m_state = TS_CANCELLED;
            m_errorMessage = "DRC cancelled by user";

            if( frame )
            {
                if( DIALOG_DRC* dialog = drcTool->GetDRCDialog() )
                    dialog->UpdateData();

                frame->ShowSolderMask();
            }

            return;
        }

        m_commit->Push( _( "DRC" ), SKIP_UNDO | SKIP_SET_DIRTY );
        m_board->ResolveDRCExclusions( false );

        buildResult();

        m_commit.reset();
        m_violationMarkers.clear();

        if( frame )
        {
            if( DIALOG_DRC* dialog = drcTool->GetDRCDialog() )
                dialog->UpdateData();

            frame->ShowSolderMask();
        }
    }

    void FillStatus( TaskStatusResponse& aStatus ) const override
    {
        if( m_state != TS_UNKNOWN )
            aStatus.set_state( m_state );

        if( !m_errorMessage.empty() )
            aStatus.set_error_message( m_errorMessage );
    }

    DrcTaskResult Result() const
    {
        return m_result;
    }

private:
    bool checkZonesFilled()
    {
        for( const ZONE* zone : m_board->Zones() )
        {
            if( zone->GetIsRuleArea() )
                continue;

            if( !zone->IsFilled() || zone->NeedRefill() )
            {
                m_state = TS_FAILED;
                m_errorMessage = "zone fills are not up to date; refill the zones "
                                 "(StartRefillZonesTask) before running DRC";
                return false;
            }
        }

        return true;
    }

    bool fetchNetlist()
    {
        m_netlist = std::make_unique<NETLIST>();

        if( PCB_EDIT_FRAME* frame = m_handler->frame() )
        {
            if( !frame->FetchNetlistFromSchematic( *m_netlist,
                                                   _( "Schematic parity tests require a fully annotated schematic." ) ) )
            {
                m_state = TS_FAILED;
                m_errorMessage = "could not fetch schematic netlist for parity tests";
                return false;
            }

            return true;
        }

        wxFileName schematicPath( m_board->GetFileName() );
        schematicPath.SetExt( FILEEXT::KiCadSchematicFileExtension );

        if( !schematicPath.FileExists() )
            schematicPath.SetExt( FILEEXT::LegacySchematicFileExtension );

        if( !schematicPath.FileExists() )
        {
            m_state = TS_FAILED;
            m_errorMessage = "schematic file not found for parity tests";
            return false;
        }

        KIWAY*  kiway = m_handler->pcbContext()->GetKiway();
        KIFACE* eeschema = kiway ? kiway->KiFACE( KIWAY::FACE_SCH ) : nullptr;

        if( !eeschema )
        {
            m_state = TS_FAILED;
            m_errorMessage = "could not load schematic editor for parity tests";
            return false;
        }

        auto netlister = GetKifaceService<KIFACE_NETLIST_SCHEMATIC_SERVICE>( eeschema );

        if( !netlister )
        {
            m_state = TS_FAILED;
            m_errorMessage = "could not load schematic editor for parity tests";
            return false;
        }

        std::string netlistStr;

        if( !( *netlister )( schematicPath.GetFullPath(), netlistStr, kiway )
            || netlistStr.empty() || netlistStr == MAIL_SCH_GET_NETLIST_CANCELLED )
        {
            m_state = TS_FAILED;
            m_errorMessage = "could not fetch schematic netlist for parity tests";
            return false;
        }

        STRING_LINE_READER*  lineReader = new STRING_LINE_READER( netlistStr, _( "Eeschema netlist" ) );
        KICAD_NETLIST_READER netlistReader( lineReader, m_netlist.get() );

        try
        {
            netlistReader.LoadNetlist();
        }
        catch( const IO_ERROR& )
        {
            m_state = TS_FAILED;
            m_errorMessage = "could not parse schematic netlist for parity tests";
            return false;
        }

        return true;
    }

    void buildResult()
    {
        m_result.Clear();

        std::stable_sort( m_violationMarkers.begin(), m_violationMarkers.end(),
                          []( const PCB_MARKER* a, const PCB_MARKER* b )
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

        for( const PCB_MARKER* marker : m_violationMarkers )
        {
            if( marker->GetSeverity() == RPT_SEVERITY_EXCLUSION )
                m_result.set_excluded_count( m_result.excluded_count() + 1 );

            kiapi::board::DrcMarker markerMsg;
            PCB_MARKER::ToProto( markerMsg, *marker );
            *m_result.add_violations() = std::move( markerMsg );
        }

        for( const std::reference_wrapper<RC_ITEM>& item : DRC_ITEM::GetItemsWithSeverities() )
        {
            int code = item.get().GetErrorCode();

            if( code > 0
                && m_board->GetDesignSettings().GetSeverity( code ) == RPT_SEVERITY_IGNORE )
            {
                m_result.set_ignored_count( m_result.ignored_count() + 1 );
            }
        }
    }

    API_HANDLER_PCB* m_handler;
    BOARD*           m_board;
    bool             m_reportAllTrackErrors;
    bool             m_testFootprints;
    bool             m_requireZonesFilled;

    TaskState         m_state = TS_UNKNOWN;
    std::string       m_errorMessage;

    std::unique_ptr<NETLIST>      m_netlist;
    std::unique_ptr<BOARD_COMMIT> m_commit;
    std::vector<PCB_MARKER*>      m_violationMarkers;
    DrcTaskResult                 m_result;
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


HANDLER_RESULT<StartTaskResponse> API_HANDLER_PCB::handleStartDrcTask( const HANDLER_CONTEXT<StartDrcTask>& aCtx )
{
    HANDLER_RESULT<bool> documentValidation = validateDocument( aCtx.Request.board() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    auto task = std::make_shared<API_HANDLER_PCB::DRC_TASK>( this,
                                                             aCtx.Request.report_all_track_errors(),
                                                             aCtx.Request.test_schematic_parity(),
                                                             aCtx.Request.require_zones_filled() );

    std::optional<std::string> token = API_TASK_SCHEDULER::Instance().StartTask( task, wxS( "DRC" ) );

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


HANDLER_RESULT<DrcTaskResult> API_HANDLER_PCB::handleGetDrcTaskResult( const HANDLER_CONTEXT<GetDrcTaskResult>& aCtx )
{
    if( !aCtx.Request.has_token() || aCtx.Request.token().value().empty() )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "no task token provided" );
        return tl::unexpected( e );
    }

    std::optional<TaskStatusResponse> status =
            API_TASK_SCHEDULER::Instance().GetStatus( aCtx.Request.token().value() );

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
        e.set_error_message( "the DRC task is still running" );
        return tl::unexpected( e );
    }

    if( status->state() == TS_FAILED || status->state() == TS_CANCELLED )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_INTERNAL_ERROR );
        e.set_error_message( status->error_message().empty() ? std::string( "DRC task failed" )
                                                             : status->error_message() );
        return tl::unexpected( e );
    }

    std::shared_ptr<API_TASK> task =
            API_TASK_SCHEDULER::Instance().GetFinishedTask( aCtx.Request.token().value() );

    if( !task )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( fmt::format( "no completed task with token {}", aCtx.Request.token().value() ) );
        return tl::unexpected( e );
    }

    auto* drcTask = dynamic_cast<API_HANDLER_PCB::DRC_TASK*>( task.get() );

    if( !drcTask )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "task is not a DRC task" );
        return tl::unexpected( e );
    }

    return drcTask->Result();
}
