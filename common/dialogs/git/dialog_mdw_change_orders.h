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

#pragma once

#include "dialog_mdw_change_orders_base.h"

#include <mdw/client/mdw_eco.h>
#include <mdw/client/mdw_op.h>
#include <mdw/client/mdw_review.h>
#include <widgets/mdw_change_orders.h>

#include <memory>
#include <optional>
#include <vector>

class MDW_SOURCE;


/**
 * The change orders and reviews a workspace source holds for a part or design. Read only; the
 * source's own tools create and route them, and Open in Browser leads there.
 */
class DIALOG_MDW_CHANGE_ORDERS : public DIALOG_MDW_CHANGE_ORDERS_BASE
{
public:
    DIALOG_MDW_CHANGE_ORDERS( wxWindow* aParent, std::shared_ptr<MDW_SOURCE> aSource,
                              const MDW_CHANGE_ORDERS_SUBJECT& aSubject );

    ~DIALOG_MDW_CHANGE_ORDERS() override;

    bool TransferDataToWindow() override;

    /// Show modally; does nothing when the source keeps neither change orders nor reviews
    static void Open( wxWindow* aParent, std::shared_ptr<MDW_SOURCE> aSource,
                      const MDW_CHANGE_ORDERS_SUBJECT& aSubject );

private:
    void loadEcos();
    void fillEcos();
    void loadReviews();
    void fillReviews();

    /// Widen the dialog so aGrid's auto-sized columns show in full, within the display
    void fitColumns( WX_GRID* aGrid );

    /// The text for a failed read of aThing, such as "change orders"
    wxString failureText( const MDW_RESULT& aResult, const wxString& aThing ) const;

    void showEco( int aRow );
    void showReview( int aRow );

    void OnEcoRetry( wxHyperlinkEvent& aEvent ) override;
    void OnShowClosed( wxCommandEvent& aEvent ) override;
    void OnEcoActivated( wxGridEvent& aEvent ) override;
    void OnEcoDetails( wxCommandEvent& aEvent ) override;
    void OnReviewRetry( wxHyperlinkEvent& aEvent ) override;
    void OnThisRevisionOnly( wxCommandEvent& aEvent ) override;
    void OnReviewActivated( wxGridEvent& aEvent ) override;
    void OnReviewDetails( wxCommandEvent& aEvent ) override;

    std::shared_ptr<MDW_SOURCE> m_source;
    MDW_CHANGE_ORDERS_SUBJECT   m_subject;

    std::vector<kiapi::mdw::Eco>    m_ecos; ///< Every row read so far, open and closed
    bool                            m_ecosPartial = false; ///< The page limit stopped the read
    std::vector<kiapi::mdw::Eco>    m_shownEcos;
    std::vector<kiapi::mdw::Review> m_reviews;
    bool                         m_approvalRequired = false;
    bool                         m_reviewsPartial = false;

    std::optional<kiapi::workspace::Schema> m_ecoSchema;

    MDW_OP m_ecoOp;
    MDW_OP m_reviewOp;
    MDW_OP m_detailOp;
};
