/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <board.h>
#include <board_design_settings.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <settings/settings_manager.h>


struct DRC_TEXT_VAR_TEST_FIXTURE
{
    DRC_TEXT_VAR_TEST_FIXTURE() {}

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


// Verifies that ${DRC_ERROR ...} and ${DRC_WARNING ...} tokens trigger a DRC
// violation regardless of position in the text.  Prior to the fix the regex
// was anchored with `^...$` so only text starting with the token would fire,
// breaking placeholder patterns like "Fill in: ${DRC_ERROR ...}".
BOOST_FIXTURE_TEST_CASE( DRCTextVarIssue24442, DRC_TEXT_VAR_TEST_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue24442", m_board );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    // Disable DRC tests not useful or not handled in this testcase
    for( int ii = DRCE_FIRST; ii <= DRCE_LAST; ++ii )
        bds.m_DRCSeverities[ii] = SEVERITY::RPT_SEVERITY_IGNORE;

    bds.m_DRCSeverities[DRCE_GENERIC_ERROR] = SEVERITY::RPT_SEVERITY_ERROR;
    bds.m_DRCSeverities[DRCE_GENERIC_WARNING] = SEVERITY::RPT_SEVERITY_WARNING;

    // testTextVars() (the phase that handles ${DRC_ERROR}/${DRC_WARNING}) only
    // runs when DRCE_UNRESOLVED_VARIABLE is enabled; keep it active so the
    // phase is reached.
    bds.m_DRCSeverities[DRCE_UNRESOLVED_VARIABLE] = SEVERITY::RPT_SEVERITY_ERROR;

    int genericErrors = 0;
    int genericWarnings = 0;
    int unresolvedVars = 0;
    std::vector<wxString> errorMessages;
    std::vector<wxString> warningMessages;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                 const std::function<void( PCB_MARKER* )>& aPathGenerator )
            {
                if( aItem->GetErrorCode() == DRCE_GENERIC_ERROR )
                {
                    genericErrors++;
                    errorMessages.push_back( aItem->GetErrorMessage( false ) );
                }
                else if( aItem->GetErrorCode() == DRCE_GENERIC_WARNING )
                {
                    genericWarnings++;
                    warningMessages.push_back( aItem->GetErrorMessage( false ) );
                }
                else if( aItem->GetErrorCode() == DRCE_UNRESOLVED_VARIABLE )
                {
                    unresolvedVars++;
                }
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    // The fixture board contains three texts/textboxes referencing ${DRC_ERROR ...}:
    //   - bare "${DRC_ERROR start_of_text}"
    //   - "Fill in: ${DRC_ERROR placeholder_text}"
    //   - "Note: ${DRC_ERROR text_box_error}" (textbox)
    // And two texts referencing ${DRC_WARNING ...}:
    //   - "${DRC_WARNING this_is_warning} trailing"
    //   - "prefix ${DRC_WARNING embedded_warning} suffix"
    // Plus an escaped "literal: \${DRC_ERROR not_a_real_error}" which must NOT
    // trigger DRCE_GENERIC_ERROR -- the escape is reserved for literal display.
    BOOST_CHECK_EQUAL( genericErrors, 3 );
    BOOST_CHECK_EQUAL( genericWarnings, 2 );

    // A matched marker is the intended diagnostic, so those five texts must not
    // also be flagged as unresolved variables.  Only the escaped literal, whose
    // displayed text still carries a bare ${...}, remains an unresolved variable.
    BOOST_CHECK_EQUAL( unresolvedVars, 1 );

    auto containsMsg =
            []( const std::vector<wxString>& aList, const wxString& aNeedle )
            {
                for( const wxString& msg : aList )
                {
                    if( msg.Contains( aNeedle ) )
                        return true;
                }

                return false;
            };

    BOOST_CHECK( containsMsg( errorMessages, "start_of_text" ) );
    BOOST_CHECK( containsMsg( errorMessages, "placeholder_text" ) );
    BOOST_CHECK( containsMsg( errorMessages, "text_box_error" ) );
    BOOST_CHECK( containsMsg( warningMessages, "this_is_warning" ) );
    BOOST_CHECK( containsMsg( warningMessages, "embedded_warning" ) );

    // The escaped literal must never surface as a generic error.
    BOOST_CHECK( !containsMsg( errorMessages, "not_a_real_error" ) );
}
