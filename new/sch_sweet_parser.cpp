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

#include <sch_sweet_parser.h>
#include <sch_part.h>
#include <sch_lib_table.h>
#include <sch_lpid.h>

using namespace SCH;
using namespace PR;


#define MAX_INHERITANCE_NESTING     6      // no problem going larger

/**
 * Function log2int
 * converts a logical coordinate to an internal coordinate.  Logical coordinates
 * are defined as the standard distance between pins being equal to one.
 * Internal coordinates are 1000 times that.
 */
static inline int log2int( double aCoord )
{
    return int( aCoord * 1000 );
}

static inline int internal( const STRING& aCoord )
{
    return log2int( strtod( aCoord.c_str(), NULL ) );
}


/**
 * Enum PartBit
 * is a set of bit positions that can be used to create flag bits within
 * PART::contains to indicate what state the PART is in and what it contains, i.e.
 * whether the PART has been parsed, and what the PART contains, categorically.
 */
enum PartBit
{
    PARSED,     ///< have parsed this part already, otherwise 'body' text must be parsed
    EXTENDS,    ///< saw "extends" keyword, inheriting from another PART
    VALUE,
    ANCHOR,
    REFERENCE,
    FOOTPRINT,
    DATASHEET,
    MODEL,
    KEYWORDS,
};


/// Function PB
/// is a PartBit shifter for PART::contains field.
static inline const int PB( PartBit oneBitOnly )
{
    return ( 1 << oneBitOnly );
}



void SWEET_PARSER::parseExtends( PART* me )
{
    PART*   base;
    int     offset;

    if( contains & PB(EXTENDS) )
        Duplicate( T_extends );

    NeedSYMBOLorNUMBER();
    me->setExtends( new LPID() );

    offset = me->extends->Parse( CurText() );
    if( offset > -1 )   // -1 is success
        THROW_PARSE_ERROR( _("invalid extends LPID"),
            CurSource(),
            CurLine(),
            CurLineNumber(),
            CurOffset() + offset );

    base = libs->LookupPart( *me->extends, me->Owner() );

    // we could be going in circles here, recursively, or too deep, set limits
    // and disallow extending from self (even indirectly)
    int extendsDepth = 0;
    for( PART* ancestor = base; ancestor && extendsDepth<MAX_INHERITANCE_NESTING;
            ++extendsDepth, ancestor = ancestor->base )
    {
        if( ancestor == me )
        {
            THROW_PARSE_ERROR( _("'extends' may not have self as any ancestor"),
                CurSource(),
                CurLine(),
                CurLineNumber(),
                CurOffset() );
        }
    }

    if( extendsDepth == MAX_INHERITANCE_NESTING )
    {
        THROW_PARSE_ERROR( _("max allowed extends depth exceeded"),
            CurSource(),
            CurLine(),
            CurLineNumber(),
            CurOffset() );
    }

    me->inherit( *base );
    me->base = base;
    contains |= PB(EXTENDS);
}


void SWEET_PARSER::Parse( PART* me, LIB_TABLE* aTable ) throw( IO_ERROR, PARSE_ERROR )
{
    T       tok;

    libs = aTable;

    // empty everything out, could be re-parsing this object and it may not be empty.
    me->clear();

#if 0
    // Be flexible regarding the starting point of the stream.
    // Caller may not have read the first two tokens out of the
    // stream: T_LEFT and T_part, so ignore them if seen here.
    // The 1st two tokens T_LEFT and T_part are then optional in the grammar.

    if( (tok = NextTok() ) == T_LEFT )
    {
        if( ( tok = NextTok() ) != T_part )
            Expecting( T_part );
    }

#else
    // "( part" are not optional
    NeedLEFT();

    if( ( tok = NextTok() ) != T_part )
        Expecting( T_part );
#endif

    NeedSYMBOLorNUMBER(); // read in part NAME_HINT, and toss
    tok = NextTok();

    // extends must be _first_ thing, if it is present at all, after NAME_HINT
    if( tok == T_extends )
    {
        parseExtends( me );
        tok = NextTok();
    }

    for(  ; tok!=T_RIGHT;  tok = NextTok() )
    {
        if( tok==T_EOF )
            Unexpected( T_EOF );

        if( tok == T_LEFT )
            tok = NextTok();

        switch( tok )
        {
        default:
            // describe what we expect at this level
            Expecting(
                "anchor|value|footprint|model|keywords|alternates\n"
                "|property\n"
                "  |property_del\n"
                "|pin\n"
                "  |pin_merge|pin_swap|pin_renum|pin_rename|route_pin_swap\n"
                "|polyline|line|rectangle|circle|arc|bezier|text"
             );
            break;

        case T_anchor:
            if( contains & PB(ANCHOR) )
                Duplicate( tok );
            NeedNUMBER( "anchor x" );
            me->anchor.x = internal( CurText() );
            NeedNUMBER( "anchor y" );
            me->anchor.y = internal( CurText() );
            contains |= PB(ANCHOR);
            break;

        case T_line:
            POLY_LINE*  pl;
            pl = new POLY_LINE( me );
            me->graphics.push_back( pl );
            parsePolyLine( pl );
            break;


    /*
        case T_value:
            if( contains & PB(VALUE) )
                Duplicate( tok );
            contains |= PB(VALUE);
            NeedSYMBOLorNUMBER();
            // me->value = CurText();
            NeedRIGHT();
            break;

        case T_footprint:
            break;

        case T_model:
            break;

        case T_keywords:
            break;

        case T_alternates:
            break;

        case T_property:
            break;

        case T_property_del:
            break;

        case T_pin:
            break;

        case T_pin_merge:
            break;

        case T_pin_swap:
            break;

        case T_pin_renum:
            break;

        case T_pin_rename:
            break;

        case T_route_pin_swap:
            break;

        case T_polyline:
            break;

        case T_rectangle:
            break;

        case T_circle:
            break;

        case T_arc:
            break;

        case T_bezier:
            break;

        case T_text:
            break;
    */

        // Not sure about reference in a PART, comes in at COMPONENT object.
        // It is maybe just a hint here or a prefix.
        case T_reference:
            if( contains & PB(REFERENCE) )
                Duplicate( tok );
            contains |= PB(REFERENCE);
            break;
        }
    }

    contains |= PB(PARSED);

    me->contains |= contains;
}


void SWEET_PARSER::parsePolyLine( POLY_LINE* me )
{
    T       tok;
    int     count;

    NeedLEFT();
    while( ( tok = NextTok() ) != T_RIGHT )
    {
        NeedLEFT();

        tok = NextTok();
        switch( tok )
        {
        case T_line_width:
            NeedNUMBER( "line_width" );
            me->width = strtod( CurText(), NULL );
            break;

        case T_pts:
            for( count=0;  ( tok = NextTok() ) != T_RIGHT;  ++count )
            {
                if( tok != T_LEFT )
                    Expecting( T_LEFT );

                tok = NeedSYMBOL();
                if( tok != T_xy )
                    Expecting( T_xy );

                /* @todo resume here, its late
                NeedNUMBER();
                */
            }
            break;

        case T_fill:
            break;

        default:
            Expecting( "pts|line_width|fill" );
        }
    }
}

