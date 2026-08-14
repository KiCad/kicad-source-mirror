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

/**
 * File locking utilities
 * @file lockfile.h
 */

#ifndef INCLUDE__LOCK_FILE_H_
#define INCLUDE__LOCK_FILE_H_

#include <wx/wx.h>
#include <wx/filefn.h>
#include <wx/log.h>
#include <wx/filename.h>
#include <json_common.h>
#include <kiplatform/io.h>
#include <wildcards_and_files_ext.h>

#include <cstdint>
#include <random>
#include <string>

/**
 * Flag to enable project lock file debug tracing.
 *
 * Use "KICAD_LOCKING" to enable.
 *
 * @ingroup trace_env_vars
 */
static const wxChar* traceLockFile = wxT( "KICAD_LOCKING" );

/**
 * Advisory lock over a file, taken by writing a sibling lock file and holding an exclusive
 * lock on it for as long as this object lives.
 *
 * The operating system drops that lock when the owning process dies, so a lock file we can
 * lock is one whose writer is gone and whose contents we may take over.  That keeps a session
 * killed by a crash from pinning its project read-only forever, without ever guessing at the
 * liveness of a process we cannot see.
 */
class LOCKFILE
{
public:
    LOCKFILE( const wxString &filename, bool aRemoveOnRelease = true ) :
            m_removeOnRelease( aRemoveOnRelease )
    {
        if( filename.IsEmpty() )
            return;

        wxLogTrace( traceLockFile, "Trying to lock %s", filename );
        m_lockFilename = LockPathFor( filename );

        if( !wxFileName( m_lockFilename ).IsDirWritable() )
        {
            wxLogTrace( traceLockFile, "File is not writable: %s", filename );
            m_status = true;
            m_removeOnRelease = false;
            return;
        }

        bool created = false;
        KIPLATFORM::IO::FILE_LOCK::STATE state = m_lock.Acquire( m_lockFilename, created );

        if( !m_lock.IsOpen() )
        {
            wxLogTrace( traceLockFile, "Could not open a lock file for %s", filename );
            return;
        }

        // Creating the file does not make the lock ours, since another process can take the lock
        // between our create and our own attempt.  Exclusive creation decides only where the
        // filesystem cannot lock at all.
        if( created && state != KIPLATFORM::IO::FILE_LOCK::STATE::BUSY )
        {
            claim();
            wxLogTrace( traceLockFile, "Locked %s", filename );
            return;
        }

        readOwner();

        // Whoever wrote this lock holds it until their process dies, so a lock we can take is
        // one nobody is using.  Only our own is safe to take over: where the filesystem locks
        // locally, as network shares often do, another user's lock may be live elsewhere.
        if( state == KIPLATFORM::IO::FILE_LOCK::STATE::HELD && IsLockedByMe() )
        {
            claim();
            wxLogTrace( traceLockFile, "Reclaimed the abandoned lock on %s", filename );
            return;
        }

        wxLogTrace( traceLockFile, "Existing Lock for %s", filename );
    }

    /**
     * Look at a lock without taking it: nothing is created, nothing is claimed and nothing is
     * removed on release, so a caller that only wants to know who holds a lock cannot disturb
     * it.  Valid() then answers whether the lock is free rather than whether we hold it, and
     * Locked() is always false.
     */
    static LOCKFILE Inspect( const wxString& aFilename )
    {
        LOCKFILE probe;
        bool     heldByAnother = false;

        if( !aFilename.IsEmpty() )
        {
            probe.m_lockFilename = LockPathFor( aFilename );

            if( probe.m_lock.OpenForInspect( probe.m_lockFilename, heldByAnother ) )
                probe.readOwner();
        }

        probe.m_status = !heldByAnother;

        return probe;
    }

    /**
     * @return the path of the lock file that guards aFilename.
     */
    static wxString LockPathFor( const wxString& aFilename )
    {
        wxFileName fn( aFilename );
        fn.SetName( FILEEXT::LockFilePrefix + fn.GetName() );
        fn.SetExt( fn.GetExt() + '.' + FILEEXT::LockFileExtension );

        return fn.GetFullPath();
    }

    LOCKFILE( LOCKFILE&& other ) noexcept :
            m_lockFilename( std::move( other.m_lockFilename ) ),
            m_username( std::move( other.m_username ) ),
            m_hostname( std::move( other.m_hostname ) ),
            m_token( std::move( other.m_token ) ),
            m_lock( std::move( other.m_lock ) ),
            m_owned( other.m_owned ),
            m_status( other.m_status ),
            m_removeOnRelease( other.m_removeOnRelease )
    {
        // Disable unlock in the moved-from object
        other.m_owned = false;
    }

    ~LOCKFILE()
    {
        UnlockFile();
    }

    /**
     * Unlock and remove the file from the filesystem as long as we still own it.
     */
    void UnlockFile()
    {
        wxLogTrace( traceLockFile, "Unlocking %s", m_lockFilename );

        // Remove the file before dropping the lock, so that the window in which another
        // process could adopt a lock we are abandoning never opens
        if( m_owned && stillOwnLock() )
        {
            if( m_removeOnRelease )
                wxRemoveFile( m_lockFilename );

            m_owned = false;
            m_status = false;
        }

        m_lock.Release();
    }

    /**
     * Force the lock, overwriting the data that existed already.
     *
     * @return True if we successfully overrode the lock
     */
    bool OverrideLock( bool aRemoveOnRelease = true )
    {
        wxLogTrace( traceLockFile, "Overriding lock on %s", m_lockFilename );

        if( !m_owned && m_lock.IsOpen() )
            claim();

        m_removeOnRelease = aRemoveOnRelease;

        return m_status;
    }

    bool IsLockedByMe()
    {
        // Empty owner means the lock file could not be parsed (e.g. partial cloud sync).
        // We cannot prove another user owns it, so treat it as reclaimable.
        if( m_username.IsEmpty() && m_hostname.IsEmpty() )
            return true;

        return m_username == wxGetUserId() && m_hostname == wxGetHostName();
    }

    /**
     * @return Current username.  If we own the lock, this is us.  Otherwise, this is the user
     *         that does own it.
     */
    wxString GetUsername(){ return m_username; }

    /**
     * @return Current hostname.  If we own the lock this is our computer.  Otherwise, this is
     *         the computer that does.
     */
    wxString GetHostname(){ return m_hostname; }

    bool Locked() const
    {
        return m_owned;
    }

    bool Valid() const
    {
        return m_status;
    }

private:
    // Only Inspect() builds a lock that holds nothing
    LOCKFILE() = default;

    wxString m_lockFilename;
    wxString m_username;
    wxString m_hostname;
    wxString m_token;
    KIPLATFORM::IO::FILE_LOCK m_lock;
    bool m_owned = false;
    bool m_status = false;
    bool m_removeOnRelease = false;

    /**
     * @return a value no other lock will carry.  Process ids cannot serve here: sandboxed
     *         KiCads each number their processes from one, so two of them collide routinely.
     */
    static wxString newToken()
    {
        std::random_device rd;

        uint64_t high = ( static_cast<uint64_t>( rd() ) << 32 ) | rd();
        uint64_t low = ( static_cast<uint64_t>( rd() ) << 32 ) | rd();

        return wxString::Format( wxS( "%016llx%016llx" ),
                                 static_cast<unsigned long long>( high ),
                                 static_cast<unsigned long long>( low ) );
    }

    void claim()
    {
        m_username = wxGetUserId();
        m_hostname = wxGetHostName();
        m_token = newToken();

        nlohmann::json j;
        j["username"] = std::string( m_username.mb_str() );
        j["hostname"] = std::string( m_hostname.mb_str() );
        j["token"] = std::string( m_token.mb_str() );

        if( m_lock.Rewrite( j.dump() ) )
        {
            m_owned = true;
            m_status = true;
        }
        else
        {
            wxLogTrace( traceLockFile, "Could not write the lock record for %s", m_lockFilename );
        }
    }

    /**
     * @return true if the lock file holds a record we could parse.  Cloud-synced drives can
     *         present a lock file before its contents finish syncing, so an empty or corrupt
     *         one counts as an unknown owner rather than a hard error.
     */
    bool readRecord( nlohmann::json& aRecord ) const
    {
        std::string contents;

        if( !m_lock.ReadAll( contents ) )
            return false;

        aRecord = nlohmann::json::parse( contents, nullptr, false );

        if( aRecord.is_discarded() )
        {
            wxLogTrace( traceLockFile, "Unreadable lock contents for %s", m_lockFilename );
            return false;
        }

        return true;
    }

    void readOwner()
    {
        nlohmann::json record;

        if( readRecord( record ) )
        {
            m_username = wxString( record.value( "username", std::string() ) );
            m_hostname = wxString( record.value( "hostname", std::string() ) );
            m_token = wxString( record.value( "token", std::string() ) );
        }
        else
        {
            m_username = wxEmptyString;
            m_hostname = wxEmptyString;
            m_token = wxEmptyString;
        }
    }

    /**
     * @return true if the lock file still carries the record we wrote, so that a lock another
     *         process has since taken over is left alone.
     */
    bool stillOwnLock()
    {
        nlohmann::json record;

        if( m_token.IsEmpty() || !readRecord( record ) )
            return false;

        if( m_token == wxString( record.value( "token", std::string() ) ) )
            return true;

        wxLogTrace( traceLockFile, "Lock on %s is no longer ours", m_lockFilename );

        return false;
    }
};


#endif  // INCLUDE__LOCK_FILE_H_
