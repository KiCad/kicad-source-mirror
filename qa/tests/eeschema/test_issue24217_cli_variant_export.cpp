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
 */

// Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24217

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <connection_graph.h>
#include <eeschema_helpers.h>
#include <locale_io.h>
#include <sch_reference_list.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <schematic.h>

#include <schematic_utils/schematic_file_util.h>


BOOST_AUTO_TEST_CASE( Issue24217_VariantSurvivesStaleInstancePath )
{
    LOCALE_IO dummy;

    wxString schPath = wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() ) +
                       wxS( "issue24217/issue24217.kicad_sch" );

    // Load via the CLI helper so the regression covers `kicad-cli sch export bom`.
    SCHEMATIC* sch = EESCHEMA_HELPERS::LoadSchematic( schPath, true, true );
    BOOST_REQUIRE( sch != nullptr );

    // The schematic's stored root UUID differs from the UUID in every (path ...) entry,
    // simulating a v9 file re-rooted by v10 on save.  Variant H0 marks R2/R3 DNP and
    // overrides R1's value to "10k".
    sch->SetCurrentVariant( wxS( "H0" ) );
    BOOST_CHECK_EQUAL( sch->GetCurrentVariant(), wxS( "H0" ) );

    SCH_REFERENCE_LIST refs;
    sch->Hierarchy().GetSymbols( refs, SYMBOL_FILTER_NON_POWER, false );

    BOOST_REQUIRE_EQUAL( refs.GetCount(), 3u );

    auto findRef = [&]( const wxString& aRef ) -> SCH_REFERENCE*
    {
        for( size_t i = 0; i < refs.GetCount(); ++i )
        {
            if( refs[i].GetRef() == aRef )
                return &refs[i];
        }

        return nullptr;
    };

    SCH_REFERENCE* r1 = findRef( wxS( "R1" ) );
    SCH_REFERENCE* r2 = findRef( wxS( "R2" ) );
    SCH_REFERENCE* r3 = findRef( wxS( "R3" ) );

    BOOST_REQUIRE( r1 );
    BOOST_REQUIRE( r2 );
    BOOST_REQUIRE( r3 );

    BOOST_CHECK_MESSAGE( r2->GetSymbol()->GetDNP( &r2->GetSheetPath(), wxS( "H0" ) ),
                         "R2 should be DNP in variant H0" );
    BOOST_CHECK_MESSAGE( r3->GetSymbol()->GetDNP( &r3->GetSheetPath(), wxS( "H0" ) ),
                         "R3 should be DNP in variant H0" );
    BOOST_CHECK_MESSAGE( !r1->GetSymbol()->GetDNP( &r1->GetSheetPath(), wxS( "H0" ) ),
                         "R1 should not be DNP in variant H0" );

    SCH_FIELD* r1Value = r1->GetSymbol()->GetField( FIELD_T::VALUE );
    BOOST_REQUIRE( r1Value );

    wxString resolved = r1Value->GetShownText( &r1->GetSheetPath(), false, 0, wxS( "H0" ) );
    BOOST_CHECK_EQUAL( resolved, wxS( "10k" ) );
}
