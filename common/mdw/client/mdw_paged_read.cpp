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

#include <mdw/client/mdw_paged_read.h>

#include "mdw_op_state.h"

#include <mutex>
#include <optional>
#include <set>

#include <wx/intl.h>


namespace
{

// With worker delivery a step's callback may start the next step before the starter of its own op has
// stored it, so a later step always wins the handoff
struct PAGED_RUN : std::enable_shared_from_this<PAGED_RUN>
{
    std::shared_ptr<MDW_OP_STATE> state = std::make_shared<MDW_OP_STATE>();
    MDW_PAGE_FETCH                fetch;
    int                           maxPages = 0;
    MDW_READ_CAP                  cap = MDW_READ_CAP::FAIL;
    int                           pages = 0;
    std::set<wxString>            asked;

    // Pages started inside a sync scope stay in it, or their completions would wait for the UI thread
    bool sync = MDW_SYNC_SCOPE::IsOpen();

    std::mutex mutex;
    MDW_OP     current;
    int        currentStep = -1;
    int        nextStep = 0;
    bool       cancelled = false;

    void Ask( const wxString& aToken )
    {
        std::optional<MDW_SYNC_SCOPE> scope;
        int                           step = 0;

        if( sync )
            scope.emplace();

        {
            std::lock_guard<std::mutex> lock( mutex );

            if( cancelled )
                return;

            step = nextStep++;
        }

        asked.insert( aToken );

        MDW_OP op = fetch( aToken,
                           [run = shared_from_this()]( const MDW_RESULT& aResult, const wxString& aNext )
                           {
                               run->Answered( aResult, aNext );
                           } );

        std::unique_lock<std::mutex> lock( mutex );

        if( cancelled )
        {
            lock.unlock();
            op.Cancel();
            return;
        }

        // A finished step is let go rather than cancelled, since its callback may be the caller
        MDW_OP stale = std::move( op );

        if( step > currentStep )
        {
            currentStep = step;
            std::swap( stale, current );
        }

        lock.unlock();
        stale.Detach();
    }

    void Answered( const MDW_RESULT& aResult, const wxString& aNext )
    {
        MDW_RESULT outcome = aResult;
        wxString   unread;

        if( aResult.IsOk() && !aNext.IsEmpty() )
        {
            if( asked.count( aNext ) )
            {
                outcome = MDW_RESULT::Fail( MDW_ERROR_CODE::PROTOCOL,
                                            _( "The backend sent a page it had already sent." ) );
            }
            else if( ++pages < maxPages )
            {
                Ask( aNext );
                return;
            }
            else
            {
                unread = aNext;

                if( cap == MDW_READ_CAP::FAIL )
                {
                    outcome = MDW_RESULT::Fail( MDW_ERROR_CODE::PROTOCOL,
                                                _( "The list is too long to read in full." ) );
                }
            }
        }

        state->Answer( outcome, unread );

        // The last page's op holds its callback, which holds this run
        std::unique_lock<std::mutex> lock( mutex );
        MDW_OP                       last = std::move( current );
        lock.unlock();
        last.Detach();
    }

    void Cancel()
    {
        std::unique_lock<std::mutex> lock( mutex );
        cancelled = true;
        MDW_OP op = std::move( current );
        lock.unlock();
        op.Cancel();

        // The page's callback is gone, so nothing else would ever retire the op
        state->Complete( {} );
    }
};

} // namespace


MDW_OP MdwReadPages( MDW_PAGE_FETCH aFetch, int aMaxPages, MDW_READ_CAP aCap,
                     std::function<void( const MDW_RESULT&, const wxString& )> aDone )
{
    auto run = std::make_shared<PAGED_RUN>();
    run->fetch = std::move( aFetch );
    run->state->Hold<wxString>( std::move( aDone ) );
    run->maxPages = aMaxPages;
    run->cap = aCap;

    run->state->SetCanceller(
            [weak = std::weak_ptr<PAGED_RUN>( run )]()
            {
                if( std::shared_ptr<PAGED_RUN> live = weak.lock() )
                    live->Cancel();
            } );

    MDW_OP op( run->state );
    run->Ask( wxEmptyString );
    return op;
}
