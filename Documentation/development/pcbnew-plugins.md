# Python Plugin Development for Pcbnew #

[TOC]

# Introduction # {#ppi_intro}
KiCad implements a Python plugin interface so that external Python plugins can
be run from within Pcbnew.  The interface is generated using the `Simplified
Wrapper and Interface Generator` or [SWIG](http://www.swig.org).  SWIG is
instructed to translate specific C/C++ header files into other languages using
`interface` files.  These files ultimately decide what C/C++ functions, classes
and other declarations are exported and can be found in `pcbnew/swig/`.

During build-time the SWIG interface files are used to generate the
corresponding .py files.  These files are installed into Python's system-wide
`dist-packages` repository, thus they can be imported by any Python 2
interpreter installed on the system.

# Existing Pcbnew Python API documentation # {#ppi_api_docs}
The Pcbnew Python API can be used stand-alone, i.e. no instance of Pcbnew is
running and the board project to be manipulated is loaded and saved from and to
file.  This approach is shown with some examples in the [user's
documentation](http://docs.kicad-pcb.org/stable/en/pcbnew.html#_kicad_scripting_reference).

Another documentation source is the auto-generated Doxygen reference of the
API. It can be found
[here](http://docs.kicad-pcb.org/doxygen-python/namespacepcbnew.html).

# `Action Plugin` Support # {#ppi_action_pi}
Besides the stand-alone usage of the generated Python plugin interface,
additional support regarding online manipulation of board projects is available
for Pcbnew.  Plugins using this feature are called `Action Plugins` and they are
accessible using a Pcbnew menu entry that can be found under `Tools->External
Plugins`.  KiCad plugins that follow the `Action Plugin` conventions can be made
to show up as external plugins in that menu.
The user can run the plugin resulting in calling a defined entry function in the
Python plugin's code.
This function can then be used to access and manipulate the currently loaded
board from the Python script environment.

## Typical Plugin Structure ## {#ppi_pi_struct}
The `Action Plugin` support is implemented in Pcbnew by discovering Python
packages and Python script files in specific directories on startup.
In order for the discovery process to work, the following requirements must be met.

* The plugin must be installed in the KiCad plugins search paths as documented
  in `scripting/kicadplugins.i`.
  (Currently on a Linux Mint Installation this is
  /usr/share/kicad/scripting/plugins/ and ~/.kicad_plugins/)
* Alternatively a symbolic link can be created in the KiCad plugin path link to
  the plugin file/folder in another location of the file system. This can be
  useful for development.
* The plugin must be written as a simple Python script (*.py) located in the
  plugin search path.  Note that this method is preferred for small plugins
  consisting of a single .py file.
* Alternatively the plugin must be implemented as a Python package conforming to
  the Python package standard definitions (See
  [6.4. Packages](https://docs.python.org/2/tutorial/modules.html#packages)).
  Note that this method is preferred for larger plugin projects consisting of
  multiple .py files and resource files such as dialogs or images.
* The Python plugin must contain a class derived from `pcbnew.ActionPlugin` and
  it's `register()` method must be called within the plugin.

The following examples demonstrate the plugin requirements.

## Simple Plugin Example ## {#ppi_simple_example}
The folder structure of the simple plugin is fairly straight forward.
A single Python script file is placed into a directory that is present in the
KiCad plugin path.

    + ~/.kicad_plugins/ # A folder in the KiCad plugin path
        - simple_plugin.py

The file `simple_plugin.py` contains the following.

    import pcbnew

    class SimplePlugin(pcbnew.ActionPlugin):
        def defaults(self):
            self.name = "Plugin Name as shown in Pcbnew: Tools->External Plugins"
            self.category = "A descriptive category name"
            self.description = "A description of the plugin and what it does"

        def Run(self):
            # The entry function of the plugin that is executed on user action
            print("Hello World")

    SimplePlugin().register() # Instantiate and register to Pcbnew

## Complex Plugin Example ## {#ppi_complex_example}
The complex plugin example represents a single Python package that is imported
on Pcbnew startup.  When the Python package is imported, the `__init__.py` file
is executed and is thus a perfect place to instantiate and register the plugin
to Pcbnew.
The big advantage here is, that you can modularize your plugin much better and
include other files without cluttering the KiCad plugin directory.
Additionally, the same plugin can be executed standalone using `python -m`
e.g. to perform tests on the Python code.
The following folder structure shows how complex plugins are implemented:

    + ~/.kicad_plugins/ # this directory has to be in the plugin path
        + complex_plugin/ # The plugin directory (A Python package)
            - __init__.py # This file is executed when the package is imported (on Pcbnew startup)
            - __main__.py # This file is optional. See below
            - complex_plugin_action.py # The ActionPlugin derived class lives here
            - complex_plugin_utils.py # Other Python parts of the plugin
            + otherstuff/
                - otherfile.png
                - misc.txt

It is recommended to name the file containing the ActionPlugin derived class as
`<package-name>_action.py`.
In this case the file is named `complex_plugin_action.py` with the following
contents:

    import pcbnew

    class ComplexPluginAction(pcbnew.ActionPlugin)
        def defaults(self):
            self.name = "A complex action plugin"
            self.category = "A descriptive category name"
            self.description "A description of the plugin"

        def Run(self):
            # The entry function of the plugin that is executed on user action
            print("Hello World")

The `__init__.py` file is then used to instantiate and register the plugin to
Pcbnew as follows.

    from .complex_plugin_action import ComplexPluginAction # Note the relative import!
    ComplexPluginAction().register() # Instantiate and register to Pcbnew

As described in [PEP 338](https://www.python.org/dev/peps/pep-0338/) Python can
execute packages (or modules) as scripts.  This can be useful to implement a
command-line stand-alone version of your KiCad plugin with minimum effort.
In order to implement this feature, a `__main__.py` file is created in the
package directory.
This file can be executed by running the following command.

    python -m <package_name>

Make sure that your current directory is the parent directory of the package
directory when running the command.
In these examples, this would be `~/.kicad_plugins`.
When running the command the Python interpreter runs
`/complex_plugin/__init__.py` followed by `/complex_plugin/__main__.py`.


