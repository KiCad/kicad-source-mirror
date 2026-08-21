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

#include <widgets/mdw_part_state.h>


BOOST_AUTO_TEST_SUITE( MdwPartName )


// A long name is elided so the state text after it still shows
BOOST_AUTO_TEST_CASE( LongPartNameIsElidedBeforeTheStateText )
{
    const wxString name = wxS( "R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal" );
    const wxString shown = MDW_PART_STATE::ElideName( name );

    BOOST_CHECK_EQUAL( shown.length(), MDW_PART_STATE::MAX_NAME_LENGTH );
    BOOST_CHECK( shown.StartsWith( wxS( "R_Axial_DIN0207" ) ) );
    BOOST_CHECK( shown.EndsWith( wxS( "62mm_Horizontal" ) ) );
    BOOST_CHECK_EQUAL( MDW_PART_STATE::ElideName( wxS( "LED_D5.0mm" ) ), wxS( "LED_D5.0mm" ) );

    MDW_PART_STATE part;
    part.lifecycle = true;
    part.known = true;
    part.state.category = MDW_LIFECYCLE_CATEGORY::EDITABLE;

    BOOST_CHECK_EQUAL( part.Describe( name ), shown + wxS( " · Editable · never released" ) );
}


// Revise is not open to this user while a teammate holds the claim, so the refusal names the teammate instead
BOOST_AUTO_TEST_CASE( EditRefusalNamesTheClaimHolder )
{
    MDW_PART_STATE part;
    part.lifecycle = true;
    part.known = true;
    part.state.category = MDW_LIFECYCLE_CATEGORY::RELEASED;
    part.lock.is_locked = true;
    part.lock.advisory = true;
    part.lock.owner_display_name = wxS( "Alice" );

    const wxString refusal = part.EditRefusal( wxS( "R_TEST" ) );

    BOOST_CHECK( refusal.Contains( wxS( "Alice" ) ) );
    BOOST_CHECK( !refusal.Contains( wxS( "Revise" ) ) );
}


BOOST_AUTO_TEST_SUITE_END()
