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
#include <map>
#include <memory>
#include <set>
#include <vector>
#include <unordered_map>
#include <sch_netchain.h>
#include <gal/color4d.h>

class CONNECTION_GRAPH;
class SCHEMATIC;
class SCH_ITEM;
class SCH_PIN;
class SCH_SCREEN;
class SCH_SHEET_PATH;
class SCH_SYMBOL;
class NET_SETTINGS;
void boost_test_inject_committed_net_chain( CONNECTION_GRAPH&, std::unique_ptr<SCH_NETCHAIN> );

namespace SCH_CONNECTIVITY
{
struct NETCHAIN_INPUT;

/** Persistent chain configuration and the derived chains for one schematic. */
class NETCHAIN_MANAGER
{
public:
    explicit NETCHAIN_MANAGER( SCHEMATIC* aSchematic ) : m_schematic( aSchematic ) {}

    void Rebuild( const NETCHAIN_INPUT& aConnectivity,
                  const std::function<void( NETCHAIN_MANAGER& )>& aBeforePublish = {} );
    void ClearDerived();
    void RefreshTerminalReferences();
    void Merge( NETCHAIN_MANAGER& aOther );
    void SetSchematic( SCHEMATIC* aSchematic ) { m_schematic = aSchematic; }

    SCH_NETCHAIN* FindPotentialNetChainBetweenPins( SCH_PIN* aPinA, const SCH_SHEET_PATH& aPathA,
                                                   SCH_PIN* aPinB, const SCH_SHEET_PATH& aPathB );
    static wxString NetKeyForItem( const SCH_ITEM& aItem, const SCH_SHEET_PATH& aPath );

    /**
     * Symbols from the last rebuild whose passthrough joins @p aNetKey to another member of
     * @p aChain, keyed to the screen that owns them.  Blocking them detaches the net.
     */
    std::map<SCH_SYMBOL*, SCH_SCREEN*> GetBridgeSymbols( const SCH_NETCHAIN& aChain,
                                                         const wxString& aNetKey ) const;

    SCH_NETCHAIN* GetNetChainForNet( const wxString& aNet );
    SCH_NETCHAIN* GetNetChainByName( const wxString& aName );
    struct TERMINAL_CHANGE
    {
        wxString chain;
        int endpoint = 0;
        KIID pin = niluuid;
        KIID_PATH sheet;
    };

    bool ReplaceNetChainTerminalPin( const TERMINAL_CHANGE& aChange );

    /**
     * Stash per-net-chain netclass overrides read from the schematic file.  These are
     * consumed by RebuildNetChains when committed chains are named: if a chain matches
     * one of the keys, its netclass override is set from the value.
     */
    void SetNetChainNetClassOverrides( const std::map<wxString, wxString>& aOverrides )
    {
        m_netChainNetClassOverrides = aOverrides;
    }

    const std::map<wxString, wxString>& GetNetChainNetClassOverrides() const
    {
        return m_netChainNetClassOverrides;
    }

    struct CHAIN_TERMINAL_REF
    {
        wxString ref;
        wxString pin;
    };
    using CHAIN_TERMINAL_REFS = std::pair<CHAIN_TERMINAL_REF, CHAIN_TERMINAL_REF>;

    void SetNetChainTerminalRefOverrides( const std::map<wxString, CHAIN_TERMINAL_REFS>& aRefs )
    {
        m_netChainTerminalRefOverrides = aRefs;
    }

    const std::map<wxString, CHAIN_TERMINAL_REFS>& GetNetChainTerminalRefOverrides() const
    {
        return m_netChainTerminalRefOverrides;
    }

    void SetNetChainColorOverrides( const std::map<wxString, COLOR4D>& aOverrides )
    {
        m_netChainColorOverrides = aOverrides;
    }

    const std::map<wxString, COLOR4D>& GetNetChainColorOverrides() const
    {
        return m_netChainColorOverrides;
    }

    /**
     * Stash per-chain member-net lists read from the schematic file.  Used by
     * RebuildNetChains to reconstruct manual force-created chains, which have no
     * underlying inferred potential to match against.
     */
    void SetNetChainMemberNetOverrides( const std::map<wxString, std::set<wxString>>& aOverrides )
    {
        m_netChainMemberNetOverrides = aOverrides;
    }

    const std::map<wxString, std::set<wxString>>& GetNetChainMemberNetOverrides() const
    {
        return m_netChainMemberNetOverrides;
    }

    /**
     * Potential net chains are inferred groupings produced by RebuildNetChains() but not
     * yet user-committed. Existing m_committedNetChains now represents only user-created connectivity groups.
     */
    const std::vector<std::unique_ptr<SCH_NETCHAIN>>& GetPotentialNetChains() const { return m_potentialNetChains; }

    /** Promote a potential net chain to an actual user net chain with the provided name. */
    SCH_NETCHAIN* CreateNetChainFromPotential( SCH_NETCHAIN* aPotential, const wxString& aName );

    /**
     * Commit a manually-defined net chain that the inferred-potential pass did not produce.
     *
     * @param aName        Name of the new chain.  Must satisfy SCH_NETCHAIN::IsValidName().
     * @param aSymbols     Symbols that participate in the chain.
     * @param aNets        Member nets of the chain.
     * @param aTerminalPinA First terminal pin KIID.
     * @param aTerminalPinB Second terminal pin KIID.
     * @param aRefA        Reference designator for the first terminal symbol.
     * @param aPinNumA     Pin number for the first terminal pin.
     * @param aRefB        Reference designator for the second terminal symbol.
     * @param aPinNumB     Pin number for the second terminal pin.
     *
     * @return The new committed chain, or nullptr on name collision, name validation
     *         failure, or net-ownership collision with an existing committed chain.
     */
    SCH_NETCHAIN* CreateManualNetChain( const wxString& aName,
                                        const std::set<class SCH_SYMBOL*>& aSymbols,
                                        const std::set<wxString>& aNets,
                                        const KIID& aTerminalPinA, const KIID& aTerminalPinB,
                                        const wxString& aRefA, const wxString& aPinNumA,
                                        const wxString& aRefB, const wxString& aPinNumB );

    /** Return user-created (committed) net chains (legacy accessor retained under net-chain API). */
    const std::vector<std::unique_ptr<SCH_NETCHAIN>>& GetCommittedNetChains() const { return m_committedNetChains; }

    /**
     * Mirror each committed net chain's netclass override into the project NET_SETTINGS as a
     * chain-derived pattern assignment, so SCH_ITEM::GetEffectiveNetClass() resolves the chain's
     * netclass for member nets the same way board_netlist_updater does on the PCB side.  Existing
     * chain-derived assignments are cleared first so removed or renamed chains leave no stale
     * entries.  Synthetic per-run member keys can't be matched against a resolved net name and
     * are skipped, and a chain whose netclass no longer exists is ignored.
     */
    void ApplyNetChainNetclasses();

    /** Returns true once RebuildNetChains() has completed at least once on this graph. */
    bool NetChainsBuilt() const { return m_netChainsBuilt; }

    /**
     * Delete a committed net chain by name.  Clears every net-chain override map
     * entry (netclass, colour, terminal refs, member nets) and the project chain-class
     * assignment, and resets
     * the SetNetChainName marker on every member symbol so the chain is not
     * reapplied on the next RebuildNetChains() pass.
     *
     * @return true if a chain with that name was found and removed.
     */
    bool DeleteCommittedNetChain( const wxString& aName );

    /**
     * Rename a committed net chain.  Re-keys override map entries and the project
     * chain-class assignment from the old name to the new one, and updates every
     * member symbol's net-chain name marker.  Returns false when the new name is empty, the old chain does
     * not exist, or another chain already uses the new name.
     */
    bool RenameCommittedNetChain( const wxString& aOld, const wxString& aNew );

private:
    void rebuild( const NETCHAIN_INPUT& aConnectivity );
    void setSymbolName( SCH_SYMBOL* aSymbol, const wxString& aName );
    bool resolveTerminals( SCH_NETCHAIN& aChain, const CHAIN_TERMINAL_REFS* aSavedRefs = nullptr );
    bool refreshTerminalReferences( SCH_NETCHAIN& aChain );
    void storeTerminalRefs( const SCH_NETCHAIN& aChain );

    /** Resolve a terminal pin UUID on a sheet instance to its chain member key. */
    wxString NetKeyForTerminal( const KIID& aPin, const KIID_PATH& aSheet ) const;

    /** Record the persistable subset of @p aNets as the restore fallback for @p aName. */
    void storeMemberNets( const wxString& aName, const std::set<wxString>& aNets );

    /** Project net settings, or null for staged and temporary managers. */
    std::shared_ptr<NET_SETTINGS> liveNetSettings() const;

    friend class ::CONNECTION_GRAPH;
    friend void ::boost_test_inject_committed_net_chain( CONNECTION_GRAPH&,
                                                        std::unique_ptr<SCH_NETCHAIN> );

    /**
     * Disambiguate the saved (refA.pinA, refB.pinB) terminal pair against the current set of
     * potential net chains.  Returns the potential chain whose net set contains BOTH endpoint
     * nets.  Returning the first match that contains only one net would silently pick the wrong
     * chain when two potentials share an endpoint but differ at the other terminal.  Tested
     * via the boost_test_resolve_potential_chain_by_terminals friend shim.
     */
    static SCH_NETCHAIN* resolvePotentialChainByTerminals(
            const CHAIN_TERMINAL_REFS& aTermRefs,
            const std::map<std::pair<wxString, wxString>, wxString>& aRefPinToNet,
            const std::vector<std::unique_ptr<SCH_NETCHAIN>>& aPotentials,
            const wxString& aChainName );

    static SCH_NETCHAIN* findPotentialChain( const std::vector<std::unique_ptr<SCH_NETCHAIN>>& aPotentials,
                                             const wxString& aNetA, const wxString& aNetB );

    /**
     * Move every net-chain override map entry keyed by @p aOld to @p aNew.
     * Maps that do not contain @p aOld are left untouched, so this is safe to
     * call from any rename path regardless of which overrides exist.
     */
    void rekeyOverrideMaps( const wxString& aOld, const wxString& aNew );

    /**
     * Replace the derived-view payload on @p aTarget with explicitly supplied member nets,
     * and symbols.  Preserves the chain's name and any
     * user-set netclass/color overrides stored on the chain itself.  Empty net names are
     * filtered.  Used by RebuildNetChains to refresh committed chains in place after Reset()
     * has cleared their stale schematic-item pointers.
     */
    void refreshCommittedChainPayload( SCH_NETCHAIN* aTarget, const std::set<wxString>& aNets,
                                       const std::set<class SCH_SYMBOL*>& aSymbols );

    /**
     * Refresh @p aTarget from an inferred potential chain and resync its persisted
     * member-net fallback.
     */
    void refreshCommittedChainFromPotential( SCH_NETCHAIN* aTarget, const SCH_NETCHAIN& aSource );

    // Bridge-graph helper types shared by RebuildNetChains() and FindNetChainPathsBetweenPins().
    // A bridge edge represents a 2-pin passthrough symbol that ties two distinct subgraph nets
    // together; the bridge graph is the adjacency built from the surviving (non-power-touching)
    // edges after the leaf-prune pass.

    struct SHEET_SYMBOLS;

    struct BRIDGE_EDGE
    {
        wxString             a;
        wxString             b;
        class SCH_SYMBOL*    sym;
        SCH_SCREEN*          screen;
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
    BRIDGE_GRAPH buildBridgeAdjacency( const std::vector<SHEET_SYMBOLS>& aSheets );

    std::vector<std::unique_ptr<SCH_NETCHAIN>> m_committedNetChains;
    std::vector<std::unique_ptr<SCH_NETCHAIN>> m_potentialNetChains; ///< last built potential (uncommitted) net chains
    std::vector<BRIDGE_EDGE>                   m_bridgeEdges;        ///< raw bridge edges from the last rebuild
    bool                                       m_netChainsBuilt = false;
    std::map<wxString, wxString>              m_netChainNetClassOverrides;
    std::map<wxString, COLOR4D>               m_netChainColorOverrides;
    std::map<wxString, CHAIN_TERMINAL_REFS>    m_netChainTerminalRefOverrides;
    std::map<wxString, std::set<wxString>>    m_netChainMemberNetOverrides;

    std::unordered_map<SCH_SYMBOL*, wxString>* m_pendingSymbolNames = nullptr;
    SCHEMATIC* m_schematic;
};
} // namespace SCH_CONNECTIVITY
