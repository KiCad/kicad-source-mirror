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


//-----<temporary home for PART sub objects, move after stable>------------------



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
    int             contains;

public:
    PART_PARSER( PART* aPart, SWEET_LEXER* aLexer, LIB_TABLE* aTable ) :
        in( aLexer ),
        libs( aTable ),
        contains( 0 )
    {
        parsePart( aPart );
    }

    /// @param me = ja mir, the object getting stuffed, from its perspective
    void parsePart( PART* me )
    {
        PART_T tok;

        if( (tok = in->NextTok()) == T_LEFT )
            tok = in->NextTok();

        // a token "( part .." i.e. class PART
        // Be flexible regarding the starting point of the stream.
        // Caller may not have read the first two tokens out of the
        // stream: T_LEFT and T_part, so ignore them if seen here.
        // The 1st two tokens T_LEFT and T_part are then optional in the grammar.
        if( tok == T_part )
        {
            in->NeedSYMBOLorNUMBER(); // read in part NAME_HINT, and toss
            tok = in->NextTok();
        }

        // extends must be _first_ thing, if it is present at all, after NAME_HINT
        if( tok == T_extends )
        {
            PART*   base;
            int     offset;

            if( contains & PB(EXTENDS) )
                in->Duplicate( tok );
            in->NeedSYMBOLorNUMBER();
            me->setExtends( new LPID() );
            offset = me->extends->Parse( in->CurText() );
            if( offset > -1 )   // -1 is success
                THROW_PARSE_ERROR( _("invalid extends LPID"),
                    in->CurSource(),
                    in->CurLine(),
                    in->CurLineNumber(),
                    in->CurOffset() + offset );
            // we could be going in circles here, recursively, @todo add a max counter or stack chain
            base = libs->LookupPart( *me->extends, me->Owner() );
            me->inherit( *base );
            contains |= PB(EXTENDS);
            tok = in->NextTok();
        }

        for(  ; tok!=T_RIGHT && tok!=T_EOF;  tok = in->NextTok() )
        {
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

            case T_line:
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
    }


    void parseAt( PART* me )
    {
    }
};


PART::PART( LIB* aOwner, const STRING& aPartName, const STRING& aRevision ) :
    owner( aOwner ),
    contains( 0 ),
    partName( aPartName ),
    revision( aRevision ),
    extends( 0 )
{}


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

    setExtends( other.extends ? new LPID( *other.extends ) : 0 );

    body     = other.body;
}


PART& PART::operator=( const PART& other )
{
    owner    = other.owner;
    partName = other.partName;
    revision = other.revision;

    // maintain inherit() as a partial assignment operator.
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

