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
# The controller-slot count is stored directly in directory entry 1. PADS reads
# that many 16-byte slots from file offset 6, so the offset-10 directory view has
# num_directory-1 full 16-byte entries and one final 12-byte entry. The file ends
# with the footer GUID and a u32 back-pointer to the container-item array.

meta:
  id: pads_pcb_binary
  title: PADS PowerPCB binary layout (.pcb)
  file-extension: pcb
  endian: le
  encoding: ASCII
  imports:
    - microsoft_cfb

doc: The purpose of this field is not known.
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
    doc: |
      Four zero bytes. The PADS loader first consumes the three u16 header words,
      then reads the controller-directory memory image starting at file offset 6.
      These bytes therefore become the unused +0 word of controller slot 0; the
      visible count/extent pairs begin at file offset 10. Zero on all 597 unique
      binaries in the four-root corpus.
  - id: directory
    type: 'dir_entry(_index + 1 < num_directory)'
    repeat: expr
    repeat-expr: 'num_directory'
    doc: |
      A four-byte-shifted view of the controller slots PADS reads from offset 6.
      The first num_directory-1 views are 16 bytes because they include the next
      slot's zero word; the final view is 12 bytes and ends exactly at
      loader_stream_start. Flat
      controllers use `total_bytes` as their serialized byte extent. Paged
      controllers use the same word as a count of 12-byte descriptors stored in
      section 26; their serialized extent is the sum of descriptor record counts
      times the version-specific controller stride.
  - id: physical_body
    type: physical_file_body
    size: 'footer.cntr_item_back_ptr - loader_stream_start'
    doc: |
      Exact sequential ownership of every database byte after the shifted
      controller directory and before the OLE container-item array. Logical
      circular-array views remain root instances and never determine this
      stream's boundaries.
  - id: physical_container_items
    type: cntr_item_array
    size: '_io.size - 42 - footer.cntr_item_back_ptr'
    doc: serialized OLE container-item array at the footer back-pointer
  - id: physical_footer
    type: footer
    doc: final 42-byte MFC document footer

instances:
  num_directory:
    value: _root.directory_probe.count
    doc: |
      Stored controller-slot count, used directly. The prior `count - 1` claim
      came from treating the loader's offset-6 memory-image read as an offset-10
      array read. Full-corpus values: v0x2017=73; v0x2019/v0x2021/v0x2022/v0x2024=74;
      v0x2025/v0x2026/v0x2027=75. Directory entry 1 satisfies
      `total_bytes == count * 16` on every corpus binary.
  loader_stream_start:
    value: '6 + num_directory * 16'
    doc: |
      CArchive cursor after the loader reads the directory memory image. Because
      that read begins at offset 6, the offset-10 shifted directory view ends
      with a 12-byte final entry at this same cursor. Flat tag 2 begins here.
  directory_probe:
    pos: 26
    type: 'dir_entry(true)'
    doc: directory entry 1, read early because it carries the entry count
  footer:
    pos: '_io.size - 42'
    type: footer
    doc: final 42-byte MFC document footer, anchored from EOF
  container_items:
    pos: footer.cntr_item_back_ptr
    type: cntr_item_array
    size: '_io.size - 42 - footer.cntr_item_back_ptr'
    doc: root serialized OLE container-item array, reached by the footer back-pointer
  view_state_records:
    pos: loader_stream_start
    type: sec2_view_state_array
    size: directory[2].total_bytes
    doc: section 2 records are physically written before section 1
  board_setup:
    pos: 'loader_stream_start - 12 + directory[2].total_bytes'
    type: sec1_board_setup
    size: directory[1].total_bytes
    doc: rotated section-1 logical view; its first 12 bytes precede the physical tag-2/tag-3 controller cursor
  section3_serialized_size:
    value: directory[3].total_bytes
    doc: exact physical tag-3 controller extent read by the loader
  board_parameters:
    pos: 'loader_stream_start + directory[2].total_bytes'
    type: sec3_physical_controller
    size: section3_serialized_size
    doc: complete physical tag-3 database/board-parameter controller image
  section4_physical_offset:
    value: 'loader_stream_start + directory[2].total_bytes + directory[3].total_bytes'
    doc: exact loader cursor for the flat tag-4 controller
  flat_controllers_4_24:
    pos: section4_physical_offset
    type: flat_controller_storage_4_24
    size: 'directory[4].total_bytes + directory[5].total_bytes + directory[6].total_bytes + directory[7].total_bytes + directory[8].total_bytes + directory[9].total_bytes + directory[10].total_bytes + directory[11].total_bytes + directory[12].total_bytes + directory[13].total_bytes + directory[14].total_bytes + directory[15].total_bytes + directory[16].total_bytes + directory[17].total_bytes + directory[18].total_bytes + directory[19].total_bytes + directory[20].total_bytes + directory[21].total_bytes + directory[22].total_bytes + directory[23].total_bytes + directory[24].total_bytes'
    doc: exact physical flat-controller partition for tags 4 through 24
  section4_offset:
    value: 'section4_physical_offset - 44'
    doc: logical fixed-record view; these controllers serialize their first 44 bytes at the physical ring tail
  padstack_definitions:
    pos: 'section4_physical_offset - (version == 0x2022 ? 20 : version <= 0x2021 ? 24 : 28)'
    type: sec4_padstack_array
    size: directory[4].total_bytes
    doc: versioned logical padstack grid; physical marker is +24 through v0x2021 and +28 thereafter
  pad_layer_controller_header:
    pos: 'section4_physical_offset - (version == 0x2022 ? 20 : version <= 0x2021 ? 24 : 28) + directory[4].total_bytes'
    type: saved_pad_layer_controller_header
    doc: saved section-5 controller state between the rotated padstack grid and the first layer row
  pad_layer_shapes:
    pos: 'section4_physical_offset + directory[4].total_bytes + (version == 0x2022 ? 44 : -4)'
    type: sec5_pad_layer_array
    size: directory[5].total_bytes
    doc: per-padstack layer rows after the versioned serialized controller lead-in
  text_objects:
    pos: 'section4_physical_offset + directory[4].total_bytes + directory[5].total_bytes + directory[6].total_bytes + directory[7].total_bytes - (version == 0x2017 ? 28 : 36)'
    type: sec8_text_ring
    size: 'directory[8].total_bytes + (version == 0x2017 ? 28 : 36)'
    if: directory[8].count > 0
    doc: direct circular text-record view; metadata in record K+1 owns geometry in record K
  text_string_pool:
    pos: 'section4_physical_offset + directory[4].total_bytes + directory[5].total_bytes + directory[6].total_bytes + directory[7].total_bytes + directory[8].total_bytes'
    type: sec9
    size: directory[9].total_bytes
    if: directory[9].total_bytes > 0
    doc: direct section-9 indexed string-pool allocation
  drawing_objects:
    pos: section10_physical_offset
    type: sec10_drawing_physical
    size: directory[10].total_bytes
  graphic_piece_headers:
    pos: section11_physical_offset
    type: sec11_piece_physical
    size: directory[11].total_bytes
  graphic_vertices:
    pos: section12_physical_offset
    type: sec12_vertex_array
    size: directory[12].total_bytes
  section10_physical_offset:
    value: 'section4_physical_offset + directory[4].total_bytes + directory[5].total_bytes + directory[6].total_bytes + directory[7].total_bytes + directory[8].total_bytes + directory[9].total_bytes'
    doc: direct loader cursor for the circular section-10 drawing-owner controller
  section11_physical_offset:
    value: 'section10_physical_offset + directory[10].total_bytes'
    doc: direct loader cursor for the circular section-11 graphic-piece controller
  section12_physical_offset:
    value: 'section11_physical_offset + directory[11].total_bytes'
    doc: direct loader cursor for the fixed section-12 vertex array
  section13_physical_offset:
    value: 'section12_physical_offset + directory[12].total_bytes'
    doc: direct loader cursor for the flat section-13 graphic-parameter array
  section13_offset:
    value: 'section4_offset + directory[4].total_bytes + directory[5].total_bytes + directory[6].total_bytes + directory[7].total_bytes + directory[8].total_bytes + directory[9].total_bytes + directory[10].total_bytes + directory[11].total_bytes + directory[12].total_bytes'
  hatch_segments:
    pos: section13_physical_offset
    type: sec13_hatch_array
    size: directory[13].total_bytes
    if: directory[13].count > 0
    doc: complete flat 20-byte graphic-parameter array used by arcs and copper-pour hatch segments
  section14_offset:
    value: 'section13_offset + directory[13].total_bytes'
  decal_terminal_descriptors:
    pos: section14_offset + 44
    type: sec14_terminal_descriptor_array
    size: directory[14].total_bytes
    doc: |
      Complete physical PARTDECAL terminal-run descriptor array. The nominal
      loader boundary is 44 bytes before record zero; each descriptor starts at
      its decal name and directly carries the terminal and padstack cursors used
      by the reader.
  section15_offset:
    value: 'section14_offset + directory[14].total_bytes'
  section15_logical_offset:
    value: 'section15_offset + (version <= 0x2019 ? 60 : 44)'
    doc: legacy records begin 16 bytes into the physical controller ring; modern records begin at the physical controller start
  legacy_terminal_controller_prefix:
    pos: 'section15_offset + 44'
    type: saved_terminal_controller_prefix
    if: version <= 0x2019
    doc: legacy saved tag-15 state before the terminal-slot ring
  decal_terminal_slots:
    pos: section15_logical_offset
    type: decal_terminal_slot_array
    size: directory[15].total_bytes
    doc: section 15 terminal records followed by mixed decal controller and object-dictionary storage units
  section16_offset:
    value: 'section15_offset + directory[15].total_bytes'
  decal_padstack_pairs:
    pos: section16_offset + 44
    type: decal_padstack_pair_array
    size: directory[16].total_bytes
    doc: |
      Per-terminal padstack mappings begin at the physical section-16 boundary,
      section16_offset+44 in the rotated logical view. The pair cursor is the
      owning descriptor's +88 word. Its count is in the following descriptor at
      +32 for 100-byte descriptors and +20 for 112-byte descriptors.
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
  legacy_sigpin_final_record_tail:
    pos: 'section20_offset + 44'
    type: legacy_sigpin_final_record_tail
    if: 'version <= 0x2019 and directory[20].count > 0'
    doc: final four bytes of the rotated legacy SIGPIN ring's last record
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
    doc: nominal circular-array boundary, 44 bytes before the physical tag-23 cursor
  legacy_net_controller_prefix:
    pos: section23_offset
    type: saved_net_controller_prefix
    if: version <= 0x2022
    doc: saved tag-23 controller prefix before the legacy net-record array
  nets:
    pos: 'version <= 0x2022 ? section23_offset + 44 : section23_offset'
    type: net_record_array
    size: directory[23].total_bytes
    if: directory[23].count > 0
    doc: |
      Named nets followed by the unassigned-obstacles sentinel where present.
      Through v0x2022 the 144-byte logical record view begins 20 bytes after the
      physical section-23 boundary and wraps within that controller: its first
      20 physical bytes are the final record's tail. The directory count is
      exact; bytes in section 24 are not extra net records. Modern records use
      the nominal boundary directly because their logical view begins 44 bytes
      before the physical tag-23 cursor.
  section24_offset:
    value: 'section23_offset + directory[23].total_bytes'
  section24_controller_prefix:
    pos: 'section24_offset + (version <= 0x2022 ? 44 : 0)'
    type: saved_connection_controller_prefix
    doc: saved section-24 controller state before the rotated connection-record ring
  empty_route_chain_controller_state:
    pos: 'section24_offset + 8'
    type: empty_route_chain_controller_state
    if: 'version >= 0x2024 and directory[24].count == 0'
    doc: retained allocator/list state occupying the terminal-head slot of an empty modern connection controller
  route_chains:
    pos: 'version <= 0x2022 ? section24_offset + 60 : section24_offset + 8'
    type: 'route_chain_array(directory[24].count, version >= 0x2024 ? 1 : 0)'
    size: 'directory[24].total_bytes + (version >= 0x2024 ? 36 : 0)'
    if: directory[24].count > 0
    doc: |
      Logical 68-byte topology array. Through v0x2022 record zero starts 16
      bytes after the physical section-24 boundary; the physical prefix is the
      final record's 16-byte tail. Modern record zero starts 36 bytes before
      that boundary. The directory count is the edge count: count full records
      are followed by the final edge's 36-byte topology head, ending exactly at
      the physical controller boundary. Both positions follow directly from
      the controller's fixed circular-array rotation.
  section25_offset:
    value: 'loader_stream_start + directory[2].total_bytes + directory[3].total_bytes + directory[4].total_bytes + directory[5].total_bytes + directory[6].total_bytes + directory[7].total_bytes + directory[8].total_bytes + directory[9].total_bytes + directory[10].total_bytes + directory[11].total_bytes + directory[12].total_bytes + directory[13].total_bytes + directory[14].total_bytes + directory[15].total_bytes + directory[16].total_bytes + directory[17].total_bytes + directory[18].total_bytes + directory[19].total_bytes + directory[20].total_bytes + directory[21].total_bytes + directory[22].total_bytes + directory[23].total_bytes + directory[24].total_bytes'
  route_allocator_controller:
    pos: section25_offset
    type: route_allocator_controller
    size: directory[25].total_bytes
    doc: global route-controller state, including the four section-26 allocator page-group counts
  section26_offset:
    value: 'section25_offset + directory[25].total_bytes'
  route_object_ranges:
    pos: section26_offset
    type: route_object_range_array
    size: directory[26].total_bytes
    doc: |
      Section-26 descriptor directory. Its prefix holds four variable-size
      NumOrdMdl descriptor groups. Its suffix is partitioned, without inspecting
      contents, by the page counts in directory controllers 65, 66, 45, 46, 47,
      48, 41, 42, and (when present) 74.
  section61_allocator_descriptor_offset:
    value: 'section26_offset + (route_allocator_controller.allocator20_page_count + route_allocator_controller.allocator48_page_count + route_allocator_controller.allocator88_page_count) * 12'
    doc: direct start of the fourth section-26 prefix group; its runtime object stride is 56 bytes
  section61_allocator_page_descriptors:
    pos: section61_allocator_descriptor_offset
    type: 'page_descriptor_group(route_allocator_controller.allocator56_page_count)'
    if: route_allocator_controller.allocator56_page_count > 0
    doc: |
      Page descriptors for section-61 nodes. For every page except the last,
      the following descriptor's record_count is the current page's serialized
      count; the final page owns the remaining directory[61].count records.
  section27_offset:
    value: 'section26_offset + directory[26].total_bytes'
  route_layer_object_counts:
    pos: section27_offset
    type: route_layer_object_count_array
    size: directory[27].total_bytes
    doc: per-copper-layer route-object counts; their sum is the section-29 handle count
  section29_offset:
    value: 'section27_offset + directory[27].total_bytes + directory[28].total_bytes'
  route_object_handles:
    pos: section29_offset
    type: route_object_handle_array
    size: directory[29].total_bytes
    doc: route-object handles grouped by the preceding per-layer counts
  section41_offset:
    value: 'section29_offset + directory[29].total_bytes'
    doc: |
      Physical start of paged controller 41. Its record count is the sum of the
      `record_count` fields in controller 41's section-26 page descriptors, and
      its stride is 180 bytes in v0x2017 and 188 bytes thereafter. PADS then reads
      paged controllers in order 41, 42, 45, 46, 47, 48. No controller header,
      candidate layout, or content discriminator occurs between these arrays.
  page_descriptor_suffix_offset:
    value: 'section26_offset + directory[26].total_bytes - (directory[65].total_bytes + directory[66].total_bytes + directory[45].total_bytes + directory[46].total_bytes + directory[47].total_bytes + directory[48].total_bytes + directory[41].total_bytes + directory[42].total_bytes + (num_directory > 74 ? directory[74].total_bytes : 0)) * 12'
    doc: |
      The section-26 descriptor suffix is partitioned in the loader's exact
      controller order: 65, 66, 45, 46, 47, 48, 41, 42, and optional 74.
      Each directory total_bytes is a descriptor count here, not a byte extent.
  section65_page_descriptors:
    pos: page_descriptor_suffix_offset
    type: 'page_descriptor_group(directory[65].total_bytes)'
    if: directory[65].total_bytes > 0
  section66_page_descriptors:
    pos: 'page_descriptor_suffix_offset + directory[65].total_bytes * 12'
    type: 'page_descriptor_group(directory[66].total_bytes)'
    if: directory[66].total_bytes > 0
  section45_page_descriptors:
    pos: 'page_descriptor_suffix_offset + (directory[65].total_bytes + directory[66].total_bytes) * 12'
    type: 'page_descriptor_group(directory[45].total_bytes)'
    if: directory[45].total_bytes > 0
  section46_page_descriptors:
    pos: 'page_descriptor_suffix_offset + (directory[65].total_bytes + directory[66].total_bytes + directory[45].total_bytes) * 12'
    type: 'page_descriptor_group(directory[46].total_bytes)'
    if: directory[46].total_bytes > 0
  section47_page_descriptors:
    pos: 'page_descriptor_suffix_offset + (directory[65].total_bytes + directory[66].total_bytes + directory[45].total_bytes + directory[46].total_bytes) * 12'
    type: 'page_descriptor_group(directory[47].total_bytes)'
    if: directory[47].total_bytes > 0
  section48_page_descriptors:
    pos: 'page_descriptor_suffix_offset + (directory[65].total_bytes + directory[66].total_bytes + directory[45].total_bytes + directory[46].total_bytes + directory[47].total_bytes) * 12'
    type: 'page_descriptor_group(directory[48].total_bytes)'
    if: directory[48].total_bytes > 0
  section41_page_descriptors:
    pos: 'page_descriptor_suffix_offset + (directory[65].total_bytes + directory[66].total_bytes + directory[45].total_bytes + directory[46].total_bytes + directory[47].total_bytes + directory[48].total_bytes) * 12'
    type: 'page_descriptor_group(directory[41].total_bytes)'
    if: directory[41].total_bytes > 0
  section42_page_descriptors:
    pos: 'page_descriptor_suffix_offset + (directory[65].total_bytes + directory[66].total_bytes + directory[45].total_bytes + directory[46].total_bytes + directory[47].total_bytes + directory[48].total_bytes + directory[41].total_bytes) * 12'
    type: 'page_descriptor_group(directory[42].total_bytes)'
    if: directory[42].total_bytes > 0
  section74_page_descriptors:
    pos: 'page_descriptor_suffix_offset + (directory[65].total_bytes + directory[66].total_bytes + directory[45].total_bytes + directory[46].total_bytes + directory[47].total_bytes + directory[48].total_bytes + directory[41].total_bytes + directory[42].total_bytes) * 12'
    type: 'page_descriptor_group(directory[74].total_bytes)'
    if: num_directory > 74 and directory[74].total_bytes > 0
  section41_record_count:
    value: 'directory[41].total_bytes > 0 ? section41_page_descriptors.total_records : 0'
  section42_record_count:
    value: 'directory[42].total_bytes > 0 ? section42_page_descriptors.total_records : 0'
  section45_record_count:
    value: 'directory[45].total_bytes > 0 ? section45_page_descriptors.total_records : 0'
  section46_record_count:
    value: 'directory[46].total_bytes > 0 ? section46_page_descriptors.total_records : 0'
  section47_record_count:
    value: 'directory[47].total_bytes > 0 ? section47_page_descriptors.total_records : 0'
  section48_record_count:
    value: 'directory[48].total_bytes > 0 ? section48_page_descriptors.total_records : 0'
  section65_record_count:
    value: 'directory[65].total_bytes > 0 ? section65_page_descriptors.total_records : 0'
  section66_record_count:
    value: 'directory[66].total_bytes > 0 ? section66_page_descriptors.total_records : 0'
  section74_record_count:
    value: 'num_directory > 74 and directory[74].total_bytes > 0 ? section74_page_descriptors.total_records : 0'
    doc: |
      Serialized 276-byte object count from the section-26 page descriptors.
      Six files contain 54 records. Directory[74].count is zero because this
      paged controller's flat count field is not its saved page occupancy.
  section41_pages:
    pos: section41_offset
    type: section41_paged_controller
  section42_offset:
    value: 'section41_offset + section41_record_count * (version == 0x2017 ? 180 : 188)'
  section42_pages:
    pos: section42_offset
    type: section42_paged_controller
  section45_offset:
    value: 'section42_offset + section42_record_count * 80'
  section45_pages:
    pos: section45_offset
    type: section45_paged_controller
  section46_offset:
    value: 'section45_offset + section45_record_count * (version == 0x2017 ? 116 : 124)'
  section46_pages:
    pos: section46_offset
    type: section46_paged_controller
  section46_live_record_count:
    value: section46_pages.num_live
    doc: |
      Live tag-46 heap records after PADS converts the saved heap. A source slot
      is free when bit 31 of its first word is set; conversion also discards a
      slot with a null saved via-type-set handle. Only retained destination
      slots receive a following tag-51 state word.
  section47_offset:
    value: 'section46_offset + section46_record_count * (version <= 0x2019 ? 32 : 40)'
  section47_pages:
    pos: section47_offset
    type: section47_paged_controller
  section48_offset:
    value: 'section47_offset + section47_record_count * 24'
  section48_pages:
    pos: section48_offset
    type: section48_paged_controller
  section48_diff_pair_records:
    pos: 'section48_offset - 8'
    type: section48_diff_pair_record_array
    if: version >= 0x2024 and section48_record_count > 0
    doc: logical 864-byte section-48 slots; the physical controller ring begins eight bytes into record zero
  section49_physical_offset:
    value: 'section48_offset + section48_record_count * (version <= 0x2019 ? 48 : (version <= 0x2022 ? 856 : 864))'
    doc: exact start after the six paged controllers; no scan or candidate selection
  section49_relationships:
    pos: section49_physical_offset
    type: section49_relationship_stream
    size: directory[49].total_bytes
    doc: |
      Two counted relationship arrays per live signal. Records repeat to the
      controller's declared byte end. This yields directory[23].count - 1
      records on 571 corpus binaries and directory[23].count records on 26.
      In the full-count dialect, relationship record ordinal is the section-23
      record ordinal and the unassigned-obstacles slot is retained. In the
      count-minus-one dialect, records follow named live-net order and omit it.
  section46_route_rule_states:
    pos: 'section49_physical_offset + directory[49].total_bytes'
    type: 'section46_route_rule_state_array(section46_live_record_count)'
    doc: |
      FUN_00830840 selects tag 51 and reads one four-byte state for every
      record loaded from paged controller 46. This count is serialized in the
      tag-46 page descriptors; no content marker or downstream offset is used.
  section51_physical_offset:
    value: 'section49_physical_offset + directory[49].total_bytes + section46_live_record_count * 4'
  section51_relationships:
    pos: section51_physical_offset
    type: section51_relationship_array
    size: directory[51].total_bytes
  section50_physical_offset:
    value: 'section51_physical_offset + directory[51].total_bytes'
  section50_relationships:
    pos: section50_physical_offset
    type: section50_relationship_array
    size: directory[50].total_bytes
  section52_physical_offset:
    value: 'section50_physical_offset + directory[50].total_bytes'
  section52_outline_owners:
    pos: section52_physical_offset
    type: sec52_outline_owner_array
    size: directory[52].total_bytes
  section53_outline_pieces:
    pos: 'section52_physical_offset + directory[52].total_bytes'
    type: sec53_outline_piece_array
    size: directory[53].total_bytes
  section54_outline_vertices:
    pos: 'section52_physical_offset + directory[52].total_bytes + directory[53].total_bytes'
    type: sec54_outline_vertex_array
    size: directory[54].total_bytes
  section55_outline_arcs:
    pos: 'section52_physical_offset + directory[52].total_bytes + directory[53].total_bytes + directory[54].total_bytes'
    type: sec55_outline_arc_array
    size: directory[55].total_bytes
  section56_physical_offset:
    value: 'section52_physical_offset + directory[52].total_bytes + directory[53].total_bytes + directory[54].total_bytes + directory[55].total_bytes'
  string_index:
    pos: section56_physical_offset
    type: string_index_array
    size: directory[56].total_bytes
  string_pool_offset:
    value: 'section56_physical_offset + directory[56].total_bytes'
    doc: exact section-57 start in loader order; no string-content search
  string_pool:
    pos: string_pool_offset
    type: string_pool_contents
    size: directory[57].total_bytes
  section58_physical_offset:
    value: 'string_pool_offset + directory[57].total_bytes'
  section59_physical_offset:
    value: 'section58_physical_offset + directory[58].total_bytes'
  section59_route_headers:
    pos: section59_physical_offset
    type: 'sec59_route_header_ring(directory[59].count, directory[59].count > 0 ? directory[59].total_bytes / directory[59].count : 0, version == 0x2024 ? 12 : version >= 0x2025 ? 4 : 16)'
    size: directory[59].total_bytes
    if: directory[59].count > 0
    doc: direct circular route-header view; no marker scan or phase selection
  sections59_64:
    pos: section59_physical_offset
    type: 'sections59_64_stream(directory[59].count, directory[59].count > 0 ? directory[59].total_bytes / directory[59].count : 0, directory[60].count, directory[60].count > 0 ? directory[60].total_bytes / directory[60].count : 0, directory[61].count, directory[62].count, directory[62].count > 0 ? directory[62].total_bytes / directory[62].count : 0, directory[63].count, directory[64].count)'
    size: 'directory[59].total_bytes + directory[60].total_bytes + directory[61].total_bytes + directory[62].total_bytes + directory[63].total_bytes + directory[64].total_bytes'
  section65_66_physical_offset:
    value: 'section59_physical_offset + directory[59].total_bytes + directory[60].total_bytes + directory[61].total_bytes + directory[62].total_bytes + directory[63].total_bytes + directory[64].total_bytes'
  section65_pages:
    pos: section65_66_physical_offset
    type: section65_saved_group_record
    repeat: expr
    repeat-expr: section65_record_count
    doc: saved GROUP objects counted by tag-65 page descriptors
  section66_physical_offset:
    value: 'section65_66_physical_offset + section65_record_count * 28'
  section66_pages:
    pos: section66_physical_offset
    type:
      switch-on: version <= 0x2022
      cases:
        true: section65_66_compact_net_class
        false: section65_66_net_class
    repeat: expr
    repeat-expr: section66_record_count
    doc: section-66 paged net-class records
  section67_physical_offset:
    value: 'section66_physical_offset + section66_record_count * (version <= 0x2022 ? 28 : 280)'
  section67_relationships:
    pos: section67_physical_offset
    type: sec67_design_rule_relationship_array
    size: directory[67].total_bytes
  section68_clusters:
    pos: 'section67_physical_offset + directory[67].total_bytes'
    type: cluster_record_array
    size: directory[68].total_bytes
  section69_physical_offset:
    value: 'section67_physical_offset + directory[67].total_bytes + directory[68].total_bytes'
  section69_physical_storage:
    pos: section69_physical_offset
    size: '12 + directory[69].total_bytes'
    doc: exact loader-owned bytes for the 12-byte lead-in and flat controller 69 records
  section69_controller_leadin_state:
    pos: section69_physical_offset
    type: section69_controller_leadin
  section69_layers:
    pos: 'section69_physical_offset + 12'
    type: 'sec69_layer_record_array(version <= 0x2021 ? 6 : version == 0x2022 ? 8 : 12, directory[69].count)'
    doc: logical layer records after the fixed controller lead-in
  section70_physical_offset:
    value: 'section69_physical_offset + 12 + directory[69].total_bytes'
  section70_state:
    pos: section70_physical_offset
    type: section70_serialized_layer_state
    size: 4
  section71_physical_offset:
    value: 'section70_physical_offset + 4'
  section71_preferences:
    pos: section71_physical_offset
    type: 'global_display_preferences(directory[71].total_bytes - 4)'
    size: 'directory[71].total_bytes - 4'
  section72_physical_offset:
    value: 'section70_physical_offset + directory[71].total_bytes'
  section72_error_conflicts:
    pos: section72_physical_offset
    type: saved_error_conflict_record
    repeat: expr
    repeat-expr: directory[72].count
  section73_physical_offset:
    value: 'section72_physical_offset + directory[72].total_bytes'
  section73_font_faces:
    pos: section73_physical_offset
    type:
      switch-on: 'num_directory <= 73 or directory[73].count == 0 ? 0 : directory[73].total_bytes / directory[73].count'
      cases:
        40: saved_font_face_record_v40
        52: saved_font_face_record
    repeat: expr
    repeat-expr: 'num_directory > 73 ? directory[73].count : 0'
    if: num_directory > 73
  section74_physical_offset:
    value: 'section73_physical_offset + (num_directory > 73 ? directory[73].total_bytes : 0)'
  section74_pages:
    pos: section74_physical_offset
    type: extended_layer_state_record
    repeat: expr
    repeat-expr: section74_record_count
    if: num_directory > 74
  post_layer_database:
    pos: 'section74_physical_offset + section74_record_count * 276'
    type: post_layer_database_stream
    size: 'footer.cntr_item_back_ptr - (section74_physical_offset + section74_record_count * 276)'

types:
  physical_file_body:
    doc: |
      Physical loader order from tag 2 through the embedded post-layer
      database. Every extent comes from a serialized directory value, page
      descriptor count, live-slot predicate, or the footer back-pointer. This
      nonoverlapping ownership view is deliberately raw where logical circular
      records cross controller boundaries; the root's overlapping typed
      instances assign those same bytes their field-level meanings. These
      storage leaves are not unparsed/unknown gaps.
    seq:
      - id: flat_controllers_2_27
        size: '_root.directory[2].total_bytes + _root.directory[3].total_bytes + _root.directory[4].total_bytes + _root.directory[5].total_bytes + _root.directory[6].total_bytes + _root.directory[7].total_bytes + _root.directory[8].total_bytes + _root.directory[9].total_bytes + _root.directory[10].total_bytes + _root.directory[11].total_bytes + _root.directory[12].total_bytes + _root.directory[13].total_bytes + _root.directory[14].total_bytes + _root.directory[15].total_bytes + _root.directory[16].total_bytes + _root.directory[17].total_bytes + _root.directory[18].total_bytes + _root.directory[19].total_bytes + _root.directory[20].total_bytes + _root.directory[21].total_bytes + _root.directory[22].total_bytes + _root.directory[23].total_bytes + _root.directory[24].total_bytes + _root.directory[25].total_bytes + _root.directory[26].total_bytes + _root.directory[27].total_bytes'
        doc: exact physical storage for flat controllers 2 through 27
      - id: flat_controller_28
        size: _root.directory[28].total_bytes
        doc: tag-28 controller; zero-length on every corpus file
      - id: flat_controller_29
        size: _root.directory[29].total_bytes
        doc: route-object handles grouped by tag-27 layer counts
      - id: paged_controller_41
        size: '_root.section41_record_count * (_root.version == 0x2017 ? 180 : 188)'
        doc: clearance-rule records counted by tag-41 page descriptors
      - id: paged_controller_42
        size: '_root.section42_record_count * 80'
        doc: high-speed-rule records counted by tag-42 page descriptors
      - id: paged_controller_45
        size: '_root.section45_record_count * (_root.version == 0x2017 ? 116 : 124)'
        doc: per-layer rule records counted by tag-45 page descriptors
      - id: paged_controller_46
        size: '_root.section46_record_count * (_root.version <= 0x2019 ? 32 : 40)'
        doc: route-rule heap slots counted by tag-46 page descriptors
      - id: paged_controller_47
        size: '_root.section47_record_count * 24'
        doc: route-rule relationship records counted by tag-47 page descriptors
      - id: paged_controller_48
        size: '_root.section48_record_count * (_root.version <= 0x2019 ? 48 : (_root.version <= 0x2022 ? 856 : 864))'
        doc: differential-pair slots counted by tag-48 page descriptors
      - id: flat_controller_49
        size: _root.directory[49].total_bytes
        doc: two counted relationship arrays for each serialized signal
      - id: section46_route_rule_states
        size: '_root.section46_live_record_count * 4'
        doc: one saved tag-51 state word per live tag-46 route-rule slot
      - id: flat_controller_51
        size: _root.directory[51].total_bytes
        doc: tag-51 relationship controller storage
      - id: flat_controller_50
        size: _root.directory[50].total_bytes
        doc: tag-50 relationship controller storage
      - id: flat_controllers_52_64
        size: '_root.directory[52].total_bytes + _root.directory[53].total_bytes + _root.directory[54].total_bytes + _root.directory[55].total_bytes + _root.directory[56].total_bytes + _root.directory[57].total_bytes + _root.directory[58].total_bytes + _root.directory[59].total_bytes + _root.directory[60].total_bytes + _root.directory[61].total_bytes + _root.directory[62].total_bytes + _root.directory[63].total_bytes + _root.directory[64].total_bytes'
        doc: pours, string storage, and route/object arrays in loader order
      - id: paged_controller_65
        size: '_root.section65_record_count * 28'
        doc: saved GROUP records counted by tag-65 page descriptors
      - id: paged_controller_66
        size: '_root.section66_record_count * (_root.version <= 0x2022 ? 28 : 280)'
        doc: saved net-class records counted by tag-66 page descriptors
      - id: flat_controller_67
        size: _root.directory[67].total_bytes
        doc: design-rule relationship records
      - id: flat_controller_68
        size: _root.directory[68].total_bytes
        doc: named part-cluster records
      - id: flat_controller_69
        size: '12 + _root.directory[69].total_bytes'
        doc: 12-byte layer-controller lead-in followed by physical layer records
      - id: serialized_controllers_70_71
        size: _root.directory[71].total_bytes
        doc: one four-byte tag-70 state followed by the versioned tag-71 preference object
      - id: flat_controller_72
        size: _root.directory[72].total_bytes
        doc: saved error-conflict records
      - id: flat_controller_73
        size: '_root.num_directory > 73 ? _root.directory[73].total_bytes : 0'
        doc: saved font-face records
      - id: paged_controller_74
        size: '_root.num_directory > 74 ? _root.section74_record_count * 276 : 0'
        doc: extended-layer records counted by tag-74 section-26 page descriptors
      - id: post_layer_database
        type: post_layer_database_stream
        size: '_io.size - _io.pos'
        doc: nested database/controller stream ending at the footer back-pointer


  # =========================================================================
  # CONTAINER
  # =========================================================================
  post_layer_database_stream:
    doc: |
      Trailing PowerSYS database, six Reuse controllers, eleven Attribute
      controllers, two direct geometry lists, and the Strings header/data
      allocators. Sections 70 through 74 are parsed before this type in their
      actual loader order. Controller order, IDs, and fixed-object strides come
      directly from the sdb500 reader.
    seq:
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
    instances:
      consumed_all:
        value: _io.eof
        doc: true only when the structured trailing database reaches the container-item back-pointer exactly

  global_display_preferences:
    params:
      - id: base_size
        type: u4
    seq:
      - id: legacy_preference_words
        type: u4
        repeat: expr
        repeat-expr: 23
        if: base_size == 92
        doc: |
          Complete 92-byte v0x2017 global-preference object copied by the flat
          reader; 23 retained display, viewport, layer-selection, and editor-state words
      - id: modern_preferences
        type: 'modern_global_display_preferences(base_size)'
        if: base_size > 96
    instances:
      consumed_all:
        value: _io.eof
        doc: true only when the versioned preference structure consumes all section-71 bytes

  modern_global_display_preferences:
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
      - id: display_configuration_state
        type: u4
        doc: saved display-configuration selection/state word; not an array count
      - id: display_configurations
        type: 'global_display_configuration((base_size - 104) / 10)'
        repeat: expr
        repeat-expr: 10
      - id: default_font_face_handle
        type: u4
        doc: tagged ODBFontFace handle; low byte is the saved font ordinal (0..7), or zero when unset

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
        size: 84
        doc: NUL-padded ASCII for live named slots; unused capacity retains arbitrary bytes
      - id: viewport_rectangle
        type: rect_i32
      - id: configuration_flags
        type: u4
        if: record_stride == 104

  extended_layer_state_record:
    doc: |
      Optional 276-byte ODBLayer extension serialized after section 73. Six
      scoped files contain 54 records. The earlier placement before section 70
      was a circular cancellation: the same downstream PowerSYS gap supplied
      the record count, so moving these bytes upstream still reached EOF. Their
      tail holds saved layer handles plus fixed layer-name and retained state.
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
        doc: retained ODBLayer object capacity; zero in all 54 scoped records, not alignment padding

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
      - id: power_sys_archive_tag
        type: u4
        doc: saved MFC archive tag preceding the newly named PowerSYS class; never a file offset
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
      exactly at the container-item back-pointer on every one of the 597 unique
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
      field layout are exact for every page in the scoped corpus. MiscGeom pages
      are raw word-addressed allocator storage rather than fixed objects. Slots
      released before save retain allocator free-list links in their first
      field; that retained content is state, not padding.
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
      One 32-bit word from DBC_MiscGeomCtl's saved ObjPage payload. These pages
      hold variable-size DBD_MiscGeomData side objects, including arc/graphic
      state and allocator free-list links, so the page is word-addressed rather
      than divided into a fixed record stride. All 296 pages in the scoped
      corpus tile exactly into 885,388 words.
    seq:
      - id: value_or_free_link
        type: u4

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
    params:
      - id: has_trailing_slot_word
        type: bool
    doc: |
      Four-byte-shifted controller-slot view: three live u32 fields followed by
      the next slot's zero leading word. The final view has no trailing word.

      The field named `total_bytes` by the importer is controller-dependent. For
      flat controllers it is the exact CArchive byte count. For paged controllers
      41, 42, 45, 46, 47, 48, 65, 66, and 74 it is the number of section-26 page
      descriptors. The page descriptors, not directory arithmetic or decoded
      content, give those controllers' physical byte extents.
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
        if: has_trailing_slot_word
        doc: |
          Zero high word/padding for the 32-bit in-memory base pointer. Zero in
          every directory entry on all 597 unique corpus files.

  # =========================================================================
  # SECTION 56/57 -- the attribute string index and its pool
  # =========================================================================
  string_index_entry:
    doc: |
      A 16-byte section-56 index record pointing into section 57's string pool.
      Section 56 starts after section 55 in loader order and has the exact byte
      extent declared by directory[56]. Section 57 starts immediately afterward
      and has the exact byte extent declared by directory[57].

      The layout is proved by an exact tiling invariant across the whole table:

          pool_offset[k] + length[k] == pool_offset[k+1]

      e.g. 30274+35 = 30309, +25 = 30334, +28 = 30362. The table need not
      include the pool's first string: on many files its first serialized entry
      starts at pool offset 25, immediately after
      `DFT_CONFIGURATION\0PARENT\0`.

      The tiling describes record semantics only. It does not locate or validate
      either section boundary. Some files have unindexed retained bytes at the
      beginning or end of section 57, and attribute values may contain binary
      controller bytes rather than printable text.
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
      Serialized OLE container items. The array count is zero on 594 of 597
      unique corpus binaries; three contain one populated item. Each item is an
      MFC COleClientItem carrying a length-delimited Microsoft Compound File
      Binary document and PADS view state.
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
    instances:
      consumed_all:
        value: _io.eof
        doc: true only when the container-item stream reaches the fixed footer exactly

  powerpcb_cntr_item:
    doc: |
      CPowerPCBCntrItem::Serialize payload. The first five fields come from
      MFC COleClientItem::Serialize (mfc140 ordinal 13091); the remaining view
      fields come from the PADS override at PowerUI500.dll 0x10519300.

      Three scoped corpus binaries contain one item. Field meanings and framing
      also agree with the MFC and PADS serializer implementations.
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
          that offset is a u32 item count. It is zero on 594 corpus files; three
          files store one embedded Microsoft Compound File item. This is not a
          size or checksum.

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
        doc: zero high word of the 32-bit arena pointer on all 597 unique corpus files

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
  # Measured over all 597 scoped boards: directory[2].count equals the observed
  # prefix length in 48-byte units on 597/597. 539 boards have no prefix; the
  # rest run 1..136 records and every one matches its declared count exactly.
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
        doc: off0 legacy auxiliary-controller state/count word; interpretation varies with the section-1 dialect
      - id: legacy_aux_record_bytes
        type: u4
        doc: |
          off4 legacy auxiliary-controller allocation bytes. In old dialects
          with a live count this is exactly count * 52; modern files may retain
          a nonzero allocation value with count zero.
      - id: legacy_aux_memory_base
        type: u4
        doc: off8 serialized 32-bit allocator base; zero on 595 scoped files and a retained process address on two
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
        doc: off120 alignment/state word; zero on all 597 unique corpus binaries
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
  # exact on the corpus: total_bytes == count * 48 for all 597 unique files.
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
  # SECTION 3 — complete physical database/board-parameter controller image
  # =========================================================================
  # The loader reads directory[3].total_bytes exactly. Logical board-parameter
  # views overlap retained directory/database-header state; physical framing does
  # not subtract that overlap. Default-valued state is serialized state, not padding.
  sec3_physical_controller:
    doc: |
      Complete flat tag-3 controller image. Its prefix is the physical overlap
      with section 1: section-1 logical bytes 12..end. The overlap length is
      therefore directory[1].total_bytes - 12 on every version, derived solely
      from the serialized section-1 extent. The versioned board-parameter image
      follows immediately and consumes the remainder of directory[3].total_bytes.
    seq:
      - id: board_setup_overlap_tail
        size: _root.directory[1].total_bytes - 12
        doc: section-1 logical bytes 12..end, physically shared by the circular controller views
      - id: parameters
        type: sec3_board_params
        size-eos: true

  sec3_board_params:
    doc: version-selected section 3 board-parameter serialization
    seq:
      - id: common
        type: sec3_board_params_modern
        size-eos: true

  flat_controller_storage_4_24:
    doc: |
      Physical loader-order storage for flat tags 4..24. Several logical
      fixed-record arrays are rotated left by 44 bytes across adjacent tag
      boundaries; the root's typed logical views expose their fields while this
      stream records the exact non-overlapping physical partition.
    seq:
      - id: padstack_storage
        size: _root.directory[4].total_bytes
      - id: pad_layer_storage
        size: _root.directory[5].total_bytes
      - id: text_controller_storage_6
        size: _root.directory[6].total_bytes
      - id: text_controller_storage_7
        size: _root.directory[7].total_bytes
      - id: text_object_storage
        size: _root.directory[8].total_bytes
      - id: text_drawing_bridge_storage
        size: _root.directory[9].total_bytes
      - id: drawing_owner_storage
        size: _root.directory[10].total_bytes
      - id: graphic_piece_storage
        size: _root.directory[11].total_bytes
      - id: graphic_vertex_storage
        size: _root.directory[12].total_bytes
      - id: hatch_storage
        size: _root.directory[13].total_bytes
      - id: decal_descriptor_storage
        size: _root.directory[14].total_bytes
      - id: decal_terminal_storage
        size: _root.directory[15].total_bytes
      - id: parttype_aux_storage
        size: _root.directory[16].total_bytes
      - id: parttype_storage
        size: _root.directory[17].total_bytes
      - id: parttype_gate_storage
        size: _root.directory[18].total_bytes
      - id: parttype_pin_storage
        size: _root.directory[19].total_bytes
      - id: parttype_signal_storage
        size: _root.directory[20].total_bytes
      - id: compact_pin_name_storage
        size: _root.directory[21].total_bytes
      - id: placement_storage
        size: _root.directory[22].total_bytes
      - id: net_storage
        size: _root.directory[23].total_bytes
      - id: route_chain_storage
        size: _root.directory[24].total_bytes

  sec3_board_params_modern:
    doc: |
      Common board-parameter field order. v0x2017/v0x2019 carry eight additional
      palette words, shifting the named fields by +32; v0x2021..v0x2024 carry
      four, shifting them by +16. v0x2019 ends after ARPTOMLAYER, v0x2017 after
      ARDTOPLAYER, and v0x2021 after VIAPFLAG. v0x2022/v0x2024 append FLOWFLAGS;
      v0x2025+ append five fixed auxiliary selector buffers. The two paired
      v0x2017 exports place twelve nondefault named parameters from HATCHGRID
      through STMINSPOKES at these shifted offsets; the serialized extents fix
      the v0x2017/v0x2019 tail truncations.
    seq:
      - id: display_palette_words
        type: u4
        repeat: expr
        repeat-expr: '_root.version < 0x2021 ? 216 : (_root.version < 0x2025 ? 212 : 208)'
        doc: |
          0..831 persisted 16-by-52-byte display/palette table. Sparse layer or
          display selectors are nonzero on 20 of the 590 v0x2021+ corpus files;
          the other 570 save the default all-zero state. Serialized state, not
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
          860..987 version-dependent board-parameter extension. Nonzero on 48
          of the 590 v0x2021+ corpus files: old layouts use the first four words
          for size/extent values, while v0x2025+ stores a sparse 17-word option
          vector. Zero denotes default state and is not alignment padding.
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
        if: _root.version != 0x2019
      - id: ardtom_layer
        type: s4
        doc: 1380 ARDTOMLAYER
        if: _root.version != 0x2019
      - id: ardtop
        type: s4
        doc: 1384 ARDTOP
        if: _root.version != 0x2019
      - id: ardtop_layer
        type: s4
        doc: 1388 ARDTOPLAYER
        if: _root.version != 0x2019
      - id: viap_spacing
        type: s4
        doc: 1392 VIAPSPACING
        if: _root.version >= 0x2021
      - id: viap_shape
        type: s4
        doc: 1396 VIAPSHAPE
        if: _root.version >= 0x2021
      - id: viap_to_trace
        type: s4
        doc: 1400 VIAPTOTRACE
        if: _root.version >= 0x2021
      - id: viap_fill
        type: s4
        doc: 1404 VIAPFILL
        if: _root.version >= 0x2021
      - id: viap_word_a
        type: u4
        doc: 1408 via-pattern packed word A (VIAPSHSIG name-handle)
        if: _root.version >= 0x2021
      - id: viap_word_b
        type: u4
        doc: 1412 via-pattern packed word B (high byte 0x0E const)
        if: _root.version >= 0x2021
      - id: viap_flag
        type: s4
        doc: 1416 VIAPFLAG
        if: _root.version >= 0x2021
      - id: flow_flags
        type: s4
        doc: 1420 FLOWFLAGS
        if: _root.version >= 0x2022
      - id: auxiliary_name_buffers
        type: fixed_path_storage
        repeat: expr
        repeat-expr: 4
        if: _root.version >= 0x2025
        doc: |
          1424..2463 first four fixed 260-byte auxiliary/CAM selector buffers.
      - id: final_auxiliary_name_buffer
        type: str
        size-eos: true
        encoding: ASCII
        terminator: 0
        if: _root.version >= 0x2025
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
        doc: '+52 cumulative start row into sec5 pad-shape table when layer_count > 0; retained cursor or 0xFFFFFFFF empty-list sentinel otherwise'
      - id: marker
        type: u1
        doc: '+56 0xFE = valid padstack record'
      - id: shape_code
        type: u1
        enum: pad_shape
        doc: '+57 shape'
      - id: layer_count
        type: u1
        doc: '+58 number of sec5 layer entries this padstack owns'
      - id: drill_start_layer
        type: u1
        doc: '+59 optional blind/buried drill start; zero means a through drill'
      - id: drill_end_layer
        type: u1
        doc: '+60 optional blind/buried drill end; zero means a through drill'
      - id: trailing_state
        size: 3

  sec4_padstack_v2022:
    seq:
      - id: object_state
        size: 20
      - id: pad_width
        type: s4
      - id: drill
        type: s4
      - id: fin_length
        type: s4
      - id: corner_radius
        type: s4
      - id: finger_state
        type: s4
      - id: angle_raw
        type: s4
      - id: sec5_index
        type: u4
        doc: cumulative start row into sec5 when layer_count > 0; retained cursor or empty-list sentinel otherwise
      - id: marker
        type: u1
      - id: shape_code
        type: u1
        enum: pad_shape
      - id: layer_count
        type: u1
      - id: drill_start_layer
        type: u1
      - id: drill_end_layer
        type: u1
      - id: trailing_state
        size: 3

  sec4_padstack_legacy:
    seq:
      - id: object_state
        size: 24
      - id: pad_width
        type: s4
      - id: drill
        type: s4
      - id: fin_length
        type: s4
      - id: corner_radius
        type: s4
      - id: angle_raw
        type: s4
      - id: sec5_index
        type: u4
        doc: cumulative start row into sec5 when layer_count > 0; retained cursor or empty-list sentinel otherwise
      - id: marker
        type: u1
      - id: shape_code
        type: u1
        enum: pad_shape
      - id: layer_count
        type: u1
      - id: trailing_state
        type: u1

  sec4_padstack_array:
    seq:
      - id: records
        type:
          switch-on: '_root.directory[4].total_bytes / _root.directory[4].count'
          cases:
            52: sec4_padstack_legacy
            56: sec4_padstack_v2022
            64: sec4_padstack
        repeat: eos

  saved_pad_layer_controller_header:
    doc: |
      Saved tag-5 controller header. Its serialized size is 20 bytes through
      v0x2021, 64 bytes in v0x2022, and 24 bytes thereafter. It occupies the
      exact bridge between the rotated tag-4 padstack grid and the first tag-5
      layer row; these words are retained controller/list state, not padding.
    seq:
      - id: legacy_zero_controller_state
        type: u4
        valid: 0
        if: _root.version <= 0x2021
      - id: legacy_saved_controller_flags
        type: u4
        if: _root.version <= 0x2021
        doc: zero or 2 in the scoped legacy dialect
      - id: legacy_zero_controller_tail
        type: u4
        repeat: expr
        repeat-expr: 3
        valid: 0
        if: _root.version <= 0x2021
      - id: saved_controller_flags
        type: u4
        doc: zero or 2; 2 marks retained initialized controller state
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
        doc: fixed 0x0028b1f8 in all eight scoped v2022 records
      - id: v2022_zero_allocator_state0
        type: u4
        repeat: expr
        repeat-expr: 4
        valid: 0
        if: _root.version == 0x2022
      - id: v2022_saved_capacity
        type: u4
        if: _root.version == 0x2022
        doc: fixed saved capacity 0x2ff in the scoped v2022 dialect
      - id: v2022_saved_list_handle
        type: u4
        if: _root.version == 0x2022
        doc: fixed saved list handle 0x001ff98c in the scoped v2022 dialect
      - id: v2022_zero_allocator_state1
        type: u4
        repeat: expr
        repeat-expr: 4
        valid: 0
        if: _root.version == 0x2022
      - id: modern_saved_controller_handle
        type: u4
        if: _root.version >= 0x2024
        doc: per-file saved tag-5 controller handle/counter; nonzero in every modern corpus file

  # =========================================================================
  # SECTION 5 — per-padstack pad-shape layer table
  # =========================================================================
  # Flat array of 24-B rows. sec4[k] owns rows [sec4[k].sec5_index, +layer_count).
  # Rows [0, firstStart~80) are a global default prefix.
  sec5_pad_layer_v20:
    doc: |
      Legacy 20-byte per-padstack layer row. The selector and shape are the
      first two bytes; dimensions begin at +4. Earlier schema revisions shifted
      these meanings by eight bytes. Values at +16 are angles in degrees times
      1,800,000. The C++ importer and paired pad-geometry exports consume this
      exact phase.
    seq:
      - id: layer_selector
        type: u1
        doc: '+0 PADS layer id; 0 top/default, 255 bottom/all, 21/23/27/28 documentation layers'
      - id: shape_code
        type: u1
        doc: '+1 pad-shape code; 0 OF, 1 RF, 2 round, 3 square, with 4..9 retained auxiliary shapes'
      - id: layer_state
        type: u2
        doc: '+2 saved layer-row flags/controller state'
      - id: size_a
        type: s4
        doc: '+4 pad dimension A (width or diameter) in BASIC units'
      - id: size_b
        type: s4
        doc: '+8 pad dimension B; zero for round/square rows'
      - id: shape_parameter_or_offset
        type: s4
        doc: '+12 shape-dependent offset/radius; 0x04000000|radius marks the default/thermal form'
      - id: angle
        type: s4
        doc: '+16 shape angle in degrees times 1,800,000'

  sec5_pad_layer_v24:
    doc: |
      Modern 24-byte per-padstack layer row. v0x2022 associates each selector
      after record zero with the preceding row's +4/+8 geometry carrier; later
      dialects use the same row. This is a serialized circular-controller
      association, not a content-selected phase.
    seq:
      - id: layer_selector
        type: u1
        doc: '+0 PADS layer id; 0 top/default, 255 bottom/all, and numbered documentation/copper layers'
      - id: shape_code
        type: u1
        doc: '+1 pad-shape code; 0 OF, 1 RF, 2 round, 3 square, with 4..9 retained auxiliary shapes'
      - id: layer_state
        type: u2
        doc: '+2 saved layer-row flags/controller state'
      - id: size_a
        type: s4
        doc: '+4 pad dimension A (width or diameter) in BASIC units'
      - id: size_b
        type: s4
        doc: '+8 pad dimension B; zero for round/square rows'
      - id: shape_parameter_or_offset
        type: s4
        doc: '+12 shape-dependent offset/radius; 0x04000000|radius marks the default/thermal form'
      - id: corner_radius_or_aux_dimension
        type: s4
        doc: '+16 corner radius or shape-specific auxiliary dimension in BASIC units'
      - id: angle
        type: s4
        doc: '+20 shape angle in degrees times 1,800,000'

  sec5_pad_layer_array:
    seq:
      - id: legacy_records
        type: sec5_pad_layer_v20
        repeat: eos
        if: _root.directory[5].total_bytes / _root.directory[5].count == 20
      - id: modern_records
        type: sec5_pad_layer_v24
        repeat: eos
        if: _root.directory[5].total_bytes / _root.directory[5].count == 24

  # =========================================================================
  # SECTION 8 — TEXT / label table
  # =========================================================================
  # Circular fixed array: 64 B in v0x2017, 72 B thereafter. Physical section 8
  # starts 28/36 bytes into the logical record ring. Metadata lags geometry by
  # one slot: record K+1 owns record K's geometry. Section 9 is the indexed
  # packed C-string allocation; str_offset is relative to its physical start.
  # Footprint field presentation uses the same controller without a pool string:
  # modern placement +96 directly names its first section-8 geometry record, whose
  # coordinates are local to the footprint. Metadata record K+1 association word
  # +12 uses high byte 0x08 and a low-24-bit section-8 ordinal to link geometry K
  # to the next field geometry. High byte 0x16 terminates a placement list (low 24
  # bits = placement ordinal); 0x0E terminates at a decal field list (low 24 bits =
  # decal ordinal). A chain reached from placement +96 must terminate with 0x16;
  # encountering a decal terminator there is a broken ownership link. In metadata
  # +24, byte 2 identifies the standard field: low
  # five bits 2 mean Ref.Des. and 3 mean Part Type/value; bit 5 is visibility.
  # Presentation bits 24 and 28 both set mean centered horizontally and
  # vertically; otherwise horizontal is left and bit 29 selects UP rather than
  # DOWN vertical justification. Free board text has
  # association word zero and uses the lagged metadata/string relationship above.
  sec8_text_header:
    seq:
      - id: object_id_0
        type: s4
        doc: '+0 idx0 object id / hash'
      - id: record_marker_or_retained_link
        type: u4
        doc: '+4 live-record marker 0x0000FFFE; free/capacity slots retain a process-local object link'
      - id: str_offset
        type: s4
        doc: '+8 idx2 string-pool byte offset (belongs to text K-1)'
      - id: flags
        type: u4
        doc: '+12 idx3 field-list association described above; zero for free board text'
      - id: text_state0
        type: s4
        doc: '+16 idx4 text-controller state; normally zero'
      - id: str_len_field
        type: u4
        doc: '+20 idx5 high16 = strlen+1 (drill-table subtype)'
      - id: layer_field
        type: u4
        doc: '+24 idx6 packed LEVEL, justification, mirror, and field-presentation state; metadata byte 2 low five bits identify Ref.Des. (2) or Part Type (3), and bit 5 is visibility; free text low16 is layer and high16=0x0020'
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
        doc: '+56 idx14 footprint-field mirror state (0/1); free-text geometry state is normally zero'
      - id: bbox_x
        type: s4
        doc: '+60 idx15 first-glyph corner X (RAW)'
      - id: bbox_y
        type: s4
        doc: '+64 idx16 first-glyph corner Y (RAW)'
      - id: object_id_1
        type: s4
        doc: '+68 idx17 object id / hash'

  sec8_text_header_legacy:
    seq:
      - id: object_id_0
        type: s4
      - id: record_marker_or_retained_link
        type: u4
        doc: '+4 live-record marker 0x0000FFFE; free/capacity slots retain a process-local object link'
      - id: str_offset
        type: u4
        doc: '+8 byte offset into physical section 9; belongs to geometry K-1'
      - id: flags
        type: u4
      - id: text_state0
        type: s4
      - id: str_len_field
        type: u4
      - id: layer_field
        type: u4
        doc: '+24 low byte layer; high16 0x0020 marks free board text'
      - id: height
        type: s4
      - id: line_width
        type: s4
      - id: origin_x
        type: s4
      - id: origin_y
        type: s4
      - id: rot_aux
        type: s4
      - id: text_state1
        type: s4
      - id: bbox_x
        type: s4
      - id: bbox_y
        type: s4
      - id: object_id_1
        type: s4

  sec8_text_metadata_modern:
    seq:
      - id: bytes
        size: 36
        doc: modern record K+1 metadata fields +0..+35 for geometry K

  sec8_text_metadata_legacy:
    seq:
      - id: bytes
        size: 28
        doc: v0x2017 record K+1 metadata fields +0..+27 for geometry K

  sec8_text_ring:
    seq:
      - id: modern_records
        type: sec8_text_header
        repeat: expr
        repeat-expr: _root.directory[8].count
        if: _root.version != 0x2017
      - id: modern_final_metadata
        type: sec8_text_metadata_modern
        if: _root.version != 0x2017
      - id: legacy_records
        type: sec8_text_header_legacy
        repeat: expr
        repeat-expr: _root.directory[8].count
        if: _root.version == 0x2017
      - id: legacy_final_metadata
        type: sec8_text_metadata_legacy
        if: _root.version == 0x2017

  # =========================================================================
  # SECTION 9 — text string-pool allocation
  # =========================================================================
  sec9:
    doc: |
      Exact section-9 allocation. Live strings are packed NUL-terminated ASCII;
      section-8 str_offset fields address bytes relative to this region's direct
      physical start. Remaining allocator capacity is retained zero storage.
      Section 10 begins only after this complete declared byte extent; no DRW
      record bytes belong to section 9.
    seq:
      - id: allocated_string_bytes
        size-eos: true
        doc: packed string bytes plus zero-filled unused allocator capacity

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
      - id: saved_state
        type: u2
        doc: '+2 saved object-state halfword; normally 0, value 31 on six live DRW records, and reused by non-live capacity slots'
      - id: sentinel
        type: s4
        doc: '+4 0xFFFFFFFF (-1)'
      - id: obj_index
        type: s4
        doc: '+8 first section-11 piece of the previous circular owner record'
      - id: index2
        type: s4
        doc: '+12 first section-12 vertex of the previous circular owner record'
      - id: index3
        type: s4
        doc: '+16 first section-13 arc parameter of the previous circular owner record'
      - id: zero0
        type: s4
        valid: 0
        doc: '+20 = 0'
      - id: flag6
        type: s4
        doc: '+24 section-11 piece count of the previous circular owner record'
      - id: flag7
        type: u4
        doc: '+28 packed previous-owner type; low16 item enum (0 LINES, 1 BOARD, 3 COPPER, 10 KEEPOUT), high16 flags'
      - id: retained_owner_state
        type: u4
        doc: '+32 saved owner state; normally zero, but live nonzero handles occur in the corpus'
      - id: heap_handle
        type: u4
        doc: '+36 heap/object handle, +0x41/record (handle)'
      - id: tag10
        type: u4
        doc: '+40 0x80000000'
      - id: handle_str
        size: 40
        doc: '+44 inline "DRW#######" for live objects; unused capacity retains arbitrary bytes'
      - id: block_tag
        type: u4
        doc: '+84 class tag; 0x00004900 (v0x2025/v0x2027 filled-copper owner), 0x00004D00 (v0x2026 filled-copper owner), 0 (keepout / board-outline owner)'
      - id: verts
        type: s4
        repeat: expr
        repeat-expr: 6
        doc: '+88 x0,y0,x1,y1,x2,y2 RAW (x0/y0 = *LINES* insertion point)'

  sec10_drw_record_head:
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
      - id: previous_piece_count_or_class
        type: s4
      - id: subtype
        type: u4
      - id: retained_state
        type: u4
        repeat: expr
        repeat-expr: 3

  sec10_drw_record_tail:
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

  sec10_legacy_record:
    seq:
      - id: head
        type: sec10_legacy_record_head
      - id: tail
        type: sec10_legacy_record_tail

  sec10_legacy_record_head:
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
      - id: previous_piece_count_or_class
        type: s4
      - id: subtype
        type: u4
        doc: '+28 packed previous-owner drawing subtype and flags'

  sec10_legacy_record_tail:
    seq:
      - id: name_storage
        size: 40
      - id: block_tag
        type: u4
      - id: origin_and_bbox_state
        type: s4
        repeat: expr
        repeat-expr: 6

  sec10_drawing_physical:
    doc: |
      Circular fixed array. The writer rotates the logical record bytes left by
      68 within this controller: physical storage is the last record's tail,
      records 0..count-2, then the last record's head. No byte belongs to section
      9 or 11. Record R[(i+1) mod count] carries R[i]'s piece/vertex/arc cursors.
    seq:
      - id: modern_last_record_tail
        type: sec10_drw_record_tail
        if: _root.version >= 0x2024
      - id: modern_records
        type: sec10_drw_record
        repeat: expr
        repeat-expr: _root.directory[10].count - 1
        if: _root.version >= 0x2024
      - id: modern_last_record_head
        type: sec10_drw_record_head
        if: _root.version >= 0x2024
      - id: legacy_last_record_tail
        type: sec10_legacy_record_tail
        if: _root.version <= 0x2022
        doc: logical bytes 32..99 of the final 100-byte legacy owner record
      - id: legacy_records
        type: sec10_legacy_record
        repeat: expr
        repeat-expr: _root.directory[10].count - 1
        if: _root.version <= 0x2022
      - id: legacy_last_record_head
        type: sec10_legacy_record_head
        if: _root.version <= 0x2022
        doc: logical bytes 0..31 of the final 100-byte legacy owner record

  # =========================================================================
  # SECTION 11 — graphic-piece header table + inline board-outline vertices
  # =========================================================================
  # 20 B/record HEAD (one per OPEN/CLOSED/CIRCLE piece of *LINES* / *PARTDECAL*),
  # then a TAIL (X,Y,attr) i32 triple stream of closed geometry. Arc runs use
  # attr 0,1,2... as indexes into a parallel 20-byte bbox arc table.
  #
  # Dimensions are not a dedicated section; each DIM* item is a *LINES* DRW owner
  # whose sub-pieces appear here. For those sub-pieces the +0 word (sub_flag |
  # byte1_or_next_level<<8 = low-u16 of field0) is a piece-type enum: 6162 BASPNT, 6156 ARWLN1,
  # 6157 ARWLN2, 6158 ARWHD1, 6159 ARWHD2, 6160 EXTLN1, 6161 EXTLN2; the +4 flags
  # word is the per-dimension group flag (matches the ASC piece flags column). The
  # sub-piece vertices land in sec12 in ASC order (BASPNT1 BASPNT2 ARWLN1 ARWHD1
  # ARWLN2 ARWHD2 EXTLN1 EXTLN2), reached through the DIM* owner-run vertex cursor.
  sec11_piece_hdr:
    seq:
      - id: sub_flag
        type: u1
        doc: '+0 previous-piece shape flag; value 2 marks the preceding LINES piece as CIRCLE'
      - id: byte1_or_next_level
        type: u1
        doc: |
          +1 role-dependent carrier byte. For LINES and COPPER graphic piece K,
          piece K+1 stores K's ASCII LEVEL here. The direct owner piece_start and
          piece_count ranges therefore determine every layer without inspecting
          geometry. Verified against the 5,281-piece level-25 mechanical drawing
          and level-26 RobotCub silkscreen in MAIS_L_02, plus every COPPER object in
          paired DC1096B, Ems4_Rev2, and MC4_PLUS_CSHAPE exports. For DIM
          sub-pieces it is the high byte of the u16 piece-type enum.
      - id: type_or_handle_high
        type: u2
        doc: '+2 piece-type or object-handle high bits'
      - id: flags
        type: s4
        doc: '+4 -1 default; 0x800/0x1000 keepout-restriction bits; small ints = ordinal/parent'
      - id: piece_state
        type: s4
        doc: '+8 saved piece state; values 0 through 3 occur in live corpus records'
      - id: width
        type: s4
        doc: '+12 pen width, BASIC'
      - id: corners
        type: s4
        doc: '+16 vertex/corner count; one-corner LINES point records are valid but have no drawable segment'

  sec11_piece_head:
    seq:
      - id: sub_flag
        type: u1
      - id: byte1_or_next_level
        type: u1
      - id: type_or_handle_high
        type: u2
      - id: flags
        type: s4

  sec11_piece_head_modern:
    seq:
      - id: head
        type: sec11_piece_head
      - id: retained_piece_state
        type: s4

  sec11_piece_tail:
    seq:
      - id: width
        type: s4
      - id: corners
        type: s4

  sec11_piece_hdr_legacy:
    seq:
      - id: head
        type: sec11_piece_head
      - id: width
        type: s4
      - id: corners
        type: s4

  sec11_piece_physical:
    doc: |
      Circular fixed array rotated right by its record head: twelve bytes for
      modern 20-byte records, eight for legacy 16-byte records. Physical storage
      is record 0's tail, records 1..count-1, then record 0's head. This phase is
      independently fixed by ASCII BOARD corner counts; the former +8 phase
      produced plausible records but assigned the next piece to every owner.
    seq:
      - id: modern_last_record_tail
        type: sec11_piece_tail
        if: _root.version >= 0x2025
      - id: modern_records
        type: sec11_piece_hdr
        repeat: expr
        repeat-expr: _root.directory[11].count - 1
        if: _root.version >= 0x2025
      - id: modern_last_record_head
        type: sec11_piece_head_modern
        if: _root.version >= 0x2025
      - id: legacy_last_record_tail
        type: sec11_piece_tail
        if: _root.version <= 0x2024
        doc: logical record 0's width and corner count
      - id: legacy_records
        type: sec11_piece_hdr_legacy
        repeat: expr
        repeat-expr: _root.directory[11].count - 1
        if: _root.version <= 0x2024
      - id: legacy_last_record_head
        type: sec11_piece_head
        if: _root.version <= 0x2024

  sec12_graphic_vertex:
    doc: |
      Section-12 fixed 12-byte graphic vertex. All 597 corpus files satisfy
      total_bytes == count * 12. Coordinates are DESIGN-local for the owning
      section-11 graphic piece and are not origin shifted.
    seq:
      - id: x_design
        type: s4
      - id: y_design
        type: s4
      - id: arc_ordinal
        type: s4
        doc: -1 = straight contour vertex; nonnegative = owner-local arc ordinal

  sec12_vertex_array:
    seq:
      - id: records
        type: sec12_graphic_vertex
        repeat: eos

  # =========================================================================
  # SECTION 13 — graphic arc parameters / copper-pour hatch geometry
  # =========================================================================
  # The entire directory payload is a 20-byte geometry-parameter array. Graphic
  # owners address arc bounding boxes directly as arc_start + vertex.arc_ordinal;
  # copper-pour owners address the same-width rows as hatch segments.
  sec13_hatch_seg:
    doc: 20-byte graphic parameter; four coordinates plus retained type-specific state
    seq:
      - id: x1
        type: s4
        doc: arc bbox xmin, or hatch-segment x1
      - id: y1
        type: s4
        doc: arc bbox ymin, or hatch-segment y1
      - id: layer_marker
        type: s4
        doc: arc bbox xmax, or hatch layer marker BASE-900*layer
      - id: x2
        type: s4
        doc: arc bbox ymax, or hatch-segment x2
      - id: y2
        type: s4
        doc: retained arc state, or hatch-segment y2

  sec13_hatch_array:
    seq:
      - id: records
        type: sec13_hatch_seg
        repeat: eos

  # =========================================================================
  # SECTION 14 — PARTDECAL terminal-run descriptors
  # =========================================================================
  # Physical descriptors start at the decal name, 44 bytes after the nominal
  # section boundary. Both dialects carry sentinel 0xFFFE at +64, the section-15
  # terminal cursor/count at +68/+72, and the section-16 per-pin padstack
  # cursor/count at +44/+88. Modern records are 112 bytes; legacy records are
  # 100 bytes. Reading at the nominal boundary produces a plausible but false
  # one-record-lag view of these fields.
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
      - id: decal_name
        size: 44
        doc: '+0 NUL-padded ASCII decal name for live descriptors; free slots retain arbitrary bytes'
      - id: sec16_padstack_cursor
        type: s4
        doc: '+44 first record in the section-16 per-terminal padstack map'
      - id: descriptor_state0
        size: 16
        doc: '+48..+63 saved decal-controller links and state'
      - id: sentinel
        type: u2
        doc: '+64 0xFFFE for a live descriptor'
      - id: flag_b
        type: u2
        doc: '+66 saved descriptor flags'
      - id: sec15_start
        type: s4
        doc: '+68 first terminal index in the section-15 terminal pool'
      - id: terminal_count
        type: s4
        doc: '+72 number of section-15 terminals owned by this decal'
      - id: descriptor_state1
        size: 12
        doc: '+76..+87 saved terminal-controller links and state'
      - id: sec16_padstack_count
        type: s4
        doc: '+88 number of section-16 per-terminal padstack records'
      - id: descriptor_state2
        size: 20
        doc: '+92..+111 saved modern descriptor capacity and state'
    instances:
      sec15_zero_based_start:
        value: sec15_start
        doc: zero-based first owned section-15 terminal slot

  sec14_terminal_desc_v100:
    doc: legacy 100-byte PARTDECAL terminal-run descriptor
    seq:
      - id: decal_name
        size: 44
        doc: '+0 NUL-padded ASCII decal name for live descriptors; free slots retain arbitrary bytes'
      - id: sec16_padstack_cursor
        type: s4
        doc: '+44 first record in the section-16 per-terminal padstack map'
      - id: descriptor_state0
        size: 16
        doc: '+48..+63 saved legacy decal-controller links and state'
      - id: sentinel
        type: u2
        doc: '+64 0xFFFE for a live descriptor'
      - id: flag_b
        type: u2
        doc: '+66 saved descriptor flags'
      - id: sec15_start
        type: s4
        doc: |
          +68 first terminal cursor in the legacy section-15 pool. Positive
          v2017/v2019 values are one-based pool ordinals; zero selects the
          shared first slot used by built-in via decals.
      - id: terminal_count
        type: s4
        doc: '+72 number of section-15 terminals owned by this decal'
      - id: descriptor_state1
        size: 12
        doc: '+76..+87 saved terminal-controller links and state'
      - id: sec16_padstack_count
        type: s4
        doc: '+88 number of section-16 per-terminal padstack records'
      - id: descriptor_state2
        size: 8
        doc: '+92..+99 saved legacy descriptor capacity and state'
    instances:
      sec15_zero_based_start:
        value: '_root.version <= 0x2019 and sec15_start > 0 ? sec15_start - 1 : sec15_start'
        doc: decoded zero-based first owned section-15 terminal slot

  # =========================================================================
  # SECTION 15 — PARTDECAL terminal and controller storage
  # =========================================================================
  # The directory describes fixed storage units: 20 bytes in versions 0x2017
  # and 0x2019, 36 bytes thereafter. Units hold terminal geometry or mixed
  # decal-controller/object-dictionary state. Modern files place terminal units
  # first, but their suffix is variable and is not a fixed 33-unit trailer.
  # Legacy files start after a 60-byte rotated descriptor tail, 16 bytes into the
  # physical controller ring. Modern units start after the 44-byte logical-view
  # displacement, exactly at the physical controller start. The largest serialized
  # terminal cursor plus count equals directory[15].count on every corpus file.
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

  saved_terminal_controller_prefix:
    doc: |
      Four saved tag-15 controller words serialized before the v2017/v2019
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
  # The entire declared payload is count x 8 bytes at the physical controller
  # boundary and maps each decal terminal ordinal to a section-4 padstack
  # ordinal. The former 224-byte spilled-PARTTYPE interpretation crossed into
  # section 17.
  decal_padstack_pair_array:
    seq:
      - id: records
        type: decal_padstack_pair
        repeat: eos

  decal_padstack_pair:
    seq:
      - id: terminal_ordinal
        type: s4
        doc: zero selects the decal-wide default; positive values select that one-based terminal ordinal
      - id: padstack_ordinal
        type: u4
        doc: zero-based ordinal into the section-4 padstack array

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
        size: 36
        doc: '+44 PARTTYPE NAME in live records; retained capacity can contain arbitrary bytes'
      - id: parttype_link_state
        size: 16
        doc: '+80..+95 heap links and duplicated definition indexes'
      - id: decal_selection_slots
        type: parttype_decal_selection_slot
        repeat: expr
        repeat-expr: 16
        doc: |
          +96..+223 fixed-capacity alternate-decal vector. Live slots repeat
          the same nonnegative section-14 decal ordinal in both words; the first
          nonmatching/negative slot terminates the live selection list and the
          remaining slots retain allocator state.

  parttype_record_v208:
    seq:
      - id: parttype_controller_state
        size: 44
      - id: name_storage
        size: 36
        doc: '+44 legacy PARTTYPE NAME in live records; retained capacity can contain arbitrary bytes'
      - id: parttype_link_state
        size: 32
        doc: '+80..+111 legacy definition links and controller state'
      - id: primary_decal_ordinal
        type: s4
        doc: '+112 zero-based section-14 decal ordinal; negative means no decal selection'
      - id: retained_object_capacity
        size: 92
        doc: '+116..+207 retained legacy PARTTYPE object capacity; not file padding'

  parttype_decal_selection_slot:
    seq:
      - id: decal_ordinal
        type: s4
      - id: decal_ordinal_duplicate
        type: s4

  # =========================================================================
  # SECTION 18 — final PARTTYPE metadata and gate-definition stream
  # =========================================================================
  # The nominal section boundary lands at the start of the final rotated
  # PARTTYPE metadata. Its 44-byte prefix is followed by one 8-byte gate record
  # per directory[18] item. The gate array therefore ends exactly 44 bytes into
  # nominal section 19 on all 597 corpus files.
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
        size: 3
        doc: |
          Three-byte PARTTYPE type code on modern files. Legacy v0x2017/v0x2019
          place the code later in this rotated record, so retain these bytes raw.
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
  # Controlled PADS Layout V10 exports from the exact loaded binaries confirm the
  # direct view: v0x2021 Dexter_MotorCtrl is 284/284 REFDES and v0x2026
  # parallella-rf is 162/162, with no extra or missing names. Older ASCII files
  # paired by basename can describe a different board revision and must not be
  # used to infer free slots or a second placement stream.
  # v2021/22 and older use the 96-byte prefix; v2024+ append 16 bytes.
  # Association state is one-record lagged: modern record N+1
  # +4/+17 selects record N's PARTTYPE/alternate, while legacy record N+1 +24
  # selects record N's decal. The last selector continues into the following
  # controller's 44-byte rotated metadata; it does not wrap to record zero.
  # Geometry and reference designators are not lagged.
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
        doc: '+4 parttype index for the preceding placement in the record ring on modern dialects'
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
        doc: '+20 legacy parttype/controller index; placement object ID is the record ordinal'
      - id: sentinel_m1
        type: s4
        doc: '+24 legacy decal index for the preceding placement in the record ring'
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
        doc: '+96 nonnegative value is the direct section-8 ordinal of the first field-presentation record; negative values mark placements with no saved field presentation'
      - id: handle2
        type: u4
        doc: '+100 per-instance serial/handle (handle)'
      - id: trailing_object_handle
        type: s4
        doc: '+104 normally zero; retained saved object handles occur in allocated records'
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
        doc: retained controller state; legacy decal selection is the next record's +24 word
      - id: decal_index2
        type: s4
      - id: instance_index
        type: s4
      - id: flags16
        type: s4
      - id: object_id
        type: s4
        doc: legacy parttype/controller index; placement object ID is the record ordinal
      - id: sentinel_m1
        type: s4
        doc: decal index for the preceding placement in the record ring
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
        size: 16
        doc: '+4 cluster NAME in live records; retained/free records can contain arbitrary bytes'
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
        type: 'legacy_net_record_ring(_root.directory[23].count)'
        if: _root.directory[23].total_bytes / _root.directory[23].count == 144

  saved_net_controller_prefix:
    doc: |
      Eleven saved tag-23 controller words between the rotated placement grid
      and the v2017..v2022 net-record array. They retain controller/list state
      and are not alignment padding.
    seq:
      - id: saved_controller_words
        type: u4
        repeat: expr
        repeat-expr: 11

  legacy_sigpin_final_record_tail:
    doc: |
      The v2017 56-byte SIGPIN array is rotated 48 bytes past its nominal tag-20
      boundary. This word is bytes +52..+55 of the final record's fixed-capacity
      signal-name slot. Both populated scoped rings retain 0x001d1168 here. When
      tag 20 is empty, physical +44 belongs to the following tag-21 compact-name
      or tag-22 placement controller and is not a SIGPIN field.
    seq:
      - id: retained_signal_name_capacity_tail
        type: u4

  legacy_net_record_v144:
    seq:
      - id: head
        type: legacy_net_record_head
      - id: tail
        type: legacy_net_record_tail

  legacy_net_record_head:
    seq:
      - id: anchor_part_idx
        type: s4
        doc: '+0 0-based index into section 22 of a member part'
      - id: anchor_pin
        type: s4
        doc: '+4 terminal/pin number on anchor part'
      - id: sec24_start
        type: s4
        doc: |
          +8 index of one section-24 edge in this signal component. Components
          are not contiguous in record order and this need not be their lowest
          edge index; following the edge's union-find component yields the
          complete signal. Treating it as a range start omitted the $2N171 and
          PSTAGE-002 PX1 components in paired v0x2021 exports.
      - id: name_slot
        size: 48
        doc: |
          +12 ASCII NUL-terminated net name. The section-23 directory count is
          the exact number of active legacy records.
      - id: net_controller_state0
        size: 24
        doc: '+60..+83 serialized legacy net-controller state'
      - id: net_class_owner_handle
        type: u4
        doc: '+84 saved section-66 class handle; zero when the net has no class'
      - id: net_class_membership_state
        type: u4
        doc: '+88 retained class-membership state'
      - id: conn_count
        type: s4
        doc: '+92 number of connections (= section-24 entries for this net)'
      - id: net_controller_state1
        size: 28
        doc: '+96..+123 serialized legacy net-controller state'

  legacy_net_record_tail:
    seq:
      - id: net_controller_state2
        size: 20
        doc: '+124..+143 serialized legacy net-controller state'

  legacy_net_record_ring:
    params:
      - id: num_records
        type: u4
    doc: |
      Legacy section-23 circular array. Logical record zero begins at physical
      +20: the physical prefix is the final record's 20-byte tail, followed by
      records zero through count-2 and the final record's 124-byte head.
    seq:
      - id: final_record_tail
        type: legacy_net_record_tail
      - id: complete_records
        type: legacy_net_record_v144
        repeat: expr
        repeat-expr: num_records - 1
      - id: final_record_head
        type: legacy_net_record_head

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
        doc: |
          +60 saved pin-edge count for this signal's connection component; this
          is corroborating state, not a contiguous section-24 extent. A zero-edge
          $$$ autoroute placeholder owns no pin even though its retained anchor
          fields duplicate a named signal. All 46 zero-edge $$$ records in the
          scoped corpus share such a named anchor; none is an independent signal.
      - id: anchor_part_idx
        type: s4
        doc: |
          +64 0-based index into section 22 of a member part. For zero-edge $$$
          placeholders this is retained alias state, not signal membership.
      - id: anchor_pin
        type: s4
        doc: |
          +68 terminal/pin number on the anchor part. Named connected signals and
          named singleton signals own this pin; zero-edge $$$ placeholders do not.
      - id: sec24_start
        type: s4
        doc: '+72 index of one section-24 edge in this signal component; components are not contiguous in record order'
      - id: name_slot
        size: 48
        doc: |
          +76 ASCII NUL-terminated net name. Names beginning $$$ are PADS
          autoroute aliases/placeholders; a named signal wins when both retain
          the same anchor or section-24 component.
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
  # The directory count is the number of live undirected edges between placed-
  # part terminal identities. Modern record zero is the topology root. Edge K
  # takes its placement-object indices from full record K at +60/+64 and its
  # terminal ordinals and topology expression from record K+1 at +0..+20. The
  # final record K+1 is a 36-byte head at the end of the physical controller;
  # no guessed wrap, content-selected closing edge, or fallback is involved.
  # Unioning these edges forms one component per electrical signal. Each modern
  # net record's anchor_part_idx/anchor_pin identifies its component directly;
  # sec24_start is a member edge, not a contiguous block boundary. $$$ records
  # are PADS autoroute aliases and can anchor the same component as its named
  # signal. Records carry a 0xFE high-byte state at +20 and 0x0000FFFE at +52.
  saved_connection_controller_prefix:
    doc: |
      Saved tag-24 controller prefix before the circular connection array: four
      retained words through v0x2022 and two words thereafter. The following
      logical connection record begins immediately after this prefix.
    seq:
      - id: legacy_saved_controller_links
        type: u4
        repeat: expr
        repeat-expr: 4
        if: _root.version <= 0x2022
        doc: four saved process-local topology/list links; usually nonzero and file-specific
      - id: modern_zero_controller_words
        type: u4
        repeat: expr
        repeat-expr: 2
        valid: 0
        if: _root.version >= 0x2024

  empty_route_chain_controller_state:
    doc: |
      Modern empty connection controllers retain the fixed 36-byte terminal
      head occupied by the final topology node in a nonempty ring. Ten of the
      thirteen scoped records are initialized to zero. Three retain allocator
      state: state0=2, free-list head=-1, capacity=32, list tail=-1, and a
      saved allocator handle. One of those also retains -1 in both saved links.
      These words are controller state, not padding.
    seq:
      - id: saved_controller_state0
        type: u4
      - id: saved_free_list_head
        type: s4
      - id: saved_capacity
        type: u4
      - id: saved_controller_state1
        type: u4
      - id: saved_list_tail
        type: s4
      - id: saved_link_a
        type: s4
      - id: saved_link_b
        type: s4
      - id: saved_allocator_handle
        type: u4
      - id: saved_controller_state2
        type: u4

  route_chain_array:
    params:
      - id: num_edges
        type: u4
      - id: is_modern
        type: u1
    seq:
      - id: modern_records
        type: 'route_chain_record(_index)'
        repeat: expr
        repeat-expr: num_edges
        if: is_modern != 0
      - id: modern_final_edge_head
        type: 'route_chain_edge_head(num_edges)'
        if: is_modern != 0
      - id: legacy_records
        type: legacy_route_chain_record
        repeat: expr
        repeat-expr: num_edges
        if: is_modern == 0

  route_chain_edge_head:
    params:
      - id: record_index
        type: u4
    seq:
      - id: ordinal_a
        type: s4
        doc: '+0 terminal ordinal paired with the preceding full record endpoint A'
      - id: ordinal_b
        type: s4
        doc: '+4 terminal ordinal paired with the preceding full record endpoint B'
      - id: topology_ref_a
        type: s4
        valid:
          expr: '_ >= 0 ? _ <= _root.directory[24].count : -_ - 1 < _root.directory[22].count'
        doc: |
          +8 first input of the final edge's topology expression. Nonnegative
          values are section-24 node ordinals. Negative value -(N+1) is direct
          section-22 placement leaf N and equals the preceding full record's
          endpoint_object_a when that input is a leaf.
      - id: topology_ref_b
        type: s4
        valid:
          expr: '_ >= 0 ? _ <= _root.directory[24].count : -_ - 1 < _root.directory[22].count'
        doc: |
          +12 second final-edge input, using the same section-24-node or
          section-22-placement-leaf encoding as +8 and the preceding full
          record's endpoint_object_b for a leaf.
      - id: topology_ref_c
        type: s4
        valid:
          expr: '_ >= 0 ? _ <= _root.directory[24].count : -_ <= _root.directory[23].count'
        doc: |
          +16 final-edge output. Nonnegative values are section-24 node
          ordinals. Negative value -(N+1) completes named section-23 signal N.
      - id: marker
        type: u4
        valid:
          expr: '(_ & 0xffffffc0) == 0xfe000000'
      - id: route_chain_state0
        size: 12
        doc: '+24..+35 final edge topology-controller state'

  route_chain_record:
    params:
      - id: record_index
        type: u4
    seq:
      - id: ordinal_a
        type: s4
        doc: '+0 terminal ordinal for the preceding full record endpoint A'
      - id: ordinal_b
        type: s4
        doc: '+4 terminal ordinal for the preceding full record endpoint B'
      - id: topology_ref_a
        type: s4
        valid:
          expr: '_ >= 0 ? _ <= _root.directory[24].count : -_ - 1 < _root.directory[22].count'
        doc: |
          +8 first input of the following edge's topology expression.
          Nonnegative values are section-24 node ordinals. Negative value
          -(N+1) is direct section-22 placement leaf N and equals the preceding
          full record's endpoint_object_a when that input is a leaf.
      - id: topology_ref_b
        type: s4
        valid:
          expr: '_ >= 0 ? _ <= _root.directory[24].count : -_ - 1 < _root.directory[22].count'
        doc: |
          +12 second topology-expression input, using the same nonnegative
          section-24-node / negative section-22-placement-leaf encoding as +8;
          a leaf equals the preceding full record's endpoint_object_b.
      - id: topology_ref_c
        type: s4
        valid:
          expr: '_ >= 0 ? _ <= _root.directory[24].count : -_ <= _root.directory[23].count'
        doc: |
          +16 topology-expression output. Nonnegative values are section-24
          node ordinals. Negative values are completed signal-component IDs in
          declaration order: -(N+1) closes named section-23 signal N. Values
          beyond the placement count are therefore signal IDs, not an extra
          placement-leaf namespace.
      - id: marker
        type: u4
        valid:
          expr: 'record_index == 0 or (_ & 0xffffffc0) == 0xfe000000'
        doc: |
          +20 topology-root state on modern record zero; modern later records
          have high byte 0xFE and low route flags. Legacy dialects store a
          traversal ordinal/state value instead.
      - id: route_chain_state0
        size: 12
        doc: '+24..+35 route-chain controller state'
      - id: end_x1
        type: s4
        doc: '+36 optional RAW endpoint X (else 0)'
      - id: end_y1
        type: s4
        doc: '+40 optional RAW endpoint Y (else 0)'
      - id: end_x2
        type: s4
        doc: '+44 optional RAW endpoint X (else 0)'
      - id: end_y2
        type: s4
        doc: '+48 optional RAW endpoint Y (else 0)'
      - id: flag_fffe
        type: u4
        valid:
          expr: '(_ & 0xffff) == 0xfffe'
        doc: |
          +52 modern endpoint-state word whose low half is 0xFFFE; its high
          half retains controller flags. Legacy dialects use traversal state.
      - id: route_chain_state2
        type: s4
        doc: '+56 route-chain controller state'
      - id: endpoint_object_a
        type: u4
        doc: '+60 zero-based section-22 placement-object index for endpoint A; ordinal is the following topology head +0'
      - id: endpoint_object_b
        type: u4
        doc: '+64 zero-based section-22 placement-object index for endpoint B; ordinal is the following topology head +4'

  legacy_route_chain_record:
    doc: |
      v0x2017..v0x2022 connection edge. This is the same saved controller
      field cycle as the modern record, rotated by sixteen bytes: the endpoint
      objects and terminal ordinals occupy one record, rather than pairing the
      objects with the following record's head. The marker and endpoint flag
      validate all 10,011 declared legacy records in the scoped corpus.
    seq:
      - id: flag_fffe
        type: u4
        valid:
          expr: '(_ & 0xffff) == 0xfffe'
        doc: '+0 endpoint-state word; low half is the serialized 0xFFFE flag'
      - id: route_chain_state2
        type: s4
        doc: '+4 route-chain controller state'
      - id: endpoint_object_a
        type: u4
        doc: '+8 serialized placement-object identifier for endpoint A'
      - id: endpoint_object_b
        type: u4
        doc: '+12 serialized placement-object identifier for endpoint B'
      - id: ordinal_a
        type: u4
        doc: '+16 terminal ordinal on endpoint A'
      - id: ordinal_b
        type: u4
        doc: '+20 terminal ordinal on endpoint B'
      - id: topology_ref_a
        type: s4
        doc: '+24 first saved topology-expression input/link'
      - id: topology_ref_b
        type: s4
        doc: '+28 second saved topology-expression input/link'
      - id: topology_ref_c
        type: s4
        doc: '+32 saved topology-expression output/link'
      - id: marker
        type: u4
        valid:
          expr: '(_ & 0xffffffc0) == 0xfe000000'
        doc: '+36 route-chain marker with low route flags'
      - id: route_chain_state0
        size: 12
        doc: '+40..+51 topology-controller state'
      - id: end_x1
        type: s4
        doc: '+52 optional RAW endpoint X, otherwise zero'
      - id: end_y1
        type: s4
        doc: '+56 optional RAW endpoint Y, otherwise zero'
      - id: end_x2
        type: s4
        doc: '+60 optional RAW endpoint X, otherwise zero'
      - id: end_y2
        type: s4
        doc: '+64 optional RAW endpoint Y, otherwise zero'

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
      - id: controller_state_prefix
        type: u4
        repeat: expr
        repeat-expr: 45
        doc: retained global route/controller state through byte +179
      - id: allocator20_page_count
        type: u2
        doc: '+180 number of section-26 descriptors for the runtime 20-byte allocator'
      - id: allocator48_page_count
        type: u2
        doc: '+182 number of section-26 descriptors for the runtime 48-byte allocator'
      - id: allocator88_page_count
        type: u2
        doc: '+184 number of section-26 descriptors for the runtime 88-byte allocator'
      - id: allocator56_page_count
        type: u2
        doc: '+186 number of section-26 descriptors for the runtime 56-byte section-61 node allocator'
      - id: controller_state_suffix
        type: u4
        repeat: eos
        doc: retained route/controller state following the page-group counts

  route_object_range_array:
    seq:
      - id: records
        type: route_object_range
        repeat: eos

  route_object_range:
    doc: |
      12-byte page descriptor. The loader reads `record_count * controller_stride`
      bytes for this page. `allocation_begin` and `allocation_end` are saved RAM
      addresses used only to rebuild the allocator; neither is a file offset.
    seq:
      - id: allocation_begin
        type: u4
        doc: saved process address of the first object in this allocator span
      - id: allocation_end
        type: u4
        doc: saved process address immediately after the allocator span
      - id: record_count
        type: u4
        doc: exact number of serialized fixed-stride records in this page

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
      total_records:
        value: '(first.record_count + (num_descriptors > 1 ? remaining.total_records : 0)).as<u4>'
        doc: exact serialized record count summed from this controller's pages

  route_layer_object_count_array:
    doc: |
      One u32 route-object count per copper layer. The directory count is the
      number of copper layers (2, 4, 6, 8, 10, or 12 in this corpus), and the
      sum of these values equals directory[29].count on all 597 unique files.
    seq:
      - id: object_counts
        type: u4
        repeat: eos

  route_object_handle_array:
    doc: |
      Flat route-object handle vector. Handles are saved process addresses,
      commonly 8-byte aligned on the modern allocator and spaced on a 56-byte
      object grid. Partition the vector into copper-layer groups using the
      section-27 counts. Section 25's fourth page-group count identifies the
      section-26 descriptor group for the 56-byte section-61 node allocator.
      A handle's page and 56-byte ordinal identify its section-61 record without
      searching. This direct relation resolves every nonzero section-29 handle
      on all 597 corpus binaries. The former fixed 297-word template tail was a
      phase error caused by reading at the raw, directory-overdeclared offset.
    seq:
      - id: object_handles
        type: u4
        repeat: eos
        doc: saved route-object process address; never a file offset

  # =========================================================================
  # PAGED CONTROLLERS 41, 42, 45, 46, 47, 48
  # =========================================================================
  # Each controller's directory total_bytes is its number of section-26 page
  # descriptors. Each descriptor supplies the exact record count for one page.
  # PADS writes page payloads in controller order 41,42,45,46,47,48.
  section41_paged_controller:
    seq:
      - id: records
        type:
          switch-on: _root.version
          cases:
            0x2017: section41_clearance_record_v180
            _: section41_clearance_record
        repeat: expr
        repeat-expr: '_root.section41_record_count'

  section42_paged_controller:
    seq:
      - id: records
        type: section41_high_speed_rule_record
        repeat: expr
        repeat-expr: '_root.section42_record_count'

  section45_paged_controller:
    seq:
      - id: records
        size: '_root.version == 0x2017 ? 116 : 124'
        repeat: expr
        repeat-expr: '_root.section45_record_count'
        doc: fixed-stride layer-rule objects for this allocator page

  section46_paged_controller:
    seq:
      - id: record_chain
        type: 'section46_record_chain(_root.section46_record_count)'
        if: _root.section46_record_count > 0
    instances:
      num_live:
        value: '(_root.section46_record_count > 0 ? record_chain.num_live : 0).as<u4>'

  section46_record_chain:
    params:
      - id: num_records
        type: u4
    seq:
      - id: record
        type: section46_heap_record
        if: num_records == 1
      - id: left
        type: 'section46_record_chain(num_records / 2)'
        if: num_records > 1
      - id: right
        type: 'section46_record_chain(num_records - num_records / 2)'
        if: num_records > 1
    instances:
      num_live:
        value: '(num_records == 1 ? (record.saved_rule_handle < 0x80000000 and record.saved_via_type_set_handle != 0 ? 1 : 0) : left.num_live + right.num_live).as<u4>'

  section46_heap_record:
    doc: |
      Route-rule heap slot. The source heap walker skips negative first words.
      Its conversion callback also drops null saved via-type-set handles before
      the tag-51 state reader walks the destination heap. The zero/zero slot in
      add-on_14_05_05_15_06_12 is therefore retained source capacity, not live.
    seq:
      - id: saved_rule_handle
        type: u4
      - id: saved_via_type_set_handle
        type: u4
      - id: route_rule_fields
        size: '_root.version <= 0x2019 ? 24 : 32'

  section47_paged_controller:
    seq:
      - id: records
        size: 24
        repeat: expr
        repeat-expr: '_root.section47_record_count'
        doc: fixed-stride via-rule objects for this allocator page

  section48_paged_controller:
    seq:
      - id: records
        size: '_root.version <= 0x2019 ? 48 : (_root.version <= 0x2022 ? 856 : 864)'
        repeat: expr
        repeat-expr: '_root.section48_record_count'
        doc: fixed-stride differential-pair rule objects for this allocator page

  # Retained while downstream consumers migrate to the page-framed types above.
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
    doc: 188-byte clearance rule with a 12-byte selector/handle header, 38 values, and 24 bytes of state
    seq:
      - id: layer_selector
        type: u4
        doc: zero for global rules; nonzero layer ordinal for layer-specific rules
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
        doc: CLEARANCE_RULE values in ASCII declaration order and BASIC units
      - id: rule_metadata_tail
        size: 24
        doc: common live rule ownership, width, and controller metadata; not padding

  section41_clearance_record_v180:
    doc: 180-byte v0x2017/v0x2019 clearance rule with 38 values and a 16-byte ownership trailer
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
    doc: 864-byte DIF_PAIR rule slot; active slots join member handles to section-23 nets
    seq:
      - id: controller_state
        size: 12
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
        repeat-expr: 70
        doc: diff-pair rule values; element 0 is MAX_LENGTH, element 1 inherited GAP, and element 3 GAP override
      - id: inherited_width
        type: s4
      - id: width_state
        type: s4
      - id: width_override
        type: s4
      - id: retained_allocator_capacity
        size: 260
        doc: allocator capacity, mostly 0xff free bytes but sometimes retaining live rule values; never file padding

  section48_diff_pair_record_array:
    seq:
      - id: records
        type: section41_diff_pair_record
        repeat: expr
        repeat-expr: _root.section48_record_count

  # =========================================================================
  # SECTION 49 — route-object relationship stream
  # =========================================================================
  section49_relationship_stream:
    seq:
      - id: signal_records
        type: section49_signal_relationships
        repeat: eos

  section49_signal_relationships:
    doc: |
      Connectivity lists owned by one live section-23 signal/net object. Record
      order is the live section-23 net order. Forward object IDs tagged 0x3c
      name section-60 route-junction ordinals; their tagged 0x18 values name
      section-24 route-chain ordinals. This declared graph assigns route
      junctions to nets without geometry search.
    seq:
      - id: forward_relationships
        type: section49_relationship_array(1)
      - id: reverse_relationships
        type: section49_relationship_array(0)

  section46_route_rule_state_array:
    params:
      - id: num_records
        type: u4
    seq:
      - id: states
        type: s4
        repeat: expr
        repeat-expr: num_records
        doc: per-route-rule saved state; -2 marks an unassigned rule

  section49_relationship_array:
    params:
      - id: is_forward
        type: u1
    seq:
      - id: num_relationships
        type: u4
      - id: relationships
        type: section49_relationship(is_forward)
        repeat: expr
        repeat-expr: num_relationships

  section49_relationship:
    params:
      - id: is_forward
        type: u1
    seq:
      - id: object_id
        type: u4
        valid:
          expr: '(_ & 0xff000000) == (is_forward != 0 ? 0x3c000000 : 0x18000000) and (_ & 0x00ffffff) < (is_forward != 0 ? _root.directory[60].count : _root.directory[24].count)'
        doc: saved tagged object identifier; 0x3c identifies a section-60 junction ordinal
      - id: num_values
        type: u4
      - id: values
        type: u4
        valid:
          expr: '(_ & 0xff000000) == (is_forward != 0 ? 0x18000000 : 0x3c000000) and (_ & 0x00ffffff) < (is_forward != 0 ? _root.directory[24].count : _root.directory[60].count)'
        repeat: expr
        repeat-expr: num_values
        doc: saved tagged relationship members; 0x18 identifies section-24 route-chain ordinals

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
        size: 14
        doc: |
          Retained 14-byte outline-name storage. Live records contain NUL-padded
          POR... or ANP... ASCII names; free/capacity records may retain arbitrary bytes.
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
  # SECTION 59 — circular route-style/header object array
  # =========================================================================
  sec59_route_header_ring:
    params:
      - id: num_records
        type: u4
      - id: record_stride
        type: u4
      - id: logical_phase
        type: u4
    doc: |
      Circular record array. Logical record zero begins at physical +12 in
      v0x2024, +4 in v0x2025 and later, and +16 through v0x2022. These phases
      follow directly from the versioned serialized class layout.
    seq:
      - id: final_record_tail
        size: logical_phase
      - id: complete_records
        type:
          switch-on: _root.version
          cases:
            0x2024: sec59_route_header_v2024
            0x2025: sec59_route_header_modern
            0x2026: sec59_route_header_modern
            0x2027: sec59_route_header_modern
            _: sec59_route_header_legacy
        repeat: expr
        repeat-expr: num_records - 1
      - id: final_record_head
        size: record_stride - logical_phase

  sec59_route_header_modern:
    seq:
      - id: self_handle
        type: u4
      - id: link_handle
        type: u4
      - id: class_tag
        type: u4
        doc: 0x2001 route style/header; other values are allocator object classes
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

  sec59_route_header_v2024:
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

  sec59_route_header_legacy:
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
        if: num_heap_objects > 0
      - id: route_junctions
        type: 'sec60_route_junction_ring(num_route_junctions, route_junction_stride)'
        if: num_route_junctions > 0
      - id: object_handles
        type: sec61_object_handle
        repeat: expr
        repeat-expr: num_object_handles
        if: num_object_handles > 0
      - id: route_objects
        type: 'sec62_route_object_ring(num_route_objects, route_object_stride)'
        if: num_route_objects > 0
      - id: route_layers
        type: sec63_route_layer
        repeat: expr
        repeat-expr: num_route_layers
        if: num_route_layers > 0
      - id: route_cells
        type: sec64_route_coord_pool
        repeat: expr
        repeat-expr: num_route_cells
        if: num_route_cells > 0

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
  # The physical section begins at logical record zero's X field: byte 33 in
  # modern 64-byte records and byte 17 in legacy 48-byte records. The prefix
  # before X is rotated to the physical end. Both dialects share
  # tail-relative fields: X=stride-31, Y=stride-27, section-29 route-object
  # handle=stride-23, via definition=stride-7, type=stride-4, and net
  # index=stride-3. Head bytes +1/+2 carry the previous logical junction's
  # route-transition layers; mask each with 0x1f because bit 0x20 is saved state.
  sec60_route_junction_ring:
    params:
      - id: num_records
        type: u4
      - id: record_stride
        type: u4
    doc: X-field-left-rotated physical storage for the logical junction records
    seq:
      - id: first_record_tail
        type: sec60_route_junction_tail
        doc: logical record zero from X through its final byte
      - id: subsequent_records
        type:
          switch-on: record_stride
          cases:
            64: sec60_route_junction_v64
            48: sec60_route_junction_v48
        repeat: expr
        repeat-expr: num_records - 1
      - id: first_record_head
        type:
          switch-on: record_stride
          cases:
            64: sec60_route_junction_head_v33
            48: sec60_route_junction_head_v17
        doc: logical record zero state preceding X, rotated to the physical end

  sec60_route_junction_head_v33:
    seq:
      - id: previous_junction_class_state
        type: u1
        doc: 0x17 for a preceding 0x0e junction that carries a physical via
      - id: previous_transition_start_layer_raw
        type: u1
        doc: previous logical junction's routed start layer in low five bits; bit 0x02 marks a physical via and bit 0x20 is saved state
      - id: previous_transition_end_layer_raw
        type: u1
        doc: previous logical junction's routed end layer in low five bits; bit 0x20 is saved state
      - id: previous_transition_flags
        type: u1
      - id: object_link_state
        size: 29
        doc: saved junction object links, allocator handles, and state preceding coordinates
    instances:
      previous_carries_physical_via:
        value: 'previous_junction_class_state == 0x17 and (previous_transition_start_layer_raw & 0x02) != 0'
        doc: distinguishes a physical via from other 0x0e layer-transition junctions

  sec60_route_junction_head_v17:
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

  sec60_route_junction_tail:
    doc: common final 31 bytes of a legacy or modern route-junction record
    seq:
      - id: x_raw
        type: s4
      - id: y_raw
        type: s4
      - id: route_object_handle
        type: u4
        doc: nonzero value joins section 29 and its section-27 layer group
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

  sec60_route_junction_v64:
    doc: modern 64-byte route-junction/via record
    seq:
      - id: object_state
        type: sec60_route_junction_head_v33
        doc: serialized object handles, links, and the preceding junction's route transition
      - id: x_raw
        type: s4
        doc: RAW X coordinate
      - id: y_raw
        type: s4
        doc: RAW Y coordinate
      - id: route_object_handle
        type: u4
        doc: |
          Saved route-object handle. Zero means no layer object; every nonzero
          value is present in section 29. Section-27 group counts therefore
          assign this junction's copper layer directly. This join holds for all
          154,405 nonzero values in 327,291 section-60 records across the 597
          unique corpus binaries, including v0x2017 and v0x2019.
      - id: relationship_state
        size: 12
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
        type: sec60_route_junction_head_v17
      - id: x_raw
        type: s4
      - id: y_raw
        type: s4
      - id: route_object_handle
        type: u4
        doc: nonzero value joins section 29 and its section-27 layer group
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
  # Direct 12-byte records. The old 64-byte logical framing was a false phase
  # imposed by the accumulated directory offset.
  sec61_object_handle:
    doc: |
      12-byte route allocator node. Its own handle is implicit from its page in
      section61_allocator_page_descriptors and its 56-byte page ordinal;
      section-29 stores those handles in per-layer groups, with zero representing
      a null allocator handle. The first two words are saved graph links and the
      third is an object-class tag; no geometry is stored here. For a route-object
      node, walking incoming references breadth-first to the first depth containing
      section-60 junction handles yields that object's section-49 signal; multiple
      signals at that depth are corrupt. Bit 0x00800000 selects the single node that introduces each
      section-62 route object. Walking section-29 by the section-27 per-layer
      counts and selecting this bit produces exactly section-62.count nodes on
      every corpus file. That sequence assigns route layers to section-62's
      cell-consumption order [last object, object 0, ..., object n-2].
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
  # The section is a ring rotated left by 32 physical bytes: physical +0 is the
  # final record's 32-byte tail, logical record 0 starts at physical +32, and the
  # final record's remaining head closes the section. Modern logical records are 48 bytes;
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
      - id: bound_lo
        type: s4
        doc: '+24 cached low variable-axis bound of the complete route/jumper/via object'
      - id: bound_hi
        type: s4
        doc: '+28 cached high variable-axis bound of the complete route/jumper/via object'
      - id: flags
        type: u4
        doc: |
          +32 PADS ROUTE flags. Bit 0x100 is the serialized jumper-object bit and
          bit 0x1000 is the via/special-object bit used by the flat reader and
          ASCII ROUTE writer. Remaining bits preserve thermal, teardrop, and
          routing state. Ordinary routed copper has both class bits clear.
      - id: cell_count
        type: u4
        doc: '+36 exact number of section-64 cells owned by this object'
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
        doc: cached low variable-axis bound of the complete route/jumper/via object
      - id: bound_hi
        type: s4
        doc: cached high variable-axis bound of the complete route/jumper/via object
      - id: style
        type: u4
        doc: |
          PADS ROUTE flags; 0x100 selects a jumper object and 0x1000 selects a
          via/special object. Ordinary routed copper has both class bits clear.
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
        size: 32
      - id: preceding_records
        type:
          switch-on: record_stride
          cases:
            36: sec62_route_object_v36
            48: sec62_route_object_v48
        repeat: expr
        repeat-expr: num_records - 1
      - id: final_record_head
        size: record_stride - 32

  sec63_route_layer:
    doc: serialized route-layer ordinal; the array is a permutation of active layer indices
    seq:
      - id: layer_index
        type: u2

  # =========================================================================
  # SECTION 64 — route coordinate pool
  # =========================================================================
  # Direct 12-byte geometry cells. For an ordinary section-62 route object, the
  # three values encode two points sharing one axis. The per-layer section-69
  # routing direction selects their order: V stores direct
  # (first_major,shared_minor)=(X,Y); H and NO_PREFERENCE store the transposed
  # form. Jumper (flags&0x100) and via/special (flags&0x1000) objects use their
  # owned cells for auxiliary object geometry/state instead of routed-copper
  # polylines. An ordinary object whose only cell has identical first/second
  # major values is a retained route endpoint, not a copper segment. Section-62
  # cell_count values partition this pool exactly.
  sec64_route_coord_pool:
    seq:
      - id: first_major_or_aux_word0
        type: s4
      - id: shared_minor_or_aux_word1
        type: s4
      - id: second_major_or_aux_word2
        type: s4

  section65_saved_group_record:
    doc: |
      28-byte saved GROUP object. Tag-65 contains 34 records in 33 unique corpus
      files. All 28 records with paired exports reproduce the ASCII GROUP DATA
      names exactly; observed names include BUS_H, DDR3_ADDR_CTRL1/2, Group1, and
      OUT. The first three words retain the
      group's process-local list state. The final 16-byte fixed-capacity string
      stores the NUL-terminated group name; bytes after the terminator are retained
      string-slot state, not file padding. The OUT records demonstrate this: their
      tail retains three 0x00037cf8 words while the active name remains OUT.
    seq:
      - id: saved_group_state
        type: u4
        valid: 0
        doc: serialized group controller state; zero on every corpus record
      - id: saved_member_head_handle
        type: u4
        doc: process-local group member-list head; null when no saved list is retained
      - id: saved_member_tail_handle
        type: u4
        doc: process-local group member-list tail; null when no saved list is retained
      - id: name_storage
        type: strz
        encoding: ASCII
        size: 16
        doc: active GROUP name followed by retained fixed-slot capacity

  section65_66_compact_net_class:
    doc: |
      28-byte saved-class object used through v0x2022. SI5338-EVB proves the
      fixed eight-byte name and saved class handle: its 40 section-23 members
      reference this handle at legacy net-record +84, and section-67 clearance
      relationships reference the same handle on layers 1, 3, and 6.
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

  section65_66_net_class:
    doc: |
      280-byte saved net-class object. Present only when directory section 66's
      page descriptors retain records. All 148 modern corpus records have an
      inline 48-byte name. The 216-byte object body retains membership/rule-list
      capacity: most words are zero, while four records retain live handles or
      rule values. The final two words are serialized controller state, not
      padding; the last word is zero on 14 records and 0x80000000 on 134.
    seq:
      - id: class_flags
        type: u4
        doc: zero, 0x40000000, or 0x80000000 in the corpus
      - id: saved_class_handle
        type: u4
        doc: process-local net-class object handle; null on the terminal class in the MMSP pair
      - id: name
        type: strz
        encoding: ASCII
        size: 48
      - id: retained_membership_and_rule_state
        type: u4
        repeat: expr
        repeat-expr: 54
        doc: saved member-list and rule-object capacity; zero where no state is retained
      - id: saved_controller_handle
        type: u4
      - id: controller_state
        type: u4
        doc: zero or 0x80000000 serialized object state

  # =========================================================================
  # SECTION 67 — design-rule relationship graph
  # =========================================================================
  # Physical storage rotates the logical relationship record right by one u32:
  # rule_kind is written first, followed by the saved relationship handle, two
  # scope type/reference pairs, and layer/state. This layout validates every
  # section-67 record in all 597 unique corpus files; the former seven-coordinate
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
        doc: |
          first scope enum; observed values are 3 default, 0x17/0x18 object scopes,
          0x41 saved-class dialect, 0x42 net class, and 0x4a extended object scope
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
  # The board-setup MAXIMUMLAYER field gives the active copper-record count after
  # the aggregate record. Retained records beyond that count can keep nonzero
  # copper thicknesses. The array starts 12 bytes after section 69's direct
  # physical controller boundary.
  sec69_layer_record:
    params:
      - id: num_colors_misc
        type: u4
    doc: version-sized layer definition + physical stackup + display-color record
    seq:
      - id: name
        size: 24
        doc: '+0 NUL-padded layer name in live records; final retained-capacity records can contain arbitrary bytes'
      - id: layer_state0
        type: s4
        doc: '+24 layer-controller state; normally zero'
      - id: layer_state1
        type: s4
        doc: '+28 layer-controller state; normally zero'
      - id: routing_dir
        type: s4
        doc: |
          +32 ROUTING_DIRECTION: 0=H, 1=V, 2=NO_PREFERENCE, 3=45, 4=-45. Section-64
          route cells on this layer use (shared_minor, first_major/second_major)
          as (X,Y) for H/NO_PREFERENCE/45/-45 and
          (first_major/second_major, shared_minor) for V.
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
        doc: '+116 through v0x2021, +124 in v0x2022, +140 thereafter; packed attribute bitfield (bits0-2 routable/visible/selectable)'
      - id: layer_state2
        type: s4
        doc: '+120 through v0x2021, +128 in v0x2022, +144 thereafter; layer-controller state; final record may retain allocator contents'
      - id: next_layer_type
        type: s4
        enum: layer_type
        doc: |
          +124 through v0x2021, +132 in v0x2022, +148 thereafter. Final word is
          the following record's ASCII LAYER_TYPE: 0=UNASSIGNED,
          1=ROUTING, 2=DRILL, 3=SILK_SCREEN, 4=PASTE_MASK, 5=SOLDER_MASK,
          6=ASSEMBLY. The final physical record has no successor, so this word is
          retained carrier state. Verified against paired exports for DC607A,
          Ems4_Rev2, and MC4_PLUS_CSHAPE; the one-record lag reproduces every
          layer type, including customized layer numbering.

  sec69_layer_record_array:
    params:
      - id: num_colors_misc
        type: u4
      - id: num_records
        type: u4
    seq:
      - id: records
        type: 'sec69_layer_record(num_colors_misc)'
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
