/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2023 CERN (www.cern.ch)
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

#ifndef KIWAY_PLAYER_H_
#define KIWAY_PLAYER_H_

#include <wx/frame.h>

#include <vector>
#include <kiway_holder.h>
#include <eda_base_frame.h>

class KIWAY;
class PROJECT;
struct KIFACE;
class KIFACE_BASE;
class TOOL_MANAGER;
class KIWAY_EXPRESS;

class wxGUIEventLoop;
class wxSocketServer;
class wxSocketBase;
class wxSocketEvent;
class wxCmdLineParser;


/**
 * A wxFrame capable of the OpenProjectFiles function, meaning it can load a portion of
 * a KiCad project.
 *
 * Because this class provides a dummy implementation,it is not a certainty that all
 * classes which inherit from this class intend to participate in a #KIWAY.  Those that
 * do must actually interact with the provided #KIWAY.
 *
 * #EDA_BASE_FRAME would not have sufficed because #BM2CMP_FRAME_BASE is not derived
 * from it.
 */
#ifdef SWIG
class KIWAY_PLAYER : public wxFrame, public KIWAY_HOLDER
#else
class KIWAY_PLAYER : public EDA_BASE_FRAME
#endif
{
public:
    KIWAY_PLAYER( KIWAY* aKiway, wxWindow* aParent, FRAME_T aFrameType,
                  const wxString& aTitle, const wxPoint& aPos, const wxSize& aSize,
                  long aStyle, const wxString& aFrameName, const EDA_IU_SCALE& aIuScale );

    ~KIWAY_PLAYER() throw();

    // For the aCtl argument of OpenProjectFiles()
#define KICTL_NONKICAD_ONLY     (1<<0)   ///< chosen file is non-KiCad according to user
#define KICTL_KICAD_ONLY        (1<<1)   ///< chosen file is from KiCad according to user
#define KICTL_CREATE            (1<<2)   ///< caller thinks requested project files may not exist.
#define KICTL_IMPORT_LIB        (1<<3)   ///< import all footprints into a project library.
#define KICTL_REVERT            (1<<4)   ///< reverting to a previously-saved (KiCad) file.

    /**
     * Open a project or set of files given by @a aFileList.
     *
     * This is generalized in the direction of worst case.  In a typical case @a aFileList
     * will only hold a single file, like "myboard.kicad_pcb" because any #KIWAY_PLAYER
     * is only in one #KIWAY and the #KIWAY owns the #PROJECT.  Therefore opening files
     * from multiple projects into the same #KIWAY_PLAYER is precluded.
     *
     * Each derived class should handle this in a way specific to its needs.  No filename
     * prompting is done inside here for any file or project.  There should be no need to
     * call this with \a aFileList which is empty.  However, calling it with a single
     * filename which does not exist should indicate to the implementer that a new session
     * is being started and that the given name is the desired name for the data file at
     * time of save.
     *
     * This function does not support "appending".  Use a different function for that.  Any
     * prior project data tree should be cleared before loading the new stuff.  Therefore,
     * one of the first things an implementation should do is test for existence of the first
     * file in the list, and if it does not exist, treat it as a new session, possibly with
     * a UI notification to that effect.
     *
     * After loading the window should update its Title as part of this operation.  If the
     * #KIWAY_PLAYER needs to, it can load the *.pro file as part of this operation.
     *
     * If the KIWAY_PLAYER cannot load any of the file(s) in the list, then it should say
     * why through some GUI interface, and return false.
     *
     * @param aFileList includes files that this frame should open according to the knowledge
     *                  in the derived wxFrame.  In almost every case, the list will have only
     *                  a single file in it.
     * @param aCtl is a set of bit flags ORed together from the set of KICTL_* \#defined above.
     * @return true if all requested files were opened OK, else false.
     */
    virtual bool OpenProjectFiles( const std::vector<wxString>& aFileList, int aCtl = 0 )
    {
        // overload me for your wxFrame type.

        // Any overload should probably do this also:
        // Prj().MaybeLoadProjectSettings();

        // Then update the window title.

        return false;
    }


    /**
     * Show this wxFrame as if it were a modal dialog, with all other instantiated wxFrames
     * disabled until this #KIWAY_PLAYER derivative calls DismissModal().
     *
     * This is behavior is similar to a modal dialog window.  Not all KIWAY_PLAYERs use this
     * interface, so don't call this unless the implementation knows how to call DismissModal()
     * on a button click or double click or some special event which ends the modal behavior.
     *
     * @param aResult if not NULL, indicates a place to put a resultant string.
     * @param aResultantFocusWindow if not NULL, indicates what window to pass focus to on return.
     * @return true if frame implementation called KIWAY_PLAYER::DismissModal() with aRetVal
     *         of true.
     */
    virtual bool ShowModal( wxString* aResult = nullptr,
                            wxWindow* aResultantFocusWindow = nullptr );

    /**
     * Receive #KIWAY_EXPRESS messages from other players.
     *
     * Override it in derived classes.
     */
    virtual void KiwayMailIn( KIWAY_EXPRESS& aEvent );

    /**
     * Our version of Destroy() which is virtual from wxWidgets.
     */
    bool Destroy() override;

    bool IsModal() const override       { return m_modal; }
    void SetModal( bool aIsModal )      { m_modal = aIsModal; }

    /**
     * @return false only if both the frame is acting in modal mode and it has not been
     *         dismissed yet with DismissModal().  True if the dialog is not modal or if
     *         it is modal and has been dismissed.
     */
    bool IsDismissed();

    void DismissModal( bool aRetVal, const wxString& aResult = wxEmptyString );

    /* interprocess communication */
    void CreateServer( int service, bool local = true );
    void OnSockRequest( wxSocketEvent& evt );
    void OnSockRequestServer( wxSocketEvent& evt );

    /**
     * Execute a remote command sent via socket (to port KICAD_PCB_PORT_SERVICE_NUMBER,
     * currently 4242).
     *
     * Subclasses should override to implement actual command handlers.
     */
    virtual void ExecuteRemoteCommand( const char* cmdline ){}

protected:

    /// Event handler, routes to derivative specific virtual #KiwayMailIn().
    void kiway_express( KIWAY_EXPRESS& aEvent );

    /**
     * An event handler called on a language menu selection.
     */
    void language_change( wxCommandEvent& event );

    // variables for modal behavior support, only used by a few derivatives.
    bool            m_modal;        // true if frame is intended to be modal, not modeless

    /// Points to nested event_loop. NULL means not modal and dismissed.
    wxGUIEventLoop* m_modal_loop;
    wxWindow*       m_modal_resultant_parent; // the window caller in modal mode
    wxString        m_modal_string;
    bool            m_modal_ret_val;    // true if a selection was made

    wxSocketServer*             m_socketServer;
    std::vector<wxSocketBase*>  m_sockets;         /// Interprocess communication.

#ifndef SWIG
    DECLARE_EVENT_TABLE()
#endif
};

#endif // KIWAY_PLAYER_H_
