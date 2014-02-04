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


#define VTBL_ENTRY          virtual

#define KIFACE_VERSION                      1
#define KIFACE_GETTER                       KIFACE_1

// The KIFACE acquistion function is declared extern "C" so its name should not
// be mangled (much).  Windows has leading underscore for our C function.
// Keep the trailing version number in sync with the KIFACE_GETTER define above.
#if defined(__MINGW32__)
 #define KIFACE_INSTANCE_NAME_AND_VERSION   "_KIFACE_1"
#else
 #define KIFACE_INSTANCE_NAME_AND_VERSION   "KIFACE_1"
#endif


#if defined(__linux__)
 #define LIB_ENV_VAR    wxT( "LD_LIBRARY_PATH" )

#elif defined(__WXMAC__)
 #define LIB_ENV_VAR    wxT( "DYLD_LIBRARY_PATH" )

#elif defined(__MINGW32__)
 #define LIB_ENV_VAR    wxT( "PATH" )
#endif


/**
 * Class PROJECT
 * holds project specific data.  Because it is in the neutral program top, which
 * is not linked to by subsidiarly DSOs, any functions in this interface must
 * be VTBL_ENTRYs.
 */
class PROJECT
{

public:

#if 0
    /// Derive PROJECT elements from this, it has a virtual destructor, and
    /// Elem*() functions can work with it.
    class ELEM_BASE
    {
    public:
        virtual ~ELEM_BASE() {}
    };

    VTBL_ENTRY int         ElemAllocNdx();
    VTBL_ENTRY void        ElemSet( int aIndex, ELEMENT_BASE* aBlock );
    VTBL_ENTRY ELEM_BASE*  ElemGet( int aIndex )
#endif
};


class KIWAY;
class wxWindow;
class wxApp;


/**
 * Struct KIFACE
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
    // Do not change the order of functions in this listing, add new ones at
    // the end, unless you recompile all of KiCad.

#define KFCTL_STANDALONE    (1<<0)      ///< Am running as a standalone Top.

    /**
     * Function CreateWindow
     * creates a wxTopLevelWindow for the current project.  The caller
     * must cast the return value into the known type.
     *
     * @param aClassId identifies which wxFrame or wxDialog to retrieve.
     *
     * @param aKIWAY tells the window which KIWAY (and PROJECT) it is a participant in.
     *
     * @param aCtlBits consists of bit flags from the set KFCTL_* #defined above.
     *
     * @return wxWindow* - and if not NULL, should be cast into the known type.
     */
    VTBL_ENTRY  wxWindow* CreateWindow( int aClassId, KIWAY* aKIWAY, int aCtlBits = 0 ) = 0;

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
    VTBL_ENTRY void* IfaceOrAddress( int aDataId ) = 0;
};


/**
 * Class KIWAY
 * is a minimalistic software bus for communications between various
 * DLLs/DSOs (DSOs) within the same KiCad process.  It makes it possible
 * to call between DSOs without having to link them together, and without
 * having to link to the top process module which houses the KIWAY(s).  It also
 * makes it possible to send custom wxEvents between DSOs and from the top
 * process module down into the DSOs.  The latter capability is thought useful
 * for driving the lower DSOs from a python test rig or for demo (automaton) purposes.
 * <p>
 * Most all calls are via virtual functions which means C++ vtables
 * are used to hold function pointers and eliminate the need to link to specific
 * object code libraries, speeding development and encouraging clearly defined
 * interface design.  There is one KIWAY in the launching portion of the process
 * for each open KiCad project.  Each project has its own KIWAY.  Within a KIWAY
 * is an actual PROJECT data structure.
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
        FACE_SCH,       ///< _eeschema DSO
    //  FACE_LIB,
        FACE_PCB,       ///< _pcbnew DSO
    //  FACE_MOD,

        FACE_COUNT      ///< how many KIWAY player types
    };

    // Don't change the order of these VTBL_ENTRYs, add new ones at the end,
    // unless you recompile all of KiCad.

    VTBL_ENTRY      KIFACE*     KiFACE( FACE_T aFaceId, bool doLoad );
    VTBL_ENTRY      PROJECT&    Project();

    KIWAY();

private:

    /// Get the name of the DSO holding the requested FACE_T.
    static const wxString dso_name( FACE_T aFaceId );

    // one for each FACE_T
    static wxDynamicLibrary    s_sch_dso;
    static wxDynamicLibrary    s_pcb_dso;
    //static wxDynamicLibrary    s_cvpcb_dso;        // will get merged into pcbnew

    KIFACE*     m_dso_players[FACE_COUNT];

    PROJECT     m_project;
};


/**
 * Function Pointer KIFACE_GETTER_FUNC
 * points to the one and only KIFACE export.  The export's address
 * is looked up via symbolic string and should be extern "C" to avoid name
 * mangling.  That function can also implement process initialization functionality,
 * things to do once per process that is DSO resident.  This function will only be
 * called one time.  The DSO itself however may be asked to support multiple
 * Top windows, i.e. multiple projects within its lifetime.
 *
 * @param aKIFACEversion is where to put the API version implemented by the KIFACE.
 * @param aKIWAYversion tells the KIFACE what KIWAY version will be available.
 * @param aProcess is a pointer to the basic wxApp for this process.
 * @return KIFACE* - unconditionally.
 */
typedef     KIFACE*  KIFACE_GETTER_FUNC( int* aKIFACEversion, int aKIWAYversion, wxApp* aProcess );

/// No name mangling.  Each TOPMOD will implement this once.
extern "C" KIFACE* KIFACE_GETTER(  int* aKIFACEversion, int aKIWAYversion, wxApp* aProcess );

#endif  // KIWAY_H_
