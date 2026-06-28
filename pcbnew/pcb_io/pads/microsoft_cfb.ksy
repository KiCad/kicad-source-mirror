# Microsoft Compound File Binary (CFB) container.
# Derived from the Kaitai Struct formats repository's CC0-1.0 definition.
meta:
  id: microsoft_cfb
  title: Microsoft Compound File Binary (CFB / OLE)
  license: CC0-1.0
  endian: le

seq:
  - id: header
    type: cfb_header

instances:
  sector_size:
    value: '1 << header.sector_shift'
  fat:
    pos: sector_size
    size: header.size_fat * sector_size
    type: fat_entries
  directory:
    pos: (header.ofs_dir + 1) * sector_size
    type: dir_entry

types:
  cfb_header:
    seq:
      - id: signature
        contents: [0xd0, 0xcf, 0x11, 0xe0, 0xa1, 0xb1, 0x1a, 0xe1]
      - id: clsid_padding
        contents: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
        doc: reserved root class ID; required zero by the CFB specification
      - id: version_minor
        type: u2
      - id: version_major
        type: u2
      - id: byte_order
        contents: [0xfe, 0xff]
      - id: sector_shift
        type: u2
        doc: 9 for 512-byte version-3 sectors; 12 for 4096-byte version-4 sectors
      - id: mini_sector_shift
        type: u2
      - id: header_padding
        contents: [0, 0, 0, 0, 0, 0]
        doc: CFB header padding; required zero by the CFB specification
      - id: size_dir
        type: s4
        doc: directory sector count; zero for major version 3
      - id: size_fat
        type: s4
        doc: FAT sector count
      - id: ofs_dir
        type: s4
        doc: first directory-stream sector
      - id: transaction_seq
        type: s4
      - id: mini_stream_cutoff_size
        type: s4
      - id: ofs_mini_fat
        type: s4
      - id: size_mini_fat
        type: s4
      - id: ofs_difat
        type: s4
      - id: size_difat
        type: s4
      - id: difat
        type: s4
        repeat: expr
        repeat-expr: 109
        doc: header DIFAT entries naming FAT sectors or carrying the free-sector marker

  fat_entries:
    seq:
      - id: entries
        type: s4
        repeat: eos
        doc: sector-allocation chain entries

  dir_entry:
    seq:
      - id: name
        type: str
        size: 64
        encoding: UTF-16LE
      - id: name_len
        type: u2
      - id: object_type
        type: u1
        enum: obj_type
      - id: color_flag
        type: u1
        enum: rb_color
      - id: left_sibling_id
        type: s4
      - id: right_sibling_id
        type: s4
      - id: child_id
        type: s4
      - id: clsid
        size: 16
        doc: storage object's class ID
      - id: state
        type: u4
        doc: user-defined storage state flags
      - id: time_create
        type: u8
        doc: Windows FILETIME creation timestamp
      - id: time_mod
        type: u8
        doc: Windows FILETIME modification timestamp
      - id: ofs
        type: s4
        doc: first stream sector, or the root storage's first mini-stream sector
      - id: size
        type: u8
        doc: stream or root mini-stream byte length
    instances:
      mini_stream:
        io: _root._io
        pos: (ofs + 1) * _root.sector_size
        size: size
        if: object_type == obj_type::root_storage
      child:
        io: _root._io
        pos: (_root.header.ofs_dir + 1) * _root.sector_size + child_id * 0x80
        type: dir_entry
        if: child_id != -1
      left_sibling:
        io: _root._io
        pos: (_root.header.ofs_dir + 1) * _root.sector_size + left_sibling_id * 0x80
        type: dir_entry
        if: left_sibling_id != -1
      right_sibling:
        io: _root._io
        pos: (_root.header.ofs_dir + 1) * _root.sector_size + right_sibling_id * 0x80
        type: dir_entry
        if: right_sibling_id != -1
    enums:
      obj_type:
        0: unallocated
        1: storage
        2: stream
        5: root_storage
      rb_color:
        0: red
        1: black
