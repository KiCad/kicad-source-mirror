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
 * @file python_scripting.cpp
 * @brief methods to add scripting capabilities inside pcbnew
 */

#include <python_scripting.h>

/* init functions defined by swig */

extern "C" void init_kicad(void);
extern "C" void init_pcbnew(void);


/* python inittab that links module names to module init functions 
 * we will rebuild it to include the original python modules plus
 * our own ones 
 */

struct _inittab SwigImportInittab[1000];
static int  SwigNumModules = 0;


/* Add a name + initfuction to our SwigImportInittab */

static void swigAddModule(const char *name, void (*initfunc)()) 
{
        SwigImportInittab[SwigNumModules].name = (char *)name;
        SwigImportInittab[SwigNumModules].initfunc = initfunc;
        SwigNumModules++;
        SwigImportInittab[SwigNumModules].name = (char *) 0;
        SwigImportInittab[SwigNumModules].initfunc = 0;
}

/* Add the builting python modules */

static void swigAddBuiltin() 
{
        int i = 0;
        while (PyImport_Inittab[i].name) {
                swigAddModule(PyImport_Inittab[i].name, PyImport_Inittab[i].initfunc);
                i++;
        }

}
static void swigAddModules()
{
	swigAddModule("_pcbnew",init_pcbnew);
	
	// finally it seems better to include all in just one module
  // but in case we needed to include any other modules, 
  // it must be done like this:
	//    swigAddModule("_kicad",init_kicad);
	
}

static void swigSwitchPythonBuiltin()
{
  PyImport_Inittab = SwigImportInittab;
}



void pcbnewInitPythonScripting()
{
    swigAddBuiltin();
    swigAddModules();
    swigSwitchPythonBuiltin();
    
#if 0
    /* print the list of modules available from python */
    while(PyImport_Inittab[i].name)
    {
	printf("name[%d]=>%s\n",i,PyImport_Inittab[i].name);
	i++;
    }
#endif

    Py_Initialize();

    /* setup the scripting path, we may need to add the installation path 
       of kicad here */

    PyRun_SimpleString("import sys\n"
                       "sys.path.append(\".\")\n"
                       "import pcbnew\n");


}
  