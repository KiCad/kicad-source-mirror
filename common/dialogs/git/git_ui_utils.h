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

#include <functional>
#include <memory>
#include <string>

class wxButton;
class wxDateTime;
class wxListCtrl;
class wxSplitterWindow;
class wxString;
class wxWindow;

namespace KIGIT
{

/// "just now", "3 hours ago", or the ISO date once it is more than a month old.
wxString RelativeDate( const wxDateTime& aWhen );

/// Close a splitter's second pane, keeping the sash where the user left it for next time.
void UnsplitKeepingSash( wxSplitterWindow* aSplitter, wxWindow* aPane, int& aSashPos );

/// Paint a button's text the diff's red, for one that throws something away.
void MarkDestructive( wxButton* aButton );

/// The text of the selected row in a report list, or empty when nothing is selected.
wxString SelectedText( wxListCtrl* aList, int aColumn );

/// Check a name and email for recording commits. False leaves the reason in aError.
bool ValidateIdentity( const wxString& aName, const wxString& aEmail, wxString& aError );

/**
 * Run aFn on the UI thread.
 *
 * For a transfer that outlives the window that started it. The work is queued on the
 * application rather than on that window, because the window may be part way through its own
 * destructor, and queueing onto a handler being destroyed is a race.
 */
void CallOnUi( std::function<void()> aFn );

/// As CallOnUi, for work that touches the window that started the transfer. aWatch is locked
/// on the UI thread, so a window that has gone is never touched.
void CallIfAlive( const std::weak_ptr<bool>& aWatch, std::function<void()> aFn );

/**
 * Run aWork on a thread the orphan registry owns.
 *
 * The registry joins with a bounded timeout at shutdown and detaches whatever has not
 * finished, so a window closing mid transfer never waits for the socket. False when no
 * thread could be started, which only happens once shutdown has begun.
 */
bool StartTransfer( const std::string& aLabel, std::function<void()> aWork );

} // namespace KIGIT

#endif // GIT_UI_UTILS_H
