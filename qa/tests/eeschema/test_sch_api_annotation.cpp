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

BOOST_AUTO_TEST_SUITE_END()
