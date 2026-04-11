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

#include <wx/event.h>
#include <wx/socket.h>
#include <wx/string.h>
#include <wx/timer.h>

#include <memory>
#include <optional>

#include <kicommon.h>


wxDECLARE_EXPORTED_EVENT( KICOMMON_API, EVT_OAUTH_LOOPBACK_RESULT, wxCommandEvent );


struct KICOMMON_API OAUTH_AUTHORIZATION_RESPONSE
{
    wxString code;
    wxString state;
    wxString error;
    wxString error_description;
};


class KICOMMON_API OAUTH_LOOPBACK_SERVER : public wxEvtHandler
{
public:
    OAUTH_LOOPBACK_SERVER( wxEvtHandler* aOwner, const wxString& aCallbackPath,
                           const wxString& aExpectedState );
    ~OAUTH_LOOPBACK_SERVER() override;

    bool Start();
    unsigned short GetPort() const { return m_port; }
    wxString GetRedirectUri() const;

    static bool ParseAuthorizationResponse( const wxString& aRequestLine,
                                            const wxString& aExpectedPath,
                                            const wxString& aExpectedState,
                                            OAUTH_AUTHORIZATION_RESPONSE& aResponse,
                                            wxString& aError );

private:
    void OnSocketEvent( wxSocketEvent& aEvent );
    void OnTimeout( wxTimerEvent& aEvent );
    void HandleClient( wxSocketBase* aClient );
    void SendHttpResponse( wxSocketBase* aClient, bool aSuccess );
    void Finish( bool aSuccess );
    void Shutdown();

    wxEvtHandler*                             m_owner;
    wxString                                  m_callbackPath;
    wxString                                  m_expectedState;
    std::unique_ptr<wxSocketServer>           m_server;
    wxTimer                                   m_timeout;
    unsigned short                            m_port;
    bool                                      m_done;
    std::optional<OAUTH_AUTHORIZATION_RESPONSE> m_response;
};
