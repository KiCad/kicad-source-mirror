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

#ifndef _DSN_H
#define _DSN_H
 
#include <cstdio>

#include "fctsys.h"
#include "pcbnew.h"


namespace DSN {

    
/**
 * Enum DSN_T
 * lists all the DSN lexer's tokens that are supported in lexing.  It is up
 * to the parser if it wants also to support them.
 */ 
enum DSN_T {
    
    // the first few are special (the uppercase ones)
    T_QUOTE_DEF = -8,
    T_DASH = -7,
    T_SYMBOL = -6,
    T_NUMBER = -5,
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
    T_image_conductor,
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
    T_square,
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
 * Struct IOError
 * is a class used to hold an error message and may be thrown from the LEXER.
 */
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



/**
 * Class LEXER
 * reads lexical tokens from the current LINE_READER through the NextTok()
 * function.  The NextTok() function returns one of the DSN_T values.
 */
class LEXER
{
    char*               next;    
    char*               start;
    char*               limit;
    
    LINE_READER         reader;
    int                 stringDelimiter;
    bool                space_in_quoted_tokens; ///< blank spaces within quoted strings
    
    wxString            filename;
    int                 lastTok;        ///< curTok from previous NextTok() call.
    int                 curOffset;      ///< offset within current line of the current token 
    
    DSN_T               curTok;         ///< the current token obtained on last NextTok()
    std::string         curText;        ///< the text of the current token
    std::string         lowercase;      ///< a scratch buf holding token in lowercase

    
    int readLine() throw (IOError)
    {
        int len = reader.ReadLine();

        next  = start;
        limit = start + len;
        
        return len;
    }


    /**
     * Function findToken
     * takes a string and looks up the string in the list of expected
     * tokens.
     *
     * @param tok A string holding the token text to lookup, in an 
     *   unpredictable case: uppercase or lowercase
     * @return int - DSN_T or -1 if argument string is not a recognized token.
     */ 
    int findToken( const std::string& tok );

    bool isStringTerminator( char cc )
    {
        if( !space_in_quoted_tokens && cc==' ' )
            return true;
        
        if( cc == stringDelimiter )
            return true;
        
        return false;
    }
    
    
public:
    LEXER( FILE* aFile, const wxString& aFilename );

    
    /**
     * Function SetStringDelimiter
     * changes the string delimiter from the default " to some other character
     * and returns the old value.
     * @param aStringDelimiter The character in lowest 8 bits.
     * @return int - The old delimiter in the lowest 8 bits.
     */
    int SetStringDelimiter( int aStringDelimiter )
    {
        int old = stringDelimiter;
        stringDelimiter = aStringDelimiter;
        return old;
    }

    /**
     * Function SetSpaceInQuotedTokens
     * changes the setting controlling whether a space in a quoted string is
     * a terminator
     */
    bool SetSpaceInQuotedTokens( bool val )
    {
        bool old = space_in_quoted_tokens;
        space_in_quoted_tokens = val;
        return old;
    }

    
    /**
     * Function NextTok
     * returns the next token found in the input file or T_EOF when reaching
     * the end of file.
     * @return DSN_T - the type of token found next.
     * @throw IOError - only if the LINE_READER throws it.
     */
    DSN_T NextTok() throw (IOError);
    
    
    /**
     * Function ThrowIOError
     * encapsulates the formatting of an error message which contains the exact
     * location within the input file of a lexical error.
     */
    void ThrowIOError( wxString aText, int charOffset ) throw (IOError);


    /**
     * Function GetTokenString
     * returns the wxString representation of a DSN_T value.
     */
    static wxString GetTokenString( DSN_T aTok );
    
    /**
     * Function GetTokenString
     * returns the C string representation of a DSN_T value.
     */
    static const char* GetTokenText( DSN_T aTok );

    
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
    DSN_T CurTok()
    {
        return curTok;
    }

    /**
     * Function CurOffset
     * returns the char offset within the current line, using a 1 based index.
     * @return int - a one based index into the current line.
     */
    int CurOffset()
    {
        return curOffset + 1;
    }
    
};


}   // namespace DSN

#endif  // _DSN_H
