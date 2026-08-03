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

#include <mdw/client/mdw_op_state.h>
#include <mdw/client/mdw_paged_read.h>


namespace
{

/// A backend whose every page lists one row and names aNext(page number) as the page after it
struct BACKEND
{
    std::function<wxString( int )> next;
    int                            asked = 0;

    MDW_OP Fetch( const MDW_PAGE&, MDW_DONE<MDW_PAGED<wxString>> aDone )
    {
        MDW_PAGED<wxString> page;
        page.rows.push_back( wxString::Format( wxS( "row%d" ), asked ) );
        page.next_page_token = next( asked++ );

        auto state = std::make_shared<MDW_OP_STATE>();
        state->Complete(
                [done = std::move( aDone ), page]()
                {
                    done( MDW_RESULT::Ok(), page );
                } );

        return MDW_OP( state );
    }

    std::pair<MDW_RESULT, MDW_PAGED<wxString>> Read( int aMaxPages, MDW_READ_CAP aCap )
    {
        return MdwSync<MDW_PAGED<wxString>>(
                [&]( MDW_DONE<MDW_PAGED<wxString>> aDone )
                {
                    return MdwReadAllPages<wxString>(
                            [this]( const MDW_PAGE& aPage, MDW_DONE<MDW_PAGED<wxString>> aPageDone )
                            {
                                return Fetch( aPage, std::move( aPageDone ) );
                            },
                            0, std::move( aDone ), aCap, aMaxPages );
                } );
    }
};

} // namespace


BOOST_AUTO_TEST_SUITE( MdwPagedRead )


// A backend sending back a token it was given would otherwise grow the list without end
BOOST_AUTO_TEST_CASE( RepeatedTokenEndsTheRead )
{
    BACKEND backend;
    backend.next = []( int )
    {
        return wxString( wxS( "again" ) );
    };

    auto [result, read] = backend.Read( MDW_MAX_PAGES, MDW_READ_CAP::FAIL );

    BOOST_CHECK( result.error == MDW_ERROR_CODE::PROTOCOL );
    BOOST_CHECK_EQUAL( backend.asked, 2 );
    BOOST_CHECK_EQUAL( read.rows.size(), 2u );
}


// A short list must not pass for the whole one, so the cap fails the read and still hands over the rows
BOOST_AUTO_TEST_CASE( PageCapFailsTheRead )
{
    BACKEND backend;
    backend.next = []( int aPage )
    {
        return wxString::Format( wxS( "page%d" ), aPage + 1 );
    };

    auto [result, read] = backend.Read( 3, MDW_READ_CAP::FAIL );

    BOOST_CHECK( !result.IsOk() );
    BOOST_CHECK( result.error == MDW_ERROR_CODE::PROTOCOL );
    BOOST_CHECK_EQUAL( backend.asked, 3 );
    BOOST_CHECK_EQUAL( read.rows.size(), 3u );
    BOOST_CHECK_EQUAL( read.next_page_token, wxS( "page3" ) );
}


// A caller that a short list serves opts in and still learns which page was left
BOOST_AUTO_TEST_CASE( PageCapEndsAPartialRead )
{
    BACKEND backend;
    backend.next = []( int aPage )
    {
        return wxString::Format( wxS( "page%d" ), aPage + 1 );
    };

    auto [result, read] = backend.Read( 3, MDW_READ_CAP::PARTIAL );

    BOOST_CHECK( result.IsOk() );
    BOOST_CHECK_EQUAL( backend.asked, 3 );
    BOOST_CHECK_EQUAL( read.rows.size(), 3u );
    BOOST_CHECK_EQUAL( read.next_page_token, wxS( "page3" ) );
}


// A wait ended by stopping the workers leaves a page's request, whose callback holds the read, to its canceller
BOOST_AUTO_TEST_CASE( CancelAfterTheWorkersStopWithdrawsThePage )
{
    struct WORKER_RESTART
    {
        ~WORKER_RESTART() { MdwStartWorkers(); }
    } restart;

    std::shared_ptr<MDW_OP_STATE> state;
    int                           withdrawn = 0;

    {
        MDW_SYNC_SCOPE scope;
        state = std::make_shared<MDW_OP_STATE>();
    }

    state->SetCanceller(
            [&]()
            {
                ++withdrawn;
            } );

    BOOST_REQUIRE( MdwStopWorkers( std::chrono::seconds( 5 ) ) );

    MDW_OP op( state );
    op.Wait();
    op.Cancel();
    op.Cancel();

    BOOST_CHECK_EQUAL( withdrawn, 1 );
}


BOOST_AUTO_TEST_SUITE_END()
