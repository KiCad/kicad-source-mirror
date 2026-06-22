/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
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

#include "trace_capture.h"

#include <cmath>
#include <cstdio>


DRC_PROFILE_LOG::DRC_PROFILE_LOG() :
        m_prev( wxLog::GetActiveTarget() )
{
}


void DRC_PROFILE_LOG::Clear()
{
    m_providerMs.clear();
    m_totalMs = 0.0;
}


/**
 * Pull the millisecond value out of a "... took <ms> ms" tail. The engine formats with
 * "%0.3f" under the C locale so the separator is always a dot. We anchor on " took " and
 * the trailing " ms" rather than splitting on spaces, since provider names contain spaces.
 */
static bool extractMs( const wxString& aMsg, double& aMs )
{
    int tookPos = aMsg.Find( wxT( " took " ) );

    if( tookPos == wxNOT_FOUND )
        return false;

    wxString tail = aMsg.Mid( tookPos + 6 );

    if( !tail.EndsWith( wxT( " ms" ) ) )
        return false;

    wxString number = tail.Left( tail.length() - 3 );

    return number.ToCDouble( &aMs );
}


bool DRC_PROFILE_LOG::ParseLine( const wxString& aMsg )
{
    // The active wxLog target receives the fully formatted record, which may carry a
    // timestamp and a "Trace: (KICAD_DRC_PROFILE) " prefix ahead of the engine text. Anchor
    // on the known substrings rather than the start of the line so the prefix is tolerated.
    int provPos = aMsg.Find( wxT( "DRC provider '" ) );

    if( provPos != wxNOT_FOUND )
    {
        wxString rest = aMsg.Mid( provPos + 14 );
        int      quote = rest.Find( '\'' );

        if( quote == wxNOT_FOUND )
            return false;

        wxString name = rest.Left( quote );
        double   ms = 0.0;

        if( !extractMs( aMsg, ms ) )
            return false;

        m_providerMs[name] += ms;

        return true;
    }

    int totalPos = aMsg.Find( wxT( "DRC took " ) );

    if( totalPos != wxNOT_FOUND )
    {
        wxString tail = aMsg.Mid( totalPos + 9 );

        if( !tail.EndsWith( wxT( " ms" ) ) )
            return false;

        wxString number = tail.Left( tail.length() - 3 );
        double   ms = 0.0;

        if( !number.ToCDouble( &ms ) )
            return false;

        m_totalMs = ms;

        return true;
    }

    return false;
}


void DRC_PROFILE_LOG::DoLogTextAtLevel( wxLogLevel aLevel, const wxString& aMsg )
{
    ParseLine( aMsg );

    if( m_prev && m_prev != this )
        m_prev->LogTextAtLevel( aLevel, aMsg );
}


int RunTraceCaptureSelftest()
{
    DRC_PROFILE_LOG parser;

    const wxString lines[] = {
        wxT( "DRC provider 'courtyard_overlap' took 12.345 ms" ),
        wxT( "DRC provider 'matched length' took 0.500 ms" ),
        wxT( "DRC took 42.000 ms" )
    };

    for( const wxString& line : lines )
    {
        if( !parser.ParseLine( line ) )
        {
            std::printf( "selftest FAIL: unparsed line: %s\n", static_cast<const char*>( line.utf8_str() ) );
            return 1;
        }
    }

    int rv = 0;

    auto checkProvider = [&]( const wxString& aName, double aExpected )
    {
        auto it = parser.ProviderMs().find( aName );

        if( it == parser.ProviderMs().end() || std::fabs( it->second - aExpected ) > 1e-9 )
        {
            std::printf( "selftest FAIL: provider '%s' expected %.3f got %.3f\n",
                         static_cast<const char*>( aName.utf8_str() ), aExpected,
                         it == parser.ProviderMs().end() ? -1.0 : it->second );
            rv = 1;
        }
    };

    checkProvider( wxT( "courtyard_overlap" ), 12.345 );
    checkProvider( wxT( "matched length" ), 0.500 );

    if( std::fabs( parser.TotalMs() - 42.000 ) > 1e-9 )
    {
        std::printf( "selftest FAIL: total expected 42.000 got %.3f\n", parser.TotalMs() );
        rv = 1;
    }

    if( parser.ProviderMs().size() != 2 )
    {
        std::printf( "selftest FAIL: expected 2 providers got %zu\n", parser.ProviderMs().size() );
        rv = 1;
    }

    if( rv == 0 )
        std::printf( "selftest PASS\n" );

    return rv;
}
