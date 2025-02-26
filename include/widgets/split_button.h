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

#ifndef __SPLIT_BUTTON_H__
#define __SPLIT_BUTTON_H__

#include <wx/bmpbndl.h>
#include <wx/panel.h>

class wxButton;
class wxMenu;

class SPLIT_BUTTON : public wxPanel
{
public:
    SPLIT_BUTTON( wxWindow* aParent, wxWindowID aId, const wxString& aLabel,
            const wxPoint& aPos = wxDefaultPosition, const wxSize& aSize = wxDefaultSize );

    ~SPLIT_BUTTON();

    wxMenu* GetSplitButtonMenu();
    void    SetBitmap( const wxBitmapBundle& aBmp );
    void    SetMinSize( const wxSize& aSize ) override;
    void    SetWidthPadding( int aPadding );
    void    SetLabel( const wxString& aLabel ) override;
    bool    Enable( bool aEnable = true ) override;

protected:
    void OnKillFocus( wxFocusEvent& aEvent );
    void OnMouseLeave( wxMouseEvent& aEvent );
    void OnMouseEnter( wxMouseEvent& aEvent );
    void OnLeftButtonUp( wxMouseEvent& aEvent );
    void OnLeftButtonDown( wxMouseEvent& aEvent );
    void OnPaint( wxPaintEvent& WXUNUSED( aEvent ) );
    void onThemeChanged( wxSysColourChangedEvent &aEvent );

private:
    int       m_stateButton = 0;
    int       m_stateMenu   = 0;
    bool      m_bIsEnable   = true;
    int       m_arrowButtonWidth;
    int       m_widthPadding;
    bool      m_bLButtonDown     = false;
    wxString  m_label;
    wxMenu*   m_pMenu = nullptr;
    wxBitmapBundle  m_bitmap;
    wxSize    m_unadjustedMinSize;
};

#endif /*__SPLIT_BUTTON_H__*/
