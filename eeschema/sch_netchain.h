/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SCH_NETCHAIN_H
#define SCH_NETCHAIN_H

#include <set>
#include <utility>
#include <vector>
#include <wx/string.h>
#include <gal/color4d.h>
#include <kiid.h>

class CONNECTION_GRAPH;

/**
 * A net chain is a collection of nets that are connected together through
 * passive components.
 */
class SCH_NETCHAIN
{
public:
    /// Prefix used when synthesising net names for unnamed subgraphs.  Such names
    /// embed the per-run subgraph code and so are not stable across reloads.
    static constexpr char SYNTHETIC_NET_PREFIX[] = "__SG_";

    SCH_NETCHAIN() {}

    void SetName( const wxString& aName ) { m_name = aName; }
    const wxString& GetName() const { return m_name; }

    static bool IsValidName( const wxString& aName )
    {
        if( aName.IsEmpty() )
            return false;

        for( wxUniChar c : aName )
            if( c == '"' || c == '\'' || c == '(' || c == ')' || c == ' ' )
                return false;

        return true;
    }

    void AddNet( const wxString& aNet )
    {
        m_nets.insert( aNet );
        m_orderedNetsDirty = true;
    }

    void RemoveNet( const wxString& aNet )
    {
        m_nets.erase( aNet );
        m_orderedNetsDirty = true;
    }

    void ReplaceNets( const std::set<wxString>& aNew )
    {
        m_nets = aNew;
        m_orderedNetsDirty = true;
    }

    // Track a symbol that participates in this chain (2-pin passthrough component).
    void AddSymbol( class SCH_SYMBOL* aSymbol )
    {
        m_symbols.insert( aSymbol );
        m_orderedNetsDirty = true;
    }

    const std::set<class SCH_SYMBOL*>& GetSymbols() const { return m_symbols; }

    void AbsorbSymbolsFrom( const SCH_NETCHAIN& aOther )
    {
        m_symbols.insert( aOther.m_symbols.begin(), aOther.m_symbols.end() );
        m_orderedNetsDirty = true;
    }

    // The symbol set holds non-owning raw pointers to schematic items. Callers must invoke
    // this before any pass that may free those items (e.g. CONNECTION_GRAPH::Reset()).
    void ClearSymbols()
    {
        m_symbols.clear();
        m_orderedNetsDirty = true;
    }

    const std::set<wxString>& GetNets() const { return m_nets; }

    /**
     * Return the chain's member nets ordered from terminal pin A's net to terminal pin
     * B's net along the shortest bridge-graph path; any off-path member nets (branches)
     * are appended alphabetically.  Empty if fewer than two nets, or if the terminal
     * pins are unset / cannot be resolved on @p aGraph.  The returned vector is cached
     * and invalidated by any mutator on this class.
     *
     * @param aGraph live connection graph the chain belongs to; required to resolve
     *               pin KIIDs to nets.  Passing nullptr returns an empty vector
     *               without caching.
     */
    const std::vector<wxString>& GetOrderedNets( CONNECTION_GRAPH* aGraph ) const;

    /**
     * Resolve terminal pin @p aIdx (0 or 1) to the net-chain key of its owning subgraph
     * via @p aGraph.  Returns an empty string if the pin can't be located (e.g. the
     * KIID refers to a removed item) or if @p aGraph is null.
     */
    wxString GetTerminalNetName( int aIdx, CONNECTION_GRAPH* aGraph ) const;

    void SetTerminalPins( const KIID& aPinA, const KIID& aPinB )
    {
        m_terminalPins[0] = aPinA;
        m_terminalPins[1] = aPinB;
        m_orderedNetsDirty = true;
    }

    const KIID& GetTerminalPinA() const { return m_terminalPins[0]; }
    const KIID& GetTerminalPinB() const { return m_terminalPins[1]; }

    void ReplaceTerminalPin( const KIID& aPrev, const KIID& aNew )
    {
        if( m_terminalPins[0] == aPrev )
            m_terminalPins[0] = aNew;
        else if( m_terminalPins[1] == aPrev )
            m_terminalPins[1] = aNew;
        else
            return;

        m_orderedNetsDirty = true;
    }

    /**
     * Net chains may override the netclass applied to every member net.
     * Empty string means "do not override".  The active netclass is
     * propagated to every member when the PCB is updated from the netlist.
     */
    void SetNetClass( const wxString& aNetClass ) { m_netClass = aNetClass; }
    const wxString& GetNetClass() const { return m_netClass; }

    /**
     * Optional display color for the chain.  When set to an opaque colour,
     * the PCB and schematic painters prefer it over the default chain
     * highlight emphasis.  UNSPECIFIED / fully-transparent means "use the
     * default scheme".
     */
    void SetColor( const KIGFX::COLOR4D& aColor ) { m_color = aColor; }
    const KIGFX::COLOR4D& GetColor() const { return m_color; }

    void SetTerminalRefs( const wxString& aRefA, const wxString& aPinA, const wxString& aRefB, const wxString& aPinB )
    {
        m_terminalRef[0] = aRefA;
        m_terminalPinNum[0] = aPinA;
        m_terminalRef[1] = aRefB;
        m_terminalPinNum[1] = aPinB;
    }

    const wxString& GetTerminalRef( int aIdx ) const
    {
        wxASSERT( aIdx >= 0 && aIdx < 2 );
        return m_terminalRef[aIdx];
    }

    const wxString& GetTerminalPinNum( int aIdx ) const
    {
        wxASSERT( aIdx >= 0 && aIdx < 2 );
        return m_terminalPinNum[aIdx];
    }

private:
    wxString                          m_name;
    std::set<wxString>                m_nets;
    std::set<class SCH_SYMBOL*>       m_symbols;
    KIID                              m_terminalPins[2];
    wxString                          m_terminalRef[2];
    wxString                          m_terminalPinNum[2];
    wxString                          m_netClass;
    KIGFX::COLOR4D                    m_color = KIGFX::COLOR4D::UNSPECIFIED;

    // Cached topologically ordered net list.  Populated lazily by GetOrderedNets()
    // and invalidated by any topology-mutating accessor above.
    mutable std::vector<wxString>     m_orderedNets;
    mutable bool                      m_orderedNetsDirty = true;
};

#endif
