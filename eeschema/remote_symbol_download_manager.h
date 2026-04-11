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

#ifndef REMOTE_SYMBOL_DOWNLOAD_MANAGER_H
#define REMOTE_SYMBOL_DOWNLOAD_MANAGER_H

#include <functional>
#include <vector>

#include <remote_provider_metadata.h>
#include <remote_provider_models.h>
#include <wx/string.h>


struct REMOTE_SYMBOL_FETCH_RESPONSE
{
    int                  status_code = 0;
    wxString             content_type;
    std::vector<uint8_t> payload;
};


struct REMOTE_SYMBOL_FETCHED_ASSET
{
    wxString             content_type;
    std::vector<uint8_t> payload;
};


class REMOTE_SYMBOL_DOWNLOAD_MANAGER
{
public:
    using FETCH_HANDLER =
            std::function<bool( const wxString&, REMOTE_SYMBOL_FETCH_RESPONSE&, wxString& )>;

    REMOTE_SYMBOL_DOWNLOAD_MANAGER();
    explicit REMOTE_SYMBOL_DOWNLOAD_MANAGER( FETCH_HANDLER aHandler );

    bool DownloadAndVerify( const REMOTE_PROVIDER_METADATA& aProvider,
                            const REMOTE_PROVIDER_PART_ASSET& aAsset, long long aRemainingBudget,
                            REMOTE_SYMBOL_FETCHED_ASSET& aFetched, wxString& aError ) const;

private:
    FETCH_HANDLER m_handler;
};

#endif // REMOTE_SYMBOL_DOWNLOAD_MANAGER_H
