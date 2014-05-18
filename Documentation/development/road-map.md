# Road Map # {#mainpage}

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

## Stable Release ## {#stable_release}
**Goal:**

Provide a lightweight stable release mechanism that is robust enough to meet
the requirements of Linux packagers and corporate users but avoid the burden
of back porting fixes to a maintenance branch to avoid the additional work for
developers.

**Task:**
- Devise a process to have some type of reasonably stable release protocol
  to provide "stable" releases for Linux distribution packagers and corporate
  users.
- Document "stable" release procedure.

**Dependencies:**
- None

**Status:**
- Initial planning stages.


# General # {#general}
This section defines the tasks that affect all or most of KiCad or do not
fit under as specific part of the code such as the board editor or the
schematic editor.

## Convert to a Single Process Application. ## {#kiway}
**Goal:**

Merge common schematic and board code into to separate dynamic objects to allow
Eeschema and Pcbnew to run under a single process.

**Task:**
- Convert the majority core code in Eeschema and Pcbnew into dynamic libraries.
- Provide a robust method for communicating between code running under a single
  process.
- Revise the schematic editor and board editor main windows run under a single
  process instead of multiple stand alone applications.
- Design a method for passing information between the dynamic libraries running
  under the same process.
- Remove inter-process communications between Eeschema and Pcbnew.

**Dependencies:**
- None

**Status:**
- Stage 1 code released.
- Stage 2 in process.

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

## Create Separate Build Dependency Project ## {#depends_prj}
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
- Initial concept discussions.

## Platform Binary Installers ## {#installers}
**Goal:**

Provide quality installers for all supported platforms.

**Task:**
- Bring OSX installer up to the level of the Window's and Linux installers.
- Possible use of CPack to build platform specific installers as long as they
  are of the same or better quality than the current independent installers.

**Dependencies**
- None

**Status**
- No progress


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

## Unified Geometry Library ## {#geometry_lib}
**Goal:**

Select a single geometry library so that all applications share a common
base for 2D objects.  Remove any redundant geometry libraries and code to
clean up code base.

**Task:**
- Select the best geometry library (Boost, etc.) for the task.
- Port all legacy geometry code to the selected library.
- Remove any unused geometry library code.

**Dependencies:**
- None

**Status:**
- In progress as part of push and shove router.

## Conversion to wxWidgets 3 ## {#wxwidgets3}
**Goal:**

Stop supporting the version 2 branch of wxWidgets so that newer features
provided by version 3 can be utilized.

**Task:**
- Make wxWidgets 3 a build requirement.
- Remove all wxWidgets 2 specific code.

**Dependencies:**
- wxWidgets 3 is widely available on Linux distributions.

**Status:**
- No progress

## Linux Printing Improvements ## {#linux_print}
**Goal:**

Bring printing on Linux up to par with printing on Windows.

**Task:**
- Resolve Linux printing issues.

**Dependencies**
- [wxWidgets 3](#wxwidgets3)

**Status**
- No progress.

## Object Properties and Introspection ## {#object_props}
**Goal:**

Provide an object introspection system using properties.

**Task:**
- Select existing or develop property system.
- Add definable properties to base objects.
- Create introspection framework for manipulating object properties.
- Serialization of properties to and from files and/or other I/O structures.
- Create tool to edit property name/type/value table.

**Dependencies:**
- None

**Status:**
- No progress.

## Dynamic Library Plugin ## {#plugin_base}
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


# KiCad: Application Launcher # {#kicad}
This section applies to the source code for the KiCad application launcher.


# Eeschema: Schematic Editor # {#eeschema}
This section applies to the source code for the Eeschema schematic editor.

## Coherent SCHEMATIC Object ## {#sch_object}
**Goal:**

Clean up the code related to the schematic object(s) into a coherent object for
managing and manipulating the schematic.

**Task:**
- Move most if not all of the code from SCH_SCREEN to the new SCHEMATIC object.
- Add any missing functionality to the SCHEMATIC object.

**Dependencies:**
- None

**Status:**
- No progress.

## Hierarchical Sheet Design ## {#hierarchy_fix}
**Goal:**

Create a more robust sheet instance design rather than recreating them on the
fly every time sheet information is required.

**Task:**
- Choose a data structure to contain the sheet hierarchy.
- Create helper class to manipulate the hierarchy data structure.

**Dependencies:**
- None

**Status:**
- No progress.

## Schematic and Component Library Plugin ## {#sch_plugin}
**Goal:**
Create a plugin manager for loading and saving schematics and component
libraries similar to the board plugin manager.

**Task:**
- Design plugin manager for schematics and component libraries.
- Port the current schematic and component library file formats to use the
  plugin.

**Dependencies:**
- [Dynamic library plugin](#plugin_base)

**Status:**
- No progress.

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

## S-Expression File Format ## {#sch_sexpr}
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
- [Dynamic library plugin](#plugin_base).

**Status:**
- File format document nearly complete.

## Implement Sweet Component Libraries ## {#sch_sweet}
**Goal:**

Make component library design more robust and feature rich.  Use s-expressions
to make component library files more readable.

**Task:**
- Use sweet component file format for component libraries.

**Dependencies:**
- [S-expression file format](#sch_sexpr).

**Status:**
- Initial SWEET library written.

## Component Library Editor Improvements ## {#lib_editor_usability}
**Goal:**

Make editing components with multiple units and/or alternate graphical
representations easier.

**Task:**
- Determine usability improvements in the library editor for components with
  multiple units and/or alternate graphical representations.
- Implement said useability improvements.

**Dependencies:**
- None.

**Status:**
- No progress.

## Component and Netlist Attributes ## {#netlist_attributes}
**Goal:**
Provide a method of passing information to other tools via the net list.

**Task:**
- Add virtual components and attributes to netlist to define properties that
  can be used by other tools besides the board editor.

**Dependencies:**
- [S-expression schematic file format](#sch_sexpr).

**Status:**
- No progress.


# CvPcb: Footprint Association Tool # {#cvpcb}
This section covers the source code of the footprint assignment tool CvPcb.

## Footprint Assignment Tool ##
**Goal:**

Merge the footprint assignment functionality of CvPcb into Eeschema so
footprints can be assigned inside the schematic editor eliminating the need
to launch an separate program.

**Task:**
- Merge footprint assignment capability into Pcbnew shared library.
- Remove CvPcb as a stand alone tool.
- Add functionality to both the schematic and board editors so users can assign
  footprints as they prefer.

**Dependencies:**
- [Convert to a single process application](#kiway).

**Status:**
- Initial library conversion committed to product branch.


# Pcbnew: Circuit Board Editor # {#pcbnew}
This section covers the source code of the board editing application Pcbnew.

## Tool Framework ## {#pcb_tool_framework}
**Goal:**

Unify all board editing tools under a single framework.

**Task:**
- Complete porting of all board editing tools to new tool framework so they
  are available in the OpenGL and Cairo canvases.
- Remove all duplicate legacy editing tools.

**Dependencies:**
- None

**Status:**
- Initial porting work in progress.

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

## Modeling ## {#modeling}
**Goal:**

Provide improved solid modeling support for KiCad including the file formats
available in OpenCascade.

**Task:**
- Design plugin architecture to handle loading and saving 3D models.
- Back port existing 3D formats (IDF and S3D) to plugin
- Add STEP 3D modeling capability.
- Add IGES 3D modeling capability.

**Dependencies:**
- [Dynamic library plugin](#plugin_base).

**Status:**
- No progress.

## Push and Shove Router Improvements ## {#ps_router_improvements}
**Goal:**
Add features such as matched length and microwave tools to the P&S router.

**Task:**
- Determine which features are feasible.
- Look at the recently opened FreeRouter code at
  http://www.freerouting.net/fen/download/file.php?id=146 for inspiration.

**Dependencies:**
- None

**Status:**
- No progress.

## Layer Improvements ## {#pcb_layers}
**Goal:**

Increase the number of usable technical and user defined layers in Pcbnew.

**Task:**
- Extend the number of copper and mechanical layers.
- Develop a type safe flag set template or adapt something already available.
- Refactor Pcbnew code to use new flag and remove the 32 layer limitation.
- Extend the board file format to handle the additional layers.

**Dependencies:**
- None

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

## Intelligent Selection Tool ## {#pcb_selection_tool}
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

## Conversion to Markup/down Format ## {#doc_format}
**Goal:**

Make documentation more VCS friendly and separate document content and
formatting for more uniform formatting across all user documentation.

**Task:**
- Convert the documentation to a mark up/down language to reduce the VCS
  footprint, to be able to actually use the VCS to see what changed, and
  improve the formatting consistency.

**Dependencies:**
- None

**Status:**
- Started with this document.

## Grammar Check ## {#doc_grammar}
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

## Maintenance ## {#doc_maintenance}
**Task:**
- Keep screen shots current with the source changes.

**Dependencies:**
- None.

**Status:**
- No progress.

## Convert Developer Documentation to Markup/down Format ## {#dev_doc_format}
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


# Circuit Simulation # {#simulation}
**Goal:**

Provide quality circuit simulation capabilities similar to commercial products.

**Task:**
- Evaluate and select simulation library (spice, gnucap, qucs, etc).
- Evaluate and select plotting library with wxWidgets support.
- Confirm current spice netlist export is up to the task and add missing
  support for simulations.
- Use plotting library to handle simulator output in a consistent manor similar
  to LTSpice.
- Develop a tool that allows fine tuning of components on the fly.
- Use plugin for the simulation code to allow support of different simulation
  libraries.
- Create a library of simulation components such as voltage source, current
  source, current probe, etc.

**Dependencies:**
- [Dynamic library plugin](#plugin_base).

**Status:**
- No progress.
