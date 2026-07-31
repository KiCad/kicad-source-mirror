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

#include <mdw/client/mdw_review.h>
#include <mdw/client/mdw_source.h>
#include <mdw/mdw_proto_convert.h>

#include "mdw_call.h"

#include <api/mdw/backend_review.pb.h>

#include <utility>

namespace mdw = kiapi::mdw;


namespace
{

/// A reviewed item names its source
mdw::Review stamped( mdw::Review aReview, const wxString& aLocator )
{
    if( !aReview.item().item_id().empty() )
        MdwStamp( *aReview.mutable_item(), aLocator );

    return aReview;
}

} // namespace


MDW_REVIEW::MDW_REVIEW( std::shared_ptr<MDW_SOURCE> aSource ) :
        m_source( std::move( aSource ) )
{
}


MDW_OP MDW_REVIEW::Get( const std::string& aReviewId, MDW_DONE<mdw::Review> aDone )
{
    mdw::GetReviewRequest request;
    request.set_review_id( aReviewId );
    wxString locator = m_source ? m_source->Info().locator : wxString();

    return MdwCall<mdw::GetReviewRequest, mdw::GetReviewReply, mdw::Review>(
            m_source, MDW_CAPABILITY_GROUP::REVIEW, request,
            [locator]( const mdw::GetReviewReply& aReply )
            {
                return stamped( aReply.review(), locator );
            },
            std::move( aDone ) );
}


MDW_OP MDW_REVIEW::List( const MDW_ITEM_REF& aItem, const wxString& aRevisionId, const MDW_PAGE& aPage,
                         MDW_DONE<mdw::ListReviewsReply> aDone )
{
    if( m_source && !MdwOwnItem( *m_source, aItem ) )
        return MdwForeignItem( m_source, std::move( aDone ) );

    mdw::ListReviewsRequest request;
    request.set_item_id( MdwToUtf8( aItem.item_id ) );
    request.set_revision_id( MdwToUtf8( aRevisionId ) );
    MdwToProto( request.mutable_page(), aPage );
    wxString locator = m_source ? m_source->Info().locator : wxString();

    return MdwCall<mdw::ListReviewsRequest, mdw::ListReviewsReply, mdw::ListReviewsReply>(
            m_source, MDW_CAPABILITY_GROUP::REVIEW, request,
            [locator, itemId = request.item_id()]( const mdw::ListReviewsReply& aReply )
            {
                mdw::ListReviewsReply list = aReply;
                list.clear_reviews();

                for( const mdw::Review& proto : aReply.reviews() )
                {
                    mdw::Review                      review = stamped( proto, locator );
                    const kiapi::workspace::ItemRef& item = review.item();

                    // A review of another item must never vouch for this one
                    if( !item.item_id().empty()
                        && ( item.item_id() != itemId || item.source() != MdwToUtf8( locator ) ) )
                    {
                        continue;
                    }

                    *list.add_reviews() = std::move( review );
                }

                return list;
            },
            std::move( aDone ) );
}
