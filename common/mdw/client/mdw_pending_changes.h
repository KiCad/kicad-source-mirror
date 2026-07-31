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
 * @file mdw_pending_changes.h
 * Changes a source accepted for later decision, remembered across sessions until they are decided.
 */

#ifndef MDW_PENDING_CHANGES_H
#define MDW_PENDING_CHANGES_H

#include <kicommon.h>
#include <mdw/client/mdw_client_types.h>
#include <mdw/client/mdw_op.h>
#include <mdw/client/mdw_subscription.h>

#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include <wx/string.h>

class MDW_SOURCE;


enum class MDW_PENDING_KIND
{
    PART_REVISION,
    DESIGN_BASELINE
};


struct MDW_PENDING_ENTRY
{
    wxString           locator; ///< The source that holds the change
    MDW_PENDING_CHANGE change;
    MDW_ITEM_REF       item;
    MDW_PENDING_KIND   kind = MDW_PENDING_KIND::PART_REVISION;
    MDW_PENDING_STATUS status = MDW_PENDING_STATUS::PENDING; ///< As last heard from the source
};


/**
 * A pending release or baseline survives a restart; its change id is all that finds it again.
 * Every source's changes are asked about once its session opens, and decided ones are forgotten and
 * announced as PENDING_RESOLVED. The file is read on every query and changed under a file lock,
 * so all KiCad processes agree.
 */
class KICOMMON_API MDW_PENDING_CHANGES
{
public:
    /// The store in the user's settings directory
    static MDW_PENDING_CHANGES& Get();

    explicit MDW_PENDING_CHANGES( const wxString& aFile );
    ~MDW_PENDING_CHANGES();

    void Remember( const MDW_PENDING_ENTRY& aEntry );

    void Forget( const wxString& aLocator, const wxString& aChangeId );

    /// File the changes remembered under the source locator aOld, and the items it stamped, under aNew
    void Rehome( const wxString& aOld, const wxString& aNew );

    /// The undecided changes to aItem, oldest first. aItem names its source
    std::vector<MDW_PENDING_ENTRY> For( const MDW_ITEM_REF& aItem ) const;

    std::vector<MDW_PENDING_ENTRY> All() const;

    /// Ask aSource about each change it holds, forgetting and announcing the decided ones and
    /// refreshing the rest
    void Recheck( const std::shared_ptr<MDW_SOURCE>& aSource );

    /// Recheck each source when its session opens or reconnects
    void Follow();

private:
    std::vector<MDW_PENDING_ENTRY> read() const;
    void                           write( const std::vector<MDW_PENDING_ENTRY>& aEntries ) const;

    /// Apply aChange to the stored entries as one step against every other KiCad process
    void update( const std::function<bool( std::vector<MDW_PENDING_ENTRY>& )>& aChange );

    wxString            m_file;
    mutable std::mutex  m_mutex;
    std::mutex          m_checkMutex; ///< Rechecks start from whichever thread an event arrives on
    std::vector<MDW_OP> m_checks;
    MDW_SUBSCRIPTION    m_events;
};

#endif // MDW_PENDING_CHANGES_H
