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
 */

#ifndef PCBNEW_DRC_CHAIN_TOPOLOGY_H
#define PCBNEW_DRC_CHAIN_TOPOLOGY_H

#include <set>
#include <vector>

#include <wx/string.h>

#include <layer_ids.h>
#include <math/vector2d.h>

class BOARD;
class BOARD_CONNECTED_ITEM;


/**
 * Build a topological view of a single named net chain's routed copper.
 *
 * The chain is modeled as a graph with (x, y, layer) nodes.  Tracks and arcs
 * contribute one edge each between their endpoints; vias contribute one edge
 * per adjacent copper layer pair within their layer span; series passives
 * (enumerated via EnumerateChainBridges) contribute one edge between their
 * cross-net pads on the footprint placement layer.  Mid-segment T-junctions
 * are detected via shape collision and the trunk track is split at the
 * contact point so the stub branches off the actual junction.
 *
 * When both terminal pads (NETINFO_ITEM::GetTerminalPad(0/1)) are present and
 * connected by a unique simple path, GetStatus() returns OK and:
 *   - TrunkLength() / TrunkDelay() give the terminal-to-terminal sum,
 *   - Stubs() gives the off-trunk branches with their branch points,
 *     contributing items, lengths, and delays.
 *
 * When the topology cannot be reduced to a tree with both terminals reachable,
 * callers fall back to legacy aggregate / proxy semantics.
 */
class CHAIN_TOPOLOGY
{
public:
    struct STUB
    {
        VECTOR2I                           branchPoint;
        PCB_LAYER_ID                       branchLayer;
        std::vector<BOARD_CONNECTED_ITEM*> items;
        double                             length;
        double                             delay;
    };

    enum class STATUS
    {
        OK,
        NO_ITEMS,
        NO_TERMINAL_PADS,
        DISCONNECTED,
        CYCLE_DETECTED
    };

    CHAIN_TOPOLOGY( BOARD* aBoard, const wxString& aChainName,
                    const std::set<BOARD_CONNECTED_ITEM*>& aChainItems );

    STATUS GetStatus() const { return m_status; }
    bool   IsValid() const { return m_status == STATUS::OK; }

    double TrunkLength() const { return m_trunkLength; }
    double TrunkDelay() const { return m_trunkDelay; }

    const std::vector<STUB>& Stubs() const { return m_stubs; }

    const wxString& GetChainName() const { return m_chainName; }

private:
    wxString          m_chainName;
    STATUS            m_status = STATUS::NO_ITEMS;
    double            m_trunkLength = 0.0;
    double            m_trunkDelay = 0.0;
    std::vector<STUB> m_stubs;
};

#endif // PCBNEW_DRC_CHAIN_TOPOLOGY_H
