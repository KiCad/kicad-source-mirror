/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2016 Anil8735(https://stackoverflow.com/users/3659387/anil8753) from https://stackoverflow.com/a/37274011
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

#ifndef STD_BITMAP_BUTTON_H
#define STD_BITMAP_BUTTON_H

#include <kicommon.h>
#include <wx/bmpbndl.h>
#include <wx/panel.h>

class wxButton;

/**
 * A bitmap button widget that behaves like a standard dialog button except with an icon.
 * Specifically:
 *     * It has a border
 *     * It has no hover/focused state
 *
 * In wxWidgets 3.2 the native button control is used on Mac for wxBitmapButton with or without
 * text.  Said widget has margins that are more than twice what previous versions had.  This class
 * allows our bitmap buttons to match the layout of our SPLIT_BUTTON.
 */

class KICOMMON_API STD_BITMAP_BUTTON : public wxPanel
{
public:
    // For use with wxFormBuilder on a sub-classed wxBitmapButton
    STD_BITMAP_BUTTON( wxWindow* aParent, wxWindowID aId, const wxBitmap& aDummyBitmap,
                       const wxPoint& aPos = wxDefaultPosition, const wxSize& aSize = wxDefaultSize,
                       int aStyle = 0 );

    ~STD_BITMAP_BUTTON();

    void SetBitmap( const wxBitmapBundle& aBmp );
    bool Enable( bool aEnable = true ) override;

protected:
    void OnKillFocus( wxFocusEvent& aEvent );
    void OnMouseLeave( wxMouseEvent& aEvent );
    void OnMouseEnter( wxMouseEvent& aEvent );
    void OnLeftButtonUp( wxMouseEvent& aEvent );
    void OnLeftButtonDown( wxMouseEvent& aEvent );
    void OnPaint( wxPaintEvent& WXUNUSED( aEvent ) );
    void onThemeChanged( wxSysColourChangedEvent &aEvent );

private:
    int       m_stateButton  = 0;
    bool      m_bIsEnable    = true;
    wxBitmapBundle m_bitmap;
};

#endif /*STD_BITMAP_BUTTON_H*/
