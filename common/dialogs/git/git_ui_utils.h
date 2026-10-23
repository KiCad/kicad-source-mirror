/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GIT_UI_UTILS_H
#define GIT_UI_UTILS_H

#include <mutex>
#include <vector>

class KIGIT_COMMON;
struct KIGIT_BRANCH_INFO;
class wxButton;
class wxDateTime;
class wxListCtrl;
class wxSplitterWindow;
class wxString;
class wxWindow;

namespace KIGIT
{

/// Show a busy message for a user action. UI thread only.
void ShowBusyMessage( wxWindow* aParent );

/// Try the action lock and show a busy message if it is unavailable.
/// UI thread only. Check owns_lock() before using the repository.
[[nodiscard]] std::unique_lock<std::mutex> TryLockAction( KIGIT_COMMON& aCommon, wxWindow* aParent );

/// TryLockAction that first waits a moment for a status read or fetch to let go. For applying
/// what the user has already typed, which the busy message would throw away.
[[nodiscard]] std::unique_lock<std::mutex> WaitForActionLock( KIGIT_COMMON& aCommon, wxWindow* aParent );

/// Put the current branch first, then local and remote branches with the newest tips first.
/// Use the name to break ties. Callers can choose a different order.
void SortBranchesForDisplay( std::vector<KIGIT_BRANCH_INFO>& aBranches );

/// "just now", "3 hours ago", or the ISO date once it is more than a month old.
wxString RelativeDate( const wxDateTime& aWhen );
wxString RelativeDate( const wxDateTime& aWhen, const wxDateTime& aNow );

/// Close a splitter's second pane, keeping the sash where the user left it for next time.
void UnsplitKeepingSash( wxSplitterWindow* aSplitter, wxWindow* aPane, int& aSashPos );

/// Paint a button's text the diff's red, for one that throws something away.
void MarkDestructive( wxButton* aButton );

/// The text of the selected row in a report list, or empty when nothing is selected.
wxString SelectedText( wxListCtrl* aList, int aColumn );

/// Check a name and email for recording commits. False leaves the reason in aError.
bool ValidateIdentity( const wxString& aName, const wxString& aEmail, wxString& aError );

} // namespace KIGIT

#endif // GIT_UI_UTILS_H
