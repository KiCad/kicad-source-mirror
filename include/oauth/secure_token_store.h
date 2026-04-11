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

#include <memory>
#include <optional>

#include <kicommon.h>
#include <wx/string.h>


struct KICOMMON_API OAUTH_TOKEN_SET
{
    wxString access_token;
    wxString refresh_token;
    wxString id_token;
    wxString token_type;
    wxString scope;
    long long expires_at = 0;

    bool operator==( const OAUTH_TOKEN_SET& aOther ) const = default;
};


class KICOMMON_API OAUTH_SECRET_BACKEND
{
public:
    virtual ~OAUTH_SECRET_BACKEND() = default;

    virtual bool StoreSecret( const wxString& aService, const wxString& aKey,
                              const wxString& aSecret ) = 0;
    virtual bool GetSecret( const wxString& aService, const wxString& aKey,
                            wxString& aSecret ) const = 0;
    virtual bool DeleteSecret( const wxString& aService, const wxString& aKey ) = 0;
};


class KICOMMON_API PLATFORM_SECRET_BACKEND : public OAUTH_SECRET_BACKEND
{
public:
    bool StoreSecret( const wxString& aService, const wxString& aKey,
                      const wxString& aSecret ) override;
    bool GetSecret( const wxString& aService, const wxString& aKey,
                    wxString& aSecret ) const override;
    bool DeleteSecret( const wxString& aService, const wxString& aKey ) override;
};


class KICOMMON_API SECURE_TOKEN_STORE
{
public:
    explicit SECURE_TOKEN_STORE(
            std::unique_ptr<OAUTH_SECRET_BACKEND> aBackend = std::make_unique<PLATFORM_SECRET_BACKEND>() );

    bool StoreTokens( const wxString& aProviderId, const wxString& aAccountId,
                      const OAUTH_TOKEN_SET& aTokens );
    std::optional<OAUTH_TOKEN_SET> LoadTokens( const wxString& aProviderId,
                                               const wxString& aAccountId ) const;
    bool DeleteTokens( const wxString& aProviderId, const wxString& aAccountId );

    static wxString MakeServiceName( const wxString& aProviderId );

private:
    std::unique_ptr<OAUTH_SECRET_BACKEND> m_backend;
};
