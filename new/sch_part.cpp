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
#include <sweet_lexer.h>
#include <sch_lpid.h>
#include <sch_lib_table.h>

using namespace SCH;


#define MAX_INHERITANCE_NESTING     6      // no problem going larger


//-----<temporary home for PART sub objects, move after stable>------------------
struct XY {};
struct AT {};

class POLY_LINE
{

};


//-----</temporary home for PART sub objects, move after stable>-----------------


/**
 * Class PART_PARSER
 * is a localized/hidden PART Parser.  You get here through the public interface
 * PART::Parse().  Advantages of private class declaration in this situation:
 * 1) keeps all the recursive parsing helper functions out of the main public PART
*     header file and so should speed up compilation.
 * 2) Allows use of cost-less Java like inline functions, since nobody knows about
 *    them but this source file.  Most are only used once and called from one place.
 * <p>
 * All the functions in this class throw PARSE_ERROR.  If SWEET_LEXER throws, it
 * may be an IO_ERROR, propogated from here also.  The throws() statements are left off
 * to keep the noise level down.
 */
class PART_PARSER
{
    SWEET_LEXER*    in;
    LIB_TABLE*      libs;
    int             contains;       // separate from PART::contains until done
                                    // so we can see what we inherited from base PART

public:
    PART_PARSER( PART* aPart, SWEET_LEXER* aLexer, LIB_TABLE* aTable ) :
        in( aLexer ),
        libs( aTable ),
        contains( 0 )
    {
        parsePart( aPart );
    }


    void parseXY( XY* me )
    {
    }

    void parseAt( AT* me )
    {
    }


    void parseExtends( PART* me )
    {
        PART*   base;
        int     offset;

        if( contains & PB(EXTENDS) )
            in->Duplicate( T_extends );

        in->NeedSYMBOLorNUMBER();
        me->setExtends( new LPID() );

        offset = me->extends->Parse( in->CurText() );
        if( offset > -1 )   // -1 is success
            THROW_PARSE_ERROR( _("invalid extends LPID"),
                in->CurSource(),
                in->CurLine(),
                in->CurLineNumber(),
                in->CurOffset() + offset );

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
                    in->CurSource(),
                    in->CurLine(),
                    in->CurLineNumber(),
                    in->CurOffset() );
            }
        }

        if( extendsDepth == MAX_INHERITANCE_NESTING )
        {
            THROW_PARSE_ERROR( _("max allowed extends depth exceeded"),
                in->CurSource(),
                in->CurLine(),
                in->CurLineNumber(),
                in->CurOffset() );
        }

        me->inherit( *base );
        me->base = base;
        contains |= PB(EXTENDS);
    }

    /// @param me = ja mir, the object getting stuffed, from its perspective
    void parsePart( PART* me )
    {
        PART_T tok;

#if 0
        // Be flexible regarding the starting point of the stream.
        // Caller may not have read the first two tokens out of the
        // stream: T_LEFT and T_part, so ignore them if seen here.
        // The 1st two tokens T_LEFT and T_part are then optional in the grammar.

        if( (tok = in->NextTok() ) == T_LEFT )
        {
            if( ( tok = in->NextTok() ) != T_part )
                in->Expecting( T_part );
        }

#else
        // "( part" are not optional
        in->NeedLEFT();

        if( ( tok = in->NextTok() ) != T_part )
            in->Expecting( T_part );
#endif

        in->NeedSYMBOLorNUMBER(); // read in part NAME_HINT, and toss
        tok = in->NextTok();

        // extends must be _first_ thing, if it is present at all, after NAME_HINT
        if( tok == T_extends )
        {
            parseExtends( me );
            tok = in->NextTok();
        }

        for(  ; tok!=T_RIGHT;  tok = in->NextTok() )
        {
            if( tok==T_EOF )
                in->Unexpected( _( "end of input" ) );

            if( tok == T_LEFT )
                tok = in->NextTok();

            switch( tok )
            {
            default:
                // describe what we expect at this level
                in->Expecting( wxT(
                    "anchor|value|footprint|model|keywords|alternates\n"
                    "|property\n"
                    "  |property_del\n"
                    "|pin\n"
                    "  |pin_merge|pin_swap|pin_renum|pin_rename|route_pin_swap\n"
                    "|polyline|line|rectangle|circle|arc|bezier|text"
                    )
                 );
                break;

            case T_anchor:
                if( contains & PB(ANCHOR) )
                    in->Duplicate( tok );
                contains |= PB(ANCHOR);
                break;

            case T_line:

                break;


        /*
            case T_value:
                if( contains & PB(VALUE) )
                    in->Duplicate( tok );
                contains |= PB(VALUE);
                in->NeedSYMBOLorNUMBER();
                // me->value = in->CurText();
                in->NeedRIGHT();
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
                    in->Duplicate( tok );
                contains |= PB(REFERENCE);
                break;
            }
        }

        contains |= PB(PARSED);

        me->contains |= contains;
    }

};


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


PART::~PART()
{
    delete extends;
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


void PART::Parse( SWEET_LEXER* aLexer, LIB_TABLE* aTable ) throw( IO_ERROR )
{
    PART_PARSER( this, aLexer, aTable );
}




#if 0 && defined(DEBUG)

int main( int argc, char** argv )
{
    return 0;
}

#endif

