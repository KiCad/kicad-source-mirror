/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#ifndef FILTER_READER_H_
#define FILTER_READER_H_

#include <richio.h>
#include <wx/string.h>


/**
 * Read lines of text from another #LINE_READER but only returns non-comment lines and
 * non-blank lines from its ReadLine() function.
 */
class FILTER_READER : public LINE_READER
{
public:
    /**
     * Does not take ownership over @a aReader so will not destroy it.
     */
    FILTER_READER( LINE_READER& aReader );

    ~FILTER_READER();

    char* ReadLine() override;

    const wxString& GetSource() const override
    {
        return reader.GetSource();
    }

    unsigned LineNumber() const override
    {
        return reader.LineNumber();
    }

private:
    LINE_READER& reader;
};


/**
 * Read lines of text from another #LINE_READER but only returns non-comment lines and
 * non-blank lines with leading whitespace characters (space and tab) removed from its
 * ReadLine() function.
 */
class WHITESPACE_FILTER_READER : public LINE_READER
{
public:
    /**
     * Do not take ownership over @a aReader, so will not destroy it.
     */
    WHITESPACE_FILTER_READER( LINE_READER& aReader );

    ~WHITESPACE_FILTER_READER();

    char* ReadLine() override;

    const wxString& GetSource() const override
    {
        return reader.GetSource();
    }

    unsigned LineNumber() const override
    {
        return reader.LineNumber();
    }

private:
    LINE_READER& reader;
};

#endif // FILTER_READER_H_
