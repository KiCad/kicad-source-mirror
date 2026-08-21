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

#include <lib_id.h>
#include <widgets/mdw_part_controller.h>


namespace
{

MDW_RELEASE_ITEM item( const wxString& aLibrary, const wxString& aName, bool aModified )
{
    MDW_RELEASE_ITEM part;

    part.id = LIB_ID( aLibrary, aName );
    part.name = aName;
    part.modified = aModified;

    return part;
}

} // namespace


BOOST_AUTO_TEST_SUITE( MdwReleasePromptTests )


// Unsaved edits are named and saved first, so the dialog never looks as it does for a clean part
BOOST_AUTO_TEST_CASE( UnsavedReleasedPartIsNamedAndSavedFirst )
{
    MDW_RELEASE_PROMPT prompt = MdwReleasePrompt( { item( wxS( "74xx" ), wxS( "74LS125" ), true ),
                                                    item( wxS( "Device" ), wxS( "C" ), false ) } );

    BOOST_REQUIRE_EQUAL( prompt.rows.size(), 2 );
    BOOST_REQUIRE_EQUAL( prompt.notes.GetCount(), 1 );
    BOOST_CHECK( prompt.notes[0].Contains( wxS( "74LS125" ) ) );
    BOOST_CHECK( !prompt.notes[0].StartsWith( wxS( "C " ) ) );
    BOOST_CHECK_EQUAL( prompt.okLabel, wxString( wxS( "Save and Release" ) ) );
}


BOOST_AUTO_TEST_CASE( CleanPartsReleaseWithoutSaving )
{
    MDW_RELEASE_PROMPT prompt = MdwReleasePrompt( { item( wxS( "Device" ), wxS( "C" ), false ) } );

    BOOST_CHECK( prompt.notes.IsEmpty() );
    BOOST_CHECK_EQUAL( prompt.okLabel, wxString( wxS( "Release" ) ) );
}


// Saving a library takes its other edited parts along, so the label says a save happens
BOOST_AUTO_TEST_CASE( BystanderSavedWithCleanPartIsNamedAndSaysSave )
{
    MDW_RELEASE_ITEM clean = item( wxS( "Device" ), wxS( "C" ), false );

    clean.unsaved.Add( wxS( "R" ) );

    MDW_RELEASE_PROMPT prompt = MdwReleasePrompt( { clean } );

    BOOST_REQUIRE_EQUAL( prompt.notes.GetCount(), 1 );
    BOOST_CHECK( prompt.notes[0].Contains( wxS( "R" ) ) );
    BOOST_CHECK_EQUAL( prompt.okLabel, wxString( wxS( "Save and Release" ) ) );
}


BOOST_AUTO_TEST_CASE( EveryUnsavedPartInABatchIsNamed )
{
    MDW_RELEASE_PROMPT prompt = MdwReleasePrompt( { item( wxS( "74xx" ), wxS( "74LS125" ), true ),
                                                    item( wxS( "Device" ), wxS( "C" ), true ),
                                                    item( wxS( "Device" ), wxS( "R" ), true ) } );

    BOOST_REQUIRE_EQUAL( prompt.notes.GetCount(), 3 );
    BOOST_CHECK( prompt.notes[0].Contains( wxS( "74LS125" ) ) );
    BOOST_CHECK( prompt.notes[1].StartsWith( wxS( "C " ) ) );
    BOOST_CHECK( prompt.notes[2].StartsWith( wxS( "R " ) ) );
}


// A stopped release says how many parts went out and how many did not, and a held part is neither
BOOST_AUTO_TEST_CASE( HeldPartsAreNeitherReleasedNorStopped )
{
    using OUTCOME = MDW_RELEASE_ROW::OUTCOME;

    std::vector<MDW_RELEASE_ROW> rows = { { false, OUTCOME::RELEASED }, { false, OUTCOME::FAILED },
                                          { true, OUTCOME::PENDING },   { false, OUTCOME::PENDING },
                                          { false, OUTCOME::PENDING } };

    MDW_RELEASE_TALLY tally = MdwReleaseTally( rows, 2 );

    BOOST_CHECK_EQUAL( tally.released, 1 );
    BOOST_CHECK_EQUAL( tally.failed, 1 );
    BOOST_CHECK_EQUAL( tally.stopped, 2 );

    BOOST_CHECK_EQUAL( MdwReleaseSummary( tally ),
                       wxString( wxS( "1 part released, 1 part failed, 2 parts stopped before release." ) ) );
    BOOST_CHECK( MdwReleaseSummary( MdwReleaseTally( rows, rows.size() ) ).IsEmpty() );
}


BOOST_AUTO_TEST_CASE( StopIsOfferedOnlyWhenAReleasablePartWaits )
{
    std::vector<MDW_RELEASE_ROW> rows = { {}, { true }, {} };

    BOOST_CHECK( MdwReleaseCanStop( rows, 0 ) );
    BOOST_CHECK( !MdwReleaseCanStop( rows, 2 ) );

    rows[2].held = true;
    BOOST_CHECK( !MdwReleaseCanStop( rows, 0 ) );
}


BOOST_AUTO_TEST_SUITE_END()
