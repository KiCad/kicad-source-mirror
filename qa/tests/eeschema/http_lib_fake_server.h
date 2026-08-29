/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <map>
#include <mutex>
#include <string>
#include <vector>

#include <http_lib/http_lib_connection.h>


/**
 * In-process HTTP library server used by QA tests.
 * A server instance must outlive every HTTP_LIB_CONNECTION built from its MakeRetriever()
 */
class HTTP_LIB_FAKE_SERVER
{
public:
    struct RESPONSE
    {
        int         statusCode = 200;
        std::string body;
    };

    /// Replace the response served for an exact URL.
    void SetResponse( const std::string& aUrl, RESPONSE aResponse );

    /// Number of times @p aUrl has been requested.
    int RequestCount( const std::string& aUrl ) const;

    HTTP_LIB_CONNECTION::RETRIEVER MakeRetriever();

    void InstallStandardLibrary( const std::string& aRootUrl );

private:
    mutable std::mutex                m_mutex;
    std::map<std::string, RESPONSE>   m_responses;
    std::vector<std::string>          m_requests;
};
