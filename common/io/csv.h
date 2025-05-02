/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

#include <wx/string.h>
#include <wx/stream.h>

#include <istream>


class CSV_WRITER
{
public:
    CSV_WRITER( wxOutputStream& aStream );

    /**
     * Write a single row to the stream.
     */
    void WriteLine( const std::vector<wxString>& aCols );
    /**
     * Write a vector of rows to the stream.
     *
     * @param cols The rows to write.
     */
    void WriteLines( const std::vector<std::vector<wxString>>& aRows );

    void SetDelimiter( const wxString& aDelimiter )
    {
        m_delimiter = aDelimiter;
    }

    /**
     * Set the delimiter escape char. If set to wxEmptyString (default), the
     * delimiter is doubled for escaping.
     */
    void SetEscape( const wxString& aEscape )
    {
        m_escape = aEscape;
    }

private:
    wxOutputStream& m_stream;
    wxString        m_delimiter;
    wxString        m_quote;
    wxString        m_lineTerminator;
    wxString        m_escape;
};


/**
 * Try to guess the format of a T/CSV file and decode it into aData.
 *
 * This can handle the most common cases of CSV files and TSV files
 * (double/single quoted strings, commas or tabs as delimiters, but it's
 * not completely foolproof).
 */
bool AutoDecodeCSV( const wxString& aInput, std::vector<std::vector<wxString>>& aData );
