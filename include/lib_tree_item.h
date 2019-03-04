/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see change_log.txt for contributors.
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


#ifndef LIB_TREE_ITEM_H
#define LIB_TREE_ITEM_H

#include <base_struct.h>
#include <lib_id.h>
#include <import_export.h>

/**
 * A mix-in to provide polymorphism between items stored in libraries (symbols, aliases
 * and footprints).
 *
 * It is used primarily to drive the component tree for library browsing and editing.
 */

class APIEXPORT LIB_TREE_ITEM
{
public:
    virtual LIB_ID GetLibId() const = 0;

    virtual const wxString& GetName() const = 0;
    virtual wxString GetLibNickname() const = 0;

    virtual const wxString& GetDescription() = 0;

    virtual wxString GetSearchText() { return wxEmptyString; }

    /**
     * For items having aliases, IsRoot() indicates the principal item.
     */
    virtual bool IsRoot() const { return true; }

    /**
     * For items with units, return the number of units.
     */
    virtual int GetUnitCount() { return 0; }

    /**
     * For items with units, return an identifier for unit x.
     */
    virtual wxString GetUnitReference( int aUnit ) { return wxEmptyString; }
};

#endif //LIB_TREE_ITEM_H
