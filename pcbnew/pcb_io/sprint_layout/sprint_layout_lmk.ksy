meta:
  id: sprint_layout_lmk
  endian: le

seq:
  - id: version
    type: u1
    valid:
      min: 0
      max: 6
  - id: magic
    contents: [0x33, 0xaa, 0xff]
  - id: num_objects
    type: u4
  - id: objects
    type: sl_obj(false)
    repeat: expr
    repeat-expr: num_objects

types:
  sl_obj:
    params:
      - id: is_text_child
        type: bool
    seq:
      - id: type
        type: u1
        enum: object_type
      - id: x
        type: f4
      - id: y
        type: f4
      - id: outer
        type: f4
      - id: inner
        type: f4
      - id: line_width
        type: u4
      - id: padding_1
        size: 1
      - id: layer
        type: u1
      - id: tht_shape
        type: u1
      - id: padding_2
        size: 4
      - id: component_id
        type: u2
      - id: selected
        type: u1
      - id: start_angle
        type: s4
      - id: unknown_1
        size: 5
      - id: filled
        type: u1
      - id: clearance
        type: s4
      - id: unknown_2
        size: 5
      - id: thermal_width
        type: u1
      - id: mirror
        type: u1
      - id: keepout
        type: u1
      - id: rotation
        type: s4
      - id: plated
        type: u1
      - id: soldermask
        type: u1
      - id: unknown_3
        size: 18
      - id: text
        type: var_string
        if: not is_text_child
      - id: net_name
        type: var_string
        if: not is_text_child
      - id: group_count
        type: u4
        if: not is_text_child
      - id: groups
        type: u4
        repeat: expr
        repeat-expr: group_count
        if: not is_text_child
      - id: body
        type:
          switch-on: type
          cases:
            'object_type::obj_circle': circle_body
            'object_type::obj_text': text_body
            _: points_body

  circle_body: {}

  text_body:
    seq:
      - id: child_count
        type: u4
      - id: text_children
        type: sl_obj(true)
        repeat: expr
        repeat-expr: child_count
      - id: component
        type: component_data
        if: (_parent.tht_shape == 1) and (_root.version >= 6)

  points_body:
    seq:
      - id: point_count
        type: u4
      - id: points
        type: point
        repeat: expr
        repeat-expr: point_count

  component_data:
    seq:
      - id: off_x
        type: f4
      - id: off_y
        type: f4
      - id: center_mode
        type: u1
      - id: rotation
        type: f8
      - id: package
        type: var_string
      - id: comment
        type: var_string
      - id: use
        type: u1

  point:
    seq:
      - id: x
        type: f4
      - id: y
        type: f4

  fixed_string:
    params:
      - id: fixed_len
        type: u4
    seq:
      - id: valid_len
        type: u1
      - id: str
        type: str
        size: fixed_len
        encoding: cp1251

  var_string:
    seq:
      - id: len
        type: u4
      - id: str
        type: str
        size: len
        encoding: cp1251

enums:
  object_type:
    2: obj_tht_pad
    4: obj_poly
    5: obj_circle
    6: obj_line
    7: obj_text
    8: obj_smd_pad