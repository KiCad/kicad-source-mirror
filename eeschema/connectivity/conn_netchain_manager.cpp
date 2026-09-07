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

#include "conn_netchain_manager.h"
#include <algorithm>
#include <sch_symbol.h>
#include <schematic.h>
#include <project.h>
#include <project/project_file.h>
#include <project/net_settings.h>
#include <trace_helpers.h>
#include <wx/log.h>


SCH_NETCHAIN* SCH_CONNECTIVITY::NETCHAIN_MANAGER::resolvePotentialChainByTerminals(
        const CHAIN_TERMINAL_REFS& aTermRefs, const std::map<std::pair<wxString, wxString>, wxString>& aRefPinToNet,
        const std::vector<std::unique_ptr<SCH_NETCHAIN>>& aPotentials, const wxString& aChainName )
{
    auto itFrom = aRefPinToNet.find( { aTermRefs.first.ref, aTermRefs.first.pin } );
    auto itTo = aRefPinToNet.find( { aTermRefs.second.ref, aTermRefs.second.pin } );

    if( itFrom == aRefPinToNet.end() || itTo == aRefPinToNet.end() )
    {
        wxLogTrace( traceSchNetChain, "RebuildNetChains: cannot restore chain '%s' (terminal %s.%s/%s.%s unresolved)",
                    aChainName, aTermRefs.first.ref, aTermRefs.first.pin, aTermRefs.second.ref, aTermRefs.second.pin );
        return nullptr;
    }

    for( const auto& pot : aPotentials )
    {
        if( pot && pot->GetNets().count( itFrom->second ) && pot->GetNets().count( itTo->second ) )
            return pot.get();
    }

    wxLogTrace( traceSchNetChain, "RebuildNetChains: no potential chain spans both terminals of '%s' (%s/%s)",
                aChainName, itFrom->second, itTo->second );
    return nullptr;
}


bool SCH_CONNECTIVITY::NETCHAIN_MANAGER::DeleteCommittedNetChain( const wxString& aName )
{
    if( aName.IsEmpty() )
        return false;

    auto it = std::find_if( m_committedNetChains.begin(), m_committedNetChains.end(),
                            [&]( const std::unique_ptr<SCH_NETCHAIN>& aChain )
                            {
                                return aChain && aChain->GetName() == aName;
                            } );

    if( it == m_committedNetChains.end() )
        return false;

    // Otherwise RebuildNetChains() re-promotes these symbols under the deleted name
    for( SCH_SYMBOL* sym : ( *it )->GetSymbols() )
    {
        if( sym )
            sym->SetNetChainName( wxEmptyString );
    }

    m_committedNetChains.erase( it );

    m_netChainNetClassOverrides.erase( aName );
    m_netChainColorOverrides.erase( aName );
    m_netChainTerminalRefOverrides.erase( aName );
    m_netChainTerminalOverrides.erase( aName );
    m_netChainMemberNetOverrides.erase( aName );

    return true;
}


bool SCH_CONNECTIVITY::NETCHAIN_MANAGER::RenameCommittedNetChain( const wxString& aOld, const wxString& aNew )
{
    if( aOld.IsEmpty() || aNew.IsEmpty() || aOld == aNew )
        return false;

    auto findByName = [&]( const wxString& aName ) -> SCH_NETCHAIN*
    {
        for( const std::unique_ptr<SCH_NETCHAIN>& chain : m_committedNetChains )
        {
            if( chain && chain->GetName() == aName )
                return chain.get();
        }

        return nullptr;
    };

    SCH_NETCHAIN* existing = findByName( aOld );

    if( !existing )
        return false;

    if( findByName( aNew ) )
        return false;

    existing->SetName( aNew );

    for( SCH_SYMBOL* sym : existing->GetSymbols() )
    {
        if( sym )
            sym->SetNetChainName( aNew );
    }

    rekeyOverrideMaps( aOld, aNew );

    return true;
}


void SCH_CONNECTIVITY::NETCHAIN_MANAGER::rekeyOverrideMaps( const wxString& aOld, const wxString& aNew )
{
    if( aOld == aNew )
        return;

    auto rekey = [&]( auto& aMap )
    {
        auto it = aMap.find( aOld );

        if( it != aMap.end() )
        {
            auto val = std::move( it->second );
            aMap.erase( it );
            aMap[aNew] = std::move( val );
        }
    };

    rekey( m_netChainNetClassOverrides );
    rekey( m_netChainColorOverrides );
    rekey( m_netChainTerminalRefOverrides );
    rekey( m_netChainTerminalOverrides );
    rekey( m_netChainMemberNetOverrides );
}


void SCH_CONNECTIVITY::NETCHAIN_MANAGER::refreshCommittedChainPayload(
        SCH_NETCHAIN* aTarget, const std::set<wxString>& aNets, const std::set<SCH_SYMBOL*>& aSymbols,
        const KIID& aTerminalPinA, const KIID& aTerminalPinB, const wxString& aRefA, const wxString& aPinNumA,
        const wxString& aRefB, const wxString& aPinNumB )
{
    if( !aTarget )
        return;

    std::set<wxString> filtered;

    for( const wxString& net : aNets )
    {
        if( !net.IsEmpty() )
            filtered.insert( net );
    }

    aTarget->ReplaceNets( filtered );

    aTarget->ClearSymbols();

    for( SCH_SYMBOL* sym : aSymbols )
        aTarget->AddSymbol( sym );

    // Keep a user-retargeted terminal across an unconditional Recalculate
    auto termOverride = m_netChainTerminalOverrides.find( aTarget->GetName() );

    if( termOverride != m_netChainTerminalOverrides.end() )
        aTarget->SetTerminalPins( termOverride->second.first, termOverride->second.second );
    else
        aTarget->SetTerminalPins( aTerminalPinA, aTerminalPinB );

    aTarget->SetTerminalRefs( aRefA, aPinNumA, aRefB, aPinNumB );

    for( SCH_SYMBOL* sym : aTarget->GetSymbols() )
        sym->SetNetChainName( aTarget->GetName() );
}


void SCH_CONNECTIVITY::NETCHAIN_MANAGER::refreshCommittedChainFromPotential( SCH_NETCHAIN*       aTarget,
                                                                             const SCH_NETCHAIN& aSource )
{
    refreshCommittedChainPayload( aTarget, aSource.GetNets(), aSource.GetSymbols(), aSource.GetTerminalPinA(),
                                  aSource.GetTerminalPinB(), aSource.GetTerminalRef( 0 ),
                                  aSource.GetTerminalPinNum( 0 ), aSource.GetTerminalRef( 1 ),
                                  aSource.GetTerminalPinNum( 1 ) );
}


SCH_NETCHAIN* SCH_CONNECTIVITY::NETCHAIN_MANAGER::CreateNetChainFromPotential( SCH_NETCHAIN*   aPotential,
                                                                               const wxString& aName )
{
    if( !aPotential )
        return nullptr;
    auto sig = std::make_unique<SCH_NETCHAIN>();
    for( const wxString& n : aPotential->GetNets() )
        sig->AddNet( n );
    for( SCH_SYMBOL* sym : aPotential->GetSymbols() )
        sig->AddSymbol( sym );
    sig->SetName( aName );
    sig->SetTerminalPins( aPotential->GetTerminalPinA(), aPotential->GetTerminalPinB() );
    sig->SetTerminalRefs( aPotential->GetTerminalRef( 0 ), aPotential->GetTerminalPinNum( 0 ),
                          aPotential->GetTerminalRef( 1 ), aPotential->GetTerminalPinNum( 1 ) );

    auto ncIt = m_netChainNetClassOverrides.find( aName );

    if( ncIt != m_netChainNetClassOverrides.end() )
        sig->SetNetClass( ncIt->second );

    auto colIt = m_netChainColorOverrides.find( aName );

    if( colIt != m_netChainColorOverrides.end() )
        sig->SetColor( colIt->second );

    for( SCH_SYMBOL* sym : sig->GetSymbols() )
        sym->SetNetChainName( sig->GetName() );

    // The restore pass after an unconditional Recalculate finds chains only through these maps
    CHAIN_TERMINAL_REFS termRefs{ { aPotential->GetTerminalRef( 0 ), aPotential->GetTerminalPinNum( 0 ) },
                                  { aPotential->GetTerminalRef( 1 ), aPotential->GetTerminalPinNum( 1 ) } };
    m_netChainTerminalRefOverrides[aName] = termRefs;

    // Restore fallback if the topology shifts, filtered to match what the s-expr writer saves
    std::set<wxString> persistableNets;

    for( const wxString& net : sig->GetNets() )
    {
        if( net.IsEmpty() )
            continue;

        if( net.StartsWith( SCH_NETCHAIN::SYNTHETIC_NET_PREFIX ) )
            continue;

        persistableNets.insert( net );
    }

    if( !persistableNets.empty() )
        m_netChainMemberNetOverrides[aName] = std::move( persistableNets );
    else
        m_netChainMemberNetOverrides.erase( aName );

    SCH_NETCHAIN* raw = sig.get();
    m_committedNetChains.push_back( std::move( sig ) );
    return raw;
}


SCH_NETCHAIN* SCH_CONNECTIVITY::NETCHAIN_MANAGER::CreateManualNetChain(
        const wxString& aName, const std::set<SCH_SYMBOL*>& aSymbols, const std::set<wxString>& aNets,
        const KIID& aTerminalPinA, const KIID& aTerminalPinB, const wxString& aRefA, const wxString& aPinNumA,
        const wxString& aRefB, const wxString& aPinNumB )
{
    if( !SCH_NETCHAIN::IsValidName( aName ) )
        return nullptr;

    if( GetNetChainByName( aName ) )
        return nullptr;

    // GetNetChainForNet returns the first match, so a net may belong to only one chain
    for( const wxString& net : aNets )
    {
        if( net.IsEmpty() )
            continue;

        if( GetNetChainForNet( net ) )
            return nullptr;
    }

    auto sig = std::make_unique<SCH_NETCHAIN>();
    sig->SetName( aName );

    for( const wxString& net : aNets )
    {
        if( net.IsEmpty() )
            continue;

        sig->AddNet( net );
    }

    for( SCH_SYMBOL* sym : aSymbols )
        sig->AddSymbol( sym );

    sig->SetTerminalPins( aTerminalPinA, aTerminalPinB );
    sig->SetTerminalRefs( aRefA, aPinNumA, aRefB, aPinNumB );

    auto ncIt = m_netChainNetClassOverrides.find( aName );

    if( ncIt != m_netChainNetClassOverrides.end() )
        sig->SetNetClass( ncIt->second );

    auto colIt = m_netChainColorOverrides.find( aName );

    if( colIt != m_netChainColorOverrides.end() )
        sig->SetColor( colIt->second );

    for( SCH_SYMBOL* sym : sig->GetSymbols() )
        sym->SetNetChainName( sig->GetName() );

    // The restore pass after an unconditional Recalculate finds chains only through these maps
    CHAIN_TERMINAL_REFS termRefs{ { aRefA, aPinNumA }, { aRefB, aPinNumB } };
    m_netChainTerminalRefOverrides[aName] = termRefs;
    m_netChainMemberNetOverrides[aName] = sig->GetNets();

    SCH_NETCHAIN* raw = sig.get();
    m_committedNetChains.push_back( std::move( sig ) );
    return raw;
}


SCH_NETCHAIN* SCH_CONNECTIVITY::NETCHAIN_MANAGER::GetNetChainForNet( const wxString& aNet )
{
    wxLogTrace( traceSchNetChain, "SCH_CONNECTIVITY::NETCHAIN_MANAGER::GetNetChainForNet(%s)", aNet );
    for( std::unique_ptr<SCH_NETCHAIN>& sig : m_committedNetChains )
    {
        if( !sig )
            continue;

        if( sig->GetNets().count( aNet ) )
        {
            wxLogTrace( traceSchNetChain, "GetNetChainForNet: found chain '%s'", sig->GetName() );
            return sig.get();
        }
    }

    wxLogTrace( traceSchNetChain, "GetNetChainForNet: no chain found" );
    return nullptr;
}


void SCH_CONNECTIVITY::NETCHAIN_MANAGER::ApplyNetChainNetclasses()
{
    // Staged and temporary graphs must not publish project netclass assignments
    if( !m_schematic || this != &m_schematic->NetChains() )
        return;

    std::shared_ptr<NET_SETTINGS> netSettings = m_schematic->Project().GetProjectFile().NetSettings();

    if( !netSettings )
        return;

    bool anyOverride = std::any_of( m_committedNetChains.begin(), m_committedNetChains.end(),
                                    []( const std::unique_ptr<SCH_NETCHAIN>& aChain )
                                    {
                                        return aChain && !aChain->GetNetClass().IsEmpty();
                                    } );

    // Leave the effective-netclass cache alone on chainless rebuilds
    if( !anyOverride && !netSettings->HasChainPatternAssignments( NET_CHAIN_SOURCE::SCHEMATIC ) )
        return;

    netSettings->ClearChainPatternAssignments( NET_CHAIN_SOURCE::SCHEMATIC );

    for( const std::unique_ptr<SCH_NETCHAIN>& chain : m_committedNetChains )
    {
        if( !chain )
            continue;

        const wxString& netclass = chain->GetNetClass();

        if( netclass.IsEmpty() || !netSettings->HasNetclass( netclass ) )
            continue;

        for( const wxString& net : chain->GetNets() )
        {
            // Synthetic per-run keys embed a subgraph code and never match a resolved net name
            if( net.StartsWith( SCH_NETCHAIN::SYNTHETIC_NET_PREFIX ) )
                continue;

            netSettings->SetChainPatternAssignment( NET_CHAIN_SOURCE::SCHEMATIC, net, netclass );
        }
    }
}


SCH_NETCHAIN* SCH_CONNECTIVITY::NETCHAIN_MANAGER::GetNetChainByName( const wxString& aName )
{
    wxLogTrace( traceSchNetChain, "SCH_CONNECTIVITY::NETCHAIN_MANAGER::GetNetChainByName(%s)", aName );
    for( std::unique_ptr<SCH_NETCHAIN>& sig : m_committedNetChains )
    {
        if( sig->GetName() == aName )
        {
            wxLogTrace( traceSchNetChain, "GetNetChainByName: found" );
            return sig.get();
        }
    }

    wxLogTrace( traceSchNetChain, "GetNetChainByName: not found" );
    return nullptr;
}


void SCH_CONNECTIVITY::NETCHAIN_MANAGER::ReplaceNetChainTerminalPin( const wxString& aNetChain, const KIID& aPrev,
                                                                     const KIID& aNew )
{
    wxLogTrace( traceSchNetChain, "ReplaceNetChainTerminalPin: chain='%s' prev=%s new=%s", aNetChain, aPrev.AsString(),
                aNew.AsString() );
    if( SCH_NETCHAIN* sig = GetNetChainByName( aNetChain ) )
    {
        sig->ReplaceTerminalPin( aPrev, aNew );
        m_netChainTerminalOverrides[aNetChain] = std::make_pair( sig->GetTerminalPinA(), sig->GetTerminalPinB() );
        wxLogTrace( traceSchNetChain, "ReplaceNetChainTerminalPin: updated overrides to (%s,%s)",
                    sig->GetTerminalPinA().AsString(), sig->GetTerminalPinB().AsString() );
    }
}


void SCH_CONNECTIVITY::NETCHAIN_MANAGER::SetNetChainTerminalOverrides(
        const std::map<wxString, std::pair<KIID, KIID>>& aOverrides )
{
    m_netChainTerminalOverrides = aOverrides;
    wxLogTrace( traceSchNetChain, "SetNetChainTerminalOverrides: count=%zu", m_netChainTerminalOverrides.size() );
}
