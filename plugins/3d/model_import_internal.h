/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-3.0.html
 */

#ifndef MODEL_IMPORT_INTERNAL_H
#define MODEL_IMPORT_INTERNAL_H

#include <plugins/3dapi/model_import.h>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <string>

#include <wx/filefn.h>
#include <wx/string.h>

#ifdef _WIN32
#include <clocale>
#include <cstdlib>
#else
#include <locale.h>
#endif

wxString CreateModelImportTempFileName( const wxString& aPrefix );


/**
 * Remove the scratch file an importer expanded a compressed model into.
 */
struct MODEL_IMPORT_TEMP_FILE
{
    wxString path;

    ~MODEL_IMPORT_TEMP_FILE()
    {
        if( !path.empty() )
            wxRemoveFile( path );
    }
};


enum class MODEL_IMPORT_GZIP_RESULT
{
    OK,
    TOO_LARGE,
    CORRUPT
};

/**
 * Expand a gzip stream into @a aExpanded, refusing anything over @a aMaxExpandedBytes.
 */
MODEL_IMPORT_GZIP_RESULT DecompressBoundedModel( const char* aData, std::size_t aSize,
                                                 std::uint64_t aMaxExpandedBytes, std::string& aExpanded );


struct MODEL_IMPORT_STATUS
{
    S3D::MODEL_IMPORT_ERROR error = S3D::MODEL_IMPORT_ERROR::IMPORT_FAILED;
    std::string             diagnostic;

    void Set( S3D::MODEL_IMPORT_ERROR aError, const char* aDiagnostic )
    {
        error = aError;
        diagnostic = aDiagnostic;
    }
};


class MODEL_IMPORT_CANCELED : public std::exception
{
public:
    const char* what() const noexcept override { return "Model import canceled"; }
};


class MODEL_IMPORT_NUMERIC_LOCALE
{
public:
    MODEL_IMPORT_NUMERIC_LOCALE( const MODEL_IMPORT_NUMERIC_LOCALE& ) = delete;
    MODEL_IMPORT_NUMERIC_LOCALE& operator=( const MODEL_IMPORT_NUMERIC_LOCALE& ) = delete;

#ifdef _WIN32
    MODEL_IMPORT_NUMERIC_LOCALE()
    {
        m_previousMode = _configthreadlocale( _ENABLE_PER_THREAD_LOCALE );
        const char* current = std::setlocale( LC_NUMERIC, nullptr );

        if( current )
            m_previousLocale = current;

        std::setlocale( LC_NUMERIC, "C" );
    }

    ~MODEL_IMPORT_NUMERIC_LOCALE()
    {
        if( !m_previousLocale.empty() )
            std::setlocale( LC_NUMERIC, m_previousLocale.c_str() );

        if( m_previousMode > 0 )
            _configthreadlocale( m_previousMode );
    }

private:
    int         m_previousMode = _DISABLE_PER_THREAD_LOCALE;
    std::string m_previousLocale;
#else
    MODEL_IMPORT_NUMERIC_LOCALE()
    {
        locale_t current = uselocale( static_cast<locale_t>( 0 ) );
        locale_t base = duplocale( current );

        if( !base )
            return;

        m_locale = newlocale( LC_NUMERIC_MASK, "C", base );

        if( !m_locale )
        {
            freelocale( base );
            return;
        }

        m_previous = uselocale( m_locale );
    }

    ~MODEL_IMPORT_NUMERIC_LOCALE()
    {
        if( m_previous )
            uselocale( m_previous );

        if( m_locale )
            freelocale( m_locale );
    }

private:
    locale_t m_locale = nullptr;
    locale_t m_previous = nullptr;
#endif
};

#endif
