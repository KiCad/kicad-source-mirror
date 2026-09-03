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

#include <boost/test/unit_test.hpp>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <project/project_archiver.h>
#include <reporter.h>

#include <wx/ffile.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>

#include <set>
#include <string>

#if !defined( _WIN32 )
#include <sys/stat.h>
#include <unistd.h>
#endif


BOOST_AUTO_TEST_SUITE( ProjectArchiver )

#if !defined( _WIN32 )

namespace
{

wxString makeProjectDir( const wxString& aTag )
{
    wxString dir = wxFileName::GetTempDir() + wxFileName::GetPathSeparator()
                   + wxString::Format( wxT( "kicad-archiver-%s-%ld" ), aTag,
                                       static_cast<long>( wxGetLocalTimeMillis().GetValue() ) );

    BOOST_REQUIRE( wxFileName::Mkdir( dir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );
    return dir;
}


void writeFile( const wxString& aPath, const std::string& aContent )
{
    wxFFile fp( aPath, wxT( "wb" ) );
    BOOST_REQUIRE( fp.IsOpened() );
    BOOST_REQUIRE( fp.Write( aContent.data(), aContent.size() ) == aContent.size() );
    fp.Close();
}


// Returns false when the file stays readable, which happens when the tests run as root.
bool makeUnreadable( const wxString& aPath )
{
    if( chmod( aPath.fn_str(), 0 ) != 0 )
        return false;

    wxLogNull noLog;
    wxFFile   probe( aPath, wxT( "rb" ) );
    return !probe.IsOpened();
}


std::set<wxString> zipEntryNames( const wxString& aZipPath )
{
    std::set<wxString> names;
    wxFFileInputStream stream( aZipPath );

    if( !stream.IsOk() )
        return names;

    wxZipInputStream zip( stream );

    while( wxZipEntry* entry = zip.GetNextEntry() )
    {
        names.insert( entry->GetName() );
        delete entry;
    }

    return names;
}


void removeProjectDir( const wxString& aDir, const wxString& aUnreadable )
{
    chmod( aUnreadable.fn_str(), 0600 );
    wxFileName::Rmdir( aDir, wxPATH_RMDIR_RECURSIVE );
}

} // anonymous namespace


BOOST_AUTO_TEST_CASE( Archive_UnreadableFileRaisesNoSystemError )
{
    wxString dir = makeProjectDir( wxT( "nodialog" ) );
    wxString board = dir + wxFileName::GetPathSeparator() + wxT( "board.kicad_pcb" );
    wxString sheet = dir + wxFileName::GetPathSeparator() + wxT( "sheet.kicad_sch" );
    wxString zip = dir + wxT( ".zip" );

    writeFile( board, "(kicad_pcb)\n" );
    writeFile( sheet, "(kicad_sch)\n" );

    if( !makeUnreadable( sheet ) )
    {
        BOOST_TEST_MESSAGE( "skipped, the file could not be made unreadable" );
        removeProjectDir( dir, sheet );
        return;
    }

    KI_TEST::SCOPED_COUNTING_WXLOG logOverride( nullptr, wxLOG_Warning );
    WX_STRING_REPORTER             reporter;

    PROJECT_ARCHIVER::Archive( dir, zip, reporter );

    BOOST_REQUIRE_EQUAL( logOverride.GetCount(), 0u );

    wxRemoveFile( zip );
    removeProjectDir( dir, sheet );
}


BOOST_AUTO_TEST_CASE( Archive_ReportsUnreadableFileWhenNotVerbose )
{
    wxString dir = makeProjectDir( wxT( "quietreport" ) );
    wxString board = dir + wxFileName::GetPathSeparator() + wxT( "board.kicad_pcb" );
    wxString sheet = dir + wxFileName::GetPathSeparator() + wxT( "sheet.kicad_sch" );
    wxString zip = dir + wxT( ".zip" );

    writeFile( board, "(kicad_pcb)\n" );
    writeFile( sheet, "(kicad_sch)\n" );

    if( !makeUnreadable( sheet ) )
    {
        BOOST_TEST_MESSAGE( "skipped, the file could not be made unreadable" );
        removeProjectDir( dir, sheet );
        return;
    }

    wxLogNull          noLog;
    WX_STRING_REPORTER reporter;

    PROJECT_ARCHIVER::Archive( dir, zip, reporter, false );

    BOOST_REQUIRE( reporter.HasMessageOfSeverity( RPT_SEVERITY_ERROR ) );
    BOOST_REQUIRE( reporter.GetMessages().Contains( wxT( "'sheet.kicad_sch': " ) ) );

    wxRemoveFile( zip );
    removeProjectDir( dir, sheet );
}


BOOST_AUTO_TEST_CASE( Archive_KeepsPartialArchiveWhenAFileIsUnreadable )
{
    wxString dir = makeProjectDir( wxT( "partial" ) );
    wxString board = dir + wxFileName::GetPathSeparator() + wxT( "board.kicad_pcb" );
    wxString sheet = dir + wxFileName::GetPathSeparator() + wxT( "sheet.kicad_sch" );
    wxString zip = dir + wxT( ".zip" );

    writeFile( board, "(kicad_pcb)\n" );
    writeFile( sheet, "(kicad_sch)\n" );

    if( !makeUnreadable( sheet ) )
    {
        BOOST_TEST_MESSAGE( "skipped, the file could not be made unreadable" );
        removeProjectDir( dir, sheet );
        return;
    }

    wxLogNull          noLog;
    WX_STRING_REPORTER reporter;

    BOOST_REQUIRE( PROJECT_ARCHIVER::Archive( dir, zip, reporter ) );

    std::set<wxString> names = zipEntryNames( zip );
    BOOST_REQUIRE( names.count( wxT( "board.kicad_pcb" ) ) == 1 );
    BOOST_REQUIRE( names.count( wxT( "sheet.kicad_sch" ) ) == 0 );

    wxRemoveFile( zip );
    removeProjectDir( dir, sheet );
}

#endif // !_WIN32

BOOST_AUTO_TEST_SUITE_END()
