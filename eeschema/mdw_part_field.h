/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file mdw_part_field.h
 * @brief Writing and reading the MDW_Part field on a placed symbol.
 */

#ifndef MDW_PART_FIELD_H
#define MDW_PART_FIELD_H

#include <mdw_part_link.h>

#include <optional>

class SCH_SYMBOL;


/**
 * Record on aSymbol the part revision aPart names, adding the hidden MDW_Part field when it has none.
 * Called by an explicit workspace action only, never on load, save or update.
 */
void MdwWritePartField( SCH_SYMBOL& aSymbol, const MDW_PART_IDENTITY& aPart );

/// The part aSymbol was placed or assigned as, or nothing when it has no valid MDW_Part field
std::optional<MDW_PART_IDENTITY> MdwReadPartField( const SCH_SYMBOL& aSymbol );

#endif // MDW_PART_FIELD_H
