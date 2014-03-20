#ifndef KIWAY_H_
#define KIWAY_H_
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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


/*

The KIWAY and KIFACE classes are used to communicate between various process
modules, all residing within a single process. The program modules are either
top level like an *.exe or subsidiary like a *.dll. In much of the documentation
the term DSO is used to refer to the *.dll portions, that is the term used on
linux. But it should be taken to mean DLL on Windows.

<p>These are a couple of reasons why this design was chosen:

<ol>

<li>By using DSOs within a single process, it is not necessary to use IPC.
The DSOs can send wxEvents between themselves using wxEvtHandler interfaces in
a platform independent way.  There can also be function calls from one DSO to
another.</li>

<li>The use of a number of separately linked DSOs closely resembles the original
KiCad program design, consisting of Eeschema and Pcbnew. But it also allows
separate compilation and linking of those two DSOs without a ton of inter-DSO
dependencies and common data structures. Linking smaller, purpose specific DSOs
is thought to be better for maintenance simplicity than a large single link
image. </li>

<li>By keeping the core functionality in DSOs rather than EXE tops, it becomes
possible to re-use the DSOs under different program tops. For example, a DSO
named _pcbnew.so can be used under a C++ top or under a python top. Only one
CMake target must be defined to build either. Whether that is a separate build
or not is not the important thing. Simply having a single CMake target has
advantages. (Each builder person will have his/her own intentions relative to
use of python or not.) Once a DSO is python capable, it can be driven by any
number of python program tops, including demo-ing (automaton) and testing
separately.</li>


</ol>

All KiCad source code is UTF8 encoded by law, so make sure your editor is set
as such!  As such, it is OK to use UTF8 characters:

┏ ┗ ┓ ┛ ━ ┃

<pre>

                             ┏━━━process top━━━━━┓
                             ┃                   ┃       wxEvent channels
         ┏━━━━━━━━━━━━━━━━━━━-━[KIWAY project 1]━-━━━━━━━━━━━━━━━━━━━━━━┓
         ┃                   ┃                   ┃                      ┃
         ┃     ┏━━━━━━━━━━━━━-━[KIWAY project 2]━-━━━━━━━━━━┓           ┃
         ┃     ┃             ┃                   ┃          ┃           ┃
         ┃     ┃           ┏━-━[KIWAY project 3]━-━┓        ┃           ┃
         ┃     ┃           ┃ ┗━━━━━━━━━━━━━━━━━━━┛ ┃        ┃           ┃
         ┃     ┃           ┃                       ┃        ┃           ┃
         ┃     ┃           ┃                       ┃        ┃           ┃
┏━━━━━━━━|━━━━━|━━━━━━━━━━━|━━━━━━━━━┓    ┏━━━━━━━━|━━━━━━━━|━━━━━━━━━━━|━━━━━┓
┃ KIFACE ┃     ┃           ┃         ┃    ┃ KIFACE ┃        ┃           ┃     ┃
┃        ┃     ┃           ┃         ┃    ┃        ┃        ┃           ┃     ┃
┃        ┃     ┃           ┃         ┃    ┃        ┃        ┃           ┃     ┃
┃┏━━━━━━━+━┓ ┏━+━━━━━━━┓ ┏━+━━━━━━━┓ ┃    ┃┏━━━━━━━+━┓ ┏━━━━+━━━━┓ ┏━━━━+━━━━┓┃
┃┃wxFrame  ┃ ┃wxFrame  ┃ ┃wxFrame  ┃ ┃    ┃┃wxFrame  ┃ ┃wxFrame  ┃ ┃wxFrame  ┃┃
┃┃project 1┃ ┃project 2┃ ┃project 3┃ ┃    ┃┃project 3┃ ┃project 2┃ ┃project 1┃┃
┃┗━━━━━━━━━┛ ┗━━━━━━━━━┛ ┗━━━━━━━━━┛ ┃    ┃┗━━━━━━━━━┛ ┗━━━━━━━━━┛ ┗━━━━━━━━━┛┃
┃                                    ┃    ┃                                   ┃
┃                                    ┃    ┃                                   ┃
┗━━━━━━ eeschema DSO ━━━━━━━━━━━━━━━━┛    ┗━━━━━━ pcbnew DSO ━━━━━━━━━━━━━━━━━┛

</pre>

*/


#include <wx/event.h>
#include <wx/dynlib.h>
#include <import_export.h>
#include <search_stack.h>
#include <project.h>


#define VTBL_ENTRY          virtual

#define KIFACE_VERSION      1
#define KIFACE_GETTER       KIFACE_1

// The KIFACE acquistion function is declared extern "C" so its name should not
// be mangled.
#define KIFACE_INSTANCE_NAME_AND_VERSION   "KIFACE_1"


#if defined(__linux__)
 #define LIB_ENV_VAR    wxT( "LD_LIBRARY_PATH" )

#elif defined(__WXMAC__)
 #define LIB_ENV_VAR    wxT( "DYLD_LIBRARY_PATH" )

#elif defined(__MINGW32__)
 #define LIB_ENV_VAR    wxT( "PATH" )
#endif


class wxConfigBase;


class KIWAY;
class wxWindow;
class PGM_BASE;
class wxConfigBase;


/**
 * Class KIFACE
 * is used by a participant in the KIWAY alchemy.  KIWAY is a minimalistic
 * software bus for communications between various DLLs/DSOs (DSOs) within the same
 * KiCad process.  It makes it possible to call between DSOs without having to link
 * them together.  Most all calls are via virtual functions which means C++ vtables
 * are used to hold function pointers and eliminate the need to link to specific
 * object code libraries.  There is one KIWAY in the launching portion of the process
 * for each open KiCad project.  Each project has its own KIWAY.  Within a KIWAY
 * is an actual PROJECT data structure.  A KIWAY also facilitates communicating
 * between DSOs on the topic of the project in question.
 */
struct KIFACE
{
    // The order of functions establishes the vtable sequence, do not change the
    // order of functions in this listing unless you recompile all clients of
    // this interface.

    /**
     * Function OnKifaceStart
     * is called just once shortly after the DSO is loaded.  It is the second
     * function called, immediately after the KIFACE_GETTER().  However before
     * either of those, static C++ constructors are called.  The DSO implementation
     * should do process level initialization here, not project specific since there
     * will be multiple projects open eventually.
     *
     * @param aProgram is the process block: PGM_BASE*
     *
     * @return bool - true if DSO initialized OK, false if not.  When returning
     *  false, the loader may optionally decide to terminate the process or not,
     *  but will not put out any UI because that is the duty of this function to say
     *  why it is returning false.  Never return false without having reported
     *  to the UI why.
     */
    VTBL_ENTRY bool OnKifaceStart( PGM_BASE* aProgram ) = 0;

    /**
     * Function OnKifaceEnd
     * is called just once just before the DSO is to be unloaded.  It is called
     * before static C++ destructors are called.  A default implementation is supplied.
     */
    VTBL_ENTRY void OnKifaceEnd() = 0;

#define KFCTL_STANDALONE    (1<<0)      ///< Am running as a standalone Top.

    /**
     * Function CreateWindow
     * creates a wxWindow for the current project.  The caller
     * must cast the return value into the known type.
     *
     * @param aParent may be NULL, or is otherwise the parent to connect under.  If NULL
     *  then caller may want to connect the returned wxWindow into some hierarchy after
     *  this function returns.
     *
     * @param aClassId identifies which wxFrame or wxDialog to retrieve, using a value
     *  known to the implementing KIFACE.
     *
     * @param aKIWAY tells the window which KIWAY (and PROJECT) it is a participant in.
     *
     * @param aCtlBits consists of bit flags from the set of KFCTL_* \#defines above.
     *
     * @return wxWindow* - and if not NULL, should be cast into the known type using
     *  and old school cast.  dynamic_cast is problemenatic since it needs typeinfo probably
     *  not contained in the caller's link image.
     */
    VTBL_ENTRY  wxWindow* CreateWindow( wxWindow* aParent, int aClassId,
            KIWAY* aKIWAY, int aCtlBits = 0 ) = 0;

    /**
     * Function IfaceOrAddress
     * returns a pointer to the requested object.  The safest way to use this
     * is to retrieve a pointer to a static instance of an interface, similar to
     * how the KIFACE interface is exported.  But if you know what you are doing
     * use it to retrieve anything you want.  Segfaults are your fault.
     *
     * @param aDataId identifies which object you want the address of, and consists
     *  of choices known in advance by the implementing KIFACE.
     *
     * @return void* - and must be cast into the known type.
     */
    VTBL_ENTRY void* IfaceOrAddress( int aDataId ) = 0;
};


/**
 * Class KIWAY
 * is a minimalistic software bus for communications between various
 * DLLs/DSOs (DSOs) within the same KiCad process.  It makes it possible
 * to call between DSOs without having to link them together, and without
 * having to link to the top process module which houses the KIWAY(s).  More importantly
 * it makes it possible to send custom wxEvents between DSOs and from the top
 * process module down into the DSOs.  The latter capability is thought useful
 * for driving the lower DSOs from a python test rig or for demo (automaton) purposes.
 * <p>
 * Most all calls are via virtual functions, which means C++ vtables
 * are used to hold function pointers and eliminate the need to link to specific
 * object code libraries, speeding development and encouraging clearly defined
 * interface design.  Unlike Microsoft COM, which is a multi-vendor design supporting
 * DLL's built at various points in time, the KIWAY alchemy is single project, with
 * all components being built at the same time.  So one should expect solid compatibility
 * between all KiCad components, as long at they are compiled at the same time.
 * <p>
 * There is one KIWAY in the launching portion of the process
 * for each open KiCad project.  Each project has its own KIWAY.  Available to
 * each KIWAY is an actual PROJECT data structure.  If you have a KIWAY, you
 * can get to the PROJECT using KIWAY::Prj().
 * <p>
 * In summary, a KIWAY facilitates communicating between DSOs, where the topic
 * of the communication is project specific.  Here a "project" means a BOARD
 * and a SCHEMATIC and a NETLIST, (anything relating to production of a single BOARD
 * and added to class PROJECT.)
 */
class KIWAY : public wxEvtHandler
{

public:
    /// DSO players on *this* KIWAY
    enum FACE_T
    {
        FACE_SCH,           ///< eeschema DSO
    //  FACE_LIB,
        FACE_PCB,           ///< pcbnew DSO
    //  FACE_MOD,
        FACE_CVPCB,
        FACE_BMP2CMP,
        FACE_GERBVIEW,
        FACE_PL_EDITOR,
        FACE_PCB_CALCULATOR,

        FACE_COUNT,         ///< how many KIWAY player types
    };

    /* from edaappl.h, now pgm_base.h, obsoleted by above FACE_T enum.
    enum PGM_BASE_T
    {
        APP_UNKNOWN,
        APP_EESCHEMA,
        APP_PCBNEW,
        APP_CVPCB,
        APP_GERBVIEW,
        APP_KICAD,
        APP_PL_EDITOR,
        APP_BM2CMP,
    };
    */

    // Don't change the order of these VTBL_ENTRYs, add new ones at the end,
    // unless you recompile all of KiCad.

    VTBL_ENTRY      KIFACE*     KiFACE( FACE_T aFaceId, bool doLoad );
    VTBL_ENTRY      PROJECT&    Prj()  const;

    KIWAY();

private:

    /*
    /// Get the name of the DSO holding the requested FACE_T.
    static const wxString dso_name( FACE_T aFaceId );
    */

    // one for each FACE_T
    static wxDynamicLibrary    s_sch_dso;
    static wxDynamicLibrary    s_pcb_dso;
    //static wxDynamicLibrary    s_cvpcb_dso;   // will get merged into pcbnew

    KIFACE*     m_kiface[FACE_COUNT];

    PROJECT     m_project;          // do not assume this is here, use Prj().
};


/**
 * Function Pointer KIFACE_GETTER_FUNC
 * points to the one and only KIFACE export.  The export's address
 * is looked up via symbolic string and should be extern "C" to avoid name
 * mangling. This function will only be called one time.  The DSO itself however
 * may be asked to support multiple Top windows, i.e. multiple projects
 * within its lifetime.
 *
 * @param aKIFACEversion is where to put the API version implemented by the KIFACE.
 * @param aKIWAYversion tells the KIFACE what KIWAY version will be available.
 * @param aProgram is a pointer to the PGM_BASE for this process.
 * @return KIFACE* - unconditionally, cannot fail.
 */
typedef     KIFACE*  KIFACE_GETTER_FUNC( int* aKIFACEversion, int aKIWAYversion, PGM_BASE* aProgram );

/// No name mangling.  Each KIFACE (DSO/DLL) will implement this once.
extern "C" KIFACE* KIFACE_GETTER(  int* aKIFACEversion, int aKIWAYversion, PGM_BASE* aProgram );

#endif  // KIWAY_H_
