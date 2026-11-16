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
#include <drc/rule_editor/drc_re_panel_matcher.h>
#include <drc/rule_editor/drc_re_via_style_constraint_data.h>
#include <drc/rule_editor/drc_rule_document.h>
#include <drc/rule_editor/drc_rule_editor_enums.h>
#include <drc/rule_editor/drc_rule_editor_utils.h>
#include <dialogs/panel_setup_constraint_rules.h>
#include <dialogs/panel_setup_constraints.h>
#include <dialogs/panel_setup_mask_and_paste.h>

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

    // Adding and removing a rule sends the document through the saver with every rule unedited
    DRC_RE_BASE_CONSTRAINT_DATA base( document.NextId(), wxS( "added" ) );
    auto                        data = DRC_RULE_EDITOR_UTILS::CreateConstraintData( MINIMUM_CLEARANCE, base );

    int added = document.Add( DRC_RE_LOADED_PANEL_ENTRY( MINIMUM_CLEARANCE, data, wxS( "added" ), wxEmptyString,
                                                         RPT_SEVERITY_UNDEFINED, LSET() ) );
    document.Remove( added );

    BOOST_CHECK_EQUAL( document.GetText(), TWO_CLEARANCES_AND_A_HOLE );
}


BOOST_AUTO_TEST_CASE( AddingARuleLeavesTheOthersUntouched )
{
    BOARD             board;
    DRC_RULE_DOCUMENT document( &board );

    document.SetText( TWO_CLEARANCES_AND_A_HOLE );

    DRC_RE_BASE_CONSTRAINT_DATA base( document.NextId(), wxS( "added" ) );
    auto                        data = DRC_RULE_EDITOR_UTILS::CreateConstraintData( MINIMUM_CLEARANCE, base );
    data->SetConstraintCode( wxS( "clearance" ) );
    std::dynamic_pointer_cast<DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA>( data )->SetNumericInputValue( 0.7 );

    document.Add( DRC_RE_LOADED_PANEL_ENTRY( MINIMUM_CLEARANCE, data, wxS( "added" ), wxEmptyString,
                                             RPT_SEVERITY_UNDEFINED, LSET() ) );

    BOOST_CHECK( document.IsModified() );
    BOOST_CHECK_EQUAL( document.IndicesFor( MINIMUM_CLEARANCE ).size(), 3 );

    wxString text = document.GetText();

    BOOST_CHECK( text.StartsWith( TWO_CLEARANCES_AND_A_HOLE ) );
    BOOST_CHECK( text.Contains( wxS( "(rule added\n" ) ) );
    BOOST_CHECK( text.Contains( wxS( "(constraint clearance (min 0.7mm))" ) ) );
    BOOST_CHECK_EQUAL( countOf( text, wxS( "(rule " ) ), 4 );
}


BOOST_AUTO_TEST_CASE( AMergedRuleIsWrittenBackAsOneRule )
{
    BOARD             board;
    DRC_RULE_DOCUMENT document( &board );

    // The two constraints of "merged" are listed on separate pages, so editing one has to write
    // them back under one rule rather than one rule each
    document.SetText( wxS( "(version 2)\n"
                           "(rule \"merged\"\n"
                           "\t(constraint clearance (min 0.3mm))\n"
                           "\t(constraint hole_size (min 0.2mm)))\n"
                           "(rule \"other\"\n"
                           "\t(constraint clearance (min 0.5mm)))\n" ) );

    BOOST_REQUIRE_EQUAL( document.IndicesFor( MINIMUM_CLEARANCE ).size(), 2 );
    BOOST_REQUIRE_EQUAL( document.IndicesFor( HOLE_SIZE ).size(), 1 );

    int                       holeIndex = document.IndicesFor( HOLE_SIZE ).front();
    DRC_RE_LOADED_PANEL_ENTRY hole = document.Entries()[holeIndex];

    std::dynamic_pointer_cast<DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA>( hole.constraintData )
            ->SetNumericInputValue( 0.25 );
    document.UpdateEntry( holeIndex, hole );

    wxString text = document.GetText();

    BOOST_CHECK_EQUAL( countOf( text, wxS( "(rule " ) ), 2 );
    BOOST_CHECK_EQUAL( countOf( text, wxS( "merged" ) ), 1 );
    BOOST_CHECK_MESSAGE( text.Contains( wxS( "(constraint clearance (min 0.3mm))" ) )
                                 && text.Contains( wxS( "(constraint hole_size (min 0.25mm))" ) ),
                         "the merged rule lost a constraint:\n" << text );
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

    // The last rule of a constraint has nowhere to go, and moving the other one down puts the
    // pair back as they started.
    BOOST_CHECK( !document.Move( clearances[1], 1, MINIMUM_CLEARANCE ) );
    BOOST_CHECK( document.Move( clearances[0], 1, MINIMUM_CLEARANCE ) );

    clearances = document.IndicesFor( MINIMUM_CLEARANCE );
    BOOST_CHECK_EQUAL( document.Entries()[clearances[0]].ruleName, wxS( "first" ) );
    BOOST_CHECK_EQUAL( document.Entries()[clearances[1]].ruleName, wxS( "second" ) );

    text = document.GetText();
    BOOST_CHECK( text.find( wxS( "(rule \"first\"" ) ) < text.find( wxS( "(rule \"hole\"" ) ) );
    BOOST_CHECK( text.find( wxS( "(rule \"hole\"" ) ) < text.find( wxS( "(rule \"second\"" ) ) );
}


BOOST_AUTO_TEST_CASE( TextOnlyRuleListsUnderEachConstraintItSets )
{
    BOARD             board;
    DRC_RULE_DOCUMENT document( &board );

    DRC_RE_BASE_CONSTRAINT_DATA base( document.NextId(), wxS( "combo" ) );
    auto                        custom = std::make_shared<DRC_RE_CUSTOM_RULE_CONSTRAINT_DATA>( base );
    custom->SetRuleText( wxS( "(constraint clearance (min 0.2mm))\n(constraint hole_size (min 0.3mm))" ) );

    document.Add( DRC_RE_LOADED_PANEL_ENTRY( CUSTOM_RULE, custom, wxS( "combo" ), wxEmptyString, RPT_SEVERITY_UNDEFINED,
                                             LSET() ) );

    BOOST_CHECK_EQUAL( document.IndicesFor( MINIMUM_CLEARANCE ).size(), 1 );
    BOOST_CHECK_EQUAL( document.IndicesFor( HOLE_SIZE ).size(), 1 );
    BOOST_CHECK_EQUAL( document.IndicesFor( MINIMUM_VIA_DIAMETER ).size(), 0 );
    BOOST_CHECK_EQUAL( document.IndicesFor( CUSTOM_RULE ).size(), 1 );
}


BOOST_AUTO_TEST_CASE( ImportingFromABoardWithoutARulesFileChangesNothing )
{
    BOARD             board;
    DRC_RULE_DOCUMENT document( &board );

    document.SetText( TWO_CLEARANCES_AND_A_HOLE );
    wxString before = document.GetText();

    wxFileName missing( wxFileName::CreateTempFileName( wxS( "qa_missing" ) ) );
    wxRemoveFile( missing.GetFullPath() );
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

    wxFileName other( wxFileName::CreateTempFileName( wxS( "qa_import_source" ) ) );
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


BOOST_AUTO_TEST_CASE( TypographicQuotesInAFileOnDiskStillParse )
{
    BOARD             board;
    DRC_RULE_DOCUMENT document( &board );

    // A rules file written outside KiCad can carry typographic quotes, which the parser does not
    // accept, so reading one has to normalise them.
    const wxString smartQuoted = L"(version 2)\n"
                                 L"(rule \u201Coutside\u201D\n"
                                 L"\t(constraint clearance (min 0.3mm))\n"
                                 L"\t(condition \u201CA.NetClass == \u2018HV\u2019\u201D))\n";

    wxFileName quoted( wxFileName::CreateTempFileName( wxS( "qa_smart_quotes" ) ) );
    wxFFile    file( quoted.GetFullPath(), wxS( "wb" ) );

    BOOST_REQUIRE( file.IsOpened() );
    file.Write( smartQuoted, wxConvUTF8 );
    file.Close();

    document.LoadFrom( quoted.GetFullPath() );

    BOOST_CHECK( !document.HasUnparsedText() );
    BOOST_CHECK_EQUAL( document.IndicesFor( MINIMUM_CLEARANCE ).size(), 1 );
    BOOST_CHECK( document.GetText().Contains( wxS( "\"outside\"" ) ) );

    wxRemoveFile( quoted.GetFullPath() );
}


BOOST_AUTO_TEST_CASE( ARangeWithOneEndIsNotGivenAnInventedBound )
{
    // An absent bound reads back as the largest int, so a rule that names one end of a range
    // cannot go to a panel that writes both ends. Each of these reaches a different panel.
    const std::vector<wxString> openEnded = {
        wxS( "(constraint length (min 10mm))" ),
        wxS( "(constraint length (min 10mm))\n\t(constraint skew (max 1mm))" ),
        wxS( "(constraint skew (min 1mm))" ),
        wxS( "(constraint diff_pair_uncoupled (min 1mm))" ),
    };

    for( const wxString& clauses : openEnded )
    {
        BOARD             board;
        DRC_RULE_DOCUMENT document( &board );

        document.SetText( wxS( "(version 2)\n(rule \"open_ended\"\n\t" ) + clauses + wxS( ")\n" ) );

        BOOST_REQUIRE_MESSAGE( !document.Entries().empty(), "nothing loaded for " << clauses );

        DRC_RE_LOADED_PANEL_ENTRY edited = document.Entries()[0];
        edited.ruleName = wxS( "renamed" );
        document.UpdateEntry( 0, edited );

        wxString text = document.GetText();

        BOOST_CHECK_MESSAGE( !text.Contains( wxS( "2147" ) ),
                             "an absent bound was written back as the largest int, from " << clauses << ":\n"
                                                                                          << text );
        BOOST_CHECK_MESSAGE( text.Contains( clauses ), "the clauses were rewritten, from " << clauses << ":\n"
                                                                                           << text );
    }
}


BOOST_AUTO_TEST_CASE( ARangeWithBothEndsStillReachesItsPanel )
{
    BOARD             board;
    DRC_RULE_DOCUMENT document( &board );

    document.SetText( wxS( "(version 2)\n"
                           "(rule \"both_ends\"\n"
                           "\t(constraint length (min 10mm) (max 20mm)))\n"
                           "(rule \"uncoupled\"\n"
                           "\t(constraint diff_pair_uncoupled (max 5mm)))\n"
                           "(rule \"skew\"\n"
                           "\t(constraint skew (max 1mm)))\n" ) );

    BOOST_CHECK_EQUAL( document.IndicesFor( ABSOLUTE_LENGTH ).size(), 1 );
    BOOST_CHECK_EQUAL( document.IndicesFor( ROUTING_DIFF_PAIR ).size(), 1 );
    BOOST_CHECK_EQUAL( document.IndicesFor( MATCHED_LENGTH_DIFF_PAIR ).size(), 1 );
    BOOST_CHECK_EQUAL( document.IndicesFor( CUSTOM_RULE ).size(), 0 );
}


BOOST_AUTO_TEST_CASE( AnEditedTextRuleKeepsItsIndent )
{
    BOARD             board;
    DRC_RULE_DOCUMENT document( &board );

    document.SetText( wxS( "(version 2)\n"
                           "(rule \"min_only\"\n"
                           "\t(condition \"A.NetClass == 'Default'\")\n"
                           "\t(constraint track_width (min 0.2mm)))\n" ) );

    BOOST_REQUIRE_EQUAL( document.IndicesFor( CUSTOM_RULE ).size(), 1 );

    int                       index = document.IndicesFor( CUSTOM_RULE ).front();
    DRC_RE_LOADED_PANEL_ENTRY edited = document.Entries()[index];

    edited.ruleName = wxS( "renamed" );
    document.UpdateEntry( index, edited );

    wxString text = document.GetText();

    BOOST_CHECK_MESSAGE( text.Contains( wxS( "(rule \"renamed\"\n"
                                             "\t(condition \"A.NetClass == 'Default'\")\n"
                                             "\t(constraint track_width (min 0.2mm)))" ) ),
                         "the body lost its indent:\n"
                                 << text );
}


BOOST_AUTO_TEST_CASE( AWidthOrGapMissingABoundIsKeptAsWritten )
{
    // The width and gap panels write min, opt and max around one optimum, so a rule naming fewer
    // would come back with every wider track or gap an error
    const std::vector<wxString> partial = {
        wxS( "(constraint track_width (min 0.2mm))" ),
        wxS( "(constraint track_width (min 0.2mm) (max 0.4mm))" ),
        wxS( "(constraint diff_pair_gap (min 0.1mm))" ),
    };

    for( const wxString& clauses : partial )
    {
        BOARD             board;
        DRC_RULE_DOCUMENT document( &board );

        document.SetText( wxS( "(version 2)\n(rule \"partial\"\n\t" ) + clauses + wxS( ")\n" ) );

        BOOST_REQUIRE_MESSAGE( !document.Entries().empty(), "nothing loaded for " << clauses );

        DRC_RE_LOADED_PANEL_ENTRY edited = document.Entries()[0];
        edited.ruleName = wxS( "renamed" );
        document.UpdateEntry( 0, edited );

        wxString text = document.GetText();

        BOOST_CHECK_MESSAGE( text.Contains( clauses ), "the clauses were rewritten, from " << clauses << ":\n"
                                                                                           << text );
    }
}


BOOST_AUTO_TEST_CASE( ASplitRuleKeepsThePredicateItsPartsShare )
{
    // One part of each takes the predicate out of the condition all of the parts are read with
    const std::vector<std::pair<wxString, wxString>> rules = {
        { wxS( "(constraint via_diameter (min 0.2mm))\n\t(constraint hole_size (min 0.1mm))" ),
          wxS( "A.Via_Type == 'Micro' && A.NetClass == 'HS'" ) },
        { wxS( "(constraint disallow via)" ), wxS( "A.Pad_Type == 'SMD' && A.NetClass == 'HS'" ) },
    };

    for( const auto& [clauses, condition] : rules )
    {
        for( size_t i = 0;; ++i )
        {
            BOARD             board;
            DRC_RULE_DOCUMENT document( &board );

            document.SetText( wxS( "(version 2)\n(rule \"split\"\n\t" ) + clauses
                              + wxS( "\n\t(constraint clearance (min 0.15mm))\n\t(condition \"" ) + condition
                              + wxS( "\"))\n" ) );

            if( i >= document.Entries().size() )
                break;

            // As the rule page applies an edit, with the condition its form holds
            DRC_RE_LOADED_PANEL_ENTRY edited = document.Entries()[i];
            edited.ruleName = wxS( "renamed" );
            edited.condition = edited.constraintData->GetRuleCondition();
            document.UpdateEntry( (int) i, edited );

            wxString text = document.GetText();

            BOOST_CHECK_MESSAGE( text.Contains( wxS( "(condition \"" ) + condition + wxS( "\")" ) ),
                                 "editing part " << i << " changed the condition:\n"
                                                 << text );
        }
    }
}


BOOST_AUTO_TEST_CASE( AConditionThePanelCannotWriteBackIsKeptAsWritten )
{
    // A panel puts its predicate back in front of the rest, so only that form survives an edit
    const std::vector<wxString> conditions = {
        wxS( "A.Via_Type != 'Micro'" ),
        wxS( "A.Via_Type == 'Micro' || A.Via_Type == 'Blind'" ),
        wxS( "A.NetClass == 'HS' && A.Via_Type == 'Micro'" ),
        wxS( "A.NetName == 'Micro'" ),
    };

    for( const wxString& condition : conditions )
    {
        BOARD             board;
        DRC_RULE_DOCUMENT document( &board );

        document.SetText( wxS( "(version 2)\n(rule \"vias\"\n"
                               "\t(constraint via_diameter (min 0.2mm))\n"
                               "\t(constraint hole_size (min 0.1mm))\n"
                               "\t(condition \"" )
                          + condition + wxS( "\"))\n" ) );

        BOOST_REQUIRE_EQUAL( document.Entries().size(), 1 );

        DRC_RE_LOADED_PANEL_ENTRY edited = document.Entries()[0];
        edited.ruleName = wxS( "renamed" );
        edited.condition = edited.constraintData->GetRuleCondition();
        document.UpdateEntry( 0, edited );

        wxString text = document.GetText();

        BOOST_CHECK_MESSAGE( text.Contains( wxS( "(condition \"" ) + condition + wxS( "\")" ) ),
                             "the condition was rewritten, from " << condition << ":\n"
                                                                  << text );
    }
}


BOOST_AUTO_TEST_CASE( AViaTypeChosenForASplitRuleNarrowsOnlyTheViaPart )
{
    BOARD             board;
    DRC_RULE_DOCUMENT document( &board );

    document.SetText( ONE_RULE_THREE_CONSTRAINTS );

    BOOST_REQUIRE_EQUAL( document.IndicesFor( VIA_STYLE ).size(), 1 );

    int                       index = document.IndicesFor( VIA_STYLE ).front();
    DRC_RE_LOADED_PANEL_ENTRY edited = document.Entries()[index];

    std::static_pointer_cast<DRC_RE_VIA_STYLE_CONSTRAINT_DATA>( edited.constraintData )
            ->SetViaType( VIA_STYLE_TYPE::MICRO );
    edited.condition = edited.constraintData->GetRuleCondition();
    document.UpdateEntry( index, edited );

    wxString text = document.GetText();

    BOOST_CHECK_MESSAGE(
            text.Contains( wxS( "(condition \"A.Via_Type == 'Micro' && A.intersectsArea('underFPGA')\")" ) ),
            "the via type was dropped:\n"
                    << text );

    // The clearance part still applies to every item in the area
    BOOST_CHECK_MESSAGE( text.Contains( wxS( "(condition \"A.intersectsArea('underFPGA')\")" ) ),
                         "the via type narrowed the other parts:\n"
                                 << text );
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
        DRC_RE_BASE_CONSTRAINT_DATA base( document.NextId(), wxS( "same name" ) );
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

    DRC_RE_BASE_CONSTRAINT_DATA base( document.NextId(), wxS( "combo" ) );
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

    // The optimum sits outside the window, so no graphical panel can hold this rule.
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
    // A comment inside the rule travels with the copy. One outside the rules belongs to the
    // file and does not.
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


BOOST_AUTO_TEST_CASE( SplitRuleIsListedOnceOnItsSiblingsPage )
{
    BOARD             board;
    DRC_RULE_DOCUMENT document( &board );

    // The zone connection has no graphical panel, so the whole rule is kept as text
    document.SetText( wxS( "(version 2)\n"
                           "(rule \"split\"\n"
                           "\t(constraint clearance (min 0.2mm))\n"
                           "\t(constraint zone_connection solid))\n" ) );

    BOOST_REQUIRE_EQUAL( document.Entries().size(), 1 );

    std::vector<int> clearances = document.IndicesFor( MINIMUM_CLEARANCE );

    BOOST_REQUIRE_EQUAL( clearances.size(), 1 );
    BOOST_CHECK_EQUAL( document.Entries()[clearances[0]].panelType, CUSTOM_RULE );
}


BOOST_AUTO_TEST_CASE( EveryBoardConstraintWithRulesResolvesToAPage )
{
    // These are the names PANEL_SETUP_CONSTRAINTS keys its fields by.
    const std::vector<wxString> governed = { wxS( "min_clearance" ),      wxS( "min_connection" ),
                                             wxS( "min_track_width" ),    wxS( "min_via_annular_width" ),
                                             wxS( "min_via_diameter" ),   wxS( "min_through_hole_diameter" ),
                                             wxS( "min_hole_to_hole" ),   wxS( "min_hole_clearance" ),
                                             wxS( "min_silk_clearance" ), wxS( "min_text_height" ),
                                             wxS( "min_silk_line_width" ), wxS( "min_copper_edge_clearance" ) };

    DRC_PANEL_MATCHER matcher;

    for( const wxString& setting : governed )
    {
        DRC_CONSTRAINT_T constraint = NULL_CONSTRAINT;

        BOOST_CHECK_MESSAGE( PANEL_SETUP_CONSTRAINT_RULES::RulesCanOverride( setting, &constraint ),
                             "no rule constraint governs " << setting.ToStdString() );
        BOOST_CHECK_MESSAGE( !PANEL_SETUP_CONSTRAINT_RULES::PageTitleFor( matcher.GetHomePanel( constraint ) ).IsEmpty(),
                             "no page title for " << setting.ToStdString() );
    }

    // Max error is a geometry tolerance. The groove width decides which board edge gaps a
    // creepage path has to follow, so neither is a value a rule can override.
    BOOST_CHECK( !PANEL_SETUP_CONSTRAINT_RULES::RulesCanOverride( wxS( "max_error" ), nullptr ) );
    BOOST_CHECK( !PANEL_SETUP_CONSTRAINT_RULES::RulesCanOverride( wxS( "min_groove_width" ), nullptr ) );
}


BOOST_AUTO_TEST_CASE( EveryBoardDefaultIsWrittenByOnePageOnly )
{
    const std::vector<std::pair<wxString, CONSTRAINT_DEFAULT_FIELD>> edited =
            PANEL_SETUP_CONSTRAINT_RULES::EditedDefaults();

    // Two pages writing one value would make the result depend on which was left last
    for( size_t i = 0; i < edited.size(); ++i )
    {
        for( size_t j = i + 1; j < edited.size(); ++j )
        {
            BOOST_CHECK_MESSAGE( edited[i].second.settingName != edited[j].second.settingName,
                                 "two rule pages edit " << edited[i].second.settingName.ToStdString() );
            BOOST_CHECK_MESSAGE( edited[i].second.member != edited[j].second.member,
                                 "two rule pages write the member behind "
                                         << edited[i].second.settingName.ToStdString() );
        }
    }

    // A value a rule page edits names that page, so the page it left can link to it
    for( const auto& [group, field] : edited )
    {
        BOOST_CHECK_MESSAGE( !PANEL_SETUP_CONSTRAINT_RULES::PageEditingDefault( field.settingName ).IsEmpty(),
                             "no page edits " << field.settingName.ToStdString() );
    }
}


BOOST_AUTO_TEST_CASE( EveryConstraintWithARulePageHasItsDefaultThere )
{
    // Each of these is a whole board default for the constraint its rule page is about, even
    // where the page it has always been set on holds it in a control with something else
    for( const wxString& setting :
         { wxS( "solder_mask_expansion" ), wxS( "solder_mask_min_width" ), wxS( "solder_paste_margin" ) } )
    {
        BOOST_CHECK_MESSAGE( !PANEL_SETUP_CONSTRAINT_RULES::PageEditingDefault( setting ).IsEmpty(),
                             "no rule page holds " << setting.ToStdString() );
    }

    // Max error is a geometry tolerance and the groove width feeds the creepage search, so
    // neither is a constraint a rule page could hold
    BOOST_CHECK( PANEL_SETUP_CONSTRAINT_RULES::PageEditingDefault( wxS( "max_error" ) ).IsEmpty() );
    BOOST_CHECK( PANEL_SETUP_CONSTRAINT_RULES::PageEditingDefault( wxS( "min_groove_width" ) ).IsEmpty() );
}


BOOST_AUTO_TEST_CASE( EachDefaultTravelsWithItsImportGroup )
{
    using BDS = BOARD_DESIGN_SETTINGS;
    using COPY = void ( * )( const BDS&, BDS& );

    // Board Setup imports a group and commits its staged values with the same copy, so a value
    // the copy misses is neither imported nor saved
    const std::map<wxString, COPY> copies = { { wxS( "Constraints" ), &PANEL_SETUP_CONSTRAINTS::CopySettings },
                                              { wxS( "Solder Mask/Paste" ),
                                                &PANEL_SETUP_MASK_AND_PASTE::CopySettings } };

    auto travels = []( COPY aCopy, int BDS::* aMember )
    {
        BOARD from;
        BOARD to;

        from.GetDesignSettings().*aMember = to.GetDesignSettings().*aMember + 1234;
        aCopy( from.GetDesignSettings(), to.GetDesignSettings() );

        return to.GetDesignSettings().*aMember == from.GetDesignSettings().*aMember;
    };

    for( const auto& [group, field] : PANEL_SETUP_CONSTRAINT_RULES::EditedDefaults() )
    {
        auto it = copies.find( group );

        BOOST_REQUIRE_MESSAGE( it != copies.end(), "no import carries group " << group.ToStdString() );
        BOOST_CHECK_MESSAGE( travels( it->second, field.member ),
                             field.settingName.ToStdString() << " does not travel with " << group.ToStdString() );
    }

    // No rule page holds these two, so only the Constraints page carries them
    BOOST_CHECK( travels( &PANEL_SETUP_CONSTRAINTS::CopySettings, &BDS::m_MaxError ) );
    BOOST_CHECK( travels( &PANEL_SETUP_CONSTRAINTS::CopySettings, &BDS::m_MinGrooveWidth ) );
}


BOOST_AUTO_TEST_CASE( AConditionedDefaultSitsWithTheConstraintItNarrows )
{
    // A microvia carries its own minimum for two constraints that are not microvia specific, so
    // those two values belong on the page holding the rules for the constraint they narrow
    BOOST_CHECK_EQUAL( PANEL_SETUP_CONSTRAINT_RULES::PageEditingDefault( wxS( "min_microvia_diameter" ) ),
                       PANEL_SETUP_CONSTRAINT_RULES::PageEditingDefault( wxS( "min_via_diameter" ) ) );
    BOOST_CHECK_EQUAL( PANEL_SETUP_CONSTRAINT_RULES::PageEditingDefault( wxS( "min_microvia_drill" ) ),
                       PANEL_SETUP_CONSTRAINT_RULES::PageEditingDefault( wxS( "min_through_hole_diameter" ) ) );

    BOOST_CHECK( !PANEL_SETUP_CONSTRAINT_RULES::PageEditingDefault( wxS( "min_microvia_diameter" ) ).IsEmpty() );
    BOOST_CHECK( !PANEL_SETUP_CONSTRAINT_RULES::PageEditingDefault( wxS( "min_resolved_spokes" ) ).IsEmpty() );
}


BOOST_AUTO_TEST_SUITE_END()
