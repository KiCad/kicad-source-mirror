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

#include <json_common.h>
#include <cstdlib>
#include <api/api_enums.h>
#include <api/api_utils.h>
#include <base_units.h>
#include <erc/erc_item.h>
#include <wx/arrstr.h>

#include <erc/erc_exclusion.h>
#include <erc/erc_settings.h>       // for ERCE_T enum
#include <sch_marker.h>
#include <sch_sheet_path.h>

#include <api/schematic/schematic_rules.pb.h>
#include <google/protobuf/any.h>
#include <google/protobuf/util/json_util.h>


static void canonicalizeExclusion( kiapi::schematic::ErcExclusion& aMessage )
{
    if( !aMessage.has_marker() )
        return;

    // Display names can change without changing an exclusion's sheet identity.
    auto& marker = *aMessage.mutable_marker();

    if( marker.has_sheet_specific_path() )
        marker.mutable_sheet_specific_path()->clear_path_human_readable();

    if( marker.has_main_item_sheet_path() )
        marker.mutable_main_item_sheet_path()->clear_path_human_readable();

    if( marker.has_aux_item_sheet_path() )
        marker.mutable_aux_item_sheet_path()->clear_path_human_readable();
}


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
    canonicalizeExclusion( ex.m_impl->message );
    ex.SetComment( aMarker.GetComment() );

    return ex;
}


ERC_EXCLUSION ERC_EXCLUSION::FromProto( const kiapi::schematic::ErcExclusion& aMessage )
{
    ERC_EXCLUSION ex;
    ex.m_impl->message.CopyFrom( aMessage );
    canonicalizeExclusion( ex.m_impl->message );
    return ex;
}


ERC_EXCLUSION ERC_EXCLUSION::FromLegacyStrings( const SCH_SHEET_LIST& aSheetList, const wxString& aMarkerData,
                                                const wxString& aComment )
{
    ERC_EXCLUSION ex;
    const wxArrayString props = wxSplit( aMarkerData, '|' );

    if( props.size() != 5 && props.size() != 8 )
        return ex;

    const auto item = ERC_ITEM::Create( props[0] );

    if( !item )
        return ex;

    const auto code = static_cast<ERCE_T>( item->GetErrorCode() );
    auto& marker = *ex.m_impl->message.mutable_marker();
    marker.set_error_type( ToProtoEnum<ERCE_T, kiapi::schematic::ErcErrorType>( code ) );
    const VECTOR2I position( static_cast<int>( std::strtol( props[1].c_str(), nullptr, 10 ) ),
                             static_cast<int>( std::strtol( props[2].c_str(), nullptr, 10 ) ) );
    kiapi::common::PackVector2( *marker.mutable_position(), position, schIUScale );

    const KIID main( props[3] );

    if( main != niluuid )
        marker.add_items()->set_value( main.AsStdString() );

    const bool childText = ( code == ERCE_GENERIC_WARNING || code == ERCE_GENERIC_ERROR
                             || code == ERCE_UNRESOLVED_VARIABLE )
                           && main != niluuid && !props[4].IsEmpty() && props[4] != niluuid.AsString();

    if( childText )
    {
        marker.mutable_child()->set_text_value( props[4].ToUTF8() );

        // Match the current child ID without discarding an unavailable instance's saved path.
        std::unique_ptr<SCH_MARKER> resolved( SCH_MARKER::FromProto( marker, aSheetList ) );

        if( resolved )
            marker.add_items()->set_value( resolved->GetRCItem()->GetMainItemID().AsStdString() );
    }
    else if( !props[4].IsEmpty() )
    {
        const KIID auxiliary( props[4] );

        if( auxiliary != niluuid )
            marker.add_items()->set_value( auxiliary.AsStdString() );
    }

    if( props.size() == 8 )
    {
        if( !props[5].IsEmpty() )
            kiapi::common::PackSheetPath( *marker.mutable_sheet_specific_path(), KIID_PATH( props[5] ) );

        if( !props[6].IsEmpty() )
            kiapi::common::PackSheetPath( *marker.mutable_main_item_sheet_path(), KIID_PATH( props[6] ) );

        if( !props[7].IsEmpty() )
            kiapi::common::PackSheetPath( *marker.mutable_aux_item_sheet_path(), KIID_PATH( props[7] ) );
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
    canonicalizeExclusion( aEx.m_impl->message );
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
