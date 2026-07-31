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

#include <mdw/client/mdw_bom.h>

#include "mdw_call.h"

#include <api/mdw/backend_bom.pb.h>

#include <utility>


namespace
{

MDW_BOM_LINE toStruct( const kiapi::mdw::BomLine& aProto, const wxString& aLocator )
{
    MDW_BOM_LINE line;
    line.part = MdwToStruct( aProto.part(), aLocator );
    line.part_number = MdwFromUtf8( aProto.part_number() );
    line.quantity = aProto.quantity();
    line.unit = MdwFromUtf8( aProto.unit() );

    for( const std::string& reference : aProto.references() )
        line.references.push_back( MdwFromUtf8( reference ) );

    return line;
}

} // namespace


MDW_BOM::MDW_BOM( std::shared_ptr<MDW_SOURCE> aSource ) :
        m_source( std::move( aSource ) )
{
}


MDW_OP MDW_BOM::Get( const MDW_ITEM_REF& aProject, const wxString& aRevisionId, MDW_DONE<MDW_BOM_DATA> aDone )
{
    kiapi::mdw::GetBomRequest request;

    if( m_source )
    {
        std::optional<std::string> id = MdwWireId( *m_source, aProject );

        if( !id )
            return MdwForeignItem( m_source, std::move( aDone ) );

        request.set_item_id( *id );
    }

    request.set_revision_id( MdwToUtf8( aRevisionId ) );

    return MdwCall<kiapi::mdw::GetBomRequest, kiapi::mdw::GetBomReply, MDW_BOM_DATA>(
            m_source, MDW_CAPABILITY_GROUP::BOM, request,
            [locator = m_source ? m_source->Info().locator : wxString()]( const kiapi::mdw::GetBomReply& aReply )
            {
                const kiapi::mdw::Bom& proto = aReply.bom();
                MDW_BOM_DATA           bom;

                bom.bom_id = MdwFromUtf8( proto.bom_id() );
                bom.bom_type = MdwFromUtf8( proto.bom_type() );
                bom.label = MdwFromUtf8( proto.label() );
                bom.updated = MdwFromUnixTime( proto.updated_unix() );
                bom.updated_by = MdwFromUtf8( proto.updated_by() );

                for( const kiapi::mdw::BomLine& line : proto.lines() )
                    bom.lines.push_back( toStruct( line, locator ) );

                return bom;
            },
            std::move( aDone ) );
}
