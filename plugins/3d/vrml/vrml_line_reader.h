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

#ifndef VRML_LINE_READER_H
#define VRML_LINE_READER_H

#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

#include <wx/string.h>

#include <plugins/3dapi/model_import.h>


class VRML_LINE_READER
{
public:
    VRML_LINE_READER( const wxString& aFileName, unsigned aStartingLineNumber, unsigned aMaxLineLength,
                      const S3D::MODEL_IMPORT_OPTIONS& aOptions );
    ~VRML_LINE_READER();

    VRML_LINE_READER( const VRML_LINE_READER& ) = delete;
    VRML_LINE_READER& operator=( const VRML_LINE_READER& ) = delete;

    char*           ReadLine();
    const char*     Line() const { return m_line.data(); }
    unsigned        LineNumber() const { return m_lineNumber; }
    const wxString& Source() const { return m_source; }

private:
    /// Refill the read buffer, returning false at end of file or on a read error.
    bool fill();

    FILE*                     m_file = nullptr;
    wxString                  m_source;
    std::string               m_line;
    std::vector<char>         m_buffer;
    std::size_t               m_bufferPos = 0;
    std::size_t               m_bufferEnd = 0;
    unsigned                  m_lineNumber = 0;
    unsigned                  m_maxLineLength = 0;
    S3D::MODEL_IMPORT_OPTIONS m_options;
};

#endif
