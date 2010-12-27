
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
    an assignment operator() or copy constructor, then boost::ptr_vector
    cannot be beat.
*/


#include <cstdarg>
#include <cstdio>

#include "specctra.h"
#include <wx/wfstream.h>        // wxFFileOutputStream

#include "build_version.h"


namespace DSN {

#define NESTWIDTH           2   ///< how many spaces per nestLevel

#define TOKDEF(x)    { #x, T_##x }

// This MUST be sorted alphabetically, and the order of enum DSN_T {} be
// identially alphabetized. These MUST all be lower case because of the
// conversion to lowercase in findToken().
const KEYWORD SPECCTRA_DB::keywords[] = {

    // Note that TOKDEF(string_quote) has been moved to the
    // DSNLEXER, and DSN_SYNTAX_T enum, and the string for it is "string_quote".

    TOKDEF(absolute),
    TOKDEF(added),
    TOKDEF(add_group),
    TOKDEF(add_pins),
    TOKDEF(allow_antenna),
    TOKDEF(allow_redundant_wiring),
    TOKDEF(amp),
    TOKDEF(ancestor),
    TOKDEF(antipad),
    TOKDEF(aperture_type),
    TOKDEF(array),
    TOKDEF(attach),
    TOKDEF(attr),
    TOKDEF(average_pair_length),
    TOKDEF(back),
    TOKDEF(base_design),
    TOKDEF(bbv_ctr2ctr),
    TOKDEF(bend_keepout),
    TOKDEF(bond),
    TOKDEF(both),
    TOKDEF(bottom),
    TOKDEF(bottom_layer_sel),
    TOKDEF(boundary),
    TOKDEF(brickpat),
    TOKDEF(bundle),
    TOKDEF(bus),
    TOKDEF(bypass),
    TOKDEF(capacitance_resolution),
    TOKDEF(capacitor),
    TOKDEF(case_sensitive),
    TOKDEF(cct1),
    TOKDEF(cct1a),
    TOKDEF(center_center),
    TOKDEF(checking_trim_by_pin),
    TOKDEF(circ),
    TOKDEF(circle),
    TOKDEF(circuit),
    TOKDEF(class),
    TOKDEF(class_class),
    TOKDEF(classes),
    TOKDEF(clear),
    TOKDEF(clearance),
    TOKDEF(cluster),
    TOKDEF(cm),
    TOKDEF(color),
    TOKDEF(colors),
    TOKDEF(comment),
    TOKDEF(comp),
    TOKDEF(comp_edge_center),
    TOKDEF(comp_order),
    TOKDEF(component),
    TOKDEF(composite),
    TOKDEF(conductance_resolution),
    TOKDEF(conductor),
    TOKDEF(conflict),
    TOKDEF(connect),
    TOKDEF(constant),
    TOKDEF(contact),
    TOKDEF(control),
    TOKDEF(corner),
    TOKDEF(corners),
    TOKDEF(cost),
    TOKDEF(created_time),
    TOKDEF(cross),
    TOKDEF(crosstalk_model),
    TOKDEF(current_resolution),
    TOKDEF(delete_pins),
    TOKDEF(deleted),
    TOKDEF(deleted_keepout),
    TOKDEF(delta),
    TOKDEF(diagonal),
    TOKDEF(direction),
    TOKDEF(directory),
    TOKDEF(discrete),
    TOKDEF(effective_via_length),
    TOKDEF(elongate_keepout),
    TOKDEF(exclude),
    TOKDEF(expose),
    TOKDEF(extra_image_directory),
    TOKDEF(family),
    TOKDEF(family_family),
    TOKDEF(family_family_spacing),
    TOKDEF(fanout),
    TOKDEF(farad),
    TOKDEF(file),
    TOKDEF(fit),
    TOKDEF(fix),
    TOKDEF(flip_style),
    TOKDEF(floor_plan),
    TOKDEF(footprint),
    TOKDEF(forbidden),
    TOKDEF(force_to_terminal_point),
    TOKDEF(forgotten),
    TOKDEF(free),
    TOKDEF(fromto),
    TOKDEF(front),
    TOKDEF(front_only),
    TOKDEF(gap),
    TOKDEF(gate),
    TOKDEF(gates),
    TOKDEF(generated_by_freeroute),
    TOKDEF(global),
    TOKDEF(grid),
    TOKDEF(group),
    TOKDEF(group_set),
    TOKDEF(guide),
    TOKDEF(hard),
    TOKDEF(height),
    TOKDEF(high),
    TOKDEF(history),
    TOKDEF(horizontal),
    TOKDEF(host_cad),
    TOKDEF(host_version),
    TOKDEF(image),
    TOKDEF(image_conductor),
    TOKDEF(image_image),
    TOKDEF(image_image_spacing),
    TOKDEF(image_outline_clearance),
    TOKDEF(image_set),
    TOKDEF(image_type),
    TOKDEF(inch),
    TOKDEF(include),
    TOKDEF(include_pins_in_crosstalk),
    TOKDEF(inductance_resolution),
    TOKDEF(insert),
    TOKDEF(instcnfg),
    TOKDEF(inter_layer_clearance),
    TOKDEF(jumper),
    TOKDEF(junction_type),
    TOKDEF(keepout),
    TOKDEF(kg),
    TOKDEF(kohm),
    TOKDEF(large),
    TOKDEF(large_large),
    TOKDEF(layer),
    TOKDEF(layer_depth),
    TOKDEF(layer_noise_weight),
    TOKDEF(layer_pair),
    TOKDEF(layer_rule),
    TOKDEF(length),
    TOKDEF(length_amplitude),
    TOKDEF(length_factor),
    TOKDEF(length_gap),
    TOKDEF(library),
    TOKDEF(library_out),
    TOKDEF(limit),
    TOKDEF(limit_bends),
    TOKDEF(limit_crossing),
    TOKDEF(limit_vias),
    TOKDEF(limit_way),
    TOKDEF(linear),
    TOKDEF(linear_interpolation),
    TOKDEF(load),
    TOKDEF(lock_type),
    TOKDEF(logical_part),
    TOKDEF(logical_part_mapping),
    TOKDEF(low),
    TOKDEF(match_fromto_delay),
    TOKDEF(match_fromto_length),
    TOKDEF(match_group_delay),
    TOKDEF(match_group_length),
    TOKDEF(match_net_delay),
    TOKDEF(match_net_length),
    TOKDEF(max_delay),
    TOKDEF(max_len),
    TOKDEF(max_length),
    TOKDEF(max_noise),
    TOKDEF(max_restricted_layer_length),
    TOKDEF(max_stagger),
    TOKDEF(max_stub),
    TOKDEF(max_total_delay),
    TOKDEF(max_total_length),
    TOKDEF(max_total_vias),
    TOKDEF(medium),
    TOKDEF(mhenry),
    TOKDEF(mho),
    TOKDEF(microvia),
    TOKDEF(mid_driven),
    TOKDEF(mil),
    TOKDEF(min_gap),
    TOKDEF(mirror),
    TOKDEF(mirror_first),
    TOKDEF(mixed),
    TOKDEF(mm),
    TOKDEF(negative_diagonal),
    TOKDEF(net),
    TOKDEF(net_number),
    TOKDEF(net_out),
    TOKDEF(net_pin_changes),
    TOKDEF(nets),
    TOKDEF(network),
    TOKDEF(network_out),
    TOKDEF(no),
    TOKDEF(noexpose),
    TOKDEF(noise_accumulation),
    TOKDEF(noise_calculation),
    TOKDEF(normal),
    TOKDEF(object_type),
    TOKDEF(off),
    TOKDEF(off_grid),
    TOKDEF(offset),
    TOKDEF(on),
    TOKDEF(open),
    TOKDEF(opposite_side),
    TOKDEF(order),
    TOKDEF(orthogonal),
    TOKDEF(outline),
    TOKDEF(overlap),
    TOKDEF(pad),
    TOKDEF(pad_pad),
    TOKDEF(padstack),
    TOKDEF(pair),
    TOKDEF(parallel),
    TOKDEF(parallel_noise),
    TOKDEF(parallel_segment),
    TOKDEF(parser),
    TOKDEF(part_library),
    TOKDEF(path),
    TOKDEF(pcb),
    TOKDEF(permit_orient),
    TOKDEF(permit_side),
    TOKDEF(physical),
    TOKDEF(physical_part_mapping),
    TOKDEF(piggyback),
    TOKDEF(pin),
    TOKDEF(pin_allow),
    TOKDEF(pin_cap_via),
    TOKDEF(pin_via_cap),
    TOKDEF(pin_width_taper),
    TOKDEF(pins),
    TOKDEF(pintype),
    TOKDEF(place),
    TOKDEF(place_boundary),
    TOKDEF(place_control),
    TOKDEF(place_keepout),
    TOKDEF(place_rule),
    TOKDEF(placement),
    TOKDEF(plan),
    TOKDEF(plane),
    TOKDEF(pn),
    TOKDEF(point),
    TOKDEF(polyline_path),      // used by freerouting.com
    TOKDEF(polygon),
    TOKDEF(position),
    TOKDEF(positive_diagonal),
    TOKDEF(power),
    TOKDEF(power_dissipation),
    TOKDEF(power_fanout),
    TOKDEF(prefix),
    TOKDEF(primary),
    TOKDEF(priority),
    TOKDEF(property),
    TOKDEF(protect),
    TOKDEF(qarc),
    TOKDEF(quarter),
    TOKDEF(radius),
    TOKDEF(ratio),
    TOKDEF(ratio_tolerance),
    TOKDEF(rect),
    TOKDEF(reduced),
    TOKDEF(region),
    TOKDEF(region_class),
    TOKDEF(region_class_class),
    TOKDEF(region_net),
    TOKDEF(relative_delay),
    TOKDEF(relative_group_delay),
    TOKDEF(relative_group_length),
    TOKDEF(relative_length),
    TOKDEF(reorder),
    TOKDEF(reroute_order_viols),
    TOKDEF(resistance_resolution),
    TOKDEF(resistor),
    TOKDEF(resolution),
    TOKDEF(restricted_layer_length_factor),
    TOKDEF(room),
    TOKDEF(rotate),
    TOKDEF(rotate_first),
    TOKDEF(round),
    TOKDEF(roundoff_rotation),
    TOKDEF(route),
    TOKDEF(route_to_fanout_only),
    TOKDEF(routes),
    TOKDEF(routes_include),
    TOKDEF(rule),
    TOKDEF(same_net_checking),
    TOKDEF(sample_window),
    TOKDEF(saturation_length),
    TOKDEF(sec),
    TOKDEF(secondary),
    TOKDEF(self),
    TOKDEF(sequence_number),
    TOKDEF(session),
    TOKDEF(set_color),
    TOKDEF(set_pattern),
    TOKDEF(shape),
    TOKDEF(shield),
    TOKDEF(shield_gap),
    TOKDEF(shield_loop),
    TOKDEF(shield_tie_down_interval),
    TOKDEF(shield_width),
    TOKDEF(side),
    TOKDEF(signal),
    TOKDEF(site),
    TOKDEF(small),
    TOKDEF(smd),
    TOKDEF(snap),
    TOKDEF(snap_angle),
    TOKDEF(soft),
    TOKDEF(source),
    TOKDEF(space_in_quoted_tokens),
    TOKDEF(spacing),
    TOKDEF(spare),
    TOKDEF(spiral_via),
    TOKDEF(square),
    TOKDEF(stack_via),
    TOKDEF(stack_via_depth),
    TOKDEF(standard),
    TOKDEF(starburst),
    TOKDEF(status),
    TOKDEF(structure),
    TOKDEF(structure_out),
    TOKDEF(subgate),
    TOKDEF(subgates),
    TOKDEF(substituted),
    TOKDEF(such),
    TOKDEF(suffix),
    TOKDEF(super_placement),
    TOKDEF(supply),
    TOKDEF(supply_pin),
    TOKDEF(swapping),
    TOKDEF(switch_window),
    TOKDEF(system),
    TOKDEF(tandem_noise),
    TOKDEF(tandem_segment),
    TOKDEF(tandem_shield_overhang),
    TOKDEF(terminal),
    TOKDEF(terminator),
    TOKDEF(term_only),
    TOKDEF(test),
    TOKDEF(test_points),
    TOKDEF(testpoint),
    TOKDEF(threshold),
    TOKDEF(time_length_factor),
    TOKDEF(time_resolution),
    TOKDEF(tjunction),
    TOKDEF(tolerance),
    TOKDEF(top),
    TOKDEF(topology),
    TOKDEF(total),
    TOKDEF(track_id),
    TOKDEF(turret),
    TOKDEF(type),
    TOKDEF(um),
    TOKDEF(unassigned),
    TOKDEF(unconnects),
    TOKDEF(unit),
    TOKDEF(up),
    TOKDEF(use_array),
    TOKDEF(use_layer),
    TOKDEF(use_net),
    TOKDEF(use_via),
    TOKDEF(value),
    TOKDEF(vertical),
    TOKDEF(via),
    TOKDEF(via_array_template),
    TOKDEF(via_at_smd),
    TOKDEF(via_keepout),
    TOKDEF(via_number),
    TOKDEF(via_rotate_first),
    TOKDEF(via_site),
    TOKDEF(via_size),
    TOKDEF(virtual_pin),
    TOKDEF(volt),
    TOKDEF(voltage_resolution),
    TOKDEF(was_is),
    TOKDEF(way),
    TOKDEF(weight),
    TOKDEF(width),
    TOKDEF(window),
    TOKDEF(wire),
    TOKDEF(wire_keepout),
    TOKDEF(wires),
    TOKDEF(wires_include),
    TOKDEF(wiring),
    TOKDEF(write_resolution),
    TOKDEF(x),
    TOKDEF(xy),
    TOKDEF(y),
};

const unsigned SPECCTRA_DB::keywordCount = DIM(SPECCTRA_DB::keywords);


//-----<SPECCTRA_DB>-------------------------------------------------

const char* SPECCTRA_DB::TokenName( int aTok )
{
    const char* ret;

    if( (unsigned) aTok < keywordCount )
    {
        ret = keywords[aTok].name;
    }
    else if( aTok < 0 )
    {
        return DSNLEXER::Syntax( aTok );
    }
    else
        ret = "token too big";

    return ret;
}

const char* GetTokenText( int aTok )
{
    return SPECCTRA_DB::TokenName( aTok );
}

wxString SPECCTRA_DB::GetTokenString( int aTok )
{
    wxString    ret;

    ret << wxT("'") << CONV_FROM_UTF8( GetTokenText(aTok) ) << wxT("'");

    return ret;
}

void SPECCTRA_DB::buildLayerMaps( BOARD* aBoard )
{
    // specctra wants top physical layer first, then going down to the
    // bottom most physical layer in physical sequence.
    // @question : why does Kicad not display layers in that order?
    int layerCount = aBoard->GetCopperLayerCount();

    layerIds.clear();
    pcbLayer2kicad.resize( layerCount );
    kicadLayer2pcb.resize( LAYER_N_FRONT+1 );

    for( int kiNdx=layerCount-1, pcbNdx=0;  kiNdx >= 0;  --kiNdx, ++pcbNdx )
    {
        int kilayer = kiNdx>0 && kiNdx==layerCount-1 ? LAYER_N_FRONT : kiNdx;

        // establish bi-directional mapping between kicad's BOARD layer and PCB layer
        pcbLayer2kicad[pcbNdx]  = kilayer;
        kicadLayer2pcb[kilayer] = pcbNdx;

        // save the specctra layer name in SPECCTRA_DB::layerIds for later.
        layerIds.push_back( CONV_TO_UTF8( aBoard->GetLayerName( kilayer ) ) );
    }
}


int SPECCTRA_DB::findLayerName( const std::string& aLayerName ) const
{
    for( unsigned i=0;  i<layerIds.size();  ++i )
    {
        if( 0 == aLayerName.compare( layerIds[i] ) )
            return (int) i;
    }
    return -1;
}


void SPECCTRA_DB::ThrowIOError( const wxChar* fmt, ... ) throw( IO_ERROR )
{
    wxString    errText;
    va_list     args;

    va_start( args, fmt );
    errText.PrintfV( fmt, args );
    va_end( args );

    throw IO_ERROR( errText );
}


void SPECCTRA_DB::expecting( const char* text ) throw( IO_ERROR )
{
    wxString    errText = CONV_FROM_UTF8( text );
    lexer->Expecting( errText );
}


void SPECCTRA_DB::unexpected( const char* text ) throw( IO_ERROR )
{
    wxString    errText = CONV_FROM_UTF8( text );
    lexer->Unexpected( errText );
}


DSN_T SPECCTRA_DB::nextTok()
{
    DSN_T   ret = (DSN_T) lexer->NextTok();
    return ret;
}

void SPECCTRA_DB::readCOMPnPIN( std::string* component_id, std::string* pin_id ) throw( IO_ERROR )
{
    DSN_T  tok;

    static const char pin_def[] = "<pin_reference>::=<component_id>-<pin_id>";

    if( !isSymbol( (DSN_T) lexer->CurTok() ) )
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


void SPECCTRA_DB::readTIME( time_t* time_stamp ) throw( IO_ERROR )
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

    mytime.tm_mon = 0;      // remains if we don't find a month match.
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


void SPECCTRA_DB::LoadPCB( const wxString& filename ) throw( IO_ERROR )
{
    FILE*       fp = wxFopen( filename, wxT("r") );

    if( !fp )
    {
        ThrowIOError( _("Unable to open file \"%s\""), GetChars(filename) );
    }

    delete lexer;
    lexer = 0;

    lexer = new DSNLEXER( SPECCTRA_DB::keywords, SPECCTRA_DB::keywordCount, fp, filename );

    if( nextTok() != T_LEFT )
        expecting( T_LEFT );

    if( nextTok() != T_pcb )
        expecting( T_pcb );

    SetPCB( new PCB() );

    doPCB( pcb );

    delete lexer;       // close the file.
    lexer = 0;
}


void SPECCTRA_DB::LoadSESSION( const wxString& filename ) throw( IO_ERROR )
{
    FILE*       fp = wxFopen( filename, wxT("r") );

    if( !fp )
    {
        ThrowIOError( _("Unable to open file \"%s\""), GetChars(filename) );
    }

    delete lexer;

    lexer = new DSNLEXER( SPECCTRA_DB::keywords, SPECCTRA_DB::keywordCount, fp, filename );

    if( nextTok() != T_LEFT )
        expecting( T_LEFT );

    if( nextTok() != T_session )
        expecting( T_session );

    SetSESSION( new SESSION() );

    doSESSION( session );

    delete lexer;       // close the file.
    lexer = 0;
}


void SPECCTRA_DB::doPCB( PCB* growth ) throw( IO_ERROR )
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


void SPECCTRA_DB::doPARSER( PARSER* growth ) throw( IO_ERROR )
{
    DSN_T       tok;
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

    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );

        tok = nextTok();
        switch( tok )
        {
        case T_STRING_QUOTE:
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
            needSYMBOLorNUMBER();
            growth->host_version = lexer->CurText();
            needRIGHT();
            break;

        case T_constant:
            needSYMBOLorNUMBER();
            const1 = lexer->CurText();
            needSYMBOLorNUMBER();
            const2 = lexer->CurText();
            needRIGHT();
            growth->constants.push_back( const1 );
            growth->constants.push_back( const2 );
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


void SPECCTRA_DB::doRESOLUTION( UNIT_RES* growth ) throw( IO_ERROR )
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


void SPECCTRA_DB::doUNIT( UNIT_RES* growth ) throw( IO_ERROR )
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


void SPECCTRA_DB::doLAYER_PAIR( LAYER_PAIR* growth ) throw( IO_ERROR )
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


void SPECCTRA_DB::doLAYER_NOISE_WEIGHT( LAYER_NOISE_WEIGHT* growth ) throw( IO_ERROR )
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


void SPECCTRA_DB::doSTRUCTURE( STRUCTURE* growth ) throw( IO_ERROR )
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
            if( growth->layer_noise_weight )
                unexpected( tok );
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
            if( growth->via )
                unexpected( tok );
            growth->via = new VIA( growth );
            doVIA( growth->via );
            break;

        case T_control:
            if( growth->control )
                unexpected( tok );
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


void SPECCTRA_DB::doSTRUCTURE_OUT( STRUCTURE_OUT* growth ) throw( IO_ERROR )
{
    /*
    <structure_out_descriptor >::=
        (structure_out
            {<layer_descriptor> }
            [<rule_descriptor> ]
        )
    */

    DSN_T   tok = nextTok();

    while( tok != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );

        tok = nextTok();
        switch( tok )
        {
        case T_layer:
            LAYER* layer;
            layer = new LAYER( growth );
            growth->layers.push_back( layer );
            doLAYER( layer );
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
    }
}


void SPECCTRA_DB::doKEEPOUT( KEEPOUT* growth ) throw( IO_ERROR )
{
    DSN_T   tok = nextTok();

    if( isSymbol(tok) )
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
            if( growth->shape )
                unexpected( tok );
            growth->shape = new RECTANGLE( growth );
            doRECTANGLE( (RECTANGLE*) growth->shape );
            break;

        case T_circle:
            if( growth->shape )
                unexpected( tok );
            growth->shape = new CIRCLE( growth );
            doCIRCLE( (CIRCLE*) growth->shape );
            break;

        case T_polyline_path:
            tok = T_path;
        case T_path:
        case T_polygon:
            if( growth->shape )
                unexpected( tok );
            growth->shape = new PATH( growth, tok );
            doPATH( (PATH*) growth->shape );
            break;

        case T_qarc:
            if( growth->shape )
                unexpected( tok );
            growth->shape = new QARC( growth );
            doQARC( (QARC*) growth->shape );
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


void SPECCTRA_DB::doWINDOW( WINDOW* growth ) throw( IO_ERROR )
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
            if( growth->shape )
                unexpected( tok );
            growth->shape = new RECTANGLE( growth );
            doRECTANGLE( (RECTANGLE*) growth->shape );
            break;

        case T_circle:
            if( growth->shape )
                unexpected( tok );
            growth->shape = new CIRCLE( growth );
            doCIRCLE( (CIRCLE*) growth->shape );
            break;

        case T_polyline_path:
            tok = T_path;
        case T_path:
        case T_polygon:
            if( growth->shape )
                unexpected( tok );
            growth->shape = new PATH( growth, tok );
            doPATH( (PATH*) growth->shape );
            break;

        case T_qarc:
            if( growth->shape )
                unexpected( tok );
            growth->shape = new QARC( growth );
            doQARC( (QARC*) growth->shape );
            break;

        default:
            unexpected( lexer->CurText() );
        }

        tok = nextTok();
    }
}


void SPECCTRA_DB::doBOUNDARY( BOUNDARY* growth ) throw( IO_ERROR )
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


void SPECCTRA_DB::doPATH( PATH* growth ) throw( IO_ERROR )
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


void SPECCTRA_DB::doRECTANGLE( RECTANGLE* growth ) throw( IO_ERROR )
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


void SPECCTRA_DB::doCIRCLE( CIRCLE* growth ) throw( IO_ERROR )
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


void SPECCTRA_DB::doQARC( QARC* growth ) throw( IO_ERROR )
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


void SPECCTRA_DB::doSTRINGPROP( STRINGPROP* growth ) throw( IO_ERROR )
{
    needSYMBOL();
    growth->value = lexer->CurText();
    needRIGHT();
}


void SPECCTRA_DB::doTOKPROP( TOKPROP* growth ) throw( IO_ERROR )
{
    DSN_T tok = nextTok();

    if( tok<0 )
        unexpected( lexer->CurText() );

    growth->value = tok;

    needRIGHT();
}


void SPECCTRA_DB::doVIA( VIA* growth ) throw( IO_ERROR )
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


void SPECCTRA_DB::doCONTROL( CONTROL* growth ) throw( IO_ERROR )
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


void SPECCTRA_DB::doPROPERTIES( PROPERTIES* growth ) throw( IO_ERROR )
{
    DSN_T       tok;
    PROPERTY    property;  // construct it once here, append multiple times.

    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );

        needSYMBOLorNUMBER();
        property.name = lexer->CurText();

        needSYMBOLorNUMBER();
        property.value = lexer->CurText();

        growth->push_back( property );

        needRIGHT();
    }
}


void SPECCTRA_DB::doLAYER( LAYER* growth ) throw( IO_ERROR )
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


void SPECCTRA_DB::doRULE( RULE* growth ) throw( IO_ERROR )
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
void SPECCTRA_DB::doPLACE_RULE( PLACE_RULE* growth, bool expect_object_type ) throw( IO_ERROR )
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


void SPECCTRA_DB::doREGION( REGION* growth ) throw( IO_ERROR )
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


void SPECCTRA_DB::doCLASS_CLASS( CLASS_CLASS* growth ) throw( IO_ERROR )
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


void SPECCTRA_DB::doCLASSES( CLASSES* growth ) throw( IO_ERROR )
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


void SPECCTRA_DB::doGRID( GRID* growth ) throw( IO_ERROR )
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


void SPECCTRA_DB::doLAYER_RULE( LAYER_RULE* growth ) throw( IO_ERROR )
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


void SPECCTRA_DB::doPLACE( PLACE* growth ) throw( IO_ERROR )
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
        if( tok != T_LEFT )
            expecting( T_LEFT );

        tok = nextTok();
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
            needSYMBOLorNUMBER();
            growth->part_number = lexer->CurText();
            needRIGHT();
            break;

        default:
            unexpected( tok );
        }
    }
}


void SPECCTRA_DB::doCOMPONENT( COMPONENT* growth ) throw( IO_ERROR )
{
    DSN_T   tok = nextTok();

    if( !isSymbol( tok ) && tok != T_NUMBER )
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


void SPECCTRA_DB::doPLACEMENT( PLACEMENT* growth ) throw( IO_ERROR )
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


void SPECCTRA_DB::doPADSTACK( PADSTACK* growth ) throw( IO_ERROR )
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


void SPECCTRA_DB::doSHAPE( SHAPE* growth ) throw( IO_ERROR )
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
        case T_polyline_path:
            tok = T_path;
        case T_rect:
        case T_circle:
        case T_path:
        case T_polygon:
        case T_qarc:
L_done_that:
            if( growth->shape )
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


void SPECCTRA_DB::doIMAGE( IMAGE* growth ) throw( IO_ERROR )
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

    if( !isSymbol( tok ) && tok != T_NUMBER  )
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


void SPECCTRA_DB::doPIN( PIN* growth ) throw( IO_ERROR )
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

    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok == T_LEFT )
        {
            tok = nextTok();
            if( tok != T_rotate )
                expecting( T_rotate );

            if( nextTok() != T_NUMBER )
                expecting( T_NUMBER );
            growth->SetRotation( strtod( lexer->CurText(), 0 ) );
            needRIGHT();
        }
        else
        {
            if( !isSymbol(tok) && tok!=T_NUMBER )
                expecting( "pin_id" );

            growth->pin_id = lexer->CurText();

            if( nextTok() != T_NUMBER )
                expecting( T_NUMBER );
            growth->vertex.x = strtod( lexer->CurText(), 0 );

            if( nextTok() != T_NUMBER )
                expecting( T_NUMBER );
            growth->vertex.y = strtod( lexer->CurText(), 0 );
        }
    }
}


void SPECCTRA_DB::doLIBRARY( LIBRARY* growth ) throw( IO_ERROR )
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
            padstack = new PADSTACK();
            growth->AddPadstack( padstack );
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


void SPECCTRA_DB::doNET( NET* growth ) throw( IO_ERROR )
{
    DSN_T       tok = nextTok();
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
            pin_refs = &growth->pins;
            goto L_pins;

        case T_expose:
            pin_refs = &growth->expose;
            goto L_pins;

        case T_noexpose:
            pin_refs = &growth->noexpose;
            goto L_pins;

        case T_source:
            pin_refs = &growth->source;
            goto L_pins;

        case T_load:
            pin_refs = &growth->load;
            goto L_pins;

        case T_terminator:
            pin_refs = &growth->terminator;
            //goto L_pins;

L_pins:
            {
                PIN_REF     empty( growth );
                while( (tok = nextTok()) != T_RIGHT )
                {
                    // copy the empty one, then fill its copy later thru pin_ref.
                    pin_refs->push_back( empty );

                    PIN_REF* pin_ref = &pin_refs->back();

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


void SPECCTRA_DB::doTOPOLOGY( TOPOLOGY* growth ) throw( IO_ERROR )
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


void SPECCTRA_DB::doCLASS( CLASS* growth ) throw( IO_ERROR )
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

        case T_circuit:  // handle all the circuit_descriptor here as strings
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
                        DSN_T prevTok = (DSN_T) lexer->PrevTok();

                        if( prevTok!=T_LEFT && prevTok!=T_circuit && tok!=T_RIGHT )
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
                        growth->circuit.push_back( builder );
                        break;
                    }
                }

                if( tok==T_EOF )
                    unexpected( T_EOF );
            }                                   // scope bracket
            break;

        default:
            unexpected( lexer->CurText() );
        }                                       // switch

        tok = nextTok();

    } // while
}


void SPECCTRA_DB::doNETWORK( NETWORK* growth ) throw( IO_ERROR )
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


void SPECCTRA_DB::doCOMP_ORDER( COMP_ORDER* growth ) throw( IO_ERROR )
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


void SPECCTRA_DB::doFROMTO( FROMTO* growth ) throw( IO_ERROR )
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


void SPECCTRA_DB::doWIRE( WIRE* growth ) throw( IO_ERROR )
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
            if( growth->shape )
                unexpected( tok );
            growth->shape = new RECTANGLE( growth );
            doRECTANGLE( (RECTANGLE*) growth->shape );
            break;

        case T_circle:
            if( growth->shape )
                unexpected( tok );
            growth->shape = new CIRCLE( growth );
            doCIRCLE( (CIRCLE*) growth->shape );
            break;

        case T_polyline_path:
            tok = T_path;
        case T_path:
        case T_polygon:
            if( growth->shape )
                unexpected( tok );
            growth->shape = new PATH( growth, tok );
            doPATH( (PATH*) growth->shape );
            break;

        case T_qarc:
            if( growth->shape )
                unexpected( tok );
            growth->shape = new QARC( growth );
            doQARC( (QARC*) growth->shape );
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
            growth->wire_type = tok;
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


void SPECCTRA_DB::doWIRE_VIA( WIRE_VIA* growth ) throw( IO_ERROR )
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
            growth->via_type = tok;
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


void SPECCTRA_DB::doWIRING( WIRING* growth ) throw( IO_ERROR )
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


void SPECCTRA_DB::doANCESTOR( ANCESTOR* growth ) throw( IO_ERROR )
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


void SPECCTRA_DB::doHISTORY( HISTORY* growth ) throw( IO_ERROR )
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


void SPECCTRA_DB::doSESSION( SESSION* growth ) throw( IO_ERROR )
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


void SPECCTRA_DB::doWAS_IS( WAS_IS* growth ) throw( IO_ERROR )
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


void SPECCTRA_DB::doROUTE( ROUTE* growth ) throw( IO_ERROR )
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

        case T_structure_out:
            if( growth->structure_out )
                unexpected( tok );
            growth->structure_out = new STRUCTURE_OUT( growth );
            doSTRUCTURE_OUT( growth->structure_out );
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


void SPECCTRA_DB::doNET_OUT( NET_OUT* growth ) throw( IO_ERROR )
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


void SPECCTRA_DB::doSUPPLY_PIN( SUPPLY_PIN* growth ) throw( IO_ERROR )
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


void SPECCTRA_DB::ExportPCB( wxString filename, bool aNameChange ) throw( IO_ERROR )
{
    if( pcb )
    {
        wxFFileOutputStream os( filename, wxT( "wt" ) );

        if( !os.IsOk() )
        {
            ThrowIOError( _("Unable to open file \"%s\""), GetChars(filename) );
        }

        STREAM_OUTPUTFORMATTER  outputFormatter( os, quote_char[0] );

        if( aNameChange )
            pcb->pcbname = CONV_TO_UTF8(filename);

        pcb->Format( &outputFormatter, 0 );
    }
}


void SPECCTRA_DB::ExportSESSION( wxString filename )
{
    if( session )
    {
        wxFFileOutputStream os( filename, wxT( "wt" ) );

        if( !os.IsOk() )
        {
            ThrowIOError( _("Unable to open file \"%s\""), GetChars(filename) );
        }

        STREAM_OUTPUTFORMATTER  outputFormatter( os, quote_char[0] );

        session->Format( &outputFormatter, 0 );
    }
}


PCB* SPECCTRA_DB::MakePCB()
{
    PCB*    pcb = new PCB();

    pcb->parser = new PARSER( pcb );
    pcb->resolution = new UNIT_RES( pcb, T_resolution );
    pcb->unit = new UNIT_RES( pcb, T_unit );

    pcb->structure = new STRUCTURE( pcb );
    pcb->structure->boundary = new BOUNDARY( pcb->structure );
    pcb->structure->via = new VIA( pcb->structure );
    pcb->structure->rules = new RULE( pcb->structure, T_rule );

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


void ELEM::Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IO_ERROR )
{
    out->Print( nestLevel, "(%s\n", Name() );

    FormatContents( out, nestLevel+1 );

    out->Print( nestLevel, ")\n" );
}


void ELEM_HOLDER::FormatContents( OUTPUTFORMATTER* out, int nestLevel ) throw( IO_ERROR )
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


// a reasonably small memory price to pay for improved performance
STRING_FORMATTER  ELEM::sf;


//-----<UNIT_RES>---------------------------------------------------------

UNIT_RES UNIT_RES::Default( NULL, T_resolution );


//-----<PADSTACK>---------------------------------------------------------

int PADSTACK::Compare( PADSTACK* lhs, PADSTACK* rhs )
{
    // printf( "PADSTACK::Compare( %p, %p)\n", lhs, rhs );

    if( !lhs->hash.size() )
        lhs->hash = lhs->makeHash();

    if( !rhs->hash.size() )
        rhs->hash = rhs->makeHash();

    int result = lhs->hash.compare( rhs->hash );
    if( result )
        return result;

    // Via names hold the drill diameters, so we have to include those to discern
    // between two vias with same copper size but with different drill sizes.
    result = lhs->padstack_id.compare( rhs->padstack_id );

    return result;
}


//-----<IMAGE>------------------------------------------------------------

int IMAGE::Compare( IMAGE* lhs, IMAGE* rhs )
{
    if( !lhs->hash.size() )
        lhs->hash = lhs->makeHash();

    if( !rhs->hash.size() )
        rhs->hash = rhs->makeHash();

    int result = lhs->hash.compare( rhs->hash );

    // printf("\"%s\"  \"%s\" ret=%d\n", lhs->hash.c_str(), rhs->hash.c_str(), result );

    return result;
}


//-----<COMPONENT>--------------------------------------------------------

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
    wxString msg = GetBuildVersion();
    host_version = CONV_TO_UTF8(msg);
}


void PARSER::FormatContents( OUTPUTFORMATTER* out, int nestLevel ) throw( IO_ERROR )
{
    out->Print( nestLevel, "(string_quote %c)\n", string_quote );
    out->Print( nestLevel, "(space_in_quoted_tokens %s)\n", space_in_quoted_tokens ? "on" : "off" );
    out->Print( nestLevel, "(host_cad \"%s\")\n", host_cad.c_str() );
    out->Print( nestLevel, "(host_version \"%s\")\n", host_version.c_str() );

    for( STRINGS::iterator i=constants.begin();  i!=constants.end();  )
    {
        const std::string& s1 = *i++;
        const std::string& s2 = *i++;

        const char* q1 = out->GetQuoteChar( s1.c_str() );
        const char* q2 = out->GetQuoteChar( s2.c_str() );
        out->Print( nestLevel, "(constant %s%s%s %s%s%s)\n",
            q1, s1.c_str(), q1,
            q2, s2.c_str(), q2 );
    }

    if( routes_include_testpoint || routes_include_guides || routes_include_image_conductor )
        out->Print( nestLevel, "(routes_include%s%s%s)\n",
                   routes_include_testpoint ? " testpoint" : "",
                   routes_include_guides ? " guides" : "",
                   routes_include_image_conductor ? " image_conductor" : "");

    if( wires_include_testpoint )
        out->Print( nestLevel, "(wires_include testpoint)\n" );

    if( !via_rotate_first )
        out->Print( nestLevel, "(via_rotate_first off)\n" );

    if( case_sensitive )
        out->Print( nestLevel, "(case_sensitive %s)\n", case_sensitive ? "on" : "off" );
}


void PLACE::Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IO_ERROR )
{
    bool        useMultiLine;

    const char* quote = out->GetQuoteChar( component_id.c_str() );

    if( place_rules || properties.size() || rules || region )
    {
        useMultiLine = true;

        out->Print( nestLevel, "(%s %s%s%s\n", Name(),
                                quote, component_id.c_str(), quote );

        out->Print( nestLevel+1, "%s", "" );
    }
    else
    {
        useMultiLine = false;

        out->Print( nestLevel, "(%s %s%s%s", Name(),
                                quote, component_id.c_str(), quote );
    }

    if( hasVertex )
    {
        out->Print( 0, " %.6g %.6g", vertex.x, vertex.y );

        out->Print( 0, " %s", GetTokenText( side ) );

        out->Print( 0, " %.6g", rotation );
    }

    const char* space = " ";    // one space, as c string.

    if( mirror != T_NONE )
    {
        out->Print( 0, "%s(mirror %s)", space, GetTokenText( mirror ) );
        space = "";
    }

    if( status != T_NONE )
    {
        out->Print( 0, "%s(status %s)", space, GetTokenText( status ) );
        space = "";
    }

    if( logical_part.size() )
    {
        quote = out->GetQuoteChar( logical_part.c_str() );
        out->Print( 0, "%s(logical_part %s%s%s)", space,
                   quote, logical_part.c_str(), quote );
        space = "";
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
            out->Print( nestLevel+1, "(lock_type %s)\n", GetTokenText(lock_type) );
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
    {
        if( lock_type != T_NONE )
        {
            out->Print( 0, "%s(lock_type %s)", space, GetTokenText(lock_type) );
            space = "";
        }

        if( part_number.size() )
        {
            const char* quote = out->GetQuoteChar( part_number.c_str() );
            out->Print( 0, "%s(PN %s%s%s)", space,
                       quote, part_number.c_str(), quote );
        }
    }

    out->Print( 0, ")\n" );
}

} // namespace DSN


//EOF
