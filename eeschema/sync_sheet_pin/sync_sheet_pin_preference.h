/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Ethan Chien <liangtie.qian@gmail.com>
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

#ifndef SYNC_SHEET_PIN_PREFERENCE_H
#define SYNC_SHEET_PIN_PREFERENCE_H

#include <bitmaps/bitmap_types.h>
#include <bitmaps/bitmaps_list.h>
#include <map>
#include <wx/colour.h>

class SYNC_SHEET_PIN_PREFERENCE
{
public:
    enum ICON_SIZE
    {
        NORMAL_WIDTH = 16,
        NORMAL_HEIGHT = 16

    };

    enum BOOKCTRL_ICON_INDEX
    {
        HAS_UNMATCHED,
        ALL_MATCHED
    };

    static const std::map<BOOKCTRL_ICON_INDEX, BITMAPS>& GetBookctrlPageIcon()
    {
        static std::map<BOOKCTRL_ICON_INDEX, BITMAPS> mapping = {
            { HAS_UNMATCHED, BITMAPS::erc_green }, { ALL_MATCHED, BITMAPS::ercwarn }
        };
        return mapping;
    }
};

#endif