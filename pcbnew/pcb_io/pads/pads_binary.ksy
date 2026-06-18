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
# u32 size check.

meta:
  id: pads_pcb_binary
  title: PADS PowerPCB binary layout (.pcb)
  file-extension: pcb
  endian: le
  encoding: ASCII

doc: The purpose of this field is not known.
seq:
  - id: magic
    contents: [0x00, 0xff]
    doc: file magic 00 FF
  - id: version
    type: u2
    doc: DB format version 0x2021 / 0x2022 / 0x2024 / 0x2025 / 0x2026 / 0x2027
  - id: header_extra
    size: 6
    doc: |
      6 bytes after the version (`02 00 00 00 00 00`); extra[0]=0x02 is a
      capability/flag byte, the rest reserved zero.
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
      arrived in v0x2025. Verified on 164/164 corpus files by *PCB* board-setup
      block alignment -- with the stored count every file lands on a 48-byte
      boundary, where a 73/74 version table leaves v0x2022 and v0x2024 off by 32.

      Guard before trusting it: section1.total_bytes == section1.count * 16.
      That guard holds on 663 of 663 boards across the whole corpus, so the
      directory always declares its own byte size and the count can be read
      without a version branch at all.

      Re-measured on the full 663-board corpus, the stored count by version is

          0x2017   72     6 boards      NOT 73 -- no version rule predicts this
          0x2019   73     1 board
          0x2021   73    38 boards
          0x2022   73    13 boards
          0x2024   73    17 boards
          0x2025   74    34 boards
          0x2026   74   118 boards
          0x2027   74   436 boards

      v0x2017 is what settles "stored versus implied": it declares 72, a value
      no 73/74 version table can produce. (The reader rejects 0x2017 and 0x2019
      as unsupported, so those seven boards are evidence about the container
      only.) Some v0x2026/v0x2027 boards declare 75 slots, i.e. entry_count 74.

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

types:

  # =========================================================================
  # CONTAINER
  # =========================================================================
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

        i.e. 1232 on 74-entry files and 1216 on 73-entry ones. Verified on
        165/165 corpus files: the decal table's `JMPVIA_AAAAA` lands exactly on
        `payload_offset(14) + 44`, with no near misses.

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
        size: 8
        doc: |
          The controller's in-memory base address, zero on all but two corpus
          files. Where present the addresses chain exactly with the declared
          sizes. These are MEMORY addresses and sections are not allocated in
          file order, so they must never be treated as file offsets.

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

      e.g. 30274+35 = 30309, +25 = 30334, +28 = 30362 ... and entry 0 has
      pool_offset 0 with length 25, matching the pool's opening
      `DFT_CONFIGURATION\0PARENT\0`.

      LOCATING THE POOL. Do not accumulate to it and do not scan for it. The
      terminator entry -- the one satisfying

          pool_offset + length == section57.total_bytes

      is unique, and the pool begins 17 bytes past its start:

          pool_start = terminator_entry_offset + 16
          pool_end   = pool_start + section57.total_bytes

      i.e. the pool begins immediately after the terminator's own 16-byte entry.
      An earlier draft of this spec said +17, from measuring the first PRINTABLE
      byte and so skipping a leading NUL. The +16 form is the correct one: every
      section 60 base correction measured against it is an exact multiple of 16,
      where +17 left them all congruent to -1.

      This validates on 162 of 163 corpus files. Two guards must NOT be added,
      both of which were measured and both of which break it:

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
      - id: flag
        type: u1
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
        doc: 0xFFFF when unused
      - id: sentinel
        type: u2
        doc: 0xFFFF
      - id: padding
        size: 3

  footer:
    doc: trailing integrity marker after the last section's data
    seq:
      - id: guid
        type: str
        size: 38
        encoding: ASCII
        doc: footer GUID {2FE18320-6448-11d1-A412-000000000000}
      - id: size_check
        type: u4
        doc: file size check word

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
      - id: reserved
        size: 8
        doc: always zero

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
  # Measured over all 663 boards: directory[2].count equals the observed prefix
  # length in 48-byte units on 663/663. 570 boards have no prefix at all; the
  # rest run 1..22 records and every one matches its declared count exactly.
  #
  # Against the field-range search this replaces (SCALE / BACKUPTIME / REAL WIDTH
  # / ALLSIGONOFF / REFNAMESIZE swept byte-by-byte over section 1): they agree on
  # all 633 boards where both resolve, disagree on none, and the structural rule
  # additionally resolves 7 boards the search missed. Those five fields are now
  # a validator -- a file whose layout differs yields no origin rather than a
  # plausible-looking wrong one.
  #
  # This matters more than one section: the origin at +60/+64 shifts every
  # absolute coordinate in the file, so a wrong base moves the whole board.
  sec1_board_setup:
    seq:
      - id: reserved0
        type: s4
        repeat: expr
        repeat-expr: 3
        doc: off 0/4/8 reserved zero
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
      - id: reserved88
        type: s4
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
      - id: reserved120
        type: s4
      - id: feature_flags
        type: u4
        doc: off124 0x094D4000 constant (DB-version/capability flags)
      - id: reserved128
        size: 16
        doc: off128..143 zero
      - id: job_name
        type: strz
        encoding: ASCII
        doc: off144 JOBNAME (e.g. "BR350430B.pcb")
      - id: autosave_name
        type: strz
        encoding: ASCII
        doc: 'autosave filename YY_MM_DD_HH_MM_SS.pcb, packed right after job_name'
      - id: pad
        size-eos: true
        doc: zero padding to 1200 bytes

  # =========================================================================
  # SECTION 3 — board-parameter blob (single 3932-B blob)
  # =========================================================================
  # Most words are reserved zero. Tail (2788..3931) is an embedded *VIA*
  # padstack pad-shape heap (18 x 64B).
  sec3_board_params:
    seq:
      - id: reserved_head
        size: 832
        doc: 0..831 all zero
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
      - id: gap1
        size: 128
        doc: 860..987 zero
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
      - id: reserved1172
        type: s4
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
      - id: reserved1252
        size: 8
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
      - id: reserved1296
        type: s4
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
      - id: gap2
        size: 1300
        doc: 1424..2723 zero
      - id: assoc_net_count
        type: s4
        doc: 2724 ASSOCIATEDNETNETCOUNT
      - id: assoc_plane_pin_count
        type: s4
        doc: 2728 ASSOCIATEDNETPLANEPINCOUNT
      - id: osnap
        type: s4
        doc: 2732 OSNAP
      - id: osnap_rad
        type: s4
        doc: 2736 OSNAPRAD
      - id: via_header_rec
        size: 48
        doc: 2740..2787 handle-less via stack header record
      - id: via_stack_records
        type: sec3_via_stack_rec
        repeat: eos
        doc: '2788.. embedded *VIA* padstack pad-shape heap (18 x 64B; last truncated)'

  sec3_via_stack_rec:
    doc: 64-byte embedded via padstack pad-shape record
    seq:
      - id: pre
        size: 12
        doc: zero
      - id: handle
        type: u4
        doc: '+12 object id (handle)'
      - id: reserved16
        type: s4
      - id: pad_size
        type: s4
        doc: '+20 pad SIZE (outer diameter)'
      - id: drill_or_inner
        type: s4
        doc: '+24 drill / thermal inner'
      - id: thermal_outer
        type: s4
        doc: '+28 thermal outer dia / extra'
      - id: extra1
        size: 12
        doc: '+32..+43 thermal spoke-width / 0'
      - id: level_index
        type: s4
        doc: '+44 level/line ordinal (x2 ramp)'
      - id: level_shape_flags
        type: u4
        doc: '+48 0x000L0SFE: L=layer-level group, low byte = shape (0xFE round)'
      - id: corner_or_angle
        type: s4
        doc: '+52 thermal angle (162e6 = 45deg*3.6e6) or spoke count'
      - id: tail
        size: 8
        doc: '+56..+63 mostly zero'

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
      - id: reserved0
        type: s4
        doc: '+16 always 0'
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
      - id: reserved0
        type: s4
        doc: '+16 idx4 == 0'
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
      - id: reserved1
        type: s4
        doc: '+56 idx14 == 0'
      - id: bbox_x
        type: s4
        doc: '+60 idx15 first-glyph corner X (RAW)'
      - id: bbox_y
        type: s4
        doc: '+64 idx16 first-glyph corner Y (RAW)'
      - id: object_id_1
        type: s4
        doc: '+68 idx17 object id / hash'

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
      - id: raw
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
      - id: reserved_hi
        type: u2
        doc: '+2 reserved / handle hi-bits'
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

  sec11_vertex:
    doc: TAIL inline (marker, X, Y) i32 triple; coords are DESIGN (no origin shift)
    seq:
      - id: marker
        type: s4
        doc: -1 = contour vertex; {0,1,2,3} = corner/arc-type code
      - id: x_design
        type: s4
      - id: y_design
        type: s4

  # =========================================================================
  # SECTION 13 — padstack/decal extent + copper-pour hatch-fill geometry
  # =========================================================================
  # Two regions. PREFIX: 20 B copper-pour hatch-fill segments (pour-LOCAL
  # coords). TAIL: 112 B padstack/decal extent records (inline name, units byte,
  # stack ordinal, decal-LOCAL symmetric bbox). Neither origin-shifted.
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

  sec13_stack_extent:
    doc: 112-B padstack/decal extent record (bbox decal-LOCAL, NOT origin-shifted)
    seq:
      - id: pool_off0
        type: s4
        doc: '+0 running pool offset / index'
      - id: piece_count
        type: s4
        doc: '+4 piece/primitive count of this stack'
      - id: pool_idx_a
        type: s4
        doc: '+8 cumulative geometry/terminal pool index'
      - id: pool_idx_b
        type: s4
        doc: '+12 second cumulative pool index'
      - id: heap_handle
        type: u4
        doc: '+16 heap object handle (0 for plain via)'
      - id: stackline_count
        type: s4
        doc: '+20 stack-line / sub-shape count'
      - id: rsv0
        type: s4
        doc: '+24 = 0'
      - id: rsv1
        type: s4
        doc: '+28 = 0'
      - id: misc0
        type: s4
        doc: '+32 mostly 0'
      - id: save_token
        type: s4
        doc: '+36 per-save token'
      - id: rsv2
        type: s4
        doc: '+40 = 0'
      - id: name
        type: str
        size: 40
        encoding: ASCII
        doc: '+44 inline padstack/decal NAME, NUL-padded'
      - id: units_lo
        type: u1
        doc: '+84 = 0x00'
      - id: units
        type: u1
        doc: "+85 UNITS flag 'M'=metric / 'I'=imperial"
      - id: flag_a
        type: u2
        doc: '+86 flag/restriction high bits'
      - id: stack_ordinal
        type: s4
        doc: '+88 stack ordinal (0,1,2,...)'
      - id: bbox_xmin
        type: s4
        doc: '+92 bbox X-min (decal-LOCAL)'
      - id: bbox_ymin
        type: s4
        doc: '+96 bbox Y-min'
      - id: bbox_xmax
        type: s4
        doc: '+100 bbox X-max (xmax-xmin = via max pad size)'
      - id: bbox_ymax
        type: s4
        doc: '+104 bbox Y-max'
      - id: sentinel
        type: u2
        doc: '+108 0xFFFE record sentinel'
      - id: flag_b
        type: u2
        doc: '+110 flag high half'

  # =========================================================================
  # SECTION 14 — PARTDECAL terminal-run descriptors
  # =========================================================================
  # 112 B descriptor records with sentinel 0xFFFE at +108 and decal name at +44.
  # Start index into the sec15 terminal-position pool is i32 @+0. The terminal
  # count for descriptor K is stored in descriptor K+1 @+4 (one-record lag). The
  # same lagged count stream also appears across sections 13 and 12 for decals
  # without a section-14 descriptor. Descriptor/count reads may cross directory
  # section boundaries.
  sec14_terminal_desc:
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
      - id: tail
        size: 20
        doc: '+88..+107 descriptor tail / bbox flags'
      - id: sentinel
        type: u2
        doc: '+108 0xFFFE (may cross section boundary)'
      - id: flag_b
        type: u2
        doc: '+110 flag high half'

  # =========================================================================
  # SECTION 15 — PARTDECAL terminal (pin) position table
  # =========================================================================
  # 36 B/record geometry, then a fixed 33-record (1188 B) decal-object-dictionary
  # trailer. count == n_geom + 33. Coords are DECAL-LOCAL design (not
  # origin-shifted) PARTDECAL terminal XLOC/YLOC.
  sec15_terminal:
    seq:
      - id: x
        type: s4
        doc: '+0 pin X, decal-local design'
      - id: y
        type: s4
        doc: '+4 pin Y, decal-local design'
      - id: name_x
        type: s4
        doc: '+8 NMXLOC (== x in this corpus)'
      - id: name_y
        type: s4
        doc: '+12 NMYLOC (== y)'
      - id: padstack_ptr
        type: u4
        doc: '+16 padstack object heap pointer'
      - id: pin_name_cache
        size: 4
        doc: '+20 stale ASCII pin-name cache, often empty/partial'
      - id: rsv0
        type: s4
        doc: '+24 0 (block-end discriminator)'
      - id: rsv1
        type: s4
        doc: '+28 0'
      - id: rsv2
        type: s4
        doc: '+32 0'

  # =========================================================================
  # SECTIONS 16 & 17 — PARTTYPE definitions (rotated 224-B slot stream)
  # =========================================================================
  # The parttype stream spans sec15 tail -> sec16 -> sec17. Each 224-B slot =
  # [112 B 0xFF pad][112 B payload]. Slot-rotated: NAME of parttype K is in slot
  # K @+44; its TYPE/FLAGS/GATES are in slot K+1 @+16/+17/+20. sec17 count ==
  # ASC PARTTYPE count.
  parttype_slot:
    doc: 224-byte rotated PARTTYPE slot (shared by sec16 and sec17)
    seq:
      - id: pad_lead
        size: 112
        doc: 0xFF filler (trailing pad of previous record)
      - id: payload
        type: parttype_payload

  parttype_payload:
    doc: |
      fields are slot-rotated: name belongs to THIS slot; flags/type/gates
      describe the PREVIOUS slot's name (read from this slot for name[K-1]).
    seq:
      - id: obj_ordinal
        type: s4
        doc: '+0 0-based PARTTYPE ordinal; continues across sections'
      - id: pin_cursor
        type: s4
        doc: '+4 cumulative offset into the sec19 pin table; pins=cursor[K+2]-cursor[K+1]'
      - id: reserved0
        size: 8
        doc: '+8 zero'
      - id: flags_of_prev
        type: u1
        doc: '+16 ASC FLAGS of the PREVIOUS slot name'
      - id: type_of_prev
        type: str
        size: 3
        encoding: ASCII
        doc: '+17 ASC TYPE of the PREVIOUS slot name (RES/CAP/CON/IC/...)'
      - id: gates_of_prev
        type: s4
        doc: '+20 ASC GATES of the PREVIOUS slot name'
      - id: reserved1
        size: 12
        doc: '+24 zero (LABEL spillover text in special slots)'
      - id: obj_handle
        type: u4
        doc: '+36 object handle/ID, +1 per slot (handle)'
      - id: state_flag
        type: u4
        doc: '+40 heap/object-state bit (0 / 0x80000000)'
      - id: name
        type: str
        size: 36
        encoding: ASCII
        terminator: 0
        doc: '+44 PARTTYPE NAME, NUL-terminated inline ASCII'
      - id: reserved2
        size: 12
        doc: '+80 zero (sec17: decal_flag 0/1 at +80)'
      - id: heap_ptr
        type: s4
        doc: '+92 live heap pointer (~612-byte stride)'
      - id: decal_index
        type: s4
        doc: '+96 per-file decal/footprint table index'
      - id: decal_index_dup
        type: s4
        doc: '+100 duplicate of decal_index'
      - id: pad_tail
        size: 8
        doc: '+104 0xFF (start of next slot pad_lead)'

  # =========================================================================
  # SECTION 18 — volatile object-reference pool
  # =========================================================================
  # 88 B/record, phase-shifted (sniff first [\x00\x01]U[0-9] marker; record_start
  # = marker-5). Names can be stale (refdes of parts that no longer exist).
  sec18_obj_ref:
    seq:
      - id: rsv0
        type: u4
        doc: '+0 always 0'
      - id: flag
        type: u1
        doc: '+4 0=stale/leftover, 1=fresh/active'
      - id: name
        type: strz
        encoding: ASCII
        size: 3
        doc: '+5 part refdes, may be stale'
      - id: rsv1
        size: 68
        doc: '+8..+75 always 0 (no active object state)'
      - id: heap_tail
        size: 12
        doc: '+76..+87 uninitialized heap slack; leaks unrelated strings'

  # =========================================================================
  # SECTION 19 — PARTTYPE pin-definition table
  # =========================================================================
  # 88 B/record, one per pin per parttype (not placed-pin instances). count ==
  # ASC PARTTYPE pin count. Bytes 0..43 are uninitialized scratch. This section
  # can also physically precede/carry FEFF-delimited placement tail records that
  # use the sec22 placement layout, which may cross the directory boundary into
  # the next physical section.
  sec19_pin:
    seq:
      - id: reserved_scratch
        size: 44
        doc: '+0..+43 uninitialized writer scratch (stale heap bytes)'
      - id: name_absent_flag
        type: u1
        doc: '+44 ~1 when pin_name empty (may encode PINTYPE)'
      - id: pin_id
        type: strz
        encoding: ASCII
        size: 17
        doc: '+45 SWAPTYPE letter + pin number (e.g. "U1","L2","P8","G12")'
      - id: pin_name
        type: strz
        encoding: ASCII
        size: 26
        doc: '+62 pin/signal name (empty for unnamed pins)'

  # =========================================================================
  # SECTION 22 — part placements
  # =========================================================================
  # 112 B/record. Only the first count-11 records are real placements; the
  # trailing 11 slots are an appended net-name sub-table. X@60/Y@64 RAW; refdes
  # inline @44.
  sec22_part_placement:
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

  # =========================================================================
  # CLUSTER TABLE — part clusters (.asc *CLUSTER* groups)
  # =========================================================================
  # 60 B/record, appended in the directory-covered tail of the late route-coord
  # section (attributed to sec64 or sec69 depending on layout). Records are in
  # .asc *CLUSTER* order, so a record's 1-based ordinal IS the CLSTID that the
  # sec22 placement +108 field references. The cluster NAME is also duplicated as
  # a decoy NUL-separated run in the sec8 string pool; the real table is the one
  # whose +16/+20 decode as valid RAW coords (the decoy has garbage there).
  cluster_record:
    seq:
      - id: name
        type: strz
        encoding: ASCII
        size: 16
        doc: '+0 cluster NAME, NUL-padded'
      - id: x_raw
        type: s4
        doc: '+16 XLOC RAW; design = raw - origin_x (BASIC = 1/38100 mil)'
      - id: y_raw
        type: s4
        doc: '+20 YLOC RAW; design = raw - origin_y'
      - id: parent_id
        type: s4
        doc: '+24 PARENTID/CLUSTERID (0)'
      - id: attribute
        type: s4
        doc: '+28 low 16 bits = .asc ATTRIBUTE'
      - id: flag1
        type: s4
        doc: '+32 flag (1)'
      - id: zero36
        type: s4
        doc: '+36 0'
      - id: handle40
        type: u4
        doc: '+40 handle'
      - id: zero44
        type: s4
        doc: '+44 0'
      - id: zero48
        type: s4
        doc: '+48 0'
      - id: handle52
        type: u4
        doc: '+52 handle'
      - id: trailing_marker
        type: s4
        doc: '+56 trailing marker (NOT the .asc CHILD_NUM)'

  # =========================================================================
  # SECTION 23 — net records
  # =========================================================================
  # 424 B/record. Parse until name == "___Unassigned_Obstacles_" (sentinel);
  # records after it are mis-aligned padding. Holds every named net once. No
  # geometry. Tail @200..423 is serialized-object padding.
  sec23_net:
    seq:
      - id: reserved0
        size: 92
        doc: '+0 always zero'
      - id: plane_index
        type: s4
        doc: '+92 -1 normal signal; >=1 1-based plane-assignment index'
      - id: sig_flag
        type: s4
        doc: '+96 raw PADS SIGFLAG'
      - id: conn_count
        type: s4
        doc: '+100 number of connections (= sec24 entries for this net)'
      - id: anchor_part_idx
        type: s4
        doc: '+104 0-based index into sec22 of a member part'
      - id: anchor_pin
        type: s4
        doc: '+108 terminal/pin number on anchor part (decal terminal index)'
      - id: sec24_start
        type: s4
        doc: '+112 cumulative start index into sec24 chain topology'
      - id: name
        type: strz
        encoding: ASCII
        size: 48
        doc: '+116 net name, NUL-terminated'
      - id: ser_index
        type: s4
        doc: '+164 serialized object index/handle'
      - id: reserved1
        type: s4
        doc: '+168 0'
      - id: ser_size_used
        type: s4
        doc: '+172 serialized byte-size of connection/route blob'
      - id: ser_size_cap
        type: s4
        doc: '+176 allocation capacity (rounded up)'
      - id: reserved2
        type: s4
        doc: '+180 usually 0'
      - id: net_self_ptr
        type: u4
        doc: >
          +184 the net object's own in-file CObject id (low dword of the old
          "heap_ptr0" u8). Stable within one file; used as the diff-pair member
          key: a sec49 DIF_PAIR object's member-net ptrs (+12/+16) value-equal this.
      - id: netclass_owner_ptr
        type: u4
        doc: >
          +188 the net's NET_CLASS owner object id (high dword of the old
          "heap_ptr0" u8). THIS IS THE NET->CLASS MEMBERSHIP KEY: all nets of a
          class share this value; 0 = unclassed. Grouping nets by it reproduces
          the ASC NET_CLASS membership exactly (126/126, 8/8, 1/1 across the
          corpus). Ascending distinct values == net-class declaration order ==
          the 280-byte NAME-table file order, so class_ordinal = rank(this).
          Value-joinable within one file; never dereferenced.
      - id: heap_ptr1
        type: s4
        doc: '+192 serialized live heap pointer / allocator junk'
      - id: conn_count_dup
        type: s4
        doc: '+196 duplicate of conn_count'
      - id: ser_tail
        size: 224
        doc: '+200..+423 serialized-object remainder + zero padding; no design data'

  # =========================================================================
  # SECTION 24 — route chain / pin-pair connection topology
  # =========================================================================
  # 68 B/record, one per routed pin-pair connection.
  # Real records = count - 17 (trailing 17 are allocator slack). marker
  # 0xFE000000@20 and flag 0xFFFE@52 are the validity sentinels.
  sec24_route_chain:
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
      - id: marker
        type: u4
        doc: '+20 chain marker == 0xFE000000'
      - id: reserved0
        size: 12
        doc: '+24..+35 always 0'
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
        doc: '+52 == 0x0000FFFE'
      - id: reserved1
        type: s4
        doc: '+56 always 0'
      - id: node_sec1
        type: s4
        doc: '+60 secondary node / via index'
      - id: node_sec2
        type: s4
        doc: '+64 secondary node / via index'

  # =========================================================================
  # SECTION 25 — fixed 72-entry object-pointer vector (heap snapshot)
  # =========================================================================
  # 288 B = 72 x u32 heap pointers into a shared 56-byte object pool (continues
  # in sec26/27). Semantic key is the slot index = (value - pool_base)/56.
  sec25_object_pointer_vector:
    seq:
      - id: slot_ptr
        type: u4
        repeat: expr
        repeat-expr: 72
        doc: 56-aligned heap pointer; logical slot = (value - pool_base)/56

  # =========================================================================
  # SECTION 26 — object-handle triple pool (heap snapshot)
  # =========================================================================
  # 12 B/record = 3 x u32 56-aligned object handles into the shared 56-byte pool.
  sec26_handle_triple:
    seq:
      - id: h0
        type: u4
        doc: 56-aligned object handle
      - id: h1
        type: u4
      - id: h2
        type: u4

  # =========================================================================
  # SECTION 27 — per-copper-layer object-handle array
  # =========================================================================
  # count == MAXIMUMLAYER. One u32 heap pointer per copper layer to a 56-byte
  # per-layer object (8-byte aligned, 56-byte grid). Stored in allocation order,
  # not layer order.
  sec27_layer_handle:
    seq:
      - id: layer_handle
        type: u4
        doc: heap pointer to the 56-byte per-layer object

  # =========================================================================
  # SECTION 29 — object-handle vector + fixed 297-int via/rule template pool
  # =========================================================================
  # Two regions: a leading object-handle pointer vector (count-297 i32, 8-byte
  # aligned absolute VAs, +56 stride; count==sec61.count on routed boards) then
  # an invariant 297-int default via/rule template pool (6x47 + 14). The pool
  # holds per-layer BASIC sizes + 12-mil(457200) drill + state; one variant
  # carries IEEE-754 f64 rule params (-1.0 = inherit).
  sec29:
    seq:
      - id: object_handles
        type: u4
        repeat: expr
        repeat-expr: _root.directory[29].count - 297
        doc: 8-byte-aligned absolute process VAs (handles; +56 stride)
      - id: default_template_pool
        size: 1188
        doc: |
          invariant 297 i32 (= 297*4 bytes). 6 x 47-int via/rule templates + 1
          partial 14-int block. Per template: handle:u4, zero:u4, layer_sizes
          i32[42] (BASIC), drill:i32(=457200), zero:u4, state:i32. Default/template
          arena, not the board's actual vias. (One variant: f64 rule params.)

  # =========================================================================
  # SECTION 49 — clearance/design-rule heap snapshot
  # =========================================================================
  # Two object kinds, both serialized into the MFC blob spanning sec49 (the
  # records sit just past the directory-declared sec49 byte range):
  #   (a) sec49_clearance_record  — 188 B, the per-rule clearance MATRIX
  #   (b) sec49_diff_pair_object  — 864 B, f64[70], the DIF_PAIR objects
  # data_offset not 4-aligned.
  #
  # KEY: the constant i32 457200 (== 12.0 mil = 12*38100 BASIC) at +0 is a real
  # field value (TRACK_TO_TRACK default), NOT a record delimiter. Anchoring on it
  # as a boundary shifts the window by one record and "scrambles" the matrix; the
  # true layout is a flat int32[38] in ASC field order at +20.
  sec49_clearance_record:
    doc: >
      188-byte per-rule clearance record. The 39 ASC CLEARANCE_RULE fields are a
      contiguous int32[38] (ASC order 0..37) at +20 plus field 38 at +4. Locate
      records by scanning FILE-WIDE for the i32 457200 marker (the records sit in
      a broader MFC blob just OUTSIDE the sec49 directory byte-range); record
      start = marker_offset (the marker is field 0). Do NOT treat 457200 as a
      record delimiter read at marker-108 -- that splices adjacent records and
      yields a false "scrambled / unbindable" matrix; the int32[38] core at +20
      is the correct, contiguous anchor.

      VALUE -> CLASS join is POSITIONAL by declaration order. The disc-1 (layer-0)
      records sorted by +12 self-ptr zip 1:1 to the type-66 clearance edges sorted
      by +0 rule-detail ptr (the two arenas have independent malloc bases, no
      pointer chain). Field-exact vs the ASC on all 3 corpus files carrying
      NET_CLASS DATA (BR350430B 1/1, BR350420B 2/2, BR350460A 20/20 (class,layer)
      keys); 460A's distinct REC track widths land on the correct class, proving
      the join is not accidental value-matching. The importer uses the disc-1 core
      for each class (clearance=core[0], track width=core[34], via clearance=
      core[2], min/max=core[33]/core[35]); disc-15 layer records repeat the disc-1
      clearance and are dropped.
    seq:
      - id: track_to_track
        type: s4
        doc: '+0 ASC TRACK_TO_TRACK (== 457200/12mil default on the schema default; the scanner anchor)'
      - id: same_net_track_to_crn
        type: s4
        doc: '+4 ASC field 38 SAME_NET_TRACK_TO_CRN'
      - id: scope_disc
        type: s4
        doc: >
          +8 discriminator: 1 = NET_CLASS layer-0 rule, 15 = NET_CLASS
          layer-specific, 2 = per-NET rule, 0 = DEFAULT/empty (T2T zeroed; the
          board-default clearance is NOT serialized here).
      - id: rule_self_ptr
        type: u4
        doc: '+12 rule object self-ptr (appears once; positional-join key, not value-joinable)'
      - id: reserved0
        type: s4
        doc: '+16 0'
      - id: clearance
        type: s4
        repeat: expr
        repeat-expr: 37
        doc: >
          +20 ASC clearance fields 1..37 in canonical order (field 0
          TRACK_TO_TRACK is at +0): VIA_TO_TRACK .. BODY_TO_BODY. Together with
          +0 and +4 this is the full 39-field matrix, BASIC units. Fields used by
          the importer: core[2]=SAME_NET_VIA_TO_VIA, core[33]=MIN_TRACK_WIDTH,
          core[34]=REC_TRACK_WIDTH, core[35]=MAX_TRACK_WIDTH.
      - id: tail
        size: 20
        doc: '+168 trailing per-record meta (75000 default-pour for disc 1/2, else 0)'
  sec49_diff_pair_object:
    doc: 864-byte DIF_PAIR object (f64[69] grid + structured i32 tail, -1.0/-1 = inherit)
    seq:
      - id: counter
        type: u4
        doc: '+0 object counter / first-flag'
      - id: reserved0
        type: u4
        doc: '+4 0'
      - id: flag_one
        type: u4
        doc: '+8 object flag == 1'
      - id: member_net_a
        type: u4
        doc: '+12 member-net A == sec23 net_self_ptr (+184); value-join to net name (positive)'
      - id: member_net_b
        type: u4
        doc: '+16 member-net B == sec23 net_self_ptr (+184); value-join to net name (negative)'
      - id: zero_pad
        size: 12
        doc: '+20 zero'
      - id: matrix
        type: f8
        repeat: expr
        repeat-expr: 69          # was 70; grid ENDS at +584
        doc: '+32 f64 override matrix (-1.0=inherit). [0]=MAX_LENGTH, [1]=min/inherit GAP, [3]=effective GAP override. GAP = [3] if != -1.0 else [1].'
      - id: max_obstacle_size
        type: s4
        doc: '+584 MAX_OBSTACLE_SIZE'
      - id: max_obstacle_number
        type: s4
        doc: '+588 MAX_OBSTACLE_NUMBER'
      - id: width_a
        type: s4
        doc: '+592 width fallback; WIDTH = width if width(+600) != -1 else width_a'
      - id: width_sep
        type: u4
        doc: '+596 0xFFFFFFFF separator'
      - id: width
        type: s4
        doc: '+600 WIDTH (effective when != -1)'
      - id: free_fill
        size: 260               # was free_fill size 268 starting +592
        doc: '+604 0xFF allocator free-space fill'

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
      declaration order, the same order as the ascending sec23 netclass_owner_ptr
      values, so the k-th name labels the k-th distinct owner_ptr. Membership
      itself comes from grouping sec23 nets by netclass_owner_ptr (+188).
    seq:
      - id: name
        type: strz
        encoding: ASCII
        size: 40
        doc: '+0 net-class name (ETH_RGMII_TX, POWER_SIGNALS, GROUND, ...)'
      - id: body
        size: 236
        doc: '+40 per-class state; +276 (next record) carries the next-name link'
  net_class_rule_record:
    doc: >
      24-byte per-class-per-layer rule record (trailing heap). One per ASC
      RULE_SET FOR { NET_CLASS x } LAYER n. Detect by (+4 == 0x42) and (+8 in the
      class band). Per-class counts and layers match the ASC exactly (30/30 on
      BR350460A). Gives rule existence + layer per class deterministically. The
      +0 rule-detail ptr is not value-joinable, but sorting the clearance-page
      edges by +0 yields NET_CLASS declaration order, which zips positionally to
      the sec49_clearance_record disc-1 records sorted by their +12 self-ptr ->
      this recovers the per-class clearance VALUE (see sec49_clearance_record).
    seq:
      - id: rule_detail_ptr
        type: u4
        doc: '+0 rule-detail object id (appears once; not value-joinable)'
      - id: tag
        type: u4
        doc: '+4 constant 0x42 record tag'
      - id: class_owner_ptr
        type: u4
        doc: '+8 net-class owner id == sec23 netclass_owner_ptr (+188); JOIN key'
      - id: const_three
        type: u4
        doc: '+12 constant 3'
      - id: const_mask
        type: u4
        doc: '+16 constant 0x03000000'
      - id: layer
        type: u4
        doc: '+20 layer index (0 = all-layer; else 1-based copper layer)'

  # =========================================================================
  # SECTIONS 49/50/52/53/54/56 — object-relationship token stream (shared)
  # =========================================================================
  # One contiguous variable-length tagged-token stream that the directory carves
  # into sec50/52/53/54/56 (and the start touches sec49). Each token is a u32 LE
  # at a per-file byte phase: tag = byte3, value = low 24 bits. tag 0x00 =
  # literal/count, 0x18 = sec24 connection-object handle, 0x3c = sec60 junction /
  # net-object handle. The directory boundary may cut a token mid-word. sec56
  # also embeds the 88-B pour headers.
  objrel_token:
    doc: |
      4-byte object-relationship token. tag is the destination section ordinal
      (0x18=24 connection, 0x3c=60 junction/net). A per-file phase applies. The
      0x18 value space is sec24 ids; the 0x3c value is bounded by sec60.count.
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

  # =========================================================================
  # SECTION 56 — embedded 88-byte POUR-HEADER table (within the token stream)
  # =========================================================================
  # Locate by scanning for the NUL-padded ASCII pour name; record starts 46 B
  # before the name. Coords RAW; tail (+64..) points into the polygon-vertex pool
  # (not inline).
  sec56_pour_header:
    seq:
      - id: bbox_xmin
        type: s4
        doc: '+0 pour XLOC / bbox x-min (RAW)'
      - id: bbox_ymin
        type: s4
        doc: '+4 pour YLOC / bbox y-min (RAW)'
      - id: bbox_x2
        type: s4
        doc: '+8 bbox x (RAW)'
      - id: bbox_x3
        type: s4
        doc: '+12 bbox x (RAW)'
      - id: bbox_y2
        type: s4
        doc: '+16 bbox y (RAW)'
      - id: bbox_y3
        type: s4
        doc: '+20 bbox y (RAW)'
      - id: reserved0
        type: s4
        doc: '+24 0'
      - id: reserved1
        type: s4
        doc: '+28 0'
      - id: hatch_rad_milli
        type: s4
        doc: '+32 hatch radius x1000'
      - id: hatch_grid
        type: s4
        doc: '+36 hatch grid'
      - id: pieces
        type: s4
        doc: '+40 PIECES'
      - id: name_prefix
        type: u2
        doc: '+44 0x8000 name-field prefix'
      - id: name
        type: str
        size: 14
        encoding: ASCII
        doc: '+46 pour name, NUL-padded'
      - id: obj_handle
        type: s4
        doc: '+56 object handle in the tag-0x18 id-space'
      - id: type_marker
        type: u4
        doc: '+60 record-type marker == 0x32000000'
      - id: tail
        size: 24
        doc: '+64..+87 count/corners/net-handle/polygon-vertex pointer'

  # =========================================================================
  # SECTION 59 — CAM_SECTION text spill + leaked 32-B heap-object stream
  # =========================================================================
  # Region A = tail of the CAM_SECTION ASCII KEY\0 VALUE\0 pool (begins in
  # sec57; aperture/drill tables). Region B = a 32-B heap-object pointer stream
  # (continues into sec60). Region B framing: first i32 == 0x00002001 is the tag
  # at +8; record base = off-8, stride 32. No board geometry.
  sec59_heap_obj:
    doc: 32-byte heap-object record (Region B); no geometry
    seq:
      - id: ptr0
        type: u4
        doc: '+0 heap pointer (arena A)'
      - id: ptr1
        type: u4
        doc: '+4 heap pointer (arena A mirror)'
      - id: tag
        type: u4
        doc: '+8 class enum {0x2001,0x2000,0x2400,0x1000,0}'
      - id: size
        type: s4
        doc: '+12 mostly 0; else BASIC clearance/width-class'
      - id: state
        type: u4
        doc: '+16 small enum/flags {0,0xFFFF,1,2,3,0xB}'
      - id: handle
        type: u4
        doc: '+20 near-monotone object handle (arena B)'
      - id: flag
        type: u4
        doc: '+24 single bit 0 or 0x80000000'
      - id: ptr7
        type: u4
        doc: '+28 heap pointer (arena C; strides +48)'

  # =========================================================================
  # SECTION 60 — route-junction records
  # =========================================================================
  # 64 B/record, phase-shifted. Re-anchor on the marker `16 SS SS 00 00 01 01`
  # (type byte 0x16 lands at +34); then X@+7 / Y@+11 (RAW). The leading 32-B
  # preamble is MFC CArchive object/undo framing (0xFFFF new-class + 0x2101
  # schema tags + object-id counter + 0x80000000 flag + handle-band i32s), not
  # coordinates. Net and layer are not per-junction (external).
  #
  # In v0x2026 the section is a direct 64-byte record array with byte-unaligned
  # coordinates (X @+17, Y @+21 RAW), route/via type byte @+24 in {0xEF,0xF1..0xF5},
  # group @+40, ordinal_kind @+44 (low byte 0x16=route chain, 0x0E=via/fill pool,
  # upper 24 bits = ordinal inside the local chain), class @+48. In v0x2025 placed
  # via rows have marker bytes +50=0x0E, +54=0x17, +55=0x02 with X @+23, Y @+27 RAW.
  sec60_route_junction:
    doc: marker-aligned 64-byte junction record (re-anchor before reading)
    seq:
      - id: handle_lo
        type: u2
        doc: '+0 record id / handle low'
      - id: handle_hi
        size: 5
        doc: '+2 high handle bytes + 0x80 flag'
      - id: x_raw
        type: s4
        doc: '+7 X coordinate (RAW = design + origin)'
      - id: y_raw
        type: s4
        doc: '+11 Y coordinate (RAW)'
      - id: ptr_a
        type: s4
        doc: '+15 handle/pointer; not geometry'
      - id: mid
        size: 15
        doc: '+19..+33 flags + (0x0E) terminal marker 01 21 00 00 + ref'
      - id: type
        type: u1
        doc: '+34 0x16=corner/junction, 0x0E=connection/terminal'
      - id: seq
        type: u2
        doc: '+35 ordinal within route/chain (1,2,3...)'
      - id: type_mirror
        type: u2
        doc: '+37 0x0000 for 0x16, 0x1700 for 0x0E'
      - id: flags
        type: u4
        doc: '+39 role/flags word (01 01 40 00 dominant)'
      - id: ptr_b
        type: s4
        doc: '+43 handle/pointer; not geometry'
      - id: pad
        size: 16
        doc: '+47..+62 zero'
      - id: counter_lo
        type: u1
        doc: '+63 rolling object-id low byte'

  # =========================================================================
  # SECTION 61 — object-handle / heap-bookkeeping snapshot
  # =========================================================================
  # Logical record is 64 B. total_bytes==count*12; count==sec29.count (routed
  # boards). Phase-shifted, per-file marker variant (00 16 SS / 01 21 00 00 /
  # 21 01 20 00). Undo/handle bookkeeping, not geometry.
  sec61_object_handle:
    doc: logical 64-byte heap-object record (phase + marker must be sniffed)
    seq:
      - id: class_tag
        type: u4
        doc: '+0 object class magic (13986 / 0)'
      - id: zero0
        type: u2
        doc: '+4'
      - id: flag80
        type: u1
        doc: '+6 0x80 object flag (else 0)'
      - id: ptr_a
        type: u4
        doc: '+7 heap pointer (process address)'
      - id: ptr_b
        type: u4
        doc: '+11 heap pointer / object ref'
      - id: ptr_c
        type: u4
        doc: '+15 heap pointer low-word 0x001DF8xx'
      - id: zero1
        size: 10
        doc: '+19..+28'
      - id: subtype
        type: u1
        doc: '+29 group / sub-type id (0x0C..0x0F)'
      - id: marker_role
        size: 3
        doc: '+30..+32 object-role marker (0x16 family; file-variant)'
      - id: seq
        type: u1
        doc: 'SS seq byte inside marker, resets per group'
      - id: marker_tail
        size: 5
        doc: '00 00 00 01 01 (file-variant)'
      - id: zero2
        size: 25
        doc: '+38..+62'
      - id: objid_lo
        type: u1
        doc: '+63 low byte of a global object-id counter'

  # =========================================================================
  # SECTION 62 — route-object array
  # =========================================================================
  # 48 B/record, phase-shifted (sniff the width column). Serialized C++ route
  # object with embedded heap pointers. width = quarter_width*4. The (x,y) pair
  # is an object endpoint. type_enum is a small bounded enum (1..12).
  sec62_route_object:
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

  # =========================================================================
  # SECTION 64 — route coordinate pool
  # =========================================================================
  # A small per-file header sets the coord byte-phase, then a packed stream of
  # RAW i32 coordinates using Manhattan/45 coord-sharing (a vertex contributes 1
  # or 2 i32). Polyline framing, net, layer and width come from sec62/sec60/sec24,
  # not from here. In v0x2026 records are treated as 12-byte [type, X@+1, Y@+5,
  # tail] vertices; type 0xF2/0xF4 = top copper, 0xEF/0xF1/0xF3/0xF5 = bottom.
  sec64_route_coord_pool:
    seq:
      - id: header
        size: 16
        doc: |
          per-file header (16/23/41 B; sets coord byte-phase). Placeholder size;
          a reader must sniff the phase (the offset%4 whose i32 land on route
          coords) rather than trust this fixed 16.
      - id: coords
        type: s4
        repeat: eos
        doc: |
          RAW i32 coordinates (design = value - per-axis origin). Interspersed
          0x80000000 inter-polyline markers and a recurring 0x0009685a-class
          constant pair shift the grid (the phase). Run boundaries are not in-band.

  # =========================================================================
  # SECTION 67 — secondary route coordinate pool
  # =========================================================================
  # A flat packed RAW i32 route-coordinate stream at one sniffed per-file phase.
  # Straddles the section boundaries (prefix/trailing fragments). A smaller
  # route-coord subset that overlaps sec64. Topology not in-band (same as sec64).
  sec67_route_coord_pool:
    seq:
      - id: phase_prefix
        size: 2
        doc: |
          tail bytes of the boundary-straddling start coordinate (phase 0..3,
          sniffed). The 2 here is a placeholder; sniff per file.
      - id: coords
        type: s4
        repeat: eos
        doc: |
          dense packed RAW i32 route coordinates; X uses origin_x, Y uses
          origin_y. The last 1-2 bytes are the head of a coordinate that
          continues past the section.

  # =========================================================================
  # SECTIONS 69 / 70 / 71 / 73 — layer-definition / stackup table (one stream)
  # =========================================================================
  # One contiguous 152-byte-per-layer record stream that the directory carves
  # into sec69 / sec70 (16-B bridge, all-zero or heap on big boards) / sec71
  # (1148-B continuation) and overflows past sec73 into the trailing un-indexed
  # heap blob. 31 records: (All layers) + 30 numbered layers. Locate record 0 by
  # the inline string "(All layers)" and read 152-B records, marker-scanning the
  # heap blob for big boards (marker (b & 0x86)==0x86 for copper layers, valid
  # enum@8, printable name@12). sec70/73 are alias windows over this stream.
  # STACKUP SOURCE (confirmed v0x2027, 30/30 layers EXACT on BR350420B/430B/460A vs .asc):
  # layer_thickness@+52 and copper_thickness@+56 are BASIC units == .asc
  # LAYER_THICKNESS/COPPER_THICKNESS verbatim; dielectric f4@+60 == .asc DIELECTRIC.
  # usage@+148==1 marks an active copper layer (count == .asc MAXIMUMLAYER). Locate the
  # 31-record array by the inline string "(All layers)", NOT directory data_offset.
  sec69_layer_record:
    doc: 152-byte layer definition + physical stackup + display-color record
    seq:
      - id: name
        type: str
        size: 24
        encoding: ASCII
        terminator: 0
        doc: '+0 layer name ("(All layers)","Top","Solder Mask Top",...)'
      - id: rsv0
        type: s4
        doc: '+24 const 0'
      - id: rsv1
        type: s4
        doc: '+28 const 0'
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
        repeat-expr: 12
        doc: '+92..+136 remaining per-element display colors (slot order approximate)'
      - id: flags
        type: s4
        doc: '+140 packed attribute bitfield (bits0-2 routable/visible/selectable)'
      - id: rsv2
        type: s4
        doc: '+144 const 0 (junk on final record)'
      - id: usage
        type: s4
        doc: '+148 1=routing-used(==MAXIMUMLAYER), 0=unused/drill, 2..6=doc subtype'

enums:
  pad_shape:
    0: of    # oblong / oval finger
    1: rf    # rectangular finger
    2: r     # round
    3: s     # square
