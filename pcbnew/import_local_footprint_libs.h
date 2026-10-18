/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef IMPORT_LOCAL_FOOTPRINT_LIBS_H
#define IMPORT_LOCAL_FOOTPRINT_LIBS_H

#include <vector>
#include <wx/string.h>

class PROJECT;
class REPORTER;

/**
 * Register the footprint libraries a source project names in the project footprint library table.
 *
 * A library inside @p aSourceProjectDir is copied into the project and referenced relative to it; any
 * other library keeps its absolute path.  A nickname the table already holds is left alone.
 *
 * Shared by the project manager's import mail handler and the `kicad-cli import` job.
 *
 * @param[in] aProject is the project whose table receives the rows.
 * @param[in] aSourceProjectDir is the directory of the source project.
 * @param[in] aLibraries are the absolute paths of the libraries to register.
 * @param[in] aReporter receives table save errors.
 * @return the nicknames of the rows that were added.
 */
std::vector<wxString> RegisterLocalFootprintLibraries( PROJECT& aProject, const wxString& aSourceProjectDir,
                                                       const std::vector<wxString>& aLibraries,
                                                       REPORTER&                    aReporter );

#endif // IMPORT_LOCAL_FOOTPRINT_LIBS_H
