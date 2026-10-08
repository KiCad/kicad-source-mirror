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

#include <memory>

#include <wx/ffile.h>
#include <wx/filename.h>

#include <board.h>
#include <board_design_settings.h>
#include <zone_settings.h>

#include <downgrade_scan.h>
#include <downgrade_target.h>
#include <drc_rules_downgrade.h>
#include <drc/drc_rule.h>
#include <drc/drc_rule_parser.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>
#include <settings/settings_manager.h>
#include <pcbnew_utils/board_test_utils.h>
#include <qa_utils/downgrade_oracle_utils.h>
#include <qa_utils/downgrade_golden_utils.h>
#include <qa_utils/temporary_directory.h>


BOOST_AUTO_TEST_SUITE( BoardDowngrade )

static const DOWNGRADE_TARGET& kicad9 = KI_TEST::RequireDowngradeTarget( wxT( "9.0" ) );
static const DOWNGRADE_TARGET& kicad10 = KI_TEST::RequireDowngradeTarget( wxT( "10.0" ) );


BOOST_AUTO_TEST_CASE( BoardZoneDefaultsRoundTripAndDowngrade )
{
    SETTINGS_MANAGER settingsManager;
    BOARD            board;
    board.GetDesignSettings().m_ZoneLayerProperties[F_Cu].hatching_offset = VECTOR2I( 100000, 200000 );
    board.GetDesignSettings().m_ZoneLayerProperties[B_Cu].hatching_offset = VECTOR2I( 300000, 400000 );

    KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_board_zone_defaults" );
    const std::string            src = ( tmp.GetPath() / "source.kicad_pcb" ).string();
    PCB_IO_KICAD_SEXPR           io;
    io.SaveBoard( src, board );
    std::unique_ptr<BOARD> current( io.LoadBoard( src, nullptr ) );
    BOOST_REQUIRE( current );
    const auto& loadedDefaults = current->GetDesignSettings().m_ZoneLayerProperties;
    BOOST_REQUIRE_EQUAL( loadedDefaults.size(), 2 );
    BOOST_CHECK( loadedDefaults.at( F_Cu ).hatching_offset == VECTOR2I( 100000, 200000 ) );
    BOOST_CHECK( loadedDefaults.at( B_Cu ).hatching_offset == VECTOR2I( 300000, 400000 ) );
    BOOST_CHECK( current->GetDesignSettings().GetDefaultZoneSettings().m_LayerProperties.empty() );
}


// One unknown keyword makes an old KiCad drop the whole rules file, so the export must
// filter out rules the target cannot parse and keep the rest byte-identical.
BOOST_AUTO_TEST_CASE( DrcRulesFilterForKicad9 )
{
    const wxString rules = wxT( "(version 2)\n"
                                "# keep me\n"
                                "(rule \"clearance_ok\"\n"
                                "  # chamfer runs at 45deg here\n"
                                "  (constraint clearance (min 0.2mm))\n"
                                "  (condition \"A.NetClass == 'HV'\"))\n"
                                "(rule \"mask_check\"\n"
                                "  (constraint bridged_mask))\n"
                                "(rule \"chain_check\"\n"
                                "  (condition \"inNetChain('x')\")\n"
                                "  (constraint clearance (min 0.1mm)))\n"
                                "(rule \"delay_check\"\n"
                                "  (constraint skew (max 5ps)))\n" );

    DRC_RULES_FILTER_RESULT result = FilterDrcRulesForTarget( rules, kicad9 );

    BOOST_REQUIRE_EQUAL( result.m_dropped.size(), 3 );
    BOOST_CHECK( result.m_dropped[0] == wxT( "mask_check" ) );
    BOOST_CHECK( result.m_dropped[1] == wxT( "chain_check" ) );
    BOOST_CHECK( result.m_dropped[2] == wxT( "delay_check" ) );

    BOOST_CHECK( result.m_text.Contains( wxT( "(version 1)" ) ) );
    BOOST_CHECK( result.m_text.Contains( wxT( "# keep me" ) ) );
    BOOST_CHECK( result.m_text.Contains( wxT( "(rule \"clearance_ok\"" ) ) );
    BOOST_CHECK( result.m_text.Contains( wxT( "A.NetClass == 'HV'" ) ) );
    BOOST_CHECK( !result.m_text.Contains( wxT( "bridged_mask" ) ) );
    BOOST_CHECK( !result.m_text.Contains( wxT( "inNetChain" ) ) );
    BOOST_CHECK( !result.m_text.Contains( wxT( "5ps" ) ) );
}


// 10.0 knows the mask constraints and time units. Only post-10.0 syntax drops for it.
BOOST_AUTO_TEST_CASE( DrcRulesFilterForKicad10 )
{
    const wxString rules = wxT( "(version 2)\n"
                                "(rule \"mask_check\" (constraint bridged_mask))\n"
                                "(rule \"delay_check\" (constraint skew (max 5ps)))\n"
                                "(rule \"chain_check\" (constraint net_chain_length (max 10mm)))\n" );

    DRC_RULES_FILTER_RESULT result = FilterDrcRulesForTarget( rules, kicad10 );

    BOOST_REQUIRE_EQUAL( result.m_dropped.size(), 1 );
    BOOST_CHECK( result.m_dropped[0] == wxT( "chain_check" ) );
    BOOST_CHECK( result.m_text.Contains( wxT( "bridged_mask" ) ) );
    BOOST_CHECK( result.m_text.Contains( wxT( "5ps" ) ) );
}


// Comment syntax must never participate in balancing a rule. Parentheses or quotes
// in a full-line comment used to truncate or swallow otherwise valid rules.
BOOST_AUTO_TEST_CASE( DrcRulesWithCommentsRemainIntact )
{
    const wxString kept = wxT( "(rule \"clearance # literal\"\n"
                               "  # unmatched: ) (( \" \\\n"
                               "  (condition \"A.NetName == 'SIGNAL#1'\")\n"
                               "  # inNetChain('x')\n"
                               "  (constraint clearance (min 0.2mm))\n"
                               "  # ) \" (\n"
                               ")\n"
                               "(rule \"second\" (constraint track_width (min 0.25mm)))" );
    const wxString input = wxT( "(version 2)\n" ) + kept
                           + wxT( "\n(rule \"unsupported\"\n # ) \" (\n"
                                  " (constraint net_chain_length (max 10mm)))\n" );

    for( const DOWNGRADE_TARGET& target : { kicad9, kicad10 } )
    {
        DRC_RULES_FILTER_RESULT result = FilterDrcRulesForTarget( input, target );
        BOOST_REQUIRE( result.m_error.IsEmpty() );
        BOOST_REQUIRE_EQUAL( result.m_dropped.size(), 1 );
        BOOST_CHECK( result.m_dropped.front() == wxT( "unsupported" ) );
        BOOST_CHECK( result.m_text.Contains( kept ) );

        DRC_RULES_PARSER                       parser( result.m_text, wxT( "downgrade_inline_comments" ) );
        WX_STRING_REPORTER                     reporter;
        std::vector<std::shared_ptr<DRC_RULE>> rules;
        parser.Parse( rules, &reporter );
        BOOST_CHECK_MESSAGE( !reporter.HasMessageOfSeverity( RPT_SEVERITY_ERROR ), reporter.GetMessages() );
        BOOST_REQUIRE_EQUAL( rules.size(), 2 );
        BOOST_CHECK( rules[0]->FindConstraint( CLEARANCE_CONSTRAINT ).has_value() );
        BOOST_CHECK( rules[1]->FindConstraint( TRACK_WIDTH_CONSTRAINT ).has_value() );
    }
}


BOOST_AUTO_TEST_CASE( InvalidDrcCommentAndUnbalancedSyntaxFailClosed )
{
    for( const DOWNGRADE_TARGET& target : { kicad9, kicad10 } )
    {
        for( const wxString& input : { wxT( "(version 1)\n(rule \"inline\" # )\n (constraint clearance (min 0.2mm)))" ),
                                       wxT( "(version 1)\n(rule \"missing_close\" (constraint clearance (min 0.2mm))" ),
                                       wxT( "(version 1)\n(rule \"missing_quote" ) } )
        {
            BOOST_CHECK( !FilterDrcRulesForTarget( input, target ).m_error.IsEmpty() );
        }
    }
}


BOOST_AUTO_TEST_CASE( DrcRulesMatchIndependentOlderReferences )
{
    const std::string fixture = KI_TEST::GetPcbnewTestDataDir() + "../downgrade/golden/";
    const wxString    input = wxString::FromUTF8( KI_TEST::ReadGoldenText( fixture + "current/rules.kicad_dru" ) );

    for( const DOWNGRADE_TARGET& target : { kicad9, kicad10 } )
    {
        DRC_RULES_FILTER_RESULT result = FilterDrcRulesForTarget( input, target );
        BOOST_REQUIRE( result.m_error.IsEmpty() );
        const std::string release = target.m_id == wxT( "9.0" ) ? "v9" : "v10";
        BOOST_CHECK( result.m_text.ToStdString() == KI_TEST::ReadGoldenText( fixture + release + "/rules.kicad_dru" ) );
        BOOST_CHECK_EQUAL( result.m_dropped.size(), release == "v9" ? 2 : 1 );
        DRC_RULES_PARSER                       parser( result.m_text, wxT( "downgrade_drc_golden" ) );
        WX_STRING_REPORTER                     reporter;
        std::vector<std::shared_ptr<DRC_RULE>> rules;
        parser.Parse( rules, &reporter );
        BOOST_CHECK_MESSAGE( !reporter.HasMessageOfSeverity( RPT_SEVERITY_ERROR ), reporter.GetMessages() );
        BOOST_CHECK_EQUAL( rules.size(), release == "v9" ? 1 : 2 );
    }
}


// The staged library downgrade converts and verifies every file before any original is
// replaced, and a refusal leaves the folder untouched with no temp files behind.
BOOST_AUTO_TEST_CASE( LibraryFilesDowngradeFailsClosed )
{
    KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_lib_staged", "" );
    const wxString               dir = tmp.GetPath().wstring();

    auto writeFile = [&]( const wxString& aName, const wxString& aContent )
    {
        wxFFile file( dir + wxFileName::GetPathSeparator() + aName, wxT( "wb" ) );
        BOOST_REQUIRE( file.IsOpened() );
        file.Write( aContent );
    };

    writeFile( wxT( "a.fake_mod" ), wxT( "old a" ) );
    writeFile( wxT( "b.fake_mod" ), wxT( "old b" ) );

    auto noForbidden = []( const wxString& )
    {
        return wxString();
    };

    auto convertOk = [&]( const wxString& aFile, const wxString& aTmp ) -> DOWNGRADE_FILE_RESULT
    {
        wxFFile out( aTmp, wxT( "wb" ) );
        out.Write( wxT( "(version 123) converted" ) );
        return DOWNGRADE_FILE_RESULT::CONVERTED;
    };

    BOOST_CHECK( DowngradeLibraryFilesInPlace( dir, wxT( "*.fake_mod" ), 123, noForbidden, convertOk ).IsEmpty() );

    wxFFile  check( dir + wxFileName::GetPathSeparator() + wxT( "a.fake_mod" ), wxT( "rb" ) );
    wxString content;
    BOOST_REQUIRE( check.IsOpened() && check.ReadAll( &content ) );
    BOOST_CHECK( content.Contains( wxT( "converted" ) ) );

    // Second pass: the second file refuses, so the first must stay as it is now.
    auto convertRefuseB = [&]( const wxString& aFile, const wxString& aTmp ) -> DOWNGRADE_FILE_RESULT
    {
        if( aFile.Contains( wxT( "b.fake_mod" ) ) )
            return DOWNGRADE_FILE_RESULT::REFUSED;

        wxFFile out( aTmp, wxT( "wb" ) );
        out.Write( wxT( "(version 123) second pass" ) );
        return DOWNGRADE_FILE_RESULT::CONVERTED;
    };

    wxString bad = DowngradeLibraryFilesInPlace( dir, wxT( "*.fake_mod" ), 123, noForbidden, convertRefuseB );
    BOOST_CHECK( bad.Contains( wxT( "b.fake_mod" ) ) );

    wxArrayString leftovers;
    wxDir::GetAllFiles( dir, &leftovers );
    BOOST_CHECK_EQUAL( leftovers.size(), 2 );

    wxFFile  recheck( dir + wxFileName::GetPathSeparator() + wxT( "a.fake_mod" ), wxT( "rb" ) );
    wxString unchanged;
    BOOST_REQUIRE( recheck.IsOpened() && recheck.ReadAll( &unchanged ) );
    BOOST_CHECK( !unchanged.Contains( wxT( "second pass" ) ) );
}


// The project export carries design files only. UI state and non-KiCad files stay behind.
BOOST_AUTO_TEST_CASE( DesignFileFilter )
{
    BOOST_CHECK( IsDowngradeDesignFile( wxT( "proj/proj.kicad_pro" ) ) );
    BOOST_CHECK( IsDowngradeDesignFile( wxT( "proj/sub/sheet.kicad_sch" ) ) );
    BOOST_CHECK( IsDowngradeDesignFile( wxT( "proj/proj.kicad_pcb" ) ) );
    BOOST_CHECK( IsDowngradeDesignFile( wxT( "proj/proj.kicad_dru" ) ) );
    BOOST_CHECK( IsDowngradeDesignFile( wxT( "proj/frame.kicad_wks" ) ) );
    BOOST_CHECK( IsDowngradeDesignFile( wxT( "proj/lib.kicad_sym" ) ) );
    BOOST_CHECK( IsDowngradeDesignFile( wxT( "proj/lib.pretty/fp.kicad_mod" ) ) );
    BOOST_CHECK( IsDowngradeDesignFile( wxT( "proj/sym-lib-table" ) ) );
    BOOST_CHECK( IsDowngradeDesignFile( wxT( "proj/fp-lib-table" ) ) );

    BOOST_CHECK( !IsDowngradeDesignFile( wxT( "proj/proj.kicad_prl" ) ) );
    BOOST_CHECK( !IsDowngradeDesignFile( wxT( "proj/datasheet.pdf" ) ) );
    BOOST_CHECK( !IsDowngradeDesignFile( wxT( "proj/out/proj-F_Cu.gbr" ) ) );
    BOOST_CHECK( !IsDowngradeDesignFile( wxT( "proj/proj.kicad_jobset" ) ) );
    BOOST_CHECK( !IsDowngradeDesignFile( wxT( "proj/notes.txt" ) ) );
}


BOOST_AUTO_TEST_SUITE_END()
