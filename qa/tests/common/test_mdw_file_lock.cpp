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

#include <mdw/client/mdw_file_lock.h>

#include <kiplatform/io.h>
#include <wx/filename.h>

#include <chrono>
#include <thread>


BOOST_AUTO_TEST_SUITE( MdwFileLock )


// A holder that lets go after longer than any one store used to wait must still be waited out
BOOST_AUTO_TEST_CASE( WaitsOutAHolderLongerThanTwoSeconds )
{
    const wxString file = wxFileName::CreateTempFileName( wxS( "mdwlock" ) );

    KIPLATFORM::IO::FILE_LOCK holder;
    bool                      created = false;

    BOOST_REQUIRE( holder.Acquire( file + wxS( ".lock" ), created ) == KIPLATFORM::IO::FILE_LOCK::STATE::HELD );

    std::thread releaser(
            [&holder]()
            {
                std::this_thread::sleep_for( std::chrono::milliseconds( 2500 ) );
                holder.Release();
            } );

    bool ran = false;
    bool locked = MdwWithFileLock( file,
                                   [&ran]()
                                   {
                                       ran = true;
                                   } );

    releaser.join();
    wxRemoveFile( file );
    wxRemoveFile( file + wxS( ".lock" ) );

    BOOST_CHECK( locked );
    BOOST_CHECK( ran );
}


BOOST_AUTO_TEST_SUITE_END()
