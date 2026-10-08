/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <filesystem>
#include <system_error>
#include <vector>
#include <wx/filename.h>
#include <wx/intl.h>
#include <downgrade_scan.h>

inline std::filesystem::path DowngradeNativePath( const wxString& aPath )
{
#ifdef _WIN32
    return std::filesystem::path( aPath.ToStdWstring() );
#else
    return std::filesystem::path( aPath.utf8_string() );
#endif
}

inline wxString DowngradeWxPath( const std::filesystem::path& aPath )
{
#ifdef _WIN32
    return wxString( aPath.wstring() );
#else
    return wxString::FromUTF8( aPath.string() );
#endif
}

inline bool ValidateFreshDowngradeOutput( const wxString& aOutput, wxString& aError )
{
    if( aOutput.IsEmpty() )
    {
        aError = _( "An output path is required. Use --output." );
        return false;
    }

    wxFileName output( aOutput );

    // Match the normalization used by transaction destinations before testing freshness.
    if( !output.MakeAbsolute() )
    {
        aError = _( "The output path is not accessible." );
        return false;
    }

    std::error_code error;
    auto            status = std::filesystem::symlink_status( DowngradeNativePath( output.GetFullPath() ), error );

    if( ( error && error != std::errc::no_such_file_or_directory )
        || status.type() != std::filesystem::file_type::not_found )
    {
        aError = _( "The output path already exists or is inaccessible. Choose a new destination." );
        return false;
    }

    return true;
}

inline bool IsDowngradeLibraryWorkingArtifact( const wxString& aPath )
{
    wxFileName file( aPath );
    return IsDowngradeWorkingArtifact( aPath ) || file.GetExt() == wxT( "lck" ) || file.GetExt() == wxT( "kicad_prl" )
           || file.GetFullName().Contains( wxT( ".downgrade_tmp." ) )
           || file.GetFullName().Contains( wxT( ".downgrade_backup." ) );
}

inline bool ValidateDowngradeLibrarySource( const wxString& aSource, wxString& aError )
{
    std::error_code error;
    auto            source = std::filesystem::canonical( DowngradeNativePath( aSource ), error );

    if( error || !std::filesystem::is_directory( source, error ) || error )
    {
        aError = _( "The library folder is not accessible." );
        return false;
    }

    std::filesystem::recursive_directory_iterator       iterator( source, error );
    const std::filesystem::recursive_directory_iterator end;

    while( !error && iterator != end )
    {
        const auto& entry = *iterator;
        wxString    path = DowngradeWxPath( entry.path() );

        if( IsDowngradeLibraryWorkingArtifact( path ) )
        {
            iterator.disable_recursion_pending();
        }
        else if( entry.is_symlink( error ) && entry.is_directory( error ) )
        {
            aError = _( "Library folders containing directory links cannot be copied safely." );
            return false;
        }
        else if( !error && !entry.is_directory( error ) && !entry.is_regular_file( error ) )
        {
            aError = _( "The library folder contains an inaccessible or unsupported file." );
            return false;
        }

        if( !error )
            iterator.increment( error );
    }

    if( error )
    {
        aError = _( "The library folder could not be read completely." );
        return false;
    }

    return true;
}

inline bool ValidateDowngradeLibraryCopy( const wxString& aSource, const wxString& aDestination, wxString& aError )
{
    if( !ValidateFreshDowngradeOutput( aDestination, aError ) || !ValidateDowngradeLibrarySource( aSource, aError ) )
    {
        return false;
    }

    std::error_code error;
    auto            source = std::filesystem::canonical( DowngradeNativePath( aSource ), error );
    auto            destination = std::filesystem::weakly_canonical( DowngradeNativePath( aDestination ), error );

    if( error )
    {
        aError = _( "The output path is not accessible." );
        return false;
    }

    auto relative = destination.lexically_relative( source );

    if( destination == source || ( !relative.empty() && *relative.begin() != ".." ) )
    {
        aError = _( "The library output must be outside the source library folder." );
        return false;
    }

    return true;
}

class DOWNGRADE_OUTPUT_DIRECTORIES
{
public:
    DOWNGRADE_OUTPUT_DIRECTORIES() = default;
    DOWNGRADE_OUTPUT_DIRECTORIES( const DOWNGRADE_OUTPUT_DIRECTORIES& ) = delete;
    DOWNGRADE_OUTPUT_DIRECTORIES& operator=( const DOWNGRADE_OUTPUT_DIRECTORIES& ) = delete;

    ~DOWNGRADE_OUTPUT_DIRECTORIES()
    {
        if( m_committed )
            return;

        for( auto it = m_created.rbegin(); it != m_created.rend(); ++it )
        {
            std::error_code error;

            if( it->second )
                std::filesystem::remove_all( it->first, error );
            else
                std::filesystem::remove( it->first, error );
        }
    }

    bool Create( const wxString& aDirectory, bool aOwnContents = false )
    {
        std::error_code error;
        auto directory = std::filesystem::absolute( DowngradeNativePath( aDirectory ), error ).lexically_normal();

        if( error )
            return false;

        std::vector<std::filesystem::path> missing;

        for( auto path = directory; !std::filesystem::exists( path, error ); path = path.parent_path() )
        {
            if( error || path.empty() || path == path.parent_path() )
                return false;

            missing.push_back( path );
        }

        if( error || ( aOwnContents && missing.empty() ) )
            return false;

        for( auto it = missing.rbegin(); it != missing.rend(); ++it )
        {
            if( !std::filesystem::create_directory( *it, error ) || error )
                return false;

            m_created.emplace_back( *it, aOwnContents && *it == directory );
        }

        return std::filesystem::is_directory( directory, error ) && !error;
    }

    void Commit() { m_committed = true; }

private:
    std::vector<std::pair<std::filesystem::path, bool>> m_created;
    bool                                                m_committed = false;
};

inline wxString DowngradeLibraryCopyPath( const wxString& aLibrary, const wxString& aOutputDirectory )
{
    std::error_code error;
    auto            source = std::filesystem::canonical( DowngradeNativePath( aLibrary ), error );

    if( error )
        return wxEmptyString;

    return DowngradeWxPath( DowngradeNativePath( aOutputDirectory ) / source.filename() );
}

inline bool CopyDowngradeLibraryDirectory( const wxString& aSource, const wxString& aDestination,
                                           DOWNGRADE_OUTPUT_DIRECTORIES& aDirectories, wxString& aError )
{
    if( !ValidateDowngradeLibraryCopy( aSource, aDestination, aError ) )
        return false;

    if( !aDirectories.Create( aDestination, true ) )
    {
        aError = _( "Could not create the library output folder." );
        return false;
    }

    std::error_code error;
    auto            source = std::filesystem::canonical( DowngradeNativePath( aSource ), error );
    std::filesystem::recursive_directory_iterator       iterator( source, error );
    const std::filesystem::recursive_directory_iterator end;

    while( !error && iterator != end )
    {
        const auto& entry = *iterator;

        if( IsDowngradeLibraryWorkingArtifact( DowngradeWxPath( entry.path() ) ) )
        {
            iterator.disable_recursion_pending();
        }
        else
        {
            auto destination = DowngradeNativePath( aDestination ) / entry.path().lexically_relative( source );

            if( entry.is_directory( error ) )
                std::filesystem::create_directory( destination, error );
            else if( !error )
                std::filesystem::copy_file( entry.path(), destination, error );
        }

        if( !error )
            iterator.increment( error );
    }

    if( error )
    {
        aError = _( "Could not copy the library folder." );
        return false;
    }

    return true;
}
