/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#ifndef SCH_SELECTION_H
#define SCH_SELECTION_H

class SCH_REFERENCE_LIST;
class SCH_SCREEN;
class SCH_SHEET_PATH;

#include <unordered_set>
#include <tool/selection.h>
#include <sch_sheet_path.h> // SCH_MULTI_UNIT_REFERENCE_MAP


class SCH_SELECTION : public SELECTION
{
    /**
     * Screen of selected objects.  Used to fetch library symbols for copy.
     */
    SCH_SCREEN* m_screen;

public:
    SCH_SELECTION( SCH_SCREEN* aScreen = nullptr );

    EDA_ITEM* GetTopLeftItem( bool onlyModules = false ) const override;

    BOX2I GetBoundingBox() const override;

    void SetScreen( SCH_SCREEN* aScreen ) { m_screen = aScreen; }
    SCH_SCREEN* GetScreen() { return m_screen; }

    const std::vector<KIGFX::VIEW_ITEM*> updateDrawList() const override;
};

#endif  //  SCH_SELECTION_H
