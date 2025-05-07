meta:
  id: allegro_brd
  title: Allegro board file
  application: Allegro
  tags:
    - EDA
    - PCB
  file-extension:
    - brd
    - dra
  license: GPL-2.0-or-later
  ks-version: 0.7
  endian: le
doc: |
  Cadence Allegro printed circuit board (PCB) board file. This is the native
  binary format saved by the Cadence Allegro PCB editor. Board files (.brd)
  and footprint drawing files (.dra) use the same format.

  There is no known public documentation for this format.

seq:
  - id: magic
    type: u4
    doc: The four-byte magic number, related to the file version
  - id: unk1
    type: u4
    repeat: expr
    repeat-expr: 4
  - id: object_count
    type: u4
  - id: unk2
    type: u4
    repeat: expr
    repeat-expr: 9
  - id: ll_x04
    type: linked_list
  - id: ll_x06
    type: linked_list
  - id: ll_x0c_2
    type: linked_list
  - id: ll_x0e_x28
    type: linked_list
  - id: ll_x14
    type: linked_list
  - id: ll_x1b
    type: linked_list
    doc: |
      List of nets in the board.
  - id: ll_x1c
    type: linked_list
    doc: |
      List of padstacks in the board.
  - id: ll_x24_x28
    type: linked_list
    doc: |
      List of rectangles and shapes.
  - id: ll_unused_1
    type: linked_list
  - id: ll_x2b
    type: linked_list
  - id: ll_x03_x30
    type: linked_list
    doc: |
      List of text items in the board.
  - id: ll_x0a
    type: linked_list
  - id: ll_x1d_x1e_x1f
    type: linked_list
  - id: ll_unused_2
    type: linked_list
  - id: ll_x38
    type: linked_list
  - id: ll_x2c
    type: linked_list
  - id: ll_x0c
    type: linked_list
  - id: ll_unused_3
    type: linked_list
  - id: x35_start
    type: u4
  - id: x35_end
    type: u4
  - id: ll_x36
    type: linked_list
  - id: ll_unused_4
    type: linked_list
    doc: |
      This appears to be empty in all files checked.
  - id: ll_unused_5
    type: linked_list
    doc: |
      This appears to be empty in all files checked.
  - id: ll_x0a_2
    type: linked_list
    doc: |
      This appears to be empty in some files (PreAmp, CutiePi)
      Exists in BeagleBone-AI. Presumably related to DRC.
  - type: u4
  - id: allegro_version
    type: str
    size: 60
    encoding: ASCII
  - type: u4
  - id: max_key
    type: u4
  - type: u4
    repeat: expr
    repeat-expr: 17
  - id: board_units
    type: u1
    enum: board_units
  - type: u1
  - type: u2
  - type: u4
  - type: u4
  - id: x27_end_offset
    type: u4
  - type: u4
  - id: strings_count
    type: u4
  - type: u4
    repeat: expr
    repeat-expr: 53
  - id: unit_divisor
    type: u4
  - id: layer_map
    type: layer_map

  - type: u4
    repeat: until
    repeat-until: _io.pos == 0x1200

  - id: string_map
    type: string_map(strings_count)

  - id: objects
    type: board_object
    repeat: eos

instances:
  ver:
    doc: |
      This provides the version of the file, which is used to determine
      how to parse the file. It seems that magics that vary in the last byte
      are format-compatible.
    value: magic & 0xFFFFFF00

enums:
  board_units:
    1: imperial
    3: metric

  version:
    # There are more versions than this, these are just the known ones.
    0x00130000: a160  # Allegro 16.0
    0x00130400: a162
    0x00130C00: a164
    0x00131500: a166
    0x00140400: a172
    0x00140900: a174
    0x00141500: a175

types:
  linked_list:
    seq:
      - id: tail
        type: u4
      - id: head
        type: u4

  layer_map:
    seq:
      - id: entries
        type: entry
        repeat: expr
        repeat-expr: 25

    types:
      entry:
        seq:
          - id: a
            type: u4
          - id: b
            type: u4

  string_map:
    params:
      - id: num_entries
        type: u4
    seq:
      - id: entries
        type: entry
        repeat: expr
        repeat-expr: num_entries
    types:
      entry:
        seq:
          - id: string_id
            type: u4
          - id: value
            type: str
            encoding: ASCII
            terminator: 0
          - type: u1
            # Round up to align next on 4-bytes
            if: _io.pos % 4 != 0
            repeat: until
            repeat-until: _io.pos % 4 == 0

  cadence_fp:
    seq:
      - id: a
        type: u4
      - id: b
        type: u4

  layer_info:
    seq:
      - id: family
        type: u1
        enum: layer_family
      - id: ordinal
        type: u1

    enums:
      layer_family:
        0x01: board_geometry
        0x06: copper
        0x09: silk

  string_aligned:
    params:
      - id: num_chars
        type: u2
    seq:
    # This is not always a nice string(?), so treat as bytes.
    - id: chars
      type: u1
      repeat: expr
      repeat-expr: num_chars
    - type: u1
      # Round up to align next on 4-bytes
      repeat: until
      repeat-until: _io.pos % 4 == 0
      if: _io.pos % 4 != 0

  board_object:
    doc: |
      The basic object types that make up most of a .brd file.

      The occur in a long block after the header and string tables, and are
      joined end-to end. There is no length field, and the object sizes
      vary based on content and file version, so you have to parse the objects
      to find the next one. All objects seem to be 4-byte aligned.

      Every object starts with a type byte. Most objects have a field, usually
      named 'k' in this spec, and first word in the object after the type,
      that is a four-byte unique identifier for the object.
      This is often used by other objects to refer to this object via
      "pointers" - i.e. the 'k' value of the pointed-to object.
    seq:
      - id: type
        type: u1
      - id: data
        type:
          switch-on: type
          cases:
            0x01: type_01_arc
            0x03: type_03
            0x04: type_04_net_assignment
            0x05: type_05_track
            0x06: type_06
            0x07: type_07
            0x08: type_08
            0x09: type_09
            0x0a: type_0a_drc
            0x0c: type_0c
            0x0d: type_0d_pad
            0x0e: type_0e
            0x0f: type_0f
            0x10: type_10
            0x11: type_11
            0x12: type_12
            0x14: type_14
            0x15: type_15_segment
            0x16: type_16_segment
            0x17: type_17_segment
            0x1b: type_1b_net
            0x1c: type_1c_pad_stack
            0x1d: type_1d
            0x1e: type_1e
            0x1f: type_1f
            0x20: type_20
            0x21: type_21_header
            0x22: type_22
            0x23: type_23_ratline
            0x24: type_24_rect
            0x26: type_26
            0x27: type_27
            0x28: type_28_shape
            0x29: type_29
            0x2a: type_2a
            0x2b: type_2b
            0x2c: type_2c_table
            0x2d: type_2d
            0x2e: type_2e
            0x2f: type_2f
            0x30: type_30_str_wrapper
            0x31: type_31_sgraphic
            0x32: type_32_placed_pad
            0x33: type_33_via
            0x34: type_34_keepout
            0x35: type_35
            0x36: type_36
            0x37: type_37
            0x38: type_38_film
            0x39: type_39_film_layer_list
            0x3a: type_3a_film_list_node
            0x3b: type_3b
            0x3c: type_3c
            _: type_default

  # Doesn't actually work, but it's useful to explode the
  # parser to see what's going on.
  type_default:
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
        valid:
          expr: false

  type_01_arc:
    seq:
      - id: t
        type: u1
      - id: un0
        type: u1
      - id: subtype
        type: u1
      - id: key
        type: u4
      - id: next
        type: u4
      - id: parent
        type: u4
      - id: un1
        type: u4
      - id: un6
        type: u4
        if: _root.ver >= 0x00140400
      - id: width
        type: u4
      - id: coords
        type: s4
        repeat: expr
        repeat-expr: 4
      - id: x
        type: cadence_fp
      - id: y
        type: cadence_fp
      - id: r
        type: cadence_fp
      - id: bbox
        type: s4
        repeat: expr
        repeat-expr: 4

  type_03:
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - id: next
        type: u4
      - id: un1
        type: u4
        if: _root.ver >= 0x00140400
      - id: subtype
        type: u1
      - id: unhdr
        type: u1
      - id: size
        type: u2
      - id: un2
        type: u4
        if: _root.ver >= 0x00140400
      - id: data
        type:
          switch-on: subtype
          cases:
            # Yes, four_bytes/eight_bytes are wierd,
            # but needed for C++:
            # https://github.com/kaitai-io/kaitai_struct/issues/416
            0x69: eight_bytes
            0x64: four_bytes
            0x66: four_bytes
            0x67: four_bytes
            0x6A: four_bytes
            0x6C: x6c
            0x6D: string_aligned(size)
            0x6E: string_aligned(size)
            0x6F: string_aligned(size)
            0x68: string_aligned(size)
            0x6B: string_aligned(size)
            0x71: string_aligned(size)
            0x73: string_aligned(size)
            0x78: string_aligned(size)
            0x70: x70_x74
            0x74: x70_x74
            0xF6: f6_block
        if: subtype != 0x65

    types:
      four_bytes:
        seq:
          - id: val
            type: u4
      eight_bytes:
        seq:
          - id: val1
            type: u4
          - id: val2
            type: u4
      x6c:
        seq:
          # Unsure of order
          - id: num_entries
            type: u4
          - id: entries
            type: u4
            repeat: expr
            repeat-expr: num_entries

      x70_x74:
        seq:
          - id: x0
            type: u2
          - id: x1
            type: u2
          - id: unk
            type: u1
            repeat: expr
            repeat-expr: x1 + 4 * x0

      # 0xF6 is an 80 byte block
      f6_block:
        seq:
          - type: u4
            repeat: expr
            repeat-expr: 20

  type_04_net_assignment:
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - id: next
        type: u4
      - id: ptr1
        type: u4
      - id: ptr2
        type: u4
      - type: u4
        if: _root.ver >= 0x00140900

  type_05_track:
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - type: u4
      - id: ptr0
        type: u4
      - id: ptr1
        type: u4
      - type: u4
        repeat: expr
        repeat-expr: 2
      - id: ptr2
        type: u4
        repeat: expr
        repeat-expr: 2
      - type: u4
      - id: ptr3a
        type: u4
      - id: ptr3b
        type: u4
        if: _root.ver < 0x00140400
      - type: u4
        if: _root.ver >= 0x00140400
        repeat: expr
        repeat-expr: 3
      - id: first_segment_ptr
        type: u4
      - id: ptr5
        type: u4
      - type: u4

  type_06:
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - id: next
        type: u4
        doc: |
          Pointer to the next object in the 'x06' linked list that starts in
          the header.
      - id: ptr_str
        type: u4
      - id: ptr_2
        type: u4
      - id: ptr_instance
        type: u4
      - id: ptr_fp
        type: u4
      - id: ptr_x08
        type: u4
      - id: ptr_x03_symbol
        type: u4
      - id: unk
        type: u4
        if: _root.ver >= 0x00140000

  type_07:
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - type: u4
      - id: ptr0
        type: u4
        if: _root.ver >= 0x00140400
      - id: un4
        type: u4
        if: _root.ver >= 0x00140400
      - id: un2
        type: u4
        if: _root.ver >= 0x00140400
      - id: ptr_0x2d
        type: u4
      - id: un5
        type: u4
        if: _root.ver < 0x00140400
      - id: ref_des_ref
        type: u4
      - id: ptr2
        type: u4
      - id: ptr3
        type: u4 # 0x03 or null
      - id: un3
        type: u4
      - id: ptr4 # 0x32 or null
        type: u4

  type_08:
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - id: ptr1
        type: u4
        if: _root.ver >= 0x00140400
      - id: str_ptr_16x
        type: u4
        if: _root.ver < 0x00140400
      - id: ptr2
        type: u4
      - id: str_ptr
        type: u4
        if: _root.ver >= 0x00140400
      - id: ptr3
        type: u4
      - id: un1
        type: u4
        if: _root.ver >= 0x00140400
      - id: ptr4
        type: u4

  type_09:
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - id: un1
        type: u4
        repeat: expr
        repeat-expr: 4
      - id: un3
        type: u4
        if: _root.ver >= 0x00140400
      - id: ptr1
        type: u4
      - id: ptr2
        type: u4
      - id: un2
        type: u4
      - id: ptr3
        type: u4
      - id: ptr4
        type: u4
      - id: un4
        type: u4
        if: _root.ver >= 0x00140900

  type_0a_drc:
    seq:
      - id: t
        type: u1
      - id: layer
        type: layer_info
      - id: key
        type: u4
      - id: next
        type: u4
      - id: un1
        type: u4
      - id: un2
        type: u4
        if: _root.ver >= 0x00140400
      - id: coords
        type: s4
        repeat: expr
        repeat-expr: 4
      - id: un4
        type: u4
        repeat: expr
        repeat-expr: 4
      - id: un5
        type: u4
        repeat: expr
        repeat-expr: 5
      - id: un3
        type: u4
        if: _root.ver >= 0x00140900

  type_0c:
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - id: next
        type: u4
      - type: u4
        if: _root.ver >= 0x00140400
      - type: u4
        if: _root.ver >= 0x00140400
      - type: u4
      - type: u4
      - id: keyind
        type: u4
      - type: u4
      - id: coords
        type: s4
        repeat: expr
        repeat-expr: 4
      - type: u4
        repeat: expr
        repeat-expr: 3
      - type: u4
        if: _root.ver >= 0x00140900

  type_0d_pad:
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - id: str_ptr
        type: u4
      - id: ptr2
        type: u4
      - id: un3
        type: u4
        if: _root.ver >= 0x00140900
      - id: coords
        type: s4
        repeat: expr
        repeat-expr: 2
      - id: pad_ptr
        type: u4
      - id: un1
        type: u4
      - id: un2
        type: u4
        if: _root.ver >= 0x00140400
      - id: flags
        type: b32
      - id: rotation
        type: u4

  type_0e:
    seq:
      - id: t
        type: u1
      - id: t2
        type: u2
      - id: key
        type: u4
      - id: next
        type: u4
      - id: fp_ptr
        type: u4
      - id: un2
        type: u4
        repeat: expr
        repeat-expr: 3
      - id: un1
        type: u4
        repeat: expr
        repeat-expr: 2
        if: _root.ver >= 0x00140400
      - id: coords
        type: s4
        repeat: expr
        repeat-expr: 4
      - type: u4
        repeat: expr
        repeat-expr: 4

  type_0f:
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - id: ptr1
        type: u4
      - id: s
        type: str
        size: 32
        encoding: ASCII
      - id: ptr_x06
        type: u4
      - id: ptr_x11
        type: u4
        # Always null - doesn't seem so
      - id: query
        type: u4
      - id: un2
        type: u4
        if: _root.ver >= 0x00140400
      - id: un3
        type: u4
        if: _root.ver >= 0x00140900

  type_10:
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - type: u4
        if: _root.ver >= 0x00140400
      - id: ptr1
        type: u4
      - type: u4
        if: _root.ver >= 0x00140900
      - id: ptr2
        type: u4
      - id: un1
        type: u4
      - id: ptr3
        type: u4
      - id: ptr4 # 0x0F?
        type: u4
      - id: path_str
        type: u4

  type_11:
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - id: ptr1
        type: u4
      - id: ptr2
        type: u4
      - id: ptr3
        type: u4
      - type: u4
      - type: u4
        if: _root.ver >= 0x00140900

  type_12:
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - id: ptr1
        type: u4
      - id: ptr2
        type: u4
      - id: ptr3
        type: u4
      - id: un0
        type: u4
      - id: un1
        type: u4
        if: _root.ver >= 0x00131000
      - id: un2
        type: u4
        if: _root.ver >= 0x00140900

  type_14:
    seq:
      - id: type
        type: u1
      - id: layer
        type: layer_info
      - id: key
        type: u4
      - id: next
        type: u4
      - id: ptr1
        type: u4
      - id: un2
        type: u4
      - id: un3
        type: u4
        if: _root.ver >= 0x00140400
      - id: ptr2
        type: u4
      - id: ptr3
        type: u4
      - id: ptr4
        type: u4

  type_15_segment:
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - id: next
        type: u4
      - id: parent
        type: u4
      - id: un3
        type: u4
      - id: un4
        type: u4
        if: _root.ver >= 0x00140400
      - id: width
        type: u4
      - id: coords
        type: s4
        repeat: expr
        repeat-expr: 4

  type_16_segment:
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - id: next
        type: u4
      - id: parent
        type: u4
      - id: flags
        type: b32
      - type: u4
        if: _root.ver >= 0x00140400
      - id: width
        type: u4
      - id: coords
        type: s4
        repeat: expr
        repeat-expr: 4

  type_17_segment:
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - id: next
        type: u4
      - id: parent
        type: u4
      - type: u4
      - type: u4
        if: _root.ver >= 0x00140400
      - id: width
        type: u4
      - id: coords
        type: s4
        repeat: expr
        repeat-expr: 4

  type_1b_net:
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - id: next
        type: u4
      - id: net_name
        type: u4
        doc: |
          String ID for the net name.
      - id: unknown_1
        type: u4
        doc: |
          This seems like a mask:
            PreAmp N13829: 0x20000020
      - id: unknown_2
        type: u4
        if: _root.ver >= 0x00140400
      - id: type
        type: u4
        doc: |
          Could be a mask:
            PreAmp N13829: 0x202
      - id: ptr1
        type: u4
        doc: |
          PreAmp N13829 -> 0x04
      - id: ptr2
        type: u4
        doc: |
          Points to 0x23 (ratline)
      - id: path_str_ptr
        type: u4
        doc: |
          Points to 0x03 (not to a string ID)
      - id: ptr4
        type: u4
      - id: model_ptr
        type: u4
      - id: unknown_3
        type: u4
      - id: unknown_4
        type: u4
      - id: ptr6
        type: u4

  type_1c_pad_stack:
    seq:
      - type: u1
      - id: n
        type: u1
      - type: u1
      - id: key
        type: u4
      - id: next
        type: u4
      - id: pad_str
        type: u4
        doc: |
          String ID for the padstack name.

          Examples: 'R110_95', 'S_SQU_0-95_P_V0', 'VIA26'
      - type: u4
      - type: u4
      - id: pad_path
        type: u4
      - id: un4
        type: u4
        if: _root.ver < 0x00140400
        repeat: expr
        repeat-expr: 4
      - id: pad_info
        type: pad_info
      - type: u4
        repeat: expr
        repeat-expr: 3
        if: _root.ver >= 0x00140400
      - type: u2
        if: _root.ver < 0x00140400
      - id: layer_count
        type: u2
      - type: u2
        if: _root.ver >= 0x00140400
      - type: u4
        repeat: expr
        repeat-expr: 8
      - type: u4
        repeat: expr
        repeat-expr: 28
        if: _root.ver >= 0x00140400
      - type: u4
        repeat: expr
        repeat-expr: 8
        if: _root.ver & 0x00FFF000 == 0x00131000
      - id: components
        type: padstack_component(num_components, _index)
        repeat: expr
        repeat-expr: num_components
      - id: unk2
        type: u4
        repeat: expr
        repeat-expr: n * 8
        if: _root.ver < 0x00140400
      - id: unk3
        type: u4
        repeat: expr
        repeat-expr: n * 10
        if: _root.ver >= 0x00140400

    instances:
      num_components:
        value: '(_root.ver < 0x00140400) ? (10 + layer_count * 3) : (21 + layer_count * 4)'

    enums:
      pad_type:
        0x00: through_via # Not sure about this, PreAmp all padstacks have this
        0x01: via
        0x02: smt_pin
        0x03: slot
        0x08: npth # maybe?
        0x0a: smt_pin2 # maybe?

    types:
      pad_info:
        seq:
          - id: pad_type
            type: b4
            enum: pad_type
          - id: a
            type: b4
          - id: b
            type: u1
          - id: c
            type: u1
          - id: d
            type: u1

      padstack_component:
        params:
          - id: total_count
            type: u4
          - id: index
            type: u4
        seq:
          - id: t
            type: u1
            doc: |
              0x00: empty?
              0x06: ?
          - type: u1
          - type: u1
          - type: u1
          - type: u4
            if: _root.ver >= 0x00140400
          - id: w
            type: s4
          - id: h
            type: s4
          - id: z1
            type: u4
            if: _root.ver >= 0x00140400
          - id: x3
            type: s4
          - id: x4
            type: s4
          - id: z
            type: u4
            if: _root.ver >= 0x00140400
          - id: str_ptr
            type: u4
            doc: |
              Points to 0x0F (sometimes, possibly when t = 0x06)

              If t is 0x16, points to 0x28 (shape)?
          - id: z2
            type: u4
            if: _root.ver < 0x00140400 and not (total_count - 1 == index)
            doc: |
              Seems to be missing from the last entry in version < 17.2.

  type_1d:
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - type: u4
        repeat: expr
        repeat-expr: 3
      - id: size_a
        type: u2
      - id: size_b
        type: u2
      - type: u1
        repeat: expr
        repeat-expr: size_b * 56
      - type: u1
        repeat: expr
        repeat-expr: size_a * 256
      - type: u4
        if: _root.ver >= 0x00140400

  type_1e:
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - id: un1
        type: u4
      - id: un2
        type: u2
        if: _root.ver >= 0x00140400 # unsire, not in Preamp
      - id: un3
        type: u2
        if: _root.ver >= 0x00140400 # unsire, not in Preamp
      - id: str_ptr
        type: u4
      - id: size
        type: u4
      - id: str
        type: string_aligned(size)
      - type: u4
        if: _root.ver >= 0x00140400

  type_1f:
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - type: u4
        repeat: expr
        repeat-expr: 4
      - id: un1
        type: u2
      - id: size
        type: u2
      - type: u1
        repeat: expr
        repeat-expr: size * 384 + 8
        if: _root.ver >= 0x00141500
      - type: u1
        repeat: expr
        repeat-expr: size * 280 + 8
        if: _root.ver >= 0x00140400 and _root.ver < 0x00141500
      - type: u1
        repeat: expr
        repeat-expr: size * 280 + 4
        if: _root.ver >= 0x00130400 and _root.ver < 0x00140400
      - type: u1
        repeat: expr
        repeat-expr: size * 240 + 4
        if: _root.ver < 0x00130400

  type_20:
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - id: ptr1
        type: u4
      - id: un
        type: u4
        repeat: expr
        repeat-expr: 7
      - id: un1
        type: u4
        repeat: expr
        repeat-expr: 10
        if: _root.ver >= 0x00140900

  type_21_header:
    seq:
      - id: t
        type: u1
      - id: r
        type: u2
      - id: size
        type: u4
      - id: key
        type: u4
      - type: u1
        repeat: expr
        repeat-expr: size - 12

  type_22:
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - id: un1
        type: u4
        if: _root.ver >= 0x00140400
      - id: un
        type: u4
        repeat: expr
        repeat-expr: 8

  type_23_ratline:
    seq:
      - id: type
        type: u1
      - id: layer
        type: layer_info
      - id: key
        type: u4
      - id: next
        type: u4
      - id: flags
        type: b32
        repeat: expr
        repeat-expr: 2
      - id: ptr1
        type: u4
      - id: ptr2
        type: u4
      - id: ptr3
        type: u4
      - id: coords
        type: s4
        repeat: expr
        repeat-expr: 5
      - id: un
        type: u4
        repeat: expr
        repeat-expr: 4
      - id: un2
        type: u4
        repeat: expr
        repeat-expr: 4
        if: _root.ver >= 0x00130C00
      - id: un1
        type: u4
        if: _root.ver >= 0x00140900

  type_24_rect:
    seq:
      - id: t
        type: u1
      - id: layer
        type: layer_info
      - id: key
        type: u4
      - id: next
        type: u4
      - id: ptr1
        type: u4
      - id: un1
        type: u4
      - id: un2
        type: u4
        if: _root.ver >= 0x00140400
      - id: coords
        type: s4
        repeat: expr
        repeat-expr: 4
      - id: ptr2
        type: u4
      - id: un
        type: u4
        repeat: expr
        repeat-expr: 3

  type_27:
    doc: "A block that extends to the offset in the file header"
    seq:
      - type: u1
        repeat: until
        repeat-until: _io.pos == _root.x27_end_offset - 1

  type_26:
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - id: member_ptr
        type: u4
      - id: un
        type: u4
        if: _root.ver >= 0x00140400
      - id: group_ptr
        type: u4
      - id: const_ptr
        type: u4
      - id: un1
        type: u4
        if: _root.ver >= 0x00140900

  type_28_shape:
    seq:
      - id: type
        type: u1
      - id: layer
        type: layer_info
      - id: key
        type: u4
      - id: next
        type: u4
      - id: ptr1
        type: u4
      - id: un2
        type: u4
      - id: un5
        type: u4
        repeat: expr
        repeat-expr: 2
        if: _root.ver >= 0x00140400
      - id: ptr2
        type: u4
      - id: ptr3
        type: u4
      - id: ptr4
        type: u4
      - id: first_segment_ptr
        type: u4
      - id: un3
        type: u4
      - id: un4
        type: u4
      - id: ptr7
        type: u4
        if: _root.ver >= 0x00140400
      - id: ptr6
        type: u4
      - id: ptr7_16x
        type: u4
        if: _root.ver < 0x00140400
      - id: coords
        type: s4
        repeat: expr
        repeat-expr: 4

  # Only in .dra files.
  # Looks like the pin number
  type_29:
    seq:
      - type: u1
      - id: t
        type: u2
      - id: key
        type: u4
        # Points to something in the header
      - id: ptr1
        type: u4
      - id: ptr2
        type: u4
      - type: u4 # null?
      - id: ptr3
        type: u4
      - id: coord1
        type: s4
      - id: coord2
        type: s4
      - id: ptr_padstack
        type: u4
      - type: u4
      # Pointer to a string, in R0603, that string
      # is, for example, "2"
      - id: ptr_x30
        type: u4
      - type: u4
      - type: u4
      - type: u4

  type_2a:
    seq:
      - type: u1
      - id: num_entries
        type: u2
      - id: unk
        type: u4
        if: _root.ver >= 0x00140900
      - id: nonrefs
        type: nonref
        repeat: expr
        repeat-expr: num_entries
        if: _root.ver <= 0x00130C00
      - id: refs
        type: ref_entry
        repeat: expr
        repeat-expr: num_entries
        if: _root.ver > 0x00130C00
      - id: key
        type: u4

    types:
      nonref:
        seq:
          - id: d
            type: u1
            repeat: expr
            repeat-expr: 36
      ref_entry:
        seq:
          - id: ptr
            type: u4
          - id: properties
            type: u4
          - id: un1
            type: u4

  type_2b:
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - id: fp_str_ref
        type: u4
      - type: u4
      - id: corrds
        type: u4
        repeat: expr
        repeat-expr: 4
      - id: next
        type: u4
      - id: ptr2
        type: u4
      - id: ptr3
        type: u4
      - id: ptr4
        type: u4
      - id: ptr5
        type: u4
      - id: str_ptr1
        type: u4
      - id: ptr6
        type: u4
      - id: ptr7
        type: u4
      - id: ptr8
        type: u4

      - id: un2
        type: u4
        if: _root.ver >= 0x00130C00
      - id: un3
        type: u4
        if: _root.ver >= 0x0140400

  type_2c_table:
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - id: next
        type: u4
      - type: u4
        if: _root.ver >= 0x00140400
        repeat: expr
        repeat-expr: 3
      - id: string_ptr
        type: u4
      - type: u4
        if: _root.ver < 0x00140400
      - id: ptr1
        type: u4
      - id: ptr2
        type: u4
      - id: ptr3
        type: u4
      - id: flags
        type: b32

  type_2d:
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - id: next
        type: u4
      - id: un4
        type: u4
        if: _root.ver >= 0x0140400
      - id: inst_ref_16x
        type: u4
        if: _root.ver < 0x0140400
      - type: u2
      - type: u2
      - type: u4
        if: _root.ver >= 0x0140400
      - id: flags
        type: b32
      - id: rotation
        type: u4
      - id: coord_x
        type: s4
      - id: coord_y
        type: s4
      - id: inst_ref
        type: u4
        if: _root.ver >= 0x0140400
      - id: ptr1
        type: u4
      - id: first_pad_ptr
        type: u4
      - id: ptr3
        type: u4
      - id: ptr4
        type: u4
        repeat: expr
        repeat-expr: 4
      # - id: group_assignment_ptr
      #   type: u4

  type_2e:
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - id: un
        type: u4
        repeat: expr
        repeat-expr: 7
      - id: un1
        type: u4
        if: _root.ver >= 0x00140400

  type_2f:
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - id: un
        type: u4
        repeat: expr
        repeat-expr: 6

  type_30_str_wrapper:
    seq:
      - id: type
        type: u1
      - id: layer
        type: layer_info
      - id: key
        type: u4
      - id: next
        type: u4
      - id: un4
        type: u4
        if: _root.ver >= 0x00140400
      - id: un5
        type: u4
        if: _root.ver >= 0x00140400
      - id: font
        type: text_properties
        if: _root.ver >= 0x00140400
      - id: ptr3
        type: u4
        if: _root.ver >= 0x00140400
      - id: un7
        type: u4
        if: _root.ver >= 0x00140900
      - id: str_graphic_ptr
        type: u4
      - id: un1
        type: u4
      - id: font_16x
        type: text_properties
        if: _root.ver < 0x00140400
      - id: ptr4
        type: u4
        if: _root.ver >= 0x00140400
      - id: coords
        type: s4
        repeat: expr
        repeat-expr: 2
      - id: un3
        type: u4
      - id: rotation
        type: u4
      - id: ptr3_16x
        type: u4
        if: _root.ver < 0x00140400

    enums:
      text_reversal:
        0x00: straight
        0x01: reversed

      text_aligmnent:
        0x01: left
        0x02: right
        0x03: center

    types:
      text_properties:
        seq:
          - id: key
            type: u1
          - id: flags
            type: b8
          - id: alignment
            type: u1
            enum: text_aligmnent
          - id: reversal
            type: u1
            enum: text_reversal

  type_31_sgraphic:
    seq:
      - id: t
        type: u1
      - id: layer
        type: u2
        enum: string_layer
      - id: key
        type: u4
      - id: str_graphic_wrapper_ptr
        type: u4
      - id: coords
        type: s4
        repeat: expr
        repeat-expr: 2
      - id: un
        type: u2
      - id: len
        type: u2
      - id: un2
        type: u4
        if: _root.ver >= 0x00140900
      - id: value
        type: string_aligned(len)

    enums:
      string_layer:
        0xF001: bot_text
        0xF101: top_text
        0xF609: bot_pin
        0xF709: top_pin
        0xF909: top_pin_label
        0xFA0D: bot_refdes
        0xFB0D: top_refdes

  type_32_placed_pad:
    seq:
      - id: type
        type: u1
      - id: layer
        type: layer_info
      - id: key
        type: u4
      - id: un1
        type: u4
      - id: net_ptr
        type: u4
      - id: flags
        type: b32
      - id: prev
        type: u4
        if: _root.ver >= 0x00140400
      - id: next
        type: u4
      - id: ptr3
        type: u4
      - id: ptr4
        type: u4
      - id: pad_ptr
        type: u4
      - id: ptr6
        type: u4
      - id: ptr7
        type: u4
      - id: ptr8
        type: u4
      - id: previous
        type: u4
      - id: un2
        type: u4
        if: _root.ver >= 0x00140400
      - id: ptr10
        type: u4
      - id: ptr11
        type: u4
      - id: coords
        type: s4
        repeat: expr
        repeat-expr: 4

  type_33_via:
    seq:
      - id: t
        type: u1
      - id: layer
        type: layer_info
      - id: key
        type: u4
      - id: un1
        type: u4
      - id: net_ptr
        type: u4
      - id: un2
        type: u4
      - id: un4
        type: u4
        if: _root.ver >= 0x00140400
      - id: ptr2
        type: u4
      - id: ptr7
        type: u4
        if: _root.ver >= 0x00140400
      - id: coords
        type: s4
        repeat: expr
        repeat-expr: 2
      - id: ptr3
        type: u4
      - id: ptr4
        type: u4
      - id: ptr5
        type: u4
      - id: ptr6
        type: u4
      - id: un3
        type: u4
        repeat: expr
        repeat-expr: 2
      - id: bb_coords
        type: s4
        repeat: expr
        repeat-expr: 4

  type_34_keepout:
    seq:
      - id: t
        type: u1
      - id: layer
        type: layer_info
      - id: key
        type: u4
      - id: next
        type: u4
      - id: ptr1
        type: u4
      - id: un2
        type: u4
        if: _root.ver >= 0x00140400
      - id: flags
        type: b32
      - id: ptr2
        type: u4
      - id: ptr3
        type: u4
      - id: un
        type: u4

  type_35:
    seq:
      - id: t2
        type: u1
      - id: t3
        type: u2
      - id: content
        type: u1
        repeat: expr
        repeat-expr: 120

  type_36:
    seq:
      - id: t
        type: u1
      - id: c
        type: u2
      - id: key
        type: u4
      - id: next
        type: u4
      - id: un1
        type: u4
        if: _root.ver >= 0x00140400
      - id: num_items
        type: u4
      - id: count
        type: u4
      - id: last_idx
        type: u4
      - id: un3
        type: u4
      - id: un2
        type: u4
        if: _root.ver >= 0x00140900
      - id: items
        type:
          switch-on: c
          cases:
            0x02: x02
            0x03: x03
            0x05: x05
            0x06: x06
            0x08: x08
            0x0b: x0b
            0x0c: x0c
            0x0d: x0d
            0x0f: x0f
            0x10: x10
        repeat: expr
        repeat-expr: num_items

    types:
      x02:
        seq:
          - id: str
            type: str
            size: 32
            encoding: ASCII
          - id: xs
            type: u4
            repeat: expr
            repeat-expr: 14
          - id: ys
            type: u4
            repeat: expr
            repeat-expr: 3
            if: _root.ver >= 0x00130C00
          - id: zs
            type: u4
            repeat: expr
            repeat-expr: 2
            if: _root.ver >= 0x00140400
      x03:
        seq:
          - id: str
            type: u1
            repeat: expr
            repeat-expr: 64
            # size: 64
            # encoding: ASCII
            if: _root.ver >= 0x00140400
          - id: str_16x
            type: u1
            repeat: expr
            repeat-expr: 32
            # size: 32
            # encoding: ASCII
            if: _root.ver < 0x00140400
          - id: un2
            type: u4
            if: _root.ver >= 0x00140900
      x05:
        seq:
          - id: unk
            type: u1
            repeat: expr
            repeat-expr: 28
      x06:
        seq:
          - id: n
            type: u2
          - id: r
            type: u1
          - id: s
            type: u1
          - id: un1
            type: u4
          - id: un2
            type: u4
            if: _root.ver < 0x00140400
            repeat: expr
            repeat-expr: 50
      x08:
        seq:
          - id: a
            type: u4
          - id: b
            type: u4
          - id: char_height
            type: u4
          - id: char_width
            type: u4
          - id: un2
            type: u4
            if: _root.ver >= 0x00140900
          - id: xs
            type: u4
            repeat: expr
            repeat-expr: 4
          - id: ys
            type: u4
            repeat: expr
            repeat-expr: 8
            if: _root.ver >= 0x00140400
      x0b:
        seq:
          - id: unk
            type: u1
            repeat: expr
            repeat-expr:  1016
      x0c:
        seq:
          - id: unk
            type: u1
            repeat: expr
            repeat-expr:  232
      x0d:
        seq:
          - id: unk
            type: u1
            repeat: expr
            repeat-expr:  200
      x0f:
        seq:
          - id: key
            type: u4
          - id: ptrs
            type: u4
            repeat: expr
            repeat-expr: 3
          - id: ptr2
            type: u4
      x10:
        seq:
          - id: unk
            type: u1
            repeat: expr
            repeat-expr: 108

  type_37:
    seq:
      - id: t
        type: u1
      - id: t2
        type: u2
      - id: key
        type: u4
      - id: ptr1
        type: u4
      - id: un2
        type: u4
      - id: capacity
        type: u4
      - id: count
        type: u4
      - id: un3
        type: u4
      - id: ptrs
        type: u4
        repeat: expr
        repeat-expr: 100
      - id: un4
        type: u4
        if: _root.ver >= 0x00140900

  type_38_film:
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - id: next
        type: u4
      - id: layer_list
        type: u4
      - id: film_name
        type: str
        size: 20
        encoding: ASCII
        if: _root.ver < 0x00131500
      - id: layer_name_str
        type: u4
        if: _root.ver >= 0x00131500
      - id: un2
        type: u4
        if: _root.ver >= 0x00131500
      - id: un1
        type: u4
        repeat: expr
        repeat-expr: 7
      - id: un3
        type: u4
        if: _root.ver >= 0x00140900

  type_39_film_layer_list:
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - id: parent
        type: u4
      - id: head
        type: u4
      - id: x
        type: u2
        repeat: expr
        repeat-expr: 22

  type_3a_film_list_node:
    seq:
      - id: t
        type: u1
      - id: layer
        type: layer_info
      - id: key
        type: u4
      - id: next
        type: u4
      - id: un
        type: u4
      - id: un1
        type: u4
        if: _root.ver >= 0x00140900

  type_3b:
    seq:
      - id: t
        type: u1
      - id: subtype
        type: u2
      - id: len
        type: u4
      - id: name
        type: str
        size: 128
        encoding: ASCII
        terminator: 0
      - id: type
        type: str
        size: 32
        encoding: ASCII
        terminator: 0
      - id: un1
        type: u4
      - id: un2
        type: u4
      - id: un3
        type: u4
        if: _root.ver >= 0x00140400
      - id: value
        type: string_aligned(len)

  type_3c:
    seq:
      - id: t
        type: u1
      - id: t2
        type: u2
      - id: key
        type: u4
      - id: un
        type: u4
        if: _root.ver >= 0x00140900
      - id: num_entries
        type: u4
      - id: entries
        type: u4
        repeat: expr
        repeat-expr: num_entries
