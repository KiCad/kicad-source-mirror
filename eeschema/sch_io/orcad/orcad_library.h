/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Based on the dsn2kicad reference implementation and on OrCAD file format
 * documentation from the OpenOrCadParser project (MIT licensed).
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
 * @file orcad_library.h
 *
 * Parser for the DSN root 'Library' stream: format version, fonts, page settings
 * and the global string table that property name/value indices reference
 * throughout the file.  Implemented in orcad_library.cpp.
 */

#ifndef ORCAD_LIBRARY_H_
#define ORCAD_LIBRARY_H_

#include <vector>

#include <sch_io/orcad/orcad_records.h>
#include <sch_io/orcad/orcad_stream.h>

/**
 * Read one 156-byte PageSettings block at the current cursor (embedded both in the
 * Library stream and in every Page stream).  Layout documented on
 * ORCAD_PAGE_SETTINGS.  Consumes exactly 156 bytes; throws IO_ERROR on overrun.
 */
ORCAD_PAGE_SETTINGS OrcadParsePageSettings( ORCAD_STREAM& aStream );

/**
 * Parse the whole 'Library' stream.
 *
 * Layout: NUL-terminated introduction inside a fixed 32-byte buffer, u16 major
 * version, u16 minor version, 8 bytes create/modify dates, 4 zero bytes, u16 font
 * count (stores count + 1: read count - 1 LOGFONTA records of 60 bytes each), then
 * for versionMajor >= 2 a u16 length (always 24) + 2 * length bytes + 8 bytes
 * (versionMajor < 2: a fixed 42 bytes instead), 8 lzt part-field names, the
 * 156-byte PageSettings block, the string table, u16 alias-pair count with
 * lzt/lzt pairs, and — when the introduction starts with "OrCAD Windows Design" —
 * 8 bytes followed by the lzt schematic (root folder) name.
 *
 * String table length field: modern files use u32, legacy pre-16.x files use u16.
 * Detection: read u32 first and reject it as implausible when it exceeds 2,000,000
 * or when count * 3 > remaining + 16; on rejection (or a string parse failure)
 * rewind and re-read with a u16 count.
 *
 * The caller must gate on versionMajor: designs with versionMajor < 3 use a
 * different framing and are rejected by this importer (clean IO_ERROR from the
 * plugin, not from here).
 *
 * @throws IO_ERROR when the stream is structurally unreadable.
 */
ORCAD_LIBRARY_INFO OrcadParseLibrary( const std::vector<char>& aData );

#endif // ORCAD_LIBRARY_H_
