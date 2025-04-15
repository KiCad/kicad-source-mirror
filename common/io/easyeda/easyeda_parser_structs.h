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

#ifndef EASYEDA_PARSER_STRUCTS_H_
#define EASYEDA_PARSER_STRUCTS_H_

#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>
#include <map>
#include <optional>

#include <wx/string.h>
#include <wx/arrstr.h>

#include <json_common.h>

namespace EASYEDA
{

enum class DOC_TYPE
{
    UNKNOWN = 0,

    SCHEMATIC_SHEET = 1,
    SYMBOL = 2,
    PCB = 3,
    PCB_COMPONENT = 4,
    SCHEMATIC_LIST = 5,
    PCB_MODULE = 14,
};

struct HEAD
{
    DOC_TYPE docType = DOC_TYPE::UNKNOWN;

    wxString editorVersion;
    wxString title;
    wxString description;

    double x = 0;
    double y = 0;

    std::optional<std::map<wxString, wxString>> c_para;
};

struct DOCUMENT
{
    std::optional<DOC_TYPE> docType; // May be here or in head
    HEAD head;

    // BBox
    // colors
    wxString                      canvas;
    wxString                      title;
    wxArrayString                 shape;
    std::optional<nlohmann::json> dataStr;
};

struct C_PARA
{
    wxString package;
    wxString pre;
    wxString Contributor;
    wxString link;
    wxString Model_3D;
};

struct DOCUMENT_PCB
{
    std::optional<std::map<wxString, wxString>>       c_para;
    std::vector<wxString>                             layers;
    std::optional<wxString>                           uuid;
    std::optional<std::map<wxString, nlohmann::json>> DRCRULE;
};

struct DOCUMENT_SYM
{
    std::optional<std::map<wxString, wxString>> c_para;
};

struct DOCUMENT_SCHEMATICS
{
    std::optional<std::vector<DOCUMENT>> schematics;
};

void from_json( const nlohmann::json& j, EASYEDA::DOC_TYPE& d );
void from_json( const nlohmann::json& j, EASYEDA::HEAD& d );
void from_json( const nlohmann::json& j, EASYEDA::DOCUMENT& d );
void from_json( const nlohmann::json& j, EASYEDA::C_PARA& d );
void from_json( const nlohmann::json& j, EASYEDA::DOCUMENT_PCB& d );
void from_json( const nlohmann::json& j, EASYEDA::DOCUMENT_SYM& d );
void from_json( const nlohmann::json& j, EASYEDA::DOCUMENT_SCHEMATICS& d );

enum class POWER_FLAG_STYLE
{
    UNKNOWN = -1,

    CIRCLE = 0,
    ARROW = 1,
    BAR = 2,
    WAVE = 3,
    POWER_GROUND = 4,
    SIGNAL_GROUND = 5,
    EARTH = 6,
    GOST_ARROW = 7,
    GOST_POWER_GROUND = 8,
    GOST_EARTH = 9,
    GOST_BAR = 10
};

} // namespace EASYEDA


#endif // EASYEDA_PARSER_STRUCTS_H_
