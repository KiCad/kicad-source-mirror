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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KICAD_API_CLIENT_H
#define KICAD_API_CLIENT_H

#include <string>
#include <nng/nng.h>
#include <nng/protocol/reqrep0/req.h>
#include <wx/string.h>

#include <google/protobuf/message.h>

#include <kicommon.h>
#include <api/common/envelope.pb.h>


class KICOMMON_API KICAD_API_CLIENT
{
public:
    explicit KICAD_API_CLIENT( int aTimeoutMs = 1000 );
    ~KICAD_API_CLIENT();

    bool Connect( const wxString& aSocketUrl );
    void Disconnect();

    bool IsConnected() const { return m_isOpen && m_isConnected; }

    /**
     * Send a protobuf request to the KiCad API server and wait for a response.
     *
     * @param aRequest is the command-specific protobuf message.
     * @param aResponse receives the ApiResponse envelope.
     * @param aClientName is an optional client identifier for the request header.
     * @return true if the request was sent and a valid ApiResponse was received.
     */
    bool Send( const google::protobuf::Message& aRequest, kiapi::common::ApiResponse& aResponse,
               const std::string& aClientName = "kicad" );

    const wxString& GetLastError() const { return m_lastError; }

private:
    nng_socket m_socket;
    bool       m_isOpen;
    bool       m_isConnected;
    wxString   m_lastError;
};


#endif // KICAD_API_CLIENT_H
