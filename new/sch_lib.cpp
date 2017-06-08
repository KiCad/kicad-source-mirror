/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2010 KiCad Developers, see change_log.txt for contributors.
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

#include <memory>           // std::auto_ptr
#include <wx/string.h>


#include <sch_lib.h>
#include <sch_lpid.h>
#include <sch_part.h>
#include <sch_sweet_parser.h>
#include <sch_lib_table.h>


#if 1

#include <map>

/*

The LIB part cache consists of a std::map of partnames without revisions at the
top level. Each top level map entry can point to another std::map which it owns
and which holds all the revisions for that part name. At any point in the tree,
there can be NULL pointers which allow for lazy loading, including the very top
most root pointer itself, which is PARTS* parts. We use the key to hold the
partName at one level, and revision at the deeper nested level.

1) Only things which are asked for are done.
2) Anything we learn we remember.

*/

namespace SCH {


/**
 * Struct LTREV
 * is for PART_REVS, and provides a custom way to compare rev STRINGs.
 * Namely, the revN[N..] string if present, is collated according to a
 * 'higher revision first'.
 */
struct LTREV
{
    bool operator() ( const STRING& s1, const STRING& s2 ) const
    {
        return RevCmp( s1.c_str(), s2.c_str() ) < 0;
    }
};


/**
 * Class PART_REVS
 * contains the collection of revisions for a particular part name, in the
 * form of cached PARTs.  The tuple consists of a rev string and a PART pointer.
 * The rev string is like "rev1", the PART pointer will be NULL until the PART
 * gets loaded, lazily.
 */
class PART_REVS : public std::map< STRING, PART*, LTREV >
{
public:
    ~PART_REVS()
    {
        for( iterator it = begin();  it != end();  ++it )
        {
            delete it->second;   // second may be NULL, no problem
        }
    }
};


/**
 * Class PARTS
 * contains the collection of PART_REVS for all PARTs in the lib.
 * The tuple consists of a part name and a PART_REVS pointer.
 * The part name does not have the revision attached (of course this is understood
 * by definition of "part name"). The PART_REVS pointer will be NULL until a client
 * askes about the revisions for a part name, so the loading is done lazily.
 */
class PARTS : public std::map< STRING, PART_REVS* >
{
public:
    ~PARTS()
    {
        for( iterator it = begin();  it != end();  ++it )
        {
            delete it->second;  // second may be NULL, no problem
        }
    }
};

}  // namespace SCH


#else   // was nothing but grief:

#include <boost/ptr_container/ptr_map.hpp>

namespace SCH {

/// PARTS' key is revision, like "rev12", PART pointer may be null until loaded.
typedef boost::ptr_map< STRING, boost::nullable<PART> >     PARTS;
typedef PARTS::iterator                 PARTS_ITER;
typedef PARTS::const_iterator           PARTS_CITER;


/// PART_REVS' key is part name, w/o rev, PART pointer may be null until loaded.
typedef boost::ptr_map< STRING, boost::nullable<PARTS> >    PART_REVS;
typedef PART_REVS::iterator             PART_REVS_ITER;
typedef PART_REVS::const_iterator       PART_REVS_CITER;

}  // namespace SCH

#endif


using namespace SCH;


LIB::LIB( const STRING& aLogicalLibrary, LIB_SOURCE* aSource, LIB_SINK* aSink ) :
    logicalName( aLogicalLibrary ),
    source( aSource ),
    sink( aSink ),
    cachedCategories( false ),
    parts( 0 )
{
}


LIB::~LIB()
{
    delete source;
    delete sink;
    delete parts;
}


const PART* LIB::lookupPart( const LPID& aLPID )
{
    if( !parts )
    {
        parts = new PARTS;

        source->GetCategoricalPartNames( &vfetch );

        // insert a PART_REVS for each part name
        for( STRINGS::const_iterator it = vfetch.begin();  it!=vfetch.end();  ++it )
        {
            D(printf("lookupPart:%s\n", it->c_str() );)
            (*parts)[*it] = new PART_REVS;
        }
    }

    // load all the revisions for this part name, only if it has any
    PARTS::iterator  pi = parts->find( aLPID.GetPartName() );

    PART_REVS*  revs = pi != parts->end() ? pi->second : NULL;

    // D(printf("revs:%p partName:%s\n", revs, aLPID.GetPartName().c_str() );)

    // if the key for parts has no aLPID.GetPartName() the part is not in this lib
    if( revs )
    {
        if( revs->size() == 0 ) // assume rev list has not been loaded yet
        {
            // load all the revisions for this part.
            source->GetRevisions( &vfetch, aLPID.GetPartName() );

            // create a PART_REV entry for each revision, but leave the PART* NULL
            for( STRINGS::const_iterator it = vfetch.begin();  it!=vfetch.end();  ++it )
            {
                D(printf("lookupPartRev:%s\n", it->c_str() );)
                (*revs)[*it] = 0;
            }
        }

        PART_REVS::iterator rev;

        // If caller did not say what revision, find the highest numbered one and return that.
        if( !aLPID.GetRevision().size() && revs->size() )
        {
            rev = revs->begin();     // sort order has highest rev first

            if( !rev->second )       // the PART has never been instantiated before
            {
                rev->second = new PART( this, LPID::Format( "", aLPID.GetPartName(), rev->first ) );
            }

            D(printf("lookupPartLatestRev:%s\n", rev->second->partNameAndRev.c_str() );)
            return rev->second;
        }
        else
        {
            rev = revs->find( aLPID.GetRevision() );

            if( rev != revs->end() )
            {
                if( !rev->second )    // the PART has never been instantiated before
                {
                    rev->second = new PART( this, aLPID.GetPartNameAndRev() );
                }
                return rev->second;
            }
        }
    }

    return 0;   // no such part name in this lib
}


PART* LIB::LookupPart( const LPID& aLPID, LIB_TABLE* aLibTable )
{
    PART*   part = (PART*) lookupPart( aLPID );

    if( !part )     // part does not exist in this lib
    {
        wxString msg = wxString::Format( _("part '%s' not found in lib %s" ),
                            wxString::FromUTF8( aLPID.GetPartNameAndRev().c_str() ).GetData(),
                            wxString::FromUTF8( logicalName.c_str() ).GetData() );
        THROW_IO_ERROR( msg );
    }

    if( part->body.empty() )
    {
        // load body
        source->ReadPart( &part->body, aLPID.GetPartName(), aLPID.GetRevision() );

#if 0 && defined(DEBUG)
        const STRING& body = part->body;
        printf( "body: %s", body.c_str() );
        if( !body.size() || body[body.size()-1] != '\n' )
            printf( "\n" );
#endif

        // @todo consider changing ReadPart to return a "source"
        SWEET_PARSER sp( part->body, wxString::FromUTF8( aLPID.Format().c_str() ) );

        part->Parse( &sp, aLibTable );
    }

    return part;
}


#if 0 && defined(DEBUG)

void LIB::Test( int argc, char** argv );
{
}

int main( int argc, char** argv )
{
    LIB::Test( argc, argv );

    return 0;
}

#endif
