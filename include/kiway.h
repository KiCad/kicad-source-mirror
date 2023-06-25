/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#ifndef KIWAY_H_
#define KIWAY_H_

/**
 * The KIWAY and KIFACE classes are used to communicate between various process
 * modules, all residing within a single process. The program modules are either
 * top level like an *.exe or subsidiary like a *.dll. In much of the documentation
 * the term DSO is used to refer to the *.dll portions, that is the term used on
 * linux. But it should be taken to mean DLL on Windows.
 *
 * <p>These are a couple of reasons why this design was chosen:
 *
 * <ol>
 *
 * <li>By using DSOs within a single process, it is not necessary to use IPC.
 * The DSOs can send wxEvents between themselves using wxEvtHandler interfaces in
 * a platform independent way.  There can also be function calls from one DSO to
 * another.</li>
 *
 * <li>The use of a number of separately linked DSOs closely resembles the original
 * KiCad program design, consisting of Eeschema and Pcbnew. But it also allows
 * separate compilation and linking of those two DSOs without a ton of inter-DSO
 * dependencies and common data structures. Linking smaller, purpose specific DSOs
 * is thought to be better for maintenance simplicity than a large single link
 * image. </li>
 *
 * <li>By keeping the core functionality in DSOs rather than EXE tops, it becomes
 * possible to re-use the DSOs under different program tops. For example, a DSO
 * named _pcbnew.so can be used under a C++ top or under a python top. Only one
 * CMake target must be defined to build either. Whether that is a separate build
 * or not is not the important thing. Simply having a single CMake target has
 * advantages. (Each builder person will have his/her own intentions relative to
 * use of python or not.) Once a DSO is python capable, it can be driven by any
 * number of python program tops, including demo-ing (automation) and testing
 * separately.</li>
 *
 *
 * </ol>
 *
 * All KiCad source code is UTF8 encoded by law, so make sure your editor is set
 * as such!  As such, it is OK to use UTF8 characters:
 *
 * ┏ ┗ ┓ ┛ ━ ┃
 *
 * <pre>
 *
 *                              ┏━━━process top━━━━━┓
 *                              ┃                   ┃       wxEvent channels
 *          ┏━━━━━━━━━━━━━━━━━━━-━[KIWAY project 1]━-━━━━━━━━━━━━━━━━━━━━━━┓
 *          ┃                   ┃                   ┃                      ┃
 *          ┃     ┏━━━━━━━━━━━━━-━[KIWAY project 2]━-━━━━━━━━━━┓           ┃
 *          ┃     ┃             ┃                   ┃          ┃           ┃
 *          ┃     ┃           ┏━-━[KIWAY project 3]━-━┓        ┃           ┃
 *          ┃     ┃           ┃ ┗━━━━━━━━━━━━━━━━━━━┛ ┃        ┃           ┃
 *          ┃     ┃           ┃                       ┃        ┃           ┃
 *          ┃     ┃           ┃                       ┃        ┃           ┃
 * ┏━━━━━━━━|━━━━━|━━━━━━━━━━━|━━━━━━━━━┓    ┏━━━━━━━━|━━━━━━━━|━━━━━━━━━━━|━━━━━┓
 * ┃ KIFACE ┃     ┃           ┃         ┃    ┃ KIFACE ┃        ┃           ┃     ┃
 * ┃        ┃     ┃           ┃         ┃    ┃        ┃        ┃           ┃     ┃
 * ┃        ┃     ┃           ┃         ┃    ┃        ┃        ┃           ┃     ┃
 * ┃┏━━━━━━━+━┓ ┏━+━━━━━━━┓ ┏━+━━━━━━━┓ ┃    ┃┏━━━━━━━+━┓ ┏━━━━+━━━━┓ ┏━━━━+━━━━┓┃
 * ┃┃wxFrame  ┃ ┃wxFrame  ┃ ┃wxFrame  ┃ ┃    ┃┃wxFrame  ┃ ┃wxFrame  ┃ ┃wxFrame  ┃┃
 * ┃┃project 1┃ ┃project 2┃ ┃project 3┃ ┃    ┃┃project 3┃ ┃project 2┃ ┃project 1┃┃
 * ┃┗━━━━━━━━━┛ ┗━━━━━━━━━┛ ┗━━━━━━━━━┛ ┃    ┃┗━━━━━━━━━┛ ┗━━━━━━━━━┛ ┗━━━━━━━━━┛┃
 * ┃                                    ┃    ┃                                   ┃
 * ┃                                    ┃    ┃                                   ┃
 * ┗━━━━━━ eeschema DSO ━━━━━━━━━━━━━━━━┛    ┗━━━━━━ pcbnew DSO ━━━━━━━━━━━━━━━━━┛
 *
 * </pre>
 *
 */


#include <atomic>
#include <wx/defs.h>
#include <wx/event.h>
#include <import_export.h>
#include <search_stack.h>
#include <project.h>
#include <frame_type.h>
#include <mail_type.h>
#include <ki_exception.h>


#define KIFACE_VERSION      1
#define KIFACE_GETTER       KIFACE_1

// The KIFACE acquisition function is declared extern "C" so its name should not
// be mangled.
#define KIFACE_INSTANCE_NAME_AND_VERSION   "KIFACE_1"

#ifndef SWIG
#if defined(__linux__) || defined(__FreeBSD__)
 #define LIB_ENV_VAR    wxT( "LD_LIBRARY_PATH" )
#elif defined(__WXMAC__)
 #define LIB_ENV_VAR    wxT( "DYLD_LIBRARY_PATH" )
#elif defined(_WIN32)
 #define LIB_ENV_VAR    wxT( "PATH" )
#else
 #error Platform support missing
#endif
#endif  // SWIG

class wxConfigBase;
class wxWindow;
class PGM_BASE;
class KIWAY;
class KIWAY_PLAYER;
class wxTopLevelWindow;
class TOOL_ACTION;
class JOB;
class REPORTER;
class PROGRESS_REPORTER;
class STARTWIZARD_PROVIDER;


/**
 * Implement a participant in the KIWAY alchemy.
 *
 * KIWAY is a minimalistic software bus for communications between various DLLs/DSOs
 * (DSOs) within the same KiCad process.  It makes it possible to call between DSOs
 * without having to link them together.  Most all calls are via virtual functions
 * which means C++ vtables are used to hold function pointers and eliminate the need
 * to link to specific object code libraries.  There is one KIWAY in the launching
 * portion of the process for each open KiCad project.  Each project has its own KIWAY.
 * Within a KIWAY is an actual PROJECT data structure.  A KIWAY also facilitates
 * communicating between DSOs on the topic of the project in question.
 */
struct KIFACE
{
    // The order of functions establishes the vtable sequence, do not change the
    // order of functions in this listing unless you recompile all clients of
    // this interface.

    virtual ~KIFACE() throw() {}

#define KFCTL_STANDALONE ( 1 << 0 )        ///< Running as a standalone Top.
#define KFCTL_CPP_PROJECT_SUITE ( 1 << 1 ) ///< Running under C++ project mgr, possibly with others.
#define KFCTL_CLI ( 1 << 2 )               ///< Running as CLI app


    /**
     * Called just once shortly after the DSO is loaded.
     * It is the second function called, immediately after the KIFACE_GETTER().  However
     * before either of those, static C++ constructors are called.  The DSO implementation
     * should do process level initialization here, not project specific since there will
     * be multiple projects open eventually.
     *
     * @param aProgram is the process block: #PGM_BASE*.
     * @param aCtlBits consists of bit flags from the set of KFCTL_* \#defines above.
     * @return true if DSO initialized OK, false if not.  When returning false, the loader
     *         may optionally decide to terminate the process or not, but will not put out
     *         any UI because that is the duty of this function to say why it is returning
     *         false.  Never return false without having reported to the UI why.
     */
    virtual bool OnKifaceStart( PGM_BASE* aProgram, int aCtlBits, KIWAY* aKiway ) = 0;

    /**
     * Called just once just before the DSO is to be unloaded.
     *
     * It is called before static C++ destructors are called.  A default implementation
     * is supplied.
     */
    virtual void OnKifaceEnd() = 0;

    /**
     * Reloads global state.
     */
    virtual void Reset() = 0;

    /**
     * Create a wxWindow for the current project.
     *
     * The caller must cast the return value into the known type.
     *
     * @param aParent may be NULL or is otherwise the parent to connect under.  If NULL
     *                then caller may want to connect the returned wxWindow into some
     *                hierarchy after this function returns.
     * @param aClassId identifies which wxFrame or wxDialog to retrieve, using a value
     *                 known to the implementing KIFACE.
     * @param aKIWAY tells the window which KIWAY (and PROJECT) it is a participant in.
     * @param aCtlBits consists of bit flags from the set of KFCTL_* \#defines above.
     * @return the window created and if not NULL, should be cast into the known type using
     *         and old school cast.  dynamic_cast is problematic since it needs typeinfo probably
     *         not contained in the caller's link image.
     */
    virtual wxWindow* CreateKiWindow( wxWindow* aParent, int aClassId,
                                      KIWAY* aKIWAY, int aCtlBits = 0 ) = 0;

    /**
     * Saving a file under a different name is delegated to the various KIFACEs because
     * the project doesn't know the internal format of the various files (which may have
     * paths in them that need updating).
     */
    virtual void SaveFileAs( const wxString& srcProjectBasePath,
                             const wxString& srcProjectName,
                             const wxString& newProjectBasePath,
                             const wxString& newProjectName,
                             const wxString& srcFilePath,
                             wxString& aErrors )
    {
        // If a KIFACE owns files then it needs to implement this....
    }

    /**
     * Return pointer to the requested object.
     *
     * The safest way to use this is to retrieve a pointer to a static instance of an
     * interface, similar to how the KIFACE interface is exported.  But if you know
     * what you are doing use it to retrieve anything you want.  Segfaults are your fault.
     *
     * @param aDataId identifies which object you want the address of, and consists of
     *                choices known in advance by the implementing KIFACE.
     * @return the requested object which must be cast into the known type.
     */
    virtual void* IfaceOrAddress( int aDataId ) = 0;

    /**
     * Append this Kiface's registered actions to the given list.
     */
    virtual void GetActions( std::vector<TOOL_ACTION*>& aActions ) const = 0;

    /**
     * Append this Kiface's registered startup provideers to the given list
     */
    virtual void GetStartupProviders( std::vector<std::unique_ptr<STARTWIZARD_PROVIDER>>& aProviders ) {}

    virtual int HandleJob( JOB* aJob, REPORTER* aReporter, PROGRESS_REPORTER* aProgressReporter )
    {
        return 0;
    }

    virtual bool HandleJobConfig( JOB* aJob, wxWindow* aParent )
    {
        return 0;
    }

    virtual void PreloadLibraries( PROJECT* aProject ) {}

    virtual void ProjectChanged() {}
};


/**
 * A minimalistic software bus for communications between various DLLs/DSOs (DSOs) within
 * the same KiCad process.
 *
 * It makes it possible to call between DSOs without having to link them together, and
 * without having to link to the top process module which houses the KIWAY(s).  More
 * importantly it makes it possible to send custom wxEvents between DSOs and from the top
 * process module down into the DSOs.  The latter capability is thought useful for driving
 * the lower DSOs from a python test rig or for demo (automation) purposes.
 * <p>
 * Most all calls are via virtual functions, which means C++ vtables are used to hold
 * function pointers and eliminate the need to link to specific object code libraries,
 * speeding development and encouraging clearly defined interface design.  Unlike Microsoft
 * COM, which is a multi-vendor design supporting DLL's built at various points in time,
 * the KIWAY alchemy is single project, with all components being built at the same time.
 * So one should expect solid compatibility between all KiCad components, as long at they
 * are compiled at the same time.
 * <p>
 * There is one KIWAY in the launching portion of the process for each open KiCad project.
 * Each project has its own KIWAY.  Available to each KIWAY is an actual PROJECT data
 * structure.  If you have a KIWAY, you can get to the PROJECT using #KIWAY::Prj().
 * <p>
 * In summary, a KIWAY facilitates communicating between DSOs, where the topic of the
 * communication is project specific.  Here a "project" means a #BOARD and a #SCHEMATIC
 * and a #NETLIST, (anything relating to production of a single #BOARD and added to class
 * #PROJECT.)
 */
class KICOMMON_API KIWAY : public wxEvtHandler
{
    friend struct PGM_SINGLE_TOP;        // can use set_kiface()

public:
    /// Known KIFACE implementations
    enum FACE_T
    {
        FACE_SCH,               ///< eeschema DSO
        FACE_PCB,               ///< pcbnew DSO
        FACE_CVPCB,
        FACE_GERBVIEW,
        FACE_PL_EDITOR,
        FACE_PCB_CALCULATOR,
        FACE_BMP2CMP,
        FACE_PYTHON,

        KIWAY_FACE_COUNT
    };

    ~KIWAY() throw () {}

    /**
     * A simple mapping function which returns the FACE_T which is known to implement
     * @a aFrameType.
     *
     * @return a valid value #KIWAY::FACE_T or FACE_T(-1) if given a bad @a aFrameType.
     */
    static FACE_T KifaceType( FRAME_T aFrameType );

    // If you change the vtable, recompile all of KiCad.

    /**
     * Return the KIFACE* given a FACE_T.
     *
     * If it is not already loaded, the KIFACE is loaded and initialized with a call to
     * KIFACE::OnKifaceStart().
     */
    virtual KIFACE* KiFACE( FACE_T aFaceId, bool doLoad = true );

    /**
     * Return the #KIWAY_PLAYER* given a FRAME_T.
     *
     * If it is not already created, the required KIFACE is found and loaded and initialized
     * if necessary, then the KIWAY_PLAYER window is created but not shown.  Caller must
     * Show() it.  If it is already created, then the existing KIWAY_PLAYER* pointer is
     * returned.
     *
     * @param aFrameType is from enum #FRAME_T.
     * @param doCreate when true asks that the player be created if it is not already created,
     *                 false means do not create and maybe return NULL.
     * @param aParent is a parent for modal #KIWAY_PLAYER frames, otherwise NULL used only
     *                when doCreate = true and by KIWAY_PLAYER frames created in modal form
     *
     * @return a valid opened #KIWAY_PLAYER or NULL if there is something wrong or doCreate
     *         was false and the player has yet to be created.
     *
     * @throw IO_ERROR if the *.kiface file could not be found, filled with text saying what.
     */
    virtual KIWAY_PLAYER* Player( FRAME_T aFrameType, bool doCreate = true,
                                  wxTopLevelWindow* aParent = nullptr );

    /**
     * Call the KIWAY_PLAYER::Close( bool force ) function on the window and if not vetoed,
     * returns true, else false.
     *
     * If window actually closes, then this KIWAY marks it as not opened internally.
     *
     * @return true if the window is closed and not vetoed, else false.
     */
    virtual bool PlayerClose( FRAME_T aFrameType, bool doForce );

    /**
     * Call the KIWAY_PLAYER::Close( bool force ) function on all the windows and if none
     * are vetoed, returns true, else false.
     *
     * If any window actually closes, then* this KIWAY marks it as not opened internally.
     *
     * @return true indicates that all windows closed because none were vetoed, false means
     *         at least one cast a veto.  Any that cast a veto are still open.
     */
    virtual bool PlayersClose( bool doForce );

    /**
     * Notifies a Kiway that a player has been closed.
     */
    void PlayerDidClose( FRAME_T aFrameType );

    /**
     * Send @a aPayload to @a aDestination from @a aSource.
     *
     * The recipient receives this in its #KIWAY_PLAYER::KiwayMailIn() function and can
     * efficiently switch() based on @a aCommand in there.
     */
    virtual void ExpressMail( FRAME_T aDestination, MAIL_T aCommand, std::string& aPayload,
                              wxWindow* aSource = nullptr );

    /**
     * Append all registered actions to the given list.
     */
    virtual void GetActions( std::vector<TOOL_ACTION*>& aActions ) const;

    /**
     * Return the #PROJECT associated with this KIWAY.
     *
     * This is here as an accessor, so that there is freedom to put the actual PROJECT storage
     * in a place decided by the implementation, and not known to the caller.
     */
    virtual PROJECT& Prj() const;

    /**
     * Change the language and then calls ShowChangedLanguage() on all #KIWAY_PLAYERs.
     */
    virtual void SetLanguage( int aLanguage );

    /**
     * Call CommonSettingsChanged() on all KIWAY_PLAYERs.
     *
     * Use after changing suite-wide options such as panning, autosave interval, etc.
     */
    virtual void CommonSettingsChanged( int aFlags = 0 );

    /**
     * Clear the wxWidgets file history on each open frame.  Preference records are handled
     * by SETTINGS_MANAGER (as not all frames might be open).
     */
    void ClearFileHistory();

    /**
     * Calls ProjectChanged() on all KIWAY_PLAYERs.
     * Used after changing the project to ensure all players are updated correctly.
     */
    virtual void ProjectChanged();

    KIWAY( int aCtlBits, wxFrame* aTop = nullptr );

    /**
     * Overwrites previously set ctl bits, only for use in kicad.cpp to flip between
     * standalone and manager mode before we actually load anything
     */
    void SetCtlBits( int aCtlBits ) { m_ctl = aCtlBits; }

    /**
     * Tell this KIWAY about the top most frame in the program and optionally allows it to
     * play the role of one of the KIWAY_PLAYERs if launched from single_top.cpp.
     *
     * @param aTop is the top most wxFrame in the entire program.
     */
    void SetTop( wxFrame* aTop );
    wxFrame* GetTop() { return m_top; }

    void OnKiCadExit();

    void OnKiwayEnd();

    bool ProcessEvent( wxEvent& aEvent ) override;

    int  ProcessJob( KIWAY::FACE_T aFace, JOB* aJob, REPORTER* aReporter = nullptr,
                     PROGRESS_REPORTER* aProgressReporter = nullptr );
    bool ProcessJobConfigDialog( KIWAY::FACE_T aFace, JOB* aJob, wxWindow* aWindow );

    /**
     * Gets the window pointer to the blocking dialog (to send it signals)
     * @return Pointer to blocking dialog window or null if none
     */
    wxWindow* GetBlockingDialog();
    void SetBlockingDialog( wxWindow* aWin );

private:
    /// Get the [path &] name of the DSO holding the requested FACE_T.
    const wxString dso_search_path( FACE_T aFaceId );

    bool set_kiface( FACE_T aFaceType, KIFACE* aKiface )
    {
        if( (unsigned) aFaceType < (unsigned) KIWAY_FACE_COUNT )
        {
            m_kiface[aFaceType] = aKiface;
            return true;
        }

        return false;
    }

    /**
     * @return the reference of the KIWAY_PLAYER having the type @a aFrameType if exists,
     *         or NULL if this KIWAY_PLAYER was not yet created, or was closed
     */
    KIWAY_PLAYER* GetPlayerFrame( FRAME_T aFrameType );

    static KIFACE*  m_kiface[KIWAY_FACE_COUNT];
    static int      m_kiface_version[KIWAY_FACE_COUNT];

    int             m_ctl;

    wxFrame*        m_top;      // Usually m_top is the Project manager

    wxWindowID      m_blockingDialog;

    // An array to store the window ID of PLAYER frames which were run.
    // A non empty name means only a PLAYER was run at least one time.
    // Empty entries are represented by wxID_NONE.
    // They can be closed, and the stored window ID may be invalid.
    // Call: wxWindow::FindWindowById( m_playerFrameId[aFrameType] )
    // to know if still exists (or GetPlayerFrame( FRAME_T aFrameType )
    std::atomic<wxWindowID> m_playerFrameId[KIWAY_PLAYER_COUNT];
};


#ifndef SWIG
// provided by single_top.cpp and kicad.cpp;
extern KIWAY Kiway;
// whereas python launchers: single_top.py and project manager instantiate as a python object
#endif


/**
 * Point to the one and only KIFACE export.
 *
 * The export's address is looked up via symbolic string and should be extern "C" to avoid name
 * mangling. This function will only be called one time.  The DSO itself however may be asked
 * to support multiple Top windows, i.e. multiple projects within its lifetime.
 *
 * @param aKIFACEversion is where to put the API version implemented by the KIFACE.
 * @param aKIWAYversion tells the KIFACE what KIWAY version will be available.
 * @param aProgram is a pointer to the PGM_BASE for this process.
 * @return unconditionally, cannot fail.
 */
typedef KIFACE* KIFACE_GETTER_FUNC( int* aKIFACEversion, int aKIWAYversion, PGM_BASE* aProgram );


#ifndef SWIG

/// No name mangling.  Each KIFACE (DSO/DLL) will implement this once.
extern "C" {
#if defined(BUILD_KIWAY_DLL)
    KIFACE_API KIFACE* KIFACE_GETTER(  int* aKIFACEversion, int aKIWAYversion, PGM_BASE* aProgram );
#else
    KIFACE* KIFACE_GETTER(  int* aKIFACEversion, int aKIWAYversion, PGM_BASE* aProgram );
#endif
}

#endif  // SWIG

#endif  // KIWAY_H_
