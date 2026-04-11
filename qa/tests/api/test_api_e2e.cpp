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

#include <boost/test/unit_test.hpp>
#include <wx/filefn.h>
#include <wx/filename.h>

#include "api_e2e_utils.h"

#include <api/board/board.pb.h>
#include <api/board/board_commands.pb.h>


class TEMP_KITCHEN_SINK_COPY
{
public:
    ~TEMP_KITCHEN_SINK_COPY()
    {
        if( !m_tempDir.IsEmpty() && wxFileName::DirExists( m_tempDir ) )
            wxFileName::Rmdir( m_tempDir, wxPATH_RMDIR_RECURSIVE );
    }

    bool Create( wxString* aError )
    {
        wxString   testDataDir = wxString::FromUTF8( KI_TEST::GetPcbnewTestDataDir() );
        wxFileName srcPcb( testDataDir, wxS( "api_kitchen_sink.kicad_pcb" ) );
        wxFileName srcPro( testDataDir, wxS( "api_kitchen_sink.kicad_pro" ) );
        wxFileName srcDru( testDataDir, wxS( "api_kitchen_sink.kicad_dru" ) );

        wxString tempToken = wxFileName::CreateTempFileName( wxS( "kicad-api-e2e-" ) );

        if( tempToken.IsEmpty() )
        {
            if( aError )
                *aError = wxS( "Failed to create temporary file name" );

            return false;
        }

        wxRemoveFile( tempToken );

        if( !wxFileName::Mkdir( tempToken, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
        {
            if( aError )
                *aError = wxS( "Failed to create temporary directory" );

            return false;
        }

        m_tempDir = tempToken;

        wxFileName dstPcb( m_tempDir, srcPcb.GetFullName() );
        wxFileName dstPro( m_tempDir, srcPro.GetFullName() );
        wxFileName dstDru( m_tempDir, srcDru.GetFullName() );

        if( !wxCopyFile( srcPcb.GetFullPath(), dstPcb.GetFullPath(), true )
            || !wxCopyFile( srcPro.GetFullPath(), dstPro.GetFullPath(), true )
            || !wxCopyFile( srcDru.GetFullPath(), dstDru.GetFullPath(), true ) )
        {
            if( aError )
                *aError = wxS( "Failed to copy fixture files into temporary directory" );

            return false;
        }

        m_boardPath = dstPcb.GetFullPath();
        return true;
    }

    const wxString& BoardPath() const { return m_boardPath; }

private:
    wxString m_tempDir;
    wxString m_boardPath;
};


bool SendGetBoardDesignRules( API_TEST_CLIENT& aClient, const kiapi::common::types::DocumentSpecifier& aDocument,
                              kiapi::board::commands::BoardDesignRulesResponse* aOut, wxString* aError = nullptr )
{
    kiapi::board::commands::GetBoardDesignRules request;
    *request.mutable_board() = aDocument;

    kiapi::common::ApiResponse response;

    if( !aClient.SendCommand( request, &response ) )
    {
        if( aError )
            *aError = aClient.LastError();

        return false;
    }

    if( response.status().status() != kiapi::common::AS_OK )
    {
        if( aError )
            *aError = response.status().error_message();

        return false;
    }

    if( !response.message().UnpackTo( aOut ) )
    {
        if( aError )
            *aError = wxS( "Failed to unpack BoardDesignRulesResponse" );

        return false;
    }

    return true;
}


bool SendGetCustomDesignRules( API_TEST_CLIENT& aClient, const kiapi::common::types::DocumentSpecifier& aDocument,
                               kiapi::board::commands::CustomRulesResponse* aOut, wxString* aError = nullptr )
{
    kiapi::board::commands::GetCustomDesignRules request;
    *request.mutable_board() = aDocument;

    kiapi::common::ApiResponse response;

    if( !aClient.SendCommand( request, &response ) )
    {
        if( aError )
            *aError = aClient.LastError();

        return false;
    }

    if( response.status().status() != kiapi::common::AS_OK )
    {
        if( aError )
            *aError = response.status().error_message();

        return false;
    }

    if( !response.message().UnpackTo( aOut ) )
    {
        if( aError )
            *aError = wxS( "Failed to unpack CustomRulesResponse" );

        return false;
    }

    return true;
}


BOOST_AUTO_TEST_SUITE( ApiE2E )


BOOST_FIXTURE_TEST_CASE( ServerStartsAndResponds, API_SERVER_E2E_FIXTURE )
{
    BOOST_REQUIRE_MESSAGE( Start(), LastError() );

    BOOST_CHECK_MESSAGE( Client().GetVersion(), "GetVersion failed: " + Client().LastError() );
}


BOOST_FIXTURE_TEST_CASE( OpenSingleBoard, API_SERVER_E2E_FIXTURE )
{
    BOOST_REQUIRE_MESSAGE( Start(), LastError() );

    kiapi::common::types::DocumentSpecifier document;
    wxString                                testDataDir = wxString::FromUTF8( KI_TEST::GetPcbnewTestDataDir() );
    wxFileName                              boardPath( testDataDir, wxS( "api_kitchen_sink.kicad_pcb" ) );

    BOOST_REQUIRE_MESSAGE( Client().OpenDocument( boardPath.GetFullPath(), &document ),
                           "OpenDocument failed: " + Client().LastError() );

    BOOST_REQUIRE( document.type() == kiapi::common::types::DOCTYPE_PCB );
    BOOST_REQUIRE( boardPath.GetFullName().Matches( document.board_filename() ) );

    int footprintCount = 0;

    BOOST_REQUIRE_MESSAGE( Client().GetItemsCount( document, kiapi::common::types::KOT_PCB_FOOTPRINT, &footprintCount ),
                           "GetItems failed: " + Client().LastError() );

    BOOST_CHECK_GT( footprintCount, 0 );
}


BOOST_FIXTURE_TEST_CASE( SwitchBoards, API_SERVER_E2E_FIXTURE )
{
    BOOST_REQUIRE_MESSAGE( Start(), LastError() );

    wxString   testDataDir = wxString::FromUTF8( KI_TEST::GetPcbnewTestDataDir() );
    wxFileName boardPathA( testDataDir, wxS( "api_kitchen_sink.kicad_pcb" ) );
    wxFileName boardPathB( testDataDir, wxS( "padstacks.kicad_pcb" ) );

    kiapi::common::types::DocumentSpecifier documentA;

    BOOST_REQUIRE_MESSAGE( Client().OpenDocument( boardPathA.GetFullPath(), &documentA ),
                           "OpenDocument for first board failed: " + Client().LastError() );

    FOOTPRINT footprintA( nullptr );

    BOOST_REQUIRE_MESSAGE( Client().GetFirstFootprint( documentA, &footprintA ),
                           "GetFirstFootprint for first board failed: " + Client().LastError() );

    kiapi::common::types::DocumentSpecifier documentB;

    BOOST_REQUIRE_MESSAGE( Client().OpenDocument( boardPathB.GetFullPath(), &documentB ),
                           "OpenDocument for second board failed: " + Client().LastError() );

    FOOTPRINT footprintB( nullptr );

    BOOST_REQUIRE_MESSAGE( Client().GetFirstFootprint( documentB, &footprintB ),
                           "GetFirstFootprint for second board failed: " + Client().LastError() );

    BOOST_CHECK_NE( footprintA.Similarity( footprintB ), 1.0 );

    kiapi::common::ApiStatusCode closeStatus = kiapi::common::AS_UNKNOWN;

    BOOST_REQUIRE_MESSAGE( Client().CloseDocument( &documentB, &closeStatus ),
                           "CloseDocument failed: " + Client().LastError() );
    BOOST_CHECK_EQUAL( closeStatus, kiapi::common::AS_OK );

    BOOST_REQUIRE_MESSAGE( Client().CloseDocument( nullptr, &closeStatus ),
                           "CloseDocument after already closed failed: " + Client().LastError() );
    BOOST_CHECK_EQUAL( closeStatus, kiapi::common::AS_BAD_REQUEST );
}


BOOST_FIXTURE_TEST_CASE( GetBoardDesignRules, API_SERVER_E2E_FIXTURE )
{
    BOOST_REQUIRE_MESSAGE( Start(), LastError() );

    kiapi::common::types::DocumentSpecifier document;
    wxFileName boardPath( wxString::FromUTF8( KI_TEST::GetPcbnewTestDataDir() ), wxS( "api_kitchen_sink.kicad_pcb" ) );

    BOOST_REQUIRE_MESSAGE( Client().OpenDocument( boardPath.GetFullPath(), &document ),
                           "OpenDocument failed: " + Client().LastError() );

    kiapi::board::commands::BoardDesignRulesResponse rulesResponse;
    wxString                                         error;

    BOOST_REQUIRE_MESSAGE( SendGetBoardDesignRules( Client(), document, &rulesResponse, &error ),
                           "GetBoardDesignRules failed: " + error );

    BOOST_CHECK_GE( rulesResponse.rules().constraints().min_clearance().value_nm(), 0 );
    BOOST_CHECK_GE( rulesResponse.rules().constraints().min_track_width().value_nm(), 0 );
    BOOST_CHECK( rulesResponse.custom_rules_status() == kiapi::board::commands::CRS_VALID );
}


BOOST_FIXTURE_TEST_CASE( SetBoardDesignRules, API_SERVER_E2E_FIXTURE )
{
    BOOST_REQUIRE_MESSAGE( Start(), LastError() );

    TEMP_KITCHEN_SINK_COPY fixtureCopy;
    wxString               copyError;

    BOOST_REQUIRE_MESSAGE( fixtureCopy.Create( &copyError ), copyError );

    kiapi::common::types::DocumentSpecifier document;

    BOOST_REQUIRE_MESSAGE( Client().OpenDocument( fixtureCopy.BoardPath(), &document ),
                           "OpenDocument failed: " + Client().LastError() );

    kiapi::board::commands::BoardDesignRulesResponse getResponse;

    BOOST_REQUIRE_MESSAGE( SendGetBoardDesignRules( Client(), document, &getResponse, &copyError ),
                           "Initial GetBoardDesignRules failed: " + copyError );

    int newWidth = getResponse.rules().constraints().min_track_width().value_nm() + 1000;

    kiapi::board::commands::SetBoardDesignRules setRequest;
    *setRequest.mutable_board() = document;
    *setRequest.mutable_rules()->mutable_constraints() = getResponse.rules().constraints();
    setRequest.mutable_rules()->mutable_constraints()->mutable_min_track_width()->set_value_nm( newWidth );

    kiapi::common::ApiResponse setApiResponse;

    BOOST_REQUIRE_MESSAGE( Client().SendCommand( setRequest, &setApiResponse ),
                           "SetBoardDesignRules failed to send: " + Client().LastError() );
    BOOST_REQUIRE( setApiResponse.status().status() == kiapi::common::AS_OK );

    kiapi::board::commands::BoardDesignRulesResponse setResponse;
    BOOST_REQUIRE( setApiResponse.message().UnpackTo( &setResponse ) );

    BOOST_CHECK_EQUAL( setResponse.rules().constraints().min_track_width().value_nm(), newWidth );

    kiapi::board::commands::BoardDesignRulesResponse verifyResponse;

    BOOST_REQUIRE_MESSAGE( SendGetBoardDesignRules( Client(), document, &verifyResponse, &copyError ),
                           "Verification GetBoardDesignRules failed: " + copyError );

    BOOST_CHECK_EQUAL( verifyResponse.rules().constraints().min_track_width().value_nm(), newWidth );
}


BOOST_FIXTURE_TEST_CASE( SetBoardDesignRules_ValidationFailure, API_SERVER_E2E_FIXTURE )
{
    BOOST_REQUIRE_MESSAGE( Start(), LastError() );

    TEMP_KITCHEN_SINK_COPY fixtureCopy;
    wxString               copyError;

    BOOST_REQUIRE_MESSAGE( fixtureCopy.Create( &copyError ), copyError );

    kiapi::common::types::DocumentSpecifier document;

    BOOST_REQUIRE_MESSAGE( Client().OpenDocument( fixtureCopy.BoardPath(), &document ),
                           "OpenDocument failed: " + Client().LastError() );

    kiapi::board::commands::BoardDesignRulesResponse getResponse;

    BOOST_REQUIRE_MESSAGE( SendGetBoardDesignRules( Client(), document, &getResponse, &copyError ),
                           "Initial GetBoardDesignRules failed: " + copyError );

    kiapi::board::commands::SetBoardDesignRules setRequest;
    *setRequest.mutable_board() = document;
    *setRequest.mutable_rules()->mutable_constraints() = getResponse.rules().constraints();
    setRequest.mutable_rules()->mutable_constraints()->mutable_min_track_width()->set_value_nm( 1000000000 );

    kiapi::common::ApiResponse response;

    BOOST_REQUIRE_MESSAGE( Client().SendCommand( setRequest, &response ),
                           "SetBoardDesignRules failed to send: " + Client().LastError() );

    BOOST_CHECK( response.status().status() == kiapi::common::AS_BAD_REQUEST );
    BOOST_CHECK( !response.status().error_message().empty() );
}


BOOST_FIXTURE_TEST_CASE( SetBoardDesignRules_SeverityOverrides, API_SERVER_E2E_FIXTURE )
{
    BOOST_REQUIRE_MESSAGE( Start(), LastError() );

    TEMP_KITCHEN_SINK_COPY fixtureCopy;
    wxString               copyError;

    BOOST_REQUIRE_MESSAGE( fixtureCopy.Create( &copyError ), copyError );

    kiapi::common::types::DocumentSpecifier document;

    BOOST_REQUIRE_MESSAGE( Client().OpenDocument( fixtureCopy.BoardPath(), &document ),
                           "OpenDocument failed: " + Client().LastError() );

    kiapi::board::commands::BoardDesignRulesResponse getResponse;
    BOOST_REQUIRE_MESSAGE( SendGetBoardDesignRules( Client(), document, &getResponse, &copyError ),
                           "Initial GetBoardDesignRules failed: " + copyError );

    auto applySeverityAndVerify = [&]( kiapi::common::types::RuleSeverity aSeverity )
    {
        kiapi::board::commands::SetBoardDesignRules setRequest;
        *setRequest.mutable_board() = document;

        kiapi::board::DrcSeveritySetting* setting = setRequest.mutable_rules()->add_severities();
        setting->set_rule_type( kiapi::board::DrcErrorType::DRCET_UNCONNECTED_ITEMS );
        setting->set_severity( aSeverity );

        kiapi::common::ApiResponse setApiResponse;
        BOOST_REQUIRE_MESSAGE( Client().SendCommand( setRequest, &setApiResponse ),
                               "SetBoardDesignRules failed to send: " + Client().LastError() );
        BOOST_REQUIRE( setApiResponse.status().status() == kiapi::common::AS_OK );

        kiapi::board::commands::BoardDesignRulesResponse verifyResponse;
        BOOST_REQUIRE_MESSAGE( SendGetBoardDesignRules( Client(), document, &verifyResponse, &copyError ),
                               "Verification GetBoardDesignRules failed: " + copyError );

        bool found = false;

        for( const kiapi::board::DrcSeveritySetting& severity : verifyResponse.rules().severities() )
        {
            if( severity.rule_type() == kiapi::board::DrcErrorType::DRCET_UNCONNECTED_ITEMS )
            {
                found = true;
                BOOST_CHECK( severity.severity() == aSeverity );
                break;
            }
        }

        BOOST_CHECK( found );
    };

    applySeverityAndVerify( kiapi::common::types::RS_IGNORE );
    applySeverityAndVerify( kiapi::common::types::RS_ERROR );
}


BOOST_FIXTURE_TEST_CASE( GetCustomDesignRules_ReturnsFixtureRules, API_SERVER_E2E_FIXTURE )
{
    BOOST_REQUIRE_MESSAGE( Start(), LastError() );

    kiapi::common::types::DocumentSpecifier document;
    wxFileName boardPath( wxString::FromUTF8( KI_TEST::GetPcbnewTestDataDir() ), wxS( "api_kitchen_sink.kicad_pcb" ) );

    BOOST_REQUIRE_MESSAGE( Client().OpenDocument( boardPath.GetFullPath(), &document ),
                           "OpenDocument failed: " + Client().LastError() );

    kiapi::board::commands::CustomRulesResponse response;
    wxString                                    error;

    BOOST_REQUIRE_MESSAGE( SendGetCustomDesignRules( Client(), document, &response, &error ),
                           "GetCustomDesignRules failed: " + error );

    BOOST_CHECK( response.status() == kiapi::board::commands::CRS_VALID );
    BOOST_CHECK_GT( response.rules_size(), 0 );

    bool foundFixtureRule = false;

    for( const kiapi::board::CustomRule& rule : response.rules() )
    {
        if( rule.name() == "myrule" )
        {
            foundFixtureRule = true;
            BOOST_CHECK_EQUAL( rule.condition(), "A.Name == 'test'" );
            BOOST_CHECK_EQUAL( rule.constraints_size(), 2 );
            BOOST_REQUIRE( !rule.comments().empty() );
            break;
        }
    }

    BOOST_CHECK( foundFixtureRule );
}


BOOST_FIXTURE_TEST_CASE( SetCustomDesignRules_RoundTripSingleRule, API_SERVER_E2E_FIXTURE )
{
    BOOST_REQUIRE_MESSAGE( Start(), LastError() );

    TEMP_KITCHEN_SINK_COPY fixtureCopy;
    wxString               copyError;

    BOOST_REQUIRE_MESSAGE( fixtureCopy.Create( &copyError ), copyError );

    kiapi::common::types::DocumentSpecifier document;

    BOOST_REQUIRE_MESSAGE( Client().OpenDocument( fixtureCopy.BoardPath(), &document ),
                           "OpenDocument failed: " + Client().LastError() );

    kiapi::board::commands::CustomRulesResponse response;
    wxString                                    error;

    BOOST_REQUIRE_MESSAGE( SendGetCustomDesignRules( Client(), document, &response, &error ),
                           "GetCustomDesignRules failed: " + error );

    BOOST_CHECK( response.status() == kiapi::board::commands::CRS_VALID );
    BOOST_CHECK_GT( response.rules_size(), 0 );

    kiapi::board::commands::SetCustomDesignRules setRequest;
    *setRequest.mutable_board() = document;

    setRequest.mutable_rules()->CopyFrom( response.rules() );

    kiapi::board::CustomRule* rule = setRequest.add_rules();
    rule->set_name( "api_roundtrip_rule" );
    rule->set_condition( "A.NetClass == 'HV'" );
    rule->set_layer_mode( kiapi::board::CRLM_OUTER );
    rule->set_severity( kiapi::common::types::RS_WARNING );

    kiapi::board::CustomRuleConstraint* annular = rule->add_constraints();
    annular->set_type( kiapi::board::CRCT_TRACK_WIDTH );
    annular->mutable_numeric()->set_min( 3000000 );
    annular->mutable_numeric()->set_opt( 4000000 );
    annular->mutable_numeric()->set_max( 5000000 );

    kiapi::board::CustomRuleConstraint* disallow = rule->add_constraints();
    disallow->set_type( kiapi::board::CRCT_DISALLOW );
    disallow->mutable_disallow()->add_types( kiapi::board::CRDT_BLIND_VIAS );

    kiapi::common::ApiResponse setApiResponse;

    BOOST_REQUIRE_MESSAGE( Client().SendCommand( setRequest, &setApiResponse ),
                           "SetCustomDesignRules failed to send: " + Client().LastError() );
    BOOST_REQUIRE( setApiResponse.status().status() == kiapi::common::AS_OK );

    kiapi::board::commands::CustomRulesResponse setResponse;
    BOOST_REQUIRE( setApiResponse.message().UnpackTo( &setResponse ) );

    int num_rules = setResponse.rules_size();
    BOOST_CHECK( setResponse.status() == kiapi::board::commands::CRS_VALID );
    BOOST_CHECK_EQUAL( num_rules, response.rules_size() + 1 );
    BOOST_CHECK_EQUAL( setResponse.rules( num_rules - 1 ).name(), "api_roundtrip_rule" );
}


BOOST_AUTO_TEST_SUITE_END()
