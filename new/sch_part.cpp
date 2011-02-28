/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
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

#include <wx/wx.h>          // _()

#include <sch_part.h>
#include <sch_sweet_parser.h>
#include <sch_lpid.h>
#include <sch_lib_table.h>


using namespace SCH;


PART::PART( LIB* aOwner, const STRING& aPartNameAndRev ) :
    owner( aOwner ),
    contains( 0 ),
    partNameAndRev( aPartNameAndRev ),
    extends( 0 ),
    base( 0 )
{
    // Our goal is to have class LIB only instantiate what is needed, so print here
    // what it is doing. It is the only class where PART can be instantiated.
    D(printf("PART::PART(%s)\n", aPartNameAndRev.c_str() );)
}


void PART::clear()
{
    if( extends )
    {
        delete extends;
        extends = 0;
    }

    // graphics objects I own, and the container will not destroy them:
    for( GRAPHICS::iterator it = graphics.begin();  it != graphics.end();  ++it )
        delete *it;
    graphics.clear();


    // @todo delete all properties, pins, and graphics
}


PART::~PART()
{
    clear();
}


void PART::setExtends( LPID* aLPID )
{
    delete extends;
    extends = aLPID;
}


void PART::inherit( const PART& other )
{
    contains = other.contains;

    // @todo copy the inherited drawables, properties, and pins here
}


PART& PART::operator=( const PART& other )
{
    owner          = other.owner;
    partNameAndRev = other.partNameAndRev;
    body           = other.body;
    base           = other.base;

    setExtends( other.extends ? new LPID( *other.extends ) : 0 );

    // maintain in concert with inherit(), which is a partial assignment operator.
    inherit( other );

    return *this;
}


void PART::Parse( SWEET_PARSER* aParser, LIB_TABLE* aTable ) throw( IO_ERROR, PARSE_ERROR )
{
    aParser->Parse( this, aTable );
}


#if 0 && defined(DEBUG)

int main( int argc, char** argv )
{
    return 0;
}

#endif

