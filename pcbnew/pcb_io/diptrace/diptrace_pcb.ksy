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
# The post-design-rules tail is exposed as `post_via_styles_tail` (a typed bounded
# substream) with a named type for every decoded structure, because Kaitai cannot
# express the content-anchored offsets the field-walk computes at runtime; the
# named types below document the true sequential record layouts; bytes between
# content-derived anchors are modeled as one-byte anchor-scan advances.

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
    type: post_via_styles_tail(ver)
    size-eos: true
    doc: |
      Bounded substream holding all anchored records after the design-rule section.
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
  post_via_styles_tail:
    doc: |
      Bounded PCB stream after the design-rule section.  The importer derives
      section and object offsets from anchored structural signatures rather than
      a single sequential repeat that Kaitai can express.  This container exposes
      recognized anchors plus one-byte anchor-scan advances; the concrete anchored
      layouts are modeled by the component, pad, footprint-shape, mount-hole,
      text, net, track, and copper-pour types below.
    params:
      - id: version
        type: s4
    seq:
      - id: tokens
        type: post_via_styles_tail_token(version)
        repeat: eos

  post_via_styles_tail_token:
    doc: |
      One token in the bounded PCB stream after design rules.  Known structural
      anchors are parsed at their marker; all other bytes are explicit one-byte
      anchor-scan advances because the importer derives object and section
      offsets from field-walked anchors.
    params:
      - id: version
        type: s4
    seq:
      - id: component_record_std
        type: component_record_anchor(false)
        if: is_component_boundary_std
      - id: component_record_alt
        type: component_record_anchor(true)
        if: 'not is_component_boundary_std and is_component_boundary_alt'
      - id: net
        type: net_record
        if: 'not is_component_boundary_std and not is_component_boundary_alt and is_net_record_start'
      - id: component_tail_record
        type: component_tail
        if: 'not is_component_boundary_std and not is_component_boundary_alt and not is_net_record_start and is_component_tail_start'
      - id: pad_record
        type: pad
        if: 'not is_component_boundary_std and not is_component_boundary_alt and not is_net_record_start and not is_component_tail_start and is_pad_start'
      - id: mount_hole_record
        type: mount_hole_block
        if: 'not is_component_boundary_std and not is_component_boundary_alt and not is_net_record_start and not is_component_tail_start and not is_pad_start and is_mount_hole_block_start'
      - id: footprint_shape_v37
        type: fp_shape_v37
        if: 'not is_component_boundary_std and not is_component_boundary_alt and not is_net_record_start and not is_component_tail_start and not is_pad_start and not is_mount_hole_block_start and is_fp_shape_v37_start'
      - id: footprint_shape
        type: fp_shape
        if: 'not is_component_boundary_std and not is_component_boundary_alt and not is_net_record_start and not is_component_tail_start and not is_pad_start and not is_mount_hole_block_start and not is_fp_shape_v37_start and is_fp_shape_start'
      - id: footprint_font_block
        type: fp_font_block
        if: 'not is_component_boundary_std and not is_component_boundary_alt and not is_net_record_start and not is_component_tail_start and not is_pad_start and not is_mount_hole_block_start and not is_fp_shape_v37_start and not is_fp_shape_start and is_fp_font_block_start'
      - id: footprint_chain_shape
        type: fp_chain_shape
        if: 'not is_component_boundary_std and not is_component_boundary_alt and not is_net_record_start and not is_component_tail_start and not is_pad_start and not is_mount_hole_block_start and not is_fp_shape_v37_start and not is_fp_shape_start and not is_fp_font_block_start and is_fp_chain_shape_start'
      - id: copper_pour_font
        type: copper_pour_font_preamble
        if: 'not is_component_boundary_std and not is_component_boundary_alt and not is_net_record_start and not is_component_tail_start and not is_pad_start and not is_mount_hole_block_start and not is_fp_shape_v37_start and not is_fp_shape_start and not is_fp_font_block_start and not is_fp_chain_shape_start and is_copper_pour_font_preamble_start'
      - id: copper_pour_record
        type: copper_pour
        if: 'not is_component_boundary_std and not is_component_boundary_alt and not is_net_record_start and not is_component_tail_start and not is_pad_start and not is_mount_hole_block_start and not is_fp_shape_v37_start and not is_fp_shape_start and not is_fp_font_block_start and not is_fp_chain_shape_start and not is_copper_pour_font_preamble_start and is_copper_pour_start'
      - id: copper_pour_style_record
        type: copper_pour_style
        if: 'not is_component_boundary_std and not is_component_boundary_alt and not is_net_record_start and not is_component_tail_start and not is_pad_start and not is_mount_hole_block_start and not is_fp_shape_v37_start and not is_fp_shape_start and not is_fp_font_block_start and not is_fp_chain_shape_start and not is_copper_pour_font_preamble_start and not is_copper_pour_start and is_copper_pour_style_start'
      - id: copper_pour_cached_fill_record
        type: copper_pour_cached_fill
        if: 'not is_component_boundary_std and not is_component_boundary_alt and not is_net_record_start and not is_component_tail_start and not is_pad_start and not is_mount_hole_block_start and not is_fp_shape_v37_start and not is_fp_shape_start and not is_fp_font_block_start and not is_fp_chain_shape_start and not is_copper_pour_font_preamble_start and not is_copper_pour_start and not is_copper_pour_style_start and is_copper_pour_cached_fill_start'
      - id: copper_pour_trailer_record
        type: copper_pour_trailer
        if: 'not is_component_boundary_std and not is_component_boundary_alt and not is_net_record_start and not is_component_tail_start and not is_pad_start and not is_mount_hole_block_start and not is_fp_shape_v37_start and not is_fp_shape_start and not is_fp_font_block_start and not is_fp_chain_shape_start and not is_copper_pour_font_preamble_start and not is_copper_pour_start and not is_copper_pour_style_start and not is_copper_pour_cached_fill_start and is_copper_pour_trailer_start'
      - id: track
        type: track_chain
        if: 'not is_component_boundary_std and not is_component_boundary_alt and not is_net_record_start and not is_component_tail_start and not is_pad_start and not is_mount_hole_block_start and not is_fp_shape_v37_start and not is_fp_shape_start and not is_fp_font_block_start and not is_fp_chain_shape_start and not is_copper_pour_font_preamble_start and not is_copper_pour_start and not is_copper_pour_style_start and not is_copper_pour_cached_fill_start and not is_copper_pour_trailer_start and is_track_chain_start'
      - id: text
        type: text_section
        if: 'not is_component_boundary_std and not is_component_boundary_alt and not is_net_record_start and not is_component_tail_start and not is_pad_start and not is_mount_hole_block_start and not is_fp_shape_v37_start and not is_fp_shape_start and not is_fp_font_block_start and not is_fp_chain_shape_start and not is_copper_pour_font_preamble_start and not is_copper_pour_start and not is_copper_pour_style_start and not is_copper_pour_cached_fill_start and not is_copper_pour_trailer_start and not is_track_chain_start and is_text_section_start'
      - id: anchor_scan_byte
        type: u1
        doc: One byte advanced while the importer walks toward the next content-derived anchor.
        if: 'not is_component_boundary_std and not is_component_boundary_alt and not is_net_record_start and not is_component_tail_start and not is_pad_start and not is_mount_hole_block_start and not is_fp_shape_v37_start and not is_fp_shape_start and not is_fp_font_block_start and not is_fp_chain_shape_start and not is_copper_pour_font_preamble_start and not is_copper_pour_start and not is_copper_pour_style_start and not is_copper_pour_cached_fill_start and not is_copper_pour_trailer_start and not is_track_chain_start and not is_text_section_start'
    instances:
      b0:
        pos: _io.pos
        type: u1
        if: _io.size - _io.pos >= 1
      b1:
        pos: _io.pos + 1
        type: u1
        if: _io.size - _io.pos >= 2
      b2:
        pos: _io.pos + 2
        type: u1
        if: _io.size - _io.pos >= 3
      b3:
        pos: _io.pos + 3
        type: u1
        if: _io.size - _io.pos >= 4
      b4:
        pos: _io.pos + 4
        type: u1
        if: _io.size - _io.pos >= 5
      b5:
        pos: _io.pos + 5
        type: u1
        if: _io.size - _io.pos >= 6
      b6:
        pos: _io.pos + 6
        type: u1
        if: _io.size - _io.pos >= 7
      b7:
        pos: _io.pos + 7
        type: u1
        if: _io.size - _io.pos >= 8
      b8:
        pos: _io.pos + 8
        type: u1
        if: _io.size - _io.pos >= 9
      b9:
        pos: _io.pos + 9
        type: u1
        if: _io.size - _io.pos >= 10
      b10:
        pos: _io.pos + 10
        type: u1
        if: _io.size - _io.pos >= 11
      b11:
        pos: _io.pos + 11
        type: u1
        if: _io.size - _io.pos >= 12
      b12:
        pos: _io.pos + 12
        type: u1
        if: _io.size - _io.pos >= 13
      b13:
        pos: _io.pos + 13
        type: u1
        if: _io.size - _io.pos >= 14
      text_count_probe:
        pos: _io.pos + 9
        type: int3
        if: _io.size - _io.pos >= 14
      track_node_count_probe:
        pos: _io.pos + 9
        type: int3
        if: _io.size - _io.pos >= 12
      component_tail_check_int4_a:
        pos: _io.pos + 11
        type: int4
        if: _io.size - _io.pos >= 37
      component_tail_check_int4_b:
        pos: _io.pos + 15
        type: int4
        if: _io.size - _io.pos >= 37
      component_tail_side_flag_1:
        pos: _io.pos + 19
        type: u1
        if: _io.size - _io.pos >= 37
      component_tail_visibility:
        pos: _io.pos + 20
        type: int3
        if: _io.size - _io.pos >= 37
      component_tail_side_flag_2:
        pos: _io.pos + 23
        type: u1
        if: _io.size - _io.pos >= 37
      component_tail_has_offset:
        pos: _io.pos + 35
        type: u1
        if: _io.size - _io.pos >= 37
      component_tail_terminator:
        pos: _io.pos + 36
        type: u1
        if: _io.size - _io.pos >= 37
      pad_index_probe:
        pos: _io.pos
        type: int3
        if: _io.size - _io.pos >= 18
      pad_net_index_probe:
        pos: _io.pos + 3
        type: int3
        if: _io.size - _io.pos >= 18
      pad_x_probe:
        pos: _io.pos + 6
        type: int4
        if: _io.size - _io.pos >= 18
      pad_y_probe:
        pos: _io.pos + 10
        type: int4
        if: _io.size - _io.pos >= 18
      pad_number_len_utf16:
        pos: _io.pos + 14
        type: u2
        if: _root.ver > 37 and _io.size - _io.pos >= 16
      pad_label_len_utf16:
        pos: _io.pos + 16 + pad_number_len_utf16 * 2
        type: u2
        if: _root.ver > 37 and _io.size - _io.pos >= 18 + pad_number_len_utf16 * 2 and pad_number_len_utf16 <= 64
      pad_width_utf16:
        pos: _io.pos + 18 + pad_number_len_utf16 * 2 + pad_label_len_utf16 * 2
        type: int4
        if: _root.ver > 37 and _io.size - _io.pos >= 22 + pad_number_len_utf16 * 2 + pad_label_len_utf16 * 2 and pad_number_len_utf16 <= 64 and pad_label_len_utf16 <= 64
      pad_height_utf16:
        pos: _io.pos + 22 + pad_number_len_utf16 * 2 + pad_label_len_utf16 * 2
        type: int4
        if: _root.ver > 37 and _io.size - _io.pos >= 26 + pad_number_len_utf16 * 2 + pad_label_len_utf16 * 2 and pad_number_len_utf16 <= 64 and pad_label_len_utf16 <= 64
      pad_number_len_ascii:
        pos: _io.pos + 14
        type: int3
        if: _root.ver <= 37 and _io.size - _io.pos >= 17
      pad_label_len_ascii:
        pos: _io.pos + 17 + pad_number_len_ascii.value
        type: int3
        if: _root.ver <= 37 and _io.size - _io.pos >= 20 + pad_number_len_ascii.value and pad_number_len_ascii.value >= 0 and pad_number_len_ascii.value <= 64
      pad_width_ascii:
        pos: _io.pos + 20 + pad_number_len_ascii.value + pad_label_len_ascii.value
        type: int4
        if: _root.ver <= 37 and _io.size - _io.pos >= 24 + pad_number_len_ascii.value + pad_label_len_ascii.value and pad_number_len_ascii.value >= 0 and pad_number_len_ascii.value <= 64 and pad_label_len_ascii.value >= 0 and pad_label_len_ascii.value <= 64
      pad_height_ascii:
        pos: _io.pos + 24 + pad_number_len_ascii.value + pad_label_len_ascii.value
        type: int4
        if: _root.ver <= 37 and _io.size - _io.pos >= 28 + pad_number_len_ascii.value + pad_label_len_ascii.value and pad_number_len_ascii.value >= 0 and pad_number_len_ascii.value <= 64 and pad_label_len_ascii.value >= 0 and pad_label_len_ascii.value <= 64
      mount_hole_count_field:
        pos: _io.pos
        type: int3
        if: _io.size - _io.pos >= 56
      mount_hole_header_flag:
        pos: _io.pos + 3
        type: u1
        if: _io.size - _io.pos >= 56
      mount_hole_zero_a:
        pos: _io.pos + 4
        type: int4
        if: _io.size - _io.pos >= 56
      mount_hole_zero_b:
        pos: _io.pos + 8
        type: int4
        if: _io.size - _io.pos >= 56
      mount_hole_zero_c:
        pos: _io.pos + 12
        type: int4
        if: _io.size - _io.pos >= 56
      mount_hole_zero_d:
        pos: _io.pos + 16
        type: int4
        if: _io.size - _io.pos >= 56
      mount_hole_first_flag_a:
        pos: _io.pos + 20
        type: u1
        if: _io.size - _io.pos >= 56
      mount_hole_first_flag_b:
        pos: _io.pos + 21
        type: u1
        if: _io.size - _io.pos >= 56
      mount_hole_first_outer:
        pos: _io.pos + 38
        type: int4
        if: _io.size - _io.pos >= 56
      mount_hole_first_drill:
        pos: _io.pos + 42
        type: int4
        if: _io.size - _io.pos >= 56
      fp_shape_type:
        pos: _io.pos
        type: int3
        if: _io.size - _io.pos >= 60
      fp_shape_x1:
        pos: _io.pos + 3
        type: int4
        if: _io.size - _io.pos >= 60
      fp_shape_y1:
        pos: _io.pos + 7
        type: int4
        if: _io.size - _io.pos >= 60
      fp_shape_x2:
        pos: _io.pos + 11
        type: int4
        if: _io.size - _io.pos >= 60
      fp_shape_y2:
        pos: _io.pos + 15
        type: int4
        if: _io.size - _io.pos >= 60
      fp_shape_style_flag:
        pos: _io.pos + 27
        type: u1
        if: _io.size - _io.pos >= 60
      fp_shape_v45_width:
        pos: _io.pos + 53
        type: int4
        if: _io.size - _io.pos >= 60
      fp_shape_v45_layer:
        pos: _io.pos + 57
        type: int3
        if: _io.size - _io.pos >= 60
      fp_shape_v37_width:
        pos: _io.pos + 55
        type: int4
        if: _io.size - _io.pos >= 62
      fp_shape_v37_layer:
        pos: _io.pos + 59
        type: int3
        if: _io.size - _io.pos >= 62
      fp_font_meta_flag:
        pos: _io.pos + 14
        type: u1
        if: _io.size - _io.pos >= 58
      fp_font_size:
        pos: _io.pos + 15
        type: int3
        if: _io.size - _io.pos >= 58
      fp_font_height:
        pos: _io.pos + 18
        type: int4
        if: _io.size - _io.pos >= 58
      fp_font_width:
        pos: _io.pos + 22
        type: int4
        if: _io.size - _io.pos >= 58
      fp_font_line_width:
        pos: _io.pos + 32
        type: int4
        if: _io.size - _io.pos >= 58
      fp_font_layer:
        pos: _io.pos + 36
        type: int3
        if: _io.size - _io.pos >= 58
      fp_font_x1:
        pos: _io.pos + 39
        type: int4
        if: _io.size - _io.pos >= 58
      fp_font_y1:
        pos: _io.pos + 43
        type: int4
        if: _io.size - _io.pos >= 58
      fp_font_x2:
        pos: _io.pos + 47
        type: int4
        if: _io.size - _io.pos >= 58
      fp_font_y2:
        pos: _io.pos + 51
        type: int4
        if: _io.size - _io.pos >= 58
      fp_font_shape_type:
        pos: _io.pos + 55
        type: int3
        if: _io.size - _io.pos >= 58
      fp_chain_pad_a:
        pos: _io.pos + 3
        type: u1
        if: _io.size - _io.pos >= 76
      fp_chain_pad_b:
        pos: _io.pos + 4
        type: u1
        if: _io.size - _io.pos >= 76
      fp_chain_font_len:
        pos: _io.pos + 5
        type: u2
        if: _io.size - _io.pos >= 76
      fp_chain_font_t:
        pos: _io.pos + 7
        type: u2
        if: _io.size - _io.pos >= 76
      fp_chain_font_a:
        pos: _io.pos + 9
        type: u2
        if: _io.size - _io.pos >= 76
      fp_chain_font_h:
        pos: _io.pos + 11
        type: u2
        if: _io.size - _io.pos >= 76
      fp_chain_font_o:
        pos: _io.pos + 13
        type: u2
        if: _io.size - _io.pos >= 76
      fp_chain_font_m:
        pos: _io.pos + 15
        type: u2
        if: _io.size - _io.pos >= 76
      fp_chain_font_a2:
        pos: _io.pos + 17
        type: u2
        if: _io.size - _io.pos >= 76
      fp_chain_width:
        pos: _io.pos + 37
        type: int4
        if: _io.size - _io.pos >= 76
      fp_chain_shape_type:
        pos: _io.pos + 41
        type: int3
        if: _io.size - _io.pos >= 76
      fp_chain_x1:
        pos: _io.pos + 44
        type: int4
        if: _io.size - _io.pos >= 76
      fp_chain_y1:
        pos: _io.pos + 48
        type: int4
        if: _io.size - _io.pos >= 76
      fp_chain_x2:
        pos: _io.pos + 52
        type: int4
        if: _io.size - _io.pos >= 76
      fp_chain_y2:
        pos: _io.pos + 56
        type: int4
        if: _io.size - _io.pos >= 76
      copper_font_len_utf16:
        pos: _io.pos
        type: u2
        if: _root.ver > 37 and _io.size - _io.pos >= 2
      copper_font_size_utf16:
        pos: _io.pos + 2 + copper_font_len_utf16 * 2
        type: int3
        if: _root.ver > 37 and _io.size - _io.pos >= 5 + copper_font_len_utf16 * 2 and copper_font_len_utf16 >= 1 and copper_font_len_utf16 <= 64
      copper_font_bold_utf16:
        pos: _io.pos + 5 + copper_font_len_utf16 * 2
        type: u1
        if: _root.ver > 37 and _io.size - _io.pos >= 6 + copper_font_len_utf16 * 2 and copper_font_len_utf16 >= 1 and copper_font_len_utf16 <= 64
      copper_font_height_utf16:
        pos: _io.pos + 6 + copper_font_len_utf16 * 2
        type: int4
        if: _root.ver > 37 and _io.size - _io.pos >= 10 + copper_font_len_utf16 * 2 and copper_font_len_utf16 >= 1 and copper_font_len_utf16 <= 64
      copper_font_width_utf16:
        pos: _io.pos + 10 + copper_font_len_utf16 * 2
        type: int4
        if: _root.ver > 37 and _io.size - _io.pos >= 14 + copper_font_len_utf16 * 2 and copper_font_len_utf16 >= 1 and copper_font_len_utf16 <= 64
      copper_font_tail_utf16:
        pos: _io.pos + 14 + copper_font_len_utf16 * 2
        type: int4
        if: _root.ver > 37 and _io.size - _io.pos >= 18 + copper_font_len_utf16 * 2 and copper_font_len_utf16 >= 1 and copper_font_len_utf16 <= 64
      copper_font_separator_utf16:
        pos: _io.pos + 18 + copper_font_len_utf16 * 2
        type: int3
        if: _root.ver > 37 and _io.size - _io.pos >= 21 + copper_font_len_utf16 * 2 and copper_font_len_utf16 >= 1 and copper_font_len_utf16 <= 64
      copper_font_len_ascii:
        pos: _io.pos
        type: int3
        if: _root.ver <= 37 and _io.size - _io.pos >= 3
      copper_font_size_ascii:
        pos: _io.pos + 3 + copper_font_len_ascii.value
        type: int3
        if: _root.ver <= 37 and _io.size - _io.pos >= 6 + copper_font_len_ascii.value and copper_font_len_ascii.value >= 1 and copper_font_len_ascii.value <= 64
      copper_font_bold_ascii:
        pos: _io.pos + 6 + copper_font_len_ascii.value
        type: u1
        if: _root.ver <= 37 and _io.size - _io.pos >= 7 + copper_font_len_ascii.value and copper_font_len_ascii.value >= 1 and copper_font_len_ascii.value <= 64
      copper_font_height_ascii:
        pos: _io.pos + 7 + copper_font_len_ascii.value
        type: int4
        if: _root.ver <= 37 and _io.size - _io.pos >= 11 + copper_font_len_ascii.value and copper_font_len_ascii.value >= 1 and copper_font_len_ascii.value <= 64
      copper_font_width_ascii:
        pos: _io.pos + 11 + copper_font_len_ascii.value
        type: int4
        if: _root.ver <= 37 and _io.size - _io.pos >= 15 + copper_font_len_ascii.value and copper_font_len_ascii.value >= 1 and copper_font_len_ascii.value <= 64
      copper_font_tail_ascii:
        pos: _io.pos + 15 + copper_font_len_ascii.value
        type: int4
        if: _root.ver <= 37 and _io.size - _io.pos >= 19 + copper_font_len_ascii.value and copper_font_len_ascii.value >= 1 and copper_font_len_ascii.value <= 64
      copper_font_separator_ascii:
        pos: _io.pos + 19 + copper_font_len_ascii.value
        type: int3
        if: _root.ver <= 37 and _io.size - _io.pos >= 22 + copper_font_len_ascii.value and copper_font_len_ascii.value >= 1 and copper_font_len_ascii.value <= 64
      copper_pour_fill_mode:
        pos: _io.pos + 3
        type: u1
        if: _io.size - _io.pos >= 30
      copper_pour_connection_mode:
        pos: _io.pos + 5
        type: u1
        if: _io.size - _io.pos >= 30
      copper_pour_min_width:
        pos: _io.pos + 6
        type: int4
        if: _io.size - _io.pos >= 30
      copper_pour_clearance:
        pos: _io.pos + 10
        type: int4
        if: _io.size - _io.pos >= 30
      copper_pour_min_area:
        pos: _io.pos + 14
        type: int4
        if: _io.size - _io.pos >= 30
      copper_pour_separator:
        pos: _io.pos + 18
        type: int3
        if: _io.size - _io.pos >= 30
      copper_pour_layer:
        pos: _io.pos + 21
        type: int3
        if: _io.size - _io.pos >= 30
      copper_pour_vertex_count:
        pos: _io.pos + 27
        type: int3
        if: _io.size - _io.pos >= 30
      copper_style_lead:
        pos: _io.pos
        type: int3
        if: _io.size - _io.pos >= 14
      copper_style_spoke_mode:
        pos: _io.pos + 3
        type: int3
        if: _io.size - _io.pos >= 14
      copper_style_line_spacing:
        pos: _io.pos + 6
        type: int4
        if: _io.size - _io.pos >= 14
      copper_style_spoke_width:
        pos: _io.pos + 10
        type: int4
        if: _io.size - _io.pos >= 14
      copper_cached_field0:
        pos: _io.pos
        type: int3
        if: _io.size - _io.pos >= 23
      copper_cached_field1:
        pos: _io.pos + 3
        type: int4
        if: _io.size - _io.pos >= 23
      copper_cached_field2:
        pos: _io.pos + 7
        type: int4
        if: _io.size - _io.pos >= 23
      copper_cached_field3:
        pos: _io.pos + 11
        type: int4
        if: _io.size - _io.pos >= 23
      copper_cached_field4:
        pos: _io.pos + 15
        type: int4
        if: _io.size - _io.pos >= 23
      copper_cached_field5:
        pos: _io.pos + 19
        type: int4
        if: _io.size - _io.pos >= 23
      copper_trailer_regions_counted:
        pos: _io.pos
        type: int3
        if: _io.size - _io.pos >= 28
      copper_trailer_zero:
        pos: _io.pos + 3
        type: int4
        if: _io.size - _io.pos >= 28
      copper_trailer_board_clearance:
        pos: _io.pos + 7
        type: int4
        if: _io.size - _io.pos >= 28
      copper_trailer_island_region:
        pos: _io.pos + 11
        type: u1
        if: _io.size - _io.pos >= 28
      copper_trailer_island_internal:
        pos: _io.pos + 12
        type: u1
        if: _io.size - _io.pos >= 28
      copper_trailer_island_connection:
        pos: _io.pos + 13
        type: u1
        if: _io.size - _io.pos >= 28
      copper_trailer_zone_id:
        pos: _io.pos + 14
        type: int3
        if: _io.size - _io.pos >= 28
      copper_trailer_via_direct:
        pos: _io.pos + 17
        type: u1
        if: _io.size - _io.pos >= 28
      copper_trailer_smd_separate:
        pos: _io.pos + 18
        type: u1
        if: _io.size - _io.pos >= 28
      copper_trailer_smd_spoke_mode:
        pos: _io.pos + 19
        type: int3
        if: _io.size - _io.pos >= 28
      copper_trailer_smd_spoke_width:
        pos: _io.pos + 22
        type: int4
        if: _io.size - _io.pos >= 28
      copper_trailer_ratline_mode:
        pos: _io.pos + 26
        type: u1
        if: _io.size - _io.pos >= 28
      copper_trailer_regions_done:
        pos: _io.pos + 27
        type: u1
        if: _io.size - _io.pos >= 28
      is_component_boundary_std:
        value: '_io.size - _io.pos >= 13 and b0 == 0x0f and b1 == 0x42 and b2 == 0x40 and b3 == 0x0f and b4 == 0x42 and b5 == 0x3f and b6 == 0x0f and b7 == 0x42 and b8 == 0x3f and b9 == 0x3b and b10 == 0x9a and b11 == 0xca and b12 == 0x00'
      is_component_boundary_alt:
        value: '_io.size - _io.pos >= 13 and b0 == 0x0f and b1 == 0x42 and b2 == 0x40 and b3 == 0x0f and b4 == 0x42 and b5 == 0x40 and b6 == 0x0f and b7 == 0x42 and b8 == 0x40 and b9 == 0x3b and b10 == 0x9a and b11 == 0xca and b12 == 0x00'
      is_net_record_start:
        value: '_io.size - _io.pos >= 12 and b0 == 0x0f and b1 == 0x42 and b2 == 0x40 and b3 == 0x0f and b4 == 0x42 and b5 == 0x3f and b6 == 0x0f and b7 == 0x42 and b8 == 0x3f'
      is_component_tail_start:
        value: '_io.size - _io.pos >= 37 and b0 == 0x0f and b1 == 0x42 and b2 == 0x40 and b3 == 0x3b and b4 == 0x9a and b5 == 0xca and b6 == 0x00 and b7 == 0x3b and b8 == 0x9a and b9 == 0xca and b10 == 0x00 and component_tail_check_int4_a.value == 0 and component_tail_check_int4_b.value == 0 and component_tail_side_flag_1 <= 1 and (component_tail_visibility.value == 0 or component_tail_visibility.value == -1) and component_tail_side_flag_2 <= 1 and component_tail_has_offset <= 1 and component_tail_terminator == 0'
      is_pad_start:
        value: '_io.size - _io.pos >= 18 and pad_index_probe.value >= 1 and pad_index_probe.value < 500 and pad_net_index_probe.value >= -1 and pad_net_index_probe.value <= 500 and pad_x_probe.value >= -50000000 and pad_x_probe.value <= 50000000 and pad_y_probe.value >= -50000000 and pad_y_probe.value <= 50000000 and (_root.ver > 37 ? (pad_number_len_utf16 <= 64 and pad_label_len_utf16 <= 64 and pad_width_utf16.value > 0 and pad_width_utf16.value <= 10000000 and pad_height_utf16.value > 0 and pad_height_utf16.value <= 10000000) : (pad_number_len_ascii.value >= 0 and pad_number_len_ascii.value <= 64 and pad_label_len_ascii.value >= 0 and pad_label_len_ascii.value <= 64 and pad_width_ascii.value > 0 and pad_width_ascii.value <= 10000000 and pad_height_ascii.value > 0 and pad_height_ascii.value <= 10000000))'
      is_mount_hole_block_start:
        value: '_io.size - _io.pos >= 56 and mount_hole_count_field.value >= 3 and mount_hole_count_field.value <= 66 and mount_hole_header_flag <= 1 and mount_hole_zero_a.value == 0 and mount_hole_zero_b.value == 0 and mount_hole_zero_c.value == 0 and mount_hole_zero_d.value == 0 and mount_hole_first_flag_a == 0 and mount_hole_first_flag_b <= 1 and mount_hole_first_outer.value > 0 and mount_hole_first_outer.value <= 50000000 and mount_hole_first_drill.value > 0 and mount_hole_first_drill.value <= mount_hole_first_outer.value'
      is_fp_shape_start:
        value: '_root.ver > 37 and _io.size - _io.pos >= 60 and (fp_shape_type.value == -1 or fp_shape_type.value == 0 or fp_shape_type.value == 1 or fp_shape_type.value == 2 or fp_shape_type.value == 3 or fp_shape_type.value == 6) and fp_shape_x1.value >= -20000 and fp_shape_x1.value <= 20000 and fp_shape_y1.value >= -20000 and fp_shape_y1.value <= 20000 and fp_shape_x2.value >= -20000 and fp_shape_x2.value <= 20000 and fp_shape_y2.value >= -20000 and fp_shape_y2.value <= 20000 and fp_shape_style_flag <= 1 and fp_shape_v45_width.value >= -10000 and fp_shape_v45_width.value <= 5000000 and fp_shape_v45_layer.value >= -1 and fp_shape_v45_layer.value <= 100'
      is_fp_shape_v37_start:
        value: '_root.ver <= 37 and _io.size - _io.pos >= 62 and (fp_shape_type.value == -1 or fp_shape_type.value == 0 or fp_shape_type.value == 1 or fp_shape_type.value == 2 or fp_shape_type.value == 3 or fp_shape_type.value == 6) and fp_shape_x1.value >= -20000 and fp_shape_x1.value <= 20000 and fp_shape_y1.value >= -20000 and fp_shape_y1.value <= 20000 and fp_shape_x2.value >= -20000 and fp_shape_x2.value <= 20000 and fp_shape_y2.value >= -20000 and fp_shape_y2.value <= 20000 and fp_shape_style_flag <= 1 and fp_shape_v37_width.value >= -10000 and fp_shape_v37_width.value <= 5000000 and fp_shape_v37_layer.value >= -1 and fp_shape_v37_layer.value <= 100'
      is_fp_font_block_start:
        value: '_root.ver >= 46 and _io.size - _io.pos >= 58 and b0 == 0x00 and b1 == 0x06 and b2 == 0x00 and b3 == 0x54 and b4 == 0x00 and b5 == 0x61 and b6 == 0x00 and b7 == 0x68 and b8 == 0x00 and b9 == 0x6f and b10 == 0x00 and b11 == 0x6d and b12 == 0x00 and b13 == 0x61 and fp_font_meta_flag <= 1 and fp_font_size.value >= 1 and fp_font_size.value <= 100000 and fp_font_height.value >= -10000000 and fp_font_height.value <= 10000000 and fp_font_width.value >= -10000000 and fp_font_width.value <= 10000000 and fp_font_line_width.value >= -10000 and fp_font_line_width.value <= 5000000 and fp_font_layer.value >= -1 and fp_font_layer.value <= 100 and fp_font_x1.value >= -50000000 and fp_font_x1.value <= 50000000 and fp_font_y1.value >= -50000000 and fp_font_y1.value <= 50000000 and fp_font_x2.value >= -50000000 and fp_font_x2.value <= 50000000 and fp_font_y2.value >= -50000000 and fp_font_y2.value <= 50000000 and (fp_font_shape_type.value == 0 or fp_font_shape_type.value == 1 or fp_font_shape_type.value == 2 or fp_font_shape_type.value == 3 or fp_font_shape_type.value == 5 or fp_font_shape_type.value == 6 or fp_font_shape_type.value == 7 or fp_font_shape_type.value == 700)'
      is_fp_chain_shape_start:
        value: '_root.ver >= 46 and _io.size - _io.pos >= 76 and fp_chain_pad_a == 0 and fp_chain_pad_b == 0 and fp_chain_font_len == 6 and fp_chain_font_t == 0x0054 and fp_chain_font_a == 0x0061 and fp_chain_font_h == 0x0068 and fp_chain_font_o == 0x006f and fp_chain_font_m == 0x006d and fp_chain_font_a2 == 0x0061 and fp_chain_width.value >= -10000 and fp_chain_width.value <= 5000000 and (fp_chain_shape_type.value == 2 or fp_chain_shape_type.value == 3) and fp_chain_x1.value >= -50000000 and fp_chain_x1.value <= 50000000 and fp_chain_y1.value >= -50000000 and fp_chain_y1.value <= 50000000 and fp_chain_x2.value >= -50000000 and fp_chain_x2.value <= 50000000 and fp_chain_y2.value >= -50000000 and fp_chain_y2.value <= 50000000'
      is_copper_pour_font_preamble_start:
        value: '_root.ver > 37 ? (copper_font_len_utf16 >= 1 and copper_font_len_utf16 <= 64 and copper_font_size_utf16.value >= 5 and copper_font_size_utf16.value <= 30 and copper_font_bold_utf16 <= 1 and copper_font_height_utf16.value > 0 and copper_font_height_utf16.value < 10000000 and copper_font_width_utf16.value > 0 and copper_font_width_utf16.value < 10000000 and copper_font_tail_utf16.value == -20000 and copper_font_separator_utf16.value == 0) : (copper_font_len_ascii.value >= 1 and copper_font_len_ascii.value <= 64 and copper_font_size_ascii.value >= 5 and copper_font_size_ascii.value <= 30 and copper_font_bold_ascii <= 1 and copper_font_height_ascii.value > 0 and copper_font_height_ascii.value < 10000000 and copper_font_width_ascii.value > 0 and copper_font_width_ascii.value < 10000000 and copper_font_tail_ascii.value == -20000 and copper_font_separator_ascii.value == 0)'
      is_copper_pour_start:
        value: '_io.size - _io.pos >= 30 and copper_pour_fill_mode <= 2 and copper_pour_connection_mode <= 2 and copper_pour_min_width.value > 0 and copper_pour_min_width.value <= 10000000 and copper_pour_clearance.value >= 0 and copper_pour_clearance.value <= 10000000 and copper_pour_min_area.value >= 0 and copper_pour_min_area.value <= 100000000 and copper_pour_separator.value <= 0 and copper_pour_layer.value >= 0 and copper_pour_layer.value <= 100 and copper_pour_vertex_count.value >= 3 and copper_pour_vertex_count.value <= 50000'
      is_copper_pour_style_start:
        value: '_io.size - _io.pos >= 14 and copper_style_lead.value == 0 and copper_style_spoke_mode.value >= 0 and copper_style_spoke_mode.value <= 4 and copper_style_line_spacing.value > 0 and copper_style_line_spacing.value <= 10000000 and copper_style_spoke_width.value > 0 and copper_style_spoke_width.value <= 10000000'
      is_copper_pour_cached_fill_start:
        value: '_io.size - _io.pos >= 23 and copper_cached_field0.value >= 0 and copper_cached_field0.value <= 100000 and copper_cached_field1.value >= -100000000 and copper_cached_field1.value <= 100000000 and copper_cached_field2.value >= -100000000 and copper_cached_field2.value <= 100000000 and copper_cached_field3.value >= -100000000 and copper_cached_field3.value <= 100000000 and copper_cached_field4.value >= -100000000 and copper_cached_field4.value <= 100000000 and copper_cached_field5.value >= -100000000 and copper_cached_field5.value <= 100000000'
      is_copper_pour_trailer_start:
        value: '_io.size - _io.pos >= 28 and copper_trailer_regions_counted.value >= 0 and copper_trailer_regions_counted.value <= 100000 and copper_trailer_zero.value == 0 and copper_trailer_board_clearance.value >= 0 and copper_trailer_board_clearance.value <= 10000000 and copper_trailer_island_region <= 1 and copper_trailer_island_internal <= 1 and copper_trailer_island_connection <= 1 and copper_trailer_zone_id.value >= 0 and copper_trailer_zone_id.value <= 100000 and copper_trailer_via_direct <= 1 and copper_trailer_smd_separate <= 1 and copper_trailer_smd_spoke_mode.value >= 0 and copper_trailer_smd_spoke_mode.value <= 4 and copper_trailer_smd_spoke_width.value > 0 and copper_trailer_smd_spoke_width.value <= 10000000 and copper_trailer_ratline_mode <= 2 and copper_trailer_regions_done <= 1'
      is_track_chain_start:
        value: '_io.size - _io.pos >= 12 and b0 == 0x00 and b1 == 0x00 and b2 == 0x00 and b3 == 0x0f and b4 == 0x42 and b5 == 0x3f and track_node_count_probe.value >= 1 and track_node_count_probe.value <= 10000'
      is_text_section_start:
        value: '_io.size - _io.pos >= 14 and b0 == 0x0f and b1 == 0x42 and b2 == 0x40 and b3 == 0x0f and b4 == 0x42 and b5 == 0x40 and b6 == 0x0f and b7 == 0x42 and b8 == 0x40 and text_count_probe.value >= 0 and text_count_probe.value <= 10000 and b12 == 0x01 and b13 == 0x00'

  component_record_anchor:
    doc: |
      Component boundary marker plus the following one-byte separator and forward
      component header.  FieldWalkComponentBoundaries() validates a boundary core
      with a header string at the file's global boundary delta; observed accepted
      records use delta 14, represented here as the 13-byte core plus
      boundary_suffix byte before `component`.
    params:
      - id: is_alt
        type: bool
    seq:
      - id: std_boundary
        type: component_boundary
        if: not is_alt
      - id: alt_boundary
        type: component_boundary_alt
        if: is_alt
      - id: boundary_suffix
        type: u1
        doc: Byte between the 13-byte boundary core and component library_path.
      - id: header
        type: component

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
      stored:
        value: byte0 * 65536 + byte1 * 256 + byte2
      value:
        value: stored - 1000000

  int4:
    doc: 4-byte unsigned big-endian integer biased by 1,000,000,000.
    seq:
      - id: stored
        type: u4
    instances:
      value:
        value: stored - 1000000000

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
      - id: legacy_magic_tail_a
        type: u1
        doc: First byte of the DTBOARDx.yy legacy post-font tail.
        if: _root.magic_len == 11
      - id: legacy_magic_tail_b
        type: u1
        if: _root.magic_len == 11
      - id: legacy_magic_tail_c
        type: u1
        if: _root.magic_len == 11
      - id: legacy_magic_tail_d
        type: u1
        if: _root.magic_len == 11
      - id: legacy_magic_tail_e
        type: u1
        if: _root.magic_len == 11
      - id: legacy_magic_tail_field_a
        type: int3
        doc: Observed 0 in DTBOARDx.yy legacy files.
        if: _root.magic_len == 11
      - id: legacy_magic_tail_field_b
        type: int3
        doc: Observed 0 in DTBOARDx.yy legacy files.
        if: _root.magic_len == 11
      - id: legacy_magic_tail_flag
        type: u1
        doc: Final legacy post-font tail byte before the rule-name count.
        if: _root.magic_len == 11
      - id: v37_padding
        contents: [0x00, 0x00, 0x00, 0x00, 0x00]
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
      - id: modern_post_font_flags
        type: u1
        repeat: expr
        repeat-expr: 7
        doc: |
          Seven post-font flag bytes read before the pattern/style group selector.
          Sampled modern files use combinations such as 00 00 00 00 00 00 00,
          00 00 00 01 00 00 00, and 01 01 00 00 01 00 00.  Files with the compact
          implicit-style variant use only these seven bytes and then
          `implicit_pattern_style_group`.
        if: _root.ver > 37
      - id: modern_post_font_tail
        type: u1
        repeat: expr
        repeat-expr: 3
        doc: |
          Standard v38+ tail bytes after modern_post_font_flags.  Observed as
          00 00 00 in non-implicit files; omitted by the compact implicit-style
          variant selected by the C++ parser when a UTF-16 group name begins here.
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
          - id: transition_pad
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
      mirrored exactly.  The placement rotation is NOT in this forward-read
      record: pads and footprint graphics are stored canonical (unrotated).  A
      90-degree-snapped quarter-turn count lives in a small metadata block that
      precedes the boundary marker, anchored by the component Id; it is recovered
      by scanning backwards for the zero-run `int3(0)*4 int4(0)*4` that trails the
      Id and reading the biased int3 six bytes before the Id (FindComponentRotation).
      The EXACT placement angle (biased int4 of radians * 1e4, allowing diagonal
      placements the 90-snap cannot express) lives in a separate placement section
      (ApplyPlacementAngles).  Each placement entry opens with a header that carries
      the angle as the biased int4 at header-4; there are two header kinds, both
      followed by three small int3 fields (high two bytes 0x0F42):
        FULL    -- byte(1) byte(1), the first placement of a pattern.
        COMPACT -- byte(0) byte(0) then nine 0x00 bytes + 0x0F42, a reused pattern.
      The entry ends with its refdes string; mapping each refdes to the nearest
      preceding header keys the angle to its component (validated 668/668 on
      the reference board; the unmapped objects are exactly the rotation-agnostic
      via/pad/fiducial records that carry no refdes).  Standalone-via component
      records can truncate after `pattern_name`; the importer keeps those partial
      records so the pad data can classify them as standalone vias.
    seq:
      - id: library_path
        type: dt_string
      - id: field_a
        type: int3
        doc: Raw header int3 field A.
      - id: field_b
        type: int3
        doc: Stored header int3 field B; parsed but not used by the importer.
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
      - id: filler_flag
        type: u1
        doc: Filler byte at post-dim +7; observed as a structural separator before polygon_vertex_count.
      - id: polygon_vertex_count
        type: int3
        doc: |
          Polygon vertex count at post-dim +8.  Only meaningful when
          shape_code == 3; for the fixed block it is part of the fixed
          remainder.
      - id: polygon_vertices
        type: pad_polygon_vertex
        repeat: expr
        repeat-expr: 'shape_code.value == 3 ? polygon_vertex_count.value : 0'
        doc: |
          Inline polygon vertices relative to pad center, present only when
          shape_code == 3 (PAD_POST_DIM_HEADER = 11 bytes precede this run).
      - id: fixed_tail
        type: pad_post_dim_tail
        if: shape_code.value == 3
        doc: |
          25-byte tail (PAD_POST_DIM_TAIL) after the polygon vertices.  The last
          byte is the pad orientation class (orientClass).
      - id: fixed_block_tail
        type: pad_post_dim_tail
        if: shape_code.value != 3
        doc: |
          Remainder of the 36-byte fixed block (11-byte header already consumed
          above).  The last byte is the pad orientation class (orientClass).

  pad_post_dim_tail:
    doc: |
      Tail of the pad post-dimension block.  The first 24 bytes are stored
      scalar/flag slots preserved by DipTrace before the orientation class byte;
      byte 24 is consumed as orientClass by the importer.
    seq:
      - id: tail_field_a
        type: int3
      - id: tail_flag_a
        type: u1
      - id: tail_field_b
        type: int3
      - id: tail_flag_b
        type: u1
      - id: tail_field_c
        type: int4
      - id: tail_field_d
        type: int4
      - id: tail_field_e
        type: int4
      - id: tail_field_f
        type: int3
      - id: orientation_class
        type: u1

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
        contents: [0x00, 0x00]
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
      - id: style
        type: fp_shape_style_v45
        doc: Bytes +27..+52, fixed style/paint payload before width.
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
      - id: style
        type: fp_shape_style_v37
        doc: Bytes +27..+54, legacy fixed style/paint payload before width.
      - id: width
        type: int4
        doc: Line width at record +55.
      - id: layer
        type: int3
        doc: DipTrace layer index at record +59.

  fp_shape_style_v45:
    doc: |
      v38..v45 fixed footprint-shape style payload at record +27..+52.  Project4
      v37 uses the wider fp_shape_style_v37 variant; v38 example_routed and v45
      Z80 Board records show this 26-byte payload as:
      byte(0), int3(0), five zero bytes, int3(0), int4(0), int4(0),
      int3(0), int3(0).  Width and layer follow immediately.
    seq:
      - id: style_flag_a
        type: u1
      - id: style_field_a
        type: int3
      - id: zero_run
        contents: [0x00, 0x00, 0x00, 0x00, 0x00]
        doc: Five structural zero bytes in observed v38/v45 records.
      - id: style_field_b
        type: int3
      - id: style_field_c
        type: int4
      - id: style_field_d
        type: int4
      - id: style_field_e
        type: int3
      - id: style_field_f
        type: int3

  fp_shape_style_v37:
    doc: |
      v37 legacy fixed footprint-shape style payload at record +27..+54.  Verified
      in qa/data/pcbnew/plugins/diptrace/project4.dip: byte(0), int3(0),
      int3(0), int3(0), byte(0), int3(0), int4(0), int4(0), int3(0), int3(0).
      Width and layer follow immediately at +55/+59.
    seq:
      - id: style_flag_a
        type: u1
      - id: style_field_a
        type: int3
      - id: style_field_b
        type: int3
      - id: style_field_c
        type: int3
      - id: style_flag_b
        type: u1
      - id: style_field_d
        type: int3
      - id: style_field_e
        type: int4
      - id: style_field_f
        type: int4
      - id: style_field_g
        type: int3
      - id: style_field_h
        type: int3

  fp_font_block:
    doc: |
      v46+ per-layer font-block shape (FindShapesInFontBlocks).  The first block
      is field-located at padRegionEnd + 165 + 2*u16(padRegionEnd + 163); each
      block is self-sized as 72 + 8*point_count + 2*label_chars (point_count =
      smallest leading slot in 0..3 whose int3 holds a valid shape-type code, one
      of 0, 1, 2, 3, 5, 6, 7, 700), so the blocks form a deterministic chain.  The run
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
      - id: meta_flag
        type: u1
        doc: First byte of the 25-byte metadata header.
      - id: font_size
        type: int3
        doc: Font size at meta +1.
      - id: font_height
        type: int4
        doc: Font height at meta +4.
      - id: font_width
        type: int4
        doc: Font width at meta +8.
      - id: meta_field_a
        type: int3
        doc: Metadata field at meta +12.
      - id: meta_field_b
        type: int3
        doc: Metadata field at meta +15.
      - id: line_width
        type: int4
        doc: Line width at meta +18.
      - id: meta_field_c
        type: int3
        doc: |
          Metadata field at meta +22 (end of the 25-byte header); constant, NOT the
          graphic layer. The footprint-graphic layer is the int3 stored 5 bytes ahead
          of this block's "Tahoma" pattern (0 Top Silk, 1 Top Assembly, 2 Top Mask,
          3 Top Paste, 16 Top Courtyard, 18 Top Outline).
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
          1 and 5 = line, 3 = circle, and both 2 and 6 (DT_SHAPE_ARC) = arc; 700 =
          filled obround marker (the diode cathode / pin-1 dot, rendered as a filled
          circle); 7 is accepted as a block-sizing code but emits no shape.  For arcs,
          mid_x/mid_y follow at body +35/+39.
      - id: arc_body
        type: fp_font_arc_body
        if: shape_type.value == 2 or shape_type.value == 6
        doc: |
          Bytes body +19..+42 for arc records (shape_type 2 or 6); the arc midpoint
          (mid_x at body +35, mid_y at body +39) lives inside this run (the importer
          requires body +43 <= next boundary).  Absent for non-arc shapes.

  fp_font_arc_body:
    doc: Arc-only body extension in fp_font_block; last two int4 fields are midpoint X/Y.
    seq:
      - id: arc_field_a
        type: int4
      - id: arc_field_b
        type: int4
      - id: arc_field_c
        type: int4
      - id: arc_field_d
        type: int4
      - id: mid_x
        type: int4
        doc: Arc midpoint X at body +35.
      - id: mid_y
        type: int4
        doc: Arc midpoint Y at body +39.

  fp_chain_shape:
    doc: |
      v46+ chained fixed-size shape record (FindShapesInChainedBlocks, parser
      ~2308-2398; FP_CHAIN_SHAPE_RECORD_SIZE = 76).  The block starts at
      padRegionEnd + 72 (FP_CHAIN_SHAPE_DATA_OFFSET) with the count at
      padRegionEnd + 69.  Records 0 and N-1 are framing entries; only the named
      fields are decoded.  For footprints with NPTH mounting holes, the final
      framing entry can carry a mount_hole_block overlay at byte +44.
    seq:
      - id: frame
        type: fp_chain_shape_frame
        doc: Bytes +0..+36, chained-record framing/style payload before width.
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
        type: fp_chain_shape_tail
        doc: Bytes +68..+75, chained-record trailer.

  fp_chain_shape_frame:
    doc: |
      Chained fixed-size footprint-shape framing/style payload at record +0..+36.
      This is a Tahoma font/style header: int3 field, two-byte pad, UTF-16BE
      "Tahoma", font flag/size/dimensions, and two int3 fields.  The importer does
      not consume these fields when emitting KiCad graphics, but the 37-byte payload
      is part of every 76-byte record and precedes the width at +37.
    seq:
      - id: frame_field_a
        type: int3
      - id: pad
        contents: [0x00, 0x00]
        doc: Observed 00 00 before the UTF-16BE font string.
      - id: font_pattern
        contents: [0x00, 0x06, 0x00, 0x54, 0x00, 0x61, 0x00, 0x68, 0x00, 0x6f, 0x00, 0x6d, 0x00, 0x61]
        doc: UTF-16BE length-prefixed "Tahoma" bytes (00 06 + 12 text bytes).
      - id: font_flag
        type: u1
      - id: font_size
        type: int3
      - id: font_height
        type: int4
      - id: font_width
        type: int4
      - id: frame_field_b
        type: int3
      - id: frame_field_c
        type: int3

  fp_chain_shape_tail:
    doc: |
      Chained fixed-size footprint-shape trailer at record +68..+75.  Real chain
      records store x3/y3 for arcs at +60/+64; this 8-byte trailer closes the
      fixed 76-byte record.
    seq:
      - id: tail_field_a
        type: int3
      - id: tail_field_b
        type: int4
      - id: tail_flag
        type: u1

  fp_chain_shape_mount_hole_overlay:
    doc: |
      Overlay for final v46+ chained-shape framing entries that carry footprint
      mounting holes.  The mount_hole_block starts at byte +44 of the 76-byte
      final framing record, sharing bytes with the stored framing
      payload.
    seq:
      - id: prefix
        type: fp_chain_shape_overlay_prefix
      - id: mount_holes
        type: mount_hole_block

  fp_chain_shape_overlay_prefix:
    doc: First 44 bytes of a chained-shape record before the mount-hole overlay at +44.
    seq:
      - id: frame
        type: fp_chain_shape_frame
      - id: width
        type: int4
      - id: shape_type
        type: int3

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
        contents: [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]
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
          expr: _.value >= 1 and _.value <= 10000
      - id: nodes
        type: track_node
        repeat: expr
        repeat-expr: node_count.value

  track_node:
    doc: |
      Fixed 41-byte routing point (TRACK_NODE_SIZE = 41; parser ~2981-3051).
      XML Point@Id is stored at +11 as an int3.  The +23..+26 block is int3(0)
      plus a one-byte auxiliary flag in the sampled corpus; +34 is int3(0).
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
      - id: point_id
        type: int3
        doc: XML Point@Id at +11.
      - id: width
        type: int4
        doc: Track width at +14.
      - id: via_outer_diameter
        type: int4
        doc: Via outer diameter at +18.
      - id: route_flag
        type: u1
        doc: Stored routing-point flag at +22; observed 0 or 1 in sampled files.
      - id: aux_zero
        type: int3
        doc: Auxiliary int3 at +23; observed 0 in sampled files.
      - id: aux_flag
        type: u1
        doc: Auxiliary flag byte at +26; observed 0 or 1 in sampled files.
      - id: via_style_index
        type: int3
        doc: |
          Via style index at +27 (-1 = no via).  This, not route_flag, is the
          authoritative via indicator.
      - id: via_drill_diameter
        type: int4
        doc: Via drill diameter at +30.
      - id: via_tail_zero
        type: int3
        doc: Post-via-diameter int3 at +34; observed 0 in sampled files.
      - id: route_mode
        type: int3
        doc: Raw routing-point mode at +37 (observed values 0, 1, 3, 5).
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
        contents: [0x01, 0x00]
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
          Filled-region count at +0 (not the DipTrace CopperPour priority).
      - id: fill_mode
        type: u1
        doc: Zone flag byte at +3 (validated <= 2).
      - id: header_flag_2
        type: u1
        doc: Zone header flag byte at +4; stored separately from fill_mode and connection_mode.
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
        type: copper_pour_fill_segment
        repeat: expr
        repeat-expr: fill_segment_count.value
        doc: Cached fill segments (19 bytes each); stored fill cache, not emitted as KiCad items.
      - id: fill_poly_count
        type: int3
      - id: fill_polys
        type: copper_pour_fill_poly
        repeat: expr
        repeat-expr: fill_poly_count.value
        doc: Cached fill polygons; stored fill cache, not emitted as KiCad items.
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

  copper_pour_fill_segment:
    doc: |
      One cached filled-copper segment (19 bytes): endpoint coordinates plus a small
      int3 classification field.  The importer skips these stored fill-cache records
      and lets KiCad refill the zone, but the bytes are deterministic; sampled Z80,
      logic-probe, the reference board, Banana_Pi, and BeagleBone files decode as plausible
      board coordinates.
    seq:
      - id: x1
        type: int4
      - id: y1
        type: int4
      - id: x2
        type: int4
      - id: y2
        type: int4
      - id: segment_class
        type: int3

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
        type: int3
        doc: Per-polygon trailer field.

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
      Structural 23-byte cached-fill record (CACHED_RECORD_LEN = 23; parser
      ~3411-3425), present in the inter-zone gap when the payload between the
      style block and the trailer is 23-byte aligned.  The importer preserves
      these as cached fill geometry and does not emit KiCad items from them.
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
