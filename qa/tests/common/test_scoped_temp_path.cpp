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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>

#include <gestfich.h>

#include <wx/ffile.h>
#include <wx/filename.h>


BOOST_AUTO_TEST_SUITE( ScopedTempPath )


BOOST_AUTO_TEST_CASE( RemovesWhatItCreated )
{
    wxString directory;
    wxString file;

    {
        SCOPED_TEMP_PATH tempDirectory;
        SCOPED_TEMP_PATH tempFile;
        BOOST_REQUIRE( tempDirectory.MakeDirectory( wxS( "kicad_qa_scoped" ) ) );
        BOOST_REQUIRE( tempFile.MakeFile( wxS( "kicad_qa_scoped" ) ) );
        directory = tempDirectory.Path();
        file = tempFile.Path();
        BOOST_CHECK( wxFileName::DirExists( directory ) );
        BOOST_CHECK( wxFileName::FileExists( file ) );

        wxFFile content( wxFileName( directory, wxS( "content" ) ).GetFullPath(), wxS( "w" ) );
        BOOST_REQUIRE( content.IsOpened() );
    }

    BOOST_CHECK( !wxFileName::DirExists( directory ) );
    BOOST_CHECK( !wxFileName::FileExists( file ) );
}


BOOST_AUTO_TEST_SUITE_END()
