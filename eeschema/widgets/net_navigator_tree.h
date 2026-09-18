/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef NET_NAVIGATOR_TREE_H
#define NET_NAVIGATOR_TREE_H

#include <wx/treectrl.h>
#include <wx/generic/treectlg.h>

/**
 * The net navigator's tree control.
 *
 * wxGenericTreeCtrl caches the row height it derives from the control font, but wxMSW swaps in a
 * rescaled font on a DPI change without going through the virtual SetFont() that would recompute
 * it.  This exposes the recomputation so the owner can drive it.
 */
class NET_NAVIGATOR_TREE : public wxGenericTreeCtrl
{
public:
    NET_NAVIGATOR_TREE( wxWindow* aParent, wxWindowID aId, const wxPoint& aPos, const wxSize& aSize,
                        long aStyle ) :
            wxGenericTreeCtrl( aParent, aId, aPos, aSize, aStyle )
    {
    }

    /**
     * Recompute the row height from the control's current font.
     *
     * Text extents are cached on the items, so the tree must also be repopulated for a font size
     * change to take full effect.
     */
    void RecalculateRowHeight() { CalculateLineHeight(); }
};

#endif  // NET_NAVIGATOR_TREE_H
