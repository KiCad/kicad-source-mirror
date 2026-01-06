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
#include <windows.h>

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