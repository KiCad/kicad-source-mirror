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

/**
 * @file mdw_paged_read.h
 * The one way to read every page of a paged list.
 */

#ifndef MDW_CLIENT_PAGED_READ_H
#define MDW_CLIENT_PAGED_READ_H

#include <kicommon.h>
#include <mdw/client/mdw_client_types.h>
#include <mdw/client/mdw_navigation.h>
#include <mdw/client/mdw_op.h>

#include <functional>
#include <memory>

/// Starts the read of the page at aToken and reports its status and the token of the page after it
using MDW_PAGE_FETCH = std::function<MDW_OP(
        const wxString& aToken, std::function<void( const MDW_RESULT&, const wxString& aNext )> aOnPage )>;

/// What a read does when aMaxPages stops it with pages left
enum class MDW_READ_CAP
{
    FAIL,    ///< The read fails with PROTOCOL, so a short list never passes for the whole one
    PARTIAL  ///< The read succeeds and names the first page left unread, for a caller that a short list serves
};

/**
 * Read pages from an empty token until one names no next page. The read ends early with PROTOCOL when a backend
 * sends back a token it was already given. After aMaxPages pages it ends as aCap says, with the token left unread.
 * aDone gets that token, empty when the list was read in full, even when the read failed. Cancelling or destroying
 * the returned op cancels the page in flight and suppresses aDone.
 */
KICOMMON_API MDW_OP MdwReadPages( MDW_PAGE_FETCH aFetch, int aMaxPages, MDW_READ_CAP aCap,
                                  std::function<void( const MDW_RESULT&, const wxString& aUnread )> aDone );

/**
 * Every row of a paged list, merged. aFetch reads one page with the MDW_PAGE it is given. The rows read so far reach
 * aDone even when a later page failed or aCap failed the read, so a caller may show them as a short list, and
 * next_page_token is the first page left unread when aMaxPages stopped the read. aEnough, when set, ends the read
 * after the page it accepts.
 */
template <typename ROW>
MDW_OP MdwReadAllPages( std::function<MDW_OP( const MDW_PAGE&, MDW_DONE<MDW_PAGED<ROW>> )> aFetch, unsigned aPageSize,
                        MDW_DONE<MDW_PAGED<ROW>> aDone, MDW_READ_CAP aCap = MDW_READ_CAP::FAIL,
                        int aMaxPages = MDW_MAX_PAGES,
                        std::function<bool( const MDW_PAGED<ROW>& )> aEnough = nullptr )
{
    auto read = std::make_shared<MDW_PAGED<ROW>>();

    return MdwReadPages(
            [aFetch = std::move( aFetch ), aPageSize, read, aEnough = std::move( aEnough )](
                    const wxString& aToken, std::function<void( const MDW_RESULT&, const wxString& )> aOnPage )
            {
                return aFetch( MDW_PAGE{ aPageSize, aToken },
                               [read, aOnPage = std::move( aOnPage ), aEnough]( const MDW_RESULT&         aResult,
                                                                                 const MDW_PAGED<ROW>& aPage )
                               {
                                   wxString next;

                                   if( aResult.IsOk() )
                                   {
                                       read->rows.insert( read->rows.end(), aPage.rows.begin(), aPage.rows.end() );
                                       read->total_count = aPage.total_count;
                                       next = aEnough && aEnough( aPage ) ? wxString() : aPage.next_page_token;
                                   }

                                   aOnPage( aResult, next );
                               } );
            },
            aMaxPages, aCap,
            [read, aDone = std::move( aDone )]( const MDW_RESULT& aResult, const wxString& aUnread )
            {
                read->next_page_token = aUnread;

                if( aDone )
                    aDone( aResult, *read );
            } );
}

#endif // MDW_CLIENT_PAGED_READ_H
