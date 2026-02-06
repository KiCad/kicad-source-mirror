# Can be viewed in: https://ide.kaitai.io/
#
# Reverse-engineered DipTrace PCB binary format (.dip).
# This spec follows the deterministic section order used by
# diptrace_pcb_parser.cpp.  Later component, text, net, route, and zone data
# are partly heuristic-scanned by the importer; reusable record types for those
# structures are included below, but the top-level stream keeps that tail raw.

meta:
  id: diptrace_pcb
  endian: be
  encoding: UTF-8

seq:
  - id: magic_len
    type: u1
    valid: 7
  - id: magic
    contents: [0x44, 0x54, 0x42, 0x4f, 0x41, 0x52, 0x44] # "DTBOARD"
  - id: version
    type: int3
  - id: field_0b
    type: int4
  - id: field_0f
    type: int3
  - id: field_12
    type: int3
  - id: legacy_schematic_placeholder
    type: int3
    if: version.value <= 37
  - id: schematic_path
    type: dt_string(version.value)
    if: version.value > 37
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
    type: layer(version.value)
    repeat: expr
    repeat-expr: layer_count.value
  - id: font_style
    type: font_style(version.value)
  - id: design_rules
    type: design_rules(version.value, font_style.rule_name_count.value)
  - id: component_and_post_sections
    size-eos: true

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
    params:
      - id: version
        type: s4
    seq:
      - id: len_ascii
        type: int3
        if: version <= 37
      - id: text_ascii
        type: str
        size: len_ascii.value
        encoding: ASCII
        if: version <= 37
      - id: len_utf16
        type: u2
        if: version > 37
      - id: text_utf16
        type: str
        size: len_utf16 * 2
        encoding: UTF-16BE
        if: version > 37

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
    params:
      - id: version
        type: s4
    seq:
      - id: flag
        type: u1
      - id: index
        type: int3
      - id: color
        type: rgb_color
      - id: name
        type: dt_string(version)
      - id: field_a
        type: int3
      - id: field_b
        type: int3
      - id: field_c
        type: int3
      - id: field_d
        type: int4
      - id: separator
        type: u1

  font_style:
    params:
      - id: version
        type: s4
    seq:
      - id: prefix_fields
        type: int3
        repeat: expr
        repeat-expr: 6
      - id: font_name
        type: dt_string(version)
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
      - id: v37_padding
        size: 5
        if: version <= 37
      - id: v37_extra_a
        type: int3
        if: version <= 37
      - id: v37_extra_b
        type: int3
        if: version <= 37
      - id: v37_extra_flag
        type: u1
        if: version <= 37
      - id: modern_padding
        size: 10
        if: version > 37
      - id: field_c
        type: int3
      - id: field_d
        type: int3
      - id: rule_name_count
        type: int3

  design_rules:
    params:
      - id: version
        type: s4
      - id: num_entries
        type: s4
    seq:
      - id: entries
        type: design_rule_entry(version)
        repeat: expr
        repeat-expr: num_entries
      - id: rule_set_count
        type: int3
    types:
      design_rule_entry:
        params:
          - id: version
            type: s4
        seq:
          - id: name
            type: dt_string(version)
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

      rule_set:
        params:
          - id: version
            type: s4
        seq:
          - id: name
            type: dt_string(version)
          - id: field_a
            type: int3
          - id: flags
            type: u1
            repeat: expr
            repeat-expr: 4
          - id: block_count
            type: int3
          - id: blocks
            type: rule_set_block
            repeat: expr
            repeat-expr: block_count.value

      rule_set_block:
        seq:
          - id: values
            type: int4
            repeat: expr
            repeat-expr: 26

  component_boundary:
    doc: int3(0), int3(-1/0), int3(-1/0), int4(0) component boundary.
    seq:
      - id: field_a
        contents: [0x0f, 0x42, 0x40]
      - id: field_b
        type: int3
      - id: field_c
        type: int3
      - id: field_d
        contents: [0x3b, 0x9a, 0xca, 0x00]

  net_sentinel:
    doc: int3(0), int3(-1), int3(-1), immediately before a net record.
    seq:
      - id: zero
        contents: [0x0f, 0x42, 0x40]
      - id: minus_one_a
        contents: [0x0f, 0x42, 0x3f]
      - id: minus_one_b
        contents: [0x0f, 0x42, 0x3f]

  net_record:
    params:
      - id: version
        type: s4
    seq:
      - id: sentinel
        type: net_sentinel
      - id: net_index
        type: int3
      - id: field_0
        type: int3
      - id: trace_width
        type: int4
      - id: width_2
        type: int4
      - id: name
        type: dt_string(version)

  net_routing_preamble:
    seq:
      - id: default_via_outer_diameter
        type: int4
      - id: default_via_drill_diameter
        type: int4
      - id: marker
        type: u1
      - id: zeros
        size: 7
      - id: separator
        type: int3
      - id: pad_ref_count
        type: int3
      - id: pad_refs
        type: pad_ref
        repeat: expr
        repeat-expr: pad_ref_count.value

  pad_ref:
    seq:
      - id: component_index
        type: int3
      - id: pad_index
        type: int3

  track_chain:
    seq:
      - id: chain_marker
        contents: [0x00, 0x00, 0x00, 0x0f, 0x42, 0x3f]
      - id: chain_index
        type: int3
      - id: node_count
        type: int3
      - id: nodes
        type: track_node
        repeat: expr
        repeat-expr: node_count.value

  track_node:
    doc: Fixed 41-byte routing point payload.
    seq:
      - id: x
        type: int4
      - id: y
        type: int4
      - id: layer
        type: int3
      - id: unknown_11
        size: 3
      - id: width
        type: int4
      - id: via_outer_diameter
        type: int4
      - id: route_flag
        type: u1
      - id: unknown_23
        size: 4
      - id: via_style_index
        type: int3
      - id: via_drill_diameter
        type: int4
      - id: unknown_34
        size: 3
      - id: route_mode
        type: int3
      - id: tail
        type: u1

  text_section:
    params:
      - id: version
        type: s4
    seq:
      - id: zeros
        contents: [0x0f, 0x42, 0x40, 0x0f, 0x42, 0x40, 0x0f, 0x42, 0x40]
      - id: count
        type: int3
      - id: flag_1
        type: u1
      - id: flag_2
        type: u1
      - id: records
        type: text_record(version)
        repeat: expr
        repeat-expr: count.value

  text_record:
    params:
      - id: version
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
        type: dt_string(version)
      - id: font_name
        type: dt_string(version)
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
