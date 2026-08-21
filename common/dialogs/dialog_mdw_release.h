/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef DIALOG_MDW_RELEASE_H
#define DIALOG_MDW_RELEASE_H

#include <dialogs/dialog_mdw_release_base.h>
#include <widgets/mdw_part_controller.h>

#include <functional>
#include <memory>
#include <utility>
#include <vector>


/**
 * The parts a release makes and the revision each becomes, what to know before releasing them and one comment for
 * all of them, then each part's progress as they are released one after another.
 */
class DIALOG_MDW_RELEASE : public DIALOG_MDW_RELEASE_BASE
{
public:
    using RELEASE = std::function<void( const wxString& aComment )>;

    /// aPrompt says what to show and what OK reads. aRelease starts the releases, which report through SetStatus.
    DIALOG_MDW_RELEASE( wxWindow* aParent, const MDW_RELEASE_PROMPT& aPrompt, RELEASE aRelease );

    /// Expires with the dialog, so a release answering after it closed leaves it alone
    std::weak_ptr<bool> Alive() const { return m_alive; }

    void SetStatus( size_t aRow, const wxString& aStatus );

    /// Show the concerns, notes and OK label of a listing that landed late, unless the release has started and the
    /// rows hold each part's progress
    void SetPrompt( const MDW_RELEASE_PROMPT& aPrompt );

    /// True once the user asked to stop, so no release that has not started may start
    bool CancelRequested() const { return m_state == STATE::CANCELLING; }

    /// A part started. Stop After This Part is offered only when aCanStop says a part waits behind it.
    void SetInFlight( bool aCanStop );

    /// Every release has answered or was cancelled, so the dialog can only be closed. Shows aSummary when set.
    void Finish( const wxString& aSummary = wxString() );

private:
    void onRelease( wxCommandEvent& aEvent );
    void onCancel( wxCommandEvent& aEvent );
    void showNote( const wxString& aOutcome );

    enum class STATE
    {
        READY,
        RUNNING,
        CANCELLING,
        FINISHED
    };

    RELEASE               m_release;
    wxArrayString         m_notes;
    std::shared_ptr<bool> m_alive;
    STATE                 m_state = STATE::READY;
};

#endif // DIALOG_MDW_RELEASE_H
