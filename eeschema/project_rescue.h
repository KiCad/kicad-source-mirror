/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2015 KiCad Developers, see change_log.txt for contributors.
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

#ifndef _LIB_CACHE_RESCUE_H_
#define _LIB_CACHE_RESCUE_H_

/* This code handles the case where an old schematic was made before
 * various changes were made, either to KiCad or to the libraries, and
 * the project needs to be recovered. The function of note is a member
 * of SCH_EDIT_FRAME, defined thus:
 *
 * bool SCH_EDIT_FRAME::RescueProject( bool aSilentIfNone );
 *
 * When this is called, a list of problematic components is compiled. If
 * this list is empty, then the function displays a notification and returns
 * (if aSilentIfNone is true, the notification is silenced).
 */

#include <schframe.h>

#include <vector>
#include <wx/string.h>
#include <boost/ptr_container/ptr_vector.hpp>

class LIB_PART;
class SCH_COMPONENT;
class RESCUER;

enum RESCUE_TYPE
{
    RESCUE_CONFLICT,
    RESCUE_CASE,
};

class RESCUE_CANDIDATE
{
public:
    virtual ~RESCUE_CANDIDATE() {}

    /**
     * Function GetRequestedName
     * Get the name that was originally requested in the schematic
     */
    virtual wxString GetRequestedName() const = 0;

    /**
     * Function GetNewName
     * Get the name we're proposing changing it to
     */
    virtual wxString GetNewName() const = 0;

    /**
     * Function GetCacheCandidate
     * Get the part that can be loaded from the project cache, if possible, or
     * else NULL.
     */
    virtual LIB_PART* GetCacheCandidate() const { return NULL; }

    /**
     * Function GetLibCandidate
     * Get the part the would be loaded from the libraries, if possible, or else
     * NULL.
     */
    virtual LIB_PART* GetLibCandidate() const { return NULL; }

    /**
     * Function GetActionDescription
     * Get a description of the action proposed, for displaying in the UI.
     */
    virtual wxString GetActionDescription() const = 0;

    /**
     * Function PerformAction
     * Perform the actual rescue action. If successful, this must log the rescue using
     * RESCUER::LogRescue to allow it to be reversed.
     * @return True on success.
     */
    virtual bool PerformAction( RESCUER* aRescuer ) = 0;
};

class RESCUE_LOG
{
public:
    SCH_COMPONENT*  component;
    wxString        old_name;
    wxString        new_name;
};

class RESCUER
{
    friend class DIALOG_RESCUE_EACH;

    std::vector<SCH_COMPONENT*> m_components;
    PART_LIBS* m_libs;
    PROJECT* m_prj;
    SCH_EDIT_FRAME* m_edit_frame;

    boost::ptr_vector<RESCUE_CANDIDATE> m_all_candidates;
    std::vector<RESCUE_CANDIDATE*> m_chosen_candidates;

    std::vector<RESCUE_LOG> m_rescue_log;

public:
    RESCUER( SCH_EDIT_FRAME& aEditFrame, PROJECT& aProject );

    /**
     * Function FindCandidates
     * Populate the RESCUER with all possible candidates.
     */
    void FindCandidates();

    /**
     * Function RemoveDuplicates
     * Filter out duplicately named rescue candidates.
     */
    void RemoveDuplicates();

    /**
     * Function GetCandidateCount
     */
    size_t GetCandidateCount() { return m_all_candidates.size(); }

    /**
     * Function GetChosenCandidateCount
     */
    size_t GetChosenCandidateCount() { return m_chosen_candidates.size(); }

    /**
     * Function GetComponents
     */
    std::vector<SCH_COMPONENT*>* GetComponents() { return &m_components; }

    /**
     * Function GetLibs
     */
    PART_LIBS* GetLibs() { return m_libs; }

    /**
     * Function GetPrj
     */
    PROJECT* GetPrj() { return m_prj; }

    /**
     * Function InvokeDialog
     * Display a dialog to allow the user to select rescues.
     * @param aAskShowAgain - whether the "Never Show Again" button should be visible
     */
    void InvokeDialog( bool aAskShowAgain );

    /**
     * Function LogRescue
     * Used by individual RESCUE_CANDIDATEs to log a rescue for undoing.
     */
    void LogRescue( SCH_COMPONENT *aComponent, const wxString& aOldName,
            const wxString& aNewName );

    /**
     * Function DoRescues
     * Perform all chosen rescue actions, logging them to be undone if necessary.
     * @return True on success
     */
    bool DoRescues();

    /**
     * Function UndoRescues
     * Reverse the effects of all rescues on the project.
     */
    void UndoRescues();
};

#endif // _LIB_CACHE_RESCUE_H_
