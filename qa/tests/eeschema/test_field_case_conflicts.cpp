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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <fields_data_model.h>
#include <sch_field.h>
#include <sch_reference_list.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>


struct FIELD_CASE_CONFLICT_FIXTURE
{
    SCH_SYMBOL makeSymbol( const wxString& aRef,
                           std::initializer_list<std::pair<wxString, wxString>> aUserFields )
    {
        SCH_SYMBOL sym;

        SCH_FIELD* refField = sym.GetField( FIELD_T::REFERENCE );
        wxASSERT( refField );
        refField->SetText( aRef );

        for( const auto& [name, value] : aUserFields )
        {
            SCH_FIELD f( &sym, FIELD_T::USER, name );
            f.SetText( value );
            sym.AddField( f );
        }

        return sym;
    }

    SCH_REFERENCE_LIST buildList( std::vector<SCH_SYMBOL>& aSymbols )
    {
        SCH_REFERENCE_LIST list;
        SCH_SHEET_PATH     path;

        for( SCH_SYMBOL& sym : aSymbols )
        {
            SCH_REFERENCE ref( &sym, path );
            ref.SetRef( sym.GetField( FIELD_T::REFERENCE )->GetText() );
            list.AddItem( ref );
        }

        return list;
    }
};


BOOST_FIXTURE_TEST_SUITE( FieldCaseConflicts, FIELD_CASE_CONFLICT_FIXTURE )


BOOST_AUTO_TEST_CASE( EmptyListYieldsNoConflicts )
{
    SCH_REFERENCE_LIST list;
    BOOST_CHECK( DetectFieldCaseConflicts( list ).empty() );
}


BOOST_AUTO_TEST_CASE( SingleSymbolWithoutUserFieldsYieldsNoConflicts )
{
    std::vector<SCH_SYMBOL> symbols;
    symbols.push_back( makeSymbol( "R1", {} ) );

    SCH_REFERENCE_LIST list = buildList( symbols );

    BOOST_CHECK( DetectFieldCaseConflicts( list ).empty() );
}


BOOST_AUTO_TEST_CASE( DifferentSymbolsWithCaseVariantsYieldNoConflicts )
{
    std::vector<SCH_SYMBOL> symbols;
    symbols.push_back( makeSymbol( "R1", { { "Manufacturer", "Vishay" } } ) );
    symbols.push_back( makeSymbol( "R2", { { "MANUFACTURER", "Neutrik" } } ) );

    SCH_REFERENCE_LIST list = buildList( symbols );

    BOOST_CHECK( DetectFieldCaseConflicts( list ).empty() );
}


BOOST_AUTO_TEST_CASE( SameSymbolWithBothCaseVariantsYieldsOneConflict )
{
    std::vector<SCH_SYMBOL> symbols;
    symbols.push_back( makeSymbol( "R1", { { "Manufacturer", "Vishay" },
                                           { "MANUFACTURER", "Yageo" } } ) );

    SCH_REFERENCE_LIST list = buildList( symbols );

    auto conflicts = DetectFieldCaseConflicts( list );

    BOOST_REQUIRE_EQUAL( conflicts.size(), 1u );
    BOOST_CHECK_EQUAL( conflicts[0].reference, "R1" );
    BOOST_CHECK_EQUAL( conflicts[0].caseFoldedKey, "manufacturer" );
    BOOST_REQUIRE_EQUAL( conflicts[0].variants.size(), 2u );
}


BOOST_AUTO_TEST_CASE( ThreeCaseVariantsOnSameSymbolYieldOneConflictWithThreeMembers )
{
    std::vector<SCH_SYMBOL> symbols;
    symbols.push_back( makeSymbol( "R1", { { "Manufacturer", "A" },
                                           { "MANUFACTURER", "B" },
                                           { "manufacturer", "C" } } ) );

    SCH_REFERENCE_LIST list = buildList( symbols );

    auto conflicts = DetectFieldCaseConflicts( list );

    BOOST_REQUIRE_EQUAL( conflicts.size(), 1u );
    BOOST_CHECK_EQUAL( conflicts[0].variants.size(), 3u );
}


BOOST_AUTO_TEST_CASE( MultipleSymbolsWithConflictsAreAllReported )
{
    std::vector<SCH_SYMBOL> symbols;
    symbols.push_back( makeSymbol( "R1", { { "Manufacturer", "A" },
                                           { "MANUFACTURER", "B" } } ) );
    symbols.push_back( makeSymbol( "R2", { { "Tolerance", "1%" },
                                           { "tolerance", "5%" } } ) );
    symbols.push_back( makeSymbol( "R3", { { "Manufacturer", "C" } } ) );

    SCH_REFERENCE_LIST list = buildList( symbols );

    auto conflicts = DetectFieldCaseConflicts( list );

    BOOST_CHECK_EQUAL( conflicts.size(), 2u );
}


BOOST_AUTO_TEST_CASE( DistinctFieldNamesOnSameSymbolYieldNoConflict )
{
    std::vector<SCH_SYMBOL> symbols;
    symbols.push_back( makeSymbol( "R1", { { "Manufacturer", "A" },
                                           { "PartNumber",   "B" } } ) );

    SCH_REFERENCE_LIST list = buildList( symbols );

    BOOST_CHECK( DetectFieldCaseConflicts( list ).empty() );
}


BOOST_AUTO_TEST_SUITE_END()
