/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "git_repo_mixin.h"

#include <trace_helpers.h>

#include <wx/log.h>


void KIGIT_REPO_MIXIN::ReceiveSideband( const char* aData, size_t aLength )
{
    m_pendingSideband.append( aData, aLength );

    size_t lineStart = 0;
    size_t lineEnd;

    // A multibyte character can straddle packets, so only whole lines are decoded
    while( ( lineEnd = m_pendingSideband.find_first_of( "\r\n", lineStart ) ) != std::string::npos )
    {
        wxString line = KIGIT::DecodeServerText( m_pendingSideband.data() + lineStart, lineEnd + 1 - lineStart );
        lineStart = lineEnd + 1;

        // The rest of a line too long to keep, which may hold the password of a URL whose scheme is gone
        if( m_skippingServerLine )
        {
            m_skippingServerLine = false;
            continue;
        }

        addServerLine( line );

        wxString progress = KIGIT::PrintableServerText( line ).Trim();

        if( !progress.IsEmpty() )
            UpdateProgress( 0, 0, progress );
    }

    m_pendingSideband.erase( 0, lineStart );

    // UTF-8 takes at most four bytes a character, so this line is longer than the text kept
    if( m_pendingSideband.length() > 4 * KIGIT::MAX_SERVER_TEXT )
    {
        m_pendingSideband.clear();
        m_skippingServerLine = true;
    }
}


void KIGIT_REPO_MIXIN::addServerLine( const wxString& aLine )
{
    m_serverMessage << aLine;

    // A long transfer streams progress through the same channel, and only the end can explain a refusal. Whole
    // lines go, so the scrub still sees each URL from its scheme
    while( m_serverMessage.length() > KIGIT::MAX_SERVER_TEXT )
    {
        size_t lineEnd = m_serverMessage.find_first_of( wxS( "\r\n" ) );
        m_serverMessage.erase( 0, lineEnd == wxString::npos ? wxString::npos : lineEnd + 1 );
    }
}


void KIGIT_REPO_MIXIN::SetPushRefusal( const wxString& aStatus )
{
    // Scrubbed before the cut, so the cut cannot leave half a URL that still names its password
    m_pushRefusal = KIGIT::PrintableServerText( aStatus ).Right( KIGIT::MAX_SERVER_TEXT );
}


KIGIT_FAILURE_REPORT KIGIT_REPO_MIXIN::ReportTransferFailure( const KIGIT_ERROR_INFO& aError,
                                                              KIGIT_OPERATION aOperation, const wxString& aRemoteName )
{
    // A refusal can end the transfer without a final line break
    if( !m_skippingServerLine && !m_pendingSideband.empty() )
        addServerLine( KIGIT::DecodeServerText( m_pendingSideband.data(), m_pendingSideband.size() ) );

    m_pendingSideband.clear();
    m_skippingServerLine = false;

    KIGIT_FAILURE_CONTEXT context;
    context.operation = aOperation;
    context.remoteName = aRemoteName;
    context.url = m_common->GetRemote();
    context.hostKey = m_hostKeyFailure;
    context.hostKeyRefusal = m_hostKeyRefusal;
    context.pushRefusal = m_pushRefusal;
    context.serverMessage = m_serverMessage;
    context.authFailed = m_authFailed;

    KIGIT_FAILURE_REPORT report = KIGIT::DescribeGitFailure( aError, m_authAttempt, context );

    // The credential prompt that follows asks for this key's passphrase. An older answer may name a revoked key
    m_common->SetAcceptedSSHKey( m_authAttempt.acceptedKey );

    wxLogTrace( traceGit, "Transfer failed with code %d, class %d, kind %d: %s", aError.code, aError.klass,
                static_cast<int>( report.kind ), KIGIT::PrintableServerText( aError.message ) );

    if( KIGIT::IsCredentialFailure( report.kind ) )
        m_authFailed = true;

    m_failureKind = report.kind;
    AddError( report.summary, report.details );
    return report;
}
