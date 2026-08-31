/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DIALOG_MDW_DESIGN_RELEASE_H
#define DIALOG_MDW_DESIGN_RELEASE_H

#include <dialog_shim.h>
#include <mdw/client/mdw_baseline_design.h>
#include <mdw/client/mdw_op.h>
#include <mdw/client/mdw_release_readiness.h>
#include <mdw/client/mdw_subscription.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

#include <wx/timer.h>
#include <wx/weakref.h>

class EDA_BASE_FRAME;
class GIT_COMMIT_PANE;
class MDW_SOURCE;
class PROJECT;
class wxButton;
class wxHyperlinkCtrl;
class wxListView;
class wxStaticText;
class wxTextCtrl;


/**
 * Checks every part placed in the saved schematic and board through the project's workspace,
 * takes an exception for each part its policy blocks, and creates the design baseline. Until the
 * design can be released its one action button takes the next step towards it.
 *
 * Shown quasi-modal, so the editors stay usable beside it; it checks again whenever it is
 * reactivated.
 */
class DIALOG_MDW_DESIGN_RELEASE : public DIALOG_SHIM
{
public:
    /// aPane commits and pushes for the design, and may be null
    DIALOG_MDW_DESIGN_RELEASE( EDA_BASE_FRAME* aFrame, GIT_COMMIT_PANE* aPane, PROJECT& aProject,
                               std::shared_ptr<MDW_SOURCE> aSource, std::vector<MDW_PLACED_INPUT> aInputs );

    /// The design item, known once its state has been read
    const MDW_ITEM_REF& DesignItem() const { return m_projectRef; }

private:
    void     setParts( const std::vector<MDW_PLACED_INPUT>& aInputs );
    void     findEcoSources();
    void     startCheck();

    /// Name the next step, from the editors, the project state and what the source said it would refuse on
    void     showReadiness( MDW_BASELINE_HOLD aHold, const wxString& aHolder );
    bool     workingCopy() const;

    void     startReviewCheck();
    void     showReviewFailure( const MDW_RESULT& aResult );
    void     showReviewGate();
    void     startEcoCheck();
    wxString ecoIds( const std::vector<size_t>& aIndices ) const;
    wxString exceptionText( const MDW_BASELINE_EXCEPTION& aException ) const;
    void     setLine( wxStaticText* aLine, const wxString& aText );
    void     showPending();
    void     showReview();
    void     showPartEcos( long aRow );
    void     fill();

    /// Fit the columns to the list's width, measuring their content first when aMeasure or the columns changed
    void     sizeColumns( bool aMeasure = true );

    /// Widen the dialog as far as the display allows so every column shows its content whole
    void     growToColumns();
    void     updateControls();
    void     onConnectionChanged();
    void     onRecordException( wxCommandEvent& aEvent );

    /// Prompt for an exception and record it on aRows, the rows selected when it was asked for
    void     recordException( const std::vector<long>& aRows );
    void     onAction( wxCommandEvent& aEvent );
    void     saveAll();

    /// Read the placed parts from the saved files again and check them
    void     reloadParts();

    /**
     * Read the parts again when their files changed since the last read, or only mark the list stale unless
     * aActivated, since a read shows a progress window. Otherwise check them again when aActivated, or when the
     * editors' unsaved state changed since the last check.
     */
    void     recheckIfChanged( bool aActivated );

    void     commitAndPush();

    /// Check the parts again and release them only when nothing got worse since the user saw them
    void     release();
    void     createBaseline();

    /// How many parts are flagged at aSeverity without an exception
    int      unexcepted( MDW_SEVERITY aSeverity ) const;

    EDA_BASE_FRAME*                          m_frame;
    wxWeakRef<GIT_COMMIT_PANE>               m_pane;
    PROJECT&                                 m_project;
    std::shared_ptr<MDW_SOURCE>              m_source;
    MDW_ITEM_REF                             m_projectRef;
    wxString                                 m_revisionId;
    wxString                                 m_revisionLabel;
    std::vector<MDW_DESIGN_PART>             m_parts;
    std::vector<MDW_BASELINE_CHECK>          m_checks;
    std::vector<MDW_BASELINE_EXCEPTION>      m_exceptions;
    bool                                     m_checked = false;
    bool                                     m_busy = false;
    bool                                     m_committing = false; ///< The pane commits and pushes for us
    MDW_PROJECT_STATE                        m_state;
    std::optional<MDW_RELEASE_READINESS>     m_readiness;
    bool                                     m_recheck = false;   ///< A connection changed while checking
    bool                                     m_prompting = false; ///< The exception prompt is open
    MDW_OP                                   m_op;
    MDW_OP                                   m_reviewOp;
    MDW_OP                                   m_ecoOp;
    std::vector<std::shared_ptr<MDW_SOURCE>> m_ecoSources;
    std::vector<wxString>                    m_ecoMissing;
    MDW_DESIGN_ECOS                          m_ecos;
    long                                     m_ecoColumn = -1;
    long                                     m_exceptionColumn = -1;
    kiapi::mdw::ListReviewsReply             m_reviews;
    std::optional<MDW_REVIEW_GATE>           m_reviewGate;
    std::vector<int>                         m_contentWidths; ///< Each column's autosized width
    MDW_SUBSCRIPTION                         m_connections;

    std::vector<std::filesystem::file_time_type> m_savedAt;  ///< The design files' times at the last read
    std::vector<std::filesystem::file_time_type> m_failedAt; ///< Their times at the last read that failed
    bool                                         m_reading = false;      ///< The parts are being read
    bool                                         m_stale = false;        ///< Saved since the last read
    bool                                         m_editorsDirty = false; ///< Unsaved edits at the last check

    /// Declared after the state its handler reads, so it is destroyed, and stopped, before that state
    wxTimer m_pollTimer;

    wxStaticText*    m_summary;
    wxStaticText*    m_pendingLine;
    wxStaticText*    m_reviewLine;
    wxHyperlinkCtrl* m_reviewLink;
    wxStaticText*    m_ecoLine;
    wxListView*      m_list;
    wxTextCtrl*      m_releaseNote;
    wxButton*        m_exceptionButton;
    wxButton*        m_releaseButton;
    wxButton*        m_cancelButton;
};


/**
 * Collect the parts placed in the saved schematic and board, and show the release gate for them.
 * aPane commits and pushes for the design, and may be null.
 */
void ShowDesignRelease( EDA_BASE_FRAME* aFrame, GIT_COMMIT_PANE* aPane );

#endif // DIALOG_MDW_DESIGN_RELEASE_H
