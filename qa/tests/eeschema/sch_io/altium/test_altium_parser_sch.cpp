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
#include <eeschema/sch_text.h>
#include <layer_ids.h>
#include <validators.h>
#include <wx/filename.h>

// Function declarations of private methods to test
int ReadKiCadUnitFrac( const std::map<wxString, wxString>& aProps,
                       const wxString&                     aKey );

void SetTextPositioning( EDA_TEXT* text, ASCH_LABEL_JUSTIFICATION justification,
                         ASCH_RECORD_ORIENTATION orientation );

void AdjustTextForSymbolOrientation( SCH_TEXT* aText, const ASCH_SYMBOL& aSymbol );


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

    BOOST_CHECK_MESSAGE( (!validationError.empty()) ,
                         wxString::Format( "Sheet name with '/' should fail validation" ) );
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


static ASCH_SYMBOL MakeTestSymbol( int aOrientation, bool aMirrored )
{
    std::map<wxString, wxString> props;
    props[wxT( "RECORD" )] = wxT( "1" );
    props[wxT( "ORIENTATION" )] = wxString::Format( wxT( "%d" ), aOrientation );
    props[wxT( "ISMIRRORED" )] = aMirrored ? wxT( "T" ) : wxT( "F" );
    props[wxT( "CURRENTPARTID" )] = wxT( "1" );
    props[wxT( "PARTCOUNT" )] = wxT( "2" );
    props[wxT( "DISPLAYMODECOUNT" )] = wxT( "1" );
    props[wxT( "LOCATION.X" )] = wxT( "0" );
    props[wxT( "LOCATION.Y" )] = wxT( "0" );
    return ASCH_SYMBOL( props );
}


// Simulates the angle/justification portion of what OrientAndMirrorSymbolItems
// does at render time. Position is not relevant for this test.
static void SimulateRenderTransform( SCH_TEXT* aText, int aAltiumOrientation, bool aMirrored )
{
    int nRotations = ( aAltiumOrientation + 1 ) % 4;

    for( int i = 0; i < nRotations; i++ )
        aText->Rotate90( false );

    if( aMirrored )
    {
        if( aText->GetTextAngle().IsHorizontal() )
            aText->FlipHJustify();
        else
            aText->SetVertJustify( static_cast<GR_TEXT_V_ALIGN_T>( -aText->GetVertJustify() ) );
    }
}


struct TEXT_ORIENT_CASE
{
    ASCH_RECORD_ORIENTATION    textOrientation;
    ASCH_LABEL_JUSTIFICATION   justification;
    int                        altiumSymOrientation;
    bool                       mirrored;
    EDA_ANGLE                  expectedAngle;
    GR_TEXT_H_ALIGN_T          expectedHJustify;
};


static const std::vector<TEXT_ORIENT_CASE> text_orient_cases = {
    // Horizontal text, left-justified, each symbol orientation
    { ASCH_RECORD_ORIENTATION::RIGHTWARDS, ASCH_LABEL_JUSTIFICATION::BOTTOM_LEFT,
      0, false, ANGLE_HORIZONTAL, GR_TEXT_H_ALIGN_LEFT },
    { ASCH_RECORD_ORIENTATION::RIGHTWARDS, ASCH_LABEL_JUSTIFICATION::BOTTOM_LEFT,
      1, false, ANGLE_HORIZONTAL, GR_TEXT_H_ALIGN_LEFT },
    { ASCH_RECORD_ORIENTATION::RIGHTWARDS, ASCH_LABEL_JUSTIFICATION::BOTTOM_LEFT,
      2, false, ANGLE_HORIZONTAL, GR_TEXT_H_ALIGN_LEFT },
    { ASCH_RECORD_ORIENTATION::RIGHTWARDS, ASCH_LABEL_JUSTIFICATION::BOTTOM_LEFT,
      3, false, ANGLE_HORIZONTAL, GR_TEXT_H_ALIGN_LEFT },

    // Vertical text, left-justified, each symbol orientation
    { ASCH_RECORD_ORIENTATION::UPWARDS, ASCH_LABEL_JUSTIFICATION::BOTTOM_LEFT,
      0, false, ANGLE_VERTICAL, GR_TEXT_H_ALIGN_LEFT },
    { ASCH_RECORD_ORIENTATION::UPWARDS, ASCH_LABEL_JUSTIFICATION::BOTTOM_LEFT,
      1, false, ANGLE_VERTICAL, GR_TEXT_H_ALIGN_LEFT },
    { ASCH_RECORD_ORIENTATION::UPWARDS, ASCH_LABEL_JUSTIFICATION::BOTTOM_LEFT,
      2, false, ANGLE_VERTICAL, GR_TEXT_H_ALIGN_LEFT },
    { ASCH_RECORD_ORIENTATION::UPWARDS, ASCH_LABEL_JUSTIFICATION::BOTTOM_LEFT,
      3, false, ANGLE_VERTICAL, GR_TEXT_H_ALIGN_LEFT },

    // Horizontal text, right-justified, each symbol orientation
    { ASCH_RECORD_ORIENTATION::RIGHTWARDS, ASCH_LABEL_JUSTIFICATION::BOTTOM_RIGHT,
      0, false, ANGLE_HORIZONTAL, GR_TEXT_H_ALIGN_RIGHT },
    { ASCH_RECORD_ORIENTATION::RIGHTWARDS, ASCH_LABEL_JUSTIFICATION::BOTTOM_RIGHT,
      1, false, ANGLE_HORIZONTAL, GR_TEXT_H_ALIGN_RIGHT },
    { ASCH_RECORD_ORIENTATION::RIGHTWARDS, ASCH_LABEL_JUSTIFICATION::BOTTOM_RIGHT,
      2, false, ANGLE_HORIZONTAL, GR_TEXT_H_ALIGN_RIGHT },
    { ASCH_RECORD_ORIENTATION::RIGHTWARDS, ASCH_LABEL_JUSTIFICATION::BOTTOM_RIGHT,
      3, false, ANGLE_HORIZONTAL, GR_TEXT_H_ALIGN_RIGHT },

    // Mirrored cases
    { ASCH_RECORD_ORIENTATION::RIGHTWARDS, ASCH_LABEL_JUSTIFICATION::BOTTOM_LEFT,
      0, true, ANGLE_HORIZONTAL, GR_TEXT_H_ALIGN_LEFT },
    { ASCH_RECORD_ORIENTATION::RIGHTWARDS, ASCH_LABEL_JUSTIFICATION::BOTTOM_LEFT,
      1, true, ANGLE_HORIZONTAL, GR_TEXT_H_ALIGN_LEFT },
    { ASCH_RECORD_ORIENTATION::UPWARDS, ASCH_LABEL_JUSTIFICATION::BOTTOM_RIGHT,
      0, true, ANGLE_VERTICAL, GR_TEXT_H_ALIGN_RIGHT },
    { ASCH_RECORD_ORIENTATION::UPWARDS, ASCH_LABEL_JUSTIFICATION::BOTTOM_RIGHT,
      2, true, ANGLE_VERTICAL, GR_TEXT_H_ALIGN_RIGHT },

    // LEFTWARDS text flips H-justify via SetTextPositioning
    { ASCH_RECORD_ORIENTATION::LEFTWARDS, ASCH_LABEL_JUSTIFICATION::BOTTOM_LEFT,
      0, false, ANGLE_HORIZONTAL, GR_TEXT_H_ALIGN_RIGHT },
    { ASCH_RECORD_ORIENTATION::LEFTWARDS, ASCH_LABEL_JUSTIFICATION::BOTTOM_LEFT,
      3, false, ANGLE_HORIZONTAL, GR_TEXT_H_ALIGN_RIGHT },

    // DOWNWARDS text flips H-justify via SetTextPositioning
    { ASCH_RECORD_ORIENTATION::DOWNWARDS, ASCH_LABEL_JUSTIFICATION::BOTTOM_LEFT,
      0, false, ANGLE_VERTICAL, GR_TEXT_H_ALIGN_RIGHT },
    { ASCH_RECORD_ORIENTATION::DOWNWARDS, ASCH_LABEL_JUSTIFICATION::BOTTOM_LEFT,
      3, false, ANGLE_VERTICAL, GR_TEXT_H_ALIGN_RIGHT },
};


/**
 * Verify that AdjustTextForSymbolOrientation correctly pre-compensates text properties
 * so that after the render-time symbol transform, the final text orientation matches
 * the absolute Altium orientation.
 */
BOOST_AUTO_TEST_CASE( TextOrientationCompensation )
{
    for( size_t idx = 0; idx < text_orient_cases.size(); idx++ )
    {
        const TEXT_ORIENT_CASE& c = text_orient_cases[idx];

        BOOST_TEST_CONTEXT( wxString::Format(
                wxT( "case %zu: textOri=%d justification=%d symOri=%d mirror=%d" ),
                idx, (int) c.textOrientation, (int) c.justification,
                c.altiumSymOrientation, c.mirrored ? 1 : 0 ) )
        {
            SCH_TEXT textItem( { 0, 0 }, wxT( "TEST" ), LAYER_DEVICE );

            SetTextPositioning( &textItem, c.justification, c.textOrientation );

            ASCH_SYMBOL sym = MakeTestSymbol( c.altiumSymOrientation, c.mirrored );

            AdjustTextForSymbolOrientation( &textItem, sym );

            SimulateRenderTransform( &textItem, c.altiumSymOrientation, c.mirrored );

            BOOST_CHECK_EQUAL( textItem.GetTextAngle(), c.expectedAngle );
            BOOST_CHECK_EQUAL( textItem.GetHorizJustify(), c.expectedHJustify );
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
