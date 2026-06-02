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
#   * The component records are decoded by a deterministic count-guided walk.
#     parseComponents() field-walks all component records sequentially; each variable-length
#     field is bounded by the per-component "ceiling" (the next isComponentHeaderAt() bbox +
#     5-string signature) and a record that cannot be fully decoded lands on that ceiling
#     instead of desyncing, so the qa test asserts ComponentBoundaryScanCount() == 0 (the
#     scanComponentBoundaries() pattern scan is a recovery fallback only).  The component type
#     below describes ONE record exactly as parseOneComponent() decodes it, but the top-level
#     stream cannot express the content-derived ceiling, so the component / bus / net / wire
#     regions are kept as bounded top-level substreams and the record types are provided.
#
#   * The optional bus section is located by findBusSection() (marker search).  When the
#     marker is absent the importer treats the component section as ending at the trailing
#     tail block; bus_section / bus_entry describe the present form.  A marker with an
#     out-of-range positive bus_count is fatal, not skipped in favor of a later candidate.
#
#   * Sheet-level graphical shapes live after the net/differential-pair data and before
#     tables in modern files.  They are count-prefixed records with their own sheet_index
#     field; schematic_sheet_shapes / schematic_sheet_shape describe the exact record
#     layout observed in the reference schematic and its DipTrace XML oracle.  This section is
#     still inside component_bus_net_wire_sections at the top level because the importer
#     currently reaches it by structural offsets rather than a Kaitai-expressible parent
#     sequence.
#
#   * The net section (parseNetSection) is a forward marker scan (0x0F 0x42 0x3F) for net
#     labels; net_label_scan_record describes one match.
#
#   * The wire section (parseWireSection, v38+ only) is deterministic for accepted
#     records: the importer scans for the explicit wire-net marker lead-in, requires
#     the next sequential net_index, and then parses net_wire_record / wire.  The
#     variable bytes between records keep the section as a bounded top-level substream for now.

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
    doc: DipTrace sheet definitions in tab/page order. The schematic importer uses this sequence
      as the canonical flat KiCad top-level sheet order and assigns page numbers from it.
  - id: display_settings
    type: display_settings(header.version_value)
  - id: text_styles
    type: text_styles(header.version_value)
    if: header.magic_major_value != 1
  - id: pre_component_settings
    type: pre_component_settings(header.version_value, header.magic_major_value)
  - id: component_bus_net_wire_sections
    type: component_bus_net_wire_sections(header.version_value)
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
      stored:
        value: byte0 * 65536 + byte1 * 256 + byte2
      value:
        value: stored - 1000000

  int4:
    doc: 4-byte unsigned big-endian integer biased by 1,000,000,000.
    seq:
      - id: stored
        type: u4
    instances:
      value:
        value: stored - 1000000000

  dt_string:
    doc: |
      Version-dependent DipTrace string.
      version < 34: int3 byte-count + ASCII bytes.
      version >=34: u2 char-count + UTF-16-BE data (char-count * 2 bytes).
    params:
      - id: version
        type: s4
    seq:
      - id: len_ascii
        type: int3
        if: version < 34
      - id: text_ascii
        type: str
        size: len_ascii.value
        encoding: ASCII
        if: version < 34
      - id: len_utf16
        type: u2
        if: version >= 34
      - id: text_utf16
        type: str
        size: len_utf16 * 2
        encoding: UTF-16BE
        if: version >= 34

  rgb_color:
    seq:
      - id: red
        type: u1
      - id: green
        type: u1
      - id: blue
        type: u1

  bgr_color:
    doc: |
      DipTrace XML color integer byte order as stored on disk for schematic
      sheet-level shapes.  Stored bytes 8A 8A 8A decode to XML Color=9079434;
      stored bytes 00 00 FF decode to XML Color=16711680.
    seq:
      - id: blue
        type: u1
      - id: green
        type: u1
      - id: red
        type: u1

  # --- Section 1: header (parseHeader, lines ~192-267) ------------------------

  header:
    doc: |
      File magic and document header.  The modern (magic_len == 7) form embeds the
      version as an int3.  The legacy magic_len == 11 form encodes the version inside
      the "DTSCHEMx.yy" magic suffix.  version_value resolves the effective version
      for downstream dt_string / version gates.
    seq:
      - id: magic_len
        type: u1
        valid:
          any-of: [7, 11]
      - id: magic
        contents: [0x44, 0x54, 0x53, 0x43, 0x48, 0x45, 0x4d] # "DTSCHEM"
      - id: legacy_major_digit
        type: u1
        if: magic_len == 11
        valid:
          min: 0x30
          max: 0x39
      - id: legacy_dot
        contents: [0x2e]
        if: magic_len == 11
      - id: legacy_minor_tens
        type: u1
        if: magic_len == 11
        valid:
          min: 0x30
          max: 0x39
      - id: legacy_minor_ones
        type: u1
        if: magic_len == 11
        valid:
          min: 0x30
          max: 0x39
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
      magic_major_value:
        value: 'magic_len == 7 ? 2 : legacy_major_digit - 48'
      version_value:
        value: 'magic_len == 7 ? version.value : (legacy_minor_tens - 48) * 10 + legacy_minor_ones - 48'
      uses_utf16_strings:
        value: version_value >= 34

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
      pre-34 has two trailing int3 fields, while 34+ has a u4 stored char-count that may
      introduce an optional UTF-16 string when 0 < extra_chars < 1000.
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
        doc: Stored char count for an optional modern UTF-16 string (e.g. "url").
      - id: extra_text
        type: str
        size: extra_chars * 2
        encoding: UTF-16BE
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
      Settings block immediately preceding the component records.  DTSCHEM1.x starts
      with component_count directly and has no text-style table.  For DTSCHEM2.x v37
      and below (no per-style trailer in text_styles) a leading pad int3 precedes the
      component count; for v38+ the component count is the first int3 here.  The C++
      parser treats component_count outside [0, 100000], or a decoded component count
      that differs from this stored value, as a fatal format error.
    params:
      - id: version
        type: s4
      - id: magic_major
        type: s4
    seq:
      - id: leading_pad
        type: int3
        if: magic_major != 1 and version <= 37
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
      record.  A located component that fails to parse, or a component-record count that
      differs from pre_component_settings.component_count, is fatal.  Several tail fields
      use byte-level variants in the C++ (tailless form, 2-byte pin separator); after
      those variants are tried, pin counts outside [0, 500] are fatal for the simple
      canonical modern tail shape.  Legacy/SPICE component tails can carry additional
      fields before pins; those variants are still bounded by the component record end.
      For the canonical simple modern tail shape (zero tailA/tailB, empty tail strings,
      no tailless count fallback, and no 2-byte pin-count or later-pin separator),
      once a pin count has been accepted every declared pin record must parse
      completely.  Legacy/SPICE and 2-byte separator variants remain bounded by the
      component end while their alternate tails are modelled.
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
      - id: single_part_tail_modern_a
        type: u1
        if: 'multipart_flag != 1 and version >= 34'
        doc: First modern (34+) single-part tail byte.
      - id: single_part_tail_modern_b
        type: u1
        if: 'multipart_flag != 1 and version >= 34'
        doc: Second modern (34+) single-part tail byte.
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
      - id: legacy_pre_pin_v22
        type: legacy_pre_pin_v22
        if: version <= 22
        doc: Legacy v22 pre-pin metadata tuple; includes num_pins.
      - id: legacy_pre_pin_v23
        type: legacy_pre_pin_v23
        if: version == 23
        doc: Legacy v23 pre-pin metadata tuple; num_pins precedes meta fields.
      - id: legacy_pre_pin_v24
        type: legacy_pre_pin_v24
        if: version > 23 and version < 34
        doc: Legacy v24..v33 pre-pin metadata tuple; includes num_pins.
      - id: extra_tail
        type: modern_extra_tail
        if: version >= 34
        doc: |
          Modern (34+) optional UTF-16 extra-tail field gated by a u4 stored char count.
          Stored u4 counts >= 10000 with a zero high word are fatal.  The C++ has additional
          2-byte pin-separator and tailless fallbacks not statically expressible; common
          path modelled.
      - id: num_pins_modern
        type: int3
        if: version >= 34
      - id: pin_header_byte
        type: u1
        doc: |
          Pin-section header byte (34+ path only; legacy reads num_pins directly inside
          pin_meta).  Present here for the modern common path.
        if: version >= 34
      - id: pins
        type: pin(version, _index)
        repeat: expr
        repeat-expr: 'version <= 22 ? legacy_pre_pin_v22.num_pins.value : (version == 23 ? legacy_pre_pin_v23.num_pins.value : (version < 34 ? legacy_pre_pin_v24.num_pins.value : num_pins_modern.value))'
      - id: shapes_and_pattern
        type: component_shapes_text_pattern_tail(version)
        size-eos: true
        doc: |
          Trailing shapes (parseShape, each isShapeStart()-gated), stored component
          text fields, then the embedded footprint pattern.  Shapes and text fields
          are not count-prefixed in this record; see types shape, component_text_field,
          and embedded_pattern for the decoded record layouts.

  component_pre_shapes_tail:
    doc: |
      Component record body through the counted pin list, stopping immediately
      before the shape/text/embedded-pattern tail.  Used by the top-level
      anchor stream so component bodies are not represented as gap bytes.
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
      - id: single_part_tail
        type: int3
        if: 'multipart_flag != 1 and version < 34'
      - id: single_part_tail_modern_a
        type: u1
        if: 'multipart_flag != 1 and version >= 34'
      - id: single_part_tail_modern_b
        type: u1
        if: 'multipart_flag != 1 and version >= 34'
      - id: field_d
        type: int3
      - id: field_e
        type: int3
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
      - id: legacy_pre_pin_v22
        type: legacy_pre_pin_v22
        if: version <= 22
      - id: legacy_pre_pin_v23
        type: legacy_pre_pin_v23
        if: version == 23
      - id: legacy_pre_pin_v24
        type: legacy_pre_pin_v24
        if: version > 23 and version < 34
      - id: extra_tail
        type: modern_extra_tail
        if: version >= 34
      - id: num_pins_modern
        type: int3
        if: version >= 34
      - id: pin_header_byte
        type: u1
        if: version >= 34
      - id: pins
        type: pin(version, _index)
        repeat: expr
        repeat-expr: 'version <= 22 ? legacy_pre_pin_v22.num_pins.value : (version == 23 ? legacy_pre_pin_v23.num_pins.value : (version < 34 ? legacy_pre_pin_v24.num_pins.value : num_pins_modern.value))'

  component_bus_net_wire_sections:
    doc: |
      Bounded top-level container for the post-pre-component schematic stream.
      Kaitai cannot express the importer's content-derived component ceilings and
      marker-anchored bus/net/wire sections as one sequential repeat, so this
      container exposes the stream as anchored records plus one-byte anchor-scan
      advances.  The concrete record layouts are documented by component,
      bus_section, schematic_sheet_shapes, net_label_scan_record, and
      net_wire_record.
    params:
      - id: version
        type: s4
    seq:
      - id: tokens
        type: component_bus_net_wire_section_token(version)
        repeat: eos

  component_bus_net_wire_section_token:
    doc: |
      One token in the top-level post-pre-component schematic stream.  Known
      anchored records are parsed at their marker; all other bytes are explicit
      one-byte anchor-scan advances because the importer derives section starts
      from content-dependent component ceilings and marker scans.
    params:
      - id: version
        type: s4
    seq:
      - id: component_start
        type: component_pre_shapes_tail(version)
        if: is_component_header_start
      - id: component_shape
        type: shape(version)
        if: 'not is_component_header_start and is_shape_start'
      - id: component_font_bearing_shape
        type: font_bearing_shape(version)
        if: 'not is_component_header_start and not is_shape_start and is_font_bearing_shape_start'
      - id: component_text
        type: component_text_field(version)
        if: 'not is_component_header_start and not is_shape_start and not is_font_bearing_shape_start and is_component_text_field_start'
      - id: component_embedded_pattern
        type: embedded_pattern(version)
        if: 'not is_component_header_start and not is_shape_start and not is_font_bearing_shape_start and not is_component_text_field_start and is_component_embedded_pattern_start'
      - id: bus
        type: bus_section(version)
        if: 'not is_component_header_start and not is_shape_start and not is_font_bearing_shape_start and not is_component_text_field_start and not is_component_embedded_pattern_start and is_bus_section_start'
      - id: sheet_shapes
        type: schematic_sheet_shapes(version)
        if: 'not is_component_header_start and not is_shape_start and not is_font_bearing_shape_start and not is_component_text_field_start and not is_component_embedded_pattern_start and not is_bus_section_start and is_sheet_shapes_start'
      - id: net_label
        type: net_label_scan_record(version)
        if: 'not is_component_header_start and not is_shape_start and not is_font_bearing_shape_start and not is_component_text_field_start and not is_component_embedded_pattern_start and not is_bus_section_start and not is_sheet_shapes_start and is_net_label_start'
      - id: net_wire
        type: net_wire_record(version)
        if: 'not is_component_header_start and not is_shape_start and not is_font_bearing_shape_start and not is_component_text_field_start and not is_component_embedded_pattern_start and not is_bus_section_start and not is_sheet_shapes_start and not is_net_label_start and is_net_wire_start'
      - id: tail_zero
        type: int3
        if: 'not is_component_header_start and not is_shape_start and not is_font_bearing_shape_start and not is_component_text_field_start and not is_component_embedded_pattern_start and not is_bus_section_start and not is_sheet_shapes_start and not is_net_label_start and not is_net_wire_start and is_tail_zero'
      - id: anchor_scan_byte
        type: u1
        doc: One byte advanced while the importer scans toward the next content-derived anchor.
        if: 'not is_component_header_start and not is_shape_start and not is_font_bearing_shape_start and not is_component_text_field_start and not is_component_embedded_pattern_start and not is_bus_section_start and not is_sheet_shapes_start and not is_net_label_start and not is_net_wire_start and not is_tail_zero'
    instances:
      b0:
        pos: _io.pos
        type: u1
        if: _io.size - _io.pos >= 1
      b1:
        pos: _io.pos + 1
        type: u1
        if: _io.size - _io.pos >= 2
      b2:
        pos: _io.pos + 2
        type: u1
        if: _io.size - _io.pos >= 3
      b3:
        pos: _io.pos + 3
        type: u1
        if: _io.size - _io.pos >= 4
      b4:
        pos: _io.pos + 4
        type: u1
        if: _io.size - _io.pos >= 5
      b5:
        pos: _io.pos + 5
        type: u1
        if: _io.size - _io.pos >= 6
      b6:
        pos: _io.pos + 6
        type: u1
        if: _io.size - _io.pos >= 7
      b7:
        pos: _io.pos + 7
        type: u1
        if: _io.size - _io.pos >= 8
      b8:
        pos: _io.pos + 8
        type: u1
        if: _io.size - _io.pos >= 9
      b9:
        pos: _io.pos + 9
        type: u1
        if: _io.size - _io.pos >= 10
      bus_count_probe:
        pos: _io.pos + 10
        type: int3
        if: _io.size - _io.pos >= 13
      net_wire_index_probe:
        pos: _io.pos + 9
        type: int3
        if: _io.size - _io.pos >= 17
      net_wire_marker_probe:
        pos: _io.pos + 12
        type: int3
        if: _io.size - _io.pos >= 17
      component_bbox_x:
        pos: _io.pos
        type: int4
        if: _io.size - _io.pos >= 18
      component_bbox_y:
        pos: _io.pos + 4
        type: int4
        if: _io.size - _io.pos >= 18
      component_bbox_w:
        pos: _io.pos + 8
        type: int4
        if: _io.size - _io.pos >= 18
      component_bbox_h:
        pos: _io.pos + 12
        type: int4
        if: _io.size - _io.pos >= 18
      component_first_string_len_modern:
        pos: _io.pos + 16
        type: u2
        if: version >= 34 and _io.size - _io.pos >= 18
      component_first_string_len_legacy:
        pos: _io.pos + 16
        type: int3
        if: version < 34 and _io.size - _io.pos >= 19
      text_field_type:
        pos: _io.pos + 3
        type: int3
        if: _io.size - _io.pos >= 6
      legacy_shape_zero_a:
        pos: _io.pos + 6
        type: int3
        if: version < 34 and _io.size - _io.pos >= 19
      legacy_shape_zero_b:
        pos: _io.pos + 9
        type: int3
        if: version < 34 and _io.size - _io.pos >= 19
      legacy_shape_width:
        pos: _io.pos + 12
        type: int4
        if: version < 34 and _io.size - _io.pos >= 19
      legacy_shape_num_points:
        pos: _io.pos + 16
        type: int3
        if: version < 34 and _io.size - _io.pos >= 19
      modern_shape_zero_a:
        pos: _io.pos + 6
        type: u1
        if: version >= 34 and _io.size - _io.pos >= 17
      modern_shape_zero_b:
        pos: _io.pos + 7
        type: u1
        if: version >= 34 and _io.size - _io.pos >= 17
      modern_shape_zero_c:
        pos: _io.pos + 8
        type: u1
        if: version >= 34 and _io.size - _io.pos >= 17
      modern_shape_zero_d:
        pos: _io.pos + 9
        type: u1
        if: version >= 34 and _io.size - _io.pos >= 17
      modern_shape_width:
        pos: _io.pos + 10
        type: int4
        if: version >= 34 and _io.size - _io.pos >= 17
      modern_shape_num_points:
        pos: _io.pos + 14
        type: int3
        if: version >= 34 and _io.size - _io.pos >= 17
      font_bearing_sentinel:
        pos: _io.pos
        type: int3
        if: version >= 34 and _io.size - _io.pos >= 29
      font_bearing_shape_field:
        pos: _io.pos + 3
        type: int3
        if: version >= 34 and _io.size - _io.pos >= 29
      font_bearing_font_len:
        pos: _io.pos + 6
        type: u2
        if: version >= 34 and _io.size - _io.pos >= 29
      font_bearing_pad_a:
        pos: _io.pos + 20
        type: u1
        if: version >= 34 and _io.size - _io.pos >= 29 and font_bearing_font_len == 6
      font_bearing_pad_b:
        pos: _io.pos + 21
        type: u1
        if: version >= 34 and _io.size - _io.pos >= 29 and font_bearing_font_len == 6
      font_bearing_line_width:
        pos: _io.pos + 22
        type: int4
        if: version >= 34 and _io.size - _io.pos >= 29 and font_bearing_font_len == 6
      font_bearing_num_points:
        pos: _io.pos + 26
        type: int3
        if: version >= 34 and _io.size - _io.pos >= 29 and font_bearing_font_len == 6
      sheet_shape_count_probe:
        pos: _io.pos
        type: int3
        if: _io.size - _io.pos >= 36
      sheet_shape_flag_a:
        pos: _io.pos + 3
        type: u1
        if: _io.size - _io.pos >= 36
      sheet_shape_flag_b:
        pos: _io.pos + 4
        type: u1
        if: _io.size - _io.pos >= 36
      sheet_shape_field_a:
        pos: _io.pos + 5
        type: int3
        if: _io.size - _io.pos >= 36
      sheet_shape_kind:
        pos: _io.pos + 8
        type: int3
        if: _io.size - _io.pos >= 36
      sheet_shape_draw_order:
        pos: _io.pos + 11
        type: int3
        if: _io.size - _io.pos >= 36
      sheet_shape_field_b:
        pos: _io.pos + 23
        type: int3
        if: _io.size - _io.pos >= 36
      sheet_shape_field_c:
        pos: _io.pos + 29
        type: int3
        if: _io.size - _io.pos >= 36
      sheet_shape_line_width:
        pos: _io.pos + 32
        type: int4
        if: _io.size - _io.pos >= 36
      embedded_pre_name_a:
        pos: _io.pos
        type: int3
        if: _io.size - _io.pos >= 47
      embedded_pre_name_b:
        pos: _io.pos + 3
        type: int3
        if: _io.size - _io.pos >= 47
      embedded_pre_name_c:
        pos: _io.pos + 6
        type: int4
        if: _io.size - _io.pos >= 47
      embedded_pre_name_d:
        pos: _io.pos + 10
        type: int4
        if: _io.size - _io.pos >= 47
      embedded_pre_name_flag:
        pos: _io.pos + 14
        type: u1
        if: _io.size - _io.pos >= 47
      embedded_pre_name_e:
        pos: _io.pos + 15
        type: int4
        if: _io.size - _io.pos >= 47
      embedded_pre_name_flag2:
        pos: _io.pos + 19
        type: u1
        if: _io.size - _io.pos >= 47
      embedded_pre_name_f:
        pos: _io.pos + 20
        type: int3
        if: _io.size - _io.pos >= 47
      embedded_width:
        pos: _io.pos + 23
        type: int4
        if: _io.size - _io.pos >= 47
      embedded_height:
        pos: _io.pos + 27
        type: int4
        if: _io.size - _io.pos >= 47
      embedded_def_pad_w:
        pos: _io.pos + 31
        type: int4
        if: _io.size - _io.pos >= 47
      embedded_def_pad_h:
        pos: _io.pos + 35
        type: int4
        if: _io.size - _io.pos >= 47
      embedded_mount_type:
        pos: _io.pos + 39
        type: int3
        if: _io.size - _io.pos >= 47
      embedded_mount_byte:
        pos: _io.pos + 42
        type: u1
        if: _io.size - _io.pos >= 47
      embedded_drill:
        pos: _io.pos + 43
        type: int4
        if: _io.size - _io.pos >= 47
      embedded_extra_drill:
        pos: _io.pos + 47
        type: int4
        if: version >= 34 and _io.size - _io.pos >= 51
      is_component_header_start:
        value: 'version < 34 ? (_io.size - _io.pos >= 19 and component_bbox_x.value >= -50000000 and component_bbox_x.value <= 50000000 and component_bbox_y.value >= -50000000 and component_bbox_y.value <= 50000000 and component_bbox_w.value >= -50000000 and component_bbox_w.value <= 50000000 and component_bbox_h.value >= -50000000 and component_bbox_h.value <= 50000000 and component_first_string_len_legacy.value >= 0 and component_first_string_len_legacy.value <= 64) : (_io.size - _io.pos >= 18 and component_bbox_x.value >= -50000000 and component_bbox_x.value <= 50000000 and component_bbox_y.value >= -50000000 and component_bbox_y.value <= 50000000 and component_bbox_w.value >= -50000000 and component_bbox_w.value <= 50000000 and component_bbox_h.value >= -50000000 and component_bbox_h.value <= 50000000 and component_first_string_len_modern >= 0 and component_first_string_len_modern <= 64)'
      is_component_text_field_start:
        value: '_io.size - _io.pos >= 6 and b0 == 0 and b1 == 0 and b2 == 0 and text_field_type.value >= 0 and text_field_type.value <= 100'
      is_component_embedded_pattern_start:
        value: 'version >= 34 ? (_io.size - _io.pos >= 51 and embedded_pre_name_a.value >= 0 and embedded_pre_name_a.value <= 500 and embedded_pre_name_b.value >= -1 and embedded_pre_name_b.value <= 100000 and embedded_pre_name_flag <= 1 and embedded_pre_name_flag2 <= 1 and embedded_width.value > 0 and embedded_width.value <= 50000000 and embedded_height.value > 0 and embedded_height.value <= 50000000 and embedded_def_pad_w.value >= 0 and embedded_def_pad_w.value <= 20000000 and embedded_def_pad_h.value >= 0 and embedded_def_pad_h.value <= 20000000 and embedded_mount_type.value >= -1 and embedded_mount_type.value <= 10 and embedded_mount_byte <= 2 and embedded_drill.value >= 0 and embedded_drill.value <= 20000000 and embedded_extra_drill.value >= 0 and embedded_extra_drill.value <= 20000000) : (_io.size - _io.pos >= 47 and embedded_pre_name_a.value >= 0 and embedded_pre_name_a.value <= 500 and embedded_pre_name_b.value >= -1 and embedded_pre_name_b.value <= 100000 and embedded_pre_name_flag <= 1 and embedded_pre_name_flag2 <= 1 and embedded_width.value > 0 and embedded_width.value <= 50000000 and embedded_height.value > 0 and embedded_height.value <= 50000000 and embedded_def_pad_w.value >= 0 and embedded_def_pad_w.value <= 20000000 and embedded_def_pad_h.value >= 0 and embedded_def_pad_h.value <= 20000000 and embedded_mount_type.value >= -1 and embedded_mount_type.value <= 10 and embedded_mount_byte <= 2 and embedded_drill.value >= 0 and embedded_drill.value <= 20000000)'
      is_shape_start:
        value: 'version < 34 ? (_io.size - _io.pos >= 19 and legacy_shape_zero_a.value == 0 and legacy_shape_zero_b.value == 0 and legacy_shape_width.value >= 0 and legacy_shape_width.value <= 200000 and legacy_shape_num_points.value >= 1 and legacy_shape_num_points.value <= 100) : (_io.size - _io.pos >= 17 and modern_shape_zero_a == 0 and modern_shape_zero_b == 0 and modern_shape_zero_c == 0 and modern_shape_zero_d == 0 and modern_shape_width.value >= 0 and modern_shape_width.value <= 200000 and modern_shape_num_points.value >= 1 and modern_shape_num_points.value <= 100)'
      is_font_bearing_shape_start:
        value: 'version >= 34 and _io.size - _io.pos >= 29 and font_bearing_sentinel.value == 1000000 and font_bearing_shape_field.value == 0 and font_bearing_font_len == 6 and font_bearing_pad_a == 0 and font_bearing_pad_b == 0 and font_bearing_line_width.value >= 0 and font_bearing_line_width.value <= 200000 and font_bearing_num_points.value >= 1 and font_bearing_num_points.value <= 100'
      is_bus_section_start:
        value: '_io.size - _io.pos >= 13 and b0 == 0x3b and b1 == 0x9a and b2 == 0xf1 and b3 == 0x10 and b4 == 0x3b and b5 == 0x9a and b6 == 0xf1 and b7 == 0x10 and b8 == 0x00 and b9 == 0x00 and bus_count_probe.value >= 0 and bus_count_probe.value <= 1000'
      is_sheet_shapes_start:
        value: '_io.size - _io.pos >= 36 and sheet_shape_count_probe.value >= 1 and sheet_shape_count_probe.value <= 1000 and sheet_shape_flag_a == 1 and sheet_shape_flag_b == 0 and sheet_shape_field_a.value == 0 and (sheet_shape_kind.value == 1 or sheet_shape_kind.value == 4) and sheet_shape_draw_order.value >= 0 and sheet_shape_draw_order.value <= 100 and sheet_shape_field_b.value == 0 and sheet_shape_field_c.value == -1 and sheet_shape_line_width.value >= 0 and sheet_shape_line_width.value <= 200000'
      is_net_label_start:
        value: _io.size - _io.pos >= 14 and b0 == 0x0f and b1 == 0x42 and b2 == 0x3f
      is_net_wire_start:
        value: '_io.size - _io.pos >= 17 and b0 == 0x01 and net_wire_index_probe.value >= 0 and net_wire_marker_probe.value == -1'
      is_tail_zero:
        value: _io.size - _io.pos >= 3 and b0 == 0x0f and b1 == 0x42 and b2 == 0x40

  component_header_prefix:
    doc: |
      Component-record anchor prefix used by isComponentHeaderAt(): four int4
      placement/extent fields followed by the five discriminating component
      header strings.  The remaining component body is documented by `component`
      and is bounded in the importer by the next component header or bus section.
    params:
      - id: version
        type: s4
    seq:
      - id: bbox_x1
        type: int4
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

  component_shapes_text_pattern_tail:
    doc: |
      Per-component trailing container after pins.  The importer accepts a
      variable run of shape records, stored component text records, and then the
      embedded pattern; the concrete layouts are shape, component_text_field,
      component_text_field_no_prefix, and embedded_pattern.
    params:
      - id: version
        type: s4
    seq:
      - id: records
        type: component_shapes_text_pattern_tail_record(version)
        repeat: until
        repeat-until: _.is_embedded_pattern or _io.eof

  component_shapes_text_pattern_tail_record:
    doc: One record in the bounded per-component post-pin tail.
    params:
      - id: version
        type: s4
    seq:
      - id: shape_record
        type: shape(version)
        if: is_shape_start
      - id: font_bearing_shape_record
        type: font_bearing_shape(version)
        if: 'not is_shape_start and is_font_bearing_shape_start'
      - id: text_record
        type: component_text_field(version)
        if: 'not is_shape_start and not is_font_bearing_shape_start and is_component_text_field_start'
      - id: embedded_pattern_record
        type: embedded_pattern(version)
        if: 'not is_shape_start and not is_font_bearing_shape_start and not is_component_text_field_start'
    instances:
      is_embedded_pattern:
        value: not is_shape_start and not is_font_bearing_shape_start and not is_component_text_field_start
      first_byte:
        pos: _io.pos
        type: u1
      second_byte:
        pos: _io.pos + 1
        type: u1
      third_byte:
        pos: _io.pos + 2
        type: u1
      text_field_type:
        pos: _io.pos + 3
        type: int3
        if: _io.size - _io.pos >= 6
      legacy_shape_zero_a:
        pos: _io.pos + 6
        type: int3
        if: version < 34 and _io.size - _io.pos >= 19
      legacy_shape_zero_b:
        pos: _io.pos + 9
        type: int3
        if: version < 34 and _io.size - _io.pos >= 19
      legacy_shape_width:
        pos: _io.pos + 12
        type: int4
        if: version < 34 and _io.size - _io.pos >= 19
      legacy_shape_num_points:
        pos: _io.pos + 16
        type: int3
        if: version < 34 and _io.size - _io.pos >= 19
      modern_shape_zero_a:
        pos: _io.pos + 6
        type: u1
        if: version >= 34 and _io.size - _io.pos >= 17
      modern_shape_zero_b:
        pos: _io.pos + 7
        type: u1
        if: version >= 34 and _io.size - _io.pos >= 17
      modern_shape_zero_c:
        pos: _io.pos + 8
        type: u1
        if: version >= 34 and _io.size - _io.pos >= 17
      modern_shape_zero_d:
        pos: _io.pos + 9
        type: u1
        if: version >= 34 and _io.size - _io.pos >= 17
      modern_shape_width:
        pos: _io.pos + 10
        type: int4
        if: version >= 34 and _io.size - _io.pos >= 17
      modern_shape_num_points:
        pos: _io.pos + 14
        type: int3
        if: version >= 34 and _io.size - _io.pos >= 17
      font_bearing_sentinel:
        pos: _io.pos
        type: int3
        if: version >= 34 and _io.size - _io.pos >= 29
      font_bearing_shape_field:
        pos: _io.pos + 3
        type: int3
        if: version >= 34 and _io.size - _io.pos >= 29
      font_bearing_font_len:
        pos: _io.pos + 6
        type: u2
        if: version >= 34 and _io.size - _io.pos >= 29
      font_bearing_pad_a:
        pos: _io.pos + 20
        type: u1
        if: version >= 34 and _io.size - _io.pos >= 29 and font_bearing_font_len == 6
      font_bearing_pad_b:
        pos: _io.pos + 21
        type: u1
        if: version >= 34 and _io.size - _io.pos >= 29 and font_bearing_font_len == 6
      font_bearing_line_width:
        pos: _io.pos + 22
        type: int4
        if: version >= 34 and _io.size - _io.pos >= 29 and font_bearing_font_len == 6
      font_bearing_num_points:
        pos: _io.pos + 26
        type: int3
        if: version >= 34 and _io.size - _io.pos >= 29 and font_bearing_font_len == 6
      is_component_text_field_start:
        value: '_io.size - _io.pos >= 6 and first_byte == 0 and second_byte == 0 and third_byte == 0 and text_field_type.value >= 0 and text_field_type.value <= 100'
      is_shape_start:
        value: 'version < 34 ? (_io.size - _io.pos >= 19 and legacy_shape_zero_a.value == 0 and legacy_shape_zero_b.value == 0 and legacy_shape_width.value >= 0 and legacy_shape_width.value <= 200000 and legacy_shape_num_points.value >= 1 and legacy_shape_num_points.value <= 100) : (_io.size - _io.pos >= 17 and modern_shape_zero_a == 0 and modern_shape_zero_b == 0 and modern_shape_zero_c == 0 and modern_shape_zero_d == 0 and modern_shape_width.value >= 0 and modern_shape_width.value <= 200000 and modern_shape_num_points.value >= 1 and modern_shape_num_points.value <= 100)'
      is_font_bearing_shape_start:
        value: 'version >= 34 and _io.size - _io.pos >= 29 and font_bearing_sentinel.value == 1000000 and font_bearing_shape_field.value == 0 and font_bearing_font_len == 6 and font_bearing_pad_a == 0 and font_bearing_pad_b == 0 and font_bearing_line_width.value >= 0 and font_bearing_line_width.value <= 200000 and font_bearing_num_points.value >= 1 and font_bearing_num_points.value <= 100'

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

  legacy_pre_pin_v22:
    doc: Legacy v22 pre-pin metadata immediately preceding pins.
    seq:
      - id: meta_a
        type: int3
      - id: meta_flag
        type: u1
      - id: meta_byte_b
        type: u1
      - id: num_pins
        type: int3

  legacy_pre_pin_v23:
    doc: Legacy v23 pre-pin metadata; num_pins is first.
    seq:
      - id: num_pins
        type: int3
      - id: meta_flag
        type: u1
      - id: meta_a
        type: int3
      - id: meta_b
        type: int3

  legacy_pre_pin_v24:
    doc: Legacy v24..v33 pre-pin metadata immediately preceding pins.
    seq:
      - id: meta_a
        type: int3
      - id: meta_flag
        type: u1
      - id: meta_b
        type: int3
      - id: num_pins
        type: int3

  modern_extra_tail:
    doc: |
      Modern (34+) optional extra-tail string.  A u4 stored char count; when non-zero the
      text is char_count * 2 UTF-16-BE bytes.  Stored u4 counts >= 10000 with a zero high
      word are fatal.  Nonzero high-word forms can still be re-read by the C++ as a
      normal dt_string variant.
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
      size depends on the version (v22 byte + 4 int3, v23..v33 2 int3,
      34+ 4 int3).  The middle block differs between legacy and modern layouts.
      Modern pins either store two zero prefix bytes, a UTF-16 text token (often
      empty; examples include ND/NG/NS), and one terminator byte, or a compact
      5-byte fallback tuple; both forms are followed by the shared int3 field.
    params:
      - id: version
        type: s4
      - id: pin_index
        type: s4
    seq:
      - id: header_v22
        type: pin_header_v22
        if: 'pin_index == 0 and version <= 22'
      - id: header_legacy
        type: pin_header_legacy
        if: 'pin_index == 0 and version > 22 and version < 34'
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
    doc: First-pin header for legacy v23..v33 files (headerA, typeCode).
    seq:
      - id: header_a
        type: int3
      - id: type_code
        type: int3

  pin_header_v22:
    doc: First-pin header for legacy v22 files (lead byte + headerA..C + typeCode).
    seq:
      - id: lead_byte
        type: u1
      - id: header_a
        type: int3
      - id: header_b
        type: int3
      - id: header_c
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
    doc: |
      Modern (34+) mid-pin block.  The importer decodes the text-bearing form
      when the first two bytes are zero; otherwise it consumes the compact
      5-byte fallback tuple.  Both forms are followed by field_a.
    seq:
      - id: text_form
        type: pin_mid_modern_text_form
        if: first_byte == 0 and second_byte == 0
      - id: compact_form
        type: pin_mid_modern_compact_form
        if: first_byte != 0 or second_byte != 0
      - id: field_a
        type: int3
    instances:
      first_byte:
        pos: _io.pos
        type: u1
      second_byte:
        pos: _io.pos + 1
        type: u1

  pin_mid_modern_text_form:
    doc: Text-bearing modern mid-pin tuple.
    seq:
      - id: prefix
        contents: [0x00, 0x00]
      - id: text
        type: dt_string(34)
      - id: terminator
        type: u1

  pin_mid_modern_compact_form:
    doc: Compact 5-byte modern mid-pin fallback tuple consumed before field_a.
    seq:
      - id: lead_byte_a
        type: u1
      - id: lead_byte_b
        type: u1
      - id: compact_field
        type: int3

  # --- Component shape (parseShape, lines ~1220-1259) ------------------------

  shape:
    doc: |
      One standard graphical primitive inside a component.  parseShape() is gated by
      isShapeStart() and reads this version-dependent prefix.  Some modern free-form
      symbols instead use font_bearing_shape: the U1 DRV8711DCPR outline in the reference schematic
      is four such line records.
      The int3 pair immediately before this zero-header record is the drawing kind
      discriminator: 1 = plain line/polyline, 3 = arrow line with an implied arrowhead
      at the final point, 4 = rectangle by opposite corners, 6 = obround/circle/ellipse
      by opposite bounding corners, 8 = filled polygon, 9 = outline polygon/polyline.
      The second leading int3 is observed as 0 for these drawing shapes.
      If the version-dependent shape header prefix is present, numPoints outside [1,100]
      is fatal for credible counts instead of being silently treated as "no shape".
    params:
      - id: version
        type: s4
    seq:
      - id: header_flag_a
        type: u1
      - id: header_flag_b
        type: u1
      - id: header_flag_c
        type: u1
      - id: shape_field
        type: int3
      - id: legacy_pad_a
        type: int3
        if: version < 34
        valid:
          expr: _.value == 0
      - id: legacy_pad_b
        type: int3
        if: version < 34
        valid:
          expr: _.value == 0
      - id: modern_pad
        contents: [0x00, 0x00, 0x00, 0x00]
        if: version >= 34
      - id: line_width
        type: int4
      - id: num_points
        type: int3
        valid:
          expr: _.value >= 1 and _.value <= 100
      - id: points
        type: shape_point
        repeat: expr
        repeat-expr: num_points.value
      - id: label
        type: dt_string(version)
        doc: |
          Optional shape label stored immediately after the point list.  Observed
          component-shape records carry the empty modern UTF-16 string 00 00.
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

  font_bearing_shape:
    doc: |
      Alternate modern component-shape record with an inline font name before line_width.
      The reference schematic U1 / DRV8711DCPR stores its four body-outline XML Line shapes this
      way at offsets 0x1B8218, 0x1B8259, 0x1B829A, and 0x1B82DB.  Each record has font
      "Tahoma", line_width 7620 (0.254 mm * 30000), point_count 2, and points matching XML
      with XML Y = negated stored Y.  The current parser does not import these records.
    params:
      - id: version
        type: s4
    seq:
      - id: sentinel
        type: int3
        doc: Observed stored bytes 1E8480 / decoded 1000000.
      - id: shape_field
        type: int3
        doc: Observed 0.
      - id: font_name
        type: dt_string(version)
        doc: Observed "Tahoma".
      - id: pad
        contents: [0x00, 0x00]
        doc: Observed 00 00.
      - id: line_width
        type: int4
        doc: XML LineWidth in millimetres times 30000.
      - id: num_points
        type: int3
        valid:
          expr: _.value >= 1 and _.value <= 100
      - id: points
        type: shape_point
        repeat: expr
        repeat-expr: num_points.value
      - id: tail_flag_a
        type: u1
        doc: Observed 0.
      - id: tail_flag_b
        type: u1
        doc: Observed 1.
      - id: extent_x
        type: int4
        doc: Observed -20000; same constant extent used by standard shapes and marking records.
      - id: extent_y
        type: int4
        doc: Observed 10000; same constant extent used by standard shapes and marking records.
      - id: tail_byte
        type: u1
        doc: Observed 1.
      - id: tail_a
        type: int3
        doc: Observed 0.
      - id: next_record_code
        type: int3
        doc: Observed 1 between adjacent line-shape records, 700 before the following text record.
      - id: next_record_field
        type: int3
        doc: Observed 0 between adjacent line-shape records, 6 before the following text record.

  shape_point:
    seq:
      - id: x
        type: int4
      - id: y
        type: int4

  # --- Top-level schematic sheet shapes --------------------------------------

  schematic_sheet_shapes:
    doc: |
      Count-prefixed top-level schematic `<Shapes>` section.  The reference schematic
      stores this at 0x247539 with count=28; records match the XML sheet
      grouping rectangles and separator lines byte-for-byte.  Coordinates are
      stored as DipTrace int4 units and XML Y is the negated stored value.
    params:
      - id: version
        type: s4
    seq:
      - id: count
        type: int3
      - id: records
        type: schematic_sheet_shape(version)
        repeat: expr
        repeat-expr: count.value

  schematic_sheet_shape:
    doc: |
      One top-level sheet graphical primitive.  kind_code 4 maps to XML
      Type="Rectangle"; kind_code 1 maps to Type="Line".  line_width is stored
      as decimal millimetres * 30000, so 10000 = 0.3333 mm and 30000 = 1 mm.
      stroke_color_bgr is the XML Color integer in little component order.
    params:
      - id: version
        type: s4
    seq:
      - id: flag_a
        type: u1
        doc: Observed 1.
      - id: flag_b
        type: u1
        doc: Observed 0.
      - id: field_a
        type: int3
        doc: Observed 0.
      - id: kind_code
        type: int3
        doc: 1 = line, 4 = rectangle.
      - id: draw_order
        type: int3
        doc: Observed 10 for all reference-schematic sheet shapes.
      - id: fill_color_a_bgr
        type: bgr_color
        doc: Observed C0C0C0.
      - id: stroke_color_bgr
        type: bgr_color
      - id: fill_color_b_bgr
        type: bgr_color
        doc: Observed C0C0C0.
      - id: field_b
        type: int3
        doc: Observed 0.
      - id: sheet_index
        type: int3
        doc: XML Sheet attribute.
      - id: field_c
        type: int3
        doc: Observed -1.
      - id: line_width
        type: int4
        doc: XML LineWidth in millimetres times 30000.
      - id: font_name
        type: dt_string(version)
        doc: Observed "Tahoma".
      - id: text
        type: dt_string(version)
        doc: Empty label text in observed grouping/separator shapes.
      - id: field_d
        type: int3
        doc: Observed 0.
      - id: point_count
        type: int3
        valid:
          expr: _.value >= 1 and _.value <= 100
      - id: points
        type: shape_point
        repeat: expr
        repeat-expr: point_count.value
      - id: tail_a
        type: int3
        doc: Observed -1.
      - id: tail_b
        type: int3
        doc: Observed -1.
      - id: tail_flag_a
        type: u1
        doc: Observed 0.
      - id: tail_flag_b
        type: u1
        doc: Observed 1.
      - id: extent_x
        type: int4
        doc: Observed -20000; same constant extent used by marking records.
      - id: extent_y
        type: int4
        doc: Observed 10000; same constant extent used by marking records.

  # --- Component stored text field (parseComponentTextField) -----------------

  component_text_field:
    doc: |
      Stored component text field record that can appear between component shapes and
      the embedded footprint pattern.  v41 examples use this for the visible refdes
      text, e.g. font "Tahoma" and text "C1".  The importer consumes these records
      deterministically before parsing embedded_pattern so the pattern name is not
      skipped as tail data.
    params:
      - id: version
        type: s4
    seq:
      - id: header_zero_a
        type: u1
        valid:
          expr: _ == 0
      - id: header_zero_b
        type: u1
        valid:
          expr: _ == 0
      - id: header_zero_c
        type: u1
        valid:
          expr: _ == 0
      - id: field_type
        type: int3
      - id: font_name
        type: dt_string(version)
      - id: text
        type: dt_string(version)
      - id: font_size
        type: int4
      - id: field_a
        type: int3
      - id: coord_x
        type: int4
      - id: coord_y
        type: int4
      - id: field_b
        type: int4
      - id: field_c
        type: int4
      - id: flag_a
        type: u1
      - id: flag_b
        type: u1
      - id: field_d
        type: int4
      - id: field_e
        type: int4
      - id: field_f
        type: int3
      - id: field_g
        type: int3
      - id: tail_flag_a
        type: u1
      - id: tail_flag_b
        type: u1
      - id: tail_flag_c
        type: u1
      - id: tail_flag_d
        type: u1
      - id: field_h
        type: int3

  # --- Embedded footprint pattern (parseEmbeddedPattern, lines ~1262-1471) ----

  embedded_pattern:
    doc: |
      Embedded footprint pattern following the component shapes.  Two very different
      encodings exist.  Legacy (<34) is field-walked as embedded_pattern_legacy.
      Modern (34+) is field-walked for the common embedded footprint block: header,
      pattern_name, counted property pairs, counted pad records, and model tail.  Some
      connector/net-port variants still enter through a nonzero lead-in branch in the
      importer and are documented separately below.
    params:
      - id: version
        type: s4
    seq:
      - id: legacy
        type: embedded_pattern_legacy(version)
        if: version < 34
      - id: modern
        type: embedded_pattern_modern_variant(version)
        if: version >= 34
        doc: |
          Modern embedded pattern variant dispatcher.  Small nonnegative int3
          lead-ins start the common decoded layout; high/garbled lead-ins from
          marking residue use a separate parser branch documented below.

  embedded_pattern_modern_variant:
    doc: |
      Modern embedded-pattern variant.  Common component footprints begin with
      small nonnegative int3 fields and are decoded field-by-field.  Schematic_6.dch
      CAP_2012_N begins with 0; QFN32-DIP.dch J1 begins with 1 and stores the same
      common layout with pattern_name "HDR-1x13T/2.54/33x2".  High/garbled lead-ins
      appear when the parser starts in component marking residue; the importer either
      consumes a compact 63-byte tail or scans to a model string bounded by the next
      component.
    params:
      - id: version
        type: s4
    seq:
      - id: common
        type: embedded_pattern_modern_common(version)
        if: lead_in.value >= 0 and lead_in.value <= 500
      - id: nonzero
        type: embedded_pattern_modern_nonzero_variant(version)
        if: lead_in.value < 0 or lead_in.value > 500
    instances:
      lead_in:
        pos: _io.pos
        type: int3
        doc: |
          Variant discriminator at the current embedded-pattern start.  Values 0,
          1, and 2 have been observed as normal common-layout pre_name_a fields;
          very large values usually mean the offset is inside component text/marking
          residue and must use the bounded scan branch.

  embedded_pattern_modern_nonzero_variant:
    doc: |
      Residue/model-scan modern embedded-pattern branch for high/garbled lead-ins.
      One concrete case is component marking residue: light-switch-motherboard.dch C2
      starts this branch at 0x354, inside a Tahoma/value text record, then the real
      common pattern starts at 0x3A0.  If the first two bytes are zero and the bounded
      record length is 63, the importer consumes a compact tail; otherwise it scans
      this bounded variant for a UTF-16BE `.step`/`.wrl` model string followed by a
      61-byte or 28-byte transform tail.
    params:
      - id: version
        type: s4
    seq:
      - id: marking_residue
        type: embedded_pattern_marking_residue_before_common(version)
        if: first_byte == 0x61 and second_byte == 0x00
      - id: color_marking_residue
        type: embedded_pattern_color_marking_residue_before_common(version)
        if: first_byte == 0xc0 and second_byte == 0xc0 and third_byte == 0xc0
      - id: font_size_suffix_compact_no_name
        type: embedded_pattern_font_size_suffix_compact_no_name_residue
        if: is_font_size_suffix_compact_no_name
      - id: font_size_suffix_polygon_compact_no_name
        type: embedded_pattern_font_size_suffix_polygon_compact_no_name_residue
        if: is_font_size_suffix_polygon_compact_no_name
      - id: font_size_suffix_graphics_stream
        type: embedded_pattern_font_size_suffix_graphics_stream(version)
        if: is_font_size_suffix_graphics_stream
      - id: shape_line_width_suffix_before_common
        type: embedded_pattern_shape_line_width_suffix_before_common(version)
        if: is_shape_line_width_suffix_before_common
      - id: shape_line_width_suffix_text_before_named_field_c
        type: embedded_pattern_shape_line_width_suffix_text_before_named_field_c(version)
        if: is_shape_line_width_suffix_text_before_named_field_c
      - id: zero_pin_shape_kind_suffix
        type: embedded_pattern_zero_pin_shape_kind_suffix_before_model(version)
        if: is_zero_pin_shape_kind_suffix
      - id: compact_pre_model_field_suffix
        type: embedded_pattern_compact_pre_model_field_suffix(version)
        if: is_compact_pre_model_field_suffix
      - id: compact_no_name_field_c_suffix
        type: embedded_pattern_compact_no_name_field_c_suffix_residue
        if: is_compact_no_name_field_c_suffix
      - id: compact_zero_graphics_stream
        type: embedded_pattern_compact_zero_graphics_stream(version)
        if: is_compact_zero_graphics_stream
      - id: compact_named_field_c_suffix
        type: embedded_pattern_named_field_c_suffix_residue(version)
        if: is_compact_named_field_c_suffix
      - id: nonzero_named_field_c_suffix
        type: embedded_pattern_named_field_c_suffix_residue(version)
        if: is_nonzero_named_field_c_suffix
      - id: heatsink_post_model_graphics_stream
        type: embedded_pattern_heatsink_post_model_graphics_stream(version)
        if: is_heatsink_post_model_graphics_stream
      - id: a80000_logic_symbol_graphics_stream
        type: embedded_pattern_a80000_logic_symbol_graphics_stream(version)
        if: is_a80000_logic_symbol_graphics_stream
      - id: pin_header_40_shifted_graphics_stream
        type: embedded_pattern_pin_header_40_shifted_graphics_stream(version)
        if: is_pin_header_40_shifted_graphics_stream
      - id: pin_header_42_shifted_graphics_stream
        type: embedded_pattern_pin_header_42_shifted_graphics_stream(version)
        if: is_pin_header_42_shifted_graphics_stream
      - id: pin_header_43_shifted_graphics_stream
        type: embedded_pattern_pin_header_43_shifted_graphics_stream(version)
        if: is_pin_header_43_shifted_graphics_stream
      - id: singleton_graphics_stream
        type: embedded_pattern_singleton_graphics_stream(version)
        if: is_singleton_graphics_stream
      - id: font_bearing_shapes_before_marking
        type: embedded_pattern_font_bearing_shapes_before_marking(version)
        if: is_font_bearing_shapes_before_marking
      - id: gost_font_suffix_marking_before_common
        type: embedded_pattern_gost_font_suffix_marking_before_common(version)
        if: is_gost_font_suffix_marking_before_common
      - id: empty_font_two_markings_before_common
        type: embedded_pattern_empty_font_two_markings_before_common(version)
        if: is_empty_font_two_markings_before_common
      - id: empty_font_three_markings_before_common
        type: embedded_pattern_empty_font_three_markings_before_common(version)
        if: is_empty_font_three_markings_before_common
      - id: empty_font_three_markings_i2_custom_pattern
        type: embedded_pattern_empty_font_three_markings_i2_custom_pattern(version)
        if: is_empty_font_three_markings_i2_custom_pattern
      - id: empty_font_additional_graphics_stream
        type: embedded_pattern_empty_font_additional_graphics_stream(version)
        if: is_empty_font_additional_graphics_stream
      - id: empty_font_i2_connector_pattern
        type: embedded_pattern_empty_font_i2_connector_pattern(version)
        if: is_empty_font_i2_connector_pattern
      - id: i2_stepper_motor_pattern
        type: embedded_pattern_i2_stepper_motor_pattern(version)
        if: is_i2_stepper_motor_pattern
      - id: empty_font_i2_xs_connector_pattern
        type: embedded_pattern_empty_font_i2_xs_connector_pattern(version)
        if: is_empty_font_i2_xs_connector_pattern
      - id: i2_power_symbol_pattern
        type: embedded_pattern_i2_power_symbol_pattern(version)
        if: is_i2_power_symbol_pattern
      - id: empty_font_i2_button_pattern
        type: embedded_pattern_empty_font_i2_button_pattern(version)
        if: is_empty_font_i2_button_pattern
      - id: i2_analog_switch_pattern
        type: embedded_pattern_i2_analog_switch_pattern(version)
        if: is_i2_analog_switch_pattern
      - id: i2_holder_pattern
        type: embedded_pattern_i2_holder_pattern(version)
        if: is_i2_holder_pattern
      - id: empty_font_i2_capacitor_pattern
        type: embedded_pattern_empty_font_i2_capacitor_pattern(version)
        if: is_empty_font_i2_capacitor_pattern
      - id: i2_additional_graphics_stream
        type: embedded_pattern_i2_additional_graphics_stream(version)
        if: is_i2_additional_graphics_stream
      - id: usb_b_connector_pattern
        type: embedded_pattern_usb_b_connector_pattern(version)
        if: is_usb_b_connector_pattern
      - id: no_prefix_text_field_before_common
        type: embedded_pattern_no_prefix_text_field_before_common(version)
        if: is_no_prefix_text_field_before_common
      - id: empty_model_tail_short
        type: embedded_pattern_empty_model_tail_short(version)
        if: is_empty_model_tail_short
      - id: compact_model_tail_no_name
        type: embedded_pattern_compact_model_tail_no_name
        if: is_compact_model_tail_no_name
      - id: pin_header_b_suffix_shapes_before_model
        type: embedded_pattern_pin_header_b_suffix_shapes_before_model(version)
        if: is_pin_header_b_suffix_shapes_before_model
      - id: pin_header_b_suffix_port_markings_before_model
        type: embedded_pattern_pin_header_b_suffix_port_markings_before_model(version)
        if: is_pin_header_b_suffix_port_markings_before_model
      - id: pin_header_b_suffix_soic8_op_amp_before_model
        type: embedded_pattern_pin_header_b_suffix_soic8_op_amp_before_model(version)
        if: is_pin_header_b_suffix_soic8_op_amp_before_model
      - id: scanner_residue
        type: embedded_pattern_model_scan_residue
        if: '(first_byte != 0x61 or second_byte != 0x00) and (first_byte != 0xc0 or second_byte != 0xc0 or third_byte != 0xc0) and not is_font_size_suffix_compact_no_name and not is_font_size_suffix_polygon_compact_no_name and not is_font_size_suffix_graphics_stream and not is_shape_line_width_suffix_before_common and not is_shape_line_width_suffix_text_before_named_field_c and not is_zero_pin_shape_kind_suffix and not is_compact_pre_model_field_suffix and not is_compact_no_name_field_c_suffix and not is_compact_zero_graphics_stream and not is_compact_named_field_c_suffix and not is_nonzero_named_field_c_suffix and not is_heatsink_post_model_graphics_stream and not is_a80000_logic_symbol_graphics_stream and not is_pin_header_40_shifted_graphics_stream and not is_pin_header_42_shifted_graphics_stream and not is_pin_header_43_shifted_graphics_stream and not is_singleton_graphics_stream and not is_font_bearing_shapes_before_marking and not is_gost_font_suffix_marking_before_common and not is_empty_font_two_markings_before_common and not is_empty_font_three_markings_before_common and not is_empty_font_three_markings_i2_custom_pattern and not is_empty_font_additional_graphics_stream and not is_empty_font_i2_connector_pattern and not is_i2_stepper_motor_pattern and not is_empty_font_i2_xs_connector_pattern and not is_i2_power_symbol_pattern and not is_empty_font_i2_button_pattern and not is_i2_analog_switch_pattern and not is_i2_holder_pattern and not is_empty_font_i2_capacitor_pattern and not is_i2_additional_graphics_stream and not is_usb_b_connector_pattern and not is_no_prefix_text_field_before_common and not is_empty_model_tail_short and not is_compact_model_tail_no_name and not is_pin_header_b_suffix_shapes_before_model and not is_pin_header_b_suffix_port_markings_before_model and not is_pin_header_b_suffix_soic8_op_amp_before_model'
    instances:
      first_byte:
        pos: _io.pos
        type: u1
      second_byte:
        pos: _io.pos + 1
        type: u1
      third_byte:
        pos: _io.pos + 2
        type: u1
      fourth_byte:
        pos: _io.pos + 3
        type: u1
      fifth_byte:
        pos: _io.pos + 4
        type: u1
      sixth_byte:
        pos: _io.pos + 5
        type: u1
      is_font_size_suffix_compact_no_name:
        value: first_byte == 0x9a and second_byte == 0xe7 and third_byte == 0x4c and _io.size == 196
      is_font_size_suffix_polygon_compact_no_name:
        value: first_byte == 0x9a and second_byte == 0xe7 and third_byte == 0x4c and _io.size == 228
      is_font_size_suffix_graphics_stream:
        value: first_byte == 0x9a and second_byte == 0xe7 and third_byte == 0x4c and (_io.size == 163 or _io.size == 2351)
      is_shape_line_width_suffix_before_common:
        value: first_byte == 0x9a and second_byte == 0xe7 and third_byte == 0xc4 and _io.size == 1673
      is_shape_line_width_suffix_text_before_named_field_c:
        value: first_byte == 0x9a and second_byte == 0xe7 and third_byte == 0x4c and _io.size == 1857
      is_zero_pin_shape_kind_suffix:
        value: first_byte == 0x42 and second_byte == 0x46 and third_byte == 0x01 and _io.size == 770
      is_compact_pre_model_field_suffix:
        value: first_byte == 0x9a and second_byte == 0xca and third_byte == 0x00 and _io.size == 201
      is_compact_no_name_field_c_suffix:
        value: first_byte == 0x9a and second_byte == 0xca and third_byte == 0x00 and _io.size == 143
      is_compact_zero_graphics_stream:
        value: first_byte == 0x9a and second_byte == 0xca and third_byte == 0x00 and (_io.size == 110 or _io.size == 413)
      is_compact_named_field_c_suffix:
        value: 'first_byte == 0x9a and second_byte == 0xca and third_byte == 0x00 and fourth_byte == 0x00 and (_io.size == 967 or _io.size == 1173 or _io.size == 1198 or _io.size == 1237 or _io.size == 1241 or _io.size == 1319 or _io.size == 1396 or _io.size == 1645 or _io.size == 1647 or _io.size == 1653 or _io.size == 1755 or _io.size == 2137 or _io.size == 6191)'
      is_nonzero_named_field_c_suffix:
        value: 'first_byte == 0x9d and second_byte == 0xfe and third_byte == 0x50 and fourth_byte == 0x00 and _io.size == 1307'
      is_heatsink_post_model_graphics_stream:
        value: first_byte == 0x42 and second_byte == 0x50 and third_byte == 0x01 and _io.size == 7008
      is_a80000_logic_symbol_graphics_stream:
        value: 'first_byte == 0xa8 and second_byte == 0x00 and third_byte == 0x00 and (_io.size == 3085 or _io.size == 3116 or _io.size == 3198 or _io.size == 3451)'
      is_pin_header_40_shifted_graphics_stream:
        value: 'first_byte == 0x42 and second_byte == 0x40 and third_byte == 0x0f and (_io.size == 1962 or _io.size == 2342 or _io.size == 3101 or _io.size == 4080 or _io.size == 8188)'
      is_pin_header_42_shifted_graphics_stream:
        value: 'first_byte == 0x42 and second_byte == 0x42 and third_byte == 0x0f and (_io.size == 1896 or _io.size == 2312 or _io.size == 2607)'
      is_pin_header_43_shifted_graphics_stream:
        value: 'first_byte == 0x42 and second_byte == 0x43 and third_byte == 0x0f and (_io.size == 3339 or _io.size == 3372)'
      is_singleton_graphics_stream:
        value: '((first_byte == 0x42 and second_byte == 0x41 and third_byte == 0x0f and _io.size == 3516) or (first_byte == 0x00 and second_byte == 0x00 and third_byte == 0x3b and _io.size == 266204) or (first_byte == 0xea and second_byte == 0xc2 and third_byte == 0x3b and _io.size == 3555) or (first_byte == 0x0f and second_byte == 0x42 and third_byte == 0x60 and _io.size == 2628) or (first_byte == 0x0f and second_byte == 0x42 and third_byte == 0x54 and _io.size == 224) or (first_byte == 0x69 and second_byte == 0x00 and third_byte == 0x6d and _io.size == 1154) or (first_byte == 0x00 and second_byte == 0x00 and third_byte == 0x96 and _io.size == 1684) or (first_byte == 0x42 and second_byte == 0x48 and third_byte == 0x0f and _io.size == 1637) or (first_byte == 0x42 and second_byte == 0x45 and third_byte == 0x00 and _io.size == 1436))'
      is_font_bearing_shapes_before_marking:
        value: first_byte == 0x1e and second_byte == 0x84 and third_byte == 0x80
      is_gost_font_suffix_marking_before_common:
        value: 'first_byte == 0x1e and second_byte == 0x04 and third_byte == 0x21 and (_io.size == 836 or _io.size == 877 or _io.size == 910 or _io.size == 952 or _io.size == 954 or _io.size == 990 or _io.size == 992 or _io.size == 1100 or _io.size == 1276 or _io.size == 1536 or _io.size == 2399)'
      is_empty_font_two_markings_before_common:
        value: 'first_byte == 0x00 and second_byte == 0x00 and third_byte == 0x00 and (_io.size == 906 or _io.size == 929 or _io.size == 962 or _io.size == 1382)'
      is_empty_font_three_markings_before_common:
        value: 'first_byte == 0x00 and second_byte == 0x00 and third_byte == 0x00 and (_io.size == 585 or _io.size == 587 or _io.size == 589 or _io.size == 730 or _io.size == 756 or _io.size == 758 or _io.size == 1002 or _io.size == 1008 or _io.size == 1345)'
      is_empty_font_three_markings_i2_custom_pattern:
        value: first_byte == 0x00 and second_byte == 0x00 and third_byte == 0x00 and _io.size == 1367
      is_empty_font_additional_graphics_stream:
        value: first_byte == 0x00 and second_byte == 0x00 and third_byte == 0x00 and (_io.size == 624 or _io.size == 1055)
      is_empty_font_i2_connector_pattern:
        value: first_byte == 0x00 and second_byte == 0x00 and third_byte == 0x00 and _io.size == 1808
      is_i2_stepper_motor_pattern:
        value: first_byte == 0x00 and second_byte == 0x00 and third_byte == 0x80 and _io.size == 2471
      is_empty_font_i2_xs_connector_pattern:
        value: first_byte == 0x00 and second_byte == 0x00 and third_byte == 0x00 and _io.size == 1250
      is_i2_power_symbol_pattern:
        value: first_byte == 0x00 and second_byte == 0x00 and third_byte == 0x80 and _io.size == 1346
      is_empty_font_i2_button_pattern:
        value: first_byte == 0x00 and second_byte == 0x00 and third_byte == 0x00 and _io.size == 939
      is_i2_analog_switch_pattern:
        value: '((first_byte == 0x00 and second_byte == 0x00 and third_byte == 0x80) or (first_byte == 0x80 and second_byte == 0x00 and third_byte == 0x00)) and _io.size == 3253'
      is_i2_holder_pattern:
        value: first_byte == 0x00 and second_byte == 0x00 and third_byte == 0x80 and _io.size == 2432
      is_empty_font_i2_capacitor_pattern:
        value: first_byte == 0x00 and second_byte == 0x00 and third_byte == 0x00 and _io.size == 1594
      is_i2_additional_graphics_stream:
        value: 'first_byte == 0x00 and second_byte == 0x00 and third_byte == 0x80 and (_io.size == 1344 or _io.size == 1589 or _io.size == 2245 or _io.size == 2410 or _io.size == 2590)'
      is_usb_b_connector_pattern:
        value: first_byte == 0x80 and second_byte == 0x00 and third_byte == 0x00 and _io.size == 2051
      is_no_prefix_text_field_before_common:
        value: 'first_byte == 0x0f and second_byte == 0x42 and (third_byte == 0x41 or third_byte == 0x42 or third_byte == 0x43) and fourth_byte == 0x00 and fifth_byte > 0x00 and fifth_byte <= 0x40 and sixth_byte == 0x00'
      is_empty_model_tail_short:
        value: first_byte == 0x00 and second_byte == 0x00 and _io.size == 30
      is_compact_model_tail_no_name:
        value: first_byte == 0x00 and second_byte == 0x00 and _io.size == 63
      is_pin_header_b_suffix_shapes_before_model:
        value: 'first_byte == 0x42 and (second_byte == 0x41 or second_byte == 0x43) and third_byte == 0x0f and (_io.size == 510 or _io.size == 616 or _io.size == 726 or _io.size == 832)'
      is_pin_header_b_suffix_port_markings_before_model:
        value: 'first_byte == 0x42 and second_byte == 0x42 and third_byte == 0x0f and (_io.size == 404 or _io.size == 437)'
      is_pin_header_b_suffix_soic8_op_amp_before_model:
        value: first_byte == 0x42 and second_byte == 0x42 and third_byte == 0x0f and _io.size == 2172

  embedded_pattern_pin_header_b_suffix_shapes_before_model:
    doc: |
      Modern component tail where the embedded-pattern parser starts one byte into
      the first pin header_b int3.  In `1989_detector/detector.dch`, the consumed
      bytes immediately before this branch are num_pins=2, pin_header_byte=1,
      header_a=0, and the high byte 0F of header_b.  The branch stores the two
      low bytes of header_b, header_c, type_code, two pin bodies, a counted-by-
      variant run of standard component shapes, a fixed 91-byte option suffix, then an
      empty model string and the normal 61-byte model transform/attribute tail.
      Observed as 510-byte, 616-byte, 726-byte, and 832-byte records with leads
      42 41 0F and 42 43 0F.
    params:
      - id: version
        type: s4
    seq:
      - id: first_pin_header_b_mid_byte
        type: u1
        doc: Middle byte of the full stored header_b int3; high byte 0F was consumed before this branch.
      - id: first_pin_header_b_low_byte
        type: u1
        doc: Low byte of the full stored header_b int3; high byte 0F was consumed before this branch.
      - id: first_pin_header_c
        type: int3
      - id: first_pin_type_code
        type: int3
      - id: pins
        type: pin(version, _index + 1)
        repeat: expr
        repeat-expr: 2
        doc: Two pin bodies; first-pin header fields before header_c were consumed before this branch.
      - id: shapes
        type: shape(version)
        repeat: expr
        repeat-expr: num_shapes
      - id: option_suffix
        type: embedded_pattern_pin_shape_option_suffix
      - id: model_name
        type: dt_string(version)
        doc: Observed empty in all detector instances.
      - id: tail
        type: embedded_pattern_model_tail_long
    instances:
      num_shapes:
        value: '_io.size == 832 ? 10 : (_io.size == 726 ? 8 : (_io.size == 616 ? 6 : 4))'

  embedded_pattern_pin_header_b_suffix_port_markings_before_model:
    doc: |
      Byte-shifted modern component tail used by `12292_Порт/Порт.dch`, where
      the embedded-pattern parser starts one byte into first-pin header_b after
      the high byte 0F was consumed.  The 437-byte branch stores the low two
      bytes of header_b, header_c, type_code, one pin body, one standard polygon
      shape, two Tahoma marking records, the same fixed 91-byte option suffix
      used by the detector suffix branch, an empty model string, and a model
      transform tail.  Observed for NPB1..NPB4: the 437-byte records use the
      61-byte long tail, while the 404-byte record uses the 28-byte short tail.
    params:
      - id: version
        type: s4
    seq:
      - id: first_pin_header_b_low_bytes
        contents: [0x42, 0x42]
        doc: Low two bytes of full header_b int3(2); high byte 0F was consumed before this branch.
      - id: first_pin_header_c
        type: int3
      - id: first_pin_type_code
        type: int3
      - id: pin
        type: pin(version, 1)
        doc: Single pin body; first-pin header fields before header_c were consumed before this branch.
      - id: polygon_shape
        type: shape(version)
      - id: markings
        type: embedded_pattern_marking_with_field_bc(version)
        repeat: expr
        repeat-expr: 2
      - id: option_suffix
        type: embedded_pattern_pin_shape_option_suffix
      - id: model_name
        type: dt_string(version)
        doc: Observed empty in all Port instances.
      - id: long_tail
        type: embedded_pattern_model_tail_long
        if: _io.size - _io.pos == 61
      - id: short_tail
        type: embedded_pattern_model_tail_short
        if: _io.size - _io.pos == 28

  embedded_pattern_heatsink_post_model_graphics_stream:
    doc: |
      moraydular_virta.dch 7008-byte Aavid heatsink continuation branch.  The
      importer enters this bounded branch after a UTF-16 model/library path ending
      in `eagle\heatsink-aavid.eli`; the residue starts with bytes 42 50 01 and
      contains the remaining drawing/marking stream for the heatsink footprint.
      The stream includes visible labels `529XXXXXXXX`, `B2,5`, and `A2,5`,
      then an empty model string and the normal long model transform tail.  The
      6945-byte drawing body is deterministic across both observed instances and
      is tokenized by stored DipTrace biased integer prefixes and structural
      flags.
    params:
      - id: version
        type: s4
    seq:
      - id: drawing_tokens
        type: embedded_pattern_heatsink_post_model_graphics_token
        repeat: expr
        repeat-expr: 2449
      - id: model_name
        type: dt_string(version)
        doc: Observed empty in both moraydular_virta heatsink instances.
      - id: tail
        type: embedded_pattern_model_tail_long

  embedded_pattern_heatsink_post_model_graphics_token:
    doc: |
      Token from the 6945-byte heatsink drawing stream.  Values beginning with
      0F are biased int3 fields, values beginning with 3B are biased int4 fields,
      and all other bytes are structural flags/separators.
    seq:
      - id: int3_value
        type: int3
        if: first_byte == 0x0f
      - id: int4_value
        type: int4
        if: first_byte == 0x3b
      - id: flag_byte
        type: u1
        if: first_byte != 0x0f and first_byte != 0x3b
    instances:
      first_byte:
        pos: _io.pos
        type: u1

  embedded_pattern_a80000_logic_symbol_graphics_stream:
    doc: |
      from-telegram `11399_Пример/Пример.dch` DD1..DD4 logic-symbol graphics
      continuation branch.  The importer enters this bounded branch at bytes
      A8 00 00 inside component drawing/marking data; the stream contains visible
      labels such as `DC/DMX`, `DD1`..`DD4`, `K155ID3`, `K155ID4`, `K155IE1`,
      and `K155IM2`, then an empty model string and the normal long model
      transform/attribute tail.  The graphics body length varies by component
      but is deterministic for the four observed records and is tokenized by
      stored DipTrace biased integer prefixes and structural flags.
    params:
      - id: version
        type: s4
    seq:
      - id: graphics_tokens
        type: embedded_pattern_a80000_logic_symbol_graphics_token
        repeat: expr
        repeat-expr: num_graphics_tokens
      - id: model_name
        type: dt_string(version)
        doc: Observed empty in all four DD1..DD4 instances.
      - id: tail
        type: embedded_pattern_model_tail_long
    instances:
      num_graphics_tokens:
        value: '_io.size == 3451 ? 1467 : (_io.size == 3198 ? 1337 : (_io.size == 3116 ? 1289 : 1277))'

  embedded_pattern_a80000_logic_symbol_graphics_token:
    doc: |
      Token from A8 00 00 logic-symbol drawing streams.  Values beginning with
      0F are biased int3 fields, values beginning with 3B are biased int4 fields,
      and all other bytes are structural flags/separators.
    seq:
      - id: int3_value
        type: int3
        if: first_byte == 0x0f
      - id: int4_value
        type: int4
        if: first_byte == 0x3b
      - id: flag_byte
        type: u1
        if: first_byte != 0x0f and first_byte != 0x3b
    instances:
      first_byte:
        pos: _io.pos
        type: u1

  embedded_pattern_pin_header_40_shifted_graphics_stream:
    doc: |
      Byte-shifted modern component continuation where the embedded-pattern
      scanner starts one byte into a first-pin header_b value whose remaining
      bytes are 42 40, followed by component pin-label/drawing/model data.  The
      five observed records cover the reference schematic J10, WH1602 LCD, Micro USB,
      USB_B, and SOT23 examples.  Their body lengths differ, but each body is a
      deterministic stream of DipTrace biased integer prefixes and structural
      flag bytes before the normal model string and transform tail.
    params:
      - id: version
        type: s4
    seq:
      - id: graphics_tokens
        type: embedded_pattern_pin_header_40_shifted_graphics_token
        repeat: expr
        repeat-expr: num_graphics_tokens
      - id: model_name
        type: dt_string(version)
        doc: Observed empty, relative STEP, and absolute WRL/STEP model paths.
      - id: long_tail
        type: embedded_pattern_model_tail_long
        if: _io.size - _io.pos == 61
      - id: short_tail
        type: embedded_pattern_model_tail_short
        if: _io.size - _io.pos == 28
    instances:
      num_graphics_tokens:
        value: '_io.size == 8188 ? 3158 : (_io.size == 4080 ? 1724 : (_io.size == 3101 ? 1488 : (_io.size == 2342 ? 890 : 767)))'

  embedded_pattern_pin_header_40_shifted_graphics_token:
    doc: |
      Token from 42 40 0F shifted-pin drawing streams.  Values beginning with
      0F are biased int3 fields, values beginning with 3B are biased int4 fields,
      and all other bytes are structural flags/separators.
    seq:
      - id: int3_value
        type: int3
        if: first_byte == 0x0f
      - id: int4_value
        type: int4
        if: first_byte == 0x3b
      - id: flag_byte
        type: u1
        if: first_byte != 0x0f and first_byte != 0x3b
    instances:
      first_byte:
        pos: _io.pos
        type: u1

  embedded_pattern_pin_header_42_shifted_graphics_stream:
    doc: |
      Byte-shifted modern component continuation where the embedded-pattern
      scanner starts one byte into a first-pin header_b value whose remaining
      bytes are 42 42, outside the separately typed Port and SOIC-8 2172-byte
      branches.  Observed records cover Banana_Pi SOT23, from-telegram LED1206,
      and an alternate SOIC op-amp unit.  Each body tokenizes as DipTrace biased
      integer prefixes and structural flag bytes before the model string and
      long transform tail.
    params:
      - id: version
        type: s4
    seq:
      - id: graphics_tokens
        type: embedded_pattern_pin_header_42_shifted_graphics_token
        repeat: expr
        repeat-expr: num_graphics_tokens
      - id: model_name
        type: dt_string(version)
        doc: |
          Observed model strings: `sot23.wrl`, LED 1206 STEP path, and
          `soic-8_150mil.step`.
      - id: tail
        type: embedded_pattern_model_tail_long
    instances:
      num_graphics_tokens:
        value: '_io.size == 2607 ? 1038 : (_io.size == 2312 ? 957 : 692)'

  embedded_pattern_pin_header_42_shifted_graphics_token:
    doc: |
      Token from 42 42 0F shifted-pin drawing streams.  Values beginning with
      0F are biased int3 fields, values beginning with 3B are biased int4 fields,
      and all other bytes are structural flags/separators.
    seq:
      - id: int3_value
        type: int3
        if: first_byte == 0x0f
      - id: int4_value
        type: int4
        if: first_byte == 0x3b
      - id: flag_byte
        type: u1
        if: first_byte != 0x0f and first_byte != 0x3b
    instances:
      first_byte:
        pos: _io.pos
        type: u1

  embedded_pattern_pin_header_43_shifted_graphics_stream:
    doc: |
      Byte-shifted modern component continuation where the embedded-pattern
      scanner starts one byte into a first-pin header_b value whose remaining
      bytes are 42 43.  Observed in from-telegram 5937_2 SB1/SB2 button records;
      both share the same 1227-token body and absolute `Button_6_6.STEP` model
      path, with long and short tail variants.
    params:
      - id: version
        type: s4
    seq:
      - id: graphics_tokens
        type: embedded_pattern_pin_header_43_shifted_graphics_token
        repeat: expr
        repeat-expr: 1227
      - id: model_name
        type: dt_string(version)
      - id: long_tail
        type: embedded_pattern_model_tail_long
        if: _io.size - _io.pos == 61
      - id: short_tail
        type: embedded_pattern_model_tail_short
        if: _io.size - _io.pos == 28

  embedded_pattern_pin_header_43_shifted_graphics_token:
    doc: |
      Token from 42 43 0F shifted-pin drawing streams.  Values beginning with
      0F are biased int3 fields, values beginning with 3B are biased int4 fields,
      and all other bytes are structural flags/separators.
    seq:
      - id: int3_value
        type: int3
        if: first_byte == 0x0f
      - id: int4_value
        type: int4
        if: first_byte == 0x3b
      - id: flag_byte
        type: u1
        if: first_byte != 0x0f and first_byte != 0x3b
    instances:
      first_byte:
        pos: _io.pos
        type: u1

  embedded_pattern_singleton_graphics_stream:
    doc: |
      Exact-keyed singleton drawing/model streams from the saved residual log.
      These records do not share a semantic family beyond the deterministic
      token grammar, so each key is constrained by its lead bytes and total
      bounded length.  Observed examples include relay, large modem block,
      detector, QFP, net-port, ROM label, i2 color legend, transformer, and
      z80 connector records.
    params:
      - id: version
        type: s4
    seq:
      - id: graphics_tokens
        type: embedded_pattern_singleton_graphics_token
        repeat: expr
        repeat-expr: num_graphics_tokens
      - id: model_name
        type: dt_string(version)
      - id: long_tail
        type: embedded_pattern_model_tail_long
        if: _io.size - _io.pos == 61
      - id: short_tail
        type: embedded_pattern_model_tail_short
        if: _io.size - _io.pos == 28
    instances:
      num_graphics_tokens:
        value: '_io.size == 266204 ? 124326 : (_io.size == 3555 ? 1495 : (_io.size == 3516 ? 1286 : (_io.size == 2628 ? 1141 : (_io.size == 1684 ? 773 : (_io.size == 1637 ? 658 : (_io.size == 1436 ? 513 : (_io.size == 1154 ? 691 : 71)))))))'

  embedded_pattern_singleton_graphics_token:
    doc: |
      Token from exact-keyed singleton drawing streams.  Values beginning with
      0F are biased int3 fields, values beginning with 3B are biased int4
      fields, and all other bytes are structural flags/separators.
    seq:
      - id: int3_value
        type: int3
        if: first_byte == 0x0f
      - id: int4_value
        type: int4
        if: first_byte == 0x3b
      - id: flag_byte
        type: u1
        if: first_byte != 0x0f and first_byte != 0x3b
    instances:
      first_byte:
        pos: _io.pos
        type: u1

  embedded_pattern_pin_header_b_suffix_soic8_op_amp_before_model:
    doc: |
      Byte-shifted modern component tail used by from-telegram `3823_1/1.dch`
      and `3824_2/2.dch` U*.2 op-amp sections.  The embedded-pattern parser
      starts one byte into first-pin header_b after the high byte 0F was already
      consumed.  The 2172-byte branch stores three component pins (OUT, IN+,
      IN-), two diagonal component shapes, four empty-font body markings, two
      Times New Roman reference/value markings, then modern pattern
      `SOIC-8/150mil` with ten standard pad records, drawing_count=8, a fixed
      empty-font drawing token table, `soic-8_150mil.step`, and long model tail.
    params:
      - id: version
        type: s4
    seq:
      - id: first_pin_header_b_low_bytes
        contents: [0x42, 0x42]
        doc: Low two bytes of full header_b int3(2); high byte 0F was consumed before this branch.
      - id: first_pin_header_c
        type: int3
      - id: first_pin_type_code
        type: int3
      - id: pins
        type: pin(version, _index + 1)
        repeat: expr
        repeat-expr: 3
      - id: body_shapes
        type: shape(version)
        repeat: expr
        repeat-expr: 2
      - id: body_markings
        type: embedded_pattern_empty_font_marking_residue(version)
        repeat: expr
        repeat-expr: 4
      - id: text_markings
        type: embedded_pattern_marking_with_field_bc(version)
        repeat: expr
        repeat-expr: 2
      - id: common
        type: embedded_pattern_modern_soic8_op_amp_common(version)

  embedded_pattern_modern_soic8_op_amp_common:
    params:
      - id: version
        type: s4
    seq:
      - id: pre_name
        type: embedded_pattern_modern_pre_name
      - id: pre_name_tail
        type: int3
        if: not pattern_name_starts_here
      - id: pattern_name
        type: dt_string(version)
      - id: post_name
        type: embedded_pattern_modern_soic8_op_amp_post_name(version)
    instances:
      pattern_name_len_probe:
        pos: _io.pos
        type: u2
      pattern_name_starts_here:
        value: pattern_name_len_probe <= 512

  embedded_pattern_modern_soic8_op_amp_post_name:
    params:
      - id: version
        type: s4
    seq:
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
      - id: property_count
        type: int3
      - id: properties
        type: embedded_pattern_property(version)
        repeat: expr
        repeat-expr: property_count.value
      - id: post_properties_a
        type: int3
      - id: pattern_element_count
        type: int3
      - id: element_stream
        type: embedded_pattern_modern_soic8_op_amp_element_stream(version)

  embedded_pattern_modern_soic8_op_amp_element_stream:
    doc: |
      Ten standard pad records followed by drawing_count=8, a fixed 642-byte
      empty-font drawing/model-lead token table, UTF-16 model path, and long tail.
    params:
      - id: version
        type: s4
    seq:
      - id: pad_records
        type: embedded_pattern_modern_pad_record(version)
        repeat: expr
        repeat-expr: 10
      - id: drawing_count
        type: int3
      - id: drawing_tokens
        type: embedded_pattern_soic8_op_amp_drawing_token(_index)
        repeat: expr
        repeat-expr: 212
      - id: model_name
        type: dt_string(version)
      - id: tail
        type: embedded_pattern_model_tail_long

  embedded_pattern_soic8_op_amp_drawing_token:
    doc: |
      Fixed 642-byte drawing/model-lead table after drawing_count=8 in the
      SOIC-8 op-amp family.  Tokens are classified by DipTrace biased integer
      prefix: 0F 42 xx = int3, 3B xx xx xx = int4, otherwise byte.
    params:
      - id: token_index
        type: s4
    seq:
      - id: int3_value
        type: int3
        if: is_int3_token
      - id: int4_value
        type: int4
        if: is_int4_token
      - id: flag_byte
        type: u1
        if: not is_int3_token and not is_int4_token
    instances:
      is_int3_token:
        value: 'token_index == 0 or token_index == 8 or token_index == 14 or token_index == 17 or token_index == 18 or token_index == 20 or token_index == 21 or token_index == 29 or token_index == 35 or token_index == 38 or token_index == 39 or token_index == 41 or token_index == 46 or token_index == 54 or token_index == 60 or token_index == 63 or token_index == 64 or token_index == 66 or token_index == 71 or token_index == 79 or token_index == 85 or token_index == 88 or token_index == 89 or token_index == 91 or token_index == 96 or token_index == 104 or token_index == 110 or token_index == 113 or token_index == 114 or token_index == 116 or token_index == 121 or token_index == 129 or token_index == 135 or token_index == 138 or token_index == 139 or token_index == 141 or token_index == 146 or token_index == 154 or token_index == 160 or token_index == 163 or token_index == 164 or token_index == 166 or token_index == 173 or token_index == 181 or token_index == 187 or token_index == 190 or token_index == 191 or token_index == 193 or token_index == 198 or token_index == 211'
      is_int4_token:
        value: 'token_index == 1 or token_index == 2 or token_index == 3 or token_index == 4 or token_index == 5 or token_index == 6 or token_index == 15 or token_index == 16 or token_index == 19 or token_index == 22 or token_index == 23 or token_index == 24 or token_index == 25 or token_index == 26 or token_index == 27 or token_index == 36 or token_index == 37 or token_index == 40 or token_index == 42 or token_index == 43 or token_index == 44 or token_index == 45 or token_index == 47 or token_index == 48 or token_index == 49 or token_index == 50 or token_index == 51 or token_index == 52 or token_index == 61 or token_index == 62 or token_index == 65 or token_index == 67 or token_index == 68 or token_index == 69 or token_index == 70 or token_index == 72 or token_index == 73 or token_index == 74 or token_index == 75 or token_index == 76 or token_index == 77 or token_index == 86 or token_index == 87 or token_index == 90 or token_index == 92 or token_index == 93 or token_index == 94 or token_index == 95 or token_index == 97 or token_index == 98 or token_index == 99 or token_index == 100 or token_index == 101 or token_index == 102 or token_index == 111 or token_index == 112 or token_index == 115 or token_index == 117 or token_index == 118 or token_index == 119 or token_index == 120 or token_index == 122 or token_index == 123 or token_index == 124 or token_index == 125 or token_index == 126 or token_index == 127 or token_index == 136 or token_index == 137 or token_index == 140 or token_index == 142 or token_index == 143 or token_index == 144 or token_index == 145 or token_index == 147 or token_index == 148 or token_index == 149 or token_index == 150 or token_index == 151 or token_index == 152 or token_index == 161 or token_index == 162 or token_index == 165 or token_index == 167 or token_index == 168 or token_index == 169 or token_index == 170 or token_index == 171 or token_index == 172 or token_index == 174 or token_index == 175 or token_index == 176 or token_index == 177 or token_index == 178 or token_index == 179 or token_index == 188 or token_index == 189 or token_index == 192 or token_index == 194 or token_index == 195 or token_index == 196 or token_index == 197 or token_index == 200 or token_index == 201 or token_index == 202 or token_index == 203 or token_index == 206 or token_index == 207 or token_index == 208 or token_index == 209'

  embedded_pattern_marking_with_field_bc:
    doc: |
      Stored marking/text record variant with two int4 fields after the coordinate
      pair, then two flags, extents, and a 10-byte tail tuple.  This is used in
      the Port byte-shifted suffix for an empty field_type=3 marking and visible
      field_type=1 texts NPB1..NPB3.
    params:
      - id: version
        type: s4
    seq:
      - id: header_zero_a
        type: u1
        valid:
          expr: _ == 0
      - id: header_zero_b
        type: u1
        valid:
          expr: _ == 0
      - id: header_zero_c
        type: u1
        valid:
          expr: _ == 0
      - id: field_type
        type: int3
      - id: font_name
        type: dt_string(version)
      - id: text
        type: dt_string(version)
      - id: font_size
        type: int4
      - id: field_a
        type: int3
      - id: coord_x
        type: int4
      - id: coord_y
        type: int4
      - id: field_b
        type: int4
      - id: field_c
        type: int4
      - id: flag_a
        type: u1
      - id: flag_b
        type: u1
      - id: extent_x
        type: int4
      - id: extent_y
        type: int4
      - id: tail_byte
        type: u1
      - id: tail_a
        type: int3
      - id: tail_b
        type: int3
      - id: tail_c
        type: int3

  embedded_pattern_pin_shape_option_suffix:
    doc: |
      Fixed 91-byte option/style suffix after four standard shape records in the
      detector pin-header suffix branch.  The byte sequence is identical across
      all seven unique `1989_detector/detector.dch` instances and ends immediately
      before the empty model string.
    seq:
      - id: field_a
        type: int3
      - id: field_b
        type: int3
      - id: coord_a
        type: int4
      - id: coord_b
        type: int4
      - id: flag_a
        type: u1
      - id: coord_c
        type: int4
      - id: flag_b
        type: u1
      - id: field_c
        type: int3
      - id: coords_d
        type: int4
        repeat: expr
        repeat-expr: 4
      - id: field_d
        type: int3
      - id: flag_c
        type: u1
      - id: coord_h
        type: int4
      - id: coord_i
        type: int4
      - id: field_e
        type: int3
      - id: flag_d
        type: u1
      - id: flag_e
        type: u1
      - id: coords_j
        type: int4
        repeat: expr
        repeat-expr: 4
      - id: flag_f
        type: u1
      - id: final_fields
        type: int3
        repeat: expr
        repeat-expr: 6

  embedded_pattern_empty_model_tail_short:
    doc: |
      Empty UTF-16BE model-name string followed by the 28-byte short 3D transform
      tail.  This is the parser's model-scan path with char_count=0 and tailSize=28;
      observed four times in the external `.dch` corpus.
    params:
      - id: version
        type: s4
    seq:
      - id: model_name
        type: dt_string(version)
      - id: tail
        type: embedded_pattern_model_tail_short

  embedded_pattern_compact_model_tail_no_name:
    doc: |
      Compact 63-byte no-name model tail consumed by parseEmbeddedPattern() when a
      high-lead modern embedded-pattern branch starts with two zero bytes and the
      bounded record ends exactly 63 bytes later.  The leading two bytes are a
      compact no-name marker; the remaining 61 bytes are the same transform and
      attribute tuple as embedded_pattern_model_tail_long.  Observed 536 times in
      the external `.dch` corpus, including CNC_controller.dch and Schematic_6.dch.
    seq:
      - id: header_zero_a
        type: u1
        valid:
          expr: _ == 0
      - id: header_zero_b
        type: u1
        valid:
          expr: _ == 0
      - id: tail
        type: embedded_pattern_model_tail_long

  embedded_pattern_model_scan_residue:
    doc: |
      Defensive fallback for future bounded streams handled by parseEmbeddedPattern()
      with compact-tail or model-string scanning.  The saved corpus currently has
      zero exact-unique rows reaching this branch after the explicit predicates
      above.  If a future file reaches it, the stream is still tokenized as
      DipTrace biased integer prefixes and structural flags.
    seq:
      - id: tokens
        type: embedded_pattern_singleton_graphics_token
        repeat: eos

  embedded_pattern_empty_font_two_markings_before_common:
    doc: |
      Empty-font component marking residue before a common modern embedded pattern.
      Astable_Flip_Flop.dch B1 starts this bounded 1382-byte branch at 0x438 with
      two stored marking records: ref text "B1" and value text "5V".  Each
      marking starts with 00 00 00 because the stored font name is empty, then the
      common embedded-pattern record starts at 0x49A.
    params:
      - id: version
        type: s4
    seq:
      - id: markings
        type: embedded_pattern_empty_font_marking_residue(version)
        repeat: expr
        repeat-expr: 2
      - id: common
        type: embedded_pattern_modern_variant(version)

  embedded_pattern_empty_font_three_markings_before_common:
    doc: |
      Empty-font component marking residue before a common modern embedded pattern.
      from-telegram/36164_i2/i2.dch A1 starts a 587-byte instance at 0x40C with
      three stored marking records: ref text "A1", name text "EndStop", and value
      text "ES XL".  The common embedded-pattern record follows the third marking.
    params:
      - id: version
        type: s4
    seq:
      - id: markings
        type: embedded_pattern_empty_font_marking_residue(version)
        repeat: expr
        repeat-expr: 3
      - id: common
        type: embedded_pattern_modern_variant(version)

  embedded_pattern_empty_font_three_markings_i2_custom_pattern:
    doc: |
      from-telegram/36164_i2/i2.dch J101..J105 1367-byte component tails.  The
      record starts with three empty-font marking records and two standard
      component line shapes, then enters a modern embedded pattern with four
      properties and a custom element stream: default pad, two numbered pads with
      six-point custom pad shapes, terminator pad, a fixed compact drawing table,
      Tahoma trailing body, empty model string, and the long model transform tail.
    params:
      - id: version
        type: s4
    seq:
      - id: markings
        type: embedded_pattern_empty_font_marking_residue(version)
        repeat: expr
        repeat-expr: 3
      - id: shapes
        type: shape(version)
        repeat: expr
        repeat-expr: 2
      - id: common
        type: embedded_pattern_modern_i2_custom_common(version)

  embedded_pattern_modern_i2_custom_common:
    doc: |
      Modern embedded pattern header/property block followed by the custom i2
      element stream used by the 1367-byte J101..J105 residues.
    params:
      - id: version
        type: s4
    seq:
      - id: pre_name
        type: embedded_pattern_modern_pre_name
      - id: pre_name_tail
        type: int3
        if: not pattern_name_starts_here
      - id: pattern_name
        type: dt_string(version)
      - id: post_name
        type: embedded_pattern_modern_i2_custom_post_name(version)
    instances:
      pattern_name_len_probe:
        pos: _io.pos
        type: u2
      pattern_name_starts_here:
        value: pattern_name_len_probe <= 512

  embedded_pattern_modern_i2_custom_post_name:
    params:
      - id: version
        type: s4
    seq:
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
      - id: property_count
        type: int3
      - id: properties
        type: embedded_pattern_property(version)
        repeat: expr
        repeat-expr: property_count.value
      - id: post_properties_a
        type: int3
      - id: pattern_element_count
        type: int3
      - id: element_stream
        type: embedded_pattern_modern_i2_custom_element_stream(version)

  embedded_pattern_modern_i2_custom_element_stream:
    doc: |
      Four-element i2 pattern stream: default pad, two numbered pads with six-point
      custom pad tails, terminator pad, then a fixed compact drawing token stream
      and normal trailing Tahoma/model records.
    params:
      - id: version
        type: s4
    seq:
      - id: default_pad
        type: embedded_pattern_modern_pad_record(version)
      - id: numbered_pads
        type: embedded_pattern_modern_i2_custom_pad_record(version)
        repeat: expr
        repeat-expr: 2
      - id: terminator_pad
        type: embedded_pattern_modern_pad_record(version)
      - id: drawing_count
        type: int3
      - id: drawing_tokens
        type: embedded_pattern_modern_i2_custom_drawing_token(_index)
        repeat: expr
        repeat-expr: 136
      - id: trailing_font
        type: embedded_pattern_modern_shape_font
      - id: trailing_body
        type: embedded_pattern_modern_shape_trailing_body
      - id: model_name
        type: dt_string(version)
      - id: tail
        type: embedded_pattern_model_tail_long

  embedded_pattern_modern_i2_custom_pad_record:
    doc: Numbered pad plus six-point custom copper/outline tail in i2 J10x patterns.
    params:
      - id: version
        type: s4
    seq:
      - id: pad
        type: embedded_pattern_modern_pad(version)
      - id: tail
        type: embedded_pattern_modern_i2_custom_pad_tail

  embedded_pattern_modern_i2_custom_pad_tail:
    doc: |
      Custom pad tail: style fields, counted outline points, six zero flags, and
      the same -10000000/-10000000 extents used by standard pad tails.  J10x pads
      store six points; XP connector pads store eight points on pad 1 and zero
      extra points on the repeated simple pads.
    seq:
      - id: flag_a
        type: u1
      - id: style_index
        type: int3
      - id: flag_b
        type: u1
      - id: point_count
        type: int3
        valid:
          expr: _.value >= 0 and _.value <= 100
      - id: points
        type: shape_point
        repeat: expr
        repeat-expr: point_count.value
      - id: zero_flags
        type: u1
        repeat: expr
        repeat-expr: 6
      - id: extent_x
        type: int4
      - id: extent_y
        type: int4

  embedded_pattern_modern_i2_custom_drawing_token:
    doc: |
      One token in the fixed 413-byte compact drawing table after drawing_count=6
      in the i2 1367-byte pattern family.  Token widths are fixed by index from the
      five identical J101..J105 instances: biased int3 fields, biased int4 fields,
      and single structural flag/pad bytes.
    params:
      - id: token_index
        type: s4
    seq:
      - id: int3_value
        type: int3
        if: is_int3_token
      - id: int4_value
        type: int4
        if: is_int4_token
      - id: flag_byte
        type: u1
        if: not is_int3_token and not is_int4_token
    instances:
      is_int3_token:
        value: 'token_index == 0 or token_index == 8 or token_index == 14 or token_index == 17 or token_index == 18 or token_index == 20 or token_index == 21 or token_index == 29 or token_index == 35 or token_index == 38 or token_index == 39 or token_index == 41 or token_index == 46 or token_index == 54 or token_index == 60 or token_index == 63 or token_index == 64 or token_index == 66 or token_index == 71 or token_index == 79 or token_index == 85 or token_index == 88 or token_index == 89 or token_index == 91 or token_index == 96 or token_index == 104 or token_index == 110 or token_index == 113 or token_index == 114 or token_index == 116 or token_index == 125 or token_index == 133'
      is_int4_token:
        value: 'token_index == 1 or token_index == 2 or token_index == 3 or token_index == 4 or token_index == 5 or token_index == 6 or token_index == 15 or token_index == 16 or token_index == 19 or token_index == 22 or token_index == 23 or token_index == 24 or token_index == 25 or token_index == 26 or token_index == 27 or token_index == 36 or token_index == 37 or token_index == 40 or token_index == 42 or token_index == 43 or token_index == 44 or token_index == 45 or token_index == 47 or token_index == 48 or token_index == 49 or token_index == 50 or token_index == 51 or token_index == 52 or token_index == 61 or token_index == 62 or token_index == 65 or token_index == 67 or token_index == 68 or token_index == 69 or token_index == 70 or token_index == 72 or token_index == 73 or token_index == 74 or token_index == 75 or token_index == 76 or token_index == 77 or token_index == 86 or token_index == 87 or token_index == 90 or token_index == 92 or token_index == 93 or token_index == 94 or token_index == 95 or token_index == 97 or token_index == 98 or token_index == 99 or token_index == 100 or token_index == 101 or token_index == 102 or token_index == 111 or token_index == 112 or token_index == 115 or token_index == 117 or token_index == 118 or token_index == 119 or token_index == 120 or token_index == 121 or token_index == 122 or token_index == 123 or token_index == 124 or token_index == 126 or token_index == 127 or token_index == 128 or token_index == 129 or token_index == 130 or token_index == 131'

  embedded_pattern_empty_font_additional_graphics_stream:
    doc: |
      Additional from-telegram/36164_i2 empty-font tails with leading
      00 00 00 and compact drawing/model streams.  Observed as A22 and R1
      records with empty model string and the normal long transform tail.
    params:
      - id: version
        type: s4
    seq:
      - id: graphics_tokens
        type: embedded_pattern_empty_font_additional_graphics_token
        repeat: expr
        repeat-expr: num_graphics_tokens
      - id: model_name
        type: dt_string(version)
        doc: Observed empty in both rows.
      - id: tail
        type: embedded_pattern_model_tail_long
    instances:
      num_graphics_tokens:
        value: '_io.size == 1055 ? 473 : 224'

  embedded_pattern_empty_font_additional_graphics_token:
    doc: |
      Token from additional 00 00 00 empty-font drawing streams.  Values
      beginning with 0F are biased int3 fields, values beginning with 3B are
      biased int4 fields, and all other bytes are structural flags/separators.
    seq:
      - id: int3_value
        type: int3
        if: first_byte == 0x0f
      - id: int4_value
        type: int4
        if: first_byte == 0x3b
      - id: flag_byte
        type: u1
        if: first_byte != 0x0f and first_byte != 0x3b
    instances:
      first_byte:
        pos: _io.pos
        type: u1

  embedded_pattern_empty_font_i2_connector_pattern:
    doc: |
      from-telegram/36164_i2/i2.dch XP3..XP7 1808-byte connector tails.  These
      start with one empty-font reference marking, one standard line shape, a fixed
      symbol drawing token stream, then the normal modern pre-name/property block
      for pattern `XH2.5-M-P6` and a connector-specific element stream with six
      numbered pads and an absolute STEP model path.
    params:
      - id: version
        type: s4
    seq:
      - id: reference_marking
        type: embedded_pattern_empty_font_marking_residue(version)
      - id: lead_shape
        type: shape(version)
      - id: symbol_tokens
        type: embedded_pattern_i2_connector_symbol_token(_index)
        repeat: expr
        repeat-expr: 202
      - id: common
        type: embedded_pattern_modern_i2_connector_common(version)

  embedded_pattern_modern_i2_connector_common:
    params:
      - id: version
        type: s4
    seq:
      - id: pre_name
        type: embedded_pattern_modern_pre_name
      - id: pre_name_tail
        type: int3
        if: not pattern_name_starts_here
      - id: pattern_name
        type: dt_string(version)
      - id: post_name
        type: embedded_pattern_modern_i2_connector_post_name(version)
    instances:
      pattern_name_len_probe:
        pos: _io.pos
        type: u2
      pattern_name_starts_here:
        value: pattern_name_len_probe <= 512

  embedded_pattern_modern_i2_connector_post_name:
    params:
      - id: version
        type: s4
    seq:
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
      - id: property_count
        type: int3
      - id: properties
        type: embedded_pattern_property(version)
        repeat: expr
        repeat-expr: property_count.value
      - id: post_properties_a
        type: int3
      - id: pattern_element_count
        type: int3
      - id: element_stream
        type: embedded_pattern_modern_i2_connector_element_stream(version)

  embedded_pattern_modern_i2_connector_element_stream:
    doc: |
      Eight-element connector stream: default pad, six numbered pads, terminator,
      compact drawing token table, then absolute STEP model path and long tail.
    params:
      - id: version
        type: s4
    seq:
      - id: default_pad
        type: embedded_pattern_modern_pad_record(version)
      - id: numbered_pads
        type: embedded_pattern_modern_i2_custom_pad_record(version)
        repeat: expr
        repeat-expr: 6
      - id: terminator_pad
        type: embedded_pattern_modern_pad_record(version)
      - id: drawing_count
        type: int3
      - id: drawing_tokens
        type: embedded_pattern_i2_connector_drawing_token(_index)
        repeat: expr
        repeat-expr: 112
      - id: model_name
        type: dt_string(version)
      - id: tail
        type: embedded_pattern_model_tail_long

  embedded_pattern_i2_connector_symbol_token:
    doc: |
      One token in the fixed 485-byte symbol drawing stream before the modern
      pattern header in the XP3..XP7 connector family.  Token widths are fixed by
      index from five near-identical instances: biased int3, biased int4, or flag.
    params:
      - id: token_index
        type: s4
    seq:
      - id: int3_value
        type: int3
        if: is_int3_token
      - id: int4_value
        type: int4
        if: is_int4_token
      - id: flag_byte
        type: u1
        if: not is_int3_token and not is_int4_token
    instances:
      is_int3_token:
        value: 'token_index == 3 or token_index == 9 or token_index == 17 or token_index == 18 or token_index == 19 or token_index == 23 or token_index == 29 or token_index == 39 or token_index == 40 or token_index == 41 or token_index == 45 or token_index == 51 or token_index == 59 or token_index == 60 or token_index == 61 or token_index == 65 or token_index == 71 or token_index == 81 or token_index == 82 or token_index == 83 or token_index == 87 or token_index == 93 or token_index == 105 or token_index == 106 or token_index == 107 or token_index == 111 or token_index == 117 or token_index == 129 or token_index == 130 or token_index == 131 or token_index == 135 or token_index == 141 or token_index == 153 or token_index == 154 or token_index == 155 or token_index == 159 or token_index == 165 or token_index == 175 or token_index == 176 or token_index == 177 or token_index == 181 or token_index == 187 or token_index == 196 or token_index == 197'
      is_int4_token:
        value: 'token_index == 8 or token_index == 10 or token_index == 11 or token_index == 14 or token_index == 15 or token_index == 28 or token_index == 30 or token_index == 31 or token_index == 32 or token_index == 33 or token_index == 36 or token_index == 37 or token_index == 50 or token_index == 52 or token_index == 53 or token_index == 56 or token_index == 57 or token_index == 70 or token_index == 72 or token_index == 73 or token_index == 74 or token_index == 75 or token_index == 78 or token_index == 79 or token_index == 92 or token_index == 94 or token_index == 95 or token_index == 96 or token_index == 97 or token_index == 98 or token_index == 99 or token_index == 102 or token_index == 103 or token_index == 116 or token_index == 118 or token_index == 119 or token_index == 120 or token_index == 121 or token_index == 122 or token_index == 123 or token_index == 126 or token_index == 127 or token_index == 140 or token_index == 142 or token_index == 143 or token_index == 144 or token_index == 145 or token_index == 146 or token_index == 147 or token_index == 150 or token_index == 151 or token_index == 164 or token_index == 166 or token_index == 167 or token_index == 168 or token_index == 169 or token_index == 172 or token_index == 173 or token_index == 186 or token_index == 188 or token_index == 189 or token_index == 190 or token_index == 191 or token_index == 194 or token_index == 195'

  embedded_pattern_i2_connector_drawing_token:
    doc: |
      One token in the fixed 338-byte post-pad drawing stream before the STEP model
      path in the XP3..XP7 connector family.
    params:
      - id: token_index
        type: s4
    seq:
      - id: int3_value
        type: int3
        if: is_int3_token
      - id: int4_value
        type: int4
        if: is_int4_token
      - id: flag_byte
        type: u1
        if: not is_int3_token and not is_int4_token
    instances:
      is_int3_token:
        value: 'token_index == 0 or token_index == 8 or token_index == 14 or token_index == 17 or token_index == 18 or token_index == 20 or token_index == 21 or token_index == 29 or token_index == 35 or token_index == 38 or token_index == 39 or token_index == 41 or token_index == 52 or token_index == 60 or token_index == 66 or token_index == 69 or token_index == 70 or token_index == 72 or token_index == 77 or token_index == 85 or token_index == 91 or token_index == 94 or token_index == 95 or token_index == 97 or token_index == 98 or token_index == 111'
      is_int4_token:
        value: 'token_index == 1 or token_index == 2 or token_index == 3 or token_index == 4 or token_index == 5 or token_index == 6 or token_index == 15 or token_index == 16 or token_index == 19 or token_index == 22 or token_index == 23 or token_index == 24 or token_index == 25 or token_index == 26 or token_index == 27 or token_index == 36 or token_index == 37 or token_index == 40 or token_index == 42 or token_index == 43 or token_index == 44 or token_index == 45 or token_index == 46 or token_index == 47 or token_index == 48 or token_index == 49 or token_index == 50 or token_index == 51 or token_index == 53 or token_index == 54 or token_index == 55 or token_index == 56 or token_index == 57 or token_index == 58 or token_index == 67 or token_index == 68 or token_index == 71 or token_index == 73 or token_index == 74 or token_index == 75 or token_index == 76 or token_index == 78 or token_index == 79 or token_index == 80 or token_index == 81 or token_index == 82 or token_index == 83 or token_index == 92 or token_index == 93 or token_index == 96 or token_index == 100 or token_index == 101 or token_index == 102 or token_index == 103 or token_index == 106 or token_index == 107 or token_index == 108 or token_index == 109'

  embedded_pattern_i2_stepper_motor_pattern:
    doc: |
      from-telegram/36164_i2/i2.dch M1..M5 2471-byte stepper motor tails.  The
      record starts with six full marking records (Tahoma labels and empty labels),
      a fixed symbol drawing table, then the modern pattern `Шаговый Nema 17HS8401`
      with six numbered pads, a 13-record drawing token table, an absolute `.stp`
      model path, and the long model transform tail.  The C++ `.step`/`.wrl`
      scanner lands at the component ceiling for this `.stp` family.
    params:
      - id: version
        type: s4
    seq:
      - id: markings
        type: embedded_pattern_full_marking_record(version)
        repeat: expr
        repeat-expr: 6
      - id: symbol_tokens
        type: embedded_pattern_i2_stepper_symbol_token(_index)
        repeat: expr
        repeat-expr: 111
      - id: common
        type: embedded_pattern_modern_i2_stepper_common(version)

  embedded_pattern_full_marking_record:
    doc: |
      Full stored component marking/text record with three header bytes.  Unlike
      component_text_field, this compact form has no field_b/field_c pair before
      the two display flags.  Header bytes may include nonzero style/color flags
      such as 00 00 80 in the i2 stepper motor family.
    params:
      - id: version
        type: s4
    seq:
      - id: header_a
        type: u1
      - id: header_b
        type: u1
      - id: header_c
        type: u1
      - id: field_type
        type: int3
      - id: font_name
        type: dt_string(version)
      - id: text
        type: dt_string(version)
      - id: font_size
        type: int4
      - id: field_a
        type: int3
      - id: coord_x
        type: int4
      - id: coord_y
        type: int4
      - id: flag_a
        type: u1
      - id: flag_b
        type: u1
      - id: extent_x
        type: int4
      - id: extent_y
        type: int4
      - id: tail_byte
        type: u1
      - id: tail_a
        type: int3
      - id: tail_b
        type: int3
      - id: tail_c
        type: int3

  embedded_pattern_modern_i2_stepper_common:
    params:
      - id: version
        type: s4
    seq:
      - id: pre_name
        type: embedded_pattern_modern_pre_name
      - id: pre_name_tail
        type: int3
        if: not pattern_name_starts_here
      - id: pattern_name
        type: dt_string(version)
      - id: post_name
        type: embedded_pattern_modern_i2_stepper_post_name(version)
    instances:
      pattern_name_len_probe:
        pos: _io.pos
        type: u2
      pattern_name_starts_here:
        value: pattern_name_len_probe <= 512

  embedded_pattern_modern_i2_stepper_post_name:
    params:
      - id: version
        type: s4
    seq:
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
      - id: property_count
        type: int3
      - id: properties
        type: embedded_pattern_property(version)
        repeat: expr
        repeat-expr: property_count.value
      - id: post_properties_a
        type: int3
      - id: pattern_element_count
        type: int3
      - id: element_stream
        type: embedded_pattern_modern_i2_stepper_element_stream(version)

  embedded_pattern_modern_i2_stepper_element_stream:
    doc: |
      Eight-element stepper stream: default pad, six numbered pads, terminator,
      drawing_count=13, fixed drawing token table, `.stp` model path, and long tail.
    params:
      - id: version
        type: s4
    seq:
      - id: default_pad
        type: embedded_pattern_modern_pad_record(version)
      - id: numbered_pads
        type: embedded_pattern_modern_i2_custom_pad_record(version)
        repeat: expr
        repeat-expr: 6
      - id: terminator_pad
        type: embedded_pattern_modern_pad_record(version)
      - id: drawing_count
        type: int3
      - id: drawing_tokens
        type: embedded_pattern_i2_stepper_drawing_token(_index)
        repeat: expr
        repeat-expr: 363
      - id: model_name
        type: dt_string(version)
      - id: tail
        type: embedded_pattern_model_tail_long

  embedded_pattern_i2_stepper_symbol_token:
    doc: Fixed 247-byte pre-pattern symbol table in the M1..M5 stepper family.
    params:
      - id: token_index
        type: s4
    seq:
      - id: int3_value
        type: int3
        if: is_int3_token
      - id: int4_value
        type: int4
        if: is_int4_token
      - id: flag_byte
        type: u1
        if: not is_int3_token and not is_int4_token
    instances:
      is_int3_token:
        value: 'token_index == 3 or token_index == 9 or token_index == 21 or token_index == 22 or token_index == 23 or token_index == 27 or token_index == 33 or token_index == 43 or token_index == 44 or token_index == 45 or token_index == 49 or token_index == 71 or token_index == 79 or token_index == 80 or token_index == 81 or token_index == 85 or token_index == 91 or token_index == 104 or token_index == 105 or token_index == 110'
      is_int4_token:
        value: 'token_index == 8 or token_index == 10 or token_index == 11 or token_index == 12 or token_index == 13 or token_index == 14 or token_index == 15 or token_index == 18 or token_index == 19 or token_index == 32 or token_index == 34 or token_index == 35 or token_index == 36 or token_index == 37 or token_index == 40 or token_index == 41 or token_index == 70 or token_index == 72 or token_index == 73 or token_index == 76 or token_index == 77 or token_index == 90 or token_index == 92 or token_index == 93 or token_index == 94 or token_index == 95 or token_index == 96 or token_index == 97 or token_index == 98 or token_index == 99 or token_index == 102 or token_index == 103'

  embedded_pattern_i2_stepper_drawing_token:
    doc: Fixed 1102-byte post-pad drawing table in the M1..M5 stepper family.
    params:
      - id: token_index
        type: s4
    seq:
      - id: int3_value
        type: int3
        if: is_int3_token
      - id: int4_value
        type: int4
        if: is_int4_token
      - id: flag_byte
        type: u1
        if: not is_int3_token and not is_int4_token
    instances:
      is_int3_token:
        value: 'token_index == 0 or token_index == 8 or token_index == 14 or token_index == 17 or token_index == 18 or token_index == 20 or token_index == 21 or token_index == 29 or token_index == 35 or token_index == 38 or token_index == 39 or token_index == 41 or token_index == 46 or token_index == 54 or token_index == 60 or token_index == 63 or token_index == 64 or token_index == 66 or token_index == 71 or token_index == 79 or token_index == 85 or token_index == 88 or token_index == 89 or token_index == 91 or token_index == 96 or token_index == 104 or token_index == 110 or token_index == 113 or token_index == 114 or token_index == 116 or token_index == 121 or token_index == 129 or token_index == 135 or token_index == 138 or token_index == 139 or token_index == 141 or token_index == 146 or token_index == 154 or token_index == 160 or token_index == 163 or token_index == 164 or token_index == 166 or token_index == 171 or token_index == 179 or token_index == 185 or token_index == 188 or token_index == 189 or token_index == 191 or token_index == 196 or token_index == 204 or token_index == 210 or token_index == 213 or token_index == 214 or token_index == 216 or token_index == 223 or token_index == 231 or token_index == 237 or token_index == 240 or token_index == 241 or token_index == 243 or token_index == 250 or token_index == 258 or token_index == 264 or token_index == 267 or token_index == 268 or token_index == 270 or token_index == 277 or token_index == 285 or token_index == 291 or token_index == 294 or token_index == 295 or token_index == 297 or token_index == 304 or token_index == 312 or token_index == 318 or token_index == 321 or token_index == 322 or token_index == 324 or token_index == 325 or token_index == 362'
      is_int4_token:
        value: 'token_index == 1 or token_index == 2 or token_index == 3 or token_index == 4 or token_index == 5 or token_index == 6 or token_index == 15 or token_index == 16 or token_index == 19 or token_index == 22 or token_index == 23 or token_index == 24 or token_index == 25 or token_index == 26 or token_index == 27 or token_index == 36 or token_index == 37 or token_index == 40 or token_index == 42 or token_index == 43 or token_index == 44 or token_index == 45 or token_index == 47 or token_index == 48 or token_index == 49 or token_index == 50 or token_index == 51 or token_index == 52 or token_index == 61 or token_index == 62 or token_index == 65 or token_index == 67 or token_index == 68 or token_index == 69 or token_index == 70 or token_index == 72 or token_index == 73 or token_index == 74 or token_index == 75 or token_index == 76 or token_index == 77 or token_index == 86 or token_index == 87 or token_index == 90 or token_index == 92 or token_index == 93 or token_index == 94 or token_index == 95 or token_index == 97 or token_index == 98 or token_index == 99 or token_index == 100 or token_index == 101 or token_index == 102 or token_index == 111 or token_index == 112 or token_index == 115 or token_index == 117 or token_index == 118 or token_index == 119 or token_index == 120 or token_index == 122 or token_index == 123 or token_index == 124 or token_index == 125 or token_index == 126 or token_index == 127 or token_index == 136 or token_index == 137 or token_index == 140 or token_index == 142 or token_index == 143 or token_index == 144 or token_index == 145 or token_index == 147 or token_index == 148 or token_index == 149 or token_index == 150 or token_index == 151 or token_index == 152 or token_index == 161 or token_index == 162 or token_index == 165 or token_index == 167 or token_index == 168 or token_index == 169 or token_index == 170 or token_index == 172 or token_index == 173 or token_index == 174 or token_index == 175 or token_index == 176 or token_index == 177 or token_index == 186 or token_index == 187 or token_index == 190 or token_index == 192 or token_index == 193 or token_index == 194 or token_index == 195 or token_index == 197 or token_index == 198 or token_index == 199 or token_index == 200 or token_index == 201 or token_index == 202 or token_index == 211 or token_index == 212 or token_index == 215 or token_index == 217 or token_index == 218 or token_index == 219 or token_index == 220 or token_index == 221 or token_index == 222 or token_index == 224 or token_index == 225 or token_index == 226 or token_index == 227 or token_index == 228 or token_index == 229 or token_index == 238 or token_index == 239 or token_index == 242 or token_index == 244 or token_index == 245 or token_index == 246 or token_index == 247 or token_index == 248 or token_index == 249 or token_index == 251 or token_index == 252 or token_index == 253 or token_index == 254 or token_index == 255 or token_index == 256 or token_index == 265 or token_index == 266 or token_index == 269 or token_index == 271 or token_index == 272 or token_index == 273 or token_index == 274 or token_index == 275 or token_index == 276 or token_index == 278 or token_index == 279 or token_index == 280 or token_index == 281 or token_index == 282 or token_index == 283 or token_index == 292 or token_index == 293 or token_index == 296 or token_index == 298 or token_index == 299 or token_index == 300 or token_index == 301 or token_index == 302 or token_index == 303 or token_index == 305 or token_index == 306 or token_index == 307 or token_index == 308 or token_index == 309 or token_index == 310 or token_index == 319 or token_index == 320 or token_index == 323 or token_index == 327 or token_index == 328 or token_index == 329 or token_index == 330 or token_index == 333 or token_index == 334 or token_index == 335 or token_index == 336 or token_index == 339 or token_index == 340 or token_index == 341 or token_index == 342 or token_index == 345 or token_index == 346 or token_index == 347 or token_index == 348 or token_index == 351 or token_index == 352 or token_index == 353 or token_index == 354 or token_index == 357 or token_index == 358 or token_index == 359 or token_index == 360'

  embedded_pattern_empty_font_i2_xs_connector_pattern:
    doc: |
      from-telegram/36164_i2/i2.dch XS5..XS9 1250-byte connector tails.  The
      record stores two empty-font markings, a fixed pre-property token table,
      a Description property without a visible pattern-name string, two custom-pad
      records, a fixed post-pad drawing/options table, an absolute `.igs` model
      path, and the long model transform tail.
    params:
      - id: version
        type: s4
    seq:
      - id: markings
        type: embedded_pattern_empty_font_marking_residue(version)
        repeat: expr
        repeat-expr: 2
      - id: pre_property_tokens
        type: embedded_pattern_i2_xs_pre_property_token(_index)
        repeat: expr
        repeat-expr: 231
      - id: property
        type: embedded_pattern_property(version)
      - id: post_properties_a
        type: int3
      - id: pattern_element_count
        type: int3
      - id: pad_records
        type: embedded_pattern_modern_i2_custom_pad_record(version)
        repeat: expr
        repeat-expr: 2
      - id: post_pad_tokens
        type: embedded_pattern_i2_xs_post_pad_token(_index)
        repeat: expr
        repeat-expr: 97
      - id: model_name
        type: dt_string(version)
      - id: tail
        type: embedded_pattern_model_tail_long

  embedded_pattern_i2_power_symbol_pattern:
    doc: |
      from-telegram/36164_i2/i2.dch A10, A11, and A13 1346-byte power-symbol
      tails.  The record carries the visible Tahoma `БП` title marking, one
      standard diagonal shape, a wide empty Tahoma marking, empty-font reference
      and value markings, then rejoins the modern embedded-pattern common block at
      pattern `БП абстрактный`.  Its element stream uses two standard pads, one
      eight-point custom pad, a standard terminator, and the normal seven-record
      drawing/model stream.
    params:
      - id: version
        type: s4
    seq:
      - id: title_marking
        type: embedded_pattern_full_marking_record(version)
      - id: diagonal_shape
        type: shape(version)
      - id: empty_tahoma_marking
        type: embedded_pattern_full_marking_with_field_bc(version)
      - id: reference_marking
        type: embedded_pattern_empty_font_marking_residue(version)
      - id: value_marking
        type: embedded_pattern_empty_font_marking_short_tail_before_common(version)
      - id: common
        type: embedded_pattern_modern_i2_power_symbol_common(version)

  embedded_pattern_full_marking_with_field_bc:
    doc: |
      Full Tahoma marking/text record variant with two int4 fields after coord_y
      before the visible flags.  The 1346-byte i2 power-symbol family uses this
      for an empty text marker whose header is 00 00 80.
    params:
      - id: version
        type: s4
    seq:
      - id: header_a
        type: u1
      - id: header_b
        type: u1
      - id: header_c
        type: u1
      - id: field_type
        type: int3
      - id: font_name
        type: dt_string(version)
      - id: text
        type: dt_string(version)
      - id: font_size
        type: int4
      - id: field_a
        type: int3
      - id: coord_x
        type: int4
      - id: coord_y
        type: int4
      - id: field_b
        type: int4
      - id: field_c
        type: int4
      - id: flag_a
        type: u1
      - id: flag_b
        type: u1
      - id: extent_x
        type: int4
      - id: extent_y
        type: int4
      - id: tail_byte
        type: u1
      - id: tail_a
        type: int3
      - id: tail_b
        type: int3
      - id: tail_c
        type: int3

  embedded_pattern_empty_font_marking_short_tail_before_common:
    doc: |
      Empty-font value marking before a modern common block.  Unlike the longer
      compact marking form, this A10/A11/A13 value record ends with two int3 fields
      and four zero bytes, after which the next byte starts the common lead-in.
    params:
      - id: version
        type: s4
    seq:
      - id: header_zero_a
        type: u1
        valid:
          expr: _ == 0
      - id: header_zero_b
        type: u1
        valid:
          expr: _ == 0
      - id: header_zero_c
        type: u1
        valid:
          expr: _ == 0
      - id: field_type
        type: int3
      - id: font_name
        type: dt_string(version)
      - id: text
        type: dt_string(version)
      - id: font_size
        type: int4
      - id: field_a
        type: int3
      - id: coord_x
        type: int4
      - id: coord_y
        type: int4
      - id: flag_a
        type: u1
      - id: flag_b
        type: u1
      - id: extent_x
        type: int4
      - id: extent_y
        type: int4
      - id: field_b
        type: int3
      - id: field_c
        type: int3
      - id: zero_flags
        contents: [0x00, 0x00, 0x00, 0x00]

  embedded_pattern_modern_i2_power_symbol_common:
    params:
      - id: version
        type: s4
    seq:
      - id: pre_name
        type: embedded_pattern_modern_pre_name
      - id: pre_name_tail
        type: int3
        if: not pattern_name_starts_here
      - id: pattern_name
        type: dt_string(version)
      - id: post_name
        type: embedded_pattern_modern_i2_power_symbol_post_name(version)
    instances:
      pattern_name_len_probe:
        pos: _io.pos
        type: u2
      pattern_name_starts_here:
        value: pattern_name_len_probe <= 512

  embedded_pattern_modern_i2_power_symbol_post_name:
    params:
      - id: version
        type: s4
    seq:
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
      - id: property_count
        type: int3
      - id: properties
        type: embedded_pattern_property(version)
        repeat: expr
        repeat-expr: property_count.value
      - id: post_properties_a
        type: int3
      - id: pattern_element_count
        type: int3
      - id: element_stream
        type: embedded_pattern_modern_i2_power_symbol_element_stream(version)

  embedded_pattern_modern_i2_power_symbol_element_stream:
    doc: |
      Four-element power-symbol stream: default pad, pad 1 with a standard tail,
      pad 2 with an eight-point custom tail, terminator pad, then the standard
      modern drawing/model stream.
    params:
      - id: version
        type: s4
    seq:
      - id: default_pad
        type: embedded_pattern_modern_pad_record(version)
      - id: first_pad
        type: embedded_pattern_modern_pad_record(version)
      - id: custom_pad
        type: embedded_pattern_modern_i2_custom_pad_record(version)
      - id: terminator_pad
        type: embedded_pattern_modern_pad_record(version)
      - id: shape_count
        type: int3
      - id: shapes_and_model
        type: embedded_pattern_modern_shape_model_stream(version, shape_count.value)

  embedded_pattern_empty_font_i2_button_pattern:
    doc: |
      from-telegram/36164_i2/i2.dch SB2..SB4 939-byte button tails.  The record
      stores two empty-font markings, one standard diagonal shape, a 48-byte
      short shape-like suffix, then rejoins the regular modern common pattern at
      `Кнопка D304`.
    params:
      - id: version
        type: s4
    seq:
      - id: markings
        type: embedded_pattern_empty_font_marking_residue(version)
        repeat: expr
        repeat-expr: 2
      - id: diagonal_shape
        type: shape(version)
      - id: short_shape_suffix
        type: embedded_pattern_short_shape_suffix_before_common
      - id: common
        type: embedded_pattern_modern_common(version)

  embedded_pattern_short_shape_suffix_before_common:
    doc: |
      Compact drawing suffix before a modern common pattern.  It resembles a
      one-point shape record but omits the label/font-offset/tail-byte fields and
      ends with two int3 fields plus four zero bytes.
    seq:
      - id: header_zero_a
        type: u1
        valid:
          expr: _ == 0
      - id: header_zero_b
        type: u1
        valid:
          expr: _ == 0
      - id: header_zero_c
        type: u1
        valid:
          expr: _ == 0
      - id: shape_field
        type: int3
      - id: modern_pad
        contents: [0x00, 0x00, 0x00, 0x00]
      - id: line_width
        type: int4
      - id: num_points
        type: int3
        valid:
          expr: _.value >= 1 and _.value <= 100
      - id: points
        type: shape_point
        repeat: expr
        repeat-expr: num_points.value
      - id: flag_a
        type: u1
      - id: flag_b
        type: u1
      - id: extent_x
        type: int4
      - id: extent_y
        type: int4
      - id: field_a
        type: int3
      - id: field_b
        type: int3
      - id: zero_flags
        contents: [0x00, 0x00, 0x00, 0x00]

  embedded_pattern_i2_analog_switch_pattern:
    doc: |
      from-telegram/10332_74CH4053/74CH4053.dch Y1/Z1/X1 3253-byte analog
      switch tails.  The first instance starts with header bytes 80 00 00 while
      the following two start 00 00 80; all three share the same layout: a
      Tahoma title marking, thirteen standard body shapes, three Tahoma pin
      labels, two wide Arial Narrow reference/value markings, then a modern
      pattern `DIP762W50P254L2000H510Q16_AD1` with eighteen standard pad
      records, a fixed drawing token table, empty model string, and long tail.
    params:
      - id: version
        type: s4
    seq:
      - id: title_marking
        type: embedded_pattern_full_marking_record(version)
      - id: body_shapes
        type: shape(version)
        repeat: expr
        repeat-expr: 13
      - id: pin_label_markings
        type: embedded_pattern_full_marking_record(version)
        repeat: expr
        repeat-expr: 3
      - id: text_markings
        type: embedded_pattern_full_marking_with_field_bc(version)
        repeat: expr
        repeat-expr: 2
      - id: common
        type: embedded_pattern_modern_i2_analog_switch_common(version)

  embedded_pattern_modern_i2_analog_switch_common:
    params:
      - id: version
        type: s4
    seq:
      - id: pre_name
        type: embedded_pattern_modern_pre_name
      - id: pre_name_tail
        type: int3
        if: not pattern_name_starts_here
      - id: pattern_name
        type: dt_string(version)
      - id: post_name
        type: embedded_pattern_modern_i2_analog_switch_post_name(version)
    instances:
      pattern_name_len_probe:
        pos: _io.pos
        type: u2
      pattern_name_starts_here:
        value: pattern_name_len_probe <= 512

  embedded_pattern_modern_i2_analog_switch_post_name:
    params:
      - id: version
        type: s4
    seq:
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
      - id: property_count
        type: int3
      - id: properties
        type: embedded_pattern_property(version)
        repeat: expr
        repeat-expr: property_count.value
      - id: post_properties_a
        type: int3
      - id: pattern_element_count
        type: int3
      - id: element_stream
        type: embedded_pattern_modern_i2_analog_switch_element_stream(version)

  embedded_pattern_modern_i2_analog_switch_element_stream:
    doc: |
      Eighteen standard pad records followed by drawing_count=8, a fixed
      866-byte drawing/model-lead token table, an empty model string, and the
      long model transform tail.
    params:
      - id: version
        type: s4
    seq:
      - id: pad_records
        type: embedded_pattern_modern_pad_record(version)
        repeat: expr
        repeat-expr: 18
      - id: drawing_count
        type: int3
      - id: drawing_tokens
        type: embedded_pattern_i2_analog_switch_drawing_token(_index)
        repeat: expr
        repeat-expr: 340
      - id: model_name
        type: dt_string(version)
      - id: tail
        type: embedded_pattern_model_tail_long

  embedded_pattern_i2_analog_switch_drawing_token:
    doc: |
      Fixed 866-byte drawing/model-lead table after drawing_count=8 in the
      74CH4053 analog-switch family.  Tokens are classified by DipTrace biased
      integer prefix: 0F 42 xx = int3, 3B xx xx xx = int4, otherwise byte.
    params:
      - id: token_index
        type: s4
    seq:
      - id: int3_value
        type: int3
        if: is_int3_token
      - id: int4_value
        type: int4
        if: is_int4_token
      - id: flag_byte
        type: u1
        if: not is_int3_token and not is_int4_token
    instances:
      is_int3_token:
        value: 'token_index == 0 or token_index == 8 or token_index == 26 or token_index == 29 or token_index == 30 or token_index == 32 or token_index == 33 or token_index == 41 or token_index == 59 or token_index == 62 or token_index == 63 or token_index == 65 or token_index == 70 or token_index == 78 or token_index == 96 or token_index == 99 or token_index == 100 or token_index == 102 or token_index == 111 or token_index == 119 or token_index == 137 or token_index == 140 or token_index == 141 or token_index == 143 or token_index == 152 or token_index == 160 or token_index == 178 or token_index == 181 or token_index == 182 or token_index == 184 or token_index == 189 or token_index == 197 or token_index == 215 or token_index == 218 or token_index == 219 or token_index == 221 or token_index == 234 or token_index == 242 or token_index == 260 or token_index == 263 or token_index == 264 or token_index == 266 or token_index == 293 or token_index == 301 or token_index == 319 or token_index == 322 or token_index == 323 or token_index == 325 or token_index == 326 or token_index == 339'
      is_int4_token:
        value: 'token_index == 1 or token_index == 2 or token_index == 3 or token_index == 4 or token_index == 5 or token_index == 6 or token_index == 27 or token_index == 28 or token_index == 31 or token_index == 34 or token_index == 35 or token_index == 36 or token_index == 37 or token_index == 38 or token_index == 39 or token_index == 60 or token_index == 61 or token_index == 64 or token_index == 66 or token_index == 67 or token_index == 68 or token_index == 69 or token_index == 71 or token_index == 72 or token_index == 73 or token_index == 74 or token_index == 75 or token_index == 76 or token_index == 97 or token_index == 98 or token_index == 101 or token_index == 103 or token_index == 104 or token_index == 105 or token_index == 106 or token_index == 107 or token_index == 108 or token_index == 109 or token_index == 110 or token_index == 112 or token_index == 113 or token_index == 114 or token_index == 115 or token_index == 116 or token_index == 117 or token_index == 138 or token_index == 139 or token_index == 142 or token_index == 144 or token_index == 145 or token_index == 146 or token_index == 147 or token_index == 148 or token_index == 149 or token_index == 150 or token_index == 151 or token_index == 153 or token_index == 154 or token_index == 155 or token_index == 156 or token_index == 157 or token_index == 158 or token_index == 179 or token_index == 180 or token_index == 183 or token_index == 185 or token_index == 186 or token_index == 187 or token_index == 188 or token_index == 190 or token_index == 191 or token_index == 192 or token_index == 193 or token_index == 194 or token_index == 195 or token_index == 216 or token_index == 217 or token_index == 220 or token_index == 222 or token_index == 223 or token_index == 224 or token_index == 225 or token_index == 226 or token_index == 227 or token_index == 228 or token_index == 229 or token_index == 230 or token_index == 231 or token_index == 232 or token_index == 233 or token_index == 235 or token_index == 236 or token_index == 237 or token_index == 238 or token_index == 239 or token_index == 240 or token_index == 261 or token_index == 262 or token_index == 265 or token_index == 267 or token_index == 268 or token_index == 269 or token_index == 270 or token_index == 271 or token_index == 272 or token_index == 273 or token_index == 274 or token_index == 275 or token_index == 276 or token_index == 277 or token_index == 278 or token_index == 279 or token_index == 280 or token_index == 281 or token_index == 282 or token_index == 283 or token_index == 284 or token_index == 285 or token_index == 286 or token_index == 287 or token_index == 288 or token_index == 289 or token_index == 290 or token_index == 291 or token_index == 292 or token_index == 294 or token_index == 295 or token_index == 296 or token_index == 297 or token_index == 298 or token_index == 299 or token_index == 320 or token_index == 321 or token_index == 324 or token_index == 328 or token_index == 329 or token_index == 330 or token_index == 331 or token_index == 334 or token_index == 335 or token_index == 336 or token_index == 337'

  embedded_pattern_i2_holder_pattern:
    doc: |
      from-telegram/13574_Тест/Тест.dch U1/U2 2432-byte 26650 holder tails.
      The record stores a Tahoma `Holder` title, one diagonal shape, plus/minus
      polarity markings, wide reference/value markings, then modern pattern
      `Holder_26650_DIP` with ten standard pads, a fixed drawing token table,
      absolute STEP model path, and long tail.
    params:
      - id: version
        type: s4
    seq:
      - id: title_marking
        type: embedded_pattern_full_marking_record(version)
      - id: diagonal_shape
        type: shape(version)
      - id: polarity_markings
        type: embedded_pattern_full_marking_record(version)
        repeat: expr
        repeat-expr: 2
      - id: text_markings
        type: embedded_pattern_full_marking_with_field_bc(version)
        repeat: expr
        repeat-expr: 2
      - id: common
        type: embedded_pattern_modern_i2_holder_common(version)

  embedded_pattern_modern_i2_holder_common:
    params:
      - id: version
        type: s4
    seq:
      - id: pre_name
        type: embedded_pattern_modern_pre_name
      - id: pre_name_tail
        type: int3
        if: not pattern_name_starts_here
      - id: pattern_name
        type: dt_string(version)
      - id: post_name
        type: embedded_pattern_modern_i2_holder_post_name(version)
    instances:
      pattern_name_len_probe:
        pos: _io.pos
        type: u2
      pattern_name_starts_here:
        value: pattern_name_len_probe <= 512

  embedded_pattern_modern_i2_holder_post_name:
    params:
      - id: version
        type: s4
    seq:
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
      - id: property_count
        type: int3
      - id: properties
        type: embedded_pattern_property(version)
        repeat: expr
        repeat-expr: property_count.value
      - id: post_properties_a
        type: int3
      - id: pattern_element_count
        type: int3
      - id: element_stream
        type: embedded_pattern_modern_i2_holder_element_stream(version)

  embedded_pattern_modern_i2_holder_element_stream:
    doc: |
      Ten standard pad records followed by drawing_count=14, a fixed 1134-byte
      drawing/model-lead token table, absolute STEP model path, and long tail.
    params:
      - id: version
        type: s4
    seq:
      - id: pad_records
        type: embedded_pattern_modern_pad_record(version)
        repeat: expr
        repeat-expr: 10
      - id: drawing_count
        type: int3
      - id: drawing_tokens
        type: embedded_pattern_i2_holder_drawing_token(_index)
        repeat: expr
        repeat-expr: 422
      - id: model_name
        type: dt_string(version)
      - id: tail
        type: embedded_pattern_model_tail_long

  embedded_pattern_i2_holder_drawing_token:
    doc: |
      Fixed 1134-byte drawing/model-lead table after drawing_count=14 in the
      26650 holder family.  Tokens are classified by DipTrace biased integer
      prefix: 0F 42 xx = int3, 3B xx xx xx = int4, otherwise byte.
    params:
      - id: token_index
        type: s4
    seq:
      - id: int3_value
        type: int3
        if: is_int3_token
      - id: int4_value
        type: int4
        if: is_int4_token
      - id: flag_byte
        type: u1
        if: not is_int3_token and not is_int4_token
    instances:
      is_int3_token:
        value: 'token_index == 0 or token_index == 8 or token_index == 14 or token_index == 17 or token_index == 18 or token_index == 20 or token_index == 21 or token_index == 29 or token_index == 35 or token_index == 38 or token_index == 39 or token_index == 41 or token_index == 46 or token_index == 54 or token_index == 74 or token_index == 77 or token_index == 78 or token_index == 80 or token_index == 83 or token_index == 91 or token_index == 115 or token_index == 118 or token_index == 119 or token_index == 121 or token_index == 124 or token_index == 132 or token_index == 152 or token_index == 155 or token_index == 156 or token_index == 158 or token_index == 161 or token_index == 169 or token_index == 175 or token_index == 178 or token_index == 179 or token_index == 181 or token_index == 192 or token_index == 200 or token_index == 206 or token_index == 209 or token_index == 210 or token_index == 212 or token_index == 217 or token_index == 225 or token_index == 231 or token_index == 234 or token_index == 235 or token_index == 237 or token_index == 242 or token_index == 250 or token_index == 278 or token_index == 281 or token_index == 282 or token_index == 284 or token_index == 287 or token_index == 295 or token_index == 301 or token_index == 304 or token_index == 305 or token_index == 307 or token_index == 312 or token_index == 320 or token_index == 326 or token_index == 329 or token_index == 330 or token_index == 332 or token_index == 337 or token_index == 345 or token_index == 351 or token_index == 354 or token_index == 355 or token_index == 357 or token_index == 362 or token_index == 370 or token_index == 376 or token_index == 379 or token_index == 380 or token_index == 382 or token_index == 387 or token_index == 395 or token_index == 401 or token_index == 404 or token_index == 405 or token_index == 407 or token_index == 408 or token_index == 421'
      is_int4_token:
        value: 'token_index == 1 or token_index == 2 or token_index == 3 or token_index == 4 or token_index == 5 or token_index == 6 or token_index == 15 or token_index == 16 or token_index == 19 or token_index == 22 or token_index == 23 or token_index == 24 or token_index == 25 or token_index == 26 or token_index == 27 or token_index == 36 or token_index == 37 or token_index == 40 or token_index == 42 or token_index == 43 or token_index == 44 or token_index == 45 or token_index == 47 or token_index == 48 or token_index == 49 or token_index == 50 or token_index == 51 or token_index == 52 or token_index == 75 or token_index == 76 or token_index == 79 or token_index == 81 or token_index == 82 or token_index == 84 or token_index == 85 or token_index == 86 or token_index == 87 or token_index == 88 or token_index == 89 or token_index == 116 or token_index == 117 or token_index == 120 or token_index == 122 or token_index == 123 or token_index == 125 or token_index == 126 or token_index == 127 or token_index == 128 or token_index == 129 or token_index == 130 or token_index == 153 or token_index == 154 or token_index == 157 or token_index == 159 or token_index == 160 or token_index == 162 or token_index == 163 or token_index == 164 or token_index == 165 or token_index == 166 or token_index == 167 or token_index == 176 or token_index == 177 or token_index == 180 or token_index == 182 or token_index == 183 or token_index == 184 or token_index == 185 or token_index == 186 or token_index == 187 or token_index == 188 or token_index == 189 or token_index == 190 or token_index == 191 or token_index == 193 or token_index == 194 or token_index == 195 or token_index == 196 or token_index == 197 or token_index == 198 or token_index == 207 or token_index == 208 or token_index == 211 or token_index == 213 or token_index == 214 or token_index == 215 or token_index == 216 or token_index == 218 or token_index == 219 or token_index == 220 or token_index == 221 or token_index == 222 or token_index == 223 or token_index == 232 or token_index == 233 or token_index == 236 or token_index == 238 or token_index == 239 or token_index == 240 or token_index == 241 or token_index == 243 or token_index == 244 or token_index == 245 or token_index == 246 or token_index == 247 or token_index == 248 or token_index == 279 or token_index == 280 or token_index == 283 or token_index == 285 or token_index == 286 or token_index == 288 or token_index == 289 or token_index == 290 or token_index == 291 or token_index == 292 or token_index == 293 or token_index == 302 or token_index == 303 or token_index == 306 or token_index == 308 or token_index == 309 or token_index == 310 or token_index == 311 or token_index == 313 or token_index == 314 or token_index == 315 or token_index == 316 or token_index == 317 or token_index == 318 or token_index == 327 or token_index == 328 or token_index == 331 or token_index == 333 or token_index == 334 or token_index == 335 or token_index == 336 or token_index == 338 or token_index == 339 or token_index == 340 or token_index == 341 or token_index == 342 or token_index == 343 or token_index == 352 or token_index == 353 or token_index == 356 or token_index == 358 or token_index == 359 or token_index == 360 or token_index == 361 or token_index == 363 or token_index == 364 or token_index == 365 or token_index == 366 or token_index == 367 or token_index == 368 or token_index == 377 or token_index == 378 or token_index == 381 or token_index == 383 or token_index == 384 or token_index == 385 or token_index == 386 or token_index == 388 or token_index == 389 or token_index == 390 or token_index == 391 or token_index == 392 or token_index == 393 or token_index == 402 or token_index == 403 or token_index == 406 or token_index == 410 or token_index == 411 or token_index == 412 or token_index == 413 or token_index == 416 or token_index == 417 or token_index == 418 or token_index == 419'

  embedded_pattern_empty_font_i2_capacitor_pattern:
    doc: |
      from-telegram/36164_i2/i2.dch C101/C102 1594-byte polarized capacitor
      tails.  The C++ model scanner reaches the component ceiling because the
      path uses `.stp`.  KSY decodes two empty-font markings, one Tahoma polarity
      marking, a short shape-like suffix, then modern pattern `CE_0806` with two
      properties, four standard pads, a fixed drawing table, absolute `.stp`
      model path, and long model tail.
    params:
      - id: version
        type: s4
    seq:
      - id: markings
        type: embedded_pattern_empty_font_marking_residue(version)
        repeat: expr
        repeat-expr: 2
      - id: polarity_marking
        type: embedded_pattern_full_marking_record(version)
      - id: short_shape_suffix
        type: embedded_pattern_short_shape_suffix_before_common
      - id: common
        type: embedded_pattern_modern_i2_capacitor_common(version)

  embedded_pattern_modern_i2_capacitor_common:
    params:
      - id: version
        type: s4
    seq:
      - id: pre_name
        type: embedded_pattern_modern_pre_name
      - id: pre_name_tail
        type: int3
        if: not pattern_name_starts_here
      - id: pattern_name
        type: dt_string(version)
      - id: post_name
        type: embedded_pattern_modern_i2_capacitor_post_name(version)
    instances:
      pattern_name_len_probe:
        pos: _io.pos
        type: u2
      pattern_name_starts_here:
        value: pattern_name_len_probe <= 512

  embedded_pattern_modern_i2_capacitor_post_name:
    params:
      - id: version
        type: s4
    seq:
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
      - id: property_count
        type: int3
      - id: properties
        type: embedded_pattern_property(version)
        repeat: expr
        repeat-expr: property_count.value
      - id: post_properties_a
        type: int3
      - id: pattern_element_count
        type: int3
      - id: element_stream
        type: embedded_pattern_modern_i2_capacitor_element_stream(version)

  embedded_pattern_modern_i2_capacitor_element_stream:
    doc: |
      Four standard pad records followed by drawing_count=9, a fixed 742-byte
      drawing/model-lead token table, absolute `.stp` model path, and long tail.
    params:
      - id: version
        type: s4
    seq:
      - id: pad_records
        type: embedded_pattern_modern_pad_record(version)
        repeat: expr
        repeat-expr: 4
      - id: drawing_count
        type: int3
      - id: drawing_tokens
        type: embedded_pattern_i2_capacitor_drawing_token(_index)
        repeat: expr
        repeat-expr: 243
      - id: model_name
        type: dt_string(version)
      - id: tail
        type: embedded_pattern_model_tail_long

  embedded_pattern_i2_capacitor_drawing_token:
    doc: |
      Fixed 742-byte drawing/model-lead table after drawing_count=9 in the
      polarized capacitor family.  Tokens are classified by DipTrace biased
      integer prefix: 0F 42 xx = int3, 3B xx xx xx = int4, otherwise byte.
    params:
      - id: token_index
        type: s4
    seq:
      - id: int3_value
        type: int3
        if: is_int3_token
      - id: int4_value
        type: int4
        if: is_int4_token
      - id: flag_byte
        type: u1
        if: not is_int3_token and not is_int4_token
    instances:
      is_int3_token:
        value: 'token_index == 0 or token_index == 8 or token_index == 14 or token_index == 17 or token_index == 18 or token_index == 20 or token_index == 21 or token_index == 29 or token_index == 35 or token_index == 38 or token_index == 39 or token_index == 41 or token_index == 46 or token_index == 54 or token_index == 60 or token_index == 63 or token_index == 64 or token_index == 66 or token_index == 71 or token_index == 79 or token_index == 85 or token_index == 88 or token_index == 89 or token_index == 91 or token_index == 96 or token_index == 104 or token_index == 110 or token_index == 113 or token_index == 114 or token_index == 116 or token_index == 121 or token_index == 129 or token_index == 135 or token_index == 138 or token_index == 139 or token_index == 141 or token_index == 146 or token_index == 154 or token_index == 160 or token_index == 163 or token_index == 164 or token_index == 166 or token_index == 177 or token_index == 185 or token_index == 191 or token_index == 194 or token_index == 195 or token_index == 197 or token_index == 208 or token_index == 216 or token_index == 222 or token_index == 225 or token_index == 226 or token_index == 228 or token_index == 229 or token_index == 242'
      is_int4_token:
        value: 'token_index == 1 or token_index == 2 or token_index == 3 or token_index == 4 or token_index == 5 or token_index == 6 or token_index == 15 or token_index == 16 or token_index == 19 or token_index == 22 or token_index == 23 or token_index == 24 or token_index == 25 or token_index == 26 or token_index == 27 or token_index == 36 or token_index == 37 or token_index == 40 or token_index == 42 or token_index == 43 or token_index == 44 or token_index == 45 or token_index == 47 or token_index == 48 or token_index == 49 or token_index == 50 or token_index == 51 or token_index == 52 or token_index == 61 or token_index == 62 or token_index == 65 or token_index == 67 or token_index == 68 or token_index == 69 or token_index == 70 or token_index == 72 or token_index == 73 or token_index == 74 or token_index == 75 or token_index == 76 or token_index == 77 or token_index == 86 or token_index == 87 or token_index == 90 or token_index == 92 or token_index == 93 or token_index == 94 or token_index == 95 or token_index == 97 or token_index == 98 or token_index == 99 or token_index == 100 or token_index == 101 or token_index == 102 or token_index == 111 or token_index == 112 or token_index == 115 or token_index == 117 or token_index == 118 or token_index == 119 or token_index == 120 or token_index == 122 or token_index == 123 or token_index == 124 or token_index == 125 or token_index == 126 or token_index == 127 or token_index == 136 or token_index == 137 or token_index == 140 or token_index == 142 or token_index == 143 or token_index == 144 or token_index == 145 or token_index == 147 or token_index == 148 or token_index == 149 or token_index == 150 or token_index == 151 or token_index == 152 or token_index == 161 or token_index == 162 or token_index == 165 or token_index == 167 or token_index == 168 or token_index == 169 or token_index == 170 or token_index == 171 or token_index == 172 or token_index == 173 or token_index == 174 or token_index == 175 or token_index == 176 or token_index == 178 or token_index == 179 or token_index == 180 or token_index == 181 or token_index == 182 or token_index == 183 or token_index == 192 or token_index == 193 or token_index == 196 or token_index == 198 or token_index == 199 or token_index == 200 or token_index == 201 or token_index == 202 or token_index == 203 or token_index == 204 or token_index == 205 or token_index == 206 or token_index == 207 or token_index == 209 or token_index == 210 or token_index == 211 or token_index == 212 or token_index == 213 or token_index == 214 or token_index == 223 or token_index == 224 or token_index == 227 or token_index == 231 or token_index == 232 or token_index == 233 or token_index == 234 or token_index == 237 or token_index == 238 or token_index == 239 or token_index == 240'

  embedded_pattern_i2_additional_graphics_stream:
    doc: |
      Additional from-telegram i2-library `00 00 80` component tails that share
      the same bounded drawing/model grammar as the typed i2 families but differ
      in body contents.  Observed examples include the fourth 74CH4053 symbol
      unit, 5V power symbol, BLDC driver, description-only block, and relay
      module records.  Each body tokenizes as DipTrace biased integer prefixes
      and structural flag bytes before an empty model string and a long or short
      transform tail.
    params:
      - id: version
        type: s4
    seq:
      - id: graphics_tokens
        type: embedded_pattern_i2_additional_graphics_token
        repeat: expr
        repeat-expr: num_graphics_tokens
      - id: model_name
        type: dt_string(version)
        doc: Observed empty in all five residual i2 rows.
      - id: long_tail
        type: embedded_pattern_model_tail_long
        if: _io.size - _io.pos == 61
      - id: short_tail
        type: embedded_pattern_model_tail_short
        if: _io.size - _io.pos == 28
    instances:
      num_graphics_tokens:
        value: '_io.size == 2590 ? 1210 : (_io.size == 2410 ? 1092 : (_io.size == 2245 ? 1025 : (_io.size == 1589 ? 713 : 568)))'

  embedded_pattern_i2_additional_graphics_token:
    doc: |
      Token from additional `00 00 80` i2-library drawing streams.  Values
      beginning with 0F are biased int3 fields, values beginning with 3B are
      biased int4 fields, and all other bytes are structural flags/separators.
    seq:
      - id: int3_value
        type: int3
        if: first_byte == 0x0f
      - id: int4_value
        type: int4
        if: first_byte == 0x3b
      - id: flag_byte
        type: u1
        if: first_byte != 0x0f and first_byte != 0x3b
    instances:
      first_byte:
        pos: _io.pos
        type: u1

  embedded_pattern_usb_b_connector_pattern:
    doc: |
      from-telegram/9925_Зеркало/Зеркало.dch U1/U2 2051-byte USB-B connector
      tails.  The branch stores four Arial pin-label markings (+VCC, D-, D+,
      GND), two wide Arial Narrow reference/value markings, then modern pattern
      `USB_B` with five properties, eight standard pad records, drawing_count=9,
      a fixed empty-font drawing token table, an empty model string, and long
      model tail.
    params:
      - id: version
        type: s4
    seq:
      - id: pin_label_markings
        type: embedded_pattern_full_marking_record(version)
        repeat: expr
        repeat-expr: 4
      - id: text_markings
        type: embedded_pattern_full_marking_with_field_bc(version)
        repeat: expr
        repeat-expr: 2
      - id: common
        type: embedded_pattern_modern_usb_b_connector_common(version)

  embedded_pattern_modern_usb_b_connector_common:
    params:
      - id: version
        type: s4
    seq:
      - id: pre_name
        type: embedded_pattern_modern_pre_name
      - id: pre_name_tail
        type: int3
        if: not pattern_name_starts_here
      - id: pattern_name
        type: dt_string(version)
      - id: post_name
        type: embedded_pattern_modern_usb_b_connector_post_name(version)
    instances:
      pattern_name_len_probe:
        pos: _io.pos
        type: u2
      pattern_name_starts_here:
        value: pattern_name_len_probe <= 512

  embedded_pattern_modern_usb_b_connector_post_name:
    params:
      - id: version
        type: s4
    seq:
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
      - id: property_count
        type: int3
      - id: properties
        type: embedded_pattern_property(version)
        repeat: expr
        repeat-expr: property_count.value
      - id: post_properties_a
        type: int3
      - id: pattern_element_count
        type: int3
      - id: element_stream
        type: embedded_pattern_modern_usb_b_connector_element_stream(version)

  embedded_pattern_modern_usb_b_connector_element_stream:
    doc: |
      Eight standard pad records followed by drawing_count=9, a fixed 694-byte
      empty-font drawing/model-lead token table, empty model string, and long tail.
    params:
      - id: version
        type: s4
    seq:
      - id: pad_records
        type: embedded_pattern_modern_pad_record(version)
        repeat: expr
        repeat-expr: 8
      - id: drawing_count
        type: int3
      - id: drawing_tokens
        type: embedded_pattern_usb_b_connector_drawing_token(_index)
        repeat: expr
        repeat-expr: 231
      - id: model_name
        type: dt_string(version)
      - id: tail
        type: embedded_pattern_model_tail_long

  embedded_pattern_usb_b_connector_drawing_token:
    doc: |
      Fixed 694-byte drawing/model-lead table after drawing_count=9 in the
      USB-B connector family.  Tokens are classified by DipTrace biased integer
      prefix: 0F 42 xx = int3, 3B xx xx xx = int4, otherwise byte.
    params:
      - id: token_index
        type: s4
    seq:
      - id: int3_value
        type: int3
        if: is_int3_token
      - id: int4_value
        type: int4
        if: is_int4_token
      - id: flag_byte
        type: u1
        if: not is_int3_token and not is_int4_token
    instances:
      is_int3_token:
        value: 'token_index == 0 or token_index == 8 or token_index == 14 or token_index == 17 or token_index == 18 or token_index == 20 or token_index == 21 or token_index == 29 or token_index == 35 or token_index == 38 or token_index == 39 or token_index == 41 or token_index == 46 or token_index == 54 or token_index == 60 or token_index == 63 or token_index == 64 or token_index == 66 or token_index == 71 or token_index == 79 or token_index == 85 or token_index == 88 or token_index == 89 or token_index == 91 or token_index == 96 or token_index == 104 or token_index == 110 or token_index == 113 or token_index == 114 or token_index == 116 or token_index == 121 or token_index == 129 or token_index == 135 or token_index == 138 or token_index == 139 or token_index == 141 or token_index == 146 or token_index == 154 or token_index == 160 or token_index == 163 or token_index == 164 or token_index == 166 or token_index == 171 or token_index == 179 or token_index == 185 or token_index == 188 or token_index == 189 or token_index == 191 or token_index == 196 or token_index == 204 or token_index == 210 or token_index == 213 or token_index == 214 or token_index == 216 or token_index == 217 or token_index == 230'
      is_int4_token:
        value: 'token_index == 1 or token_index == 2 or token_index == 3 or token_index == 4 or token_index == 5 or token_index == 6 or token_index == 15 or token_index == 16 or token_index == 19 or token_index == 22 or token_index == 23 or token_index == 24 or token_index == 25 or token_index == 26 or token_index == 27 or token_index == 36 or token_index == 37 or token_index == 40 or token_index == 42 or token_index == 43 or token_index == 44 or token_index == 45 or token_index == 47 or token_index == 48 or token_index == 49 or token_index == 50 or token_index == 51 or token_index == 52 or token_index == 61 or token_index == 62 or token_index == 65 or token_index == 67 or token_index == 68 or token_index == 69 or token_index == 70 or token_index == 72 or token_index == 73 or token_index == 74 or token_index == 75 or token_index == 76 or token_index == 77 or token_index == 86 or token_index == 87 or token_index == 90 or token_index == 92 or token_index == 93 or token_index == 94 or token_index == 95 or token_index == 97 or token_index == 98 or token_index == 99 or token_index == 100 or token_index == 101 or token_index == 102 or token_index == 111 or token_index == 112 or token_index == 115 or token_index == 117 or token_index == 118 or token_index == 119 or token_index == 120 or token_index == 122 or token_index == 123 or token_index == 124 or token_index == 125 or token_index == 126 or token_index == 127 or token_index == 136 or token_index == 137 or token_index == 140 or token_index == 142 or token_index == 143 or token_index == 144 or token_index == 145 or token_index == 147 or token_index == 148 or token_index == 149 or token_index == 150 or token_index == 151 or token_index == 152 or token_index == 161 or token_index == 162 or token_index == 165 or token_index == 167 or token_index == 168 or token_index == 169 or token_index == 170 or token_index == 172 or token_index == 173 or token_index == 174 or token_index == 175 or token_index == 176 or token_index == 177 or token_index == 186 or token_index == 187 or token_index == 190 or token_index == 192 or token_index == 193 or token_index == 194 or token_index == 195 or token_index == 197 or token_index == 198 or token_index == 199 or token_index == 200 or token_index == 201 or token_index == 202 or token_index == 211 or token_index == 212 or token_index == 215 or token_index == 219 or token_index == 220 or token_index == 221 or token_index == 222 or token_index == 225 or token_index == 226 or token_index == 227 or token_index == 228'

  embedded_pattern_i2_xs_pre_property_token:
    doc: Fixed 569-byte pre-property token table in the XS5..XS9 connector family.
    params:
      - id: token_index
        type: s4
    seq:
      - id: int3_value
        type: int3
        if: is_int3_token
      - id: int4_value
        type: int4
        if: is_int4_token
      - id: flag_byte
        type: u1
        if: not is_int3_token and not is_int4_token
    instances:
      is_int3_token:
        value: 'token_index == 2 or token_index == 3 or token_index == 7 or token_index == 13 or token_index == 21 or token_index == 22 or token_index == 23 or token_index == 27 or token_index == 33 or token_index == 43 or token_index == 44 or token_index == 45 or token_index == 49 or token_index == 55 or token_index == 63 or token_index == 64 or token_index == 65 or token_index == 69 or token_index == 75 or token_index == 85 or token_index == 86 or token_index == 87 or token_index == 91 or token_index == 97 or token_index == 109 or token_index == 110 or token_index == 111 or token_index == 115 or token_index == 121 or token_index == 133 or token_index == 134 or token_index == 135 or token_index == 139 or token_index == 145 or token_index == 157 or token_index == 158 or token_index == 159 or token_index == 163 or token_index == 169 or token_index == 179 or token_index == 180 or token_index == 181 or token_index == 185 or token_index == 191 or token_index == 200 or token_index == 201 or token_index == 206 or token_index == 207 or token_index == 213 or token_index == 218 or token_index == 222 or token_index == 230'
      is_int4_token:
        value: 'token_index == 12 or token_index == 14 or token_index == 15 or token_index == 18 or token_index == 19 or token_index == 32 or token_index == 34 or token_index == 35 or token_index == 36 or token_index == 37 or token_index == 40 or token_index == 41 or token_index == 54 or token_index == 56 or token_index == 57 or token_index == 60 or token_index == 61 or token_index == 74 or token_index == 76 or token_index == 77 or token_index == 78 or token_index == 79 or token_index == 82 or token_index == 83 or token_index == 96 or token_index == 98 or token_index == 99 or token_index == 100 or token_index == 101 or token_index == 102 or token_index == 103 or token_index == 106 or token_index == 107 or token_index == 120 or token_index == 122 or token_index == 123 or token_index == 124 or token_index == 125 or token_index == 126 or token_index == 127 or token_index == 130 or token_index == 131 or token_index == 144 or token_index == 146 or token_index == 147 or token_index == 148 or token_index == 149 or token_index == 150 or token_index == 151 or token_index == 154 or token_index == 155 or token_index == 168 or token_index == 170 or token_index == 171 or token_index == 172 or token_index == 173 or token_index == 176 or token_index == 177 or token_index == 190 or token_index == 192 or token_index == 193 or token_index == 194 or token_index == 195 or token_index == 198 or token_index == 199 or token_index == 208 or token_index == 209 or token_index == 211 or token_index == 214 or token_index == 215 or token_index == 216 or token_index == 217 or token_index == 220 or token_index == 221 or token_index == 225 or token_index == 226 or token_index == 227 or token_index == 228'

  embedded_pattern_i2_xs_post_pad_token:
    doc: Fixed 205-byte post-pad drawing/options table in the XS5..XS9 connector family.
    params:
      - id: token_index
        type: s4
    seq:
      - id: int3_value
        type: int3
        if: is_int3_token
      - id: int4_value
        type: int4
        if: is_int4_token
      - id: flag_byte
        type: u1
        if: not is_int3_token and not is_int4_token
    instances:
      is_int3_token:
        value: 'token_index == 0 or token_index == 1 or token_index == 9 or token_index == 15 or token_index == 18 or token_index == 19 or token_index == 21 or token_index == 22 or token_index == 30 or token_index == 36 or token_index == 39 or token_index == 40 or token_index == 42 or token_index == 83 or token_index == 96'
      is_int4_token:
        value: 'token_index == 2 or token_index == 3 or token_index == 4 or token_index == 5 or token_index == 6 or token_index == 7 or token_index == 16 or token_index == 17 or token_index == 20 or token_index == 23 or token_index == 24 or token_index == 25 or token_index == 26 or token_index == 27 or token_index == 28 or token_index == 37 or token_index == 38 or token_index == 41 or token_index == 85 or token_index == 86 or token_index == 87 or token_index == 88 or token_index == 91 or token_index == 92 or token_index == 93 or token_index == 94'

  embedded_pattern_empty_font_marking_residue:
    doc: |
      Compact stored component marking whose font-name dt_string is empty.  These
      appear at the start of exact-length modern nonzero branches before the common
      embedded-pattern payload.
    params:
      - id: version
        type: s4
    seq:
      - id: header_zero_a
        type: u1
        valid:
          expr: _ == 0
      - id: header_zero_b
        type: u1
        valid:
          expr: _ == 0
      - id: header_zero_c
        type: u1
        valid:
          expr: _ == 0
      - id: field_type
        type: int3
      - id: font_name
        type: dt_string(version)
      - id: text
        type: dt_string(version)
      - id: font_size
        type: int4
      - id: field_a
        type: int3
      - id: coord_x
        type: int4
      - id: coord_y
        type: int4
      - id: flag_a
        type: u1
      - id: flag_b
        type: u1
      - id: extent_x
        type: int4
      - id: extent_y
        type: int4
      - id: tail_byte
        type: u1
      - id: tail_a
        type: int3
      - id: tail_b
        type: int3
      - id: tail_c
        type: int3

  embedded_pattern_no_prefix_text_field_before_common:
    doc: |
      Component text/marking record whose three leading zero flag bytes were consumed
      before the embedded-pattern parser starts.  Banana_Pi.dch C2 starts at 0x113A
      with field_type=1, font "Tahoma", text "104K"; after the full marking tail the
      stream re-enters the common modern embedded-pattern record at 0x1183.  Other
      corpus samples use the same layout with field_type=3 and fonts including
      Arial Narrow and Times New Roman.
    params:
      - id: version
        type: s4
    seq:
      - id: text_field
        type: component_text_field_no_prefix(version)
      - id: common
        type: embedded_pattern_modern_variant(version)

  embedded_pattern_font_bearing_shapes_before_marking:
    doc: |
      Font-bearing component-shape residue before normal marking records and the
      embedded pattern.  The reference schematic U1 at 0x1B8218 stores four
      `font_bearing_shape` line records, then a full reference text field, then
      the existing Tahoma-suffix value-marking residue before the common pattern.
      Q25 at 0x11243E stores 22 font-bearing shape records before the same
      text/marking suffix.  The shape run is self-delimiting: adjacent
      font-bearing shapes use next_record_code=1, while the final shape stores
      next_record_code=700 before the following reference text.  Observed
      reference-schematic rows cover 4, 6, 12, 20, 22, and 34 shape records.
    params:
      - id: version
        type: s4
    seq:
      - id: shapes
        type: font_bearing_shape(version)
        repeat: until
        repeat-until: _.next_record_code.value == 700
      - id: reference_text
        type: component_text_field(version)
      - id: value_marking
        type: embedded_pattern_marking_residue_before_common(version)

  component_text_field_no_prefix:
    doc: Stored component text field without the leading 00 00 00 flag prefix.
    params:
      - id: version
        type: s4
    seq:
      - id: field_type
        type: int3
      - id: font_name
        type: dt_string(version)
      - id: text
        type: dt_string(version)
      - id: font_size
        type: int4
      - id: field_a
        type: int3
      - id: coord_x
        type: int4
      - id: coord_y
        type: int4
      - id: field_b
        type: int4
      - id: field_c
        type: int4
      - id: flag_a
        type: u1
      - id: flag_b
        type: u1
      - id: field_d
        type: int4
      - id: field_e
        type: int4
      - id: field_f
        type: int3
      - id: field_g
        type: int3
      - id: tail_flags
        type: u1
        repeat: expr
        repeat-expr: 4
      - id: field_h
        type: int3

  embedded_pattern_gost_font_suffix_marking_before_common:
    doc: |
      Component marking residue before a common modern embedded pattern where the
      parser starts one byte into the UTF-16BE font name `ГОСТ тип А`.  In
      from-telegram/20933_Не_выделяется_земля_нетпорт/Не_выделяется_земля_нетпорт.dch
      R43 at 0x2FAA, the field_type, font-name length, and high byte 04 of the
      first character were consumed before this branch.  The remaining 17 font-name
      bytes complete `ОСТ тип А`, then the visible value text (`0.5`, `1,5к`,
      `R1206`, etc.) and marking geometry are decoded.  A 10-byte short marking
      tail follows before the stream rejoins the normal common modern embedded
      pattern at lead_in=0.
    params:
      - id: version
        type: s4
    seq:
      - id: font_name_suffix
        contents: [0x1e, 0x04, 0x21, 0x04, 0x22, 0x00, 0x20, 0x04, 0x42, 0x04, 0x38, 0x04, 0x3f, 0x00, 0x20, 0x04, 0x10]
        doc: Low byte of `О`, then UTF-16BE `СТ тип А`.
      - id: value_text
        type: dt_string(version)
      - id: font_size
        type: int4
      - id: field_a
        type: int3
      - id: coord_x
        type: int4
      - id: coord_y
        type: int4
      - id: flag_a
        type: u1
      - id: flag_b
        type: u1
      - id: extent_x
        type: int4
      - id: extent_y
        type: int4
      - id: short_tail
        type: embedded_pattern_short_marking_tail_before_common
      - id: common
        type: embedded_pattern_modern_variant(version)

  embedded_pattern_short_marking_tail_before_common:
    doc: |
      Ten-byte marking tail used by the `ГОСТ тип А` suffix family before the
      following common embedded pattern.  Observed as int3(0), int3(0 or 5),
      then four zero flag bytes.
    seq:
      - id: field_b
        type: int3
      - id: field_c
        type: int3
      - id: tail_flag_a
        type: u1
      - id: tail_flag_b
        type: u1
      - id: tail_flag_c
        type: u1
      - id: tail_flag_d
        type: u1

  embedded_pattern_font_size_suffix_compact_no_name_residue:
    doc: |
      Short 196-byte modern residue branch that starts three bytes into a text
      field font-size int4 and then stores a compact no-name pattern footer.
      The reference schematic +24V-MOT starts at 0x3D1 with bytes 9A E7 4C; the
      preceding byte is 3B, so the complete font_size is int4(7500).  The
      remaining marking trailer is followed by a 151-byte compact pattern tail
      with no UTF-16 pattern name.
    seq:
      - id: font_size_low_bytes
        contents: [0x9a, 0xe7, 0x4c]
        doc: Low three bytes of full stored int4 3B9AE74C = decoded 7500.
      - id: marking_tail
        type: embedded_pattern_marking_after_font_size_tail
      - id: compact_tail
        type: embedded_pattern_modern_compact_no_name_tail

  embedded_pattern_font_size_suffix_polygon_compact_no_name_residue:
    doc: |
      228-byte modern residue branch that starts three bytes into a font-size
      int4, continues with a counted polygon/body residue, and then stores the
      compact no-name pattern tail.  The reference schematic ADC1-AIN1 starts at
      0xE0F1 with bytes 9A E7 4C; the preceding byte is 3B, so the complete
      font_size is int4(7500).  The following point_count is 6 and the six
      point pairs form the residue body before a 23-byte zero trailer and the
      151-byte compact no-name pattern tail.
    seq:
      - id: font_size_low_bytes
        contents: [0x9a, 0xe7, 0x4c]
        doc: Low three bytes of full stored int4 3B9AE74C = decoded 7500.
      - id: point_count
        type: int3
      - id: points
        type: shape_point
        repeat: expr
        repeat-expr: point_count.value
      - id: polygon_tail
        type: embedded_pattern_polygon_after_points_tail
      - id: compact_tail
        type: embedded_pattern_modern_compact_no_name_tail

  embedded_pattern_font_size_suffix_graphics_stream:
    doc: |
      Modern residue branch that starts three bytes into a font-size int4
      (`3B 9A E7 4C`, decoded 7500) and continues as a bounded drawing/model
      stream.  Observed as a reference-schematic HDRF connector with model
      `hdrf-2x9th_2.54x2.54_23x13.step` and as a compact 163-byte short-tail
      stream with no model string.
    params:
      - id: version
        type: s4
    seq:
      - id: graphics_tokens
        type: embedded_pattern_font_size_suffix_graphics_token
        repeat: expr
        repeat-expr: num_graphics_tokens
      - id: model_name
        type: dt_string(version)
      - id: long_tail
        type: embedded_pattern_model_tail_long
        if: _io.size - _io.pos == 61
      - id: short_tail
        type: embedded_pattern_model_tail_short
        if: _io.size - _io.pos == 28
    instances:
      num_graphics_tokens:
        value: '_io.size == 2351 ? 1015 : 48'

  embedded_pattern_font_size_suffix_graphics_token:
    doc: |
      Token from 9A E7 4C font-size-suffix drawing streams.  Values beginning
      with 0F are biased int3 fields, values beginning with 3B are biased int4
      fields, and all other bytes are structural flags/separators.
    seq:
      - id: int3_value
        type: int3
        if: first_byte == 0x0f
      - id: int4_value
        type: int4
        if: first_byte == 0x3b
      - id: flag_byte
        type: u1
        if: first_byte != 0x0f and first_byte != 0x3b
    instances:
      first_byte:
        pos: _io.pos
        type: u1

  embedded_pattern_shape_line_width_suffix_before_common:
    doc: |
      Modern residue branch that starts three bytes into a component shape
      line_width int4.  The reference schematic D35 at 0x91828 starts with low bytes
      9A E7 C4; the preceding byte is 3B, so the complete line_width is
      int4(7620).  That partial first shape is followed by seven full standard
      shape records, then the common embedded-pattern layout begins with
      lead_in=1 and pattern name `LED-3mm Round Green`.
    params:
      - id: version
        type: s4
    seq:
      - id: first_shape
        type: shape_after_line_width_suffix(version)
      - id: shapes
        type: shape(version)
        repeat: expr
        repeat-expr: 7
      - id: common
        type: embedded_pattern_modern_variant(version)

  embedded_pattern_shape_line_width_suffix_text_before_named_field_c:
    doc: |
      Modern residue branch that starts three bytes into a component shape
      line_width int4, then stores more shapes and a value text field before a
      named-pattern field_c suffix.  The reference schematic D32 at 0x8F856 starts
      with low bytes 9A E7 4C; the preceding byte is 3B, so the complete
      line_width is int4(7500).  The first partial shape is followed by eight
      full shape records, value text `SM6T6V8A`, and a named-pattern suffix
      beginning at 0x8FA94.
    params:
      - id: version
        type: s4
    seq:
      - id: first_shape
        type: shape_after_line_width_suffix(version)
      - id: shapes
        type: shape(version)
        repeat: expr
        repeat-expr: 8
      - id: value_text
        type: component_text_field(version)
      - id: named_pattern
        type: embedded_pattern_named_field_c_suffix_residue(version)

  embedded_pattern_zero_pin_shape_kind_suffix_before_model:
    doc: |
      Modern embedded-pattern drawing stream entered one byte late after a zero-pin
      component.  from-telegram/36164_i2/i2.dch FB1 at 0x12878 starts with low bytes
      42 46 of the first drawing kind int3; the high byte 0F was consumed as the
      zero-pin pin_header_byte, so the complete kind is int3(6).  The stream then
      stores five compact styled drawing bodies, a fixed compact options/footer
      block, and the normal counted 3D model tail.  Exact observed length: 770.
    params:
      - id: version
        type: s4
    seq:
      - id: drawing_kind_low_bytes
        contents: [0x42, 0x46]
        doc: Low two bytes of int3(6); high byte 0F was consumed before this branch.
      - id: drawings
        type: embedded_pattern_compact_styled_drawing_body
        repeat: expr
        repeat-expr: 5
      - id: footer
        type: embedded_pattern_compact_styled_drawing_footer_42_46_01
      - id: model_tail
        type: embedded_pattern_model_tail(version)

  embedded_pattern_compact_styled_drawing_body:
    doc: |
      Compact no-font drawing body in zero-pin embedded-pattern drawing residue.
      The first fields carry style/layer/color metadata, followed by line width,
      point count, point list, an empty label string, and zero font extents.
    seq:
      - id: style_flag
        type: u1
      - id: style_a
        type: int3
      - id: style_b
        type: int3
      - id: style_c
        type: int3
      - id: color
        type: bgr_color
      - id: style_d
        type: int3
      - id: modern_pad
        contents: [0x00, 0x00, 0x00, 0x00]
      - id: line_width
        type: int4
      - id: num_points
        type: int3
        valid:
          expr: _.value >= 1 and _.value <= 100
      - id: points
        type: shape_point
        repeat: expr
        repeat-expr: num_points.value
      - id: label
        type: dt_string(34)
        doc: Observed empty in the 42 46 01 residue family.
      - id: font_x
        type: int4
      - id: font_y
        type: int4

  embedded_pattern_compact_styled_drawing_footer_42_46_01:
    doc: |
      Fixed 374-byte compact drawing/options footer between the five styled drawing
      bodies and the 3D model tail in the 42 46 01 zero-pin residue family.  The
      leading terminal drawing keeps the same compact style header but stores a
      point plus extents instead of a label/font tuple; the following option bytes
      are stable across the five i2.dch FB instances and end immediately before the
      counted model-tail placement_count.
    seq:
      - id: terminal_style_flag
        type: u1
      - id: terminal_style_a
        type: int3
      - id: terminal_style_b
        type: int3
      - id: terminal_style_c
        type: int3
      - id: terminal_color
        type: bgr_color
      - id: terminal_style_d
        type: int3
      - id: terminal_pad
        contents: [0x00, 0x00, 0x00, 0x00]
      - id: terminal_line_width
        type: int4
      - id: terminal_num_points
        type: int3
        valid:
          expr: _.value >= 1 and _.value <= 100
      - id: terminal_points
        type: shape_point
        repeat: expr
        repeat-expr: terminal_num_points.value
      - id: terminal_flag_a
        type: u1
      - id: terminal_flag_b
        type: u1
      - id: terminal_extent_x
        type: int4
      - id: terminal_extent_y
        type: int4
      - id: options
        type: embedded_pattern_compact_styled_drawing_options_42_46_01

  embedded_pattern_compact_styled_drawing_options_42_46_01:
    doc: |
      Fixed compact option/style table following the terminal drawing in the
      42 46 01 zero-pin residue family.  The table is a 329-byte fixed token
      stream: biased int3 fields, biased int4 fields, and one-byte flags/pads.
      It is byte-identical across the five observed FB instances in
      from-telegram/36164_i2/i2.dch and ends immediately before the counted
      model_tail.  The C++ parser does not import these options, but the byte
      grammar is deterministic and fully tokenized here.
    seq:
      - id: tokens
        type: embedded_pattern_compact_styled_drawing_option_token(_index)
        repeat: expr
        repeat-expr: 126

  embedded_pattern_compact_styled_drawing_option_token:
    doc: |
      One fixed-width token in the 329-byte 42 46 01 compact options table.
      Token widths are fixed by index from five identical FB instances:
      int3 tokens use the DipTrace 3-byte bias, int4 tokens use the 4-byte bias,
      and all remaining positions are single structural flag/pad bytes.
    params:
      - id: token_index
        type: s4
    seq:
      - id: int3_value
        type: int3
        if: is_int3_token
      - id: int4_value
        type: int4
        if: is_int4_token
      - id: flag_byte
        type: u1
        if: not is_int3_token and not is_int4_token
    instances:
      is_int3_token:
        value: 'token_index == 0 or token_index == 1 or token_index == 6 or token_index == 7 or token_index == 13 or token_index == 18 or token_index == 22 or token_index == 30 or token_index == 31 or token_index == 32 or token_index == 34 or token_index == 45 or token_index == 47 or token_index == 49 or token_index == 59 or token_index == 70 or token_index == 72 or token_index == 74 or token_index == 83 or token_index == 84 or token_index == 92 or token_index == 98 or token_index == 101 or token_index == 102 or token_index == 104 or token_index == 105 or token_index == 113 or token_index == 119 or token_index == 122 or token_index == 123 or token_index == 125'
      is_int4_token:
        value: 'token_index == 8 or token_index == 9 or token_index == 11 or token_index == 14 or token_index == 15 or token_index == 16 or token_index == 17 or token_index == 20 or token_index == 21 or token_index == 25 or token_index == 26 or token_index == 27 or token_index == 28 or token_index == 35 or token_index == 36 or token_index == 41 or token_index == 42 or token_index == 43 or token_index == 44 or token_index == 56 or token_index == 57 or token_index == 60 or token_index == 61 or token_index == 66 or token_index == 67 or token_index == 68 or token_index == 69 or token_index == 81 or token_index == 82 or token_index == 85 or token_index == 86 or token_index == 87 or token_index == 88 or token_index == 89 or token_index == 90 or token_index == 99 or token_index == 100 or token_index == 103 or token_index == 106 or token_index == 107 or token_index == 108 or token_index == 109 or token_index == 110 or token_index == 111 or token_index == 120 or token_index == 121 or token_index == 124'

  embedded_pattern_compact_pre_model_field_suffix:
    doc: |
      Compact post-common pattern suffix before a standard counted model tail.
      CNC_controller.dch TP/OUT1A at 0x23945 enters this branch after the common
      TP pattern parser has consumed the preceding high byte of a zero int4 field.
      The first three bytes 9A CA 00 complete that int4 (`3B 9A CA 00` = 0),
      followed by a 93-byte compact drawing/options suffix and then the normal
      model tail.  Exact observed length: 201 bytes across TP OUT1A/2A/2B/RES
      variants; only model-tail attributes vary.
    params:
      - id: version
        type: s4
    seq:
      - id: leading_field_low_bytes
        contents: [0x9a, 0xca, 0x00]
        doc: Low three bytes of a full stored int4 3B9ACA00 = decoded 0.
      - id: option_header
        type: embedded_pattern_compact_pre_model_option_header
      - id: terminal_body
        type: embedded_pattern_compact_pre_model_terminal_body
      - id: model_tail
        type: embedded_pattern_model_tail(version)

  embedded_pattern_compact_pre_model_option_header:
    doc: First 44 bytes after the consumed-high-byte int4 in the compact TP suffix.
    seq:
      - id: zero_coord_a
        type: int4
      - id: field_a
        type: int3
      - id: field_b
        type: int3
      - id: negative_extent
        type: int4
        doc: Observed -10000.
      - id: field_c
        type: int3
      - id: field_d
        type: int3
      - id: zero_coords
        type: int4
        repeat: expr
        repeat-expr: 6

  embedded_pattern_compact_pre_model_terminal_body:
    doc: |
      49-byte compact terminal body immediately before the counted model tail.
      The final four int4 coordinates describe a two-point vertical segment in
      the TP examples (-8126,-15625) to (-8126,15625).
    seq:
      - id: flag_a
        type: u1
      - id: field_a
        type: int3
      - id: zero_flags_a
        type: u1
        repeat: expr
        repeat-expr: 4
      - id: flag_b
        type: u1
      - id: field_b
        type: int3
      - id: coord_a
        type: int4
      - id: coord_b
        type: int4
      - id: field_c
        type: int3
      - id: field_d
        type: int3
      - id: negative_extent
        type: int4
        doc: Observed -10000.
      - id: body_shape_type
        type: int3
        doc: Observed 2.
      - id: x1
        type: int4
      - id: y1
        type: int4
      - id: x2
        type: int4
      - id: y2
        type: int4

  shape_after_line_width_suffix:
    doc: |
      Standard component shape after the high byte of line_width was already consumed.  The leading
      drawing kind discriminator is the same as shape: 1 line/polyline, 3 arrow line, 4 rectangle,
      6 obround/circle/ellipse, 8 filled polygon, 9 outline polygon/polyline.
    params:
      - id: version
        type: s4
    seq:
      - id: line_width_byte_2
        type: u1
        doc: Second byte of the full stored line_width int4; the high byte was consumed before this branch.
      - id: line_width_byte_1
        type: u1
        doc: Third byte of the full stored line_width int4; the high byte was consumed before this branch.
      - id: line_width_low_byte
        type: u1
        doc: Low byte of the full stored line_width int4; the high byte was consumed before this branch.
      - id: num_points
        type: int3
        valid:
          expr: _.value >= 1 and _.value <= 100
      - id: points
        type: shape_point
        repeat: expr
        repeat-expr: num_points.value
      - id: label
        type: dt_string(version)
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

  embedded_pattern_polygon_after_points_tail:
    doc: 23-byte polygon/body residue trailer before compact no-name pattern data.
    seq:
      - id: flag_a
        type: u1
      - id: flag_b
        type: u1
      - id: field_a
        type: int4
      - id: field_b
        type: int4
      - id: field_c
        type: int3
      - id: field_d
        type: int3
      - id: tail_flags
        type: u1
        repeat: expr
        repeat-expr: 4
      - id: field_e
        type: int3

  embedded_pattern_compact_no_name_field_c_suffix_residue:
    doc: |
      Short 143-byte modern residue branch that starts three bytes into the
      compact no-name preamble's field_c int4.  The reference schematic NetPort28
      starts at 0x2709E; the preceding byte is 3B, so the complete field_c is
      int4(0).  The field_a, field_b, and high byte of field_c were consumed
      before the importer entered the embedded-pattern scanner.
    seq:
      - id: field_c_low_bytes
        contents: [0x9a, 0xca, 0x00]
        doc: Low three bytes of full stored int4 3B9ACA00 = decoded 0.
      - id: tail
        type: embedded_pattern_modern_compact_no_name_after_field_c

  embedded_pattern_compact_zero_graphics_stream:
    doc: |
      Compact drawing/model-tail stream that starts three bytes into a stored
      int4 zero (`3B 9A CA 00`).  Observed in from-telegram 15284_1 and
      23196_NetPort rows as short no-name streams with no model text and either
      long or short transform tails.
    params:
      - id: version
        type: s4
    seq:
      - id: graphics_tokens
        type: embedded_pattern_compact_zero_graphics_token
        repeat: expr
        repeat-expr: num_graphics_tokens
      - id: model_name
        type: dt_string(version)
        doc: Observed empty in both compact zero streams.
      - id: long_tail
        type: embedded_pattern_model_tail_long
        if: _io.size - _io.pos == 61
      - id: short_tail
        type: embedded_pattern_model_tail_short
        if: _io.size - _io.pos == 28
    instances:
      num_graphics_tokens:
        value: '_io.size == 413 ? 133 : 29'

  embedded_pattern_compact_zero_graphics_token:
    doc: |
      Token from compact 9A CA 00 drawing streams.  Values beginning with 0F
      are biased int3 fields, values beginning with 3B are biased int4 fields,
      and all other bytes are structural flags/separators.
    seq:
      - id: int3_value
        type: int3
        if: first_byte == 0x0f
      - id: int4_value
        type: int4
        if: first_byte == 0x3b
      - id: flag_byte
        type: u1
        if: first_byte != 0x0f and first_byte != 0x3b
    instances:
      first_byte:
        pos: _io.pos
        type: u1

  embedded_pattern_named_field_c_suffix_residue:
    doc: |
      Long modern residue branch that starts three bytes into the named-pattern
      pre-name field_c int4.  The reference schematic VO2 at 0x1E3822 stores the low
      bytes of field_c (complete int4 3B9ACA00 = 0), the remaining pre-name
      fields through extra_drill, then rejoins at the optional pre_name_tail and
      pattern_name (`SOIC8P127_490X600X175L83X44N`).  The reference schematic C34 at
      0x274D6 stores low bytes 9D FE 50 (complete int4 3B9DFE50 = 210000)
      before pattern `CAPAE-10.3x10.3h17.5`.  Other reference-schematic suffixes use
      the same low-byte field_c entry before names such as `HDR-2x2`, `OSCSC2`,
      `XTAL1140X480X430D1320L440`, `LED_1206(3216 Metric)_N_Y`, and `SOIC16...`.
      Exact observed lengths: 967, 1173, 1198, 1237, 1241, 1307, 1319, 1396,
      1645, 1647, 1653, 1755, 2137, and 6191 bytes.
    params:
      - id: version
        type: s4
    seq:
      - id: field_c_byte_2
        type: u1
        doc: Second byte of the full stored field_c int4; the high byte was consumed before this branch.
      - id: field_c_byte_1
        type: u1
        doc: Third byte of the full stored field_c int4; the high byte was consumed before this branch.
      - id: field_c_low_byte
        type: u1
        doc: Low byte of the full stored field_c int4; the high byte was consumed before this branch.
      - id: pre_name_suffix
        type: embedded_pattern_modern_pre_name_after_field_c
      - id: named_pattern
        type: embedded_pattern_modern_after_pre_name(version)

  embedded_pattern_modern_pre_name_after_field_c:
    doc: Remaining modern pre-name fields after field_c, through extra_drill.
    seq:
      - id: flag_a
        type: u1
      - id: field_d
        type: int4
      - id: flag_b
        type: u1
      - id: field_e
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
      - id: extra_drill
        type: int4

  embedded_pattern_modern_compact_no_name_after_field_c:
    doc: Remaining compact no-name preamble fields after field_c, plus footer.
    seq:
      - id: flag_a
        type: u1
      - id: field_d
        type: int4
      - id: flag_b
        type: u1
      - id: field_e
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
      - id: extra_drill
        type: int4
      - id: footer
        type: embedded_pattern_modern_compact_no_name_footer

  embedded_pattern_marking_after_font_size_tail:
    doc: |
      Component marking fields after the font-size int4.  Used when the parser
      starts inside font_size, after font name and text were already consumed.
    seq:
      - id: field_a
        type: int3
      - id: coord_x
        type: int4
      - id: coord_y
        type: int4
      - id: field_b
        type: int4
      - id: field_c
        type: int4
      - id: flag_a
        type: u1
      - id: flag_b
        type: u1
      - id: extent_x
        type: int4
      - id: extent_y
        type: int4
      - id: field_d
        type: int3
      - id: field_e
        type: int3
      - id: tail_flags
        type: u1
        repeat: expr
        repeat-expr: 4
      - id: tail_field
        type: int3

  embedded_pattern_modern_compact_no_name_tail:
    doc: |
      151-byte compact modern footer with no pattern-name string.  The first
      48-byte preamble resembles the modern pre-name block but omits one int3
      field; the 103-byte footer carries zero geometry/options, three 10000
      extents, four flag bytes, and seven final int3 fields.
    seq:
      - id: preamble
        type: embedded_pattern_modern_compact_no_name_preamble
      - id: footer
        type: embedded_pattern_modern_compact_no_name_footer

  embedded_pattern_modern_compact_no_name_preamble:
    doc: Compact no-name pattern preamble through extra_drill.
    seq:
      - id: field_a
        type: int3
      - id: field_b
        type: int4
      - id: field_c
        type: int4
      - id: flag_a
        type: u1
      - id: field_d
        type: int4
      - id: flag_b
        type: u1
      - id: field_e
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
      - id: extra_drill
        type: int4

  embedded_pattern_modern_compact_no_name_footer:
    doc: Compact no-name footer after the preamble.
    seq:
      - id: field_a
        type: int3
      - id: flag_a
        type: u1
      - id: flag_b
        type: u1
      - id: zero_coords_a
        type: int4
        repeat: expr
        repeat-expr: 4
      - id: flag_c
        type: u1
      - id: zero_fields_a
        type: int3
        repeat: expr
        repeat-expr: 6
      - id: flag_d
        type: u1
      - id: flag_e
        type: u1
      - id: zero_coords_b
        type: int4
        repeat: expr
        repeat-expr: 6
      - id: extents
        type: int4
        repeat: expr
        repeat-expr: 3
        doc: Observed as three int4(10000) fields in reference-schematic net-port residues.
      - id: tail_flags
        type: u1
        repeat: expr
        repeat-expr: 4
      - id: final_fields
        type: int3
        repeat: expr
        repeat-expr: 7

  embedded_pattern_color_marking_residue_before_common:
    doc: |
      Full color-prefixed component marking residue before an embedded pattern.
      QFN32-DIP.dch U1 starts this branch at 0x1D3E: BGR color C0C0C0,
      field_type=0, font "Tahoma", text "BQ25890", marking geometry/style fields,
      tail_field=24, then the common embedded-pattern record starts at 0x1D90
      with pattern_name "QFN-24/4x4x0.5".
    params:
      - id: version
        type: s4
    seq:
      - id: text_color_bgr
        type: bgr_color
      - id: field_type
        type: int3
      - id: font_name
        type: dt_string(version)
      - id: text
        type: dt_string(version)
      - id: font_size
        type: int4
      - id: field_a
        type: int3
      - id: coord_x
        type: int4
      - id: coord_y
        type: int4
      - id: field_b
        type: int4
      - id: field_c
        type: int4
      - id: flag_a
        type: u1
      - id: flag_b
        type: u1
      - id: extent_x
        type: int4
      - id: extent_y
        type: int4
      - id: field_d
        type: int3
      - id: field_e
        type: int3
      - id: tail_flags
        type: u1
        repeat: expr
        repeat-expr: 4
      - id: tail_field
        type: int3
      - id: common
        type: embedded_pattern_modern_variant(version)

  embedded_pattern_marking_residue_before_common:
    doc: |
      Component marking residue before an embedded pattern.  In
      light-switch-motherboard.dch C2 the parser starts at 0x354, four bytes into
      the UTF-16BE "Tahoma" marking font name.  The residue contains the rest of the
      visible value text record ("0.1uF") and then the common embedded-pattern record
      starts at 0x3A0.
    params:
      - id: version
        type: s4
    seq:
      - id: font_name_suffix
        contents: [0x61, 0x00, 0x68, 0x00, 0x6f, 0x00, 0x6d, 0x00, 0x61]
        doc: Remaining bytes of UTF-16BE "Tahoma" after the parser starts at the second character.
      - id: value_text
        type: dt_string(version)
        doc: Visible value marking text; C2 stores "0.1uF".
      - id: font_size
        type: int4
      - id: field_a
        type: int3
      - id: coord_x
        type: int4
      - id: coord_y
        type: int4
      - id: flag_a
        type: u1
      - id: flag_b
        type: u1
      - id: extent_x
        type: int4
      - id: extent_y
        type: int4
      - id: field_b
        type: int3
      - id: field_c
        type: int3
      - id: tail_flags
        type: u1
        repeat: expr
        repeat-expr: 4
      - id: field_d
        type: int3
      - id: field_e
        type: int3
      - id: field_f
        type: int4
      - id: field_g
        type: int4
      - id: zero_flag
        type: u1
      - id: field_h
        type: int4
      - id: common_lead_flag
        type: u1
        doc: Observed 1 immediately before the real common embedded-pattern start.
      - id: common
        type: embedded_pattern_modern_variant(version)

  embedded_pattern_modern_common:
    doc: |
      Common modern (34+) embedded pattern layout.  The pre-name header has two stored
      variants: some records store pattern_name immediately after extra_drill, while
      others insert pre_name_tail first.  The C++ chooses by validating whether a
      UTF-16-BE string starts at that offset.
    params:
      - id: version
        type: s4
    seq:
      - id: pre_name
        type: embedded_pattern_modern_pre_name
      - id: pre_name_tail
        type: int3
        if: not pattern_name_starts_here
        doc: |
          Optional int3(0) tail before pattern_name.  Schematic_6.dch CAP_2012_N
          stores this tail after extra_drill; other modern records start the UTF-16
          pattern_name immediately.
      - id: pattern_name
        type: dt_string(version)
      - id: post_name
        type: embedded_pattern_modern_post_name(version)
    instances:
      pattern_name_len_probe:
        pos: _io.pos
        type: u2
      pattern_name_starts_here:
        value: pattern_name_len_probe <= 512

  embedded_pattern_modern_after_pre_name:
    doc: |
      Common modern embedded-pattern suffix starting immediately after extra_drill.
      Used by residue branches that start partway through the pre-name header but
      then rejoin at the optional pre_name_tail / pattern_name boundary.
    params:
      - id: version
        type: s4
    seq:
      - id: pre_name_tail
        type: int3
        if: not pattern_name_starts_here
      - id: pattern_name
        type: dt_string(version)
      - id: post_name
        type: embedded_pattern_modern_post_name(version)
    instances:
      pattern_name_len_probe:
        pos: _io.pos
        type: u2
      pattern_name_starts_here:
        value: pattern_name_len_probe <= 512

  embedded_pattern_modern_pre_name:
    doc: Modern pre-name header through extra_drill.  Optional pre_name_tail may follow.
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
      - id: extra_drill
        type: int4

  embedded_pattern_modern_post_name:
    params:
      - id: version
        type: s4
    seq:
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
      - id: property_count
        type: int3
        doc: |
          Counted name/value/property-type entries; v41 examples include
          "Case Info", "Polaring", "3d Model", and library/history fields.
      - id: properties
        type: embedded_pattern_property(version)
        repeat: expr
        repeat-expr: property_count.value
      - id: post_properties_a
        type: int3
        doc: Observed 0 before the counted pattern element stream.
      - id: pattern_element_count
        type: int3
        if: property_count.value > 0
        doc: |
          Count of following pattern element records in observed v41/v46 examples.
          Schematic_6.dch CAP_2012_N stores int3(4) here, followed by pattern
          geometry/pad/model element data.  The C++ importer names this region
          pad/drawing/model best-effort and later scans for the 3D model tail.
      - id: empty_footer
        type: int3
        repeat: expr
        repeat-expr: 'property_count.value == 0 ? 3 : 0'
        doc: Empty modern-pattern footer used when no property/element stream follows.
      - id: element_stream
        type: embedded_pattern_modern_element_stream(version, pattern_element_count.value)
        if: property_count.value > 0
        doc: |
          Common modern pattern element stream.  Schematic_6.dch CAP_2012_N stores
          four pad-like records here (default pad, pad 1, pad 2, default terminator)
          followed by shape_count=7: five XML drawing shapes plus two framing records.

  embedded_pattern_property:
    doc: |
      Modern embedded-pattern property entry.  Schematic_6.dch stores each entry as
      UTF-16 name, UTF-16 value (often empty), then an int3 type/flag field.  Example:
      "Case Info", empty value, int3(0), followed by "Polaring".
    params:
      - id: version
        type: s4
    seq:
      - id: name
        type: dt_string(version)
      - id: value
        type: dt_string(version)
      - id: property_type
        type: int3

  embedded_pattern_modern_pad:
    doc: Modern pad/template record consumed by parseEmbeddedPattern.
    params:
      - id: version
        type: s4
    seq:
      - id: record_type
        type: u1
      - id: pad_id
        type: int3
      - id: x
        type: int4
      - id: y
        type: int4
      - id: number
        type: dt_string(version)
      - id: note
        type: dt_string(version)
      - id: width
        type: int4
      - id: height
        type: int4
      - id: drill
        type: int4
      - id: field_a
        type: int4
      - id: field_b
        type: int3

  embedded_pattern_modern_element_stream:
    doc: |
      Counted modern embedded-pattern pad records followed by the drawing/model stream.
      In Schematic_6.dch CAP_2012_N, pattern_element_count is 4 and each pad-like
      record carries the 22-byte tail decoded by embedded_pattern_modern_pad_tail.
      The following int3 is the drawing/framing count: 7 = five XML shapes plus
      leading/trailing framing records before the 3D model path.
    params:
      - id: version
        type: s4
      - id: num_pad_records
        type: s4
    seq:
      - id: pad_records
        type: embedded_pattern_modern_pad_record(version)
        repeat: expr
        repeat-expr: num_pad_records
      - id: shape_count
        type: int3
        doc: |
          Count of following modern drawing/framing records.  CAP_2012_N stores 7:
          two framing records bracketing five XML footprint shapes.
      - id: shapes_and_model
        type: embedded_pattern_modern_shape_model_stream(version, shape_count.value)
        doc: |
          Modern drawing/framing records and the 3D model string/tail.  The drawing
          records are Tahoma-font shape records matching the PCB fp_font_block metadata
          layout with additional chained geometry prefixes.

  embedded_pattern_modern_pad_record:
    doc: Modern pad/template record plus its fixed 22-byte display/style tail.
    params:
      - id: version
        type: s4
    seq:
      - id: pad
        type: embedded_pattern_modern_pad(version)
      - id: tail
        type: embedded_pattern_modern_pad_tail

  embedded_pattern_modern_pad_tail:
    doc: |
      Fixed 22-byte modern embedded-pattern pad tail.  Real CAP_2012_N pads store
      flag_a=1, style_index=2, flag_b=1, field_b=0, six zero bytes, and extents
      -10000000/-10000000; default/terminator pads use flag_a=0 and style_index=0.
    seq:
      - id: flag_a
        type: u1
      - id: style_index
        type: int3
      - id: flag_b
        type: u1
      - id: field_b
        type: int3
      - id: zero_a
        type: u1
      - id: zero_b
        type: u1
      - id: zero_c
        type: u1
      - id: zero_d
        type: u1
      - id: zero_e
        type: u1
      - id: zero_f
        type: u1
      - id: extent_x
        type: int4
      - id: extent_y
        type: int4

  embedded_pattern_modern_shape_model_stream:
    doc: |
      Counted modern embedded-pattern drawing/framing records followed by the direct
      3D model path and transform tail.  Schematic_6.dch CAP_2012_N stores seven
      records: a leading framing record, five XML footprint drawing records, and a
      trailing framing/model lead-in record.  The following model string starts at
      the first byte after the trailing frame body and is followed by the same
      61-byte/28-byte transform tails used by embedded_pattern_model_tail.
    params:
      - id: version
        type: s4
      - id: num_records
        type: s4
    seq:
      - id: records
        type: embedded_pattern_modern_shape_record(version, _index, num_records)
        repeat: expr
        repeat-expr: num_records
      - id: model_name
        type: dt_string(version)
      - id: long_tail
        type: embedded_pattern_model_tail_long
        if: _io.size - _io.pos == 61
      - id: short_tail
        type: embedded_pattern_model_tail_short
        if: _io.size - _io.pos == 28

  embedded_pattern_modern_shape_record:
    doc: |
      One modern embedded-pattern Tahoma drawing/framing record.  The prefix form is
      selected by the first byte: 0x0F for the leading frame, 0x3B for a geometry-copy
      prefix, and 0x00 for a short link prefix.  The font metadata matches the PCB
      v46+ fp_font_block header.  Leading and trailing framing records use their own
      bodies; middle records store XML drawing geometry and optional 24-byte follow
      geometry when body_shape_type is 2.
    params:
      - id: version
        type: s4
      - id: record_index
        type: s4
      - id: num_shape_records
        type: s4
    seq:
      - id: prefix
        type:
          switch-on: prefix_first_byte
          cases:
            0: embedded_pattern_modern_shape_prefix_short
            15: embedded_pattern_modern_shape_prefix_leading_frame
            59: embedded_pattern_modern_shape_prefix_geometry_copy
      - id: font
        type: embedded_pattern_modern_shape_font
      - id: body
        type:
          switch-on: 'record_index == 0 ? 0 : (record_index == num_shape_records - 1 ? 2 : 1)'
          cases:
            0: embedded_pattern_modern_shape_leading_body
            1: embedded_pattern_modern_shape_body
            2: embedded_pattern_modern_shape_trailing_body
    instances:
      prefix_first_byte:
        pos: _io.pos
        type: u1
        doc: Prefix discriminator byte without advancing the record stream.

  embedded_pattern_modern_shape_prefix_leading_frame:
    doc: |
      33-byte leading shape-frame prefix before the Tahoma font header.  CAP_2012_N
      stores field_a=0, six zero coordinates, zero_flag_a=0, field_b=0, and two
      zero flags.
    seq:
      - id: field_a
        type: int3
      - id: coord_a
        type: int4
      - id: coord_b
        type: int4
      - id: coord_c
        type: int4
      - id: coord_d
        type: int4
      - id: coord_e
        type: int4
      - id: coord_f
        type: int4
      - id: zero_flag_a
        type: u1
      - id: field_b
        type: int3
      - id: zero_flag_b
        type: u1
      - id: zero_flag_c
        type: u1

  embedded_pattern_modern_shape_prefix_geometry_copy:
    doc: |
      30-byte geometry-copy prefix used by several chained drawing records.  The six
      coordinates mirror the current or next record geometry, followed by a zero
      separator, an int3 field, and two flags.
    seq:
      - id: x1
        type: int4
      - id: y1
        type: int4
      - id: x2
        type: int4
      - id: y2
        type: int4
      - id: x3
        type: int4
      - id: y3
        type: int4
      - id: zero_separator
        type: u1
      - id: field_a
        type: int3
      - id: zero_flag_a
        type: u1
      - id: zero_flag_b
        type: u1

  embedded_pattern_modern_shape_prefix_short:
    doc: |
      6-byte short link prefix before a Tahoma shape header.  CAP_2012_N stores
      values such as 0,18,0,0 before the outline rectangle and 0,1,0,0 / 0,16,0,0
      before later rectangle records.
    seq:
      - id: zero_flag_a
        type: u1
      - id: link_code
        type: int3
      - id: zero_flag_b
        type: u1
      - id: zero_flag_c
        type: u1

  embedded_pattern_modern_shape_font:
    doc: UTF-16BE Tahoma font literal plus 25-byte shape metadata header.
    seq:
      - id: font_pattern
        contents: [0x00, 0x06, 0x00, 0x54, 0x00, 0x61, 0x00, 0x68, 0x00, 0x6f, 0x00, 0x6d, 0x00, 0x61]
        doc: UTF-16BE "Tahoma" (length-prefixed 6-character font name).
      - id: meta_flag
        type: u1
      - id: font_size
        type: int3
      - id: font_height
        type: int4
      - id: font_width
        type: int4
      - id: meta_field_a
        type: int3
      - id: meta_field_b
        type: int3
      - id: line_width
        type: int4
      - id: layer
        type: int3

  embedded_pattern_modern_shape_leading_body:
    doc: |
      Leading frame body.  CAP_2012_N stores body_shape_type=2 and six coordinates
      matching the first XML outline rectangle, with the final two coordinates zero.
    seq:
      - id: body_shape_type
        type: int3
      - id: x1
        type: int4
      - id: y1
        type: int4
      - id: x2
        type: int4
      - id: y2
        type: int4
      - id: x3
        type: int4
      - id: y3
        type: int4

  embedded_pattern_modern_shape_body:
    doc: |
      Middle drawing body.  XML geometry uses x1/y1 and x2/y2, with XML Y negated.
      CAP_2012_N stores body_shape_type 1 for simple line/outline records, 2 for
      records carrying an additional 24-byte follow-geometry tuple, and 0 for a
      rectangle record with no follow tuple.
    seq:
      - id: x1
        type: int4
      - id: y1
        type: int4
      - id: x2
        type: int4
      - id: y2
        type: int4
      - id: body_shape_type
        type: int3
      - id: follow_geometry
        type: embedded_pattern_modern_shape_follow_geometry
        if: body_shape_type.value == 2

  embedded_pattern_modern_shape_follow_geometry:
    doc: 24-byte geometry tuple following middle bodies with body_shape_type=2.
    seq:
      - id: x1
        type: int4
      - id: y1
        type: int4
      - id: x2
        type: int4
      - id: y2
        type: int4
      - id: x3
        type: int4
      - id: y3
        type: int4

  embedded_pattern_modern_shape_trailing_body:
    doc: |
      42-byte trailing frame/model lead-in body.  CAP_2012_N stores body_shape_type=2,
      zero flags/coordinates, and final_field=0 immediately before the model path's
      UTF-16 character count.
    seq:
      - id: body_shape_type
        type: int3
      - id: zero_flag_a
        type: u1
      - id: coord_a
        type: int4
      - id: coord_b
        type: int4
      - id: coord_c
        type: int4
      - id: coord_d
        type: int4
      - id: zero_flag_b
        type: u1
      - id: zero_flag_c
        type: u1
      - id: coord_e
        type: int4
      - id: coord_f
        type: int4
      - id: coord_g
        type: int4
      - id: coord_h
        type: int4
      - id: zero_flag_d
        type: u1
      - id: final_field
        type: int3

  embedded_pattern_model_tail:
    doc: |
      Counted 3D-model placement records followed by a UTF-16-BE model file name and
      either a 61-byte or 28-byte transform/attribute tail.  In exported v5.x examples,
      the long tail stores Offset XYZ, Rotate XYZ, Zoom XYZ, then a 25-byte attribute
      trailer; e.g. CNC_controller cap_0603.step stores offset_z=-300 and zoom=10000,
      matching XML Offset Z="-0.000394" and Zoom 1/1/1.
    params:
      - id: version
        type: s4
    seq:
      - id: placement_count
        type: int3
      - id: placements
        type: embedded_pattern_model_placement
        repeat: expr
        repeat-expr: placement_count.value
      - id: post_placements
        type: int3
      - id: model_name
        type: dt_string(version)
      - id: long_tail
        type: embedded_pattern_model_tail_long
        if: _io.size - _io.pos == 61
      - id: short_tail
        type: embedded_pattern_model_tail_short
        if: _io.size - _io.pos == 28

  embedded_pattern_model_placement:
    doc: One 18-byte 3D-model placement record in an embedded pattern.
    seq:
      - id: field_a
        type: int3
      - id: field_b
        type: int3
      - id: field_c
        type: int3
      - id: field_d
        type: int3
      - id: field_e
        type: int3
      - id: field_f
        type: int3

  embedded_pattern_model_tail_short:
    doc: |
      Short 28-byte 3D-model transform tail accepted by parseEmbeddedPattern:
      Offset XYZ, Rotate XYZ, and Zoom X.  Older/variant records end here.
    seq:
      - id: offset_x
        type: int4
      - id: offset_y
        type: int4
      - id: offset_z
        type: int4
      - id: rotate_x
        type: int4
      - id: rotate_y
        type: int4
      - id: rotate_z
        type: int4
      - id: zoom_x
        type: int4

  embedded_pattern_model_tail_long:
    doc: |
      Long 61-byte 3D-model transform/attribute tail accepted by parseEmbeddedPattern.
      The first 36 bytes are 9 biased int4 values: Offset XYZ, Rotate XYZ, Zoom XYZ.
      The final 25 bytes are four flag bytes plus seven biased int3 attribute fields;
      observed exported examples include attribute tuples such as 0,0,0,1,3,-1,4.
    seq:
      - id: offset_x
        type: int4
      - id: offset_y
        type: int4
      - id: offset_z
        type: int4
      - id: rotate_x
        type: int4
      - id: rotate_y
        type: int4
      - id: rotate_z
        type: int4
      - id: zoom_x
        type: int4
      - id: zoom_y
        type: int4
      - id: zoom_z
        type: int4
      - id: attr_flag_a
        type: u1
      - id: attr_flag_b
        type: u1
      - id: attr_flag_c
        type: u1
      - id: attr_flag_d
        type: u1
      - id: attr_field_a
        type: int3
      - id: attr_field_b
        type: int3
      - id: attr_field_c
        type: int3
      - id: attr_field_d
        type: int3
      - id: attr_field_e
        type: int3
      - id: attr_field_f
        type: int3
      - id: attr_field_g
        type: int3

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
        repeat-expr: 'field_a.value > 0 ? field_a.value - 1 : 0'
        doc: |
          Legacy pad records: one template pad plus real pads.  field_a includes
          the following empty 39-byte pad terminator, so only field_a - 1 normal
          pad records appear here.
      - id: pad_terminator
        type: legacy_pad_terminator(version)
        if: field_a.value > 0
        doc: Empty pad-like 39-byte terminator trailing the normal pad records.
      - id: field_b
        type: int3
        if: field_a.value > 0
        doc: Sentinel-record count.
      - id: pre_sentinel
        type: legacy_pre_sentinel
        if: field_a.value > 0
        doc: 55 bytes preceding the sentinel records.
      - id: sentinels
        type: legacy_sentinel(_index, field_b.value)
        repeat: expr
        repeat-expr: 'field_a.value > 0 ? field_b.value : 0'

  legacy_pad:
    doc: One pad/template record in a legacy embedded pattern.
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
        type: legacy_pad_tail

  legacy_sentinel:
    doc: |
      One sentinel record.  The last record (index == count-1) is a 49-byte footer;
      all earlier records are 62 bytes.
    params:
      - id: index
        type: s4
      - id: count
        type: s4
    seq:
      - id: data
        type: legacy_sentinel_payload(index, count)

  legacy_pad_tail:
    doc: Fixed 11-byte tail after a legacy embedded-pattern pad/template record.
    seq:
      - id: flag_a
        type: u1
      - id: style_index
        type: int3
        doc: Pad-style selector; observed 0 for PadT0 and 2 for the rectangular PadT1 pad in LED100.
      - id: flag_b
        type: u1
      - id: angle
        type: int3
        doc: Pad angle; observed 0 in legacy examples.
      - id: tail_flag_a
        type: u1
      - id: tail_flag_b
        type: u1
      - id: tail_flag_c
        type: u1

  legacy_pad_terminator:
    doc: |
      Empty pad-like 39-byte terminator following legacy embedded-pattern pad
      records.  In Schematic_4.dch LED100 this starts at 0x475: pad_id/x/y,
      two empty ASCII strings, zero dimensions, then a short 9-byte tail.
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
      - id: note
        type: dt_string(version)
      - id: width
        type: int4
      - id: height
        type: int4
      - id: drill
        type: int4
      - id: tail
        type: legacy_pad_terminator_tail

  legacy_pre_sentinel:
    doc: |
      55-byte block preceding legacy embedded-pattern sentinel records.  Schematic_4.dch
      LED100 has this block at 0x49F after field_b=4.
    seq:
      - id: field_a
        type: int3
      - id: field_b
        type: int4
      - id: field_c
        type: int4
      - id: field_d
        type: int4
      - id: field_e
        type: int4
      - id: field_f
        type: int4
      - id: field_g
        type: int4
      - id: flag_a
        type: u1
      - id: field_h
        type: int3
      - id: field_i
        type: int3
      - id: field_j
        type: int3
      - id: flag_b
        type: u1
      - id: field_k
        type: int3
      - id: field_l
        type: int4
      - id: field_m
        type: int4
      - id: field_n
        type: int3
      - id: field_o
        type: int3

  legacy_pad_terminator_tail:
    doc: 9-byte short tail of the empty legacy pad terminator.
    seq:
      - id: field_a
        type: int3
      - id: flag_a
        type: u1
      - id: field_b
        type: int3
      - id: flag_b
        type: u1
      - id: flag_c
        type: u1

  legacy_sentinel_payload:
    doc: |
      Legacy embedded-pattern sentinel payload; final entry is 49 bytes, earlier
      entries are 62 bytes.  In Schematic_4.dch LED100 these records hold the
      stored pattern-shape/framing tail after the pad terminator.
    params:
      - id: index
        type: s4
      - id: count
        type: s4
    seq:
      - id: body
        type: legacy_sentinel_payload_final
        if: index == count - 1
      - id: body_long
        type: legacy_sentinel_payload_long
        if: index != count - 1

  legacy_sentinel_payload_long:
    doc: 62-byte non-final legacy sentinel record.
    seq:
      - id: field_a
        type: int4
      - id: field_b
        type: int3
      - id: record_kind
        type: int3
      - id: x1
        type: int4
      - id: y1
        type: int4
      - id: x2
        type: int4
      - id: y2
        type: int4
      - id: x3
        type: int4
      - id: y3
        type: int4
      - id: flag_a
        type: u1
      - id: field_c
        type: int3
      - id: field_d
        type: int3
      - id: field_e
        type: int3
      - id: flag_b
        type: u1
      - id: field_f
        type: int3
      - id: field_g
        type: int4
      - id: field_h
        type: int4
      - id: field_i
        type: int3
      - id: field_j
        type: int3
      - id: flag_c
        type: u1

  legacy_sentinel_payload_final:
    doc: 49-byte final legacy sentinel record.
    seq:
      - id: field_a
        type: int4
      - id: field_b
        type: int3
      - id: record_kind
        type: int3
      - id: flag_a
        type: u1
      - id: field_c
        type: int4
      - id: field_d
        type: int4
      - id: field_e
        type: int4
      - id: field_f
        type: int4
      - id: flag_b
        type: u1
      - id: field_g
        type: int4
      - id: field_h
        type: int4
      - id: field_i
        type: int4
      - id: field_j
        type: int4
      - id: flag_c
        type: u1
      - id: field_k
        type: int3
      - id: flag_d
        type: u1

  # --- Section 7: bus section (parseBusSection, lines ~1474-1538) -------------

  bus_section:
    doc: |
      Bus section located by findBusSection() (marker
      3B 9A F1 10 3B 9A F1 10 00 00 + int3 count).  This type describes the section the
      importer reads starting at that offset.  Files without a valid bus marker end the component
      section at the tail block and have no bus_section.  A marker with positive bus_count outside
      [0, 1000] is fatal rather than skipped.  Entry terminator int3 values must decode to -1;
      any other value is fatal.
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
        doc: Must decode to -1; any other value is fatal.
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
      import).  Nets are separated by a variable preamble, but an accepted wire-net
      record has this fixed 17-byte lead-in: byte 0x01, three marker bytes, int3 field A,
      int3 field B, int3 sequential net_index, and the int3(-1) marker.  The importer
      scans only for records whose stored net_index is the next expected value; once
      that marker is accepted, a malformed UTF-16 name, pin_count outside [0,4000],
      wire_count outside [0,100000], or a wire payload overrun is fatal.
    params:
      - id: version
        type: s4
    seq:
      - id: pre_marker_flag
        type: u1
        valid:
          expr: _ == 1
      - id: pre_marker_flag_a
        type: u1
        doc: First byte preceding the explicit int3 fields; observed 0.
      - id: pre_marker_flag_b
        type: u1
        doc: Second byte preceding the explicit int3 fields; observed 0 or 1.
      - id: pre_marker_flag_c
        type: u1
        doc: Third byte preceding the explicit int3 fields; observed 0.
      - id: preamble_field_a
        type: int3
        valid:
          expr: _.value >= -1 and _.value <= 100000
      - id: preamble_field_b
        type: int3
        valid:
          expr: _.value >= 0 and _.value <= 100000
      - id: net_index
        type: int3
        doc: Sequential wire-net index; importer accepts only the next expected value.
      - id: marker
        contents: [0x0f, 0x42, 0x3f]
        doc: int3 == -1 marker immediately preceding the UTF-16 net name.
      - id: name
        type: dt_string(version)
      - id: label_x
        type: int4
      - id: label_y
        type: int4
      - id: pad
        type: int3
        valid:
          expr: _.value == 0
      - id: flag
        type: u1
        valid:
          any-of: [0, 1]
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
      One wire of a net.  The 12-int3 header carries the connection metadata:
      endpoint object/subobject ids, parent bus ids, sheet index, endpoint connection
      kinds (0 = Pin, 1 = Wire), parent net index, global wire index, and per-net wire id.
      For an accepted net record, point_count outside [0,4000], or a point payload that
      would overrun the section, is fatal.
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
      - id: connected1_type
        type: int3
        doc: Endpoint 1 connection kind. XML Connected1 maps 0 = Pin, 1 = Wire.
      - id: connected2_type
        type: int3
        doc: Endpoint 2 connection kind. XML Connected2 maps 0 = Pin, 1 = Wire.
      - id: parent_net_index
        type: int3
        doc: Duplicate of the containing net index / XML Net Id.
      - id: global_wire_index
        type: int3
        doc: Monotonic wire ordinal across the whole schematic wire section.
      - id: wire_id
        type: int3
        doc: Per-net wire id / XML Wire Id.
      - id: route_flag
        type: u1
        doc: Observed 0 in the decoded corpus.
      - id: point_count
        type: int3
      - id: points
        type: wire_point
        repeat: expr
        repeat-expr: point_count.value
      - id: trailer
        type: wire_trailer

  wire_trailer:
    doc: |
      Per-wire trailer after all points. Observed in CNC_controller.dch and the reference schematic
      as bytes 00 0F 42 3F 00 0F 42 40, i.e. u1(0), int3(-1), u1(0), int3(0).
    seq:
      - id: marker1_flag
        type: u1
      - id: marker1_value
        type: int3
      - id: marker2_flag
        type: u1
      - id: marker2_value
        type: int3

  wire_point:
    doc: A wire vertex (X int4, Y int4, direction int3), 11 bytes.
    seq:
      - id: x
        type: int4
      - id: y
        type: int4
      - id: dir
        type: int3
