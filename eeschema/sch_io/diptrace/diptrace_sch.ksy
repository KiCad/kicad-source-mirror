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
#     regions are kept raw at the top level and only the record types are provided.
#
#   * The optional bus section is located by findBusSection() (marker search).  When the
#     marker is absent the importer treats the component section as ending at the trailing
#     tail block; bus_section / bus_entry describe the present form.  A marker with an
#     out-of-range positive bus_count is fatal, not skipped in favor of a later candidate.
#
#   * The net section (parseNetSection) is a forward marker scan (0x0F 0x42 0x3F) for net
#     labels; net_label_scan_record describes one match.
#
#   * The wire section (parseWireSection, v38+ only) is deterministic for accepted
#     records: the importer scans for the explicit wire-net marker lead-in, requires
#     the next sequential net_index, and then parses net_wire_record / wire.  The
#     variable bytes between records keep the section raw at the top level for now.

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
    if: header.magic_major_value != 1
  - id: pre_component_settings
    type: pre_component_settings(header.version_value, header.magic_major_value)
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
      version < 34: int3 byte-count + raw ASCII bytes.
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
          Raw u4 counts >= 10000 with a zero high word are fatal.  The C++ has additional
          2-byte pin-separator and tailless fallbacks not statically expressible; common
          path modelled.
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
          Trailing shapes (parseShape, each isShapeStart()-gated), stored component
          text fields, then the embedded footprint pattern.  Shapes and text fields
          are not count-prefixed in this record; see types shape, component_text_field,
          and embedded_pattern for the decoded record layouts.

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
      payload is char_count * 2 UTF-16-BE bytes.  Raw u4 counts >= 10000 with a zero high
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
      size depends on the version (2 int3 for <34, 4 int3 for 34+).  A 5-field skip /
      int3 block in the middle differs between legacy and modern layouts.  Modern pins
      commonly store two zero prefix bytes, a UTF-16 text token (often empty; examples
      include ND/NG/NS), one terminator byte, then an int3 field.
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
    doc: |
      Modern (34+) mid-pin block.  The importer decodes the represented common form
      when the first two bytes are zero.  Nonzero-prefix variants fall back to the older
      fixed 5-byte skip until their stored fields are identified.
    seq:
      - id: prefix
        size: 2
      - id: text
        type: dt_string(34)
      - id: terminator
        type: u1
      - id: field_a
        type: int3

  # --- Component shape (parseShape, lines ~1220-1259) ------------------------

  shape:
    doc: |
      One graphical polyline inside a component.  parseShape() is gated by isShapeStart()
      and reads a version-dependent prefix.  If the version-dependent shape header prefix
      is present, numPoints outside [1,100] is fatal for credible counts instead of being
      silently treated as "no shape".
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
        valid:
          min: 1
          max: 100
      - id: points
        type: shape_point
        repeat: expr
        repeat-expr: num_points.value
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
      - id: flags
        size: 3
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
      - id: flags2
        size: 4
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
      - id: modern_raw
        size-eos: true
        if: version >= 34
        doc: |
          Modern embedded pattern blob.  See embedded_pattern_modern_common for the
          deterministic common layout currently parsed by the importer.  This remains
          raw at this wrapper level because modern connector/net-port variants can
          start with nonzero lead-in tails before reaching the common layout.

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
      - id: pattern_name
        type: dt_string(version)
      - id: post_name
        type: embedded_pattern_modern_post_name(version)

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
        doc: Counted name/value property pairs; v41 examples include "Case Info".
      - id: properties
        type: embedded_pattern_property(version)
        repeat: expr
        repeat-expr: property_count.value
      - id: post_properties_a
        type: int3
      - id: post_properties_b
        type: int3
        if: property_count.value > 0
      - id: pad_count
        type: int3
      - id: empty_footer
        type: int3
        repeat: expr
        repeat-expr: 'pad_count.value == 0 ? 3 : 0'
      - id: pads_and_model
        size-eos: true
        if: pad_count.value > 0
        doc: |
          Modern pad records and model tail.  The importer validates pad records and
          model tail placement while consuming; see embedded_pattern_modern_pad and
          embedded_pattern_model_tail.

  embedded_pattern_property:
    params:
      - id: version
        type: s4
    seq:
      - id: name
        type: dt_string(version)
      - id: value
        type: dt_string(version)

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

  embedded_pattern_model_tail:
    doc: |
      Counted 3D-model placement records followed by a UTF-16-BE model file name and
      either a 61-byte or 28-byte tail.
    params:
      - id: version
        type: s4
    seq:
      - id: placement_count
        type: int3
      - id: placements
        size: 18
        repeat: expr
        repeat-expr: placement_count.value
      - id: post_placements
        type: int3
      - id: model_name
        type: dt_string(version)
      - id: tail
        size-eos: true

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
      record has this fixed 17-byte lead-in: byte 0x01, three raw bytes, int3 field A,
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
          expr: 1
      - id: pre_marker_raw
        size: 3
        doc: Three bytes preceding the explicit int3 fields; observed as 00 00 00 or 00 01 00.
      - id: preamble_field_a
        type: int3
        valid:
          min: -1
          max: 100000
      - id: preamble_field_b
        type: int3
        valid:
          min: 0
          max: 100000
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
          expr: 0
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
      One wire of a net.  The 12-int3 header carries the connection metadata; the
      importer only assigns meaning to the first seven tokens (object1, object2,
      sub_object1, sub_object2, bus1, bus2, sheet_index) and skips the remaining five.
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
