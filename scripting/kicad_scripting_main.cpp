/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2021 Kicad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <kiface_i.h>
#include <kiface_ids.h>
#include <kiway.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>

#include <kipython_settings.h>
#include <python_scripting.h>

#include <sstream>


//-----<KIFACE>-----------------------------------------------------------------

namespace KIPYTHON {

static struct IFACE : public KIFACE_I
{
    bool OnKifaceStart( PGM_BASE* aProgram, int aCtlBits ) override;

    void OnKifaceEnd() override;

    wxWindow* CreateWindow( wxWindow* aParent, int aClassId, KIWAY* aKiway, int aCtlBits = 0 ) override
    {
        InitSettings( new KIPYTHON_SETTINGS );
        Pgm().GetSettingsManager().RegisterSettings( KifaceSettings() );


        // passing window ids instead of pointers is because wxPython is not
        // exposing the needed c++ apis to make that possible.
        std::stringstream pcbnew_pyshell_one_step;
        pcbnew_pyshell_one_step << "import kicad_pyshell\n";
        pcbnew_pyshell_one_step << "import wx\n";
        pcbnew_pyshell_one_step << "\n";

        // parent is actually *PCB_EDIT_FRAME
        if( aParent )
        {
            pcbnew_pyshell_one_step << "parent = wx.FindWindowById( " << aParent->GetId() << " )\n";
            pcbnew_pyshell_one_step << "newshell = kicad_pyshell.makePcbnewShellWindow( parent )\n";
        }
        else
        {
            pcbnew_pyshell_one_step << "newshell = kicad_pyshell.makePcbnewShellWindow()\n";
        }

        pcbnew_pyshell_one_step << "newshell.SetName( \"KiCad Shell\" )\n";
        // return value goes into a "global". It's not actually global, but rather
        // the dict that is passed to PyRun_String
        pcbnew_pyshell_one_step << "retval = newshell.GetId()\n";

        // As always, first grab the GIL
        PyLOCK      lock;

        // Now make a dictionary to serve as the global namespace when the code is
        // executed.  Put a reference to the builtins module in it.

        PyObject*   globals     = PyDict_New();
        PyObject*   builtins    = PyImport_ImportModule( "builtins" );

        wxASSERT( builtins );

        PyDict_SetItemString( globals, "__builtins__", builtins );
        Py_DECREF( builtins );

        // Execute the code to make the makeWindow function we defined above
        PyObject*   result = PyRun_String( pcbnew_pyshell_one_step.str().c_str(), Py_file_input,
                                           globals, globals );

        // Was there an exception?
        if( !result )
        {
            PyErr_Print();
            return NULL;
        }

        Py_DECREF( result );

        result = PyDict_GetItemString( globals, "retval" );

        if( !PyLong_Check( result ) )
        {
            wxLogError( "creation of scripting window didn't return a number" );
            return NULL;
        }

        const long windowId = PyLong_AsLong( result );

        // It's important not to decref globals before extracting the window id.
        // If you do it early, globals, and the retval int it contains, may/will be garbage collected.
        // We do not need to decref result, because GetItemString returns a borrowed reference.
        Py_DECREF( globals );

        wxWindow* window = wxWindow::FindWindowById( windowId );

        if( !window )
        {
            wxLogError( "unable to find pyshell window with id %d", windowId );
            return NULL;
        }

        return window;
    }

    /**
     * Function IfaceOrAddress
     * return a pointer to the requested object.  The safest way to use this
     * is to retrieve a pointer to a static instance of an interface, similar to
     * how the KIFACE interface is exported.  But if you know what you are doing
     * use it to retrieve anything you want.
     *
     * @param aDataId identifies which object you want the address of.
     *
     * @return void* - and must be cast into the know type.
     */
    void* IfaceOrAddress( int aDataId ) override
    {
        return NULL;
    }

    IFACE( const char* aDSOname, KIWAY::FACE_T aType ) :
        KIFACE_I( aDSOname, aType )
    {}

} kiface( "KIPYTHON", KIWAY::FACE_PYTHON );

}   // namespace KIPYTHON

using namespace KIPYTHON;

static PGM_BASE* process;

KIFACE_I& Kiface()
{
    return kiface;
}


// KIFACE_GETTER's actual spelling is a substitution macro found in kiway.h.
// KIFACE_GETTER will not have name mangling due to declaration in kiway.h.
KIFACE* KIFACE_GETTER( int* aKIFACEversion, int aKIWAYversion, PGM_BASE* aProgram )
{
    process = (PGM_BASE*) aProgram;
    return &kiface;
}


PGM_BASE& Pgm()
{
    wxASSERT( process );    // KIFACE_GETTER has already been called.
    return *process;
}

// Similar to PGM_BASE& Pgm(), but return nullptr when a *.ki_face is run from
// a python script or something else.
PGM_BASE* PgmOrNull()
{
    return process;
}


bool IFACE::OnKifaceStart( PGM_BASE* aProgram, int aCtlBits )
{

    ScriptingSetup();
    return start_common( aCtlBits );
}


void IFACE::OnKifaceEnd()
{
    end_common();
}
