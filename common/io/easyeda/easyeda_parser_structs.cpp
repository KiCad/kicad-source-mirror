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

#include "easyeda_parser_structs.h"

#include <json_common.h>
#include <core/json_serializers.h>


#define PARSE_VALUE( name )                                                                        \
    if( j.find( #name ) != j.end() )                                                               \
    j.at( #name ).get_to( d.name )

#define PARSE_TO_DOUBLE( name, def )                                                               \
    if( j.find( #name ) == j.end() )                                                               \
    {                                                                                              \
        d.name = def;                                                                              \
    }                                                                                              \
    else if( j.at( #name ).is_string() )                                                           \
    {                                                                                              \
        wxString str = j.at( #name ).get<wxString>();                                              \
                                                                                                   \
        double out = 0;                                                                            \
        str.ToCDouble( &out );                                                                     \
        d.name = out;                                                                              \
    }                                                                                              \
    else if( j.at( #name ).is_number() )                                                           \
    {                                                                                              \
        d.name = j.at( #name ).get<double>();                                                      \
    }


void EASYEDA::from_json( const nlohmann::json& j, EASYEDA::DOC_TYPE& d )
{
    if( j.is_string() )
    {
        wxString str = j.get<wxString>();

        int out = 0;
        str.ToInt( &out );
        d = static_cast<EASYEDA::DOC_TYPE>( out );
    }
    else if( j.is_number() )
    {
        d = static_cast<EASYEDA::DOC_TYPE>( j.get<int>() );
    }
}


void EASYEDA::from_json( const nlohmann::json& j, EASYEDA::HEAD& d )
{
    PARSE_VALUE( docType );

    PARSE_VALUE( editorVersion );
    PARSE_VALUE( title );
    PARSE_VALUE( description );

    if( j.find( "c_para" ) != j.end() && j.at( "c_para" ).is_object() )
        d.c_para = j.at( "c_para" );

    PARSE_TO_DOUBLE( x, 0 );
    PARSE_TO_DOUBLE( y, 0 );
}


void EASYEDA::from_json( const nlohmann::json& j, EASYEDA::DOCUMENT& d )
{
    PARSE_VALUE( docType );
    PARSE_VALUE( head );

    PARSE_VALUE( canvas );
    PARSE_VALUE( title );
    PARSE_VALUE( shape );
    PARSE_VALUE( dataStr );
}


void EASYEDA::from_json( const nlohmann::json& j, EASYEDA::DOCUMENT_PCB& d )
{
    PARSE_VALUE( c_para );
    d.layers = j.at( "layers" );

    if( j.find( "DRCRULE" ) != j.end() && j.at( "DRCRULE" ).is_object() )
        d.DRCRULE = j.at( "DRCRULE" );
}


void EASYEDA::from_json( const nlohmann::json& j, EASYEDA::DOCUMENT_SYM& d )
{
    PARSE_VALUE( c_para );
}


void EASYEDA::from_json( const nlohmann::json& j, EASYEDA::DOCUMENT_SCHEMATICS& d )
{
    PARSE_VALUE( schematics );
}


void EASYEDA::from_json( const nlohmann::json& j, EASYEDA::C_PARA& d )
{
    PARSE_VALUE( package );
    PARSE_VALUE( pre );
    PARSE_VALUE( Contributor );
    PARSE_VALUE( link );
    PARSE_VALUE( Model_3D );
}
