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

#ifndef EASYEDAPRO_PARSER_H_
#define EASYEDAPRO_PARSER_H_

#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>
#include <map>
#include <optional>

#include <wx/string.h>

#include <json_common.h>
#include <math/vector2d.h>


namespace EASYEDAPRO
{

static const bool IMPORT_POURED = true;
static const bool IMPORT_POURED_ECOP = false;

enum class SYMBOL_TYPE
{
    NORMAL = 2,
    POWER_PORT = 18,
    NETPORT = 19,
    SHEET_SYMBOL = 20,
    SHORT = 22,
};

enum class FOOTPRINT_TYPE
{
    NORMAL = 4,
};

struct SCH_ATTR
{
    wxString                id;
    wxString                parentId;
    wxString                key;
    wxString                value;
    bool                    keyVisible = false;
    bool                    valVisible = false;
    std::optional<VECTOR2D> position;
    double                  rotation = 0;
    wxString                fontStyle;
};

struct PCB_ATTR
{
    wxString                id;
    int                     layer = 0;
    wxString                parentId;
    int                     textOrigin = 0;
    VECTOR2D                position;
    wxString                key;
    wxString                value;
    bool                    keyVisible = false;
    bool                    valVisible = false;
    wxString                fontName;
    double                  height = 0;
    double                  strokeWidth = 0;
    double                  rotation = 0;
    int                     inverted = 0;
};

struct SCH_COMPONENT
{
    wxString id;
    wxString name;

    VECTOR2D position;
    double   rotation = 0;
    bool     mirror = false;

    nlohmann::json customProps;

    int unk1 = 0;
    int unk2 = 0;
};

struct SCH_WIRE
{
    wxString                         id;
    std::vector<std::vector<double>> geometry;
    wxString                         lineStyle;

    int unk1 = 0;
};

struct SYM_PIN
{
    wxString id;

    VECTOR2D position;
    double   length = 0;
    double   rotation = 0;
    bool     inverted = false;
};

struct SYM_HEAD
{
    VECTOR2D    origin;
    wxString    version;
    int         maxId = 0;
    SYMBOL_TYPE symbolType = SYMBOL_TYPE::NORMAL;
};

struct PRJ_SHEET
{
    int         id = 0;
    wxString    name;
    wxString    uuid;
};

struct PRJ_SCHEMATIC
{
    wxString               name;
    std::vector<PRJ_SHEET> sheets;
};

struct PRJ_BOARD
{
    wxString schematic;
    wxString pcb;
};

struct PRJ_SYMBOL
{
    wxString       source;
    wxString       desc;
    nlohmann::json tags;
    nlohmann::json custom_tags;
    wxString       title;
    wxString       version;
    SYMBOL_TYPE    type = SYMBOL_TYPE::NORMAL;
};

struct PRJ_FOOTPRINT
{
    wxString       source;
    wxString       desc;
    nlohmann::json tags;
    nlohmann::json custom_tags;
    wxString       title;
    wxString       version;
    FOOTPRINT_TYPE type = FOOTPRINT_TYPE::NORMAL;
};

struct PRJ_DEVICE
{
    wxString                     source;
    wxString                     description;
    nlohmann::json               tags;
    nlohmann::json               custom_tags;
    wxString                     title;
    wxString                     version;
    std::map<wxString, wxString> attributes;
};

struct BLOB
{
    wxString objectId;
    wxString url;
};

struct POURED
{
    wxString       pouredId;
    wxString       parentId;
    int            unki = 0;
    bool           isPoly = false;
    nlohmann::json polyData;
};

void from_json( const nlohmann::json& j, EASYEDAPRO::SCH_ATTR& d );
void from_json( const nlohmann::json& j, EASYEDAPRO::PCB_ATTR& d );
void from_json( const nlohmann::json& j, EASYEDAPRO::SCH_COMPONENT& d );
void from_json( const nlohmann::json& j, EASYEDAPRO::SCH_WIRE& d );
void from_json( const nlohmann::json& j, EASYEDAPRO::SYM_PIN& d );
void from_json( const nlohmann::json& j, EASYEDAPRO::SYM_HEAD& d );
void from_json( const nlohmann::json& j, EASYEDAPRO::PRJ_SHEET& d );
void from_json( const nlohmann::json& j, EASYEDAPRO::PRJ_SCHEMATIC& d );
void from_json( const nlohmann::json& j, EASYEDAPRO::PRJ_BOARD& d );
void from_json( const nlohmann::json& j, EASYEDAPRO::PRJ_SYMBOL& d );
void from_json( const nlohmann::json& j, EASYEDAPRO::PRJ_FOOTPRINT& d );
void from_json( const nlohmann::json& j, EASYEDAPRO::PRJ_DEVICE& d );
void from_json( const nlohmann::json& j, EASYEDAPRO::BLOB& d );
void from_json( const nlohmann::json& j, EASYEDAPRO::POURED& d );

} // namespace EASYEDAPRO


#endif // EASYEDAPRO_PARSER_H_
