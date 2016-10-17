# Version 5 Road Map # {#v5_road_map}

This document is the KiCad version 5 Developer's road map document.  It is
living document that should be maintained during the version 5 development
cycle.  The goal of this document is to provide an overview for developers
of the goals for the project for the version 5 release of KiCad.  It is
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

# Project # {#v5_project}
This section defines the tasks for the project related goals that are not
related to coding or documentation.  It is a catch all for issues such as
developer and user relations, dissemination of information on websites,
policies, etc.


# General # {#v5_general}
This section defines the tasks that affect all or most of KiCad or do not
fit under as specific part of the code such as the board editor or the
schematic editor.

## User Interface Modernization ## {#v5_wxaui}
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

## Search Tree Control ## {#v5_re_search_control}

**Goal:**
Create a user interface element that allows searching through a list of
items in a tree control for library searching.

**Task:**
- Create hybrid tree control with search text control for displaying filtered
  objects (both symbol and footprint libraries) in a parent window.

**Dependencies:**
- None

**Status:**
- Initial container searching code completed.
- Wildcard and regular expression container searching completed.
- Control code in progress.

# Common Library # {#v5_common_lib}
This section covers the source code shared between all of the KiCad
applications

## Unified Rendering Framework ## {#v5_unified_rendering}
**Goal:**

Provide a single framework for developing new tools.  Port existing tools
to the new framework and remove the legacy framework tools.

**Task:**
- Port wxDC to GAL or get Cairo rendering to nearly the performance of the
  current wxDC rendering so that we have a single framework to develop new
  tools and we can continue to support systems that don't have a complete
  OpenGL stack.

**Dependencies**
- [Tool framework](http://www.ohwr.org/projects/cern-kicad/wiki/WorkPackages)

**Status**
- In progress

## Printing Improvements ## {#v5_print}
**Goal:**

Make printing quality consistent across platforms.

**Task:**
- Resolve printing issues on all platforms.

**Dependencies**
- None

**Status**
- No progress.

## Object Properties and Introspection ## {#v5_object_props}
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
- No progress.

## 3D Viewer Dynamic Library Plugin ## {#v5_plugin_base}
**Goal:**

Create a base library plugin for handling external file I/O for the 3D viewer.
This will allow plugins to be provided that are external to the project such
as providing solid model file support (STEP, IGES, etc.) using OpenCascade
without making it a project dependency.

**Task:**
- Create a plugin to handle dynamically registered plugins for loading and
  saving file formats.
- This object should be flexible enough to be extended for handling all file
  plugin types including schematic, board, footprint library, component
  library, etc. (optional)
- See [blueprint](https://blueprints.launchpad.net/kicad/+spec/pluggable-file-io)
  on Launchpad for more information.

**Dependencies:**
- None

**Status:**
- 3D plugin code complete and legacy formats implemented.


# Eeschema: Schematic Editor # {#v5_eeschema}
This section applies to the source code for the Eeschema schematic editor.

## Coherent SCHEMATIC Object ## {#v5_sch_object}
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
- [Schematic and Component Library Plugin](#v5_sch_plugin)

**Status:**
- In progress.

## Schematic and Component Library I/O Manager Plugin ## {#v5_sch_plugin}
**Goal:**
Create a plugin manager for loading and saving schematics and component
libraries similar to the board plugin manager.

**Task:**
- Design plugin manager for schematics and component libraries.
- Port the current schematic and component library file formats to use the
  plugin.

**Dependencies:**
- None

**Status:**
- I/O manager and plugin objects are complete.
- Legacy schematic file parser almost ready to commit.


## S-Expression File Format ## {#v5_sch_sexpr}
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
- [Schematic and component I/O manager plugin](#v5_sch_plugin)

**Status:**
- File format document nearly complete.

## Implement Sweet Component Libraries ## {#v5_sch_sweet}
**Goal:**

Make component library design more robust and feature rich.  Use s-expressions
to make component library files more readable.

**Task:**
- Use sweet component file format for component libraries.

**Dependencies:**
- [S-expression file format](#v5_sch_sexpr).
  [Schematic and component I/O manager plugin](#v5_sch_plugin)

**Status:**
- Initial SWEET library file format document  written.

## Component Library Editor Usability Improvements ## {#v5_lib_editor_usability}
**Goal:**

Make editing schematic symbol libraries easier to manage.

**Task:**
- Determine usability improvements in the library editor for components with
  multiple units and/or alternate graphical representations.
- Replace current library/symbols selection process with new hybrid tree search
  widget in new window pain for selection libraries and symbols.  Provide drag
  and drop symbol copy/move between libraries.
- Allow editing of symbol libraries not defined in footprint library table(s)
  using the file/path dialog to open a library.

**Dependencies:**
- [Search Tree Control](#v5_re_search_control)

**Status:**
- Determined alternate UI designs using new hybrid search tree control.

## Component and Netlist Attributes ## {#v5_netlist_attributes}
**Goal:**

Provide a method of passing information to other tools via the net list.

**Task:**
- Add virtual components and attributes to netlist to define properties that
  can be used by other tools besides the board editor.
- Attributes (properties) are automatically included as part of the new file
  format.

**Dependencies:**
- [S-expression schematic file format](#v5_sch_sexpr).

**Status:**
- No progress.

## Move Common Schematic Code into a Shared Object ## {#v5_sch_shared_object}
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

## ERC Improvements ## {#v5_sch_erc_improvements}
**Goal:**

Improve the coverage and useability of the electrical rules checker (ERC).

**Task:**
- Add warning when multiple labels are defined for a single net.  The user should
  be able to disable this warning.
- Save electrical rules settings to project file between sessions.

**Dependencies:**
- None

**Status:**
- No progress.

# CvPcb: Footprint Association Tool # {#v5_cvpcb}
This section covers the source code of the footprint assignment tool CvPcb.

## Improved Footprint Search Tool ## {#v5_cvpcb_search}

**Goal:**

Provide advanced search features such as wild card and regular expression
searches using the type as you go feature of the current search dialog.

**Task:**
- Add code for wild card and regular expression pattern matching to search
  container objects.
- Add search dialog to CvPcb to search container of footprint names.

**Dependencies:**
- [Search Tree Control](#v5_re_search_control)

**Status:**
- Pattern matching added to search container objects.

# Circuit Simulation # {#simulation}
**Goal:**

Provide quality circuit simulation capabilities similar to commercial products.

**Task:**
- Evaluate and select simulation library (ngspice, gnucap, qucs, etc).
- Evaluate and select plotting library with wxWidgets support.
- Confirm current spice netlist export is up to the task and add missing
  support for simulations.
- Use plotting library to handle simulator output in a consistent manor similar
  to LTSpice.
- Develop a tool that allows fine tuning of components on the fly.
- Use plugin for the simulation code to allow support of different simulation
  libraries.
- Create dialogs for configuring of simulation of Spice primitive components
  such as voltage sources, current sources, etc.
- Create dialog(s) for configuration of simulation types transient, DC operating
  point, AC analysis, etc.

**Dependencies:**
- None

**Status:**
- Done ([announcement message](https://lists.launchpad.net/kicad-developers/msg25483.html))


# Pcbnew: Circuit Board Editor # {#v5_pcbnew}
This section covers the source code of the board editing application Pcbnew.

## Tool Framework ## {#v5_pcb_tool_framework}
**Goal:**

Unify all board editing tools under a single framework.

**Task:**
- Drop footprint edit mode.
- Port auto-router to GAL.
- Complete porting of all board editing tools to new tool framework so they
  are available in the OpenGL and Cairo canvases.
- Remove all duplicate legacy editing tools.

**Dependencies:**
- In progress.

**Status:**
- Initial porting work in progress.

## Modeling ## {#v5_modeling}

**Goal:**

Provide improved solid modeling support for KiCad including the file formats
available in OpenCascade.

**Task:**
- Improve low level code design.
- Design plugin architecture to handle loading and saving 3D models.
- Back port existing 3D formats (IDF and S3D) to plugin

**Dependencies:**
- [Dynamic library plugin](#v5_plugin_base).

**Status:**
- Completed.

## Push and Shove Router Improvements ## {#v5_ps_router_improvements}

**Goal:**

Add finishing touches to push and shove router.

**Task:**
- Microwave tools to be added as parametrized shapes generated by Python
  scripts.
- Determine which features are feasible.
- Factor out KiCad-specific code from PNS_ROUTER class.
- Delete and backspace in idle mode
- Differential pair clearance fixes.
- Differential pair optimizer improvements (recognize differential pairs)
- Persistent differential pair gap/width setting.
- Walk around in drag mode.
- Optimize trace being dragged too. (currently no optimization)
- Backspace to erase last routed segment.
- Auto-finish traces (if time permits)
- Additional optimization pass for spring back algorithm using area-minimization
  strategy. (improves tightness of routing)
- Restrict optimization area to view port (if user wants to)
- Support 45 degree tuning meanders.
- Respect trace/via locking!
- Keep out zone support.

**Dependencies:**
- None

**Status:**
- Feature feasibility determined.
- In Progress.

## Selection Filtering ## {#v5_pcb_selection_filtering}
**Goal:**

Make the selection tool easier for the user to determine which object(s) are
being selected by filtering.

**Task:**
- Provide filtered object selection by adding a third tab to the layer manager
  or possibly some other UI element to provide filtered selection options.

**Dependencies:**
- None

**Status:**
- Initial design concept discussed.

## Design Rule Check (DRC) Improvements. ## {#v5_drc_improvements}
**Goal:**

Create additional DRC tests for improved error checking.

**Task:**
- Replace geometry code with [unified geometry library](#v5_geometry_lib).
- Remove floating point code from clearance calculations to prevent rounding
  errors.
- Add checks for component, silk screen, and mask clearances.
- Add checks for keep out zones.
- Remove DRC related limitations such as no arc or text on copper layers.
- Add option for saving and loading DRC options.

**Dependencies:**
- [Unified geometry library.](#v5_geometry_lib)

**Progress:**
- In progress.

## Segment End Point Snapping. ## {#v5_segment_snapping}
**Goal:**

It is not uncommon for board edge segment end points to inadvertently not
be closed causing issues for the 3D viewer and exporting to different file
formats due the board outline not being a fully enclosed polygon.  This
feature would add segment end snapping support to allow the board outline
to be fully enclosed.  This feature would only need to be supported by the
GAL rendering.

**Tasks**
- Mark board edge segment ends with a drag indicator to make it visible to the
  user that the segment end does not have an endpoint with any other board edge
  segment.
- Allow the user to snap the unconnected segment end to the nearest segment end
  point.
- Automatically connect unconnected segments with and additional segment when
  opening the 3D viewer or exporting the board to another format.  Warn the
  user that an addition segment has be added and should be verified.

**Dependencies:**
- None

**Progress:**
- Initial discussion.

## Net Highlighting ## {#v5_pcb_net_highlight}
**Goal:**

Highlight rats nest links and/or traces when corresponding net in Eeschema is selected.

**Task:**
- Add communications link to handle net selection from Eeschema.
- Implement highlight algorithm for objects connected to the selected net.
- Highlight objects connected to net selected in Eeschema

**Dependencies:**
- None.

**Status:**
- No progress.

## Complex Pad Shapes ## {#v5_pcb_complex_pads}
**Goal:**

Add capability to create complex pad shapes from existing primitives such as arcs,
segments, and circles or polygons.

**Task:**
- Add new complex pad type.
- Add code to load and save complex pad type.
- Add code to convert complex pad type to polygon for DRC testing.
- Add code to DRC to support complex pad types.
- Add code to footprint editor to create complex pad types.

**Dependencies:**
- [Unified geometry library.](#v5_geometry_lib)

**Progress:**
- In progress.


## Stitching Via Support ## {#v5_pcb_stitching_vias}
**Goal:**

Add capability to add vias for stitching and thermal transfer purposes
that do not require being attached to tracks.

**Task:**
- Develop more robust connectivity checking algorithm.
- Create a UI element to allow the user to select a net from the list of
  valid nets.
- Connection propagation fix for the current issue of vias that are not
  connected to tracks being tagged as unassigned and removed.
- Manual via placement tool.
- Improve the DRC to handle cases of orphaned vias.


**Dependencies:**
- None

**Progress:**
- Patch available to fix via propagation issue.


# Documentation # {#v5_documentation}
This section defines the tasks for both the user and developer documentation.

## Grammar Check ## {#v5_doc_grammar}
**Goal:**

Improve user documentation readability and make life easier to for translators.

**Task:**
- Review and revise all of the English documentation so that it is update with
  the current functionality of the code.
- Translate the update documentation into other languages.

**Dependencies:**
- None

**Status:**
- No progress.

## Maintenance ## {#v5_doc_maintenance}
**Task:**
- Keep screen shots current with the source changes.

**Dependencies:**
- None.

**Status:**
- No progress.

## Convert Developer Documentation to Markup/down Format ## {#v5_dev_doc_format}
**Goal:**

Improve developers documentation to make life easier for new developers to get
involved with the project.

**Task:**
- Convert platform build instructions from plain text to new format to be
  merged with the developer documentation.
- Convert how to contribute to KiCad instructions from plain text to the new
  format to merged with the developer documentation.

**Dependencies:**
- None.

**Status:**
- In progress.  Most of the developer documentation has been converted to
  [Doxygen markdown](http://www.stack.nl/~dimitri/doxygen/manual/markdown.html)
  and the [output][kicad-docs] is rebuilt automatically when a commit is
  made to the KiCad repo.

[kicad-website]:http://kicad-pcb.org/
[kicad-docs]:http://ci.kicad-pcb.org/job/kicad-doxygen/ws/Documentation/doxygen/html/index.html
