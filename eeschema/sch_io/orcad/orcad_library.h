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


#ifndef ORCAD_LIBRARY_H_
#define ORCAD_LIBRARY_H_

#include <vector>

#include <sch_io/orcad/orcad_records.h>
#include <sch_io/orcad/orcad_stream.h>

/** Consumes 156 bytes; throws IO_ERROR on overrun. */
ORCAD_PAGE_SETTINGS OrcadParsePageSettings( ORCAD_STREAM& aStream );

/** The Library version selects the string-count width. Throws IO_ERROR for invalid data. */
ORCAD_LIBRARY_INFO OrcadParseLibrary( const std::vector<char>& aData );

#endif // ORCAD_LIBRARY_H_
