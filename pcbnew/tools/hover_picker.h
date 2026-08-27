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

#ifndef HOVER_PICKER_H
#define HOVER_PICKER_H

#include <kiid.h>
#include <math/vector2d.h>

#include <functional>
#include <vector>

class BOARD_ITEM;
class PCB_SELECTION_TOOL;
class TOOL_MANAGER;

/// The board item under the pointer, and the highlight that follows it.
class HOVER_PICKER
{
public:
    /// Everything it sees is already selectable.
    using ACCEPTS = std::function<bool( BOARD_ITEM& aItem )>;

    /// Optional tie-break among what the selection heuristics left.  Nearest wins.
    using PROXIMITY = std::function<double( const BOARD_ITEM& aItem, const VECTOR2I& aPointer )>;

    explicit HOVER_PICKER( TOOL_MANAGER* aToolMgr );

    /// Puts back whatever is still lit, so an early exit cannot strand a highlight.
    ~HOVER_PICKER();

    /// The item a click would take, or null.  CollectPoint() decides, so the layer and size
    /// preferences and the Selection Filter all apply.  Highlights nothing by itself.
    BOARD_ITEM* Pick( const VECTOR2I& aPointer, const ACCEPTS& aAccepts,
                      const PROXIMITY& aProximity = nullptr ) const;

    void Brighten( BOARD_ITEM* aItem );

    /// Light exactly these, leaving what is already lit alone.  A preview recomputes its whole
    /// set on every motion, and clearing it first would redraw items that did not change.
    void BrightenOnly( const std::vector<BOARD_ITEM*>& aItems );

    /// Items go back by id, so a commit that replaced one still puts the original back.
    void ClearBrightening();

private:
    TOOL_MANAGER*       m_toolMgr;
    PCB_SELECTION_TOOL* m_selectionTool;
    std::vector<KIID>   m_brightened;
};

#endif
