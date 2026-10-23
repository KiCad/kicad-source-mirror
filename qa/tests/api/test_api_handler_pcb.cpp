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

#include <memory>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <wx/filename.h>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>

#include <board.h>
#include <connectivity/connectivity_data.h>
#include <drc/drc_exclusion.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <pcb_marker.h>
#include <settings/settings_manager.h>
#include <zone.h>
#include <zone_filler.h>

#include <api/api_handler_pcb.h>
#include <api/api_task_scheduler.h>
#include <api/headless_pcb_context.h>
#include <api/board/board_commands.pb.h>
#include <api/board/board_rules.pb.h>
#include <api/board/board_tasks.pb.h>
#include <api/common/commands/editor_commands.pb.h>
#include <api/common/envelope.pb.h>
#include <api/common/tasks.pb.h>
#include <api/common/types/base_types.pb.h>
#include <jobs/job_export_pcb_odb.h>


namespace
{

/// issue5830 is a four-copper-zone board with stable, human-readable zone UUIDs.
const wxString F_CU_ZONE   = wxS( "00000000-0000-0000-0000-00005c07d704" );
const wxString B_CU_ZONE   = wxS( "00000000-0000-0000-0000-00005c07d701" );
const wxString IN1_CU_ZONE = wxS( "00000000-0000-0000-0000-00005c07d707" );
const wxString IN2_CU_ZONE = wxS( "00000000-0000-0000-0000-00005c07d70a" );


struct API_HANDLER_PCB_FIXTURE
{
    SETTINGS_MANAGER                      m_settingsManager;
    std::unique_ptr<BOARD>                m_board;
    std::shared_ptr<HEADLESS_PCB_CONTEXT> m_context;

    // The context takes ownership of the board; the returned raw pointer lets the test inspect
    // zone state after the handler runs.
    BOARD* loadBoard( const wxString& aRelPath )
    {
        KI_TEST::LoadBoard( m_settingsManager, aRelPath, m_board );

        BOARD* board = m_board.get();
        m_context = std::make_shared<HEADLESS_PCB_CONTEXT>( std::move( m_board ),
                                                            &m_settingsManager.Prj(), nullptr );
        return board;
    }

    kiapi::common::ApiRequest makeRefillRequest( BOARD* aBoard, const std::vector<wxString>& aZoneIds ) const
    {
        kiapi::board::commands::RefillZones command;
        command.mutable_board()->set_type( kiapi::common::types::DocumentType::DOCTYPE_PCB );
        command.mutable_board()->set_board_filename(
                wxFileName( aBoard->GetFileName() ).GetFullName().ToStdString() );

        for( const wxString& id : aZoneIds )
            command.add_zones()->set_value( id.ToStdString() );

        kiapi::common::ApiRequest request;
        request.mutable_header()->set_client_name( "kicad.qa" );
        BOOST_REQUIRE( request.mutable_message()->PackFrom( command ) );

        return request;
    }

    ZONE* zoneByUuid( BOARD* aBoard, const wxString& aUuid ) const
    {
        for( ZONE* zone : aBoard->Zones() )
        {
            if( zone->m_Uuid.AsString() == aUuid )
                return zone;
        }

        return nullptr;
    }

    void unfillAll( BOARD* aBoard ) const
    {
        // Start from a clean slate so a positive IsFilled() result can only come from this fill
        for( ZONE* zone : aBoard->Zones() )
        {
            zone->UnFill();
            BOOST_REQUIRE( !zone->IsFilled() );
        }
    }
};


kiapi::common::ApiRequest makeBeginCommitRequest()
{
    // No header, so the pre-11.0 path assumes the PCB editor
    kiapi::common::commands::BeginCommit command;

    kiapi::common::ApiRequest request;
    request.mutable_header()->set_client_name( "kicad.qa" );
    request.mutable_message()->PackFrom( command );

    return request;
}


kiapi::common::ApiRequest makeRevertRequest( BOARD* aBoard )
{
    kiapi::common::commands::RevertDocument command;
    command.mutable_document()->set_type( kiapi::common::types::DocumentType::DOCTYPE_PCB );
    command.mutable_document()->set_board_filename( wxFileName( aBoard->GetFileName() ).GetFullName().ToStdString() );

    kiapi::common::ApiRequest request;
    request.mutable_header()->set_client_name( "kicad.qa" );
    request.mutable_message()->PackFrom( command );

    return request;
}


kiapi::common::types::DocumentSpecifier makeBoardDocument( const BOARD& aBoard )
{
    kiapi::common::types::DocumentSpecifier document;
    document.set_type( kiapi::common::types::DocumentType::DOCTYPE_PCB );
    document.set_board_filename(
            wxFileName( aBoard.GetFileName() ).GetFullName().ToStdString() );

    return document;
}


kiapi::common::ApiRequest makeStartRefillZonesTaskRequest(
        const BOARD& aBoard, const std::vector<wxString>& aZoneIds = {} )
{
    kiapi::board::tasks::StartRefillZonesTask command;
    *command.mutable_board() = makeBoardDocument( aBoard );

    for( const wxString& id : aZoneIds )
        command.add_zones()->set_value( id.ToStdString() );

    kiapi::common::ApiRequest request;
    request.mutable_header()->set_client_name( "kicad.qa" );
    BOOST_REQUIRE( request.mutable_message()->PackFrom( command ) );

    return request;
}


} // namespace


BOOST_FIXTURE_TEST_SUITE( ApiHandlerPcb, API_HANDLER_PCB_FIXTURE )


BOOST_AUTO_TEST_CASE( OdbJobRequestMapsVariants )
{
    using PACKAGING = JOB_EXPORT_PCB_ODB::VARIANT_PACKAGING;
    kiapi::board::jobs::RunBoardJobExportODB request;
    request.add_variant_names( "var1" );
    request.add_variant_names( "var2" );
    request.set_variant( "legacy" );
    request.set_variant_packaging( kiapi::board::jobs::ODBVP_COMBINED );
    request.set_manufacturer_part_number_column( "MPN" );
    JOB_EXPORT_PCB_ODB job;
    ApplyOdbJobRequest( request, job );
    BOOST_REQUIRE_EQUAL( job.m_variantNames.size(), 2u );
    BOOST_CHECK_EQUAL( job.m_variantNames[0], wxS( "var1" ) );
    BOOST_CHECK_EQUAL( job.m_variantNames[1], wxS( "var2" ) );
    BOOST_CHECK( job.m_variantPackaging == PACKAGING::COMBINED );
    BOOST_CHECK_EQUAL( job.m_colMfgPn, wxString( wxS( "MPN" ) ) );

    kiapi::board::jobs::RunBoardJobExportODB legacyRequest;
    legacyRequest.set_variant( "legacy" );
    JOB_EXPORT_PCB_ODB legacyJob;
    ApplyOdbJobRequest( legacyRequest, legacyJob );
    BOOST_REQUIRE_EQUAL( legacyJob.m_variantNames.size(), 1u );
    BOOST_CHECK_EQUAL( legacyJob.m_variantNames[0], wxS( "legacy" ) );
    BOOST_CHECK( legacyJob.m_variantPackaging == PACKAGING::SEPARATE );
}


BOOST_AUTO_TEST_CASE( OdbJobRequestMapsExportOptions )
{
    kiapi::board::jobs::RunBoardJobExportODB request;
    request.set_origin( kiapi::board::jobs::ODBO_GRID );
    request.set_product_name( "board product" );
    request.mutable_content()->set_data_set( kiapi::board::jobs::FDS_ASSEMBLY );
    request.mutable_content()->set_sections( "KP" );
    request.mutable_content()->set_net_names( kiapi::board::jobs::FNNP_ANONYMIZE );

    JOB_EXPORT_PCB_ODB job;
    ApplyOdbJobRequest( request, job );
    BOOST_CHECK( job.m_origin == JOB_EXPORT_PCB_ODB::ORIGIN::GRID );
    BOOST_CHECK_EQUAL( job.m_productName, wxString( wxS( "board product" ) ) );
    BOOST_CHECK( job.m_dataSet == JOB_EXPORT_PCB_FAB::DATA_SET::ASSEMBLY );
    BOOST_REQUIRE( job.m_sections );
    BOOST_CHECK_EQUAL( *job.m_sections, wxString( wxS( "KP" ) ) );
    BOOST_CHECK( job.m_netNames == JOB_EXPORT_PCB_FAB::NET_NAMES::ANONYMIZE );

    kiapi::board::jobs::RunBoardJobExportODB legacy;
    JOB_EXPORT_PCB_ODB                       defaultJob;
    ApplyOdbJobRequest( legacy, defaultJob );
    BOOST_CHECK( defaultJob.m_origin == JOB_EXPORT_PCB_ODB::ORIGIN::ABSOLUTE_COORDS );
    BOOST_CHECK( defaultJob.m_dataSet == JOB_EXPORT_PCB_FAB::DATA_SET::USERDEF );
    BOOST_CHECK( defaultJob.m_netNames == JOB_EXPORT_PCB_FAB::NET_NAMES::INCLUDE );
}


BOOST_AUTO_TEST_CASE( RefillZonesSubset )
{
    BOARD* board = loadBoard( wxS( "issue5830" ) );

    unfillAll( board );

    API_HANDLER_PCB           handler( m_context );
    kiapi::common::ApiRequest request = makeRefillRequest( board, { F_CU_ZONE, IN1_CU_ZONE } );
    API_RESULT                result = handler.Handle( request );

    if( !result.has_value() )
    {
        BOOST_FAIL( "RefillZones returned status " << result.error().status() << ": "
                                                    << result.error().error_message() );
    }

    BOOST_CHECK_EQUAL( result->status().status(), kiapi::common::ApiStatusCode::AS_OK );

    ZONE* fCu   = zoneByUuid( board, F_CU_ZONE );
    ZONE* bCu   = zoneByUuid( board, B_CU_ZONE );
    ZONE* in1Cu = zoneByUuid( board, IN1_CU_ZONE );
    ZONE* in2Cu = zoneByUuid( board, IN2_CU_ZONE );

    BOOST_REQUIRE( fCu && bCu && in1Cu && in2Cu );

    // Exactly the requested zones must be filled; the others must be untouched.
    BOOST_CHECK( fCu->IsFilled() );
    BOOST_CHECK( in1Cu->IsFilled() );
    BOOST_CHECK( !bCu->IsFilled() );
    BOOST_CHECK( !in2Cu->IsFilled() );
}


BOOST_AUTO_TEST_CASE( RefillZonesSubsetRebuildsConnectivity )
{
    BOARD* board = loadBoard( wxS( "issue5830" ) );

    unfillAll( board );

    // Baseline ratsnest with every zone empty; the GND planes are unfilled so their pads still
    // ratsnest together.
    board->BuildConnectivity();
    const unsigned baseline = board->GetConnectivity()->GetUnconnectedCount( false );
    BOOST_REQUIRE_MESSAGE( baseline > 0, "expected an unconnected baseline with zones empty" );

    API_HANDLER_PCB           handler( m_context );
    kiapi::common::ApiRequest request = makeRefillRequest( board, { F_CU_ZONE, IN1_CU_ZONE } );
    API_RESULT                result = handler.Handle( request );

    if( !result.has_value() )
    {
        BOOST_FAIL( "RefillZones returned status " << result.error().status() << ": "
                                                    << result.error().error_message() );
    }

    const unsigned afterFill = board->GetConnectivity()->GetUnconnectedCount( false );

    // Filling the GND planes bridges GND pads that previously ratsnested, so the unconnected
    // count drops.  Push cleared the ratsnest, so if the handler skipped the connectivity
    // rebuild this would read zero instead of the reduced-but-nonzero count.
    BOOST_CHECK_MESSAGE( afterFill > 0, "connectivity was cleared, not rebuilt, after the fill" );
    BOOST_CHECK_MESSAGE( afterFill < baseline,
                         "filling the GND planes should reduce the unconnected count ("
                                 << afterFill << " vs baseline " << baseline << ")" );
}


BOOST_AUTO_TEST_CASE( RefillZonesAllHeadless )
{
    BOARD* board = loadBoard( wxS( "issue5830" ) );

    unfillAll( board );

    API_HANDLER_PCB           handler( m_context );
    kiapi::common::ApiRequest request = makeRefillRequest( board, {} );
    API_RESULT                result = handler.Handle( request );

    if( !result.has_value() )
    {
        BOOST_FAIL( "RefillZones returned status " << result.error().status() << ": "
                                                    << result.error().error_message() );
    }

    BOOST_CHECK_EQUAL( result->status().status(), kiapi::common::ApiStatusCode::AS_OK );

    // With no frame the empty-zones request must fill everything synchronously
    for( ZONE* zone : board->Zones() )
        BOOST_CHECK_MESSAGE( zone->IsFilled(), "zone " << zone->m_Uuid.AsStdString() << " not filled" );
}


BOOST_AUTO_TEST_CASE( RefillZonesUnknownIdRejected )
{
    BOARD* board = loadBoard( wxS( "issue5830" ) );

    API_HANDLER_PCB           handler( m_context );
    kiapi::common::ApiRequest request =
            makeRefillRequest( board, { wxS( "deadbeef-0000-0000-0000-000000000000" ) } );
    API_RESULT                result = handler.Handle( request );

    BOOST_REQUIRE( !result.has_value() );
    BOOST_CHECK_EQUAL( result.error().status(), kiapi::common::ApiStatusCode::AS_BAD_REQUEST );
}


// RevertDocument reloads the board, freeing every item an in-flight client commit points at.
// The handler must refuse with AS_BUSY while a commit is open, or the pointers dangle
// (use-after-free). Latent UAF found via #24803.
BOOST_AUTO_TEST_CASE( RevertDocumentRejectedWithOpenCommit )
{
    BOARD* board = loadBoard( wxS( "issue5830" ) );

    API_HANDLER_PCB handler( m_context );

    // Open a client transaction, as a client staging edits would
    kiapi::common::ApiRequest beginRequest = makeBeginCommitRequest();
    BOOST_REQUIRE( handler.Handle( beginRequest ).has_value() );

    kiapi::common::ApiRequest request = makeRevertRequest( board );
    API_RESULT                result = handler.Handle( request );

    BOOST_REQUIRE( !result.has_value() );
    BOOST_CHECK_EQUAL( result.error().status(), kiapi::common::ApiStatusCode::AS_BUSY );
    BOOST_CHECK( result.error().error_message().find( "commit" ) != std::string::npos );
}


// With no open commit the guard passes; the reload then needs a running editor, so a headless
// handler reports AS_UNIMPLEMENTED. This confirms the guard does not reject the normal path.
BOOST_AUTO_TEST_CASE( RevertDocumentWithoutCommitPassesGuard )
{
    BOARD* board = loadBoard( wxS( "issue5830" ) );

    API_HANDLER_PCB           handler( m_context );
    kiapi::common::ApiRequest request = makeRevertRequest( board );
    API_RESULT                result = handler.Handle( request );

    BOOST_REQUIRE( !result.has_value() );
    BOOST_CHECK_EQUAL( result.error().status(), kiapi::common::ApiStatusCode::AS_BAD_REQUEST );
}


/*
 * Long-running API task: StartRefillZonesTask must fill the requested zones via the task
 * scheduler and report COMPLETED.
 */

BOOST_AUTO_TEST_CASE( StartRefillZonesTaskCompletes )
{
    BOARD* board = loadBoard( wxS( "issue5830" ) );

    unfillAll( board );

    API_HANDLER_PCB handler( m_context );

    kiapi::board::tasks::StartRefillZonesTask command;
    command.mutable_board()->set_type( kiapi::common::types::DocumentType::DOCTYPE_PCB );
    command.mutable_board()->set_board_filename(
            wxFileName( board->GetFileName() ).GetFullName().ToStdString() );
    command.add_zones()->set_value( F_CU_ZONE.ToStdString() );
    command.add_zones()->set_value( IN1_CU_ZONE.ToStdString() );

    kiapi::common::ApiRequest request;
    request.mutable_header()->set_client_name( "kicad.qa" );
    BOOST_REQUIRE( request.mutable_message()->PackFrom( command ) );

    API_RESULT result = handler.Handle( request );

    BOOST_REQUIRE( result.has_value() );

    kiapi::common::tasks::StartTaskResponse response;
    BOOST_REQUIRE( result->message().UnpackTo( &response ) );
    BOOST_REQUIRE( !response.token().value().empty() );

    // No event loop here: drain the scheduler synchronously until the task finishes
    API_TASK_SCHEDULER::Instance().Pump();

    auto status = API_TASK_SCHEDULER::Instance().GetStatus( response.token().value() );
    BOOST_REQUIRE( status.has_value() );
    BOOST_CHECK_EQUAL( status->state(), kiapi::common::tasks::TS_COMPLETED );

    ZONE* fCu   = zoneByUuid( board, F_CU_ZONE );
    ZONE* in1Cu = zoneByUuid( board, IN1_CU_ZONE );
    ZONE* bCu   = zoneByUuid( board, B_CU_ZONE );

    BOOST_REQUIRE( fCu && in1Cu && bCu );
    BOOST_CHECK( fCu->IsFilled() );
    BOOST_CHECK( in1Cu->IsFilled() );
    BOOST_CHECK( !bCu->IsFilled() );
}


BOOST_AUTO_TEST_CASE( StartRefillZonesTaskUnknownIdRejected )
{
    BOARD* board = loadBoard( wxS( "issue5830" ) );

    API_HANDLER_PCB handler( m_context );

    kiapi::board::tasks::StartRefillZonesTask command;
    command.mutable_board()->set_type( kiapi::common::types::DocumentType::DOCTYPE_PCB );
    command.mutable_board()->set_board_filename(
            wxFileName( board->GetFileName() ).GetFullName().ToStdString() );
    command.add_zones()->set_value( "deadbeef-0000-0000-0000-000000000000" );

    kiapi::common::ApiRequest request;
    request.mutable_header()->set_client_name( "kicad.qa" );
    BOOST_REQUIRE( request.mutable_message()->PackFrom( command ) );

    API_RESULT result = handler.Handle( request );

    // Validation happens at request time, before any task starts
    BOOST_REQUIRE( !result.has_value() );
    BOOST_CHECK_EQUAL( result.error().status(), kiapi::common::ApiStatusCode::AS_BAD_REQUEST );
}


BOOST_AUTO_TEST_CASE( StartRefillZonesTaskEmptyListFillsAllZones )
{
    BOARD* board = loadBoard( wxS( "issue5830" ) );

    unfillAll( board );

    API_HANDLER_PCB handler( m_context );

    // An empty zone list means "all zones", matching the synchronous RefillZones command
    kiapi::common::ApiRequest request = makeStartRefillZonesTaskRequest( *board );
    API_RESULT result = handler.Handle( request );
    BOOST_REQUIRE( result.has_value() );

    kiapi::common::tasks::StartTaskResponse response;
    BOOST_REQUIRE( result->message().UnpackTo( &response ) );
    BOOST_REQUIRE( !response.token().value().empty() );

    API_TASK_SCHEDULER::Instance().Pump();

    auto status = API_TASK_SCHEDULER::Instance().GetStatus( response.token().value() );
    BOOST_REQUIRE( status.has_value() );
    BOOST_CHECK_EQUAL( status->state(), kiapi::common::tasks::TS_COMPLETED );

    for( ZONE* zone : board->Zones() )
    {
        if( !zone->GetIsRuleArea() )
            BOOST_CHECK_MESSAGE( zone->IsFilled(), "zone " << zone->GetZoneName() << " was not filled" );
    }

    // Headless contexts track modification themselves; the task must report it like the
    // synchronous command so clients know the board needs saving.
    kiapi::common::commands::GetDocumentModifiedState stateCommand;
    *stateCommand.mutable_document() = makeBoardDocument( *board );

    kiapi::common::ApiRequest stateRequest;
    stateRequest.mutable_header()->set_client_name( "kicad.qa" );
    BOOST_REQUIRE( stateRequest.mutable_message()->PackFrom( stateCommand ) );

    API_RESULT stateResult = handler.Handle( stateRequest );
    BOOST_REQUIRE( stateResult.has_value() );

    kiapi::common::commands::GetDocumentModifiedStateResponse state;
    BOOST_REQUIRE( stateResult->message().UnpackTo( &state ) );
    BOOST_CHECK_EQUAL( state.state(), kiapi::common::commands::DMS_MODIFIED );
}


BOOST_AUTO_TEST_CASE( DrcExclusionDropsMarkerReportFields )
{
    BOARD* board = loadBoard( wxS( "issue5830" ) );

    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_CLEARANCE );
    drcItem->SetItems( KIID( "12345678-1234-1234-1234-123456789abc" ),
                       KIID( "87654321-4321-4321-4321-cba987654321" ) );

    DRC_RULE rule( wxS( "custom clearance" ) );
    drcItem->SetViolatingRule( &rule );

    PCB_MARKER* marker = new PCB_MARKER( drcItem, VECTOR2I( 1000, 1000 ) );
    board->Add( marker );
    marker->SetExcluded( true, "Test exclusion comment" );

    kiapi::board::DrcMarker msg;
    PCB_MARKER::ToProto( msg, *marker );
    BOOST_CHECK_EQUAL( msg.violating_rule(), "custom clearance" );
    BOOST_CHECK( !msg.description().empty() );
    BOOST_CHECK( msg.excluded() );

    DRC_EXCLUSION exclusion = DRC_EXCLUSION::FromMarker( *marker );
    const kiapi::board::DrcMarker& canonical = exclusion.ToProto().marker();

    BOOST_CHECK( canonical.violating_rule().empty() );
    BOOST_CHECK( canonical.description().empty() );
    BOOST_CHECK_EQUAL( canonical.severity(), kiapi::common::types::RuleSeverity::RS_UNKNOWN );
    BOOST_CHECK( !canonical.excluded() );

    // Canonicalization is stable across a proto round trip
    DRC_EXCLUSION roundTripped = DRC_EXCLUSION::FromProto( exclusion.ToProto() );
    BOOST_CHECK( roundTripped == exclusion );

    const kiapi::board::DrcMarker& roundTrippedMsg = roundTripped.ToProto().marker();
    BOOST_CHECK( roundTrippedMsg.violating_rule().empty() );
    BOOST_CHECK( roundTrippedMsg.description().empty() );
    BOOST_CHECK( !roundTrippedMsg.excluded() );
}


BOOST_AUTO_TEST_SUITE_END()
