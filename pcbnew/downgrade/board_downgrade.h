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

class BOARD;
class FOOTPRINT;

/// Report what a board would lose if exported to an older KiCad. Does not modify the board.
/// aDropInsteadOfApproximate reports LOWER features as DROP and selects their omission transforms.
COMPATIBILITY_REPORT ClassifyBoardForDowngrade( const BOARD* aBoard, const DOWNGRADE_TARGET& aTarget,
                                                bool aDropInsteadOfApproximate = false );

/// Approximate or remove, in place, what the target cannot represent.
/// When aDropInsteadOfApproximate is true, omit LOWER features without generating replacements.
void DowngradeBoardInPlace( BOARD* aBoard, const DOWNGRADE_TARGET& aTarget, bool aDropInsteadOfApproximate = false );

/// Footprint-scoped downgrade for standalone .kicad_mod files the board pass never sees.
void DowngradeFootprintInPlace( FOOTPRINT* aFootprint, const DOWNGRADE_TARGET& aTarget,
                                bool aDropInsteadOfApproximate = false );

/// Bake a variant's overrides into the base footprint attributes. Run before the variant
/// registry is dropped, so the exported board matches the chosen variant.
void FlattenBoardVariant( BOARD* aBoard, const wxString& aVariantName );

/// Scan serialized board text for tokens the target cannot parse. Returns the first such token, or
/// empty if the output is safe. Non-empty means refuse the export.
wxString FindUnsupportedBoardToken( const wxString& aSerialized, const DOWNGRADE_TARGET& aTarget );

/// The format the denylist was generated against. A format bump forces a review.
int BoardDowngradeCoveredVersion();

/// First board format that stores custom footprint stackups.
constexpr int FIRST_FP_CUSTOM_STACKUP = 20250818;

/// First board format that stores variants.
constexpr int FIRST_PCB_VARIANTS = 20260101;
