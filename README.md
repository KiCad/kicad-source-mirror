# KiCad README

For specific documentation like [Compiling](Documentation/development/compiling.md), GUI translation, old
changelogs see the [Documentation](Documentation) subfolder.

You may also take a look into the [Wiki](https://gitlab.com/kicad/code/kicad/-/wikis/home),
the [Developers](https://kicad-pcb.org/contribute/developers/) section on our [Website](https://kicad-pcb.org/) and
our [Forum](https://forum.kicad.info/).

## Build state

[<img alt="Jenkins" src="https://img.shields.io/jenkins/build?jobUrl=https%3A%2F%2Fjenkins.simonrichter.eu%2Fview%2FKiCad%2520Status%2Fjob%2Flinux-kicad-head%2F&label=Linux%2C%20amd64&style=plastic">](https://jenkins.simonrichter.eu/view/KiCad%20Status/job/linux-kicad-head/)
[<img alt="Jenkins" src="https://img.shields.io/jenkins/build?jobUrl=https%3A%2F%2Fjenkins.simonrichter.eu%2Fview%2FKiCad%2520Status%2Fjob%2Fwindows-kicad-msvc-head%2F&label=Windows%2C%20MSVC&style=plastic">](https://jenkins.simonrichter.eu/view/KiCad%20Status/job/windows-kicad-msvc-head/)
[<img alt="Jenkins" src="https://img.shields.io/jenkins/build?jobUrl=https%3A%2F%2Fjenkins.simonrichter.eu%2Fview%2FKiCad%2520Status%2Fjob%2Fwindows-kicad-msys2-pipeline%2F&label=%28Windows%2C%20MSYS2%29%20Nightly%20Build&style=plastic">](https://jenkins.simonrichter.eu/view/KiCad%20Status/job/windows-kicad-msys2-pipeline/)

## Release status
[![latest released version(s)](https://repology.org/badge/latest-versions/kicad.svg)](https://repology.org/project/kicad/versions)
[![Release status](https://repology.org/badge/tiny-repos/kicad.svg)](https://repology.org/metapackage/kicad/versions)

## Files
* [AUTHORS.txt](AUTHORS.txt) - The authors, contributors, document writers and translators list
* [CMakeLists.txt](CMakeLists.txt) - Main CMAKE build tool script
* [copyright.h](copyright.h) - A very short copy of the GNU General Public License to be included in new source files
* [CTestConfig.cmake](CTestConfig.cmake) - Support for CTest and CDash testing tools
* [Doxyfile](Doxyfile) - Doxygen config file for KiCad
* [INSTALL.txt](INSTALL.txt) - The release (binary) installation instructions
* [uncrustify.cfg](uncrustify.cfg) - Uncrustify config file for uncrustify sources formatting tool
* [_clang-format](_clang-format) - clang config file for clang-format sources formatting tool

## Subdirectories

* [3d-viewer](3d-viewer)         - Sourcecode of the 3D viewer
* [bitmap2component](bitmap2component)  - Sourcecode of the bitmap to pcb artwork converter
* [bitmaps_png](bitmaps_png)       - Menu and program icons
* [CMakeModules](CMakeModules)      - Modules for the CMAKE build tool
* [common](common)            - Sourcecode of the common library
* [cvpcb](cvpcb)             - Sourcecode of the CvPCB tool
* [demos](demos)             - Some demo examples
* [Documentation](Documentation)     - Developer documentation. Old changelogs etcetera.
* [eeschema](eeschema)          - Sourcecode of the schematic editor
* [gerbview](gerbview)          - Sourcecode of the gerber viewer
* [helpers](helpers)           - Helper tools and utilities for development
* [include](include)           - Interfaces to the common library
* [kicad](kicad)             - Sourcecode of the project manager
* [libs](libs)           - Sourcecode of kicad utilities (geometry and others)
* [pagelayout_editor](pagelayout_editor) - Sourcecode of the pagelayout editor
* [patches](patches)           - Collection of patches for external dependencies
* [pcbnew](pcbnew)           - Sourcecode of the printed circuit board editor
* [plugins](plugins)           - Sourcecode for the 3d viewer plugins
* [qa](qa)                - Unit testing framework for KiCad
* [resources](resources)         - Resources for freedesktop mime-types for linux
* [scripting](scripting)         - SWIG Python scripting definitions and build scripts
* [scripts](scripts)           - Example scripts for distribution with KiCad
* [template](template)          - Project template
* [thirdparty](thirdparty)           - Sourcecode of external libraries used in kicad but not written by Kicad team
* [tools](tools)             - Other miscellaneous helpers for testing
* [utils](utils)             - Small utils for kicad, e.g. IDF, STEP and OGL tools and converters

