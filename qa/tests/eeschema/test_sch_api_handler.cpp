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

#include <api/api_handler_sch.h>
#include <api/api_utils.h>
#include <api/common/commands/editor_commands.pb.h>
#include <api/common/envelope.pb.h>
#include <api/headless_sch_context.h>
#include <api/schematic/schematic_types.pb.h>
#include <schematic_utils/schematic_file_util.h>
#include <settings/settings_manager.h>

#include <schematic.h>


namespace
{

struct API_HANDLER_SCH_FIXTURE
{
    SETTINGS_MANAGER                      m_settingsManager;
    std::unique_ptr<SCHEMATIC>            m_schematic;
    std::shared_ptr<HEADLESS_SCH_CONTEXT> m_context;

    SCHEMATIC* loadSchematic( const wxString& aRelPath )
    {
        KI_TEST::LoadSchematic( m_settingsManager, aRelPath, m_schematic );
        m_context = std::make_shared<HEADLESS_SCH_CONTEXT>( m_schematic.get(),
                                                            &m_settingsManager.Prj() );
        return m_schematic.get();
    }
};


kiapi::common::types::DocumentSpecifier makeDocument( const SCHEMATIC& aSchematic )
{
    kiapi::common::types::DocumentSpecifier document;
    document.set_type( kiapi::common::types::DocumentType::DOCTYPE_SCHEMATIC );
    kiapi::common::PackProject( *document.mutable_project(), aSchematic.Project() );

    return document;
}


kiapi::common::ApiRequest makeBeginCommitRequest( const SCHEMATIC& aSchematic )
{
    kiapi::common::commands::BeginCommit command;
    *command.mutable_header()->mutable_document() = makeDocument( aSchematic );

    kiapi::common::ApiRequest request;
    request.mutable_header()->set_client_name( "kicad.qa" );
    BOOST_REQUIRE( request.mutable_message()->PackFrom( command ) );

    return request;
}


kiapi::common::ApiRequest makeRevertRequest( const SCHEMATIC& aSchematic )
{
    kiapi::common::commands::RevertDocument command;
    *command.mutable_document() = makeDocument( aSchematic );

    kiapi::common::ApiRequest request;
    request.mutable_header()->set_client_name( "kicad.qa" );
    BOOST_REQUIRE( request.mutable_message()->PackFrom( command ) );

    return request;
}


}


BOOST_FIXTURE_TEST_SUITE( ApiHandlerSch, API_HANDLER_SCH_FIXTURE )


BOOST_AUTO_TEST_CASE( RevertDocumentRejectedWithOpenCommit )
{
    SCHEMATIC* schematic = loadSchematic( wxS( "api_kitchen_sink" ) );

    API_HANDLER_SCH handler( m_context );

    kiapi::common::ApiRequest beginRequest = makeBeginCommitRequest( *schematic );
    BOOST_REQUIRE( handler.Handle( beginRequest ).has_value() );

    kiapi::common::ApiRequest request = makeRevertRequest( *schematic );
    API_RESULT                result = handler.Handle( request );

    BOOST_REQUIRE( !result.has_value() );
    BOOST_CHECK_EQUAL( result.error().status(), kiapi::common::ApiStatusCode::AS_BUSY );
    BOOST_CHECK( result.error().error_message().find( "commit" ) != std::string::npos );
}



BOOST_AUTO_TEST_CASE( CustomPropertyCannotDuplicateSystemProperty )
{
    SCHEMATIC* schematic = loadSchematic( wxS( "api_kitchen_sink" ) );

    API_HANDLER_SCH handler( m_context );
    ApiRequest beginRequest = makeBeginCommitRequest( *schematic );
    BOOST_REQUIRE( handler.Handle( beginRequest ).has_value() );

    commands::CreateItems command;
    command.mutable_header()->mutable_document()->CopyFrom( makeDocument( *schematic ) );

    schematic::types::SchematicLine line;
    line.mutable_id()->set_value( "83618809-06d6-4cbe-ba0e-ec60fe58921c" );
    line.set_type( schematic::types::SchematicLineType::SLT_WIRE );
    line.add_custom_properties()->set_key( "linE Width" );
    line.add_custom_properties()->set_key( "start X" );

    command.add_items()->PackFrom( line );

    ApiRequest request;
    request.mutable_header()->set_client_name( "kicad.qa" );
    BOOST_REQUIRE( request.mutable_message()->PackFrom( command ) );

    API_RESULT result = handler.Handle( request );
    BOOST_REQUIRE( result.has_value() );

    commands::CreateItemsResponse response;
    BOOST_REQUIRE( result->message().UnpackTo( &response ) );
    BOOST_REQUIRE_EQUAL( response.created_items_size(), 1 );

    const commands::ItemStatus& status = response.created_items( 0 ).status();

    // Should be rejected; duplicates built in property
    BOOST_CHECK_EQUAL( status.code(), kiapi::common::commands::ItemStatusCode::ISC_INVALID_DATA );
    BOOST_CHECK_NE( status.error_message().find( "Invalid custom properties" ), std::string::npos );
}


BOOST_AUTO_TEST_CASE( SymbolFieldTakesPrecedenceOverCustomProperty )
{
    SCHEMATIC* schematic = loadSchematic( wxS( "api_kitchen_sink" ) );

    API_HANDLER_SCH handler( m_context );
    ApiRequest beginRequest = makeBeginCommitRequest( *schematic );
    BOOST_REQUIRE( handler.Handle( beginRequest ).has_value() );

    commands::CreateItems command;
    command.mutable_header()->mutable_document()->CopyFrom( makeDocument( *schematic ) );

    schematic::types::SchematicSymbolInstance symbol;
    symbol.mutable_id()->set_value( "f8688bac-2fcb-4184-ab0e-0cf46a139c43" );
    symbol.mutable_transform()->set_orientation( schematic::types::SchematicSymbolOrientation::SSO_0 );

    schematic::types::SchematicField* field = symbol.add_user_fields();
    field->set_name( "MPN" );
    field->mutable_text()->set_text( "123" );

    types::CustomProperty* prop = symbol.add_custom_properties();
    prop->set_key( "MPN" );
    prop->set_value( "456" );

    command.add_items()->PackFrom( symbol );

    ApiRequest request;
    request.mutable_header()->set_client_name( "kicad.qa" );
    BOOST_REQUIRE( request.mutable_message()->PackFrom( command ) );

    API_RESULT result = handler.Handle( request );
    BOOST_REQUIRE( result.has_value() );

    commands::CreateItemsResponse response;
    BOOST_REQUIRE( result->message().UnpackTo( &response ) );
    BOOST_REQUIRE_EQUAL( response.created_items_size(), 1 );

    const commands::ItemStatus& status = response.created_items( 0 ).status();

    // The field takes precedence: a custom property duplicating a field name is rejected.
    BOOST_CHECK_EQUAL( status.code(), kiapi::common::commands::ItemStatusCode::ISC_INVALID_DATA );
    BOOST_CHECK_NE( status.error_message().find( "MPN" ), std::string::npos );
}

BOOST_AUTO_TEST_SUITE_END()
