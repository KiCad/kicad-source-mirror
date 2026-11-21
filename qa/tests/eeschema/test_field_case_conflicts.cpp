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

#include <memory>

#include <eeschema_helpers.h>
#include <eeschema_test_utils.h>
#include <sch_field.h>
#include <sch_reference_list.h>
#include <sch_symbol.h>
#include <schematic.h>


BOOST_AUTO_TEST_SUITE( MandatoryFieldCaseOnLoad )


BOOST_AUTO_TEST_CASE( CaseVariantDoesNotOverwriteMandatoryField )
{
    wxString schPath =
            wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() ) + wxS( "issue23952/issue23952.kicad_sch" );

    std::unique_ptr<SCHEMATIC> schematic( EESCHEMA_HELPERS::LoadSchematic( schPath, true, false ) );
    BOOST_REQUIRE( schematic );

    SCH_REFERENCE_LIST refs;
    schematic->Hierarchy().GetSymbols( refs, SYMBOL_FILTER_NON_POWER );
    BOOST_REQUIRE_EQUAL( refs.GetCount(), 1u );

    SCH_SYMBOL* symbol = refs[0].GetSymbol();
    BOOST_REQUIRE( symbol );

    BOOST_CHECK_EQUAL( symbol->GetField( FIELD_T::VALUE )->GetText(), wxS( "10k" ) );

    SCH_FIELD* userField = symbol->GetField( wxS( "value_1" ) );
    BOOST_REQUIRE_MESSAGE( userField, "the lower case value field was lost on load" );

    BOOST_CHECK_EQUAL( userField->GetText(), wxS( "" ) );
}


BOOST_AUTO_TEST_SUITE_END()
