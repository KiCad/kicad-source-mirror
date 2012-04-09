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
#  define SETBITMAPS( aImage ) item->SetBitmaps( KiBitmap( apply_xpm ), KiBitmap( aImage ) )
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
        item->SetBitmaps( KiBitmap( apply_xpm ), aImage );
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
        item->SetBitmaps( KiBitmap( apply_xpm ), aImage );
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
