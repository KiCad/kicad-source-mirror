/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef API_GENERATED_TABLE_UTILS_H
#define API_GENERATED_TABLE_UTILS_H

#include <algorithm>
#include <cstdint>
#include <type_traits>

#include <api/api_enums.h>
#include <api/api_utils.h>
#include <base_units.h>
#include <pcb_generated_table.h>

#include <google/protobuf/any.pb.h>


/**
 * Pack what every generated table shares into its message.
 *
 * Each generated table has its own message, so this relies on every one of them naming these
 * fields the same way: table, columns (id, heading, align, width, hidden), units, precision and
 * row_keys. Column ids and alignments are offset by one, leaving zero as the unset value.
 */
template <typename MSG>
void PackGeneratedTable( const PCB_GENERATED_TABLE& aTable, MSG& aMsg )
{
    google::protobuf::Any tableAny;
    aTable.PCB_TABLE::Serialize( tableAny );
    tableAny.UnpackTo( aMsg.mutable_table() );

    for( const GENERATED_TABLE_COLUMN& col : aTable.AllColumns() )
    {
        auto* proto = aMsg.add_columns();

        using ID = std::remove_cvref_t<decltype( proto->id() )>;
        using ALIGN = std::remove_cvref_t<decltype( proto->align() )>;

        proto->set_id( static_cast<ID>( col.m_Id + 1 ) );
        proto->set_heading( col.m_Heading.ToStdString() );
        proto->set_align( static_cast<ALIGN>( static_cast<int>( col.m_Align ) + 1 ) );
        kiapi::common::PackDistance( *proto->mutable_width(), col.m_Width );
        proto->set_hidden( !col.m_Visible );
    }

    aMsg.set_units( ToProtoEnum<GENERATED_TABLE_UNITS, kiapi::common::types::Units>( aTable.GetUnits() ) );

    aMsg.set_precision( aTable.GetPrecision() );

    for( const auto& [row, key] : aTable.RowKeys() )
        ( *aMsg.mutable_row_keys() )[row] = key;
}


/**
 * The reverse of PackGeneratedTable. False, possibly with aTable partly updated, when the
 * message puts the table on a manufacturing layer, names a column the table's schema does
 * not know, leaves no usable column set, or asks for a precision outside the supported range.
 */
template <typename MSG>
bool UnpackGeneratedTable( const MSG& aMsg, PCB_GENERATED_TABLE& aTable )
{
    google::protobuf::Any tableAny;
    tableAny.PackFrom( aMsg.table() );

    if( !aTable.PCB_TABLE::Deserialize( tableAny ) )
        return false;

    // The table carries the layer. A table on a manufacturing layer would be plotted into a
    // fabrication output rather than the documentation
    if( !DocumentationLayers().Contains( aTable.GetLayer() ) )
        return false;

    std::vector<GENERATED_TABLE_COLUMN> columns;

    for( const auto& proto : aMsg.columns() )
    {
        const int wireId = static_cast<int>( proto.id() );

        // Zero is the unset value. A negative id is refused before the offset can overflow
        if( wireId == 0 )
            continue;

        if( wireId < 0 )
            return false;

        const int id = wireId - 1;

        if( !aTable.Schema().Find( id ) )
            return false;

        const int align = static_cast<int>( proto.align() );

        if( align < 0 || align > static_cast<int>( GENERATED_TABLE_ALIGN::RIGHT ) + 1 )
            return false;

        GENERATED_TABLE_COLUMN col;
        col.m_Id = id;
        col.m_Heading = wxString::FromUTF8( proto.heading() );

        if( align > 0 )
            col.m_Align = static_cast<GENERATED_TABLE_ALIGN>( align - 1 );

        kiapi::common::types::Distance width;
        width.set_value_nm( std::clamp<int64_t>( proto.width().value_nm(), 0,
                                                 pcbIUScale.IUToNm( GENERATED_TABLE_MAX_COLUMN_WIDTH ) ) );
        col.m_Width = kiapi::common::UnpackDistance( width );
        col.m_Visible = !proto.hidden();
        columns.push_back( col );
    }

    // No columns divides by zero the next time this is rebuilt or autosized. Repeats and
    // implausible widths reach table geometry
    if( !aTable.Schema().Validate( columns ) )
        return false;

    // A visibility edit can leave the message's cell grid at its old width until the caller rebuilds it.
    if( std::none_of( columns.begin(), columns.end(),
                      []( const GENERATED_TABLE_COLUMN& aCol )
                      {
                          return aCol.m_Visible;
                      } ) )
        return false;

    aTable.RestoreColumns( columns );

    aTable.SetUnits( FromProtoEnum<GENERATED_TABLE_UNITS, kiapi::common::types::Units>( aMsg.units() ) );

    aTable.RowKeys().clear();

    for( const auto& [row, key] : aMsg.row_keys() )
        aTable.RowKeys()[row] = key;

    // Refused rather than clamped, so a client never reads back a value it did not send
    if( aMsg.precision() < 0 || aMsg.precision() > GENERATED_TABLE_MAX_PRECISION )
        return false;

    aTable.SetPrecision( aMsg.precision() );

    return true;
}

#endif // API_GENERATED_TABLE_UTILS_H
