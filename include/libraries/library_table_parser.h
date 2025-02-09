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

#ifndef LIBRARY_TABLE_PARSER_H
#define LIBRARY_TABLE_PARSER_H

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <tl/expected.hpp>
#include <wx/wx.h>
#include <libraries/library_table.h>

struct LIBRARY_TABLE_ROW_IR
{
    std::string nickname;
    std::string uri;
    std::string type;
    std::string options;
    std::string description;
    bool disabled = false;
    bool hidden = false;
};

/**
 * The intermediate representation that a library table is parsed into
 */
struct LIBRARY_TABLE_IR
{
    std::string version;
    LIBRARY_TABLE_TYPE type;
    std::vector<LIBRARY_TABLE_ROW_IR> rows;
};

struct LIBRARY_PARSE_ERROR
{
    wxString description;
    size_t line = 0;
    size_t column = 0;
};

class KICOMMON_API LIBRARY_TABLE_PARSER
{
public:
    LIBRARY_TABLE_PARSER();

    tl::expected<LIBRARY_TABLE_IR, LIBRARY_PARSE_ERROR> Parse( const std::filesystem::path& aPath );

    tl::expected<LIBRARY_TABLE_IR, LIBRARY_PARSE_ERROR> ParseBuffer( const std::string& aBuffer );

private:

};

#endif //LIBRARY_TABLE_PARSER_H
