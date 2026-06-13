/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef LIB_MERGE_APPLIER_H
#define LIB_MERGE_APPLIER_H

#include <diff_merge/kicad_diff_types.h>
#include <diff_merge/kicad_merge_engine.h>

#include <trace_helpers.h>

#include <map>
#include <memory>
#include <set>
#include <vector>

#include <wx/log.h>
#include <wx/string.h>


namespace KICAD_DIFF
{

/**
 * Materialize a MERGE_PLAN into a merged name-keyed library.
 *
 * Library identity is the canonical item name (LIB_SYMBOL or FOOTPRINT).
 * Plan IDs come from `LibraryItemKiidPath(name)` on the differ side; the
 * applier reconstructs the same KIID_PATH from each input map's keys and
 * looks up matching resolutions.
 *
 * Per-property merge (MERGE_PROPS) at the item level is not implemented;
 * such resolutions are downgraded to TAKE_OURS and counted in
 * `mergePropsFallback` so the job handler can surface them as unresolved
 * to the user.
 */
template <typename ITEM>
class LIB_MERGE_APPLIER
{
public:
    using ITEM_MAP = std::map<wxString, const ITEM*>;

    LIB_MERGE_APPLIER( const ITEM_MAP& aAncestor, const ITEM_MAP& aOurs, const ITEM_MAP& aTheirs, MERGE_PLAN aPlan ) :
            m_ancestor( aAncestor ),
            m_ours( aOurs ),
            m_theirs( aTheirs ),
            m_plan( std::move( aPlan ) )
    {
    }

    struct REPORT
    {
        std::size_t itemsTakenOurs = 0;
        std::size_t itemsTakenTheirs = 0;
        std::size_t itemsTakenAncestor = 0;
        std::size_t itemsDeleted = 0;
        std::size_t itemsKept = 0;
        std::size_t mergePropsFallback = 0;

        /// Item IDs (KIID_PATH from LibraryItemKiidPath) for resolutions that
        /// the applier silently downgraded (currently: MERGE_PROPS -> TAKE_
        /// OURS). The job handler folds these into the unresolved set so the
        /// reported conflict count reflects each affected item.
        std::vector<KIID_PATH> mergePropsFallbackIds;
    };

    std::vector<std::unique_ptr<ITEM>> Apply()
    {
        m_report = {};

        std::map<wxString, const ITEM*> live = m_ancestor;

        std::map<KIID_PATH, const ITEM_RESOLUTION*> actionsById;

        for( const ITEM_RESOLUTION& res : m_plan.actions )
            actionsById[res.id] = &res;

        const std::set<wxString> allNames = collectNames( m_ancestor, m_ours, m_theirs );

        for( const wxString& name : allNames )
        {
            auto resIt = actionsById.find( LibraryItemKiidPath( name ) );

            if( resIt == actionsById.end() )
                continue;

            const ITEM_RESOLUTION& res = *resIt->second;

            switch( res.kind )
            {
            case ITEM_RES::TAKE_OURS: takeFrom( m_ours, name, live, m_report.itemsTakenOurs ); break;
            case ITEM_RES::TAKE_THEIRS: takeFrom( m_theirs, name, live, m_report.itemsTakenTheirs ); break;
            case ITEM_RES::TAKE_ANCESTOR: takeFrom( m_ancestor, name, live, m_report.itemsTakenAncestor ); break;

            case ITEM_RES::DELETE_ITEM:
                live.erase( name );
                ++m_report.itemsDeleted;
                break;

            case ITEM_RES::KEEP:
            {
                // KEEP is emitted for unresolved both-added collisions (and
                // similar) where the engine wants the applier to do something
                // safe. Walk ancestor -> ours -> theirs so an independent
                // same-name add on both sides doesn't silently vanish.
                const ITEM* src = findItem( m_ancestor, name );

                if( !src )
                    src = findItem( m_ours, name );

                if( !src )
                    src = findItem( m_theirs, name );

                if( src )
                    live[name] = src;
                else
                    live.erase( name );

                ++m_report.itemsKept;
                break;
            }

            case ITEM_RES::MERGE_PROPS:
                // Per-property library-item merge would have to splice pins,
                // fields, primitives, parent weak_ptr graphs — out of scope.
                // Take ours so the chosen side at least lands intact, and
                // surface the count + ID so the handler can rebuild the
                // marker plan with this item flagged.
                takeFrom( m_ours, name, live, m_report.mergePropsFallback );
                m_report.mergePropsFallbackIds.push_back( res.id );
                wxLogTrace( traceDiffMerge,
                            wxT( "lib_merge_applier: MERGE_PROPS for '%s' fell back to "
                                 "TAKE_OURS" ),
                            name );
                break;
            }
        }

        std::vector<std::unique_ptr<ITEM>> out;
        out.reserve( live.size() );

        for( const auto& [name, item] : live )
        {
            if( item )
                out.emplace_back( std::make_unique<ITEM>( *item ) );
        }

        return out;
    }

    const REPORT& GetReport() const { return m_report; }

private:
    static const ITEM* findItem( const ITEM_MAP& aMap, const wxString& aName )
    {
        auto it = aMap.find( aName );
        return it == aMap.end() ? nullptr : it->second;
    }

    static std::set<wxString> collectNames( const ITEM_MAP& aAncestor, const ITEM_MAP& aOurs, const ITEM_MAP& aTheirs )
    {
        std::set<wxString> out;

        for( const auto& [name, _] : aAncestor )
            out.insert( name );
        for( const auto& [name, _] : aOurs )
            out.insert( name );
        for( const auto& [name, _] : aTheirs )
            out.insert( name );

        return out;
    }

    static void takeFrom( const ITEM_MAP& aSource, const wxString& aName, std::map<wxString, const ITEM*>& aLive,
                          std::size_t& aCounter )
    {
        if( const ITEM* src = findItem( aSource, aName ) )
            aLive[aName] = src;
        else
            aLive.erase( aName );

        ++aCounter;
    }

    const ITEM_MAP& m_ancestor;
    const ITEM_MAP& m_ours;
    const ITEM_MAP& m_theirs;
    MERGE_PLAN      m_plan;
    REPORT          m_report;
};

} // namespace KICAD_DIFF

#endif // LIB_MERGE_APPLIER_H
