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

#include <lockfile.h>
#include <json_common.h>
#include <kiplatform/io.h>

#include <wx/ffile.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/utils.h>

#if !defined( __WINDOWS__ )
#include <unistd.h>
#endif


namespace
{

using FILE_LOCK = KIPLATFORM::IO::FILE_LOCK;


wxString makeTempTargetPath( const wxString& aTag )
{
    wxString tempDir = wxFileName::GetTempDir();
    wxString leaf = wxString::Format( wxT( "kicad-lockfile-%s-%ld.kicad_pcb" ), aTag,
                                      static_cast<long>( wxGetLocalTimeMillis().GetValue() ) );
    return tempDir + wxFileName::GetPathSeparator() + leaf;
}


void writeRaw( const wxString& aPath, const std::string& aContent )
{
    wxFFile fp( aPath, wxT( "wb" ) );
    BOOST_REQUIRE( fp.IsOpened() );

    if( !aContent.empty() )
        BOOST_REQUIRE( fp.Write( aContent.data(), aContent.size() ) == aContent.size() );

    fp.Close();
}


nlohmann::json readLockJson( const wxString& aPath )
{
    wxFFile fp( aPath, wxT( "rb" ) );
    BOOST_REQUIRE( fp.IsOpened() );

    wxString contents;
    BOOST_REQUIRE( fp.ReadAll( &contents ) );

    return nlohmann::json::parse( std::string( contents.mb_str() ) );
}


std::string selfOwnedLockJson()
{
    nlohmann::json j;
    j["username"] = std::string( wxGetUserId().mb_str() );
    j["hostname"] = std::string( wxGetHostName().mb_str() );
    j["token"] = "0123456789abcdef0123456789abcdef";
    return j.dump();
}


// Simulates another live KiCad process holding the OS lock
FILE_LOCK holdLockOn( const wxString& aLockPath )
{
    FILE_LOCK holder;
    bool      created = false;

    BOOST_REQUIRE( holder.Acquire( aLockPath, created ) == FILE_LOCK::STATE::HELD );

    return holder;
}


// Root ignores directory permissions, so the read-only cases can't be tested as root
bool canTestUnwritableDirs()
{
#if defined( __WINDOWS__ )
    return false;
#else
    return geteuid() != 0;
#endif
}

} // anonymous namespace


BOOST_AUTO_TEST_SUITE( LockFileTests )


BOOST_AUTO_TEST_CASE( AcquireFreshLock )
{
    wxString target = makeTempTargetPath( wxT( "fresh" ) );
    wxString lockPath = LOCKFILE::LockPathFor( target );

    BOOST_REQUIRE( !wxFileName::FileExists( lockPath ) );

    {
        LOCKFILE lock( target );
        BOOST_CHECK( lock.Valid() );
        BOOST_CHECK( lock.Locked() );
        BOOST_CHECK( wxFileName::FileExists( lockPath ) );
    }

    BOOST_CHECK( !wxFileName::FileExists( lockPath ) );
}


// A foreign lock is never stale to us and must survive so its owner can still be named
BOOST_AUTO_TEST_CASE( ForeignLockNotOwned )
{
    wxString target = makeTempTargetPath( wxT( "foreign" ) );
    wxString lockPath = LOCKFILE::LockPathFor( target );

    writeRaw( lockPath, R"({"username":"someone-else","hostname":"another-host"})" );

    {
        LOCKFILE lock( target );
        BOOST_CHECK( !lock.Valid() );
        BOOST_CHECK( !lock.IsLockedByMe() );
        BOOST_CHECK_EQUAL( lock.GetUsername(), wxString( "someone-else" ) );
        BOOST_CHECK_EQUAL( lock.GetHostname(), wxString( "another-host" ) );
    }

    BOOST_REQUIRE( wxFileName::FileExists( lockPath ) );

    LOCKFILE reread( target );
    BOOST_CHECK_EQUAL( reread.GetUsername(), wxString( "someone-else" ) );

    wxRemoveFile( lockPath );
}


// Regression: #23734 — empty lock (unfinished cloud sync) must be reclaimable, not a hard error.
BOOST_AUTO_TEST_CASE( EmptyStaleLockIsReclaimable )
{
    wxString target = makeTempTargetPath( wxT( "empty" ) );
    wxString lockPath = LOCKFILE::LockPathFor( target );

    writeRaw( lockPath, "" );

    {
        LOCKFILE lock( target );
        BOOST_CHECK( lock.Valid() );
        BOOST_CHECK_EQUAL( lock.GetUsername(), wxGetUserId() );
    }

    BOOST_CHECK( !wxFileName::FileExists( lockPath ) );
}


// Regression: #23734 — corrupt/truncated lock (partial sync) must behave the same as an empty one.
BOOST_AUTO_TEST_CASE( CorruptStaleLockIsReclaimable )
{
    wxString target = makeTempTargetPath( wxT( "corrupt" ) );
    wxString lockPath = LOCKFILE::LockPathFor( target );

    writeRaw( lockPath, R"({"username":"partia)" );

    {
        LOCKFILE lock( target );
        BOOST_CHECK( lock.Valid() );
    }

    BOOST_CHECK( !wxFileName::FileExists( lockPath ) );
}


// Issue #11458 - our own crashed session's lock must not pin the target read-only
BOOST_AUTO_TEST_CASE( AbandonedSelfLockIsReclaimed )
{
    wxString target = makeTempTargetPath( wxT( "abandoned" ) );
    wxString lockPath = LOCKFILE::LockPathFor( target );

    writeRaw( lockPath, selfOwnedLockJson() );

    {
        LOCKFILE lock( target );
        BOOST_CHECK( lock.Valid() );
        BOOST_CHECK( lock.Locked() );
        BOOST_CHECK_EQUAL( lock.GetUsername(), wxGetUserId() );

        // Reclaim must replace the owner record, or release would refuse to clean up
        BOOST_CHECK( readLockJson( lockPath ).value( "token", std::string() )
                     != "0123456789abcdef0123456789abcdef" );
    }

    BOOST_CHECK( !wxFileName::FileExists( lockPath ) );
}


// A live same-user lock, as when kicad holds it while pcbnew opens the same project, must never
// be taken
BOOST_AUTO_TEST_CASE( LiveLockIsNeverTaken )
{
    wxString target = makeTempTargetPath( wxT( "live" ) );
    wxString lockPath = LOCKFILE::LockPathFor( target );

    writeRaw( lockPath, selfOwnedLockJson() );

    FILE_LOCK owner = holdLockOn( lockPath );

    {
        LOCKFILE lock( target );
        BOOST_CHECK( !lock.Valid() );
        BOOST_CHECK( !lock.Locked() );
        BOOST_CHECK_EQUAL( lock.GetUsername(), wxGetUserId() );
    }

    BOOST_REQUIRE( wxFileName::FileExists( lockPath ) );
    BOOST_CHECK_EQUAL( readLockJson( lockPath ).value( "token", std::string() ),
                       std::string( "0123456789abcdef0123456789abcdef" ) );

    owner.Release();
    wxRemoveFile( lockPath );
}


// Inspect must not create, claim, or remove the lock it looks at
BOOST_AUTO_TEST_CASE( InspectDisturbsNothing )
{
    wxString target = makeTempTargetPath( wxT( "inspect" ) );
    wxString lockPath = LOCKFILE::LockPathFor( target );

    {
        LOCKFILE probe = LOCKFILE::Inspect( target );
        BOOST_CHECK( probe.Valid() );
        BOOST_CHECK( !probe.Locked() );
    }

    BOOST_CHECK( !wxFileName::FileExists( lockPath ) );

    writeRaw( lockPath, selfOwnedLockJson() );

    {
        LOCKFILE probe = LOCKFILE::Inspect( target );
        BOOST_CHECK( probe.Valid() );
        BOOST_CHECK( !probe.Locked() );
        BOOST_CHECK_EQUAL( probe.GetUsername(), wxGetUserId() );
    }

    BOOST_REQUIRE( wxFileName::FileExists( lockPath ) );
    BOOST_CHECK_EQUAL( readLockJson( lockPath ).value( "token", std::string() ),
                       std::string( "0123456789abcdef0123456789abcdef" ) );

    FILE_LOCK owner = holdLockOn( lockPath );

    {
        LOCKFILE probe = LOCKFILE::Inspect( target );
        BOOST_CHECK( !probe.Valid() );
        BOOST_CHECK_EQUAL( probe.GetUsername(), wxGetUserId() );
    }

    owner.Release();
    wxRemoveFile( lockPath );
}


// A second holder in this process is as foreign as any other process
BOOST_AUTO_TEST_CASE( SecondHolderOfTheSameTargetIsReadOnly )
{
    wxString target = makeTempTargetPath( wxT( "second" ) );

    LOCKFILE first( target );
    BOOST_REQUIRE( first.Valid() );

    {
        LOCKFILE second( target );
        BOOST_CHECK( !second.Valid() );
        BOOST_CHECK( !second.Locked() );
    }

    // Releasing the reader must not have disturbed the lock the first holder still owns
    BOOST_CHECK( wxFileName::FileExists( LOCKFILE::LockPathFor( target ) ) );
    BOOST_CHECK( first.Valid() );
}


// The user is still allowed to force a lock away from a live owner
BOOST_AUTO_TEST_CASE( LiveLockCanBeOverridden )
{
    wxString target = makeTempTargetPath( wxT( "override" ) );
    wxString lockPath = LOCKFILE::LockPathFor( target );

    writeRaw( lockPath, selfOwnedLockJson() );

    FILE_LOCK owner = holdLockOn( lockPath );

    {
        LOCKFILE lock( target );
        BOOST_REQUIRE( !lock.Valid() );

        BOOST_CHECK( lock.OverrideLock() );
        BOOST_CHECK( lock.Valid() );
    }

    owner.Release();

    if( wxFileName::FileExists( lockPath ) )
        wxRemoveFile( lockPath );
}


// Releasing a lock that someone else has since taken over must leave their lock alone
BOOST_AUTO_TEST_CASE( TakenOverLockNotRemovedOnRelease )
{
    wxString target = makeTempTargetPath( wxT( "taken-over" ) );
    wxString lockPath = LOCKFILE::LockPathFor( target );

    {
        LOCKFILE lock( target );
        BOOST_REQUIRE( lock.Valid() );

        nlohmann::json newOwner = readLockJson( lockPath );
        newOwner["token"] = "ffffffffffffffffffffffffffffffff";
        writeRaw( lockPath, newOwner.dump() );
    }

    BOOST_CHECK( wxFileName::FileExists( lockPath ) );
    wxRemoveFile( lockPath );
}


// A lock we can read but not write must still name its owner
BOOST_AUTO_TEST_CASE( UnwritableLockStillNamesItsOwner )
{
    if( !canTestUnwritableDirs() )
        return;

    wxString target = makeTempTargetPath( wxT( "unwritable" ) );
    wxString lockPath = LOCKFILE::LockPathFor( target );

    writeRaw( lockPath, R"({"username":"someone-else","hostname":"another-host"})" );
    BOOST_REQUIRE( wxFileName( lockPath ).SetPermissions( wxS_IRUSR ) );

    {
        LOCKFILE lock( target );
        BOOST_CHECK( !lock.Valid() );
        BOOST_CHECK_EQUAL( lock.GetUsername(), wxString( "someone-else" ) );
    }

    wxFileName( lockPath ).SetPermissions( wxS_IRUSR | wxS_IWUSR );
    wxRemoveFile( lockPath );
}


// An unwritable directory must not block opening a read-only project
BOOST_AUTO_TEST_CASE( UnwritableDirectoryDegradesToAllow )
{
    if( !canTestUnwritableDirs() )
        return;

    wxString dir = wxFileName::GetTempDir() + wxFileName::GetPathSeparator()
                   + wxString::Format( wxT( "kicad-lockfile-ro-%ld" ),
                                       static_cast<long>( wxGetLocalTimeMillis().GetValue() ) );

    BOOST_REQUIRE( wxFileName::Mkdir( dir, 0700, wxPATH_MKDIR_FULL ) );

    wxString target = dir + wxFileName::GetPathSeparator() + wxT( "ro.kicad_pro" );
    writeRaw( target, "{}" );

    BOOST_REQUIRE( wxFileName( dir ).SetPermissions( wxS_IRUSR | wxS_IXUSR ) );

    {
        LOCKFILE lock( target );
        BOOST_CHECK( lock.Valid() );
        BOOST_CHECK( !lock.Locked() );
        BOOST_CHECK( !wxFileName::FileExists( LOCKFILE::LockPathFor( target ) ) );
    }

    wxFileName( dir ).SetPermissions( wxS_IRUSR | wxS_IWUSR | wxS_IXUSR );
    wxRemoveFile( target );
    wxFileName::Rmdir( dir );
}


BOOST_AUTO_TEST_SUITE_END()
