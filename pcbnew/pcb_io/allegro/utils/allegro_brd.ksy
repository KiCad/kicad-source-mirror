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
    doc: |
      Seems empty, even if there are many 0x04 objects in the file.
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
  - type: u4
    repeat: expr
    repeat-expr: 110
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
            doc: Seems small
          - id: b
            type: u4
            doc: Pointer?

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
      - id: lclass
        type: u1
        enum: layer_class
      - id: subclass
        type:
          switch-on: lclass
          cases:
            layer_class::board_geometry: subclass_board_geom
            layer_class::manufacturing: subclass_manufacturing
            layer_class::etch: u1
            layer_class::package_geometry: subclass_package_geom
            _: u1
    enums:
      layer_class:
        0x01: board_geometry
        0x06: etch
        0x07: manufacturing
        0x09: package_geometry
        0x0d: ref_des

      enum_package_geom:
        0xef: dfa_bounds_top
        0xf2: display_top
        0xf9: dimension
        0xfb: place_bounds_top
        0xfd: assembly_top

      enum_board_geom:
        0xf1: unknown

      enum_manufacturing:
        0xf0: unknown

    types:
      subclass_board_geom:
        seq:
          - id: sc
            type: u1
            enum: enum_board_geom

      subclass_manufacturing:
        seq:
          - id: sc
            type: u1
            enum: enum_manufacturing

      subclass_package_geom:
        seq:
          - id: sc
            type: u1
            enum: enum_package_geom

        # To identify:
        # pin
        #   soldermask_top/bottom
        #   pastemask_top
        #   filmmasktop
        #   top/bottom (etch?)
        # package_geom
        #    pin_number
        # board_geometry
        #   design_outline
        #   outline
        #   dimension
        # via_class
        #   soldermask_top/bottom
        #   top/bottom (etch layers?)
        # manufacturing
        #   xsection_chart
        #   nclegend-1-2
        # route_keepin
        #   all
        # package_keepin
        #   all

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

  coords:
    seq:
      - id: 'x'
        type: s4
      - id: 'y'
        type: s4

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
            0x15: type_15_16_17_segment
            0x16: type_15_16_17_segment
            0x17: type_15_16_17_segment
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
      - id: t1
        type: u1
      - id: t2
        type: u1
      - id: subtype
        type: u1
        doc: |
          Can be 0x00, 0x40
      - id: key
        type: u4
      - id: next
        type: u4
      - id: parent
        type: u4
      - id: unknown_1
        type: u4
        doc: |
          Can be 0x00, 0x20. Seems independent of subtype.
      - id: unknown_2
        type: u4
        if: _root.ver >= 0x00140400
      - id: width
        type: u4
      - id: coords_0
        type: coords
      - id: coords_1
        type: coords
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
      - id: unknown_1
        type: u4
        if: _root.ver >= 0x00140400
      - id: subtype
        type: u1
      - id: unknown_hdr
        type: u1
      - id: size
        type: u2
      - id: unknown_2
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
      - id: str
        type: u4
        doc: |
          String ID

          Examples:
            PreAmp: R_RES2012X50N_0805_510

      - id: str_2
        type: u4
        doc: |
          String ID

          Examples:
            PreAmp: RES2012X50N_0805
      - id: ptr_instance
        type: u4
        doc: |
          Points to an 0x07
      - id: ptr_fp
        type: u4
        doc: |
          Points to a 0x0F
      - id: ptr_x08
        type: u4
        doc: |
          Points to a 0x08. Reverse of the 0x06/0x08 pointer in 0x08, if it's an 0x06.
      - id: ptr_x03_symbol
        type: u4
        doc: |
          Points to a 0x03
      - id: unknown_1
        type: u4
        if: _root.ver >= 0x00140000

  type_07:
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - type: u4
      - id: ptr_1
        type: u4
        if: _root.ver >= 0x00140400
        doc: |
          Points to a 0x06.
      - id: unknown_1
        type: u4
        if: _root.ver >= 0x00140400
        doc: |
          Null?
      - id: unknown_2
        type: u4
        if: _root.ver >= 0x00140400
        doc: |
          Null?
      - id: ptr_0x2d
        type: u4
        doc: |
          Points to a 0x2D
      - id: unknown_3
        type: u4
        if: _root.ver < 0x00140400
      - id: ref_des_ref
        type: u4
        doc: |
          String ID of a reference designator.

          Examples: 'R1', 'TP3'
      - id: ptr_2
        type: u4
        doc: |
          Points to a 0x10.
          Reverse of the 0x07 pointer in 0x10.
      - id: ptr_3
        type: u4
        doc: |
          Points to a 0x03 or null.
      - id: un3
        type: u4
        doc: |
          Null so far.
      - id: ptr_4
        type: u4
        doc: |
          Points to a 0x32 or null.

  type_08:
    doc: |
      Seems to be a counterpart to 0x11 - the counts are the same and they have 1:1
      pointers to each other.

      They form some kind of chain with interlinks

          0x11 -> 0x11 -> 0x11 <-> 0x0F
            ^       ^       ^       ^
            v       v       v       v
          0x08 -> 0x08 -> 0x08 <-> 0x06

      From 17.2, there is a backlink too.
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - id: prev_ptr
        type: u4
        if: _root.ver >= 0x00140400
        doc: |
          Points to the previous 0x08 (i.e. the reverse of next_ptr)
      - id: str_ptr_16x
        type: u4
        if: _root.ver < 0x00140400
      - id: next_ptr
        type: u4
        doc: |
          Points to 0x06 or another 0x08.

          When the 0x11 counterpart has a pointer to 0x0F, this points to 0x06.
      - id: str_ptr
        type: u4
        if: _root.ver >= 0x00140400
        doc: |
          String ID

          Same ID as the 0x11 counterpart.
      - id: ptr3
        type: u4
        doc: |
          Points to 0x11. Seems to be the reverse of the 0x08 pointer in 0x11.
      - id: unknown_1
        type: u4
        if: _root.ver >= 0x00140400
      - id: ptr4
        type: u4
        doc: |
          Null?

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
      - id: str_unk
        type: u4
        doc: |
          String ID.
          Seems to always be 'Gn': G1, G7, etc.
      - id: s
        type: str
        size: 32
        encoding: ASCII
        doc: |
          Footprint name and value together. In the .alg, this is COMP_DEVICE_TYPE.
          E.g.:
          PreAmp: R_RES2012X50N_0805_510, R_RES2012X50N_0805_1K
      - id: ptr_x06
        type: u4
        doc:
          Points to a 0x06.
      - id: ptr_x11
        type: u4
        doc: |
          Points to a 0x11 (a comment here said it can be null?)
      - id: unknown_1
        type: u4
        doc: |
          Null?
      - id: unknown_2
        type: u4
        if: _root.ver >= 0x00140400
      - id: unknown_3
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
        doc: |
          Points to 0x07
      - type: u4
        if: _root.ver >= 0x00140900
      - id: ptr2
        type: u4
        doc: |
          Points to 0x12.
      - id: un1
        type: u4
      - id: str
        type: u4
        doc: |
          String id. Always seems to be Fn, where n is a number, which can be from 0
          to several hundred.

          Examples:
            PreAmp: F0, F1, ...
            CutiePi: F3, F4, ...
            BeagleBone-AI: F57, F55, ...

          These strings seem not to be in the .alg export
      - id: ptr4
        type: u4
        doc: |
          Points to 0x0F
      - id: path_str
        type: u4
        doc: |
          Points to 0x03 (not to a string ID)

          PreAmp: k=0xf7900e0 -> 0x03/0x68 @preampl_schem.schematic1(sch_1):page1_ins20635@discrete.\r.normal\(chips)

  type_11:
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - id: name
        type: u4
        doc: |
          String ID

          Examples:
            PreAmp: CATHODE, ANODE, 1, 2

          Could be a pin name.
      - id: ptr1
        type: u4
        doc: |
          Points to other 0x11s or 0x0F.

          In a "Group" (e.g. 1/2, C/A, B/C/E), one of them points to 0x0F and the others
          form a list ending in that one. So far the first in the file seems to be the 0x0F one.
      - id: ptr2
        type: u4
        doc: |
          Points to 0x08
      - id: unknown_1
        type: u4
        doc: |
          Always 0x00?
      - id: unknown_2
        type: u4
        if: _root.ver >= 0x00140900

  type_12:
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - id: next
        type: u4
      - id: ptr_0x11
        type: u4
      - id: pad_ptr
        type: u4
      - id: unknown_1
        type: u4
      - id: unknown_2
        type: u4
        if: _root.ver >= 0x00131000
      - id: unknown_3
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
        doc: |
          Points to the next item in the list.

          In the ll_x14 header list, this continues to the tail of that list.
          For items in footprint instances, this continues until the FP instance's key.
      - id: parent_ptr
        type: u4
        doc: |
          Points to 0x2d (parent FP). Can also point to the 'tail' of the ll_x14 list, even if next is another 0x14
          (the last entry has next == ptr1 == ll_x14.tail ).
      - id: unknown_1
        type: u4
        doc: |
          0x00 or 0x20 - flags?
      - id: un3
        type: u4
        if: _root.ver >= 0x00140400
      - id: segment_ptr
        type: u4
        doc: |
          Points to a 0x15 or 0x16 or 0x17.
      - id: ptr_0x03
        type: u4
        doc: |
          Null, or 0x03
      - id: ptr_0x26
        type: u4
        doc: |
          Null, or 0x26 (group?)

  type_15_16_17_segment:
    doc: |
      The difference between 15,16,17 seems to be:

      - 15: horizontal
      - 16: oblique
      - 17: vertical
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - id: next
        type: u4
        doc: |
          Can be 0x15, 0x16, 0x17, 0x05
      - id: parent_ptr
        type: u4
        doc: |
          Can be 0x05, 0x14, 0x28
      - id: flags
        type: u4
        doc: |
          0x00 or 0x20 - flags?
      - id: unknown_1
        type: u4
        if: _root.ver >= 0x00140400
      - id: width
        type: u4
      - id: coords_0
        type: coords
      - id: coords_1
        type: coords

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
      - id: unknown_1
        type: u4
      - id: unknown_2
        type: u4
      - id: pad_path
        type: u4
      - id: unknown_3
        type: u4
        if: _root.ver < 0x00140400
      - id: unknown_4
        type: u4
        if: _root.ver < 0x00140400
      - id: unknown_5
        type: u4
        if: _root.ver < 0x00140400
      - id: unknown_6
        type: u4
        if: _root.ver < 0x00140400
      - id: pad_info
        type: pad_info
      - id: unknown_7
        type: u4
        if: _root.ver >= 0x00140400
      - id: unknown_8
        type: u4
        if: _root.ver >= 0x00140400
      - id: unknown_9
        type: u4
        if: _root.ver >= 0x00140400
      - id: unknown_10
        type: u2
        if: _root.ver < 0x00140400
      - id: layer_count
        type: u2
      - id: unknown_11
        type: u2
        if: _root.ver >= 0x00140400
      - id: unknown_arr8
        type: u4
        repeat: expr
        repeat-expr: 8
      - id: unknown_arr28
        type: u4
        repeat: expr
        repeat-expr: 28
        if: _root.ver >= 0x00140400
      - id: unknown_arr8_2
        type: u4
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

              Can also be 0x00
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
        if: _root.ver >= 0x00130C00 # unsure, but in Kinoma, not in Preamp
      - id: un3
        type: u2
        if: _root.ver >= 0x00130C00 # unsure, but in Kinoma, not in Preamp
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
        doc: |
          In PreAmp, all rectangles set this to 0x09a272d4, which is the tail of the x24_x28
          linked list.
      - id: unknown_1
        type: u4
      - id: unknown_2
        type: u4
        if: _root.ver >= 0x00140400
      - id: coords_0
        type: coords
      - id: coords_1
        type: coords
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
      - id: parent
        type: u4
        doc: |
          As for 0x24, points to the tail of the x24_x28 linked list for
          top level items, or the footprint if a child item.
      - id: unknown_1
        type: u4
        doc: Always 0x00 so far.
      - id: unknown_2
        type: u4
        if: _root.ver >= 0x00140400
      - id: unknown_3
        type: u4
        if: _root.ver >= 0x00140400
      - id: ptr2
        type: u4
        doc: Always 0x00 so far.
      - id: ptr3
        type: u4
        doc: Always 0x00 so far.
      - id: ptr4
        type: u4
        doc: Always 0x00 so far.
      - id: first_segment_ptr
        type: u4
        doc: points to 0x16 or 0x17 segments
      - id: unknown_4
        type: u4
      - id: unknown_5
        type: u4
      - id: ptr5
        type: u4
        if: _root.ver >= 0x00140400
      - id: ptr6
        type: u4
        doc: |
          Points to 0x03, which is of a a text subtype.

          Examples:
            CutiePi: 0x28 k=0xc0c0558 -> 0x03 k=0x0c0c07d0, subtype=0x68 = 'CLIP_1'

          This string doesn't seem to get exported in the .alg file, so maybe it's not useful
          (or maybe Altium is dropping it on the floor)
      - id: ptr7_16x
        type: u4
        if: _root.ver < 0x00140400
        doc: Always 0x00 so far.
      # Suspect this is a bounding box
      - id: coords_0
        type: coords
      - id: coords_1
        type: coords

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
      - id: unknown_1
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
          - id: name
            type: string_aligned(36)
            doc: |
              String, zero terminated.
              There doesn't seem to be any other data in here.
      ref_entry:
        seq:
          - id: ptr
            type: u4
            doc: |
              String ID - name
              E.g. 'DIMENSIONS', 'NCLEGEND-1-1'
          - id: properties
            type: u4
            doc: |
              Some kind of mask:
               E.g. TOP:        0x088001 (PreAmp)
                    BOTTOM:     0x108002 (PreAmp)

                    CutiePi copper layers;
                        TOP:            0x088001
                        GND02/05:       0x000103
                        L03:            0x008003
                        L04:            0x008003
                        BOTTOM:         0x108002

                    BeagleBone-AI:
                        TOP:            0x088000
                        GND_L2/4/9/11:  0x000700
                        INT_L3:         0x008600
                        SIG_L5/8/10:    0x008600
                        PWR_L6/7:       0x000700
                        BOTTOM:         0x108000

                    DIMENSIONS: 0x000000 (PreAmp)
                    IMGOLD, IPC_L_03: 0 (CutiePi)
                    GND02       0x000103 (CutiePi)




          - id: un1
            type: u4
            doc: |
              Always seems to be 0

  type_2b:
    seq:
      - type: u1
      - type: u2
      - id: key
        type: u4
      - id: fp_str_ref
        type: u4
        doc: |
          E.g. RES2012X50N_0805
      - id: unknown_1
        type: u4
        doc: |
          E.g. 0x00000000
      - id: coords0
        type: coords
      - id: coords1
        type: coords
      - id: next
        type: u4
      - id: first_inst_ptr
        type: u4
      - id: ptr_2
        type: u4
        doc: Points to 0x0d
      - id: ptr_3
        type: u4
        doc: |
          Points to a 0x32 (pad?)
      - id: ptr_4
        type: u4
        doc: |
          Points to a 0x14 (segment - list of graphics?)
      - id: str_ptr1
        type: u4
        doc: |
          Points to a 0x03
      - id: ptr_5
        type: u4
        doc: |
          Points to a 0x28 - courtyard?
      - id: ptr_6
        type: u4
        doc: |
          Null?
      - id: ptr_7
        type: u4
        doc: |
          Null?

      - id: unknown_2
        type: u4
        if: _root.ver >= 0x00130C00
      - id: unknown_3
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
      - id: unknown_1
        type: u4
        if: _root.ver >= 0x00140400
      - id: unknown_2
        type: u4
        if: _root.ver >= 0x00140400
      - id: unknown_3
        type: u4
        if: _root.ver >= 0x00140400
      - id: subclass_str
        type: u4
        doc: |
          String ID for the table SUBCLASS output in the .alg from AllegroGeometryView.txt
          E.g.:
            PreAmp: XSECTION_CHART, DRILL_LEGEND_1_2

          Can be null.
      - id: unknown_4
        type: u4
        if: _root.ver < 0x00140400
      - id: ptr_1
        type: u4
        doc: |
          Points to a 0x37 or 0x3c
      - id: ptr_2
        type: u4
        doc: |
          Null?
      - id: ptr_3
        type: u4
        doc: |
          Null?
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
        doc: |
          Points to a chain of 0x2d and finally 0x2b
      - id: unknown_1
        type: u4
        if: _root.ver >= 0x0140400
      - id: inst_ref_16x
        type: u4
        if: _root.ver < 0x0140400
        doc: |
          Points to a 0x07
      - id: unknown_2
        type: u2
        doc:
          E.g. 0x0020
      - id: unknown_3
        type: u2
        doc: |
          E.g. 0x2000
      - id: unknown_4
        type: u4
        if: _root.ver >= 0x0140400
      - id: flags
        type: b32
        doc: |
          E.g. 0x20000000
      - id: rotation
        type: u4
        doc: |
          In millidegrees: 90000 = 90 degrees
      - id: coords
        type: coords
      - id: inst_ref
        type: u4
        if: _root.ver >= 0x0140400
      - id: ptr_1
        type: u4
        doc: |
          Points to a 0x14
      - id: first_pad_ptr
        type: u4
        doc: |
          Points to a 0x32
      - id: ptr_2
        type: u4
        doc: |
          Points to a 0x30
      - id: ptr_3
        type: u4
        doc: |
          Null?
      - id: ptr_4
        type: u4
        doc: |
          Points to a 0x28 (shape? - courtyard?)
      - id: ptr_5
        type: u4
        doc: |
          Null?
      - id: ptr_6
        type: u4
        doc: |
          Null?
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
      - id: unknown_1
        type: u4
        if: _root.ver >= 0x00140400
      - id: unknown_2
        type: u4
        if: _root.ver >= 0x00140400
      - id: font
        type: text_properties
        if: _root.ver >= 0x00140400
      - id: ptr3
        type: u4
        if: _root.ver >= 0x00140400
      - id: unknown_3
        type: u4
        if: _root.ver >= 0x00140900
      - id: str_graphic_ptr
        type: u4
      - id: unknown_4
        type: u4
      - id: font_16x
        type: text_properties
        if: _root.ver < 0x00140400
      - id: ptr4
        type: u4
        if: _root.ver >= 0x00140400
      - id: coords
        type: coords
      - id: unknown_5
        type: u4
      - id: rotation
        type: u4
      - id: ptr3_16x
        type: u4
        if: _root.ver < 0x00140400
        doc: |
          Can be null.

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
        type: layer_info
      - id: key
        type: u4
      - id: str_graphic_wrapper_ptr
        type: u4
      - id: coords
        type: coords
      - id: unknown_1
        type: u2
      - id: len
        type: u2
      - id: unknown_2
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
      - id: unknown_1
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
      - id: coords_0
        type: coords
      - id: coords_1
        type: coords

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
          - id: unknown_1
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
      - id: ptr_1
        type: u4
        doc: |
          Points back to 0x2c (table)
      - id: unknown_1
        type: u4
      - id: capacity
        type: u4
      - id: count
        type: u4
      - id: unknown_2
        type: u4
      - id: ptrs
        type: u4
        repeat: expr
        repeat-expr: capacity
        doc: |
          List of:
            - 0x14 - segment
            - 0x30 - text
            - 0x0c - ?
            - 0x0e - ?
            - 0x24 - rectangle
      - id: unknown_3
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
      - id: unknown_1
        type: u4
        if: _root.ver >= 0x00131500
      - id: unknown_2
        type: u4
        repeat: expr
        repeat-expr: 7
      - id: unknown_3
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
      - id: unknown_1
        type: u4
        doc: |
          Null?
      - id: unknown_2
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
      - id: unknown_1
        type: u4
        if: _root.ver >= 0x00140900
      - id: num_entries
        type: u4
      - id: entries
        type: u4
        repeat: expr
        repeat-expr: num_entries
        doc: |
          Entries can be:
            - null
            - 0x32
            - 0x1b
            - 0x24
