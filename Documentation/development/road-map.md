# Road Map #

This document is the KiCad Developer's road map document.  It is a living
document that should be maintained as the project progresses.  The goal of
this document is to provide an overview for developers of where the project
is headed beyond the current development cycle road map (currently
[version 6](./v6_road_map.html) to prevent resource conflicts and endless
rehashing of previously discussed topics.  It is broken into sections for
each major component of the KiCad source code and documentation.  It defines
tasks that developers an use to contribute to the project and provides updated
status information.  Tasks should define clear objectives and avoid vague
generalizations so that a new developer can complete the task.  It is not a
place for developers to add their own personal wish list  It should only be
updated with approval of the project manager after discussion with the lead
developers.

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


# Build Tools # {#build_tools}
This section covers build tools for both the KiCad source as well as the
custom dependency builds required to build KiCad.


# Common Library # {#common_lib}
This section covers the source code shared between all of the KiCad
applications


# KiCad: Application Launcher # {#kicad}
This section applies to the source code for the KiCad application launcher.


# Eeschema: Schematic Editor # {#eeschema}
This section applies to the source code for the Eeschema schematic editor.


# CvPcb: Footprint Association Tool # {#cvpcb}
This section covers the source code of the footprint assignment tool CvPcb.


# Pcbnew: Circuit Board Editor # {#pcbnew}
This section covers the source code of the board editing application Pcbnew.


# GerbView: Gerber File Viewer # {#gerbview}

This section covers the source code for the GerbView gerber file viewer.


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


[kicad-website]:http://kicad.org/
[kicad-docs]:http://ci.kicad.org/job/kicad-doxygen/ws/Documentation/doxygen/html/index.html
