# Kaitai Struct definition for the PADS PowerPCB binary `.pcb` format.
#
# Coordinate convention: absolute geometry is RAW = design + per-axis origin.
# The origin (i32 x,y) is stored at section[1] +60/+64. Design_x = raw_x -
# origin_x and design_y = raw_y - origin_y, and the two axes may differ. Some
# sections store DESIGN coordinates directly (per-type docs note which). Widths,
# sizes, drills, grids and angles are not origin-shifted.
#
# Units: BASIC = 1/38100 mil. Angles are stored as degrees * 1,800,000.
#
# The controller-slot count is stored directly in directory entry 1. PADS reads
# that number of 16-byte slots from file offset 6. The offset-10 directory view
# has num_directory_entries-1 full entries and one last 12-byte entry. The file ends
# with the footer GUID and a u32 back-pointer to the container-item array.

meta:
  id: pads_pcb_binary
  title: PADS PowerPCB binary layout (.pcb)
  file-extension: pcb
  endian: le
  encoding: ASCII
  imports:
    - microsoft_cfb

doc: |
  PADS PowerPCB binary board file. Versions 0x2017, 0x2019, 0x2021, 0x2022,
  0x2024, 0x2025, 0x2026, and 0x2027 are supported.

  Flat controllers and paged controllers have a fixed serialized order. For a
  flat controller, `stored_extent` is the byte length. For a paged controller,
  `stored_extent` is the number of section 26 page descriptors.

  Root offsets use values from the header, the controller directory, page
  descriptors, live-slot conditions, and the footer back-pointer. A later
  boundary does not set an earlier boundary.

seq:
  - id: magic
    contents: [0x00, 0xff]
    doc: File magic 00 FF.
  - id: version
    type: u2
    doc: The DB format version is 0x2017, 0x2019, 0x2021, 0x2022, 0x2024, 0x2025, 0x2026, or 0x2027.
  - id: subversion
    type: u2
    doc: |
      Format subversion. The values are 0, 1, 2, and 3. The value does not
      depend on the number of directory entries.
  - id: header_padding
    contents: [0, 0, 0, 0]
    doc: |
      Four zero bytes. The controller directory starts at file offset 6. Thus,
      these bytes are the unused first word of controller slot 0.
  - id: directory_entries
    type: 'directory_entry(_index + 1 < num_directory_entries)'
    repeat: expr
    repeat-expr: 'num_directory_entries'
    doc: |
      This is a four-byte-shifted view of the controller slots. All entries
      except the last entry include the zero word from the next slot.
      The last entry ends at `controller_stream_offset`.

      For a flat controller, `stored_extent` is the byte length. For a paged
      controller, it is the number of 12-byte descriptors in section 26.
  - id: physical_body
    type: physical_file_body
    size: 'footer.container_item_back_pointer - controller_stream_offset'
    doc: |
      This field contains the database bytes between the controller directory
      and the OLE container-item array. Root instances give the logical
      circular-array views.
  - id: physical_container_items
    type: container_item_array
    size: '_io.size - 42 - footer.container_item_back_pointer'
    doc: This is the serialized OLE container-item array at the footer back-pointer.
  - id: physical_footer
    type: footer
    doc: This is the last 42-byte MFC document footer.

instances:
  num_directory_entries:
    value: _root.directory_count_entry.num_items
    doc: |
      This is the stored number of controller slots. Directory entry 1 has the
      value. Its `stored_extent` value is `num_items * 16`.
  controller_stream_offset:
    value: '6 + num_directory_entries * 16'
    doc: |
      This is the file offset after the controller directory. The directory
      starts at offset 6. The last directory entry has 12 bytes.
  directory_count_entry:
    pos: 26
    type: 'directory_entry(true)'
    doc: Directory entry 1 contains the number of directory entries.
  footer:
    pos: '_io.size - 42'
    type: footer
    doc: This is the last 42-byte MFC document footer, anchored from EOF.
  container_items:
    pos: footer.container_item_back_pointer
    type: container_item_array
    size: '_io.size - 42 - footer.container_item_back_pointer'
    doc: Root serialized OLE container-item array, reached by the footer back-pointer.
  view_state_records:
    pos: controller_stream_offset
    type: section_2_view_state_array
    size: directory_entries[2].stored_extent
    doc: Section 2 records are physically written before section 1.
  board_setup:
    pos: 'controller_stream_offset - 12 + directory_entries[2].stored_extent'
    type: section_1_board_setup
    size: directory_entries[1].stored_extent
    doc: Rotated section 1 logical view. Its first 12 bytes precede the physical section 2/section 3 controller cursor.
  len_board_parameters:
    value: directory_entries[3].stored_extent
    doc: This is the byte length of the physical section 3 controller.
  board_parameters:
    pos: 'controller_stream_offset + directory_entries[2].stored_extent'
    type: section_3_physical_controller
    size: len_board_parameters
    doc: This is the full physical section 3 database/board-parameter controller image.
  section_4_physical_offset:
    value: 'controller_stream_offset + directory_entries[2].stored_extent + directory_entries[3].stored_extent'
    doc: This is the physical file offset of the flat section 4 controller.
  flat_controllers_4_to_24:
    pos: section_4_physical_offset
    type: flat_controller_storage_4_to_24
    size: 'directory_entries[4].stored_extent + directory_entries[5].stored_extent + directory_entries[6].stored_extent + directory_entries[7].stored_extent + directory_entries[8].stored_extent + directory_entries[9].stored_extent + directory_entries[10].stored_extent + directory_entries[11].stored_extent + directory_entries[12].stored_extent + directory_entries[13].stored_extent + directory_entries[14].stored_extent + directory_entries[15].stored_extent + directory_entries[16].stored_extent + directory_entries[17].stored_extent + directory_entries[18].stored_extent + directory_entries[19].stored_extent + directory_entries[20].stored_extent + directory_entries[21].stored_extent + directory_entries[22].stored_extent + directory_entries[23].stored_extent + directory_entries[24].stored_extent'
    doc: This is the physical flat-controller partition for tags 4 through 24.
  section_4_offset:
    value: 'section_4_physical_offset - 44'
    doc: This is the logical fixed-record view. These controllers serialize their first 44 bytes at the physical ring tail.
  padstack_definitions:
    pos: 'section_4_physical_offset - (version == 0x2022 ? 20 : version <= 0x2021 ? 24 : 28)'
    type: section_4_padstack_array
    size: directory_entries[4].stored_extent
    doc: Versioned logical padstack grid. Physical marker is +24 through v0x2021 and +28 thereafter.
  pad_layer_controller_header:
    pos: 'section_4_physical_offset - (version == 0x2022 ? 20 : version <= 0x2021 ? 24 : 28) + directory_entries[4].stored_extent'
    type: saved_pad_layer_controller_header
    doc: This is the saved section 5 controller state between the rotated padstack grid and the first layer row.
  pad_layer_shapes:
    pos: 'section_4_physical_offset + directory_entries[4].stored_extent + (version == 0x2022 ? 44 : -4)'
    type: section_5_pad_layer_array
    size: directory_entries[5].stored_extent
    doc: Per-padstack layer rows after the versioned serialized controller lead-in.
  text_objects:
    pos: 'section_4_physical_offset + directory_entries[4].stored_extent + directory_entries[5].stored_extent + directory_entries[6].stored_extent + directory_entries[7].stored_extent - (version == 0x2017 ? 28 : 36)'
    type: section_8_text_ring
    size: 'directory_entries[8].stored_extent + (version == 0x2017 ? 28 : 36)'
    if: directory_entries[8].num_items > 0
    doc: This is the circular text-record view. Metadata in record K+1 owns geometry in record K.
  text_string_pool:
    pos: 'section_4_physical_offset + directory_entries[4].stored_extent + directory_entries[5].stored_extent + directory_entries[6].stored_extent + directory_entries[7].stored_extent + directory_entries[8].stored_extent'
    type: section_9
    size: directory_entries[9].stored_extent
    if: directory_entries[9].stored_extent > 0
    doc: Section 9 indexed string-pool allocation.
  drawing_objects:
    pos: section_10_physical_offset
    type: section_10_drawing_physical
    size: directory_entries[10].stored_extent
  graphic_piece_headers:
    pos: section_11_physical_offset
    type: section_11_piece_physical
    size: directory_entries[11].stored_extent
  graphic_vertices:
    pos: section_12_physical_offset
    type: section_12_vertex_array
    size: directory_entries[12].stored_extent
  section_10_physical_offset:
    value: 'section_4_physical_offset + directory_entries[4].stored_extent + directory_entries[5].stored_extent + directory_entries[6].stored_extent + directory_entries[7].stored_extent + directory_entries[8].stored_extent + directory_entries[9].stored_extent'
    doc: This is the physical file offset of the section 10 drawing-owner controller.
  section_11_physical_offset:
    value: 'section_10_physical_offset + directory_entries[10].stored_extent'
    doc: This is the physical file offset of the section 11 graphic-piece controller.
  section_12_physical_offset:
    value: 'section_11_physical_offset + directory_entries[11].stored_extent'
    doc: This is the physical file offset of the section 12 vertex array.
  section_13_physical_offset:
    value: 'section_12_physical_offset + directory_entries[12].stored_extent'
    doc: This is the physical file offset of the section 13 graphic-parameter array.
  section_13_offset:
    value: 'section_4_offset + directory_entries[4].stored_extent + directory_entries[5].stored_extent + directory_entries[6].stored_extent + directory_entries[7].stored_extent + directory_entries[8].stored_extent + directory_entries[9].stored_extent + directory_entries[10].stored_extent + directory_entries[11].stored_extent + directory_entries[12].stored_extent'
  hatch_segments:
    pos: section_13_physical_offset
    type: section_13_hatch_array
    size: directory_entries[13].stored_extent
    if: directory_entries[13].num_items > 0
    doc: This is the full flat 20-byte graphic-parameter array used by arcs and copper-pour hatch segments.
  section_14_offset:
    value: 'section_13_offset + directory_entries[13].stored_extent'
  decal_terminal_descriptors:
    pos: section_14_offset + 44
    type: section_14_terminal_descriptor_array
    size: directory_entries[14].stored_extent
    doc: |
      This is the PARTDECAL terminal-run descriptor array. The nominal section
      boundary is 44 bytes before record zero. Each descriptor contains the
      terminal cursor and the padstack cursor.
  section_15_offset:
    value: 'section_14_offset + directory_entries[14].stored_extent'
  section_15_logical_offset:
    value: 'section_15_offset + (version <= 0x2019 ? 60 : 44)'
    doc: Legacy records begin 16 bytes into the physical controller ring. Modern records begin at the physical controller start.
  legacy_terminal_controller_prefix:
    pos: 'section_15_offset + 44'
    type: saved_terminal_controller_prefix
    if: version <= 0x2019
    doc: This is the legacy saved section 15 state before the terminal-slot ring.
  decal_terminal_slots:
    pos: section_15_logical_offset
    type: decal_terminal_slot_array
    size: directory_entries[15].stored_extent
    doc: Section 15 terminal records followed by mixed decal controller and object-dictionary storage units.
  section_16_offset:
    value: 'section_15_offset + directory_entries[15].stored_extent'
  decal_padstack_pairs:
    pos: section_16_offset + 44
    type: decal_padstack_pair_array
    size: directory_entries[16].stored_extent
    doc: |
      Per-terminal padstack mappings begin at the physical section 16 boundary,
      section_16_offset+44 in the rotated logical view. The pair cursor is the
      owning descriptor's +88 word. The next descriptor contains its count at
      +32 for 100-byte descriptors and +20 for 112-byte descriptors.
  section_17_offset:
    value: 'section_16_offset + directory_entries[16].stored_extent'
  part_types:
    pos: section_17_offset
    type: part_type_record_array
    size: directory_entries[17].stored_extent
    doc: This is the full PARTTYPE definition array in declaration order.
  section_18_offset:
    value: 'section_13_offset + directory_entries[13].stored_extent + directory_entries[14].stored_extent + directory_entries[15].stored_extent + directory_entries[16].stored_extent + directory_entries[17].stored_extent'
  part_type_last_metadata:
    pos: section_18_offset
    type: part_type_last_metadata
    doc: This is the last rotated PARTTYPE metadata, ending 44 bytes after the nominal section 18 boundary.
  part_type_gates:
    pos: 'section_18_offset + 44'
    type: part_type_gate_array
    size: directory_entries[18].stored_extent
    doc: Gate definitions begin after the last PARTTYPE metadata and end 44 bytes into nominal section 19.
  section_19_offset:
    value: 'section_18_offset + directory_entries[18].stored_extent'
  part_type_pins:
    pos: 'section_19_offset + 44'
    type: part_type_pin_array
    size: directory_entries[19].stored_extent
    if: directory_entries[19].num_items > 0
    doc: PARTTYPE pin records are phase-shifted 44 bytes past the nominal section 19 boundary.
  section_20_offset:
    value: 'section_19_offset + directory_entries[19].stored_extent'
  section_20_logical_offset:
    value: 'section_20_offset + (version <= 0x2019 ? 48 : 44)'
    doc: Legacy SIGPIN records have one additional rotated four-byte word before their first pin ordinal.
  legacy_signal_pin_last_record_tail:
    pos: 'section_20_offset + 44'
    type: legacy_signal_pin_last_record_tail
    if: 'version <= 0x2019 and directory_entries[20].num_items > 0'
    doc: This is the last four bytes of the rotated legacy SIGPIN ring's last record.
  part_type_signal_pins:
    pos: section_20_logical_offset
    type: part_type_signal_pin_array
    size: directory_entries[20].stored_extent
    if: directory_entries[20].num_items > 0
    doc: ASCII SIGPIN pin-to-signal mappings attached to PARTTYPE definitions.
  section_21_offset:
    value: 'section_20_offset + directory_entries[20].stored_extent'
  compact_part_type_pin_names:
    pos: 'section_21_offset + 44'
    type: compact_part_type_pin_name_array
    size: directory_entries[21].stored_extent
    if: directory_entries[21].num_items > 0
    doc: These compact pin-name lists are next to the PARTTYPE declarations.
  section_22_offset:
    value: 'section_19_offset + directory_entries[19].stored_extent + directory_entries[20].stored_extent + directory_entries[21].stored_extent'
  part_placements:
    pos: section_22_offset
    type: part_placement_array
    size: directory_entries[22].stored_extent
    if: directory_entries[22].num_items > 0
    doc: This is the full placed-part array. Directory count equals the live placement count.
  section_23_offset:
    value: 'section_22_offset + directory_entries[22].stored_extent'
    doc: Nominal circular-array boundary, 44 bytes before the physical section 23 cursor.
  legacy_net_controller_prefix:
    pos: section_23_offset
    type: saved_net_controller_prefix
    if: version <= 0x2022
    doc: This is the saved section 23 controller prefix before the legacy net-record array.
  nets:
    pos: 'version <= 0x2022 ? section_23_offset + 44 : section_23_offset'
    type: net_record_array
    size: directory_entries[23].stored_extent
    if: directory_entries[23].num_items > 0
    doc: |
      Named nets followed by the unassigned-obstacles sentinel where present.
      Through v0x2022, the 144-byte logical view starts 20 bytes after the
      physical section 23 boundary. It wraps within the controller. Its first
      20 bytes are the last record's tail. The directory count is
      correct. Bytes in section 24 are not extra net records. Modern records use
      the nominal boundary directly because their logical view begins 44 bytes
      before the physical section 23 cursor.
  section_24_offset:
    value: 'section_23_offset + directory_entries[23].stored_extent'
  section_24_controller_prefix:
    pos: 'section_24_offset + (version <= 0x2022 ? 44 : 0)'
    type: saved_connection_controller_prefix
    doc: This is the saved section 24 controller state before the rotated connection-record ring.
  empty_route_chain_controller_state:
    pos: 'section_24_offset + 8'
    type: empty_route_chain_controller_state
    if: 'version >= 0x2024 and directory_entries[24].num_items == 0'
    doc: This is the retained allocator/list state occupying the terminal-head slot of an empty modern connection controller.
  route_chains:
    pos: 'version <= 0x2022 ? section_24_offset + 60 : section_24_offset + 8'
    type: 'route_chain_array(directory_entries[24].num_items, version >= 0x2024 ? 1 : 0)'
    size: 'directory_entries[24].stored_extent + (version >= 0x2024 ? 36 : 0)'
    if: directory_entries[24].num_items > 0
    doc: |
      Logical 68-byte topology array. Through v0x2022 record zero starts 16
      bytes after the physical section 24 boundary. The physical prefix is the
      last record's 16-byte tail. Modern record zero starts 36 bytes before
      that boundary. The directory count is the edge count. The last edge's
      36-byte topology head follows the full records. It ends at the physical
      controller boundary. The controller uses a fixed circular-array rotation.
  section_25_offset:
    value: 'controller_stream_offset + directory_entries[2].stored_extent + directory_entries[3].stored_extent + directory_entries[4].stored_extent + directory_entries[5].stored_extent + directory_entries[6].stored_extent + directory_entries[7].stored_extent + directory_entries[8].stored_extent + directory_entries[9].stored_extent + directory_entries[10].stored_extent + directory_entries[11].stored_extent + directory_entries[12].stored_extent + directory_entries[13].stored_extent + directory_entries[14].stored_extent + directory_entries[15].stored_extent + directory_entries[16].stored_extent + directory_entries[17].stored_extent + directory_entries[18].stored_extent + directory_entries[19].stored_extent + directory_entries[20].stored_extent + directory_entries[21].stored_extent + directory_entries[22].stored_extent + directory_entries[23].stored_extent + directory_entries[24].stored_extent'
  route_allocator_controller:
    pos: section_25_offset
    type: route_allocator_controller
    size: directory_entries[25].stored_extent
    doc: Global route-controller state, including the four section 26 allocator page-group counts.
  section_26_offset:
    value: 'section_25_offset + directory_entries[25].stored_extent'
  route_object_ranges:
    pos: section_26_offset
    type: route_object_range_array
    size: directory_entries[26].stored_extent
    doc: |
      Section-26 descriptor directory. Its prefix holds four variable-size
      NumOrdMdl descriptor groups. Page counts partition the suffix. The directory
      controllers are 65, 66, 45, 46, 47, 48, 41, 42, and optionally 74.
  section_61_allocator_descriptor_offset:
    value: 'section_26_offset + (route_allocator_controller.num_allocator_20_pages + route_allocator_controller.num_allocator_48_pages + route_allocator_controller.num_allocator_88_pages) * 12'
    doc: Start of the fourth section 26 prefix group. Its runtime object stride is 56 bytes.
  section_61_allocator_page_descriptors:
    pos: section_61_allocator_descriptor_offset
    type: 'page_descriptor_group(route_allocator_controller.num_allocator_56_pages)'
    if: route_allocator_controller.num_allocator_56_pages > 0
    doc: |
      These are page descriptors for section 61 nodes. For each page except the
      last page, the next descriptor gives the record count. The last
      page contains the remaining records.
  section_27_offset:
    value: 'section_26_offset + directory_entries[26].stored_extent'
  route_layer_object_counts:
    pos: section_27_offset
    type: route_layer_object_count_array
    size: directory_entries[27].stored_extent
    doc: Per-copper-layer route-object counts. Their sum is the section 29 handle count.
  section_29_offset:
    value: 'section_27_offset + directory_entries[27].stored_extent + directory_entries[28].stored_extent'
  route_object_handles:
    pos: section_29_offset
    type: route_object_handle_array
    size: directory_entries[29].stored_extent
    doc: This is the route-object handles grouped by the previous per-layer counts.
  section_41_offset:
    value: 'section_29_offset + directory_entries[29].stored_extent'
    doc: |
      This is the physical start of paged controller 41. Its page descriptors
      give the record count. The record length is 180 bytes in v0x2017 and 188
      bytes thereafter. Paged controllers occur in this order: 41, 42, 45, 46,
      47, and 48. There is no controller header between these arrays.
  page_descriptor_suffix_offset:
    value: 'section_26_offset + directory_entries[26].stored_extent - (directory_entries[65].stored_extent + directory_entries[66].stored_extent + directory_entries[45].stored_extent + directory_entries[46].stored_extent + directory_entries[47].stored_extent + directory_entries[48].stored_extent + directory_entries[41].stored_extent + directory_entries[42].stored_extent + (num_directory_entries > 74 ? directory_entries[74].stored_extent : 0)) * 12'
    doc: |
      The section 26 descriptor suffix uses this controller order: 65, 66, 45,
      46, 47, 48, 41, 42, and optional 74. Here, each `stored_extent` value is a
      descriptor count, not a byte length.
  section_65_page_descriptors:
    pos: page_descriptor_suffix_offset
    type: 'page_descriptor_group(directory_entries[65].stored_extent)'
    if: directory_entries[65].stored_extent > 0
  section_66_page_descriptors:
    pos: 'page_descriptor_suffix_offset + directory_entries[65].stored_extent * 12'
    type: 'page_descriptor_group(directory_entries[66].stored_extent)'
    if: directory_entries[66].stored_extent > 0
  section_45_page_descriptors:
    pos: 'page_descriptor_suffix_offset + (directory_entries[65].stored_extent + directory_entries[66].stored_extent) * 12'
    type: 'page_descriptor_group(directory_entries[45].stored_extent)'
    if: directory_entries[45].stored_extent > 0
  section_46_page_descriptors:
    pos: 'page_descriptor_suffix_offset + (directory_entries[65].stored_extent + directory_entries[66].stored_extent + directory_entries[45].stored_extent) * 12'
    type: 'page_descriptor_group(directory_entries[46].stored_extent)'
    if: directory_entries[46].stored_extent > 0
  section_47_page_descriptors:
    pos: 'page_descriptor_suffix_offset + (directory_entries[65].stored_extent + directory_entries[66].stored_extent + directory_entries[45].stored_extent + directory_entries[46].stored_extent) * 12'
    type: 'page_descriptor_group(directory_entries[47].stored_extent)'
    if: directory_entries[47].stored_extent > 0
  section_48_page_descriptors:
    pos: 'page_descriptor_suffix_offset + (directory_entries[65].stored_extent + directory_entries[66].stored_extent + directory_entries[45].stored_extent + directory_entries[46].stored_extent + directory_entries[47].stored_extent) * 12'
    type: 'page_descriptor_group(directory_entries[48].stored_extent)'
    if: directory_entries[48].stored_extent > 0
  section_41_page_descriptors:
    pos: 'page_descriptor_suffix_offset + (directory_entries[65].stored_extent + directory_entries[66].stored_extent + directory_entries[45].stored_extent + directory_entries[46].stored_extent + directory_entries[47].stored_extent + directory_entries[48].stored_extent) * 12'
    type: 'page_descriptor_group(directory_entries[41].stored_extent)'
    if: directory_entries[41].stored_extent > 0
  section_42_page_descriptors:
    pos: 'page_descriptor_suffix_offset + (directory_entries[65].stored_extent + directory_entries[66].stored_extent + directory_entries[45].stored_extent + directory_entries[46].stored_extent + directory_entries[47].stored_extent + directory_entries[48].stored_extent + directory_entries[41].stored_extent) * 12'
    type: 'page_descriptor_group(directory_entries[42].stored_extent)'
    if: directory_entries[42].stored_extent > 0
  section_74_page_descriptors:
    pos: 'page_descriptor_suffix_offset + (directory_entries[65].stored_extent + directory_entries[66].stored_extent + directory_entries[45].stored_extent + directory_entries[46].stored_extent + directory_entries[47].stored_extent + directory_entries[48].stored_extent + directory_entries[41].stored_extent + directory_entries[42].stored_extent) * 12'
    type: 'page_descriptor_group(directory_entries[74].stored_extent)'
    if: num_directory_entries > 74 and directory_entries[74].stored_extent > 0
  num_section_41_records:
    value: 'directory_entries[41].stored_extent > 0 ? section_41_page_descriptors.num_records : 0'
  num_section_42_records:
    value: 'directory_entries[42].stored_extent > 0 ? section_42_page_descriptors.num_records : 0'
  num_section_45_records:
    value: 'directory_entries[45].stored_extent > 0 ? section_45_page_descriptors.num_records : 0'
  num_section_46_records:
    value: 'directory_entries[46].stored_extent > 0 ? section_46_page_descriptors.num_records : 0'
  num_section_47_records:
    value: 'directory_entries[47].stored_extent > 0 ? section_47_page_descriptors.num_records : 0'
  num_section_48_records:
    value: 'directory_entries[48].stored_extent > 0 ? section_48_page_descriptors.num_records : 0'
  num_section_65_records:
    value: 'directory_entries[65].stored_extent > 0 ? section_65_page_descriptors.num_records : 0'
  num_section_66_records:
    value: 'directory_entries[66].stored_extent > 0 ? section_66_page_descriptors.num_records : 0'
  num_section_74_records:
    value: 'num_directory_entries > 74 and directory_entries[74].stored_extent > 0 ? section_74_page_descriptors.num_records : 0'
    doc: |
      The section 26 page descriptors give this number of 276-byte objects.
      `directory_entries[74].num_items` is not the number of saved records.
  section_41_pages:
    pos: section_41_offset
    type: section_41_paged_controller
  section_42_offset:
    value: 'section_41_offset + num_section_41_records * (version == 0x2017 ? 180 : 188)'
  section_42_pages:
    pos: section_42_offset
    type: section_42_paged_controller
  section_45_offset:
    value: 'section_42_offset + num_section_42_records * 80'
  section_45_pages:
    pos: section_45_offset
    type: section_45_paged_controller
  section_46_offset:
    value: 'section_45_offset + num_section_45_records * (version == 0x2017 ? 116 : 124)'
  section_46_pages:
    pos: section_46_offset
    type: section_46_paged_controller
  num_live_section_46_records:
    value: section_46_pages.num_live
    doc: |
      This is the number of live section 46 heap records. A slot is free when
      bit 31 of its first word is set. A null via-type-set handle also identifies
      an unused slot. Each live slot has one next section 51 state word.
  section_47_offset:
    value: 'section_46_offset + num_section_46_records * (version <= 0x2019 ? 32 : 40)'
  section_47_pages:
    pos: section_47_offset
    type: section_47_paged_controller
  section_48_offset:
    value: 'section_47_offset + num_section_47_records * 24'
  section_48_pages:
    pos: section_48_offset
    type: section_48_paged_controller
  section_48_diff_pair_records:
    pos: 'section_48_offset - 8'
    type: section_48_diff_pair_record_array
    if: version >= 0x2024 and num_section_48_records > 0
    doc: These are the logical 864-byte section 48 slots. The physical controller ring begins eight bytes into record zero.
  section_49_physical_offset:
    value: 'section_48_offset + num_section_48_records * (version <= 0x2019 ? 48 : (version <= 0x2022 ? 856 : 864))'
    doc: This is the offset after the six paged controllers.
  section_49_relationships:
    pos: section_49_physical_offset
    type: section_49_relationship_stream
    size: directory_entries[49].stored_extent
    doc: |
      Each live signal has two counted relationship arrays. The arrays continue
      to the declared end of the controller.

      Some format variants include the unassigned-obstacles slot. Other format
      variants omit this slot. We believe that this difference changes the
      relationship-record count by one.
  section_46_route_rule_states:
    pos: 'section_49_physical_offset + directory_entries[49].stored_extent'
    type: 'section_46_route_rule_state_array(num_live_section_46_records)'
    doc: |
      Each live section 46 record has one four-byte section 51 state value.
      The section 46 page descriptors give the number of values.
  section_51_physical_offset:
    value: 'section_49_physical_offset + directory_entries[49].stored_extent + num_live_section_46_records * 4'
  section_51_relationships:
    pos: section_51_physical_offset
    type: section_51_relationship_array
    size: directory_entries[51].stored_extent
  section_50_physical_offset:
    value: 'section_51_physical_offset + directory_entries[51].stored_extent'
  section_50_relationships:
    pos: section_50_physical_offset
    type: section_50_relationship_array
    size: directory_entries[50].stored_extent
  section_52_physical_offset:
    value: 'section_50_physical_offset + directory_entries[50].stored_extent'
  section_52_outline_owners:
    pos: section_52_physical_offset
    type: section_52_outline_owner_array
    size: directory_entries[52].stored_extent
  section_53_outline_pieces:
    pos: 'section_52_physical_offset + directory_entries[52].stored_extent'
    type: section_53_outline_piece_array
    size: directory_entries[53].stored_extent
  section_54_outline_vertices:
    pos: 'section_52_physical_offset + directory_entries[52].stored_extent + directory_entries[53].stored_extent'
    type: section_54_outline_vertex_array
    size: directory_entries[54].stored_extent
  section_55_outline_arcs:
    pos: 'section_52_physical_offset + directory_entries[52].stored_extent + directory_entries[53].stored_extent + directory_entries[54].stored_extent'
    type: section_55_outline_arc_array
    size: directory_entries[55].stored_extent
  section_56_physical_offset:
    value: 'section_52_physical_offset + directory_entries[52].stored_extent + directory_entries[53].stored_extent + directory_entries[54].stored_extent + directory_entries[55].stored_extent'
  string_index:
    pos: section_56_physical_offset
    type: string_index_array
    size: directory_entries[56].stored_extent
  string_pool_offset:
    value: 'section_56_physical_offset + directory_entries[56].stored_extent'
    doc: This is the section 57 offset in the serialized controller order.
  string_pool:
    pos: string_pool_offset
    type: string_pool_contents
    size: directory_entries[57].stored_extent
  section_58_physical_offset:
    value: 'string_pool_offset + directory_entries[57].stored_extent'
  section_59_physical_offset:
    value: 'section_58_physical_offset + directory_entries[58].stored_extent'
  section_59_route_headers:
    pos: section_59_physical_offset
    type: 'section_59_route_header_ring(directory_entries[59].num_items, directory_entries[59].num_items > 0 ? directory_entries[59].stored_extent / directory_entries[59].num_items : 0, version == 0x2024 ? 12 : version >= 0x2025 ? 4 : 16)'
    size: directory_entries[59].stored_extent
    if: directory_entries[59].num_items > 0
    doc: This is the circular route-header view at its fixed offset.
  section_59_to_64:
    pos: section_59_physical_offset
    type: 'section_59_to_64_stream(directory_entries[59].num_items, directory_entries[59].num_items > 0 ? directory_entries[59].stored_extent / directory_entries[59].num_items : 0, directory_entries[60].num_items, directory_entries[60].num_items > 0 ? directory_entries[60].stored_extent / directory_entries[60].num_items : 0, directory_entries[61].num_items, directory_entries[62].num_items, directory_entries[62].num_items > 0 ? directory_entries[62].stored_extent / directory_entries[62].num_items : 0, directory_entries[63].num_items, directory_entries[64].num_items)'
    size: 'directory_entries[59].stored_extent + directory_entries[60].stored_extent + directory_entries[61].stored_extent + directory_entries[62].stored_extent + directory_entries[63].stored_extent + directory_entries[64].stored_extent'
  section_65_to_66_physical_offset:
    value: 'section_59_physical_offset + directory_entries[59].stored_extent + directory_entries[60].stored_extent + directory_entries[61].stored_extent + directory_entries[62].stored_extent + directory_entries[63].stored_extent + directory_entries[64].stored_extent'
  section_65_records:
    pos: section_65_to_66_physical_offset
    type: section_65_saved_group_record
    repeat: expr
    repeat-expr: num_section_65_records
    doc: These are saved GROUP objects counted by section 65 page descriptors.
  section_66_physical_offset:
    value: 'section_65_to_66_physical_offset + num_section_65_records * 28'
  section_66_records:
    pos: section_66_physical_offset
    type:
      switch-on: version <= 0x2022
      cases:
        true: section_65_to_66_compact_net_class
        false: section_65_to_66_net_class
    repeat: expr
    repeat-expr: num_section_66_records
    doc: Section 66 paged net-class records.
  section_67_physical_offset:
    value: 'section_66_physical_offset + num_section_66_records * (version <= 0x2022 ? 28 : 280)'
  section_67_relationships:
    pos: section_67_physical_offset
    type: section_67_design_rule_relationship_array
    size: directory_entries[67].stored_extent
  section_68_clusters:
    pos: 'section_67_physical_offset + directory_entries[67].stored_extent'
    type: cluster_record_array
    size: directory_entries[68].stored_extent
  section_69_physical_offset:
    value: 'section_67_physical_offset + directory_entries[67].stored_extent + directory_entries[68].stored_extent'
  section_69_physical_storage:
    pos: section_69_physical_offset
    size: '12 + directory_entries[69].stored_extent'
    doc: This field contains the 12-byte prefix and the flat section 69 records.
  section_69_controller_leadin_state:
    pos: section_69_physical_offset
    type: section_69_controller_leadin
  section_69_layers:
    pos: 'section_69_physical_offset + 12'
    type: 'section_69_layer_record_array(version <= 0x2021 ? 6 : version == 0x2022 ? 8 : 12, directory_entries[69].num_items)'
    doc: These are the logical layer records after the fixed controller lead-in.
  section_70_physical_offset:
    value: 'section_69_physical_offset + 12 + directory_entries[69].stored_extent'
  section_70_state:
    pos: section_70_physical_offset
    type: section_70_serialized_layer_state
    size: 4
  section_71_physical_offset:
    value: 'section_70_physical_offset + 4'
  section_71_preferences:
    pos: section_71_physical_offset
    type: 'global_display_preferences(directory_entries[71].stored_extent - 4)'
    size: 'directory_entries[71].stored_extent - 4'
  section_72_physical_offset:
    value: 'section_70_physical_offset + directory_entries[71].stored_extent'
  section_72_error_conflicts:
    pos: section_72_physical_offset
    type: saved_error_conflict_record
    repeat: expr
    repeat-expr: directory_entries[72].num_items
  section_73_physical_offset:
    value: 'section_72_physical_offset + directory_entries[72].stored_extent'
  section_73_font_faces:
    pos: section_73_physical_offset
    type:
      switch-on: 'num_directory_entries <= 73 or directory_entries[73].num_items == 0 ? 0 : directory_entries[73].stored_extent / directory_entries[73].num_items'
      cases:
        40: saved_font_face_record_40_bytes
        52: saved_font_face_record
    repeat: expr
    repeat-expr: 'num_directory_entries > 73 ? directory_entries[73].num_items : 0'
    if: num_directory_entries > 73
  section_74_physical_offset:
    value: 'section_73_physical_offset + (num_directory_entries > 73 ? directory_entries[73].stored_extent : 0)'
  section_74_records:
    pos: section_74_physical_offset
    type: extended_layer_state_record
    repeat: expr
    repeat-expr: num_section_74_records
    if: num_directory_entries > 74
  post_layer_database:
    pos: 'section_74_physical_offset + num_section_74_records * 276'
    type: post_layer_database_stream
    size: 'footer.container_item_back_pointer - (section_74_physical_offset + num_section_74_records * 276)'

types:
  physical_file_body:
    doc: |
      This type contains sections 2 through 74 and the embedded database. The
      serialized values give all byte lengths. Root instances give logical
      views of circular records that cross controller boundaries.
    seq:
      - id: flat_controllers_2_to_27
        size: '_root.directory_entries[2].stored_extent + _root.directory_entries[3].stored_extent + _root.directory_entries[4].stored_extent + _root.directory_entries[5].stored_extent + _root.directory_entries[6].stored_extent + _root.directory_entries[7].stored_extent + _root.directory_entries[8].stored_extent + _root.directory_entries[9].stored_extent + _root.directory_entries[10].stored_extent + _root.directory_entries[11].stored_extent + _root.directory_entries[12].stored_extent + _root.directory_entries[13].stored_extent + _root.directory_entries[14].stored_extent + _root.directory_entries[15].stored_extent + _root.directory_entries[16].stored_extent + _root.directory_entries[17].stored_extent + _root.directory_entries[18].stored_extent + _root.directory_entries[19].stored_extent + _root.directory_entries[20].stored_extent + _root.directory_entries[21].stored_extent + _root.directory_entries[22].stored_extent + _root.directory_entries[23].stored_extent + _root.directory_entries[24].stored_extent + _root.directory_entries[25].stored_extent + _root.directory_entries[26].stored_extent + _root.directory_entries[27].stored_extent'
        doc: This is the physical storage for flat controllers 2 through 27.
      - id: flat_controller_28
        size: _root.directory_entries[28].stored_extent
        doc: We believe that this section 28 controller has no records.
      - id: flat_controller_29
        size: _root.directory_entries[29].stored_extent
        doc: This is the route-object handles grouped by section 27 layer counts.
      - id: paged_controller_41
        size: '_root.num_section_41_records * (_root.version == 0x2017 ? 180 : 188)'
        doc: Clearance-rule records counted by section 41 page descriptors.
      - id: paged_controller_42
        size: '_root.num_section_42_records * 80'
        doc: High-speed-rule records counted by section 42 page descriptors.
      - id: paged_controller_45
        size: '_root.num_section_45_records * (_root.version == 0x2017 ? 116 : 124)'
        doc: Per-layer rule records counted by section 45 page descriptors.
      - id: paged_controller_46
        size: '_root.num_section_46_records * (_root.version <= 0x2019 ? 32 : 40)'
        doc: Route-rule heap slots counted by section 46 page descriptors.
      - id: paged_controller_47
        size: '_root.num_section_47_records * 24'
        doc: Route-rule relationship records counted by section 47 page descriptors.
      - id: paged_controller_48
        size: '_root.num_section_48_records * (_root.version <= 0x2019 ? 48 : (_root.version <= 0x2022 ? 856 : 864))'
        doc: Differential-pair slots counted by section 48 page descriptors.
      - id: flat_controller_49
        size: _root.directory_entries[49].stored_extent
        doc: Two counted relationship arrays for each serialized signal.
      - id: section_46_route_rule_states
        size: '_root.num_live_section_46_records * 4'
        doc: This is one saved section 51 state word per live section 46 route-rule slot.
      - id: flat_controller_51
        size: _root.directory_entries[51].stored_extent
        doc: Section 51 relationship controller storage.
      - id: flat_controller_50
        size: _root.directory_entries[50].stored_extent
        doc: Section 50 relationship controller storage.
      - id: flat_controllers_52_to_64
        size: '_root.directory_entries[52].stored_extent + _root.directory_entries[53].stored_extent + _root.directory_entries[54].stored_extent + _root.directory_entries[55].stored_extent + _root.directory_entries[56].stored_extent + _root.directory_entries[57].stored_extent + _root.directory_entries[58].stored_extent + _root.directory_entries[59].stored_extent + _root.directory_entries[60].stored_extent + _root.directory_entries[61].stored_extent + _root.directory_entries[62].stored_extent + _root.directory_entries[63].stored_extent + _root.directory_entries[64].stored_extent'
        doc: These controllers contain pours, strings, and route objects in serialized order.
      - id: paged_controller_65
        size: '_root.num_section_65_records * 28'
        doc: These are saved GROUP records counted by section 65 page descriptors.
      - id: paged_controller_66
        size: '_root.num_section_66_records * (_root.version <= 0x2022 ? 28 : 280)'
        doc: These are saved net-class records counted by section 66 page descriptors.
      - id: flat_controller_67
        size: _root.directory_entries[67].stored_extent
        doc: These are design-rule relationship records.
      - id: flat_controller_68
        size: _root.directory_entries[68].stored_extent
        doc: Named part-cluster records.
      - id: flat_controller_69
        size: '12 + _root.directory_entries[69].stored_extent'
        doc: This is a 12-byte layer-controller lead-in followed by physical layer records.
      - id: serialized_controllers_70_to_71
        size: _root.directory_entries[71].stored_extent
        doc: This is one four-byte section 70 state followed by the versioned section 71 preference object.
      - id: flat_controller_72
        size: _root.directory_entries[72].stored_extent
        doc: These are saved error-conflict records.
      - id: flat_controller_73
        size: '_root.num_directory_entries > 73 ? _root.directory_entries[73].stored_extent : 0'
        doc: These are saved font-face records.
      - id: paged_controller_74
        size: '_root.num_directory_entries > 74 ? _root.num_section_74_records * 276 : 0'
        doc: Extended-layer records counted by section 74 section 26 page descriptors.
      - id: post_layer_database
        type: post_layer_database_stream
        size: '_io.size - _io.pos'
        doc: Nested database/controller stream ending at the footer back-pointer.


  # =========================================================================
  # CONTAINER
  # =========================================================================
  post_layer_database_stream:
    doc: |
      This stream contains the PowerSYS database, reuse controllers, attribute
      controllers, geometry lists, and string allocators. Sections 70 through
      74 occur before this stream in serialized order.
    seq:
      - id: database_header
        type: embedded_database_header
      - id: reuse_entity
        type: legacy_fixed_header_controller(0, 52)
      - id: reuse_entity_component
        type: legacy_fixed_header_controller(1, 48)
      - id: reuse_entity_signal
        type: legacy_fixed_header_controller(2, 48)
      - id: reuse
        type: legacy_fixed_header_controller(3, 56)
      - id: reuse_component
        type: legacy_fixed_header_controller(4, 48)
      - id: reuse_signal
        type: legacy_fixed_header_controller(5, 48)
      - id: attribute_type
        type: legacy_fixed_header_controller(100, 68)
      - id: attribute_type_bool
        type: legacy_fixed_header_controller(101, 28)
      - id: attribute_type_int
        type: legacy_fixed_header_controller(102, 28)
      - id: attribute_type_double
        type: legacy_fixed_header_controller(103, 28)
      - id: attribute_type_quantity
        type: legacy_fixed_header_controller(104, 40)
      - id: attribute_unit
        type: legacy_fixed_header_controller(105, 52)
      - id: attribute_type_list
        type: legacy_fixed_header_controller(106, 36)
      - id: attribute_text_item
        type: legacy_fixed_header_controller(107, 48)
      - id: attribute_type_text
        type: legacy_fixed_header_controller(108, 28)
      - id: attribute_value
        type: legacy_fixed_header_controller(109, 68)
      - id: attribute_inheritance
        type: legacy_fixed_header_controller(110, 36)
      - id: post_controller_counts
        type: embedded_database_post_controller_counts
      - id: misc_geometry
        type: legacy_misc_geometry_controller
      - id: strings
        type: legacy_string_controller
    instances:
      consumed_all:
        value: _io.eof
        doc: This is true only when the structured trailing database ends at the container-item back-pointer.

  global_display_preferences:
    params:
      - id: len_preferences
        type: u4
    seq:
      - id: legacy_preference_words
        type: u4
        repeat: expr
        repeat-expr: 23
        if: len_preferences == 92
        doc: |
          This 92-byte v0x2017 object contains 23 display, viewport,
          layer-selection, and editor-state words.
      - id: modern_preferences
        type: 'modern_global_display_preferences(len_preferences)'
        if: len_preferences > 96
    instances:
      consumed_all:
        value: _io.eof
        doc: This is true only when the versioned preference structure consumes all section 71 bytes.

  modern_global_display_preferences:
    params:
      - id: len_preferences
        type: u4
    doc: |
      Section 71 contains the global display preferences. A 100-byte header is
      followed by ten display-configuration slots and one font-state word.

      The slot length is 100 bytes through v0x2025. It is 104 bytes in v0x2026
      and v0x2027. Each slot contains a name and a viewport rectangle. A
      104-byte slot also contains flags.
    seq:
      - id: display_flags
        type: u4
        doc: This is the packed global visibility and editor-option bits.
      - id: primary_display_color_index
        type: u4
        doc: The global-preference initializer sets this field to palette index 15.
      - id: secondary_display_color_index
        type: u4
        doc: The global-preference initializer sets this field to palette index 14.
      - id: display_mode
        type: u4
      - id: editor_display_state
        size: 44
        doc: Persisted editor and stroke-font display state.
      - id: saved_first_layer_handle
        type: u4
        doc: This is the saved reference that identifies the first physical layer.
      - id: saved_last_layer_handle
        type: u4
        doc: This is the saved reference that identifies the last physical layer.
      - id: retained_layer_reference_state
        size: 8
        doc: This is the retained layer-selection reference state.
      - id: current_viewport_rectangle
        type: rectangle_i32
        doc: This is the saved viewport rectangle.
      - id: code_page
        type: u4
        doc: Windows character-set/code-page state. Initialized from GetACP.
      - id: display_configuration_state
        type: u4
        doc: This is the saved display-configuration selection/state word. Not an array count.
      - id: display_configurations
        type: 'global_display_configuration((len_preferences - 104) / 10)'
        repeat: expr
        repeat-expr: 10
      - id: default_font_face_handle
        type: u4
        doc: Tagged ODBFontFace handle. Low byte is the saved font ordinal (0..7), or zero when unset.

  section_70_serialized_layer_state:
    doc: |
      Section 70 contains one four-byte layer-state word. Its directory entry
      gives a 16-byte memory size. The other 12 bytes are not serialized.
    seq:
      - id: layer_state
        type: u4

  global_display_configuration:
    params:
      - id: record_stride
        type: u4
    doc: Fixed-capacity named global display/viewport configuration slot.
    seq:
      - id: name_storage
        size: 84
        doc: NUL-padded ASCII for live named slots. Unused capacity retains arbitrary bytes.
      - id: viewport_rectangle
        type: rectangle_i32
      - id: configuration_flags
        type: u4
        if: record_stride == 104

  extended_layer_state_record:
    doc: |
      This optional 276-byte layer extension occurs after section 73. Its last
      fields contain layer handles, a fixed-length layer name, and retained
      layer state.
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
        doc: We believe that this is retained layer-object state.

  saved_font_face_record:
    doc: |
      This is a 52-byte section 73 font-face record. The two fixed string fields
      can contain retained suffix bytes. These bytes are object state, not
      padding.
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

  saved_font_face_record_40_bytes:
    doc: |
      This is a 40-byte v0x2019 font-face record. Two identification words occur
      before the fixed 32-byte face-name field.
    seq:
      - id: saved_font_handle
        type: u4
      - id: font_ordinal
        type: u4
      - id: face_name_storage
        size: 32

  saved_error_conflict_record:
    doc: |
      This is a 32-byte section 72 error-conflict record. We believe that the
      last word is retained record state. It is not padding.
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
      This stream contains string headers and string data. The primary pages
      contain fixed eight-byte string-header records. The next data pages
      contain capacity-framed string-data slots.
    seq:
      - id: title
        type: legacy_short_string
      - id: num_header_pages
        type: u4
      - id: num_live_headers
        type: u4
        if: num_header_pages > 0
      - id: header_pages
        type: serialized_string_header_page
        repeat: expr
        repeat-expr: num_header_pages
      - id: num_data_pages
        type: u4
        if: num_header_pages > 0
      - id: data_pages
        type: serialized_normal_page
        repeat: expr
        repeat-expr: num_data_pages
        if: num_header_pages > 0

  embedded_database_header:
    doc: |
      This is the nested PowerSYS database header. It contains 13 state words
      and a byte directory. `num_controller_directory_entries` gives the byte-
      directory length.
    seq:
      - id: power_sys_archive_tag
        type: u4
        doc: This is the saved MFC archive tag before the PowerSYS class name. Never a file offset.
      - id: title
        type: legacy_short_string
      - id: database_version
        type: u2
      - id: storage_flags
        type: u2
      - id: database_state
        type: u4
        repeat: expr
        repeat-expr: 4
      - id: num_controller_directory_entries
        type: u4
      - id: database_state_tail
        type: u4
        repeat: expr
        repeat-expr: 8
      - id: controller_directory_entries
        type: u1
        repeat: expr
        repeat-expr: num_controller_directory_entries

  embedded_database_post_controller_counts:
    doc: |
      These counts occur after the indexed object controllers. They occur before
      the geometry controller and the string controller. Zero values are data,
      not padding.
    seq:
      - id: num_multi_layers
        type: u4
      - id: num_geometry_primary_pages
        type: u4
      - id: num_geometry_secondary_pages
        type: u4

  legacy_misc_geometry_controller:
    doc: |
      This geometry stream has two independent page-list counts. The counts are
      present when their values are zero. We believe that the secondary list is
      not used.
    seq:
      - id: title
        type: legacy_short_string
      - id: num_primary_pages
        type: u4
      - id: primary_pages
        type: 'legacy_allocator_page(0xffffffff)'
        repeat: expr
        repeat-expr: num_primary_pages
      - id: num_secondary_pages
        type: u4

  serialized_string_header_page:
    doc: Saved DBM_LayerPage containing fixed DBD_StringHdr records.
    seq:
      - id: len_allocation
        type: s4
        doc: This is the saved page allocation extent. -1 marks the compact header-page allocation.
      - id: len_headers
        type: u4
        valid:
          max: 0xfff0
      - id: saved_page_base
        type: u4
        doc: This is the original allocation base used for pointer relocation.
      - id: headers
        type: string_header_page_contents
        size: len_headers

  string_header_page_contents:
    doc: This fixed-size DBD_StringHdr array fills the page.
    seq:
      - id: headers
        type: saved_string_header_record
        repeat: eos

  saved_string_header_record:
    doc: Saved DBD_StringHdr linking a stable string handle to its DBD_StringData slot.
    seq:
      - id: saved_string_data_handle
        type: u4
      - id: header_state
        type: u4

  serialized_normal_page_list:
    doc: |
      This is a counted list of 64-KiB database allocator pages. The list ends at
      the container-item back-pointer. Each page except the last page contains
      65,520 live bytes. The last page can contain fewer live bytes.
    seq:
      - id: num_pages
        type: u4
      - id: pages
        type: serialized_normal_page
        repeat: expr
        repeat-expr: num_pages

  legacy_short_string:
    doc: MFC CArchive short CString encoding used by database-controller titles.
    seq:
      - id: len_value
        type: u1
      - id: value
        type: str
        size: len_value
        encoding: ASCII

  legacy_database_controller_header:
    doc: This is the controller identity and primary allocator-page count.
    seq:
      - id: controller_index
        type: u4
      - id: title
        type: legacy_short_string
      - id: num_primary_pages
        type: u4

  legacy_fixed_header_state:
    doc: |
      DBC_FixHdr saved-list state. The first three words identify the saved
      first/last handles and live object count. The remaining four words are
      intrusive-list and allocator state retained by the controller.
    seq:
      - id: saved_first_handle
        type: u4
      - id: saved_last_handle
        type: u4
      - id: num_live_records
        type: u4
      - id: list_state
        type: u4
        repeat: expr
        repeat-expr: 4

  legacy_fixed_header_controller:
    params:
      - id: controller_id
        type: u4
      - id: record_stride
        type: u4
    doc: |
      Serialized DBC_FixHdr controller. Empty controllers stop after the standard
      header. Non-empty controllers carry fixed-record pages, three version
      words, then a counted list of variable-member allocator pages. The zeros
      next AttributeInheritance belong to the enclosing database counts.
    seq:
      - id: header
        type: legacy_database_controller_header
      - id: fixed_header_state
        type: legacy_fixed_header_state
        if: header.num_primary_pages > 0
      - id: primary_pages
        type: legacy_fixed_object_page(controller_id, record_stride)
        repeat: expr
        repeat-expr: header.num_primary_pages
      - id: version_state
        type: u4
        repeat: expr
        repeat-expr: 3
        if: header.num_primary_pages > 0
      - id: num_secondary_pages
        type: u4
        if: header.num_primary_pages > 0
      - id: secondary_pages
        type: 'legacy_allocator_page(controller_id)'
        repeat: expr
        repeat-expr: num_secondary_pages
        if: header.num_primary_pages > 0

  legacy_fixed_object_page:
    params:
      - id: controller_id
        type: u4
      - id: record_stride
        type: u4
    doc: Saved 64 KiB DBC_FixHdr page framed by class, live length, and relocation base.
    seq:
      - id: controller_type
        type: u4
        valid: controller_id
      - id: len_payload
        type: u4
        valid:
          max: 0xfff0
      - id: saved_page_base
        type: u4
        doc: This is the original 64 KiB-aligned allocation base used for pointer relocation.
      - id: payload
        type: 'legacy_fixed_object_page_payload(controller_id, record_stride)'
        size: len_payload

  legacy_fixed_object_page_payload:
    params:
      - id: controller_id
        type: u4
      - id: record_stride
        type: u4
    doc: Full saved objects followed by any retained partial allocator slot.
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
        doc: Occupied bytes of a last incomplete capacity slot. Retained data, not padding.

  saved_partial_capacity_value:
    params:
      - id: controller_id
        type: u4
    doc: This is one retained four-byte fragment of the controller's next fixed-record capacity slot.
    seq:
      - id: retained_value
        type: u4

  saved_fixed_header_object_header:
    doc: |
      This 24-byte header contains list links and variable-member links.
      A nonzero next or previous handle identifies a record in the same
      controller. The reciprocal field points back to this record.
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
    doc: This is a 28-byte Bool/Int/Double/Text specialization linked to its base AttributeType.
    seq:
      - id: header
        type: saved_fixed_header_object_header
      - id: base_attribute_type_handle
        type: u4

  saved_attribute_quantity_type_record:
    doc: This is a 40-byte quantity specialization with its unit relationship.
    seq:
      - id: header
        type: saved_fixed_header_object_header
      - id: base_attribute_type_handle
        type: u4
      - id: next_quantity_type_handle
        type: u4
      - id: previous_quantity_type_handle
        type: u4
      - id: attribute_unit_handle
        type: u4

  saved_attribute_unit_record:
    doc: This is a 52-byte attribute-unit object with three string fields and quantity-type binding.
    seq:
      - id: header
        type: saved_fixed_header_object_header
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
    doc: This is a 36-byte list specialization owning a counted AttributeTextItem list.
    seq:
      - id: header
        type: saved_fixed_header_object_header
      - id: base_attribute_type_handle
        type: u4
      - id: first_text_item_handle
        type: u4
      - id: num_text_items
        type: u4

  saved_attribute_text_item_record:
    doc: This is a 48-byte named list item with intrusive links, owner type, and optional value binding.
    seq:
      - id: header
        type: saved_fixed_header_object_header
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
    doc: This is a 36-byte inheritance edge between an owning value/object and an AttributeType.
    seq:
      - id: header
        type: saved_fixed_header_object_header
      - id: next_inheritance_handle
        type: u4
      - id: previous_inheritance_handle
        type: u4
      - id: attribute_type_handle
        type: u4

  saved_attribute_type_record:
    doc: This is a 68-byte base attribute definition with name and three counted relationship lists.
    seq:
      - id: header
        type: saved_fixed_header_object_header
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
      - id: num_text_items
        type: u4
      - id: first_value_handle
        type: u4
      - id: num_values
        type: u4
      - id: first_inheritance_handle
        type: u4
      - id: num_inheritances
        type: u4
      - id: specialized_type_handle
        type: u4

  saved_attribute_value_record:
    doc: This is a 68-byte attribute value with owner links, type binding, value links, and optional text item.
    seq:
      - id: header
        type: saved_fixed_header_object_header
      - id: next_owner_value_handle
        type: u4
      - id: previous_owner_value_handle
        type: u4
      - id: owner_value_state_handle
        type: u4
      - id: first_child_value_handle
        type: u4
      - id: num_child_values
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
    doc: This is a 52-byte reuse-entity root with component and signal collections.
    seq:
      - id: header
        type: saved_fixed_header_object_header
      - id: name_string_handle
        type: u4
      - id: reuse_handle
        type: u4
      - id: entity_state
        type: u4
      - id: first_component_handle
        type: u4
      - id: num_components
        type: u4
      - id: first_signal_handle
        type: u4
      - id: num_signals
        type: u4

  saved_reuse_record:
    doc: This is a 56-byte reuse definition with entity, component, and signal relationships.
    seq:
      - id: header
        type: saved_fixed_header_object_header
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
      - id: num_components
        type: u4
      - id: first_signal_handle
        type: u4
      - id: num_signals
        type: u4

  saved_reuse_entity_component_record:
    doc: This is a 48-byte component membership linking a reuse entity to a reuse component.
    seq:
      - id: header
        type: saved_fixed_header_object_header
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
    doc: This is a 48-byte reuse component linked to its entity-component membership.
    seq:
      - id: header
        type: saved_fixed_header_object_header
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
    doc: This is a 48-byte signal membership linking a reuse entity to a reuse signal.
    seq:
      - id: header
        type: saved_fixed_header_object_header
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
    doc: This is a 48-byte reuse signal linked to its entity-signal membership.
    seq:
      - id: header
        type: saved_fixed_header_object_header
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
    doc: Variable-member allocator page referenced by saved fixed objects.
    seq:
      - id: len_logical_allocation
        type: u4
        doc: This is the logical controller-allocation length.
      - id: len_payload
        type: u4
        valid:
          max: 0xfff0
      - id: saved_page_base
        type: u4
        doc: This is the original allocation base used for pointer relocation.
      - id: payload
        type: 'legacy_allocator_page_payload(controller_id)'
        size: len_payload

  legacy_allocator_page_payload:
    params:
      - id: controller_id
        type: u4
    doc: |
      This payload contains controller-specific variable-member blocks.
      Geometry pages contain word-addressed allocator storage. They do not
      contain fixed-size objects. A free slot can contain a free-list link.
    seq:
      - id: member_values
        type:
          switch-on: controller_id
          cases:
            0xffffffff: saved_misc_geometry_allocator_word
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

  saved_misc_geometry_allocator_word:
    doc: |
      This is one 32-bit word from a geometry allocator page. The page contains
      variable-length geometry state and allocator links. Thus, the page is
      word-addressed and does not have a fixed record length.
    seq:
      - id: value_or_free_link
        type: u4

  saved_modern_variable_member_owner:
    doc: 'Allocation backlink added in v0x2024: owning fixed-record ordinal and retained/free state.'
    seq:
      - id: owning_fixed_record_ordinal
        type: u4
      - id: allocation_state
        type: u4

  saved_reuse_entity_variable_member:
    doc: This is a 24-byte ReuseEntity variable block.
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
    doc: Eight-byte collection membership state used by Reuse component and signal controllers.
    seq:
      - id: membership_value_or_free_link
        type: u4
      - id: membership_state
        type: u4

  saved_reuse_variable_member:
    doc: This is a 60-byte Reuse variable block.
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
    doc: Base AttributeType flags and display/style code.
    seq:
      - id: enabled_or_free_link
        type: u4
      - id: display_style
        type: u4
      - id: owner
        type: saved_modern_variable_member_owner
        if: _root.version >= 0x2024

  saved_attribute_bool_variable_member:
    doc: Boolean AttributeType default and state.
    seq:
      - id: default_value_or_free_link
        type: u4
      - id: value_state
        type: u4
      - id: owner
        type: saved_modern_variable_member_owner
        if: _root.version >= 0x2024

  saved_attribute_int_variable_member:
    doc: Integer AttributeType default, minimum, and maximum.
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
    doc: Floating-point AttributeType default, minimum, and maximum.
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
    doc: Quantity AttributeType state followed by minimum and maximum floating-point values.
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
    doc: AttributeUnit formatting state plus affine conversion offset and scale.
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
    doc: List AttributeType default-item selection and state.
    seq:
      - id: default_item_or_free_link
        type: u4
      - id: list_state
        type: u4
      - id: owner
        type: saved_modern_variable_member_owner
        if: _root.version >= 0x2024

  saved_attribute_text_item_variable_member:
    doc: AttributeTextItem selection and state.
    seq:
      - id: selected_or_free_link
        type: u4
      - id: item_state
        type: u4
      - id: owner
        type: saved_modern_variable_member_owner
        if: _root.version >= 0x2024

  saved_attribute_text_variable_member:
    doc: Text AttributeType state and text constraint.
    seq:
      - id: enabled_or_free_link
        type: u4
      - id: text_state
        type: u4
      - id: owner
        type: saved_modern_variable_member_owner
        if: _root.version >= 0x2024

  saved_attribute_value_variable_member:
    doc: AttributeValue discriminator and 64-bit scalar/text payload.
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
    doc: AttributeInheritance relationship state, scope kind, and inheritance mode.
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
      This is a serialized database allocator page. `len_allocation` gives the
      memory-page length. `saved_page_base` is the pointer-relocation base. The
      payload contains capacity-framed string slots.
    seq:
      - id: len_allocation
        type: u4
        valid: 0x10000
        doc: This is a 64 KiB allocation that includes the reconstructed 16-byte runtime header.
      - id: len_string_allocator_slots
        type: u4
        valid:
          max: 0xfff0
        doc: Live page bytes copied to memory offset 16. 0xFFF0 on every non-last normal page.
      - id: saved_page_base
        type: u4
        doc: This is the original 64 KiB-aligned allocation base used to relocate serialized pointers.
      - id: string_allocator_slots
        type: string_allocator_page_contents
        size: len_string_allocator_slots
        doc: This is the active strings and retained free/capacity storage.

  string_allocator_page_contents:
    doc: DBC_StringCtl allocator slots that fill the page.
    seq:
      - id: slots
        type: string_allocator_slot
        repeat: eos

  string_allocator_slot:
    doc: |
      This is a capacity-framed string slot. `len_active_contents` is 0xFFFF for
      a free slot. Otherwise, it gives the number of active bytes.

      Active content is usually a NUL-terminated ASCII string. It can also
      contain binary controller values. Retained capacity is not padding.
    seq:
      - id: capacity
        type: u2
        valid:
          min: 1
      - id: len_active_contents
        type: u2
        doc: This is the active byte count, or 0xFFFF when this allocator slot is free.
      - id: active_contents
        size: len_active_contents
        if: len_active_contents != 0xffff
        doc: This is the active string or binary controller value.
      - id: retained_capacity_contents
        size: 'len_active_contents == 0xffff ? capacity : capacity - len_active_contents'
        doc: Free-slot contents or unused capacity retaining prior bytes. Never file padding.

  directory_entry:
    params:
      - id: has_trailing_slot_word
        type: bool
    doc: |
      This is a four-byte-shifted controller-slot view. It contains three u32
      fields and the zero word from the next slot. The last entry does
      not have the next word.

      The meaning of `stored_extent` depends on the controller type. For a flat
      controller, it is the byte length. For a paged controller, it is the
      number of section 26 page descriptors.
    seq:
      - id: num_items
        type: u4
        doc: Record or allocation-unit count.
      - id: stored_extent
        type: u4
        doc: |
          This value is the data-region byte length for a flat controller. It is
          the number of page descriptors for a paged controller.
      - id: in_memory_base
        type: u4
        doc: |
          This is the serialized 32-bit memory base address. It is not a file
          offset. A nonzero address can identify a memory-allocation chain.
      - id: base_pointer_high_padding
        contents: [0, 0, 0, 0]
        if: has_trailing_slot_word
        doc: |
          This is the zero high word of the 32-bit memory base pointer.

  # =========================================================================
  # SECTION 56/57 -- the attribute string index and its pool
  # =========================================================================
  string_index_entry:
    doc: |
      This 16-byte section 56 record points into the section 57 string pool.
      Section 57 starts immediately after section 56. The directory entries give
      the byte lengths of both sections.

      Adjacent index records obey this relationship:

          pool_offset[k] + len_string[k] == pool_offset[k+1]

      The index does not have to include the first string. Section 57 can have
      retained bytes before or after the indexed strings. An attribute value can
      contain binary controller data.
    seq:
      - id: pool_offset
        type: u4
        doc: This is the byte offset of this string within section 57's pool.
      - id: len_string
        type: u2
        doc: This is the string length in bytes.
      - id: handle_a
        type: u2
      - id: handle_b
        type: u2
      - id: handle_c
        type: u2
        doc: The value is 0xFFFF when this handle is not used.
      - id: record_metadata
        type: u4
        doc: |
          This is serialized record metadata, not padding. We believe that some
          values are graphics-configuration masks or packed values.

  string_index_array:
    seq:
      - id: entries
        type: string_index_entry
        repeat: eos

  string_pool_contents:
    doc: |
      Section-57 indexed string storage. Most values are NUL-terminated ASCII.
      Some attribute values intentionally contain binary controller bytes. Each
      byte is owned by a string-index entry or retained pool string tail.
    seq:
      - id: indexed_string_bytes
        size-eos: true

  container_item_array:
    doc: |
      This is an array of serialized OLE container items. Each item contains a
      length-delimited Microsoft Compound File Binary document and PADS view
      state.
    seq:
      - id: num_items
        type: u4
        doc: This is the number of serialized CPowerPCBCntrItem objects.
      - id: mfc_new_class_tag
        contents: [0xff, 0xff]
        if: num_items > 0
        doc: MFC CArchive new-class tag.
      - id: mfc_schema
        type: u2
        if: num_items > 0
        doc: MFC runtime-class schema number.
      - id: len_class_name
        type: u2
        if: num_items > 0
        doc: MFC runtime-class name length. 17 for CPowerPCBCntrItem.
      - id: class_name
        type: str
        size: len_class_name
        encoding: ASCII
        if: num_items > 0
        doc: MFC runtime-class name CPowerPCBCntrItem.
      - id: items
        type: powerpcb_container_item
        repeat: expr
        repeat-expr: num_items
        doc: This is the serialized embedded OLE items.
    instances:
      consumed_all:
        value: _io.eof
        doc: This is true only when the container-item stream ends at the fixed footer.

  powerpcb_container_item:
    doc: |
      This is a serialized PowerPCB OLE container item. It contains the OLE item
      state, an embedded compound file, and the PADS view state.
    seq:
      - id: ole_item_format_version
        type: u4
        valid: 0x100
        doc: COleClientItem serialization format marker.
      - id: item_number
        type: u4
        doc: COleClientItem document-item ordinal.
      - id: ole_object_reference
        type: u4
        doc: MFC CArchive object reference for the linked OLE object.
      - id: link_unavailable
        type: u2
        doc: COleClientItem link-unavailable flag serialized as a u16.
      - id: draw_aspect
        type: u4
        doc: OLE DVASPECT value.
      - id: len_compound_file
        type: u4
        doc: This is the byte length of the next Microsoft CFB document.
      - id: compound_file
        type: microsoft_cfb
        size: len_compound_file
        doc: |
          Embedded OLE compound document containing the linked item's streams.
      - id: document_guid
        type: str
        size: 38
        encoding: ASCII
        doc: PADS PCB document GUID {2FE18320-6448-11d1-A412-000000000000}.
      - id: database_box
        type: rectangle_i32
        doc: PADS database-coordinate bounding box written from object offset 0x84.
      - id: original_pixel_rect
        type: serialized_rectangle
        doc: This is the original pixel rectangle. The CRect fields are serialized as left, bottom, right, and top.
      - id: plane
        type: u4
        doc: Display plane from CPowerPCBCntrItem object offset 0xA4.
      - id: white_background
        type: u4
        doc: White-background display flag from object offset 0xA8.

  rectangle_i32:
    seq:
      - id: left
        type: s4
      - id: top
        type: s4
      - id: right
        type: s4
      - id: bottom
        type: s4

  serialized_rectangle:
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
    doc: MFC document footer after the serialized container-item array.
    seq:
      - id: guid
        type: str
        size: 38
        encoding: ASCII
        doc: Footer GUID {2FE18320-6448-11d1-A412-000000000000}.
      - id: container_item_back_pointer
        type: u4
        doc: |
          This is the absolute file offset of the container-item array. A u32
          item count occurs at this offset. The value is not a size or a
          checksum.

  # =========================================================================
  # SECTION 0 — file-global object header (no data region)
  # =========================================================================
  # Directory slot 0 contains the data. `data_offset` is zero, so there is no region.
  section_0_global_header:
    doc: |
      Section 0 has no data region. Its directory slot contains this global
      header. `num_global_objects` is the sum of the item counts in sections 2
      through N-1. `global_payload_bytes` is the sum of their byte lengths.
    seq:
      - id: num_global_objects
        type: u4
      - id: global_payload_bytes
        type: u4
      - id: in_memory_base
        type: u4
        doc: This is the serialized controller memory base address.
      - id: base_pointer_high_padding
        contents: [0, 0, 0, 0]
        doc: This is the zero high word of the 32-bit memory pointer.

  # =========================================================================
  # SECTION 1 — board setup / view-state header (single 1200-B blob)
  # =========================================================================
  # This section is not a record array. Use this expression for its location:
  #
  #     base = 10 + (directory_entries[1].num_items - 1)*16 + directory_entries[2].num_items*48
  #
  # The first term uses the stored number of controller slots. The second term
  # is the byte length of the section 2 view-state records. The origin at +60
  # and +64 applies to absolute coordinates in the board.
  section_1_board_setup:
    seq:
      - id: num_legacy_aux_records
        type: u4
        doc: At +0, this field contains the legacy auxiliary-controller state and count word. Its interpretation varies with the section 1 dialect.
      - id: len_legacy_aux_records
        type: u4
        doc: |
          At +4, this field contains the legacy auxiliary-controller allocation bytes. In old dialects
          with a live count this is count * 52. Modern files may retain
          a nonzero allocation value with count zero.
      - id: legacy_aux_memory_base
        type: u4
        doc: At +8, this field is the serialized 32-bit allocator base address.
      - id: user_grid
        type: s4
        doc: At +12, this field contains USERGRID (X. Y assumed equal).
      - id: maximum_layer
        type: s4
        doc: At +16, this field contains MAXIMUMLAYER (max routing layer).
      - id: work_level
        type: s4
        doc: At +20, this field contains WORKLEVEL.
      - id: display_level
        type: s4
        doc: At +24, this field contains DISPLAYLEVEL.
      - id: layer_pair
        type: u4
        doc: At +28, this field contains two packed layers. The low 16 bits contain layer 1, and the high 16 bits contain layer 2.
      - id: layer_pair_max
        type: u4
        doc: At +32, this field contains (maxlayer<<16)|0x41. Low byte 0x41 = VIAMODE 'A'.
      - id: line_width
        type: s4
        doc: At +36, this field contains LINEWIDTH (default item width).
      - id: text_height
        type: s4
        doc: At +40, this field contains TEXTSIZE[0].
      - id: text_line_width
        type: s4
        doc: At +44, this field contains TEXTSIZE[1].
      - id: job_time
        type: s4
        doc: At +48, this field contains JOBTIME (seconds).
      - id: dot_grid
        type: s4
        doc: At +52, this field contains DOTGRID.
      - id: view_scale
        type: f4
        doc: At +56, this field contains SCALE (view zoom. Live view state).
      - id: origin_x
        type: s4
        doc: At +60, this field contains RAW origin X. Design = raw - origin.
      - id: origin_y
        type: s4
        doc: At +64, this field contains RAW origin Y.
      - id: window_center_x
        type: s4
        doc: At +68, this field contains RAW window center X (live view state).
      - id: window_center_y
        type: s4
        doc: At +72, this field contains RAW window center Y.
      - id: backup_time
        type: s4
        doc: At +76, this field contains BACKUPTIME (minutes).
      - id: real_width
        type: s4
        doc: At +80, this field contains REAL WIDTH.
      - id: all_signal_on_off
        type: s4
        doc: At +84, this field contains ALLSIGONOFF.
      - id: view_extension_flags
        type: u4
        doc: At +88, this field contains the saved view-extension flags.
      - id: reference_name_height
        type: s4
        doc: At +92, this field contains REFNAMESIZE[0].
      - id: reference_name_line_width
        type: s4
        doc: At +96, this field contains REFNAMESIZE[1].
      - id: default_size_a
        type: s4
        doc: At +100, this field contains the constant value 457200, which is 12 mil.
      - id: default_size_b
        type: s4
        doc: At +104, this field contains the constant value 457200.
      - id: default_size_c
        type: s4
        doc: At +108, this field contains the constant value 457200.
      - id: highlight
        type: s4
        doc: At +112, this field contains HIGHLIGHT.
      - id: connection_color_flag
        type: s4
        doc: At +116, this field contains CONCOL.
      - id: unknown_zero_at_120
        contents: [0, 0, 0, 0]
        doc: At +120, this field is zero. The purpose of this field is not known.
      - id: feature_flags
        type: u4
        doc: At +124, this field contains the constant value 0x094D4000 for DB version and capability flags.
      - id: view_extension_values
        type: s4
        repeat: expr
        repeat-expr: 4
        doc: |
          Bytes +128 through +143 contain four saved view or grid values. The
          `view_extension_flags` field enables these values.
      - id: job_name_storage
        type: str
        size: 260
        encoding: ASCII
        terminator: 0
        doc: |
          Bytes +144 through +403 contain a fixed-length job-name buffer. Bytes after
          the first NUL are retained buffer capacity. They are not padding.
      - id: editor_view_state_words
        type: u4
        repeat: eos
        doc: |
          Bytes +404 through the end contain the saved editor and display state. Zero
          values identify default or disabled settings. They are not padding.

  # =========================================================================
  # SECTION 2 — persisted editor view records
  # =========================================================================
  # Section 2 occurs before section 1 in the serialized stream. Each record has
  # 48 bytes.
  section_2_view_state:
    doc: This is a 48-byte persisted editor viewport/object-view record.
    seq:
      - id: controller_state
        type: u4
      - id: object_handle
        type: u4
      - id: owner_pointer
        type: u4
        doc: This is the serialized 32-bit view/object owner reference.
      - id: center_x_raw
        type: s4
        doc: This is the saved viewport/object X in RAW database coordinates.
      - id: center_y_raw
        type: s4
        doc: This is the saved viewport/object Y in RAW database coordinates.
      - id: display_flags
        type: u4
      - id: layer_selection
        type: u4
      - id: transform_flags
        type: u4
      - id: zoom_scale
        type: f4
        doc: This is the saved view scale. Usual bit patterns include 1.875 and 15.0.
      - id: view_state_a
        type: u4
      - id: view_parameter
        type: f4
      - id: view_state_b
        type: u4

  section_2_view_state_array:
    seq:
      - id: records
        type: section_2_view_state
        repeat: eos

  # =========================================================================
  # SECTION 3 — full physical database/board-parameter controller image
  # =========================================================================
  # The directory entry gives the physical byte length. Logical board-parameter
  # views overlap saved directory and database-header state.
  section_3_physical_controller:
    doc: |
      Full flat section 3 controller image. Its prefix is the physical overlap
      with section 1: section 1 logical bytes 12..end. The overlap length is
      therefore `directory_entries[1].stored_extent - 12`. The versioned board-parameter image
      follows immediately and consumes the remainder of directory_entries[3].stored_extent.
    seq:
      - id: board_setup_overlap_tail
        size: _root.directory_entries[1].stored_extent - 12
        doc: Section 1 logical bytes 12..end, physically shared by the circular controller views.
      - id: parameters
        type: section_3_board_parameters
        size-eos: true

  section_3_board_parameters:
    doc: Version-selected section 3 board-parameter serialization.
    seq:
      - id: serialized_parameters
        type: section_3_board_parameters_modern
        size-eos: true

  flat_controller_storage_4_to_24:
    doc: |
      This is the physical storage for flat sections 4 through 24. Some logical
      record arrays rotate by 44 bytes across adjacent section boundaries. Root
      instances give the logical views.
    seq:
      - id: padstack_storage
        size: _root.directory_entries[4].stored_extent
      - id: pad_layer_storage
        size: _root.directory_entries[5].stored_extent
      - id: text_controller_storage_6
        size: _root.directory_entries[6].stored_extent
      - id: text_controller_storage_7
        size: _root.directory_entries[7].stored_extent
      - id: text_object_storage
        size: _root.directory_entries[8].stored_extent
      - id: text_drawing_bridge_storage
        size: _root.directory_entries[9].stored_extent
      - id: drawing_owner_storage
        size: _root.directory_entries[10].stored_extent
      - id: graphic_piece_storage
        size: _root.directory_entries[11].stored_extent
      - id: graphic_vertex_storage
        size: _root.directory_entries[12].stored_extent
      - id: hatch_storage
        size: _root.directory_entries[13].stored_extent
      - id: decal_descriptor_storage
        size: _root.directory_entries[14].stored_extent
      - id: decal_terminal_storage
        size: _root.directory_entries[15].stored_extent
      - id: part_type_aux_storage
        size: _root.directory_entries[16].stored_extent
      - id: part_type_storage
        size: _root.directory_entries[17].stored_extent
      - id: part_type_gate_storage
        size: _root.directory_entries[18].stored_extent
      - id: part_type_pin_storage
        size: _root.directory_entries[19].stored_extent
      - id: part_type_signal_storage
        size: _root.directory_entries[20].stored_extent
      - id: compact_pin_name_storage
        size: _root.directory_entries[21].stored_extent
      - id: placement_storage
        size: _root.directory_entries[22].stored_extent
      - id: net_storage
        size: _root.directory_entries[23].stored_extent
      - id: route_chain_storage
        size: _root.directory_entries[24].stored_extent

  section_3_board_parameters_modern:
    doc: |
      Shared board-parameter field order. Versions v0x2017 and v0x2019 contain
      eight additional palette words. Versions v0x2021 through v0x2024 contain
      four additional palette words. Version v0x2019 ends after ARPTOMLAYER.
      Version v0x2017 ends after ARDTOPLAYER. Version v0x2021 ends after VIAPFLAG.
      Versions v0x2022 and v0x2024 add FLOWFLAGS. Versions v0x2025 and later add
      five fixed auxiliary selector buffers. Version v0x2017 has 12 shifted
      parameters from HATCHGRID through STMINSPOKES.
    seq:
      - id: display_palette_words
        type: u4
        repeat: expr
        repeat-expr: '_root.version < 0x2021 ? 216 : (_root.version < 0x2025 ? 212 : 208)'
        doc: |
          Bytes +0 through +831 contain a 16-by-52-byte display-palette table. A zero
          value is default state. It is not alignment padding.
      - id: foreground_background_color_0
        type: s4
        doc: 'At +832, this field contains FBGCOL[0].'
      - id: foreground_background_color_1
        type: s4
        doc: 'At +836, this field contains FBGCOL[1].'
      - id: hatch_grid
        type: s4
        doc: At +840, this field contains HATCHGRID.
      - id: teardrop
        type: s4
        doc: At +844, this field contains TEARDROP.
      - id: default_sizes_a
        type: s4
        repeat: expr
        repeat-expr: 3
        doc: Fields +848, +852, and +856 contain the constant default sizes.
      - id: legacy_parameter_extension_words
        type: s4
        repeat: expr
        repeat-expr: 32
        doc: |
          Bytes +860 through +987 contain version-dependent board parameters. Older
          versions use the first four words for size values. Versions v0x2025
          and later use a 17-word option vector.
      - id: fixed_flag_table_marker
        type: s4
        doc: At +988, this field contains the value 8. This value is the length marker for the table below.
      - id: thermal_line_width
        type: s4
        doc: At +992, this field contains THERLINEWID.
      - id: fixed_flag_table
        type: s4
        repeat: expr
        repeat-expr: 30
        doc: Bytes +996 through +1115 contain alternating values 0x102 and 0x101. This array has a fixed size and is not per layer.
      - id: default_size_b
        type: s4
        doc: At +1116, this field contains the constant value 457200, which is 12 mil.
      - id: default_size_c
        type: s4
        doc: At +1120, this field contains the constant value 952500, which is 25 mil.
      - id: all_signal_flags
        type: u4
        doc: At +1124, this field contains ALLSIGFLAGS.
      - id: fixed_pre_drc_values
        type: s4
        repeat: expr
        repeat-expr: 8
        doc: At +1128..1159, this field contains (200,200,PSVIAGRID,6,50,50,85,3).
      - id: pad_fill_width
        type: s4
        doc: At +1160, this field contains PADFILLWID.
      - id: thermal_smd_width
        type: s4
        doc: At +1164, this field contains THERSMDWID.
      - id: minimum_hatch_area
        type: s4
        doc: At +1168, this field contains the MINHATAREA or HATCHMODE value. It is zero.
      - id: hatch_mode
        type: s4
        doc: At +1172, this field contains HATCHMODE.
      - id: hatch_display
        type: s4
        doc: At +1176, this field contains HATCHDISP.
      - id: drill_hole
        type: s4
        doc: At +1180, this field contains DRILLHOLE.
      - id: mitre_radii
        type: s4
        repeat: expr
        repeat-expr: 7
        doc: At +1184..1211, this field contains MITRERADII x1000.
      - id: mitre_type
        type: s4
        doc: At +1212, this field contains MITRETYPE.
      - id: hatch_radius
        type: s4
        doc: At +1216, this field contains HATCHRAD x1000.
      - id: mitre_angles
        type: s4
        repeat: expr
        repeat-expr: 7
        doc: At +1220..1247, this field contains MITREANG.
      - id: default_text_size
        type: s4
        doc: At +1248, this field contains the constant value 3810000.
      - id: teardrop_angle_limit
        type: s4
        doc: At +1252, this field contains an optional teardrop angle limit. The value 81,000,000 is 45 degrees in PADS angular units.
      - id: teardrop_length_limit
        type: s4
        doc: At +1256, this field contains an optional teardrop length threshold in BASIC units.
      - id: thermal_flags
        type: u4
        doc: At +1260, this field contains THERFLAGS.
      - id: drill_oversize
        type: s4
        doc: At +1264, this field contains DRLOVERSIZE.
      - id: dot_grid
        type: s4
        doc: At +1268, this field contains DOTGRID.
      - id: grid_default
        type: s4
        doc: 1272.
      - id: user_grid
        type: s4
        doc: At +1276, this field contains USERGRID.
      - id: plane_radius
        type: s4
        doc: At +1280, this field contains PLANERAD x1000.
      - id: plane_flags_packed
        type: u4
        doc: At +1284, this field contains PLANEFLAGS packed bitfield.
      - id: component_height
        type: s4
        doc: At +1288, this field contains COMPHEIGHT.
      - id: keepout_hatch_grid
        type: s4
        doc: At +1292, this field contains KPTHATCHGRID.
      - id: bottom_component_height
        type: s4
        doc: At +1296, this field contains BOTCMPHEIGHT.
      - id: fanout_grid_x
        type: s4
        doc: At +1300, this field contains FANOUTGRID X.
      - id: fanout_grid_y
        type: s4
        doc: At +1304, this field contains FANOUTGRID Y.
      - id: fanout_length
        type: s4
        doc: At +1308, this field contains FANOUTLENGTH.
      - id: router_flags
        type: u4
        doc: At +1312, this field contains ROUTERFLAGS.
      - id: verify_flags
        type: u4
        doc: At +1316, this field contains VERIFYFLAGS.
      - id: fabrication_check_flags
        type: u4
        doc: At +1320, this field contains FABCHKFLAGS.
      - id: at_max_size
        type: s4
        doc: At +1324, this field contains ATMAXSIZE.
      - id: at_max_angle
        type: s4
        doc: At +1328, this field contains ATMAXANGLE.
      - id: sl_min_copper
        type: s4
        doc: At +1332, this field contains SLMINCOPPER.
      - id: sl_min_mask
        type: s4
        doc: At +1336, this field contains SLMINMASK.
      - id: st_min_clear
        type: s4
        doc: At +1340, this field contains STMINCLEAR.
      - id: st_min_spokes
        type: s4
        doc: At +1344, this field contains STMINSPOKES.
      - id: tp_min_width
        type: s4
        doc: At +1348, this field contains TPMINWIDTH.
      - id: tp_min_size
        type: s4
        doc: At +1352, this field contains TPMINSIZE.
      - id: ss_min_gap
        type: s4
        doc: At +1356, this field contains SSMINGAP.
      - id: sb_min_gap
        type: s4
        doc: At +1360, this field contains SBMINGAP.
      - id: sb_layer
        type: s4
        doc: At +1364, this field contains SBLAYER.
      - id: arptom
        type: s4
        doc: At +1368, this field contains ARPTOM.
      - id: arptom_layer
        type: s4
        doc: At +1372, this field contains ARPTOMLAYER.
      - id: ardtom
        type: s4
        doc: At +1376, this field contains ARDTOM.
        if: _root.version != 0x2019
      - id: ardtom_layer
        type: s4
        doc: At +1380, this field contains ARDTOMLAYER.
        if: _root.version != 0x2019
      - id: ardtop
        type: s4
        doc: At +1384, this field contains ARDTOP.
        if: _root.version != 0x2019
      - id: ardtop_layer
        type: s4
        doc: At +1388, this field contains ARDTOPLAYER.
        if: _root.version != 0x2019
      - id: viap_spacing
        type: s4
        doc: At +1392, this field contains VIAPSPACING.
        if: _root.version >= 0x2021
      - id: viap_shape
        type: s4
        doc: At +1396, this field contains VIAPSHAPE.
        if: _root.version >= 0x2021
      - id: viap_to_trace
        type: s4
        doc: At +1400, this field contains VIAPTOTRACE.
        if: _root.version >= 0x2021
      - id: viap_fill
        type: s4
        doc: At +1404, this field contains VIAPFILL.
        if: _root.version >= 0x2021
      - id: viap_word_a
        type: u4
        doc: At +1408, this field contains via-pattern packed word A, which is the VIAPSHSIG name handle.
        if: _root.version >= 0x2021
      - id: viap_word_b
        type: u4
        doc: At +1412, this field contains via-pattern packed word B. Its high byte is the constant value 0x0E.
        if: _root.version >= 0x2021
      - id: viap_flag
        type: s4
        doc: At +1416, this field contains VIAPFLAG.
        if: _root.version >= 0x2021
      - id: flow_flags
        type: s4
        doc: At +1420, this field contains FLOWFLAGS.
        if: _root.version >= 0x2022
      - id: auxiliary_name_buffers
        type: fixed_path_storage
        repeat: expr
        repeat-expr: 4
        if: _root.version >= 0x2025
        doc: |
          Bytes +1424 through +2463 contain the first four fixed auxiliary or CAM selector buffers.
      - id: last_auxiliary_name_buffer
        type: str
        size-eos: true
        encoding: ASCII
        terminator: 0
        if: _root.version >= 0x2025
        doc: |
          This is the truncated fifth auxiliary or CAM selector buffer. It has
          232 bytes in v0x2025 and 236 bytes in v0x2026 and v0x2027.
          Spare capacity is retained storage, not padding.

  fixed_path_storage:
    doc: Fixed 260-byte filename, selector, or command buffer with retained capacity.
    seq:
      - id: value
        type: str
        size: 260
        encoding: ASCII
        terminator: 0

  # =========================================================================
  # SECTION 4 — padstack definitions
  # =========================================================================
  # 64 B/record, one per distinct padstack definition. Marker 0xFE @56 = valid.
  # Shape values are 0 OF, 1 RF, 2 R, and 3 S.
  section_4_padstack:
    seq:
      - id: flags
        type: s4
        doc: At +0, this field contains 0, 2, or 8. The value 2 identifies the first padstack of a decal group.
      - id: finger_offset_x
        type: s4
        doc: At +4, this field contains the finger X offset from FINOFFSET.
      - id: finger_length_or_y_offset
        type: s4
        doc: At +8, this field contains the finger length or Y offset.
      - id: finger_orientation_raw
        type: s4
        doc: At +12, this field contains the finger angle in degrees multiplied by 1,800,000. The values are zero or 162,000,000 for 90 degrees.
      - id: padstack_state
        type: s4
        doc: At +16, this field contains the padstack controller state. It is usually zero.
      - id: object_id
        type: u4
        doc: At +20, this field contains an object identifier or handle.
      - id: library_and_plating_flags
        type: u4
        doc: At +24, bit 31 contains the library or plated flag.
      - id: pad_width
        type: s4
        doc: At +28, this field contains pad width A in BASIC units.
      - id: drill
        type: s4
        doc: At +32, this field contains the drill or inner diameter. Zero identifies a surface-mount pad.
      - id: finger_length
        type: s4
        doc: At +36, this field contains finger length B for finger pads. It is zero for other pads.
      - id: finger_offset_2
        type: s4
        doc: At +40, this field contains the second finger-offset component.
      - id: corner_radius
        type: s4
        doc: At +44, this field contains the corner radius.
      - id: rotation_raw
        type: s4
        doc: At +48, this field contains the rotation angle in degrees multiplied by 1,800,000. The angle is zero or 90 degrees.
      - id: section_5_index
        type: u4
        doc: In v0x2027, this field contains the first section 5 geometry row when `num_layers` is greater than zero. Otherwise, it contains a retained cursor or 0xFFFFFFFF sentinel.
      - id: marker
        type: u1
        doc: At +56, the value 0xFE identifies a valid padstack record.
      - id: shape_code
        type: u1
        enum: pad_shape
        doc: At +57, this field contains the shape.
      - id: num_layers
        type: u1
        doc: At +58, this field contains the number of section 5 layer entries that this padstack owns.
      - id: drill_start_layer
        type: u1
        doc: At +59, this field contains an optional blind-drill or buried-drill start layer. Zero means a through drill.
      - id: drill_end_layer
        type: u1
        doc: At +60, this field contains an optional blind-drill or buried-drill end layer. Zero means a through drill.
      - id: trailing_state
        size: 3

  section_4_padstack_v2022:
    seq:
      - id: object_state
        size: 20
      - id: pad_width
        type: s4
      - id: drill
        type: s4
      - id: finger_length
        type: s4
      - id: corner_radius
        type: s4
      - id: finger_state
        type: s4
      - id: rotation_raw
        type: s4
      - id: section_5_index
        type: u4
        doc: This is the circular section 5 controller cursor. Geometry row j is (section_5_index - 2 + j) modulo the row count.
      - id: marker
        type: u1
      - id: shape_code
        type: u1
        enum: pad_shape
      - id: num_layers
        type: u1
      - id: drill_start_layer
        type: u1
      - id: drill_end_layer
        type: u1
      - id: trailing_state
        size: 3

  section_4_padstack_legacy:
    seq:
      - id: object_state
        size: 24
      - id: pad_width
        type: s4
      - id: drill
        type: s4
      - id: finger_length
        type: s4
      - id: corner_radius
        type: s4
      - id: rotation_raw
        type: s4
      - id: section_5_index
        type: u4
        doc: Cumulative start row into section 5 when num_layers > 0. Retained cursor or empty-list sentinel otherwise.
      - id: marker
        type: u1
      - id: shape_code
        type: u1
        enum: pad_shape
      - id: num_layers
        type: u1
      - id: trailing_state
        type: u1

  section_4_padstack_array:
    seq:
      - id: records
        type:
          switch-on: '_root.directory_entries[4].stored_extent / _root.directory_entries[4].num_items'
          cases:
            52: section_4_padstack_legacy
            56: section_4_padstack_v2022
            64: section_4_padstack
        repeat: eos

  saved_pad_layer_controller_header:
    doc: |
      Saved section 5 controller header. Its serialized size is 20 bytes through
      v0x2021, 64 bytes in v0x2022, and 24 bytes thereafter. It occupies the
      gap between the rotated section 4 padstack grid and the first
      section 5 layer row. These words are controller state, not padding.
    seq:
      - id: legacy_zero_controller_state
        type: u4
        valid: 0
        if: _root.version <= 0x2021
      - id: legacy_saved_controller_flags
        type: u4
        if: _root.version <= 0x2021
        doc: The values are zero and 2.
      - id: legacy_zero_controller_tail
        type: u4
        repeat: expr
        repeat-expr: 3
        valid: 0
        if: _root.version <= 0x2021
      - id: saved_controller_flags
        type: u4
        doc: Zero or 2. 2 marks retained initialized controller state.
        if: _root.version >= 0x2022
      - id: zero_controller_state
        type: u4
        repeat: expr
        repeat-expr: 4
        valid: 0
        if: _root.version >= 0x2022
      - id: v2022_saved_allocator_handle
        type: u4
        if: _root.version == 0x2022
        doc: We believe that this is a saved allocator handle.
      - id: v2022_zero_allocator_state_0
        type: u4
        repeat: expr
        repeat-expr: 4
        valid: 0
        if: _root.version == 0x2022
      - id: v2022_saved_capacity
        type: u4
        if: _root.version == 0x2022
        doc: We believe that this is saved allocator capacity.
      - id: v2022_saved_list_handle
        type: u4
        if: _root.version == 0x2022
        doc: We believe that this is a saved list handle.
      - id: v2022_zero_allocator_state_1
        type: u4
        repeat: expr
        repeat-expr: 4
        valid: 0
        if: _root.version == 0x2022
      - id: modern_saved_controller_state
        type: u4
        if: _root.version >= 0x2024
        doc: We believe that this is a saved section 5 controller handle or counter.

  # =========================================================================
  # SECTION 5 — per-padstack pad-shape layer table
  # =========================================================================
  # Flat array of 20-B or 24-B rows. Version v0x2022 geometry row j is at
  # (section_5_index - 2 + j) modulo the row count. In v0x2027, section_5_index is the first
  # geometry row. Both store selector/shape in each geometry row's serialized
  # successor, including a trailing carrier after the declared rows.
  # Rows before the first geometry row are a global default prefix.
  section_5_pad_layer_20_bytes:
    doc: |
      This is a legacy 20-byte pad-layer row. The selector and shape are the
      first two bytes. Dimensions start at +4. The value at +16 is an angle in
      degrees times 1,800,000.
    seq:
      - id: layer_selector
        type: u1
        doc: At +0, this field contains the PADS layer identifier. Zero selects the top or default layer. The value 255 selects the bottom or all layers. Values 21, 23, 27, and 28 select documentation layers.
      - id: shape_code
        type: u1
        doc: At +1, this field contains the pad-shape code. Values 0, 1, 2, and 3 select OF, RF, round, and square shapes.
      - id: layer_state
        type: u2
        doc: At +2, this field contains the saved layer-row flags and controller state.
      - id: size_a
        type: s4
        doc: 'At +4, this field contains pad dimension A (width or diameter) in BASIC units.'
      - id: size_b
        type: s4
        doc: At +8, this field contains pad dimension B. RT and ST rows use it as the thermal outer diameter.
      - id: shape_parameter_or_offset
        type: s4
        doc: At +12, this field contains the shape-dependent offset or radius. Bit 0x04000000 marks the default or thermal form.
      - id: angle
        type: s4
        doc: At +16, this field contains the shape angle in degrees multiplied by 1,800,000.

  section_5_pad_layer_24_bytes:
    doc: |
      This is a modern 24-byte pad-layer row. In v0x2022 and v0x2027, the
      next row supplies the selector and shape for this row's geometry.
      A carrier can occur after the declared row array.
    seq:
      - id: layer_selector
        type: u1
        doc: At +0, this field contains the PADS layer identifier. Zero selects the top or default layer. The value 255 selects the bottom or all layers. Other values select numbered documentation or copper layers.
      - id: shape_code
        type: u1
        doc: At +1, this field contains the pad-shape code. Values 0, 1, 2, and 3 select OF, RF, round, and square shapes.
      - id: layer_state
        type: u2
        doc: At +2, this field contains the saved layer-row flags and controller state.
      - id: size_a
        type: s4
        doc: 'At +4, this field contains pad dimension A (width or diameter) in BASIC units.'
      - id: size_b
        type: s4
        doc: At +8, this field contains pad dimension B. RT and ST rows use it as the thermal outer diameter.
      - id: shape_parameter_or_offset
        type: s4
        doc: At +12, this field contains the shape-dependent offset or radius. Bit 0x04000000 marks the default or thermal form.
      - id: corner_radius_or_aux_dimension
        type: s4
        doc: 'At +16, this field contains corner radius or shape-specific auxiliary dimension in BASIC units.'
      - id: angle
        type: s4
        doc: At +20, this field contains the shape angle in degrees multiplied by 1,800,000.

  section_5_pad_layer_array:
    seq:
      - id: legacy_records
        type: section_5_pad_layer_20_bytes
        repeat: eos
        if: _root.directory_entries[5].stored_extent / _root.directory_entries[5].num_items == 20
      - id: modern_records
        type: section_5_pad_layer_24_bytes
        repeat: eos
        if: _root.directory_entries[5].stored_extent / _root.directory_entries[5].num_items == 24

  # =========================================================================
  # SECTION 8 — TEXT / label table
  # =========================================================================
  # Circular fixed array: 64 B in v0x2017, 72 B thereafter. Physical section 8
  # starts 28/36 bytes into the logical record ring. Metadata lags geometry by
  # one slot: record K+1 owns record K's geometry. Section 9 is the indexed
  # packed C-string allocation. String_offset is relative to its physical start.
  # Footprint field presentation uses the same controller without a pool string.
  # Modern placement +96 names its first section 8 geometry record. Its coordinates
  # are local to the footprint. Metadata record K+1 association word +12 uses high
  # byte 0x08 and a low-24-bit section 8 ordinal. It links geometry K to the next
  # field geometry. High byte 0x16 stops a placement list (low 24
  # bits = placement ordinal). 0x0E stops at a decal field list (low 24 bits =
  # decal ordinal). A chain reached from placement +96 must stop with 0x16.
  # Encountering a decal terminator there is a broken ownership link. In metadata
  # +24, byte 2 identifies the standard field: low
  # five bits 2 and 3 mean Ref.Des. and Part Type or value. Bit 5 is visibility.
  # Presentation bits 24 and 28 both set mean centered horizontally and
  # vertically. Otherwise horizontal is left and bit 29 selects UP rather than
  # DOWN vertical justification. Free board text has
  # association word zero and uses the lagged metadata/string relationship above.
  section_8_text_header:
    seq:
      - id: leading_object_id
        type: s4
        doc: We believe that +0 contains an object identifier.
      - id: record_marker_or_retained_link
        type: u4
        doc: At +4, this field contains the live-record marker 0x0000FFFE. Free and capacity slots retain a process-local object link.
      - id: string_offset
        type: s4
        doc: At +8, this field contains the string-pool byte offset for text K-1.
      - id: association_flags
        type: u4
        doc: At +12, this field contains the field-list association. It is zero for free board text.
      - id: text_state_0
        type: s4
        doc: At +16, this field contains text-controller state. It is usually zero.
      - id: packed_len_string
        type: u4
        doc: At +20, this field contains the string length in its high 16 bits.
      - id: packed_layer_and_presentation
        type: u4
        doc: At +24, this field contains the layer, justification, mirror, and presentation state.
      - id: tag
        type: u4
        doc: At +28, this field contains free-text tag 0x48FE0000 or 0x49000000.
      - id: unknown_zero_at_32
        type: s4
        doc: At +32, this field is zero. The purpose of this field is not known.
      - id: height
        type: s4
        doc: At +36, this field contains the text height in BASIC units.
      - id: line_width
        type: s4
        doc: At +40, this field contains the text-line width in BASIC units.
      - id: origin_x
        type: s4
        doc: At +44, this field contains the RAW insertion X coordinate.
      - id: origin_y
        type: s4
        doc: At +48, this field contains the RAW insertion Y coordinate.
      - id: rotation_auxiliary_state
        type: s4
        doc: At +52, this field contains rotation or secondary-offset state.
      - id: text_state_1
        type: s4
        doc: At +56, this field contains footprint-field mirror state. Free-text geometry state is usually zero.
      - id: bounding_box_x
        type: s4
        doc: At +60, this field contains the RAW X coordinate of the first-glyph corner.
      - id: bounding_box_y
        type: s4
        doc: At +64, this field contains the RAW Y coordinate of the first-glyph corner.
      - id: trailing_object_id
        type: s4
        doc: We believe that +68 contains an object identifier.

  section_8_text_header_legacy:
    seq:
      - id: leading_object_id
        type: s4
      - id: record_marker_or_retained_link
        type: u4
        doc: At +4, this field contains the live-record marker 0x0000FFFE. Free and capacity slots retain a process-local object link.
      - id: string_offset
        type: u4
        doc: At +8, this field contains the byte offset into physical section 9. It belongs to geometry K-1.
      - id: association_flags
        type: u4
      - id: text_state_0
        type: s4
      - id: packed_len_string
        type: u4
      - id: packed_layer_and_presentation
        type: u4
        doc: At +24, the low byte contains the layer. Value 0x0020 in the high 16 bits marks free board text.
      - id: height
        type: s4
      - id: line_width
        type: s4
      - id: origin_x
        type: s4
      - id: origin_y
        type: s4
      - id: rotation_auxiliary_state
        type: s4
      - id: text_state_1
        type: s4
      - id: bounding_box_x
        type: s4
      - id: bounding_box_y
        type: s4
      - id: trailing_object_id
        type: s4

  section_8_text_metadata_modern:
    seq:
      - id: metadata_bytes
        size: 36
        doc: These are the modern record K+1 metadata fields at +0 through +35 for geometry K.

  section_8_text_metadata_legacy:
    seq:
      - id: metadata_bytes
        size: 28
        doc: V0x2017 record K+1 metadata fields +0..+27 for geometry K.

  section_8_text_ring:
    seq:
      - id: modern_records
        type: section_8_text_header
        repeat: expr
        repeat-expr: _root.directory_entries[8].num_items
        if: _root.version != 0x2017
      - id: modern_last_metadata
        type: section_8_text_metadata_modern
        if: _root.version != 0x2017
      - id: legacy_records
        type: section_8_text_header_legacy
        repeat: expr
        repeat-expr: _root.directory_entries[8].num_items
        if: _root.version == 0x2017
      - id: legacy_last_metadata
        type: section_8_text_metadata_legacy
        if: _root.version == 0x2017

  # =========================================================================
  # SECTION 9 — text string-pool allocation
  # =========================================================================
  section_9:
    doc: |
      This is the section 9 allocation. Live strings are packed NUL-terminated ASCII.
      Section 8 string_offset fields address bytes relative to this region's
      physical start. Remaining allocator capacity is retained zero storage.
      Section 10 begins only after this full declared byte extent. No DRW
      record bytes belong to section 9.
    seq:
      - id: allocated_string_bytes
        size-eos: true
        doc: These are packed string bytes and zero-filled unused allocator capacity.

  section_10_drawing_record:
    doc: |
      This 112-byte LINES drawing-object record can own a board outline,
      dimension, milling path, fiducial, filled copper shape, or keepout. Most
      records contain the marker FE FF 00 00 FF FF FF FF at +0. The first
      unmodified piece coordinate is at +88.

      A filled COPPER or COPCLS owner has a name that starts with DRW. In
      v0x2025 and v0x2027, the class tag at +84 is 0x00004900. The flag at +24
      is 1, and the flag at +28 is not 3. In v0x2026, the class tag is
      0x00004D00. The flag at +24 is 7, and the flag at +28 is zero. The
      unmodified bounding-box coordinates are at +96, +100, +104, and +108.
      Subtract the origin at +88 and +92 to get the section 12 local bounding
      box.

      A KEEPOUT or KPTCLS owner has a name that starts with DRW. Its class tag
      at +84 is zero. The flag at +24 is 1. The flag at +28 is 1 or 10.
    seq:
      - id: marker
        type: u2
        doc: 'At +0, this field contains 0xFFFE.'
      - id: saved_state
        type: u2
        doc: At +2, this field contains saved object state. The values include zero and 31.
      - id: sentinel
        type: s4
        doc: 'At +4, this field contains 0xFFFFFFFF (-1).'
      - id: previous_piece_index
        type: s4
        doc: At +8, this field contains the first section 11 piece of the previous circular owner record.
      - id: previous_vertex_index
        type: s4
        doc: At +12, this field contains the first section 12 vertex of the previous circular owner record.
      - id: previous_arc_parameter_index
        type: s4
        doc: At +16, this field contains the first section 13 arc parameter of the previous circular owner record.
      - id: unknown_zero_at_20
        type: s4
        valid: 0
        doc: At +20, this field is zero. The purpose of this field is not known.
      - id: num_previous_pieces
        type: s4
        doc: At +24, this field contains the section 11 piece count of the previous circular owner record.
      - id: previous_owner_type_and_flags
        type: u4
        doc: At +28, this field contains the packed previous-owner type. The low 16 bits contain the item type. The high 16 bits contain flags. Item types are 0 for LINES, 1 for BOARD, 3 for COPPER, and 10 for KEEPOUT.
      - id: retained_owner_state
        type: u4
        doc: At +32, this field contains saved owner state. A nonzero value can be a live handle.
      - id: heap_handle
        type: u4
        doc: At +36, this field contains a heap-object handle. The value increases by 0x41 for each record.
      - id: section_10_tag
        type: u4
        doc: 'At +40, this field contains 0x80000000.'
      - id: handle_str
        size: 40
        doc: At +44, this field contains an inline DRW name for live objects. Unused capacity retains arbitrary bytes.
      - id: block_tag
        type: u4
        doc: At +84, this field contains the class tag. Filled-copper owners use 0x00004900 or 0x00004D00. Keepout and board-outline owners use zero.
      - id: verts
        type: s4
        repeat: expr
        repeat-expr: 6
        doc: At +88, this field contains three unmodified X and Y coordinate pairs. The first pair is the LINES insertion point.

  section_10_drawing_record_head:
    seq:
      - id: marker
        type: u2
      - id: saved_state
        type: u2
      - id: previous_link
        type: s4
      - id: previous_piece_start
        type: s4
      - id: previous_vertex_start
        type: s4
      - id: previous_arc_start
        type: s4
      - id: retained_zero
        type: s4
        valid: 0
      - id: num_previous_pieces_or_class
        type: s4
      - id: subtype
        type: u4
      - id: retained_state
        type: u4
        repeat: expr
        repeat-expr: 3

  section_10_drawing_record_tail:
    seq:
      - id: name_storage
        size: 40
      - id: block_tag
        type: u4
      - id: origin_x_raw
        type: s4
      - id: origin_y_raw
        type: s4
      - id: bbox_min_x_raw
        type: s4
      - id: bbox_min_y_raw
        type: s4
      - id: bbox_max_x_raw
        type: s4
      - id: bbox_max_y_raw
        type: s4

  section_10_legacy_record:
    seq:
      - id: head
        type: section_10_legacy_record_head
      - id: tail
        type: section_10_legacy_record_tail

  section_10_legacy_record_head:
    seq:
      - id: marker
        type: u2
      - id: saved_state
        type: u2
      - id: previous_link
        type: s4
      - id: previous_piece_start
        type: s4
      - id: previous_vertex_start
        type: s4
      - id: previous_arc_start
        type: s4
      - id: retained_owner_state
        type: s4
      - id: num_previous_pieces_or_class
        type: s4
      - id: subtype
        type: u4
        doc: At +28, this field contains the packed previous-owner drawing subtype and flags.

  section_10_legacy_record_tail:
    seq:
      - id: name_storage
        size: 40
      - id: block_tag
        type: u4
      - id: origin_and_bbox_state
        type: s4
        repeat: expr
        repeat-expr: 6

  section_10_drawing_physical:
    doc: |
      This is a circular fixed array. The logical records rotate left by 68
      bytes. Physical storage contains the last tail, the other records, and
      the last head. No byte belongs to section
      9 or 11. Record R[(i+1) mod count] carries R[i]'s piece/vertex/arc cursors.
    seq:
      - id: modern_last_record_tail
        type: section_10_drawing_record_tail
        if: _root.version >= 0x2024
      - id: modern_records
        type: section_10_drawing_record
        repeat: expr
        repeat-expr: _root.directory_entries[10].num_items - 1
        if: _root.version >= 0x2024
      - id: modern_last_record_head
        type: section_10_drawing_record_head
        if: _root.version >= 0x2024
      - id: legacy_last_record_tail
        type: section_10_legacy_record_tail
        if: _root.version <= 0x2022
        doc: These are logical bytes 32 through 99 of the last 100-byte legacy owner record.
      - id: legacy_records
        type: section_10_legacy_record
        repeat: expr
        repeat-expr: _root.directory_entries[10].num_items - 1
        if: _root.version <= 0x2022
      - id: legacy_last_record_head
        type: section_10_legacy_record_head
        if: _root.version <= 0x2022
        doc: These are logical bytes 0 through 31 of the last 100-byte legacy owner record.

  # =========================================================================
  # SECTION 11 — graphic-piece header table + inline board-outline vertices
  # =========================================================================
  # 20 B/record HEAD (one per OPEN/CLOSED/CIRCLE piece of *LINES* / *PARTDECAL*),
  # then a TAIL (X,Y,attr) i32 triple stream of closed geometry. Arc runs use
  # attribute values as indexes into a parallel 20-byte bounding-box arc table.
  #
  # Dimensions are not a dedicated section. Each DIM* item is a *LINES* DRW owner
  # whose sub-pieces appear here. For these sub-pieces, the +0 word is a piece-type
  # value. The values identify BASPNT, ARWLN1, ARWLN2, ARWHD1, ARWHD2, EXTLN1,
  # and EXTLN2. The +4 flags
  # word is the per-dimension group flag (matches the ASC piece flags column). The
  # sub-piece vertices occur in section 12 in declaration order. The DIM* owner-run
  # vertex cursor identifies them.
  section_11_piece_header:
    seq:
      - id: sub_flag
        type: u1
        doc: At +0, this field contains the previous-piece shape flag. The value 2 marks the previous LINES piece as CIRCLE.
      - id: byte1_or_next_level
        type: u1
        doc: |
          At +1, this field is a role-dependent carrier byte. For a LINES or COPPER piece, the
          next piece stores this piece's ASCII LEVEL. For a DIM sub-piece,
          it is the high byte of the u16 piece-type value.
      - id: type_or_handle_high
        type: u2
        doc: At +2, this field contains the piece-type or object-handle high bits.
      - id: flags
        type: s4
        doc: 'At +4, this field contains -1 default. 0x800/0x1000 keepout-restriction bits. Small ints = ordinal/parent.'
      - id: piece_state
        type: s4
        doc: At +8, this field contains the saved piece state. The values are 0 through 3.
      - id: width
        type: s4
        doc: At +12, this field contains the pen width in BASIC units.
      - id: corners
        type: s4
        doc: At +16, this field contains the vertex or corner count. One-corner LINES point records are valid but have no drawable segment.

  section_11_piece_head:
    seq:
      - id: sub_flag
        type: u1
      - id: byte1_or_next_level
        type: u1
      - id: type_or_handle_high
        type: u2
      - id: flags
        type: s4

  section_11_piece_head_modern:
    seq:
      - id: head
        type: section_11_piece_head
      - id: retained_piece_state
        type: s4

  section_11_piece_tail:
    seq:
      - id: width
        type: s4
      - id: corners
        type: s4

  section_11_piece_header_legacy:
    seq:
      - id: head
        type: section_11_piece_head
      - id: width
        type: s4
      - id: corners
        type: s4

  section_11_piece_physical:
    doc: |
      Circular fixed array rotated right by its record head: twelve bytes for
      modern 20-byte records, eight for legacy 16-byte records. Physical storage
      is record 0's tail, records 1 through count-1, then record 0's head.
    seq:
      - id: modern_last_record_tail
        type: section_11_piece_tail
        if: _root.version >= 0x2025
      - id: modern_records
        type: section_11_piece_header
        repeat: expr
        repeat-expr: _root.directory_entries[11].num_items - 1
        if: _root.version >= 0x2025
      - id: modern_last_record_head
        type: section_11_piece_head_modern
        if: _root.version >= 0x2025
      - id: legacy_last_record_tail
        type: section_11_piece_tail
        if: _root.version <= 0x2024
        doc: This is the logical record 0's width and corner count.
      - id: legacy_records
        type: section_11_piece_header_legacy
        repeat: expr
        repeat-expr: _root.directory_entries[11].num_items - 1
        if: _root.version <= 0x2024
      - id: legacy_last_record_head
        type: section_11_piece_head
        if: _root.version <= 0x2024

  section_12_graphic_vertex:
    doc: |
      This is a fixed 12-byte graphic vertex. The coordinates are local to the
      section 11 graphic piece. The origin does not apply to these coordinates.
    seq:
      - id: x_design
        type: s4
      - id: y_design
        type: s4
      - id: arc_ordinal
        type: s4
        doc: The value -1 selects a straight contour vertex. A nonnegative value is an owner-local arc ordinal.

  section_12_vertex_array:
    seq:
      - id: records
        type: section_12_graphic_vertex
        repeat: eos

  # =========================================================================
  # SECTION 13 — graphic arc parameters / copper-pour hatch geometry
  # =========================================================================
  # The full directory payload is a 20-byte geometry-parameter array. Graphic
  # owners address arc bounding boxes directly as arc_start + vertex.arc_ordinal.
  # Copper-pour owners address the same-width rows as hatch segments.
  section_13_hatch_segment:
    doc: This is a 20-byte graphic parameter. It contains four coordinates and retained type-specific state.
    seq:
      - id: x1
        type: s4
        doc: This is the arc bounding-box minimum X or hatch-segment X1.
      - id: y1
        type: s4
        doc: This is the arc bounding-box minimum Y or hatch-segment Y1.
      - id: layer_marker
        type: s4
        doc: This is the arc bounding-box maximum X or the hatch layer marker BASE-900*layer.
      - id: x2
        type: s4
        doc: This is the arc bounding-box maximum Y or hatch-segment X2.
      - id: y2
        type: s4
        doc: This is the retained arc state, or hatch-segment y2.

  section_13_hatch_array:
    seq:
      - id: records
        type: section_13_hatch_segment
        repeat: eos

  # =========================================================================
  # SECTION 14 — PARTDECAL terminal-run descriptors
  # =========================================================================
  # Physical descriptors start at the decal name, 44 bytes after the nominal
  # section boundary. Both dialects carry sentinel 0xFFFE at +64. The section 15
  # terminal cursor and count are at +68 and +72. The section 16 cursor and count
  # are at +44 and +88. Modern records are 112 bytes. Legacy records are
  # 100 bytes.
  section_14_terminal_descriptor_array:
    seq:
      - id: modern_records
        type: section_14_terminal_descriptor_112_bytes
        repeat: eos
        if: _root.directory_entries[14].stored_extent / _root.directory_entries[14].num_items == 112
      - id: legacy_records
        type: section_14_terminal_descriptor_100_bytes
        repeat: eos
        if: _root.directory_entries[14].stored_extent / _root.directory_entries[14].num_items == 100

  section_14_terminal_descriptor_112_bytes:
    seq:
      - id: decal_name
        size: 44
        doc: 'At +0, this field contains NUL-padded ASCII decal name for live descriptors. Free slots retain arbitrary bytes.'
      - id: section_16_padstack_cursor
        type: s4
        doc: At +44, this field contains the first record in the section 16 per-terminal padstack map.
      - id: descriptor_state_0
        size: 16
        doc: Bytes +48 through +63 contain saved decal-controller links and state.
      - id: sentinel
        type: u2
        doc: 'At +64, this field contains 0xFFFE for a live descriptor.'
      - id: flag_b
        type: u2
        doc: 'At +66, this field contains saved descriptor flags.'
      - id: section_15_start
        type: s4
        doc: At +68, this field contains the first terminal index in the section 15 terminal pool.
      - id: num_terminals
        type: s4
        doc: At +72, this field contains the number of section 15 terminals that this decal owns.
      - id: descriptor_state_1
        size: 12
        doc: Bytes +76 through +87 contain saved terminal-controller links and state.
      - id: num_section_16_padstacks
        type: s4
        doc: At +88, this field contains the number of section 16 per-terminal padstack records.
      - id: descriptor_state_2
        size: 20
        doc: Bytes +92 through +111 contain saved modern descriptor capacity and state.
    instances:
      section_15_zero_based_start:
        value: section_15_start
        doc: This is the zero-based first owned section 15 terminal slot.

  section_14_terminal_descriptor_100_bytes:
    doc: This is the legacy 100-byte PARTDECAL terminal-run descriptor.
    seq:
      - id: decal_name
        size: 44
        doc: 'At +0, this field contains NUL-padded ASCII decal name for live descriptors. Free slots retain arbitrary bytes.'
      - id: section_16_padstack_cursor
        type: s4
        doc: At +44, this field contains the first record in the section 16 per-terminal padstack map.
      - id: descriptor_state_0
        size: 16
        doc: Bytes +48 through +63 contain saved legacy decal-controller links and state.
      - id: sentinel
        type: u2
        doc: 'At +64, this field contains 0xFFFE for a live descriptor.'
      - id: flag_b
        type: u2
        doc: 'At +66, this field contains saved descriptor flags.'
      - id: section_15_start
        type: s4
        doc: |
          At +68, this field contains the first terminal cursor in the legacy section 15 pool. Positive
          v2017/v2019 values are one-based pool ordinals. Zero selects the
          shared first slot used by built-in via decals.
      - id: num_terminals
        type: s4
        doc: At +72, this field contains the number of section 15 terminals that this decal owns.
      - id: descriptor_state_1
        size: 12
        doc: Bytes +76 through +87 contain saved terminal-controller links and state.
      - id: num_section_16_padstacks
        type: s4
        doc: At +88, this field contains the number of section 16 per-terminal padstack records.
      - id: descriptor_state_2
        size: 8
        doc: Bytes +92 through +99 contain saved legacy descriptor capacity and state.
    instances:
      section_15_zero_based_start:
        value: '_root.version <= 0x2019 and section_15_start > 0 ? section_15_start - 1 : section_15_start'
        doc: Decoded zero-based first owned section 15 terminal slot.

  # =========================================================================
  # SECTION 15 — PARTDECAL terminal and controller storage
  # =========================================================================
  # The directory entry describes fixed storage units: 20 bytes in versions 0x2017
  # and 0x2019, 36 bytes thereafter. Units hold terminal geometry or mixed
  # decal-controller/object-dictionary state. Modern files place terminal units
  # first, but their suffix is variable and is not a fixed 33-unit trailer.
  # Legacy records start after a 60-byte rotated descriptor tail. Modern records
  # start after the 44-byte logical-view displacement.
  decal_terminal_slot_array:
    seq:
      - id: records
        type:
          switch-on: _root.version <= 0x2019
          cases:
            true: decal_terminal_slot_20_bytes
            false: decal_terminal_slot_36_bytes
        repeat: eos

  decal_terminal_slot_36_bytes:
    seq:
      - id: x_or_controller_index
        type: s4
        doc: At +0, this field contains pin X in terminal units and a controller index in suffix units.
      - id: y_or_controller_flags
        type: s4
        doc: At +4, this field contains pin Y in terminal units and controller flags in suffix units.
      - id: name_x_or_object_index
        type: s4
        doc: 'At +8, this field contains NMXLOC in terminal units. Object index in suffix units.'
      - id: name_y_or_object_state
        type: s4
        doc: 'At +12, this field contains NMYLOC in terminal units. Object state in suffix units.'
      - id: padstack_or_object_handle
        type: u4
        doc: At +16, this field contains a padstack pointer in terminal units. It is an object handle in suffix units.
      - id: pin_name_or_controller_cache
        size: 4
        doc: At +20, this field contains retained pin-name bytes in terminal units and a controller cache in suffix units.
      - id: retained_controller_state_0
        type: s4
        doc: At +24, this field contains zero in terminal units and mixed controller state in suffix units.
      - id: retained_controller_state_1
        type: s4
        doc: At +28, this field contains zero in terminal units and mixed controller state in suffix units.
      - id: retained_controller_state_2
        type: s4
        doc: At +32, this field contains zero in terminal units and mixed controller state in suffix units.

  decal_terminal_slot_20_bytes:
    seq:
      - id: padstack_or_object_handle
        type: u4
        doc: At +0, this field contains a padstack pointer in terminal units. It is an object handle in suffix units.
      - id: x_or_controller_word0
        type: s4
        doc: At +4, this field contains pin X in terminal units and a controller word in suffix units.
      - id: y_or_controller_word1
        type: s4
        doc: At +8, this field contains pin Y in terminal units and a controller word in suffix units.
      - id: name_x_or_object_index
        type: s4
        doc: 'At +12, this field contains NMXLOC in terminal units. Object index in suffix units.'
      - id: name_y_or_object_state
        type: s4
        doc: 'At +16, this field contains NMYLOC in terminal units. Object state in suffix units.'

  saved_terminal_controller_prefix:
    doc: |
      Four saved section 15 controller words serialized before the v2017/v2019
      terminal-slot ring. These are retained list/object state, not padding.
    seq:
      - id: zero_saved_controller_words
        type: u4
        repeat: expr
        repeat-expr: 4
        valid: 0

  # =========================================================================
  # SECTION 16 — per-terminal padstack mapping ring
  # =========================================================================
  # The declared payload is count x 8 bytes at the physical controller boundary.
  # It maps each decal terminal ordinal to a section 4 padstack ordinal.
  decal_padstack_pair_array:
    seq:
      - id: records
        type: decal_padstack_pair
        repeat: eos

  decal_padstack_pair:
    seq:
      - id: terminal_ordinal
        type: s4
        doc: Zero selects the decal-wide default. Positive values select that one-based terminal ordinal.
      - id: padstack_ordinal
        type: u4
        doc: This is the zero-based ordinal into the section 4 padstack array.

  # =========================================================================
  # SECTION 17 — PARTTYPE definitions
  # =========================================================================
  # Every directory record is a PARTTYPE definition in declaration order. Name
  # storage is at +44 in both dialects. Modern records are 224 bytes. V2017–22
  # use a 208-byte prefix. The second 112 bytes are retained object capacity,
  # can contain 0xFF bytes or live duplicated indexes and state.
  part_type_record_array:
    seq:
      - id: modern_records
        type: part_type_record_224_bytes
        repeat: eos
        if: _root.directory_entries[17].stored_extent / _root.directory_entries[17].num_items == 224
      - id: legacy_records
        type: part_type_record_208_bytes
        repeat: eos
        if: _root.directory_entries[17].stored_extent / _root.directory_entries[17].num_items == 208

  part_type_record_224_bytes:
    seq:
      - id: part_type_controller_state
        size: 44
        doc: Bytes +0 through +43 contain terminal cursors, object ordinals, handles, and serialization state.
      - id: name_storage
        size: 36
        doc: 'At +44, this field contains PARTTYPE NAME in live records. Retained capacity can contain arbitrary bytes.'
      - id: part_type_link_state
        size: 16
        doc: Bytes +80 through +95 contain heap links and duplicated definition indexes.
      - id: decal_selection_slots
        type: part_type_decal_selection_slot
        repeat: expr
        repeat-expr: 16
        doc: |
          Bytes +96 through +223 contain the fixed-capacity alternative-decal vector.
          A live slot has the same nonnegative section 14 ordinal in both words.
          The first different or negative slot ends the live list.

  part_type_record_208_bytes:
    seq:
      - id: part_type_controller_state
        size: 44
      - id: name_storage
        size: 36
        doc: 'At +44, this field contains legacy PARTTYPE NAME in live records. Retained capacity can contain arbitrary bytes.'
      - id: part_type_link_state
        size: 32
        doc: Bytes +80 through +111 contain legacy definition links and controller state.
      - id: primary_decal_ordinal
        type: s4
        doc: 'At +112, this field contains zero-based section 14 decal ordinal. Negative means no decal selection.'
      - id: retained_object_capacity
        size: 92
        doc: Bytes +116 through +207 contain retained legacy PARTTYPE object capacity. These bytes are not file padding.

  part_type_decal_selection_slot:
    seq:
      - id: decal_ordinal
        type: s4
      - id: decal_ordinal_duplicate
        type: s4

  # =========================================================================
  # SECTION 18 — last PARTTYPE metadata and gate-definition stream
  # =========================================================================
  # The nominal section boundary is at the start of the last rotated PARTTYPE
  # metadata. A 44-byte prefix occurs before the 8-byte gate records.
  part_type_last_metadata:
    seq:
      - id: object_ordinal
        type: s4
        doc: Ordinal of the last PARTTYPE.
      - id: pin_cursor
        type: s4
        doc: This is the first section 19 pin ordinal for the last PARTTYPE. The value is -1 when it has no pins.
      - id: prior_slot_state_0
        size: 8
      - id: flags
        type: u1
        doc: ASCII PARTTYPE FLAGS for the last definition.
      - id: part_type
        size: 3
        doc: |
          Three-byte PARTTYPE type code on modern files. Legacy v0x2017/v0x2019
          place the code later in this rotated record, so retain these bytes raw.
      - id: gates
        type: s4
        doc: ASCII PARTTYPE GATES for the last definition.
      - id: prior_slot_state_1
        size: 12
      - id: object_handle
        type: u4
      - id: state_flag
        type: u4

  part_type_gate_array:
    seq:
      - id: records
        type: part_type_gate_record
        repeat: eos

  part_type_gate_record:
    seq:
      - id: num_pins
        type: u4
        doc: This is the pin count from the ASCII `G` declaration.
      - id: swap_type_flags
        type: u4
        doc: This is the gate-swap type. The values include 2, 3, and 0xFFFFFF00.

  # =========================================================================
  # SECTION 19 — phase-shifted PARTTYPE pin-definition table
  # =========================================================================
  # Records begin at nominal section 19 +44, immediately after the section 18 gate
  # stream. Record strides are 24 (v2017), 60 (v2019), 69 (v2021/22), and 88
  # (v2024+).
  part_type_pin_array:
    seq:
      - id: v88_records
        type: part_type_pin_88_bytes
        repeat: eos
        if: _root.directory_entries[19].stored_extent / _root.directory_entries[19].num_items == 88
      - id: v69_records
        type: part_type_pin_69_bytes
        repeat: eos
        if: _root.directory_entries[19].stored_extent / _root.directory_entries[19].num_items == 69
      - id: v60_records
        type: part_type_pin_60_bytes
        repeat: eos
        if: _root.directory_entries[19].stored_extent / _root.directory_entries[19].num_items == 60
      - id: v24_records
        type: part_type_pin_24_bytes
        repeat: eos
        if: _root.directory_entries[19].stored_extent / _root.directory_entries[19].num_items == 24

  part_type_pin_88_bytes:
    seq:
      - id: pin_state
        type: u1
        doc: Pin type/name state. Commonly 0 or 1.
      - id: pin_id
        type: strz
        encoding: ASCII
        size: 17
        doc: SWAPTYPE letter plus pin number, such as U1, L2, P8, G12, or A1.
      - id: pin_name_storage
        type: strz
        encoding: ASCII
        size: 70
        doc: This is the active pin or signal name followed by retained fixed-slot capacity. The retained bytes are not file padding.

  part_type_pin_69_bytes:
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
        doc: This is the legacy pin-name buffer and retained slot capacity.

  part_type_pin_60_bytes:
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
        doc: V2019 pin-name buffer and retained slot capacity.

  part_type_pin_24_bytes:
    seq:
      - id: pin_number
        type: u4
      - id: pin_state
        type: u1
      - id: swap_type
        type: u1
        doc: ASCII swap-type character in populated v2017 records.
      - id: pin_name_storage
        type: strz
        encoding: ASCII
        size: 18
        doc: V2017 pin-name buffer and retained slot capacity.

  # =========================================================================
  # SECTIONS 20 AND 21 — PARTTYPE SIGPIN mappings and compact pin names
  # =========================================================================
  # Section 20 serializes ASCII SIGPIN declarations. Section 21 stores compact pin names used
  # by the pin-name continuation lines of PARTTYPE declarations. Both arrays are
  # rotated 44 bytes beyond their nominal directory boundaries. Legacy SIGPIN
  # records have one additional four-byte lead-in. Fixed storage after a NUL can
  # contain retained/live bytes from the overlapping next logical view and is
  # therefore capacity, never padding.
  part_type_signal_pin_array:
    seq:
      - id: modern_records
        type: part_type_signal_pin_72_bytes
        repeat: eos
        if: _root.directory_entries[20].stored_extent / _root.directory_entries[20].num_items == 72
      - id: legacy_records
        type: part_type_signal_pin_56_bytes
        repeat: eos
        if: _root.directory_entries[20].stored_extent / _root.directory_entries[20].num_items == 56

  part_type_signal_pin_72_bytes:
    seq:
      - id: pin_id_storage
        type: strz
        encoding: ASCII
        size: 16
        doc: SIGPIN pin identifier and retained fixed-slot capacity.
      - id: signal_name_storage
        type: strz
        encoding: ASCII
        size: 56
        doc: SIGPIN signal name and retained fixed-slot capacity.

  part_type_signal_pin_56_bytes:
    seq:
      - id: pin_ordinal
        type: u4
        doc: Numeric SIGPIN pin identifier in legacy files.
      - id: signal_name_storage
        type: strz
        encoding: ASCII
        size: 52
        doc: This is the legacy SIGPIN signal name and retained fixed-slot capacity.

  compact_part_type_pin_name_array:
    seq:
      - id: modern_records
        type: compact_part_type_pin_name_16_bytes
        repeat: eos
        if: _root.directory_entries[21].stored_extent / _root.directory_entries[21].num_items == 16
      - id: legacy_records
        type: compact_part_type_pin_name_8_bytes
        repeat: eos
        if: _root.directory_entries[21].stored_extent / _root.directory_entries[21].num_items == 8

  compact_part_type_pin_name_16_bytes:
    seq:
      - id: pin_name_storage
        type: strz
        encoding: ASCII
        size: 16
        doc: This is the compact PARTTYPE pin name and retained fixed-slot capacity.

  compact_part_type_pin_name_8_bytes:
    seq:
      - id: pin_name_storage
        type: strz
        encoding: ASCII
        size: 8
        doc: This is the legacy compact PARTTYPE pin name and retained fixed-slot capacity.

  # =========================================================================
  # SECTION 22 — part placements
  # =========================================================================
  # Each directory record is a live placement.
  # Versions v2021 and v2022 use the 96-byte prefix. v2024 and later add 16 bytes.
  # Association state is one-record lagged. Modern record N+1 selects record N's
  # PARTTYPE and alternate at +4 and +17. Legacy record N+1 selects record N's
  # decal at +24. The last selector continues into the next
  # controller's 44-byte rotated metadata. It does not wrap to record zero.
  # Geometry and reference designators are not lagged.
  part_placement_array:
    seq:
      - id: modern_records
        type: part_placement_112_bytes
        repeat: eos
        if: _root.directory_entries[22].stored_extent / _root.directory_entries[22].num_items == 112
      - id: legacy_records
        type: part_placement_96_bytes
        repeat: eos
        if: _root.directory_entries[22].stored_extent / _root.directory_entries[22].num_items == 96

  part_placement_112_bytes:
    seq:
      - id: marker
        type: s4
        doc: At +0, this field is always zero for a live record.
      - id: decal_index
        type: s4
        doc: At +4, this field contains the part-type index for the previous placement in the record ring in modern dialects.
      - id: secondary_decal_index
        type: s4
        doc: At +8, this field contains the secondary geometry or decal index.
      - id: instance_index
        type: s4
        doc: We believe that +12 contains a per-instance index.
      - id: placement_side_state
        type: s4
        doc: We believe that +16 contains placement-side state or a saved handle.
      - id: object_id
        type: s4
        doc: At +20, this field contains the legacy part-type or controller index. The placement object identifier is the record ordinal.
      - id: previous_decal_index
        type: s4
        doc: At +24, this field contains the legacy decal index for the previous placement in the record ring.
      - id: link_state_0
        type: s4
        doc: 'At +28, this field contains -1 or 0.'
      - id: link_state_1
        type: s4
        doc: 'At +32, this field contains -1 or 0.'
      - id: handle_low
        type: u4
        doc: At +36, this field contains the low word of the per-instance handle.
      - id: handle_high
        type: u4
        doc: At +40, this field contains zero or 0x80000000, which is the tag or high word of the handle.
      - id: reference_designator
        type: strz
        encoding: ASCII
        size: 16
        doc: 'At +44, this field contains REFDES, inline NUL-terminated ASCII.'
      - id: x_raw
        type: s4
        doc: At +60, this field contains placement X, which is design X plus origin X.
      - id: y_raw
        type: s4
        doc: At +64, this field contains placement Y, which is design Y plus origin Y.
      - id: orientation
        type: s4
        doc: At +68, this field contains the orientation in degrees multiplied by 1,800,000.
      - id: mirror
        type: s4
        doc: At +72, bit 0 is 1 when the placement is mirrored to the bottom side.
      - id: bounding_box_x_a
        type: s4
        doc: 'At +76, this field contains bounding-box X (a) RAW.'
      - id: bounding_box_y_a
        type: s4
        doc: 'At +80, this field contains bounding-box Y (a) RAW.'
      - id: bounding_box_x_b
        type: s4
        doc: 'At +84, this field contains bounding-box X (b) RAW.'
      - id: bounding_box_y_b
        type: s4
        doc: 'At +88, this field contains bounding-box Y (b) RAW.'
      - id: marker_fffe
        type: s4
        doc: 'At +92, this field contains 0xFFFE in simple files.'
      - id: field_record_ordinal
        type: s4
        doc: At +96, a nonnegative value is the section 8 ordinal of the first field-presentation record. A negative value means that no presentation is saved.
      - id: instance_serial_handle
        type: u4
        doc: At +100, this field contains a per-instance serial number or handle.
      - id: trailing_object_handle
        type: s4
        doc: At +104, this field is usually zero. Allocated records can contain saved object handles.
      - id: cluster_id
        type: s4
        doc: |
          At +108, this field contains a one-based CLSTID into the cluster table
          after section 64. The value -1 means that the part is not in a
          cluster. Only the new 112-byte layout contains this field. The 96-byte
          layout has no room for it.

  part_placement_96_bytes:
    seq:
      - id: marker
        type: s4
      - id: decal_index
        type: s4
        doc: This is the retained controller state. Legacy decal selection is the next record's +24 word.
      - id: secondary_decal_index
        type: s4
      - id: instance_index
        type: s4
      - id: placement_side_state
        type: s4
      - id: object_id
        type: s4
        doc: This is the legacy part_type/controller index. Placement object ID is the record ordinal.
      - id: previous_decal_index
        type: s4
        doc: Decal index for the previous placement in the record ring.
      - id: link_state_0
        type: s4
      - id: link_state_1
        type: s4
      - id: handle_low
        type: u4
      - id: handle_high
        type: u4
      - id: reference_designator
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
      - id: bounding_box_x_a
        type: s4
      - id: bounding_box_y_a
        type: s4
      - id: bounding_box_x_b
        type: s4
      - id: bounding_box_y_b
        type: s4
      - id: marker_fffe
        type: s4

  # =========================================================================
  # CLUSTER TABLE — part clusters (.asc *CLUSTER* groups)
  # =========================================================================
  # Section 68 has 60 bytes per record and ends 12 bytes before section 69's
  # layer-record array. Records are in
  # .asc *CLUSTER* order. The stored +0 id and the record's 1-based ordinal both
  # equal the CLSTID that the section 22 placement +108 field references. The cluster
  # The NAME also occurs in the section 8 string pool. This table contains the
  # cluster coordinates at +20 and +24.
  cluster_record:
    seq:
      - id: cluster_id
        type: u4
        doc: At +0, this field contains the stored one-based CLSTID. It equals this record ordinal plus one.
      - id: name
        size: 16
        doc: At +4, this field contains the cluster name in live records. Retained and free records can contain arbitrary bytes.
      - id: x_raw
        type: s4
        doc: 'At +20, this field contains XLOC RAW. Design = raw - origin_x (BASIC = 1/38100 mil).'
      - id: y_raw
        type: s4
        doc: 'At +24, this field contains YLOC RAW. Design = raw - origin_y.'
      - id: retained_orientation
        type: s4
        doc: At +28, this field contains the retained cluster orientation in PADS angular units.
      - id: attribute
        type: s4
        doc: At +32, the low 16 bits contain the ASCII ATTRIBUTE value.
      - id: retained_group_state
        type: s4
        doc: At +36, this field contains saved group state. The values are 0 and 1.
      - id: null_relationship_handle
        type: s4
        doc: At +40, this field contains a null relationship handle.
      - id: child_list_handle
        type: u4
        doc: At +44, this field contains the tagged child-list handle. The low word agrees with the ASCII CHILD_NUM value.
      - id: null_previous_handle
        type: s4
        doc: At +48, this field contains a null previous-link handle.
      - id: null_next_handle
        type: s4
        doc: At +52, this field contains a null next-link handle.
      - id: object_handle
        type: u4
        doc: At +56, this field contains the tagged cluster object handle.

  # =========================================================================
  # SECTION 23 — net records
  # =========================================================================
  # Names start at +76. Modern boards can have one unassigned-obstacles sentinel.
  # Versions v2017 through v2022 use 144 bytes. v2024 uses 416 bytes. Later versions use 424 bytes.
  net_record_array:
    seq:
      - id: v424_records
        type: net_record_424_bytes
        repeat: eos
        if: _root.directory_entries[23].stored_extent / _root.directory_entries[23].num_items == 424
      - id: v416_records
        type: net_record_416_bytes
        repeat: eos
        if: _root.directory_entries[23].stored_extent / _root.directory_entries[23].num_items == 416
      - id: v144_records
        type: 'legacy_net_record_ring(_root.directory_entries[23].num_items)'
        if: _root.directory_entries[23].stored_extent / _root.directory_entries[23].num_items == 144

  saved_net_controller_prefix:
    doc: |
      Eleven saved section 23 controller words between the rotated placement grid
      and the v2017..v2022 net-record array. They retain controller/list state
      and are not alignment padding.
    seq:
      - id: saved_controller_words
        type: u4
        repeat: expr
        repeat-expr: 11

  legacy_signal_pin_last_record_tail:
    doc: |
      The v0x2017 56-byte signal-pin array rotates 48 bytes past its nominal
      section 20 boundary. This word is the last four bytes of the last
      signal-name slot. The value is 0x001d1168.
    seq:
      - id: retained_signal_name_capacity_tail
        type: u4

  legacy_net_record_144_bytes:
    seq:
      - id: head
        type: legacy_net_record_head
      - id: tail
        type: legacy_net_record_tail

  legacy_net_record_head:
    seq:
      - id: anchor_part_index
        type: s4
        doc: At +0, this field contains a zero-based index into section 22 of a member part.
      - id: anchor_pin
        type: s4
        doc: At +4, this field contains the terminal or pin number on the anchor part.
      - id: section_24_start
        type: s4
        doc: |
          At +8, this field contains the index of one section 24 edge in this signal component. Components
          are not contiguous in record order. This value does not have to be the
          lowest edge index. The edge component gives the full signal.
      - id: name_slot
        size: 48
        doc: |
          At +12, this field contains ASCII NUL-terminated net name. The section 23 directory count is
          the number of active legacy records.
      - id: net_controller_state_0
        size: 24
        doc: 'At +60..+83, this field contains serialized legacy net-controller state.'
      - id: net_class_owner_handle
        type: u4
        doc: 'At +84, this field contains saved section 66 class handle. Zero when the net has no class.'
      - id: net_class_membership_state
        type: u4
        doc: 'At +88, this field contains retained class-membership state.'
      - id: num_connections
        type: s4
        doc: At +92, this field contains the number of connections. This number equals the number of section 24 entries for this net.
      - id: net_controller_state_1
        size: 28
        doc: 'At +96..+123, this field contains serialized legacy net-controller state.'

  legacy_net_record_tail:
    seq:
      - id: net_controller_state_2
        size: 20
        doc: 'At +124..+143, this field contains serialized legacy net-controller state.'

  legacy_net_record_ring:
    params:
      - id: num_records
        type: u4
    doc: |
      This is a legacy section 23 circular array. Logical record zero starts at
      physical +20. The last record's 20-byte tail is the physical prefix. The
      other records and the last record's 124-byte head follow it.
    seq:
      - id: last_record_tail
        type: legacy_net_record_tail
      - id: full_records
        type: legacy_net_record_144_bytes
        repeat: expr
        repeat-expr: num_records - 1
      - id: last_record_head
        type: legacy_net_record_head

  net_record_144_bytes:
    seq:
      - id: net_controller_state_0
        size: 52
        doc: 'At +0..+51, this field contains serialized net-controller state.'
      - id: plane_index
        type: s4
        doc: 'At +52, this field contains -1 normal signal. >=1 1-based plane-assignment index.'
      - id: signal_flags
        type: s4
        doc: 'At +56, this field contains raw PADS SIGFLAG.'
      - id: num_connections
        type: s4
        doc: |
          At +60, this field is the saved pin-edge count for this connection component. A
          zero-edge autoroute placeholder does not own a pin. We believe that
          its retained anchor identifies an existing signal.
      - id: anchor_part_index
        type: s4
        doc: |
          At +64, this field contains a zero-based index into section 22 of a member part. For zero-edge $$$
          placeholders this is retained alias state, not signal membership.
      - id: anchor_pin
        type: s4
        doc: |
          At +68, this field contains the terminal or pin number on the anchor part. Named connected signals and
          named singleton signals own this pin. Zero-edge $$$ placeholders do not.
      - id: section_24_start
        type: s4
        doc: At +72, this field contains the index of one section 24 edge in this signal component. Components are not contiguous in record order.
      - id: name_slot
        size: 48
        doc: |
          At +76, this field contains ASCII NUL-terminated net name. Names beginning $$$ are PADS
          autoroute aliases/placeholders. A named signal wins when both retain
          the same anchor or section 24 component.
      - id: serialized_index
        type: s4
        doc: 'At +124, this field contains serialized object index/handle.'
      - id: net_controller_state_1
        type: s4
        doc: 'At +128, this field contains net serialization state.'
      - id: len_serialized_data
        type: s4
        doc: 'At +132, this field contains serialized byte-size of connection/route data.'
      - id: len_serialized_capacity
        type: s4
        doc: 'At +136, this field contains allocation capacity.'
      - id: net_controller_state_2
        type: s4
        doc: 'At +140, this field contains net serialization state.'

  net_record_416_bytes:
    seq:
      - id: base
        type: net_record_144_bytes
      - id: net_self_pointer
        type: u4
        doc: >
          At +144, this field contains the net object's own in-file CObject id. Stable within one file.
          This value is the diff-pair member key. The section 49 DIF_PAIR member
          pointers at +12 and +16 have the same value.
      - id: net_class_owner_pointer
        type: u4
        doc: >
          At +148, this field is the NET_CLASS owner-object identifier. Nets in one class have
          the same value. Zero means that the net has no class. The ascending
          values give the net-class declaration order. This is an identifier,
          not a pointer.
      - id: heap_pointer_1
        type: s4
        doc: 'At +152, this field contains serialized heap pointer/state.'
      - id: num_connections_duplicate
        type: s4
        doc: 'At +156, this field contains duplicate of connection count.'
      - id: retained_object_capacity
        size: 256
        doc: 'At +160..+415, this field contains retained serialized-object capacity and controller state. Not file padding.'

  net_record_424_bytes:
    seq:
      - id: base
        type: net_record_144_bytes
      - id: net_self_pointer
        type: u4
      - id: net_class_owner_pointer
        type: u4
      - id: heap_pointer_1
        type: s4
      - id: num_connections_duplicate
        type: s4
      - id: retained_object_capacity
        size: 264
        doc: 'At +160..+423, this field contains retained serialized-object capacity and controller state. Not file padding.'

  # =========================================================================
  # SECTION 24 — route chain / pin-pair connection topology
  # =========================================================================
  # The directory count is the number of live undirected edges between placed-
  # part terminal identities. Modern record zero is the topology root. Edge K
  # gets its placement-object indices from full record K at +60 and +64. It gets
  # its terminal ordinals and topology expression from record K+1 at +0 through +20. The
  # last record K+1 is a 36-byte head at the end of the physical controller.
  # Unioning these edges forms one component per electrical signal. Each modern
  # net record's anchor_part_index/anchor_pin identifies its component directly.
  # Section_24_start is a member edge, not a contiguous block boundary. $$$ records
  # are PADS autoroute aliases and can anchor the same component as its named
  # signal. Records carry a 0xFE high-byte state at +20 and 0x0000FFFE at +52.
  saved_connection_controller_prefix:
    doc: |
      Saved section 24 controller prefix before the circular connection array: four
      retained words through v0x2022 and two words thereafter. The next
      logical connection record begins immediately after this prefix.
    seq:
      - id: legacy_saved_controller_links
        type: u4
        repeat: expr
        repeat-expr: 4
        if: _root.version <= 0x2022
        doc: These are four saved topology or list links. They are not file offsets.
      - id: modern_zero_controller_words
        type: u4
        repeat: expr
        repeat-expr: 2
        valid: 0
        if: _root.version >= 0x2024

  empty_route_chain_controller_state:
    doc: |
      An empty modern connection controller retains a 36-byte terminal head.
      This head can contain allocator state. These words are controller state,
      not padding.
    seq:
      - id: saved_controller_state_0
        type: u4
      - id: saved_free_list_head
        type: s4
      - id: saved_capacity
        type: u4
      - id: saved_controller_state_1
        type: u4
      - id: saved_list_tail
        type: s4
      - id: saved_link_a
        type: s4
      - id: saved_link_b
        type: s4
      - id: saved_allocator_handle
        type: u4
      - id: saved_controller_state_2
        type: u4

  route_chain_array:
    params:
      - id: num_edges
        type: u4
      - id: is_modern
        type: u1
    seq:
      - id: edges
        type:
          switch-on: is_modern
          cases:
            0: legacy_route_chain_record
            1: 'route_chain_record(_index)'
        repeat: expr
        repeat-expr: num_edges
      - id: modern_last_edge_head
        type: 'route_chain_edge_head(num_edges)'
        if: is_modern != 0

  route_chain_edge_head:
    params:
      - id: record_index
        type: u4
    seq:
      - id: ordinal_a
        type: s4
        doc: At +0, this field contains the terminal ordinal paired with endpoint A of the previous full record.
      - id: ordinal_b
        type: s4
        doc: At +4, this field contains the terminal ordinal paired with endpoint B of the previous full record.
      - id: topology_ref_a
        type: s4
        valid:
          expr: '_ >= 0 ? _ <= _root.directory_entries[24].num_items : -_ - 1 < _root.directory_entries[22].num_items'
        doc: |
          At +8, this field contains the first input of the last edge's topology expression. Nonnegative
          values are section 24 node ordinals. Negative value -(N+1) is
          section 22 placement leaf N and equals the previous full record's
          endpoint_object_a when that input is a leaf.
      - id: topology_ref_b
        type: s4
        valid:
          expr: '_ >= 0 ? _ <= _root.directory_entries[24].num_items : -_ - 1 < _root.directory_entries[22].num_items'
        doc: |
          At +12, this field contains the second input of the last edge. It uses the same section 24 node or
          section 22-placement-leaf encoding as +8. For a leaf, it equals the
          previous full record's endpoint_object_b.
      - id: topology_ref_c
        type: s4
        valid:
          expr: '_ >= 0 ? _ <= _root.directory_entries[24].num_items : -_ <= _root.directory_entries[23].num_items'
        doc: |
          At +16, this field contains the output of the last edge. Nonnegative values are section 24 node
          ordinals. Negative value -(N+1) completes named section 23 signal N.
      - id: marker
        type: u4
        valid:
          expr: '(_ & 0xffffffc0) == 0xfe000000'
      - id: route_chain_state_0
        size: 12
        doc: Bytes +24 through +35 contain the topology-controller state of the last edge.

  route_chain_record:
    params:
      - id: record_index
        type: u4
    seq:
      - id: ordinal_a
        type: s4
        doc: At +0, this field contains the terminal ordinal for endpoint A of the previous full record.
      - id: ordinal_b
        type: s4
        doc: At +4, this field contains the terminal ordinal for endpoint B of the previous full record.
      - id: topology_ref_a
        type: s4
        valid:
          expr: '_ >= 0 ? _ <= _root.directory_entries[24].num_items : -_ - 1 < _root.directory_entries[22].num_items'
        doc: |
          At +8, this field contains the first input of the next edge's topology expression.
          Nonnegative values are section 24 node ordinals. Negative value
          -(N+1) is section 22 placement leaf N and equals the previous
          full record's endpoint_object_a when that input is a leaf.
      - id: topology_ref_b
        type: s4
        valid:
          expr: '_ >= 0 ? _ <= _root.directory_entries[24].num_items : -_ - 1 < _root.directory_entries[22].num_items'
        doc: |
          At +12, this field contains the second topology-expression input. It uses the same section 24 node
          or section 22-placement-leaf encoding as +8. For a leaf, it equals the
          previous full record's endpoint_object_b.
      - id: topology_ref_c
        type: s4
        valid:
          expr: '_ >= 0 ? _ <= _root.directory_entries[24].num_items : -_ <= _root.directory_entries[23].num_items'
        doc: |
          At +16, this field contains the topology-expression output. Nonnegative values are section 24
          node ordinals. Negative values are completed signal-component IDs in
          declaration order: -(N+1) closes named section 23 signal N. Values
          beyond the placement count are therefore signal IDs, not an extra
          placement-leaf namespace.
      - id: marker
        type: u4
        valid:
          expr: 'record_index == 0 or (_ & 0xffffffc0) == 0xfe000000'
        doc: |
          At +20, this field contains the topology-root state on modern record zero. Modern later records
          have high byte 0xFE and low route flags. Legacy dialects store a
          traversal ordinal/state value instead.
      - id: route_chain_state_0
        size: 12
        doc: Bytes +24 through +35 contain the route-chain controller state.
      - id: end_x1
        type: s4
        doc: 'At +36, this field contains optional RAW endpoint X (else 0).'
      - id: end_y1
        type: s4
        doc: 'At +40, this field contains optional RAW endpoint Y (else 0).'
      - id: end_x2
        type: s4
        doc: 'At +44, this field contains optional RAW endpoint X (else 0).'
      - id: end_y2
        type: s4
        doc: 'At +48, this field contains optional RAW endpoint Y (else 0).'
      - id: flag_fffe
        type: u4
        valid:
          expr: '(_ & 0xffff) == 0xfffe'
        doc: |
          At +52, this field contains the modern endpoint-state word. Its low half is 0xFFFE. Its high
          half retains controller flags. Legacy dialects use traversal state.
      - id: route_chain_state_2
        type: s4
        doc: At +56, this field contains the route-chain controller state.
      - id: endpoint_object_a
        type: u4
        doc: At +60, this field contains the zero-based section 22 placement-object index for endpoint A. The ordinal is at +0 in the next topology head.
      - id: endpoint_object_b
        type: u4
        doc: At +64, this field contains the zero-based section 22 placement-object index for endpoint B. The ordinal is at +4 in the next topology head.

  legacy_route_chain_record:
    doc: |
      This is a v0x2017 through v0x2022 connection edge. The record rotates the
      modern field sequence by 16 bytes. Thus, the endpoint objects and terminal
      ordinals occur in one record.
    seq:
      - id: flag_fffe
        type: u4
        valid:
          expr: '(_ & 0xffff) == 0xfffe'
        doc: At +0, this field contains the endpoint-state word. The low half is the serialized 0xFFFE flag.
      - id: route_chain_state_2
        type: s4
        doc: At +4, this field contains the route-chain controller state.
      - id: endpoint_object_a
        type: u4
        doc: At +8, this field contains the serialized placement-object identifier for endpoint A.
      - id: endpoint_object_b
        type: u4
        doc: At +12, this field contains the serialized placement-object identifier for endpoint B.
      - id: ordinal_a
        type: u4
        doc: At +16, this field contains the terminal ordinal on endpoint A.
      - id: ordinal_b
        type: u4
        doc: At +20, this field contains the terminal ordinal on endpoint B.
      - id: topology_ref_a
        type: s4
        doc: At +24, this field contains the first saved topology-expression input or link.
      - id: topology_ref_b
        type: s4
        doc: At +28, this field contains the second saved topology-expression input or link.
      - id: topology_ref_c
        type: s4
        doc: At +32, this field contains the saved topology-expression output or link.
      - id: marker
        type: u4
        valid:
          expr: '(_ & 0xffffffc0) == 0xfe000000'
        doc: At +36, this field contains the route-chain marker with low route flags.
      - id: route_chain_state_0
        size: 12
        doc: Bytes +40 through +51 contain the topology-controller state.
      - id: end_x1
        type: s4
        doc: 'At +52, this field contains optional RAW endpoint X, otherwise zero.'
      - id: end_y1
        type: s4
        doc: 'At +56, this field contains optional RAW endpoint Y, otherwise zero.'
      - id: end_x2
        type: s4
        doc: 'At +60, this field contains optional RAW endpoint X, otherwise zero.'
      - id: end_y2
        type: s4
        doc: 'At +64, this field contains optional RAW endpoint Y, otherwise zero.'

  # =========================================================================
  # SECTIONS 25–29 — rotated route-object allocator tables
  # =========================================================================
  # These tables share the same 44-byte rotation as the PARTTYPE arrays. The
  # section 25 controller consumes its declared 280/288 bytes plus the first 44
  # nominal bytes of section 26. Each next logical array begins at its
  # nominal directory boundary +44 and ends +44 into the next non-empty entry.
  # We believe that sections 28 and 30 through 40 have no records. Thus, the section 29
  # handle vector ends 44 bytes into the nominal section 41 region.
  route_allocator_controller:
    doc: |
      This is route-object allocator state. The legacy layout has 324 bytes. The
      modern layout has 332 bytes. These controller words are not padding.
    seq:
      - id: controller_state_prefix
        type: u4
        repeat: expr
        repeat-expr: 45
        doc: This is the retained global route/controller state through byte +179.
      - id: num_allocator_20_pages
        type: u2
        doc: At +180, this field contains the number of section 26 descriptors for the runtime 20-byte allocator.
      - id: num_allocator_48_pages
        type: u2
        doc: At +182, this field contains the number of section 26 descriptors for the runtime 48-byte allocator.
      - id: num_allocator_88_pages
        type: u2
        doc: At +184, this field contains the number of section 26 descriptors for the runtime 88-byte allocator.
      - id: num_allocator_56_pages
        type: u2
        doc: At +186, this field contains the number of section 26 descriptors for the runtime 56-byte section 61 node allocator.
      - id: controller_state_suffix
        type: u4
        repeat: eos
        doc: This is the retained route-controller state next to the page-group counts.

  route_object_range_array:
    seq:
      - id: records
        type: route_object_range
        repeat: eos

  route_object_range:
    doc: |
      This is a 12-byte page descriptor. The page length is `num_records *
      controller_stride`. `allocation_begin` and `allocation_end` are saved
      memory addresses. They are not file offsets.
    seq:
      - id: allocation_begin
        type: u4
        doc: This is the saved process address of the first object in this allocator span.
      - id: allocation_end
        type: u4
        doc: This is the saved process address immediately after the allocator span.
      - id: num_records
        type: u4
        doc: This is the number of serialized fixed-stride records in this page.

  page_descriptor_group:
    params:
      - id: num_descriptors
        type: u4
    seq:
      - id: first
        type: route_object_range
      - id: remaining
        type: 'page_descriptor_group(num_descriptors - 1)'
        if: num_descriptors > 1
    instances:
      num_records:
        value: '(first.num_records + (num_descriptors > 1 ? remaining.num_records : 0)).as<u4>'
        doc: This is the serialized record count summed from this controller's pages.

  route_layer_object_count_array:
    doc: |
      This array contains one u32 route-object count for each copper layer. The
      number of values is the number of copper layers. Their sum is
      `directory_entries[29].num_items`.
    seq:
      - id: num_objects_by_layer
        type: u4
        repeat: eos

  route_object_handle_array:
    doc: |
      Flat route-object handle vector. Handles are saved process addresses,
      commonly 8-byte aligned on the modern allocator and spaced on a 56-byte
      object grid. Partition the vector into copper-layer groups using the
      section 27 counts. Section 25's fourth page-group count identifies the
      section 26 descriptor group for the 56-byte section 61 node allocator.
      A handle's page and 56-byte ordinal identify its section 61 record. A
      search is not necessary.
    seq:
      - id: object_handles
        type: u4
        repeat: eos
        doc: This is the saved route-object process address. Never a file offset.

  # =========================================================================
  # PAGED CONTROLLERS 41, 42, 45, 46, 47, 48
  # =========================================================================
  # Each controller's directory-entry `stored_extent` is its number of section 26 page
  # descriptors. Each descriptor supplies the record count for one page.
  # PADS writes page payloads in controller order 41,42,45,46,47,48.
  section_41_paged_controller:
    seq:
      - id: records
        type:
          switch-on: _root.version
          cases:
            0x2017: section_41_clearance_record_180_bytes
            _: section_41_clearance_record
        repeat: expr
        repeat-expr: '_root.num_section_41_records'

  section_42_paged_controller:
    seq:
      - id: records
        type: section_41_high_speed_rule_record
        repeat: expr
        repeat-expr: '_root.num_section_42_records'

  section_45_paged_controller:
    seq:
      - id: records
        size: '_root.version == 0x2017 ? 116 : 124'
        repeat: expr
        repeat-expr: '_root.num_section_45_records'
        doc: These are fixed-stride layer-rule objects for this allocator page.

  section_46_paged_controller:
    seq:
      - id: record_chain
        type: 'section_46_record_chain(_root.num_section_46_records)'
        if: _root.num_section_46_records > 0
    instances:
      num_live:
        value: '(_root.num_section_46_records > 0 ? record_chain.num_live : 0).as<u4>'

  section_46_record_chain:
    params:
      - id: num_records
        type: u4
    seq:
      - id: record
        type: section_46_heap_record
        if: num_records == 1
      - id: left
        type: 'section_46_record_chain(num_records / 2)'
        if: num_records > 1
      - id: right
        type: 'section_46_record_chain(num_records - num_records / 2)'
        if: num_records > 1
    instances:
      num_live:
        value: '(num_records == 1 ? (record.saved_rule_handle < 0x80000000 and record.saved_via_type_set_handle != 0 ? 1 : 0) : left.num_live + right.num_live).as<u4>'

  section_46_heap_record:
    doc: |
      This is a route-rule heap slot. A negative first word identifies a free
      slot. A null via-type-set handle also identifies an unused slot. Such a
      slot is retained capacity, not a live record.
    seq:
      - id: saved_rule_handle
        type: u4
      - id: saved_via_type_set_handle
        type: u4
      - id: route_rule_fields
        size: '_root.version <= 0x2019 ? 24 : 32'

  section_47_paged_controller:
    seq:
      - id: records
        size: 24
        repeat: expr
        repeat-expr: '_root.num_section_47_records'
        doc: These are fixed-stride via-rule objects for this allocator page.

  section_48_paged_controller:
    seq:
      - id: records
        size: '_root.version <= 0x2019 ? 48 : (_root.version <= 0x2022 ? 856 : 864)'
        repeat: expr
        repeat-expr: '_root.num_section_48_records'
        doc: These are fixed-stride differential-pair rule objects for this allocator page.

  section_41_design_rule_stream:
    params:
      - id: num_clearance_records
        type: u4
      - id: len_clearance_record
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
      - id: len_route_record
        type: u4
      - id: num_diff_pair_records
        type: u4
      - id: num_section_49_prefix_words
        type: u4
    seq:
      - id: rule_page
        type: section_41_rule_page(num_clearance_records, len_clearance_record)
      - id: layer_clearance_records
        type: section_41_legacy_layer_clearance_record
        repeat: expr
        repeat-expr: num_layer_clearance_records
      - id: rule_relation_prefixes
        type: section_41_rule_relation_prefix
        repeat: expr
        repeat-expr: num_rule_relation_prefixes
      - id: high_speed_records
        type: section_41_high_speed_rule_record
        repeat: expr
        repeat-expr: num_high_speed_records
      - id: per_layer_rule_matrices
        type: section_41_per_layer_rule_matrix(num_layers)
        repeat: expr
        repeat-expr: num_per_layer_rule_matrices
        doc: These are retained layer-sized design-rule matrices.
      - id: route_records
        type:
          switch-on: len_route_record
          cases:
            32: section_41_route_rule_record_32_bytes
            40: section_41_route_rule_record
        repeat: expr
        repeat-expr: num_route_records
      - id: diff_pair_records
        type: section_41_diff_pair_record
        repeat: expr
        repeat-expr: num_diff_pair_records
      - id: section_49_prefix
        type: section_49_route_rule_prefix(num_section_49_prefix_words)

  section_41_rule_page:
    params:
      - id: num_clearance_records
        type: u4
      - id: len_clearance_record
        type: u4
    seq:
      - id: controller_header
        type: section_41_rule_page_header
      - id: clearance_records
        type:
          switch-on: len_clearance_record
          cases:
            180: section_41_clearance_record_180_bytes
            188: section_41_clearance_record
        repeat: expr
        repeat-expr: num_clearance_records

  section_41_per_layer_rule_matrix:
    params:
      - id: num_values
        type: u4
    seq:
      - id: rule_selector_or_handle
        type: u4
        doc: Layer/rule selector in compact records. Saved process-local rule handle in expanded records.
      - id: clearance_values
        type: s4
        repeat: expr
        repeat-expr: num_values - 2
        doc: Signed BASIC-unit clearance values. -1 means inherited/not applicable.
      - id: rule_state
        type: u4
        doc: This is the compact rule ownership/controller state.

  section_41_rule_page_header:
    seq:
      - id: saved_list_heads
        type: u4
        repeat: expr
        repeat-expr: 11
        doc: These are the saved process-local heads for the design-rule category lists.
      - id: controller_state
        type: u4
        doc: This is the design-rule page controller state. Zero in the standard layout.

  section_41_clearance_record:
    doc: This is a 188-byte clearance rule with a 12-byte selector/handle header, 38 values, and 24 bytes of state.
    seq:
      - id: layer_selector
        type: u4
        doc: Zero for global rules. Nonzero layer ordinal for layer-specific rules.
      - id: saved_rule_handle
        type: u4
        doc: This is the process-local rule relationship handle. Never a file offset.
      - id: rule_state
        type: u4
        doc: This is the rule object state.
      - id: clearance_values
        type: s4
        repeat: expr
        repeat-expr: 38
        doc: CLEARANCE_RULE values in ASCII declaration order and BASIC units.
      - id: rule_metadata_tail
        size: 24
        doc: This is the shared live rule ownership, width, and controller metadata. Not padding.

  section_41_clearance_record_180_bytes:
    doc: This is a 180-byte v0x2017/v0x2019 clearance rule with 38 values and a 16-byte ownership trailer.
    seq:
      - id: layer_selector
        type: u4
      - id: saved_rule_handle
        type: u4
      - id: rule_state
        type: u4
      - id: clearance_values
        type: s4
        repeat: expr
        repeat-expr: 38
        doc: CLEARANCE_RULE values in BASIC units.
      - id: rule_metadata
        size: 16
        doc: This is the legacy ownership and controller metadata.

  section_41_legacy_layer_clearance_record:
    doc: This is a 188-byte v0x2019 layer/relationship-specific clearance rule.
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
        doc: CLEARANCE_RULE values in BASIC units.
      - id: rule_metadata
        size: 8
        doc: This is the legacy layer-rule ownership/controller metadata.

  section_41_rule_relation_prefix:
    doc: This is a 16-byte relationship/layer selector prefix attached to a next rule object.
    seq:
      - id: layer_rule_selector
        type: u4
      - id: recommended_track_width
        type: s4
      - id: saved_owner_handle
        type: u4
      - id: relationship_state
        type: u4

  section_41_high_speed_rule_record:
    doc: This is an 80-byte HIGH_SPEED_RULE object. Numeric fields are serialized in ASCII declaration order.
    seq:
      - id: saved_rule_handle
        type: u4
        doc: This is the process-local rule handle. Never a file offset.
      - id: min_length
        type: f8
        doc: ASCII MIN_LENGTH in BASIC units.
      - id: max_length
        type: f8
        doc: ASCII MAX_LENGTH in BASIC units.
      - id: stub_length
        type: s4
        doc: ASCII STUB_LENGTH.
      - id: parallel_length
        type: s4
        doc: ASCII PARALLEL_LENGTH.
      - id: parallel_gap
        type: s4
        doc: ASCII PARALLEL_GAP.
      - id: tandem_length
        type: s4
        doc: ASCII TANDEM_LENGTH.
      - id: tandem_gap
        type: s4
        doc: ASCII TANDEM_GAP.
      - id: min_delay
        type: f4
        doc: ASCII MIN_DELAY.
      - id: max_delay
        type: f4
        doc: ASCII MAX_DELAY.
      - id: min_capacitance
        type: f4
        doc: ASCII MIN_CAPACITANCE.
      - id: max_capacitance
        type: f4
        doc: ASCII MAX_CAPACITANCE.
      - id: min_impedance
        type: f4
        doc: ASCII MIN_IMPEDANCE.
      - id: max_impedance
        type: f4
        doc: ASCII MAX_IMPEDANCE.
      - id: shield_gap
        type: s4
        doc: ASCII SHIELD_GAP.
      - id: match_length_tolerance
        type: s4
        doc: ASCII MATCH_LENGTH_TOLERANCE.
      - id: rule_flags
        type: u4
        doc: Shield-net and match-length option state.
      - id: rule_state
        type: u4
        doc: This is the saved rule/controller state.

  section_41_route_rule_record:
    doc: This is a 40-byte ROUTE_RULE object. Option and layer masks correspond to the ASCII route-rule declarations.
    seq:
      - id: saved_rule_handle
        type: u4
        doc: This is the process-local rule handle. Never a file offset.
      - id: saved_via_type_set_handle
        type: u4
        doc: This is the process-local handle for the VALID_VIA_TYPE set.
      - id: length_minimization_type
        type: u4
        doc: ASCII LENGTH_MINIMIZATION_TYPE.
      - id: route_priority
        type: u4
        doc: ASCII ROUTE_PRIORITY.
      - id: route_option_flags
        type: u4
        doc: VIA_SHARE, TRACE_SHARE, AUTO_ROUTE, RIPUP, and SHOVE state.
      - id: valid_layer_mask
        type: u4
        doc: Bit mask formed by the ASCII VALID_LAYER declarations.
      - id: via_type_set_state
        type: u4
        doc: VALID_VIA_TYPE set controller state.
      - id: max_number_of_vias
        type: s4
        doc: ASCII MAX_NUMBER_OF_VIAS. -1 means unlimited.
      - id: rule_flags
        type: u4
      - id: rule_state
        type: u4

  section_41_route_rule_record_32_bytes:
    doc: This is a 32-byte v0x2017/v0x2019 ROUTE_RULE object.
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
        doc: This is the legacy combined maximum-via/controller state word.

  section_41_diff_pair_record:
    doc: This is an 864-byte DIF_PAIR rule slot. Active slots join member handles to section 23 nets.
    seq:
      - id: controller_state
        size: 12
      - id: saved_member_handle_a
        type: u4
        doc: This is the saved process-local relationship handle for the first member net.
      - id: saved_member_handle_b
        type: u4
        doc: This is the saved process-local relationship handle for the second member net.
      - id: relationship_state
        size: 12
        doc: Member-net relationship/controller state.
      - id: rule_matrix
        type: f8
        repeat: expr
        repeat-expr: 70
        doc: Diff-pair rule values. Element 0 is MAX_LENGTH, element 1 inherited GAP, and element 3 GAP override.
      - id: inherited_width
        type: s4
      - id: width_state
        type: s4
      - id: width_override
        type: s4
      - id: retained_allocator_capacity
        size: 260
        doc: Allocator capacity can contain 0xFF bytes or retained live rule values. It is not file padding.

  section_48_diff_pair_record_array:
    seq:
      - id: records
        type: section_41_diff_pair_record
        repeat: expr
        repeat-expr: _root.num_section_48_records

  # =========================================================================
  # SECTION 49 — route-object relationship stream
  # =========================================================================
  section_49_relationship_stream:
    seq:
      - id: signal_records
        type: section_49_signal_relationships
        repeat: eos

  section_49_signal_relationships:
    doc: |
      Connectivity lists owned by one live section 23 signal/net object. Record
      order is the live section 23 net order. Forward object IDs tagged 0x3c
      name section 60 route-junction ordinals. Their tagged 0x18 values name
      section 24 route-chain ordinals. This declared graph assigns route
      junctions to nets without geometry search.
    seq:
      - id: forward_relationships
        type: section_49_relationship_array(1)
      - id: reverse_relationships
        type: section_49_relationship_array(0)

  section_46_route_rule_state_array:
    params:
      - id: num_states
        type: u4
    seq:
      - id: states
        type: s4
        repeat: expr
        repeat-expr: num_states
        doc: Per-route-rule saved state. -2 marks an unassigned rule.

  section_49_relationship_array:
    params:
      - id: is_forward
        type: u1
    seq:
      - id: num_relationships
        type: u4
      - id: relationships
        type: section_49_relationship(is_forward)
        repeat: expr
        repeat-expr: num_relationships

  section_49_relationship:
    params:
      - id: is_forward
        type: u1
    seq:
      - id: object_id
        type: u4
        valid:
          expr: '(_ & 0xff000000) == (is_forward != 0 ? 0x3c000000 : 0x18000000) and (_ & 0x00ffffff) < (is_forward != 0 ? _root.directory_entries[60].num_items : _root.directory_entries[24].num_items)'
        doc: This is the saved tagged object identifier. 0x3c identifies a section 60 junction ordinal.
      - id: num_values
        type: u4
      - id: values
        type: u4
        valid:
          expr: '(_ & 0xff000000) == (is_forward != 0 ? 0x18000000 : 0x3c000000) and (_ & 0x00ffffff) < (is_forward != 0 ? _root.directory_entries[24].num_items : _root.directory_entries[60].num_items)'
        repeat: expr
        repeat-expr: num_values
        doc: These are saved tagged relationship members. 0x18 identifies section 24 route-chain ordinals.

  # Each populated payload is a sequence of four-byte storage words. The high byte
  # identifies a section 24 ordinal, a section 60 ordinal, a literal, or a sentinel.
  # Legacy-layout payloads can contain allocator-state words. These words are
  # live state, not padding.
  # Some route-rule controller variants have tagged storage words before the
  # directory-counted payload. Thus, the prefix count is explicit.
  section_49_route_rule_prefix:
    params:
      - id: num_words
        type: u4
    seq:
      - id: words
        type: section_49_storage_word
        repeat: expr
        repeat-expr: num_words

  section_49_storage_array:
    seq:
      - id: words
        type: section_49_storage_word
        repeat: eos

  section_49_storage_word:
    seq:
      - id: payload_low
        type: u2
        doc: This is the low 16 bits of the relationship ordinal, literal, or signed state.
      - id: payload_mid
        type: u1
        doc: These bits 16..23 of the relationship ordinal, literal, or signed state.
      - id: tag_or_high_state
        type: u1
        doc: The high byte identifies a literal, an ordinal, or a signed sentinel.

  active_layer_ordinal_vector:
    params:
      - id: num_layer_ordinals
        type: u4
    doc: This is a u16 ordinal vector next to section 49. Values are 0 through num_layer_ordinals-1.
    seq:
      - id: layer_ordinals
        type: u2
        repeat: expr
        repeat-expr: num_layer_ordinals

  # =========================================================================
  # NET-CLASS DEFINITIONS (trailing-heap, design-rule graph)
  # =========================================================================
  # The trailing region is a contiguous serialized object arena. We believe that
  # its pointer values are arena offsets, not memory addresses:
  #   file_offset = pointer - K
  # The value K depends on the serialized arena. Do not use a fixed K value.
  # Net-class membership uses value equality and does not use this conversion.
  net_class_name_record:
    doc: >
      This is a 280-byte net-class name record. Records occur in NET_CLASS
      declaration order. They occur in the variable allocator area before the
      layer table.
    seq:
      - id: saved_class_handle
        type: u4
        doc: At +0, this field contains a process-local net-class object handle. It is never a file offset.
      - id: name
        type: strz
        encoding: ASCII
        size: 40
        doc: At +4, this field contains the net-class name.
      - id: zero_membership_slots
        type: u4
        valid: 0
        repeat: expr
        repeat-expr: 56
        doc: Bytes +44 through +267 contain cleared retained membership capacity.
      - id: saved_controller_handle
        type: u4
        doc: At +268, this field contains the saved per-class controller handle.
      - id: controller_state
        type: u4
        doc: 'At +272, this field contains 0x80000000 when the terminal controller is live, otherwise zero.'
      - id: terminal_rule_kind
        type: u4
        doc: At +276, this field contains the terminal rule kind 0x29 when present. Otherwise, it is zero.

  net_class_name_record_v280_header:
    doc: Alternate 280-byte net-class object with controller state before the fixed name.
    seq:
      - id: retained_archive_state
        type: u4
        repeat: expr
        repeat-expr: 4
        doc: At +0..+15, this field contains retained archive or controller state. It is usually cleared.
      - id: saved_class_handle
        type: u4
        doc: 'At +16, this field contains saved net-class object handle.'
      - id: controller_state
        type: u4
        doc: At +20, this field contains saved controller state. The usual value is 0x80000000.
      - id: relationship_state
        type: u4
        doc: At +24, this field contains saved relationship state. The usual value is 0x80000000.
      - id: saved_name_handle
        type: u4
        doc: At +28, this field contains the saved handle associated with the inline class name.
      - id: name
        type: strz
        encoding: ASCII
        size: 40
        doc: 'At +32, this field contains fixed-width zero-terminated net-class name.'
      - id: zero_membership_slots
        type: u4
        valid: 0
        repeat: expr
        repeat-expr: 52
        doc: 'At +72..+279, this field contains cleared retained membership capacity.'

  compact_net_class_name_record_60_bytes:
    doc: This is the compact saved net-class membership/name association used by the 0x2026/0x2027 archive dialect.
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
        doc: 'At +24, this field contains one-based name/member ordinal.'
      - id: name
        type: strz
        encoding: ASCII
        size: 16
        doc: 'At +28, this field contains fixed-width zero-terminated compact class name.'
      - id: saved_controller_handle
        type: u4
      - id: saved_owner_handle
        type: u4
      - id: saved_membership_handle
        type: u4
      - id: capacity_flags
        type: u4
        doc: At +56, this field contains retained-capacity flags. The values are 0x100 and 0x400.
  design_rule_relationship_record:
    doc: >
      This is a 28-byte design-rule graph edge immediately before the layer table. Scope
      types 3, 0x17, and 0x42 select none/default, NET, and NET_CLASS references.
      Their next words are sentinel or reference values, not constant masks.
      The last word is the rule kind. Saved handles are process-local
      relationship identifiers, not file offsets.
    seq:
      - id: rule_detail_handle
        type: u4
        doc: At +0, this field contains the saved rule-detail relationship handle.
      - id: scope_a_type
        type: u4
        doc: At +4, this field contains the first scope type.
      - id: scope_a_reference
        type: u4
        doc: At +8, this field contains the first scope reference or default sentinel.
      - id: scope_b_type
        type: u4
        doc: At +12, this field contains the second scope type.
      - id: scope_b_reference
        type: u4
        doc: At +16, this field contains the second scope reference or default sentinel.
      - id: layer_or_state
        type: u4
        doc: 'At +20, this field contains layer selector or relationship state.'
      - id: rule_kind
        type: u4
        doc: 'At +24, this field contains design-rule kind enum.'

  pre_layer_design_rule_stream:
    params:
      - id: num_alignment_zeros
        type: u4
      - id: num_relationships
        type: u4
      - id: num_trailing_zeros
        type: u4
    doc: This is the controller state and design-rule graph edges immediately before the layer table.
    seq:
      - id: controller_words
        type: u4
        repeat: expr
        repeat-expr: 8
        doc: This is the retained default-rule handles, kinds, masks, and controller state.
      - id: alignment_zeros
        type: u1
        valid: 0
        repeat: expr
        repeat-expr: num_alignment_zeros
        doc: Zero byte alignment before the 28-byte relationship array.
      - id: relationships
        type: design_rule_relationship_record
        repeat: expr
        repeat-expr: num_relationships
      - id: trailing_zeros
        type: u1
        valid: 0
        repeat: expr
        repeat-expr: num_trailing_zeros
        doc: Zero controller tail before the layer-record array.

  pre_layer_design_rule_controller_44_bytes:
    doc: This is a 44-byte modern default-rule controller used when no relationship-edge array follows.
    seq:
      - id: controller_words
        type: u4
        repeat: expr
        repeat-expr: 11
        doc: This is the rule kinds, saved handles, layer selectors, masks, and controller state.

  pre_layer_design_rule_controller_28_bytes:
    doc: This is a 28-byte legacy default-rule controller through database version 0x2022.
    seq:
      - id: controller_words
        type: u4
        repeat: expr
        repeat-expr: 7
        doc: This is the legacy rule kinds, saved handles, selectors, and state.

  pre_layer_relationship_controller:
    doc: This is a 28-byte saved relationship-list controller for prior graph edges or net-class objects.
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
        doc: The value is zero or 0x80000000.
      - id: rule_kind
        type: u4

  pre_layer_saved_relationship_terminator:
    doc: This is a 28-byte terminal saved relationship object. The eight-byte zero controller tail follows it.
    seq:
      - id: num_associations
        type: u4
        doc: The values are zero and one.
      - id: unknown_zero_at_4
        type: u4
        valid: 0
        doc: At +4, this field is zero. The purpose of this field is not known.
      - id: saved_object_handle
        type: u4
        doc: This is the saved object identifier with class byte 0x16.
      - id: zero_state
        type: u4
        valid: 0
        repeat: expr
        repeat-expr: 2
      - id: saved_scope_handle
        type: u4
        doc: Zero or a saved 0x0a-class scope handle.
      - id: rule_kind
        type: u4
        valid: 0x86

  pre_layer_retained_zero_relationship_slot:
    doc: This is the retained 28-byte relationship-capacity slot. Owned storage, not alignment padding.
    seq:
      - id: zero_state
        type: u4
        valid: 0
        repeat: expr
        repeat-expr: 7

  pre_layer_terminal_rule_kind:
    doc: Four-byte terminal rule selector in the smallest archive dialect.
    seq:
      - id: rule_kind
        type: u4
        doc: The values are 6 and 0x86.

  pre_layer_allocator64_stream:
    params:
      - id: num_allocator_records
        type: u4
    doc: >
      This 64-byte pre-layer allocator variant contains counted allocator
      records and an eight-byte controller prefix. Three terminal relationship
      records and an eight-byte zero tail follow them.
    seq:
      - id: allocator_records
        type: pre_layer_allocator_record_64_bytes
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

  pre_layer_allocator_record_64_bytes:
    doc: This is a 64-byte saved design-rule allocator object with process-local handles and zero state.
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
        doc: This is one-based allocator object ordinal.
      - id: object_flags
        type: u4
        doc: The values are 0x00400001 and 0x00480101.
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

  section_50_relationship_halfword:
    doc: >
      One u16 directory slice through the four-byte tagged object-relationship
      stream next section 49. Section 50 can start or end in the middle of
      a relationship token. Thus, each halfword contains an object identifier or
      tag state.
    seq:
      - id: relationship_token_half
        type: u2

  section_51_relationship_word:
    doc: >
      One u32 boundary word in the object-relationship stream between section
      50 and the copper-outline or string-index storage.
    seq:
      - id: relationship_token_word
        type: u4

  section_50_relationship_array:
    seq:
      - id: records
        type: section_50_relationship_halfword
        repeat: eos

  section_51_relationship_array:
    seq:
      - id: records
        type: section_51_relationship_word
        repeat: eos

  # =========================================================================
  # SECTIONS 52..55 -- copper-outline owner, piece, vertex and arc arrays
  # =========================================================================
  # Sections 52 through 55 usually contain four fixed arrays. A legacy variant
  # uses `legacy_object_relationship_token` instead.
  legacy_object_relationship_token:
    doc: |
      This four-byte object-relationship token occurs in a legacy section 52
      variant. `tag` is the destination section number. This is not the usual
      layout of sections 52 through 55.
    seq:
      - id: value_low
        type: u2
        doc: This field contains the low 16 bits of the object identifier or literal count.
      - id: value_middle
        type: u1
        doc: Bits 16 through 23 are usually zero for an in-range handle.
      - id: tag
        type: u1
        doc: The high byte identifies a literal or a saved object handle.

  legacy_object_relationship_token_array:
    seq:
      - id: records
        type: legacy_object_relationship_token
        repeat: eos

  section_52_outline_owner_array:
    seq:
      - id: records
        type: section_52_outline_owner
        repeat: eos

  section_53_outline_piece_array:
    seq:
      - id: records
        type: section_53_outline_piece
        repeat: eos

  section_54_outline_vertex_array:
    seq:
      - id: records
        type: section_54_outline_vertex
        repeat: eos

  section_55_outline_arc_array:
    seq:
      - id: records
        type: section_55_outline_arc
        repeat: eos

  section_52_outline_owner:
    doc: |
      This is an 88-byte copper-outline owner. Covers POUROUT, HATOUT, VOIDOUT, VIATHERM
      and related ASC objects. The first three fields are cumulative indices into
      sections 53, 54 and 55. Piece and vertex indices remain file-global. An
      allocator-page transition may restart arc_start at zero. Coordinates are RAW.
    seq:
      - id: piece_start
        type: u4
        doc: This is the first section 53 piece ordinal.
      - id: vertex_start
        type: u4
        doc: This is the first section 54 vertex ordinal.
      - id: arc_start
        type: u4
        doc: This is the first section 55 arc ordinal. May restart at an allocator-page boundary.
      - id: relationship_id
        type: s4
        doc: Signed outline relationship identifier.
      - id: object_handle
        type: s4
        doc: Database object handle.
      - id: parent_relationship_id
        type: s4
        doc: Parent relationship identifier. -1 for a root outline.
      - id: location_x
        type: s4
        doc: ASC XLOC in RAW coordinates. Zero for child outlines with absolute vertices.
      - id: location_y
        type: s4
        doc: ASC YLOC in RAW coordinates. Zero for child outlines with absolute vertices.
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
        doc: The value is -1 when this field is not used.
      - id: num_pieces
        type: u4
        doc: This is the number of consecutive section 53 pieces owned by this outline.
      - id: name_flags
        type: u2
        doc: We believe that these bits identify POUROUT, HATOUT, and VOIDOUT states.
      - id: name
        size: 14
        doc: |
          Retained 14-byte outline-name storage. Live records use NUL-padded
          ASCII names with a POR or ANP prefix. Free records can retain arbitrary bytes.
      - id: outline_type
        type: u4
        doc: Type enum in the high byte. 0x32 POUROUT, 0x33 HATOUT, 0x34 VOIDOUT, others related outline types.

  section_53_outline_piece:
    doc: This is one 16-byte outline-geometry piece.
    seq:
      - id: num_corners
        type: u4
      - id: num_arcs
        type: u4
      - id: width
        type: s4
        doc: Outline width in BASIC units.
      - id: piece_type
        type: u1
        doc: The value 0x32 selects a polygon. The value 0x33 selects a circle.
      - id: layer
        type: u1
        doc: This is a one-based PADS layer ordinal.
      - id: piece_flags
        type: u2

  section_54_outline_vertex:
    doc: This is an 8-byte outline vertex in the owner's coordinate convention.
    seq:
      - id: x
        type: s4
      - id: y
        type: s4

  section_55_outline_arc:
    doc: |
      This record adds an arc to one section 54 vertex. The angle word contains
      two signed 16-bit ASC angle values.
    seq:
      - id: x
        type: s4
        doc: This is the decorated vertex X coordinate.
      - id: y
        type: s4
        doc: This is the decorated vertex Y coordinate.
      - id: arc_index
        type: u4
        doc: This is the zero-based arc ordinal within the owning piece.
      - id: begin_angle
        type: s2
        doc: ASC begin angle.
      - id: delta_angle
        type: s2
        doc: ASC signed sweep angle.

  # =========================================================================
  # SECTION 59 — circular route-style/header object array
  # =========================================================================
  section_59_route_header_ring:
    params:
      - id: num_records
        type: u4
      - id: record_stride
        type: u4
      - id: len_last_record_tail
        type: u4
    doc: |
      Circular record array. Logical record zero begins at physical +12 in
      v0x2024, +4 in v0x2025 and later, and +16 through v0x2022. These offsets
      follow directly from the versioned serialized class layout.
    seq:
      - id: last_record_tail
        size: len_last_record_tail
      - id: full_records
        type:
          switch-on: _root.version
          cases:
            0x2024: section_59_route_header_v2024
            0x2025: section_59_route_header_modern
            0x2026: section_59_route_header_modern
            0x2027: section_59_route_header_modern
            _: section_59_route_header_legacy
        repeat: expr
        repeat-expr: num_records - 1
      - id: last_record_head
        size: record_stride - len_last_record_tail

  section_59_route_header_modern:
    seq:
      - id: self_handle
        type: u4
      - id: link_handle
        type: u4
      - id: class_tag
        type: u4
        doc: The value 0x2001 identifies a route-style header. Other values identify allocator object classes.
      - id: half_width
        type: s4
      - id: layer_index
        type: u4
      - id: relationship_state
        type: u4
      - id: flags
        type: u4
      - id: trailing_state
        type: u4

  section_59_route_header_v2024:
    seq:
      - id: class_tag
        type: u4
      - id: half_width
        type: s4
      - id: layer_index
        type: u4
      - id: relationship_state
        type: u4
      - id: flags
        type: u4
      - id: trailing_state
        size: 12

  section_59_route_header_legacy:
    seq:
      - id: half_width
        type: s4
      - id: layer_index
        type: u4
      - id: relationship_state
        type: u4
      - id: node_a_handle
        type: u4
      - id: node_b_handle
        type: u4
      - id: flags
        type: u4

  section_59_to_64_stream:
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
    doc: This is the full physical section 59 through section 64 route/object stream.
    seq:
      - id: heap_objects
        type:
          switch-on: heap_object_stride
          cases:
            24: section_59_heap_object_24_bytes
            32: section_59_heap_object_32_bytes
        repeat: expr
        repeat-expr: num_heap_objects
        if: num_heap_objects > 0
      - id: route_junctions
        type: 'section_60_route_junction_ring(num_route_junctions, route_junction_stride)'
        if: num_route_junctions > 0
      - id: object_handles
        type: section_61_object_handle
        repeat: expr
        repeat-expr: num_object_handles
        if: num_object_handles > 0
      - id: route_objects
        type: 'section_62_route_object_ring(num_route_objects, route_object_stride)'
        if: num_route_objects > 0
      - id: route_layers
        type: section_63_route_layer
        repeat: expr
        repeat-expr: num_route_layers
        if: num_route_layers > 0
      - id: route_cells
        type: section_64_route_coordinate_pool
        repeat: expr
        repeat-expr: num_route_cells
        if: num_route_cells > 0

  section_59_heap_object_32_bytes:
    doc: This is the modern 32-byte heap-object record. No board geometry.
    seq:
      - id: pointer_0
        type: u4
      - id: pointer_1
        type: u4
      - id: pointer_2
        type: u4
      - id: class_tag
        type: u4
        doc: Class enum including 0x2001, 0x2000, 0x2400 and 0x1000.
      - id: scalar
        type: s4
        doc: BASIC width/clearance-class scalar, or zero.
      - id: object_state
        type: u4
      - id: object_handle
        type: u4
      - id: flags
        type: u4

  section_59_heap_object_24_bytes:
    doc: This is the legacy 24-byte heap-object record. No board geometry.
    seq:
      - id: pointer_0
        type: u4
      - id: class_tag
        type: u4
      - id: scalar
        type: s4
        doc: BASIC width/clearance-class scalar.
      - id: object_state
        type: u4
      - id: object_handle
        type: u4
      - id: pointer_1
        type: u4

  # =========================================================================
  # SECTION 60 — route-junction records
  # =========================================================================
  # The physical section begins at the X field of logical record zero. This is
  # byte 33 in modern records and byte 17 in legacy records. The prefix before X
  # is rotated to the physical end. Both dialects share tail-relative fields.
  # These fields identify X, Y, the route-object handle, the via, the type, and
  # the net index. Head bytes +1 and +2 carry the previous logical junction's
  # route-transition layers. Mask each with 0x1f because bit 0x20 is saved state.
  section_60_route_junction_ring:
    params:
      - id: num_records
        type: u4
      - id: record_stride
        type: u4
    doc: X-field-left-rotated physical storage for the logical junction records.
    seq:
      - id: first_record_tail
        type: section_60_route_junction_tail
        doc: This is the logical record zero from X through its last byte.
      - id: remaining_records
        type:
          switch-on: record_stride
          cases:
            64: section_60_route_junction_64_bytes
            48: section_60_route_junction_48_bytes
        repeat: expr
        repeat-expr: num_records - 1
      - id: first_record_head
        type:
          switch-on: record_stride
          cases:
            64: section_60_route_junction_head_33_bytes
            48: section_60_route_junction_head_17_bytes
        doc: This is the logical record-zero state before X. It is rotated to the physical end.

  section_60_route_junction_head_33_bytes:
    seq:
      - id: previous_junction_class_state
        type: u1
        doc: The value 0x17 identifies a previous 0x0E junction that carries a physical via.
      - id: previous_transition_start_layer_raw
        type: u1
        doc: The low five bits contain the routed start layer of the previous logical junction. Bit 0x02 marks a physical via. Bit 0x20 is saved state.
      - id: previous_transition_end_layer_raw
        type: u1
        doc: The low five bits contain the routed end layer of the previous logical junction. Bit 0x20 is saved state.
      - id: previous_transition_flags
        type: u1
      - id: object_link_state
        size: 29
        doc: These are saved junction object links, allocator handles, and state before the coordinates.
    instances:
      previous_carries_physical_via:
        value: 'previous_junction_class_state == 0x17 and (previous_transition_start_layer_raw & 0x02) != 0'
        doc: Distinguishes a physical via from other 0x0e layer-transition junctions.

  section_60_route_junction_head_17_bytes:
    seq:
      - id: previous_junction_class_state
        type: u1
      - id: previous_transition_start_layer_raw
        type: u1
      - id: previous_transition_end_layer_raw
        type: u1
      - id: previous_transition_flags
        type: u1
      - id: object_link_state
        size: 13
    instances:
      previous_carries_physical_via:
        value: 'previous_junction_class_state == 0x17 and (previous_transition_start_layer_raw & 0x02) != 0'

  section_60_route_junction_tail:
    doc: This is the shared last 31 bytes of a legacy or modern route-junction record.
    seq:
      - id: x_raw
        type: s4
      - id: y_raw
        type: s4
      - id: route_object_handle
        type: u4
        doc: A nonzero value joins section 29 and its section 27 layer group.
      - id: relationship_state
        size: 12
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

  section_60_route_junction_64_bytes:
    doc: This is the modern 64-byte route-junction/via record.
    seq:
      - id: object_state
        type: section_60_route_junction_head_33_bytes
        doc: These are the serialized object handles, links, and the previous junction's route transition.
      - id: x_raw
        type: s4
        doc: This is the unmodified X coordinate.
      - id: y_raw
        type: s4
        doc: This is the unmodified Y coordinate.
      - id: route_object_handle
        type: u4
        doc: |
          This is the saved route-object handle. Zero means that there is no
          layer object. A nonzero value identifies a section 29 handle. Section
          27 then gives the copper-layer group.
      - id: relationship_state
        size: 12
        doc: Route-chain links and role flags.
      - id: via_definition_index
        type: u1
      - id: pre_type_state
        type: u2
      - id: junction_type
        type: u1
        doc: The value 0x16 identifies a corner. The value 0x0E identifies a via or connection.
      - id: net_index
        type: u2
      - id: trailing_state
        type: u1

  section_60_route_junction_48_bytes:
    doc: This is the legacy 48-byte route-junction/via record.
    seq:
      - id: object_state
        type: section_60_route_junction_head_17_bytes
      - id: x_raw
        type: s4
      - id: y_raw
        type: s4
      - id: route_object_handle
        type: u4
        doc: A nonzero value joins section 29 and its section 27 layer group.
      - id: relationship_state
        size: 12
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
  # These are 12-byte records at the accumulated directory offset.
  section_61_object_handle:
    doc: |
      This is a 12-byte route-allocator node. Its page and 56-byte page ordinal
      identify its handle. Section 29 stores these handles in layer groups. Zero
      is a null handle.

      The first two words are graph links. The third word is an object-class
      tag. This record does not contain geometry. We believe that bit 0x00800000
      identifies the node that introduces a section 62 route object.
    seq:
      - id: previous_handle
        type: u4
      - id: next_handle
        type: u4
      - id: class_tag
        type: u4

  # =========================================================================
  # SECTION 62 — route-object array
  # =========================================================================
  # The section is a ring rotated left by 32 physical bytes. Physical +0 is the
  # last record's 32-byte tail. Logical record 0 starts at physical +32. The last
  # record's remaining head closes the section. Modern logical records are 48 bytes.
  # Legacy records are 36 bytes. Width = quarter_width*4.
  section_62_route_object_48_bytes:
    seq:
      - id: leading_tag
        type: u4
        doc: We believe that +0 contains an object type tag or a saved handle.
      - id: next_pointer
        type: u4
        doc: We believe that +4 contains the next saved pointer. It is usually zero.
      - id: id_or_pointer
        type: u4
        doc: We believe that +8 contains an object identifier or a saved pointer.
      - id: sign_bit_flags
        type: u4
        doc: We believe that +12 contains flags. Its usual value is 0x80000000.
      - id: object_pointer
        type: u4
        doc: We believe that +16 contains a saved object pointer.
      - id: quarter_width
        type: s4
        doc: 'At +20, this field contains route_width = quarter_width * 4, BASIC.'
      - id: lower_bound
        type: s4
        doc: 'At +24, this field contains cached low variable-axis bound of the full route/jumper/via object.'
      - id: upper_bound
        type: s4
        doc: 'At +28, this field contains cached high variable-axis bound of the full route/jumper/via object.'
      - id: flags
        type: u4
        doc: |
          At +32, this field contains PADS ROUTE flags. Bit 0x100 is the serialized jumper-object bit and
          bit 0x1000 is the via or special-object bit. The other bits contain
          thermal, teardrop, and routing state. Ordinary routed copper has both
          class bits clear.
      - id: num_cells
        type: u4
        doc: At +36, this field contains the number of section 64 cells that this object owns.
      - id: pointer_b
        type: u4
        doc: We believe that +40 contains a saved pointer. It is usually zero.
      - id: trailing_tag
        type: u4
        doc: 'At +44, this field contains mirror of leading_tag.'

  section_62_route_object_36_bytes:
    seq:
      - id: leading_tag
        type: u4
      - id: object_handle
        type: u4
      - id: quarter_width
        type: s4
        doc: Route_width = quarter_width * 4, BASIC.
      - id: lower_bound
        type: s4
        doc: This is the cached low variable-axis bound of the full route/jumper/via object.
      - id: upper_bound
        type: s4
        doc: This is the cached high variable-axis bound of the full route/jumper/via object.
      - id: style
        type: u4
        doc: |
          PADS ROUTE flags. 0x100 selects a jumper object and 0x1000 selects a
          via/special object. Ordinary routed copper has both class bits clear.
      - id: num_cells
        type: u4
      - id: relationship_handle
        type: u4
      - id: trailing_tag
        type: u4
        doc: Mirror of leading_tag.

  section_62_route_object_ring:
    params:
      - id: num_records
        type: u4
      - id: record_stride
        type: u4
    doc: This is a 32-byte-left-rotated physical section 62 record ring.
    seq:
      - id: last_record_tail
        size: 32
      - id: previous_records
        type:
          switch-on: record_stride
          cases:
            36: section_62_route_object_36_bytes
            48: section_62_route_object_48_bytes
        repeat: expr
        repeat-expr: num_records - 1
      - id: last_record_head
        size: record_stride - 32

  section_63_route_layer:
    doc: This is the serialized route-layer ordinal. The array is a permutation of active layer indices.
    seq:
      - id: layer_index
        type: u2

  # =========================================================================
  # SECTION 64 — route coordinate pool
  # =========================================================================
  # These are 12-byte geometry cells. For an ordinary section 62 route object, the
  # three values encode two points sharing one axis. The per-layer section 69
  # routing direction selects their order: V stores
  # (first_major,shared_minor)=(X,Y). H and NO_PREFERENCE store the transposed
  # form. Jumper (flags&0x100) and via/special (flags&0x1000) objects use their
  # owned cells for auxiliary object geometry/state instead of routed-copper
  # polylines. An ordinary object whose only cell has identical first/second
  # major values is a retained route endpoint, not a copper segment. Section-62
  # num_cells values partition this pool.
  section_64_route_coordinate_pool:
    seq:
      - id: first_major_or_auxiliary_word_0
        type: s4
      - id: shared_minor_or_auxiliary_word_1
        type: s4
      - id: second_major_or_auxiliary_word_2
        type: s4

  section_65_saved_group_record:
    doc: |
      This is a 28-byte saved GROUP object. The first three words contain saved
      group-list state. The last 16-byte field contains the NUL-terminated group
      name. Bytes after the terminator are retained string state, not padding.
    seq:
      - id: saved_group_state
        type: u4
        valid: 0
        doc: This is serialized group-controller state. The value is zero.
      - id: saved_member_head_handle
        type: u4
        doc: This is the process-local group member-list head. It is null when no saved list is retained.
      - id: saved_member_tail_handle
        type: u4
        doc: This is the process-local group member-list tail. It is null when no saved list is retained.
      - id: name_storage
        type: strz
        encoding: ASCII
        size: 16
        doc: This is the active GROUP name followed by retained fixed-slot capacity.

  section_65_to_66_compact_net_class:
    doc: |
      This 28-byte saved net-class object is used through v0x2022. It contains a
      fixed eight-byte name and a saved class handle. Sections 23 and 67 can
      refer to this handle.
    seq:
      - id: class_state
        type: u4
      - id: saved_class_handle
        type: u4
      - id: name
        type: str
        encoding: ASCII
        size: 8
      - id: saved_previous_handle
        type: u4
      - id: saved_next_handle
        type: u4
      - id: retained_state
        type: u4

  section_65_to_66_net_class:
    doc: |
      This is a 280-byte saved net-class object. It contains a 48-byte name and
      216 bytes of retained membership or rule-list state. The last two words
      are controller state, not padding.
    seq:
      - id: class_flags
        type: u4
        doc: The values are zero, 0x40000000, and 0x80000000.
      - id: saved_class_handle
        type: u4
        doc: This is the process-local net-class object handle. It is null on the last class in a paired list.
      - id: name
        type: strz
        encoding: ASCII
        size: 48
      - id: retained_membership_and_rule_state
        type: u4
        repeat: expr
        repeat-expr: 54
        doc: This is the saved member-list and rule-object capacity. It is zero when no state is retained.
      - id: saved_controller_handle
        type: u4
      - id: controller_state
        type: u4
        doc: Zero or 0x80000000 serialized object state.

  # =========================================================================
  # SECTION 67 — design-rule relationship graph
  # =========================================================================
  # Physical storage rotates the logical relationship record right by one u32.
  # `rule_kind` occurs first. A saved relationship handle, two scope pairs, and
  # layer state follow it.
  section_67_design_rule_relationship:
    seq:
      - id: rule_kind
        type: u4
        doc: This is the design-rule kind enum (0, 6, 0x29, 0x2a, 0x2d, 0x2e, 0x30, or 0x86).
      - id: rule_detail_handle
        type: u4
        doc: This is the saved process-local relationship handle.
      - id: scope_a_type
        type: u4
        doc: |
          This is the first scope type. The values are 3, 0x17, 0x18, 0x41,
          0x42, and 0x4a.
      - id: scope_a_reference
        type: u4
        doc: This is the first saved scope reference. The value 0x03000000 is the sentinel for the default scope.
      - id: scope_b_type
        type: u4
      - id: scope_b_reference
        type: u4
      - id: layer_or_state
        type: u4
        doc: This is the layer selector or relationship state.

  section_67_design_rule_relationship_array:
    seq:
      - id: records
        type: section_67_design_rule_relationship
        repeat: eos

  cluster_record_array:
    seq:
      - id: records
        type: cluster_record
        repeat: eos

  section_69_controller_leadin:
    doc: This is a 12-byte terminal rule selector and cleared layer-controller state.
    seq:
      - id: terminal_rule_kind
        type: u4
        doc: The low byte is 6 or 0x86. Bit 31 can retain controller state.
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
  # Sections 70 through 73 follow the layer array. They contain layer state,
  # global display preferences, error conflicts, and font faces.
  # STACKUP SOURCE: layer_thickness@+52 and copper_thickness@+56 are BASIC units.
  # The f4 value at +60 is the dielectric constant.
  # The board-setup MAXIMUMLAYER field gives the active copper-record count after
  # the aggregate record. Retained records beyond that count can keep nonzero
  # copper thicknesses. The array starts 12 bytes after section 69's
  # physical controller boundary.
  section_69_layer_record:
    params:
      - id: num_misc_colors
        type: u4
    doc: Version-sized layer definition + physical stackup + display-color record.
    seq:
      - id: name
        size: 24
        doc: 'At +0, this field contains NUL-padded layer name in live records. Last retained-capacity records can contain arbitrary bytes.'
      - id: layer_state_0
        type: s4
        doc: At +24, this field contains layer-controller state. It is usually zero.
      - id: layer_state_1
        type: s4
        doc: At +28, this field contains layer-controller state. It is usually zero.
      - id: routing_dir
        type: s4
        doc: |
          At +32, this field contains ROUTING_DIRECTION: 0=H, 1=V, 2=NO_PREFERENCE, 3=45, 4=-45. Section-64
          route cells on this layer use (shared_minor, first_major/second_major)
          as (X,Y) for H/NO_PREFERENCE/45/-45 and
          (first_major/second_major, shared_minor) for V.
      - id: assoc_silk
        type: s4
        doc: 'At +36, this field contains ASSOCIATED_SILK_SCREEN doc-layer # (-1 none).'
      - id: assoc_paste
        type: s4
        doc: 'At +40, this field contains ASSOCIATED_PASTE_MASK doc-layer #.'
      - id: assoc_mask
        type: s4
        doc: 'At +44, this field contains ASSOCIATED_SOLDER_MASK doc-layer #.'
      - id: assoc_assembly
        type: s4
        doc: 'At +48, this field contains ASSOCIATED_ASSEMBLY doc-layer #.'
      - id: layer_thickness
        type: s4
        doc: 'At +52, this field contains LAYER_THICKNESS, BASIC.'
      - id: copper_thickness
        type: s4
        doc: 'At +56, this field contains COPPER_THICKNESS, BASIC.'
      - id: dielectric
        type: f4
        doc: 'At +60, this field contains DIELECTRIC constant Er (3.3 / 4.3).'
      - id: color_route
        type: s4
        doc: 'At +64, this field contains ROUTE color (palette index).'
      - id: color_via
        type: s4
        doc: 'At +68, this field contains VIA color (== route).'
      - id: color_pad
        type: s4
        doc: 'At +72, this field contains PAD color (== route).'
      - id: color_copper
        type: s4
        doc: 'At +76, this field contains COPPER color (== route).'
      - id: color_2dline
        type: s4
        doc: 'At +80, this field contains 2DLINE color (== route).'
      - id: color_text
        type: s4
        doc: 'At +84, this field contains TEXT color (== route).'
      - id: color_error
        type: s4
        doc: 'At +88, this field contains element color (14 on data layers, 0/1 empty).'
      - id: misc_colors
        type: s4
        repeat: expr
        repeat-expr: num_misc_colors
        doc: These are the remaining display colors. There are 6 slots through v0x2021, 8 in v0x2022, and 12 from v0x2024.
      - id: flags
        type: s4
        doc: This attribute bitfield is at +116 through v0x2021, +124 in v0x2022, and +140 thereafter. Bits 0 through 2 control routing, visibility, and selection.
      - id: layer_state_2
        type: s4
        doc: This layer-controller state is at +120 through v0x2021, +128 in v0x2022, and +144 thereafter. The last record can retain allocator contents.
      - id: next_layer_type
        type: s4
        enum: layer_type
        doc: |
          This field is at +124 through v0x2021, +132 in v0x2022, and +148
          thereafter. It contains the next record's ASCII LAYER_TYPE value.
          The values are 0 for UNASSIGNED, 1 for ROUTING, 2 for DRILL, 3 for
          SILK_SCREEN, 4 for PASTE_MASK, 5 for SOLDER_MASK, and 6 for ASSEMBLY.
          The last physical record has no successor. Thus, this word contains
          retained carrier state.

  section_69_layer_record_array:
    params:
      - id: num_misc_colors
        type: u4
      - id: num_records
        type: u4
    seq:
      - id: records
        type: 'section_69_layer_record(num_misc_colors)'
        repeat: expr
        repeat-expr: num_records

enums:
  layer_type:
    0: unassigned
    1: routing
    2: drill
    3: silk_screen
    4: paste_mask
    5: solder_mask
    6: assembly
  pad_shape:
    0: of    # oblong / oval finger
    1: rf    # rectangular finger
    2: r     # round
    3: s     # square
