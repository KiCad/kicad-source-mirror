/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.TXT for contributors.
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


enum class KICURSOR
{
    DEFAULT,
    ARROW,
    ARROW64,
    MOVING,
    MOVING64,
    PENCIL,
    PENCIL64,
    REMOVE,
    REMOVE64,
    HAND,
    HAND64,
    BULLSEYE,
    BULLSEYE64,
    VOLTAGE_PROBE,
    VOLTAGE_PROBE64,
    CURRENT_PROBE,
    CURRENT_PROBE64,
    TUNE,
    TUNE64,
    TEXT,
    TEXT64,
    MEASURE,
    MEASURE64,
    ADD,
    ADD64,
    SUBTRACT,
    SUBTRACT64,
    XOR,
    XOR64,
    ZOOM_IN,
    ZOOM_IN64,
    ZOOM_OUT,
    ZOOM_OUT64,
    LABEL_NET,
    LABEL_NET64,
    LABEL_GLOBAL,
    LABEL_GLOBAL64,
    COMPONENT,
    COMPONENT64,
    SELECT_WINDOW,
    SELECT_WINDOW64,
    SELECT_LASSO,
    SELECT_LASSO64,
    LINE_BUS,
    LINE_BUS64,
    LINE_GRAPHIC,
    LINE_GRAPHIC64,
    LINE_WIRE,
    LINE_WIRE64,
    LINE_WIRE_ADD,
    LINE_WIRE_ADD64,
    LABEL_HIER,
    LABEL_HIER64,
    PLACE,
    PLACE64
};

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
        ///< The ID key used to uniquely identify a cursor in a given store
        KICURSOR m_id_key;

        ///< The image data bitmap
        const unsigned char* m_image_data;

        ///< The mask data bitmap
        const unsigned char* m_mask_data;

        const char** m_xpm;

        ///< The image size in pixels
        wxSize m_size;

        ///< The "hotspot" where the cursor "is" in the image
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
    const wxCursor& Get( KICURSOR aIdKey ) const;

    static const wxCursor GetCursor( KICURSOR aCursorType );

    static const wxCursor GetHiDPICursor( KICURSOR aCursorType );

    static wxStockCursor GetStockCursor( KICURSOR aCursorType );

private:
    ///< Internal store of cursors by ID
    std::map<KICURSOR, wxCursor> m_store;
};

#endif // CURSOR_STORE__H
