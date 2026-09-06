/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2025 VUT Brno, Faculty of Electrical Engineering and Communication
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

#ifndef KICAD_SCH_DRAG_NET_COLLISION_H
#define KICAD_SCH_DRAG_NET_COLLISION_H

#include <cstddef>
#include <optional>
#include <unordered_map>
#include <vector>
#include <memory>

#include <gal/cursors.h>
#include <math/vector2d.h>
#include <sch_sheet_path.h>

class SCH_EDIT_FRAME;
class SCH_ITEM;
class SCH_JUNCTION;
class SCH_SELECTION;

namespace KIGFX
{
class VIEW;
class VIEW_OVERLAY;
}

/**
 * Helper responsible for tracking the original net assignments of items involved in a drag
 * operation and providing approximate local merge and broken-contact warnings.
 * Net assignments are frozen at startup; dragging never rebuilds the connectivity graph.
 */
class SCH_DRAG_NET_COLLISION_MONITOR
{
public:
    SCH_DRAG_NET_COLLISION_MONITOR( SCH_EDIT_FRAME* aFrame, KIGFX::VIEW* aView );
    ~SCH_DRAG_NET_COLLISION_MONITOR();

    void Initialize( const SCH_SELECTION& aSelection );

    bool Update( const std::vector<SCH_JUNCTION*>& aJunctions, const SCH_SELECTION& aSelection );

    void Reset();

    KICURSOR AdjustCursor( KICURSOR aBaseCursor ) const;

private:
    struct COLLISION_MARKER
    {
        VECTOR2I position;
        double   radius;
    };

    struct DISCONNECTION_MARKER
    {
        VECTOR2I pointA;
        VECTOR2I pointB;
        double   radius;
    };

    struct ORIGINAL_CONNECTION
    {
        SCH_ITEM* itemA;
        size_t    indexA;
        SCH_ITEM* itemB;
        size_t    indexB;
    };

    std::optional<COLLISION_MARKER> analyzeJunction( SCH_JUNCTION* aJunction,
                                                      const SCH_SELECTION& aSelection ) const;

    void recordItemNet( SCH_ITEM* aItem );
    void recordOriginalConnections( const SCH_SELECTION& aSelection );
    std::vector<DISCONNECTION_MARKER> collectDisconnectedMarkers( const SCH_SELECTION& aSelection ) const;

    void ensureOverlay();
    void clearOverlay() const;

private:
    SCH_EDIT_FRAME* m_frame;
    KIGFX::VIEW*    m_view;
    std::shared_ptr<KIGFX::VIEW_OVERLAY> m_overlay;
    std::unordered_map<const SCH_ITEM*, std::optional<int>> m_itemNetCodes;
    SCH_SHEET_PATH m_sheetPath;
    std::vector<ORIGINAL_CONNECTION> m_originalConnections;
    bool            m_hasCollision;
};

#endif // KICAD_SCH_DRAG_NET_COLLISION_H
