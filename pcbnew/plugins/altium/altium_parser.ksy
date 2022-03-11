# Can be viewed in: https://ide.kaitai.io/
#
# This file is a formal specification of the binary format used in Altium.
# Files need to manually extracted using a program which can read the Microsoft Compound File Format.
#
# While I do not create a parser using this file, it is still very helpful to understand the binary
# format.

meta:
  id: altium_binary
  endian: le
  encoding: ISO8859-1

seq:
# Only relevant for *.PcbLib files
#  - id: name
#    type: name
  - id: record
    type: record
    repeat: eos

# https://github.com/thesourcerer8/altium2kicad/blob/master/convertpcb.pl#L1291
types:
  name:
    seq:
    - id: sub1_len
      type: u4
    - id: data
      type: name_sub1
      size: sub1_len

  name_sub1:
    seq:
    - id: name_len
      type: u1
    - id: name
      size: name_len
      type: str

  record:
    seq:
      - id: recordtype
        type: u1
        enum: record_id
      - id: record
        type:
          switch-on: recordtype
          cases:
            record_id::arc6: arc
            record_id::pad6: pad
            record_id::via6: via
            record_id::track6: track
            record_id::text6: text
            record_id::fill6: fill
            record_id::region6: region
            record_id::componentbody6: componentbody

  arc:
    seq:
    - id: sub1_len
      type: u4
    - id: data
      type: arc_sub1(sub1_len)
      size: sub1_len

  arc_sub1:
    params:
      - id: sub1_len
        type: u4
    seq:
      - id: layer
        type: u1
      - #id: flags_u7
        type: b1
      - #id: flags_u6
        type: b1
      - #id: flags_u5
        type: b1
      - #id: flags_u4
        type: b1
      - #id: flags_u3
        type: b1
      - id: is_not_locked
        type: b1
      - id: is_not_polygonoutline
        type: b1
      - #id: flags_u0
        type: b1
      - id: is_keepout
        type: u1  # KEEPOUT = 2
      - id: net
        type: u2
      - id: subpolyindex
        type: u2
      - id: component
        type: u2
      - size: 4
      - id: center
        type: xy
      - id: radius
        type: u4
      - id: start_angle
        type: f8
      - id: end_angle
        type: f8
      - id: width
        type: u4
      - size: 11
      - id: keepout_restrictions
        type: keepout_restrictions
        if: sub1_len >= 57

  pad:
    seq:
    - id: sub1_len
      type: u4
    - id: designator
      type: pad_sub1
      size: sub1_len
    - id: sub2_len
      type: u4
    - size: sub2_len
    - id: sub3_len
      type: u4
    - size: sub3_len
    - id: sub4_len
      type: u4
    - size: sub4_len
    - id: sub5_len
      type: u4
    - id: size_and_shape
      type: pad_sub5
      size: sub5_len
    - id: sub6_len
      type: u4
    - id: size_and_shape_by_layer
      type: pad_sub6
      size: sub6_len
      if: sub6_len > 0

  pad_sub1:
    seq:
      - id: name_len  # = len-1?
        type: u1
      - id: name
        type: str
        size: name_len

  pad_sub5:
    seq:
      - id: layer  # $pos+23
        type: u1
        enum: layer
      - id: test_fab_top
        type: b1
      - id: tent_bottom
        type: b1
      - id: tent_top
        type: b1
      - #id: flags_u4
        type: b1
      - #id: flags_u3
        type: b1
      - id: is_not_locked
        type: b1
      - #id: flags_u1
        type: b1
      - #id: flags_u0
        type: b1
      - #id: flags2_u7
        type: b1
      - #id: flags2_u6
        type: b1
      - #id: flags2_u5
        type: b1
      - #id: flags2_u4
        type: b1
      - #id: flags2_u3
        type: b1
      - #id: flags2_u2
        type: b1
      - #id: flags2_u1
        type: b1
      - id: test_fab_bottom
        type: b1
      #- id: u
      #  size: 1
      - id: net  # $pos+26
        type: u2
      - size: 2
      - id: component  # $pos+30
        type: u2
      - size: 4
      - id: position  # $pos+36, $pos+40
        type: xy
      - id: topsize  # $pos+44, $pos+48
        type: xy
      - id: midsize  # $pos+52, $pos+56
        type: xy
      - id: botsize  # $pos+60, $pos+64
        type: xy
      - id: holesize  # $pos+68
        type: u4
      - id: topshape  # $pos+72
        type: u1
        enum: pad_shape
      - id: midshape  # $pos+73
        type: u1
        enum: pad_shape
      - id: botshape  # $pos+74
        type: u1
        enum: pad_shape
      - id: direction  # $pos+75
        type: f8
      - id: plated  # $pos+83
        type: u1
        enum: boolean
      - size: 1
      - id: pad_mode  # $pos+85
        type: u1
        enum: pad_mode
      - size: 5
      - id: ccw  # $pos+91
        type: u4
      - id: cen  # $pos+95
        type: u1
      - size: 1
      - id: cag  # $pos+97
        type: u4
      - id: cpr  # $pos+101
        type: u4
      - id: cpc  # $pos+105
        type: u4
      - id: pastemaskexpanionmanual
        type: s4
      - id: soldermaskexpansionmanual  # $pos+113
        type: s4
      - id: cpl  # $pos+117
        type: u1
      - size: 6
      - id: pastemaskexpansionmode  # $pos+124
        type: u1
        enum: pad_mode_rule
      - id: soldermaskexpansionmode  # $pos+125
        type: u1
        enum: pad_mode_rule
      - size: 3
      - id: holerotation  # $pos+129
        type: f8
      - size: 4
      - id: testpoint_assembly_top
        type: u1
        enum: boolean
      - id: testpoint_assembly_bottom
        type: u1
        enum: boolean

  pad_sub6:
    seq:
    - id: x
      type: s4
      repeat: expr
      repeat-expr: 29
    - id: y
      type: s4
      repeat: expr
      repeat-expr: 29
    - id: shape
      type: u1
      enum: pad_shape
      repeat: expr
      repeat-expr: 29
    - size: 1
    - id: hole_type
      type: u1
      enum: pad_hole_type
    - id: slot_length
      type: s4
    - id: slot_rotation
      type: f8
    - id: holeoffset_x
      type: s4
      repeat: expr
      repeat-expr: 32
    - id: holeoffset_y
      type: s4
      repeat: expr
      repeat-expr: 32
    - size: 1
    - id: shape_alt
      type: u1
      enum: pad_shape_alt
      repeat: expr
      repeat-expr: 32
    - id: corner_radius
      type: u1
      repeat: expr
      repeat-expr: 32
    - size: 32

  via:
    seq:
    - id: sub1_len
      type: u4
    - id: data
      type: via_sub1
      size: sub1_len

  via_sub1:
    seq:
      - size: 1
      - id: test_fab_top
        type: b1
      - id: tent_bottom
        type: b1
      - id: tent_top
        type: b1
      - #id: flags_u4
        type: b1
      - #id: flags_u3
        type: b1
      - id: is_not_locked
        type: b1
      - #id: flags_u1
        type: b1
      - #id: flags_u0
        type: b1
      - #id: flags2_u7
        type: b1
      - #id: flags2_u6
        type: b1
      - #id: flags2_u5
        type: b1
      - #id: flags2_u4
        type: b1
      - #id: flags2_u3
        type: b1
      - #id: flags2_u2
        type: b1
      - #id: flags2_u1
        type: b1
      - id: test_fab_bottom
        type: b1
      - id: net
        type: u2
      - size: 2
      - id: component
        type: u2
      - size: 4
      - id: pos  # 13
        type: xy
      - id: diameter # 21
        type: s4
      - id: holesize # 29
        type: s4
      - id: start_layer
        type: u1
        enum: layer
      - id: end_layer
        type: u1
        enum: layer
      - size: 43
      - id: via_mode
        type: u1
        enum: pad_mode
      - id: diameter_alt
        type: s4
        repeat: expr
        repeat-expr: 32

  track:
    seq:
    - id: sub1_len
      type: u4
    - id: data
      type: track_sub1(sub1_len)
      size: sub1_len

  track_sub1:
    params:
      - id: sub1_len
        type: u4
    seq:
      - id: layer
        type: u1
        enum: layer
      - #id: flags_u7
        type: b1
      - #id: flags_u6
        type: b1
      - #id: flags_u5
        type: b1
      - #id: flags_u4
        type: b1
      - #id: flags_u3
        type: b1
      - id: is_not_locked
        type: b1
      - id: is_not_polygonoutline
        type: b1
      - #id: flags_u0
        type: b1
      - id: is_keepout
        type: u1  # KEEPOUT = 2
      - id: net
        type: u2
      - id: subpolyindex
        type: u2
      - id: component
        type: u2
      - size: 4
      - id: start  # 13
        type: xy
      - id: end  # 21
        type: xy
      - id: width # 29
        type: s4
      - size: 12
      - id: keepout_restrictions
        type: keepout_restrictions
        if: sub1_len >= 46

  text:
    seq:
    - id: sub1_len
      type: u4
    - id: properties
      type: text_sub1
      size: sub1_len
    - id: sub2_len
      type: u4
    - id: text
      type: text_sub2
      size: sub2_len

  text_sub1:
    seq:
      - id: layer
        type: u1
      - #id: flags_u7
        type: b1
      - #id: flags_u6
        type: b1
      - #id: flags_u5
        type: b1
      - #id: flags_u4
        type: b1
      - #id: flags_u3
        type: b1
      - id: is_not_locked
        type: b1
      - #id: flags_u1
        type: b1
      - #id: flags_u0
        type: b1
      - size: 1
      - id: net
        type: u2
      - size: 2
      - id: component
        type: u2
      - size: 4
      - id: pos
        type: xy
      - id: height
        type: u4
      - id: font_name_id
        type: u1
      - size: 1
      - id: rotation
        type: f8
      - id: is_mirrored
        type: u1
        enum: boolean
      - id: strokewidth
        type: u4
      - id: is_comment
        type: u1
        enum: boolean
      - id: is_designator
        type: u1
        enum: boolean
      - size: 2
      - id: is_bold
        type: u1
        enum: boolean
      - id: is_italic
        type: u1
        enum: boolean
      - id: font_name
        size: 64
        type: str  # TODO: terminates with [0, 0]
        encoding: UTF-16
      - id: is_inverted
        type: u1
        enum: boolean
      - id: margin
        type: s4
      - id: use_offset  # use margin otherwise
        type: u1
        enum: boolean
      - size: 16
      - id: position
        type: u1
        enum: text_position
      - id: offset
        type: s4
      - id: barcode_full_size  # TODO: also for non-barcode?
        type: xy
      - id: barcode_margin  # TODO: also for non-barcode?
        type: xy
      - size: 4
      - id: barcode_type
        type: u1
        enum: text_barcode_type
      - size: 1
      - id: barcode_inverted
        type: u1
        enum: boolean
      - id: font_type
        type: u1
        enum: text_font_type
      - id: barcode_name
        size: 64
        type: str   # TODO: terminates with [0, 0]
        encoding: UTF-16
      - id: barcode_show_text
        type: u1
        enum: boolean

  text_sub2:
    seq:
      - id: len
        type: u1
      - id: name
        type: str
        size: len

  fill:
    seq:
    - id: sub1_len
      type: u4
    - id: data
      type: fill_sub1(sub1_len)
      size: sub1_len

  fill_sub1:
    params:
      - id: sub1_len
        type: u4
    seq:
      - id: layer
        type: u1
      - #id: flags_u7
        type: b1
      - #id: flags_u6
        type: b1
      - #id: flags_u5
        type: b1
      - #id: flags_u4
        type: b1
      - #id: flags_u3
        type: b1
      - id: is_not_locked
        type: b1
      - #id: flags_u1
        type: b1
      - #id: flags_u0
        type: b1
      - id: is_keepout
        type: u1  # KEEPOUT = 2
      - id: net
        type: u2
      - size: 2
      - id: component
        type: u2
      - size: 4
      - id: pos1
        type: xy
      - id: pos2
        type: xy
      - id: rotation
        type: f8
      - size: 9
      - id: keepout_restrictions
        type: keepout_restrictions
        if: sub1_len >= 47

  region:
    seq:
    - id: sub1_len
      type: u4
    - id: data
      type: region_sub1
      size: sub1_len

  region_sub1:
    seq:
    - id: layer
      type: u1
    - #id: flags_u7
      type: b1
    - #id: flags_u6
      type: b1
    - #id: flags_u5
      type: b1
    - #id: flags_u4
      type: b1
    - #id: flags_u3
      type: b1
    - id: is_not_locked
      type: b1
    - #id: flags_u1
      type: b1
    - #id: flags_u0
      type: b1
    - id: is_keepout
      type: u1  # KEEPOUT = 2
    - id: net
      type: u2
    - id: subpolyindex
      type: u2
    - id: component
      type: u2
    - size: 5
    - id: holecount  # TODO: check
      type: u2
    - size: 2
    - id: propterties_len
      type: u4
    - id: properties
      size: propterties_len
      type: str
    # region1 type
    - id: outline
      type: region1_part
    - id: holes
      type: region1_part
      repeat-expr: holecount
      repeat: expr
    # TODO
    #- id: vertices_num
    #  type: u4
    #- id: vertices2 # region2 type
    #  repeat: expr
    #  repeat-expr: vertices_num+1
    #  type: xyf2

  region1_part:
    seq:
    - id: vertices_num
      type: u4
    - id: vertices  # region1 type
      repeat: expr
      repeat-expr: vertices_num
      type: xyf

  componentbody:
    seq:
    - id: sub1_len
      type: u4
    - id: data
      type: componentbody_sub1
      size: sub1_len

  componentbody_sub1:
    seq:
    - size: 7
    - id: component
      type: u2
    - size: 9
    - id: propterties_len
      type: u4
    - id: properties
      size: propterties_len
      type: str

  keepout_restrictions:
    seq:
      - id: keepout_restriction_unknown  # not used
        type: b3
      - id: keepout_restriction_pth
        type: b1
      - id: keepout_restriction_smd
        type: b1
      - id: keepout_restriction_copper
        type: b1
      - id: keepout_restriction_track
        type: b1
      - id: keepout_restriction_via
        type: b1

  xy:
    seq:
      - id: x
        type: s4
      - id: y
        type: s4

  xyf:  # no idea why two different formats?
    seq:
      - id: x
        type: f8
      - id: y
        type: f8

  xyf2:  # no idea why two different formats?
    seq:
      - id: is_round
        type: u1
        enum: boolean
      - id: position
        type: xy
      - id: center
        type: xy
      - id: radius
        type: u4
      - id: angle1
        type: f8
      - id: angle2
        type: f8

enums:
  record_id:
    0x01: arc6
    0x02: pad6
    0x03: via6
    0x04: track6
    0x05: text6
    0x06: fill6
    0x0b: region6
    0x0c: componentbody6

  boolean:
    0: false
    1: true

  pad_shape:
    0: unknown
    1: circle
    2: rect
    3: octagonal

  pad_shape_alt:
    0: unknown
    1: round
    2: rect
    3: octagonal
    9: roundrectangle

  pad_hole_type:
    0: normal
    1: square
    2: slot

  pad_mode:
    0: simple
    1: top_middle_bottom
    2: full_stack

  pad_mode_rule:
    0: unknown
    1: rule
    2: manual

  text_position:
    1: left_top
    2: left_center
    3: left_bottom
    4: center_top
    5: center_center
    6: center_bottom
    7: right_top
    8: right_center
    9: right_bottom

  text_font_type:
    0: stroke
    1: truetype
    2: barcode

  text_barcode_type:
    0: code39
    1: code128

  layer:
    1: f_cu
    32: b_cu
