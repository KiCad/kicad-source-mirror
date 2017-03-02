/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2017 KiCad Developers, see change_log.txt for contributors.
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


#include <wx/image.h>
#include <wx/bitmap.h>
#include <wx/mstream.h>
#include <wx/menu.h>
#include <wx/menuitem.h>

#include <common.h>
#include <bitmaps.h>
#include <pgm_base.h>

wxBitmap KiBitmap( BITMAP_DEF aBitmap )
{
    wxMemoryInputStream is( aBitmap->png, aBitmap->byteCount );
    wxImage image( is, wxBITMAP_TYPE_PNG );
    wxBitmap bitmap( image );

    return bitmap;
}


wxBitmap* KiBitmapNew( BITMAP_DEF aBitmap )
{
    wxMemoryInputStream is( aBitmap->png, aBitmap->byteCount );
    wxImage image( is, wxBITMAP_TYPE_PNG );
    wxBitmap* bitmap = new wxBitmap( image );

    return bitmap;
}

wxMenuItem* AddMenuItem( wxMenu* aMenu, int aId, const wxString& aText,
                         const wxBitmap&  aImage, wxItemKind aType = wxITEM_NORMAL )
{
    wxMenuItem* item;

    item = new wxMenuItem( aMenu, aId, aText, wxEmptyString, aType );

    // Retrieve the global applicaton show icon option:
    bool useImagesInMenus = Pgm().GetUseIconsInMenus();

    if( useImagesInMenus )
    {
        if( aType == wxITEM_CHECK )
        {
    #if defined(  __WINDOWS__ )
            item->SetBitmaps( KiBitmap( checked_ok_xpm ), aImage );
            // A workaround to a strange bug on Windows, wx Widgets 3.0:
            // size of bitmaps is not taken in account for wxITEM_CHECK menu
            // unless we call SetFont
            item->SetFont(*wxNORMAL_FONT);
    #endif
        }
        else
            item->SetBitmap( aImage );
    }

    aMenu->Append( item );

    return item;
}

wxMenuItem* AddMenuItem( wxMenu* aMenu, int aId, const wxString& aText,
                         const wxString& aHelpText, const wxBitmap& aImage,
                         wxItemKind aType = wxITEM_NORMAL )
{
    wxMenuItem* item;

    item = new wxMenuItem( aMenu, aId, aText, aHelpText, aType );

    // Retrieve the global applicaton show icon option:
    bool useImagesInMenus = Pgm().GetUseIconsInMenus();

    if( useImagesInMenus )
    {
        if( aType == wxITEM_CHECK )
        {
    #if defined(  __WINDOWS__ )
            item->SetBitmaps( KiBitmap( checked_ok_xpm ), aImage );
            // A workaround to a strange bug on Windows, wx Widgets 3.0:
            // size of bitmaps is not taken in account for wxITEM_CHECK menu
            // unless we call SetFont
            item->SetFont(*wxNORMAL_FONT);
    #endif
        }
        else
            item->SetBitmap( aImage );
    }

    aMenu->Append( item );

    return item;
}

wxMenuItem* AddMenuItem( wxMenu* aMenu, wxMenu* aSubMenu, int aId,
                         const wxString& aText, const wxBitmap& aImage )
{
    wxMenuItem* item;

    item = new wxMenuItem( aMenu, aId, aText );
    item->SetSubMenu( aSubMenu );

    // Retrieve the global applicaton show icon option:
    bool useImagesInMenus = Pgm().GetUseIconsInMenus();

    if( useImagesInMenus )
        item->SetBitmap( aImage );

    aMenu->Append( item );

    return item;
};


wxMenuItem* AddMenuItem( wxMenu* aMenu, wxMenu* aSubMenu, int aId,
                         const wxString& aText, const wxString& aHelpText,
                         const wxBitmap&  aImage )
{
    wxMenuItem* item;

    item = new wxMenuItem( aMenu, aId, aText, aHelpText );
    item->SetSubMenu( aSubMenu );

    // Retrieve the global applicaton show icon option:
    bool useImagesInMenus = Pgm().GetUseIconsInMenus();

    if( useImagesInMenus )
        item->SetBitmap( aImage );

    aMenu->Append( item );

    return item;
};
