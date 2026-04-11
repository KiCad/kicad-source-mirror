/*
 * This program source code file is part of KiCad, a free EDA CAD application.

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

/**
 * @file pcb_tool_utils.h
 *
 * Utility functions that can be shared by PCB tools.
 * Public-API class operations that are not specific to any one tool.
 */

#pragma once

#include <optional>

class BOARD_ITEM;

/**
 * Gets the width of a BOARD_ITEM, for items that have a meaningful width.
 * (e.g. some graphics, tracks).
 *
 * @param aItem The item to get the width of.
 * @return The width of the item, or std::nullopt if the item has no useful width.
 */
std::optional<int> GetBoardItemWidth( const BOARD_ITEM& aItem );