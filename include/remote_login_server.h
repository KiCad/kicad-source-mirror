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
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once
#include <wx/event.h>
#include <wx/socket.h>
#include <wx/string.h>
#include <wx/timer.h>

#include <memory>

wxDECLARE_EVENT( EVT_REMOTE_SYMBOL_LOGIN_RESULT, wxCommandEvent );

class REMOTE_LOGIN_SERVER : public wxEvtHandler
{
public:
    REMOTE_LOGIN_SERVER( wxEvtHandler* aOwner, const wxString& aRedirectUrl );
    ~REMOTE_LOGIN_SERVER() override;

    bool Start();
    unsigned short GetPort() const { return m_port; }

private:
    void OnSocketEvent( wxSocketEvent& aEvent );
    void OnTimeout( wxTimerEvent& aEvent );
    void HandleClient( wxSocketBase* aClient );
    wxString ExtractUserId( const wxString& aRequestLine ) const;
    void SendHttpResponse( wxSocketBase* aClient );
    void Finish( bool aSuccess, const wxString& aUserId );
    void Shutdown();

    wxEvtHandler*                   m_owner;
    wxString                        m_redirectUrl;
    std::unique_ptr<wxSocketServer> m_server;
    wxTimer                         m_timeout;
    unsigned short                  m_port;
    bool                            m_done;
};