/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

class SCH_REFERENCE;
class SCH_SYMBOL;
class SCHEMATIC;

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
