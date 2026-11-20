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

#include <unordered_map>
#include <vector>

#include <kiid.h>

class BOARD;
class EDA_ITEM;
class PICKED_ITEMS_LIST;

/**
 * Keeps group membership consistent through one undo or redo of a picked items list.
 *
 * Group links are back-references that the item swaps do not carry, and taking an item off the
 * board while the list is applied also drops it from its live group.  Construct this before the
 * list is applied to capture the membership being left, and call Restore() once it has been.
 */
class UNDO_GROUP_STATE
{
public:
    UNDO_GROUP_STATE( BOARD& aBoard, const PICKED_ITEMS_LIST& aList );

    /**
     * Rebuild the membership recorded in the list, then record the captured membership in its
     * place for the reverse operation.
     */
    void Restore( PICKED_ITEMS_LIST& aList );

private:
    struct STATE
    {
        KIID              m_groupId;
        std::vector<KIID> m_members;
    };

    BOARD&                                     m_board;
    std::unordered_map<const EDA_ITEM*, STATE> m_captured;
};
