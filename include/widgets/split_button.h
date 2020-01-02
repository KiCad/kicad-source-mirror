/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2016 Anil8735(https://stackoverflow.com/users/3659387/anil8753) from https://stackoverflow.com/a/37274011
 *
 * This file is shared under the CC BY-SA 4.0 License
 * https://creativecommons.org/licenses/by-sa/4.0/
 */

#ifndef __SPLIT_BUTTON_H__
#define __SPLIT_BUTTON_H__

#include <wx/menu.h>
#include <wx/wx.h>

class SPLIT_BUTTON : public wxPanel
{
public:
    SPLIT_BUTTON( wxWindow* aParent, wxWindowID aId, const wxString& aLabel,
            const wxPoint& aPos = wxDefaultPosition, const wxSize& aSize = wxDefaultSize );

    ~SPLIT_BUTTON();

    wxMenu* GetSplitButtonMenu();
    void    SetBitmap( const wxBitmap& aBmp );
    void    SetMinSize( const wxSize& aSize ) override;
    void    SetWidthPadding( int padding );

protected:
    void OnKillFocus( wxFocusEvent& aEvent );
    void OnMouseLeave( wxMouseEvent& aEvent );
    void OnMouseEnter( wxMouseEvent& aEvent );
    void OnLeftButtonUp( wxMouseEvent& aEvent );
    void OnLeftButtonDown( wxMouseEvent& aEvent );
    void OnPaint( wxPaintEvent& WXUNUSED( aEvent ) );

    bool Enable( bool aEnable = true ) override;

private:
    int       m_stateButton = 0;
    int       m_stateMenu   = 0;
    bool      m_bIsEnable   = true;
    wxColor   m_colorNormal;
    wxColor   m_colorDisabled;
    const int m_arrowButtonWidth = 20;
    int       m_widthPadding     = 20;
    bool      m_bLButtonDown     = false;
    wxString  m_label;
    wxMenu*   m_pMenu = nullptr;
    wxBitmap  m_bitmap;
    wxSize    unAdjustedMinSize;
};

#endif /*__SPLIT_BUTTON_H__*/