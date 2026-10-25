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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * R1 pin 1 (38.1, 50.8) and R2 pin 1 (50.8, 38.1) are unconnected but equidistant from the
 * origin.  Ordered containers keyed by VECTOR2I must keep them apart when the standard
 * library resolves std::less to operator< directly, as libc++ does.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <eeschema_test_utils.h>

#include <eeschema_helpers.h>
#include <locale_io.h>
#include <sch_pin.h>
#include <sch_screen.h>
#include <sch_symbol.h>
#include <schematic.h>

#include <set>


BOOST_AUTO_TEST_CASE( Issue25583EquidistantPinsStayDistinct )
{
    LOCALE_IO dummy;

    wxString schPath = wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() )
                       + wxS( "issue25583/issue25583.kicad_sch" );

    std::unique_ptr<SCHEMATIC> sch( EESCHEMA_HELPERS::LoadSchematic( schPath, true, true ) );
    BOOST_REQUIRE( sch != nullptr );

    std::set<VECTOR2I, std::less<>> positions;

    for( SCH_ITEM* item : sch->RootScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        for( SCH_PIN* pin : static_cast<SCH_SYMBOL*>( item )->GetPinsByNumber( wxS( "1" ) ) )
            positions.insert( pin->GetPosition() );
    }

    BOOST_CHECK_EQUAL( positions.size(), 2 );
}
