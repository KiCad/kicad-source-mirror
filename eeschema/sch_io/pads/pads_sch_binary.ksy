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
        type: global_string_record
        repeat: eos

  global_string_record:
    seq:
      - id: prefix
        type: u1
      - id: body
        type:
          switch-on: prefix
          cases:
            0x46: title_field_slot
            _: generic_global_string_tail

  title_field_slot:
    seq:
      - id: field_marker_tail
        contents: [0x69, 0x65, 0x6c, 0x64, 0x0a]
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

  generic_global_string_tail:
    seq:
      - id: tail
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
        size: pool_directory[2].used_bytes
      - id: controller_payload_4
        size: pool_directory[3].used_bytes
      - id: controller_payload_5
        size: pool_directory[4].used_bytes
      - id: controller_payload_6
        size: pool_directory[5].used_bytes
      - id: controller_payload_7
        size: pool_directory[6].used_bytes
      - id: controller_payload_8
        size: pool_directory[7].used_bytes
      - id: controller_payload_9
        size: pool_directory[8].used_bytes
      - id: controller_payload_10
        size: pool_directory[9].used_bytes
      - id: controller_payload_11
        size: pool_directory[10].used_bytes
      - id: controller_payload_12
        size: pool_directory[11].used_bytes
      - id: controller_payload_13
        size: pool_directory[12].used_bytes
      - id: controller_payload_14
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
