/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SCH_IO_ALTIUM_LIB_CACHE_H_
#define SCH_IO_ALTIUM_LIB_CACHE_H_

#include "sch_io/sch_io_lib_cache.h"

class FILE_LINE_READER;
class SCH_IO_KICAD_SEXPR;

/**
 * A cache assistant for Altium symbol libraries.
 */
class SCH_IO_ALTIUM_LIB_CACHE : public SCH_IO_LIB_CACHE
{
public:
    SCH_IO_ALTIUM_LIB_CACHE( const wxString& aLibraryPath );
    virtual ~SCH_IO_ALTIUM_LIB_CACHE();

    void Load() override;

private:
    friend SCH_IO_KICAD_SEXPR;

};

#endif    // SCH_IO_ALTIUM_LIB_CACHE_H_
