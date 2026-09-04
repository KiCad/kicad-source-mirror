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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WORKSPACE_PARTS_SEARCH_H
#define WORKSPACE_PARTS_SEARCH_H

#include <lib_id.h>
#include <mdw/client/mdw_part_catalog.h>

#include <wx/timer.h>

#include <functional>
#include <memory>
#include <vector>

class MDW_SOURCE;


/**
 * The symbol chooser's search of every source that offers PARTS. It keeps one result group per
 * source, pages through results and backs off when a source is rate limited. Everything runs on
 * the UI thread; the owner redraws a group when told it changed.
 */
class WORKSPACE_PARTS_SEARCH
{
public:
    static constexpr unsigned PAGE_SIZE = 25;

    /// Used when a rate-limited source gives no retry delay
    static constexpr unsigned DEFAULT_RETRY_MS = 2000;

    enum class ROW_KIND
    {
        PART,
        STATUS,     ///< Searching, empty, offline or denied; inert
        MORE,       ///< Selecting it fetches the next page
        RETRY       ///< Selecting it runs the search again
    };

    enum class RESOLUTION
    {
        RESOLVED,
        NOT_SET_UP,     ///< The symbol's library is not configured here
        NO_SYMBOL,
        UNRESOLVED      ///< The source names a symbol that none of its libraries holds
    };

    struct ROW
    {
        ROW_KIND      kind = ROW_KIND::STATUS;
        wxString      text;
        MDW_PART_INFO part;
        LIB_ID        symbol;
        RESOLUTION    resolution = RESOLUTION::NO_SYMBOL;
    };

    struct GROUP
    {
        wxString         name;
        wxString         source_name;
        std::vector<ROW> rows;
    };

    /// Find the local symbol for a part; fills aSymbol when RESOLVED
    using RESOLVER = std::function<RESOLUTION( const MDW_PART_INFO& aPart, LIB_ID& aSymbol )>;

    using CHANGED = std::function<void( size_t aGroup )>;

    WORKSPACE_PARTS_SEARCH( std::vector<std::shared_ptr<MDW_SOURCE>> aSources, RESOLVER aResolver, CHANGED aChanged );

    /// Every source that offers PARTS, the project's first
    static std::vector<std::shared_ptr<MDW_SOURCE>> PartSources();

    /// Search every source for aText; empty text cancels and clears every group
    void SetQuery( const wxString& aText );

    const std::vector<GROUP>& Groups() const { return m_groups; }

    /// Act on a MORE or RETRY row; other rows are ignored
    void Activate( size_t aGroup, size_t aRow );

    /// Resolve again the parts whose library was not available, as after libraries finish loading
    void Reresolve();

    std::shared_ptr<MDW_SOURCE> Source( size_t aGroup ) const;

private:
    struct SOURCE_STATE
    {
        std::shared_ptr<MDW_SOURCE> source;
        MDW_OP                      op;
        std::unique_ptr<wxTimer>    retry;
        wxString                    next_page_token;
        std::vector<ROW>            parts;
    };

    void startPage( size_t aGroup, bool aFirstPage );
    void onPage( size_t aGroup, bool aFirstPage, const MDW_RESULT& aResult, const MDW_PAGED<MDW_PART_INFO>& aPage );
    void setStatus( size_t aGroup, ROW_KIND aKind, const wxString& aText );
    void publish( size_t aGroup, std::vector<ROW> aTail );

    RESOLVER                   m_resolver;
    CHANGED                    m_changed;
    wxString                   m_query;
    std::vector<SOURCE_STATE>  m_sources;
    std::vector<GROUP>         m_groups;
};

#endif // WORKSPACE_PARTS_SEARCH_H
