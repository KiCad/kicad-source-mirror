/*
* This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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

#include <nlohmann/json.hpp>

#include <board_design_settings.h>
#include <pcb_marker.h>

#include <api/board/board_rules.pb.h>
#include <google/protobuf/any.h>
#include <google/protobuf/util/json_util.h>


struct DRC_EXCLUSION::IMPL
{
    kiapi::board::DrcExclusion message;
};


DRC_EXCLUSION::DRC_EXCLUSION() :
        m_impl( std::make_unique<IMPL>() )
{
}


DRC_EXCLUSION::DRC_EXCLUSION( const DRC_EXCLUSION& aOther ) :
        m_impl( std::make_unique<IMPL>( *aOther.m_impl ) )
{
}


DRC_EXCLUSION& DRC_EXCLUSION::operator=( const DRC_EXCLUSION& aOther )
{
    *m_impl = *aOther.m_impl;
    return *this;
}


DRC_EXCLUSION::~DRC_EXCLUSION() = default;


DRC_EXCLUSION DRC_EXCLUSION::FromMarker( const PCB_MARKER& aMarker )
{
    DRC_EXCLUSION ex;
    google::protobuf::Any container;

    aMarker.Serialize( container );
    container.UnpackTo( ex.m_impl->message.mutable_marker() );
    ex.SetComment( aMarker.GetComment() );

    return ex;
}


DRC_EXCLUSION DRC_EXCLUSION::FromProto( const kiapi::board::DrcExclusion& aMessage )
{
    DRC_EXCLUSION ex;
    ex.m_impl->message.CopyFrom( aMessage );
    return ex;
}


DRC_EXCLUSION DRC_EXCLUSION::FromLegacyStrings( const wxString& aMarkerData, const wxString& aComment )
{
    DRC_EXCLUSION ex;

    if( PCB_MARKER* marker = PCB_MARKER::FromLegacyString( aMarkerData ) )
    {
        google::protobuf::Any container;
        marker->Serialize( container );
        container.UnpackTo( ex.m_impl->message.mutable_marker() );
        delete marker;
    }

    ex.SetComment( aComment );
    return ex;
}


const kiapi::board::DrcExclusion& DRC_EXCLUSION::ToProto() const
{
    return m_impl->message;
}


void to_json( nlohmann::json& aJson, const DRC_EXCLUSION& aEx )
{
    google::protobuf::util::JsonPrintOptions jsonOptions;
    jsonOptions.preserve_proto_field_names = true;

    std::string json;
    std::ignore = google::protobuf::util::MessageToJsonString( aEx.m_impl->message, &json, jsonOptions ).ok();

    try
    {
        aJson = nlohmann::json::parse( json );
    }
    catch( ... )
    {
        aJson = nlohmann::json{};
    }
}


void from_json( const nlohmann::json& aJson, DRC_EXCLUSION& aEx )
{
    std::ignore = google::protobuf::util::JsonStringToMessage( aJson.dump(), &aEx.m_impl->message ).ok();
}


std::string DRC_EXCLUSION::GetSortKey() const
{
    if( !m_impl->message.has_marker() )
        return std::string();

    // Unconnected-item markers may be reported on several layers
    if( m_impl->message.marker().error_type() == kiapi::board::DRCET_UNCONNECTED_ITEMS )
    {
        kiapi::board::DrcMarker marker = m_impl->message.marker();
        marker.clear_layer();
        return marker.SerializeAsString();
    }

    return m_impl->message.marker().SerializeAsString();
}


wxString DRC_EXCLUSION::GetComment() const
{
    return wxString::FromUTF8( m_impl->message.comment() );
}


void DRC_EXCLUSION::SetComment( const wxString& aComment )
{
    m_impl->message.set_comment( aComment.ToUTF8() );
}
