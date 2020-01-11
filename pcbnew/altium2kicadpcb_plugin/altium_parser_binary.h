/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Thomas Pointhuber <thomas.pointhuber@gmx.at>
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

#ifndef ALTIUM_PARSER_BINARY_H
#define ALTIUM_PARSER_BINARY_H

#include <memory>

#include <wx/gdicmn.h>


namespace CFB {
    class CompoundFileReader;
    struct COMPOUND_FILE_ENTRY;
}


class ALTIUM_PARSER_BINARY {
public:
    ALTIUM_PARSER_BINARY( const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry );
    ~ALTIUM_PARSER_BINARY();

    template<typename Type>
    Type read() {
        if (bytes_remaining() >= sizeof(Type)) {
            Type val = *(Type *) (pos);
            pos += sizeof(Type);
            return val;
        } else {
            error = true;
            return 0;
        }
    }

    std::string read_string() {
        u_int8_t len = read<u_int8_t>();
        if(bytes_remaining() >= len) {
            std::string val = std::string(pos, len);
            pos += len;
            return val;
        } else {
            error = true;
            return "";
        }
    }

    wxPoint read_point() {
        int32_t x = kicad_x( read<int32_t>() );
        int32_t y = kicad_y( read<int32_t>()) ;
        return {x, y};
    }

    wxSize read_size() {
        int32_t x = kicad_unit( read<int32_t>() );
        int32_t y = kicad_unit( read<int32_t>() );
        return {x, y};
    }

    static int32_t kicad_unit( const int32_t x ) {
        return (((int64_t) x) * 256L) / 100;
    }

    static int32_t kicad_x( const int32_t x ) {
        return kicad_unit( x );
    }

    static int32_t kicad_y( const int32_t y ) {
        return -kicad_unit( y );
    }

    void skip(size_t len) {
        if( bytes_remaining() >= len )
        {
            pos += len;
        }
        else
        {
            error = true;
        }
    }

    size_t bytes_remaining() const {
        return pos == nullptr ? 0 : size - (pos - content.get());
    }

    bool parser_error() {
        return error;
    }

private:

    std::unique_ptr<char[]> content;
    size_t size;

    char* pos;  // current read pointer
    bool error;
};


#endif //ALTIUM_PARSER_BINARY_H
