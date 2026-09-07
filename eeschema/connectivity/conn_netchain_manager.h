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
/** Committed net chains and overrides.  Staged and temporary graphs own a private instance. */
class NETCHAIN_MANAGER
{
public:
    explicit NETCHAIN_MANAGER( SCHEMATIC* aSchematic ) :
            m_schematic( aSchematic )
    {
    }

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

private:
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

    SCHEMATIC* m_schematic;
};
} // namespace SCH_CONNECTIVITY
