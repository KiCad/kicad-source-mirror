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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * File locking utilities
 * @file lockfile.h
 */

#ifndef INCLUDE__LOCK_FILE_H_
#define INCLUDE__LOCK_FILE_H_

#include <wx/wx.h>
#include <wx/file.h>
#include <wx/filefn.h>
#include <wx/log.h>
#include <wx/filename.h>
#include <json_common.h>
#include <wildcards_and_files_ext.h>

#define LCK "KICAD_LOCKING"

class LOCKFILE
{
public:
    LOCKFILE( const wxString &filename, bool aRemoveOnRelease = true ) :
            m_originalFile( filename ), m_fileCreated( false ), m_status( false ),
            m_removeOnRelease( aRemoveOnRelease ), m_errorMsg( "" )
    {
        if( filename.IsEmpty() )
            return;

        wxLogTrace( LCK, "Trying to lock %s", filename );
        wxFileName fn( filename );
        fn.SetName( FILEEXT::LockFilePrefix + fn.GetName() );
        fn.SetExt( fn.GetExt() + '.' + FILEEXT::LockFileExtension );

        if( !fn.IsDirWritable() )
        {
            wxLogTrace( LCK, "File is not writable: %s", filename );
            m_status = true;
            m_removeOnRelease = false;
            return;
        }

        m_lockFilename = fn.GetFullPath();

        wxFile file;
        try
        {
            bool lock_success = false;
            bool rw_success = false;

            {
                wxLogNull suppressExpectedErrorMessages;

                lock_success = file.Open( m_lockFilename, wxFile::write_excl );

                if( !lock_success )
                    rw_success = file.Open( m_lockFilename, wxFile::read );
            }

            if( lock_success )
            {
                // Lock file doesn't exist, create one
                m_fileCreated = true;
                m_status = true;
                m_username = wxGetUserId();
                m_hostname = wxGetHostName();
                nlohmann::json j;
                j["username"] = std::string( m_username.mb_str() );
                j["hostname"] = std::string( m_hostname.mb_str() );
                std::string lock_info = j.dump();
                file.Write( lock_info );
                file.Close();
                wxLogTrace( LCK, "Locked %s", filename );
            }
            else if( rw_success )
            {
                // Lock file already exists, read the details
                wxString lock_info;
                file.ReadAll( &lock_info );
                nlohmann::json j = nlohmann::json::parse( std::string( lock_info.mb_str() ) );
                m_username = wxString( j["username"].get<std::string>() );
                m_hostname = wxString( j["hostname"].get<std::string>() );
                file.Close();
                m_errorMsg = _( "Lock file already exists" );
                wxLogTrace( LCK, "Existing Lock for %s", filename );
            }
            else
            {
                throw std::runtime_error( "Failed to open lock file" );
            }
        }
        catch( std::exception& e )
        {
            wxLogError( "Got an error trying to lock %s: %s", filename, e.what() );

            // Delete lock file if it was created above but we threw an exception somehow
            if( m_fileCreated )
            {
                wxRemoveFile( m_lockFilename );
                m_fileCreated = false; // Reset the flag since file has been deleted manually
            }

            m_errorMsg = _( "Failed to access lock file" );
            m_status = false;
        }
    }

    LOCKFILE( LOCKFILE&& other ) noexcept :
            m_originalFile( std::move( other.m_originalFile ) ),
            m_lockFilename( std::move( other.m_lockFilename ) ),
            m_username( std::move( other.m_username ) ),
            m_hostname( std::move( other.m_hostname ) ),
            m_fileCreated( other.m_fileCreated ),
            m_status( other.m_status ),
            m_removeOnRelease( other.m_removeOnRelease ),
            m_errorMsg( std::move( other.m_errorMsg ) )
    {
        // Disable unlock in the moved-from object
        other.m_fileCreated = false;
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
        wxLogTrace( LCK, "Unlocking %s", m_lockFilename );

        // Delete lock file only if the file was created in the constructor and if the file
        // contains the correct user and host names.
        if( m_fileCreated && checkUserAndHost() )
        {
            if( m_removeOnRelease )
                wxRemoveFile( m_lockFilename );

            m_fileCreated = false; // Reset the flag since file has been deleted manually
            m_status = false;
            m_errorMsg = wxEmptyString;
        }
    }

    /**
     * Force the lock, overwriting the data that existed already.
     *
     * @return True if we successfully overrode the lock
     */
    bool OverrideLock( bool aRemoveOnRelease = true )
    {
        wxLogTrace( LCK, "Overriding lock on %s", m_lockFilename );

        if( !m_fileCreated )
        {
            try
            {
                wxFile file;
                bool success = false;

                {
                    wxLogNull suppressExpectedErrorMessages;
                    success = file.Open( m_lockFilename, wxFile::write );
                }

                if( success )
                {
                    m_username = wxGetUserId();
                    m_hostname = wxGetHostName();
                    nlohmann::json j;
                    j["username"] = std::string( m_username.mb_str() );
                    j["hostname"] = std::string( m_hostname.mb_str() );
                    std::string lock_info = j.dump();
                    file.Write( lock_info );
                    file.Close();
                    m_fileCreated = true;
                    m_status = true;
                    m_removeOnRelease = aRemoveOnRelease;
                    m_errorMsg = wxEmptyString;
                    wxLogTrace( LCK, "Successfully overrode lock on %s", m_lockFilename );
                    return true;
                }

                return false;
            }
            catch( std::exception& e )
            {
                wxLogError( "Got exception trying to override lock on %s: %s",
                            m_lockFilename, e.what() );

                return false;
            }
        }
        else
        {
            wxLogTrace( LCK, "Upgraded lock on %s to delete on release", m_lockFilename );
            m_removeOnRelease = aRemoveOnRelease;
        }

        return true;
    }

    bool IsLockedByMe()
    {
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

    /**
     * @return Last error message generated.
     */
    wxString GetErrorMsg(){ return m_errorMsg; }

    bool Locked() const
    {
        return m_fileCreated;
    }

    bool Valid() const
    {
        return m_status;
    }

    explicit operator bool() const
    {
        return m_status;
    }

private:
    wxString m_originalFile;
    wxString m_lockFilename;
    wxString m_username;
    wxString m_hostname;
    bool m_fileCreated;
    bool m_status;
    bool m_removeOnRelease;
    wxString m_errorMsg;

    bool checkUserAndHost()
    {
        wxFileName fileName( m_lockFilename );

        if( !fileName.FileExists() )
        {
            wxLogTrace( LCK, "File does not exist: %s", m_lockFilename );
            return false;
        }

        wxFile file;

        try
        {
            if( file.Open( m_lockFilename, wxFile::read ) )
            {
                wxString lock_info;
                file.ReadAll( &lock_info );
                nlohmann::json j = nlohmann::json::parse( std::string( lock_info.mb_str() ) );

                if( m_username == wxString( j["username"].get<std::string>() )
                        && m_hostname == wxString( j["hostname"].get<std::string>() ) )
                {
                    wxLogTrace( LCK, "User and host match for lock %s", m_lockFilename );
                    return true;
                }
            }
        }
        catch( std::exception &e )
        {
            wxLogError( "Got exception trying to check user/host for lock on %s: %s",
                        m_lockFilename,
                        e.what() );
        }

        wxLogTrace( LCK, "User and host DID NOT match for lock %s", m_lockFilename );
        return false;
    }
};


#endif  // INCLUDE__LOCK_FILE_H_
