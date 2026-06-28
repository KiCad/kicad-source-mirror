# Kaitai Struct definition for the PADS PowerPCB binary `.pcb` format.
#
# Coordinate convention: absolute geometry is RAW = design + per-axis origin.
# The origin (i32 x,y) is stored at section[1] +60/+64; design_x = raw_x -
# origin_x and design_y = raw_y - origin_y, and the two axes may differ. Some
# sections store DESIGN coordinates directly (per-type docs note which). Widths,
# sizes, drills, grids and angles are not origin-shifted.
#
# Units: BASIC = 1/38100 mil. Angles are stored as degrees * 1,800,000.
#
# The directory entry count is STORED (directory entry 1's count, less one), not
# implied by the version: v0x2021, v0x2022 and v0x2024 have 73 entries and only
# v0x2025 onward have 74. Section payloads are laid out contiguously in directory
# order, but the accumulated offset must be corrected by the section-3
# over-declaration -- see `dir_entry`. The file ends with the footer GUID and a
# u32 back-pointer to the serialized container-item array.

meta:
  id: pads_pcb_binary
  title: PADS PowerPCB binary layout (.pcb)
  file-extension: pcb
  endian: le
  encoding: ASCII
  imports:
    - microsoft_cfb

doc: The purpose of this field is not known.
params:
  - id: section41_num_clearance_records
    type: u4
  - id: section41_clearance_record_size
    type: u4
  - id: section41_num_layer_clearance_records
    type: u4
  - id: section41_num_rule_relation_prefixes
    type: u4
  - id: section41_num_high_speed_records
    type: u4
  - id: section41_num_per_layer_rule_matrices
    type: u4
  - id: section41_num_layers
    type: u4
  - id: section41_num_route_records
    type: u4
  - id: section41_route_record_size
    type: u4
  - id: section41_num_diff_pair_records
    type: u4
  - id: section41_num_section49_prefix_words
    type: u4
  - id: section49_storage_offset
    type: u4
    doc: validated physical start of section 49 after any rule-stream overhang
  - id: section49_physical_offset
    type: u4
    doc: physical start of section 49 before any section-41 rule-stream overhang
  - id: string_pool_offset
    type: u4
    doc: validated section-57 pool start from its terminating index entry
  - id: section52_uses_legacy_tokens
    type: u1
  - id: section65_66_num_saved_relationship_links
    type: u4
  - id: section65_66_num_compact_net_classes
    type: u4
  - id: section65_66_num_net_classes
    type: u4
  - id: layer_table_offset
    type: u4
    doc: validated section-69 layer-record base
  - id: post_layer_offset
    type: u4
    doc: validated start of the post-layer database stream
  - id: num_extended_layer_states
    type: u4
  - id: preferences_base_size
    type: u4
  - id: num_error_conflicts
    type: u4
  - id: num_font_faces
    type: u4
  - id: font_stride
    type: u4

# These parameters are outputs of the bounded structural locators documented
# above. The file does not serialize the rule-array counts or physical late-
# stream offsets; explicit inputs avoid byte searches and corpus-fitted constants.
seq:
  - id: magic
    contents: [0x00, 0xff]
    doc: file magic 00 FF
  - id: version
    type: u2
    doc: DB format version 0x2017 / 0x2019 / 0x2021 / 0x2022 / 0x2024 / 0x2025 / 0x2026 / 0x2027
  - id: subversion
    type: u2
    doc: |
      Format subversion. Values 0, 1, 2 and 3 occur in the corpus. The value is
      independent of the directory-entry count: v0x2017 uses 2/3, v0x2019
      through v0x2026 use 0, and v0x2027 uses 0/1/2.
  - id: header_padding
    contents: [0, 0, 0, 0]
    doc: four zero padding bytes; zero on 90 of 90 unique corpus files
  - id: directory
    type: dir_entry
    repeat: expr
    repeat-expr: 'num_directory'
    doc: |
      entry_count x 16-byte directory entries from offset 10. Each section's
      payload begins at HEADER(10) + entry_count*16 + sum(prior total_bytes),
      MINUS the section-3 over-declaration (see `payload_offset` below).
      sec0 has no region.

instances:
  num_directory:
    value: '(_root.directory_probe.count - 1)'
    doc: |
      The entry count is STORED, not implied by the version. Directory entry 1
      describes the section table itself -- stride 16, one slot per controller --
      and declares one slot more than is written, the same in-memory-versus-
      written over-declaration section 3 makes. So the written count is its
      declared count less one.

      This makes v0x2022 and v0x2024 73-entry files, NOT 74: the extra controller
      arrived in v0x2025. Verified on all 90 scoped files by *PCB* board-setup
      block alignment -- with the stored count every file lands on a 48-byte
      boundary, where a 73/74 version table leaves v0x2022 and v0x2024 off by 32.

      Guard before trusting it: section1.total_bytes == section1.count * 16.
      That guard holds on 90 of 90 scoped corpus boards, so the
      directory always declares its own byte size and the count can be read
      without a version branch at all.

      Re-measured on the scoped corpus, the stored count by version is

          0x2021   73    17 boards
          0x2022   73     5 boards
          0x2024   73     2 boards
          0x2025   74     1 board
          0x2026   74    12 boards
          0x2027   74    53 boards

      NOT YET APPLIED IN THE READER, AND RE-CONFIRMED AS BLOCKED.
      PADS_SDB::directoryEntryCount() still returns the version-implied 73/74.
      Switching it moves every v0x2022 and v0x2024 data_offset by 16 bytes at
      once, and the 81 call sites still on data_offset are calibrated against
      the wrong value, so it can only land together with moving them to
      payload_offset -- where the offset and the section-3 overshoot both change
      by 16 and cancel. The origin locator below reads the stored count directly
      instead of waiting for that migration.

      Tried again on 2026-08-06, reading the stored count with the self-check
      total_bytes == slots*16 as a guard. Still exactly 9 failures, still only on
      those two dialects, so the blocker is live and the migration really is a
      prerequisite rather than a stale note. The failing cases, for whoever picks
      this up:

          V2022MechanicalDecalsMatch          checked 0 against 8
          V2022RouteNetIndexBiasDecodes       GND and PGND via nets empty
          V2024RouteGeometryMatchesAscii      geometry mismatch
          V2022PadNetsMatchAscii              net set mismatch
          V2022PadGeometryMatchesAscii        pad set mismatch
          V2024InlineTerminalsAndPadstackPairsDecode   fiducials 0 against 3
          V2024ZeroPadstackOverridesDecode    fatal, WE-SL1 pin 2

      THIS IS THE HIGHEST-LEVERAGE REMAINING WORK. An audit attributes 19 of the
      41 parser search sites to one defect -- the declared data_offset of
      sections 5, 8, 9, 10, 12, 14, 15 does not match where their records start,
      which three separate code comments already note. Those 19 retire together
      once the lead-in model is right, the way deriving payload_offset dissolved
      four fitted constants at once. Retiring locators one at a time is mostly
      wasted motion by comparison.
  directory_probe:
    pos: 26
    type: dir_entry
    doc: directory entry 1, read early because it carries the entry count
  footer:
    pos: '_io.size - 42'
    type: footer
    doc: final 42-byte MFC document footer, anchored from EOF
  container_items:
    pos: footer.cntr_item_back_ptr
    type: cntr_item_array
    doc: root serialized OLE container-item array, reached by the footer back-pointer
  view_state_records:
    pos: '10 + num_directory * 16'
    type: sec2_view_state_array
    size: directory[2].total_bytes
    doc: section 2 records are physically written before section 1
  board_setup:
    pos: '10 + num_directory * 16 + directory[2].total_bytes'
    type: sec1_board_setup
    size: directory[1].total_bytes
  section3_serialized_size:
    value: 'directory[3].total_bytes - (num_directory * 16 + 48)'
    doc: section 3's directory size is its in-memory image; the directory and 48-byte database header are not serialized again
  board_parameters:
    pos: '10 + num_directory * 16 + directory[2].total_bytes + directory[1].total_bytes'
    type: sec3_board_params
    size: section3_serialized_size
    doc: section 3 omits the already-written directory and 48-byte database header from its image
  section4_offset:
    value: '10 + num_directory * 16 + directory[2].total_bytes + directory[1].total_bytes + section3_serialized_size'
  padstack_definitions:
    pos: section4_offset
    type: sec4_padstack_array
    size: directory[4].total_bytes
  pad_layer_shapes:
    pos: 'section4_offset + directory[4].total_bytes'
    type: sec5_pad_layer_array
    size: directory[5].total_bytes
  text_objects:
    pos: 'section4_offset + directory[4].total_bytes + directory[5].total_bytes + directory[6].total_bytes + directory[7].total_bytes'
    type: sec8_text_array
    size: directory[8].total_bytes
  text_and_drawing_bridge:
    pos: 'section4_offset + directory[4].total_bytes + directory[5].total_bytes + directory[6].total_bytes + directory[7].total_bytes + directory[8].total_bytes'
    type: sec9
    size: directory[9].total_bytes
  drawing_objects:
    pos: 'section4_offset + directory[4].total_bytes + directory[5].total_bytes + directory[6].total_bytes + directory[7].total_bytes + directory[8].total_bytes + directory[9].total_bytes'
    type: sec10_drawing_array
    size: directory[10].total_bytes
  graphic_piece_headers:
    pos: 'section4_offset + directory[4].total_bytes + directory[5].total_bytes + directory[6].total_bytes + directory[7].total_bytes + directory[8].total_bytes + directory[9].total_bytes + directory[10].total_bytes'
    type: sec11_piece_array
    size: directory[11].total_bytes
  graphic_vertices:
    pos: 'section4_offset + directory[4].total_bytes + directory[5].total_bytes + directory[6].total_bytes + directory[7].total_bytes + directory[8].total_bytes + directory[9].total_bytes + directory[10].total_bytes + directory[11].total_bytes'
    type: sec12_vertex_array
    size: directory[12].total_bytes
  section13_offset:
    value: 'section4_offset + directory[4].total_bytes + directory[5].total_bytes + directory[6].total_bytes + directory[7].total_bytes + directory[8].total_bytes + directory[9].total_bytes + directory[10].total_bytes + directory[11].total_bytes + directory[12].total_bytes'
  hatch_segments:
    pos: section13_offset
    type: sec13_hatch_array
    size: directory[13].total_bytes
    if: directory[13].count > 0
    doc: complete 20-byte copper-pour hatch segment array
  section14_offset:
    value: 'section13_offset + directory[13].total_bytes'
  decal_terminal_descriptors:
    pos: section14_offset
    type: sec14_terminal_descriptor_array
    size: directory[14].total_bytes
    doc: complete PARTDECAL terminal-run descriptor array
  section15_offset:
    value: 'section14_offset + directory[14].total_bytes'
  section15_logical_offset:
    value: 'section15_offset + (version <= 0x2019 ? 60 : num_directory * 16 + 48)'
    doc: legacy records follow a 60-byte rotated descriptor tail; modern records follow the serialized database-header image omitted from section 3
  decal_terminal_slots:
    pos: section15_logical_offset
    type: decal_terminal_slot_array
    size: directory[15].total_bytes
    doc: section 15 terminal records followed by mixed decal controller and object-dictionary storage units
  section16_offset:
    value: 'section15_offset + directory[15].total_bytes'
  parttype_aux_records:
    pos: section16_offset
    type: parttype_aux_array
    size: directory[16].total_bytes
    doc: eight-byte PARTTYPE auxiliary index/state records
  section17_offset:
    value: 'section16_offset + directory[16].total_bytes'
  parttypes:
    pos: section17_offset
    type: parttype_record_array
    size: directory[17].total_bytes
    doc: complete PARTTYPE definition array in declaration order
  section18_offset:
    value: 'section13_offset + directory[13].total_bytes + directory[14].total_bytes + directory[15].total_bytes + directory[16].total_bytes + directory[17].total_bytes'
  parttype_final_metadata:
    pos: section18_offset
    type: parttype_final_metadata
    doc: final rotated PARTTYPE metadata, ending 44 bytes after the nominal section-18 boundary
  parttype_gates:
    pos: 'section18_offset + 44'
    type: parttype_gate_array
    size: directory[18].total_bytes
    doc: gate definitions begin after the final PARTTYPE metadata and end 44 bytes into nominal section 19
  section19_offset:
    value: 'section18_offset + directory[18].total_bytes'
  parttype_pins:
    pos: 'section19_offset + 44'
    type: parttype_pin_array
    size: directory[19].total_bytes
    if: directory[19].count > 0
    doc: PARTTYPE pin records are phase-shifted 44 bytes past the nominal section-19 boundary
  section20_offset:
    value: 'section19_offset + directory[19].total_bytes'
  section20_logical_offset:
    value: 'section20_offset + (version <= 0x2019 ? 48 : 44)'
    doc: legacy SIGPIN records have one additional rotated four-byte word before their first pin ordinal
  parttype_signal_pins:
    pos: section20_logical_offset
    type: parttype_sigpin_array
    size: directory[20].total_bytes
    if: directory[20].count > 0
    doc: ASCII SIGPIN pin-to-signal mappings attached to PARTTYPE definitions
  section21_offset:
    value: 'section20_offset + directory[20].total_bytes'
  compact_parttype_pin_names:
    pos: 'section21_offset + 44'
    type: compact_parttype_pin_name_array
    size: directory[21].total_bytes
    if: directory[21].count > 0
    doc: compact pin-name lists following PARTTYPE declarations
  section22_offset:
    value: 'section19_offset + directory[19].total_bytes + directory[20].total_bytes + directory[21].total_bytes'
  part_placements:
    pos: section22_offset
    type: part_placement_array
    size: directory[22].total_bytes
    if: directory[22].count > 0
    doc: complete placed-part array; directory count equals the live placement count
  section23_offset:
    value: 'section22_offset + directory[22].total_bytes'
  nets:
    pos: section23_offset
    type: net_record_array
    size: directory[23].total_bytes
    if: directory[23].count > 0
    doc: named nets followed by the unassigned-obstacles sentinel where present
  section24_offset:
    value: 'section23_offset + directory[23].total_bytes'
  route_chains:
    pos: section24_offset
    type: route_chain_array
    size: directory[24].total_bytes
    if: directory[24].count > 0
    doc: one 68-byte topology record per net connection
  section25_offset:
    value: 'section24_offset + directory[24].total_bytes'
  route_allocator_controller:
    pos: section25_offset
    type: route_allocator_controller
    size: 'directory[25].total_bytes + 44'
    doc: route-object allocator/controller state; its final 44 bytes occupy the nominal section-26 prefix
  section26_offset:
    value: 'section25_offset + directory[25].total_bytes'
  route_object_ranges:
    pos: 'section26_offset + 44'
    type: route_object_range_array
    size: directory[26].total_bytes
    doc: allocator ranges begin after the rotated 44-byte controller tail
  section27_offset:
    value: 'section26_offset + directory[26].total_bytes'
  route_layer_object_counts:
    pos: 'section27_offset + 44'
    type: route_layer_object_count_array
    size: directory[27].total_bytes
    doc: per-copper-layer route-object counts; their sum is the section-29 handle count
  section29_offset:
    value: 'section27_offset + directory[27].total_bytes + directory[28].total_bytes'
  route_object_handles:
    pos: 'section29_offset + 44'
    type: route_object_handle_array
    size: directory[29].total_bytes
    doc: route-object handles grouped by the preceding per-layer counts
  section41_offset:
    value: 'section29_offset + 44 + directory[29].total_bytes'
    doc: sections 30 through 40 are empty on all 90 corpus files; the design-rule controller follows the rotated section-29 handle vector directly
  design_rule_stream:
    pos: section41_offset
    type: 'section41_design_rule_stream(section41_num_clearance_records, section41_clearance_record_size, section41_num_layer_clearance_records, section41_num_rule_relation_prefixes, section41_num_high_speed_records, section41_num_per_layer_rule_matrices, section41_num_layers, section41_num_route_records, section41_route_record_size, section41_num_diff_pair_records, section41_num_section49_prefix_words)'
    doc: complete section-41 controller and typed rule arrays
  section49_storage:
    pos: section49_storage_offset
    type: section49_storage_array
    size: 'section49_physical_offset + directory[49].total_bytes - section49_storage_offset'
  section50_relationships:
    pos: 'section49_physical_offset + directory[49].total_bytes'
    type: section50_relationship_array
    size: directory[50].total_bytes
  section51_relationships:
    pos: 'section49_physical_offset + directory[49].total_bytes + directory[50].total_bytes'
    type: section51_relationship_array
    size: directory[51].total_bytes
  sections52_55_legacy_tokens:
    pos: 'section49_physical_offset + directory[49].total_bytes + directory[50].total_bytes + directory[51].total_bytes'
    type: legacy_objrel_token_array
    size: 'directory[52].total_bytes + directory[53].total_bytes + directory[54].total_bytes + directory[55].total_bytes'
    if: section52_uses_legacy_tokens != 0
  section52_outline_owners:
    pos: 'section49_physical_offset + directory[49].total_bytes + directory[50].total_bytes + directory[51].total_bytes'
    type: sec52_outline_owner_array
    size: directory[52].total_bytes
    if: section52_uses_legacy_tokens == 0
  section53_outline_pieces:
    pos: 'section49_physical_offset + directory[49].total_bytes + directory[50].total_bytes + directory[51].total_bytes + directory[52].total_bytes'
    type: sec53_outline_piece_array
    size: directory[53].total_bytes
    if: section52_uses_legacy_tokens == 0
  section54_outline_vertices:
    pos: 'section49_physical_offset + directory[49].total_bytes + directory[50].total_bytes + directory[51].total_bytes + directory[52].total_bytes + directory[53].total_bytes'
    type: sec54_outline_vertex_array
    size: directory[54].total_bytes
    if: section52_uses_legacy_tokens == 0
  section55_outline_arcs:
    pos: 'section49_physical_offset + directory[49].total_bytes + directory[50].total_bytes + directory[51].total_bytes + directory[52].total_bytes + directory[53].total_bytes + directory[54].total_bytes'
    type: sec55_outline_arc_array
    size: directory[55].total_bytes
    if: section52_uses_legacy_tokens == 0
  string_index:
    pos: 'string_pool_offset - directory[56].count * 16'
    type: string_index_array
    size: 'directory[56].count * 16'
  string_pool:
    pos: string_pool_offset
    type: string_pool_contents
    size: directory[57].total_bytes
  section60_record_stride:
    value: 'directory[60].total_bytes / directory[60].count'
  late_route_overlap:
    value: 'section60_record_stride - 32'
  section59_physical_offset:
    value: 'string_pool_offset + directory[57].total_bytes - late_route_overlap'
  sections59_64:
    pos: section59_physical_offset
    type: 'sections59_64_stream(directory[59].count, directory[59].total_bytes / directory[59].count, directory[60].count, section60_record_stride, directory[61].count, directory[62].count, directory[62].total_bytes / directory[62].count, directory[63].count, directory[64].count)'
  section65_66_physical_offset:
    value: 'section59_physical_offset + directory[59].total_bytes + directory[60].total_bytes + directory[61].total_bytes + directory[62].total_bytes + directory[63].total_bytes + directory[64].total_bytes'
  section65_66_archive:
    pos: section65_66_physical_offset
    type: 'section65_66_archive(late_route_overlap, section65_66_num_saved_relationship_links, section65_66_num_compact_net_classes, section65_66_num_net_classes)'
  section67_physical_offset:
    value: 'layer_table_offset - 12 - directory[68].total_bytes - directory[67].total_bytes'
  section67_relationships:
    pos: section67_physical_offset
    type: sec67_design_rule_relationship_array
    size: directory[67].total_bytes
  section68_clusters:
    pos: 'layer_table_offset - 12 - directory[68].total_bytes'
    type: cluster_record_array
    size: directory[68].total_bytes
  section69_controller_leadin:
    pos: 'layer_table_offset - 12'
    type: section69_controller_leadin
  section69_layers:
    pos: layer_table_offset
    type: 'sec69_layer_record_array(version <= 0x2021 ? 6 : version == 0x2022 ? 8 : 12)'
    size: directory[69].total_bytes
  post_layer_database:
    pos: post_layer_offset
    type: 'post_layer_database_stream(num_extended_layer_states, preferences_base_size, num_error_conflicts, num_font_faces, font_stride)'

types:

  # =========================================================================
  # CONTAINER
  # =========================================================================
  post_layer_database_stream:
    params:
      - id: num_extended_layer_states
        type: u4
      - id: preferences_base_size
        type: u4
      - id: num_error_conflicts
        type: u4
      - id: num_font_faces
        type: u4
      - id: font_stride
        type: u4
    doc: |
      Complete located post-layer controller stream through the final Strings
      data pages. Optional 276-byte extended ODBLayer records precede the flat
      section-70/71/72/73 state, followed by the nested PowerSYS
      database, six Reuse controllers, eleven Attribute controllers, two direct
      geometry lists, and the Strings header/data allocators. Controller order,
      IDs, and fixed-object strides come directly from the sdb500 reader.
    seq:
      - id: extended_layer_states
        type: extended_layer_state_record
        repeat: expr
        repeat-expr: num_extended_layer_states
      - id: layer_controller_state
        type: section70_serialized_layer_state
      - id: display_preferences
        type: global_display_preferences(preferences_base_size)
        size: preferences_base_size - 4
      - id: error_conflicts
        type: saved_error_conflict_record
        repeat: expr
        repeat-expr: num_error_conflicts
      - id: font_faces
        type:
          switch-on: font_stride
          cases:
            40: saved_font_face_record_v40
            52: saved_font_face_record
        repeat: expr
        repeat-expr: num_font_faces
      - id: database_header
        type: embedded_database_header
      - id: reuse_entity
        type: legacy_fixhdr_controller(0, 52)
      - id: reuse_entity_component
        type: legacy_fixhdr_controller(1, 48)
      - id: reuse_entity_signal
        type: legacy_fixhdr_controller(2, 48)
      - id: reuse
        type: legacy_fixhdr_controller(3, 56)
      - id: reuse_component
        type: legacy_fixhdr_controller(4, 48)
      - id: reuse_signal
        type: legacy_fixhdr_controller(5, 48)
      - id: attribute_type
        type: legacy_fixhdr_controller(100, 68)
      - id: attribute_type_bool
        type: legacy_fixhdr_controller(101, 28)
      - id: attribute_type_int
        type: legacy_fixhdr_controller(102, 28)
      - id: attribute_type_double
        type: legacy_fixhdr_controller(103, 28)
      - id: attribute_type_quantity
        type: legacy_fixhdr_controller(104, 40)
      - id: attribute_unit
        type: legacy_fixhdr_controller(105, 52)
      - id: attribute_type_list
        type: legacy_fixhdr_controller(106, 36)
      - id: attribute_text_item
        type: legacy_fixhdr_controller(107, 48)
      - id: attribute_type_text
        type: legacy_fixhdr_controller(108, 28)
      - id: attribute_value
        type: legacy_fixhdr_controller(109, 68)
      - id: attribute_inheritance
        type: legacy_fixhdr_controller(110, 36)
      - id: post_controller_counts
        type: embedded_database_post_controller_counts
      - id: misc_geometry
        type: legacy_misc_geometry_controller
      - id: strings
        type: legacy_string_controller

  global_display_preferences:
    params:
      - id: base_size
        type: u4
    doc: The purpose of this field is not known.
    seq:
      - id: display_flags
        type: u4
        doc: packed global visibility and editor-option bits
      - id: primary_display_color_index
        type: u4
        doc: initialized to palette index 15 by the global-preference initializer
      - id: secondary_display_color_index
        type: u4
        doc: initialized to palette index 14 by the global-preference initializer
      - id: display_mode
        type: u4
      - id: editor_display_state
        size: 44
        doc: persisted editor and stroke-font display state
      - id: saved_first_layer_handle
        type: u4
        doc: saved reference used to restore the first physical layer
      - id: saved_last_layer_handle
        type: u4
        doc: saved reference used to restore the final physical layer
      - id: retained_layer_reference_state
        size: 8
        doc: retained layer-selection reference state
      - id: current_viewport_rectangle
        type: rect_i32
        doc: viewport captured from DrawArea::GetVP immediately before serialization
      - id: code_page
        type: u4
        doc: Windows character-set/code-page state; initialized from GetACP
      - id: configuration_count
        type: u4
        valid:
          max: 10
      - id: display_configurations
        type: 'global_display_configuration((base_size - 108) / 10)'
        repeat: expr
        repeat-expr: 10
      - id: default_font_face_handle
        type: u4
        doc: tagged ODBFontFace handle; 0x49000000 on 87 scoped files and zero on three

  section70_serialized_layer_state:
    doc: |
      Section 70 / flat-loader tag 0x46. The directory declares one 16-byte
      in-memory object, but the scoped files serialize only its leading state
      word. The following byte is GlobalPrefObj.display_flags, proving there are
      no twelve padding bytes in the file.
    seq:
      - id: layer_state
        type: u4

  global_display_configuration:
    params:
      - id: record_stride
        type: u4
    doc: Fixed-capacity named global display/viewport configuration slot
    seq:
      - id: name_storage
        type: strz
        size: 84
        encoding: ASCII
      - id: viewport_rectangle
        type: rect_i32
      - id: configuration_flags
        type: u4
        if: record_stride == 104

  extended_layer_state_record:
    doc: |
      Optional 276-byte ODBLayer extension serialized before section 70. Three
      scoped files contain 37 records. The earlier `saved_view` interpretation
      was false: tag ordering places these records before the global preference
      object, and their tail holds saved layer handles plus a fixed layer name.
      The 204-byte prefix contains eleven display-controller words, three exact
      P_STDBFontFace-shaped font slots, and font-selection state.
    seq:
      - id: display_controller_state
        type: u4
        repeat: expr
        repeat-expr: 11
      - id: text_font_faces
        type: saved_font_face_record
        repeat: expr
        repeat-expr: 3
      - id: font_selection_state
        type: u4
      - id: layer_flags
        type: u4
      - id: saved_runtime_layer_handle
        type: u4
      - id: saved_link_handle
        type: u4
      - id: saved_layer_object_handle
        type: u4
      - id: layer_name_state
        type: u4
      - id: layer_name_storage
        type: strz
        size: 32
        encoding: ASCII
      - id: retained_layer_state
        type: u4
        repeat: expr
        repeat-expr: 5
        doc: retained ODBLayer object capacity; zero in all 37 scoped records, not alignment padding

  saved_font_face_record:
    doc: |
      Section-73 P_STDBFontFace record, exactly 52 bytes. The flat reader copies
      the twelve payload words after `saved_font_handle` into a 0x34-byte
      runtime font object and clears all but bit zero of `font_flags`. The two
      fixed string-storage regions may contain overwritten or retained suffixes
      when a face name exceeds its historical slot; those bytes are serialized
      object state, never padding.
    seq:
      - id: saved_font_handle
        type: u4
      - id: font_ordinal
        type: u4
      - id: face_name_storage
        size: 12
      - id: face_description_storage
        size: 20
      - id: font_flags
        type: u4
      - id: font_state
        type: u4
      - id: nominal_height
        type: u4

  saved_font_face_record_v40:
    doc: |
      v0x2019 section-73 predecessor. This older P_STDBFontFace save record is
      40 bytes: two identifying words followed by its fixed 32-byte face-name
      storage. No scoped file uses this dialect.
    seq:
      - id: saved_font_handle
        type: u4
      - id: font_ordinal
        type: u4
      - id: face_name_storage
        size: 32

  saved_error_conflict_record:
    doc: |
      Section-72 P_ErrConf record, exactly 32 bytes. The flat reader copies the
      handle and following 24 bytes into a runtime error-conflict object. The
      final word remains part of the fixed saved record but is not copied into
      the runtime object's 0x20-byte core; it is retained record state, not
      padding, and is nonzero in corpus files.
    seq:
      - id: saved_conflict_handle
        type: u4
      - id: conflict_object_handle
        type: u4
      - id: conflicting_object_handle
        type: u4
      - id: error_code
        type: u4
      - id: conflict_flags
        type: u4
      - id: conflict_metric
        type: f8
      - id: retained_record_state
        type: u4

  legacy_string_controller:
    doc: |
      Complete DBC_StringCtl stream. The controller's primary pages contain
      fixed eight-byte DBD_StringHdr records. Its following data pages contain
      the capacity-framed DBD_StringData slots. This ownership and ordering are
      the direct behavior of DBC_StringCtl::Read/Write in sdb500.dll.
    seq:
      - id: title
        type: legacy_short_mfc_string
      - id: header_page_count
        type: u4
      - id: live_header_count
        type: u4
        if: header_page_count > 0
      - id: header_pages
        type: serialized_string_header_page
        repeat: expr
        repeat-expr: header_page_count
      - id: data_page_count
        type: u4
        if: header_page_count > 0
      - id: data_pages
        type: serialized_normal_page
        repeat: expr
        repeat-expr: data_page_count
        if: header_page_count > 0

  embedded_database_header:
    doc: |
      Header of the nested PowerSYS DBS_Database. The thirteen state words and
      byte directory are written by DBS_Database::WriteDB before its live
      controller streams. `controller_slot_count` is the byte-directory length.
    seq:
      - id: outer_controller_index
        type: u4
      - id: title
        type: legacy_short_mfc_string
      - id: database_version
        type: u2
      - id: storage_flags
        type: u2
      - id: database_state
        type: u4
        repeat: expr
        repeat-expr: 4
      - id: controller_slot_count
        type: u4
      - id: database_state_tail
        type: u4
        repeat: expr
        repeat-expr: 8
      - id: controller_directory
        type: u1
        repeat: expr
        repeat-expr: controller_slot_count

  embedded_database_post_controller_counts:
    doc: |
      Counts consumed by DBS_Database::ReadDB after its indexed object
      controllers and before the direct Misc and Strings controllers. These are
      real zero-valued counts in this corpus, not padding.
    seq:
      - id: multi_layer_count
        type: u4
      - id: geometry_primary_page_count
        type: u4
      - id: geometry_secondary_page_count
        type: u4

  legacy_misc_geometry_controller:
    doc: |
      Direct DBC_MiscGeomCtl stream. Its two independent page-list counts are
      written even when zero; the secondary count is zero on all corpus files.
    seq:
      - id: title
        type: legacy_short_mfc_string
      - id: primary_page_count
        type: u4
      - id: primary_pages
        type: 'legacy_allocator_page(0xffffffff)'
        repeat: expr
        repeat-expr: primary_page_count
      - id: secondary_page_count
        type: u4

  serialized_string_header_page:
    doc: Saved DBM_LayerPage containing fixed DBD_StringHdr records
    seq:
      - id: allocation_size
        type: s4
        doc: saved page allocation extent; -1 marks the compact header-page allocation
      - id: payload_length
        type: u4
        valid:
          max: 0xfff0
      - id: saved_page_base
        type: u4
        doc: original allocation base used for pointer relocation
      - id: headers
        type: string_header_page_contents
        size: payload_length

  string_header_page_contents:
    doc: Exactly tiled fixed-size DBD_StringHdr array
    seq:
      - id: headers
        type: saved_string_header_record
        repeat: eos

  saved_string_header_record:
    doc: Saved DBD_StringHdr linking a stable string handle to its DBD_StringData slot
    seq:
      - id: saved_string_data_handle
        type: u4
      - id: header_state
        type: u4

  serialized_normal_page_list:
    doc: |
      Counted tail list of normal 64 KiB database allocator pages. A list ends
      exactly at the container-item back-pointer on every one of the 90 unique
      corpus files. Its count word is immediately followed by `count` page
      frames. All pages except the final page carry 65,520 live bytes; the final
      page may be partially occupied.
    seq:
      - id: count
        type: u4
      - id: pages
        type: serialized_normal_page
        repeat: expr
        repeat-expr: count

  legacy_short_mfc_string:
    doc: MFC CArchive short CString encoding used by database-controller titles
    seq:
      - id: length
        type: u1
      - id: value
        type: str
        size: length
        encoding: ASCII

  legacy_database_controller_header:
    doc: Common controller identity and primary allocator-page count
    seq:
      - id: controller_index
        type: u4
      - id: title
        type: legacy_short_mfc_string
      - id: primary_page_count
        type: u4

  legacy_fixhdr_state:
    doc: |
      DBC_FixHdr saved-list state. The first three words identify the saved
      first/last handles and live object count. The remaining four words are
      intrusive-list and allocator state retained by the controller.
    seq:
      - id: saved_first_handle
        type: u4
      - id: saved_last_handle
        type: u4
      - id: live_record_count
        type: u4
      - id: list_state
        type: u4
        repeat: expr
        repeat-expr: 4

  legacy_fixhdr_controller:
    params:
      - id: controller_id
        type: u4
      - id: record_stride
        type: u4
    doc: |
      Serialized DBC_FixHdr controller. Empty controllers stop after the common
      header. Non-empty controllers carry fixed-record pages, three version
      words, then a counted list of variable-member allocator pages. The zeros
      following AttributeInheritance belong to the enclosing database counts.
    seq:
      - id: header
        type: legacy_database_controller_header
      - id: fixhdr_state
        type: legacy_fixhdr_state
        if: header.primary_page_count > 0
      - id: primary_pages
        type: legacy_fixed_object_page(controller_id, record_stride)
        repeat: expr
        repeat-expr: header.primary_page_count
      - id: version_state
        type: u4
        repeat: expr
        repeat-expr: 3
        if: header.primary_page_count > 0
      - id: secondary_page_count
        type: u4
        if: header.primary_page_count > 0
      - id: secondary_pages
        type: 'legacy_allocator_page(controller_id)'
        repeat: expr
        repeat-expr: secondary_page_count
        if: header.primary_page_count > 0

  legacy_fixed_object_page:
    params:
      - id: controller_id
        type: u4
      - id: record_stride
        type: u4
    doc: Saved 64 KiB DBC_FixHdr page framed by class, live length, and relocation base
    seq:
      - id: controller_type
        type: u4
        valid: controller_id
      - id: payload_length
        type: u4
        valid:
          max: 0xfff0
      - id: saved_page_base
        type: u4
        doc: original 64 KiB-aligned allocation base used for pointer relocation
      - id: payload
        type: 'legacy_fixed_object_page_payload(controller_id, record_stride)'
        size: payload_length

  legacy_fixed_object_page_payload:
    params:
      - id: controller_id
        type: u4
      - id: record_stride
        type: u4
    doc: Complete saved objects followed by any retained partial allocator slot
    seq:
      - id: records
        type:
          switch-on: controller_id
          cases:
            0: saved_reuse_entity_record
            1: saved_reuse_entity_component_record
            2: saved_reuse_entity_signal_record
            3: saved_reuse_record
            4: saved_reuse_component_record
            5: saved_reuse_signal_record
            100: saved_attribute_type_record
            101: saved_attribute_scalar_type_record
            102: saved_attribute_scalar_type_record
            103: saved_attribute_scalar_type_record
            104: saved_attribute_quantity_type_record
            105: saved_attribute_unit_record
            106: saved_attribute_list_type_record
            107: saved_attribute_text_item_record
            108: saved_attribute_scalar_type_record
            109: saved_attribute_value_record
            110: saved_attribute_inheritance_record
        repeat: expr
        repeat-expr: _io.size / record_stride
      - id: retained_partial_slot_values
        type: 'saved_partial_capacity_value(controller_id)'
        repeat: eos
        doc: occupied bytes of a final incomplete capacity slot; retained data, not padding

  saved_partial_capacity_value:
    params:
      - id: controller_id
        type: u4
    doc: one retained four-byte fragment of the controller's next fixed-record capacity slot
    seq:
      - id: retained_value
        type: u4

  saved_fixhdr_object_header:
    doc: |
      Common 24-byte intrusive-list and variable-member header of every saved
      fixed object. Across 1,028,672 scoped capacity records, every nonzero next
      and previous handle resolves to another record in the same controller and
      points back through the reciprocal field; both state words are zero.
    seq:
      - id: retained_record_flags
        type: u4
      - id: next_object_handle
        type: u4
      - id: previous_object_handle
        type: u4
      - id: saved_owner_state
        type: u4
      - id: variable_member_handle
        type: u4
      - id: variable_member_state
        type: u4

  saved_attribute_scalar_type_record:
    doc: 28-byte Bool/Int/Double/Text specialization linked to its base AttributeType
    seq:
      - id: header
        type: saved_fixhdr_object_header
      - id: base_attribute_type_handle
        type: u4

  saved_attribute_quantity_type_record:
    doc: 40-byte quantity specialization with its unit relationship
    seq:
      - id: header
        type: saved_fixhdr_object_header
      - id: base_attribute_type_handle
        type: u4
      - id: next_quantity_type_handle
        type: u4
      - id: previous_quantity_type_handle
        type: u4
      - id: attribute_unit_handle
        type: u4

  saved_attribute_unit_record:
    doc: 52-byte attribute-unit object with three string fields and quantity-type binding
    seq:
      - id: header
        type: saved_fixhdr_object_header
      - id: name_string_handle
        type: u4
      - id: name_state
        type: u4
      - id: abbreviation_string_handle
        type: u4
      - id: abbreviation_state
        type: u4
      - id: format_string_handle
        type: u4
      - id: quantity_type_handle
        type: u4
      - id: unit_ordinal
        type: u4

  saved_attribute_list_type_record:
    doc: 36-byte list specialization owning a counted AttributeTextItem list
    seq:
      - id: header
        type: saved_fixhdr_object_header
      - id: base_attribute_type_handle
        type: u4
      - id: first_text_item_handle
        type: u4
      - id: text_item_count
        type: u4

  saved_attribute_text_item_record:
    doc: 48-byte named list item with intrusive links, owner type, and optional value binding
    seq:
      - id: header
        type: saved_fixhdr_object_header
      - id: text_string_handle
        type: u4
      - id: next_text_item_handle
        type: u4
      - id: previous_text_item_handle
        type: u4
      - id: owner_list_type_handle
        type: u4
      - id: attribute_value_handle
        type: u4
      - id: item_ordinal_or_state
        type: u4

  saved_attribute_inheritance_record:
    doc: 36-byte inheritance edge between an owning value/object and an AttributeType
    seq:
      - id: header
        type: saved_fixhdr_object_header
      - id: next_inheritance_handle
        type: u4
      - id: previous_inheritance_handle
        type: u4
      - id: attribute_type_handle
        type: u4

  saved_attribute_type_record:
    doc: 68-byte base attribute definition with name and three counted relationship lists
    seq:
      - id: header
        type: saved_fixhdr_object_header
      - id: name_string_handle
        type: u4
      - id: next_type_handle
        type: u4
      - id: previous_type_handle
        type: u4
      - id: first_text_item_handle
        type: u4
      - id: last_text_item_handle
        type: u4
      - id: text_item_count
        type: u4
      - id: first_value_handle
        type: u4
      - id: value_count
        type: u4
      - id: first_inheritance_handle
        type: u4
      - id: inheritance_count
        type: u4
      - id: specialized_type_handle
        type: u4

  saved_attribute_value_record:
    doc: 68-byte attribute value with owner links, type binding, value links, and optional text item
    seq:
      - id: header
        type: saved_fixhdr_object_header
      - id: next_owner_value_handle
        type: u4
      - id: previous_owner_value_handle
        type: u4
      - id: owner_value_state_handle
        type: u4
      - id: first_child_value_handle
        type: u4
      - id: child_value_count
        type: u4
      - id: next_child_value_handle
        type: u4
      - id: previous_child_value_handle
        type: u4
      - id: attribute_type_handle
        type: u4
      - id: next_typed_value_handle
        type: u4
      - id: previous_typed_value_handle
        type: u4
      - id: text_item_handle
        type: u4

  saved_reuse_entity_record:
    doc: 52-byte reuse-entity root with component and signal collections
    seq:
      - id: header
        type: saved_fixhdr_object_header
      - id: name_string_handle
        type: u4
      - id: reuse_handle
        type: u4
      - id: entity_state
        type: u4
      - id: first_component_handle
        type: u4
      - id: component_count
        type: u4
      - id: first_signal_handle
        type: u4
      - id: signal_count
        type: u4

  saved_reuse_record:
    doc: 56-byte reuse definition with entity, component, and signal relationships
    seq:
      - id: header
        type: saved_fixhdr_object_header
      - id: name_string_handle
        type: u4
      - id: next_entity_handle
        type: u4
      - id: previous_entity_handle
        type: u4
      - id: owner_entity_handle
        type: u4
      - id: first_component_handle
        type: u4
      - id: component_count
        type: u4
      - id: first_signal_handle
        type: u4
      - id: signal_count
        type: u4

  saved_reuse_entity_component_record:
    doc: 48-byte component membership linking a reuse entity to a reuse component
    seq:
      - id: header
        type: saved_fixhdr_object_header
      - id: name_string_handle
        type: u4
      - id: next_component_handle
        type: u4
      - id: previous_component_handle
        type: u4
      - id: owner_reuse_handle
        type: u4
      - id: reuse_component_handle
        type: u4
      - id: membership_state
        type: u4

  saved_reuse_component_record:
    doc: 48-byte reuse component linked to its entity-component membership
    seq:
      - id: header
        type: saved_fixhdr_object_header
      - id: next_component_handle
        type: u4
      - id: previous_component_handle
        type: u4
      - id: owner_entity_handle
        type: u4
      - id: first_member_handle
        type: u4
      - id: last_member_handle
        type: u4
      - id: entity_component_handle
        type: u4

  saved_reuse_entity_signal_record:
    doc: 48-byte signal membership linking a reuse entity to a reuse signal
    seq:
      - id: header
        type: saved_fixhdr_object_header
      - id: name_string_handle
        type: u4
      - id: next_signal_handle
        type: u4
      - id: previous_signal_handle
        type: u4
      - id: owner_reuse_handle
        type: u4
      - id: reuse_signal_handle
        type: u4
      - id: membership_state
        type: u4

  saved_reuse_signal_record:
    doc: 48-byte reuse signal linked to its entity-signal membership
    seq:
      - id: header
        type: saved_fixhdr_object_header
      - id: next_signal_handle
        type: u4
      - id: previous_signal_handle
        type: u4
      - id: owner_entity_handle
        type: u4
      - id: first_member_handle
        type: u4
      - id: last_member_handle
        type: u4
      - id: entity_signal_handle
        type: u4

  legacy_allocator_page:
    params:
      - id: controller_id
        type: u4
    doc: Variable-member allocator page referenced by saved fixed objects
    seq:
      - id: logical_size
        type: u4
        doc: controller allocator extent; 0x2713 in the observed attribute streams
      - id: payload_length
        type: u4
        valid:
          max: 0xfff0
      - id: saved_page_base
        type: u4
        doc: original allocation base used for pointer relocation
      - id: payload
        type: 'legacy_allocator_page_payload(controller_id)'
        size: payload_length

  legacy_allocator_page_payload:
    params:
      - id: controller_id
        type: u4
    doc: |
      Controller-specific variable-member allocation blocks. Block size and
      field layout are exact for every page in the scoped corpus. Slots released
      before save retain allocator free-list links in their first field; that
      retained content is state, not padding.
    seq:
      - id: member_values
        type:
          switch-on: controller_id
          cases:
            0: saved_reuse_entity_variable_member
            1: saved_reuse_membership_variable_member
            2: saved_reuse_membership_variable_member
            3: saved_reuse_variable_member
            4: saved_reuse_membership_variable_member
            5: saved_reuse_membership_variable_member
            100: saved_attribute_type_variable_member
            101: saved_attribute_bool_variable_member
            102: saved_attribute_int_variable_member
            103: saved_attribute_double_variable_member
            104: saved_attribute_quantity_variable_member
            105: saved_attribute_unit_variable_member
            106: saved_attribute_list_variable_member
            107: saved_attribute_text_item_variable_member
            108: saved_attribute_text_variable_member
            109: saved_attribute_value_variable_member
            110: saved_attribute_inheritance_variable_member
        repeat: eos

  saved_modern_variable_member_owner:
    doc: 'Allocation backlink added in v0x2024: owning fixed-record ordinal and retained/free state'
    seq:
      - id: owning_fixed_record_ordinal
        type: u4
      - id: allocation_state
        type: u4

  saved_reuse_entity_variable_member:
    doc: 24-byte ReuseEntity variable block; six blocks tile each observed page
    seq:
      - id: retained_or_active_state
        type: u4
      - id: component_collection_handle
        type: u4
      - id: signal_collection_handle
        type: u4
      - id: entity_revision_or_free_link
        type: u4
      - id: allocation_state
        type: u4
      - id: object_kind
        type: u4

  saved_reuse_membership_variable_member:
    doc: Eight-byte collection membership state used by Reuse component and signal controllers
    seq:
      - id: membership_value_or_free_link
        type: u4
      - id: membership_state
        type: u4

  saved_reuse_variable_member:
    doc: 60-byte Reuse object variable block; eight blocks tile the largest observed page
    seq:
      - id: retained_or_active_state
        type: u4
      - id: entity_collection_handle
        type: u4
      - id: component_collection_handle
        type: u4
      - id: signal_collection_handle
        type: u4
      - id: relationship_state
        type: u4
      - id: allocation_state
        type: u4
      - id: object_kind
        type: u4
      - id: retained_relationship_words
        type: u4
        repeat: expr
        repeat-expr: 8

  saved_attribute_type_variable_member:
    doc: Base AttributeType flags and display/style code
    seq:
      - id: enabled_or_free_link
        type: u4
      - id: display_style
        type: u4
      - id: owner
        type: saved_modern_variable_member_owner
        if: _root.version >= 0x2024

  saved_attribute_bool_variable_member:
    doc: Boolean AttributeType default and state
    seq:
      - id: default_value_or_free_link
        type: u4
      - id: value_state
        type: u4
      - id: owner
        type: saved_modern_variable_member_owner
        if: _root.version >= 0x2024

  saved_attribute_int_variable_member:
    doc: Integer AttributeType default, minimum, and maximum
    seq:
      - id: default_value_or_free_link
        type: u4
      - id: minimum_value
        type: u4
      - id: maximum_value
        type: u4
      - id: modern_alignment_state
        type: u4
        if: _root.version >= 0x2024
      - id: owner
        type: saved_modern_variable_member_owner
        if: _root.version >= 0x2024

  saved_attribute_double_variable_member:
    doc: Floating-point AttributeType default, minimum, and maximum
    seq:
      - id: default_value
        type: f8
      - id: minimum_value
        type: f8
      - id: maximum_value
        type: f8
      - id: owner
        type: saved_modern_variable_member_owner
        if: _root.version >= 0x2024

  saved_attribute_quantity_variable_member:
    doc: Quantity AttributeType state followed by minimum and maximum floating-point values
    seq:
      - id: quantity_state_or_free_link
        type: u4
      - id: quantity_state_high
        type: u4
      - id: minimum_value
        type: f8
      - id: maximum_value
        type: f8
      - id: owner
        type: saved_modern_variable_member_owner
        if: _root.version >= 0x2024

  saved_attribute_unit_variable_member:
    doc: AttributeUnit formatting state plus affine conversion offset and scale
    seq:
      - id: format_state_or_free_link
        type: u4
      - id: display_precision
        type: u4
      - id: conversion_offset
        type: f8
      - id: conversion_scale
        type: f8
      - id: owner
        type: saved_modern_variable_member_owner
        if: _root.version >= 0x2024

  saved_attribute_list_variable_member:
    doc: List AttributeType default-item selection and state
    seq:
      - id: default_item_or_free_link
        type: u4
      - id: list_state
        type: u4
      - id: owner
        type: saved_modern_variable_member_owner
        if: _root.version >= 0x2024

  saved_attribute_text_item_variable_member:
    doc: AttributeTextItem selection and state
    seq:
      - id: selected_or_free_link
        type: u4
      - id: item_state
        type: u4
      - id: owner
        type: saved_modern_variable_member_owner
        if: _root.version >= 0x2024

  saved_attribute_text_variable_member:
    doc: Text AttributeType state and text constraint
    seq:
      - id: enabled_or_free_link
        type: u4
      - id: text_state
        type: u4
      - id: owner
        type: saved_modern_variable_member_owner
        if: _root.version >= 0x2024

  saved_attribute_value_variable_member:
    doc: AttributeValue discriminator and 64-bit scalar/text payload
    seq:
      - id: value_state_or_free_link
        type: u4
      - id: value_kind
        type: u4
      - id: payload_low
        type: u4
      - id: payload_high
        type: u4
      - id: owner
        type: saved_modern_variable_member_owner
        if: _root.version >= 0x2024

  saved_attribute_inheritance_variable_member:
    doc: AttributeInheritance relationship state, scope kind, and inheritance mode
    seq:
      - id: relationship_state_or_free_link
        type: u4
      - id: scope_kind
        type: u4
      - id: inheritance_mode
        type: u4
      - id: modern_alignment_state
        type: u4
        if: _root.version >= 0x2024
      - id: owning_fixed_record_ordinal
        type: u4
        if: _root.version >= 0x2024
      - id: allocation_state
        type: u4
        if: _root.version >= 0x2024

  serialized_normal_page:
    doc: |
      On-disk form of a normal DBM allocator page. The loader allocates
      `allocation_size`, registers `saved_page_base` in its pointer-relocation
      table, and copies `payload_length` bytes verbatim to memory-page offset
      16. The payload is an exactly tiled array of capacity-framed StringCtl
      slots on all 454 corpus pages (1,082,121 slots total).
    seq:
      - id: allocation_size
        type: u4
        valid: 0x10000
        doc: 64 KiB allocation including the reconstructed 16-byte runtime header
      - id: payload_length
        type: u4
        valid:
          max: 0xfff0
        doc: live page bytes copied to memory offset 16; 0xFFF0 on every non-final normal page
      - id: saved_page_base
        type: u4
        doc: original 64 KiB-aligned allocation base used to relocate serialized pointers
      - id: string_allocator_slots
        type: string_allocator_page_contents
        size: payload_length
        doc: active strings and retained free/capacity storage

  string_allocator_page_contents:
    doc: exactly tiled DBC_StringCtl allocator slots
    seq:
      - id: slots
        type: string_allocator_slot
        repeat: eos

  string_allocator_slot:
    doc: |
      Capacity-framed DBC_StringCtl slot. `used_length` is 0xFFFF for a free
      slot; otherwise it selects the active bytes at the start of the capacity.
      Active content is usually a NUL-terminated ASCII string, but 46 corpus
      slots contain binary controller values. Retained capacity is not padding:
      it is nonzero in 147,459 of 1,075,221 active slots; 6,900 free slots retain arbitrary old
      contents.
    seq:
      - id: capacity
        type: u2
        valid:
          min: 1
      - id: used_length
        type: u2
        doc: active byte count, or 0xFFFF when this allocator slot is free
      - id: active_contents
        size: used_length
        if: used_length != 0xffff
        doc: active string or binary controller value
      - id: retained_capacity_contents
        size: 'used_length == 0xffff ? capacity : capacity - used_length'
        doc: free-slot contents or unused capacity retaining prior bytes; never file padding

  dir_entry:
    doc: |
      16-byte directory entry.

      `total_bytes` is the controller's IN-MEMORY footprint, not the bytes it
      wrote, and `count` is likewise the container's capacity for some sections
      and the live record count for others. Two consequences, both verified
      corpus-wide:

      * Section 3 over-declares by `entry_count*16 + 48` -- the directory plus a
        48-byte header, written once as the FILE header and never repeated in
        section 3's payload. Every section after 3 therefore accumulates that
        much too high. The corrected value is

            payload_offset = data_offset - (entry_count*16 + 48)   for i > 3

        i.e. 1232 on 74-entry files and 1216 on 73-entry ones. Verified by the
        fixed section arrays on all 90 scoped corpus files.

      * Section 1 over-declares by exactly one entry (see `entry_count`).

      `total_bytes / count` IS the real record stride where both are non-zero,
      and should be preferred over a per-version constant -- it distinguishes the
      100-byte old and 112-byte new decal records with no version branch.
    seq:
      - id: count
        type: u4
        doc: record or allocation-unit count
      - id: total_bytes
        type: u4
        doc: serialized payload bytes of this section's data region
      - id: in_memory_base
        type: u4
        doc: |
          The controller's serialized 32-bit in-memory base address, zero on all
          but one unique corpus file. Where present the addresses chain exactly
          with the declared sizes. These are MEMORY addresses and sections are
          not allocated in file order, so they must never be treated as file
          offsets.
      - id: base_pointer_high_padding
        contents: [0, 0, 0, 0]
        doc: |
          Zero high word/padding for the 32-bit in-memory base pointer. Zero in
          every directory entry on 90 of 90 unique corpus files.

  # =========================================================================
  # SECTION 56/57 -- the attribute string index and its pool
  # =========================================================================
  string_index_entry:
    doc: |
      A 16-byte index record pointing into section 57's string pool. The index
      begins inside section 56, after that section's live records, and runs up to
      the pool itself; the directory's section 56/57 boundary falls in the middle
      of it, so the two must be decoded together.

      The layout is proved by an exact tiling invariant across the whole table:

          pool_offset[k] + length[k] == pool_offset[k+1]

      e.g. 30274+35 = 30309, +25 = 30334, +28 = 30362. The table need not
      include the pool's first string: on many files its first serialized entry
      starts at pool offset 25, immediately after
      `DFT_CONFIGURATION\0PARENT\0`.

      LOCATING THE POOL. Do not accumulate to it and do not scan for it. The
      terminator entry -- the one satisfying

          pool_offset + length == section57.total_bytes

      is unique, and the pool begins 16 bytes past its true start:

          pool_start = terminator_entry_offset + 16
          pool_end   = pool_start + section57.total_bytes

      The old pseudo-entry started one byte before this type, decoded its offset
      at +1, and consequently placed the pool at pseudo-entry +16. That included
      the high byte of the preceding record metadata as pool content. Expressed
      against that obsolete origin the correction is +17; expressed against this
      complete 16-byte type it is the ordinary +16 with no intervening padding.

      This resolves all 89 scoped files with a nonempty pool; the remaining file
      has no section-57 pool. Two guards must NOT be added:

        * requiring the back-walk to reach entry 0 -- locating the pool never
          needs entry 0, and insisting on it rejects valid terminators wherever
          the tiling has a gap (score falls to 34/163);
        * requiring the pool to be entirely printable -- attribute values embed
          binary, e.g. CANFILTER-001's pool is 99.92% printable with bytes like
          `04 e4 6e 12` inside `750000 N + 0 Y 0 0 <bin> NUMBER_2`. A 98%
          threshold takes the score from 155 to 162.

      The one board that still misses, TMS1mmX19, has its last entry ending at
      36651 against a declared pool length of 36703 -- 52 bytes of pool tail
      indexed by no entry, which a small slack accepts.
    seq:
      - id: pool_offset
        type: u4
        doc: byte offset of this string within section 57's pool
      - id: length
        type: u2
        doc: string length; pool_offset + length == next entry's pool_offset
      - id: handle_a
        type: u2
      - id: handle_b
        type: u2
      - id: handle_c
        type: u2
        doc: 0xFFFF when unused; non-FFFF on 3,123 of 65,414 corpus records
      - id: record_metadata
        type: u4
        doc: |
          Serialized per-record metadata, not padding. Nonzero on 32,972 of
          65,414 records; observed values include graphics-configuration masks,
          packed values, 0xFFFFFFFF, and zero.

  string_index_array:
    seq:
      - id: entries
        type: string_index_entry
        repeat: eos

  string_pool_contents:
    doc: |
      Section-57 indexed string storage. Most values are NUL-terminated ASCII;
      some attribute values intentionally contain binary controller bytes. Each
      byte is owned by a string-index entry or retained pool string tail.
    seq:
      - id: indexed_string_bytes
        size-eos: true

  cntr_item_array:
    doc: |
      Serialized OLE container items. The array count is zero on all 90 corpus
      files. The populated representation below is retained from serializer
      analysis, but no scoped corpus file exercises it. Each populated item is an MFC COleClientItem carrying a
      length-delimited Microsoft Compound File Binary document and PADS view state.
    seq:
      - id: count
        type: u4
        doc: number of serialized CPowerPCBCntrItem objects
      - id: mfc_new_class_tag
        contents: [0xff, 0xff]
        if: count > 0
        doc: MFC CArchive new-class tag
      - id: mfc_schema
        type: u2
        if: count > 0
        doc: MFC runtime-class schema number
      - id: class_name_length
        type: u2
        if: count > 0
        doc: MFC runtime-class name length; 17 for CPowerPCBCntrItem
      - id: class_name
        type: str
        size: class_name_length
        encoding: ASCII
        if: count > 0
        doc: MFC runtime-class name CPowerPCBCntrItem
      - id: items
        type: powerpcb_cntr_item
        repeat: expr
        repeat-expr: count
        doc: serialized embedded OLE items

  powerpcb_cntr_item:
    doc: |
      CPowerPCBCntrItem::Serialize payload. The first five fields come from
      MFC COleClientItem::Serialize (mfc140 ordinal 13091); the remaining view
      fields come from the PADS override at PowerUI500.dll 0x10519300.

      No scoped corpus file contains an item. Field meanings and framing come
      from the MFC and PADS serializer implementations.
    seq:
      - id: ole_item_format_version
        type: u4
        valid: 0x100
        doc: COleClientItem serialization format marker
      - id: item_number
        type: u4
        doc: COleClientItem document-item ordinal
      - id: ole_object_reference
        type: u4
        doc: MFC CArchive object reference for the linked OLE object
      - id: link_unavailable
        type: u2
        doc: COleClientItem link-unavailable flag serialized as a u16
      - id: draw_aspect
        type: u4
        doc: OLE DVASPECT value
      - id: compound_file_size
        type: u4
        doc: exact byte length of the following Microsoft CFB document
      - id: compound_file
        type: microsoft_cfb
        size: compound_file_size
        doc: |
          Embedded OLE compound document containing the linked item's streams.
      - id: document_guid
        type: str
        size: 38
        encoding: ASCII
        doc: PADS PCB document GUID {2FE18320-6448-11d1-A412-000000000000}
      - id: database_box
        type: rect_i32
        doc: PADS database-coordinate bounding box written from object offset 0x84
      - id: original_pixel_rect
        type: serialized_crect
        doc: original pixel rectangle; CRect fields serialized left, bottom, right, top
      - id: plane
        type: u4
        doc: display plane from CPowerPCBCntrItem object offset 0xA4
      - id: white_background
        type: u4
        doc: white-background display flag from object offset 0xA8

  rect_i32:
    seq:
      - id: left
        type: s4
      - id: top
        type: s4
      - id: right
        type: s4
      - id: bottom
        type: s4

  serialized_crect:
    seq:
      - id: left
        type: s4
      - id: bottom
        type: s4
      - id: right
        type: s4
      - id: top
        type: s4

  footer:
    doc: MFC document footer after the serialized container-item array
    seq:
      - id: guid
        type: str
        size: 38
        encoding: ASCII
        doc: footer GUID {2FE18320-6448-11d1-A412-000000000000}
      - id: cntr_item_back_ptr
        type: u4
        doc: |
          Absolute file offset of the serialized CPowerPCBCntrItem array. At
          that offset is a u32 item count. On all 90 corpus files the count is
          zero and the pointer targets the four bytes immediately before the
          footer GUID. This is not a size or checksum.

  # =========================================================================
  # SECTION 0 — file-global object header (no data region)
  # =========================================================================
  # The directory slot at index 0 is the data. data_offset==0, no region.
  sec0_global_header:
    doc: |
      Section 0 has no data region; its directory slot is a file-global header.
      global_object_count == sum(section[i].count for i in 2..N-1).
      global_payload_bytes == sum(section[i].total_bytes for i in 2..N-1 with
      count>0). Excludes sec0 (self) and sec1 (board-param table). MFC CArchive
      object-pool pre-sizing header.
    seq:
      - id: global_object_count
        type: u4
      - id: global_payload_bytes
        type: u4
      - id: in_memory_base
        type: u4
        doc: serialized controller arena base; nonzero on the address-chain corpus file
      - id: base_pointer_high_padding
        contents: [0, 0, 0, 0]
        doc: zero high word of the 32-bit arena pointer on 90 of 90 corpus files

  # =========================================================================
  # SECTION 1 — board setup / view-state header (single 1200-B blob)
  # =========================================================================
  # Not a record array. Only the first ~180 bytes carry data; the rest is zero.
  #
  # LOCATING IT. The block does not start at the accumulated data_offset, and the
  # displacement is fully explained by two declared quantities:
  #
  #     base = 10 + (directory[1].count - 1)*16 + directory[2].count*48
  #
  # The first term is the stored slot count (see `num_directory`); using the
  # version-implied count instead is what made v0x2022 and v0x2024 read 16 bytes
  # short. The second is section 2, a run of 48-byte view-state records PADS
  # writes ahead of the block -- its length is just that section's declared
  # count, so the "variable-length prefix" that used to force a search was in the
  # directory all along.
  #
  # Measured over all 90 scoped boards: directory[2].count equals the observed
  # prefix length in 48-byte units on 90/90. 74 boards have no prefix; the rest
  # run 1..22 records and every one matches its declared count exactly.
  #
  # Against the field-range search this replaces (SCALE / BACKUPTIME / REAL WIDTH
  # / ALLSIGONOFF / REFNAMESIZE swept byte-by-byte over section 1): they agree on
  # every scoped board. Those five fields are now
  # a validator -- a file whose layout differs yields no origin rather than a
  # plausible-looking wrong one.
  #
  # This matters more than one section: the origin at +60/+64 shifts every
  # absolute coordinate in the file, so a wrong base moves the whole board.
  sec1_board_setup:
    seq:
      - id: legacy_aux_record_count
        type: u4
        doc: off0 legacy auxiliary-controller live record count; nonzero on 17 scoped files
      - id: legacy_aux_record_bytes
        type: u4
        doc: |
          off4 legacy auxiliary-controller allocation bytes. In old dialects
          with a live count this is exactly count * 52; modern files may retain
          a nonzero allocation value with count zero.
      - id: legacy_aux_memory_base
        type: u4
        doc: off8 serialized 32-bit allocator base; zero on all 90 scoped files
      - id: user_grid
        type: s4
        doc: off12 USERGRID (X; Y assumed equal)
      - id: maximum_layer
        type: s4
        doc: off16 MAXIMUMLAYER (max routing layer)
      - id: work_level
        type: s4
        doc: off20 WORKLEVEL
      - id: display_level
        type: s4
        doc: off24 DISPLAYLEVEL
      - id: layer_pair
        type: u4
        doc: off28 packed (layer2<<16)|layer1
      - id: layer_pair_max
        type: u4
        doc: off32 (maxlayer<<16)|0x41; low byte 0x41 = VIAMODE 'A'
      - id: line_width
        type: s4
        doc: off36 LINEWIDTH (default item width)
      - id: text_height
        type: s4
        doc: off40 TEXTSIZE[0]
      - id: text_line_width
        type: s4
        doc: off44 TEXTSIZE[1]
      - id: job_time
        type: s4
        doc: off48 JOBTIME (seconds)
      - id: dot_grid
        type: s4
        doc: off52 DOTGRID
      - id: view_scale
        type: f4
        doc: off56 SCALE (view zoom; live view state)
      - id: origin_x
        type: s4
        doc: off60 RAW origin X; design = raw - origin
      - id: origin_y
        type: s4
        doc: off64 RAW origin Y
      - id: window_center_x
        type: s4
        doc: off68 RAW window center X (live view state)
      - id: window_center_y
        type: s4
        doc: off72 RAW window center Y
      - id: backup_time
        type: s4
        doc: off76 BACKUPTIME (minutes)
      - id: real_width
        type: s4
        doc: off80 REAL WIDTH
      - id: all_sig_on_off
        type: s4
        doc: off84 ALLSIGONOFF
      - id: view_extension_flags
        type: u4
        doc: off88 persisted view-extension flags; 0x40 on 10 scoped files, otherwise zero
      - id: refname_height
        type: s4
        doc: off92 REFNAMESIZE[0]
      - id: refname_line_width
        type: s4
        doc: off96 REFNAMESIZE[1]
      - id: default_size_a
        type: s4
        doc: off100 const 457200 (=12 mil)
      - id: default_size_b
        type: s4
        doc: off104 const 457200
      - id: default_size_c
        type: s4
        doc: off108 const 457200
      - id: highlight
        type: s4
        doc: off112 HIGHLIGHT
      - id: concol_flag
        type: s4
        doc: off116 CONCOL
      - id: zero120
        contents: [0, 0, 0, 0]
        doc: off120 zero on 90 of 90 unique corpus files
      - id: feature_flags
        type: u4
        doc: off124 0x094D4000 constant (DB-version/capability flags)
      - id: view_extension_values
        type: s4
        repeat: expr
        repeat-expr: 4
        doc: |
          off128..143 four persisted view/grid values enabled by
          view_extension_flags. Nonzero on the same 10 scoped files as flag 0x40;
          these bytes were formerly and incorrectly called reserved zero.
      - id: job_name_storage
        type: str
        size: 260
        encoding: ASCII
        terminator: 0
        doc: |
          off144..403 fixed MAX_PATH-sized JOBNAME buffer. Bytes after the
          first NUL are retained buffer capacity and can contain prior filenames;
          they are not padding (nonzero in 40 scoped files).
      - id: editor_view_state_words
        type: u4
        repeat: eos
        doc: |
          off404..end persisted editor/display state vector. Section 1 is 1168,
          1184, or 1200 bytes in the corpus; the vector therefore has 191, 195,
          or 199 words. Zero values are disabled/default settings, not padding.

  # =========================================================================
  # SECTION 2 — persisted editor view records
  # =========================================================================
  # Written before section 1 despite its directory index. The directory is
  # exact on the corpus: total_bytes == count * 48 for all 90 unique files.
  sec2_view_state:
    doc: 48-byte persisted editor viewport/object-view record
    seq:
      - id: controller_state
        type: u4
      - id: object_handle
        type: u4
      - id: owner_pointer
        type: u4
        doc: serialized 32-bit view/object owner reference
      - id: center_x_raw
        type: s4
        doc: saved viewport/object X in RAW database coordinates
      - id: center_y_raw
        type: s4
        doc: saved viewport/object Y in RAW database coordinates
      - id: display_flags
        type: u4
      - id: layer_selection
        type: u4
      - id: transform_flags
        type: u4
      - id: zoom_scale
        type: f4
        doc: saved view scale; common bit patterns include 1.875 and 15.0
      - id: view_state_a
        type: u4
      - id: view_parameter
        type: f4
      - id: view_state_b
        type: u4

  sec2_view_state_array:
    seq:
      - id: records
        type: sec2_view_state
        repeat: eos

  # =========================================================================
  # SECTION 3 — board-parameter image (declared footprint minus directory/header)
  # =========================================================================
  # The directory declares 3928/3932 bytes in modern files, but only 2696/2700
  # bytes are written: directory_bytes + 48 were already emitted at file start.
  # Default-valued state dominates, but no broad range is padding.
  sec3_board_params:
    doc: version-selected section 3 board-parameter serialization
    seq:
      - id: modern
        type: sec3_board_params_modern
        size-eos: true
        if: _root.version >= 0x2025
      - id: legacy
        type: sec3_board_params_legacy
        size-eos: true
        if: _root.version < 0x2025

  sec3_board_params_legacy:
    doc: |
      Legacy v0x2017..v0x2024 written board-parameter image: declared footprint
      minus directory_bytes+48, yielding 1,380 bytes in v0x2017/v0x2019 and
      1,392 bytes in v0x2021..v0x2024.
      It contains the same PCB general-parameter, display-palette, DRC-default,
      and embedded via-stack state as the modern layout, but fields after the
      fixed-flag table move by dialect. Kept as typed 32-bit serialized state
      words until the seven-file legacy sample can support a safe field split;
      zero words are disabled/default settings, not padding.
    seq:
      - id: parameter_state_words
        type: u4
        repeat: eos

  sec3_board_params_modern:
    seq:
      - id: display_palette_words
        type: u4
        repeat: expr
        repeat-expr: 208
        doc: |
          0..831 persisted 16-by-52-byte display/palette table. Default-zero on
          all 66 scoped files using this modern layout; serialized state, not
          alignment padding.
      - id: fbgcol
        type: s4
        doc: '832 FBGCOL[0]'
      - id: fbgcol2
        type: s4
        doc: '836 FBGCOL[1]'
      - id: hatch_grid
        type: s4
        doc: 840 HATCHGRID
      - id: teardrop
        type: s4
        doc: 844 TEARDROP
      - id: default_sizes_a
        type: s4
        repeat: expr
        repeat-expr: 3
        doc: 848/852/856 default sizes 25/12/13 mil (const)
      - id: legacy_parameter_extension_words
        type: s4
        repeat: expr
        repeat-expr: 32
        doc: |
          860..987 version-dependent board-parameter extension. Default-zero on
          all 66 scoped files using this modern layout; zero denotes default
          state and is not alignment padding.
      - id: table_len
        type: s4
        doc: '988 = 8 (length marker for the 0x102/0x101 table below)'
      - id: ther_line_wid
        type: s4
        doc: 992 THERLINEWID
      - id: fixed_flag_table
        type: s4
        repeat: expr
        repeat-expr: 30
        doc: 996..1115 alternating 0x102/0x101; fixed-size, not per-layer
      - id: default_size_b
        type: s4
        doc: 1116 const 457200 (12 mil)
      - id: default_size_c
        type: s4
        doc: 1120 const 952500 (25 mil)
      - id: all_sig_flags
        type: u4
        doc: 1124 ALLSIGFLAGS
      - id: const_pre_drc
        type: s4
        repeat: expr
        repeat-expr: 8
        doc: 1128..1159 (200,200,PSVIAGRID,6,50,50,85,3)
      - id: pad_fill_wid
        type: s4
        doc: 1160 PADFILLWID
      - id: ther_smd_wid
        type: s4
        doc: 1164 THERSMDWID
      - id: min_hat_area
        type: s4
        doc: 1168 MINHATAREA / HATCHMODE (0)
      - id: hatch_mode
        type: s4
        doc: 1172 HATCHMODE
      - id: hatch_disp
        type: s4
        doc: 1176 HATCHDISP
      - id: drill_hole
        type: s4
        doc: 1180 DRILLHOLE
      - id: mitre_radii
        type: s4
        repeat: expr
        repeat-expr: 7
        doc: 1184..1211 MITRERADII x1000
      - id: mitre_type
        type: s4
        doc: 1212 MITRETYPE
      - id: hatch_rad
        type: s4
        doc: 1216 HATCHRAD x1000
      - id: mitre_ang
        type: s4
        repeat: expr
        repeat-expr: 7
        doc: 1220..1247 MITREANG
      - id: default_text_size
        type: s4
        doc: 1248 const 3810000
      - id: teardrop_angle_limit
        type: s4
        doc: 1252 optional teardrop angle limit; 81000000 = 45 degrees * 1800000
      - id: teardrop_length_limit
        type: s4
        doc: 1256 optional teardrop length threshold, BASIC
      - id: ther_flags
        type: u4
        doc: 1260 THERFLAGS
      - id: drl_oversize
        type: s4
        doc: 1264 DRLOVERSIZE
      - id: dot_grid
        type: s4
        doc: 1268 DOTGRID
      - id: grid_default
        type: s4
        doc: 1272
      - id: user_grid
        type: s4
        doc: 1276 USERGRID
      - id: plane_rad
        type: s4
        doc: 1280 PLANERAD x1000
      - id: plane_flags_packed
        type: u4
        doc: 1284 PLANEFLAGS packed bitfield
      - id: comp_height
        type: s4
        doc: 1288 COMPHEIGHT
      - id: kpt_hatch_grid
        type: s4
        doc: 1292 KPTHATCHGRID
      - id: bottom_component_height
        type: s4
        doc: 1296 BOTCMPHEIGHT
      - id: fanout_grid_x
        type: s4
        doc: 1300 FANOUTGRID X
      - id: fanout_grid_y
        type: s4
        doc: 1304 FANOUTGRID Y
      - id: fanout_length
        type: s4
        doc: 1308 FANOUTLENGTH
      - id: router_flags
        type: u4
        doc: 1312 ROUTERFLAGS
      - id: verify_flags
        type: u4
        doc: 1316 VERIFYFLAGS
      - id: fab_chk_flags
        type: u4
        doc: 1320 FABCHKFLAGS
      - id: at_max_size
        type: s4
        doc: 1324 ATMAXSIZE
      - id: at_max_angle
        type: s4
        doc: 1328 ATMAXANGLE
      - id: sl_min_copper
        type: s4
        doc: 1332 SLMINCOPPER
      - id: sl_min_mask
        type: s4
        doc: 1336 SLMINMASK
      - id: st_min_clear
        type: s4
        doc: 1340 STMINCLEAR
      - id: st_min_spokes
        type: s4
        doc: 1344 STMINSPOKES
      - id: tp_min_width
        type: s4
        doc: 1348 TPMINWIDTH
      - id: tp_min_size
        type: s4
        doc: 1352 TPMINSIZE
      - id: ss_min_gap
        type: s4
        doc: 1356 SSMINGAP
      - id: sb_min_gap
        type: s4
        doc: 1360 SBMINGAP
      - id: sb_layer
        type: s4
        doc: 1364 SBLAYER
      - id: arptom
        type: s4
        doc: 1368 ARPTOM
      - id: arptom_layer
        type: s4
        doc: 1372 ARPTOMLAYER
      - id: ardtom
        type: s4
        doc: 1376 ARDTOM
      - id: ardtom_layer
        type: s4
        doc: 1380 ARDTOMLAYER
      - id: ardtop
        type: s4
        doc: 1384 ARDTOP
      - id: ardtop_layer
        type: s4
        doc: 1388 ARDTOPLAYER
      - id: viap_spacing
        type: s4
        doc: 1392 VIAPSPACING
      - id: viap_shape
        type: s4
        doc: 1396 VIAPSHAPE
      - id: viap_to_trace
        type: s4
        doc: 1400 VIAPTOTRACE
      - id: viap_fill
        type: s4
        doc: 1404 VIAPFILL
      - id: viap_word_a
        type: u4
        doc: 1408 via-pattern packed word A (VIAPSHSIG name-handle)
      - id: viap_word_b
        type: u4
        doc: 1412 via-pattern packed word B (high byte 0x0E const)
      - id: viap_flag
        type: s4
        doc: 1416 VIAPFLAG
      - id: flow_flags
        type: s4
        doc: 1420 FLOWFLAGS
      - id: auxiliary_name_buffers
        type: fixed_path_storage
        repeat: expr
        repeat-expr: 4
        doc: |
          1424..2463 first four fixed 260-byte auxiliary/CAM selector buffers.
      - id: final_auxiliary_name_buffer
        type: str
        size-eos: true
        encoding: ASCII
        terminator: 0
        doc: |
          Truncated fifth auxiliary/CAM selector buffer: 232 bytes in v0x2025,
          236 in v0x2026/v0x2027. Seven large designs populate these five
          buffers with values such as C,#_C / P,J / D,CR / L,#_L / R,#_R.
          Spare capacity is retained storage, not padding.

  fixed_path_storage:
    doc: fixed 260-byte filename, selector, or command buffer with retained capacity
    seq:
      - id: value
        type: str
        size: 260
        encoding: ASCII
        terminator: 0

  # =========================================================================
  # SECTION 4 — padstack definitions
  # =========================================================================
  # 64 B/record, one per distinct padstack definition. marker 0xFE @56 = valid.
  # shape enum {0:OF,1:RF,2:R,3:S}.
  sec4_padstack:
    seq:
      - id: flags
        type: s4
        doc: '+0 {0,2,8}; 2 = first padstack of a decal group'
      - id: finger_off_x
        type: s4
        doc: '+4 finger X-offset (FINOFFSET part)'
      - id: finger_len_y
        type: s4
        doc: '+8 finger length / Y-offset'
      - id: finger_ori_raw
        type: s4
        doc: '+12 finger orientation = deg*1800000 (only 0 / 162000000=90deg)'
      - id: padstack_state
        type: s4
        doc: '+16 padstack controller state; usually zero'
      - id: object_id
        type: u4
        doc: '+20 object id (handle)'
      - id: lib_flag
        type: u4
        doc: '+24 bit31 = library/plated flag'
      - id: pad_width
        type: s4
        doc: '+28 padWidth (size A), BASIC'
      - id: drill
        type: s4
        doc: '+32 drill / inner diameter (0 = SMD)'
      - id: fin_length
        type: s4
        doc: '+36 finLength (size B) for finger pads, else 0'
      - id: finger_off_2
        type: s4
        doc: '+40 finger offset (second component)'
      - id: corner_radius
        type: s4
        doc: '+44 corner radius'
      - id: angle_raw
        type: s4
        doc: '+48 rotation = deg*1800000 (only 0 / 90deg)'
      - id: sec5_index
        type: u4
        doc: '+52 cumulative start row into sec5 pad-shape table'
      - id: marker
        type: u1
        doc: '+56 0xFE = valid padstack record'
      - id: shape_code
        type: u1
        enum: pad_shape
        doc: '+57 shape'
      - id: layer_count
        type: u2
        doc: '+58 number of sec5 layer entries this padstack owns'
      - id: housekeeping
        type: s4
        doc: '+60 back-link / checksum'

  sec4_padstack_array:
    seq:
      - id: modern_records
        type: sec4_padstack
        repeat: eos
        if: _root.directory[4].total_bytes / _root.directory[4].count == 64
      - id: legacy_state_words
        type: u4
        repeat: eos
        if: _root.directory[4].total_bytes / _root.directory[4].count != 64
        doc: v0x2017..v0x2022 52/56-byte padstack dialect state

  # =========================================================================
  # SECTION 5 — per-padstack pad-shape layer table
  # =========================================================================
  # Flat array of 24-B rows. sec4[k] owns rows [sec4[k].sec5_index, +layer_count).
  # Rows [0, firstStart~80) are a global default prefix.
  sec5_pad_layer:
    seq:
      - id: dim_b
        type: s4
        doc: '+0 corner radius / mask delta (shape-dependent; raw geom in tail rows)'
      - id: shape_param
        type: u4
        doc: '+4 shape-class parameter word + flags, NOT a coord (81000000=thermal)'
      - id: layer
        type: u1
        doc: '+8 PADS layer id (0 top,21/23/27/28 silk/mask/paste,254/255 special)'
      - id: shape_class
        type: u1
        doc: '+9 {0,1,2,3,4,6,8} pad-entry class (2=primary copper pad)'
      - id: layer_seq
        type: s2
        doc: '+10 signed sequence index within stack (usually 0)'
      - id: size_a
        type: s4
        doc: '+12 pad dimension A (width / diameter)'
      - id: size_b
        type: s4
        doc: '+16 pad dimension B (height); 0 for round/square'
      - id: default_extra
        type: u4
        doc: '+20 0x04000000|radius on layer255/shape2 rows, else 0'

  sec5_pad_layer_array:
    seq:
      - id: modern_records
        type: sec5_pad_layer
        repeat: eos
        if: _root.directory[5].total_bytes / _root.directory[5].count == 24
      - id: legacy_state_words
        type: u4
        repeat: eos
        if: _root.directory[5].total_bytes / _root.directory[5].count != 24
        doc: v0x2017/v0x2019/v0x2021 20-byte pad-layer dialect state

  # =========================================================================
  # SECTION 8 — TEXT / label table
  # =========================================================================
  # 72 B header records (marker 0xFFFE@4, reserved0@16) then a packed C-string
  # pool. The text-header stream spans the sec5 tail; the string pool spills
  # into sec9. Free-text metadata lags geometry by one slot: text K's geometry
  # is in rec K; its string offset, layer and tag are in rec K+1.
  sec8_text_header:
    seq:
      - id: object_id_0
        type: s4
        doc: '+0 idx0 object id / hash'
      - id: marker
        type: u2
        doc: '+4 idx1 record-type marker == 0xFFFE'
      - id: pad0
        type: u2
      - id: str_offset
        type: s4
        doc: '+8 idx2 string-pool byte offset (belongs to text K-1)'
      - id: flags
        type: u4
        doc: '+12 idx3 justification / mirror (packed)'
      - id: text_state0
        type: s4
        doc: '+16 idx4 text-controller state; normally zero'
      - id: str_len_field
        type: u4
        doc: '+20 idx5 high16 = strlen+1 (drill-table subtype)'
      - id: layer_field
        type: u4
        doc: '+24 idx6 low16=layer/LEVEL, high16=0x0020 marks TEXT object'
      - id: tag
        type: u4
        doc: '+28 idx7 free-text tag 0x48FE0000 / 0x49000000'
      - id: tag_hi
        type: s4
        doc: '+32 idx8 == 0'
      - id: height
        type: s4
        doc: '+36 idx9 text HEIGHT, BASIC'
      - id: line_width
        type: s4
        doc: '+40 idx10 text line WIDTH, BASIC'
      - id: origin_x
        type: s4
        doc: '+44 idx11 insertion X (RAW = design + origin)'
      - id: origin_y
        type: s4
        doc: '+48 idx12 insertion Y (RAW)'
      - id: rot_aux
        type: s4
        doc: '+52 idx13 rotation / secondary offset (0 / 162e6 / 324e6)'
      - id: text_state1
        type: s4
        doc: '+56 idx14 text geometry state; normally zero'
      - id: bbox_x
        type: s4
        doc: '+60 idx15 first-glyph corner X (RAW)'
      - id: bbox_y
        type: s4
        doc: '+64 idx16 first-glyph corner Y (RAW)'
      - id: object_id_1
        type: s4
        doc: '+68 idx17 object id / hash'

  sec8_text_array:
    seq:
      - id: records
        type: sec8_text_header
        repeat: eos
        if: _root.directory[8].total_bytes / _root.directory[8].count == 72
      - id: v2017_state_words
        type: u4
        repeat: eos
        if: _root.directory[8].total_bytes / _root.directory[8].count == 64

  # =========================================================================
  # SECTION 9 — string-pool tail + phase-shifted head of sec10 DRW/LINES array
  # =========================================================================
  # A slice of one contiguous stream spanning sec8->sec10: a C-string pool tail
  # (continues sec8) then a fixed 1148-B head of the sec10 112-byte DRW/LINES
  # record array. The cut lands mid-string and mid-record.
  sec9:
    doc: |
      sec9 = string_pool_tail (variable; ends at the first 00 FF 00 00, the start
      of the first DRW record) + lines_array_head (constant 1148 B). The pool/DRW
      split is content-dependent. The C-string pool tail continues sec8's pool;
      the remaining 1148 B is the phase-shifted head of the sec10 *LINES* (DRW)
      112-byte record array, whose records continue byte-for-byte into sec10 with
      a continuous obj_index.
    seq:
      - id: string_and_drawing_bytes
        size-eos: true
        doc: |
          whole sec9 region; split at the first 00 FF 00 00 into the C-string
          pool tail then the 1148-B DRW/LINES array head.

  sec10_drw_record:
    doc: |
      112-byte *LINES* (DRW) drawing-object record (board outline / dimension /
      milling / fiducial / filled copper shape / keepout owner). Marker
      FE FF 00 00 FF FF FF FF at +0 for most records. First inline piece coord
      (RAW) at +88.

      Filled COPPER/COPCLS owner: name DRW*, class_tag +84 == 0x00004900,
      flag6 +24 == 1, flag7 +28 != 3 (v0x2025/v0x2027); class_tag +84 ==
      0x00004D00, flag6 +24 == 7, flag7 +28 == 0 (v0x2026). bbox is RAW at
      +96/+100/+104/+108; subtract origin +88/+92 for the section-12 local bbox.

      KEEPOUT/KPTCLS owner: name DRW*, class_tag +84 == 0, flag6 +24 == 1, flag7
      +28 in {1,10}.
    seq:
      - id: marker
        type: u2
        doc: '+0 0xFFFE'
      - id: pad
        type: u2
        doc: '+2 0'
      - id: sentinel
        type: s4
        doc: '+4 0xFFFFFFFF (-1)'
      - id: obj_index
        type: s4
        doc: '+8 global object index (continuous sec9->sec10)'
      - id: index2
        type: s4
        doc: '+12 secondary index (into a sibling vertex/list table)'
      - id: index3
        type: s4
        doc: '+16 zero for filled-copper owners; nonzero on keepout owners'
      - id: zero0
        type: s4
        doc: '+20 = 0'
      - id: flag6
        type: s4
        doc: '+24 small enum (1); not the layer'
      - id: flag7
        type: u4
        doc: '+28 flags; 0, 3, 0x00020003 for copper/graphics; 1 or 10 on keepout owners'
      - id: zero1
        type: u4
        doc: '+32 = 0'
      - id: heap_handle
        type: u4
        doc: '+36 heap/object handle, +0x41/record (handle)'
      - id: tag10
        type: u4
        doc: '+40 0x80000000'
      - id: handle_str
        type: strz
        encoding: ASCII
        size: 40
        doc: '+44 inline "DRW#######" object handle, zero-padded to +84'
      - id: block_tag
        type: u4
        doc: '+84 class tag; 0x00004900 (v0x2025/v0x2027 filled-copper owner), 0x00004D00 (v0x2026 filled-copper owner), 0 (keepout / board-outline owner)'
      - id: verts
        type: s4
        repeat: expr
        repeat-expr: 6
        doc: '+88 x0,y0,x1,y1,x2,y2 RAW (x0/y0 = *LINES* insertion point)'

  sec10_drawing_array:
    seq:
      - id: records
        type: sec10_drw_record
        repeat: eos
        if: _root.directory[10].total_bytes / _root.directory[10].count == 112
      - id: legacy_state_words
        type: u4
        repeat: eos
        if: _root.directory[10].total_bytes / _root.directory[10].count == 100

  # =========================================================================
  # SECTION 11 — graphic-piece header table + inline board-outline vertices
  # =========================================================================
  # 20 B/record HEAD (one per OPEN/CLOSED/CIRCLE piece of *LINES* / *PARTDECAL*),
  # then a TAIL (X,Y,attr) i32 triple stream of closed geometry. Arc runs use
  # attr 0,1,2... as indexes into a parallel 20-byte bbox arc table.
  #
  # Dimensions are not a dedicated section; each DIM* item is a *LINES* DRW owner
  # whose sub-pieces appear here. For those sub-pieces the +0 word (sub_flag |
  # byte1<<8 = low-u16 of field0) is a piece-type enum: 6162 BASPNT, 6156 ARWLN1,
  # 6157 ARWLN2, 6158 ARWHD1, 6159 ARWHD2, 6160 EXTLN1, 6161 EXTLN2; the +4 flags
  # word is the per-dimension group flag (matches the ASC piece flags column). The
  # sub-piece vertices land in sec12 in ASC order (BASPNT1 BASPNT2 ARWLN1 ARWHD1
  # ARWLN2 ARWHD2 EXTLN1 EXTLN2), reached through the DIM* owner-run vertex cursor.
  sec11_piece_hdr:
    seq:
      - id: sub_flag
        type: u1
        doc: '+0 piece sub-flag enum {0,1,2,4} (fill/mirror/draw-order)'
      - id: byte1
        type: u1
        doc: '+1 0x01 on DRW piece headers'
      - id: type_or_handle_high
        type: u2
        doc: '+2 piece-type or object-handle high bits'
      - id: flags
        type: s4
        doc: '+4 -1 default; 0x800/0x1000 keepout-restriction bits; small ints = ordinal/parent'
      - id: zero
        type: s4
        doc: '+8 constant 0 (head/tail discriminator)'
      - id: width
        type: s4
        doc: '+12 pen width, BASIC'
      - id: corners
        type: s4
        doc: '+16 vertex/corner count of the piece'

  sec11_piece_array:
    seq:
      - id: records
        type: sec11_piece_hdr
        repeat: eos
        if: _root.directory[11].total_bytes / _root.directory[11].count == 20
      - id: legacy_state_words
        type: u4
        repeat: eos
        if: _root.directory[11].total_bytes / _root.directory[11].count == 16

  sec12_graphic_vertex:
    doc: |
      Section-12 fixed 12-byte graphic vertex. All 90 corpus files satisfy
      total_bytes == count * 12. Coordinates are DESIGN-local for the owning
      section-11 graphic piece and are not origin shifted.
    seq:
      - id: marker
        type: s4
        doc: -1 = contour vertex; {0,1,2,3} = corner/arc-type code
      - id: x_design
        type: s4
      - id: y_design
        type: s4

  sec12_vertex_array:
    seq:
      - id: records
        type: sec12_graphic_vertex
        repeat: eos

  # =========================================================================
  # SECTION 13 — copper-pour hatch-fill geometry
  # =========================================================================
  # The entire directory payload is a 20-byte segment array: 38,268 records in
  # 527 populated files; 70 files are empty. The former 112-byte stack-extent
  # tail was a directory-overdeclaration phase error. Coordinates are pour-local.
  sec13_hatch_seg:
    doc: 20-B copper-pour hatch-fill line segment (pour-LOCAL coords)
    seq:
      - id: x1
        type: s4
      - id: y1
        type: s4
      - id: layer_marker
        type: s4
        doc: 'BASE-900*layer (BASE=-117962100), layer 0..3 = the 4 copper layers'
      - id: x2
        type: s4
      - id: y2
        type: s4

  sec13_hatch_array:
    seq:
      - id: records
        type: sec13_hatch_seg
        repeat: eos

  # =========================================================================
  # SECTION 14 — PARTDECAL terminal-run descriptors
  # =========================================================================
  # Modern 112-byte descriptors carry sentinel 0xFFFE at +108; legacy files use
  # the 100-byte prefix. Both keep the decal name at +44.
  # Start index into the sec15 terminal-position pool is i32 @+0. The terminal
  # count for descriptor K is stored in descriptor K+1 @+4 (one-record lag). The
  # same lagged count stream also appears across sections 13 and 12 for decals
  # without a section-14 descriptor. Descriptor/count reads may cross directory
  # section boundaries.
  sec14_terminal_descriptor_array:
    seq:
      - id: modern_records
        type: sec14_terminal_desc_v112
        repeat: eos
        if: _root.directory[14].total_bytes / _root.directory[14].count == 112
      - id: legacy_records
        type: sec14_terminal_desc_v100
        repeat: eos
        if: _root.directory[14].total_bytes / _root.directory[14].count == 100

  sec14_terminal_desc_v112:
    seq:
      - id: sec15_start
        type: s4
        doc: '+0 first terminal index in sec15 pool'
      - id: lagged_prev_terminal_count
        type: s4
        doc: '+4 terminal count for previous descriptor/name'
      - id: payload
        size: 36
        doc: '+8..+43 descriptor payload / handles'
      - id: decal_name
        type: strz
        encoding: ASCII
        size: 44
        doc: '+44 decal name'
      - id: descriptor_state
        size: 20
        doc: '+88..+107 descriptor tail / bbox flags'
      - id: sentinel
        type: u2
        doc: '+108 0xFFFE (may cross section boundary)'
      - id: flag_b
        type: u2
        doc: '+110 flag high half'

  sec14_terminal_desc_v100:
    doc: legacy 100-byte PARTDECAL terminal-run descriptor
    seq:
      - id: sec15_start
        type: s4
        doc: '+0 first terminal index in the legacy terminal pool'
      - id: lagged_prev_terminal_count
        type: s4
        doc: '+4 terminal count for the previous descriptor/name'
      - id: descriptor_payload
        size: 36
        doc: '+8..+43 descriptor indexes, handles, and controller state'
      - id: decal_name
        type: strz
        encoding: ASCII
        size: 44
        doc: '+44 decal name'
      - id: descriptor_state
        size: 12
        doc: '+88..+99 legacy descriptor tail'

  # =========================================================================
  # SECTION 15 — PARTDECAL terminal and controller storage
  # =========================================================================
  # The directory describes fixed storage units: 20 bytes in versions 0x2017
  # and 0x2019, 36 bytes thereafter. Units hold terminal geometry or mixed
  # decal-controller/object-dictionary state. Modern files place terminal units
  # first, but their suffix is variable and is not a fixed 33-unit trailer.
  # Legacy files can also place controller units before geometry and start after a
  # 60-byte rotated descriptor tail. Modern units start after the database
  # header/directory image omitted from section 3, num_directory*16 + 48 bytes.
  decal_terminal_slot_array:
    seq:
      - id: records
        type:
          switch-on: _root.version <= 0x2019
          cases:
            true: decal_terminal_slot_v20
            false: decal_terminal_slot_v36
        repeat: eos

  decal_terminal_slot_v36:
    seq:
      - id: x_or_controller_index
        type: s4
        doc: '+0 pin X in terminal units; controller index in suffix units'
      - id: y_or_controller_flags
        type: s4
        doc: '+4 pin Y in terminal units; controller flags in suffix units'
      - id: name_x_or_object_index
        type: s4
        doc: '+8 NMXLOC in terminal units; object index in suffix units'
      - id: name_y_or_object_state
        type: s4
        doc: '+12 NMYLOC in terminal units; object state in suffix units'
      - id: padstack_or_object_handle
        type: u4
        doc: '+16 padstack pointer in terminal units; object handle in suffix units'
      - id: pin_name_or_controller_cache
        size: 4
        doc: '+20 stale pin-name bytes in terminal units; controller cache in suffix units'
      - id: state0
        type: s4
        doc: '+24 zero in terminal units; mixed controller state in suffix units'
      - id: state1
        type: s4
        doc: '+28 zero in terminal units; mixed controller state in suffix units'
      - id: state2
        type: s4
        doc: '+32 zero in terminal units; mixed controller state in suffix units'

  decal_terminal_slot_v20:
    seq:
      - id: padstack_or_object_handle
        type: u4
        doc: '+0 padstack pointer in terminal units; object handle in suffix units'
      - id: x_or_controller_word0
        type: s4
        doc: '+4 pin X in terminal units; controller word in suffix units'
      - id: y_or_controller_word1
        type: s4
        doc: '+8 pin Y in terminal units; controller word in suffix units'
      - id: name_x_or_object_index
        type: s4
        doc: '+12 NMXLOC in terminal units; object index in suffix units'
      - id: name_y_or_object_state
        type: s4
        doc: '+16 NMYLOC in terminal units; object state in suffix units'

  # =========================================================================
  # SECTION 16 — PARTTYPE auxiliary index/state table
  # =========================================================================
  # The entire declared payload is count x 8 bytes (4,088 records on 90 files).
  # The former 224-byte spilled-PARTTYPE interpretation was measured at the raw
  # directory-overdeclared offset and crossed into section 17.
  parttype_aux_array:
    seq:
      - id: records
        type: parttype_aux_record
        repeat: eos

  parttype_aux_record:
    seq:
      - id: object_index_or_handle
        type: u4
        doc: PARTTYPE/decal object ordinal; saved object handle in allocator-state entries
      - id: state_or_ordinal
        type: u4
        doc: auxiliary PARTTYPE state, count, or ordinal; zero in 17,733 records

  # =========================================================================
  # SECTION 17 — PARTTYPE definitions
  # =========================================================================
  # Every directory record is a PARTTYPE definition in declaration order. Name
  # storage is at +44 in both dialects. Modern records are 224 bytes; v2017–22
  # use a 208-byte prefix. The second 112 bytes are retained object capacity,
  # often 0xFF-filled but sometimes containing live duplicated indexes/state.
  parttype_record_array:
    seq:
      - id: modern_records
        type: parttype_record_v224
        repeat: eos
        if: _root.directory[17].total_bytes / _root.directory[17].count == 224
      - id: legacy_records
        type: parttype_record_v208
        repeat: eos
        if: _root.directory[17].total_bytes / _root.directory[17].count == 208

  parttype_record_v224:
    seq:
      - id: parttype_controller_state
        size: 44
        doc: '+0..+43 terminal cursors, object ordinals, handles, and serialization state'
      - id: name_storage
        type: str
        size: 36
        encoding: ASCII
        doc: '+44 PARTTYPE NAME in declaration order; NUL-padded unless all 36 bytes are used'
      - id: parttype_link_state
        size: 32
        doc: '+80..+111 decal selection, heap links, and duplicated definition indexes'
      - id: retained_object_capacity
        size: 112
        doc: '+112..+223 retained PARTTYPE object capacity; live state, not file padding'

  parttype_record_v208:
    seq:
      - id: parttype_controller_state
        size: 44
      - id: name_storage
        type: str
        size: 36
        encoding: ASCII
        doc: '+44 legacy PARTTYPE NAME in declaration order'
      - id: parttype_link_state
        size: 16
        doc: '+80..+95 legacy decal selection and definition links'
      - id: retained_object_capacity
        size: 112
        doc: '+96..+207 retained legacy PARTTYPE object capacity; not file padding'

  # =========================================================================
  # SECTION 18 — final PARTTYPE metadata and gate-definition stream
  # =========================================================================
  # The nominal section boundary lands at the start of the final rotated
  # PARTTYPE metadata. Its 44-byte prefix is followed by one 8-byte gate record
  # per directory[18] item. The gate array therefore ends exactly 44 bytes into
  # nominal section 19 on all 90 corpus files.
  parttype_final_metadata:
    seq:
      - id: obj_ordinal
        type: s4
        doc: ordinal of the final PARTTYPE
      - id: pin_cursor
        type: s4
        doc: first section-19 pin ordinal for the final PARTTYPE; -1 when it has no pins
      - id: prior_slot_state0
        size: 8
      - id: flags
        type: u1
        doc: ASCII PARTTYPE FLAGS for the final definition
      - id: part_type
        type: str
        encoding: ASCII
        size: 3
        doc: ASCII PARTTYPE TYPE for the final definition, such as UND, CON, JUM, CAP, or RES
      - id: gates
        type: s4
        doc: ASCII PARTTYPE GATES for the final definition
      - id: prior_slot_state1
        size: 12
      - id: obj_handle
        type: u4
      - id: state_flag
        type: u4

  parttype_gate_array:
    seq:
      - id: records
        type: parttype_gate_record
        repeat: eos

  parttype_gate_record:
    seq:
      - id: pin_count
        type: u4
        doc: ASCII `G` declaration pin count; 0..400 in the corpus
      - id: swap_type_flags
        type: u4
        doc: ASCII gate swap type on modern paired exports; legacy files also use values 2, 3, and 0xFFFFFF00

  # =========================================================================
  # SECTION 19 — phase-shifted PARTTYPE pin-definition table
  # =========================================================================
  # Records begin at nominal section19 +44, immediately after the section-18 gate
  # stream. The prior model began at the directory boundary and consequently
  # misidentified the second half of each record as its active data. Record
  # strides are 24 (v2017), 60 (v2019), 69 (v2021/22), and 88 (v2024+).
  parttype_pin_array:
    seq:
      - id: v88_records
        type: parttype_pin_v88
        repeat: eos
        if: _root.directory[19].total_bytes / _root.directory[19].count == 88
      - id: v69_records
        type: parttype_pin_v69
        repeat: eos
        if: _root.directory[19].total_bytes / _root.directory[19].count == 69
      - id: v60_records
        type: parttype_pin_v60
        repeat: eos
        if: _root.directory[19].total_bytes / _root.directory[19].count == 60
      - id: v24_records
        type: parttype_pin_v24
        repeat: eos
        if: _root.directory[19].total_bytes / _root.directory[19].count == 24

  parttype_pin_v88:
    seq:
      - id: pin_state
        type: u1
        doc: pin type/name state; commonly 0 or 1
      - id: pin_id
        type: strz
        encoding: ASCII
        size: 17
        doc: SWAPTYPE letter plus pin number, such as U1, L2, P8, G12, or A1
      - id: pin_name_storage
        type: strz
        encoding: ASCII
        size: 70
        doc: active pin/signal name followed by retained fixed-slot capacity; the retained bytes are not file padding

  parttype_pin_v69:
    seq:
      - id: pin_state
        type: u1
      - id: pin_id
        type: strz
        encoding: ASCII
        size: 17
      - id: pin_name_storage
        type: strz
        encoding: ASCII
        size: 51
        doc: legacy pin-name buffer and retained slot capacity

  parttype_pin_v60:
    seq:
      - id: pin_number
        type: u4
      - id: pin_state
        type: u1
      - id: pin_id
        type: strz
        encoding: ASCII
        size: 11
      - id: pin_name_storage
        type: strz
        encoding: ASCII
        size: 44
        doc: v2019 pin-name buffer and retained slot capacity

  parttype_pin_v24:
    seq:
      - id: pin_number
        type: u4
      - id: pin_state
        type: u1
      - id: swap_type
        type: u1
        doc: ASCII swap-type character in populated v2017 records
      - id: pin_name_storage
        type: strz
        encoding: ASCII
        size: 18
        doc: v2017 pin-name buffer and retained slot capacity

  # =========================================================================
  # SECTIONS 20 AND 21 — PARTTYPE SIGPIN mappings and compact pin names
  # =========================================================================
  # Section 20 serializes ASCII SIGPIN declarations. Its modern records match
  # four paired ASCII exports exactly. Section 21 stores compact pin names used
  # by the pin-name continuation lines of PARTTYPE declarations. Both arrays are
  # rotated 44 bytes beyond their nominal directory boundaries; legacy SIGPIN
  # records have one additional four-byte lead-in. Fixed storage after a NUL can
  # contain retained/live bytes from the overlapping next logical view and is
  # therefore capacity, never padding.
  parttype_sigpin_array:
    seq:
      - id: modern_records
        type: parttype_sigpin_v72
        repeat: eos
        if: _root.directory[20].total_bytes / _root.directory[20].count == 72
      - id: legacy_records
        type: parttype_sigpin_v56
        repeat: eos
        if: _root.directory[20].total_bytes / _root.directory[20].count == 56

  parttype_sigpin_v72:
    seq:
      - id: pin_id_storage
        type: strz
        encoding: ASCII
        size: 16
        doc: SIGPIN pin identifier and retained fixed-slot capacity
      - id: signal_name_storage
        type: strz
        encoding: ASCII
        size: 56
        doc: SIGPIN signal name and retained fixed-slot capacity

  parttype_sigpin_v56:
    seq:
      - id: pin_ordinal
        type: u4
        doc: numeric SIGPIN pin identifier in legacy files
      - id: signal_name_storage
        type: strz
        encoding: ASCII
        size: 52
        doc: legacy SIGPIN signal name and retained fixed-slot capacity

  compact_parttype_pin_name_array:
    seq:
      - id: modern_records
        type: compact_parttype_pin_name_v16
        repeat: eos
        if: _root.directory[21].total_bytes / _root.directory[21].count == 16
      - id: legacy_records
        type: compact_parttype_pin_name_v8
        repeat: eos
        if: _root.directory[21].total_bytes / _root.directory[21].count == 8

  compact_parttype_pin_name_v16:
    seq:
      - id: pin_name_storage
        type: strz
        encoding: ASCII
        size: 16
        doc: compact PARTTYPE pin name and retained fixed-slot capacity

  compact_parttype_pin_name_v8:
    seq:
      - id: pin_name_storage
        type: strz
        encoding: ASCII
        size: 8
        doc: legacy compact PARTTYPE pin name and retained fixed-slot capacity

  # =========================================================================
  # SECTION 22 — part placements
  # =========================================================================
  # Every directory record is a live placement. Correcting section 3's serialized
  # size removes the former apparent 11-record tail.
  # v2021/22 and older use the 96-byte prefix; v2024+ append 16 bytes.
  part_placement_array:
    seq:
      - id: modern_records
        type: part_placement_v112
        repeat: eos
        if: _root.directory[22].total_bytes / _root.directory[22].count == 112
      - id: legacy_records
        type: part_placement_v96
        repeat: eos
        if: _root.directory[22].total_bytes / _root.directory[22].count == 96

  part_placement_v112:
    seq:
      - id: marker
        type: s4
        doc: '+0 always 0 for real records (valid marker)'
      - id: decal_index
        type: s4
        doc: '+4 decal/footprint pool index (shared by same-footprint parts)'
      - id: decal_index2
        type: s4
        doc: '+8 secondary geometry/decal index'
      - id: instance_index
        type: s4
        doc: '+12 near-unique per-instance index'
      - id: flags16
        type: s4
        doc: '+16 mostly 32; side/flags enum, sometimes a handle'
      - id: object_id
        type: s4
        doc: '+20 global object id (shared sequence)'
      - id: sentinel_m1
        type: s4
        doc: '+24 -1'
      - id: link0
        type: s4
        doc: '+28 -1 or 0'
      - id: link1
        type: s4
        doc: '+32 -1 or 0'
      - id: handle_lo
        type: u4
        doc: '+36 per-instance handle low word (handle)'
      - id: handle_hi
        type: u4
        doc: '+40 0x80000000 or 0 (tag/high word of handle)'
      - id: refdes
        type: strz
        encoding: ASCII
        size: 16
        doc: '+44 REFDES, inline NUL-terminated ASCII'
      - id: x_raw
        type: s4
        doc: '+60 placement X = design_x + origin_x'
      - id: y_raw
        type: s4
        doc: '+64 placement Y = design_y + origin_y'
      - id: orientation
        type: s4
        doc: '+68 orientation = degrees * 1800000'
      - id: mirror
        type: s4
        doc: '+72 bit0: 1 = mirrored (bottom side)'
      - id: bbox_xa
        type: s4
        doc: '+76 bounding-box X (a) RAW'
      - id: bbox_ya
        type: s4
        doc: '+80 bounding-box Y (a) RAW'
      - id: bbox_xb
        type: s4
        doc: '+84 bounding-box X (b) RAW'
      - id: bbox_yb
        type: s4
        doc: '+88 bounding-box Y (b) RAW'
      - id: const_fffe
        type: s4
        doc: '+92 0xFFFE in simple files'
      - id: instance_val
        type: s4
        doc: '+96 signed small per-instance value'
      - id: handle2
        type: u4
        doc: '+100 per-instance serial/handle (handle)'
      - id: zero
        type: s4
        doc: '+104 0'
      - id: cluster_id
        type: s4
        doc: |
          +108 1-based CLSTID into the cluster table (sec64/sec69 tail, see
          cluster_record). -1 = part is in no cluster. New 112 B layout only;
          the old 96 B layout has no room for this field.

  part_placement_v96:
    seq:
      - id: marker
        type: s4
      - id: decal_index
        type: s4
      - id: decal_index2
        type: s4
      - id: instance_index
        type: s4
      - id: flags16
        type: s4
      - id: object_id
        type: s4
      - id: sentinel_m1
        type: s4
      - id: link0
        type: s4
      - id: link1
        type: s4
      - id: handle_lo
        type: u4
      - id: handle_hi
        type: u4
      - id: refdes
        type: strz
        encoding: ASCII
        size: 16
      - id: x_raw
        type: s4
      - id: y_raw
        type: s4
      - id: orientation
        type: s4
      - id: mirror
        type: s4
      - id: bbox_xa
        type: s4
      - id: bbox_ya
        type: s4
      - id: bbox_xb
        type: s4
      - id: bbox_yb
        type: s4
      - id: const_fffe
        type: s4

  # =========================================================================
  # CLUSTER TABLE — part clusters (.asc *CLUSTER* groups)
  # =========================================================================
  # Section 68 is exactly 60 B/record and ends 12 bytes before section 69's
  # layer-record array. Records are in
  # .asc *CLUSTER* order. The stored +0 id and the record's 1-based ordinal both
  # equal the CLSTID that the sec22 placement +108 field references. The cluster
  # NAME is also duplicated as
  # a decoy NUL-separated run in the sec8 string pool; the real table is the one
  # whose +20/+24 decode as valid RAW coords (the decoy has garbage there).
  cluster_record:
    seq:
      - id: cluster_id
        type: u4
        doc: '+0 stored 1-based CLSTID; equals this record ordinal plus one'
      - id: name
        type: strz
        encoding: ASCII
        size: 16
        doc: '+4 cluster NAME, NUL-padded'
      - id: x_raw
        type: s4
        doc: '+20 XLOC RAW; design = raw - origin_x (BASIC = 1/38100 mil)'
      - id: y_raw
        type: s4
        doc: '+24 YLOC RAW; design = raw - origin_y'
      - id: retained_orientation
        type: s4
        doc: '+28 retained cluster orientation in PADS angular units; not emitted by ASCII export'
      - id: attribute
        type: s4
        doc: '+32 low 16 bits = .asc ATTRIBUTE'
      - id: retained_group_state
        type: s4
        doc: '+36 saved group state; 0 or 1 in the scoped corpus'
      - id: null_relationship_handle
        type: s4
        doc: '+40 null relationship handle; zero throughout the scoped corpus'
      - id: child_list_handle
        type: u4
        doc: '+44 tagged child-list handle; low word agrees with .asc CHILD_NUM'
      - id: null_previous_handle
        type: s4
        doc: '+48 null previous-link handle; zero throughout the scoped corpus'
      - id: null_next_handle
        type: s4
        doc: '+52 null next-link handle; zero throughout the scoped corpus'
      - id: object_handle
        type: u4
        doc: '+56 tagged cluster object handle'

  # =========================================================================
  # SECTION 23 — net records
  # =========================================================================
  # Names are at +76, not +116. The former layout was shifted 40 bytes and made
  # valid records after the unassigned-obstacles sentinel look inactive. Paired
  # exports contain every named net once plus one sentinel on modern boards.
  # v2017..22 use 144 bytes, v2024 uses 416, and v2025+ use 424.
  net_record_array:
    seq:
      - id: v424_records
        type: net_record_v424
        repeat: eos
        if: _root.directory[23].total_bytes / _root.directory[23].count == 424
      - id: v416_records
        type: net_record_v416
        repeat: eos
        if: _root.directory[23].total_bytes / _root.directory[23].count == 416
      - id: v144_records
        type: net_record_v144
        repeat: eos
        if: _root.directory[23].total_bytes / _root.directory[23].count == 144

  net_record_v144:
    seq:
      - id: net_controller_state0
        size: 52
        doc: '+0..+51 serialized net-controller state'
      - id: plane_index
        type: s4
        doc: '+52 -1 normal signal; >=1 1-based plane-assignment index'
      - id: sig_flag
        type: s4
        doc: '+56 raw PADS SIGFLAG'
      - id: conn_count
        type: s4
        doc: '+60 number of connections (= section-24 entries for this net)'
      - id: anchor_part_idx
        type: s4
        doc: '+64 0-based index into section 22 of a member part'
      - id: anchor_pin
        type: s4
        doc: '+68 terminal/pin number on anchor part'
      - id: sec24_start
        type: s4
        doc: '+72 cumulative start index into section-24 chain topology'
      - id: name
        type: strz
        encoding: ASCII
        size: 48
        doc: '+76 net name, NUL-terminated'
      - id: ser_index
        type: s4
        doc: '+124 serialized object index/handle'
      - id: net_controller_state1
        type: s4
        doc: '+128 net serialization state'
      - id: ser_size_used
        type: s4
        doc: '+132 serialized byte-size of connection/route data'
      - id: ser_size_cap
        type: s4
        doc: '+136 allocation capacity'
      - id: net_controller_state2
        type: s4
        doc: '+140 net serialization state'

  net_record_v416:
    seq:
      - id: base
        type: net_record_v144
      - id: net_self_ptr
        type: u4
        doc: >
          +144 the net object's own in-file CObject id. Stable within one file;
          used as the diff-pair member
          key: a sec49 DIF_PAIR object's member-net ptrs (+12/+16) value-equal this.
      - id: netclass_owner_ptr
        type: u4
        doc: >
          +148 the net's NET_CLASS owner object id. This is the membership key: all nets of a
          class share this value; 0 = unclassed. Grouping nets by it reproduces
          the ASC NET_CLASS membership exactly (126/126, 8/8, 1/1 across the
          corpus). Ascending distinct values == net-class declaration order ==
          the 280-byte NAME-table file order, so class_ordinal = rank(this).
          Value-joinable within one file; never dereferenced.
      - id: heap_ptr1
        type: s4
        doc: '+152 serialized heap pointer/state'
      - id: conn_count_dup
        type: s4
        doc: '+156 duplicate of connection count'
      - id: retained_object_capacity
        size: 256
        doc: '+160..+415 retained serialized-object capacity and controller state; not file padding'

  net_record_v424:
    seq:
      - id: base
        type: net_record_v144
      - id: net_self_ptr
        type: u4
      - id: netclass_owner_ptr
        type: u4
      - id: heap_ptr1
        type: s4
      - id: conn_count_dup
        type: s4
      - id: retained_object_capacity
        size: 264
        doc: '+160..+423 retained serialized-object capacity and controller state; not file padding'

  # =========================================================================
  # SECTION 24 — route chain / pin-pair connection topology
  # =========================================================================
  # Every directory record is live. Sum(net.conn_count) equals section-24 count
  # on paired exports. Record zero is a zero-initialized topology root; remaining
  # records carry a 0xFE high-byte marker plus low flag bits at +28, and
  # 0x0000FFFE at +60. The previous +20/+52
  # layout was shifted eight bytes and created a false 17-record slack tail.
  route_chain_array:
    seq:
      - id: records
        type: route_chain_record
        repeat: eos

  route_chain_record:
    seq:
      - id: node_a
        type: s4
        doc: '+0 topology node index (endpoint A)'
      - id: node_b
        type: s4
        doc: '+4 topology node index (endpoint B)'
      - id: link_c
        type: s4
        doc: '+8 linked node index (next/alt traversal)'
      - id: link_d
        type: s4
        doc: '+12 linked node index'
      - id: link_next
        type: s4
        doc: '+16 linked node index (unique per record = next pointer)'
      - id: route_chain_state0
        size: 8
        doc: '+20..+27 route-chain link/controller state'
      - id: marker
        type: u4
        doc: '+28 topology-root state on record zero; later records have high byte 0xFE and low route flags'
      - id: route_chain_state1
        size: 12
        doc: '+32..+43 route-chain controller state'
      - id: end_x1
        type: s4
        doc: '+44 optional RAW endpoint X (else 0)'
      - id: end_y1
        type: s4
        doc: '+48 optional RAW endpoint Y (else 0)'
      - id: end_x2
        type: s4
        doc: '+52 optional RAW endpoint X (else 0)'
      - id: end_y2
        type: s4
        doc: '+56 optional RAW endpoint Y (else 0)'
      - id: flag_fffe
        type: u4
        doc: '+60 == 0x0000FFFE'
      - id: route_chain_state2
        type: s4
        doc: '+64 route-chain controller state'

  # =========================================================================
  # SECTIONS 25–29 — rotated route-object allocator tables
  # =========================================================================
  # These tables share the same 44-byte rotation as the PARTTYPE arrays. The
  # section-25 controller consumes its declared 280/288 bytes plus the first 44
  # nominal bytes of section 26. Each following logical array begins at its
  # nominal directory boundary +44 and ends +44 into the next non-empty entry.
  # Sections 28 and 30..40 are empty throughout the corpus, so the section-29
  # handle vector ends 44 bytes into the nominal section-41 region.
  route_allocator_controller:
    doc: |
      Route-object allocator/controller state. Serialized size is 324 bytes in
      the 34 legacy-layout files and 332 bytes in the 563 modern-layout files.
      These are live controller words, not padding: the leading counters and
      flags vary with route-object population and retained allocator state.
    seq:
      - id: controller_state_words
        type: u4
        repeat: eos

  route_object_range_array:
    seq:
      - id: records
        type: route_object_range
        repeat: eos

  route_object_range:
    doc: 12-byte allocator range describing a span of route objects
    seq:
      - id: allocation_begin
        type: u4
        doc: saved process address of the first object in this allocator span
      - id: allocation_end
        type: u4
        doc: saved process address immediately after the allocator span
      - id: range_state
        type: u4
        doc: small live-count/type state; observed range 0..0xFFFF

  route_layer_object_count_array:
    doc: |
      One u32 route-object count per copper layer. The directory count is the
      number of copper layers (2, 4, 6, 8, 10, or 12 in this corpus), and the
      sum of these values equals directory[29].count on all 90 files.
    seq:
      - id: object_counts
        type: u4
        repeat: eos

  route_object_handle_array:
    doc: |
      Flat route-object handle vector. Handles are saved process addresses,
      commonly 8-byte aligned on the modern allocator and spaced on a 56-byte
      object grid. Partition the vector into copper-layer groups using the
      section-27 counts. The former fixed 297-word template tail was a phase
      error caused by reading at the raw, directory-overdeclared offset.
    seq:
      - id: object_handles
        type: u4
        repeat: eos
        doc: saved route-object process address; never a file offset

  # =========================================================================
  # SECTION 41 UNDECLARED PREFIX — design-rule controller page
  # =========================================================================
  # The section-41 directory entry is a presence/state flag, not this page's
  # serialized length. The page begins at the nominal section-41 boundary. Its
  # clearance framing is 48 + N*180 or 48 + N*188 bytes. Optional legacy
  # layer-clearance and relationship records precede HIGH_SPEED_RULE. Compact
  # per-layer matrices and ROUTE_RULE records follow; the surrounding stream
  # supplies their counts before reaching the DIF_PAIR array and section 49.
  section41_design_rule_stream:
    params:
      - id: num_clearance_records
        type: u4
      - id: clearance_record_size
        type: u4
      - id: num_layer_clearance_records
        type: u4
      - id: num_rule_relation_prefixes
        type: u4
      - id: num_high_speed_records
        type: u4
      - id: num_per_layer_rule_matrices
        type: u4
      - id: num_layers
        type: u4
      - id: num_route_records
        type: u4
      - id: route_record_size
        type: u4
      - id: num_diff_pair_records
        type: u4
      - id: num_section49_prefix_words
        type: u4
    seq:
      - id: rule_page
        type: section41_rule_page(num_clearance_records, clearance_record_size)
      - id: layer_clearance_records
        type: section41_legacy_layer_clearance_record
        repeat: expr
        repeat-expr: num_layer_clearance_records
      - id: rule_relation_prefixes
        type: section41_rule_relation_prefix
        repeat: expr
        repeat-expr: num_rule_relation_prefixes
      - id: high_speed_records
        type: section41_high_speed_rule_record
        repeat: expr
        repeat-expr: num_high_speed_records
      - id: per_layer_rule_matrices
        type: section41_per_layer_rule_matrix(num_layers)
        repeat: expr
        repeat-expr: num_per_layer_rule_matrices
        doc: retained layer-sized design-rule matrices; BR430 stores one and Si534x stores two
      - id: route_records
        type:
          switch-on: route_record_size
          cases:
            32: section41_route_rule_record_v32
            40: section41_route_rule_record
        repeat: expr
        repeat-expr: num_route_records
      - id: diff_pair_records
        type: section41_diff_pair_record
        repeat: expr
        repeat-expr: num_diff_pair_records
      - id: section49_prefix
        type: section49_route_rule_prefix(num_section49_prefix_words)

  section41_rule_page:
    params:
      - id: num_clearance_records
        type: u4
      - id: clearance_record_size
        type: u4
    seq:
      - id: controller_header
        type: section41_rule_page_header
      - id: clearance_records
        type:
          switch-on: clearance_record_size
          cases:
            180: section41_clearance_record_v180
            188: section41_clearance_record
        repeat: expr
        repeat-expr: num_clearance_records

  section41_per_layer_rule_matrix:
    params:
      - id: num_values
        type: u4
    seq:
      - id: rule_selector_or_handle
        type: u4
        doc: layer/rule selector in compact records; saved process-local rule handle in expanded records
      - id: clearance_values
        type: s4
        repeat: expr
        repeat-expr: num_values - 2
        doc: signed BASIC-unit clearance values; -1 means inherited/not applicable
      - id: rule_state
        type: u4
        doc: compact rule ownership/controller state

  section41_rule_page_header:
    seq:
      - id: saved_list_heads
        type: u4
        repeat: expr
        repeat-expr: 11
        doc: saved process-local heads for design-rule category lists
      - id: controller_state
        type: u4
        doc: design-rule page controller state; zero in the common layout

  section41_clearance_record:
    doc: >
      188-byte clearance rule. The first 38 values are common. Word +160 is the
      39th clearance value when nonnegative, or the first ownership-metadata word
      when negative; DC2317A demonstrates the latter dialect. The remaining
      24-byte metadata tail is common, so both layouts are reachable without an
      impossible size-based switch between two 188-byte types.
    seq:
      - id: saved_rule_handle
        type: u4
        doc: process-local rule relationship handle; never a file offset
      - id: rule_state
        type: u4
        doc: rule object state
      - id: clearance_values
        type: s4
        repeat: expr
        repeat-expr: 38
        doc: first 38 CLEARANCE_RULE values in ASCII declaration order and BASIC units
      - id: final_value_or_metadata
        type: s4
        doc: nonnegative 39th clearance value, or negative first ownership-metadata word in the 38-value dialect
      - id: rule_metadata_tail
        size: 24
        doc: common live rule ownership, width, and controller metadata; not padding

  section41_clearance_record_v180:
    doc: 180-byte v0x2017/v0x2019 clearance rule with 39 values and a 16-byte ownership trailer
    seq:
      - id: saved_rule_handle
        type: u4
      - id: rule_state
        type: u4
      - id: clearance_values
        type: s4
        repeat: expr
        repeat-expr: 39
        doc: CLEARANCE_RULE values in BASIC units
      - id: rule_metadata
        size: 16
        doc: legacy ownership and controller metadata

  section41_legacy_layer_clearance_record:
    doc: 188-byte v0x2019 layer/relationship-specific clearance rule
    seq:
      - id: layer_rule_selector
        type: u4
      - id: recommended_track_width
        type: s4
      - id: saved_owner_handle
        type: u4
      - id: relationship_state
        type: u4
      - id: saved_rule_handle
        type: u4
      - id: rule_state
        type: u4
      - id: clearance_values
        type: s4
        repeat: expr
        repeat-expr: 39
        doc: CLEARANCE_RULE values in BASIC units
      - id: rule_metadata
        size: 8
        doc: legacy layer-rule ownership/controller metadata

  section41_rule_relation_prefix:
    doc: 16-byte relationship/layer selector prefix attached to a following rule object
    seq:
      - id: layer_rule_selector
        type: u4
      - id: recommended_track_width
        type: s4
      - id: saved_owner_handle
        type: u4
      - id: relationship_state
        type: u4

  section41_high_speed_rule_record:
    doc: 80-byte HIGH_SPEED_RULE object; numeric fields serialize in ASCII declaration order
    seq:
      - id: saved_rule_handle
        type: u4
        doc: process-local rule handle; never a file offset
      - id: min_length
        type: f8
        doc: ASCII MIN_LENGTH in BASIC units
      - id: max_length
        type: f8
        doc: ASCII MAX_LENGTH in BASIC units
      - id: stub_length
        type: s4
        doc: ASCII STUB_LENGTH
      - id: parallel_length
        type: s4
        doc: ASCII PARALLEL_LENGTH
      - id: parallel_gap
        type: s4
        doc: ASCII PARALLEL_GAP
      - id: tandem_length
        type: s4
        doc: ASCII TANDEM_LENGTH
      - id: tandem_gap
        type: s4
        doc: ASCII TANDEM_GAP
      - id: min_delay
        type: f4
        doc: ASCII MIN_DELAY
      - id: max_delay
        type: f4
        doc: ASCII MAX_DELAY
      - id: min_capacitance
        type: f4
        doc: ASCII MIN_CAPACITANCE
      - id: max_capacitance
        type: f4
        doc: ASCII MAX_CAPACITANCE
      - id: min_impedance
        type: f4
        doc: ASCII MIN_IMPEDANCE
      - id: max_impedance
        type: f4
        doc: ASCII MAX_IMPEDANCE
      - id: shield_gap
        type: s4
        doc: ASCII SHIELD_GAP
      - id: match_length_tolerance
        type: s4
        doc: ASCII MATCH_LENGTH_TOLERANCE
      - id: rule_flags
        type: u4
        doc: shield-net and match-length option state
      - id: rule_state
        type: u4
        doc: saved rule/controller state

  section41_route_rule_record:
    doc: 40-byte ROUTE_RULE object; option and layer masks correspond to the ASCII route-rule declarations
    seq:
      - id: saved_rule_handle
        type: u4
        doc: process-local rule handle; never a file offset
      - id: saved_via_type_set_handle
        type: u4
        doc: process-local handle for the VALID_VIA_TYPE set
      - id: length_minimization_type
        type: u4
        doc: ASCII LENGTH_MINIMIZATION_TYPE
      - id: route_priority
        type: u4
        doc: ASCII ROUTE_PRIORITY
      - id: route_option_flags
        type: u4
        doc: VIA_SHARE, TRACE_SHARE, AUTO_ROUTE, RIPUP, and SHOVE state
      - id: valid_layer_mask
        type: u4
        doc: bit mask formed by the ASCII VALID_LAYER declarations
      - id: via_type_set_state
        type: u4
        doc: VALID_VIA_TYPE set controller state
      - id: max_number_of_vias
        type: s4
        doc: ASCII MAX_NUMBER_OF_VIAS; -1 means unlimited
      - id: rule_flags
        type: u4
      - id: rule_state
        type: u4

  section41_route_rule_record_v32:
    doc: 32-byte v0x2017/v0x2019 ROUTE_RULE object
    seq:
      - id: saved_rule_handle
        type: u4
      - id: saved_via_type_set_handle
        type: u4
      - id: length_minimization_type
        type: u4
      - id: route_priority
        type: u4
      - id: route_option_flags
        type: u4
      - id: valid_layer_mask
        type: u4
      - id: via_type_set_state
        type: u4
      - id: max_number_of_vias_or_rule_state
        type: s4
        doc: legacy combined maximum-via/controller state word

  section41_diff_pair_record:
    doc: 864-byte DIF_PAIR rule object; paired ASCII MAX_LENGTH, GAP, obstacle limits, and WIDTH match exactly
    seq:
      - id: saved_member_handle_a
        type: u4
        doc: saved process-local relationship handle for the first member net
      - id: saved_member_handle_b
        type: u4
        doc: saved process-local relationship handle for the second member net
      - id: relationship_state
        size: 12
        doc: member-net relationship/controller state
      - id: rule_matrix
        type: f8
        repeat: expr
        repeat-expr: 69
        doc: diff-pair rule values; element 0 is MAX_LENGTH and element 1 is GAP in paired exports; -1.0 means inherit
      - id: max_obstacle_size
        type: s4
        doc: ASCII MAX_OBSTACLE_SIZE
      - id: max_obstacle_number
        type: s4
        doc: ASCII MAX_OBSTACLE_NUMBER
      - id: width
        type: s4
        doc: ASCII WIDTH
      - id: retained_allocator_capacity
        size: 268
        doc: allocator capacity, mostly 0xff free bytes but sometimes retaining live rule values; never file padding
      - id: ownership_handle
        type: u4
        doc: saved owner relationship handle
      - id: ownership_state
        type: u4
      - id: object_reference_index
        type: u4
        doc: reference index in the owning design-rule object table

  # =========================================================================
  # SECTION 49 — route-object relationship stream
  # =========================================================================
  # At the corrected late-section phase, every populated payload is a sequence
  # of four-byte storage words. The high byte tags a section-24 route-chain
  # ordinal (0x18), a section-60 route-junction ordinal (0x3c), a literal/count
  # (0x00), or a signed sentinel (0xff). The 188-byte clearance and 864-byte
  # diff-pair interpretations were measurements at the wrong accumulated phase.
  # One legacy-layout payload contains two allocator-state words whose high
  # bytes are 0x0a and 0x0d; they remain live word state, not padding.
  # Some route-rule controller dialects precede the directory-counted payload
  # with tagged storage words. BR420/430/460 store route_count-1 words; DC2100
  # stores none despite two route records, so the prefix count is explicit.
  section49_route_rule_prefix:
    params:
      - id: num_words
        type: u4
    seq:
      - id: words
        type: section49_storage_word
        repeat: expr
        repeat-expr: num_words

  section49_storage_array:
    seq:
      - id: words
        type: section49_storage_word
        repeat: eos

  section49_storage_word:
    seq:
      - id: payload_low
        type: u2
        doc: low 16 bits of the relationship ordinal, literal, or signed state
      - id: payload_mid
        type: u1
        doc: bits 16..23 of the relationship ordinal, literal, or signed state
      - id: tag_or_high_state
        type: u1
        doc: '0x00 literal/count | 0x18 section-24 ordinal | 0x3c section-60 ordinal | 0xff signed sentinel; 0x0a/0x0d are legacy allocator high bytes'

  active_layer_ordinal_vector:
    params:
      - id: num_layers
        type: u4
    doc: u16 ordinal vector following section 49; values are exactly 0 through num_layers-1
    seq:
      - id: layer_ordinals
        type: u2
        repeat: expr
        repeat-expr: num_layers

  # =========================================================================
  # NET-CLASS DEFINITIONS (trailing-heap, design-rule graph)
  # =========================================================================
  # The trailing region is a CONTIGUOUSLY-serialized object arena (an MFC heap
  # snapshot), so the "pointer" values are arena OFFSETS, not heap addresses:
  #   file_offset = pointer - K
  #   K = min(distinct nonzero sec23 netclass_owner_ptr) - net_class_name_table_head
  # K is per-file, byte-exact (a K+-1 control collapses the cross-links to 0). The
  # famous sec49 "idx30" pointer (the prior "non-portable heap pointer") rebases
  # cleanly with this K. Bands are file-specific (detect the class band empirically
  # as the distinct nonzero sec23 netclass_owner_ptr values; do NOT hardcode 0x1e20).
  # Net->class MEMBERSHIP and rule->class+layer binding need NO rebase: they are
  # value-equality joins on the netclass_owner_ptr (sec23 +188 == rule table +8).
  net_class_name_record:
    doc: >
      280-byte (0x118 stride) net-class NAME record. Emitted in NET_CLASS
      declaration order. These records occupy the variable allocator span before
      the layer table; paired ASCII files account for that span as one 280-byte
      record per NET_CLASS, plus controller framing.
    seq:
      - id: saved_class_handle
        type: u4
        doc: '+0 process-local net-class object handle; never a file offset'
      - id: name
        type: strz
        encoding: ASCII
        size: 40
        doc: '+4 net-class name (ETH_RGMII_TX, POWER_SIGNALS, GROUND, ...)'
      - id: zero_membership_slots
        type: u4
        valid: 0
        repeat: expr
        repeat-expr: 56
        doc: '+44..+267 cleared retained membership capacity'
      - id: saved_controller_handle
        type: u4
        doc: '+268 saved per-class controller handle'
      - id: controller_state
        type: u4
        doc: '+272 0x80000000 when the terminating controller is live, otherwise zero'
      - id: terminal_rule_kind
        type: u4
        doc: '+276 terminating rule kind 0x29 when present, otherwise zero'

  net_class_name_record_v280_header:
    doc: alternate 280-byte net-class object with controller state before the fixed name
    seq:
      - id: retained_archive_state
        type: u4
        repeat: expr
        repeat-expr: 4
        doc: '+0..+15 retained archive/controller state; normally cleared'
      - id: saved_class_handle
        type: u4
        doc: '+16 saved net-class object handle'
      - id: controller_state
        type: u4
        doc: '+20 saved controller state, normally 0x80000000'
      - id: relationship_state
        type: u4
        doc: '+24 saved relationship state, normally 0x80000000'
      - id: saved_name_handle
        type: u4
        doc: '+28 saved handle associated with the inline class name'
      - id: name
        type: strz
        encoding: ASCII
        size: 40
        doc: '+32 fixed-width zero-terminated net-class name'
      - id: zero_membership_slots
        type: u4
        valid: 0
        repeat: expr
        repeat-expr: 52
        doc: '+72..+279 cleared retained membership capacity'

  compact_net_class_name_record_v60:
    doc: compact saved net-class membership/name association used by the 0x2026/0x2027 archive dialect
    seq:
      - id: association_handle
        type: u4
      - id: scope_a_type
        type: u4
      - id: object_handle
        type: u4
      - id: scope_b_type
        type: u4
      - id: scope_b_reference
        type: u4
      - id: layer_or_state
        type: u4
      - id: ordinal
        type: u4
        doc: '+24 one-based name/member ordinal'
      - id: name
        type: strz
        encoding: ASCII
        size: 16
        doc: '+28 fixed-width zero-terminated compact class name'
      - id: saved_controller_handle
        type: u4
      - id: saved_owner_handle
        type: u4
      - id: saved_membership_handle
        type: u4
      - id: capacity_flags
        type: u4
        doc: '+56 retained-capacity flags, 0x100 or 0x400 in the corpus'
  design_rule_relationship_record:
    doc: >
      28-byte design-rule graph edge immediately before the layer table. Scope
      types 3, 0x17, and 0x42 select none/default, NET, and NET_CLASS references;
      their following words are sentinel/reference values, not constant masks.
      The final word is the rule kind. Saved handles are process-local
      relationship identifiers, not file offsets.
    seq:
      - id: rule_detail_handle
        type: u4
        doc: '+0 saved rule-detail relationship handle'
      - id: scope_a_type
        type: u4
        doc: '+4 first scope type'
      - id: scope_a_reference
        type: u4
        doc: '+8 first scope reference or default sentinel'
      - id: scope_b_type
        type: u4
        doc: '+12 second scope type'
      - id: scope_b_reference
        type: u4
        doc: '+16 second scope reference or default sentinel'
      - id: layer_or_state
        type: u4
        doc: '+20 layer selector or relationship state'
      - id: rule_kind
        type: u4
        doc: '+24 design-rule kind enum'

  pre_layer_design_rule_stream:
    params:
      - id: num_alignment_bytes
        type: u4
      - id: num_relationships
        type: u4
      - id: num_trailing_zeros
        type: u4
    doc: controller state and design-rule graph edges immediately preceding the layer table
    seq:
      - id: controller_words
        type: u4
        repeat: expr
        repeat-expr: 8
        doc: retained default-rule handles, kinds, masks, and controller state
      - id: alignment_zeros
        type: u1
        valid: 0
        repeat: expr
        repeat-expr: num_alignment_bytes
        doc: zero byte alignment before the 28-byte relationship array
      - id: relationships
        type: design_rule_relationship_record
        repeat: expr
        repeat-expr: num_relationships
      - id: trailing_zeros
        type: u1
        valid: 0
        repeat: expr
        repeat-expr: num_trailing_zeros
        doc: zero controller tail before the layer-record array

  pre_layer_design_rule_controller_v44:
    doc: 44-byte modern default-rule controller used when no relationship-edge array follows
    seq:
      - id: controller_words
        type: u4
        repeat: expr
        repeat-expr: 11
        doc: rule kinds, saved handles, layer selectors, masks, and controller state

  pre_layer_design_rule_controller_v28:
    doc: 28-byte legacy default-rule controller through database version 0x2022
    seq:
      - id: controller_words
        type: u4
        repeat: expr
        repeat-expr: 7
        doc: legacy rule kinds, saved handles, selectors, and state

  pre_layer_relationship_controller:
    doc: 28-byte saved relationship-list controller preceding graph edges or net-class objects
    seq:
      - id: zero_state
        type: u4
        valid: 0
        repeat: expr
        repeat-expr: 4
      - id: saved_relationship_handle
        type: u4
      - id: controller_state
        type: u4
        doc: zero or 0x80000000
      - id: rule_kind
        type: u4

  pre_layer_saved_relationship_terminator:
    doc: 28-byte terminal saved relationship object; followed by the eight-byte zero controller tail
    seq:
      - id: association_count
        type: u4
        doc: zero or one in the corpus
      - id: zero0
        type: u4
        valid: 0
      - id: saved_object_handle
        type: u4
        doc: saved object identifier with class byte 0x16
      - id: zero_state
        type: u4
        valid: 0
        repeat: expr
        repeat-expr: 2
      - id: saved_scope_handle
        type: u4
        doc: zero or a saved 0x0a-class scope handle
      - id: rule_kind
        type: u4
        valid: 0x86

  pre_layer_retained_zero_relationship_slot:
    doc: retained 28-byte relationship-capacity slot; owned storage, not alignment padding
    seq:
      - id: zero_state
        type: u4
        valid: 0
        repeat: expr
        repeat-expr: 7

  pre_layer_terminal_rule_kind:
    doc: four-byte terminal rule selector in the smallest archive dialect
    seq:
      - id: rule_kind
        type: u4
        doc: 6 or 0x86 in the corpus

  pre_layer_allocator64_stream:
    params:
      - id: num_allocator_records
        type: u4
    doc: >
      Fully framed 64-byte pre-layer allocator dialect: counted allocator
      records, an eight-byte relationship controller prefix, three terminating
      relationship records of kinds 0x2a/0x2e/0x86, and an eight-byte zero tail.
    seq:
      - id: allocator_records
        type: pre_layer_allocator_record_v64
        repeat: expr
        repeat-expr: num_allocator_records
      - id: relationship_prefix
        contents: [0x00, 0x00, 0x01, 0x00, 0x29, 0x00, 0x00, 0x00]
      - id: relationships
        type: design_rule_relationship_record
        repeat: expr
        repeat-expr: 3
      - id: trailing_zeros
        contents: [0, 0, 0, 0, 0, 0, 0, 0]

  pre_layer_allocator_record_v64:
    doc: 64-byte saved design-rule allocator object with process-local handles and exact zero state
    seq:
      - id: saved_object_handle
        type: u4
      - id: saved_owner_handle
        type: u4
      - id: zero_state
        type: u4
        valid: 0
        repeat: expr
        repeat-expr: 4
      - id: class_tag
        type: u4
        valid: 0x16000000
      - id: ordinal
        type: u4
        doc: one-based allocator object ordinal
      - id: object_flags
        type: u4
        doc: 0x00400001 or 0x00480101 in this corpus dialect
      - id: zero_links
        type: u4
        valid: 0
        repeat: expr
        repeat-expr: 5
      - id: object_id
        type: u4
      - id: zero_tail
        type: u4
        valid: 0

  section50_relationship_halfword:
    doc: >
      One u16 directory slice through the four-byte tagged object-relationship
      stream following section 49. Section 50 can start or end in the middle of
      a relationship token; paired halfwords therefore carry either the low
      object identifier or the high tag/state half (including tag 0x3c00).
    seq:
      - id: relationship_token_half
        type: u2

  section51_relationship_word:
    doc: >
      One u32 boundary word in the object-relationship stream between section
      50 and the copper-outline/string-index storage. Present in one corpus file.
    seq:
      - id: relationship_token_word
        type: u4

  section50_relationship_array:
    seq:
      - id: records
        type: section50_relationship_halfword
        repeat: eos

  section51_relationship_array:
    seq:
      - id: records
        type: section51_relationship_word
        repeat: eos

  # =========================================================================
  # SECTIONS 52..55 -- copper-outline owner, piece, vertex and arc arrays
  # =========================================================================
  # The older token-stream interpretation came from reading at accumulated
  # directory offsets. Anchor section 56 at the string pool instead and walk
  # backward by declared sizes. Scoped files resolve to four ordinary, exactly
  # sized arrays except for the sole token-stream variant, which uses
  # legacy_objrel_token instead.
  legacy_objrel_token:
    doc: |
      Four-byte object-relationship token used by the sole section-52 legacy
      variant in the corpus. Tag is the destination section ordinal. This is not
      the normal layout of sections 52..55.
    seq:
      - id: value_lo
        type: u2
        doc: low 16 bits of the object id / literal count
      - id: value_mid
        type: u1
        doc: bits 16..23 (usually 0 for in-range handles)
      - id: tag
        type: u1
        doc: '0x00 literal/count | 0x18 sec24 handle | 0x3c sec60/net handle'

  legacy_objrel_token_array:
    seq:
      - id: records
        type: legacy_objrel_token
        repeat: eos

  sec52_outline_owner_array:
    seq:
      - id: records
        type: sec52_outline_owner
        repeat: eos

  sec53_outline_piece_array:
    seq:
      - id: records
        type: sec53_outline_piece
        repeat: eos

  sec54_outline_vertex_array:
    seq:
      - id: records
        type: sec54_outline_vertex
        repeat: eos

  sec55_outline_arc_array:
    seq:
      - id: records
        type: sec55_outline_arc
        repeat: eos

  sec52_outline_owner:
    doc: |
      88-byte copper-outline owner. Covers POUROUT, HATOUT, VOIDOUT, VIATHERM
      and related ASC objects. The first three fields are cumulative indices into
      sections 53, 54 and 55. Piece and vertex indices remain file-global; an
      allocator-page transition may restart arc_start at zero. Coordinates are RAW.
    seq:
      - id: piece_start
        type: u4
        doc: first section-53 piece ordinal
      - id: vertex_start
        type: u4
        doc: first section-54 vertex ordinal
      - id: arc_start
        type: u4
        doc: first section-55 arc ordinal; may restart at an allocator-page boundary
      - id: relationship_id
        type: s4
        doc: signed outline relationship identifier
      - id: object_handle
        type: s4
        doc: database object handle
      - id: parent_relationship_id
        type: s4
        doc: parent relationship identifier; -1 for a root outline
      - id: location_x
        type: s4
        doc: ASC XLOC in RAW coordinates; zero for child outlines with absolute vertices
      - id: location_y
        type: s4
        doc: ASC YLOC in RAW coordinates; zero for child outlines with absolute vertices
      - id: bbox_x_min
        type: s4
      - id: bbox_y_min
        type: s4
      - id: bbox_x_max
        type: s4
      - id: bbox_y_max
        type: s4
      - id: association_handle
        type: u4
      - id: state_flags
        type: u4
      - id: hatch_parameter
        type: s4
      - id: secondary_association_handle
        type: s4
        doc: -1 when unused
      - id: piece_count
        type: u4
        doc: number of consecutive section-53 pieces owned by this outline
      - id: name_flags
        type: u2
        doc: outline/name state bits; observed values distinguish POUROUT/HATOUT/VOIDOUT states
      - id: name
        type: str
        size: 14
        encoding: ASCII
        doc: NUL-padded POR... or ANP... outline name
      - id: outline_type
        type: u4
        doc: type enum in the high byte; 0x32 POUROUT, 0x33 HATOUT, 0x34 VOIDOUT, others related outline types

  sec53_outline_piece:
    doc: |
      One 16-byte geometry piece. The example `05 00 00 00 00 00 00 00
      24 e8 02 00 32 04 00 00` is ASC `POLY 5 0 190500 4` exactly.
    seq:
      - id: corner_count
        type: u4
      - id: arc_count
        type: u4
      - id: width
        type: s4
        doc: outline width in BASIC units
      - id: piece_type
        type: u1
        doc: 0x32 polygon; 0x33 circle
      - id: layer
        type: u1
        doc: 1-based PADS layer ordinal
      - id: piece_flags
        type: u2

  sec54_outline_vertex:
    doc: 8-byte outline vertex in the owner's coordinate convention
    seq:
      - id: x
        type: s4
      - id: y
        type: s4

  sec55_outline_arc:
    doc: |
      Arc decoration for one section-54 vertex. The angle word is two signed
      16-bit ASC angle values. Corpus/ASCII example: begin 1244, delta -1589 is
      stored as 0xF9CB04DC.
    seq:
      - id: x
        type: s4
        doc: decorated vertex X coordinate
      - id: y
        type: s4
        doc: decorated vertex Y coordinate
      - id: arc_index
        type: u4
        doc: zero-based arc ordinal within the owning piece
      - id: begin_angle
        type: s2
        doc: ASC begin angle
      - id: delta_angle
        type: s2
        doc: ASC signed sweep angle

  # =========================================================================
  # SECTION 59 — heap-object array overlaid on the string-pool tail
  # =========================================================================
  # Its first 32 bytes in the modern dialect, or first 16 bytes in the legacy
  # dialect, overlay the final string-pool bytes. The remaining records end
  # exactly at section 60.
  sections59_64_stream:
    params:
      - id: num_heap_objects
        type: u4
      - id: heap_object_stride
        type: u4
      - id: num_route_junctions
        type: u4
      - id: route_junction_stride
        type: u4
      - id: num_object_handles
        type: u4
      - id: num_route_objects
        type: u4
      - id: route_object_stride
        type: u4
      - id: num_route_layers
        type: u4
      - id: num_route_cells
        type: u4
    doc: complete physical section-59 through section-64 route/object stream
    seq:
      - id: heap_objects
        type:
          switch-on: heap_object_stride
          cases:
            24: sec59_heap_obj_v24
            32: sec59_heap_obj_v32
        repeat: expr
        repeat-expr: num_heap_objects
      - id: route_junctions
        type: 'sec60_route_junction_ring(num_route_junctions, route_junction_stride)'
      - id: object_handles
        type: sec61_object_handle
        repeat: expr
        repeat-expr: num_object_handles
      - id: route_objects
        type: 'sec62_route_object_ring(num_route_objects, route_object_stride)'
      - id: route_layers
        type: sec63_route_layer
        repeat: expr
        repeat-expr: num_route_layers
      - id: route_cells
        type: sec64_route_coord_pool
        repeat: expr
        repeat-expr: num_route_cells

  sec59_heap_obj_v32:
    doc: modern 32-byte heap-object record; no board geometry
    seq:
      - id: ptr0
        type: u4
      - id: ptr1
        type: u4
      - id: ptr2
        type: u4
      - id: class_tag
        type: u4
        doc: class enum including 0x2001, 0x2000, 0x2400 and 0x1000
      - id: scalar
        type: s4
        doc: BASIC width/clearance-class scalar, or zero
      - id: state
        type: u4
      - id: handle
        type: u4
      - id: flags
        type: u4

  sec59_heap_obj_v24:
    doc: legacy 24-byte heap-object record; no board geometry
    seq:
      - id: ptr0
        type: u4
      - id: class_tag
        type: u4
      - id: scalar
        type: s4
        doc: BASIC width/clearance-class scalar
      - id: state
        type: u4
      - id: handle
        type: u4
      - id: ptr1
        type: u4

  # =========================================================================
  # SECTION 60 — route-junction records
  # =========================================================================
  # The physical section is the logical record array rotated left by one byte:
  # physical byte zero is logical byte one, and the physical final byte closes
  # logical record zero. The field grid therefore starts one byte before the
  # physical range. Both dialects share
  # tail-relative fields: X=stride-31, Y=stride-27, via definition=stride-7,
  # type=stride-4, and net index=stride-3.
  sec60_route_junction_ring:
    params:
      - id: num_records
        type: u4
      - id: record_stride
        type: u4
    doc: one-byte-left-rotated physical storage for the logical junction records
    seq:
      - id: first_record_tail
        size: record_stride - 1
        doc: logical record zero bytes 1 through record_stride-1
      - id: subsequent_records
        type:
          switch-on: record_stride
          cases:
            64: sec60_route_junction_v64
            48: sec60_route_junction_v48
        repeat: expr
        repeat-expr: num_records - 1
      - id: first_record_head
        type: u1
        doc: logical record zero byte 0, rotated to the physical end

  sec60_route_junction_v64:
    doc: modern 64-byte route-junction/via record
    seq:
      - id: object_state
        size: 33
        doc: serialized object handles, links and state before the coordinates
      - id: x_raw
        type: s4
        doc: RAW X coordinate
      - id: y_raw
        type: s4
        doc: RAW Y coordinate
      - id: relationship_state
        size: 16
        doc: route-chain links and role flags
      - id: via_definition_index
        type: u1
      - id: pre_type_state
        type: u2
      - id: junction_type
        type: u1
        doc: 0x16 corner/junction; 0x0E via/connection
      - id: net_index
        type: u2
      - id: trailing_state
        type: u1

  sec60_route_junction_v48:
    doc: legacy 48-byte route-junction/via record
    seq:
      - id: object_state
        size: 17
      - id: x_raw
        type: s4
      - id: y_raw
        type: s4
      - id: relationship_state
        size: 16
      - id: via_definition_index
        type: u1
      - id: pre_type_state
        type: u2
      - id: junction_type
        type: u1
      - id: net_index
        type: u2
      - id: trailing_state
        type: u1

  # =========================================================================
  # SECTION 61 — object-handle / heap-bookkeeping snapshot
  # =========================================================================
  # Direct 12-byte records. The old 64-byte logical framing was a false phase
  # imposed by the accumulated directory offset.
  sec61_object_handle:
    doc: 12-byte route object-handle / bookkeeping record; no geometry
    seq:
      - id: state
        type: u4
      - id: object_handle
        type: u4
      - id: relationship_handle
        type: u4

  # =========================================================================
  # SECTION 62 — route-object array
  # =========================================================================
  # The section is a ring rotated left by 32 physical bytes: physical +0 is the
  # final record's 32-byte tail, logical record 0 starts at physical +32, and the
  # final record's head closes the section. Modern logical records are 48 bytes;
  # legacy records are 36. width = quarter_width*4.
  sec62_route_object_v48:
    seq:
      - id: tag0
        type: u4
        doc: '+0 object type tag/handle or heap addr'
      - id: ptr_next
        type: u4
        doc: '+4 pointer, usually 0'
      - id: id_or_ptr
        type: u4
        doc: '+8 id or pointer'
      - id: flag_signbit
        type: u4
        doc: '+12 often 0x80000000'
      - id: obj_ptr
        type: u4
        doc: '+16 heap pointer (constant high word per file)'
      - id: quarter_width
        type: s4
        doc: '+20 route_width = quarter_width * 4, BASIC'
      - id: y_raw
        type: s4
        doc: '+24 endpoint Y (RAW; not a routed vertex)'
      - id: x_raw
        type: s4
        doc: '+28 endpoint X (RAW; not a routed vertex)'
      - id: flags
        type: u4
        doc: '+32 object flag bits (THERMAL/TEARDROP family)'
      - id: type_enum
        type: s4
        doc: '+36 small bounded enum (1..~12)'
      - id: ptr_b
        type: u4
        doc: '+40 pointer, usually 0'
      - id: tag1
        type: u4
        doc: '+44 mirror of tag0'

  sec62_route_object_v36:
    seq:
      - id: tag0
        type: u4
      - id: object_handle
        type: u4
      - id: quarter_width
        type: s4
        doc: route_width = quarter_width * 4, BASIC
      - id: bound_lo
        type: s4
      - id: bound_hi
        type: s4
      - id: style
        type: u4
      - id: cell_count
        type: u4
      - id: relationship_handle
        type: u4
      - id: tag1
        type: u4
        doc: mirror of tag0

  sec62_route_object_ring:
    params:
      - id: num_records
        type: u4
      - id: record_stride
        type: u4
    doc: 32-byte-left-rotated physical section-62 record ring
    seq:
      - id: final_record_tail
        size: record_stride - 32
      - id: preceding_records
        type:
          switch-on: record_stride
          cases:
            36: sec62_route_object_v36
            48: sec62_route_object_v48
        repeat: expr
        repeat-expr: num_records - 1
      - id: final_record_head
        size: 32

  sec63_route_layer:
    doc: serialized route-layer ordinal; the array is a permutation of active layer indices
    seq:
      - id: layer_index
        type: u2

  # =========================================================================
  # SECTION 64 — route coordinate pool
  # =========================================================================
  # Direct 12-byte compressed route cells. The three coordinates encode two
  # points sharing one axis: (x1,y)-(x2,y), or the transposed form selected for
  # the file. Section-62 cell_count values sum to section64.count.
  sec64_route_coord_pool:
    seq:
      - id: first_major
        type: s4
      - id: shared_minor
        type: s4
      - id: second_major
        type: s4

  section65_66_saved_controller_v16:
    doc: legacy optional rule/class archive controller; four saved process-local links
    seq:
      - id: saved_links
        type: u4
        repeat: expr
        repeat-expr: 4

  section65_66_saved_controller_v32:
    doc: modern optional rule/class archive controller; eight saved process-local links
    seq:
      - id: saved_links
        type: u4
        repeat: expr
        repeat-expr: 8

  section65_66_saved_relationship_link:
    doc: |
      Eight saved process-local links associated with one section-67 design-rule
      relationship. The sole scoped file using this dialect has exactly one
      32-byte link record per section-67 relationship. These values are runtime
      pointers/handles, never file offsets or geometry.
    seq:
      - id: saved_links
        type: u4
        repeat: expr
        repeat-expr: 8

  section65_66_compact_net_class:
    doc: 28-byte compact named-class object used by the two RFE_EYEBROW files
    seq:
      - id: cleared_state
        type: u4
        valid: 0
        repeat: expr
        repeat-expr: 3
      - id: name
        type: strz
        encoding: ASCII
        size: 16

  section65_66_net_class:
    doc: |
      280-byte saved net-class object. Present only when directory section 66's
      one-byte in-memory-presence value is set. All 60 scoped records have an
      inline 48-byte name and cleared retained membership capacity. The final
      two words remain live controller state and are not padding.
    seq:
      - id: class_flags
        type: u4
        doc: zero or 0x40000000 in the scoped corpus
      - id: saved_class_handle
        type: u4
        doc: process-local net-class object handle; null on the terminal class in the MMSP pair
      - id: name
        type: strz
        encoding: ASCII
        size: 48
      - id: cleared_membership_capacity
        type: u4
        valid: 0
        repeat: expr
        repeat-expr: 54
        doc: retained member-link capacity, cleared on every scoped record
      - id: saved_controller_handle
        type: u4
      - id: controller_state
        type: u4
        valid: 0x80000000

  section65_66_archive:
    params:
      - id: controller_size
        type: u4
      - id: num_saved_relationship_links
        type: u4
      - id: num_compact_net_classes
        type: u4
      - id: num_net_classes
        type: u4
    doc: complete optional section-65/66 saved rule/class archive
    seq:
      - id: controller
        type:
          switch-on: controller_size
          cases:
            16: section65_66_saved_controller_v16
            32: section65_66_saved_controller_v32
      - id: saved_relationship_links
        type: section65_66_saved_relationship_link
        repeat: expr
        repeat-expr: num_saved_relationship_links
      - id: compact_net_classes
        type: section65_66_compact_net_class
        repeat: expr
        repeat-expr: num_compact_net_classes
      - id: net_classes
        type: section65_66_net_class
        repeat: expr
        repeat-expr: num_net_classes

  # =========================================================================
  # SECTION 67 — design-rule relationship graph
  # =========================================================================
  # Physical storage rotates the logical relationship record right by one u32:
  # rule_kind is written first, followed by the saved relationship handle, two
  # scope type/reference pairs, and layer/state. This layout validates every
  # section-67 record in all 90 scoped files; the former seven-coordinate
  # interpretation was a phase error.
  sec67_design_rule_relationship:
    seq:
      - id: rule_kind
        type: u4
        doc: design-rule kind enum (0, 6, 0x29, 0x2a, 0x2d, 0x2e, 0x30, or 0x86)
      - id: rule_detail_handle
        type: u4
        doc: saved process-local relationship handle
      - id: scope_a_type
        type: u4
        doc: first scope type (default, NET, NET_CLASS, or saved class dialect)
      - id: scope_a_reference
        type: u4
        doc: first saved scope reference; 0x03000000 sentinel for default scope
      - id: scope_b_type
        type: u4
      - id: scope_b_reference
        type: u4
      - id: layer_or_state
        type: u4
        doc: layer selector or relationship state

  sec67_design_rule_relationship_array:
    seq:
      - id: records
        type: sec67_design_rule_relationship
        repeat: eos

  cluster_record_array:
    seq:
      - id: records
        type: cluster_record
        repeat: eos

  section69_controller_leadin:
    doc: 12-byte terminal rule selector and cleared layer-controller state
    seq:
      - id: terminal_rule_kind
        type: u4
        doc: low byte 6 or 0x86; bit 31 may retain controller state
      - id: cleared_state
        type: u4
        valid: 0
        repeat: expr
        repeat-expr: 2

  # =========================================================================
  # SECTION 69 — layer-definition / stackup table
  # =========================================================================
  # Section 69 is a fixed-stride layer-record array: 31 records, `(All layers)`
  # plus 30 numbered layers. Record size is 128 bytes in v0x2017..v0x2021, 136
  # in v0x2022, and 152 in v0x2024 and later.
  # The dialects differ only in miscellaneous display-color slot count (6/8/12).
  # The flat loader reads tag 0x45 into the layer array, then independently reads
  # tag 0x46 (section 70), tag 0x47 (global display preferences), tag 0x48
  # (error conflicts), and tag 0x49 (font faces). Earlier notes claiming sections
  # 71 and 73 were layer-table overflow windows are disproved by that read order
  # and by their exact corpus framing.
  # STACKUP SOURCE: layer_thickness@+52 and copper_thickness@+56 are BASIC units;
  # dielectric f4@+60 is the dielectric constant.
  # usage@+148==1 marks an active copper layer (count == .asc MAXIMUMLAYER). Locate the
  # 31-record array by the inline string "(All layers)", NOT directory data_offset.
  sec69_layer_record:
    params:
      - id: num_colors_misc
        type: u4
    doc: version-sized layer definition + physical stackup + display-color record
    seq:
      - id: name
        type: str
        size: 24
        encoding: ASCII
        terminator: 0
        doc: '+0 layer name ("(All layers)","Top","Solder Mask Top",...); AEA-001 has zeroed initial L bytes in its Layer_N names but retains the same field and stride'
      - id: layer_state0
        type: s4
        doc: '+24 layer-controller state; normally zero'
      - id: layer_state1
        type: s4
        doc: '+28 layer-controller state; normally zero'
      - id: routing_dir
        type: s4
        doc: '+32 ROUTING_DIRECTION 0=H 1=V 2=NO_PREFERENCE'
      - id: assoc_silk
        type: s4
        doc: '+36 ASSOCIATED_SILK_SCREEN doc-layer # (-1 none)'
      - id: assoc_paste
        type: s4
        doc: '+40 ASSOCIATED_PASTE_MASK doc-layer #'
      - id: assoc_mask
        type: s4
        doc: '+44 ASSOCIATED_SOLDER_MASK doc-layer #'
      - id: assoc_assembly
        type: s4
        doc: '+48 ASSOCIATED_ASSEMBLY doc-layer #'
      - id: layer_thickness
        type: s4
        doc: '+52 LAYER_THICKNESS, BASIC'
      - id: copper_thickness
        type: s4
        doc: '+56 COPPER_THICKNESS, BASIC'
      - id: dielectric
        type: f4
        doc: '+60 DIELECTRIC constant Er (3.3 / 4.3)'
      - id: color_route
        type: s4
        doc: '+64 ROUTE color (palette index)'
      - id: color_via
        type: s4
        doc: '+68 VIA color (== route)'
      - id: color_pad
        type: s4
        doc: '+72 PAD color (== route)'
      - id: color_copper
        type: s4
        doc: '+76 COPPER color (== route)'
      - id: color_2dline
        type: s4
        doc: '+80 2DLINE color (== route)'
      - id: color_text
        type: s4
        doc: '+84 TEXT color (== route)'
      - id: color_error
        type: s4
        doc: '+88 element color (14 on data layers, 0/1 empty)'
      - id: colors_misc
        type: s4
        repeat: expr
        repeat-expr: num_colors_misc
        doc: remaining per-element display colors; 6 slots through v0x2021, 8 in v0x2022, 12 from v0x2024
      - id: flags
        type: s4
        doc: '+140 packed attribute bitfield (bits0-2 routable/visible/selectable)'
      - id: layer_state2
        type: s4
        doc: '+144 layer-controller state; final record may retain allocator contents'
      - id: usage
        type: s4
        doc: '+148 1=routing-used(==MAXIMUMLAYER), 0=unused/drill, 2..6=doc subtype'

  sec69_layer_record_array:
    params:
      - id: num_colors_misc
        type: u4
    seq:
      - id: records
        type: 'sec69_layer_record(num_colors_misc)'
        repeat: eos

enums:
  pad_shape:
    0: of    # oblong / oval finger
    1: rf    # rectangular finger
    2: r     # round
    3: s     # square
