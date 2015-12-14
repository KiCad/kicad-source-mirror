KiCad README
============
For specific documentation like Compiling, GUI translation, Old changelogs see the
Documentation subfolder.

Files
-----
AUTHORS.txt         - The authors, contributors, document writers and translators list
CHANGELOG.txt       - This years changelog (see for previous years Documentation/changelogs)
CMakeList.txt       - CMAKE build tool script
COPYRIGHT.txt       - A copy of the GNU General Public License Version 2
CTestConfig.cmake   - Support for CTest and CDash testing tools
Doxyfile            - Doxygen config file for Kicad
INSTALL.txt         - The release (binary) installation instructions
TODO.txt            - Todo list
uncrustify.cfg      - Uncrustify config file for uncrustify sources formatting tool

Subdirectories
--------------
3d-viewer      - Sourcecode of 3D viewer
bitmaps        - Menu and program icons
bitmap2component - Sourcecode of a small application to create a footprint or a component from a B&W bitmap
                    this component or footprint has just graphic items that show the bitmap
CMakeModules   - Modules for the CMAKE build tool
common         - Sourcecode of the common library (common functions shared across whole suite)
cvpcb          - Sourcecode of CvPCB, tool to link components with footprints sourcecode
demos          - Some demo examples
Documentation  - Compiling documentation. Translating the GUI, old changelogs etcetera.
eeschema       - Sourcecode of the schematic editor
gerbview       - Sourcecode of the gerber viewer
helpers        - Helper tools and utilities for development
include        - Interfaces to the common library
internat       - Internationalization files
kicad          - Sourcecode of the project manager
packaging      - Files for packaging on Windows and Mac OSX
pcbnew         - Sourcecode of the printed circuit board editor
polygon        - Sourcecode of the polygon library
resources      - Resources for installation, freedesktop mime-types for linux
scripts        - Helper scripts. For building, sourcecode packaging.
template       - Project template(s)

