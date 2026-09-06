# Parse an extracted OLE stream, not the complete DSN or OLB file.
# See ORCAD_V2_FORMAT.md for the container layout.

meta:
  id: orcad_dsn
  endian: le
  encoding: windows-1252

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
  Readers that cannot fully interpret a body seek to stop[0] instead; drawn_instance
  additionally uses the second-to-last stop to find its reference designator, and
  symbol_def uses the next stop above the cursor to find its bounding box.

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

  framed_structure:
    doc: |
      One structure at its type's long-prefix depth.  This table is authoritative, not
      descriptive: OrcadLongPrefixCount() in orcad_structures.cpp is exactly it, and a type
      absent from it is rejected rather than probed.

        1: 9; occurrence records 66, 67, 68, 69, 82, 91
        2: 10, 16, 17, 20, 21, 26, 27, 29, 32, 37, 38, 39, 48, 49,
           55..62, 88, 89
        3: 2, 6, 12, 23, 31, 65, 77
        4: 13, 33, 34, 35, 64, 75, 76
        5: 24

      Two contexts override the table.  A nested type-48 SymbolVector is `u8 48` followed by
      a one-long-prefix chain starting at a SECOND 48, and occurrence records (0x42, 0x43,
      0x44, 0x45, 0x52, 0x5b) always carry exactly one long prefix.  Both pass their count
      explicitly.  The body is opaque here because C++ dispatches on type_id.

      HOW MUCH OF THIS TABLE IS ACTUALLY VERIFIED.  11 of the 46 registered types -- types 6, 9, 10,
      24, 31, 33, 34, 35, 64, 75, 76 -- were measured against 868 modern corpus designs,
      117357 records, with zero mismatches.  The other 35 sit nested inside record bodies
      where a byte probe cannot reach them; they are exercised only indirectly, by the
      corpus importing to identical object counts.  A registered type that no corpus file
      contains is unverified, and type 98 is absent from the table for exactly that reason.

      Depth must be resolved LONGEST CHAIN FIRST.  A shorter chain frames successfully at
      the inner starting offsets of a longer one -- measuring type 24 shortest-first yields
      spurious matches at depths 0 through 4 alongside the real 5.  That ambiguity is what
      the old probing reader was resolving by luck, and it is why a fixed table is safer
      than a search that "validates" its guess.
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
    doc: Executable boundary grammar for a structure at a given prefix depth.
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
      Format version, the font table, the Design Template font assignments, the eight named
      part fields, the default page settings and -- most importantly -- the global string
      table that every property index in every other stream resolves against.

      version_major also selects the whole file's format family: >= 3 modern, < 3 legacy.

      The string-table count width follows that same split and nothing else: u16 when
      version_major is below 3, u32 otherwise.  The width is not sniffed and the other one
      is never tried.  A count that cannot fit in the bytes remaining is an error and the
      file is rejected, rather than the table being silently left empty -- an empty table
      resolves every property index everywhere to the empty string, which produces an
      import that looks like it worked and has lost every name.
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
      - id: create_timestamp
        type: u4
      - id: modify_timestamp
        type: u4
      - id: zeros
        size: 4
      - id: num_fonts_plus_one
        type: u2
      - id: fonts
        type: logfont
        repeat: expr
        repeat-expr: num_fonts_plus_one - 1
      - id: num_template_fonts
        type: u2
        if: version_major >= 2
        doc: Always 24 in observed files; version_major >= 2 only.
      - id: template_fonts_modern
        type: u2
        repeat: expr
        repeat-expr: num_template_fonts
        if: version_major >= 2
      - id: template_fonts_legacy
        type: u2
        repeat: expr
        repeat-expr: 17
        if: version_major < 2
        doc: |
          v1.x carries the same leading slots with no count field.  17 slots plus the
          8-byte tail below is the 42-byte block older notes called "legacy preferences".
      - id: template_tail
        size: 8
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
        doc: Global string table; every property index elsewhere addresses this.
      # A failure in this optional tail leaves aliases and the root name empty.
      - id: num_aliases
        type: u2
      - id: aliases
        type: part_alias
        repeat: expr
        repeat-expr: num_aliases
      - id: schematic_name_pad
        size: 8
        doc: Design files only; a standalone .OLB stops after the aliases.
      - id: schematic_name
        type: lzt
        doc: |
          Name of the root schematic FOLDER under /Views.  SCH_IO_ORCAD matches this
          case-insensitively against the folder names to pick the root; when it does not
          match, the first folder in visible order is used.
    instances:
      num_strings:
        value: 'version_major < 3 ? num_strings_legacy : num_strings_modern'

  logfont:
    doc: |
      A Win32 LOGFONTA record, 60 bytes.  Point size is approximately -height * 72 / 96;
      a zero width lets the font pick its own aspect ratio, and a non-zero one is what the
      importer reproduces by scaling glyph width.  Font indexes elsewhere are 1-based, with
      0 meaning default.
    seq:
      - id: height
        type: s4
        doc: lfHeight, negative device units.
      - id: width
        type: s4
        doc: lfWidth; 0 means natural aspect ratio.
      - id: escapement
        type: s4
        doc: lfEscapement, baseline direction in tenths of a degree.
      - id: orientation
        type: s4
        doc: lfOrientation, glyph direction in tenths of a degree.
      - id: weight
        type: s4
        doc: '>= 600 is rendered bold.'
      - id: italic
        type: u1
      - id: underline_strikeout_charset
        size: 3
      - id: out_clip_quality
        size: 3
      - id: pitch_and_family
        type: u1
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
      156 bytes, shared by the Library defaults and by every page_stream.  The grid
      reference fields describe the printed border ruling; the eight trailing flags are
      each a full u32 holding a boolean.
    seq:
      - id: create_timestamp
        type: u4
      - id: modify_timestamp
        type: u4
      - id: unknown_a
        size: 16
      - id: width
        type: u4
        doc: Mils, or micrometres when is_metric is set.
      - id: height
        type: u4
      - id: pin_to_pin
        type: u4
        doc: Grid pitch; 100 mils by default.
      - id: unknown_b
        size: 2
      - id: horizontal_count
        type: u2
        doc: Grid reference divisions along the top and bottom border.
      - id: vertical_count
        type: u2
      - id: unknown_c
        size: 2
      - id: horizontal_width
        type: u4
        doc: Border band width, same units as width/height.
      - id: vertical_width
        type: u4
      - id: unknown_d
        size: 48
      - id: horizontal_char
        type: u4
        doc: Non-zero labels the horizontal divisions with letters instead of numbers.
      - id: unknown_e
        size: 4
      - id: horizontal_ascending
        type: u4
      - id: vertical_char
        type: u4
      - id: unknown_f
        size: 4
      - id: vertical_ascending
        type: u4
      - id: is_metric
        type: u4
        doc: Non-zero selects metric units for width/height.
      - id: border_displayed
        type: u4
      - id: border_printed
        type: u4
      - id: grid_ref_displayed
        type: u4
      - id: grid_ref_printed
        type: u4
      - id: titleblock_displayed
        type: u4
      - id: titleblock_printed
        type: u4
      - id: ansi_grid_refs
        type: u4

  # -- /Cache, /Packages and /Symbols --------------------------------------------------

  cache_stream:
    doc: |
      Symbol and package definitions, in four counted sections.  This is the whole grammar --
      there is no entry-shape recognition and no terminal-entry special case.

      Section 0 holds loose symbols (types 33, 34, 35, 64, 75, 76), section 1 LibraryParts
      (type 24), section 2 PartCells (type 6), section 3 Packages (type 31).  A section
      carrying a type it does not own means the counts no longer describe the stream, so the
      walk stops rather than decoding a record at the wrong offset.

      An empty cache needs no special case: the marker plus four zero counts is exactly ten
      bytes, which is the ten-byte Cache stream seen in .OLB files.  Exactly four sections are
      consumed and trailing bytes are an error.

      A record whose BODY fails to decode is recoverable, because the record's own frame says
      where the next one starts; a failure of the framing itself is not, so the walk stops and
      warns.  Nothing below SCH_IO_ORCAD::LoadSchematicFile catches IO_ERROR, so a cache that
      cannot be framed must not throw out of the importer -- whatever was read is kept and the
      rest of the design falls back to synthesized placeholder symbols.

      Several entries may share a name.  The first is the definition the importer uses; later
      same-name entries are kept in stream order as variants, and a placement picks among them
      by matching its pin positions.

      Validated against all 21 modern Cache streams in the corpus, each ending exactly at EOF.
    seq:
      - id: marker
        contents: [0, 0]
      - id: sections
        type: cache_section
        repeat: expr
        repeat-expr: 4

  cache_section:
    seq:
      - id: num_groups
        type: u2
      - id: groups
        type: cache_group
        repeat: expr
        repeat-expr: num_groups

  cache_group:
    doc: One cached definition name, with every stored revision of it.
    seq:
      - id: name
        type: lzt
      - id: num_variants
        type: u2
      - id: variants
        type: cache_variant
        repeat: expr
        repeat-expr: num_variants

  cache_variant:
    doc: |
      One stored revision.  The two timestamps are independent fields; an older reader treated
      them as a database id written twice and required them equal, which silently assumed a
      cached symbol was never modified after it was created.
    seq:
      - id: source_library
        type: lzt
      - id: created
        type: u4
      - id: modified
        type: u4
      - id: entry_type
        type: u1
        enum: structure_type
        doc: Must be a type the enclosing section owns.
      - id: pad
        contents: [0]
      - id: record
        type: framed_record(1)
        doc: |
          Depth comes from OrcadLongPrefixCount(entry_type), which yields 4 long prefixes for
          a section-0 symbol, 5 for a type-24 LibraryPart, and 3 for a PartCell or Package.
          The body must end exactly at the record's outer stop.

  packages_stream:
    doc: |
      A locally modified part, one per /Packages child stream: u16 PartCell count, that many
      (part_cell, u16 LibraryPart count, that many LibraryParts), then one type-31 Package
      and EOF.  /Symbols children instead contain exactly one framed symbol and EOF.

      A legacy file always uses the legacy grammar here.  A modern file may still hold a
      legacy-framed /Packages stream, so isLongFramedPackageStream checks that the first
      record's four-byte zero pad sits at bytes 7..10 before choosing this grammar.
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
        doc: Body is part_cell_body; the reader seeks to the cell's stop before continuing.
      - id: num_library_parts
        type: u2
      - id: library_parts
        type: framed_record(5)
        repeat: expr
        repeat-expr: num_library_parts

  part_cell_body:
    doc: |
      Type 6.  The cell's short-prefix properties are inherited by every LibraryPart under
      it that does not define the same name.  The view names are read only to stay aligned;
      the reader then seeks to the cell's stop regardless.
    seq:
      - id: name
        type: lzt
      - id: source_lib
        type: lzt
      - id: num_views
        type: u2
      - id: views
        type: lzt
        repeat: expr
        repeat-expr: num_views

  symbol_def:
    doc: |
      Body of every decodable symbol type (24 LibraryPart, 33/34/35 global/port/off-page,
      64 titleblock, 75 ERC, 76 bookmark) and of the type-2 SthInPages0 body nested inside a
      graphic instance.  Type 98 is NOT in that set: it has no registered prefix depth, so it
      is neither framed nor decoded.

      EVERYTHING AFTER THE BOUNDING BOX IS CONDITIONAL.  A type-2 SthInPages0 ends there.
      The pin list, the property list and the general-properties tail are read only for the
      LibraryPart family, and the tail itself only for type 24.

      NOT fully sequential.  After the primitives the reader takes the next prefix stop
      ABOVE the cursor and reads the bounding box from the last 8 bytes before it, because
      the gap is 8 bytes in modern records and 16 in ones carrying a legacy trailer.  A box
      wider or taller than 4000 DBU is rejected as a mis-frame and dropped.

      Between two primitives there is sometimes an 8-byte zero filler.  It is detected, not
      counted, and the test is exact: the 8 bytes at the cursor are skipped only when all
      eight of them are zero.
    seq:
      - id: name
        type: lzt
        doc: Cache name, e.g. "C.Normal".
      - id: source_lib
        type: lzt
      - id: color
        type: u4
      - id: num_primitives
        type: u2
      - id: primitives
        type: primitive
        repeat: expr
        repeat-expr: num_primitives
      # The reader seeks to (next stop above the cursor) - 8 here.
      - id: bbox
        type: bbox_i2
        doc: Symbol-space body box; the block rectangle's size for a hierarchical block.
      - id: num_pins
        type: u2
        doc: Absent for a type-2 SthInPages0, which ends at the bounding box.
      - id: pins
        type: symbol_pin
        repeat: expr
        repeat-expr: num_pins
      - id: num_props
        type: u2
      - id: props
        type: framed_record(2)
        repeat: expr
        repeat-expr: num_props
      - id: general_properties
        type: general_properties
        doc: Type 24 only, and itself optional; see the type.

  general_properties:
    doc: |
      Tail of a type-24 LibraryPart.  Read speculatively: the four strings and the flag word
      are accepted only if they land exactly on the structure's stop, and otherwise the
      reader falls back to reading just the flags from the last two bytes before the stop.
    seq:
      - id: implementation_path
        type: lzt
      - id: implementation
        type: lzt
      - id: reference_prefix
        type: lzt
      - id: part_value
        type: lzt
      - id: flags
        type: u2
        doc: |
          bit0 pin names visible, bit1 pin text rotates with vertical pins,
          bit2 pin numbers hidden.

  bbox_i2:
    seq:
      - id: x1
        type: s2
      - id: y1
        type: s2
      - id: x2
        type: s2
      - id: y2
        type: s2

  primitive:
    doc: |
      A drawable inside a symbol_def.  The two leading type bytes must be equal and must
      name one of the kinds in primitive_type; a type-48 SymbolVector instead opens its own
      prefix chain and recurses (its children use a 3-byte type, 0x00, type prefix).

      The type selects one forward-only layout; the stored u32 byteLength is then CHECKED
      against what that layout consumed, never sniffed to decide the layout.  Three rules,
      by kind:

        * OLE image (90) -- inclusive, exactly.  Consumed size must equal byteLength, and
          the payload runs from the end of the header to start + byteLength.
        * Comment text (46) -- EXCLUSIVE IN EVERY ERA.  The record ends at
          start + byteLength + 8 and the reader seeks there, because the record keeps
          undecoded padding after the string.  Applying the general rule below to type 46
          instead loses the record: it cost 1228 text, 63 shape and 21 bitmap objects
          across 93 corpus designs before that was caught.
        * everything else -- fully decoded, so consumed size must equal byteLength or
          byteLength + 8.  The modern convention counts the u32 size word and its 4-byte
          pad; the legacy one does not.  Both are accepted; neither is guessed at.

      A stored size larger than the bytes available is an error.

      An OLE image's payload is not opaque bytes; see ole_payload below.

      Field layouts: rect/ellipse and line are i32 x1,y1,x2,y2 then u32 line_style,
      line_width (and fill_style, hatch_style for the closed kinds); arc adds i32 start and
      end points; polygon, polyline and bezier are u32 line_style, line_width, then
      fill_style and hatch_style for polygon only, then a u16 point count; comment text is
      i32 box, i32 text-bounds origin, u16 font index, 2 bytes, lzt text; bitmap is the i32
      box, 16 bytes of duplicate corner and pixel size, then a u32-counted payload; OLE
      image is the i32 box, 16 bytes of crop and original extent, then a compound-document
      payload filling the record.  Polygon points are stored Y FIRST.
    seq:
      - id: type_a
        type: u1
        enum: primitive_type
      - id: type_b
        type: u1
        enum: primitive_type
      - id: len_body
        type: u4
      - id: pad
        contents: [0, 0, 0, 0]
      - id: body
        size: len_body

  symbol_pin:
    doc: |
      Types 26 (scalar) and 27 (bus).  A single 0x00 byte where a prefix chain would start
      marks a skipped pin slot, which keeps the slot's index but contributes no pin; the
      surviving pins remember their slot, because a package device's pin-number list is
      indexed by it.

      The display-property list is optional and is read only when a type echo actually
      follows the port type with at least 6 bytes left before the stop.  Coordinates are
      symbol-space, Y-down.
    seq:
      - id: name
        type: lzt
      - id: start_x
        type: s4
        doc: Free end of the pin leg.
      - id: start_y
        type: s4
      - id: hotpt_x
        type: s4
        doc: Connection point; this is what a wire attaches to.
      - id: hotpt_y
        type: s4
      - id: shape_bits
        type: u2
        doc: bit1 clock, bit2 inverted dot, bit7 power style.
      - id: uninitialized
        size: 2
      - id: port_type
        type: u4
        enum: port_type
      - id: type_echo
        type: u1
        doc: Repeats the pin's own structure type; its absence ends the record.
      - id: reserved
        size: 3
      - id: num_display_props
        type: u2
      - id: display_props
        type: framed_record(2)
        repeat: expr
        repeat-expr: num_display_props

  package_body:
    doc: |
      Type 31.  Names the physical part: its reference prefix, its PCB footprint, and one
      device per package unit.  The short-prefix properties here are the part-level ones
      (Description, Tolerance, ...) that every placement inherits, since a placement stores
      only its own overrides.
    seq:
      - id: name
        type: lzt
      - id: source_lib
        type: lzt
      - id: ref_des
        type: lzt
      - id: unknown
        type: lzt
      - id: pcb_footprint
        type: lzt
      - id: num_devices
        type: u2
      - id: devices
        type: framed_record(2)
        repeat: expr
        repeat-expr: num_devices

  device_body:
    doc: |
      Type 32, one package unit.  pin_numbers is indexed by the owning symbol's pin SLOT,
      including slots that were skipped, so it must not be compacted.  An FF FF marker in
      place of a length is an empty slot; otherwise the number is followed by a config byte
      whose bit 7 marks the pin as ignored.
    seq:
      - id: unit_ref
        type: lzt
        doc: 'Unit suffix, e.g. "A".'
      - id: ref_des
        type: lzt
      - id: num_pins
        type: u2
      - id: pin_slots
        type: device_pin_slot
        repeat: expr
        repeat-expr: num_pins

  device_pin_slot:
    doc: |
      One slot.  FF FF where a length would be marks an empty slot, which still occupies its
      index -- the list is addressed by the owning symbol's pin slot number, including slots
      the symbol skipped, so it must never be compacted.
    instances:
      marker:
        pos: _io.pos
        type: u2
    seq:
      - id: empty
        contents: [0xFF, 0xFF]
        if: marker == 0xFFFF
      - id: number
        type: lzt
        if: marker != 0xFFFF
      - id: config
        type: u1
        if: marker != 0xFFFF
        doc: Bit 7 marks the pin as ignored.

  # -- /Views Directory ----------------------------------------------------------------

  views_directory:
    doc: |
      Which schematic folders Capture shows, in display order.  A folder present under
      /Views but absent here is hidden, and the importer keeps it as an unreferenced page
      set.  The stream must end exactly after the last record; trailing bytes are rejected
      rather than tolerated, since a mis-framed record would otherwise silently truncate the
      folder list.  If it cannot be read at all the folders fall back to sorted name order.
    seq:
      - id: unknown
        size: 4
      - id: num_folders
        type: u2
      - id: folders
        type: views_directory_entry
        repeat: expr
        repeat-expr: num_folders

  views_directory_entry:
    seq:
      - id: name
        type: lzt
      - id: kind
        contents: [9, 0]
        doc: Always structure type 9 (SchLib).
      - id: unknown
        size: 20

  # -- /Views/<folder>/Schematic -------------------------------------------------------

  schematic_stream:
    doc: |
      One schematic folder's page display order.  The importer intersects these names
      with the streams that actually exist under Pages/, appends any page the order
      stream omits, and falls back to name order when this stream cannot be read.
      Bytes after page_names belong to other Capture folder data and are not a second
      page-order grammar.
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
    doc: Legacy form; identical body behind a short prefix.  Selected by library version.
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
      flat page list, and what supplies per-occurrence reference designators, unit
      suffixes, properties and net names for designs that leave the placed instance's own
      field as the "C?" template.

      A scope is one point in an instantiation path.  Its block occurrences carry a child
      folder name and their own nested scope, so a schematic reused N times yields N
      independent designator sets.

      Legacy pre-preamble streams do not start with a 0x42 long prefix.  Those are
      instance-annotated and OrcadReadOccurrenceTree() returns an empty tree.  A modern
      tree that fails to parse warns and disables hierarchy reconstruction for the whole
      design; no signature scan follows.
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
      - id: num_power_entries
        type: u2
      - id: power_entries
        type: occ_named
        repeat: expr
        repeat-expr: num_power_entries
        doc: Header type 0x44.  Root table, present only here.
      - id: root_scope
        type: occ_scope

  occ_header:
    doc: |
      Occurrence records repeat the framing of a normal structure but always with exactly
      one long prefix.  The type byte identifies what the record is; the reader checks it
      against the type the position demands and fails the whole Hierarchy stream on
      mismatch, rather than resynchronizing.
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
      The occurrences visible at one point in an instantiation path.  Title-block and global
      occurrences are read only to keep the stream aligned; the importer keeps the net names
      and the part and block occurrences.
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
    doc: |
      Header type 0x43.  The name is the effective net name along this instantiation path,
      which is what makes a net reused under several block occurrences resolve differently
      per occurrence.
    seq:
      - id: header
        type: occ_header
      - id: db_id
        type: u4
      - id: name
        type: lzt

  occ_named:
    doc: Root power-table entry, header type 0x44.
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

      For a part occurrence the header's short-prefix properties are the per-occurrence
      property overrides, and they are what a CIS variant later rewrites.
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
      - id: unit_ref_idx
        type: u4
        doc: |
          String index naming this occurrence's package unit, e.g. "A".  Zero or an
          out-of-range index means the occurrence does not override the unit.
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

  # Structure type 66 (0x42) is the occurrence record; it also supplies child-folder links.

  # -- /Views/<folder>/Pages/<page> -----------------------------------------------------

  page_stream:
    doc: |
      One schematic page.  The whole stream is a single framed structure of type 10, and
      the fields below are its BODY: seek past the prefix chain and preamble first.  The
      type-10 short-prefix properties are the page's own properties.

      The body is a run of u16-counted lists.  The list order is not documented anywhere
      else; it is the order OrcadParsePage() walks, and getting it wrong is the usual way a
      page fails to load:

        1.  title blocks         framed type 65, each followed by 12 bytes
        2.  net records          raw, NOT prefix-framed (see net_record)
        3.  net group records    raw, NOT prefix-framed (see net_group_record)
        4.  net table            lzt name + u32 db id per entry (see net_entry)
        5.  wires                framed types 20 (scalar), 21 (bus)
        6.  placed parts         framed type 13 (part) and type 12 (block) in one list
        7.  ports                framed type 23, each followed by 9 bytes
        8.  globals              framed type 37, each followed by 5 bytes
        9.  off-page connectors  framed type 38, each followed by 5 bytes
        10. ERC objects          framed type 77, each followed by three lzt strings
        11. bus entries          framed type 29
        12. graphic instances    framed types 55..62, 88, 89

      The legacy page grammar walks the SAME twelve lists in the same order; only the
      framing and the index widths differ.  See page_stream_v2.

      The net table is authoritative for net names: wires carry net db ids that key it, and
      the importer computes junctions from it rather than from the wire geometry.  One db
      id may appear more than once, and every name recorded for it is kept -- the last wins
      as the net's name, the others become aliases.

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
        doc: The 12-byte trailer is the tail of this record's own body.
        repeat: expr
        repeat-expr: num_title_blocks
      - id: num_net_records
        type: u2
      - id: net_records
        type: net_record
        repeat: expr
        repeat-expr: num_net_records
      - id: num_net_group_records
        type: u2
      - id: net_group_records
        type: net_group_record
        repeat: expr
        repeat-expr: num_net_group_records
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
        doc: The 9-byte trailer is the tail of this record's own body.
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
      u16 count followed by that many framed structures.  Callers supply the prefix depth
      the list position expects.
    seq:
      - id: count
        type: u2

  net_entry:
    seq:
      - id: name
        type: lzt
      - id: db_id
        type: u4

  net_record:
    doc: |
      NOT prefix-framed, unlike everything else in the page.  One entry per net on the page,
      naming it and carrying its drawing style.  In the legacy grammar these same fields
      appear as a bare u32 id and lzt name with no leading pad.
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

  net_group_record:
    doc: |
      A net_record followed by the net ids it groups: this is how a bus names the scalar
      nets it carries.
    seq:
      - id: base
        type: net_record
      - id: num_members
        type: u2
      - id: members
        type: u4
        repeat: expr
        repeat-expr: num_members

  display_prop:
    doc: |
      Type 39.  How one property of the owning instance is drawn.  Absent means the
      property is not displayed at all, so the list membership is itself the visibility
      flag.  The trailing byte must be zero; a non-zero one means the record was mis-framed.
    seq:
      - id: name_idx
        type: u4
      - id: x
        type: s2
      - id: y
        type: s2
      - id: rot_font
        type: u2
        doc: bits 0..13 are the 1-based font index, bits 14..15 quarter turns.
      - id: color
        type: u1
      - id: disp_mode
        type: u2
      - id: terminator
        contents: [0]

  alias_body:
    doc: Type 49, a net alias label attached to a wire.
    seq:
      - id: x
        type: s4
      - id: y
        type: s4
      - id: color
        type: u4
      - id: rotation
        type: u4
        doc: Quarter turns in bits 0..1.
      - id: font_idx
        type: u4
      - id: name
        type: lzt

  wire_body:
    doc: |
      Types 20 (scalar) and 21 (bus).  net_db_id keys the page net table, and that -- not
      the geometry -- is what the importer uses to name the net and to place junctions.
    seq:
      - id: db_id
        type: u4
      - id: net_db_id
        type: u4
      - id: color
        type: u4
      - id: x1
        type: s4
      - id: y1
        type: s4
      - id: x2
        type: s4
      - id: y2
        type: s4
      - id: unknown
        size: 1
      - id: num_aliases
        type: u2
      - id: aliases
        type: framed_record(2)
        repeat: expr
        repeat-expr: num_aliases
      - id: num_props
        type: u2
      - id: props
        type: framed_record(2)
        repeat: expr
        repeat-expr: num_props
      - id: line_width
        type: u4
      - id: line_style
        type: u4

  bus_entry_body:
    doc: Type 29.
    seq:
      - id: color
        type: u4
      - id: x1
        type: s4
      - id: y1
        type: s4
      - id: x2
        type: s4
      - id: y2
        type: s4
      - id: reserved
        size: 8

  placed_instance:
    doc: |
      Type 13: a part placed on a page.  The bounding box is stored Y FIRST and includes
      the displayed text, so it is not the symbol body box.

      pkg_name is the cache key ("C.Normal"); source_package plus unit_index is the
      package-level identity, and unit_index is the zero-based device within that package.
      The reference and value stored here may be the unedited "C?" template -- an
      occurrence-annotated design carries the real ones in the Hierarchy stream instead.
    seq:
      - id: unknown_a
        type: u4
      - id: source_lib_idx
        type: u4
        doc: String index of the library this instance came from.
      - id: pkg_name
        type: lzt
      - id: db_id
        type: u4
        doc: Matched against occurrence.target_db_id to find this part's occurrences.
      - id: bbox_y1
        type: s2
      - id: bbox_x1
        type: s2
      - id: bbox_y2
        type: s2
      - id: bbox_x2
        type: s2
      - id: x
        type: s2
      - id: y
        type: s2
      - id: color
        type: u1
      - id: orientation
        type: u1
        doc: bits 0..1 quarter turns, bit 2 mirror.
      - id: part_index
        type: u1
      - id: part_byte
        type: u1
      - id: num_display_props
        type: u2
      - id: display_props
        type: framed_record(2)
        repeat: expr
        repeat-expr: num_display_props
      - id: unknown_b
        size: 1
      - id: reference
        type: lzt
      - id: value_idx
        type: u4
        doc: String index of the Part Value.
      - id: unknown_c
        size: 10
      - id: num_pins
        type: u2
      - id: pins
        type: framed_record(2)
        repeat: expr
        repeat-expr: num_pins
        doc: Type 16/17 pin_inst records.
      - id: source_package
        type: lzt
      - id: unit_index
        type: u2

  pin_inst:
    doc: |
      Types 16 (scalar) and 17 (bus): one pin of a placed instance, carrying the pin's
      absolute page position -- the point wires attach to.

      pin_index is SIGNED and a negative value marks the pin as explicitly no-connected;
      that is where a KiCad no-connect flag comes from, not from the ERC object list.

      The record does not end after the display properties: the remaining body bytes are
      padding and the reader seeks to the structure's stop.
    seq:
      - id: pin_index
        type: s2
        doc: Negative means no-connect.
      - id: x
        type: s2
      - id: y
        type: s2
      - id: word_a
        type: u4
      - id: word_b
        type: u4
      - id: num_display_props
        type: u2
      - id: display_props
        type: framed_record(2)
        repeat: expr
        repeat-expr: num_display_props

  graphic_inst:
    doc: |
      Types 37 (placed global), 38 (off-page connector), 55..62, 88, 89 (free graphics), 23
      (port), 65 (title block) and 77 (ERC object) all share this body; the list the record
      appeared in, plus its trailer, is what distinguishes them.

      The anchor and bounding box are INTERLEAVED, y before x and the box corners out of
      order: y, x, y2, x2, x1, y1.

      A 0x02 flag introduces one nested type-2 SthInPages0 symbol body holding the drawable
      primitives.  Its absence means the instance draws nothing of its own.
    seq:
      - id: name_idx
        type: u4
        doc: Logical net or port name; the port's real name, as opposed to its cache name.
      - id: source_lib_idx
        type: u4
      - id: name
        type: lzt
        doc: Cache symbol name.
      - id: db_id
        type: u4
      - id: y
        type: s2
      - id: x
        type: s2
      - id: bbox_y2
        type: s2
      - id: bbox_x2
        type: s2
      - id: bbox_x1
        type: s2
      - id: bbox_y1
        type: s2
      - id: color
        type: u1
      - id: orientation
        type: u1
        doc: bits 0..1 quarter turns, bit 2 mirror.
      - id: struct_id
        size: 2
      - id: num_display_props
        type: u2
      - id: display_props
        type: framed_record(2)
        repeat: expr
        repeat-expr: num_display_props
      - id: nested_flag
        type: u1
        doc: 0x02 introduces one nested SthInPages0; anything else ends the record.
      - id: nested
        type: framed_record(3)
        if: nested_flag == 2

  drawn_instance:
    doc: |
      Structure type 12: a hierarchical block placed on a page.  This is the parent side
      of the hierarchy -- the rectangle the user sees, and the pins that the child
      schematic's ports connect to.

      The block's pin INTERFACE is an inline library_part (nested flag byte 24) whose pins
      give the names and electrical types, in order.  The pin POSITIONS are the type-16
      records at the end, in the same order, in absolute page coordinates.  The two lists
      are zipped to produce ORCAD_BLOCK_PIN, and a negative pin_index there makes the block
      pin a no-connect.  When every type-16 record reports the same point -- which happens
      on blocks Capture never re-laid-out -- the nested part's own hot points are used
      instead, offset from the block rectangle.

      The reference designator does not follow the nested part contiguously: it starts at
      the second-to-last prefix stop offset, so the reader seeks there before reading it.

      The block rectangle is (x1, y1) plus the nested part's bounding-box size, in DBU
      (1 DBU = 10 mil), transposed when the orientation is an odd quarter turn.
    seq:
      - id: name_idx
        type: u4
        doc: |
          String index of the block's intrinsic Name property.  Unlike a part, a block
          uses it -- flat-net scoping keys off it.
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
        type: framed_record(5)
        doc: |
          One inline type-24 LibraryPart carrying the pin interface, read immediately after
          the flag byte.  No count precedes it.  Its bounding box is the block rectangle's
          size.
      # The reader seeks to stops[size - 2] here before continuing.
      - id: reference
        type: lzt
      - id: unknown
        size: 14
      - id: pin_positions
        type: unparsed_structure_list
        doc: Framed type 16 records, absolute page positions, in nested_part pin order.

  ole_payload:
    doc: |
      The payload of a type-90 OLE image primitive.  A 26-byte OrCAD prologue, then a whole
      Microsoft Compound File holding the embedded object's OLE 2.0 storage.

      The prologue states the compound file's length TWICE, which is what makes the offset
      readable rather than searchable: at offset 0 as the length plus 22, and at offset 22 as
      the length itself.  The compound file begins at offset 26.  Verified on every embedded
      object in the corpus -- 110 of 110, no exceptions.

      This is NOT a Microsoft OLE 1.0 ObjectHeader.  MS-OLEDS requires FormatID to be 1 or 2;
      the word here is 0x00000100, and there are no ClassName, TopicName or ItemName strings
      anywhere in the prologue.  The embedded object's class name lives in the compound file's
      own \1CompObj stream.

      Two traps.  The byte 0x16 that appears 26 bytes before the magic is not a tag; it is the
      low byte of length_plus_22, and only reads 0x16 when the length is a multiple of 256
      (103 of 110).  And cfb_length is not always sector aligned: 7 of 110 fall 63 to 428 bytes
      short of a 512 boundary because Capture drops the unused tail of the final sector.  They
      still parse.  Never round the length up.
    seq:
      - id: length_plus_22
        type: u4
        doc: Must equal cfb_length + 22.
      - id: unknown_a
        contents: [0x00, 0x01, 0x00, 0x00]
      - id: index
        type: u2
        doc: Small varying index, 1..0x25.
      - id: unknown_b
        size: 12
      - id: cfb_length
        type: u4
      - id: compound_file
        size: cfb_length
        doc: A complete MS-CFB file, opening with D0 CF 11 E0 A1 B1 1A E1.

  ci_image_payload:
    doc: |
      The other embedded-raster shape, used when the payload carries no compound file.  A
      preview raster, then the marker "~~CI_IMAGE~~", then the authoritative full-resolution
      image behind a counted length.

      The length is ASCII, not binary: a zero byte, a one-byte digit count, then that many
      decimal digits.  Reading it is what replaced hunting for a PNG, JPEG or BMP signature
      and taking everything to the end of the payload, which dragged any trailing bytes along
      with the image.  Every occurrence in the corpus conforms.
    seq:
      - id: preview
        size-eos: true
        doc: |
          Preview raster, then the ASCII marker "~~CI_IMAGE~~".  Its extent is not modelled
          here; the reader locates the marker within the payload.
      - id: zero
        contents: [0]
      - id: num_digits
        type: u1
      - id: digits
        type: str
        size: num_digits
        doc: Decimal byte count of the image that follows.
      - id: image
        size: digits.to_i

  # Versions below 3 use short prefixes and u16 string indices.
  # Legacy records have no stop offsets, so a framing failure ends the stream.

  short_prefix_v2:
    doc: Legacy framing; the whole prefix, with u16 property indices.
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

  page_stream_v2:
    doc: |
      The legacy page.  Same twelve lists in the same order as page_stream; only lists 2 and
      3 look different, and only because the modern file stores those same two tables in the
      raw un-framed form of net_record and net_group_record.
    seq:
      - id: prefix
        type: short_prefix_v2
      - id: name
        type: lzt
      - id: page_size
        type: lzt
      - id: settings
        type: page_settings
      - id: num_title_blocks
        type: u2
      - id: title_blocks
        type: graphic_inst_v2_with_trailer(12)
        repeat: expr
        repeat-expr: num_title_blocks
      - id: num_net_records
        type: u2
      - id: net_records
        type: net_record_v2
        repeat: expr
        repeat-expr: num_net_records
      - id: num_net_group_records
        type: u2
      - id: net_group_records
        type: net_group_record_v2
        repeat: expr
        repeat-expr: num_net_group_records
      - id: num_nets
        type: u2
      - id: nets
        type: net_entry
        repeat: expr
        repeat-expr: num_nets
      - id: num_wires
        type: u2
      - id: wires
        type: wire_v2
        repeat: expr
        repeat-expr: num_wires
      - id: num_placed
        type: u2
      - id: placed
        type: placed_record_v2
        repeat: expr
        repeat-expr: num_placed
      - id: num_ports
        type: u2
      - id: ports
        type: graphic_inst_v2_with_trailer(9)
        repeat: expr
        repeat-expr: num_ports
      - id: num_globals
        type: u2
      - id: globals
        type: graphic_inst_v2_with_trailer(5)
        repeat: expr
        repeat-expr: num_globals
      - id: num_offpage
        type: u2
      - id: offpage
        type: graphic_inst_v2_with_trailer(5)
        repeat: expr
        repeat-expr: num_offpage
      - id: num_erc
        type: u2
      - id: erc
        type: erc_record_v2
        repeat: expr
        repeat-expr: num_erc
      - id: num_bus_entries
        type: u2
      - id: bus_entries
        type: bus_entry_v2
        repeat: expr
        repeat-expr: num_bus_entries
      - id: num_graphics
        type: u2
      - id: graphics
        type: graphic_inst_v2
        repeat: expr
        repeat-expr: num_graphics

  net_record_v2:
    doc: The legacy form of net_record, without the 9-byte pad and the style words.
    seq:
      - id: id
        type: u4
      - id: name
        type: lzt

  net_group_record_v2:
    seq:
      - id: id
        type: u4
      - id: name
        type: lzt
      - id: num_members
        type: u2
      - id: members
        type: u4
        repeat: expr
        repeat-expr: num_members

  display_prop_v2:
    doc: |
      Two body widths exist.  The long one matches display_prop from color onward; the short
      one, seen only in the oldest streams, is 10 bytes ending in a byte of font size and a
      byte of colour, with no display mode and no terminator.  The width is selected once
      for the whole file by library_stream.version_major < 2 and passed down to every
      reader.  The stream is never re-parsed under the other width.
    seq:
      - id: prefix
        type: short_prefix_v2
      - id: name_idx
        type: u2
      - id: x
        type: s2
      - id: y
        type: s2
      - id: rot_font
        type: u2
      - id: color
        type: u1
      - id: disp_mode
        type: u2
      - id: terminator
        contents: [0]

  display_prop_list_v2:
    seq:
      - id: count
        type: u2
      - id: props
        type: display_prop_v2
        repeat: expr
        repeat-expr: count

  alias_v2:
    seq:
      - id: prefix
        type: short_prefix_v2
      - id: x
        type: s4
      - id: y
        type: s4
      - id: color
        type: u4
      - id: rotation
        type: u4
      - id: font_idx
        type: u4
      - id: name
        type: lzt

  wire_v2:
    seq:
      - id: prefix
        type: short_prefix_v2
      - id: db_id
        type: u4
      - id: net_db_id
        type: u4
      - id: color
        type: u4
      - id: x1
        type: s4
      - id: y1
        type: s4
      - id: x2
        type: s4
      - id: y2
        type: s4
      - id: unknown
        size: 1
      - id: num_aliases
        type: u2
      - id: aliases
        type: alias_v2
        repeat: expr
        repeat-expr: num_aliases
      - id: num_props
        type: u2
      - id: props
        type: framed_structure_v2
        repeat: expr
        repeat-expr: num_props
        doc: Legacy wire property records, consumed in full.

  framed_structure_v2:
    doc: A legacy structure whose body this schema does not model; see v2Structure.
    seq:
      - id: prefix
        type: short_prefix_v2

  pin_inst_v2:
    seq:
      - id: prefix
        type: short_prefix_v2
      - id: pin_index
        type: s2
        doc: Negative means no-connect.
      - id: x
        type: s2
      - id: y
        type: s2
      - id: word_a
        type: u4
      - id: word_b
        type: u4
      - id: display_props
        type: display_prop_list_v2

  placed_record_v2:
    doc: |
      Type 12 and type 13 share this list, told apart by peeking the type byte the legacy
      prefix opens with.
    seq:
      - id: record
        type: 'next_type == 12 ? drawn_instance_v2 : placed_instance_v2'
    instances:
      next_type:
        pos: _io.pos
        type: u1

  placed_instance_v2:
    doc: |
      Legacy type 13.  Same field order as placed_instance, but the header word pair is u16
      rather than u32, the Part Value is a u16 string index, and the block of unknown bytes
      after it is 6 rather than 10.
    seq:
      - id: prefix
        type: short_prefix_v2
      - id: unknown_a
        type: u2
      - id: source_lib_idx
        type: u2
      - id: unknown_b
        size: 4
      - id: pkg_name
        type: lzt
      - id: db_id
        type: u4
      - id: bbox_y1
        type: s2
      - id: bbox_x1
        type: s2
      - id: bbox_y2
        type: s2
      - id: bbox_x2
        type: s2
      - id: x
        type: s2
      - id: y
        type: s2
      - id: color
        type: u1
      - id: orientation
        type: u1
      - id: part_index
        type: u1
      - id: part_byte
        type: u1
      - id: display_props
        type: display_prop_list_v2
      - id: unknown_c
        size: 1
      - id: reference
        type: lzt
      - id: value_idx
        type: u2
      - id: unknown_d
        size: 6
      - id: num_pins
        type: u2
      - id: pins
        type: pin_inst_v2
        repeat: expr
        repeat-expr: num_pins
      - id: source_package
        type: lzt
      - id: unit_index
        type: u2

  drawn_instance_v2:
    doc: |
      Legacy type 12.  Same shape as the modern one but with u16 string indices, no prefix
      stops, and no inner 0x42 marker, so it reads straight through.  The reference and the
      trailing indices follow the nested part contiguously here, rather than at a stop offset.
    seq:
      - id: prefix
        type: short_prefix_v2
      - id: name_idx
        type: u2
      - id: source_library_idx
        type: u2
      - id: unknown_a
        size: 4
      - id: name
        type: lzt
      - id: db_id
        type: u4
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
      - id: color
        type: u1
      - id: orientation
        type: u1
      - id: struct_id
        size: 2
      - id: display_props
        type: display_prop_list_v2
      - id: nested_flag
        contents: [24]
      - id: nested_part
        type: library_part_v2
        doc: One inline legacy LibraryPart carrying the pin interface; no count precedes it.
      - id: unknown_b
        size: 14
      - id: reference
        type: lzt
      - id: value_idx
        type: u2
      - id: unknown_c
        type: u2
      - id: implementation_idx
        type: u2
      - id: unknown_d
        type: u2
      - id: num_pins
        type: u2
      - id: pins
        type: pin_inst_v2
        repeat: expr
        repeat-expr: num_pins

  library_part_v2:
    doc: |
      A legacy LibraryPart body, opaque here.  Its production lives in v2LibSymbolDef and
      v2LibraryPartTail (orcad_page.cpp) and differs from the modern symbol_def in more than
      framing, so it is not the modern body behind a short prefix.
    seq:
      - id: prefix
        type: short_prefix_v2

  graphic_inst_v2:
    doc: |
      Legacy types 23, 37, 38, 55..62, 65, 77.  Same field order as the modern graphic_inst,
      with u16 string indices.  The anchor and box are interleaved the same way, y before x.
    seq:
      - id: prefix
        type: short_prefix_v2
      - id: name_idx
        type: u2
      - id: source_library_idx
        type: u2
      - id: unknown_a
        size: 4
      - id: name
        type: lzt
      - id: db_id
        type: u4
      - id: y
        type: s2
      - id: x
        type: s2
      - id: bbox_y2
        type: s2
      - id: bbox_x2
        type: s2
      - id: bbox_x1
        type: s2
      - id: bbox_y1
        type: s2
      - id: color
        type: u1
      - id: orientation
        type: u1
      - id: struct_id
        size: 2
      - id: display_props
        type: display_prop_list_v2
      - id: nested_flag
        type: u1
        doc: 0x02 introduces one nested legacy symbol definition; anything else ends the record.
      - id: nested
        type: symbol_def_v2
        if: nested_flag == 2

  symbol_def_v2:
    doc: |
      Legacy nested symbol body, opaque here.  Its production is v2SymbolDef in orcad_page.cpp;
      legacy primitives drop the doubled type byte and the byteLength envelope, so the modern
      primitive type does not describe them.
    seq:
      - id: prefix
        type: short_prefix_v2

  graphic_inst_v2_with_trailer:
    params:
      - id: len_trailer
        type: s4
    seq:
      - id: record
        type: graphic_inst_v2
      - id: trailer
        size: len_trailer

  erc_record_v2:
    seq:
      - id: record
        type: graphic_inst_v2
      - id: trailer
        type: lzt
        repeat: expr
        repeat-expr: 3

  bus_entry_v2:
    seq:
      - id: prefix
        type: short_prefix_v2
      - id: body
        type: bus_entry_body

  occurrence_tree_v2:
    doc: |
      The legacy Hierarchy stream, when it has one.  Opens with the view name and 5 bytes
      rather than the modern 7, and the whole stream must be consumed exactly; trailing
      bytes are rejected.
    seq:
      - id: view_name
        type: lzt
      - id: unknown
        size: 5
      - id: num_power_entries
        type: u2
      - id: power_entries
        type: occ_named_v2
        repeat: expr
        repeat-expr: num_power_entries
      - id: root_scope
        type: occ_scope_v2

  occ_named_v2:
    doc: Root power-table entry, prefix type 0x44.
    seq:
      - id: prefix
        type: short_prefix_v2
      - id: db_id
        type: u4
      - id: name
        type: lzt

  occ_scope_v2:
    doc: |
      The legacy scope.  Shorter than the modern one: there is no global/off-page table and no
      optional separator preamble, and the net and title-block tables are the whole preamble to
      the occurrences.
    seq:
      - id: num_nets
        type: u2
      - id: nets
        type: occ_net_v2
        repeat: expr
        repeat-expr: num_nets
      - id: num_title_blocks
        type: u2
      - id: title_blocks
        type: occ_pair_v2
        repeat: expr
        repeat-expr: num_title_blocks
      - id: num_occurrences
        type: u2
      - id: occurrences
        type: occurrence_v2
        repeat: expr
        repeat-expr: num_occurrences

  occ_net_v2:
    doc: Prefix type 0x43.
    seq:
      - id: prefix
        type: short_prefix_v2
      - id: db_id
        type: u4
      - id: name
        type: lzt

  occ_pair_v2:
    doc: Title-block occurrence, prefix type 0x52; contents unused.
    seq:
      - id: prefix
        type: short_prefix_v2
      - id: db_id
        type: u4
      - id: unknown
        type: u4

  occurrence_v2:
    doc: |
      One part or block occurrence, prefix type 0x42.  Two differences from the modern
      occurrence beyond framing: there is no inner 0x42 marker, and the unit reference is a
      u16 string index rather than a u32.  A non-empty child folder still marks a block.
    seq:
      - id: prefix
        type: short_prefix_v2
      - id: own_db_id
        type: u4
      - id: target_db_id
        type: u4
      - id: child_folder
        type: lzt
      - id: reference
        type: lzt
      - id: unit_ref_idx
        type: u2
      - id: num_pins
        type: u2
      - id: pins
        type: occ_pin_v2
        repeat: expr
        repeat-expr: num_pins
      - id: nested
        type: occ_scope_v2

  occ_pin_v2:
    doc: Prefix type 0x44 for a scalar pin occurrence, 0x45 for a bus pin.
    seq:
      - id: prefix
        type: short_prefix_v2
      - id: db_id
        type: u4
      - id: pin_index
        type: u2

  # CIS variants override occurrence properties. See the CIS types below for the stream layouts.

  cis_counted_list:
    doc: |
      /CIS/VariantStore/BOM/BOMDataStream (the variant names) and each variant's own
      definition stream (the property groups it selects).  The first field is the item count
      in decimal, and it must match the number of items that follow.
    seq:
      - id: len_payload
        type: u4
        doc: Must equal the remaining stream length exactly.
      - id: payload
        size-eos: true
        doc: '0xF9-separated; first field is the decimal count of the rest.'

  cis_property_updates:
    doc: |
      A group's UpdateStorageGroupDataStream or UpdateStorageSubGroupDataStream.  Records are
      '~'-separated; each is a decimal occurrence id, 0xB0, '^'-separated property names,
      0xC0, '^'-separated values.  The two lists must be the same length and no name may be
      empty.
    seq:
      - id: len_payload
        type: u4
      - id: payload
        size-eos: true

  cis_memberships:
    doc: |
      Which occurrences belong to a group: '~'-separated records of one '0' or '1' byte,
      0xB0, then the decimal occurrence id.
    seq:
      - id: len_payload
        type: u4
      - id: payload
        size-eos: true

  cis_schematic_info_stream:
    doc: |
      /Views/<folder>/CISSchematic/SchematicInfoStorage/<name>.  Unlike the streams above
      this one is binary and length-prefixed throughout, and it must end exactly at the last
      record.  Each record binds one database id to a set of groups, each group carrying its
      own property overrides; the group and state lists and the property-set count must all
      have the same length.
    seq:
      - id: num_records
        type: u4
      - id: records
        type: cis_schematic_record
        repeat: expr
        repeat-expr: num_records

  cis_schematic_record:
    seq:
      - id: database_id
        type: u4
      - id: groups
        type: cis_string_list
      - id: states
        type: cis_string_list
      - id: num_property_sets
        type: u4
        doc: Must equal the group count.
      - id: property_sets
        type: cis_property_set
        repeat: expr
        repeat-expr: num_property_sets
      - id: trailer
        type: u4

  cis_property_set:
    seq:
      - id: names
        type: cis_string_list
      - id: values
        type: cis_string_list

  cis_string_list:
    seq:
      - id: count
        type: u4
      - id: items
        type: lzt
        repeat: expr
        repeat-expr: count

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
    52:  net_record
    53:  net_group_record
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
    66:  hierarchy_link_is_occurrence_0x42
    67:  occurrence_67
    68:  occurrence_68
    69:  occurrence_69
    75:  erc_symbol
    76:  bookmark_symbol
    77:  erc_object
    78:  bookmark_inst
    82:  occurrence_82
    88:  graphic_bezier_inst
    89:  graphic_ole_inst
    91:  occurrence_91
    98:  pin_shape_symbol_unregistered
    103: net_group

  primitive_type:
    40: rect
    41: line
    42: arc
    43: ellipse
    44: polygon
    45: polyline
    46: comment_text
    47: bitmap
    48: symbol_vector
    87: bezier
    90: ole_image

  port_type:
    0: input
    1: bidirectional
    2: output
    3: open_collector
    4: passive
    5: tri_state
    6: open_emitter
    7: power_in

  occurrence_type:
    0x42: part_or_block
    0x43: net
    0x44: pin_scalar
    0x45: pin_bus
    0x52: title_block
    0x5b: global_or_offpage
