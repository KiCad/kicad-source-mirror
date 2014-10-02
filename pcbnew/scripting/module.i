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
 * @file module.i
 * @brief Specific BOARD extensions and templates
 */


%extend MODULE
{
  %pythoncode
  {

    #def SaveToLibrary(self,filename):
    #  return SaveModuleToLibrary(filename,self)

    #
    # add function, clears the thisown to avoid python from deleting
    # the object in the garbage collector
    #

    def Add(self,item):

        itemC = item.Cast()

        if type(itemC) is D_PAD:
            item.thisown=0
            self.Pads().PushBack(itemC)
        elif type(itemC) in [ TEXTE_PCB, DIMENSION, TEXTE_MODULE, DRAWSEGMENT,EDGE_MODULE]:
            item.thisown = 0
            self.GraphicalItems().PushBack(item)
  }

}

%pythoncode
{

    def GetPluginForPath(lpath):
        return IO_MGR.PluginFind(IO_MGR.LEGACY)

    def FootprintEnumerate(lpath):
        plug = GetPluginForPath(lpath)
        return plug.FootprintEnumerate(lpath)

    def FootprintLoad(lpath,name):
        plug = GetPluginForPath(lpath)
        return plug.FootprintLoad(lpath,name)

    def FootprintSave(lpath,module):
        plug = GetPluginForPath(lpath)
        return plug.FootprintSave(lpath,module)

    def FootprintDelete(lpath,name):
        plug = GetPluginForPath(lpath)
        plug.FootprintDelete(lpath,name)

    def FootprintLibCreate(lpath):
        plug = GetPluginForPath(lpath)
        plug.FootprintLibCreate(lpath)

    def FootprintLibDelete(lpath):
        plug = GetPluginForPath(lpath)
        plug.FootprintLibDelete(lpath)

    def FootprintIsWritable(lpath):
        plug = GetPluginForPath(lpath)
        plug.FootprintLibIsWritable(lpath)
}

%{
    MODULE *PyModule_to_MODULE(PyObject *obj0)
    {
        void *argp;
        int res1 = SWIG_ConvertPtr(obj0, &argp,SWIGTYPE_p_MODULE, 0 |  0 );
        if (!SWIG_IsOK(res1))
        {
            SWIG_exception_fail(SWIG_ArgError(res1), "Converting object to MODULE*");
        }

        return (MODULE*)argp;

        fail:
        return NULL;

    }

%}
