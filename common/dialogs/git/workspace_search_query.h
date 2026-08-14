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

#ifndef WORKSPACE_SEARCH_QUERY_H
#define WORKSPACE_SEARCH_QUERY_H

#include <kicommon.h>
#include <mdw/client/mdw_client_types.h>
#include <mdw/client/mdw_item.h>
#include <mdw/client/mdw_op.h>
#include <mdw/client/mdw_search.h>

#include <wx/datetime.h>
#include <wx/event.h>
#include <wx/timer.h>

#include <functional>
#include <memory>
#include <optional>
#include <vector>


/// What the Workspace page has shown so far, for the status line under the list
struct WORKSPACE_SEARCH_VIEW
{
    wxString                source_name;
    MDW_SEARCH_QUERY        query;
    size_t                  shown = 0;
    std::optional<uint64_t> total;
    wxDateTime              last_good;      ///< Invalid until a page has arrived
};


/**
 * The Workspace page's search box answered by a source that offers SEARCH; for a source that cannot search, the
 * page keeps filtering the rows it already holds. Typing is debounced, a newer query cancels an older one, and a
 * rate-limited source is retried after the delay it asks for.
 *
 * Rows arrive a page at a time through the callback Begin() was given, on the UI thread. Destroying the query
 * cancels it. The query owns every search state and its copy, the debounce and any retry delay; the page shows
 * what it is told.
 */
class KICOMMON_API WORKSPACE_SEARCH_QUERY : public wxEvtHandler
{
public:
    /// What the page shows under the list
    struct STATUS
    {
        wxString                text;              ///< Shown verbatim, such as "Searching…" or "50 of 312 items"
        bool                    stale = false;     ///< The rows are the last good result; grey them
        bool                    can_retry = false; ///< Offer Retry, which calls Retry()
        std::optional<uint64_t> total_count;       ///< Every match, when the source knows it
        MDW_RESULT              result = MDW_RESULT::Ok(); ///< The last call's outcome, already put into text
        bool                    searching = false;         ///< A request is in flight; the next update settles it
    };

    /**
     * aRows replaces the shown rows when aFirstPage and appends otherwise. A status-only update
     * passes no rows and aFirstPage false.
     */
    using ROWS = std::function<void( const std::vector<MDW_ITEM_INFO>& aRows, bool aFirstPage, bool aHasMore,
                                     const STATUS& aStatus )>;

    static constexpr int      DEBOUNCE_MS = 300;
    static constexpr int      DEFAULT_RETRY_MS = 2000;
    static constexpr int      MAX_RETRY_MS = 60000;
    static constexpr unsigned PAGE_SIZE = 50;

    explicit WORKSPACE_SEARCH_QUERY( std::shared_ptr<MDW_SOURCE> aSource );
    ~WORKSPACE_SEARCH_QUERY() override;

    /// A query for aSource, or null when it does not offer SEARCH
    static std::unique_ptr<WORKSPACE_SEARCH_QUERY> Create( const std::shared_ptr<MDW_SOURCE>& aSource );

    /// True when aSource offers SEARCH, so Create() answers a query
    static bool Offers( const std::shared_ptr<MDW_SOURCE>& aSource );

    /// The search box hint of a query, which needs no query to read
    static wxString Hint();

    /// Start or replace the query; a newer call supersedes an older one
    void Begin( const MDW_SEARCH_QUERY& aQuery, ROWS aRows );

    /// Fetch the next page; ignored when the last page has arrived
    void More();

    /// Run the current query again from its first page
    void Retry();

    void Cancel();

private:
    void run( bool aFirstPage );
    void onPage( bool aFirstPage, const MDW_RESULT& aResult, const MDW_PAGED<MDW_ITEM_INFO>& aPage );

    std::shared_ptr<MDW_SOURCE> m_source;
    MDW_SEARCH                  m_search;
    WORKSPACE_SEARCH_VIEW       m_view;
    WORKSPACE_SEARCH_VIEW       m_displayed;    ///< m_view as of the rows now on the page
    ROWS                        m_rows;
    wxString                    m_nextPageToken;
    MDW_OP                      m_op;
    bool                        m_inFlight = false;
    wxTimer                     m_debounce;     ///< Owns its events, which its handler is bound to
    wxTimer                     m_retry;        ///< Owns its events, which its handler is bound to
    bool                        m_retryFirstPage = true;
};

/// True for the failures shown as Offline, which keep the rows greyed and offer nothing that needs the server
KICOMMON_API bool WorkspaceSearchOffline( const MDW_RESULT& aResult );

/// Whether a page showing aShown runs its search again now the project's connection is aState
KICOMMON_API bool WorkspaceSearchRefreshes( const WORKSPACE_SEARCH_QUERY::STATUS& aShown, MDW_CONNECTION aState );

/// The status a page takes when its session goes offline, keeping aShown's rows greyed under when they were last good
KICOMMON_API WORKSPACE_SEARCH_QUERY::STATUS WorkspaceOfflineStatus( const WORKSPACE_SEARCH_QUERY::STATUS& aShown,
                                                                    bool aHasRows, const wxDateTime& aLastGood );

#endif // WORKSPACE_SEARCH_QUERY_H
