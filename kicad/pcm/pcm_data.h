/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Andrew Lutsenko, anlutsenko at gmail dot com
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

#ifndef PCM_DATA_H_
#define PCM_DATA_H_

#include "core/wx_stl_compat.h"

#include <map>
#include <json_common.h>
#include <core/json_serializers.h>
#include <optional>
#include <string>
#include <tuple>
#include <vector>
#include <wx/string.h>


using STRING_MAP = std::map<std::string, wxString>;
using nlohmann::json;


///< Supported package types
enum PCM_PACKAGE_TYPE
{
    PT_INVALID,
    PT_PLUGIN,
    PT_FAB,
    PT_LIBRARY,
    PT_DATASOURCE,
    PT_COLORTHEME,
};

///< Plugin categories
enum PCM_PLUGIN_CATEGORY
{
    PC_INVALID,
    PC_GENERAL,
    PC_FAB,
};


///< Status of specific package version
enum PCM_PACKAGE_VERSION_STATUS
{
    PVS_INVALID,
    PVS_STABLE,
    PVS_TESTING,
    PVS_DEVELOPMENT,
    PVS_DEPRECATED
};

///< The runtime a plugin package uses
enum class PCM_PACKAGE_RUNTIME
{
    PPR_SWIG,
    PPR_IPC
};


///< Describes a person's name and contact information
struct PCM_CONTACT
{
    wxString   name;
    STRING_MAP contact;
};


// MSVC, wxWidgets and nlohmann_json don't play nice together and
// create linker errors about redefinition of some vector members
// if an attempt to use vector<wxString> in json is made.

///< Package version metadata
struct PACKAGE_VERSION
{
    wxString                   version;
    std::optional<int>         version_epoch;
    std::optional<wxString>    download_url;
    std::optional<wxString>    download_sha256;
    std::optional<uint64_t>    download_size;
    std::optional<uint64_t>    install_size;
    PCM_PACKAGE_VERSION_STATUS status;
    std::vector<std::string>   platforms;
    wxString                   kicad_version;
    std::optional<wxString>    kicad_version_max;
    std::vector<std::string>   keep_on_update;
    std::optional<PCM_PACKAGE_RUNTIME> runtime;

    // Not serialized fields
    std::tuple<int, int, int, int> parsed_version; // Full version tuple for sorting
    bool                           compatible = true;
};


///< Package metadata
struct PCM_PACKAGE
{
    wxString                           name;
    wxString                           description;
    wxString                           description_full;
    wxString                           identifier;
    PCM_PACKAGE_TYPE                   type;
    std::optional<PCM_PLUGIN_CATEGORY> category;
    PCM_CONTACT                        author;
    std::optional<PCM_CONTACT>         maintainer;
    wxString                           license;
    STRING_MAP                         resources;
    std::vector<std::string>           tags;
    std::vector<std::string>           keep_on_update;
    std::vector<PACKAGE_VERSION>       versions;
};


///< Repository reference to a resource
struct PCM_RESOURCE_REFERENCE
{
    wxString                url;
    std::optional<wxString> sha256;
    uint64_t                update_timestamp;
};


///< Repository metadata
struct PCM_REPOSITORY
{
    wxString                              name;
    PCM_RESOURCE_REFERENCE                packages;
    std::optional<PCM_RESOURCE_REFERENCE> resources;
    std::optional<PCM_RESOURCE_REFERENCE> manifests;
    std::optional<PCM_CONTACT>            maintainer;

    // Not serialized fields
    std::vector<PCM_PACKAGE> package_list;
    // pkg id to index of package from package_list for quick lookup
    std::unordered_map<wxString, size_t> package_map;
};


///< Package installation entry
struct PCM_INSTALLATION_ENTRY
{
    PCM_PACKAGE package;
    wxString    current_version;
    wxString    repository_id;
    wxString    repository_name;
    uint64_t    install_timestamp;
    bool        pinned;

    // Not serialized fields
    bool update_available;
};


NLOHMANN_JSON_SERIALIZE_ENUM( PCM_PACKAGE_TYPE, {
                                                        { PT_INVALID, "invalid" },
                                                        { PT_PLUGIN, "plugin" },
                                                        { PT_FAB, "fab" },
                                                        { PT_LIBRARY, "library" },
                                                        { PT_DATASOURCE, "datasource" },
                                                        { PT_COLORTHEME, "colortheme" },
                                                } )

NLOHMANN_JSON_SERIALIZE_ENUM( PCM_PLUGIN_CATEGORY, {
                                                        { PC_INVALID, "invalid" },
                                                        { PC_GENERAL, "general" },
                                                        { PC_FAB, "fab" },
                                                } )


NLOHMANN_JSON_SERIALIZE_ENUM( PCM_PACKAGE_VERSION_STATUS,
                              {
                                      { PVS_INVALID, "invalid" },
                                      { PVS_STABLE, "stable" },
                                      { PVS_TESTING, "testing" },
                                      { PVS_DEVELOPMENT, "development" },
                                      { PVS_DEPRECATED, "deprecated" },
                              } )

NLOHMANN_JSON_SERIALIZE_ENUM( PCM_PACKAGE_RUNTIME,
                              {
                                      { PCM_PACKAGE_RUNTIME::PPR_SWIG, "swig" },
                                      { PCM_PACKAGE_RUNTIME::PPR_IPC, "ipc" },
                              } )


// Following are templates and definitions for en/decoding above structs
// to/from json

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE( PCM_CONTACT, name, contact )


void to_json( json& j, const PACKAGE_VERSION& v );
void from_json( const json& j, PACKAGE_VERSION& v );


void to_json( json& j, const PCM_PACKAGE& p );
void from_json( const json& j, PCM_PACKAGE& p );


void to_json( json& j, const PCM_RESOURCE_REFERENCE& r );
void from_json( const json& j, PCM_RESOURCE_REFERENCE& r );


void to_json( json& j, const PCM_REPOSITORY& r );
void from_json( const json& j, PCM_REPOSITORY& r );


void to_json( json& j, const PCM_INSTALLATION_ENTRY& e );
void from_json( const json& j, PCM_INSTALLATION_ENTRY& e );


#endif // PCM_DATA_H_
