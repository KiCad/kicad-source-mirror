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

#include <boost/test/unit_test.hpp>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <api/api_handler_sch.h>
#include <api/api_sch_annotation.h>
#include <api/api_utils.h>
#include <api/common/commands/annotation_commands.pb.h>
#include <api/common/envelope.pb.h>
#include <api/headless_sch_context.h>
#include <api/internal/annotation.pb.h>
#include <schematic_utils/schematic_file_util.h>
#include <settings/settings_manager.h>

#include <sch_reference_list.h>
#include <kiway.h>
#include <sch_sheet.h>
#include <sch_symbol.h>
#include <schematic.h>


namespace
{

struct API_SCH_ANNOTATION_FIXTURE
{
    SETTINGS_MANAGER                      m_settingsManager;
    std::unique_ptr<SCHEMATIC>            m_schematic;
    SCHEMATIC*                            m_schematicSlot = nullptr;
    std::shared_ptr<HEADLESS_SCH_CONTEXT> m_context;

    SCHEMATIC* loadSchematic( const wxString& aRelPath )
    {
        KI_TEST::LoadSchematic( m_settingsManager, aRelPath, m_schematic );
        m_schematicSlot = m_schematic.get();
        m_context = std::make_shared<HEADLESS_SCH_CONTEXT>( &m_schematicSlot,
                                                            &m_settingsManager.Prj() );
        return m_schematic.get();
    }
};

} // namespace


BOOST_FIXTURE_TEST_SUITE( ApiSchAnnotation, API_SCH_ANNOTATION_FIXTURE )

BOOST_AUTO_TEST_CASE( InternalGenerateNetlistReturnsNetlist )
{
    loadSchematic( wxS( "api_kitchen_sink" ) );
    API_HANDLER_SCH handler( m_context );

    kiapi::internal::GenerateNetlistRequest request;
    request.mutable_document()->set_type( kiapi::common::types::DOCTYPE_SCHEMATIC );
    kiapi::common::PackProject( *request.mutable_document()->mutable_project(),
                                m_settingsManager.Prj() );

    kiapi::common::ApiRequest envelope;
    envelope.mutable_message()->PackFrom( request );

    std::string responseText;
    BOOST_REQUIRE( handler.HandleInternalApiRequest( envelope.SerializeAsString(), responseText ) );

    kiapi::common::ApiResponse response;
    BOOST_REQUIRE( response.ParseFromString( responseText ) );
    BOOST_REQUIRE_EQUAL( response.status().status(), kiapi::common::AS_OK );

    kiapi::internal::GenerateNetlistResponse result;
    BOOST_REQUIRE( response.message().UnpackTo( &result ) );
    BOOST_REQUIRE( result.success() );
    BOOST_REQUIRE( !result.netlist().empty() );
    BOOST_REQUIRE( result.netlist().find( "(components" ) != std::string::npos );
}

BOOST_AUTO_TEST_CASE( InternalGenerateNetlistRejectsUnannotatedSchematic )
{
    loadSchematic( wxS( "api_kitchen_sink" ) );
    API_HANDLER_SCH handler( m_context );

    SCH_SHEET_LIST sheets = m_context->GetSchematic()->Hierarchy();
    SCH_REFERENCE_LIST refs;
    sheets.GetSymbols( refs, SYMBOL_FILTER_NON_POWER );
    BOOST_REQUIRE( refs.GetCount() > 0 );

    refs[0].GetSymbol()->SetRef( &refs[0].GetSheetPath(), wxT( "U?" ) );

    kiapi::internal::GenerateNetlistRequest request;
    request.mutable_document()->set_type( kiapi::common::types::DOCTYPE_SCHEMATIC );
    kiapi::common::PackProject( *request.mutable_document()->mutable_project(),
                                m_settingsManager.Prj() );

    kiapi::common::ApiRequest envelope;
    envelope.mutable_message()->PackFrom( request );

    std::string responseText;
    BOOST_REQUIRE( handler.HandleInternalApiRequest( envelope.SerializeAsString(), responseText ) );

    kiapi::common::ApiResponse response;
    BOOST_REQUIRE( response.ParseFromString( responseText ) );
    BOOST_REQUIRE_EQUAL( response.status().status(), kiapi::common::AS_BAD_REQUEST );
}


static std::string MakeBoardNetlist( const SCH_REFERENCE& aRef, const wxString& aValue )
{
    return std::string( "(pcb_netlist\n  (ref \"" )
           + aRef.GetComposedRef().ToUTF8().data() + "\" (fpid \""
           + aRef.GetFootprint().ToUTF8().data() + "\") (value \""
           + aValue.ToUTF8().data() + "\") (timestamp \""
           + aRef.GetFullPath().ToUTF8().data() + "\") (fields))\n)\n";
}


BOOST_AUTO_TEST_CASE( BackAnnotationUpdatesSymbolValue )
{
    loadSchematic( wxS( "api_kitchen_sink" ) );

    SCH_SHEET_LIST sheets = m_context->GetSchematic()->Hierarchy();
    SCH_REFERENCE_LIST refs;
    sheets.GetSymbols( refs, SYMBOL_FILTER_NON_POWER );
    BOOST_REQUIRE( refs.GetCount() > 0 );

    SCH_SYMBOL*    symbol = refs[0].GetSymbol();
    const wxString oldValue = symbol->GetValue( &refs[0].GetSheetPath(), RAW_VALUE );
    BOOST_REQUIRE( oldValue != wxT( "CHANGED" ) );

    kiapi::common::commands::BackAnnotationSettings settings;
    settings.set_reference_mode( kiapi::common::commands::BARM_UPDATE_REFERENCES );
    settings.set_update_values( true );

    WX_STRING_REPORTER reporter;
    ANNOTATION_RESULT  result;

    BOOST_REQUIRE( ApplyBoardNetlistToSchematic(
            m_context.get(), nullptr, MakeBoardNetlist( refs[0], wxT( "CHANGED" ) ), settings,
            result, reporter ) );
    BOOST_REQUIRE_EQUAL( symbol->GetValue( &refs[0].GetSheetPath(), RAW_VALUE ), wxT( "CHANGED" ) );
}


BOOST_AUTO_TEST_CASE( BackAnnotationDryRunDoesNotModify )
{
    loadSchematic( wxS( "api_kitchen_sink" ) );

    SCH_SHEET_LIST sheets = m_context->GetSchematic()->Hierarchy();
    SCH_REFERENCE_LIST refs;
    sheets.GetSymbols( refs, SYMBOL_FILTER_NON_POWER );
    BOOST_REQUIRE( refs.GetCount() > 0 );

    SCH_SYMBOL*    symbol = refs[0].GetSymbol();
    const wxString oldValue = symbol->GetValue( &refs[0].GetSheetPath(), RAW_VALUE );
    BOOST_REQUIRE( oldValue != wxT( "CHANGED" ) );

    kiapi::common::commands::BackAnnotationSettings settings;
    settings.set_reference_mode( kiapi::common::commands::BARM_UPDATE_REFERENCES );
    settings.set_update_values( true );
    settings.set_dry_run( true );

    WX_STRING_REPORTER reporter;
    ANNOTATION_RESULT  result;

    BOOST_REQUIRE( ApplyBoardNetlistToSchematic(
            m_context.get(), nullptr, MakeBoardNetlist( refs[0], wxT( "CHANGED" ) ), settings,
            result, reporter ) );
    BOOST_REQUIRE_EQUAL( symbol->GetValue( &refs[0].GetSheetPath(), RAW_VALUE ), oldValue );
    BOOST_REQUIRE_GT( result.changeCount, 0 );
}


class TEST_BACK_HANDLER : public API_HANDLER_SCH
{
public:
    using API_HANDLER_SCH::API_HANDLER_SCH;

    std::string m_boardNetlist;
    bool        m_boardSuccess = true;
    bool        m_relinkSent = false;
    bool        m_statusOnly = false;
    ApiStatusCode m_statusOnlyStatus = ApiStatusCode::AS_BAD_REQUEST;
    bool        m_relinkStatusOnly = false;
    std::string m_relinkError;
    bool        m_relinkSuccess = true;

protected:
    bool processApiRequest( KIWAY::FACE_T aFace, const std::string& aRequest, std::string& aResponse ) override
    {
        BOOST_REQUIRE_EQUAL( aFace, KIWAY::FACE_PCB );

        ApiRequest envelope;
        BOOST_REQUIRE( envelope.ParseFromString( aRequest ) );

        ApiResponse response;

        if( kiapi::internal::GenerateNetlistRequest netlistRequest; envelope.message().UnpackTo( &netlistRequest ) )
        {
            if( m_statusOnly )
            {
                response.mutable_status()->set_status( m_statusOnlyStatus );
                aResponse = response.SerializeAsString();
                return true;
            }

            kiapi::internal::GenerateNetlistResponse result;
            result.set_success( m_boardSuccess );
            result.set_netlist( m_boardNetlist );

            if( !m_boardSuccess )
                result.set_report( "board netlist unavailable" );

            response.mutable_status()->set_status( m_boardSuccess ? ApiStatusCode::AS_OK
                                                                  : ApiStatusCode::AS_BAD_REQUEST );
            response.mutable_message()->PackFrom( result );
            aResponse = response.SerializeAsString();
            return true;
        }

        if( kiapi::internal::RelinkFootprintsRequest relinkRequest; envelope.message().UnpackTo( &relinkRequest ) )
        {
            m_relinkSent = true;

            if( m_relinkStatusOnly )
            {
                response.mutable_status()->set_status( ApiStatusCode::AS_BAD_REQUEST );
                response.mutable_status()->set_error_message( m_relinkError );
                aResponse = response.SerializeAsString();
                return true;
            }

            kiapi::internal::RelinkFootprintsResponse result;
            result.set_success( m_relinkSuccess );
            result.set_report( m_relinkSuccess ? "relinked" : m_relinkError );

            response.mutable_status()->set_status( ApiStatusCode::AS_OK );
            response.mutable_message()->PackFrom( result );
            aResponse = response.SerializeAsString();
            return true;
        }

        return false;
    }
};


static ApiRequest makeBackAnnotationRequest( const PROJECT& aProject,
                                             const commands::BackAnnotationSettings& aSettings )
{
    commands::UpdateSchematicFromBoard command;
    command.mutable_schematic()->set_type( kiapi::common::types::DOCTYPE_SCHEMATIC );
    PackProject( *command.mutable_schematic()->mutable_project(), aProject );
    command.mutable_board()->set_type( kiapi::common::types::DOCTYPE_PCB );
    command.mutable_board()->set_board_filename( "test.kicad_pcb" );
    *command.mutable_settings() = aSettings;

    ApiRequest request;
    request.mutable_header()->set_client_name( "kicad.qa" );
    BOOST_REQUIRE( request.mutable_message()->PackFrom( command ) );

    return request;
}


BOOST_AUTO_TEST_CASE( BackCommandUpdatesSymbolValue )
{
    loadSchematic( wxS( "api_kitchen_sink" ) );

    SCH_SHEET_LIST     sheets = m_context->GetSchematic()->Hierarchy();
    SCH_REFERENCE_LIST refs;
    sheets.GetSymbols( refs, SYMBOL_FILTER_NON_POWER );
    BOOST_REQUIRE( refs.GetCount() > 0 );

    SCH_SYMBOL*    symbol = refs[0].GetSymbol();
    const wxString oldValue = symbol->GetValue( &refs[0].GetSheetPath(), RAW_VALUE );
    BOOST_REQUIRE( oldValue != wxT( "CHANGED" ) );

    TEST_BACK_HANDLER handler( m_context );
    handler.m_boardNetlist = MakeBoardNetlist( refs[0], wxT( "CHANGED" ) );

    commands::BackAnnotationSettings settings;
    settings.set_reference_mode( commands::BackAnnotationReferenceMode::BARM_UPDATE_REFERENCES );
    settings.set_update_values( true );

    ApiRequest request = makeBackAnnotationRequest( m_settingsManager.Prj(), settings );
    API_RESULT result = handler.Handle( request );

    BOOST_REQUIRE( result.has_value() );
    BOOST_REQUIRE_EQUAL( result->status().status(), ApiStatusCode::AS_OK );

    commands::BackAnnotationResponse response;
    BOOST_REQUIRE( result->message().UnpackTo( &response ) );
    BOOST_REQUIRE_GT( response.change_count(), 0 );

    BOOST_REQUIRE_EQUAL( symbol->GetValue( &refs[0].GetSheetPath(), RAW_VALUE ), wxT( "CHANGED" ) );
    BOOST_REQUIRE( !handler.m_relinkSent );
}


BOOST_AUTO_TEST_CASE( BackCommandDryRunDoesNotRelink )
{
    loadSchematic( wxS( "api_kitchen_sink" ) );

    SCH_SHEET_LIST     sheets = m_context->GetSchematic()->Hierarchy();
    SCH_REFERENCE_LIST refs;
    sheets.GetSymbols( refs, SYMBOL_FILTER_NON_POWER );
    BOOST_REQUIRE( refs.GetCount() > 0 );

    SCH_SYMBOL*    symbol = refs[0].GetSymbol();
    const wxString oldValue = symbol->GetValue( &refs[0].GetSheetPath(), RAW_VALUE );
    BOOST_REQUIRE( oldValue != wxT( "CHANGED" ) );

    TEST_BACK_HANDLER handler( m_context );
    handler.m_boardNetlist = MakeBoardNetlist( refs[0], wxT( "CHANGED" ) );

    commands::BackAnnotationSettings settings;
    settings.set_reference_mode( commands::BackAnnotationReferenceMode::BARM_RELINK_FOOTPRINTS );
    settings.set_update_values( true );
    settings.set_dry_run( true );

    ApiRequest request = makeBackAnnotationRequest( m_settingsManager.Prj(), settings );
    API_RESULT result = handler.Handle( request );

    BOOST_REQUIRE( result.has_value() );
    BOOST_REQUIRE_EQUAL( result->status().status(), ApiStatusCode::AS_OK );
    BOOST_REQUIRE_EQUAL( symbol->GetValue( &refs[0].GetSheetPath(), RAW_VALUE ), oldValue );
    BOOST_REQUIRE( !handler.m_relinkSent );
}


BOOST_AUTO_TEST_CASE( BackCommandRelinksAfterUpdate )
{
    loadSchematic( wxS( "api_kitchen_sink" ) );

    SCH_SHEET_LIST     sheets = m_context->GetSchematic()->Hierarchy();
    SCH_REFERENCE_LIST refs;
    sheets.GetSymbols( refs, SYMBOL_FILTER_NON_POWER );
    BOOST_REQUIRE( refs.GetCount() > 0 );

    SCH_SYMBOL*    symbol = refs[0].GetSymbol();
    const wxString oldValue = symbol->GetValue( &refs[0].GetSheetPath(), RAW_VALUE );
    BOOST_REQUIRE( oldValue != wxT( "CHANGED" ) );

    TEST_BACK_HANDLER handler( m_context );
    handler.m_boardNetlist = MakeBoardNetlist( refs[0], wxT( "CHANGED" ) );

    commands::BackAnnotationSettings settings;
    settings.set_reference_mode( commands::BackAnnotationReferenceMode::BARM_RELINK_FOOTPRINTS );
    settings.set_update_values( true );

    ApiRequest request = makeBackAnnotationRequest( m_settingsManager.Prj(), settings );
    API_RESULT result = handler.Handle( request );

    BOOST_REQUIRE( result.has_value() );
    BOOST_REQUIRE_EQUAL( result->status().status(), ApiStatusCode::AS_OK );
    BOOST_REQUIRE_EQUAL( symbol->GetValue( &refs[0].GetSheetPath(), RAW_VALUE ), wxT( "CHANGED" ) );
    BOOST_REQUIRE( handler.m_relinkSent );
}

BOOST_AUTO_TEST_CASE( BackCommandPropagatesRemoteBadRequest )
{
    loadSchematic( wxS( "api_kitchen_sink" ) );

    SCH_SHEET_LIST     sheets = m_context->GetSchematic()->Hierarchy();
    SCH_REFERENCE_LIST refs;
    sheets.GetSymbols( refs, SYMBOL_FILTER_NON_POWER );
    BOOST_REQUIRE( refs.GetCount() > 0 );

    TEST_BACK_HANDLER handler( m_context );
    handler.m_statusOnly = true;
    handler.m_statusOnlyStatus = ApiStatusCode::AS_BAD_REQUEST;
    handler.m_boardNetlist = MakeBoardNetlist( refs[0], wxT( "CHANGED" ) );

    commands::BackAnnotationSettings settings;
    settings.set_reference_mode( commands::BackAnnotationReferenceMode::BARM_UPDATE_REFERENCES );
    settings.set_update_values( true );

    ApiRequest request = makeBackAnnotationRequest( m_settingsManager.Prj(), settings );
    API_RESULT result = handler.Handle( request );

    BOOST_REQUIRE( !result.has_value() );
    BOOST_REQUIRE_EQUAL( result.error().status(), ApiStatusCode::AS_BAD_REQUEST );
}

BOOST_AUTO_TEST_CASE( BackCommandRelinkFailureIsReported )
{
    loadSchematic( wxS( "api_kitchen_sink" ) );

    SCH_SHEET_LIST     sheets = m_context->GetSchematic()->Hierarchy();
    SCH_REFERENCE_LIST refs;
    sheets.GetSymbols( refs, SYMBOL_FILTER_NON_POWER );
    BOOST_REQUIRE( refs.GetCount() > 0 );

    SCH_SYMBOL*    symbol = refs[0].GetSymbol();
    const wxString oldValue = symbol->GetValue( &refs[0].GetSheetPath(), RAW_VALUE );
    BOOST_REQUIRE( oldValue != wxT( "CHANGED" ) );

    TEST_BACK_HANDLER handler( m_context );
    handler.m_boardNetlist = MakeBoardNetlist( refs[0], wxT( "CHANGED" ) );
    handler.m_relinkSuccess = false;
    handler.m_relinkError = "relink refused";

    commands::BackAnnotationSettings settings;
    settings.set_reference_mode( commands::BackAnnotationReferenceMode::BARM_RELINK_FOOTPRINTS );
    settings.set_update_values( true );

    ApiRequest request = makeBackAnnotationRequest( m_settingsManager.Prj(), settings );
    API_RESULT result = handler.Handle( request );

    BOOST_REQUIRE( result.has_value() );
    BOOST_REQUIRE_EQUAL( result->status().status(), ApiStatusCode::AS_OK );

    commands::BackAnnotationResponse response;
    BOOST_REQUIRE( result->message().UnpackTo( &response ) );
    BOOST_REQUIRE_GE( response.error_count(), 1 );
    BOOST_REQUIRE( response.report().find( "relink refused" ) != std::string::npos );
    BOOST_REQUIRE_EQUAL( symbol->GetValue( &refs[0].GetSheetPath(), RAW_VALUE ), wxT( "CHANGED" ) );
    BOOST_REQUIRE( handler.m_relinkSent );
}

BOOST_AUTO_TEST_CASE( BackCommandRelinkStatusFailureIsReported )
{
    loadSchematic( wxS( "api_kitchen_sink" ) );

    SCH_SHEET_LIST     sheets = m_context->GetSchematic()->Hierarchy();
    SCH_REFERENCE_LIST refs;
    sheets.GetSymbols( refs, SYMBOL_FILTER_NON_POWER );
    BOOST_REQUIRE( refs.GetCount() > 0 );

    SCH_SYMBOL*    symbol = refs[0].GetSymbol();
    const wxString oldValue = symbol->GetValue( &refs[0].GetSheetPath(), RAW_VALUE );
    BOOST_REQUIRE( oldValue != wxT( "CHANGED" ) );

    TEST_BACK_HANDLER handler( m_context );
    handler.m_boardNetlist = MakeBoardNetlist( refs[0], wxT( "CHANGED" ) );
    handler.m_relinkStatusOnly = true;
    handler.m_relinkError = "relink refused";

    commands::BackAnnotationSettings settings;
    settings.set_reference_mode( commands::BackAnnotationReferenceMode::BARM_RELINK_FOOTPRINTS );
    settings.set_update_values( true );

    ApiRequest request = makeBackAnnotationRequest( m_settingsManager.Prj(), settings );
    API_RESULT result = handler.Handle( request );

    BOOST_REQUIRE( result.has_value() );
    BOOST_REQUIRE_EQUAL( result->status().status(), ApiStatusCode::AS_OK );

    commands::BackAnnotationResponse response;
    BOOST_REQUIRE( result->message().UnpackTo( &response ) );
    BOOST_REQUIRE_GE( response.error_count(), 1 );
    BOOST_REQUIRE( response.report().find( "relink refused" ) != std::string::npos );
    BOOST_REQUIRE_EQUAL( symbol->GetValue( &refs[0].GetSheetPath(), RAW_VALUE ), wxT( "CHANGED" ) );
}

BOOST_AUTO_TEST_CASE( InternalNetlistRequestRejectsOpenCommit )
{
    loadSchematic( wxS( "api_kitchen_sink" ) );

    API_HANDLER_SCH handler( m_context );

    types::DocumentSpecifier document;
    document.set_type( types::DOCTYPE_SCHEMATIC );
    PackProject( *document.mutable_project(), m_settingsManager.Prj() );

    commands::BeginCommit begin;
    *begin.mutable_header()->mutable_document() = document;

    ApiRequest beginEnvelope;
    beginEnvelope.mutable_header()->set_client_name( "kicad.qa" );
    beginEnvelope.mutable_message()->PackFrom( begin );

    API_RESULT beginResult = handler.Handle( beginEnvelope );
    BOOST_REQUIRE( beginResult.has_value() );
    BOOST_REQUIRE_EQUAL( beginResult->status().status(), ApiStatusCode::AS_OK );

    kiapi::internal::GenerateNetlistRequest netlistRequest;
    *netlistRequest.mutable_document() = document;

    ApiRequest envelope;
    envelope.mutable_message()->PackFrom( netlistRequest );

    std::string responseText;
    BOOST_REQUIRE( handler.HandleInternalApiRequest( envelope.SerializeAsString(), responseText ) );

    ApiResponse response;
    BOOST_REQUIRE( response.ParseFromString( responseText ) );
    BOOST_REQUIRE_EQUAL( response.status().status(), ApiStatusCode::AS_BUSY );
}

BOOST_AUTO_TEST_SUITE_END()
