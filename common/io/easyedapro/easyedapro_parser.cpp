/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Alex Shvartzkop <dudesuchamazing@gmail.com>
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

#include "easyedapro_parser.h"

#include <memory>

#include <json_common.h>
#include <core/json_serializers.h>
#include <string_utils.h>
#include <io/easyedapro/easyedapro_import_utils.h>


void EASYEDAPRO::from_json( const nlohmann::json& j, EASYEDAPRO::SCH_ATTR& d )
{
    d.id = j.at( 1 ).get<wxString>();
    d.parentId = j.at( 2 ).get<wxString>();
    d.key = j.at( 3 ).get<wxString>();

    if( j.at( 4 ).is_string() )
        d.value = j.at( 4 ).get<wxString>();

    if( j.at( 5 ).is_number() )
        d.keyVisible = j.at( 5 ).get<int>();
    else if( j.at( 5 ).is_boolean() )
        d.keyVisible = j.at( 5 ).get<bool>();

    if( j.at( 6 ).is_number() )
        d.valVisible = j.at( 6 ).get<int>();
    else if( j.at( 6 ).is_boolean() )
        d.valVisible = j.at( 6 ).get<bool>();

    if( j.at( 7 ).is_number() && j.at( 8 ).is_number() )
        d.position = VECTOR2D( j.at( 7 ), j.at( 8 ) );

    if( j.at( 9 ).is_number() )
        d.rotation = j.at( 9 );

    if( j.at( 10 ).is_string() )
        d.fontStyle = j.at( 10 ).get<wxString>();
}


void EASYEDAPRO::from_json( const nlohmann::json& j, EASYEDAPRO::PCB_ATTR& d )
{
    d.id = j.at( 1 ).get<wxString>();
    d.parentId = j.at( 3 ).get<wxString>();
    d.layer = j.at( 4 );

    if( j.at( 5 ).is_number() && j.at( 6 ).is_number() )
        d.position = VECTOR2D( j.at( 5 ), j.at( 6 ) );

    d.key = j.at( 7 ).get<wxString>();

    if( j.at( 8 ).is_string() )
        d.value = j.at( 8 ).get<wxString>();
    else if( j.at( 8 ).is_number_integer() )
        d.value << ( j.at( 8 ).get<int>() );

    if( j.at( 9 ).is_number() )
        d.keyVisible = j.at( 9 ).get<int>();
    else if( j.at( 9 ).is_boolean() )
        d.keyVisible = j.at( 9 ).get<bool>();

    if( j.at( 10 ).is_number() )
        d.valVisible = j.at( 10 ).get<int>();
    else if( j.at( 10 ).is_boolean() )
        d.valVisible = j.at( 10 ).get<bool>();

    if( j.at( 11 ).is_string() )
        d.fontName = j.at( 11 ).get<wxString>();

    if( j.at( 12 ).is_number() )
        d.height = j.at( 12 );

    if( j.at( 13 ).is_number() )
        d.strokeWidth = j.at( 13 );

    if( j.at( 16 ).is_number() )
        d.textOrigin = j.at( 16 );

    if( j.at( 17 ).is_number() )
        d.rotation = j.at( 17 );

    if( j.at( 18 ).is_number() )
        d.inverted = j.at( 18 );
}


void EASYEDAPRO::from_json( const nlohmann::json& j, EASYEDAPRO::SCH_COMPONENT& d )
{
    d.id = j.at( 1 ).get<wxString>();
    d.name = j.at( 2 ).get<wxString>();

    if( j.at( 3 ).is_number() && j.at( 4 ).is_number() )
        d.position = VECTOR2D( j.at( 3 ), j.at( 4 ) );

    if( j.at( 5 ).is_number() )
        d.rotation = j.at( 5 );

    if( j.at( 6 ).is_number() )
        d.mirror = j.at( 6 ).get<int>();

    if( j.at( 6 ).is_number() )
        d.unk1 = j.at( 6 );

    if( j.at( 7 ).is_object() )
        d.customProps = j.at( 7 );

    if( j.at( 8 ).is_number() )
        d.unk2 = j.at( 8 );
}


void EASYEDAPRO::from_json( const nlohmann::json& j, EASYEDAPRO::SCH_WIRE& d )
{
    d.id = j.at( 1 ).get<wxString>();
    d.geometry = j.at( 2 );

    if( j.at( 3 ).is_string() )
        d.lineStyle = j.at( 3 ).get<wxString>();
}


void EASYEDAPRO::from_json( const nlohmann::json& j, EASYEDAPRO::SYM_PIN& d )
{
    d.id = j.at( 1 ).get<wxString>();

    if( j.at( 4 ).is_number() && j.at( 5 ).is_number() )
        d.position = VECTOR2D( j.at( 4 ), j.at( 5 ) );

    if( j.at( 6 ).is_number() )
        d.length = j.at( 6 );

    if( j.at( 7 ).is_number() )
        d.rotation = j.at( 7 );

    if( j.at( 9 ).is_number() )
        d.inverted = j.at( 9 ).get<int>() == 2;
}


void EASYEDAPRO::from_json( const nlohmann::json& j, EASYEDAPRO::SYM_HEAD& d )
{
    if( !j.at( 1 ).is_object() )
        return;

    nlohmann::json config = j.at( 1 );

    d.origin.x = config.value( "originX", 0 );
    d.origin.y = config.value( "originY", 0 );
    d.maxId = config.value( "maxId", 0 );
    d.version = config.value( "version", "" );
    d.symbolType = config.value( "symbolType", SYMBOL_TYPE::NORMAL );

}


void EASYEDAPRO::from_json( const nlohmann::json& j, EASYEDAPRO::PRJ_SHEET& d )
{
    d.name = j.value( "name", "" );
    d.uuid = j.value( "uuid", "" );
    d.id = j.value( "id", 0 );
}


void EASYEDAPRO::from_json( const nlohmann::json& j, EASYEDAPRO::PRJ_SCHEMATIC& d )
{
    d.name = j.value( "name", "" );
    d.sheets = j.value( "sheets", std::vector<PRJ_SHEET>{} );
}


void EASYEDAPRO::from_json( const nlohmann::json& j, EASYEDAPRO::PRJ_BOARD& d )
{
    d.schematic = j.value( "schematic", "" );
    d.pcb = j.value( "pcb", "" );
}


void EASYEDAPRO::from_json( const nlohmann::json& j, EASYEDAPRO::PRJ_SYMBOL& d )
{
    if( j.at( "source" ).is_string() )
        d.source = j.at( "source" ).get<wxString>();

    if( j.contains( "desc" ) )
        d.desc = j.at( "desc" ).get<wxString>();
    else if( j.contains( "description" ) )
        d.desc = j.at( "description" ).get<wxString>();

    if( j.contains( "display_title" ) )
        d.title = j.at( "display_title" ).get<wxString>();
    else if( j.contains( "title" ) )
        d.title = j.at( "title" ).get<wxString>();

    if( j.at( "version" ).is_string() )
        d.version = j.at( "version" ).get<wxString>();

    if( j.at( "type" ).is_number() )
        d.type = j.at( "type" );

    if( j.find( "tags" ) != j.end() && j.at( "tags" ).is_object() )
        d.tags = j.at( "tags" );

    if( j.find( "custom_tags" ) != j.end() && j.at( "custom_tags" ).is_object() )
        d.custom_tags = j.at( "custom_tags" );
}


void EASYEDAPRO::from_json( const nlohmann::json& j, EASYEDAPRO::PRJ_FOOTPRINT& d )
{
    if( j.at( "source" ).is_string() )
        d.source = j.at( "source" ).get<wxString>();

    if( j.contains( "desc" ) )
        d.desc = j.at( "desc" ).get<wxString>();
    else if( j.contains( "description" ) )
        d.desc = j.at( "description" ).get<wxString>();

    if( j.contains( "display_title" ) )
        d.title = j.at( "display_title" ).get<wxString>();
    else if( j.contains( "title" ) )
        d.title = j.at( "title" ).get<wxString>();

    if( j.at( "version" ).is_string() )
        d.version = j.at( "version" ).get<wxString>();

    if( j.at( "type" ).is_number() )
        d.type = j.at( "type" );

    if( j.find( "tags" ) != j.end() && j.at( "tags" ).is_object() )
        d.tags = j.at( "tags" );

    if( j.find( "custom_tags" ) != j.end() && j.at( "custom_tags" ).is_object() )
        d.custom_tags = j.at( "custom_tags" );
}


void EASYEDAPRO::from_json( const nlohmann::json& j, EASYEDAPRO::PRJ_DEVICE& d )
{
    if( j.at( "source" ).is_string() )
        d.source = j.at( "source" ).get<wxString>();

    if( j.contains( "desc" ) )
        d.description = j.at( "desc" ).get<wxString>();
    else if( j.contains( "description" ) )
        d.description = j.at( "description" ).get<wxString>();

    if( j.contains( "display_title" ) )
        d.title = j.at( "display_title" ).get<wxString>();
    else if( j.contains( "title" ) )
        d.title = j.at( "title" ).get<wxString>();

    if( j.at( "version" ).is_string() )
        d.version = j.at( "version" ).get<wxString>();

    if( j.find( "tags" ) != j.end() && j.at( "tags" ).is_object() )
        d.tags = j.at( "tags" );

    if( j.find( "custom_tags" ) != j.end() && j.at( "custom_tags" ).is_object() )
        d.custom_tags = j.at( "custom_tags" );

    if( j.at( "attributes" ).is_object() )
        d.attributes = AnyMapToStringMap( j.at( "attributes" ) );
}


void EASYEDAPRO::from_json( const nlohmann::json& j, EASYEDAPRO::BLOB& d )
{
    d.objectId = j.at( 1 ).get<wxString>();
    d.url = j.at( 3 ).get<wxString>();
}


void EASYEDAPRO::from_json( const nlohmann::json& j, EASYEDAPRO::POURED& d )
{
    d.pouredId = j.at( 1 ).get<wxString>();
    d.parentId = j.at( 2 ).get<wxString>();
    d.unki = j.at( 3 ).get<int>();
    d.isPoly = j.at( 4 ).get<bool>();
    d.polyData = j.at( 5 );
}
