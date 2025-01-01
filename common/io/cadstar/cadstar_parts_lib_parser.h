/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CADSTAR_PARTS_LIB_PARSER_H
#define CADSTAR_PARTS_LIB_PARSER_H

#include <filesystem>
#include <string>

#include "cadstar_parts_lib_model.h"

class CADSTAR_PARTS_LIB_PARSER
{
public:
    CADSTAR_PARTS_LIB_PARSER(){};
    ~CADSTAR_PARTS_LIB_PARSER(){};

    bool CheckContentHeader( const std::string& aSource ) const;

    bool CheckFileHeader( const std::filesystem::path& aPath ) const;

    bool CheckFileHeader( const std::string& aPath ) const
    {
        return CheckFileHeader( std::filesystem::path( aPath ) );
    };


    CADSTAR_PARTS_LIB_MODEL ReadContent( const std::string& aSource ) const;

    CADSTAR_PARTS_LIB_MODEL ReadFile( const std::filesystem::path& aPath ) const;

    CADSTAR_PARTS_LIB_MODEL ReadFile( const std::string& aPath ) const
    {
        return ReadFile( std::filesystem::path( aPath ) );
    };
};

#endif //CADSTAR_PARTS_LIB_PARSER_H

