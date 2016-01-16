/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2014 KiCad Developers.
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

#include <bitmaps.h>

/**
 * Definition SETBITMAPS
 * is a macro use to add a bitmaps to check menu item.
 * @note Do not use with normal menu items or any platform other than Windows.
 * @param aImage is the image to add the menu item.
 */
#if defined( USE_IMAGES_IN_MENUS ) && defined(  __WINDOWS__ )
#  define SETBITMAPS( aImage ) item->SetBitmaps( KiBitmap( checked_ok_xpm ), KiBitmap( aImage ) )
#else
#  define SETBITMAPS( aImage )
#endif

/**
 * Definition SETBITMAP
 * is a macro use to add a bitmap to a menu items.
 * @note Do not use with checked menu items.
 * @param aImage is the image to add the menu item.
 */
#if !defined( USE_IMAGES_IN_MENUS )
#  define SET_BITMAP( aImage )
#else
#  define SET_BITMAP( aImage ) item->SetBitmap( aImage )
#endif

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
static inline wxMenuItem* AddMenuItem( wxMenu*          aMenu,
                                       int              aId,
                                       const wxString&  aText,
                                       const wxBitmap&  aImage,
                                       wxItemKind       aType = wxITEM_NORMAL )
{
    wxMenuItem* item;

    item = new wxMenuItem( aMenu, aId, aText, wxEmptyString, aType );

    if( aType == wxITEM_CHECK )
    {
#if defined( USE_IMAGES_IN_MENUS ) && defined(  __WINDOWS__ )
        item->SetBitmaps( KiBitmap( checked_ok_xpm ), aImage );
        // A workaround to a strange bug on Windows, wx Widgets 3.0:
        // size of bitmaps is not taken in account for wxITEM_CHECK menu
        // unless we call SetFont
        item->SetFont(*wxNORMAL_FONT);
#endif
    }
    else
    {
        SET_BITMAP( aImage );
    }

    aMenu->Append( item );

    return item;
}


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
static inline wxMenuItem* AddMenuItem( wxMenu*          aMenu,
                                       int              aId,
                                       const wxString&  aText,
                                       const wxString&  aHelpText,
                                       const wxBitmap&  aImage,
                                       wxItemKind       aType = wxITEM_NORMAL )
{
    wxMenuItem* item;

    item = new wxMenuItem( aMenu, aId, aText, aHelpText, aType );

    if( aType == wxITEM_CHECK )
    {
#if defined( USE_IMAGES_IN_MENUS ) && defined(  __WINDOWS__ )
        item->SetBitmaps( KiBitmap( checked_ok_xpm ), aImage );
        // A workaround to a strange bug on Windows, wx Widgets 3.0:
        // size of bitmaps is not taken in account for wxITEM_CHECK menu
        // unless we call SetFont
        item->SetFont(*wxNORMAL_FONT);
#endif
    }
    else
    {
        SET_BITMAP( aImage );
    }

    aMenu->Append( item );

    return item;
}


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
static inline wxMenuItem* AddMenuItem( wxMenu*          aMenu,
                                       wxMenu*          aSubMenu,
                                       int              aId,
                                       const wxString&  aText,
                                       const wxBitmap&  aImage )
{
    wxMenuItem* item;

    item = new wxMenuItem( aMenu, aId, aText );
    item->SetSubMenu( aSubMenu );

    SET_BITMAP( aImage );

    aMenu->Append( item );

    return item;
};


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
static inline wxMenuItem* AddMenuItem( wxMenu*          aMenu,
                                       wxMenu*          aSubMenu,
                                       int              aId,
                                       const wxString&  aText,
                                       const wxString&  aHelpText,
                                       const wxBitmap&  aImage )
{
    wxMenuItem* item;

    item = new wxMenuItem( aMenu, aId, aText, aHelpText );
    item->SetSubMenu( aSubMenu );

    SET_BITMAP( aImage );

    aMenu->Append( item );

    return item;
};

#endif // MENUS_HELPERS_H_
