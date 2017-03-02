/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2017 KiCad Developers.
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

#ifndef MENUS_HELPERS_H_
#define MENUS_HELPERS_H_

/**
 * @file menus_helpers.h
 * @brief Usefull macros and inline functions to create menus items
 * in menubars or popup menus
 */


#include <wx/menu.h>
#include <wx/menuitem.h>
#include <bitmaps.h>



/**
 * Function AddMenuItem
 * is an inline helper function to create and insert a menu item with an icon
 * into \a aMenu
 *
 * @param aMenu is the menu to add the new item.
 * @param aId is the command ID for the new menu item.
 * @param aText is the string for the new menu item.
 * @param aImage is the icon to add to the new menu item.
 * @param aType is the type of menu :wxITEM_NORMAL (default), wxITEM_CHECK ...
 * @return a pointer to the new created wxMenuItem
 */
wxMenuItem* AddMenuItem( wxMenu* aMenu, int aId, const wxString&  aText,
                         const wxBitmap&  aImage, wxItemKind aType = wxITEM_NORMAL );


/**
 * Function AddMenuItem
 * is an inline helper function to create and insert a menu item with an icon
 * and a help message string into \a aMenu
 *
 * @param aMenu is the menu to add the new item.
 * @param aId is the command ID for the new menu item.
 * @param aText is the string for the new menu item.
 * @param aHelpText is the help message string for the new menu item.
 * @param aImage is the icon to add to the new menu item.
 * @param aType is the type of menu :wxITEM_NORMAL (default), wxITEM_CHECK ...
 * @return a pointer to the new created wxMenuItem
 */
wxMenuItem* AddMenuItem( wxMenu* aMenu, int aId, const wxString&  aText,
                         const wxString& aHelpText, const wxBitmap& aImage,
                         wxItemKind aType = wxITEM_NORMAL );


/**
 * Function AddMenuItem
 * is an inline helper function to create and insert a menu item with an icon
 * into \a aSubMenu in \a aMenu
 *
 * @param aMenu is the menu to add the new submenu item.
 * @param aSubMenu is the submenu to add the new menu.
 * @param aId is the command ID for the new menu item.
 * @param aText is the string for the new menu item.
 * @param aImage is the icon to add to the new menu item.
 * @return a pointer to the new created wxMenuItem
 */
wxMenuItem* AddMenuItem( wxMenu* aMenu, wxMenu* aSubMenu, int aId,
                         const wxString& aText, const wxBitmap& aImage );


/**
 * Function AddMenuItem
 * is an inline helper function to create and insert a menu item with an icon
 * and a help message string into \a aSubMenu in \a aMenu
 *
 * @param aMenu is the menu to add the new submenu item.
 * @param aSubMenu is the submenu to add the new menu.
 * @param aId is the command ID for the new menu item.
 * @param aText is the string for the new menu item.
 * @param aHelpText is the help message string for the new menu item.
 * @param aImage is the icon to add to the new menu item.
 * @return a pointer to the new created wxMenuItem
 */
wxMenuItem* AddMenuItem( wxMenu* aMenu, wxMenu* aSubMenu, int aId,
                         const wxString& aText, const wxString& aHelpText,
                         const wxBitmap&  aImage );

#endif // MENUS_HELPERS_H_
