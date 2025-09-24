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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

#include <optional>
#include <set>

#include <sch_item.h>
#include <tool/selection.h>

class SCHEMATIC;
class SCH_REFERENCE;
class SCH_SYMBOL;
class SCH_PIN;
class SCHEMATIC;
class LIB_ID;

wxString GetSchItemAsText( const SCH_ITEM& aItem );

wxString GetSelectedItemsAsText( const SELECTION& aSel );

/**
 * Get a list of unplaced (i.e. not in schamtic) unit numbers for a symbol.
 */
std::set<int> GetUnplacedUnitsForSymbol( const SCH_SYMBOL& aSym );

/**
 * Find a symbol by reference and unit.
 *
 * @return A SCH_REFERENCE to the found symbol, or std::nullopt if not found.
 */
std::optional<SCH_REFERENCE> FindSymbolByRefAndUnit( const SCHEMATIC& aSheet, const wxString& aRef,
                                                     int aUnit );

/**
 * Validates and gathers a selection containing multiple symbol units that all belong to the
 * same reference designator and originate from the same library symbol (LIB_ID).
 *
 * Used by gate label swap.
 *
 * @return a SELECTION of SCH_SYMBOL* if valid, or an empty vector if not valid.
 */
std::vector<SCH_SYMBOL*> GetSameSymbolMultiUnitSelection( const SELECTION& aSel );

/**
 * Swap the positions/lengths/etc. between two pins in a symbol in such a way that connectivity is maintained.
 *
 * The swap operates on the owning library pins when available so that callers may refresh symbol
 * instances afterwards via SCH_SYMBOL::UpdatePins().
 *
 * @return true if the swap touched a shared library pin.
 */
bool SwapPinGeometry( SCH_PIN* aFirst, SCH_PIN* aSecond );

/**
 * Returns true when the given symbol has instances, e.g. is used by more than one sheet instance in this project
 * or by more than one project.
 *
 * @param aSheetNames if not nullptr, will be filled with the sheet paths that have instances of aSymbol.
 * @param aProjectNames if not nullptr, will be filled with the names of other projects that have instances of aSymbol.
 *
 * @return true if the symbol has shared instances.
 */
bool SymbolHasSheetInstances( const SCH_SYMBOL& aSymbol, const wxString& aCurrentProject,
                              std::set<wxString>* aSheetPaths = nullptr, std::set<wxString>* aProjectNames = nullptr );

/**
 * Get human-readable sheet names from a set of sheet paths, e.g. the SHEETNAME field
 *
 * @param aSheetPaths set of sheet paths to convert
 * @param aSchematic the schematic to search for sheet names
 *
 * @return a set of human-readable sheet names, or the original sheet path if no name can be resolved in this schmatic (this happens when sheets are shared across projects)
 */
std::set<wxString> GetSheetNamesFromPaths( const std::set<wxString>& aSheetPaths, const SCHEMATIC& aSchematic );
