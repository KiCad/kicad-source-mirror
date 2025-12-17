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
 * @file module.i
 * @brief Specific MODULE extensions and templates, and a few methods to access
 * footprints in library files
 */


%template(MAP_STRING_STRING) std::map<wxString, wxString>;
%feature("flatnested");
%include footprint.h
%feature("flatnested", "");

%rename(Get) operator   FOOTPRINT*;
%{
#include <footprint.h>
%}
%template(VECTOR_FP_3DMODEL) std::vector<FP_3DMODEL>;


// BOARD_ITEM_CONTAINER's interface functions will be implemented by SWIG
// automatically and inherited by the python wrapper class.


%extend FOOTPRINT
{
    %pythoncode
    %{

    #def SaveToLibrary(self,filename):
    #  return SaveFootprintToLibrary(filename,self)

    #
    # add function, clears the thisown to avoid python from deleting
    # the object in the garbage collector
    #

    def GetFieldsText(self):
      """ Returns footprint fields name to text map. """
      fields = self.GetFields()
      return {str(field.GetName()): str(field.GetText()) for field in fields}

    def GetFieldsShownText(self):
      """ Returns footprint fields name to shown text map. """
      fields = self.GetFields()
      return {str(field.GetName()): str(field.GetShownText(False)) for field in fields}

    def GetFieldText(self, key):
      """ Returns Field text with a given key if it exists, throws KeyError otherwise. """
      if self.HasField(key):
        return self.GetField(key).GetText()
      else:
        raise KeyError("Field not found: " + key)

    def GetFieldShownText(self, key):
      """ Returns Field shown text with a given key if it exists, throws KeyError otherwise. """
      if self.HasField(key):
        return self.GetField(key).GetShownText(False)
      else:
        raise KeyError("Field not found: " + key)

    def SetField(self, key, value):
      if self.HasField(key):
        self.GetField(key).SetText(value)
      else:
        field = PCB_FIELD(self, FIELD_T_USER, key)
        field.SetText(value)
        self.Add(field)

    def SetFields(self, fields):
      """ Sets footprint fields map. """
      for k, v in fields.items():
        self.SetField(k, v)
    %}

    // Compatibility shim
    const BOX2I GetBoundingBox( bool aIncludeText, bool includeHiddenText ) const
    {
        return ( $self )->GetBoundingBox( aIncludeText );
    }
}


%extend PCB_IO
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

        # Old function name for compatibility with pre-v8 scripts, use CreateLibrary() for new scripts.
        def FootprintLibCreate(self, aLibraryPath, aProperties=None):
            self.CreateLibrary(aLibraryPath, aProperties)

        # Old function name for compatibility with pre-v8 scripts, use DeleteLibrary() for new scripts.
        def FootprintLibDelete(self, aLibraryPath, aProperties=None):
            return self.DeleteLibrary(aLibraryPath, aProperties)

        # Old function name for compatibility with pre-v8 scripts, use IsLibraryWritable() for new scripts.
        def IsFootprintLibWritable(self, aLibraryPath):
            return self.IsLibraryWritable(aLibraryPath)
    %}
}

%pythoncode
%{
    def GetPluginForPath(libname):
        plugin_type = PCB_IO_MGR.GuessPluginTypeFromLibPath( libname );
        return PCB_IO_MGR.FindPlugin(plugin_type)

    def FootprintEnumerate(libname):
        plug = GetPluginForPath(libname)
        return plug.FootprintEnumerate(libname)

    def FootprintLoad(libname,name,preserveUUID=False):
        plug = GetPluginForPath(libname)
        return plug.FootprintLoad(libname,name,preserveUUID)

    def FootprintSave(libname,module):
        plug = GetPluginForPath(libname)
        return plug.FootprintSave(libname,module)

    def FootprintDelete(libname,name):
        plug = GetPluginForPath(libname)
        plug.FootprintDelete(libname,name)

    def FootprintLibCreate(libname):
        plug = GetPluginForPath(libname)
        plug.CreateLibrary(libname)

    def FootprintLibDelete(libname):
        plug = GetPluginForPath(libname)
        plug.DeleteLibrary(libname)

    def FootprintIsWritable(libname):
        plug = GetPluginForPath(libname)
        plug.FootprintLibIsWritable(libname)
%}


%{


// called from pcbnew/swig/pcbnew_footprint_wizards.cpp
FOOTPRINT* PyFootprint_to_FOOTPRINT( PyObject *obj0 )
{
    void* argp;
    int res1 = SWIG_ConvertPtr( obj0, &argp,SWIGTYPE_p_FOOTPRINT, 0 |  0 );

    if( !SWIG_IsOK( res1 ) )
    {
        SWIG_exception_fail( SWIG_ArgError( res1 ), "Converting object to FOOTPRINT*" );
    }

    return ( FOOTPRINT *) argp;

fail:
    return nullptr;
}

%}
