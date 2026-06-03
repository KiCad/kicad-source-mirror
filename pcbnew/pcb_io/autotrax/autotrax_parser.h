/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Format interpretation derived from pcb-rnd src_plugins/io_autotrax:
 *   Copyright (C) 2016, 2017, 2018, 2020 Tibor 'Igor2' Palinkas
 *   Copyright (C) 2016, 2017 Erich S. Heinzle
 * Used under GPL v2-or-later.
 *
 * Copyright (C) 2026 KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef AUTOTRAX_PARSER_H_
#define AUTOTRAX_PARSER_H_

#include <wx/arrstr.h>
#include <wx/string.h>

#include "autotrax_model.h"

class REPORTER;


/**
 * Line-oriented parser for Protel Autotrax / Easytrax PCB files.
 *
 * The format is a flat state machine: a two-letter record keyword on its own
 * line is followed by one or more data lines. Free primitives use F-prefixed
 * keywords (FT/FA/FV/FF/FP/FS); the same primitives inside a COMP block use
 * C-prefixed keywords. The parser emits an intermediate model rather than
 * touching any KiCad object.
 *
 * Parsing is tolerant. A malformed record is reported and skipped instead of
 * aborting the import, so partially corrupt files still yield a usable board.
 */
class AUTOTRAX_PARSER
{
public:
    explicit AUTOTRAX_PARSER( REPORTER* aReporter ) :
            m_reporter( aReporter )
    {
    }

    /**
     * Parse @p aContents into @p aBoard.
     *
     * @return true if a valid "PCB FILE 4/5" header was seen.
     */
    bool Parse( const wxString& aContents, AUTOTRAX::BOARD_DATA& aBoard );

    /// Cheap content sniff: the first non-blank, non-comment line is the magic
    /// header "PCB FILE 4" (Autotrax) or "PCB FILE 5" (Easytrax).
    static bool Sniff( const wxString& aContents );

private:
    /// Return the next line trimmed of surrounding whitespace, or false at end of
    /// input. Advances m_lineNo.
    bool nextLine( wxString& aLine );

    /// Tokenize a whitespace-separated data line into C-locale-parseable tokens.
    static wxArrayString tokenize( const wxString& aLine );

    void warn( const wxString& aMsg ) const;

    // Record parsers. Each consumes the data line(s) following its keyword and
    // appends to the supplied container (a component's or the board's).
    bool parseTrack( AUTOTRAX::TRACK& aOut );
    bool parseArc( AUTOTRAX::ARC& aOut );
    bool parseVia( AUTOTRAX::VIA& aOut );
    bool parsePad( AUTOTRAX::PAD& aOut );
    bool parseFill( AUTOTRAX::FILL& aOut );
    bool parseText( AUTOTRAX::TEXT& aOut );
    void parseComponent( AUTOTRAX::COMPONENT& aOut );
    void parseNetDef();

    REPORTER*             m_reporter = nullptr;
    AUTOTRAX::BOARD_DATA* m_board = nullptr;

    wxArrayString m_lines;
    size_t        m_index = 0;
    int           m_lineNo = 0;
};

#endif // AUTOTRAX_PARSER_H_
