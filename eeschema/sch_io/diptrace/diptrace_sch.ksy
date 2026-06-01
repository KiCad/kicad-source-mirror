# Can be viewed in: https://ide.kaitai.io/
#
# Reverse-engineered DipTrace schematic binary format (.dch).
# This spec mirrors the section order driven by diptrace_sch_parser.cpp::Parse():
#   parseHeader -> parseSheetDefinitions -> parseDisplaySettings -> parseTextStyles
#   -> parsePreComponentSettings -> (components) -> parseBusSection -> parseNetSection
#   -> parseWireSection.
#
# The leading sections (header, sheet definitions, display settings, text styles,
# pre-component settings) are fully deterministic and are modelled in the top-level
# seq.  After that point the importer no longer reads the stream linearly:
#
#   * The component records are located heuristically.  parseComponents() first tries
#     a count-guided linear walk and, on failure, falls back to scanComponentBoundaries()
#     (a bbox + 5-string pattern scan).  The component type below describes ONE record
#     exactly as parseOneComponent() decodes it, but the top-level stream cannot express
#     a clean repeat, so the component / bus / net / wire regions are kept raw at the
#     top level and only the record types are provided.
#
#   * The bus section is located by findBusSection() (marker search).  parseBusSection()
#     then reads it deterministically; bus_section / bus_entry describe it.
#
#   * The net section (parseNetSection) is a forward marker scan (0x0F 0x42 0x3F) for net
#     labels; net_label_scan_record describes one match.
#
#   * The wire section (parseWireSection, v38+ only) is internally deterministic per net
#     record but nets are separated by a variable preamble the importer skips via an
#     isPlausibleNetName() forward scan, so a clean top-level repeat is not expressible.
#     net_wire_record / wire describe one net's worth of structured wire data.

meta:
  id: diptrace_sch
  endian: be
  encoding: UTF-8

seq:
  - id: header
    type: header
  - id: sheet_defs
    type: sheet_def(header.version_value)
    repeat: expr
    repeat-expr: header.num_sheets.value
  - id: display_settings
    type: display_settings(header.version_value)
  - id: text_styles
    type: text_styles(header.version_value)
  - id: pre_component_settings
    type: pre_component_settings(header.version_value)
  - id: component_bus_net_wire_sections
    doc: |
      Remaining bytes covering the component records, bus section, net-label scan
      region, and net/wire section.  These are boundary-scanned by the importer
      (see file-level doc); the record types component, bus_section,
      net_label_scan_record, and net_wire_record describe their structures.
    size-eos: true

types:
  # --- Primitives (identical encoding to diptrace_pcb.ksy) --------------------

  int3:
    doc: 3-byte unsigned big-endian integer biased by 1,000,000.
    seq:
      - id: byte0
        type: u1
      - id: byte1
        type: u1
      - id: byte2
        type: u1
    instances:
      raw:
        value: byte0 * 65536 + byte1 * 256 + byte2
      value:
        value: raw - 1000000

  int4:
    doc: 4-byte unsigned big-endian integer biased by 1,000,000,000.
    seq:
      - id: raw
        type: u4
    instances:
      value:
        value: raw - 1000000000

  dt_string:
    doc: |
      Version-dependent DipTrace string.
      version <= 37: int3 byte-count + raw ASCII bytes.
      version  > 37: u2 char-count + UTF-16-BE data (char-count * 2 bytes).
    params:
      - id: version
        type: s4
    seq:
      - id: len_ascii
        type: int3
        if: version <= 37
      - id: text_ascii
        type: str
        size: len_ascii.value
        encoding: ASCII
        if: version <= 37
      - id: len_utf16
        type: u2
        if: version > 37
      - id: text_utf16
        type: str
        size: len_utf16 * 2
        encoding: UTF-16BE
        if: version > 37

  rgb_color:
    seq:
      - id: red
        type: u1
      - id: green
        type: u1
      - id: blue
        type: u1

  # --- Section 1: header (parseHeader, lines ~192-267) ------------------------

  header:
    doc: |
      File magic and document header.  The modern (magic_len == 7) form embeds the
      version as an int3.  The legacy magic_len == 11 form encodes the version inside
      the "DTSCHEMx.yy" magic suffix (parsed in C++ from the ASCII suffix bytes); this
      spec models the modern 7-byte case as primary and exposes the legacy suffix bytes
      as magic_suffix for inspection.  version_value resolves the effective version for
      downstream dt_string / version gates (defaulting the legacy case to 33, matching
      the parser's parsedMinor fallback).
    seq:
      - id: magic_len
        type: u1
        valid:
          any-of: [7, 11]
      - id: magic
        contents: [0x44, 0x54, 0x53, 0x43, 0x48, 0x45, 0x4d] # "DTSCHEM"
      - id: magic_suffix
        size: magic_len - 7
        doc: Legacy "x.yy" version suffix; empty for the modern 7-byte magic.
      - id: version
        type: int3
        if: magic_len == 7
      - id: field_0b
        type: int4
      - id: field_0f
        type: int3
      - id: field_12
        type: int3
      - id: field_15
        type: int3
      - id: num_sheets
        type: int3
    instances:
      version_value:
        value: 'magic_len == 7 ? version.value : 33'

  # --- Section 2: sheet definitions (parseSheetDefinitions, lines ~270-279) ---

  sheet_def:
    params:
      - id: version
        type: s4
    seq:
      - id: name
        type: dt_string(version)
      - id: field_a
        type: int3

  # --- Section 3: display settings (parseDisplaySettings, lines ~282-313) -----

  display_settings:
    doc: |
      Global display/grid settings block.  Layout differs below V31_CUTOVER (34):
      pre-34 has two trailing int3 fields, while 34+ has a u4 raw char-count that may
      introduce an optional UTF-16 payload skipped when 0 < extra_chars < 1000.
    params:
      - id: version
        type: s4
    seq:
      - id: fields_a
        type: int3
        repeat: expr
        repeat-expr: 5
      - id: flag_a
        type: u1
      - id: field_b
        type: int4
      - id: field_c
        type: int4
      - id: legacy_field_a
        type: int3
        if: version < 34
      - id: legacy_field_b
        type: int3
        if: version < 34
      - id: extra_chars
        type: u4
        if: version >= 34
        doc: Raw char count for an optional modern UTF-16 payload (e.g. "url").
      - id: extra_payload
        size: extra_chars * 2
        if: 'version >= 34 and extra_chars > 0 and extra_chars < 1000'
      - id: flag_b
        type: u1
      - id: field_d
        type: int4
      - id: field_e
        type: int4

  # --- Section 4: text styles (parseTextStyles, lines ~316-344) ---------------

  text_styles:
    doc: |
      Text-style table.  Absent in DTSCHEM1.x legacy files (m_magicMajor == 1), which
      this spec does not model (modern files use magic_major == 2).
    params:
      - id: version
        type: s4
    seq:
      - id: num_styles
        type: int3
      - id: styles
        type: text_style(version)
        repeat: expr
        repeat-expr: num_styles.value

  text_style:
    params:
      - id: version
        type: s4
    seq:
      - id: name
        type: dt_string(version)
      - id: field_a
        type: int3
      - id: field_b
        type: int4
      - id: field_c
        type: int4
      - id: trailer
        type: int3
        if: version > 37
        doc: |
          Per-style trailing int3 present only for v38+ (UTF-16) files.  The v46 fix:
          in single-style legacy files this byte was historically consumed as the
          leading pad int3 of pre_component_settings instead.

  # --- Section 5: pre-component settings (parsePreComponentSettings, ~347-396)-

  pre_component_settings:
    doc: |
      Settings block immediately preceding the component records.  Models the modern
      (magic_major == 2) branch.  For v37 and below (no per-style trailer in
      text_styles) a leading pad int3 precedes the component count; for v38+ the
      component count is the first int3 here.
    params:
      - id: version
        type: s4
    seq:
      - id: leading_pad
        type: int3
        if: version <= 37
      - id: component_count
        type: int3
      - id: flag_a
        type: u1
      - id: flag_b
        type: u1
      - id: fields_a
        type: int3
        repeat: expr
        repeat-expr: 5
      - id: field_b
        type: int3
      - id: field_c
        type: int3

  # --- Section 6: component record (parseOneComponent, lines ~759-1150) -------

  component:
    doc: |
      One component record as decoded by parseOneComponent().  The importer locates
      these heuristically (count-guided walk with a scanComponentBoundaries() fallback),
      so this type cannot appear as a clean top-level repeat; it describes a single
      record.  Several tail fields use byte-level fallbacks in the C++ (tailless-fallback,
      2-byte pin separator) that a static grammar cannot branch on; the common path is
      modelled and the divergent bytes are noted.
    params:
      - id: version
        type: s4
    seq:
      - id: bbox_x1
        type: int4
        doc: Placement posX (first two int4) / size (last two) per parseOneComponent.
      - id: bbox_y1
        type: int4
      - id: bbox_x2
        type: int4
      - id: bbox_y2
        type: int4
      - id: comp_name
        type: dt_string(version)
      - id: refdes
        type: dt_string(version)
      - id: value
        type: dt_string(version)
      - id: prefix
        type: dt_string(version)
      - id: name_dup
        type: dt_string(version)
      - id: post_a
        type: int3
      - id: post_b
        type: int3
      - id: flag1
        type: u1
      - id: post_c
        type: int3
      - id: post_d
        type: int3
      - id: part_name
        type: dt_string(version)
      - id: part_number
        type: dt_string(version)
      - id: multipart_flag
        type: u1
        doc: 1 means multi-part (part_id string follows the part bbox).
      - id: sheet_index
        type: int3
      - id: part_field_b
        type: int3
      - id: part_field_c
        type: int3
      - id: part_bbox_x1
        type: int4
      - id: part_bbox_y1
        type: int4
      - id: part_bbox_x2
        type: int4
      - id: part_bbox_y2
        type: int4
      - id: part_id
        type: dt_string(version)
        if: multipart_flag == 1
        doc: Present only for multi-part components.
      - id: single_part_tail
        type: int3
        if: 'multipart_flag != 1 and version < 34'
        doc: |
          Legacy (<34) single-part discriminator int3.  The C++ may instead re-read this
          region as a length-prefixed ASCII token (connector family) when the int3 looks
          like a small ASCII length; that ambiguous re-read is not statically expressible.
      - id: single_part_tail_modern
        size: 2
        if: 'multipart_flag != 1 and version >= 34'
        doc: Modern (34+) single-part records skip two bytes here.
      - id: field_d
        type: int3
      - id: field_e
        type: int3
        doc: When 1..999, field_e count of string-triples follows (field_e_entries).
      - id: field_e_entries
        type: field_e_entry(version)
        repeat: expr
        repeat-expr: 'field_e.value >= 1 and field_e.value < 1000 ? field_e.value : 0'
      - id: field_f
        type: int3
      - id: field_g
        type: int3
      - id: byte4
        type: u1
      - id: lib_id
        type: int4
      - id: lib_path
        type: dt_string(version)
      - id: tail_a
        type: int3
      - id: tail_b
        type: int3
      - id: tail_str_a
        type: dt_string(version)
        doc: |
          On the modern (34+) path the importer may discard this string and re-seek when
          the pin count fails to validate (tailless fallback); on the legacy path it is
          always read.  Common path modelled.
      - id: pin_meta
        type: legacy_pin_meta
        if: version < 34
        doc: Legacy int3 + byte + int3 pre-pin metadata tuple (34+ uses extra_tail instead).
      - id: extra_tail
        type: modern_extra_tail
        if: version >= 34
        doc: |
          Modern (34+) optional UTF-16 extra-tail field gated by a u4 raw char count.
          The C++ has additional 2-byte pin-separator and tailless fallbacks not
          statically expressible; common path modelled.
      - id: num_pins
        type: int3
      - id: pin_header_byte
        type: u1
        doc: |
          Pin-section header byte (34+ path only; legacy reads num_pins directly inside
          pin_meta).  Present here for the modern common path.
        if: version >= 34
      - id: pins
        type: pin(version, _index)
        repeat: expr
        repeat-expr: 'num_pins.value >= 0 and num_pins.value <= 500 ? num_pins.value : 0'
      - id: shapes_and_pattern
        size-eos: true
        doc: |
          Trailing shapes (parseShape, each isShapeStart()-gated) followed by the
          embedded footprint pattern (parseEmbeddedPattern).  Both are scanned, not
          count-prefixed, so they are kept raw here; see types shape and embedded_pattern.

  field_e_entry:
    doc: One string/string/int3 triple consumed when component.field_e is in 1..999.
    params:
      - id: version
        type: s4
    seq:
      - id: str_a
        type: dt_string(version)
      - id: str_b
        type: dt_string(version)
      - id: field
        type: int3

  legacy_pin_meta:
    doc: Legacy (<34) pre-pin metadata tuple immediately preceding num_pins.
    seq:
      - id: meta_a
        type: int3
      - id: meta_flag
        type: u1
      - id: meta_b
        type: int3

  modern_extra_tail:
    doc: |
      Modern (34+) optional extra-tail string.  A u4 raw char count; when non-zero the
      payload is char_count * 2 UTF-16-BE bytes.  (The C++ ReadString() fallback for
      oversized counts is not modelled.)
    seq:
      - id: char_count
        type: u4
      - id: text
        type: str
        size: char_count * 2
        encoding: UTF-16BE
        if: 'char_count > 0 and char_count < 10000'

  # --- Component pin (parsePin, lines ~1153-1217) ----------------------------

  pin:
    doc: |
      One component pin.  The first pin (pin_index == 0) carries a header block whose
      size depends on the version (2 int3 for <34, 4 int3 for 34+).  A 5-field skip /
      int3 block in the middle differs between legacy and modern layouts.
    params:
      - id: version
        type: s4
      - id: pin_index
        type: s4
    seq:
      - id: header_legacy
        type: pin_header_legacy
        if: 'pin_index == 0 and version < 34'
      - id: header_modern
        type: pin_header_modern
        if: 'pin_index == 0 and version >= 34'
      - id: x
        type: int4
      - id: y
        type: int4
      - id: length
        type: int4
      - id: name
        type: dt_string(version)
      - id: number
        type: dt_string(version)
      - id: net_flag_a
        type: u1
      - id: net_flag_b
        type: u1
      - id: label_x_off
        type: int4
      - id: label_y_off
        type: int4
      - id: num_x_off
        type: int4
      - id: num_y_off
        type: int4
      - id: post_a
        type: int3
      - id: mid_legacy
        type: pin_mid_legacy
        if: version < 34
      - id: mid_modern
        type: pin_mid_modern
        if: version >= 34
      - id: stub_dx
        type: int4
      - id: stub_dy
        type: int4
      - id: tail_byte
        type: u1
      - id: tail_b
        type: int3
      - id: tail_c
        type: int3
      - id: tail_d
        type: int3
      - id: tail_e
        type: int3

  pin_header_legacy:
    doc: First-pin header for legacy (<34) files (headerA, typeCode).
    seq:
      - id: header_a
        type: int3
      - id: type_code
        type: int3

  pin_header_modern:
    doc: First-pin header for modern (34+) files (headerA..C, typeCode).
    seq:
      - id: header_a
        type: int3
      - id: header_b
        type: int3
      - id: header_c
        type: int3
      - id: type_code
        type: int3

  pin_mid_legacy:
    doc: Legacy (<34) mid-pin block (byte, byte, int3, byte, int3).
    seq:
      - id: byte_a
        type: u1
      - id: byte_b
        type: u1
      - id: field_a
        type: int3
      - id: byte_c
        type: u1
      - id: field_b
        type: int3

  pin_mid_modern:
    doc: Modern (34+) mid-pin block (5 skipped bytes + one int3).
    seq:
      - id: unknown_0
        size: 5
        doc: Five bytes skipped by parsePin() without interpretation.
      - id: field_a
        type: int3

  # --- Component shape (parseShape, lines ~1220-1259) ------------------------

  shape:
    doc: |
      One graphical polyline inside a component.  parseShape() is gated by isShapeStart()
      and reads a version-dependent prefix.  A trailing point with numPoints out of
      [1,100] aborts the shape early (no points / tail consumed); modelled assuming a
      valid point count.
    params:
      - id: version
        type: s4
    seq:
      - id: flags
        size: 3
      - id: shape_field
        type: int3
      - id: legacy_pad_a
        type: int3
        if: version < 34
      - id: legacy_pad_b
        type: int3
        if: version < 34
      - id: modern_pad
        size: 4
        if: version >= 34
      - id: line_width
        type: int4
      - id: num_points
        type: int3
      - id: points
        type: shape_point
        repeat: expr
        repeat-expr: 'num_points.value >= 1 and num_points.value <= 100 ? num_points.value : 0'
      - id: unknown_after_points
        size: 2
        doc: Two bytes skipped after the point list.
      - id: font_x
        type: int4
      - id: font_y
        type: int4
      - id: tail_byte
        type: u1
      - id: tail_a
        type: int3
      - id: tail_b
        type: int3
      - id: tail_c
        type: int3

  shape_point:
    seq:
      - id: x
        type: int4
      - id: y
        type: int4

  # --- Embedded footprint pattern (parseEmbeddedPattern, lines ~1262-1471) ----

  embedded_pattern:
    doc: |
      Embedded footprint pattern following the component shapes.  Two very different
      encodings exist.  Legacy (<34) is field-walked and modelled below as
      embedded_pattern_legacy.  Modern (34+) is NOT linearly parsed by the importer:
      it scans for a UTF-16-BE "Case Info" anchor and back-computes the pattern name,
      so it cannot be expressed as a deterministic grammar and is left as raw bytes.
    params:
      - id: version
        type: s4
    seq:
      - id: legacy
        type: embedded_pattern_legacy(version)
        if: version < 34
      - id: modern_raw
        size-eos: true
        if: version >= 34
        doc: |
          Modern pattern blob (library metadata, 3D models, etc.).  The importer locates
          patternName by scanning for the "Case Info" UTF-16-BE marker and walking
          backward; no field-by-field layout is decoded.

  embedded_pattern_legacy:
    doc: |
      Legacy (<34) embedded pattern.  Begins with an int3(0)+int3(0) sentinel.  The
      number of pad records (field_a) and sentinel records (field_b) are validated;
      out-of-range values abort the parse (importer seeks back to start).
    params:
      - id: version
        type: s4
    seq:
      - id: pre_name_a
        type: int3
      - id: pre_name_b
        type: int3
      - id: pre_name_c
        type: int4
      - id: pre_name_d
        type: int4
      - id: pre_name_flag
        type: u1
      - id: pre_name_e
        type: int4
      - id: pre_name_flag2
        type: u1
      - id: pre_name_f
        type: int3
      - id: width
        type: int4
      - id: height
        type: int4
      - id: def_pad_w
        type: int4
      - id: def_pad_h
        type: int4
      - id: mount_type
        type: int3
      - id: mount_byte
        type: u1
      - id: drill
        type: int4
      - id: pattern_name
        type: dt_string(version)
      - id: org_x
        type: int4
      - id: org_y
        type: int4
      - id: post_name_a
        type: int4
      - id: post_name_b
        type: int4
      - id: post_name_flag
        type: u1
      - id: post_name_c
        type: int3
      - id: post_name_d
        type: int3
      - id: field_a
        type: int3
        doc: Pad record count (incl. template + terminator); 0 means empty pattern.
      - id: empty_footer
        type: int3
        repeat: expr
        repeat-expr: 'field_a.value == 0 ? 3 : 0'
        doc: 3 * int3(0) footer present only for empty patterns (field_a == 0).
      - id: pad_separator
        type: u1
        if: field_a.value > 0
        doc: byte(0) separator before the pad template.
      - id: pads
        type: legacy_pad(version)
        repeat: expr
        repeat-expr: 'field_a.value > 0 ? field_a.value : 0'
      - id: pad_terminator
        size: 39
        if: field_a.value > 0
        doc: 39 zero bytes trailing the pad records.
      - id: field_b
        type: int3
        if: field_a.value > 0
        doc: Sentinel-record count.
      - id: pre_sentinel
        size: 55
        if: field_a.value > 0
        doc: 55 bytes preceding the sentinel records.
      - id: sentinels
        type: legacy_sentinel(_index, field_b.value)
        repeat: expr
        repeat-expr: 'field_a.value > 0 ? field_b.value : 0'

  legacy_pad:
    doc: One pad record in a legacy embedded pattern (skipped by the importer).
    params:
      - id: version
        type: s4
    seq:
      - id: pad_id
        type: int3
      - id: x
        type: int4
      - id: y
        type: int4
      - id: number
        type: dt_string(version)
        doc: Legacy ASCII string (version < 34 implies legacy string encoding).
      - id: note
        type: dt_string(version)
      - id: width
        type: int4
      - id: height
        type: int4
      - id: drill
        type: int4
      - id: tail
        size: 11

  legacy_sentinel:
    doc: |
      One sentinel record.  The last record (index == count-1) is a 49-byte footer;
      all earlier records are 62 bytes.  Bytes are skipped by the importer.
    params:
      - id: index
        type: s4
      - id: count
        type: s4
    seq:
      - id: data
        size: 'index == count - 1 ? 49 : 62'

  # --- Section 7: bus section (parseBusSection, lines ~1474-1538) -------------

  bus_section:
    doc: |
      Bus section located by findBusSection() (marker
      3B 9A F1 10 3B 9A F1 10 00 00 + int3 count).  This type describes the section the
      importer reads starting at that offset.  Entry parsing stops early if an entry's
      terminator int3 != -1.
    params:
      - id: version
        type: s4
    seq:
      - id: field_a
        type: int4
      - id: field_b
        type: int4
      - id: flag_a
        type: u1
      - id: flag_b
        type: u1
      - id: bus_count
        type: int3
      - id: entries
        type: bus_entry(version)
        repeat: expr
        repeat-expr: 'bus_count.value >= 0 and bus_count.value <= 1000 ? bus_count.value : 0'

  bus_entry:
    params:
      - id: version
        type: s4
    seq:
      - id: lead_a
        type: u1
      - id: lead_b
        type: u1
      - id: lead_c
        type: u1
      - id: coord_x
        type: int4
      - id: coord_y
        type: int4
      - id: sheet_index
        type: int3
      - id: bus_type
        type: int3
      - id: instance_id
        type: int3
      - id: signal_count
        type: int3
      - id: terminator
        type: int3
        doc: Must decode to -1; any other value aborts bus parsing in the importer.
      - id: name
        type: dt_string(version)
      - id: trailing_flag
        type: u1

  # --- Section 8a: net-label scan (parseNetSection, lines ~1541-1664) ---------

  net_label_scan_record:
    doc: |
      One net-label entry discovered by parseNetSection().  The importer scans for the
      0x0F 0x42 0x3F marker (an int3 == -1 prefix) followed by a plausible name string
      and an 11-byte coordinate tail.  This type models a single match starting at the
      marker; the surrounding scan and rejected candidates are not part of the grammar.
    params:
      - id: version
        type: s4
    seq:
      - id: marker
        contents: [0x0f, 0x42, 0x3f]
        doc: int3 == -1 marker preceding the net name.
      - id: name
        type: dt_string(version)
      - id: coord_x
        type: int4
      - id: coord_y
        type: int4
      - id: field1
        type: int3

  # --- Section 8b: net/wire records (parseWireSection, lines ~1956-2099) ------

  net_wire_record:
    doc: |
      One net's structured wire data (v38+ only; legacy files keep net-label-only
      import).  Nets are separated by a variable preamble that the importer skips via an
      isPlausibleNetName() forward scan, so a clean repeat of this record is not
      expressible at the section level; this type describes one net record beginning at
      its name string.  Per the importer, pin_count is bounded to [0,4000] and
      wire_count to [0,100000]; out-of-range values abort the walk.
    params:
      - id: version
        type: s4
    seq:
      - id: name
        type: dt_string(version)
      - id: label_x
        type: int4
      - id: label_y
        type: int4
      - id: pad
        type: int3
      - id: flag
        type: u1
      - id: pin_count
        type: int3
      - id: pins
        type: net_pin_ref
        repeat: expr
        repeat-expr: pin_count.value
      - id: wire_count
        type: int3
      - id: wires
        type: wire
        repeat: expr
        repeat-expr: wire_count.value

  net_pin_ref:
    doc: A net pin reference (part index + pin index), 6 bytes.
    seq:
      - id: part_index
        type: int3
      - id: pin_index
        type: int3

  wire:
    doc: |
      One wire of a net.  The 12-int3 header carries the connection metadata; the
      importer only assigns meaning to the first seven tokens (object1, object2,
      sub_object1, sub_object2, bus1, bus2, sheet_index) and skips the remaining five.
    seq:
      - id: object1
        type: int3
      - id: object2
        type: int3
      - id: sub_object1
        type: int3
      - id: sub_object2
        type: int3
      - id: bus1
        type: int3
      - id: bus2
        type: int3
      - id: sheet_index
        type: int3
      - id: unknown_7
        type: int3
        doc: Header token 7; read past but not interpreted by the importer.
      - id: connected2_type
        type: int3
        doc: Header token 8; read past but not interpreted.
      - id: flag9
        type: int3
        doc: Header token 9; read past but not interpreted.
      - id: flag10
        type: int3
        doc: Header token 10; read past but not interpreted.
      - id: wire_id
        type: int3
        doc: Header token 11; read past but not interpreted.
      - id: flag
        type: u1
      - id: point_count
        type: int3
      - id: points
        type: wire_point
        repeat: expr
        repeat-expr: point_count.value
      - id: trailer
        size: 8
        doc: Per-wire 8-byte trailer skipped by the importer.

  wire_point:
    doc: A wire vertex (X int4, Y int4, direction int3), 11 bytes.
    seq:
      - id: x
        type: int4
      - id: y
        type: int4
      - id: dir
        type: int3
