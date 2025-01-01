/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 NBEE Embedded Systems, Miguel Angel Ajo <miguelangel@nbee.es>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

 /**
  * This file builds the base classes for all kind of python plugins that
  * can be included into kicad.
  * they provide generic code to all the classes:
  *
  * KiCadPlugin
  *  /|\
  *   |
  *   |\-FilePlugin
  *   |\-FootprintWizardPlugin
  *   |\-ActionPlugin
  *
  * It defines the LoadPlugins() function that loads all the plugins
  * available in the system
  *
  */

/*
 * Remark:
 * Avoid using the print function in python wizards
 *
 * Be aware print messages create IO exceptions, because the wizard
 * is run from Pcbnew. And if pcbnew is not run from a console, there is
 * no io channel to read the output of print function.
 * When the io buffer is full, a IO exception is thrown.
 */

%pythoncode
{

KICAD_PLUGINS={}    # the list of loaded footprint wizards

""" the list of not loaded python scripts
    (usually because there is a syntax error in python script)
    this is the python script full filenames list.
    filenames are separated by '\n'
"""
NOT_LOADED_WIZARDS=""

""" the list of paths used to search python scripts.
    Stored here to be displayed on request in Pcbnew
    paths are separated by '\n'
"""
PLUGIN_DIRECTORIES_SEARCH=""

"""
    the trace of errors during execution of footprint wizards scripts
    Warning: strings (internally unicode) are returned as UTF-8 compatible C strings
"""

FULL_BACK_TRACE=""

def GetUnLoadableWizards():
    global NOT_LOADED_WIZARDS
    import sys

    if sys.version_info[0] < 3:
        utf8_str = NOT_LOADED_WIZARDS.encode( 'UTF-8' )
    else:
        utf8_str = NOT_LOADED_WIZARDS

    return utf8_str

def GetWizardsSearchPaths():
    global PLUGIN_DIRECTORIES_SEARCH
    import sys

    if sys.version_info[0] < 3:
        utf8_str = PLUGIN_DIRECTORIES_SEARCH.encode( 'UTF-8' )
    else:
        utf8_str = PLUGIN_DIRECTORIES_SEARCH

    return utf8_str

def GetWizardsBackTrace():
    global FULL_BACK_TRACE  # Already correct format
    return FULL_BACK_TRACE


def LoadPluginModule(Dirname, ModuleName, FileName):
    """
    Load the plugin module named ModuleName located in the folder Dirname.
    The module can be either inside a file called FileName or a subdirectory
    called FileName that contains a __init__.py file.
    If this module cannot be loaded, its name is stored in failed_wizards_list
    and the error trace is stored in FULL_BACK_TRACE
    """
    import os
    import sys
    import traceback

    global NOT_LOADED_WIZARDS
    global FULL_BACK_TRACE
    global KICAD_PLUGINS

    try:  # If there is an error loading the script, skip it

        module_filename = os.path.join( Dirname, FileName )
        mtime = os.path.getmtime( module_filename )
        mods_before = set( sys.modules )

        if ModuleName in KICAD_PLUGINS:
            plugin = KICAD_PLUGINS[ModuleName]

            for dependency in plugin["dependencies"]:
                if dependency in sys.modules:
                    del sys.modules[dependency]

            mods_before = set( sys.modules )

        if sys.version_info >= (3,0,0):
            import importlib
            mod = importlib.import_module( ModuleName )
        else:
            mod = __import__( ModuleName, locals(), globals() )

        mods_after = set( sys.modules ).difference( mods_before )
        dependencies = [m for m in mods_after if m.startswith(ModuleName)]

        KICAD_PLUGINS[ModuleName]={ "filename":module_filename,
                                    "modification_time":mtime,
                                    "ModuleName":mod,
                                    "dependencies": dependencies }

    except:
        if ModuleName in KICAD_PLUGINS:
            del KICAD_PLUGINS[ModuleName]

        if NOT_LOADED_WIZARDS != "" :
            NOT_LOADED_WIZARDS += "\n"
        NOT_LOADED_WIZARDS += module_filename
        FULL_BACK_TRACE += traceback.format_exc()


def LoadPlugins(bundlepath=None, userpath=None, thirdpartypath=None):
    """
    Initialise Scripting/Plugin python environment and load plugins.

    Arguments:
    Note: bundlepath and userpath are given  utf8 encoded, to be compatible with asimple C string
    bundlepath -- The path to the bundled scripts.
                  The bundled Plugins are relative to this path, in the
                  "plugins" subdirectory.
                WARNING: bundlepath must use '/' as path separator, and not '\'
                because it creates issues:
                \n and \r are seen as a escaped seq when passing this string to this method
                I am thinking this is due to the fact LoadPlugins is called from C++ code by
                PyRun_SimpleString()

    NOTE: These are all of the possible "default" search paths for kicad
          python scripts.  These paths will ONLY be added to the python
          search path ONLY IF they already exist.

        The Scripts bundled with the KiCad installation:
            <bundlepath>/
            <bundlepath>/plugins/

        The Scripts relative to the KiCad Users configuration:
            <userpath>/
            <userpath>/plugins/

        The plugins from 3rd party packages:
            $KICAD_3RD_PARTY/plugins/
    """
    import os
    import sys
    import traceback
    import pcbnew

    if sys.version_info >= (3,3,0):
        import importlib
        importlib.invalidate_caches()

    """
    bundlepath and userpath are strings utf-8 encoded (compatible "C" strings).
    So convert these utf8 encoding to unicode strings to avoid any encoding issue.
    """
    try:
        bundlepath = bundlepath.decode( 'UTF-8' )
        userpath = userpath.decode( 'UTF-8' )
        thirdpartypath = thirdpartypath.decode( 'UTF-8' )
    except AttributeError:
        pass

    config_path = pcbnew.SETTINGS_MANAGER.GetUserSettingsPath()
    plugin_directories=[]

    """
    To be consistent with others paths, on windows, convert the unix '/' separator
    to the windows separator, although using '/' works
    """
    if sys.platform.startswith('win32'):
        if bundlepath:
            bundlepath = bundlepath.replace("/","\\")
        if thirdpartypath:
            thirdpartypath = thirdpartypath.replace("/","\\")

    if bundlepath:
        plugin_directories.append(bundlepath)
        plugin_directories.append(os.path.join(bundlepath, 'plugins'))

    if config_path:
        plugin_directories.append(os.path.join(config_path, 'scripting'))
        plugin_directories.append(os.path.join(config_path, 'scripting', 'plugins'))

    if userpath:
        plugin_directories.append(userpath)
        plugin_directories.append(os.path.join(userpath, 'plugins'))

    if thirdpartypath:
        plugin_directories.append(thirdpartypath)

    global PLUGIN_DIRECTORIES_SEARCH
    PLUGIN_DIRECTORIES_SEARCH=""
    for plugins_dir in plugin_directories:    # save search path list for later use
        if PLUGIN_DIRECTORIES_SEARCH != "" :
            PLUGIN_DIRECTORIES_SEARCH += "\n"
        PLUGIN_DIRECTORIES_SEARCH += plugins_dir

    global FULL_BACK_TRACE
    FULL_BACK_TRACE=""          # clear any existing trace

    global NOT_LOADED_WIZARDS
    NOT_LOADED_WIZARDS = ""     # save not loaded wizards names list for later use

    global KICAD_PLUGINS

    for plugins_dir in plugin_directories:
        if not os.path.isdir( plugins_dir ):
            continue

        if plugins_dir not in sys.path:
            sys.path.append( plugins_dir )

        for module in os.listdir(plugins_dir):
            fullPath = os.path.join( plugins_dir, module )

            if os.path.isdir( fullPath ):
                if os.path.exists( os.path.join( fullPath, '__init__.py' ) ):
                    LoadPluginModule( plugins_dir, module, module )
                else:
                    if NOT_LOADED_WIZARDS != "" :
                        NOT_LOADED_WIZARDS += "\n"
                    NOT_LOADED_WIZARDS += 'Skip subdir ' + fullPath
                continue

            if module == '__init__.py' or module[-3:] != '.py':
                continue

            LoadPluginModule( plugins_dir, module[:-3], module )


class KiCadPlugin:
    def __init__(self):
        pass

    def register(self):
        import inspect
        import os

        if isinstance(self,FilePlugin):
            pass # register to file plugins in C++

        if isinstance(self,FootprintWizardPlugin):
            PYTHON_FOOTPRINT_WIZARD_LIST.register_wizard(self)
            return

        if isinstance(self,ActionPlugin):
            """
            Get path to .py or .pyc that has definition of plugin class.
            If path is binary but source also exists, assume definition is in source.
            """
            self.__plugin_path = inspect.getfile(self.__class__)
            if self.__plugin_path.endswith('.pyc') and os.path.isfile(self.__plugin_path[:-1]):
                self.__plugin_path = self.__plugin_path[:-1]
            self.__plugin_path = self.__plugin_path + '/' + self.__class__.__name__
            PYTHON_ACTION_PLUGINS.register_action(self)
            return

        return

    def deregister(self):
        if isinstance(self,FilePlugin):
            pass # deregister to file plugins in C++

        if isinstance(self,FootprintWizardPlugin):
            PYTHON_FOOTPRINT_WIZARD_LIST.deregister_wizard(self)
            return

        if isinstance(self,ActionPlugin):
            PYTHON_ACTION_PLUGINS.deregister_action(self)
            return

        return

    def GetPluginPath( self ):
        return self.__plugin_path


class FilePlugin(KiCadPlugin):
    def __init__(self):
        KiCadPlugin.__init__(self)


from math import ceil, floor, sqrt

uMM = "mm"              # Millimetres
uMils = "mils"          # Mils
uFloat = "float"        # Natural number units (dimensionless)
uInteger = "integer"    # Integer (no decimals, numeric, dimensionless)
uBool = "bool"          # Boolean value
uRadians = "radians"    # Angular units (radians)
uDegrees = "degrees"    # Angular units (degrees)
uPercent = "%"          # Percent (0% -> 100%)
uString = "string"      # Raw string

uNumeric = [uMM, uMils, uFloat, uInteger, uDegrees, uRadians, uPercent]                  # List of numeric types
uUnits   = [uMM, uMils, uFloat, uInteger, uBool, uDegrees, uRadians, uPercent, uString]  # List of allowable types

class FootprintWizardParameter(object):
    _true  = ['true','t','y','yes','on','1',1,]
    _false = ['false','f','n','no','off','0',0,'',None]

    _bools = _true + _false

    def __init__(self, page, name, units, default, **kwarg):
        self.page = page
        self.name = name
        self.hint = kwarg.get('hint','')               # Parameter hint (shown as mouse-over text)
        self.designator = kwarg.get('designator',' ')  # Parameter designator such as "e, D, p" (etc)

        if units.lower() in uUnits:
            self.units = units.lower()
        elif units.lower() == 'percent':
            self.units = uPercent
        elif type(units) in [list, tuple]:  # Convert a list of options into a single string
            self.units = ",".join([str(el).strip() for el in units])
        else:
            self.units = units

        self.multiple = int(kwarg.get('multiple',1))   # Check integer values are multiples of this number
        self.min_value = kwarg.get('min_value',None)   # Check numeric values are above or equal to this number
        self.max_value = kwarg.get('max_value',None)   # Check numeric values are below or equal to this number

        self.SetValue(default)
        self.default = self.raw_value  # Save value as default

    def ClearErrors(self):
        self.error_list = []

    def AddError(self, err, info=None):

        if err in self.error_list:  # prevent duplicate error messages
            return
        if info is not None:
            err = err + " (" + str(info) + ")"

        self.error_list.append(err)

    def Check(self, min_value=None, max_value=None, multiple=None, info=None):

        if min_value is None:
            min_value = self.min_value
        if max_value is None:
            max_value = self.max_value
        if multiple is None:
            multiple = self.multiple

        if self.units not in uUnits and ',' not in self.units:  # Allow either valid units or a list of strings
            self.AddError("type '{t}' unknown".format(t=self.units),info)
            self.AddError("Allowable types: " + str(self.units),info)

        if self.units in uNumeric:
            try:
                to_num = float(self.raw_value)

                if min_value is not None:  # Check minimum value if it is present
                    if to_num < min_value:
                        self.AddError("value '{v}' is below minimum ({m})".format(v=self.raw_value,m=min_value),info)

                if max_value is not None:  # Check maximum value if it is present
                    if to_num > max_value:
                        self.AddError("value '{v}' is above maximum ({m})".format(v=self.raw_value,m=max_value),info)

            except:
                self.AddError("value '{v}' is not of type '{t}'".format(v = self.raw_value, t=self.units),info)

        if self.units == uInteger:  # Perform integer specific checks
            try:
                to_int = int(self.raw_value)

                if multiple is not None and multiple > 1:
                    if (to_int % multiple) > 0:
                        self.AddError("value '{v}' is not a multiple of {m}".format(v=self.raw_value,m=multiple),info)
            except:
                self.AddError("value '{v}' is not an integer".format(v=self.raw_value),info)

        if self.units == uBool:  # Check that the value is of a correct boolean format
            if self.raw_value in [True,False] or str(self.raw_value).lower() in self._bools:
                pass
            else:
                self.AddError("value '{v}' is not a boolean value".format(v = self.raw_value),info)

    @property
    def value(self):  # Return the current value, converted to appropriate units (from string representation) if required
        v = str(self.raw_value)  # Enforce string type for known starting point

        if self.units == uInteger:  # Integer values
            return int(v)
        elif self.units in uNumeric:  # Any values that use floating points
            v = v.replace(",",".")  # Replace "," separators with "."
            v = float(v)

            if self.units == uMM: # Convert from millimetres to nanometres
                return FromMM(v)

            elif self.units == uMils:  # Convert from mils to nanometres
                return FromMils(v)

            else:  # Any other floating-point values
                return v

        elif self.units == uBool:
            if v.lower() in self._true:
                return True
            else:
                return False
        else:
            return v

    def DefaultValue(self):  # Reset the value of the parameter to its default
        self.raw_value = str(self.default)

    def SetValue(self, new_value):  # Update the value
        new_value = str(new_value)

        if len(new_value.strip()) == 0:
            if not self.units in [uString, uBool]:
                return  # Ignore empty values unless for strings or bools

        if self.units == uBool:  # Enforce the same boolean representation as is used in KiCad
            new_value = "1" if new_value.lower() in self._true else "0"
        elif self.units in uNumeric:
            new_value = new_value.replace(",", ".")  # Enforce decimal point separators
        elif ',' in self.units:  # Select from a list of values
            if new_value not in self.units.split(','):
                new_value = self.units.split(',')[0]

        self.raw_value = new_value

    def __str__(self):  # pretty-print the parameter

        s = self.name + ": " + str(self.raw_value)

        if self.units in [uMM, uMils, uPercent, uRadians, uDegrees]:
            s += self.units
        elif self.units == uBool:  # Special case for Boolean values
            s = self.name + ": {b}".format(b = "True" if self.value else "False")
        elif self.units == uString:
            s = self.name + ": '" + self.raw_value + "'"

        return s


class FootprintWizardPlugin(KiCadPlugin, object):
    def __init__(self):
        KiCadPlugin.__init__(self)
        self.defaults()

    def defaults(self):
        self.module = None
        self.params = []  # List of added parameters that observes addition order

        self.name = "KiCad FP Wizard"
        self.description = "Undefined Footprint Wizard plugin"
        self.image = ""
        self.buildmessages = ""

    def AddParam(self, page, name, unit, default, **kwarg):

        if self.GetParam(page,name) is not None:  # Param already exists!
            return

        param = FootprintWizardParameter(page, name, unit, default, **kwarg)  # Create a new parameter
        self.params.append(param)

    @property
    def parameters(self):  # This is a helper function that returns a nested (unordered) dict of the VALUES of parameters
        pages = {}  # Page dict
        for p in self.params:
            if p.page not in pages:
                pages[p.page] = {}

            pages[p.page][p.name] = p.value  # Return the 'converted' value (convert from string to actual useful units)

        return pages

    @property
    def values(self):  # Same as above
        return self.parameters

    def ResetWizard(self):  # Reset all parameters to default values
        for p in self.params:
            p.DefaultValue()

    def GetName(self):  # Return the name of this wizard
        return self.name

    def GetImage(self):  # Return the filename of the preview image associated with this wizard
        return self.image

    def GetDescription(self):  # Return the description text
        return self.description

    def GetValue(self):
        raise NotImplementedError

    def GetReferencePrefix(self):
        return "REF"  # Default reference prefix for any footprint

    def GetParam(self, page, name):  # Grab a parameter
        for p in self.params:
            if p.page == page and p.name == name:
                return p

        return None

    def CheckParam(self, page, name, **kwarg):
        self.GetParam(page,name).Check(**kwarg)

    def AnyErrors(self):
        return any([len(p.error_list) > 0 for p in self.params])

    @property
    def pages(self):  # Return an (ordered) list of the available page names
        page_list = []
        for p in self.params:
            if p.page not in page_list:
                page_list.append(p.page)

        return page_list

    def GetNumParameterPages(self):  # Return the number of parameter pages
        return len(self.pages)

    def GetParameterPageName(self,page_n):  # Return the name of a page at a given index
        return self.pages[page_n]

    def GetParametersByPageName(self, page_name):  # Return a list of parameters on a given page
        params = []

        for p in self.params:
            if p.page == page_name:
                params.append(p)

        return params

    def GetParametersByPageIndex(self, page_index):  # Return an ordered list of parameters on a given page
        return self.GetParametersByPageName(self.GetParameterPageName(page_index))

    def GetParameterDesignators(self, page_index):  # Return a list of designators associated with a given page
        params = self.GetParametersByPageIndex(page_index)
        return [p.designator for p in params]

    def GetParameterNames(self,page_index):  # Return the list of names associated with a given page
        params = self.GetParametersByPageIndex(page_index)
        return [p.name for p in params]

    def GetParameterValues(self,page_index):  # Return the list of values associated with a given page
        params = self.GetParametersByPageIndex(page_index)
        return [str(p.raw_value) for p in params]

    def GetParameterErrors(self,page_index):  # Return list of errors associated with a given page
        params = self.GetParametersByPageIndex(page_index)
        return [str("\n".join(p.error_list)) for p in params]

    def GetParameterTypes(self, page_index):  # Return list of units associated with a given page
        params = self.GetParametersByPageIndex(page_index)
        return [str(p.units) for p in params]

    def GetParameterHints(self, page_index):  # Return a list of units associated with a given page
        params = self.GetParametersByPageIndex(page_index)
        return [str(p.hint) for p in params]

    def GetParameterDesignators(self, page_index):  # Return a list of designators associated with a given page
        params = self.GetParametersByPageIndex(page_index)
        return [str(p.designator) for p in params]

    def SetParameterValues(self, page_index, list_of_values):  # Update values on a given page

        params = self.GetParametersByPageIndex(page_index)

        for i, param in enumerate(params):
            if i >= len(list_of_values):
                break
            param.SetValue(list_of_values[i])

    def GetFootprint( self ):
        self.BuildFootprint()
        return self.module

    def BuildFootprint(self):
        return

    def GetBuildMessages( self ):
        return self.buildmessages

    def Show(self):
        text  = "Footprint Wizard Name:        {name}\n".format(name=self.GetName())
        text += "Footprint Wizard Description: {desc}\n".format(desc=self.GetDescription())

        n_pages = self.GetNumParameterPages()

        text += "Pages: {n}\n".format(n=n_pages)

        for i in range(n_pages):
            name = self.GetParameterPageName(i)

            params = self.GetParametersByPageName(name)

            text += "{name}\n".format(name=name)

            for j in range(len(params)):
                text += ("\t{param}{err}\n".format(
                    param = str(params[j]),
                    err = ' *' if len(params[j].error_list) > 0 else ''
                    ))

        if self.AnyErrors():
            text += " * Errors exist for these parameters"

        return text

class ActionPlugin(KiCadPlugin, object):
    def __init__( self ):
        KiCadPlugin.__init__( self )
        self.icon_file_name = ""
        self.dark_icon_file_name = ""
        self.show_toolbar_button = False
        self.defaults()

    def defaults( self ):
        self.name = "Undefined Action plugin"
        self.category = "Undefined"
        self.description = ""

    def GetClassName(self):
        return type(self).__name__

    def GetName( self ):
        return self.name

    def GetCategoryName( self ):
        return self.category

    def GetDescription( self ):
        return self.description

    def GetShowToolbarButton( self ):
        return self.show_toolbar_button

    def GetIconFileName( self, dark ):
        if dark and self.dark_icon_file_name:
            return self.dark_icon_file_name
        else:
            return self.icon_file_name

    def Run(self):
        return

}
