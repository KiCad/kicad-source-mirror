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

#ifndef MDW_PLACED_PARTS_H
#define MDW_PLACED_PARTS_H

#include <optional>
#include <string>

#include <wx/string.h>

class PROGRESS_REPORTER;
class PROJECT;


/**
 * The symbols placed in the schematic saved at aFile, read as Release Design checks them. A JSON array of
 * objects with reference, lib_id, digest, power and, for a symbol with an MDW_Part field, part. Nothing when
 * the schematic does not load. aReporter, when given, shows the load's progress.
 */
std::optional<std::string> SchCollectPlacedParts( const wxString& aFile, PROJECT& aProject,
                                                  PROGRESS_REPORTER* aReporter = nullptr );

#endif // MDW_PLACED_PARTS_H
