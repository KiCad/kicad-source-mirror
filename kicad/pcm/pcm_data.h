/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Andrew Lutsenko, anlutsenko at gmail dot com
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <boost/optional.hpp>
#include <map>
#include <nlohmann/json.hpp>
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
    PT_LIBRARY,
    PT_COLORTHEME,
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
    boost::optional<int>       version_epoch;
    boost::optional<wxString>  download_url;
    boost::optional<wxString>  download_sha256;
    boost::optional<uint64_t>  download_size;
    boost::optional<uint64_t>  install_size;
    PCM_PACKAGE_VERSION_STATUS status;
    std::vector<std::string>   platforms;
    wxString                   kicad_version;
    boost::optional<wxString>  kicad_version_max;

    // Not serialized fields
    std::tuple<int, int, int, int> parsed_version; // Full version tuple for sorting
    bool                           compatible;
};


///< Package metadata
struct PCM_PACKAGE
{
    wxString                     name;
    wxString                     description;
    wxString                     description_full;
    wxString                     identifier;
    PCM_PACKAGE_TYPE             type;
    PCM_CONTACT                  author;
    boost::optional<PCM_CONTACT> maintainer;
    wxString                     license;
    STRING_MAP                   resources;
    std::vector<std::string>     tags;
    std::vector<PACKAGE_VERSION> versions;
};


///< Repository reference to a resource
struct PCM_RESOURCE_REFERENCE
{
    wxString                  url;
    boost::optional<wxString> sha256;
    uint64_t                  update_timestamp;
};


///< Repository metadata
struct PCM_REPOSITORY
{
    wxString                                name;
    PCM_RESOURCE_REFERENCE                  packages;
    boost::optional<PCM_RESOURCE_REFERENCE> resources;
    boost::optional<PCM_RESOURCE_REFERENCE> manifests;
    boost::optional<PCM_CONTACT>            maintainer;

    // Not serialized fields
    std::vector<PCM_PACKAGE> package_list;
};


///< Package installation entry
struct PCM_INSTALLATION_ENTRY
{
    PCM_PACKAGE package;
    wxString    current_version;
    wxString    repository_id;
    wxString    repository_name;
    uint64_t    install_timestamp;
};


// Teaching json en/decoder to understand wxStrings
namespace nlohmann
{
template <>
struct adl_serializer<wxString>
{
    static void to_json( json& j, const wxString& s ) { j = s.ToUTF8(); }

    static void from_json( const json& j, wxString& s )
    {
        s = wxString::FromUTF8( j.get<std::string>().c_str() );
    }
};
} // namespace nlohmann


NLOHMANN_JSON_SERIALIZE_ENUM( PCM_PACKAGE_TYPE, {
                                                        { PT_INVALID, "invalid" },
                                                        { PT_PLUGIN, "plugin" },
                                                        { PT_LIBRARY, "library" },
                                                        { PT_COLORTHEME, "colortheme" },
                                                } )


NLOHMANN_JSON_SERIALIZE_ENUM( PCM_PACKAGE_VERSION_STATUS,
                              {
                                      { PVS_INVALID, "invalid" },
                                      { PVS_STABLE, "stable" },
                                      { PVS_TESTING, "testing" },
                                      { PVS_DEVELOPMENT, "development" },
                                      { PVS_DEPRECATED, "deprecated" },
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


NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE( PCM_INSTALLATION_ENTRY, package, current_version, repository_id,
                                    repository_name, install_timestamp );


#endif // PCM_DATA_H_
