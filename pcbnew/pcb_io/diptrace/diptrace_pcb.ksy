# Can be viewed in: https://ide.kaitai.io/
#
# Reverse-engineered DipTrace PCB binary format (.dip).
#
# This spec follows the deterministic section order used by
# diptrace_pcb_parser.cpp.  The board header through the design-rules block is a
# strictly sequential stream (ParseMagic ... ParseDesignRules) and is modeled in
# the top-level `seq`.
#
# Everything after the design-rules block (components, pads, footprint shapes,
# mount holes, the component tail, text objects, net records + routing, track
# chains, and copper-pour zones) is now decoded by a DETERMINISTIC FIELD-WALK:
# each record's position is derived from a fixed offset or the preceding record's
# length, anchored on a per-record structural signature, rather than scanned for.
# The qa test ObjectsAreFieldLocatedNotScanned asserts PCB_PARSER::
# ScanLocatorUseCount() == 0, i.e. no object or section is located by the legacy
# byte-pattern scan (which is retained only as a recovery fallback).  The walk:
#   * Components: from the post-design-rules offset, FieldWalkComponentBoundaries()
#     advances to the nearest following boundary core (int3(0) int3(-1) int3(-1)
#     int4(0), or the all-zero alt variant) that carries a header string at the
#     file's global string delta (any length, incl. the empty header of
#     standalone-via records).  No global boundary scan; the result is
#     cross-validated against the legacy scan.  High-bit invalid header flag bytes
#     are fatal.
#   * Pads: the first pad record begins a fixed preamble after the component
#     header strings -- headerEndOffset + 74 (v39+ uint16 strings) or + 76 (v37
#     ASCII) -- then a chain-walk of `pad` records to padRegionEnd.
#   * Footprint shapes: v46+ `fp_font_block` records are field-walked from
#     padRegionEnd + 165 + 2*u16(padRegionEnd+163); each block is self-sized as
#     72 + 8*point_count + 2*label_chars and the run ends at the value/framing
#     block.  v<46 `fp_shape`/`fp_chain_shape` records are count-guided from
#     padRegionEnd + fixed offsets.
#   * ParseComponentTail() consumes the fixed 37-byte `component_tail`.
#   * Sections: each is located by its own structural prefix -- the board TEXT
#     section by a nine-byte zero separator + int3 count + 01 00 flags; the NET
#     section by the int3 record count five bytes ahead of the index-0 sentinel
#     (`net_record` walked by sequential index); the ZONE section by its font
#     preamble ending int4(-20000) ahead of a 30-byte zone header.
# The post-design-rules tail is still exposed as `post_via_styles_tail` (a raw
# substream) with a named type for every decoded structure, because Kaitai cannot
# express the content-anchored offsets the field-walk computes at runtime; the
# named types below document the true sequential record layouts.

meta:
  id: diptrace_pcb
  endian: be
  encoding: UTF-8

seq:
  - id: magic_len
    type: u1
    valid:
      any-of: [7, 11]
  - id: magic
    contents: [0x44, 0x54, 0x42, 0x4f, 0x41, 0x52, 0x44] # "DTBOARD"
  - id: legacy_major_digit
    type: u1
    doc: Legacy "x.yy" major-version digit.
    if: magic_len == 11
  - id: legacy_dot
    contents: [0x2e] # "."
    if: magic_len == 11
  - id: legacy_minor_tens
    type: u1
    doc: Legacy "x.yy" minor-version tens digit.
    if: magic_len == 11
  - id: legacy_minor_ones
    type: u1
    doc: Legacy "x.yy" minor-version ones digit.
    if: magic_len == 11
  - id: version
    type: int3
    if: magic_len == 7
  - id: field_0b
    type: int4
  - id: field_0f
    type: int3
  - id: field_12
    type: int3
  - id: legacy_schematic_placeholder
    type: int3
    if: ver <= 37
  - id: schematic_path
    type: dt_string
    if: ver > 37
  - id: flag_byte
    type: u1
  - id: bbox_x_min
    type: int4
  - id: bbox_y_min
    type: int4
  - id: bbox_x_max
    type: int4
  - id: bbox_y_max
    type: int4
  - id: outline_vertex_count
    type: int3
  - id: outline_vertices
    type: outline_vertex
    repeat: expr
    repeat-expr: outline_vertex_count.value
  - id: post_outline
    type: post_outline
  - id: layer_count
    type: int3
  - id: layers
    type: layer
    repeat: expr
    repeat-expr: layer_count.value
  - id: font_style
    type: font_style
  - id: via_styles
    type: via_styles
  - id: netclasses
    type: netclasses

  - id: post_via_styles_tail
    size-eos: true
    doc: |
      Raw substream holding all anchored records after the design-rule section.
      The importer does not parse this region as one repeat; it searches bounded
      regions for the record anchors described in the file-level documentation.
      The deterministic byte layout of each accepted record is fully modeled by the types
      `component`, `component_tail`, `pad`, `fp_shape`, `fp_shape_v37`,
      `fp_font_block`, `fp_chain_shape`, `mount_hole_block`, `text_section`,
      `net_record`, `net_routing_preamble`, `track_chain`, `track_node`,
      `copper_pour_font_preamble`, and `copper_pour` below.
      
instances:
  ver:
    value: 'magic_len == 7 ? version.value : (legacy_minor_tens - 48) * 10 + (legacy_minor_ones - 48)'

types:
  int3:
    doc: 3-byte unsigned big-endian integer biased by 1,000,000.
    seq:
      - id: byte0
        type: u1
      - id: byte1
        type: u1
      - id: byte2
        type: u1
    instances:
      raw:
        value: byte0 * 65536 + byte1 * 256 + byte2
      value:
        value: raw - 1000000

  int4:
    doc: 4-byte unsigned big-endian integer biased by 1,000,000,000.
    seq:
      - id: raw
        type: u4
    instances:
      value:
        value: raw - 1000000000

  dt_string:
    doc: Version-dependent DipTrace string.
    seq:
      - id: len_ascii
        type: int3
        if: _root.ver <= 37
      - id: text_ascii
        type: str
        size: len_ascii.value
        encoding: ASCII
        if: _root.ver <= 37
      - id: len_utf16
        type: u2
        if: _root.ver > 37
      - id: text_utf16
        type: str
        size: len_utf16 * 2
        encoding: UTF-16BE
        if: _root.ver > 37

  rgb_color:
    seq:
      - id: red
        type: u1
      - id: green
        type: u1
      - id: blue
        type: u1

  outline_vertex:
    seq:
      - id: x
        type: int4
      - id: y
        type: int4
      - id: arc
        type: u1

  post_outline:
    seq:
      - id: end_marker
        type: u1
      - id: field_a
        type: int3
      - id: field_b
        type: int3
      - id: grid_x
        type: int4
      - id: grid_y
        type: int4
      - id: pad_byte
        type: u1
      - id: fields_c
        type: int4
        repeat: expr
        repeat-expr: 4
      - id: trail_bytes
        type: u1
        repeat: expr
        repeat-expr: 3

  layer:
    doc: |
      One copper-layer record (ParseLayers, parser ~1137-1165).  Each record is
      exactly: flag u1, index int3, color rgb(3), name dt_string, layer_type
      int3, field_b int3, plane_net_index int3, field_d int4, separator u1.
    seq:
      - id: flag
        type: u1
      - id: index
        type: int3
      - id: color
        type: rgb_color
      - id: name
        type: dt_string
      - id: layer_type
        type: int3
        doc: Layer Type from record field_a (0 = Signal, 1 = Plane).
      - id: field_b
        type: int3
        doc: Unused trailing field between the layer type and the plane net index.
      - id: plane_net_index
        type: int3
        doc: |
          DipTrace net index for a Plane layer (record field_c); -1 for Signal
          layers or planes with no assigned net.  Matches the CopperLayers
          <Lay Type=.. NetId=..> oracle.
      - id: field_d
        type: int4
        doc: Possibly the default trace width for the layer.
      - id: separator
        type: u1

  font_style:
    seq:
      - id: prefix_fields
        type: int3
        repeat: expr
        repeat-expr: 6
      - id: font_name
        type: dt_string
      - id: field_a
        type: int4
      - id: field_b
        type: int4
      - id: flag_a
        type: u1
      - id: font_height
        type: int4
      - id: font_width
        type: int4
      - id: legacy_magic_padding
        size: 12
        doc: DTBOARDx.yy legacy files store twelve bytes here, followed directly by the rule-name count.
        if: _root.magic_len == 11
      - id: v37_padding
        size: 5
        if: _root.magic_len == 7 and _root.ver <= 37
      - id: v37_extra_a
        type: int3
        if: _root.magic_len == 7 and _root.ver <= 37
      - id: v37_extra_b
        type: int3
        if: _root.magic_len == 7 and _root.ver <= 37
      - id: v37_extra_flag
        type: u1
        if: _root.magic_len == 7 and _root.ver <= 37
      - id: modern_padding
        size: 10
        doc: |
          Standard v38+ post-font padding.  Most modern files then store a zero
          field_c followed by a pattern-name group count.  Some v49+ files store
          the pattern-style group count directly in field_c.  Files with the
          compact implicit-style variant use only the first seven bytes of this
          padding and then `implicit_pattern_style_group`.
        if: _root.ver > 37
      - id: field_c
        type: int3
        doc: Zero for pattern-name groups; nonzero v49+ value is a pattern-style group count.
        if: _root.magic_len == 7
      - id: field_d
        type: int3
        doc: Pattern-name group count when field_c is zero.
        if: _root.magic_len == 7 and (_root.ver <= 37 or field_c.value == 0)
      - id: pattern_groups
        type: pattern_name_groups
        if: _root.magic_len == 7 and (_root.ver <= 37 or field_c.value == 0)
      - id: style_groups
        type: pattern_style_groups
        if: _root.magic_len == 7 and _root.ver > 37 and field_c.value > 0
      - id: rule_name_count
        type: int3
        doc: Number of following design-rule/via-style entries for the pattern-name group layout.
        if: _root.magic_len == 7 and (_root.ver <= 37 or field_c.value == 0)

  pattern_name_groups:
    seq:
      - id: groups
        type: pattern_name_group
        repeat: expr
        repeat-expr: _parent.field_d.value

  pattern_name_group:
    seq:
      - id: name
        type: dt_string
      - id: field_a
        type: int3
      - id: block_count
        type: int3
      - id: blocks
        type: pattern_name_block
        repeat: expr
        repeat-expr: block_count.value

  pattern_name_block:
    seq:
      - id: block_id
        type: int3
      - id: name
        type: dt_string

  pattern_style_groups:
    doc: |
      v49+ style/category groups.  The group count is stored in font_style.field_c.
      Each group contributes entry_count following design-rule/via-style entries.
    seq:
      - id: groups
        type: pattern_style_group
        repeat: expr
        repeat-expr: _parent.field_c.value

  pattern_style_group:
    seq:
      - id: name
        type: dt_string
      - id: color
        type: rgb_color
      - id: field_a
        type: int3
      - id: field_b
        type: int3
      - id: field_c
        type: int3
      - id: entry_count
        type: int3

  implicit_pattern_style_group:
    doc: |
      Compact v49+ style group observed with @MingLiU-ExtB files.  It follows a
      seven-byte post-font padding prefix instead of the standard ten-byte
      padding and has no explicit group-count int3.
    seq:
      - id: name
        type: dt_string
      - id: flag
        type: u1
      - id: field_a
        type: int3
      - id: field_b
        type: int3
      - id: entry_count
        type: int3

  via_styles:
    seq:
      - id: num_entries
        type: int3
      - id: entries
        type: via_style_entry
        repeat: expr
        repeat-expr: num_entries.value
    types:
      via_style_entry:
        seq:
          - id: name
            type: dt_string
          - id: flag
            type: u1
          - id: value_1
            type: int4
          - id: value_2
            type: int4
          - id: field_a
            type: int3
          - id: field_b
            type: int3
            
  netclasses:
    seq:
      - id: num_entries
        type: int3
      - id: entries
        type: netclass_entry
        repeat: expr
        repeat-expr: num_entries.value
    types:
      netclass_entry:
        seq:
          - id: name
            type: dt_string
          - id: field_a
            type: int3
          - id: flags
            type: u1
            repeat: expr
            repeat-expr: 4
          - id: block_count
            type: int3
            valid:
              expr: 
                _.value < 10000
          - id: blocks
            type: netclass_block
            repeat: expr
            repeat-expr: block_count.value
          - id: trailer_a
            type: int4
          - id: trailer_b
            type: int4
          - id: unk1_count
            type: int3
          - id: unk1_data
            type: int3
            repeat: expr
            repeat-expr: unk1_count.value
          - id: raw_pad_1
            type: s4
          - id: unk2_count
            type: int3
          - id: unk3_sth
            type: int3
            valid: 
              expr:
                _.value == 10000
          - id: transition
            type: ruleset_transition

      netclass_block:
        seq:
          - id: values
            type: int4
            repeat: expr
            repeat-expr: 25

      ruleset_transition:
        doc: |
          Inter-ruleset separator.  Most files end with field_d as int3.  Some
          v54+ DT_PAD files instead store the five-byte suffix
          01 00 14 89 03 before the next UTF-16 ruleset name.
        seq:
          - id: marker
            contents: [0x4d, 0x7c, 0x6d, 0x00]
          - id: field_a
            type: int4
          - id: field_b
            type: int4
          - id: field_c
            type: int3
          - id: field_d
            type: int3
          - id: dt_pad_suffix
            contents: [0x01, 0x00, 0x14, 0x89, 0x03]
            doc: Alternative to field_d in DT_PAD-style v54+ files; documented here as the named fixed suffix consumed by the importer.
            if: false

  # ----------------------------------------------------------------------------
  # Field-walked tail records (post_via_styles_tail).
  # ----------------------------------------------------------------------------

  component_boundary:
    doc: |
      Standard component boundary marker: int3(0) int3(-1) int3(-1) int4(0)
      (BOUNDARY_STD).  FieldWalkComponentBoundaries() advances to each next
      component as the nearest following 13-byte core carrying a header string at
      the file's global boundary delta, so the markers form a deterministic
      sequential chain (no global pattern scan; cross-validated against the legacy
      scan).  For a component with a plausible library path, high-bit invalid
      header flag bytes are fatal.  The alternate v41 nameplate marker swaps the
      two int3(-1) fields for int3(0) (see component_boundary_alt).
    seq:
      - id: field_a
        contents: [0x0f, 0x42, 0x40]
      - id: field_b
        contents: [0x0f, 0x42, 0x3f]
      - id: field_c
        contents: [0x0f, 0x42, 0x3f]
      - id: field_d
        contents: [0x3b, 0x9a, 0xca, 0x00]

  component_boundary_alt:
    doc: |
      Alternate component boundary marker int3(0) int3(0) int3(0) int4(0)
      (BOUNDARY_ALT), used by v41 nameplate files.
    seq:
      - id: field_a
        contents: [0x0f, 0x42, 0x40]
      - id: field_b
        contents: [0x0f, 0x42, 0x40]
      - id: field_c
        contents: [0x0f, 0x42, 0x40]
      - id: field_d
        contents: [0x3b, 0x9a, 0xca, 0x00]

  component:
    doc: |
      One placed component / footprint record (ParseSingleComponent, parser
      ~1595-1702).  The reader seeks to boundaryOffset+14 (past the boundary
      marker) and reads this structure sequentially.  Field order and sizes are
      mirrored exactly; `placement_quarter_turns` lives 59 bytes BEFORE the
      boundary marker (int3 read at boundaryOffset-59) and so is not part of this
      forward-read structure.  Standalone-via component records can truncate
      after `pattern_name`; the importer keeps those partial records so the pad
      data can classify them as standalone vias.
    seq:
      - id: library_path
        type: dt_string
      - id: field_a
        type: int3
        doc: Raw header int3 field A.
      - id: field_b
        type: int3
        doc: Raw header int3 field B (unused by importer).
      - id: flags
        type: u1
        repeat: expr
        repeat-expr: 4
        doc: |
          4 flag bytes; each is validated to be <= 10.  flags[1] is the component
          side (0 = top, 1 = bottom) before any tail override.
      - id: position_x
        type: int4
      - id: position_y
        type: int4
      - id: rotation
        type: int4
        doc: Raw header int4; matches Pattern.Float1 in DipXML (not the placement angle).
      - id: field_c
        type: int4
        doc: Raw header int4; matches Pattern.Float2 in DipXML.
      - id: sep_1
        type: u1
        doc: Separator, must be 0 or the record is rejected.
      - id: field_d
        type: int4
        doc: Raw header int4; matches Pattern.Float3 in DipXML.
      - id: sep_2
        type: u1
        doc: Separator, must be <= 1.
      - id: sep_3
        type: u1
        doc: Separator, must be 0.
      - id: field_e
        type: int3
      - id: field_f
        type: int3
        doc: Component kind discriminator (used in standalone-via classification).
      - id: pattern_name
        type: dt_string
      - id: bbox_width
        type: int4
        doc: Footprint X extent in DipTrace units (for shape scaling).
      - id: bbox_height
        type: int4
        doc: Footprint Y extent in DipTrace units.
      - id: pad_width_hint
        type: int4
      - id: pad_height_hint
        type: int4
      - id: drill_width_hint
        type: int4
      - id: drill_height_hint
        type: int4
      - id: field_g
        type: int3
      - id: field_h
        type: int3
      - id: display_name
        type: dt_string
      - id: refdes
        type: dt_string
      - id: value
        type: dt_string
      - id: extra_string
        type: dt_string
        doc: Trailing string after value; always empty in observed files.

  component_tail:
    doc: |
      Fixed 37-byte component tail carrying refdes/value text positioning
      (ParseComponentTail / tryParseTailAt, parser ~2405-2489).  Located at the
      end of each component region.  The first 11 bytes are a constant pattern
      (COMPONENT_TAIL_PATTERN).
    seq:
      - id: lead
        contents: [0x0f, 0x42, 0x40]      # int3(0)
        doc: Constant int3(0).
      - id: const_int4_a
        contents: [0x3b, 0x9a, 0xca, 0x00] # int4(0)
        doc: Constant int4(0).
      - id: const_int4_b
        contents: [0x3b, 0x9a, 0xca, 0x00] # int4(0)
        doc: Constant int4(0).
      - id: check_int4_a
        type: int4
        doc: Validated to be 0 (read at tail+11).
      - id: check_int4_b
        type: int4
        doc: Validated to be 0 (read at tail+15).
      - id: side_flag_1
        type: u1
        doc: |
          Text side flag.  Together with side_flag_2 carries the component side;
          both = 1 for bottom-side parts.
      - id: visibility
        type: int3
        doc: Text visibility (0 = visible, -1 = hidden); any other value rejects the tail.
      - id: side_flag_2
        type: u1
        doc: Text side flag 2 (mirrors side_flag_1).
      - id: order_index
        type: int3
        doc: Ordering index.
      - id: refdes_y_offset
        type: int4
        doc: Refdes text Y offset from component origin (DipTrace units).
      - id: value_y_offset
        type: int4
        doc: Value text Y offset from component origin (DipTrace units).
      - id: has_offset
        type: u1
        doc: Has-offset flag; validated to be <= 1.
      - id: terminator
        type: u1
        doc: Constant 0 terminator.

  pad:
    doc: |
      One pad record (FindPadsInRegion).  The first pad is field-located at
      component header_end + 74 (v39+ uint16 strings) or + 76 (v37 ASCII), then
      pads are walked sequentially; each record begins with int3(index).  Layout
      is pre-header(14) + number string + label string + dimensions(16) +
      post-dimension block (fixed 36 bytes, or 11-byte header + N polygon
      vertices + 25-byte tail when style_code == 3).  The int3(1) index scan is a
      recovery fallback only.
    seq:
      - id: index
        type: int3
        doc: Sequential pad index within the component (1-based).
      - id: net_index
        type: int3
        doc: Net index from the DipTrace file (-1 = unconnected); validated to [-1, 500].
      - id: x
        type: int4
        doc: X offset from component origin in DipTrace units.
      - id: y
        type: int4
        doc: Y offset from component origin in DipTrace units.
      - id: number
        type: dt_string
        doc: Pad number/name (e.g. "1", "2").
      - id: label
        type: dt_string
        doc: Functional label (e.g. "POS", "GND").
      - id: width
        type: int4
        doc: Copper pad width in DipTrace units (validated > 0).
      - id: height
        type: int4
        doc: Copper pad height in DipTrace units (validated > 0).
      - id: drill_width
        type: int4
        doc: Drill width in DipTrace units (0 for SMD).
      - id: drill_height
        type: int4
        doc: Drill height in DipTrace units (0 for SMD).
      - id: post_dim
        type: pad_post_dim
        doc: Post-dimension block; see pad_post_dim.

  pad_post_dim:
    doc: |
      Pad post-dimension block (FindPadsInRegion, parser ~1826-1888).  Only a few
      fields are assigned by the importer; the rest are part of the fixed 36-byte
      block (PAD_POST_DIM_FIXED_SIZE) or, for polygon pads, the 11-byte header +
      vertex run + 25-byte tail.  Field_a is read by the optional pad-post dump
      but otherwise unused.
    seq:
      - id: field_a
        type: int3
        doc: Read at post-dim +0 by the debug dump; otherwise unused.
      - id: mount_type
        type: u1
        doc: Explicit mount class at post-dim +3 (0 = through, 1 = SMD).
      - id: shape_code
        type: int3
        doc: |
          Pad style at post-dim +4 (0 = Ellipse, 1 = Obround, 2 = Rectangle,
          3 = Polygon).
      - id: unknown_7
        type: u1
        doc: Filler byte at post-dim +7 (not assigned a meaning).
      - id: polygon_vertex_count
        type: int3
        doc: |
          Polygon vertex count at post-dim +8.  Only meaningful when
          shape_code == 3; for the fixed block it is part of the unassigned
          remainder.
      - id: polygon_vertices
        type: pad_polygon_vertex
        repeat: expr
        repeat-expr: 'shape_code.value == 3 ? polygon_vertex_count.value : 0'
        doc: |
          Inline polygon vertices relative to pad center, present only when
          shape_code == 3 (PAD_POST_DIM_HEADER = 11 bytes precede this run).
      - id: fixed_tail
        size: 25
        if: shape_code.value == 3
        doc: |
          25-byte tail (PAD_POST_DIM_TAIL) after the polygon vertices.  The last
          byte is the pad orientation class (orientClass).
      - id: fixed_block_tail
        size: 25
        if: shape_code.value != 3
        doc: |
          Remainder of the 36-byte fixed block (11-byte header already consumed
          above).  The last byte is the pad orientation class (orientClass).

  pad_polygon_vertex:
    doc: One polygon vertex relative to pad center (PAD_POLYGON_VERTEX_SIZE = 8).
    seq:
      - id: x
        type: int4
      - id: y
        type: int4

  mount_hole_block:
    doc: |
      Footprint mount-hole (NPTH) block (FindMountHolesInRegion, parser
      ~1916-2074).  The importer checks explicit modeled positions only:
      immediately after the pad chain, after a fixed-size footprint-shape block,
      and at byte +44 of the final v46+ chained-shape framing record
      (`fp_chain_shape_mount_hole_overlay`).  The block is a 20-byte header
      (int3(hole_count + 2) + flag u1 + 4 zero int4) followed by hole_count
      18-byte records, a 2-byte 0x00 0x00 terminator, and a 16-byte zero trailer.
    seq:
      - id: count_field
        type: int3
        doc: hole_count + 2 (validated so 0 < hole_count <= 64).
      - id: header_flag
        type: u1
        doc: Validated to be <= 1.
      - id: header_zeros
        type: int4
        repeat: expr
        repeat-expr: 4
        doc: 4 int4 fields, all validated to be 0.
      - id: holes
        type: mount_hole
        repeat: expr
        repeat-expr: count_field.value - 2
      - id: terminator
        size: 2
        doc: Two 0x00 bytes (MOUNT_HOLE_TERM_SIZE).
      - id: trailer_zeros
        type: int4
        repeat: expr
        repeat-expr: 4
        doc: 16-byte zero trailer (MOUNT_HOLE_TRAILER_SIZE), all int4 = 0.

  mount_hole:
    doc: Per-hole record (MOUNT_HOLE_RECORD_SIZE = 18).
    seq:
      - id: flag_a
        type: u1
        doc: Validated to be 0.
      - id: flag_b
        type: u1
        doc: Validated to be <= 1.
      - id: x
        type: int4
        doc: X offset from component origin in DipTrace units.
      - id: y
        type: int4
        doc: Y offset from component origin in DipTrace units.
      - id: outer_diameter
        type: int4
        doc: Non-copper/clearance diameter in DipTrace units.
      - id: drill_diameter
        type: int4
        doc: Drill diameter in DipTrace units (validated <= outer_diameter).

  fp_shape:
    doc: |
      Footprint outline shape record for v45+ contiguous shape blocks
      (FindShapesInRegion, parser ~2116-2204; FP_SHAPE_RECORD_SIZE_V45 = 60).
      The block starts at padRegionEnd + 74 (FP_SHAPE_DATA_OFFSET) with the count
      at padRegionEnd + 71.  Only the fields the importer assigns are named; the
      record is padded to its fixed size.  See fp_shape_v37 for the legacy
      62-byte variant.
    seq:
      - id: shape_type
        type: int3
        doc: |
          Shape type (0 = empty, 1 = line, 2 = rect, 3 = circle, 6 = arc,
          -1 = end marker).  Only line/circle/arc are imported.
      - id: x1
        type: int4
        doc: Start X in normalized units (range +/-5000), read at record +3.
      - id: y1
        type: int4
        doc: Start Y at record +7.
      - id: x2
        type: int4
        doc: End X at record +11.
      - id: y2
        type: int4
        doc: End Y at record +15.
      - id: mid_x
        type: int4
        doc: Arc midpoint X at record +19.
      - id: mid_y
        type: int4
        doc: Arc midpoint Y at record +23.
      - id: gap_27
        size: 26
        doc: Bytes +27..+52 are not assigned by the importer.
      - id: width
        type: int4
        doc: Line width at record +53 (-10000 = use default).
      - id: layer
        type: int3
        doc: DipTrace layer index at record +57.
      # Total fixed size is 60 bytes; the 3 trailing bytes (+57..+59 hold layer)
      # complete the record.

  fp_shape_v37:
    doc: |
      Legacy v37 footprint shape record (FindShapesInRegion, parser ~2191-2194;
      FP_SHAPE_RECORD_SIZE_V37 = 62).  Identical to fp_shape except width is read
      at +55 and layer at +59.
    seq:
      - id: shape_type
        type: int3
      - id: x1
        type: int4
      - id: y1
        type: int4
      - id: x2
        type: int4
      - id: y2
        type: int4
      - id: mid_x
        type: int4
      - id: mid_y
        type: int4
      - id: gap_27
        size: 28
        doc: Bytes +27..+54 are not assigned by the importer.
      - id: width
        type: int4
        doc: Line width at record +55.
      - id: layer
        type: int3
        doc: DipTrace layer index at record +59.

  fp_font_block:
    doc: |
      v46+ per-layer font-block shape (FindShapesInFontBlocks).  The first block
      is field-located at padRegionEnd + 165 + 2*u16(padRegionEnd + 163); each
      block is self-sized as 72 + 8*point_count + 2*label_chars (point_count =
      smallest leading slot in 0..3 whose int3 holds a valid shape-type code, one
      of 0, 1, 2, 3, 6, 7, 700), so the blocks form a deterministic chain.  The run
      ends at the trailing value/framing text label -- the first block whose
      leading slot carries shape-type 0 (point_count == 0) or no shape-type code at
      all; a block that instead runs past the region edge is a truncation that
      reverts to the Tahoma scan.  Each block begins with the UTF-16BE "Tahoma"
      pattern (14 bytes) + a 25-byte metadata header (FONT_BLOCK_HEADER_SIZE) + the
      geometry body; line_width and layer are read from inside the metadata header.
      The Tahoma scan is a fallback.
    seq:
      - id: font_pattern
        contents: [0x00, 0x06, 0x00, 0x54, 0x00, 0x61, 0x00, 0x68, 0x00, 0x6f, 0x00, 0x6d, 0x00, 0x61]
        doc: UTF-16BE "Tahoma" (TAHOMA_FONT_PATTERN).
      - id: meta_head
        size: 18
        doc: |
          First 18 bytes of the 25-byte metadata header
          (flag u1 + fontSize int3 + fontH int4 + fontW int4 + field_b int3).
      - id: line_width
        type: int4
        doc: Line width at meta +18.
      - id: layer
        type: int3
        doc: DipTrace layer index at meta +22 (end of the 25-byte header).
      - id: x1
        type: int4
        doc: Start X (normalized, +/-10000) at body +0.
      - id: y1
        type: int4
        doc: Start Y at body +4.
      - id: x2
        type: int4
        doc: End X at body +8.
      - id: y2
        type: int4
        doc: End Y at body +12.
      - id: shape_type
        type: int3
        doc: |
          Shape-type discriminator at body +16.  In the font-block decode 0 = rect,
          1 = line, 3 = circle, and both 2 and 6 (DT_SHAPE_ARC) = arc; 7 is accepted
          as a block-sizing code but emits no shape, and 700 = skip.  For arcs,
          mid_x/mid_y follow at body +35/+39.
      - id: arc_body
        size: 24
        if: shape_type.value == 2 or shape_type.value == 6
        doc: |
          Bytes body +19..+42 for arc records (shape_type 2 or 6); the arc midpoint
          (mid_x at body +35, mid_y at body +39) lives inside this run (the importer
          requires body +43 <= next boundary).  Absent for non-arc shapes.

  fp_chain_shape:
    doc: |
      v46+ chained fixed-size shape record (FindShapesInChainedBlocks, parser
      ~2308-2398; FP_CHAIN_SHAPE_RECORD_SIZE = 76).  The block starts at
      padRegionEnd + 72 (FP_CHAIN_SHAPE_DATA_OFFSET) with the count at
      padRegionEnd + 69.  Records 0 and N-1 are framing entries; only the named
      fields are decoded.  For footprints with NPTH mounting holes, the final
      framing entry can carry a mount_hole_block overlay at byte +44.
    seq:
      - id: head
        size: 37
        doc: Bytes +0..+36 not assigned by the importer.
      - id: width
        type: int4
        doc: Line width at record +37 (FP_CHAIN_SHAPE_WIDTH_OFFSET).
      - id: shape_type
        type: int3
        doc: Raw type at record +41 (2 = line, 3 = arc).
      - id: x1
        type: int4
        doc: X1 at record +44.
      - id: y1
        type: int4
        doc: Y1 at record +48.
      - id: x2
        type: int4
        doc: X2 at record +52 (arc midpoint X for arcs).
      - id: y2
        type: int4
        doc: Y2 at record +56 (arc midpoint Y for arcs).
      - id: x3
        type: int4
        doc: X3 at record +60 (arc end X; only used for arcs).
      - id: y3
        type: int4
        doc: Y3 at record +64 (arc end Y; only used for arcs).
      - id: tail
        size: 8
        doc: Bytes +68..+75 not assigned by the importer.

  fp_chain_shape_mount_hole_overlay:
    doc: |
      Overlay for final v46+ chained-shape framing entries that carry footprint
      mounting holes.  The mount_hole_block starts at byte +44 of the 76-byte
      final framing record, sharing bytes with the otherwise-unassigned framing
      payload.
    seq:
      - id: prefix
        size: 44
      - id: mount_holes
        type: mount_hole_block

  net_sentinel:
    doc: int3(0), int3(-1), int3(-1), immediately before a net record (NET_SENTINEL).
    seq:
      - id: zero
        contents: [0x0f, 0x42, 0x40]
      - id: minus_one_a
        contents: [0x0f, 0x42, 0x3f]
      - id: minus_one_b
        contents: [0x0f, 0x42, 0x3f]

  net_record:
    doc: |
      Net record (FindAndParseNets).  The net section is field-anchored by its
      record count: an int3(net_count) sits five bytes ahead of the index-0
      net_sentinel (some versions prepend int3(0) and a 01 00 flag pair).  Each
      record begins with the 9-byte net_sentinel, then int3(net_index),
      int3(field_0, a per-net mode flag in [0,10]), int4(trace_width),
      int4(width_2), string(net_name).  Routing (net_routing_preamble +
      track_chains) follows in the same net body (ParseNetRouting).  The records
      are walked by sequential net index, which rejects the false net_sentinel
      hits inside the component region.
      DipTrace permits empty stored names; the importer synthesizes stable KiCad
      names from the DipTrace net index for those records.
    seq:
      - id: sentinel
        type: net_sentinel
      - id: net_index
        type: int3
      - id: field_0
        type: int3
        doc: Per-net mode/route flag (observed 0 and 3); validated to [0, 10].
      - id: trace_width
        type: int4
      - id: width_2
        type: int4
      - id: name
        type: dt_string

  net_routing_preamble:
    doc: |
      Optional net-level routing preamble (ParseNetRouting, parser ~2877-2935).
      Read immediately after the net name when validation passes: int4(via OD
      default), int4(via drill default), marker u1 (<= 1), 7 zero bytes,
      int3(separator = 0), int3(pad_ref_count), then pad_ref_count pad_ref pairs.
    seq:
      - id: default_via_outer_diameter
        type: int4
      - id: default_via_drill_diameter
        type: int4
      - id: marker
        type: u1
        doc: Validated to be <= 1.
      - id: zeros
        size: 7
        doc: 7 bytes validated to be 0 (zeroBlock at startPos+9..+15).
      - id: separator
        type: int3
        doc: Validated to be 0.
      - id: pad_ref_count
        type: int3
      - id: pad_refs
        type: pad_ref
        repeat: expr
        repeat-expr: pad_ref_count.value

  pad_ref:
    doc: One net-to-pad link (6 bytes); compIndex >= 0 and padIndex > 0 to be kept.
    seq:
      - id: component_index
        type: int3
      - id: pad_index
        type: int3

  track_chain:
    doc: |
      Routing track chain (ParseNetRouting, parser ~2940-3058).  Anchored on the
      6-byte CHAIN_HEADER (00 00 00 + int3(-1)), then int3(chain_index),
      int3(node_count), then node_count fixed 41-byte track_node records.  The parser
      scans for CHAIN_HEADER; false hits are ignored unless the first following
      track_node is structurally plausible.  For a real chain, node_count must be
      [1,10000] and must not overrun the net record.
    seq:
      - id: chain_marker
        contents: [0x00, 0x00, 0x00, 0x0f, 0x42, 0x3f]
      - id: chain_index
        type: int3
      - id: node_count
        type: int3
        valid:
          min: 1
          max: 10000
      - id: nodes
        type: track_node
        repeat: expr
        repeat-expr: node_count.value

  track_node:
    doc: |
      Fixed 41-byte routing point (TRACK_NODE_SIZE = 41; parser ~2981-3051).
      Only the fields the importer assigns are named; the gaps are read-and-
      discarded filler.
    seq:
      - id: x
        type: int4
        doc: X coordinate at +0.
      - id: y
        type: int4
        doc: Y coordinate at +4.
      - id: layer
        type: int3
        doc: Copper layer index at +8 (0 = top, 1 = bottom, 14+ = inner).
      - id: unknown_11
        size: 3
        doc: Bytes +11..+13 not assigned by the importer.
      - id: width
        type: int4
        doc: Track width at +14.
      - id: via_outer_diameter
        type: int4
        doc: Via outer diameter at +18.
      - id: route_flag
        type: u1
        doc: Raw routing-point flag at +22 (semantics unresolved).
      - id: unknown_23
        size: 4
        doc: Bytes +23..+26 not assigned by the importer.
      - id: via_style_index
        type: int3
        doc: |
          Via style index at +27 (-1 = no via).  This, not route_flag, is the
          authoritative via indicator.
      - id: via_drill_diameter
        type: int4
        doc: Via drill diameter at +30.
      - id: unknown_34
        size: 3
        doc: Bytes +34..+36 not assigned by the importer.
      - id: route_mode
        type: int3
        doc: Raw routing-point mode at +37 (observed values 0, 1, 3).
      - id: tail
        type: u1
        doc: Trailing byte at +40 (0 in sampled files).

  text_section:
    doc: |
      Board TEXT-object section (FindAndParseTextObjects).  The section's
      structural anchor is 3x int3(0) (TEXT_SECTION_ZEROS) + int3(count) +
      flag_1 u1 (= 1) + flag_2 u1 (= 0); the count text_record entries are then
      walked and must all decode inside the post-component section before the
      anchor is accepted (so the section is field-located, not pattern-matched).
      Most boards carry no board TEXT section -- footprint refdes/value text is
      per-component.
    seq:
      - id: zeros
        contents: [0x0f, 0x42, 0x40, 0x0f, 0x42, 0x40, 0x0f, 0x42, 0x40]
      - id: count
        type: int3
      - id: flag_1
        type: u1
        doc: Validated to be 1.
      - id: flag_2
        type: u1
        doc: Validated to be 0.
      - id: records
        type: text_record(count.value)
        repeat: expr
        repeat-expr: count.value

  text_record:
    doc: |
      One text object (ParseTextRecords, parser ~2614-2667).  Mirrors the exact
      read order.  A 2-byte inter-record separator (0x01 0x00) follows every
      record except the last; modeled by record_separator with the
      is_last/count gate.
    params:
      - id: count
        type: s4
    seq:
      - id: type_a
        type: int3
      - id: flag_a
        type: u1
      - id: type_b
        type: int3
      - id: fields
        type: int3
        repeat: expr
        repeat-expr: 5
        doc: field_a..field_e int3 values.
      - id: color_1
        type: rgb_color
      - id: color_2
        type: rgb_color
      - id: color_3
        type: rgb_color
      - id: line_width
        type: int4
      - id: layer
        type: int3
      - id: x1
        type: int4
      - id: y1
        type: int4
      - id: x2
        type: int4
      - id: y2
        type: int4
      - id: text
        type: dt_string
      - id: font_name
        type: dt_string
      - id: separator
        type: u1
      - id: field_pf_1
        type: int3
      - id: flag_pf
        type: u1
      - id: text_offset_1
        type: int4
      - id: text_offset_2
        type: int4
      - id: record_index
        type: int3
      - id: end_flag
        type: u1
      - id: record_separator
        size: 2
        if: _index < count - 1
        doc: 2-byte inter-record separator (observed 0x01 0x00); absent after the last record.

  copper_pour_font_preamble:
    doc: |
      Zone-section lead-in consumed by FindAndParseZones before a `copper_pour`
      record.  This font preamble (font name + size + height/width + a trailing
      int4(-20000)) is the ZONE section's deterministic structural anchor: it is
      accepted only when followed immediately by a strictly valid 30-byte
      `copper_pour` header (and a zone-shaped-but-invalid header is fatal).  Font
      blocks not followed by a zone-shaped header are unrelated post-component
      data and skipped.  A zone section located through this preamble is treated
      as field-anchored; the plausible-header structural scan is the fallback.
    seq:
      - id: font_name
        type: dt_string
      - id: font_size
        type: int3
        doc: Validated by parser as 5..30.
      - id: bold
        type: u1
        doc: Validated by parser as 0 or 1.
      - id: font_height
        type: int4
        doc: Positive font height, validated below 10,000,000.
      - id: font_width
        type: int4
        doc: Positive font width, validated below 10,000,000.
      - id: tail
        type: int4
        doc: Zone font-preamble tail, validated as -20000.
      - id: zone_separator
        type: int3
        doc: Separator between the font preamble and zone header; observed int3(0).

  copper_pour:
    doc: |
      Copper-pour zone (FindAndParseZones, parser ~3205-3936; CreateZones source
      record).  Located by a validated `copper_pour_font_preamble` immediately
      before the header, or by structural scan of the post-component gap when no
      accepted preamble is present.  Layout is a 30-byte header, then the outline
      vertex run, a fill-segment block, a fill-polygon block, an optional
      post-fill style block, and (inside the inter-zone gap) an optional zone
      trailer.
    seq:
      - id: field_a
        type: int3
        doc: |
          Filled-region count at +0 (NOT the CopperPour priority; true priority
          storage is not yet located).
      - id: fill_mode
        type: u1
        doc: Zone flag byte at +3 (validated <= 2).
      - id: raw_flag_2
        type: u1
        doc: Zone flag byte at +4 (semantics unknown).
      - id: connection_mode
        type: u1
        doc: Zone flag byte at +5 (validated <= 2).
      - id: min_width
        type: int4
        doc: Copper pour line width at +6 (DipXML LineWidth).
      - id: clearance
        type: int4
        doc: Zone clearance at +10 (DipTrace units).
      - id: minimum_area
        type: int4
        doc: Minimum island area scalar at +14 (DipXML MinimumArea).
      - id: separator
        type: int3
        doc: Separator at +18 (validated <= 0).
      - id: layer
        type: int3
        doc: DipTrace layer at +21 (0 = top, 1 = bottom).
      - id: field_b
        type: int3
        doc: Connected net id at +24 (-1 = unconnected).
      - id: vertex_count
        type: int3
        doc: Outline vertex count at +27 (validated 3..50000).
      - id: outline
        type: copper_pour_vertex
        repeat: expr
        repeat-expr: vertex_count.value
      - id: fill_segment_count
        type: int3
      - id: fill_segments
        size: 19
        repeat: expr
        repeat-expr: fill_segment_count.value
        doc: Cached fill segments (19 bytes each); skipped by the importer.
      - id: fill_poly_count
        type: int3
      - id: fill_polys
        type: copper_pour_fill_poly
        repeat: expr
        repeat-expr: fill_poly_count.value
        doc: Cached fill polygons; skipped by the importer.
      - id: style_block
        type: copper_pour_style
        doc: |
          Optional post-fill style block; only applied when its validation
          passes (styleLead == 0, spokeMode 0..4, positive line_spacing and
          spoke_width).  Always 14 bytes when present.

  copper_pour_vertex:
    doc: One zone outline vertex (8 bytes), validated against board bounds.
    seq:
      - id: x
        type: int4
      - id: y
        type: int4

  copper_pour_fill_poly:
    doc: |
      One cached fill polygon (parser ~3850-3872): int3(vertex_count) + vertices
      (8 bytes each) + a 3-byte trailer.  Skipped by the importer.
    seq:
      - id: vertex_count
        type: int3
      - id: vertices
        type: copper_pour_vertex
        repeat: expr
        repeat-expr: vertex_count.value
      - id: trailer
        size: 3

  copper_pour_style:
    doc: |
      Post-fill thermal style block (parser ~3880-3903): int3(lead = 0),
      int3(spoke_mode), int4(line_spacing), int4(spoke_width).  spoke_mode maps
      to the UI enum (0 = Direct, 1 = 2 spoke 90, 2 = 2 spoke, 3 = 4 spoke 45,
      4 = 4 spoke).
    seq:
      - id: lead
        type: int3
      - id: spoke_mode
        type: int3
      - id: line_spacing
        type: int4
      - id: spoke_width
        type: int4

  copper_pour_trailer:
    doc: |
      Zone trailer (parseZoneTrailer lambda, parser ~3319-3445; TRAILER_LEN = 28).
      Found by backward scan inside the inter-zone gap; preceded optionally by a
      14-byte style block (copper_pour_style) and a run of 23-byte cached-fill
      records (copper_pour_cached_fill).  Carries the DipTrace zone properties
      that have no native KiCad equivalent (BoardClearance, ViaDirect, SMD
      spoke, island flags) consumed by GenerateDesignRules().
    seq:
      - id: regions_counted
        type: int3
        doc: lead at +0 (CopperPour Regions_Counted); validated 0..100000.
      - id: zero_int4
        type: int4
        doc: int4 at +3, validated to be 0.
      - id: board_clearance
        type: int4
        doc: Zone-to-board-edge clearance at +7.
      - id: island_region
        type: u1
        doc: IslandRegion flag at +11 (<= 1).
      - id: island_internal
        type: u1
        doc: IslandInternal flag at +12 (<= 1).
      - id: island_connection
        type: u1
        doc: IslandConnection flag at +13 (<= 1).
      - id: zone_id
        type: int3
        doc: Per-zone id at +14 (DipXML CopperPour@Id); validated 0..100000.
      - id: via_direct
        type: u1
        doc: ViaDirect flag at +17 (<= 1).
      - id: smd_separate
        type: u1
        doc: SMD_Separate flag at +18 (<= 1).
      - id: smd_spoke_mode
        type: int3
        doc: SMD_Spoke enum at +19 (0..4).
      - id: smd_spoke_width
        type: int4
        doc: SMD_SpokeWidth at +22 (> 0).
      - id: ratline_mode
        type: u1
        doc: Ratline mode at +26 (0 = Automatically, 1 = All Ratlines, 2 = Do Not Hide).
      - id: regions_done
        type: u1
        doc: RegionsDone flag at +27 (<= 1).

  copper_pour_cached_fill:
    doc: |
      Raw 23-byte cached-fill record (CACHED_RECORD_LEN = 23; parser
      ~3411-3425), present in the inter-zone gap when the payload between the
      style block and the trailer is 23-byte aligned.  All fields are raw,
      unassigned scalars.
    seq:
      - id: field0
        type: int3
      - id: field1
        type: int4
      - id: field2
        type: int4
      - id: field3
        type: int4
      - id: field4
        type: int4
      - id: field5
        type: int4
