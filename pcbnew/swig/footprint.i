/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 NBEE Embedded Systems, Miguel Angel Ajo <miguelangel@nbee.es>
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file module.i
 * @brief Specific MODULE extensions and templates, and a few methods to access
 * footprints in library files
 */


%rename(MODULE_3D_SETTINGS_VECTOR3D) MODULE_3D_SETTINGS::VECTOR3D;
%feature("flatnested");
%include class_module.h
%feature("flatnested", "");

%rename(Get) operator   MODULE*;
%{
#include <class_module.h>
%}
%template(MODULE_3D_SETTINGS_List) std::list<MODULE_3D_SETTINGS>;


// BOARD_ITEM_CONTAINER's interface functions will be implemented by SWIG
// automatically and inherited by the python wrapper class.


%extend MODULE
{
    %pythoncode
    %{

    #def SaveToLibrary(self,filename):
    #  return SaveModuleToLibrary(filename,self)

    #
    # add function, clears the thisown to avoid python from deleting
    # the object in the garbage collector
    #
    %}
}

%extend PLUGIN
{
    // This version of FootprintEnumerate is for Python scripts, because the c++
    // version of FootprintEnumerate is not easy to handle in these Python scripts
    // if aExitOnError = true, footprintPyEnumerate throws a IO_ERROR.
    // if false, errors are silently ignored
    // in any case, only valid footprints are listed (especially for .pretty kicad libs
    // and GEDA fp libs, which are folder containing separate fp files)

    wxArrayString footprintPyEnumerate( const wxString& aLibraryPath, bool aExitOnError )
    {
        wxArrayString footprintNames;

        self->FootprintEnumerate( footprintNames, aLibraryPath, !aExitOnError );

        return footprintNames;
    }

    %pythoncode
    %{
        def FootprintEnumerate(self, libname):
            return self.footprintPyEnumerate( libname, True )
    %}
}

%pythoncode
%{
    def GetPluginForPath(libname):
        plugin_type = IO_MGR.GuessPluginTypeFromLibPath( libname );
        return IO_MGR.PluginFind(plugin_type)

    def FootprintEnumerate(libname):
        plug = GetPluginForPath(libname)
        return plug.FootprintEnumerate(libname)

    def FootprintLoad(libname,name):
        plug = GetPluginForPath(libname)
        return plug.FootprintLoad(libname,name)

    def FootprintSave(libname,module):
        plug = GetPluginForPath(libname)
        return plug.FootprintSave(libname,module)

    def FootprintDelete(libname,name):
        plug = GetPluginForPath(libname)
        plug.FootprintDelete(libname,name)

    def FootprintLibCreate(libname):
        plug = GetPluginForPath(libname)
        plug.FootprintLibCreate(libname)

    def FootprintLibDelete(libname):
        plug = GetPluginForPath(libname)
        plug.FootprintLibDelete(libname)

    def FootprintIsWritable(libname):
        plug = GetPluginForPath(libname)
        plug.FootprintLibIsWritable(libname)
%}


%{

// called from pcbnew/swig/pcbnew_footprint_wizards.cpp
MODULE* PyModule_to_MODULE(PyObject *obj0)
{
    void* argp;
    int res1 = SWIG_ConvertPtr(obj0, &argp,SWIGTYPE_p_MODULE, 0 |  0 );
    if (!SWIG_IsOK(res1))
    {
        SWIG_exception_fail(SWIG_ArgError(res1), "Converting object to MODULE*");
    }

    return (MODULE*) argp;

fail:
    return NULL;
}

%}
