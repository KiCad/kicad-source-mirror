/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2011 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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
//#include <richio.h>

using namespace SCH;


PART::PART( LIB* aOwner, const STRING& aPartNameAndRev ) :
    owner( aOwner ),
    contains( 0 ),
    partNameAndRev( aPartNameAndRev ),
    extends( 0 ),
    base( 0 ),
    reference(  this, wxT( "reference " ) ),
    value(      this, wxT( "value" ) ),
    footprint(  this, wxT( "footprint" ) ),
    model(      this, wxT( "model" ) ),
    datasheet(  this, wxT( "datasheet" ) )
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

    // delete graphics I own, since their container will not destroy them:
    for( GRAPHICS::iterator it = graphics.begin();  it != graphics.end();  ++it )
        delete *it;
    graphics.clear();

    // delete PINs I own, since their container will not destroy them.
    for( PINS::iterator it = pins.begin();  it != pins.end();  ++it )
        delete *it;
    pins.clear();

    // delete non-mandatory properties I own, since their container will not destroy them:
    for( PROPERTIES::iterator it = properties.begin();  it != properties.end();  ++it )
        delete *it;
    properties.clear();

    keywords.clear();

    contains = 0;

    // @todo clear the mandatory fields
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


void PART::PropertyDelete( const wxString& aPropertyName ) throw( IO_ERROR )
{
    PROPERTIES::iterator it = propertyFind( aPropertyName );
    if( it == properties.end() )
    {
        wxString    msg;
        msg.Printf( _( "Unable to find property: %s" ), aPropertyName.GetData() );
        THROW_IO_ERROR( msg );
    }

    delete *it;
    properties.erase( it );
    return;
}


PROPERTIES::iterator PART::propertyFind( const wxString& aPropertyName )
{
    PROPERTIES::iterator it;
    for( it = properties.begin();  it!=properties.end();  ++it )
        if( (*it)->name == aPropertyName )
            break;
    return it;
}


void PART::Format( OUTPUTFORMATTER* out, int indent, int ctl ) const throw( IO_ERROR )
{
    out->Print( indent, "(part %s", partNameAndRev.c_str() );

    if( extends )
        out->Print( 0, " inherits %s", extends->Format().c_str() );

    out->Print( 0, "\n" );

/*
    @todo
    for( int i=0; i<MANDATORY_FIELDS;  ++i )
    {
    }
*/
    for( PROPERTIES::const_iterator it = properties.begin();  it != properties.end();  ++it )
    {
        (*it)->Format( out, indent+1, ctl );
    }

    if( anchor.x || anchor.y )
    {
        out->Print( indent+1, "(anchor (at %.6g %.6g))\n",
            InternalToLogical( anchor.x ),
            InternalToLogical( anchor.y ) );
    }

    for( GRAPHICS::const_iterator it = graphics.begin();  it != graphics.end();  ++it )
    {
        (*it)->Format( out, indent+1, ctl );
    }

    for( PINS::const_iterator it = pins.begin();  it != pins.end();  ++it )
    {
        (*it)->Format( out, indent+1, ctl );
    }
}


