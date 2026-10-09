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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file mdw_design_parts.h
 * The parts a project's saved schematic and board place, read through the editors' kifaces.
 */

#ifndef MDW_DESIGN_PARTS_H
#define MDW_DESIGN_PARTS_H

#include <mdw/client/mdw_baseline_design.h>

#include <wx/string.h>

#include <filesystem>
#include <vector>

class KIWAY;
class PROJECT;
class wxWindow;


/// When the schematic and board were last written, the minimum for one that does not exist
std::vector<std::filesystem::file_time_type> MdwDesignWriteTimes( PROJECT& aProject );

/**
 * Collect the parts placed in the project's saved schematic and board into aInputs, showing progress over
 * aParent. Answers the reason when they cannot be read, and empty when they were.
 */
wxString MdwCollectPlacedParts( wxWindow* aParent, KIWAY& aKiway, PROJECT& aProject,
                                std::vector<MDW_PLACED_INPUT>& aInputs );

/// The references that place aMember, each once, in the order a schematic numbers them
std::vector<wxString> MdwWhereUsed( const std::vector<MDW_PLACED_INPUT>& aInputs, const MDW_ITEM_REF& aMember );

#endif // MDW_DESIGN_PARTS_H
