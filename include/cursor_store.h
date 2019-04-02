/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#ifndef CURSOR_STORE__H
#define CURSOR_STORE__H

#include <wx/cursor.h>

#include <map>
#include <vector>


/**
 * Simple class to construct and store cursors against unique ID keys.
 *
 * This can be used to lazily construct cursors as needed for specific
 * applications.
 */
class CURSOR_STORE
{
public:
    /**
     * Definition of a cursor
     */
    struct CURSOR_DEF
    {
        ///> The ID key used to uniquely identify a cursor in a given store
        int                  m_id_key;

        ///> The image data bitmap
        const unsigned char* m_image_data;

        ///> The mask data bitmap
        const unsigned char* m_mask_data;

        ///> The image size in pixels
        wxSize m_size;

        ///> The "hotspot" where the cursor "is" in the image
        wxPoint m_hotspot;
    };

    /**
     * Construct a store with a pre-set list of cursors.
     *
     * In future, an "Add()" function could be added if stores need to
     * dynamically add cursors.
     *
     * @param aDefs: the list of pre-set cursor definitions
     */
    CURSOR_STORE( const std::vector<CURSOR_DEF>& aDefs );

    /**
     * Get a given cursor by its ID
     * @param  aIdKey the ID key to look up
     * @return        the cursor, if found, else wxNullCursor
     */
    const wxCursor& Get( int aIdKey ) const;

private:
    ///> Internal store of cursors by ID
    std::map<int, wxCursor> m_store;
};

#endif // CURSOR_STORE__H