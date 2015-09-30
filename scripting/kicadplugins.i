/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 NBEE Embedded Systems, Miguel Angel Ajo <miguelangel@nbee.es>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

KICAD_PLUGINS={}

def ReloadPlugin(name):
    if not KICAD_PLUGINS.has_key(name):
        return False

    KICAD_PLUGINS[name]["object"].deregister()
    mod = reload(KICAD_PLUGINS[name]["module"])
    KICAD_PLUGINS[name]["object"]= mod.register()


def ReloadPlugins():
    import os.path
    for k in KICAD_PLUGINS.keys():
        plugin = KICAD_PLUGINS[k]

        filename = plugin["filename"]
        mtime = plugin["modification_time"]
        now_mtime = os.path.getmtime(filename)

        if mtime!=now_mtime:
            # /* print filename, " is modified, reloading" */
            KICAD_PLUGINS[k]["modification_time"]=now_mtime
            ReloadPlugin(k)


def LoadPlugins( plugpath ):
    import os
    import sys

    kicad_path = os.environ.get('KICAD_PATH')
    plugin_directories=[]

    if plugpath and os.path.isdir( plugpath ):
        plugin_directories.append( plugpath )

    if kicad_path and os.path.isdir(kicad_path):
        plugin_directories.append(os.path.join(kicad_path, 'scripting', 'plugins'))

    if sys.platform.startswith('linux'):
        plugin_directories.append(os.environ['HOME']+'/.kicad_plugins/')
        plugin_directories.append(os.environ['HOME']+'/.kicad/scripting/plugins/')

    if sys.platform.startswith('darwin'):
        for singlepath in sys.path:
            if os.path.isdir( os.path.join( singlepath, 'scripting', 'plugins') ):
                plugin_directories.append( os.path.join( singlepath, 'scripting', 'plugins') )

    for plugins_dir in plugin_directories:
        if not os.path.isdir(plugins_dir):
            continue

        sys.path.append(plugins_dir)

        for module in os.listdir(plugins_dir):
            if os.path.isdir(plugins_dir+module):
                __import__(module, locals(), globals())

            if module == '__init__.py' or module[-3:] != '.py':
                continue

            mod = __import__(module[:-3], locals(), globals())

            module_filename = plugins_dir+"/"+module
            mtime = os.path.getmtime(module_filename)
            if hasattr(mod,'register'):
                KICAD_PLUGINS[module]={"filename":module_filename,
                                       "modification_time":mtime,
                                       "object":mod.register(),
                                       "module":mod}



class KiCadPlugin:
    def __init__(self):
        pass

    def register(self):
        if isinstance(self,FilePlugin):
            pass # register to file plugins in C++
        if isinstance(self,FootprintWizardPlugin):
            PYTHON_FOOTPRINT_WIZARDS.register_wizard(self)
            return

        if isinstance(self,ActionPlugin):
            pass # register to action plugins in C++

        return

    def deregister(self):
        if isinstance(self,FilePlugin):
            pass # register to file plugins in C++
        if isinstance(self,FootprintWizardPlugin):
            PYTHON_FOOTPRINT_WIZARDS.deregister_wizard(self)
            return

        if isinstance(self,ActionPlugin):
            pass # register to action plugins in C++

        return




class FilePlugin(KiCadPlugin):
    def __init__(self):
        KiCadPlugin.__init__(self)



class FootprintWizardPlugin(KiCadPlugin):
    def __init__(self):
        KiCadPlugin.__init__(self)
        self.defaults()

    def defaults(self):
        self.module = None
        self.parameters = {}
        self.parameter_errors={}
        self.name = "Undefined Footprint Wizard plugin"
        self.description = ""
        self.image = ""
        self.buildmessages = ""

    def GetName(self):
        return self.name

    def GetImage(self):
        return self.image

    def GetDescription(self):
        return self.description


    def GetNumParameterPages(self):
        return len(self.parameters)

    def GetParameterPageName(self,page_n):
        return self.parameters.keys()[page_n]

    def GetParameterNames(self,page_n):
        name = self.GetParameterPageName(page_n)
        return self.parameters[name].keys()

    def GetParameterValues(self,page_n):
        name = self.GetParameterPageName(page_n)
        values = self.parameters[name].values()
        return map( lambda x: str(x) , values) # list elements as strings

    def GetParameterErrors(self,page_n):
        self.CheckParameters()
        name = self.GetParameterPageName(page_n)
        values = self.parameter_errors[name].values()
        return map( lambda x: str(x) , values) # list elements as strings

    def CheckParameters(self):
        return ""

    def TryConvertToFloat(self,value):
        v = value
        try:
            v = float(value)
        except:
            pass

        return v

    def SetParameterValues(self,page_n,values):
        name = self.GetParameterPageName(page_n)
        keys = self.parameters[name].keys()
        n=0
        for key in keys:
            val = self.TryConvertToFloat(values[n])
            self.parameters[name][key] = val
            n+=1


    def ClearErrors(self):
        errs={}

        for page in self.parameters.keys():
            page_dict = self.parameters[page]
            page_params = {}
            for param in page_dict.keys():
                page_params[param]=""

            errs[page]=page_params

        self.parameter_errors = errs


    def GetFootprint( self ):
        self.BuildFootprint()
        return self.module

    def BuildFootprint(self):
        return

    def GetBuildMessages( self ):
        return self.buildmessages

    def Show(self):
        print "Footprint Wizard Name:        ",self.GetName()
        print "Footprint Wizard Description: ",self.GetDescription()
        n_pages = self.GetNumParameterPages()
        print " setup pages: ",n_pages
        for page in range(0,n_pages):
            name = self.GetParameterPageName(page)
            values = self.GetParameterValues(page)
            names =  self.GetParameterNames(page)
            print "page %d) %s"%(page,name)
            for n in range (0,len(values)):
                print "\t%s\t:\t%s"%(names[n],values[n])

class ActionPlugin(KiCadPlugin):
    def __init__(self):
        KiCadPlugin.__init__(self)

}
