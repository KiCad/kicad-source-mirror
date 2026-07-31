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

#include <mdw/client/mdw_search.h>
#include <mdw/client/mdw_source.h>
#include "mdw_call.h"

#include <api/mdw/backend_search.pb.h>

#include <utility>

namespace mdw = kiapi::mdw;


MDW_SEARCH::MDW_SEARCH( std::shared_ptr<MDW_SOURCE> aSource ) :
        m_source( std::move( aSource ) )
{
}


MDW_OP MDW_SEARCH::Fields( MDW_DONE<std::vector<kiapi::workspace::SearchField>> aDone )
{
    return MdwCall<mdw::GetSearchSchemaRequest, mdw::GetSearchSchemaReply, std::vector<kiapi::workspace::SearchField>>(
            m_source, MDW_CAPABILITY_GROUP::SEARCH, mdw::GetSearchSchemaRequest(),
            []( const mdw::GetSearchSchemaReply& aReply )
            {
                std::vector<kiapi::workspace::SearchField> fields( aReply.fields().begin(), aReply.fields().end() );

                for( kiapi::workspace::SearchField& field : fields )
                {
                    if( !kiapi::workspace::AttrType_IsValid( field.type() ) )
                        field.set_type( kiapi::workspace::AT_STRING );
                }

                return fields;
            },
            std::move( aDone ) );
}


MDW_OP MDW_SEARCH::Run( const MDW_SEARCH_QUERY& aQuery, const MDW_PAGE& aPage,
                        MDW_DONE<MDW_PAGED<MDW_ITEM_INFO>> aDone )
{
    mdw::SearchRequest request;
    request.set_text( MdwToUtf8( aQuery.text ) );
    MdwToProto( request.mutable_page(), aPage );

    for( const MDW_SEARCH_FILTER& filter : aQuery.filters )
        MdwToProto( request.add_filters(), filter );

    return MdwCall<mdw::SearchRequest, mdw::SearchReply, MDW_PAGED<MDW_ITEM_INFO>>(
            m_source, MDW_CAPABILITY_GROUP::SEARCH, request,
            []( const mdw::SearchReply& aReply )
            {
                MDW_PAGED<MDW_ITEM_INFO> page;

                for( const kiapi::workspace::ItemInfo& item : aReply.items() )
                    page.rows.push_back( MdwToStruct( item ) );

                page.next_page_token = MdwFromUtf8( aReply.next_page_token() );

                if( aReply.has_total_count() )
                    page.total_count = aReply.total_count();

                return page;
            },
            std::move( aDone ) );
}
