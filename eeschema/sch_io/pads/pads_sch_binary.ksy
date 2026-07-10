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
      - id: controller_payload_1
        size: _root.pool_directory[1].used_bytes
      - id: controller_payload_2
        size: _root.pool_directory[2].used_bytes
      - id: controller_payload_3
        size: _root.pool_directory[3].used_bytes
      - id: controller_payload_4
        size: _root.pool_directory[4].used_bytes
      - id: controller_payload_5
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
      - id: controller_payload_1
        size: pool_directory[0].used_bytes
      - id: controller_payload_2
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
