/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <schematic.h>
#include <erc/erc_settings.h>
#include <erc/erc.h>
#include <settings/settings_manager.h>
#include <locale_io.h>


struct ERC_TEXT_VAR_FIXTURE
{
    ERC_TEXT_VAR_FIXTURE() {}

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


// Verifies that ${ERC_ERROR ...} and ${ERC_WARNING ...} tokens trigger an
// ERC violation regardless of position in the text.  Prior to the fix the
// regex was anchored with `^...$` so only text starting with the token would
// fire, breaking placeholder patterns like "Fill in: ${ERC_ERROR ...}".
BOOST_FIXTURE_TEST_CASE( ERCTextVarIssue24442, ERC_TEXT_VAR_FIXTURE )
{
    LOCALE_IO dummy;

    KI_TEST::LoadSchematic( m_settingsManager, "issue24442", m_schematic );

    ERC_SETTINGS& settings = m_schematic->ErcSettings();

    // Silence noise from other ERC checks; we care only about generic ERC
    // text-variable violations here.
    for( int ii = 0; ii < ERCE_LAST; ++ii )
        settings.m_ERCSeverities[ii] = RPT_SEVERITY_IGNORE;

    settings.m_ERCSeverities[ERCE_GENERIC_ERROR] = RPT_SEVERITY_ERROR;
    settings.m_ERCSeverities[ERCE_GENERIC_WARNING] = RPT_SEVERITY_WARNING;
    settings.m_ERCSeverities[ERCE_UNRESOLVED_VARIABLE] = RPT_SEVERITY_ERROR;

    ERC_TESTER tester( m_schematic.get() );
    tester.TestTextVars( nullptr );

    SHEETLIST_ERC_ITEMS_PROVIDER errors( m_schematic.get() );
    errors.SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );

    int genericErrors = 0;
    int genericWarnings = 0;
    int unresolvedVars = 0;
    std::vector<wxString> errorMessages;
    std::vector<wxString> warningMessages;

    for( int ii = 0; ii < errors.GetCount(); ++ii )
    {
        std::shared_ptr<RC_ITEM> rc = errors.GetItem( ii );

        if( rc->GetErrorCode() == ERCE_GENERIC_ERROR )
        {
            genericErrors++;
            errorMessages.push_back( rc->GetErrorMessage( false ) );
        }
        else if( rc->GetErrorCode() == ERCE_GENERIC_WARNING )
        {
            genericWarnings++;
            warningMessages.push_back( rc->GetErrorMessage( false ) );
        }
        else if( rc->GetErrorCode() == ERCE_UNRESOLVED_VARIABLE )
        {
            unresolvedVars++;
        }
    }

    // The fixture schematic contains two ${ERC_ERROR ...} texts (one at start,
    // one mid-string), two ${ERC_WARNING ...} texts (start + middle), and one
    // escaped \${ERC_ERROR ...} that must NOT count.
    BOOST_CHECK_EQUAL( genericErrors, 2 );
    BOOST_CHECK_EQUAL( genericWarnings, 2 );

    // The four matched markers must not double-report as unresolved variables;
    // only the escaped literal, whose shown text keeps a bare ${...}, remains.
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
    BOOST_CHECK( containsMsg( warningMessages, "this_is_warning" ) );
    BOOST_CHECK( containsMsg( warningMessages, "embedded_warning" ) );
    BOOST_CHECK( !containsMsg( errorMessages, "not_a_real_error" ) );
}
