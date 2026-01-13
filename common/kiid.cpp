/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Ian McInerney <ian.s.mcinerney@ieee.org>
 * Copyright (C) 2007-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <kiid.h>

#include <boost/random/mersenne_twister.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#if BOOST_VERSION >= 106700
#include <boost/uuid/entropy_error.hpp>
#endif

#include <json_common.h>

#include <cctype>
#include <mutex>
#include <utility>
#include <stdlib.h>

#include <wx/log.h>

// boost:mt19937 is not thread-safe
static std::mutex                                           rng_mutex;

// Static rng and generators are used because the overhead of constant seeding is expensive
// We rely on the default non-arg constructor of basic_random_generator to provide a random seed.
// We use a separate rng object for cases where we want to control the basic_random_generator
// initial seed by calling SeedGenerator from unit tests and other special cases.
static boost::mt19937                                       rng;
static boost::uuids::basic_random_generator<boost::mt19937> randomGenerator;

// These don't have the same performance penalty, but we might as well be consistent
static boost::uuids::string_generator                       stringGenerator;
static boost::uuids::nil_generator                          nilGenerator;


// Global nil reference
KIID niluuid( 0 );


// When true, always create nil uuids for performance, when valid ones aren't needed.
// Thread-local to prevent background library loading from affecting other threads.
static thread_local bool g_createNilUuids = false;


// For static initialization
KIID& NilUuid()
{
    static KIID nil( 0 );
    return nil;
}


KIID::KIID()
{
#if BOOST_VERSION >= 106700
    try
    {
#endif

        if( g_createNilUuids )
        {
            m_uuid = nilGenerator();
        }
        else
        {
            std::lock_guard<std::mutex> lock( rng_mutex );
            m_uuid = randomGenerator();
        }

#if BOOST_VERSION >= 106700
    }
    catch( const boost::uuids::entropy_error& )
    {
        wxLogFatalError( "A Boost UUID entropy exception was thrown in %s:%s.",
                         __FILE__, __FUNCTION__ );
    }
#endif
}


KIID::KIID( int null ) :
        m_uuid( nilGenerator() )
{
    wxASSERT( null == 0 );
}


KIID::KIID( const std::string& aString ) :
        m_uuid()
{
    if( !aString.empty() && aString.length() <= 8
        && std::all_of( aString.begin(), aString.end(),
                        []( unsigned char c )
                        {
                            return std::isxdigit( c );
                        } ) )
    {
        // A legacy-timestamp-based UUID has only the last 4 octets filled in.
        // Convert them individually to avoid stepping in the little-endian/big-endian
        // doo-doo.
        for( int i = 0; i < 4; i++ )
        {
            int start = static_cast<int>( aString.length() ) - 8 + i * 2;
            int end = start + 2;

            start = std::max( 0, start );
            int len = std::max( 0, end - start );

            std::string octet = aString.substr( start, len );
            m_uuid.data[i + 12] = strtol( octet.data(), nullptr, 16 );
        }
    }
    else
    {
        try
        {
            m_uuid = stringGenerator( aString );
        }
        catch( ... )
        {
            // Failed to parse string representation; best we can do is assign a new
            // random one.
#if BOOST_VERSION >= 106700
            try
            {
#endif

                m_uuid = randomGenerator();

#if BOOST_VERSION >= 106700
            }
            catch( const boost::uuids::entropy_error& )
            {
                wxLogFatalError( "A Boost UUID entropy exception was thrown in %s:%s.",
                                 __FILE__, __FUNCTION__ );
            }
#endif
        }
    }
}


KIID::KIID( const char* aString ) :
        KIID( std::string( aString ) )
{
}


KIID::KIID( const wxString& aString ) :
        KIID( std::string( aString.ToUTF8() ) )
{
}


bool KIID::SniffTest( const wxString& aCandidate )
{
    static wxString niluuidStr = niluuid.AsString();

    if( aCandidate.Length() != niluuidStr.Length() )
        return false;

    for( wxChar c : aCandidate )
    {
        if( c >= '0' && c <= '9' )
            continue;

        if( c >= 'a' && c <= 'f' )
            continue;

        if( c >= 'A' && c <= 'F' )
            continue;

        if( c == '-' )
            continue;

        return false;
    }

    return true;
}


KIID::KIID( timestamp_t aTimestamp )
{
    m_uuid.data[12] = static_cast<uint8_t>( aTimestamp >> 24 );
    m_uuid.data[13] = static_cast<uint8_t>( aTimestamp >> 16 );
    m_uuid.data[14] = static_cast<uint8_t>( aTimestamp >> 8 );
    m_uuid.data[15] = static_cast<uint8_t>( aTimestamp );
}


bool KIID::IsLegacyTimestamp() const
{
    return !m_uuid.data[8] && !m_uuid.data[9] && !m_uuid.data[10] && !m_uuid.data[11];
}


timestamp_t KIID::AsLegacyTimestamp() const
{
    timestamp_t ret = 0;

    ret |= m_uuid.data[12] << 24;
    ret |= m_uuid.data[13] << 16;
    ret |= m_uuid.data[14] << 8;
    ret |= m_uuid.data[15];

    return ret;
}


size_t KIID::Hash() const
{
    return boost::uuids::hash_value( m_uuid );
}


void KIID::Clone( const KIID& aUUID )
{
    m_uuid = aUUID.m_uuid;
}


wxString KIID::AsString() const
{
    return boost::uuids::to_string( m_uuid );
}


std::string KIID::AsStdString() const
{
    return boost::uuids::to_string( m_uuid );
}


wxString KIID::AsLegacyTimestampString() const
{
    return wxString::Format( "%8.8lX", (unsigned long) AsLegacyTimestamp() );
}


void KIID::ConvertTimestampToUuid()
{
    if( !IsLegacyTimestamp() )
        return;

    m_uuid = randomGenerator();
}


void KIID::Increment()
{
    // This obviously destroys uniform distribution, but it can be useful when a
    // deterministic replacement for a duplicate ID is required.

    for( int i = 15; i >= 0; --i )
    {
        m_uuid.data[i]++;

        if( m_uuid.data[i] != 0 )
            break;
    }
}


void KIID::CreateNilUuids( bool aNil )
{
    g_createNilUuids = aNil;
}


void KIID::SeedGenerator( unsigned int aSeed )
{
    rng.seed( aSeed );
    randomGenerator = boost::uuids::basic_random_generator<boost::mt19937>( rng );
}


KIID_PATH::KIID_PATH( const wxString& aString )
{
    for( const wxString& pathStep : wxSplit( aString, '/' ) )
    {
        if( !pathStep.empty() )
            emplace_back( KIID( pathStep ) );
    }
}


bool KIID_PATH::MakeRelativeTo( const KIID_PATH& aPath )
{
    KIID_PATH copy = *this;
    clear();

    if( aPath.size() > copy.size() )
        return false; // this path is not contained within aPath

    for( size_t i = 0; i < aPath.size(); ++i )
    {
        if( copy.at( i ) != aPath.at( i ) )
        {
            *this = copy;
            return false; // this path is not contained within aPath
        }
    }

    for( size_t i = aPath.size(); i < copy.size(); ++i )
        push_back( copy.at( i ) );

    return true;
}


bool KIID_PATH::EndsWith( const KIID_PATH& aPath ) const
{
    if( aPath.size() > size() )
        return false;                      // this path can not end aPath

    KIID_PATH copyThis = *this;
    KIID_PATH copyThat = aPath;

    while( !copyThat.empty() )
    {
        if( *std::prev( copyThis.end() ) != *std::prev( copyThat.end() ) )
            return false;

        copyThis.pop_back();
        copyThat.pop_back();
    }

    return true;
}


wxString KIID_PATH::AsString() const
{
    wxString path;

    for( const KIID& pathStep : *this )
        path += '/' + pathStep.AsString();

    return path;
}


void to_json( nlohmann::json& aJson, const KIID& aKIID )
{
    aJson = aKIID.AsString().ToUTF8();
}


void from_json( const nlohmann::json& aJson, KIID& aKIID )
{
    aKIID = KIID( aJson.get<std::string>() );
}
