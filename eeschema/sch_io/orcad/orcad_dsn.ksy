# Parse an extracted OLE stream, not the complete DSN or OLB file.
# See ORCAD_V2_FORMAT.md for the container layout.

meta:
  id: orcad_dsn
  endian: le
  encoding: ISO-8859-1

doc: |
  Structure framing.  Every framed structure in a Page/Cache/Schematic/Hierarchy stream is

      N x long prefix    u8 type_id, u32 body_len, u32 (always zero)
      1 x short prefix   u8 type_id, i16 n_props, n_props x (u32 name_idx, u32 value_idx)
      preamble           FF E4 5C 39, u32 trail_len, trail_len bytes
      body               type-specific

  All prefix type bytes in one chain carry the same value.  A negative short-prefix pair
  count means "no pairs".  Property indices address library_stream.strings.

  Long-prefix body_len counts the bytes from the END of its own 9-byte record to the end
  of the structure, so long prefix i yields the checkpoint

      stop[i] = structure_start + 9 * i + 9 + body_len[i]

  and stop[0] is the offset just past the whole structure.  Type 9 is a stream header:
  its sole stop equals the body start, and the page-order payload continues inline.
  Readers that cannot fully
  interpret a body seek to stop[0] instead; drawn_instance additionally uses the
  second-to-last stop to find its reference designator.

types:

  # -- primitives ----------------------------------------------------------------------

  lzt:
    doc: |
      u16 length, that many bytes, then a NUL that is NOT counted by the length.
      The importer rejects a string whose terminator byte is non-zero, which is the
      main guard against a mis-framed record silently consuming the rest of a stream.
    seq:
      - id: len
        type: u2
      - id: value
        type: str
        size: len
      - id: terminator
        contents: [0]

  zt:
    doc: Bare NUL-terminated string, used only for the Library introduction field.
    seq:
      - id: value
        type: strz

  # -- framing -------------------------------------------------------------------------

  long_prefix:
    seq:
      - id: type_id
        type: u1
        enum: structure_type
      - id: body_len
        type: u4
        doc: Bytes from the end of this 9-byte record to the end of the structure.
      - id: pad
        contents: [0, 0, 0, 0]

  short_prefix:
    seq:
      - id: type_id
        type: u1
        enum: structure_type
      - id: num_props
        type: s2
        doc: Negative means no property pairs follow.
      - id: props
        type: prop_pair
        repeat: expr
        repeat-expr: 'num_props < 0 ? 0 : num_props'

  prop_pair:
    doc: Both fields index library_stream.strings.
    seq:
      - id: name_idx
        type: u4
      - id: value_idx
        type: u4

  preamble:
    seq:
      - id: magic
        contents: [0xFF, 0xE4, 0x5C, 0x39]
      - id: len_trail
        type: u4
      - id: trail
        size: len_trail

  short_prefix_v2:
    doc: Short-only framing used by legacy and short-framed version-3.2 streams.
    seq:
      - id: type_id
        type: u1
        enum: structure_type
      - id: num_props
        type: s2
      - id: props
        type: prop_pair_v2
        repeat: expr
        repeat-expr: 'num_props < 0 ? 0 : num_props'

  prop_pair_v2:
    seq:
      - id: name_idx
        type: u2
      - id: value_idx
        type: u2

  framed_structure:
    doc: |
      One structure with the type-owned long-prefix count supplied by its grammar
      position.  Registered counts are:

        1: 9; occurrences 66, 67, 68, 69, 82, 91
        2: 10, 16, 17, 20, 21, 26, 27, 29, 32, 37, 38, 39, 48, 49,
           55..62, 88, 89
        3: 2, 6, 12, 23, 31, 65, 77
        4: 13, 33, 34, 35, 64, 75, 76
        5: 24

      Context-owned exceptions pass an explicit count, such as the one-prefix nested
      type-48 SymbolVector.  The body is opaque here because C++ dispatches on type_id.
    params:
      - id: num_long_prefixes
        type: s4
      - id: len_body
        type: s4
    seq:
      - id: long_prefixes
        type: long_prefix
        repeat: expr
        repeat-expr: num_long_prefixes
      - id: short_prefix
        type: short_prefix
      - id: preamble
        type: preamble
      - id: body
        size: len_body

  framed_record:
    doc: Executable boundary grammar for a structure whose type-owned prefix depth is known.
    params:
      - id: num_long_prefixes
        type: s4
    seq:
      - id: type_id
        type: u1
      - id: len_tail
        type: u4
      - id: pad
        contents: [0, 0, 0, 0]
      - id: tail
        type: framed_record_tail(num_long_prefixes - 1, type_id)
        size: len_tail

  framed_record_tail:
    params:
      - id: num_long_prefixes
        type: s4
      - id: expected_type
        type: u1
    seq:
      - id: long_prefixes
        type: matching_long_prefix(expected_type)
        repeat: expr
        repeat-expr: num_long_prefixes
      - id: short_prefix
        type: matching_short_prefix(expected_type)
      - id: preamble
        type: preamble
      - id: body
        size-eos: true

  matching_long_prefix:
    params:
      - id: expected_type
        type: u1
    seq:
      - id: type_id
        type: u1
        valid: expected_type
      - id: body_len
        type: u4
      - id: pad
        contents: [0, 0, 0, 0]

  matching_short_prefix:
    params:
      - id: expected_type
        type: u1
    seq:
      - id: type_id
        type: u1
        valid: expected_type
      - id: num_props
        type: s2
      - id: props
        type: prop_pair
        repeat: expr
        repeat-expr: 'num_props < 0 ? 0 : num_props'

  # -- /Library ------------------------------------------------------------------------

  library_stream:
    doc: |
      Format version, the font table, the eight named part fields, the default page
      settings and -- most importantly -- the global string table that every property
      index in every other stream resolves against.

      The Library major version selects the string-table count width: versions 1 and 2
      use u16; version 3 uses u32.  Other major versions are unsupported.  The parser
      never retries the other width.
    seq:
      - id: introduction
        type: strz
        size: 32
        doc: |
          NUL-terminated inside a fixed 32-byte buffer.  A design file starts with
          "OrCAD Windows Design"; a standalone .OLB does not, and only a design carries
          schematic_name below.
      - id: version_major
        type: u2
      - id: version_minor
        type: u2
      - id: dates
        size: 8
        doc: Create and modify timestamps.
      - id: zeros
        size: 4
      - id: num_fonts_plus_one
        type: u2
      - id: fonts
        type: logfont
        repeat: expr
        repeat-expr: num_fonts_plus_one - 1
      - id: unknown_len
        type: u2
        if: version_major >= 2
        doc: Always 24 in observed files; version_major >= 2 only.
      - id: unknown_block
        size: unknown_len * 2
        if: version_major >= 2
      - id: unknown_tail
        size: 8
        if: version_major >= 2
      - id: legacy_preferences
        size: 42
        if: version_major < 2
      - id: part_fields
        type: lzt
        repeat: expr
        repeat-expr: 8
        doc: '"Part Reference", "Value", ... in fixed order.'
      - id: page_settings
        type: page_settings
      - id: num_strings_legacy
        type: u2
        if: version_major < 3
      - id: num_strings_modern
        type: u4
        if: version_major >= 3
      - id: strings
        type: lzt
        repeat: expr
        repeat-expr: num_strings
        doc: Global string table; every u32 property index elsewhere addresses this.
      - id: num_aliases
        type: u2
      - id: aliases
        type: part_alias
        repeat: expr
        repeat-expr: num_aliases
      - id: schematic_name_pad
        size: 8
        doc: Design files only (see introduction).
      - id: schematic_name
        type: lzt
        doc: |
          Name of the root schematic FOLDER under /Views.  SCH_IO_ORCAD matches this
          case-insensitively against the folder names to pick the root; when it does not
          match, the first folder in sorted order is used.
    instances:
      num_strings:
        value: 'version_major < 3 ? num_strings_legacy : num_strings_modern'

  logfont:
    doc: A Win32 LOGFONTA record.  Point size is approximately -height * 72 / 96.
    seq:
      - id: height
        type: s4
        doc: Negative device units.
      - id: unknown_a
        size: 12
      - id: weight
        type: s4
        doc: '>= 600 is rendered bold.'
      - id: italic
        type: u1
      - id: unknown_b
        size: 7
      - id: face_name
        type: strz
        size: 32

  part_alias:
    seq:
      - id: alias
        type: lzt
      - id: part
        type: lzt

  page_settings:
    doc: |
      156 bytes.  Only width, height, pin_to_pin and flags[0] are consumed; the rest is
      grid and drawing-preference state that has no KiCad counterpart.
    seq:
      - id: dates
        size: 8
      - id: unknown_a
        size: 16
      - id: width
        type: u4
        doc: Mils, or micrometres when flags[0] is set.
      - id: height
        type: u4
      - id: pin_to_pin
        type: u4
        doc: Grid pitch; 100 mils by default.
      - id: unknown_b
        size: 2
      - id: grid_counts
        size: 4
      - id: unknown_c
        size: 2
      - id: grid_widths
        size: 8
      - id: unknown_d
        size: 48
      - id: grid_ref_chars
        size: 24
      - id: flags
        type: u4
        repeat: expr
        repeat-expr: 8
        doc: flags[0] non-zero selects metric units for width/height.

  # -- /Cache and /Packages ------------------------------------------------------------

  cache_stream:
    doc: |
      The C++ reader consumes this payload sequentially.  After the four-byte header,
      each entry is: an LZT name (optionally preceded by one opaque u16), zero or more
      (u16 kind, LZT descriptor) pairs, duplicated u32 database ids, a u16 structure
      type echo, then exactly one type-framed structure.  A descriptor may likewise
      have one opaque u16 before its LZT.

      The final entry is allowed to be metadata-only.  It ends immediately after its
      descriptor kind, or after one additional zero byte.  That explicit production
      replaces the former search for a later structure preamble.  The lexical LZT test
      is bounded to 1024 printable bytes (plus byte 0x02) and requires its NUL.

      cache_payload remains opaque here because Kaitai cannot express that bounded
      lexical lookahead without duplicating the stream.  orcad_cache.cpp implements the
      production above without moving the cursor backward.
    seq:
      - id: zero_header
        contents: [0, 0]
      - id: unknown
        type: u2
      - id: cache_payload
        size-eos: true

  packages_stream:
    doc: |
      u16 PartCell count; for every type-6 PartCell, a following u16 LibraryPart count
      and that many type-24 LibraryParts; finally one type-31 Package and EOF.  Symbol
      streams contain exactly one type-framed symbol and EOF.  These are distinct
      top-level grammars; the importer does not run the Cache grammar against them.
    seq:
      - id: num_part_cells
        type: u2
      - id: part_cells
        type: package_part_cell
        repeat: expr
        repeat-expr: num_part_cells
      - id: package
        type: framed_record(3)

  package_part_cell:
    seq:
      - id: cell
        type: framed_record(3)
      - id: num_library_parts
        type: u2
      - id: library_parts
        type: framed_record(5)
        repeat: expr
        repeat-expr: num_library_parts

  # -- /Views/<folder>/Schematic -------------------------------------------------------

  schematic_stream:
    doc: |
      One schematic folder's page display order.  The importer intersects these names
      with the streams that actually exist under Pages/, appends any page the order
      stream omits, and falls back to name order when this stream cannot be read.
      The C++ parser selects long versus short framing once from the long prefix's
      mandatory four-byte zero pad.  Bytes after page_names belong to other Capture
      folder data and are not a second page-order grammar.
    seq:
      - id: header
        type: framed_structure(1, 0)
        doc: The body is the rest of this type, read inline rather than as a size.
      - id: folder_name
        type: lzt
      - id: unknown
        size: 4
      - id: num_page_names
        type: u2
      - id: page_names
        type: lzt
        repeat: expr
        repeat-expr: num_page_names
        doc: Stored LAST page first; reverse for display order.

  schematic_stream_short:
    doc: Short-framed form selected when bytes 5..8 are not the mandatory long-prefix zero pad.
    seq:
      - id: header
        type: short_prefix_v2
      - id: folder_name
        type: lzt
      - id: unknown
        size: 4
      - id: num_page_names
        type: u2
      - id: page_names
        type: lzt
        repeat: expr
        repeat-expr: num_page_names

  # -- /Views/<root>/Hierarchy/Hierarchy ------------------------------------------------

  hierarchy_stream:
    doc: |
      The design occurrence tree: every instantiation path in the design, once.  This is
      what lets the importer rebuild Capture's hierarchy as KiCad sheets instead of a
      flat page list, and what supplies per-occurrence reference designators for designs
      that leave the placed instance's own field as the "C?" template.

      A scope is one point in an instantiation path.  Its block occurrences carry a child
      folder name and their own nested scope, so a schematic reused N times yields N
      independent designator sets.

      Legacy pre-preamble streams do not start with a 0x42 long prefix.  Those are
      instance-annotated and OrcadReadOccurrenceTree() returns an empty tree.  Malformed
      modern trees disable hierarchy reconstruction; no signature scan follows.
    seq:
      - id: type_id
        contents: [0x42]
      - id: body_len
        type: u4
      - id: pad
        contents: [0, 0, 0, 0]
      - id: view_name
        type: lzt
      - id: zeros
        size: 7
      - id: root_scope
        type: root_occ_scope

  root_occ_scope:
    doc: |
      Root-only tables precede the recursive occurrence scope: power entries (0x44),
      net mappings (0x43), title blocks (0x52), globals (0x5b), then part/block
      occurrences (0x42).  Nested occurrences use occ_scope below.
    seq:
      - id: num_power_entries
        type: u2
      - id: power_entries
        type: occ_named
        repeat: expr
        repeat-expr: num_power_entries
      - id: num_nets
        type: u2
      - id: nets
        type: occ_named
        repeat: expr
        repeat-expr: num_nets
      - id: num_title_blocks
        type: u2
      - id: title_blocks
        type: occ_pair
        repeat: expr
        repeat-expr: num_title_blocks
      - id: num_globals
        type: u4
      - id: globals
        type: occ_pair
        repeat: expr
        repeat-expr: num_globals
      - id: separator
        type: preamble
        if: has_separator == 0x395CE4FF
      - id: num_occurrences
        type: u2
      - id: occurrences
        type: occurrence
        repeat: expr
        repeat-expr: num_occurrences
    instances:
      has_separator:
        pos: _io.pos
        type: u4
        if: _io.size - _io.pos >= 4

  occ_header:
    doc: |
      Occurrence records repeat the framing of a normal structure but always with exactly
      one long prefix.  The type byte identifies what the record is; the reader checks it
      against the type the position demands and fails the Hierarchy stream on mismatch.
    seq:
      - id: type_id
        type: u1
        enum: occurrence_type
      - id: body_len
        type: u4
      - id: pad
        contents: [0, 0, 0, 0]
      - id: short_prefix
        type: short_prefix
      - id: preamble
        type: preamble

  occ_scope:
    doc: |
      The occurrences visible at one point in an instantiation path.  Net, title-block and
      global occurrences are read only to keep the stream aligned; the importer keeps the
      part and block occurrences.
    seq:
      - id: num_nets
        type: u2
      - id: nets
        type: occ_net
        repeat: expr
        repeat-expr: num_nets
      - id: num_title_blocks
        type: u2
      - id: title_blocks
        type: occ_pair
        repeat: expr
        repeat-expr: num_title_blocks
      - id: num_globals
        type: u4
        doc: u32, unlike the u16 counts around it.
      - id: globals
        type: occ_pair
        repeat: expr
        repeat-expr: num_globals
      - id: separator
        type: preamble
        if: has_separator == 0x395CE4FF
        doc: |
          A bare preamble sometimes sits between the global occurrences and the part /
          block occurrences.  It carries no payload the importer needs; both it and its
          trailing block are skipped.
      - id: num_occurrences
        type: u2
      - id: occurrences
        type: occurrence
        repeat: expr
        repeat-expr: num_occurrences
    instances:
      has_separator:
        pos: _io.pos
        type: u4
        doc: |
          Peek at the four bytes that follow the globals; FF E4 5C 39 read little-endian
          is 0x395CE4FF.  Compared, not stored, so the peek does not advance the cursor.
        if: _io.size - _io.pos >= 4

  occ_net:
    seq:
      - id: header
        type: occ_header
      - id: db_id
        type: u4
      - id: name
        type: lzt

  occ_named:
    doc: Root power/net table entry; header type is fixed by its containing table.
    seq:
      - id: header
        type: occ_header
      - id: db_id
        type: u4
      - id: name
        type: lzt

  occ_pair:
    doc: Title-block (0x52) and global/off-page (0x5b) occurrences; contents unused.
    seq:
      - id: header
        type: occ_header
      - id: db_id
        type: u4
      - id: unknown
        type: u4

  occurrence:
    doc: |
      One part or block occurrence.  child_folder is empty for a part and names the child
      schematic folder for a hierarchical block; that is the only discriminator.

      target_db_id is the db id of the record this occurrence stands for on the parent
      page: a placed_instance (type 13) for a part, a drawn_instance (type 12) for a
      block.  ORCAD_CONVERTER::canBuildHierarchy() requires every block occurrence in a
      scope to match exactly one drawn_instance across the pages of the parent schematic,
      and every drawn_instance to be accounted for, before it will rebuild the hierarchy.
    seq:
      - id: header
        type: occ_header
      - id: own_db_id
        type: u4
      - id: target_db_id
        type: u4
      - id: inner_marker
        contents: [0x42]
      - id: complexity
        type: u4
        doc: Correlates with symbol complexity; unused.
      - id: unknown_d
        type: u4
      - id: child_folder
        type: lzt
        doc: Child schematic folder name; empty for a part occurrence.
      - id: reference
        type: lzt
        doc: Occurrence reference designator; empty when the design has none.
      - id: unknown_f
        type: u4
        doc: Zero except on rare multi-unit parts.
      - id: num_pins
        type: u2
      - id: pins
        type: occ_pin
        repeat: expr
        repeat-expr: num_pins
      - id: nested
        type: occ_scope
        doc: The child's occurrences under this instantiation path.

  occ_pin:
    doc: type_id is 0x44 for a scalar pin occurrence and 0x45 for a bus pin occurrence.
    seq:
      - id: header
        type: occ_header
      - id: db_id
        type: u4
      - id: pin_index
        type: u2

  hierarchy_link:
    doc: Structure type 66, retained as a documented record body; not used for resynchronization.
    seq:
      - id: unknown_a
        type: u4
      - id: instance_db_id
        type: u4
      - id: inner_marker
        contents: [0x42]
      - id: unknown_b
        type: u4
      - id: unknown_c
        type: u4
      - id: child_folder
        type: lzt
        doc: Empty when this link is not a hierarchical block.
      - id: reference
        type: lzt
        doc: Occurrence reference designator; empty when none.

  # -- /Views/<folder>/Pages/<page> -----------------------------------------------------

  page_stream:
    doc: |
      One schematic page.  The whole stream is a single framed structure of type 10, and
      the fields below are its BODY: seek past the prefix chain and preamble first.

      The body is a run of u16-counted lists whose entries use the fixed type-to-prefix
      table.  The list order is
      here; it is the order OrcadParsePage() walks, and getting it wrong is the usual
      way a page fails to load:

        1.  title blocks         framed type 65
        2.  t0x34 records        raw, NOT prefix-framed (see t0x34)
        3.  t0x35 records        raw, NOT prefix-framed (see t0x35)
        4.  net table            lzt name + u32 db id per entry (see net_entry)
        5.  wires                framed types 20 (scalar), 21 (bus)
        6.  placed parts         framed type 13 (part) and type 12 (block) in one list
        7.  ports                framed type 23
        8.  globals              framed type 37, each entry followed by 5 bytes
        9.  off-page connectors  framed type 38, each entry followed by 5 bytes
        10. ERC objects          framed type 77
        11. bus entries          framed type 29
        12. graphic instances    framed types 55..62, 88, 89

      The net table is authoritative for net names: wires carry net db ids that key it,
      and the importer computes junctions from it rather than from the wire geometry.

      Two of these lists carry the hierarchy.  List 6 holds the drawn_instance records
      that are a page's hierarchical blocks; the presence of any of them is what marks a
      page as hierarchical.  List 7 holds the ports that become a child sheet's
      hierarchical labels, and their names are what match a parent block's pins --
      including which page of a multi-page child schematic owns each pin.
    seq:
      - id: name
        type: lzt
      - id: page_size
        type: lzt
        doc: Page-size name, e.g. "B"; may be empty.
      - id: settings
        type: page_settings
      - id: num_title_blocks
        type: u2
      - id: title_blocks
        type: framed_record(3)
        repeat: expr
        repeat-expr: num_title_blocks
      - id: num_t0x34_records
        type: u2
      - id: t0x34_records
        type: t0x34
        repeat: expr
        repeat-expr: num_t0x34_records
      - id: num_t0x35_records
        type: u2
      - id: t0x35_records
        type: t0x35
        repeat: expr
        repeat-expr: num_t0x35_records
      - id: num_nets
        type: u2
      - id: nets
        type: net_entry
        repeat: expr
        repeat-expr: num_nets
      - id: num_wires
        type: u2
      - id: wires
        type: framed_record(2)
        repeat: expr
        repeat-expr: num_wires
      - id: num_placed
        type: u2
      - id: placed
        type: placed_record
        repeat: expr
        repeat-expr: num_placed
      - id: num_ports
        type: u2
      - id: ports
        type: framed_record(3)
        repeat: expr
        repeat-expr: num_ports
      - id: num_globals
        type: u2
      - id: globals
        type: framed_record_with_trailer(2, 5)
        repeat: expr
        repeat-expr: num_globals
      - id: num_offpage
        type: u2
      - id: offpage
        type: framed_record_with_trailer(2, 5)
        repeat: expr
        repeat-expr: num_offpage
      - id: num_erc
        type: u2
      - id: erc
        type: framed_record(3)
        repeat: expr
        repeat-expr: num_erc
      - id: num_bus_entries
        type: u2
      - id: bus_entries
        type: framed_record(2)
        repeat: expr
        repeat-expr: num_bus_entries
      - id: num_graphics
        type: u2
      - id: graphics
        type: framed_record(2)
        repeat: expr
        repeat-expr: num_graphics

  placed_record:
    seq:
      - id: record
        type: 'framed_record(next_type == 12 ? 3 : 4)'
    instances:
      next_type:
        pos: _io.pos
        type: u1

  framed_record_with_trailer:
    params:
      - id: num_long_prefixes
        type: s4
      - id: len_trailer
        type: s4
    seq:
      - id: record
        type: framed_record(num_long_prefixes)
      - id: trailer
        size: len_trailer

  unparsed_structure_list:
    doc: |
      u16 count followed by that many framed structures.  Callers supply the registered
      prefix depth allowed by that list position.
    seq:
      - id: count
        type: u2

  net_entry:
    seq:
      - id: name
        type: lzt
      - id: db_id
        type: u4

  t0x34:
    doc: NOT prefix-framed, unlike everything else in the page.  Content is unused.
    seq:
      - id: unknown_a
        size: 9
      - id: id
        type: u4
      - id: name
        type: lzt
      - id: unknown_b
        type: u4
      - id: color
        type: u4
      - id: line_style
        type: u4
      - id: line_width
        type: u4

  t0x35:
    doc: A t0x34 followed by a u16-counted list of 4-byte entries.
    seq:
      - id: base
        type: t0x34
      - id: num_entries
        type: u2
      - id: entries
        size: 4
        repeat: expr
        repeat-expr: num_entries

  drawn_instance:
    doc: |
      Structure type 12: a hierarchical block placed on a page.  This is the parent side
      of the hierarchy -- the rectangle the user sees, and the pins that the child
      schematic's ports connect to.

      The block's pin INTERFACE is an inline library_part (nested flag byte 24) whose pins
      give the names and electrical types, in order.  The pin POSITIONS are the type-16
      records at the end, in the same order, in absolute page coordinates.  The two lists
      are zipped to produce ORCAD_BLOCK_PIN.

      The reference designator does not follow the nested part contiguously: it starts at
      the second-to-last prefix stop offset, so the reader seeks there before reading it.

      The block rectangle is (x1, y1) plus the nested part's bounding-box size, in DBU
      (1 DBU = 10 mil).
    seq:
      - id: name_idx
        type: u4
        doc: String index; empty for a block.
      - id: source_lib_idx
        type: u4
      - id: name
        type: lzt
      - id: db_id
        type: u4
        doc: Matched against occurrence.target_db_id to find this block's child schematic.
      - id: anchor_y
        type: s2
      - id: anchor_x
        type: s2
      - id: bbox_y2
        type: s2
      - id: bbox_x2
        type: s2
      - id: x1
        type: s2
      - id: y1
        type: s2
      - id: color_and_orient
        size: 2
      - id: struct_id
        size: 2
      - id: display_props
        type: unparsed_structure_list
        doc: Framed type 39.
      - id: nested_flag
        contents: [24]
      - id: nested_part
        type: unparsed_structure_list
        doc: |
          Inline library_part (type 24) carrying the pin interface.  Its bounding box is
          the block rectangle's size.
      # The reader seeks to stops[size - 2] here before continuing.
      - id: reference
        type: lzt
      - id: unknown
        size: 14
      - id: pin_positions
        type: unparsed_structure_list
        doc: Framed type 16 records, absolute page positions, in nested_part pin order.

enums:

  structure_type:
    2:   sth_in_pages0
    4:   dsn_stream
    6:   part_cell
    9:   sch_lib
    10:  page
    11:  part_instance
    12:  drawn_instance
    13:  placed_instance
    16:  pin_inst_scalar
    17:  pin_inst_bus
    20:  wire_scalar
    21:  wire_bus
    23:  port
    24:  library_part
    26:  symbol_pin_scalar
    27:  symbol_pin_bus
    29:  bus_entry
    31:  package
    32:  device
    33:  global_symbol
    34:  port_symbol
    35:  offpage_symbol
    37:  placed_global
    38:  offpage_connector
    39:  symbol_display_prop
    48:  symbol_vector
    49:  alias
    52:  t0x34
    53:  t0x35
    55:  graphic_box_inst
    56:  graphic_line_inst
    57:  graphic_arc_inst
    58:  graphic_ellipse_inst
    59:  graphic_polygon_inst
    60:  graphic_polyline_inst
    61:  graphic_comment_text_inst
    62:  graphic_bitmap_inst
    64:  titleblock_symbol
    65:  titleblock
    66:  hierarchy_link
    75:  erc_symbol
    76:  bookmark_symbol
    77:  erc_object
    78:  bookmark_inst
    88:  graphic_bezier_inst
    89:  graphic_ole_inst
    98:  pin_shape_symbol
    103: net_group

  occurrence_type:
    0x42: part_or_block
    0x43: net
    0x44: pin_scalar
    0x45: pin_bus
    0x52: title_block
    0x5b: global_or_offpage
