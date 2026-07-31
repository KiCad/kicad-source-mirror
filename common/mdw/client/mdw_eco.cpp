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

#include <mdw/client/mdw_eco.h>

#include "mdw_call.h"


#include <utility>


namespace
{

/// A newer backend's category reads as UNKNOWN, and every affected item names its source
kiapi::mdw::Eco normalized( kiapi::mdw::Eco aEco, const wxString& aLocator )
{
    if( !kiapi::mdw::EcoState_IsValid( aEco.state_category() ) )
        aEco.set_state_category( kiapi::mdw::ECO_UNKNOWN );

    for( kiapi::workspace::ItemRef& item : *aEco.mutable_affected_items() )
        MdwStamp( item, aLocator );

    return aEco;
}

} // namespace


MDW_ECO::MDW_ECO( std::shared_ptr<MDW_SOURCE> aSource ) :
        m_source( std::move( aSource ) )
{
}


MDW_OP MDW_ECO::Schema( MDW_DONE<kiapi::workspace::Schema> aDone )
{
    return MdwCall<kiapi::mdw::GetEcoSchemaRequest, kiapi::mdw::GetEcoSchemaReply, kiapi::workspace::Schema>(
            m_source, MDW_CAPABILITY_GROUP::ECO, kiapi::mdw::GetEcoSchemaRequest(),
            []( const kiapi::mdw::GetEcoSchemaReply& aReply )
            {
                return aReply.schema();
            },
            std::move( aDone ) );
}


MDW_OP MDW_ECO::Get( const std::string& aEcoId, MDW_DONE<kiapi::mdw::Eco> aDone )
{
    kiapi::mdw::GetEcoRequest request;
    request.set_eco_id( aEcoId );

    return MdwCall<kiapi::mdw::GetEcoRequest, kiapi::mdw::GetEcoReply, kiapi::mdw::Eco>(
            m_source, MDW_CAPABILITY_GROUP::ECO, request,
            [locator = m_source ? m_source->Info().locator : wxString()]( const kiapi::mdw::GetEcoReply& aReply )
            {
                return normalized( aReply.eco(), locator );
            },
            std::move( aDone ) );
}


MDW_OP MDW_ECO::List( const std::vector<MDW_ITEM_REF>& aItems, bool aIncludeClosed, const MDW_PAGE& aPage,
                      MDW_DONE<MDW_PAGED<kiapi::mdw::Eco>> aDone )
{
    kiapi::mdw::ListEcosRequest request;

    for( MDW_ITEM_REF item : aItems )
    {
        // The receiving backend's own items travel without a source
        if( m_source && MdwOwnItem( *m_source, item ) )
            item.source.clear();

        MdwToProto( request.add_items(), item );
    }

    request.set_include_closed( aIncludeClosed );
    MdwToProto( request.mutable_page(), aPage );

    return MdwCall<kiapi::mdw::ListEcosRequest, kiapi::mdw::ListEcosReply, MDW_PAGED<kiapi::mdw::Eco>>(
            m_source, MDW_CAPABILITY_GROUP::ECO, request,
            [locator = m_source ? m_source->Info().locator : wxString()]( const kiapi::mdw::ListEcosReply& aReply )
            {
                MDW_PAGED<kiapi::mdw::Eco> page;
                page.next_page_token = MdwFromUtf8( aReply.next_page_token() );

                for( const kiapi::mdw::Eco& eco : aReply.ecos() )
                    page.rows.push_back( normalized( eco, locator ) );

                return page;
            },
            std::move( aDone ) );
}
