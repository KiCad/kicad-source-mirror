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

## Create Schematic Code Shared Object. ## {#v5_kiway}
**Goal:**

Merge common schematic code into to a separate shared object to allow access
to schematic objects by third party code and Python support.

**Task:**

**Dependencies:**
- None

**Status:**
- No Progress.

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
- [wxWidgets 3](#wxwidgets3)

**Status:**
- No progress.

## Regular Expression Library Search ## {#v5_gen_lib_reg_ex}

**Goal:**
Add regular expression and wildcard searching to component and footprint
library search dialogs.

**Task:**
- Add implementation to component and footprint search dialogs.

**Dependencies:**
- None

**Status:**
- Initial container searching code completed.
- Searching implemented in component library search dialog.
- Needs to be added to footprint library search dialog in CvPcb and Pcbnew.

# Build Tools # {#v5_build_tools}
This section covers build tools for both the KiCad source as well as the
custom dependency builds required to build KiCad.

## Create Separate Build Dependency Project ## {#v5_depends_prj}
**Goal:**

Move the library dependencies and their patches into a separate project to
developers to build and install them as required instead of requiring them
at build time.  Give developers the flexibility to build and/or install
library dependencies as they see fit.  Remove them from the KiCad source code
to reduce the build footprint.

**Task:**
- Create a separate project to build all external dependency libraries that are
  currently build from source (Boost, OpenSSL, etc).
- Use CMake to create a package configuration file for each library so the
  KiCad find package can pull in header paths, library dependencies, compile
  flags, and link flags to build KiCad.
- Use CMake find package to pull external dependencies.
- Remove all build from source dependencies for KiCad source code.

**Dependencies:**
- None

**Status:**
- In progress.


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

## Linux Printing Improvements ## {#v5_linux_print}
**Goal:**

Bring printing on Linux up to par with printing on Windows.

**Task:**
- Resolve Linux printing issues.

**Dependencies**
- [wxWidgets 3](#wxwidgets3)

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

## Dynamic Library Plugin ## {#v5_plugin_base}
**Goal:**

Create a base library plugin for handling external file I/O.  This will allow
plugins to be provided that are external to the project such as providing solid
model file support (STEP, IGES, etc.) using OpenCascade without making it a
project dependency.

**Task:**
- Create a plugin to handle dynamically registered plugins for loading and
  saving file formats.
- This object should be flexible enough to be extended for handling all file
  plugin types including schematic, board, footprint library, component
  library, etc.
- See [blueprint](https://blueprints.launchpad.net/kicad/+spec/pluggable-file-io)
  on Launchpad for more information.

**Dependencies:**
- None

**Status:**
- No progress.


# KiCad: Application Launcher # {#v5_kicad}
This section applies to the source code for the KiCad application launcher.


# Eeschema: Schematic Editor # {#v5_eeschema}
This section applies to the source code for the Eeschema schematic editor.

## Coherent SCHEMATIC Object ## {#v5_sch_object}
**Goal:**

Clean up the code related to the schematic object(s) into a coherent object for
managing and manipulating the schematic.

**Task:**
- Move most if not all of the code from SCH_SCREEN to the new SCHEMATIC object.
- Add any missing functionality to the SCHEMATIC object.

**Dependencies:**
- None

**Status:**
- In progress.

## Hierarchical Sheet Design ## {#v5_hierarchy_fix}
**Goal:**

Create a more robust sheet instance design rather than recreating them on the
fly every time sheet information is required.

**Task:**
- Choose a data structure to contain the sheet hierarchy.
- Create helper class to manipulate the hierarchy data structure.

**Dependencies:**
- None

**Status:**
- In progress.

## Schematic and Component Library Plugin ## {#v5_sch_plugin}
**Goal:**
Create a plugin manager for loading and saving schematics and component
libraries similar to the board plugin manager.

**Task:**
- Design plugin manager for schematics and component libraries.
- Port the current schematic and component library file formats to use the
  plugin.

**Dependencies:**
- [Dynamic library plugin](#v5_plugin_base)

**Status:**
- No progress.

## Graphics Abstraction Layer Conversion ## {#v5_sch_gal}
**Goal:**

Take advantage of advanced graphics rendering in Eeschema.

**Task:**
- Port graphics rendering to GAL.

**Dependencies:**
- None

**Status:**
- No progress.

## Port Editing Tools ## {#v5_sch_tool_framework}
**Goal:**

Use standard tool framework across all applications.

**Task:**
- Rewrite editing tools using the new tool framework.

**Dependencies:**
- [GAL port](#v5_sch_gal).

**Status:**
- Initial Discussion..

## S-Expression File Format ## {#v5_sch_sexpr}
**Goal:**

Make schematic file format more readable, add new features, and take advantage
of the s-expression capability used in Pcbnew.

**Task:**
- Finalize feature set and file format.
- Discuss the possibility of dropping the unit-less proposal temporarily to get
  the s-expression file format and SWEET library format implemented without
  completely rewriting Eeschema.
- Add new s-expression file format to plugin.

**Dependencies:**
- [Dynamic library plugin](#v5_plugin_base).

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

**Status:**
- Initial SWEET library written.

## Component Library Editor Usability Improvements ## {#v5_lib_editor_usability}
**Goal:**

Make editing components with multiple units and/or alternate graphical
representations easier.

**Task:**
- Determine usability improvements in the library editor for components with
  multiple units and/or alternate graphical representations.
- Implement said usability improvements.

**Dependencies:**
- None.

**Status:**
- No progress.

## Component and Netlist Attributes ## {#v5_netlist_attributes}
**Goal:**
Provide a method of passing information to other tools via the net list.

**Task:**
- Add virtual components and attributes to netlist to define properties that
  can be used by other tools besides the board editor.

**Dependencies:**
- [S-expression schematic file format](#v5_sch_sexpr).

**Status:**
- No progress.

## Net Highlighting ## {#v5_sch_net_highlight}
**Goal:**
Highlight wires, buses, and junctions when corresponding net in Pcbnew is selected.

**Task:**
- Add communications link to handle net selection from Pcbnew.
- Implement highlight algorithm for net objects.
- Highlight objects connected to net selected in Pcbnew.

**Dependencies:**
- [GAL port, maybe](#v5_sch_gal).

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
- None

**Status:**
- Pattern matching added to search container objects.

## Add Progress Dialog on Start Up ## {#v5_cvpcb_progress}

**Goal:**
Provide user feedback when loading footprint libraries are loading after
start up.

**Task:**
- Create a progress dialog to show the percentage of libraries loaded.

**Dependencies:**
- None

**Status:**
- No Progress

# Pcbnew: Circuit Board Editor # {#v5_pcbnew}
This section covers the source code of the board editing application Pcbnew.

## Tool Framework ## {#v5_pcb_tool_framework}
**Goal:**

Unify all board editing tools under a single framework.

**Task:**
- Complete porting of all board editing tools to new tool framework so they
  are available in the OpenGL and Cairo canvases.
- Remove all duplicate legacy editing tools.

**Dependencies:**
- In progress.

**Status:**
- Initial porting work in progress.

## Linked Objects ## {#v5_pcb_linked_objects}
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

## Modeling ## {#v5_modeling}

**Goal:**
Provide improved solid modeling support for KiCad including the file formats
available in OpenCascade.

**Task:**
- Improve low level code design.
- Design plugin architecture to handle loading and saving 3D models.
- Back port existing 3D formats (IDF and S3D) to plugin
- Add STEP 3D modeling capability.
- Add IGES 3D modeling capability.

**Dependencies:**
- [Dynamic library plugin](#v5_plugin_base).

**Status:**
- 3D Viewer work in progress.  There is also now and external tool [KiCadStepUp]
  (http://sourceforge.net/projects/kicadstepup/) which allows [FreeCAD]
  (http://www.freecadweb.org/) to create parametric models from KiCad board
  files.

## Push and Shove Router Improvements ## {#v5_ps_router_improvements}

**Goal:**
Add features such as microwave tools to the P&S router.

**Task:**
- Determine which features are feasible.
- Look at the recently opened FreeRouter code at
  http://www.freerouting.net/fen/download/file.php?id=146 for inspiration.

**Dependencies:**
- None

**Status:**
- No Progress.

## Pin and Part Swapping ## {#v5_pcb_drc}
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
- [S-expression schematic file format](#v5_sch_sexpr).
- [Convert to a single process application](#v5_kiway).

**Status:**
- No progress.

## Intelligent Selection Tool ## {#v5_pcb_selection_tool}
**Goal:**

Make the selection tool easier for the user to determine which object(s) are
being selected.

**Task:**
- Determine and define the actual desired behavior.
- Improve ambiguous selections when multiple items are under the cursor or in
  the selection bounding box.

**Dependencies:**
- Tool framework.
- Unified geometry library.

**Status:**
- Initial design committed to product branch.

## Clipboard Support ## {#v5_fp_edit_clipboard}
**Goal:**

Provide clipboard cut and paste for footprint management in the footprint
editor.

**Task:**
- Clipboard cut and paste to and from clipboard of footprints in footprint
  editor.

**Dependencies:**
- None

**Status:**
- No progress.

## Design Rule Check (DRC) Improvements. ## {#v5_drc_improvements}
**Goal:**

Create additional DRC tests for improved error checking.

**Task:**
- Replace geometry code with [unified geometry library](#v5_geometry_lib).
- Remove floating point code from clearance calculations to prevent rounding
errors.
- Add checks for component, silk screen, and mask clearances.
- Add checks for vias in zones for proper connections without having to add
traces.
- Add checks for keep out zones.
- Remove DRC related limitations such as no arc or text on copper layers.
- Add option for saving and loading DRC options.

**Dependencies:**
- [Unified geometry library.](#v5_geometry_lib)

**Progress:**
- Planning

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

## Keepout Zones. ## {#v5_keepout_zones}
**Goal:**

Add support for keepout zones on boards and footprints.

**Task:**
- Add keepout support to zone classes.
- Add keepout zone support to board editor.
- Add keepout zone support to library editor.

**Dependencies:**
- [DRC Improvements.](#v5_drc_improvements)

**Progress:**
- Planning

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

## Complex Pad Shapes "" {#v5_pcb_complex_pads}
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


# GerbView: Gerber File Viewer # {#v5_gerbview}

This section covers the source code for the GerbView gerber file viewer.

## Graphics Abstraction Layer ## {#v5_gerbview_gal}
**Goal:**

Graphics rendering unification.

**Task:**
- Port graphics rendering layer to GAL.

**Dependencies:**
- None.

**Status**
- No progress.

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
- In progress.  Most of the developer documentation has been convert to
  [Doxygen markdown](http://www.stack.nl/~dimitri/doxygen/manual/markdown.html)
  and the [output][kicad-docs] is rebuilt automatically when a commit is
  made to the KiCad repo.

[kicad-website]:http://kicad-pcb.org/
[kicad-docs]:http://ci.kicad-pcb.org/job/kicad-doxygen/ws/Documentation/doxygen/html/index.html
