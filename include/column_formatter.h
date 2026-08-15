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

#pragma once

#include <wx/string.h>

#include <reporter.h>
#include <units_provider.h>


class COLUMN_FORMATTER
{
public:
    enum class BOOL_FORMAT
    {
        ZERO_ONE,
        TRUE_FALSE,
    };

    virtual ~COLUMN_FORMATTER() = default;

    COLUMN_FORMATTER( UNITS_PROVIDER& aUnitsProvider, bool aIncludeUnits, BOOL_FORMAT aBoolFormat,
                      REPORTER& aReporter ) :
            m_unitsProvider( aUnitsProvider ),
            m_includeUnits( aIncludeUnits ),
            m_boolFormat( aBoolFormat ),
            m_reporter( aReporter )
    {
    }

protected:
    wxString stringFromBool( bool aValue ) const;

    bool boolFromString( const wxString& aValue, REPORTER& aReporter ) const;

    UNITS_PROVIDER& m_unitsProvider;
    bool            m_includeUnits;
    BOOL_FORMAT     m_boolFormat;
    REPORTER&       m_reporter;
};


/**
 * Return true if the given string matches either the translated or native version of the given label.
 */
bool MatchTranslationOrNative( const wxString& aStr, const wxString& aNativeLabel, bool aCaseSensitive );