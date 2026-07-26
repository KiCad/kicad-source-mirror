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

#include <mdw/mdw_claim_text.h>


BOOST_AUTO_TEST_SUITE( MdwClaimText )


// Git cannot refuse a teammate's push, so its claims must never read like an Odoo check out
BOOST_AUTO_TEST_CASE( ClaimTextBranchesOnAdvisory )
{
    MDW_LOCK_INFO dave;
    dave.is_locked = true;
    dave.owner_id = wxS( "dave@example.com" );
    dave.owner_display_name = wxS( "Dave" );
    dave.lock_timestamp = wxDateTime( 15, wxDateTime::Oct, 2026, 9, 24 );
    dave.advisory = true;

    wxString advisory = MdwClaimSavePrompt( dave, wxS( "amp-board.kicad_sch" ) );

    BOOST_CHECK( advisory.Contains( wxS( "Dave" ) ) );
    BOOST_CHECK( !advisory.Contains( wxS( "cannot be checked in" ) ) );

    dave.advisory = false;
    BOOST_CHECK( MdwClaimSavePrompt( dave, wxS( "amp-board.kicad_sch" ) ).Contains( wxS( "cannot be checked in" ) ) );

    // A claim left for a month must say so
    wxString label = MdwClaimLabel( dave, wxDateTime( 14, wxDateTime::Nov, 2026, 12, 0 ) );

    BOOST_CHECK( label.Contains( wxS( "Dave" ) ) );
    BOOST_CHECK( label.Contains( wxS( "30 days" ) ) );

    // Giving up one's own Odoo check out is what routes Stop Editing to CancelCheckout
    dave.locked_by_me = true;

    std::vector<MDW_CLAIM_ACTION> mine = MdwClaimActions( dave );

    BOOST_REQUIRE_EQUAL( mine.size(), 1 );
    BOOST_CHECK( mine[0].kind == MDW_CLAIM_ACTION::KIND::STOP );
    BOOST_CHECK_EQUAL( mine[0].label, wxS( "Undo Check Out" ) );
}


// Claims not read yet may hide a teammate's, so no surface may offer the item as free
BOOST_AUTO_TEST_CASE( UnknownClaimNeverReadsFree )
{
    MDW_LOCK_INFO unread;
    unread.known = false;

    std::vector<MDW_CLAIM_ACTION> actions = MdwClaimActions( unread );

    BOOST_REQUIRE_EQUAL( actions.size(), 1 );
    BOOST_CHECK( actions[0].kind == MDW_CLAIM_ACTION::KIND::START );
    BOOST_CHECK( actions[0].enabled );
    BOOST_CHECK_NE( actions[0].label, MdwClaimActions( MDW_LOCK_INFO() )[0].label );
    BOOST_CHECK( !MdwClaimLabel( unread, wxDateTime::Now() ).IsEmpty() );
    BOOST_CHECK(
            MdwClaimCommitWarning( unread, wxS( "amp-board.kicad_sch" ), false ).Contains( wxS( "not been read" ) ) );
    BOOST_CHECK( MdwClaimCommitWarning( unread, wxS( "amp-board.kicad_sch" ), false, 3 )
                         .Contains( wxS( "3 of these files have not" ) ) );
}


BOOST_AUTO_TEST_SUITE_END()
