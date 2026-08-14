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
  graphic_line_style:
    0: dashed
    0xff: solid
  offpage_variant:
    0: variant_0
    1: variant_1
    2: variant_2
    3: variant_3
    4: variant_4
    5: variant_5
    0xfe: local
    0xff: bus_entry
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
        type: indexed_placement_attribute_heap
        size: _root.pool_directory[2].used_bytes
      - id: sheet_index
        type: sheet_index_controller(_root.pool_directory[3].used_count)
        size: _root.pool_directory[3].used_bytes
      - id: controller_payload_4
        type:
          switch-on: _root.version
          cases:
            0x000c: preserved_v12_connectivity_controller
            0x000d: shared_sheet_membership_controller(_root.pool_directory[4].used_count)
        size: _root.pool_directory[4].used_bytes
      - id: design_settings
        type: design_settings_record
        size: _root.pool_directory[5].used_bytes
      - id: controller_payload_6
        type: placement_group_controller(_root.pool_directory[6].used_count)
        size: _root.pool_directory[6].used_bytes
      - id: controller_payload_7
        type: placement_attribute_offset_controller(_root.pool_directory[7].used_count)
        size: _root.pool_directory[7].used_bytes
      - id: controller_payload_8
        type:
          switch-on: _root.version
          cases:
            0x000c: preserved_v12_connectivity_controller
            0x000d: global_net_controller(_root.pool_directory[8].used_count)
        size: _root.pool_directory[8].used_bytes
      - id: preserved_controller_9_payload
        size: _root.pool_directory[9].used_bytes
        doc: Exact pool-directory-bounded bytes; audited private files contain records but no paired diff proves their semantics. Schema disposition is UNSUPPORTED and the bytes remain ledger-owned.
      - id: preserved_controller_10_payload
        size: _root.pool_directory[10].used_bytes
        doc: Exact pool-directory-bounded bytes; audited private files contain records but no paired diff proves their semantics. Schema disposition is UNSUPPORTED and the bytes remain ledger-owned.
      - id: preserved_controller_11_payload
        size: _root.pool_directory[11].used_bytes
        doc: Exact pool-directory-bounded bytes; audited public and private files contain records but no paired diff proves their semantics. Schema disposition is UNSUPPORTED and the bytes remain ledger-owned.
      - id: preserved_controller_12_payload
        size: _root.pool_directory[12].used_bytes
        doc: Exact pool-directory-bounded bytes; every audited file contains this payload but no paired diff proves its semantics. Schema disposition is UNSUPPORTED and the bytes remain ledger-owned.
      - id: preserved_controller_13_payload
        size: _root.pool_directory[13].used_bytes
        doc: Exact pool-directory-bounded bytes; every audited file contains this payload but no paired diff proves its semantics. Schema disposition is UNSUPPORTED and the bytes remain ledger-owned.
      - id: preserved_controller_14_payload
        size: _root.pool_directory[14].used_bytes
        doc: Exact pool-directory-bounded bytes; every audited file contains this payload but no paired diff proves its semantics. Schema disposition is UNSUPPORTED and the bytes remain ledger-owned.
      - id: preserved_controller_15_payload
        size: _root.pool_directory[15].used_bytes
        doc: Exact pool-directory-bounded bytes; all 28 audited files leave this controller empty. Schema disposition is PRESERVED if encountered.
      - id: preserved_controller_16_payload
        size: _root.pool_directory[16].used_bytes
        doc: Exact pool-directory-bounded bytes; all 28 audited files leave this controller empty. Schema disposition is PRESERVED if encountered.
      - id: preserved_controller_17_payload
        size: _root.pool_directory[17].used_bytes
        doc: Exact pool-directory-bounded bytes; all 28 audited files leave this controller empty. Schema disposition is PRESERVED if encountered.
      - id: preserved_controller_18_payload
        size: _root.pool_directory[18].used_bytes
        doc: Exact pool-directory-bounded bytes; audited public and private files contain records but no paired diff proves their semantics. Schema disposition is UNSUPPORTED and the bytes remain ledger-owned.
      - id: controller_payload_19
        type: placement_font_controller(_root.pool_directory[19].used_count)
        size: _root.pool_directory[19].used_bytes

  global_string_controller:
    seq:
      - id: records
        type: global_string_record(_io.pos)
        repeat: eos

  shared_sheet_membership_controller:
    params:
      - id: num_sheet_indices
        type: u4
    seq:
      - id: sheet_indices
        type: u2
        repeat: expr
        repeat-expr: num_sheet_indices

  global_net_controller:
    params:
      - id: num_records
        type: u4
    seq:
      - id: records
        type: global_net_record
        repeat: expr
        repeat-expr: num_records

  global_net_record:
    seq:
      - id: preserved_net_identity
        type: u4
        doc: Exact v0x000D value; generated net fixtures use zero and minimal_v13 is nonzero, so semantics are unproved and importer disposition is PRESERVED.
      - id: sheet_membership_start
        type: u4
      - id: alias_string_offset
        type: u4
      - id: alias_member_ordinal
        type: u4
      - id: sheet_membership_count
        type: u2
      - id: alias_member_count
        type: u2
      - id: reserved_zero_14
        contents: [0, 0]
      - id: net_kind_flags
        type: u2
      - id: name
        type: strz
        size: 56
        encoding: windows-1252
      - id: unset_link_50
        contents: [0xff, 0xff, 0xff, 0xff]
      - id: preserved_net_relationship_54
        type: u4
        doc: Exact v0x000D tail link; private files prove nonzero values without a paired semantic change, so importer disposition is PRESERVED.

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
        doc: Exact fixed-offset design-settings bytes; controlled diffs do not associate them with an exported setting, so importer disposition is PRESERVED.
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
        doc: Exact fixed-offset design-settings bytes; controlled page and width diffs leave their semantics unproved, so importer disposition is PRESERVED.
      - id: page_size_storage
        type: page_size_slot
        size: 11
      - id: preserved_design_settings_113
        size: 125
        doc: Exact fixed-offset design-settings bytes; no paired ASCII field owns them, so importer disposition is PRESERVED.

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
        doc: Free-text records use this as a signed outer controller-19 font handle. Controlled Logic 9.0 save/re-export proves handles for regular, bold, italic, and bold-italic fonts; page-owned text records use the word as relationship state instead.
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
        doc: The low byte is the Logic 9.0 justification code; paired vendor fields prove the high byte carries unrelated packed data. For free text, controlled crosshair fixtures prove low-nibble horizontal classes 0/2/8 left, 4/6/10/12/14 center, and the remaining codes right. All 16 codes use the same baseline anchor. Other controller-1 record classes use their own justification interpretation.
      - id: string_bytes_including_nul
        type: u2
      - id: height_half_mil
        type: u2
      - id: predecessor_ordinal
        type: u2
        doc: Previous record in an embedded page-text ownership chain. Paired DRW5982 follows this link backward from controller-3's terminal record.
      - id: successor_ordinal
        type: u2
        doc: Preserved record relationship distinct from the embedded page-text chain; free text uses self ordinals.
      - id: relationship_word_28
        type: u2
        doc: Preserved relationship/ordinal-like word; paired ASCII proves this is not a presentation flag.
      - id: width_factor
        type: u1
        doc: Logic 9.0 text width factor. This is independent from the following display byte.
      - id: display_flags
        type: u1
        doc: Controlled Logic 9.0 free text proves bit 0 marks hidden text; remaining bits are not present in the observed corpus.

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
        doc: Exact controller-2 NUL-terminated string bytes; typed controller offsets select individual strings and raw bytes remain preserved.
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
        type:
          switch-on: _root.version
          cases:
            0x000c: preserved_v12_placement_controller
            0x000d: placement_controller(pool_directory[14].used_count)
        size: pool_directory[14].used_bytes
      - id: controller_payload_16
        type:
          switch-on: _root.version
          cases:
            0x000c: preserved_v12_placement_controller
            0x000d: placed_pin_controller(pool_directory[15].used_count)
        size: pool_directory[15].used_bytes
      - id: controller_payload_17
        type:
          switch-on: _root.version
          cases:
            0x000c: preserved_v12_placement_controller
            0x000d: placement_field_controller(pool_directory[16].used_count)
        size: pool_directory[16].used_bytes
      - id: controller_payload_18
        type:
          switch-on: _root.version
          cases:
            0x000c: preserved_v12_connectivity_controller
            0x000d: bus_controller(pool_directory[17].used_count)
        size: pool_directory[17].used_bytes
      - id: controller_payload_19
        type:
          switch-on: _root.version
          cases:
            0x000c: preserved_v12_connectivity_controller
            0x000d: junction_controller(pool_directory[18].used_count)
        size: pool_directory[18].used_bytes
      - id: controller_payload_20
        type:
          switch-on: _root.version
          cases:
            0x000c: preserved_v12_connectivity_controller
            0x000d: offpage_controller(pool_directory[19].used_count)
        size: pool_directory[19].used_bytes
      - id: controller_payload_21
        type:
          switch-on: _root.version
          cases:
            0x000c: preserved_v12_connectivity_controller
            0x000d: connection_controller(pool_directory[20].used_count)
        size: pool_directory[20].used_bytes
      - id: controller_payload_22
        type:
          switch-on: _root.version
          cases:
            0x000c: preserved_v12_connectivity_controller
            0x000d: connection_vertex_controller(pool_directory[21].used_count)
        size: pool_directory[21].used_bytes
      - id: controller_payload_23
        type:
          switch-on: _root.version
          cases:
            0x000c: preserved_v12_connectivity_controller
            0x000d: net_name_controller(pool_directory[22].used_count)
        size: pool_directory[22].used_bytes

  preserved_v12_definition_controller:
    doc: Exact bounded v0x000C definition payload; semantics unsupported without paired ASCII evidence.
    seq:
      - id: preserved_payload
        size-eos: true
        doc: All seven v0x000C files contribute exact controller bytes; importer disposition is UNSUPPORTED with raw bytes preserved.

  preserved_v12_placement_controller:
    doc: Exact bounded v0x000C placement payload; semantics unsupported without paired ASCII evidence.
    seq:
      - id: preserved_payload
        size-eos: true
        doc: All seven v0x000C files contribute exact controller bytes; importer disposition is UNSUPPORTED with raw bytes preserved.

  preserved_v12_connectivity_controller:
    doc: Exact bounded v0x000C connectivity payload; semantics unsupported without paired ASCII evidence.
    seq:
      - id: preserved_payload
        size-eos: true
        doc: All seven v0x000C files contribute exact controller bytes; importer disposition is UNSUPPORTED with raw bytes preserved.

  net_name_controller:
    params:
      - id: num_records
        type: u4
    seq:
      - id: records
        type: net_name_record
        repeat: expr
        repeat-expr: num_records

  net_name_record:
    doc: Exact 48-byte v0x000D *NETNAMES* presentation record. Record counts match every native Logic 9 ASCII sheet in SC350460A01.
    seq:
      - id: font_handle
        type: s2
      - id: height_half_mil
        type: u2
      - id: width_factor
        type: u2
      - id: preserved_presentation_06
        size: 10
        doc: Exact presentation bytes; the paired ASCII fields do not distinguish their semantics, so importer disposition is PRESERVED.
      - id: global_net_record
        type: u4
      - id: text_x_offset_quarter_mil
        type: s2
      - id: text_y_offset_quarter_mil
        type: s2
      - id: rotation_tenths_degree
        type: u2
      - id: justification
        type: u2
        doc: The low byte is the Logic 9.0 justification code; paired vendor fields prove the high byte carries unrelated packed data.
      - id: secondary_x_offset_quarter_mil
        type: s2
      - id: secondary_y_offset_quarter_mil
        type: s2
      - id: preserved_presentation_20
        type: u2
        doc: Exact paired-ASCII presentation word with unproved semantics; importer disposition is PRESERVED.
      - id: presentation_flags
        type: u2
      - id: predecessor_handle
        type: u2
      - id: owner_handle
        type: u2
        doc: "Typed owner: 0x2xxx controller-20 off-page record, 0x4xxx controller-18 bus record, otherwise controller-15 placement record."
      - id: owner_child_handle
        type: u2
        doc: Bus member ordinal for 0x4xxx owners; placement-local pin ordinal for controller-15 owners.
      - id: predecessor_record
        type: u2
      - id: successor_record
        type: u2
      - id: preserved_tail
        type: u2
        doc: Exact record tail; paired ASCII does not expose semantics, so importer disposition is PRESERVED.

  bus_controller:
    params:
      - id: num_records
        type: u4
    seq:
      - id: records
        type: bus_record
        repeat: expr
        repeat-expr: num_records

  bus_record:
    seq:
      - id: reserved_zero_00
        contents: [0, 0, 0, 0]
      - id: vertex_prefix_index
        type: u4
      - id: global_net_record
        type: u4
      - id: preserved_relationship_0c
        type: u4
        doc: Exact bus relationship word; generated bus diffs do not identify an exported property, so importer disposition is PRESERVED.
      - id: preserved_relationship_10
        type: u4
        doc: Exact bus relationship word; generated bus diffs do not identify an exported property, so importer disposition is PRESERVED.
      - id: preserved_relationship_14
        type: u4
        doc: Exact bus relationship word; generated bus diffs do not identify an exported property, so importer disposition is PRESERVED.
      - id: tail_bus_entry_handle
        type: u2
      - id: preserved_relationship_1a
        type: u2
        doc: Exact bus relationship word; generated bus diffs do not identify an exported property, so importer disposition is PRESERVED.
      - id: bounds_x1_biased_quarter_mil
        type: u2
      - id: bounds_y1_biased_quarter_mil
        type: u2
      - id: bounds_x2_biased_quarter_mil
        type: u2
      - id: bounds_y2_biased_quarter_mil
        type: u2
      - id: preserved_relationship_24
        type: u2
        doc: Exact bus relationship word; generated bus diffs do not identify an exported property, so importer disposition is PRESERVED.
      - id: class_and_status
        type: u2
      - id: reserved_zero_28
        contents: [0, 0, 0, 0]

  junction_controller:
    params:
      - id: num_records
        type: u4
    seq:
      - id: records
        type: junction_record
        repeat: expr
        repeat-expr: num_records

  junction_record:
    seq:
      - id: reserved_zero_00
        contents: [0, 0, 0, 0]
      - id: x_biased_quarter_mil
        type: u2
      - id: y_biased_quarter_mil
        type: u2
      - id: connection_handle
        type: u2
      - id: status
        type: u2

  offpage_controller:
    params:
      - id: num_records
        type: u4
    seq:
      - id: records
        type: offpage_record
        repeat: expr
        repeat-expr: num_records

  offpage_record:
    seq:
      - id: reserved_zero_00
        contents: [0, 0, 0, 0]
      - id: decal_or_entry_predecessor_handle
        type: u2
      - id: preserved_relationship_06
        type: u2
        doc: Exact off-page relationship word; label-kind and ownership diffs do not prove semantics, so importer disposition is PRESERVED.
      - id: connection_record
        type: u2
      - id: preserved_relationship_0a
        type: u2
        doc: Exact off-page relationship word; label-kind and ownership diffs do not prove semantics, so importer disposition is PRESERVED.
      - id: bounds_x1_biased_quarter_mil
        type: u2
      - id: bounds_y1_biased_quarter_mil
        type: u2
      - id: bounds_x2_biased_quarter_mil
        type: u2
      - id: bounds_y2_biased_quarter_mil
        type: u2
      - id: preserved_relationship_14
        type: u2
        doc: Exact off-page relationship word; label-kind and ownership diffs do not prove semantics, so importer disposition is PRESERVED.
      - id: x_biased_quarter_mil
        type: u2
      - id: y_biased_quarter_mil
        type: u2
      - id: rotation_tenths_degree
        type: u2
      - id: preserved_relationship_1c
        type: u2
        doc: Exact off-page relationship word; label-kind and ownership diffs do not prove semantics, so importer disposition is PRESERVED.
      - id: variant
        type: u1
        enum: offpage_variant
        doc: Variant within the controller-7 decal family. Paired Logic 9.0 exports prove $OSR_ variants 0 through 5 are signal off-page references; $GND_SYMS variants select GND/GNDA/GNDCH, and $PWR_SYMS variants select +5V/+12V/-5V/-12V/+5VA circle or filled-triangle presentations. Values 0xfe and 0xff identify local labels and bus entries.
      - id: reserved_zero_1f
        contents: [0]

  connection_controller:
    params:
      - id: num_records
        type: u4
    seq:
      - id: records
        type: connection_record
        repeat: expr
        repeat-expr: num_records

  connection_record:
    seq:
      - id: reserved_zero_00
        contents: [0, 0, 0, 0]
      - id: vertex_prefix_index
        type: u4
      - id: global_net_record
        type: u4
      - id: endpoint_a_handle
        type: u2
      - id: endpoint_b_handle
        type: u2
      - id: endpoint_a_relationship
        type: u4
        doc: Exact producer-owned endpoint relationship. SC350460A01 pin-to-pin records disprove a pin-ordinal interpretation, so importer disposition is PRESERVED.
      - id: endpoint_b_relationship
        type: u4
        doc: Exact producer-owned endpoint relationship. SC350460A01 pin-to-pin records disprove a pin-ordinal interpretation, so importer disposition is PRESERVED.
      - id: bounds_x1_biased_quarter_mil
        type: u2
      - id: bounds_y1_biased_quarter_mil
        type: u2
      - id: bounds_x2_biased_quarter_mil
        type: u2
      - id: bounds_y2_biased_quarter_mil
        type: u2
      - id: preserved_relationship_20
        type: u2
        doc: Exact connection relationship word; endpoint and net controlled diffs leave semantics unproved, so importer disposition is PRESERVED.
      - id: class_and_status
        type: u2
      - id: preserved_connection_tail_24
        type: u4
        doc: Exact connection tail word; generated topology diffs do not identify an exported property, so importer disposition is PRESERVED.

  connection_vertex_controller:
    params:
      - id: num_records
        type: u4
    seq:
      - id: records
        type: connection_vertex_record
        repeat: expr
        repeat-expr: num_records

  connection_vertex_record:
    seq:
      - id: reserved_zero_00
        contents: [0, 0, 0, 0]
      - id: x_biased_quarter_mil
        type: u2
      - id: y_biased_quarter_mil
        type: u2

  indexed_placement_attribute_heap:
    doc: NUL-terminated component attribute slots indexed by outer controller 7; byte 1 separates custom-field names and values.
    seq:
      - id: indexed_attribute_bytes
        size-eos: true
        doc: Exact length-bounded string heap; validated outer-controller offsets select slots and raw bytes remain preserved.

  placement_group_controller:
    params:
      - id: num_records
        type: u4
    seq:
      - id: records
        type: placement_group_record
        repeat: expr
        repeat-expr: num_records

  placement_group_record:
    seq:
      - id: first_attribute_index
        type: u4
      - id: preserved_group_properties
        size: 16
        doc: Exact component-group bytes; placement and field controlled diffs do not prove individual semantics, so importer disposition is PRESERVED.
      - id: attribute_count
        type: u2
      - id: preserved_group_tail
        size: 2
        doc: Exact component-group tail; all paired placement exports leave semantics unproved, so importer disposition is PRESERVED.

  placement_attribute_offset_controller:
    params:
      - id: num_heap_offsets
        type: u4
    seq:
      - id: heap_offsets
        type: u4
        repeat: expr
        repeat-expr: num_heap_offsets

  placement_font_controller:
    params:
      - id: num_records
        type: u4
    seq:
      - id: records
        type: placement_font_record
        repeat: expr
        repeat-expr: num_records

  placement_font_record:
    seq:
      - id: style_flags
        type: u4
        doc: Logic 9 native save/re-export proves bit 0 italic, bit 1 bold, and bit 2 underline.
      - id: family
        type: strz
        size: 32
        encoding: windows-1252

  placement_controller:
    params:
      - id: num_records
        type: u4
    seq:
      - id: records
        type: placement_record
        repeat: expr
        repeat-expr: num_records

  placement_record:
    seq:
      - id: reference_font_handle
        type: s2
      - id: part_type_font_handle
        type: s2
      - id: value_font_handle
        type: s2
      - id: wildcard_font_handle
        type: s2
      - id: preserved_field_link_properties
        size: 12
        doc: Exact placement field-link bytes; field fixtures prove the adjacent handles only, so importer disposition is PRESERVED.
      - id: placed_pin_start
        type: u4
      - id: component_identity
        type: u4
      - id: component_group_handle
        type: u4
      - id: x_biased_quarter_mil
        type: u2
      - id: y_biased_quarter_mil
        type: u2
      - id: rotation_tenths_degree
        type: u2
        doc: Observed semantic values are 0, 900, 1800, and 2700; other values are preserved and warned by the importer.
      - id: mirror_flags
        type: u2
        doc: >-
          Controlled native-save/re-export parity proves values 0 and 1; the paired production
          corpus proves 2 and 3. Bit 0 maps to KiCad SYM_MIRROR_Y and bit 1 maps to
          SYM_MIRROR_X. Other values are preserved and warned by the importer.
      - id: reference_x_half_mil
        type: s2
      - id: reference_y_half_mil
        type: s2
      - id: reference_rotation_tenths_degree
        type: u2
      - id: reference_justification
        type: u2
        doc: Controlled native-save/reopen parity covers low-byte codes 0 through 15. Codes 0/1 are bottom left/right, 2/3/4/5/6/7 are top left/right/center/right/left/right, and 8/9/10/11/12/13/14/15 are center left/right/center/right/center/right/center/right. The high byte carries unrelated packed data.
      - id: part_type_x_half_mil
        type: s2
      - id: part_type_y_half_mil
        type: s2
      - id: part_type_rotation_tenths_degree
        type: u2
      - id: part_type_justification
        type: u2
        doc: Controlled native-save/reopen parity covers low-byte codes 0 through 15 using the same vertical-band mapping as reference fields. The high byte carries unrelated packed data.
      - id: preserved_instance_properties_38
        size: 10
        doc: Exact placement-instance bytes; transform and field diffs leave these values unchanged or semantically unowned, so importer disposition is PRESERVED.
      - id: part_type_handle
        type: u2
      - id: used_decal_handle
        type: u2
      - id: preserved_decal_link_properties
        size: 4
        doc: Exact placement decal-link bytes; typed part, gate, and decal joins use adjacent fields, so importer disposition is PRESERVED.
      - id: gate_index
        type: u2
      - id: placed_pin_count
        type: u2
      - id: custom_field_count
        type: u2
      - id: reference_height_half_mil
        type: u2
      - id: part_type_height_half_mil
        type: u2
      - id: preserved_text_size_properties
        size: 4
        doc: Exact placement text-size bytes; independent proven heights and widths are adjacent, so importer disposition is PRESERVED.
      - id: reference_width_half_mil
        type: u1
      - id: part_type_width_half_mil
        type: u1
      - id: preserved_text_presentation
        size: 4
        doc: Exact placement text-presentation bytes; generated ASCII pairs do not expose their semantics, so importer disposition is PRESERVED.
      - id: reference_designator
        type: strz
        size: 40
        encoding: windows-1252
      - id: reserved_instance_tail_zero
        contents: [0]
      - id: item_visibility_flags
        type: u1
        doc: Logic 9 paired placements prove bit 0 hides REF-DES, bit 1 hides PART-TYPE, bit 3 hides pin names, and bit 4 hides pin numbers. Controlled native records retain all values 0 through 31; bit 2 has no observed presentation effect and remains preserved only.

  placed_pin_controller:
    params:
      - id: num_records
        type: u4
    seq:
      - id: records
        type: placed_pin_record
        repeat: expr
        repeat-expr: num_records

  placed_pin_record:
    seq:
      - id: preserved_pin_link_prefix
        size: 4
        doc: Exact placed-pin link prefix; controller-16 ordinal diffs prove only the adjacent pin ordinal, so importer disposition is PRESERVED.
      - id: definition_pin_ordinal
        type: u2
      - id: number_offset_x_half_mil_divided_by_2
        type: s2
        doc: Paired Logic 9 placement pin records prove this signed value times two is the pin-number X offset in source half-mils.
      - id: number_offset_y_half_mil_divided_by_2
        type: s2
        doc: Paired Logic 9 placement pin records prove this signed value times two is the pin-number Y offset in source half-mils.
      - id: number_presentation_flags
        type: u2
        doc: >-
          Bit 0 rotates the pin number. A native-reopened controlled fixture proves bits 4-7
          encode Logic 9 justification codes 0-15 through nibble values
          0,2,8,A,1,3,9,B,4,6,C,E,5,7,D,F. Remaining bits are retained.

  placement_field_controller:
    params:
      - id: num_records
        type: u4
    seq:
      - id: records
        type: placement_field_record
        repeat: expr
        repeat-expr: num_records

  placement_field_record:
    seq:
      - id: font_handle
        type: s2
      - id: preserved_field_link
        size: 6
        doc: Exact custom-field link bytes; generated field diffs prove the adjacent font and presentation fields only, so importer disposition is PRESERVED.
      - id: x_half_mil
        type: s2
      - id: y_half_mil
        type: s2
      - id: rotation_tenths_degree
        type: u2
      - id: justification
        type: u1
        doc: Controlled native-save/reopen parity covers codes 0 through 15. Codes 0/1 are bottom left/right, 2/3/4/5/6/7 are top left/right/center/right/left/right, and 8/9/10/11/12/13/14/15 are center left/right/center/right/center/right/center/right.
      - id: presentation_class
        type: u1
      - id: component_attribute_index
        type: u2
        doc: Index within the placement's outer-controller component attribute slice; 0xffff is used by unnamed records.
      - id: height_half_mil
        type: u2
      - id: width_half_mil
        type: u1
      - id: display_flags
        type: u1
      - id: preserved_field_tail
        type: u2
        doc: Exact custom-field tail; generated field exports provide no corresponding property, so importer disposition is PRESERVED.

  preserved_definition_controller:
    doc: Exact bounded controller payload whose semantics are not yet proven.
    seq:
      - id: preserved_payload
        size-eos: true
        doc: Controller bytes are bounded by the owning sheet pool; importer disposition is UNSUPPORTED with raw bytes preserved.

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
        doc: Exact definition name/class byte; all generated primitive classes leave its semantics unproved, so importer disposition is PRESERVED.
      - id: preserved_name_class_byte_27
        type: u1
        doc: Exact definition name/class byte; all generated primitive classes leave its semantics unproved, so importer disposition is PRESERVED.
      - id: preserved_name_class_byte_28
        type: u1
        doc: Exact definition name/class byte; all generated primitive classes leave its semantics unproved, so importer disposition is PRESERVED.
      - id: object_class
        type: u1
        doc: Class 0 owns page drawing groups; class 6 owns reusable symbol definitions.
      - id: graphic_piece_count
        type: u2
        doc: Exact controller-4 piece count. Paired DRW5982 is a 69-piece drawing-sheet group with 58 embedded texts and is promoted to an embedded KiCad worksheet.
      - id: preserved_definition_word_2c
        type: u4
        doc: Exact definition word; primitive count and ownership diffs prove adjacent fields only, so importer disposition is PRESERVED.
      - id: terminal_prefix_index
        type: u4
      - id: vertex_prefix_index
        type: u4
      - id: preserved_definition_word_38
        type: u4
        doc: Exact definition word; vertex and terminal ownership diffs prove adjacent indexes only, so importer disposition is PRESERVED.
      - id: timestamp_or_page_origin
        type: symbol_definition_timestamp_or_page_origin
        doc: Object class 0 stores the page-graphic group origin as biased quarter-mil X/Y words; symbol definitions store the original 32-bit timestamp.
      - id: embedded_text_count
        type: u2
        doc: Native Logic 9.0 exports prove the number of embedded controller-1 text records for both generated and legacy DRW groups.
      - id: embedded_text_last_record
        type: u2
        doc: Terminal controller-1 record of the embedded-text chain. The declared count and each record's predecessor ordinal recover exact generated and legacy DRW ownership without assuming contiguous controller order.
      - id: preserved_definition_style_word_44
        type: s2
        doc: Exact definition style word; generated line/fill/text variants expose no ASCII counterpart, so importer disposition is PRESERVED.
      - id: preserved_definition_style_word_46
        type: s2
        doc: Exact definition style word; generated line/fill/text variants expose no ASCII counterpart, so importer disposition is PRESERVED.
      - id: preserved_definition_style_word_48
        type: s2
        doc: Exact definition style word; generated line/fill/text variants expose no ASCII counterpart, so importer disposition is PRESERVED.
      - id: preserved_definition_style_word_4a
        type: s2
        doc: Exact definition style word; generated line/fill/text variants expose no ASCII counterpart, so importer disposition is PRESERVED.
      - id: preserved_definition_style_word_4c
        type: s2
        doc: Exact definition style word; generated line/fill/text variants expose no ASCII counterpart, so importer disposition is PRESERVED.
      - id: preserved_definition_style_word_4e
        type: s2
        doc: Exact definition style word; generated line/fill/text variants expose no ASCII counterpart, so importer disposition is PRESERVED.

  symbol_definition_timestamp_or_page_origin:
    seq:
      - id: timestamp_low_or_page_origin_x_biased_quarter_mil
        type: u2
      - id: timestamp_high_or_page_origin_y_biased_quarter_mil
        type: u2

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
      - id: line_style
        type: u1
        enum: graphic_line_style
        doc: Logic 9 paired exports prove 0 only for dashed closed polygons and 0xff for solid graphics. Controlled ASCII probes rejected 1 and open-graphic 0; other values remain unsupported rather than inferred.
      - id: vertex_count
        type: u2
      - id: stroke_width_mils
        type: u1
        doc: Direct mil width. Native Logic 9.0 exports correlate values 1, 2, 5, 7, 8, 10, 11, 15, 20, 25, 30, 31, and 40 exactly, including legacy DRW groups.
      - id: preserved_presentation
        type: u1
        doc: The purpose of this field is not known.

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
      - id: x_quarter_mil
        type: s2
      - id: y_quarter_mil
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
        doc: Exact arc marker between direction and bounds; generated clockwise/counterclockwise arcs do not prove semantics, so importer disposition is PRESERVED.
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
        doc: Exact used-decal flag word; controlled fixtures do not prove individual bit semantics, so importer disposition is PRESERVED.
      - id: terminal_count
        type: u1
      - id: pin_origin_code
        type: u1
      - id: terminal_prefix_index
        type: u2
      - id: preserved_used_decal_word_2e
        type: u2
        doc: Exact used-decal word; definition and terminal controlled diffs leave semantics unproved, so importer disposition is PRESERVED.
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
        doc: The low byte is the Logic 9.0 justification code; paired vendor fields prove the high byte carries unrelated packed data.
      - id: part_type_x_half_mil_divided_by_2
        type: s2
      - id: part_type_y_half_mil_divided_by_2
        type: s2
      - id: part_type_angle_tenths_degree
        type: u2
      - id: part_type_justification
        type: u2
        doc: The low byte is the Logic 9.0 justification code; paired vendor fields prove the high byte carries unrelated packed data.
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
        doc: Exact used-decal presentation word; field geometry is proven by adjacent values, so importer disposition is PRESERVED.
      - id: preserved_decal_word_56
        type: u2
        doc: Exact used-decal presentation word; field geometry is proven by adjacent values, so importer disposition is PRESERVED.
      - id: reference_height_half_mil
        type: u2
      - id: part_type_height_half_mil
        type: u2
      - id: value_height_half_mil
        type: u2
      - id: wildcard_height_half_mil
        type: u2
      - id: reference_width_half_mil
        type: u1
      - id: part_type_width_half_mil
        type: u1
      - id: value_width_half_mil
        type: u1
      - id: wildcard_width_half_mil
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
        type: s2
      - id: pin_name_width_half_mil_divided_by_2
        type: s2
      - id: pin_number_height_half_mil_divided_by_2
        type: s2
      - id: pin_number_width_half_mil_divided_by_2
        type: s2
      - id: name_offset_x_half_mil_divided_by_2
        type: s2
      - id: name_offset_y_half_mil_divided_by_2
        type: s2
      - id: number_offset_x_half_mil_divided_by_2
        type: s2
      - id: number_offset_y_half_mil_divided_by_2
        type: s2
      - id: side_and_name_presentation_flags
        type: u2
        doc: >-
          Terminal rotation is bit 0 and the side selector is bits 1-2. Bit 8 rotates the pin
          name. Bits 12-15 encode the PADS justification code. At zero degrees the inverse
          nibble permutation is 0,4,1,5,8,12,9,13,2,6,3,7,10,14,11,15; at 90 degrees it is
          0,4,2,6,8,12,10,14,1,5,3,7,9,13,11,15. Genuine native-save/reopen fixtures cover
          every source code 0 through 15 at both angles.
      - id: visibility_and_number_presentation_flags
        type: u2
        doc: >-
          Bit 0 rotates the pin number. Bits 4-7 use the same angle-dependent justification
          permutations as the pin name. Bits 8-11 select the name/number offset presentation,
          and bits 14-15 are exported as PADS pin visibility flags.

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
        doc: Exact part-type word; multigate and connector diffs prove adjacent prefix indexes only, so importer disposition is PRESERVED.
      - id: preserved_part_type_word_38
        type: u4
        doc: Exact part-type word; multigate and connector diffs prove adjacent prefix indexes only, so importer disposition is PRESERVED.
      - id: pin_name_heap_base
        type: u4
        doc: Controller-11 pin-name offsets are relative to this byte offset in controller 14; the SC350460 301212 and 301211 records prove independent consecutive name slices.
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
      - id: container_item_count
        type: u4
      - id: container_items
        type:
          switch-on: container_item_count
          cases:
            0: no_cfb_container_items
            _: first_cfb_container_item(container_item_count)

  no_cfb_container_items:
    seq: []

  first_cfb_container_item:
    params:
      - id: container_item_count
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
        doc: Exact MFC OLE-item wrapper bytes from the controlled and private corpora; semantics are UNSUPPORTED and retained by the schema ledger.
      - id: len_cfb
        type: u4
      - id: container_item
        type: cfb_container_item_chain(len_cfb, container_item_count)

  cfb_container_item_chain:
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
            1: final_cfb_container_item_trailer
            _: nonfinal_cfb_container_item_trailer(remaining_count - 1)

  nonfinal_cfb_container_item_trailer:
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
        type: ole_extent
      - id: rectangle
        type: ole_database_box
      - id: sheet_plane
        type: u4
        doc: Zero-based PADS sheet/plane index. CPowerPCBCntrItem::GetOLEObjectExtents filters on this value before returning the database box.
      - id: flags
        type: u4
        doc: The purpose of this field is not known.
      - id: next_item_state
        size: 20
        doc: MFC state preceding the next serialized CPowerPCBCntrItem. Semantics are UNSUPPORTED and bytes are retained by the schema ledger.
      - id: next_len_cfb
        type: u4
      - id: next_container_item
        type: cfb_container_item_chain(next_len_cfb, remaining_count)

  final_cfb_container_item_trailer:
    seq:
      - id: class_id
        contents: [0x7b, 0x46, 0x34, 0x39, 0x39, 0x37, 0x44, 0x37,
                   0x30, 0x2d, 0x41, 0x46, 0x38, 0x41, 0x2d, 0x31,
                   0x31, 0x44, 0x30, 0x2d, 0x41, 0x33, 0x37, 0x33,
                   0x2d, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
                   0x30, 0x30, 0x30, 0x30, 0x30, 0x7d]
      - id: extent
        type: ole_extent
      - id: rectangle
        type: ole_database_box
      - id: sheet_plane
        type: u4
        doc: Zero-based PADS sheet/plane index. CPowerPCBCntrItem::GetOLEObjectExtents filters on this value before returning the database box.
      - id: flags
        type: u4
        doc: The purpose of this field is not known.

  ole_extent:
    doc: CRect serialized by COleClientItem::Serialize from CPowerPCBCntrItem offset +0x84. Controlled objects use 30,30 followed by the native server extent.
    seq:
      - id: left
        type: s4
      - id: top
        type: s4
      - id: right
        type: s4
      - id: bottom
        type: s4

  ole_database_box:
    doc: PADS SYS_Box stored at CPowerPCBCntrItem offset +0x94 and returned directly by GetDBBox. Serialize writes left, bottom, right, top in this order.
    seq:
      - id: left
        type: s4
      - id: bottom
        type: s4
      - id: right
        type: s4
      - id: top
        type: s4

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
