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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef WX_DATAVIEWCTRL_H_
#define WX_DATAVIEWCTRL_H_

#include <wx/dataview.h>

/**
 * Extension of the wxDataViewCtrl to include some helper functions for working with items.
 *
 * These should probably be sent upstream, since they may be useful to others, but for now
 * just extend the class with them ourselves.
 */
class WX_DATAVIEWCTRL : public wxDataViewCtrl
{
public:
    // Just take all constructors
    using wxDataViewCtrl::wxDataViewCtrl;

    /**
     * Get the previous item in list order.
     *
     * @param aItem a valid item in the control's model.
     * @return the item before aItem, or an invalid item if aItem is at the top.
     */
    wxDataViewItem GetPrevItem( wxDataViewItem const& aItem );

    /**
     * Get the next item in list order.
     *
     * @param aItem a valid item in the control's model.
     * @return the item after aItem, or an invalid item if aItem is at the bottom.
     */
    wxDataViewItem GetNextItem( wxDataViewItem const& aItem );

    /**
     * Get the previous sibling of an item.
     *
     * @param aItem a valid item in the control's model.
     * @return the sibling before aItem, or an invalid item if aItem has no siblings before it.
     */
    wxDataViewItem GetPrevSibling( wxDataViewItem const& aItem );

    /**
     * Get the next sibling of an item.
     *
     * @param aItem a valid item in the control's model.
     * @return the sibling after aItem, or an invalid item if aItem has no siblings after it.
     */
    wxDataViewItem GetNextSibling( wxDataViewItem const& aItem );

    void DoSetToolTipText( const wxString &tip ) override {}

    void ExpandAll();
    void CollapseAll();
};

#endif // WX_DATAVIEWCTRL_H_
