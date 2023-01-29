/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <filesystem>
#include <iostream>
#include <string>

#include <argparse/argparse.hpp>
#include <fmt.h>
#include <nlohmann/json.hpp>

#define MAGIC_ENUM_RANGE_MAX 1024
#include <magic_enum.hpp>

#include <layer_ids.h>
#include <eda_shape.h>
#include <core/typeinfo.h>


template<typename T>
nlohmann::json FormatEnum()
{
    nlohmann::json js;

    js["type"] = magic_enum::enum_type_name<T>();
    js["values"] = nlohmann::json::array();

    for( const std::pair<T, std::string_view>& entry : magic_enum::enum_entries<T>() )
    {
        js["values"].emplace_back( nlohmann::json( {
                { "key", entry.second },
                { "value", static_cast<int>( entry.first ) }
            } ) );
    }

    return js;
}


int main( int argc, char* argv[] )
{
    argparse::ArgumentParser args( "enum_exporter" );

    args.add_argument( "output_dir" ).default_value( std::string() );

    try
    {
        args.parse_args( argc, argv );
    }
    catch( const std::runtime_error& err )
    {
        std::cerr << err.what() << std::endl;
        std::cerr << args;
        std::exit( 1 );
    }

    std::filesystem::path path( args.get<std::string>( "output_dir" ) );
    std::ofstream outfile;

    if( !path.empty() )
    {
        path = std::filesystem::absolute( path );
        outfile.open( path );
    }

    std::ostream& out = outfile.is_open() ? outfile : std::cout;

    nlohmann::json js = nlohmann::json::array();

    js += FormatEnum<PCB_LAYER_ID>();
    js += FormatEnum<SHAPE_T>();
    js += FormatEnum<KICAD_T>();

    out << js.dump( 4 ) << std::endl;
}
