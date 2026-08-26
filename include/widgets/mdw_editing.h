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


#ifndef MDW_EDITING_H
#define MDW_EDITING_H

#include <kicommon.h>
#include <mdw/client/mdw_lock.h>

#include <functional>
#include <memory>
#include <optional>

#include <wx/string.h>

class MDW_SOURCE;
struct MDW_CLAIM_ACTION;
struct MDW_ITEM_REF;
struct MDW_RESULT;
class wxWindow;


/**
 * Report an asynchronous failure without a modal dialog, which would open a nested loop from a callback the
 * user did not just trigger. The owning frame's infobar shows it when it has one, otherwise its status bar
 * warning list does. Only an owner with no frame, such as one inside a dialog, gets an error box.
 */
KICOMMON_API void MdwReportFailure( wxWindow* aOwner, const wxString& aWhat, const wxString& aDetails = wxEmptyString );

/**
 * Report a failed result as above, with its headline in the infobar. The transport's own text in
 * MDW_RESULT::detail goes to the status bar warning list, where it can be read and never leads the report.
 */
KICOMMON_API void MdwReportFailure( wxWindow* aOwner, const wxString& aWhat, const MDW_RESULT& aResult );


/**
 * Start or stop editing aItem by taking or releasing its lock in aSource, or with aForce remove another
 * user's lock and tell them aReason. Giving up an enforced lock of our own undoes the check out. The lock
 * changes even if aOwner closes first. A refusal is reported by MdwReportFailure(); aDone runs
 * afterwards either way, unless aOwner is gone. aExpected is the lock the user confirmed removing, which a
 * forced release leaves alone once another claim holds the item.
 */
KICOMMON_API void MdwSetEditing( wxWindow* aOwner, const std::shared_ptr<MDW_SOURCE>& aSource,
                                 const MDW_ITEM_REF& aItem, bool aStart, bool aForce, const wxString& aReason,
                                 std::function<void()> aDone = nullptr,
                                 const std::optional<MDW_LOCK_INFO>& aExpected = std::nullopt );

/// Ask over aOwner for the reason the holder is told when aAction, a RELEASE, removes their claim. Nothing on cancel
KICOMMON_API std::optional<wxString> MdwAskClaimReason( wxWindow* aOwner, const MDW_CLAIM_ACTION& aAction );

/**
 * Run aAction, one of MdwClaimActions(), on aItem through MdwSetEditing(). A RELEASE first asks the user
 * for the reason its holder is told, and does nothing when they cancel.
 */
KICOMMON_API void MdwRunClaimAction( wxWindow* aOwner, const std::shared_ptr<MDW_SOURCE>& aSource,
                                     const MDW_ITEM_REF& aItem, const MDW_CLAIM_ACTION& aAction,
                                     std::function<void()> aDone = nullptr );

#endif // MDW_EDITING_H
