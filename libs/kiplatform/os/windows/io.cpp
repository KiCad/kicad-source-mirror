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
* with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <kiplatform/io.h>

#include <wx/string.h>
#include <wx/wxcrt.h>
#include <wx/filename.h>

#include <string>
#include <windows.h>
#include <shlwapi.h>
#include <winternl.h>

// NtQueryDirectoryFile-based directory enumeration for fast file listing.
// This approach is based on git-for-windows fscache implementation:
// https://github.com/git-for-windows/git/blob/main/compat/win32/fscache.c
// Copyright (C) Johannes Schindelin and the Git for Windows project
// Licensed under GPL v2.
//
// FILE_FULL_DIR_INFORMATION is documented in the Windows Driver Kit but not the SDK.
typedef struct _FILE_FULL_DIR_INFORMATION
{
    ULONG         NextEntryOffset;
    ULONG         FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG         FileAttributes;
    ULONG         FileNameLength;
    ULONG         EaSize;
    WCHAR         FileName[1];
} FILE_FULL_DIR_INFORMATION, *PFILE_FULL_DIR_INFORMATION;

typedef NTSTATUS( NTAPI* PFN_NtQueryDirectoryFile )( HANDLE, HANDLE, PIO_APC_ROUTINE, PVOID,
                                                     PIO_STATUS_BLOCK, PVOID, ULONG,
                                                     FILE_INFORMATION_CLASS, BOOLEAN,
                                                     PUNICODE_STRING, BOOLEAN );

#define FileFullDirectoryInformation ( (FILE_INFORMATION_CLASS) 2 )

// Define USE_MSYS2_FALlBACK if the code for _MSC_VER does not compile on msys2
//#define  USE_MSYS2_FALLBACK

FILE* KIPLATFORM::IO::SeqFOpen( const wxString& aPath, const wxString& aMode )
{
#if defined( _MSC_VER ) || !defined( USE_MSYS2_FALLBACK )
    // We need to use the win32 api to setup a file handle with sequential scan flagged
    // and pass it up the chain to create a normal FILE stream
    HANDLE hFile = INVALID_HANDLE_VALUE;
    hFile = CreateFileW( aPath.wc_str(),
                         GENERIC_READ,
                         FILE_SHARE_READ,
                         NULL,
                         OPEN_EXISTING,
                         FILE_FLAG_SEQUENTIAL_SCAN,
                         NULL );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        return NULL;
    }

    int fd = _open_osfhandle( reinterpret_cast<intptr_t>( hFile ), 0 );

    if( fd == -1 )
    {
        // close the handle manually as the ownership didnt transfer
        CloseHandle( hFile );
        return NULL;
    }

    FILE* fp = _fdopen( fd, aMode.c_str() );

    if( !fp )
    {
        // close the file descriptor manually as the ownership didnt transfer
        _close( fd );
    }

    return fp;
#else
    // Fallback for MSYS2
    return wxFopen( aPath, aMode );
#endif
}

bool KIPLATFORM::IO::DuplicatePermissions( const wxString &aSrc, const wxString &aDest )
{
    bool retval = false;
    DWORD dwSize = 0;

    // Retrieve the security descriptor from the source file
    if( GetFileSecurity( aSrc.wc_str(),
            OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
            NULL, 0, &dwSize ) )
    {
        #ifdef __MINGW32__
        // pSD is used as PSECURITY_DESCRIPTOR, aka void* pointer
        // it create an annoying warning on gcc with "delete[] pSD;" :
        // "warning: deleting 'PSECURITY_DESCRIPTOR' {aka 'void*'} is undefined"
        // so use a BYTE* pointer (do not cast it to a void pointer)
        BYTE* pSD = new BYTE[dwSize];
        #else
        PSECURITY_DESCRIPTOR pSD = static_cast<PSECURITY_DESCRIPTOR>( new BYTE[dwSize] );
        #endif

        if( !pSD )
            return false;

        if( !GetFileSecurity( aSrc.wc_str(),
                OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION
                        | DACL_SECURITY_INFORMATION, pSD, dwSize, &dwSize ) )
        {
            delete[] pSD;
            return false;
        }

        // Assign the retrieved security descriptor to the destination file
        if( !SetFileSecurity( aDest.wc_str(),
                OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION
                        | DACL_SECURITY_INFORMATION, pSD ) )
        {
            retval = false;
        }

        delete[] pSD;
    }

    return retval;
}

bool KIPLATFORM::IO::MakeWriteable( const wxString& aFilePath )
{
    DWORD attrs = GetFileAttributesW( aFilePath.wc_str() );

    if( attrs == INVALID_FILE_ATTRIBUTES )
        return false;

    // Remove read-only and hidden attributes if present. Both of these can prevent file
    // operations on Windows. Hidden files in particular can cause issues when files are
    // synced via cloud services like OneDrive.
    DWORD attrsToRemove = FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN;

    if( attrs & attrsToRemove )
    {
        attrs &= ~attrsToRemove;
        return SetFileAttributesW( aFilePath.wc_str(), attrs ) != 0;
    }

    return true;
}

bool KIPLATFORM::IO::IsFileHidden( const wxString& aFileName )
{
    bool result = false;

    if( ( GetFileAttributesW( aFileName.fn_str() ) & FILE_ATTRIBUTE_HIDDEN ) )
        result = true;

    return result;
}


void KIPLATFORM::IO::LongPathAdjustment( wxFileName& aFilename )
{
    // dont shortcut this for shorter lengths as there are uses like directory
    // paths that exceed the path length when you start traversing their subdirectories
    // so we want to start with the long path prefix all the time

    if( aFilename.GetVolume().Length() == 1 )
        // assume single letter == drive volume
        aFilename.SetVolume( "\\\\?\\" + aFilename.GetVolume() + ":" );
    else if( aFilename.GetVolume().Length() > 1
            && aFilename.GetVolume().StartsWith( wxT( "\\\\" ) )
            && !aFilename.GetVolume().StartsWith( wxT( "\\\\?" ) ) )
        // unc path aka network share, wx returns with \\ already
        // so skip the first slash and combine with the prefix
        // which in the case of UNCs is actually \\?\UNC\<server>\<share>
        // where UNC is literally the text UNC
        aFilename.SetVolume( "\\\\?\\UNC" + aFilename.GetVolume().Mid( 1 ) );
    else if( aFilename.GetVolume().StartsWith( wxT( "\\\\?" ) )
             && aFilename.GetDirs().size() >= 2
             && aFilename.GetDirs()[0] == "UNC" )
    {
        // wxWidgets can parse \\?\UNC\<server> into a mess
        // UNC gets stored into a directory
        // volume gets reduced to just \\?
        // so we need to repair it
        aFilename.SetVolume( "\\\\?\\UNC\\" + aFilename.GetDirs()[1] );
        aFilename.RemoveDir( 0 );
        aFilename.RemoveDir( 0 );
    }
}


long long KIPLATFORM::IO::TimestampDir( const wxString& aDirPath, const wxString& aFilespec )
{
    long long timestamp = 0;

    // Use NtQueryDirectoryFile for fast directory enumeration (same approach as git-for-windows).
    // This retrieves multiple directory entries per syscall into a large buffer, reducing
    // kernel transitions compared to FindFirstFile/FindNextFile.
    static PFN_NtQueryDirectoryFile pNtQueryDirectoryFile = nullptr;

    if( !pNtQueryDirectoryFile )
    {
        HMODULE ntdll = GetModuleHandleW( L"ntdll.dll" );

        if( ntdll )
        {
            pNtQueryDirectoryFile =
                    (PFN_NtQueryDirectoryFile) GetProcAddress( ntdll, "NtQueryDirectoryFile" );
        }
    }

    if( !pNtQueryDirectoryFile )
        return timestamp;

    std::wstring dirPath( aDirPath.t_str() );

    if( !dirPath.empty() && dirPath.back() != L'\\' )
        dirPath += L'\\';

    // Prefix with \\?\ for long path support, handling UNC paths specially
    std::wstring ntPath;

    if( dirPath.size() >= 2 && dirPath[0] == L'\\' && dirPath[1] == L'\\' )
    {
        if( dirPath.size() >= 4 && dirPath[2] == L'?' && dirPath[3] == L'\\' )
        {
            // Already has \\?\ prefix
            ntPath = dirPath;
        }
        else
        {
            // UNC path: \\server\share -> \\?\UNC\server\share
            ntPath = L"\\\\?\\UNC\\" + dirPath.substr( 2 );
        }
    }
    else
    {
        // Local path: C:\foo -> \\?\C:\foo
        ntPath = L"\\\\?\\" + dirPath;
    }

    HANDLE hDir = CreateFileW( ntPath.c_str(), FILE_LIST_DIRECTORY,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                               OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr );

    if( hDir == INVALID_HANDLE_VALUE )
        return timestamp;

    std::wstring pattern( aFilespec.t_str() );

    // 64KB buffer for directory entries (same size as git-for-windows)
    alignas( sizeof( LONGLONG ) ) char buffer[64 * 1024];

    IO_STATUS_BLOCK iosb;
    NTSTATUS        status;
    bool            firstQuery = true;

    for( ;; )
    {
        status = pNtQueryDirectoryFile( hDir, nullptr, nullptr, nullptr, &iosb, buffer,
                                        sizeof( buffer ), FileFullDirectoryInformation, FALSE,
                                        nullptr, firstQuery ? TRUE : FALSE );
        firstQuery = false;

        if( status != 0 )
            break;

        PFILE_FULL_DIR_INFORMATION dirInfo = (PFILE_FULL_DIR_INFORMATION) buffer;

        for( ;; )
        {
            // Extract null-terminated filename
            std::wstring fileName( dirInfo->FileName, dirInfo->FileNameLength / sizeof( WCHAR ) );

            // Skip directories and match against pattern
            if( !( dirInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY )
                && PathMatchSpecW( fileName.c_str(), pattern.c_str() ) )
            {
                // Shift right by 13 (~0.8ms resolution) to avoid overflow when summing many files
                timestamp += dirInfo->LastWriteTime.QuadPart >> 13;
                timestamp += dirInfo->EndOfFile.LowPart;
            }

            if( dirInfo->NextEntryOffset == 0 )
                break;

            dirInfo = (PFILE_FULL_DIR_INFORMATION) ( (char*) dirInfo + dirInfo->NextEntryOffset );
        }
    }

    CloseHandle( hDir );

    return timestamp;
}