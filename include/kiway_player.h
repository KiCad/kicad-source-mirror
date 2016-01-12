/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2014 KiCad Developers, see change_log.txt for contributors.
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
#include <wxstruct.h>


class KIWAY;
class PROJECT;
struct KIFACE;
class KIFACE_I;

#define VTBL_ENTRY          virtual


/**
 * Class KIWAY_HOLDER
 * is a mix in class which holds the location of a wxWindow's KIWAY.  It allows
 * calls to Kiway() and SetKiway().
 *
 * Known to be used in at least DIALOG_SHIM and KIWAY_PLAYER classes.
 */
class KIWAY_HOLDER
{
public:
    KIWAY_HOLDER( KIWAY* aKiway ) :
        m_kiway( aKiway )
    {}

    /**
     * Function Kiway
     * returns a reference to the KIWAY that this object has an opportunity
     * to participate in.  A KIWAY_HOLDER is not necessarily a KIWAY_PLAYER.
     */
    KIWAY& Kiway() const
    {
        wxASSERT( m_kiway );    // smoke out bugs in Debug build, then Release runs fine.
        return *m_kiway;
    }

    /**
     * Function Prj
     * returns a reference to the PROJECT "associated with" this KIWAY.
     */
    PROJECT& Prj() const;

    /**
     * Function SetKiway
     *
     * @param aDest is the recipient of aKiway pointer.
     *  It is only used for debugging, since "this" is not a wxWindow*.  "this" is
     *  a KIWAY_HOLDER mix-in.
     *
     * @param aKiway is often from a parent window, or from KIFACE::CreateWindow().
     */
    void SetKiway( wxWindow* aDest, KIWAY* aKiway );

private:
    // private, all setting is done through SetKiway().
    KIWAY*          m_kiway;            // no ownership.
};


class KIWAY_EXPRESS;

#define WX_EVENT_LOOP      wxGUIEventLoop


class WX_EVENT_LOOP;


/**
 * Class KIWAY_PLAYER
 * is a wxFrame capable of the OpenProjectFiles function, meaning it can load
 * a portion of a KiCad project.  Because this class provides a dummy implementation,
 * it is not a certainty that all classes which inherit from this clas intend to
 * participate in a KIWAY.  Those that do must actually interact with the provided
 * KIWAY*.
 * <p>
 * EDA_BASE_FRAME would not have sufficed because BM2CMP_FRAME_BASE is not
 * derived from it.
 */
class KIWAY_PLAYER : public EDA_BASE_FRAME, public KIWAY_HOLDER
{
public:
    KIWAY_PLAYER( KIWAY* aKiway, wxWindow* aParent, FRAME_T aFrameType,
            const wxString& aTitle, const wxPoint& aPos, const wxSize& aSize,
            long aStyle, const wxString& aWdoName = wxFrameNameStr );

    /// Don't use this one, only wxformbuilder uses it, and it must be augmented with
    /// a SetKiway() early in derived constructor.
    KIWAY_PLAYER( wxWindow* aParent, wxWindowID aId, const wxString& aTitle,
            const wxPoint& aPos, const wxSize& aSize, long aStyle,
            const wxString& aWdoName = wxFrameNameStr );

    ~KIWAY_PLAYER();

    //----<Cross Module API>-----------------------------------------------------

    // For the aCtl argument of OpenProjectFiles()
#define KICTL_EAGLE_BRD         (1<<0)      ///< chosen *.brd file is Eagle according to user.
#define KICTL_CREATE            (1<<1)      ///< caller thinks requested project files may not exist

    /**
     * Function OpenProjectFiles
     * is abstract, and opens a project or set of files given by @a aFileList.
     * This is generalized in the direction of worst case.  In a typical case
     * @a aFileList will only hold a single file, like "myboard.kicad_pcb",
     * because any KIWAY_PLAYER is only in one KIWAY and the KIWAY owns the
     * PROJECT.  Therefore opening files from multiple projects into the same
     * KIWAY_PLAYER is precluded.
     * <p>
     * Each derived class should handle this in a way specific to its needs.
     * No filename prompting is done inside here for any file or project.  There should
     * be no need to call this with aFileList which is empty.  However, calling it with
     * a single filename which does not exist should indicate to the implementor
     * that a new session is being started and that the given name is the desired
     * name for the data file at time of save.
     * <p>
     * This function does not support "appending".  Use a different function for that.
     * Any prior project data tree should be cleared before loading the new stuff.
     * <p>
     * Therefore, one of the first things an implementation should do is test for
     * existence of the first file in the list, and if it does not exist, treat
     * it as a new session, possibly with a UI notification to that effect.
     * <p>
     * After loading the window should update its Title as part of this operation.
     * If the KIWAY_PLAYER needs to, it can load the *.pro file as part of this operation.
     * <p>
     * If the KIWAY_PLAYER cannot load any of the file(s) in the list, then it
     * should say why through some GUI interface, and return false.
     *
     * @param aFileList includes files that this frame should open
     *  according to the knowledge in the derived wxFrame.  In almost every case,
     *  the list will have only a single file in it.
     *
     * @param aCtl is a set of bit flags ORed together from the set of KICTL_* \#defined above.
     *
     * @return bool - true if all requested files were opened OK, else false.
     */
    VTBL_ENTRY bool OpenProjectFiles( const std::vector<wxString>& aFileList, int aCtl = 0 )
    {
        // overload me for your wxFrame type.

        // Any overload should probably do this also:
        // Prj().MaybeLoadProjectSettings();

        // Then update the window title.

        return false;
    }

    /**
     * Function ShowModal
     * puts up this wxFrame as if it were a modal dialog, with all other instantiated
     * wxFrames disabled until this KIWAY_PLAYER derivative calls DismissModal().
     * That is, behavior is similar to a modal dialog window.  Not all KIWAY_PLAYERs
     * use this interface, so don't call this unless the implementation knows how
     * to call DismissModal() on a button click or double click or some special
     * event which ends the modal behavior.
     *
     * @param aResult if not NULL, indicates a place to put a resultant string.
     * @param aResultantFocusWindow if not NULL, indicates what window to pass focus to on return.
     *
     * @return bool - true if frame implementation called KIWAY_PLAYER::DismissModal()
     *  with aRetVal of true.
     */
    VTBL_ENTRY bool ShowModal( wxString* aResult = NULL, wxWindow* aResultantFocusWindow = NULL );

    //----</Cross Module API>----------------------------------------------------


    /**
     * Function KiwayMailIn
     * receives KIWAY_EXPRESS messages from other players.  Merely override it
     * in derived classes.
     */
    virtual void KiwayMailIn( KIWAY_EXPRESS& aEvent );

    /**
     * Our version of Destroy() which is virtual from wxWidgets
     */
    bool Destroy();

protected:

    bool IsModal()                      { return m_modal; }
    void SetModal( bool IsModal )       { m_modal = IsModal; }

    /**
     * Function IsDismissed
     * returns false only if both the frame is acting in modal mode and it has not been
     * dismissed yet with DismissModal().  IOW, it will return true if the dialog is
     * not modal or if it is modal and has been dismissed.
     */
    bool IsDismissed();

    void DismissModal( bool aRetVal, const wxString& aResult = wxEmptyString );

    /// event handler, routes to derivative specific virtual KiwayMailIn()
    void kiway_express( KIWAY_EXPRESS& aEvent );

    /**
     * Function language_change
     * is an event handler called on a language menu selection.
     */
    void language_change( wxCommandEvent& event );

    // variables for modal behavior support, only used by a few derivatives.
    bool            m_modal;        // true if frame is intended to be modal, not modeless
    WX_EVENT_LOOP*  m_modal_loop;   // points to nested event_loop, NULL means not modal and dismissed
    wxWindow*       m_modal_resultant_parent; // the window caller in modal mode
    wxString        m_modal_string;
    bool            m_modal_ret_val;    // true if a selection was made

    DECLARE_EVENT_TABLE()
};


// psuedo code for OpenProjectFiles
#if 0

bool OpenProjectFiles( const std::vector<wxString>& aFileList, int aCtl = 0 )
{
    if( aFileList.size() != 1 )
    {
        complain via UI.
        return false
    }

    assert( aFileList[0] is absolute )      // bug in single_top.cpp or project manager.

    if( !Pgm().LockFile( fullFileName ) )
    {
        DisplayError( this, _( "This file is already open." ) );
        return false;
    }

    if current open project files have been modified
    {
        ask if user wants to save them and if yes save.
    }

    unload any currently open project files.

    Prj().SetProjectFullName( )

    if( aFileList[0] does not exist )
    {
        notify user file does not exist and ask if he wants to create it
        if( yes )
        {
            create empty project file(s)
            mark file as modified.

            use the default project config file.
        }
        else
            return false
    }
    else
    {
        load aFileList[0]

        use the project config file for project given by aFileList[0]s full path.
    }

    UpdateFileHistory( g_RootSheet->GetScreen()->GetFileName() );

    /* done in ReDraw typically:
    UpdateTitle();
    */

    show contents.
}



#endif




#endif // KIWAY_PLAYER_H_
