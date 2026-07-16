meta:
  id: pads_sch_binary
  title: PADS Logic binary schematic database
  license: GPL-3.0-or-later
  endian: le
  encoding: ASCII
  file-extension: sch
  imports:
    - /microsoft_cfb

seq:
  - id: header
    type:
      switch-on: version
      cases:
        0x000c: header_v12
        0x000d: header_v13
  - id: pool_directory
    type: pool_descriptor
    repeat: expr
    repeat-expr: 20
  - id: controller_24
    type: controller_24
  - id: sheets
    type: sheet_database
    repeat: expr
    repeat-expr: pool_directory[3].used_count
  - id: footer_aux
    type: footer_aux
  - id: footer
    type: database_footer

instances:
  version:
    pos: 2
    type: u2
    valid:
      any-of: [0x000c, 0x000d]

enums:
  symbol_graphic_kind:
    0: open
    1: closed
    2: circle
    4: filled_closed
  page_size_prefix:
    0x53: size
    0x57: wditbsize
  page_size_designator:
    0x41: a
    0x42: b
    0x43: c
    0x44: d
    0x45: e

types:
  header_v12:
    seq:
      - id: magic
        contents: [0x00, 0xfe]
      - id: version
        type: u2
        valid: 0x000c
      - id: format_flags
        type: u2
        valid: 1
      - id: application_header_word_06
        type: u2
        doc: Application-owned header value passed through the derived database reader.
      - id: reserved_zero_08
        contents: [0, 0, 0, 0, 0, 0, 0, 0]
      - id: database_identifier
        type: u4
      - id: reserved_zero_14
        contents: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]

  header_v13:
    seq:
      - id: magic
        contents: [0x00, 0xfe]
      - id: version
        type: u2
        valid: 0x000d
      - id: format_flags
        type: u2
        valid: 0
      - id: application_header_word_06
        type: u2
        doc: Application-owned header value passed through the derived database reader.
      - id: reserved_zero_08
        contents: [0, 0, 0, 0, 0, 0, 0, 0]
      - id: database_identifier
        type: u4
      - id: reserved_zero_14
        contents: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]

  pool_descriptor:
    seq:
      - id: controller_flags
        type: u4
      - id: allocated_bytes
        type: u4
      - id: used_count
        type: u4
      - id: used_bytes
        type: u4
      - id: object_id
        type: u4
      - id: controller_word_14
        type: u4
      - id: controller_word_18
        type: u4

  controller_24:
    seq:
      - id: ordinal
        type: u4
        valid: 0x24
      - id: global_string_pool
        type: global_string_controller
        size: _root.pool_directory[1].used_bytes
      - id: controller_payload_2
        size: _root.pool_directory[2].used_bytes
      - id: sheet_index
        type: sheet_index_controller(_root.pool_directory[3].used_count)
        size: _root.pool_directory[3].used_bytes
      - id: controller_payload_4
        size: _root.pool_directory[4].used_bytes
      - id: design_settings
        type: design_settings_record
        size: _root.pool_directory[5].used_bytes
      - id: controller_payload_6
        size: _root.pool_directory[6].used_bytes
      - id: controller_payload_7
        size: _root.pool_directory[7].used_bytes
      - id: controller_payload_8
        size: _root.pool_directory[8].used_bytes
      - id: controller_payload_9
        size: _root.pool_directory[9].used_bytes
      - id: controller_payload_10
        size: _root.pool_directory[10].used_bytes
      - id: controller_payload_11
        size: _root.pool_directory[11].used_bytes
      - id: controller_payload_12
        size: _root.pool_directory[12].used_bytes
      - id: controller_payload_13
        size: _root.pool_directory[13].used_bytes
      - id: controller_payload_14
        size: _root.pool_directory[14].used_bytes
      - id: controller_payload_15
        size: _root.pool_directory[15].used_bytes
      - id: controller_payload_16
        size: _root.pool_directory[16].used_bytes
      - id: controller_payload_17
        size: _root.pool_directory[17].used_bytes
      - id: controller_payload_18
        size: _root.pool_directory[18].used_bytes
      - id: controller_payload_19
        size: _root.pool_directory[19].used_bytes

  global_string_controller:
    seq:
      - id: records
        type: global_string_record(_io.pos)
        repeat: eos

  global_string_record:
    params:
      - id: slot_start
        type: u8
    seq:
      - id: raw_slot
        terminator: 0
        include: true
    instances:
      title_field:
        pos: slot_start
        type: title_field_slot
        size: raw_slot.size
        if: >
          raw_slot.size >= 6 and raw_slot[0] == 0x46 and raw_slot[1] == 0x69 and
          raw_slot[2] == 0x65 and raw_slot[3] == 0x6c and raw_slot[4] == 0x64 and
          raw_slot[5] == 0x0a

  title_field_slot:
    seq:
      - id: field_marker
        contents: [0x46, 0x69, 0x65, 0x6c, 0x64, 0x0a]
      - id: name
        type: str
        encoding: windows-1252
        terminator: 1
        consume: false
        eos-error: true
      - id: separator
        contents: [1]
      - id: value
        type: strz
        encoding: windows-1252

  sheet_index_controller:
    params:
      - id: num_records
        type: u4
    seq:
      - id: records
        type: sheet_index_record
        repeat: expr
        repeat-expr: num_records

  sheet_index_record:
    seq:
      - id: sheet_database_offset
        type: u4
      - id: sheet_database_bytes
        type: u4
      - id: sheet_id
        type: u2
        valid:
          min: 1
      - id: class_marker_1
        contents: [0xff, 0xff]
      - id: class_marker_2
        contents: [0xff, 0xff]
      - id: sheet_name_storage
        type: strz
        encoding: windows-1252
        size: 34

  design_settings_record:
    seq:
      - id: active_sheet
        type: u4
      - id: user_grid_mils
        type: u4
      - id: text_grid_mils
        type: u4
      - id: default_line_width_mils
        type: u4
      - id: default_bus_width_mils
        type: u4
      - id: bus_angle
        type: u4
      - id: preserved_design_settings_18
        size: 12
      - id: pin_name_height_mils
        type: u2
      - id: pin_name_width_mils
        type: u2
      - id: text_height_mils
        type: u2
      - id: text_width_mils
        type: u2
      - id: preserved_design_settings_2c
        size: 220
      - id: page_size_storage
        type: page_size_slot
        size: 11
      - id: preserved_design_settings_113
        size: 125

  sheet_text_controller:
    params:
      - id: num_records
        type: u4
    seq:
      - id: records
        type: sheet_text_record
        repeat: expr
        repeat-expr: num_records

  sheet_text_record:
    seq:
      - id: link_state
        type: u2
      - id: object_state
        type: u2
      - id: owner_state
        type: u4
      - id: string_heap_offset
        type: u4
      - id: x_biased_half_mil
        type: u2
      - id: y_biased_half_mil
        type: u2
      - id: angle_tenths_degree
        type: u2
      - id: justification
        type: u2
      - id: string_bytes_including_nul
        type: u2
      - id: height_mils
        type: u2
      - id: predecessor_ordinal
        type: u2
      - id: successor_ordinal
        type: u2
      - id: relationship_word_28
        type: u2
        doc: Preserved relationship/ordinal-like word; paired ASCII proves this is not a presentation flag.
      - id: line_width_mils
        type: u2

  page_size_slot:
    seq:
      - id: prefix
        type: u1
        enum: page_size_prefix
      - id: value
        type:
          switch-on: prefix
          cases:
            'page_size_prefix::size': page_size_short
            'page_size_prefix::wditbsize': page_size_legacy

  page_size_short:
    seq:
      - id: prefix_tail
        contents: [0x49, 0x5a, 0x45]
      - id: designator
        type: u1
        enum: page_size_designator
      - id: terminator
        contents: [0]
      - id: padding
        contents: [0, 0, 0, 0, 0]

  page_size_legacy:
    seq:
      - id: prefix_tail
        contents: [0x44, 0x49, 0x54, 0x42, 0x53, 0x49, 0x5a, 0x45]
      - id: designator
        type: u1
        enum: page_size_designator
      - id: terminator
        contents: [0]

  sheet_pool_descriptor:
    seq:
      - id: controller_word_00
        type: u4
      - id: controller_flags
        type: u4
      - id: allocated_bytes
        type: u4
      - id: used_count
        type: u4
      - id: used_bytes
        type: u4
      - id: object_id
        type: u4
      - id: controller_word_18
        type: u4

  sheet_database:
    seq:
      - id: header_bytes
        type: u4
        valid: 20
      - id: header_count
        type: u4
        valid: 5
      - id: header_stride
        type: u4
        valid: 20
      - id: object_id
        type: u4
      - id: reserved_zero
        type: u4
        valid: 0
      - id: pool_directory
        type: sheet_pool_descriptor
        repeat: expr
        repeat-expr: 24
      - id: text_and_presentation_records
        type: sheet_text_controller(pool_directory[0].used_count)
        size: pool_directory[0].used_bytes
      - id: indexed_string_heap
        size: pool_directory[1].used_bytes
      - id: controller_payload_3
        type:
          switch-on: _root.version
          cases:
            0x000c: preserved_v12_definition_controller
            0x000d: symbol_definition_controller(pool_directory[2].used_count)
        size: pool_directory[2].used_bytes
      - id: controller_payload_4
        type:
          switch-on: _root.version
          cases:
            0x000c: preserved_v12_definition_controller
            0x000d: symbol_piece_controller(pool_directory[3].used_count)
        size: pool_directory[3].used_bytes
      - id: controller_payload_5
        type:
          switch-on: _root.version
          cases:
            0x000c: preserved_v12_definition_controller
            0x000d: symbol_vertex_controller(pool_directory[4].used_count)
        size: pool_directory[4].used_bytes
      - id: controller_payload_6
        type:
          switch-on: _root.version
          cases:
            0x000c: preserved_v12_definition_controller
            0x000d: symbol_arc_controller(pool_directory[5].used_count)
        size: pool_directory[5].used_bytes
      - id: controller_payload_7
        type:
          switch-on: _root.version
          cases:
            0x000c: preserved_v12_definition_controller
            0x000d: used_decal_controller(pool_directory[6].used_count)
        size: pool_directory[6].used_bytes
      - id: controller_payload_8
        type:
          switch-on: _root.version
          cases:
            0x000c: preserved_v12_definition_controller
            0x000d: symbol_terminal_controller(pool_directory[7].used_count)
        size: pool_directory[7].used_bytes
      - id: controller_payload_9
        type:
          switch-on: _root.version
          cases:
            0x000c: preserved_v12_definition_controller
            0x000d: part_type_controller(pool_directory[8].used_count)
        size: pool_directory[8].used_bytes
      - id: controller_payload_10
        type:
          switch-on: _root.version
          cases:
            0x000c: preserved_v12_definition_controller
            0x000d: gate_controller(pool_directory[9].used_count)
        size: pool_directory[9].used_bytes
      - id: controller_payload_11
        type:
          switch-on: _root.version
          cases:
            0x000c: preserved_v12_definition_controller
            0x000d: part_pin_controller(pool_directory[10].used_count)
        size: pool_directory[10].used_bytes
      - id: controller_payload_12
        type:
          switch-on: _root.version
          cases:
            0x000c: preserved_v12_definition_controller
            0x000d: signal_pin_controller(pool_directory[11].used_count)
        size: pool_directory[11].used_bytes
      - id: controller_payload_13
        type: preserved_definition_controller
        size: pool_directory[12].used_bytes
      - id: controller_payload_14
        type:
          switch-on: _root.version
          cases:
            0x000c: preserved_v12_definition_controller
            0x000d: pin_name_heap
        size: pool_directory[13].used_bytes
      - id: controller_payload_15
        size: pool_directory[14].used_bytes
      - id: controller_payload_16
        size: pool_directory[15].used_bytes
      - id: controller_payload_17
        size: pool_directory[16].used_bytes
      - id: controller_payload_18
        size: pool_directory[17].used_bytes
      - id: controller_payload_19
        size: pool_directory[18].used_bytes
      - id: controller_payload_20
        size: pool_directory[19].used_bytes
      - id: controller_payload_21
        size: pool_directory[20].used_bytes
      - id: controller_payload_22
        size: pool_directory[21].used_bytes
      - id: controller_payload_23
        size: pool_directory[22].used_bytes

  preserved_v12_definition_controller:
    doc: Exact bounded v0x000C definition payload; semantics unsupported without paired ASCII evidence.
    seq:
      - id: preserved_payload
        size-eos: true

  preserved_definition_controller:
    doc: Exact bounded controller payload whose semantics are not yet proven.
    seq:
      - id: preserved_payload
        size-eos: true

  symbol_definition_controller:
    params:
      - id: num_records
        type: u4
    seq:
      - id: records
        type: symbol_definition_record
        repeat: expr
        repeat-expr: num_records

  symbol_definition_record:
    seq:
      - id: name
        type: strz
        size: 38
        encoding: windows-1252
      - id: preserved_name_class_byte_26
        type: u1
      - id: preserved_name_class_byte_27
        type: u1
      - id: preserved_name_class_byte_28
        type: u1
      - id: object_class
        type: u1
      - id: graphic_piece_count
        type: u2
      - id: preserved_definition_word_2c
        type: u4
      - id: terminal_prefix_index
        type: u4
      - id: vertex_prefix_index
        type: u4
      - id: preserved_definition_word_38
        type: u4
      - id: timestamp
        type: u4
      - id: embedded_text_count
        type: u2
      - id: preserved_definition_style_word_42
        type: s2
      - id: preserved_definition_style_word_44
        type: s2
      - id: preserved_definition_style_word_46
        type: s2
      - id: preserved_definition_style_word_48
        type: s2
      - id: preserved_definition_style_word_4a
        type: s2
      - id: preserved_definition_style_word_4c
        type: s2
      - id: preserved_definition_style_word_4e
        type: s2

  symbol_piece_controller:
    params:
      - id: num_records
        type: u4
    seq:
      - id: records
        type: symbol_piece_record
        repeat: expr
        repeat-expr: num_records

  symbol_piece_record:
    seq:
      - id: kind
        type: u1
        enum: symbol_graphic_kind
      - id: continuation_marker
        type: u1
      - id: vertex_count
        type: u2
      - id: stroke_width_mils
        type: u2

  symbol_vertex_controller:
    params:
      - id: num_records
        type: u4
    seq:
      - id: records
        type: symbol_vertex_record
        repeat: expr
        repeat-expr: num_records

  symbol_vertex_record:
    seq:
      - id: x_half_mil_divided_by_2
        type: s2
      - id: y_half_mil_divided_by_2
        type: s2
      - id: arc_marker
        type: s2

  symbol_arc_controller:
    params:
      - id: num_records
        type: u4
    seq:
      - id: records
        type: symbol_arc_record
        repeat: expr
        repeat-expr: num_records

  symbol_arc_record:
    seq:
      - id: sweep_angle_tenths_degree
        type: u2
      - id: direction_marker
        type: s2
      - id: preserved_arc_word_04
        type: s2
      - id: bounding_x1_half_mil_divided_by_2
        type: s2
      - id: bounding_y1_half_mil_divided_by_2
        type: s2
      - id: bounding_x2_half_mil_divided_by_2
        type: s2
      - id: bounding_y2_half_mil_divided_by_2
        type: s2

  used_decal_controller:
    params:
      - id: num_records
        type: u4
    seq:
      - id: records
        type: used_decal_record
        repeat: expr
        repeat-expr: num_records

  used_decal_record:
    seq:
      - id: name
        type: strz
        size: 40
        encoding: windows-1252
      - id: preserved_decal_flags
        type: u2
        doc: Exact preserved value; controlled fixtures do not prove individual bit semantics.
      - id: terminal_count
        type: u1
      - id: pin_origin_code
        type: u1
      - id: terminal_prefix_index
        type: u2
      - id: preserved_used_decal_word_2e
        type: u2
      - id: definition_index
        type: u4
      - id: definition_field_index
        type: u4
      - id: timestamp
        type: u4
      - id: reference_x_half_mil_divided_by_2
        type: s2
      - id: reference_y_half_mil_divided_by_2
        type: s2
      - id: reference_angle_tenths_degree
        type: u2
      - id: reference_justification
        type: u2
      - id: part_type_x_half_mil_divided_by_2
        type: s2
      - id: part_type_y_half_mil_divided_by_2
        type: s2
      - id: part_type_angle_tenths_degree
        type: u2
      - id: part_type_justification
        type: u2
      - id: bounding_x1_half_mil_divided_by_2
        type: s2
      - id: bounding_y1_half_mil_divided_by_2
        type: s2
      - id: bounding_x2_half_mil_divided_by_2
        type: s2
      - id: bounding_y2_half_mil_divided_by_2
        type: s2
      - id: preserved_decal_word_54
        type: u2
      - id: preserved_decal_word_56
        type: u2
      - id: reference_height_half_mil_divided_by_2
        type: u2
      - id: part_type_height_half_mil_divided_by_2
        type: u2
      - id: value_height_half_mil_divided_by_2
        type: u2
      - id: wildcard_height_half_mil_divided_by_2
        type: u2
      - id: reference_width_half_mil_divided_by_2
        type: u1
      - id: part_type_width_half_mil_divided_by_2
        type: u1
      - id: value_width_half_mil_divided_by_2
        type: u1
      - id: wildcard_width_half_mil_divided_by_2
        type: u1
      - id: definition_font_handles
        type: s2
        repeat: expr
        repeat-expr: 4

  symbol_terminal_controller:
    params:
      - id: num_records
        type: u4
    seq:
      - id: records
        type: symbol_terminal_record
        repeat: expr
        repeat-expr: num_records

  symbol_terminal_record:
    seq:
      - id: pin_decal_handle
        type: u2
      - id: x_half_mil_divided_by_2
        type: s2
      - id: y_half_mil_divided_by_2
        type: s2
      - id: pin_name_height_half_mil_divided_by_2
        type: u2
      - id: pin_name_width_half_mil_divided_by_2
        type: u2
      - id: pin_number_height_half_mil_divided_by_2
        type: u2
      - id: pin_number_width_half_mil_divided_by_2
        type: u2
      - id: name_offset_x_half_mil_divided_by_2
        type: s2
      - id: name_offset_y_half_mil_divided_by_2
        type: s2
      - id: number_offset_x_half_mil_divided_by_2
        type: s2
      - id: number_offset_y_half_mil_divided_by_2
        type: s2
      - id: side
        type: u2
      - id: visibility_flags
        type: u2

  part_type_controller:
    params:
      - id: num_records
        type: u4
    seq:
      - id: records
        type: part_type_record
        repeat: expr
        repeat-expr: num_records

  part_type_record:
    seq:
      - id: name
        type: strz
        size: 44
        encoding: windows-1252
      - id: gate_prefix_index
        type: u4
      - id: pin_prefix_index
        type: u4
      - id: preserved_part_type_word_34
        type: u4
      - id: preserved_part_type_word_38
        type: u4
      - id: preserved_part_type_word_3c
        type: u4
      - id: timestamp
        type: u4
      - id: gate_count
        type: u2
      - id: signal_pin_count
        type: u2
      - id: category
        type: strz
        size: 4
        encoding: windows-1252

  gate_controller:
    params:
      - id: num_records
        type: u4
    seq:
      - id: records
        type: gate_record
        repeat: expr
        repeat-expr: num_records

  gate_record:
    seq:
      - id: definition_handles
        type: u2
        repeat: expr
        repeat-expr: 4
      - id: pin_count
        type: u2
      - id: swap_group
        type: u2

  part_pin_controller:
    params:
      - id: num_records
        type: u4
    seq:
      - id: records
        type: part_pin_record
        repeat: expr
        repeat-expr: num_records

  part_pin_record:
    seq:
      - id: name_heap_offset
        type: u4
      - id: number
        type: strz
        size: 16
        encoding: windows-1252
      - id: swap_group
        type: u1
      - id: electrical_type
        type: u1
      - id: pin_flags
        type: u2

  signal_pin_controller:
    params:
      - id: num_records
        type: u4
    seq:
      - id: records
        type: signal_pin_record
        repeat: expr
        repeat-expr: num_records

  signal_pin_record:
    seq:
      - id: number
        type: strz
        size: 16
        encoding: windows-1252
      - id: signal_name
        type: strz
        size: 48
        encoding: windows-1252

  pin_name_heap:
    seq:
      - id: names
        type: strz
        encoding: windows-1252
        repeat: eos

  footer_aux:
    seq:
      - id: preview_count
        type: u4
      - id: previews
        type:
          switch-on: preview_count
          cases:
            0: no_cfb_previews
            _: first_cfb_preview(preview_count)

  no_cfb_previews:
    seq: []

  first_cfb_preview:
    params:
      - id: preview_count
        type: u4
    seq:
      - id: mfc_class_marker
        contents: [0xff, 0xff, 0x01, 0x00]
      - id: len_class_name
        type: u2
        valid: 17
      - id: class_name
        contents: [0x43, 0x50, 0x6f, 0x77, 0x65, 0x72, 0x50, 0x43,
                   0x42, 0x43, 0x6e, 0x74, 0x72, 0x49, 0x74, 0x65, 0x6d]
      - id: item_state
        size: 18
      - id: len_cfb
        type: u4
      - id: preview
        type: cfb_preview_chain(len_cfb, preview_count)

  cfb_preview_chain:
    params:
      - id: len_cfb
        type: u4
      - id: remaining_count
        type: u4
    seq:
      - id: cfb
        type: microsoft_cfb
        size: len_cfb
      - id: trailer
        type:
          switch-on: remaining_count
          cases:
            1: final_cfb_preview_trailer
            _: nonfinal_cfb_preview_trailer(remaining_count - 1)

  nonfinal_cfb_preview_trailer:
    params:
      - id: remaining_count
        type: u4
    seq:
      - id: class_id
        contents: [0x7b, 0x46, 0x34, 0x39, 0x39, 0x37, 0x44, 0x37,
                   0x30, 0x2d, 0x41, 0x46, 0x38, 0x41, 0x2d, 0x31,
                   0x31, 0x44, 0x30, 0x2d, 0x41, 0x33, 0x37, 0x33,
                   0x2d, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
                   0x30, 0x30, 0x30, 0x30, 0x30, 0x7d]
      - id: extent
        size: 16
      - id: rectangle
        size: 16
      - id: item_state
        size: 28
      - id: next_len_cfb
        type: u4
      - id: next_preview
        type: cfb_preview_chain(next_len_cfb, remaining_count)

  final_cfb_preview_trailer:
    seq:
      - id: class_id
        contents: [0x7b, 0x46, 0x34, 0x39, 0x39, 0x37, 0x44, 0x37,
                   0x30, 0x2d, 0x41, 0x46, 0x38, 0x41, 0x2d, 0x31,
                   0x31, 0x44, 0x30, 0x2d, 0x41, 0x33, 0x37, 0x33,
                   0x2d, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
                   0x30, 0x30, 0x30, 0x30, 0x30, 0x7d]
      - id: extent
        size: 16
      - id: rectangle
        size: 16
      - id: item_state
        size: 8

  database_footer:
    seq:
      - id: class_id
        contents: [0x7b, 0x46, 0x34, 0x39, 0x39, 0x37, 0x44, 0x37,
                   0x30, 0x2d, 0x41, 0x46, 0x38, 0x41, 0x2d, 0x31,
                   0x31, 0x44, 0x30, 0x2d, 0x41, 0x33, 0x37, 0x33,
                   0x2d, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
                   0x30, 0x30, 0x30, 0x30, 0x30, 0x7d]
      - id: back_pointer
        type: u4
        doc: Database-relative reference associated with the final class record.
