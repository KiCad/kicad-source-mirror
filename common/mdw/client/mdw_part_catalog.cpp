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

#include <mdw/client/mdw_part_catalog.h>
#include <mdw/client/mdw_source.h>

#include "mdw_call.h"

#include <api/mdw/backend_parts.pb.h>

#include <utility>

namespace mdw = kiapi::mdw;

namespace
{

/// The contract's bound on one ResolveParts batch
constexpr size_t MAX_RESOLVE_MEMBERS = 1000;


/// Members may live in another source, so only the part itself takes this source's locator
MDW_PART_INFO toPart( const kiapi::workspace::Part& aProto, const wxString& aLocator )
{
    MDW_PART_INFO part;
    part.part = MdwStamped( MDW_ITEM_REF{ MdwFromUtf8( aProto.part_id() ), MdwFromUtf8( aProto.part_number() ),
                                          wxEmptyString },
                            aLocator );
    part.part_number = MdwFromUtf8( aProto.part_number() );
    part.name = MdwFromUtf8( aProto.name() );
    part.description = MdwFromUtf8( aProto.description() );
    part.revision_id = MdwFromUtf8( aProto.revision_id() );
    part.revision_label = MdwFromUtf8( aProto.revision_label() );
    part.state = MdwToStruct( aProto.state() );
    part.url = MdwFromUtf8( aProto.url() );

    for( const kiapi::workspace::PartMember& member : aProto.members() )
    {
        part.members.push_back( { MdwFromProto( member.kind() ), MdwStamped( MdwToStruct( member.member() ), aLocator ),
                                  MdwFromUtf8( member.revision_id() ) } );
    }

    for( const kiapi::workspace::ManufacturerPart& mpn : aProto.manufacturer_parts() )
        part.manufacturer_parts.push_back( { MdwFromUtf8( mpn.manufacturer() ), MdwFromUtf8( mpn.mpn() ) } );

    return part;
}

} // namespace


MDW_PART_CATALOG::MDW_PART_CATALOG( std::shared_ptr<MDW_SOURCE> aSource ) :
        m_source( std::move( aSource ) )
{
}


MDW_OP MDW_PART_CATALOG::Get( const MDW_ITEM_REF& aPart, const wxString& aRevisionId, MDW_DONE<MDW_PART_INFO> aDone )
{
    if( m_source && !MdwOwnItem( *m_source, aPart ) )
    {
        return MdwFailNow<MDW_PART_INFO>( MDW_RESULT::Fail( MDW_ERROR_CODE::INVALID_PARAMETER,
                                                            wxS( "The part belongs to another source" ) ),
                                          std::move( aDone ) );
    }

    mdw::GetPartRequest request;
    request.set_part_id( MdwToUtf8( aPart.item_id ) );
    request.set_revision_id( MdwToUtf8( aRevisionId ) );

    return MdwCall<mdw::GetPartRequest, mdw::GetPartReply, MDW_PART_INFO>(
            m_source, MDW_CAPABILITY_GROUP::PARTS, request,
            [locator = m_source ? m_source->Info().locator : wxString()]( const mdw::GetPartReply& aReply )
            {
                return toPart( aReply.part(), locator );
            },
            std::move( aDone ) );
}


MDW_OP MDW_PART_CATALOG::Search( const wxString& aText, const MDW_PAGE& aPage,
                                 MDW_DONE<MDW_PAGED<MDW_PART_INFO>> aDone )
{
    mdw::SearchPartsRequest request;
    request.set_text( MdwToUtf8( aText ) );
    MdwToProto( request.mutable_page(), aPage );

    return MdwCall<mdw::SearchPartsRequest, mdw::SearchPartsReply, MDW_PAGED<MDW_PART_INFO>>(
            m_source, MDW_CAPABILITY_GROUP::PARTS, request,
            [locator = m_source ? m_source->Info().locator : wxString()]( const mdw::SearchPartsReply& aReply )
            {
                MDW_PAGED<MDW_PART_INFO> page;

                for( const kiapi::workspace::Part& part : aReply.parts() )
                    page.rows.push_back( toPart( part, locator ) );

                page.next_page_token = MdwFromUtf8( aReply.next_page_token() );

                if( aReply.has_total_count() )
                    page.total_count = aReply.total_count();

                return page;
            },
            std::move( aDone ) );
}


MDW_OP MDW_PART_CATALOG::Resolve( const std::vector<MDW_ITEM_REF>& aMembers,
                                  MDW_DONE<std::vector<MDW_PART_CANDIDATES>> aDone )
{
    if( aMembers.size() > MAX_RESOLVE_MEMBERS )
    {
        return MdwFailNow<std::vector<MDW_PART_CANDIDATES>>(
                MDW_RESULT::Fail( MDW_ERROR_CODE::INVALID_PARAMETER ), std::move( aDone ) );
    }

    const wxString locator = m_source ? m_source->Info().locator : wxString();

    // Members are lookup keys that may name another source; the receiving backend's own go out empty
    mdw::ResolvePartsRequest request;

    for( MDW_ITEM_REF member : aMembers )
    {
        if( member.source == locator )
            member.source.clear();

        MdwToProto( request.add_members(), member );
    }

    return MdwCall<mdw::ResolvePartsRequest, mdw::ResolvePartsReply, std::vector<MDW_PART_CANDIDATES>>(
            m_source, MDW_CAPABILITY_GROUP::PARTS, request,
            [locator]( const mdw::ResolvePartsReply& aReply )
            {
                std::vector<MDW_PART_CANDIDATES> resolved;

                for( const mdw::ResolvedPart& proto : aReply.parts() )
                {
                    MDW_PART_CANDIDATES entry;
                    entry.member = MdwStamped( MdwToStruct( proto.member() ), locator );

                    for( const kiapi::workspace::Part& part : proto.candidates() )
                        entry.candidates.push_back( toPart( part, locator ) );

                    resolved.push_back( std::move( entry ) );
                }

                return resolved;
            },
            std::move( aDone ) );
}
