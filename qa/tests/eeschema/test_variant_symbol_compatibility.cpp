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

#include <lib_symbol.h>
#include <sch_pin.h>
#include <sch_symbol.h>
#include <variant_symbol_utils.h>


namespace
{

/**
 * Add a pin to a LIB_SYMBOL with the given number, position, electrical type,
 * unit, and body style.
 */
void AddPin( LIB_SYMBOL& aSymbol, const wxString& aNumber, const VECTOR2I& aPos,
             ELECTRICAL_PINTYPE aType = ELECTRICAL_PINTYPE::PT_PASSIVE,
             int aUnit = 1, int aBodyStyle = 1 )
{
    SCH_PIN* pin = new SCH_PIN( &aSymbol );
    pin->SetNumber( aNumber );
    pin->SetPosition( aPos );
    pin->SetType( aType );
    pin->SetUnit( aUnit );
    pin->SetBodyStyle( aBodyStyle );
    aSymbol.AddDrawItem( pin );
}


/**
 * Build a simple two-pin passive symbol (like a resistor).
 */
std::unique_ptr<LIB_SYMBOL> MakeTwoPinPassive( const wxString& aName )
{
    auto sym = std::make_unique<LIB_SYMBOL>( aName );
    AddPin( *sym, wxS( "1" ), VECTOR2I( 0, 0 ) );
    AddPin( *sym, wxS( "2" ), VECTOR2I( 0, 5080000 ) );
    return sym;
}


bool HasError( const std::vector<VARIANT_COMPAT_RESULT>& aResults, VARIANT_COMPAT_ERROR aError )
{
    for( const VARIANT_COMPAT_RESULT& r : aResults )
    {
        if( r.error == aError )
            return true;
    }

    return false;
}


bool HasErrorForPin( const std::vector<VARIANT_COMPAT_RESULT>& aResults,
                     VARIANT_COMPAT_ERROR aError, const wxString& aPin )
{
    for( const VARIANT_COMPAT_RESULT& r : aResults )
    {
        if( r.error == aError && r.pinNumber == aPin )
            return true;
    }

    return false;
}

} // anonymous namespace


BOOST_AUTO_TEST_SUITE( VariantSymbolCompatibility )


BOOST_AUTO_TEST_CASE( IdenticalSymbols_Compatible )
{
    auto base = MakeTwoPinPassive( wxS( "R_100R" ) );
    auto candidate = MakeTwoPinPassive( wxS( "R_100R" ) );

    auto results = ValidateVariantSymbolCompatibility( *base, *candidate );

    BOOST_CHECK( results.empty() );
}


BOOST_AUTO_TEST_CASE( SamePinLayout_Compatible )
{
    auto base = MakeTwoPinPassive( wxS( "R_100R" ) );
    auto candidate = MakeTwoPinPassive( wxS( "R_1K" ) );

    auto results = ValidateVariantSymbolCompatibility( *base, *candidate );

    BOOST_CHECK( results.empty() );
}


BOOST_AUTO_TEST_CASE( SamePinLayout_DifferentFootprint_Compatible )
{
    auto base = MakeTwoPinPassive( wxS( "R_100R" ) );
    base->GetFootprintField().SetText( wxS( "R_0402" ) );

    auto candidate = MakeTwoPinPassive( wxS( "R_10K" ) );
    candidate->GetFootprintField().SetText( wxS( "R_0603" ) );

    auto results = ValidateVariantSymbolCompatibility( *base, *candidate );

    BOOST_CHECK( results.empty() );
}


BOOST_AUTO_TEST_CASE( CandidateHasExtraPins_Incompatible )
{
    auto base = MakeTwoPinPassive( wxS( "R_100R" ) );

    auto candidate = std::make_unique<LIB_SYMBOL>( wxS( "R_3PIN" ) );
    AddPin( *candidate, wxS( "1" ), VECTOR2I( 0, 0 ) );
    AddPin( *candidate, wxS( "2" ), VECTOR2I( 0, 5080000 ) );
    AddPin( *candidate, wxS( "3" ), VECTOR2I( 2540000, 2540000 ) );

    auto results = ValidateVariantSymbolCompatibility( *base, *candidate );

    BOOST_CHECK( !results.empty() );
}


BOOST_AUTO_TEST_CASE( DuplicatePinNumbersAtMatchingPositions_Compatible )
{
    LIB_SYMBOL base( wxS( "STACKED_BASE" ) );
    AddPin( base, wxS( "1" ), VECTOR2I( 0, 0 ) );
    AddPin( base, wxS( "1" ), VECTOR2I( 0, 2540000 ) );

    LIB_SYMBOL candidate( wxS( "STACKED_CANDIDATE" ) );
    AddPin( candidate, wxS( "1" ), VECTOR2I( 0, 2540000 ) );
    AddPin( candidate, wxS( "1" ), VECTOR2I( 0, 0 ) );

    BOOST_CHECK( ValidateVariantSymbolCompatibility( base, candidate ).empty() );
}


BOOST_AUTO_TEST_CASE( DuplicatePinNumbersMissingOccurrence_Incompatible )
{
    LIB_SYMBOL base( wxS( "STACKED_BASE" ) );
    AddPin( base, wxS( "1" ), VECTOR2I( 0, 0 ) );
    AddPin( base, wxS( "1" ), VECTOR2I( 0, 2540000 ) );

    LIB_SYMBOL candidate( wxS( "STACKED_CANDIDATE" ) );
    AddPin( candidate, wxS( "1" ), VECTOR2I( 0, 0 ) );

    BOOST_CHECK( !ValidateVariantSymbolCompatibility( base, candidate ).empty() );
}


BOOST_AUTO_TEST_CASE( DuplicatePinNumbersMapByPosition )
{
    LIB_SYMBOL base( wxS( "STACKED_BASE" ) );
    AddPin( base, wxS( "1" ), VECTOR2I( 0, 0 ) );
    AddPin( base, wxS( "1" ), VECTOR2I( 0, 2540000 ) );

    SCH_SHEET_PATH path;
    SCH_SYMBOL     symbol( base, base.GetLibId(), &path, 1 );

    LIB_SYMBOL candidate( wxS( "STACKED_CANDIDATE" ) );
    AddPin( candidate, wxS( "1" ), VECTOR2I( 0, 2540000 ) );
    AddPin( candidate, wxS( "1" ), VECTOR2I( 0, 0 ) );

    const LIB_SYMBOL&             constCandidate = candidate;
    std::vector<const SCH_PIN*>   candidatePins = constCandidate.GetGraphicalPins( 1, 1 );
    std::vector<SCH_PIN*>         mapped = symbol.MapLibPins( candidatePins, true );

    BOOST_REQUIRE_EQUAL( mapped.size(), 2 );
    BOOST_REQUIRE( mapped[0] );
    BOOST_REQUIRE( mapped[1] );
    BOOST_CHECK_NE( mapped[0], mapped[1] );
    BOOST_CHECK_EQUAL( mapped[0]->GetLibPin()->GetPosition(), candidatePins[0]->GetPosition() );
    BOOST_CHECK_EQUAL( mapped[1]->GetLibPin()->GetPosition(), candidatePins[1]->GetPosition() );
}


BOOST_AUTO_TEST_CASE( CandidateMissingPin_Incompatible )
{
    auto base = std::make_unique<LIB_SYMBOL>( wxS( "R_3PIN" ) );
    AddPin( *base, wxS( "1" ), VECTOR2I( 0, 0 ) );
    AddPin( *base, wxS( "2" ), VECTOR2I( 0, 5080000 ) );
    AddPin( *base, wxS( "3" ), VECTOR2I( 2540000, 2540000 ) );

    auto candidate = MakeTwoPinPassive( wxS( "R_100R" ) );

    auto results = ValidateVariantSymbolCompatibility( *base, *candidate );

    BOOST_CHECK( !results.empty() );
    BOOST_CHECK( HasErrorForPin( results, VARIANT_COMPAT_ERROR::MISSING_PIN_NUMBER, wxS( "3" ) ) );
}


BOOST_AUTO_TEST_CASE( PinPositionMismatch_Incompatible )
{
    auto base = MakeTwoPinPassive( wxS( "R_100R" ) );

    auto candidate = std::make_unique<LIB_SYMBOL>( wxS( "R_WRONG_POS" ) );
    AddPin( *candidate, wxS( "1" ), VECTOR2I( 0, 0 ) );
    AddPin( *candidate, wxS( "2" ), VECTOR2I( 0, 7620000 ) );

    auto results = ValidateVariantSymbolCompatibility( *base, *candidate );

    BOOST_CHECK( !results.empty() );
    BOOST_CHECK( HasErrorForPin( results, VARIANT_COMPAT_ERROR::PIN_POSITION_MISMATCH,
                                 wxS( "2" ) ) );
}


BOOST_AUTO_TEST_CASE( PinTypeMismatch_Incompatible )
{
    auto base = MakeTwoPinPassive( wxS( "R_100R" ) );

    auto candidate = std::make_unique<LIB_SYMBOL>( wxS( "R_WRONG_TYPE" ) );
    AddPin( *candidate, wxS( "1" ), VECTOR2I( 0, 0 ), ELECTRICAL_PINTYPE::PT_INPUT );
    AddPin( *candidate, wxS( "2" ), VECTOR2I( 0, 5080000 ) );

    auto results = ValidateVariantSymbolCompatibility( *base, *candidate );

    BOOST_CHECK( !results.empty() );
    BOOST_CHECK( HasErrorForPin( results, VARIANT_COMPAT_ERROR::PIN_TYPE_MISMATCH,
                                 wxS( "1" ) ) );
}


BOOST_AUTO_TEST_CASE( DifferentSymbolType_Incompatible )
{
    auto base = MakeTwoPinPassive( wxS( "R_100R" ) );

    auto candidate = std::make_unique<LIB_SYMBOL>( wxS( "C_100nF" ) );
    AddPin( *candidate, wxS( "1" ), VECTOR2I( 0, 2540000 ) );
    AddPin( *candidate, wxS( "2" ), VECTOR2I( 0, -2540000 ) );

    auto results = ValidateVariantSymbolCompatibility( *base, *candidate );

    BOOST_CHECK( !results.empty() );
    BOOST_CHECK( HasError( results, VARIANT_COMPAT_ERROR::PIN_POSITION_MISMATCH ) );
}


BOOST_AUTO_TEST_CASE( MultiUnit_AllUnitsMatch_Compatible )
{
    auto base = std::make_unique<LIB_SYMBOL>( wxS( "DUAL_OPAMP_BASE" ) );
    base->SetUnitCount( 2, false );
    AddPin( *base, wxS( "1" ), VECTOR2I( 0, 0 ), ELECTRICAL_PINTYPE::PT_INPUT, 1 );
    AddPin( *base, wxS( "2" ), VECTOR2I( 0, 2540000 ), ELECTRICAL_PINTYPE::PT_INPUT, 1 );
    AddPin( *base, wxS( "3" ), VECTOR2I( 5080000, 1270000 ), ELECTRICAL_PINTYPE::PT_OUTPUT, 1 );
    AddPin( *base, wxS( "5" ), VECTOR2I( 0, 0 ), ELECTRICAL_PINTYPE::PT_INPUT, 2 );
    AddPin( *base, wxS( "6" ), VECTOR2I( 0, 2540000 ), ELECTRICAL_PINTYPE::PT_INPUT, 2 );
    AddPin( *base, wxS( "7" ), VECTOR2I( 5080000, 1270000 ), ELECTRICAL_PINTYPE::PT_OUTPUT, 2 );

    auto candidate = std::make_unique<LIB_SYMBOL>( wxS( "DUAL_OPAMP_CAND" ) );
    candidate->SetUnitCount( 2, false );
    AddPin( *candidate, wxS( "1" ), VECTOR2I( 0, 0 ), ELECTRICAL_PINTYPE::PT_INPUT, 1 );
    AddPin( *candidate, wxS( "2" ), VECTOR2I( 0, 2540000 ), ELECTRICAL_PINTYPE::PT_INPUT, 1 );
    AddPin( *candidate, wxS( "3" ), VECTOR2I( 5080000, 1270000 ), ELECTRICAL_PINTYPE::PT_OUTPUT, 1 );
    AddPin( *candidate, wxS( "5" ), VECTOR2I( 0, 0 ), ELECTRICAL_PINTYPE::PT_INPUT, 2 );
    AddPin( *candidate, wxS( "6" ), VECTOR2I( 0, 2540000 ), ELECTRICAL_PINTYPE::PT_INPUT, 2 );
    AddPin( *candidate, wxS( "7" ), VECTOR2I( 5080000, 1270000 ), ELECTRICAL_PINTYPE::PT_OUTPUT, 2 );

    auto results = ValidateVariantSymbolCompatibility( *base, *candidate );

    BOOST_CHECK( results.empty() );
}


BOOST_AUTO_TEST_CASE( MultiUnit_OneUnitMismatch_Incompatible )
{
    auto base = std::make_unique<LIB_SYMBOL>( wxS( "DUAL_OPAMP_BASE" ) );
    base->SetUnitCount( 2, false );
    AddPin( *base, wxS( "1" ), VECTOR2I( 0, 0 ), ELECTRICAL_PINTYPE::PT_INPUT, 1 );
    AddPin( *base, wxS( "2" ), VECTOR2I( 0, 2540000 ), ELECTRICAL_PINTYPE::PT_INPUT, 1 );
    AddPin( *base, wxS( "3" ), VECTOR2I( 5080000, 1270000 ), ELECTRICAL_PINTYPE::PT_OUTPUT, 1 );
    AddPin( *base, wxS( "5" ), VECTOR2I( 0, 0 ), ELECTRICAL_PINTYPE::PT_INPUT, 2 );
    AddPin( *base, wxS( "6" ), VECTOR2I( 0, 2540000 ), ELECTRICAL_PINTYPE::PT_INPUT, 2 );
    AddPin( *base, wxS( "7" ), VECTOR2I( 5080000, 1270000 ), ELECTRICAL_PINTYPE::PT_OUTPUT, 2 );

    auto candidate = std::make_unique<LIB_SYMBOL>( wxS( "DUAL_OPAMP_CAND" ) );
    candidate->SetUnitCount( 2, false );
    AddPin( *candidate, wxS( "1" ), VECTOR2I( 0, 0 ), ELECTRICAL_PINTYPE::PT_INPUT, 1 );
    AddPin( *candidate, wxS( "2" ), VECTOR2I( 0, 2540000 ), ELECTRICAL_PINTYPE::PT_INPUT, 1 );
    AddPin( *candidate, wxS( "3" ), VECTOR2I( 5080000, 1270000 ), ELECTRICAL_PINTYPE::PT_OUTPUT, 1 );
    // Unit 2 is missing pin "7"
    AddPin( *candidate, wxS( "5" ), VECTOR2I( 0, 0 ), ELECTRICAL_PINTYPE::PT_INPUT, 2 );
    AddPin( *candidate, wxS( "6" ), VECTOR2I( 0, 2540000 ), ELECTRICAL_PINTYPE::PT_INPUT, 2 );

    auto results = ValidateVariantSymbolCompatibility( *base, *candidate );

    BOOST_CHECK( !results.empty() );
    BOOST_CHECK( HasErrorForPin( results, VARIANT_COMPAT_ERROR::MISSING_PIN_NUMBER, wxS( "7" ) ) );
}


BOOST_AUTO_TEST_CASE( CandidateFewerUnits_Incompatible )
{
    auto base = std::make_unique<LIB_SYMBOL>( wxS( "QUAD_OPAMP" ) );
    base->SetUnitCount( 4, false );
    AddPin( *base, wxS( "1" ), VECTOR2I( 0, 0 ), ELECTRICAL_PINTYPE::PT_INPUT, 1 );
    AddPin( *base, wxS( "2" ), VECTOR2I( 0, 0 ), ELECTRICAL_PINTYPE::PT_INPUT, 2 );
    AddPin( *base, wxS( "3" ), VECTOR2I( 0, 0 ), ELECTRICAL_PINTYPE::PT_INPUT, 3 );
    AddPin( *base, wxS( "4" ), VECTOR2I( 0, 0 ), ELECTRICAL_PINTYPE::PT_INPUT, 4 );

    auto candidate = std::make_unique<LIB_SYMBOL>( wxS( "DUAL_OPAMP" ) );
    candidate->SetUnitCount( 2, false );
    AddPin( *candidate, wxS( "1" ), VECTOR2I( 0, 0 ), ELECTRICAL_PINTYPE::PT_INPUT, 1 );
    AddPin( *candidate, wxS( "2" ), VECTOR2I( 0, 0 ), ELECTRICAL_PINTYPE::PT_INPUT, 2 );

    auto results = ValidateVariantSymbolCompatibility( *base, *candidate );

    BOOST_CHECK( !results.empty() );
    BOOST_CHECK( HasError( results, VARIANT_COMPAT_ERROR::INSUFFICIENT_UNITS ) );
}


BOOST_AUTO_TEST_CASE( CandidateMoreUnits_Incompatible )
{
    auto base = std::make_unique<LIB_SYMBOL>( wxS( "DUAL_OPAMP" ) );
    base->SetUnitCount( 2, false );
    AddPin( *base, wxS( "1" ), VECTOR2I( 0, 0 ), ELECTRICAL_PINTYPE::PT_INPUT, 1 );
    AddPin( *base, wxS( "2" ), VECTOR2I( 0, 0 ), ELECTRICAL_PINTYPE::PT_INPUT, 2 );

    auto candidate = std::make_unique<LIB_SYMBOL>( wxS( "QUAD_OPAMP" ) );
    candidate->SetUnitCount( 4, false );
    AddPin( *candidate, wxS( "1" ), VECTOR2I( 0, 0 ), ELECTRICAL_PINTYPE::PT_INPUT, 1 );
    AddPin( *candidate, wxS( "2" ), VECTOR2I( 0, 0 ), ELECTRICAL_PINTYPE::PT_INPUT, 2 );
    AddPin( *candidate, wxS( "3" ), VECTOR2I( 0, 0 ), ELECTRICAL_PINTYPE::PT_INPUT, 3 );
    AddPin( *candidate, wxS( "4" ), VECTOR2I( 0, 0 ), ELECTRICAL_PINTYPE::PT_INPUT, 4 );

    auto results = ValidateVariantSymbolCompatibility( *base, *candidate );

    BOOST_CHECK( !results.empty() );
}


BOOST_AUTO_TEST_CASE( BodyStyle_BothHave_Compatible )
{
    auto base = std::make_unique<LIB_SYMBOL>( wxS( "GATE_BASE" ) );
    base->SetHasDeMorganBodyStyles( true );
    AddPin( *base, wxS( "1" ), VECTOR2I( 0, 0 ), ELECTRICAL_PINTYPE::PT_INPUT, 1, 1 );
    AddPin( *base, wxS( "2" ), VECTOR2I( 0, 2540000 ), ELECTRICAL_PINTYPE::PT_OUTPUT, 1, 1 );
    AddPin( *base, wxS( "1" ), VECTOR2I( 0, 0 ), ELECTRICAL_PINTYPE::PT_INPUT, 1, 2 );
    AddPin( *base, wxS( "2" ), VECTOR2I( 0, 2540000 ), ELECTRICAL_PINTYPE::PT_OUTPUT, 1, 2 );

    auto candidate = std::make_unique<LIB_SYMBOL>( wxS( "GATE_CAND" ) );
    candidate->SetHasDeMorganBodyStyles( true );
    AddPin( *candidate, wxS( "1" ), VECTOR2I( 0, 0 ), ELECTRICAL_PINTYPE::PT_INPUT, 1, 1 );
    AddPin( *candidate, wxS( "2" ), VECTOR2I( 0, 2540000 ), ELECTRICAL_PINTYPE::PT_OUTPUT, 1, 1 );
    AddPin( *candidate, wxS( "1" ), VECTOR2I( 0, 0 ), ELECTRICAL_PINTYPE::PT_INPUT, 1, 2 );
    AddPin( *candidate, wxS( "2" ), VECTOR2I( 0, 2540000 ), ELECTRICAL_PINTYPE::PT_OUTPUT, 1, 2 );

    auto results = ValidateVariantSymbolCompatibility( *base, *candidate );

    BOOST_CHECK( results.empty() );
}


BOOST_AUTO_TEST_CASE( BodyStyle_CandidateMissing_Incompatible )
{
    auto base = std::make_unique<LIB_SYMBOL>( wxS( "GATE_BASE" ) );
    base->SetHasDeMorganBodyStyles( true );
    AddPin( *base, wxS( "1" ), VECTOR2I( 0, 0 ), ELECTRICAL_PINTYPE::PT_INPUT, 1, 1 );
    AddPin( *base, wxS( "2" ), VECTOR2I( 0, 2540000 ), ELECTRICAL_PINTYPE::PT_OUTPUT, 1, 1 );
    AddPin( *base, wxS( "1" ), VECTOR2I( 0, 0 ), ELECTRICAL_PINTYPE::PT_INPUT, 1, 2 );
    AddPin( *base, wxS( "2" ), VECTOR2I( 0, 2540000 ), ELECTRICAL_PINTYPE::PT_OUTPUT, 1, 2 );

    auto candidate = std::make_unique<LIB_SYMBOL>( wxS( "GATE_CAND" ) );
    AddPin( *candidate, wxS( "1" ), VECTOR2I( 0, 0 ), ELECTRICAL_PINTYPE::PT_INPUT, 1, 1 );
    AddPin( *candidate, wxS( "2" ), VECTOR2I( 0, 2540000 ), ELECTRICAL_PINTYPE::PT_OUTPUT, 1, 1 );

    auto results = ValidateVariantSymbolCompatibility( *base, *candidate );

    BOOST_CHECK( !results.empty() );
    BOOST_CHECK( HasError( results, VARIANT_COMPAT_ERROR::MISSING_BODY_STYLE ) );
}


BOOST_AUTO_TEST_CASE( EmptySymbols_Compatible )
{
    LIB_SYMBOL base( wxS( "EMPTY_A" ) );
    LIB_SYMBOL candidate( wxS( "EMPTY_B" ) );

    auto results = ValidateVariantSymbolCompatibility( base, candidate );

    BOOST_CHECK( results.empty() );
}


BOOST_AUTO_TEST_CASE( MultipleErrors_AllReported )
{
    auto base = std::make_unique<LIB_SYMBOL>( wxS( "BASE" ) );
    AddPin( *base, wxS( "1" ), VECTOR2I( 0, 0 ), ELECTRICAL_PINTYPE::PT_PASSIVE );
    AddPin( *base, wxS( "2" ), VECTOR2I( 0, 5080000 ), ELECTRICAL_PINTYPE::PT_PASSIVE );
    AddPin( *base, wxS( "3" ), VECTOR2I( 2540000, 0 ), ELECTRICAL_PINTYPE::PT_INPUT );

    auto candidate = std::make_unique<LIB_SYMBOL>( wxS( "CAND" ) );
    // Pin 1 has wrong type
    AddPin( *candidate, wxS( "1" ), VECTOR2I( 0, 0 ), ELECTRICAL_PINTYPE::PT_OUTPUT );
    // Pin 2 has wrong position
    AddPin( *candidate, wxS( "2" ), VECTOR2I( 0, 7620000 ), ELECTRICAL_PINTYPE::PT_PASSIVE );
    // Pin 3 is missing

    auto results = ValidateVariantSymbolCompatibility( *base, *candidate );

    BOOST_CHECK_EQUAL( results.size(), 3 );
    BOOST_CHECK( HasErrorForPin( results, VARIANT_COMPAT_ERROR::PIN_TYPE_MISMATCH, wxS( "1" ) ) );
    BOOST_CHECK( HasErrorForPin( results, VARIANT_COMPAT_ERROR::PIN_POSITION_MISMATCH,
                                 wxS( "2" ) ) );
    BOOST_CHECK( HasErrorForPin( results, VARIANT_COMPAT_ERROR::MISSING_PIN_NUMBER, wxS( "3" ) ) );
}


BOOST_AUTO_TEST_SUITE_END()
