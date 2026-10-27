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

#include "sqlite_backend.h"

#include "db_backend_support.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <mutex>
#include <thread>

#include <sqlite3.h>

#include <wx/intl.h>
#include <wx/log.h>
#include <wx/string.h>

#if defined( _WIN32 )
#include <windows.h>
#else
#include <sys/stat.h>
#endif

#if defined( __linux__ )
#include <sys/vfs.h>
#elif defined( __APPLE__ )
#include <sys/mount.h>
#include <sys/param.h>
#endif


namespace
{


/// The outcome of a call made on the worker; text for the user is composed later, on the caller's thread
struct CALL_RESULT
{
    bool        ok = true;
    DB_FAILURE  failure = DB_FAILURE::UNKNOWN;
    std::string detail; ///< The SQLite message, or what the job found
};


CALL_RESULT failed( DB_FAILURE aFailure, std::string aDetail )
{
    return { false, aFailure, std::move( aDetail ) };
}


/// What a catalog call found
struct CATALOG_RESULT
{
    CALL_RESULT              call;
    std::vector<std::string> names;
    std::string              table;
    bool                     found = false;
    std::vector<DB_COLUMN>   columns;
};


std::filesystem::path pathFromUtf8( const std::string& aPath )
{
    return std::filesystem::path( std::u8string( aPath.begin(), aPath.end() ) );
}


std::string quoted( const std::string& aIdentifier )
{
    std::string result = "\"";

    for( char c : aIdentifier )
    {
        if( c == '"' )
            result += '"';

        result += c;
    }

    return result + "\"";
}


CALL_RESULT sqliteError( sqlite3* aDb, int aCode )
{
    const std::string message = aDb ? sqlite3_errmsg( aDb ) : sqlite3_errstr( aCode );

    // A failed lock request is a conflict with another program, not a lost file
    if( aCode == SQLITE_IOERR_LOCK )
        return failed( DB_FAILURE::FILE_IN_USE, message );

    switch( aCode & 0xFF )
    {
    case SQLITE_BUSY:
    case SQLITE_LOCKED: return failed( DB_FAILURE::FILE_IN_USE, message );
    case SQLITE_IOERR:
    case SQLITE_CANTOPEN: return failed( DB_FAILURE::UNREACHABLE, message );
    default: return failed( DB_FAILURE::UNKNOWN, message );
    }
}


/// Finalizes its statement on every exit
class STATEMENT
{
public:
    STATEMENT( sqlite3* aDb, const std::string& aSql ) :
            m_db( aDb )
    {
        m_code = sqlite3_prepare_v2( aDb, aSql.c_str(), static_cast<int>( aSql.size() ), &m_stmt, nullptr );
    }

    ~STATEMENT() { sqlite3_finalize( m_stmt ); }

    STATEMENT( const STATEMENT& ) = delete;
    STATEMENT& operator=( const STATEMENT& ) = delete;

    bool Ok() const { return m_code == SQLITE_OK; }

    CALL_RESULT Error() const { return sqliteError( m_db, m_code ); }

    sqlite3_stmt* Get() const { return m_stmt; }

    int Step() { return sqlite3_step( m_stmt ); }

    std::string Text( int aColumn ) const
    {
        const unsigned char* text = sqlite3_column_text( m_stmt, aColumn );
        const int            bytes = sqlite3_column_bytes( m_stmt, aColumn );

        return text ? std::string( reinterpret_cast<const char*>( text ), static_cast<size_t>( bytes ) )
                    : std::string();
    }

private:
    sqlite3*      m_db;
    sqlite3_stmt* m_stmt = nullptr;
    int           m_code = SQLITE_OK;
};


/// Names a file independent of the path that reached it
struct FILE_ID
{
    bool     valid = false;
    uint64_t volume = 0;
    uint64_t index = 0;

    bool operator==( const FILE_ID& aOther ) const
    {
        return valid == aOther.valid && volume == aOther.volume && index == aOther.index;
    }
};


FILE_ID fileIdOf( const std::string& aPath )
{
    FILE_ID id;

#ifdef _WIN32
    HANDLE file = CreateFileW( pathFromUtf8( aPath ).c_str(), 0,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                               FILE_FLAG_BACKUP_SEMANTICS, nullptr );

    if( file == INVALID_HANDLE_VALUE )
        return id;

    BY_HANDLE_FILE_INFORMATION info;

    if( GetFileInformationByHandle( file, &info ) )
    {
        id.valid = true;
        id.volume = info.dwVolumeSerialNumber;
        id.index = ( static_cast<uint64_t>( info.nFileIndexHigh ) << 32 ) | info.nFileIndexLow;
    }

    CloseHandle( file );
#else
    struct stat info;

    if( stat( aPath.c_str(), &info ) == 0 )
    {
        id.valid = true;
        id.volume = static_cast<uint64_t>( info.st_dev );
        id.index = static_cast<uint64_t>( info.st_ino );
    }
#endif

    return id;
}


/// @return the format version bytes 18 and 19 hold when they agree (2 is WAL), or 0
int journalFormat( const std::string& aPath )
{
    std::ifstream file( pathFromUtf8( aPath ), std::ios::binary );
    char          header[20] = {};

    if( !file.read( header, sizeof header ) )
        return 0;

    return header[18] == header[19] ? header[18] : 0;
}


// SQLite opens files on Windows without FILE_SHARE_DELETE, so an idle handle stops a sync client from replacing a
// local library. A share is not replaced that way, and reopening it every read upsets other clients' leases
bool closeWhenIdleFor( const std::string& aPath )
{
#ifdef _WIN32
    return !SQLITE_BACKEND::IsNetworkPath( aPath );
#else
    return false;
#endif
}
} // namespace


/// Owns one connection, which a worker thread opens and uses
struct SQLITE_DB
{
    SQLITE_DB( std::string aPath, std::string aUri, int aTimeout ) :
            path( std::move( aPath ) ),
            uri( std::move( aUri ) ),
            timeout( aTimeout )
    {
    }

    // Whichever thread drops the last reference, it is never the caller that does the I/O
    ~SQLITE_DB()
    {
        if( !handle )
            return;

        if( !DB_WORKER_THREAD::OnWorkerThread() )
        {
            try
            {
                std::thread( [h = handle]() { closeHandle( h ); } ).detach();
                handle = nullptr;
                return;
            }
            catch( const std::system_error& )
            {
                // No thread to spare, so the close happens here after all
            }
        }

        Close();
    }

    SQLITE_DB( const SQLITE_DB& ) = delete;
    SQLITE_DB& operator=( const SQLITE_DB& ) = delete;

    void Close()
    {
        if( handle )
            closeHandle( handle );

        handle = nullptr;
    }

    CALL_RESULT Open();

    /// Opens the file and proves it is a database
    CALL_RESULT Connect();

    /// Reopens the connection if the path now names a different file than the one opened
    CALL_RESULT Refresh();

    static void closeHandle( sqlite3* aHandle ) { sqlite3_close_v2( aHandle ); }

    sqlite3*    handle = nullptr;
    std::string path;
    std::string uri;
    int         timeout;
    FILE_ID     id;
    bool        closeWhenIdle = false;
};


namespace
{


/// One read transaction, whose read lock also keeps other programs from switching the file to WAL
class READ_SNAPSHOT
{
public:
    explicit READ_SNAPSHOT( SQLITE_DB& aDb ) :
            m_db( aDb )
    {
    }

    ~READ_SNAPSHOT()
    {
        rollback();

        if( m_db.closeWhenIdle )
            m_db.Close();
    }

    READ_SNAPSHOT( const READ_SNAPSHOT& ) = delete;
    READ_SNAPSHOT& operator=( const READ_SNAPSHOT& ) = delete;

    CALL_RESULT Begin()
    {
        CALL_RESULT result = m_db.Refresh();

        return result.ok ? begin() : result;
    }

    CALL_RESULT Commit()
    {
        const int code = sqlite3_exec( m_db.handle, "COMMIT", nullptr, nullptr, nullptr );

        if( code != SQLITE_OK )
            return sqliteError( m_db.handle, code );

        m_open = false;

        return {};
    }

private:
    void rollback()
    {
        if( m_open && m_db.handle )
            sqlite3_exec( m_db.handle, "ROLLBACK", nullptr, nullptr, nullptr );

        m_open = false;
    }

    // SQLite only learns the file's state once it reads, so step a query before asking the journal mode
    CALL_RESULT begin()
    {
        sqlite3* db = m_db.handle;
        int      code = sqlite3_exec( db, "BEGIN", nullptr, nullptr, nullptr );

        if( code != SQLITE_OK )
            return sqliteError( db, code );

        m_open = true;

        {
            STATEMENT probe( db, "SELECT 1 FROM sqlite_master LIMIT 1" );

            if( !probe.Ok() )
                return probe.Error();

            code = probe.Step();

            if( code != SQLITE_ROW && code != SQLITE_DONE )
                return sqliteError( db, code );
        }

        std::string mode;

        {
            STATEMENT pragma( db, "PRAGMA main.journal_mode" );

            if( !pragma.Ok() )
                return pragma.Error();

            if( pragma.Step() == SQLITE_ROW )
                mode = pragma.Text( 0 );
        }

        std::transform( mode.begin(), mode.end(), mode.begin(),
                        []( unsigned char c )
                        {
                            return static_cast<char>( std::tolower( c ) );
                        } );

        if( mode == "wal" && SQLITE_BACKEND::IsNetworkPath( m_db.path ) )
            return failed( DB_FAILURE::NETWORK_WAL, std::string() );

        return {};
    }

    SQLITE_DB& m_db;
    bool       m_open = false;
};
} // namespace


CALL_RESULT SQLITE_DB::Open()
{
    closeWhenIdle = closeWhenIdleFor( path );

    std::error_code ec;

    if( !std::filesystem::exists( pathFromUtf8( path ), ec ) )
        return failed( DB_FAILURE::FILE_NOT_FOUND, std::string() );

    // A WAL database on a network drive is refused before SQLite can create a wal-index on the share
    if( journalFormat( path ) == 2 && SQLITE_BACKEND::IsNetworkPath( path ) )
        return failed( DB_FAILURE::NETWORK_WAL, std::string() );

    // A rename can land between the open and the stat, so the file is opened again until both agree
    static constexpr int ATTEMPTS = 3;

    for( int attempt = 1;; ++attempt )
    {
        const FILE_ID before = fileIdOf( path );
        const int     code = sqlite3_open_v2( uri.c_str(), &handle, SQLITE_OPEN_READONLY | SQLITE_OPEN_URI, nullptr );

        if( code != SQLITE_OK )
        {
            CALL_RESULT result = sqliteError( handle, code );

            Close();
            return result;
        }

        sqlite3_extended_result_codes( handle, 1 );
        sqlite3_busy_timeout( handle, timeout * 1000 );

        // Recording the identity from before the open lets Refresh see a swap that came later
        id = before;

        const FILE_ID after = fileIdOf( path );

        if( !before.valid || !after.valid || before == after || attempt >= ATTEMPTS )
            return {};

        Close();
    }
}


CALL_RESULT SQLITE_DB::Connect()
{
    CALL_RESULT result = Open();

    if( !result.ok )
        return result;

    // Opening reads nothing, so take a snapshot now to learn whether this is a database at all
    {
        READ_SNAPSHOT snapshot( *this );

        result = snapshot.Begin();

        if( result.ok )
            result = snapshot.Commit();
    }

    if( !result.ok )
        Close();

    return result;
}


CALL_RESULT SQLITE_DB::Refresh()
{
    const FILE_ID now = fileIdOf( path );

    if( handle && ( !now.valid || now == id ) )
        return {};

    Close();

    return Open();
}


namespace
{
/// The ODBC SQL data type nearest to a declared SQLite column type
int typeCode( const std::string& aDeclared )
{
    std::string upper = aDeclared;

    std::transform( upper.begin(), upper.end(), upper.begin(),
                    []( unsigned char c )
                    {
                        return static_cast<char>( std::toupper( c ) );
                    } );

    auto has = [&]( const char* aPart )
               {
                   return upper.find( aPart ) != std::string::npos;
               };

    if( has( "INT" ) )
        return 4; // SQL_INTEGER

    if( has( "CHAR" ) || has( "CLOB" ) || has( "TEXT" ) )
        return 12; // SQL_VARCHAR

    if( has( "BLOB" ) )
        return -3; // SQL_VARBINARY

    if( has( "REAL" ) || has( "FLOA" ) || has( "DOUB" ) )
        return 8; // SQL_DOUBLE

    return 2; // SQL_NUMERIC
}


/// Lists the readable columns of a table, which includes generated ones and leaves out hidden ones
CALL_RESULT readColumns( sqlite3* aDb, const std::string& aTable, std::vector<DB_COLUMN>& aColumns )
{
    STATEMENT info( aDb, "PRAGMA table_xinfo(" + quoted( aTable ) + ")" );

    if( !info.Ok() )
        return info.Error();

    int code;

    while( ( code = info.Step() ) == SQLITE_ROW )
    {
        const int hidden = sqlite3_column_int( info.Get(), 6 );

        // 1 is a virtual table's hidden column, 2 and 3 are generated columns
        if( hidden == 1 )
            continue;

        DB_COLUMN column;

        column.name = info.Text( 1 );
        column.type = typeCode( info.Text( 2 ) );
        column.supported = column.type != -3;
        aColumns.push_back( std::move( column ) );
    }

    if( code != SQLITE_DONE )
        return sqliteError( aDb, code );

    return {};
}


CATALOG_RESULT listTables( SQLITE_DB& aConn )
{
    CATALOG_RESULT result;
    READ_SNAPSHOT  snapshot( aConn );

    result.call = snapshot.Begin();

    if( !result.call.ok )
        return result;

    sqlite3* db = aConn.handle;

    {
        STATEMENT list( db, "SELECT name FROM sqlite_master WHERE type IN ('table', 'view') "
                             "AND name NOT LIKE 'sqlite\\_%' ESCAPE '\\' ORDER BY name" );

        if( !list.Ok() )
        {
            result.call = list.Error();
            return result;
        }

        int code;

        while( ( code = list.Step() ) == SQLITE_ROW )
            result.names.push_back( list.Text( 0 ) );

        if( code != SQLITE_DONE )
        {
            result.call = sqliteError( db, code );
            return result;
        }
    }

    result.call = snapshot.Commit();
    return result;
}


CATALOG_RESULT describeTable( SQLITE_DB& aConn, const std::string& aTable )
{
    CATALOG_RESULT result;
    READ_SNAPSHOT  snapshot( aConn );

    result.call = snapshot.Begin();

    if( !result.call.ok )
        return result;

    sqlite3* db = aConn.handle;

    result.table = aTable;

    // Scoped so the lookup statement is finalized before the snapshot commits
    {
        STATEMENT find( db, "SELECT name FROM sqlite_master WHERE type IN ('table', 'view') "
                             "AND name = ?1 COLLATE NOCASE" );

        if( !find.Ok() )
        {
            result.call = find.Error();
            return result;
        }

        sqlite3_bind_text( find.Get(), 1, aTable.c_str(), static_cast<int>( aTable.size() ), SQLITE_TRANSIENT );

        int code;

        while( ( code = find.Step() ) == SQLITE_ROW )
        {
            const std::string name = find.Text( 0 );

            // The exact spelling wins when names differ only in case
            if( !result.found || name == aTable )
            {
                result.found = true;
                result.table = name;
            }
        }

        if( code != SQLITE_DONE )
        {
            result.call = sqliteError( db, code );
            return result;
        }
    }

    if( result.found )
    {
        result.call = readColumns( db, result.table, result.columns );

        if( !result.call.ok )
            return result;
    }

    result.call = snapshot.Commit();
    return result;
}


DB_VALUE cellValue( sqlite3_stmt* aStmt, int aColumn )
{
    switch( sqlite3_column_type( aStmt, aColumn ) )
    {
    case SQLITE_INTEGER: return static_cast<int64_t>( sqlite3_column_int64( aStmt, aColumn ) );
    case SQLITE_FLOAT: return sqlite3_column_double( aStmt, aColumn );

    case SQLITE_TEXT:
    {
        const unsigned char* text = sqlite3_column_text( aStmt, aColumn );
        const int            bytes = sqlite3_column_bytes( aStmt, aColumn );

        return std::string( reinterpret_cast<const char*>( text ), static_cast<size_t>( bytes ) );
    }

    // There is no way to show raw bytes as a field value
    case SQLITE_BLOB: return std::string();
    default: return std::monostate();
    }
}

} // namespace


std::string SQLITE_BACKEND::FileUri( const std::string& aPath, bool aWindowsPath )
{
    std::string path = DB_WORKER_THREAD::NormalizePath( aPath, aWindowsPath );

    const bool unc = aWindowsPath && path.rfind( "//", 0 ) == 0;
    const bool drive = aWindowsPath && path.size() >= 2 && path[1] == ':'
                       && std::isalpha( static_cast<unsigned char>( path[0] ) );

    // Extra leading slashes name the same root on Unix, and would otherwise read as an authority
    if( !aWindowsPath )
    {
        const size_t first = path.find_first_not_of( '/' );

        if( first != std::string::npos && first > 1 )
            path.erase( 0, first - 1 );
    }

    static const char hex[] = "0123456789ABCDEF";
    std::string       encoded;

    for( size_t i = 0; i < path.size(); ++i )
    {
        const unsigned char c = static_cast<unsigned char>( path[i] );
        const bool          plain = std::isalnum( c ) && c < 0x80;

        if( plain || c == '-' || c == '.' || c == '_' || c == '~' || c == '/' || ( drive && i == 1 && c == ':' ) )
        {
            encoded += static_cast<char>( c );
            continue;
        }

        encoded += '%';
        encoded += hex[c >> 4];
        encoded += hex[c & 0x0F];
    }

    // SQLite takes only an empty authority, so a UNC path keeps its two slashes as the path's own
    if( unc )
        return "file://" + encoded + "?mode=ro";

    return std::string( "file:" ) + ( drive ? "/" : "" ) + encoded + "?mode=ro";
}


bool SQLITE_BACKEND::IsUncPath( const std::string& aPath )
{
    const std::string path = DB_WORKER_THREAD::NormalizePath( aPath, true );

    return path.size() > 2 && path[0] == '/' && path[1] == '/' && path[2] != '/';
}


bool SQLITE_BACKEND::IsRemoteFuseMount( const std::string& aMountInfo, const std::string& aPath )
{
    static const char* remote[] = { "sshfs",    "rclone",
                                    "s3fs",     "gcsfuse",
                                    "smbnetfs", "google-drive-ocamlfuse",
                                    "gvfsd-fuse" };

    auto unescape = []( const std::string& aField )
                    {
                        std::string result;

                        for( size_t i = 0; i < aField.size(); ++i )
                        {
                            if( aField[i] == '\\' && i + 3 < aField.size() && std::isdigit( aField[i + 1] ) )
                            {
                                result += static_cast<char>( std::stoi( aField.substr( i + 1, 3 ), nullptr, 8 ) );
                                i += 3;
                            }
                            else
                            {
                                result += aField[i];
                            }
                        }

                        return result;
                    };

    size_t      bestLength = 0;
    bool        found = false;
    std::string bestType;
    size_t      lineStart = 0;

    while( lineStart < aMountInfo.size() )
    {
        size_t lineEnd = aMountInfo.find( '\n', lineStart );

        if( lineEnd == std::string::npos )
            lineEnd = aMountInfo.size();

        const std::string line = aMountInfo.substr( lineStart, lineEnd - lineStart );

        lineStart = lineEnd + 1;

        const size_t separator = line.find( " - " );

        if( separator == std::string::npos )
            continue;

        std::vector<std::string> head;

        for( size_t pos = 0; pos < separator; )
        {
            size_t end = line.find( ' ', pos );

            if( end == std::string::npos || end > separator )
                end = separator;

            head.push_back( line.substr( pos, end - pos ) );
            pos = end + 1;
        }

        if( head.size() < 5 )
            continue;

        const std::string mount = unescape( head[4] );
        const size_t      typeEnd = line.find( ' ', separator + 3 );
        const std::string type = line.substr( separator + 3, typeEnd == std::string::npos ? std::string::npos
                                                                                          : typeEnd - separator - 3 );

        const bool inside = mount == "/" || aPath == mount
                            || ( aPath.size() > mount.size() && aPath.compare( 0, mount.size(), mount ) == 0
                                 && aPath[mount.size()] == '/' );

        if( inside && mount.size() >= bestLength )
        {
            bestLength = mount.size();
            bestType = type;
            found = true;
        }
    }

    if( !found || bestType.rfind( "fuse.", 0 ) != 0 )
        return false;

    const std::string subtype = bestType.substr( 5 );

    return std::any_of( std::begin( remote ), std::end( remote ),
                        [&]( const char* aName )
                        {
                            return subtype == aName;
                        } );
}


#if defined( _WIN32 )
namespace
{
/// Follows junctions and symlinks to the real location and drops the extended prefix, except for UNC paths
std::string resolvedWindowsPath( const std::string& aPath )
{
    const std::wstring wide = wxString::FromUTF8( aPath ).ToStdWstring();
    std::wstring       resolved = wide;
    std::wstring       opened = wide;

    if( DWORD size = GetFullPathNameW( wide.c_str(), 0, nullptr, nullptr ); size > 0 )
    {
        std::vector<wchar_t> full( size );
        DWORD                length = GetFullPathNameW( wide.c_str(), size, full.data(), nullptr );

        if( length > 0 && length < size )
            opened.assign( full.data(), length );
    }

    // Past MAX_PATH the open needs the extended prefix unless the system enables long paths
    if( opened.size() > 2 && opened[1] == L':' && opened[2] == L'\\' )
        opened = L"\\\\?\\" + opened;

    HANDLE handle = CreateFileW( opened.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                 OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr );

    if( handle != INVALID_HANDLE_VALUE )
    {
        std::vector<wchar_t> buffer( MAX_PATH + 1 );
        DWORD length = GetFinalPathNameByHandleW( handle, buffer.data(), static_cast<DWORD>( buffer.size() ),
                                                  VOLUME_NAME_DOS );

        if( length >= buffer.size() )
        {
            buffer.resize( length + 1 );
            length = GetFinalPathNameByHandleW( handle, buffer.data(), static_cast<DWORD>( buffer.size() ),
                                                VOLUME_NAME_DOS );
        }

        if( length > 0 && length < buffer.size() )
            resolved.assign( buffer.data(), length );

        CloseHandle( handle );
    }

    static const std::wstring extended = L"\\\\?\\";
    static const std::wstring extendedUnc = L"\\\\?\\UNC\\";

    // The prefix means a UNC path only when UNC follows it
    if( resolved.compare( 0, extendedUnc.size(), extendedUnc ) == 0 )
        resolved = L"\\\\" + resolved.substr( extendedUnc.size() );
    else if( resolved.compare( 0, extended.size(), extended ) == 0 )
        resolved = resolved.substr( extended.size() );

    return wxString( resolved ).ToUTF8().data();
}
} // namespace
#endif


bool SQLITE_BACKEND::IsNetworkPath( const std::string& aPath )
{
#if defined( _WIN32 )
    if( IsUncPath( aPath ) )
        return true;

    // A junction or symlink may lead from a local folder to a share, so the target is classified
    const std::string target = resolvedWindowsPath( aPath );

    if( IsUncPath( target ) )
        return true;

    const std::wstring   wide = wxString::FromUTF8( target ).ToStdWstring();
    std::vector<wchar_t> root( std::max<size_t>( wide.size(), MAX_PATH ) + 1, L'\0' );

    if( !GetVolumePathNameW( wide.c_str(), root.data(), static_cast<DWORD>( root.size() ) ) )
        return false;

    return GetDriveTypeW( root.data() ) == DRIVE_REMOTE;
#elif defined( __linux__ )
    struct statfs info;

    if( statfs( aPath.c_str(), &info ) != 0 )
        return false;

    // Magic numbers from linux/magic.h, written out so the build does not depend on that header
    switch( static_cast<uint32_t>( info.f_type ) )
    {
    case 0x6969:     // NFS
    case 0x517B:     // SMB
    case 0xFF534D42: // CIFS
    case 0xFE534D42: // SMB2
    case 0x73757245: // Coda
    case 0x5346414F: // AFS
    case 0x564C:     // NCP
    case 0x01021997: // 9p
    case 0x00C36400: // Ceph
    case 0x0BD00BD0: // Lustre
    case 0x01161970: // GFS2
    case 0x7461636F: // OCFS2
        return true;
    case 0x65735546: // FUSE, which is network only for the remote subtypes
    {
        std::error_code ec;
        std::string     real = std::filesystem::weakly_canonical( pathFromUtf8( aPath ), ec ).string();
        std::ifstream   mounts( "/proc/self/mountinfo" );
        std::string     text( ( std::istreambuf_iterator<char>( mounts ) ), std::istreambuf_iterator<char>() );

        return IsRemoteFuseMount( text, ec ? aPath : real );
    }

    default: return false;
    }
#elif defined( __APPLE__ )
    struct statfs info;

    if( statfs( aPath.c_str(), &info ) != 0 )
        return false;

    return ( info.f_flags & MNT_LOCAL ) == 0;
#else
    return false;
#endif
}


std::chrono::milliseconds SQLITE_BACKEND::callerBound() const
{
    return DB_WORKER_THREAD::CallerBound( m_timeout );
}


SQLITE_BACKEND::SQLITE_BACKEND( std::string aPath, int aTimeoutSeconds ) :
        m_path( std::move( aPath ) ),
        m_timeout( aTimeoutSeconds ),
        m_worker( std::make_unique<DB_WORKER_THREAD>( DB_WORKER_THREAD::SourceKeyFor( m_path ) ) )
{
    m_worker->SetShutdownBound( DB_WORKER_THREAD::CallerBound( m_timeout ) );
}


SQLITE_BACKEND::~SQLITE_BACKEND()
{
    Disconnect();
}


void SQLITE_BACKEND::fail( DB_FAILURE aFailure, std::string aError )
{
    m_lastFailure = aFailure;
    m_lastError = std::move( aError );

    wxLogTrace( traceDatabase, wxT( "SQLITE_BACKEND: %s" ), wxString::FromUTF8( m_lastError ) );
}


/// Composes the message for a failed call, naming the file
static std::string describeFailure( const std::string& aPath, const CALL_RESULT& aResult )
{
    const wxString path = wxString::FromUTF8( aPath );

    switch( aResult.failure )
    {
    case DB_FAILURE::FILE_NOT_FOUND:
        return utf8( wxString::Format( _( "database file %s was not found" ), path ) );

    case DB_FAILURE::FILE_IN_USE:
        return utf8( wxString::Format( _( "database file %s is locked by another program (%s)" ), path,
                                       wxString::FromUTF8( aResult.detail ) ) );

    case DB_FAILURE::NETWORK_WAL:
        return utf8( wxString::Format( _( "database file %s uses write-ahead logging on a network drive, which "
                                          "SQLite cannot read safely; set journal_mode=DELETE on the database" ),
                                       path ) );

    case DB_FAILURE::UNREACHABLE:
        return utf8( wxString::Format( _( "database file %s could not be reached (%s)" ), path,
                                       wxString::FromUTF8( aResult.detail ) ) );

    default:
        return utf8( wxString::Format( _( "database file %s could not be read (%s)" ), path,
                                       wxString::FromUTF8( aResult.detail ) ) );
    }
}


/// A failure after which the connection can no longer be trusted
static bool invalidates( DB_FAILURE aFailure )
{
    return aFailure == DB_FAILURE::NETWORK_WAL || aFailure == DB_FAILURE::UNREACHABLE
           || aFailure == DB_FAILURE::FILE_NOT_FOUND;
}


template <typename R>
std::optional<R> SQLITE_BACKEND::runOnWorker( std::function<R( DB_JOB_CONTEXT& )> aJob, const DB_ROW_SINK* aSink )
{
    std::optional<R> result = m_worker->Run<R>( std::move( aJob ), callerBound(), aSink );

    if( result )
        return result;

    const DB_WORKER_FAILURE failure = DescribeWorkerOutcome( m_worker->LastOutcome(), m_path );

    if( failure.connection == DB_WORKER_FAILURE::CONNECTION::DROP )
    {
        dropConnection();
    }
    else if( failure.connection == DB_WORKER_FAILURE::CONNECTION::FORGET )
    {
        // The abandoned job holds the last reference and closes the connection when the blocked call returns
        m_connected = false;
        m_db.reset();
    }

    fail( failure.failure, failure.message );

    return std::nullopt;
}


void SQLITE_BACKEND::failCall( DB_FAILURE aFailure, std::string aMessage )
{
    fail( aFailure, std::move( aMessage ) );

    if( invalidates( aFailure ) )
        dropConnection();
}


bool SQLITE_BACKEND::requireConnection()
{
    if( m_db )
        return true;

    fail( DB_FAILURE::UNKNOWN, utf8( _( "not connected to a database" ) ) );
    return false;
}


void SQLITE_BACKEND::dropConnection()
{
    std::shared_ptr<SQLITE_DB> db = std::move( m_db );

    m_connected = false;

    if( !db )
        return;

    // Copies of the closure may outlive the call, so the close is explicit rather than left to the last reference
    m_worker->RunCleanup(
            [db]()
            {
                db->Close();
            },
            callerBound() );
}


bool SQLITE_BACKEND::Connect()
{
    m_lastError.clear();
    m_lastFailure = DB_FAILURE::UNKNOWN;

    if( m_connected )
        return true;

    if( m_path.empty() )
    {
        fail( DB_FAILURE::FILE_NOT_FOUND, utf8( _( "no database file is set" ) ) );
        return false;
    }

#ifdef _WIN32
    const bool windows = true;
#else
    const bool windows = false;
#endif

    auto db = std::make_shared<SQLITE_DB>( m_path, FileUri( m_path, windows ), m_timeout );

    std::optional<CALL_RESULT> result = runOnWorker<CALL_RESULT>(
            [db]( DB_JOB_CONTEXT& )
            {
                return db->Connect();
            },
            nullptr );

    if( !result )
        return false;

    if( !result->ok )
    {
        fail( result->failure, describeFailure( m_path, *result ) );
        return false;
    }

    m_db = db;
    m_connected = true;
    return true;
}


void SQLITE_BACKEND::Disconnect()
{
    dropConnection();
}


bool SQLITE_BACKEND::ListTables( std::vector<std::string>& aTables )
{
    aTables.clear();
    m_lastError.clear();

    if( !requireConnection() )
        return false;

    std::shared_ptr<SQLITE_DB> db = m_db;

    std::optional<CATALOG_RESULT> result = runOnWorker<CATALOG_RESULT>(
            [db]( DB_JOB_CONTEXT& )
            {
                return listTables( *db );
            },
            nullptr );

    if( !result )
        return false;

    if( !result->call.ok )
    {
        failCall( result->call.failure, describeFailure( m_path, result->call ) );

        return false;
    }

    aTables = std::move( result->names );
    std::sort( aTables.begin(), aTables.end() );
    return true;
}


std::optional<DB_TABLE_INFO> SQLITE_BACKEND::DescribeTable( const std::string& aTable )
{
    // A table that is simply absent leaves no error, so nothing of an earlier failure may remain
    m_lastError.clear();
    m_lastFailure = DB_FAILURE::UNKNOWN;

    if( !requireConnection() )
        return std::nullopt;

    std::shared_ptr<SQLITE_DB> db = m_db;

    std::optional<CATALOG_RESULT> result = runOnWorker<CATALOG_RESULT>(
            [db, aTable]( DB_JOB_CONTEXT& )
            {
                return describeTable( *db, aTable );
            },
            nullptr );

    if( !result )
        return std::nullopt;

    if( !result->call.ok )
    {
        failCall( result->call.failure, describeFailure( m_path, result->call ) );

        return std::nullopt;
    }

    if( !result->found )
        return std::nullopt;

    DB_TABLE_INFO info;

    info.name = std::move( result->table );
    info.columns = std::move( result->columns );

    return info;
}


bool SQLITE_BACKEND::ReadTable( const DB_TABLE_INFO& aTable, const std::vector<std::string>& aColumns,
                                size_t aMaxRows, std::vector<std::string>& aLabels, const DB_ROW_SINK& aSink )
{
    aLabels.clear();
    m_lastError.clear();

    if( !requireConnection() )
        return false;

    std::string sql = "SELECT ";

    if( aColumns.empty() )
    {
        sql += "*";
    }
    else
    {
        for( size_t i = 0; i < aColumns.size(); ++i )
            sql += ( i ? ", " : "" ) + quoted( aColumns[i] );
    }

    sql += " FROM " + quoted( aTable.name );

    if( aMaxRows != ALL_ROWS )
        sql += " ORDER BY 1 LIMIT " + std::to_string( aMaxRows );

    DB_READ_DELIVERY           delivery( aLabels, aSink );
    auto                       labels = delivery.Labels();
    std::shared_ptr<SQLITE_DB> db = m_db;
    const bool                 fetchRows = aMaxRows != 0;

    std::optional<CALL_RESULT> result = runOnWorker<CALL_RESULT>(
            [db, sql, labels, fetchRows]( DB_JOB_CONTEXT& aContext ) -> CALL_RESULT
            {
                READ_SNAPSHOT snapshot( *db );
                CALL_RESULT   began = snapshot.Begin();

                if( !began.ok )
                    return began;

                {
                    STATEMENT query( db->handle, sql );

                    if( !query.Ok() )
                        return query.Error();

                    const int count = sqlite3_column_count( query.Get() );

                    for( int i = 0; i < count; ++i )
                        labels->emplace_back( sqlite3_column_name( query.Get(), i ) );

                    int code = SQLITE_ROW;

                    while( fetchRows && ( code = query.Step() ) == SQLITE_ROW )
                    {
                        std::vector<DB_VALUE> row;
                        row.reserve( static_cast<size_t>( count ) );

                        for( int i = 0; i < count; ++i )
                            row.push_back( cellValue( query.Get(), i ) );

                        if( !aContext.Emit( std::move( row ) ) )
                            return failed( DB_FAILURE::UNKNOWN, "the rows were not accepted" );
                    }

                    if( fetchRows && code != SQLITE_DONE )
                        return sqliteError( db->handle, code );
                }

                if( !aContext.Flush() )
                    return failed( DB_FAILURE::UNKNOWN, "the rows were not accepted" );

                return snapshot.Commit();
            },
            delivery.Sink() );

    if( !result )
        return false;

    delivery.Finish();

    if( !result->ok )
    {
        aLabels.clear();

        if( delivery.SinkThrew() )
            fail( DB_FAILURE::UNKNOWN, DB_READ_DELIVERY::SinkFailureMessage() );
        else
            fail( result->failure, describeFailure( m_path, *result ) );

        if( invalidates( result->failure ) )
            dropConnection();

        return false;
    }

    return true;
}
