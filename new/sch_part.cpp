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

#include <wx/wx.h>          // _()

#include <sch_part.h>
#include <sch_sweet_parser.h>
#include <sch_lpid.h>
#include <sch_lib_table.h>
#include <macros.h>



/**
 * Function formatAt
 * outputs a formatted "(at X Y [ANGLE])" s-expression
 */
 static void formatAt( OUTPUTFORMATTER* out, const SCH::POINT& aPos, ANGLE aAngle, int indent=0 )
{
    // if( aPos.x || aPos.y || aAngle )
    {
        out->Print( indent, aAngle!=0.0 ? "(at %.6g %.6g %.6g)" : "(at %.6g %.6g)",
            InternalToLogical( aPos.x ), InternalToLogical( aPos.y ),
            double( aAngle ) );
    }
}

static void formatStroke( OUTPUTFORMATTER* out, STROKE aStroke, int indent=0 )
{
    if( aStroke == STROKE_DEFAULT )
        out->Print( indent, "(stroke %.6g)", InternalToWidth( aStroke ) );
}


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

    for( int i=REFERENCE;  i<END;  ++i )
        mandatory[i] = 0;
}


void PART::clear()
{
    if( extends )
    {
        delete extends;
        extends = 0;
    }

    // clear the mandatory fields
    for( int ndx = REFERENCE;  ndx < END;  ++ndx )
    {
        delete mandatory[ndx];
        mandatory[ndx] = 0;
    }

    // delete properties I own, since their container will not destroy them:
    for( PROPERTIES::iterator it = properties.begin();  it != properties.end();  ++it )
        delete *it;
    properties.clear();

    // delete graphics I own, since their container will not destroy them:
    for( GRAPHICS::iterator it = graphics.begin();  it != graphics.end();  ++it )
        delete *it;
    graphics.clear();

    // delete PINs I own, since their container will not destroy them.
    for( PINS::iterator it = pins.begin();  it != pins.end();  ++it )
        delete *it;
    pins.clear();

    alternates.clear();

    keywords.clear();

    pin_merges.clear();

    contains = 0;
}


PROPERTY* PART::FieldLookup( PROP_ID aPropertyId )
{
    wxASSERT( unsigned(aPropertyId) < unsigned(END) );

    PROPERTY* p = mandatory[aPropertyId];

    if( !p )
    {
        switch( aPropertyId )
        {
        case REFERENCE:
            p = new PROPERTY( this, wxT( "reference" ) );
            p->text = wxT( "U?" );
            break;

        case VALUE:
            p = new PROPERTY( this, wxT( "value" ) );
            break;

        case FOOTPRINT:
            p = new PROPERTY(  this, wxT( "footprint" ) );
            break;

        case DATASHEET:
            p = new PROPERTY( this, wxT( "datasheet" ) );
            break;

        case MODEL:
            p = new PROPERTY( this, wxT( "model" ) );
            break;

        default:
            ;
        }

        mandatory[aPropertyId] = p;
    }

    return p;
}


PROPERTY& PROPERTY::operator = ( const PROPERTY& r )
{
    *(BASE_GRAPHIC*) this = (BASE_GRAPHIC&) r;

    name  = r.name;
    text  = r.text;

    delete effects;

    if( r.effects )
        effects = new TEXT_EFFECTS( *r.effects );
    else
        effects = 0;

    return *this;
}


PINS::iterator PART::pinFindByPad( const wxString& aPad )
{
    PINS::iterator it;

    for( it = pins.begin();  it != pins.end();  ++it )
    {
        if( (*it)->pad.text == aPad )
            break;
    }

    return it;
}


void PART::PinsFindBySignal( PIN_LIST* aResults, const wxString& aSignal )
{
    for( PINS::const_iterator it = pins.begin();  it != pins.end();  ++it )
    {
        if( (*it)->signal.text == aSignal )
        {
            aResults->push_back( *it );
        }
    }
}


bool PART::PinDelete( const wxString& aPad )
{
    PINS::iterator it = pinFindByPad( aPad );
    if( it != pins.end() )
    {
        delete *it;
        pins.erase( it );
        return true;
    }

    // there is only one reason this can fail: not found:
    return false;
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


void PART::inherit( const PART& r )
{
    // Inherit can be called at any time, even from an interactive text
    // editor, so cannot assume 'this' object is new.  Clear it.
    clear();

    // copy anything inherited, such as drawables, properties, pins, etc. here
    contains = r.contains;

    base     = &r;

    anchor   = r.anchor;

    for( int i=REFERENCE;  i<END;  ++i )
    {
        if( r.mandatory[i] )
            mandatory[i] = (PROPERTY*) r.mandatory[i]->Clone( this );
    }

    for( PROPERTIES::const_iterator it = r.properties.begin();  it != r.properties.end();  ++it )
        properties.push_back( (PROPERTY*) (*it)->Clone( this ) );

    for( GRAPHICS::const_iterator it = r.graphics.begin();  it != r.graphics.end();  ++it )
        graphics.push_back( (*it)->Clone( this ) );

    for( PINS::const_iterator it = r.pins.begin();  it != r.pins.end();  ++it )
        pins.push_back( (PIN*) (*it)->Clone( this ) );

    /* not sure about this concept yet:
    for( PART_REFS::const_iterator it = r.alternates.begin();  it != r.alternates.end();  ++it )
        alternates.push_back( *it );
    */

    for( KEYWORDS::const_iterator it = r.keywords.begin();  it != r.keywords.end();  ++it )
        keywords.insert( *it );

    for( MERGE_SETS::const_iterator it = r.pin_merges.begin();  it != r.pin_merges.end();  ++it )
    {
        pin_merges[ *it->first ] = * new MERGE_SET( *it->second );
    }
}


PART& PART::operator=( const PART& r )
{
    // maintain in concert with inherit(), which is a partial assignment operator.
    inherit( r );

    owner          = r.owner;
    partNameAndRev = r.partNameAndRev;
    body           = r.body;
    base           = r.base;

    setExtends( r.extends ? new LPID( *r.extends ) : 0 );

    return *this;
}


void PART::Parse( SWEET_PARSER* aParser , LIB_TABLE* aTable ) throw( IO_ERROR, PARSE_ERROR )
{
    aParser->Parse( this, aTable );
}


bool PART::PropDelete( const wxString& aPropertyName )
{
    PROPERTIES::iterator it = propertyFind( aPropertyName );
    if( it != properties.end() )
    {
        delete *it;
        properties.erase( it );
        return true;
    }

    return false;
}


PROPERTIES::iterator PART::propertyFind( const wxString& aPropertyName )
{
    PROPERTIES::iterator it;
    for( it = properties.begin();  it!=properties.end();  ++it )
        if( (*it)->name == aPropertyName )
            break;
    return it;
}


void PART::Format( OUTPUTFORMATTER* out, int indent, int ctl ) const
{
    out->Print( indent, "(part %s", partNameAndRev.c_str() );

    if( extends )
        out->Print( 0, " inherits %s", extends->Format().c_str() );

    out->Print( 0, "\n" );

    for( int i = REFERENCE;  i < END;  ++i )
    {
        PROPERTY* prop = Field( PROP_ID( i ) );
        if( prop )
            prop->Format( out, indent+1, ctl );
    }

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

    if( keywords.size() )
    {
        out->Print( indent+1, "(keywords" );
        for( KEYWORDS::iterator it = keywords.begin();  it != keywords.end();  ++it )
            out->Print( 0, " %s", out->Quotew( *it ).c_str() );
        out->Print( 0, ")\n" );
    }

    for( GRAPHICS::const_iterator it = graphics.begin();  it != graphics.end();  ++it )
    {
        (*it)->Format( out, indent+1, ctl );
    }

    for( PINS::const_iterator it = pins.begin();  it != pins.end();  ++it )
    {
        (*it)->Format( out, indent+1, ctl );
    }

    if( alternates.size() )
    {
        out->Print( indent+1, "(alternates" );
        for( PART_REFS::const_iterator it = alternates.begin();  it!=alternates.end();  ++it )
            out->Print( 0, " %s", out->Quotes( it->Format() ).c_str() );
        out->Print( 0, ")\n" );
    }

    for( MERGE_SETS::const_iterator mit = pin_merges.begin();  mit != pin_merges.end();  ++mit )
    {
        out->Print( indent+1, "(pin_merge %s (pads", out->Quotew( mit->first ).c_str() );

        const MERGE_SET& mset = *mit->second;
        for( MERGE_SET::const_iterator pit = mset.begin();  pit != mset.end();  ++pit )
        {
            out->Print( 0, " %s", out->Quotew( *pit ).c_str() );
        }
        out->Print( 0, "))\n" );
    }

    out->Print( indent, ")\n" );
}


//-----< PART objects  >------------------------------------------------------


void PROPERTY::Format( OUTPUTFORMATTER* out, int indent, int ctl ) const
{
    wxASSERT( owner );      // all PROPERTYs should have an owner.

    int i;
    for( i = PART::REFERENCE;  i < PART::END;  ++i )
    {
        if( owner->Field( PART::PROP_ID(i) ) == this )
            break;
    }

    if( i < PART::END )   // is a field not a property
        out->Print( indent, "(%s", TO_UTF8( name ) );
    else
        out->Print( indent, "(property %s", out->Quotew( name ).c_str() );

    if( effects )
    {
        out->Print( 0, " %s\n", out->Quotew( text ).c_str() );
        effects->Format( out, indent+1, ctl | CTL_OMIT_NL );
        out->Print( 0, ")\n" );
    }
    else
    {
        out->Print( 0, " %s)\n", out->Quotew( text ).c_str() );
    }
}


TEXT_EFFECTS* PROPERTY::EffectsLookup()
{
    if( !effects )
    {
        effects = new TEXT_EFFECTS();
    }

    return effects;
}


void TEXT_EFFECTS::Format( OUTPUTFORMATTER* out, int indent, int ctl ) const
{
    if( propName.IsEmpty() )
        out->Print( indent, "(effects " );
    else
        out->Print( indent, "(effects %s ",  out->Quotew( propName ).c_str() );

    formatAt( out, pos, angle );

    font.Format( out, 0, ctl | CTL_OMIT_NL );

    out->Print( 0, "(visible %s))%s",
            isVisible ? "yes" : "no",
            ctl & CTL_OMIT_NL ? "" : "\n" );
}


void FONT::Format( OUTPUTFORMATTER* out, int indent, int ctl ) const
{
    if( italic || bold || !name.IsEmpty() || size.height != FONTZ_DEFAULT || size.width != FONTZ_DEFAULT )
    {
        if( name.IsEmpty() )
            out->Print( indent, "(font " );
        else
            out->Print( indent, "(font %s ", out->Quotew( name ).c_str() );

        out->Print( 0, "(size %.6g %.6g)",
                InternalToFontz( size.height ),
                InternalToFontz( size.width ) );

        if( italic )
            out->Print( 0, " italic" );

        if( bold )
            out->Print( 0, " bold" );

        out->Print( 0, ")%s", (ctl & CTL_OMIT_NL) ? "" : "\n" );
    }
}


void PIN::Format( OUTPUTFORMATTER* out, int indent, int ctl ) const
{
    bool    hasSignal = !signal.text.IsEmpty();
    bool    hasPad    = !pad.text.IsEmpty();

    out->Print( indent, "(pin" );

    if( connectionType != PIN_CONN_DEFAULT )
        out->Print( 0, " %s", ShowType() );

    if( shape != PIN_SHAPE_DEFAULT )
        out->Print( 0, " %s", ShowShape() );

    out->Print( 0, " " );

    if( pos.x || pos.y || angle )
        formatAt( out, pos, angle );

    if( length != PIN_LEN_DEFAULT )
        out->Print( 0, "(length %.6g)", InternalToLogical( length ) );

    if( !isVisible )
        out->Print( 0, "(visible %s)", isVisible ? "yes" : "no" );

    if( hasSignal )
        signal.Format(  out, "signal",  0, CTL_OMIT_NL );

    if( hasPad )
        pad.Format( out, "pad", 0, CTL_OMIT_NL );

    out->Print( 0, ")\n" );
}


PIN::~PIN()
{
}


void PINTEXT::Format( OUTPUTFORMATTER* out, const char* aElement, int indent, int ctl ) const
{
    out->Print( indent, "(%s %s", aElement, out->Quotew( text ).c_str() );

    font.Format( out, 0, CTL_OMIT_NL );

    if( !isVisible )
        out->Print( 0, " (visible %s)", isVisible ? "yes" : "no" );

    out->Print( 0, ")%s", ctl & CTL_OMIT_NL ? "" : "\n" );
}


void POLY_LINE::Format( OUTPUTFORMATTER* out, int indent, int ctl ) const
{
    out->Print( indent, "(%s ",  pts.size() == 2 ? "line" : "polyline" );
    formatContents( out, indent, ctl );
}


void POLY_LINE::formatContents(  OUTPUTFORMATTER* out, int indent, int ctl ) const
{
    formatStroke( out, stroke );

    if( fillType != PR::T_none )
        out->Print( 0, "(fill %s)", ShowFill( fillType ) );

    out->Print( 0, "\n" );

    if( pts.size() )
    {
        const int   maxLength = 75;
        int         len = 10;

        len += out->Print( indent+1, "(pts " );

        for( POINTS::const_iterator it = pts.begin();  it != pts.end();  ++it )
        {
            if( len > maxLength )
            {
                len = 10;
                out->Print( 0, "\n" );
                out->Print( indent+2, "(xy %.6g %.6g)",
                    InternalToLogical( it->x ), InternalToLogical( it->y ) );
            }
            else
                out->Print( 0, "(xy %.6g %.6g)",
                    InternalToLogical( it->x ), InternalToLogical( it->y ) );
        }

        out->Print( 0, ")" );
    }

    out->Print( 0, ")\n" );
}


void BEZIER::Format( OUTPUTFORMATTER* out, int indent, int ctl ) const
{
    out->Print( indent, "(bezier " );
    formatContents( out, indent, ctl );     // inherited from POLY_LINE
}

void RECTANGLE::Format( OUTPUTFORMATTER* out, int indent, int ctl ) const
{
    // (rectangle (start X Y) (end X Y) [(stroke WIDTH)] (fill FILL_TYPE))

    out->Print( indent, "(rectangle (start %.6g %.6g)(end %.6g %.6g)",
        InternalToLogical( start.x ), InternalToLogical( start.y ),
        InternalToLogical( end.x ), InternalToLogical( end.y )
        );

    formatStroke( out, stroke );

    if( fillType != PR::T_none )
        out->Print( 0, "(fill %s)", ShowFill( fillType ) );

    out->Print( 0, ")\n" );
}


void CIRCLE::Format( OUTPUTFORMATTER* out, int indent, int ctl ) const
{
    /*
        (circle (center X Y)(radius LENGTH) [(stroke WIDTH)] (fill FILL_TYPE))
    */

    out->Print( indent, "(circle (center %.6g %.6g)(radius %.6g)",
        InternalToLogical( center.x ), InternalToLogical( center.y ),
        InternalToLogical( radius) );

    formatStroke( out, stroke );

    if( fillType != PR::T_none )
        out->Print( 0, "(fill %s)", ShowFill( fillType ) );

    out->Print( 0, ")\n" );
}


void ARC::Format( OUTPUTFORMATTER* out, int indent, int ctl ) const
{
    /*
        (arc (pos X Y)(radius RADIUS)(start X Y)(end X Y) [(stroke WIDTH)] (fill FILL_TYPE))
    */

    out->Print( indent, "(arc (pos %.6g %.6g)(radius %.6g)(start %.6g %.6g)(end %.6g %.6g)",
        InternalToLogical( pos.x ), InternalToLogical( pos.y ),
        InternalToLogical( radius),
        InternalToLogical( start.x ), InternalToLogical( start.y ),
        InternalToLogical( end.x ),   InternalToLogical( end.y )
        );

    formatStroke( out, stroke );

    if( fillType != PR::T_none )
        out->Print( 0, "(fill %s)", ShowFill( fillType ) );

    out->Print( 0, ")\n" );
}


void GR_TEXT::Format( OUTPUTFORMATTER* out, int indent, int ctl ) const
{
    /*
        (text "This is the text that gets drawn."
            (at X Y [ANGLE])(justify HORIZONTAL_JUSTIFY VERTICAL_JUSTIFY)(visible YES)(fill FILL_TYPE)
            (font [FONT] (size HEIGHT WIDTH) [italic] [bold])
        )
    */

    out->Print( indent, "(text %s\n", out->Quotew( text ).c_str() );

    formatAt( out, pos, angle, indent+1 );

    if( hjustify != PR::T_left || vjustify != PR::T_bottom )
        out->Print( 0, "(justify %s %s)",
            ShowJustify( hjustify ), ShowJustify( vjustify ) );

    if( !isVisible )
        out->Print( 0, "(visible %s)", isVisible ? "yes" : "no" );

    if( fillType != PR::T_filled )
        out->Print( 0, "(fill %s)", ShowFill( fillType ) );

    font.Format( out, 0, CTL_OMIT_NL );
    out->Print( 0, ")\n" );
}

