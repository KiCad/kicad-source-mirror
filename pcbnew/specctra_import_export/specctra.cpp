
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2011 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2007-2023 KiCad Developers, see AUTHORS.txt for contributors.
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


/*
 * This source file implements export and import capabilities to the
 * specctra dsn file format.  The grammar for that file format is documented
 * fairly well.  There are classes for each major type of descriptor in the
 * spec.
 *
 * Since there are so many classes in here, it may be helpful to generate
 * the Doxygen directory:
 *
 * $ cd <kicadSourceRoot>
 * $ doxygen
 *
 * Then you can view the html documentation in the <kicadSourceRoot>/doxygen
 * directory.  The main class in this file is SPECCTRA_DB and its main
 * functions are LoadPCB(), LoadSESSION(), and ExportPCB().
 *
 * Wide use is made of boost::ptr_vector<> and std::vector<> template classes.
 * If the contained object is small, then std::vector tends to be used.
 * If the contained object is large, variable size, or would require writing
 * an assignment operator() or copy constructor, then boost::ptr_vector
 * cannot be beat.
 */


#include <cstdarg>
#include <cstdio>

#include <build_version.h>

#include <board.h>
#include <pcb_track.h>

#include "specctra.h"
#include <macros.h>


namespace DSN {

#define NESTWIDTH           2   ///< how many spaces per nestLevel

//-----<SPECCTRA_DB>-------------------------------------------------


const char* GetTokenText( T aTok )
{
    return SPECCTRA_LEXER::TokenName( aTok );
}


void SPECCTRA_DB::buildLayerMaps( BOARD* aBoard )
{
    // specctra wants top physical layer first, then going down to the
    // bottom most physical layer in physical sequence.
    // Same as KiCad now except for B_Cu
    unsigned layerCount = aBoard->GetCopperLayerCount();

    m_layerIds.clear();
    m_pcbLayer2kicad.resize( layerCount );
    m_kicadLayer2pcb.resize( B_Cu + 1 );

#if 0 // was:
    for( int kiNdx = layerCount - 1, pcbNdx=FIRST_LAYER; kiNdx >= 0; --kiNdx, ++pcbNdx )
    {
        int kilayer = (kiNdx>0 && kiNdx==layerCount-1) ? F_Cu : kiNdx;

        // establish bi-directional mapping between KiCad's BOARD layer and PCB layer
        pcbLayer2kicad[pcbNdx]  = kilayer;
        kicadLayer2pcb[kilayer] = pcbNdx;

        // save the specctra layer name in SPECCTRA_DB::layerIds for later.
        layerIds.push_back( TO_UTF8( aBoard->GetLayerName( ToLAYER_ID( kilayer ) ) ) );
    }
#else

    // establish bi-directional mapping between KiCad's BOARD layer and PCB layer

    for( unsigned i = 0; i < m_kicadLayer2pcb.size(); ++i )
    {
        if( i < layerCount-1 )
            m_kicadLayer2pcb[i] = i;
        else
            m_kicadLayer2pcb[i] = layerCount - 1;
    }

    for( unsigned i = 0; i < m_pcbLayer2kicad.size(); ++i )
    {
        PCB_LAYER_ID id = ( i < layerCount-1 ) ? ToLAYER_ID( i ) : B_Cu;

        m_pcbLayer2kicad[i] = id;

        // save the specctra layer name in SPECCTRA_DB::layerIds for later.
        m_layerIds.push_back(TO_UTF8( aBoard->GetLayerName( id ) ) );
    }

#endif
}


int SPECCTRA_DB::findLayerName( const std::string& aLayerName ) const
{
    for( int i = 0; i < int( m_layerIds.size() ); ++i )
    {
        if( 0 == aLayerName.compare( m_layerIds[i] ) )
            return i;
    }

    return -1;
}


void SPECCTRA_DB::readCOMPnPIN( std::string* component_id, std::string* pin_id )
{
    T      tok;

    static const char pin_def[] = "<pin_reference>::=<component_id>-<pin_id>";

    if( !IsSymbol( (T) CurTok() ) )
        Expecting( pin_def );

    // case for:  A12-14, i.e. no wrapping quotes.  This should be a single
    // token, so split it.
    if( CurTok() != T_STRING )
    {
        const char* toktext = CurText();
        const char* dash    = strchr( toktext, '-' );

        if( !dash )
            Expecting( pin_def );

        while( toktext != dash )
            *component_id += *toktext++;

        ++toktext;  // skip the dash

        while( *toktext )
            *pin_id += *toktext++;
    }
    else    // quoted string:  "U12"-"14" or "U12"-14,  3 tokens in either case
    {
        *component_id = CurText();

        tok = NextTok();

        if( tok!=T_DASH )
            Expecting( pin_def );

        NextTok();          // accept anything after the dash.
        *pin_id = CurText();
    }
}


void SPECCTRA_DB::readTIME( time_t* time_stamp )
{
    T     tok;

    struct tm   mytime;

    static const char time_toks[] = "<month> <day> <hour> : <minute> : <second> <year> or <month> <day> <hour>:<minute>:<second> <year>";

    static const char* months[] = {  // index 0 = Jan
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", nullptr
    };

    NeedSYMBOL();       // month

    const char* ptok = CurText();

    mytime.tm_mon = 0;      // remains if we don't find a month match.

    for( int m = 0; months[m]; ++m )
    {
        if( !strcasecmp( months[m], ptok ) )
        {
            mytime.tm_mon = m;
            break;
        }
    }

    tok = NextTok();    // day

    if( tok != T_NUMBER )
        Expecting( time_toks );

    mytime.tm_mday = atoi( CurText() );

    tok = NextTok();    // hour or H:M:S

    if( tok == T_NUMBER )
    {
        mytime.tm_hour = atoi( CurText() );

        // : colon
        NeedSYMBOL();

        if( *CurText() != ':' || strlen( CurText() ) != 1 )
            Expecting( time_toks );

        tok = NextTok(); // minute

        if( tok != T_NUMBER )
            Expecting( time_toks );

        mytime.tm_min = atoi( CurText() );

        // : colon
        NeedSYMBOL();

        if( *CurText() != ':' || strlen( CurText() ) != 1 )
            Expecting( time_toks );

        tok = NextTok(); // second

        if( tok != T_NUMBER )
            Expecting( time_toks );

        mytime.tm_sec = atoi( CurText() );
    }
    else if( tok == T_SYMBOL )
    {
        wxString      str = wxString( CurText() );
        wxArrayString arr = wxSplit( str, ':', '\0' );

        if( arr.size() != 3 )
            Expecting( time_toks );

        mytime.tm_hour = wxAtoi( arr[0] );
        mytime.tm_min = wxAtoi( arr[1] );
        mytime.tm_sec = wxAtoi( arr[2] );
    }

    tok = NextTok();    // year

    if( tok != T_NUMBER )
        Expecting( time_toks );

    mytime.tm_year = atoi( CurText() ) - 1900;
    mytime.tm_isdst = 0;   // useless param here.

    *time_stamp = mktime( &mytime );
}


void SPECCTRA_DB::LoadPCB( const wxString& aFilename )
{
    FILE_LINE_READER curr_reader( aFilename );

    PushReader( &curr_reader );

    if( NextTok() != T_LEFT )
        Expecting( T_LEFT );

    if( NextTok() != T_pcb )
        Expecting( T_pcb );

    SetPCB( new PCB() );

    doPCB( m_pcb );
    PopReader();
}


void SPECCTRA_DB::LoadSESSION( const wxString& aFilename )
{
    FILE_LINE_READER curr_reader( aFilename );

    PushReader( &curr_reader );

    if( NextTok() != T_LEFT )
        Expecting( T_LEFT );

    if( NextTok() != T_session )
        Expecting( T_session );

    SetSESSION( new SESSION() );

    doSESSION( m_session );

    PopReader();
}


void SPECCTRA_DB::doPCB( PCB* growth )
{
    T     tok;

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

    NeedSYMBOL();
    growth->m_pcbname = CurText();

    while( (tok = NextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_parser:
            if( growth->m_parser )
                Unexpected( tok );

            growth->m_parser = new PARSER( growth );
            doPARSER( growth->m_parser );
            break;

        case T_unit:
            if( growth->m_unit )
                Unexpected( tok );

            growth->m_unit = new UNIT_RES( growth, tok );
            doUNIT( growth->m_unit );
            break;

        case T_resolution:
            if( growth->m_resolution )
                Unexpected( tok );

            growth->m_resolution = new UNIT_RES( growth, tok );
            doRESOLUTION( growth->m_resolution );
            break;

        case T_structure:
            if( growth->m_structure )
                Unexpected( tok );

            growth->m_structure = new STRUCTURE( growth );
            doSTRUCTURE( growth->m_structure );
            break;

        case T_placement:
            if( growth->m_placement )
                Unexpected( tok );

            growth->m_placement = new PLACEMENT( growth );
            doPLACEMENT( growth->m_placement );
            break;

        case T_library:
            if( growth->m_library )
                Unexpected( tok );

            growth->m_library = new LIBRARY( growth );
            doLIBRARY( growth->m_library );
            break;

        case T_network:
            if( growth->m_network )
                Unexpected( tok );

            growth->m_network = new NETWORK( growth );
            doNETWORK( growth->m_network );
            break;

        case T_wiring:
            if( growth->m_wiring )
                Unexpected( tok );

            growth->m_wiring = new WIRING( growth );
            doWIRING( growth->m_wiring );
            break;

        default:
            Unexpected( CurText() );
        }
    }

    tok = NextTok();

    if( tok != T_EOF )
        Expecting( T_EOF );
}


void SPECCTRA_DB::doPARSER( PARSER* growth )
{
    T           tok;
    std::string const1;
    std::string const2;

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

    while( (tok = NextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_STRING_QUOTE:
            tok = NextTok();

            if( tok != T_QUOTE_DEF )
                Expecting( T_QUOTE_DEF );

            SetStringDelimiter( (unsigned char) *CurText() );
            growth->string_quote = *CurText();
            m_quote_char = CurText();
            NeedRIGHT();
            break;

        case T_space_in_quoted_tokens:
            tok = NextTok();

            if( tok!=T_on && tok!=T_off )
                Expecting( "on|off" );

            SetSpaceInQuotedTokens( tok==T_on );
            growth->space_in_quoted_tokens = (tok==T_on);
            NeedRIGHT();
            break;

        case T_host_cad:
            NeedSYMBOL();
            growth->host_cad = CurText();
            NeedRIGHT();
            break;

        case T_host_version:
            NeedSYMBOLorNUMBER();
            growth->host_version = CurText();
            NeedRIGHT();
            break;

        case T_constant:
            NeedSYMBOLorNUMBER();
            const1 = CurText();
            NeedSYMBOLorNUMBER();
            const2 = CurText();
            NeedRIGHT();
            growth->constants.push_back( const1 );
            growth->constants.push_back( const2 );
            break;

        case T_write_resolution:   // [(writee_resolution {<character> <positive_integer >})]
            while( (tok = NextTok()) != T_RIGHT )
            {
                if( tok!=T_SYMBOL )
                    Expecting( T_SYMBOL );

                tok = NextTok();

                if( tok!=T_NUMBER )
                    Expecting( T_NUMBER );

                // @todo
            }

            break;

        case T_routes_include:  // [(routes_include {[testpoint | guides | image_conductor]})]
            while( (tok = NextTok()) != T_RIGHT )
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
                    Expecting( "testpoint|guides|image_conductor" );
                }
            }

            break;

        case T_wires_include:   // [(wires_include testpoint)]
            tok = NextTok();

            if( tok != T_testpoint )
                Expecting( T_testpoint );

            growth->routes_include_testpoint = true;
            NeedRIGHT();
            break;

        case T_case_sensitive:
            tok = NextTok();

            if( tok!=T_on && tok!=T_off )
                Expecting( "on|off" );

            growth->case_sensitive = (tok==T_on);
            NeedRIGHT();
            break;

        case T_via_rotate_first:    // [(via_rotate_first [on | off])]
            tok = NextTok();

            if( tok!=T_on && tok!=T_off )
                Expecting( "on|off" );

            growth->via_rotate_first = (tok==T_on);
            NeedRIGHT();
            break;

        case T_generated_by_freeroute:
            growth->generated_by_freeroute = true;
            NeedRIGHT();
            break;

        default:
            Unexpected( CurText() );
        }
    }
}


void SPECCTRA_DB::doRESOLUTION( UNIT_RES* growth )
{
    NextTok();
    wxString str = wxString( CurText() ).MakeLower();

    if( str == wxS( "inch" ) )
        growth->units = T_inch;
    else if( str == wxS( "mil" ) )
        growth->units = T_mil;
    else if( str == wxS( "cm" ) )
        growth->units = T_cm;
    else if( str == wxS( "mm" ) )
        growth->units = T_mm;
    else if( str == wxS( "um" ) )
        growth->units = T_um;
    else
        Expecting( "inch|mil|cm|mm|um" );

    T tok = NextTok();

    if( tok != T_NUMBER )
        Expecting( T_NUMBER );

    growth->value = atoi( CurText() );

    NeedRIGHT();
}


void SPECCTRA_DB::doUNIT( UNIT_RES* growth )
{
    T       tok = NextTok();

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
        Expecting( "inch|mil|cm|mm|um" );
    }

    NeedRIGHT();
}


void SPECCTRA_DB::doSPECCTRA_LAYER_PAIR( SPECCTRA_LAYER_PAIR* growth )
{
    NeedSYMBOL();
    growth->layer_id0 = CurText();

    NeedSYMBOL();
    growth->layer_id1 = CurText();

    if( NextTok() != T_NUMBER )
        Expecting( T_NUMBER );

    growth->layer_weight = strtod( CurText(), 0 );

    NeedRIGHT();
}


void SPECCTRA_DB::doLAYER_NOISE_WEIGHT( LAYER_NOISE_WEIGHT* growth )

{
    T       tok;

    while( ( tok = NextTok() ) != T_RIGHT )
    {
        if( tok != T_LEFT )
            Expecting( T_LEFT );

        if( NextTok() != T_layer_pair )
            Expecting( T_layer_pair );

        SPECCTRA_LAYER_PAIR* layer_pair = new SPECCTRA_LAYER_PAIR( growth );
        growth->layer_pairs.push_back( layer_pair );
        doSPECCTRA_LAYER_PAIR( layer_pair );
    }
}


void SPECCTRA_DB::doSTRUCTURE( STRUCTURE* growth )
{
    T       tok;

    while( ( tok = NextTok() ) != T_RIGHT )
    {
        if( tok != T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_unit:
            if( growth->m_unit )
                Unexpected( tok );

            growth->m_unit = new UNIT_RES( growth, tok );
            doUNIT( growth->m_unit );
            break;

        case T_resolution:
            if( growth->m_unit )
                Unexpected( tok );

            growth->m_unit = new UNIT_RES( growth, tok );
            doRESOLUTION( growth->m_unit );
            break;

        case T_layer_noise_weight:
            if( growth->m_layer_noise_weight )
                Unexpected( tok );

            growth->m_layer_noise_weight = new LAYER_NOISE_WEIGHT( growth );
            doLAYER_NOISE_WEIGHT( growth->m_layer_noise_weight );
            break;

        case T_place_boundary:
L_place:
            if( growth->m_place_boundary )
                Unexpected( tok );

            growth->m_place_boundary = new BOUNDARY( growth, T_place_boundary );
            doBOUNDARY( growth->m_place_boundary );
            break;

        case T_boundary:
            if( growth->m_boundary )
            {
                if( growth->m_place_boundary )
                    Unexpected( tok );

                goto L_place;
            }

            growth->m_boundary = new BOUNDARY( growth );
            doBOUNDARY( growth->m_boundary );
            break;

        case T_plane:
            COPPER_PLANE* plane;
            plane = new COPPER_PLANE( growth );
            growth->m_planes.push_back( plane );
            doKEEPOUT( plane );
            break;

        case T_region:
            REGION* region;
            region = new REGION( growth );
            growth->m_regions.push_back( region );
            doREGION( region );
            break;

        case T_snap_angle:
            STRINGPROP* stringprop;
            stringprop = new STRINGPROP( growth, T_snap_angle );
            growth->Append( stringprop );
            doSTRINGPROP( stringprop );
            break;

        case T_via:
            if( growth->m_via )
                Unexpected( tok );

            growth->m_via = new VIA( growth );
            doVIA( growth->m_via );
            break;

        case T_control:
            if( growth->m_control )
                Unexpected( tok );

            growth->m_control = new CONTROL( growth );
            doCONTROL( growth->m_control );
            break;

        case T_layer:
            LAYER* layer;
            layer = new LAYER( growth );
            growth->m_layers.push_back( layer );
            doLAYER( layer );
            break;

        case T_rule:
            if( growth->m_rules )
                Unexpected( tok );

            growth->m_rules = new RULE( growth, T_rule );
            doRULE( growth->m_rules );
            break;

        case T_place_rule:
            if( growth->m_place_rules )
                Unexpected( tok );

            growth->m_place_rules = new RULE( growth, T_place_rule );
            doRULE( growth->m_place_rules );
            break;

        case T_keepout:
        case T_place_keepout:
        case T_via_keepout:
        case T_wire_keepout:
        case T_bend_keepout:
        case T_elongate_keepout:
            KEEPOUT* keepout;
            keepout = new KEEPOUT( growth, tok );
            growth->m_keepouts.push_back( keepout );
            doKEEPOUT( keepout );
            break;

        case T_grid:
            GRID* grid;
            grid = new GRID( growth );
            growth->m_grids.push_back( grid );
            doGRID( grid );
            break;

        default:
            Unexpected( CurText() );
        }
    }
}


void SPECCTRA_DB::doSTRUCTURE_OUT( STRUCTURE_OUT* growth )
{
    /*
    <structure_out_descriptor >::=
        (structure_out
            {<layer_descriptor> }
            [<rule_descriptor> ]
        )
    */

    T       tok = NextTok();

    while( tok != T_RIGHT )
    {
        if( tok != T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_layer:
            LAYER* layer;
            layer = new LAYER( growth );
            growth->m_layers.push_back( layer );
            doLAYER( layer );
            break;

        case T_rule:
            if( growth->m_rules )
                Unexpected( tok );

            growth->m_rules = new RULE( growth, T_rule );
            doRULE( growth->m_rules );
            break;

        default:
            Unexpected( CurText() );
        }

        tok = NextTok();
    }
}


void SPECCTRA_DB::doKEEPOUT( KEEPOUT* growth )
{
    T       tok = NextTok();

    if( IsSymbol(tok) )
    {
        growth->m_name = CurText();
        tok = NextTok();
    }

    if( tok!=T_LEFT )
        Expecting( T_LEFT );

    while( tok != T_RIGHT )
    {
        if( tok!=T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_sequence_number:
            if( NextTok() != T_NUMBER )
                Expecting( T_NUMBER );

            growth->m_sequence_number = atoi( CurText() );
            NeedRIGHT();
            break;

        case T_rule:
            if( growth->m_rules )
                Unexpected( tok );

            growth->m_rules = new RULE( growth, T_rule );
            doRULE( growth->m_rules );
            break;

        case T_place_rule:
            if( growth->m_place_rules )
                Unexpected( tok );

            growth->m_place_rules = new RULE( growth, T_place_rule );
            doRULE( growth->m_place_rules );
            break;

        case T_rect:
            if( growth->m_shape )
                Unexpected( tok );

            growth->m_shape = new RECTANGLE( growth );
            doRECTANGLE( (RECTANGLE*) growth->m_shape );
            break;

        case T_circle:
            if( growth->m_shape )
                Unexpected( tok );

            growth->m_shape = new CIRCLE( growth );
            doCIRCLE( (CIRCLE*) growth->m_shape );
            break;

        case T_polyline_path:
            tok = T_path;
            KI_FALLTHROUGH;

        case T_path:
        case T_polygon:
            if( growth->m_shape )
                Unexpected( tok );

            growth->m_shape = new PATH( growth, tok );
            doPATH( (PATH*) growth->m_shape );
            break;

        case T_qarc:
            if( growth->m_shape )
                Unexpected( tok );

            growth->m_shape = new QARC( growth );
            doQARC( (QARC*) growth->m_shape );
            break;

        case T_window:
            WINDOW* window;
            window = new WINDOW( growth );
            growth->m_windows.push_back( window );
            doWINDOW( window );
            break;

        default:
            Unexpected( CurText() );
        }

        tok = NextTok();
    }
}


void SPECCTRA_DB::doCONNECT( CONNECT* growth )
{
    /*  from page 143 of specctra spec:

        (connect
            {(terminal <object_type> [<pin_reference> ])}
        )
    */

    T       tok = NextTok();

    while( tok != T_RIGHT )
    {
        if( tok!=T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_terminal:
            // since we do not use the terminal information, simply toss it.
            while( ( tok = NextTok() ) != T_RIGHT && tok != T_EOF )
                ;
            break;

        default:
            Unexpected( CurText() );
        }

        tok = NextTok();
    }
}


void SPECCTRA_DB::doWINDOW( WINDOW* growth )
{
    T       tok = NextTok();

    while( tok != T_RIGHT )
    {
        if( tok!=T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_rect:
            if( growth->shape )
                Unexpected( tok );

            growth->shape = new RECTANGLE( growth );
            doRECTANGLE( (RECTANGLE*) growth->shape );
            break;

        case T_circle:
            if( growth->shape )
                Unexpected( tok );

            growth->shape = new CIRCLE( growth );
            doCIRCLE( (CIRCLE*) growth->shape );
            break;

        case T_polyline_path:
            tok = T_path;
            KI_FALLTHROUGH;

        case T_path:
        case T_polygon:
            if( growth->shape )
                Unexpected( tok );

            growth->shape = new PATH( growth, tok );
            doPATH( (PATH*) growth->shape );
            break;

        case T_qarc:
            if( growth->shape )
                Unexpected( tok );

            growth->shape = new QARC( growth );
            doQARC( (QARC*) growth->shape );
            break;

        default:
            Unexpected( CurText() );
        }

        tok = NextTok();
    }
}


void SPECCTRA_DB::doBOUNDARY( BOUNDARY* growth )
{
    T       tok = NextTok();

    if( tok != T_LEFT )
        Expecting( T_LEFT );

    tok = NextTok();

    if( tok == T_rect )
    {
        if( growth->paths.size() )
            Unexpected( "rect when path already encountered" );

        growth->rectangle = new RECTANGLE( growth );
        doRECTANGLE( growth->rectangle );
        NeedRIGHT();
    }
    else if( tok == T_path )
    {
        if( growth->rectangle )
            Unexpected( "path when rect already encountered" );

        for(;;)
        {
            if( tok != T_path )
                Expecting( T_path );

            PATH* path = new PATH( growth, T_path );
            growth->paths.push_back( path );

            doPATH( path );

            tok = NextTok();
            if( tok == T_RIGHT )
                break;

            if( tok != T_LEFT )
                Expecting(T_LEFT);

            tok = NextTok();
        }
    }
    else
    {
        Expecting( "rect|path" );
    }
}


void SPECCTRA_DB::doPATH( PATH* growth )
{
    T tok = NextTok();

    if( !IsSymbol( tok ) && tok != T_NUMBER )   // a layer name can be like a number like +12
        Expecting( "layer_id" );

    growth->layer_id = CurText();

    if( NextTok() != T_NUMBER )
        Expecting( "aperture_width" );

    growth->aperture_width = strtod( CurText(), nullptr );

    POINT   ptTemp;

    tok = NextTok();

    do
    {
        if( tok != T_NUMBER )
            Expecting( T_NUMBER );

        ptTemp.x = strtod( CurText(), nullptr );

        if( NextTok() != T_NUMBER )
            Expecting( T_NUMBER );

        ptTemp.y = strtod( CurText(), nullptr );

        growth->points.push_back( ptTemp );

    } while( ( tok = NextTok() ) != T_RIGHT && tok != T_LEFT );

    if( tok == T_LEFT )
    {
        if( NextTok() != T_aperture_type )
            Expecting( T_aperture_type );

        tok = NextTok();

        if( tok!=T_round && tok!=T_square )
            Expecting( "round|square" );

        growth->aperture_type = tok;

        NeedRIGHT();
    }
}


void SPECCTRA_DB::doRECTANGLE( RECTANGLE* growth )
{
    NeedSYMBOL();
    growth->layer_id = CurText();

    if( NextTok() != T_NUMBER )
        Expecting( T_NUMBER );

    growth->point0.x = strtod( CurText(), nullptr );

    if( NextTok() != T_NUMBER )
        Expecting( T_NUMBER );

    growth->point0.y = strtod( CurText(), nullptr );

    if( NextTok() != T_NUMBER )
        Expecting( T_NUMBER );

    growth->point1.x = strtod( CurText(), nullptr );

    if( NextTok() != T_NUMBER )
        Expecting( T_NUMBER );

    growth->point1.y = strtod( CurText(), nullptr );

    NeedRIGHT();
}


void SPECCTRA_DB::doCIRCLE( CIRCLE* growth )
{
    T       tok;

    NeedSYMBOLorNUMBER();
    growth->layer_id = CurText();

    if( NextTok() != T_NUMBER )
        Expecting( T_NUMBER );

    growth->diameter = strtod( CurText(), 0 );

    tok = NextTok();

    if( tok == T_NUMBER )
    {
        growth->vertex.x = strtod( CurText(), 0 );

        if( NextTok() != T_NUMBER )
            Expecting( T_NUMBER );

        growth->vertex.y = strtod( CurText(), 0 );

        tok = NextTok();
    }

    if( tok != T_RIGHT )
        Expecting( T_RIGHT );
}


void SPECCTRA_DB::doQARC( QARC* growth )
{
    NeedSYMBOL();
    growth->layer_id = CurText();

    if( NextTok() != T_NUMBER )
        Expecting( T_NUMBER );

    growth->aperture_width = strtod( CurText(), 0 );

    for( int i = 0; i < 3; ++i )
    {
        if( NextTok() != T_NUMBER )
            Expecting( T_NUMBER );

        growth->vertex[i].x = strtod( CurText(), 0 );

        if( NextTok() != T_NUMBER )
            Expecting( T_NUMBER );

        growth->vertex[i].y = strtod( CurText(), 0 );
    }

    NeedRIGHT();
}


void SPECCTRA_DB::doSTRINGPROP( STRINGPROP* growth )
{
    NeedSYMBOL();
    growth->value = CurText();
    NeedRIGHT();
}


void SPECCTRA_DB::doTOKPROP( TOKPROP* growth )
{
    T     tok = NextTok();

    if( tok<0 )
        Unexpected( CurText() );

    growth->value = tok;

    NeedRIGHT();
}


void SPECCTRA_DB::doVIA( VIA* growth )
{
    T       tok;

    while( ( tok = NextTok() ) != T_RIGHT )
    {
        if( tok == T_LEFT )
        {
            if( NextTok() != T_spare )
                Expecting( T_spare );

            while( (tok = NextTok()) != T_RIGHT )
            {
                if( !IsSymbol( tok ) )
                    Expecting( T_SYMBOL );

                growth->m_spares.push_back( CurText() );
            }
        }
        else if( IsSymbol( tok ) )
        {
            growth->m_padstacks.push_back( CurText() );
        }
        else
        {
            Unexpected( CurText() );
        }
    }
}


void SPECCTRA_DB::doCONTROL( CONTROL* growth )
{
    T       tok;

    while( (tok = NextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_via_at_smd:
            tok = NextTok();

            if( tok!=T_on && tok!=T_off )
                Expecting( "on|off" );

            growth->via_at_smd = (tok==T_on);
            NeedRIGHT();
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
            tokprop = new TOKPROP( growth, tok );
            growth->Append( tokprop );
            doTOKPROP( tokprop );
            break;

        default:
            Unexpected( CurText() );
        }
    }
}


void SPECCTRA_DB::doPROPERTIES( PROPERTIES* growth )
{
    T           tok;
    PROPERTY    property;  // construct it once here, append multiple times.

    while( ( tok = NextTok() ) != T_RIGHT )
    {
        if( tok != T_LEFT )
            Expecting( T_LEFT );

        NeedSYMBOLorNUMBER();
        property.name = CurText();

        NeedSYMBOLorNUMBER();
        property.value = CurText();

        growth->push_back( property );

        NeedRIGHT();
    }
}


void SPECCTRA_DB::doLAYER( LAYER* growth )
{
    T       tok = NextTok();

    if( !IsSymbol( tok ) )
        Expecting( T_SYMBOL );

    growth->name = CurText();

    while( ( tok = NextTok() ) != T_RIGHT )
    {
        if( tok != T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_type:
            tok = NextTok();

            if( tok != T_signal && tok != T_power && tok != T_mixed && tok != T_jumper )
                Expecting( "signal|power|mixed|jumper" );

            growth->layer_type = tok;

            if( NextTok()!=T_RIGHT )
                Expecting(T_RIGHT);

            break;

        case T_rule:
            growth->rules = new RULE( growth, T_rule );
            doRULE( growth->rules );
            break;

        case T_property:
            doPROPERTIES( &growth->properties );
            break;

        case T_direction:
            tok = NextTok();

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
                if( !strcmp( "hori", CurText() ) )
                {
                    growth->direction = T_horizontal;
                    break;
                }
                else if( !strcmp( "vert", CurText() ) )
                {
                    growth->direction = T_vertical;
                    break;
                }

                Expecting( "horizontal|vertical|orthogonal|positive_diagonal|negative_diagonal|"
                           "diagonal|off" );
            }

            if( NextTok() != T_RIGHT )
                Expecting( T_RIGHT );

            break;

        case T_cost:
            tok = NextTok();

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
                // T     (positive) and T_NUMBER (negative)
                growth->cost = -atoi( CurText() );
                break;
            default:
                Expecting( "forbidden|high|medium|low|free|<positive_integer>|-1" );
            }

            tok = NextTok();

            if( tok == T_LEFT )
            {
                if( NextTok() != T_type )
                    Unexpected( CurText() );

                tok = NextTok();

                if( tok!=T_length && tok!=T_way )
                    Expecting( "length|way" );

                growth->cost_type = tok;

                if( NextTok()!=T_RIGHT )
                    Expecting( T_RIGHT );

                tok = NextTok();
            }

            if( tok != T_RIGHT )
                Expecting( T_RIGHT );

            break;

        case T_use_net:
            while( ( tok = NextTok() ) != T_RIGHT )
            {
                if( !IsSymbol( tok ) )
                    Expecting( T_SYMBOL );

                growth->use_net.push_back( CurText() );
            }

            break;

        default:
            Unexpected( CurText() );
        }
    }
}


void SPECCTRA_DB::doRULE( RULE* growth )
{
    std::string     builder;
    int             bracketNesting = 1; // we already saw the opening T_LEFT
    T               tok = T_NONE;

    while( bracketNesting != 0 && tok != T_EOF )
    {
        tok = NextTok();

        if( tok==T_LEFT)
            ++bracketNesting;
        else if( tok==T_RIGHT )
            --bracketNesting;

        if( bracketNesting >= 1 )
        {
            if( PrevTok() != T_LEFT && tok != T_RIGHT && ( tok != T_LEFT || bracketNesting > 2 ) )
                builder += ' ';

            if( tok == T_STRING )
                builder += m_quote_char;

            builder += CurText();

            if( tok == T_STRING )
                builder += m_quote_char;
        }

        // When the nested rule is closed with a T_RIGHT and we are back down
        // to bracketNesting == 1, (inside the <rule_descriptor> but outside
        // the last rule).  Then save the last rule and clear the string builder.
        if( bracketNesting == 1 )
        {
           growth->m_rules.push_back( builder );
           builder.clear();
        }
    }

    if( tok==T_EOF )
        Unexpected( T_EOF );
}


#if 0
void SPECCTRA_DB::doPLACE_RULE( PLACE_RULE* growth, bool expect_object_type )
{
    /*   (place_rule [<structure_place_rule_object> ]
         {[<spacing_descriptor> |
         <permit_orient_descriptor> |
         <permit_side_descriptor> |
         <opposite_side_descriptor> ]}
         )
    */

    T       tok = NextTok();

    if( tok != T_LEFT )
        Expecting( T_LEFT );

    tok = NextTok();

    if( tok == T_object_type )
    {
        if( !expect_object_type )
            Unexpected( tok );

        /*  [(object_type
              [pcb |
              image_set [large | small | discrete | capacitor | resistor]
              [(image_type [smd | pin])]]
            )]
        */

        tok = NextTok();

        switch( tok )
        {
        case T_pcb:
            growth->object_type = tok;
            break;

        case T_image_set:
            tok = NextTok();

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
                Unexpected( CurText() );
            }

            break;

        default:
            Unexpected( CurText() );
        }

        tok = NextTok();

        if( tok == T_LEFT )
        {
            tok = NextTok();

            if( tok != T_image_type )
                Expecting( T_image_type );

            tok = NextTok();

            if( tok!=T_smd && tok!=T_pin )
                Expecting( "smd|pin" );

            NeedRIGHT();

            tok = NextTok();
        }

        if( tok != T_RIGHT )
            Expecting( T_RIGHT );

        tok = NextTok();
    }

    /*  {[<spacing_descriptor> |
        <permit_orient_descriptor> |
        <permit_side_descriptor> | <opposite_side_descriptor> ]}
    */
    doRULE( growth );
}
#endif


void SPECCTRA_DB::doREGION( REGION* growth )
{
    T     tok = NextTok();

    if( IsSymbol( tok ) )
    {
        growth->m_region_id = CurText();
        tok = NextTok();
    }

    for(;;)
    {
        if( tok != T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_rect:
            if( growth->m_rectangle )
                Unexpected( tok );

            growth->m_rectangle = new RECTANGLE( growth );
            doRECTANGLE( growth->m_rectangle );
            break;

        case T_polygon:
            if( growth->m_polygon )
                Unexpected( tok );

            growth->m_polygon = new PATH( growth, T_polygon );
            doPATH( growth->m_polygon );
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
            if( growth->m_rules )
                Unexpected( tok );

            growth->m_rules = new RULE( growth, T_rule );
            doRULE( growth->m_rules );
            break;

        default:
            Unexpected( CurText() );
        }

        tok = NextTok();

        if( tok == T_RIGHT )
        {
            if( !growth->m_rules )
                Expecting( T_rule );

            break;
        }
    }
}


void SPECCTRA_DB::doCLASS_CLASS( CLASS_CLASS* growth )
{
    T       tok = NextTok();

    if( tok != T_LEFT )
        Expecting( T_LEFT );

    while( ( tok = NextTok() ) != T_RIGHT )
    {
        switch( tok )
        {
        case T_classes:
            if( growth->classes )
                Unexpected( tok );

            growth->classes = new CLASSES( growth );
            doCLASSES( growth->classes );
            break;

        case T_rule:
            // only T_class_class takes a T_rule
            if( growth->Type() == T_region_class_class )
                Unexpected( tok );

            RULE* rule;
            rule = new RULE( growth, T_rule );
            growth->Append( rule );
            doRULE( rule );
            break;

        case T_layer_rule:
            // only T_class_class takes a T_layer_rule
            if( growth->Type() == T_region_class_class )
                Unexpected( tok );

            LAYER_RULE* layer_rule;
            layer_rule = new LAYER_RULE( growth );
            growth->Append( layer_rule );
            doLAYER_RULE( layer_rule );
            break;

        default:
            Unexpected( tok );
        }
    }
}


void SPECCTRA_DB::doCLASSES( CLASSES* growth )
{
    T       tok = NextTok();

    // require at least 2 class_ids

    if( !IsSymbol( tok ) )
        Expecting( "class_id" );

    growth->class_ids.push_back( CurText() );

    do
    {
        tok = NextTok();

        if( !IsSymbol( tok ) )
            Expecting( "class_id" );

        growth->class_ids.push_back( CurText() );

    } while( ( tok = NextTok() ) != T_RIGHT );
}


void SPECCTRA_DB::doGRID( GRID* growth )
{
    T       tok = NextTok();

    switch( tok )
    {
    case T_via:
    case T_wire:
    case T_via_keepout:
    case T_snap:
    case T_place:
        growth->m_grid_type = tok;

        if( NextTok() != T_NUMBER )
            Expecting( T_NUMBER );

        growth->m_dimension = strtod( CurText(), 0 );
        tok = NextTok();

        if( tok == T_LEFT )
        {
            while( ( tok = NextTok() ) != T_RIGHT )
            {
                if( tok == T_direction )
                {
                    if( growth->m_grid_type == T_place )
                        Unexpected( tok );

                    tok = NextTok();

                    if( tok != T_x && tok != T_y )
                        Unexpected( CurText() );

                    growth->m_direction = tok;

                    if( NextTok() != T_RIGHT )
                        Expecting(T_RIGHT);
                }
                else if( tok == T_offset )
                {
                    if( growth->m_grid_type == T_place )
                        Unexpected( tok );

                    if( NextTok() != T_NUMBER )
                        Expecting( T_NUMBER );

                    growth->m_offset = strtod( CurText(), 0 );

                    if( NextTok() != T_RIGHT )
                        Expecting( T_RIGHT );
                }
                else if( tok == T_image_type )
                {
                    if( growth->m_grid_type != T_place )
                        Unexpected( tok );

                    tok = NextTok();

                    if( tok != T_smd && tok != T_pin )
                        Unexpected( CurText() );

                    growth->m_image_type = tok;

                    if( NextTok() != T_RIGHT )
                        Expecting( T_RIGHT );
                }
            }
        }

        break;

    default:
        Unexpected( tok );
    }
}


void SPECCTRA_DB::doLAYER_RULE( LAYER_RULE* growth )
{
    T       tok;

    NeedSYMBOL();

    do
    {
        growth->m_layer_ids.push_back( CurText() );

    } while( IsSymbol( tok = NextTok() ) );

    if( tok != T_LEFT )
        Expecting( T_LEFT );

    if( NextTok() != T_rule )
        Expecting( T_rule );

    growth->m_rule = new RULE( growth, T_rule );
    doRULE( growth->m_rule );

    NeedRIGHT();
}


void SPECCTRA_DB::doPLACE( PLACE* growth )
{
    T       tok = NextTok();

    if( !IsSymbol( tok ) )
        Expecting( "component_id" );

    growth->m_component_id = CurText();

    tok = NextTok();

    if( tok == T_NUMBER )
    {
        POINT   point;

        point.x = strtod( CurText(), 0 );

        if( NextTok() != T_NUMBER )
            Expecting( T_NUMBER );

        point.y = strtod( CurText(), 0 );

        growth->SetVertex( point );

        tok = NextTok();

        if( tok != T_front && tok != T_back )
            Expecting( "front|back" );

        growth->m_side = tok;

        if( NextTok() != T_NUMBER )
            Expecting( "rotation" );

        growth->SetRotation( strtod( CurText(), 0 ) );
    }

    while( ( tok = NextTok() ) != T_RIGHT )
    {
        if( tok != T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_mirror:
            tok = NextTok();

            if( tok == T_x || tok == T_y || tok == T_xy || tok == T_off )
                growth->m_mirror = tok;
            else
                Expecting( "x|y|xy|off" );

            break;

        case T_status:
            tok = NextTok();

            if( tok==T_added || tok==T_deleted || tok==T_substituted )
                growth->m_status = tok;
            else
                Expecting("added|deleted|substituted");

            break;

        case T_logical_part:
            if( growth->m_logical_part.size() )
                Unexpected( tok );

            tok = NextTok();

            if( !IsSymbol( tok ) )
                Expecting( "logical_part_id");

            growth->m_logical_part = CurText();
            break;

        case T_place_rule:
            if( growth->m_place_rules )
                Unexpected( tok );

            growth->m_place_rules = new RULE( growth, T_place_rule );
            doRULE( growth->m_place_rules );
            break;

        case T_property:
            if( growth->m_properties.size() )
                Unexpected( tok );

            doPROPERTIES( &growth->m_properties );
            break;

        case T_lock_type:
            tok = NextTok();

            if( tok == T_position || tok == T_gate || tok == T_subgate || tok == T_pin )
                growth->m_lock_type = tok;
            else
                Expecting( "position|gate|subgate|pin" );

            break;

        case T_rule:
            if( growth->m_rules || growth->m_region )
                Unexpected( tok );

            growth->m_rules = new RULE( growth, T_rule );
            doRULE( growth->m_rules );
            break;

        case T_region:
            if( growth->m_rules || growth->m_region )
                Unexpected( tok );

            growth->m_region = new REGION( growth );
            doREGION( growth->m_region );
            break;

        case T_pn:
            if( growth->m_part_number.size() )
                Unexpected( tok );

            NeedSYMBOLorNUMBER();
            growth->m_part_number = CurText();
            NeedRIGHT();
            break;

        default:
            Unexpected( tok );
        }
    }
}


void SPECCTRA_DB::doCOMPONENT( COMPONENT* growth )
{
    T       tok = NextTok();

    if( !IsSymbol( tok ) && tok != T_NUMBER )
        Expecting( "image_id" );

    growth->m_image_id = CurText();

    while( ( tok = NextTok() ) != T_RIGHT )
    {
        if( tok != T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_place:
            PLACE* place;
            place = new PLACE( growth );
            growth->m_places.push_back( place );
            doPLACE( place );
            break;

        default:
            Unexpected( tok );
        }
    }
}


void SPECCTRA_DB::doPLACEMENT( PLACEMENT* growth )
{
    T       tok;

    while( ( tok = NextTok() ) != T_RIGHT )
    {
        if( tok == T_EOF )
            Unexpected( T_EOF );

        if( tok != T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_unit:
        case T_resolution:
            growth->m_unit = new UNIT_RES( growth, tok );

            if( tok == T_resolution )
                doRESOLUTION( growth->m_unit );
            else
                doUNIT( growth->m_unit );
            break;

        case T_place_control:
            NeedRIGHT();
            tok = NextTok();

            if( tok != T_flip_style )
                Expecting( T_flip_style );

            tok = NextTok();

            if( tok == T_mirror_first || tok == T_rotate_first )
                growth->m_flip_style = tok;
            else
                Expecting( "mirror_first|rotate_first" );

            NeedRIGHT();
            NeedRIGHT();
            break;

        case T_component:
            COMPONENT* component;
            component = new COMPONENT( growth );
            growth->m_components.push_back( component );
            doCOMPONENT( component );
            break;

        default:
            Unexpected( tok );
        }
    }
}


void SPECCTRA_DB::doPADSTACK( PADSTACK* growth )
{
    T       tok = NextTok();

    /*  (padstack <m_padstack_id >
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

    // m_padstack_id may be a number
    if( !IsSymbol( tok ) && tok != T_NUMBER )
        Expecting( "m_padstack_id" );

    growth->m_padstack_id = CurText();

    while( ( tok = NextTok() ) != T_RIGHT )
    {
        if( tok != T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_unit:
            if( growth->m_unit )
                Unexpected( tok );

            growth->m_unit = new UNIT_RES( growth, tok );
            doUNIT( growth->m_unit );
            break;

        case T_rotate:
            tok = NextTok();

            if( tok != T_on && tok != T_off )
                Expecting( "on|off" );

            growth->m_rotate = tok;
            NeedRIGHT();
            break;

        case T_absolute:
            tok = NextTok();

            if( tok != T_on && tok != T_off )
                Expecting( "on|off" );

            growth->m_absolute = tok;
            NeedRIGHT();
            break;

        case T_shape:
            SHAPE* shape;
            shape = new SHAPE( growth );
            growth->Append( shape );
            doSHAPE( shape );
            break;

        case T_attach:
            tok = NextTok();

            if( tok != T_off && tok != T_on )
                Expecting( "off|on" );

            growth->m_attach = tok;
            tok = NextTok();

            if( tok == T_LEFT )
            {
                if( NextTok() != T_use_via )
                    Expecting( T_use_via );

                NeedSYMBOL();
                growth->m_via_id = CurText();

                NeedRIGHT();
                NeedRIGHT();
            }

            break;

        /*
        case T_via_site:        not supported
            break;
        */

        case T_rule:

            if( growth->m_rules )
                Unexpected( tok );

            growth->m_rules = new RULE( growth, T_rule );
            doRULE( growth->m_rules );
            break;

        default:
            Unexpected( CurText() );
        }
    }
}


void SPECCTRA_DB::doSHAPE( SHAPE* growth )
{
    T       tok;

    /*  (shape <shape_descriptor>
         [<reduced_shape_descriptor> ]
         [(connect [on | off])]
         [{<window_descriptor> }])
    */

    while( ( tok = NextTok() ) != T_RIGHT )
    {
        if( tok != T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_polyline_path:
            tok = T_path;
            KI_FALLTHROUGH;

        case T_rect:
        case T_circle:
        case T_path:
        case T_polygon:
        case T_qarc:
L_done_that:
            if( growth->shape )
                Unexpected( tok );

            break;

        default:
            // the example in the spec uses "circ" instead of "circle".  Bad!
            if( !strcmp( "circ", CurText() ) )
            {
                tok = T_circle;
                goto L_done_that;
            }
        }

        switch( tok )
        {
        case T_rect:
            growth->shape = new RECTANGLE( growth );
            doRECTANGLE( (RECTANGLE*) growth->shape );
            break;

        case T_circle:
            growth->shape = new CIRCLE( growth );
            doCIRCLE( (CIRCLE*)growth->shape );
            break;

        case T_path:
        case T_polygon:
            growth->shape = new PATH( growth, tok );
            doPATH( (PATH*)growth->shape );
            break;

        case T_qarc:
            growth->shape = new QARC( growth );
            doQARC( (QARC*)growth->shape );
            break;

        case T_connect:
            tok = NextTok();
            if( tok!=T_on && tok!=T_off )
                Expecting( "on|off" );
            growth->m_connect = tok;
            NeedRIGHT();
            break;

        case T_window:
            WINDOW* window;
            window = new WINDOW( growth );
            growth->m_windows.push_back( window );
            doWINDOW( window );
            break;

        default:
            Unexpected( CurText() );
        }
    }
}


void SPECCTRA_DB::doIMAGE( IMAGE* growth )
{
    T       tok = NextTok();

    /*  <image_descriptor >::=
        (image <image_id >
           [(side [front | back | both])]
           [<unit_descriptor> ]
           [<outline_descriptor> ]
           {(pin <m_padstack_id > [(rotate <rotation> )]
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

    if( !IsSymbol( tok ) && tok != T_NUMBER  )
        Expecting( "image_id" );

    growth->m_image_id = CurText();

    while( ( tok = NextTok() ) != T_RIGHT )
    {
        if( tok != T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_unit:
            if( growth->m_unit )
                Unexpected( tok );

            growth->m_unit = new UNIT_RES( growth, tok );
            doUNIT( growth->m_unit );
            break;

        case T_side:
            tok = NextTok();

            if( tok != T_front && tok != T_back && tok != T_both )
                Expecting( "front|back|both" );

            growth->m_side = tok;
            NeedRIGHT();
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
            growth->m_pins.push_back( pin );
            doPIN( pin );
            break;

        case T_rule:
            if( growth->m_rules )
                Unexpected( tok );

            growth->m_rules = new RULE( growth, tok );
            doRULE( growth->m_rules );
            break;

        case T_place_rule:
            if( growth->m_place_rules )
                Unexpected( tok );

            growth->m_place_rules = new RULE( growth, tok );
            doRULE( growth->m_place_rules );
            break;

        case T_keepout:
        case T_place_keepout:
        case T_via_keepout:
        case T_wire_keepout:
        case T_bend_keepout:
        case T_elongate_keepout:
            KEEPOUT* keepout;
            keepout = new KEEPOUT( growth, tok );
            growth->m_keepouts.push_back( keepout );
            doKEEPOUT( keepout );
            break;

        default:
            Unexpected( CurText() );
        }
    }
}


void SPECCTRA_DB::doPIN( PIN* growth )
{
    T       tok = NextTok();

    /*  (pin <m_padstack_id > [(rotate <rotation> )]
          [<reference_descriptor> | <pin_array_descriptor> ]
          [<user_property_descriptor> ])
    */

    // a m_padstack_id may be a number
    if( !IsSymbol( tok ) && tok!=T_NUMBER )
        Expecting( "m_padstack_id" );

    growth->m_padstack_id = CurText();

    while( ( tok = NextTok() ) != T_RIGHT )
    {
        if( tok == T_LEFT )
        {
            tok = NextTok();

            if( tok != T_rotate )
                Expecting( T_rotate );

            if( NextTok() != T_NUMBER )
                Expecting( T_NUMBER );

            growth->SetRotation( strtod( CurText(), 0 ) );
            NeedRIGHT();
        }
        else
        {
            if( !IsSymbol( tok ) && tok != T_NUMBER )
                Expecting( "pin_id" );

            growth->m_pin_id = CurText();

            if( NextTok() != T_NUMBER )
                Expecting( T_NUMBER );

            growth->m_vertex.x = strtod( CurText(), 0 );

            if( NextTok() != T_NUMBER )
                Expecting( T_NUMBER );

            growth->m_vertex.y = strtod( CurText(), 0 );
        }
    }
}


void SPECCTRA_DB::doLIBRARY( LIBRARY* growth )
{
    T       tok;

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

    while( ( tok = NextTok() ) != T_RIGHT )
    {
        if( tok != T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_unit:
            if( growth->m_unit )
                Unexpected( tok );

            growth->m_unit = new UNIT_RES( growth, tok );
            doUNIT( growth->m_unit );
            break;

        case T_padstack:
            PADSTACK* padstack;
            padstack = new PADSTACK();
            growth->AddPadstack( padstack );
            doPADSTACK( padstack );
            break;

        case T_image:
            IMAGE*  image;
            image = new IMAGE( growth );
            growth->m_images.push_back( image );
            doIMAGE( image );
            break;

        default:
            Unexpected( CurText() );
        }
    }
}


void SPECCTRA_DB::doNET( NET* growth )
{
    T           tok = NextTok();
    PIN_REFS*   pin_refs;

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

    if( !IsSymbol( tok ) )
        Expecting( "net_id" );

    growth->m_net_id = CurText();

    while( ( tok = NextTok() ) != T_RIGHT )
    {
        if( tok != T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_unassigned:
            growth->m_unassigned = true;
            NeedRIGHT();
            break;

        case T_net_number:
            if( NextTok() != T_NUMBER )
                Expecting( T_NUMBER );

            growth->m_net_number = atoi( CurText() );
            NeedRIGHT();
            break;

        case T_pins:
        case T_order:
            growth->m_pins_type = tok;
            pin_refs = &growth->m_pins;
            goto L_pins;

        case T_expose:
            pin_refs = &growth->m_expose;
            goto L_pins;

        case T_noexpose:
            pin_refs = &growth->m_noexpose;
            goto L_pins;

        case T_source:
            pin_refs = &growth->m_source;
            goto L_pins;

        case T_load:
            pin_refs = &growth->m_load;
            goto L_pins;

        case T_terminator:
            pin_refs = &growth->m_terminator;
            //goto L_pins;

L_pins:
            {
                PIN_REF     empty( growth );

                while( ( tok = NextTok() ) != T_RIGHT )
                {
                    // copy the empty one, then fill its copy later thru pin_ref.
                    pin_refs->push_back( empty );

                    PIN_REF* pin_ref = &pin_refs->back();

                    readCOMPnPIN( &pin_ref->component_id, &pin_ref->pin_id );
                }
            }

            break;

        case T_comp_order:
            if( growth->m_comp_order )
                Unexpected( tok );

            growth->m_comp_order = new COMP_ORDER( growth );
            doCOMP_ORDER( growth->m_comp_order );
            break;

        case T_type:
            tok = NextTok();

            if( tok!=T_fix && tok!=T_normal )
                Expecting( "fix|normal" );

            growth->type = tok;
            NeedRIGHT();
            break;

/* @todo
        case T_circuit:
            break;
*/

        case T_rule:
            if( growth->m_rules )
                Unexpected( tok );

            growth->m_rules = new RULE( growth, T_rule );
            doRULE( growth->m_rules );
            break;

        case T_layer_rule:
            LAYER_RULE* layer_rule;
            layer_rule = new LAYER_RULE( growth );
            growth->m_layer_rules.push_back( layer_rule );
            doLAYER_RULE( layer_rule );
            break;

        case T_fromto:
            FROMTO* fromto;
            fromto = new FROMTO( growth );
            growth->m_fromtos.push_back( fromto );
            doFROMTO( fromto );
            break;

        default:
            Unexpected( CurText() );
        }
    }
}


void SPECCTRA_DB::doTOPOLOGY( TOPOLOGY* growth )
{
    T       tok;

    /*  <topology_descriptor >::=
        (topology {[<fromto_descriptor> |
        <component_order_descriptor> ]})
    */

    while( ( tok = NextTok() ) != T_RIGHT )
    {
        if( tok != T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_fromto:
            FROMTO* fromto;
            fromto = new FROMTO( growth );
            growth->m_fromtos.push_back( fromto );
            doFROMTO( fromto );
            break;

        case T_comp_order:
            COMP_ORDER*  comp_order;
            comp_order = new COMP_ORDER( growth );
            growth->m_comp_orders.push_back( comp_order );
            doCOMP_ORDER( comp_order );
            break;

        default:
            Unexpected( CurText() );
        }
    }
}


void SPECCTRA_DB::doCLASS( CLASS* growth )
{
    T       tok;

    /*  <class_descriptor >::=
        (class
           <class_id > {[{<net_id >} | {<composite_name_list> }]}
           [<circuit_descriptor> ]
           [<rule_descriptor> ]
           [{<layer_rule_descriptor> }]
           [<topology_descriptor> ]
        )
    */

    NeedSYMBOL();

    growth->m_class_id = CurText();

    // do net_ids, do not support <composite_name_list>s at this time
    while( IsSymbol( tok = NextTok() ) )
    {
        growth->m_net_ids.push_back( CurText() );
    }


    while( tok != T_RIGHT )
    {
        if( tok != T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_rule:
            if( growth->m_rules )
                Unexpected( tok );

            growth->m_rules = new RULE( growth, T_rule );
            doRULE( growth->m_rules );
            break;

        case T_layer_rule:
            LAYER_RULE* layer_rule;
            layer_rule = new LAYER_RULE( growth );
            growth->m_layer_rules.push_back( layer_rule );
            doLAYER_RULE( layer_rule );
            break;

        case T_topology:
            if( growth->m_topology )
                Unexpected( tok );

            growth->m_topology = new TOPOLOGY( growth );
            doTOPOLOGY( growth->m_topology );
            break;

        case T_circuit:  // handle all the circuit_descriptor here as strings
        {
            std::string     builder;
            int             bracketNesting = 1; // we already saw the opening T_LEFT
            tok = T_NONE;

            while( bracketNesting != 0 && tok != T_EOF )
            {
                tok = NextTok();

                if( tok == T_LEFT )
                    ++bracketNesting;
                else if( tok == T_RIGHT )
                    --bracketNesting;

                if( bracketNesting >= 1 )
                {
                    T previousTok = (T) PrevTok();

                    if( previousTok != T_LEFT && previousTok != T_circuit && tok != T_RIGHT )
                        builder += ' ';

                    if( tok == T_STRING )
                        builder += m_quote_char;

                    builder += CurText();

                    if( tok == T_STRING )
                        builder += m_quote_char;
                }

                // When the nested rule is closed with a T_RIGHT and we are back down
                // to bracketNesting == 0, then save the builder and break;
                if( bracketNesting == 0 )
                {
                    growth->m_circuit.push_back( builder );
                    break;
                }
            }

            if( tok == T_EOF )
                Unexpected( T_EOF );

            break;
        }                                   // scope bracket

        default:
            Unexpected( CurText() );
        }                                       // switch

        tok = NextTok();

    } // while
}


void SPECCTRA_DB::doNETWORK( NETWORK* growth )
{
    T       tok;

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

    while( ( tok = NextTok() ) != T_RIGHT )
    {
        if( tok != T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_net:
            NET* net;
            net = new NET( growth );
            growth->m_nets.push_back( net );
            doNET( net );
            break;

        case T_class:
            CLASS*  myclass;
            myclass = new CLASS( growth );
            growth->m_classes.push_back( myclass );
            doCLASS( myclass );
            break;

        default:
            Unexpected( CurText() );
        }
    }
}


void SPECCTRA_DB::doCOMP_ORDER( COMP_ORDER* growth )
{
    T       tok;

    /*  <component_order_descriptor >::=
        (comp_order {<placement_id> })
    */

    while( IsSymbol( tok = NextTok() ) )
        growth->m_placement_ids.push_back( CurText() );

    if( tok != T_RIGHT )
        Expecting( T_RIGHT );
}


void SPECCTRA_DB::doFROMTO( FROMTO* growth )
{
    T       tok;

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

    char old = SetStringDelimiter( 0 );

    if( !IsSymbol(NextTok() ) )
    {
        SetStringDelimiter( old );
        Expecting( T_SYMBOL );
    }

    growth->m_fromText = CurText();

    if( !IsSymbol(NextTok() ) )
    {
        SetStringDelimiter( old );
        Expecting( T_SYMBOL );
    }

    growth->m_toText = CurText();

    SetStringDelimiter( old );

    while( ( tok = NextTok() ) != T_RIGHT )
    {
        if( tok != T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_type:
            tok = NextTok();

            if( tok != T_fix && tok != T_normal && tok != T_soft )
                Expecting( "fix|normal|soft" );

            growth->m_fromto_type = tok;
            NeedRIGHT();
            break;

        case T_rule:
            if( growth->m_rules )
                Unexpected( tok );

            growth->m_rules = new RULE( growth, T_rule );
            doRULE( growth->m_rules );
            break;

        case T_layer_rule:
            LAYER_RULE* layer_rule;
            layer_rule = new LAYER_RULE( growth );
            growth->m_layer_rules.push_back( layer_rule );
            doLAYER_RULE( layer_rule );
            break;

        case T_net:
            if( growth->m_net_id.size() )
                Unexpected( tok );

            NeedSYMBOL();
            growth->m_net_id = CurText();
            NeedRIGHT();
            break;

        // circuit descriptor not supported at this time

        default:
            Unexpected( CurText() );
        }
    }
}


void SPECCTRA_DB::doWIRE( WIRE* growth )
{
    T       tok;

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

    while( ( tok = NextTok() ) != T_RIGHT )
    {
        if( tok != T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_rect:
            if( growth->m_shape )
                Unexpected( tok );

            growth->m_shape = new RECTANGLE( growth );
            doRECTANGLE( (RECTANGLE*) growth->m_shape );
            break;

        case T_circle:
            if( growth->m_shape )
                Unexpected( tok );

            growth->m_shape = new CIRCLE( growth );
            doCIRCLE( (CIRCLE*) growth->m_shape );
            break;

        case T_polyline_path:
            tok = T_path;
            KI_FALLTHROUGH;

        case T_path:
        case T_polygon:
            if( growth->m_shape )
                Unexpected( tok );

            growth->m_shape = new PATH( growth, tok );
            doPATH( (PATH*) growth->m_shape );
            break;

        case T_qarc:
            if( growth->m_shape )
                Unexpected( tok );

            growth->m_shape = new QARC( growth );
            doQARC( (QARC*) growth->m_shape );
            break;

        case T_net:
            NeedSYMBOLorNUMBER();
            growth->m_net_id = CurText();
            NeedRIGHT();
            break;

        case T_turret:
            if( NextTok() != T_NUMBER )
                Expecting( T_NUMBER );

            growth->m_turret = atoi( CurText() );
            NeedRIGHT();
            break;

        case T_type:
            tok = NextTok();

            if( tok != T_fix && tok != T_route && tok != T_normal && tok != T_protect )
                Expecting( "fix|route|normal|protect" );

            growth->m_wire_type = tok;
            NeedRIGHT();
            break;

        case T_attr:
            tok = NextTok();

            if( tok != T_test && tok != T_fanout && tok != T_bus && tok != T_jumper )
                Expecting( "test|fanout|bus|jumper" );

            growth->m_attr = tok;
            NeedRIGHT();
            break;

        case T_shield:
            NeedSYMBOL();
            growth->m_shield = CurText();
            NeedRIGHT();
            break;

        case T_window:
            WINDOW* window;
            window = new WINDOW( growth );
            growth->m_windows.push_back( window );
            doWINDOW( window );
            break;

        case T_connect:
            if( growth->m_connect )
                Unexpected( tok );

            growth->m_connect = new CONNECT( growth );
            doCONNECT( growth->m_connect );
            break;

        case T_supply:
            growth->m_supply = true;
            NeedRIGHT();
            break;

        default:
            Unexpected( CurText() );
        }
    }
}


void SPECCTRA_DB::doWIRE_VIA( WIRE_VIA* growth )
{
    T       tok;
    POINT   point;

    /*  <wire_via_descriptor >::=
        (via
           <m_padstack_id > {<vertex> }
           [(net <net_id >)]
           [(via_number <via#> )]
           [(type [fix | route | normal | protect])]
           [(attr [test | fanout | jumper |
              virtual_pin <m_virtual_pin_name> ])]
           [(contact {<layer_id >})]
           [(supply)]
        )
        (virtual_pin
           <m_virtual_pin_name> <vertex> (net <net_id >)
        )
    */

    NeedSYMBOL();
    growth->m_padstack_id = CurText();

    while( ( tok = NextTok() ) == T_NUMBER )
    {
        point.x = strtod( CurText(), 0 );

        if( NextTok() != T_NUMBER )
            Expecting( "vertex.y" );

        point.y = strtod( CurText(), 0 );

        growth->m_vertexes.push_back( point );
    }

    while( tok != T_RIGHT )
    {
        if( tok != T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_net:
            NeedSYMBOL();
            growth->m_net_id = CurText();
            NeedRIGHT();
            break;

        case T_via_number:
            if( NextTok() != T_NUMBER )
                Expecting( "<via#>" );

            growth->m_via_number = atoi( CurText() );
            NeedRIGHT();
            break;

        case T_type:
            tok = NextTok();

            if( tok != T_fix && tok != T_route && tok != T_normal && tok != T_protect )
                Expecting( "fix|route|normal|protect" );

            growth->m_via_type = tok;
            NeedRIGHT();
            break;

        case T_attr:
            tok = NextTok();

            if( tok != T_test && tok != T_fanout && tok != T_jumper && tok != T_virtual_pin )
                Expecting( "test|fanout|jumper|virtual_pin" );

            growth->m_attr = tok;

            if( tok == T_virtual_pin )
            {
                NeedSYMBOL();
                growth->m_virtual_pin_name = CurText();
            }

            NeedRIGHT();
            break;

        case T_contact:
            NeedSYMBOL();
            tok = T_SYMBOL;

            while( IsSymbol( tok ) )
            {
                growth->m_contact_layers.push_back( CurText() );
                tok = NextTok();
            }

            if( tok != T_RIGHT )
                Expecting( T_RIGHT );

            break;

        case T_supply:
            growth->m_supply = true;
            NeedRIGHT();
            break;

        default:
            Unexpected( CurText() );
        }

        tok = NextTok();
    }
}


void SPECCTRA_DB::doWIRING( WIRING* growth )
{
    T       tok;

    /*  <wiring_descriptor >::=
        (wiring
          [<unit_descriptor> | <resolution_descriptor> | null]
          {<wire_descriptor> }
          [<test_points_descriptor> ]
          {[<supply_pin_descriptor> ]}
        )
    */

    while( ( tok = NextTok() ) != T_RIGHT )
    {
        if( tok != T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_unit:
            if( growth->unit )
                Unexpected( tok );

            growth->unit = new UNIT_RES( growth, tok );
            doUNIT( growth->unit );
            break;

        case T_resolution:
            if( growth->unit )
                Unexpected( tok );

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
            Unexpected( CurText() );
        }
    }
}


void SPECCTRA_DB::doANCESTOR( ANCESTOR* growth )
{
    T       tok;

    /*  <ancestor_file_descriptor >::=
          (ancestor <file_path_name> (created_time <time_stamp> )
          [(comment <comment_string> )])
    */

    NeedSYMBOL();
    growth->filename = CurText();

    while( ( tok = NextTok() ) != T_RIGHT )
    {
        if( tok != T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_created_time:
            readTIME( &growth->time_stamp );
            NeedRIGHT();
            break;

        case T_comment:
            NeedSYMBOL();
            growth->comment = CurText();
            NeedRIGHT();
            break;

        default:
            Unexpected( CurText() );
        }
    }
}


void SPECCTRA_DB::doHISTORY( HISTORY* growth )
{
    T       tok;

    /*  <history_descriptor >::=
        (history [{<ancestor_file_descriptor> }] <self_descriptor> )
    */

    while( ( tok = NextTok() ) != T_RIGHT )
    {
        if( tok != T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_ancestor:
            ANCESTOR* ancestor;
            ancestor = new ANCESTOR( growth );
            growth->ancestors.push_back( ancestor );
            doANCESTOR( ancestor );
            break;

        case T_self:
            while( ( tok = NextTok() ) != T_RIGHT )
            {
                if( tok != T_LEFT )
                    Expecting( T_LEFT );

                tok = NextTok();

                switch( tok )
                {
                case T_created_time:
                    readTIME( &growth->time_stamp );
                    NeedRIGHT();
                    break;

                case T_comment:
                    NeedSYMBOL();
                    growth->comments.push_back( CurText() );
                    NeedRIGHT();
                    break;

                default:
                    Unexpected( CurText() );
                }
            }

            break;

        default:
            Unexpected( CurText() );
        }
    }
}


void SPECCTRA_DB::doSESSION( SESSION* growth )
{
    T       tok;

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

    // The path can be defined by multiple tokens if there are spaces in it (e.g. by TopoR).
    NeedSYMBOL();
    std::stringstream fullPath;
    fullPath << CurText();

    while( ( tok = NextTok() ) != T_LEFT )
        fullPath << " " << CurText();

    growth->session_id = fullPath.str();

    do
    {
        if( tok != T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_base_design:
            NeedSYMBOL();
            growth->base_design = CurText();
            NeedRIGHT();
            break;

        case T_history:
            if( growth->history )
                Unexpected( tok );

            growth->history = new HISTORY( growth );
            doHISTORY( growth->history );
            break;

        case T_structure:
            if( growth->structure )
                Unexpected( tok );

            growth->structure = new STRUCTURE( growth );
            doSTRUCTURE( growth->structure );
            break;

        case T_placement:
            if( growth->placement )
                Unexpected( tok );

            growth->placement = new PLACEMENT( growth );
            doPLACEMENT( growth->placement );
            break;

        case T_was_is:
            if( growth->was_is )
                Unexpected( tok );

            growth->was_is = new WAS_IS( growth );
            doWAS_IS( growth->was_is );
            break;

        case T_routes:
            if( growth->route )
                Unexpected( tok );

            growth->route = new ROUTE( growth );
            doROUTE( growth->route );
            break;

        default:
            Unexpected( CurText() );
        }
    } while( ( tok = NextTok() ) != T_RIGHT );
}


void SPECCTRA_DB::doWAS_IS( WAS_IS* growth )
{
    T           tok;
    PIN_PAIR    empty( growth );
    PIN_PAIR*   pin_pair;

    /*  <was_is_descriptor >::=
        (was_is {(pins <pin_reference> <pin_reference> )})
    */

    // none of the pins is ok too
    while( ( tok = NextTok() ) != T_RIGHT )
    {
        if( tok != T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_pins:
            // copy the empty one, then fill its copy later thru pin_pair.
            growth->pin_pairs.push_back( empty );
            pin_pair= &growth->pin_pairs.back();

            NeedSYMBOL();       // readCOMPnPIN() expects 1st token to have been read
            readCOMPnPIN( &pin_pair->was.component_id, &pin_pair->was.pin_id );

            NeedSYMBOL();       // readCOMPnPIN() expects 1st token to have been read
            readCOMPnPIN( &pin_pair->is.component_id, &pin_pair->is.pin_id );

            NeedRIGHT();
            break;

        default:
            Unexpected( CurText() );
        }
    }
}


void SPECCTRA_DB::doROUTE( ROUTE* growth )
{
    T       tok;

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

    while( ( tok = NextTok() ) != T_RIGHT )
    {
        if( tok != T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_resolution:
            if( growth->resolution )
                Unexpected( tok );

            growth->resolution = new UNIT_RES( growth, tok );
            doRESOLUTION( growth->resolution );
            break;

        case T_parser:
            if( growth->parser )
            {
#if 0           // Electra 2.9.1 emits two (parser ) elements in a row.
                // Work around their bug for now.
                Unexpected( tok );
#else
                delete growth->parser;
#endif
            }

            growth->parser = new PARSER( growth );
            doPARSER( growth->parser );
            break;

        case T_structure_out:
            if( growth->structure_out )
                Unexpected( tok );

            growth->structure_out = new STRUCTURE_OUT( growth );
            doSTRUCTURE_OUT( growth->structure_out );
            break;

        case T_library_out:
            if( growth->library )
                Unexpected( tok );

            growth->library = new LIBRARY( growth, tok );
            doLIBRARY( growth->library );
            break;

        case T_network_out:
            while( ( tok = NextTok() ) != T_RIGHT )
            {
                if( tok != T_LEFT )
                    Expecting( T_LEFT );

                tok = NextTok();

                // it is class NET_OUT, but token T_net in Freerouting
                // Allegro PCB Router (Specctra) uses capitalized "Net"
                if( tok != T_net && !( tok == T_SYMBOL && !strcmp( CurText(), "Net" ) ) )
                    Unexpected( CurText() );

                NET_OUT*    net_out;
                net_out = new NET_OUT( growth );

                growth->net_outs.push_back( net_out );
                doNET_OUT( net_out );
            }

            break;

        case T_test_points:
            while( ( tok = NextTok() ) != T_RIGHT )
            {
                // TODO: Not supported yet
                Unexpected( CurText() );
            }
            break;

        default:
            Unexpected( CurText() );
        }
    }
}


void SPECCTRA_DB::doNET_OUT( NET_OUT* growth )
{
    T       tok;

    /*  <net_out_descriptor >::=
        (net <net_id >
          [(net_number <integer >)]
          [<rule_descriptor> ]
          {[<wire_shape_descriptor> | <wire_guide_descriptor> |
             <wire_via_descriptor> | <bond_shape_descriptor> ]}
          {[<supply_pin_descriptor> ]}
        )
    */

    NeedSYMBOLorNUMBER();
    growth->net_id = CurText();

    while( ( tok = NextTok() ) != T_RIGHT )
    {
        if( tok != T_LEFT )
            Expecting( T_LEFT );

        tok = NextTok();

        switch( tok )
        {
        case T_net_number:
            tok = NextTok();

            if( tok!= T_NUMBER )
                Expecting( T_NUMBER );

            growth->net_number = atoi( CurText() );
            NeedRIGHT();
            break;

        case T_rule:
            if( growth->rules )
                Unexpected( tok );

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
            Unexpected( CurText() );
        }
    }
}


void SPECCTRA_DB::doSUPPLY_PIN( SUPPLY_PIN* growth )
{
    T       tok;
    PIN_REF empty(growth);

    /*  <supply_pin_descriptor >::=
        (supply_pin {<pin_reference> } [(net <net_id >)])
    */

    NeedSYMBOL();
    growth->net_id = CurText();

    while( ( tok = NextTok() ) != T_RIGHT )
    {
        if( IsSymbol(tok) )
        {
            growth->pin_refs.push_back( empty );

            PIN_REF*    pin_ref = &growth->pin_refs.back();

            readCOMPnPIN( &pin_ref->component_id, &pin_ref->pin_id );
        }
        else if( tok == T_LEFT )
        {
            tok = NextTok();

            if( tok != T_net )
                Expecting( T_net );

            growth->net_id = CurText();
            NeedRIGHT();
        }
        else
            Unexpected( CurText() );
    }
}


void SPECCTRA_DB::ExportPCB( const wxString& aFilename, bool aNameChange )
{
    if( m_pcb )
    {
        FILE_OUTPUTFORMATTER    formatter( aFilename, wxT( "wt" ), m_quote_char[0] );

        if( aNameChange )
            m_pcb->m_pcbname = TO_UTF8( aFilename );

        m_pcb->Format( &formatter, 0 );
    }
}


void SPECCTRA_DB::ExportSESSION( const wxString& aFilename )
{
    if( m_session )
    {
        FILE_OUTPUTFORMATTER formatter( aFilename, wxT( "wt" ), m_quote_char[0] );

        m_session->Format( &formatter, 0 );
    }
}


PCB* SPECCTRA_DB::MakePCB()
{
    PCB*    pcb = new PCB();

    pcb->m_parser = new PARSER( pcb );
    pcb->m_resolution = new UNIT_RES( pcb, T_resolution );
    pcb->m_unit = new UNIT_RES( pcb, T_unit );

    pcb->m_structure = new STRUCTURE( pcb );
    pcb->m_structure->m_boundary = new BOUNDARY( pcb->m_structure );
    pcb->m_structure->m_via = new VIA( pcb->m_structure );
    pcb->m_structure->m_rules = new RULE( pcb->m_structure, T_rule );

    pcb->m_placement = new PLACEMENT( pcb );

    pcb->m_library = new LIBRARY( pcb );

    pcb->m_network = new NETWORK( pcb );

    pcb->m_wiring = new WIRING( pcb );

    return pcb;
}


//-----<ELEM>---------------------------------------------------------------

ELEM::ELEM( T     aType, ELEM* aParent ) :
   type( aType ),
   parent( aParent )
{
}


ELEM::~ELEM()
{
}

const char* ELEM::Name() const
{
    return SPECCTRA_DB::TokenName( type );
}

UNIT_RES* ELEM::GetUnits() const
{
    if( parent )
        return parent->GetUnits();

    return &UNIT_RES::Default;
}


void ELEM::Format( OUTPUTFORMATTER* out, int nestLevel )
{
    out->Print( nestLevel, "(%s\n", Name() );

    FormatContents( out, nestLevel+1 );

    out->Print( nestLevel, ")\n" );
}


void ELEM_HOLDER::FormatContents( OUTPUTFORMATTER* out, int nestLevel )
{
    for( int i = 0; i < Length(); ++i )
        At(i)->Format( out, nestLevel );
}


int ELEM_HOLDER::FindElem( T aType, int instanceNum )
{
    int repeats=0;

    for( unsigned i = 0; i < kids.size(); ++i )
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


// a reasonably small memory price to pay for improved performance
STRING_FORMATTER  ELEM::sf;


UNIT_RES UNIT_RES::Default( nullptr, T_resolution );


int PADSTACK::Compare( PADSTACK* lhs, PADSTACK* rhs )
{
    if( !lhs->m_hash.size() )
        lhs->m_hash = lhs->makeHash();

    if( !rhs->m_hash.size() )
        rhs->m_hash = rhs->makeHash();

    int result = lhs->m_hash.compare( rhs->m_hash );

    if( result )
        return result;

    // Via names hold the drill diameters, so we have to include those to discern
    // between two vias with same copper size but with different drill sizes.
    result = lhs->m_padstack_id.compare( rhs->m_padstack_id );

    return result;
}


int IMAGE::Compare( IMAGE* lhs, IMAGE* rhs )
{
    if( !lhs->m_hash.size() )
        lhs->m_hash = lhs->makeHash();

    if( !rhs->m_hash.size() )
        rhs->m_hash = rhs->makeHash();

    int result = lhs->m_hash.compare( rhs->m_hash );

    return result;
}


/*
int COMPONENT::Compare( COMPONENT* lhs, COMPONENT* rhs )
{
    if( !lhs->hash.size() )
        lhs->hash = lhs->makeHash();

    if( !rhs->hash.size() )
        rhs->hash = rhs->makeHash();

    int result = lhs->hash.compare( rhs->hash );
    return result;
}
*/

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

    host_cad = "KiCad's Pcbnew";
    wxString msg = GetBuildVersion();
    host_version = TO_UTF8(msg);
}


void PARSER::FormatContents( OUTPUTFORMATTER* out, int nestLevel )
{
    out->Print( nestLevel, "(string_quote %c)\n", string_quote );
    out->Print( nestLevel, "(space_in_quoted_tokens %s)\n", space_in_quoted_tokens ? "on" : "off" );
    out->Print( nestLevel, "(host_cad \"%s\")\n", host_cad.c_str() );
    out->Print( nestLevel, "(host_version \"%s\")\n", host_version.c_str() );

    for( STRINGS::iterator i = constants.begin(); i != constants.end(); )
    {
        const std::string& s1 = *i++;
        const std::string& s2 = *i++;

        const char* q1 = out->GetQuoteChar( s1.c_str() );
        const char* q2 = out->GetQuoteChar( s2.c_str() );
        out->Print( nestLevel, "(constant %s%s%s %s%s%s)\n",
                    q1, s1.c_str(), q1, q2, s2.c_str(), q2 );
    }

    if( routes_include_testpoint || routes_include_guides || routes_include_image_conductor )
    {
        out->Print( nestLevel, "(routes_include%s%s%s)\n",
                    routes_include_testpoint ? " testpoint" : "",
                    routes_include_guides ? " guides" : "",
                    routes_include_image_conductor ? " image_conductor" : "" );
    }

    if( wires_include_testpoint )
        out->Print( nestLevel, "(wires_include testpoint)\n" );

    if( !via_rotate_first )
        out->Print( nestLevel, "(via_rotate_first off)\n" );

    if( case_sensitive )
        out->Print( nestLevel, "(case_sensitive %s)\n", case_sensitive ? "on" : "off" );
}


void PLACE::Format( OUTPUTFORMATTER* out, int nestLevel )
{
    bool        useMultiLine;

    const char* quote = out->GetQuoteChar( m_component_id.c_str() );

    if( m_place_rules || m_properties.size() || m_rules || m_region )
    {
        useMultiLine = true;

        out->Print( nestLevel, "(%s %s%s%s\n", Name(), quote, m_component_id.c_str(), quote );
        out->Print( nestLevel+1, "%s", "" );
    }
    else
    {
        useMultiLine = false;

        out->Print( nestLevel, "(%s %s%s%s", Name(), quote, m_component_id.c_str(), quote );
    }

    if( m_hasVertex )
    {
        out->Print( 0, " %.6f %.6f", m_vertex.x, m_vertex.y );
        out->Print( 0, " %s", GetTokenText( m_side ) );
        out->Print( 0, " %.6f", m_rotation );
    }

    const char* space = " ";    // one space, as c string.

    if( m_mirror != T_NONE )
    {
        out->Print( 0, "%s(mirror %s)", space, GetTokenText( m_mirror ) );
        space = "";
    }

    if( m_status != T_NONE )
    {
        out->Print( 0, "%s(status %s)", space, GetTokenText( m_status ) );
        space = "";
    }

    if( m_logical_part.size() )
    {
        quote = out->GetQuoteChar( m_logical_part.c_str() );
        out->Print( 0, "%s(logical_part %s%s%s)", space, quote, m_logical_part.c_str(), quote );
        space = "";
    }

    if( useMultiLine )
    {
        out->Print( 0, "\n" );

        if( m_place_rules )
            m_place_rules->Format( out, nestLevel+1 );

        if( m_properties.size() )
        {
            out->Print( nestLevel + 1, "(property \n" );

            for( PROPERTIES::const_iterator i = m_properties.begin(); i != m_properties.end(); ++i )
                i->Format( out, nestLevel + 2 );

            out->Print( nestLevel + 1, ")\n" );
        }

        if( m_lock_type != T_NONE )
            out->Print( nestLevel + 1, "(lock_type %s)\n", GetTokenText( m_lock_type ) );

        if( m_rules )
            m_rules->Format( out, nestLevel+1 );

        if( m_region )
            m_region->Format( out, nestLevel+1 );

        if( m_part_number.size() )
        {
            quote = out->GetQuoteChar( m_part_number.c_str() );
            out->Print( nestLevel + 1, "(PN %s%s%s)\n", quote, m_part_number.c_str(), quote );
        }
    }
    else
    {
        if( m_lock_type != T_NONE )
        {
            out->Print( 0, "%s(lock_type %s)", space, GetTokenText( m_lock_type ) );
            space = "";
        }

        if( m_part_number.size() )
        {
            quote = out->GetQuoteChar( m_part_number.c_str() );
            out->Print( 0, "%s(PN %s%s%s)", space, quote, m_part_number.c_str(), quote );
        }
    }

    out->Print( 0, ")\n" );
}

} // namespace DSN
