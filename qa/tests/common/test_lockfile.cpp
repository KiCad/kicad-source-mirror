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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>

#include <lockfile.h>
#include <wildcards_and_files_ext.h>

#include <wx/ffile.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/utils.h>


namespace
{

wxString makeTempTargetPath( const wxString& aTag )
{
    wxString tempDir = wxFileName::GetTempDir();
    wxString leaf = wxString::Format( wxT( "kicad-lockfile-%s-%ld.kicad_pcb" ), aTag,
                                      static_cast<long>( wxGetLocalTimeMillis().GetValue() ) );
    return tempDir + wxFileName::GetPathSeparator() + leaf;
}


wxString lockPathFor( const wxString& aTargetPath )
{
    wxFileName fn( aTargetPath );
    fn.SetName( FILEEXT::LockFilePrefix + fn.GetName() );
    fn.SetExt( fn.GetExt() + '.' + FILEEXT::LockFileExtension );
    return fn.GetFullPath();
}


void writeRaw( const wxString& aPath, const std::string& aContent )
{
    wxFFile fp( aPath, wxT( "wb" ) );
    BOOST_REQUIRE( fp.IsOpened() );

    if( !aContent.empty() )
        BOOST_REQUIRE( fp.Write( aContent.data(), aContent.size() ) == aContent.size() );

    fp.Close();
}

} // anonymous namespace


BOOST_AUTO_TEST_SUITE( LockFileTests )


BOOST_AUTO_TEST_CASE( AcquireFreshLock )
{
    wxString target = makeTempTargetPath( wxT( "fresh" ) );
    wxString lockPath = lockPathFor( target );

    BOOST_REQUIRE( !wxFileName::FileExists( lockPath ) );

    {
        LOCKFILE lock( target );
        BOOST_CHECK( lock.Valid() );
        BOOST_CHECK( lock.Locked() );
        BOOST_CHECK( wxFileName::FileExists( lockPath ) );
    }

    BOOST_CHECK( !wxFileName::FileExists( lockPath ) );
}


BOOST_AUTO_TEST_CASE( ForeignLockNotOwned )
{
    wxString target = makeTempTargetPath( wxT( "foreign" ) );
    wxString lockPath = lockPathFor( target );

    writeRaw( lockPath, R"({"username":"someone-else","hostname":"another-host"})" );

    {
        LOCKFILE lock( target );
        BOOST_CHECK( !lock.Valid() );
        BOOST_CHECK( !lock.IsLockedByMe() );
        BOOST_CHECK_EQUAL( lock.GetUsername(), wxString( "someone-else" ) );
        BOOST_CHECK_EQUAL( lock.GetHostname(), wxString( "another-host" ) );
    }

    wxRemoveFile( lockPath );
}


// Regression: #23734 — empty lock (unfinished cloud sync) must be reclaimable, not a hard error.
BOOST_AUTO_TEST_CASE( EmptyStaleLockIsReclaimable )
{
    wxString target = makeTempTargetPath( wxT( "empty" ) );
    wxString lockPath = lockPathFor( target );

    writeRaw( lockPath, "" );

    {
        LOCKFILE lock( target );
        BOOST_CHECK( !lock.Valid() );
        BOOST_CHECK( lock.IsLockedByMe() );
        BOOST_CHECK( lock.OverrideLock() );
        BOOST_CHECK( lock.Valid() );
    }

    BOOST_CHECK( !wxFileName::FileExists( lockPath ) );
}


// Regression: #23734 — corrupt/truncated lock (partial sync) must behave the same as an empty one.
BOOST_AUTO_TEST_CASE( CorruptStaleLockIsReclaimable )
{
    wxString target = makeTempTargetPath( wxT( "corrupt" ) );
    wxString lockPath = lockPathFor( target );

    writeRaw( lockPath, R"({"username":"partia)" );

    {
        LOCKFILE lock( target );
        BOOST_CHECK( !lock.Valid() );
        BOOST_CHECK( lock.IsLockedByMe() );
        BOOST_CHECK( lock.GetUsername().IsEmpty() );
        BOOST_CHECK( lock.GetHostname().IsEmpty() );
    }

    if( wxFileName::FileExists( lockPath ) )
        wxRemoveFile( lockPath );
}


BOOST_AUTO_TEST_SUITE_END()
