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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <nlohmann/json.hpp>

#include <erc/erc_exclusion.h>
#include <sch_marker.h>
#include <sch_sheet_path.h>

#include <api/schematic/schematic_rules.pb.h>
#include <google/protobuf/any.h>
#include <google/protobuf/util/json_util.h>


struct ERC_EXCLUSION::IMPL
{
    kiapi::schematic::ErcExclusion message;
};


ERC_EXCLUSION::ERC_EXCLUSION() : m_impl( std::make_unique<IMPL>() ) {}


ERC_EXCLUSION::ERC_EXCLUSION( const ERC_EXCLUSION& aOther ) :
        m_impl( std::make_unique<IMPL>( *aOther.m_impl ) )
{}


ERC_EXCLUSION& ERC_EXCLUSION::operator=( const ERC_EXCLUSION& aOther )
{
    *m_impl = *aOther.m_impl;
    return *this;
}


ERC_EXCLUSION::~ERC_EXCLUSION() = default;


ERC_EXCLUSION ERC_EXCLUSION::FromMarker( const SCH_MARKER& aMarker )
{
    ERC_EXCLUSION ex;
    google::protobuf::Any container;

    aMarker.Serialize( container );
    container.UnpackTo( ex.m_impl->message.mutable_marker() );
    ex.SetComment( aMarker.GetComment() );

    return ex;
}


ERC_EXCLUSION ERC_EXCLUSION::FromProto( const kiapi::schematic::ErcExclusion& aMessage )
{
    ERC_EXCLUSION ex;
    ex.m_impl->message.CopyFrom( aMessage );
    return ex;
}


ERC_EXCLUSION ERC_EXCLUSION::FromLegacyStrings( const SCH_SHEET_LIST& aSheetList, const wxString& aMarkerData,
                                                const wxString& aComment )
{
    ERC_EXCLUSION ex;

    if( SCH_MARKER* marker = SCH_MARKER::FromLegacyString( aSheetList, aMarkerData ) )
    {
        google::protobuf::Any container;
        marker->Serialize( container );
        container.UnpackTo( ex.m_impl->message.mutable_marker() );
        delete marker;
    }

    ex.SetComment( aComment );
    return ex;
}


const kiapi::schematic::ErcExclusion& ERC_EXCLUSION::ToProto() const
{
    return m_impl->message;
}


void to_json( nlohmann::json& aJson, const ERC_EXCLUSION& aEx )
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


void from_json( const nlohmann::json& aJson, ERC_EXCLUSION& aEx )
{
    std::ignore = google::protobuf::util::JsonStringToMessage( aJson.dump(), &aEx.m_impl->message ).ok();
}


std::string ERC_EXCLUSION::GetSortKey() const
{
    if( m_impl->message.has_marker() )
        return m_impl->message.marker().SerializeAsString();

    return std::string();
}


wxString ERC_EXCLUSION::GetComment() const
{
    return wxString::FromUTF8( m_impl->message.comment() );
}


void ERC_EXCLUSION::SetComment( const wxString& aComment )
{
    m_impl->message.set_comment( aComment.ToUTF8() );
}
