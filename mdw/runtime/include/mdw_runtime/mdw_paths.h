/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 KiCad Developers, see AUTHORS.txt for contributors.
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
/**
 * @file mdw_paths.h
 * @brief UTF-8 conversion for the filesystem paths a backend receives.
 *
 * Every path on the wire is UTF-8. Windows reads a narrow std::string in the active code page, so a path outside
 * it would name a different file.
 */

#ifndef MDW_PATHS_H
#define MDW_PATHS_H

#include <algorithm>
#include <filesystem>
#include <iterator>
#include <string>

/// The filesystem path a UTF-8 wire path names, on every platform
inline std::filesystem::path MdwFsPath( const std::string& aUtf8 )
{
    return std::filesystem::path( std::u8string( aUtf8.begin(), aUtf8.end() ) );
}


inline std::string MdwUtf8Path( const std::filesystem::path& aPath )
{
    std::u8string utf8 = aPath.u8string();

    return std::string( utf8.begin(), utf8.end() );
}


/// True when aPath is a clean repository-relative path with no '.', '..' or empty parts
inline bool MdwIsSafeRelativePath( const std::string& aPath )
{
    if( aPath.empty() || aPath.front() == '/' || aPath.find_first_of( std::string( "\\:\0", 3 ) ) != std::string::npos )
        return false;

    size_t start = 0;

    while( start <= aPath.size() )
    {
        size_t      end = aPath.find( '/', start );
        std::string part = aPath.substr( start, end == std::string::npos ? std::string::npos : end - start );

        if( part.empty() || part == "." || part == ".." )
            return false;

        if( end == std::string::npos )
            break;

        start = end + 1;
    }

    return true;
}


/**
 * True when aPath passes MdwIsSafeRelativePath and names nothing inside a ".git" folder. The name is matched as
 * Windows and macOS open it, so case, trailing dots and spaces, and the short name GIT~1 do not slip past.
 */
inline bool MdwIsSafeRepoPath( const std::string& aPath )
{
    if( !MdwIsSafeRelativePath( aPath ) )
        return false;

    for( size_t start = 0; start < aPath.size(); )
    {
        size_t      end = std::min( aPath.find( '/', start ), aPath.size() );
        std::string part = aPath.substr( start, end - start );

        while( !part.empty() && ( part.back() == '.' || part.back() == ' ' ) )
            part.pop_back();

        for( char& c : part )
        {
            if( c >= 'A' && c <= 'Z' )
                c = static_cast<char>( c - 'A' + 'a' );
        }

        if( part == ".git" || part == "git~1" )
            return false;

        start = end + 1;
    }

    return true;
}


/**
 * aRoot / aRelative when aRelative passes MdwIsSafeRepoPath, every folder on the way is a real folder and the
 * file is a regular file, so a planted link or junction cannot redirect a read or write out of aRoot. Empty
 * otherwise. With aAllowMissing a part that does not exist yet ends the walk, for a file about to be written.
 */
inline std::filesystem::path MdwLinkFreePath( const std::filesystem::path& aRoot, const std::string& aRelative,
                                              bool aAllowMissing )
{
    namespace fs = std::filesystem;

    if( aRoot.empty() || !MdwIsSafeRepoPath( aRelative ) )
        return {};

    const fs::path relative = MdwFsPath( aRelative );
    fs::path       current = aRoot;

    for( auto it = relative.begin(); it != relative.end(); ++it )
    {
        std::error_code ec;

        current /= *it;

        const fs::file_type type = fs::symlink_status( current, ec ).type();

        // libstdc++ also sets ec for a missing path, and everything below a missing part is missing too
        if( type == fs::file_type::not_found )
            return aAllowMissing ? aRoot / relative : fs::path();

        // symlink_status never follows, and a junction reports its own type, so both are refused
        if( ec || type != ( std::next( it ) == relative.end() ? fs::file_type::regular : fs::file_type::directory ) )
            return {};
    }

    return current;
}


/**
 * True when both name the same local root. They compare with '/' separators, no "." segments, repeated or trailing
 * separators, and ASCII case folded on Windows and macOS, whose default filesystems ignore case. ".." is kept,
 * since resolving it without the filesystem can name a different directory through a symlink. An empty root names
 * nothing, so it matches nothing.
 */
inline bool MdwSameRoot( const std::string& aUtf8, const std::string& aOtherUtf8 )
{
    auto key = []( const std::string& aRoot )
    {
        const std::filesystem::path path = MdwFsPath( aRoot );
        std::filesystem::path       normal = path.root_path();

        for( const std::filesystem::path& part : path.relative_path() )
        {
            if( !part.empty() && part != "." )
                normal /= part;
        }

        const std::u8string generic = normal.generic_u8string();
        std::string         folded( generic.begin(), generic.end() );

#if defined( _WIN32 ) || defined( __APPLE__ )
        for( char& c : folded )
        {
            if( c >= 'A' && c <= 'Z' )
                c = static_cast<char>( c - 'A' + 'a' );
        }
#endif

        return folded;
    };

    return !aUtf8.empty() && !aOtherUtf8.empty() && key( aUtf8 ) == key( aOtherUtf8 );
}

#endif // MDW_PATHS_H
