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
  - id: raw_database_payload
    size: _io.size - _io.pos - 42
    doc: Serialized schematic database controllers and embedded preview data.
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
