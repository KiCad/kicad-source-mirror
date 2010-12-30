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

using namespace SCH;


LIB::LIB( const STRING& aLogicalLibrary, LIB_SOURCE* aSource, LIB_SINK* aSink ) :
    name( aLogicalLibrary ),
    source( aSource ),
    sink( aSink )
{
}


LIB::~LIB()
{
    delete source;
    delete sink;
}


PART* LIB::LookupPart( const LPID& aLPID, LIB_TABLE* aLibTable ) throw( IO_ERROR )
{
    PART*   part;

    // If part not already cached
    if( 1 /* @todo test cache */ )
    {
        // load it.

        part = new PART( this, aLPID.GetPartName(), aLPID.GetRevision() );

        std::auto_ptr<PART> wrapped( part );

        source->ReadPart( &part->body, aLPID.GetPartName(), aLPID.GetRevision() );

#if defined(DEBUG)
        const STRING& body = part->body;
        printf( "body: %s", body.c_str() );
        if( !body.size() || body[body.size()-1] != '\n' )
            printf( "\n" );
#endif

        SWEET_LEXER sw( part->body, wxString::FromUTF8("body") /* @todo have ReadPart give better source */ );

        part->Parse( &sw, aLibTable );


        // stuff the part into this LIBs cache:
        // @todo

        wrapped.release();
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
