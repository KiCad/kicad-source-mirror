/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#include <sch_sweet_parser.h>
#include <sch_part.h>
#include <sch_lib_table.h>
#include <sch_lpid.h>

#include <macros.h>

using namespace SCH;
using namespace PR;


#define MAX_INHERITANCE_NESTING     6       ///< max depth of inheritance, no problem going larger


static inline int internal( const STRING& aCoord )
{
    return LogicalToInternal( strtod( aCoord.c_str(), NULL ) );
}

static inline int fromWidth( const STRING& aWidth )
{
    return WidthToInternal( strtod( aWidth.c_str(), NULL ) );
}

static inline int fromFontz( const STRING& aFontSize )
{
    return FontzToInternal( strtod( aFontSize.c_str(), NULL ) );
}


/**
 * Enum PartBit
 * is a set of bit positions that can be used to create flag bits within
 * PART::contains to indicate what state the PART is in and what it contains, i.e.
 * whether the PART has been parsed, and what the PART contains, categorically.
 */
enum PartBit
{
    parsed,     ///< have parsed this part already, otherwise 'body' text must be parsed
    extends,    ///< saw "extends" keyword, inheriting from another PART
    value,
    anchor,
    reference,
    footprint,
    datasheet,
    model,
    keywords,
};


/// Function PB
/// is a PartBit shifter for PART::contains field.
static inline const int PB( PartBit oneBitOnly )
{
    return ( 1 << oneBitOnly );
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
    if( ( tok = NextTok() ) == T_LEFT )
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

    NeedSYMBOLorNUMBER();       // toss NAME_HINT
    tok = NextTok();

    // extends must be _first_ thing, if it is present at all, after NAME_HINT
    if( tok == T_extends )
    {
        parseExtends( me );
        tok = NextTok();
    }

    for(  ; tok!=T_RIGHT;  tok = NextTok() )
    {
        if( tok == T_LEFT )
        {
            PROPERTY*   prop;

            tok = NextTok();

            // because exceptions are thrown, any 'new' allocation has to be stored
            // somewhere other than on the stack, ASAP.

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
                if( contains & PB(anchor) )
                    Duplicate( tok );
                NeedNUMBER( "anchor x" );
                me->anchor.x = internal( CurText() );
                NeedNUMBER( "anchor y" );
                me->anchor.y = internal( CurText() );
                contains |= PB(anchor);
                break;

            case T_line:
            case T_polyline:
                POLY_LINE*  pl;
                pl = new POLY_LINE( me );
                me->graphics.push_back( pl );
                parsePolyLine( pl );
                break;

            case T_rectangle:
                RECTANGLE* rect;
                rect = new RECTANGLE( me );
                me->graphics.push_back( rect );
                parseRectangle( rect );
                break;

            case T_circle:
                CIRCLE* circ;
                circ = new CIRCLE( me );
                me->graphics.push_back( circ );
                parseCircle( circ );
                break;

            case T_arc:
                ARC* arc;
                arc = new ARC( me );
                me->graphics.push_back( arc );
                parseArc( arc );
                break;

            case T_bezier:
                BEZIER* bezier;
                bezier = new BEZIER( me );
                me->graphics.push_back( bezier );
                parseBezier( bezier );
                break;

            case T_text:
                GR_TEXT* text;
                text = new GR_TEXT( me );
                me->graphics.push_back( text );
                parseText( text );
                break;

            case T_property:
                prop = new PROPERTY( me );
                // @todo check for uniqueness
                me->properties.push_back( prop );
                NeedSYMBOLorNUMBER();
                prop->name = FromUTF8();

            L_prop:
                NeedSYMBOLorNUMBER();
                prop->text = FromUTF8();
                tok = NextTok();
                if( tok == T_LEFT )
                {
                    tok = NextTok();
                    if( tok != T_effects )
                        Expecting( T_effects );
                    parseTextEffects( prop->EffectsLookup() );
                    NeedRIGHT();
                }
                else if( tok != T_RIGHT )
                    Expecting( ") | effects" );
                break;

            case T_property_del:
                parsePropertyDel( me );
                break;

            // reference in a PART is incomplete, it is just the prefix of an
            // unannotated reference. Only components have full reference designators.
            case T_reference:
                if( contains & PB(reference) )
                    Duplicate( tok );
                contains |= PB(reference);
                prop = me->FieldLookup( PART::REFERENCE );
                goto L_prop;

            case T_value:
                if( contains & PB(value) )
                    Duplicate( tok );
                contains |= PB(value);
                prop = me->FieldLookup( PART::VALUE );
                goto L_prop;

            case T_footprint:
                if( contains & PB(footprint) )
                    Duplicate( tok );
                contains |= PB(footprint);
                prop = me->FieldLookup( PART::FOOTPRINT );
                goto L_prop;

            case T_datasheet:
                if( contains & PB(datasheet) )
                    Duplicate( tok );
                contains |= PB(datasheet);
                prop = me->FieldLookup( PART::DATASHEET );
                goto L_prop;

            case T_model:
                if( contains & PB(model) )
                    Duplicate( tok );
                contains |= PB(model);
                prop = me->FieldLookup( PART::MODEL );
                goto L_prop;

            case T_keywords:
                parseKeywords( me );
                break;

            case T_alternates:
                // @todo: do we want to inherit alternates?
                parseAlternates( me );
                break;

            case T_pin:
                // @todo PADNAMEs must be unique
                PIN* pin;
                pin = new PIN( me );
                me->pins.push_back( pin );
                parsePin( pin );
                break;

            case T_pin_del:
                parsePinDel( me );
                break;

            case T_pin_swap:
                parsePinSwap( me );
                break;

            case T_pin_renum:
                parsePinRenum( me );
                break;

            case T_pin_rename:
                parsePinRename( me );
                break;

            case T_pin_merge:
                parsePinMerge( me );
                break;

            /*
            @todo

            case T_route_pin_swap:
                break;
            */
            }
        }

        else
        {
            switch( tok )
            {
            default:
                Unexpected( tok );
            }
        }
    }

    contains |= PB(parsed);

    me->contains |= contains;
}


void SWEET_PARSER::parseExtends( PART* me )
{
    PART*   base;
    int     offset;

    if( contains & PB(extends) )
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
    for( const PART* ancestor = base; ancestor && extendsDepth<MAX_INHERITANCE_NESTING;
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
    contains |= PB(extends);
}


void SWEET_PARSER::parseAlternates( PART* me )
{
    T           tok;
    PART_REF    lpid;
    int         offset;

    while( ( tok = NextTok() ) != T_RIGHT )
    {
        if( !IsSymbol( tok ) && tok != T_NUMBER )
            Expecting( "lpid" );

        // lpid.clear(); Parse does this

        offset = lpid.Parse( CurText() );
        if( offset > -1 )
        THROW_PARSE_ERROR( _("invalid alternates LPID"),
            CurSource(),
            CurLine(),
            CurLineNumber(),
            CurOffset() + offset );

        // PART_REF assignment should be OK, it contains no ownership
        me->alternates.push_back( lpid );
    }
}


void SWEET_PARSER::parseKeywords( PART* me )
{
    T   tok;

    while( ( tok = NextTok() ) != T_RIGHT )
    {
        if( !IsSymbol( tok ) && tok!=T_NUMBER )
            Expecting( "symbol|number" );

        // just insert them, duplicates are silently removed and tossed.
        me->keywords.insert( FromUTF8() );
    }
}


void SWEET_PARSER::parseFont( FONT* me )
{
    /*
        # The FONT value needs to be defined.  Currently, EESchema does not support
        # different fonts.  In the future this feature may be implemented and at
        # that time FONT will have to be defined.  Initially, only the font size and
        # style are required.  Italic and bold styles are optional.  The font size
        # height and width are in units yet to be determined.
        (font [FONT] (size HEIGHT WIDTH) [italic] [bold])
    */

    // handle the [FONT] 'position dependently', i.e. first
    T       tok = NextTok();
    bool    sawBold   = false;
    bool    sawItalic = false;
    bool    sawSize   = false;

    if( IsSymbol( tok ) )
    {
        me->name = FromUTF8();
        tok = NextTok();
    }

    for( ; tok != T_RIGHT; tok = NextTok() )
    {
        if( tok == T_LEFT )
        {
            tok = NextTok();

            switch( tok )
            {
            case T_size:
                if( sawSize )
                    Duplicate( T_size );
                sawSize = true;

                NeedNUMBER( "size height" );
                me->size.height = fromFontz( CurText() );

                NeedNUMBER( "size width" );
                me->size.width = fromFontz( CurText() );
                NeedRIGHT();
                break;

            default:
                Expecting( "size" );
            }
        }
        else
        {
            switch( tok )
            {
            case T_bold:
                if( sawBold )
                    Duplicate( T_bold );
                sawBold = true;
                me->bold = true;
                break;

            case T_italic:
                if( sawItalic )
                    Duplicate( T_italic );
                sawItalic = true;
                me->italic = true;
                break;

            default:
                Unexpected( "bold|italic" );
            }
        }
    }
}


void SWEET_PARSER::parseBool( bool* aBool )
{
    T   tok = NeedSYMBOL();

    switch( tok )
    {
    case T_yes:
    case T_no:
        *aBool = (tok == T_yes);
        break;
    default:
        Expecting( "yes|no" );
    }
}


void SWEET_PARSER::parseStroke( STROKE* me )
{
    /*
        (stroke [WIDTH] [(style [(dashed...)]...)])

        future place holder for arrow heads, dashed lines, all line glamour
    */

    NeedNUMBER( "stroke" );
    *me = fromWidth( CurText() );
    NeedRIGHT();
}


void SWEET_PARSER::parsePinText( PINTEXT* me )
{
    /*  either:
        (signal SIGNAL   (font [FONT] (size HEIGHT WIDTH) [italic] [bold])(visible YES))
        or
        (pad PADNAME (font [FONT] (size HEIGHT WIDTH) [italic] [bold])(visible YES))
    */
    T       tok;
    bool    sawFont = false;
    bool    sawVis  = false;

    // pad or signal text
    NeedSYMBOLorNUMBER();
    me->text = FromUTF8();

    while( ( tok = NextTok() ) != T_RIGHT )
    {
        if( tok == T_LEFT )
        {
            tok = NextTok();

            switch( tok )
            {
            case T_font:
                if( sawFont )
                    Duplicate( tok );
                sawFont = true;
                parseFont( &me->font );
                break;

            case T_visible:
                if( sawVis )
                    Duplicate( tok );
                sawVis = true;
                parseBool( &me->isVisible );
                NeedRIGHT();
                break;

            default:
                Expecting( "font" );
            }
        }

        else
        {
            switch( tok )
            {
            default:
                Expecting( T_LEFT );
            }
        }
    }
}


void SWEET_PARSER::parsePin( PIN* me )
{
    /*
        (pin TYPE SHAPE
            (at X Y [ANGLE])
            (length LENGTH)
            (signal NAME (font [FONT] (size HEIGHT WIDTH) [italic] [bold])(visible YES))
            (pad NUMBER (font [FONT] (size HEIGHT WIDTH) [italic] [bold] (visible YES))
            (visible YES)
        )
    */

    T       tok;
    bool    sawShape   = false;
    bool    sawType    = false;
    bool    sawAt      = false;
    bool    sawLen     = false;
    bool    sawSignal  = false;
    bool    sawPad     = false;
    bool    sawVis     = false;

    while( ( tok = NextTok() ) != T_RIGHT )
    {
        if( tok == T_LEFT )
        {
            tok = NextTok();

            switch( tok )
            {
            case T_at:
                if( sawAt )
                    Duplicate( tok );
                sawAt = true;
                parseAt( &me->pos, &me->angle );
                break;

            case T_length:
                if( sawLen )
                    Duplicate( tok );
                sawLen = true;
                NeedNUMBER( "length" );
                me->length = internal( CurText() );
                NeedRIGHT();
                break;

            case T_signal:
                if( sawSignal )
                    Duplicate( tok );
                sawSignal = true;
                parsePinText( &me->signal );
                break;

            case T_pad:
                if( sawPad )
                    Duplicate( tok );
                sawPad = true;
                parsePinText( &me->pad );
                break;

            case T_visible:
                if( sawVis )
                    Duplicate( tok );
                sawVis = true;
                parseBool( &me->isVisible );
                NeedRIGHT();
                break;

            default:
                Unexpected( tok );
            }
        }

        else    // not wrapped in parentheses
        {
            switch( tok )
            {
            case T_in:
            case T_out:
            case T_inout:
            case T_tristate:
            case T_passive:
            case T_unspecified:
            case T_power_in:
            case T_power_out:
            case T_open_collector:
            case T_open_emitter:
            case T_unconnected:
                if( sawType )
                    Duplicate( tok );
                sawType = true;
                me->connectionType = tok;
                break;

            case T_none:
            case T_line:
            case T_inverted:
            case T_clock:
            case T_inverted_clk:
            case T_input_low:
            case T_clock_low:
            case T_falling_edge:
            case T_non_logic:
                if( sawShape )
                    Duplicate( tok );
                sawShape = true;
                me->shape = tok;
                break;

            default:
                Unexpected( tok );
            }
        }
    }
}


void SWEET_PARSER::parsePinDel( PART* me )
{
    wxString    pad;

    // we do this somewhat unorthodoxically because we want to avoid doing two lookups,
    // which would need to be done to 1) find pin, and 2) delete pin.  Only one
    // lookup is needed with this scheme.

    NeedSYMBOLorNUMBER();
    pad = FromUTF8();

    // lookup now while CurOffset() is still meaningful.
    PINS::iterator it = me->pinFindByPad( pad );
    if( it == me->pins.end() )
    {
        THROW_PARSE_ERROR( _("undefined pin"),
            CurSource(),
            CurLine(),
            CurLineNumber(),
            CurOffset() );
    }

/* enable in future, but not now while testing
    if( (*it)->birthplace == me )
    {
        THROW_PARSE_ERROR( _("pin_del allowed for inherited pins only"),
            CurSource(),
            CurLine(),
            CurLineNumber(),
            CurOffset() );
    }
*/

    NeedRIGHT();

    delete *it;         // good thing I'm a friend.
    me->pins.erase( it );
}


void SWEET_PARSER::parsePinSwap( PART* me )
{
    PIN*        pin1;
    PIN*        pin2;

    wxString    pad;

    NeedSYMBOLorNUMBER();
    pad = FromUTF8();

    // lookup now while CurOffset() is still meaningful.
    pin1 = me->PinFindByPad( pad );
    if( !pin1 )
    {
        THROW_PARSE_ERROR( _("undefined pin"),
            CurSource(),
            CurLine(),
            CurLineNumber(),
            CurOffset() );
    }

    NeedSYMBOLorNUMBER();
    pad = FromUTF8();

    pin2 = me->PinFindByPad( pad );
    if( !pin2 )
    {
        THROW_PARSE_ERROR( _("undefined pin"),
            CurSource(),
            CurLine(),
            CurLineNumber(),
            CurOffset() );
    }

    NeedRIGHT();

    // swap only the text, but might want to swap entire PIN_TEXTs
    pin2->pad.text = pin1->pad.text;
    pin1->pad.text = pad;
}


void SWEET_PARSER::parsePinRenum( PART* me )
{
    PIN*        pin;

    wxString    oldPad;
    wxString    newPad;

    NeedSYMBOLorNUMBER();
    oldPad = FromUTF8();

    // lookup now while CurOffset() is still meaningful.
    pin = me->PinFindByPad( oldPad );
    if( !pin )
    {
        THROW_PARSE_ERROR( _("undefined pin"),
            CurSource(),
            CurLine(),
            CurLineNumber(),
            CurOffset() );
    }

    NeedSYMBOLorNUMBER();
    newPad = FromUTF8();

    NeedRIGHT();

    // @todo: check for pad legalities
    pin->pad.text = newPad;
}


void SWEET_PARSER::parsePinRename( PART* me )
{
    PIN*        pin;

    wxString    pad;
    wxString    newSignal;

    NeedSYMBOLorNUMBER();
    pad = FromUTF8();

    // lookup now while CurOffset() is still meaningful.
    pin = me->PinFindByPad( pad );
    if( !pin )
    {
        THROW_PARSE_ERROR( _("undefined pin"),
            CurSource(),
            CurLine(),
            CurLineNumber(),
            CurOffset() );
    }

    NeedSYMBOLorNUMBER();
    newSignal = FromUTF8();

    NeedRIGHT();

    pin->signal.text = newSignal;
}


void SWEET_PARSER::parsePinMerge( PART* me )
{
    T           tok;
    wxString    pad;
    wxString    signal;
    wxString    msg;

    NeedSYMBOLorNUMBER();

    wxString    anchorPad = FromUTF8();

    // lookup now while CurOffset() is still good.
    PINS::iterator pit = me->pinFindByPad( anchorPad );
    if( pit == me->pins.end() )
    {
        msg.Printf( _( "undefined pin %s" ), anchorPad.GetData() );
        THROW_PARSE_ERROR( msg,
            CurSource(),
            CurLine(),
            CurLineNumber(),
            CurOffset() );
    }

    if( !(*pit)->pin_merge.IsEmpty() && anchorPad != (*pit)->pin_merge )
    {
        msg.Printf( _( "pin %s already in pin_merge group %s" ),
                anchorPad.GetData(), (*pit)->pin_merge.GetData() );

        THROW_PARSE_ERROR( msg,
            CurSource(),
            CurLine(),
            CurLineNumber(),
            CurOffset() );
    }

    (*pit)->isVisible = true;
    (*pit)->pin_merge = anchorPad;

    // allocate or find a MERGE_SET;
    MERGE_SET& ms = me->pin_merges[anchorPad];

    while( ( tok = NextTok() ) != T_RIGHT )
    {
        if( tok == T_LEFT )
        {
            tok = NextTok();

            switch( tok )
            {
            case T_signals:
                {
                    PINS    sigPins;   // no ownership

                    while( ( tok = NextTok() ) != T_RIGHT )
                    {
                        if( !IsSymbol( tok ) && tok != T_NUMBER )
                            Expecting( "signal" );

                        signal = FromUTF8();

                        sigPins.clear();

                        me->PinsFindBySignal( &sigPins, signal );

                        if( !sigPins.size() )
                        {
                            msg.Printf( _( "no pins with signal %s" ), signal.GetData() );
                            THROW_PARSE_ERROR( msg,
                                CurSource(),
                                CurLine(),
                                CurLineNumber(),
                                CurOffset() );
                        }

                        for( pit = sigPins.begin();  pit != sigPins.end();  ++pit )
                        {
                            if( !(*pit)->pin_merge.IsEmpty() && anchorPad != (*pit)->pin_merge  )
                            {
                                msg.Printf( _( "signal pin %s already in pin_merge group %s" ),
                                        pad.GetData(), (*pit)->pin_merge.GetData() );

                                THROW_PARSE_ERROR( msg,
                                    CurSource(),
                                    CurLine(),
                                    CurLineNumber(),
                                    CurOffset() );
                            }

                            (*pit)->isVisible = true;
                            (*pit)->pin_merge = anchorPad;
                            ms.insert( pad );
                        }
                    }
                }
                break;

            case T_pads:
                while( ( tok = NextTok() ) != T_RIGHT )
                {
                    if( !IsSymbol( tok ) && tok != T_NUMBER )
                        Expecting( "pad" );

                    pad = FromUTF8();

                    D(printf("pad=%s\n", TO_UTF8( pad ) );)

                    // find the PIN and mark it as being in this MERGE_SET or throw
                    // error if already in another MERGET_SET.

                    pit = me->pinFindByPad( pad );
                    if( pit == me->pins.end() )
                    {
                        msg.Printf( _( "undefined pin %s" ), pad.GetData() );
                        THROW_PARSE_ERROR( msg,
                            CurSource(),
                            CurLine(),
                            CurLineNumber(),
                            CurOffset() );
                    }

                    if( !(*pit)->pin_merge.IsEmpty() /* && anchorPad != (*pit)->pin_merge */ )
                    {
                        msg.Printf( _( "pin %s already in pin_merge group %s" ),
                                pad.GetData(), (*pit)->pin_merge.GetData() );

                        THROW_PARSE_ERROR( msg,
                            CurSource(),
                            CurLine(),
                            CurLineNumber(),
                            CurOffset() );
                    }

                    (*pit)->isVisible = false;
                    (*pit)->pin_merge = anchorPad;

                    ms.insert( pad );
                }
                break;

            default:
                Expecting( "pads|signals" );
                break;
            }
        }
        else
        {
            Expecting( T_LEFT );
        }
    }
}


void SWEET_PARSER::parsePropertyDel( PART* me )
{
    NeedSYMBOLorNUMBER();

    wxString propertyName = FromUTF8();

    if( !me->PropDelete( propertyName ) )
    {
        wxString    msg;
        msg.Printf( _( "Unable to find property: %s" ), propertyName.GetData() );
        THROW_IO_ERROR( msg );
    }
    NeedRIGHT();
}


void SWEET_PARSER::parseTextEffects( TEXT_EFFECTS* me )
{
    /*
        (effects [PROPERTY]

            # Position requires X and Y coordinates.  Position coordinates can be
            # non-intergr.  Angle is in degrees and defaults to 0 if not defined.
            (at X Y [ANGLE])

            # The FONT value needs to be defined.  Currently, EESchema does not support
            # different fonts.  In the future this feature may be implemented and at
            # that time FONT will have to be defined.  Initially, only the font size and
            # style are required.  Italic and bold styles are optional.  The font size
            # height and width are in units yet to be determined.
            (font [FONT] (size HEIGHT WIDTH) [italic] [bold])

            # Valid visibility values are yes and no.
            (visible YES)
        )
    */

    bool    sawFont = false;
    bool    sawAt   = false;
    bool    sawVis  = false;

    T       tok = NextTok();

    if( IsSymbol( tok ) )
    {
        me->propName = FromUTF8();
        tok = NextTok();
    }

    for(  ; tok != T_RIGHT;  tok = NextTok() )
    {
        if( tok != T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_at:
            if( sawAt )
                Duplicate( tok );
            sawAt = true;
            parseAt( &me->pos, &me->angle );
            break;

        case T_font:
            if( sawFont )
                Duplicate( tok );
            sawFont = true;
            parseFont( &me->font );
            break;

        case T_visible:
            if( sawVis )
                Duplicate( sawVis );
            sawVis = true;
            parseBool( &me->isVisible );
            NeedRIGHT();
            break;

        default:
            Expecting( "at|font|visible" );
        }
    }
}


void SWEET_PARSER::parsePolyLine( POLY_LINE* me )
{
    /*
        (polyline|line
            (pts (xy X Y) (xy X Y) (xy X Y) (xy X Y) (xy X Y))

            # Line widths are in percent of a pin delta
            [(stroke [WIDTH] [(style [(dashed...)]...)])]


            # Valid fill types are none, filled, and transparent.
            (fill FILL_TYPE)
        )
    */

    T       tok;
    int     count = 0;
    bool    sawStroke = false;
    bool    sawFill   = false;

    while( ( tok = NextTok() ) != T_RIGHT )
    {
        if( tok != T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_stroke:
            if( sawStroke )
                Duplicate( tok );
            sawStroke = true;
            parseStroke( &me->stroke );
            break;

        case T_pts:
            if( count )
                Duplicate( tok );
            for(  ;  ( tok = NextTok() ) != T_RIGHT;  ++count )
            {
                if( tok != T_LEFT )
                    Expecting( T_LEFT );

                tok = NeedSYMBOL();
                if( tok != T_xy )
                    Expecting( T_xy );

                me->pts.push_back( POINT() );

                NeedNUMBER( "x" );
                me->pts.back().x = internal( CurText() );

                NeedNUMBER( "y" );
                me->pts.back().y = internal( CurText() );

                NeedRIGHT();
            }
            if( count < 2 )
                Expecting( ">= 2 pts" );
            break;

        case T_fill:
            if( sawFill )
                Duplicate( tok );
            tok = NeedSYMBOL();
            switch( tok )
            {
            case T_none:
            case T_filled:
            case T_transparent:
                me->fillType = tok;
                break;
            default:
                Expecting( "none|filled|transparent" );
            }
            NeedRIGHT();
            sawFill = true;
            break;

        default:
            Expecting( "pts|stroke|fill" );
        }
    }
}


void SWEET_PARSER::parseBezier( BEZIER* me )
{
    parsePolyLine( me );
}


void SWEET_PARSER::parseRectangle( RECTANGLE* me )
{
    /*
        (rectangle (start X Y) (end X Y) (stroke WIDTH) (fill FILL_TYPE))
    */

    T       tok;
    bool    sawStart = false;
    bool    sawEnd    = false;
    bool    sawStroke = false;
    bool    sawFill   = false;

    while( ( tok = NextTok() ) != T_RIGHT )
    {
        if( tok != T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_stroke:
            if( sawStroke )
                Duplicate( tok );
            sawStroke = true;
            parseStroke( &me->stroke );
            break;

        case T_fill:
            if( sawFill )
                Duplicate( tok );
            sawFill = true;
            tok = NeedSYMBOL();
            switch( tok )
            {
            case T_none:
            case T_filled:
            case T_transparent:
                me->fillType = tok;
                break;
            default:
                Expecting( "none|filled|transparent" );
            }
            NeedRIGHT();
            break;

        case T_start:
            if( sawStart )
                Duplicate( tok );
            sawStart = true;
            NeedNUMBER( "x" );
            me->start.x = internal( CurText() );
            NeedNUMBER( "y" );
            me->start.y = internal( CurText() );
            NeedRIGHT();
            break;

        case T_end:
            if( sawEnd )
                Duplicate( tok );
            sawEnd = true;
            NeedNUMBER( "x" );
            me->end.x = internal( CurText() );
            NeedNUMBER( "y" );
            me->end.y = internal( CurText() );
            NeedRIGHT();
            break;

        default:
            Expecting( "start|end|stroke|fill" );
        }
    }
}


void SWEET_PARSER::parseCircle( CIRCLE* me )
{
    /*
        (circle (center X Y)
            # Radius length is in units if defined or mils.
            (radius LENGTH)
            (stroke WIDTH)
            (fill FILL_TYPE)
        )
    */

    T       tok;
    bool    sawCenter = false;
    bool    sawRadius = false;
    bool    sawStroke = false;
    bool    sawFill   = false;

    while( ( tok = NextTok() ) != T_RIGHT )
    {
        if( tok != T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_stroke:
            if( sawStroke )
                Duplicate( tok );
            sawStroke = true;
            parseStroke( &me->stroke );
            break;

        case T_fill:
            if( sawFill )
                Duplicate( tok );
            sawFill = true;
            tok = NeedSYMBOL();
            switch( tok )
            {
            case T_none:
            case T_filled:
            case T_transparent:
                me->fillType = tok;
                break;
            default:
                Expecting( "none|filled|transparent" );
            }
            NeedRIGHT();
            break;

        case T_center:
            if( sawCenter )
                Duplicate( tok );
            sawCenter = true;
            NeedNUMBER( "center x" );
            me->center.x = internal( CurText() );
            NeedNUMBER( "center y" );
            me->center.y = internal( CurText() );
            NeedRIGHT();
            break;

        case T_radius:
            if( sawRadius )
                Duplicate( tok );
            sawRadius = true;
            NeedNUMBER( "radius" );
            me->radius = internal( CurText() );
            NeedRIGHT();
            break;

        default:
            Expecting( "center|radius|stroke|fill" );
        }
    }
}


void SWEET_PARSER::parseArc( ARC* me )
{
    /*
        (arc (pos X Y) (radius RADIUS) (start X Y) (end X Y)
            (stroke WIDTH)
            (fill FILL_TYPE)
        )
    */

    T       tok;
    bool    sawPos    = false;
    bool    sawStart  = false;
    bool    sawEnd    = false;
    bool    sawRadius = false;
    bool    sawStroke = false;
    bool    sawFill   = false;

    while( ( tok = NextTok() ) != T_RIGHT )
    {
        if( tok != T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_stroke:
            if( sawStroke )
                Duplicate( tok );
            sawStroke = true;
            parseStroke( &me->stroke );
            break;

        case T_fill:
            if( sawFill )
                Duplicate( tok );
            sawFill = true;
            tok = NeedSYMBOL();
            switch( tok )
            {
            case T_none:
            case T_filled:
            case T_transparent:
                me->fillType = tok;
                break;
            default:
                Expecting( "none|filled|transparent" );
            }
            NeedRIGHT();
            break;

        case T_pos:
            if( sawPos )
                Duplicate( tok );
            sawPos = true;
            NeedNUMBER( "pos x" );
            me->pos.x = internal( CurText() );
            NeedNUMBER( "pos y" );
            me->pos.y = internal( CurText() );
            NeedRIGHT();
            break;

        case T_radius:
            if( sawRadius )
                Duplicate( tok );
            sawRadius = true;
            NeedNUMBER( "radius" );
            me->radius = internal( CurText() );
            NeedRIGHT();
            break;

        case T_start:
            if( sawStart )
                Duplicate( tok );
            sawStart = true;
            NeedNUMBER( "start x" );
            me->start.x = internal( CurText() );
            NeedNUMBER( "start y" );
            me->start.y = internal( CurText() );
            NeedRIGHT();
            break;

        case T_end:
            if( sawEnd )
                Duplicate( tok );
            sawEnd = true;
            NeedNUMBER( "end x" );
            me->end.x = internal( CurText() );
            NeedNUMBER( "end y" );
            me->end.y = internal( CurText() );
            NeedRIGHT();
            break;

        default:
            Expecting( "center|radius|stroke|fill" );
        }
    }
}


void SWEET_PARSER::parseAt( POINT* pos, float* angle )
{
    T       tok;

    NeedNUMBER( "at x" );
    pos->x = internal( CurText() );

    NeedNUMBER( "at y" );
    pos->y = internal( CurText() );

    tok = NextTok();
    if( angle && tok == T_NUMBER )
    {
        *angle = strtod( CurText(), NULL );
        tok = NextTok();
    }
    if( tok != T_RIGHT )
        Expecting( T_RIGHT );
}


void SWEET_PARSER::parseText( GR_TEXT* me )
{
    /*
        (text "This is the text that gets drawn."
            (at X Y [ANGLE])

            # Valid horizontal justification values are center, right, and left.  Valid
            # vertical justification values are center, top, bottom.
            (justify HORIZONTAL_JUSTIFY VERTICAL_JUSTIFY)
            (font [FONT] (size HEIGHT WIDTH) [italic] [bold])
            (visible YES)
            (fill FILL_TYPE)
        )
    */

    T       tok;
    bool    sawAt   = false;
    bool    sawFill = false;
    bool    sawFont = false;
    bool    sawVis  = false;
    bool    sawJust = false;
    bool    sawText = false;

    while( ( tok = NextTok() ) != T_RIGHT )
    {
        if( tok == T_LEFT )
        {
            tok = NextTok();

            switch( tok )
            {
            case T_at:
                if( sawAt )
                    Duplicate( tok );
                parseAt( &me->pos, &me->angle );
                sawAt = true;
                break;

            case T_fill:
                if( sawFill )
                    Duplicate( tok );
                tok = NeedSYMBOL();
                switch( tok )
                {
                case T_none:
                case T_filled:
                case T_transparent:
                    me->fillType = tok;
                    break;
                default:
                    Expecting( "none|filled|transparent" );
                }
                NeedRIGHT();
                sawFill = true;
                break;

            case T_justify:
                if( sawJust )
                    Duplicate( tok );
                tok = NeedSYMBOL();
                switch( tok )
                {
                case T_center:
                case T_right:
                case T_left:
                    me->hjustify = tok;
                    break;
                default:
                    Expecting( "center|right|left" );
                }

                tok = NeedSYMBOL();
                switch( tok )
                {
                case T_center:
                case T_top:
                case T_bottom:
                    me->vjustify = tok;
                    break;
                default:
                    Expecting( "center|top|bottom" );
                }
                NeedRIGHT();
                sawJust = true;
                break;

            case T_visible:
                if( sawVis )
                    Duplicate( tok );
                parseBool( &me->isVisible );
                NeedRIGHT();
                sawVis = true;
                break;

            case T_font:
                if( sawFont )
                    Duplicate( tok );
                sawFont = true;
                parseFont( &me->font );
                break;

            default:
                Expecting( "at|justify|font|visible|fill" );
            }
        }
        else
        {
            if( !IsSymbol( tok ) && tok != T_NUMBER )
                Expecting( T_STRING );

            if( sawText )
                Duplicate( tok );
            sawText = true;

            me->text = wxString::FromUTF8( CurText() );
        }
    }
}

