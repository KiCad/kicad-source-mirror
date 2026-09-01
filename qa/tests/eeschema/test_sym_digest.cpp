/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <diff_merge/sym_digest.h>
#include <diff_merge/sym_lib_differ.h>
#include <lib_symbol.h>
#include <sch_pin.h>
#include <widgets/mdw_part_controller.h>
#include <widgets/mdw_part_state.h>

#include <wx/filename.h>


using namespace KICAD_DIFF;


namespace
{
wxString deviceLibraryPath()
{
    wxFileName fn( KI_TEST::GetEeschemaTestDataDir(), wxS( "Device.kicad_sym" ) );
    fn.RemoveLastDir();
    fn.AppendDir( wxS( "libraries" ) );
    return fn.GetFullPath();
}
} // namespace


BOOST_AUTO_TEST_SUITE( SymDigest )


// A derived symbol publishes its parent's pins and graphics, so a parent's edit releases it anew
BOOST_AUTO_TEST_CASE( ParentEditChangesDerivedDigest )
{
    auto [owners, map] = SYM_LIB_DIFFER::LoadLibrary( deviceLibraryPath() );

    const LIB_SYMBOL* derived = map.at( wxS( "Filter_EMI_C" ) );
    LIB_SYMBOL*       parent = const_cast<LIB_SYMBOL*>( map.at( wxS( "C_Feedthrough" ) ) );

    BOOST_REQUIRE( derived->IsDerived() );

    const wxString before = SymbolDigest( *derived );
    SCH_PIN*       pin = parent->GetGraphicalPins().front();

    pin->SetLength( pin->GetLength() + schIUScale.MilsToIU( 50 ) );

    BOOST_CHECK_NE( SymbolDigest( *derived ), before );
}


// A release records the derived symbol as its library saves it, so moving it to an equivalent parent is a change to
// release, as Check In sees it. A release from before member digests still matches on the flattened symbol
BOOST_AUTO_TEST_CASE( ReparentedSymbolNoLongerMatchesItsRelease )
{
    auto [owners, map] = SYM_LIB_DIFFER::LoadLibrary( deviceLibraryPath() );

    LIB_SYMBOL* derived = const_cast<LIB_SYMBOL*>( map.at( wxS( "Filter_EMI_C" ) ) );
    LIB_SYMBOL  twin( *map.at( wxS( "C_Feedthrough" ) ) );

    twin.SetName( wxS( "C_Feedthrough_Twin" ) );

    MDW_RELEASE_ITEM item;
    SetReleaseDigests( item, *derived );

    MDW_REVISION release;
    release.revision_id = wxS( "f650295" );
    release.kind = MDW_REVISION_KIND::RELEASE;
    release.content_digest = item.digest;
    release.member_digest = item.memberDigest;

    BOOST_REQUIRE( !release.member_digest.IsEmpty() );

    derived->SetParent( &twin );

    MDW_PART_STATE state;
    state.release = release;
    state.digest = SymbolDigest( *derived );
    state.memberDigest = SymbolDigest( *derived, SYMBOL_DIGEST::MEMBER );

    BOOST_CHECK_EQUAL( state.digest, release.content_digest );
    BOOST_CHECK( !state.MatchesRelease() );

    state.release->member_digest.clear();

    BOOST_CHECK( state.MatchesRelease() );
}


BOOST_AUTO_TEST_SUITE_END()
