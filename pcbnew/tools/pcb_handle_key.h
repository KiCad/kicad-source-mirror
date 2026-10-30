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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PCB_HANDLE_KEY_H
#define PCB_HANDLE_KEY_H

#include <optional>

#include <kiid.h>

class EDIT_POINT;
class EDIT_POINTS;

/**
 * Lasting name for one handle in an EDIT_POINTS collection.
 *
 * Point collections are rebuilt while editing, so an EDIT_POINT pointer goes stale.
 * A key names the handle by the parent item's KIID, whether the handle is a point or
 * a line, and the handle's index. Two keys for the same handle are the same key, so a
 * picked set holds each handle once.
 *
 * It also stores the collection's point and line counts, which Resolve() checks. If
 * the parent gained or lost handles the index could now name a different handle, so
 * the key stops resolving and the caller drops it.
 */
class PCB_HANDLE_KEY
{
public:
    /**
     * Build the key for a handle in a collection.
     *
     * @param aPoints the collection holding the handle.
     * @param aPoint the handle, looked up by address among the collection's points and lines.
     * @return the key, or std::nullopt when the handle is not in the collection or the
     *         collection has no parent.
     */
    static std::optional<PCB_HANDLE_KEY> Make( const EDIT_POINTS& aPoints, const EDIT_POINT* aPoint );

    /**
     * Find this key's handle in a collection, usually after it was rebuilt.
     *
     * @return the handle, or nullptr when the parent is a different item, the handle
     *         counts changed since the key was made, or the index is out of range.
     */
    EDIT_POINT* Resolve( EDIT_POINTS& aPoints ) const;

    const KIID& Parent() const { return m_parent; }
    bool        IsLine() const { return m_isLine; }
    int         Index() const { return m_index; }

    bool operator==( const PCB_HANDLE_KEY& aOther ) const;
    bool operator<( const PCB_HANDLE_KEY& aOther ) const;

private:
    PCB_HANDLE_KEY( const KIID& aParent, bool aIsLine, int aIndex, unsigned aPointCount, unsigned aLineCount );

    KIID     m_parent;
    bool     m_isLine;
    int      m_index;
    unsigned m_pointCount;
    unsigned m_lineCount;
};

#endif // PCB_HANDLE_KEY_H
