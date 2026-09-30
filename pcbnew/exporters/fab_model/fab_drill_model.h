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

#ifndef FAB_DRILL_MODEL_H
#define FAB_DRILL_MODEL_H

#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

#include <drill/drill_operation.h>

class BOARD;
class BOARD_ITEM;


struct FAB_DRILL_LAYER
{
    DRILL_SPAN m_Span;
    std::vector<DRILL_OPERATION> m_Holes;
    std::vector<DRILL_OPERATION> m_Slots;
};


class FAB_DRILL_MODEL
{
public:
    explicit FAB_DRILL_MODEL( const BOARD& aBoard );
    FAB_DRILL_MODEL( const FAB_DRILL_MODEL& ) = delete;
    FAB_DRILL_MODEL& operator=( const FAB_DRILL_MODEL& ) = delete;

    const std::vector<FAB_DRILL_LAYER>& Layers() const { return m_layers; }

    const DRILL_OPERATION* Find( const DRILL_SPAN& aSpan, const BOARD_ITEM* aItem,
                                 bool aIgnorePlating = false ) const;

private:
    std::vector<FAB_DRILL_LAYER> m_layers;
    std::map<std::pair<DRILL_LAYER_PAIR, bool>, std::unordered_map<const BOARD_ITEM*, const DRILL_OPERATION*>> m_index;
};

#endif // FAB_DRILL_MODEL_H
