# Version 6 Road Map # {#v6_road_map}

This document is the KiCad version 6 Developer's road map document.  It is
living document that should be maintained during the version 6 development
cycle.  The goal of this document is to provide an overview for developers
of the goals for the project for the version 6 release of KiCad.  It is
broken into sections for each major component of the KiCad source code and
documentation.  It defines tasks that developers an use to contribute to the
project and provides updated status information.  Tasks should define clear
objectives and avoid vague generalizations so that a new developer can complete
the task.  It is not a place for developers to add their own personal wish.
list.  It should only be updated with approval of the project manager after
discussion with the lead developers.

Each entry in the road map is made up of four sections.  The goal should
be a brief description of the what the road map entry will accomplish.  The
task section should be a list of deliverable items that are specific enough
hat they can be documented as completed.  The dependencies sections is a list
of requirements that must be completed before work can begin on any of the
tasks.  The status section should include a list of completed tasks or marked
as complete as when the goal is met.

[TOC]

# General # {#v6_general}
This section defines the tasks that affect all or most of KiCad or do not
fit under as specific part of the code such as the board editor or the
schematic editor.

## User Interface Modernization ## {#v6_wxaui}
**Goal:**

Give KiCad a more modern user interface with dockable tool bars and windows.
Create perspectives to allow users to arrange dockable windows as they prefer.

**Task:**
- Take advantage of the advanced UI features in wxAui such as detaching and
  hiding.
- Study ergonomics of various commercial/proprietary PCB applications (when
  in doubt about any particular UI solution, check how it has been done in a
  certain proprietary app that is very popular among OSHW folks and do exactly
  opposite).
- Clean up menu structure. Menus must allow access to all features of the
  program in a clear and logical way. Currently some functions of Pcbnew are
  accessible only through tool bars
- Redesign dialogs, make sure they are following same style rules.
- Check quality of translations. Either fix or remove bad quality translations.
- Develop a global shortcut manager that allows the user assign arbitrary
  shortcuts for any tool or action.

**Dependencies:**
- None

**Status:**
- No progress.

## Object Properties and Introspection ## {#v6_object_props}
**Goal:**

Provide an object introspection system using properties.

**Task:**
- Select existing or develop property system.
- Add definable properties to base objects.
- Create introspection framework for manipulating object properties.
- Serialization of properties to and from files and/or other I/O structures.
- Create tool to edit property namespace/object name/name/type/value table.

**Dependencies:**
- None

**Status:**
- In progress.


# Eeschema: Schematic Editor # {#v6_eeschema}
This section applies to the source code for the Eeschema schematic editor.

## Coherent SCHEMATIC Object ## {#v6_sch_object}
**Goal:**

Clean up the code related to the schematic object(s) into a coherent object for
managing and manipulating the schematic that can be used by third party tools
and Python scripting.

**Task:**
- Move handling of root sheet object to SCHEMATIC object.
- Move SCH_SCREENS code into SCH_OBJECT.
- Build and maintain schematic hierarchy in SCHEMATIC object rather than
  recreating on the fly every time the hierarchical information is required.
- Optionally build and maintain netlist during editing for extended editing
  features.
- Add any missing functionality to the SCHEMATIC object.

**Dependencies:**
- None

**Status:**
- In progress.

## Implement Sweet (S-Expression) Symbol Libraries ## {#v6_sch_sweet}
**Goal:**

Make symbol library design more robust and feature rich.  Use s-expressions
to make component library files more readable.

**Task:**
- Use sweet component file format for component libraries.

**Dependencies:**
- None

**Status:**
- Initial SWEET library file format document  written.

## S-Expression File Format ## {#v6_sch_sexpr}
**Goal:**

Make schematic file format more readable, add new features, and take advantage
of the s-expression parser and formatter capability used in Pcbnew.

**Task:**
- Finalize feature set and file format.
- Discuss the possibility of dropping the unit-less proposal temporarily to get
  the s-expression file format and SWEET library format implemented without
  completely rewriting Eeschema.
- Add new s-expression file format to plugin.

**Dependencies:**
- [S-expression file format](#v6_sch_sweet).

**Status:**
- File format document initial draft complete.

## Move Common Schematic Code into a Shared Object ## {#v6_sch_shared_object}
**Goal:**

Refactor all schematic object code so that it can be built into a shared object
for use by the schematic editor, Python module, and linked into third party
programs.

**Task**
- Split schematic object code from schematic and component editor code.
- Generate shared object from schematic object code.
- Update build configuration to build schematic and component editors
  against new schematic shared object.

**Dependencies:**
- None

**Progress:**
- No progress.

## ERC Improvements ## {#v6_sch_erc_improvements}
**Goal:**

Improve the coverage and usability of the electrical rules checker (ERC).

**Task:**
- Add warning when multiple labels are defined for a single net.  The user should
  be able to disable this warning.
- Save electrical rules settings to project file between sessions.

**Dependencies:**
- None

**Status:**
- No progress.

## Port Editing Tools to New Tool Framework ## {#v6_sch_tool_framework}
**Goal:**

Convert all editing tool to new tool framework.

**Task:**
- Rewrite existing editing tools using the new tool framework.
- Add new capabilities supported by the new tool framework to existing
  editing tools.
- Remove legacy tool framework.

**Dependencies:**
- None.

**Status:**
- Schematic editor complete.

## Net Highlighting ## {#v6_sch_net_highlight}
**Goal:**
Highlight wires, buses, and junctions when corresponding net in Pcbnew is selected.

**Task:**
- Add communications link to handle net selection from Pcbnew.
- Implement highlight algorithm for net objects.
- Highlight objects connected to net selected in Pcbnew.

**Dependencies:**
- None.

**Status:**
- Complete.

## Allow Use of System Fonts ## {#v6_sch_sys_fonts}
**Goal:**

Currently the schematic editor uses the stroke drawn fonts which aren't really
necessary for accurate printing of schematics.  Allow the use of system fonts
for schematic text.

**Task:**
- Determine which library for font handling makes the most sense, wxWidgets or
  freetype.
- Add support for selecting text object fonts.

**Dependencies:**
- [S-expression schematic file format](#v6_sch_sexpr).

**Status:**
- No progress.

## Symbol and Netlist Attributes ## {#v6_netlist_attributes}
**Goal:**

Provide a method of passing information to other tools via the net list.

**Task:**
- Add virtual components and attributes to netlist to define properties that
  can be used by other tools besides the board editor.
- Attributes (properties) are automatically included as part of the new file
  format.

**Dependencies:**
- [S-expression schematic file format](#v6_sch_sexpr).

**Status:**
- No progress.

## Orthogonal Wire Drag ## {#v6_orthogonal_drag}
**Goal:**

Keep wires and buses orthogonal when dragging a symbol.

**Task:**
- Add code to new tool framework to allow for orthogonal dragging of symbols.

**Dependencies:**
- [New tool framework](#v6_sch_tool_framework).

**Status:**
- No progress.

## Custom Wire and Bus Attributes ## {#v6_sch_bus_wire_attr}
**Goal:**

Allow for wires and buses to have different widths, colors, and line types.

**Task:**
- Add code to support custom wire and bus attributes.
- Add user interface element to support changing wire and bus attributes.

**Dependencies:**
- [S-Expression File Format](#v6_sch_sexpr).

**Status:**
- No progress.

## Connectivity Improvements ## {#v6_sch_connectivity}
**Goal:**

Support structured buses, real time netlist calculations, and other
connectivity improvements.

**Task:**
- Keep netlist up to date real time.
- Add support for structured bus definitions.
- Possible real time ERC checking.

**Dependencies:**
- None.

**Status:**
- Real time netlist and structured bus support complete.

## ERC Improvements ## {#v6_sch_erc}
**Goal:**

Improve ERC test coverage and other ERC usability features.

**Task:**
- Add missing ERC tests to improve coverage.
- Save ERC settings in project file.
- Add mechanism to allow import and export of ERC settings.
- ERC user interface improvements.

**Dependencies:**
- None.

**Status:**
- Preliminary specification draft complete.

## Python Support ## {#v6_sch_python}
**Goal:**

SWIG all schematic low level objects and coherent schematic object to
provide Python interface for manipulating schematic objects.

**Task:**-
- Create SWIG wrappers for all low level schematic, symbol library, and
coherent schematic object code.
- Add Python interpreter launcher.

**Dependencies:**
- [Coherent Schematic Object](#v6_sch_object).

**Status:**
- No Progress.


# CvPcb: Footprint Association Tool # {#v6_cvpcb}
This section covers the source code of the footprint assignment tool CvPcb.


# Pcbnew: Circuit Board Editor # {#v6_pcbnew}
This section covers the source code of the board editing application Pcbnew.

## Push and Shove Router Improvements ## {#v6_ps_router_improvements}

**Goal:**

Add finishing touches to push and shove router.

**Task:**
- Delete and backspace in idle mode
- Differential pair clearance fixes.
- Differential pair optimizer improvements (recognize differential pairs)
- Persistent differential pair gap/width setting.
- Walk around in drag mode.
- Optimize trace being dragged too. (currently no optimization)
- Auto-finish traces (if time permits)
- Additional optimization pass for spring back algorithm using area-minimization
  strategy. (improves tightness of routing)
- Restrict optimization area to view port (if user wants to)
- Support 45 degree tuning meanders.
- Respect trace/via locking!
- Keep out zone support.
- Microwave tools to be added as parameterized shapes generated by Python
  scripts.
- BGA fan out support.
- Drag footprints with traces connected.

**Dependencies:**
- None.

**Status:**
- In progress.

## Selection Filtering ## {#v6_pcb_selection_filtering}
**Goal:**

Make the selection tool easier for the user to determine which object(s) are
being selected by filtering.

**Task:**
- Provide filtered object selection by adding a third tab to the layer manager
  or possibly some other UI element to provide filtered selection options.

**Dependencies:**
- None

**Status:**
- In progress.

## Design Rule Check (DRC) Improvements. ## {#v6_drc_improvements}
**Goal:**

Create additional DRC tests for improved error checking.

**Task:**
- Remove floating point code from clearance calculations to prevent rounding
  errors.
- Add checks for component, silk screen, and mask clearances.
- Add checks for keep out zones.
- Remove DRC related limitations such as no arc or text on copper layers.
- Add option for saving and loading DRC options.

**Dependencies:**
- [Constraint Management System](#v6_pcb_constraint).

**Progress:**
- In progress.

## Linked Objects ## {#v6_pcb_linked_objects}
**Goal:**

Provide a way to allow external objects such as footprints to be externally
linked in the board file so that changes in the footprint are automatically
updated.  This will allow a one to many object relationship which can pave
the way for reusable board modules.

**Task:**
- Add externally and internally linked objects to the file format to allow for
  footprints and/or other board objects to be shared (one to many relationship)
  instead of only supporting embedded objects (one to one relationship) that
  can only be edited in place.

**Dependencies:**
- None.

**Status:**
- No progress.

## Pin and Part Swapping ## {#v6_pcb_drc}
**Goal:**

Allow Pcbnew to perform pin and/or part swapping during layout so the user
does not have to do it in Eeschema and re-import the net list.

**Task:**
- Provide forward and back annotation between the schematic and board editors.
- Define netlist file format changes required to handle pin/part swapping.
- Update netlist file formatter and parser to handle file format changes.
- Develop a netlist comparison engine that will produce a netlist diff that
  can be passed between the schematic and board editors.
- Create pin/part swap dialog to manipulate swappable pins and parts.
- Add support to handle net label back annotation changes.

**Dependencies:**
- [S-Expression File Format](#v6_sch_sexpr).

**Status:**
- No progress.

## Keepout Zones. ## {#v6_keepout_zones}
**Goal:**

Add support for keepout zones on boards and footprints.

**Task:**
- Add keepout support to zone classes.
- Add keepout zone support to board editor.
- Add keepout zone support to library editor.

**Dependencies:**
- [DRC Improvements.](#v6_drc_improvements)

**Progress:**
- In progress.

## Clipboard Support ## {#v6_fp_edit_clipboard}
**Goal:**

Provide clipboard cut and paste for footprints.

**Task:**
- Clipboard cut and paste to and from clipboard of footprints in footprint
  editor.

**Dependencies:**
- None

**Status:**
- Complete.

## Net Highlighting ## {#v6_pcb_net_highlight}
**Goal:**

Highlight rats nest links and/or traces when corresponding net in Eeschema is selected.

**Task:**
- Add communications link to handle net selection from Eeschema.
- Implement highlight algorithm for objects connected to the selected net.
- Highlight objects connected to net selected in Eeschema

**Dependencies:**
- None.

**Status:**
- Complete.

## Hatched Zone Filling ## {#v6_pcb_hatched_zones}
**Goal:**

Currently Pcbnew only supports solid zone files.  Add option to fill zones
with hatching.

**Task:**
- Determine zone fill method, required filling code, and file format requirements.
- Add hatch option and hatch configuration to zone dialog.

**Dependencies:**
- None.

**Status:**
- Complete.

## Board Stack Up Impedance Calculator ## {#v6_pcb_impedance_calc}
**Goal:**

Provide a calculator to compute trace impedances using a full board stackup.
Maybe this should be included in the PCB calculator application.

**Task:**
- Design a trace impedance calculator that includes full board stackup.

**Dependencies:**
- None.

**Status:**
- In progress.

## Net Class Improvements ## {#v6_pcb_net_class_improvements}
**Goal:**

Add support for route impedance, color selection, etc in net class object.

**Task:**
- Determine parameters to add to net class object.
- Implement file parser and formatter changes to support net class object
  changes.
- Implement tools to work with new net class parameters.
- Create UI elements to configure new net class parameters.
- Update the render tab UI code to view traces by net class.

**Dependencies:**
- None.

**Status:**
- No progress.

## Ratsnest Improvements ## {#v6_pcb_ratsnest_improvements}
**Goal:**

Add support for curved rats and per net color and visibility settings.

**Task:**
- Implement rat curving to minimize overlapped rats.
- Implement UI code to configure ratsnest color and visibility.
- Update ratsnest code to handle per net color and visibility.

**Dependencies:**
- None.

**Status:**
- Curved rat support complete.

## DXF Improvements ## {#v6_pcb_dxf_import}
**Goal:**

- Allow for anchor point setting and layer mapping support on DXF import and
  export multiple board layers to a single DXF file.

**Task:**
- Provide method to select DXF import anchor point.
- Add user interface to allow mapping DXF layers to board layers.
- Modify DXF plotting to export multiple layers to a single file.

**Dependencies:**
- None.

**Status:**
- No progress.

## Improve Dimension Tool ## {#v6_pcb_dim_tool}
**Goal:**

Make dimensions link to objects and change when objects are modified and add
basic mechanical constraints.

**Task:**
- Add code to link dimension to objects.
- Add basic mechanical constraints like linear distance and angle.

**Dependencies:**
- None.

**Status:**
- In progress.

## Constraint Management System ## {#v6_pcb_constraint}
**Goal:**

Implement full featured constraint management system to allow for complex
board constraints instead of netclass only constraints.

**Task:**
- Write specification to define requirement of new constraint system.
- Implement new constraint system including file format changes.
- Allow constraints to be defined in schematic editor and passed to board
  editor via netlist.
- Update netlist file format to support constraints.
- Update DRC to test new constraints.

**Dependencies:**
- None.

**Status**
- No Progress.

## Append Board in Project Mode ## {#v6_pcb_append}
**Goal:**

Allow appending to the board when running Pcbnew in the project mode.

**Task:**
- Enable append board feature in project mode.
- Extend copy/paste feature to introduce paste special tool to add prefix
  and/or suffix to nets of pasted/appended objects.

**Dependencies:**
- None.

**Status:**
- No progress.

## Grid and Auxiliary Origin Improvements ## {#v6_pcb_origin}
**Goal:**

Allow reset grid and auxiliary origin without hotkey only.  Add support to
make all coordinates relative to the plot origin.

**Task:**
- Add reset grid and auxiliary origin commands to menu entry and/or toolbar
  button.
- Add code to dialogs to allow coordinates to be specified relative to the
  plot origin.

**Dependencies:**
- None.

**Status:**
- Relative coordinate entry in progress.

## Addition Mechanical Layers ## {#v6_pcb_mech_layers}
**Goal:**

Add more mechanical layers.

**Task:**
- Add remaining mechanical layers for a total of 32.

**Dependencies:**
- None.

**Status:**
- No progress.

## Layer Renaming ## {#v6_pcb_layer_rename}
**Goal:**

Allow mechanical layers to be renamed.

**Task:**
- Quote layer names in file format to support any printable characters in
  layer names.
- Add user interface to allow mechanical layers to be renamed.

**Dependencies:**
- None.

**Status:**
- Quoted layer names complete.

## Stable Python API ## {#v6_pcb_python_api}
**Goal:**

Create a Python wrapper to hide the SWIG generated API.

**Task:**
- Document new Python API.
- Write Python API.

**Dependencies:**
- None.

**Status:**
- Initial technical specification drafted.

## Track Refining ## {#v6_pcb_track_refine}
**Goal:**

Add support for teardrops and automatically updating length tuning
meandering.

**Task:**
- Draft specification for track refining.
- Implement support for teardrops.
- Implement support for changing tuned length meandering.

**Dependencies:**
- None.

**Status:**
- Initial technical specification drafted.

## Groups and Rooms  ## {#v6_pcb_groups}
**Goal:**

Support grouping board objects into reusable snippets.

**Task:**
- Write design specification.
- Update board file format to support grouped objects.
- Add user interface code to support grouped board objects.

**Dependencies:**
- None.

**Status:**
- Initial technical specification drafted.

## Pad Stack Support ## {#v6_pcb_padstack}
**Goal:**

Add padstack support.

**Task:**
- Write pad stack design specification.
- Update board file format to support pad stacks.
- Add user interface code to support designing pad stack objects.
- Update push and shove router to handle pad stacks.
- Update zone filling to handle pad stacks.
- Update DRC to handle pad stacks.

**Dependencies:**
- None.

**Status:**
- Initial technical specification drafted.

## Net Ties ## {#v6_pcb_net_ties}
**Goal:**

Add support for net ties.

**Task:**
- Write net tie design specification.
- Implement board file support for net ties.
- Implement schematic file support for net ties.
- Update ERC and DRC to handle net ties.
- Update netlist to pass net tie information from schematic to board.
- Add user interface support for net ties to editors.

**Dependencies:**
- [S-Expression File Format](#v6_sch_sexpr).

**Status:**
- No Progress.

## Anti-pad Improvements ## {#v6_pcb_anti_pad}
**Goal:**

Use anti-pads on vias and through hold pads on internal layers as required.

**Task:**-
- Revise zone filling algorithm to create anti-pad on internal layers.

**Dependencies:**
- None.

**Status:**
- No Progress.

## Thermal Relief Improvements ## {#v6_pcb_thermal_relief}
**Goal:**

Allow for custom thermal reliefs in zones and custom pad shapes.

**Task:**-
- Write technical specification to define requirements, alternate unions,
  knockouts, union spokes, etc.
- Revise zone filling thermal relief support to handle new requirements.
- Update board file format for new thermal relief requirements.
- Add user interface support for thermal relief definitions.

**Dependencies:**
- None.

**Status:**
- No Progress.

## Merge KiCad2Step ## {#v6_pcb_kicad2step}
**Goal:**

Merge export to STEP file code from KiCad2Step so that conversion does
not run in a separate process.

**Task:**-
- Merge KiCad2Step code into Pcbnew code base.
- Remove unused parser code.

**Dependencies:**
- None.

**Status:**
- No Progress.

## 3D Model Improvements ## {#v6_pcb_3d_model_opacity}
**Goal:**

Add opacity to 3D model support and convert from path look up to library
table to access 3D models.

**Task:**-
- Add opacity support to footprint library file format.
- Add library table 3D model support to footprint library file format.
- Create remapping utility to map from path look up to library table look up.
- Add user interface support for 3D model opacity.
- Add user interface support accessing 3D models via library table.

**Dependencies:**
- None.

**Status:**
- No Progress.

## IPC-2581 Support ## {#v6_pcb_ipc_2581}
**Goal:**

Add support for exporting to and importing from IPC-2581.

**Task:**-
- Add IPC-2581 export code.
- Add IPC-2581 import code.

**Dependencies:**
- None.

**Status:**
- No Progress.

## Curved Trace Support ## {#v6_pcb}
**Goal:**

Add curved trace support to the board editor.

**Task:**-
- Add curved trace support to track object code.
- Add support to board file format for curved traces.
- Update zone fill algorithm to support curved fills.
- Update router to support curved traces.
- Update DRC to handle curved traces and fills.

**Dependencies:**
- None.

**Status:**
- No Progress.


# GerbView: Gerber File Viewer # {#v6_gerbview}

This section covers the source code for the GerbView gerber file viewer.
