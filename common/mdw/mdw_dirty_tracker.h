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
 * @file mdw_dirty_tracker.h
 * @brief The local-modification flag for managed items, shared across frames.
 */

#ifndef MDW_DIRTY_TRACKER_H
#define MDW_DIRTY_TRACKER_H

#include <kicommon.h>

#include <cstdint>
#include <map>
#include <mutex>
#include <set>

#include <wx/string.h>

class KICOMMON_API MDW_DIRTY_TRACKER
{
public:
    void SetDirty( const wxString& aItemId );

    bool IsDirty( const wxString& aItemId ) const;

    /// Changes each time aItemId is marked dirty, so a check-in can tell a save that overtook it
    std::uint64_t Generation( const wxString& aItemId ) const;

    /// Clear aItemId unless it was marked dirty again after aGeneration was read
    void ClearIfUnchanged( const wxString& aItemId, std::uint64_t aGeneration );

private:
    mutable std::mutex                m_mutex;
    std::set<wxString>                m_dirty;
    std::map<wxString, std::uint64_t> m_generation;
    std::uint64_t                     m_lastGeneration = 0;
};

#endif // MDW_DIRTY_TRACKER_H
