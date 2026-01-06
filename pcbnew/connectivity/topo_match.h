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
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef __TOPO_MATCH_H
#define __TOPO_MATCH_H

#include <vector>
#include <map>
#include <optional>

#include <wx/string.h>

class FOOTPRINT;

/* A very simple (but working) partial connection graph isomorphism algorithm,
   operating on sets of footprints and connections between their pads.
*/

namespace TMATCH
{

class PIN;
class CONNECTION_GRAPH;

struct TOPOLOGY_MISMATCH_REASON
{
    wxString m_reference;
    wxString m_candidate;
    wxString m_reason;
};

class COMPONENT
{
    friend class PIN;
    friend class CONNECTION_GRAPH;

public:
    COMPONENT( const wxString& aRef, FOOTPRINT* aParentFp, std::optional<VECTOR2I> aRaOffset = std::optional<VECTOR2I>() );
    ~COMPONENT();

    bool               IsSameKind( const COMPONENT& b ) const;
    void               AddPin( PIN* p );
    int                GetPinCount() const { return m_pins.size(); }
    bool               MatchesWith( COMPONENT* b, TOPOLOGY_MISMATCH_REASON& aDetail );
    std::vector<PIN*>& Pins() { return m_pins; }
    FOOTPRINT* GetParent() const { return m_parentFootprint; }

    bool HasRAOffset() const { return m_raOffset.has_value(); }
    const VECTOR2I GetRAOffset() const { return *m_raOffset; }

private:
    void sortPinsByName();


    std::optional<VECTOR2I> m_raOffset;
    wxString          m_reference;
    wxString          m_prefix;
    FOOTPRINT*        m_parentFootprint = nullptr;
    std::vector<PIN*> m_pins;
};

class PIN
{
    friend class CONNECTION_GRAPH;

public:
    PIN() : m_netcode( 0 ), m_parent( nullptr ) {}
    ~PIN() {}

    void SetParent( COMPONENT* parent ) { m_parent = parent; }

    const wxString Format() const { return m_parent->m_reference + wxT( "-" ) + m_ref; }

    void AddConnection( PIN* pin ) { m_conns.push_back( pin ); }

    bool IsTopologicallySimilar( const PIN& b ) const
    {
        wxASSERT( m_parent != b.m_parent );

        if( !m_parent->IsSameKind( *b.m_parent ) )
            return false;

        return m_ref == b.m_ref;
    }

    bool IsIsomorphic( const PIN& b, TOPOLOGY_MISMATCH_REASON& aDetail ) const;

    int GetNetCode() const { return m_netcode; }

    const wxString& GetReference() const { return m_ref; }

    COMPONENT* GetParent() const { return m_parent; }

private:

    wxString          m_ref;
    int               m_netcode;
    COMPONENT*        m_parent;
    std::vector<PIN*> m_conns;
};

class BACKTRACK_STAGE
{
    friend class CONNECTION_GRAPH;

public:
    BACKTRACK_STAGE()
    {
        m_ref = nullptr;
        m_currentMatch = -1;
        m_nloops = 0;
        m_refIndex = 0;
    }

    BACKTRACK_STAGE( const BACKTRACK_STAGE& other )
    {
        m_currentMatch = other.m_currentMatch;
        m_ref = other.m_ref;
        m_matches = other.m_matches;
        m_locked = other.m_locked;
        m_nloops = other.m_nloops;
        m_refIndex = other.m_refIndex;
    }

    const std::map<COMPONENT*, COMPONENT*>& GetMatchingComponentPairs() const { return m_locked; }

private:
    COMPONENT*                       m_ref;
    int                              m_currentMatch = -1;
    int                              m_nloops;
    std::vector<COMPONENT*>          m_matches;
    std::map<COMPONENT*, COMPONENT*> m_locked;
    int m_refIndex;
};

typedef std::map<FOOTPRINT*, FOOTPRINT*> COMPONENT_MATCHES;

class CONNECTION_GRAPH
{
public:
    const int c_ITER_LIMIT = 10000;

    CONNECTION_GRAPH();
    ~CONNECTION_GRAPH();

    void   BuildConnectivity();
    void   AddFootprint( FOOTPRINT* aFp, const VECTOR2I& aOffset );
    bool   FindIsomorphism( CONNECTION_GRAPH* target, COMPONENT_MATCHES& result,
                            std::vector<TOPOLOGY_MISMATCH_REASON>& aFailureDetails );
    static std::unique_ptr<CONNECTION_GRAPH> BuildFromFootprintSet( const std::set<FOOTPRINT*>& aFps );
    std::vector<COMPONENT*> &Components() { return m_components; }

private:
    void sortByPinCount();


    std::vector<COMPONENT*> findMatchingComponents( CONNECTION_GRAPH* aRefGraph,
                                                    COMPONENT*             ref,
                                                    const BACKTRACK_STAGE& partialMatches,
                                                    std::vector<TOPOLOGY_MISMATCH_REASON>& aFailureDetails );

    std::vector<COMPONENT*> m_components;

};

}; // namespace TMATCH

#endif
