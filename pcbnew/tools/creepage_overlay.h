/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers.
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

#include <map>
#include <memory>
#include <vector>

#include <layer_ids.h>

#include <drc/drc_creepage_engine.h>

class BOARD;
class BOARD_ITEM;
class DRC_ENGINE;

namespace KIGFX
{
class VIEW;
class VIEW_OVERLAY;
}


/**
 * Live creepage overlay shown while dragging items.
 *
 * Runs one interactive CREEPAGE_ENGINE per copper layer the dragged items occupy (creepage is a
 * per-layer surface distance) and draws onto a single GAL VIEW_OVERLAY. At drag start each engine
 * snapshots the static board-edge sub-graph; each frame it recomputes creepage for the dragged
 * nets at the items' current position and redraws the violating and near-violating paths. The
 * whole feature is gated by the RealtimeCreepage advanced config flag, so it is only constructed
 * when enabled.
 */
class CREEPAGE_OVERLAY
{
public:
    CREEPAGE_OVERLAY( BOARD* aBoard, std::shared_ptr<DRC_ENGINE> aDrcEngine, KIGFX::VIEW* aView );
    ~CREEPAGE_OVERLAY();

    /// True when the board has creepage constraints and the feature flag is on.
    bool IsEnabled() const { return m_enabled; }

    /// Begin a drag session for the given items, on every copper layer they occupy. No-op if not
    /// enabled.
    void Start( const std::vector<BOARD_ITEM*>& aMovingItems );

    /// Recompute and redraw at the items' current board positions.
    void Update();

    /// End the session and clear the overlay.
    void Stop();

private:
    void clearOverlay();

    BOARD*                      m_board;
    std::shared_ptr<DRC_ENGINE> m_drcEngine;
    KIGFX::VIEW*                m_view;
    int                         m_minGrooveWidth;

    // One interactive engine per dragged copper layer, each with its own near-violation band
    // (a layer's band must not exceed the search radius that layer's engine actually built).
    struct LAYER_ENGINE
    {
        std::unique_ptr<CREEPAGE_ENGINE> m_engine;
        int                              m_nearMargin = 0;
    };

    std::shared_ptr<KIGFX::VIEW_OVERLAY>     m_overlay;
    std::map<PCB_LAYER_ID, LAYER_ENGINE>     m_engines;

    bool         m_enabled;
    bool         m_active;

    // Frame-drop throttle: a heavy Update defers the next m_skipFrames motion frames so the drag
    // stays responsive rather than blocking. The overlay keeps showing the last result meanwhile.
    int m_skipFrames;
};
