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

#pragma once

#include <vector>

#include <json_conversions.h>
#include <kicommon.h>
#include <wx/string.h>

#include <nlohmann/json_fwd.hpp>


struct KICOMMON_API REMOTE_PROVIDER_ENTRY
{
    wxString provider_id;
    wxString metadata_url;
    wxString display_name_override;
    wxString last_account_label;
    wxString last_auth_status;

    bool operator==( const REMOTE_PROVIDER_ENTRY& aOther ) const = default;
};


struct KICOMMON_API REMOTE_PROVIDER_SETTINGS
{
    REMOTE_PROVIDER_SETTINGS()
    {
        ResetToDefaults();
    }

    std::vector<REMOTE_PROVIDER_ENTRY> providers;
    wxString                           last_used_provider_id;
    wxString                           destination_dir;
    wxString                           library_prefix;
    bool                               add_to_global_table;

    void ResetToDefaults();

    static wxString DefaultDestinationDir();
    static wxString DefaultLibraryPrefix();
    static wxString CreateProviderId( const wxString& aMetadataUrl );

    REMOTE_PROVIDER_ENTRY* FindProviderById( const wxString& aProviderId );
    const REMOTE_PROVIDER_ENTRY* FindProviderById( const wxString& aProviderId ) const;
    REMOTE_PROVIDER_ENTRY* FindProviderByMetadataUrl( const wxString& aMetadataUrl );
    const REMOTE_PROVIDER_ENTRY* FindProviderByMetadataUrl( const wxString& aMetadataUrl ) const;
    REMOTE_PROVIDER_ENTRY& UpsertProvider( const wxString& aMetadataUrl );
};


KICOMMON_API void to_json( nlohmann::json& aJson, const REMOTE_PROVIDER_ENTRY& aEntry );
KICOMMON_API void from_json( const nlohmann::json& aJson, REMOTE_PROVIDER_ENTRY& aEntry );
KICOMMON_API void to_json( nlohmann::json& aJson, const REMOTE_PROVIDER_SETTINGS& aSettings );
KICOMMON_API void from_json( const nlohmann::json& aJson, REMOTE_PROVIDER_SETTINGS& aSettings );
