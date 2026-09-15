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
#include <advanced_config.h>
#include <connection_graph.h>
#include <sch_label.h>
#include <sch_line.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <set>
#include <scoped_set_reset.h>


namespace
{

struct API_HANDLER_SCH_FIXTURE
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

    symbol.mutable_definition()->mutable_id()->set_entry_name( "R" );

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

BOOST_AUTO_TEST_CASE( NetlistRebuildsSelectedConnectivityAndFiltersNativeItems )
{
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, false );
    SCHEMATIC* model = loadSchematic( wxS( "net_chains_four_nets_labeled" ) );
    const auto path = model->Hierarchy().front();
    SCH_LABEL* label = nullptr;

    for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_LABEL_T ) )
    {
        if( static_cast<SCH_LABEL*>( item )->GetText() == wxS( "SIG" ) )
            label = static_cast<SCH_LABEL*>( item );
    }

    BOOST_REQUIRE( label );
    API_HANDLER_SCH handler( m_context );

    for( bool published : { false, true } )
    {
        BOOST_TEST_CONTEXT( "published=" << published )
        {
            enabled = published;
            const wxString name = published ? wxS( "PUBLISHED_API" ) : wxS( "LEGACY_API" );
            label->SetText( name );

            // Only the engine handler rebuilds; legacy answers from the graph the editor keeps current
            if( published )
                model->ConnectionGraph()->Reset();
            else
                model->RebuildConnectivity();

            for( bool filtered : { false, true } )
            {
                kiapi::schematic::commands::GetSchematicNetlist command;
                *command.mutable_document() = makeDocument( *model );

                if( filtered )
                    command.add_types( kiapi::common::types::KOT_SCH_LABEL );

                ApiRequest request;
                request.mutable_header()->set_client_name( "kicad.qa" );
                BOOST_REQUIRE( request.mutable_message()->PackFrom( command ) );
                API_RESULT result = handler.Handle( request );
                BOOST_REQUIRE( result.has_value() );
                kiapi::schematic::commands::SchematicNetlistResponse response;
                BOOST_REQUIRE( result->message().UnpackTo( &response ) );
                std::set<std::string> items;

                for( const auto& net : response.nets() )
                {
                    if( net.name() != ( wxS( "/" ) + name ).ToStdString() )
                        continue;

                    for( const auto& sheet : net.sheets() )
                    {
                        for( const auto& item : sheet.items() )
                            items.insert( item.value() );
                    }
                }

                BOOST_CHECK( items.contains( label->m_Uuid.AsStdString() ) );

                if( filtered )
                    BOOST_CHECK_EQUAL( items.size(), 1 );
                else
                    BOOST_CHECK_GT( items.size(), 1 );
            }
        }
    }
}

BOOST_AUTO_TEST_CASE( NetlistPreservesNativeHierarchyMembershipAndExcludesBuses )
{
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, false );
    SCHEMATIC* model = loadSchematic( wxS( "netlists/hierarchy_aliases/hierarchy_aliases" ) );
    auto* root = model->Hierarchy().front().LastScreen();
    auto labels = root->Items().OfType( SCH_LABEL_T );
    BOOST_REQUIRE( labels.begin() != labels.end() );
    auto* unusedBus = static_cast<SCH_LABEL*>( ( *labels.begin() )->Clone() );
    const_cast<KIID&>( unusedBus->m_Uuid ) = KIID();
    unusedBus->SetText( wxS( "API_UNUSED[0..1]" ) );
    unusedBus->SetPosition( VECTOR2I( 0, 0 ) );
    root->Append( unusedBus );
    SCH_LINE* undriven = nullptr;

    for( SCH_ITEM* item : root->Items().OfType( SCH_LINE_T ) )
    {
        auto* line = static_cast<SCH_LINE*>( item );

        if( line->IsWire() )
        {
            undriven = static_cast<SCH_LINE*>( line->Clone() );
            break;
        }
    }

    BOOST_REQUIRE( undriven );
    const_cast<KIID&>( undriven->m_Uuid ) = KIID();
    undriven->Move( VECTOR2I( -1000000, -1000000 ) - undriven->GetStartPoint() );
    root->Append( undriven );
    API_HANDLER_SCH handler( m_context );
    using CONTENTS = std::map<std::string, std::map<std::vector<std::string>, std::set<std::string>>>;

    for( bool filtered : { false, true } )
    {
        CONTENTS expected;

        for( bool published : { false, true } )
        {
            BOOST_TEST_CONTEXT( "published=" << published << " filtered=" << filtered )
            {
                enabled = published;

                // Only the engine handler rebuilds; legacy answers from the graph the editor keeps current
                if( published )
                    model->ConnectionGraph()->Reset();
                else
                    model->RebuildConnectivity();

                kiapi::schematic::commands::GetSchematicNetlist command;
                *command.mutable_document() = makeDocument( *model );

                if( filtered )
                    command.add_types( kiapi::common::types::KOT_SCH_PIN );

                ApiRequest request;
                request.mutable_header()->set_client_name( "kicad.qa" );
                BOOST_REQUIRE( request.mutable_message()->PackFrom( command ) );
                API_RESULT result = handler.Handle( request );
                BOOST_REQUIRE( result.has_value() );
                kiapi::schematic::commands::SchematicNetlistResponse response;
                BOOST_REQUIRE( result->message().UnpackTo( &response ) );
                CONTENTS actual;
                size_t nestedItems = 0;

                for( const auto& net : response.nets() )
                {
                    BOOST_CHECK_GT( net.sheets_size(), 0 );
                    BOOST_CHECK( net.name().find( '{' ) == std::string::npos );
                    BOOST_CHECK( net.name().find( "API_UNUSED" ) == std::string::npos );

                    for( const auto& sheet : net.sheets() )
                    {
                        std::vector<std::string> path;

                        for( const auto& id : sheet.path().path() )
                            path.push_back( id.value() );

                        auto& items = actual[net.name()][path];

                        for( const auto& item : sheet.items() )
                        {
                            BOOST_CHECK_NE( item.value(), undriven->m_Uuid.AsStdString() );
                            items.insert( item.value() );
                        }

                        if( path.size() > 2 )
                            nestedItems += items.size();
                    }
                }

                BOOST_CHECK( actual.contains( "/S0.BOOT.SDA" ) );
                BOOST_CHECK( actual.contains( "/S1.BOOT.SDA" ) );
                BOOST_CHECK_GT( nestedItems, 0 );

                if( published )
                {
                    BOOST_CHECK( actual == expected );
                }
                else
                {
                    expected = std::move( actual );
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE( NetlistReportsRebuildFailureAndAllowsRetry )
{
    struct FAILING_LABEL : SCH_LABEL
    {
        explicit FAILING_LABEL( const SCH_LABEL& source ) : SCH_LABEL( source ) {}

        wxString GetShownText( const SCH_SHEET_PATH* path, RESOLUTION_CONTEXT context, int depth = 0 ) const override
        {
            if( fail )
                throw std::runtime_error( "Native label extraction failed" );

            return SCH_LABEL::GetShownText( path, context, depth );
        }

        bool fail = false;
    };

    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, false );
    SCHEMATIC* model = loadSchematic( wxS( "net_chains_four_nets_labeled" ) );
    auto* screen = model->Hierarchy().front().LastScreen();
    auto labels = screen->Items().OfType( SCH_LABEL_T );
    BOOST_REQUIRE( labels.begin() != labels.end() );
    auto* original = static_cast<SCH_LABEL*>( *labels.begin() );
    auto* replacement = new FAILING_LABEL( *original );
    screen->Remove( original );
    delete original;
    screen->Append( replacement );
    model->ConnectionGraph()->Reset();
    replacement->fail = true;
    enabled = true;
    API_HANDLER_SCH handler( m_context );
    kiapi::schematic::commands::GetSchematicNetlist command;
    *command.mutable_document() = makeDocument( *model );
    ApiRequest request;
    request.mutable_header()->set_client_name( "kicad.qa" );
    BOOST_REQUIRE( request.mutable_message()->PackFrom( command ) );
    API_RESULT result = handler.Handle( request );
    BOOST_REQUIRE( !result.has_value() );
    BOOST_CHECK_EQUAL( result.error().status(), ApiStatusCode::AS_UNKNOWN );
    BOOST_CHECK_EQUAL( result.error().error_message(), "Native label extraction failed" );
    replacement->fail = false;
    result = handler.Handle( request );
    BOOST_REQUIRE( result.has_value() );
    kiapi::schematic::commands::SchematicNetlistResponse response;
    BOOST_REQUIRE( result->message().UnpackTo( &response ) );
    BOOST_CHECK_GT( response.nets_size(), 0 );
}

BOOST_AUTO_TEST_SUITE_END()
