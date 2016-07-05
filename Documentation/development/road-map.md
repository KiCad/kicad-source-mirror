# Road Map #

This document is the KiCad Developer's road map document.  It is a living
document that should be maintained as the project progresses.  The goal of
this document is to provide an overview for developers of where the project
is headed to prevent resource conflicts and endless rehashing of previously
discussed topics.  It is broken into sections for each major component of
the KiCad source code and documentation.  It defines tasks that developers
an use to contribute to the project and provides updated status information.
Tasks should define clear objective and avoid vague generalizations so that
a new developer can complete the task.  It is not a place for developers to
add their own personal wish list  It should only be updated with approval
of the project manager after discussion with the lead developers.

Each entry in the road map is made up of four sections.  The goal should
be a brief description of the what the road map entry will accomplish.  The
task section should be a list of deliverable items that are specific enough
hat they can be documented as completed.  The dependencies sections is a list
of requirements that must be completed before work can begin on any of the
tasks.  The status section should include a list of completed tasks or marked
as complete as when the goal is met.

[TOC]

# Project # {#project}
This section defines the tasks for the project related goals that are not
related to coding or documentation.  It is a catch all for issues such as
developer and user relations, dissemination of information on websites,
policies, etc.


# General # {#general}
This section defines the tasks that affect all or most of KiCad or do not
fit under as specific part of the code such as the board editor or the
schematic editor.

## User Interface Modernization ## {#wxaui}
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


# Build Tools # {#build_tools}
This section covers build tools for both the KiCad source as well as the
custom dependency builds required to build KiCad.

# Common Library # {#common_lib}
This section covers the source code shared between all of the KiCad
applications

## Unified Rendering Framework ## {#unified_rendering}
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
- No progress

# KiCad: Application Launcher # {#kicad}
This section applies to the source code for the KiCad application launcher.


# Eeschema: Schematic Editor # {#eeschema}
This section applies to the source code for the Eeschema schematic editor.

## Graphics Abstraction Layer Conversion ## {#sch_gal}
**Goal:**

Take advantage of advanced graphics rendering in Eeschema.

**Task:**
- Port graphics rendering to GAL.

**Dependencies:**
- None

**Status:**
- No progress.

## Port Editing Tools ## {#sch_tool_framework}
**Goal:**

Use standard tool framework across all applications.

**Task:**
- Rewrite editing tools using the new tool framework.

**Dependencies:**
- [GAL port](#sch_gal).

**Status:**
- No progress.

## Allow Use of System Fonts ## {#sch_sys_fonts}
**Goal:**

Currently the schematic editor uses the stroke drawn fonts which aren't really
necessary for accurated printing of schematics.  Allow the use of system fonts
for schematic text.

**Task:**
- Determine which library for font handling makes the most sense, wxWidgets or
  freetype.
- Add support for selecting text object fonts.

**Dependencies:**
- [S-expression schematic file format](#sch_sexpr).

**Status:**
- No progress.

# CvPcb: Footprint Association Tool # {#cvpcb}
This section covers the source code of the footprint assignment tool CvPcb.

# Pcbnew: Circuit Board Editor # {#pcbnew}
This section covers the source code of the board editing application Pcbnew.

## Model Export ## {#model_export}

**Goal:**

Provide improved solid modeling export to the file formats available in
OpenCascade.

**Task:**
- Add STEP 3D modeling capability.
- Add IGES 3D modeling capability.
- Add any other file formats supported by OpenCascade that make sense for
  KiCad.

**Dependencies:**
- None

**Status:**
- None

## Linked Objects ## {#pcb_linked_objects}
**Goal:**

Provide a way to allow external objects such as footprints to be externally
linked in the board file so that changes in the footprint are automatically
updated.  This will all a one to many object relationship which can pave the
way for real board modules.

**Task:**
- Add externally and internally linked objects to the file format to allow for
  footprints and/or other board objects to be shared (one to many relationship)
  instead of only supporting embedded objects (one to one relationship) that
  can only be edited in place.

**Dependencies:**
- None.

**Status:**
- No progress.

## Pin and Part Swapping ## {#pcb_drc}
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
- [S-expression schematic file format](#sch_sexpr).
- [Convert to a single process application](#kiway).

**Status:**
- No progress.

## Clipboard Support ## {#fp_edit_clipboard}
**Goal:**

Provide clipboard cut and paste for footprints..

**Task:**
- Clipboard cut and paste to and from clipboard of footprints in footprint
  editor.

**Dependencies:**
- None

**Status:**
- No progress.

## Segment End Point Snapping. ## {#segment_snapping}
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
- Allow the user to smap the unconnected segment end to the nearest segment end
  point.
- Automatically connect unconnected segments with and additional segment when
  opening the 3D viewer or exporting the board to another format.  Warn the
  user that an addition segment has be added and should be verified.

**Dependencies:**
- None

**Progress:**
- Initial discussion.

## Keepout Zones. ## {#keepout_zones}
**Goal:**

Add support for keepout zones on boards and footprints.

**Task:**
- Add keepout support to zone classes.
- Add keepout zone support to board editor.
- Add keepout zone support to library editor.

**Dependencies:**
- [DRC Improvements.](#drc_improvements)

**Progress:**
- Planning


## Net Highlighting ## {#pcb_net_highlight}
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

## Hatched Zone Filling ## {#pcb_hatched_zones}
**Goal:**

Currently Pcbnew only supports solid zone files.  Add option to fill zones
with hatching.

**Task:**
- Determine zone fill method, required filling code, and file format requirements.
- Add hatch option and hatch configuration to zone dialog.

**Dependencies:**
- None.

**Status:**
- No progress.


## Board Stack Up Impedance Calculator ## {#pcb_impedance_calc}
**Goal:**

Provide a calculator to compute trace impedances using a full board stackup.
Maybe this should be included in the PCB calculator application.

**Task:**
- Design a trace impedance calculator that includes full board stackup.

**Dependencies:**
- None.

**Status:**
- No progress.

## Net Class Improvements ## {#pcb_net_class_improvements}
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

## Ratsnest Improvements ## {#pcb_ratsnest_improvements}
**Goal:**

Add support for per net color and visibility settings.

**Task:**
- Implement UI code to configure ratsnest color and visibility.
- Update ratsnest code to handle per net color and visibility.

**Dependencies:**
- None.

**Status:**
- No progress.


# GerbView: Gerber File Viewer # {#gerbview}

This section covers the source code for the GerbView gerber file viewer.

## Graphics Abstraction Layer ## {#gerbview_gal}
**Goal:**

Graphics rendering unification.

**Task:**
- Port graphics rendering layer to GAL.

**Dependencies:**
- None.

**Status**
- No progress.

# Documentation # {#documentation}
This section defines the tasks for both the user and developer documentation.

## Maintenance ## {#doc_maintenance}
**Task:**
- Keep screen shots current with the source changes.

**Dependencies:**
- None.

**Status:**
- No progress.

# Unit Testing # {#unittest}
**Goal:**

Improve the quality of KiCad and ensure changes do no break existing
capabilities.

**Task:**
- Explore the possibility of including a C++ unit test framework in addition
  to the existing Python framework.
- Create robust enough test coverage to determine if code changes break any
  core functionality.

**Dependencies:**
- Completion of the initial release of this document.

**Status:**
- In progress.


[kicad-website]:http://kicad-pcb.org/
[kicad-docs]:http://ci.kicad-pcb.org/job/kicad-doxygen/ws/Documentation/doxygen/html/index.html
