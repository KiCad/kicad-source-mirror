/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * KiCad is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KiCad is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KiCad.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <regex>
#include <algorithm>
#include <iostream>

#include <sch_reference_list.h>

#include "refdes_tracker.h"

REFDES_TRACKER::REFDES_TRACKER( bool aThreadSafe ) :
    m_threadSafe( aThreadSafe ), m_reuseRefDes( true )
{
}

bool REFDES_TRACKER::Insert( const std::string& aRefDes )
{
    std::unique_lock<std::mutex> lock;

    if( m_threadSafe )
        lock = std::unique_lock<std::mutex>( m_mutex );

    return insertImpl( aRefDes );
}

bool REFDES_TRACKER::insertImpl( const std::string& aRefDes )
{
    if( m_allRefDes.find( aRefDes ) != m_allRefDes.end() )
        return false;

    auto [prefix, number] = parseRefDes( aRefDes );

    m_allRefDes.insert( aRefDes );

    // Insert the number and update caches
    return insertNumber( prefix, number );
}

bool REFDES_TRACKER::insertNumber( const std::string& aPrefix, int aNumber )
{
    PREFIX_DATA& data = m_prefixData[aPrefix];

    if( data.m_usedNumbers.find( aNumber ) != data.m_usedNumbers.end() )
        return false;

    data.m_usedNumbers.insert( aNumber );

    if( aNumber > 0 )
        updateCacheOnInsert( data, aNumber );

    return true;
}


bool REFDES_TRACKER::containsImpl( const std::string& aRefDes ) const
{
    return m_allRefDes.contains( aRefDes );
}

bool REFDES_TRACKER::Contains( const std::string& aRefDes ) const
{
    std::unique_lock<std::mutex> lock;

    if( m_threadSafe )
        lock = std::unique_lock<std::mutex>( m_mutex );

    return containsImpl( aRefDes );
}


int REFDES_TRACKER::GetNextRefDesForUnits( const SCH_REFERENCE& aRef,
                                           const std::map<int, std::vector<SCH_REFERENCE>>& aRefNumberMap,
                                           const std::vector<int>& aRequiredUnits,
                                           int aMinValue )
{
    std::unique_lock<std::mutex> lock;

    if( m_threadSafe )
        lock = std::unique_lock<std::mutex>( m_mutex );

    // Filter out negative unit numbers
    std::vector<int> validUnits;
    std::copy_if( aRequiredUnits.begin(), aRequiredUnits.end(),
                  std::back_inserter( validUnits ),
                  []( int unit ) { return unit >= 0; } );

    int candidate = aMinValue;

    while( true )
    {
        // Check if this candidate number is currently in use
        auto mapIt = aRefNumberMap.find( candidate );

        if( mapIt == aRefNumberMap.end() )
        {
            // Not currently in use - check if it was previously used
            std::string candidateRefDes = aRef.GetRef().ToStdString() + std::to_string( candidate );

            if( m_reuseRefDes || !containsImpl( candidateRefDes ) )
            {
                // Completely unused - this is our answer
                insertNumber( aRef.GetRefStr(), candidate );
                m_allRefDes.insert( candidateRefDes );
                return candidate;
            }
            else
            {
                // Previously used but no longer active - skip to next candidate
                candidate++;
                continue;
            }
        }
        else
        {
            // Currently in use - check if required units are available
            if( validUnits.empty() )
            {
                // Need completely unused reference, but this one is in use
                candidate++;
                continue;
            }

            // Check if required units are available
            bool unitsAvailable;
            if( m_externalUnitsChecker )
            {
                // Use external units checker if available
                unitsAvailable = m_externalUnitsChecker( aRef, mapIt->second, validUnits );
            }
            else
            {
                // Use default implementation
                unitsAvailable = areUnitsAvailable( aRef, mapIt->second, validUnits );
            }

            if( unitsAvailable )
            {
                // All required units are available - this is our answer
                // Note: Don't insert into tracker since reference is already in use
                return candidate;
            }
            else
            {
                // Some required units are not available - try next candidate
                candidate++;
                continue;
            }
        }
    }
}


bool REFDES_TRACKER::areUnitsAvailable( const SCH_REFERENCE& aRef,
                                        const std::vector<SCH_REFERENCE>& aRefVector,
                                        const std::vector<int>& aRequiredUnits ) const
{
    for( const int& unit : aRequiredUnits )
    {
        for( const SCH_REFERENCE& ref : aRefVector )
        {
            // If we have a different library or different value,
            // we cannot share a reference designator.  Also, if the unit matches,
            // the reference designator + unit is already in use.
            if( ref.CompareLibName( aRef ) != 0
                || ref.CompareValue( aRef ) != 0
                || ref.GetUnit() == unit )
            {
                return false;  // Conflict found
            }
        }
    }

    return true;  // All required units are available
}


std::pair<std::string, int> REFDES_TRACKER::parseRefDes( const std::string& aRefDes ) const
{
    if( aRefDes.empty() )
        return { "", 0 };

    // Find the last sequence of digits at the end
    std::regex pattern( R"(^([A-Za-z]+)(\d+)?$)" );
    std::smatch match;

    if( std::regex_match( aRefDes, match, pattern ) )
    {
        std::string prefix = match[1].str();
        if( match[2].matched )
        {
            int number = std::stoi( match[2].str() );
            return { prefix, number };
        }
        else
        {
            return { prefix, 0 };  // No number suffix
        }
    }

    // If it doesn't match our expected pattern, treat the whole thing as prefix
    return { aRefDes, 0 };
}

void REFDES_TRACKER::updateBaseNext( PREFIX_DATA& aData ) const
{
    if( aData.m_cacheValid )
        return;

    // Find the first gap in the sequence starting from 1
    int candidate = 1;
    for( int used : aData.m_usedNumbers )
    {
        if( used <= 0 )
            continue;  // Skip non-positive numbers (like our 0 marker)
        if( used == candidate )
        {
            candidate++;
        }
        else if( used > candidate )
        {
            break;  // Found a gap
        }
    }

    aData.m_baseNext = candidate;
    aData.m_cacheValid = true;
}

void REFDES_TRACKER::updateCacheOnInsert( PREFIX_DATA& aData, int aInsertedNumber ) const
{
    // Update base next cache if it's valid and affected
    if( aData.m_cacheValid )
    {
        if( aInsertedNumber == aData.m_baseNext )
        {
            // The base next was just used, find the new next
            int candidate = aData.m_baseNext + 1;
            while( aData.m_usedNumbers.find( candidate ) != aData.m_usedNumbers.end() )
            {
                candidate++;
            }
            aData.m_baseNext = candidate;
        }
        // If aInsertedNumber > m_baseNext, base cache is still valid
        // If aInsertedNumber < m_baseNext, base cache is still valid
    }

    for( auto cacheIt = aData.m_nextCache.begin(); cacheIt != aData.m_nextCache.end(); ++cacheIt )
    {
        int cachedNext = cacheIt->second;

        if( aInsertedNumber == cachedNext )
        {
            // This cached value was just used, need to update it
            int candidate = cachedNext + 1;

            while( aData.m_usedNumbers.contains( candidate ) )
                candidate++;

            cacheIt->second = candidate;
        }
    }
}

int REFDES_TRACKER::findNextAvailable( const PREFIX_DATA& aData, int aMinValue ) const
{
    if( auto cacheIt = aData.m_nextCache.find( aMinValue ); cacheIt != aData.m_nextCache.end() )
        return cacheIt->second;

    updateBaseNext( const_cast<PREFIX_DATA&>( aData ) );

    int candidate;

    if( aMinValue <= 1 )
    {
        candidate = aData.m_baseNext;
    }
    else
    {
        // Start search from aMinValue
        candidate = aMinValue;

        while( aData.m_usedNumbers.find( candidate ) != aData.m_usedNumbers.end() )
            candidate++;
    }

    // Cache the result
    aData.m_nextCache[aMinValue] = candidate;

    return candidate;
}

std::string REFDES_TRACKER::Serialize() const
{
    std::unique_lock<std::mutex> lock;
    if( m_threadSafe )
        lock = std::unique_lock<std::mutex>( m_mutex );

    std::ostringstream result;
    bool first = true;

    for( const auto& [prefix, data] : m_prefixData )
    {
        if( !first )
            result << ",";
        first = false;

        std::string escapedPrefix = escapeForSerialization( prefix );

        // Separate numbers from prefix-only entries
        std::vector<int> numbers;
        bool hasPrefix = false;

        for( int num : data.m_usedNumbers )
        {
            if( num > 0 )
                numbers.push_back( num );
            else if( num == 0 )
                hasPrefix = true;
        }

        if( numbers.empty() && !hasPrefix )
            continue;  // No data for this prefix

        // Create ranges for numbered entries
        std::vector<std::pair<int, int>> ranges;

        if( !numbers.empty() )
        {
            int start = numbers[0];
            int end = numbers[0];

            for( size_t i = 1; i < numbers.size(); ++i )
            {
                if( numbers[i] == end + 1 )
                {
                    end = numbers[i];
                }
                else
                {
                    ranges.push_back( { start, end } );
                    start = end = numbers[i];
                }
            }
            ranges.push_back( { start, end } );
        }

        bool firstRange = true;
        for( const auto& [start, end] : ranges )
        {
            if( !firstRange )
                result << ",";
            firstRange = false;

            result << escapedPrefix;
            if( start == end )
            {
                result << start;
            }
            else
            {
                result << start << "-" << end;
            }
        }

        // Add prefix-only entry if it exists
        if( hasPrefix )
        {
            if( !firstRange )
                result << ",";
            result << escapedPrefix;
        }
    }

    return result.str();
}

bool REFDES_TRACKER::Deserialize( const std::string& aData )
{
    std::unique_lock<std::mutex> lock;

    if( m_threadSafe )
        lock = std::unique_lock<std::mutex>( m_mutex );

    clearImpl();

    if( aData.empty() )
        return true;

    auto parts = splitString( aData, ',' );

    for( const std::string& part : parts )
    {
        std::string unescaped = unescapeFromSerialization( part );

        // Parse each part
        std::regex rangePattern( R"(^([A-Za-z]+)(\d+)(?:-(\d+))?$)" );
        std::regex prefixOnlyPattern( R"(^([A-Za-z]+)$)" );
        std::smatch match;

        if( std::regex_match( unescaped, match, rangePattern ) )
        {
            std::string prefix = match[1].str();
            int start = std::stoi( match[2].str() );
            int end = match[3].matched ? std::stoi( match[3].str() ) : start;

            for( int i = start; i <= end; ++i )
            {
                if( !insertImpl( prefix + std::to_string( i ) ) )
                {
                    // Note: insertImpl might fail if number already exists for prefix
                    // but that's okay during deserialization of valid data
                }
            }
        }
        else if( std::regex_match( unescaped, match, prefixOnlyPattern ) )
        {
            std::string prefix = match[1].str();
            if( !insertImpl( prefix ) )
            {
                // Note: insertImpl might fail if prefix already exists
                // but that's okay during deserialization of valid data
            }
        }
        else
        {
            // Invalid format
            clearImpl();
            return false;
        }
    }

    return true;
}

void REFDES_TRACKER::Clear()
{
    std::unique_lock<std::mutex> lock;

    if( m_threadSafe )
        lock = std::unique_lock<std::mutex>( m_mutex );

    clearImpl();
}


void REFDES_TRACKER::clearImpl()
{
    m_prefixData.clear();
    m_allRefDes.clear();
}

size_t REFDES_TRACKER::Size() const
{
    std::unique_lock<std::mutex> lock;

    if( m_threadSafe )
        lock = std::unique_lock<std::mutex>( m_mutex );

    return m_allRefDes.size();
}

std::string REFDES_TRACKER::escapeForSerialization( const std::string& aStr ) const
{
    std::string result;
    result.reserve( aStr.length() * 2 ); // Reserve space to avoid frequent reallocations

    for( char c : aStr )
    {
        if( c == '\\' || c == ',' || c == '-' )
            result += '\\';
        result += c;
    }
    return result;
}

std::string REFDES_TRACKER::unescapeFromSerialization( const std::string& aStr ) const
{
    std::string result;
    result.reserve( aStr.length() );

    bool escaped = false;
    for( char c : aStr )
    {
        if( escaped )
        {
            result += c;
            escaped = false;
        }
        else if( c == '\\' )
        {
            escaped = true;
        }
        else
        {
            result += c;
        }
    }
    return result;
}

std::vector<std::string> REFDES_TRACKER::splitString( const std::string& aStr, char aDelimiter ) const
{
    std::vector<std::string> result;
    std::string current;
    bool escaped = false;

    for( char c : aStr )
    {
        if( escaped )
        {
            current += c;
            escaped = false;
        }
        else if( c == '\\' )
        {
            escaped = true;
            current += c;
        }
        else if( c == aDelimiter )
        {
            result.push_back( current );
            current.clear();
        }
        else
        {
            current += c;
        }
    }

    if( !current.empty() )
        result.push_back( current );

    return result;
}


void REFDES_TRACKER::SetUnitsChecker( const UNITS_CHECKER_FUNC<SCH_REFERENCE>& aChecker )
{
    std::unique_lock<std::mutex> lock;

    if( m_threadSafe )
        lock = std::unique_lock<std::mutex>( m_mutex );

    m_externalUnitsChecker = aChecker;
}


void REFDES_TRACKER::ClearUnitsChecker()
{
    std::unique_lock<std::mutex> lock;

    if( m_threadSafe )
        lock = std::unique_lock<std::mutex>( m_mutex );

    m_externalUnitsChecker = nullptr;
}