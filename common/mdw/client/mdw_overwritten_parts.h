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
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file mdw_overwritten_parts.h
 * Team parts a save wrote over a version of their library copy that the editor never loaded.
 */

#ifndef MDW_OVERWRITTEN_PARTS_H
#define MDW_OVERWRITTEN_PARTS_H

#include <kicommon.h>

#include <functional>
#include <mutex>
#include <set>
#include <utility>
#include <vector>

#include <wx/string.h>


/**
 * A part saved over a newer version in its copy holds content made from the older one, which a release would
 * publish over the newer version unseen. Each such part stays marked until it is settled against the copy, and
 * the mark survives a restart. The file is read on every query and changed under a file lock, so every process
 * agrees.
 */
class KICOMMON_API MDW_OVERWRITTEN_PARTS
{
public:
    /// The store in the user's settings directory
    static MDW_OVERWRITTEN_PARTS& Get();

    explicit MDW_OVERWRITTEN_PARTS( const wxString& aFile );

    /// Mark aItemIds of the copy whose rows carry aMarker, all or none; false when they could not be stored
    bool Mark( const wxString& aMarker, const std::vector<wxString>& aItemIds );

    /// Forget the mark of aItemId in the copy aMarker; false when it could not be forgotten
    bool Clear( const wxString& aMarker, const wxString& aItemId );

    bool Has( const wxString& aMarker, const wxString& aItemId ) const;

private:
    using PARTS = std::set<std::pair<wxString, wxString>>;

    PARTS read() const;
    bool  update( const std::function<bool( PARTS& )>& aChange );

    wxString           m_file;
    mutable std::mutex m_mutex;
};

#endif // MDW_OVERWRITTEN_PARTS_H
