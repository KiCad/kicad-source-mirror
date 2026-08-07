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

#include <mdw/client/mdw_item_locator.h>


BOOST_AUTO_TEST_SUITE( MdwItemLocator )


// An upload backend that cannot locate leaves the design unlinked, which is no failure to read it
BOOST_AUTO_TEST_CASE( UploadRefusalIsNoItem )
{
    const MDW_RESULT                refused = MDW_RESULT::Fail( MDW_ERROR_CODE::NOT_SUPPORTED );
    std::optional<MDW_LOCATED_ITEM> located;

    BOOST_CHECK( MdwLocatedItem( false, refused, {}, wxS( "odoo" ), wxS( "amp-board.kicad_pro" ), located ).IsOk() );
    BOOST_CHECK( !located );

    BOOST_CHECK( MdwLocatedItem( true, refused, {}, wxS( "git" ), wxS( "amp-board.kicad_pro" ), located ).error
                 == MDW_ERROR_CODE::NOT_SUPPORTED );
}


// Odoo names the revision a design never checked out is based on, and Release Design reads it from here
BOOST_AUTO_TEST_CASE( LocatedItemCarriesItsBase )
{
    MDW_LOCATED_ITEM found;
    found.item.item_id = wxS( "1898/amp-board.kicad_pro" );
    found.base_revision_id = wxS( "7010" );
    found.base_revision_label = wxS( "1" );

    std::optional<MDW_LOCATED_ITEM> located;

    BOOST_REQUIRE( MdwLocatedItem( false, MDW_RESULT::Ok(), { found }, wxS( "odoo" ), wxS( "amp-board.kicad_pro" ),
                                   located )
                           .IsOk() );
    BOOST_REQUIRE( located );
    BOOST_CHECK( located->item.item_id == wxS( "1898/amp-board.kicad_pro" ) );
    BOOST_CHECK( located->item.source == wxS( "odoo" ) );
    BOOST_CHECK( located->item.path_hint == wxS( "amp-board.kicad_pro" ) );
    BOOST_CHECK( located->base_revision_id == wxS( "7010" ) );
    BOOST_CHECK( located->base_revision_label == wxS( "1" ) );
}


BOOST_AUTO_TEST_SUITE_END()
