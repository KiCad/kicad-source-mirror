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

#ifndef REMOTE_SYMBOL_IMPORT_JOB_H
#define REMOTE_SYMBOL_IMPORT_JOB_H

#include <memory>

#include <remote_provider_metadata.h>
#include <remote_provider_models.h>
#include <remote_symbol_download_manager.h>


class SCH_EDIT_FRAME;

struct REMOTE_SYMBOL_IMPORT_CONTEXT
{
    wxString symbol_name;
    wxString library_name;
};


class REMOTE_SYMBOL_IMPORT_JOB
{
public:
    explicit REMOTE_SYMBOL_IMPORT_JOB( SCH_EDIT_FRAME* aFrame,
                                       REMOTE_SYMBOL_DOWNLOAD_MANAGER* aDownloader = nullptr );

    bool Import( const REMOTE_PROVIDER_METADATA& aProvider, const REMOTE_SYMBOL_IMPORT_CONTEXT& aContext,
                 const REMOTE_PROVIDER_PART_MANIFEST& aManifest, bool aPlaceSymbol, wxString& aError );

private:
    const REMOTE_SYMBOL_DOWNLOAD_MANAGER& downloader() const;

private:
    SCH_EDIT_FRAME*                                m_frame;
    REMOTE_SYMBOL_DOWNLOAD_MANAGER*                m_downloader;
    std::unique_ptr<REMOTE_SYMBOL_DOWNLOAD_MANAGER> m_ownedDownloader;
};

#endif // REMOTE_SYMBOL_IMPORT_JOB_H
