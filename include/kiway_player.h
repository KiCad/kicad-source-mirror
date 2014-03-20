#ifndef KIWAY_PLAYER_H_
#define KIWAY_PLAYER_H_

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

#include <wx/frame.h>
#include <vector>
#include <wxstruct.h>


class KIWAY;
class PROJECT;
struct KIFACE;
class KIFACE_I;


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
    KIWAY*      m_kiway;                        // no ownership.
};


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
    KIWAY_PLAYER( KIWAY* aKiway, wxWindow* aParent, ID_DRAWFRAME_TYPE aFrameType,
            const wxString& aTitle, const wxPoint& aPos, const wxSize& aSize,
            long aStyle, const wxString& aWdoName = wxFrameNameStr ) :
        EDA_BASE_FRAME( aParent, aFrameType, aTitle, aPos, aSize, aStyle, aWdoName ),
        KIWAY_HOLDER( aKiway )
    {}

    /// Don't use this one, only wxformbuilder uses it, and it must be augmented with
    /// a SetKiway() early in derived constructor.
    KIWAY_PLAYER( wxWindow* aParent, wxWindowID aId, const wxString& aTitle,
            const wxPoint& aPos, const wxSize& aSize, long aStyle,
            const wxString& aWdoName = wxFrameNameStr ) :
        EDA_BASE_FRAME( aParent, (ID_DRAWFRAME_TYPE) aId, aTitle, aPos, aSize, aStyle, aWdoName ),
        KIWAY_HOLDER( 0 )
    {}


    // For the aCtl argument of OpenProjectFiles()
#define KICTL_OPEN_APPEND       (1<<0)      ///< append the data file, rather than replace
#define KICTL_EAGLE_BRD         (1<<1)      ///< chosen *.brd file is Eagle according to user.

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
     * No prompting is done inside here for any file or project.  There is no
     * need to call this with aFileList which is empty.
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
    virtual bool OpenProjectFiles( const std::vector<wxString>& aFileList, int aCtl = 0 )
    {
        // overload me for your wxFrame type.

        // Any overload should probably do this also:
        // Prj().MaybeLoadProjectSettings();

        // Then update the window title.

        return false;
    }
};

#endif // KIWAY_PLAYER_H_
