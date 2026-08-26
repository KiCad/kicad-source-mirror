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


#include <widgets/mdw_editing.h>

#include <confirm.h>
#include <mdw/client/mdw_lock.h>
#include <mdw/client/mdw_source.h>
#include <mdw/mdw_claim_text.h>
#include <trace_helpers.h>
#include <ui_events.h>
#include <widgets/kistatusbar.h>

#include <wx/app.h>
#include <wx/filename.h>
#include <wx/frame.h>
#include <wx/log.h>
#include <wx/textdlg.h>
#include <wx/weakref.h>
#include <wx/window.h>
#include <wx/intl.h>
#include <wx/thread.h>


namespace
{
// The transport thread copies the callback, and a weak ref may only be touched on the UI thread
void deleteOnUiThread( wxWeakRef<wxWindow>* aRef )
{
    if( wxIsMainThread() )
    {
        delete aRef;
    }
    else if( wxTheApp )
    {
        wxTheApp->CallAfter(
                [aRef]()
                {
                    delete aRef;
                } );
    }
}


KISTATUSBAR* statusBarOf( wxWindow* aOwner )
{
    wxFrame* frame = aOwner ? dynamic_cast<wxFrame*>( wxGetTopLevelParent( aOwner ) ) : nullptr;

    return frame ? dynamic_cast<KISTATUSBAR*>( frame->GetStatusBar() ) : nullptr;
}
} // namespace


void MdwReportFailure( wxWindow* aOwner, const wxString& aWhat, const wxString& aDetails )
{
    const wxString text = aDetails.IsEmpty() ? aWhat : aWhat + wxS( " " ) + aDetails;
    wxCommandEvent report( EDA_EVT_SHOW_ERROR_INFOBAR );

    report.SetString( text );

    // Command events bubble up to the owning frame
    if( aOwner && aOwner->GetEventHandler()->ProcessEvent( report ) )
        return;

    KISTATUSBAR* statusBar = statusBarOf( aOwner );

    if( statusBar )
        statusBar->AddWarningMessages( wxS( "mdw" ), text );
    else
        DisplayErrorMessage( aOwner, aWhat, aDetails );
}


void MdwReportFailure( wxWindow* aOwner, const wxString& aWhat, const MDW_RESULT& aResult )
{
    wxWeakRef<wxWindow> owner( aOwner );

    MdwReportFailure( aOwner, aWhat, MdwErrorMessage( aResult ) );

    if( aResult.detail.IsEmpty() )
        return;

    // The report may have been a modal error box, and the owner can close under it
    KISTATUSBAR* statusBar = owner ? statusBarOf( owner.get() ) : nullptr;

    if( statusBar )
        statusBar->AddWarningMessages( wxS( "mdw" ), aResult.detail );
    else
        wxLogTrace( traceMdw, wxS( "%s %s" ), aWhat, aResult.detail );
}


void MdwSetEditing( wxWindow* aOwner, const std::shared_ptr<MDW_SOURCE>& aSource, const MDW_ITEM_REF& aItem,
                    bool aStart, bool aForce, const wxString& aReason, std::function<void()> aDone,
                    const std::optional<MDW_LOCK_INFO>& aExpected )
{
    if( !aSource || aItem.item_id.IsEmpty() )
        return;

    std::shared_ptr<wxWeakRef<wxWindow>> owner( new wxWeakRef<wxWindow>( aOwner ), deleteOnUiThread );
    wxString                             name = wxFileName( aItem.item_id ).GetFullName();
    const MDW_LOCK_INFO                  held = aSource->Claim( aItem );

    auto report = [owner, name, aStart, aForce, aDone]( const MDW_RESULT& aResult )
    {
        if( !*owner )
            return;

        if( !aResult.IsOk() )
        {
            wxString what = aStart   ? wxString::Format( _( "Could not start editing %s." ), name )
                            : aForce ? wxString::Format( _( "Could not release the claim on %s." ), name )
                                     : wxString::Format( _( "Could not stop editing %s." ), name );

            MdwReportFailure( owner->get(), what, aResult );
        }

        // A modal error box above may have outlived the owner that aDone works on
        if( aDone && *owner )
            aDone();
    };

    MDW_OP op;

    if( aStart )
    {
        op = aSource->Lock( aItem,
                            [report]( const MDW_RESULT& aResult, const MDW_LOCK_INFO& )
                            {
                                report( aResult );
                            } );
    }
    else if( !aForce && held.locked_by_me && !held.advisory )
    {
        // An enforced lock is a check out, which Undo Check Out gives back through the backend's own cancel
        op = aSource->CancelCheckout( aItem, wxEmptyString,
                                      [report]( const MDW_RESULT& aResult, const MDW_NONE& )
                                      {
                                          report( aResult );
                                      } );
    }
    else
    {
        op = aSource->Unlock(
                aItem, aForce,
                [report]( const MDW_RESULT& aResult, const MDW_LOCK_INFO& )
                {
                    report( aResult );
                },
                aReason, aExpected );
    }

    // Dropping the handle would cancel the lock change along with the report
    op.Detach();
}


std::optional<wxString> MdwAskClaimReason( wxWindow* aOwner, const MDW_CLAIM_ACTION& aAction )
{
    wxString caption;

    if( !aAction.label.EndsWith( wxS( "..." ), &caption ) )
        caption = aAction.label;

    wxTextEntryDialog dlg( aOwner, aAction.confirm, caption );

    if( dlg.ShowModal() != wxID_OK )
        return std::nullopt;

    return dlg.GetValue();
}


void MdwRunClaimAction( wxWindow* aOwner, const std::shared_ptr<MDW_SOURCE>& aSource, const MDW_ITEM_REF& aItem,
                        const MDW_CLAIM_ACTION& aAction, std::function<void()> aDone )
{
    using KIND = MDW_CLAIM_ACTION::KIND;

    wxString            reason;
    wxWeakRef<wxWindow> owner( aOwner );

    if( aAction.kind == KIND::RELEASE )
    {
        std::optional<wxString> asked = MdwAskClaimReason( aOwner, aAction );

        // The owner can close while the question is up
        if( !asked || ( aOwner && !owner ) )
            return;

        reason = *asked;
    }

    // The claim may change hands while the reason is asked, and only the one the user confirmed may go
    std::optional<MDW_LOCK_INFO> expected;

    if( aAction.kind == KIND::RELEASE )
        expected = aAction.claim;

    MdwSetEditing( aOwner, aSource, aItem, aAction.kind == KIND::START, aAction.kind == KIND::RELEASE, reason,
                   std::move( aDone ), expected );
}
