/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Ian McInerney <ian.s.mcinerney@ieee.org>
 * Copyright (C) 2007-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2020 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <boost/uuid/uuid.hpp>
#include <macros_swig.h>

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

class KIID
{
public:
    KIID();
    KIID( int null );
    KIID( const wxString& aString );
    KIID( timestamp_t aTimestamp );

    void Clone( const KIID& aUUID );

    size_t Hash() const;

    bool        IsLegacyTimestamp() const;
    timestamp_t AsLegacyTimestamp() const;

    wxString AsString() const;
    wxString AsLegacyTimestampString() const;

    static bool SniffTest( const wxString& aCandidate );

    static void CreateNilUuids( bool aNil = true );

    /**
     * Change an existing time stamp based UUID into a true UUID.
     *
     * If this is not a time stamp based UUID, then no change is made.
     */
    void ConvertTimestampToUuid();

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

    timestamp_t m_cached_timestamp;
};


extern KIID niluuid;

KIID& NilUuid();

// declare KIID_VECT_LIST as std::vector<KIID> both for c++ and swig:
DECL_VEC_FOR_SWIG( KIID_VECT_LIST, KIID )

class KIID_PATH : public KIID_VECT_LIST
{
public:
    KIID_PATH()
    {
    }

    KIID_PATH( const wxString& aString );


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
};

#endif // KIID_H
