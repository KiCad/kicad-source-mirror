
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


#include <cstdarg>
#include <cstdio>
#include <cstdlib>         // bsearch()
#include <cctype>


#include "dsnlexer.h"

#include "fctsys.h"
#include "pcbnew.h"

//#define STANDALONE  1       // enable this for stand alone testing.

static int compare( const void* a1, const void* a2 )
{
    const KEYWORD* k1 = (const KEYWORD*) a1;
    const KEYWORD* k2 = (const KEYWORD*) a2;

    int ret = strcmp( k1->name, k2->name );
    return ret;
}


//-----<DSNLEXER>-------------------------------------------------------------

void DSNLEXER::init()
{
    curTok = DSN_NONE;
    stringDelimiter = '"';

    space_in_quoted_tokens = true;

    commentsAreTokens = false;

    // "start" should never change until we change the reader.  The DSN
    // format spec supports an include file mechanism but we can add that later
    // using a std::stack to hold a stack of LINE_READERs to track nesting.
    start = (char*) (*reader);

    limit = start;
    next  = start;
}

DSNLEXER::DSNLEXER( FILE* aFile, const wxString& aFilename,
        const KEYWORD* aKeywordTable, unsigned aKeywordCount ) :
    keywords( aKeywordTable ),
    keywordCount( aKeywordCount )
{
    filename = aFilename;

    reader = new FILE_LINE_READER( aFile, 4096 );

    init();
}


DSNLEXER::DSNLEXER( const std::string& aClipboardTxt,
        const KEYWORD* aKeywordTable, unsigned aKeywordCount ) :
    keywords( aKeywordTable ),
    keywordCount( aKeywordCount )
{
    filename = _( "clipboard" );

    reader = new STRING_LINE_READER( aClipboardTxt );

    init();
}


int DSNLEXER::findToken( const std::string& tok )
{
    // convert to lower case once, this should be faster than using strcasecmp()
    // for each test in compare().
    lowercase.clear();

    for( std::string::const_iterator iter = tok.begin();  iter!=tok.end();  ++iter )
        lowercase += (char) tolower( *iter );

    KEYWORD search;

    search.name = lowercase.c_str();

    // a boost hashtable might be a few percent faster, depending on
    // hashtable size and quality of the hash function.

    const KEYWORD* findings = (const KEYWORD*) bsearch( &search,
                                   keywords, keywordCount,
                                   sizeof(KEYWORD), compare );
    if( findings )
        return findings->token;
    else
        return -1;
}


const char* DSNLEXER::Syntax( int aTok )
{
    const char* ret;

    switch( aTok )
    {
    case DSN_NONE:
        ret = "NONE";
        break;
    case DSN_STRING_QUOTE:
        ret = "string_quote";   // a special DSN syntax token, see specctra spec.
        break;
    case DSN_QUOTE_DEF:
        ret = "quoted text delimiter";
        break;
    case DSN_DASH:
        ret = "-";
        break;
    case DSN_SYMBOL:
        ret = "symbol";
        break;
    case DSN_NUMBER:
        ret = "number";
        break;
    case DSN_RIGHT:
        ret = ")";
        break;
    case DSN_LEFT:
        ret = "(";
        break;
    case DSN_STRING:
        ret = "quoted string";
        break;
    case DSN_EOF:
        ret = "end of file";
        break;
    default:
        ret = "???";
    }

    return ret;
}


const char* DSNLEXER::GetTokenText( int aTok )
{
    const char* ret;

    if( aTok < 0 )
    {
        return Syntax( aTok );
    }
    else if( (unsigned) aTok < keywordCount )
    {
        ret = keywords[aTok].name;
    }
    else
        ret = "token too big";

    return ret;
}


void DSNLEXER::ThrowIOError( wxString aText, int charOffset ) throw (IOError)
{
    // append to aText, do not overwrite
    aText << wxT(" ") << _("in") << wxT(" \"") << filename
          << wxT("\" ") << _("on line") << wxT(" ") << reader->LineNumber()
          << wxT(" ") << _("at offset") << wxT(" ") << charOffset;

    throw IOError( aText );
}


/**
 * Function isspace
 * strips the upper bits of the int to ensure the value passed to ::isspace() is
 * in the range of 0-255
 */
static inline bool isSpace( int cc )
{
    // make sure int passed to ::isspace() is 0-255
    return ::isspace( cc & 0xff );
}


int DSNLEXER::NextTok() throw (IOError)
{
    char*   cur  = next;
    char*   head = cur;

    prevTok = curTok;

    if( curTok != DSN_EOF )
    {
        if( cur >= limit )
        {
L_read:
            // blank lines are returned as "\n" and will have a len of 1.
            // EOF will have a len of 0 and so is detectable.
            int len = readLine();
            if( len == 0 )
            {
                curTok = DSN_EOF;
                goto exit;
            }

            cur = start;

            // skip leading whitespace
            while( cur<limit && isSpace(*cur) )
                ++cur;

            // If the first non-blank character is #, this line is a comment.
            // Comments cannot follow any other token on the same line.
            if( cur<limit && *cur=='#' )
            {
                if( commentsAreTokens )
                {
                    // save the entire line, including new line as the current token.
                    // the '#' character may not be at offset zero.
                    curText = start;        // entire line is the token
                    cur     = start;        // ensure a good curOffset below
                    curTok  = DSN_COMMENT;
                    head    = limit;        // do a readLine() on next call in here.
                    goto exit;
                }
                else
                    goto L_read;
            }
        }
        else
        {
            // skip leading whitespace
            while( cur<limit && isSpace(*cur) )
                ++cur;
        }

        if( cur >= limit )
            goto L_read;

        // switching the string_quote character
        if( prevTok == DSN_STRING_QUOTE )
        {
            static const wxString errtxt( _("String delimiter must be a single character of ', \", or $"));

            char cc = *cur;
            switch( cc )
            {
            case '\'':
            case '$':
            case '"':
                break;
            default:
                ThrowIOError( errtxt, CurOffset() );
            }

            curText = cc;

            head = cur+1;

            if( head<limit && *head!=')' && *head!='(' && !isSpace(*head) )
            {
                ThrowIOError( errtxt, CurOffset() );
            }

            curTok = DSN_QUOTE_DEF;
            goto exit;
        }

        if( *cur == '(' )
        {
            curText = *cur;
            curTok = DSN_LEFT;
            head = cur+1;
            goto exit;
        }

        if( *cur == ')' )
        {
            curText = *cur;
            curTok = DSN_RIGHT;
            head = cur+1;
            goto exit;
        }

        /*  get the dash out of a <pin_reference> which is embedded for example
            like:  U2-14 or "U2"-"14"
            This is detectable by a non-space immediately preceeding the dash.
        */
        if( *cur == '-' && cur>start && !isSpace( cur[-1] ) )
        {
            curText = '-';
            curTok = DSN_DASH;
            head = cur+1;
            goto exit;
        }

        // handle DSN_NUMBER
        if( strchr( "+-.0123456789", *cur ) )
        {
            head = cur+1;
            while( head<limit && strchr( ".0123456789", *head )  )
                ++head;

            if( (head<limit && isSpace(*head)) || *head==')' || *head=='(' || head==limit )
            {
                curText.clear();
                curText.append( cur, head );
                curTok = DSN_NUMBER;
                goto exit;
            }

            // else it was something like +5V, fall through below
        }

        // a quoted string
        if( *cur == stringDelimiter )
        {
            // New code, understands nested quotes, and deliberately restricts
            // strings to a single line. Still strips off leading and trailing
            // quotes, and now removes internal doubled up quotes
#if 1
            head = cur;

            // copy the token, character by character so we can remove doubled up quotes.
            curText.clear();

            while( head < limit )
            {
                if( *head==stringDelimiter )
                {
                    if( head+1<limit && head[1]==stringDelimiter )
                    {
                        // include only one of the doubled-up stringDelimiters
                        curText += *head;
                        head    += 2;
                        continue;
                    }
                    else if( head == cur )
                    {
                        ++head;     // skip the leading quote
                        continue;
                    }

                    // fall thru
                }

                // check for a terminator
                if( isStringTerminator( *head ) )
                {
                    curTok = DSN_STRING;
                    ++head;
                    goto exit;
                }

                curText += *head++;
            }

            wxString errtxt(_("Un-terminated delimited string") );
            ThrowIOError( errtxt, CurOffset() );

#else   // old code, did not understand nested quotes
            ++cur;  // skip over the leading delimiter: ",', or $

            head = cur;

            while( head<limit  &&  !isStringTerminator( *head ) )
                ++head;

            if( head >= limit )
            {
                wxString errtxt(_("Un-terminated delimited string") );
                ThrowIOError( errtxt, CurOffset() );
            }

            curText.clear();
            curText.append( cur, head );

            ++head;     // skip over the trailing delimiter

            curTok  = DSN_STRING;
            goto exit;
#endif
        }

        // Maybe it is a token we will find in the token table.
        // If not, then call it a DSN_SYMBOL.
        {
            head = cur+1;
            while( head<limit && !isSpace( *head ) && *head!=')' && *head!='(' )
                ++head;

            curText.clear();
            curText.append( cur, head );

            int found = findToken( curText );

            if( found != -1 )
                curTok = found;

            else if( 0 == curText.compare( "string_quote" ) )
                curTok = DSN_STRING_QUOTE;

            else                    // unrecogized token, call it a symbol
                curTok = DSN_SYMBOL;
        }
    }

exit:   // single point of exit, no returns elsewhere please.

    curOffset = cur - start;

    next = head;

    // printf("tok:\"%s\"\n", curText.c_str() );
    return curTok;
}


// stand alone testing
#if defined(STANDALONE)

#include <wx/dataobj.h>
#include <wx/clipbrd.h>


enum TEST_T {

    // these first few are negative special ones for syntax, and are
    // inherited from DSNLEXER.
    T_NONE = DSN_NONE,
    T_STRING_QUOTE = DSN_STRING_QUOTE,
    T_QUOTE_DEF = DSN_QUOTE_DEF,
    T_DASH = DSN_DASH,
    T_SYMBOL = DSN_SYMBOL,
    T_NUMBER = DSN_NUMBER,
    T_RIGHT = DSN_RIGHT,    // right bracket, ')'
    T_LEFT = DSN_LEFT,      // left bracket, '('
    T_STRING = DSN_STRING,  // a quoted string, stripped of the quotes
    T_EOF = DSN_EOF,        // special case for end of file


    // This should be coordinated with the
    // const static KEYWORD tokens[] array, and both must be sorted
    // identically and alphabetically.  Remember that '_' is less than any
    // alpha character according to ASCII.

    T_absolute = 0,        // this one should be == zero
    T_added,
    T_add_group,
    T_add_pins,
    T_allow_antenna,
    T_allow_redundant_wiring,
    T_amp,
    T_ancestor,
    T_antipad,
    T_aperture_type,
    T_array,
    T_attach,
    T_attr,
    T_average_pair_length,
    T_back,
    T_base_design,
    T_bbv_ctr2ctr,
    T_bend_keepout,
    T_bond,
    T_both,
    T_bottom,
    T_bottom_layer_sel,
    T_boundary,
    T_brickpat,
    T_bundle,
    T_bus,
    T_bypass,
    T_capacitance_resolution,
    T_capacitor,
    T_case_sensitive,
    T_cct1,
    T_cct1a,
    T_center_center,
    T_checking_trim_by_pin,
    T_circ,
    T_circle,
    T_circuit,
    T_class,
    T_class_class,
    T_classes,
    T_clear,
    T_clearance,
    T_cluster,
    T_cm,
    T_color,
    T_colors,
    T_comment,
    T_comp,
    T_comp_edge_center,
    T_comp_order,
    T_component,
    T_composite,
    T_conductance_resolution,
    T_conductor,
    T_conflict,
    T_connect,
    T_constant,
    T_contact,
    T_control,
    T_corner,
    T_corners,
    T_cost,
    T_created_time,
    T_cross,
    T_crosstalk_model,
    T_current_resolution,
    T_delete_pins,
    T_deleted,
    T_deleted_keepout,
    T_delta,
    T_diagonal,
    T_direction,
    T_directory,
    T_discrete,
    T_effective_via_length,
    T_elongate_keepout,
    T_exclude,
    T_expose,
    T_extra_image_directory,
    T_family,
    T_family_family,
    T_family_family_spacing,
    T_fanout,
    T_farad,
    T_file,
    T_fit,
    T_fix,
    T_flip_style,
    T_floor_plan,
    T_footprint,
    T_forbidden,
    T_force_to_terminal_point,
    T_free,
    T_forgotten,
    T_fromto,
    T_front,
    T_front_only,
    T_gap,
    T_gate,
    T_gates,
    T_generated_by_freeroute,
    T_global,
    T_grid,
    T_group,
    T_group_set,
    T_guide,
    T_hard,
    T_height,
    T_high,
    T_history,
    T_horizontal,
    T_host_cad,
    T_host_version,
    T_image,
    T_image_conductor,
    T_image_image,
    T_image_image_spacing,
    T_image_outline_clearance,
    T_image_set,
    T_image_type,
    T_inch,
    T_include,
    T_include_pins_in_crosstalk,
    T_inductance_resolution,
    T_insert,
    T_instcnfg,
    T_inter_layer_clearance,
    T_jumper,
    T_junction_type,
    T_keepout,
    T_kg,
    T_kohm,
    T_large,
    T_large_large,
    T_layer,
    T_layer_depth,
    T_layer_noise_weight,
    T_layer_pair,
    T_layer_rule,
    T_length,
    T_length_amplitude,
    T_length_factor,
    T_length_gap,
    T_library,
    T_library_out,
    T_limit,
    T_limit_bends,
    T_limit_crossing,
    T_limit_vias,
    T_limit_way,
    T_linear,
    T_linear_interpolation,
    T_load,
    T_lock_type,
    T_logical_part,
    T_logical_part_mapping,
    T_low,
    T_match_fromto_delay,
    T_match_fromto_length,
    T_match_group_delay,
    T_match_group_length,
    T_match_net_delay,
    T_match_net_length,
    T_max_delay,
    T_max_len,
    T_max_length,
    T_max_noise,
    T_max_restricted_layer_length,
    T_max_stagger,
    T_max_stub,
    T_max_total_delay,
    T_max_total_length,
    T_max_total_vias,
    T_medium,
    T_mhenry,
    T_mho,
    T_microvia,
    T_mid_driven,
    T_mil,
    T_min_gap,
    T_mirror,
    T_mirror_first,
    T_mixed,
    T_mm,
    T_negative_diagonal,
    T_net,
    T_net_number,
    T_net_out,
    T_net_pin_changes,
    T_nets,
    T_network,
    T_network_out,
    T_no,
    T_noexpose,
    T_noise_accumulation,
    T_noise_calculation,
    T_normal,
    T_object_type,
    T_off,
    T_off_grid,
    T_offset,
    T_on,
    T_open,
    T_opposite_side,
    T_order,
    T_orthogonal,
    T_outline,
    T_overlap,
    T_pad,
    T_pad_pad,
    T_padstack,
    T_pair,
    T_parallel,
    T_parallel_noise,
    T_parallel_segment,
    T_parser,
    T_part_library,
    T_path,
    T_pcb,
    T_permit_orient,
    T_permit_side,
    T_physical,
    T_physical_part_mapping,
    T_piggyback,
    T_pin,
    T_pin_allow,
    T_pin_cap_via,
    T_pin_via_cap,
    T_pin_width_taper,
    T_pins,
    T_pintype,
    T_place,
    T_place_boundary,
    T_place_control,
    T_place_keepout,
    T_place_rule,
    T_placement,
    T_plan,
    T_plane,
    T_pn,
    T_point,
    T_polyline_path,
    T_polygon,
    T_position,
    T_positive_diagonal,
    T_power,
    T_power_dissipation,
    T_power_fanout,
    T_prefix,
    T_primary,
    T_priority,
    T_property,
    T_protect,
    T_qarc,
    T_quarter,
    T_radius,
    T_ratio,
    T_ratio_tolerance,
    T_rect,
    T_reduced,
    T_region,
    T_region_class,
    T_region_class_class,
    T_region_net,
    T_relative_delay,
    T_relative_group_delay,
    T_relative_group_length,
    T_relative_length,
    T_reorder,
    T_reroute_order_viols,
    T_resistance_resolution,
    T_resistor,
    T_resolution,
    T_restricted_layer_length_factor,
    T_room,
    T_rotate,
    T_rotate_first,
    T_round,
    T_roundoff_rotation,
    T_route,
    T_route_to_fanout_only,
    T_routes,
    T_routes_include,
    T_rule,
    T_same_net_checking,
    T_sample_window,
    T_saturation_length,
    T_sec,
    T_secondary,
    T_self,
    T_sequence_number,
    T_session,
    T_set_color,
    T_set_pattern,
    T_shape,
    T_shield,
    T_shield_gap,
    T_shield_loop,
    T_shield_tie_down_interval,
    T_shield_width,
    T_side,
    T_signal,
    T_site,
    T_small,
    T_smd,
    T_snap,
    T_snap_angle,
    T_soft,
    T_source,
    T_space_in_quoted_tokens,
    T_spacing,
    T_spare,
    T_spiral_via,
    T_square,
    T_stack_via,
    T_stack_via_depth,
    T_standard,
    T_starburst,
    T_status,
    T_structure,
    T_structure_out,
    T_subgate,
    T_subgates,
    T_substituted,
    T_such,
    T_suffix,
    T_super_placement,
    T_supply,
    T_supply_pin,
    T_swapping,
    T_switch_window,
    T_system,
    T_tandem_noise,
    T_tandem_segment,
    T_tandem_shield_overhang,
    T_terminal,
    T_terminator,
    T_term_only,
    T_test,
    T_test_points,
    T_testpoint,
    T_threshold,
    T_time_length_factor,
    T_time_resolution,
    T_tjunction,
    T_tolerance,
    T_top,
    T_topology,
    T_total,
    T_track_id,
    T_turret,
    T_type,
    T_um,
    T_unassigned,
    T_unconnects,
    T_unit,
    T_up,
    T_use_array,
    T_use_layer,
    T_use_net,
    T_use_via,
    T_value,
    T_vertical,
    T_via,
    T_via_array_template,
    T_via_at_smd,
    T_via_keepout,
    T_via_number,
    T_via_rotate_first,
    T_via_site,
    T_via_size,
    T_virtual_pin,
    T_volt,
    T_voltage_resolution,
    T_was_is,
    T_way,
    T_weight,
    T_width,
    T_window,
    T_wire,
    T_wire_keepout,
    T_wires,
    T_wires_include,
    T_wiring,
    T_write_resolution,
    T_x,
    T_xy,
    T_y,
};


#define TOKDEF(x)    { #x, T_##x }

// This MUST be sorted alphabetically, and the order of enum DSN_T {} be
// identially alphabetized. These MUST all be lower case because of the
// conversion to lowercase in findToken().
static const KEYWORD keywords[] = {

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


/*  To run this test code, simply copy some DSN text to the clipboard, then run
    the program from the command line and it will beautify the input from the
    clipboard to stdout.  stderr gets errors, if any.
    The wxApp is involved because the clipboard is not available to a raw
    int main() type program on all platforms.
*/


class DSNTEST : public wxApp
{

    DSNLEXER*   lexer;
    int         nestLevel;

    void        recursion() throw( IOError );

    void        indent()
    {
        const int NESTWIDTH = 2;

        printf("\n");
        for( int i=0; i<nestLevel;  ++i )
            printf( "%*c", NESTWIDTH, ' ' );
    }


public:
    DSNTEST() :
        lexer(0),
        nestLevel(0)
    {}

    ~DSNTEST()
    {
        delete lexer;
    }

    virtual bool OnInit();
};


IMPLEMENT_APP( DSNTEST )

bool DSNTEST::OnInit()
{

#if 0   // file based LINE_READER.
    wxFFile     file;

    wxString    filename( wxT("/tmp/testdesigns/test.dsn") );

    FILE*   fp = wxFopen( filename, wxT("r") );

    if( !fp )
    {
        fprintf( stderr, "unable to open file \"%s\"\n",
                (const char*) filename.mb_str() );
        exit(1);
    }

    file.Attach( fp );      // "exception safe" way to close the file.

    // this won't compile without a token table.
    DSNLEXER  lexer( fp, filename, keywords, DIM(keywords) );

#else   // clipboard based line reader

    if( !wxTheClipboard->Open() )
    {
        fprintf( stderr, "unable to open clipboard\n" );
        exit( 1 );
    }

    wxTextDataObject    dataObj;

    if( !wxTheClipboard->GetData( dataObj ) )
    {
        fprintf( stderr, "nothing of interest on clipboard\n" );
        exit( 2 );
    }

    int formatCount = dataObj.GetFormatCount();

    fprintf( stderr, "formatCount:%d\n", formatCount );

    wxDataFormat* formats = new wxDataFormat[formatCount];

    dataObj.GetAllFormats( formats );

    for( int fmt=0;  fmt<formatCount;  ++fmt )
    {
        fprintf( stderr, "format:%d\n", formats[fmt].GetType() );
        // @todo: what are these formats in terms of enum strings, and how
        // do they vary across platforms.  I am seeing
        // on linux: 2 formats, 13 and 1
    }


    lexer = new DSNLEXER( std::string( CONV_TO_UTF8( dataObj.GetText() ) ),
        keywords, DIM(keywords) );

#endif

    // read the stream via the lexer, and use recursion to establish a nesting
    // level and some output.
    try
    {
        int         tok;
        while( (tok = lexer->NextTok()) != DSN_EOF )
        {
            if( tok == DSN_LEFT )
            {
                recursion();
            }
            else
                printf( " %s", lexer->CurText() );
        }
        printf("\n");
    }
    catch( IOError ioe )
    {
        fprintf( stderr, "%s\n", CONV_TO_UTF8( ioe.errorText ) );
    }

    return 0;
}


void DSNTEST::recursion() throw(IOError)
{
    int         tok;
    const char* space = "";

    indent();
    printf("(");

    while( (tok = lexer->NextTok()) != DSN_EOF && tok != DSN_RIGHT )
    {
        if( tok == DSN_LEFT )
        {
            ++nestLevel;

            recursion();

            --nestLevel;
        }
        else
            printf( "%s%s", space, lexer->CurText() );

        space = " ";    // only the first tok gets no leading space.
    }

    printf(")");
}

#endif
