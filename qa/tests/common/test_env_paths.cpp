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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <env_paths.h>

#include <wx/filename.h>


BOOST_AUTO_TEST_SUITE( EnvPaths )


BOOST_AUTO_TEST_CASE( MakeRelativePathToProject )
{
#ifdef __WINDOWS__
    const wxString root = wxS( "C:\\" );
#else
    const wxString root = wxS( "/" );
#endif

    const wxString project = root + wxS( "work/board/" );

    BOOST_CHECK_EQUAL( MakeRelativePath( root + wxS( "work/board/out/fab.zip" ), project ), wxS( "out/fab.zip" ) );
    BOOST_CHECK_EQUAL( MakeRelativePath( root + wxS( "work/board/out/" ), project ), wxS( "out/" ) );
    BOOST_CHECK_EQUAL( MakeRelativePath( root + wxS( "work/sheets/a4.kicad_wks" ), project ),
                       wxS( "../sheets/a4.kicad_wks" ) );
    BOOST_CHECK_EQUAL( MakeRelativePath( root + wxS( "work/board" ), project ), wxS( "." ) );
    BOOST_CHECK_EQUAL( MakeRelativePath( wxS( "out/fab.zip" ), project ), wxS( "out/fab.zip" ) );
    BOOST_CHECK_EQUAL( MakeRelativePath( root + wxS( "work/board/out/" ), wxEmptyString ),
                       root + wxS( "work/board/out/" ) );

#ifdef __WINDOWS__
    BOOST_CHECK_EQUAL( MakeRelativePath( wxS( "D:\\out\\fab.zip" ), project ), wxS( "D:\\out\\fab.zip" ) );
#endif
}


BOOST_AUTO_TEST_CASE( NormalizePathForProjectPrecedence )
{
    wxFileName tempDir = wxFileName::DirName( wxFileName::GetTempDir() );
    tempDir.Normalize( wxPATH_NORM_ALL & ~wxPATH_NORM_CASE );

    const wxString   temp = tempDir.GetPathWithSep();
    const wxString   sheet = temp + wxS( "sheets" ) + wxFileName::GetPathSeparator() + wxS( "a4.kicad_wks" );
    const wxString   project = temp + wxS( "board" );
    const ENV_VAR_MAP envVars = { { wxS( "SHARED" ), ENV_VAR_ITEM( temp ) } };

    BOOST_CHECK_EQUAL( NormalizePathForProject( project + wxS( "/out/fab.zip" ), &envVars, project, false ),
                       wxS( "out/fab.zip" ) );
    BOOST_CHECK_EQUAL( NormalizePathForProject( sheet, &envVars, project, true ),
                       wxS( "${SHARED}/sheets/a4.kicad_wks" ) );
    BOOST_CHECK_EQUAL( NormalizePathForProject( sheet, nullptr, project, false ), sheet );
    BOOST_CHECK_EQUAL( NormalizePathForProject( sheet, nullptr, project, true ), wxS( "../sheets/a4.kicad_wks" ) );
}


BOOST_AUTO_TEST_SUITE_END()
