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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <board.h>
#include <drc/rule_editor/drc_re_custom_rule_constraint_data.h>
#include <drc/rule_editor/drc_re_loaded_rule.h>
#include <drc/rule_editor/drc_re_numeric_input_constraint_data.h>
#include <drc/rule_editor/drc_rule_document.h>
#include <drc/rule_editor/drc_rule_editor_enums.h>
#include <drc/rule_editor/drc_rule_editor_utils.h>
#include <dialogs/panel_setup_constraint_rules.h>

#include <wx/ffile.h>
#include <wx/filename.h>


namespace
{

const wxString TWO_CLEARANCES_AND_A_HOLE = wxS( "(version 2)\n"
                                                "# header comment\n"
                                                "(rule \"first\"\n"
                                                "\t# keep this comment\n"
                                                "\t(constraint clearance (min 0.3mm))\n"
                                                "\t(condition \"A.NetClass == 'HV'\"))\n"
                                                "(rule \"hole\"\n"
                                                "\t(constraint hole_size (min 0.2mm)))\n"
                                                "(rule \"second\"\n"
                                                "\t(constraint clearance (min 0.5mm)))\n" );


int countOf( const wxString& aText, const wxString& aNeedle )
{
    int    count = 0;
    size_t pos = aText.find( aNeedle );

    while( pos != wxString::npos )
    {
        count++;
        pos = aText.find( aNeedle, pos + 1 );
    }

    return count;
}


const wxString ONE_RULE_THREE_CONSTRAINTS = wxS( "(version 2)\n"
                                                 "(rule \"under_fpga\"\n"
                                                 "\t(constraint clearance (min 0.1mm))\n"
                                                 "\t(constraint hole_size (min 0.2mm))\n"
                                                 "\t(constraint via_diameter (min 0.4mm))\n"
                                                 "\t(condition \"A.intersectsArea('underFPGA')\"))\n" );

} // namespace


BOOST_AUTO_TEST_SUITE( DRC_RULE_DOCUMENT_TESTS )


BOOST_AUTO_TEST_CASE( UneditedRulesRoundTripVerbatim )
{
    BOARD             board;
    DRC_RULE_DOCUMENT document( &board );

    document.SetText( TWO_CLEARANCES_AND_A_HOLE );

    BOOST_REQUIRE_EQUAL( document.IndicesFor( MINIMUM_CLEARANCE ).size(), 2 );
    BOOST_REQUIRE_EQUAL( document.IndicesFor( HOLE_SIZE ).size(), 1 );

    wxString text = document.GetText();

    BOOST_CHECK_MESSAGE( text.Contains( wxS( "# keep this comment" ) ), "comment inside a rule was lost:\n" << text );
    BOOST_CHECK_MESSAGE( text.Contains( wxS( "# header comment" ) ), "comment outside the rules was lost:\n" << text );
    BOOST_CHECK( text.Contains( wxS( "(constraint clearance (min 0.3mm))" ) ) );
    BOOST_CHECK( text.Contains( wxS( "(condition \"A.NetClass == 'HV'\")" ) ) );
    BOOST_CHECK( text.StartsWith( wxS( "(version 2)" ) ) );
}


BOOST_AUTO_TEST_CASE( AddingARuleLeavesTheOthersUntouched )
{
    BOARD             board;
    DRC_RULE_DOCUMENT document( &board );

    document.SetText( TWO_CLEARANCES_AND_A_HOLE );

    DRC_RE_BASE_CONSTRAINT_DATA base( document.NextId(), 0, wxS( "added" ) );
    auto                        data = DRC_RULE_EDITOR_UTILS::CreateConstraintData( MINIMUM_CLEARANCE, base );
    data->SetConstraintCode( wxS( "clearance" ) );
    std::dynamic_pointer_cast<DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA>( data )->SetNumericInputValue( 0.7 );

    document.Add( DRC_RE_LOADED_PANEL_ENTRY( MINIMUM_CLEARANCE, data, wxS( "added" ), wxEmptyString,
                                             RPT_SEVERITY_UNDEFINED, LSET() ) );

    BOOST_CHECK( document.IsModified() );
    BOOST_CHECK_EQUAL( document.IndicesFor( MINIMUM_CLEARANCE ).size(), 3 );

    wxString text = document.GetText();

    BOOST_CHECK( text.Contains( wxS( "# keep this comment" ) ) );
    BOOST_CHECK( text.Contains( wxS( "(rule added\n" ) ) );
    BOOST_CHECK( text.Contains( wxS( "(constraint clearance (min 0.7mm))" ) ) );
    BOOST_CHECK_EQUAL( countOf( text, wxS( "(rule " ) ), 4 );
}


BOOST_AUTO_TEST_CASE( MoveSwapsWithTheNeighbourOfTheSameConstraint )
{
    BOARD             board;
    DRC_RULE_DOCUMENT document( &board );

    document.SetText( TWO_CLEARANCES_AND_A_HOLE );

    std::vector<int> clearances = document.IndicesFor( MINIMUM_CLEARANCE );
    BOOST_REQUIRE_EQUAL( clearances.size(), 2 );
    BOOST_CHECK_EQUAL( document.Entries()[clearances[0]].ruleName, wxS( "first" ) );

    BOOST_CHECK( !document.Move( clearances[0], -1, MINIMUM_CLEARANCE ) );
    BOOST_CHECK( document.Move( clearances[1], -1, MINIMUM_CLEARANCE ) );

    clearances = document.IndicesFor( MINIMUM_CLEARANCE );
    BOOST_CHECK_EQUAL( document.Entries()[clearances[0]].ruleName, wxS( "second" ) );
    BOOST_CHECK_EQUAL( document.Entries()[clearances[1]].ruleName, wxS( "first" ) );

    wxString text = document.GetText();
    BOOST_CHECK( text.find( wxS( "(rule \"second\"" ) ) < text.find( wxS( "(rule \"hole\"" ) ) );
    BOOST_CHECK( text.find( wxS( "(rule \"hole\"" ) ) < text.find( wxS( "(rule \"first\"" ) ) );
}


BOOST_AUTO_TEST_CASE( TextOnlyRuleListsUnderEachConstraintItSets )
{
    BOARD             board;
    DRC_RULE_DOCUMENT document( &board );

    DRC_RE_BASE_CONSTRAINT_DATA base( document.NextId(), 0, wxS( "combo" ) );
    auto                        custom = std::make_shared<DRC_RE_CUSTOM_RULE_CONSTRAINT_DATA>( base );
    custom->SetRuleText( wxS( "(constraint clearance (min 0.2mm))\n(constraint hole_size (min 0.3mm))" ) );

    document.Add( DRC_RE_LOADED_PANEL_ENTRY( CUSTOM_RULE, custom, wxS( "combo" ), wxEmptyString, RPT_SEVERITY_UNDEFINED,
                                             LSET() ) );

    BOOST_CHECK_EQUAL( document.IndicesFor( MINIMUM_CLEARANCE ).size(), 1 );
    BOOST_CHECK_EQUAL( document.IndicesFor( HOLE_SIZE ).size(), 1 );
    BOOST_CHECK_EQUAL( document.IndicesFor( MINIMUM_VIA_DIAMETER ).size(), 0 );
    BOOST_CHECK_EQUAL( document.IndicesFor( CUSTOM_RULE ).size(), 1 );
}


BOOST_AUTO_TEST_CASE( ARuleNeverAppliedLeavesNoTrace )
{
    BOARD             board;
    DRC_RULE_DOCUMENT document( &board );

    document.Load();
    BOOST_REQUIRE( !document.IsModified() );

    DRC_RE_BASE_CONSTRAINT_DATA base( document.NextId(), 0, wxS( "draft" ) );
    auto                        data = DRC_RULE_EDITOR_UTILS::CreateConstraintData( MINIMUM_CLEARANCE, base );
    data->SetIsNew( true );

    int index = document.Add( DRC_RE_LOADED_PANEL_ENTRY( MINIMUM_CLEARANCE, data, wxS( "draft" ), wxEmptyString,
                                                         RPT_SEVERITY_UNDEFINED, LSET() ) );

    BOOST_CHECK_EQUAL( document.IndicesFor( MINIMUM_CLEARANCE ).size(), 1 );
    BOOST_CHECK( !document.IsModified() );

    document.Remove( index );

    BOOST_CHECK( !document.IsModified() );
    BOOST_CHECK_EQUAL( document.IndicesFor( MINIMUM_CLEARANCE ).size(), 0 );
}


BOOST_AUTO_TEST_CASE( ImportingFromABoardWithoutARulesFileChangesNothing )
{
    BOARD             board;
    DRC_RULE_DOCUMENT document( &board );

    document.SetText( TWO_CLEARANCES_AND_A_HOLE );
    wxString before = document.GetText();

    wxFileName missing( wxFileName::GetTempDir(), wxS( "no_such_board.kicad_dru" ) );
    BOOST_REQUIRE( !missing.FileExists() );

    document.LoadFrom( missing.GetFullPath() );

    BOOST_CHECK_EQUAL( document.IndicesFor( MINIMUM_CLEARANCE ).size(), 2 );
    BOOST_CHECK_EQUAL( document.GetText(), before );
}


BOOST_AUTO_TEST_CASE( ImportingFromABoardWithRulesReplacesThem )
{
    BOARD             board;
    DRC_RULE_DOCUMENT document( &board );

    document.SetText( TWO_CLEARANCES_AND_A_HOLE );

    wxFileName other( wxFileName::GetTempDir(), wxS( "qa_import_source.kicad_dru" ) );
    wxFFile    file( other.GetFullPath(), wxS( "wb" ) );

    BOOST_REQUIRE( file.IsOpened() );
    file.Write( wxS( "(version 2)\n(rule \"imported\"\n\t(constraint clearance (min 0.9mm)))\n" ) );
    file.Close();

    document.LoadFrom( other.GetFullPath() );

    BOOST_CHECK( document.IsModified() );
    BOOST_CHECK_EQUAL( document.IndicesFor( MINIMUM_CLEARANCE ).size(), 1 );
    BOOST_CHECK_EQUAL( document.IndicesFor( HOLE_SIZE ).size(), 0 );
    BOOST_CHECK( document.GetText().Contains( wxS( "imported" ) ) );

    wxRemoveFile( other.GetFullPath() );
}


BOOST_AUTO_TEST_CASE( RenamingOneConstraintRenamesTheWholeRule )
{
    BOARD             board;
    DRC_RULE_DOCUMENT document( &board );

    document.SetText( ONE_RULE_THREE_CONSTRAINTS );

    int clearance = document.IndicesFor( MINIMUM_CLEARANCE ).front();

    BOOST_REQUIRE_EQUAL( document.IndicesFor( VIA_STYLE ).size(), 1 );

    DRC_RE_LOADED_PANEL_ENTRY edited = document.Entries()[clearance];
    edited.ruleName = wxS( "renamed" );
    document.UpdateEntry( clearance, edited );

    wxString text = document.GetText();

    BOOST_CHECK_MESSAGE( countOf( text, wxS( "(rule " ) ) == 1, "the rule was split in two:\n" << text );
    BOOST_CHECK_MESSAGE( text.Contains( wxS( "(rule renamed" ) ), "the new name was dropped:\n" << text );
    BOOST_CHECK_MESSAGE( !text.Contains( wxS( "under_fpga\"\n" ) ), "the old name survived:\n" << text );
    BOOST_CHECK_EQUAL( countOf( text, wxS( "(constraint " ) ), 3 );

    BOOST_CHECK_EQUAL( document.Entries()[document.IndicesFor( VIA_STYLE ).front()].ruleName, wxS( "renamed" ) );
}


BOOST_AUTO_TEST_CASE( RulesAddedSeparatelyStayApartWhenTheyShareAName )
{
    BOARD             board;
    DRC_RULE_DOCUMENT document( &board );

    for( DRC_RULE_EDITOR_CONSTRAINT_NAME type : { MINIMUM_CLEARANCE, HOLE_SIZE } )
    {
        DRC_RE_BASE_CONSTRAINT_DATA base( document.NextId(), 0, wxS( "same name" ) );
        auto                        data = DRC_RULE_EDITOR_UTILS::CreateConstraintData( type, base );
        data->SetConstraintCode( DRC_RULE_EDITOR_UTILS::ConstraintToKicadDrc( type ) );
        std::dynamic_pointer_cast<DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA>( data )->SetNumericInputValue( 0.3 );

        document.Add( DRC_RE_LOADED_PANEL_ENTRY( type, data, wxS( "same name" ), wxEmptyString, RPT_SEVERITY_UNDEFINED,
                                                 LSET() ) );
    }

    wxString text = document.GetText();

    BOOST_CHECK_MESSAGE( countOf( text, wxS( "(rule " ) ) == 2, "two unrelated rules were merged:\n" << text );
}


BOOST_AUTO_TEST_CASE( TextRuleMovesAmongTheRulesOfThePageItIsShownOn )
{
    BOARD             board;
    DRC_RULE_DOCUMENT document( &board );

    document.SetText( TWO_CLEARANCES_AND_A_HOLE );

    DRC_RE_BASE_CONSTRAINT_DATA base( document.NextId(), 0, wxS( "combo" ) );
    auto                        custom = std::make_shared<DRC_RE_CUSTOM_RULE_CONSTRAINT_DATA>( base );
    custom->SetRuleText( wxS( "(constraint clearance (min 0.2mm))\n(constraint hole_size (min 0.3mm))" ) );

    int combo = document.Add( DRC_RE_LOADED_PANEL_ENTRY( CUSTOM_RULE, custom, wxS( "combo" ), wxEmptyString,
                                                         RPT_SEVERITY_UNDEFINED, LSET() ) );

    std::vector<int> clearances = document.IndicesFor( MINIMUM_CLEARANCE );
    BOOST_REQUIRE_EQUAL( clearances.size(), 3 );
    BOOST_CHECK_EQUAL( clearances[2], combo );

    BOOST_CHECK( document.Move( combo, -1, MINIMUM_CLEARANCE ) );

    clearances = document.IndicesFor( MINIMUM_CLEARANCE );
    BOOST_CHECK_EQUAL( document.Entries()[clearances[1]].ruleName, wxS( "combo" ) );
    BOOST_CHECK_EQUAL( document.Entries()[clearances[2]].ruleName, wxS( "second" ) );
}


BOOST_AUTO_TEST_CASE( ATextRuleIsListedOnceWhenTwoSectionsShareAConstraintCode )
{
    BOARD             board;
    DRC_RULE_DOCUMENT document( &board );

    // The optimum sits outside the window, so no graphical panel can model this rule.
    document.SetText( wxS( "(version 2)\n"
                           "(rule \"byte2\"\n"
                           "\t(constraint length (min 39mm) (max 40mm) (opt 40.5mm))\n"
                           "\t(condition \"A.NetClass == 'DDR4'\"))\n" ) );

    BOOST_REQUIRE_EQUAL( document.IndicesFor( CUSTOM_RULE ).size(), 1 );

    BOOST_CHECK_MESSAGE( document.IndicesFor( ABSOLUTE_LENGTH ).size() == 1,
                         "a plain length rule belongs under absolute length" );
    BOOST_CHECK_MESSAGE( document.IndicesFor( MATCHED_LENGTH_DIFF_PAIR ).empty(),
                         "the same rule was also listed under matched length on the same page" );
}


BOOST_AUTO_TEST_CASE( TextThatDoesNotParseIsKeptAsWritten )
{
    BOARD             board;
    DRC_RULE_DOCUMENT document( &board );

    document.SetText( TWO_CLEARANCES_AND_A_HOLE );

    wxString broken = wxS( "(version 2)\n(rule \"unfinished\"\n\t(constraint clearance (min 0.3mm)" );
    document.SetText( broken );

    BOOST_CHECK( document.IsModified() );
    BOOST_CHECK( document.HasUnparsedText() );
    BOOST_CHECK_EQUAL( document.GetText(), broken );

    document.SetText( TWO_CLEARANCES_AND_A_HOLE );

    BOOST_CHECK( !document.HasUnparsedText() );
    BOOST_CHECK_EQUAL( document.IndicesFor( MINIMUM_CLEARANCE ).size(), 2 );
    BOOST_CHECK( document.GetText().Contains( wxS( "(rule \"hole\"" ) ) );
}


BOOST_AUTO_TEST_CASE( ConstraintCodeMustEndAtADelimiter )
{
    wxString text = wxS( "(constraint clearance_under_fpga (min 1mm))\n(constraint clearance (min 2mm))" );

    BOOST_CHECK_EQUAL( DRC_RULE_DOCUMENT::FindConstraintClause( text, wxS( "clearance" ) ),
                       text.find( wxS( "(constraint clearance (" ) ) );
    BOOST_CHECK_EQUAL(
            DRC_RULE_DOCUMENT::FindConstraintClause( wxS( "(constraint hole_size (min 1mm))" ), wxS( "hole" ) ),
            wxString::npos );
}


BOOST_AUTO_TEST_CASE( DuplicateFollowsItsOriginalAsAnIndependentRule )
{
    BOARD             board;
    DRC_RULE_DOCUMENT document( &board );

    document.SetText( TWO_CLEARANCES_AND_A_HOLE );

    std::vector<int> clearances = document.IndicesFor( MINIMUM_CLEARANCE );
    BOOST_REQUIRE_EQUAL( clearances.size(), 2 );
    BOOST_REQUIRE_EQUAL( document.Entries()[clearances[0]].ruleName, wxS( "first" ) );

    int copy = document.Duplicate( clearances[0] );

    BOOST_REQUIRE_EQUAL( copy, clearances[0] + 1 );
    BOOST_CHECK( document.IsModified() );

    clearances = document.IndicesFor( MINIMUM_CLEARANCE );
    BOOST_REQUIRE_EQUAL( clearances.size(), 3 );
    BOOST_CHECK_EQUAL( document.Entries()[clearances[0]].ruleName, wxS( "first" ) );
    BOOST_CHECK_EQUAL( document.Entries()[clearances[1]].ruleName, wxS( "first 1" ) );

    BOOST_CHECK_MESSAGE( document.Entries()[clearances[0]].constraintData
                                 != document.Entries()[clearances[1]].constraintData,
                         "the copy shares its data with the original" );

    auto original = std::dynamic_pointer_cast<DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA>(
            document.Entries()[clearances[0]].constraintData );
    auto duplicated = std::dynamic_pointer_cast<DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA>(
            document.Entries()[clearances[1]].constraintData );

    BOOST_REQUIRE( original );
    BOOST_REQUIRE( duplicated );
    BOOST_CHECK_CLOSE( duplicated->GetNumericInputValue(), original->GetNumericInputValue(), 0.0001 );
    BOOST_CHECK_EQUAL( duplicated->GetRuleCondition(), original->GetRuleCondition() );

    wxString text = document.GetText();

    BOOST_CHECK_MESSAGE( countOf( text, wxS( "(rule " ) ) == 4, "the copy was not written once:\n" << text );
    // The comment inside the rule belongs to the rule and travels with the copy. The one outside
    // the rules belongs to the file and must not.
    BOOST_CHECK_MESSAGE( countOf( text, wxS( "# keep this comment" ) ) == 2,
                         "the copy lost the comment written inside the rule:\n"
                                 << text );
    BOOST_CHECK_MESSAGE( countOf( text, wxS( "# header comment" ) ) == 1,
                         "the file header was emitted more than once:\n"
                                 << text );

    DRC_RULE_DOCUMENT reread( &board );
    reread.SetText( text );

    BOOST_CHECK_MESSAGE( reread.IndicesFor( MINIMUM_CLEARANCE ).size() == 3, "the saved copy does not read back:\n"
                                                                                     << text );
}


BOOST_AUTO_TEST_CASE( DuplicatingACopyCountsOnFromTheOriginalName )
{
    BOARD             board;
    DRC_RULE_DOCUMENT document( &board );

    document.SetText( TWO_CLEARANCES_AND_A_HOLE );

    int first = document.Duplicate( document.IndicesFor( MINIMUM_CLEARANCE ).front() );
    BOOST_REQUIRE_GE( first, 0 );
    BOOST_CHECK_EQUAL( document.Entries()[first].ruleName, wxS( "first 1" ) );

    int second = document.Duplicate( first );
    BOOST_REQUIRE_GE( second, 0 );
    BOOST_CHECK_EQUAL( document.Entries()[second].ruleName, wxS( "first 2" ) );
}


BOOST_AUTO_TEST_CASE( RuleNamesAreUniqueWithinAConstraint )
{
    BOARD             board;
    DRC_RULE_DOCUMENT document( &board );

    document.SetText( TWO_CLEARANCES_AND_A_HOLE );

    std::vector<int> clearances = document.IndicesFor( MINIMUM_CLEARANCE );
    int              hole = document.IndicesFor( HOLE_SIZE ).front();

    BOOST_CHECK( !document.IsRuleNameUnique( wxS( "second" ), MINIMUM_CLEARANCE, clearances[0] ) );
    BOOST_CHECK( document.IsRuleNameUnique( wxS( "second" ), MINIMUM_CLEARANCE, clearances[1] ) );
    BOOST_CHECK( document.IsRuleNameUnique( wxS( "second" ), HOLE_SIZE, hole ) );

    BOOST_CHECK( !document.IsRuleNameUnique( wxS( "second" ), MINIMUM_CLEARANCE, -1 ) );
    BOOST_CHECK( document.IsRuleNameUnique( wxS( "second" ), HOLE_SIZE, -1 ) );
    BOOST_CHECK( document.IsRuleNameUnique( wxS( "third" ), MINIMUM_CLEARANCE, -1 ) );

    BOOST_CHECK_EQUAL( document.UniqueRuleName( wxS( "Minimum clearance" ) ), wxS( "Minimum clearance 1" ) );
    BOOST_CHECK_EQUAL( document.UniqueRuleName( wxS( "first" ) ), wxS( "first 1" ) );
}


BOOST_AUTO_TEST_CASE( EveryBoardConstraintWithRulesResolvesToAPage )
{
    // The names are the ones PANEL_SETUP_CONSTRAINTS keys its fields by.
    const std::vector<wxString> governed = { wxS( "min_clearance" ),
                                             wxS( "min_connection" ),
                                             wxS( "min_track_width" ),
                                             wxS( "min_via_annular_width" ),
                                             wxS( "min_via_diameter" ),
                                             wxS( "min_through_hole_diameter" ),
                                             wxS( "min_hole_to_hole" ),
                                             wxS( "min_hole_clearance" ),
                                             wxS( "min_silk_clearance" ),
                                             wxS( "min_groove_width" ),
                                             wxS( "min_text_height" ),
                                             wxS( "min_text_thickness" ),
                                             wxS( "min_copper_edge_clearance" ) };

    for( const wxString& setting : governed )
    {
        DRC_RULE_EDITOR_CONSTRAINT_NAME type = CUSTOM_RULE;
        wxString                        page;

        BOOST_CHECK_MESSAGE( PANEL_SETUP_CONSTRAINT_RULES::RulesCanOverride( setting, &type, &page ),
                             "no rule page governs " << setting.ToStdString() );
        BOOST_CHECK_MESSAGE( !page.IsEmpty(), "no page title for " << setting.ToStdString() );
    }

    // Max error is a geometry tolerance, not something a rule can override.
    BOOST_CHECK( !PANEL_SETUP_CONSTRAINT_RULES::RulesCanOverride( wxS( "max_error" ), nullptr, nullptr ) );
}


BOOST_AUTO_TEST_SUITE_END()
