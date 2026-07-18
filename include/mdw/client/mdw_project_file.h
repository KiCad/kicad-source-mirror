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

/**
 * @file mdw_project_file.h
 * @brief The save policy for workspace items and the local record of check-outs.
 *
 * The policy functions are pure. The record functions persist which item each local file was
 * checked out from and at which revision.
 */

#ifndef MDW_PROJECT_FILE_H
#define MDW_PROJECT_FILE_H

#include <kicommon.h>
#include <mdw/client/mdw_lock.h>
#include <mdw/client/mdw_types.h>

#include <wx/string.h>

#include <atomic>
#include <mutex>
#include <string>

class PROJECT;
class SETTINGS_MANAGER;
struct MDW_PROJECT_BINDING;
struct MDW_ITEM_BASE;

/// What a save of a local file would run into, as MDW_CLIENT::CheckSave settles it
struct MDW_SAVE_CHECK
{
    bool                   managed = false; ///< False when not an item, unknown, or integration off
    MDW_LIFECYCLE_CATEGORY category = MDW_LIFECYCLE_CATEGORY::EDITABLE;
    MDW_LOCK_INFO          lock; ///< The lock tracker's cached entry, not a backend read
    wxString               item_name;
};

namespace MDW_SAVE_GUARD
{
/// The step of a save check still pending when its caller stopped waiting
enum class SAVE_PHASE
{
    LOOKUP,
    LIFECYCLE,
    SETTLED
};

enum class SAVE_ACTION
{
    SAVE,
    SAVE_UNCHECKED, ///< Save, then say the save was not checked against the release state
    PROMPT          ///< Ask before saving
};

/**
 * What to do with a save once its check settled or its wait ran out in aPhase. A lock already
 * cached for another user always prompts. aResult is read only when aPhase is SETTLED.
 */
KICOMMON_API SAVE_ACTION SaveAction( bool aCachedForeignLock, SAVE_PHASE aPhase, const MDW_SAVE_CHECK& aResult );

/**
 * The project file's bytes, taken before a backend call so the record functions can tell whether
 * the call rewrote it. Empty when the file cannot be read.
 */
KICOMMON_API std::string ProjectFileSnapshot( const PROJECT& aProject );

/**
 * Bind a checked-out working file to its item, persisted in the project's local settings. An empty
 * item id forgets the binding. aBase is the revision the reply named. aSnapshot is ProjectFileSnapshot
 * from before the backend call; when the project file differs from it now, its new settings are reread
 * into memory, and otherwise the shared project file is left untouched. aManager owns aProject. Main
 * thread only.
 */
KICOMMON_API bool RecordCheckout( SETTINGS_MANAGER& aManager, PROJECT& aProject, const wxString& aLocalPath,
                                  const wxString& aItemId, const MDW_ITEM_BASE& aBase, const std::string& aSnapshot );

/**
 * Move a file's base to the revision a download, check-in or release reply named, persisted in
 * the project's local settings. The base moves only when the file is bound to aItemId. aSnapshot
 * works as for RecordCheckout. aManager owns aProject. Main thread only.
 */
KICOMMON_API bool RecordBase( SETTINGS_MANAGER& aManager, PROJECT& aProject, const wxString& aLocalPath,
                              const wxString& aItemId, const MDW_ITEM_BASE& aBase, const std::string& aSnapshot );

/**
 * Store aBinding in aProject's file, saving it only when the binding changed, and clear any decision to sit
 * this computer out. A changed binding forgets every checkout, whose ids name the old workspace's items.
 * False, with the binding and every checkout as they were, when either file could not be saved. aManager owns
 * aProject. Main thread only.
 */
KICOMMON_API bool RecordBinding( SETTINGS_MANAGER& aManager, PROJECT& aProject, const MDW_PROJECT_BINDING& aBinding );

/**
 * The revision a check-in of aLocalPath as aItemId was edited from, as the last checkout, download or
 * check-in recorded it. Empty when no record names one for that item.
 */
KICOMMON_API wxString CheckinBase( const PROJECT& aProject, const wxString& aLocalPath, const wxString& aItemId );
} // namespace MDW_SAVE_GUARD

/// How far a CheckSave has got, written as it runs and readable from any thread
class MDW_SAVE_PROGRESS
{
public:
    std::atomic<MDW_SAVE_GUARD::SAVE_PHASE> phase{ MDW_SAVE_GUARD::SAVE_PHASE::LOOKUP };

    /// The item's cached lock, known once the lookup has found the item
    MDW_LOCK_INFO Lock() const
    {
        std::lock_guard<std::mutex> guard( m_mutex );
        return m_lock;
    }

    void SetLock( const MDW_LOCK_INFO& aLock )
    {
        std::lock_guard<std::mutex> guard( m_mutex );
        m_lock = aLock;
    }

private:
    mutable std::mutex m_mutex;
    MDW_LOCK_INFO      m_lock;
};

#endif // MDW_PROJECT_FILE_H
