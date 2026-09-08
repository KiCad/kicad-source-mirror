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

#include <functional>
#include <unordered_map>
#include <map>
#include <memory>
#include <set>
#include <vector>
#include <sch_netchain.h>

class CONNECTION_GRAPH;
class SCHEMATIC;
void boost_test_inject_committed_net_chain( CONNECTION_GRAPH&, std::unique_ptr<SCH_NETCHAIN> );

namespace SCH_CONNECTIVITY
{
struct NETCHAIN_INPUT;

/** Committed net chains and overrides.  Staged and temporary graphs own a private instance. */
class NETCHAIN_MANAGER
{
public:
    explicit NETCHAIN_MANAGER( SCHEMATIC* aSchematic ) :
            m_schematic( aSchematic )
    {
    }

    void Rebuild( const NETCHAIN_INPUT& aConnectivity, const std::function<void( NETCHAIN_MANAGER& )>& aBeforePublish = {} );

    void ClearDerived();
    void Merge( NETCHAIN_MANAGER& aOther );

    void SetSchematic( SCHEMATIC* aSchematic ) { m_schematic = aSchematic; }

    struct CHAIN_TERMINAL_REF
    {
        wxString ref;
        wxString pin;
    };
    using CHAIN_TERMINAL_REFS = std::pair<CHAIN_TERMINAL_REF, CHAIN_TERMINAL_REF>;

    static SCH_NETCHAIN* resolvePotentialChainByTerminals(
            const CHAIN_TERMINAL_REFS& aTermRefs, const std::map<std::pair<wxString, wxString>, wxString>& aRefPinToNet,
            const std::vector<std::unique_ptr<SCH_NETCHAIN>>& aPotentials, const wxString& aChainName );

    bool DeleteCommittedNetChain( const wxString& aName );

    bool RenameCommittedNetChain( const wxString& aOld, const wxString& aNew );

    void rekeyOverrideMaps( const wxString& aOld, const wxString& aNew );

    void refreshCommittedChainPayload( SCH_NETCHAIN* aTarget, const std::set<wxString>& aNets,
                                       const std::set<SCH_SYMBOL*>& aSymbols, const KIID& aTerminalPinA,
                                       const KIID& aTerminalPinB, const wxString& aRefA, const wxString& aPinNumA,
                                       const wxString& aRefB, const wxString& aPinNumB );

    void refreshCommittedChainFromPotential( SCH_NETCHAIN* aTarget, const SCH_NETCHAIN& aSource );

    SCH_NETCHAIN* CreateNetChainFromPotential( SCH_NETCHAIN* aPotential, const wxString& aName );

    SCH_NETCHAIN* CreateManualNetChain( const wxString& aName, const std::set<SCH_SYMBOL*>& aSymbols,
                                        const std::set<wxString>& aNets, const KIID& aTerminalPinA,
                                        const KIID& aTerminalPinB, const wxString& aRefA, const wxString& aPinNumA,
                                        const wxString& aRefB, const wxString& aPinNumB );

    SCH_NETCHAIN* GetNetChainForNet( const wxString& aNet );

    void ApplyNetChainNetclasses();

    SCH_NETCHAIN* GetNetChainByName( const wxString& aName );

    void ReplaceNetChainTerminalPin( const wxString& aNetChain, const KIID& aPrev, const KIID& aNew );

    void SetNetChainTerminalOverrides( const std::map<wxString, std::pair<KIID, KIID>>& aOverrides );

    const std::vector<std::unique_ptr<SCH_NETCHAIN>>& GetPotentialNetChains() const { return m_potentialNetChains; }
    const std::vector<std::unique_ptr<SCH_NETCHAIN>>& GetCommittedNetChains() const { return m_committedNetChains; }
    bool NetChainsBuilt() const { return m_netChainsBuilt; }

    const std::map<wxString, std::set<wxString>>& GetNetChainMemberNetOverrides() const
    {
        return m_netChainMemberNetOverrides;
    }

    const std::map<wxString, wxString>& GetNetChainNetClassOverrides() const
    {
        return m_netChainNetClassOverrides;
    }

    const std::map<wxString, KIGFX::COLOR4D>& GetNetChainColorOverrides() const
    {
        return m_netChainColorOverrides;
    }

    const std::map<wxString, CHAIN_TERMINAL_REFS>& GetNetChainTerminalRefOverrides() const
    {
        return m_netChainTerminalRefOverrides;
    }

private:
    void rebuild( const NETCHAIN_INPUT& aConnectivity );
    void setSymbolName( SCH_SYMBOL* aSymbol, const wxString& aName );
    friend class ::CONNECTION_GRAPH;
    friend void ::boost_test_inject_committed_net_chain( CONNECTION_GRAPH&, std::unique_ptr<SCH_NETCHAIN> );

    std::vector<std::unique_ptr<SCH_NETCHAIN>> m_committedNetChains;
    std::vector<std::unique_ptr<SCH_NETCHAIN>> m_potentialNetChains;
    bool                                       m_netChainsBuilt = false;
    std::map<wxString, std::pair<KIID, KIID>>  m_netChainTerminalOverrides;
    std::map<wxString, wxString>               m_netChainNetClassOverrides;
    std::map<wxString, KIGFX::COLOR4D>         m_netChainColorOverrides;
    std::map<wxString, CHAIN_TERMINAL_REFS>    m_netChainTerminalRefOverrides;
    std::map<wxString, std::set<wxString>>     m_netChainMemberNetOverrides;

    // Bridge-graph helper types shared by RebuildNetChains() and FindNetChainPathsBetweenPins().
    // A bridge edge represents a 2-pin passthrough symbol that ties two distinct subgraph nets
    // together; the bridge graph is the adjacency built from the surviving (non-power-touching)
    // edges after the leaf-prune pass.

    struct BRIDGE_EDGE
    {
        wxString             a;
        wxString             b;
        class SCH_SYMBOL*    sym;
    };

    struct BRIDGE_NEIGHBOR
    {
        wxString             other;
        class SCH_SYMBOL*    sym;
    };

    struct BRIDGE_GRAPH
    {
        std::map<wxString, std::vector<BRIDGE_NEIGHBOR>> adjacency;
        std::vector<BRIDGE_EDGE>                         edges;
    };

    /**
     * Build the bridge graph used for net-chain discovery.  Walks every 2-pin passthrough
     * symbol on every sheet and records the raw bridge edge list in `edges`; the returned
     * `adjacency` is built from those edges after dropping any that touch a power subgraph
     * and after iteratively pruning power-adjacent leaf nets.  `edges` itself stays raw
     * because RebuildNetChains() still iterates the full list to attach bridging symbols
     * to their owning component.  Does NOT apply the legacy >4-net stub trim — that fossil
     * lives only in RebuildNetChains() so the path-enumeration API can see the unpruned
     * adjacency.
     */
    BRIDGE_GRAPH buildBridgeAdjacency( const NETCHAIN_INPUT& aConnectivity );

    std::unordered_map<SCH_SYMBOL*, wxString>* m_pendingSymbolNames = nullptr;
    SCHEMATIC* m_schematic;
};
} // namespace SCH_CONNECTIVITY
