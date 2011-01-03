/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2010 Kicad Developers, see change_log.txt for contributors.
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
#include <sweet_lexer.h>
#include <sch_lib_table.h>


#if 1

#include <map>

/*

The LIB part cache consist of a std::map of partnames without revisions at the top level.
Each top level map entry can point to another std::map which it owns and holds all the revisions
for that part name.  At any point in the tree, there can be NULL pointers which
allow for lazy loading, including the very top most root pointer itself, which
is PARTS* parts.  We use the key to hold the partName at one level, and revision
at the deeper nested level, and that key information may not be present within
right hand side of the map tuple.

1) Only things which are asked for are done.
2) Anything we learn we remember.

*/

namespace SCH {

class PART_REVS : public std::map< STRING, PART* >
{
    // @todo provide an integer sort on revN.. strings here.

public:
    ~PART_REVS()
    {
        for( iterator it = begin();  it != end();  ++it )
        {
            delete it->second;   // second may be NULL, no problem
        }
    }
};

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


const PART* LIB::findPart( const LPID& aLPID ) throw( IO_ERROR )
{
    if( !parts )
    {
        parts = new PARTS;

        source->GetCategoricalPartNames( &vfetch );

        // insert a PART_REVS for each part name
        for( STRINGS::const_iterator it = vfetch.begin();  it!=vfetch.end();  ++it )
        {
            // D(printf("findPart:%s\n", it->c_str() );)
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
        if( revs->size() == 0 )
        {
            // load all the revisions for this part.
            source->GetRevisions( &vfetch, aLPID.GetPartName() );

            // creat a PART_REV entry for revision, but leave the PART* NULL
            for( STRINGS::const_iterator it = vfetch.begin();  it!=vfetch.end();  ++it )
            {
                // D(printf("findPartRev:%s\n", it->c_str() );)
                (*revs)[*it] = 0;
            }

        }

        PART_REVS::iterator  result = revs->find( aLPID.GetPartNameAndRev() );

        if( result != revs->end() )
        {
            if( !result->second )    // the PART has never been loaded before
            {
                result->second = new PART( this, aLPID.GetPartNameAndRev() );
            }

            return result->second;
        }

        // If caller did not say what revision, find the highest numbered one and return that.
        // Otherwise he knew what he wanted specifically, and we do not have it.
        if( !aLPID.GetRevision().size() && revs->size() )
        {
            result = revs->begin();     // sort order has highest rev first

            if( !result->second )       // the PART has never been loaded before
            {
                result->second = new PART( this, LPID::Format( "", aLPID.GetPartName(), result->first ) );
            }

            return result->second;
        }
    }

    return 0;   // no such part name in this lib
}


PART* LIB::LookupPart( const LPID& aLPID, LIB_TABLE* aLibTable ) throw( IO_ERROR )
{
    PART*   part = (PART*) findPart( aLPID );

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

        SWEET_LEXER sw( part->body, wxString::FromUTF8("body") /* @todo have ReadPart give better source */ );

        part->Parse( &sw, aLibTable );
    }

    return part;
}


#if 0 && defined(DEBUG)

void LIB::Test( int argc, char** argv ) throw( IO_ERROR );
{
}

int main( int argc, char** argv )
{
    LIB::Test( argc, argv );

    return 0;
}

#endif
