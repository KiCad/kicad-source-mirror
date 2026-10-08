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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <vector>

#include <compatibility_report.h>

#include <wx/string.h>

class KIWAY;

/// Run the board and schematic classification for a whole project without writing anything.
/// Returns false with aError set when a document or requested variant cannot be exported.
/// When the target drops variants, aFoundVariants receives the schematic's variant names for a picker.
/// Use the same aVariant and aDropInsteadOfApproximate policy for the preview and the real export.
bool ClassifyProjectForDowngrade( KIWAY& aKiway, const wxString& aProjectDir, const wxString& aProjectName,
                                  const wxString& aTargetName, COMPATIBILITY_REPORT& aReport, wxString& aError,
                                  std::vector<wxString>* aFoundVariants = nullptr,
                                  bool aDropInsteadOfApproximate = false, const wxString& aVariant = wxEmptyString );

/// Export a downgraded copy of the project into aDestDir. The destination must be a new or
/// empty folder. Copies design files only, downgrades them in the copy, and verifies the
/// result. Fails closed: on any failure the destination is removed and aError explains why.
/// The caller is expected to have classified first and obtained user consent. A non-empty
/// aVariant is flattened into the export.
bool ExportProjectForDowngrade( KIWAY& aKiway, const wxString& aProjectDir, const wxString& aProjectName,
                                const wxString& aTargetName, const wxString& aDestDir, wxString& aError,
                                const wxString& aVariant = wxEmptyString, bool aDropInsteadOfApproximate = false );
