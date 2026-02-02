/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

/**
 * @file test_altium_parser_sch.cpp
 * Test suite for #ALTIUM_PARSER_SCH
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <eeschema/sch_io/altium/altium_parser_sch.h>
#include <validators.h>
#include <wx/filename.h>

// Function declarations of private methods to test
int ReadKiCadUnitFrac( const std::map<wxString, wxString>& aProps,
                       const wxString&                     aKey );


struct ALTIUM_PARSER_SCH_FIXTURE
{
    ALTIUM_PARSER_SCH_FIXTURE() {}
};


/**
 * Declares the struct as the Boost test fixture.
 */
BOOST_FIXTURE_TEST_SUITE( AltiumParserSch, ALTIUM_PARSER_SCH_FIXTURE )

struct ALTIUM_TO_KICAD_UNIT_FRAC_CASE
{
    wxString input;
    wxString input_frac;
    int      exp_result;
};

/**
 * A list of valid internal unit conversation factors
 */
static const std::vector<ALTIUM_TO_KICAD_UNIT_FRAC_CASE> altium_to_kicad_unit_frac = {
    // Some simple values
    { "0", "0", 0 },
    { "1", "0", 2540 },
    { "2", "0", 5080 },
    { "-1", "0", -2540 },
    { "-2", "0", -5080 },
    // Decimal Places
    { "0", "1", 0 },
    { "0", "10", 0 },
    { "0", "100", 0 },
    { "0", "1000", 30 },
    { "0", "10000", 250 },
    { "1", "10000", 2790 },
    { "0", "-1", 0 },
    { "0", "-10", 0 },
    { "0", "-100", 0 },
    { "0", "-1000", -30 },
    { "0", "-10000", -250 },
    { "-1", "-10000", -2790 },
    // Edge Cases
    // Clamp bigger values
    // imperial rounded units as input
    // metric rounded units as input
};

/**
 * Test conversation from Altium internal units into KiCad internal units using properties with FRAC
 */
BOOST_AUTO_TEST_CASE( PropertiesReadKiCadUnitFracConversation )
{
    for( const auto& c : altium_to_kicad_unit_frac )
    {
        BOOST_TEST_CONTEXT(
                wxString::Format( wxT( "%s FRAC %s -> %i" ), c.input, c.input_frac, c.exp_result ) )
        {
            std::map<wxString, wxString> properties = { { "TEST", c.input },
                                                        { "TEST_FRAC", c.input_frac } };

            int result = ReadKiCadUnitFrac( properties, "TEST" );

            // These are all valid
            BOOST_CHECK_EQUAL( result, c.exp_result );
        }
    }
}


struct SHEET_NAME_SANITIZE_CASE
{
    wxString input;
    wxString expected;
};

static const std::vector<SHEET_NAME_SANITIZE_CASE> sheet_name_sanitize_cases = {
    { wxT( "SimpleSheet" ), wxT( "SimpleSheet" ) },
    { wxT( "POWER PROTECTION/MONITORING" ), wxT( "POWER PROTECTION_MONITORING" ) },
    { wxT( "A/B/C" ), wxT( "A_B_C" ) },
    { wxT( "/" ), wxT( "_" ) },
    { wxT( "no_slash_here" ), wxT( "no_slash_here" ) },
    { wxT( "trailing/" ), wxT( "trailing_" ) },
    { wxT( "/leading" ), wxT( "_leading" ) },
};

/**
 * Verify that the slash-to-underscore sanitization used by the Altium importer produces valid
 * KiCad sheet names.
 */
BOOST_AUTO_TEST_CASE( SheetNameSlashSanitization )
{
    for( const auto& c : sheet_name_sanitize_cases )
    {
        BOOST_TEST_CONTEXT( wxString::Format( wxT( "'%s' -> '%s'" ), c.input, c.expected ) )
        {
            wxString sanitized = c.input;
            sanitized.Replace( wxT( "/" ), wxT( "_" ) );

            BOOST_CHECK_EQUAL( sanitized, c.expected );

            wxString validationError = GetFieldValidationErrorMessage( FIELD_T::SHEET_NAME,
                                                                       sanitized );
            BOOST_CHECK_MESSAGE( validationError.empty(),
                                 wxString::Format( wxT( "Sanitized name '%s' failed validation: %s" ),
                                                   sanitized, validationError ) );
        }
    }
}

/**
 * Verify that unsanitized sheet names with slashes are rejected by KiCad validation.
 */
BOOST_AUTO_TEST_CASE( SheetNameSlashRejection )
{
    wxString invalidName = wxT( "POWER PROTECTION/MONITORING" );
    wxString validationError = GetFieldValidationErrorMessage( FIELD_T::SHEET_NAME, invalidName );

    BOOST_CHECK_MESSAGE( !validationError.empty(),
                         wxT( "Sheet name with '/' should fail validation" ) );
}

/**
 * Verify that wxFileName::HasExt() correctly distinguishes extensionless Altium filenames
 * from those with extensions. The Altium importer uses this to decide when to try appending
 * .SchDoc during the hierarchy descent.
 */
BOOST_AUTO_TEST_CASE( ExtensionlessFilenameDetection )
{
    // Filenames WITH extensions should be detected
    BOOST_CHECK( wxFileName( wxT( "6_power.SchDoc" ) ).HasExt() );
    BOOST_CHECK( wxFileName( wxT( "my_sheet.SCHDOC" ) ).HasExt() );

    // Filenames WITHOUT extensions should not be detected
    BOOST_CHECK( !wxFileName( wxT( "6_power" ) ).HasExt() );
    BOOST_CHECK( !wxFileName( wxT( "POWER MANAGEMENT" ) ).HasExt() );
    BOOST_CHECK( !wxFileName( wxT( "" ) ).HasExt() );
}


/**
 * Verify that case-insensitive base-name matching works for resolving extensionless Altium
 * sheet symbol filenames to .SchDoc files on disk.
 */
BOOST_AUTO_TEST_CASE( ExtensionlessBaseNameMatching )
{
    wxFileName sheetFn( wxT( "6_power" ) );
    bool       extensionless = !sheetFn.HasExt();

    BOOST_CHECK( extensionless );

    wxFileName candidateA( wxT( "6_power.SchDoc" ) );
    wxFileName candidateB( wxT( "6_POWER.SCHDOC" ) );
    wxFileName candidateC( wxT( "7_other.SchDoc" ) );
    wxFileName candidateD( wxT( "6_power.txt" ) );

    // Should match same base name with .SchDoc extension (case-insensitive on both parts)
    auto matches = [&]( const wxFileName& candidate )
    {
        return candidate.GetFullName().IsSameAs( sheetFn.GetFullName(), false )
               || ( extensionless
                    && !sheetFn.GetName().empty()
                    && candidate.GetName().IsSameAs( sheetFn.GetName(), false )
                    && candidate.GetExt().IsSameAs( wxT( "SchDoc" ), false ) );
    };

    BOOST_CHECK( matches( candidateA ) );
    BOOST_CHECK( matches( candidateB ) );   // Case-insensitive match on both name and ext
    BOOST_CHECK( !matches( candidateC ) );  // Different base name
    BOOST_CHECK( !matches( candidateD ) );  // Wrong extension
}


/**
 * Verify that name derivation from filenames for top-level sheets produces correct
 * deduplicated names, matching the logic in LoadSchematicProject.
 */
BOOST_AUTO_TEST_CASE( SheetNameDerivationFromFilename )
{
    auto deriveName = []( const wxString& aFilePath, const std::set<wxString>& aExistingNames )
    {
        wxString baseName = wxFileName( aFilePath ).GetName();
        baseName.Replace( wxT( "/" ), wxT( "_" ) );

        wxString sheetName = baseName;

        for( int ii = 1; aExistingNames.count( sheetName ); ++ii )
            sheetName = baseName + wxString::Format( wxT( "_%d" ), ii );

        return sheetName;
    };

    BOOST_CHECK_EQUAL( deriveName( wxT( "6_power.SchDoc" ), {} ), wxT( "6_power" ) );
    BOOST_CHECK_EQUAL( deriveName( wxT( "POWER.SchDoc" ), {} ), wxT( "POWER" ) );

    // Deduplication when a name already exists
    std::set<wxString> existing = { wxT( "6_power" ) };
    BOOST_CHECK_EQUAL( deriveName( wxT( "6_power.SchDoc" ), existing ), wxT( "6_power_1" ) );

    // Multiple duplicates
    existing.insert( wxT( "6_power_1" ) );
    BOOST_CHECK_EQUAL( deriveName( wxT( "6_power.SchDoc" ), existing ), wxT( "6_power_2" ) );
}

BOOST_AUTO_TEST_SUITE_END()
