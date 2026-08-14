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

#include <dialogs/mdw_library_checkin_rows.h>
#include <mdw/client/mdw_types.h>
#include <mdw/client/mdw_library.h>
#include <mdw/client/mdw_lifecycle.h>
#include <widgets/mdw_part_state.h>


BOOST_AUTO_TEST_SUITE( MdwRevisionLabels )


// A check-in's commit hash is no release, and a revising part still names the release it matches
BOOST_AUTO_TEST_CASE( OnlyReleasesAndBaselinesAreNamed )
{
    MDW_REVISION release;
    release.kind = MDW_REVISION_KIND::RELEASE;
    release.label = wxS( "1" );
    release.content_digest = wxS( "v1:capacitor" );

    MDW_REVISION baseline;
    baseline.kind = MDW_REVISION_KIND::BASELINE;
    baseline.label = wxS( "Baseline 1" );

    MDW_MEMBER_RESULT checkIn;
    checkIn.status = MDW_RESULT::Ok();
    checkIn.change.revision.kind = MDW_REVISION_KIND::CHECKIN;
    checkIn.change.revision.label = wxS( "594de9ee" );

    BOOST_CHECK_EQUAL( MdwRevisionText( release ), wxS( "Rev 1" ) );
    BOOST_CHECK_EQUAL( MdwRevisionText( baseline ), wxS( "Baseline 1" ) );
    BOOST_CHECK_EQUAL( MdwMemberOutcome( checkIn ), wxS( "Checked in" ) );

    MDW_PART_STATE part;
    part.lifecycle = true;
    part.known = true;
    part.state.category = MDW_LIFECYCLE_CATEGORY::EDITABLE;

    BOOST_CHECK_EQUAL( part.Describe( wxS( "L" ) ), wxS( "L · Editable · never released" ) );

    part.release = release;
    part.digest = release.content_digest;
    part.lock.is_locked = true;
    part.lock.locked_by_me = true;

    BOOST_CHECK_EQUAL( part.Describe( wxS( "C" ) ), wxS( "C · Editable · matches Rev 1 · revising" ) );
}


// A hook can print pages before it refuses, and the refusal comes last
BOOST_AUTO_TEST_CASE( LongTransportTextKeepsItsEnd )
{
    MDW_RESULT refused =
            MDW_RESULT::Fail( MDW_ERROR_CODE::PERMISSION_DENIED, wxS( "git.example.com refused the push." ) );

    for( int i = 0; i < 140; ++i )
        refused.detail << wxString::Format( wxS( "remote: hook step %d ran\n" ), i );

    refused.detail << wxS( "refs/heads/main: pre-receive hook declined" );

    const wxString text = MdwErrorText( refused );

    BOOST_CHECK_GT( refused.detail.length(), 3000u );
    BOOST_CHECK_LT( text.length(), refused.message.length() + 320 );
    BOOST_CHECK( text.StartsWith( refused.message ) );
    BOOST_CHECK( text.EndsWith( wxS( "pre-receive hook declined" ) ) );
    BOOST_CHECK( !text.Contains( wxS( "hook step 0 " ) ) );

    refused.detail = wxS( "pre-receive hook declined" );
    BOOST_CHECK_EQUAL( MdwErrorText( refused ), refused.message + wxS( "\n\npre-receive hook declined" ) );
}


BOOST_AUTO_TEST_SUITE_END()
