/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
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

#ifndef EDA_ITEM_FLAGS_H
#define EDA_ITEM_FLAGS_H

#include <cstdint>

// These define are used for the .m_flags member of the class EDA_ITEM
//
// NB: DO NOT ADD FLAGS ANYWHERE BUT AT THE END: THE FLAG-SET IS STORED AS AN INTEGER IN FILES.
//
#define IS_CHANGED              (1UL << 0)    ///< Item was edited, and modified
#define IS_LINKED               (1UL << 1)    ///< Used in calculation to mark linked items (temporary use)
#define IN_EDIT                 (1UL << 2)    ///< Item currently edited
#define IS_MOVING               (1UL << 3)    ///< Item being moved
#define IS_NEW                  (1UL << 4)    ///< New item, just created
#define IS_BROKEN               (1UL << 5)    ///< Is a segment just broken by BreakSegment

#define IS_DELETED              (1UL << 7)

#define STARTPOINT              (1UL << 9)    ///< When a line is selected, these flags indicate which
#define ENDPOINT                (1UL << 10)   ///< ends.  (Used to support dragging.)
#define SELECTED                (1UL << 11)   ///< Item was manually selected by the user
#define SELECTED_BY_DRAG        (1UL << 12)   ///< Item was algorithmically selected as a dragged item
#define STRUCT_DELETED          (1UL << 13)   ///< flag indication structures to be erased
#define CANDIDATE               (1UL << 14)   ///< flag indicating that the structure is connected
#define SKIP_STRUCT             (1UL << 15)   ///< flag indicating that the structure should be ignored

#define IS_PASTED               (1UL << 17)   ///< Modifier on IS_NEW which indicates it came from clipboard
#define IS_SHOWN_AS_BITMAP      (1UL << 18)
#define COURTYARD_CONFLICT      (1UL << 19)   ///< temporary set when moving footprints
                                              ///< having courtyard overlapping
#define MALFORMED_F_COURTYARD   (1UL << 20)
#define MALFORMED_B_COURTYARD   (1UL << 21)
#define MALFORMED_COURTYARDS    ( MALFORMED_F_COURTYARD | MALFORMED_B_COURTYARD )

#define ROUTER_TRANSIENT        (1UL << 22)   ///< transient items that should NOT be cached

#define CONNECTIVITY_CANDIDATE  (1UL << 23)   ///< flag indicating that the structure is connected for connectivity

#define HOLE_PROXY              (1UL << 24)   ///< Indicates the BOARD_ITEM is a proxy for its hole
#define SHOW_ELEC_TYPE          (1UL << 25)   ///< Show pin electrical type
#define BRIGHTENED              (1UL << 26)   ///< item is drawn with a bright contour

#define MCT_SKIP_STRUCT (1 << 27)    ///< flag used by the multichannel tool to mark items that should be skipped

#define UR_TRANSIENT             (1UL << 28)   ///< indicates the item is owned by the undo/redo stack

#define IS_DANGLING              (1UL << 29)   ///< indicates a pin is dangling
#define ENTERED                  (1UL << 30)   ///< indicates a group has been entered
#define SELECTION_CANDIDATE      (1UL << 31)   ///< indicates an item is a candidate for selection

// WARNING: if you add flags, you'll probably need to adjust the masks in GetEditFlags() and
// ClearTempFlags().

#define EDA_ITEM_ALL_FLAGS UINT32_MAX

typedef std::uint32_t EDA_ITEM_FLAGS;

// Helper function to convert flags to string descriptions
#include <string>
#include <vector>
#include <sstream>

inline std::string EDAItemFlagsToString( EDA_ITEM_FLAGS flags )
{
    struct FlagDesc
    {
        EDA_ITEM_FLAGS value;
        const char*    name;
    };

    static const FlagDesc flagDescs[] = { { IS_CHANGED, "IS_CHANGED" },
                                          { IS_LINKED, "IS_LINKED" },
                                          { IN_EDIT, "IN_EDIT" },
                                          { IS_MOVING, "IS_MOVING" },
                                          { IS_NEW, "IS_NEW" },
                                          { IS_BROKEN, "IS_BROKEN" },
                                          { IS_DELETED, "IS_DELETED" },
                                          { STARTPOINT, "STARTPOINT" },
                                          { ENDPOINT, "ENDPOINT" },
                                          { SELECTED, "SELECTED" },
                                          { SELECTED_BY_DRAG, "SELECTED_BY_DRAG" },
                                          { STRUCT_DELETED, "STRUCT_DELETED" },
                                          { CANDIDATE, "CANDIDATE" },
                                          { SKIP_STRUCT, "SKIP_STRUCT" },
                                          { IS_PASTED, "IS_PASTED" },
                                          { IS_SHOWN_AS_BITMAP, "IS_SHOWN_AS_BITMAP" },
                                          { COURTYARD_CONFLICT, "COURTYARD_CONFLICT" },
                                          { MALFORMED_F_COURTYARD, "MALFORMED_F_COURTYARD" },
                                          { MALFORMED_B_COURTYARD, "MALFORMED_B_COURTYARD" },
                                          { ROUTER_TRANSIENT, "ROUTER_TRANSIENT" },
                                          { CONNECTIVITY_CANDIDATE, "CONNECTIVITY_CANDIDATE" },
                                          { HOLE_PROXY, "HOLE_PROXY" },
                                          { SHOW_ELEC_TYPE, "SHOW_ELEC_TYPE" },
                                          { BRIGHTENED, "BRIGHTENED" },
                                          { UR_TRANSIENT, "UR_TRANSIENT" },
                                          { IS_DANGLING, "IS_DANGLING" },
                                          { ENTERED, "ENTERED" },
                                          { SELECTION_CANDIDATE, "SELECTION_CANDIDATE" } };

    std::vector<std::string> setFlags;
    for( const auto& desc : flagDescs )
    {
        if( flags & desc.value )
            setFlags.push_back( desc.name );
    }

    std::ostringstream oss;
    for( size_t i = 0; i < setFlags.size(); ++i )
    {
        if( i > 0 )
            oss << " | ";
        oss << setFlags[i];
    }
    if( setFlags.empty() )
        return "0";
    return oss.str();
}


#endif
