/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef REMOTE_PROVIDER_MODELS_H
#define REMOTE_PROVIDER_MODELS_H

#include <optional>
#include <vector>

#include <kicommon.h>
#include <json_common.h>
#include <wx/filename.h>
#include <wx/string.h>


enum class REMOTE_PROVIDER_ERROR_TYPE
{
    NONE,
    NETWORK,
    AUTH_REQUIRED,
    ACCESS_DENIED,
    NOT_FOUND,
    SERVER,
    INVALID_RESPONSE
};


struct KICOMMON_API REMOTE_PROVIDER_ERROR
{
    REMOTE_PROVIDER_ERROR_TYPE type = REMOTE_PROVIDER_ERROR_TYPE::NONE;
    int                        http_status = 0;
    wxString                   message;

    void Clear()
    {
        type = REMOTE_PROVIDER_ERROR_TYPE::NONE;
        http_status = 0;
        message.clear();
    }
};


enum class REMOTE_PROVIDER_SIGNIN_STATE
{
    NOT_REQUIRED,
    REQUIRED,
    AVAILABLE
};


struct KICOMMON_API REMOTE_PROVIDER_OAUTH_SERVER_METADATA
{
    wxString issuer;
    wxString authorization_endpoint;
    wxString token_endpoint;
    wxString revocation_endpoint;

    static std::optional<REMOTE_PROVIDER_OAUTH_SERVER_METADATA>
    FromJson( const nlohmann::json& aJson, bool aAllowInsecureLocalhost, wxString& aError );
};

struct KICOMMON_API REMOTE_PROVIDER_PART_ASSET
{
    wxString  asset_type;
    wxString  name;
    wxString  target_library;
    wxString  target_name;
    wxString  content_type;
    long long size_bytes = 0;
    wxString  sha256;
    wxString  download_url;
    bool      required = false;
};


struct KICOMMON_API REMOTE_PROVIDER_PART_MANIFEST
{
    wxString                                part_id;
    wxString                                display_name;
    wxString                                summary;
    wxString                                license;
    std::vector<REMOTE_PROVIDER_PART_ASSET> assets;

    static wxFileName DefaultSchemaPath();

    static std::optional<REMOTE_PROVIDER_PART_MANIFEST> FromJson( const nlohmann::json& aJson,
                                                                  bool aAllowInsecureLocalhost, wxString& aError );

    static std::optional<REMOTE_PROVIDER_PART_MANIFEST> FromJson( const nlohmann::json& aJson,
                                                                  const wxFileName&     aSchemaFile,
                                                                  bool aAllowInsecureLocalhost, wxString& aError );
};

#endif // REMOTE_PROVIDER_MODELS_H
