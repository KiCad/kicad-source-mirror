
/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2007 Dick Hollenbeck, dick@softplc.com
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

 
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>         // bsearch()
#include <cctype>

#include "fctsys.h"
#include "pcbnew.h"
#include "common.h"

namespace DSN {

    
/**
 * Enum DSN_T
 * lists all the DSN lexer's tokens that are supported in lexing.  It is up
 * to the parser if it wants also to support them.
 */ 
enum DSN_T {
    
    // the first few are special
    T_DASH = -8,
    T_SYMBOL = -7,
    T_NUMBER = -6,
    T_NONE = -5,            // not a token
    T_RIGHT = -4,           // right bracket, ')'
    T_LEFT = -3,            // left bracket, '('
    T_STRING = -2,          // a quoted string, stripped of the quotes
    T_EOF = -1,             // special case for end of file

    // from here down, this list segment should be coordinated with the
    // const static KEYWORD tokens[] array below, and must be sorted
    // alphabetically.

    T_absolute,             // this one should be == zero
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
    T_base_design,
    T_bbv_ctr2ctr,
    T_bond,
    T_bottom,
    T_bottom_layer_sel,
    T_boundary,
    T_brickpat,
    T_bundle,
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
    T_component,
    T_comp_order,
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
    T_deleted_keepout,
    T_delete_pins,
    T_delta,
    T_direction,
    T_directory,
    T_effective_via_length,
    T_exclude,
    T_expose,
    T_extra_image_directory,
    T_family,
    T_family_family,
    T_family_family_spacing,
    T_farad,
    T_file,
    T_fit,
    T_fix,
    T_flip_style,
    T_floor_plan,
    T_footprint,
    T_forbidden,
    T_force_to_terminal_point,
    T_forgotten,
    T_fromto,
    T_front,
    T_front_only,
    T_gap,
    T_gates,
    T_global,
    T_grid,
    T_group,
    T_group_set,
    T_guide,
    T_hard,
    T_height,
    T_history,
    T_horizontal,
    T_host_cad,
    T_host_version,
    T_image,
    T_image_image,
    T_image_image_spacing,
    T_image_outline_clearance,
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
    T_mhenry,
    T_mho,
    T_microvia,
    T_mid_driven,
    T_mil,
    T_min_gap,
    T_mirror,
    T_mirror_first,
    T_mm,
    T_net,
    T_net_number,
    T_net_pin_changes,
    T_nets,
    T_network,
    T_network_out,
    T_no,
    T_noexpose,
    T_noise_accumulation,
    T_noise_calculation,
    T_object_type,
    T_off,
    T_off_grid,
    T_offset,
    T_on,
    T_open,
    T_opposite_side,
    T_order,
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
    T_pins,
    T_pintype,
    T_pin_via_cap,
    T_pin_width_taper,
    T_place,
    T_place_boundary,
    T_place_control,
    T_placement,
    T_place_rule,
    T_plan,
    T_plane,
    T_PN,
    T_point,
    T_polygon,
    T_position,
    T_power,
    T_power_dissipation,
    T_power_fanout,
    T_prefix,
    T_primary,
    T_priority,
    T_property,
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
    T_resolution,
    T_restricted_layer_length_factor,
    T_room,
    T_rotate,
    T_rotate_first,
    T_round,
    T_roundoff_rotation,
    T_route,
    T_routes,
    T_routes_include,
    T_route_to_fanout_only,
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
    T_smd,
    T_snap_angle,
    T_source,
    T_space_in_quoted_tokens,
    T_spacing,
    T_spare,
    T_spiral_via,
    T_stack_via,
    T_stack_via_depth,
    T_standard,
    T_starburst,
    T_status,
    T_string_quote,
    T_structure,
    T_structure_out,
    T_subgates,
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
    T_testpoint,
    T_test_points,
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
    T_wires,
    T_wires_include,
    T_wiring,
    T_write_resolution,
    T_x,
    T_END       // just a sentinel, not a token
};


/**
 * Struct KEYWORD
 * holds a string and a DSN_T
 */
struct KEYWORD
{
    const char* name;
    int         token;
};


#define TOKDEF(x)    { #x, T_##x },

// This MUST be sorted alphabetically, and also so MUST enum DSN_T {} be alphabetized.
// These MUST all be lower case because of the call to strlower() in findToken(). 
const static KEYWORD tokens[] = {
    TOKDEF(absolute)
    TOKDEF(added)
    TOKDEF(add_group)
    TOKDEF(add_pins)
    TOKDEF(allow_antenna)
    TOKDEF(allow_redundant_wiring)
    TOKDEF(amp)
    TOKDEF(ancestor)
    TOKDEF(antipad)
    TOKDEF(aperture_type)
    TOKDEF(array)
    TOKDEF(attach)
    TOKDEF(attr)
    TOKDEF(average_pair_length)
    TOKDEF(base_design)
    TOKDEF(bbv_ctr2ctr)
    TOKDEF(bond)
    TOKDEF(bottom)
    TOKDEF(bottom_layer_sel)
    TOKDEF(boundary)
    TOKDEF(brickpat)
    TOKDEF(bundle)
    TOKDEF(bypass)
    TOKDEF(capacitance_resolution)
    TOKDEF(capacitor)
    TOKDEF(case_sensitive)
    TOKDEF(cct1)
    TOKDEF(cct1a)
    TOKDEF(center_center)
    TOKDEF(checking_trim_by_pin)
    TOKDEF(circ)
    TOKDEF(circle)
    TOKDEF(circuit)
    TOKDEF(class)
    TOKDEF(class_class)
    TOKDEF(classes)
    TOKDEF(clear)
    TOKDEF(clearance)
    TOKDEF(cluster)
    TOKDEF(cm)
    TOKDEF(color)
    TOKDEF(colors)
    TOKDEF(comment)
    TOKDEF(comp)
    TOKDEF(comp_edge_center)
    TOKDEF(component)
    TOKDEF(comp_order)
    TOKDEF(composite)
    TOKDEF(conductance_resolution)
    TOKDEF(conductor)
    TOKDEF(conflict)
    TOKDEF(connect)
    TOKDEF(constant)
    TOKDEF(contact)
    TOKDEF(control)
    TOKDEF(corner)
    TOKDEF(corners)
    TOKDEF(cost)
    TOKDEF(created_time)
    TOKDEF(cross)
    TOKDEF(crosstalk_model)
    TOKDEF(current_resolution)
    TOKDEF(deleted_keepout)
    TOKDEF(delete_pins)
    TOKDEF(delta)
    TOKDEF(direction)
    TOKDEF(directory)
    TOKDEF(effective_via_length)
    TOKDEF(exclude)
    TOKDEF(expose)
    TOKDEF(extra_image_directory)
    TOKDEF(family)
    TOKDEF(family_family)
    TOKDEF(family_family_spacing)
    TOKDEF(farad)
    TOKDEF(file)
    TOKDEF(fit)
    TOKDEF(fix)
    TOKDEF(flip_style)
    TOKDEF(floor_plan)
    TOKDEF(footprint)
    TOKDEF(forbidden)
    TOKDEF(force_to_terminal_point)
    TOKDEF(forgotten)
    TOKDEF(fromto)
    TOKDEF(front)
    TOKDEF(front_only)
    TOKDEF(gap)
    TOKDEF(gates)
    TOKDEF(global)
    TOKDEF(grid)
    TOKDEF(group)
    TOKDEF(group_set)
    TOKDEF(guide)
    TOKDEF(hard)
    TOKDEF(height)
    TOKDEF(history)
    TOKDEF(horizontal)
    TOKDEF(host_cad)
    TOKDEF(host_version)
    TOKDEF(image)
    TOKDEF(image_image)
    TOKDEF(image_image_spacing)
    TOKDEF(image_outline_clearance)
    TOKDEF(image_type)
    TOKDEF(inch)
    TOKDEF(include)
    TOKDEF(include_pins_in_crosstalk)
    TOKDEF(inductance_resolution)
    TOKDEF(insert)
    TOKDEF(instcnfg)
    TOKDEF(inter_layer_clearance)
    TOKDEF(jumper)
    TOKDEF(junction_type)
    TOKDEF(keepout)
    TOKDEF(kg)
    TOKDEF(kohm)
    TOKDEF(large)
    TOKDEF(large_large)
    TOKDEF(layer)
    TOKDEF(layer_depth)
    TOKDEF(layer_noise_weight)
    TOKDEF(layer_pair)
    TOKDEF(layer_rule)
    TOKDEF(length)
    TOKDEF(length_amplitude)
    TOKDEF(length_factor)
    TOKDEF(length_gap)
    TOKDEF(library)
    TOKDEF(library_out)
    TOKDEF(limit)
    TOKDEF(limit_bends)
    TOKDEF(limit_crossing)
    TOKDEF(limit_vias)
    TOKDEF(limit_way)
    TOKDEF(linear)
    TOKDEF(linear_interpolation)
    TOKDEF(load)
    TOKDEF(lock_type)
    TOKDEF(logical_part)
    TOKDEF(logical_part_mapping)
    TOKDEF(match_fromto_delay)
    TOKDEF(match_fromto_length)
    TOKDEF(match_group_delay)
    TOKDEF(match_group_length)
    TOKDEF(match_net_delay)
    TOKDEF(match_net_length)
    TOKDEF(max_delay)
    TOKDEF(max_len)
    TOKDEF(max_length)
    TOKDEF(max_noise)
    TOKDEF(max_restricted_layer_length)
    TOKDEF(max_stagger)
    TOKDEF(max_stub)
    TOKDEF(max_total_delay)
    TOKDEF(max_total_length)
    TOKDEF(max_total_vias)
    TOKDEF(mhenry)
    TOKDEF(mho)
    TOKDEF(microvia)
    TOKDEF(mid_driven)
    TOKDEF(mil)
    TOKDEF(min_gap)
    TOKDEF(mirror)
    TOKDEF(mirror_first)
    TOKDEF(mm)
    TOKDEF(net)
    TOKDEF(net_number)
    TOKDEF(net_pin_changes)
    TOKDEF(nets)
    TOKDEF(network)
    TOKDEF(network_out)
    TOKDEF(no)
    TOKDEF(noexpose)
    TOKDEF(noise_accumulation)
    TOKDEF(noise_calculation)
    TOKDEF(object_type)
    TOKDEF(off)
    TOKDEF(off_grid)
    TOKDEF(offset)
    TOKDEF(on)
    TOKDEF(open)
    TOKDEF(opposite_side)
    TOKDEF(order)
    TOKDEF(outline)
    TOKDEF(overlap)
    TOKDEF(pad)
    TOKDEF(pad_pad)
    TOKDEF(padstack)
    TOKDEF(pair)
    TOKDEF(parallel)
    TOKDEF(parallel_noise)
    TOKDEF(parallel_segment)
    TOKDEF(parser)
    TOKDEF(part_library)
    TOKDEF(path)
    TOKDEF(pcb)
    TOKDEF(permit_orient)
    TOKDEF(permit_side)
    TOKDEF(physical)
    TOKDEF(physical_part_mapping)
    TOKDEF(piggyback)
    TOKDEF(pin)
    TOKDEF(pin_allow)
    TOKDEF(pin_cap_via)
    TOKDEF(pins)
    TOKDEF(pintype)
    TOKDEF(pin_via_cap)
    TOKDEF(pin_width_taper)
    TOKDEF(place)
    TOKDEF(place_boundary)
    TOKDEF(place_control)
    TOKDEF(placement)
    TOKDEF(place_rule)
    TOKDEF(plan)
    TOKDEF(plane)
    TOKDEF(PN)
    TOKDEF(point)
    TOKDEF(polygon)
    TOKDEF(position)
    TOKDEF(power)
    TOKDEF(power_dissipation)
    TOKDEF(power_fanout)
    TOKDEF(prefix)
    TOKDEF(primary)
    TOKDEF(priority)
    TOKDEF(property)
    TOKDEF(qarc)
    TOKDEF(quarter)
    TOKDEF(radius)
    TOKDEF(ratio)
    TOKDEF(ratio_tolerance)
    TOKDEF(rect)
    TOKDEF(reduced)
    TOKDEF(region)
    TOKDEF(region_class)
    TOKDEF(region_class_class)
    TOKDEF(region_net)
    TOKDEF(relative_delay)
    TOKDEF(relative_group_delay)
    TOKDEF(relative_group_length)
    TOKDEF(relative_length)
    TOKDEF(reorder)
    TOKDEF(reroute_order_viols)
    TOKDEF(resistance_resolution)
    TOKDEF(resolution)
    TOKDEF(restricted_layer_length_factor)
    TOKDEF(room)
    TOKDEF(rotate)
    TOKDEF(rotate_first)
    TOKDEF(round)
    TOKDEF(roundoff_rotation)
    TOKDEF(route)
    TOKDEF(routes)
    TOKDEF(routes_include)
    TOKDEF(route_to_fanout_only)
    TOKDEF(rule)
    TOKDEF(same_net_checking)
    TOKDEF(sample_window)
    TOKDEF(saturation_length)
    TOKDEF(sec)
    TOKDEF(secondary)
    TOKDEF(self)
    TOKDEF(sequence_number)
    TOKDEF(session)
    TOKDEF(set_color)
    TOKDEF(set_pattern)
    TOKDEF(shape)
    TOKDEF(shield)
    TOKDEF(shield_gap)
    TOKDEF(shield_loop)
    TOKDEF(shield_tie_down_interval)
    TOKDEF(shield_width)
    TOKDEF(side)
    TOKDEF(signal)
    TOKDEF(site)
    TOKDEF(smd)
    TOKDEF(snap_angle)
    TOKDEF(source)
    TOKDEF(space_in_quoted_tokens)
    TOKDEF(spacing)
    TOKDEF(spare)
    TOKDEF(spiral_via)
    TOKDEF(stack_via)
    TOKDEF(stack_via_depth)
    TOKDEF(standard)
    TOKDEF(starburst)
    TOKDEF(status)
    TOKDEF(string_quote)
    TOKDEF(structure)
    TOKDEF(structure_out)
    TOKDEF(subgates)
    TOKDEF(such)
    TOKDEF(suffix)
    TOKDEF(super_placement)
    TOKDEF(supply)
    TOKDEF(supply_pin)
    TOKDEF(swapping)
    TOKDEF(switch_window)
    TOKDEF(system)
    TOKDEF(tandem_noise)
    TOKDEF(tandem_segment)
    TOKDEF(tandem_shield_overhang)
    TOKDEF(terminal)
    TOKDEF(terminator)
    TOKDEF(term_only)
    TOKDEF(test)
    TOKDEF(testpoint)
    TOKDEF(test_points)
    TOKDEF(threshold)
    TOKDEF(time_length_factor)
    TOKDEF(time_resolution)
    TOKDEF(tjunction)
    TOKDEF(tolerance)
    TOKDEF(top)
    TOKDEF(topology)
    TOKDEF(total)
    TOKDEF(track_id)
    TOKDEF(turret)
    TOKDEF(type)
    TOKDEF(um)
    TOKDEF(unassigned)
    TOKDEF(unconnects)
    TOKDEF(unit)
    TOKDEF(up)
    TOKDEF(use_array)
    TOKDEF(use_layer)
    TOKDEF(use_net)
    TOKDEF(use_via)
    TOKDEF(value)
    TOKDEF(via)
    TOKDEF(via_array_template)
    TOKDEF(via_at_smd)
    TOKDEF(via_keepout)
    TOKDEF(via_number)
    TOKDEF(via_rotate_first)
    TOKDEF(via_site)
    TOKDEF(via_size)
    TOKDEF(virtual_pin)
    TOKDEF(volt)
    TOKDEF(voltage_resolution)
    TOKDEF(was_is)
    TOKDEF(way)
    TOKDEF(weight)
    TOKDEF(width)
    TOKDEF(window)
    TOKDEF(wire)
    TOKDEF(wires)
    TOKDEF(wires_include)
    TOKDEF(wiring)
    TOKDEF(write_resolution)
    TOKDEF(x)
};

static int compare( const void* a1, const void* a2 )
{
    const KEYWORD* k1 = (const KEYWORD*) a1;
    const KEYWORD* k2 = (const KEYWORD*) a2;
    
    int ret = strcmp( k1->name, k2->name );
    return ret;
}


/**
 * Function findToken
 * takes a string and looks up the string in the list of expected
 * tokens.
 * @return int - DSN_T or -1 if argument string is not a recognized token.
*/ 
static int findToken( const char* tok )
{
    char    lowercase[80];
    
    strcpy( lowercase, tok );
    
    strlower( lowercase );

    KEYWORD search;
    search.name = lowercase;
    
    const KEYWORD* findings = (const KEYWORD*) bsearch( &search, 
                                   tokens, sizeof(tokens)/sizeof(tokens[0]),
                                   sizeof(KEYWORD), compare );
    if( findings )
        return findings->token;
    else
        return -1;
}


struct IOError
{
    wxString    errorText;
    
    IOError( const wxChar* aMsg ) :
        errorText( aMsg )
    {
    }
    
    IOError( const wxString& aMsg ) :
        errorText( aMsg )
    {
    }
};


/**
 * Class LINE_READER
 * reads single lines of text into its buffer and increments a line number counter.
 * It throws an exception if a line is too long.
 */
class LINE_READER
{
protected:
    
    FILE*               fp;
    int                 lineNum;
    unsigned            maxLineLength;
    unsigned            length;
    char*               line;
    unsigned            capacity;
    
public:
    
    /**
     * Constructor LINE_READER
     * takes an open FILE and the size of the desired line buffer.
     * @param aFile An open file in "ascii" mode, not binary mode.
     * @param aMaxLineLength The number of bytes to use in the line buffer.
     */
    LINE_READER( FILE* aFile,  unsigned aMaxLineLength );
    
    ~LINE_READER()
    {
        delete[] line;
    }

    /*    
    int  CharAt( int aNdx )
    {
        if( (unsigned) aNdx < capacity )
            return (char) (unsigned char) line[aNdx];
        return -1;
    }
    */

    /**
     * Function ReadLine
     * reads a line of text into the buffer and increments the line number 
     * counter.  If the line is larger than the buffer size, then an exception
     * is thrown.
     * @return int - The number of bytes read, 0 at end of file.
     * @throw IOError only when a line is too long. 
     */
    int ReadLine() throw (IOError);

    operator char* ()
    {
        return line;
    }
    
    int LineNumber()
    {
        return lineNum;
    }
    
    unsigned Length()
    {
        return length; 
    }
};


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
    const char* p = fgets( &line[0], capacity, fp );
    
    if( !p )
    {
        length = 0;
    }
    else
    {
        length = strlen( &line[0] );

        if( length > maxLineLength )        
            throw IOError( _("Line length exceeded") );
        
        ++lineNum;
    }
    
    return length;
}


/**
 * Class LEXER
 * reads lexical tokens from the current LINE_READER through the NextTok()
 * function.  The NextTok() function returns one of the DSN_T values.
 */
class LEXER
{
    char*               head;

    char*               cur;    
    char*               start;
    char*               limit;
    
    LINE_READER         reader;
    int                 stringDelimiter;
    wxString            filename;
    int                 lastTok;        ///< curTok from las NextTok() call.
    
    int                 curTok;         ///< the DSN_T value of current token
    std::string         curText;        ///< the text of the current token

    
    int readLine() throw (IOError)
    {
        int len = reader.ReadLine();

        cur   = start;
        limit = start + len;
        
        return len;
    }
    

public:
    LEXER( FILE* aFile, const wxString& aFilename ) :
        reader( aFile, 4096 )
    {
        curTok = T_NONE;
        stringDelimiter = '"';
        filename = aFilename;
        
        // start should never change until we change the reader, and the DSN
        // format supports an include mechanism but we'll add that later.
        start = (char*) reader;     
        
        limit = start;
        cur   = start;
        head  = start;
    }

    
    /**
     * Function NextTok
     * returns the next token found in the input file or T_EOF when reaching
     * the end of file.
     * @return int - one of the DSN_T values.
     * @throw IOError - only if the LINE_READER throws it.
     */
    int NextTok() throw (IOError);
    
    
    /**
     * Function ThrowIOError
     * encapsulates the formatting of an error message which contains the exact
     * location within the input file of a lexical error.
     */
    void ThrowIOError( wxString aText, int charOffset ) throw (IOError)
    {
        aText << wxT(" ") << _("in file") << wxT(" \"") << filename 
              << wxT("\" ") << _("on line") << wxT(" ") << reader.LineNumber()
              << wxT(" ") << _("at offset") << wxT(" ") << charOffset;
              
        throw IOError( aText );
    }
    
    
    /**
     * Function CurText
     * returns a pointer to the current token's text.
     */
    const char* CurText()
    {
        return curText.c_str();
    }

    
    /**
     * Function CurTok
     * returns whatever NextTok() returned the last time it was called.
     */
    int CurTok()
    {
        return curTok;
    }
    
};


int LEXER::NextTok() throw (IOError)
{
L_next:        
    lastTok = curTok;
    
    cur = head;
    
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
        }
        
        // skip leading whitespace
        while( cur<limit && isspace(*cur) )
            ++cur;

        if( cur >= limit )
            goto L_read;
        
        else
        {
            // switching the string_quote character
            if( lastTok == T_string_quote )
            {
                stringDelimiter = *cur;
                
                curText.clear();
                curText += *cur;
                
                head = cur+1;
                curTok = T_NONE;
                
                // Do not return this to the caller, consume it internally
                // and go get another token for the caller.
                goto L_next;
            }
            
            // a quoted string
            else if( *cur == stringDelimiter )
            {
                ++cur;  // skip over the leading "

                head = cur;
            
                while( head<limit && *head!=stringDelimiter )
                    ++head;
                
                if( head >= limit )
                {
                    wxString errtxt(_("Un-terminated delimited string") );
                    ThrowIOError( errtxt, limit-start+1 );
                }
                
                curText.clear();
                curText.append( cur, head );
                
                ++head;     // skip over the trailing delimiter
                
                curTok  = T_STRING;                    
            }
        
            else if( *cur == '(' )
            {
                curText.clear();
                curText += *cur;
                
                curTok = T_LEFT;
                head = cur+1;
            }
            
            else if( *cur == ')' )
            {
                curText.clear();
                curText += *cur;
                
                curTok = T_RIGHT;
                head = cur+1;
            }

            // handle T_DASH or T_NUMBER                
            else if( strchr( "+-.0123456789", *cur ) )
            {
                if( *cur=='-' && !strchr( ".0123456789", *(cur+1) ) )
                {
                    head = cur+1;
                    
                    curText.clear();
                    curText += *cur;
                    curTok = T_DASH;
                }
                else
                {
                    head = cur+1;
                    while( head<limit && strchr( ".0123456789", *head )  )
                        ++head;
                    
                    curText.clear();
                    curText.append( cur, head );
                    curTok = T_NUMBER;
                }
            }
            
            // a token we hope to find in the tokens[] array.  If not, then
            // call it a T_SYMBOL.
            else
            {
                head = cur+1;
                while( head<limit && !isspace( *head ) && *head!=')' )
                    ++head;
                
                curText.clear();
                curText.append( cur, head );
                
                curTok  = findToken( curText.c_str() );
                
                if( curTok == -1 )  // unrecogized token
                {
                    curTok = T_SYMBOL;
                    
                    /*
                    wxString    errTxt( CONV_FROM_UTF8( curText.c_str() ) );
                    errTxt << wxT(" ") << _("is an unrecognized token");
                    ThrowIOError( errTxt, cur-start+1 );
                    */
                }
            }
        }
    }

exit:        
    return curTok;
}



}   // namespace DSN



#if defined(STANDALONE)

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
