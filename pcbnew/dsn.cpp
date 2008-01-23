
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


#include "dsn.h"

#include "fctsys.h"
#include "pcbnew.h"


/**
 * Class LEXER
 * implements a lexical analyzer for the SPECCTRA DSN file format.  It
 * reads lexical tokens from the current LINE_READER through the NextTok()
 * function.  The NextTok() function returns one of the DSN_T values.
 */


namespace DSN {


/**
 * Struct KEYWORD
 * holds a string and a DSN_T
 */
struct KEYWORD
{
    const char* name;
//    int         token;
};


//#define TOKDEF(x)    { #x, T_##x }
#define TOKDEF(x)    { #x }


// This MUST be sorted alphabetically, and also so MUST enum DSN_T {} be alphabetized.
// These MUST all be lower case because of the conversion to lowercase in findToken(). 
const static KEYWORD tokens[] = {
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
    TOKDEF(string_quote),
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


static int compare( const void* a1, const void* a2 )
{
    const KEYWORD* k1 = (const KEYWORD*) a1;
    const KEYWORD* k2 = (const KEYWORD*) a2;
    
    int ret = strcmp( k1->name, k2->name );
    return ret;
}


//-----<LINE_READER>------------------------------------------------------

LINE_READER::LINE_READER( FILE* aFile,  unsigned aMaxLineLength )
{
    fp = aFile;
    lineNum = 0;
    maxLineLength = aMaxLineLength;

    // the real capacity is 10 bytes larger than requested.    
    capacity = aMaxLineLength + 10;
    
    line = new char[capacity];
    
    line[0] = '\0';
    length  = 0;
}


int LINE_READER::ReadLine() throw (IOError)
{
    const char* p = fgets( line, capacity, fp );
    
    if( !p )
    {
        line[0] = 0;
        length  = 0;
    }
    else
    {
        length = strlen( line );

        if( length > maxLineLength )        
            throw IOError( _("Line length exceeded") );
        
        ++lineNum;
    }
    
    return length;
}


//-----<LEXER>-------------------------------------------------------------


LEXER::LEXER( FILE* aFile, const wxString& aFilename ) :
        reader( aFile, 4096 )
{
    curTok = T_END;
    stringDelimiter = '"';
    filename = aFilename;

    space_in_quoted_tokens = true;
    
    // "start" should never change until we change the reader.  The DSN
    // format spec supports an include file mechanism but we can add that later
    // using a std::stack to hold a stack of LINE_READERs to track nesting.
    start = (char*) reader;     
    
    limit = start;
    next  = start;
}


int LEXER::findToken( const std::string& tok )
{
    // convert to lower case once, this should be faster than using strcasecmp()
    // for each test in compare().
    lowercase.clear();
    
    for( std::string::const_iterator iter = tok.begin();  iter!=tok.end();  ++iter )
        lowercase += (char) tolower( *iter );
    
    KEYWORD search;
    search.name = lowercase.c_str();
    
    const KEYWORD* findings = (const KEYWORD*) bsearch( &search, 
                                   tokens, sizeof(tokens)/sizeof(tokens[0]),
                                   sizeof(KEYWORD), compare );
    if( findings )
//        return findings->token;
        return findings - tokens; 
    else
        return -1;
}


const char* LEXER::GetTokenText( DSN_T aTok )
{
    const char* ret;
    
    if( aTok < 0 )
    {
        switch( aTok )
        {
        case T_NONE:
            ret = "NONE";
            break;
        case T_QUOTE_DEF:
            ret = "quoted text delimiter";
            break;
        case T_DASH:
            ret = "-";
            break;
        case T_SYMBOL:
            ret = "symbol";
            break;
        case T_NUMBER:
            ret = "number";
            break;
        case T_RIGHT:
            ret = ")";
            break;
        case T_LEFT:
            ret = "(";
            break;
        case T_STRING:
            ret = "quoted string";
            break;
        case T_EOF:
            ret = "end of file";
            break;
        default:
            ret = "???";
        }
    }
    else
    {
        ret = tokens[aTok].name;
    }
    
    return ret;
}


wxString LEXER::GetTokenString( DSN_T aTok )
{
    wxString    ret;
    
    ret << wxT("'") << CONV_FROM_UTF8( GetTokenText(aTok) ) << wxT("'");

    return ret;    
}

    
void LEXER::ThrowIOError( wxString aText, int charOffset ) throw (IOError)
{
    aText << wxT(" ") << _("in file") << wxT(" \"") << filename 
          << wxT("\" ") << _("on line") << wxT(" ") << reader.LineNumber()
          << wxT(" ") << _("at offset") << wxT(" ") << charOffset;
          
    throw IOError( aText );
}

    
DSN_T LEXER::NextTok() throw (IOError)
{
    char*   cur  = next;
    char*   head = cur;

    prevTok = curTok;
    
    if( curTok != T_EOF )
    {
        if( cur >= limit )
        {
L_read:                
            int len = readLine();
            if( len == 0 )
            {
                curTok = T_EOF;
                goto exit;
            }
            
            cur = start;
            
            // skip leading whitespace
            while( cur<limit && isspace(*cur) )
                ++cur;

            // if the first non-blank character is #, this line is a comment.
            if( cur<limit && *cur=='#' )
                goto L_read;
        }
        else
        {
            // skip leading whitespace
            while( cur<limit && isspace(*cur) )
                ++cur;
        }

        if( cur >= limit )
            goto L_read;
        
        // switching the string_quote character
        if( prevTok == T_string_quote )
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
            
            curText.clear();
            curText += cc;
            
            head = cur+1;

            if( head<limit && *head!=')' && *head!='(' && !isspace(*head) )
            {
                ThrowIOError( errtxt, CurOffset() );
            }
            
            curTok = T_QUOTE_DEF;
            goto exit;
        }
        
        if( *cur == '(' )
        {
            curText.clear();
            curText += *cur;
            
            curTok = T_LEFT;
            head = cur+1;
            goto exit;
        }
        
        if( *cur == ')' )
        {
            curText.clear();
            curText += *cur;
            
            curTok = T_RIGHT;
            head = cur+1;
            goto exit;
        }

        /*  get the dash out of a <pin_reference> which is embedded for example
            like:  U2-14 or "U2"-"14"
            This is detectable by a non-space immediately preceeding the dash.
        */
        if( *cur == '-' && cur>start && !isspace( cur[-1] ) )
        {
            head = cur+1;
            curText.clear();
            curText += '-';
            curTok = T_DASH;
            goto exit;
        }
        
        // handle T_NUMBER                
        if( strchr( "+-.0123456789", *cur ) )
        {
            head = cur+1;
            while( head<limit && strchr( ".0123456789", *head )  )
                ++head;

            if( (head<limit && isspace(*head)) || *head==')' || *head=='(' || head==limit )
            {
                curText.clear();
                curText.append( cur, head );
                curTok = T_NUMBER;
                goto exit;
            }
            
            // else it was something like +5V, reset head back
        }
        
        // a quoted string
        if( *cur == stringDelimiter )
        {
            ++cur;  // skip over the leading delimiter: ",', or $

            head = cur;
        
            while( head<limit &&  !isStringTerminator( *head ) )
                ++head;
            
            if( head >= limit )
            {
                wxString errtxt(_("Un-terminated delimited string") );
                ThrowIOError( errtxt, CurOffset() );
            }
            
            curText.clear();
            curText.append( cur, head );
            
            ++head;     // skip over the trailing delimiter
            
            curTok  = T_STRING;
            goto exit;
        }
    
        // a token we hope to find in the tokens[] array.  If not, then
        // call it a T_SYMBOL.
        {
            head = cur+1;
            while( head<limit && !isspace( *head ) && *head!=')' && *head!='(' )
                ++head;
            
            curText.clear();
            curText.append( cur, head );
            
            int found = findToken( curText );
            
            if( found != -1 )
                curTok = (DSN_T) found;
            
            else                    // unrecogized token, call it a symbol
                curTok = T_SYMBOL;
        }
    }

exit:                   // single point of exit
    
    curOffset = cur - start;
    
    next = head;

    // printf("tok:\"%s\"\n", curText.c_str() );
    
    return curTok;
}


}   // namespace DSN



#if 0 && defined(STANDALONE)

// stand alone testing

int main( int argc, char** argv )
{

//  wxString    filename( wxT("/tmp/fpcroute/Sample_1sided/demo_1sided.dsn") );
    wxString    filename( wxT("/tmp/testdesigns/test.dsn") );
    
    FILE*   fp = wxFopen( filename, wxT("r") );
    
    if( !fp )
    {
        fprintf( stderr, "unable to open file \"%s\"\n", 
                (const char*) filename.mb_str() );
        exit(1);
    }
    
    DSN::LEXER  lexer( fp, filename );

    try 
    {
        int tok;
        while( (tok = lexer.NextTok()) != DSN::T_EOF )
        {
            printf( "%-3d %s\n", tok, lexer.CurText() );
        }
    }
    catch( DSN::IOError ioe )
    {
        printf( "%s\n", (const char*) ioe.errorText.mb_str() );
    }

    fclose( fp );
    
    return 0;
}

#endif
