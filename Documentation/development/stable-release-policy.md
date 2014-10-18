# Stable Release Policy #

This document defines the project requirements that must be satisfied in order to create a new
stable release of the KiCad project.  It is designed to be a reference for developers and user's
so that both groups expectations are understood.  This document is only to be modified by the
project leader or at the request of the project leader.  It should be noted that this policy is
not cast in stone and at any time in the future, should the decision be made by the project at
large that it can be revised to suit the ongoing needs of the project and it's users.

The current release policy is to support the concept of a lightweight stable release.  The goal
is to provide regular stable releases of KiCad without the burden of trying to provide long term
support of a full stable release branch.  Therefore, once a new release is created, the only
patches that will be made to the stable release branch will be for bugs that cause KiCad to crash
or possible corruption and/or loss of data.  No other changes from the current development branch
will be backported to the last stable release by the project.

[TOC]

# Stable Release Interval # {#stable_release_interval}

The criteria required for new stable releases is based on the developers decision that enough
new features and/or improvements have been made to the current development branch to justify a
new stable release.  This decision is completely discretionary and can be proposed at any time
by any developer on the KiCad developers mailing list.  Once a request for a new stable release
is made, a consensus must be reached by the primary developers to proceed with the release with
the final decision and announcement being made by the project leader.


# Feature Freeze # {#feature_freeze}

Once the announcement has been made that a new stable release is in effect, the current
development branch is frozen.  No new features or potentially disruptive core code changes can
be committed with out approval of the primary developers and/or the project leader.

# Bug Fixing # {#bug_fixing}

After the development branch has been frozen, work will continue to fix bugs reported against
the development branch.  Bugs will be prioritized based on their severity.  All bugs that cause
KiCad to crash or cause loss and/or corruption of data must be fixed.  All other bugs must be
evaluated to see if they fit into the scope of the stable release.  All bugs that fit into the
scope of the stable release will be tagged and must be fixed.  All other bugs will be tagged for
the next stable release and fixed when it is convenient.  Once the stable release is officially
announced, the bugs tagged as "Fix Committed" that are relevant to the stable release will be
changed to "Fix Released".

# User Documentation # {#user_docs}

The user documentation will be updated to reflect the current changes in the code.  This includes
all new features, any behavioral changes to existing features, and all screen shots as required.
Completion of the English version of the user documentation is minimum that is required for
release.  Foreign language translations can be released at any time as the become available.

# Stable Release Series Branch # {#stable_branch}

Once the primary developers decide that the stable release criteria has been met, a new series
branch will be created from the current product branch on Launchpad.  At this time the freeze
will be removed from the product branch and normal development can resume.  The stable release
version will be incremented from the previous stable release and tagged in the stable release
branch build configuration.

# System Installers # {#system_installers}

To proved the best user experience for platforms that do not have package managers, full system
installers will be provided.  Currently this only pertains to Windows and OSX.  The full system
installers will include all KiCad binary files, all binary library dependencies, user
documentation, component libraries, 3D model libraries, demo project files, and project template
files.  Optionally, the footprint libraries can be included for users who prefer not us use the
GitHub plugin.

# Source Archives # {#source_archives}

To provide a convenient method for system packagers to build KiCad from known stable sources,
source archives in the most common formats along with the resulting md5sum checksum will be
added to either the KiCad developer's site on Launchpad or the main website at www.kicad-pcb.org.

# Stable Release Announcement # {#announcement}

Once all of the above tasks have been completed, the project leader will post an announcement on
the developers mailing list and the Launchpad site.  This announcement should include a list of
new features and improvements made since the previous stable release.
