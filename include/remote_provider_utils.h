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

#ifndef REMOTE_PROVIDER_UTILS_H
#define REMOTE_PROVIDER_UTILS_H

#include <vector>

#include <json_schema_validator.h>
#include <kicommon.h>
#include <wx/string.h>


/**
 * Percent-encode a string for use in URL query parameters (RFC 3986
 * unreserved characters are passed through unchanged).
 */
KICOMMON_API wxString UrlEncode( const wxString& aValue );

/**
 * Extract an optional string value from a JSON object, returning an empty
 * wxString when the key is absent or the value is not a string.
 */
KICOMMON_API wxString RemoteProviderJsonString( const nlohmann::json& aObject, const char* aKey );

/**
 * Return true when \a aHost resolves to a loopback address (localhost,
 * 127.0.0.1, or ::1).  IPv6 bracket notation is handled.
 */
KICOMMON_API bool IsLoopbackHost( const wxString& aHost );

/**
 * Validate that \a aUrl uses HTTPS, or HTTP on a loopback address when
 * \a aAllowInsecureLocalhost is true.  On failure, \a aError is populated
 * with a diagnostic that includes \a aLabel.
 */
KICOMMON_API bool ValidateRemoteUrlSecurity( const wxString& aUrl, bool aAllowInsecureLocalhost,
                                             wxString& aError, const wxString& aLabel );

/**
 * Return a normalized scheme://host:port origin string for \a aUrl.
 * Empty when the URL cannot be parsed.
 */
KICOMMON_API wxString NormalizedUrlOrigin( const wxString& aUrl );


/**
 * Collects JSON-schema validation errors so the caller can inspect them
 * after a validation pass.
 */
class KICOMMON_API COLLECTING_JSON_ERROR_HANDLER : public nlohmann::json_schema::error_handler
{
public:
    void error( const nlohmann::json::json_pointer& aPointer, const nlohmann::json& aInstance,
                const std::string& aMessage ) override;

    bool     HasErrors() const { return !m_errors.empty(); }
    wxString FirstError() const;

private:
    std::vector<wxString> m_errors;
};

#endif // REMOTE_PROVIDER_UTILS_H
