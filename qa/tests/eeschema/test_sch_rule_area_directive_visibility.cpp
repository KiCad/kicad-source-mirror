/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one at
 * http://www.gnu.org/licenses/
 */

/**
 * @file test_sch_rule_area_directive_visibility.cpp
 *
 * Regression test for issue 23597.  When "Show Directive Labels" is off, a rule area that exists
 * only to carry directive labels must follow the labels and become both invisible and unselectable.
 * A rule area which also applies DNP or one of the exclude-from flags has a design effect of its
 * own and must stay visible, so it does not qualify as a directive-label-only area.
 * SCH_RULE_AREA::IsDirectiveLabelOnlyArea() is the shared gate that both the painter and the
 * selection tool consult, so the visibility contract is exercised through it.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <sch_label.h>
#include <sch_rule_area.h>


BOOST_AUTO_TEST_SUITE( SchRuleAreaDirectiveVisibility )


/// Attach a directive label to a rule area, establishing the bidirectional cache link that
/// addDirective() builds at runtime.  Linking both directions keeps ~SCH_RULE_AREA safe regardless
/// of the order in which the fixtures are torn down.
static void attachDirective( SCH_RULE_AREA& aRuleArea, SCH_DIRECTIVE_LABEL& aDirective )
{
    aDirective.AddConnectedRuleArea( &aRuleArea );
    aRuleArea.m_directives.insert( &aDirective );
}


BOOST_AUTO_TEST_CASE( AreaWithoutDirectiveIsNotDirectiveLabelOnly )
{
    SCH_RULE_AREA ruleArea;

    BOOST_CHECK( !ruleArea.IsDirectiveLabelOnlyArea() );
}


BOOST_AUTO_TEST_CASE( AreaWithDirectiveAndNoFlagsIsDirectiveLabelOnly )
{
    SCH_DIRECTIVE_LABEL directive;
    SCH_RULE_AREA       ruleArea;

    attachDirective( ruleArea, directive );

    BOOST_CHECK( ruleArea.IsDirectiveLabelOnlyArea() );
}


BOOST_AUTO_TEST_CASE( AnyDesignFlagDisqualifiesDirectiveLabelOnly )
{
    using FlagSetter = void ( SCH_RULE_AREA::* )( bool, const SCH_SHEET_PATH*, const wxString& );

    const FlagSetter setters[] = { &SCH_RULE_AREA::SetDNP, &SCH_RULE_AREA::SetExcludedFromSim,
                                   &SCH_RULE_AREA::SetExcludedFromBOM,
                                   &SCH_RULE_AREA::SetExcludedFromBoard };

    for( const FlagSetter setter : setters )
    {
        SCH_DIRECTIVE_LABEL directive;
        SCH_RULE_AREA       ruleArea;

        attachDirective( ruleArea, directive );
        ( ruleArea.*setter )( true, nullptr, wxEmptyString );

        BOOST_CHECK( !ruleArea.IsDirectiveLabelOnlyArea() );
    }
}


BOOST_AUTO_TEST_CASE( ClearingFlagsRestoresDirectiveLabelOnly )
{
    SCH_DIRECTIVE_LABEL directive;
    SCH_RULE_AREA       ruleArea;

    attachDirective( ruleArea, directive );

    ruleArea.SetDNP( true );
    ruleArea.SetExcludedFromBoard( true );
    BOOST_CHECK( !ruleArea.IsDirectiveLabelOnlyArea() );

    ruleArea.SetDNP( false );
    ruleArea.SetExcludedFromBoard( false );
    BOOST_CHECK( ruleArea.IsDirectiveLabelOnlyArea() );
}


BOOST_AUTO_TEST_SUITE_END()
