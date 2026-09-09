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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SCH_NETCHAIN_H
#define SCH_NETCHAIN_H

#include <cstdint>
#include <set>
#include <type_traits>
#include <utility>
#include <wx/string.h>
#include <gal/color4d.h>
#include <kiid.h>

/**
 * A net chain is a collection of nets that are connected together through
 * passive components.
 */
class SCH_NETCHAIN
{
public:
    /// Prefix used when synthesising net names for unnamed subgraphs.  Such names
    /// embed a per-run component identity and so are not stable across reloads.
    static constexpr wxStringCharType SYNTHETIC_NET_PREFIX[] = wxS( "__SG_" );

    static wxString MakeKey( const wxString& aName, uint32_t aComponent );

    /// Synthetic keys do not survive a reload, so only named nets are written out.
    static bool IsPersistableNet( const wxString& aNet )
    {
        return !aNet.IsEmpty() && !aNet.StartsWith( SYNTHETIC_NET_PREFIX );
    }

    SCH_NETCHAIN() {}

    friend void swap( SCH_NETCHAIN& aLeft, SCH_NETCHAIN& aRight ) noexcept
    {
        const auto swapMember = []<typename T>( T& a, T& b ) noexcept
        {
            static_assert( std::is_nothrow_swappable_v<T> );
            using std::swap;
            swap( a, b );
        };

        swapMember( aLeft.m_name, aRight.m_name );
        swapMember( aLeft.m_nets, aRight.m_nets );
        swapMember( aLeft.m_symbols, aRight.m_symbols );
        swapMember( aLeft.m_terminalPins, aRight.m_terminalPins );
        swapMember( aLeft.m_terminalPaths, aRight.m_terminalPaths );
        swapMember( aLeft.m_terminalRef, aRight.m_terminalRef );
        swapMember( aLeft.m_terminalPinNum, aRight.m_terminalPinNum );
        swapMember( aLeft.m_netClass, aRight.m_netClass );
        swapMember( aLeft.m_color, aRight.m_color );
    }

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
    }

    void RemoveNet( const wxString& aNet )
    {
        m_nets.erase( aNet );
    }

    void ReplaceNets( const std::set<wxString>& aNew )
    {
        m_nets = aNew;
    }

    // Track a symbol that participates in this chain (2-pin passthrough component).
    void AddSymbol( class SCH_SYMBOL* aSymbol )
    {
        m_symbols.insert( aSymbol );
    }

    const std::set<class SCH_SYMBOL*>& GetSymbols() const { return m_symbols; }

    void AbsorbSymbolsFrom( const SCH_NETCHAIN& aOther )
    {
        m_symbols.insert( aOther.m_symbols.begin(), aOther.m_symbols.end() );
    }

    // The symbol set holds non-owning raw pointers to schematic items. Callers must invoke
    // this before any pass that may free those items.
    void ClearSymbols()
    {
        m_symbols.clear();
    }

    const std::set<wxString>& GetNets() const { return m_nets; }

    void SetTerminalPins( const KIID& aPinA, const KIID& aPinB )
    {
        m_terminalPins[0] = aPinA;
        m_terminalPins[1] = aPinB;
    }

    const KIID& GetTerminalPinA() const { return m_terminalPins[0]; }
    const KIID& GetTerminalPinB() const { return m_terminalPins[1]; }

    const KIID_PATH& GetTerminalPath( int aIdx ) const { return m_terminalPaths[aIdx]; }

    void SetTerminalPaths( const KIID_PATH& aPathA, const KIID_PATH& aPathB )
    {
        m_terminalPaths[0] = aPathA;
        m_terminalPaths[1] = aPathB;
    }

    bool IsTerminal( const KIID& aPin, const KIID_PATH& aSheet ) const
    {
        return ( m_terminalPins[0] == aPin && m_terminalPaths[0] == aSheet )
               || ( m_terminalPins[1] == aPin && m_terminalPaths[1] == aSheet );
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
    KIID_PATH                         m_terminalPaths[2];
    wxString                          m_terminalRef[2];
    wxString                          m_terminalPinNum[2];
    wxString                          m_netClass;
    KIGFX::COLOR4D                    m_color = KIGFX::COLOR4D::UNSPECIFIED;
};

#endif
