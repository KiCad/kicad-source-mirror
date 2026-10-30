meta:
  id: t3001
  title: TARGET 3001! project file (.T3001)
  file-extension: t3001
  endian: le
  encoding: windows-1252
doc: |
  A TARGET 3001! project is a single sequential stream with no container framing: a header, then four
  element trees (signals, components, free schematic elements, free board elements), then project settings.
  Every element is a one-byte class tag followed by that class's body; lists are elements too, so the
  element trees are one recursive grammar. Nothing in the file is located by offset or by search.

  Units: coordinates and sizes are nanometres, Y up; angles (f4) are degrees counter-clockwise and always finite.
  Schematic sheets have their origin at the sheet centre. TDateTime values count days from 1899-12-30 00:00,
  with the fraction as time of day.
  Element layer numbers depend on class: ordinary schematic graphics use 101..200 for sheet (layer - 100).
  For pad, layer >= 101 selects project padstack (layer - 101) instead. Layer 100 selects every copper
  layer: lines, arcs, discs, text and fiducials on it are drawn identically on each copper layer, without the
  mirroring a bottom-side layer would otherwise imply; a text's own mirrored flag still applies (verified against
  vendor exports); a hole requires a nonzero drill. Layers 0..99 are board layers. Default board layer functions
  (verified against TARGET's ODB++ export):
  2 copper bottom, 16 copper top, 4/18 solder mask bottom/top, 5/19 paste bottom/top, 7/21 position (silk)
  bottom/top, 1/15 deletion bottom/top, 0/8/14 area, 22 dimensioning, 23 board outline, 24 drill plan.
  Projects assign layer functions in header.layer_functions; inner copper is 10, 13 on 4-layer boards and
  10, 13, 16, 19 on 6-layer boards, which move top copper to 22. Physical stack order runs from top copper
  through the inner copper layers in descending layer number to bottom copper.

  Decoded element semantics below are verified against the corpus and the vendor's own exports unless marked
  "unconfirmed"; unconfirmed meanings have no matching corpus case yet.

  This spec covers files whose extension is .T3001. The legacy .T3 / .T2001 / .T20 layouts are selected by
  file extension, not by content, and are out of scope here. Password-protected files (header marker
  "$%&\u00a7" / "$%&\u00a7!") are recognised but not decoded.
seq:
  - id: version
    type: s4
    doc: Signed. Writer version, 10014..10051 in the corpus (10051 = TARGET 3001! V33).
  - id: header
    type: header
  - id: signals
    type: elem
    doc: |
      Normally the signal tree, with the component tree next. The trees are told apart by their records'
      tags: one corpus file stores the components here and the signals next, and the vendor schematic
      shows both in full. Its board view of that file shows neither, but the records are complete.
    valid:
      expr: _.tag == elem_tag::list
  - id: components
    type: elem
    valid:
      expr: _.tag == elem_tag::list
  - id: sch_free
    type: elem
    doc: |
      Schematic items that belong to no signal or component. This can include a whole symbol,
      whose reference and value texts then hold their placeholders and are drawn as stored.
  - id: pcb_free
    type: elem
  - id: rest
    type: rest
  - id: trailing
    size-eos: true
    doc: |
      Unread. The reader stops after the settings its version knows and, when more than one byte remains,
      warns that the project came from a newer version, then loads it anyway. TARGET 14.2.0.35 leaves 44
      zero bytes here at version 0x271f; every other corpus file ends at the settings.
types:
  rest:
    doc: Project settings that follow the element trees. Gates compare the header version.
    seq:
      - id: signal_width_keys
        type: s4
        repeat: expr
        repeat-expr: 4
        doc: Schematic signal widths stored on the four width hotkeys, in nm.
      - id: track_width_keys
        type: s4
        repeat: expr
        repeat-expr: 4
        doc: Track widths stored on the four width hotkeys, in nm.
      - id: layer_sets_9bd
        size: 13
        repeat: expr
        repeat-expr: 4
        doc: Four 13-byte board-layer bitsets. Meaning unknown; mostly zero in the corpus.
      - id: recent_sheets
        type: u1
        repeat: expr
        repeat-expr: 4
        doc: Recently shown schematic sheets, newest first; new projects start with 1, 2, 3, 4.
      - id: recent_copper_layers
        type: u1
        repeat: expr
        repeat-expr: 4
        doc: Recently used copper layers for tracks, newest first; 99 marks an empty slot.
      - id: recent_drawing_layers
        type: u1
        repeat: expr
        repeat-expr: 4
        doc: Recently used non-copper drawing layers, newest first.
      - id: recent_schematic_grids
        type: s4
        repeat: expr
        repeat-expr: 10
        doc: Recently used schematic grid pitches in nm, newest first; offered by the grid dialog.
      - id: recent_board_grids
        type: s4
        repeat: expr
        repeat-expr: 10
        doc: Recently used board grid pitches in nm, newest first; offered by the grid dialog.
      - id: rule_clearance
        type: u4
        doc: Default copper clearance for design rule checks and the router, in nm.
      - id: rule_track_width
        type: u4
        doc: Default (minimum) track width for design rule checks and the router, in nm.
      - id: default_via_drill
        type: u4
        doc: Project default via drill in nm; seeds padstack_via.via_drill before each record is read.
      - id: rule_drill_spacing
        type: u4
        doc: Minimum distance between drill holes for design rule checks, in nm.
      - id: default_via_ring
        type: u4
        doc: Project default via annular ring in nm; seeds padstack_via.via_ring before each record is read.
      - id: print_scale
        type: f4
        doc: Print scale in percent as last set in the print dialog (100.0 in 416 of 735 corpus files).
      - id: print_correction_x
        type: f4
        doc: |
          Printer calibration factor along X, applied to printed and PDF output on top of print_scale;
          1.0 in 669 of 735 corpus files, others near 0.95 to 1.02. Drawing geometry is unaffected.
      - id: print_correction_y
        type: f4
        doc: Printer calibration factor along Y, as print_correction_x.
      - id: print_unknown
        size: 8
        doc: Meaning unknown; zero in every corpus file.
      - id: print_offset_x
        type: s4
        doc: Print offset in nm, as set in the print dialog (20 mm in 6 corpus files, otherwise 0).
      - id: print_offset_y
        type: s4
      - id: print_in_color
        type: u1
        doc: Non-zero prints in colour; zero prints sharp black and white. 1 in 223 of 734 corpus files.
      - id: transient_analysis
        type: transient_analysis
      - id: num_probe_records
        type: s4
        valid:
          min: 0
      - id: probe_records
        type: probe_record
        repeat: expr
        repeat-expr: num_probe_records
      - id: num_layer_function_names
        type: s4
        valid:
          min: 0
      - id: layer_function_names
        type: lstr
        repeat: expr
        repeat-expr: num_layer_function_names
        doc: Names of user-defined layer functions, e.g. "Siebdruck oben".
      - id: stale_30
        size: 30
        doc: |
          Leftover bytes with no setting in them. Bytes 0..3 change on every save; bytes 4..29 are the
          same in 717 of 734 corpus files. Readers should ignore them.
      - id: board_material
        type: lstr
        doc: Board base material; "FR4" in every corpus file.
      - id: num_route_layers
        type: u2
      - id: route_layers
        type: route_layer
        repeat: expr
        repeat-expr: num_route_layers
      - id: num_project_props
        type: s4
        valid:
          min: 0
      - id: project_props
        type: lstr
        repeat: expr
        repeat-expr: num_project_props
        doc: Alternating property keys and values; the count is of strings, not pairs, and is even in the corpus.
      - id: num_padstacks
        type: s4
        valid:
          min: 0
      - id: padstacks
        type: padstack
        repeat: expr
        repeat-expr: num_padstacks
      - id: route_layer_costs
        type: route_layer_cost
        repeat: expr
        repeat-expr: num_route_layers
      - id: padstack_vias
        type: padstack_via
        repeat: expr
        repeat-expr: num_padstacks
        doc: Router via parameters, one per padstack, in padstack order.
      - id: router_costs
        type: router_costs
      - id: num_spice_texts
        type: s4
        valid:
          min: 0
      - id: spice_texts
        type: lstr
        repeat: expr
        repeat-expr: num_spice_texts
        doc: Project SPICE directives and models; entries start with "SPICE" and a 0x03 separator.
      - id: layer_fill_patterns
        type: u1
        enum: fill_pattern
        repeat: expr
        repeat-expr: 100
        doc: Fill pattern of board layer 0..99 on screen.
      - id: translucent_layers
        size: 13
        doc: Board layers drawn with the global opacity setting, bit n of byte n / 8 for layer n.
      - id: layer_button_sets
        size: 13
        repeat: expr
        repeat-expr: 16
        doc: Visible-layer set stored on each of the 16 user layer buttons, as header.layer_view.visible_layers.
      - id: layer_button_names
        type: lstr
        repeat: expr
        repeat-expr: 16
        doc: |
          Labels of the 16 user layer buttons; unused buttons read "XX: undefined" or similar.
      - id: unknown_flag_a
        type: u1
        doc: Meaning unknown; 0 in all 734 corpus files and in new projects.
      - id: project_note
        type: lstr
        doc: |
          Free project text. New projects store "No special Info" (English program) or
          "Keine spezielle Information" (German program); the corpus holds only these two values.
      - id: customer_history
        type: lstr
        doc: |
          One "KD=<customer number>" line (CR LF) per change of the installation that saved the project; a
          save appends a line when the last one differs. Shown as "Additional Project Information".
      - id: simulation_analysis
        type: simulation_analysis
      - id: router_options
        type: router_options
      - id: created_with
        type: lstr
        doc: Program version string of the writer that created the project (shown as "Created" in project info).
      - id: saved_by
        type: lstr
        doc: Program version string of the writer.
      - id: num_simulation_traces
        type: s4
        valid:
          min: 0
      - id: simulation_traces
        type: simulation_trace
        repeat: expr
        repeat-expr: num_simulation_traces
      - id: spice_subcircuit_project
        type: u1
        doc: |
          Nonzero marks a project whose simulation data predates the current format and describes a SPICE
          subcircuit; on conversion the program adds the text "PROJECTTYPE: SUBCKT". 0 in all corpus files.
      - id: sheet_names
        doc: Schematic sheet names; entry i names sheet i + 1 (element layer 101 + i). Empty means unused.
        type: lstr
        repeat: expr
        repeat-expr: 100
      - id: variant_names
        type: lstr
        repeat: expr
        repeat-expr: 32
        doc: |
          Names of assembly variants 1..32 (entry i names variant i + 1); new projects name variant 1
          "<unbenannt>" / "<unnamed>". Corpus: "1st source"/"2nd source", "Standard"/"mit Temp.-sensor".
      - id: active_variant
        type: u1
        doc: Number of the assembly variant the editor shows (1..255); 0 in 75 corpus files.
      - id: dc_sweep
        type: dc_sweep
        if: _root.version > 0x2329
      - id: obfuscated_text
        type: lstr
        if: _root.version > 0x232a
        doc: Obfuscated text the writer sets; not needed to read the design.
      - id: standard_track_width
        type: s4
        if: _root.version >= 0x232c
        doc: Standard track width in nm (300000 by default; 0 is read as 300000).
      - id: standard_track_spacing
        type: s4
        if: _root.version >= 0x232c
        doc: Standard track spacing in nm (300000 by default; 0 is read as 300000).
      - id: logbook_version
        type: lstr
        if: _root.version >= 0x232d
        doc: Version text, e.g. "Version 0.0.1" (text function version reads its first "Version" line).
      - id: logbook_history
        type: lstr
        if: _root.version >= 0x232d
        doc: 'Dated change history, CR LF separated, starting "<dd.mm.yyyy>: Created project".'
      - id: logbook_notes
        type: lstr
        if: _root.version >= 0x232d
        doc: Change notes; new projects store "Important changes (must-have) ... Other (nice-to-have)" (or German).
      - id: selected_checks
        type: lstr
        if: _root.version > 0x232d
        doc: |
          Design checks selected in the check dialog, one check identifier per line (CR LF), e.g. Zeichnung,
          KurzschlussS, Leiterbahnen, Abstand; empty (322 corpus files) selects the default set.
      - id: monthly_backup
        type: u1
        if: _root.version > 0x232e
        doc: Keep monthly backups (settings dialog); 0 in the corpus.
      - id: weekly_backup
        type: u1
        if: _root.version > 0x232e
        doc: Keep weekly backups (settings dialog); 0 in the corpus.
      - id: check_parameter_file
        type: lstr
        if: _root.version > 0x232f
        doc: File of design-check parameters loaded in the check dialog; empty in the corpus.
      - id: button_presets
        type: button_presets
        if: _root.version > 0x232f and not _io.eof
        doc: Present only when the stream has not ended here (the reader tests position against size).
      - id: component_extra
        type: component_extra
        repeat: expr
        repeat-expr: _root.components.body.as<list>.num_items
        if: _root.version >= 0x2711
      - id: more_variant_names
        type: lstr
        repeat: expr
        repeat-expr: 223
        if: _root.version >= 0x2711
        doc: Names of assembly variants 33..255; empty in the whole corpus.
      - id: creator_id
        type: lstr
        if: _root.version > 0x2711
        doc: Obfuscated identification of the installation that created the project.
      - id: last_saver_id
        type: lstr
        if: _root.version > 0x2711
        doc: Obfuscated identification of the installation that last saved the project ("?" in new projects).
      - id: last_file_save
        type: f8
        if: _root.version >= 0x271b
        doc: TDateTime set when the project was created and kept through Save As; not the change date.
      - id: track_width_list_a
        type: s4
        repeat: expr
        repeat-expr: 10
        if: _root.version > 0x271b
        doc: Ten track widths in nm offered by the main toolbar width box (default 150000, 200000, 250000, 300000, ...).
      - id: track_width_list_b
        type: s4
        repeat: expr
        repeat-expr: 10
        if: _root.version > 0x271b
        doc: Second list of ten track widths for the same box; a width typed into the box is added to both lists.
      - id: unknown_flag_b
        type: u1
        if: _root.version > 0x271c
        doc: Meaning unknown; 0 in 729 corpus files, 3 in 5.
      - id: unknown_flag_c
        type: u1
        if: _root.version > 0x271c
        doc: Meaning unknown; 0 in the whole corpus.
      - id: text_language
        type: u1
        enum: text_language
        if: _root.version > 0x271d
        doc: |
          Language in which literal texts with $%&<LANGUAGE> variants are shown; the switch applies only when
          some text in the file reads AUTOLANGUAGE=TRUE.
      - id: output_variants
        size: 32
        if: _root.version > 0x271e
        doc: |
          256-bit set (bit n = variant n) of assembly variants chosen for print preview and bill of materials
          output. Corpus: only bit 0 set (193 files), none (532), bits 0..21 (5).
      - id: preview_board_outline_counts
        type: u1
        if: _root.version > 0x271f
        doc: Print preview option "PCB outline counts" (1 in 631 corpus files).
      - id: num_embedded_models
        type: s4
        if: _root.version > 0x272d
        valid:
          min: 0
      - id: embedded_models
        type: embedded_file
        repeat: expr
        repeat-expr: num_embedded_models
        if: _root.version > 0x272d
      - id: pad_preset_no_aura
        type: lstr
        repeat: expr
        repeat-expr: 12
        if: _root.version > 0x272e
        doc: NO_AURA layer list of each button_presets.pad_presets entry, in order; empty in the corpus.
      - id: integrity_seed
        type: u4
        if: _root.version > 0x2738
        doc: Integrity check value; not needed to read the design. Unique per file.
      - id: integrity_value
        type: u4
        if: _root.version > 0x2738
        doc: Second integrity check value; unique per file.
      - id: layer_button_copper
        type: u1
        repeat: expr
        repeat-expr: 16
        if: _root.version > 0x2742
        doc: Active copper layer stored on each of 16 layer buttons; 255 = unset (all 22 corpus files).
      - id: layer_button_drawing
        type: u1
        repeat: expr
        repeat-expr: 16
        if: _root.version > 0x2742
        doc: Active drawing layer stored on each of the same 16 buttons; 255 = unset.
  padstack:
    doc: |
      Project padstack. A pad whose base.layer >= 101 uses padstack (base.layer - 101) as its layer set. The pad
      keeps its own shape, width, height, direction, drill and polygon; the padstack only says on which layers the
      pad exists and how much it grows there. The copper pad on copper layer L exists iff on_layer[L] != 0, with
      size (width + 2 * oversize[L]) x (height + 2 * oversize[L]). The drill is a hole even when only one copper
      layer is set.
    seq:
      - id: name
        type: lstr
        doc: Trimmed; an empty name reads as "PadStack<n>".
      - id: cost
        type: u1
        doc: Autorouter via cost; new padstacks start at 1. No geometry meaning.
      - id: use_count
        type: s4
        doc: Cached count of referencing elements, recomputed by the program and often stale. No geometry meaning.
      - id: on_layer
        type: u1
        repeat: expr
        repeat-expr: 100
        doc: |
          Indexed by board layer 0..99; nonzero means the pad exists on that layer (only 0 and 1 occur). Non-copper
          layers (mask, paste, deletion) are listed too; copper membership is the subset whose
          header.layer_functions value is 0..2.
      - id: oversize
        type: s4
        repeat: expr
        repeat-expr: 100
        doc: |
          Growth per side in nm on each board layer, paired with on_layer by index. On a flagged deletion layer
          the pad outline grown by this amount is cut from the computed copper of the associated layer (from the
          program's logic; no vendor export exercises it). In the corpus every nonzero value is on a
          deletion_inside layer (plane clearance); copper oversize is always 0.
  padstack_via:
    doc: |
      Parameters the autorouter uses when it places a new via of this kind. Existing pad elements never read
      them; their own drill and width apply. A padstack created interactively leaves these bytes uninitialised,
      so many corpus values are arbitrary.
    seq:
      - id: via_drill
        type: s4
        doc: Via drill diameter in nm.
      - id: via_ring
        type: s4
        doc: Via annular ring in nm; via copper diameter = via_drill + 2 * via_ring.
  transient_analysis:
    doc: SPICE transient analysis settings (.TRAN), in seconds.
    seq:
      - id: unknown_0
        type: u1
        doc: Meaning unknown; 10 in every corpus file.
      - id: stop_time
        type: f4
      - id: step
        type: f4
      - id: max_step
        type: f4
      - id: use_initial_conditions
        type: f4
        doc: UIC is emitted when greater than zero.
      - id: start_time
        type: f4
      - id: unknown_21
        type: f4
        doc: Meaning unknown; 10.0 in every corpus file.
      - id: unknown_25
        type: f4
        doc: Meaning unknown; -10.0 in every corpus file.
  probe_record:
    doc: Legacy SPICE probe; no corpus file has any.
    seq:
      - id: unknown_0
        size: 11
      - id: flag
        type: u1
      - id: object_id
        type: u2
        doc: Component or signal the probe is attached to.
      - id: pin
        type: u2
  route_layer:
    doc: Autorouter settings of one board layer.
    seq:
      - id: layer
        type: u1
      - id: mode
        type: u1
        enum: route_layer_mode
      - id: signal
        type: u2
        doc: Signal the layer is reserved for when mode is only_signal, otherwise 0.
  route_layer_cost:
    seq:
      - id: step_cost
        type: u1
        doc: Extra router cost per step on this layer; 0 in every corpus file.
      - id: unknown_1
        type: u4
        doc: Meaning unknown; 0 in every corpus file.
  router_costs:
    doc: Contour router step costs, relative to one 0.1 mm step.
    seq:
      - id: preferred_direction
        type: u1
        doc: Cost of a step in the layer's preferred direction; 2 by default.
      - id: against_direction
        type: u1
        doc: Cost of a step against the preferred direction; 2 by default.
      - id: via
        type: u1
        doc: Cost of a standard via.
      - id: fence
        type: u1
        doc: Cost of crossing a route fence; 50 by default.
  simulation_analysis:
    doc: SPICE analysis set up in the simulation dialog. New projects use .TRAN, LIN, 100 points, 10 to 100000.
    seq:
      - id: analysis
        type: u1
        enum: spice_analysis
      - id: ac_sweep
        type: u1
        enum: spice_ac_sweep
      - id: ac_points
        type: s4
        doc: Points of the .AC sweep (default 100).
      - id: ac_start
        type: f8
        doc: .AC start frequency in Hz (default 10).
      - id: ac_stop
        type: f8
        doc: .AC stop frequency in Hz (default 100000).
  router_options:
    doc: Options of the autorouter dialogs.
    seq:
      - id: thick_first
        type: u1
        doc: Route thick signal tracks first (default 0).
      - id: short_first
        type: u1
        doc: Route short signal tracks first (default 1).
      - id: memory_first
        type: u1
        doc: Route memory (bus) structures first (default 0).
      - id: escape_vias
        type: u1
        doc: Escape vias for SMD and BGA pads (default 0).
      - id: rip_up_violations
        type: u1
        doc: Remove connections involved in rule violations at the end, leaving them unrouted (default 1).
      - id: vias_in_smd_pads
        type: u1
        doc: Allow vias within SMD pads (default 0).
      - id: stacked_vias
        type: u1
        doc: Allow vias of different signals on top of each other, e.g. blind vias (default 0).
      - id: untangle_passes
        type: s4
        doc: Passes of the untangling phase, which accepts spacing violations (default 2).
      - id: optimize_passes
        type: s4
        doc: Passes of the optimisation phase, which allows no spacing violations (default 1).
  simulation_trace:
    doc: A curve of the simulation oscilloscope, e.g. V(Vin) or I(r13,2).
    seq:
      - id: expression
        type: lstr
      - id: kind
        type: u1
        doc: Nonzero for a probe trace; 0 marks a trace computed by the graph calculator (no corpus case).
      - id: flags
        size: 4
        doc: Meaning unknown; 00 01 00 01 or 00 00 00 01 in the corpus.
      - id: color
        type: u4
        doc: Trace colour as 0x00BBGGRR (corpus uses red, yellow, blue, magenta, cyan).
  dc_sweep:
    doc: .DC analysis set-up; empty and zero in the whole corpus.
    seq:
      - id: source_1
        type: lstr
        doc: Name of the first swept voltage source (written as V<name>); empty means none.
      - id: source_2
        type: lstr
        doc: Name of the second swept source.
      - id: start_1
        type: f8
      - id: stop_1
        type: f8
      - id: step_1
        type: f8
      - id: second_enabled
        type: u1
        doc: Nonzero adds the second sweep to the .DC line.
      - id: start_2
        type: f8
      - id: stop_2
        type: f8
      - id: step_2
        type: f8
  button_presets:
    doc: Twelve programmable buttons each for the track and the pad dialog.
    seq:
      - id: track_presets
        type: track_preset
        repeat: expr
        repeat-expr: 12
      - id: pad_presets
        type: pad_preset
        repeat: expr
        repeat-expr: 12
  track_preset:
    seq:
      - id: name
        type: lstr
        doc: Button label; its first three characters are shown on the button.
      - id: width
        type: s4
        doc: Track width in nm.
      - id: aura
        type: s4
        doc: Track aura (clearance) in nm.
      - id: layer
        type: u1
        doc: Layer to switch to; 100 means the preset keeps the current layer.
      - id: component
        type: u1
        doc: Value of the dialog's component checkbox.
  pad_preset:
    seq:
      - id: name
        type: lstr
        doc: |
          Button label. In files before pad_preset_no_aura existed, a "^" followed by one character may end the
          name: bits 0-1 of that character give the solder-mask setting, bits 2-3 the paste setting; the reader
          strips it.
      - id: height
        type: s4
      - id: width
        type: s4
      - id: aura
        type: s4
      - id: drill
        type: s4
      - id: layer
        type: u1
        doc: 100 selects all copper layers.
      - id: shape
        type: u1
        doc: Pad shape code, as pad_shape.
  component_extra:
    doc: |
      One entry per slot of the component tree. When present, variants_high supplies bytes 4..31 of the
      component's 256-bit assembly-variant set (variants 32..255); without it the component belongs to every
      variant above 32.
    seq:
      - id: present
        type: u1
      - id: variants_high
        size: 28
        if: present != 0
        doc: |
          Bytes 4..31 of the variant set, ORed in after bits 33..255 are cleared. Some writers leave stale bytes here.
  embedded_file:
    doc: File stored inside the project, e.g. a component's 3D model as "<library>-<model>.step" and ".stl".
    seq:
      - id: name
        type: lstr
      - id: len_data
        type: s4
        valid:
          min: 0
      - id: data
        size: len_data
        doc: zlib stream of the file's bytes (STEP text, or binary STL whose header starts "TARGET 3001!").
  signal_electrical:
    doc: |
      Eight 6-byte reals (byte 0 exponent e, bytes 1..5 mantissa little-endian with sign in the top bit;
      value = (1 + mantissa / 2^39) * 2^(e - 129), 0 when e = 0). Defaults 0, 0, 0, 0, 0.1, 1, 1, 2.
    seq:
      - id: frequency_khz
        size: 6
      - id: rise_time_ns
        size: 6
      - id: pulse_width_ns
        size: 6
      - id: period_ns
        size: 6
      - id: current_a
        size: 6
      - id: voltage_v
        size: 6
      - id: inner_resistance_ohm
        size: 6
      - id: allowed_noise_mv
        size: 6
  lstr:
    seq:
      - id: len
        type: s4
        valid:
          min: 0
      - id: value
        size: len
        doc: Bytes in the writer's ANSI code page (windows-1252 for this corpus); some fields hold obfuscated data.
  header:
    seq:
      - id: schematic_text_font
        type: lstr
        valid:
          expr: _.value != [0x24, 0x25, 0x26, 0xa7] and _.value != [0x24, 0x25, 0x26, 0xa7, 0x21]
        doc: |
          Default font for new schematic texts. A password marker here is unsupported.
          "Arial" in 674 of 734 corpus files, empty in 31, "DIN-ISO-ANSI" in 21.
      - id: board_text_font
        type: lstr
        doc: |
          Default font for new board texts. "DIN-ISO-ANSI" (TARGET's built-in stroke font) in 557
          of 734 corpus files, otherwise a TrueType name such as "Arial Black" or "Arial", or empty.
      - id: signal_name_font
        type: lstr
        doc: Default font for new schematic signal names; "Arial" in 688 of 734 corpus files, "TARGET" in 43.
      - id: editor_defaults
        type: editor_defaults
        doc: Values the editor uses for newly drawn objects, as set on the defaults pages of the project settings.
      - id: text_and_grid_defaults
        type: text_and_grid_defaults
      - id: layer_functions
        type: u1
        enum: layer_function
        repeat: expr
        repeat-expr: 100
        doc: |
          Function of board layer 0..99, indexed by layer number. This per-project table is authoritative;
          enum layer_function_default only lists the values a new project starts with.
          The corpus also uses codes outside the enum (26, 27, 30, 35, 36, 45..52, 101..163). They carry only
          non-copper graphics; 30 holds DRC error markers (drc_marker). Copper is exactly codes 0..2.
      - id: layer_associations
        type: u1
        repeat: expr
        repeat-expr: 100
        doc: |
          Copper layer that board layer 0..99 belongs to (255 = none). Copper layers point at themselves;
          area and deletion layers point at the copper layer they pour on or cut, normally layer + 2 and
          layer + 1, but projects can reassign them. An entry that names no copper layer (seen as 0) is
          unset, and the area or deletion layer pairs with layer + 2 or layer + 1.
      - id: layer_view
        type: layer_view
      - id: num_prefixes
        type: u1
      - id: prefixes
        type: prefix
        repeat: expr
        repeat-expr: num_prefixes
  prefix:
    seq:
      - id: key
        type: u2
      - id: text
        type: lstr
  editor_defaults:
    doc: Defaults for new objects. Lengths in nm.
    seq:
      - id: track_layer
        type: u1
        doc: Board layer for new tracks.
      - id: track_width
        type: s4
        doc: Width of new tracks.
      - id: schematic_trace_width
        type: s4
        doc: Width of new schematic signal traces; 300000 in every corpus file.
      - id: line_width
        type: s4
        doc: Width of new drawing lines.
      - id: bus_width
        type: s4
        doc: Width of new schematic buses; 1000000 in every corpus file.
      - id: thermal_bridge_width
        type: s4
        doc: Width of new copper bridges (thermal spokes) on the board.
      - id: aura
        type: s4
        doc: Default aura (clearance) around new pads and tracks.
      - id: drawing_layer
        type: u1
        doc: Board layer for new non-copper drawing objects.
      - id: unknown_26
        type: u1
        doc: Meaning unknown; 0 in every corpus file.
      - id: pin_is_clock
        type: u1
        doc: New pins get the clock marker when non-zero.
      - id: pin_number_visible
        type: u1
        doc: New pins show their number in the schematic when non-zero.
      - id: pin_is_inverted
        type: u1
        doc: New pins get the inversion marker when non-zero.
      - id: pin_swap_number
        type: u2
        doc: Pin swap group for new pins.
      - id: symbol_swap_number
        type: u2
        doc: Symbol (gate) swap group for new symbols.
      - id: pin_length
        type: s4
      - id: pin_width
        type: s4
      - id: pad_layer
        type: u1
        doc: Board layer for new pads; 100 means all copper layers.
      - id: pad_shape
        type: u1
        doc: Shape of new pads, in the order round, octagonal, rectangular, oblong, polygonal.
      - id: pad_width
        type: s4
      - id: pad_height
        type: s4
      - id: pad_drill
        type: s4
      - id: via_layer
        type: u1
        doc: Board layer for new vias; 100 means all copper layers.
      - id: via_shape
        type: u1
        doc: Shape of new vias, in the order round, octagonal, rectangular, oblong.
      - id: via_width
        type: s4
        doc: Width of new vias; width and height may be swapped, they are equal in the corpus.
      - id: via_height
        type: s4
      - id: via_drill
        type: s4
  text_and_grid_defaults:
    seq:
      - id: schematic_text_style
        type: text_style
        doc: Default style for new schematic texts.
      - id: board_text_style
        type: text_style
        doc: Default style for new board texts.
      - id: signal_name_style
        type: text_style
        doc: Default style for new schematic signal names.
      - id: node_radius
        type: s4
        doc: Radius of schematic junction dots in nm.
      - id: schematic_grid
        type: grid_view
        doc: Grid and view state of the schematic window.
      - id: board_grid
        type: grid_view
        doc: Grid and view state of the board window.
  text_style:
    seq:
      - id: height
        type: s4
        doc: Character height in nm.
      - id: width
        type: s4
        doc: Character width in nm.
      - id: thickness
        type: u2
        doc: Stroke thickness in percent of the height, 1..100; 13 in most corpus files.
  grid_view:
    doc: The window's grid settings, as shown in the grid dialog. Lengths in nm.
    seq:
      - id: crosshair
        type: u1
        doc: 0 none, 1 orthogonal crosshair, 2 orthogonal and diagonal crosshair.
      - id: relative_origin_x
        type: s4
        doc: Origin of the window's relative coordinate display.
      - id: relative_origin_y
        type: s4
      - id: grid_style
        type: u1
        doc: 0 grid drawn as points, 1 as lines, 2 hidden.
      - id: snap_to_grid
        type: u1
        doc: Cursor snaps to the grid when non-zero.
      - id: grid
        type: s4
        doc: Grid pitch.
      - id: grid_min_pixels
        type: u1
        doc: Grid is drawn only when its pitch is at least this many screen pixels.
      - id: grid_major_every
        type: u1
        doc: Every Nth grid line or point is emphasised.
      - id: grid_offset_x
        type: s4
        doc: Offset of the grid from the coordinate origin.
      - id: grid_offset_y
        type: s4
      - id: min_line_pixels
        type: s4
        doc: Lines thinner than this many screen pixels are drawn as a thin substitute line.
      - id: unit
        type: u1
        doc: Display unit, 0 mil, 1 inch, 2 mm, 3 µm, 4 nm.
  layer_view:
    seq:
      - id: layer_colors
        type: u1
        repeat: expr
        repeat-expr: 100
        doc: Display colour of board layer 0..99 as a palette index 0..15.
      - id: visible_layers
        size: 13
        doc: Visible board layers, bit n of byte n / 8 (LSB first) for layer n.
      - id: current_sheet
        type: u1
        doc: Schematic sheet shown when the project was saved; always equals the first entry of rest.recent_sheets.
      - id: unknown_114
        type: u1
        doc: Meaning unknown; 0 in 673 of 734 corpus files, otherwise 1.
      - id: unknown_115
        type: u2
        doc: Meaning unknown.
      - id: unknown_117
        type: u2
        doc: Meaning unknown.
  elem:
    seq:
      - id: tag
        type: u1
        enum: elem_tag
      - id: body
        type:
          switch-on: tag
          cases:
            'elem_tag::line': line
            'elem_tag::pin': pin
            'elem_tag::pad': pad
            'elem_tag::triangle': triangle
            'elem_tag::rectangle': rectangle
            'elem_tag::arc': arc
            'elem_tag::disc': disc
            'elem_tag::fiducial': fiducial
            'elem_tag::symbol': symbol
            'elem_tag::package': package
            'elem_tag::spline': spline
            'elem_tag::drc_marker': drc_marker
            'elem_tag::text': text
            'elem_tag::signal': signal
            'elem_tag::component': component
            'elem_tag::list': list
            'elem_tag::layer_function_list': layer_function_list
            'elem_tag::nil': nil
            _: unsupported
  nil:
    doc: An absent element; the writer stores a null reference as the single tag byte 0xff.
    seq: []
  unsupported:
    seq:
      - id: never
        size: 0
        valid:
          expr: false
  base:
    doc: Fields common to every drawable element. Properties are alternating key, value strings.
    seq:
      - id: component
        type: u2
        doc: |
          ID of the owning component, or 0. A component stores its own ID here; IDs are unique per file and
          independent of list position. When signal is 65503 this field holds a pad number instead (see signal).
      - id: signal
        type: u2
        doc: |
          ID of the owning signal (net), or 0. A signal stores its own ID here and every element in its
          connections and tracks repeats it; pads and vias join a net by this ID, not by list position.
          65503 marks an element of a component that is linked to one of that component's pads: component then
          holds the linked pad's pad_number (non-zero), the element takes that pad's signal, and its owner is
          the enclosing component. Found on pads and copper lines inside components; every link in the
          corpus names exactly one pad, and that pad never carries 65503 itself.
      - id: x1
        type: s4
      - id: y1
        type: s4
      - id: flags
        type: u1
        doc: |
          Bit 0 means hidden/unplaced: board elements with it set are not drawn or exported (1467 of
          1471 flagged copper track lines in the vendor-export corpus are absent from the export; the rest
          coincide with other lines). On a schematic symbol (gate) it marks the gate as unplaced: the
          sheet does not show it, and its pins carry the bit too (505 such gates in the corpus; no pin of
          a placed gate is flagged). Every pin of a placed gate, supply symbols included, is drawn with
          its full length. Other persistent bit meanings are unconfirmed.
          Elements with any flag set are ignored when computing the contour box and area fills
          (see layer_function area_bottom); deletion elements are excluded by bit 0 alone.
      - id: unknown_byte
        type: u1
        doc: Meaning unconfirmed; the reader normalizes 1 to 255.
      - id: layer
        type: u1
        doc: Class-dependent; pad values >= 101 select project padstack (layer - 101).
      - id: tag_copy
        type: u1
        doc: Repeats the element's class tag (every element in the corpus).
      - id: extra_value
        type: u4
        if: (flags & 0xfe) == 2
        doc: Present when flags are 2 or 3; meaning unknown (no corpus case).
      - id: nprops
        type: s4
        valid:
          expr: _ >= 0 and _ % 2 == 0
      - id: props
        type: lstr
        repeat: expr
        repeat-expr: nprops
        doc: Alternating property keys and values; nprops counts strings, not pairs.
  list:
    seq:
      - id: num_items
        type: s4
        valid:
          min: 0
      - id: items
        type: elem
        repeat: expr
        repeat-expr: num_items
  layer_function_list:
    seq:
      - id: num_items
        type: s4
        valid:
          min: 0
      - id: items
        type: lstr
        repeat: expr
        repeat-expr: num_items
  line:
    doc: Straight stroked segment from base x1/y1 to x2/y2; width is the full stroke width.
    seq:
      - id: base
        type: base
      - id: x2
        type: s4
      - id: y2
        type: s4
      - id: width
        type: s4
      - id: clearance
        doc: |
          Copper clearance of a stroke on a copper layer. A copper area of another signal keeps exactly
          this distance from the stroke's edge (about 4000 measured strokes in the corpus); a stroke of
          the area's own signal with clearance 0 joins the area.
        type: s4
      - id: dash_style
        type: u1
        enum: line_dash
      - id: round_ends
        type: u1
        doc: Nonzero enables rounded ends.
  pin:
    doc: |
      Schematic pin. Base x1/y1 is the connection point; length extends toward the symbol body.
      Direction 0 is right, 90 up, 180 left, 270 down. Child text function pin_name carries the name;
      its hidden flag controls name visibility independently of show_number.
    seq:
      - id: base
        type: base
      - id: direction
        type: f4
      - id: length
        type: s4
      - id: width
        type: s4
      - id: electrical_type
        type: u1
        enum: pin_electrical_type
      - id: swap_level
        type: u2
      - id: inverted
        type: u1
        doc: Nonzero enables the inversion bubble.
      - id: clock
        type: u1
        doc: Nonzero enables the clock marker.
      - id: show_number
        type: u1
        doc: Nonzero shows the pin number.
      - id: supply_pin
        type: u1
        doc: Nonzero designates a reference/supply pin.
      - id: pin_number
        type: u2
      - id: child
        type: elem
  pad:
    doc: |
      Pad or via centred at base x1/y1. Width and height are full local dimensions before rotation.
      Unlike other element classes, base.layer >= 101 selects project padstack (base.layer - 101).
      PADNAME, when present, is the pad's name in place of pad_number, even when empty; a name equal to
      pad_number is redundant. Without it a nonzero pad_number is the name and 0 means unnamed.
      POLY_PAD="x,y;x,y;..." supplies polygon vertices in nanometres in the pad's own frame (Y up), centred
      on the pad and rotated counter-clockwise by direction (verified against vendor surfaces); fewer than
      three vertices falls back to shape octagon.
      RECT_ROUNDED="v" (decimal percent) rounds the corners of a rectangle pad with corner radius
      v / 200 * min(width, height); other shapes ignore it (corpus pads matched to vendor exports: 4633
      rounded rectangles for values 1..50, 3660 non-rectangles unaffected). Drill zero means no hole; SLOT_HOLE > drill
      supplies elongated-hole length (decimal nanometres) along the pad's local X, with drill as its width,
      rotated by direction (verified against vendor drill exports). DRILLPICT selects the drill drawing glyph (0..21,
      default 21),
      independently of padstack selection.
    seq:
      - id: base
        type: base
      - id: direction
        type: f4
      - id: shape
        type: u1
        enum: pad_shape
        doc: |
          Round with width != height is a true ellipse (not a stadium); oval is the stadium, with semicircular
          ends of diameter min(width, height) along the longer axis; octagon is a rectangle with every
          corner chamfered by 0.25 * min(width, height). The reader clamps values above 4 to 2.
      - id: width
        type: s4
        doc: Size along the pad's own X axis before rotation by direction.
      - id: height
        type: s4
      - id: aura
        doc: |
          Copper clearance. A copper area keeps this distance from the element even when both belong to
          the same signal, so a via or pad in its own area stands isolated unless signal lines drawn to it
          bridge the gap. Base property NO_AURA, a comma-separated list of board layer numbers (e.g. "2,16"),
          drops the aura on those layers and the area then joins the element solidly.
          An aura of 0 leaves no gap on any layer.
          On two-layer boards this predicts the vendor fill around 1846 of 1929 signal vias inside areas of
          their own signal;
          the other 83 are grouped in a few files and remain unexplained.
        type: s4
      - id: drill
        type: s4
        doc: |
          Hole diameter; 0 means no hole. A hole is unplated only when both width and height are smaller than it;
          copper exactly as large as the hole still plates it (verified against the vendor's ODB++ drill plating
          attribute).
      - id: unknown_value
        type: s4
        doc: Meaning unknown; zero in the checked corpus.
      - id: pad_number
        type: u2
  triangle:
    doc: Filled triangle; base x1/y1 is the first corner.
    seq:
      - id: base
        type: base
      - id: x2
        type: s4
      - id: y2
        type: s4
      - id: x3
        type: s4
      - id: y3
        type: s4
  rectangle:
    doc: |
      Filled rectangle anchored at base x1/y1. Width and height are signed local dimensions;
      rotate offsets (0,0), (width,0), (width,height), (0,height) about the anchor by direction.
      A base property JPEG_PICTURE turns the rectangle into a picture: its value is base64 without padding,
      with '_' in place of '+', and decodes to a JPEG (68 of 68 in the corpus). On schematic sheets the
      image is stretched to fill the rectangle, independently along each axis, with its top row at the
      rectangle's larger-Y edge. On board layers, outputs (ODB++, copper) treat it as the plain filled
      rectangle. SOURCE names where it came from (a file name or Clipboard); OPACITY is rare.
    seq:
      - id: base
        type: base
      - id: height
        type: s4
      - id: width
        type: s4
      - id: direction
        type: f4
  arc:
    doc: |
      Stroked arc/circle. Base x1/y1 is the start endpoint, end_x/end_y the end endpoint;
      center_x/center_y is the centre. Radius is the centreline radius, width the full stroke width.
      Angles are degrees from +X, positive counter-clockwise; values above 360 occur.
      The sweep runs counter-clockwise from start_angle to end_angle: end minus start, plus whole turns
      until it lies in (0, 360]. Equal angles, like 0 to 360, mean a full circle; coincident endpoints do
      not imply zero sweep. No corpus arc sweeps more than one turn. Centre, radius and angles define the
      drawn arc.
      No body fill flag; a filled circle is disc.
    seq:
      - id: base
        type: base
      - id: end_x
        type: s4
      - id: end_y
        type: s4
      - id: center_x
        type: s4
      - id: center_y
        type: s4
      - id: radius
        type: s4
      - id: width
        type: s4
      - id: clearance
        type: s4
      - id: start_angle
        type: f4
      - id: end_angle
        type: f4
  disc:
    doc: Filled disc centred at base x1/y1; radius is not the diameter.
    seq:
      - id: base
        type: base
      - id: radius
        type: s4
  fiducial:
    doc: Fiducial/alignment mark centred at base x1/y1; ring centreline radius is radius minus half width.
    seq:
      - id: base
        type: base
      - id: kind
        type: u1
        enum: fiducial_kind
      - id: radius
        type: s4
        doc: Outer radius.
      - id: width
        type: s4
      - id: direction
        type: f4
  symbol:
    doc: |
      Schematic gate/unit. ROTATION and MIRRORED properties describe placement; missing ROTATION
      uses -360 as a sentinel. Child coordinates already include rotation, mirroring and translation;
      do not apply the parent transform again.
    seq:
      - id: base
        type: base
      - id: name
        type: lstr
        doc: Full gate identifier, e.g. D2a.
      - id: gate_suffix
        type: lstr
        doc: Gate suffix, e.g. a.
      - id: gate_ordinal
        type: u2
        doc: One-based gate ordinal.
      - id: insert_required
        type: u1
        doc: Nonzero means required for normal insertion; independent of current placement.
      - id: swap_level
        type: u2
      - id: child
        type: elem
  package:
    doc: |
      Footprint placement with origin at base x1/y1 and rotation direction in degrees.
      Associated graphics and pads already use board coordinates. Layer is a drawing layer;
      bottom_side determines reverse placement.
    seq:
      - id: base
        type: base
      - id: package
        type: lstr
        doc: Used package name; installed as the parent component's USED_PACKAGE property.
      - id: name
        type: lstr
        doc: Package source library/identity.
      - id: num_pads
        type: u2
      - id: bottom_side
        type: u1
        doc: Nonzero means bottom/reverse placement.
      - id: direction
        type: f4
  spline:
    doc: |
      Quadratic Bezier from P0 (base x1/y1) to P2 (end_x/end_y) with control point C (control_x/control_y).
      B(t) = (1-t)^2 P0 + 2t(1-t) C + t^2 P2; width is the full stroke width.
    seq:
      - id: base
        type: base
      - id: end_x
        type: s4
      - id: end_y
        type: s4
      - id: control_x
        type: s4
      - id: control_y
        type: s4
      - id: width
        type: s4
      - id: clearance
        type: s4
  text:
    doc: |
      Text anchored at base x1/y1, the baseline-left point of the first glyph. Height and width are
      character dimensions, not string bounds. Glyph coordinates are mirrored first (x negated when
      mirrored, so mirrored text runs towards -X from the anchor), then rotated counter-clockwise by
      direction; the pen advances along the rotated +X (or -X when mirrored).
      Built-in fonts are measured on stroke centrelines: the stem of a left-stemmed glyph such as B or H lies
      on the anchor, and its foot on the baseline through it.
      Built-in font (font "" or "TARGET"): fixed pitch, cap height = height, x-height 7/12 height,
      descenders to -height/3; advance per character round(width * (100 + 2 * weight) / 100), so
      the string length is round(n * width * (100 + 2 * weight) / 100) for n characters.
      "DIN-ISO-ANSI": proportional glyphs scaled uniformly by S = max(height, width); cap height
      0.9 * S, descenders about -0.27 * S; advance round(glyph_width * S / 100 + 0.1 * S) + stroke,
      where glyph_width comes from the font's own glyph table. Other fonts are TrueType outlines named by
      font, rendered at a fixed master size and scaled linearly: the em square is (300 / 222) * height
      vertically ((300 / 241) * height for font "Calibri"), and width is the font's average character width
      (OS/2 xAvgCharWidth, the weighted lowercase average of the version 1 definition), so the horizontal em
      is width / average. weight * 35 is the Windows font weight: up to 15 renders regular, 16 and above
      select the bold face where one exists. A true ITALIC property selects italic. Vendor geometry comes
      from hinted outlines at 300 pixels per em, so it differs from unhinted rendering by up to about 0.3%.
      Boolean properties are true when their trimmed, upper-cased value is TRUE, YES, 1, JA, OUI or OK.
      ALIGN_RIGHT=TRUE marks right anchoring for editing: the program keeps the right end fixed and moves the
      start by -length (+length when mirrored), rotated, whenever the text changes. Stored x1/y1 is already
      that start, so without ARXY the text is placed from x1/y1 like any other (vendor exports agree for
      540 of 548 unambiguous right-aligned silkscreen texts). Modern ARXY="x|y" (decimal nanometres)
      stores the right end itself; it is authoritative, the start is derived from it on load, and for
      mirrored text it lies on the -X side of the anchor.
      no_auto_upright disables dynamic upright orientation (DYN=NO), independently of visibility.
      Function property_lookup takes its property key from the property named $%&.
      Properties $%&DEUTSCH, $%&ENGLISH and $%&FRANCAIS hold language variants of literal text (0x7f stands
      for a space; written only when a variant differs from the text). They replace the text only in a
      project where some text reads AUTOLANGUAGE=TRUE (trimmed, case-insensitive), and then the variant for
      the program's own language (recorded as settings text_language) is shown when it is non-empty;
      otherwise the stored text is shown.
    seq:
      - id: base
        type: base
      - id: text
        type: lstr
      - id: height
        type: s4
        doc: Vertical character size (cap height of the built-in font).
      - id: width
        type: s4
        doc: Horizontal character cell size of the built-in font.
      - id: direction
        type: f4
        doc: Rotation in degrees, counter-clockwise.
      - id: weight
        type: u1
        doc: |
          Stroke width as a percentage: of width for the built-in font (font "" or "TARGET"),
          of max(height, width) for "DIN-ISO-ANSI"; other fonts are outline (TrueType) fonts
          with no stroke. Font names compare trimmed and case-insensitively.
          The reader normalizes zero to hidden text with weight 10.
      - id: no_auto_upright
        type: u1
        doc: Nonzero disables automatic upright orientation.
      - id: mirrored
        type: u1
        doc: Nonzero mirrors the text.
      - id: function
        type: u1
        enum: text_function
      - id: font
        type: lstr
  component:
    doc: |
      Logical component; reference fields are distinct from the component index in base.component.
      The reference is reference_prefix followed by the decimal reference number (reference_number_repl
      when present); a number of 0 adds no digits.
    seq:
      - id: base
        type: base
      - id: name
        type: lstr
      - id: value
        type: lstr
      - id: reference_prefix
        type: lstr
      - id: library
        type: lstr
        doc: Component source library/path or identity.
      - id: package
        type: lstr
      - id: package_source
        type: lstr
        doc: Package-library path in older files; may be a database package ID in newer files.
      - id: reference_number
        type: u2
      - id: reference_number_repl
        type: u2
        if: reference_number >= 65000 and reference_number < 65535
        doc: A first word in 65000..65534 is a marker and the real value follows.
      - id: num_connections
        type: u2
      - id: gate_state
        type: u1
        enum: gate_state
      - id: packages
        type: elem
      - id: symbols
        type: elem
      - id: variants_low
        size: 5
        doc: |
          Bytes 0..4 of the component's 256-bit assembly-variant set (bit n set = fitted in variant n; bit 0 unused);
          only bits 0..32 count. Bits 33..255 come from rest.component_extra or default to set.
          New components store fe ff ff ff 01 (all of variants 1..32); a cleared bit removes the part from that
          variant (corpus: 61 parts of one project drop variant 2 "2nd source").
  signal:
    seq:
      - id: base
        type: base
      - id: name
        type: lstr
      - id: connections
        type: elem
        doc: |
          Schematic connectivity: line records are wires and disc records mark junctions. One signal joins
          its wires on every sheet it reaches by identity alone; a signal spanning several sheets needs no
          name text on any of them (4215 corpus signals span sheets, each with a sheet that has none).
      - id: tracks
        type: elem
        doc: |
          Board copper of the signal. Triangles on copper layers are stored copper surfaces, distinct from the
          computed fill of area layers. Lines with a true THERMAL_SNIPPET property are thermal spokes the area
          fill generated; the vendor ODB++ export omits them (4 of 3288 in the corpus, each coinciding with a
          routed line), whatever their unknown_byte.
          Old style areas store their spokes as flat-ended lines with unknown_byte 0 and no properties, starting
          in a pad or via of the signal on the area's copper layer (20892 of 21449 such corpus lines start at a
          pad centre, 0.5 to 2.4 mm long); the importer makes them thermal reliefs of that pad.
          Signal polygons are base properties "$%&POLY" (one per polygon, the key repeats) holding
          "layer,id,width,hatch,rank,thermals,islands,x,y;...;x,y*contour,rgb,bits,props": copper layer,
          random 64-bit POLYGONID, fill stroke width (0 = board default), hatch pitch (0 = solid), rank
          (higher ranks are cut out of lower ones), own pads by spokes, remove islands, a closed outline in
          nanometres, then the board contour distance, a colour (-1 = layer colour), thermal bits and the
          polygon's own properties (commas escaped as 0x1c, name and value split by 0x1d; AURA is the
          distance to other signals' polygons). Older records drop trailing tail fields. The polygon's
          computed fill is stored here as elements carrying its POLYGONID and an ELEMENT_FLAG bit mask
          (1 member, 2 interior triangle, 4 THERMAL_LIGAMENT spoke); flag-1 boundary strokes have the
          polygon's width, and spokes also carry THERMAL_LIGAMENT=TRUE. Spoked pads sit in knockouts
          their thermal gap wide, octagonal around rectangular pads.
      - id: min_width
        type: s4
      - id: min_spacing
        type: s4
      - id: type_coarse
        type: lstr
        doc: |
          Signal type as chosen in the signal dialog, e.g. CMOS, "Versorgungsspannung DC"; "undefiniert"/"undefined" by
          default.
      - id: type_fine
        type: lstr
        doc: Signal subtype, e.g. "4000 B", ">10V", "bis 1 MHz".
      - id: electrical
        type: signal_electrical
      - id: return_signal
        type: lstr
        doc: Name of the return conductor (reference signal), e.g. GND; empty in 98% of corpus signals.
  drc_marker:
    seq:
      - id: base
        type: base
      - id: radius
        type: s4
      - id: text
        type: lstr
enums:
  route_layer_mode:
    0: all_directions
    1: horizontal
    2: vertical
    3: blocked
    4: only_signal
  fill_pattern:
    0: solid
    1: horizontal
    2: vertical
    3: backward_diagonal
    4: forward_diagonal
    5: cross
    6: diagonal_cross
  pad_shape:
    0: round
    1: octagon
    2: rectangle
    3: oval
    4: polygon
  text_function:
    # Stored text is the last evaluated string; placeholders are in the authoring program's language
    # (e.g. !A-DATUM, !SEITE), so readers evaluate by code, not by stored text. A literal (0) text whose
    # stored text starts with "!" is a property lookup keyed by the rest of the text, trimmed and upper-cased.
    0: literal
    1: component_reference
    2: value
    3:
      id: symbol_name
      doc: Owning symbol's name, the component reference followed by the gate suffix as stored (e.g. IC6a).
    4:
      id: signal_name
      doc: The owning signal's name; hiding the text leaves the signal's connectivity unchanged.
    5: pin_name
    6:
      id: legacy_package_type
      doc: Converted to property_lookup with key PACKAGETYPE.
    7:
      id: legacy_article_number
      doc: Converted to property_lookup with key ARTICLENUMBER.
    8:
      id: legacy_manufacturer
      doc: Converted to property_lookup with key MANUFACTURER.
    9:
      id: legacy_order_number
      doc: Converted to property_lookup with key ORDERNUMBER.
    10:
      id: legacy_price
      doc: Converted to property_lookup with key PRICE.
    11:
      id: change_date
      doc: |
        Change date in the project date format (project property $%&FORMAT_DATE, default dd.mm.yyyy): the later
        of the file modification time and project property 1218 (TDateTime as decimal text, comma or point),
        which holds the previous save time.
    12:
      id: change_time
      doc: Change time as hh:nn (24-hour, locale separator) of the file modification time.
    13:
      id: output_date
      doc: Current date at print, export or redraw, in the project date format.
    14:
      id: output_time
      doc: Current time as hh:nn at print, export or redraw.
    15:
      id: scale
      doc: |
        Scale of the output in percent. Printing shows the scale actually used, with two decimals and the
        locale decimal separator (100.00% for a true-scale PDF, whatever print scale the project stores);
        outline and ODB++ exports show 100%.
    16:
      id: project_name
      doc: Project file name without directory and extension.
    100:
      id: sheet_number
      doc: Number of the schematic sheet being drawn (1..100, layer minus 100), unpadded; unexpanded on boards.
    101:
      id: property_lookup
      doc: |
        Value of the property named by the text's $%& property (trimmed, case-insensitive; a trailing * matches a
        prefix), searched on the owning component, then signal, then project properties. An empty value shows
        <empty>, no match shows <unknown> (<unbekannt> in German builds).
    102:
      id: variant_name
      doc: Name of the current assembly variant, <all> for variant 0, <unused> when unnamed.
    103:
      id: filename
      doc: Project file name with extension, without directory.
    104:
      id: type_name
      doc: Name (type) of the owning component.
    105:
      id: page_name
      doc: Name of the schematic sheet being drawn (names_1060), <unused> when empty; schematic only.
    106:
      id: variant_number
      doc: Current assembly variant number.
    107:
      id: variant_revision
      doc: Current variant revision (project property REVISION for variant 0).
    108:
      id: signal_pages
      doc: "unconfirmed: no matching corpus case."
    109:
      id: component_usage
      doc: Owning component's COMPONENT_USAGE property, <component missing> without a component.
    110:
      id: version
      doc: Version from the first logbook line starting with "Version", minus that word (default V0.0.0).
    112:
      id: maximum_page
      doc: Highest named schematic sheet.
  spice_analysis:
    0: transient
    1: ac
    2: dc
  spice_ac_sweep:
    0: lin
    1: dec
    2: oct
  text_language:
    0: german
    1: english
    2: french
  pin_electrical_type:
    0: no_connect
    1: input
    2: output
    3: bidirectional
    4: open_collector
    5: power
    6: passive
    7: high_impedance
    8:
      id: supply
      doc: Supply pin type, distinct from power (named Supply in the program's pin-type list).
  line_dash:
    0: solid
    1: long_dash
    2: dash_dot
    3:
      id: style_3
      doc: "unconfirmed: exact pattern name unresolved; present in corpus."
    4: short_dash
  fiducial_kind:
    0:
      id: crosshair_rings
      doc: |
        Called Target in the program.
        With r = radius - width / 2: circles of centreline radius r, 2r/3 and r/3, and two perpendicular
        diameters of length 2r along the mark's axes, all stroked with width (verified against vendor
        exports). direction rotates the axes; the corpus holds only 0 and 180.
    1:
      id: arrow
      doc: Called Arrow in the program; geometry unconfirmed (no corpus case).
    2:
      id: paired_sectors
      doc: Called Two quarters in the program. Ring with paired filled sectors; direction rotates the motif.
    3:
      id: filled_circle
      doc: Called Disk in the program. Solid disc of radius radius; width is unused (verified against vendor exports).
  gate_state:
    0:
      id: all_unplaced
      doc: All gates hidden/unplaced.
    1:
      id: required_missing
      doc: Some gates placed; required gates still missing.
    2:
      id: optional_missing
      doc: Only optional gates missing.
    3:
      id: all_placed_or_no_gates
      doc: All gates placed, or no gates.
  layer_function:
    0: copper_bottom
    1: copper_inside
    2: copper_top
    3: position_bottom
    4: position_top
    5:
      id: deletion_bottom
      doc: &deletion_doc |
        Regions cut from computed copper areas on the associated copper layer (layer_associations).
        Lines, rectangles, triangles and discs cut by their outline; text cuts by its glyph strokes, not its
        bounding box. Hidden (flag bit 0) and weight-zero text cuts nothing.
    6:
      id: deletion_inside
      doc: *deletion_doc
    7:
      id: deletion_top
      doc: *deletion_doc
    8:
      id: area_bottom
      doc: &area_doc |
        Copper area outlines of the signal that owns them; the fill is computed, never stored.
        An area element is filled only when its anchor (base x1, y1) lies inside the contour box,
        bounds inclusive. That box spans the defining points of the elements with flags == 0 on
        the first board_outline layer, ignoring line widths and text: line endpoints; arc anchor,
        end point, points at the start and end angles, and the axis extremes the sweep passes
        (equal angles mean a full circle); polygon corners; the anchor of anything else. Islands
        of a fill are kept even when unconnected. Area elements of one signal on one copper layer unite into
        one fill. Without any contour element no anchor test applies (no corpus case). The fill is not clipped to the
        board contour, so an
        area larger than the board fills past it. A contour stroke with clearance 0 carves nothing and
        the fill runs to the board edge. How a contour stroke with a nonzero clearance carves is not
        resolved: in the corpus, strokes 0.3 wide with clearance 0.15 or 0.3 left 0.49 to 0.51 between
        the fill and the stroke centre, and a stroke with clearance 0.5 left the area's own 0.38 inset
        intact, so neither the stroke clearance nor clearance plus half width accounts for it. Copper of another signal
        is carved out by its own clearance alone
        (line, arc and spline clearance, pad aura); a clearance of 0 carves nothing and the fill
        covers the element. Signal min_spacing plays no part.
    9:
      id: area_inside
      doc: *area_doc
    10:
      id: area_top
      doc: *area_doc
    11: solder_mask_bottom
    12: solder_mask_top
    13: drill_plan
    14: paste_bottom
    15: paste_top
    16: gold_bottom
    17: gold_top
    18: glue_bottom
    19: glue_top
    20: dimensioning
    21:
      id: board_outline
      doc: The board contour; the vendor's ODB++ profile is built from this layer alone.
    22:
      id: milling
      doc: Router tool path, typically offset outside the contour; exported as rout, not as the profile.
    23: pad_numbers
    24: other
    25: air_wires
    41: component_names_top
    42: component_values_top
    43: component_names_bottom
    44: component_values_bottom
  layer_function_default:
    0: area_bottom
    1: deletion_bottom
    2: copper_bottom
    3: gold_bottom
    4: solder_mask_bottom
    5: paste_bottom
    6: glue_bottom
    7: position_bottom
    8: area_inside
    9: deletion_inside
    14: area_top
    15: deletion_top
    16: copper_top
    17: gold_top
    18: solder_mask_top
    19: paste_top
    20: glue_top
    21: position_top
    22: dimensioning
    23: board_outline
    24: drill_plan
    25: milling
    26: pad_numbers
    27: air_wires
    28: component_values_bottom
    29: component_names_bottom
    30: component_values_top
    31: component_names_top
    100: all_copper
  elem_tag:
    1: line
    2: pin
    3: pad
    4: triangle
    5: rectangle
    6: arc
    7: disc
    8: text
    9: fiducial
    10: component
    11: symbol
    12: package
    13: signal
    14: spline
    15: drc_marker
    16: list
    17: layer_function_list
    255: nil
