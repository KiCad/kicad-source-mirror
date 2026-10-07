/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <filesystem>
#include <string>
#include <system_error>

namespace KI_TEST
{

/**
 * A temporary directory deleted when it goes out of scope.
 *
 * Footprint saves validate the entire containing directory as a library. Isolating each test
 * prevents unrelated .kicad_mod files in the system temporary directory from affecting it.
 */
class TEMPORARY_DIRECTORY
{
public:
    TEMPORARY_DIRECTORY( const std::string& aNamePrefix, const std::string& aSuffix = "" )
    {
        for( size_t i = 0;; ++i )
        {
            m_path = std::filesystem::temp_directory_path() / ( aNamePrefix + std::to_string( i ) + aSuffix );

            std::error_code error;

            if( std::filesystem::create_directory( m_path, error ) )
                break;

            if( error && error != std::errc::file_exists )
                throw std::filesystem::filesystem_error( "Cannot create temporary test directory", m_path, error );
        }
    }

    TEMPORARY_DIRECTORY( const TEMPORARY_DIRECTORY& ) = delete;
    TEMPORARY_DIRECTORY& operator=( const TEMPORARY_DIRECTORY& ) = delete;

    ~TEMPORARY_DIRECTORY()
    {
        std::error_code ignored;
        std::filesystem::remove_all( m_path, ignored );
    }

    const std::filesystem::path& GetPath() const { return m_path; }

private:
    std::filesystem::path m_path;
};

} // namespace KI_TEST
