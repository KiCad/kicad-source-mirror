/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file
 * wxDataViewCtrl helper functions. These are functions that should be methods
 * of wxDataViewCtrl, but aren't.
 */

#ifndef WXDATAVIEWCTRL_HELPERS_H
#define WXDATAVIEWCTRL_HELPERS_H

#include <wx/dataview.h>

/**
 * Get the previous item in list order.
 *
 * @param aView - a wxDataViewCtrl with valid model
 * @param aItem - a valid item in the model
 * @return the item before aItem, or an invalid item if aItem is at the top.
 */
wxDataViewItem GetPrevItem( wxDataViewCtrl const& aView, wxDataViewItem const& aItem );

/**
 * Get the next item in list order.
 *
 * @param aView - a wxDataViewCtrl with valid model
 * @param aItem - a valid item in the model
 * @return the item after aItem, or an invalid item if aItem is at the bottom.
 */
wxDataViewItem GetNextItem( wxDataViewCtrl const& aView, wxDataViewItem const& aItem );

/**
 * Get the previous sibling of an item.
 *
 * @param aView - awxDataViewCtrl with valid model
 * @param aItem - a valid item in the model
 * @return the sibling before aItem, or an invalid item if aItem has no siblings before it.
 */
wxDataViewItem GetPrevSibling( wxDataViewCtrl const& aView, wxDataViewItem const& aItem );

/**
 * Get the next sibling of an item.
 *
 * @param aView - awxDataViewCtrl with valid model
 * @param aItem - a valid item in the model
 * @return the sibling after aItem, or an invalid item if aItem has no siblings after it.
 */
wxDataViewItem GetNextSibling( wxDataViewCtrl const& aView, wxDataViewItem const& aItem );

#endif // WXDATAVIEWCTRL_HELPERS_H
