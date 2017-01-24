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


# Eeschema: Schematic Editor # {#v6_eeschema}
This section applies to the source code for the Eeschema schematic editor.

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

## Implement GAL and New Tool Framework ## {#v6_sch_gal}
**Goal:**

Implement the GAL and the tool framework used by Pcbnew in Eechema to
provide advanced graphics and tool capabilities.

**Task:**
- Implement graphics abstraction layer along side current legacy rendering
  framework.

**Dependencies:**
- None

**Status:**
- Initial Discussion..

## Port Editing Tools ## {#v6_sch_tool_framework}
**Goal:**

Convert all editing tool to new tool framework.

-**Task:**
- Rewrite existing editing tools using the new tool framework.
- Add new capabilities supported by the new tool framework to existing
  editing tools.

-**Dependencies:**
- [GAL and new tool framework port](#v6_sch_gal).

-**Status:**
- Initial Discussion..

## Net Highlighting ## {#v6_sch_net_highlight}
**Goal:**
Highlight wires, buses, and junctions when corresponding net in Pcbnew is selected.

**Task:**
- Add communications link to handle net selection from Pcbnew.
- Implement highlight algorithm for net objects.
- Highlight objects connected to net selected in Pcbnew.

**Dependencies:**
- [GAL and new tool framework port, maybe](#v6_sch_gal).

**Status:**
- No progress.

## Component Library Editor Improvements ## {#lib_editor_usability}
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
- [S-expression schematic file format](#sch_sexpr).

**Status:**
- No progress.


# CvPcb: Footprint Association Tool # {#v6_cvpcb}
This section covers the source code of the footprint assignment tool CvPcb.

## Improved Footprint Search Tool ## {#v6_cvpcb_search}

**Goal:**

Provide advanced search features such as wild card and regular expression
searches using the type as you go feature of the current search dialog.

**Task:**
- Add code for wild card and regular expression pattern matching to search
  container objects.
- Add search dialog to CvPcb to search container of footprint names.

**Dependencies:**
- [Search Tree Control](#v6_re_search_control)

**Status:**
- Pattern matching added to search container objects.


# Pcbnew: Circuit Board Editor # {#v6_pcbnew}
This section covers the source code of the board editing application Pcbnew.

## Push and Shove Router Improvements ## {#v6_ps_router_improvements}

**Goal:**

Add finishing touches to push and shove router.

**Task:**
- Microwave tools to be added as parametrized shapes generated by Python
  scripts.

**Dependencies:**
- None

**Status:**
- None

## Design Rule Check (DRC) Improvements. ## {#v6_drc_improvements}
**Goal:**

Create additional DRC tests for improved error checking.

**Task:**
- Replace geometry code with [unified geometry library](#v6_geometry_lib).
- Remove floating point code from clearance calculations to prevent rounding
  errors.
- Add checks for component, silk screen, and mask clearances.
- Add checks for keep out zones.
- Remove DRC related limitations such as no arc or text on copper layers.
- Add option for saving and loading DRC options.

**Dependencies:**
- [Unified geometry library.](#v6_geometry_lib)

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
- None

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
- Planning

## Clipboard Support ## {#v6_fp_edit_clipboard}
**Goal:**

Provide clipboard cut and paste for footprints.

**Task:**
- Clipboard cut and paste to and from clipboard of footprints in footprint
  editor.

**Dependencies:**
- None

**Status:**
- No progress.

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
- No progress.

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
- No progress.

## Board Stack Up Impedance Calculator ## {#v6_pcb_impedance_calc}
**Goal:**

Provide a calculator to compute trace impedances using a full board stackup.
Maybe this should be included in the PCB calculator application.

**Task:**
- Design a trace impedance calculator that includes full board stackup.

**Dependencies:**
- None.

**Status:**
- No progress.

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

Add support for per net color and visibility settings.

**Task:**
- Implement UI code to configure ratsnest color and visibility.
- Update ratsnest code to handle per net color and visibility.

**Dependencies:**
- None.

**Status:**
- No progress.


# GerbView: Gerber File Viewer # {#v6_gerbview}

This section covers the source code for the GerbView gerber file viewer.

## Graphics Abstraction Layer ## {#v6_gerbview_gal}
**Goal:**

Graphics rendering unification.

**Task:**
- Port graphics rendering layer to GAL.

**Dependencies:**
- None.

**Status**
- No progress.
