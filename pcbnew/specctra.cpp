
/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2007-2008 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2007 Kicad Developers, see change_log.txt for contributors.
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

 
/*  This source file implements export and import capabilities to the 
    specctra dsn file format.  The grammar for that file format is documented
    fairly well.  There are classes for each major type of descriptor in the
    spec.
    
    Since there are so many classes in here, it may be helpful to generate 
    the Doxygen directory:
    
    $ cd <kicadSourceRoot>
    $ doxygen
    
    Then you can view the html documentation in the <kicadSourceRoot>/doxygen
    directory.  The main class in this file is SPECCTRA_DB and its main
    functions are LoadPCB(), LoadSESSION(), and ExportPCB().
    
    Wide use is made of boost::ptr_vector<> and std::vector<> template classes.
    If the contained object is small, then std::vector tends to be used.
    If the contained object is large, variable size, or would require writing
    an assignment operator() or copy constructore, then boost::ptr_vector
    cannot be beat.
*/    


#include <cstdarg>
#include <cstdio>

#include "specctra.h"

#include <wx/ffile.h>


//#define STANDALONE        // define "stand alone, i.e. unit testing"


#if defined(STANDALONE)
 #define EDA_BASE           // build_version.h behavior
 #undef  COMMON_GLOBL
 #define COMMON_GLOBL       // build_version.h behavior
#endif
#include "build_version.h"


namespace DSN {

#define NESTWIDTH           2   ///< how many spaces per nestLevel    



//-----<SPECCTRA_DB>-------------------------------------------------

void SPECCTRA_DB::ThrowIOError( const wxChar* fmt, ... ) throw( IOError )
{
    wxString    errText;
    va_list     args;

    va_start( args, fmt );
    errText.PrintfV( fmt, args );
    va_end( args );
    
    throw IOError( errText );
}


void SPECCTRA_DB::expecting( DSN_T aTok ) throw( IOError )
{
    wxString    errText( _("Expecting") );
    errText << wxT(" ") << LEXER::GetTokenString( aTok );
    lexer->ThrowIOError( errText, lexer->CurOffset() ); 
}

void SPECCTRA_DB::expecting( const char* text ) throw( IOError )
{
    wxString    errText( _("Expecting") );
    errText << wxT(" '") << CONV_FROM_UTF8(text) << wxT("'");
    lexer->ThrowIOError( errText, lexer->CurOffset() ); 
}

void SPECCTRA_DB::unexpected( DSN_T aTok ) throw( IOError )
{
    wxString    errText( _("Unexpected") );
    errText << wxT(" ") << LEXER::GetTokenString( aTok );
    lexer->ThrowIOError( errText, lexer->CurOffset() ); 
}

void SPECCTRA_DB::unexpected( const char* text ) throw( IOError )
{
    wxString    errText( _("Unexpected") );
    errText << wxT(" '") << CONV_FROM_UTF8(text) << wxT("'");
    lexer->ThrowIOError( errText, lexer->CurOffset() ); 
}


DSN_T SPECCTRA_DB::nextTok()
{
    DSN_T ret = lexer->NextTok();
    return ret;
}


bool SPECCTRA_DB::isSymbol( DSN_T aTok )
{
    // if aTok is >= 0, then it might be a coincidental match to a keyword.
    return aTok==T_SYMBOL || aTok==T_STRING || aTok>=0;
}
    

void SPECCTRA_DB::needLEFT() throw( IOError )
{
    DSN_T tok = nextTok();
    if( tok != T_LEFT )
        expecting( T_LEFT );
}

void SPECCTRA_DB::needRIGHT() throw( IOError )
{
    DSN_T tok = nextTok();
    if( tok != T_RIGHT )
        expecting( T_RIGHT );
}

void SPECCTRA_DB::needSYMBOL() throw( IOError )
{
    DSN_T tok = nextTok();
    if( !isSymbol( tok ) )
        expecting( T_SYMBOL );
}


void SPECCTRA_DB::readCOMPnPIN( std::string* component_id, std::string* pin_id ) throw( IOError )
{
    DSN_T tok;
    
    static const char pin_def[] = "<pin_reference>::=<component_id>-<pin_id>"; 
    
    if( !isSymbol( lexer->CurTok() ) )
        expecting( pin_def );

    // case for:  A12-14, i.e. no wrapping quotes.  This should be a single
    // token, so split it.    
    if( lexer->CurTok() != T_STRING )
    {
        const char* toktext = lexer->CurText();
        const char* dash    = strchr( toktext, '-' );
        
        if( !dash )
            expecting( pin_def );
        
        while( toktext != dash )
            *component_id += *toktext++;

        ++toktext;  // skip the dash

        while( *toktext )
            *pin_id += *toktext++;
    }
    
    // quoted string:  "U12"-"14" or "U12"-14,  3 tokens in either case
    else
    {
        *component_id = lexer->CurText();

        tok = nextTok();
        if( tok!=T_DASH )
            expecting( pin_def );
        
        nextTok();          // accept anything after the dash.
        *pin_id = lexer->CurText();
    }
}


void SPECCTRA_DB::readTIME( time_t* time_stamp ) throw( IOError )
{
    DSN_T tok;
    
    struct tm   mytime;
    
    static const char time_toks[] = "<month> <day> <hour> : <minute> : <second> <year>"; 

    static const char* months[] = {  // index 0 = Jan
        "Jan", "Feb", "Mar", "Apr", "May", "Jun", 
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", NULL
    };
    
    needSYMBOL();       // month
    
    const char* ptok = lexer->CurText();

    mytime.tm_mon = 0;      // remains of we don't find a month match.
    for( int m=0;  months[m];  ++m )
    {
        if( !stricmp( months[m], ptok ) )
        {
            mytime.tm_mon = m;
            break;
        }
    }
    
    tok = nextTok();    // day
    if( tok != T_NUMBER )
        expecting( time_toks );
    mytime.tm_mday = atoi( lexer->CurText() );
    
    tok = nextTok();    // hour
    if( tok != T_NUMBER )
        expecting( time_toks );
    mytime.tm_hour = atoi( lexer->CurText() );

    // : colon    
    needSYMBOL();
    if( *lexer->CurText() != ':' || strlen( lexer->CurText() )!=1 )
        expecting( time_toks );

    tok = nextTok();    // minute
    if( tok != T_NUMBER )
        expecting( time_toks );
    mytime.tm_min = atoi( lexer->CurText() );
    
    // : colon    
    needSYMBOL();
    if( *lexer->CurText() != ':' || strlen( lexer->CurText() )!=1 )
        expecting( time_toks );

    tok = nextTok();    // second
    if( tok != T_NUMBER )
        expecting( time_toks );
    mytime.tm_sec = atoi( lexer->CurText() );
    
    tok = nextTok();    // year
    if( tok != T_NUMBER )
        expecting( time_toks );
    mytime.tm_year = atoi( lexer->CurText() ) - 1900;
    
    *time_stamp = mktime( &mytime ); 
}


void SPECCTRA_DB::LoadPCB( const wxString& filename ) throw( IOError )
{
    wxFFile     file;
    
    FILE*       fp = wxFopen( filename, wxT("r") );
    
    if( !fp )
    {
        ThrowIOError( _("Unable to open file \"%s\""), filename.GetData() );  
    }

    file.Attach( fp );      // "exception safe" way to close the file.
    
    delete lexer;  
    lexer = 0;
    
    lexer = new LEXER( file.fp(), filename );

    if( nextTok() != T_LEFT )
        expecting( T_LEFT );
    
    if( nextTok() != T_pcb )
        expecting( T_pcb );

    SetPCB( new PCB() );
    
    doPCB( pcb );
}


void SPECCTRA_DB::LoadSESSION( const wxString& filename ) throw( IOError )
{
    wxFFile     file;
    
    FILE*       fp = wxFopen( filename, wxT("r") );
    
    if( !fp )
    {
        ThrowIOError( _("Unable to open file \"%s\""), filename.GetData() );  
    }

    file.Attach( fp );      // "exception safe" way to close the file.
    
    delete lexer;  
    lexer = 0;
    
    lexer = new LEXER( file.fp(), filename );

    if( nextTok() != T_LEFT )
        expecting( T_LEFT );
    
    if( nextTok() != T_session )
        expecting( T_session );

    SetSESSION( new SESSION() );
    
    doSESSION( session );
}


void SPECCTRA_DB::doPCB( PCB* growth ) throw( IOError )
{
    DSN_T tok;

    /*  <design_descriptor >::=
        (pcb <pcb_id >
          [<parser_descriptor> ]
          [<capacitance_resolution_descriptor> ]
          [<conductance_resolution_descriptor> ]
          [<current_resolution_descriptor> ]
          [<inductance_resolution_descriptor> ]
          [<resistance_resolution_descriptor> ]
          [<resolution_descriptor> ]
          [<time_resolution_descriptor> ]
          [<voltage_resolution_descriptor> ]
          [<unit_descriptor> ]
          [<structure_descriptor> | <file_descriptor> ]
          [<placement_descriptor> | <file_descriptor> ]
          [<library_descriptor> | <file_descriptor> ]
          [<floor_plan_descriptor> | <file_descriptor> ]
          [<part_library_descriptor> | <file_descriptor> ]
          [<network_descriptor> | <file_descriptor> ]
          [<wiring_descriptor> ]
          [<color_descriptor> ]
        )
    */
    
    needSYMBOL();
    growth->pcbname = lexer->CurText();    
    
    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_parser:
            if( growth->parser )
                unexpected( tok );
            growth->parser = new PARSER( growth );
            doPARSER( growth->parser );
            break;
            
        case T_unit:
            if( growth->unit )
                unexpected( tok );
            growth->unit = new UNIT_RES( growth, tok );
            doUNIT( growth->unit );
            break;
            
        case T_resolution:
            if( growth->resolution )
                unexpected( tok );
            growth->resolution = new UNIT_RES( growth, tok );
            doRESOLUTION( growth->resolution );
            break;
            
        case T_structure:
            if( growth->structure )
                unexpected( tok );
            growth->structure = new STRUCTURE( growth );
            doSTRUCTURE( growth->structure );
            break;

        case T_placement:
            if( growth->placement )
                unexpected( tok );
            growth->placement = new PLACEMENT( growth );
            doPLACEMENT( growth->placement );
            break;
            
        case T_library:
            if( growth->library )
                unexpected( tok );
            growth->library = new LIBRARY( growth );
            doLIBRARY( growth->library );
            break;
            
        case T_network:
            if( growth->network )
                unexpected( tok );
            growth->network = new NETWORK( growth );
            doNETWORK( growth->network );
            break;

        case T_wiring:
            if( growth->wiring )
                unexpected( tok );
            growth->wiring = new WIRING( growth );
            doWIRING( growth->wiring );
            break;
            
        default:
            unexpected( lexer->CurText() );
        }
    }
    
    tok = nextTok();
    if( tok != T_EOF )
        expecting( T_EOF );
}


void SPECCTRA_DB::doPARSER( PARSER* growth ) throw( IOError )
{
    DSN_T   tok;
    
    /*  <parser_descriptor >::=
        (parser
          [(string_quote <quote_char >)]
          (space_in_quoted_tokens [on | off])
          [(host_cad <id >)]
          [(host_version <id >)]
          [{(constant <id > <id >)}]
          [(write_resolution] {<character> <positive_integer >})]
          [(routes_include {[testpoint | guides |
             image_conductor]})]
          [(wires_include testpoint)]
          [(case_sensitive [on | off])]
          [(via_rotate_first [on | off])]
        )
    */
    
    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_string_quote:
            tok = nextTok();
            if( tok != T_QUOTE_DEF )
                expecting( T_QUOTE_DEF );
            lexer->SetStringDelimiter( (unsigned char) *lexer->CurText() );
            growth->string_quote = *lexer->CurText();
            quote_char = lexer->CurText(); 
            needRIGHT();        
            break;
            
        case T_space_in_quoted_tokens:
            tok = nextTok();
            if( tok!=T_on && tok!=T_off )
                expecting( "on|off" );
            lexer->SetSpaceInQuotedTokens( tok==T_on );
            growth->space_in_quoted_tokens = (tok==T_on);
            needRIGHT();        
            break;
            
        case T_host_cad:
            needSYMBOL();
            growth->host_cad = lexer->CurText();
            needRIGHT();        
            break;
            
        case T_host_version:
            needSYMBOL();
            growth->host_version = lexer->CurText();
            needRIGHT();        
            break;

        case T_constant:
            needSYMBOL();
            growth->const_id1 = lexer->CurText();
            needSYMBOL();
            growth->const_id2 = lexer->CurText();
            needRIGHT();        
            break;

        case T_write_resolution:   // [(writee_resolution {<character> <positive_integer >})]
            while( (tok = nextTok()) != T_RIGHT )
            {
                if( tok!=T_SYMBOL )
                    expecting( T_SYMBOL );
                tok = nextTok();
                if( tok!=T_NUMBER )
                    expecting( T_NUMBER );
                // @todo
            }
            break;

        case T_routes_include:  // [(routes_include {[testpoint | guides | image_conductor]})]
            while( (tok = nextTok()) != T_RIGHT )
            {
                switch( tok )
                {
                case T_testpoint:
                    growth->routes_include_testpoint = true;
                    break;
                case T_guide:
                    growth->routes_include_guides = true;
                    break;
                case T_image_conductor:
                    growth->routes_include_image_conductor = true;
                    break;
                default:
                    expecting( "testpoint|guides|image_conductor" );
                }
            }
            break;

        case T_wires_include:   // [(wires_include testpoint)]
            tok = nextTok();
            if( tok != T_testpoint )
                expecting( T_testpoint );
            growth->routes_include_testpoint = true;
            needRIGHT();        
            break;
            
        case T_case_sensitive:
            tok = nextTok();
            if( tok!=T_on && tok!=T_off )
                expecting( "on|off" );
            growth->case_sensitive = (tok==T_on);
            needRIGHT();        
            break;

        case T_via_rotate_first:    // [(via_rotate_first [on | off])]
            tok = nextTok();
            if( tok!=T_on && tok!=T_off )
                expecting( "on|off" );
            growth->via_rotate_first = (tok==T_on);
            needRIGHT();        
            break;

        case T_generated_by_freeroute:
            growth->generated_by_freeroute = true;
            needRIGHT();        
            break;
            
        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doRESOLUTION( UNIT_RES* growth ) throw(IOError)
{
    DSN_T   tok = nextTok();

    switch( tok )
    {
    case T_inch:
    case T_mil:
    case T_cm:
    case T_mm:
    case T_um:
        growth->units = tok;
        break;
    default:
        expecting( "inch|mil|cm|mm|um" );
    }
    
    tok = nextTok();
    if( tok != T_NUMBER )
        expecting( T_NUMBER );

    growth->value = atoi( lexer->CurText() );
    
    needRIGHT();
}


void SPECCTRA_DB::doUNIT( UNIT_RES* growth ) throw(IOError)
{
    DSN_T   tok = nextTok();

    switch( tok )
    {
    case T_inch:
    case T_mil:
    case T_cm:
    case T_mm:
    case T_um:
        growth->units = tok;
        break;
    default:
        expecting( "inch|mil|cm|mm|um" );
    }
    
    needRIGHT();
}


void SPECCTRA_DB::doLAYER_PAIR( LAYER_PAIR* growth ) throw( IOError )
{
    needSYMBOL();
    growth->layer_id0 = lexer->CurText();

    needSYMBOL();
    growth->layer_id1 = lexer->CurText();
    
    if( nextTok() != T_NUMBER )
        expecting( T_NUMBER );
    growth->layer_weight = strtod( lexer->CurText(), 0 );

    needRIGHT();
}


void SPECCTRA_DB::doLAYER_NOISE_WEIGHT( LAYER_NOISE_WEIGHT* growth ) throw( IOError )
{
    DSN_T   tok;
    
    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        if( nextTok() != T_layer_pair )
            expecting( T_layer_pair );
        
        LAYER_PAIR* layer_pair = new LAYER_PAIR( growth );
        growth->layer_pairs.push_back( layer_pair );
        doLAYER_PAIR( layer_pair );
    }
}


void SPECCTRA_DB::doSTRUCTURE( STRUCTURE* growth ) throw(IOError)
{
    DSN_T   tok;
    
    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_unit:
            if( growth->unit )
                unexpected( tok );
            growth->unit = new UNIT_RES( growth, tok );
            doUNIT( growth->unit );
            break;
            
        case T_resolution:
            if( growth->unit )
                unexpected( tok );
            growth->unit = new UNIT_RES( growth, tok );
            doRESOLUTION( growth->unit );
            break;

        case T_layer_noise_weight:
            growth->layer_noise_weight = new LAYER_NOISE_WEIGHT( growth );
            doLAYER_NOISE_WEIGHT( growth->layer_noise_weight );
            break;            
            
        case T_place_boundary:
L_place:            
            if( growth->place_boundary )
                unexpected( tok );
            growth->place_boundary = new BOUNDARY( growth, T_place_boundary );
            doBOUNDARY( growth->place_boundary );
            break;
            
        case T_boundary:
            if( growth->boundary )
            {
                if( growth->place_boundary )
                    unexpected( tok );
                goto L_place;
            }
            growth->boundary = new BOUNDARY( growth );
            doBOUNDARY( growth->boundary );
            break;

        case T_plane:
            COPPER_PLANE* plane;
            plane = new COPPER_PLANE( growth );
            growth->planes.push_back( plane );
            doKEEPOUT( plane );
            break;

        case T_region:
            REGION* region;
            region = new REGION( growth );
            growth->regions.push_back( region );
            doREGION( region );
            break;
            
        case T_snap_angle:
            STRINGPROP* stringprop;
            stringprop = new STRINGPROP( growth, T_snap_angle ); 
            growth->Append( stringprop );
            doSTRINGPROP( stringprop );
            break;

        case T_via:
            growth->via = new VIA( growth );
            doVIA( growth->via );
            break;
            
        case T_control:
            growth->control = new CONTROL( growth );
            doCONTROL( growth->control );
            break;

        case T_layer:
            LAYER* layer;
            layer = new LAYER( growth );
            growth->layers.push_back( layer );
            doLAYER( layer );
            break;

        case T_rule:
            growth->rules = new RULE( growth, T_rule );
            doRULE( growth->rules );
            break;

        case T_place_rule:
            growth->place_rules = new RULE( growth, T_place_rule );
            doRULE( growth->place_rules );
            break;            
            
        case T_keepout:
        case T_place_keepout:
        case T_via_keepout:
        case T_wire_keepout:
        case T_bend_keepout:
        case T_elongate_keepout:
            KEEPOUT* keepout;
            keepout = new KEEPOUT( growth, tok );
            growth->keepouts.push_back( keepout );
            doKEEPOUT( keepout );
            break;

        case T_grid:
            GRID* grid;
            grid = new GRID( growth );
            growth->grids.push_back( grid );
            doGRID( grid );
            break;
            
        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doKEEPOUT( KEEPOUT* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();
    
    if( tok==T_SYMBOL || tok==T_STRING )
    {
        growth->name = lexer->CurText();
        tok = nextTok();
    }
    
    if( tok!=T_LEFT )    
        expecting( T_LEFT );

    while( tok != T_RIGHT )
    {
        if( tok!=T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        
        switch( tok )
        {
        case T_sequence_number:
            if( nextTok() != T_NUMBER )
                expecting( T_NUMBER );
            growth->sequence_number = atoi( lexer->CurText() );
            needRIGHT();
            break;
            
        case T_rule:
            if( growth->rules )
                unexpected( tok );
            growth->rules = new RULE( growth, T_rule );
            doRULE( growth->rules );
            break;
            
        case T_place_rule:
            if( growth->place_rules )
                unexpected( tok );
            growth->place_rules = new RULE( growth, T_place_rule );
            doRULE( growth->place_rules );
            break;
            
        case T_rect:
            growth->rectangle = new RECTANGLE( growth );
            doRECTANGLE( growth->rectangle );
            break;
            
        case T_circle:
            growth->circle = new CIRCLE( growth );
            doCIRCLE( growth->circle );
            break;
            
        case T_path:
        case T_polygon:
            growth->path = new PATH( growth, tok );
            doPATH( growth->path );
            break;
            
        case T_qarc:
            growth->qarc = new QARC( growth );
            doQARC( growth->qarc );
            break;
            
        case T_window:
            WINDOW* window;
            window = new WINDOW( growth );
            growth->windows.push_back( window );
            doWINDOW( window );
            break;

        default:
            unexpected( lexer->CurText() );
        }
        
        tok = nextTok();
    }
}


void SPECCTRA_DB::doWINDOW( WINDOW* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();
    
    while( tok != T_RIGHT )
    {
        if( tok!=T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_rect:
            growth->rectangle = new RECTANGLE( growth );
            doRECTANGLE( growth->rectangle );
            break;
            
        case T_circle:
            growth->circle = new CIRCLE( growth );
            doCIRCLE( growth->circle );
            break;
            
        case T_path:
        case T_polygon:
            growth->path = new PATH( growth, tok );
            doPATH( growth->path );
            break;
            
        case T_qarc:
            growth->qarc = new QARC( growth );
            doQARC( growth->qarc );
            break;
            
        default:
            unexpected( lexer->CurText() );
        }
        
        tok = nextTok();
    }
}


void SPECCTRA_DB::doBOUNDARY( BOUNDARY* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();
    
    if( tok != T_LEFT )
        expecting( T_LEFT );
        
    tok = nextTok();
    if( tok == T_rect )
    {
        if( growth->paths.size() )
            unexpected( "rect when path already encountered" );
    
        growth->rectangle = new RECTANGLE( growth );
        doRECTANGLE( growth->rectangle );
        needRIGHT();
    }
    else if( tok == T_path )
    {
        if( growth->rectangle )
            unexpected( "path when rect already encountered" );

        for(;;)
        {
            if( tok != T_path )
                expecting( T_path );
                    
            PATH* path = new PATH( growth, T_path ) ;
            growth->paths.push_back( path );
            
            doPATH( path );

            tok = nextTok();
            if( tok == T_RIGHT )
                break;

            if( tok != T_LEFT )
                expecting(T_LEFT);

            tok = nextTok();            
        }
    }
    else
        expecting( "rect|path" );
}


void SPECCTRA_DB::doPATH( PATH* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();

    if( !isSymbol( tok ) )
        expecting( "layer_id" );
    
    growth->layer_id = lexer->CurText();

    if( nextTok() != T_NUMBER )
        expecting( "aperture_width" );
    
    growth->aperture_width = strtod( lexer->CurText(), NULL );

    POINT   ptTemp;
    
    tok = nextTok();
    
    do
    {
        if( tok != T_NUMBER )
            expecting( T_NUMBER );
        ptTemp.x = strtod( lexer->CurText(), NULL );
    
        if( nextTok() != T_NUMBER )
            expecting( T_NUMBER );
        ptTemp.y = strtod( lexer->CurText(), NULL );
        
        growth->points.push_back( ptTemp );
           
    } while( (tok = nextTok())!=T_RIGHT && tok!=T_LEFT );
    
    if( tok == T_LEFT )
    {
        if( nextTok() != T_aperture_type )
            expecting( T_aperture_type );
        
        tok = nextTok();
        if( tok!=T_round && tok!=T_square )
            expecting( "round|square" );

        growth->aperture_type = tok;
        
        needRIGHT();
    }
}


void SPECCTRA_DB::doRECTANGLE( RECTANGLE* growth ) throw( IOError )
{
    needSYMBOL();
    growth->layer_id = lexer->CurText();
    
    if( nextTok() != T_NUMBER )
        expecting( T_NUMBER );
    growth->point0.x = strtod( lexer->CurText(), NULL );

    if( nextTok() != T_NUMBER )
        expecting( T_NUMBER );
    growth->point0.y = strtod( lexer->CurText(), NULL );

    if( nextTok() != T_NUMBER )
        expecting( T_NUMBER );
    growth->point1.x = strtod( lexer->CurText(), NULL );

    if( nextTok() != T_NUMBER )
        expecting( T_NUMBER );
    growth->point1.y = strtod( lexer->CurText(), NULL );

    needRIGHT();
}


void SPECCTRA_DB::doCIRCLE( CIRCLE* growth ) throw( IOError )
{
    DSN_T   tok;
    
    needSYMBOL();
    growth->layer_id = lexer->CurText();
    
    if( nextTok() != T_NUMBER )
        expecting( T_NUMBER );
    growth->diameter = strtod( lexer->CurText(), 0 );
    
    tok = nextTok();
    if( tok == T_NUMBER )
    {
        growth->vertex.x = strtod( lexer->CurText(), 0 );
        
        if( nextTok() != T_NUMBER )
            expecting( T_NUMBER );
        growth->vertex.y = strtod( lexer->CurText(), 0 );
        
        tok = nextTok();
    }

    if( tok != T_RIGHT )
        expecting( T_RIGHT );
}


void SPECCTRA_DB::doQARC( QARC* growth ) throw( IOError )
{
    needSYMBOL();
    growth->layer_id = lexer->CurText();
    
    if( nextTok() != T_NUMBER )
        expecting( T_NUMBER );
    growth->aperture_width = strtod( lexer->CurText(), 0 );
    
    for( int i=0;  i<3;  ++i )
    {
        if( nextTok() != T_NUMBER )
            expecting( T_NUMBER );
        growth->vertex[i].x = strtod( lexer->CurText(), 0 );
        
        if( nextTok() != T_NUMBER )
            expecting( T_NUMBER );
        growth->vertex[i].y = strtod( lexer->CurText(), 0 );
    }

    needRIGHT();    
}


void SPECCTRA_DB::doSTRINGPROP( STRINGPROP* growth ) throw( IOError )
{
    needSYMBOL();
    growth->value = lexer->CurText();
    needRIGHT();
}


void SPECCTRA_DB::doTOKPROP( TOKPROP* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();
    
    if( tok<0 )
        unexpected( lexer->CurText() );

    growth->value = tok;

    needRIGHT();    
}


void SPECCTRA_DB::doVIA( VIA* growth ) throw( IOError )
{
    DSN_T   tok;
    
    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok == T_LEFT )
        {
            if( nextTok() != T_spare )
                expecting( T_spare );
            
            while( (tok = nextTok()) != T_RIGHT )
            {
                if( !isSymbol( tok ) )
                    expecting( T_SYMBOL );
                
                growth->spares.push_back( lexer->CurText() );
            }
        }
        else if( isSymbol( tok ) )
        {
            growth->padstacks.push_back( lexer->CurText() );
        }
        else
            unexpected( lexer->CurText() );
    }
}


void SPECCTRA_DB::doCONTROL( CONTROL* growth ) throw( IOError )
{
    DSN_T   tok;
    
    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );

        tok = nextTok();
        switch( tok )
        {
        case T_via_at_smd:
            tok = nextTok();
            if( tok!=T_on && tok!=T_off )
                expecting( "on|off" );
            growth->via_at_smd = (tok==T_on);
            needRIGHT();
            break;
            
        case T_off_grid:
        case T_route_to_fanout_only:
        case T_force_to_terminal_point:
        case T_same_net_checking:
        case T_checking_trim_by_pin:
        case T_noise_calculation:
        case T_noise_accumulation:
        case T_include_pins_in_crosstalk:
        case T_bbv_ctr2ctr:
        case T_average_pair_length:
        case T_crosstalk_model:
        case T_roundoff_rotation:
        case T_microvia:
        case T_reroute_order_viols:
            TOKPROP* tokprop;
            tokprop = new TOKPROP( growth, tok ) ;
            growth->Append( tokprop );
            doTOKPROP( tokprop );
            break;
            
        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doPROPERTIES( PROPERTIES* growth ) throw( IOError )
{
    DSN_T       tok;
    PROPERTY    property;  // construct it once here, append multiple times.

    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        needSYMBOL();
        property.name = lexer->CurText();

        needSYMBOL();        
        property.value = lexer->CurText();
        
        growth->push_back( property );

        needRIGHT();        
    }
}


void SPECCTRA_DB::doLAYER( LAYER* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();
    
    if( !isSymbol(tok) )
        expecting(T_SYMBOL);

    growth->name = lexer->CurText();
    
    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );

        tok = nextTok();
        switch( tok )
        {
        case T_type:
            tok = nextTok();
            if( tok!=T_signal && tok!=T_power && tok!=T_mixed && tok!=T_jumper )
                expecting( "signal|power|mixed|jumper" );
            growth->layer_type = tok;
            if( nextTok()!=T_RIGHT )
                expecting(T_RIGHT);
            break;

        case T_rule:
            growth->rules = new RULE( growth, T_rule );
            doRULE( growth->rules );
            break;
            
        case T_property:
            doPROPERTIES( &growth->properties );
            break;
            
        case T_direction:
            tok = nextTok();
            switch( tok )
            {
            case T_horizontal:
            case T_vertical:
            case T_orthogonal:
            case T_positive_diagonal:
            case T_negative_diagonal:
            case T_diagonal:
            case T_off:
                growth->direction = tok;
                break;
            default:
                // the spec has an example show an abbreviation of the "horizontal" keyword.  Ouch.
                if( !strcmp( "hori", lexer->CurText() ) )
                {
                    growth->direction = T_horizontal;
                    break;
                }
                else if( !strcmp( "vert", lexer->CurText() ) )
                {
                    growth->direction = T_vertical;
                    break;
                }
                expecting( "horizontal|vertical|orthogonal|positive_diagonal|negative_diagonal|diagonal|off" );
            }
            if( nextTok()!=T_RIGHT )
                expecting(T_RIGHT);
            break;

        case T_cost:
            tok = nextTok();
            switch( tok )
            {
            case T_forbidden:
            case T_high:
            case T_medium:
            case T_low:
            case T_free:
                growth->cost = tok;
                break;
            case T_NUMBER:
                // store as negative so we can differentiate between 
                // DSN_T (positive) and T_NUMBER (negative)
                growth->cost = -atoi( lexer->CurText() );   
                break;
            default:
                expecting( "forbidden|high|medium|low|free|<positive_integer>|-1" );
            }
            tok = nextTok();
            if( tok == T_LEFT )
            {
                if( nextTok() != T_type )
                    unexpected( lexer->CurText() );
                
                tok = nextTok();
                if( tok!=T_length && tok!=T_way )
                    expecting( "length|way" );
                
                growth->cost_type = tok;
                if( nextTok()!=T_RIGHT )
                    expecting(T_RIGHT);
                
                tok = nextTok();
            }
            if( tok!=T_RIGHT )
                expecting(T_RIGHT);
            break;

        case T_use_net:
            while( (tok = nextTok()) != T_RIGHT )
            {
                if( !isSymbol(tok) )
                    expecting( T_SYMBOL );
                
                growth->use_net.push_back( lexer->CurText() );
            }
            break;
            
        default:
            unexpected( lexer->CurText() );            
        }
    }
}


void SPECCTRA_DB::doRULE( RULE* growth ) throw( IOError )
{
    std::string     builder;
    int             bracketNesting = 1; // we already saw the opening T_LEFT
    DSN_T           tok = T_NONE;

    while( bracketNesting!=0 && tok!=T_EOF )
    {
        tok = nextTok();
        
        if( tok==T_LEFT)
            ++bracketNesting;
        
        else if( tok==T_RIGHT )
            --bracketNesting;

        if( bracketNesting >= 1 )
        {
            if( lexer->PrevTok()!=T_LEFT && tok!=T_RIGHT && (tok!=T_LEFT || bracketNesting>2) )
                builder += ' ';

            if( tok==T_STRING )
                builder += quote_char;
            
            builder += lexer->CurText();
            
            if( tok==T_STRING )
                builder += quote_char;
        }

        // When the nested rule is closed with a T_RIGHT and we are back down
        // to bracketNesting == 1, (inside the <rule_descriptor> but outside
        // the last rule).  Then save the last rule and clear the string builder.
        if( bracketNesting == 1 )
        {
           growth->rules.push_back( builder );
           builder.clear();
        }
    }
    
    if( tok==T_EOF )
        unexpected( T_EOF );
}


#if 0
void SPECCTRA_DB::doPLACE_RULE( PLACE_RULE* growth, bool expect_object_type ) throw( IOError )
{
    /*   (place_rule [<structure_place_rule_object> ]
         {[<spacing_descriptor> |
         <permit_orient_descriptor> |
         <permit_side_descriptor> |
         <opposite_side_descriptor> ]}
         )
    */
    
    DSN_T   tok = nextTok();
    
    if( tok!=T_LEFT )
        expecting( T_LEFT );
    
    tok = nextTok();
    if( tok==T_object_type )
    {
        if( !expect_object_type )
            unexpected( tok );
        
        /*  [(object_type
              [pcb |
              image_set [large | small | discrete | capacitor | resistor]
              [(image_type [smd | pin])]]
            )]
        */
        
        tok = nextTok();
        switch( tok )
        {
        case T_pcb:
            growth->object_type = tok;
            break;
            
        case T_image_set:
            tok = nextTok();
            switch( tok )
            {
            case T_large:
            case T_small:
            case T_discrete:
            case T_capacitor:
            case T_resistor:
                growth->object_type = tok;
                break;
            default:
                unexpected( lexer->CurText() );
            }
            break;
            
        default:
            unexpected( lexer->CurText() );
        }
        
        tok = nextTok();
        if( tok == T_LEFT )
        {
            tok = nextTok();
            if( tok != T_image_type )
                expecting( T_image_type );
            
            tok = nextTok();
            if( tok!=T_smd && tok!=T_pin )
                expecting( "smd|pin" );
            
            needRIGHT();
            
            tok = nextTok();
        }
        
        if( tok != T_RIGHT )
            expecting( T_RIGHT );
        
        tok = nextTok();
    }

    /*  {[<spacing_descriptor> | 
        <permit_orient_descriptor> | 
        <permit_side_descriptor> | <opposite_side_descriptor> ]}
    */
    doRULE( growth );
}
#endif


void SPECCTRA_DB::doREGION( REGION* growth ) throw( IOError )
{
    DSN_T tok = nextTok();
    
    if( isSymbol(tok) )
    {
        growth->region_id = lexer->CurText();
        tok = nextTok();
    }

    for(;;)
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );

        tok = nextTok();
        switch( tok )
        {
        case T_rect:
            if( growth->rectangle )
                unexpected( tok );
            growth->rectangle = new RECTANGLE( growth );
            doRECTANGLE( growth->rectangle );
            break;
            
        case T_polygon:
            if( growth->polygon )
                unexpected( tok );
            growth->polygon = new PATH( growth, T_polygon );
            doPATH( growth->polygon );
            break;
            
        case T_region_net:
        case T_region_class:
            STRINGPROP* stringprop;
            stringprop = new STRINGPROP( growth, tok );
            growth->Append( stringprop );
            doSTRINGPROP( stringprop );
            break;            

        case T_region_class_class:
            CLASS_CLASS* class_class;
            class_class = new CLASS_CLASS( growth, tok );
            growth->Append( class_class );
            doCLASS_CLASS( class_class ); 
            break;
            
        case T_rule:
            if( growth->rules )
                unexpected( tok );
            growth->rules = new RULE( growth, T_rule );
            doRULE( growth->rules );
            break;
            
        default:
            unexpected( lexer->CurText() );
        }

        tok = nextTok();
        if( tok == T_RIGHT )
        {
            if( !growth->rules )
                expecting( T_rule );
            break;
        }
    }
}


void SPECCTRA_DB::doCLASS_CLASS( CLASS_CLASS* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();
    
    if( tok != T_LEFT )
        expecting( T_LEFT );
    
    while( (tok = nextTok()) != T_RIGHT )
    {
        switch( tok )
        {
        case T_classes:
            if( growth->classes )
                unexpected( tok );
            growth->classes = new CLASSES( growth );
            doCLASSES( growth->classes );
            break;
            
        case T_rule:
            // only T_class_class takes a T_rule
            if( growth->Type() == T_region_class_class )
                unexpected( tok );
            RULE* rule;
            rule = new RULE( growth, T_rule );
            growth->Append( rule );
            doRULE( rule );
            break;
            
        case T_layer_rule:
            // only T_class_class takes a T_layer_rule
            if( growth->Type() == T_region_class_class )
                unexpected( tok );
            LAYER_RULE* layer_rule;
            layer_rule = new LAYER_RULE( growth );
            growth->Append( layer_rule );
            doLAYER_RULE( layer_rule );
            break;
            
        default:            
            unexpected( tok );
        }
    }
}


void SPECCTRA_DB::doCLASSES( CLASSES* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();

    // require at least 2 class_ids
    
    if( !isSymbol( tok ) )
        expecting( "class_id" );
    
    growth->class_ids.push_back( lexer->CurText() );
    
    do
    {
        tok = nextTok();
        if( !isSymbol( tok ) )
            expecting( "class_id" );
        
        growth->class_ids.push_back( lexer->CurText() );
        
    } while( (tok = nextTok()) != T_RIGHT );
}


void SPECCTRA_DB::doGRID( GRID* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();
    
    switch( tok )
    {
    case T_via:
    case T_wire:
    case T_via_keepout:
    case T_snap:
    case T_place:
        growth->grid_type = tok;
        if( nextTok() != T_NUMBER )
            expecting( T_NUMBER );
        growth->dimension = strtod( lexer->CurText(), 0 );
        tok = nextTok();
        if( tok == T_LEFT )
        {
            while( (tok=nextTok()) != T_RIGHT )
            {
                if( tok==T_direction )
                {
                    if( growth->grid_type == T_place )
                        unexpected( tok );
                    tok = nextTok();
                    if( tok!=T_x && tok!=T_y )
                        unexpected( lexer->CurText() );
                    growth->direction = tok;
                    if( nextTok() != T_RIGHT )
                        expecting(T_RIGHT);
                }
                else if( tok==T_offset )
                {
                    if( growth->grid_type == T_place )
                        unexpected( tok );
                    
                    if( nextTok() != T_NUMBER )
                        expecting( T_NUMBER );
                    
                    growth->offset = strtod( lexer->CurText(), 0 );
                    
                    if( nextTok() != T_RIGHT )
                        expecting(T_RIGHT);
                }
                else if( tok==T_image_type )
                {
                    if( growth->grid_type != T_place )
                        unexpected( tok );
                    tok = nextTok();
                    if( tok!=T_smd && tok!=T_pin )
                        unexpected( lexer->CurText() );
                    growth->image_type = tok;
                    if( nextTok() != T_RIGHT )
                        expecting(T_RIGHT);
                }
            }
        }
        break;

    default:
        unexpected( tok );
    }
}


void SPECCTRA_DB::doLAYER_RULE( LAYER_RULE* growth ) throw( IOError )
{
    DSN_T   tok;
    
    needSYMBOL();
    
    do
    {
        growth->layer_ids.push_back( lexer->CurText() );
        
    }  while( isSymbol(tok = nextTok()) );
 
    if( tok != T_LEFT )
        expecting( T_LEFT );
    
    if( nextTok() != T_rule )
        expecting( T_rule );
    
    growth->rule = new RULE( growth, T_rule );
    doRULE( growth->rule );
    
    needRIGHT();
}


void SPECCTRA_DB::doPLACE( PLACE* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();
    
    if( !isSymbol( tok ) )
        expecting( "component_id" );
    
    growth->component_id = lexer->CurText();    
    
    tok = nextTok();
    if( tok == T_NUMBER )
    {
        POINT   point;
        
        point.x = strtod( lexer->CurText(), 0 );
        
        if( nextTok() != T_NUMBER )
            expecting( T_NUMBER );
        point.y = strtod( lexer->CurText(), 0 );
        
        growth->SetVertex( point );
        
        tok = nextTok();
        if( tok!=T_front && tok!=T_back )
            expecting( "front|back" );
        growth->side = tok;
        
        if( nextTok() != T_NUMBER )
            expecting( "rotation" );
        growth->SetRotation( strtod( lexer->CurText(), 0)  );
    }

    while( (tok = nextTok()) != T_RIGHT )
    {
        switch( tok )
        {
        case T_mirror:
            tok = nextTok();
            if( tok==T_x || tok==T_y || tok==T_xy || tok==T_off )
                growth->mirror = tok;
            else
                expecting("x|y|xy|off");
            break;
           
        case T_status:
            tok = nextTok();
            if( tok==T_added || tok==T_deleted || tok==T_substituted )
                growth->status = tok;
            else
                expecting("added|deleted|substituted");
            break;
            
        case T_logical_part:
            if( growth->logical_part.size() )
                unexpected( tok );
            tok = nextTok();
            if( !isSymbol( tok ) )
                expecting( "logical_part_id");
            growth->logical_part = lexer->CurText();
            break;
            
        case T_place_rule:
            if( growth->place_rules )
                unexpected( tok );
            growth->place_rules = new RULE( growth, T_place_rule );
            doRULE( growth->place_rules );
            break;
            
        case T_property:
            if( growth->properties.size() )
                unexpected( tok );
            doPROPERTIES( &growth->properties );
            break;
            
        case T_lock_type:
            tok = nextTok();
            if( tok==T_position || tok==T_gate || tok==T_subgate || tok==T_pin )
                growth->lock_type = tok;
            else
                expecting("position|gate|subgate|pin");
            break;

        case T_rule:
            if( growth->rules || growth->region )
                unexpected( tok );
            growth->rules = new RULE( growth, T_rule );
            doRULE( growth->rules );
            break;

        case T_region:
            if( growth->rules || growth->region )
                unexpected( tok );
            growth->region = new REGION( growth );
            doREGION( growth->region );
            break;

        case T_pn:
            if( growth->part_number.size() )
                unexpected( tok );
            growth->part_number = lexer->CurText();
            break;
            
        default:
            unexpected( tok );
        }
    }
}


void SPECCTRA_DB::doCOMPONENT( COMPONENT* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();
    
    if( !isSymbol( tok ) )
        expecting( "image_id" );
    growth->image_id = lexer->CurText();

    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_place:
            PLACE* place;
            place = new PLACE( growth );
            growth->places.push_back( place );
            doPLACE( place );
            break;
            
        default:
            unexpected(tok);
        }
    }
}


void SPECCTRA_DB::doPLACEMENT( PLACEMENT* growth ) throw( IOError )
{
    DSN_T   tok;
    
    needLEFT();
    
    tok = nextTok();
    if( tok==T_unit || tok==T_resolution )
    {
        growth->unit = new UNIT_RES( growth, tok );
        if( tok==T_resolution )
            doRESOLUTION( growth->unit );
        else
            doUNIT( growth->unit );
        
        if( nextTok() != T_LEFT )
            expecting( T_LEFT );
        tok = nextTok();
    }
    
    if( tok == T_place_control )
    {
        if( nextTok() != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        if( tok != T_flip_style )
            expecting( T_flip_style );
        
        tok = nextTok();
        if( tok==T_mirror_first || tok==T_rotate_first )
            growth->flip_style = tok;
        else
            expecting("mirror_first|rotate_first");

        needRIGHT();
        needRIGHT();
        needLEFT();        
        tok = nextTok();
    }

    while( tok == T_component )
    {
        COMPONENT* component = new COMPONENT( growth );
        growth->components.push_back( component );
        doCOMPONENT( component );
        
        tok = nextTok();
        if( tok == T_RIGHT )
            return;

        else if( tok == T_LEFT )        
            tok = nextTok();
    }
    
    unexpected( lexer->CurText() );
}


void SPECCTRA_DB::doPADSTACK( PADSTACK* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();

    /*  (padstack <padstack_id >
        [<unit_descriptor> ]
        {(shape <shape_descriptor>
            [<reduced_shape_descriptor> ]
            [(connect [on | off])]
            [{<window_descriptor> }]
        )}
        [<attach_descriptor> ]
        [{<pad_via_site_descriptor> }]
        [(rotate [on | off])]
        [(absolute [on | off])]
        [(rule <clearance_descriptor> )])
    */
    
    // padstack_id may be a number
    if( !isSymbol( tok ) && tok!=T_NUMBER )
        expecting( "padstack_id" );
    
    growth->padstack_id = lexer->CurText();
    
    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_unit:
            if( growth->unit )
                unexpected( tok );
            growth->unit = new UNIT_RES( growth, tok );
            doUNIT( growth->unit );
            break;

        case T_rotate:
            tok = nextTok();
            if( tok!=T_on && tok!=T_off )
                expecting( "on|off" );
            growth->rotate = tok;
            needRIGHT();
            break;

        case T_absolute:
            tok = nextTok();
            if( tok!=T_on && tok!=T_off )
                expecting( "on|off" );
            growth->absolute = tok;
            needRIGHT();
            break;
            
        case T_shape:
            SHAPE* shape;
            shape = new SHAPE( growth );
            growth->Append( shape );
            doSHAPE( shape );
            break;

        case T_attach:
            tok = nextTok();
            if( tok!=T_off && tok!=T_on )
                expecting( "off|on" );
            growth->attach = tok;
            tok = nextTok();
            if( tok == T_LEFT )
            {
                if( nextTok() != T_use_via )
                    expecting( T_use_via );
                
                needSYMBOL();
                growth->via_id = lexer->CurText();

                needRIGHT();
                needRIGHT();                
            }
            break;
            
        /*
        case T_via_site:        not supported
            break;
        */            
            
        case T_rule:
            if( growth->rules )
                unexpected( tok );
            growth->rules = new RULE( growth, T_rule );
            doRULE( growth->rules );
            break;
            
        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doSHAPE( SHAPE* growth ) throw( IOError )
{
    DSN_T   tok;

    /*  (shape <shape_descriptor>
         [<reduced_shape_descriptor> ]
         [(connect [on | off])]
         [{<window_descriptor> }])
    */
    
    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_rect:
        case T_circle:
        case T_path:
        case T_polygon:
        case T_qarc:
L_done_that:    
            if( growth->rectangle || growth->circle || growth->path || growth->qarc )
                unexpected( tok );
            break;
        default:
            // the example in the spec uses "circ" instead of "circle".  Bad!
            if( !strcmp( "circ", lexer->CurText() ) )
            {
                tok = T_circle;
                goto L_done_that;
            }
        }
        
        switch( tok )
        {
        case T_rect:
            growth->rectangle = new RECTANGLE( growth );
            doRECTANGLE( growth->rectangle );
            break;
            
        case T_circle:
            growth->circle = new CIRCLE( growth );
            doCIRCLE( growth->circle );
            break;
        
        case T_path:
        case T_polygon:
            growth->path = new PATH( growth, tok );
            doPATH( growth->path );
            break;
            
        case T_qarc:
            growth->qarc = new QARC( growth );
            doQARC( growth->qarc );
            break;

        case T_connect:
            tok = nextTok();
            if( tok!=T_on && tok!=T_off )
                expecting( "on|off" );
            growth->connect = tok;
            needRIGHT();
            break;

        case T_window:
            WINDOW* window;
            window = new WINDOW( growth );
            growth->windows.push_back( window );
            doWINDOW( window );
            break;
            
        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doIMAGE( IMAGE* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();

    /*  <image_descriptor >::=
        (image <image_id >
           [(side [front | back | both])]
           [<unit_descriptor> ]
           [<outline_descriptor> ]
           {(pin <padstack_id > [(rotate <rotation> )]
              [<reference_descriptor> | <pin_array_descriptor> ]
              [<user_property_descriptor> ])}
           [{<conductor_shape_descriptor> }]
           [{<conductor_via_descriptor> }]
           [<rule_descriptor> ]
           [<place_rule_descriptor> ]
           [{<keepout_descriptor> }]
        [<image_property_descriptor> ]
        )
    */

    if( !isSymbol( tok ) )
        expecting( "image_id" );
    
    growth->image_id = lexer->CurText();
    
    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_unit:
            if( growth->unit )
                unexpected( tok );
            growth->unit = new UNIT_RES( growth, tok );
            doUNIT( growth->unit );
            break;

        case T_side:
            tok = nextTok();
            if( tok!=T_front && tok!=T_back && tok!=T_both )
                expecting( "front|back|both" );
            growth->side = tok;
            needRIGHT();
            break;

        case T_outline:
            SHAPE* outline;
            outline = new SHAPE( growth, T_outline );   // use SHAPE for T_outline
            growth->Append( outline );
            doSHAPE( outline );
            break;

        case T_pin:
            PIN* pin;
            pin = new PIN( growth );
            growth->pins.push_back( pin );
            doPIN( pin );
            break;
            
        case T_rule:
            if( growth->rules )
                unexpected( tok );
            growth->rules = new RULE( growth, tok );
            doRULE( growth->rules );
            break;

        case T_place_rule:
            if( growth->place_rules )
                unexpected( tok );
            growth->place_rules = new RULE( growth, tok );
            doRULE( growth->place_rules );
            break;

        case T_keepout:
        case T_place_keepout:
        case T_via_keepout:
        case T_wire_keepout:
        case T_bend_keepout:
        case T_elongate_keepout:
            KEEPOUT* keepout;
            keepout = new KEEPOUT( growth, tok );
            growth->keepouts.push_back( keepout );
            doKEEPOUT( keepout );
            break;
            
        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doPIN( PIN* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();

    /*  (pin <padstack_id > [(rotate <rotation> )]
          [<reference_descriptor> | <pin_array_descriptor> ]
          [<user_property_descriptor> ])
    */

    // a padstack_id may be a number
    if( !isSymbol( tok ) && tok!=T_NUMBER )
        expecting( "padstack_id" );
    
    growth->padstack_id = lexer->CurText();
    
    tok = nextTok();
    if( tok == T_LEFT )
    {
        tok = nextTok();
        if( tok != T_rotate )
            expecting( T_rotate );
        
        if( nextTok() != T_NUMBER )
            expecting( T_NUMBER );
        growth->SetRotation( strtod( lexer->CurText(), 0 ) );
        needRIGHT();
        tok = nextTok();
    }
    
    if( !isSymbol(tok) && tok!=T_NUMBER )
        expecting( "pin_id" );

    growth->pin_id = lexer->CurText();

    if( nextTok() != T_NUMBER )
        expecting( T_NUMBER );
    growth->vertex.x = strtod( lexer->CurText(), 0 );
    
    if( nextTok() != T_NUMBER )
        expecting( T_NUMBER );
    growth->vertex.y = strtod( lexer->CurText(), 0 );

    if( nextTok() != T_RIGHT )
        unexpected( lexer->CurText() );
}


void SPECCTRA_DB::doLIBRARY( LIBRARY* growth ) throw( IOError )
{
    DSN_T   tok;

    /*  <library_descriptor >::=
        (library
           [<unit_descriptor> ]
           {<image_descriptor> }
           [{<jumper_descriptor> }]
           {<padstack_descriptor> }
           {<via_array_template_descriptor> }
           [<directory_descriptor> ]
           [<extra_image_directory_descriptor> ]
           [{<family_family_descriptor> }]
           [{<image_image_descriptor> }]
        )
    */
    
    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_unit:
            if( growth->unit )
                unexpected( tok );
            growth->unit = new UNIT_RES( growth, tok );
            doUNIT( growth->unit );
            break;
            
        case T_padstack:
            PADSTACK* padstack;
            padstack = new PADSTACK( growth );
            growth->padstacks.push_back( padstack );
            doPADSTACK( padstack );
            break;

        case T_image:
            IMAGE*  image;
            image = new IMAGE( growth );
            growth->images.push_back( image );
            doIMAGE( image );
            break;
            
        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doNET( NET* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();

    /*  <net_descriptor >::=
        (net <net_id >
          [(unassigned)]
          [(net_number <integer >)]
          [(pins {<pin_reference> }) | (order {<pin_reference> })]
          [<component_order_descriptor> ]
          [(type [fix | normal])]
          [<user_property_descriptor> ]
          [<circuit_descriptor> ]
          [<rule_descriptor> ]
          [{<layer_rule_descriptor> }]
          [<fromto_descriptor> ]
          [(expose {<pin_reference> })]
          [(noexpose {<pin_reference> })]
          [(source {<pin_reference> })]
          [(load {<pin_reference> })]
          [(terminator {<pin_reference> })]
          [(supply [power | ground])]
        )
    */

    if( !isSymbol( tok ) )
        expecting( "net_id" );
    
    growth->net_id = lexer->CurText();
    
    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_unassigned:
            growth->unassigned = true;
            needRIGHT();
            break;

        case T_net_number:
            if( nextTok() != T_NUMBER )
                expecting( T_NUMBER );
            growth->net_number = atoi( lexer->CurText() );
            needRIGHT();
            break;

        case T_pins:
        case T_order:
            growth->pins_type = tok;
            {
                PIN_REF     empty( growth );
                while( (tok = nextTok()) != T_RIGHT )
                {
                    // copy the empty one, then fill its copy later thru pin_ref.                
                    growth->pins.push_back( empty );

                    PIN_REF* pin_ref = &growth->pins.back();

                    readCOMPnPIN( &pin_ref->component_id, &pin_ref->pin_id );
                }
            }
            break;

        case T_comp_order:
            if( growth->comp_order )
                unexpected( tok );
            growth->comp_order = new COMP_ORDER( growth );
            doCOMP_ORDER( growth->comp_order );
            break;
            
        case T_type:
            tok = nextTok();
            if( tok!=T_fix && tok!=T_normal )
                expecting( "fix|normal" );
            growth->type = tok;
            needRIGHT();
            break;

/* @todo            
        case T_circuit:
            break;
*/

        case T_rule:
            if( growth->rules )
                unexpected( tok );
            growth->rules = new RULE( growth, T_rule );
            doRULE( growth->rules );
            break;
            
        case T_layer_rule:
            LAYER_RULE* layer_rule;
            layer_rule = new LAYER_RULE( growth );
            growth->layer_rules.push_back( layer_rule );
            doLAYER_RULE( layer_rule );
            break;
            
        case T_fromto:
            FROMTO* fromto;
            fromto = new FROMTO( growth );
            growth->fromtos.push_back( fromto );
            doFROMTO( fromto );
            break;

        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doTOPOLOGY( TOPOLOGY* growth ) throw( IOError )
{
    DSN_T   tok;

    /*  <topology_descriptor >::=
        (topology {[<fromto_descriptor> |
        <component_order_descriptor> ]})
    */

    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_fromto:
            FROMTO* fromto;
            fromto = new FROMTO( growth );
            growth->fromtos.push_back( fromto );
            doFROMTO( fromto );
            break;
        
        case T_comp_order:
            COMP_ORDER*  comp_order;
            comp_order = new COMP_ORDER( growth );
            growth->comp_orders.push_back( comp_order );
            doCOMP_ORDER( comp_order );
            break;

        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doCLASS( CLASS* growth ) throw( IOError )
{
    DSN_T   tok;

    /*  <class_descriptor >::=
        (class
           <class_id > {[{<net_id >} | {<composite_name_list> }]}
           [<circuit_descriptor> ]
           [<rule_descriptor> ]
           [{<layer_rule_descriptor> }]
           [<topology_descriptor> ]
        )
    */

    needSYMBOL();
    
    growth->class_id = lexer->CurText();

    // do net_ids, do not support <composite_name_list>s at this time
    while( isSymbol(tok = nextTok()) )
    {
        growth->net_ids.push_back( lexer->CurText() );
    }
    
    
    while( tok != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_rule:
            if( growth->rules )
                unexpected( tok );
            growth->rules = new RULE( growth, T_rule );
            doRULE( growth->rules );
            break;
            
        case T_layer_rule:
            LAYER_RULE* layer_rule;
            layer_rule = new LAYER_RULE( growth );
            growth->layer_rules.push_back( layer_rule );
            doLAYER_RULE( layer_rule );
            break;
            
        case T_topology:
            if( growth->topology )
                unexpected( tok );
            growth->topology = new TOPOLOGY( growth );
            doTOPOLOGY( growth->topology );
            break;
            
        default:    // handle all the circuit_descriptor here as strings
            {
                std::string     builder;
                int             bracketNesting = 1; // we already saw the opening T_LEFT
                DSN_T           tok = T_NONE;
            
                builder += '(';
                builder += lexer->CurText();
                
                while( bracketNesting!=0 && tok!=T_EOF )
                {
                    tok = nextTok();
                    
                    if( tok==T_LEFT)
                        ++bracketNesting;
                    
                    else if( tok==T_RIGHT )
                        --bracketNesting;
            
                    if( bracketNesting >= 1 )
                    {
                        if( lexer->PrevTok() != T_LEFT && tok!=T_RIGHT )
                            builder += ' ';
            
                        if( tok==T_STRING )
                            builder += quote_char;
                        
                        builder += lexer->CurText();
                        
                        if( tok==T_STRING )
                            builder += quote_char;
                    }
            
                    // When the nested rule is closed with a T_RIGHT and we are back down
                    // to bracketNesting == 0, then save the builder and break;
                    if( bracketNesting == 0 )
                    {
                        builder += ')';
                       growth->circuit.push_back( builder );
                       break;
                    }
                }
                
                if( tok==T_EOF )
                    unexpected( T_EOF );
            }                                   // scope bracket
        }                                       // switch
        
        tok = nextTok();
        
    } // while
}


void SPECCTRA_DB::doNETWORK( NETWORK* growth ) throw( IOError )
{
    DSN_T   tok;

    /*  <network_descriptor >::=
        (network
          {<net_descriptor>}
          [{<class_descriptor> }]
          [{<class_class_descriptor> }]
          [{<group_descriptor> }]
          [{<group_set_descriptor> }]
          [{<pair_descriptor> }]
          [{<bundle_descriptor> }]
        )
    */

    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_net:
            NET* net;
            net = new NET( growth );
            growth->nets.push_back( net );
            doNET( net );
            break;
        
        case T_class:
            CLASS*  myclass;
            myclass = new CLASS( growth );
            growth->classes.push_back( myclass );
            doCLASS( myclass );
            break;

        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doCOMP_ORDER( COMP_ORDER* growth ) throw( IOError )
{
    DSN_T   tok;

    /*  <component_order_descriptor >::=
        (comp_order {<placement_id> })
    */

    while( isSymbol(tok = nextTok()) )
    {
        growth->placement_ids.push_back( lexer->CurText() );
    }
    
    if( tok != T_RIGHT )
        expecting( T_RIGHT );
}


void SPECCTRA_DB::doFROMTO( FROMTO* growth ) throw( IOError )
{
    DSN_T   tok;

    /*  <fromto_descriptor >::=
        {(fromto
           [<pin_reference> | <virtual_pin_descriptor> ] | <component_id >]
           [<pin_reference> | <virtual_pin_descriptor> | <component_id >]
           [(type [fix | normal | soft])]
           [(net <net_id >)]
           [<rule_descriptor> ]
           [<circuit_descriptor> ]
           [{<layer_rule_descriptor> }]
        )}
    */

    
    // read the first two grammar items in as 2 single tokens, i.e. do not
    // split apart the <pin_reference>s into 3 separate tokens.  Do this by
    // turning off the string delimiter in the lexer.
    
    int old = lexer->SetStringDelimiter( 0 );
    
    if( !isSymbol(nextTok() ) )
    {
        lexer->SetStringDelimiter( old );
        expecting( T_SYMBOL );
    }
    growth->fromText = lexer->CurText();
    
    if( !isSymbol(nextTok() ) )
    {
        lexer->SetStringDelimiter( old );
        expecting( T_SYMBOL );
    }
    growth->toText = lexer->CurText();

    lexer->SetStringDelimiter( old );
    
    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_type:
            tok = nextTok();
            if( tok!=T_fix && tok!=T_normal && tok!=T_soft )
                expecting( "fix|normal|soft" );
            growth->fromto_type = tok;
            needRIGHT();
            break;
        
        case T_rule:
            if( growth->rules )
                unexpected( tok );
            growth->rules = new RULE( growth, T_rule );
            doRULE( growth->rules );
            break;

        case T_layer_rule:
            LAYER_RULE* layer_rule;
            layer_rule = new LAYER_RULE( growth );
            growth->layer_rules.push_back( layer_rule );
            doLAYER_RULE( layer_rule );
            break;

        case T_net:
            if( growth->net_id.size() )
                unexpected( tok );
            needSYMBOL();
            growth->net_id = lexer->CurText();
            needRIGHT();
            break;
            
        // circuit descriptor not supported at this time
            
        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doWIRE( WIRE* growth ) throw( IOError )
{
    DSN_T   tok;

    /*  <wire_shape_descriptor >::=
        (wire
          <shape_descriptor>
          [(net <net_id >)]
          [(turret <turret#> )]
          [(type [fix | route | normal | protect])]
          [(attr [test | fanout | bus | jumper])]
          [(shield <net_id >)]
          [{<window_descriptor> }]
          [(connect
             (terminal <object_type> [<pin_reference> ])
             (terminal <object_type> [<pin_reference> ])
          )]
          [(supply)]
        )
    */

    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );

        tok = nextTok();
        switch( tok )
        {
        case T_rect:
        case T_circle:
        case T_path:
        case T_polygon:
        case T_qarc:
            if( growth->rectangle || growth->circle || growth->path || growth->qarc )
                unexpected( tok );
        default: ;
        }
        
        switch( tok )
        {
        case T_rect:
            growth->rectangle = new RECTANGLE( growth );
            doRECTANGLE( growth->rectangle );
            break;
            
        case T_circle:
            growth->circle = new CIRCLE( growth );
            doCIRCLE( growth->circle );
            break;
        
        case T_path:
        case T_polygon:
            growth->path = new PATH( growth, tok );
            doPATH( growth->path );
            break;
            
        case T_qarc:
            growth->qarc = new QARC( growth );
            doQARC( growth->qarc );
            break;

        case T_net:
            needSYMBOL();
            growth->net_id = lexer->CurText();
            needRIGHT();
            break;
            
        case T_turret:
            if( nextTok() != T_NUMBER )
                expecting( T_NUMBER );
            growth->turret = atoi( lexer->CurText() );
            needRIGHT();
            break;
            
        case T_type:
            tok = nextTok();
            if( tok!=T_fix && tok!=T_route && tok!=T_normal && tok!=T_protect )
                expecting( "fix|route|normal|protect" );
            growth->type = tok;
            needRIGHT();
            break;

        case T_attr:
            tok = nextTok();
            if( tok!=T_test && tok!=T_fanout && tok!=T_bus && tok!=T_jumper )
                expecting( "test|fanout|bus|jumper" );
            growth->attr = tok;
            needRIGHT();
            break;

        case T_shield:
            needSYMBOL();
            growth->shield = lexer->CurText();
            needRIGHT();
            break;
            
        case T_window:
            WINDOW* window;
            window = new WINDOW( growth );
            growth->windows.push_back( window );
            doWINDOW( window );
            break;
            
        case T_connect:
            if( growth->connect )
                unexpected( tok );
/* @todo            
            growth->connect = new CONNECT( growth );
            doCONNECT( growth->connect );
*/            
            break;
            
        case T_supply:
            growth->supply = true;
            needRIGHT();
            break;
            
        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doWIRE_VIA( WIRE_VIA* growth ) throw( IOError )
{
    DSN_T   tok;
    POINT   point;
    
    /*  <wire_via_descriptor >::=
        (via
           <padstack_id > {<vertex> }
           [(net <net_id >)]
           [(via_number <via#> )]
           [(type [fix | route | normal | protect])]
           [(attr [test | fanout | jumper |
              virtual_pin <virtual_pin_name> ])]
           [(contact {<layer_id >})]
           [(supply)]
        )
        (virtual_pin
           <virtual_pin_name> <vertex> (net <net_id >)
        )
    */

    needSYMBOL();
    growth->padstack_id = lexer->CurText();

    while( (tok = nextTok()) == T_NUMBER )
    {
        point.x = strtod( lexer->CurText(), 0 );
        
        if( nextTok() != T_NUMBER )
            expecting( "vertex.y" );
        
        point.y = strtod( lexer->CurText(), 0 );
        
        growth->vertexes.push_back( point );
    }

    while( tok != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_net:
            needSYMBOL();
            growth->net_id = lexer->CurText();
            needRIGHT();
            break;
            
        case T_via_number:
            if( nextTok() != T_NUMBER )
                expecting( "<via#>" );
            growth->via_number = atoi( lexer->CurText() );
            needRIGHT();
            break;
            
        case T_type:
            tok = nextTok();
            if( tok!=T_fix && tok!=T_route && tok!=T_normal && tok!=T_protect )
                expecting( "fix|route|normal|protect" );
            growth->type = tok;
            needRIGHT();
            break;
            
        case T_attr:
            tok = nextTok();
            if( tok!=T_test && tok!=T_fanout && tok!=T_jumper && tok!=T_virtual_pin )
                expecting( "test|fanout|jumper|virtual_pin" );
            growth->attr = tok;
            if( tok == T_virtual_pin )
            {
                needSYMBOL();
                growth->virtual_pin_name = lexer->CurText();
            }
            needRIGHT();
            break;
            
        case T_contact:
            needSYMBOL();
            tok = T_SYMBOL;
            while( isSymbol(tok) )
            {
                growth->contact_layers.push_back( lexer->CurText() );
                tok = nextTok();
            }
            if( tok != T_RIGHT )
                expecting( T_RIGHT );
            break;
            
        case T_supply:
            growth->supply = true;
            needRIGHT();
            break;
            
        default:
            unexpected( lexer->CurText() );
        }
        
        tok = nextTok();
    }
}


void SPECCTRA_DB::doWIRING( WIRING* growth ) throw( IOError )
{
    DSN_T   tok;

    /*  <wiring_descriptor >::=
        (wiring
          [<unit_descriptor> | <resolution_descriptor> | null]
          {<wire_descriptor> }
          [<test_points_descriptor> ]
          {[<supply_pin_descriptor> ]}
        )
    */

    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_unit:
            if( growth->unit )
                unexpected( tok );
            growth->unit = new UNIT_RES( growth, tok );
            doUNIT( growth->unit );
            break;
            
        case T_resolution:
            if( growth->unit )
                unexpected( tok );
            growth->unit = new UNIT_RES( growth, tok );
            doRESOLUTION( growth->unit );
            break;
            
        case T_wire:
            WIRE* wire;
            wire = new WIRE( growth );
            growth->wires.push_back( wire );
            doWIRE( wire );
            break;

        case T_via:
            WIRE_VIA* wire_via;
            wire_via = new WIRE_VIA( growth );
            growth->wire_vias.push_back( wire_via );
            doWIRE_VIA( wire_via );
            break;
            
        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doANCESTOR( ANCESTOR* growth ) throw( IOError )
{
    DSN_T   tok;

    /*  <ancestor_file_descriptor >::=
          (ancestor <file_path_name> (created_time <time_stamp> )
          [(comment <comment_string> )])
    */
    
    needSYMBOL();
    growth->filename = lexer->CurText();
    
    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_created_time:
            readTIME( &growth->time_stamp );
            needRIGHT();
            break;
            
        case T_comment:
            needSYMBOL();
            growth->comment = lexer->CurText();
            needRIGHT();
            break;
            
        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doHISTORY( HISTORY* growth ) throw( IOError )
{
    DSN_T   tok;

    /*  <history_descriptor >::=
        (history [{<ancestor_file_descriptor> }] <self_descriptor> )
    */
    
    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_ancestor:
            ANCESTOR* ancestor;
            ancestor = new ANCESTOR( growth );
            growth->ancestors.push_back( ancestor );
            doANCESTOR( ancestor );
            break;
            
        case T_self:
            while( (tok = nextTok()) != T_RIGHT )
            {
                if( tok != T_LEFT )
                    expecting( T_LEFT );

                tok = nextTok();                
                switch( tok )
                {
                case T_created_time:
                    readTIME( &growth->time_stamp );
                    needRIGHT();
                    break;
                
                case T_comment:
                    needSYMBOL();
                    growth->comments.push_back( lexer->CurText() );
                    needRIGHT();
                    break;
                    
                default:
                    unexpected( lexer->CurText() );
                }
            }
            break;
            
        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doSESSION( SESSION* growth ) throw( IOError )
{
    DSN_T   tok;

    /*  <session_file_descriptor >::=
        (session <session_id >
          (base_design <path/filename >)
          [<history_descriptor> ]
          [<session_structure_descriptor> ]
          [<placement_descriptor> ]
          [<floor_plan_descriptor> ]
          [<net_pin_changes_descriptor> ]
          [<was_is_descriptor> ]
          <swap_history_descriptor> ]
          [<route_descriptor> ]
        )
    */
    
    needSYMBOL();
    growth->session_id = lexer->CurText();

    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_base_design:
            needSYMBOL();
            growth->base_design = lexer->CurText();
            needRIGHT();
            break;
            
        case T_history:
            if( growth->history )
                unexpected( tok );
            growth->history = new HISTORY( growth );
            doHISTORY( growth->history );
            break;
            
        case T_structure:
            if( growth->structure )
                unexpected( tok );
            growth->structure = new STRUCTURE( growth );
            doSTRUCTURE( growth->structure );
            break;
            
        case T_placement:
            if( growth->placement )
                unexpected( tok );
            growth->placement = new PLACEMENT( growth );
            doPLACEMENT( growth->placement );
            break;
            
        case T_was_is:
            if( growth->was_is )
                unexpected( tok );
            growth->was_is = new WAS_IS( growth );
            doWAS_IS( growth->was_is );
            break;

        case T_routes:
            if( growth->route )
                unexpected( tok );
            growth->route = new ROUTE( growth );
            doROUTE( growth->route );
            break;
            
        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doWAS_IS( WAS_IS* growth ) throw( IOError )
{
    DSN_T       tok;
    PIN_PAIR    empty( growth );
    PIN_PAIR*   pin_pair;

    /*  <was_is_descriptor >::=
        (was_is {(pins <pin_reference> <pin_reference> )})
    */

    // none of the pins is ok too
    while( (tok = nextTok()) != T_RIGHT )
    {
                
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_pins:
            // copy the empty one, then fill its copy later thru pin_pair.                
            growth->pin_pairs.push_back( empty );
            pin_pair= &growth->pin_pairs.back();
            
            needSYMBOL();       // readCOMPnPIN() expects 1st token to have been read
            readCOMPnPIN( &pin_pair->was.component_id, &pin_pair->was.pin_id );
            
            needSYMBOL();       // readCOMPnPIN() expects 1st token to have been read
            readCOMPnPIN( &pin_pair->is.component_id, &pin_pair->is.pin_id );
            
            needRIGHT();
            break;
            
        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doROUTE( ROUTE* growth ) throw( IOError )
{
    DSN_T   tok;

    /*  <route_descriptor >::=
        (routes
           <resolution_descriptor>
           <parser_descriptor>
           <structure_out_descriptor>
           <library_out_descriptor>
           <network_out_descriptor>
           <test_points_descriptor>
        )
    */

    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_resolution:
            if( growth->resolution )
                unexpected( tok );
            growth->resolution = new UNIT_RES( growth, tok );
            doRESOLUTION( growth->resolution );
            break;

        case T_parser:
            if( growth->parser )
                unexpected( tok );
            growth->parser = new PARSER( growth );
            doPARSER( growth->parser );
            break;

        case T_structure:
            if( growth->structure )
                unexpected( tok );
            growth->structure = new STRUCTURE( growth );
            doSTRUCTURE( growth->structure );
            break;

        case T_library_out:
            if( growth->library )
                unexpected( tok );
            growth->library = new LIBRARY( growth, tok );
            doLIBRARY( growth->library );
            break;
                    
        case T_network_out:
            while( (tok = nextTok()) != T_RIGHT )
            {
                if( tok != T_LEFT )
                    expecting( T_LEFT );
                
                tok = nextTok();
                if( tok != T_net )      // it is class NET_OUT, but token T_net
                    unexpected( lexer->CurText() );
                
                NET_OUT*    net_out;
                net_out = new NET_OUT( growth );
                
                growth->net_outs.push_back( net_out );
                doNET_OUT( net_out );
            }
            break;

        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doNET_OUT( NET_OUT* growth ) throw( IOError )
{
    DSN_T   tok;

    /*  <net_out_descriptor >::=
        (net <net_id >
          [(net_number <integer >)]
          [<rule_descriptor> ]
          {[<wire_shape_descriptor> | <wire_guide_descriptor> |
             <wire_via_descriptor> | <bond_shape_descriptor> ]}
          {[<supply_pin_descriptor> ]}
        )
    */
    
    needSYMBOL();
    growth->net_id = lexer->CurText();
    
    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_net_number:
            tok = nextTok();
            if( tok!= T_NUMBER )
                expecting( T_NUMBER );
            growth->net_number = atoi( lexer->CurText() );
            needRIGHT();
            break;

        case T_rule:
            if( growth->rules )
                unexpected( tok );
            growth->rules = new RULE( growth, tok );
            doRULE( growth->rules );
            break;

        case T_wire:
            WIRE* wire;
            wire = new WIRE( growth );
            growth->wires.push_back( wire );
            doWIRE( wire );
            break;

        case T_via:
            WIRE_VIA* wire_via;
            wire_via = new WIRE_VIA( growth );
            growth->wire_vias.push_back( wire_via );
            doWIRE_VIA( wire_via );
            break;
                    
        case T_supply_pin:
            SUPPLY_PIN* supply_pin;
            supply_pin = new SUPPLY_PIN( growth );
            growth->supply_pins.push_back( supply_pin );
            doSUPPLY_PIN( supply_pin );
            break;

        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doSUPPLY_PIN( SUPPLY_PIN* growth ) throw( IOError )
{
    DSN_T   tok;
    PIN_REF empty(growth);

    /*  <supply_pin_descriptor >::=
        (supply_pin {<pin_reference> } [(net <net_id >)])
    */
    
    needSYMBOL();
    growth->net_id = lexer->CurText();
    
    while( (tok = nextTok()) != T_RIGHT )
    {
        if( isSymbol(tok) )
        {
            growth->pin_refs.push_back( empty );
            
            PIN_REF*    pin_ref = &growth->pin_refs.back();
            
            readCOMPnPIN( &pin_ref->component_id, &pin_ref->pin_id );
        }
        else if( tok == T_LEFT )
        {
            tok = nextTok();
            if( tok != T_net )
                expecting( T_net );
            growth->net_id = lexer->CurText();
            needRIGHT();
        }
        else
            unexpected( lexer->CurText() );
    }
}


int SPECCTRA_DB::Print( int nestLevel, const char* fmt, ... ) throw( IOError )
{
    va_list     args;

    va_start( args, fmt );
    
    int result = 0;
    int total  = 0;
    
    for( int i=0; i<nestLevel;  ++i )
    {
        result = fprintf( fp, "%*c", NESTWIDTH, ' ' );
        if( result < 0 )
            break;
        
        total += result;
    }
    
    if( result<0 || (result=vfprintf( fp, fmt, args ))<0 )
        ThrowIOError( _("System file error writing to file \"%s\""), filename.GetData() );
    
    va_end( args );
    
    total += result;
    return total;
}


const char* SPECCTRA_DB::GetQuoteChar( const char* wrapee ) 
{
    // I include '#' so a symbol is not confused with a comment.  We intend
    // to wrap any symbol starting with a '#'.
    // Our LEXER class handles comments, and comments appear to be an extension
    // to the SPECCTRA DSN specification. 
    if( *wrapee == '#' )
        return quote_char.c_str();

    if( strlen(wrapee)==0 )
        return quote_char.c_str();
        
    bool    isNumber = true;
    
    for(  ; *wrapee;  ++wrapee )
    {
        // if the string to be wrapped (wrapee) has a delimiter in it, 
        // return the quote_char so caller wraps the wrapee.
        if( strchr( "\t ()", *wrapee ) )
            return quote_char.c_str();
        
        if( !strchr( "01234567890.-+", *wrapee ) )
            isNumber = false;
    }
    
    if( isNumber )
        return quote_char.c_str();
    
    return "";      // can use an unwrapped string.
}


void SPECCTRA_DB::ExportPCB( wxString filename, bool aNameChange ) throw( IOError )
{
    fp = wxFopen( filename, wxT("w") );
    
    if( !fp )
    {
        ThrowIOError( _("Unable to open file \"%s\""), filename.GetData() );  
    }

    if( pcb )
    {
        if( aNameChange )
            pcb->pcbname = CONV_TO_UTF8(filename); 
        
        pcb->Format( this, 0 );
    }

    // if an exception is thrown by Format, then ~SPECCTRA_DB() will close
    // the file.
    
    fclose( fp );
    fp = 0;
}


void SPECCTRA_DB::ExportSESSION( wxString filename )
{
    fp = wxFopen( filename, wxT("w") );
    
    if( !fp )
    {
        ThrowIOError( _("Unable to open file \"%s\""), filename.GetData() );  
    }

    if( session )
        session->Format( this, 0 );    
    
    fclose( fp );
    fp = 0;
}


PCB* SPECCTRA_DB::MakePCB()
{
    PCB*    pcb = new PCB();
    
    pcb->parser = new PARSER( pcb );
    pcb->resolution = new UNIT_RES( pcb, T_resolution );
    pcb->unit = new UNIT_RES( pcb, T_unit );
    
    pcb->structure = new STRUCTURE( pcb );
    pcb->structure->boundary = new BOUNDARY( pcb->structure );
    
    pcb->placement = new PLACEMENT( pcb );
    
    pcb->library = new LIBRARY( pcb );
    
    pcb->network = new NETWORK( pcb );
    
    pcb->wiring = new WIRING( pcb );

    return pcb;
}


//-----<ELEM>---------------------------------------------------------------

ELEM::ELEM( DSN_T aType, ELEM* aParent ) :
   type( aType ),
   parent( aParent )
{
}


ELEM::~ELEM()
{
}


void ELEM::Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
{
    out->Print( nestLevel, "(%s\n", LEXER::GetTokenText( Type() ) ); 

    FormatContents( out, nestLevel+1 );
    
    out->Print( nestLevel, ")\n" ); 
}


void ELEM_HOLDER::FormatContents( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
{
    for( int i=0;  i<Length();  ++i )
    {
        At(i)->Format( out, nestLevel );
    }
}


int ELEM_HOLDER::FindElem( DSN_T aType, int instanceNum )
{
    int repeats=0;
    for( unsigned i=0;  i<kids.size();  ++i )
    {
        if( kids[i].Type() == aType )
        {
            if( repeats == instanceNum )
                return i;
            ++repeats;
        }
    }
    return -1;
}


//-----<PARSER>-----------------------------------------------------------


PARSER::PARSER( ELEM* aParent ) :
    ELEM( T_parser, aParent )
{
    string_quote = '"';
    space_in_quoted_tokens = false;

    case_sensitive = false;
    wires_include_testpoint = false;
    routes_include_testpoint = false;
    routes_include_guides = false;
    routes_include_image_conductor = false;
    via_rotate_first = true;
    generated_by_freeroute = false;
    
    host_cad = "Kicad's PCBNEW";
    host_version = CONV_TO_UTF8(g_BuildVersion);
}


void PARSER::FormatContents( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
{
    out->Print( nestLevel, "(string_quote %c)\n", string_quote );
    out->Print( nestLevel, "(space_in_quoted_tokens %s)\n", space_in_quoted_tokens ? "on" : "off" );
    out->Print( nestLevel, "(host_cad \"%s\")\n", host_cad.c_str() ); 
    out->Print( nestLevel, "(host_version \"%s\")\n", host_version.c_str() );
    
    if( const_id1.length()>0 || const_id2.length()>0 )
        out->Print( nestLevel, "(constant %c%s%c %c%s%c)\n", 
            string_quote, const_id1.c_str(), string_quote,
            string_quote, const_id2.c_str(), string_quote );

    if( routes_include_testpoint || routes_include_guides || routes_include_image_conductor )
        out->Print( nestLevel, "(routes_include%s%s%s)\n",
                   routes_include_testpoint ? " testpoint" : "",
                   routes_include_guides ? " guides" : "",
                   routes_include_image_conductor ? " image_conductor" : "");
    
    if( wires_include_testpoint )
        out->Print( nestLevel, "(wires_include testpoint)\n" );
        
    if( !via_rotate_first )
        out->Print( nestLevel, "(via_rotate_first off)\n" );
    
    out->Print( nestLevel, "(case_sensitive %s)\n", case_sensitive ? "on" : "off" );
}


void PLACE::Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
{
    bool        useMultiLine;
    
    const char* quote = out->GetQuoteChar( component_id.c_str() );

    if( place_rules || properties.size() || lock_type!=T_NONE || rules 
        || region || part_number.size() )
    {
        useMultiLine = true;
        
        out->Print( nestLevel, "(%s %s%s%s\n", LEXER::GetTokenText( Type() ),
                                quote, component_id.c_str(), quote );
    
        out->Print( nestLevel+1, "%s", "" );
    }
    else
    {
        useMultiLine = false;
        
        out->Print( nestLevel, "(%s %s%s%s", LEXER::GetTokenText( Type() ),
                                quote, component_id.c_str(), quote );
    }

    if( hasVertex )
    {
        out->Print( 0, " %.6g %.6g", vertex.x, vertex.y );
    
        out->Print( 0, " %s", LEXER::GetTokenText( side ) );
    
        out->Print( 0, " %.6g", rotation );
    }
    
    if( mirror != T_NONE )
        out->Print( 0, " (mirror %s)", LEXER::GetTokenText( mirror ) );
                   
    if( status != T_NONE )
        out->Print( 0, " (status %s)", LEXER::GetTokenText( status ) );
    
    if( logical_part.size() )
    {
        quote = out->GetQuoteChar( logical_part.c_str() );
        out->Print( 0, " (logical_part %s%s%s)", 
                   quote, logical_part.c_str(), quote );
    }

    if( useMultiLine )
    {
        out->Print( 0, "\n" );
        if( place_rules )
        {
            place_rules->Format( out, nestLevel+1 );
        }
        
        if( properties.size() )
        {
            out->Print( nestLevel+1, "(property \n" );
            
            for( PROPERTIES::const_iterator i = properties.begin();
                i != properties.end();  ++i )
            {
                i->Format( out, nestLevel+2 );
            }
            out->Print( nestLevel+1, ")\n" );
        }
        if( lock_type != T_NONE )
            out->Print( nestLevel+1, "(lock_type %s)\n", 
                       LEXER::GetTokenText(lock_type) );
        if( rules )
            rules->Format( out, nestLevel+1 );
        
        if( region )
            region->Format( out, nestLevel+1 );
        
        if( part_number.size() )
        {
            const char* quote = out->GetQuoteChar( part_number.c_str() );
            out->Print( nestLevel+1, "(PN %s%s%s)\n",
                       quote, part_number.c_str(), quote );
        }
    }
    else
        out->Print( 0, ")\n" );
}


} // namespace DSN


// unit test this source file

#if defined(STANDALONE)

using namespace DSN;


int main( int argc, char** argv )
{
//    wxString    filename( wxT("/tmp/fpcroute/Sample_1sided/demo_1sided.dsn") );
//    wxString    filename( wxT("/tmp/testdesigns/test.dsn") );
    wxString    filename( wxT("/tmp/testdesigns/test.ses") );
//    wxString    filename( wxT("/tmp/specctra_big.dsn") );

    SPECCTRA_DB     db;
    bool            failed = false;
    
    try 
    {
//        db.LoadPCB( filename );
        db.LoadSESSION( filename );
    } 
    catch( IOError ioe )
    {
        printf( "%s\n", CONV_TO_UTF8(ioe.errorText) );
        failed = true;
    }

    if( !failed )    
        printf("loaded OK\n");


    // export what we read in, making this test program basically a beautifier
    db.ExportSESSION( wxT("/tmp/export.ses") );
//    db.ExportPCB( wxT("/tmp/export.dsn") ); 
    
}

#endif


//EOF

