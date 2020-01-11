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
  encoding: UTF-8

seq:
  - id: pad
    type: record
    repeat: eos

# https://github.com/thesourcerer8/altium2kicad/blob/master/convertpcb.pl#L1291
types:
  record:
    seq:
      - id: recordtype
        type: u1
        enum: record_id
      - id: pad
        type: pad
        if: recordtype == record_id::pad6
      - id: track
        type: track
        size: 49
        if: recordtype == record_id::track6

  pad:
    seq:
      - id: len  # TODO: what len?
        type: u4
      - id: name_len  # = len-1?
        type: u1
      - id: name
        type: str
        size: name_len
      - size: 19
      - id: some_len
        type: u1
      - size: 3
      - id: altlayer  # $pos+23
        type: u1
        enum: layer
      - size: 2
      - id: onet  # $pos+26
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
      - id: cpe  # $pos+109  - pastemaskexpanionmanual?
        type: u4
      - id: cse  # $pos+113
        type: u4
      - id: cpl  # $pos+117
        type: u1
      - size: 6
      - id: pastemaskexpansionmode  # $pos+124 - cpev?
        type: u1
      - id: soldermaskexpansion  # $pos+125 - csev?
        type: u1
        enum: pad_soldermaskexpansion
      - size: 3
      - id: holerotation  # $pos+129
        type: f8
      - size: 4
        if: some_len == 114
      - id: tolayer
        type: u1
        enum: layer
        if: some_len == 120
      - size: 2
        if: some_len == 120
      - id: fromlayer
        type: u1
        enum: layer
        if: some_len == 120
      - size: 2
        if: some_len == 120
      - id: last_section_length
        type: u4
        if: some_len > 106
      - size: 53
        if: some_len == 171
      - id: record_len
        type: u4
        if: some_len == 171
      - size: record_len
        if: some_len == 171

  track:
    seq:
      - size: 4
      - id: layer
        type: u1
        enum: layer
      - size: 12
      - id: start  # 13
        type: xy
      - id: end  # 21
        type: xy
      - id: width # 29
        type: s4

  xy:
    seq:
      - id: x
        type: s4
      - id: y
        type: s4

enums:
  record_id:
    0x02: pad6
    0x04: track6

  boolean:
    0: false
    1: true

  pad_shape:
    0: unknown
    1: circle
    2: rect
    3: oval

  pad_shape_alt:  # TODO: for what?
    0: round2
    1: round1
    2: rectangle
    3: octagonal
    4: thermalrelief
    6: point0
    7: point1
    9: roundrectangle

  pad_mode:
    0: simple
    1: top_middle_bottom
    2: full_stack

  pad_soldermaskexpansion:
    1: rule
    2: manual

  layer:
    1: f_cu
    32: b_cu
