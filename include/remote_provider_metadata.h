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

#ifndef REMOTE_PROVIDER_METADATA_H
#define REMOTE_PROVIDER_METADATA_H

#include <optional>
#include <vector>

#include <kicommon.h>
#include <json_common.h>
#include <wx/filename.h>
#include <wx/string.h>

enum class REMOTE_PROVIDER_AUTH_TYPE
{
    NONE,
    OAUTH2
};


struct KICOMMON_API REMOTE_PROVIDER_AUTH_METADATA
{
    REMOTE_PROVIDER_AUTH_TYPE type = REMOTE_PROVIDER_AUTH_TYPE::NONE;
    wxString                  metadata_url;
    wxString                  client_id;
    std::vector<wxString>     scopes;
};


struct KICOMMON_API REMOTE_PROVIDER_METADATA
{
    wxString                      provider_name;
    wxString                      provider_version;
    wxString                      api_base_url;
    wxString                      panel_url;
    wxString                      session_bootstrap_url;
    REMOTE_PROVIDER_AUTH_METADATA auth;
    bool                          web_ui_v1 = false;
    bool                          parts_v1 = false;
    bool                          direct_downloads_v1 = false;
    bool                          inline_payloads_v1 = false;
    long long                     max_download_bytes = 0;
    std::vector<wxString>         supported_asset_types;
    wxString                      parts_endpoint_template;
    wxString                      documentation_url;
    wxString                      terms_url;
    wxString                      privacy_url;
    bool                          allow_insecure_localhost = false;

    static wxFileName DefaultSchemaPath();

    static std::optional<REMOTE_PROVIDER_METADATA> FromJson( const nlohmann::json& aJson,
                                                             wxString& aError );

    static std::optional<REMOTE_PROVIDER_METADATA> FromJson( const nlohmann::json& aJson,
                                                             const wxFileName& aSchemaFile,
                                                             wxString& aError );
};

#endif // REMOTE_PROVIDER_METADATA_H
