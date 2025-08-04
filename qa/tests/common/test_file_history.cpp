/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

/**
 * @file
 * Test suite for FILE_HISTORY.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

// Code under test
#include <file_history.h>
#include <id.h>

#include <vector>

BOOST_AUTO_TEST_SUITE( FileHistory )

BOOST_AUTO_TEST_CASE( AddRemove )
{
    FILE_HISTORY history( 3, ID_FILE1, ID_FILE_LIST_CLEAR, "Clear" );

    history.AddFileToHistory( "file1" );
    history.AddFileToHistory( "file2" );
    history.AddFileToHistory( "file3" );

    BOOST_CHECK_EQUAL( history.GetCount(), 3 );
    BOOST_CHECK_EQUAL( history.GetHistoryFile( 0 ), wxString( "file3" ) );
    BOOST_CHECK_EQUAL( history.GetHistoryFile( 2 ), wxString( "file1" ) );

    history.AddFileToHistory( "file4" );

    BOOST_CHECK_EQUAL( history.GetCount(), 3 );
    BOOST_CHECK_EQUAL( history.GetHistoryFile( 0 ), wxString( "file4" ) );
    BOOST_CHECK_EQUAL( history.GetHistoryFile( 2 ), wxString( "file2" ) );

    history.RemoveFileFromHistory( 1 );
    BOOST_CHECK_EQUAL( history.GetCount(), 2 );
    BOOST_CHECK_EQUAL( history.GetHistoryFile( 1 ), wxString( "file2" ) );
}

BOOST_AUTO_TEST_CASE( Persistence )
{
    FILE_HISTORY history( 5, ID_FILE1, ID_FILE_LIST_CLEAR, "Clear" );

    history.AddFileToHistory( "a" );
    history.AddFileToHistory( "b" );
    history.AddFileToHistory( "c" );

    std::vector<wxString> saved;
    history.Save( &saved );

    FILE_HISTORY loaded( 5, ID_FILE1, ID_FILE_LIST_CLEAR, "Clear" );
    loaded.Load( saved );

    BOOST_CHECK_EQUAL( loaded.GetCount(), 3 );
    BOOST_CHECK_EQUAL( loaded.GetHistoryFile( 0 ), wxString( "a" ) );
    BOOST_CHECK_EQUAL( loaded.GetHistoryFile( 2 ), wxString( "c" ) );

    loaded.ClearFileHistory();
    BOOST_CHECK_EQUAL( loaded.GetCount(), 0 );
}

BOOST_AUTO_TEST_SUITE_END()
