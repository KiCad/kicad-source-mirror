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

#include <compatibility_report.h>
#include <downgrade_target.h>

class SCH_SCREEN;
class SCHEMATIC;
class LIB_SYMBOL;

/// Report what a schematic screen would lose if exported to an older KiCad. Does not modify it.
/// aDropInsteadOfApproximate reports LOWER features as DROP and selects their omission transforms.
COMPATIBILITY_REPORT ClassifyScreenForDowngrade( const SCH_SCREEN* aScreen, const DOWNGRADE_TARGET& aTarget,
                                                 bool aDropInsteadOfApproximate = false );

/// Schematic-wide structure the per-screen rules cannot see. Does not modify it.
COMPATIBILITY_REPORT ClassifySchematicStructureForDowngrade( const SCHEMATIC&        aSchematic,
                                                             const DOWNGRADE_TARGET& aTarget );

/// Approximate or remove, in place, what the target cannot represent.
/// When aDropInsteadOfApproximate is true, omit LOWER features without generating replacements.
void DowngradeScreenInPlace( SCH_SCREEN* aScreen, const DOWNGRADE_TARGET& aTarget,
                             bool aDropInsteadOfApproximate = false );

/// Report what a library symbol would lose if exported to an older KiCad. Does not modify it.
COMPATIBILITY_REPORT ClassifyLibSymbolForDowngrade( const LIB_SYMBOL* aSymbol, const DOWNGRADE_TARGET& aTarget,
                                                    bool aDropInsteadOfApproximate = false );

/// Symbol-scoped downgrade for standalone .kicad_sym files the screen pass never sees.
void DowngradeLibSymbolInPlace( LIB_SYMBOL* aSymbol, const DOWNGRADE_TARGET& aTarget,
                                bool aDropInsteadOfApproximate = false );

/// Scan serialized schematic text for tokens the target cannot parse. Returns the first such
/// token, or empty if the output is safe. Non-empty means refuse the export.
wxString FindUnsupportedSchToken( const wxString& aSerialized, const DOWNGRADE_TARGET& aTarget );

/// The format the denylist was generated against. A format bump forces a review.
int SchDowngradeCoveredVersion();

/// The symbol library format the denylist was generated against.
int SymbolLibDowngradeCoveredVersion();

/// Format dates for rules the jobs handler applies at schematic scope.
constexpr int SCH_VER_VARIANTS = 20250922;
constexpr int SCH_VER_NET_CHAINS = 20260512;
constexpr int SCH_VER_FLAT_HIERARCHY = 20251012;
