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


BOOST_AUTO_TEST_SUITE_END()
