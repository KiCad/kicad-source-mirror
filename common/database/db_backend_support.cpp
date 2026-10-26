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

#include "db_backend_support.h"

#include <cctype>
#include <cstddef>
#include <utility>

#include <wx/intl.h>
#include <wx/string.h>


DB_WORKER_FAILURE DescribeWorkerOutcome( DB_WORKER_THREAD::OUTCOME aOutcome, const std::string& aPath )
{
    const wxString path = wxString::FromUTF8( aPath );

    switch( aOutcome )
    {
    case DB_WORKER_THREAD::OUTCOME::REFUSED:
        return { DB_FAILURE::TIMEOUT,
                 utf8( wxString::Format( _( "still waiting on an earlier request for %s" ), path ) ),
                 DB_WORKER_FAILURE::CONNECTION::KEEP };

    case DB_WORKER_THREAD::OUTCOME::TOO_LATE:
        return { DB_FAILURE::TIMEOUT, utf8( wxString::Format( _( "%s is busy with an earlier request" ), path ) ),
                 DB_WORKER_FAILURE::CONNECTION::KEEP };

    case DB_WORKER_THREAD::OUTCOME::UNAVAILABLE:
        return { DB_FAILURE::UNKNOWN, utf8( wxString::Format( _( "could not start a thread to read %s" ), path ) ),
                 DB_WORKER_FAILURE::CONNECTION::DROP };

    default:
        return { DB_FAILURE::TIMEOUT, utf8( wxString::Format( _( "timed out waiting for %s" ), path ) ),
                 DB_WORKER_FAILURE::CONNECTION::FORGET };
    }
}


DB_READ_DELIVERY::DB_READ_DELIVERY( std::vector<std::string>& aLabels, const DB_ROW_SINK& aSink ) :
        m_labels( aLabels ),
        m_sink( aSink ),
        m_jobLabels( std::make_shared<std::vector<std::string>>() )
{
    m_deliver = [this]( std::vector<DB_VALUE>&& aRow )
    {
        if( !m_labelsCopied )
        {
            m_labels = *m_jobLabels;
            m_labelsCopied = true;
        }

        try
        {
            m_sink( std::move( aRow ) );
        }
        catch( ... )
        {
            m_sinkThrew = true;
            throw;
        }
    };
}


void DB_READ_DELIVERY::Finish()
{
    if( !m_labelsCopied )
        m_labels = *m_jobLabels;
}


std::string DB_READ_DELIVERY::SinkFailureMessage()
{
    return utf8( _( "the rows read from the database were not accepted" ) );
}


std::string ScrubSecret( std::string aText, const std::string& aSecret )
{
    static constexpr size_t EVERYWHERE = 4;

    if( aSecret.empty() )
        return aText;

    auto inWord = []( unsigned char aChar )
    {
        return std::isalnum( aChar ) || aChar >= 0x80;
    };

    auto scrub = [&]( const std::string& aForm )
    {
        for( size_t at = aText.find( aForm ); at != std::string::npos; )
        {
            const size_t end = at + aForm.size();
            const bool   alone =
                    ( at == 0 || !inWord( aText[at - 1] ) ) && ( end >= aText.size() || !inWord( aText[end] ) );

            if( aForm.size() >= EVERYWHERE || alone )
            {
                aText.replace( at, aForm.size(), "***" );
                at = aText.find( aForm, at + 3 );
            }
            else
            {
                at = aText.find( aForm, at + 1 );
            }
        }
    };

    // The doubled form first, since it contains the raw one only where a quote is absent
    if( aSecret.find( '"' ) != std::string::npos )
    {
        std::string doubled;

        for( char c : aSecret )
        {
            doubled += c;

            if( c == '"' )
                doubled += c;
        }

        scrub( doubled );
    }

    scrub( aSecret );
    return aText;
}


std::string RedactDetail( const std::string& aDetail, const std::string& aSecret )
{
    static constexpr size_t SHORT_SECRET = 4;

    if( !aSecret.empty() && aSecret.size() < SHORT_SECRET )
        return std::string();

    return ScrubSecret( aDetail, aSecret );
}
