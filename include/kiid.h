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

#ifndef KIID_H
#define KIID_H

#include <kicommon.h>
#include <boost/uuid/uuid.hpp>
#include <macros_swig.h>
#include <nlohmann/json_fwd.hpp>

#include <string>

class wxString;

/**
 * timestamp_t is our type to represent unique IDs for all kinds of elements;
 * historically simply the timestamp when they were created.
 *
 * Long term, this type might be renamed to something like unique_id_t
 * (and then rename all the methods from {Get,Set}TimeStamp()
 * to {Get,Set}Id()) ?
 */
typedef uint32_t timestamp_t;

class KICOMMON_API KIID
{
public:
    KIID();
    KIID( int null );
    KIID( const std::string& aString );
    KIID( const char* aString );
    KIID( const wxString& aString );
    KIID( timestamp_t aTimestamp );

    void Clone( const KIID& aUUID );

    size_t Hash() const;

    bool        IsLegacyTimestamp() const;
    timestamp_t AsLegacyTimestamp() const;

    wxString AsString() const;
    wxString AsLegacyTimestampString() const;
    std::string AsStdString() const;

    /**
     * Returns true if a string has the correct formatting to be a KIID.
     */
    static bool SniffTest( const wxString& aCandidate );

    /**
     * A performance optimization which disables/enables the generation of pseudo-random UUIDs.
     *
     * NB: uses a global.  Not thread safe!
     */
    static void CreateNilUuids( bool aNil = true );

    /**
     * Re-initialize the UUID generator with a given seed (for testing or QA purposes)
     *
     * WARNING: Do not call this function from within KiCad or via a Python action plugin.  It is
     * only to be used inside QA tests or in external Python scripts.  Resetting the UUID generator
     * in the middle of a KiCad GUI run will potentially have harmful effects on file integrity.
     *
     * @param aSeed is a seed to pass to the boost::mt19937 pseudo-random number generator
     */
    static void SeedGenerator( unsigned int aSeed );

    /**
     * Change an existing time stamp based UUID into a true UUID.
     *
     * If this is not a time stamp based UUID, then no change is made.
     */
    void ConvertTimestampToUuid();

    /**
     * Generates a deterministic replacement for a given ID.
     *
     * NB: destroys uniform distribution!  But it's the only thing we have when a deterministic
     * replacement for a duplicate ID is required.
     */
    void Increment();

    bool operator==( KIID const& rhs ) const
    {
        return m_uuid == rhs.m_uuid;
    }

    bool operator!=( KIID const& rhs ) const
    {
        return m_uuid != rhs.m_uuid;
    }

    bool operator<( KIID const& rhs ) const
    {
        return m_uuid < rhs.m_uuid;
    }

    bool operator>( KIID const& rhs ) const
    {
        return m_uuid > rhs.m_uuid;
    }

private:
    boost::uuids::uuid m_uuid;
};


extern KICOMMON_API KIID niluuid;

KICOMMON_API KIID& NilUuid();

// declare KIID_VECT_LIST as std::vector<KIID> both for c++ and swig:
DECL_VEC_FOR_SWIG( KIID_VECT_LIST, KIID )

class KICOMMON_API KIID_PATH : public KIID_VECT_LIST
{
public:
    KIID_PATH()
    {
    }

    KIID_PATH( const wxString& aString );

    bool MakeRelativeTo( const KIID_PATH& aPath );

    /**
     * Test if \a aPath from the last path towards the first path.
     *
     * This is useful for testing for existing schematic symbol and sheet instances when
     * copying or adding a new sheet that is lower in the hierarchy than the current path.
     *
     * @param aPath is the path to compare this path against.
     * @return true if this path ends with \a aPath or false if it does not.
     */
    bool EndsWith( const KIID_PATH& aPath ) const;

    wxString AsString() const;

    bool operator==( KIID_PATH const& rhs ) const
    {
        if( size() != rhs.size() )
            return false;

        for( size_t i = 0; i < size(); ++i )
        {
            if( at( i ) != rhs.at( i ) )
                return false;
        }

        return true;
    }

    bool operator<( KIID_PATH const& rhs ) const
    {
        if( size() != rhs.size() )
            return size() < rhs.size();

        for( size_t i = 0; i < size(); ++i )
        {
            if( at( i ) < rhs.at( i ) )
                return true;

            if( at( i ) != rhs.at( i ) )
                return false;
        }

        return false;
    }

    bool operator>( KIID_PATH const& rhs ) const
    {
        if( size() != rhs.size() )
            return size() > rhs.size();

        for( size_t i = 0; i < size(); ++i )
        {
            if( at( i ) > rhs.at( i ) )
                return true;

            if( at( i ) != rhs.at( i ) )
                return false;
        }

        return false;
    }

    KIID_PATH& operator+=( const KIID_PATH& aRhs )
    {
        for( const KIID& kiid : aRhs )
            emplace_back( kiid );

        return *this;
    }

    friend KIID_PATH operator+( KIID_PATH aLhs, const KIID_PATH& aRhs )
    {
        aLhs += aRhs;
        return aLhs;
    }
};

/**
 * RAII class to safely set/reset nil KIIDs for use in footprint/symbol loading
 */
class KICOMMON_API KIID_NIL_SET_RESET
{
public:
    KIID_NIL_SET_RESET()
    {
        KIID::CreateNilUuids( true );
    };

    ~KIID_NIL_SET_RESET()
    {
        KIID::CreateNilUuids( false );
    }
};

KICOMMON_API void to_json( nlohmann::json& aJson, const KIID& aKIID );

KICOMMON_API void from_json( const nlohmann::json& aJson, KIID& aKIID );

template<> struct KICOMMON_API std::hash<KIID>
{
    std::size_t operator()( const KIID& aId ) const
    {
        return aId.Hash();
    }
};

#endif // KIID_H
