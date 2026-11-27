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

#include <algorithm>
#include <memory>

#include <boost/test/unit_test.hpp>

#include <wx/filename.h>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>

#include <api/api_handler_pcb.h>
#include <api/api_pcb_annotation.h>
#include <api/api_utils.h>
#include <api/common/commands/annotation_commands.pb.h>
#include <api/common/envelope.pb.h>
#include <api/headless_pcb_context.h>
#include <api/internal/annotation.pb.h>

#include <board.h>
#include <footprint.h>
#include <kiway.h>
#include <reporter.h>
#include <settings/settings_manager.h>


namespace
{

struct API_PCB_ANNOTATION_FIXTURE
{
    SETTINGS_MANAGER                      m_settingsManager;
    std::unique_ptr<BOARD>                m_board;
    std::shared_ptr<HEADLESS_PCB_CONTEXT> m_context;

    BOARD* loadBoard( const wxString& aRelPath )
    {
        KI_TEST::LoadBoard( m_settingsManager, aRelPath, m_board );

        BOARD* board = m_board.get();
        m_context = std::make_shared<HEADLESS_PCB_CONTEXT>( std::move( m_board ),
                                                            &m_settingsManager.Prj(), nullptr );
        return board;
    }
};


kiapi::internal::GenerateNetlistRequest makeNetlistRequest( const BOARD& aBoard )
{
    kiapi::internal::GenerateNetlistRequest request;
    request.mutable_document()->set_type( kiapi::common::types::DOCTYPE_PCB );
    request.mutable_document()->set_board_filename(
            wxFileName( aBoard.GetFileName() ).GetFullName().ToStdString() );

    return request;
}


std::string makeSchematicNetlist( BOARD& aBoard, const wxString& aOverrideRef = wxEmptyString,
                                  const wxString& aOverrideValue = wxEmptyString )
{
    std::string netlist = "(export (version \"E\")\n  (components\n";

    for( FOOTPRINT* footprint : aBoard.Footprints() )
    {
        if( footprint->GetAttributes() & FP_BOARD_ONLY )
            continue;

        wxString value = footprint->GetValue();

        if( footprint->GetReference() == aOverrideRef )
            value = aOverrideValue;

        netlist += "    (comp (ref \"" + std::string( footprint->GetReference().ToUTF8() )
                   + "\") (value \"" + std::string( value.ToUTF8() )
                   + "\") (footprint \"" + std::string( footprint->GetFPID().GetUniStringLibId().ToUTF8() )
                   + "\"))\n";
    }

    netlist += "  )\n)\n";

    return netlist;
}


bool boardHasRef( BOARD& aBoard, const wxString& aRef )
{
    return std::any_of( aBoard.Footprints().begin(), aBoard.Footprints().end(),
                        [&]( const FOOTPRINT* aFootprint )
                        {
                            return aFootprint->GetReference() == aRef;
                        } );
}

} // namespace


class TEST_FORWARD_HANDLER : public API_HANDLER_PCB
{
public:
    using API_HANDLER_PCB::API_HANDLER_PCB;

    std::string m_netlist;
    bool        m_schematicSuccess = true;
    bool        m_statusOnly = false;
    kiapi::common::ApiStatusCode m_statusOnlyStatus = kiapi::common::AS_BUSY;

protected:
    bool processApiRequest( KIWAY::FACE_T aFace, const std::string&, std::string& aResponse ) override
    {
        BOOST_REQUIRE_EQUAL( aFace, KIWAY::FACE_SCH );

        kiapi::common::ApiResponse response;

        if( m_statusOnly )
        {
            response.mutable_status()->set_status( m_statusOnlyStatus );
            aResponse = response.SerializeAsString();
            return true;
        }

        kiapi::internal::GenerateNetlistResponse result;
        result.set_success( m_schematicSuccess );
        result.set_netlist( m_netlist );

        if( !m_schematicSuccess )
            result.set_report( "schematic is not annotated" );

        response.mutable_status()->set_status( m_schematicSuccess ? kiapi::common::AS_OK
                                                                  : kiapi::common::AS_BAD_REQUEST );
        response.mutable_message()->PackFrom( result );
        aResponse = response.SerializeAsString();
        return true;
    }
};


kiapi::common::ApiRequest makeForwardRequest(
        const BOARD& aBoard,
        const kiapi::common::commands::ForwardAnnotationSettings& aSettings )
{
    kiapi::common::commands::UpdateBoardFromSchematic command;
    command.mutable_board()->set_type( kiapi::common::types::DOCTYPE_PCB );
    command.mutable_board()->set_board_filename(
            wxFileName( aBoard.GetFileName() ).GetFullName().ToStdString() );
    command.mutable_schematic()->set_type( kiapi::common::types::DOCTYPE_SCHEMATIC );
    *command.mutable_settings() = aSettings;

    kiapi::common::ApiRequest request;
    request.mutable_header()->set_client_name( "kicad.qa" );
    BOOST_REQUIRE( request.mutable_message()->PackFrom( command ) );

    return request;
}


BOOST_FIXTURE_TEST_SUITE( ApiPcbAnnotation, API_PCB_ANNOTATION_FIXTURE )

BOOST_AUTO_TEST_CASE( InternalGenerateNetlistReturnsBoardNetlist )
{
    BOARD* board = loadBoard( wxS( "api_kitchen_sink" ) );

    API_HANDLER_PCB handler( m_context, nullptr );

    kiapi::common::ApiRequest envelope;
    envelope.mutable_message()->PackFrom( makeNetlistRequest( *board ) );

    std::string responseText;
    BOOST_REQUIRE( handler.HandleInternalApiRequest( envelope.SerializeAsString(), responseText ) );

    kiapi::common::ApiResponse response;
    BOOST_REQUIRE( response.ParseFromString( responseText ) );
    BOOST_REQUIRE_EQUAL( response.status().status(), kiapi::common::AS_OK );

    kiapi::internal::GenerateNetlistResponse result;
    BOOST_REQUIRE( response.message().UnpackTo( &result ) );
    BOOST_REQUIRE( result.success() );
    BOOST_REQUIRE( result.netlist().find( "(pcb_netlist" ) != std::string::npos );
}

BOOST_AUTO_TEST_CASE( InternalRelinkFootprintsSucceeds )
{
    BOARD* board = loadBoard( wxS( "api_kitchen_sink" ) );

    const std::string netlistText = makeSchematicNetlist( *board );

    API_HANDLER_PCB handler( m_context, nullptr );

    kiapi::internal::RelinkFootprintsRequest request;
    *request.mutable_board() = makeNetlistRequest( *board ).document();
    request.set_schematic_netlist( netlistText );

    kiapi::common::ApiRequest envelope;
    envelope.mutable_message()->PackFrom( request );

    std::string responseText;
    BOOST_REQUIRE( handler.HandleInternalApiRequest( envelope.SerializeAsString(), responseText ) );

    kiapi::common::ApiResponse response;
    BOOST_REQUIRE( response.ParseFromString( responseText ) );
    BOOST_REQUIRE_EQUAL( response.status().status(), kiapi::common::AS_OK );

    kiapi::internal::RelinkFootprintsResponse result;
    BOOST_REQUIRE( response.message().UnpackTo( &result ) );
    BOOST_REQUIRE( result.success() );
}

BOOST_AUTO_TEST_CASE( ForwardApplyUpdatesFootprintValue )
{
    BOARD* board = loadBoard( wxS( "api_kitchen_sink" ) );
    BOOST_REQUIRE( !board->Footprints().empty() );

    FOOTPRINT*     footprint = board->Footprints().front();
    const wxString oldValue = footprint->GetValue();
    BOOST_REQUIRE( oldValue != wxT( "CHANGED" ) );

    const std::string netlist = makeSchematicNetlist( *board, footprint->GetReference(),
                                                      wxT( "CHANGED" ) );

    kiapi::common::commands::ForwardAnnotationSettings settings;
    settings.set_match_mode( kiapi::common::commands::AMM_REFERENCE );
    settings.set_update_fields( true );

    WX_STRING_REPORTER reporter;
    ANNOTATION_RESULT  result;

    BOOST_REQUIRE( ApplySchematicNetlist( *m_context, netlist, settings, result, reporter ) );
    BOOST_REQUIRE_EQUAL( footprint->GetValue(), wxT( "CHANGED" ) );
    BOOST_REQUIRE( m_context->IsContentModified() );
}

BOOST_AUTO_TEST_CASE( ForwardApplyDryRunDoesNotModify )
{
    BOARD* board = loadBoard( wxS( "api_kitchen_sink" ) );
    BOOST_REQUIRE( !board->Footprints().empty() );

    FOOTPRINT*     footprint = board->Footprints().front();
    const wxString oldValue = footprint->GetValue();
    BOOST_REQUIRE( oldValue != wxT( "CHANGED" ) );

    const std::string netlist = makeSchematicNetlist( *board, footprint->GetReference(),
                                                      wxT( "CHANGED" ) );

    kiapi::common::commands::ForwardAnnotationSettings settings;
    settings.set_match_mode( kiapi::common::commands::AMM_REFERENCE );
    settings.set_update_fields( true );
    settings.set_dry_run( true );

    WX_STRING_REPORTER reporter;
    ANNOTATION_RESULT  result;

    BOOST_REQUIRE( ApplySchematicNetlist( *m_context, netlist, settings, result, reporter ) );
    BOOST_REQUIRE_EQUAL( footprint->GetValue(), oldValue );
    BOOST_REQUIRE( !m_context->IsContentModified() );
}

BOOST_AUTO_TEST_CASE( DeleteExtraFootprintsRemovesOnlyUnlockedUnusedFootprints )
{
    BOARD* board = loadBoard( wxS( "api_kitchen_sink" ) );

    const std::string netlistText = makeSchematicNetlist( *board );

    FOOTPRINT* unlocked = new FOOTPRINT( board );
    unlocked->SetReference( wxT( "EXTRA_UNLOCKED" ) );
    unlocked->SetValue( wxT( "EXTRA" ) );
    board->Add( unlocked );

    FOOTPRINT* locked = new FOOTPRINT( board );
    locked->SetReference( wxT( "EXTRA_LOCKED" ) );
    locked->SetValue( wxT( "EXTRA" ) );
    locked->SetLocked( true );
    board->Add( locked );

    kiapi::common::commands::ForwardAnnotationSettings settings;
    settings.set_match_mode( kiapi::common::commands::AMM_REFERENCE );
    settings.set_delete_extra_footprints( true );
    settings.set_override_locks( false );

    WX_STRING_REPORTER reporter;
    ANNOTATION_RESULT  result;

    BOOST_REQUIRE( ApplySchematicNetlist( *m_context, netlistText, settings, result, reporter ) );
    BOOST_REQUIRE( !boardHasRef( *board, wxT( "EXTRA_UNLOCKED" ) ) );
    BOOST_REQUIRE( boardHasRef( *board, wxT( "EXTRA_LOCKED" ) ) );
    BOOST_REQUIRE_GE( result.warningCount, 1 );

    settings.set_override_locks( true );
    BOOST_REQUIRE( ApplySchematicNetlist( *m_context, netlistText, settings, result, reporter ) );
    BOOST_REQUIRE( !boardHasRef( *board, wxT( "EXTRA_LOCKED" ) ) );
}


BOOST_AUTO_TEST_CASE( ForwardCommandUpdatesFootprintValue )
{
    BOARD* board = loadBoard( wxS( "api_kitchen_sink" ) );
    BOOST_REQUIRE( !board->Footprints().empty() );

    FOOTPRINT*     footprint = board->Footprints().front();
    const wxString oldValue = footprint->GetValue();
    BOOST_REQUIRE( oldValue != wxT( "CHANGED" ) );

    TEST_FORWARD_HANDLER handler( m_context, nullptr );
    handler.m_netlist = makeSchematicNetlist( *board, footprint->GetReference(),
                                              wxT( "CHANGED" ) );

    kiapi::common::commands::ForwardAnnotationSettings settings;
    settings.set_match_mode( kiapi::common::commands::AMM_REFERENCE );
    settings.set_update_fields( true );

    kiapi::common::ApiRequest request = makeForwardRequest( *board, settings );
    API_RESULT result = handler.Handle( request );

    BOOST_REQUIRE( result.has_value() );
    BOOST_REQUIRE_EQUAL( result->status().status(), kiapi::common::AS_OK );
    BOOST_REQUIRE_EQUAL( footprint->GetValue(), wxT( "CHANGED" ) );
}


BOOST_AUTO_TEST_CASE( ForwardCommandDryRunDoesNotModify )
{
    BOARD* board = loadBoard( wxS( "api_kitchen_sink" ) );
    BOOST_REQUIRE( !board->Footprints().empty() );

    FOOTPRINT*     footprint = board->Footprints().front();
    const wxString oldValue = footprint->GetValue();
    BOOST_REQUIRE( oldValue != wxT( "CHANGED" ) );

    TEST_FORWARD_HANDLER handler( m_context, nullptr );
    handler.m_netlist = makeSchematicNetlist( *board, footprint->GetReference(),
                                              wxT( "CHANGED" ) );

    kiapi::common::commands::ForwardAnnotationSettings settings;
    settings.set_match_mode( kiapi::common::commands::AMM_REFERENCE );
    settings.set_update_fields( true );
    settings.set_dry_run( true );

    kiapi::common::ApiRequest request = makeForwardRequest( *board, settings );
    API_RESULT result = handler.Handle( request );

    BOOST_REQUIRE( result.has_value() );
    BOOST_REQUIRE_EQUAL( result->status().status(), kiapi::common::AS_OK );
    BOOST_REQUIRE_EQUAL( footprint->GetValue(), oldValue );
    BOOST_REQUIRE( !m_context->IsContentModified() );
}


BOOST_AUTO_TEST_CASE( ForwardCommandRejectsFailedSchematic )
{
    BOARD* board = loadBoard( wxS( "api_kitchen_sink" ) );
    BOOST_REQUIRE( !board->Footprints().empty() );

    FOOTPRINT*     footprint = board->Footprints().front();
    const wxString oldValue = footprint->GetValue();

    TEST_FORWARD_HANDLER handler( m_context, nullptr );
    handler.m_schematicSuccess = false;
    handler.m_netlist = makeSchematicNetlist( *board, footprint->GetReference(),
                                              wxT( "CHANGED" ) );

    kiapi::common::commands::ForwardAnnotationSettings settings;
    settings.set_match_mode( kiapi::common::commands::AMM_REFERENCE );
    settings.set_update_fields( true );

    kiapi::common::ApiRequest request = makeForwardRequest( *board, settings );
    API_RESULT result = handler.Handle( request );

    BOOST_REQUIRE( !result.has_value() );
    BOOST_REQUIRE_EQUAL( result.error().status(), kiapi::common::AS_BAD_REQUEST );
    BOOST_REQUIRE_EQUAL( footprint->GetValue(), oldValue );
}

BOOST_AUTO_TEST_CASE( ForwardCommandPropagatesRemoteBusy )
{
    BOARD* board = loadBoard( wxS( "api_kitchen_sink" ) );
    BOOST_REQUIRE( !board->Footprints().empty() );

    TEST_FORWARD_HANDLER handler( m_context, nullptr );
    handler.m_statusOnly = true;
    handler.m_statusOnlyStatus = kiapi::common::AS_BUSY;
    handler.m_netlist = makeSchematicNetlist( *board );

    kiapi::common::commands::ForwardAnnotationSettings settings;
    settings.set_match_mode( kiapi::common::commands::AMM_REFERENCE );

    kiapi::common::ApiRequest request = makeForwardRequest( *board, settings );
    API_RESULT result = handler.Handle( request );

    BOOST_REQUIRE( !result.has_value() );
    BOOST_REQUIRE_EQUAL( result.error().status(), kiapi::common::AS_BUSY );
}

BOOST_AUTO_TEST_CASE( InternalNetlistRequestRejectsOpenCommit )
{
    BOARD* board = loadBoard( wxS( "api_kitchen_sink" ) );

    API_HANDLER_PCB handler( m_context, nullptr );

    kiapi::common::commands::BeginCommit begin;
    *begin.mutable_header()->mutable_document() = makeNetlistRequest( *board ).document();

    kiapi::common::ApiRequest beginEnvelope;
    beginEnvelope.mutable_header()->set_client_name( "kicad.qa" );
    beginEnvelope.mutable_message()->PackFrom( begin );

    API_RESULT beginResult = handler.Handle( beginEnvelope );
    BOOST_REQUIRE( beginResult.has_value() );
    BOOST_REQUIRE_EQUAL( beginResult->status().status(), kiapi::common::AS_OK );

    kiapi::common::ApiRequest envelope;
    envelope.mutable_message()->PackFrom( makeNetlistRequest( *board ) );

    std::string responseText;
    BOOST_REQUIRE( handler.HandleInternalApiRequest( envelope.SerializeAsString(), responseText ) );

    kiapi::common::ApiResponse response;
    BOOST_REQUIRE( response.ParseFromString( responseText ) );
    BOOST_REQUIRE_EQUAL( response.status().status(), kiapi::common::AS_BUSY );
}

BOOST_AUTO_TEST_SUITE_END()
