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
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KICAD_SCH_API_SAVE_H
#define KICAD_SCH_API_SAVE_H

class PROJECT;
class SCHEMATIC;
class SCH_SHEET;

#include <wx/string.h>

namespace SCH_API_SAVE
{

/// Write a single sheet to disk via SCH_IO.
bool SaveSheetToFile( SCH_SHEET* aSheet, SCHEMATIC& aSchematic, const wxString& aPath );

/// Sync schematic metadata into the project file (.kicad_pro) and save it.
void UpdateProjectFile( SCHEMATIC& aSchematic, PROJECT& aProject );

/// Save every screen in the hierarchy to its current path, then update the project file.
bool SaveSchematic( SCHEMATIC& aSchematic, PROJECT& aProject );

/**
 * Save the root schematic to @a aFileName without changing the open document.
 *
 * If @a aFileName matches the open root schematic, this delegates to SaveSchematic().
 * Otherwise only the root sheet is written (subsheets are not remapped).
 *
 * When @a aCreateProject is true and the destination project file does not exist yet,
 * a copy of the current project is written alongside the new schematic.
 */
bool SaveSchematicCopy( SCHEMATIC& aSchematic, PROJECT& aProject, const wxString& aFileName,
                        bool aCreateProject );

/**
 * Save the schematic to @a aFileName and adopt it: the root sheet and screen are re-homed to
 * the new path, the whole hierarchy is written, and the project is re-targeted to the new
 * location (creating a project file if none exists there).  Used by the headless IPC API
 * SaveDocumentAs handler; there is no frame, so no lock or file history is updated.
 *
 * @return true if the schematic was saved and the project re-targeted.
 */
bool SaveSchematicAs( SCHEMATIC& aSchematic, PROJECT& aProject, const wxString& aFileName );

} // namespace SCH_API_SAVE

#endif
